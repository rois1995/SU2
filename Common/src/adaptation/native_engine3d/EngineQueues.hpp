// Chooses and services candidates through canonical queue state; preserves container and protocol order.
#pragma once

struct EngineContext;

struct EngineQueues {
  std::optional<std::map<Id, std::set<Id>>> inactive_blocked_edges;
  std::optional<size_t> inactive_blocked_links;
  std::optional<std::array<bool, 2>> length_blocks_dirty;

  void LengthDirection(EngineContext& context, bool collapse);
  bool Coalesce(EngineContext&, std::vector<Choice>& choices, bool curved);
  bool ServiceFrontier(EngineContext&, Feature feature, bool active, bool accepted,
                       const std::array<RepairReceipt, MaximumCandidateBatch>* extra = nullptr, size_t count = 0,
                       unsigned disposition = 4);
  bool ServiceReceipts(EngineContext&, std::vector<RepairReceipt>& receipts, bool certify_support,
                       CLocalFailure& failure);

  EngineQueues();
  ~EngineQueues();
  EngineQueues(const EngineQueues&) = delete;
  EngineQueues& operator=(const EngineQueues&) = delete;
  EngineQueues(EngineQueues&&) = delete;
  EngineQueues& operator=(EngineQueues&&) = delete;
};
