// Owns setup RAII and executes the original setup stages using borrowed canonical state.
// Included privately after its dependencies in CNativeEngine3D.cpp.
#pragma once
#include "EngineContext.hpp"

struct EngineSetup {
  RestoreAdmission admission;
  EngineControlPrefix control_prefix;
  std::optional<SU2NativeTiming3D::Session> timer_session;
  std::optional<SU2NativeD9::Root> d9_profile;
  std::optional<wall_diagnostics::Counters> wall_counters;
  std::optional<lane_census::Counters> lane_counters;
  std::optional<std::shared_ptr<BoundaryParameterState>> private_parameters;
  std::optional<std::shared_ptr<const PlanarReference>> private_geometry;
  std::optional<SU2NativeGeometry::AtlasBudget> local_atlas_budget;
  std::optional<RestoreBudgetWindow> restore_budget;
  std::optional<std::unique_ptr<PlanarReference>> atlas_geometry;
  std::optional<std::shared_ptr<void>> target_geometry_lease;

  static void CheckInitialNumericPolicies(EngineContext&, bool, SU2NativeGeometry::AtlasBudget&, CLocalFailure&);
  static void CheckInitialWallPolicies(EngineContext&, CLocalFailure&);
  static void ExchangeInitialWallPolicies(EngineContext&, CLocalFailure&, bool&);
  static void PopulateInitialStateAndBuildIndexes(EngineContext&, bool, CLocalFailure&, bool&, Id&, Id&);
  template <class Finish>
  static std::optional<EngineResult> RefreshWorkingSurface(EngineContext&, SU2NativeGeometry::AtlasBudget&, Finish&);
  template <class Finish>
  static std::optional<EngineResult> PublishRefreshedSurface(EngineContext&, SU2NativeGeometry::AtlasBudget&,
                                                             SU2NativeGeometry::BoundaryCoverage&, CLocalFailure&,
                                                             size_t, Finish&);
  static bool CheckRegionApplicability(EngineContext&, bool&, bool);
  template <class Finish>
  static std::optional<EngineResult> ConstructRegion(EngineContext&, uint64_t, CLocalFailure&, Finish&);
  template <class Finish>
  static std::optional<EngineResult> GatherRegion(EngineContext&, uint64_t, size_t, size_t, CLocalFailure&, Finish&);
  template <class Rows, class Finish>
  static std::optional<EngineResult> PrepareRegionField(EngineContext&, uint64_t, size_t, size_t, CLocalFailure&, bool&,
                                                        Rows&, std::vector<DonorRegion>&, Patch&, Patch&,
                                                        std::vector<OwnedCell>&, std::vector<BoundaryParameterEdit>&,
                                                        bool&, Finish&);
  template <class Finish>
  static std::optional<EngineResult> RunAndFinalizeRegion(EngineContext&, RegionFieldService&, RegionReport&,
                                                          CLocalFailure&, size_t, bool&, Patch&,
                                                          std::vector<OwnedCell>&, std::vector<BoundaryParameterEdit>&,
                                                          Finish&);
  template <class Finish>
  static std::optional<EngineResult> FinalizeRegionTail(EngineContext&, size_t, bool&, CLocalFailure&, Patch&,
                                                        std::vector<OwnedCell>&, std::vector<BoundaryParameterEdit>&,
                                                        Finish&);
  template <class Finish>
  static std::optional<EngineResult> PublishRegionResult(EngineContext&, size_t, bool&, CLocalFailure&, Patch&,
                                                         std::vector<OwnedCell>&, std::vector<BoundaryParameterEdit>&,
                                                         bool&, Finish&);
  template <class Rows>
  static void EvaluateRegionTarget(EngineContext&, size_t, const RegionMetric&, Rows&, Patch&, Patch&, RegionReport&,
                                   bool&, std::vector<BoundaryParameterEdit>&, std::vector<OwnedCell>&);
  static void PrepareRegionSkin(const PlanarReference&, const Patch&, std::map<Id, Point>&, std::vector<Facet>&);
  static void ConstructRegionVolume(EngineContext&, size_t, Patch&, const std::vector<Facet>&, const RegionMetric&,
                                    Patch&, RegionReport&, bool&);
  template <class Rows>
  static void RecordRegionResult(EngineContext&, Patch&, Patch&, Rows&, RegionReport&, bool, bool, bool, bool&,
                                 std::vector<BoundaryParameterEdit>&, std::vector<OwnedCell>&);
  static bool PlanFirstRows(EngineContext& context, std::set<Id>& pending, CLocalFailure& failure,
                            const size_t& layerScratch, std::vector<Feature>& features, std::vector<Id>& queued,
                            DependencyBatch& dependencies, ImportedBatch& imported, ReservationBatch& reserved);

  static void PrepareFirstRowPublication(EngineContext& context, CLocalFailure& failure, std::vector<Patch>& proposals,
                                         ReservationBatch& reserved, ImportedBatch& imported,
                                         DependencyBatch& dependencies, std::vector<std::vector<Dependency>>& toRemove,
                                         std::vector<std::vector<OwnedCell>>& toAdd, size_t& published,
                                         std::vector<BoundaryParameterEdit>& layer_parameter_edits,
                                         SU2NativeGeometry::BoundaryCoverage& layer_coverage);

  template <class NeedsLayer>
  static bool PublishFirstRows(EngineContext& context, CLocalFailure& failure, std::set<Id>& pending,
                               const size_t& layerScratch, uint64_t& epoch, std::vector<Patch>& proposals,
                               ReservationBatch& reserved, ImportedBatch& imported, DependencyBatch& dependencies,
                               NeedsLayer& needs_layer);

  template <class JointCandidate>
  static bool ImportFirstRowDonors(EngineContext& context, CLocalFailure& failure, const size_t& layerScratch,
                                   size_t at, const size_t& slots, size_t& group, size_t& span, DonorPatch& donors,
                                   ImportedBatch& imported, ReservationBatch& reserved,
                                   JointCandidate& joint_candidate);

  static void FirstRowBackgrounds(EngineContext& context, const Patch& old, std::vector<Point>& backgrounds,
                                  std::map<Id, Point>& coordinates);

  static void ScoreFirstRowBackgrounds(EngineContext& context, const Patch& old, Patch& fresh, Id node, Id cell,
                                       std::unique_ptr<FrozenField>& target, std::vector<Point>& backgrounds,
                                       long double& bestLength, std::map<Edge, EdgeMeasure>& reuse, bool& approved);

  static void ConstructFirstRowJoint(EngineContext& context, const Patch& old, Patch& fresh, Id node, Id cell,
                                     std::unique_ptr<FrozenField>& target, bool& approved);

  template <class JointCandidate>
  static void ConstructFirstRowCandidate(EngineContext& context, CLocalFailure& failure, uint64_t& epoch, size_t k,
                                         ImportedBatch& imported, std::vector<Patch>& proposals,
                                         std::unique_ptr<FrozenField>& target, JointCandidate& joint_candidate,
                                         bool& rejected);

  template <class TimedOut, class JointCandidate>
  static bool ConstructFirstRowGroup(EngineContext& context, CLocalFailure& failure, const size_t& layerScratch,
                                     uint64_t& epoch, const size_t& slots, size_t& at, size_t& span,
                                     const bool& ranked_background, ImportedBatch& imported, ReservationBatch& reserved,
                                     std::vector<Id>& queued, std::vector<Patch>& proposals, TimedOut& timed_out,
                                     JointCandidate& joint_candidate);

  template <class TimedOut, class JointCandidate>
  static bool ConstructFirstRows(EngineContext& context, CLocalFailure& failure, const size_t& layerScratch,
                                 uint64_t& epoch, const size_t& slots, size_t& span, const bool& ranked_background,
                                 ImportedBatch& imported, ReservationBatch& reserved, std::vector<Id>& queued,
                                 std::vector<Patch>& proposals, TimedOut& timed_out, JointCandidate& joint_candidate);

  template <class NeedsLayer, class Finish>
  static std::optional<EngineResult> IterateFirstRows(EngineContext& context, CLocalFailure& failure,
                                                      std::set<Id>& pending, const size_t& layerScratch,
                                                      uint64_t& epoch, NeedsLayer& needs_layer, Finish& finish);

  template <class Finish>
  static std::optional<EngineResult> InitializeFirstRows(EngineContext& context, Finish& finish);

  static void BeginConstructionPass(EngineContext&);
  static bool SeedFrontier(EngineContext&, CLocalFailure&);
  EngineSetup();
  ~EngineSetup();
  EngineSetup(const EngineSetup&) = delete;
  EngineSetup& operator=(const EngineSetup&) = delete;
  EngineSetup(EngineSetup&&) = delete;
  EngineSetup& operator=(EngineSetup&&) = delete;
};
