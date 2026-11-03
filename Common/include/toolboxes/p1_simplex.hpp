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

/*--- Moved (MOD_CENTROIDS) dual of a tetrahedron (P1_VISCOUS_MOVED.md, Lemma MV1). With p_o the dual point of the face
 * f_o opposite node o (CPrimalGrid::getFaceCG) and xi_o = p_o - xbar_o its shift from the face centroid, the dual
 * facets of node a inside T have the total inward area vector nu_a = (|T| I + Y^T) g_a, Y = (3|T|/2) sum_o xi_o (x) g_o,
 * independent of the cell point. On a face f_o the part of node a is a_a = |f_o| (1 - beta_a)/2 with the barycentric
 * coordinate beta_a = 1/3 + g_a.xi_o of p_o. ---*/

/*!
 * \brief Shift xi_o = p_o - xbar_o of the dual point of each face of a tetrahedron, indexed by the opposite node.
 * \param[in] element - The tetrahedron (getFaceCG, GetFaces), its dual points are those of the control volumes.
 * \param[in] x - Coordinates of its nodes, best recentred on the element (std::array of pointers, as getFaceCG).
 * \param[out] xi - xi[o][iDim].
 */
template <class Element, class Coords>
inline void FaceShifts(Element& element, const Coords& x, su2double xi[][3]) {
  for (unsigned short iFace = 0; iFace < 4; ++iFace) {
    unsigned short opposite = 6;  /*--- 0 + 1 + 2 + 3 minus the face nodes. ---*/
    su2double centroid[3] = {0.0, 0.0, 0.0}, point[3] = {0.0, 0.0, 0.0};
    for (unsigned short k = 0; k < 3; ++k) {
      const auto node = element.GetFaces(iFace, k);
      opposite -= node;
      for (unsigned short iDim = 0; iDim < 3; ++iDim) centroid[iDim] += x[node][iDim] / 3.0;
    }
    element.getFaceCG(iFace, 3, x, point);
    for (unsigned short iDim = 0; iDim < 3; ++iDim) xi[opposite][iDim] = point[iDim] - centroid[iDim];
  }
}

/*!
 * \brief Y = (3|T|/2) sum_o xi_o (x) g_o, Y[i][j] = (3|T|/2) sum_o xi_o,i g_o,j (traceless, zero iff all xi_o = 0).
 */
inline void MovedFaceTensor(const su2double& size, const su2double (*grad)[3], const su2double (*xi)[3],
                            su2double Y[][3]) {
  for (unsigned short i = 0; i < 3; ++i) {
    for (unsigned short j = 0; j < 3; ++j) {
      su2double sum = 0.0;
      for (unsigned short o = 0; o < 4; ++o) sum += xi[o][i] * grad[o][j];
      Y[i][j] = 1.5 * size * sum;
    }
  }
}

/*!
 * \brief Share a_a/|f| = (1 - beta_a)/2 = 1/3 - g_a.xi_o/2 of each node a of the face opposite node o, in [1/4, 1/2]
 *        for MOD_CENTROIDS, sum 1. frac[o] is set to 0.
 */
inline void MovedFaceShares(const su2double (*grad)[3], unsigned short opposite, const su2double* xi, su2double* frac) {
  for (unsigned short a = 0; a < 4; ++a) {
    frac[a] = 0.0;
    if (a == opposite) continue;
    su2double dot = 0.0;
    for (unsigned short iDim = 0; iDim < 3; ++iDim) dot += grad[a][iDim] * xi[iDim];
    frac[a] = 1.0 / 3.0 - 0.5 * dot;
  }
}

}  // namespace P1Simplex
