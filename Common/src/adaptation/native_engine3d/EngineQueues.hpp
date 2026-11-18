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
                       const std::array<RepairReceipt, MaximumRoundBatch>* extra = nullptr, size_t count = 0,
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

struct RoundState;

// Borrows one round's active slots and leaves acquisition results in their existing storage.
class RoundAcquisition {
 public:
  RoundAcquisition(EngineContext&, RoundState&, SU2NativeGeometry::AtlasBudget&, bool curved,
                   size_t diagnostic_rejection_limit);
  bool Acquire();
  void Refuse(const Choice&, const Feature&, const std::string& why);
  std::optional<EngineResult> finished;

 private:
  EngineContext& context;
  RoundState& round_state;
  SU2NativeGeometry::AtlasBudget& atlas_budget;
  bool curved;
  size_t diagnostic_rejection_limit;

  bool FinishRound();
  bool PlanChoices();
  bool AcquireBatch();
  bool AdmitBatchStorage();
  bool SetRepairSlots(CLocalFailure&, size_t&);
  bool SelectBatch(CLocalFailure&, size_t repair_slots);
  bool AdmitBatchWork();
  bool AcquireBatchDependencies(CLocalFailure&);
  bool FetchBatch(CLocalFailure&, std::vector<Feature>&, size_t& retained_cells);
  bool ExpandRepairSupport(CLocalFailure&, size_t retained_cells);
  void SelectRepairSupport(std::vector<Feature>&, std::vector<std::pair<size_t, size_t>>&);
  void MergeRepairSupport(const DependencyBatch&, const std::vector<std::pair<size_t, size_t>>&, bool& expanded);
  bool ServiceFetchRefusals();
  bool ReserveBatch(CLocalFailure&);
  void CountReservationLosses();
  bool AcquireSingle();
};
