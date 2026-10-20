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
      /*--- Judge reconstruction against the target actually requested by adaptive forcing. ---*/
      const double tolerance = std::max(
          attempt.linearTarget, config->GetCFL_Adapt()
                                    ? SU2_TYPE::GetValue(config->GetCFL_AdaptParam(4)) * attempt.toleranceFactor : 0.0);
      const double achieved = attempt.reportedLinearResidual;
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
  if (assembledOperator) {
    if (legacyFlowControls || lowerTriangular)
      throw std::invalid_argument("Assembled Newton operators exclude legacy flow controls and lower FD coupling");
    /*--- The native linear-solver settings apply to the native operator; there is no forcing sequence. ---*/
    const double tolerance = SU2_TYPE::GetValue(config->GetLinear_Solver_Error());
    if (!(tolerance >= 0 && tolerance < 1) || !config->GetLinear_Solver_Iter())
      throw std::invalid_argument(
          "Assembled Newton operators need LINEAR_SOLVER_ERROR in [0,1) and LINEAR_SOLVER_ITER > 0");
    options.linearTolerance = std::max(std::numeric_limits<double>::epsilon(), tolerance);
    options.etaMin = std::min(options.etaMin, options.linearTolerance);
    options.etaMax = std::max(options.etaMax, options.linearTolerance);
    options.linearIterations = std::min(1000ul, config->GetLinear_Solver_Iter());
    options.adaptiveForcing = false;
    /*--- Follow the native CFL controls: fixed without CFL_ADAPT, otherwise its factors and bounds. ---*/
    options.adaptCFL = options.adaptCFL && config->GetCFL_Adapt();
    cflLinearTolerance = std::max(SU2_TYPE::GetValue(config->GetCFL_AdaptParam(4)), tolerance);
    if (config->GetCFL_Adapt()) {
      options.shrink = SU2_TYPE::GetValue(config->GetCFL_AdaptParam(0));
      options.grow = SU2_TYPE::GetValue(config->GetCFL_AdaptParam(1));
      options.cflMin = SU2_TYPE::GetValue(config->GetCFL_AdaptParam(2));
      options.cflMax = SU2_TYPE::GetValue(config->GetCFL_AdaptParam(3));
    }
  }
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
  if (assembledOperator) localCFL.assign(geometry->GetnPoint(), cfl);
  setup = true;
}

void CNewtonIntegration::SetCFL(double value) {
  const bool local = !localCFL.empty();
  for (const auto& block : equations->GetBlocks()) {
    const double factor = CFLFactor(block) / referenceCFLFactor;
    auto* nodes = block.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) nodes->SetLocalCFL(p, (local ? localCFL[p] : value) * factor);
    END_SU2_OMP_FOR
    SU2_OMP_SAFE_GLOBAL_ACCESS(block.solver->SetCFL_Local_Stats(value * factor);)
  }
}

double CNewtonIntegration::ScaleLocalCFL(double factor) {
  double maximum = 0;
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < localCFL.size(); ++p) {
    localCFL[p] = std::max(options.cflMin, std::min(options.cflMax, localCFL[p] * factor));
    if (p < geometry->GetnPointDomain()) maximum = std::max(maximum, localCFL[p]);
  }
  END_SU2_OMP_FOR
  return Reduce(maximum, true);
}

double CNewtonIntegration::AdaptLocalCFL(const Attempt& attempt) {
  /*--- Native rule: the linear solve and the line search decide globally, the under-relaxation of each point's
   *    update (already scaled by the accepted step) decides locally, so hard points keep a small CFL. ---*/
  const bool reduce = attempt.search.step < 1 || !(attempt.linearDefect <= 1.2 * cflLinearTolerance);
  const bool canIncrease = attempt.linearDefect < cflLinearTolerance;
  double maximum = 0;
  SU2_OMP_FOR_STAT(1024)
  for (auto p = 0ul; p < localCFL.size(); ++p) {
    double relaxation = 1;
    for (const auto& block : equations->GetBlocks())
      relaxation = std::min(relaxation, SU2_TYPE::GetValue(block.solver->GetNodes()->GetUnderRelaxation(p)));
    const double factor = NonlinearGlobalization::LocalFactor(options, relaxation, reduce, canIncrease);
    localCFL[p] = std::max(options.cflMin, std::min(options.cflMax, localCFL[p] * factor));
    if (p < geometry->GetnPointDomain()) maximum = std::max(maximum, localCFL[p]);
  }
  END_SU2_OMP_FOR
  return Reduce(maximum, true);
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
  /*--- Most assembled full steps are accepted: assemble their matrices now for the next base. ---*/
  const bool implicit = assembledOperator && step == 1;
  Residual(implicit);
  SU2_OMP_SAFE_GLOBAL_ACCESS(implicitTrial = implicit;)
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
    preconditioner->Solve(u, v, 0, flow.innerTolerance, false);
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
    /*--- Start from no relaxation: solvers that compute it (compressible flow, SA, SST) overwrite this, others
     *    (SA-neg, incompressible flow) would otherwise keep the previous step factor and shrink every later update. ---*/
    auto* nodes = block.solver->GetNodes();
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p) {
      nodes->SetUnderRelaxation(p, 1.0);
      for (auto v = 0u; v < block.variables; ++v)
        block.solver->LinSysSol(p, v) =
            IsFree(block, p, v) ? correction(p, block.offset + v) * scale(p, block.offset + v) : 0;
    }
    END_SU2_OMP_FOR
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), block.runtime);)
    block.solver->CompleteImplicitIteration(geometry, solvers, config);
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
  /*--- Reduced-communication RMS values are local; every rank must agree to idle the group. ---*/
  if (residualTolerance > 0 && config->GetComm_Level() != COMM_FULL)
    resolved = Reduce(resolved ? 0.0 : 1.0, true) == 0;
  if (resolved) {
    attempt.search = {Armijo::Status::Accepted, 0, attempt.base, 0};
    attempt.linearDefect = std::numeric_limits<double>::quiet_NaN();
    return attempt;
  }
  /*--- Assembled solves keep the base residual of the implicit assembly; native preparation changes its sign. ---*/
  if (assembledOperator) equations->CopyResidual(rhs);
  for (const auto& block : equations->GetBlocks()) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), block.runtime);)
    if (block.solver->Jacobian.HasCoefficients())
      block.solver->PrepareImplicitIteration(geometry, solvers, config, false);
  }
  if (assembledOperator) equations->SetResidual(rhs);
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
    /*--- Rollback compares explicit residuals, including failures before SetBase prepares that reference. ---*/
    Perturb(direction, 0);
    Residual();
    attempt.base = Merit();
    attempt.physicalMerit = attempt.search.value = attempt.base;
    Restore(attempt);
    return attempt;
  }
  const double probeStep =
      legacyFlowControls
          ? options.fdStep * std::max(1.0, baseState.norm() / std::sqrt(geometry->GetGlobal_nPointDomain()))
          : options.fdStep * std::sqrt(equations->GetnVar() * geometry->GetGlobal_nPointDomain());
  /*--- Verify every group's products: scalar and first-order residuals are not smooth everywhere either
   *     (e.g. SA-neg at zero), and an unverified product lets Krylov converge on a wrong action.
   *     Assembled operators use no finite-difference products, so the live base residual serves as reference. ---*/
  const bool verifyStep = !assembledOperator;
  const bool valid = linearization->SetBase(
      scale, pseudoTime->GetDiagonal(), probeStep, options.relativeFDStep,
      [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) { return IsFree(b, p, v); },
      &probeReference, 1.0, legacyFlowControls, verifyStep, pseudoTime.get(), !assembledOperator);
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
  /*--- Relax the direction solve only within the unchanged true-defect acceptance limit. ---*/
  attempt.linearTarget = std::min(attempt.linearTarget, options.etaMax);
  if (attempt.startup) attempt.linearTarget = flow.linearTolerance;
  if (attempt.base == 0) {
    attempt.search = {Armijo::Status::Accepted, 0, 0, 0};
    attempt.linearDefect = std::numeric_limits<double>::quiet_NaN();
    Residual();
    return attempt;
  }
  SU2_OMP_SAFE_GLOBAL_ACCESS(directionOperator = linearization.get();
                             nativeDirection = attempt.startup || assembledOperator;)
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
  if (assembledOperator && config->GetKind_Linear_Solver() == BCGSTAB)
    attempt.linearIterations =
        linearSolver.BCGSTAB_LinSolver(rhs, correction, Product(*this), Preconditioner(*this), attempt.linearTarget,
                                       iterations, residual, false, config);
  else if (attempt.startup && config->GetKind_Linear_Solver_Inner() == LINEAR_SOLVER_INNER::BCGSTAB)
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
  attempt.failedProducts = failedProducts + preconditioner->GetFailedProducts();
  /*--- An assembled direction is judged against the operator actually solved: the solver's relative residual
   *    already measures it (as for the native solver), unless it is not a reduction. ---*/
  if (assembledOperator && Armijo::IsFinite(residual) && residual < 1) {
    attempt.linearDefect = residual;
  } else {
    MatrixFreeProduct(correction, product, assembledOperator);
    attempt.failedProducts = failedProducts + preconditioner->GetFailedProducts();
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
    attempt.linearDefect =
        anyInvalid ? std::numeric_limits<double>::infinity() : std::sqrt(squaredDefect) / rhs.norm();
  }
  /*--- The assembled operator is approximate: any progress is admissible, the line search judges the step. ---*/
  const bool defectTooLarge = nativeFallback      ? false
                              : assembledOperator ? attempt.linearDefect >= 1
                                                  : attempt.linearDefect > options.etaMax;
  if (!Armijo::IsFinite(attempt.linearDefect) || defectTooLarge) {
    attempt.linearFailure = true;
    attempt.rejectionReason = preconditioner->GetFailedProducts() ? Name(preconditioner->GetFailure())
                                : failedProducts ? productFailure : "TRUE_LINEAR_DEFECT";
    Restore(attempt);
    return attempt;
  }
  /*--- Rejected directions already refresh the captured base in Restore; prepare accepted native endpoints here.
   *     Assembled solves use no residual probes, so the base evaluated by SetBase is still live. ---*/
  if (!assembledOperator) {
    Perturb(direction, 0);
    Residual();
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
  /*--- Defect correction with an approximate operator is often not monotone: compare assembled trials with the
   *    largest of the last accepted base merits (Grippo-Lampariello-Lucidi). Rollback still restores this base. ---*/
  double reference = attempt.base;
  if (assembledOperator)
    for (const double merit : recentMerits) reference = std::max(reference, merit);
  const bool tiny = !invalid && relative <= options.stepTolerance;
  if (nativeFallback && !invalid) {
    /*--- Exhausted assembled recovery: the native solver would take this full step unchecked. Take it if the
     *    state stays admissible, so the assembled path is never less robust than the native one. ---*/
    const double value = Evaluate(1, "TRIAL");
    if (residualValid && Armijo::IsFinite(value)) {
      attempt.search = {Armijo::Status::Accepted, 1, value, 1};
      attempt.rejectionReason = "NATIVE_FALLBACK";
      attempt.physicalMerit = Merit();
    } else {
      attempt.search = {Armijo::Status::InvalidInput, 0, attempt.base, 1};
      attempt.rejectionReason = "NATIVE_FALLBACK";
      Restore(attempt);
    }
    return;
  }
  if (tiny) {
    /*--- Below the step tolerance a line search cannot resolve the merit change. Try the full step once:
     *    keep it if the merit does not rise above the reference, else keep the base. Neither is a failed
     *    search, and CFL is not cut, which would only shrink the next direction. ---*/
    const double value = relative > 0 ? Evaluate(1, "TRIAL") : std::numeric_limits<double>::infinity();
    attempt.stationary = true;
    if (residualValid && Armijo::IsFinite(value) && value <= reference) {
      attempt.search = {Armijo::Status::Accepted, 1, value, 1};
      attempt.rejectionReason = "TINY_STEP";
    } else {
      if (relative > 0) Restore(attempt);
      attempt.search = {Armijo::Status::Accepted, 0, attempt.base, 0};
      attempt.rejectionReason = "STATIONARY";
      return;
    }
  } else if (!invalid && relative > 0 && assembledOperator) {
    /*--- The direction solves A d = -F, A = J_a+D, to relative defect eta. A minimal-residual solve leaves
     *    r = F + A d orthogonal to A d, so F.r = ||r||^2 and the merit model m(a) = 0.5||F + a A d||^2 has
     *    slope -2 m0 (1-eta^2) and ||A d||^2 = 2 m0 (1-eta^2). J_a only approximates J: Armijo then asks for
     *    a real decrease, the model agreement steers CFL and recovery handles the rest. ---*/
    const double reduction = 1 - std::pow(std::min(1.0, attempt.linearDefect), 2);
    attempt.slope = -2 * attempt.base * reduction;
    attempt.productNorm2 = 2 * attempt.base * reduction;
  } else if (!invalid && relative > 0) {
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
  /*--- The model slope of an approximate operator overstates the decrease: assembled steps need simple decrease. ---*/
  const Armijo::Parameters parameters{assembledOperator ? 1e-12 : options.armijo, options.interpolationMin,
                                      options.interpolationMax, options.maxBacktracks};
  if (!tiny)
    attempt.search = Armijo::Search(
        reference, attempt.slope, relative > 0 ? options.stepTolerance / relative : 2,
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
  try {
    Iterate(geometry[zone][instance][MESH_0], solvers[zone][instance][MESH_0], numerics[zone][instance][MESH_0],
            config[zone]);
  } catch (const std::invalid_argument& error) {
    SU2_MPI::Error(error.what(), CURRENT_FUNCTION);
  }
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
    /*--- After a coupled update of every equation, the accepted trial residuals are still current. ---*/
    const bool valid = (liveResidual || equations->EvaluateResiduals(false, true)) && RefreshStatistics(true);
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
  /*--- The accepted assembled trial is still the live state, residual and spatial assembly if nothing has evaluated
   *    since, and no fixed-CL feedback moves the boundary state. The time term is added later from the new CFL. ---*/
  const bool reuse = reuseAssembly && constraintsReady && !config->GetFixed_CL_Mode() &&
                     assemblyStamp == EvaluationCount();
  SU2_OMP_SAFE_GLOBAL_ACCESS(liveResidual = false; reuseAssembly = false;)
  SetCFL(cfl);
  /*--- An initial or native state can still carry wall-normal momentum; the base must satisfy v.n=0.
   *    A reused trial state is already projected. ---*/
  if (!reuse) equations->ProjectSymmetryWalls();
  for (const auto& block : equations->GetBlocks()) block.solver->Set_OldSolution();
  if (config->GetFixed_CL_Mode() && IsNewtonEquation(RUNTIME_FLOW_SYS)) {
    /*--- Apply the pending outer feedback once; trial evaluations keep the new angle fixed. ---*/
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_FLOW_SYS);)
    solvers[FLOW_SOL]->Preprocessing(geometry, solvers, config, MESH_0, NO_RK_ITER, RUNTIME_FLOW_SYS, true);
  }
  if (!constraintsReady) {
    const bool valid = equations->EvaluateResiduals(true, true);
    SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid; constraintsReady = true;)
  }
  if (reuse) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = true;)
  } else {
    Residual(true);
  }
  for (const auto& block : equations->GetBlocks()) block.solver->Set_OldSolution();
  equations->Capture();
  struct FixedFlowGuard {
    CNewtonEquationSet& equations;
    explicit FixedFlowGuard(CNewtonEquationSet& e) : equations(e) { equations.BeginFixedFlowReuse(); }
    ~FixedFlowGuard() { equations.EndFixedFlowReuse(); }
  } fixedFlow(*equations);
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
      const double adapted =
          assembledOperator
              ? (attempt.stationary ? ScaleLocalCFL(options.adaptCFL ? options.grow : 1) : AdaptLocalCFL(attempt))
          : attempt.stationary ? NonlinearGlobalization::Stationary(options, cfl)
                               : NonlinearGlobalization::Adapt(options, cfl, attempt.agreement, attempt.search.step);
      const double next = CommitFlow(attempt, adapted);
      Log(attempt, {Reason::Accepted, next});
      SU2_OMP_SAFE_GLOBAL_ACCESS(nativeFallback = false;)
      if (assembledOperator) {
        BEGIN_SU2_OMP_SAFE_GLOBAL_ACCESS {
          recentMerits.push_back(attempt.base);
          if (recentMerits.size() > 10) recentMerits.pop_front();
        }
        END_SU2_OMP_SAFE_GLOBAL_ACCESS
      }
      SU2_OMP_SAFE_GLOBAL_ACCESS(
          cfl = next;
          if (attempt.search.step > 0)
            eta = NonlinearGlobalization::Forcing(options, eta, attempt.base, attempt.search.value, attempt.search.step);)
      /*--- Every update leaves the residual of its accepted state (trial, restored base or resolved base). ---*/
      bool live = true;
      for (auto runtime : {RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS, RUNTIME_TRANS_SYS, RUNTIME_SPECIES_SYS})
        if (solvers[config->GetContainerPosition(runtime)] && !IsNewtonEquation(runtime)) live = false;
      if (std::find(systems.begin(), systems.end(), RUNTIME_FLOW_SYS) != systems.end()) {
        /*--- Changed reconstruction history changes the residual of the same state. ---*/
        if (equations->CommitHistory()) live = false;
        SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_FLOW_SYS);)
        solvers[FLOW_SOL]->Postprocessing(geometry, solvers, config, MESH_0);
        solvers[FLOW_SOL]->Pressure_Forces(geometry, config);
        solvers[FLOW_SOL]->Momentum_Forces(geometry, config);
        solvers[FLOW_SOL]->Friction_Forces(geometry, config);
      }
      SU2_OMP_SAFE_GLOBAL_ACCESS(liveResidual = live;
                                 reuseAssembly = live && implicitTrial && attempt.search.step == 1;
                                 assemblyStamp = EvaluationCount();)
      return;
    }
    const bool invalidState = (attempt.residualFailure && attempt.search.evaluations == 0) ||
                              !Armijo::IsFinite(attempt.base) || !Armijo::IsFinite(attempt.rollbackMeritError) ||
                              attempt.rollbackStateError != 0 ||
                              attempt.rollbackMeritError >
                                  options.restoreTolerance * std::max(attempt.base, std::numeric_limits<double>::min());
    const auto decision = NonlinearGlobalization::Reject(options, cfl, index, invalidState);
    Log(attempt, decision);
    if (decision.reason != Reason::Retry && assembledOperator && !nativeFallback && !invalidState &&
        decision.reason != Reason::Disabled) {
      SU2_OMP_SAFE_GLOBAL_ACCESS(nativeFallback = true;)
      continue;
    }
    SU2_OMP_SAFE_GLOBAL_ACCESS(nativeFallback = false;)
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
    /*--- Assembled retries scale the whole local CFL field; the controller CFL is its maximum. ---*/
    const double next = assembledOperator ? ScaleLocalCFL(decision.nextCFL / cfl) : decision.nextCFL;
    SU2_OMP_SAFE_GLOBAL_ACCESS(cfl = next;
                               eta = std::max(options.etaMin, std::min(eta, options.linearTolerance) * options.shrink);)
  }
}
#endif
