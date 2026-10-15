/*!
 * \file CNativeEngine3D.hpp
 * \brief Incremental synchronous tetrahedral refinement/coarsening on explicit worker communicators.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeDonor3D.hpp"
#include "CNativeMetric3D.hpp"
namespace SU2Native3D {
constexpr double CoarseningMetricEdge = .9;  // Frozen candidate hysteresis; final upper/shape gates remain strict.
constexpr size_t MaximumCandidateBatch = 8;
struct EngineOptions {
  size_t maximum_rounds = 0, memory_ceiling = size_t(2) << 30, caller_bytes = 0;
  // Zero rounds uses a work-bounded search; positive rounds remain an explicit diagnostic ceiling.
  size_t maximum_work = SIZE_MAX, stagnation_window = 64;  // SIZE_MAX: max(4096,64*global input cells).
  double maximum_seconds = 0;  // Optional kernel elapsed ceiling, checked at collective safe points.
  double surface_tolerance = 0;  // Analytic authority requires a positive frozen physical error ceiling.
  std::map<int,double> wall_heights;  // Frozen marker policy; empty releases the first-row constraint.
  MetricEdgeBreakpoints edge_breakpoints;  // Frozen geometry bands; required for composed target edge audits.
  // Optional rank-local diagnostic; must not enter worker/world collectives or retain borrowed patches.
  std::function<void(const Patch&,const Patch&,Feature,const MetricDiagnosis&,bool,const std::string&)> rejected_trial;
  bool timings = false;
  size_t candidate_batch = MaximumCandidateBatch;  // Internal control for paired correctness/work comparisons.
};
struct SplitRecord {
  Edge edge{};
  Node point;
};
enum class EngineStop { None, Complete, RoundBudget, WorkBudget, TimeBudget, OperatorStagnation, Failure };
inline const char* EngineStopName(EngineStop stop) {
  constexpr const char* names[]={"none","complete","round_budget","work_budget","time_budget","operator_stagnation","failure"};
  const auto i=static_cast<unsigned>(stop); return i<7 ? names[i] : "failure";
}
struct EngineStats {
  EngineStop stop = EngineStop::None;
  uint64_t global_work = 0, work_limit = 0, progress_windows = 0, stagnant_windows = 0, search_windows = 0;
  uint64_t size_progress = 0, quality_progress = 0, cell_removals = 0, geometry_progress = 0;
  uint64_t layer_published = 0, height_checks = 0, height_rejected = 0, footprint_rejected = 0;
  double layer_seconds = 0;  // Exclusive first-row initialization before metric seeding.
  uint64_t geometry_trials = 0, geometry_rejected = 0, metric_trials = 0;
  uint64_t metric_shape_rejected = 0, metric_progress_rejected = 0, metric_measurement_rejected = 0;
  MetricDiagnosis last_metric_rejection;  // Most recent private failure, including work/time exits; not an infeasibility proof.
  Feature last_rejected_feature;
  Point last_rejected_cavity_centroid{};  // Representative cavity location, not an infeasibility certificate.

  uint64_t rounds = 0, proposed = 0, conflicts = 0, attempted = 0, accepted = 0, rejected = 0;
  uint64_t batch_rounds = 0, batch_candidates = 0, batch_reserved = 0;
  uint64_t initial_cells = 0, updated_cells = 0, edge_evaluations = 0, edge_reused = 0, selections = 0;  // Edge evaluations here count selection-cache measurements only.
  // Local cached state at every exit, including rejection; remaining candidates are separate from frozen metric violations.
  uint64_t final_owned_cells = 0, cached_measured_cells = 0, cached_invalid_cells = 0;
  uint64_t remaining_edges = 0, remaining_repairs = 0;
  uint64_t field_requests = 0, field_hits = 0, field_misses = 0, field_invalid_queries = 0;
  uint64_t field_failures = 0, field_evictions = 0, field_compositions = 0, field_edges = 0;
  uint64_t donor_batches = 0, batch_retries = 0, query_import_retries = 0, private_query_retries = 0;
  uint64_t donor_candidates = 0, donor_filtered = 0, donor_box_filtered = 0, donor_payloads = 0, donor_reused = 0, edge_separated = 0;
  // Per-call deltas of existing World wrappers, not complete MPI/network/wait instrumentation.
  uint64_t tracked_bytes = 0, tracked_calls = 0, directory_elections = 0;
  double tracked_collective_seconds = 0, directory_election_seconds = 0;
  size_t donor_maximum_patch = 0;
  double donor_search_seconds = 0, donor_filter_seconds = 0;  // Nested in import/search.
  uint64_t repair_proposals = 0, moved = 0, reconnected = 0, inserted = 0;
  uint64_t placement_classified_vertices = 0, placement_dependency_expansions = 0;
  uint64_t placement_connectivity_trials = 0, placement_flips = 0, placement_moves = 0;
  uint64_t metric_linearizations = 0, metric_probes = 0, metric_linear_iterations = 0, metric_solve_trials = 0;
  uint64_t placement_trials = 0, placement_validated = 0;  // Bounded joint placement; validation precedes publication/clearance.
  uint64_t repair_fallbacks = 0, fallback_accepted = 0, recovery_starts = 0;
  uint64_t wall_reconstruction_trials = 0, wall_reconstruction_accepted = 0;
  uint64_t wall_joint_trials = 0, wall_joint_moves = 0, wall_joint_removals = 0, wall_joint_accepted = 0;
  uint64_t wall_topology_trials = 0, wall_topology_accepted = 0;
  uint64_t dependency_backtracks = 0, private_continuations = 0;
  uint64_t collapse_geometry_checks = 0, collapse_filtered = 0;
  uint64_t collapse_orientation_checks = 0, collapse_orientation_filtered = 0, collapse_reactivated = 0;
  uint64_t changed_star_scans = 0, changed_vertices_sent = 0, changed_vertices_received = 0;
  double collapse_orientation_seconds = 0, change_notification_seconds = 0;  // Nested in selection/publication.
  double collapse_geometry_seconds = 0;  // Nested in selection, seed and update, not metric queries.
  double process_cpu_seconds = 0, reconstruction_cpu_seconds = 0, metric_cpu_seconds = 0, measurement_cpu_seconds = 0;
  // Nested query detail; never add to the exclusive engine phases below.
  double field_build_seconds = 0, field_query_seconds = 0, field_cache_seconds = 0, field_search_seconds = 0;
  double field_interpolation_seconds = 0, field_composition_seconds = 0;
  double edge_seconds = 0, edge_search_seconds = 0, edge_trace_seconds = 0, edge_integral_seconds = 0;
  double edge_geometry_seconds = 0, edge_composition_seconds = 0;  // Nested in edge integration.
  size_t maximum_work_bytes = 0, maximum_owned = 0;
  double total_seconds = 0, initialization_seconds = 0, seed_seconds = 0, selection_seconds = 0;
  double dependency_seconds = 0, reservation_seconds = 0, donor_seconds = 0;
  double reconstruction_seconds = 0, metric_seconds = 0, routing_seconds = 0;
  double publication_seconds = 0, update_seconds = 0, completion_seconds = 0, other_seconds = 0;
};
struct EngineResult {
  Owned cells;  // Private result; on failure this is rejected progress, never an accepted CFD replacement.
  std::vector<SplitRecord> splits;
  std::vector<Edge> collapses;  // Directed {retained, removed}; no split-history requirement.
  bool valid = false;
  std::string reason;
  EngineStats stats;
};
/*! Complete metric refinement with bounded private reconnection after rejection and one repair-queue recovery.
 * Recovery retains one dependency-tracked repair queue and retries newly unblocked splits after edits.
 * It shares the same frozen source/indexes, round budget and strict completion gates.
 * All world ranks call together; accepted and original source remain unchanged. Same options/immutable geometry
 * authority and frozen composition policy on every worker; source must cover the original accepted domain.
 * Donor/dependency limits can reject the whole event. Exact edge splits or certified edge-cavity insertion
 * preserve the admitted cavity and physical planar domain; caller supplies embedded conforming accepted input and facet authority.
 * Success means all working cells satisfy the frozen metric gates, not CFD/CGNS/partition qualification.
 * Each instance/call runs on one thread per worker. Byte models describe requested storage, not allocator/RSS.
 */
EngineResult Refine(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition = {}, EngineOptions options = {});
/*! General directed vertex-star coarsening on the same frozen-target transaction path. A successful event
 * passes all final metric/shape gates and exhausts admitted short-edge proposals; it need not reach a minimal
 * mesh. Blocked directions are retried only after adjacent topology changes. Complete vertex dependencies
 * and immutable incident finite facets protect geometry; no inverse-split history is required. Rejected
 * removal in an invalid field can try movement/reconnection; a stalled invalid mesh admits one repair queue.
 * Already-admissible meshes retain the cheaper removal path; coarsening need not minimize cell count. */
EngineResult Coarsen(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition = {}, EngineOptions options = {});
/*! Automatically reconnects admitted face/edge cavities or moves complete vertex stars of failing cells.
 * At most16 private proposals per cavity; strict fresh shape/sliver and original-field progress gates remain.
 * Success requires every working cell to pass the full frozen target. Stagnation rejects the private event;
 * finite reference facets protect planar motion and original seams. No refinement/coarsening target relaxation. */
EngineResult Repair(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition = {}, EngineOptions options = {});
}  // namespace SU2Native3D
