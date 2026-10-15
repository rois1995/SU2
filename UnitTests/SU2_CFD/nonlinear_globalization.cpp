/*!
 * \file nonlinear_globalization.cpp
 * \brief Line-search, pseudo-time recovery and inexact-Newton controller tests.
 * \author rois1995
 */
#include "catch.hpp"
#include "../../Common/include/linear_algebra/NonlinearGlobalization.hpp"
#include <stdexcept>
#include <vector>

TEST_CASE("Pseudo-time merit retains the implicit equation for a nonnormal stable system", "[globalization]") {
  // J=[1,100;0,1] has positive eigenvalues; its physical residual can have transient growth.
  const double p1 = 100.0 / 121.0, p2 = -1.0 / 11.0, diagonal = 10;
  double physical = 0;
  const auto result = Armijo::Search(
      0.5, -1, 1e-12,
      [&](double alpha) {
        const double f1 = alpha * (p1 + 100 * p2), f2 = 1 + alpha * p2;
        physical = 0.5 * (f1 * f1 + f2 * f2);
        const double g1 = f1 + diagonal * alpha * p1, g2 = f2 + diagonal * alpha * p2;
        return 0.5 * (g1 * g1 + g2 * g2);
      },
      [] {});
  REQUIRE(result.status == Armijo::Status::Accepted);
  REQUIRE(result.step == 1);
  REQUIRE(result.value < 1e-25);
  REQUIRE(physical > 0.5);
}

TEST_CASE("Globalization options validate their numerical contracts", "[globalization]") {
  NonlinearGlobalization::Options<> options;
  REQUIRE(options.Validate().empty());
  SECTION("NaN") { options.armijo = std::numeric_limits<double>::quiet_NaN(); }
  SECTION("negative CFL") { options.cflMin = -1; }
  SECTION("reversed CFL bounds") { options.cflMax = options.cflMin / 2; }
  SECTION("noncontracting recovery") { options.shrink = 1; }
  SECTION("reversed agreement thresholds") { options.poorAgreement = options.goodAgreement; }
  SECTION("invalid interpolation") { options.interpolationMin = options.interpolationMax + 0.1; }
  SECTION("invalid forcing") { options.etaMin = options.etaMax * 2; }
  SECTION("zero backtracking budget") { options.maxBacktracks = 0; }
  SECTION("zero linear iterations") { options.linearIterations = 0; }
  SECTION("recovery without line search") { options.lineSearch = false; }
  REQUIRE_FALSE(options.Validate().empty());
}

TEST_CASE("Pseudo-time rejection is bounded and preserves terminal reasons", "[globalization]") {
  using namespace NonlinearGlobalization;
  Options<> options;
  options.cflMin = 1;
  options.maxRetries = 2;
  REQUIRE(Reject(options, 100, 0).reason == RecoveryReason::Retry);
  REQUIRE(Reject(options, 100, 0).nextCFL == 25);
  REQUIRE(Reject(options, 100, 2).reason == RecoveryReason::RetryLimit);
  REQUIRE(Reject(options, 1, 0).reason == RecoveryReason::MinimumCFL);
  REQUIRE(Reject(options, 100, 0, true).reason == RecoveryReason::InvalidState);
  REQUIRE(Reject(options, 2, 0).nextCFL == 1);
  options.recovery = false;
  REQUIRE(Reject(options, 100, 0).reason == RecoveryReason::Disabled);
}

TEST_CASE("Accepted steps adapt pseudo-time from model agreement", "[globalization]") {
  using namespace NonlinearGlobalization;
  Options<> options;
  REQUIRE(Adapt(options, 100, 0.9, 1) == 200);
  REQUIRE(Adapt(options, 100, 0.1, 1) == 25);
  REQUIRE(Adapt(options, 100, 0.9, 0.1) == 25);
  REQUIRE(Adapt(options, 100, 0.5, 1) == 100);
  REQUIRE(Adapt(options, options.cflMax, 0.9, 1) == options.cflMax);
  REQUIRE(Adapt(options, options.cflMin, 0.1, 1) == options.cflMin);
  REQUIRE(Adapt(options, 100, std::numeric_limits<double>::quiet_NaN(), 1) == 25);
  options.adaptCFL = false;
  REQUIRE(Adapt(options, 100, 0.9, 1) == 100);
}

TEST_CASE("Inexact forcing is bounded and resets after a damped step", "[globalization]") {
  using namespace NonlinearGlobalization;
  Options<> options;
  const auto eta = Forcing(options, 0.01, 1, 0.01, 1);
  REQUIRE(eta >= options.etaMin);
  REQUIRE(eta <= options.etaMax);
  REQUIRE(Forcing(options, 0.01, 1, 0.01, 0.5) == options.linearTolerance);
  REQUIRE(Forcing(options, 0.01, 0, 0, 1) == options.linearTolerance);
  options.adaptiveForcing = false;
  REQUIRE(Forcing(options, 0.01, 1, 0.01, 1) == options.linearTolerance);
}

TEST_CASE("Recovery recomputes a direction and accepts a nonlinear update", "[globalization]") {
  using namespace NonlinearGlobalization;
  Options<> options;
  double x = 0.1, cfl = 10000;
  const double base = x;
  const double f = x * x * x - 1;
  unsigned attempts = 0, rollbacks = 0;
  double previousDirection = 0;
  for (;; ++attempts) {
    const double direction = -f / (3 * base * base + 1 / cfl);
    if (attempts) REQUIRE(direction != previousDirection);
    previousDirection = direction;
    const auto result = Armijo::Search(
        0.5 * f * f, f * 3 * base * base * direction, 1e-12,
        [&](double alpha) {
          x = base + alpha * direction;
          const double r = x * x * x - 1;
          return 0.5 * r * r;
        },
        [&] {
          x = base;
          ++rollbacks;
        },
        {options.armijo, 0.1, 0.5, 1});
    if (result.status == Armijo::Status::Accepted) {
      REQUIRE(attempts > 0);
      REQUIRE(rollbacks == attempts);
      REQUIRE(result.value < 0.5 * f * f);
      break;
    }
    REQUIRE(x == base);
    const auto decision = Reject(options, cfl, attempts);
    REQUIRE(decision.reason == RecoveryReason::Retry);
    cfl = decision.nextCFL;
  }
}

TEST_CASE("Armijo acceptance interpolation and rollback", "[globalization]") {
  using Armijo::Status;
  unsigned restored = 0;
  auto restore = [&] { ++restored; };

  auto full = Armijo::Search(
      0.5, -1.0, 1e-12, [](double a) { return 0.5 * (1 - a) * (1 - a); }, restore);
  REQUIRE((full.status == Status::Accepted && full.step == 1 && full.evaluations == 1 && restored == 0));

  auto insufficient = Armijo::Search(
      1, -1, 1e-12, [](double a) { return 1 - a + 0.99999 * a * a; }, restore);
  REQUIRE((insufficient.status == Status::Accepted && insufficient.step == 0.5 && insufficient.evaluations == 2));

  // Newton overshoots for x^2-1 at x=0.1. Backtracking must accept an actual residual decrease.
  double state = 0.1;
  const double residual = state * state - 1;
  const double direction = -residual / (2 * state);
  std::vector<double> attempts;
  auto overshoot = Armijo::Search(
      0.5 * residual * residual, -residual * residual, 1e-12,
      [&](double a) {
        attempts.push_back(a);
        state = 0.1 + a * direction;
        const double f = state * state - 1;
        return 0.5 * f * f;
      },
      [&] {
        state = 0.1;
        ++restored;
      });
  REQUIRE((overshoot.status == Status::Accepted && overshoot.step < 1 && overshoot.evaluations > 1));
  REQUIRE((overshoot.value <= 0.5 * residual * residual - 1e-4 * overshoot.step * residual * residual));
  for (unsigned i = 1; i < attempts.size(); ++i)
    REQUIRE((attempts[i] >= 0.1 * attempts[i - 1] && attempts[i] <= 0.5 * attempts[i - 1]));

  auto exactCubic = Armijo::Search(
      1, -1, 1e-12, [](double a) { return 1 - a + 10 * a * a + 100 * a * a * a; }, restore);
  REQUIRE((exactCubic.status == Status::Accepted && exactCubic.evaluations == 3));
  REQUIRE((std::abs(exactCubic.step - 1.0 / 30.0) < 1e-14));

  // Multiple rejected finite trials exercise cubic interpolation, not only the first quadratic fit.
  attempts.clear();
  auto cubic = Armijo::Search(
      1, -1, 1e-12,
      [&](double a) {
        attempts.push_back(a);
        return 1 - a + 10000 * a * a * a * a;
      },
      restore);
  REQUIRE((cubic.status == Status::Accepted && cubic.evaluations >= 4));
  REQUIRE((cubic.value <= 1 - 1e-4 * cubic.step));
  for (unsigned i = 1; i < attempts.size(); ++i)
    REQUIRE((attempts[i] >= 0.1 * attempts[i - 1] && attempts[i] <= 0.5 * attempts[i - 1]));

  unsigned calls = 0;
  auto nonfinite = Armijo::Search(
      1, -1, 1e-12,
      [&](double a) {
        ++calls;
        if (a >= 0.5) return std::numeric_limits<double>::quiet_NaN();
        return 1 - a;
      },
      restore);
  REQUIRE((nonfinite.status == Status::Accepted && nonfinite.step == 0.25 && calls == 3));
  REQUIRE((!Armijo::IsFinite(std::numeric_limits<double>::infinity())));
  REQUIRE((!Armijo::IsFinite(std::numeric_limits<double>::quiet_NaN())));

  unsigned before = restored;
  auto ascent = Armijo::Search(
      1, 1, 1e-12,
      [](double) {
        REQUIRE((false));
        return 0.0;
      },
      restore);
  REQUIRE((ascent.status == Status::NonDescent && ascent.evaluations == 0 && restored == before + 1));
  auto plateau = Armijo::Search(
      1, -1, 0.01, [](double) { return 1.0; }, restore);
  REQUIRE((plateau.status == Status::StepTooSmall && plateau.step == 0 && restored == before + 2));
  auto limited = Armijo::Search(
      1, -1, 1e-100, [](double) { return std::numeric_limits<double>::infinity(); }, restore);
  REQUIRE((limited.status == Status::EvaluationLimit && limited.evaluations == 20 && restored == before + 3));
  auto invalid = Armijo::Search(
      1, -1, 0,
      [](double) {
        REQUIRE((false));
        return 0.0;
      },
      restore);
  REQUIRE((invalid.status == Status::InvalidInput && restored == before + 4));
  bool caught = false;
  try {
    Armijo::Search(
        1, -1, 1e-12, [](double) -> double { throw std::runtime_error("domain error"); }, restore);
  } catch (const std::runtime_error&) {
    caught = true;
  }
  REQUIRE((caught && restored == before + 5));

  // The projected direction must be re-differentiated, rather than using the raw Newton slope.
  const double projectedDirection = 0.5;
  const double projectedSlope = (0.0 - 1.0) * projectedDirection;
  auto bounded = Armijo::Search(
      0.5, projectedSlope, 1e-12,
      [&](double a) {
        const double x = a * projectedDirection;
        REQUIRE((x >= 0 && x <= 0.5));
        return 0.5 * (x - 1) * (x - 1);
      },
      restore);
  REQUIRE((bounded.status == Status::Accepted && bounded.step == 1));
}
