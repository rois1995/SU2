/*!
 * \file CNewtonIntegration.hpp
 * \brief Globalized Newton-Krylov integration of selected equation groups.
 * \author SU2 Contributors
 */
#pragma once

#include "CIntegration.hpp"
#include "CNewtonLinearization.hpp"
#include "../../../Common/include/linear_algebra/CSysSolve.hpp"
#include <memory>

class CNewtonIntegration final : public CIntegration {
  std::vector<unsigned short> systems;
  NonlinearGlobalization::Options<> options;
  bool lowerTriangular;
  std::string prefix;

 public:
  CNewtonIntegration(std::vector<unsigned short> selected, NonlinearGlobalization::Options<> settings,
                     bool lower = false, std::string diagnostics = "newton")
      : systems(std::move(selected)), options(settings), lowerTriangular(lower), prefix(std::move(diagnostics)) {}
  void MultiGrid_Iteration(CGeometry****, CSolver*****, CNumerics******, CConfig**, unsigned short, unsigned short,
                           unsigned short) override;
#if !defined(CODI_FORWARD_TYPE) && !defined(CODI_REVERSE_TYPE)
  using Scalar = passivedouble;
  struct Attempt {
    Armijo::Result search{Armijo::Status::InvalidInput, 0, 0, 0};
    double base = 0, physicalMerit = 0, slope = 0, productNorm2 = 0, prediction = 0;
    double agreement = std::numeric_limits<double>::quiet_NaN();
    double linearDefect = 0, reportedLinearResidual = 0, rollbackMeritError = 0, rollbackStateError = 0;
    unsigned linearIterations = 0;
    bool linearFailure = false;
  };
  bool GetNumericalFailure() const override { return failed; }
  const char* GetNumericalFailureMessage() const override { return failureMessage.c_str(); }
  const Attempt& GetLastAttempt() const { return lastAttempt; }
  void Iterate(CGeometry*, CSolver**, CNumerics***, CConfig*);
  void MatrixFreeProduct(const CSysVector<Scalar>&, CSysVector<Scalar>&);
  void Precondition(const CSysVector<Scalar>&, CSysVector<Scalar>&) const;

 private:
  CConfig* config = nullptr;
  CGeometry* geometry = nullptr;
  CSolver** solvers = nullptr;
  CNumerics*** numerics = nullptr;
  CSysVector<Scalar> scale, rhs, correction, direction, product, pseudoDiagonal, probeReference, baseState;
  CSysSolve<Scalar> linearSolver;
  std::unique_ptr<CNewtonEquationSet> equations;
  std::unique_ptr<CNewtonLinearization> linearization;
  std::unique_ptr<CNewtonBlockPreconditioner> preconditioner;
  std::vector<unsigned char> free;
  Attempt lastAttempt;
  bool setup = false, failed = false, residualValid = true, constraintsReady = false;
  std::string failureMessage;
  double cfl = 0, eta = 0, referenceCFLFactor = 1;
  mutable double reduction = 0;
  unsigned attemptNumber = 0, trialNumber = 0;

  double Reduce(double, bool maximum = false) const;
  double CFLFactor(const CNewtonEquationSet::Block&) const;
  bool IsFree(const CNewtonEquationSet::Block& b, unsigned long p, unsigned short v) const {
    return free[p * equations->GetnVar() + b.offset + v];
  }
  void Setup();
  void Advance();
  void UpdateScale();
  void SetCFL(double);
  void Perturb(const CSysVector<Scalar>&, double);
  void Residual(bool implicit = false);
  double Merit(double step = 0) const;
  double Evaluate(double step, const char* phase);
  void Restore(Attempt&);
  void Endpoint();
  Attempt SolveAttempt();
  void Search(Attempt&);
  void Log(const Attempt&, const NonlinearGlobalization::Decision&) const;
#endif
};
