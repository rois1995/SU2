/*!
 * \file CNewtonIntegration.cpp
 * \brief Globalized inexact Newton iteration for selected equation groups.
 * \author rois1995
 */
#include "../../include/integration/CNewtonIntegration.hpp"
#include "../../../Common/include/linear_algebra/CMatrixVectorProduct.hpp"
#include <fstream>
#include <sstream>

#if defined(CODI_REVERSE_TYPE) || defined(CODI_FORWARD_TYPE)
void CNewtonIntegration::MultiGrid_Iteration(CGeometry****, CSolver*****, CNumerics******, CConfig**, unsigned short,
                                             unsigned short, unsigned short) {
  SU2_MPI::Error("Common Newton integration requires a primal executable", CURRENT_FUNCTION);
}
void CNewtonIntegration::RefreshAfterOuterIteration() {}
#else

namespace {
using Scalar = CNewtonIntegration::Scalar;
class Product final : public CMatrixVectorProduct<Scalar> {
  CNewtonIntegration& integration;

 public:
  explicit Product(CNewtonIntegration& instance) : integration(instance) {}
  void operator()(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const override {
    integration.MatrixFreeProduct(u, v, true);
  }
};
class Preconditioner final : public CPreconditioner<Scalar> {
  const CNewtonIntegration& integration;

 public:
  explicit Preconditioner(const CNewtonIntegration& instance) : integration(instance) {}
  void operator()(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const override { integration.Precondition(u, v); }
};
const char* Name(CNewtonLinearization::Failure failure) {
  using Failure = CNewtonLinearization::Failure;
  switch (failure) {
    case Failure::Direction:
      return "DERIVATIVE_DIRECTION";
    case Failure::Admissibility:
      return "DERIVATIVE_ADMISSIBILITY";
    case Failure::Verification:
      return "DERIVATIVE_VERIFICATION";
    default:
      return "NONE";
  }
}
const char* Name(Armijo::Status status) {
  switch (status) {
    case Armijo::Status::Accepted:
      return "ACCEPTED";
    case Armijo::Status::NonDescent:
      return "NON_DESCENT";
    case Armijo::Status::StepTooSmall:
      return "STEP_TOO_SMALL";
    case Armijo::Status::EvaluationLimit:
      return "EVALUATION_LIMIT";
    default:
      return "INVALID_INPUT";
  }
}
const char* Name(NonlinearGlobalization::RecoveryReason reason) {
  using Reason = NonlinearGlobalization::RecoveryReason;
  switch (reason) {
    case Reason::Accepted:
      return "ACCEPTED";
    case Reason::Retry:
      return "RETRY";
    case Reason::RetryLimit:
      return "RETRY_LIMIT";
    case Reason::MinimumCFL:
      return "MINIMUM_CFL";
    case Reason::Disabled:
      return "RECOVERY_DISABLED";
    default:
      return "INVALID_STATE";
  }
}
}  // namespace

double CNewtonIntegration::Reduce(double local, bool maximum) const {
  reductionPartials[omp_get_thread_num()] = local;
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    /*--- Thread arrival order must not change globalization decisions. ---*/
    double combined = reductionPartials[0];
    for (int thread = 1; thread < omp_get_num_threads(); ++thread)
      combined = maximum ? std::max(combined, reductionPartials[thread]) : combined + reductionPartials[thread];
    SU2_MPI::Allreduce(&combined, &reduction, 1, MPI_DOUBLE, maximum ? MPI_MAX : MPI_SUM, SU2_MPI::GetComm());
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  const double result = reduction;
  SU2_OMP_BARRIER
  return result;
}

double CNewtonIntegration::CFLFactor(const CNewtonEquationSet::Block& block) const {
  if (block.slot == TURB_SOL || block.slot == TRANS_SOL) return SU2_TYPE::GetValue(config->GetCFLRedCoeff_Turb());
  if (block.slot == SPECIES_SOL) return SU2_TYPE::GetValue(config->GetCFLRedCoeff_Species());
  return 1;
}

void CNewtonIntegration::ConfigureFlow() {
  if (std::find(systems.begin(), systems.end(), RUNTIME_FLOW_SYS) == systems.end())
    throw std::invalid_argument("Legacy flow controls require a group containing flow");
  const auto integers = config->GetNewtonKrylovIntParam();
  const auto values = config->GetNewtonKrylovDblParam();
  for (const auto& value : values)
    if (!Armijo::IsFinite(SU2_TYPE::GetValue(value)))
      throw std::invalid_argument("NEWTON_KRYLOV_DPARAM must be finite");
  flow.remaining = integers[0];
  flow.innerIterations = integers[1];
  flow.toleranceRelaxation = integers[2];
  flow.deflation = integers[3] != 0;
  flow.startupDrop = SU2_TYPE::GetValue(values[0]);
  flow.innerTolerance = SU2_TYPE::GetValue(values[1]);
  flow.fullToleranceDrop = SU2_TYPE::GetValue(values[2]);
  flow.automatic = values[4] < 0;
  flow.reconstruction = flow.automatic ? 1 : std::min(1.0, SU2_TYPE::GetValue(values[4]));
  flow.startup = (flow.remaining || flow.startupDrop < 0) && config->GetKind_TimeIntScheme_Flow() == EULER_IMPLICIT;
  flow.linearIterations = config->GetLinear_Solver_Iter();
  flow.linearTolerance = SU2_TYPE::GetValue(config->GetLinear_Solver_Error());
  if (!(values[3] > 0 && flow.innerTolerance >= 0 && flow.linearTolerance >= 0 && flow.linearTolerance < 1) ||
      !Armijo::IsFinite(flow.linearTolerance) || !flow.linearIterations ||
      (!flow.deflation && flow.linearIterations > 1000))
    throw std::invalid_argument("Invalid legacy Newton finite-difference, inner or outer linear-solver controls");
  if (config->GetKind_Linear_Solver_Inner() == LINEAR_SOLVER_INNER::NONE &&
      (flow.innerIterations > 1000 || (flow.startup && flow.linearIterations > 1000)))
    throw std::invalid_argument("Native FGMRES subspaces cannot exceed 1000 directions");
  options.fdStep = SU2_TYPE::GetValue(values[3]);
  options.linearTolerance = std::max(std::numeric_limits<double>::epsilon(), flow.linearTolerance);
  options.etaMin = std::min(options.etaMin, options.linearTolerance);
  options.etaMax = std::max(options.etaMax, options.linearTolerance);
  options.linearIterations = std::min(1000ul, flow.linearIterations);
  options.adaptCFL = options.adaptCFL && config->GetCFL_Adapt();
  if (config->GetCFL_Adapt()) {
    options.cflMin = SU2_TYPE::GetValue(config->GetCFL_AdaptParam(2));
    options.cflMax = SU2_TYPE::GetValue(config->GetCFL_AdaptParam(3));
  }
}

void CNewtonIntegration::PrepareFlow(Attempt& attempt) {
  if (!legacyFlowControls) return;
  const auto& block = *std::find_if(equations->GetBlocks().begin(), equations->GetBlocks().end(),
                                    [](const CNewtonEquationSet::Block& b) { return b.slot == FLOW_SOL; });
  double mean = 0;
  for (auto v = 0u; v < block.variables; ++v) {
    double local = 0;
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p) {
      const double residual = rhs(p, block.offset + v) * scale(p, block.offset + v);
      local += residual * residual;
    }
    END_SU2_OMP_FOR
    mean += std::log10(std::max(EPS * EPS, std::sqrt(Reduce(local) / geometry->GetGlobal_nPointDomain()))) /
            block.variables;
  }
  SU2_OMP_SAFE_GLOBAL_ACCESS(flow.firstResidual = std::max(flow.firstResidual, mean);)
  attempt.meanFlowResidual = mean;
  attempt.startup = flow.startup;
  attempt.deflated = !flow.startup && flow.deflation;
  attempt.reconstruction = flow.startup ? 1 : flow.reconstruction;
  if (!flow.startup && flow.toleranceRelaxation > 1 && flow.fullToleranceDrop < 0) {
    const double progress = (mean - flow.firstResidual) / flow.fullToleranceDrop;
    attempt.toleranceFactor = 1 + (flow.toleranceRelaxation - 1) * std::max(0.0, 1 - progress);
  }
}

double CNewtonIntegration::CommitFlow(const Attempt& attempt, double nextCFL) {
  if (!legacyFlowControls || attempt.search.step == 0) return nextCFL;
  BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
    if (attempt.startup) {
      if (flow.remaining) --flow.remaining;
      if (!flow.remaining && attempt.meanFlowResidual - flow.firstResidual < flow.startupDrop) {
        flow.startup = false;
        flow.firstResidual = attempt.meanFlowResidual;
        if (flow.automatic) flow.reconstruction = 0;
      }
    } else if (flow.automatic) {
      const double tolerance = std::max(
          flow.linearTolerance, config->GetCFL_Adapt() ? SU2_TYPE::GetValue(config->GetCFL_AdaptParam(4)) : 0.0);
      const double achieved = attempt.reportedLinearResidual / attempt.toleranceFactor;
      if (achieved > tolerance)
        flow.reconstruction *= 0.9;
      else if (achieved < 0.9 * tolerance)
        flow.reconstruction = std::min(1.0, std::max(0.05, flow.reconstruction * 1.05));
    }
  }
  END_SU2_OMP_SAFE_GLOBAL_ACCESS
  if (attempt.startup && !flow.startup)
    nextCFL = std::max(options.cflMin, std::min(options.cflMax, SU2_TYPE::GetValue(config->GetCFL(MESH_0))));
  return nextCFL;
}

void CNewtonIntegration::Setup() {
  if (legacyFlowControls) ConfigureFlow();
  const auto error = options.Validate();
  if (!error.empty()) throw std::invalid_argument(error);
  if (config->GetTime_Marching() != TIME_MARCHING::STEADY || config->GetTime_Domain() ||
      config->GetMultizone_Problem() || config->GetnZone() != 1 || config->GetCUDA() ||
      config->GetContinuous_Adjoint() || config->GetDiscrete_Adjoint() || config->GetDirectDiff() != NO_DERIVATIVE ||
      config->GetFinestMesh() != MESH_0)
    throw std::invalid_argument(
        "Common Newton currently requires steady primal single-zone CPU cases");
  if (config->GetnMarker_Periodic() &&
      (config->GetKind_Species_Model() != SPECIES_MODEL::NONE ||
       config->GetKind_Streamwise_Periodic() != ENUM_STREAMWISE_PERIODIC::NONE))
    throw std::invalid_argument("Periodic common Newton currently excludes species and streamwise models");
  if (config->GetKind_Regime() == ENUM_REGIME::INCOMPRESSIBLE &&
      config->GetKind_Incomp_System() == INCOMP_SYSTEM::PRESSURE_BASED)
    throw std::invalid_argument("Newton flow groups require the density-based incompressible formulation");
  equations = std::make_unique<CNewtonEquationSet>(geometry, solvers, numerics, config, systems, residualWorkspace);
  for (const auto& block : equations->GetBlocks()) {
    const auto time = block.slot == FLOW_SOL ? config->GetKind_TimeIntScheme_Flow()
                                             : block.slot == SPECIES_SOL ? config->GetKind_TimeIntScheme_Species()
                                                                         : config->GetKind_TimeIntScheme_Turb();
    if ((time != EULER_IMPLICIT && block.slot != FLOW_SOL) || !(CFLFactor(block) > 0))
      throw std::invalid_argument("Newton scalar equations require implicit discretization; all selected CFL factors must be positive");
  }
  for (auto* vector : {&scale, &rhs, &correction, &direction, &product, &pseudoDirection, &probeReference, &baseState})
    vector->Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), equations->GetnVar(), 0.0);
  free.resize(geometry->GetnPoint() * equations->GetnVar(), true);
  linearSolver.SetxIsZero(true);
  pseudoTime = std::make_unique<CNewtonPseudoTime>(*equations, geometry, config);
  linearization = std::make_unique<CNewtonLinearization>(*equations, geometry, config);
  directionOperator = linearization.get();
  if (legacyFlowControls && (flow.automatic || flow.reconstruction < 1))
    modelLinearization = std::make_unique<CNewtonLinearization>(*equations, geometry, config);
  preconditioner =
      std::make_unique<CNewtonBlockPreconditioner>(*linearization, geometry, config, lowerTriangular,
                                                   legacyFlowControls ? flow.innerIterations : 0, flow.innerTolerance);
  const auto primary = std::min_element(
      equations->GetBlocks().begin(), equations->GetBlocks().end(),
      [](const CNewtonEquationSet::Block& a, const CNewtonEquationSet::Block& b) { return a.slot < b.slot; });
  referenceCFLFactor = CFLFactor(*primary);
  cfl = std::max(options.cflMin,
                 std::min(options.cflMax, SU2_TYPE::GetValue(config->GetCFL(MESH_0)) * referenceCFLFactor));
  eta = options.linearTolerance;
  setup = true;
}

void CNewtonIntegration::SetCFL(double value) {
  for (const auto& block : equations->GetBlocks()) {
    const double native = value * CFLFactor(block) / referenceCFLFactor;
    auto* nodes = block.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) nodes->SetLocalCFL(p, native);
    END_SU2_OMP_FOR
    SU2_OMP_SAFE_GLOBAL_ACCESS(block.solver->SetCFL_Local_Stats(native);)
  }
}

void CNewtonIntegration::UpdateScale() {
  equations->BuildScales(scale, probeReference);
  /*--- Legacy flow controls retain their original physical probe metric and relative references. ---*/
  if (legacyFlowControls) probeReference = scale;
}

void CNewtonIntegration::Perturb(const CSysVector<Scalar>& vector, double step) {
  equations->SetTrial(vector, step, scale, [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) {
    return IsFree(b, p, v);
  });
}

void CNewtonIntegration::Residual(bool implicit) {
  const bool valid = equations->EvaluateResiduals(implicit);
  SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid;)
}

bool CNewtonIntegration::RefreshStatistics(bool allEquations) {
  bool valid = true;
  for (auto runtime : {RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS, RUNTIME_TRANS_SYS, RUNTIME_SPECIES_SYS}) {
    if (!allEquations && !IsNewtonEquation(runtime)) continue;
    auto* solver = solvers[config->GetContainerPosition(runtime)];
    if (solver && !solver->RefreshResidualStatistics(geometry, config, false)) valid = false;
  }
  return valid;
}

double CNewtonIntegration::Merit(double step) const {
  if (!residualValid) return std::numeric_limits<double>::infinity();
  double value = 0, invalid = 0;
  for (const auto& block : equations->GetBlocks()) {
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPointDomain(); ++p)
      for (auto v = 0u; v < block.variables; ++v) {
        const auto j = block.offset + v;
        double r = SU2_TYPE::GetValue(block.solver->LinSysRes(p, v)) / scale(p, j);
        if (options.pseudoTimeMerit && step != 0) r += step * pseudoDirection(p, j);
        if (!Armijo::IsFinite(r))
          invalid = 1;
        else
          value += 0.5 * r * r;
      }
    END_SU2_OMP_FOR
  }
  if (!Armijo::IsFinite(value)) invalid = 1;
  const double anyInvalid = Reduce(invalid, true);
  const double global = Reduce(value);
  return anyInvalid ? std::numeric_limits<double>::infinity() : global;
}

double CNewtonIntegration::Evaluate(double step, const char* phase) {
  Perturb(direction, step);
  Residual();
  const double physical = Merit();
  const double value = Armijo::IsFinite(physical) && options.pseudoTimeMerit && step != 0 ? Merit(step) : physical;
  LogEvaluation(step, phase, value, physical);
  return value;
}

void CNewtonIntegration::LogEvaluation(double step, const char* phase, double value, double physical) {
  if (options.diagnostics && rank == MASTER_NODE && omp_get_thread_num() == 0) {
    std::ofstream out(prefix + "_trials.csv", std::ios::app);
    if (out.tellp() == 0) out << "iteration,attempt,evaluation,phase,step,merit,physical_merit\n";
    out.precision(17);
    out << config->GetInnerIter() << ',' << attemptNumber << ',' << trialNumber++ << ',' << phase << ',' << step << ','
        << value << ',' << physical << '\n';
  }
}

void CNewtonIntegration::MatrixFreeProduct(const CSysVector<Scalar>& u, CSysVector<Scalar>& v, bool directionModel) {
  if (directionModel && nativeDirection) {
    preconditioner->Multiply(u, v);
    return;
  }
  CNewtonLinearization::Failure failure;
  const bool valid = (directionModel ? directionOperator : linearization.get())->Apply(u, v, true, true, 0, &failure);
  SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid;
                             if (!valid) {
                               if (failedProducts == 0) productFailure = Name(failure);
                               ++failedProducts;
                             })
}

void CNewtonIntegration::Precondition(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const {
  if (nativeDirection)
    preconditioner->Solve(u, v, 0, flow.innerTolerance);
  else
    (*preconditioner)(u, v);
}

void CNewtonIntegration::Restore(Attempt& attempt) {
  const bool valid = equations->Restore();
  SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid;)
  const double restored = Merit();
  attempt.rollbackMeritError = std::abs(restored - attempt.base);
  attempt.rollbackStateError = equations->GetRestorationError();
}

void CNewtonIntegration::Endpoint() {
  for (const auto& block : equations->GetBlocks()) {
    /*--- Each native limiter sees the same accepted density, before the independently limited endpoints are combined. ---*/
    Perturb(direction, 0);
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < block.variables; ++v)
        block.solver->LinSysSol(p, v) =
            IsFree(block, p, v) ? correction(p, block.offset + v) * scale(p, block.offset + v) : 0;
    END_SU2_OMP_FOR
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), block.runtime);)
    block.solver->CompleteImplicitIteration(geometry, solvers, config);
    const auto* nodes = block.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < block.variables; ++v) {
        const auto j = block.offset + v;
        direction(p, j) =
            IsFree(block, p, v)
                ? (SU2_TYPE::GetValue(equations->MassFactor(block, p) * nodes->GetSolution(p, v)) - baseState(p, j)) /
                      scale(p, j)
                : 0;
      }
    END_SU2_OMP_FOR
  }
}

CNewtonIntegration::Attempt CNewtonIntegration::SolveAttempt(bool preparedBase) {
  Attempt attempt;
  /*--- Each attempt freezes the pseudo-time action from its prepared base residual and native matrices. ---*/
  SU2_OMP_SAFE_GLOBAL_ACCESS(trialNumber = 0; failedProducts = 0; productFailure = "NONE";)
  if (!preparedBase) {
    /*--- Recovery changes the CFL and time-step action; rebuild the accepted base and its native matrices. ---*/
    SetCFL(cfl);
    Perturb(direction, 0);
    Residual(true);
  }
  attempt.base = Merit();
  attempt.physicalMerit = attempt.search.value = attempt.base;
  if (!Armijo::IsFinite(attempt.base) || !RefreshStatistics()) {
    attempt.residualFailure = true;
    attempt.rejectionReason = "BASE_RESIDUAL";
    Restore(attempt);
    return attempt;
  }
  /*--- Flow time steps precede scalar time steps even if the requested group has a different packing order. ---*/
  for (auto slot : {FLOW_SOL, TURB_SOL, TRANS_SOL, SPECIES_SOL})
    for (const auto& block : equations->GetBlocks()) {
      if (block.slot != slot) continue;
      SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), block.runtime);)
      block.solver->SetTime_Step(geometry, solvers, config, MESH_0, config->GetTimeIter());
    }
  equations->SynchronizeTimeSteps();
  /*--- Small scalar units must not hide a significant state-scaled residual. ---*/
  bool resolved = residualTolerance > 0 &&
                  std::sqrt(attempt.base / geometry->GetGlobal_nPointDomain()) <= residualTolerance / std::sqrt(2.0);
  for (const auto& block : equations->GetBlocks())
    for (auto v = 0u; v < block.variables; ++v)
      resolved = resolved && block.solver->GetRes_RMS(v) <= residualTolerance;
  if (resolved) {
    attempt.search = {Armijo::Status::Accepted, 0, attempt.base, 0};
    attempt.linearDefect = std::numeric_limits<double>::quiet_NaN();
    return attempt;
  }
  for (const auto& block : equations->GetBlocks()) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), block.runtime);)
    if (block.solver->Jacobian.HasCoefficients())
      block.solver->PrepareImplicitIteration(geometry, solvers, config, false);
  }
  for (const auto& block : equations->GetBlocks()) {
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < block.variables; ++v)
        free[p * equations->GetnVar() + block.offset + v] = !block.solver->Jacobian.IsPrescribedRow(p, v);
    END_SU2_OMP_FOR
  }
  if (!pseudoTime->Build(scale, free)) {
    attempt.linearFailure = true;
    attempt.rejectionReason = "PSEUDO_TIME";
    Restore(attempt);
    return attempt;
  }
  const double probeStep =
      legacyFlowControls
          ? options.fdStep * std::max(1.0, baseState.norm() / std::sqrt(geometry->GetGlobal_nPointDomain()))
          : options.fdStep * std::sqrt(equations->GetnVar() * geometry->GetGlobal_nPointDomain());
  const bool verifyStep = config->GetMUSCL_Flow() &&
                          std::find(systems.begin(), systems.end(), RUNTIME_FLOW_SYS) != systems.end();
  const bool valid = linearization->SetBase(
      scale, pseudoTime->GetDiagonal(), probeStep, options.relativeFDStep,
      [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) { return IsFree(b, p, v); },
      &probeReference, 1.0, legacyFlowControls, verifyStep, pseudoTime.get());
  SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid;)
  attempt.base = Merit();
  attempt.physicalMerit = attempt.search.value = attempt.base;
  if (!Armijo::IsFinite(attempt.base)) {
    attempt.residualFailure = true;
    attempt.rejectionReason = "BASE_RESIDUAL";
    Restore(attempt);
    return attempt;
  }
  /*--- Keep the physical right hand side independently of native sign flips and pseudo-time assembly. ---*/
  equations->CopyResidual(rhs);
  SU2_OMP_FOR_STAT(1024)
  for (auto i = 0ul; i < rhs.GetLocSize(); ++i) rhs[i] = -rhs[i] / scale[i];
  END_SU2_OMP_FOR
  PrepareFlow(attempt);
  attempt.linearTarget = legacyFlowControls && flow.linearTolerance == 0 ? 0 : eta * attempt.toleranceFactor;
  if (attempt.startup) attempt.linearTarget = flow.linearTolerance;
  if (attempt.base == 0) {
    attempt.search = {Armijo::Status::Accepted, 0, 0, 0};
    attempt.linearDefect = std::numeric_limits<double>::quiet_NaN();
    Residual();
    return attempt;
  }
  SU2_OMP_SAFE_GLOBAL_ACCESS(directionOperator = linearization.get(); nativeDirection = attempt.startup;)
  if (modelLinearization && attempt.reconstruction < 1) {
    const bool modelValid = modelLinearization->SetBase(
        scale, pseudoTime->GetDiagonal(), probeStep, options.relativeFDStep,
        [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) { return IsFree(b, p, v); },
        &probeReference, attempt.reconstruction, true, verifyStep, pseudoTime.get());
    if (!modelValid) {
      attempt.linearFailure = true;
      attempt.rejectionReason = "MODEL_BASE_RESIDUAL";
      Restore(attempt);
      return attempt;
    }
    SU2_OMP_SAFE_GLOBAL_ACCESS(directionOperator = modelLinearization.get();)
  }
  preconditioner->Build();
  correction = Scalar(0);
  double residual = attempt.linearTarget;
  const auto iterations = legacyFlowControls ? flow.linearIterations : options.linearIterations;
  if (attempt.startup && config->GetKind_Linear_Solver_Inner() == LINEAR_SOLVER_INNER::BCGSTAB)
    attempt.linearIterations =
        linearSolver.BCGSTAB_LinSolver(rhs, correction, Product(*this), Preconditioner(*this), attempt.linearTarget,
                                       iterations, residual, false, config);
  else if (attempt.startup && config->GetKind_Linear_Solver_Inner() == LINEAR_SOLVER_INNER::SMOOTHER)
    attempt.linearIterations = linearSolver.Smoother_LinSolver(rhs, correction, Product(*this), Preconditioner(*this),
                                                               0, iterations, residual, false, config);
  else if (attempt.deflated)
    attempt.linearIterations =
        linearSolver.FGCRODR_LinSolver(rhs, correction, Product(*this), Preconditioner(*this), attempt.linearTarget,
                                       iterations, residual, false, config);
  else
    attempt.linearIterations = linearSolver.FGMRES_LinSolver(rhs, correction, Product(*this), Preconditioner(*this),
                                                             attempt.linearTarget, iterations, residual, false, config);
  attempt.preconditionerIterations = preconditioner->GetWork();
  attempt.reportedLinearResidual = residual;
  MatrixFreeProduct(correction, product);
  attempt.failedProducts = failedProducts;
  double defect = 0, invalid = 0;
  SU2_OMP_FOR_STAT(1024)
  for (auto i = 0ul; i < rhs.GetNElmDomain(); ++i) {
    const double r = product[i] - rhs[i];
    if (!Armijo::IsFinite(r))
      invalid = 1;
    else
      defect += r * r;
  }
  END_SU2_OMP_FOR
  const double anyInvalid = Reduce(invalid, true);
  const double squaredDefect = Reduce(defect);
  attempt.linearDefect = anyInvalid ? std::numeric_limits<double>::infinity() : std::sqrt(squaredDefect) / rhs.norm();
  Perturb(direction, 0);
  Residual();
  if (!Armijo::IsFinite(attempt.linearDefect) || attempt.linearDefect > options.etaMax) {
    attempt.linearFailure = true;
    attempt.rejectionReason = failedProducts ? productFailure : "TRUE_LINEAR_DEFECT";
    Restore(attempt);
    return attempt;
  }
  Endpoint();
  pseudoTime->Apply(direction, pseudoDirection);
  if (options.lineSearch)
    Search(attempt);
  else {
    const double value = Evaluate(1, "TRIAL");
    if (Armijo::IsFinite(value)) {
      attempt.physicalMerit = Merit();
      attempt.search = {Armijo::Status::Accepted, 1, value, 1};
    } else {
      attempt.inadmissibleTrials = !residualValid;
      attempt.rejectionReason = residualValid ? "TRIAL_MERIT" : "TRIAL_ADMISSIBILITY";
      Restore(attempt);
    }
  }
  for (const auto& block : equations->GetBlocks()) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(block.solver->SetIterLinSolver(attempt.linearIterations);
                               block.solver->SetResLinSolver(attempt.linearDefect);)
  }
  return attempt;
}

void CNewtonIntegration::Search(Attempt& attempt) {
  /*--- Acceptance uses the actual endpoint direction and full physical operator, even after a startup solve. ---*/
  double relative = 0, invalid = 0;
  SU2_OMP_FOR_STAT(1024)
  for (auto i = 0ul; i < direction.GetNElmDomain(); ++i) {
    if (!Armijo::IsFinite(direction[i]))
      invalid = 1;
    else
      relative = std::max(relative, std::abs(direction[i]) / (1.0 + std::abs(baseState[i] / scale[i])));
  }
  END_SU2_OMP_FOR
  invalid = Reduce(invalid, true);
  relative = Reduce(relative, true);
  /*--- Reuse the explicit base merit; the verified product and each trial install their own captured state.
   *    Tiny/invalid directions retain the base evaluation, and every failed search rolls back through Armijo. ---*/
  if (!invalid && relative > options.stepTolerance)
    LogEvaluation(0, "BASE", attempt.base, attempt.base);
  else
    Evaluate(0, "BASE");
  if (!invalid && relative > 0) {
    /*--- Physical merit needs its own verified J*p, without subtracting a dominant mass action afterward. ---*/
    CNewtonLinearization::Failure failure;
    const bool valid = linearization->Apply(direction, product, options.pseudoTimeMerit, true, 0, &failure);
    if (!valid) {
      ++attempt.failedProducts;
      attempt.rejectionReason = Name(failure);
    }
    SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid;)
    double slope = 0, norm2 = 0;
    SU2_OMP_FOR_STAT(1024)
    for (auto i = 0ul; i < rhs.GetNElmDomain(); ++i) {
      const double jp = product[i];
      slope -= rhs[i] * jp;
      norm2 += jp * jp;
    }
    END_SU2_OMP_FOR
    attempt.slope = Reduce(slope);
    attempt.productNorm2 = Reduce(norm2);
  } else if (invalid) {
    attempt.slope = std::numeric_limits<double>::quiet_NaN();
    attempt.rejectionReason = "ENDPOINT_DIRECTION";
  }
  const Armijo::Parameters parameters{options.armijo, options.interpolationMin, options.interpolationMax,
                                      options.maxBacktracks};
  attempt.search = Armijo::Search(
      attempt.base, attempt.slope, relative > 0 ? options.stepTolerance / relative : 2,
      [&](double alpha) {
        const double value = Evaluate(alpha, "TRIAL");
        if (!residualValid) ++attempt.inadmissibleTrials;
        return value;
      }, [&] { Restore(attempt); }, parameters);
  if (attempt.search.status != Armijo::Status::Accepted && attempt.failedProducts == 0 && !invalid)
    attempt.rejectionReason = attempt.search.evaluations &&
                                      attempt.inadmissibleTrials == attempt.search.evaluations
                                  ? "TRIAL_ADMISSIBILITY" : Name(attempt.search.status);
  if (attempt.search.status == Armijo::Status::Accepted) {
    attempt.physicalMerit = Merit();
    const double alpha = attempt.search.step;
    attempt.prediction = -alpha * attempt.slope - 0.5 * alpha * alpha * attempt.productNorm2;
    if (attempt.prediction > 0) attempt.agreement = (attempt.base - attempt.search.value) / attempt.prediction;
    for (const auto& block : equations->GetBlocks()) {
      auto* nodes = block.solver->GetNodes();
      SU2_OMP_FOR_STAT(1024)
      for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
        nodes->SetUnderRelaxation(p, alpha * nodes->GetUnderRelaxation(p));
      END_SU2_OMP_FOR
    }
  }
}

void CNewtonIntegration::Log(const Attempt& attempt, const NonlinearGlobalization::Decision& decision) const {
  if (!options.diagnostics || rank != MASTER_NODE || omp_get_thread_num() != 0) return;
  if (!diagnosticHeaderChecked) {
    /*--- Restarted runs must retain the existing CSV schema; inspect it only once per controller. ---*/
    std::ifstream existing(prefix + ".csv");
    std::string header;
    if (std::getline(existing, header))
      detailedDiagnostics = header.find(",rejection_reason,failed_products,inadmissible_trials") != std::string::npos;
    diagnosticHeaderChecked = true;
  }
  std::ofstream out(prefix + ".csv", std::ios::app);
  if (out.tellp() == 0) {
    out << "iteration,attempt,cfl,next_cfl,status,decision,linear_iterations,linear_target,reported_linear_residual,"
           "true_linear_defect,base_merit,merit,step,evaluations,slope,predicted_reduction,model_agreement,"
           "rollback_state_error,rollback_merit_error,physical_merit,pseudo_time_merit,flow_residual_evaluations,"
           "turbulence_residual_evaluations,transition_residual_evaluations,species_residual_evaluations,"
           "startup,deflated,jacobian_reconstruction,tolerance_factor,native_linear_iterations";
    if (detailedDiagnostics) out << ",rejection_reason,failed_products,inadmissible_trials";
    out << '\n';
  }
  out.precision(17);
  const bool resolved = attempt.search.status == Armijo::Status::Accepted && attempt.search.step == 0;
  out << config->GetInnerIter() << ',' << attemptNumber << ',' << cfl << ',' << decision.nextCFL << ','
      << (attempt.residualFailure ? "RESIDUAL_FAILURE" : attempt.linearFailure ? "LINEAR_FAILURE" :
          resolved ? "RESOLVED" : Name(attempt.search.status))
      << ',' << Name(decision.reason) << ','
      << attempt.linearIterations << ',' << attempt.linearTarget << ',' << attempt.reportedLinearResidual << ','
      << attempt.linearDefect << ',' << attempt.base << ',' << attempt.search.value << ',' << attempt.search.step << ','
      << attempt.search.evaluations << ',' << attempt.slope << ',' << attempt.prediction << ',' << attempt.agreement
      << ',' << attempt.rollbackStateError << ',' << attempt.rollbackMeritError << ',' << attempt.physicalMerit << ','
      << options.pseudoTimeMerit;
  for (auto count : equations->GetResidualEvaluations()) out << ',' << count;
  out << ',' << attempt.startup << ',' << attempt.deflated << ',' << attempt.reconstruction << ','
      << attempt.toleranceFactor << ',' << attempt.preconditionerIterations;
  if (detailedDiagnostics)
    out << ',' << attempt.rejectionReason << ',' << attempt.failedProducts << ',' << attempt.inadmissibleTrials;
  out << '\n';
}

void CNewtonIntegration::MultiGrid_Iteration(CGeometry**** geometry, CSolver***** solvers, CNumerics****** numerics,
                                             CConfig** config, unsigned short, unsigned short zone,
                                             unsigned short instance) {
  Iterate(geometry[zone][instance][MESH_0], solvers[zone][instance][MESH_0], numerics[zone][instance][MESH_0],
          config[zone]);
}

void CNewtonIntegration::Iterate(CGeometry* g, CSolver** s, CNumerics*** n, CConfig* c) {
  if (failed) return;
  geometry = g;
  solvers = s;
  numerics = n;
  config = c;
  if (!setup) Setup();
  const auto settings = config->GetEquationSettings();
  reductionPartials.resize(omp_get_max_threads());
  SU2_OMP_PARALLEL { Advance(); }
  END_SU2_OMP_PARALLEL
  config->SetEquationSettings(settings);
}

void CNewtonIntegration::RefreshAfterOuterIteration() {
  if (!setup || failed) return;
  SU2_OMP_PARALLEL {
    const bool valid = equations->EvaluateResiduals(false, true) && RefreshStatistics(true);
    if (valid) {
      const auto settings = config->GetEquationSettings();
      SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_FLOW_SYS);)
      solvers[FLOW_SOL]->Postprocessing(geometry, solvers, config, MESH_0);
      solvers[FLOW_SOL]->Pressure_Forces(geometry, config);
      solvers[FLOW_SOL]->Momentum_Forces(geometry, config);
      solvers[FLOW_SOL]->Friction_Forces(geometry, config);
      SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetEquationSettings(settings);)
    } else {
      SU2_OMP_SAFE_GLOBAL_ACCESS(failed = true; failureMessage = "Newton final-state residual refresh failed";)
    }
  }
  END_SU2_OMP_PARALLEL
}

void CNewtonIntegration::Advance() {
  SetCFL(cfl);
  for (const auto& block : equations->GetBlocks()) block.solver->Set_OldSolution();
  if (!constraintsReady) {
    const bool valid = equations->EvaluateResiduals(true, true);
    SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid; constraintsReady = true;)
  }
  Residual(true);
  for (const auto& block : equations->GetBlocks()) block.solver->Set_OldSolution();
  equations->Capture();
  equations->Pack(baseState);
  UpdateScale();
  /*--- Every retry uses this captured base and scale; rejection must restore both state and merit first. ---*/
  for (unsigned index = 0;; ++index) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(attemptNumber = index;)
    /*--- Packing and scale construction do not change the base residual or its assembled Jacobians. ---*/
    auto attempt = SolveAttempt(index == 0);
    if (attempt.search.status == Armijo::Status::Accepted && !attempt.linearFailure && !RefreshStatistics()) {
      attempt.residualFailure = true;
      attempt.rejectionReason = "ACCEPTED_STATISTICS";
      attempt.search.status = Armijo::Status::InvalidInput;
      Restore(attempt);
      if (!RefreshStatistics()) attempt.rollbackMeritError = std::numeric_limits<double>::infinity();
    }
    SU2_OMP_SAFE_GLOBAL_ACCESS(lastAttempt = attempt;)
    using Reason = NonlinearGlobalization::RecoveryReason;
    if (attempt.search.status == Armijo::Status::Accepted && !attempt.linearFailure) {
      const double next =
          CommitFlow(attempt, NonlinearGlobalization::Adapt(options, cfl, attempt.agreement, attempt.search.step));
      Log(attempt, {Reason::Accepted, next});
      SU2_OMP_SAFE_GLOBAL_ACCESS(
          cfl = next;
          if (attempt.search.step > 0)
            eta = NonlinearGlobalization::Forcing(options, eta, attempt.base, attempt.search.value, attempt.search.step);)
      if (std::find(systems.begin(), systems.end(), RUNTIME_FLOW_SYS) != systems.end()) {
        equations->CommitHistory();
        SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_FLOW_SYS);)
        solvers[FLOW_SOL]->Postprocessing(geometry, solvers, config, MESH_0);
        solvers[FLOW_SOL]->Pressure_Forces(geometry, config);
        solvers[FLOW_SOL]->Momentum_Forces(geometry, config);
        solvers[FLOW_SOL]->Friction_Forces(geometry, config);
      }
      return;
    }
    const bool invalidState = (attempt.residualFailure && attempt.search.evaluations == 0) ||
                              !Armijo::IsFinite(attempt.base) || !Armijo::IsFinite(attempt.rollbackMeritError) ||
                              attempt.rollbackStateError != 0 ||
                              attempt.rollbackMeritError >
                                  options.restoreTolerance * std::max(attempt.base, std::numeric_limits<double>::min());
    const auto decision = NonlinearGlobalization::Reject(options, cfl, index, invalidState);
    Log(attempt, decision);
    if (decision.reason != Reason::Retry) {
      BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
        failed = true;
        std::ostringstream message;
        message << "Newton recovery failed: " << Name(decision.reason) << ", "
                << (attempt.residualFailure ? "RESIDUAL_FAILURE" : attempt.linearFailure ? "LINEAR_FAILURE" : Name(attempt.search.status))
                << " (" << attempt.rejectionReason << ") at iteration "
                << config->GetInnerIter() << ", attempt " << index << ", CFL " << cfl;
        failureMessage = message.str();
      }
      END_SU2_OMP_SAFE_GLOBAL_ACCESS
      return;
    }
    SU2_OMP_SAFE_GLOBAL_ACCESS(cfl = decision.nextCFL;
                               eta = std::max(options.etaMin, std::min(eta, options.linearTolerance) * options.shrink);)
  }
}
#endif
