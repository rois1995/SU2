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
enum class MetricChange { Refine, Coarsen, Repair };
/*! Requires separately validated geometry/topology/dependencies. Actual centroid target and source-resolved
 * target edge lengths; bounded128 cells. Final completion requires the strict gate on the whole mesh. */
bool ValidateMetricPatch(const std::vector<Cell>& cells, FrozenField& target, PatchMetric& measured,
                         std::string& reason, MetricStats* stats = nullptr);
/*! Private progress may retain pre-existing oversized edges. Fresh shape/sliver gates are strict;
 * refinement must reduce changed-edge excess, coarsening must introduce no oversized edges and remove cells.
 * Common unchanged edges cancel exactly by identity+coordinates, not subtraction of large residual sums.
 * Repair must reduce changed-edge excess or improve quality/sliver bounds without increasing excess.
 * This is not final acceptance. Unsupported composed integration rejects, never checks the sensor substitute. */
bool ValidateMetricChange(const std::vector<Cell>& old, const std::vector<Cell>& fresh, MetricChange kind,
                          FrozenField& target, PatchMetric& measured, std::string& reason,
                          MetricStats* stats = nullptr);
}  // namespace SU2Native3D
