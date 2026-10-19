/*!
 * \file CNewtonEquationSet.cpp
 * \brief Grouped state installation and deterministic residual refresh for Newton-Krylov.
 * \author SU2 Contributors
 */
#include "../../include/integration/CNewtonEquationSet.hpp"
#include "../../include/variables/CFlowVariable.hpp"

namespace {
constexpr unsigned short runtimes[] = {RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS, RUNTIME_TRANS_SYS, RUNTIME_SPECIES_SYS};

/*! \brief Retain the native preprocessing cadence for flow controllers with external feedback. */
bool HasFlowFeedback(const CConfig& config) {
  return config.GetnMarker_EngineInflow() || config.GetnMarker_EngineExhaust() ||
         config.GetnMarker_ActDiskInlet() || config.GetnMarker_ActDiskOutlet();
}

/*! \brief Boundaries and first-order auxiliary sources only consume the freshly computed temperature/velocity gradients. */
bool CanReuseBoundaryGradients(const CConfig& config) {
  /*--- Flow MUSCL fluxes are omitted here; these boundaries use nodal characteristic states. ---*/
  if (config.GetMUSCL_Turb() || config.GetWall_Functions() ||
      config.GetKind_Species_Model() != SPECIES_MODEL::NONE ||
      config.GetKind_HybridRANSLES() != NO_HYBRIDRANSLES || config.GetKind_RoeLowDiss() != NO_ROELOWDISS ||
      config.GetKind_Streamwise_Periodic() != ENUM_STREAMWISE_PERIODIC::NONE ||
      (config.GetKind_Turb_Model() != TURB_MODEL::SA && config.GetKind_Turb_Model() != TURB_MODEL::SST)) return false;
  for (auto marker = 0u; marker < config.GetnMarker_All(); ++marker) {
    switch (config.GetMarker_All_KindBC(marker)) {
      case FAR_FIELD: case HEAT_FLUX: case ISOTHERMAL: case HEAT_TRANSFER:
      case EULER_WALL: case SYMMETRY_PLANE: case PERIODIC_BOUNDARY: case SEND_RECEIVE: break;
      default: return false;
    }
  }
  return true;
}

/*! \brief SA changes transport coefficients, but not compressible thermodynamic gradient inputs. */
bool CanReuseFixedFlow(const CConfig& config) {
  if (config.GetKind_Regime() != ENUM_REGIME::COMPRESSIBLE || config.GetKind_Turb_Model() != TURB_MODEL::SA ||
      config.GetKind_Trans_Model() != TURB_TRANS_MODEL::NONE ||
      config.GetKind_Species_Model() != SPECIES_MODEL::NONE || config.GetWall_Functions() ||
      config.GetDynamic_Grid() || config.GetTime_Domain() || config.GetTime_Marching() != TIME_MARCHING::STEADY ||
      config.GetFixed_CL_Mode() || HasFlowFeedback(config) || config.GetPyCustomSource() || config.GetBounded_Scalar() ||
      config.GetKind_HybridRANSLES() != NO_HYBRIDRANSLES || config.GetKind_RoeLowDiss() != NO_ROELOWDISS ||
      config.GetKind_Streamwise_Periodic() != ENUM_STREAMWISE_PERIODIC::NONE) return false;
  for (auto marker = 0u; marker < config.GetnMarker_All(); ++marker) {
    switch (config.GetMarker_All_KindBC(marker)) {
      case FAR_FIELD: case HEAT_FLUX: case EULER_WALL: case SYMMETRY_PLANE:
      case PERIODIC_BOUNDARY: case SEND_RECEIVE: break;
      default: return false;
    }
  }
  return true;
}

/*! \brief Restore shared equation settings and periodic assembly flags after every evaluation or early return. */
class EquationSettingsGuard {
  CConfig& config;
  CConfig::EquationSettings settings;
  su2double relaxation;
  CSolver** solvers;
  std::array<bool, 4> implicitPeriodic{};

 public:
  EquationSettingsGuard(CConfig& c, CSolver** s)
      : config(c), settings(c.GetEquationSettings()), relaxation(c.GetNewtonKrylovRelaxation()), solvers(s) {
    if (config.GetnMarker_Periodic())
      for (auto i = 0u; i < 4; ++i) {
        const auto* solver = solvers[config.GetContainerPosition(runtimes[i])];
        implicitPeriodic[i] = solver && solver->GetImplicitPeriodic();
      }
    SU2_OMP_BARRIER
    if (config.GetnMarker_Periodic()) {
      /*--- Retain all periodic rows for the explicit consistency equations, including explicit-flow probes. ---*/
      SU2_OMP_SAFE_GLOBAL_ACCESS(
          for (auto runtime : runtimes)
            if (auto* solver = solvers[config.GetContainerPosition(runtime)]) solver->SetImplicitPeriodic(true);)
    }
  }
  ~EquationSettingsGuard() {
    SU2_OMP_SAFE_GLOBAL_ACCESS(
        config.SetEquationSettings(settings); config.SetNewtonKrylovRelaxation(relaxation);
        if (config.GetnMarker_Periodic())
          for (auto i = 0u; i < 4; ++i)
            if (auto* solver = solvers[config.GetContainerPosition(runtimes[i])])
              solver->SetImplicitPeriodic(implicitPeriodic[i]);)
  }
};
}  // namespace

CNewtonEquationSet::CNewtonEquationSet(CGeometry* g, CSolver** s, CNumerics*** n, CConfig* c,
                                       const std::vector<unsigned short>& selected,
                                       std::shared_ptr<ResidualWorkspace> work)
    : geometry(g), solvers(s), numerics(n), config(c),
      workspace(work ? std::move(work) : std::make_shared<ResidualWorkspace>()) {
  if (!geometry || !solvers || !solvers[FLOW_SOL] || !numerics || !config)
    throw std::invalid_argument("Newton equation sets need geometry, flow, numerics and configuration");
  density.resize(g->GetnPoint());
  if (config->GetNEMOProblem())
    throw std::invalid_argument("NEMO is excluded from the common Newton equation interface");
  /*--- Actuator controllers mutate thrust history during preprocessing; it is not a trial-state dependency. ---*/
  if (config->GetnMarker_ActDiskInlet() || config->GetnMarker_ActDiskOutlet())
    throw std::invalid_argument("Common Newton currently excludes actuator-disk feedback");
  const auto flowEntries = g->GetnPoint() * s[FLOW_SOL]->GetnVar();
  if (!workspace->trialFlow.empty() && workspace->trialFlow.size() != flowEntries)
    throw std::invalid_argument("Shared Newton residual workspace has a different flow layout");
  workspace->trialFlow.resize(flowEntries);
  if (auto* flow = dynamic_cast<CFlowVariable*>(s[FLOW_SOL]->GetNodes())) {
    reconstructionHistory = &flow->NonPhysicalEdgeCounter;
    acceptedReconstruction.resize(reconstructionHistory->size());
    nextReconstruction.resize(reconstructionHistory->size());
  }
  const auto edges = reconstructionHistory ? reconstructionHistory->size() : 0;
  if (!workspace->savedReconstruction.empty() && workspace->savedReconstruction.size() != edges)
    throw std::invalid_argument("Shared Newton residual workspace has a different reconstruction layout");
  workspace->savedReconstruction.resize(edges);
  if (selected.empty()) throw std::invalid_argument("A Newton equation group cannot be empty");
  for (auto runtime : selected) {
    if (std::find(std::begin(runtimes), std::end(runtimes), runtime) == std::end(runtimes))
      throw std::invalid_argument("Unsupported Newton equation system");
    const auto slot = config->GetContainerPosition(runtime);
    if (!solvers[slot]) throw std::invalid_argument("Selected Newton equation system is unavailable");
    if (std::any_of(blocks.begin(), blocks.end(), [=](const Block& b) { return b.slot == slot; }))
      throw std::invalid_argument("Duplicate equation in Newton group");
    const auto count = solvers[slot]->GetnVar();
    if (count > std::numeric_limits<unsigned short>::max() - variables)
      throw std::invalid_argument("Newton group exceeds the vector variable-count range");
    blocks.push_back({runtime, slot, variables, count, solvers[slot]});
    variables += count;
  }
  for (auto runtime : runtimes) {
    const auto slot = config->GetContainerPosition(runtime);
    auto* solver = solvers[slot];
    if (!solver) continue;
    const auto count = solver->GetnVar();
    solver->Jacobian.TrackPrescribedRows(geometry->GetnPoint(), count);
    const auto entries = geometry->GetnPoint() * count;
    auto& scratch = workspace->systems[slot];
    if (!scratch.evaluationHistory.empty() && scratch.evaluationHistory.size() != entries)
      throw std::invalid_argument("Shared Newton residual workspace has a different equation layout");
    scratch.evaluationHistory.resize(entries);
    scratch.boundarySolution.resize(entries);
    scratch.boundaryInput.resize(entries);
    /*--- Accepted states and rollback metadata outlive evaluations and remain private to each group. ---*/
    snapshots.push_back({solver, slot, count, std::vector<su2double>(entries), std::vector<su2double>(entries),
                         std::vector<su2double>(geometry->GetnPoint()), std::vector<unsigned char>(entries), &scratch});
  }
}

void CNewtonEquationSet::Capture() {
  assert(!reuseFixedFlow);
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p) density[p] = solvers[FLOW_SOL]->GetNodes()->GetDensity(p);
  END_SU2_OMP_FOR
  for (auto& snapshot : snapshots) {
    const auto* nodes = snapshot.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
      snapshot.relaxation[p] = nodes->GetUnderRelaxation(p);
      for (auto v = 0u; v < snapshot.variables; ++v) {
        const auto i = p * snapshot.variables + v;
        snapshot.solution[i] = nodes->GetSolution(p, v);
        snapshot.oldSolution[i] = nodes->GetSolution_Old(p, v);
        snapshot.prescribed[i] = snapshot.solver->Jacobian.IsPrescribedRow(p, v);
      }
    }
    END_SU2_OMP_FOR
  }
  SU2_OMP_FOR_STAT(1024)
  for (auto e = 0ul; e < acceptedReconstruction.size(); ++e)
    acceptedReconstruction[e] = (*reconstructionHistory)[e];
  END_SU2_OMP_FOR
  SU2_OMP_SAFE_GLOBAL_ACCESS(captured = true;)
}

bool CNewtonEquationSet::FeasibleProbes(const CSysVector<passivedouble>& direction,
                                       const CSysVector<passivedouble>& scale, double step,
                                       const std::vector<unsigned char>& free, CSysVector<passivedouble>& a,
                                       CSysVector<passivedouble>& b, bool& shifted) {
  assert(captured && step != 0 && free.size() == direction.GetLocSize());
  const bool compressible = config->GetKind_Regime() == ENUM_REGIME::COMPRESSIBLE;
  auto limits = [&](const Block& block, unsigned short v) {
    auto bounds = block.solver->GetSolutionBounds(v);
    if (block.slot == SPECIES_SOL && config->GetKind_FluidModel() == FLUID_MIXTURE) {
      bounds.first = std::max(0.0, bounds.first);
      bounds.second = std::min(1.0, bounds.second);
    }
    if (block.slot == FLOW_SOL && compressible && v == 0) bounds.first = 0;
    return bounds;
  };
  auto inside = [&](const Block& block, unsigned short v, double value) {
    const auto bounds = limits(block, v);
    return Armijo::IsFinite(value) && value >= bounds.first && value <= bounds.second &&
           (!(block.slot == FLOW_SOL && compressible && v == 0) || value > 0);
  };
  auto snapshot = [&](const Block& block) -> const Snapshot& {
    return *std::find_if(snapshots.begin(), snapshots.end(),
                         [&](const Snapshot& s) { return s.solver == block.solver; });
  };
  SU2_OMP_SAFE_GLOBAL_ACCESS(invalidCount = 0;)
  double local = 0;
  for (const auto& block : blocks) {
    const auto& base = snapshot(block).solution;
    const bool mixture = block.slot == SPECIES_SOL && config->GetKind_FluidModel() == FLUID_MIXTURE;
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
      double sum = 0;
      for (auto v = 0u; v < block.variables; ++v) {
        const auto j = block.offset + v;
        const bool active = free[p * variables + j];
        const double value = SU2_TYPE::GetValue(base[p * block.variables + v]) +
                             (active ? step * direction(p, j) * scale(p, j) /
                                           SU2_TYPE::GetValue(MassFactor(block, p)) : 0);
        if (active && !inside(block, v, value)) local = 1;
        if (mixture) sum += value;
      }
      if (mixture && sum > 1) local = 1;
    }
    END_SU2_OMP_FOR
  }
  atomicMax(local, invalidCount);
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    double global = 0;
    SU2_MPI::Allreduce(&invalidCount, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    invalidCount = global;
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  shifted = invalidCount != 0;
  a = direction;
  if (!shifted) {
    SU2_OMP_BARRIER
    return true;
  }
  SU2_OMP_SAFE_GLOBAL_ACCESS(
      if (b.GetLocSize() != direction.GetLocSize())
        b.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), variables, 0.0);
      invalidCount = 0;)
  b = passivedouble(0);
  SU2_OMP_BARRIER
  /*--- Shift both probes inward, keeping a-b equal to the requested direction rather than clipping it. ---*/
  local = 0;
  for (const auto& block : blocks) {
    const auto& base = snapshot(block).solution;
    const bool mixture = block.slot == SPECIES_SOL && config->GetKind_FluidModel() == FLUID_MIXTURE;
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
      const double mass = SU2_TYPE::GetValue(MassFactor(block, p));
      for (auto v = 0u; v < block.variables; ++v) {
        const auto j = block.offset + v;
        if (!free[p * variables + j]) {
          a(p, j) = 0;
          continue;
        }
        const double value = SU2_TYPE::GetValue(base[p * block.variables + v]);
        const double change = step * direction(p, j) * scale(p, j) / mass;
        if (!inside(block, v, value + change)) {
          a(p, j) = 0;
          b(p, j) = -direction(p, j);
          if (!inside(block, v, value - change)) local = 1;
        }
      }
      if (!mixture) continue;
      auto value = [&](const CSysVector<passivedouble>& probe, unsigned short v) {
        return SU2_TYPE::GetValue(base[p * block.variables + v]) +
               step * probe(p, block.offset + v) * scale(p, block.offset + v) / mass;
      };
      double sumA = 0, sumB = 0;
      for (auto v = 0u; v < block.variables; ++v) {
        sumA += value(a, v);
        sumB += value(b, v);
      }
      double excess = std::max(sumA, sumB) - 1;
      if (excess > 0) {
        /*--- A common inward shift preserves a-b while leaving room for the implicit last species. ---*/
        excess += 32 * std::numeric_limits<double>::epsilon() * block.variables;
        for (auto v = 0u; v < block.variables && excess > 0; ++v) {
          const auto j = block.offset + v;
          if (!free[p * variables + j]) continue;
          const double capacity = 0.5 * std::max(0.0, std::min(value(a, v), value(b, v)) - limits(block, v).first);
          const double inward = std::min(excess, capacity);
          const double correction = inward * mass / (step * scale(p, j));
          a(p, j) -= correction;
          b(p, j) -= correction;
          excess -= inward;
        }
        if (excess > 0) local = 1;
      }
    }
    END_SU2_OMP_FOR
  }
  atomicMax(local, invalidCount);
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    double global = 0;
    SU2_MPI::Allreduce(&invalidCount, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    invalidCount = global;
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  const bool feasible = invalidCount == 0;
  SU2_OMP_BARRIER
  return feasible;
}

void CNewtonEquationSet::RestoreSolutions() {
  assert(captured);
  SU2_OMP_FOR_STAT(1024)
  for (auto e = 0ul; e < acceptedReconstruction.size(); ++e)
    (*reconstructionHistory)[e] = acceptedReconstruction[e];
  END_SU2_OMP_FOR
  for (const auto& snapshot : snapshots) {
    auto* nodes = snapshot.solver->GetNodes();
    snapshot.solver->Jacobian.ClearPrescribedRows();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
      for (auto v = 0u; v < snapshot.variables; ++v) {
        const auto i = p * snapshot.variables + v;
        nodes->SetSolution(p, v, snapshot.solution[i]);
        nodes->SetSolution_Old(p, v, snapshot.oldSolution[i]);
        if (snapshot.prescribed[i]) snapshot.solver->Jacobian.MarkPrescribedRow(p, v);
      }
    }
    END_SU2_OMP_FOR
  }
}

void CNewtonEquationSet::Communicate() {
  for (const auto& snapshot : snapshots) {
    /*--- SetTrial restores the synchronized FLOW owned and halo primary values within this scope. ---*/
    if (reuseFixedFlow && snapshot.slot == FLOW_SOL) continue;
    snapshot.solver->InitiateComms(geometry, config, MPI_QUANTITIES::SOLUTION);
    snapshot.solver->CompleteComms(geometry, config, MPI_QUANTITIES::SOLUTION);
  }
}

void CNewtonEquationSet::ProjectSymmetryWalls() {
  /*--- Only groups that own FLOW move it; otherwise the flow is already projected by its own controller. ---*/
  if (std::none_of(blocks.begin(), blocks.end(), [](const Block& b) { return b.slot == FLOW_SOL; })) return;
  solvers[FLOW_SOL]->ProjectSymmetryWalls(geometry, config);
}

void CNewtonEquationSet::BeginFixedFlowReuse() {
  assert(captured && !reuseFixedFlow);
  if (blocks.size() != 1 || blocks.front().slot != TURB_SOL || config->GetKind_Turb_Model() != TURB_MODEL::SA) return;
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    /*--- Marker ownership differs by rank; every rank must omit the same gradient communications. ---*/
    const int local = CanReuseFixedFlow(*config) ? 1 : 0;
    int global = 0;
    SU2_MPI::Allreduce(&local, &global, 1, MPI_INT, MPI_MIN, SU2_MPI::GetComm());
    reuseFixedFlow = global != 0;
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
}

void CNewtonEquationSet::EndFixedFlowReuse() {
  if (!reuseFixedFlow) return;
  SU2_OMP_SAFE_GLOBAL_ACCESS(reuseFixedFlow = false;)
}

void CNewtonEquationSet::ProjectPeriodic(CSysVector<passivedouble>& vector,
                                         const CSysVector<passivedouble>* scale) const {
  if (!config->GetnMarker_Periodic()) return;
  assert(vector.GetNVar() == variables && (!scale || scale->GetLocSize() == vector.GetLocSize()));
  SU2_OMP_BARRIER
  for (const auto& block : blocks) {
    /*--- Carry a copy count with each native block, so axes and intersecting periodic pairs use the same P. ---*/
    SU2_OMP_SAFE_GLOBAL_ACCESS(periodicWork.resize(geometry->GetnPoint(), block.variables + 1);)
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
      for (auto v = 0u; v < block.variables; ++v) {
        const auto j = block.offset + v;
        periodicWork(p, v) = vector(p, j) * (scale ? (*scale)(p, j) : 1.0);
      }
      periodicWork(p, block.variables) = 1;
    }
    END_SU2_OMP_FOR
    geometry->AllocatePeriodicComms(block.variables + 1);
    SU2_OMP_SAFE_GLOBAL_ACCESS(geometry->SumPeriodicGeometry(config, periodicWork, block.slot == FLOW_SOL ? 1 : -1);)
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < block.variables; ++v) {
        const auto j = block.offset + v;
        vector(p, j) = SU2_TYPE::GetValue(periodicWork(p, v) / periodicWork(p, block.variables)) /
                       (scale ? (*scale)(p, j) : 1.0);
      }
    END_SU2_OMP_FOR
  }
  CSysMatrixComms::Initiate(vector, geometry, config);
  CSysMatrixComms::Complete(vector, geometry, config);
}

void CNewtonEquationSet::AddPeriodicConstraints(CSolver* solver) {
  if (!config->GetnMarker_Periodic()) return;
  const auto count = solver->GetnVar();
  const auto* nodes = solver->GetNodes();
  SU2_OMP_SAFE_GLOBAL_ACCESS(periodicWork.resize(geometry->GetnPoint(), count + 1);)
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
    for (auto v = 0u; v < count; ++v) periodicWork(p, v) = nodes->GetSolution(p, v);
    periodicWork(p, count) = 1;
  }
  END_SU2_OMP_FOR
  geometry->AllocatePeriodicComms(count + 1);
  SU2_OMP_SAFE_GLOBAL_ACCESS(geometry->SumPeriodicGeometry(config, periodicWork, solver == solvers[FLOW_SOL] ? 1 : -1);)
  /*--- Native assembly supplies P*R(w). Add B(w)*(I-P)*w without overwriting the supplied unknowns.
   *     Live density in B preserves conservative-scalar cross derivatives; M_base only defines coordinates. ---*/
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
    const su2double mass = solver->GetConservative() ? solvers[FLOW_SOL]->GetNodes()->GetDensity(p) : su2double(1);
    for (auto v = 0u; v < count; ++v)
      solver->LinSysRes(p, v) += mass * (nodes->GetSolution(p, v) - periodicWork(p, v) / periodicWork(p, count));
  }
  END_SU2_OMP_FOR
}

void CNewtonEquationSet::SynchronizeTimeSteps() {
  if (!config->GetnMarker_Periodic()) return;
  EquationSettingsGuard guard(*config, solvers);
  SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetFrozenIterationHistory(false);)
  for (const auto& block : blocks) {
    /*--- Reuse the native periodic minimum for base dt, restoring the residual it also communicates. ---*/
    SU2_OMP_SAFE_GLOBAL_ACCESS(periodicWork.resize(geometry->GetnPointDomain(), block.variables);)
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto v = 0u; v < block.variables; ++v) periodicWork(p, v) = block.solver->LinSysRes(p, v);
    END_SU2_OMP_FOR
    for (auto pair = 1u; pair <= config->GetnMarker_Periodic() / 2; ++pair) {
      block.solver->InitiatePeriodicComms(geometry, config, pair, PERIODIC_RESIDUAL);
      block.solver->CompletePeriodicComms(geometry, config, pair, PERIODIC_RESIDUAL);
    }
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto v = 0u; v < block.variables; ++v) block.solver->LinSysRes(p, v) = periodicWork(p, v);
    END_SU2_OMP_FOR
  }
}

void CNewtonEquationSet::Refresh() {
  SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_FLOW_SYS);)
  if (reuseFixedFlow)
    solvers[FLOW_SOL]->Preprocessing_Properties(geometry, solvers, config);
  else
    solvers[FLOW_SOL]->Preprocessing(geometry, solvers, config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, true);
  if (solvers[TRANS_SOL]) solvers[FLOW_SOL]->Friction_Forces(geometry, config);
  if (solvers[TURB_SOL]) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_TURB_SYS);)
    solvers[TURB_SOL]->Postprocessing(geometry, solvers, config, MESH_0);
  }
  if (solvers[TRANS_SOL]) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_TRANS_SYS);)
    solvers[TRANS_SOL]->Postprocessing(geometry, solvers, config, MESH_0);
  }
  /*--- Copy the newly computed eddy viscosity into the flow primitive/transport data. ---*/
  if (solvers[TURB_SOL] || HasFlowFeedback(*config)) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_FLOW_SYS);)
    if (config->GetWall_Functions() || HasFlowFeedback(*config))
      solvers[FLOW_SOL]->Preprocessing(geometry, solvers, config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, true);
    else
      /*--- Only derived transport data changed; temperature/velocity gradients remain current. ---*/
      solvers[FLOW_SOL]->Preprocessing_Properties(geometry, solvers, config);
  }
  /*--- Mixture flow fluxes consume species gradients even when species is outside the Newton group. ---*/
  if (solvers[SPECIES_SOL]) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_SPECIES_SYS);)
    solvers[SPECIES_SOL]->Preprocessing(geometry, solvers, config, MESH_0, NO_RK_ITER, RUNTIME_SPECIES_SYS, true);
  }
}

void CNewtonEquationSet::InstallStrongValues(CSolver* solver) const {
  auto* nodes = solver->GetNodes();
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
    for (auto v = 0u; v < solver->GetnVar(); ++v)
      if (solver->Jacobian.IsPrescribedRow(p, v)) nodes->SetSolution(p, v, nodes->GetSolution_Old(p, v));
  END_SU2_OMP_FOR
}

void CNewtonEquationSet::StrongState(unsigned short runtime) {
  const auto slot = config->GetContainerPosition(runtime);
  if (!solvers[slot]) return;
  SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), runtime);)
  solvers[slot]->Set_OldSolution();
  StrongBoundaryConditions(geometry, solvers, numerics[slot], config, runtime);
  InstallStrongValues(solvers[slot]);
}

bool CNewtonEquationSet::CheckState(bool compareFlow, bool modelBounds) {
  SU2_OMP_SAFE_GLOBAL_ACCESS(invalidCount = 0;)
  double local = 0;
  const auto* flow = solvers[FLOW_SOL]->GetNodes();
  const auto nFlow = solvers[FLOW_SOL]->GetnVar();
  const bool compressible = config->GetKind_Regime() == ENUM_REGIME::COMPRESSIBLE;
  const bool viscous = config->GetViscous();
  const bool ideal = config->GetKind_FluidModel() == STANDARD_AIR || config->GetKind_FluidModel() == IDEAL_GAS;
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
    for (auto v = 0u; v < nFlow; ++v) {
      const auto value = flow->GetSolution(p, v);
      if (!Armijo::IsFinite(SU2_TYPE::GetValue(value))) local += 1;
      if (compareFlow && value != workspace->trialFlow[p * nFlow + v]) local += 1;
    }
    if (compressible) {
      const auto rho = flow->GetSolution(p, 0);
      if (!(rho > 0))
        local += 1;
      else if (ideal) {
        auto internal = flow->GetSolution(p, nFlow - 1);
        for (auto v = 1u; v <= geometry->GetnDim(); ++v) internal -= 0.5 * pow(flow->GetSolution(p, v), 2) / rho;
        if (config->GetKind_Turb_Model() == TURB_MODEL::SST)
          internal -= rho * solvers[TURB_SOL]->GetNodes()->GetSolution(p, 0);
        if (!(internal > 0)) local += 1;
      }
    }
    if (compareFlow) {
      const auto temperature = SU2_TYPE::GetValue(flow->GetTemperature(p));
      const auto rho = SU2_TYPE::GetValue(flow->GetDensity(p));
      if (!(temperature > 0 && rho > 0) || !Armijo::IsFinite(temperature) || !Armijo::IsFinite(rho)) local += 1;
      if (compressible)
        for (auto value : {SU2_TYPE::GetValue(flow->GetPressure(p)), SU2_TYPE::GetValue(flow->GetSoundSpeed(p))})
          if (!(value > 0) || !Armijo::IsFinite(value)) local += 1;
    }
    if (compareFlow && (!compressible || viscous)) {
      const auto cp = SU2_TYPE::GetValue(flow->GetSpecificHeatCp(p));
      if (!(cp > 0) || !Armijo::IsFinite(cp)) local += 1;
    }
    if (compareFlow && viscous)
      for (auto value : {SU2_TYPE::GetValue(flow->GetLaminarViscosity(p)),
                         SU2_TYPE::GetValue(flow->GetThermalConductivity(p))})
        if (!(value >= 0) || !Armijo::IsFinite(value)) local += 1;
  }
  END_SU2_OMP_FOR
  for (const auto& block : snapshots) {
    if (block.slot == FLOW_SOL) continue;
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
      double sum = 0;
      for (auto v = 0u; v < block.variables; ++v) {
        const auto value = SU2_TYPE::GetValue(block.solver->GetNodes()->GetSolution(p, v));
        if (!Armijo::IsFinite(value)) local += 1;
        if (modelBounds && !block.solver->Jacobian.IsPrescribedRow(p, v)) {
          const auto bounds = block.solver->GetSolutionBounds(v);
          if (!(value >= bounds.first && value <= bounds.second)) local += 1;
        }
        if (block.slot == TURB_SOL && config->GetKind_Turb_Model() == TURB_MODEL::SST &&
            (v == 0 ? value < 0 : !(value > 0))) local += 1;
        if (block.slot == TRANS_SOL && (v == 0 ? value < 0 : !(value > 0))) local += 1;
        if (block.slot == SPECIES_SOL && config->GetKind_FluidModel() == FLUID_MIXTURE) {
          if (!(value >= 0 && value <= 1)) local += 1;
          sum += value;
        }
      }
      if (sum > 1) local += 1;
      if (compareFlow && block.slot == SPECIES_SOL) {
        /*--- Ordinary mixture fluxes also consume diffusivity of the untransported last fraction. ---*/
        const auto count = block.variables + (config->GetKind_Species_Model() == SPECIES_MODEL::SPECIES_TRANSPORT);
        for (auto v = 0u; v < count; ++v) {
          const auto value = SU2_TYPE::GetValue(block.solver->GetNodes()->GetDiffusivity(p, v));
          if (!(value >= 0) || !Armijo::IsFinite(value)) local += 1;
        }
      }
    }
    END_SU2_OMP_FOR
  }
  atomicAdd(local, invalidCount);
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    double global = 0;
    SU2_MPI::Allreduce(&invalidCount, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    valid = global == 0;
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  return valid;
}

bool CNewtonEquationSet::EvaluateResiduals(bool implicit, bool allEquations, double reconstruction,
                                          unsigned short residualRuntime) {
  assert(reconstruction >= 0 && reconstruction <= 1);
  assert(!residualRuntime || (!implicit && !allEquations &&
      std::any_of(blocks.begin(), blocks.end(), [=](const Block& b) { return b.runtime == residualRuntime; })));
  EquationSettingsGuard guard(*config, solvers);
  struct HistoryGuard {
    CNewtonEquationSet& equations;
    explicit HistoryGuard(CNewtonEquationSet& e) : equations(e) { equations.SaveHistory(); }
    ~HistoryGuard() { equations.RestoreHistory(); }
  } history(*this);
  /*--- Physical evaluations use full reconstruction; only an explicit Jacobian-model request may relax it. ---*/
  SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetResidualOnly(true); config->SetFrozenIterationHistory(true);
                             config->SetNewtonKrylovRelaxation(reconstruction);)
  Communicate();
  if (!CheckState(false)) return false;
  for (auto& snapshot : snapshots) {
    snapshot.solver->Jacobian.ClearPrescribedRows();
    const auto* nodes = snapshot.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < snapshot.variables; ++v)
        snapshot.work->boundaryInput[p * snapshot.variables + v] = nodes->GetSolution(p, v);
    END_SU2_OMP_FOR
  }
  /*--- Composition and no-slip values precede the first thermodynamic/wall-function refresh. ---*/
  for (auto runtime : {RUNTIME_SPECIES_SYS, RUNTIME_FLOW_SYS}) StrongState(runtime);
  Communicate();
  if (!CheckState(false)) return false;
  const auto* flow = solvers[FLOW_SOL]->GetNodes();
  const auto nFlow = solvers[FLOW_SOL]->GetnVar();
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
    for (auto v = 0u; v < nFlow; ++v) workspace->trialFlow[p * nFlow + v] = flow->GetSolution(p, v);
  END_SU2_OMP_FOR
  Refresh();
  if (!CheckState(true)) return false;
  /*--- Coupled strong values must settle with their properties before any differentiated gradient or source. ---*/
  bool constrained = false;
  for (unsigned sweep = 0; sweep < 200; ++sweep) {
    for (auto& snapshot : snapshots) {
      snapshot.solver->Jacobian.ClearPrescribedRows();
      const auto* nodes = snapshot.solver->GetNodes();
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < snapshot.variables; ++v)
          snapshot.work->boundarySolution[p * snapshot.variables + v] = nodes->GetSolution(p, v);
      END_SU2_OMP_FOR
    }
    for (auto runtime : runtimes) StrongState(runtime);
    /*--- A conditional row that becomes free must retain the supplied trial value. ---*/
    for (const auto& snapshot : snapshots) {
      auto* nodes = snapshot.solver->GetNodes();
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        for (auto v = 0u; v < snapshot.variables; ++v)
          if (!snapshot.solver->Jacobian.IsPrescribedRow(p, v))
            nodes->SetSolution(p, v, snapshot.work->boundaryInput[p * snapshot.variables + v]);
      END_SU2_OMP_FOR
    }
    Communicate();
    SU2_OMP_SAFE_GLOBAL_ACCESS(invalidCount = 0;)
    double changed = 0;
    for (const auto& snapshot : snapshots) {
      const auto* nodes = snapshot.solver->GetNodes();
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
        for (auto v = 0u; v < snapshot.variables; ++v)
          if (nodes->GetSolution(p, v) != snapshot.work->boundarySolution[p * snapshot.variables + v]) changed = 1;
      END_SU2_OMP_FOR
    }
    atomicAdd(changed, invalidCount);
    BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
      /*--- Boundary ownership differs by rank. Agree on gradient communication with the existing closure reduction. ---*/
      const double local[2] = {invalidCount, CanReuseBoundaryGradients(*config) ? 0.0 : 1.0};
      double global[2] = {};
      SU2_MPI::Allreduce(local, global, 2, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      valid = global[0] == 0;
      reuseBoundaryGradients = global[1] == 0;
    }
    END_SU2_OMP_SAFE_GLOBAL_ACCESS
    const bool unchanged = valid;
    if (unchanged && !HasFlowFeedback(*config)) {
      /*--- Unchanged primary values retain the live coefficients from the preceding refresh. ---*/
      if (!CheckState(true, true)) return false;
      constrained = true;
      break;
    }
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < nFlow; ++v) workspace->trialFlow[p * nFlow + v] = flow->GetSolution(p, v);
    END_SU2_OMP_FOR
    Refresh();
    /*--- Exact strong data take precedence over numerical clipping floors. ---*/
    if (!CheckState(true, true)) return false;
    if (unchanged) {
      constrained = true;
      break;
    }
  }
  if (!constrained) return false;
  /*--- Flow first also refreshes characteristic boundary states consumed by auxiliary equations. ---*/
  for (auto index = 0u; index < 4; ++index) {
    const auto runtime = runtimes[index];
    auto block = std::find_if(blocks.begin(), blocks.end(), [=](const Block& b) { return b.runtime == runtime; });
    const bool selected = block != blocks.end();
    const bool assemble = allEquations || (selected && (!residualRuntime || residualRuntime == runtime));
    if (!selected && !allEquations && runtime != RUNTIME_FLOW_SYS) continue;
    const auto slot = config->GetContainerPosition(runtime);
    if (!solvers[slot]) continue;
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetResidualOnly(!implicit || !assemble);
                               config->SetGlobalParam(config->GetKind_Solver(), runtime);)
    /*--- Bounded scalars need live edge mass fluxes. Keep feedback and custom-source failure checks.
     *     Auxiliary owners never commit flow reconstruction history; its saved counters remain frozen. ---*/
    const bool boundaryOnly = runtime == RUNTIME_FLOW_SYS && !assemble &&
                              !config->GetBounded_Scalar() && !config->GetPyCustomSource() && !HasFlowFeedback(*config);
    if (boundaryOnly && (reuseBoundaryGradients || reuseFixedFlow))
      /*--- Refresh() already computed the gradients used here; retain native properties and residual preparation. ---*/
      solvers[slot]->Preprocessing_Properties(geometry, solvers, config, false);
    else
      /*--- Dependency-only preprocessing retains the settled row masks and live turbulence gradients. ---*/
      solvers[slot]->Preprocessing(geometry, solvers, config, MESH_0, NO_RK_ITER, runtime,
                                   !assemble && runtime != RUNTIME_FLOW_SYS);
    if (!assemble && runtime != RUNTIME_FLOW_SYS) continue;
    if (boundaryOnly) {
      /*--- Reducer preprocessing leaves the residual uncleared until the volume pass. ---*/
      solvers[slot]->LinSysRes.SetValZero();
      SU2_OMP_BARRIER
      Boundary_Integration(geometry, solvers, numerics[slot], config, MESH_0, runtime);
      SU2_OMP_SAFE_GLOBAL_ACCESS(++flowBoundaryEvaluations;)
    } else {
      Space_Integration(geometry, solvers, numerics[slot], config, MESH_0, NO_RK_ITER, runtime);
      SU2_OMP_SAFE_GLOBAL_ACCESS(++residualEvaluations[index];)
    }
    InstallStrongValues(solvers[slot]);
    /*--- Keep the supplied unknowns: the consistency equations remove the missing periodic modes. ---*/
    if (!boundaryOnly) AddPeriodicConstraints(solvers[slot]);
  }
  SU2_OMP_FOR_STAT(1024)
  for (auto e = 0ul; e < nextReconstruction.size(); ++e)
    nextReconstruction[e] = (*reconstructionHistory)[e];
  END_SU2_OMP_FOR
  SU2_OMP_SAFE_GLOBAL_ACCESS(invalidCount = 0;)
  double invalid = 0;
  for (const auto& snapshot : snapshots) {
    if (!allEquations && snapshot.slot != FLOW_SOL &&
        std::none_of(blocks.begin(), blocks.end(), [&](const Block& b) {
          return b.slot == snapshot.slot && (!residualRuntime || b.runtime == residualRuntime);
        })) continue;
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto v = 0u; v < snapshot.variables; ++v)
        if (!Armijo::IsFinite(SU2_TYPE::GetValue(snapshot.solver->LinSysRes(p, v)))) invalid = 1;
    END_SU2_OMP_FOR
  }
  atomicAdd(invalid, invalidCount);
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    double global = 0;
    SU2_MPI::Allreduce(&invalidCount, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
    valid = global == 0;
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  return valid;
}

void CNewtonEquationSet::SaveHistory() {
  SU2_OMP_FOR_STAT(1024)
  for (auto e = 0ul; e < workspace->savedReconstruction.size(); ++e)
    workspace->savedReconstruction[e] = (*reconstructionHistory)[e];
  END_SU2_OMP_FOR
  for (auto& snapshot : snapshots) {
    const auto* nodes = snapshot.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < snapshot.variables; ++v)
        snapshot.work->evaluationHistory[p * snapshot.variables + v] = nodes->GetSolution_Old(p, v);
    END_SU2_OMP_FOR
  }
}

void CNewtonEquationSet::RestoreHistory() {
  SU2_OMP_FOR_STAT(1024)
  for (auto e = 0ul; e < workspace->savedReconstruction.size(); ++e)
    (*reconstructionHistory)[e] = workspace->savedReconstruction[e];
  END_SU2_OMP_FOR
  for (const auto& snapshot : snapshots) {
    auto* nodes = snapshot.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < snapshot.variables; ++v)
        nodes->SetSolution_Old(p, v, snapshot.work->evaluationHistory[p * snapshot.variables + v]);
    END_SU2_OMP_FOR
  }
}

void CNewtonEquationSet::CommitHistory() {
  assert(captured && valid);
  SU2_OMP_FOR_STAT(1024)
  for (auto e = 0ul; e < nextReconstruction.size(); ++e)
    (*reconstructionHistory)[e] = nextReconstruction[e];
  END_SU2_OMP_FOR
}

bool CNewtonEquationSet::Restore() {
  RestoreSolutions();
  const bool restored = EvaluateResiduals();
  for (const auto& snapshot : snapshots) {
    auto* nodes = snapshot.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) nodes->SetUnderRelaxation(p, snapshot.relaxation[p]);
    END_SU2_OMP_FOR
  }
  return restored;
}

double CNewtonEquationSet::GetRestorationError() {
  assert(captured);
  SU2_OMP_SAFE_GLOBAL_ACCESS(invalidCount = 0;)
  double local = 0;
  auto compare = [&](const su2double& current, const su2double& saved) {
    const double delta = SU2_TYPE::GetValue(current - saved);
    local = Armijo::IsFinite(delta) ? std::max(local, std::abs(delta)) : std::numeric_limits<double>::infinity();
  };
  for (const auto& snapshot : snapshots) {
    const auto* nodes = snapshot.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
      compare(nodes->GetUnderRelaxation(p), snapshot.relaxation[p]);
      for (auto v = 0u; v < snapshot.variables; ++v) {
        const auto j = p * snapshot.variables + v;
        compare(nodes->GetSolution(p, v), snapshot.solution[j]);
        compare(nodes->GetSolution_Old(p, v), snapshot.oldSolution[j]);
        if (snapshot.solver->Jacobian.IsPrescribedRow(p, v) != bool(snapshot.prescribed[j]))
          local = std::max(local, 1.0);
      }
    }
    END_SU2_OMP_FOR
  }
  SU2_OMP_FOR_STAT(1024)
  for (auto e = 0ul; e < acceptedReconstruction.size(); ++e)
    local = std::max(local, std::abs(double((*reconstructionHistory)[e]) - acceptedReconstruction[e]));
  END_SU2_OMP_FOR
  atomicMax(local, invalidCount);
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    double global = 0;
    SU2_MPI::Allreduce(&invalidCount, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    invalidCount = global;
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  const double error = invalidCount;
  SU2_OMP_BARRIER
  return error;
}
