/*!
 * \file CComplexityNormalization.hpp
 * \brief Bounded scale search driven by a globally assessed integration uncertainty.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace SU2Complexity {

struct Estimate {
  double value=0,uncertainty=0;
};
enum class Status { EstimatedTolerance, IntegrationLimited, OutsideBracket, TrialLimit };
struct Normalization {
  double logScale=0;
  Estimate estimate;
  Status status=Status::TrialLimit;
  unsigned trials=0;
};

/*! Search a monotone scale response. The callback returns GLOBAL value/absolute uncertainty and may
 *  stop short of the requested accuracy. Its uncertainty may be heuristic: EstimatedTolerance is
 *  agreement with that estimate, never proof of an exact root or of a rigorous integration bound.
 *  Repeated requests at one scale tighten integration of the SAME field, not its donor representation. */
template<class Integrate>
Normalization Normalize(double target,double lower,double upper,double initial,double relativeTolerance,
                        unsigned maximumTrials,Integrate integrate) {
  if(!(target>0) || !std::isfinite(target) || !std::isfinite(lower) || !std::isfinite(upper) ||
      !(lower<upper) || !std::isfinite(initial) || !(relativeTolerance>0 && relativeTolerance<1) ||
      !maximumTrials)throw std::invalid_argument("Invalid uncertainty-aware complexity search.");
  const double allowed=relativeTolerance*target;
  if(!(allowed>0))throw std::invalid_argument("Unrepresentable complexity tolerance.");
  Normalization result;
  auto sample=[&](double x) {
    double requested=std::max(allowed*.25,target*.05);
    double previous=INFINITY;
    bool refined=false;
    for(;;) {
      if(result.trials==maximumTrials){result.status=Status::TrialLimit;return 0;}
      result.logScale=x;result.estimate=integrate(x,requested);++result.trials;
      const auto& e=result.estimate;
      if(!(e.value>0) || !std::isfinite(e.value) || std::isnan(e.uncertainty) || e.uncertainty<0)
        throw std::runtime_error("Invalid global complexity estimate.");
      if(std::abs(e.value-target)+e.uncertainty<=allowed){result.status=Status::EstimatedTolerance;return 0;}
      if(e.value+e.uncertainty<target)return -1;
      if(e.value-e.uncertainty>target)return 1;
      if(refined && !(e.uncertainty<previous)) {result.status=Status::IntegrationLimited;return 0;}
      const double next=std::max(allowed*.25,std::min(requested*.25,e.uncertainty*.25));
      if(!(next<requested)){result.status=Status::IntegrationLimited;return 0;}
      previous=e.uncertainty;requested=next;refined=true;
    }
  };
  initial=std::clamp(initial,lower,upper);
  const int first=sample(initial);if(!first)return result;
  double lo=initial,hi=initial,lowValue=result.estimate.value,highValue=lowValue;
  double step=std::log(4.);
  for(;;) {
    const double x=first<0?std::min(upper,initial+step):std::max(lower,initial-step);
    if(x==(first<0?hi:lo)){result.status=Status::OutsideBracket;return result;}
    const int sign=sample(x);if(!sign)return result;
    if(sign<0){lo=x;lowValue=result.estimate.value;}else{hi=x;highValue=result.estimate.value;}
    if(sign!=first)break;
    if(first<0)hi=lo;else lo=hi;
    step*=2;
  }
  for(;;) {
    const double a=std::log(lowValue)-std::log(target),b=std::log(highValue)-std::log(target);
    double x=lo+(hi-lo)*(-a/(b-a));
    x=std::clamp(x,lo+.1*(hi-lo),hi-.1*(hi-lo));
    if(!(x>lo && x<hi)){result.status=Status::TrialLimit;return result;}
    const int sign=sample(x);if(!sign)return result;
    if(sign<0){lo=x;lowValue=result.estimate.value;}else{hi=x;highValue=result.estimate.value;}
  }
}

/*! Distributed callers reduce the maximum eligible ABSOLUTE error, then select locally.
 *  Infinite uncertainty is handled before finite contributions; no cell list is gathered. */
inline bool SelectError(double localError,double globalMaximum) {
  if(std::isnan(localError) || localError<0 || std::isnan(globalMaximum) || globalMaximum<0)
    throw std::invalid_argument("Invalid distributed complexity priority.");
  return globalMaximum>0 && (std::isinf(globalMaximum)?std::isinf(localError):localError>=.5*globalMaximum);
}

}  // namespace SU2Complexity
