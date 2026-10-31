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
  size_t maximum_sweeps = 8;        // Relaxation, topology repair, then enrichment over the current bad cells.
  double maximum_seconds = 0;       // Optional elapsed bound (0: none).
  size_t maximum_work = SIZE_MAX;   // Bound on evaluated candidate operations.
  size_t maximum_shell = 8;         // Edge removal shells 3 <= n <= maximum_shell (2n-4 new cells).
  bool swaps = true, collapses = true, smoothing = true;
  const PlanarReference* geometry = nullptr;  // Enables physical-boundary motion through forward geometry bindings.
  WallHeights wall_heights;
  double surface_tolerance = 0;
  bool boundary = true;             // O5/O6 boundary flips/contractions (off on a mapped parameter reference).
  bool insertions = true;           // DC-13: O7 shell / O8 neighbourhood / O7L length-driven vertex insertion.
  size_t placement_budget = 160;    // Approximate cone evaluations of one placement search (proposal only).
  std::set<Id> frozen_cells;        // No operation may replace these cells (caller-certified first rows).
  std::set<Id> frozen_vertices;     // Never moved or removed (including physical boundary vertices).
  std::set<Id> keep_vertices;       // May move (new identity, relabel) but are never removed (no collapse).
  /*! DC-17: a cell holding one of these vertices keeps its topology: no swap, collapse, insertion or boundary
   *  operation replaces it; vertex moves (O4) of its other vertices stay allowed. The caller passes vertices that never
   *  move (e.g. registered first-row apices, also in frozen_vertices), so the set follows renumbered cells. */
  std::set<Id> structure_vertices;
  /*! TEST-ONLY DIAGNOSTIC (DC-13 cut-artifact check), never set in production: skin vertices allowed to slide in the plane
   * through their position with the given unit normal (moves by O4m only, positivity/validity checked, skin geometry
   * NOT certified: no interface/facet check). Their boundary faces keep their bindings. */
  std::map<Id, std::array<double, 3>> diagnostic_sliding;
  size_t cache_entries = size_t(1) << 20;  // Bounded scratch (tensor/score/length memos), cleared when reached.
  /*! DC-17: optional exact quality of a CURRENT mesh cell by identity (cells are immutable by identity): the caller's
   *  memo of the same measure (metric at the actual centroid, the target's own point query). The whole-mesh scans
   *  (census, bad-cell list, re-checks) use it instead of re-measuring every cell per sweep; fresh candidates are
   *  always measured. Returns false when unknown. */
  std::function<bool(Id, double&, double&)> mesh_quality;
};
struct OptimizeReport {
  size_t sweeps = 0, trials = 0, edge_removals = 0, face_swaps = 0, collapses = 0, moves = 0;
  size_t boundary_flips = 0, boundary_collapses = 0;  // O5/O6: faces stay inside their facet
  size_t length_removals = 0, long_edges_seen = 0;    // O1L: interior edges > 1.8 removed by swaps
  size_t rejected_orientation = 0, rejected_interface = 0, rejected_quality = 0, rejected_edge = 0, rejected_frozen = 0;
  size_t rejected_validity = 0;  // a fresh cell without exact positive orientation or finite measured shape
  size_t shell_insertions = 0, neighbourhood_insertions = 0, length_insertions = 0;  // O7 / O8 / O7L (DC-13)
  size_t placements = 0, placement_refusals = 0;   // placement searches; proposals refused by the exact checks
  uint64_t placement_evaluations = 0;               // approximate cone evaluations (no field query)
  size_t bad_before = 0, bad_after = 0;
  double minimum_before = 0, minimum_after = 0;  // Minimum metric/first-height score over unfrozen cells.
  uint64_t point_queries = 0, edge_integrations = 0, edge_surrogates = 0;
  size_t boundary_stars_searched=0,boundary_stars_reused=0,boundary_candidates=0;
  size_t boundary_rejected_validity=0,boundary_rejected_shape=0,boundary_rejected_edge=0,boundary_rejected_geometry=0;
  size_t boundary_guidance_calls=0,boundary_upper_queries=0,boundary_moves=0;
  long double boundary_debt_reduction=0;double boundary_seconds=0;
  size_t interior_stars_searched=0,interior_stars_reused=0,interior_candidates=0;
  size_t interior_rejected_validity=0,interior_rejected_shape=0,interior_rejected_edge=0,interior_rejected_geometry=0;
  size_t interior_guidance_calls=0,interior_upper_queries=0,interior_moves=0;
  long double interior_debt_reduction=0;double interior_seconds=0;
  double seconds = 0;
  std::string stop;
  SU2NativeGeometry::BoundaryCoverage coverage;  // Current private physical faces/traces, published with parameters.
  uint64_t atlas_work=0;size_t atlas_peak_bytes=0;
  BoundaryPoints parameters;  // Private bindings of moved physical points; caller publishes atomically with cells.
  std::map<Id, Id> relabel;  // Moved vertex: previous identity -> new identity (chains resolved).
};
/*! Serial mesh-wide optimization in the frozen target, measured in the metric (actual centroid tensors).
 * Operations: edge removal n -> 2n-4 with an optimal ring triangulation (3-2, 4-4, ...), face removal 2-3, interior vertex
 * collapse and interior vertex smoothing (metric spring proposal and hinge Gauss-Newton step; the moved vertex gets a new
 * identity), boundary edge flips and boundary vertex contractions between faces bound to one facet (coplanar within 1e-12
 * relative, unfolded, area-preserving, prescribed oriented skin; no new point, so no new boundary parameters). No skin
 * vertex moves without an explicit geometry authority; certified physical moves carry new forward parameters. Every accepted operation strictly raises the minimum score
 * metric/first-height score of its replaced cells (the certified monotone quantity), keeps exact positive orientation and
 * exact positive orientation and finite measured shape of every fresh cell. Edge guard: candidates are screened with a Simpson
 * estimate from three point tensors; an accepted operation integrates exactly every new edge whose estimate exceeds 1.4
 * and refuses it above max(1.8, the edge it replaces); estimates <= 1.4 are not integrated (heuristic, not a certificate:
 * callers re-measure every edge exactly). Final gates are not checked here.
 * DC-13: smoothing is a min-max placement search over the star (O4m); O7 replaces the shell of an edge of a bad cell by
 * the cone from one new interior vertex, O8 the bad cell and its face neighbours, O7L the shell of an interior edge
 * longer than 1.8 (size split: both halves estimated <= .6 of it). Placements are proposed with vertex-mean tensors
 * (no acceptance on them) and accepted only through the exact checks above.
 * Each sweep relaxes existing vertices before swaps/collapses and then insertion. Remaining finite work is shared
 * equally among remaining enabled stages; unused work passes forward, so refused moves cannot starve enrichment.
 * Returns true if the mesh changed. Identities: new cells from next_cell, moved/new vertices from next_node. */
bool OptimizeMetricMesh(Owned& mesh, const RegionMetric& target, Id& next_node, Id& next_cell,
                        const OptimizeOptions& options, OptimizeReport& report);
/*! DC-13 helper for refinement callers: re-cone a star-shaped cavity (its cells, boundary kept) from one new vertex
 * `node` placed by the min-max search of OptimizeMetricMesh from `start`. If split_edge is given (a,b ids in the
 * cavity), the search is restricted to points whose estimated metric lengths to a and to b are <= split_ratio x the
 * estimated length of (a,b). On success `fresh` holds positive cells (ids from first_cell) with every fresh cell valid
 * and `score` their exact minimum score at actual centroids; topology, physical and admission checks are the caller's.
 * `replaced`: a cavity vertex the new one replaces (a move: it may vanish); `region`: an extra admissible region of the
 * search (e.g. a first-child height window), a proposal restriction only. */
bool PlaceConeInsertion(const std::vector<Cell>& cavity, Point start, Id node, Id first_cell, const RegionMetric& target,
                        std::vector<Cell>& fresh, double& score, const Edge* split_edge = nullptr,
                        double split_ratio = .6, size_t budget = 160, Id replaced = UINT64_MAX,
                        const std::function<bool(Point)>& region = nullptr);
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
