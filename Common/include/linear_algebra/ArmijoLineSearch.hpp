/*!
 * \brief Armijo backtracking for a fixed feasible direction and fixed merit scaling.
 * \author rois1995
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace Armijo {
enum class Status { Accepted, NonDescent, StepTooSmall, EvaluationLimit, InvalidInput };

struct Result {
  Status status;
  double step;
  double value;
  unsigned evaluations;
};

struct Parameters {
  double sufficientDecrease = 1e-4;
  double interpolationMin = 0.1;
  double interpolationMax = 0.5;
  unsigned maximumEvaluations = 20;
};

/*--- std::isfinite can be optimized away by the solver's -ffast-math build. ---*/
inline bool IsFinite(double value) {
  static_assert(sizeof(double) == sizeof(std::uint64_t) && std::numeric_limits<double>::is_iec559,
                "Line-search control requires IEEE binary64");
  std::uint64_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  return (bits & UINT64_C(0x7ff0000000000000)) != UINT64_C(0x7ff0000000000000);
}

/*!
 * \brief Search a fixed feasible direction with safeguarded interpolation and sufficient decrease.
 * \details The caller supplies the actual directional derivative, including any direction projection.
 * Failed searches and evaluator exceptions restore the base state through rollback().
 * Armijo c1 and safeguarded quadratic/cubic interpolation follow PETSc SNESLINESEARCHBT.
 */
template <class Evaluate, class Rollback>
Result Search(double base, double slope, double minimumStep, Evaluate&& evaluate, Rollback&& rollback,
              const Parameters& parameters = {}) {
  const double c1 = parameters.sufficientDecrease;
  const unsigned maximumEvaluations = parameters.maximumEvaluations;
  unsigned evaluations = 0;
  auto fail = [&](Status status) {
    rollback();
    return Result{status, 0.0, base, evaluations};
  };
  if (!IsFinite(base) || base < 0 || !IsFinite(slope) || !IsFinite(minimumStep) || minimumStep <= 0)
    return fail(Status::InvalidInput);
  if (!(IsFinite(c1) && c1 > 0 && c1 < 1 && IsFinite(parameters.interpolationMin) &&
        IsFinite(parameters.interpolationMax) && parameters.interpolationMin > 0 &&
        parameters.interpolationMin <= parameters.interpolationMax && parameters.interpolationMax < 1 &&
        maximumEvaluations > 0))
    return fail(Status::InvalidInput);
  if (slope >= 0) return fail(Status::NonDescent);

  double step = 1.0, previousStep = 0.0, previousValue = 0.0;
  while (evaluations < maximumEvaluations) {
    if (step < minimumStep) return fail(Status::StepTooSmall);
    double value;
    try {
      value = evaluate(step);
    } catch (...) {
      rollback();
      throw;
    }
    ++evaluations;
    const bool valid = IsFinite(value) && value >= 0;
    /*--- Strict decrease prevents a rounded-to-zero decrement from accepting stagnation. ---*/
    if (valid && value < base && value <= base + c1 * step * slope) return {Status::Accepted, step, value, evaluations};

    double next = 0.5 * step;
    if (valid) {
      const double remainder = (value - base - step * slope) / (step * step);
      const double quadratic = -slope / (2.0 * remainder);
      if (IsFinite(quadratic) && quadratic > 0) next = quadratic;
      if (previousStep > step) {
        const double previousRemainder = (previousValue - base - previousStep * slope) / (previousStep * previousStep);
        const double a = (remainder - previousRemainder) / (step - previousStep);
        const double b = (step * previousRemainder - previousStep * remainder) / (step - previousStep);
        const double discriminant = b * b - 3.0 * a * slope;
        if (IsFinite(discriminant) && discriminant >= 0) {
          const double denominator = b + std::sqrt(discriminant);
          const double cubic = -slope / denominator;
          if (denominator > 0 && IsFinite(cubic) && cubic > 0) next = cubic;
        }
      }
      previousStep = step;
      previousValue = value;
    }
    step = std::max(parameters.interpolationMin * step, std::min(parameters.interpolationMax * step, next));
  }
  return fail(Status::EvaluationLimit);
}
}  // namespace Armijo
