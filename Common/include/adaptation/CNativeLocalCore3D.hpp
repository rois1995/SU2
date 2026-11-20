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
#include <functional>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

namespace SU2NativeLocal3D {
using SU2Native3D::DonorCell;
using SU2Native3D::FacetBinding;
using SU2Native3D::Id;
using SU2Native3D::MetricMeasures;
using SU2Native3D::Node;
using SU2Native3D::Owned;
using SU2Native3D::Point;
using SU2Native3D::Tensor;
using SU2Native3D::World;

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
constexpr unsigned PassCount = 2;

struct CoreOptions {
  bool memory = true;
  bool shape_test = true;
  bool float_trials = true;
  bool surface_first = true;
  bool work_list = true;
  bool split_progress = true;
  double short_edge = ShortEdge;
};

struct PointRow {
  Node node;
  Tensor tensor;
  Slot incident = NoSlot;
  bool surface = false, border = false, removed = false, wall_near = false, unresolved = false;
  uint64_t changed_at = 0;
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
  uint64_t failed_at = 0;
};

struct EdgeUse {
  Edge edge;
  Slot cell;
};

enum class Refusal { None, Ineligible, Validity, Size, Topology, Memory, Numeric, Shape, Blocked };

bool SampleMovedTensor(const std::vector<DonorCell>& donors, Point point, Tensor& tensor);
bool SampleMovedTensorFloat(const std::vector<DonorCell>& donors, Point point, Tensor& tensor, size_t* chosen = nullptr);
bool EvenOrder(const Vertices& cell, Vertices order);
std::vector<Id> UnresolvedSurfacePoints(const Owned& owned);

struct SweepCounters {
  size_t collapses = 0, splits = 0, edge_removals = 0, face_swaps = 0, moves = 0;
  size_t deferred = 0, skipped_memory = 0, blocked = 0;
  size_t refused_numeric = 0, refused_valid = 0, refused_size = 0, refused_topology = 0, refused_memory = 0,
         refused_shape = 0, refused_exact = 0;
  size_t evaluated_collapse = 0, evaluated_split = 0, evaluated_quality = 0;
};

struct SweepRow {
  const char* kind;
  unsigned round, sweep;
  size_t candidates, live_cells, bad_listed;
  double seconds;
  SweepCounters counters;
};

void PrintSweepFields(std::ostream& output, const SweepRow& row);

struct PassStats : SweepCounters {
  size_t cells_in = 0, cells_out = 0, points_in = 0, points_out = 0;
  size_t surface_points = 0, border_points = 0, wall_near_points = 0;
  double seconds_build = 0, seconds_collapse = 0, seconds_split = 0, seconds_quality = 0, seconds_check = 0,
         seconds_write = 0;
  std::string round_changes, quality_changes;
  size_t long_left_surface = 0, long_left_border = 0, short_out = 0, edges_out = 0;
  size_t bad_in = 0, bad_out = 0, bad_out_free = 0;
  unsigned sweeps_collapse = 0, sweeps_split = 0, sweeps_quality = 0, rounds = 0;
  bool limit = false;
  bool report = true;
  std::function<void(const SweepRow&)> sink;

  template <class CountType>
  void Increment(CountType& value) {
    if (report) ++value;
  }
  void Count(Refusal refusal);
  void CountExact() {
    Increment(refused_valid);
    Increment(refused_exact);
  }
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

/*! The worker supplies the retained storage and the production Owned row charge. */
struct MemoryBudget {
  size_t ceiling, caller_bytes, owned_row_bytes, maximum_bytes = 0;
  size_t OwnedBytes(size_t cells) const;
  void Require(const World& world, size_t bytes, const char* reason);
};

struct BorderPoint {
  Id id = 0;
  int lower = 0, upper = 0;
  bool surface = false, unresolved = false;

  template <class Stream>
  void Fields(Stream& stream) {
    stream(id, lower, upper, surface, unresolved);
  }
};

struct BorderMeeting {
  std::vector<BorderPoint> points;
  std::vector<Id> rendezvous;
  const BorderPoint* Find(Id id) const;
  size_t Bytes() const;
};

enum class Direction { Lower, Higher };

struct ShiftStats {
  size_t cells_before = 0, cells_after = 0, sent = 0, received = 0;
  size_t border_points = 0, frozen_in_both = 0;
};

BorderMeeting MeetBorderPoints(World& world, const Owned& owned, MemoryBudget& budget, size_t retained = 0,
                               bool report = true);
ShiftStats ShiftBorders(World& world, Owned& owned, const BorderMeeting& meeting, Direction direction,
                        MemoryBudget& budget, const BorderMeeting* previous = nullptr, bool report = true);

/*! One pass owns its arrays. Slots may be reused, while point ids are never reused. */
class Mesh {
 public:
  explicit Mesh(const Owned& owned, bool allow_border = false, CoreOptions options = {});

  const std::vector<PointRow>& Points() const { return points_; }
  const std::vector<CellRow>& Cells() const { return cells_; }
  std::array<FacetBinding, 4> Bindings(Slot cell) const;
  std::vector<EdgeUse> Edges() const;
  bool ClosedRing(Edge edge, Slot seed) const;
  void MarkWallNear();
  void MarkBorderSurface(const BorderMeeting& meeting);

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
  Refusal MovePoint(Slot point, IdSequence& ids, uint32_t stamp, PassStats& stats, bool* exact_refusal = nullptr);

  size_t CollapseSweeps(PassStats& stats);
  void LengthWork(IdSequence& point_ids, size_t cell_limit, PassStats& stats);
  void QualityWork(IdSequence& point_ids, size_t cell_limit, PassStats& stats);
  PassStats RunPass(IdSequence& point_ids, size_t cell_limit, bool report, CoreOptions options = {});
  PassStats RunPass(IdSequence& point_ids, size_t cell_limit, PassStats stats, CoreOptions options = {});

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
  bool FloatPositive(Vertices vertices) const;
  bool TrialPositive(Vertices vertices) const;
  bool ExactlyPositive(const Plan& plan) const;
  static bool TrialEdgeShorter(const PointRow& a, const PointRow& b, double limit);
  bool Blocked(Vertices vertices) const;
  bool Blocked(const std::vector<Slot>& cells) const;
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
  void RememberWorkPoints(const Plan& plan);
  std::vector<EdgeUse> NextEdges(const std::vector<Edge>& deferred);

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
  CoreOptions options_;
  uint64_t change_ = 1;
  std::map<std::array<Id, 2>, uint64_t> collapse_refusals_;
  uint64_t work_start_ = 0;
  std::vector<Slot> work_points_;
};
}  // namespace SU2NativeLocal3D
