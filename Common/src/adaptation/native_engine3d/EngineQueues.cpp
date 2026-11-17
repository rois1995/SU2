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
