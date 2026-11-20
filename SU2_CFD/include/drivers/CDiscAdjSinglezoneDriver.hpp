/*!
 * \file CDiscAdjSinglezoneDriver.hpp
 * \brief Headers of the main subroutines for driving single or multi-zone problems.
 *        The subroutines and functions are in the <i>driver_structure.cpp</i> file.
 * \author T. Economon, H. Kline, R. Sanchez
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
#include "CSinglezoneDriver.hpp"

/*!
 * \class CDiscAdjSinglezoneDriver
 * \ingroup DiscAdj
 * \brief Class for driving single-zone adjoint solvers.
 * \author R. Sanchez
 * \version 8.6.0 "Harrier"
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

  /*!
   * \brief Classical EPM sensitivities after adjoint convergence; dispatches on SST_EPM_EXTRACTION.
   */
  void ExtractEPMSensitivities();

  /*!
   * \brief One scalar-amplitude replay per EPM case and probe (SST_EPM_EXTRACTION= SCALAR_REPLAY).
   */
  void ExtractEPMScalarReplay();

  /*!
   * \brief One field replay: per-site sensitivities, then the classical cases and probes as passive dot
   *        products over the stored sites (SST_EPM_EXTRACTION= FIELD).
   */
  void ExtractEPMFieldSensitivities();

  /*!
   * \brief Extract-only (SST_EPM_EXTRACT_ONLY): evaluate the residual criterion of the CONV_FIELD residuals measured
   *        by the single iteration from the loaded adjoint, set the output convergence flag read by the extraction
   *        guard, and record the values in epm_extract_only_residuals_json.
   */
  void CheckEPMExtractOnlyResidual();

  /*!
   * \brief Extract-only with MPI: residual of the total adjoint (owned value plus all halo copies, summed by the
   *        reverse halo exchange) after the measuring iteration, against the loaded adjoint. Fills
   *        epm_total_adjoint_residuals (log10, RMS_ADJ_* and MAX_ADJ_* history names); empty in serial, where the
   *        history residuals are already those of the total adjoint.
   * \param[in] loaded_adjoint - Adjoint solution before the iteration (layout of GetAllSolutions).
   */
  void MeasureEPMTotalAdjointResidual(const su2passivematrix& loaded_adjoint);

  vector<pair<string, passivedouble>> epm_total_adjoint_residuals; /*!< \brief See MeasureEPMTotalAdjointResidual. */

  std::string epm_extract_only_residuals_json;  /*!< \brief JSON value of the extract-only residuals ("criterion",
                                                     "fields" (log10, by CONV_FIELD name), "passed"); empty if not
                                                     extract-only. Written as "extract_only_residuals". */

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
};
