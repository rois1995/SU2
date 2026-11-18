// Owns only empty optional slots; Adapt-local ContextValues govern payload lifetimes.
#include "EngineCommit.hpp"

EngineCommit::EngineCommit() = default;
EngineCommit::~EngineCommit() = default;

// Batch-only routing buffers are active from routing preparation through publication and census update.
struct RepairBatchRoutes {
  std::vector<std::vector<Dependency>> removals;
  std::vector<std::vector<OwnedCell>> additions;
  std::vector<std::vector<BlockedEdge>> blocked;
  std::vector<BoundaryParameterEdit> parameter_edits;
  SU2NativeGeometry::BoundaryCoverage coverage;
  std::array<RepairReceipt, MaximumCandidateBatch> receipts{};
  size_t receipt_count = 0;

  RepairBatchRoutes() = default;
  RepairBatchRoutes(const RepairBatchRoutes&) = delete;
  RepairBatchRoutes& operator=(const RepairBatchRoutes&) = delete;
  RepairBatchRoutes(RepairBatchRoutes&&) = delete;
  RepairBatchRoutes& operator=(RepairBatchRoutes&&) = delete;
};

// The original three received lists stay live together until census update completes.
struct RepairBatchChanges {
  std::vector<Dependency> removed;
  std::vector<OwnedCell> added;
  std::vector<BlockedEdge> failures;

  RepairBatchChanges() = default;
  RepairBatchChanges(const RepairBatchChanges&) = delete;
  RepairBatchChanges& operator=(const RepairBatchChanges&) = delete;
  RepairBatchChanges(RepairBatchChanges&&) = delete;
  RepairBatchChanges& operator=(RepairBatchChanges&&) = delete;
};

// Uses the approved private proposals in receive order and retains their batch-only conflict rules.
class RepairBatchCommit {
 public:
  RepairBatchCommit(EngineContext&, RoundState&, SU2NativeGeometry::AtlasBudget&, bool curved,
                    size_t diagnostic_rejection_limit);
  bool Run();
  std::optional<EngineResult> finished;

 private:
  EngineContext& context;
  RoundState& round_state;
  SU2NativeGeometry::AtlasBudget& atlas_budget;
  bool curved;
  size_t diagnostic_rejection_limit;

  bool FinishRound();
  void ReportPrivateWork();
  bool JoinPrivateWork();
  bool BuildContactList(CLocalFailure&, std::vector<ClearanceCandidate>&, bool& needs_contact);
  bool CheckContact(std::vector<ClearanceCandidate>&, bool needs_contact);
  bool ReserveFinalInputs(CLocalFailure&, ImportedBatch&, std::vector<unsigned>&, std::optional<ReservationBatch>&);
  void CountReserved(ImportedBatch&, const ReservationBatch&);
  void CollectWallEdits(RepairProposal&, RepairBatchRoutes&);
  void RouteApproved(RepairProposal&, RepairBatchRoutes&);
  void RouteRefused(const RepairProposal&, RepairBatchRoutes&);
  void FinishProposalRouting(const RepairProposal&, RepairBatchRoutes&);
  bool AssembleRoutes(CLocalFailure&, RepairBatchRoutes&);
  bool ExchangeRoutes(RepairBatchRoutes&, RepairBatchChanges&);
  bool AdmitRoutedChanges(RepairBatchRoutes&, RepairBatchChanges&, const std::vector<ClearanceCandidate>&);
  bool Publish(RepairBatchRoutes&, RepairBatchChanges&);
  bool ServiceReceipts(RepairBatchRoutes&, RepairBatchChanges&);
  void UpdateRows(RepairBatchChanges&);
  void CountPublishedRepairs();
  bool UpdateSelection(CLocalFailure&, RepairBatchChanges&);
};

// Borrows round state and keeps its private work and proof accounting bound to that state.
RepairBatchCommit::RepairBatchCommit(EngineContext& context, RoundState& round_state,
                                     SU2NativeGeometry::AtlasBudget& atlas_budget, bool curved,
                                     size_t diagnostic_rejection_limit)
    : context(context),
      round_state(round_state),
      atlas_budget(atlas_budget),
      curved(curved),
      diagnostic_rejection_limit(diagnostic_rejection_limit) {}

// Evaluates finalization while the batch's live timer and transaction buffers are still in scope.
bool RepairBatchCommit::FinishRound() {
  auto& working = *context.ownership.working;
  const auto& started = *context.setup.control_prefix.started;
  const auto& started_cpu = *context.setup.control_prefix.started_cpu;
  finished.emplace(EngineControl::Finish(context, curved, working, atlas_budget, started, started_cpu));
  return false;
}

// Joins local work before contact, final reservation, routing, publication and census update.
bool RepairBatchCommit::Run() {
  ReportPrivateWork();
  if (!JoinPrivateWork()) return false;
  CLocalFailure failure;
  std::vector<ClearanceCandidate> contact;
  bool needs_contact = false;
  if (!BuildContactList(failure, contact, needs_contact) || !CheckContact(contact, needs_contact)) return false;
  ImportedBatch final_inputs;
  std::vector<unsigned> priorities;
  std::optional<ReservationBatch> final_reserved;
  if (!ReserveFinalInputs(failure, final_inputs, priorities, final_reserved)) return false;
  CountReserved(final_inputs, *final_reserved);
  RepairBatchRoutes routes;
  if (!AssembleRoutes(failure, routes)) return false;
  RepairBatchChanges changes;
  return ExchangeRoutes(routes, changes) && AdmitRoutedChanges(routes, changes, contact) && Publish(routes, changes) &&
         ServiceReceipts(routes, changes) && UpdateSelection(failure, changes);
}

// Records private work before waiting for the other ranks.
void RepairBatchCommit::ReportPrivateWork() {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& private_work_used = *round_state.private_work_used;
  auto& private_started = *round_state.private_started;
  auto& private_collectives = *round_state.private_collectives;
  const double private_busy_seconds = std::chrono::duration<double>(Clock::now() - private_started).count();
  stats.quality.busy_seconds += private_busy_seconds;
  stats.quality.private_work_used += private_work_used;
  if (options.timings) {
    SU2NativeTiming3D::Scope output_timer(SU2NativeTiming3D::Operation::Output);
    std::cout << "NATIVE3D_PRIVATE_WORK rank=" << world.rank << " round=" << stats.rounds
              << " private_work_used=" << private_work_used << " busy_seconds=" << private_busy_seconds << '\n';
  }
  stats.quality.private_tracked_collectives +=
      world.collective_calls + world.election_collective_calls - private_collectives;
}

// Performs the first post-slot collective and accounts for the common work reservation.
bool RepairBatchCommit::JoinPrivateWork() {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& started = *context.setup.control_prefix.started;
  auto& private_failure = *round_state.private_failure;
  auto& private_work_used = *round_state.private_work_used;
  auto& private_work_reserved = *round_state.private_work_reserved;
  auto& private_deadline = *round_state.private_deadline;
  const auto joined = Clock::now();
  // The legacy counter still surrounds the same call; F3 names its join wait separately.
  const auto used = [&]() {
    SU2NativeTiming3D::Scope join_timer(SU2NativeTiming3D::Operation::JoinWait);
    return CPassiveComm::Allreduce(private_work_used, CPassiveComm::Op::SUM, world.comm);
  }();
  stats.quality.join_wait_seconds += std::chrono::duration<double>(Clock::now() - joined).count();
  if (used > private_work_reserved)
    private_failure.Set(2, 0, "Native3D private repair exceeds its common work reservation.");
  else
    stats.global_work -= private_work_reserved - used;
  private_deadline =
      private_deadline || (options.maximum_seconds &&
                           std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds);
  if (world.maximum(private_deadline)) {
    stats.stop = EngineStop::TimeBudget;
    private_failure.Set(1, 0, "Native3D private repair round exhausts elapsed-time budget.");
  }
  if (!context.exchange.Vote(context, private_failure)) return FinishRound();
  return true;
}

// Prepares clearance candidates with complete current dependency versions.
bool RepairBatchCommit::BuildContactList(CLocalFailure& failure, std::vector<ClearanceCandidate>& contact,
                                         bool& needs_contact) {
  auto& repair_proposals = *round_state.repair_proposals;
  {
    SU2NativeTiming3D::Scope contact_timer(SU2NativeTiming3D::Operation::Clearance);
    try {
      contact.reserve(repair_proposals.size());
      for (auto& proposal : repair_proposals) {
        if (proposal.approved) {
          if (proposal.old.cells.size() != proposal.imported.cells.size() ||
              proposal.dependencies.cells.size() != proposal.imported.cells.size())
            throw std::runtime_error("Native3D final repair lacks its complete imported dependencies.");
          std::set<Id> cells;
          for (const auto& c : proposal.old.cells)
            if (!cells.insert(c.id).second) throw std::runtime_error("Native3D final repair repeats an old cell.");
          for (const auto& dependency : proposal.dependencies.cells) {
            const auto row = std::find_if(proposal.imported.cells.begin(), proposal.imported.cells.end(),
                                          [&](const OwnedCell& c) { return c.cell.id == dependency.cell; });
            if (!cells.erase(dependency.cell) || row == proposal.imported.cells.end() ||
                row->version != dependency.version)
              throw std::runtime_error("Native3D final repair lost a versioned old dependency.");
          }
          needs_contact = true;  // Full vertex disjointness does not certify simultaneous physical contacts.
        }
        contact.push_back({&proposal.old,
                           &proposal.fresh,
                           proposal.approved,
                           false,
                           proposal.choice.reservation_priority,
                           proposal.coordinate_only ? proposal.imported.coordinate_star : Feature{},
                           {}});
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
  }
  if (!context.exchange.Vote(context, failure)) return FinishRound();
  return true;
}

// Applies the existing distributed clearance decisions to proposals.
bool RepairBatchCommit::CheckContact(std::vector<ClearanceCandidate>& contact, bool needs_contact) {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& repair_proposals = *round_state.repair_proposals;
  if (world.maximum(needs_contact)) {
    if (!directory->ClearanceBatch(working, contact, result.reason, options.memory_ceiling,
                                   transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes())))
      return FinishRound();
    for (size_t i = 0; i < contact.size(); ++i) {
      auto& proposal = repair_proposals[i];
      const auto& checked = contact[i];
      if (checked.deferred) {
        proposal.chosen = false;
        ++stats.conflicts;
        ++stats.rejected;
        ++stats.proposal_deferred;
      } else if (proposal.approved && !checked.candidate)
        EngineTrial::GeometryTrial(context, false);
      if (proposal.approved && !checked.candidate && proposal.ladder < 3)
        ++stats.quality.ladder[proposal.ladder].rejected[5];
      proposal.approved = checked.candidate;
      if (!checked.reason.empty()) proposal.rejection = checked.reason;
    }
  }
  return true;
}

// Reserves the final proposals in their existing order, lending the imported cells to the reservation.
bool RepairBatchCommit::ReserveFinalInputs(CLocalFailure& failure, ImportedBatch& final_inputs,
                                           std::vector<unsigned>& priorities,
                                           std::optional<ReservationBatch>& reservation) {
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& repair_proposals = *round_state.repair_proposals;
  try {
    final_inputs.valid = true;
    final_inputs.candidates.resize(repair_proposals.size());
    priorities.reserve(repair_proposals.size());
    for (size_t i = 0; i < repair_proposals.size(); ++i) {
      auto& proposal = repair_proposals[i];
      priorities.push_back(proposal.choice.reservation_priority);
      if (proposal.approved) final_inputs.candidates[i] = std::move(proposal.imported);
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return FinishRound();
  auto& final_reserved = reservation.emplace();
  {
    Timer timer(stats.reservation_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
    ++stats.final_reservations;
    final_reserved = directory->ReserveBatch(
        final_inputs, options.memory_ceiling,
        transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes()), priorities);
  }
  if (!final_reserved.valid) {
    result.reason = final_reserved.reason;
    return FinishRound();
  }
  return true;
}

// Restores imports and counts only the proposals that retain their final reservation.
void RepairBatchCommit::CountReserved(ImportedBatch& final_inputs, const ReservationBatch& final_reserved) {
  auto& world = context.world;
  auto& stats = context.control.result->stats;
  auto& repair_proposals = *round_state.repair_proposals;
  uint64_t commits = 0;
  for (size_t i = 0; i < repair_proposals.size(); ++i) {
    auto& proposal = repair_proposals[i];
    if (proposal.approved) {
      proposal.imported = std::move(final_inputs.candidates[i]);
      if (!final_reserved.chosen.at(i)) {
        if (proposal.ladder < 3) ++stats.quality.ladder[proposal.ladder].rejected[6];
        proposal.approved = proposal.chosen = false;
        ++stats.conflicts;
        ++stats.rejected;
        ++stats.proposal_deferred;
      }
    }
    commits += proposal.approved;
  }
  commits = TimedAllreduce(commits, CPassiveComm::Op::SUM, world.comm);
  stats.maximum_parallel_commits = std::max(stats.maximum_parallel_commits, commits);
}

// Collects parameter edits and describes wall commits only for timing output.
void RepairBatchCommit::CollectWallEdits(RepairProposal& proposal, RepairBatchRoutes& routes) {
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& selection = *context.census.selection;
  auto& repair_proposals = *round_state.repair_proposals;
  auto& lane_slots = *round_state.lane_slots;
  auto& parameter_edits = routes.parameter_edits;
  if (geometry.Mapped()) {
    auto edits = BoundaryParameterState::Changes(proposal.old, proposal.fresh, geometry.Parameters()->Revision());
    if (options.timings) {
      auto& lane_slot = lane_slots[&proposal - repair_proposals.data()];
      lane_slot.no_parameter_edits = edits.empty();
      if (lane_slot.category == 1)
        lane_census::DescribeWall(
            lane_slot, proposal.old, proposal.fresh, edits, SameSkin(proposal.old, proposal.fresh),
            proposal.fresh.coverage.faces.empty() && proposal.fresh.coverage.traces.empty(),
            WallLinkLocal(proposal.old, geometry.Parameters(), selection, working), proposal.ladder);
    }
    parameter_edits.insert(parameter_edits.end(), edits.begin(), edits.end());
  } else if (options.timings) {
    auto& lane_slot = lane_slots[&proposal - repair_proposals.data()];
    if (lane_slot.category == 1)
      lane_census::DescribeWall(lane_slot, proposal.old, proposal.fresh, {}, SameSkin(proposal.old, proposal.fresh),
                                proposal.fresh.coverage.faces.empty() && proposal.fresh.coverage.traces.empty(),
                                WallLinkLocal(proposal.old, geometry.Parameters(), selection, working),
                                proposal.ladder);
  }
}

// Nets coverage and routes new metric-bearing cells to the owner of each original parent.
void RepairBatchCommit::RouteApproved(RepairProposal& proposal, RepairBatchRoutes& routes) {
  auto& removals = routes.removals;
  auto& additions = routes.additions;
  auto& coverage = routes.coverage;
  CollectWallEdits(proposal, routes);
  for (const auto& face : proposal.fresh.coverage.faces)
    if (!coverage.faces.emplace(face).second)
      throw std::runtime_error("Native3D private repairs share a boundary coverage face.");
  for (const auto& trace : proposal.fresh.coverage.traces)
    if (!coverage.traces.emplace(trace).second)
      throw std::runtime_error("Native3D private repairs share a boundary coverage trace.");
  std::map<Id, int> owners;
  std::map<Id, uint64_t> versions;
  for (const auto& dependency : proposal.dependencies.cells) {
    owners.emplace(dependency.cell, dependency.owner);
    versions.emplace(dependency.cell, dependency.version);
    removals.at(dependency.owner).push_back(dependency);
  }
  std::vector<OwnedCell> metric_rows;
  for (const auto& cell : proposal.fresh.cells) {
    OwnedCell row;
    row.cell = cell;
    metric_rows.push_back(row);
  }
  PublishMetrics(proposal.imported.cells, metric_rows);
  std::map<Face, Id> physical;
  for (const auto& face : proposal.fresh.surface) physical.emplace(Key(face.v), face.facet);
  if (proposal.coordinate_only && proposal.fresh.cells.size() != proposal.old.cells.size())
    throw std::logic_error("Native3D coordinate repair changes cell count.");
  for (size_t i = 0; i < metric_rows.size(); ++i) {
    auto& row = metric_rows[i];
    const Id parent = proposal.coordinate_only ? row.cell.id : proposal.old.cells.at(i % proposal.old.cells.size()).id;
    if (proposal.coordinate_only) {
      const auto version = versions.at(row.cell.id);
      if (version == UINT64_MAX) throw std::overflow_error("Native3D coordinate repair version overflows.");
      row.version = version + 1;
    }
    const auto faces = Faces(row.cell);
    for (size_t f = 0; f < 4; ++f) {
      const auto found = physical.find(Key(faces[f]));
      if (found != physical.end()) row.physical[f] = {true, found->second};
    }
    additions.at(owners.at(parent)).push_back(std::move(row));
  }
}

// Routes refused proposals without changing their original support or receipt disposition.
void RepairBatchCommit::RouteRefused(const RepairProposal& proposal, RepairBatchRoutes& routes) {
  auto& stats = context.control.result->stats;
  auto& last_rejection = *context.control.last_rejection;
  auto& blocked = routes.blocked;
  ++stats.rejected;
  stats.last_metric_rejection = proposal.certified;
  stats.last_rejected_feature = proposal.choice.feature;
  if (!proposal.old.cells.empty()) stats.last_rejected_cavity_centroid = Centroid(proposal.old.cells.front());
  last_rejection = "feature " + std::to_string(proposal.choice.feature.count) + ":" +
                   std::to_string(proposal.choice.feature.vertices[0]) + ": " + proposal.rejection;
  EngineReports::RecordRejection(context, diagnostic_rejection_limit, proposal.choice.feature, &proposal.old,
                                 &proposal.certified, proposal.rejection);
  std::set<int> owners;
  for (const auto& dependency : proposal.dependencies.cells) owners.insert(dependency.owner);
  for (int owner : owners) blocked.at(owner).push_back({proposal.choice.edge, proposal.choice.feature});
}

// Emits the existing receipt and observer call after the proposal has been routed.
void RepairBatchCommit::FinishProposalRouting(const RepairProposal& proposal, RepairBatchRoutes& routes) {
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& composition = context.composition;
  auto& round = round_state.round;
  auto& receipts = routes.receipts;
  auto& receipt_count = routes.receipt_count;
  if (proposal.chosen) {
    if (receipt_count == MaximumCandidateBatch) throw std::logic_error("Native3D repair receipts exceed round bound.");
    receipts[receipt_count++] = {
        proposal.choice.feature, proposal.approved,
        proposal.certified.failure == MetricFailure::None ? 4u : 10u + unsigned(proposal.certified.failure)};
  }
  if (proposal.capture)
    options.transaction_observer({"transaction_end", proposal.choice.feature, uint64_t(round),
                                  proposal.capture_selected, proposal.approved, working, geometry,
                                  proposal.dependencies, proposal.imported, proposal.old, proposal.fresh,
                                  proposal.point, proposal.first_cell, proposal.first_node, proposal.certified,
                                  proposal.rejection, nullptr, composition, options.edge_breakpoints});
}

// Assembles routes for all proposals under the existing routing timer and failure vote.
bool RepairBatchCommit::AssembleRoutes(CLocalFailure& failure, RepairBatchRoutes& routes) {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& repair_proposals = *round_state.repair_proposals;
  auto& removals = routes.removals;
  auto& additions = routes.additions;
  auto& blocked = routes.blocked;
  {
    Timer timer(stats.routing_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
    try {
      removals.resize(world.size);
      additions.resize(world.size);
      blocked.resize(world.size);
      for (auto& proposal : repair_proposals) {
        if (proposal.approved)
          RouteApproved(proposal, routes);
        else if (proposal.chosen)
          RouteRefused(proposal, routes);
        FinishProposalRouting(proposal, routes);
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
  }
  if (!context.exchange.Vote(context, failure)) return FinishRound();
  return true;
}

// Exchanges each route with the bytes already received still charged.
bool RepairBatchCommit::ExchangeRoutes(RepairBatchRoutes& routes, RepairBatchChanges& changes) {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& removals = routes.removals;
  auto& additions = routes.additions;
  auto& blocked = routes.blocked;
  auto& removed = changes.removed;
  auto& added = changes.added;
  auto& failures = changes.failures;
  {
    Timer timer(stats.routing_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
    bool admitted = false;
    const auto baseline =
        transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), directory->ResidentBytes());
    removed = world.exchange(removals, &admitted, options.memory_ceiling, baseline);
    if (!admitted) {
      result.reason = "Native3D round removal routing exceeds storage budget.";
      return FinishRound();
    }
    added = world.exchange(additions, &admitted, options.memory_ceiling,
                           transfer_memory::Add(baseline, transfer_memory::Bytes(removed)));
    if (!admitted) {
      result.reason = "Native3D round addition routing exceeds storage budget.";
      return FinishRound();
    }
    failures =
        world.exchange(blocked, &admitted, options.memory_ceiling,
                       transfer_memory::Add(baseline, transfer_memory::Bytes(removed), transfer_memory::Bytes(added)));
    if (!admitted) {
      result.reason = "Native3D round rejection routing exceeds storage budget.";
      return FinishRound();
    }
  }
  return true;
}

// Charges all simultaneous routing buffers before publication.
bool RepairBatchCommit::AdmitRoutedChanges(RepairBatchRoutes& routes, RepairBatchChanges& changes,
                                           const std::vector<ClearanceCandidate>& contact) {
  auto& batch_bytes = *context.control.batch_bytes;
  auto& removals = routes.removals;
  auto& additions = routes.additions;
  auto& blocked = routes.blocked;
  auto& parameter_edits = routes.parameter_edits;
  auto& coverage = routes.coverage;
  auto& removed = changes.removed;
  auto& added = changes.added;
  auto& failures = changes.failures;
  batch_bytes = transfer_memory::Add(
      batch_bytes, transfer_memory::Bytes(removed), transfer_memory::Bytes(added), transfer_memory::Bytes(failures),
      transfer_memory::Bytes(removals), transfer_memory::Bytes(additions), transfer_memory::Bytes(blocked),
      transfer_memory::Bytes(contact), transfer_memory::Bytes(parameter_edits), BoundaryCoverageBytes(coverage),
      transfer_memory::GrowthBound(added.size(), sizeof(Id)));
  if (!context.exchange.Admit(context, 0)) return FinishRound();
  return true;
}

// Publishes the approved batch, preserving the parameter and coverage transaction.
bool RepairBatchCommit::Publish(RepairBatchRoutes& routes, RepairBatchChanges& changes) {
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& parameter_edits = routes.parameter_edits;
  auto& coverage = routes.coverage;
  auto& removed = changes.removed;
  auto& added = changes.added;
  {
    Timer timer(stats.publication_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
    if (!directory->Publish(
            working, removed, added, true, result.reason, options.memory_ceiling,
            transfer_memory::Add(context.exchange.RealBytes(context), source.ResidentBytes(), MaximumFrozenFieldBytes),
            parameter_edits, coverage, geometry.AtlasReservation(), true))
      return FinishRound();
  }
  return true;
}

// Services the existing frontier only after publication and fixed-metric ID refresh.
bool RepairBatchCommit::ServiceReceipts(RepairBatchRoutes& routes, RepairBatchChanges& changes) {
  auto& selection = *context.census.selection;
  auto& receipts = routes.receipts;
  auto& receipt_count = routes.receipt_count;
  auto& removed = changes.removed;
  auto& added = changes.added;
  EngineReports::UpdateFixedMetricCells(context, removed, added);
  if (!removed.empty() || !added.empty()) selection.progress_consistent = false;
  if (!context.queues.ServiceFrontier(context, {}, false, false, &receipts, receipt_count)) return FinishRound();
  return true;
}

// Removes stale census rows, records new cells and blocks failed support.
void RepairBatchCommit::UpdateRows(RepairBatchChanges& changes) {
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& retired_edges = *context.census.retired_edges;
  auto& batch_changed = *context.commit.batch_changed;
  auto& checkpoint = *context.control.checkpoint;
  auto& removed = changes.removed;
  auto& added = changes.added;
  auto& failures = changes.failures;
  for (const auto& row : removed) {
    const auto cached = selection.cells.find(row.cell);
    if (cached == selection.cells.end()) {
      ++stats.unmeasured_retired;
      continue;
    }
    if (checkpoint.valid) checkpoint.changed.insert(cached->second.vertices.begin(), cached->second.vertices.end());
    for (const auto& edge : cached->second.edges) retired_edges.emplace(edge, selection.edges.at(edge).measure);
    selection.Drop(row.cell);
  }
  batch_changed.reserve(added.size());
  for (const auto& row : added) {
    batch_changed.push_back(row.cell.id);
    if (checkpoint.valid)
      for (const auto& node : row.cell.v) checkpoint.changed.insert(node.id);
  }
  for (const auto& blocked_feature : failures) selection.Block(blocked_feature.edge, blocked_feature.feature, true);
}

// Accounts for each published repair while retaining per-ladder and wall diagnostics.
void RepairBatchCommit::CountPublishedRepairs() {
  auto& options = context.options;
  auto& surface_target = context.surface_target;
  auto& stats = context.control.result->stats;
  auto& improvement_changed = *context.setup.control_prefix.improvement_changed;
  auto& wall_counters = *context.setup.wall_counters;
  auto& lane_counters = *context.setup.lane_counters;
  auto& checkpoint = *context.control.checkpoint;
  auto& standalone_coarsening = *context.control.standalone_coarsening;
  auto& repair_proposals = *round_state.repair_proposals;
  auto& lane_slots = *round_state.lane_slots;
  for (const auto& proposal : repair_proposals)
    if (proposal.approved) {
      if (standalone_coarsening && checkpoint.valid) checkpoint.trial = true;
      improvement_changed = true;
      ++stats.accepted;
      wall_counters.Accepted();
      ++stats.quality.commits;
      if (options.timings) {
        const auto& lane_slot = lane_slots[&proposal - repair_proposals.data()];
        lane_counters.Commit(
            true, lane_slot,
            lane_slot.wall.described ? lane_slot.wall.same_skin : SameSkin(proposal.old, proposal.fresh),
            lane_slot.no_parameter_edits,
            proposal.fresh.coverage.faces.empty() && proposal.fresh.coverage.traces.empty());
        lane_counters.CommitWall(true, lane_slot);
      }
      if (!surface_target)
        AccountQualityChurn(proposal.old, proposal.fresh, proposal.old_flags, proposal.fresh_flags, stats.quality);
      if (proposal.ladder < 3) {
        auto& counter = stats.quality.ladder[proposal.ladder];
        ++counter.published;
        counter.debt_removed += proposal.certified.old_shape_debt - proposal.certified.new_shape_debt;
        counter.q_removed += int64_t(proposal.certified.old_q_bad_cells) - int64_t(proposal.certified.new_q_bad_cells);
        counter.j_removed += int64_t(proposal.certified.old_j_bad_cells) - int64_t(proposal.certified.new_j_bad_cells);
      }
      if (proposal.optimization_approved) {
        stats.optimization_moves += proposal.optimization.moves;
        stats.optimization_insertions += proposal.optimization.insertions;
        stats.optimization_collapses += proposal.optimization.collapses;
        stats.optimization_swaps += proposal.optimization.swaps;
      }
      stats.geometry_progress += proposal.geometric_progress;
      stats.size_progress += proposal.certified.size_progress;
      stats.quality_progress += proposal.certified.quality_progress;
      if (proposal.cavity_inserted)
        ++stats.inserted;
      else if (proposal.repair_approved) {
        if (proposal.moved)
          ++stats.moved;
        else
          ++stats.reconnected;
      }
    }
}

// Rebuilds the affected census before reactivating touched support.
bool RepairBatchCommit::UpdateSelection(CLocalFailure& failure, RepairBatchChanges& changes) {
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& retired_edges = *context.census.retired_edges;
  auto& batch_changed = *context.commit.batch_changed;
  auto& directory = *context.exchange.directory;
  auto& checkpoint = *context.control.checkpoint;
  auto& removed = changes.removed;
  auto& added = changes.added;
  Timer timer(stats.update_seconds, options.timings, SU2NativeTiming3D::Operation::Topology);
  if (!context.exchange.Admit(
          context,
          transfer_memory::Add(
              transfer_memory::Mul(working.size(), context.control.SelectionBound(context, true)),
              transfer_memory::Mul(transfer_memory::Mul(6, removed.size()),
                                   sizeof(std::pair<const Edge, EdgeMeasure>) + 4 * sizeof(void*)),
              transfer_memory::Mul(transfer_memory::Mul(14, removed.size()), sizeof(RepairFeature) + 4 * sizeof(void*)),
              checkpoint.valid
                  ? transfer_memory::Mul(transfer_memory::Mul(4, transfer_memory::Add(removed.size(), added.size())),
                                         sizeof(Id) + 4 * sizeof(void*))
                  : 0)))
    return FinishRound();
  try {
    UpdateRows(changes);
    CountPublishedRepairs();
    stats.maximum_owned = std::max(stats.maximum_owned, working.size());
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure) ||
      !context.census.Measure(context, batch_changed, false, &stats.update_seconds))
    return FinishRound();
  try {
    selection.FinishRepairs();
    for (const auto& support : directory->ChangedRepairSupports()) selection.TouchRepairSupport(support);
    for (Id vertex : directory->ChangedVertices()) selection.Touch(vertex);
    selection.progress_consistent = true;
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return FinishRound();
  batch_changed.clear();
  retired_edges.clear();
  return true;
}
