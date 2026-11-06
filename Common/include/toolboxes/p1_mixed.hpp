/*!
 * \file p1_mixed.hpp
 * \brief Split-averaged P1 pieces of non-simplex cells (EDGE_FORM_P1_MIXED, P1_MIXED.md): the neighbour-fan
 *        subset of the pulling splits, merged into one weighted list of sub-simplices per cell type, and the
 *        sub-triangles of a quadrilateral boundary face with both diagonals.
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

#include <algorithm>
#include <array>
#include <vector>
#include "../basic_types/datatype_structure.hpp"

/*--- EDGE_FORM_P1_MIXED (P1_MIXED.md, sections 1-6). Every non-simplex cell C carries the mean of a family of
 * pulling splits s (weights 1/|family|), each a conforming triangulation of C into sub-simplices t without new
 * points. Merging equal node tuples gives one list of (t, omega_t), omega_t = (number of splits holding t)/|family|.
 * All P1 pieces of the cell are the omega-weighted sums over that list of the simplex pieces:
 *   n_ik += omega_t m_t (g^t_k - g^t_i),  E = 0  (dual M0: W^t_a = m_t = |t|/(d+1)),
 *   viscous V-A on each t (residual and Jacobian times omega_t),
 *   weak walls (closure C and SIPG) on each t with a face on the wall, with that t and its height d|t|/|tau|.
 * The family is the neighbour-fan subset (Lemma X9): the pulling splits in which every face without the apex v that
 * holds exactly one edge-neighbour u of v is fanned through u. For hexahedra these are the 4 Kuhn triangulations (8
 * splits, 24 sub-tetrahedra of weight 1/4, orthoschemes on bricks); prisms, pyramids and quadrilaterals have no such
 * face, so all their pulling splits are kept (12, 6 and 4 splits). The subset is face balanced (Lemma X2): each quad
 * face gets each diagonal with total weight 1/2, so interior faces cancel in assembly (Corollary X3). ---*/

namespace P1Mixed {

/*!
 * \brief One sub-simplex of the split average of a cell type.
 */
struct Simplex {
  std::array<unsigned short, 4> nodes = {{0, 0, 0, 0}}; /*!< \brief Cell-local nodes, nDim+1 are used. */
  passivedouble weight = 0.0;                           /*!< \brief omega_t, the share of the splits holding it. */
};

/*!
 * \brief Neighbour-fan test of one pulling split (Lemma X9).
 * \param[in] element - Cell (GetnFaces, GetnNodesFace, GetFaces, GetnNeighbor_Nodes, GetNeighbor_Nodes).
 * \param[in] nDim - 2 or 3; in 2D the faces are segments and every split passes.
 * \param[in] simplices - Sub-simplices of the split, apex first (the order of CPhysicalGeometry::GetSplitPatterns).
 * \return True if every quad face without the apex v that holds exactly one edge-neighbour u of v is fanned through u,
 *         i.e. a sub-simplex (v, a, b, c) with a, b, c on that face holds u and the face node opposite u.
 */
template <class Element, class Simplices>
bool NeighbourFan(const Element& element, unsigned short nDim, const Simplices& simplices) {
  if ((nDim != 3) || simplices.empty()) return true;
  const unsigned short v = simplices[0][0];

  auto isNeighbor = [&](unsigned short node) {
    for (unsigned short j = 0; j < element.GetnNeighbor_Nodes(v); ++j)
      if (element.GetNeighbor_Nodes(v, j) == node) return true;
    return false;
  };

  for (unsigned short iFace = 0; iFace < element.GetnFaces(); ++iFace) {
    if (element.GetnNodesFace(iFace) != 4) continue;
    bool hasV = false;
    unsigned short nNeighbor = 0, position = 0;
    for (unsigned short k = 0; k < 4; ++k) {
      const auto node = element.GetFaces(iFace, k);
      hasV = hasV || (node == v);
      if (isNeighbor(node)) {
        ++nNeighbor;
        position = k;
      }
    }
    if (hasV || (nNeighbor != 1)) continue;

    const unsigned short u = element.GetFaces(iFace, position);
    const unsigned short opposite = element.GetFaces(iFace, (position + 2) % 4);
    auto onFace = [&](unsigned short node) {
      for (unsigned short k = 0; k < 4; ++k)
        if (element.GetFaces(iFace, k) == node) return true;
      return false;
    };

    bool fanned = false;
    for (const auto& t : simplices) {
      if ((t[0] != v) || !onFace(t[1]) || !onFace(t[2]) || !onFace(t[3])) continue;
      bool hasU = false, hasOpposite = false;
      for (unsigned short a = 1; a < 4; ++a) {
        hasU = hasU || (t[a] == u);
        hasOpposite = hasOpposite || (t[a] == opposite);
      }
      fanned = fanned || (hasU && hasOpposite);
    }
    if (!fanned) return false;
  }
  return true;
}

/*!
 * \brief Weighted sub-simplex list of the neighbour-fan subset of the pulling splits of a cell type.
 * \param[in] element - A cell of that type.
 * \param[in] nDim - 2 or 3.
 * \param[in] patterns - All pulling splits, each with a member "simplices" (CPhysicalGeometry::GetSplitPatterns).
 * \return Distinct sub-simplices (first node order met) with omega_t; sum_t omega_t |t| = |C| on convex cells.
 */
template <class Element, class Patterns>
std::vector<Simplex> NeighbourFanAverage(const Element& element, unsigned short nDim, const Patterns& patterns) {
  const unsigned short nNodes = nDim + 1;
  size_t nKept = 0;
  for (const auto& pattern : patterns) nKept += NeighbourFan(element, nDim, pattern.simplices);

  std::vector<Simplex> list;
  std::vector<std::array<unsigned short, 4>> keys;
  if (nKept == 0) return list;
  const passivedouble weight = 1.0 / nKept;

  for (const auto& pattern : patterns) {
    if (!NeighbourFan(element, nDim, pattern.simplices)) continue;
    for (const auto& t : pattern.simplices) {
      std::array<unsigned short, 4> key = {{0, 0, 0, 0}};
      for (unsigned short a = 0; a < nNodes; ++a) key[a] = t[a];
      std::sort(key.begin(), key.begin() + nNodes);
      const auto found = std::find(keys.begin(), keys.end(), key);
      if (found != keys.end()) {
        list[found - keys.begin()].weight += weight;
        continue;
      }
      Simplex entry;
      for (unsigned short a = 0; a < nNodes; ++a) entry.nodes[a] = t[a];
      entry.weight = weight;
      list.push_back(entry);
      keys.push_back(key);
    }
  }
  return list;
}

/*!
 * \brief Sub-triangles of a quadrilateral face (local nodes 0-3 in the cyclic order of the face): the two of the
 *        diagonal 0-2, then the two of the diagonal 1-3. Each diagonal has weight 1/2 (Theorem X10).
 */
constexpr unsigned short QuadTriangles[4][3] = {{0, 1, 2}, {0, 2, 3}, {1, 2, 3}, {1, 3, 0}};

/*!
 * \brief Area vectors of the four sub-triangles of a quadrilateral face, times the weight 1/2 of their diagonal,
 *        oriented like the face. Both pairs sum to the area vector (x2 - x0) x (x3 - x1) / 2 of the face, warped or not.
 * \param[in] x - Coordinates of the face nodes, x[k][iDim], in the cyclic order of the face.
 * \param[in] faceNormal - Area vector of the face (only its orientation is used).
 * \param[out] normals - normals[tri][iDim] = A_tau / 2.
 */
template <class Coords>
void QuadSubNormals(const Coords& x, const su2double* faceNormal, su2double normals[][3]) {
  su2double total = 0.0;
  for (unsigned short tri = 0; tri < 4; ++tri) {
    const auto* t = QuadTriangles[tri];
    su2double a[3], b[3];
    for (unsigned short iDim = 0; iDim < 3; ++iDim) {
      a[iDim] = x[t[1]][iDim] - x[t[0]][iDim];
      b[iDim] = x[t[2]][iDim] - x[t[0]][iDim];
    }
    normals[tri][0] = 0.25 * (a[1] * b[2] - a[2] * b[1]);
    normals[tri][1] = 0.25 * (a[2] * b[0] - a[0] * b[2]);
    normals[tri][2] = 0.25 * (a[0] * b[1] - a[1] * b[0]);
    for (unsigned short iDim = 0; iDim < 3; ++iDim) total += normals[tri][iDim] * faceNormal[iDim];
  }
  if (total < 0.0) {
    for (unsigned short tri = 0; tri < 4; ++tri)
      for (unsigned short iDim = 0; iDim < 3; ++iDim) normals[tri][iDim] = -normals[tri][iDim];
  }
}

}  // namespace P1Mixed
