/*!
 * \file CSSTNewtonIntegration.hpp
 * \brief Coefficient-aware SST Newton-Krylov with line search and pseudo-time recovery.
 * \author rois1995
 */
#pragma once

#include "CIntegration.hpp"
#include "CNewtonLinearization.hpp"
#include "../../../Common/include/linear_algebra/CPreconditioner.hpp"
#include "../../../Common/include/linear_algebra/CSysSolve.hpp"
#include <memory>

class CSSTNewtonIntegration final : public CIntegration {
 public:
  void MultiGrid_Iteration(CGeometry****, CSolver*****, CNumerics******, CConfig**, unsigned short, unsigned short,
                           unsigned short) override;
#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
  using Scalar = passivedouble;
  bool GetNumericalFailure() const override { return failed; }
  const char* GetNumericalFailureMessage() const override { return failureMessage.c_str(); }
  void MatrixFreeProduct(const CSysVector<Scalar>&, CSysVector<Scalar>&);
  void Precondition(const CSysVector<Scalar>&, CSysVector<Scalar>&) const;

 private:
  struct Attempt {
    Armijo::Result search{Armijo::Status::InvalidInput, 0, 0, 0};
    double base = 0, physicalMerit = 0, slope = 0, productNorm2 = 0, prediction = 0;
    double agreement = std::numeric_limits<double>::quiet_NaN();
    double linearDefect = 0, reportedLinearResidual = 0, rollbackMeritError = 0, rollbackStateError = 0;
    unsigned linearIterations = 0;
    bool linearFailure = false;
  };
  CConfig* config = nullptr;
  CGeometry* geometry = nullptr;
  CSolver** solvers = nullptr;
  CNumerics*** numerics = nullptr;
  NonlinearGlobalization::Options<> options;
  CSysVector<Scalar> scale, rhs, correction, direction, product, pseudoDiagonal, probeReference;
  CSysSolve<Scalar> linearSolver;
  std::vector<double> oldRelaxation;
  std::unique_ptr<CNewtonEquationSet> equations;
  std::unique_ptr<CNewtonLinearization> linearization;
  std::unique_ptr<CNewtonBlockPreconditioner> preconditioner;
  bool setup = false, failed = false, residualValid = true;
  std::string failureMessage;
  double cfl = 0, eta = 0;
  unsigned attemptNumber = 0, trialNumber = 0;

  void Setup();
  void UpdateScale();
  void SetCFL(double);
  void Perturb(const CSysVector<Scalar>&, double);
  void Residual(bool implicit = false);
  double Merit(double step = 0) const;
  double Evaluate(double step, const char* phase);
  void Restore(Attempt&);
  Attempt SolveAttempt();
  void Search(Attempt&);
  void Log(const Attempt&, const NonlinearGlobalization::Decision&) const;
#endif
};
