// Owns only empty optional slots; Adapt-local ContextValues govern payload lifetimes.
#include "EngineCommit.hpp"

EngineCommit::EngineCommit() = default;
EngineCommit::~EngineCommit() = default;

namespace commit_cert = geometry_certificates;

// The exported patch stores the actual cell coordinates and its forward bindings.
std::optional<commit_cert::Key> CommitFaceKey(const BoundaryParameterState& state, const SurfaceFace& face,
                                             const BoundaryPoints* patch_points) {
  std::array<commit_cert::Binding, 3> bindings;
  std::array<Point, 3> points;
  for (size_t i = 0; i < face.v.size(); ++i) {
    const BoundaryPoint* point;
    if (patch_points) {
      const auto found = patch_points->find(face.v[i]);
      point = found == patch_points->end() ? nullptr : &found->second;
    } else {
      point = state.Find(face.v[i]);
    }
    if (!point) return std::nullopt;
    points[i] = point->node.p;
    bindings[i] = point->binding;
  }
  return commit_cert::FaceKey(state, face.facet, bindings, points);
}

// Retain only whole-face bounds of changed physical faces that survived publication.
void RetainCommitCertificates(commit_cert::Event& certificates, const BoundaryParameterState& state,
                              const Patch& old, const Patch& fresh, const commit_cert::Budget& budget) {
  for (const auto& face : fresh.surface) {
    const auto candidate = CommitFaceKey(state, face, &fresh.parameters);
    if (!candidate || certificates.Find(*candidate)) continue;
    const auto found = std::lower_bound(
        certificates.trial.begin(), certificates.trial.end(), *candidate,
        [](const commit_cert::Record& row, const commit_cert::Key& key) { return row.key < key; });
    if (found == certificates.trial.end() || !(found->key == *candidate)) continue;
    const auto published = state.Surface().find(Key(face.v));
    if (published == state.Surface().end() || !(CommitFaceKey(state, published->second, nullptr) == candidate))
      continue;
    if (std::any_of(old.surface.begin(), old.surface.end(), [&](const SurfaceFace& previous) {
          return previous.facet == face.facet && CommitFaceKey(state, previous, &old.parameters) == candidate;
        }))
      continue;
    const auto live = transfer_memory::Add(budget.caller_bytes, certificates.Bytes() - certificates.atlas_charged_bytes);
    auto& records = certificates.committed;
    const auto index = std::lower_bound(records.begin(), records.end(), *found, commit_cert::ByKey) - records.begin();
    certificates.Push(records, *found, live, budget.maximum_bytes);
    std::rotate(records.begin() + index, records.end() - 1, records.end());
  }
}

// Batch-only routing buffers are active from routing preparation through publication and census update.
struct RepairBatchRoutes {
  std::vector<std::vector<Dependency>> removals;
  std::vector<std::vector<OwnedCell>> additions;
  std::vector<std::vector<BlockedEdge>> blocked;
  std::vector<BoundaryParameterEdit> parameter_edits;
  SU2NativeGeometry::BoundaryCoverage coverage;
  std::array<RepairReceipt, MaximumRoundBatch> receipts{};
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
              << " private_work_used=" << private_work_used << " busy_seconds=" << private_busy_seconds
              << " choices=" << round_state.choices->size() << " lost_own=" << round_state.lost_own
              << " lost_other=" << round_state.lost_other << '\n';
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
      if (proposal.approved && !checked.candidate && proposal.ladder < 3) {
        ++stats.quality.ladder[proposal.ladder].rejected[5];
        if (options.timings) context.census.selection->NoteRepairRefusal(proposal.choice.feature, proposal.ladder, 5);
      }
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
    if (receipt_count == MaximumRoundBatch) throw std::logic_error("Native3D repair receipts exceed round bound.");
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
  if (!removed.empty() || !added.empty()) selection.census.progress_consistent = false;
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
    const auto cached = selection.census.cells.find(row.cell);
    if (cached == selection.census.cells.end()) {
      ++stats.unmeasured_retired;
      continue;
    }
    if (checkpoint.valid) checkpoint.changed.insert(cached->second.vertices.begin(), cached->second.vertices.end());
    for (const auto& edge : cached->second.edges) retired_edges.emplace(edge, selection.census.edges.at(edge).measure);
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
    if (auto* certificates = commit_cert::active; certificates && !certificates->trial.empty()) {
      commit_cert::SortProof(certificates->trial);
      for (const auto& proposal : *round_state.repair_proposals)
        if (proposal.approved)
          RetainCommitCertificates(*certificates, *context.geometry.Parameters(), proposal.old, proposal.fresh,
                                   atlas_budget);
    }
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
    selection.census.progress_consistent = true;
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return FinishRound();
  batch_changed.clear();
  retired_edges.clear();
  return true;
}

// Publishes a single collective slot using its active cavity and failure scopes.
class SlotCommit {
  using Phase = EnginePhase;

 public:
  SlotCommit(EngineContext&, RoundState&, SlotState&, SU2NativeGeometry::AtlasBudget&,
             const SU2NativeGeometry::BoundaryCoverage& no_coverage, bool curved, size_t diagnostic_rejection_limit);
  template <class Reserve, class Observe>
  bool Run(Reserve& reserve_dependencies, Observe& observe_transaction);
  std::optional<EngineResult> finished;

 private:
  EngineContext& context;
  RoundState& round_state;
  SlotState& slot_state;
  SU2NativeGeometry::AtlasBudget& atlas_budget;
  const SU2NativeGeometry::BoundaryCoverage& no_coverage;
  bool curved;
  size_t diagnostic_rejection_limit;
  std::optional<Selection::CollapseProbeCounts> collapse_observation;

  bool FinishSlot();
  void AppendCorrectionRefusal();
  bool ClearContact();
  void VerifyReservedFootprint();
  template <class Reserve>
  bool ReserveFinal(Reserve& reserve_dependencies);
  template <class Observe>
  bool PrepareParametersAndObserve(Observe& observe_transaction);
  void RecordRejectedSlot();
  void ObserveCollapse(const std::vector<OwnedCell>& metric_rows) noexcept;
  void RouteApprovedRows();
  void ExchangeSlotRoutes();
  bool RouteSlot();
  bool Publish();
  bool ServiceReceipt();
  void AdmitSelectionUpdate();
  void ApplyChangedRows();
  void RecordAcceptedSlot();
  void RecordSplitRatio();
  void MeasureChangedCells();
  void TouchChangedSupport();
  bool UpdateSelection();
};

// Borrows trial and round payloads while the caller still owns the cavity scope.
SlotCommit::SlotCommit(EngineContext& context, RoundState& round_state, SlotState& slot_state,
                       SU2NativeGeometry::AtlasBudget& atlas_budget,
                       const SU2NativeGeometry::BoundaryCoverage& no_coverage, bool curved,
                       size_t diagnostic_rejection_limit)
    : context(context),
      round_state(round_state),
      slot_state(slot_state),
      atlas_budget(atlas_budget),
      no_coverage(no_coverage),
      curved(curved),
      diagnostic_rejection_limit(diagnostic_rejection_limit) {}

// Preserves the rejection and active scopes at the original exit site.
bool SlotCommit::FinishSlot() {
  auto& working = *context.ownership.working;
  const auto& started = *context.setup.control_prefix.started;
  const auto& started_cpu = *context.setup.control_prefix.started_cpu;
  finished.emplace(EngineControl::Finish(context, curved, working, atlas_budget, started, started_cpu));
  return false;
}

// Sequences one slot's collective publication and census update under the caller's cavity scope.
template <class Reserve, class Observe>
bool SlotCommit::Run(Reserve& reserve_dependencies, Observe& observe_transaction) {
  auto& world = context.world;
  auto& result = *context.control.result;
  auto& approved = *slot_state.approved;
  auto& chosen = *slot_state.chosen;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  AppendCorrectionRefusal();
  // Successful peer contact or reservation votes must not erase this worker's construction refusal.
  ContextValue pending_rejection_lifetime(slot_state.pending_rejection,
                                          chosen && !approved ? result.reason : std::string{});
  ContextValue surface_edit_lifetime(slot_state.surface_edit, approved && !SameSkin(old, fresh));
  if (!ClearContact() || !ReserveFinal(reserve_dependencies)) return false;
  const auto& pending_rejection = pending_rejection_lifetime.Get();
  if (chosen && !approved && !pending_rejection.empty()) result.reason = pending_rejection;
  ContextValue parameter_edits_lifetime(slot_state.parameter_edits);
  if (!PrepareParametersAndObserve(observe_transaction)) return false;
  RecordRejectedSlot();
  ContextValue removals_lifetime(slot_state.removals, world.size);
  ContextValue additions_lifetime(slot_state.additions, world.size);
  ContextValue blocked_lifetime(slot_state.blocked, world.size);
  ContextValue removed_lifetime(slot_state.removed);
  ContextValue added_lifetime(slot_state.added);
  ContextValue failures_lifetime(slot_state.failures);
  if (!RouteSlot() || !Publish() || !ServiceReceipt()) return false;
  ContextValue changed_lifetime(slot_state.changed);
  return UpdateSelection();
}

// Appends the bounded collapse-correction diagnostic before saving the refusal.
void SlotCommit::AppendCorrectionRefusal() {
  auto& result = *context.control.result;
  auto& chosen = *slot_state.chosen;
  auto& approved = *slot_state.approved;
  auto& collapse_correction_refusal = *slot_state.collapse_correction_refusal;
  if (chosen && !approved && !collapse_correction_refusal.empty())
    result.reason += " Bounded correction: " + collapse_correction_refusal;
}

// Keeps the original clearance guards and applies any defer or rejection.
bool SlotCommit::ClearContact() {
  auto& world = context.world;
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& timed_lane = *round_state.timed_lane;
  auto& choice = *round_state.choice;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& chosen = *slot_state.chosen;
  auto& approved = *slot_state.approved;
  auto& collapse_corrected = *slot_state.collapse_corrected;
  auto& private_scratch = *slot_state.private_scratch;
  auto& coordinate_only = *slot_state.coordinate_only;
  auto& imported = *slot_state.imported;
  auto& physical_edge = *slot_state.physical_edge;
  auto& surface_edit = *slot_state.surface_edit;
  if (timed_lane == 2 && surface_edit && !physical_edge) ++stats.interior_boundary_discoveries;
  if ((curved && world.maximum(surface_edit)) || world.maximum(coordinate_only && approved) ||
      world.maximum(collapse_corrected && approved)) {
    const bool prior = approved;
    bool deferred = false;
    if (!directory->Clearance(
            working, old, fresh, approved, result.reason, options.memory_ceiling,
            transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), private_scratch), &deferred,
            choice.reservation_priority, coordinate_only ? imported.coordinate_star : Feature{}))
      return FinishSlot();
    if (deferred) {
      chosen = false;
      ++stats.conflicts;
      ++stats.rejected;
      ++stats.proposal_deferred;
    } else if (prior && !approved)
      EngineTrial::GeometryTrial(context, false);
  }
  return true;
}

// Checks each approved cavity against the complete versioned imported footprint.
void SlotCommit::VerifyReservedFootprint() {
  auto& old = *slot_state.old;
  auto& dependencies = *slot_state.dependencies;
  auto& imported = *slot_state.imported;
  if (old.cells.size() != imported.cells.size() || dependencies.cells.size() != imported.cells.size())
    throw std::runtime_error("Native3D final proposal lacks complete imported dependencies.");
  std::set<Id> cells;
  for (const auto& c : old.cells)
    if (!cells.insert(c.id).second) throw std::runtime_error("Native3D final proposal repeats an old cell.");
  for (const auto& d : dependencies.cells) {
    const auto row =
        std::find_if(imported.cells.begin(), imported.cells.end(), [&](const auto& c) { return c.cell.id == d.cell; });
    if (!cells.erase(d.cell) || row == imported.cells.end() || row->version != d.version)
      throw std::runtime_error("Native3D final proposal lost a versioned old cell.");
  }
}

// Finalizes disjoint reservations inside the Communication and failure scopes.
template <class Reserve>
bool SlotCommit::ReserveFinal(Reserve& reserve_dependencies) {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& choice = *round_state.choice;
  auto& concurrent = *round_state.concurrent;
  auto& chosen = *slot_state.chosen;
  auto& approved = *slot_state.approved;
  auto& collapse_corrected = *slot_state.collapse_corrected;
  auto& private_scratch = *slot_state.private_scratch;
  auto& imported = *slot_state.imported;
  if ((concurrent || world.maximum(collapse_corrected)) && world.maximum(approved)) {
    Timer timer(stats.reservation_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
    CLocalFailure final_failure;
    // Every approved proposal claims its final old footprint together, including completed peers.
    if (approved) try {
        VerifyReservedFootprint();
      } catch (const std::exception& e) {
        final_failure.Set(2, 0, e.what());
      }
    if (!context.exchange.Vote(context, final_failure)) return FinishSlot();
    ++stats.final_reservations;
    const bool admitted = reserve_dependencies(
        imported, approved, result.reason, options.memory_ceiling,
        transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), private_scratch),
        choice.reservation_priority);
    if (approved && !admitted) {
      if (directory->PassDeferred() || result.reason == "Native 3D conflicting dependency reservation.") {
        approved = false;
        chosen = false;
        ++stats.conflicts;
        ++stats.rejected;
        ++stats.proposal_deferred;
      } else
        final_failure.Set(1, 0, "Native3D final proposal reservation failed: " + result.reason);
    }
    if (!context.exchange.Vote(context, final_failure)) return FinishSlot();
    const auto commits = TimedAllreduce(uint64_t(approved), CPassiveComm::Op::SUM, world.comm);
    stats.maximum_parallel_commits = std::max(stats.maximum_parallel_commits, commits);
  }
  return true;
}

// Exports mapped edits and invokes the existing transaction observer and vote.
template <class Observe>
bool SlotCommit::PrepareParametersAndObserve(Observe& observe_transaction) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& coarsening = *context.control.coarsening;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& point = *slot_state.point;
  auto& approved = *slot_state.approved;
  auto& repair_approved = *slot_state.repair_approved;
  auto& failure = *slot_state.failure;
  auto& parameter_edits = *slot_state.parameter_edits;
  if (approved && geometry.Mapped()) try {
      geometry.ExportParameters(old);
      geometry.ExportParameters(fresh);
      parameter_edits = BoundaryParameterState::Changes(old, fresh, geometry.Parameters()->Revision());
    } catch (const std::exception& e) {
      approved = false;
      result.reason = e.what();
    }
  if (approved && !coarsening && !repair_approved)
    for (const auto& cell : fresh.cells)
      for (auto node : cell.v)
        if (node.id == point.id) point = node;
  try {
    observe_transaction("transaction_end", approved);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (options.transaction_observer && !context.exchange.Vote(context, failure)) return FinishSlot();
  return true;
}

// Keeps the last completed rejection aligned with the private sample.
void SlotCommit::RecordRejectedSlot() {
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& last_rejection = *context.control.last_rejection;
  auto& choice = *round_state.choice;
  auto& old = *slot_state.old;
  auto& chosen = *slot_state.chosen;
  auto& approved = *slot_state.approved;
  auto& certified = *slot_state.certified;
  if (chosen && !approved) {
    // Keep the completed rejection consistent with the earlier private/budget-exit sample.
    stats.last_metric_rejection = certified;
    stats.last_rejected_feature = Feature{{choice.edge[0], choice.edge[1], 0}, 2};
    if (!old.cells.empty()) stats.last_rejected_cavity_centroid = Centroid(old.cells.front());
  }
  if (chosen && !approved)
    last_rejection =
        "edge " + std::to_string(choice.edge[0]) + "," + std::to_string(choice.edge[1]) + ": " + result.reason;
  if (chosen && !approved)
    EngineReports::RecordRejection(context, diagnostic_rejection_limit, stats.last_rejected_feature, &old, &certified,
                                   result.reason);
}

void SlotCommit::ObserveCollapse(const std::vector<OwnedCell>& metric_rows) noexcept {
  if (!context.census.selection->collapse_probe || !*context.control.coarsening) return;
  const auto& old = *slot_state.old;
  const auto& fresh = *slot_state.fresh;
  const auto removed = round_state.choice->edge[1];
  auto& probe = collapse_observation.emplace();
  probe.accepted = 1;
  probe.shared = std::any_of(slot_state.dependencies->cells.begin(), slot_state.dependencies->cells.end(),
                             [&](const auto& dep) { return dep.owner != context.world.rank; });
  probe.local = !probe.shared;
  if (std::any_of(old.surface.begin(), old.surface.end(), [&](const auto& face) {
        return std::find(face.v.begin(), face.v.end(), removed) != face.v.end();
      })) {
    try {
      const auto* state = context.geometry.Parameters();
      if (state) {
        const auto dimension = state->Geometry().MobilityDimension(old.parameters.at(removed).binding, state->Mobility());
        ++probe.surface.at(dimension);
      } else {
        probe.unclassified = 1;
        probe.incomplete = true;
      }
    } catch (...) {
      probe.unclassified = 1;
      probe.incomplete = true;
    }
  } else
    probe.volume = 1;
  const auto measure = [&](const auto& cells, const auto& tensors, auto& values) {
    values.total = cells.size();
    for (const auto& cell : cells) {
      const auto row = std::find_if(tensors.begin(), tensors.end(),
                                    [&](const auto& entry) { return entry.cell.id == cell.id; });
      if (row != tensors.end() && row->sensor_present) {
        try {
          values.bad += SU2NativeLocal3D::NodalMargin(*row) < SU2NativeLocal3D::BadMargin;
          continue;
        } catch (...) {
          probe.incomplete = true;
        }
      }
      ++values.unmeasured;
    }
  };
  measure(old.cells, slot_state.imported->cells, probe.old);
  measure(fresh.cells, metric_rows, probe.fresh);
}

// Routes verified cells to original owners using the per-slot parent layout.
void SlotCommit::RouteApprovedRows() {
  auto& coarsening = *context.control.coarsening;
  auto& choice = *round_state.choice;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& collapse_corrected = *slot_state.collapse_corrected;
  auto& coordinate_only = *slot_state.coordinate_only;
  auto& dependencies = *slot_state.dependencies;
  auto& imported = *slot_state.imported;
  auto& repair_approved = *slot_state.repair_approved;
  auto& cavity_inserted = *slot_state.cavity_inserted;
  auto& removals = *slot_state.removals;
  auto& additions = *slot_state.additions;
  std::vector<Id> parents;
  if (coordinate_only) {
    if (fresh.cells.size() != old.cells.size())
      throw std::logic_error("Native coordinate-star movement changed cell count.");
    for (const auto& cell : fresh.cells) parents.push_back(cell.id);
  } else if (repair_approved || cavity_inserted || collapse_corrected) {
    for (size_t i = 0; i < fresh.cells.size(); ++i) parents.push_back(old.cells[i % old.cells.size()].id);
  } else
    for (const auto& cell : old.cells) {
      bool keep = false;
      for (auto node : cell.v) keep = keep || node.id == choice.edge[0];
      if (!coarsening || !keep) parents.push_back(cell.id);
      if (!coarsening) parents.push_back(cell.id);
    }
  if (fresh.cells.size() != parents.size()) throw std::logic_error("Unexpected native tetrahedral edit layout.");
  std::map<Id, int> owners;
  std::map<Id, uint64_t> versions;
  for (const auto& row : dependencies.cells) {
    owners.emplace(row.cell, row.owner);
    versions.emplace(row.cell, row.version);
    removals[row.owner].push_back(row);
  }
  std::vector<OwnedCell> metric_rows;
  for (const auto& cell : fresh.cells) {
    OwnedCell row;
    row.cell = cell;
    metric_rows.push_back(row);
  }
  PublishMetrics(imported.cells, metric_rows);
  ObserveCollapse(metric_rows);
  std::map<Face, Id> physical;
  for (auto face : fresh.surface) physical.emplace(Key(face.v), face.facet);
  for (size_t i = 0; i < fresh.cells.size(); ++i) {
    OwnedCell row = metric_rows.at(i);
    if (coordinate_only) {
      const auto version = versions.at(row.cell.id);
      if (version == UINT64_MAX) throw std::overflow_error("Native coordinate-star cell version overflows.");
      row.version = version + 1;
    }
    const auto faces = Faces(row.cell);
    for (size_t f = 0; f < 4; ++f) {
      const auto found = physical.find(Key(faces[f]));
      if (found != physical.end()) row.physical[f] = {true, found->second};
    }
    additions[owners.at(parents[i])].push_back(row);
  }
}

// Exchanges all three routes under the same existing per-slot byte baseline.
void SlotCommit::ExchangeSlotRoutes() {
  auto& world = context.world;
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& ready = *context.control.ready;
  auto& imported = *slot_state.imported;
  auto& failure = *slot_state.failure;
  auto& removals = *slot_state.removals;
  auto& additions = *slot_state.additions;
  auto& blocked = *slot_state.blocked;
  auto& removed = *slot_state.removed;
  auto& added = *slot_state.added;
  auto& failures = *slot_state.failures;
  ready = context.exchange.Vote(context, failure);
  if (ready) {
    bool admitted = false;
    const auto baseline =
        transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), directory->ResidentBytes(),
                             transfer_memory::Bytes(imported.cells), MaximumFrozenFieldBytes);
    removed = world.exchange(removals, &admitted, options.memory_ceiling, baseline);
    ready = admitted;
    if (ready) {
      added = world.exchange(additions, &admitted, options.memory_ceiling, baseline);
      ready = admitted;
    }
    if (ready) {
      failures = world.exchange(blocked, &admitted, options.memory_ceiling, baseline);
      ready = admitted;
    }
    if (!ready) result.reason = "Native 3D publication routing exceeds storage budget.";
  }
}

// Keeps preparation and exchanges under one routing timer.
bool SlotCommit::RouteSlot() {
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& ready = *context.control.ready;
  auto& choice = *round_state.choice;
  auto& chosen = *slot_state.chosen;
  auto& approved = *slot_state.approved;
  auto& pass_deferred = *slot_state.pass_deferred;
  auto& dependencies = *slot_state.dependencies;
  auto& failure = *slot_state.failure;
  auto& blocked = *slot_state.blocked;
  {
    Timer timer(stats.routing_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
    if (chosen && !approved) ++stats.rejected;
    try {
      if (approved) {
        RouteApprovedRows();
      } else if (chosen && !pass_deferred) {
        std::set<int> owners;
        for (const auto& row : dependencies.cells) owners.insert(row.owner);
        for (int owner : owners) blocked[owner].push_back({choice.edge, choice.feature});
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    ExchangeSlotRoutes();
  }
  if (!ready) return FinishSlot();
  return true;
}

// Publishes the single slot and tests readiness after the publication timer.
bool SlotCommit::Publish() {
  auto& working = *context.ownership.working;
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& directory = *context.exchange.directory;
  auto& source = *context.exchange.source;
  auto& ready = *context.control.ready;
  auto& fresh = *slot_state.fresh;
  auto& approved = *slot_state.approved;
  auto& parameter_edits = *slot_state.parameter_edits;
  auto& removed = *slot_state.removed;
  auto& added = *slot_state.added;
  {
    Timer timer(stats.publication_seconds, options.timings, SU2NativeTiming3D::Operation::Communication);
    ready = directory->Publish(
        working, removed, added, true, result.reason, options.memory_ceiling,
        transfer_memory::Add(context.exchange.RealBytes(context), source.ResidentBytes(), MaximumFrozenFieldBytes),
        parameter_edits, approved ? fresh.coverage : no_coverage, geometry.AtlasReservation(), false);
  }
  if (!ready) return FinishSlot();
  return true;
}

// Services completed proposals without refreshing the age of conflicts or deferred attempts.
bool SlotCommit::ServiceReceipt() {
  auto& world = context.world;
  auto& bidirectional = context.bidirectional;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& phase = *context.setup.control_prefix.phase;
  auto& coarsening = *context.control.coarsening;
  auto& choice = *round_state.choice;
  auto& chosen = *slot_state.chosen;
  auto& approved = *slot_state.approved;
  auto& pass_deferred = *slot_state.pass_deferred;
  auto& certified = *slot_state.certified;
  auto& removed = *slot_state.removed;
  auto& added = *slot_state.added;
  EngineReports::UpdateFixedMetricCells(context, removed, added);
  if (bidirectional && phase == Phase::Construction && !coarsening) {
    const auto split_commits = TimedAllreduce(uint64_t(approved), CPassiveComm::Op::SUM, world.comm);
    stats.maximum_parallel_split_commits = std::max(stats.maximum_parallel_split_commits, split_commits);
  }
  if (!removed.empty() || !added.empty()) selection.census.progress_consistent = false;
  // A completed attempt is served once on every replica. Conflict/deferred candidates never
  // enter this receipt stream and retain their waiting age.
  stats.length_pass_deferred += pass_deferred && !approved;
  const unsigned disposition = pass_deferred && !chosen                   ? 2
                               : certified.failure == MetricFailure::None ? 4
                                                                          : 10 + unsigned(certified.failure);
  if (!context.queues.ServiceFrontier(context, Feature{{choice.edge[0], choice.edge[1], 0}, 2}, chosen || pass_deferred,
                                      approved, nullptr, 0, disposition))
    return FinishSlot();
  return true;
}

// Admits the full census workspace before any selection update.
void SlotCommit::AdmitSelectionUpdate() {
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& ready = *context.control.ready;
  auto& checkpoint = *context.control.checkpoint;
  auto& removed = *slot_state.removed;
  auto& added = *slot_state.added;
  auto& failures = *slot_state.failures;
  ready = context.exchange.Admit(
      context,
      transfer_memory::Add(
          checkpoint.valid
              ? transfer_memory::Mul(transfer_memory::Mul(4, transfer_memory::Add(removed.size(), added.size())),
                                     sizeof(Id) + 4 * sizeof(void*))
              : 0,
          transfer_memory::Mul(working.size(), context.control.SelectionBound(context, selection.track_repairs)),
          transfer_memory::Mul(transfer_memory::Mul(6, removed.size()),
                               sizeof(std::pair<const Edge, EdgeMeasure>) + 4 * sizeof(void*)),
          selection.track_repairs ? transfer_memory::Mul(transfer_memory::Mul(14, removed.size()),
                                                         sizeof(RepairFeature) + 4 * sizeof(void*))
                                  : 0,
          transfer_memory::Mul(failures.size(),
                               2 * (sizeof(std::pair<const Id, std::set<Id>>) + sizeof(Id) + 8 * sizeof(void*)))));
}

// Updates the checkpoint, retires old census rows and blocks failed supports.
void SlotCommit::ApplyChangedRows() {
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& retired_edges = *context.census.retired_edges;
  auto& checkpoint = *context.control.checkpoint;
  auto& removed = *slot_state.removed;
  auto& added = *slot_state.added;
  auto& failures = *slot_state.failures;
  auto& changed = *slot_state.changed;
  if (checkpoint.valid) {
    for (const auto& row : removed) {
      const auto found = selection.census.cells.find(row.cell);
      if (found == selection.census.cells.end())
        throw std::logic_error("Checkpoint trial lost a changed owned support row.");
      checkpoint.changed.insert(found->second.vertices.begin(), found->second.vertices.end());
    }
    for (const auto& row : added)
      for (auto node : row.cell.v) checkpoint.changed.insert(node.id);
  }
  // Retain retired scalars until all children are measured against this event's immutable source.
  for (const auto& row : removed) {
    const auto cached = selection.census.cells.find(row.cell);
    if (cached == selection.census.cells.end()) {
      ++stats.unmeasured_retired;
      continue;
    }
    for (auto key : cached->second.edges) retired_edges.emplace(key, selection.census.edges.at(key).measure);
    selection.Drop(row.cell);
  }
  selection.census.progress_consistent = true;
  for (const auto& row : added) changed.push_back(row.cell.id);
  for (auto row : failures) selection.Block(row.edge, row.feature, false);
}

// Records accepted slot work and exact split or collapse history.
void SlotCommit::RecordAcceptedSlot() {
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& wall_counters = *context.setup.wall_counters;
  auto& lane_counters = *context.setup.lane_counters;
  auto& construction_changed = *context.setup.control_prefix.construction_changed;
  auto& construction_generation_changed = *context.setup.control_prefix.construction_generation_changed;
  auto& coarsening = *context.control.coarsening;
  auto& checkpoint = *context.control.checkpoint;
  auto& standalone_coarsening = *context.control.standalone_coarsening;
  auto& choice = *round_state.choice;
  auto& lane_slots = *round_state.lane_slots;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& point = *slot_state.point;
  auto& approved = *slot_state.approved;
  auto& geometric_progress = *slot_state.geometric_progress;
  auto& repair_approved = *slot_state.repair_approved;
  auto& cavity_inserted = *slot_state.cavity_inserted;
  auto& moved = *slot_state.moved;
  auto& optimization_approved = *slot_state.optimization_approved;
  auto& repair_optimization = *slot_state.repair_optimization;
  auto& certified = *slot_state.certified;
  auto& surface_edit = *slot_state.surface_edit;
  auto& parameter_edits = *slot_state.parameter_edits;
  auto& slot = slot_state.slot;
  if (approved) {
    if (standalone_coarsening && checkpoint.valid) checkpoint.trial = true;
    construction_changed = true;
    construction_generation_changed = true;
    ++stats.accepted;
    if (collapse_observation) {
      auto& total = *context.census.selection->collapse_probe;
      const auto& slot = *collapse_observation;
      total.accepted += slot.accepted;
      total.volume += slot.volume;
      total.unclassified += slot.unclassified;
      total.shared += slot.shared;
      total.local += slot.local;
      for (size_t d = 0; d < total.surface.size(); ++d) total.surface[d] += slot.surface[d];
      total.old.total += slot.old.total;
      total.old.bad += slot.old.bad;
      total.old.unmeasured += slot.old.unmeasured;
      total.fresh.total += slot.fresh.total;
      total.fresh.bad += slot.fresh.bad;
      total.fresh.unmeasured += slot.fresh.unmeasured;
      total.incomplete |= slot.incomplete;
    }
    wall_counters.Accepted();
    if (!coarsening) wall_counters.ObserveSplit(choice.geometric);
    if (options.timings && lane_slots[slot].required_primary_split)
      wall_counters.AcceptRequiredSplit(old, fresh, choice.edge, point.id);
    if (options.timings && lane_slots[slot].checked_primary_split) wall_counters.AcceptCheckedSplit();
    if (options.timings)
      lane_counters.Commit(false, lane_slots[slot], !surface_edit, parameter_edits.empty(),
                           fresh.coverage.faces.empty() && fresh.coverage.traces.empty());
    if (coarsening)
      ++stats.length_collapses;
    else
      ++stats.length_splits;
    if (optimization_approved) {
      stats.optimization_moves += repair_optimization.moves;
      stats.optimization_insertions += repair_optimization.insertions;
      stats.optimization_collapses += repair_optimization.collapses;
      stats.optimization_swaps += repair_optimization.swaps;
    }
    stats.geometry_progress += geometric_progress;
    stats.size_progress += certified.size_progress;
    stats.quality_progress += certified.quality_progress;
    stats.cell_removals += coarsening && !repair_approved && certified.cell_removal;
    if (cavity_inserted)
      ++stats.inserted;
    else if (repair_approved) {
      if (moved)
        ++stats.moved;
      else
        ++stats.reconnected;
    } else if (coarsening)
      result.collapses.push_back(choice.edge);
    else
      result.splits.push_back({choice.edge, point});
  }
}

// Updates the child-parent ratio when both child measures are available.
void SlotCommit::RecordSplitRatio() {
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& coarsening = *context.control.coarsening;
  auto& choice = *round_state.choice;
  auto& point = *slot_state.point;
  auto& chosen = *slot_state.chosen;
  auto& approved = *slot_state.approved;
  auto& repair_approved = *slot_state.repair_approved;
  stats.maximum_owned = std::max(stats.maximum_owned, working.size());
  if (chosen && approved && !coarsening && !repair_approved && choice.measure.length > 0) {
    Edge a{choice.edge[0], point.id}, b{choice.edge[1], point.id};
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    const auto first = campaign_edges.find(a), second = campaign_edges.find(b);
    if (first != campaign_edges.end() && second != campaign_edges.end()) {
      ++stats.child_ratio_measured;
      stats.maximum_child_parent_ratio =
          std::max(stats.maximum_child_parent_ratio,
                   double(std::max(first->second.length, second->second.length) / choice.measure.length));
    }
  }
}

// Votes on the slot update, then measures its fresh cells.
void SlotCommit::MeasureChangedCells() {
  auto& stats = context.control.result->stats;
  auto& ready = *context.control.ready;
  auto& failure = *slot_state.failure;
  auto& changed = *slot_state.changed;
  if (ready) ready = context.exchange.Vote(context, failure);
  if (ready) ready = context.census.Measure(context, changed, false, &stats.update_seconds);
}

// Reactivates changed supports and retires conflicts in the current pass.
void SlotCommit::TouchChangedSupport() {
  auto& bidirectional = context.bidirectional;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& phase = *context.setup.control_prefix.phase;
  auto& retired_edges = *context.census.retired_edges;
  auto& directory = *context.exchange.directory;
  auto& construction_claims = *context.exchange.construction_claims;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  auto& ready = *context.control.ready;
  auto& failure = *slot_state.failure;
  if (ready) {
    try {
      selection.FinishRepairs();
      for (const auto& support : directory->ChangedRepairSupports()) selection.TouchRepairSupport(support);
      for (Id vertex : directory->ChangedVertices()) selection.Touch(vertex);
      // Retire known conflicts in this pass without another global reservation per edge.
      // Unmeasured/remote support and expanded cavities remain guarded by Directory claims.
      if (bidirectional && (phase == Phase::Construction || metric_cleanup) && construction_claims)
        for (Id vertex : directory->ChangedVertices())
          stats.length_pass_deferred += selection.ServiceClaimedVertex(vertex);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    ready = context.exchange.Vote(context, failure);
  }
  retired_edges.clear();
}

// Retains the update timer across admission, census rebuild and support refresh.
bool SlotCommit::UpdateSelection() {
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& ready = *context.control.ready;
  auto& failure = *slot_state.failure;
  {
    Timer timer(stats.update_seconds, options.timings, SU2NativeTiming3D::Operation::Topology);
    AdmitSelectionUpdate();
    if (ready) try {
        ApplyChangedRows();
        RecordAcceptedSlot();
        RecordSplitRatio();
        if (auto* certificates = commit_cert::active;
            certificates && *slot_state.approved && !certificates->trial.empty()) {
          commit_cert::SortProof(certificates->trial);
          RetainCommitCertificates(*certificates, *context.geometry.Parameters(), *slot_state.old,
                                   *slot_state.fresh, atlas_budget);
        }
      } catch (const std::exception& e) {
        failure.Set(2, 0, e.what());
      }
    MeasureChangedCells();
    TouchChangedSupport();
  }
  if (!ready) return FinishSlot();
  return true;
}
