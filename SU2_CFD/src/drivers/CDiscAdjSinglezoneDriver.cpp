/*!
 * \file driver_adjoint_singlezone.cpp
 * \brief The main subroutines for driving adjoint single-zone problems.
 * \author R. Sanchez
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

#include "../../include/drivers/CDiscAdjSinglezoneDriver.hpp"
#include "../../include/output/tools/CWindowingTools.hpp"
#include "../../include/output/COutputFactory.hpp"
#include "../../include/output/COutput.hpp"
#include "../../include/iteration/CIterationFactory.hpp"
#include "../../include/iteration/CTurboIteration.hpp"
#include "../../../Common/include/toolboxes/CQuasiNewtonInvLeastSquares.hpp"
#include "../../../Common/include/CEPMFieldSites.hpp"
#include "../../include/numerics/CNumerics.hpp"


CDiscAdjSinglezoneDriver::CDiscAdjSinglezoneDriver(char* confFile,
                                                   unsigned short val_nZone,
                                                   SU2_Comm MPICommunicator) : CSinglezoneDriver(confFile,
                                                                                                 val_nZone,
                                                                                                 MPICommunicator) {


  /*--- Store the number of internal iterations that will be run by the adjoint solver ---*/
  nAdjoint_Iter = config_container[ZONE_0]->GetnInner_Iter();

  /*--- Store the pointers ---*/
  config      = config_container[ZONE_0];
  iteration   = iteration_container[ZONE_0][INST_0];
  solver      = solver_container[ZONE_0][INST_0][MESH_0];
  numerics    = numerics_container[ZONE_0][INST_0][MESH_0];
  geometry    = geometry_container[ZONE_0][INST_0][MESH_0];
  integration = integration_container[ZONE_0][INST_0];

  /*--- Store the recording state ---*/
  RecordingState = RECORDING::CLEAR_INDICES;

  /*--- Initialize the direct iteration ---*/

  switch (config->GetKind_Solver()) {

  case MAIN_SOLVER::DISC_ADJ_EULER: case MAIN_SOLVER::DISC_ADJ_NAVIER_STOKES: case MAIN_SOLVER::DISC_ADJ_RANS:
  case MAIN_SOLVER::DISC_ADJ_INC_EULER: case MAIN_SOLVER::DISC_ADJ_INC_NAVIER_STOKES: case MAIN_SOLVER::DISC_ADJ_INC_RANS:
    if (rank == MASTER_NODE)
      cout << "Direct iteration: Euler/Navier-Stokes/RANS equation." << endl;

    if (config->GetBoolTurbomachinery()) {
      direct_iteration = new CTurboIteration(config);
    }
    else { direct_iteration = CIterationFactory::CreateIteration(MAIN_SOLVER::EULER, config); }

    if (config->GetKind_Regime() == ENUM_REGIME::COMPRESSIBLE) {
      direct_output = COutputFactory::CreateOutput(MAIN_SOLVER::EULER, config, nDim);
    }
    else { direct_output =  COutputFactory::CreateOutput(MAIN_SOLVER::INC_EULER, config, nDim); }

    MainVariables = RECORDING::SOLUTION_VARIABLES;
    if (config->GetDeform_Mesh()) {
      SecondaryVariables = RECORDING::MESH_DEFORM;
    }
    else { SecondaryVariables = RECORDING::MESH_COORDS; }
    MainSolver = ADJFLOW_SOL;
    break;

  case MAIN_SOLVER::DISC_ADJ_FEM_EULER : case MAIN_SOLVER::DISC_ADJ_FEM_NS : case MAIN_SOLVER::DISC_ADJ_FEM_RANS :
    if (rank == MASTER_NODE)
      cout << "Direct iteration: Euler/Navier-Stokes/RANS equation." << endl;
    direct_iteration = CIterationFactory::CreateIteration(MAIN_SOLVER::FEM_EULER, config);
    direct_output = COutputFactory::CreateOutput(MAIN_SOLVER::FEM_EULER, config, nDim);
    MainVariables = RECORDING::SOLUTION_VARIABLES;
    SecondaryVariables = RECORDING::MESH_COORDS;
    MainSolver = ADJFLOW_SOL;
    break;

  case MAIN_SOLVER::DISC_ADJ_FEM:
    if (rank == MASTER_NODE)
      cout << "Direct iteration: elasticity equation." << endl;
    direct_iteration =  CIterationFactory::CreateIteration(MAIN_SOLVER::FEM_ELASTICITY, config);
    direct_output = COutputFactory::CreateOutput(MAIN_SOLVER::FEM_ELASTICITY, config, nDim);
    MainVariables = RECORDING::SOLUTION_VARIABLES;
    SecondaryVariables = RECORDING::MESH_COORDS;
    MainSolver = ADJFEA_SOL;
    break;

  case MAIN_SOLVER::DISC_ADJ_HEAT:
    if (rank == MASTER_NODE)
      cout << "Direct iteration: heat equation." << endl;
    direct_iteration = CIterationFactory::CreateIteration(MAIN_SOLVER::HEAT_EQUATION, config);
    direct_output = COutputFactory::CreateOutput(MAIN_SOLVER::HEAT_EQUATION, config, nDim);
    MainVariables = RECORDING::SOLUTION_VARIABLES;
    SecondaryVariables = RECORDING::MESH_COORDS;
    MainSolver = ADJHEAT_SOL;
    break;

  default:
    break;

  }

 direct_output->PreprocessHistoryOutput(config, false);

}

CDiscAdjSinglezoneDriver::~CDiscAdjSinglezoneDriver() {
  SU2_ZONE_SCOPED

  delete direct_iteration;
  delete direct_output;

}

void CDiscAdjSinglezoneDriver::Preprocess(unsigned long TimeIter) {
  SU2_ZONE_SCOPED

  /*--- Set the current time iteration in the config and also in the driver
   * because the python interface doesn't offer an explicit way of doing it. ---*/

  this->TimeIter = TimeIter;
  config_container[ZONE_0]->SetTimeIter(TimeIter);

  /*--- Preprocess the adjoint iteration ---*/

  iteration->Preprocess(output_container[ZONE_0], integration_container, geometry_container,
                        solver_container, numerics_container, config_container,
                        surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

  /*--- For the adjoint iteration we need the derivatives of the iteration function with
   *--- respect to the conservative variables. Since these derivatives do not change in the steady state case
   *--- we only have to record if the current recording is different from the main variables. ---*/

  if (RecordingState != MainVariables){
    MainRecording();
  }

}

void CDiscAdjSinglezoneDriver::Run() {
  SU2_ZONE_SCOPED

  /*--- No need to solve anything, the tape for the main recording is empty. ---*/
  if (TrivialFunction()) {
    SetAllSolutions(ZONE_0, true, [](auto, auto) { return 0; });
    return;
  }

  CQuasiNewtonInvLeastSquares<passivedouble> fixPtCorrector;
  if (config->GetnQuasiNewtonSamples() > 1) {
    fixPtCorrector.resize(config->GetnQuasiNewtonSamples(),
                          geometry_container[ZONE_0][INST_0][MESH_0]->GetnPoint(),
                          GetTotalNumberOfVariables(ZONE_0,true),
                          geometry_container[ZONE_0][INST_0][MESH_0]->GetnPointDomain());

    if (TimeIter != 0) GetAllSolutions(ZONE_0, true, fixPtCorrector);
  }

  /*--- EPM extract-only (SST_EPM_EXTRACT_ONLY, ITER= 1): the single iteration only measures the residual of the
   *    loaded adjoint; its update is discarded below, so the replay is seeded with the loaded adjoint. ---*/
  const bool epm_extract_only = config->GetEPMExtraction().enabled && config->GetEPMExtraction().extract_only;
  su2passivematrix loaded_adjoint;
  if (epm_extract_only) {
    loaded_adjoint.resize(geometry_container[ZONE_0][INST_0][MESH_0]->GetnPoint(),
                          GetTotalNumberOfVariables(ZONE_0, true));
    GetAllSolutions(ZONE_0, true, loaded_adjoint);
  }

  for (auto Adjoint_Iter = 0ul; Adjoint_Iter < nAdjoint_Iter; Adjoint_Iter++) {

    /*--- Initialize the adjoint of the output variables of the iteration with the adjoint solution
     *--- of the previous iteration. The values are passed to the AD tool.
     *--- Issues with iteration number should be dealt with once the output structure is in place. ---*/

    config->SetInnerIter(Adjoint_Iter);

    iteration->InitializeAdjoint(solver_container, geometry_container, config_container, ZONE_0, INST_0);

    /*--- Initialize the adjoint of the objective function with 1.0. ---*/

    SetAdjObjFunction();

    /*--- Interpret the stored information by calling the corresponding routine of the AD tool. ---*/

    AD::ComputeAdjoint();

    /*--- Extract the computed adjoint values of the input variables and store them for the next iteration. ---*/

    iteration->IterateDiscAdj(geometry_container, solver_container,
                              config_container, ZONE_0, INST_0, false);

    /*--- Monitor the pseudo-time ---*/

    StopCalc = iteration->Monitor(output_container[ZONE_0], integration_container, geometry_container,
                                  solver_container, numerics_container, config_container,
                                  surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

    /*--- Clear the stored adjoint information to be ready for a new evaluation. ---*/

    AD::ClearAdjoints();

    /*--- Output files for steady state simulations. ---*/

    if (!config->GetTime_Domain()) {
      iteration->Output(output_container[ZONE_0], geometry_container, solver_container,
                        config_container, Adjoint_Iter, false, ZONE_0, INST_0);
    }

    if (StopCalc) break;

    /*--- Correct the solution with the quasi-Newton approach. ---*/

    if (fixPtCorrector.size()) {
      GetAllSolutions(ZONE_0, true, fixPtCorrector.FPresult());
      SetAllSolutions(ZONE_0, true, fixPtCorrector.compute());
    }

  }

  if (epm_extract_only) {
    SetAllSolutions(ZONE_0, true, loaded_adjoint);
    CheckEPMExtractOnlyResidual();
  }

}

void CDiscAdjSinglezoneDriver::CheckEPMExtractOnlyResidual() {
  auto* output = output_container[ZONE_0];
  const passivedouble criterion = SU2_TYPE::GetValue(config->GetMinLogResidual());

  /*--- Current values of the monitored residual fields. The output drops CONV_FIELD entries that are not history
   *    fields, so every configured entry (all RMS_/MAX_, checked in CConfig) must be found here; with no
   *    CONV_FIELD the output's default field is used. ---*/
  const auto monitored = output->GetResidualConvFields();
  vector<string> names;
  for (unsigned short iField = 0; iField < config->GetnConv_Field(); ++iField)
    names.push_back(config->GetConv_Field(iField));
  if (names.empty())
    for (const auto& field : monitored) names.push_back(field.first);
  if (names.empty())
    SU2_MPI::Error("SST_EPM_EXTRACT_ONLY found no adjoint residual convergence field.", CURRENT_FUNCTION);

  /*--- COutput::ConvergenceMonitoring never converges on the first iteration (nor before CONV_STARTITER), so with
   *    ITER= 1 the residual criterion is evaluated here, on the same history values and with the same test. ---*/
  const auto json_number = [](passivedouble value) {
    std::ostringstream text;
    text << std::setprecision(17);
    if (std::isfinite(value)) text << value; else text << "null";
    return text.str();
  };
  bool passed = true;
  std::ostringstream json, screen;
  json << "{\"criterion\": " << json_number(criterion) << ", \"fields\": {";
  screen << std::setprecision(6);
  for (size_t iField = 0; iField < names.size(); ++iField) {
    const auto it = std::find_if(monitored.begin(), monitored.end(),
                                 [&](const pair<string, passivedouble>& f) { return f.first == names[iField]; });
    if (it == monitored.end())
      SU2_MPI::Error("SST_EPM_EXTRACT_ONLY: convergence field " + names[iField] +
                     " is not an adjoint residual history field.", CURRENT_FUNCTION);
    const passivedouble value = it->second;
    passed = passed && value <= criterion;
    json << (iField ? ", " : "") << '"' << names[iField] << "\": " << json_number(value);
    screen << "  " << names[iField] << " = " << value << "\n";
  }

  /*--- History residuals are global reductions; agree on the outcome anyway. ---*/
  unsigned short local = passed, global = 0;
  SU2_MPI::Allreduce(&local, &global, 1, MPI_UNSIGNED_SHORT, MPI_MIN, SU2_MPI::GetComm());
  passed = global > 0;
  json << "}, \"passed\": " << (passed ? "true" : "false") << "}";
  epm_extract_only_residuals_json = json.str();

  if (rank == MASTER_NODE)
    cout << "\nEPM extract-only: residual of one adjoint iteration from the loaded adjoint (update discarded, "
         << "criterion CONV_RESIDUAL_MINVAL= " << criterion << "): " << (passed ? "passed" : "FAILED") << "\n"
         << screen.str() << endl;

  /*--- The extraction guard reads this flag; an interrupt keeps the flag set by the output. ---*/
  if (!output->GetConvergenceInterrupted()) output->SetConvergence(passed);
}

void CDiscAdjSinglezoneDriver::Postprocess() {
  SU2_ZONE_SCOPED

  if (config->GetEPMExtraction().enabled) ExtractEPMSensitivities();

  switch(config->GetKind_Solver())
  {
    case MAIN_SOLVER::DISC_ADJ_EULER :     case MAIN_SOLVER::DISC_ADJ_NAVIER_STOKES :     case MAIN_SOLVER::DISC_ADJ_RANS :
    case MAIN_SOLVER::DISC_ADJ_INC_EULER : case MAIN_SOLVER::DISC_ADJ_INC_NAVIER_STOKES : case MAIN_SOLVER::DISC_ADJ_INC_RANS :
    case MAIN_SOLVER::DISC_ADJ_HEAT :

      /*--- Compute the geometrical sensitivities ---*/
      SecondaryRecording();
      break;

    case MAIN_SOLVER::DISC_ADJ_FEM :

      /*--- Compute the geometrical sensitivities ---*/
      SecondaryRecording();

      iteration->Postprocess(output_container[ZONE_0], integration_container, geometry_container,
                             solver_container, numerics_container, config_container,
                             surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);
      break;

    default:
      break;

  }//switch

}

void CDiscAdjSinglezoneDriver::SetRecording(RECORDING kind_recording){
  SU2_ZONE_SCOPED

  AD::Reset();

  /*--- Prepare for recording by resetting the solution to the initial converged solution. ---*/

  for (unsigned short iSol=0; iSol < MAX_SOLS; iSol++) {
    for (unsigned short iMesh = 0; iMesh <= config_container[ZONE_0]->GetnMGLevels(); iMesh++) {
      auto solver = solver_container[ZONE_0][INST_0][iMesh][iSol];
      if (solver && solver->GetAdjoint()) {
        SU2_OMP_PARALLEL_(if (solver->GetHasHybridParallel()))
        solver->SetRecording(geometry_container[ZONE_0][INST_0][iMesh], config_container[ZONE_0]);
        END_SU2_OMP_PARALLEL
      }
    }
  }

  if (rank == MASTER_NODE) {
    cout << "\n-------------------------------------------------------------------------\n";
    switch(kind_recording) {
    case RECORDING::CLEAR_INDICES: cout << "Clearing the computational graph." << endl; break;
    case RECORDING::MESH_COORDS:   cout << "Storing computational graph wrt MESH COORDINATES." << endl; break;
    case RECORDING::SOLUTION_VARIABLES:
      cout << "Direct iteration to store the primal computational graph.\n";
      cout << "Computing residuals to check the convergence of the direct problem." << endl; break;
    default: break;
    }
  }

  /*---Enable recording and register input of the iteration --- */

  if (kind_recording != RECORDING::CLEAR_INDICES){

    AD::StartRecording();

    iteration->RegisterInput(solver_container, geometry_container, config_container, ZONE_0, INST_0, kind_recording);
    auto& epm = config->GetEPMExtraction();
    if (epm.active && epm.mode == CEPMExtraction::Mode::FIELD) {
      if (!epm.field) SU2_MPI::Error("EPM field recording without a field store.", CURRENT_FUNCTION);
      epm.field->RegisterInputs();
    } else if (epm.active) {
      auto& amplitude = epm.amplitude;
      amplitude = 0.0;
      AD::ResetInput(amplitude);
      AD::RegisterInput(amplitude);
    }
  }

  /*--- Set the dependencies of the iteration ---*/

  iteration->SetDependencies(solver_container, geometry_container, numerics_container, config_container, ZONE_0,
                             INST_0, kind_recording);

  /*--- Do one iteration of the direct solver ---*/

  DirectRun(kind_recording);

  /*--- Store the recording state ---*/

  RecordingState = kind_recording;

  /*--- Register Output of the iteration ---*/

  iteration->RegisterOutput(solver_container, geometry_container, config_container, ZONE_0, INST_0);

  /*--- Extract the objective function and store it --- */

  SetObjFunction();

  if (rank == MASTER_NODE &&
      (kind_recording == RECORDING::SOLUTION_VARIABLES || (TrivialFunction() && kind_recording == RECORDING::MESH_COORDS))) {
    cout << "\nObjective function value: " << std::setprecision(config_container[ZONE_0]->GetOutput_Precision()) << ObjFunc;
    cout << "\n-------------------------------------------------------------------------\n" << endl;
  }

  if (kind_recording != RECORDING::CLEAR_INDICES && config_container[ZONE_0]->GetWrt_AD_Statistics()) {
    AD::PrintStatistics(SU2_MPI::GetComm(), rank == MASTER_NODE);
  }

  AD::StopRecording();

}

void CDiscAdjSinglezoneDriver::SetAdjObjFunction(){
  SU2_ZONE_SCOPED
  su2double seeding = 1.0;

  if (config->GetTime_Domain()) {
    const auto IterAvg_Obj = config->GetIter_Avg_Objective();
    if (TimeIter < IterAvg_Obj) {
      /*--- Default behavior when no window is chosen is to use Square-Windowing, i.e. the numerator equals 1.0 ---*/
      auto windowEvaluator = CWindowingTools();
      const su2double weight = windowEvaluator.GetWndWeight(config->GetKindWindow(), TimeIter, IterAvg_Obj - 1);
      seeding = weight / IterAvg_Obj;
    }
    else {
      seeding = 0.0;
    }
  }
  if (rank == MASTER_NODE) {
    SU2_TYPE::SetDerivative(ObjFunc, SU2_TYPE::GetValue(seeding));
  } else {
    SU2_TYPE::SetDerivative(ObjFunc, 0.0);
  }
}

void CDiscAdjSinglezoneDriver::SetObjFunction(){
  SU2_ZONE_SCOPED

  ObjFunc = 0.0;

  /*--- Specific scalar objective functions ---*/

  switch (config->GetKind_Solver()) {
  case MAIN_SOLVER::DISC_ADJ_INC_EULER:       case MAIN_SOLVER::DISC_ADJ_INC_NAVIER_STOKES:      case MAIN_SOLVER::DISC_ADJ_INC_RANS:
  case MAIN_SOLVER::DISC_ADJ_EULER:           case MAIN_SOLVER::DISC_ADJ_NAVIER_STOKES:          case MAIN_SOLVER::DISC_ADJ_RANS:
  case MAIN_SOLVER::DISC_ADJ_FEM_EULER:       case MAIN_SOLVER::DISC_ADJ_FEM_NS:                 case MAIN_SOLVER::DISC_ADJ_FEM_RANS:

    /*--- Surface based obj. function ---*/

    direct_output->SetHistoryOutput(geometry, solver, config, config->GetTimeIter(),
                                     config->GetOuterIter(), config->GetInnerIter());
    ObjFunc += solver[FLOW_SOL]->GetTotal_ComboObj();
    break;

  case MAIN_SOLVER::DISC_ADJ_HEAT:
    direct_output->SetHistoryOutput(geometry, solver, config, config->GetTimeIter(),
                                     config->GetOuterIter(), config->GetInnerIter());
    ObjFunc = solver[HEAT_SOL]->GetTotal_ComboObj();
    break;

  case MAIN_SOLVER::DISC_ADJ_FEM:
    solver[FEA_SOL]->Postprocessing(geometry, config, numerics_container[ZONE_0][INST_0][MESH_0][FEA_SOL], true);

    direct_output->SetHistoryOutput(geometry, solver, config, config->GetTimeIter(),
                                   config->GetOuterIter(), config->GetInnerIter());
    ObjFunc = solver[FEA_SOL]->GetTotal_ComboObj();
    break;

  default:
    break;
  }

  if (rank == MASTER_NODE){
    AD::RegisterOutput(ObjFunc);
  }

}

void CDiscAdjSinglezoneDriver::DirectRun(RECORDING kind_recording){
  SU2_ZONE_SCOPED

  /*--- Mesh movement ---*/

  direct_iteration->SetMesh_Deformation(geometry_container[ZONE_0][INST_0], solver, numerics, config, kind_recording);

  /*--- Zone preprocessing ---*/

  direct_iteration->Preprocess(direct_output, integration_container, geometry_container, solver_container, numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

  /*--- Iterate the direct solver ---*/

  direct_iteration->Iterate(direct_output, integration_container, geometry_container, solver_container, numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

  /*--- Postprocess the direct solver ---*/

  direct_iteration->Postprocess(direct_output, integration_container, geometry_container, solver_container, numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

  /*--- Print the direct residual to screen ---*/

  PrintDirectResidual(kind_recording);

}

void CDiscAdjSinglezoneDriver::MainRecording(){
  SU2_ZONE_SCOPED

  /*--- We know this function only depends on the secondary variables, hence skip the main recording. ---*/

  if (TrivialFunction()) {
    if (rank == MASTER_NODE) {
      cout << "Trivial objective function, skipping the solution of the adjoint equations." << endl;
    }
    return;
  }

  /*--- SetRecording stores the computational graph on one iteration of the direct problem. Calling it with
   *    RECORDING::CLEAR_INDICES as argument ensures that all information from a previous recording is removed. ---*/

  SetRecording(RECORDING::CLEAR_INDICES);

  /*--- Store the computational graph of one direct iteration with the solution variables as input. ---*/

  SetRecording(MainVariables);

}

void CDiscAdjSinglezoneDriver::SecondaryRecording(){
  SU2_ZONE_SCOPED
  /*--- SetRecording stores the computational graph on one iteration of the direct problem. Calling it with
   *    RECORDING::CLEAR_INDICES as argument ensures that all information from a previous recording is removed. ---*/

  SetRecording(RECORDING::CLEAR_INDICES);

  /*--- Store the computational graph of one direct iteration with the secondary variables as input. ---*/

  SetRecording(SecondaryVariables);

  /*--- Initialize the adjoint of the output variables of the iteration with the adjoint solution
   *    of the current iteration. The values are passed to the AD tool. ---*/

  iteration->InitializeAdjoint(solver_container, geometry_container, config_container, ZONE_0, INST_0);

  /*--- Initialize the adjoint of the objective function with 1.0. ---*/

  SetAdjObjFunction();

  /*--- Interpret the stored information by calling the corresponding routine of the AD tool. ---*/

  AD::ComputeAdjoint();

  /*--- Extract the computed sensitivity values. ---*/

  if (SecondaryVariables == RECORDING::MESH_COORDS) {
    solver[MainSolver]->SetSensitivity(geometry, config);
  }
  else { // MESH_DEFORM
    solver[ADJMESH_SOL]->SetSensitivity(geometry, config, solver[MainSolver]);
  }

  /*--- Clear the stored adjoint information to be ready for a new evaluation. ---*/

  AD::ClearAdjoints();

}

void CDiscAdjSinglezoneDriver::ExtractEPMSensitivities() {
  if (!output_container[ZONE_0]->GetConvergence() || output_container[ZONE_0]->GetConvergenceInterrupted())
    SU2_MPI::Error("EPM extraction requires the native adjoint convergence criterion to pass.", CURRENT_FUNCTION);

  if (config->GetEPMExtraction().mode == CEPMExtraction::Mode::FIELD) ExtractEPMFieldSensitivities();
  else ExtractEPMScalarReplay();
}

void CDiscAdjSinglezoneDriver::ExtractEPMScalarReplay() {
  using PassiveMPI = SelectMPIWrapper<passivedouble>::W;
  auto& epm = config->GetEPMExtraction();
  const auto saved_iter = config->GetInnerIter();
  const passivedouble baseline = SU2_TYPE::GetValue(ObjFunc);
  std::vector<passivedouble> baseline_outputs[2], adjoints[2];
  const unsigned short direct_blocks[] = {FLOW_SOL, TURB_SOL};
  const unsigned short adjoint_blocks[] = {ADJFLOW_SOL, ADJTURB_SOL};
  for (unsigned short block = 0; block < 2; ++block) {
    for (unsigned long point = 0; point < geometry->GetnPoint(); ++point)
      for (unsigned short var = 0; var < solver[direct_blocks[block]]->GetnVar(); ++var) {
        baseline_outputs[block].push_back(SU2_TYPE::GetValue(solver[direct_blocks[block]]->GetNodes()->GetSolution(point,var)));
        adjoints[block].push_back(SU2_TYPE::GetValue(solver[adjoint_blocks[block]]->GetNodes()->GetSolution(point,var)));
      }
  }
  const char* names[] = {"1C_KEEP", "1C_SWAP", "2C_KEEP", "2C_SWAP", "3C"};
  std::ostringstream rows;
  rows << std::setprecision(17);
  rows << "case,objective,dimension,mpi_ranks,probe_pair,probe_angle,J_baseline,dJ_da_at_zero,J_linear_at_one,"
          "ambiguous_evaluations,total_evaluations,stress_evaluations,source_evaluations,"
          "gap_tolerance,delta_b,tensor_blend_factor,"
          "primal_converged,adjoint_converged,status,extraction\n";
  passivedouble lower = baseline, upper = baseline;

  for (unsigned short mode = 0; mode < 5; ++mode) {
    epm.component = mode / 2 + 1;
    epm.permute = mode % 2;
    for (unsigned short probe = 0; probe < 4; ++probe) {
      epm.active = false;
      config->SetInnerIter(saved_iter);
      SetRecording(RECORDING::CLEAR_INDICES);
      epm.angle = probe == 0 ? 0.0 : PI_NUMBER / 2;
      epm.probe_pair = probe == 0 ? 3 : probe - 1;
      epm.evaluations = epm.ambiguous = epm.stress_evaluations = 0;
      epm.active = true;
      config->SetInnerIter(saved_iter);
      SetRecording(MainVariables);
      const auto recorded_objective = SU2_TYPE::GetValue(ObjFunc);
      if (!std::isfinite(recorded_objective) || fabs(recorded_objective - baseline) > 1e-10 * max(1.0, fabs(baseline)))
        SU2_MPI::Error("EPM zero-amplitude replay changed the baseline objective.", CURRENT_FUNCTION);
      for (unsigned short block = 0; block < 2; ++block) {
        size_t entry = 0;
        for (unsigned long point = 0; point < geometry->GetnPoint(); ++point)
          for (unsigned short var = 0; var < solver[direct_blocks[block]]->GetnVar(); ++var, ++entry) {
            const auto value = SU2_TYPE::GetValue(solver[direct_blocks[block]]->GetNodes()->GetSolution(point,var));
            if (!std::isfinite(value) || fabs(value - baseline_outputs[block][entry]) >
                1e-12 * max(1.0, fabs(baseline_outputs[block][entry])))
              SU2_MPI::Error("EPM replay changed the baseline iteration output.", CURRENT_FUNCTION);
            if (SU2_TYPE::GetValue(solver[adjoint_blocks[block]]->GetNodes()->GetSolution(point,var)) != adjoints[block][entry])
              SU2_MPI::Error("EPM replay changed the saved coupled adjoint.", CURRENT_FUNCTION);
          }
      }
      iteration->InitializeAdjoint(solver_container, geometry_container, config_container, ZONE_0, INST_0);
      SetAdjObjFunction();
      AD::ComputeAdjoint();
      const passivedouble local_gradient = SU2_TYPE::GetDerivative(epm.amplitude);
      passivedouble gradient = 0;
      PassiveMPI::Allreduce(&local_gradient, &gradient, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      const unsigned long local_counts[] = {epm.ambiguous, epm.evaluations, epm.stress_evaluations};
      unsigned long global_counts[3] = {};
      PassiveMPI::Allreduce(local_counts, global_counts, 3, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
      if (!std::isfinite(gradient)) SU2_MPI::Error("Nonfinite EPM sensitivity.", CURRENT_FUNCTION);
      rows << names[mode] << ',' << (config->GetKind_ObjFunc() == DRAG_COEFFICIENT ? "DRAG" : "LIFT") << ','
           << nDim << ',' << SU2_MPI::GetSize() << ','
           << epm.probe_pair << ',' << epm.angle << ',' << baseline << ',' << gradient << ',' << baseline + gradient << ','
           << global_counts[0] << ',' << global_counts[1] << ',' << global_counts[2] << ','
           << global_counts[1] - global_counts[2] << ',' << CEPMExtraction::gap_tolerance
           << ",1,1,unverified,true,frozen_forcing_unverified,scalar_replay\n";
      lower = min(lower, baseline + gradient);
      upper = max(upper, baseline + gradient);
      AD::ClearAdjoints();
      epm.active = false;
      // Every rank must perform the same recordings and reverse communication sequence.
      if (global_counts[0] == 0) break;
    }
  }

  // Clear tape indices, then restore all direct blocks and rebuild their dependencies.
  SetRecording(RECORDING::CLEAR_INDICES);
  for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol)
    if (solver[iSol] && solver[iSol]->GetAdjoint()) solver[iSol]->SetRecording(geometry, config);
  config->SetInnerIter(saved_iter);
  iteration->SetDependencies(solver_container, geometry_container, numerics_container, config_container,
                             ZONE_0, INST_0, MainVariables);
  AD::ResetInput(epm.amplitude);
  epm.amplitude = 0.0;
  if (rank == MASTER_NODE) {
    std::ofstream output("sst_epm_sensitivities.csv");
    output << rows.str();
    output.close();
    if (!output) SU2_MPI::Error("Could not write sst_epm_sensitivities.csv.", CURRENT_FUNCTION);
    cout << "\nClassical EPM first-order frozen-forcing predictions: [" << lower << ", " << upper
         << "]. Includes baseline and sampled orientations; not verified nonlinear bounds.\n";
  }
}

void CDiscAdjSinglezoneDriver::ExtractEPMFieldSensitivities() {
  using PassiveMPI = SelectMPIWrapper<passivedouble>::W;
  using Record = CEPMFieldSites::Record;
  using SiteKind = CEPMExtraction::SiteKind;
  auto& epm = config->GetEPMExtraction();
  if (!epm.field) epm.field = std::make_shared<CEPMFieldSites>();
  auto& field = *epm.field;
  const auto saved_iter = config->GetInnerIter();
  const passivedouble baseline = SU2_TYPE::GetValue(ObjFunc);
  std::vector<passivedouble> baseline_outputs[2], adjoints[2];
  const unsigned short direct_blocks[] = {FLOW_SOL, TURB_SOL};
  const unsigned short adjoint_blocks[] = {ADJFLOW_SOL, ADJTURB_SOL};
  for (unsigned short block = 0; block < 2; ++block) {
    for (unsigned long point = 0; point < geometry->GetnPoint(); ++point)
      for (unsigned short var = 0; var < solver[direct_blocks[block]]->GetnVar(); ++var) {
        baseline_outputs[block].push_back(SU2_TYPE::GetValue(solver[direct_blocks[block]]->GetNodes()->GetSolution(point,var)));
        adjoints[block].push_back(SU2_TYPE::GetValue(solver[adjoint_blocks[block]]->GetNodes()->GetSolution(point,var)));
      }
  }

  /*--- Discovery pass: the passive CLEAR_INDICES iteration creates one site per hook key. ---*/
  field.Clear();
  epm.active = true;
  config->SetInnerIter(saved_iter);
  SetRecording(RECORDING::CLEAR_INDICES);

  /*--- No new sites from here on. The hooks fill the state of a site on its first use, so drop the state of the
   *    discovery pass, and its counts, so that records and counters describe the real recording only. ---*/
  field.Freeze();
  field.ResetRecords();
  epm.evaluations = epm.ambiguous = epm.stress_evaluations = 0;

  /*--- Real recording; SetRecording registers the zero-valued field inputs. ---*/
  config->SetInnerIter(saved_iter);
  SetRecording(MainVariables);
  const auto recorded_objective = SU2_TYPE::GetValue(ObjFunc);
  if (!std::isfinite(recorded_objective) || fabs(recorded_objective - baseline) > 1e-10 * max(1.0, fabs(baseline)))
    SU2_MPI::Error("EPM zero-amplitude replay changed the baseline objective.", CURRENT_FUNCTION);
  for (unsigned short block = 0; block < 2; ++block) {
    size_t entry = 0;
    for (unsigned long point = 0; point < geometry->GetnPoint(); ++point)
      for (unsigned short var = 0; var < solver[direct_blocks[block]]->GetnVar(); ++var, ++entry) {
        const auto value = SU2_TYPE::GetValue(solver[direct_blocks[block]]->GetNodes()->GetSolution(point,var));
        if (!std::isfinite(value) || fabs(value - baseline_outputs[block][entry]) >
            1e-12 * max(1.0, fabs(baseline_outputs[block][entry])))
          SU2_MPI::Error("EPM replay changed the baseline iteration output.", CURRENT_FUNCTION);
        if (SU2_TYPE::GetValue(solver[adjoint_blocks[block]]->GetNodes()->GetSolution(point,var)) != adjoints[block][entry])
          SU2_MPI::Error("EPM replay changed the saved coupled adjoint.", CURRENT_FUNCTION);
      }
  }

  /*--- One reverse sweep seeded with the saved coupled adjoint gives the sensitivity of every site. ---*/
  iteration->InitializeAdjoint(solver_container, geometry_container, config_container, ZONE_0, INST_0);
  SetAdjObjFunction();
  AD::ComputeAdjoint();
  field.ExtractAdjoints();
  AD::ClearAdjoints();
  epm.active = false;

  /*--- Stress corrections are symmetric, so only sym(S_tau) is meaningful; the dot products are unchanged. ---*/
  for (size_t site = 0; site < field.Size(); ++site) {
    auto* S = field.Data(site).S_tau;
    for (unsigned short i = 0; i < nDim; ++i)
      for (unsigned short j = 0; j < i; ++j) S[i*3+j] = S[j*3+i] = 0.5 * (S[i*3+j] + S[j*3+i]);
  }

  /*--- Coordinates of the points of each site; keys as in the solver loops (Site() returns npos for no site). ---*/
  const auto set_coord = [&](passivedouble (&coord)[3], unsigned long iPoint) {
    for (unsigned short iDim = 0; iDim < 3; ++iDim)
      coord[iDim] = iDim < nDim ? SU2_TYPE::GetValue(geometry->nodes->GetCoord(iPoint, iDim)) : 0.0;
  };
  for (unsigned long iEdge = 0; iEdge < geometry->GetnEdge(); ++iEdge) {
    auto iPoint = geometry->edges->GetNode(iEdge, 0), jPoint = geometry->edges->GetNode(iEdge, 1);
    if (geometry->nodes->GetGlobalIndex(jPoint) < geometry->nodes->GetGlobalIndex(iPoint)) std::swap(iPoint, jPoint);
    const auto site = field.Site(SiteKind::EDGE, geometry->nodes->GetGlobalIndex(iPoint),
                                 geometry->nodes->GetGlobalIndex(jPoint));
    if (site == CEPMFieldSites::npos) continue;
    set_coord(field.Data(site).coord_a, iPoint);  // key_a is the smaller global id
    set_coord(field.Data(site).coord_b, jPoint);
  }
  for (unsigned short iMarker = 0; iMarker < config->GetnMarker_All(); ++iMarker) {
    const auto marker = config->GetMarker_CfgFile_TagBound(config->GetMarker_All_TagBound(iMarker));
    for (unsigned long iVertex = 0; iVertex < geometry->GetnVertex(iMarker); ++iVertex) {
      const auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
      const auto site = field.Site(SiteKind::BOUNDARY, geometry->nodes->GetGlobalIndex(iPoint), marker);
      if (site != CEPMFieldSites::npos) set_coord(field.Data(site).coord_a, iPoint);
    }
  }
  for (unsigned long iPoint = 0; iPoint < geometry->GetnPoint(); ++iPoint) {
    const auto site = field.Site(SiteKind::NODE, geometry->nodes->GetGlobalIndex(iPoint), 0);
    if (site != CEPMFieldSites::npos) set_coord(field.Data(site).coord_a, iPoint);
  }

  /*--- Contribution S . delta of one site to dJ/da of one case and probe. The canonical case (probe 0) uses the
   *    corrections stored by the hooks; the probes recompute them from the stored passive state. The case is
   *    recomputed in both, for its ambiguity flag. ---*/
  const auto site_gradient = [&](const Record& record, unsigned short mode, unsigned short probe, bool& ambiguous) {
    const unsigned short component = mode / 2 + 1;
    const bool permute = mode % 2;
    const double angle = probe == 0 ? 0.0 : PI_NUMBER / 2;
    const unsigned short probe_pair = probe == 0 ? 3 : probe - 1;
    passivedouble grad[3][3];
    for (unsigned short i = 0; i < 3; ++i)
      for (unsigned short j = 0; j < 3; ++j) grad[i][j] = record.grad[i*3+j];
    passivedouble gradient = 0.0;

    if (static_cast<SiteKind>(static_cast<int>(record.kind)) != SiteKind::NODE) {
      passivedouble correction[3][3] = {};
      if (probe == 0) {
        CNumerics::EPMCaseTarget(nDim, grad, record.rho, record.mu_t, record.k, component, permute, angle,
                                 probe_pair, correction, ambiguous);
        for (unsigned short i = 0; i < nDim; ++i)
          for (unsigned short j = 0; j < nDim; ++j) correction[i][j] = record.dtau[mode][i*3+j];
      } else {
        CNumerics::EPMCaseStressCorrection(nDim, grad, record.rho, record.mu_t, record.k, component, permute,
                                           angle, probe_pair, correction, ambiguous);
      }
      for (unsigned short i = 0; i < nDim; ++i)
        for (unsigned short j = 0; j < nDim; ++j) gradient += record.S_tau[i*3+j] * correction[i][j];
      return gradient;
    }

    passivedouble target[3][3];
    CNumerics::EPMCaseTarget(nDim, grad, record.rho, record.mu_t, record.k, component, permute, angle,
                             probe_pair, target, ambiguous);
    passivedouble dpk = record.dpk[mode], dpw = record.dpw[mode];
    if (probe != 0) {
      /*--- Same clip and omega production as the scalar replay and the source hook. ---*/
      const auto raw = CNumerics::ComputeReynoldsStressProduction(nDim, target, grad, record.rho);
      const auto epm_pk = max(0.0, min(raw, record.prod_limit));
      const auto epm_pw = record.mu_t > 0 ? record.alfa_blended * record.rho / record.mu_t * epm_pk : 0.0;
      dpk = epm_pk - record.pk0;
      dpw = epm_pw - record.pw0;
    }
    return record.S_pk * dpk + record.S_pw * dpw;
  };

  const char* names[] = {"1C_KEEP", "1C_SWAP", "2C_KEEP", "2C_SWAP", "3C"};
  std::ostringstream rows;
  rows << std::setprecision(17);
  rows << "case,objective,dimension,mpi_ranks,probe_pair,probe_angle,J_baseline,dJ_da_at_zero,J_linear_at_one,"
          "ambiguous_evaluations,total_evaluations,stress_evaluations,source_evaluations,"
          "gap_tolerance,delta_b,tensor_blend_factor,"
          "primal_converged,adjoint_converged,status,extraction\n";
  passivedouble lower = baseline, upper = baseline;

  for (unsigned short mode = 0; mode < 5; ++mode) {
    epm.component = mode / 2 + 1;
    epm.permute = mode % 2;
    for (unsigned short probe = 0; probe < 4; ++probe) {
      epm.angle = probe == 0 ? 0.0 : PI_NUMBER / 2;
      epm.probe_pair = probe == 0 ? 3 : probe - 1;
      passivedouble local_gradient = 0.0;
      unsigned long local_ambiguous = 0;
      for (size_t site = 0; site < field.Size(); ++site) {
        const auto& record = field.Data(site);
        if (record.uses == 0) continue;  // not evaluated in the recording: no state and no sensitivity
        bool ambiguous = false;
        local_gradient += site_gradient(record, mode, probe, ambiguous);
        /*--- The scalar replay counts every evaluation; a site stands for its uses at the same state. ---*/
        if (ambiguous) local_ambiguous += static_cast<unsigned long>(record.uses);
      }
      passivedouble gradient = 0;
      PassiveMPI::Allreduce(&local_gradient, &gradient, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      const unsigned long local_counts[] = {local_ambiguous, epm.evaluations, epm.stress_evaluations};
      unsigned long global_counts[3] = {};
      PassiveMPI::Allreduce(local_counts, global_counts, 3, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
      if (!std::isfinite(gradient)) SU2_MPI::Error("Nonfinite EPM sensitivity.", CURRENT_FUNCTION);
      rows << names[mode] << ',' << (config->GetKind_ObjFunc() == DRAG_COEFFICIENT ? "DRAG" : "LIFT") << ','
           << nDim << ',' << SU2_MPI::GetSize() << ','
           << epm.probe_pair << ',' << epm.angle << ',' << baseline << ',' << gradient << ',' << baseline + gradient << ','
           << global_counts[0] << ',' << global_counts[1] << ',' << global_counts[2] << ','
           << global_counts[1] - global_counts[2] << ',' << CEPMExtraction::gap_tolerance
           << ",1,1,unverified,true,frozen_forcing_unverified,field\n";
      lower = min(lower, baseline + gradient);
      upper = max(upper, baseline + gradient);
      // Every rank must perform the same reductions in the same order.
      if (global_counts[0] == 0) break;
    }
  }

  // Clear tape indices, then restore all direct blocks and rebuild their dependencies.
  SetRecording(RECORDING::CLEAR_INDICES);
  for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol)
    if (solver[iSol] && solver[iSol]->GetAdjoint()) solver[iSol]->SetRecording(geometry, config);
  config->SetInnerIter(saved_iter);
  iteration->SetDependencies(solver_container, geometry_container, numerics_container, config_container,
                             ZONE_0, INST_0, MainVariables);
  if (rank == MASTER_NODE) {
    std::ofstream output("sst_epm_sensitivities.csv");
    output << rows.str();
    output.close();
    if (!output) SU2_MPI::Error("Could not write sst_epm_sensitivities.csv.", CURRENT_FUNCTION);
    cout << "\nClassical EPM first-order frozen-forcing predictions: [" << lower << ", " << upper
         << "]. Includes baseline and sampled orientations; not verified nonlinear bounds.\n";
  }

  /*--- TODO(Task 7): write the per-rank field files and the JSON layout here, e.g.
   *    WriteEPMField(field, *config, *geometry, objective, extras_json); the records hold the recorded state,
   *    the symmetrized sensitivities, the canonical corrections and the coordinates. The value of
   *    "extract_only_residuals" is epm_extract_only_residuals_json (empty, i.e. null, unless extract-only). ---*/
}
