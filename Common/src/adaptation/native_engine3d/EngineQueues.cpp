// Runs the original coalescing/frontier transactions; owns no additional queue payloads.
#include "EngineQueues.hpp"
#include "EngineContext.hpp"

EngineQueues::EngineQueues() = default;
EngineQueues::~EngineQueues() = default;

void EngineQueues::LengthDirection(EngineContext& context, bool collapse) {
  auto& selection = *context.census.selection;
  auto& inactive_blocked_edges = *this->inactive_blocked_edges;
  auto& inactive_blocked_links = *this->inactive_blocked_links;
  auto& coarsening = *context.control.coarsening;
  auto& construction_kind = *context.control.construction_kind;
  auto& length_blocks_dirty = *this->length_blocks_dirty;

  if (coarsening != collapse) {
    selection.blocked_edges.swap(inactive_blocked_edges);
    std::swap(selection.blocked_links, inactive_blocked_links);
    coarsening = selection.coarsening = collapse;
    construction_kind = collapse ? MetricChange::Coarsen : MetricChange::Refine;
  }
  if (length_blocks_dirty[collapse]) {
    selection.blocked_edges.clear();
    selection.blocked_links = 0;
    length_blocks_dirty[collapse] = false;
  }
}

template <class FeatureFn>
bool CoalesceOrder(EngineContext& context, std::vector<Choice>& choices, std::vector<ProposalKey>& all,
                   bool concurrent_refine, CLocalFailure& failure, FeatureFn& feature) {
  auto& world = context.world;
  auto& stats = context.control.result->stats;
  auto& repairing = *context.control.repairing;
  try {
    if (all.size() > std::numeric_limits<unsigned>::max())
      throw std::logic_error("Native edge proposal ordering exceeds its bound.");
    for (const auto& key : all)
      if (key.rank < 0 || key.rank >= world.size || key.slot >= MaximumCandidateBatch || key.feature.count < 1 ||
          key.feature.count > 3 || !std::isfinite(key.score))
        throw std::logic_error("Invalid native edge proposal identity/priority.");
    for (const auto& key : all)
      for (unsigned i = 0; i < key.feature.count; ++i)
        for (unsigned j = i + 1; j < key.feature.count; ++j)
          if (key.feature.vertices[i] == key.feature.vertices[j])
            throw std::logic_error("Repeated native proposal feature vertex.");
    // Demand priority precedes ownership; this score orders search, never certifies a mesh.
    std::sort(all.begin(), all.end(), [&](const auto& a, const auto& b) {
      if (repairing) {
        if (a.fair_repair != b.fair_repair) return a.fair_repair;
        if (a.fair_repair && a.epoch != b.epoch) return a.epoch < b.epoch;
        if (a.score != b.score) return a.score < b.score;
        if (a.epoch != b.epoch) return a.epoch < b.epoch;
      }
      return std::make_tuple(!a.geometric, a.score, a.cell, a.feature.count, a.feature.vertices, a.rank, a.slot) <
             std::make_tuple(!b.geometric, b.score, b.cell, b.feature.count, b.feature.vertices, b.rank, b.slot);
    });
    std::vector<Choice> unique;
    for (size_t i = 0; i < choices.size(); ++i) {
      const auto owner = std::find_if(all.begin(), all.end(), [&](const auto& key) {
        const auto f = feature(choices[i]);
        return key.feature.count == f.count && key.feature.vertices == f.vertices;
      });
      if (owner == all.end()) throw std::logic_error("Missing native edge proposal.");
      if (owner->rank != world.rank || owner->slot != i)
        ++stats.proposal_duplicates;
      else {
        if (repairing && !concurrent_refine && owner != all.begin()) continue;
        if (concurrent_refine || repairing) choices[i].reservation_priority = unsigned(owner - all.begin());
        if (repairing) choices[i].repair_epoch = owner->epoch;
        unique.push_back(choices[i]);
      }
    }
    choices.swap(unique);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  return context.exchange.Vote(context, failure);
}

bool EngineQueues::Coalesce(EngineContext& context, std::vector<Choice>& choices, bool curved) {
  auto& world = context.world;
  auto& options = context.options;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& result = *context.control.result;
  auto& selection = *context.census.selection;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& composition = context.composition;
  auto& bidirectional = context.bidirectional;

  const bool collapse = coarsening && !curved && !composition;
  const bool concurrent_refine = (repairing || (!coarsening && (curved || composition || selection.split_lanes)) ||
                                  (bidirectional && coarsening)) &&
                                 options.candidate_batch > 1;
  if (world.size == 1 || (!repairing && !collapse && !concurrent_refine)) return true;
  const auto feature = [&](const Choice& c) { return repairing ? c.feature : Feature{{c.edge[0], c.edge[1], 0}, 2}; };
  // Replicated vertex/edge/face demands are one proposal; directed collapses retain distinct endpoints.
  const auto scratch = transfer_memory::Mul(transfer_memory::Mul(world.size, MaximumCandidateBatch),
                                            8 * sizeof(ProposalKey) + 8 * sizeof(void*));
  if (!context.exchange.Admit(context, scratch)) return false;
  CLocalFailure failure;
  std::vector<std::vector<ProposalKey>> to;
  try {
    if (choices.size() > MaximumCandidateBatch) throw std::logic_error("Native edge proposal table exceeds its bound.");
    std::vector<ProposalKey> local;
    for (size_t i = 0; i < choices.size(); ++i)
      local.push_back({feature(choices[i]), world.rank, i, choices[i].geometric, double(choices[i].priority_score),
                       choices[i].priority_cell, choices[i].repair_epoch, choices[i].fair_repair});
    to.assign(world.size, local);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  bool admitted = false;
  auto all = world.exchange(to, &admitted, options.memory_ceiling,
                            transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(),
                                                 directory->ResidentBytes(), scratch));
  if (!admitted) {
    result.reason = "Native edge proposal exchange exceeds storage.";
    return false;
  }
  return CoalesceOrder(context, choices, all, concurrent_refine, failure, feature);
}

bool EngineQueues::ServiceFrontier(EngineContext& context, Feature feature, bool active, bool accepted,
                                   const std::array<RepairReceipt, MaximumCandidateBatch>* extra, size_t count,
                                   unsigned disposition) {
  auto& world = context.world;
  auto& options = context.options;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& result = *context.control.result;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& checkpoint = *context.control.checkpoint;

  // Only the bounded elected frontier is replicated, never the resident demand queues.
  const bool certify_support = coarsening && !repairing && checkpoint.valid;
  const auto scratch = transfer_memory::Add(
      transfer_memory::Mul(world.size, sizeof(std::vector<RepairReceipt>)),
      transfer_memory::Mul(transfer_memory::Mul(world.size, MaximumCandidateBatch + 1), 4 * sizeof(RepairReceipt)));
  if (!context.exchange.Admit(
          context, transfer_memory::Add(
                       scratch, certify_support
                                    ? transfer_memory::Mul(transfer_memory::Mul(world.size, MaximumCandidateBatch + 1),
                                                           sizeof(Edge) + 4 * sizeof(void*))
                                    : 0)))
    return false;
  CLocalFailure failure;
  std::vector<std::vector<RepairReceipt>> to;
  try {
    if (count > MaximumCandidateBatch || (count && !extra))
      throw std::logic_error("Native frontier receipt list exceeds its bound.");
    to.resize(world.size);
    for (auto& bucket : to) {
      if (active) bucket.push_back({feature, accepted, disposition});
      for (size_t i = 0; i < count; ++i) bucket.push_back(extra->at(i));
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  bool admitted = false;
  auto receipts = world.exchange(to, &admitted, options.memory_ceiling,
                                 transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(),
                                                      directory->ResidentBytes(), scratch));
  if (!admitted) {
    result.reason = "Native repair service receipts exceed storage budget.";
    return false;
  }
  try {
    if (receipts.size() > transfer_memory::Mul(world.size, MaximumCandidateBatch + 1))
      throw std::logic_error("Native repair service receipts exceed frontier bound.");
    if (certify_support)
      std::sort(receipts.begin(), receipts.end(), [](const auto& a, const auto& b) {
        return std::tie(a.feature.count, a.feature.vertices, a.accepted) <
               std::tie(b.feature.count, b.feature.vertices, b.accepted);
      });
    for (const auto& receipt : receipts)
      if (receipt.feature.count < 1 || receipt.feature.count > 3 || (!repairing && receipt.feature.count != 2))
        throw std::logic_error("Invalid native repair service receipt.");
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (certify_support && !context.exchange.Vote(context, failure)) return false;
  return ServiceReceipts(context, receipts, certify_support, failure);
}

bool EngineQueues::ServiceReceipts(EngineContext& context, std::vector<RepairReceipt>& receipts, bool certify_support,
                                   CLocalFailure& failure) {
  auto& world = context.world;
  auto& selection = *context.census.selection;
  auto& repairing = *context.control.repairing;
  auto& checkpoint = *context.control.checkpoint;
  for (const auto& receipt : receipts) {
    if (!certify_support && failure.Failed()) break;
    const bool checkpoint_support =
        certify_support && !world.maximum(checkpoint.changed.count(receipt.feature.vertices[1]) != 0);
    try {
      if (checkpoint_support) {
        const Edge edge{receipt.feature.vertices[0], receipt.feature.vertices[1]};
        auto key = edge;
        std::sort(key.begin(), key.end());
        if (checkpoint.edges.count(key)) checkpoint.attempted.insert(edge);
      }
      if (repairing)
        selection.Service(receipt.feature, receipt.accepted);
      else {
        if (receipt.feature.count != 2) throw std::logic_error("Invalid native construction service receipt.");
        selection.Note({receipt.feature.vertices[0], receipt.feature.vertices[1]},
                       receipt.accepted ? 5 : receipt.disposition);
        selection.ServiceEdge({receipt.feature.vertices[0], receipt.feature.vertices[1]});
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    // Every rank reaches the next receipt support vote even after a local allocation failure.
    if (certify_support && !context.exchange.Vote(context, failure)) return false;
  }
  return certify_support || context.exchange.Vote(context, failure);
}

// Binds the live round slots without copying choice, dependency or receipt state.
RoundAcquisition::RoundAcquisition(EngineContext& context, RoundState& round_state,
                                   SU2NativeGeometry::AtlasBudget& atlas_budget, bool curved,
                                   size_t diagnostic_rejection_limit)
    : context(context),
      round_state(round_state),
      atlas_budget(atlas_budget),
      curved(curved),
      diagnostic_rejection_limit(diagnostic_rejection_limit) {}

// Finalizes before nested scheduling, topology and communication scopes unwind.
bool RoundAcquisition::FinishRound() {
  auto& working = *context.ownership.working;
  const auto& started = *context.setup.control_prefix.started;
  const auto& started_cpu = *context.setup.control_prefix.started_cpu;
  finished.emplace(EngineControl::Finish(context, curved, working, atlas_budget, started, started_cpu));
  return false;
}

// Keeps the original batched and single scheduling paths distinct.
bool RoundAcquisition::Acquire() { return *round_state.batched ? AcquireBatch() : AcquireSingle(); }

// Keeps the shared failure and slot allowance alive through reservation.
bool RoundAcquisition::AcquireBatch() {
  if (!AdmitBatchStorage()) return false;
  CLocalFailure failure;
  size_t repair_slots = context.options.candidate_batch;
  return SetRepairSlots(failure, repair_slots) && SelectBatch(failure, repair_slots) && AdmitBatchWork() &&
         AcquireBatchDependencies(failure) && ReserveBatch(failure);
}

// Records a refused choice and any existing frontier receipt.
void RoundAcquisition::Refuse(const Choice& candidate, const Feature& feature, const std::string& why) {
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& repairing = *context.control.repairing;
  auto& last_rejection = *context.control.last_rejection;
  auto& wall_counters = *context.setup.wall_counters;
  auto& planned_coarsening = *round_state.planned_coarsening;
  auto& batched = *round_state.batched;
  auto& refused_candidates = *round_state.refused_candidates;
  auto& refused_receipts = *round_state.refused_receipts;
  auto& refused_receipt_count = *round_state.refused_receipt_count;
  selection.Block(candidate.edge, candidate.feature, repairing);
  ++stats.attempted;
  ++stats.rejected;
  ++stats.oversized_stars;
  wall_counters.PlanningRefusal();
  if (batched || (!repairing && planned_coarsening)) {
    if (refused_receipt_count == MaximumCandidateBatch)
      throw std::logic_error("Native refused frontier exceeds batch bound.");
    refused_receipts[refused_receipt_count++] = {
        repairing ? feature : Feature{{candidate.edge[0], candidate.edge[1], 0}, 2}, false};
  }
  stats.last_rejected_feature = feature;
  ++refused_candidates;
  last_rejection = "feature " + std::to_string(feature.count) + ":" + std::to_string(feature.vertices[0]) + ": " + why;
  EngineReports::RecordRejection(context, diagnostic_rejection_limit, feature, nullptr, nullptr, why);
}

// Plans only the existing coarsening candidates and leaves its timer before returning failure.
bool RoundAcquisition::PlanChoices() {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& ready = *context.control.ready;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& choices = *round_state.choices;
  auto& dependency_batch = *round_state.dependency_batch;
  auto& planned_coarsening = *round_state.planned_coarsening;
  auto& refused_candidates = *round_state.refused_candidates;
  auto& refused_receipts = *round_state.refused_receipts;
  auto& refused_receipt_count = *round_state.refused_receipt_count;
  auto& pass_deferred_candidates = *round_state.pass_deferred_candidates;
  CLocalFailure failure;
  stats.batch_considered += choices.size();
  if (stats.global_work == stats.work_limit) {
    stats.stop = EngineStop::WorkBudget;
    result.reason = "Native 3D global proposal budget exhausted before admission; feasibility is undetermined.";
    return false;
  }
  if (planned_coarsening) {
    Timer timer(stats.planning_seconds, options.timings, SU2NativeTiming3D::Operation::Scheduling);
    std::vector<Feature> features;
    std::vector<size_t> selected;
    try {
      for (const auto& candidate : choices) features.push_back({{candidate.edge[1], 0, 0}, 1});
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, failure))) return false;
    dependency_batch =
        directory->PlanBatch(features, selected, options.memory_ceiling,
                             transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()));
    if (!dependency_batch.valid) {
      result.reason = dependency_batch.reason;
      return false;
    }
    try {
      for (size_t i : dependency_batch.refused)
        Refuse(choices.at(i), features.at(i), "Native 3D dependency star exceeds budget.");
      for (size_t i : dependency_batch.pass_deferred) {
        if (refused_receipt_count == MaximumCandidateBatch)
          throw std::logic_error("Native construction deferrals exceed frontier bound.");
        const auto edge = choices.at(i).edge;
        refused_receipts[refused_receipt_count++] = {Feature{{edge[0], edge[1], 0}, 2}, false};
        ++pass_deferred_candidates;
        ++stats.length_pass_deferred;
      }
      std::vector<Choice> planned;
      planned.reserve(selected.size());
      for (size_t i : selected) planned.push_back(choices.at(i));
      stats.batch_deferred += choices.size() - planned.size() - dependency_batch.refused.size();
      choices.swap(planned);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, failure))) return false;
    if (world.maximum(refused_receipt_count != 0)) {
      if (!context.queues.ServiceFrontier(context, {}, false, false, &refused_receipts, refused_receipt_count))
        return false;
      refused_receipt_count = 0;
    }
    if (!world.maximum(!choices.empty() || refused_candidates != 0 || pass_deferred_candidates != 0)) {
      result.reason = "Native 3D complete-star selection made no progress.";
      return false;
    }
  }
  return true;
}

// Allocates repair slots in the same proportion as the cached debt share.
bool RoundAcquisition::SetRepairSlots(CLocalFailure& failure, size_t& repair_slots) {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& selection = *context.census.selection;
  auto& repairing = *context.control.repairing;
  if (repairing) {
    double local_debt = 0;
    for (const auto& row : selection.repair_cell_severity) local_debt += double(row.second);
    if (!std::isfinite(local_debt) || local_debt < 0) failure.Set(1, 0, "Invalid cached required repair debt.");
    if (!context.exchange.Vote(context, failure)) return FinishRound();
    const bool complete = !world.maximum(!selection.progress_consistent || selection.cells.size() != working.size() ||
                                         selection.unmeasured_shape != 0);
    const double total_debt = TimedAllreduce(local_debt, CPassiveComm::Op::SUM, world.comm);
    if (complete && std::isfinite(total_debt) && total_debt > 0) {
      const double share = std::clamp(local_debt / total_debt * world.size, 0., 1.);
      repair_slots = std::max<size_t>(1, size_t(std::ceil(options.candidate_batch * share)));
    }
  }
  return true;
}

// Selects and coalesces choices before agreeing the slot count.
bool RoundAcquisition::SelectBatch(CLocalFailure& failure, size_t repair_slots) {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& ready = *context.control.ready;
  auto& repairing = *context.control.repairing;
  auto& choices = *round_state.choices;
  auto& slots = *round_state.slots;
  auto& refused_candidates = *round_state.refused_candidates;
  Timer timer(stats.selection_seconds, options.timings, SU2NativeTiming3D::Operation::Scheduling);
  try {
    bool oversized = false;
    choices = repairing ? selection.RepairBatch(working, repair_slots, oversized)
                        : selection.Batch(working, options.candidate_batch);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!(ready = context.exchange.Vote(context, failure))) return FinishRound();
  if (!context.queues.Coalesce(context, choices, curved)) return FinishRound();
  if (!PlanChoices()) return FinishRound();
  slots = TimedAllreduce(choices.size(), CPassiveComm::Op::MAX, world.comm);
  ++stats.batch_rounds;
  stats.batch_candidates += choices.size();
  stats.proposed += choices.size() + refused_candidates;
  return true;
}

// Keeps the single-candidate election and its work admission under one Scheduling timer.
bool RoundAcquisition::AcquireSingle() {
  auto& world = context.world;
  auto& options = context.options;
  auto& composition = context.composition;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& repairing = *context.control.repairing;
  auto& coarsening = *context.control.coarsening;
  auto& choice = *round_state.choice;
  auto& choices = *round_state.choices;
  auto& concurrent = *round_state.concurrent;
  Timer timer(stats.selection_seconds, options.timings, SU2NativeTiming3D::Operation::Scheduling);
  if ((curved || composition) && !repairing && !concurrent) {
    // Single-proposal requests and coarsening retain their established serial ordering.
    if (composition && !curved && !repairing) {
      // Choose global metric priority before ownership; rank numbering must not decide the next edit.
      const double priority = choice.active ? double(selection.Priority(choice.measure.length)) : INFINITY;
      const double best = TimedAllreduce(priority, CPassiveComm::Op::MIN, world.comm);
      choice.active = choice.active && priority == best;
      const Id first = TimedAllreduce(choice.active ? choice.edge[0] : UINT64_MAX, CPassiveComm::Op::MIN, world.comm);
      choice.active = choice.active && choice.edge[0] == first;
      const Id second = TimedAllreduce(choice.active ? choice.edge[1] : UINT64_MAX, CPassiveComm::Op::MIN, world.comm);
      choice.active = choice.active && choice.edge[1] == second;
    }
    const int owner = TimedAllreduce(choice.active ? world.rank : world.size, CPassiveComm::Op::MIN, world.comm);
    choice.active = choice.active && world.rank == owner;
  }
  if (repairing || concurrent || (coarsening && !repairing && !curved && !composition && world.size > 1)) {
    CLocalFailure failure;
    try {
      if (choice.active) choices.push_back(choice);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure) || !context.queues.Coalesce(context, choices, curved) ||
        !PlanChoices())
      return FinishRound();
    choice = choices.empty() ? Choice{} : choices.front();
  }
  stats.proposed += choice.active;
  const uint64_t proposed = TimedAllreduce(uint64_t(choice.active), CPassiveComm::Op::SUM, world.comm);
  if (proposed > stats.work_limit - stats.global_work) {
    stats.stop = EngineStop::WorkBudget;
    result.reason = "Native 3D global proposal budget exhausted before admission; feasibility is undetermined.";
    return FinishRound();
  }
  stats.global_work += proposed;
  return true;
}
