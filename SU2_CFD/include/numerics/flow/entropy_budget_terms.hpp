/*!
 * \file entropy_budget_terms.hpp
 * \brief Scalar diagnostics for the JST entropy balance.
 * \copyright Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * \license GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once

#include "p1_boundary_closure.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <type_traits>

namespace EntropyBudgetTerms {

inline constexpr auto MaxVar = P1BoundaryClosure::ES_NVAR;
inline constexpr unsigned RatioBins = 42;
using State = std::array<double, MaxVar>;
using Normal = std::array<double, 3>;

template <class T>
double Value(const T& x) {
  if constexpr (std::is_floating_point_v<T>) return x;
  else return SU2_TYPE::GetValue(x);
}

// eq:production: w and the entropy potential psi = rho v.
inline State Variables(unsigned short dim, const su2double& gamma, const P1BoundaryClosure::EsState& s) {
  su2double w[MaxVar];
  P1BoundaryClosure::EsEntropyVariables(dim, gamma, s, w);
  State result{};
  for (unsigned k = 0; k < dim + 2u; ++k) result[k] = Value(w[k]);
  return result;
}

inline Normal Potential(unsigned short dim, const P1BoundaryClosure::EsState& s) {
  Normal psi{};
  for (unsigned d = 0; d < dim; ++d) psi[d] = Value(s.rho) * Value(s.vel[d]);
  return psi;
}

// eq:production: contract the assembled rows, keeping their absolute scale for round-off checks.
struct Contraction {
  double value = 0, scale = 0;
  void Add(double product) {
    value += product;
    scale += std::abs(product);
  }
};

template <class A, class B>
Contraction Dot(unsigned size, const A& a, const B& b) {
  Contraction result;
  for (unsigned k = 0; k < size; ++k) result.Add(Value(a[k]) * Value(b[k]));
  return result;
}

// eq:tadmor, thm:eps2 and sec:eps4: [a] = a_j - a_i.
template <std::size_t N>
std::array<double, N> Jump(const std::array<double, N>& i, const std::array<double, N>& j) {
  std::array<double, N> result{};
  for (unsigned k = 0; k < N; ++k) result[k] = j[k] - i[k];
  return result;
}

// eq:tadmor, thm:eps2, sec:eps4: signed edge productions, with [a] = a_j - a_i.
struct EdgeProduction {
  double central, eps2, eps4;
  Contraction enthalpy;
};

template <class AreaVector>
EdgeProduction ProduceEdge(unsigned dim, const State& dw, const Normal& dpsi, const AreaVector& normal,
                           const State& centralFlux, const State& dut, const State& dlapl, const double* chi) {
  const unsigned nVar = dim + 2;
  const auto dc = Dot(nVar, dw, centralFlux).value - Dot(dim, dpsi, normal).value;
  const auto enthalpy = Dot(nVar, dw, dut);
  return {dc, -chi[0] * enthalpy.value, chi[1] * Dot(nVar, dw, dlapl).value, enthalpy};
}

// thm:wall: use the difference so D_T = 0 also has a defined test.
inline double WallDefect(double production, double dissipation, double penalty) {
  return production - dissipation / penalty;
}

// thm:eps2 and thm:visc: bound cancellation in a contraction of the residual operands.
inline double Roundoff(unsigned nVar, double scale) {
  return 8 * nVar * std::numeric_limits<passivedouble>::epsilon() * scale;
}

inline bool ResolvedRatio(unsigned nVar, Contraction denominator, double globalMax) {
  return denominator.value > Roundoff(nVar, denominator.scale) && denominator.value >= 1e-14 * globalMax;
}

inline std::array<double, 2> RatioBounds(double gamma) {
  const auto width = std::sqrt(gamma * (gamma - 1));
  return {{0.5 * (gamma + 1 - width), 0.5 * (gamma + 1 + width)}};
}

inline unsigned RatioBin(double r) {
  if (r < 0.7) return 0;
  if (r > 1.7) return RatioBins - 1;
  return 1 + std::min(39u, static_cast<unsigned>((r - 0.7) * 40));
}

// eq:complete: select a single rank for a shared edge, simplex or boundary face.
template <class Index, class Owned>
bool OwnsItem(const unsigned long* point, unsigned count, Index globalIndex, Owned owns) {
  const auto* end = point + count;
  const auto* first = std::min_element(point, end, [&](auto a, auto b) { return globalIndex(a) < globalIndex(b); });
  return first != end && owns(*first);
}

inline double Relative(double error, double scale) {
  return scale > 0 ? std::abs(error) / scale : (error == 0 ? 0 : std::numeric_limits<double>::infinity());
}

}  // namespace EntropyBudgetTerms
