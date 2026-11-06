/*!
 * \file p1_ale_entropy.hpp
 * \brief Boundary closure C of the P1 entropy methods on moving meshes (GRID_ENTROPY_P1 with BC_ENTROPY_P1 and
 *        BC_WALL_ENTROPY_P1), ideal gas, scalar code.
 * \version 8.6.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cmath>
#include "../../../../Common/include/basic_types/datatype_structure.hpp"

/*--- Entropy U = -rho s/(gamma-1), s = ln(p rho^-gamma), entropy variables w = ((gamma-s)/(gamma-1) - beta|u|^2,
 * 2 beta u, -2 beta), beta = rho/(2p), flux potential psi = rho u, temporal potential phi = w.U - U = rho.
 *
 * Closure C, ALE form (P1_ALE.md, 6.1, Theorem A13). On a boundary simplex face with outward area vector A_f nu,
 * points x_q with the degree 2 rules of the static closure (2-point Gauss on segments, (2/3,1/6,1/6) on triangles),
 * N_q = a_q A_f nu, w~_q = sum_k phi_k(x_q) w_k, u~_q = u(w~_q), v_q = sum_k phi_k(x_q) v_k (grid velocities):
 *   gamma_iq = 0.5 (v_i + v_q).N_q + kappa_q,     gamma_q = v_q.N_q + kappa_q = sum_k phi_k(x_q) gamma_qk,
 *   F^A(a, b; N, gamma) = F_IR(a, b; N) - ubar^EC(a, b) gamma,
 *   b_i = sum_q phi_i(x_q) [ F^A(u_i, u~_q; N_q, gamma_iq) - sum_k phi_k(x_q) F^A(u~_q, u_k; N_q, gamma_qk) + F*_q ],
 * with the moving pressure wall F*_q = (0, p~ N_q, p~ gamma_q) (Lemma A14, entropy conservative at any state) and at
 * the far field F*_q = F^A(u~_q, g_q; N_q, gamma_q) - lam/2 (U(g_q) - U(u~_q)), lam = max(|u.N_q - gamma_q|) + c|N_q|
 * over u~_q and the characteristic ghost g_q of the relative normal velocity (Lemma A15). The boundary grid flux is
 * g^b_i = sum_q phi_i(x_q) gamma_iq: its v part is the 5:1 / 6:1:1 closure of CGeometry::SetGridFluxP1 with
 * BC_WEAK_P1, and kappa_q = kappa_f of CGeometry::BoundaryFaceKappa gives the nodal swept remainder, because
 * sum_q phi_a(x_q) = 1 for both rules. Then
 *   sum_i [psi_i.beta_i - phi_i g^b_i - w_i.b_i] = sum_q [psi~_q.N_q - phi~_q gamma_q - w~_q.F*_q]  (exact),
 * which is zero at walls. With v = 0 and kappa = 0 the closure is the static one of BC_ENTROPY_P1. ---*/

namespace P1AleEntropy {

constexpr unsigned short MAXNVAR = 5;

struct State {
  su2double rho = 1.0, vel[3] = {0.0, 0.0, 0.0}, p = 1.0;
};

/*--- Logarithmic mean in the stable form of Ismail and Roe (2009), as EsLogMean of the static closure. ---*/
inline su2double LogMean(const su2double& a, const su2double& b) {
  const su2double f = (a - b) / (a + b);
  const su2double u = f * f;
  if (u < 1e-2) return (a + b) / (2.0 * (1.0 + u * (1.0 / 3.0 + u * (1.0 / 5.0 + u * (1.0 / 7.0)))));
  return (a + b) * f / log(a / b);
}

/*--- Ismail-Roe entropy conservative flux on "normal", the formulas of ismailRoeProjFlux. ---*/
inline void IsmailRoeFlux(unsigned short nDim, const su2double& gamma, const State& L, const State& R,
                          const su2double* normal, su2double* flux) {
  const su2double zL = sqrt(L.rho / L.p), zR = sqrt(R.rho / R.p);
  const su2double yL = zL * L.p, yR = zR * R.p;
  const su2double zAvg = 0.5 * (zL + zR), yAvg = 0.5 * (yL + yR);
  const su2double zLog = LogMean(zL, zR), yLog = LogMean(yL, yR);

  const su2double density = zAvg * yLog;
  su2double vel[3] = {0.0, 0.0, 0.0}, vel2 = 0.0, velN = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    vel[iDim] = 0.5 * (zL * L.vel[iDim] + zR * R.vel[iDim]) / zAvg;
    vel2 += vel[iDim] * vel[iDim];
    velN += vel[iDim] * normal[iDim];
  }
  const su2double pressure1 = yAvg / zAvg;
  const su2double pressure2 = ((gamma + 1.0) * yLog / zLog + (gamma - 1.0) * pressure1) / (2.0 * gamma);
  const su2double enthalpy = gamma * pressure2 / ((gamma - 1.0) * density) + 0.5 * vel2;

  const su2double mdot = density * velN;
  flux[0] = mdot;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) flux[iDim + 1] = mdot * vel[iDim] + normal[iDim] * pressure1;
  flux[nDim + 1] = mdot * enthalpy;
}

/*--- Temporal entropy conservative average (Lemma A6): [[w]].ubar = [[rho]], ubar(U,U) = U, symmetric. ---*/
inline void TemporalAverage(unsigned short nDim, const su2double& gamma, const State& L, const State& R,
                            su2double* ubar) {
  const su2double rhoLog = LogMean(L.rho, R.rho);
  const su2double betaLog = LogMean(0.5 * L.rho / L.p, 0.5 * R.rho / R.p);
  su2double velAvg2 = 0.0, vel2Avg = 0.0;
  ubar[0] = rhoLog;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    const su2double velAvg = 0.5 * (L.vel[iDim] + R.vel[iDim]);
    velAvg2 += velAvg * velAvg;
    vel2Avg += 0.5 * (L.vel[iDim] * L.vel[iDim] + R.vel[iDim] * R.vel[iDim]);
    ubar[iDim + 1] = rhoLog * velAvg;
  }
  ubar[nDim + 1] = rhoLog * (0.5 / ((gamma - 1.0) * betaLog) + velAvg2 - 0.5 * vel2Avg);
}

inline void EntropyVariables(unsigned short nDim, const su2double& gamma, const State& S, su2double* w) {
  const su2double beta = 0.5 * S.rho / S.p;
  su2double vel2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) vel2 += S.vel[iDim] * S.vel[iDim];
  const su2double entropy = log(S.p) - gamma * log(S.rho);
  w[0] = (gamma - entropy) / (gamma - 1.0) - beta * vel2;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) w[iDim + 1] = 2.0 * beta * S.vel[iDim];
  w[nDim + 1] = -2.0 * beta;
}

/*--- Inverse map u(w), admissible for w_E < 0 (convex combinations keep it). ---*/
inline void StateFromEntropyVariables(unsigned short nDim, const su2double& gamma, const su2double* w, State& S) {
  const su2double beta = -0.5 * w[nDim + 1];
  su2double vel2 = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    S.vel[iDim] = 0.5 * w[iDim + 1] / beta;
    vel2 += S.vel[iDim] * S.vel[iDim];
  }
  const su2double entropy = gamma - (gamma - 1.0) * (w[0] + beta * vel2);
  S.rho = exp((log(2.0 * beta) + entropy) / (1.0 - gamma));
  S.p = 0.5 * S.rho / beta;
}

inline void Conservative(unsigned short nDim, const su2double& gamma, const State& S, su2double* U) {
  su2double vel2 = 0.0;
  U[0] = S.rho;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    U[iDim + 1] = S.rho * S.vel[iDim];
    vel2 += S.vel[iDim] * S.vel[iDim];
  }
  U[nDim + 1] = S.p / (gamma - 1.0) + 0.5 * S.rho * vel2;
}

/*--- Characteristic far-field ghost of the static closure with the normal velocities relative to the boundary,
 * which moves with the normal speed gridSpeed: the Riemann invariants and the inflow/outflow switch use u.nu -
 * gridSpeed, the ghost normal velocity is the relative one plus gridSpeed. gridSpeed = 0 is the static ghost. ---*/
inline void RelativeFarFieldGhost(unsigned short nDim, const su2double& gamma, const State& S, const State& Inf,
                                  const su2double* unitNormal, const su2double& gridSpeed, State& G) {
  const su2double gm1 = gamma - 1.0;
  su2double VnB = -gridSpeed, VnI = -gridSpeed;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    VnB += S.vel[iDim] * unitNormal[iDim];
    VnI += Inf.vel[iDim] * unitNormal[iDim];
  }
  const su2double cB = sqrt(gamma * S.p / S.rho), cI = sqrt(gamma * Inf.p / Inf.rho);
  const su2double entropyB = pow(S.rho, gamma) / S.p, entropyI = pow(Inf.rho, gamma) / Inf.p;

  su2double rPlus, rMinus;
  if (VnI > -cI) rPlus = VnB + 2.0 * cB / gm1;
  else rPlus = VnI + 2.0 * cI / gm1;
  if (VnI > cI) rMinus = VnB - 2.0 * cB / gm1;
  else rMinus = VnI - 2.0 * cI / gm1;
  const su2double Vn = 0.5 * (rPlus + rMinus);
  const su2double c = 0.25 * (rPlus - rMinus) * gm1;

  const bool outflow = (VnI > 0.0);
  const su2double entropy = outflow ? entropyB : entropyI;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    if (outflow) G.vel[iDim] = S.vel[iDim] + (Vn - VnB) * unitNormal[iDim];
    else G.vel[iDim] = Inf.vel[iDim] + (Vn - VnI) * unitNormal[iDim];
  }
  G.rho = pow(entropy * c * c / gamma, 1.0 / gm1);
  G.p = G.rho * c * c / gamma;
}

/*--- F^A(a, b; N, gamma) = F_IR(a, b; N) - ubar^EC(a, b) gamma. ---*/
inline void AleFlux(unsigned short nDim, const su2double& gamma, const State& a, const State& b, const su2double* N,
                    const su2double& gridFlux, su2double* flux) {
  su2double ubar[MAXNVAR] = {0.0};
  IsmailRoeFlux(nDim, gamma, a, b, N, flux);
  TemporalAverage(nDim, gamma, a, b, ubar);
  for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) flux[iVar] -= ubar[iVar] * gridFlux;
}

/*--- Face rule of closure C: phi[q][k] = phi_k(x_q), a_q = weight. ---*/
inline su2double FaceRule(unsigned short nNodes, su2double (*phi)[3]) {
  for (unsigned short q = 0; q < 3; ++q)
    for (unsigned short k = 0; k < 3; ++k) phi[q][k] = 0.0;
  if (nNodes == 2) {
    const su2double g = 0.5 / sqrt(3.0);
    phi[0][0] = 0.5 + g;
    phi[0][1] = 0.5 - g;
    phi[1][0] = 0.5 - g;
    phi[1][1] = 0.5 + g;
    return 0.5;
  }
  for (unsigned short q = 0; q < 3; ++q)
    for (unsigned short k = 0; k < 3; ++k) phi[q][k] = (q == k) ? 2.0 / 3.0 : 1.0 / 6.0;
  return 1.0 / 3.0;
}

/*!
 * \brief Closure C-ALE on one boundary face (segment: 2 nodes, triangle: 3 nodes).
 * \param[in] inf - Free stream state for the far field, nullptr for a (moving) pressure wall.
 * \param[in] faceNormal - Outward area vector A_f nu.
 * \param[in] nodeState - States of the face nodes.
 * \param[in] gridVel - Grid velocities of the face nodes.
 * \param[in] kappa - Point grid flux kappa_q = kappa_f of the nodal swept remainder (0 semi-discretely).
 * \param[out] res - res[i][iVar], the boundary residual b_i of the face nodes, overwritten.
 */
inline void FaceResidual(unsigned short nDim, unsigned short nNodes, const su2double& gamma, const State* inf,
                         const su2double* faceNormal, const State* nodeState, const su2double (*gridVel)[3],
                         const su2double& kappa, su2double (*res)[MAXNVAR]) {
  const unsigned short nVar = nDim + 2;

  su2double phi[3][3];
  const su2double weight = FaceRule(nNodes, phi);

  su2double area = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) area += faceNormal[iDim] * faceNormal[iDim];
  area = sqrt(area);
  su2double unitNormal[3] = {0.0, 0.0, 0.0}, pointNormal[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    unitNormal[iDim] = faceNormal[iDim] / area;
    pointNormal[iDim] = weight * faceNormal[iDim];
  }
  const su2double pointArea = weight * area;

  su2double w[3][MAXNVAR] = {{0.0}};
  for (unsigned short k = 0; k < nNodes; ++k) EntropyVariables(nDim, gamma, nodeState[k], w[k]);

  for (unsigned short i = 0; i < nNodes; ++i)
    for (unsigned short iVar = 0; iVar < nVar; ++iVar) res[i][iVar] = 0.0;

  for (unsigned short q = 0; q < nNodes; ++q) {
    su2double wq[MAXNVAR] = {0.0}, vq[3] = {0.0, 0.0, 0.0};
    for (unsigned short k = 0; k < nNodes; ++k) {
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) wq[iVar] += phi[q][k] * w[k][iVar];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) vq[iDim] += phi[q][k] * gridVel[k][iDim];
    }
    State sq;
    StateFromEntropyVariables(nDim, gamma, wq, sq);

    su2double gammaQ = kappa;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) gammaQ += vq[iDim] * pointNormal[iDim];

    /*--- G_qk = F^A(u~_q, u_k; N_q, gamma_qk) once per node, F^A and gamma_qk are symmetric. ---*/
    su2double G[3][MAXNVAR] = {{0.0}}, Gbar[MAXNVAR] = {0.0};
    for (unsigned short k = 0; k < nNodes; ++k) {
      su2double gammaQK = kappa;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) gammaQK += 0.5 * (gridVel[k][iDim] + vq[iDim]) * pointNormal[iDim];
      AleFlux(nDim, gamma, sq, nodeState[k], pointNormal, gammaQK, G[k]);
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) Gbar[iVar] += phi[q][k] * G[k][iVar];
    }

    su2double Fstar[MAXNVAR] = {0.0};
    if (inf == nullptr) {
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) Fstar[iDim + 1] = sq.p * pointNormal[iDim];
      Fstar[nDim + 1] = sq.p * gammaQ;
    } else {
      const su2double gridSpeed = gammaQ / pointArea;
      State gq;
      RelativeFarFieldGhost(nDim, gamma, sq, *inf, unitNormal, gridSpeed, gq);
      AleFlux(nDim, gamma, sq, gq, pointNormal, gammaQ, Fstar);
      su2double velNq = -gridSpeed, velNg = -gridSpeed;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        velNq += sq.vel[iDim] * unitNormal[iDim];
        velNg += gq.vel[iDim] * unitNormal[iDim];
      }
      const su2double lamQ = fabs(velNq) + sqrt(gamma * sq.p / sq.rho);
      const su2double lamG = fabs(velNg) + sqrt(gamma * gq.p / gq.rho);
      const su2double lam = fmax(lamQ, lamG);
      su2double Uq[MAXNVAR] = {0.0}, Ug[MAXNVAR] = {0.0};
      Conservative(nDim, gamma, sq, Uq);
      Conservative(nDim, gamma, gq, Ug);
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) Fstar[iVar] -= 0.5 * lam * pointArea * (Ug[iVar] - Uq[iVar]);
    }

    for (unsigned short i = 0; i < nNodes; ++i)
      for (unsigned short iVar = 0; iVar < nVar; ++iVar) res[i][iVar] += phi[q][i] * (G[i][iVar] - Gbar[iVar] + Fstar[iVar]);
  }
}

}  // namespace P1AleEntropy
