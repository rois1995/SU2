/*!
 * \file CNativeMetric3D.hpp
 * \brief Frozen-target private progress checks and strict final tetrahedral metric gates.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeField3D.hpp"
#include "CNativeCavity3D.hpp"
#include <map>
namespace SU2Native3D {
constexpr double MaximumMetricEdge = 1.8;
struct EdgeMeasure {
  Point a, b;
  long double length = 0;
};
struct PatchMetric {
  double minimum_mean_ratio = 1, minimum_scaled_jacobian = 1;
  Id weakest_cell = 0, sliver_cell = 0;
  Edge longest_edge{};
  long double maximum_length = 0;
  std::map<Edge, EdgeMeasure> edges;
};
struct MetricStats {
  uint64_t patches = 0, cells = 0, edges = 0, changes = 0, failures = 0;
  uint64_t edge_evaluations = 0, edge_reused = 0;
  size_t maximum_cells = 0, maximum_nodes = 0, maximum_edges = 0;
};
// Geometry intents require independently certified geometric progress at the engine caller.
enum class MetricChange { Refine, Coarsen, Repair, GeometryRefine, GeometryRepair };
enum class MetricFailure { None, Measurement, Shape, Progress };
struct MetricDiagnosis {
  MetricFailure failure = MetricFailure::None;
  bool size_progress = false, quality_progress = false, cell_removal = false;
  long double removed_excess = 0, added_excess = 0;
  double minimum_mean_ratio = 1, minimum_scaled_jacobian = 1;
  Id weakest_cell = 0, sliver_cell = 0;
};
/*! Measures only actual centroids and source-resolved edges of the supplied known cells; throws on
 * invalid geometry/source coverage. A caller can merge bounded batches by cell minima and unique edges.
 * The source must remain the same original frozen target throughout, including any scalar-edge reuse. */
PatchMetric MeasureMetricCells(const std::vector<Cell>& cells, FrozenField& target, MetricStats* stats = nullptr,
                              const std::map<Edge, EdgeMeasure>* frozen_reuse = nullptr,
                              bool require_shape = false, MetricDiagnosis* diagnosis = nullptr);
/*! Same progress gate on complete measurements assembled from the same frozen target. The caller must
 * retain all cells/edges and verify geometry/dependencies separately; this does not certify source coverage.
 * GeometryRefine/GeometryRepair need caller-certified geometric-error progress; metric shape remains strict and changed
 * oversized-edge excess must not increase. It does not authorize geometric progress by itself. */
bool ValidateMetricChange(size_t old_count, const PatchMetric& before, size_t fresh_count,
                          const PatchMetric& measured, MetricChange kind, std::string& reason,
                          MetricDiagnosis* diagnosis = nullptr);
/*! Requires separately validated geometry/topology/dependencies. Actual centroid target and source-resolved
 * target edge lengths; bounded128 cells. Final completion requires the strict gate on the whole mesh.
 * Optional reuse must come from the same frozen original sensor target; identity and coordinates are checked.
 * Composed fields always perform actual-target integration, including when reuse is supplied. */
bool ValidateMetricPatch(const std::vector<Cell>& cells, FrozenField& target, PatchMetric& measured,
                         std::string& reason, MetricStats* stats = nullptr,
                         const std::map<Edge, EdgeMeasure>* frozen_reuse = nullptr);
/*! Private progress may retain pre-existing oversized edges. Fresh shape/sliver gates are strict;
 * refinement must reduce changed-edge excess, coarsening must introduce no oversized edges and remove cells.
 * Common unchanged edges cancel exactly by identity+coordinates, not subtraction of large residual sums.
 * Repair must reduce changed-edge excess or improve quality/sliver bounds without increasing excess.
 * This is not final acceptance. Unsupported composed integration rejects, never checks the sensor substitute. */
bool ValidateMetricChange(const std::vector<Cell>& old, const std::vector<Cell>& fresh, MetricChange kind,
                          FrozenField& target, PatchMetric& measured, std::string& reason,
                          MetricStats* stats = nullptr,
                          const std::map<Edge, EdgeMeasure>* frozen_reuse = nullptr,
                          MetricDiagnosis* diagnosis = nullptr);
}  // namespace SU2Native3D
