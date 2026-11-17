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
      proof_phase.emplace(SU2NativeTiming3D::Phase::FinalProof);
      proof_scope.emplace(SU2NativeTiming3D::Operation::Certification);
    }
    CLocalFailure geometryFailure;
    try {
      D9QueryScope d9_query(SU2NativeD9::Site::Finish, &geometry);
      ProveOwnedSurface(world, working, geometry, atlas_budget, stats);
    } catch (const std::exception& e) {
      geometryFailure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, geometryFailure)) {
      result.valid = result.surface_prepared = false;
      stats.stop = EngineStop::Failure;
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
      proof_phase.emplace(SU2NativeTiming3D::Phase::FinalProof);
      proof_scope.emplace(SU2NativeTiming3D::Operation::Certification);
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
      proof_phase.emplace(SU2NativeTiming3D::Phase::FinalProof);
      proof_scope.emplace(SU2NativeTiming3D::Operation::Certification);
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
  stats.cached_measured_cells = selection.cells.size();
  stats.cached_invalid_cells = selection.invalid;
  stats.remaining_edges = selection.queue.size() + selection.surface_queue.size() + selection.geometry_queue.size();
  stats.remaining_repairs = selection.repair_queue.size();
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
  if (curved && !result.valid && !selection.cells.empty()) {
    size_t metric_bad = 0, geometry_bad = 0;
    for (const auto& row : selection.cells) {
      metric_bad += !row.second.target_valid;
      geometry_bad += row.second.geometry_invalid;
    }
    result.reason += " Local cached demands: geometry_cells=" + std::to_string(geometry_bad) +
                     ", metric_or_height_cells=" + std::to_string(metric_bad) +
                     ", geometry_edges=" + std::to_string(selection.geometry_queue.size()) +
                     ", metric_edges=" + std::to_string(selection.queue.size()) + ".";
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
  ObserveProgress(context, working, atlas_budget, "exit");
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
                                    SU2NativeGeometry::AtlasBudget& atlas_budget, const char* stage) {
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

  if (!context.exchange.Admit(
          context,
          transfer_memory::Add(transfer_memory::Mul(working.size(), context.control.SelectionBound(context, false)),
                               transfer_memory::Mul(transfer_memory::Mul(collapse ? 12 : 6, working.size()),
                                                    sizeof(Edge) + 4 * sizeof(void*)))))
    return false;
  CLocalFailure failure;
  try {
    selection.lane = Selection::Lane::Surface;
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
  return !world.maximum(selection.construction_invalid != 0 || !context.census.ScopeCensusComplete(context));
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
      generation_window_count = 0;
      selection.edge_history.clear();
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
    if (!refine_geometry) selection.ChangePhase(repairing, working, field_identity);
    initial.clear();
    initial.reserve(working.size());
    for (const auto& row : working) {
      const auto found = selection.cells.find(row.first);
      if (found == selection.cells.end() ||
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
  if (repairing && !context.ownership.PhaseOwnership(context, true, atlas_budget)) return false;
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
      MapBytes(selection.blocked_edges), transfer_memory::Mul(selection.blocked_links, sizeof(Id) + 4 * sizeof(void*)),
      transfer_memory::Mul(transfer_memory::Mul(6, working.size()), sizeof(Edge) + 4 * sizeof(void*)));
  if (!context.exchange.Admit(context, extra)) return false;
  try {
    checkpoint.cells = working;
    checkpoint.blocked = selection.blocked_edges;
    checkpoint.blocked_links = selection.blocked_links;
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
  selection.blocked_edges.swap(checkpoint.blocked);
  std::swap(selection.blocked_links, checkpoint.blocked_links);
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
