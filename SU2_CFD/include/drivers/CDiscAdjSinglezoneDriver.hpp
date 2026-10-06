/*!
 * \file CDiscAdjSinglezoneDriver.hpp
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
#include "CSinglezoneDriver.hpp"

/*!
 * \class CDiscAdjSinglezoneDriver
 * \ingroup DiscAdj
 * \brief Class for driving single-zone adjoint solvers.
 * \author R. Sanchez
 * \version 8.5.0 "Harrier"
 */
class CDiscAdjSinglezoneDriver : public CSinglezoneDriver {
protected:

  unsigned long nAdjoint_Iter;                  /*!< \brief The number of adjoint iterations that are run on the fixed-point solver.*/
  RECORDING RecordingState;                     /*!< \brief The kind of recording the tape currently holds.*/
  RECORDING MainVariables;                      /*!< \brief The kind of recording linked to the main variables of the problem.*/
  RECORDING SecondaryVariables;                 /*!< \brief The kind of recording linked to the secondary variables of the problem.*/
  int MainSolver;                               /*!< \brief Index of the main adjoint solver. */
  su2double ObjFunc;                            /*!< \brief The value of the objective function.*/
  CIteration* direct_iteration;                 /*!< \brief A pointer to the direct iteration.*/

  CConfig *config;                              /*!< \brief Definition of the particular problem. */
  CIteration *iteration;                        /*!< \brief Container vector with all the iteration methods. */
  CIntegration **integration;                   /*!< \brief Container vector with all the integration methods. */
  CGeometry *geometry;                          /*!< \brief Geometrical definition of the problem. */
  CSolver **solver;                             /*!< \brief Container vector with all the solutions. */
  COutput *direct_output;
  CNumerics ***numerics;                        /*!< \brief Container vector with all the numerics. */

  /*!
   * \brief Returns true if the objective function does not depend on the main variables. In which case,
   * the adjoint variables are 0 and the sensitivities can be computed just with the secondary recording.
   */
  bool TrivialFunction() const {
    return config_container[ZONE_0]->GetnObj() == 1 && config_container[ZONE_0]->GetKind_ObjFunc() == VOLUME_FRACTION;
  }

  /*!
   * \brief Record one iteration of a flow iteration in within multiple zones.
   * \param[in] kind_recording - Type of recording (full list in ENUM_RECORDING, option_structure.hpp)
   */
  void SetRecording(RECORDING kind_recording);

  /*!
   * \brief Run one iteration of the solver.
   * \param[in] kind_recording - Type of recording (full list in ENUM_RECORDING, option_structure.hpp)
   */
  void DirectRun(RECORDING kind_recording);

  /*!
   * \brief Set the objective function.
   */
  void SetObjFunction(void);

  /*!
   * \brief Initialize the adjoint value of the objective function.
   */
  void SetAdjObjFunction(void);

  /*!
   * \brief Record the main computational path.
   */
  void MainRecording(void);

  /*!
   * \brief Record the secondary computational path.
   */
  void SecondaryRecording(void);

  /*--- Residual adjoint (stage G, ADAP_ADJ_LAMBDA= YES). ---*/
  su2passivematrix LambdaCheck_SolutionOut; /*!< \brief U_out of the main recording, kept for the check. */
  passivedouble LambdaSweep_SensAoA = 0.0;  /*!< \brief d/dAlpha (radians) of the capture sweep. */
  passivedouble LambdaSweep_SensMach = 0.0; /*!< \brief d/dMach of the capture sweep. */

  /*!
   * \brief One extra reverse sweep of the main recording that copies the adjoint of the right-hand side of the
   *        flow linear solve (the residual adjoint lambda) into the adjoint flow solver. The adjoint solution
   *        and the sensitivities are not changed. Needs the main tape.
   */
  void CaptureResidualAdjoint(void);

  /*!
   * \brief Developer check (ADAP_ADJ_LAMBDA_CHECK= YES) of the identity
   *        d/dp (tape sweep) = dJ/dp(U_out)|explicit - sum_owned lambda . dR/dp(U_in)
   *        for p = Alpha and Mach, with central finite differences of the residual and of the objective.
   *        Runs with the tape off, after the last use of the tapes; restores the flow solution and the
   *        free-stream velocity afterwards.
   */
  void CheckResidualAdjoint(void);

  /*!
   * \brief Goal-oriented metric (ADAP_SENSOR= GOAL, stage G) of the current mesh, after the residual adjoint capture
   *        and the secondary recording: fields (lambda, fluxes of Solution_Direct), H_go, then the metric. The tape must
   *        be inactive; the final output writes the fields.
   */
  void ComputeGoalMetric(void);

  /*--- Goal-oriented adaptation loop (stage G3, ADAP_LOOP with ADAP_SENSOR= GOAL). ---*/
  bool goalLoop = false;          /*!< \brief Inside RunGoalAdaptationLoop (Run writes no files, tracks residuals). */
  bool directStateSet = false;    /*!< \brief Solution_Direct holds the state of the current mesh (Preprocess). */
  unsigned long goalAdjointIters = 0; /*!< \brief Adjoint iterations executed by the last Run. */
  vector<pair<string, passivedouble>> goalAdjointMax, goalAdjointLast; /*!< \brief Largest and last value of each
                                                                           adjoint convergence field in the last Run. */

  /*!
   * \brief State of the primal phase of a cycle, kept right after it (the recordings run the direct iteration again).
   */
  struct PrimalPhase {
    unsigned long nIter = 0;      /*!< \brief Iterations executed. */
    bool converged = false;       /*!< \brief Stopped by the convergence criteria. */
    passivedouble J = 0.0, CL = 0.0, CD = 0.0; /*!< \brief Objective, lift and drag after the phase. */
    vector<pair<string, passivedouble>> residualMax, residualLast; /*!< \brief Convergence fields: largest, last. */
    passivedouble time = 0.0;     /*!< \brief Wall time (s). */
  };

  /*!
   * \brief Goal-oriented mesh adaptation loop: per cycle the primal phase (tape off, RunPrimalPhase), the primal files,
   *        the main recording and the adjoint (Run), the residual adjoint capture, on the last cycle the geometric
   *        sensitivities (SecondaryRecording), the goal metric (ComputeGoalMetric), the adjoint files; then, except on
   *        the last cycle, MMG with that metric and SwapMesh. Summary table and adap_goal_summary.csv at the end.
   */
  void RunGoalAdaptationLoop();

  /*!
   * \brief Primal phase of a cycle of the goal loop with the tape off: the direct iteration as SU2_CFD runs it (time and
   *        outer iteration 0, SetInitialCondition, Preprocess once, then Iterate, Postprocess and Monitor with the direct
   *        output), at most nIter iterations or until its convergence criteria are met.
   */
  PrimalPhase RunPrimalPhase(unsigned long nIter);

  /*!
   * \brief Pointers of the driver to the current mesh (config, iteration, solvers, numerics, geometry, integration) and
   *        the state of a new mesh: nothing recorded, direct output reset, counters at 0.
   */
  void RefreshMeshPointers();

  /*!
   * \brief gets Convergence on physical time scale, (deactivated in adjoint case)
   * \return false
   */
  inline bool GetTimeConvergence() const override { return false; }

public:

  /*!
   * \brief Constructor of the class.
   * \param[in] confFile - Configuration file name.
   * \param[in] val_nZone - Total number of zones.
   * \param[in] val_nDim - Total number of dimensions.
   * \param[in] MPICommunicator - MPI communicator for SU2.
   */
  CDiscAdjSinglezoneDriver(char* confFile,
             unsigned short val_nZone,
             SU2_Comm MPICommunicator);

  /*!
   * \brief Destructor of the class.
   */
  ~CDiscAdjSinglezoneDriver(void) override;

  /*!
   * \brief Preprocess the single-zone iteration
   * \param[in] TimeIter - index of the current time-step.
   */
  void Preprocess(unsigned long TimeIter) override;

  /*!
   * \brief Run a single iteration of the discrete adjoint solver with a single zone.
   */
  void Run(void) override;

  /*!
   * \brief Postprocess the adjoint iteration for ZONE_0.
   */
  void Postprocess(void) override;

  /*!
   * \brief [Overload] Launch the computation: the goal-oriented adaptation loop with ADAP_LOOP= YES, else as the
   *        single-zone driver.
   */
  void StartSolver() override;

  /*!
   * \brief Replace the mesh of the discrete adjoint problem (stage G2): the flow solution is set to the state the adjoint
   *        belongs to (Solution_Direct, if a Preprocess set it), the tape is reset (it refers to the solvers that are
   *        released), CSinglezoneDriver::ReplaceMesh builds the flow and adjoint solvers of the new mesh and runs the
   *        transfer (CDiscAdjTransfer: primal and adjoint), then the driver pointers are refreshed (RefreshMeshPointers).
   *        The next Preprocess records the new mesh. The tape must be inactive.
   * \param[in] remeshed - This rank's part of the new mesh.
   * \param[in] transfer - Sets the primal and adjoint solutions on the new mesh.
   */
  void SwapMesh(const CRemeshResult& remeshed, CSolutionTransfer& transfer);

  /*!
   * \brief SwapMesh with a complete mesh (tests; MPI: needed on the master rank only).
   */
  void SwapMesh(const CSimplexMesh& mesh, CSolutionTransfer& transfer);
};
