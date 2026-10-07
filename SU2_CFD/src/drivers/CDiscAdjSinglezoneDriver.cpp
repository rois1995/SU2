/*!
 * \file driver_adjoint_singlezone.cpp
 * \brief The main subroutines for driving adjoint single-zone problems.
 * \author R. Sanchez
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

#include "../../include/drivers/CDiscAdjSinglezoneDriver.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO.hpp"
#include "../../include/output/tools/CWindowingTools.hpp"
#include "../../include/output/COutputFactory.hpp"
#include "../../include/output/COutput.hpp"
#include "../../include/iteration/CIterationFactory.hpp"
#include "../../include/iteration/CTurboIteration.hpp"
#include "../../include/solvers/CDiscAdjSolver.hpp"
#include "../../../Common/include/toolboxes/CQuasiNewtonInvLeastSquares.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/toolboxes/printing_toolbox.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../include/adaptation/CAdjointTransfer.hpp"
#include "../../include/adaptation/CBarycentricTransfer.hpp"
#include "../../include/adaptation/CConservativeTransfer.hpp"

#include <fstream>
#include <limits>
#include <memory>
#include <string>

namespace {

/*--- Current resident memory of this process (MB, /proc/self/status VmRSS; 0 if unknown). ---*/
passivedouble CurrentRSS() {
  std::ifstream status("/proc/self/status");
  std::string line;
  while (std::getline(status, line)) {
    if (line.compare(0, 6, "VmRSS:") == 0) return std::stod(line.substr(6)) / 1024.0;
  }
  return 0.0;
}

/*--- Largest and last value of each convergence field over the iterations of a phase. ---*/
void TrackResiduals(const vector<pair<string, passivedouble>>& fields, vector<pair<string, passivedouble>>& largest,
                    vector<pair<string, passivedouble>>& last) {
  if (largest.size() != fields.size()) largest = fields;
  for (auto i = 0ul; i < fields.size(); ++i) largest[i].second = std::max(largest[i].second, fields[i].second);
  last = fields;
}

/*--- Smallest drop (orders of magnitude) over the convergence fields: largest minus last value. ---*/
passivedouble Reduction(const vector<pair<string, passivedouble>>& largest,
                        const vector<pair<string, passivedouble>>& last) {
  passivedouble drop = std::numeric_limits<passivedouble>::max();
  for (auto i = 0ul; i < last.size() && i < largest.size(); ++i) drop = std::min(drop, largest[i].second - last[i].second);
  return last.empty() ? 0.0 : drop;
}

}  // namespace


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
  directStateSet = true;

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

  if (goalLoop) {
    goalAdjointIters = 0;
    goalAdjointMax.clear();
    goalAdjointLast.clear();
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

    /*--- The goal-oriented loop writes the files of a cycle itself (after the capture and the metric). ---*/

    if (goalLoop) {
      goalAdjointIters = Adjoint_Iter + 1;
      TrackResiduals(output_container[ZONE_0]->GetResidualConvFields(), goalAdjointMax, goalAdjointLast);
    }

    /*--- Output files for steady state simulations. ---*/

    if (!config->GetTime_Domain() && !goalLoop) {
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

}

void CDiscAdjSinglezoneDriver::Postprocess() {
  SU2_ZONE_SCOPED

  switch(config->GetKind_Solver())
  {
    case MAIN_SOLVER::DISC_ADJ_EULER :     case MAIN_SOLVER::DISC_ADJ_NAVIER_STOKES :     case MAIN_SOLVER::DISC_ADJ_RANS :
    case MAIN_SOLVER::DISC_ADJ_INC_EULER : case MAIN_SOLVER::DISC_ADJ_INC_NAVIER_STOKES : case MAIN_SOLVER::DISC_ADJ_INC_RANS :
    case MAIN_SOLVER::DISC_ADJ_HEAT :

      /*--- Residual adjoint (stage G): one extra sweep of the main recording before it is replaced. ---*/
      if (config->GetAdap_Adj_Lambda()) CaptureResidualAdjoint();

      /*--- Compute the geometrical sensitivities ---*/
      SecondaryRecording();

      /*--- Developer check of the residual adjoint, after the last use of the tapes. ---*/
      if (config->GetAdap_Adj_Lambda_Check()) CheckResidualAdjoint();

      /*--- Goal-oriented metric of the current mesh (stage G1b), written by the final output. ---*/
      if (config->GetGoal_Oriented_Metric()) ComputeGoalMetric();
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

void CDiscAdjSinglezoneDriver::CaptureResidualAdjoint() {
  SU2_ZONE_SCOPED

#ifndef CODI_REVERSE_TYPE
  SU2_MPI::Error("ADAP_ADJ_LAMBDA= YES needs a reverse-mode AD build (SU2_CFD_AD).", CURRENT_FUNCTION);
#else
  if (omp_get_max_threads() > 1) {
    SU2_MPI::Error("ADAP_ADJ_LAMBDA= YES does not support OpenMP threads (run with one thread).", CURRENT_FUNCTION);
  }
  if (RecordingState != MainVariables || TrivialFunction()) {
    SU2_MPI::Error("ADAP_ADJ_LAMBDA= YES needs the main recording of the flow iteration.", CURRENT_FUNCTION);
  }

  auto* flow = solver[FLOW_SOL];
  auto* adjoint = dynamic_cast<CDiscAdjSolver*>(solver[ADJFLOW_SOL]);
  if (adjoint == nullptr) SU2_MPI::Error("No discrete adjoint flow solver.", CURRENT_FUNCTION);
  const auto nPoint = geometry->GetnPoint();
  const auto nPointDomain = geometry->GetnPointDomain();
  const auto nVar = flow->GetnVar();

  /*--- Keep U_out (the state on which the objective of the main recording was evaluated) for the check. ---*/
  if (config->GetAdap_Adj_Lambda_Check()) {
    LambdaCheck_SolutionOut.resize(nPoint, nVar);
    for (auto iPoint = 0ul; iPoint < nPoint; iPoint++)
      for (auto iVar = 0u; iVar < nVar; iVar++)
        LambdaCheck_SolutionOut(iPoint, iVar) = SU2_TYPE::GetValue(flow->GetNodes()->GetSolution(iPoint, iVar));
  }

  /*--- One reverse sweep seeded as in Run, without extracting the adjoint solution. ---*/
  flow->System.StartRhsAdjointCapture();

  iteration->InitializeAdjoint(solver_container, geometry_container, config_container, ZONE_0, INST_0);
  SetAdjObjFunction();
  AD::ComputeAdjoint();

  AD::BeginUseAdjoints();
  adjoint->GetSweepFreeStreamDerivatives(LambdaSweep_SensAoA, LambdaSweep_SensMach);
  AD::EndUseAdjoints();

  AD::ClearAdjoints();
  flow->System.StopRhsAdjointCapture();

  /*--- Exactly one flow linear solve must be on the tape, on every rank. ---*/
  const unsigned long nLocal = flow->System.GetnRhsAdjointCaptures();
  unsigned long nMin = 0, nMax = 0;
  SU2_MPI::Allreduce(&nLocal, &nMin, 1, MPI_UNSIGNED_LONG, MPI_MIN, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&nLocal, &nMax, 1, MPI_UNSIGNED_LONG, MPI_MAX, SU2_MPI::GetComm());
  if (nMin != 1 || nMax != 1) {
    SU2_MPI::Error("ADAP_ADJ_LAMBDA: the main recording holds " + to_string(nMax) +
                   " flow linear solves (exactly 1 is needed; check MGLEVEL= 0 and one RK step).", CURRENT_FUNCTION);
  }
  const auto nonFinite = adjoint->SetResidualAdjoint(geometry, config, flow->System.GetRhsAdjoint());
  if (nonFinite > 0) {
    SU2_MPI::Error("ADAP_ADJ_LAMBDA: the residual adjoint has " + to_string(nonFinite) + " non-finite values.",
                   CURRENT_FUNCTION);
  }

  /*--- Report: relaxation of the recorded update, norms of lambda and psi (global, owned points). ---*/
  passivedouble minRelaxLocal = 1e300;
  vector<passivedouble> local(3 * nVar + 1, 0.0), global(3 * nVar + 1, 0.0);
  for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
    minRelaxLocal = min(minRelaxLocal, SU2_TYPE::GetValue(flow->GetNodes()->GetUnderRelaxation(iPoint)));
    for (auto iVar = 0u; iVar < nVar; iVar++) {
      const passivedouble lam = adjoint->GetResidualAdjoint(iPoint, iVar);
      const passivedouble psi = SU2_TYPE::GetValue(adjoint->GetNodes()->GetSolution(iPoint, iVar));
      local[iVar] += lam * lam;
      local[nVar + iVar] += psi * psi;
      local[2 * nVar + iVar] += (lam - psi) * (lam - psi);
    }
  }
  passivedouble minRelax = 0.0;
  SelectMPIWrapper<passivedouble>::W::Allreduce(&minRelaxLocal, &minRelax, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
  SelectMPIWrapper<passivedouble>::W::Allreduce(local.data(), global.data(), 3 * nVar, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());

  if (rank == MASTER_NODE) {
    passivedouble sumLam = 0.0, sumDiff = 0.0;
    cout << "\n-------------------------------------------------------------------------\n";
    cout << "Residual adjoint (lambda = adjoint of the RHS of the flow solve; adjoint of R = -lambda).\n";
    cout << "Reverse linear solve: " << flow->System.GetIterations() << " iterations, residual "
         << flow->System.GetResidual() << ". Min. under-relaxation of the recorded update: " << minRelax << ".\n";
    cout << scientific << setprecision(6);
    for (auto iVar = 0u; iVar < nVar; iVar++) {
      cout << "  var " << iVar << ": ||lambda|| " << sqrt(global[iVar]) << ", ||psi|| " << sqrt(global[nVar + iVar])
           << ", ||lambda - psi|| " << sqrt(global[2 * nVar + iVar]) << "\n";
      sumLam += global[iVar];
      sumDiff += global[2 * nVar + iVar];
    }
    cout << "  ||lambda - psi|| / ||lambda|| = " << sqrt(sumDiff / max(sumLam, 1e-300)) << "\n";
    cout << setprecision(16) << "  Sweep d/dAlpha (rad) " << LambdaSweep_SensAoA << ", d/dMach "
         << LambdaSweep_SensMach << "\n";
    cout << "-------------------------------------------------------------------------\n" << endl;
    cout.unsetf(ios_base::floatfield);
    cout << setprecision(6);
  }
#endif
}

void CDiscAdjSinglezoneDriver::CheckResidualAdjoint() {
  SU2_ZONE_SCOPED

  auto* flow = solver[FLOW_SOL];
  auto* adjoint = dynamic_cast<CDiscAdjSolver*>(solver[ADJFLOW_SOL]);
  const auto nPoint = geometry->GetnPoint();
  const auto nPointDomain = geometry->GetnPointDomain();
  const auto nVar = flow->GetnVar();
  if (adjoint == nullptr || LambdaCheck_SolutionOut.rows() != nPoint) {
    SU2_MPI::Error("ADAP_ADJ_LAMBDA_CHECK needs the residual adjoint capture.", CURRENT_FUNCTION);
  }
  auto* flowNodes = flow->GetNodes();

  /*--- Save what the check changes: flow solution and the free-stream velocity elements. ---*/
  vector<su2double> savedSolution(nPoint * nVar);
  for (auto iPoint = 0ul; iPoint < nPoint; iPoint++)
    for (auto iVar = 0u; iVar < nVar; iVar++) savedSolution[iPoint * nVar + iVar] = flowNodes->GetSolution(iPoint, iVar);
  su2double* velocity = config->GetVelocity_FreeStreamND();
  su2double savedVelocity[3] = {0.0, 0.0, 0.0};
  for (auto iDim = 0u; iDim < nDim; iDim++) savedVelocity[iDim] = velocity[iDim];

  /*--- Parameters as registered in CDiscAdjSolver::RegisterVariables: |V| proportional to Mach, direction from
   *    Alpha and Beta; everything else (config AoA/Mach, reference values, pressure, temperature) fixed. ---*/
  const passivedouble alpha0 = SU2_TYPE::GetValue(config->GetAoA()) * PI_NUMBER / 180.0;
  const passivedouble beta0 = SU2_TYPE::GetValue(config->GetAoS()) * PI_NUMBER / 180.0;
  const passivedouble mach0 = SU2_TYPE::GetValue(config->GetMach());
  passivedouble vmod0 = 0.0;
  for (auto iDim = 0u; iDim < nDim; iDim++) vmod0 += pow(SU2_TYPE::GetValue(savedVelocity[iDim]), 2);
  vmod0 = sqrt(vmod0);

  auto setFreeStream = [&](passivedouble alpha, passivedouble mach) {
    const passivedouble scale = vmod0 * mach / mach0;
    if (nDim == 2) {
      velocity[0] = cos(alpha) * scale;
      velocity[1] = sin(alpha) * scale;
    } else {
      velocity[0] = cos(alpha) * cos(beta0) * scale;
      velocity[1] = sin(beta0) * scale;
      velocity[2] = sin(alpha) * cos(beta0) * scale;
    }
  };

  /*--- Residual of U_in (owned points with dt != 0, as in the recorded right-hand side) and objective of U_out. ---*/
  vector<passivedouble> residual(nPointDomain * nVar);
  auto evaluate = [&](passivedouble alpha, passivedouble mach) {
    setFreeStream(alpha, mach);

    for (auto iPoint = 0ul; iPoint < nPoint; iPoint++) flowNodes->SetSolution(iPoint, adjoint->GetNodes()->GetSolution_Direct(iPoint));
    flow->Preprocessing(geometry, solver, config, MESH_0, 0, RUNTIME_FLOW_SYS, false);
    flow->SetTime_Step(geometry, solver, config, MESH_0, config->GetTimeIter());
    integration[FLOW_SOL]->ComputeResidual(geometry, solver, numerics[FLOW_SOL], config, MESH_0, RUNTIME_FLOW_SYS);
    for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
      const bool active = flowNodes->GetDelta_Time(iPoint) != 0.0;
      for (auto iVar = 0u; iVar < nVar; iVar++)
        residual[iPoint * nVar + iVar] = active ? SU2_TYPE::GetValue(flow->LinSysRes(iPoint, iVar)) : 0.0;
    }

    for (auto iPoint = 0ul; iPoint < nPoint; iPoint++)
      for (auto iVar = 0u; iVar < nVar; iVar++)
        flowNodes->SetSolution(iPoint, iVar, LambdaCheck_SolutionOut(iPoint, iVar));
    flow->Preprocessing(geometry, solver, config, MESH_0, 0, RUNTIME_FLOW_SYS, true);
    flow->Pressure_Forces(geometry, config);
    flow->Momentum_Forces(geometry, config);
    flow->Friction_Forces(geometry, config);
    flow->Evaluate_ObjFunc(config, solver);
    return SU2_TYPE::GetValue(flow->GetTotal_ComboObj());
  };

  if (rank == MASTER_NODE) {
    cout << "\n-------------------------------------------------------------------------\n";
    cout << "Residual adjoint check: tape = d/dp of the capture sweep; identity = dJ/dp(U_out)|explicit\n"
         << "  - sum_owned lambda . dR/dp(U_in), central differences with step h (Mach: h * Mach).\n";
    cout << "  p      h          tape                    identity                explicit           rel. diff\n";
  }

  const passivedouble steps[] = {1e-3, 1e-4, 1e-5, 1e-6, 1e-7, 1e-8};
  for (int iParam = 0; iParam < 2; iParam++) {
    const passivedouble tape = (iParam == 0) ? LambdaSweep_SensAoA : LambdaSweep_SensMach;
    for (const auto h : steps) {
      const passivedouble dp = (iParam == 0) ? h : h * mach0;
      vector<passivedouble> resPlus, resMinus;
      const passivedouble jPlus = (iParam == 0) ? evaluate(alpha0 + dp, mach0) : evaluate(alpha0, mach0 + dp);
      resPlus = residual;
      const passivedouble jMinus = (iParam == 0) ? evaluate(alpha0 - dp, mach0) : evaluate(alpha0, mach0 - dp);
      resMinus = residual;

      passivedouble contractionLocal = 0.0, contraction = 0.0;
      for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++)
        for (auto iVar = 0u; iVar < nVar; iVar++)
          contractionLocal += adjoint->GetResidualAdjoint(iPoint, iVar) *
                              (resPlus[iPoint * nVar + iVar] - resMinus[iPoint * nVar + iVar]) / (2.0 * dp);
      SelectMPIWrapper<passivedouble>::W::Allreduce(&contractionLocal, &contraction, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());

      const passivedouble explicitTerm = (jPlus - jMinus) / (2.0 * dp);
      const passivedouble identity = explicitTerm - contraction;
      if (rank == MASTER_NODE) {
        cout << "  " << (iParam == 0 ? "Alpha" : "Mach ") << "  " << scientific << setprecision(1) << h << "  "
             << setprecision(16) << tape << "  " << identity << "  " << setprecision(6) << explicitTerm << "  "
             << setprecision(3) << fabs(tape - identity) / max(fabs(tape), 1e-300) << "\n";
      }
    }
  }
  if (rank == MASTER_NODE) {
    cout << "-------------------------------------------------------------------------\n" << endl;
    cout.unsetf(ios_base::floatfield);
    cout << setprecision(6);
  }

  /*--- Restore. ---*/
  for (auto iDim = 0u; iDim < nDim; iDim++) velocity[iDim] = savedVelocity[iDim];
  for (auto iPoint = 0ul; iPoint < nPoint; iPoint++)
    for (auto iVar = 0u; iVar < nVar; iVar++) flowNodes->SetSolution(iPoint, iVar, savedSolution[iPoint * nVar + iVar]);
}

void CDiscAdjSinglezoneDriver::ComputeGoalMetric() {
  SU2_ZONE_SCOPED

  if (AD::TapeActive()) SU2_MPI::Error("The goal-oriented metric needs the tape to be inactive.", CURRENT_FUNCTION);
  auto* flow = solver[FLOW_SOL];
  auto* adjoint = solver[ADJFLOW_SOL];
  if (flow == nullptr || adjoint == nullptr) SU2_MPI::Error("No flow or adjoint flow solver.", CURRENT_FUNCTION);

  if (rank == MASTER_NODE)
    cout << endl << "-------------------------- Compute Goal Metric --------------------------" << endl;
  const auto startTime = SU2_MPI::Wtime();

  /*--- State used (the differentiated primal, Solution_Direct) against the flow solution in memory (U_out of the
   *    last recording). ---*/
  const auto nPointDomain = geometry->GetnPointDomain();
  const auto nVar = flow->GetnVar();
  passivedouble local[2] = {0.0, 0.0}, global[2] = {0.0, 0.0};
  for (auto iPoint = 0ul; iPoint < nPointDomain; iPoint++) {
    const su2double* U = adjoint->GetNodes()->GetSolution_Direct(iPoint);
    for (auto iVar = 0u; iVar < nVar; iVar++) {
      local[0] = max(local[0], fabs(SU2_TYPE::GetValue(U[iVar] - flow->GetNodes()->GetSolution(iPoint, iVar))));
      local[1] = max(local[1], fabs(SU2_TYPE::GetValue(U[iVar])));
    }
  }
  SelectMPIWrapper<passivedouble>::W::Allreduce(local, global, 2, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  if (rank == MASTER_NODE) {
    cout << "Primal state of the estimate: Solution_Direct; max |Solution_Direct - U_out| / max |Solution_Direct| = "
         << scientific << setprecision(3) << global[0] / max(global[1], 1e-300) << "." << endl;
    cout.unsetf(ios_base::floatfield);
    cout << setprecision(6);
  }

  vector<su2double> state;
  flow->SetGoalFields_Adapt(geometry, config, adjoint, state);
  flow->ComputeGoalHessian(geometry, config, &state);

  /*--- ADAP_BL_METHOD= TWO_PASS: the remesher builds the boundary-layer metric itself. ---*/
  const bool boundaryLayer = config->GetKind_Adap_BL_Method() != ADAP_BL_METHOD::TWO_PASS;
  if (config->GetKind_Adap_Remesher() == ADAP_REMESHER::NATIVE_CAVITY &&
      config->GetnAdap_BL() && (!nativeReference || !nativeReference->original)) {
    auto remesher = MakeRemesher();
  }
  flow->ComputeMetric(geometry, config, nullptr, boundaryLayer, nativeReference.get());
  if (rank == MASTER_NODE) cout << "Goal metric computed in " << SU2_MPI::Wtime() - startTime << " s." << endl;
}

void CDiscAdjSinglezoneDriver::StartSolver() {
  SU2_ZONE_SCOPED

  /*--- ADAP_LOOP with a discrete adjoint: the config allows it only with ADAP_SENSOR= GOAL. ---*/
  if (config_container[ZONE_0]->GetAdap_Loop()) {
    RunGoalAdaptationLoop();
    return;
  }
  CSinglezoneDriver::StartSolver();
}

void CDiscAdjSinglezoneDriver::RefreshMeshPointers() {
  config = config_container[ZONE_0];
  iteration = iteration_container[ZONE_0][INST_0];
  solver = solver_container[ZONE_0][INST_0][MESH_0];
  numerics = numerics_container[ZONE_0][INST_0][MESH_0];
  geometry = geometry_container[ZONE_0][INST_0][MESH_0];
  integration = integration_container[ZONE_0][INST_0];

  /*--- Nothing is recorded on the new mesh; the direct output starts again (the adjoint output was reset by
   *    ReplaceMesh). ---*/
  RecordingState = RECORDING::CLEAR_INDICES;
  directStateSet = false;
  direct_output->ResetMeshDependentData();
  direct_output->ResetConvergenceMonitoring(0);
  direct_output->SetConvergence(false);
  StopCalc = false;
  TimeIter = 0;
  config->SetTimeIter(0);
  config->SetOuterIter(0);
  config->SetInnerIter(0);
}

void CDiscAdjSinglezoneDriver::SwapMesh(const CSimplexMesh& mesh, CSolutionTransfer& transfer) {
  CRemeshResult remeshed;
  remeshed.slices = CReaderSlices::FromComplete(mesh, MASTER_NODE);
  remeshed.markers = remeshed.slices.markersWithElements;
  SwapMesh(remeshed, transfer);
}

void CDiscAdjSinglezoneDriver::SwapMesh(const CRemeshResult& remeshed, CSolutionTransfer& transfer) {
  SU2_ZONE_SCOPED

  if (AD::TapeActive()) SU2_MPI::Error("The mesh of a discrete adjoint problem is replaced with the tape inactive.",
                                       CURRENT_FUNCTION);
  auto* flow = solver[FLOW_SOL];
  auto* adjoint = solver[ADJFLOW_SOL];
  if (flow == nullptr || adjoint == nullptr) SU2_MPI::Error("No flow or adjoint flow solver.", CURRENT_FUNCTION);

  /*--- The primal state the adjoint belongs to: Solution_Direct (U_in of the recordings); after a recording the flow
   *    solution holds U_out, one direct iteration further. ---*/
  if (directStateSet) {
    for (auto iPoint = 0ul; iPoint < geometry->GetnPoint(); iPoint++)
      flow->GetNodes()->SetSolution(iPoint, adjoint->GetNodes()->GetSolution_Direct(iPoint));
  }

  /*--- The tape (with the user data of its external functions) refers to the objects of this mesh. ---*/
  AD::Reset();
  RecordingState = RECORDING::CLEAR_INDICES;

  ReplaceMesh(remeshed, transfer);

  RefreshMeshPointers();

  /*--- The transferred fields include owner copies on halos for interpolation/inspection. A discrete-adjoint
   *    iteration seeds all recorded primal outputs, whose halo communication is already on the tape. Seeding
   *    these copies again duplicates the owner's contribution. As with a fresh adjoint restart, only owned
   *    solution values are warm-start seeds; ghost inputs extract zero adjoints after reverse communication. ---*/
  for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
    auto* adjointSolver = solver[iSol];
    if (adjointSolver == nullptr || !adjointSolver->GetAdjoint()) continue;
    auto* nodes = adjointSolver->GetNodes();
    for (auto iPoint = geometry->GetnPointDomain(); iPoint < geometry->GetnPoint(); ++iPoint)
      for (unsigned short iVar = 0; iVar < adjointSolver->GetnVar(); ++iVar)
        nodes->SetSolution(iPoint, iVar, 0.0);
    nodes->Set_OldSolution();
  }
}

CDiscAdjSinglezoneDriver::PrimalPhase CDiscAdjSinglezoneDriver::RunPrimalPhase(unsigned long nIter) {
  SU2_ZONE_SCOPED

  if (AD::TapeActive()) SU2_MPI::Error("The primal phase runs with the tape inactive.", CURRENT_FUNCTION);

  PrimalPhase phase;
  const auto startTime = SU2_MPI::Wtime();

  /*--- As SU2_CFD: CSinglezoneDriver::Preprocess of time step 0 (the steady initial condition is the solution in
   *    memory), then CFluidIteration::Solve. ---*/
  TimeIter = 0;
  config->SetTimeIter(0);
  config->SetOuterIter(0);
  config->SetPhysicalTime(0.0);
  config->SetnInner_Iter(nIter);
  solver[FLOW_SOL]->SetInitialCondition(geometry_container[ZONE_0][INST_0], solver_container[ZONE_0][INST_0], config, 0);
  direct_output->ResetConvergenceMonitoring(0);
  direct_output->SetConvergence(false);

  direct_iteration->StartTimer();
  direct_iteration->Preprocess(direct_output, integration_container, geometry_container, solver_container,
                               numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0,
                               INST_0);

  const auto screenFreq = max<unsigned long>(1, config->GetScreen_Wrt_Freq(2));
  for (unsigned long iter = 0; iter < nIter; iter++) {
    config->SetInnerIter(iter);
    direct_iteration->Iterate(direct_output, integration_container, geometry_container, solver_container,
                              numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0,
                              INST_0);
    direct_iteration->Postprocess(direct_output, integration_container, geometry_container, solver_container,
                                  numerics_container, config_container, surface_movement, grid_movement, FFDBox,
                                  ZONE_0, INST_0);
    const bool stop = direct_iteration->Monitor(direct_output, integration_container, geometry_container,
                                                solver_container, numerics_container, config_container,
                                                surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);
    phase.nIter = iter + 1;
    TrackResiduals(direct_output->GetResidualConvFields(), phase.residualMax, phase.residualLast);

    if (rank == MASTER_NODE && (iter % screenFreq == 0 || stop || iter + 1 == nIter)) {
      cout << "  primal iteration " << iter;
      for (const auto& field : phase.residualLast) cout << "  " << field.first << " " << field.second;
      cout << std::setprecision(16) << "  J " << SU2_TYPE::GetValue(solver[FLOW_SOL]->GetTotal_ComboObj())
           << std::setprecision(6) << endl;
    }
    if (stop) {
      phase.converged = !direct_output->GetConvergenceInterrupted();
      break;
    }
  }

  phase.J = SU2_TYPE::GetValue(solver[FLOW_SOL]->GetTotal_ComboObj());
  phase.CL = SU2_TYPE::GetValue(solver[FLOW_SOL]->GetTotal_CL());
  phase.CD = SU2_TYPE::GetValue(solver[FLOW_SOL]->GetTotal_CD());
  phase.time = SU2_MPI::Wtime() - startTime;
  return phase;
}

void CDiscAdjSinglezoneDriver::PreprocessGoalAdjoint() {
  SU2_ZONE_SCOPED

  /*--- A fresh DA recording starts at iteration 0. Recompute the MUSCL limiter even after LIMITER_ITER.
   *    MUSCL reconstruction and JST dissipation have no other iteration gates; RAMP_MUSCL is rejected.
   *    CFL adaptation is off for DA; MGLEVEL= 0 excludes Full-MG CFL ramps. BC_EVAL_FREQ is bypassed for DA
   *    (engine boundaries are rejected). Fixed-CL, actuator disks and outlet/motion ramps are rejected too. ---*/
  config->SetInnerIter(0);
  config->SetOuterIter(0);
  Preprocess(0);
}

void CDiscAdjSinglezoneDriver::RunGoalAdaptationLoop() {
  SU2_ZONE_SCOPED

  auto* output = output_container[ZONE_0];

  /*--- Stop before the first solve if the problem or the build cannot be adapted. ---*/
  CheckMeshAdaptation();
  if (!config->GetGoal_Oriented_Metric() || config->GetKind_Solver() != MAIN_SOLVER::DISC_ADJ_EULER) {
    SU2_MPI::Error("The adaptation loop of a discrete adjoint problem needs ADAP_SENSOR= (GOAL) and SOLVER= EULER.",
                   CURRENT_FUNCTION);
  }
  if (config->GetAdap_Mesh_Output()) CheckAdaptedMeshNames();

  auto remesher = MakeRemesher();
  std::unique_ptr<CSolutionTransfer> primalTransfer;
  switch (config->GetKind_Adap_Transfer()) {
    case ADAP_TRANSFER::BARYCENTRIC: primalTransfer = std::make_unique<CBarycentricTransfer>(); break;
    case ADAP_TRANSFER::CONSERVATIVE: primalTransfer = std::make_unique<CConservativeTransfer>(); break;
    case ADAP_TRANSFER::FREESTREAM: primalTransfer = std::make_unique<CFreeStreamTransfer>(); break;
  }
  CDiscAdjTransfer transfer(std::move(primalTransfer), config->GetAdap_Adj_WarmStart());

  goalLoop = true;
  StartTime = SU2_MPI::Wtime();
  config->Set_StartTime(StartTime);

  /*--- Convergence monitors: residual fields of CONV_FIELD that each output has, else its density residual. ---*/
  vector<string> convNames;
  for (unsigned short iField = 0; iField < config->GetnConv_Field(); iField++) convNames.push_back(config->GetConv_Field(iField));
  const auto primalFields = direct_output->SetResidualConvergenceFields(convNames, "RMS_DENSITY");
  const auto adjointFields = output->SetResidualConvergenceFields(convNames, "RMS_ADJ_DENSITY");

  /*--- Primal files of every cycle; the geometric sensitivities only in the files of the last cycle. ---*/
  direct_output->PreprocessVolumeOutput(config);
  output->SetVolumeOutputExclusions({"SENSITIVITY"});

  const unsigned long nIterRun = config->GetnInner_Iter();
  const auto nCycles = config->GetnAdap_Cycles();

  if (rank == MASTER_NODE) {
    cout << endl << "------------------- Goal-Oriented Mesh Adaptation Loop -------------------" << endl;
    cout << nCycles << " adaptations, objective " << config->GetObjFunc_Extension("") << ", adjoint "
         << (config->GetAdap_Adj_WarmStart() ? "warm" : "cold") << " start on the adapted meshes." << endl;
    cout << "Primal convergence fields:";
    for (const auto& name : primalFields) cout << " " << name;
    cout << ". Adjoint convergence fields:";
    for (const auto& name : adjointFields) cout << " " << name;
    cout << "." << endl;
  }

  struct CycleRow {
    unsigned long cycle = 0, requested = 0, nPoint = 0, primalIter = 0, adjointIter = 0;
    passivedouble preBL = 0.0, final = 0.0, J = 0.0, CL = 0.0, CD = 0.0;
    passivedouble primalRes = 0.0, primalDrop = 0.0, adjointRes = 0.0, adjointDrop = 0.0;
    bool primalConverged = false, adjointConverged = false, bracketed = false, warm = false;
    bool interrupted = false, sensitivities = false;
    passivedouble sensGeo = 0.0, sensAoA = 0.0, sensMach = 0.0;
    unsigned long goalRejected = 0, goalNonFinite = 0;
    passivedouble goalMinRatio = 0.0;
    passivedouble tPrimal = 0.0, tRecord = 0.0, tAdjoint = 0.0, tMetric = 0.0, tOutput = 0.0, tRemesh = 0.0;
    passivedouble rss = 0.0, rssMax = 0.0;
  };
  vector<CycleRow> rows;
  unsigned long primalOffset = 0, adjointOffset = 0;

  for (unsigned long iCycle = 0; iCycle <= nCycles; iCycle++) {
    const bool last = (iCycle == nCycles);

    /*--- The metric of this cycle makes the mesh of the next one (the last cycle keeps the last level). ---*/
    config->SetAdap_MetricLevel(config->GetAdap_CycleLevel(min(iCycle + 1, nCycles)));
    const CAdapLevel* level = (iCycle == 0) ? nullptr : &config->GetAdap_Level(config->GetAdap_CycleLevel(iCycle));
    output->SetAdaptationCycle(iCycle, adjointOffset);
    direct_output->SetAdaptationCycle(iCycle, primalOffset);

    CycleRow row;
    row.cycle = iCycle;
    row.requested = config->GetAdap_Complexity();
    row.nPoint = geometry->GetGlobal_nPointDomain();
    row.warm = iCycle > 0 && config->GetAdap_Adj_WarmStart();

    if (rank == MASTER_NODE) {
      cout << endl << "---------------------- Goal-Oriented Adaptation Cycle ----------------------" << endl;
      cout << "Cycle " << iCycle << " of " << nCycles << ": " << row.nPoint << " points, CFL " << config->GetCFL(MESH_0)
           << ", primal " << (iCycle == 0 ? "read from the solution file" : "transferred") << "." << endl;
      if (!last) cout << "Its metric makes the next mesh: complexity " << config->GetAdap_Complexity() << "." << endl;
    }

    /*--- 1. Primal phase (tape off), then its files: the state that is differentiated. ---*/
    const unsigned long nPrimal = level ? level->flowIter : nIterRun;
    direct_output->SetResidualReduction(level ? level->residualReduction : 0.0);
    if (rank == MASTER_NODE) cout << endl << "Primal phase: at most " << nPrimal << " iterations." << endl;
    const auto primal = RunPrimalPhase(nPrimal);
    primalOffset += primal.nIter;
    row.primalIter = primal.nIter;
    row.primalConverged = primal.converged;
    row.J = primal.J;
    row.CL = primal.CL;
    row.CD = primal.CD;
    row.primalRes = primal.residualLast.empty() ? 0.0 : primal.residualLast.front().second;
    row.primalDrop = Reduction(primal.residualMax, primal.residualLast);
    row.tPrimal = primal.time;
    if (rank == MASTER_NODE) {
      cout << "Primal phase: " << primal.nIter << " iterations (" << (primal.converged ? "converged" : "not converged")
           << ", residual drop " << row.primalDrop << " orders), " << std::setprecision(16) << "J " << primal.J
           << ", CL " << primal.CL << ", CD " << primal.CD << std::setprecision(6) << ", " << primal.time << " s."
           << endl;
    }
    auto tOutput = SU2_MPI::Wtime();
    direct_output->SetResultFiles(geometry, config, solver, primal.nIter - 1, true);
    row.tOutput = SU2_MPI::Wtime() - tOutput;

    /*--- ConvergenceMonitoring already propagates interruption to every rank. Keep the available files. ---*/
    row.interrupted = direct_output->GetConvergenceInterrupted();
    if (row.interrupted) {
      if (rank == MASTER_NODE)
        cout << "Goal adaptation interrupted during the primal phase. Primal files saved; stopping before remeshing."
             << endl;
      row.rss = CurrentRSS();
      row.rssMax = CPassiveComm::Allreduce(row.rss, CPassiveComm::Op::MAX);
      rows.push_back(row);
      AD::Reset();
      break;
    }

    /*--- 2. Adjoint phase: main recording of the primal state, fixed-point iterations. ---*/
    nAdjoint_Iter = level ? level->adjIter : nIterRun;
    config->SetnInner_Iter(nAdjoint_Iter);
    output->SetResidualReduction(level ? level->adjResidualReduction : 0.0);
    output->ResetConvergenceMonitoring(0);
    output->SetConvergence(false);
    StopCalc = false;
    auto tPhase = SU2_MPI::Wtime();
    PreprocessGoalAdjoint();
    row.tRecord = SU2_MPI::Wtime() - tPhase;
    tPhase = SU2_MPI::Wtime();
    Run();
    row.tAdjoint = SU2_MPI::Wtime() - tPhase;
    adjointOffset += goalAdjointIters;
    row.adjointIter = goalAdjointIters;
    row.interrupted = direct_output->GetConvergenceInterrupted() || output->GetConvergenceInterrupted();
    row.adjointConverged = !row.interrupted && (output->GetConvergence() || (goalAdjointIters < nAdjoint_Iter));
    row.adjointRes = goalAdjointLast.empty() ? 0.0 : goalAdjointLast.front().second;
    row.adjointDrop = Reduction(goalAdjointMax, goalAdjointLast);

    if (row.interrupted) {
      tOutput = SU2_MPI::Wtime();
      output->SetResultFiles(geometry, config, solver, goalAdjointIters > 0 ? goalAdjointIters - 1 : 0, true);
      row.tOutput += SU2_MPI::Wtime() - tOutput;
      if (rank == MASTER_NODE)
        cout << "Goal adaptation interrupted during the adjoint phase. Primal and adjoint files saved; stopping before "
                "remeshing." << endl;
      row.rss = CurrentRSS();
      row.rssMax = CPassiveComm::Allreduce(row.rss, CPassiveComm::Op::MAX);
      rows.push_back(row);
      AD::Reset();
      break;
    }

    /*--- 3. Residual adjoint, the sensitivities on the last cycle, the goal metric (tape off). ---*/
    tPhase = SU2_MPI::Wtime();
    CaptureResidualAdjoint();
    if (last) {
      SecondaryRecording();
      /*--- As in any DA run, history SENS_GEO is not updated after the last recording. Keep Adap_Iter increasing.
       *    Report the final solver totals in the summary instead. Total_Sens_AoA is per radian. ---*/
      row.sensitivities = true;
      row.sensGeo = SU2_TYPE::GetValue(solver[ADJFLOW_SOL]->GetTotal_Sens_Geo());
      row.sensAoA = SU2_TYPE::GetValue(solver[ADJFLOW_SOL]->GetTotal_Sens_AoA());
      row.sensMach = SU2_TYPE::GetValue(solver[ADJFLOW_SOL]->GetTotal_Sens_Mach());
      output->SetVolumeOutputExclusions({});
    }
    if (AD::TapeActive()) AD::StopRecording();
    ComputeGoalMetric();
    row.tMetric = SU2_MPI::Wtime() - tPhase;
    row.preBL = solver[FLOW_SOL]->GetMetricComplexityPreBL();
    row.final = solver[FLOW_SOL]->GetMetricComplexityFinal();
    row.bracketed = solver[FLOW_SOL]->GetMetricComplexityBracketed();
    row.goalRejected = solver[FLOW_SOL]->GetGoalRejected();
    row.goalNonFinite = solver[FLOW_SOL]->GetGoalNonFinite();
    row.goalMinRatio = solver[FLOW_SOL]->GetGoalMinRatio();

    /*--- 4. Files of the adjoint of this cycle, before any replacement (lambda, H_go, metric). ---*/
    tOutput = SU2_MPI::Wtime();
    output->SetResultFiles(geometry, config, solver, goalAdjointIters > 0 ? goalAdjointIters - 1 : 0, true);
    row.tOutput += SU2_MPI::Wtime() - tOutput;

    if (!last) {
      /*--- 5. New mesh from the goal metric, primal and adjoint transferred. ---*/
      tPhase = SU2_MPI::Wtime();
      AD::Reset();
      const auto mesh = remesher->Remesh(*config, *geometry, solver[FLOW_SOL]->GetNodes()->GetMetric());
      if (iCycle == 0) config->SetSolutionInMemory();
      const auto iLevel = config->GetAdap_CycleLevel(iCycle + 1);
      initialRunState.SetCFL(config->GetAdap_Level(iLevel).flowCFL);
      SwapMesh(mesh, transfer);
      if (config->GetAdap_Mesh_Output()) WriteAdaptedMesh(iCycle + 1);
      row.tRemesh = SU2_MPI::Wtime() - tPhase;
    }

    row.rss = CurrentRSS();
    row.rssMax = CPassiveComm::Allreduce(row.rss, CPassiveComm::Op::MAX);
    rows.push_back(row);
  }

  /*--- The tape refers to the solvers that the driver releases at the end. ---*/
  AD::Reset();
  RecordingState = RECORDING::CLEAR_INDICES;
  goalLoop = false;

  if (rank == MASTER_NODE) {
    cout << endl << "----------------------- Goal-Oriented Adaptation Summary ----------------------" << endl;
    PrintingToolbox::CTablePrinter table(&cout);
    table.AddColumn("Cycle", 6);
    table.AddColumn("Points", 9);
    table.AddColumn("Target", 9);
    table.AddColumn("Pre-BL C", 12);
    table.AddColumn("J", 16);
    table.AddColumn("Primal it", 10);
    table.AddColumn("Adj. it", 10);
    table.AddColumn("Adj. drop", 10);
    table.AddColumn("Interrupted", 12);
    table.AddColumn("Time [s]", 10);
    table.AddColumn("RSS [MB]", 10);
    table.PrintHeader();
    for (const auto& row : rows) {
      table << row.cycle << row.nPoint << row.requested << row.preBL << row.J << row.primalIter << row.adjointIter
            << row.adjointDrop << row.interrupted
            << (row.tPrimal + row.tRecord + row.tAdjoint + row.tMetric + row.tOutput + row.tRemesh)
            << row.rssMax;
    }
    table.PrintFooter();
    for (const auto& row : rows) {
      if (row.sensitivities)
        cout << std::setprecision(16) << "Final sensitivities, cycle " << row.cycle << ": sens_geo " << row.sensGeo
             << ", sens_aoa (per radian) " << row.sensAoA << ", sens_mach " << row.sensMach
             << std::setprecision(6) << endl;
    }
    cout << "Target and pre-BL complexity: of the metric computed on the mesh of the cycle (it makes the next mesh; the "
            "last cycle uses the last level). The geometric sensitivities are computed on the last cycle only." << endl;

    std::ofstream csv("adap_goal_summary.csv");
    csv << "cycle,points,requested_complexity,preBL_complexity,final_complexity,bracketed,J,CL,CD,primal_iter,"
           "primal_converged,primal_res,primal_drop,adjoint_iter,adjoint_converged,adjoint_res,adjoint_drop,warm_start,"
           "goal_rejected,goal_nonfinite,goal_min_ratio,t_primal,t_record,t_adjoint,t_capture_metric,t_output,"
           "t_remesh_swap,rss_mb,rss_max_mb,interrupted,sens_geo,sens_aoa,sens_mach\n";
    csv << std::setprecision(16);
    for (const auto& row : rows) {
      csv << row.cycle << "," << row.nPoint << "," << row.requested << "," << row.preBL << "," << row.final << ","
          << row.bracketed << "," << row.J << "," << row.CL << "," << row.CD << "," << row.primalIter << ","
          << row.primalConverged << "," << row.primalRes << "," << row.primalDrop << "," << row.adjointIter << ","
          << row.adjointConverged << "," << row.adjointRes << "," << row.adjointDrop << "," << row.warm << ","
          << row.goalRejected << "," << row.goalNonFinite << "," << row.goalMinRatio << "," << row.tPrimal << ","
          << row.tRecord << "," << row.tAdjoint << "," << row.tMetric << "," << row.tOutput << "," << row.tRemesh
          << "," << row.rss << "," << row.rssMax << "," << row.interrupted << ",";
      if (row.sensitivities) csv << row.sensGeo << "," << row.sensAoA << "," << row.sensMach;
      else csv << ",,";
      csv << "\n";
    }
  }
}
