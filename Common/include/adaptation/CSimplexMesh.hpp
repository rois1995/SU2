/*!
 * \file CSimplexMesh.hpp
 * \brief Simplex mesh stored as plain arrays, exchanged between SU2 and the remesher.
 * \version 8.5.0 "Harrier"
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

#include <string>
#include <vector>

#include "../basic_types/datatype_structure.hpp"

/*!
 * \brief Simplex mesh (triangles in 2D, tetrahedra in 3D) stored as plain arrays.
 * \note This is the exchange format between SU2 and MMG, and the input to rebuild a CPhysicalGeometry.
 *       Point indices are 0-based, volume elements are positively oriented. Boundary elements are grouped
 *       by physical marker name; the orientation of boundary elements is not prescribed.
 */
struct CSimplexMesh {
  /*!
   * \brief Boundary elements of one physical marker.
   */
  struct Marker {
    std::string name;                /*!< \brief Physical marker name (MARKER_* tag). */
    int ref = 0;                     /*!< \brief MMG reference of the marker (> 0). */
    std::vector<unsigned long> elem; /*!< \brief Boundary connectivity, nDim points per element (lines or triangles). */

    unsigned long GetnElem(unsigned short nDim) const { return elem.size() / nDim; }
  };

  unsigned short nDim = 0;            /*!< \brief Number of dimensions (2 or 3). */
  std::vector<passivedouble> coord;   /*!< \brief Point coordinates, nPoint x nDim. */
  std::vector<passivedouble> metric;  /*!< \brief Metric at the points, nPoint x nMetric, upper triangle (may be empty). */
  std::vector<unsigned long> elem;    /*!< \brief Volume connectivity, nElem x (nDim+1). */
  std::vector<int> elemRef;           /*!< \brief Reference of each volume element. */
  std::vector<Marker> markers;        /*!< \brief Boundary elements per physical marker. */

  /*! \brief Number of metric components, (xx,xy,yy) in 2D, (xx,xy,xz,yy,yz,zz) in 3D. */
  static unsigned short GetnMetric(unsigned short nDim) { return nDim * (nDim + 1) / 2; }

  unsigned long GetnPoint() const { return nDim ? coord.size() / nDim : 0; }
  unsigned long GetnElem() const { return nDim ? elem.size() / (nDim + 1) : 0; }

  /*!
   * \brief Find a marker by its physical name.
   * \return Pointer to the marker, nullptr if it does not exist.
   */
  const Marker* FindMarker(const std::string& name) const {
    for (const auto& marker : markers)
      if (marker.name == name) return &marker;
    return nullptr;
  }
};
