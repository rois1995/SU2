/*!
 * \file boundary_closure.hpp
 * \brief Boundary weights that make the median-dual edge sum exact for a P1 flux.
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

#include "../geometry/CGeometry.hpp"

namespace BoundaryClosure {

/*!
 * \brief Add the closure flux of a scalar field through marker faces, per marker vertex and component.
 *        For node i of a boundary face with outward area vector A:
 *        segment (i,j):     (A/2) (5 phi_i + phi_j) / 6
 *        triangle (i,j,k):  (A/3) (6 phi_i + phi_j + phi_k) / 8
 *        quadrilateral:     the triangle rule on its four sub-triangles tau with A_tau/2, both diagonals with weight
 *                           1/2 (EDGE_FORM_P1_MIXED, P1_MIXED.md 8.6), 9:1:1:1/48 on parallelograms
 * \note Only owned vertices are updated. phi is read at all nodes of the local faces, halos included.
 *       Every face touching an owned node is local, see DistributeSurfaceConnectivity.
 * \param[in] geometry - Fine grid, the boundary elements must have their normals (SetBoundCVCoeffs).
 * \param[in] iMarker - Boundary marker.
 * \param[in] phi - Callable returning the field at a local point.
 * \param[in,out] flux - (nVertex of the marker, nDim) container, added to.
 */
template <class Field, class Matrix>
void AddMarkerFlux(const CGeometry& geometry, unsigned short iMarker, const Field& phi, Matrix& flux) {
  const auto nDim = geometry.GetnDim();

  for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
    auto* element = geometry.bound[iMarker][iElem];
    const auto nNodes = element->GetnNodes();

    if ((nDim == 3) && (nNodes == 4)) {
      std::array<const su2double*, 4> coord;
      for (auto k = 0u; k < 4; ++k) coord[k] = geometry.nodes->GetCoord(element->GetNode(k));
      su2double subNormal[4][3];
      P1Mixed::QuadSubNormals(coord, element->getNormal(), subNormal);
      for (auto tri = 0u; tri < 4; ++tri) {
        const auto* local = P1Mixed::QuadTriangles[tri];
        for (auto a = 0u; a < 3; ++a) {
          const auto iPoint = element->GetNode(local[a]);
          if (!geometry.nodes->GetDomain(iPoint)) continue;
          su2double value = 0.0;
          for (auto b = 0u; b < 3; ++b) value += ((b == a) ? 6.0 : 1.0) * phi(element->GetNode(local[b])) / 24.0;
          const auto iVertex = geometry.nodes->GetVertex(iPoint, iMarker);
          for (auto iDim = 0u; iDim < nDim; ++iDim) flux(iVertex, iDim) += subNormal[tri][iDim] * value;
        }
      }
      continue;
    }
    if (nNodes != nDim) {
      SU2_MPI::Error("The boundary closure weights are defined on segments, triangles and quadrilaterals only.", CURRENT_FUNCTION);
    }
    const su2double* area = element->getNormal();

    for (auto iNode = 0u; iNode < nNodes; ++iNode) {
      const auto iPoint = element->GetNode(iNode);
      if (!geometry.nodes->GetDomain(iPoint)) continue;

      /*--- The weights are (5,1)/12 and (6,1,1)/24. ---*/
      su2double value = 0.0;
      for (auto jNode = 0u; jNode < nNodes; ++jNode) {
        const auto jValue = phi(element->GetNode(jNode));
        value += (nDim == 2) ? ((jNode == iNode) ? 5.0 : 1.0) * jValue / 12.0
                             : ((jNode == iNode) ? 6.0 : 1.0) * jValue / 24.0;
      }

      const auto iVertex = geometry.nodes->GetVertex(iPoint, iMarker);
      for (auto iDim = 0u; iDim < nDim; ++iDim) flux(iVertex, iDim) += area[iDim] * value;
    }
  }
}

}  // namespace BoundaryClosure
