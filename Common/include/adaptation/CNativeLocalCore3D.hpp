/*!
 * \file CNativeLocalCore3D.hpp
 * \brief Deterministic, validity-first local tetrahedral adaptation with a fixed boundary.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once

#include "CNativeDistributed3D.hpp"
#include "CNativeMetric3D.hpp"
#include <array>
#include <cstdint>
#include <vector>

namespace SU2NativeLocal3D {
using SU2Native3D::FacetBinding;
using SU2Native3D::Id;
using SU2Native3D::MetricMeasures;
using SU2Native3D::Node;
using SU2Native3D::Owned;
using SU2Native3D::Point;
using SU2Native3D::Tensor;

using Slot = uint32_t;
using Vertices = std::array<Slot, 4>;
using Edge = std::array<Slot, 2>;
using Face = std::array<Slot, 3>;
constexpr Slot NoSlot = UINT32_MAX;
constexpr int32_t Physical = -1;
constexpr int32_t Border = -2;
constexpr double LongEdge = SU2Native3D::MaximumMetricEdge * (1 - 1e-12);
constexpr double ShortEdge = SU2Native3D::CoarseningMetricEdge;
constexpr double BadMargin = 1 + 1e-12;

struct PointRow {
  Node node;
  Tensor tensor;
  Slot incident = NoSlot;
  bool surface = false, border = false, removed = false, wall_near = false;
  bool Free() const { return !surface && !border && !removed; }
};

struct CellRow {
  Vertices vertices{};
  std::array<int32_t, 4> neighbors{};
  Id id = 0;
  uint64_t version = 0;
  int32_t binding = -1;
  uint32_t stamp = 0;
  bool live = true, changed = false;
};

struct EdgeUse {
  Edge edge;
  Slot cell;
};

enum class Refusal { None, Ineligible, Validity, Size, Topology, Memory };

struct PassStats {
  size_t cells_in = 0, cells_out = 0, points_in = 0, points_out = 0;
  size_t surface_points = 0, border_points = 0, wall_near_points = 0;
  size_t collapses = 0, splits = 0, edge_removals = 0, face_swaps = 0, moves = 0;
  size_t deferred = 0, refused_valid = 0, refused_size = 0, refused_topology = 0, refused_memory = 0;
  size_t long_left_surface = 0, long_left_border = 0, short_out = 0, edges_out = 0;
  size_t bad_in = 0, bad_out = 0, bad_out_free = 0;
  unsigned sweeps_collapse = 0, sweeps_split = 0, sweeps_quality = 0, rounds = 0;
  bool limit = false;
  bool report = true;

  template <class CountType>
  void Increment(CountType& value) {
    if (report) ++value;
  }
  void Count(Refusal refusal);
};

/*! Each rank owns a disjoint progression above the greatest input id. */
class IdSequence {
 public:
  explicit IdSequence(Id base, int rank = 0, int ranks = 1);
  Id Peek() const;
  Id Take();

 private:
  Id last_, stride_, ranks_;
};

/*! One pass owns its arrays. Slots may be reused, while point ids are never reused. */
class Mesh {
 public:
  explicit Mesh(const Owned& owned, bool allow_border = false);

  const std::vector<PointRow>& Points() const { return points_; }
  const std::vector<CellRow>& Cells() const { return cells_; }
  std::array<FacetBinding, 4> Bindings(Slot cell) const;
  std::vector<EdgeUse> Edges() const;
  bool ClosedRing(Edge edge, Slot seed) const;
  void MarkWallNear();

  double Length(Edge edge) const;
  MetricMeasures Shape(Vertices vertices) const;
  double Margin(Vertices vertices) const;
  void Check() const;
  void WriteBack(Owned& owned, IdSequence& ids) const;

  Refusal ReplaceCells(const std::vector<Slot>& old, const std::vector<Vertices>& fresh, uint32_t stamp);
  Refusal Collapse(Edge edge, uint32_t stamp, PassStats& stats, bool apply = true);
  Refusal Split(Edge edge, Slot seed, IdSequence& point_ids, uint32_t stamp, size_t cell_limit, PassStats& stats);
  Refusal RemoveEdge(Edge edge, Slot seed, uint32_t stamp, size_t cell_limit, PassStats& stats);
  Refusal SwapFace(Slot cell, unsigned face, uint32_t stamp, size_t cell_limit, PassStats& stats);
  Refusal MovePoint(Slot point, IdSequence& ids, uint32_t stamp, PassStats& stats);

  size_t CollapseSweeps(PassStats& stats);
  void LengthWork(IdSequence& point_ids, size_t cell_limit, PassStats& stats);
  void QualityWork(IdSequence& point_ids, size_t cell_limit, PassStats& stats);
  PassStats RunPass(IdSequence& point_ids, size_t cell_limit, bool report);

  static size_t ArrayBytes(size_t cells);
  static const size_t BytesPerCell;

 private:
  struct Ring {
    std::vector<Slot> cells;
    bool surface = false, border = false;
    bool Closed() const { return !cells.empty() && !surface && !border; }
  };

  struct Link {
    int32_t cell = Border;
    unsigned face = 0;
    bool fresh = false;
  };

  struct FreshCell {
    Vertices vertices;
    std::array<Link, 4> links{};
    std::array<FacetBinding, 4> bindings{};
  };

  struct Plan {
    std::vector<Slot> old;
    std::vector<FreshCell> fresh;
    Refusal refusal = Refusal::Ineligible;
    double score = 0;
    bool Accepted() const { return refusal == Refusal::None; }
  };

  void BuildFaces(bool allow_border);
  bool Positive(Vertices vertices) const;
  Edge ById(Edge edge) const;
  std::array<Id, 4> Key(Vertices vertices) const;
  std::vector<Slot> Star(Slot point) const;
  Ring EdgeRing(Edge edge, Slot seed) const;
  std::vector<Slot> OrderedRim(Edge edge, const Ring& ring) const;
  bool CanCollapse(Slot point) const;
  bool Fits(size_t old, size_t fresh, size_t cell_limit) const;
  double Worst(const std::vector<Slot>& cells) const;

  Plan Prepare(const std::vector<Slot>& old, const std::vector<Vertices>& fresh) const;
  Plan CollapsePlan(Slot remove, Slot keep) const;
  Plan RemoveEdgePlan(Edge edge, Slot seed, size_t cell_limit) const;
  Plan SwapFacePlan(Slot cell, unsigned face, size_t cell_limit) const;
  void Commit(const Plan& plan, uint32_t stamp);
  void MarkChanged(CellRow& row, uint32_t stamp);
  int32_t SaveBindings(const std::array<FacetBinding, 4>& binding);
  size_t SplitSweeps(IdSequence& point_ids, size_t cell_limit, PassStats& stats);
  bool QualitySweep(IdSequence& point_ids, size_t cell_limit, PassStats& stats);
  void ReportResult(PassStats& stats) const;

  struct FrozenPoint {
    Slot slot;
    Node node;
    Tensor tensor;
  };

  std::vector<PointRow> points_;
  std::vector<CellRow> cells_;
  std::vector<std::array<FacetBinding, 4>> bindings_;
  std::vector<Slot> free_cells_, free_points_, free_bindings_;
  std::vector<Id> erased_ids_;
  std::vector<FrozenPoint> frozen_;
  size_t live_cells_ = 0;
  uint32_t sweep_ = 0;
};
}  // namespace SU2NativeLocal3D
