// Owns the original setup RAII in declaration order; never performs collectives or decisions.
// Included privately after its dependencies in CNativeEngine3D.cpp.
#pragma once

struct EngineSetup {
  RestoreAdmission admission;
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

  EngineSetup();
  ~EngineSetup();
  EngineSetup(const EngineSetup&) = delete;
  EngineSetup& operator=(const EngineSetup&) = delete;
  EngineSetup(EngineSetup&&) = delete;
  EngineSetup& operator=(EngineSetup&&) = delete;
};
