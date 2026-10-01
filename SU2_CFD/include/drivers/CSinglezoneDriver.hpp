/*!
 * \file CSinglezoneDriver.hpp
 * \brief Headers of the main subroutines for driving single or multi-zone problems.
 *        The subroutines and functions are in the <i>driver_structure.cpp</i> file.
 * \author T. Economon, H. Kline, R. Sanchez
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
#include "CDriver.hpp"
#include "../../../Common/include/adaptation/CSimplexMesh.hpp"

class CSolutionTransfer;

/*!
 * \class CSinglezoneDriver
 * \ingroup Drivers
 * \brief Class for driving single-zone solvers.
 * \author R. Sanchez
 * \version 8.5.0 "Harrier"
 */
class CSinglezoneDriver : public CDriver {
protected:

  unsigned long TimeIter;

  /*!
     * \brief  Returns whether all specified windowed-time-averaged ouputs have been converged
     * \return Boolean indicating whether the problem is converged.
     */
  virtual bool GetTimeConvergence() const;

  /*!
   * \brief Stop with an error if the mesh of this problem cannot be adapted or replaced: compressible EULER,
   *        NAVIER_STOKES or RANS, steady, and the cases of CMMGInterface::CheckSupport (one rank, one zone, ...).
   */
  void CheckMeshAdaptation() const;

public:

  /*!
   * \brief Constructor of the class.
   * \param[in] confFile - Configuration file name.
   * \param[in] val_nZone - Total number of zones.
   * \param[in] MPICommunicator - MPI communicator for SU2.
   */
  CSinglezoneDriver(char* confFile,
             unsigned short val_nZone,
             SU2_Comm MPICommunicator);

  /*!
   * \brief Destructor of the class.
   */
  ~CSinglezoneDriver(void) override;

  /*!
   * \brief [Overload] Launch the computation for single-zone problems.
   */
  void StartSolver() override;

  /*!
   * \brief Preprocess the single-zone iteration
   */
  void Preprocess(unsigned long TimeIter) override;

  /*!
   * \brief Run the iteration for ZONE_0.
   */
  void Run() override;

  /*!
   * \brief Postprocess the iteration for ZONE_0.
   */
  virtual void Postprocess();

  /*!
   * \brief Update the dual-time solution within multiple zones.
   */
  void Update() override;

  /*!
   * \brief Output the solution in solution file.
   */
  void Output(unsigned long TimeIter) override;

  /*!
   * \brief Perform a dynamic mesh deformation, included grid velocity computation and the update of the multigrid structure.
   */
  void DynamicMeshUpdate(unsigned long TimeIter) override;

  /*!
   * \brief Perform a mesh deformation as initial condition.
   */
  void SetInitialMesh() override;

  /*!
   * \brief Monitor
   * \param ExtIter
   */
  bool Monitor(unsigned long TimeIter) override;

  /*!
   * \brief Compute the mesh adaptation metric (COMPUTE_METRIC) from the flow solution.
   * \note Called at the end of Postprocess, outside of OpenMP parallel regions.
   */
  void ComputeMetric();

  /*!
   * \brief Remesh with MMG from the metric of the current flow solution (COMPUTE_METRIC= YES).
   * \note The metric is computed again from the current solution. The driver is not changed. Stops with an error
   *       if SU2 was built without MMG.
   * \return The adapted mesh, validated.
   */
  CSimplexMesh RemeshFromMetric();

  /*!
   * \brief Replace the mesh of the problem: geometry, solvers, numerics, integration and iteration are built for
   *        the new mesh, the solution is set by the transfer, then the driver switches to them and the previous ones
   *        are deleted. The config gets back its state from the start of the driver (CConfigRunState) and the
   *        output its convergence monitoring, so the next solve runs like a fresh run on the new mesh (from the
   *        transferred solution). The history file continues.
   * \note Everything is built while the driver still uses the previous mesh; any error stops the run before the
   *       switch. The restart files are not read (they belong to the first mesh). The element orientation is
   *       always checked (also with REORIENT_ELEMENTS= NO).
   * \param[in] mesh - New mesh, same markers as the current one (e.g. from RemeshFromMetric).
   * \param[in] transfer - Sets the solution on the new mesh from the previous one.
   */
  void ReplaceMesh(const CSimplexMesh& mesh, CSolutionTransfer& transfer);

  /*!
   * \brief One mesh adaptation cycle: RemeshFromMetric, then ReplaceMesh. Call it after a converged solve
   *        (e.g. StartSolver), then solve again.
   * \note Developer entry point, not used by the normal run.
   * \param[in] transfer - Sets the solution on the new mesh from the previous one.
   */
  void AdaptMesh(CSolutionTransfer& transfer);

};
