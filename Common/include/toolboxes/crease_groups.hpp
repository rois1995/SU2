/*!
 * \file crease_groups.hpp
 * \brief Crease groups of the incident faces of a wall point (ROE_TECNO_PLAN.md 6.11 B, 6.14): used by
 *        BC_ENTROPY_P1_STRONG_SLIP and EULER_WALL_CREASE_NORMALS through CGeometry::ComputeCreaseNormals.
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
#include <cmath>
#include <cstddef>
#include "../basic_types/datatype_structure.hpp"

namespace CreaseGroups {

/*--- Crease groups of a wall point (ROE_TECNO_PLAN.md 6.11, item 2). The vertex normal of a marker point is the sum of
 * the corner area vectors N_f of its incident faces (SU2's median-dual pieces of each face at the point). Where incident
 * faces fold by 90 deg or more (N_f . N_g <= 0, e.g. the upper and lower faces at a sharp trailing edge) that sum can be a
 * cancellation residue: on the ONERA M6 trailing edge it is 76 deg (median) away from the face normals and the slip
 * plane contains face normals (penetration |u.n_f|/|u| up to 0.99 in the S4c state). Discrete grouping rule: two groups
 * are merged across an edge (adjacent faces: sharing another node; 2D: the two segments of a point) only if every pair
 * of faces of the merged group has N_f . N_g > 0; then the group normal n = sum N_g satisfies n . N_f >= |N_f|^2 > 0 for
 * every face of the group (inside its dual cone, for any area weights; for N_f . N_g < 0 there are weights for which
 * this fails). Every group contributes its summed normal (the groups' sum is the vertex normal); one group: the vertex
 * normal is kept. Deterministic for a given face order (the caller orders the faces by their global nodes). ---*/
constexpr unsigned short MAX_FACES = 32;

/*!
 * \brief Groups of the nFaces incident faces of a point (corner area vectors N, consistently oriented; for each face up
 *        to 3 other nodes, nOther per face). Adjacent pairs (f, g), f < g, in lexicographic order merge their groups if
 *        all cross products of the two groups are positive.
 * \return number of groups; groupN[q] the summed area vector of group q (groups in order of their first face).
 */
inline unsigned short FaceGroups(unsigned short nDim, unsigned short nFaces, const su2double (*N)[3],
                                     const unsigned long (*other)[3], const unsigned short* nOther,
                                     su2double (*groupN)[3], unsigned short* faceGroup = nullptr) {
  unsigned short group[MAX_FACES];
  for (unsigned short f = 0; f < nFaces; ++f) group[f] = f;
  auto dot = [&](unsigned short f, unsigned short g) {
    su2double d = 0.0;
    for (unsigned short k = 0; k < nDim; ++k) d += N[f][k] * N[g][k];
    return d;
  };
  for (unsigned short f = 0; f < nFaces; ++f) {
    for (unsigned short g = f + 1; g < nFaces; ++g) {
      if (group[f] == group[g]) continue;
      bool adjacent = (nDim == 2);
      for (unsigned short a = 0; a < nOther[f] && !adjacent; ++a)
        for (unsigned short b = 0; b < nOther[g]; ++b) adjacent = adjacent || (other[f][a] == other[g][b]);
      if (!adjacent) continue;
      bool positive = true;
      for (unsigned short p = 0; p < nFaces && positive; ++p) {
        if (group[p] != group[f]) continue;
        for (unsigned short q = 0; q < nFaces && positive; ++q)
          if (group[q] == group[g]) positive = (dot(p, q) > 0.0);
      }
      if (!positive) continue;
      const auto keep = std::min(group[f], group[g]), drop = std::max(group[f], group[g]);
      for (unsigned short p = 0; p < nFaces; ++p)
        if (group[p] == drop) group[p] = keep;
    }
  }
  unsigned short nGroups = 0, slotOf[MAX_FACES];
  for (unsigned short f = 0; f < nFaces; ++f) {
    if (group[f] == f) {
      slotOf[f] = nGroups;
      for (unsigned short d = 0; d < 3; ++d) groupN[nGroups][d] = 0.0;
      ++nGroups;
    }
    for (unsigned short d = 0; d < nDim; ++d) groupN[slotOf[group[f]]][d] += N[f][d];
    if (faceGroup) faceGroup[f] = slotOf[group[f]];
  }
  return nGroups;
}

/*--- Squared orthogonalized norm below which a further normal is taken as parallel to the kept ones: sin^2 of SU2's
 * parallel test 1 - |cos| < 0.001 of CGeometry::ComputeModifiedSymmetryNormals, 2 t - t^2. ---*/
constexpr su2double PARALLEL_SIN2 = 2.0 * 0.001 - 0.001 * 0.001;

/*!
 * \brief Orthonormal basis Q (rows) of the span of nNormals normals, Gram-Schmidt in the given order, a normal dropped
 *        when its orthogonalized squared norm (relative to its own) is below PARALLEL_SIN2.
 * \return rank (number of rows of Q, at most nDim).
 */
inline unsigned short Basis(unsigned short nDim, unsigned short nNormals, const su2double (*normals)[3],
                            su2double (*Q)[3]) {
  unsigned short rank = 0;
  for (unsigned short a = 0; a < nNormals && rank < nDim; ++a) {
    su2double v[3] = {0.0, 0.0, 0.0}, norm2 = 0.0;
    for (unsigned short d = 0; d < nDim; ++d) {
      v[d] = normals[a][d];
      norm2 += v[d] * v[d];
    }
    if (!(norm2 > 0.0)) continue;
    for (unsigned short r = 0; r < rank; ++r) {
      su2double proj = 0.0;
      for (unsigned short d = 0; d < nDim; ++d) proj += Q[r][d] * v[d];
      for (unsigned short d = 0; d < nDim; ++d) v[d] -= proj * Q[r][d];
    }
    su2double rest2 = 0.0;
    for (unsigned short d = 0; d < nDim; ++d) rest2 += v[d] * v[d];
    if (rest2 < PARALLEL_SIN2 * norm2) continue;
    const su2double inv = 1.0 / sqrt(rest2);
    for (unsigned short d = 0; d < nDim; ++d) Q[rank][d] = v[d] * inv;
    ++rank;
  }
  return rank;
}

/*!
 * \brief Symmetry correction of a point gradient for the subspace S spanned by the orthonormal rows of Q (rank r):
 *        invariance under the point reflection R = I - 2 P_S (P_S = Q^T Q): scalars g <- (I - P_S) g, velocity gradient
 *        G <- P_S G P_S + (I - P_S) G (I - P_S). For r = 1 this is Blazek's eq. 8.40 (detail::correctGradient); for r >= 2
 *        it does not depend on the basis of S. gradients[iVar][iDim], velocity rows idxVel..idxVel+nDim-1 (-1: none).
 */
template <size_t nDim, class Matrix>
inline void SubspaceGradientCorrection(size_t varBegin, size_t varEnd, int idxVel, unsigned short rank,
                                       const su2double (*Q)[3], Matrix&& gradients) {
  su2double P[nDim][nDim] = {};
  for (size_t i = 0; i < nDim; ++i)
    for (size_t j = 0; j < nDim; ++j)
      for (unsigned short r = 0; r < rank; ++r) P[i][j] += Q[r][i] * Q[r][j];
  if (idxVel >= static_cast<int>(varBegin) && idxVel + nDim <= varEnd) {
    su2double G[nDim][nDim], PG[nDim][nDim] = {}, out[nDim][nDim] = {};
    for (size_t i = 0; i < nDim; ++i)
      for (size_t j = 0; j < nDim; ++j) G[i][j] = gradients[idxVel + i][j];
    for (size_t i = 0; i < nDim; ++i)
      for (size_t j = 0; j < nDim; ++j)
        for (size_t k = 0; k < nDim; ++k) PG[i][j] += P[i][k] * G[k][j];
    /*--- out = G - (I - P) G P - P G (I - P) = G - G P - P G + 2 P G P. ---*/
    for (size_t i = 0; i < nDim; ++i)
      for (size_t j = 0; j < nDim; ++j) {
        su2double GP = 0.0, PGP = 0.0;
        for (size_t k = 0; k < nDim; ++k) {
          GP += G[i][k] * P[k][j];
          PGP += PG[i][k] * P[k][j];
        }
        out[i][j] = G[i][j] - GP - PG[i][j] + 2.0 * PGP;
      }
    for (size_t i = 0; i < nDim; ++i)
      for (size_t j = 0; j < nDim; ++j) gradients[idxVel + i][j] = out[i][j];
  }
  for (auto iVar = varBegin; iVar < varEnd; iVar++) {
    if (idxVel != -1 && static_cast<int>(iVar) >= idxVel && iVar < idxVel + nDim) continue;
    su2double Pg[nDim] = {};
    for (size_t i = 0; i < nDim; ++i)
      for (size_t j = 0; j < nDim; ++j) Pg[i] += P[i][j] * gradients[iVar][j];
    for (size_t i = 0; i < nDim; ++i) gradients[iVar][i] -= Pg[i];
  }
}

}  // namespace CreaseGroups
