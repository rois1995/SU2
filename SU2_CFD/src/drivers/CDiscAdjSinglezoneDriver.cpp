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
#include "../../include/output/tools/CWindowingTools.hpp"
#include "../../include/output/COutputFactory.hpp"
#include "../../include/output/COutput.hpp"
#include "../../include/iteration/CIterationFactory.hpp"
#include "../../include/iteration/CTurboIteration.hpp"
#include "../../include/solvers/CDiscAdjSolver.hpp"
#include "../../../Common/include/toolboxes/CQuasiNewtonInvLeastSquares.hpp"


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
