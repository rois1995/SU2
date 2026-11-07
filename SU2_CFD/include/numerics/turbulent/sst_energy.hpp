/*!
 * \file sst_energy.hpp
 * \brief SST_ENERGY_CONSISTENT: the k diffusion of the SST k row, for the total-energy flux of the flow.
 * \note The k row (CScalarFlux_SST::coefficients and CAvgGradScalarBase::diffusionTerms, interior edges, corrected
 *       gradient) has the edge flux -D_ij g_ij, added to the residual of i and subtracted from that of j, with
 *         D_ij = 0.5 [(mu_i + s_i mu_t,i) + (mu_j + s_j mu_t,j)],   s = F1 sigma_k1 + (1 - F1) sigma_k2,
 *         g_ij = (gbar - c e) . n,   gbar = (grad k_i + grad k_j)/2,   c = (gbar . e - (k_j - k_i)) / max(|e|^2, EPS),
 *       e = x_j - x_i and n the edge normal. With SST_ENERGY_CONSISTENT the energy row gets the same number, so the
 *       energy flux holds (mu + sigma_k mu_t) grad k, as in the k equation (P1_SST_COUPLED.md, section 4 and M3).
 *       The functions below repeat the operations of the k row in the same order; the k row itself is not changed.
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
#include "../../../../Common/include/option_structure.hpp"

namespace SSTEnergy {

/*! \brief sigma_k of the two SST layers (CScalarFlux_SST asserts that its constants are these). */
constexpr passivedouble sigma_k1 = 0.85;
constexpr passivedouble sigma_k2 = 1.0;

/*!
 * \brief Edge diffusivity D_ij of the SST k row.
 */
template <class T>
FORCEINLINE T EdgeDiffusivity(const T& mu_i, const T& muT_i, const T& F1_i, const T& mu_j, const T& muT_j,
                              const T& F1_j) {
  const T sigma_kine_i = F1_i * sigma_k1 + (1.0 - F1_i) * sigma_k2;
  const T sigma_kine_j = F1_j * sigma_k1 + (1.0 - F1_j) * sigma_k2;
  return 0.5 * ((mu_i + sigma_kine_i * muT_i) + (mu_j + sigma_kine_j * muT_j));
}

/*!
 * \brief Corrected projected gradient g_ij of the SST k row.
 * \param[in] grad_i, grad_j - Nodal gradients of k (indexable, nDim entries).
 * \param[in] k_i, k_j - Nodal k.
 * \param[in] vector_ij - x_j - x_i.
 * \param[in] normal - Edge normal (not unit).
 */
template <class T, class GradType, class VecType, class NormalType>
FORCEINLINE T CorrectedProjection(int nDim, const GradType& grad_i, const GradType& grad_j, const T& k_i,
                                  const T& k_j, const VecType& vector_ij, const NormalType& normal) {
  T dist2_ij = 0.0;
  for (int iDim = 0; iDim < nDim; ++iDim) dist2_ij += vector_ij[iDim] * vector_ij[iDim];
  dist2_ij = fmax(dist2_ij, EPS);

  T avgGrad[3] = {0.0, 0.0, 0.0};
  for (int iDim = 0; iDim < nDim; ++iDim) avgGrad[iDim] = 0.5 * (grad_i[iDim] + grad_j[iDim]);

  T proj_e = 0.0;
  for (int iDim = 0; iDim < nDim; ++iDim) proj_e += avgGrad[iDim] * vector_ij[iDim];
  const T corr = (proj_e - k_j + k_i) / dist2_ij;
  for (int iDim = 0; iDim < nDim; ++iDim) avgGrad[iDim] -= corr * vector_ij[iDim];

  T projGrad = 0.0;
  for (int iDim = 0; iDim < nDim; ++iDim) projGrad += avgGrad[iDim] * normal[iDim];
  return projGrad;
}

}  // namespace SSTEnergy
