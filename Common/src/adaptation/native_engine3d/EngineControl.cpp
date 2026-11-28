// Runs phase and checkpoint transitions, observation and finalization using borrowed canonical state without policy.
#include "EngineControl.hpp"
#include "EngineContext.hpp"
#include "EngineReports.hpp"
#include "EngineTrial.hpp"

size_t EngineControl::SelectionBound(EngineContext& context, bool repairs) {
  auto& coarsening = *this->coarsening;
  auto& bidirectional = context.bidirectional;
  return transfer_memory::Add(sizeof(Selection::CellDebt), repairs ? 4096u : 0u,
                              (coarsening || bidirectional) ? (repairs ? 8192u : 5120u) : (repairs ? 6144u : 3072u));
}

// Terminal control methods borrow the live Adapt state.
// Certifies the entity-backed final surface under the original proof scope.
void EngineControl::ProveEntitySurface(EngineContext& context, Owned& working,
                                       SU2NativeGeometry::AtlasBudget& atlas_budget) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  if ((result.valid || result.surface_prepared) && geometry.EntityGeometry()) {
    std::optional<SU2NativeTiming3D::PhaseScope> proof_phase;
    std::optional<SU2NativeTiming3D::Scope> proof_scope;
    if (options.timings) {
      proof_phase.emplace(SU2NativeTiming3D::Phase::FinalProof, SU2NativeTiming3D::SourceSite{__FILE__, __LINE__});
      proof_scope.emplace(SU2NativeTiming3D::Operation::Certification, SU2NativeTiming3D::SourceSite{__FILE__, __LINE__});
    }
    CLocalFailure geometryFailure;
    FinalProofReport proof;
    try {
      D9QueryScope d9_query(SU2NativeD9::Site::Finish, &geometry);
      proof = ProveOwnedSurface(world, working, geometry, atlas_budget, stats, options.timings);
    } catch (const std::exception& e) {
      geometryFailure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, geometryFailure)) {
      result.valid = result.surface_prepared = false;
      stats.stop = EngineStop::Failure;
    } else if (options.timings) {
      proof_scope.reset();
      proof_phase.reset();
      PrintFinalProof(proof, world.rank, geometry.Parameters()->GeometryTolerance());
    }
  }
}

// Checks the prepared non-entity surface and retains its original vote.
void EngineControl::ProvePreparedSurface(EngineContext& context, Owned& working) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  if (result.surface_prepared && !geometry.EntityGeometry()) {
    std::optional<SU2NativeTiming3D::PhaseScope> proof_phase;
    std::optional<SU2NativeTiming3D::Scope> proof_scope;
    if (options.timings) {
      proof_phase.emplace(SU2NativeTiming3D::Phase::FinalProof, SU2NativeTiming3D::SourceSite{__FILE__, __LINE__});
      proof_scope.emplace(SU2NativeTiming3D::Operation::Certification, SU2NativeTiming3D::SourceSite{__FILE__, __LINE__});
    }
    CLocalFailure geometryFailure;
    try {
      for (const auto& row : working) {
        const auto patch = GeometryPatch(Imported{{row.second}, true, {}}, &geometry);
        for (const auto& face : patch.surface) {
          std::array<Point, 3> triangle{};
          for (unsigned k = 0; k < 3; ++k)
            for (const auto& node : row.second.cell.v)
              if (node.id == face.v[k]) triangle[k] = node.p;
          if (geometry.SurfaceDeviation(face.facet, triangle, SU2NativeGeometry::SurfaceQueryNeed::Classify()) >
              options.surface_tolerance)
            geometryFailure.Set(1, face.facet, "Prepared surface does not meet the prescribed geometry tolerance.");
        }
      }
    } catch (const std::exception& e) {
      geometryFailure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, geometryFailure)) {
      result.surface_prepared = false;
      stats.stop = EngineStop::Failure;
    }
  }
}

// Checks final wall heights and retains the original certification vote.
void EngineControl::ProveFinalHeights(EngineContext& context, Owned& working) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& surface_target = context.surface_target;
  if ((result.valid || result.surface_prepared) && !options.wall_heights.empty()) {
    std::optional<SU2NativeTiming3D::PhaseScope> proof_phase;
    std::optional<SU2NativeTiming3D::Scope> proof_scope;
    if (options.timings) {
      proof_phase.emplace(SU2NativeTiming3D::Phase::FinalProof, SU2NativeTiming3D::SourceSite{__FILE__, __LINE__});
      proof_scope.emplace(SU2NativeTiming3D::Operation::Certification, SU2NativeTiming3D::SourceSite{__FILE__, __LINE__});
    }
    CLocalFailure heightFailure;
    for (const auto& row : working) {
      std::string reason;
      if (!EngineTrial::HeightCheck(context, GeometryPatch(Imported{{row.second}, true, {}}, &geometry), reason,
                                    bool(surface_target)))
        heightFailure.Set(1, row.first, reason);
    }
    if (!context.exchange.Vote(context, heightFailure)) {
      result.valid = result.surface_prepared = false;
      stats.stop = EngineStop::Failure;
    }
  }
}

// Runs the synchronous rejected-state callback and elected failure path.
void EngineControl::ObserveRejectedState(EngineContext& context, Owned& working) {
  auto& observe_state = *context.control.observe_state;
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  if (observe_state) {
    CLocalFailure observed;
    if (!result.valid && !result.surface_prepared && options.rejected_state) try {
        options.rejected_state(working, geometry, result.reason);
      } catch (const std::exception& e) {
        observed.Set(2, 0, e.what());
      }
    const auto elected = world.elect(observed);
    if (elected.any) {
      result.valid = result.surface_prepared = false;
      stats.stop = EngineStop::Failure;
      result.reason += " Rejected-state diagnostic failed: " + elected.message;
    }
  }
}

// Captures final selection and tracked-communication counters.
void EngineControl::CaptureFinalCounts(EngineContext& context, Owned& working) {
  auto& world = context.world;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  const auto& tracked_bytes_begin = *context.control.tracked_bytes_begin;
  const auto& tracked_calls_begin = *context.control.tracked_calls_begin;
  const auto& tracked_seconds_begin = *context.control.tracked_seconds_begin;
  stats.final_owned_cells = working.size();
  stats.cached_measured_cells = selection.census.cells.size();
  stats.cached_invalid_cells = selection.census.invalid;
  stats.remaining_edges =
      selection.queues.queue.size() + selection.queues.surface_queue.size() + selection.queues.geometry_queue.size();
  stats.remaining_repairs = selection.queues.repair_queue.size();
  stats.tracked_bytes = world.bytes_sent - tracked_bytes_begin;
  stats.tracked_calls = world.collective_calls - tracked_calls_begin;
  stats.tracked_collective_seconds = world.collective_seconds - tracked_seconds_begin;
}

// Retires directories and copies donor/atlas totals in original order.
void EngineControl::CaptureDirectoryDonorStatistics(EngineContext& context, DirectoryStats& retired_directory,
                                                    SU2NativeGeometry::AtlasBudget& atlas_budget) {
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& directory = *context.exchange.directory;
  auto& checkpoint = *context.control.checkpoint;
  auto& source = *context.exchange.source;
  auto& private_parameters = *context.setup.private_parameters;
  auto& private_geometry = *context.setup.private_geometry;
  const auto& atlas_work_initial = *context.setup.control_prefix.atlas_work_initial;
  const auto saved_directory = retired_directory;
  context.exchange.RetireDirectory(directory->Statistics());
  if (checkpoint.directory) context.exchange.RetireDirectory(checkpoint.directory->Statistics());
  const auto all_directory = retired_directory;
  retired_directory = saved_directory;
  stats.collision_seconds = all_directory.collision_seconds;
  stats.collision_builds = all_directory.collision_builds;
  stats.collision_candidates = all_directory.collision_candidates;
  stats.collision_checks = all_directory.collision_checks;
  stats.directory_elections = all_directory.elections;
  stats.directory_election_seconds = all_directory.election_seconds;
  stats.parameter_agreement_seconds = all_directory.parameter_agreement_seconds;
  stats.parameter_publication_seconds = all_directory.parameter_publication_seconds;
  stats.parameter_edits = all_directory.parameter_edits;
  stats.parameter_faces = all_directory.parameter_faces;
  stats.parameter_payload_bytes = all_directory.parameter_payload_bytes;
  stats.changed_star_scans = all_directory.change_scanned;
  stats.changed_vertices_sent = all_directory.change_sent;
  stats.changed_vertices_received = all_directory.change_received;
  stats.change_notification_seconds = all_directory.change_seconds;
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, all_directory.maximum_work_bytes);
  const auto& donor = source.Statistics();
  stats.donor_candidates = donor.index_candidates;
  stats.donor_filtered = donor.filtered;
  stats.donor_box_filtered = donor.box_filtered;
  stats.donor_payloads = donor.payloads;
  stats.donor_reused = donor.reused_candidates;
  stats.donor_maximum_patch = donor.maximum_patch;
  stats.donor_search_seconds = donor.search_seconds;
  stats.donor_filter_seconds = donor.filter_seconds;
  stats.metric_support_cells = donor.metric_sample_cells;
  stats.metric_support_edges = donor.metric_edge_queries;
  stats.metric_support_skipped = donor.metric_edge_skipped;
  stats.metric_support_duplicates = donor.metric_edge_duplicates;
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, donor.maximum_work_bytes);
  stats.atlas_work = atlas_budget.work - atlas_work_initial;
  stats.atlas_peak_bytes = atlas_budget.peak_bytes;
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, atlas_budget.peak_bytes);
  result.parameters = private_parameters;
  result.reference = private_geometry;
}

// Adds original cached-demand details and initial timing totals.
void EngineControl::EnrichFinalReasonAndTiming(EngineContext& context, bool curved, Owned& working,
                                               const Clock::time_point& started, const std::clock_t& started_cpu) {
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& selection = *context.census.selection;
  auto& last_rejection = *context.control.last_rejection;
  if (curved && !result.valid && !selection.census.cells.empty()) {
    size_t metric_bad = 0, geometry_bad = 0;
    for (const auto& row : selection.census.cells) {
      metric_bad += !row.second.target_valid;
      geometry_bad += row.second.geometry_invalid;
    }
    result.reason += " Local cached demands: geometry_cells=" + std::to_string(geometry_bad) +
                     ", metric_or_height_cells=" + std::to_string(metric_bad) +
                     ", geometry_edges=" + std::to_string(selection.queues.geometry_queue.size()) +
                     ", metric_edges=" + std::to_string(selection.queues.queue.size()) + ".";
  }
  if (!result.valid && !last_rejection.empty()) result.reason += " Last local rejection: " + last_rejection;
  if (options.timings) {
    stats.process_cpu_seconds = double(std::clock() - started_cpu) / CLOCKS_PER_SEC;
    stats.total_seconds = std::chrono::duration<double>(Clock::now() - started).count();
    const double phases = stats.initialization_seconds + stats.seed_seconds + stats.selection_seconds +
                          stats.dependency_seconds + stats.reservation_seconds + stats.donor_seconds +
                          stats.reconstruction_seconds + stats.metric_seconds + stats.routing_seconds +
                          stats.publication_seconds + stats.update_seconds + stats.completion_seconds +
                          stats.collision_seconds;
    stats.other_seconds = stats.total_seconds - phases - stats.layer_seconds;
  }
}

// Emits final rows, rereads totals, swaps cells, and moves the result.
EngineResult EngineControl::EmitFinalRowsAndMove(EngineContext& context, Owned& working,
                                                 SU2NativeGeometry::AtlasBudget& atlas_budget,
                                                 const Clock::time_point& started, const std::clock_t& started_cpu) {
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& region_rebuilt = *context.setup.control_prefix.region_rebuilt;
  const auto& tracked_bytes_begin = *context.control.tracked_bytes_begin;
  const auto& tracked_calls_begin = *context.control.tracked_calls_begin;
  const auto& tracked_seconds_begin = *context.control.tracked_seconds_begin;
  auto& wall_counters = *context.setup.wall_counters;
  auto& lane_counters = *context.setup.lane_counters;
  PrintWallLawCensus(working, geometry, options.surface_tolerance, world.rank, "exit", options.timings);
  bool quality_reported = false;
  ObserveProgress(context, working, atlas_budget, "exit", &quality_reported);
  wall_counters.Print(world.rank, {stats.geometry_trials, stats.geometry_rejected, stats.geometry_guidance_trials,
                                   stats.geometry_guidance_rejected, stats.accepted, stats.metric_trials,
                                   stats.proposed, stats.attempted});
  lane_counters.Print(world.rank);
  stats.tracked_bytes = world.bytes_sent - tracked_bytes_begin;
  stats.tracked_calls = world.collective_calls - tracked_calls_begin;
  stats.tracked_collective_seconds = world.collective_seconds - tracked_seconds_begin;
  if (options.timings) {
    const auto total = std::chrono::duration<double>(Clock::now() - started).count();
    stats.other_seconds += total - stats.total_seconds;
    stats.total_seconds = total;
    stats.process_cpu_seconds = double(std::clock() - started_cpu) / CLOCKS_PER_SEC;
  }
  result.rebuilt_domain = result.valid && region_rebuilt;
  if (options.timings && options.report_rejected_neighborhood && quality_reported && !result.valid &&
      !result.surface_prepared)
    EngineReports::PrintStuckNeighborhood(context);
  result.cells.swap(working);
  return std::move(result);
}

// Calls the stage observer synchronously and elects any callback failure.
bool EngineControl::NotifyStageObserver(EngineContext& context, const char* stage, Owned& working) {
  using Phase = EnginePhase;
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& selection = *context.census.selection;
  auto& original = context.original;
  auto& geometry = context.geometry;
  auto& composition = context.composition;
  const auto& started = *context.setup.control_prefix.started;
  auto& last_rejection = *context.control.last_rejection;
  auto& diagnostic_rejections = *context.control.diagnostic_rejections;
  auto& observe_stage = *context.control.observe_stage;
  auto& stage_storage_admitted = *context.control.stage_storage_admitted;
  auto& stage_observer_failed = *context.control.stage_observer_failed;
  auto& phase = *context.setup.control_prefix.phase;
  if (!observe_stage || !stage_storage_admitted || stage_observer_failed) return !stage_observer_failed;
  CLocalFailure failure;
  if (options.stage_observer) try {
      const auto progress = selection.Progress(working.size());
      options.stage_observer({stage,
                              phase,
                              stats.rounds,
                              result.valid,
                              progress.complete,
                              progress,
                              stats,
                              result.reason,
                              working,
                              original,
                              geometry,
                              composition,
                              options.edge_breakpoints,
                              options.wall_heights,
                              options.region_walls,
                              options.surface_tolerance,
                              options.maximum_seconds,
                              std::chrono::duration<double>(Clock::now() - started).count(),
                              options.maximum_rounds,
                              options.maximum_work,
                              options.memory_ceiling,
                              options.caller_bytes,
                              options.stage_observer_bytes,
                              [&](Id id, CellDiagnostic& out) {
                                const auto row = working.find(id);
                                return row != working.end() && selection.Diagnose(row->second, out);
                              },
                              &last_rejection,
                              &diagnostic_rejections,
                              world.rank,
                              world.size});
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
  const auto elected = world.elect(failure);
  if (elected.any) {
    stage_observer_failed = true;
    result.valid = result.surface_prepared = false;
    stats.stop = EngineStop::Failure;
    result.reason += " Stage progress observer failed: " + elected.message;
    return false;
  }
  return true;
}

// Preserves ordered progress elections, refresh, census, and output.
bool EngineControl::ObserveProgress(EngineContext& context, Owned& working,
                                    SU2NativeGeometry::AtlasBudget& atlas_budget, const char* stage,
                                    bool* quality_reported) {
  using Phase = EnginePhase;
  auto& world = context.world;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& selection = *context.census.selection;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& private_parameters = *context.setup.private_parameters;
  auto& geometry_authority = *context.control.geometry_authority;
  auto& phase = *context.setup.control_prefix.phase;
  const bool surface_target = bool(context.surface_target);

  stats.job_peak_rss_lower = transfer_memory::Add(JobPeakRSS3D(world.comm), options.idle_peak_rss_bytes);
  CLocalFailure memory_failure;
  if (options.job_memory_limit && stats.job_peak_rss_lower > options.job_memory_limit)
    memory_failure.Set(3, 0, "Native3D whole-job process peak RSS exceeds memory budget.");
  const auto memory_vote = world.elect(memory_failure);
  if (memory_vote.any) {
    result.valid = result.surface_prepared = false;
    stats.stop = EngineStop::MemoryBudget;
    result.reason = memory_vote.message;
    return false;
  }
  if (options.timings && (std::strcmp(stage, "length_collapse_exit") == 0 ||
                          std::strcmp(stage, "construction_entry") == 0 || std::strcmp(stage, "exit") == 0))
    SU2NativeTiming3D::Session::Checkpoint(stage);
  if (surface_target) {
    CLocalFailure failure;
    try {
      context.census.RefreshSurfaceCensus(context);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    const auto elected = world.elect(failure);
    if (elected.any) {
      result.valid = result.surface_prepared = false;
      result.reason = elected.message;
      return false;
    }
  }
  if (phase == Phase::Improvement) {
    SU2NativeTiming3D::Scope progress_timer(SU2NativeTiming3D::Operation::Census);
    const auto census = selection.Progress(working.size());
    const bool complete =
        !world.maximum(!census.census_complete || !census.shape_complete || !census.geometry_complete);
    if (complete) {
      EngineReports::EmitFixedMetricDrift(context, stage);
      EngineReports::EmitQualityCensus(context, stage, census);
      if (quality_reported) *quality_reported = true;
    }
  }
  if (options.timings && (std::string(stage) != "window" || phase == Phase::Improvement)) {
    SU2NativeTiming3D::Scope output_scope(SU2NativeTiming3D::Operation::Output);
    const auto p = selection.Progress(working.size());
    const auto* state = geometry.Parameters();
    const auto cache = geometry_authority ? geometry_authority->GeometryStatsSnapshot()
                                          : SU2NativeGeometry::MappedFacetReference::GeometryStats{};
    const auto authority = geometry_authority ? geometry_authority->ResidentBytes() : 0;
    std::cout << "NATIVE3D_STORAGE rank=" << world.rank << " stage=" << stage << " authority_plus_memo_bytes="
              << (authority >= cache.prepared_limit ? authority - cache.prepared_limit : authority)
              << " prepared_reserved_bytes=" << cache.prepared_limit << " facet_live_bytes=" << cache.facet_bytes
              << " scope_live_bytes=" << cache.scope_bytes << " private_cells_bytes=" << MapBytes(working)
              << " selection_bytes=" << selection.Bytes() << " directory_bytes=" << directory->ResidentBytes()
              << " metric_source_bytes=" << source.ResidentBytes()
              << " parameter_bytes=" << (private_parameters ? private_parameters->ResidentBytes() : 0)
              << " caller_reservation_bytes=" << options.caller_bytes
              << " native_peak_bytes=" << stats.maximum_work_bytes << '\n';
    EngineReports::EmitStageRows(context, stage, p, state, atlas_budget);
  }
  return NotifyStageObserver(context, stage, working);
}

// Runs the sole terminal proof and result-producing path.
EngineResult EngineControl::Finish(EngineContext& context, bool curved, Owned& working,
                                   SU2NativeGeometry::AtlasBudget& atlas_budget, const Clock::time_point& started,
                                   const std::clock_t& started_cpu) {
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& region_rebuilt = *context.setup.control_prefix.region_rebuilt;
  auto& layer_active = *context.control.layer_active;
  auto& region_active = *context.control.region_active;
  auto& layer_started = *context.control.layer_started;
  auto& region_started = *context.control.region_started;
  auto& retired_directory = *context.exchange.retired_directory;

  // Directory stages can refuse shared work and return directly here without an engine vote.
  const auto prior_reason = result.reason;
  const auto prior_stop = stats.stop;
  if (!context.exchange.Vote(context, CLocalFailure{})) {
    result.valid = result.surface_prepared = false;
    if (prior_stop == EngineStop::WorkBudget && !prior_reason.empty())
      result.reason = prior_reason;
    else if (!prior_reason.empty() && prior_reason != result.reason)
      result.reason += " Prior failure: " + prior_reason;
  }
  if (region_active && options.timings)
    stats.reconstruction_seconds += std::chrono::duration<double>(Clock::now() - region_started).count();
  region_active = false;
  if (layer_active && options.timings)
    stats.layer_seconds += std::chrono::duration<double>(Clock::now() - layer_started).count();
  layer_active = false;
  ProveEntitySurface(context, working, atlas_budget);
  ProvePreparedSurface(context, working);
  ProveFinalHeights(context, working);
  result.rebuilt_domain = result.valid && region_rebuilt;
  if (stats.stop == EngineStop::None)
    stats.stop = result.valid || result.surface_prepared ? EngineStop::Complete : EngineStop::Failure;
  ObserveRejectedState(context, working);
  CaptureFinalCounts(context, working);
  CaptureDirectoryDonorStatistics(context, retired_directory, atlas_budget);
  EnrichFinalReasonAndTiming(context, curved, working, started, started_cpu);
  return EmitFinalRowsAndMove(context, working, atlas_budget, started, started_cpu);
}

bool EngineControl::BeginLengthGeneration(EngineContext& context, bool collapse) {
  auto& attempted_at_window = *context.control.attempted_at_window;
  auto& construction_claims = *context.exchange.construction_claims;
  auto& construction_generation_changed = *context.setup.control_prefix.construction_generation_changed;
  auto& deferred_at_window = *context.control.deferred_at_window;
  auto& directory = *context.exchange.directory;
  auto& field_identity = *context.census.field_identity;
  auto& phase_start_round = *context.setup.control_prefix.phase_start_round;
  auto& progress_at_window = *context.control.progress_at_window;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& working = *context.ownership.working;

  SU2NativeTiming3D::Scope scheduling_timer(SU2NativeTiming3D::Operation::Scheduling);
  if (!context.exchange.Admit(
          context,
          transfer_memory::Add(transfer_memory::Mul(working.size(), context.control.SelectionBound(context, false)),
                               transfer_memory::Mul(transfer_memory::Mul(collapse ? 12 : 6, working.size()),
                                                    sizeof(Edge) + 4 * sizeof(void*)))))
    return false;
  CLocalFailure failure;
  try {
    selection.queues.lane = Selection::Lane::Surface;
    construction_claims = true;
    stats.split_lane = collapse ? 0 : 1;
    if (!collapse) {
      stats.split_generation = stats.length_split_passes + 1;
      selection.RefreshHistory(true);
    }
    directory->BeginConstructionPass();
    context.queues.LengthDirection(context, collapse);
    selection.ChangePhase(false, working, field_identity);
  } catch (const std::exception& error) {
    failure.Set(2, 0, error.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  construction_generation_changed = false;
  phase_start_round = stats.rounds;
  progress_at_window = stats.size_progress + stats.quality_progress + stats.cell_removals + stats.geometry_progress;
  attempted_at_window = stats.attempted;
  deferred_at_window = stats.length_pass_deferred;
  return true;
}

bool EngineControl::SurfaceReady(EngineContext& context) {
  auto& selection = *context.census.selection;
  auto& world = context.world;

  // Known debt already proves unreadiness; scan coverage only at a possible readiness boundary.
  return !world.maximum(selection.census.construction_invalid != 0 || !context.census.ScopeCensusComplete(context));
}

bool EngineControl::EnterSurfaceRepair(EngineContext& context, SU2NativeGeometry::AtlasBudget& atlas_budget) {
  auto& attempted_at_window = *context.control.attempted_at_window;
  auto& construction_claims = *context.exchange.construction_claims;
  auto& deferred_at_window = *context.control.deferred_at_window;
  auto& directory = *context.exchange.directory;
  auto& field_identity = *context.census.field_identity;
  auto& phase_start_round = *context.setup.control_prefix.phase_start_round;
  auto& progress_at_window = *context.control.progress_at_window;
  auto& repairing = *context.control.repairing;
  auto& result = *context.control.result;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& working = *context.ownership.working;

  if (SurfaceReady(context)) {
    result.surface_prepared = true;
    result.reason.clear();
    return true;
  }
  // Use the existing repair service without enabling shape measurement or its optimizer.
  repairing = true;
  context.queues.LengthDirection(context, false);
  directory->EndConstructionPass();
  construction_claims = false;
  CLocalFailure failure;
  try {
    selection.ChangePhase(true, working, field_identity);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  phase_start_round = stats.rounds;
  progress_at_window = stats.size_progress + stats.cell_removals + stats.geometry_progress;
  attempted_at_window = stats.attempted;
  deferred_at_window = stats.length_pass_deferred;
  return EngineControl::ObserveProgress(context, working, atlas_budget, "surface_repair_entry");
}

void EngineControl::PreparePhaseDirection(EngineContext& context, EnginePhase next, bool refine_geometry) {
  using Phase = EnginePhase;
  bool& bidirectional = context.bidirectional;
  auto& construction_claims = *context.exchange.construction_claims;
  auto& directory = *context.exchange.directory;
  auto& generation_window_count = *context.control.generation_window_count;
  auto& improvement_changed = *context.setup.control_prefix.improvement_changed;
  auto& length_blocks_dirty = *context.queues.length_blocks_dirty;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& world = context.world;
  if (bidirectional) {
    construction_claims = next == Phase::Construction;
    stats.split_lane = 0;
    if (next == Phase::Construction) {
      selection.collapse_closed = context.options.close_construction_collapse;
      generation_window_count = 0;
      selection.queues.edge_history.clear();
      if (world.maximum(improvement_changed)) length_blocks_dirty = {{true, true}};
      if (!refine_geometry) directory->BeginConstructionPass();
    } else
      directory->EndConstructionPass();
    if (!refine_geometry) context.queues.LengthDirection(context, next == Phase::Construction);
  }
}

bool EngineControl::CompletePhaseEntry(EngineContext& context, EnginePhase next, bool refine_geometry,
                                       SU2NativeGeometry::AtlasBudget& atlas_budget) {
  using Phase = EnginePhase;
  auto& composition = context.composition;
  auto& construction_entry_reported = *context.control.construction_entry_reported;
  auto& field_identity = *context.census.field_identity;
  const auto& geometry = context.geometry;
  auto& initial = *context.census.initial;
  auto& options = context.options;
  auto& repairing = *context.control.repairing;
  auto& retired_edges = *context.census.retired_edges;
  auto& seed_slot = *context.census.seed_slot;
  auto& selection = *context.census.selection;
  const auto& started = *context.setup.control_prefix.started;
  auto& working = *context.ownership.working;
  CLocalFailure failure;
  try {
    SU2NativeTiming3D::Scope scheduling_timer(SU2NativeTiming3D::Operation::Scheduling);
    if (!refine_geometry) selection.ChangePhase(repairing, working, field_identity);
    initial.clear();
    initial.reserve(working.size());
    for (const auto& row : working) {
      const auto found = selection.census.cells.find(row.first);
      if (found == selection.census.cells.end() ||
          (repairing &&
           (!found->second.shape_measured || (!options.wall_heights.empty() && !found->second.debt.height_measured))))
        initial.push_back(row.first);
    }
    seed_slot = 0;
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  // A phase census has no enclosing update timer to debit for donor imports.
  const bool measured = context.census.Measure(context, initial, false, nullptr);
  retired_edges.clear();
  if (!measured) return false;
  seed_slot = initial.size();
  if (refine_geometry && !composition && options.wall_heights.empty() && geometry.EntityGeometry()) {
    if (!context.exchange.Admit(context, selection.GeometryPriorityBytes())) return false;
    CLocalFailure priority_failure;
    try {
      selection.PrioritizeGeometrySplits(started, options.maximum_seconds);
    } catch (const std::exception& e) {
      priority_failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, priority_failure)) return false;
  }
  if (!repairing) construction_entry_reported = true;
  if (repairing) {
    bool owned = false;
    {
      SU2NativeTiming3D::Scope ownership_timer(SU2NativeTiming3D::Operation::Communication);
      owned = context.ownership.PhaseOwnership(context, true, atlas_budget);
    }
    if (!owned) return false;
  }
  return EngineControl::ObserveProgress(context, working, atlas_budget,
                                        repairing ? "quality_entry" : "construction_entry");
}

bool EngineControl::EnterPhase(EngineContext& context, EnginePhase next, bool refine_geometry,
                               SU2NativeGeometry::AtlasBudget& atlas_budget) {
  using Phase = EnginePhase;
  auto& attempted_at_window = *context.control.attempted_at_window;
  auto& construction_changed = *context.setup.control_prefix.construction_changed;
  auto& construction_entry_reported = *context.control.construction_entry_reported;
  auto& construction_generation_changed = *context.setup.control_prefix.construction_generation_changed;
  auto& construction_handoff_progress = *context.setup.control_prefix.construction_handoff_progress;
  auto& deferred_at_window = *context.control.deferred_at_window;
  auto& improvement_changed = *context.setup.control_prefix.improvement_changed;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  auto& phase = *context.setup.control_prefix.phase;
  auto& phase_start_round = *context.setup.control_prefix.phase_start_round;
  auto& progress_at_window = *context.control.progress_at_window;
  auto& repairing = *context.control.repairing;
  auto& stats = context.control.result->stats;
  auto& surface_target = context.surface_target;
  auto& working = *context.ownership.working;
  auto& world = context.world;

  if (auto& stall = context.setup.control_prefix.quality_stall) stall->ResetPhase();
  if (surface_target && next == Phase::Improvement) {
    return EnterSurfaceRepair(context, atlas_budget);
  }

  metric_cleanup = false;
  PreparePhaseDirection(context, next, refine_geometry);

  if (phase == Phase::Construction && !construction_entry_reported) {
    if (!EngineControl::ObserveProgress(context, working, atlas_budget, "construction_entry")) return false;
    construction_entry_reported = true;
  }
  construction_handoff_progress = next == Phase::Improvement && world.maximum(construction_changed);
  if (next == Phase::Construction) {
    construction_changed = false;
    construction_generation_changed = false;
  }
  phase = next;
  repairing = next == Phase::Improvement;
  improvement_changed = false;
  phase_start_round = stats.rounds;
  progress_at_window = stats.size_progress + stats.quality_progress + stats.cell_removals + stats.geometry_progress;
  attempted_at_window = stats.attempted;
  deferred_at_window = stats.length_pass_deferred;
  if (repairing)
    ++stats.quality_stages;
  else
    ++stats.construction_returns;
  if (!context.exchange.Admit(
          context, transfer_memory::Add(
                       transfer_memory::Mul(working.size(), sizeof(Id)),
                       transfer_memory::Mul(working.size(), context.control.SelectionBound(context, repairing)))))
    return false;
  if (refine_geometry && !BeginLengthGeneration(context, false)) return false;
  return CompletePhaseEntry(context, next, refine_geometry, atlas_budget);
}

void EngineControl::DiscardCheckpoint(EngineContext& context) {
  auto& checkpoint = *context.control.checkpoint;
  auto& directory = *context.exchange.directory;

  if (checkpoint.directory) context.exchange.RetireDirectory(checkpoint.directory->Statistics());
  checkpoint.directory.reset();
  checkpoint.parameters.reset();
  Owned().swap(checkpoint.cells);
  decltype(checkpoint.blocked)().swap(checkpoint.blocked);
  std::set<Edge>().swap(checkpoint.edges);
  std::set<Edge>().swap(checkpoint.attempted);
  std::set<Id>().swap(checkpoint.changed);
  checkpoint.blocked_links = 0;
  checkpoint.valid = checkpoint.trial = false;
}

bool EngineControl::CaptureCheckpoint(EngineContext& context) {
  auto& checkpoint = *context.control.checkpoint;
  auto& directory = *context.exchange.directory;
  const auto& geometry = context.geometry;
  auto& options = context.options;
  auto& private_parameters = *context.setup.private_parameters;
  auto& result = *context.control.result;
  auto& selection = *context.census.selection;
  auto& source = *context.exchange.source;
  auto& stats = context.control.result->stats;
  auto& working = *context.ownership.working;
  auto& world = context.world;

  // This call follows the complete fresh quality census, including measured strict height.
  // No mutation intervenes; no initial/construction/partial state is presumed certified.
  CLocalFailure failure;
  const auto extra = transfer_memory::Add(
      MapBytes(working), directory->ResidentBytes(), private_parameters ? private_parameters->ResidentBytes() : 0,
      MapBytes(selection.queues.blocked_edges),
      transfer_memory::Mul(selection.queues.blocked_links, sizeof(Id) + 4 * sizeof(void*)),
      transfer_memory::Mul(transfer_memory::Mul(6, working.size()), sizeof(Edge) + 4 * sizeof(void*)));
  if (!context.exchange.Admit(context, extra)) return false;
  try {
    checkpoint.cells = working;
    checkpoint.blocked = selection.queues.blocked_edges;
    checkpoint.blocked_links = selection.queues.blocked_links;
    for (const auto& row : working)
      for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = i + 1; j < 4; ++j) {
          Edge edge{row.second.cell.v[i].id, row.second.cell.v[j].id};
          std::sort(edge.begin(), edge.end());
          checkpoint.edges.insert(edge);
        }
    if (private_parameters) checkpoint.parameters = std::make_unique<BoundaryParameterState>(*private_parameters);
    checkpoint.directory = std::make_unique<Directory>(world, true, geometry.Parameters());
  } catch (const std::exception& e) {
    failure.Set(3, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  // Prebuild while both directories see the checkpoint parameter contents. This dormant
  // directory is never queried during the speculative trial; rollback needs no allocation.
  if (!checkpoint.directory->Build(
          checkpoint.cells, result.reason, options.memory_ceiling,
          transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), directory->ResidentBytes())))
    return false;
  checkpoint.global_cells = TimedAllreduce(working.size(), CPassiveComm::Op::SUM, world.comm);
  checkpoint.collapses = result.collapses.size();
  checkpoint.splits = result.splits.size();
  checkpoint.valid = true;
  checkpoint.trial = false;
  ++stats.coarsen_checkpoints;
  return true;
}

bool EngineControl::RollbackCheckpoint(EngineContext& context, SU2NativeGeometry::AtlasBudget& atlas_budget) {
  using Phase = EnginePhase;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& checkpoint = *context.control.checkpoint;
  auto& construction_changed = *context.setup.control_prefix.construction_changed;
  auto& construction_handoff_progress = *context.setup.control_prefix.construction_handoff_progress;
  auto& directory = *context.exchange.directory;
  auto& improvement_changed = *context.setup.control_prefix.improvement_changed;
  auto& private_parameters = *context.setup.private_parameters;
  auto& result = *context.control.result;
  auto& retired_edges = *context.census.retired_edges;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& working = *context.ownership.working;
  auto& world = context.world;

  static_assert(std::is_nothrow_swappable<BoundaryParameterState>::value,
                "Private checkpoint parameter contents must restore without allocation.");
  CLocalFailure failure;
  size_t retired = 0;
  if (!checkpoint.valid || !checkpoint.directory || checkpoint.collapses > result.collapses.size() ||
      checkpoint.splits > result.splits.size())
    failure.Set(1, 0, "Native coarsening rollback lacks its complete checkpoint.");
  if (bool(private_parameters) != bool(checkpoint.parameters) ||
      (private_parameters && private_parameters->Authority().get() != checkpoint.parameters->Authority().get()))
    failure.Set(1, 0, "Native coarsening rollback changes immutable geometry authority.");
  if (!context.exchange.Vote(context, failure)) return false;
  if (!context.exchange.Admit(
          context, transfer_memory::Mul(checkpoint.attempted.size(), 2 * (sizeof(Edge) + 8 * sizeof(void*)))))
    return false;
  try {
    for (const auto& edge : checkpoint.attempted) {
      auto key = edge;
      std::sort(key.begin(), key.end());
      if (!checkpoint.edges.count(key)) continue;  // Descendants are absent after rollback.
      const bool inserted = checkpoint.blocked[edge[1]].insert(edge[0]).second;
      checkpoint.blocked_links += inserted;
      retired += inserted;
    }
  } catch (const std::exception& e) {
    failure.Set(3, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  if (!world.maximum(retired != 0)) {
    result.reason = "Native coarsening trial made no net reduction and retired no new checkpoint obligation.";
    return false;
  }
  // All proposal/import/private-parameter scopes have ended at this phase boundary. Restore
  // local ownership and exact identities together, leaving IDs/work/deadline/service epochs monotone.
  if (private_parameters) std::swap(*private_parameters, *checkpoint.parameters);
  working.swap(checkpoint.cells);
  directory.swap(checkpoint.directory);
  selection.queues.blocked_edges.swap(checkpoint.blocked);
  std::swap(selection.queues.blocked_links, checkpoint.blocked_links);
  stats.coarsen_rolled_back_collapses += result.collapses.size() - checkpoint.collapses;
  result.collapses.resize(checkpoint.collapses);
  result.splits.resize(checkpoint.splits);
  ++stats.coarsen_rolled_back_passes;
  stats.coarsen_retired_directions += retired;
  stats.coarsen_restored_cells += working.size();
  DiscardCheckpoint(context);
  selection.Reset(true);
  retired_edges.clear();
  campaign_edges.clear();
  construction_changed = construction_handoff_progress = improvement_changed = false;
  // A strict checkpoint is not permission to return a stale census or to forgive a budget exit.
  return EnterPhase(context, Phase::Improvement, false, atlas_budget);
}

// Decisions at existing round boundaries; only an operator round advances the round index.
enum class RoundDecision { Proceed, Restart, Finish, Break };

// Borrows the live round state, preserving caller-owned phase and lane timers.
class RoundHead {
  using Phase = EnginePhase;

 public:
  RoundHead(EngineContext& context, RoundState& round_state, SU2NativeGeometry::AtlasBudget& atlas_budget, bool curved);
  template <class Finish>
  static std::optional<EngineResult> SurfacePreparedExit(EngineContext&, Finish& finish);
  template <class Pick>
  RoundDecision Decide(Pick& pick);

  std::optional<EngineResult> finished;

 private:
  EngineContext& context;
  RoundState& round_state;
  SU2NativeGeometry::AtlasBudget& atlas_budget;
  bool curved;
  std::optional<ContextValue<bool>> clean_quality_lifetime;

  RoundDecision FinishRound();
  void MeasureCleanQuality();
  template <class Pick>
  RoundDecision SeedIdleSurfaceRank(Pick& pick);
  RoundDecision CheckSelectionDeadline();
  RoundDecision StartRequiredSurfacePass();
  RoundDecision RefillEntryCensus();
  RoundDecision CloseCoarseningEntry();
  RoundDecision EndMetricCleanup();
  RoundDecision StartSizeRepair();
  RoundDecision StartGeometryRepair();
  RoundDecision RollbackStalledCoarsening();
  RoundDecision CycleCoarseningCheckpoint();
  RoundDecision ReturnToConstruction();
  RoundDecision StartInteriorSplits();
  RoundDecision ResumeSurfaceSplits();
  RoundDecision EndLengthGeneration();
  RoundDecision AdvanceRefinementGeneration();
  RoundDecision CloseRefinement();
  RoundDecision CheckScheduledDeadline();
  RoundDecision StartMetricCleanup();
  RoundDecision AcceptCompleteMesh();
  RoundDecision RequestFineRepair();
  RoundDecision CheckStagnationWindow();
  RoundDecision StopWithoutCandidate();
  RoundDecision CheckRoundLimit();
};

// Stores references only; the clean-quality value is constructed at its original decision.
RoundHead::RoundHead(EngineContext& context, RoundState& round_state, SU2NativeGeometry::AtlasBudget& atlas_budget,
                     bool curved)
    : context(context), round_state(round_state), atlas_budget(atlas_budget), curved(curved) {}

// Finalizes a prepared surface while the phase scope is live and before the lane timer starts.
template <class Finish>
std::optional<EngineResult> RoundHead::SurfacePreparedExit(EngineContext& context, Finish& finish) {
  auto& world = context.world;
  auto& options = context.options;
  auto& surface_target = context.surface_target;
  auto& started = *context.setup.control_prefix.started;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& repairing = *context.control.repairing;
  if (surface_target && (result.surface_prepared || (repairing && context.control.SurfaceReady(context)))) {
    if (world.maximum(stats.global_work > stats.work_limit)) {
      result.surface_prepared = false;
      stats.stop = EngineStop::WorkBudget;
      result.reason = "Surface preparation exhausted work before complete readiness.";
      return finish();
    }
    if (options.maximum_seconds &&
        world.maximum(std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)) {
      result.surface_prepared = false;
      stats.stop = EngineStop::TimeBudget;
      result.reason = "Surface preparation exhausted elapsed time before complete readiness.";
      return finish();
    }
    result.surface_prepared = true;
    result.reason.clear();
    return finish();
  }
  return std::nullopt;
}

// Executes finalization before any timer or failure object in the deciding block unwinds.
RoundDecision RoundHead::FinishRound() {
  auto& working = *context.ownership.working;
  auto& started = *context.setup.control_prefix.started;
  auto& started_cpu = *context.setup.control_prefix.started_cpu;
  finished.emplace(EngineControl::Finish(context, curved, working, atlas_budget, started, started_cpu));
  return RoundDecision::Finish;
}

// A clean quality census is only needed after the entry and refill decisions.
void RoundHead::MeasureCleanQuality() {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& selection = *context.census.selection;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  // Amortize the strict census over a useful repair pass before retrying outstanding construction.
  clean_quality_lifetime.emplace(
      round_state.clean_quality,
      !metric_cleanup && phase == Phase::Improvement &&
          !world.maximum(selection.census.quality_invalid != 0 || selection.census.unmeasured_shape != 0 ||
                         selection.census.cells.size() != working.size()));
}

// Refills one idle census rank while other ranks have work.
template <class Pick>
RoundDecision RoundHead::SeedIdleSurfaceRank(Pick& pick) {
  auto& world = context.world;
  auto& surface_target = context.surface_target;
  auto& phase = *context.setup.control_prefix.phase;
  auto& selection = *context.census.selection;
  auto& seed_slot = *context.census.seed_slot;
  auto& initial = *context.census.initial;
  auto& coarsening = *context.control.coarsening;
  auto& choice = *round_state.choice;
  auto& any = *round_state.any;
  auto& bad_field = *round_state.bad_field;
  auto& complete = *round_state.complete;
  auto& refill = *round_state.refill;
  if (surface_target && phase == Phase::Construction && any &&
      world.maximum(!choice.active && seed_slot < initial.size())) {
    // One idle-rank census batch must not drain ahead of the active round.
    if (!context.census.SeedNext(context, !choice.active)) return FinishRound();
    bad_field = world.maximum(selection.census.invalid != 0);
    if (!pick()) return FinishRound();
    complete = !world.maximum(selection.census.invalid != 0 || seed_slot < initial.size() ||
                              !selection.census.progress_consistent || (coarsening && !selection.queues.queue.empty()));
    refill = !complete && !any && world.maximum(seed_slot < initial.size());
  }
  return RoundDecision::Proceed;
}

// Checks the deadline after selection and the idle-rank refill.
RoundDecision RoundHead::CheckSelectionDeadline() {
  auto& world = context.world;
  auto& options = context.options;
  auto& started = *context.setup.control_prefix.started;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  if (options.maximum_seconds &&
      world.maximum(std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)) {
    stats.stop = EngineStop::TimeBudget;
    result.reason = "Native 3D kernel elapsed-time budget exhausted; feasibility is undetermined.";
    return FinishRound();
  }
  return RoundDecision::Proceed;
}

// Services required surface work before an optional collapse sweep.
RoundDecision RoundHead::StartRequiredSurfacePass() {
  auto& world = context.world;
  auto& surface_target = context.surface_target;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  if (surface_target && phase == Phase::Construction && !repairing && coarsening &&
      world.maximum(selection.census.construction_invalid != 0)) {
    // Required surface debt must not wait for an exhaustive optional short-edge sweep.
    if (!context.control.BeginLengthGeneration(context, false)) return FinishRound();
    ++stats.surface_required_handoffs;
    if (!EngineControl::ObserveProgress(context, working, atlas_budget, "surface_required_entry")) return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Refills the census before the first coarsening checkpoint.
RoundDecision RoundHead::RefillEntryCensus() {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& selection = *context.census.selection;
  auto& seed_slot = *context.census.seed_slot;
  auto& initial = *context.census.initial;
  auto& repairing = *context.control.repairing;
  auto& standalone_coarsening = *context.control.standalone_coarsening;
  auto& ready = *context.control.ready;
  auto& checkpoint = *context.control.checkpoint;
  auto& refill = *round_state.refill;
  // Before its first strict checkpoint, coarsening must finish the edge-only entry census.
  // A size/geometry-valid input can then establish its checkpoint in the quality phase before
  // destructive removals; invalid inputs still use construction until a strict closure exists.
  if (standalone_coarsening && !checkpoint.valid && phase == Phase::Construction && !repairing &&
      world.maximum(selection.census.cells.size() != working.size()) && world.maximum(seed_slot < initial.size()))
    refill = true;
  if (refill) {
    ready = context.census.SeedNext(context);
    if (!ready) return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Starts improvement when entry allows a strict coarsening checkpoint.
RoundDecision RoundHead::CloseCoarseningEntry() {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& selection = *context.census.selection;
  auto& repairing = *context.control.repairing;
  auto& standalone_coarsening = *context.control.standalone_coarsening;
  auto& checkpoint = *context.control.checkpoint;
  if (standalone_coarsening && !checkpoint.valid && phase == Phase::Construction && !repairing &&
      !world.maximum(selection.census.cells.size() != working.size() || selection.census.construction_invalid != 0)) {
    if (!context.control.EnterPhase(context, Phase::Improvement, false, atlas_budget)) return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Returns to improvement when the optional cleanup runs out of candidates.
RoundDecision RoundHead::EndMetricCleanup() {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  auto& any = *round_state.any;
  if (metric_cleanup && !any) {
    if (world.maximum(!selection.Progress(working.size()).complete)) {
      result.reason = "Native3D metric cleanup lost its complete improvement census.";
      return FinishRound();
    }
    ++stats.length_collapse_passes;
    if (!EngineControl::ObserveProgress(context, working, atlas_budget, "metric_cleanup_exit") ||
        !context.control.EnterPhase(context, Phase::Improvement, false, atlas_budget))
      return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Enables residual size repairs after the clean quality census.
RoundDecision RoundHead::StartSizeRepair() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& clean_quality = *round_state.clean_quality;
  // Close shape/height first, then give existing topology repair the residual length debt.
  if (bidirectional && clean_quality && !selection.size_repairs &&
      world.maximum(selection.census.debt.size_bad_cells != 0)) {
    if (!context.exchange.Admit(context,
                                transfer_memory::Mul(working.size(), context.control.SelectionBound(context, true))))
      return FinishRound();
    CLocalFailure failure;
    try {
      selection.EnableSizeRepairs(working);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure) ||
        !EngineControl::ObserveProgress(context, working, atlas_budget, "size_repair_entry"))
      return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Enables geometry-only repair after the other requirements have closed.
RoundDecision RoundHead::StartGeometryRepair() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& clean_quality = *round_state.clean_quality;
  // Geometry-only demand gets its own pass after shape, height and size have closed.
  if (bidirectional && clean_quality && !selection.geometry_repairs &&
      !world.maximum(selection.census.debt.size_bad_cells != 0) &&
      world.maximum(selection.census.construction_invalid != 0)) {
    if (!context.exchange.Admit(context,
                                transfer_memory::Mul(working.size(), context.control.SelectionBound(context, true))))
      return FinishRound();
    CLocalFailure failure;
    try {
      selection.EnableGeometryRepairs(working);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure) ||
        !EngineControl::ObserveProgress(context, working, atlas_budget, "geometry_repair_entry"))
      return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Restores the checkpoint when its quality closure has stalled.
RoundDecision RoundHead::RollbackStalledCoarsening() {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& standalone_coarsening = *context.control.standalone_coarsening;
  auto& checkpoint = *context.control.checkpoint;
  auto& any = *round_state.any;
  auto& clean_quality = *round_state.clean_quality;
  if (standalone_coarsening && phase == Phase::Improvement && checkpoint.valid && !any && !clean_quality &&
      !world.maximum(selection.census.cells.size() != working.size()) && world.maximum(checkpoint.trial)) {
    if (stats.global_work >= stats.work_limit) {
      stats.stop = EngineStop::WorkBudget;
      result.reason = "Native coarsening quality closure exhausted its work budget.";
      return FinishRound();
    }
    if (!context.control.RollbackCheckpoint(context, atlas_budget)) return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Checks the trial checkpoint against its entry cell count and remaining demand.
RoundDecision RoundHead::CycleCoarseningCheckpoint() {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& standalone_coarsening = *context.control.standalone_coarsening;
  auto& checkpoint = *context.control.checkpoint;
  auto& clean_quality = *round_state.clean_quality;
  if (standalone_coarsening && clean_quality && !world.maximum(selection.census.construction_invalid != 0)) {
    if (checkpoint.valid && world.maximum(checkpoint.trial)) {
      const size_t cells = TimedAllreduce(working.size(), CPassiveComm::Op::SUM, world.comm);
      if (cells >= checkpoint.global_cells) {
        if (!context.control.RollbackCheckpoint(context, atlas_budget)) return FinishRound();
        return RoundDecision::Restart;
      }
      ++stats.coarsen_committed_passes;
      context.control.DiscardCheckpoint(context);
    }
    if (!world.maximum(selection.HasCoarseningDemand())) {
      result.valid = true;
      result.reason.clear();
      return FinishRound();
    }
    if (!checkpoint.valid) {
      if (!context.control.CaptureCheckpoint(context) ||
          !context.control.EnterPhase(context, Phase::Construction, false, atlas_budget))
        return FinishRound();
      return RoundDecision::Restart;
    }
  }
  return RoundDecision::Proceed;
}

// Hands residual geometry or coarsening demand back to construction.
RoundDecision RoundHead::ReturnToConstruction() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& selection = *context.census.selection;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  auto& construction_handoff_progress = *context.setup.control_prefix.construction_handoff_progress;
  auto& improvement_changed = *context.setup.control_prefix.improvement_changed;
  auto& standalone_coarsening = *context.control.standalone_coarsening;
  auto& any = *round_state.any;
  auto& clean_quality = *round_state.clean_quality;
  const auto& stall = context.setup.control_prefix.quality_stall;
  const bool request = stall && selection.collapse_closed && stall->request;
  const auto decision = quality_stall::EvaluateHandoff(
      metric_cleanup, phase == Phase::Improvement,
      [&] { return world.maximum(improvement_changed) || (construction_handoff_progress && clean_quality); },
      [&] { return standalone_coarsening || world.maximum(selection.census.construction_invalid != 0); },
      [&] {
        return !world.maximum(selection.census.quality_invalid != 0) && !selection.size_repairs &&
               !selection.geometry_repairs;
      },
      any, request);
  if (context.options.timings && request)
    quality_stall::PrintHandoff(context.control.result->stats.rounds, decision);
  if (decision.Taken()) {
    const auto progress = selection.Progress(working.size());
    // An exhausted shape pass needs its residual surface rebuilt before another global coarsening sweep.
    const bool refine_geometry = bidirectional &&
                                 !world.maximum(!progress.complete || progress.size_bad_cells ||
                                                progress.height_bad_cells || progress.height_unresolved_cells) &&
                                 world.maximum(progress.geometry_bad_cells != 0);
    if (!EngineControl::ObserveProgress(context, working, atlas_budget, "quality_exit") ||
        !context.control.EnterPhase(context, Phase::Construction, refine_geometry, atlas_budget))
      return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Starts the interior split lane after draining the surface lane.
RoundDecision RoundHead::StartInteriorSplits() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& directory = *context.exchange.directory;
  auto& construction_claims = *context.exchange.construction_claims;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& any = *round_state.any;
  if (bidirectional && phase == Phase::Construction && !repairing && !coarsening && !any &&
      selection.queues.lane == Selection::Lane::Surface &&
      !world.maximum(!context.census.ScopeCensusComplete(context))) {
    if (!context.census.GenerationCensus(context) ||
        !EngineControl::ObserveProgress(context, working, atlas_budget, "split_surface_exit"))
      return FinishRound();
    // Interior publications can reopen surface demand after the full-vertex claims end.
    selection.queues.geometry_split_priorities.clear();
    selection.queues.geometry_priority_mode = false;
    directory->EndConstructionPass();
    construction_claims = false;
    selection.queues.lane = Selection::Lane::Interior;
    stats.split_lane = 2;
    if (!EngineControl::ObserveProgress(context, working, atlas_budget, "split_interior_entry")) return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Reopens the surface lane when interior work uncovers more demand.
RoundDecision RoundHead::ResumeSurfaceSplits() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& directory = *context.exchange.directory;
  auto& construction_claims = *context.exchange.construction_claims;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& any = *round_state.any;
  if (bidirectional && phase == Phase::Construction && !repairing && !coarsening && !any &&
      selection.queues.lane == Selection::Lane::Interior &&
      world.maximum(!selection.queues.surface_queue.empty() || !selection.queues.geometry_queue.empty())) {
    // Interior publications may reveal/reopen entry-edge surface demand. Drain it before ending
    // the finite generation; newly created edges still cannot enter its immutable frontier.
    if (!context.census.GenerationCensus(context) ||
        !EngineControl::ObserveProgress(context, working, atlas_budget, "split_interior_exit"))
      return FinishRound();
    directory->BeginConstructionPass();
    construction_claims = true;
    selection.queues.lane = Selection::Lane::Surface;
    stats.split_lane = 1;
    if (!EngineControl::ObserveProgress(context, working, atlas_budget, "split_surface_resume")) return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Closes a bidirectional generation and chooses its existing successor.
RoundDecision RoundHead::EndLengthGeneration() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& directory = *context.exchange.directory;
  auto& length_blocks_dirty = *context.queues.length_blocks_dirty;
  auto& construction_generation_changed = *context.setup.control_prefix.construction_generation_changed;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& generation_window = *context.control.generation_window;
  auto& generation_window_count = *context.control.generation_window_count;
  auto& any = *round_state.any;
  if (bidirectional && phase == Phase::Construction && !repairing && !any &&
      !world.maximum(!context.census.ScopeCensusComplete(context))) {
    const bool productive = world.maximum(construction_generation_changed);
    if (!context.census.GenerationCensus(context)) return FinishRound();
    const auto current_debt = context.census.CurrentGenerationDebt(context);
    bool progressing = true;
    if (coarsening) {
      generation_window_count = 1;
      generation_window[0] = current_debt;
    } else {
      if (generation_window_count == generation_window.size()) {
        std::move(generation_window.begin() + 1, generation_window.end(), generation_window.begin());
        --generation_window_count;
      }
      generation_window[generation_window_count++] = current_debt;
      if (generation_window_count == generation_window.size()) {
        const auto& before = generation_window.front();
        auto fell = [](double old, double now) { return now < old - std::max(1e-12, std::abs(old) * 1e-6); };
        progressing = fell(before.edges, current_debt.edges) || fell(before.maximum, current_debt.maximum) ||
                      (before.geometry_complete && current_debt.geometry_complete &&
                       fell(before.geometry, current_debt.geometry));
      }
    }
    if (productive) length_blocks_dirty = {{true, true}};
    if (coarsening)
      ++stats.length_collapse_passes;
    else
      ++stats.length_split_passes;
    EngineReports::EmitLengthPass(context, atlas_budget);
    if (coarsening && selection.collapse_probe) {
      const auto& probe = *selection.collapse_probe;
      try {
        std::ostringstream row;
        row << "NATIVE3D_FP3_ENGINE_COLLAPSE rank=" << world.rank << " accepted=" << probe.accepted
            << " removed_volume=" << probe.volume << " removed_surface_2=" << probe.surface[2]
            << " removed_surface_1=" << probe.surface[1] << " removed_surface_0=" << probe.surface[0]
            << " unclassified=" << probe.unclassified << " shared=" << probe.shared << " local=" << probe.local
            << " old_cells=" << probe.old.total << " old_bad=" << probe.old.bad
            << " old_unmeasured=" << probe.old.unmeasured << " fresh_cells=" << probe.fresh.total
            << " fresh_bad=" << probe.fresh.bad << " fresh_unmeasured=" << probe.fresh.unmeasured
            << " status=" << (probe.incomplete ? "incomplete" : "complete") << '\n';
        std::cout << row.str();
      } catch (...) {}
      selection.collapse_probe.reset();
    }
    if (!EngineControl::ObserveProgress(context, working, atlas_budget,
                                        coarsening ? "length_collapse_exit" : "length_split_exit"))
      return FinishRound();
    if (!coarsening) {
      // Descendant size demand stays in construction. Once size closes, give shape/height
      // improvement a turn before more geometry-only splitting; residual geometry can return here.
      const bool size_demand = world.maximum(selection.census.debt.size_bad_cells != 0);
      if (productive && progressing && size_demand) {
        if (stats.global_work >= stats.work_limit) {
          stats.stop = EngineStop::WorkBudget;
          result.reason = "Native3D construction generation exhausted global work; feasibility is undetermined.";
          return FinishRound();
        }
        if (!context.control.BeginLengthGeneration(context, false)) return FinishRound();
        return RoundDecision::Restart;
      }
      if (productive && !progressing) ++stats.construction_stall_handoffs;
      if (!context.control.EnterPhase(context, Phase::Improvement, false, atlas_budget)) return FinishRound();
    } else {
      // The major direction boundary is quiescent; no repartition is evaluated at lane/generation switches.
      context.queues.LengthDirection(context, false);
      directory->EndConstructionPass();
      bool owned = false;
      {
        SU2NativeTiming3D::Scope ownership_timer(SU2NativeTiming3D::Operation::Communication);
        owned = context.ownership.PhaseOwnership(context, false, atlas_budget);
      }
      if (!owned || !context.control.BeginLengthGeneration(context, false)) return FinishRound();
    }
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Refreshes the finite frontier for another unidirectional refinement pass.
RoundDecision RoundHead::AdvanceRefinementGeneration() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& field_identity = *context.census.field_identity;
  auto& construction_generation_changed = *context.setup.control_prefix.construction_generation_changed;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& any = *round_state.any;
  if (!bidirectional && !coarsening && phase == Phase::Construction && !repairing && !any) {
    // New edges belong to the next finite generation; productive construction need not remeasure quality first.
    const bool census = world.maximum(!selection.Progress(working.size()).census_complete) == 0;
    const bool demanded = world.maximum(selection.census.construction_invalid != 0);
    const bool productive = world.maximum(construction_generation_changed);
    if (census && demanded && productive) {
      if (stats.global_work >= stats.work_limit) {
        stats.stop = EngineStop::WorkBudget;
        result.reason = "Native3D construction generation exhausted global work; feasibility is undetermined.";
        return FinishRound();
      }
      if (!context.exchange.Admit(context,
                                  transfer_memory::Mul(working.size(), context.control.SelectionBound(context, false))))
        return FinishRound();
      CLocalFailure failure;
      try {
        selection.ChangePhase(false, working, field_identity);
      } catch (const std::exception& e) {
        failure.Set(2, 0, e.what());
      }
      if (!context.exchange.Vote(context, failure)) return FinishRound();
      construction_generation_changed = false;
      return RoundDecision::Restart;
    }
  }
  return RoundDecision::Proceed;
}

// Starts improvement when unidirectional construction can end.
RoundDecision RoundHead::CloseRefinement() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& selection = *context.census.selection;
  auto& repairing = *context.control.repairing;
  auto& any = *round_state.any;
  auto& complete = *round_state.complete;
  if (!bidirectional && phase == Phase::Construction && !repairing &&
      (complete || (!any && !world.maximum(selection.census.cells.size() != working.size())))) {
    if (!context.control.EnterPhase(context, Phase::Improvement, false, atlas_budget)) return FinishRound();
    return RoundDecision::Restart;
  }
  return RoundDecision::Proceed;
}

// Checks the second deadline under the original Scheduling timer.
RoundDecision RoundHead::CheckScheduledDeadline() {
  auto& world = context.world;
  auto& options = context.options;
  auto& started = *context.setup.control_prefix.started;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  Timer timer(stats.selection_seconds, options.timings, SU2NativeTiming3D::Operation::Scheduling);
  if (options.maximum_seconds &&
      world.maximum(std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)) {
    stats.stop = EngineStop::TimeBudget;
    result.reason = "Native 3D kernel elapsed-time budget exhausted; feasibility is undetermined.";
    return FinishRound();
  }
  return RoundDecision::Proceed;
}

// Admits the single optional short-edge pass after quality stalls.
RoundDecision RoundHead::StartMetricCleanup() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& phase_start_round = *context.setup.control_prefix.phase_start_round;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& field_identity = *context.census.field_identity;
  auto& directory = *context.exchange.directory;
  auto& construction_claims = *context.exchange.construction_claims;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  auto& metric_cleanup_tried = *context.setup.control_prefix.metric_cleanup_tried;
  auto& repairing = *context.control.repairing;
  auto& progress_at_window = *context.control.progress_at_window;
  auto& attempted_at_window = *context.control.attempted_at_window;
  auto& deferred_at_window = *context.control.deferred_at_window;
  auto& any = *round_state.any;
  auto& clean_quality = *round_state.clean_quality;
  // One finite, shape-preserving short-edge opportunity can change a stalled repair's neighbors.
  // A mesh that already satisfies every gate needs no optional cleanup before acceptance.
  if (bidirectional && !metric_cleanup && !metric_cleanup_tried && phase == Phase::Improvement && !any &&
      !clean_quality) {
    const auto progress = selection.Progress(working.size());
    if (!world.maximum(!progress.complete || progress.geometry_bad_cells || progress.size_bad_cells ||
                       progress.height_bad_cells || progress.height_unresolved_cells)) {
      bool short_edges = false;
      for (const auto& edge : selection.census.edges) short_edges |= edge.second.measure.length < CoarseningMetricEdge;
      if (world.maximum(short_edges)) {
        if (!context.exchange.Admit(context, transfer_memory::Mul(transfer_memory::Mul(12, working.size()),
                                                                  sizeof(Edge) + 4 * sizeof(void*))))
          return FinishRound();
        CLocalFailure failure;
        try {
          directory->BeginConstructionPass();
          construction_claims = true;
          selection.collapse_closed = false;
          context.queues.LengthDirection(context, true);
          selection.ChangePhase(false, working, field_identity);
        } catch (const std::exception& e) {
          failure.Set(2, 0, e.what());
        }
        if (!context.exchange.Vote(context, failure)) return FinishRound();
        metric_cleanup = metric_cleanup_tried = true;
        repairing = false;
        phase_start_round = stats.rounds;
        progress_at_window =
            stats.size_progress + stats.quality_progress + stats.cell_removals + stats.geometry_progress;
        attempted_at_window = stats.attempted;
        deferred_at_window = stats.length_pass_deferred;
        if (!EngineControl::ObserveProgress(context, working, atlas_budget, "metric_cleanup_entry"))
          return FinishRound();
        return RoundDecision::Restart;
      }
    }
  }
  return RoundDecision::Proceed;
}

// Finalizes an improvement state whose existing census gates pass.
RoundDecision RoundHead::AcceptCompleteMesh() {
  auto& world = context.world;
  auto& phase = *context.setup.control_prefix.phase;
  auto& result = *context.control.result;
  auto& selection = *context.census.selection;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  auto& complete = *round_state.complete;
  if (!metric_cleanup && complete && phase == Phase::Improvement &&
      !world.maximum(selection.census.unmeasured_shape != 0 || selection.census.quality_invalid != 0 ||
                     selection.census.construction_invalid != 0)) {
    result.valid = true;
    result.reason.clear();
    return FinishRound();
  }
  return RoundDecision::Proceed;
}

// Reopens stuck vertices at the last quality head and repicks only when a rank reopened work.
RoundDecision RoundHead::RequestFineRepair() {
  auto& selection = *context.census.selection;
  auto& any = *round_state.any;
  const bool repairing = *context.control.repairing;
  if (any || !repairing || *context.setup.control_prefix.phase != Phase::Improvement || context.surface_target ||
      selection.geometry_repairs || selection.size_repairs || *context.setup.control_prefix.metric_cleanup)
    return RoundDecision::Proceed;

  const bool timings = context.options.timings;
  size_t stuck = 0, reopened = 0;
  CLocalFailure failure;
  try {
    reopened = selection.RequestFineMoves(timings ? &stuck : nullptr);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  auto& world = context.world;
  const int disposition =
      CPassiveComm::Allreduce(failure.Failed() ? 2 : int(reopened != 0), CPassiveComm::Op::MAX, world.comm);
  if (disposition == 2) {
    context.exchange.Vote(context, failure);
    return FinishRound();
  }
  if (!disposition) return RoundDecision::Proceed;
  if (timings && reopened)
    std::cout << "NATIVE3D_FINE_REQUEST rank=" << world.rank << " round=" << context.control.result->stats.rounds
              << " stuck=" << stuck << " reopened=" << reopened << '\n';
  auto& choice = *round_state.choice;
  choice = selection.Best(repairing);
  any = world.maximum(choice.active);
  return RoundDecision::Proceed;
}

// Checks rank-wide progress and ownership under the original Scheduling timer.
RoundDecision RoundHead::CheckStagnationWindow() {
  auto& world = context.world;
  auto& options = context.options;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& phase = *context.setup.control_prefix.phase;
  auto& phase_start_round = *context.setup.control_prefix.phase_start_round;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  auto& repairing = *context.control.repairing;
  auto& progress_at_window = *context.control.progress_at_window;
  auto& attempted_at_window = *context.control.attempted_at_window;
  auto& deferred_at_window = *context.control.deferred_at_window;
  auto& round = round_state.round;
  Timer timer(stats.selection_seconds, options.timings, SU2NativeTiming3D::Operation::Scheduling);
  if (stats.rounds > phase_start_round && round && round % WindowRounds(options, repairing) == 0) {
    ++stats.progress_windows;
    const uint64_t progress =
        stats.size_progress + stats.quality_progress + stats.cell_removals + stats.geometry_progress;
    const bool mesh_progress = world.maximum(progress != progress_at_window);
    const bool search_progress = world.maximum(stats.attempted != attempted_at_window ||
                                               (bidirectional && stats.length_pass_deferred != deferred_at_window));
    progress_at_window = progress;
    attempted_at_window = stats.attempted;
    deferred_at_window = stats.length_pass_deferred;
    // Valid coarsening may exhaust rejected directions without changing an already admissible mesh.
    if (!mesh_progress) ++stats.stagnant_windows;
    if (!mesh_progress && search_progress) ++stats.search_windows;
    if (!EngineControl::ObserveProgress(context, working, atlas_budget, "window")) return FinishRound();
    if (!metric_cleanup && phase == Phase::Improvement &&
        !context.ownership.PhaseOwnership(context, true, atlas_budget))
      return FinishRound();
    auto& stall = context.setup.control_prefix.quality_stall;
    if (quality_stall::Active(stall ? &*stall : nullptr, selection.collapse_closed, phase == Phase::Improvement,
                              repairing, metric_cleanup))
      quality_stall::Observe(*stall, selection.Progress(working.size()), world.comm, options.timings, stats.rounds);
    // Fresh rejected directions are finite search work even on invalid meshes; do not declare infeasibility early.
    if (!mesh_progress && !search_progress) {
      if (!bidirectional && phase == Phase::Construction && !repairing &&
          !world.maximum(selection.census.cells.size() != working.size())) {
        if (!context.control.EnterPhase(context, Phase::Improvement, false, atlas_budget)) return FinishRound();
        return RoundDecision::Restart;
      }
      stats.stop = EngineStop::OperatorStagnation;
      result.reason =
          "Native 3D admitted operators made no certified progress over the observation window; feasibility is "
          "undetermined.";
      return FinishRound();
    }
  }
  return RoundDecision::Proceed;
}

// Elects the existing no-candidate failure before finalizing.
RoundDecision RoundHead::StopWithoutCandidate() {
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& any = *round_state.any;
  if (!any) {
    stats.stop = EngineStop::OperatorStagnation;
    CLocalFailure failure;
    if (selection.census.invalid || selection.census.cells.size() != working.size())
      failure.Set(1, 0,
                  "Native 3D adaptation stagnates: admitted operators cannot satisfy the frozen size/shape gates.");
    context.exchange.Vote(context, failure);
    return FinishRound();
  }
  return RoundDecision::Proceed;
}

// Stops before allocating another numbered operator round.
RoundDecision RoundHead::CheckRoundLimit() {
  auto& options = context.options;
  auto& round = round_state.round;
  // A final census/phase transition consumes no additional operator round.
  if (options.maximum_rounds && round >= options.maximum_rounds) return RoundDecision::Break;
  return RoundDecision::Proceed;
}

// Applies the remaining head decisions in the same order as their collective guards.
template <class Pick>
RoundDecision RoundHead::Decide(Pick& pick) {
  if (const auto decision = SeedIdleSurfaceRank(pick); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = CheckSelectionDeadline(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = StartRequiredSurfacePass(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = RefillEntryCensus(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = CloseCoarseningEntry(); decision != RoundDecision::Proceed) return decision;
  MeasureCleanQuality();
  if (const auto decision = EndMetricCleanup(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = StartSizeRepair(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = StartGeometryRepair(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = RollbackStalledCoarsening(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = CycleCoarseningCheckpoint(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = ReturnToConstruction(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = StartInteriorSplits(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = ResumeSurfaceSplits(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = EndLengthGeneration(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = AdvanceRefinementGeneration(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = CloseRefinement(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = CheckScheduledDeadline(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = StartMetricCleanup(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = AcceptCompleteMesh(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = RequestFineRepair(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = CheckStagnationWindow(); decision != RoundDecision::Proceed) return decision;
  if (const auto decision = StopWithoutCandidate(); decision != RoundDecision::Proceed) return decision;
  return CheckRoundLimit();
}
