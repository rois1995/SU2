/*!
 * \file p1_viscous.hpp
 * \brief Viscous residual of one simplex with P1 shape functions (VISCOUS_SCHEME= P1_CELL).
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

namespace P1Viscous {

/*!
 * \brief Residual of the viscous flux F_v of a simplex, for every node i: R_i = |T| F_v . grad(phi_i).
 *        The sign is the one of the edge scheme, where R_i = -sum_k (F_v . n_ik): with the median dual,
 *        the dual faces of node i inside T add up to -|T| grad(phi_i).
 * \note Boundary terms are not included, they stay with the boundary conditions.
 * \param[in] nDim - Number of dimensions, the simplex has nDim+1 nodes.
 * \param[in] size - Area or volume of the simplex.
 * \param[in] grad - Gradients of the shape functions.
 * \param[in] velocity - Nodal velocity, velocity[iNode][iDim].
 * \param[in] temperature - Nodal temperature.
 * \param[in] viscosity - Nodal total viscosity, the mean is used.
 * \param[in] conductivity - Nodal total conductivity, the mean is used.
 * \param[out] residual - residual[iNode][iVar], for the compressible conservative variables.
 */
inline void CellResidual(unsigned short nDim, su2double size, const su2double grad[][3], const su2double velocity[][3],
                         const su2double* temperature, const su2double* viscosity, const su2double* conductivity,
                         su2double residual[][5]) {
  const unsigned short nNodes = nDim + 1;

  su2double gradVel[3][3] = {}, gradT[3] = {}, meanVel[3] = {}, mu = 0.0, k = 0.0;

  for (auto iNode = 0u; iNode < nNodes; ++iNode) {
    mu += viscosity[iNode] / nNodes;
    k += conductivity[iNode] / nNodes;
    for (auto iDim = 0u; iDim < nDim; ++iDim) {
      meanVel[iDim] += velocity[iNode][iDim] / nNodes;
      gradT[iDim] += temperature[iNode] * grad[iNode][iDim];
      for (auto jDim = 0u; jDim < nDim; ++jDim) gradVel[iDim][jDim] += velocity[iNode][iDim] * grad[iNode][jDim];
    }
  }

  su2double div = 0.0;
  for (auto iDim = 0u; iDim < nDim; ++iDim) div += gradVel[iDim][iDim];

  su2double tau[3][3] = {}, energyFlux[3] = {};
  for (auto iDim = 0u; iDim < nDim; ++iDim) {
    for (auto jDim = 0u; jDim < nDim; ++jDim) tau[iDim][jDim] = mu * (gradVel[iDim][jDim] + gradVel[jDim][iDim]);
    tau[iDim][iDim] -= 2.0 / 3.0 * mu * div;
  }
  for (auto iDim = 0u; iDim < nDim; ++iDim) {
    energyFlux[iDim] = k * gradT[iDim];
    for (auto jDim = 0u; jDim < nDim; ++jDim) energyFlux[iDim] += meanVel[jDim] * tau[jDim][iDim];
  }

  for (auto iNode = 0u; iNode < nNodes; ++iNode) {
    residual[iNode][0] = 0.0;
    residual[iNode][nDim + 1] = 0.0;
    for (auto iDim = 0u; iDim < nDim; ++iDim) {
      residual[iNode][iDim + 1] = 0.0;
      for (auto jDim = 0u; jDim < nDim; ++jDim) residual[iNode][iDim + 1] += size * tau[iDim][jDim] * grad[iNode][jDim];
      residual[iNode][nDim + 1] += size * energyFlux[iDim] * grad[iNode][iDim];
    }
  }
}

}  // namespace P1Viscous
