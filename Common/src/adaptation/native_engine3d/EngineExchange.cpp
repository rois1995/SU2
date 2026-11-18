// Owns only empty optional slots; local Adapt scopes govern payload lifetimes.
#include "EngineExchange.hpp"
#include "EngineContext.hpp"

EngineExchange::EngineExchange() = default;
EngineExchange::~EngineExchange() = default;

void EngineExchange::RetireDirectory(const DirectoryStats& old) {
  auto& retired_directory = *this->retired_directory;

  retired_directory.collision_seconds += old.collision_seconds;
  retired_directory.collision_builds += old.collision_builds;
  retired_directory.collision_candidates += old.collision_candidates;
  retired_directory.collision_checks += old.collision_checks;
  retired_directory.elections += old.elections;
  retired_directory.election_seconds += old.election_seconds;
  retired_directory.parameter_agreement_seconds += old.parameter_agreement_seconds;
  retired_directory.parameter_publication_seconds += old.parameter_publication_seconds;
  retired_directory.parameter_edits += old.parameter_edits;
  retired_directory.parameter_faces += old.parameter_faces;
  retired_directory.parameter_payload_bytes += old.parameter_payload_bytes;
  retired_directory.change_scanned += old.change_scanned;
  retired_directory.change_sent += old.change_sent;
  retired_directory.change_received += old.change_received;
  retired_directory.change_seconds += old.change_seconds;
  retired_directory.maximum_work_bytes = std::max(retired_directory.maximum_work_bytes, old.maximum_work_bytes);
}

CElectedFailure EngineExchange::ElectTrialFailure(EngineContext& context, const CLocalFailure& local) {
  auto& private_repair = *context.exchange.private_repair;
  auto& world = context.world;
  if (private_repair) return CElectedFailure{local.Failed(), world.rank, local.severity, local.gid, local.message};
  return world.elect(local);
}

bool EngineExchange::Vote(EngineContext& context, const CLocalFailure& local) {
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& private_repair = *context.exchange.private_repair;
  auto& atlas_budget =
      options.shared_geometry_budget ? *options.shared_geometry_budget : *context.setup.local_atlas_budget;
  const auto& atlas_work_initial = *context.setup.control_prefix.atlas_work_initial;
  if (private_repair && atlas_budget.within_time && !atlas_budget.within_time()) atlas_budget.time_exhausted = true;
  stats.atlas_work = atlas_budget.work - atlas_work_initial;
  stats.atlas_peak_bytes = atlas_budget.peak_bytes;
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, atlas_budget.peak_bytes);
  // Elect sticky geometry failures at the existing stage votes, even if a trial caught the exception.
  constexpr unsigned short atlas_work_refusal = 4;
  auto pending = local;
  if (atlas_budget.work_exhausted)
    pending.Set(atlas_work_refusal, 0, "Native3D shared atlas work budget exhausted; feasibility is undetermined.");
  constexpr unsigned short geometry_backend_failure = 5;
  if (!atlas_budget.geometry_failure.empty())
    pending.Set(geometry_backend_failure, 0, "Native3D geometry backend failure: " + atlas_budget.geometry_failure);
  constexpr unsigned short atlas_time_refusal = 6;
  if (atlas_budget.time_exhausted)
    pending.Set(atlas_time_refusal, 0,
                "Native3D geometry/private work exhausts elapsed-time budget; feasibility is undetermined.");
  const auto elected = ElectTrialFailure(context, pending);
  if (elected.any) result.reason = elected.message;
  if (elected.any && (elected.severity == atlas_work_refusal || elected.severity == geometry_backend_failure ||
                      elected.severity == atlas_time_refusal)) {
    // A quota refusal need not be the only failure; retain the ordinary elected diagnostic too.
    const auto original = ElectTrialFailure(context, local);
    if (original.any) result.reason += " Original failure: " + original.message;
    result.valid = result.surface_prepared = false;
    stats.stop = elected.severity == atlas_time_refusal   ? EngineStop::TimeBudget
                 : elected.severity == atlas_work_refusal ? EngineStop::WorkBudget
                                                          : EngineStop::Failure;
  }
  return !elected.any;
}

size_t EngineExchange::Bytes(EngineContext& context) {
  auto& options = context.options;
  auto& accepted_bytes = *context.control.accepted_bytes;
  auto& batch_bytes = *context.control.batch_bytes;
  const auto& diagnostic_bytes = *context.control.diagnostic_bytes;
  auto& checkpoint = *context.control.checkpoint;
  auto& result = *context.control.result;
  auto& working = *context.ownership.working;
  auto& batch_changed = *context.commit.batch_changed;
  auto& batch_touched = *context.commit.batch_touched;
  auto& batch_supports = *context.commit.batch_supports;
  const auto& original = context.original;
  auto& initial = *context.census.initial;
  auto& eligible_extensions = *context.census.eligible_extensions;
  auto& selection = *context.census.selection;
  auto& retired_edges = *context.census.retired_edges;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& field_identity = *context.census.field_identity;
  auto& inactive_blocked_edges = *context.queues.inactive_blocked_edges;
  auto& inactive_blocked_links = *context.queues.inactive_blocked_links;
  return transfer_memory::Add(
      options.caller_bytes, MapBytes(options.wall_heights), MapBytes(options.region_walls), accepted_bytes, batch_bytes,
      diagnostic_bytes, MapBytes(working), checkpoint.Bytes(), transfer_memory::Bytes(batch_changed),
      MapBytes(batch_touched), MapBytes(batch_supports), transfer_memory::Bytes(original),
      transfer_memory::Bytes(initial), transfer_memory::Bytes(eligible_extensions),
      transfer_memory::Bytes(result.splits), transfer_memory::Bytes(result.collapses), selection.Bytes(),
      MapBytes(retired_edges), MapBytes(campaign_edges), MapBytes(inactive_blocked_edges),
      transfer_memory::Mul(inactive_blocked_links, sizeof(Id) + 4 * sizeof(void*)), field_identity ? 64 : 0);
}

size_t EngineExchange::RealBytes(EngineContext& context) {
  const auto live = Bytes(context);
  auto& atlas_geometry = *context.setup.atlas_geometry;
  const auto& atlas_scratch = *context.setup.control_prefix.atlas_scratch;
  return atlas_geometry && live != SIZE_MAX ? live - atlas_scratch : live;
}

bool EngineExchange::Admit(EngineContext& context, size_t extra) {
  auto& options = context.options;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& stats = context.control.result->stats;
  auto& atlas_budget =
      options.shared_geometry_budget ? *options.shared_geometry_budget : *context.setup.local_atlas_budget;
  auto& atlas_geometry = *context.setup.atlas_geometry;
  const auto& atlas_scratch = *context.setup.control_prefix.atlas_scratch;
  const auto peak = transfer_memory::Add(Bytes(context), source.ResidentBytes(), directory->ResidentBytes(), extra);
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  CLocalFailure failure;
  if (peak == SIZE_MAX || peak > options.memory_ceiling)
    failure.Set(3, 0, "Native 3D adaptation working storage exceeds budget.");
  const bool admitted = Vote(context, failure);
  // Integer workspace stays fixed; geometric query arrays use the remaining configured memory pool.
  if (admitted && atlas_geometry) {
    atlas_budget.caller_bytes = peak - atlas_scratch;
    atlas_budget.maximum_bytes = options.memory_ceiling;
  }
  return admitted;
}

DonorPatch EngineExchange::ImportSource(EngineContext& context, const std::vector<Cell>& cells, size_t extra,
                                        bool layer, const std::vector<unsigned char>* requests) {
  auto& stats = context.control.result->stats;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& options = context.options;
  if (layer)
    ++stats.layer_donor_batches;
  else
    ++stats.donor_batches;
  return source.ImportCells(cells, options.memory_ceiling,
                            transfer_memory::Add(Bytes(context), directory->ResidentBytes(), extra));
}

// Holds the single original Topology scope across lookup, support expansion and frontier receipts.
bool RoundAcquisition::AcquireBatchDependencies(CLocalFailure& failure) {
  auto& stats = context.control.result->stats;
  Timer timer(stats.dependency_seconds, context.options.timings, SU2NativeTiming3D::Operation::Topology);
  std::vector<Feature> features;
  size_t retained_cells;
  return FetchBatch(failure, features, retained_cells) && ExpandRepairSupport(failure, retained_cells) &&
         ServiceFetchRefusals();
}

// Charges retained batch memory before any candidate acquisition.
bool RoundAcquisition::AdmitBatchStorage() {
  auto& batch_bytes = *context.control.batch_bytes;
  // All retained metadata/payloads remain byte-admitted while each disjoint candidate uses the existing transaction.
  batch_bytes = transfer_memory::Add(
      transfer_memory::Mul(round_state.batch, sizeof(Choice) + sizeof(Feature) + sizeof(Dependencies) +
                                                        sizeof(Imported) + 2 * sizeof(bool) + 2 * sizeof(size_t)),
      transfer_memory::Mul(transfer_memory::Mul(round_state.batch, MaximumCavityCells),
                           sizeof(Dependency) + sizeof(OwnedCell)),
      transfer_memory::Mul(transfer_memory::Mul(8, round_state.batch * MaximumCavityCells),
                           sizeof(Id) + 4 * sizeof(void*)));
  if (!context.exchange.Admit(context, 0)) return FinishRound();
  return true;
}

// Charges initial proposals including planned refusals.
bool RoundAcquisition::AdmitBatchWork() {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& choices = *round_state.choices;
  auto& refused_candidates = *round_state.refused_candidates;
  Timer timer(stats.selection_seconds, options.timings, SU2NativeTiming3D::Operation::Scheduling);
  // Planned refusals are proposals too: they consume the same global work as serial oversized stars.
  const uint64_t proposed =
      TimedAllreduce(uint64_t(choices.size() + refused_candidates), CPassiveComm::Op::SUM, world.comm);
  if (proposed > stats.work_limit - stats.global_work) {
    stats.stop = EngineStop::WorkBudget;
    result.reason = "Native 3D global proposal budget exhausted before admission; feasibility is undetermined.";
    return FinishRound();
  }
  stats.global_work += proposed;
  return true;
}

// Looks up and fetches complete candidate dependencies under their existing byte admission.
bool RoundAcquisition::FetchBatch(CLocalFailure& failure, std::vector<Feature>& features, size_t& retained_cells) {
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& ready = *context.control.ready;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& batch_bytes = *context.control.batch_bytes;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& choices = *round_state.choices;
  auto& dependency_batch = *round_state.dependency_batch;
  auto& imported_batch = *round_state.imported_batch;
  auto& planned_coarsening = *round_state.planned_coarsening;
  auto& refused_slots = *round_state.refused_slots;
  try {
    for (const auto& candidate : choices)
      features.push_back(repairing    ? candidate.feature
                         : coarsening ? Feature{{candidate.edge[1], 0, 0}, 1}
                                      : Feature{{candidate.edge[0], candidate.edge[1], 0}, 2});
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!(ready = context.exchange.Vote(context, failure))) return FinishRound();
  if (!planned_coarsening)
    dependency_batch = directory->LookupBatch(
        features, options.memory_ceiling, transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()),
        repairing);
  if (!dependency_batch.valid) {
    result.reason = dependency_batch.reason;
    return FinishRound();
  }
  // A remote complete coordinate star may exceed the ordinary cavity bound. Retained
  // imports and footprint storage stay admitted across every subsequent private slot.
  retained_cells = 0;
  for (const auto& candidate : dependency_batch.candidates)
    retained_cells = transfer_memory::Add(retained_cells, candidate.cells.size());
  const auto ordinary_cells = transfer_memory::Mul(round_state.batch, MaximumCavityCells);
  if (retained_cells > ordinary_cells)
    batch_bytes = transfer_memory::Add(
        batch_bytes, transfer_memory::Mul(retained_cells - ordinary_cells, sizeof(Dependency) + sizeof(OwnedCell) +
                                                                               8 * (sizeof(Id) + 4 * sizeof(void*))));
  if (!context.exchange.Admit(context, 0)) return FinishRound();
  imported_batch = directory->FetchBatch(dependency_batch, working, options.memory_ceiling,
                                         transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()));
  if (!imported_batch.valid) {
    result.reason = imported_batch.reason;
    return FinishRound();
  }
  try {
    for (size_t i = 0; i < imported_batch.candidates.size(); ++i) {
      const auto& candidate = imported_batch.candidates[i];
      if (candidate.valid) continue;
      const auto& star = dependency_batch.candidates.at(i);
      if (!star.valid && star.oversized) {
        Refuse(choices.at(i), features.at(i), star.reason);
        ++refused_slots;
      } else
        failure.Set(1, 0, candidate.reason);
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!(ready = context.exchange.Vote(context, failure))) return FinishRound();
  return true;
}

// Fetches the full vertex stars of curved repair candidates before reservation.
bool RoundAcquisition::ExpandRepairSupport(CLocalFailure& failure, size_t retained_cells) {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& repairing = *context.control.repairing;
  auto& batch_bytes = *context.control.batch_bytes;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& choices = *round_state.choices;
  auto& dependency_batch = *round_state.dependency_batch;
  auto& imported_batch = *round_state.imported_batch;
  if (repairing && curved) {
    const auto support_bytes =
        transfer_memory::Mul(transfer_memory::Mul(4, choices.size()),
                             transfer_memory::Mul(MaximumCavityCells, sizeof(Dependency) + sizeof(OwnedCell) +
                                                                          8 * (sizeof(Id) + 4 * sizeof(void*))));
    batch_bytes = transfer_memory::Add(batch_bytes, support_bytes,
                                       transfer_memory::Mul(retained_cells, sizeof(OwnedCell) + sizeof(Dependency)));
    if (!context.exchange.Admit(context, 0)) return FinishRound();
    std::vector<Feature> supports;
    std::vector<std::pair<size_t, size_t>> groups;
    try {
      SelectRepairSupport(supports, groups);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return FinishRound();
    if (world.maximum(!supports.empty())) {
      // Complete moved stars are fetched before the existing full-vertex-disjoint reservation.
      const auto extra =
          directory->LookupBatch(supports, options.memory_ceiling,
                                 transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()));
      if (!extra.valid) {
        result.reason = extra.reason;
        return FinishRound();
      }
      bool expanded = false;
      try {
        MergeRepairSupport(extra, groups, expanded);
      } catch (const std::exception& e) {
        failure.Set(2, 0, e.what());
      }
      if (!context.exchange.Vote(context, failure)) return FinishRound();
      if (world.maximum(expanded)) {
        auto complete =
            directory->FetchBatch(dependency_batch, working, options.memory_ceiling,
                                  transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()));
        if (!complete.valid) {
          result.reason = complete.reason;
          return FinishRound();
        }
        imported_batch = std::move(complete);
      }
    }
  }
  return true;
}

// Requests the four vertex stars of the selected weak core cell of each cavity.
void RoundAcquisition::SelectRepairSupport(std::vector<Feature>& supports,
                                           std::vector<std::pair<size_t, size_t>>& groups) {
  auto& selection = *context.census.selection;
  auto& imported_batch = *round_state.imported_batch;
  for (size_t i = 0; i < imported_batch.candidates.size(); ++i) {
    const auto& candidate = imported_batch.candidates[i];
    if (!candidate.valid || candidate.cells.size() > MaximumCavityCells) continue;
    const OwnedCell* core = nullptr;
    long double worst = -1;
    for (const auto& row : candidate.cells) {
      const auto cached = selection.cells.find(row.cell.id);
      if (cached == selection.cells.end() || !cached->second.geometry_invalid) continue;
      const auto physical =
          std::count_if(row.physical.begin(), row.physical.end(), [](const auto& f) { return f.present; });
      if (selection.geometry_repairs
              ? physical == 0
              : (!cached->second.shape_measured || cached->second.quality_valid || physical != 2))
        continue;
      const auto weight = selection.RepairWeight(cached->second);
      if (weight > worst || (weight == worst && core && row.cell.id < core->cell.id)) {
        worst = weight;
        core = &row;
      }
    }
    if (!core) continue;
    groups.push_back({i, supports.size()});
    for (const auto& node : core->cell.v) supports.push_back({{node.id, 0, 0}, 1});
  }
}

// Merges version-consistent support in canonical cell order without exceeding the original cavity bound.
void RoundAcquisition::MergeRepairSupport(const DependencyBatch& extra,
                                          const std::vector<std::pair<size_t, size_t>>& groups, bool& expanded) {
  auto& dependency_batch = *round_state.dependency_batch;
  for (const auto& group : groups) {
    auto& original = dependency_batch.candidates.at(group.first);
    std::map<Id, Dependency> merged;
    for (const auto& row : original.cells) merged.emplace(row.cell, row);
    bool complete = true;
    for (size_t k = 0; k < 4; ++k) {
      const auto& star = extra.candidates.at(group.second + k);
      if (!star.valid) {
        if (!star.oversized) throw std::logic_error("Coupled boundary support lost a current vertex star.");
        complete = false;
        continue;
      }
      for (const auto& row : star.cells) {
        const auto prior = merged.emplace(row.cell, row);
        if (!prior.second && (prior.first->second.owner != row.owner || prior.first->second.version != row.version))
          throw std::logic_error("Coupled boundary support changed dependency ownership/version.");
      }
    }
    if (!complete || merged.size() > MaximumCavityCells || merged.size() == original.cells.size()) continue;
    original.cells.clear();
    for (const auto& row : merged) original.cells.push_back(row.second);
    original.coordinate_star = {};
    expanded = true;
  }
}

// Services fetch refusal receipts before leaving the dependency timer.
bool RoundAcquisition::ServiceFetchRefusals() {
  auto& world = context.world;
  auto& refused_receipts = *round_state.refused_receipts;
  auto& refused_receipt_count = *round_state.refused_receipt_count;
  if (world.maximum(refused_receipt_count != 0)) {
    if (!context.queues.ServiceFrontier(context, {}, false, false, &refused_receipts, refused_receipt_count))
      return FinishRound();
    refused_receipt_count = 0;
  }
  return true;
}

// Reserves disjoint candidates and services construction deferrals in the existing communication scope.
bool RoundAcquisition::ReserveBatch(CLocalFailure& failure) {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& repairing = *context.control.repairing;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& choices = *round_state.choices;
  auto& imported_batch = *round_state.imported_batch;
  auto& reserved = *round_state.reserved;
  auto& refused_candidates = *round_state.refused_candidates;
  auto& refused_slots = *round_state.refused_slots;
  auto& refused_receipts = *round_state.refused_receipts;
  auto& refused_receipt_count = *round_state.refused_receipt_count;
  auto& pass_deferred_candidates = *round_state.pass_deferred_candidates;
  Timer timer(stats.reservation_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
  std::vector<unsigned> priorities;
  try {
    if (repairing)
      for (const auto& candidate : choices) priorities.push_back(candidate.reservation_priority);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return FinishRound();
  reserved = directory->ReserveBatch(imported_batch, options.memory_ceiling,
                                     transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()),
                                     priorities);
  if (!reserved.valid) {
    result.reason = reserved.reason;
    return FinishRound();
  }
  size_t winners = 0;
  for (bool chosen : reserved.chosen) winners += chosen;
  for (size_t i = 0; i < reserved.pass_deferred.size(); ++i)
    if (reserved.pass_deferred[i]) {
      if (refused_receipt_count == MaximumRoundBatch)
        throw std::logic_error("Native construction deferrals exceed frontier bound.");
      const auto edge = choices.at(i).edge;
      refused_receipts[refused_receipt_count++] = {Feature{{edge[0], edge[1], 0}, 2}, false};
      ++pass_deferred_candidates;
      ++stats.length_pass_deferred;
    }
  if (world.maximum(refused_receipt_count != 0)) {
    if (!context.queues.ServiceFrontier(context, {}, false, false, &refused_receipts, refused_receipt_count))
      return FinishRound();
    refused_receipt_count = 0;
  }
  stats.batch_reserved += winners;
  stats.conflicts += choices.size() - winners - refused_slots;
  if (!world.maximum(winners != 0 || refused_candidates != 0 || pass_deferred_candidates != 0)) {
    result.reason = "Native 3D batch reservation made no progress.";
    return FinishRound();
  }
  return true;
}

// Only Proceed opens the trial and its cavity scope.
enum class SlotDecision { Next, Proceed, Finish };

// Borrows the current choice and slot outputs without owning their lifetimes.
class SlotAcquisition {
 public:
  SlotAcquisition(EngineContext&, RoundState&, SlotState&, SU2NativeGeometry::AtlasBudget&, bool curved);
  template <class Reserve>
  SlotDecision Run(RoundAcquisition&, wall_diagnostics::Choice&, Reserve& reserve_dependencies);
  std::optional<EngineResult> finished;

 private:
  EngineContext& context;
  RoundState& round_state;
  SlotState& slot_state;
  SU2NativeGeometry::AtlasBudget& atlas_budget;
  bool curved;

  SlotDecision FinishSlot();
  SlotDecision UnpackBatch(wall_diagnostics::Choice&);
  SlotDecision FetchSingle(RoundAcquisition&, wall_diagnostics::Choice&);
  SlotDecision CorrectCollapse();
  template <class Reserve>
  SlotDecision ReserveSingle(wall_diagnostics::Choice&, Reserve& reserve_dependencies);
};

// Acquires into the active caller-owned slots without moving the choice observer.
SlotAcquisition::SlotAcquisition(EngineContext& context, RoundState& round_state, SlotState& slot_state,
                                 SU2NativeGeometry::AtlasBudget& atlas_budget, bool curved)
    : context(context), round_state(round_state), slot_state(slot_state), atlas_budget(atlas_budget), curved(curved) {}

// Evaluates terminal reporting before any timer or failure object in the originating operation unwinds.
SlotDecision SlotAcquisition::FinishSlot() {
  auto& working = *context.ownership.working;
  const auto& started = *context.setup.control_prefix.started;
  const auto& started_cpu = *context.setup.control_prefix.started_cpu;
  finished.emplace(EngineControl::Finish(context, curved, working, atlas_budget, started, started_cpu));
  return SlotDecision::Finish;
}

// Unpacks one pre-reserved candidate while preserving the private or collective conflict check.
SlotDecision SlotAcquisition::UnpackBatch(wall_diagnostics::Choice& wall_choice) {
  auto& world = context.world;
  auto& repairing = *context.control.repairing;
  auto& choice = *round_state.choice;
  auto& choices = *round_state.choices;
  auto& dependency_batch = *round_state.dependency_batch;
  auto& imported_batch = *round_state.imported_batch;
  auto& reserved = *round_state.reserved;
  auto& chosen = *slot_state.chosen;
  auto& dependencies = *slot_state.dependencies;
  auto& imported = *slot_state.imported;
  auto& slot = slot_state.slot;
  choice = slot < choices.size() ? choices[slot] : Choice{};
  wall_choice.Begin(choice.active);
  if (choice.active) {
    dependencies = std::move(dependency_batch.candidates[slot]);
    imported = std::move(imported_batch.candidates[slot]);
    chosen = reserved.chosen[slot];
  }
  if (repairing ? !chosen : !world.maximum(chosen)) {
    wall_choice.Disposition(wall_diagnostics::PreTrialDisposition::Conflict);
    return SlotDecision::Next;
  }
  return SlotDecision::Proceed;
}

// Performs the original complete-star lookup and fetch under one Topology timer.
SlotDecision SlotAcquisition::FetchSingle(RoundAcquisition& round_acquisition, wall_diagnostics::Choice& wall_choice) {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& choice = *round_state.choice;
  auto& dependency_batch = *round_state.dependency_batch;
  auto& planned_coarsening = *round_state.planned_coarsening;
  auto& dependencies = *slot_state.dependencies;
  auto& imported = *slot_state.imported;
  {
    Timer timer(stats.dependency_seconds, options.timings, SU2NativeTiming3D::Operation::Topology);
    const Feature feature =
        repairing ? choice.feature
                  : (coarsening ? Feature{{choice.edge[1], 0, 0}, 1} : Feature{{choice.edge[0], choice.edge[1], 0}, 2});
    const auto features = choice.active ? std::vector<Feature>{feature} : std::vector<Feature>{};
    if (planned_coarsening) {
      // Every proposal of this round may have been refused for an oversized star during planning.
      if (!world.maximum(choice.active)) return SlotDecision::Next;
      if (choice.active)
        dependencies = std::move(dependency_batch.candidates.at(0));
      else
        dependencies.valid = true;
    } else {
      auto batch = directory->LookupBatch(features, options.memory_ceiling,
                                          transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()),
                                          repairing);
      if (!batch.valid) {
        result.reason = batch.reason;
        return FinishSlot();
      }
      if (choice.active)
        dependencies = std::move(batch.candidates.at(0));
      else
        dependencies.valid = true;
      const bool oversized = choice.active && !dependencies.valid && dependencies.oversized;
      CLocalFailure lookup_failure;
      if (choice.active && !dependencies.valid && !oversized) lookup_failure.Set(1, 0, dependencies.reason);
      if (!context.exchange.Vote(context, lookup_failure)) return FinishSlot();
      if (oversized) {
        // Refuse this proposal; publication still requires its complete star. Changed stars reactivate it.
        wall_choice.Disposition(wall_diagnostics::PreTrialDisposition::Oversized);
        try {
          round_acquisition.Refuse(choice, feature, dependencies.reason);
        } catch (const std::exception& e) {
          lookup_failure.Set(2, 0, e.what());
        }
        choice.active = false;
        dependencies = Dependencies{};
        dependencies.valid = true;
      }
      if (!context.exchange.Vote(context, lookup_failure)) return FinishSlot();
      if (world.maximum(oversized) &&
          !context.queues.ServiceFrontier(
              context, repairing ? feature : Feature{{choice.edge[0], choice.edge[1], 0}, 2}, oversized, false))
        return FinishSlot();
      // Concurrent peers keep their own proposals; a slot whose every proposal was refused is skipped.
      if (!world.maximum(choice.active)) return SlotDecision::Next;
    }
    if (dependencies.valid)
      imported = directory->Fetch(dependencies, working, options.memory_ceiling,
                                  transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()));
  }
  return SlotDecision::Proceed;
}

// Runs the bounded surface collapse correction at its original Candidate boundary.
SlotDecision SlotAcquisition::CorrectCollapse() {
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& choice = *round_state.choice;
  auto& collapse_correction_refusal = *slot_state.collapse_correction_refusal;
  auto& collapse_corrected = *slot_state.collapse_corrected;
  auto& collapse_apex = *slot_state.collapse_apex;
  auto& dependencies = *slot_state.dependencies;
  auto& imported = *slot_state.imported;
  if (surface_target && curved && coarsening && !repairing) {
    Timer timer(stats.collapse_correction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
    const size_t seed_cells = imported.cells.size();
    auto correction = directory->CorrectCollapse(
        dependencies, imported, working, choice.edge[1], choice.edge[0], choice.active, geometry,
        stats.work_limit - stats.global_work, atlas_budget.within_time, options.memory_ceiling,
        transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()));
    stats.global_work += correction.work;
    stats.collapse_correction_work += correction.work;
    if (!correction.valid) {
      result.reason = correction.reason;
      return FinishSlot();
    }
    collapse_corrected = correction.corrected;
    collapse_apex = correction.apex;
    if (!collapse_corrected) collapse_correction_refusal = std::move(correction.reason);
    if (collapse_corrected) {
      ++stats.collapse_corrections;
      stats.collapse_correction_cells += correction.imported.cells.size() - seed_cells;
    }
    dependencies = std::move(correction.dependencies);
    imported = std::move(correction.imported);
  }
  return SlotDecision::Proceed;
}

// Reserves the fetched support and serves pass deferrals while the Communication timer is live.
template <class Reserve>
SlotDecision SlotAcquisition::ReserveSingle(wall_diagnostics::Choice& wall_choice, Reserve& reserve_dependencies) {
  auto& world = context.world;
  auto& options = context.options;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& source = *context.exchange.source;
  auto& ready = *context.control.ready;
  auto& timed_lane = *round_state.timed_lane;
  auto& choice = *round_state.choice;
  auto& chosen = *slot_state.chosen;
  auto& pass_deferred = *slot_state.pass_deferred;
  auto& imported = *slot_state.imported;
  Timer timer(stats.reservation_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
  chosen = reserve_dependencies(imported, choice.active, result.reason, options.memory_ceiling,
                                transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()),
                                choice.reservation_priority);
  stats.conflicts += choice.active && !chosen;
  if (choice.active && !chosen) {
    wall_choice.Disposition(pass_deferred ? wall_diagnostics::PreTrialDisposition::Deferred
                                          : wall_diagnostics::PreTrialDisposition::Conflict);
    selection.Note(choice.edge, pass_deferred ? 2 : 1);
    if (timed_lane == 1)
      ++stats.surface_conflicts;
    else if (timed_lane == 2)
      ++stats.interior_conflicts;
  }
  const int admitted = world.maximum(chosen ? 2 : pass_deferred ? 1 : 0);
  if (!admitted)
    ready = false;
  else if (admitted == 1) {
    if (!context.queues.ServiceFrontier(context, Feature{{choice.edge[0], choice.edge[1], 0}, 2},
                                        choice.active && pass_deferred, false, nullptr, 0, 2))
      return FinishSlot();
    stats.length_pass_deferred += pass_deferred;
    return SlotDecision::Next;
  }
  return SlotDecision::Proceed;
}

// Selects the existing batched path or the ordered single lookup, correction and reservation path.
template <class Reserve>
SlotDecision SlotAcquisition::Run(RoundAcquisition& round_acquisition, wall_diagnostics::Choice& wall_choice,
                                  Reserve& reserve_dependencies) {
  auto& result = *context.control.result;
  auto& ready = *context.control.ready;
  auto& batched = *round_state.batched;
  auto& dependencies = *slot_state.dependencies;
  auto& imported = *slot_state.imported;
  if (batched) return UnpackBatch(wall_choice);
  if (const auto decision = FetchSingle(round_acquisition, wall_choice); decision != SlotDecision::Proceed)
    return decision;
  if (!dependencies.valid || !imported.valid) {
    result.reason = dependencies.valid ? imported.reason : dependencies.reason;
    return FinishSlot();
  }
  if (const auto decision = CorrectCollapse(); decision != SlotDecision::Proceed) return decision;
  if (const auto decision = ReserveSingle(wall_choice, reserve_dependencies); decision != SlotDecision::Proceed)
    return decision;
  if (!ready) return FinishSlot();
  return SlotDecision::Proceed;
}

// Private trials already hold the value locally; collective trials preserve the World counter path.
int TrialComm::Maximum(int value) { return private_repair ? value : world.maximum(value); }

// Preserves the direct timed collective, including its separate accounting.
template <class T>
T TrialComm::Reduce(T value, CPassiveComm::Op op) {
  return private_repair ? value : TimedAllreduce(value, op, world.comm);
}

// Private trials read the current imported cavity rather than requesting donor cells.
DonorPatch TrialComm::ImportCells(const std::vector<Cell>& cells, size_t ceiling, size_t caller) {
  return private_repair ? source.ImportCellsLocal(cells, imported.cells, ceiling, caller)
                        : source.ImportCells(cells, ceiling, caller);
}

// Uses the same sampler and request mask in both trial modes.
DonorPatch TrialComm::ImportSamples(const std::vector<Cell>& cells, const std::vector<Id>& extensions, size_t ceiling,
                                    size_t caller, const std::vector<unsigned char>& requests, bool shape) {
  return private_repair
             ? source.ImportMetricSamplesLocal(cells, imported.cells, extensions, ceiling, caller, requests, shape)
             : source.ImportMetricSamples(cells, extensions, ceiling, caller, requests, shape);
}

// Uses cavity-local donors only for private point samples.
DonorSamples TrialComm::SamplePoints(const std::vector<Point>& points, const std::vector<Id>& extensions,
                                     size_t ceiling, size_t caller) {
  return private_repair ? source.SamplePointsLocal(points, imported.cells, extensions, ceiling, caller)
                        : source.SamplePoints(points, extensions, ceiling, caller);
}

// Uses the same locality choice for segment midpoint samples.
DonorSamples TrialComm::SampleMidpoints(const std::vector<std::array<Point, 2>>& points,
                                        const std::vector<Id>& extensions, size_t ceiling, size_t caller) {
  return private_repair ? source.SampleMidpointsLocal(points, imported.cells, extensions, ceiling, caller)
                        : source.SampleMidpoints(points, extensions, ceiling, caller);
}
