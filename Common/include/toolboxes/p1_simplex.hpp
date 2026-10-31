/*!
 * \file p1_simplex.hpp
 * \brief Linear (P1) shape function data of triangles and tetrahedra.
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

#include "../basic_types/datatype_structure.hpp"

namespace P1Simplex {

/*!
 * \brief Gradients of the shape functions of a triangle (nDim=2) or tetrahedron (nDim=3).
 * \param[in] nDim - Number of dimensions, the simplex has nDim+1 nodes.
 * \param[in] x - Coordinates, x[iNode][iDim].
 * \param[out] grad - Gradients, grad[iNode][iDim].
 * \return Signed area or volume, positive when the node order gives a positive determinant.
 */
template <class Coords>
inline su2double ShapeGradients(unsigned short nDim, const Coords& x, su2double grad[][3]) {
  su2double a[3] = {}, b[3] = {}, c[3] = {};
  for (auto iDim = 0u; iDim < nDim; ++iDim) {
    a[iDim] = x[1][iDim] - x[0][iDim];
    b[iDim] = x[2][iDim] - x[0][iDim];
    if (nDim == 3) c[iDim] = x[3][iDim] - x[0][iDim];
  }

  if (nDim == 2) {
    const su2double det = a[0] * b[1] - a[1] * b[0];
    grad[1][0] = b[1] / det;
    grad[1][1] = -b[0] / det;
    grad[2][0] = -a[1] / det;
    grad[2][1] = a[0] / det;
    grad[0][0] = -grad[1][0] - grad[2][0];
    grad[0][1] = -grad[1][1] - grad[2][1];
    return det / 2;
  }

  const su2double bc[3] = {b[1] * c[2] - b[2] * c[1], b[2] * c[0] - b[0] * c[2], b[0] * c[1] - b[1] * c[0]};
  const su2double ca[3] = {c[1] * a[2] - c[2] * a[1], c[2] * a[0] - c[0] * a[2], c[0] * a[1] - c[1] * a[0]};
  const su2double ab[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
  const su2double det = a[0] * bc[0] + a[1] * bc[1] + a[2] * bc[2];

  for (auto iDim = 0u; iDim < 3; ++iDim) {
    grad[1][iDim] = bc[iDim] / det;
    grad[2][iDim] = ca[iDim] / det;
    grad[3][iDim] = ab[iDim] / det;
    grad[0][iDim] = -grad[1][iDim] - grad[2][iDim] - grad[3][iDim];
  }
  return det / 6;
}

}  // namespace P1Simplex
