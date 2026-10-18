/*!
 * \file CNativeField3D.hpp
 * \brief Immutable original sensor-only tetrahedral P1 queries and bounded local reuse.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeMesh3D.hpp"
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

class CADTElemClass;
namespace SU2Native3D {
class PlanarReference;
constexpr size_t MaximumDonors = MaximumSegmentDonors, MaximumSensorSamples = 2048, MaximumGeometricEdgeCuts = 65536;
// Imported records/indexes, sensor cache, exact segment scratch and global edge heap/growth; excludes CFD/RSS.
constexpr size_t MaximumFrozenFieldBytes = MaximumDonors * 12288 + MaximumSensorSamples * 256 + MaximumGeometricEdgeCuts * (sizeof(double)+64);
/*! Identity of one immutable original sensor, geometry, composition and edge-accuracy policy.
 * Share only between complete imports of that same field; a new normalization stage needs a new identity. */
struct FrozenMetricIdentity {};
using Metric = std::function<Tensor(Point)>;
using MetricComposition = std::function<Tensor(Point, Tensor)>;
using MetricEdgeBreakpoints = std::function<std::vector<double>(Point, Point)>;
struct DonorCell {
  Cell cell;
  std::array<Tensor, 4> sensor;
  Id extension_component = UINT64_MAX; // Original source, or explicitly declared finite extension component.
  Id parent_cell = UINT64_MAX; // Generated exterior records migrate with their CFD source cell.
  template <class Stream>
  void Fields(Stream& s) {
    s(cell, sensor, extension_component, parent_cell);
  }
  std::array<Id, 4> Key() const;
};
/*! Associates only received exterior components present in the caller's declared geometry authority. */
std::vector<Id> AssociatedExtensions(const std::vector<DonorCell>& source,const PlanarReference& reference);
/*! Original P1 arithmetic shared by local and owner-routed queries; weights follow donor.cell.v. */
Tensor InterpolateSensor(const DonorCell& donor,const std::array<long double,4>& weights);
struct FieldSample {
  Tensor sensor, target;
  std::array<Id, 4> donor{};
  std::array<long double, 4> weights{};
  Id extension_component = UINT64_MAX;
};
struct FieldStats {
  uint64_t requests = 0, invalid_queries = 0, hits = 0, misses = 0, failures = 0, evictions = 0;
  uint64_t box_candidates = 0, containment_tests = 0, interpolations = 0, compositions = 0;
  uint64_t donor_hint_tests=0,donor_hint_hits=0;
  size_t maximum_candidates = 0, maximum_cache_entries = 0, index_bytes = 0;
  double build_seconds = 0, query_seconds = 0, cache_seconds = 0;
  double search_seconds = 0, interpolation_seconds = 0, composition_seconds = 0;
  uint64_t edge_requests = 0, edge_failures = 0, edge_box_candidates = 0, edge_pieces = 0;
  uint64_t edge_containment_tests = 0, edge_direct_intervals = 0, edge_compositions = 0;
  uint64_t extended_queries = 0, extended_edge_pieces = 0;
  size_t maximum_edge_candidates = 0, maximum_edge_pieces = 0;
  double edge_seconds = 0, edge_search_seconds = 0, edge_trace_seconds = 0, edge_integral_seconds = 0;
  double edge_geometry_seconds = 0, edge_composition_seconds = 0;  // Nested in edge_integral_seconds.
  KernelStats kernel;
};
/*! Bounded imported original donor patch, not an evolving-mesh metric. One instance per private worker.
 * Canonical original-node keys resolve containment ties; ADT is only a conservative box broad phase.
 * Only sensor samples are cached. Composition runs at each actual query, including cache hits.
 * Exact containment only. Original physical P1 wins over declared extension cells at every covered query.
 * Extensions require an explicitly associated component before freezing; their generation/admission belongs
 * to the import layer. No point-query fallback or original-target redefinition occurs during reconstruction. */
class FrozenField {
 public:
  explicit FrozenField(std::vector<DonorCell> original, MetricComposition composition = {}, bool timings = false,
                       std::vector<Id> extension_components = {}, MetricEdgeBreakpoints edge_breakpoints = {},
                       std::shared_ptr<const FrozenMetricIdentity> identity = {}, bool reuse_interior = true);
  ~FrozenField();
  FieldSample Query(Point point);
  /*! Sensor P1 at the ideal endpoint midpoint; BL at its represented coordinate. Never populates Query's point cache. */
  FieldSample QueryMidpoint(Point a,Point b);
  Tensor evaluate(Point point) { return Query(point).target; }
  /*! Integrates SENSOR ONLY through exact original/declared-extension intervals; original physical P1 takes priority.
   * Composition is deliberately excluded.
   * Composed-target acceptance requires separate geometric breakpoints/integration, never this sensor audit alone. */
  long double SensorEdgeLength(Point a, Point b);
  /*! Actual-target integral through exact source intervals and geometric distance bands.
   * Composed fields without declared geometric breakpoints reject; sensor-only fields stay analytic. */
  long double TargetEdgeLength(Point a, Point b);
  /*! Full-target length-balanced coordinate proposal; geometry kernels still certify the rounded point. */
  Point BalancedSplitPoint(Point a, Point b, double fraction = .5);
  /*! A source-contained corner of the exact affine rounding box, nearest corner first.
   * At most8 candidates; no snapping/extrapolation of ordinary point queries. Geometry gates remain separate. */
  Point SegmentPoint(Point a,Point b,double parameter);
  bool SensorOnly() const { return !composition; }
  void RequireEdgePolicy() const;
  const std::shared_ptr<const FrozenMetricIdentity>& Identity() const { return identity; }
  const FieldStats& Statistics() const { return stats; }

 private:
  long double EdgeLength(Point a, Point b, bool composed);
  void FindCandidates(Point lo, Point hi);
  struct Hash {
    size_t operator()(const std::array<double, 3>& point) const;
  };
  struct SensorValue {
    Tensor tensor;
    size_t donor = 0;
    std::array<long double, 4> weights{};
  };
  void CacheSensor(const std::array<double,3>& key,const SensorValue& value);
  std::vector<DonorCell> donors;
  const MetricComposition composition;
  const MetricEdgeBreakpoints edge_breakpoints;
  const bool timings,reuse_interior;
  size_t last_donor=SIZE_MAX;  // A hypothesis only; strict physical interior containment is checked at every new point.
  const std::shared_ptr<const FrozenMetricIdentity> identity;
  std::unique_ptr<CADTElemClass> search;
  std::vector<unsigned long> candidates;
  std::unordered_map<std::array<double, 3>, SensorValue, Hash> cache;
  std::deque<std::array<double, 3>> recent;
  FieldStats stats;
};
}  // namespace SU2Native3D
