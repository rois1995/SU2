/*!
 * \file CNativeEngine3D.hpp
 * \brief Incremental synchronous tetrahedral refinement/coarsening on explicit worker communicators.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeDonor3D.hpp"
#include "CNativeMetric3D.hpp"
#include "CNativeRegion3D.hpp"
namespace SU2Native3D {
constexpr double CoarseningMetricEdge = .9;  // Frozen candidate hysteresis; final upper/shape gates remain strict.
constexpr size_t MaximumCandidateBatch = 8;
enum class EnginePhase { Construction, Improvement };
enum class PhaseOwnership { Automatic, Disabled, DiagnosticAlways };
struct EngineStats;
/*! Rank-local owned-cell diagnostics. Sums cover measured cells only; completeness/counts must
 * accompany them. Edge debt counts all six cell incidences, not unique global edges. */
struct StageProgress {
  size_t owned_cells=0,measured_cells=0,unmeasured_cells=0;
  size_t invalid_cells=0,construction_invalid_cells=0,quality_invalid_cells=0,remaining_edges=0,remaining_repairs=0;
  size_t shape_measured_cells=0,shape_unmeasured_cells=0;
  size_t q_measured_cells=0,j_measured_cells=0,height_measured_cells=0,height_unmeasured_cells=0;
  size_t geometry_measured_cells=0,geometry_unmeasured_cells=0;
  size_t height_unresolved_cells=0;  // Missing/nonfinite report or unresolved reference-clearance validity.
  size_t q_bad_cells=0,j_bad_cells=0,size_bad_cells=0,height_bad_cells=0,geometry_bad_cells=0;
  // Shape: max(0,1-q/.20)^2 and max(0,1-J/.05)^2. Size: sum over six incidences
  // of max(0,L/1.8-1)^2. Height: per-cell report max-error/tolerance excess squared
  // plus max(0,-3*minimum_barycentric)^2; zero measured/unmeasured counts mean not requested.
  long double q_hinge_squared=0,j_hinge_squared=0,size_hinge_squared=0,height_hinge_squared=0;
  long double geometry_excess_squared=0;  // Sum of squared physical surface-error excesses (length squared).
  bool height_requested=false,surface_error_requested=false;
  bool cache_consistent=true;  // False during an interrupted publication/cache update: all current data unknown.
  bool census_complete=false,shape_complete=false,height_complete=false,geometry_complete=false,complete=false;
};
/*! Borrowed synchronous view. No collectives, mutation or retained references in observers.
 * `valid` is final acceptance only; `complete` means all diagnostic channels are measured.
 * Function objects are borrowed rebuilding context, not a serializable field specification. */
struct StageProgressView {
  const char* stage;  // construction_entry, length_split_exit, length_collapse_exit, quality_entry, ownership_staged, window, exit
  EnginePhase phase;
  uint64_t round;
  bool valid,complete;
  const StageProgress& progress;
  const EngineStats& stats;
  const std::string& reason;
  const Owned& working;
  const std::vector<DonorCell>& original;
  const PlanarReference& geometry;
  const MetricComposition& composition;
  const MetricEdgeBreakpoints& breakpoints;
  const WallHeights& wall_heights;
  const std::map<int,RegionWallPolicy>& region_walls;
  double surface_tolerance,maximum_seconds,elapsed_seconds;
  size_t maximum_rounds,maximum_work,memory_ceiling,caller_bytes,stage_observer_bytes;
};
/*! Borrowed diagnostic view; callbacks copy bounded data and never call collectives or mutate state. */
struct TransactionView {
  const char* stage;
  Feature feature;
  uint64_t round;
  bool selected, passed;
  const Owned& working;
  const PlanarReference& geometry;
  const Dependencies& dependencies;
  const Imported& imported;
  const Patch& old;
  const Patch& fresh;
  Node point;
  Id first_cell, first_node;
  const MetricDiagnosis& metric;
  const std::string& reason;
  const DonorPatch* donors;
  const MetricComposition& composition;
  const MetricEdgeBreakpoints& breakpoints;
};
struct EngineOptions {
  size_t maximum_rounds = 0, memory_ceiling = size_t(2) << 30, caller_bytes = 0;
  // Zero rounds uses a work-bounded search; positive rounds remain an explicit diagnostic ceiling.
  size_t maximum_work = SIZE_MAX, stagnation_window = 64;  // SIZE_MAX: max(4096,64*global input cells).
  double maximum_seconds = 0;  // Optional kernel elapsed ceiling, checked at collective safe points.
  double surface_tolerance = 0;  // Analytic authority requires a positive frozen physical error ceiling.
  WallHeights wall_heights;  // Frozen marker h0 and tolerance T (ADAP_BL_FIRST_HEIGHT_TOL); empty releases the first-row constraint.
  std::map<int,RegionWallPolicy> region_walls;  // Same existing frozen BL configuration; no scenario selection.
  MetricEdgeBreakpoints edge_breakpoints;  // Frozen geometry bands; required for composed target edge audits.
  // Optional rank-local diagnostic; must not enter worker/world collectives or retain borrowed patches.
  std::function<void(const Patch&,const Patch&,Feature,const MetricDiagnosis&,bool,const std::string&)> rejected_trial;
  // Optional rank-local diagnostic: cells and their matching private geometry; no collectives or retention.
  std::function<void(const Owned&,const PlanarReference&,const std::string&)> rejected_state;
  // Nonzero count selects the first matching transaction collectively; zero observes the first local operator failure.
  Feature capture_feature{};
  std::function<void(const TransactionView&)> transaction_observer;
  std::function<void(const StageProgressView&)> stage_observer;
  size_t stage_observer_bytes=0;  // Additional observer storage reserved in every runtime admission while installed.
  PhaseOwnership phase_ownership=PhaseOwnership::Automatic;  // DiagnosticAlways exercises migration without a profitability veto.
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
/*! Sufficient local counterexample for an unchanged surviving cell of a directed collapse.
 * Cells containing both endpoints disappear and provide no witness; false is not admission. */
bool CollapseConstraintViolation(const OwnedCell& cell,Id removed,Node retained,const PlanarReference& geometry,
                                double tolerance,const WallHeights& heights);
struct EngineStats {
  EngineStop stop = EngineStop::None;
  uint64_t global_work = 0, work_limit = 0, progress_windows = 0, stagnant_windows = 0, search_windows = 0;
  uint64_t size_progress = 0, quality_progress = 0, cell_removals = 0, geometry_progress = 0;
  uint64_t geometry_reactivated=0;
  // Strict checkpoints certify hard mesh constraints; speculative publications are counted independently.
  uint64_t coarsen_checkpoints=0,coarsen_committed_passes=0,coarsen_rolled_back_passes=0;
  uint64_t coarsen_retired_directions=0,coarsen_restored_cells=0,coarsen_rolled_back_collapses=0;
  bool bidirectional=false;
  // 0: legacy/mixed, 1: surface split lane, 2: interior split lane.
  unsigned split_lane=0;
  uint64_t split_generation=0,surface_rounds=0,interior_rounds=0,interior_boundary_discoveries=0;
  uint64_t surface_conflicts=0,interior_conflicts=0,construction_stall_handoffs=0,maximum_parallel_split_commits=0;
  uint64_t child_ratio_measured=0;
  double surface_seconds=0,interior_seconds=0,maximum_child_parent_ratio=0;
  uint64_t unique_long_surface=0,unique_long_interior=0;
  double unique_edge_debt=0,maximum_edge=0;
  std::array<uint64_t,3> edge_histogram{};
  uint64_t length_split_passes=0,length_collapse_passes=0,length_splits=0,length_collapses=0,length_pass_deferred=0,length_expansion_deferrals=0;
  uint64_t ownership_evaluations=0,ownership_changes=0,ownership_moved_cells=0;
  double ownership_seconds=0,ownership_predicted_gain=0,ownership_predicted_cost=0;
  uint64_t quality_stages=0,construction_returns=0;  // Fresh strict censuses and size/geometry returns within this event.
  uint64_t region_field_points=0,region_field_sensor_hits=0,region_field_interior_hits=0;
  uint64_t region_field_edges=0,region_field_edge_hits=0,region_field_imports=0;
  size_t region_field_maximum_resident=0;
  uint64_t region_attempts=0, region_cells=0, region_surface_trials=0, region_junction_trials=0;
  uint64_t region_core_trials=0, region_core_regions=0;
  uint64_t region_transition_fronts=0, region_front_trials=0;
  double region_cap_seconds=0;  // Nested in regional sampling.
  double region_sampling_seconds=0, region_first_band_seconds=0, region_transition_seconds=0;
  double region_junction_seconds=0, region_core_seconds=0, region_validation_seconds=0;
  uint64_t layer_published = 0, height_checks = 0, height_rejected = 0, footprint_rejected = 0;
  uint64_t layer_edge_evaluations=0,layer_edge_reused=0;
  uint64_t layer_batches=0,layer_donor_batches=0,layer_donor_retries=0;
  uint64_t layer_shape_screens=0,layer_shape_pruned=0;
  size_t layer_maximum_batch=0;
  // Nested in layer_seconds; source/geometry/metric/publication remain distinct from metadata and residual work.
  double layer_metadata_seconds=0,layer_donor_seconds=0,layer_geometry_seconds=0,layer_metric_seconds=0,layer_publication_seconds=0;
  double layer_seconds = 0;  // Exclusive first-row initialization before metric seeding.
  uint64_t geometry_trials = 0, geometry_rejected = 0, metric_trials = 0;
  uint64_t geometry_guidance_trials = 0, geometry_guidance_rejected = 0;
  double geometry_guidance_seconds = 0;
  uint64_t metric_shape_rejected = 0, metric_progress_rejected = 0, metric_measurement_rejected = 0;
  MetricDiagnosis last_metric_rejection;  // Most recent private failure, including work/time exits; not an infeasibility proof.
  Feature last_rejected_feature;
  Point last_rejected_cavity_centroid{};  // Representative cavity location, not an infeasibility certificate.

  uint64_t rounds = 0, proposed = 0, conflicts = 0, attempted = 0, accepted = 0, rejected = 0;
  uint64_t batch_rounds = 0, batch_candidates = 0, batch_reserved = 0;
  uint64_t concurrent_rounds=0,final_reservations=0,proposal_deferred=0,maximum_parallel_commits=0;
  uint64_t batch_considered = 0, batch_deferred = 0;  // Bounded metadata frontiers (including one-slot rounds), before payload/transaction admission.
  uint64_t proposal_duplicates = 0;  // Replicated directed collapses suppressed before work/payload admission.
  uint64_t initial_cells = 0, updated_cells = 0, edge_evaluations = 0, edge_reused = 0, selections = 0;  // Edge evaluations here count selection-cache measurements only.
  uint64_t seed_batches=0,unmeasured_retired=0;  // Full collective seed batches; pending cells removed by fully validated cavities.
  // Local cached state at every exit, including rejection; remaining candidates are separate from frozen metric violations.
  uint64_t final_owned_cells = 0, cached_measured_cells = 0, cached_invalid_cells = 0;
  uint64_t remaining_edges = 0, remaining_repairs = 0;
  uint64_t field_requests = 0, field_hits = 0, field_misses = 0, field_invalid_queries = 0;
  uint64_t field_failures = 0, field_evictions = 0, field_compositions = 0, field_edges = 0;
  uint64_t donor_hint_tests=0,donor_hint_hits=0;
  uint64_t private_edge_evaluations=0,private_edge_reused=0,private_edge_campaign_hits=0;
  uint64_t donor_batches = 0, batch_retries = 0, query_import_retries = 0, private_query_retries = 0;
  uint64_t donor_candidates = 0, donor_filtered = 0, donor_box_filtered = 0, donor_payloads = 0, donor_reused = 0, edge_separated = 0;
  uint64_t metric_support_cells=0,metric_support_edges=0,metric_support_skipped=0,metric_support_duplicates=0,collapse_constraint_filtered=0;
  // Per-call deltas of existing World wrappers, not complete MPI/network/wait instrumentation.
  uint64_t tracked_bytes = 0, tracked_calls = 0, directory_elections = 0;
  double tracked_collective_seconds = 0, directory_election_seconds = 0;
  size_t donor_maximum_patch = 0;
  double donor_search_seconds = 0, donor_filter_seconds = 0;  // Nested in import/search.
  uint64_t repair_proposals = 0, moved = 0, reconnected = 0, inserted = 0;
  uint64_t placement_classified_vertices = 0, placement_dependency_expansions = 0;
  uint64_t optimization_trials = 0, optimization_validated = 0;
  uint64_t atlas_work = 0;size_t atlas_peak_bytes = 0;  // Shared exact geometry work/storage receipts.
  uint64_t optimization_moves = 0, optimization_insertions = 0, optimization_collapses = 0, optimization_swaps = 0;  // Nested operations in published repairs only.
  uint64_t boundary_smoothing_stars=0,boundary_smoothing_reused=0,boundary_smoothing_candidates=0;
  uint64_t boundary_smoothing_validity_rejects=0,boundary_smoothing_shape_rejects=0,boundary_smoothing_edge_rejects=0,boundary_smoothing_geometry_rejects=0;
  uint64_t boundary_smoothing_guidance_calls=0,boundary_smoothing_upper_queries=0,boundary_smoothing_moves=0;
  long double boundary_smoothing_debt_reduction=0;double boundary_smoothing_seconds=0;
  uint64_t interior_smoothing_stars=0,interior_smoothing_reused=0,interior_smoothing_candidates=0;
  uint64_t interior_smoothing_validity_rejects=0,interior_smoothing_shape_rejects=0,interior_smoothing_edge_rejects=0,interior_smoothing_geometry_rejects=0;
  uint64_t interior_smoothing_guidance_calls=0,interior_smoothing_upper_queries=0,interior_smoothing_moves=0;
  long double interior_smoothing_debt_reduction=0;double interior_smoothing_seconds=0;
  uint64_t placement_connectivity_trials = 0, placement_flips = 0, placement_moves = 0;
  uint64_t metric_linearizations = 0, metric_probes = 0, metric_linear_iterations = 0, metric_solve_trials = 0;
  uint64_t placement_trials = 0, placement_validated = 0;  // Bounded joint placement; validation precedes publication/clearance.
  uint64_t repair_fallbacks = 0, fallback_accepted = 0, recovery_starts = 0;
  uint64_t wall_reconstruction_trials = 0, wall_reconstruction_accepted = 0;
  uint64_t wall_joint_trials = 0, wall_joint_moves = 0, wall_joint_removals = 0, wall_joint_accepted = 0;
  // Joint connectivity-repair rejection histogram (summed private reports; not an infeasibility measure).
  uint64_t wall_joint_flips = 0, wall_joint_vertex_moves = 0, wall_joint_coordinated = 0, wall_joint_solve_rejected = 0;
  uint64_t wall_joint_move_geometry_rejected = 0, wall_joint_move_height_rejected = 0, wall_joint_move_loss_rejected = 0;
  uint64_t oversized_stars = 0;  // Candidates refused for a complete star above the cavity bound; blocked, never published.
  uint64_t wall_topology_trials = 0, wall_topology_accepted = 0;
  uint64_t collapse_geometry_checks = 0, collapse_filtered = 0;
  uint64_t collapse_orientation_checks = 0, collapse_orientation_filtered = 0, collapse_reactivated = 0;
  uint64_t blocked_queue_skips=0,blocked_unmeasured=0;  // Measurement is not a topology-change notification.
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
  uint64_t collision_builds=0,collision_candidates=0,collision_checks=0;
  double collision_seconds=0; // Exclusive current-mesh contact search.
  double parameter_setup_seconds=0,parameter_agreement_seconds=0,parameter_publication_seconds=0; // Nested geometry detail.
  uint64_t parameter_edits=0,parameter_faces=0,parameter_payload_bytes=0;
  double total_seconds = 0, initialization_seconds = 0, seed_seconds = 0, selection_seconds = 0;
  double planning_seconds = 0;  // Nested in selection: authoritative metadata lookup/election before payload admission.
  double dependency_seconds = 0, reservation_seconds = 0, donor_seconds = 0;
  double reconstruction_seconds = 0, metric_seconds = 0, routing_seconds = 0;
  double publication_seconds = 0, update_seconds = 0, completion_seconds = 0, other_seconds = 0;
};
struct EngineResult {
  Owned cells;  // Private result; on failure this is rejected progress, never an accepted CFD replacement.
  std::shared_ptr<BoundaryParameterState> parameters;
  std::shared_ptr<const PlanarReference> reference;  // Already staged view; caller publication does not allocate. // Same private result/failed-progress scope as cells.
  std::vector<SplitRecord> splits;
  std::vector<Edge> collapses;  // Directed {retained, removed}; no split-history requirement.
  bool valid = false;
  // A complete metric-driven domain replacement includes both refinement and coarsening.
  bool rebuilt_domain = false;
  std::string reason;
  EngineStats stats;
};
/*! Production metric adaptation alternates vertex-disjoint split/collapse passes, followed by
 * metric/BL quality improvement. Exact positivity, geometry and frozen edge lengths govern construction;
 * missing q/J/height measurements are added at quality entry and versioned certificates remain reusable.
 * Donors, private geometry, ownership and work/deadline are shared. Only complete strict final acceptance
 * makes the private result valid. All worker ranks call together. */
EngineResult AdaptMetric(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition = {}, EngineOptions options = {});
/*! Refinement construction uses the same staged quality closure and unchanged final gates. */
EngineResult Refine(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition = {}, EngineOptions options = {});
/*! General directed vertex-star coarsening on the same frozen-target transaction path. A successful event
 * passes all final metric/shape gates and exhausts admitted short-edge proposals; it need not reach a minimal
 * mesh. Removal and ensuing quality recovery form one speculative trial: a completed strict closure
 * must reduce global cell count, or it restores a strict checkpoint and retires the actually attempted
 * directions attempted on matching checkpoint support. A finite exhausted repair queue rejects its
 * trial as well. Collateral disappearances remain obligations after rollback; budget exits still reject
 * the event. Blocked directions survive censuses and reopen after support changes. Complete vertex
 * dependencies and immutable incident finite facets protect geometry; no inverse-split history is required.
 * Already-admissible meshes enter removal from a strict checkpoint; coarsening need not minimize cell count. */
EngineResult Coarsen(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition = {}, EngineOptions options = {});
/*! Uses the same distributed construction/improvement lifecycle, with complete vertex/edge/face repairs
 * for quality and requested first-height deficits. Exact-positive intermediate cells have no q/J floor;
 * final success still requires all strict frozen target gates. */
EngineResult Repair(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition = {}, EngineOptions options = {});
}  // namespace SU2Native3D
