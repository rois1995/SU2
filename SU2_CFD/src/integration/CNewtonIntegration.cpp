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
#else

namespace {
using Scalar = CNewtonIntegration::Scalar;
class Product final : public CMatrixVectorProduct<Scalar> {
  CNewtonIntegration& integration;

 public:
  explicit Product(CNewtonIntegration& instance) : integration(instance) {}
  void operator()(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const override {
    integration.MatrixFreeProduct(u, v);
  }
};
class Preconditioner final : public CPreconditioner<Scalar> {
  const CNewtonIntegration& integration;

 public:
  explicit Preconditioner(const CNewtonIntegration& instance) : integration(instance) {}
  void operator()(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const override { integration.Precondition(u, v); }
};
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
    // Thread arrival order must not change globalization decisions.
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

void CNewtonIntegration::Setup() {
  const auto error = options.Validate();
  if (!error.empty()) throw std::invalid_argument(error);
  if (config->GetTime_Marching() != TIME_MARCHING::STEADY || config->GetTime_Domain() ||
      config->GetMultizone_Problem() || config->GetnZone() != 1 || config->GetCUDA() || config->GetnMarker_Periodic() ||
      config->GetFinestMesh() != MESH_0)
    throw std::invalid_argument(
        "Common Newton currently requires steady primal single-zone CPU cases without periodic markers");
  equations = std::make_unique<CNewtonEquationSet>(geometry, solvers, numerics, config, systems);
  for (const auto& block : equations->GetBlocks()) {
    const auto time = block.slot == FLOW_SOL ? config->GetKind_TimeIntScheme_Flow()
                                             : block.slot == SPECIES_SOL ? config->GetKind_TimeIntScheme_Species()
                                                                         : config->GetKind_TimeIntScheme_Turb();
    if (time != EULER_IMPLICIT || !(CFLFactor(block) > 0))
      throw std::invalid_argument("Selected Newton equations require implicit discretization and positive CFL factors");
  }
  for (auto runtime : {RUNTIME_FLOW_SYS, RUNTIME_TURB_SYS, RUNTIME_TRANS_SYS, RUNTIME_SPECIES_SYS}) {
    auto* solver = solvers[config->GetContainerPosition(runtime)];
    if (solver) solver->Jacobian.TrackPrescribedRows();
  }
  for (auto* vector : {&scale, &rhs, &correction, &direction, &product, &pseudoDiagonal, &probeReference, &baseState})
    vector->Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), equations->GetnVar(), 0.0);
  free.resize(geometry->GetnPoint() * equations->GetnVar(), true);
  linearSolver.SetxIsZero(true);
  linearization = std::make_unique<CNewtonLinearization>(*equations, geometry, config);
  preconditioner = std::make_unique<CNewtonBlockPreconditioner>(*linearization, geometry, config, lowerTriangular);
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

void CNewtonIntegration::UpdateScale() { equations->BuildScales(scale, probeReference); }

void CNewtonIntegration::Perturb(const CSysVector<Scalar>& vector, double step) {
  equations->SetTrial(vector, step, scale, [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) {
    return IsFree(b, p, v);
  });
}

void CNewtonIntegration::Residual(bool implicit) {
  const bool valid = equations->EvaluateResiduals(implicit);
  SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid;)
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
        if (options.pseudoTimeMerit && step != 0) r += step * pseudoDiagonal(p, j) * direction(p, j);
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
  if (options.diagnostics && rank == MASTER_NODE && omp_get_thread_num() == 0) {
    std::ofstream out(prefix + "_trials.csv", std::ios::app);
    if (out.tellp() == 0) out << "iteration,attempt,evaluation,phase,step,merit,physical_merit\n";
    out.precision(17);
    out << config->GetInnerIter() << ',' << attemptNumber << ',' << trialNumber++ << ',' << phase << ',' << step << ','
        << value << ',' << physical << '\n';
  }
  return value;
}

void CNewtonIntegration::MatrixFreeProduct(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) {
  const bool valid = linearization->Apply(u, v);
  SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid;)
}

void CNewtonIntegration::Precondition(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const {
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
    // Each native limiter sees the same accepted density, before the independently limited endpoints are combined.
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

CNewtonIntegration::Attempt CNewtonIntegration::SolveAttempt() {
  Attempt attempt;
  SU2_OMP_SAFE_GLOBAL_ACCESS(trialNumber = 0;)
  SetCFL(cfl);
  Perturb(direction, 0);
  Residual(true);
  attempt.base = Merit();
  attempt.physicalMerit = attempt.search.value = attempt.base;
  if (!Armijo::IsFinite(attempt.base)) {
    Restore(attempt);
    return attempt;
  }
  // Flow time steps precede scalar time steps even if the requested group has a different packing order.
  for (auto slot : {FLOW_SOL, TURB_SOL, TRANS_SOL, SPECIES_SOL})
    for (const auto& block : equations->GetBlocks()) {
      if (block.slot != slot) continue;
      SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), block.runtime);)
      block.solver->SetTime_Step(geometry, solvers, config, MESH_0, config->GetTimeIter());
    }
  for (const auto& block : equations->GetBlocks()) {
    SU2_OMP_FOR_STAT(1024)
    for (auto p = 0ul; p < geometry->GetnPoint(); ++p)
      for (auto v = 0u; v < block.variables; ++v) {
        pseudoDiagonal(p, block.offset + v) =
            SU2_TYPE::GetValue(geometry->nodes->GetVolume(p) + geometry->nodes->GetPeriodicVolume(p)) /
            std::max(EPS, SU2_TYPE::GetValue(block.solver->GetNodes()->GetDelta_Time(p)));
        free[p * equations->GetnVar() + block.offset + v] = !block.solver->Jacobian.IsPrescribedRow(p, v);
      }
    END_SU2_OMP_FOR
  }
  const bool valid = linearization->SetBase(
      scale, pseudoDiagonal, options.fdStep * std::sqrt(equations->GetnVar() * geometry->GetGlobal_nPointDomain()),
      options.relativeFDStep,
      [&](const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) { return IsFree(b, p, v); },
      &probeReference);
  SU2_OMP_SAFE_GLOBAL_ACCESS(residualValid = valid;)
  attempt.base = Merit();
  attempt.physicalMerit = attempt.search.value = attempt.base;
  if (!Armijo::IsFinite(attempt.base)) {
    Restore(attempt);
    return attempt;
  }
  // Keep the physical right hand side independently of native sign flips and pseudo-time assembly.
  equations->CopyResidual(rhs);
  SU2_OMP_FOR_STAT(1024)
  for (auto i = 0ul; i < rhs.GetLocSize(); ++i) rhs[i] = -rhs[i] / scale[i];
  END_SU2_OMP_FOR
  for (const auto& block : equations->GetBlocks()) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), block.runtime);)
    block.solver->PrepareImplicitIteration(geometry, solvers, config);
  }
  if (attempt.base == 0) {
    attempt.search = {Armijo::Status::Accepted, 0, 0, 0};
    Residual();
    return attempt;
  }
  preconditioner->Build();
  correction = Scalar(0);
  double residual = eta;
  attempt.linearIterations = linearSolver.FGMRES_LinSolver(rhs, correction, Product(*this), Preconditioner(*this), eta,
                                                           options.linearIterations, residual, false, config);
  attempt.reportedLinearResidual = residual;
  MatrixFreeProduct(correction, product);
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
    Restore(attempt);
    return attempt;
  }
  Endpoint();
  if (options.lineSearch)
    Search(attempt);
  else {
    const double value = Evaluate(1, "TRIAL");
    if (Armijo::IsFinite(value)) {
      attempt.physicalMerit = Merit();
      attempt.search = {Armijo::Status::Accepted, 1, value, 1};
    } else
      Restore(attempt);
  }
  for (const auto& block : equations->GetBlocks()) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(block.solver->SetIterLinSolver(attempt.linearIterations);
                               block.solver->SetResLinSolver(attempt.linearDefect);)
  }
  return attempt;
}

void CNewtonIntegration::Search(Attempt& attempt) {
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
  Evaluate(0, "BASE");
  if (!invalid && relative > 0) {
    MatrixFreeProduct(direction, product);
    double slope = 0, norm2 = 0;
    SU2_OMP_FOR_STAT(1024)
    for (auto i = 0ul; i < rhs.GetNElmDomain(); ++i) {
      const double jp = product[i] - (options.pseudoTimeMerit ? 0.0 : pseudoDiagonal[i] * direction[i]);
      slope -= rhs[i] * jp;
      norm2 += jp * jp;
    }
    END_SU2_OMP_FOR
    attempt.slope = Reduce(slope);
    attempt.productNorm2 = Reduce(norm2);
  } else if (invalid)
    attempt.slope = std::numeric_limits<double>::quiet_NaN();
  const Armijo::Parameters parameters{options.armijo, options.interpolationMin, options.interpolationMax,
                                      options.maxBacktracks};
  attempt.search = Armijo::Search(
      attempt.base, attempt.slope, relative > 0 ? options.stepTolerance / relative : 2,
      [&](double alpha) { return Evaluate(alpha, "TRIAL"); }, [&] { Restore(attempt); }, parameters);
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
  std::ofstream out(prefix + ".csv", std::ios::app);
  if (out.tellp() == 0)
    out << "iteration,attempt,cfl,next_cfl,status,decision,linear_iterations,linear_target,reported_linear_residual,"
           "true_linear_defect,base_merit,merit,step,evaluations,slope,predicted_reduction,model_agreement,"
           "rollback_state_error,rollback_merit_error,physical_merit,pseudo_time_merit,flow_residual_evaluations,"
           "turbulence_residual_evaluations,transition_residual_evaluations,species_residual_evaluations\n";
  out.precision(17);
  out << config->GetInnerIter() << ',' << attemptNumber << ',' << cfl << ',' << decision.nextCFL << ','
      << (attempt.linearFailure ? "LINEAR_FAILURE" : Name(attempt.search.status)) << ',' << Name(decision.reason) << ','
      << attempt.linearIterations << ',' << eta << ',' << attempt.reportedLinearResidual << ',' << attempt.linearDefect
      << ',' << attempt.base << ',' << attempt.search.value << ',' << attempt.search.step << ','
      << attempt.search.evaluations << ',' << attempt.slope << ',' << attempt.prediction << ',' << attempt.agreement
      << ',' << attempt.rollbackStateError << ',' << attempt.rollbackMeritError << ',' << attempt.physicalMerit << ','
      << options.pseudoTimeMerit;
  for (auto count : equations->GetResidualEvaluations()) out << ',' << count;
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

void CNewtonIntegration::Advance() {
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
  for (unsigned index = 0;; ++index) {
    SU2_OMP_SAFE_GLOBAL_ACCESS(attemptNumber = index;)
    const auto attempt = SolveAttempt();
    SU2_OMP_SAFE_GLOBAL_ACCESS(lastAttempt = attempt;)
    using Reason = NonlinearGlobalization::RecoveryReason;
    if (attempt.search.status == Armijo::Status::Accepted && !attempt.linearFailure) {
      const double next = NonlinearGlobalization::Adapt(options, cfl, attempt.agreement, attempt.search.step);
      Log(attempt, {Reason::Accepted, next});
      SU2_OMP_SAFE_GLOBAL_ACCESS(
          cfl = next;
          eta = NonlinearGlobalization::Forcing(options, eta, attempt.base, attempt.search.value, attempt.search.step);)
      for (const auto& block : equations->GetBlocks()) block.solver->RefreshResidualStatistics(geometry, config);
      if (std::find(systems.begin(), systems.end(), RUNTIME_FLOW_SYS) != systems.end()) {
        SU2_OMP_SAFE_GLOBAL_ACCESS(config->SetGlobalParam(config->GetKind_Solver(), RUNTIME_FLOW_SYS);)
        solvers[FLOW_SOL]->Postprocessing(geometry, solvers, config, MESH_0);
        solvers[FLOW_SOL]->Pressure_Forces(geometry, config);
        solvers[FLOW_SOL]->Momentum_Forces(geometry, config);
        solvers[FLOW_SOL]->Friction_Forces(geometry, config);
      }
      return;
    }
    const bool invalidState = !Armijo::IsFinite(attempt.base) || !Armijo::IsFinite(attempt.rollbackMeritError) ||
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
                << (attempt.linearFailure ? "LINEAR_FAILURE" : Name(attempt.search.status)) << " at iteration "
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
