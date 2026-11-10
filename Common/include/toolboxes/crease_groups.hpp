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

/*--- Crease groups of a wall point (ROE_TECNO_PLAN.md 6.11, item 2; 6.14 rev 5). The vertex normal of a marker point is the
 * sum of the corner area vectors N_f of its incident faces (SU2's median-dual pieces of each face at the point). Where
 * incident faces fold by 90 deg or more (e.g. the upper and lower faces at a sharp trailing edge) that sum can be a
 * cancellation residue: on the ONERA M6 trailing edge it is 76 deg (median) away from the face normals.
 * Compatibility: N_f . N_g > COMPATIBLE_COS |N_f| |N_g|. A group G whose faces are pairwise compatible has a summed
 * normal n = sum N_g with n . N_f >= |N_f|^2 > 0 for every face of G (inside the dual cone of its faces, for any area
 * weights) and |n|^2 >= sum |N_g|^2 (no cancellation), for nonzero corner vectors; for a pair with N_f . N_g < 0 there
 * are area weights for which both fail (90 deg is the mathematical bound; COMPATIBLE_COS is a round-off guard above it). Rule (order independent): links are the adjacent compatible pairs (adjacent: sharing another node; 2D: the two
 * segments of a point), weighted by their cosine. The groups are the maximal pairwise-compatible clusters of the
 * single-linkage hierarchy of the links: connected components of the links, and every component that is not pairwise
 * compatible is split by removing its weakest links (smallest cosine, ties together), recursively. These clusters are
 * nested, so the maximal ones partition the faces; the result depends only on the set of faces, not on their order.
 * Where every component of the links is pairwise compatible (all points of the 2D meshes and all but two points of the
 * M6 wing) the groups are those components, the same as a greedy merge in face order with the same compatibility test
 * (the earlier rule used N_f . N_g > 0: pairs with 0 < cos <= COMPATIBLE_COS are now creases). A face compatible with
 * two mutually incompatible neighbours joins the one with the larger cosine (equal: it forms its own group). Implied
 * feature angle: folds of less than 90 deg inside a compatible cluster are averaged (one group), as SU2's vertex
 * normal does everywhere. Every group contributes its summed normal (the groups' sum is the vertex normal). ---*/
constexpr unsigned short MAX_FACES = 32;

/*--- Compatibility threshold of the cosine: SU2's tolerance for normal pairs (PARALLEL_TOLERANCE of
 * CGeometry::ComputeModifiedSymmetryNormals), so that a nominal right angle is a crease whatever the round-off. ---*/
constexpr passivedouble COMPATIBLE_COS = 0.001;

/*!
 * \brief Groups of the nFaces incident faces of a point (corner area vectors N, consistently oriented; for each face up
 *        to 3 other nodes, nOther per face): maximal pairwise-compatible single-linkage clusters (see above).
 * \return number of groups; groupN[q] the summed area vector of group q (groups in order of their first face).
 */
inline unsigned short FaceGroups(unsigned short nDim, unsigned short nFaces, const su2double (*N)[3],
                                 const unsigned long (*other)[3], const unsigned short* nOther,
                                 su2double (*groupN)[3], unsigned short* faceGroup = nullptr) {
  /*--- Links: adjacent compatible pairs, with their cosine. ---*/
  constexpr unsigned short MAX_LINKS = MAX_FACES * (MAX_FACES - 1) / 2;
  su2double len[MAX_FACES], dotFG[MAX_FACES][MAX_FACES];
  for (unsigned short f = 0; f < nFaces; ++f) {
    for (unsigned short g = 0; g < nFaces; ++g) {
      su2double d = 0.0;
      for (unsigned short k = 0; k < nDim; ++k) d += N[f][k] * N[g][k];
      dotFG[f][g] = d;
    }
    len[f] = sqrt(dotFG[f][f]);
  }
  auto compatible = [&](unsigned short f, unsigned short g) { return dotFG[f][g] > COMPATIBLE_COS * len[f] * len[g]; };
  unsigned short linkA[MAX_LINKS], linkB[MAX_LINKS], nLinks = 0;
  su2double linkCos[MAX_LINKS];
  bool active[MAX_LINKS];
  for (unsigned short f = 0; f < nFaces; ++f) {
    for (unsigned short g = f + 1; g < nFaces; ++g) {
      bool adjacent = (nDim == 2);
      for (unsigned short a = 0; a < nOther[f] && !adjacent; ++a)
        for (unsigned short b = 0; b < nOther[g]; ++b) adjacent = adjacent || (other[f][a] == other[g][b]);
      if (!adjacent || !compatible(f, g)) continue;
      linkA[nLinks] = f;
      linkB[nLinks] = g;
      linkCos[nLinks] = dotFG[f][g] / (len[f] * len[g]);
      active[nLinks] = true;
      ++nLinks;
    }
  }
  /*--- Components of the active links (labelled by their smallest face); every component that is not pairwise
   * compatible loses all of its weakest links (smallest cosine, ties together); repeat until all are compatible. ---*/
  unsigned short group[MAX_FACES];
  while (true) {
    for (unsigned short f = 0; f < nFaces; ++f) group[f] = f;
    for (bool changed = true; changed;) {
      changed = false;
      for (unsigned short l = 0; l < nLinks; ++l) {
        if (!active[l] || (group[linkA[l]] == group[linkB[l]])) continue;
        const auto keep = std::min(group[linkA[l]], group[linkB[l]]), drop = std::max(group[linkA[l]], group[linkB[l]]);
        for (unsigned short p = 0; p < nFaces; ++p)
          if (group[p] == drop) group[p] = keep;
        changed = true;
      }
    }
    bool split = false;
    for (unsigned short r = 0; r < nFaces; ++r) {
      if (group[r] != r) continue;
      bool clique = true;
      for (unsigned short p = r; p < nFaces && clique; ++p) {
        if (group[p] != r) continue;
        for (unsigned short q = p + 1; q < nFaces && clique; ++q)
          if (group[q] == r) clique = compatible(p, q);
      }
      if (clique) continue;
      su2double weakest = 2.0;
      for (unsigned short l = 0; l < nLinks; ++l)
        if (active[l] && (group[linkA[l]] == r)) weakest = std::min(weakest, linkCos[l]);
      for (unsigned short l = 0; l < nLinks; ++l)
        if (active[l] && (group[linkA[l]] == r) && (linkCos[l] <= weakest)) active[l] = false;
      split = true;
    }
    if (!split) break;
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
