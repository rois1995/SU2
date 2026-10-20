/*!
 * \file CNewtonIntegration.hpp
 * \brief Globalized Newton-Krylov integration of selected equation groups.
 * \author SU2 Contributors
 */
#pragma once

#include "CIntegration.hpp"
#include "CNewtonLinearization.hpp"
#include "../../../Common/include/linear_algebra/CSysSolve.hpp"
#include <deque>
#include <numeric>
#include <memory>

/*!
 * \class CNewtonIntegration
 * \brief Own the globalized Newton iteration for one selected equation group.
 * \details Segregated groups have separate owners; a coupled group shares one owner and differentiates
 * all cross responses. Each attempt freezes its base/scales, solves a pseudo-transient system, checks the
 * true linear defect and searches using the physical residual. Rejection restores the complete base before
 * a bounded CFL retry. Existing NEWTON_KRYLOV flow controls use the same acceptance and recovery checks.
 */
class CNewtonIntegration final : public CIntegration {
  std::vector<unsigned short> systems;
  NonlinearGlobalization::Options<> options;
  bool lowerTriangular, legacyFlowControls, assembledOperator;
  std::string prefix;
  std::shared_ptr<CNewtonEquationSet::ResidualWorkspace> residualWorkspace;
  double residualTolerance = 0;

 public:
  CNewtonIntegration() : CNewtonIntegration({RUNTIME_FLOW_SYS}, {}, false, "newton", true) {}
  /*!
   * \brief Select the equation order, globalization policy and native block preconditioner.
   * \param[in] selected - Runtime systems advanced by this owner, in packed block order.
   * \param[in] settings - Validated line-search, linear-solve and bounded recovery controls.
   * \param[in] lower - Subtract matrix-free lower cross blocks in the preconditioner.
   * \param[in] diagnostics - Prefix used by iteration and recovery records.
   * \param[in] flowControls - Honor existing flow startup, reconstruction and inner-solve controls.
   * \param[in] workspace - Optional evaluation scratch shared by sequential equation groups.
   * \param[in] assembled - Solve directions with the native assembled Jacobians instead of finite differences.
   */
  CNewtonIntegration(std::vector<unsigned short> selected, NonlinearGlobalization::Options<> settings,
                     bool lower = false, std::string diagnostics = "newton", bool flowControls = false,
                     std::shared_ptr<CNewtonEquationSet::ResidualWorkspace> workspace = {}, bool assembled = false)
      : systems(std::move(selected)),
        options(settings),
        lowerTriangular(lower),
        legacyFlowControls(flowControls),
        assembledOperator(assembled),
        prefix(std::move(diagnostics)), residualWorkspace(std::move(workspace)) {}
  void MultiGrid_Iteration(CGeometry****, CSolver*****, CNumerics******, CConfig**, unsigned short, unsigned short,
                           unsigned short) override;
  void SingleGrid_Iteration(CGeometry**** g, CSolver***** s, CNumerics****** n, CConfig** c, unsigned short runtime,
                            unsigned short zone, unsigned short instance) override {
    MultiGrid_Iteration(g, s, n, c, runtime, zone, instance);
  }
  bool IsNewtonEquation(unsigned short runtime) const override {
    return std::find(systems.begin(), systems.end(), runtime) != systems.end();
  }
  void RefreshAfterOuterIteration() override;
  void SetResidualTolerance(double tolerance) override {
    residualTolerance = Armijo::IsFinite(tolerance) && tolerance > 0 ? tolerance : 0;
  }
#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
  using Scalar = passivedouble;
  /*! \brief Outcome of one solve/search attempt, including independent defect and rollback checks. */
  struct Attempt {
    Armijo::Result search{Armijo::Status::InvalidInput, 0, 0, 0};
    double base = 0, physicalMerit = 0, slope = 0, productNorm2 = 0, prediction = 0;
    double agreement = std::numeric_limits<double>::quiet_NaN();
    double linearDefect = 0, reportedLinearResidual = 0, rollbackMeritError = 0, rollbackStateError = 0;
    unsigned linearIterations = 0;
    const char* rejectionReason = "NONE";
    unsigned failedProducts = 0, inadmissibleTrials = 0;
    bool linearFailure = false, residualFailure = false, startup = false, deflated = false, stationary = false;
    double reconstruction = 1, linearTarget = 0, toleranceFactor = 1, meanFlowResidual = 0;
    unsigned long preconditionerIterations = 0;
  };
  bool GetNumericalFailure() const override { return failed; }
  const char* GetNumericalFailureMessage() const override { return failureMessage.c_str(); }
  const Attempt& GetLastAttempt() const { return lastAttempt; }
  /*! \brief Full residual assemblies per equation slot (FLOW, TURB, TRANS, SPECIES) since setup. */
  const std::array<unsigned long, 4>& GetResidualEvaluations() const { return equations->GetResidualEvaluations(); }
  /*! \brief Advance this group by one accepted update, or report terminal numerical failure to the driver. */
  void Iterate(CGeometry*, CSolver**, CNumerics***, CConfig*);
  /*! \brief Apply the acceptance operator; directionModel requests the optional legacy startup model. */
  void MatrixFreeProduct(const CSysVector<Scalar>&, CSysVector<Scalar>&, bool directionModel = false);
  /*! \brief Apply native block preconditioning in the group's scaled coordinates. */
  void Precondition(const CSysVector<Scalar>&, CSysVector<Scalar>&) const;

 private:
  CConfig* config = nullptr;
  CGeometry* geometry = nullptr;
  CSolver** solvers = nullptr;
  CNumerics*** numerics = nullptr;
  CSysVector<Scalar> scale, rhs, correction, direction, product, pseudoDirection, probeReference, baseState;
  CSysSolve<Scalar> linearSolver;
  std::unique_ptr<CNewtonEquationSet> equations;
  std::unique_ptr<CNewtonLinearization> linearization, modelLinearization;
  std::unique_ptr<CNewtonPseudoTime> pseudoTime;
  CNewtonLinearization* directionOperator = nullptr;
  struct FlowPolicy {
    bool startup = false, deflation = false, automatic = false;
    unsigned remaining = 0, innerIterations = 0, toleranceRelaxation = 1;
    unsigned long linearIterations = 0;
    double startupDrop = 0, fullToleranceDrop = 0, firstResidual = -20;
    double reconstruction = 1, innerTolerance = 0.1, linearTolerance = 0;
  } flow;
  std::unique_ptr<CNewtonBlockPreconditioner> preconditioner;
  std::vector<unsigned char> free;
  Attempt lastAttempt;
  bool setup = false, failed = false, residualValid = true, constraintsReady = false, nativeDirection = false;
  std::string failureMessage;
  double cfl = 0, eta = 0, referenceCFLFactor = 1;
  double cflLinearTolerance = 0; /*!< \brief Native CFL rule: acceptable linear residual (assembled operators). */
  std::vector<double> localCFL; /*!< \brief Per-point CFL of assembled operators (native local CFL rule). */
  NonlinearGlobalization::Options<> localRule; /*!< \brief Native CFL_ADAPT_PARAM factors of the per-point rule. */
  /*! \brief Scale the assembled local CFL field, clamp it to the CFL bounds and return its global maximum. */
  double ScaleLocalCFL(double factor);
  /*! \brief Apply the native per-point CFL rule after an accepted assembled update; return the global maximum. */
  double AdaptLocalCFL(const Attempt& attempt);
  bool nativeFallback = false; /*!< \brief Assembled recovery exhausted: take one native full step. */
  bool liveResidual = false; /*!< \brief The accepted update left current residuals of every equation of the zone. */
  /*! \brief Assembled full trials also assemble the native matrices; an accepted one (or the outer refresh) is
   *  the next base assembly, if the state of every solver is still the recorded one. */
  bool implicitTrial = false, reuseAssembly = false;
  unsigned long assemblyStamp = 0; /*!< \brief Residual evaluations when the reusable assembly was made. */
  std::vector<passivedouble> assemblyState; /*!< \brief Solutions of every solver at the reusable assembly. */
  /*! \brief Record the live residuals and matrices of this group as the next base assembly. */
  void RecordAssembly();
  /*! \brief Collective: the recorded assembly is still the live base of the current state. */
  bool AssemblyCurrent();
  unsigned long EvaluationCount() const {
    const auto& counts = equations->GetResidualEvaluations();
    return std::accumulate(counts.begin(), counts.end(), 0ul);
  }
  std::deque<double> recentMerits; /*!< \brief Accepted base merits for the non-monotone assembled line search. */
  mutable double reduction = 0;
  mutable std::vector<double> reductionPartials;
  unsigned attemptNumber = 0, trialNumber = 0, failedProducts = 0;
  const char* productFailure = "NONE";
  mutable bool diagnosticHeaderChecked = false, detailedDiagnostics = true;

  double Reduce(double, bool maximum = false) const;
  double CFLFactor(const CNewtonEquationSet::Block&) const;
  bool IsFree(const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) const {
    return free[p * equations->GetnVar() + b.offset + v];
  }
  void Setup();
  void ConfigureFlow();
  void PrepareFlow(Attempt&);
  double CommitFlow(const Attempt&, double nextCFL);
  /*! \brief Capture one base and run accepted-update or bounded-recovery decisions. */
  void Advance();
  void UpdateScale();
  void SetCFL(double);
  void Perturb(const CSysVector<Scalar>&, double);
  void Residual(bool implicit = false);
  bool RefreshStatistics(bool allEquations = false);
  /*! \brief Sum the fixed-scale residual merit, including the configured frozen pseudo-time term. */
  double Merit(double step = 0) const;
  double Evaluate(double step, const char* phase);
  /*! \brief Record a trial or an unchanged base reference without forcing another residual evaluation. */
  void LogEvaluation(double step, const char* phase, double value, double physical);
  /*! \brief Restore the captured state and record state/merit errors before any retry. */
  void Restore(Attempt&);
  /*! \brief Apply native endpoint boundary handling and read back the actual direction before searching. */
  void Endpoint();
  /*! \brief Solve and check a direction; only the first attempt may reuse Advance's unchanged base assembly. */
  Attempt SolveAttempt(bool preparedBase);
  /*! \brief Compute the actual merit slope and search without changing base scales or pseudo-time data. */
  void Search(Attempt&);
  void Log(const Attempt&, const NonlinearGlobalization::Decision&) const;
#endif
};
