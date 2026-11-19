// Adapt orchestration. Every helper owns one original lifetime block through its continuation.
struct EngineRun {
  EngineContext& context;
  SU2NativeGeometry::AtlasBudget& atlas_budget;
  const SU2NativeGeometry::BoundaryCoverage& no_coverage;
  bool curved = false;
  std::optional<EngineResult> finished = std::nullopt;
  static constexpr size_t diagnostic_rejection_limit = 64;
  using Phase = EnginePhase;

  // Finalizes the result while the originating timer and cavity scopes are still active.
  EngineResult Finish() {
    return EngineControl::Finish(context, curved, *context.ownership.working, atlas_budget,
                                 *context.setup.control_prefix.started, *context.setup.control_prefix.started_cpu);
  }

  // Carries the already-produced result across exactly one round/slot control boundary.
  RoundDecision Done(EngineResult result) {
    finished.emplace(std::move(result));
    return RoundDecision::Finish;
  }

  // Binds the control and census owners before frontier setup.
  EngineResult Run() {
    const auto& started = *context.setup.control_prefix.started;
    auto& parameter_bound = *context.setup.control_prefix.parameter_bound;
    auto& world = context.world;
    auto& geometry = context.geometry;
    auto& composition = context.composition;
    auto& options = context.options;
    auto& intent = context.intent;
    auto& bidirectional = context.bidirectional;
    auto& surface_target = context.surface_target;
    auto& wall_counters = *context.setup.wall_counters;
    auto& control = context.control;
    if (options.timings)
      PrintAdaptationRequirements(MakeAdaptationRequirements(geometry, bool(composition), options), world.rank,
                                  std::cout);
    ContextValue parameter_setup_lifetime(
        control.parameter_setup, options.timings ? std::chrono::duration<double>(Clock::now() - started).count() : 0);
    const auto& parameter_setup = parameter_setup_lifetime.Get();
    curved = !geometry.ExtensionComponents().empty();
    ContextValue standalone_coarsening_lifetime(control.standalone_coarsening, intent == MetricChange::Coarsen);
    const auto& standalone_coarsening = standalone_coarsening_lifetime.Get();
    ContextValue coarsening_lifetime(control.coarsening, standalone_coarsening || bidirectional);
    auto& coarsening = coarsening_lifetime.Get();
    ContextValue repairing_lifetime(control.repairing, false);
    auto& repairing = repairing_lifetime.Get();
    ContextValue construction_kind_lifetime(control.construction_kind,
                                            coarsening ? MetricChange::Coarsen : MetricChange::Refine);
    ContextValue tracked_bytes_begin_lifetime(control.tracked_bytes_begin, world.bytes_sent);
    ContextValue tracked_calls_begin_lifetime(control.tracked_calls_begin, world.collective_calls);
    ContextValue tracked_seconds_begin_lifetime(control.tracked_seconds_begin, world.collective_seconds);
    ContextValue election_calls_begin_lifetime(control.election_calls_begin, world.election_collective_calls);
    ContextValue election_seconds_begin_lifetime(control.election_seconds_begin, world.election_collective_seconds);
    ContextValue geometry_authority_lifetime(control.geometry_authority,
                                             geometry.Parameters() ? &geometry.Parameters()->Geometry() : nullptr);
    auto& geometry_authority = geometry_authority_lifetime.Get();
    ContextValue geometry_begin_lifetime(control.geometry_begin,
                                         geometry_authority ? geometry_authority->GeometryStatsSnapshot()
                                                            : SU2NativeGeometry::MappedFacetReference::GeometryStats{});
    ContextValue result_lifetime(control.result);
    auto& result = result_lifetime.Get();
    auto& stats = result.stats;
    stats.bidirectional = bidirectional;
    stats.parameter_setup_seconds = parameter_setup;
    stats.initialization_seconds = parameter_setup;
    stats.maximum_work_bytes = parameter_bound;
    ContextValue working_lifetime(context.ownership.working);
    ContextValue selection_lifetime(context.census.selection, coarsening, repairing, geometry, stats, options.timings,
                                    options.surface_tolerance, options.wall_heights, bidirectional,
                                    bool(surface_target), &wall_counters);
    auto& selection = selection_lifetime.Get();
    selection.queues.split_lanes = bidirectional;
    ContextValue construction_claims_lifetime(context.exchange.construction_claims, bidirectional);
    ContextValue edge_progress_lifetime(context.census.edge_progress);
    ContextValue edge_census_round_lifetime(context.census.edge_census_round, UINT64_MAX);
    return InitializeFrontier();
  }

  // Owns the directory, source and frontier state through the event.
  EngineResult InitializeFrontier() {
    auto& working = *context.ownership.working;
    auto& selection = *context.census.selection;
    auto& world = context.world;
    auto& geometry = context.geometry;
    auto& accepted = context.accepted;
    auto& composition = context.composition;
    auto& options = context.options;
    auto& control = context.control;
    ContextValue generation_window_lifetime(control.generation_window);
    ContextValue generation_window_count_lifetime(control.generation_window_count, 0);
    ContextValue retired_edges_lifetime(context.census.retired_edges);
    ContextValue campaign_edges_lifetime(context.census.campaign_edges);
    ContextValue inactive_blocked_edges_lifetime(context.queues.inactive_blocked_edges);
    ContextValue inactive_blocked_links_lifetime(context.queues.inactive_blocked_links, 0);
    ContextValue length_blocks_dirty_lifetime(context.queues.length_blocks_dirty, std::array<bool, 2>{{false, false}});
    ContextValue field_identity_lifetime(context.census.field_identity);
    // Joint split feasibility depends on neighboring vertex stars, even when the edge itself is unchanged.
    selection.track_splits = bool(composition) || curved;
    // Every adaptation can enter quality improvement; rejected repair support must be invalidated
    // even for flat, uncomposed refinement/repair events.
    ContextValue directory_lifetime(context.exchange.directory,
                                    std::make_unique<Directory>(world, true, geometry.Parameters()));
    ContextValue source_lifetime(context.exchange.source, world, working, options.timings);
    ContextValue checkpoint_lifetime(control.checkpoint);
    ContextValue retired_directory_lifetime(context.exchange.retired_directory);
    ContextValue accepted_bytes_lifetime(control.accepted_bytes, MapBytes(accepted));
    ContextValue initial_lifetime(context.census.initial);
    ContextValue eligible_extensions_lifetime(context.census.eligible_extensions);
    ContextValue seed_slot_lifetime(context.census.seed_slot, 0);
    ContextValue private_repair_lifetime(context.exchange.private_repair, false);
    ContextValue deferred_votes_lifetime(context.exchange.deferred_votes, EngineExchange::DeferredVotes{});
    ContextValue private_optimizer_allowance_lifetime(control.private_optimizer_allowance, 0);
    ContextValue batch_bytes_lifetime(control.batch_bytes, 0);
    ContextValue diagnostic_bytes_lifetime(control.diagnostic_bytes,
                                           transfer_memory::Add(16384, transfer_memory::Mul(world.size, 4096)));
    ContextValue fixed_metric_points_lifetime(control.fixed_metric_points);
    ContextValue fixed_metric_cells_lifetime(control.fixed_metric_cells);
    auto& fixed_metric_cells = fixed_metric_cells_lifetime.Get();
    fixed_metric_cells.fill(UINT64_MAX);
    ContextValue fixed_metric_initialized_lifetime(control.fixed_metric_initialized, false);
    ContextValue fixed_metric_available_lifetime(control.fixed_metric_available, false);
    ContextValue batch_changed_lifetime(context.commit.batch_changed);
    ContextValue batch_touched_lifetime(context.commit.batch_touched);
    ContextValue batch_supports_lifetime(context.commit.batch_supports);
    ContextValue last_rejection_lifetime(control.last_rejection);
    ContextValue diagnostic_rejections_lifetime(control.diagnostic_rejections);
    auto& diagnostic_rejections = diagnostic_rejections_lifetime.Get();
    ContextValue diagnostic_sequence_lifetime(control.diagnostic_sequence, 0);
    if (options.stage_observer &&
        options.stage_observer_bytes >= diagnostic_rejection_limit * sizeof(RejectionDiagnostic) + (size_t(64) << 10))
      diagnostic_rejections.reserve(diagnostic_rejection_limit);
    ContextValue progress_at_window_lifetime(control.progress_at_window, 0);
    ContextValue attempted_at_window_lifetime(control.attempted_at_window, 0);
    ContextValue deferred_at_window_lifetime(control.deferred_at_window, 0);
    return InitializeObservers();
  }

  // Keeps the original observer votes and quality ledger alive.
  EngineResult InitializeObservers() {
    auto& world = context.world;
    auto& options = context.options;
    auto& control = context.control;
    ContextValue observe_rejections_lifetime(
        control.observe_rejections, world.maximum(bool(options.rejected_trial) || bool(options.transaction_observer)));
    ContextValue observe_state_lifetime(control.observe_state, world.maximum(bool(options.rejected_state)));
    ContextValue observe_stage_lifetime(control.observe_stage, world.maximum(bool(options.stage_observer)));
    ContextValue stage_storage_admitted_lifetime(control.stage_storage_admitted, false);
    ContextValue stage_observer_failed_lifetime(control.stage_observer_failed, false);
    ContextValue construction_entry_reported_lifetime(control.construction_entry_reported, false);
    ContextValue previous_quality_lifetime(control.previous_quality);
    ContextValue previous_quality_census_lifetime(control.previous_quality_census);
    ContextValue previous_quality_elapsed_lifetime(control.previous_quality_elapsed, 0);
    ContextValue previous_quality_collective_lifetime(control.previous_quality_collective, 0);
    ContextValue quality_census_serial_lifetime(control.quality_census_serial, 0);
    ContextValue previous_quality_construction_lifetime(control.previous_quality_construction, 0);
    ContextValue previous_quality_ownership_lifetime(control.previous_quality_ownership, 0);
    ContextValue quality_census_started_lifetime(control.quality_census_started, false);
    ContextValue transaction_captured_lifetime(control.transaction_captured, false);
    ContextValue layer_active_lifetime(control.layer_active, false);
    ContextValue region_active_lifetime(control.region_active, false);
    ContextValue layer_started_lifetime(control.layer_started);
    ContextValue region_started_lifetime(control.region_started);
    return InitializeWorkingMesh();
  }

  // Admits observation and creates the initial mesh and indexes.
  EngineResult InitializeWorkingMesh() {
    const auto& observe_stage = *context.control.observe_stage;
    auto& stage_storage_admitted = *context.control.stage_storage_admitted;
    auto& options = context.options;
    auto& control = context.control;
    auto& stats = context.control.result->stats;
    if (observe_stage) {
      stage_storage_admitted = context.exchange.Admit(context, 0);
      if (!stage_storage_admitted) return Finish();
    }
    ContextValue ready_lifetime(control.ready, false);
    auto& ready = ready_lifetime.Get();
    ContextValue maximum_node_lifetime(context.commit.maximum_node, 0);
    auto& maximum_node = maximum_node_lifetime.Get();
    ContextValue maximum_cell_lifetime(context.commit.maximum_cell, 0);
    auto& maximum_cell = maximum_cell_lifetime.Get();
    {
      Timer timer(stats.initialization_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
      CLocalFailure failure;
      EngineSetup::CheckInitialNumericPolicies(context, curved, atlas_budget, failure);
      EngineSetup::CheckInitialWallPolicies(context, failure);
      ready = context.exchange.Vote(context, failure);
      EngineSetup::ExchangeInitialWallPolicies(context, failure, ready);
      EngineSetup::PopulateInitialStateAndBuildIndexes(context, curved, failure, ready, maximum_node, maximum_cell);
    }
    if (!ready) return Finish();
    auto setup_finish = [&]() { return Finish(); };
    if (auto done = EngineSetup::RefreshWorkingSurface(context, atlas_budget, setup_finish)) {
      return std::move(*done);
    }
    return InitializeRegion(setup_finish);
  }

  // Runs the initial region and first-row stages.
  template <class SetupFinish>
  EngineResult InitializeRegion(SetupFinish& setup_finish) {
    auto& coarsening = *context.control.coarsening;
    auto& repairing = *context.control.repairing;
    auto& region_active = *context.control.region_active;
    auto& region_started = *context.control.region_started;
    auto& world = context.world;
    auto& geometry = context.geometry;
    auto& original = context.original;
    auto& options = context.options;
    auto& surface_target = context.surface_target;
    // A complete visible radial region is constructed as one transaction, never as a larger local cavity.
    const auto global_original = TimedAllreduce(uint64_t(original.size()), CPassiveComm::Op::SUM, world.comm);
    const bool prismatic =
        !surface_target && !repairing && options.region_walls.size() == options.wall_heights.size() &&
        (!coarsening || options.wall_heights.empty()) && PrismaticWallRegion(geometry, options.region_walls);
    bool region_applicable = !surface_target && curved && !repairing && options.region_walls.size() <= 1 &&
                             options.region_walls.size() == options.wall_heights.size() &&
                             (!coarsening || options.wall_heights.empty());
    if (!EngineSetup::CheckRegionApplicability(context, region_applicable, prismatic)) return Finish();
    if (region_applicable) {
      region_active = true;
      region_started = Clock::now();
      CLocalFailure failure;
      auto region_done = EngineSetup::ConstructRegion(context, global_original, failure, setup_finish);
      if (region_done) return std::move(*region_done);
    }
    auto first_rows_done = EngineSetup::InitializeFirstRows(context, setup_finish);
    if (first_rows_done) return std::move(*first_rows_done);
    return SeedAndRun(setup_finish);
  }

  // Keeps the ownership state through seeding and rounds.
  template <class SetupFinish>
  EngineResult SeedAndRun(SetupFinish& setup_finish) {
    auto& coarsening = *context.control.coarsening;
    auto& working = *context.ownership.working;
    auto& selection = *context.census.selection;
    auto& initial = *context.census.initial;
    auto& seed_slot = *context.census.seed_slot;
    auto& ready = *context.control.ready;
    auto& world = context.world;
    auto& surface_target = context.surface_target;
    // Construction requests missing edges only; improvement also imports centroid support.
    ContextValue ownership_evaluated_lifetime(context.ownership.ownership_evaluated,
                                              std::array<bool, 2>{{false, false}});
    ContextValue quality_ownership_round_lifetime(context.ownership.quality_ownership_round, 0);
    EngineSetup::BeginConstructionPass(context);
    if (!context.exchange.Admit(context, transfer_memory::Mul(transfer_memory::Mul(coarsening ? 12 : 6, working.size()),
                                                              sizeof(Edge) + 4 * sizeof(void*))))
      return Finish();
    {
      CLocalFailure failure;
      if (!EngineSetup::SeedFrontier(context, failure)) return Finish();
    }
    // Discover required surface work before optional collapses, without delaying a known deficit for the full census.
    do {
      ready = context.census.SeedNext(context);
      if (!ready) return Finish();
    } while (surface_target && !world.maximum(selection.census.construction_invalid != 0) &&
             world.maximum(seed_slot < initial.size()));
    return RunRounds(setup_finish);
  }

  // Runs successive rounds and certifies after their state is destroyed.
  template <class SetupFinish>
  EngineResult RunRounds(SetupFinish& setup_finish) {
    auto& phase = *context.setup.control_prefix.phase;
    auto& improvement_changed = *context.setup.control_prefix.improvement_changed;
    auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
    const auto& standalone_coarsening = *context.control.standalone_coarsening;
    auto& coarsening = *context.control.coarsening;
    auto& result = *context.control.result;
    auto& working = *context.ownership.working;
    auto& selection = *context.census.selection;
    auto& world = context.world;
    auto& options = context.options;
    auto& stats = context.control.result->stats;
    for (RoundState round_state;;) {
      const auto decision = RunRound(round_state, setup_finish);
      if (decision == RoundDecision::Finish) return std::move(*finished);
      if (decision == RoundDecision::Break) break;
    }
    {
      Timer timer(stats.completion_seconds, options.timings, SU2NativeTiming3D::Operation::Certification);
      const int incomplete =
          world.maximum(selection.census.cells.size() != working.size()
                            ? 3
                            : (selection.census.invalid ? 2 : (coarsening && !selection.queues.queue.empty() ? 1 : 0)));
      result.valid =
          !metric_cleanup && phase == Phase::Improvement && incomplete == 0 &&
          !world.maximum(selection.census.unmeasured_shape != 0 || selection.census.quality_invalid != 0 ||
                         selection.census.construction_invalid != 0 || (standalone_coarsening && improvement_changed));
      stats.stop = result.valid ? EngineStop::Complete : EngineStop::RoundBudget;
      if (incomplete == 1)
        result.reason =
            "Native 3D coarsening round limit reached with valid cached metric cells and pending collapse candidates.";
      else if (incomplete == 2)
        result.reason =
            "Native 3D adaptation round limit reached before cached metric size/shape gates were satisfied.";
      else if (incomplete == 3)
        result.reason = "Native 3D adaptation round limit reached with incomplete metric-cell measurements.";
      else if (!result.valid)
        result.reason = "Native 3D adaptation round limit reached before strict quality/coarsening completion.";
      else
        result.reason.clear();
    }
    return Finish();
  }

  // Keeps the phase and lane scopes active through the full round.
  template <class SetupFinish>
  RoundDecision RunRound(RoundState& round_state, SetupFinish& setup_finish) {
    auto& phase = *context.setup.control_prefix.phase;
    auto& coarsening = *context.control.coarsening;
    auto& repairing = *context.control.repairing;
    auto& options = context.options;
    auto& bidirectional = context.bidirectional;
    auto& surface_target = context.surface_target;
    auto& stats = context.control.result->stats;
    auto& round = round_state.round;
    std::optional<SU2NativeTiming3D::PhaseScope> phase_scope;
    if (options.timings)
      phase_scope.emplace(phase == Phase::Improvement ? SU2NativeTiming3D::Phase::Quality
                          : surface_target            ? SU2NativeTiming3D::Phase::Surface
                                                      : SU2NativeTiming3D::Phase::Construction);
    if (auto prepared = RoundHead::SurfacePreparedExit(context, setup_finish)) return Done(std::move(*prepared));
    ContextValue timed_lane_lifetime(
        round_state.timed_lane,
        bidirectional && phase == Phase::Construction && !repairing && !coarsening ? stats.split_lane : 0);
    const unsigned& timed_lane = timed_lane_lifetime.Get();
    ContextValue untimed_lane_lifetime(round_state.untimed_lane, 0);
    double& untimed_lane = untimed_lane_lifetime.Get();
    Timer lane_timer(timed_lane == 1   ? stats.surface_seconds
                     : timed_lane == 2 ? stats.interior_seconds
                                       : untimed_lane,
                     options.timings && timed_lane != 0);
    ContextValue choice_lifetime(round_state.choice);
    ContextValue complete_lifetime(round_state.complete, false);
    ContextValue any_lifetime(round_state.any, false);
    ContextValue bad_field_lifetime(round_state.bad_field, false);
    ContextValue refill_lifetime(round_state.refill, false);
    auto pick = [&] { return PickRound(round_state); };
    SelectRound(round_state);
    if (!*context.control.ready) return Done(Finish());
    RoundHead round_head(context, round_state, atlas_budget, curved);
    const auto decision = round_head.Decide(pick);
    if (decision == RoundDecision::Finish) return Done(std::move(*round_head.finished));
    if (decision != RoundDecision::Proceed) return decision;
    round_state.batch = RoundBatch(options, repairing);
    return ReserveHistoryAndAdvance(round_state);
  }

  // Selects the next candidate with the existing vote.
  bool PickRound(RoundState& round_state) {
    auto& coarsening = *context.control.coarsening;
    auto& repairing = *context.control.repairing;
    auto& working = *context.ownership.working;
    auto& selection = *context.census.selection;
    auto& ready = *context.control.ready;
    Choice& choice = *round_state.choice;
    bool& any = *round_state.any;
    bool& bad_field = *round_state.bad_field;
    auto& world = context.world;
    auto& bidirectional = context.bidirectional;
    if (!coarsening) {
      choice = selection.Best(repairing);
      any = world.maximum(choice.active);
      return true;
    }
    CLocalFailure failure;
    try {
      choice = selection.Best(repairing);
      while (!repairing && (!bad_field || bidirectional) && selection.FilterCollapse(choice, working))
        choice = selection.Best(false);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, failure))) return false;
    any = world.maximum(choice.active);
    return true;
  }

  // Measures pending census and refills within the scheduling timer.
  void SelectRound(RoundState& round_state) {
    auto& coarsening = *context.control.coarsening;
    auto& repairing = *context.control.repairing;
    auto& working = *context.ownership.working;
    auto& selection = *context.census.selection;
    auto& initial = *context.census.initial;
    auto& seed_slot = *context.census.seed_slot;
    auto& ready = *context.control.ready;
    bool& complete = *round_state.complete;
    bool& any = *round_state.any;
    bool& bad_field = *round_state.bad_field;
    bool& refill = *round_state.refill;
    auto& world = context.world;
    auto& options = context.options;
    auto& bidirectional = context.bidirectional;
    auto& surface_target = context.surface_target;
    auto& stats = context.control.result->stats;
    {
      Timer timer(stats.selection_seconds, options.timings, SU2NativeTiming3D::Operation::Scheduling);
      ++stats.selections;
      // The surface cursor covers only demand owners. Complete versioned coverage is checked at quiescent boundaries.
      const bool pending_census = surface_target ? seed_slot < initial.size() || !selection.census.progress_consistent
                                                 : selection.census.cells.size() != working.size();
      int dirty = world.maximum(
          selection.census.invalid ? 2 : (pending_census || (coarsening && !selection.queues.queue.empty()) ? 1 : 0));
      bad_field = dirty == 2;
      if (coarsening && !repairing && (!bad_field || bidirectional) && !selection.prune_collapses) {
        // Joint construction filters necessary validity constraints even while target debt remains.
        CLocalFailure failure;
        try {
          selection.EnableCollapseFilter();
        } catch (const std::exception& e) {
          failure.Set(2, 0, e.what());
        }
        ready = context.exchange.Vote(context, failure);
        if (ready) dirty = world.maximum(pending_census || !selection.queues.queue.empty());
      }
      if (ready) {
        complete = dirty == 0;
        PickRound(round_state);
        if (ready && coarsening && !repairing && !bad_field && !any)
          complete = !world.maximum(pending_census || !selection.queues.queue.empty());
        if (ready && !complete && !any) refill = world.maximum(seed_slot < initial.size());
      }
    }
  }

  // Admits the history and advances the epoch before acquiring dependencies.
  RoundDecision ReserveHistoryAndAdvance(RoundState& round_state) {
    auto& coarsening = *context.control.coarsening;
    auto& repairing = *context.control.repairing;
    auto& result = *context.control.result;
    auto& selection = *context.census.selection;
    auto& ready = *context.control.ready;
    auto& options = context.options;
    auto& stats = context.control.result->stats;
    {
      Timer timer(stats.selection_seconds, options.timings, SU2NativeTiming3D::Operation::Scheduling);
      const size_t used = coarsening ? result.collapses.size() : result.splits.size();
      const size_t capacity = coarsening ? result.collapses.capacity() : result.splits.capacity();
      const size_t needed = transfer_memory::Add(used, options.candidate_batch);
      const size_t next = std::min<uint64_t>(stats.work_limit, std::max(needed, transfer_memory::Mul(capacity, 2)));
      CLocalFailure failure;
      ready = context.exchange.Admit(context,
                                     !repairing && next > capacity
                                         ? transfer_memory::Mul(next, coarsening ? sizeof(Edge) : sizeof(SplitRecord))
                                         : 0);
      if (ready && !repairing && next > capacity) {
        if (ready) try {
            if (coarsening)
              result.collapses.reserve(next);
            else
              result.splits.reserve(next);
          } catch (const std::exception& e) {
            failure.Set(2, 0, e.what());
          }
      }
      // Every worker votes even if its history already has room.
      if (ready) ready = context.exchange.Vote(context, failure);
      if (!ready) return Done(Finish());
    }
    {
      CLocalFailure failure;
      if (selection.queues.repair_epoch == UINT64_MAX) failure.Set(1, 0, "Native repair service epoch overflows.");
      if (!context.exchange.Vote(context, failure)) return Done(Finish());
      ++selection.queues.repair_epoch;
    }
    return AcquireRound(round_state);
  }

  // Keeps acquisition and its reserved containers alive through slot publication.
  RoundDecision AcquireRound(RoundState& round_state) {
    auto& coarsening = *context.control.coarsening;
    auto& repairing = *context.control.repairing;
    auto& selection = *context.census.selection;
    const unsigned& timed_lane = *round_state.timed_lane;
    bool& bad_field = *round_state.bad_field;
    auto& world = context.world;
    auto& composition = context.composition;
    auto& options = context.options;
    auto& bidirectional = context.bidirectional;
    auto& stats = context.control.result->stats;
    ++stats.rounds;
    if (timed_lane == 1)
      ++stats.surface_rounds;
    else if (timed_lane == 2)
      ++stats.interior_rounds;
    ContextValue concurrent_lifetime(
        round_state.concurrent,
        (repairing || ((composition || curved || selection.queues.split_lanes) && !coarsening) ||
         (bidirectional && coarsening)) &&
            options.candidate_batch > 1 && world.size > 1);
    const bool& concurrent = concurrent_lifetime.Get();
    stats.concurrent_rounds += concurrent;
    // Repair uses one complete round snapshot, including the one-candidate correctness path.
    ContextValue batched_lifetime(
        round_state.batched,
        repairing || (options.candidate_batch > 1 && (!(selection.queues.split_lanes && !coarsening) && !curved &&
                                                      !composition && (!coarsening || !bad_field))));
    ContextValue choices_lifetime(round_state.choices);
    ContextValue dependency_batch_lifetime(round_state.dependency_batch);
    ContextValue imported_batch_lifetime(round_state.imported_batch);
    ContextValue reserved_lifetime(round_state.reserved);
    ContextValue slots_lifetime(round_state.slots, 1);
    ContextValue planned_coarsening_lifetime(round_state.planned_coarsening,
                                             coarsening && !repairing && !curved && !composition && world.size > 1);
    // A complete star above the cavity bound blocks only its own proposal, as on every lookup path; changed
    // stars reactivate it. It is never published and never fails the event.
    ContextValue refused_candidates_lifetime(round_state.refused_candidates, 0);
    ContextValue refused_slots_lifetime(round_state.refused_slots, 0);
    ContextValue pass_deferred_candidates_lifetime(round_state.pass_deferred_candidates, 0);
    if (options.timings) {
      round_state.lost_own = 0;
      round_state.lost_other = 0;
    }
    ContextValue refused_receipts_lifetime(round_state.refused_receipts,
                                           std::array<RepairReceipt, MaximumRoundBatch>{});
    ContextValue refused_receipt_count_lifetime(round_state.refused_receipt_count, 0);
    RoundAcquisition acquisition(context, round_state, atlas_budget, curved, diagnostic_rejection_limit);
    if (!acquisition.Acquire()) return Done(std::move(*acquisition.finished));
    return PreparePrivateRound(round_state, acquisition);
  }

  // Allocates private-round state at the original admission points.
  RoundDecision PreparePrivateRound(RoundState& round_state, RoundAcquisition& acquisition) {
    auto& repairing = *context.control.repairing;
    auto& private_optimizer_allowance = *context.control.private_optimizer_allowance;
    auto& batch_bytes = *context.control.batch_bytes;
    std::vector<Choice>& choices = *round_state.choices;
    ImportedBatch& imported_batch = *round_state.imported_batch;
    ReservationBatch& reserved = *round_state.reserved;
    auto& world = context.world;
    auto& stats = context.control.result->stats;
    ContextValue repair_proposals_lifetime(round_state.repair_proposals);
    std::vector<RepairProposal>& repair_proposals = repair_proposals_lifetime.Get();
    ContextValue private_failure_lifetime(round_state.private_failure);
    CLocalFailure& private_failure = private_failure_lifetime.Get();
    ContextValue private_work_used_lifetime(round_state.private_work_used, 0);
    ContextValue private_work_reserved_lifetime(round_state.private_work_reserved, 0);
    uint64_t& private_work_reserved = private_work_reserved_lifetime.Get();
    ContextValue private_deadline_lifetime(round_state.private_deadline, false);
    if (repairing) {
      size_t retained = transfer_memory::Mul(choices.size(), sizeof(RepairProposal) + 1024);
      uint64_t eligible = 0;
      for (size_t i = 0; i < choices.size(); ++i)
        if (reserved.chosen.at(i)) {
          const auto& imported = imported_batch.candidates.at(i);
          const bool coordinate = imported.coordinate_star.count == 1 && imported.cells.size() > MaximumCavityCells;
          eligible += !coordinate;
          retained = transfer_memory::Add(
              retained, transfer_memory::Mul(std::max(MaximumReplacementCells, imported.cells.size()), 16384));
        }
      batch_bytes = transfer_memory::Add(batch_bytes, retained);
      if (!context.exchange.Admit(context, 0)) return Done(Finish());
      try {
        repair_proposals.resize(choices.size());
      } catch (const std::exception& e) {
        private_failure.Set(2, 0, e.what());
      }
      if (!context.exchange.Vote(context, private_failure)) return Done(Finish());
      eligible = TimedAllreduce(eligible, CPassiveComm::Op::SUM, world.comm);
      private_optimizer_allowance =
          eligible ? std::min<uint64_t>(160, (stats.work_limit - stats.global_work) / eligible) : 0;
      private_work_reserved = eligible * private_optimizer_allowance;
      stats.global_work += private_work_reserved;
    }
    ContextValue lane_slots_lifetime(round_state.lane_slots, std::array<lane_census::Slot, MaximumRoundBatch>{});
    ContextValue private_started_lifetime(round_state.private_started, repairing ? Clock::now() : Clock::time_point{});
    ContextValue private_collectives_lifetime(round_state.private_collectives,
                                              world.collective_calls + world.election_collective_calls);
    return RunSlots(round_state, acquisition);
  }

  // Visits each slot and joins the repair batch after every slot has unwound.
  RoundDecision RunSlots(RoundState& round_state, RoundAcquisition& acquisition) {
    auto& repairing = *context.control.repairing;
    auto& batch_bytes = *context.control.batch_bytes;
    std::vector<Choice>& choices = *round_state.choices;
    size_t& slots = *round_state.slots;
    auto& round = round_state.round;
    for (SlotState slot_state; slot_state.slot < (repairing ? choices.size() : slots); ++slot_state.slot) {
      const auto decision = RunSlot(round_state, slot_state, acquisition);
      if (decision == RoundDecision::Finish) return decision;
      if (decision == RoundDecision::Break) break;
    }
    if (repairing) {
      RepairBatchCommit batch_commit(context, round_state, atlas_budget, curved, diagnostic_rejection_limit);
      if (!batch_commit.Run()) return Done(std::move(*batch_commit.finished));
    }
    round_state.token_base =
        transfer_memory::Add(round_state.token_base, transfer_memory::Mul(context.world.size, round_state.batch));
    batch_bytes = 0;
    ++round;
    return RoundDecision::Proceed;
  }

  // Owns the acquired payload and diagnostics through the slot callback.
  RoundDecision RunSlot(RoundState& round_state, SlotState& slot_state, RoundAcquisition& acquisition) {
    auto& directory = *context.exchange.directory;
    auto& stats = context.control.result->stats;
    auto& wall_counters = *context.setup.wall_counters;
    size_t& slot = slot_state.slot;
    ContextValue dependencies_lifetime(slot_state.dependencies);
    ContextValue imported_lifetime(slot_state.imported);
    ContextValue chosen_lifetime(slot_state.chosen, false);
    bool& chosen = chosen_lifetime.Get();
    ContextValue pass_deferred_lifetime(slot_state.pass_deferred, false);
    bool& pass_deferred = pass_deferred_lifetime.Get();
    ContextValue collapse_corrected_lifetime(slot_state.collapse_corrected, false);
    ContextValue collapse_apex_lifetime(slot_state.collapse_apex, 0);
    ContextValue collapse_correction_refusal_lifetime(slot_state.collapse_correction_refusal);
    wall_diagnostics::Choice wall_choice(wall_counters);
    auto reserve_dependencies = [&](const Imported& rows, bool active, std::string& why, size_t ceiling, size_t caller,
                                    unsigned priority) {
      const bool selected = directory->Reserve(rows, active, why, ceiling, caller, priority);
      if (chosen && directory->PassDeferred()) ++stats.length_expansion_deferrals;
      pass_deferred = pass_deferred || directory->PassDeferred();
      return selected;
    };
    SlotAcquisition slot_acquisition(context, round_state, slot_state, atlas_budget, curved);
    const auto slot_decision = slot_acquisition.Run(acquisition, wall_choice, reserve_dependencies);
    if (slot_decision == SlotDecision::Next) return RoundDecision::Proceed;
    if (slot_decision == SlotDecision::Finish) return Done(std::move(*slot_acquisition.finished));
    return PrepareCandidate(round_state, slot_state, wall_choice, reserve_dependencies);
  }

  // Owns trial communication, patches and failure state through the remaining slot.
  template <class Reserve>
  RoundDecision PrepareCandidate(RoundState& round_state, SlotState& slot_state, wall_diagnostics::Choice& wall_choice,
                                 Reserve& reserve_dependencies) {
    auto& repairing = *context.control.repairing;
    auto& selection = *context.census.selection;
    auto& source = *context.exchange.source;
    auto& private_repair = *context.exchange.private_repair;
    Imported& imported = *slot_state.imported;
    auto& world = context.world;
    auto& surface_target = context.surface_target;
    TrialComm trial_comm{private_repair, world, source, imported};
    TrialChecks trial_checks{context, round_state, slot_state, atlas_budget, trial_comm, curved};
    ContextValue failure_lifetime(slot_state.failure);
    ContextValue coordinate_only_lifetime(slot_state.coordinate_only, repairing &&
                                                                          imported.coordinate_star.count == 1 &&
                                                                          imported.cells.size() > MaximumCavityCells);
    ContextValue private_scratch_lifetime(
        slot_state.private_scratch,
        transfer_memory::Add(
            transfer_memory::Bytes(imported.cells),
            transfer_memory::Mul(std::max(MaximumReplacementCells, imported.cells.size()), 8192),
            surface_target ? SurfacePlanningScratchBytes(std::max(MaximumReplacementCells, imported.cells.size())) : 0,
            transfer_memory::Mul(selection.geometry_repairs ? 2 : 1, MaximumFrozenFieldBytes),
            MetricPatchSolveScratchBytes,
            transfer_memory::Mul(3 * MaximumCampaignEdges,
                                 sizeof(std::pair<const Edge, EdgeMeasure>) + 6 * sizeof(void*)),
            transfer_memory::Mul(8 * MaximumReplacementCells, 64)));
    ContextValue old_lifetime(slot_state.old);
    ContextValue fresh_lifetime(slot_state.fresh);
    ContextValue point_lifetime(slot_state.point);
    ContextValue approved_lifetime(slot_state.approved, false);
    ContextValue moved_lifetime(slot_state.moved, false);
    ContextValue repair_approved_lifetime(slot_state.repair_approved, false);
    ContextValue cavity_inserted_lifetime(slot_state.cavity_inserted, false);
    ContextValue optimization_approved_lifetime(slot_state.optimization_approved, false);
    ContextValue repair_optimization_lifetime(slot_state.repair_optimization);
    ContextValue certified_lifetime(slot_state.certified);
    ContextValue geometric_progress_lifetime(slot_state.geometric_progress, false);
    ContextValue last_geometry_operation_lifetime(slot_state.last_geometry_operation, nullptr);
    ContextValue field_lifetime(slot_state.field);
    return PrepareIdentifiers(round_state, slot_state, wall_choice, reserve_dependencies, trial_comm, trial_checks);
  }

  // Binds IDs and preserves the active cavity through evaluation and commit.
  template <class Reserve>
  RoundDecision PrepareIdentifiers(RoundState& round_state, SlotState& slot_state,
                                   wall_diagnostics::Choice& wall_choice, Reserve& reserve_dependencies,
                                   TrialComm& trial_comm, TrialChecks& trial_checks) {
    auto& repairing = *context.control.repairing;
    auto& source = *context.exchange.source;
    Choice& choice = *round_state.choice;
    Imported& imported = *slot_state.imported;
    auto& world = context.world;
    auto& composition = context.composition;
    auto& options = context.options;
    auto& slot = slot_state.slot;
    ContextValue token_lifetime(
        slot_state.token,
        transfer_memory::Add(round_state.token_base, transfer_memory::Mul(world.rank, round_state.batch), slot));
    const size_t& token = token_lifetime.Get();
    ContextValue offset_lifetime(slot_state.offset, transfer_memory::Mul(token, MaximumReplacementCells));
    ContextValue construction_nodes_lifetime(slot_state.construction_nodes, !options.wall_heights.empty()
                                                                                ? MaximumReplacementCells + 3
                                                                            : (composition || curved) ? 9
                                                                                                      : 1);
    const size_t& construction_nodes = construction_nodes_lifetime.Get();
    ContextValue node_stride_lifetime(slot_state.node_stride, construction_nodes + 160);
    const size_t& node_stride = node_stride_lifetime.Get();
    ContextValue node_offset_lifetime(slot_state.node_offset, transfer_memory::Mul(token, node_stride));
    ContextValue capture_lifetime(slot_state.capture, false);
    ContextValue capture_selected_lifetime(slot_state.capture_selected, false);
    ContextValue accepted_ladder_lifetime(slot_state.accepted_ladder, 3);
    ContextValue old_flags_lifetime(slot_state.old_flags);
    ContextValue fresh_flags_lifetime(slot_state.fresh_flags);
    ContextValue measured_flags_lifetime(slot_state.measured_flags);
    ContextValue transaction_feature_lifetime(
        slot_state.transaction_feature, repairing ? choice.feature : Feature{{choice.edge[0], choice.edge[1], 0}, 2});
    auto observe_transaction = [&](const char* stage, bool passed, const DonorPatch* donors = nullptr) {
      ObserveSlotTransaction(round_state, slot_state, stage, passed, donors);
    };
    ContextValue repair_budget_lifetime(slot_state.repair_budget, 0);
    ContextValue repair_work_lifetime(slot_state.repair_work, 0);
    source.SetCavity(&imported.cells);
    struct CavityScope {
      CavityMetricSource& source;
      ~CavityScope() { source.SetCavity(nullptr); }
    } cavity_scope{source};
    return RunSlotTrial(round_state, slot_state, wall_choice, reserve_dependencies, trial_comm, trial_checks,
                        observe_transaction);
  }

  // Passes the same live transaction payload to the observer.
  void ObserveSlotTransaction(RoundState& round_state, SlotState& slot_state, const char* stage, bool passed,
                              const DonorPatch* donors) {
    auto& result = *context.control.result;
    auto& working = *context.ownership.working;
    auto& maximum_node = *context.commit.maximum_node;
    auto& maximum_cell = *context.commit.maximum_cell;
    Dependencies& dependencies = *slot_state.dependencies;
    Imported& imported = *slot_state.imported;
    Patch& old = *slot_state.old;
    Patch& fresh = *slot_state.fresh;
    Node& point = *slot_state.point;
    MetricDiagnosis& certified = *slot_state.certified;
    const size_t& offset = *slot_state.offset;
    const size_t& node_offset = *slot_state.node_offset;
    bool& capture = *slot_state.capture;
    bool& capture_selected = *slot_state.capture_selected;
    const Feature& transaction_feature = *slot_state.transaction_feature;
    auto& geometry = context.geometry;
    auto& composition = context.composition;
    auto& options = context.options;
    auto& round = round_state.round;
    if (capture)
      options.transaction_observer({stage, transaction_feature, uint64_t(round), capture_selected, passed, working,
                                    geometry, dependencies, imported, old, fresh, point, maximum_cell + 1 + offset,
                                    maximum_node + 1 + node_offset, certified, result.reason, donors, composition,
                                    options.edge_breakpoints});
  }

  // Checks the full replacement while preserving the existing exception routing.
  bool BoundaryReplacementReady(SlotState& slot_state) {
    auto& phase = *context.setup.control_prefix.phase;
    auto& result = *context.control.result;
    auto& directory = *context.exchange.directory;
    auto& source = *context.exchange.source;
    const size_t& private_scratch = *slot_state.private_scratch;
    Patch& old = *slot_state.old;
    Patch& fresh = *slot_state.fresh;
    auto& geometry = context.geometry;
    auto& options = context.options;
    if (phase != Phase::Improvement || !geometry.Mapped() || (old.surface.empty() && fresh.surface.empty()))
      return true;
    try {
      geometry.ExportParameters(fresh);
      // Exact unchanged-face contracts avoid rebuilding a parameter delta for interior repairs.
      bool unchanged = old.surface.size() == fresh.surface.size();
      std::map<Face, Id> physical;
      if (unchanged)
        for (const auto& face : old.surface)
          if (!physical.emplace(Key(face.v), face.facet).second) {
            unchanged = false;
            break;
          }
      if (unchanged)
        for (const auto& face : fresh.surface) {
          const auto found = physical.find(Key(face.v));
          if (found == physical.end() || found->second != face.facet) {
            unchanged = false;
            break;
          }
          std::array<BoundaryPoint, 3> points;
          for (unsigned k = 0; k < 3; ++k) points[k] = fresh.parameters.at(face.v[k]);
          if (!geometry.HasAcceptedFaceCertificate(face, points, fresh.coverage)) {
            unchanged = false;
            break;
          }
          physical.erase(found);
        }
      if (unchanged && physical.empty()) return true;
      // The accepted parameter tables are replicated; this is one complete proposal, not an owner fragment.
      return geometry.Parameters()->CheckReplacement(
          old, fresh, result.reason, options.memory_ceiling,
          transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), directory->ResidentBytes(),
                               private_scratch, BoundaryCoverageBytes(old.coverage),
                               BoundaryCoverageBytes(fresh.coverage)),
          geometry.AtlasReservation());
    } catch (const std::bad_alloc&) {
      throw;
    } catch (const std::exception& e) {
      result.reason = e.what();
      return false;
    }
  }

  // Dispatches the existing private and collective trial paths.
  template <class Reserve, class Observe>
  RoundDecision RunSlotTrial(RoundState& round_state, SlotState& slot_state, wall_diagnostics::Choice& wall_choice,
                             Reserve& reserve_dependencies, TrialComm& trial_comm, TrialChecks& trial_checks,
                             Observe& observe_transaction) {
    Choice& choice = *round_state.choice;
    Imported& imported = *slot_state.imported;
    bool& chosen = *slot_state.chosen;
    ContextValue physical_edge_lifetime(
        slot_state.physical_edge,
        chosen && std::any_of(imported.cells.begin(), imported.cells.end(), [&](const OwnedCell& c) {
          const auto faces = Faces(c.cell);
          for (unsigned i = 0; i < 4; ++i)
            if (c.physical[i].present &&
                std::find(faces[i].begin(), faces[i].end(), choice.edge[0]) != faces[i].end() &&
                std::find(faces[i].begin(), faces[i].end(), choice.edge[1]) != faces[i].end())
              return true;
          return false;
        }));
    auto boundary_replacement_ready = [&] { return BoundaryReplacementReady(slot_state); };
    CandidateTrial candidate_trial(context, round_state, slot_state, trial_comm, trial_checks, reserve_dependencies,
                                   boundary_replacement_ready, observe_transaction, wall_choice, curved);
    if (*context.control.repairing)
      return EvaluatePrivateTrial(round_state, slot_state, candidate_trial, boundary_replacement_ready);
    return EvaluateCollectiveSlot(round_state, slot_state, candidate_trial, reserve_dependencies, observe_transaction);
  }

  // Runs the private candidate and publishes its proposal without a collective slot commit.
  template <class Candidate, class BoundaryReady>
  RoundDecision EvaluatePrivateTrial(RoundState& round_state, SlotState& slot_state, Candidate& candidate_trial,
                                     BoundaryReady& boundary_replacement_ready) {
    const auto& started = *context.setup.control_prefix.started;
    auto& result = *context.control.result;
    auto& private_repair = *context.exchange.private_repair;
    CLocalFailure& private_failure = *round_state.private_failure;
    uint64_t& private_work_used = *round_state.private_work_used;
    bool& private_deadline = *round_state.private_deadline;
    std::array<lane_census::Slot, MaximumRoundBatch>& lane_slots = *round_state.lane_slots;
    bool& approved = *slot_state.approved;
    size_t& repair_work = *slot_state.repair_work;
    auto& options = context.options;
    auto& lane_counters = *context.setup.lane_counters;
    auto& slot = slot_state.slot;
    if (options.maximum_seconds &&
        std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds) {
      private_deadline = true;
      private_failure.Set(1, 0, "Native3D private repair round exhausts elapsed-time budget.");
      return RoundDecision::Break;
    }
    bool evaluated = false;
    private_repair = true;
    try {
      evaluated = candidate_trial.Run();
      if (evaluated) evaluated = AdmitPrivateCoverage(slot_state, boundary_replacement_ready);
    } catch (const std::exception& e) {
      result.reason = e.what();
      evaluated = false;
    }
    private_repair = false;
    private_work_used += repair_work;
    if (!evaluated) {
      private_failure.Set(2, 0, result.reason);
      return RoundDecision::Break;
    }
    lane_counters.Approved(true, lane_slots[slot], approved);
    try {
      StoreRepairProposal(round_state, slot_state);
    } catch (const std::exception& e) {
      private_failure.Set(2, 0, e.what());
      return RoundDecision::Break;
    }
    return RoundDecision::Proceed;
  }

  // Accounts for the retained coverage inside the original private-trial catch boundary.
  template <class BoundaryReady>
  bool AdmitPrivateCoverage(SlotState& slot_state, BoundaryReady& boundary_replacement_ready) {
    auto& batch_bytes = *context.control.batch_bytes;
    const size_t& private_scratch = *slot_state.private_scratch;
    Patch& old = *slot_state.old;
    Patch& fresh = *slot_state.fresh;
    bool& approved = *slot_state.approved;
    bool& repair_approved = *slot_state.repair_approved;
    bool& optimization_approved = *slot_state.optimization_approved;
    bool& geometric_progress = *slot_state.geometric_progress;
    unsigned& accepted_ladder = *slot_state.accepted_ladder;
    auto& geometry = context.geometry;
    auto& stats = context.control.result->stats;
    size_t retained_coverage = 0;
    auto admit_coverage = [&]() {
      const auto live =
          transfer_memory::Add(BoundaryCoverageBytes(old.coverage), BoundaryCoverageBytes(fresh.coverage));
      if (live == SIZE_MAX || live < retained_coverage)
        throw std::length_error("Native3D retained repair coverage exceeds storage admission.");
      batch_bytes = transfer_memory::Add(batch_bytes, live - retained_coverage);
      retained_coverage = live;
      return context.exchange.Admit(context, private_scratch);
    };
    bool evaluated = admit_coverage();
    if (evaluated && approved && geometry.Mapped()) {
      geometry.ExportParameters(old);
      evaluated = admit_coverage();
      if (evaluated) {
        geometry.ExportParameters(fresh);
        evaluated = admit_coverage();
      }
    }
    if (evaluated && approved && !boundary_replacement_ready()) {
      EngineTrial::GeometryTrial(context, false);
      approved = repair_approved = optimization_approved = geometric_progress = false;
      if (accepted_ladder < 3) ++stats.quality.ladder[accepted_ladder].rejected[0];
    }
    return evaluated;
  }

  // Moves the live private slot into its original batch record.
  void StoreRepairProposal(RoundState& round_state, SlotState& slot_state) {
    auto& result = *context.control.result;
    auto& maximum_node = *context.commit.maximum_node;
    auto& maximum_cell = *context.commit.maximum_cell;
    Choice& choice = *round_state.choice;
    std::vector<RepairProposal>& repair_proposals = *round_state.repair_proposals;
    Dependencies& dependencies = *slot_state.dependencies;
    Imported& imported = *slot_state.imported;
    bool& chosen = *slot_state.chosen;
    const bool& coordinate_only = *slot_state.coordinate_only;
    Patch& old = *slot_state.old;
    Patch& fresh = *slot_state.fresh;
    Node& point = *slot_state.point;
    bool& approved = *slot_state.approved;
    bool& moved = *slot_state.moved;
    bool& repair_approved = *slot_state.repair_approved;
    bool& cavity_inserted = *slot_state.cavity_inserted;
    bool& optimization_approved = *slot_state.optimization_approved;
    OptimizationOperations& repair_optimization = *slot_state.repair_optimization;
    MetricDiagnosis& certified = *slot_state.certified;
    bool& geometric_progress = *slot_state.geometric_progress;
    const size_t& offset = *slot_state.offset;
    const size_t& node_offset = *slot_state.node_offset;
    bool& capture = *slot_state.capture;
    bool& capture_selected = *slot_state.capture_selected;
    unsigned& accepted_ladder = *slot_state.accepted_ladder;
    std::vector<CellQualityFlag>& old_flags = *slot_state.old_flags;
    std::vector<CellQualityFlag>& fresh_flags = *slot_state.fresh_flags;
    size_t& repair_work = *slot_state.repair_work;
    auto& slot = slot_state.slot;
    auto& proposal = repair_proposals.at(slot);
    proposal.choice = choice;
    proposal.dependencies = std::move(dependencies);
    proposal.imported = std::move(imported);
    proposal.old = std::move(old);
    proposal.fresh = std::move(fresh);
    proposal.point = point;
    proposal.certified = certified;
    proposal.optimization = repair_optimization;
    proposal.chosen = chosen;
    proposal.approved = approved;
    proposal.coordinate_only = coordinate_only;
    proposal.moved = moved;
    proposal.repair_approved = repair_approved;
    proposal.cavity_inserted = cavity_inserted;
    proposal.optimization_approved = optimization_approved;
    proposal.geometric_progress = geometric_progress;
    proposal.capture = capture;
    proposal.capture_selected = capture_selected;
    proposal.first_cell = maximum_cell + 1 + offset;
    proposal.first_node = maximum_node + 1 + node_offset;
    proposal.optimizer_work = repair_work;
    proposal.rejection = approved ? std::string{} : result.reason;
    proposal.ladder = accepted_ladder;
    proposal.old_flags = std::move(old_flags);
    proposal.fresh_flags = std::move(fresh_flags);
  }

  // Preserves the exact collective slot route and observer ordering.
  template <class Candidate, class Reserve, class Observe>
  RoundDecision EvaluateCollectiveSlot(RoundState& round_state, SlotState& slot_state, Candidate& candidate_trial,
                                       Reserve& reserve_dependencies, Observe& observe_transaction) {
    auto& coarsening = *context.control.coarsening;
    auto& repairing = *context.control.repairing;
    auto& result = *context.control.result;
    auto& private_repair = *context.exchange.private_repair;
    const auto& observe_rejections = *context.control.observe_rejections;
    auto& ready = *context.control.ready;
    std::array<lane_census::Slot, MaximumRoundBatch>& lane_slots = *round_state.lane_slots;
    bool& approved = *slot_state.approved;
    auto& world = context.world;
    auto& surface_target = context.surface_target;
    auto& stats = context.control.result->stats;
    auto& lane_counters = *context.setup.lane_counters;
    auto& slot = slot_state.slot;
    if (surface_target && coarsening && !repairing && !observe_rejections) {
      // The complete reserved collapse cavity supplies every retained node. Opaque callbacks keep the collective path.
      const auto elections = world.election_collective_calls, collectives = world.collective_calls,
                 work = stats.global_work;
      CLocalFailure local;
      bool evaluated = false;
      private_repair = true;
      try {
        evaluated = candidate_trial.Run();
      } catch (const std::exception& e) {
        result.reason = e.what();
      }
      if (atlas_budget.within_time && !atlas_budget.within_time()) atlas_budget.time_exhausted = true;
      private_repair = false;
      if (!evaluated)
        local.Set(2, 0, result.reason.empty() ? "Native3D private surface collapse evaluation failed." : result.reason);
      if (world.election_collective_calls != elections || world.collective_calls != collectives ||
          stats.global_work != work)
        local.Set(2, 0, "Native3D private surface collapse performed collective or unreserved operator work.");
      if (!(ready = context.exchange.Vote(context, local))) return Done(Finish());
    } else if (!candidate_trial.Run())
      return Done(Finish());
    lane_counters.Approved(false, lane_slots[slot], approved);
    SlotCommit slot_commit(context, round_state, slot_state, atlas_budget, no_coverage, curved,
                           diagnostic_rejection_limit);
    if (!slot_commit.Run(reserve_dependencies, observe_transaction)) return Done(std::move(*slot_commit.finished));
    return RoundDecision::Proceed;
  }
};

// Admits private boundary parameters before constructing the engine context.
std::optional<EngineResult> SetupBoundaryParameters(EngineSetup& setup, World& world, const Owned& accepted,
                                                    const std::vector<DonorCell>& original,
                                                    const PlanarReference& accepted_geometry, EngineOptions& options,
                                                    SU2NativeGeometry::AtlasBudget& atlas_budget) {
  auto& private_parameters = *setup.private_parameters;
  auto& private_geometry = *setup.private_geometry;
  auto& atlas_geometry = *setup.atlas_geometry;
  auto& parameter_failure = *setup.control_prefix.parameter_failure;
  auto& parameter_bound = *setup.control_prefix.parameter_bound;
  const auto atlas_scratch = *setup.control_prefix.atlas_scratch;
  const unsigned mapped = accepted_geometry.Mapped();
  const auto mapped_low = TimedAllreduce(mapped, CPassiveComm::Op::MIN, world.comm),
             mapped_high = TimedAllreduce(mapped, CPassiveComm::Op::MAX, world.comm);
  if (mapped_low != mapped_high)
    parameter_failure.Set(1, 0, "Native3D parameter geometry mode differs across workers.");
  if (mapped) try {
      const auto* parameters = accepted_geometry.Parameters();
      // Stage clones are boundary-only and admitted once, never per transaction. Private overlays are bounded.
      parameter_bound = transfer_memory::Add(options.caller_bytes, accepted_geometry.ResidentBytes(),
                                             transfer_memory::Mul(parameters->ResidentBytes(), 2), atlas_scratch,
                                             MapBytes(accepted), transfer_memory::Bytes(original));
      if (parameter_bound == SIZE_MAX || parameter_bound > options.memory_ceiling)
        throw std::length_error("Native boundary parameter stage exceeds storage admission.");
      private_parameters = std::make_shared<BoundaryParameterState>(*parameters);
      private_geometry = accepted_geometry.WithParameterState(private_parameters);
      options.caller_bytes = transfer_memory::Add(options.caller_bytes, atlas_scratch);
      if (private_parameters->Mobility().identity || !private_parameters->Coverage().faces.empty() ||
          !private_parameters->Coverage().traces.empty()) {
        atlas_budget.caller_bytes = parameter_bound - atlas_scratch;
        atlas_budget.maximum_bytes = options.memory_ceiling;
        // Half holds the nested planner/reconstruction arithmetic arrays; half holds correspondence records.
        size_t low = 0, high = AtlasScratchBytes * 8;
        while (low < high) {
          const size_t bits = low + (high - low + 1) / 2;
          if (transfer_memory::Mul(2, SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits)) <= AtlasScratchBytes / 2)
            low = bits;
          else
            high = bits - 1;
        }
        if (!low) throw std::length_error("Native3D atlas integer workspace has no admitted capacity.");
        atlas_budget.maximum_integer_bits = low;
        atlas_geometry = private_geometry->WithAtlasReservation(atlas_budget);
      }
    } catch (const std::exception& e) {
      parameter_failure.Set(3, 0, e.what());
    }
  if (mapped_high) {
    const auto failure = world.elect(parameter_failure);
    if (failure.any) {
      EngineResult rejected;
      rejected.reason = failure.message;
      rejected.stats.stop = EngineStop::Failure;
      rejected.stats.maximum_work_bytes = parameter_bound;
      return rejected;
    }
  }
  return std::nullopt;
}
