/*!
 * \file entropy_budget_tests.cpp
 * \brief Unit checks for the JST entropy budget and read-only pair tap.
 * \copyright Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * \license GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../../SU2_CFD/include/numerics/flow/entropy_budget_terms.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_pair_jst.hpp"
#include <cstring>
#include <random>

namespace ebt = EntropyBudgetTerms;

TEST_CASE("Entropy budget signs agree with the conservative edge orientation", "[entropy_budget]") {
  const ebt::State w_i{{1, 2, 0, -1}}, w_j{{2, 3, 1, -1}};
  const ebt::State central{{4, 2, 0, 1}}, dut{{3, 2, 1, 0}}, dlapl{{2, -1, -1, 0}};
  const ebt::Normal dpsi{{1, -1, 0}}, normal{{2, 1, 0}};
  const double chi[] = {0.5, 0.25};
  const auto value = ebt::ProduceEdge(2, ebt::Jump(w_i, w_j), dpsi, normal, central, dut, dlapl, chi);
  CHECK(value.central == 5);
  CHECK(value.eps2 == -3);
  CHECK(value.eps4 == 0);

  const ebt::State variedLapl{{2, 0, -1, 0}};
  const auto varied = ebt::ProduceEdge(2, ebt::Jump(w_i, w_j), dpsi, normal, central, dut, variedLapl, chi);
  CHECK(varied.eps4 == 0.25);
  CHECK(ebt::Dot(4, w_j, central).value - ebt::Dot(4, w_i, central).value == value.central + 1);
}

TEST_CASE("Entropy potential cancels the Ismail-Roe central production", "[entropy_budget]") {
  using namespace P1BoundaryClosure;
  constexpr unsigned dim = 2;
  const su2double gamma = 1.4, normal[] = {0.3, -0.5};
  const EsState i{0.8, {0.1, 0.2, 0}, 1.1}, j{1.7, {-0.2, 0.3, 0}, 0.9};
  su2double flux[ES_NVAR]{};
  EsIsmailRoeFlux(dim, gamma, i, j, normal, flux);
  const auto dw = ebt::Jump(ebt::Variables(dim, gamma, i), ebt::Variables(dim, gamma, j));
  const auto dpsi = ebt::Jump(ebt::Potential(dim, i), ebt::Potential(dim, j));
  const auto work = ebt::Dot(dim + 2, dw, flux);
  const auto potential = ebt::Dot(dim, dpsi, normal);
  CHECK(std::abs(work.value - potential.value) <= ebt::Roundoff(dim + 2, work.scale + potential.scale));
}

TEST_CASE("Entropy budget handles empty samples and shared items", "[entropy_budget]") {
  CHECK_FALSE(ebt::ResolvedRatio(4, {}, 0));
  CHECK_FALSE(ebt::ResolvedRatio(4, {1e-15, 1e-15}, 1));
  CHECK_FALSE(ebt::ResolvedRatio(4, {-1, 2}, 1));
  CHECK(ebt::ResolvedRatio(4, {1e-14, 1e-14}, 1));
  CHECK(ebt::Relative(0, 0) == 0);
  CHECK(ebt::RatioBin(0.69) == 0);
  CHECK(ebt::RatioBin(0.7) == 1);
  CHECK(ebt::RatioBin(1.699) == 40);
  CHECK(ebt::RatioBin(1.7) == 40);
  CHECK(ebt::RatioBin(1.71) == 41);

  const auto bounds = ebt::RatioBounds(1.4);
  CHECK(bounds[0] == Approx(0.8258342613));
  CHECK(bounds[1] == Approx(1.5741657387));
  CHECK(ebt::WallDefect(0.01, 0, 2.5) > 0);
  CHECK(ebt::WallDefect(0.4, 1, 2.5) == 0);

  const unsigned long point[] = {1, 2, 0};
  const unsigned long global[] = {2, 8, 6};
  const auto index = [&](unsigned long p) { return global[p]; };
  CHECK(ebt::OwnsItem(point, 3, index, [](unsigned long p) { return p == 0; }));
  CHECK_FALSE(ebt::OwnsItem(point, 3, index, [](unsigned long p) { return p != 0; }));
}

TEST_CASE("Capturing split JST coefficients preserves residual and Jacobian bits", "[entropy_budget]") {
  if constexpr (!std::is_same_v<su2double, passivedouble>) return;
  std::mt19937 random(85);
  std::uniform_real_distribution<double> value(-1.0, 1.0);
  constexpr unsigned dim = 3, nVar = dim + 2;
  const auto node = [&]() {
    P1PairJST::Node result;
    for (auto& v : result.vel) v = value(random);
    for (auto& u : result.diss) u = value(random);
    for (auto& l : result.lapl) l = value(random);
    result.soundSpeed = 1.5 + value(random);
    result.sensor = 0.1 * std::abs(value(random));
    result.lambda = 6 + value(random);
    result.nNeighbor = 4 + std::abs(value(random));
    return result;
  };
  for (unsigned draw = 0; draw < 100; ++draw) {
    auto i = node();
    const auto j = node();
    i.wall = draw % 2;
    P1PairJST::Params params;
    params.weakWall = draw % 3 == 0;
    const su2double share = draw % 2 == 0 ? 0.5 : 1.0;
    const su2double normal[dim] = {draw == 0 ? 0.0 : value(random), draw == 0 ? 0.0 : value(random), 0.0};
    su2double flux[nVar]{}, tappedFlux[nVar]{}, chi[2] = {-1, -1};
    su2double jacI[nVar][nVar]{}, jacJ[nVar][nVar]{}, tappedI[nVar][nVar]{}, tappedJ[nVar][nVar]{};
    P1PairJST::AddPairDissipation(dim, params, i, j, normal, flux, true, jacI, jacJ, share);
    P1PairJST::AddPairDissipation(dim, params, i, j, normal, tappedFlux, true, tappedI, tappedJ, share, nullptr, chi);
    CHECK(std::memcmp(flux, tappedFlux, sizeof(flux)) == 0);
    CHECK(std::memcmp(jacI, tappedI, sizeof(jacI)) == 0);
    CHECK(std::memcmp(jacJ, tappedJ, sizeof(jacJ)) == 0);
    if (draw == 0 || (params.weakWall && ebt::Value(i.wall) != 0.0)) {
      CHECK(ebt::Value(chi[0]) == 0);
      CHECK(ebt::Value(chi[1]) == 0);
    }
    for (unsigned k = 0; k < nVar; ++k) {
      const auto d2 = ebt::Value(chi[0] * (i.diss[k] - j.diss[k]));
      const auto d4 = ebt::Value(chi[1] * (i.lapl[k] - j.lapl[k]));
      CHECK(ebt::Relative(ebt::Value(flux[k]) - (d2 - d4), std::abs(d2) + std::abs(d4)) <=
            std::max(1e-14, ebt::Roundoff(nVar, 1)));
    }
  }
}
