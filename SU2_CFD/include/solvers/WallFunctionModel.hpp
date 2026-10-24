/*!
 * \file WallFunctionModel.hpp
 * \brief Local compressible wall-model evaluation and safeguarded shear solve.
 * SU2 Project: LGPL-2.1-or-later, copyright 2012-2026 SU2 Contributors.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include "../../../Common/include/basic_types/datatype_structure.hpp"

namespace WallFunctionModel {

struct Input {
  su2double pressure, normal_temperature, wall_temperature;
  su2double wall_viscosity, normal_viscosity, conductivity, distance, velocity, heat_flux;
  su2double gas_constant, cp, recovery, kappa, B;
  bool isothermal;
};

struct State {
  su2double temperature = 0.0, density = 0.0, y_plus = 0.0, eddy_viscosity = 0.0;
  su2double residual = 0.0, derivative = 0.0, u_tau = 0.0;
};

inline bool Finite(const su2double& value) { return std::isfinite(SU2_TYPE::GetValue(value)); }

inline su2double ExpMinusOne(const su2double& x) {
  using std::exp;
  if (std::abs(SU2_TYPE::GetValue(x)) < 1e-3)
    return x * (1 + x * (0.5 + x * (1.0/6 + x * (1.0/24 + x * (1.0/120 + x/720)))));
  return exp(x) - 1;
}

inline bool Valid(const Input& p) {
  return Finite(p.pressure) && p.pressure > 0 && Finite(p.normal_temperature) && p.normal_temperature > 0 &&
         Finite(p.wall_temperature) && p.wall_temperature > 0 && Finite(p.wall_viscosity) && p.wall_viscosity > 0 &&
         Finite(p.normal_viscosity) && p.normal_viscosity > 0 && Finite(p.conductivity) && p.conductivity > 0 &&
         Finite(p.distance) && p.distance > 0 && Finite(p.velocity) && p.velocity > 0 && Finite(p.heat_flux) &&
         Finite(p.gas_constant) && p.gas_constant > 0 && Finite(p.cp) && p.cp > 0 &&
         Finite(p.recovery) && p.recovery > 0 && Finite(p.kappa) && p.kappa > 0 && Finite(p.B);
}

inline bool Evaluate(const Input& p, const su2double& u_tau, State& out) {
  if (!Valid(p) || !Finite(u_tau) || u_tau <= 0) return false;
  using std::asin;
  using std::exp;
  using std::sqrt;
  State state;
  state.u_tau = u_tau;
  state.temperature = p.wall_temperature;
  if (!p.isothermal) {
    /*--- Solve the local thermal relation before evaluating the shear residual. ---*/
    const su2double heating = p.recovery * p.velocity * p.velocity / (2 * p.cp);
    const su2double transfer = p.heat_flux * p.wall_viscosity * p.gas_constant * p.velocity /
                              (p.pressure * p.conductivity * u_tau * u_tau);
    const su2double denominator = 1 + transfer;
    if (!Finite(denominator) || denominator <= 0) return false;
    state.temperature = (p.normal_temperature + heating) / denominator;
  }
  if (!Finite(state.temperature) || state.temperature <= 0) return false;
  state.density = p.pressure / (p.gas_constant * state.temperature);
  const su2double up = p.velocity / u_tau;
  const su2double gamma = p.recovery * u_tau * u_tau / (2 * p.cp * state.temperature);
  if (!Finite(gamma) || gamma <= 0 || !Finite(state.density) || state.density <= 0) return false;
  const su2double beta = p.heat_flux * p.wall_viscosity /
                        (state.density * state.temperature * p.conductivity * u_tau);
  const su2double q = sqrt(beta * beta + 4 * gamma);
  if (!Finite(q) || q <= 0) return false;
  const su2double a = (2 * gamma * up - beta) / q;
  const su2double b = -beta / q;
  if (!Finite(a) || !Finite(b) || a <= -1 || a >= 1 || b <= -1 || b >= 1) return false;
  // shortcut: heated-wall angles lose accuracy in float; reformulate before validating single-precision use.
  const su2double exponent = p.kappa / sqrt(gamma) * (asin(a) - asin(b));
  if (!Finite(exponent) || SU2_TYPE::GetValue(exponent) > 700) return false;
  const su2double scale = exp(-p.kappa * p.B);
  const su2double white = scale * exp(exponent);
  const su2double ku = p.kappa * up;
  const su2double model_y = up + scale * (ExpMinusOne(exponent) - ku - ku * ku / 2 - ku * ku * ku / 6);
  state.y_plus = state.density * u_tau * p.distance / p.wall_viscosity;
  state.residual = state.y_plus - model_y;
  /*--- Retain the existing eddy-viscosity closure while changing only the root solve. ---*/
  const su2double white_gradient = 2 * white * p.kappa * sqrt(gamma) / q * sqrt(1 - a * a);
  state.eddy_viscosity = p.wall_viscosity *
      (1 + white_gradient - p.kappa * scale * (1 + ku + ku * ku / 2) - p.normal_viscosity / p.wall_viscosity);
  if (!Finite(state.residual) || !Finite(state.y_plus) || !Finite(state.eddy_viscosity)) return false;
  state.eddy_viscosity = std::max(su2double(1e-6), state.eddy_viscosity);
  if (p.heat_flux == 0) {
    state.derivative = state.density * p.distance / p.wall_viscosity + up / u_tau +
        (white * exponent - scale * (ku + ku * ku + ku * ku * ku / 2)) / u_tau;
  }
  out = state;
  return true;
}

inline su2double Derivative(const Input& p, const State& state) {
  if (p.heat_flux == 0) return state.derivative;
  const su2double step = state.u_tau * std::max(1e-5, std::cbrt(double(std::numeric_limits<passivedouble>::epsilon())));
  State left, right;
  if (!Evaluate(p, state.u_tau - step, left) || !Evaluate(p, state.u_tau + step, right)) return 0.0;
  return (right.residual - left.residual) / (2 * step);
}

inline bool Converged(const State& state) {
  using std::abs;
  const double relative = std::max(1e-10, 32.0 * std::numeric_limits<passivedouble>::epsilon());
  return abs(state.residual) <= 1e-12 + relative * std::max(su2double(1), su2double(abs(state.y_plus)));
}

inline bool Finish(const Input& p, State state, State& out) {
  /*--- Newton polishing gives the accepted root its implicit sensitivity after bracketed steps. ---*/
  for (int i = 0; i < 2; ++i) {
    const auto derivative = Derivative(p, state);
    if (!Finite(derivative) || derivative <= 0) return false;
    State next;
    if (!Evaluate(p, state.u_tau - state.residual / derivative, next) || !Converged(next)) return false;
    state = next;
  }
  out = state;
  return true;
}

inline bool Solve(const Input& p, su2double seed, unsigned short max_iterations, su2double relaxation, State& out) {
  if (!Valid(p) || !Finite(seed) || seed <= 0 || !Finite(relaxation) || relaxation <= 0 ||
      relaxation > 1 || max_iterations == 0) return false;
  using std::abs;
  using std::sqrt;
  su2double domain_min = 0.0;
  const su2double thermal_bound = -p.heat_flux * p.wall_viscosity * p.gas_constant * p.velocity /
                                 (p.pressure * p.conductivity);
  if (!p.isothermal && thermal_bound > 0) domain_min = sqrt(thermal_bound);
  seed = std::max(seed, su2double(domain_min * 1.01 + 1e-12));
  su2double low = 0.0, high = seed;
  bool have_low = false, have_high = false;
  State state;
  for (int i = 0; i < 64; ++i) {
    if (Evaluate(p, high, state)) {
      if (Converged(state)) return Finish(p, state, out);
      if (state.residual >= 0) { have_high = true; break; }
      low = high;
      have_low = true;
    }
    high *= 2;
    if (!Finite(high)) return false;
  }
  if (!have_high) return false;
  if (!have_low) {
    for (int i = 0; i < 64; ++i) {
      const su2double trial = (domain_min + high) / 2;
      if (!Evaluate(p, trial, state)) { domain_min = trial; continue; }
      if (Converged(state)) return Finish(p, state, out);
      if (state.residual < 0) { low = trial; have_low = true; break; }
      high = trial;
    }
  }
  if (!have_low) return false;
  su2double trial = (low + high) / 2;
  if (!Evaluate(p, trial, state)) return false;
  for (auto i = 0u; i < max_iterations; ++i) {
    if (Converged(state)) return Finish(p, state, out);
    if (state.residual < 0) low = trial;
    else high = trial;
    const auto derivative = Derivative(p, state);
    const su2double next = (Finite(derivative) && derivative > 0) ? su2double(trial - relaxation * state.residual / derivative) :
                                                                  su2double((low + high) / 2);
    State candidate;
    if (!Finite(next) || next <= low || next >= high || !Evaluate(p, next, candidate) ||
        abs(candidate.residual) >= abs(state.residual)) {
      trial = (low + high) / 2;
      if (!Evaluate(p, trial, state)) return false;
    } else {
      trial = next;
      state = candidate; // Reuse the accepted evaluation instead of recording it twice on the AD tape.
    }
  }
  return false;
}

}  // namespace WallFunctionModel
