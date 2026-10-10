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
constexpr size_t MaximumDonors = MaximumSegmentDonors, MaximumSensorSamples = 2048;
using Metric = std::function<Tensor(Point)>;
using MetricComposition = std::function<Tensor(Point, Tensor)>;
struct DonorCell {
  Cell cell;
  std::array<Tensor, 4> sensor;
  template <class Stream>
  void Fields(Stream& s) {
    s(cell, sensor);
  }
  std::array<Id, 4> Key() const;
};
struct FieldSample {
  Tensor sensor, target;
  std::array<Id, 4> donor{};
  std::array<long double, 4> weights{};
};
struct FieldStats {
  uint64_t requests = 0, invalid_queries = 0, hits = 0, misses = 0, failures = 0, evictions = 0;
  uint64_t box_candidates = 0, containment_tests = 0, interpolations = 0, compositions = 0;
  size_t maximum_candidates = 0, maximum_cache_entries = 0, index_bytes = 0;
  double build_seconds = 0, query_seconds = 0, cache_seconds = 0;
  double search_seconds = 0, interpolation_seconds = 0, composition_seconds = 0;
  uint64_t edge_requests = 0, edge_failures = 0, edge_box_candidates = 0, edge_pieces = 0;
  uint64_t edge_containment_tests = 0, edge_direct_intervals = 0;
  size_t maximum_edge_candidates = 0, maximum_edge_pieces = 0;
  double edge_seconds = 0, edge_search_seconds = 0, edge_trace_seconds = 0, edge_integral_seconds = 0;
  KernelStats kernel;
};
/*! Bounded imported original donor patch, not an evolving-mesh metric. One instance per private worker.
 * Canonical original-node keys resolve containment ties; ADT is only a conservative box broad phase.
 * Only sensor samples are cached. Composition runs at each actual query, including cache hits.
 * Exact containment only: physical-reference extension/roundoff admission belongs to the import layer. */
class FrozenField {
 public:
  explicit FrozenField(std::vector<DonorCell> original, MetricComposition composition = {}, bool timings = false);
  ~FrozenField();
  FieldSample Query(Point point);
  Tensor evaluate(Point point) { return Query(point).target; }
  /*! Integrates ORIGINAL SENSOR ONLY through exact source-cell intervals; composition is deliberately excluded.
   * Composed-target acceptance requires separate geometric breakpoints/integration, never this sensor audit alone. */
  long double SensorEdgeLength(Point a, Point b);
  const FieldStats& Statistics() const { return stats; }

 private:
  void FindCandidates(Point lo, Point hi);
  struct Hash {
    size_t operator()(const std::array<double, 3>& point) const;
  };
  struct SensorValue {
    Tensor tensor;
    size_t donor = 0;
    std::array<long double, 4> weights{};
  };
  std::vector<DonorCell> donors;
  const MetricComposition composition;
  const bool timings;
  std::unique_ptr<CADTElemClass> search;
  std::vector<unsigned long> candidates;
  std::unordered_map<std::array<double, 3>, SensorValue, Hash> cache;
  std::deque<std::array<double, 3>> recent;
  FieldStats stats;
};
}  // namespace SU2Native3D
