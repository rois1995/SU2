// Owns Working and ownership-boundary markers; original Adapt scopes activate values.
#pragma once

struct EngineContext;

struct EngineOwnership {
  std::optional<Owned> working;
  std::optional<std::array<bool, 2>> ownership_evaluated;
  std::optional<uint64_t> quality_ownership_round;

  bool WithinTime(EngineContext& context);
  bool PhaseOwnership(EngineContext& context, bool quality, SU2NativeGeometry::AtlasBudget& atlas_budget);
  bool EstimateOwnership(EngineContext& context, bool quality, SU2NativeGeometry::AtlasBudget& atlas_budget,
                         uint64_t global_cells, double transition_started);
  void FillOwnershipCosts(EngineContext& context, bool quality, const std::map<Id, uint64_t>& complete_stars,
                          CLocalFailure& failure, std::vector<uint32_t>& weights, std::vector<uint32_t>& geometric,
                          std::vector<uint32_t>& quality_weights, std::vector<double>& costs,
                          std::vector<double>& geometry_costs, std::vector<double>& quality_debts, double& local_demand,
                          double& local_geometry, double& local_quality_debt);
  void AccumulateOwnershipCell(EngineContext& context, bool quality, const Owned::value_type& owned,
                               const std::map<Id, uint64_t>& complete_stars, std::vector<double>& costs,
                               std::vector<double>& geometry_costs, std::vector<double>& quality_debts,
                               double& local_demand, double& local_geometry, double& local_quality_debt);
  double OwnershipGeometryCost(EngineContext& context, bool quality, const Owned::value_type& owned, double demand);
  void NormalizeOwnershipWeights(EngineContext& context, bool quality, std::vector<uint32_t>& weights,
                                 std::vector<uint32_t>& geometric, std::vector<uint32_t>& quality_weights,
                                 std::vector<double>& costs, std::vector<double>& geometry_costs,
                                 std::vector<double>& quality_debts);
  bool ShouldMigrate(EngineContext& context, bool quality, uint64_t global_cells, double local_demand,
                     double local_geometry, double local_quality_debt);
  bool MigrateOwnership(EngineContext& context, bool quality, SU2NativeGeometry::AtlasBudget& atlas_budget,
                        double transition_started, size_t scratch, CLocalFailure& failure,
                        std::vector<uint32_t>& weights, std::vector<uint32_t>& geometric,
                        std::vector<uint32_t>& quality_weights);
  bool MigrateCertificates(EngineContext& context, bool quality, SU2NativeGeometry::AtlasBudget& atlas_budget,
                           double transition_started, size_t scratch, CLocalFailure& failure, size_t baseline,
                           Owned& pending, std::vector<Id>& pending_initial,
                           std::unique_ptr<Directory>& pending_directory, Selection& pending_selection,
                           PartitionStats& partition);
  bool RestoreOwnership(EngineContext& context, bool quality, SU2NativeGeometry::AtlasBudget& atlas_budget,
                        double transition_started, CLocalFailure& failure, Owned& pending,
                        std::vector<Id>& pending_initial, std::unique_ptr<Directory>& pending_directory,
                        Selection& pending_selection, PartitionStats& partition,
                        std::vector<std::vector<Selection::CachedRow>>& outgoing,
                        const std::vector<Selection::CachedRow>& records, size_t pending_bytes);
  bool FinalizeOwnership(EngineContext& context, bool quality, SU2NativeGeometry::AtlasBudget& atlas_budget,
                         double transition_started, Owned& pending, std::vector<Id>& pending_initial,
                         std::unique_ptr<Directory>& pending_directory, Selection& pending_selection,
                         PartitionStats& partition, std::vector<std::vector<Selection::CachedRow>>& outgoing,
                         const std::vector<Selection::CachedRow>& records, size_t pending_bytes);

  EngineOwnership();
  ~EngineOwnership();
  EngineOwnership(const EngineOwnership&) = delete;
  EngineOwnership& operator=(const EngineOwnership&) = delete;
  EngineOwnership(EngineOwnership&&) = delete;
  EngineOwnership& operator=(EngineOwnership&&) = delete;
};
