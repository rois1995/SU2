/*!
 * \file p1_sst_matched.hpp
 * \brief SST with the P1 options: the matched k form of P1_SST_COUPLED.md (M1-M4, Jacobians D1-D4).
 * \note The P1 parts act on the hat state Uhat = (rho, m, rhoE - rho k). Every edge adds mdot k_up to the energy row,
 *       mdot its final mass flux and k_up the upwind k, and the SST k row uses the same mass flux, so the k parts of the
 *       energy and k rows are the same numbers (M1). Closure C adds k_b m per face node, k_b = k_a for outflow (m >= 0)
 *       and the boundary k otherwise; the SST k row gets k_a M+ + sum min(m,0) k_b from the same numbers (M2). With
 *       VISC_P1_ENTROPY and SST_ENERGY_CONSISTENT, both rows get the P1 element k diffusion (M3) and the k production
 *       is the mu_t part of the element dissipation (M4). Then the coupled entropy rate is the hat scheme's (Theorem C3).
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

#include "../../../../Common/include/basic_types/datatype_structure.hpp"
#include "../turbulent/sst_energy.hpp"

namespace P1SSTMatched {

/*!
 * \brief Upwind k of a mass flux m from i to j: k_i if m >= 0, else k_j (Lemma C4: first order, sign of m).
 */
template <class T>
FORCEINLINE T KUpwind(const T& m, const T& k_i, const T& k_j) {
  const T fromI = (m >= 0.0);
  return fromI * k_i + (1.0 - fromI) * k_j;
}

/*!
 * \brief MUSCL_TURB with the matched form (SSTPATHS_PLAN.md 2): the face k of the upwind side of the mass flux m. kL, kR
 *        are the U-MUSCL face values of the k row (turbulence gradient, limiter and kappa); a face value outside
 *        [lo, hi] falls back to its node value (the bounds of CScalarSolver), and so does the side of a strong-k node
 *        (strong = 1, a wall node of the SST: first order there, Lemma C4 / M5). Uniform k gives k for any m.
 */
template <class T>
FORCEINLINE T MusclFaceKine(const T& m, const T& kL, const T& kR, const T& k_i, const T& k_j, const T& strong_i,
                            const T& strong_j, const su2double& lo, const su2double& hi) {
  T out_i = fmax(kL < lo, kL > hi);
  T out_j = fmax(kR < lo, kR > hi);
  out_i = fmax(out_i, strong_i);
  out_j = fmax(out_j, strong_j);
  const T face_i = out_i * k_i + (1.0 - out_i) * kL;
  const T face_j = out_j * k_j + (1.0 - out_j) * kR;
  return KUpwind(m, face_i, face_j);
}

/*!
 * \brief Jacobian of the matched flux from the hat Jacobian (D1): J = L(kRow) Jhat T(kCol), with
 *        T(k) = I - k e_E e_rho^T (Uhat = T(k) U at a fixed k) and L(k) = I + k e_E e_rho^T (the row of mdot k_up):
 *        column rho -= kCol column E, then row E += kRow row rho. Matrix accessed as jac(iVar, jVar).
 */
template <class T, class MatrixType>
FORCEINLINE void TransformJacobian(unsigned short nVar, MatrixType& jac, const T& kRow, const T& kCol) {
  const unsigned short iE = nVar - 1;
  for (unsigned short iVar = 0; iVar < nVar; ++iVar) jac(iVar, 0) -= kCol * jac(iVar, iE);
  for (unsigned short jVar = 0; jVar < nVar; ++jVar) jac(iE, jVar) += kRow * jac(0, jVar);
}

/*!
 * \brief Closure C face node (M2): the k of its mass row m, k_b = k_a for outflow (m >= 0) and kBc for inflow, and the
 *        parts of the SST k row, massPlus += max(m, 0) and inflow += min(m, 0) kBc, so that k_b m = k_a max(m,0) + inflow.
 * \return k_b.
 */
inline su2double FaceKine(const su2double& m, const su2double& kNode, const su2double& kBc, su2double& massPlus,
                          su2double& inflow) {
  massPlus += fmax(m, 0.0);
  inflow += fmin(m, 0.0) * kBc;
  return (m >= 0.0) ? kNode : kBc;
}

/*!
 * \brief Matched k of a pair that is not an edge of the edge loop (split pairs, mixed-cell diagonals, SSTPATHS_PLAN.md 1):
 *        the k flux k_up m of the mass flux m from i to k, added to the energy row of the pair and, as the same number,
 *        to the SST k row (+ at i, - at k).
 */
inline su2double PairKine(const su2double& m, const su2double& k_i, const su2double& k_k) {
  return KUpwind(m, k_i, k_k) * m;
}

/*!
 * \brief Diagonal Jacobian of PairKine in the k rows w.r.t. rho k (m frozen): max(m,0)/rho_i at i, -min(m,0)/rho_k at k.
 *        The off-diagonal blocks of non-edge pairs are not in the sparsity pattern (as for the flow).
 */
inline void PairKineDiag(const su2double& m, const su2double& rho_i, const su2double& rho_k, su2double& diag_i,
                         su2double& diag_k) {
  diag_i = fmax(m, 0.0) / rho_i;
  diag_k = -fmin(m, 0.0) / rho_k;
}

/*!
 * \brief Element diffusivity of k, the vertex mean of mu + sigma_k mu_t (sigma_k blended by F1, as the k row).
 */
inline su2double ElementKineDiffusivity(unsigned short nNodes, const su2double* mu, const su2double* muT,
                                        const su2double* F1) {
  su2double D = 0.0;
  for (unsigned short a = 0; a < nNodes; ++a) {
    const su2double sigma = F1[a] * SSTEnergy::sigma_k1 + (1.0 - F1[a]) * SSTEnergy::sigma_k2;
    D += (mu[a] + sigma * muT[a]) / nNodes;
  }
  return D;
}

/*!
 * \brief P1 element k diffusion (M3): R_a = |T| D grad k_T . g_a, grad k_T = sum_b k_b g_b, the Galerkin weak form with
 *        the sign of the viscous residual (R_a = |T| F . g_a). The same function gives the energy row and the k row.
 * \param[in] nDim - Dimension, nDim + 1 nodes.
 * \param[in] size - |T|.
 * \param[in] grad - Shape gradients g_a.
 * \param[in] k - Nodal k.
 * \param[in] D - ElementKineDiffusivity.
 * \param[out] res - R_a.
 */
inline void KineDiffusion(unsigned short nDim, const su2double& size, const su2double (*grad)[3], const su2double* k,
                          const su2double& D, su2double* res) {
  const unsigned short nNodes = nDim + 1;
  su2double gradK[3] = {0.0, 0.0, 0.0};
  for (unsigned short b = 0; b < nNodes; ++b)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) gradK[iDim] += k[b] * grad[b][iDim];
  for (unsigned short a = 0; a < nNodes; ++a) {
    su2double proj = 0.0;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) proj += gradK[iDim] * grad[a][iDim];
    res[a] = size * D * proj;
  }
}

/*!
 * \brief dR_a/dk_b of KineDiffusion, |T| D g_a . g_b (D frozen).
 */
inline su2double KineDiffusionJacobian(unsigned short nDim, const su2double& size, const su2double (*grad)[3],
                                       const su2double& D, unsigned short a, unsigned short b) {
  su2double proj = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) proj += grad[a][iDim] * grad[b][iDim];
  return size * D * proj;
}

/*!
 * \brief Weak no-slip wall closure (BC_WALL_ENTROPY_P1, pressure flux of closure C) at strong-k nodes with k_D != 0
 *        (rough walls): the closure rows have nodal masses m_a = sum_q phi_a(x_q) (G_qa - Gbar_q)[0] (zero sum over the
 *        face). The energy row of the face node a gets k_D,a m_a: with U = Uhat + rho k e_E this is the k part of the
 *        closure on the node's own k (the wall rule k_b = k_a), and it is the number k_D,a R_rho,a that the Dirichlet k
 *        row stands for (Theorem C1 with Dirichlet nodes), so X_a - k_D,a R_rho,a = 0 for these terms (M5, exact).
 *        Returns the added k-row numbers in kRow (for P1KNodalRes). Smooth walls (k_D = 0) add nothing.
 */
template <class Res>
FORCEINLINE void WallNodeKine(unsigned short nFace, unsigned short nVar, const su2double* kWall, Res& res,
                              su2double* kRow) {
  for (unsigned short a = 0; a < nFace; ++a) {
    kRow[a] = kWall[a] * res[a][0];
    res[a][nVar - 1] += kRow[a];
  }
}

}  // namespace P1SSTMatched
