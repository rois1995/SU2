/*!
 * \file p1_strong_slip.hpp
 * \brief BC_ENTROPY_P1_STRONG_SLIP: strong u.n = 0 at the Euler walls and symmetry planes of closure C
 *        (ROE_TECNO_PLAN.md 6.8). Pure helpers of the momentum projection P = I - Q Q^T, Q an orthonormal basis of the
 *        wall normals of a point.
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
#include "../../../../Common/include/basic_types/datatype_structure.hpp"

/*--- Why strong: at a weak slip wall of closure C (pressure flux, entropy conservative) the normal velocity of a wall
 * node is set only by its normal-momentum balance, in which the dissipation fluxes of the wall-crossing edges enter one
 * sided; their mirror partners would cancel that component. The adjoint TeCNO dissipation (C^T D C) does not control
 * this wall mode, and the shock foot of a reflection then feeds the wall rows an entropy layer (model: slip-wall
 * reflection, AR 513). The restriction of the mirror-symmetric interior scheme to the half domain has u.n = 0 at the
 * wall node and no normal-momentum equation there; this is SU2's BC_Sym_Plane projection (Blazek ch. 8).
 * Entropy: with Q^T m = 0 in the evaluated state, w_m = 2 beta u has no component in span Q, so the removed residual
 * part (I - P) R carries no entropy and sum_i w_i . R_i is unchanged (semi-discrete statement). ---*/

namespace P1StrongSlip {

/*--- Squared orthogonalized norm below which a further normal is taken as parallel to the kept ones: sin^2 of SU2's
 * parallel test 1 - |cos| < 0.001 of CGeometry::ComputeModifiedSymmetryNormals, 2 t - t^2. ---*/
constexpr su2double PARALLEL_SIN2 = 2.0 * 0.001 - 0.001 * 0.001;

/*!
 * \brief Orthonormal basis Q (rows) of the span of nNormals unit (or any nonzero) normals, Gram-Schmidt in the given
 *        order, a normal dropped when its orthogonalized squared norm (relative to its own) is below PARALLEL_SIN2.
 * \return rank (number of rows of Q, at most nDim).
 */
inline unsigned short SlipBasis(unsigned short nDim, unsigned short nNormals, const su2double (*normals)[3],
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
constexpr unsigned short SLIP_MAX_FACES = 32;

/*!
 * \brief Groups of the nFaces incident faces of a point (corner area vectors N, consistently oriented; for each face up
 *        to 3 other nodes, nOther per face). Adjacent pairs (f, g), f < g, in lexicographic order merge their groups if
 *        all cross products of the two groups are positive.
 * \return number of groups; groupN[q] the summed area vector of group q (groups in order of their first face).
 */
inline unsigned short SlipFaceGroups(unsigned short nDim, unsigned short nFaces, const su2double (*N)[3],
                                     const unsigned long (*other)[3], const unsigned short* nOther,
                                     su2double (*groupN)[3]) {
  unsigned short group[SLIP_MAX_FACES];
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
  unsigned short nGroups = 0, slotOf[SLIP_MAX_FACES];
  for (unsigned short f = 0; f < nFaces; ++f) {
    if (group[f] == f) {
      slotOf[f] = nGroups;
      for (unsigned short d = 0; d < 3; ++d) groupN[nGroups][d] = 0.0;
      ++nGroups;
    }
    for (unsigned short d = 0; d < nDim; ++d) groupN[slotOf[group[f]]][d] += N[f][d];
  }
  return nGroups;
}

/*!
 * \brief vec[iVel + d] <- (P vec)[iVel + d], P = I - Q^T Q on the momentum components (Q rows orthonormal).
 */
template <class VecType>
inline void SlipProject(unsigned short nDim, unsigned short rank, const su2double (*Q)[3], unsigned short iVel,
                        VecType& vec) {
  for (unsigned short r = 0; r < rank; ++r) {
    su2double proj = 0.0;
    for (unsigned short d = 0; d < nDim; ++d) proj += Q[r][d] * vec[iVel + d];
    for (unsigned short d = 0; d < nDim; ++d) vec[iVel + d] -= proj * Q[r][d];
  }
}

/*!
 * \brief Momentum rows of a row-major nVar x nVar block left-multiplied by P (each column projected).
 */
template <class BlockType>
inline void SlipProjectRows(unsigned short nDim, unsigned short nVar, unsigned short rank, const su2double (*Q)[3],
                            unsigned short iVel, BlockType& block) {
  for (unsigned short j = 0; j < nVar; ++j) {
    for (unsigned short r = 0; r < rank; ++r) {
      su2double proj = 0.0;
      for (unsigned short d = 0; d < nDim; ++d) proj += Q[r][d] * block[(iVel + d) * nVar + j];
      for (unsigned short d = 0; d < nDim; ++d) block[(iVel + d) * nVar + j] -= proj * Q[r][d];
    }
  }
}

/*!
 * \brief |Q m| / max(|m|, mRef) of the momentum m = vec[iVel..] (penetration diagnostic), 0 if both are 0. mRef (the free
 *        stream momentum) keeps the value meaningful where m is roundoff, e.g. at points with rank = nDim.
 */
template <class VecType>
inline su2double SlipPenetration(unsigned short nDim, unsigned short rank, const su2double (*Q)[3],
                                 unsigned short iVel, const VecType& vec, su2double mRef) {
  su2double m2 = 0.0, q2 = 0.0;
  for (unsigned short d = 0; d < nDim; ++d) m2 += vec[iVel + d] * vec[iVel + d];
  for (unsigned short r = 0; r < rank; ++r) {
    su2double proj = 0.0;
    for (unsigned short d = 0; d < nDim; ++d) proj += Q[r][d] * vec[iVel + d];
    q2 += proj * proj;
  }
  const su2double den2 = fmax(m2, mRef * mRef);
  if (!(den2 > 0.0)) return 0.0;
  return sqrt(q2 / den2);
}

}  // namespace P1StrongSlip
