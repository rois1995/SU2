/*!
 * \file jst_entropy_fourth.hpp
 * \brief Entropy-stable JST fourth difference, R^(4) = A^T J A w.
 */

#pragma once

#include "p1_boundary_closure.hpp"

namespace JSTEntropyFourth {

inline su2double CouplingWeight(su2double chi, su2double share = 1.0) { return sqrt(share * chi); }

template <class T>
inline T StencilFlag(const T& boundary_i, const T& boundary_k) {
  return 1.0 - boundary_i * (1.0 - boundary_k);
}

/*--- q = J x, J = d(rho, rho v, rho H)/dw. Apply dP/dw as in EsProjectionJacobian,
 * then differentiate rho H = gamma p/(gamma-1) + rho |v|^2/2 (also rho Hhat with matched k). ---*/
inline void EnthalpyStateAction(unsigned short nDim, su2double gamma, const P1BoundaryClosure::EsState& state,
                                const su2double* x, su2double* q) {
  const su2double beta = 0.5 * state.rho / state.p;
  const su2double dBeta = -0.5 * x[nDim + 1];
  su2double vel2 = 0.0, velDVel = 0.0;
  su2double dVel[3];
  for (auto d = 0u; d < nDim; ++d) {
    dVel[d] = (0.5 * x[d + 1] - state.vel[d] * dBeta) / beta;
    vel2 += state.vel[d] * state.vel[d];
    velDVel += state.vel[d] * dVel[d];
  }
  const su2double dEntropy = -(gamma - 1.0) * (x[0] + vel2 * dBeta + 2.0 * beta * velDVel);
  const su2double dRho = state.rho * (dBeta / beta + dEntropy) / (1.0 - gamma);
  const su2double dPressure = state.p * (dRho / state.rho - dBeta / beta);
  q[0] = dRho;
  for (auto d = 0u; d < nDim; ++d) q[d + 1] = state.vel[d] * dRho + state.rho * dVel[d];
  q[nDim + 1] = 0.5 * vel2 * dRho + state.rho * velDVel + gamma / (gamma - 1.0) * dPressure;
}

inline void AddStencilDifference(unsigned short nVar, su2double a, const su2double* w_i, const su2double* w_k,
                                 su2double* x) {
  for (auto v = 0u; v < nVar; ++v) x[v] += a * (w_i[v] - w_k[v]);
}

/*--- The two directed entries of A give a single conservative flux in A^T q. ---*/
template <class T, class Row, class Flux>
inline void AddCouplingFlux(unsigned short nVar, const T& a, const T& s_ik, const T& s_ki, const Row& q_i,
                            const Row& q_k, Flux& flux) {
  for (auto v = 0u; v < nVar; ++v) flux[v] += a * (s_ik * q_i[v] - s_ki * q_k[v]);
}

/*--- Frozen a (sensor, radii and masks) and J. Drop third-node derivatives and replace J_k W_i by J_i W_i.
 * The hooks multiply the signed factors by fixFactor and retain the unmasked eps2 Jacobian shift.
 * With matched SST, the existing transform uses frozen nodal k and the selected face k.
 * The reducer assembles true-edge blocks only. Matrix-free products refresh q through Centered_Residual.
 * Coarse centered grids use Lax. ---*/
template <class T>
inline void JacobianCoefficients(const T& a, const T& s_ik, const T& s_ki, const T& S_i, const T& S_k,
                                 T& d_i, T& d_k) {
  d_i = a * (s_ik * S_i + s_ki * a);
  d_k = a * (s_ik * a + s_ki * S_k);
}

}  // namespace JSTEntropyFourth
