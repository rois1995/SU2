/*!
 * \file jst_weak_wall.hpp
 * \brief JST_WEAK_WALL_DISSIPATION: flags of the nodes of the weak no-slip walls, read by the fourth-order term of the
 *        JST schemes (jstWallFactor in numerics_simd/flow/convection/common.hpp).
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

#include "../../../../Common/include/CConfig.hpp"
#include "../../../../Common/include/geometry/CGeometry.hpp"
#include "../../../../Common/include/linear_algebra/CSysVector.hpp"
#include "../../../../Common/include/linear_algebra/CSysMatrix.hpp"

/*--- P1_WALL_DISSIPATION.md. The undivided Laplacian of a wall node has only boundary neighbours, so on an edge from a
 * weak no-slip wall node s to a node k off that wall the fourth-order flux eps4 lam (L_k - L_s) is a first or second
 * difference that competes with the no-slip penalty. JST_WEAK_WALL_DISSIPATION sets eps4 = 0 on exactly those edges.
 * The edge loop of a rank also computes the edges between an owned node and a halo node, so both flags must be known
 * for halo nodes too. ---*/

namespace JSTWeakWall {

/*!
 * \brief Flag 1 on the nodes of the markers selected by isWall(iMarker), 0 elsewhere, on every node of the rank.
 * \note The rank that owns a node holds all the boundary faces of that node, so its flag is exact. A halo node can lie
 *       on the wall with none of its wall faces on this rank (they have no owned node), so the owned values are sent
 *       to the halos. Collective (point-to-point comms), call outside parallel regions.
 * \param[in] geometry - Geometry with its point-to-point comms set up.
 * \param[in] config - Definition of the problem.
 * \param[in] isWall - Predicate on the local marker index.
 * \param[out] flags - Resized to the number of points of the rank.
 */
template <class IsWall>
void WallNodeFlags(CGeometry& geometry, const CConfig& config, const IsWall& isWall, su2activevector& flags) {
  const unsigned long nPoint = geometry.GetnPoint();
  CSysVector<su2mixedfloat> local(nPoint, geometry.GetnPointDomain(), 1, 0.0);

  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    if (!isWall(iMarker)) continue;
    for (unsigned long iVertex = 0; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
      local(geometry.vertex[iMarker][iVertex]->GetNode(), 0) = 1.0;
    }
  }

  /*--- Owned values overwrite the halo values. ---*/
  CSysMatrixComms::Initiate(local, &geometry, &config);
  CSysMatrixComms::Complete(local, &geometry, &config);

  flags.resize(nPoint) = su2double(0.0);
  for (unsigned long iPoint = 0; iPoint < nPoint; ++iPoint) {
    if (local(iPoint, 0) > 0.5) flags(iPoint) = su2double(1.0);
  }
}

}  // namespace JSTWeakWall
