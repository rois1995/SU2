/*!
 * \file SimplexMeshTestCase.hpp
 * \brief Structured simplex meshes (rectangle, cube) for the mesh adaptation unit tests.
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

#include <algorithm>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "../../../Common/include/adaptation/CSimplexMesh.hpp"
#include "../../../Common/include/option_structure.hpp"

namespace simplex_test {

using MarkerFunction = std::function<std::string(const passivedouble*)>;

/*!
 * \brief Structured simplex mesh: the rectangle [0,2]x[0,1] (2D, 2 triangles per cell) or the unit cube (3D, Kuhn
 *        subdivision, 6 tetrahedra per cell). The boundary faces are given to markers by their centroid with
 *        markerOf; markers are sorted by name, their references are 0. Half of the 2D elements and some of the 3D
 *        ones are negatively oriented, the nodes of the boundary faces are sorted (no orientation). n must be even.
 */
inline CSimplexMesh MakeSimplexMesh(unsigned short nDim, unsigned long n, const MarkerFunction& markerOf) {
  const unsigned long nx = (nDim == 2) ? 2 * n : n, ny = n, nz = (nDim == 2) ? 0 : n;
  const passivedouble h = 1.0 / n;
  auto index = [&](unsigned long i, unsigned long j, unsigned long k) { return i + (nx + 1) * (j + (ny + 1) * k); };

  CSimplexMesh mesh;
  mesh.nDim = nDim;
  for (unsigned long k = 0; k <= nz; ++k)
    for (unsigned long j = 0; j <= ny; ++j)
      for (unsigned long i = 0; i <= nx; ++i) {
        mesh.coord.push_back(i * h);
        mesh.coord.push_back(j * h);
        if (nDim == 3) mesh.coord.push_back(k * h);
      }

  std::vector<std::vector<unsigned long>> elems;
  if (nDim == 2) {
    for (unsigned long j = 0; j < ny; ++j)
      for (unsigned long i = 0; i < nx; ++i) {
        elems.push_back({index(i, j, 0), index(i + 1, j, 0), index(i + 1, j + 1, 0)});
        elems.push_back({index(i, j, 0), index(i, j + 1, 0), index(i + 1, j + 1, 0)});  // negative orientation
      }
  } else {
    const int perms[6][3] = {{0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}};
    for (unsigned long k = 0; k < nz; ++k)
      for (unsigned long j = 0; j < ny; ++j)
        for (unsigned long i = 0; i < nx; ++i)
          for (const auto& perm : perms) {
            unsigned long ijk[3] = {i, j, k};
            std::vector<unsigned long> tet = {index(i, j, k)};
            for (int d = 0; d < 3; ++d) {
              ijk[perm[d]]++;
              tet.push_back(index(ijk[0], ijk[1], ijk[2]));
            }
            elems.push_back(tet);
          }
  }
  for (const auto& elem : elems) mesh.elem.insert(mesh.elem.end(), elem.begin(), elem.end());
  mesh.elemRef.assign(elems.size(), 0);

  /*--- Boundary faces: faces of a single element. ---*/
  std::map<std::vector<unsigned long>, int> faceCount;
  for (const auto& elem : elems)
    for (unsigned short iFace = 0; iFace <= nDim; ++iFace) {
      std::vector<unsigned long> face;
      for (unsigned short iNode = 0; iNode <= nDim; ++iNode)
        if (iNode != iFace) face.push_back(elem[iNode]);
      std::sort(face.begin(), face.end());
      faceCount[face]++;
    }
  std::map<std::string, std::vector<unsigned long>> markers;
  for (const auto& entry : faceCount) {
    if (entry.second != 1) continue;
    passivedouble centroid[3] = {0.0, 0.0, 0.0};
    for (const auto iPoint : entry.first)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) centroid[iDim] += mesh.coord[iPoint * nDim + iDim] / nDim;
    auto& marker = markers[markerOf(centroid)];
    marker.insert(marker.end(), entry.first.begin(), entry.first.end());
  }
  for (auto& entry : markers) {
    CSimplexMesh::Marker marker;
    marker.name = entry.first;
    marker.elem = std::move(entry.second);
    mesh.markers.push_back(std::move(marker));
  }
  return mesh;
}

/*!
 * \brief Write a simplex mesh in SU2 format (full precision).
 */
inline void WriteSU2Mesh(const CSimplexMesh& mesh, const std::string& filename) {
  const auto nDim = mesh.nDim;
  std::ofstream file(filename);
  file.precision(17);
  file << "NDIME= " << nDim << "\n";
  file << "NELEM= " << mesh.GetnElem() << "\n";
  for (auto iElem = 0ul; iElem < mesh.GetnElem(); ++iElem) {
    file << (nDim == 2 ? TRIANGLE : TETRAHEDRON);
    for (unsigned short iNode = 0; iNode <= nDim; ++iNode) file << " " << mesh.elem[iElem * (nDim + 1) + iNode];
    file << " " << iElem << "\n";
  }
  file << "NPOIN= " << mesh.GetnPoint() << "\n";
  for (auto iPoint = 0ul; iPoint < mesh.GetnPoint(); ++iPoint) {
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) file << mesh.coord[iPoint * nDim + iDim] << " ";
    file << iPoint << "\n";
  }
  file << "NMARK= " << mesh.markers.size() << "\n";
  for (const auto& marker : mesh.markers) {
    file << "MARKER_TAG= " << marker.name << "\n";
    file << "MARKER_ELEMS= " << marker.GetnElem(nDim) << "\n";
    for (auto iElem = 0ul; iElem < marker.GetnElem(nDim); ++iElem) {
      file << (nDim == 2 ? LINE : TRIANGLE);
      for (unsigned short iNode = 0; iNode < nDim; ++iNode) file << " " << marker.elem[iElem * nDim + iNode];
      file << "\n";
    }
  }
}

/*--- Rectangle [0,2]x[0,1]: the lower side is split in two coplanar markers at x = 1. ---*/
inline std::string Marker2D(const passivedouble* x) {
  if (x[1] < 1e-12) return x[0] < 1.0 ? "lower_a" : "lower_b";
  if (x[1] > 1.0 - 1e-12) return "upper";
  return x[0] < 1e-12 ? "left" : "right";
}

/*--- Unit cube: the face z = 0 is split in two coplanar markers at x = 0.5. ---*/
inline std::string Marker3D(const passivedouble* x) {
  if (x[2] < 1e-12) return x[0] < 0.5 ? "z_minus_a" : "z_minus_b";
  if (x[2] > 1.0 - 1e-12) return "z_plus";
  if (x[0] < 1e-12) return "x_minus";
  if (x[0] > 1.0 - 1e-12) return "x_plus";
  return x[1] < 1e-12 ? "y_minus" : "y_plus";
}

}  // namespace simplex_test
