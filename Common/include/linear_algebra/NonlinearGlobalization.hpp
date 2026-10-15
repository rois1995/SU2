/*!
 * \file NonlinearGlobalization.hpp
 * \brief Parameters and pseudo-time control for globalized inexact Newton iterations.
 * \author rois1995
 */
#pragma once

#include "ArmijoLineSearch.hpp"
#include <string>

namespace NonlinearGlobalization {
template <class Real = double>
struct Options {
  bool enabled = false;
  bool lineSearch = true;
  bool recovery = true;
  bool adaptCFL = true;
  bool adaptiveForcing = true;
  bool diagnostics = true;
  bool pseudoTimeMerit = true;
  Real armijo = 0.0001;
  Real stepTolerance = 3.666852862501036e-11;
  Real restoreTolerance = 1e-10;
  Real interpolationMin = 0.1;
  Real interpolationMax = 0.5;
  Real cflMin = 0.001;
  Real cflMax = 1000000;
  Real shrink = 0.25;
  Real grow = 2;
  Real poorAgreement = 0.25;
  Real goodAgreement = 0.75;
  Real fdStep = 1e-8;
  Real relativeFDStep = 0.0001;
  Real linearTolerance = 0.001;
  Real etaMin = 0.00001;
  Real etaMax = 0.1;
  Real forcingGamma = 0.9;
  Real forcingPower = 1.5;
  unsigned short maxBacktracks = 20;
  unsigned short maxRetries = 16;
  unsigned short linearIterations = 80;

  template <class Convert>
  Options<double> Passive(Convert convert) const {
    Options<double> result;
    result.enabled = enabled;
    result.lineSearch = lineSearch;
    result.recovery = recovery;
    result.adaptCFL = adaptCFL;
    result.adaptiveForcing = adaptiveForcing;
    result.diagnostics = diagnostics;
    result.pseudoTimeMerit = pseudoTimeMerit;
    result.maxBacktracks = maxBacktracks;
    result.maxRetries = maxRetries;
    result.linearIterations = linearIterations;
    result.armijo = convert(armijo);
    result.stepTolerance = convert(stepTolerance);
    result.restoreTolerance = convert(restoreTolerance);
    result.interpolationMin = convert(interpolationMin);
    result.interpolationMax = convert(interpolationMax);
    result.cflMin = convert(cflMin);
    result.cflMax = convert(cflMax);
    result.shrink = convert(shrink);
    result.grow = convert(grow);
    result.poorAgreement = convert(poorAgreement);
    result.goodAgreement = convert(goodAgreement);
    result.fdStep = convert(fdStep);
    result.relativeFDStep = convert(relativeFDStep);
    result.linearTolerance = convert(linearTolerance);
    result.etaMin = convert(etaMin);
    result.etaMax = convert(etaMax);
    result.forcingGamma = convert(forcingGamma);
    result.forcingPower = convert(forcingPower);
    return result;
  }

  std::string Validate() const {
    if (!Armijo::IsFinite(restoreTolerance) || restoreTolerance < 0 || restoreTolerance >= 1)
      return "SST_NEWTON: invalid relative merit restoration tolerance";
    if (!Armijo::IsFinite(armijo)) return "SST_NEWTON: armijo must be finite";
    if (!Armijo::IsFinite(stepTolerance)) return "SST_NEWTON: stepTolerance must be finite";
    if (!Armijo::IsFinite(interpolationMin)) return "SST_NEWTON: interpolationMin must be finite";
    if (!Armijo::IsFinite(interpolationMax)) return "SST_NEWTON: interpolationMax must be finite";
    if (!Armijo::IsFinite(cflMin)) return "SST_NEWTON: cflMin must be finite";
    if (!Armijo::IsFinite(cflMax)) return "SST_NEWTON: cflMax must be finite";
    if (!Armijo::IsFinite(shrink)) return "SST_NEWTON: shrink must be finite";
    if (!Armijo::IsFinite(grow)) return "SST_NEWTON: grow must be finite";
    if (!Armijo::IsFinite(poorAgreement)) return "SST_NEWTON: poorAgreement must be finite";
    if (!Armijo::IsFinite(goodAgreement)) return "SST_NEWTON: goodAgreement must be finite";
    if (!Armijo::IsFinite(fdStep)) return "SST_NEWTON: fdStep must be finite";
    if (!Armijo::IsFinite(relativeFDStep)) return "SST_NEWTON: relativeFDStep must be finite";
    if (!Armijo::IsFinite(linearTolerance)) return "SST_NEWTON: linearTolerance must be finite";
    if (!Armijo::IsFinite(etaMin)) return "SST_NEWTON: etaMin must be finite";
    if (!Armijo::IsFinite(etaMax)) return "SST_NEWTON: etaMax must be finite";
    if (!Armijo::IsFinite(forcingGamma)) return "SST_NEWTON: forcingGamma must be finite";
    if (!Armijo::IsFinite(forcingPower)) return "SST_NEWTON: forcingPower must be finite";
    if (!(armijo > 0 && armijo < 1 && stepTolerance > 0 && interpolationMin > 0 &&
          interpolationMin <= interpolationMax && interpolationMax < 1))
      return "SST_NEWTON: invalid Armijo, step tolerance or interpolation safeguards";
    if (!(cflMin > 0 && cflMax >= cflMin && shrink > 0 && shrink < 1 && grow > 1 && poorAgreement > 0 &&
          poorAgreement < goodAgreement && goodAgreement < 1))
      return "SST_NEWTON: invalid CFL bounds, growth/shrink factors or model-agreement thresholds";
    if (!(fdStep > 0 && relativeFDStep > 0 && relativeFDStep < 1 && etaMin > 0 && etaMin <= linearTolerance &&
          linearTolerance <= etaMax && etaMax < 1 && forcingGamma > 0 && forcingGamma < 1 && forcingPower > 1 &&
          forcingPower <= 2))
      return "SST_NEWTON: invalid finite-difference or inexact-Newton forcing parameters";
    if (!maxBacktracks || !linearIterations || linearIterations > 1000)
      return "SST_NEWTON: backtracking limit must be positive and Krylov subspace must lie in [1,1000]";
    if (!lineSearch && (recovery || adaptCFL)) return "SST_NEWTON: recovery and CFL adaptation require line search";
    return {};
  }
};

enum class RecoveryReason { Accepted, Retry, RetryLimit, MinimumCFL, InvalidState, Disabled };

struct Decision {
  RecoveryReason reason;
  double nextCFL;
};

inline Decision Reject(const Options<>& options, double cfl, unsigned attempt, bool invalidBase = false) {
  if (invalidBase) return {RecoveryReason::InvalidState, cfl};
  if (!options.recovery) return {RecoveryReason::Disabled, cfl};
  if (attempt >= options.maxRetries) return {RecoveryReason::RetryLimit, cfl};
  const double next = std::max(options.cflMin, cfl * options.shrink);
  if (!(next < cfl)) return {RecoveryReason::MinimumCFL, cfl};
  return {RecoveryReason::Retry, next};
}

inline double Adapt(const Options<>& options, double cfl, double agreement, double step) {
  if (!options.adaptCFL || step == 0) return cfl;
  if (!Armijo::IsFinite(agreement) || agreement < options.poorAgreement || step < options.interpolationMax)
    cfl *= options.shrink;
  else if (agreement > options.goodAgreement && step == 1.0)
    cfl *= options.grow;
  return std::max(options.cflMin, std::min(options.cflMax, cfl));
}

// Eisenstat-Walker choice 2 uses norms of the same scaled residual and undamped steps.
inline double Forcing(const Options<>& options, double eta, double base, double accepted, double step) {
  if (!options.adaptiveForcing || step < 1.0 || !(base > 0)) return options.linearTolerance;
  const double candidate = options.forcingGamma * std::pow(std::sqrt(accepted / base), options.forcingPower);
  const double safeguard = options.forcingGamma * std::pow(eta, options.forcingPower);
  return std::max(options.etaMin, std::min(options.etaMax, std::max(candidate, safeguard)));
}
}  // namespace NonlinearGlobalization
