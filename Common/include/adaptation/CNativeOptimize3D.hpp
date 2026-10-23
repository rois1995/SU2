/*!
 * \file CNativeOptimize3D.hpp
 * \brief Mesh-wide metric quality optimization of a native tetrahedral mesh (swaps, collapse, smoothing) and the serial
 *        opt-in build-then-optimize refinement driver.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeEngine3D.hpp"
#include <set>
namespace SU2Native3D {
struct OptimizeOptions {
  size_t maximum_sweeps = 8;        // Inner sweeps over the bad cells per call.
  double maximum_seconds = 0;       // Optional elapsed bound (0: none).
  size_t maximum_work = SIZE_MAX;   // Bound on evaluated candidate operations.
  size_t maximum_shell = 8;         // Edge removal shells 3 <= n <= maximum_shell (2n-4 new cells).
  bool swaps = true, collapses = true, smoothing = true;
  std::set<Id> frozen_cells;        // No operation may replace these cells (caller-certified first rows).
  std::set<Id> frozen_vertices;     // Never moved or removed (boundary vertices are always fixed).
  size_t cache_entries = size_t(1) << 20;  // Bounded scratch (tensor/score/length memos), cleared when reached.
};
struct OptimizeReport {
  size_t sweeps = 0, trials = 0, edge_removals = 0, face_swaps = 0, collapses = 0, moves = 0;
  size_t boundary_flips = 0, boundary_collapses = 0;  // O5/O6: faces stay inside their facet
  size_t length_removals = 0, long_edges_seen = 0;    // O1L: interior edges > 1.8 removed by swaps
  size_t rejected_orientation = 0, rejected_interface = 0, rejected_quality = 0, rejected_edge = 0, rejected_frozen = 0;
  size_t rejected_validity = 0;  // a fresh cell below q > 0, J >= ValiditySliverJacobian
  size_t bad_before = 0, bad_after = 0;
  double minimum_before = 0, minimum_after = 0;  // Minimum score min(q/.20, J/.05) over unfrozen cells.
  uint64_t point_queries = 0, edge_integrations = 0, edge_surrogates = 0;
  double seconds = 0;
  std::string stop;
  std::map<Id, Id> relabel;  // Moved interior vertex: previous identity -> new identity (chains resolved).
};
/*! Serial mesh-wide optimization in the frozen target, measured in the metric (actual centroid tensors).
 * Operations: edge removal n -> 2n-4 with an optimal ring triangulation (3-2, 4-4, ...), face removal 2-3, interior vertex
 * collapse and interior vertex smoothing (metric spring proposal and hinge Gauss-Newton step; the moved vertex gets a new
 * identity), boundary edge flips and boundary vertex contractions between faces bound to one facet (coplanar within 1e-12
 * relative, unfolded, area-preserving, prescribed oriented skin; no new point, so no new boundary parameters). No skin
 * vertex moves; physical bindings follow their faces. Every accepted operation strictly raises the minimum score
 * min(q/.20, J/.05) of its replaced cells (the only certified monotone quantity), keeps exact positive orientation and
 * validity of every fresh cell (q > 0, J >= ValiditySliverJacobian). Edge guard: candidates are screened with a Simpson
 * estimate from three point tensors; an accepted operation integrates exactly every new edge whose estimate exceeds 1.4
 * and refuses it above max(1.8, the edge it replaces); estimates <= 1.4 are not integrated (heuristic, not a certificate:
 * callers re-measure every edge exactly). Final gates are not checked here.
 * Returns true if the mesh changed. Identities: new cells from next_cell, moved vertices from next_node. */
bool OptimizeMetricMesh(Owned& mesh, const RegionMetric& target, Id& next_node, Id& next_cell,
                        const OptimizeOptions& options, OptimizeReport& report);
struct RefineOptimizeReport {
  size_t rounds = 0;
  std::vector<std::string> stops;      // Engine stop per Refine round.
  std::vector<size_t> cells;           // Cells after each Refine round.
  std::vector<OptimizeReport> optimize;
  double refine_seconds = 0, optimize_seconds = 0;
};
/*! Serial opt-in build-then-optimize: Refine under ShapeAdmission::ValidityOnly, then OptimizeMetricMesh on the result
 * (original-source RegionFieldService target, cells with a wall_heights face and their vertices frozen), re-entering
 * Refine with the optimized mesh, until the engine's strict completion, no change, the round cap or the shared time
 * budget. Success is only the engine's own strict final completion. More than one rank: plain Refine (stated in reason).
 * The caller's ShapeAdmission is restored on exit. */
EngineResult RefineOptimize(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                            const PlanarReference& geometry, MetricComposition composition, EngineOptions options,
                            OptimizeOptions optimize = {}, RefineOptimizeReport* report = nullptr,
                            size_t maximum_rounds = 16);
}  // namespace SU2Native3D
