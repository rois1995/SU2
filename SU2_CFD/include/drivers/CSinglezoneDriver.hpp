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
class CRemesher;

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

  /*--- Time window of the metric of the time-domain adaptation loop. ---*/
  vector<su2double> windowHessian;  /*!< \brief Sum of |H| of each sensor over the time steps of the window. */
  unsigned long windowSamples = 0;  /*!< \brief Number of time steps in windowHessian. */
  su2double windowMetricTime = 0.0; /*!< \brief Time spent on the window metric (sensors, Hessians, metric). */
  su2double lastReplaceTime = 0.0, lastTransferTime = 0.0; /*!< \brief Times of the last ReplaceMesh. */

  /*--- ADAP_UNSTEADY_METRIC= PREDICT: first metric snapshot of the window (on the mesh of the window). ---*/
  vector<su2double> predictSnapshot;       /*!< \brief Metric of snapshot j (nPointDomain rows, upper triangle). */
  unsigned long predictSnapshotStep = 0;   /*!< \brief Time step of snapshot j. */
  bool predictSnapshotValid = false;       /*!< \brief Snapshot j exists on the current mesh. */
  unsigned long windowFirstStep = 0;       /*!< \brief First time step solved on the current mesh. */

  /*!
   * \brief Stop with an error if the mesh of this problem cannot be adapted or replaced: compressible EULER,
   *        NAVIER_STOKES or RANS, steady or dual time stepping, and the cases of CMMGInterface::CheckSupport (one
   *        rank, one zone, static mesh, ...).
   */
  void CheckMeshAdaptation() const;

  /*!
   * \brief Time-domain mesh adaptation loop (ADAP_LOOP with TIME_DOMAIN= YES, dual time stepping): the time loop of
   *        RunTimeLoop, and after every time step that ends a time window (CConfig::GetAdap_TimeWindowEnd, every
   *        ADAP_FREQ steps) the mesh is remeshed from the metric of that window (SampleTimeWindowMetric), the solution
   *        and its time history are transferred, and the run continues with the next time step on the new mesh.
   * \note Adaptation point: after Update (the dual-time history holds U^n in Solution_time_n and U^(n-1) in
   *       Solution_time_n1), Monitor and Output of time step n, before the preprocessing of step n+1. The time counters
   *       continue, no restart file is read. The output files keep the usual unsteady names (time step only). The
   *       restart files of the transferred steps n and n-1 are written right after the transfer and replace those of
   *       the previous mesh, so that the time steps on the new mesh can be restarted with it. The cycles (history
   *       column Adap_Cycle) are the time windows counted from time step 0 (cycle = time step / ADAP_FREQ).
   */
  void RunTimeAdaptationLoop();

  /*!
   * \brief Metric of the time window of the time-domain adaptation loop, called by Postprocess after each time step:
   *        adds the absolute Hessians |H| of the sensors of this time step to the window, and at the end of the
   *        window (CConfig::GetAdap_TimeWindowEnd) computes the metric from their mean over the window (the Hessians
   *        of the flow variables are replaced by the mean, so the volume output of that step shows what was used).
   * \note The mean of |H| over the window is the time integral of |H| over the window (Lp error in space, L1 in time,
   *       Alauzet & Olivier 2011) up to a constant that the complexity scaling removes. The window is the previous
   *       ADAP_FREQ time steps ("a posteriori" window): the new mesh covers where the features were, it lags behind
   *       features that move more than about their own size within one window.
   */
  void SampleTimeWindowMetric();

  /*!
   * \brief ADAP_UNSTEADY_METRIC= PREDICT, called by SampleTimeWindowMetric after each time step: the metric of the
   *        time steps of the two snapshots of the window (step end - ADAP_PREDICT_SEPARATION, or the first step on the
   *        current mesh if later, and the last step of the window) is computed without the boundary-layer metric;
   *        at the end of the window the motion of its features (optical flow of the invariant 0.5 log10 det M between
   *        the snapshots, CMetricPredictor) moves the last snapshot over ADAP_PREDICT_HORIZON time steps, the instants
   *        are intersected, and CSolver::ComputeMetric scales the result to the complexity, applies the bounds and the
   *        boundary-layer metric. Both snapshots are on the mesh of the window.
   */
  void PredictWindowMetric();

  /*!
   * \brief Write the restart files of the current solution (RESTART and RESTART_ASCII of OUTPUT_FILES) at the
   *        current time step and, with 2nd-order dual time stepping, the time history Solution_time_n1 at the step
   *        before (both transferred to the new mesh). They replace the files of these steps on the previous mesh.
   */
  void WriteTimeHistoryRestarts();

  /*!
   * \brief Solve on the current mesh: the time loop (one pass for steady problems), as StartSolver without ADAP_LOOP.
   */
  void RunTimeLoop();

  /*!
   * \brief Mesh adaptation loop (ADAP_LOOP): solve on the input mesh (cycle 0), then for each cycle remesh from the
   *        metric of the last solution with the options of its level (ADAP_SIZES, ...), transfer the solution
   *        (ADAP_TRANSFER) and solve again. Steady problems, single rank, MMG.
   * \note An adaptation cycle has three parts behind small interfaces: the metric (ComputeMetric, feature-based here,
   *       a goal-based metric overrides it), the remesher (CRemesher: MMG) and the solution transfer
   *       (CSolutionTransfer: barycentric or conservative). The mesh replacement (ReplaceMesh) is the same for all of them.
   */
  void RunAdaptationLoop();

  /*!
   * \brief Stop if an adapted mesh written with WRT_ADAP_MESH= YES would overwrite the input mesh or an output file.
   */
  void CheckAdaptedMeshNames() const;

  /*!
   * \brief Name of the adapted mesh of a cycle, without extension: MESH_OUT_FILENAME_adap_<cycle> for steady runs;
   *        MESH_OUT_FILENAME_<first time step solved on it> for time-domain runs (e.g. mesh_out_00160: built after
   *        step 159, used from step 160), so a restart at step k uses the mesh with the largest step <= k.
   * \param[in] iCycle - Adaptation cycle of the mesh (steady runs).
   * \param[in] firstTimeIter - First time step solved on the mesh (time-domain runs).
   */
  string AdaptedMeshName(unsigned long iCycle, unsigned long firstTimeIter) const;

  /*!
   * \brief Write the current mesh as the adapted mesh of a cycle (WRT_ADAP_MESH): AdaptedMeshName in the format of
   *        MESH_OUT_FORMAT, the markers are taken from the geometry.
   * \param[in] iCycle - Adaptation cycle of the mesh.
   * \param[in] firstTimeIter - First time step solved on the mesh (time-domain runs).
   */
  void WriteAdaptedMesh(unsigned long iCycle, unsigned long firstTimeIter = 0) const;

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
   * \brief [Overload] Launch the computation for single-zone problems (the mesh adaptation loop with ADAP_LOOP= YES).
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
   * \note Called at the end of Postprocess, outside of OpenMP parallel regions. The metric source of mesh adaptation:
   *       feature-based (Hessians of the ADAP_SENSOR fields); a driver with an adjoint overrides it for a goal-based
   *       metric. The metric is left in the flow variables (CVariable::GetMetric()).
   */
  virtual void ComputeMetric();

  /*!
   * \brief Remesh with MMG from the metric of the current flow solution (COMPUTE_METRIC= YES).
   * \note The metric is computed again from the current solution. The driver is not changed. Stops with an error
   *       if SU2 was built without MMG.
   * \return The adapted mesh, validated.
   */
  CSimplexMesh RemeshFromMetric();

  /*!
   * \brief Remesh with the given remesher from the metric of the current flow solution (COMPUTE_METRIC= YES).
   * \note As RemeshFromMetric(), with any remesher.
   * \param[in] remesher - Makes the new mesh from the geometry and the metric.
   * \return The adapted mesh, validated.
   */
  CSimplexMesh RemeshFromMetric(CRemesher& remesher);

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
