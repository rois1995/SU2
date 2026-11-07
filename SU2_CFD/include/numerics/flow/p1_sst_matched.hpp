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

}  // namespace P1SSTMatched
