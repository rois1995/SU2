// Owns observer and finalization flow; borrows canonical state and adds no policy.
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
