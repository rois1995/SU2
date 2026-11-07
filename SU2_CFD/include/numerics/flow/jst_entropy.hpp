/*!
 * \file jst_entropy.hpp
 * \brief JST_ENTROPY_BUDGET: the node budget of the JST fourth-order term (P1_JST_ENTROPY.md, Theorem J4).
 * \note The JST dissipation of an edge ik is d_ik = c2 (Ut_i - Ut_k) - c4 (L_i - L_k), c2 = eps2 lam, c4 = eps4 lam,
 *       Ut = (rho, m, rho H), L the undivided Laplacian of Ut (CEulerSolver::SetUndivided_Laplacian). The eps2 part is
 *       entropy dissipative (Lemma J1). With JST_ENTROPY_BUDGET the fourth-order term is the matched one (Theorem J7):
 *       with N*(i) the JST boundary rule, chi_ik = [k in N*(i)], cb_i the mean of c4 over N*(i) and
 *       Lh_i = sum_{N*(i)} (c4/cb_i)(Ut_k - Ut_i), the eps4 flux is -c4 (chi_ik (1-theta_i) Lh_i - chi_ki (1-theta_k) Lh_k).
 *       Its node production Pi4_i = -Lh_i . Lam_i(w), Lam_i(w) = sum_{N*(i)} c4 (w_k - w_i), is a square to leading order for
 *       any coefficients (the inner and outer weights match). With B_i = P2_i + Pi4_i, theta_i = B_i^+ / (Pi4_i + D_i) and
 *       the floor 1/2 (theta_i + theta_k) mu_ik (U_i - U_k), mu_ik = c4 (n_i + n_k + 2)/2, the production of all JST terms
 *       is sum_i min(B_i, 0) <= 0 for any states, coefficients and mesh.
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

namespace JSTEntropy {

constexpr unsigned short MAXNVAR = 5;

/*!
 * \brief Node state of the budget: conservative U = (rho, m, rhoE), Ut = (rho, m, rho H) and the entropy variables
 *        w = ((gamma - s)/(gamma-1) - beta |u|^2, 2 beta u, -2 beta), beta = rho/(2p), s = ln p - gamma ln rho, of U.
 *        With the matched SST form all of them are of the hat state (H - k).
 */
struct NodeState {
  su2double U[MAXNVAR] = {0.0}, Ut[MAXNVAR] = {0.0}, w[MAXNVAR] = {0.0};
};

/*!
 * \brief NodeState from the primitives rho, u, p, h (h = (rho E + p)/rho, hat with the matched form).
 */
inline void SetState(unsigned short nDim, const su2double& gamma, const su2double& rho, const su2double* vel,
                     const su2double& p, const su2double& h, NodeState& S) {
  const su2double beta = 0.5 * rho / p, s = log(p) - gamma * log(rho);
  su2double vel2 = 0.0;
  S.U[0] = S.Ut[0] = rho;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    vel2 += vel[iDim] * vel[iDim];
    S.U[iDim + 1] = S.Ut[iDim + 1] = rho * vel[iDim];
    S.w[iDim + 1] = 2.0 * beta * vel[iDim];
  }
  S.Ut[nDim + 1] = rho * h;
  S.U[nDim + 1] = rho * h - p;
  S.w[0] = (gamma - s) / (gamma - 1.0) - beta * vel2;
  S.w[nDim + 1] = -2.0 * beta;
}

/*!
 * \brief Accumulators of node i over its edges ik: P2_i and D_i over all edges, Lam_i over N*(i) only (Theorem J7).
 *        Call once per edge of i.
 * \param[in] c2, c4 - eps2 lam and eps4 lam of the edge (after the weak-wall factor).
 * \param[in] mu - c4 (n_i + n_k + 2)/2.
 * \param[in] inRing - k in N*(i) (chi_ik).
 */
inline void AddEdge(unsigned short nVar, const NodeState& Si, const NodeState& Sk, const su2double& c2,
                    const su2double& c4, const su2double& mu, bool inRing, su2double& P2, su2double* Lam, su2double& D) {
  su2double dwUt = 0.0, dwU = 0.0;
  for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
    const su2double dw = Si.w[iVar] - Sk.w[iVar];
    dwUt += dw * (Si.Ut[iVar] - Sk.Ut[iVar]);
    dwU += dw * (Si.U[iVar] - Sk.U[iVar]);
    if (inRing) Lam[iVar] += c4 * (Sk.w[iVar] - Si.w[iVar]);
  }
  P2 -= 0.5 * c2 * dwUt;
  D += 0.5 * mu * dwU;
}

/*!
 * \brief Matched Laplacian of node i from its N*(i) edges: sum = sum c4 (Ut_k - Ut_i), cSum = sum c4, nRing = |N*(i)|.
 *        cb = cSum / nRing and Lh = sum / cb; both 0 if cb = 0 (then every eps4 coefficient of the ring is 0).
 */
inline void MatchedLaplacian(unsigned short nVar, const su2double* sum, const su2double& cSum, unsigned short nRing,
                             su2double* Lh, su2double& cb) {
  cb = (nRing > 0) ? su2double(cSum / nRing) : su2double(0.0);
  for (unsigned short iVar = 0; iVar < nVar; ++iVar) Lh[iVar] = (cb > 0.0) ? su2double(sum[iVar] / cb) : su2double(0.0);
}

/*!
 * \brief theta_i = B_i^+ / max(Pi4_i + D_i, B_i^+), B_i = P2_i + Pi4_i, Pi4_i = -Lh_i . Lam_i; in [0, 1].
 */
inline su2double NodeTheta(unsigned short nVar, const su2double& P2, const su2double* Lh, const su2double* Lam,
                           const su2double& D) {
  su2double Pi4 = 0.0;
  for (unsigned short iVar = 0; iVar < nVar; ++iVar) Pi4 -= Lh[iVar] * Lam[iVar];
  const su2double B = P2 + Pi4;
  if (!(B > 0.0)) return 0.0;
  const su2double den = fmax(Pi4 + D, B);
  return B / den;
}

/*!
 * \brief The correction of the JST flux of edge ik (added to R_i, subtracted from R_k): the matched, budgeted eps4 flux
 *        minus the kernel's -c4 (L_i - L_k), plus the floor:
 *        -c4 [(chi_ik (1-theta_i) Lh_i - L_i) - (chi_ki (1-theta_k) Lh_k - L_k)] + 1/2 (theta_i + theta_k) mu (U_i - U_k).
 */
inline void CorrectionFlux(unsigned short nVar, const su2double& c4, const su2double& mu, const su2double& theta_i,
                           const su2double& theta_k, bool chi_ik, bool chi_ki, const su2double* Lh_i,
                           const su2double* Lh_k, const su2double* L_i, const su2double* L_k, const su2double* U_i,
                           const su2double* U_k, su2double* f) {
  const su2double floor = 0.5 * (theta_i + theta_k) * mu;
  const su2double a_i = chi_ik ? su2double(1.0 - theta_i) : su2double(0.0);
  const su2double a_k = chi_ki ? su2double(1.0 - theta_k) : su2double(0.0);
  for (unsigned short iVar = 0; iVar < nVar; ++iVar)
    f[iVar] = -c4 * ((a_i * Lh_i[iVar] - L_i[iVar]) - (a_k * Lh_k[iVar] - L_k[iVar])) + floor * (U_i[iVar] - U_k[iVar]);
}

/*!
 * \brief Factors of dUt/dU in the frozen-theta Jacobian of CorrectionFlux, in SU2's surrogate convention for L (full
 *        neighbour counts n, neighbour-of-neighbour terms dropped), without fixFactor and the floor:
 *        d f/dU_i ~ fii dUt/dU, d f/dU_k ~ fik dUt/dU, rho_ik = c4/cb_i, rho_ki = c4/cb_k (0 if cb = 0).
 */
inline void CorrectionJacobianFactors(const su2double& c4, const su2double& theta_i, const su2double& theta_k, bool chi_ik,
                                      bool chi_ki, const su2double& n_i, const su2double& n_k, const su2double& cb_i,
                                      const su2double& cb_k, su2double& fii, su2double& fik) {
  const su2double a_i = chi_ik ? su2double(1.0 - theta_i) : su2double(0.0);
  const su2double a_k = chi_ki ? su2double(1.0 - theta_k) : su2double(0.0);
  const su2double rho_ik = (cb_i > 0.0) ? su2double(c4 / cb_i) : su2double(0.0);
  const su2double rho_ki = (cb_k > 0.0) ? su2double(c4 / cb_k) : su2double(0.0);
  fii = c4 * (a_i * n_i + a_k * rho_ki - n_i - 1.0);
  fik = -c4 * (a_i * rho_ik + a_k * n_k - n_k - 1.0);
}

}  // namespace JSTEntropy
