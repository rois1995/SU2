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

#include "../../../Common/include/containers/C2DContainer.hpp"
#include "../../../Common/include/option_structure.hpp"

class CConfig;
class CGeometry;
class CSolver;
class CVariable;

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
  /*!
   * \brief Short report of the last transfer for the tables of the adaptation loop.
   */
  struct Report {
    unsigned long nOutside = 0;           /*!< \brief New points (or control volumes) not inside the donor mesh. */
    passivedouble maxDistance = 0.0;      /*!< \brief Largest distance to the donor used for them. */
    passivedouble maxRelDistance = 0.0;   /*!< \brief Same, relative to the size of the donor face. */
    passivedouble conservationDefect = -1.0; /*!< \brief Largest relative change of the integrals of the conservative
                                                  variables (-1: not measured). */
  };

  virtual ~CSolutionTransfer() = default;

  /*!
   * \brief Set the solution of the new mesh.
   * \param[in] config - Definition of the problem, describes the new mesh.
   * \param[in] donor - Geometry and solvers of the previous mesh.
   * \param[in] geometry - Geometry of each multigrid level of the new mesh.
   * \param[in,out] solver - Solvers of each multigrid level of the new mesh.
   */
  virtual void Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver) = 0;

  /*!
   * \brief Report of the last transfer.
   */
  virtual Report GetReport() const { return Report(); }

 protected:
  using ArrayGetter = su2activematrix& (*)(CVariable*);

  /*!
   * \brief Arrays of the solvers that a transfer sets: the solution and, in the time domain, its history.
   */
  struct TransferArrays {
    std::vector<unsigned short> solverIndices; /*!< \brief Transferred solvers (flow, turbulence). */
    bool hasTimeN = false, hasTimeN1 = false;  /*!< \brief History arrays allocated (time domain). */
    bool interpolateTimeN1 = false;            /*!< \brief U^(n-1) is transferred (2nd-order dual time stepping). */
    std::vector<ArrayGetter> history;          /*!< \brief Getters of the history arrays (Solution_time_n, _n1). */
    std::vector<MPI_QUANTITIES> historyComms;  /*!< \brief MPI quantities of the history arrays. */
  };

  /*!
   * \brief Check that the problem is supported (compressible flow, SA or SST, one rank, static mesh) and that the
   *        donor and the new mesh have the same solvers and arrays; in the time domain, that the donor solution and
   *        Solution_time_n are the same state (the transfer is done at the end of a time step).
   * \param[in] name - Name of the transfer for the messages.
   */
  static TransferArrays CheckProblem(const std::string& name, CConfig* config, const CMeshDonor& donor,
                                     CGeometry** geometry, CSolver*** solver);

  /*!
   * \brief As after loading a restart file: old solution, communication, primitive variables, eddy viscosity,
   *        restriction to the coarse levels, and the restriction of the time history.
   */
  static void FinishTransfer(CConfig* config, CGeometry** geometry, CSolver*** solver, const TransferArrays& arrays);
};

/*!
 * \class CFreeStreamTransfer
 * \brief No transfer: the new mesh keeps the free-stream state the solvers were created with.
 */
class CFreeStreamTransfer final : public CSolutionTransfer {
 public:
  void Transfer(CConfig*, const CMeshDonor&, CGeometry**, CSolver***) override {}
};
