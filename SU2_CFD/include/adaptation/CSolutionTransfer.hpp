/*!
 * \file CSolutionTransfer.hpp
 * \brief Transfer of the solution from the previous mesh to a new one (mesh adaptation).
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

class CConfig;
class CGeometry;
class CSolver;

/*!
 * \brief Geometry and solvers of the mesh that is being replaced (the donor of a solution transfer).
 * \note The donor stays alive until the driver uses the new mesh. The config already describes the new mesh
 *       (number of multigrid levels, Marker_All_* arrays), so the donor carries its own level count and marker names.
 */
struct CMeshDonor {
  CGeometry** geometry = nullptr;       /*!< \brief Geometry of each multigrid level of the donor. */
  CSolver*** solver = nullptr;          /*!< \brief Solvers of each multigrid level of the donor ([iMesh][iSol]). */
  unsigned short nMGLevels = 0;         /*!< \brief Number of coarse levels of the donor. */
  std::vector<std::string> markerTags;  /*!< \brief Name of each marker of the donor geometry (by iMarker). */
};

/*!
 * \class CSolutionTransfer
 * \brief Interface to set the solution on a new mesh from the solution on the previous one.
 * \note Called by the driver after the solvers of the new mesh were created (free-stream state on all levels) and
 *       before the driver switches to them. On return the solution must be complete on all multigrid levels (as after
 *       loading a restart file). Errors stop the run (SU2_MPI::Error), the driver still uses the donor at that point.
 */
class CSolutionTransfer {
 public:
  virtual ~CSolutionTransfer() = default;

  /*!
   * \brief Set the solution of the new mesh.
   * \param[in] config - Definition of the problem, describes the new mesh.
   * \param[in] donor - Geometry and solvers of the previous mesh.
   * \param[in] geometry - Geometry of each multigrid level of the new mesh.
   * \param[in,out] solver - Solvers of each multigrid level of the new mesh.
   */
  virtual void Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver) = 0;
};

/*!
 * \class CFreeStreamTransfer
 * \brief No transfer: the new mesh keeps the free-stream state the solvers were created with.
 */
class CFreeStreamTransfer final : public CSolutionTransfer {
 public:
  void Transfer(CConfig*, const CMeshDonor&, CGeometry**, CSolver***) override {}
};
