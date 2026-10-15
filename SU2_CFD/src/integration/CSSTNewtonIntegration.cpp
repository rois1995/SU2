/*!
 * \file CSSTNewtonIntegration.cpp
 * \brief Globalized inexact Newton iteration for the segregated SST equations.
 * \author rois1995
 */
#include "../../include/integration/CSSTNewtonIntegration.hpp"
#include "../../../Common/include/linear_algebra/CMatrixVectorProduct.hpp"
#include <fstream>
#include <sstream>

#if defined(CODI_REVERSE_TYPE) || defined(CODI_FORWARD_TYPE)
void CSSTNewtonIntegration::MultiGrid_Iteration(CGeometry****, CSolver*****, CNumerics******, CConfig**, unsigned short,
                                                unsigned short, unsigned short) {
  SU2_MPI::Error("SST_NEWTON requires a primal executable", CURRENT_FUNCTION);
}
#else

namespace {
using Scalar = CSSTNewtonIntegration::Scalar;
class Product final : public CMatrixVectorProduct<Scalar> {
  CSSTNewtonIntegration& integration;

 public:
  explicit Product(CSSTNewtonIntegration& instance) : integration(instance) {}
  void operator()(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const override {
    integration.MatrixFreeProduct(u, v);
  }
};
class Preconditioner final : public CPreconditioner<Scalar> {
  const CSSTNewtonIntegration& integration;

 public:
  explicit Preconditioner(const CSSTNewtonIntegration& instance) : integration(instance) {}
  void operator()(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const override { integration.Precondition(u, v); }
};
double Sum(double local) {
  double global = 0;
  SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
  return global;
}
double Maximum(double local) {
  double global = 0;
  SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  return global;
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

void CSSTNewtonIntegration::Setup() {
  options = config->GetSSTNewtonOptions().Passive([](const su2double& x) { return SU2_TYPE::GetValue(x); });
  for (auto* vector : {&scale, &rhs, &correction, &direction, &product})
    vector->Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), 2, 0.0);
  precIn.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), 2, 0.0);
  precOut.Initialize(geometry->GetnPoint(), geometry->GetnPointDomain(), 2, 0.0);
  oldRelaxation.resize(geometry->GetnPoint());
  const auto kind = static_cast<ENUM_LINEAR_SOLVER_PREC>(config->GetKind_Linear_Solver_Prec());
  preconditioner.reset(CPreconditioner<MixedScalar>::Create(kind, solvers[TURB_SOL]->Jacobian, geometry, config));
  linearSolver.SetxIsZero(true);
  equations = std::make_unique<CNewtonEquationSet>(geometry, solvers, numerics, config,
                                                  std::vector<unsigned short>{RUNTIME_TURB_SYS});
  cfl = std::max(options.cflMin,
                 std::min(options.cflMax, SU2_TYPE::GetValue(config->GetCFL(MESH_0) * config->GetCFLRedCoeff_Turb())));
  eta = options.linearTolerance;
  setup = true;
}

void CSSTNewtonIntegration::SetCFL(double value) {
  auto* nodes = solvers[TURB_SOL]->GetNodes();
  for (unsigned long p = 0; p < geometry->GetnPoint(); ++p) nodes->SetLocalCFL(p, value);
  solvers[TURB_SOL]->SetCFL_Local_Stats(value);
}

void CSSTNewtonIntegration::UpdateScale() {
  const auto* reference = solvers[TURB_SOL]->GetLimiterReference();
  for (unsigned long p = 0; p < geometry->GetnPoint(); ++p)
    for (unsigned short v = 0; v < 2; ++v) {
      const double field = SU2_TYPE::GetValue(solvers[TURB_SOL]->GetNodes()->GetSolution_Old(p)[v]);
      const double ref = SU2_TYPE::GetValue(reference[v]);
      scale(p, v) =
          SU2_TYPE::GetValue(solvers[FLOW_SOL]->GetNodes()->GetDensity(p)) * std::max(std::abs(field), std::abs(ref));
      if (scale(p, v) == 0) scale(p, v) = 1.0;
    }
}

void CSSTNewtonIntegration::Perturb(const CSysVector<Scalar>& vector, double step) {
  equations->SetTrial(vector, step, scale, [&](const CNewtonEquationSet::Block&, unsigned long p, unsigned short) {
    return !geometry->nodes->GetSolidBoundary(p);
  });
}

void CSSTNewtonIntegration::Residual(bool implicit) { residualValid = equations->EvaluateResiduals(implicit); }

double CSSTNewtonIntegration::Merit(double step) const {
  if (!residualValid) return std::numeric_limits<double>::infinity();
  double value = 0, invalid = 0;
  for (unsigned long p = 0; p < geometry->GetnPointDomain(); ++p)
    for (unsigned short v = 0; v < 2; ++v) {
      double r = SU2_TYPE::GetValue(solvers[TURB_SOL]->LinSysRes(p, v)) / scale(p, v);
      if (options.pseudoTimeMerit && step != 0) {
        const double diagonal =
            SU2_TYPE::GetValue(geometry->nodes->GetVolume(p) + geometry->nodes->GetPeriodicVolume(p)) /
            std::max(EPS, SU2_TYPE::GetValue(solvers[TURB_SOL]->GetNodes()->GetDelta_Time(p)));
        r += step * diagonal * direction(p, v);
      }
      if (!Armijo::IsFinite(r))
        invalid = 1;
      else
        value += 0.5 * r * r;
    }
  if (!Armijo::IsFinite(value)) invalid = 1;
  const double anyInvalid = Maximum(invalid);
  const double global = Sum(value);
  return anyInvalid ? std::numeric_limits<double>::infinity() : global;
}

double CSSTNewtonIntegration::Evaluate(double step, const char* phase) {
  Perturb(direction, step);
  Residual();
  const double physical = Merit();
  const double value = Armijo::IsFinite(physical) && options.pseudoTimeMerit && step != 0 ? Merit(step) : physical;
  if (options.diagnostics && rank == MASTER_NODE) {
    std::ofstream out("sst_newton_trials.csv", std::ios::app);
    if (out.tellp() == 0) out << "iteration,attempt,evaluation,phase,step,merit,physical_merit\n";
    out.precision(17);
    out << config->GetInnerIter() << ',' << attemptNumber << ',' << trialNumber++ << ',' << phase << ',' << step << ','
        << value << ',' << physical << '\n';
  }
  return value;
}

void CSSTNewtonIntegration::MatrixFreeProduct(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) {
  const double norm = u.norm();
  if (norm == 0) {
    v = Scalar(0);
    return;
  }
  double relative = 0;
  const auto* ref = solvers[TURB_SOL]->GetLimiterReference();
  for (unsigned long p = 0; p < geometry->GetnPointDomain(); ++p) {
    if (geometry->nodes->GetSolidBoundary(p)) continue;
    const double rho = SU2_TYPE::GetValue(solvers[FLOW_SOL]->GetNodes()->GetDensity(p));
    for (unsigned short j = 0; j < 2; ++j) {
      const double field = std::max(std::abs(SU2_TYPE::GetValue(solvers[TURB_SOL]->GetNodes()->GetSolution_Old(p)[j])),
                                    std::abs(SU2_TYPE::GetValue(ref[j])) * 1e-12);
      if (field > 0) relative = std::max(relative, std::abs(u(p, j) * scale(p, j) / (rho * field)));
    }
  }
  relative = Maximum(relative);
  double h = options.fdStep * std::sqrt(2.0 * geometry->GetGlobal_nPointDomain()) / norm;
  if (relative > 0) h = std::min(h, options.relativeFDStep / relative);
  if (!(h > 0) || !Armijo::IsFinite(h)) {
    v = Scalar(std::numeric_limits<double>::quiet_NaN());
    return;
  }
  Perturb(u, h);
  Residual();
  if (!residualValid) {
    v = Scalar(std::numeric_limits<double>::quiet_NaN());
    return;
  }
  for (unsigned long p = 0; p < geometry->GetnPointDomain(); ++p) {
    const double diagonal = SU2_TYPE::GetValue(geometry->nodes->GetVolume(p) + geometry->nodes->GetPeriodicVolume(p)) /
                            std::max(EPS, SU2_TYPE::GetValue(solvers[TURB_SOL]->GetNodes()->GetDelta_Time(p)));
    for (unsigned short j = 0; j < 2; ++j) {
      const double r = SU2_TYPE::GetValue(solvers[TURB_SOL]->LinSysRes(p, j)) / scale(p, j);
      v(p, j) = (r + rhs(p, j)) / h + diagonal * u(p, j);
      if (geometry->nodes->GetSolidBoundary(p)) v(p, j) = (1.0 + diagonal) * u(p, j);
    }
  }
  CSysMatrixComms::Initiate(v, geometry, config);
  CSysMatrixComms::Complete(v, geometry, config);
}

void CSSTNewtonIntegration::Precondition(const CSysVector<Scalar>& u, CSysVector<Scalar>& v) const {
  if (!preconditioner) {
    v = u;
    return;
  }
  for (unsigned long i = 0; i < u.GetLocSize(); ++i) precIn[i] = u[i] * scale[i];
  (*preconditioner)(precIn, precOut);
  for (unsigned long i = 0; i < u.GetLocSize(); ++i) v[i] = precOut[i] / scale[i];
}

void CSSTNewtonIntegration::Restore(Attempt& attempt) {
  const double restored = Evaluate(0, "ROLLBACK");
  attempt.rollbackMeritError = std::abs(restored - attempt.base);
  double error = 0;
  auto* nodes = solvers[TURB_SOL]->GetNodes();
  for (unsigned long p = 0; p < geometry->GetnPoint(); ++p) {
    nodes->SetUnderRelaxation(p, oldRelaxation[p]);
    for (unsigned short v = 0; v < 2; ++v)
      error = std::max(error, std::abs(SU2_TYPE::GetValue(nodes->GetSolution(p, v) - nodes->GetSolution_Old(p)[v])));
  }
  attempt.rollbackStateError = Maximum(error);
}

CSSTNewtonIntegration::Attempt CSSTNewtonIntegration::SolveAttempt() {
  auto* solver = solvers[TURB_SOL];
  Attempt attempt;
  trialNumber = 0;
  SetCFL(cfl);
  Perturb(direction, 0);
  Residual(true);
  attempt.base = Merit();
  attempt.physicalMerit = attempt.base;
  attempt.search.value = attempt.base;
  if (!Armijo::IsFinite(attempt.base)) {
    Restore(attempt);
    return attempt;
  }
  solver->SetTime_Step(geometry, solvers, config, MESH_0, config->GetTimeIter());
  solver->PrepareImplicitIteration(geometry, solvers, config);
  for (unsigned long i = 0; i < rhs.GetNElmDomain(); ++i) rhs[i] = SU2_TYPE::GetValue(solver->LinSysRes[i]) / scale[i];
  if (attempt.base == 0) {
    attempt.search = {Armijo::Status::Accepted, 0, 0, 0};
    Residual();
    return attempt;
  }
  if (preconditioner) preconditioner->Build();
  correction = Scalar(0);
  double residual = eta;
  attempt.linearIterations = linearSolver.FGMRES_LinSolver(rhs, correction, Product(*this), Preconditioner(*this), eta,
                                                           options.linearIterations, residual, false, config);
  attempt.reportedLinearResidual = residual;
  MatrixFreeProduct(correction, product);
  double defect = 0, invalid = 0;
  for (unsigned long i = 0; i < rhs.GetNElmDomain(); ++i) {
    const double r = product[i] - rhs[i];
    if (!Armijo::IsFinite(r))
      invalid = 1;
    else
      defect += r * r;
  }
  attempt.linearDefect =
      Maximum(invalid) ? std::numeric_limits<double>::infinity() : std::sqrt(Sum(defect)) / rhs.norm();
  Perturb(direction, 0);
  Residual();
  if (!Armijo::IsFinite(attempt.linearDefect) || attempt.linearDefect > options.etaMax) {
    attempt.linearFailure = true;
    Restore(attempt);
    return attempt;
  }
  for (unsigned long i = 0; i < correction.GetLocSize(); ++i) solver->LinSysSol[i] = correction[i] * scale[i];
  if (options.lineSearch)
    Search(attempt);
  else {
    solver->CompleteImplicitIteration(geometry, solvers, config);
    Residual();
    const double value = Merit();
    if (Armijo::IsFinite(value)) {
      attempt.physicalMerit = value;
      attempt.search = {Armijo::Status::Accepted, 1, value, 1};
    } else
      Restore(attempt);
  }
  solver->SetIterLinSolver(attempt.linearIterations);
  solver->SetResLinSolver(attempt.linearDefect);
  return attempt;
}

void CSSTNewtonIntegration::Search(Attempt& attempt) {
  auto* solver = solvers[TURB_SOL];
  auto* nodes = solver->GetNodes();
  solver->CompleteImplicitIteration(geometry, solvers, config);
  double relative = 0, invalid = 0;
  for (unsigned long p = 0; p < geometry->GetnPoint(); ++p) {
    const double rho = SU2_TYPE::GetValue(solvers[FLOW_SOL]->GetNodes()->GetDensity(p));
    for (unsigned short v = 0; v < 2; ++v) {
      const double old = SU2_TYPE::GetValue(nodes->GetSolution_Old(p)[v]);
      direction(p, v) =
          geometry->nodes->GetSolidBoundary(p)
              ? 0.0
              : rho * SU2_TYPE::GetValue(nodes->GetSolution(p, v) - nodes->GetSolution_Old(p)[v]) / scale(p, v);
      if (!Armijo::IsFinite(direction(p, v)))
        invalid = 1;
      else if (p < geometry->GetnPointDomain())
        relative = std::max(relative, std::abs(direction(p, v)) / (1.0 + std::abs(rho * old / scale(p, v))));
    }
  }
  invalid = Maximum(invalid);
  relative = Maximum(relative);
  Evaluate(0, "BASE");
  if (!invalid && relative > 0) {
    MatrixFreeProduct(direction, product);
    double slope = 0, norm2 = 0;
    for (unsigned long p = 0; p < geometry->GetnPointDomain(); ++p) {
      const double diagonal =
          SU2_TYPE::GetValue(geometry->nodes->GetVolume(p) + geometry->nodes->GetPeriodicVolume(p)) /
          std::max(EPS, SU2_TYPE::GetValue(nodes->GetDelta_Time(p)));
      for (unsigned short v = 0; v < 2; ++v) {
        const double jp = product(p, v) - (options.pseudoTimeMerit ? 0.0 : diagonal * direction(p, v));
        slope -= rhs(p, v) * jp;
        norm2 += jp * jp;
      }
    }
    attempt.slope = Sum(slope);
    attempt.productNorm2 = Sum(norm2);
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
    for (unsigned long p = 0; p < geometry->GetnPoint(); ++p)
      nodes->SetUnderRelaxation(p, alpha * nodes->GetUnderRelaxation(p));
  }
}

void CSSTNewtonIntegration::Log(const Attempt& attempt, const NonlinearGlobalization::Decision& decision) const {
  if (!options.diagnostics || rank != MASTER_NODE) return;
  std::ofstream out("sst_newton.csv", std::ios::app);
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

void CSSTNewtonIntegration::MultiGrid_Iteration(CGeometry**** geometry_, CSolver***** solvers_,
                                                CNumerics****** numerics_, CConfig** config_, unsigned short,
                                                unsigned short zone, unsigned short instance) {
  if (failed) return;
  config = config_[zone];
  geometry = geometry_[zone][instance][MESH_0];
  solvers = solvers_[zone][instance][MESH_0];
  numerics = numerics_[zone][instance][MESH_0];
  if (!setup) Setup();
  // Strong wall values must precede the gradients used in the differentiated base residual.
  solvers[TURB_SOL]->Set_OldSolution();
  Residual();
  solvers[TURB_SOL]->InitiateComms(geometry, config, MPI_QUANTITIES::SOLUTION);
  solvers[TURB_SOL]->CompleteComms(geometry, config, MPI_QUANTITIES::SOLUTION);
  solvers[TURB_SOL]->Set_OldSolution();
  equations->Capture();
  UpdateScale();
  for (unsigned long p = 0; p < geometry->GetnPoint(); ++p)
    oldRelaxation[p] = SU2_TYPE::GetValue(solvers[TURB_SOL]->GetNodes()->GetUnderRelaxation(p));
  for (attemptNumber = 0;; ++attemptNumber) {
    const auto attempt = SolveAttempt();
    using Reason = NonlinearGlobalization::RecoveryReason;
    if (attempt.search.status == Armijo::Status::Accepted && !attempt.linearFailure) {
      const double next = NonlinearGlobalization::Adapt(options, cfl, attempt.agreement, attempt.search.step);
      Log(attempt, {Reason::Accepted, next});
      cfl = next;
      eta = NonlinearGlobalization::Forcing(options, eta, attempt.base, attempt.search.value, attempt.search.step);
      return;
    }
    const bool invalidState = !Armijo::IsFinite(attempt.base) || !Armijo::IsFinite(attempt.rollbackMeritError) ||
                              attempt.rollbackStateError != 0 ||
                              attempt.rollbackMeritError >
                                  options.restoreTolerance * std::max(attempt.base, std::numeric_limits<double>::min());
    const auto decision = NonlinearGlobalization::Reject(options, cfl, attemptNumber, invalidState);
    Log(attempt, decision);
    if (decision.reason != Reason::Retry) {
      failed = true;
      std::ostringstream message;
      message << "SST Newton recovery failed: " << Name(decision.reason) << ", "
              << (attempt.linearFailure ? "LINEAR_FAILURE" : Name(attempt.search.status)) << " at iteration "
              << config->GetInnerIter() << ", attempt " << attemptNumber << ", CFL " << cfl;
      failureMessage = message.str();
      return;
    }
    cfl = decision.nextCFL;
    eta = std::max(options.etaMin, std::min(eta, options.linearTolerance) * options.shrink);
  }
}
#endif
