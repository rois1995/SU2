/*!
 * \file driver_direct_singlezone.cpp
 * \brief The main subroutines for driving single-zone problems.
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

#include "../../include/drivers/CSinglezoneDriver.hpp"
#include "../../include/definition_structure.hpp"
#include "../../include/output/COutput.hpp"
#include "../../include/output/CMeshOutput.hpp"
#include "../../include/iteration/CIteration.hpp"
#include "../../include/adaptation/CSolutionTransfer.hpp"
#include "../../include/adaptation/CBarycentricTransfer.hpp"
#include "../../include/adaptation/CConservativeTransfer.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"

#include <memory>

CSinglezoneDriver::CSinglezoneDriver(char* confFile,
                       unsigned short val_nZone,
                       SU2_Comm MPICommunicator) : CDriver(confFile,
                                                          val_nZone,
                                                          MPICommunicator,
                                                          false) {

  /*--- Initialize the counter for TimeIter ---*/
  TimeIter = 0;
}

CSinglezoneDriver::~CSinglezoneDriver() = default;

void CSinglezoneDriver::StartSolver() {
  SU2_ZONE_SCOPED

  const auto* config = config_container[ZONE_0];
  if (config->GetAdap_Loop()) {
    if (config->GetTime_Domain()) RunTimeAdaptationLoop();
    else RunAdaptationLoop();
  } else {
    RunTimeLoop();
  }
}

void CSinglezoneDriver::RunTimeLoop() {
  SU2_ZONE_SCOPED

  StartTime = SU2_MPI::Wtime();

  config_container[ZONE_0]->Set_StartTime(StartTime);

  /*--- Main external loop of the solver. Runs for the number of time steps required. ---*/

  if (rank == MASTER_NODE)
    cout << endl <<"------------------------------ Begin Solver -----------------------------" << endl;

  if (rank == MASTER_NODE){
    cout << endl <<"Simulation Run using the Single-zone Driver" << endl;
    if (driver_config->GetTime_Domain())
      cout << "The simulation will run for "
           << driver_config->GetnTime_Iter() - config_container[ZONE_0]->GetRestart_Iter() << " time steps." << endl;
  }

  /*--- Set the initial time iteration to the restart iteration. ---*/
  if (config_container[ZONE_0]->GetRestart() && driver_config->GetTime_Domain())
    TimeIter = config_container[ZONE_0]->GetRestart_Iter();

  /*--- Run the problem until the number of time iterations required is reached. ---*/
  /*--- or until a SIGTERM signal stops the loop. We catch SIGTERM and exit gracefully ---*/
  while ( TimeIter < config_container[ZONE_0]->GetnTime_Iter()) {
    /*--- Perform some preprocessing before starting the time-step simulation. ---*/

    Preprocess(TimeIter);

    /*--- Run a time-step iteration of the single-zone problem. ---*/

    Run();

    /*--- Perform some postprocessing on the solution before the update ---*/

    Postprocess();

    /*--- Update the solution for dual time stepping strategy ---*/

    Update();

    /*--- Monitor the computations after each iteration. ---*/

    Monitor(TimeIter);

    /*--- Output the solution in files. ---*/

    Output(TimeIter);

    /*--- If the convergence criteria has been met, terminate the simulation. ---*/

    if (StopCalc) break;

    TimeIter++;

  }

}

void CSinglezoneDriver::Preprocess(unsigned long TimeIter) {
  SU2_ZONE_SCOPED

  /*--- Set the current time iteration in the config and also in the driver
   * because the python interface doesn't offer an explicit way of doing it. ---*/

  this->TimeIter = TimeIter;
  config_container[ZONE_0]->SetTimeIter(TimeIter);

  /*--- Store the current physical time in the config container, as
   this can be used for verification / MMS. This should also be more
   general once the drivers are more stable. ---*/

  if (config_container[ZONE_0]->GetTime_Marching() != TIME_MARCHING::STEADY)
    config_container[ZONE_0]->SetPhysicalTime(static_cast<su2double>(TimeIter)*config_container[ZONE_0]->GetDelta_UnstTimeND());
  else
    config_container[ZONE_0]->SetPhysicalTime(0.0);


  /*--- Set the initial condition for EULER/N-S/RANS ---------------------------------------------*/
  if (config_container[ZONE_0]->GetFluidProblem()) {
    solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]->SetInitialCondition(geometry_container[ZONE_0][INST_0],
                                                                            solver_container[ZONE_0][INST_0],
                                                                            config_container[ZONE_0], TimeIter);
  }
  if (config_container[ZONE_0]->GetKind_Species_Model() != SPECIES_MODEL::NONE) {
    solver_container[ZONE_0][INST_0][MESH_0][SPECIES_SOL]->SetInitialCondition(geometry_container[ZONE_0][INST_0],
                                                                                solver_container[ZONE_0][INST_0],
                                                                                config_container[ZONE_0], TimeIter);
  }
  else if (config_container[ZONE_0]->GetHeatProblem()) {
    /*--- Set the initial condition for HEAT equation ---------------------------------------------*/
    solver_container[ZONE_0][INST_0][MESH_0][HEAT_SOL]->SetInitialCondition(geometry_container[ZONE_0][INST_0],
                                                                            solver_container[ZONE_0][INST_0],
                                                                            config_container[ZONE_0], TimeIter);
  }

  SU2_MPI::Barrier(SU2_MPI::GetComm());

  /*--- Run a predictor step ---*/
  if (config_container[ZONE_0]->GetPredictor())
    iteration_container[ZONE_0][INST_0]->Predictor(output_container[ZONE_0], integration_container, geometry_container, solver_container,
        numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

  /*--- Perform a dynamic mesh update if required. ---*/
  /*--- For the Disc.Adj. of a case with (rigidly) moving grid, the appropriate
          mesh cordinates are read from the restart files. ---*/
  if (!(config_container[ZONE_0]->GetGrid_Movement() && config_container[ZONE_0]->GetDiscrete_Adjoint()))
    DynamicMeshUpdate(TimeIter);

}

void CSinglezoneDriver::Run() {
  SU2_ZONE_SCOPED

  unsigned long OuterIter = 0;
  config_container[ZONE_0]->SetOuterIter(OuterIter);

  /*--- Iterate the zone as a block, either to convergence or to a max number of iterations ---*/
  iteration_container[ZONE_0][INST_0]->Solve(output_container[ZONE_0], integration_container, geometry_container, solver_container,
        numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

}

void CSinglezoneDriver::Postprocess() {
  SU2_ZONE_SCOPED

  iteration_container[ZONE_0][INST_0]->Postprocess(output_container[ZONE_0], integration_container, geometry_container, solver_container,
      numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

  /*--- A corrector step can help preventing numerical instabilities ---*/

  if (config_container[ZONE_0]->GetRelaxation())
    iteration_container[ZONE_0][INST_0]->Relaxation(output_container[ZONE_0], integration_container, geometry_container, solver_container,
        numerics_container, config_container, surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

  /*--- Compute the metric for mesh adaptation from the converged (or current time step) solution. The time-domain
   *    adaptation loop collects the Hessians of the time steps of its window instead. ---*/

  if (config_container[ZONE_0]->GetCompute_Metric()) {
    if (config_container[ZONE_0]->GetAdap_Loop() && config_container[ZONE_0]->GetTime_Domain()) SampleTimeWindowMetric();
    else ComputeMetric();
  }

}

void CSinglezoneDriver::ComputeMetric() {
  SU2_ZONE_SCOPED

  auto* config = config_container[ZONE_0];
  auto* geometry = geometry_container[ZONE_0][INST_0][MESH_0];
  auto* solver_flow = solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];

  const auto kindSolver = config->GetKind_Solver();
  if (solver_flow == nullptr || (kindSolver != MAIN_SOLVER::EULER && kindSolver != MAIN_SOLVER::NAVIER_STOKES &&
                                 kindSolver != MAIN_SOLVER::RANS)) {
    SU2_MPI::Error("The adaptation metric is only available for compressible EULER, NAVIER_STOKES or RANS.",
                   CURRENT_FUNCTION);
  }

  if (rank == MASTER_NODE)
    cout << endl << "----------------------------- Compute Metric ----------------------------" << endl;

  /*--- Sensors, their gradients and Hessians, then the metric. The results are kept in the flow
   *    variables (also on halo points) until the next call. ---*/

  const auto startTime = SU2_MPI::Wtime();
  solver_flow->SetAuxVar_Adapt(geometry, config);
  solver_flow->SetHessian_Adapt(geometry, config);
  solver_flow->ComputeMetric(geometry, config);
  if (rank == MASTER_NODE) cout << "Metric computed in " << SU2_MPI::Wtime() - startTime << " s." << endl;
}

void CSinglezoneDriver::CheckMeshAdaptation() const {

  const auto* config = config_container[ZONE_0];

  const auto kindSolver = config->GetKind_Solver();
  if (kindSolver != MAIN_SOLVER::EULER && kindSolver != MAIN_SOLVER::NAVIER_STOKES &&
      kindSolver != MAIN_SOLVER::RANS) {
    SU2_MPI::Error("Mesh adaptation is only available for compressible EULER, NAVIER_STOKES or RANS.",
                   CURRENT_FUNCTION);
  }
  if (driver_config->GetTime_Domain() || config->GetTime_Domain()) {
    const auto marching = config->GetTime_Marching();
    if (marching != TIME_MARCHING::DT_STEPPING_1ST && marching != TIME_MARCHING::DT_STEPPING_2ND) {
      SU2_MPI::Error("Mesh adaptation of time-domain problems needs dual time stepping (TIME_MARCHING= "
                     "DUAL_TIME_STEPPING-1ST_ORDER or DUAL_TIME_STEPPING-2ND_ORDER).", CURRENT_FUNCTION);
    }
  }
  CMMGInterface::CheckSupport(*config, *geometry_container[ZONE_0][INST_0][MESH_0]);
}

CSimplexMesh CSinglezoneDriver::RemeshFromMetric() {
  CMMGRemesher remesher;
  return RemeshFromMetric(remesher);
}

CSimplexMesh CSinglezoneDriver::RemeshFromMetric(CRemesher& remesher) {
  SU2_ZONE_SCOPED

  CheckMeshAdaptation();

  auto* config = config_container[ZONE_0];
  if (!config->GetCompute_Metric()) {
    SU2_MPI::Error("Mesh adaptation needs the metric, set COMPUTE_METRIC= YES.", CURRENT_FUNCTION);
  }

  ComputeMetric();

  const auto* geometry = geometry_container[ZONE_0][INST_0][MESH_0];
  const auto* solver_flow = solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];
  return remesher.Remesh(*config, *geometry, solver_flow->GetNodes()->GetMetric());
}

void CSinglezoneDriver::RunAdaptationLoop() {
  SU2_ZONE_SCOPED

  auto* config = config_container[ZONE_0];
  auto* output = output_container[ZONE_0];

  /*--- Stop before the first solve if the problem or the build cannot be adapted. ---*/

  CheckMeshAdaptation();
  if (config->GetWrt_Adap_Mesh()) CheckAdaptedMeshNames();

  CMMGRemesher remesher;

  std::unique_ptr<CSolutionTransfer> transfer;
  switch (config->GetKind_Adap_Transfer()) {
    case ADAP_TRANSFER::BARYCENTRIC: transfer = std::make_unique<CBarycentricTransfer>(); break;
    case ADAP_TRANSFER::CONSERVATIVE: transfer = std::make_unique<CConservativeTransfer>(); break;
    case ADAP_TRANSFER::FREESTREAM: transfer = std::make_unique<CFreeStreamTransfer>(); break;
  }

  const auto nCycles = config->GetnAdap_Cycles();

  struct CycleSummary {
    unsigned long complexity, nPoint, nIter;
    passivedouble solveTime, adaptTime;
  };
  vector<CycleSummary> summary;
  unsigned long iterOffset = 0;

  for (unsigned long iCycle = 0; iCycle <= nCycles; iCycle++) {

    /*--- The metric of this solve makes the mesh of the next cycle (the last solve keeps the last level). ---*/

    config->SetAdap_MetricLevel(config->GetAdap_CycleLevel(min(iCycle + 1, nCycles)));
    output->SetAdaptationCycle(iCycle, iterOffset);

    if (rank == MASTER_NODE) {
      const auto cycle = config->GetMGCycle();
      cout << endl << "------------------------- Mesh Adaptation Cycle -------------------------" << endl;
      cout << "Cycle " << iCycle << " of " << nCycles << ": "
           << geometry_container[ZONE_0][INST_0][MESH_0]->GetGlobal_nPointDomain() << " points, at most "
           << config->GetnInner_Iter() << " iterations, CFL " << config->GetCFL(MESH_0) << ", "
           << (cycle == MG_CYCLE::W ? "W" : (cycle == MG_CYCLE::V ? "V" : "full")) << " multigrid cycle, "
           << (config->GetRestart() ? "restart" : iCycle == 0 ? "initial" :
               config->GetKind_Adap_Transfer() == ADAP_TRANSFER::FREESTREAM ? "free-stream" : "transferred")
           << " solution." << endl;
      if (iCycle < nCycles) {
        cout << "Its metric makes the next mesh: complexity " << config->GetAdap_Complexity() << ", sizes "
             << config->GetAdap_Hmin() << " to " << config->GetAdap_Hmax() << ", norm " << config->GetAdap_Norm()
             << ", aspect ratio up to " << config->GetAdap_ARmax() << "." << endl;
      }
    }

    TimeIter = 0;
    const auto solveStart = SU2_MPI::Wtime();
    RunTimeLoop();
    const auto adaptStart = SU2_MPI::Wtime();

    const auto nIter = config->GetInnerIter() + 1;
    iterOffset += nIter;
    summary.push_back({iCycle == 0 ? 0 : config->GetAdap_Level(config->GetAdap_CycleLevel(iCycle)).complexity,
                       geometry_container[ZONE_0][INST_0][MESH_0]->GetGlobal_nPointDomain(), nIter,
                       adaptStart - solveStart, 0.0});

    if (iCycle == nCycles) break;

    /*--- New mesh from the metric of this solution. ---*/

    const auto mesh = RemeshFromMetric(remesher);

    /*--- The adapted meshes start from the transferred solution, not from restart files, and with the options of
     *    their level. The first solve may have been a restart (also of the fixed-CL angle of attack). ---*/

    if (iCycle == 0) config->SetSolutionInMemory();

    const auto iLevel = config->GetAdap_CycleLevel(iCycle + 1);
    const auto& level = config->GetAdap_Level(iLevel);
    initialRunState.SetCFL(level.flowCFL);

    ReplaceMesh(mesh, *transfer);

    config->SetAdap_FlowLevel(iLevel);
    output->SetResidualReduction(level.residualReduction);

    /*--- The new mesh is accepted (built, solution transferred). ---*/

    if (config->GetWrt_Adap_Mesh()) WriteAdaptedMesh(iCycle + 1);

    summary.back().adaptTime = SU2_MPI::Wtime() - adaptStart;
  }

  if (rank == MASTER_NODE) {
    cout << endl << "------------------------ Mesh Adaptation Summary ------------------------" << endl;
    PrintingToolbox::CTablePrinter table(&cout);
    table.AddColumn("Cycle", 10);
    table.AddColumn("Complexity", 16);
    table.AddColumn("Points", 16);
    table.AddColumn("Iterations", 16);
    table.AddColumn("Solve [s]", 12);
    table.AddColumn("Adapt [s]", 12);
    table.PrintHeader();
    for (unsigned long iCycle = 0; iCycle < summary.size(); iCycle++) {
      const auto& row = summary[iCycle];
      table << iCycle << (iCycle == 0 ? string("input mesh") : to_string(row.complexity)) << row.nPoint << row.nIter
            << row.solveTime;
      if (iCycle + 1 < summary.size()) table << row.adaptTime;
      else table << "-";
    }
    table.PrintFooter();
    cout << "Solve: the flow iterations on the mesh of the cycle, with their output. Adapt: metric, MMG, new "
            "geometry and solvers, solution transfer and mesh output that make the mesh of the next cycle." << endl;
  }
}

void CSinglezoneDriver::SampleTimeWindowMetric() {
  SU2_ZONE_SCOPED

  const auto startTime = SU2_MPI::Wtime();

  auto* config = config_container[ZONE_0];
  auto* geometry = geometry_container[ZONE_0][INST_0][MESH_0];
  auto* solver_flow = solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];

  /*--- Sensors and their Hessians at this time step. ---*/

  solver_flow->SetAuxVar_Adapt(geometry, config);
  solver_flow->SetHessian_Adapt(geometry, config);

  /*--- Add |H| of each sensor (eigenvalues by their absolute values; a non-finite Hessian counts as zero, as in
   *    CSolver::ComputeMetric) to the window. ---*/

  auto* nodes = solver_flow->GetNodes();
  auto& hessian = nodes->GetHessian();
  const auto nPoint = geometry->GetnPoint();
  const auto nSensor = config->GetnAdap_Sensor();
  const unsigned short nMet = nDim * (nDim + 1) / 2;

  if (windowSamples == 0) windowHessian.assign(nPoint * nSensor * nMet, 0.0);

  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    for (auto iSensor = 0u; iSensor < nSensor; ++iSensor) {
      su2double H[3][3] = {{0.0}}, vec[3][3] = {{0.0}}, val[3] = {0.0}, work[3] = {0.0};
      nodes->GetHessianMat(iPoint, iSensor, H);
      bool finite = true;
      for (auto i = 0u; i < nDim; ++i)
        for (auto j = 0u; j < nDim; ++j) finite = finite && std::isfinite(SU2_TYPE::GetValue(H[i][j]));
      if (!finite) continue;
      CBlasStructure::EigenDecomposition(H, vec, val, nDim, work);
      for (auto i = 0u; i < nDim; ++i) val[i] = fabs(val[i]);
      CBlasStructure::EigenRecomposition(H, vec, val, nDim);
      auto* sum = &windowHessian[(iPoint * nSensor + iSensor) * nMet];
      for (unsigned short iDim = 0, iMet = 0; iDim < nDim; ++iDim)
        for (unsigned short jDim = iDim; jDim < nDim; ++jDim, ++iMet) sum[iMet] += H[iDim][jDim];
    }
  }
  windowSamples++;

  /*--- End of the window: metric of the mean |H| of its time steps, with the options of the loop. ---*/

  if (config->GetAdap_TimeWindowEnd(TimeIter)) {
    const su2double factor = 1.0 / windowSamples;
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (auto iSensor = 0u; iSensor < nSensor; ++iSensor)
        for (unsigned short iMet = 0; iMet < nMet; ++iMet)
          hessian(iPoint, iSensor, iMet) = factor * windowHessian[(iPoint * nSensor + iSensor) * nMet + iMet];

    if (rank == MASTER_NODE) {
      cout << endl << "----------------------------- Compute Metric ----------------------------" << endl;
      cout << "Metric of the time window: mean |Hessian| of the sensors over " << windowSamples
           << " time steps (" << TimeIter + 1 - windowSamples << " to " << TimeIter << ")." << endl;
    }
    solver_flow->ComputeMetric(geometry, config);
  }

  windowMetricTime += SU2_MPI::Wtime() - startTime;
}

void CSinglezoneDriver::WriteTimeHistoryRestarts() {
  SU2_ZONE_SCOPED

  auto* config = config_container[ZONE_0];
  auto* geometry = geometry_container[ZONE_0][INST_0][MESH_0];
  auto** solver = solver_container[ZONE_0][INST_0][MESH_0];
  auto* output = output_container[ZONE_0];
  const auto timeIter = config->GetTimeIter();

  /*--- U^n (Solution = Solution_time_n after the dual-time update), named for step n. The files have the usual
   *    unsteady names, so they replace the files of steps n (and n-1 below) written on the previous mesh. ---*/

  if (!output->WriteRestartFiles(geometry, config, solver)) return;

  /*--- U^(n-1) for a 2nd-order restart, named for step n-1: Solution_time_n1 written as the solution. ---*/

  if (config->GetTime_Marching() != TIME_MARCHING::DT_STEPPING_2ND || timeIter == 0) return;

  auto swapHistory = [&]() {
    for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
      if (solver[iSol] == nullptr) continue;
      auto* nodes = solver[iSol]->GetNodes();
      if (nodes->GetSolution_time_n1().size() != nodes->GetSolution().size()) continue;
      std::swap(nodes->GetSolution(), nodes->GetSolution_time_n1());
    }
  };
  swapHistory();
  config->SetTimeIter(timeIter - 1);
  output->WriteRestartFiles(geometry, config, solver);
  config->SetTimeIter(timeIter);
  swapHistory();
}

void CSinglezoneDriver::RunTimeAdaptationLoop() {
  SU2_ZONE_SCOPED

  auto* config = config_container[ZONE_0];
  auto* output = output_container[ZONE_0];

  /*--- Stop before the first time step if the problem or the build cannot be adapted. ---*/

  CheckMeshAdaptation();
  if (config->GetWrt_Adap_Mesh()) CheckAdaptedMeshNames();

  CMMGRemesher remesher;

  /*--- ADAP_TRANSFER: CONSERVATIVE (default of time-domain runs) or BARYCENTRIC (FREESTREAM is rejected for
   *    time-domain runs by the config). ---*/
  const bool conservative = config->GetKind_Adap_Transfer() == ADAP_TRANSFER::CONSERVATIVE;
  std::unique_ptr<CSolutionTransfer> transferPtr;
  if (conservative) {
    transferPtr = std::make_unique<CConservativeTransfer>();
  } else {
    transferPtr = std::make_unique<CBarycentricTransfer>();
  }
  auto& transfer = *transferPtr;

  /*--- One metric level (one complexity) for every window. ---*/

  config->SetAdap_MetricLevel(0);

  StartTime = SU2_MPI::Wtime();
  config->Set_StartTime(StartTime);

  const unsigned long firstTimeIter = config->GetRestart() ? config->GetRestart_Iter() : 0;
  const auto nTimeIter = config->GetnTime_Iter();
  const auto freq = config->GetAdap_Freq();

  if (rank == MASTER_NODE) {
    cout << endl << "------------------------------ Begin Solver -----------------------------" << endl;
    cout << endl << "Simulation Run using the Single-zone Driver, time-domain mesh adaptation" << endl;
    cout << "The simulation will run for " << nTimeIter - firstTimeIter << " time steps." << endl;
    cout << "The mesh is adapted after every " << freq << " time steps (after the time steps n with (n+1) % " << freq
         << " == 0), from the mean |Hessian| of the sensors over those steps: complexity " << config->GetAdap_Complexity()
         << ", sizes " << config->GetAdap_Hmin() << " to " << config->GetAdap_Hmax() << ", norm " << config->GetAdap_Norm()
         << ", aspect ratio up to " << config->GetAdap_ARmax() << "." << endl;
    if (conservative) {
      cout << "Solution transfer: conservative P1 projection of the solution and of its time history (the integrals "
              "of the conservative variables are kept)";
    } else {
      cout << "Solution transfer: barycentric (P1) interpolation of the solution and of its time history (the "
              "integrals of the conservative variables are not conserved)";
    }
    cout << (config->GetAdap_Transfer_Default() ? ", the default of time-domain runs." : ".") << endl;
  }

  struct CycleSummary {
    unsigned long cycle = 0, firstStep = 0, lastStep = 0, nPoint = 0, nOutside = 0;
    passivedouble solveTime = 0.0, metricTime = 0.0, remeshTime = 0.0, replaceTime = 0.0, transferTime = 0.0,
                  outputTime = 0.0, maxDistance = 0.0, maxRelDistance = 0.0, defect = -1.0;
  };
  /*--- The cycles are counted in time windows from time step 0 (cycle = time step / ADAP_FREQ), so a restarted run
   *    continues the cycle numbers (history column Adap_Cycle). The files keep the usual unsteady names. ---*/

  unsigned long iCycle = firstTimeIter / freq;
  output->SetAdaptationCycle(iCycle, 0, false);

  vector<CycleSummary> summary(1);
  summary.back().cycle = iCycle;
  summary.back().firstStep = firstTimeIter;
  summary.back().nPoint = geometry_container[ZONE_0][INST_0][MESH_0]->GetGlobal_nPointDomain();

  windowSamples = 0;
  windowMetricTime = 0.0;
  auto cycleStart = SU2_MPI::Wtime();

  /*--- The time loop of RunTimeLoop, with the adaptation after the steps that end a time window. ---*/

  TimeIter = firstTimeIter;
  while (TimeIter < nTimeIter) {

    Preprocess(TimeIter);
    Run();
    Postprocess();
    Update();
    Monitor(TimeIter);
    Output(TimeIter);

    summary.back().lastStep = TimeIter;
    if (StopCalc) break;

    /*--- Adaptation point: time step n is complete (dual-time history updated, files written). The metric of the
     *    window was computed in Postprocess. Remesh, transfer the solution and its history, continue with n+1. ---*/

    if (config->GetAdap_TimeWindowEnd(TimeIter) && TimeIter + 1 < nTimeIter) {
      auto& row = summary.back();
      const auto adaptStart = SU2_MPI::Wtime();
      row.metricTime = SU2_TYPE::GetValue(windowMetricTime);
      row.solveTime = SU2_TYPE::GetValue(adaptStart - cycleStart - windowMetricTime);

      if (rank == MASTER_NODE) {
        cout << endl << "------------------------- Mesh Adaptation Cycle -------------------------" << endl;
        cout << "After time step " << TimeIter << ": mesh of cycle " << (TimeIter + 1) / freq
             << " from the metric of the time steps " << row.firstStep << " to " << TimeIter << " (cycle " << iCycle
             << ")." << endl;
      }

      const auto mesh = remesher.Remesh(*config, *geometry_container[ZONE_0][INST_0][MESH_0],
                                        solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]->GetNodes()->GetMetric());
      const auto remeshTime = SU2_MPI::Wtime() - adaptStart;
      UsedTimePreproc += remeshTime;

      /*--- The new meshes start from the transferred solution, not from restart files. ---*/
      config->SetSolutionInMemory();

      ReplaceMesh(mesh, transfer);
      iCycle = (TimeIter + 1) / freq;
      output->SetAdaptationCycle(iCycle, 0, false);

      /*--- Files of the new mesh: the mesh (named for its first time step), and the restart files of the transferred
       *    steps n and n-1 (they replace those of the previous mesh). ---*/

      const auto outputStart = SU2_MPI::Wtime();
      if (config->GetWrt_Adap_Mesh()) WriteAdaptedMesh(iCycle, TimeIter + 1);
      WriteTimeHistoryRestarts();
      const auto outputTime = SU2_MPI::Wtime() - outputStart;
      UsedTimeOutput += outputTime;

      const auto transferReport = transfer.GetReport();
      row.remeshTime = SU2_TYPE::GetValue(remeshTime);
      row.replaceTime = SU2_TYPE::GetValue(lastReplaceTime);
      row.transferTime = SU2_TYPE::GetValue(lastTransferTime);
      row.outputTime = SU2_TYPE::GetValue(outputTime);
      row.nOutside = transferReport.nOutside;
      row.maxDistance = transferReport.maxDistance;
      row.maxRelDistance = transferReport.maxRelDistance;
      row.defect = transferReport.conservationDefect;

      CycleSummary next;
      next.cycle = iCycle;
      next.firstStep = TimeIter + 1;
      next.lastStep = TimeIter + 1;
      next.nPoint = geometry_container[ZONE_0][INST_0][MESH_0]->GetGlobal_nPointDomain();
      summary.push_back(next);

      windowSamples = 0;
      windowMetricTime = 0.0;
      cycleStart = SU2_MPI::Wtime();

      /*--- The adaptation is not part of the compute time of the next time step. ---*/
      StartTime = cycleStart;
      config->Set_StartTime(StartTime);
    }

    TimeIter++;
  }

  summary.back().metricTime = SU2_TYPE::GetValue(windowMetricTime);
  summary.back().solveTime = SU2_TYPE::GetValue(SU2_MPI::Wtime() - cycleStart - windowMetricTime);

  if (rank == MASTER_NODE) {
    cout << endl << "------------------------ Mesh Adaptation Summary ------------------------" << endl;
    PrintingToolbox::CTablePrinter table(&cout);
    table.AddColumn("Cycle", 6);
    table.AddColumn("Time steps", 14);
    table.AddColumn("Points", 9);
    table.AddColumn("Solve [s]", 10);
    table.AddColumn("Metric [s]", 10);
    table.AddColumn("Remesh [s]", 10);
    table.AddColumn("Replace [s]", 11);
    table.AddColumn("Transfer [s]", 12);
    table.AddColumn("Output [s]", 10);
    table.AddColumn("Outside", 8);
    table.AddColumn("Max dist.", 10);
    table.AddColumn("Cons. defect", 12);
    table.PrintHeader();
    passivedouble total[6] = {0.0};
    for (unsigned long i = 0; i < summary.size(); i++) {
      const auto& row = summary[i];
      table << row.cycle << to_string(row.firstStep) + "-" + to_string(row.lastStep) << row.nPoint << row.solveTime
            << row.metricTime;
      if (i + 1 < summary.size()) {
        table << row.remeshTime << row.replaceTime << row.transferTime << row.outputTime << row.nOutside
              << row.maxDistance << row.defect;
      } else {
        table << "-" << "-" << "-" << "-" << "-" << "-" << "-";
      }
      const passivedouble values[] = {row.solveTime, row.metricTime, row.remeshTime, row.replaceTime,
                                      row.transferTime, row.outputTime};
      for (int k = 0; k < 6; ++k) total[k] += values[k];
    }
    table.PrintFooter();
    cout << "Total: solve " << total[0] << " s, metric " << total[1] << " s, remesh " << total[2] << " s, replace "
         << total[3] << " s (transfer " << total[4] << " s), adapted mesh and restart output " << total[5] << " s."
         << endl;
    cout << "Solve: the time steps on the mesh of the cycle with their output, without the metric. Metric: sensors, "
            "Hessians and the window metric of every time step. Remesh: extraction, MMG and validation. Replace: new "
            "geometry and solvers with the solution transfer. Output: adapted mesh and restart files of the "
            "transferred steps. Outside: new points outside the donor mesh (closest donor boundary point used; "
            "conservative transfer: points whose control volume is partly outside), their largest distance. Cons. "
            "defect: largest relative change of the integrals of the conservative flow variables." << endl;
  }
}

void CSinglezoneDriver::CheckAdaptedMeshNames() const {

  const auto* config = config_container[ZONE_0];
  const auto baseName = config->GetMesh_Out_FileName();
  const auto extension = config->GetMesh_Out_FileExtension();

  auto check = [&](unsigned long iCycle, unsigned long firstTimeIter) {
    const auto fileName = AdaptedMeshName(iCycle, firstTimeIter) + extension;
    if (fileName == config->GetMesh_FileName()) {
      SU2_MPI::Error("The adapted mesh of cycle " + to_string(iCycle) + " would overwrite the input mesh " + fileName +
                     ", change MESH_OUT_FILENAME.", CURRENT_FUNCTION);
    }
  };
  if (config->GetTime_Domain()) {
    /*--- One cycle per time window (cycle = time step / ADAP_FREQ), the mesh name has its first time step. ---*/
    const unsigned long firstTimeIter = config->GetRestart() ? config->GetRestart_Iter() : 0;
    for (auto timeIter = firstTimeIter; timeIter + 1 < config->GetnTime_Iter(); timeIter++)
      if (config->GetAdap_TimeWindowEnd(timeIter)) check((timeIter + 1) / config->GetAdap_Freq(), timeIter + 1);
  } else {
    for (unsigned long iCycle = 1; iCycle <= config->GetnAdap_Cycles(); iCycle++) check(iCycle, 0);
  }

  /*--- The volume and surface files of the loop have the same cycle numbers. ---*/

  for (unsigned short iFile = 0; iFile < config->GetnVolumeOutputFiles(); iFile++) {
    string outputName, outputExtension;
    switch (config->GetVolumeOutputFiles()[iFile]) {
      case OUTPUT_TYPE::CGNS: outputName = config->GetVolume_FileName(); outputExtension = ".cgns"; break;
      case OUTPUT_TYPE::SURFACE_CGNS: outputName = config->GetSurfCoeff_FileName(); outputExtension = ".cgns"; break;
      case OUTPUT_TYPE::MESH: outputName = config->GetVolume_FileName(); outputExtension = ".su2"; break;
      case OUTPUT_TYPE::MESH_BINARY: outputName = config->GetVolume_FileName(); outputExtension = ".su2b"; break;
      default: continue;
    }
    if (outputName == baseName && outputExtension == extension) {
      SU2_MPI::Error("The adapted meshes would overwrite the " + outputExtension + " output files of OUTPUT_FILES (" +
                     outputName + "), change MESH_OUT_FILENAME.", CURRENT_FUNCTION);
    }
  }
}

string CSinglezoneDriver::AdaptedMeshName(unsigned long iCycle, unsigned long firstTimeIter) const {
  const auto* config = config_container[ZONE_0];
  if (!config->GetTime_Domain()) return CConfig::GetAdap_FileName(config->GetMesh_Out_FileName(), iCycle);
  return config->GetUnsteady_FileName(config->GetMesh_Out_FileName(), static_cast<int>(firstTimeIter), "");
}

void CSinglezoneDriver::WriteAdaptedMesh(unsigned long iCycle, unsigned long firstTimeIter) const {
  SU2_ZONE_SCOPED

  auto* config = config_container[ZONE_0];
  const auto fileName = AdaptedMeshName(iCycle, firstTimeIter);

  if (rank == MASTER_NODE) {
    cout << "Writing the adapted mesh of cycle " << iCycle << ": " << fileName + config->GetMesh_Out_FileExtension()
         << "." << endl;
  }
  CMeshOutput::WriteMesh(config, geometry_container[ZONE_0][INST_0][MESH_0], fileName);
}

void CSinglezoneDriver::ReplaceMesh(const CSimplexMesh& mesh, CSolutionTransfer& transfer) {
  SU2_ZONE_SCOPED

  const su2double startTime = SU2_MPI::Wtime();

  CheckMeshAdaptation();

  if (rank == MASTER_NODE)
    cout << endl << "---------------------------- Replace the Mesh ---------------------------" << endl;

  auto* config = config_container[ZONE_0];

  if (mesh.nDim != nDim) {
    SU2_MPI::Error("The new mesh has " + to_string(mesh.nDim) + " dimensions, the problem has " + to_string(nDim) +
                   ".", CURRENT_FUNCTION);
  }

  /*--- The objects of the current mesh (the donor) stay in use until the new ones are complete. The config
   *    describes the current mesh only until the new geometry is built, keep what the donor needs from it. ---*/

  CMeshDonor donor;
  donor.geometry = geometry_container[ZONE_0][INST_0];
  donor.solver = solver_container[ZONE_0][INST_0];
  donor.nMGLevels = config->GetnMGLevels();
  for (unsigned short iMarker = 0; iMarker < donor.geometry[MESH_0]->GetnMarker(); iMarker++)
    donor.markerTags.push_back(config->GetMarker_All_TagBound(iMarker));

  /*--- Start from the config state of a fresh run (requested multigrid levels, their CFL, damping, ...). ---*/

  initialRunState.Restore(*config);

  /*--- Geometry, as in InitializeGeometry. The orientation of the remeshed elements is always checked. ---*/

  CGeometry** geometry = nullptr;
  {
    CMemoryMeshReaderFVM reader(config, mesh, ZONE_0, nZone);
    BuildGeometryFVM(config, new CPhysicalGeometry(config, reader, nZone), geometry, true);
  }
  const auto nMGLevels = config->GetnMGLevels();

  geometry[MESH_0]->SetPositive_ZArea(config);
  for (unsigned short iMesh = 0; iMesh <= nMGLevels; iMesh++) geometry[iMesh]->MatchActuator_Disk(config);

  {
    if (rank == MASTER_NODE) cout << "Computing wall distances." << endl;
    CGeometry** instances[] = {geometry};
    CGeometry*** zones[] = {instances};
    CGeometry::ComputeWallDistance(config_container, zones);
  }

  /*--- Solvers in the free-stream state (the restart files belong to the first mesh), then the solution from
   *    the donor, then the objects that depend on the solvers. ---*/

  CSolver*** solver = nullptr;
  InitializeSolver(config, geometry, solver, false);

  const auto transferStart = SU2_MPI::Wtime();
  transfer.Transfer(config, donor, geometry, solver);
  const auto transferTime = SU2_MPI::Wtime() - transferStart;

  CNumerics**** numerics = nullptr;
  InitializeNumerics(config, geometry, solver, numerics);

  CIntegration** integration = nullptr;
  InitializeIntegration(config, solver[MESH_0], integration);

  CIteration* iteration = nullptr;
  PreprocessIteration(config, iteration);

  PreprocessStaticMesh(config, geometry);

  /*--- Switch the driver to the new mesh. ---*/

  auto* donorNumerics = numerics_container[ZONE_0][INST_0];
  auto* donorIntegration = integration_container[ZONE_0][INST_0];
  auto* donorIteration = iteration_container[ZONE_0][INST_0];

  geometry_container[ZONE_0][INST_0] = geometry;
  solver_container[ZONE_0][INST_0] = solver;
  numerics_container[ZONE_0][INST_0] = numerics;
  integration_container[ZONE_0][INST_0] = integration;
  iteration_container[ZONE_0][INST_0] = iteration;
  main_geometry = geometry[MESH_0];

  PreprocessPythonInterface(config_container, geometry_container, solver_container);

  /*--- Release the donor, with its own number of levels. ---*/

  FinalizeNumerics(donorNumerics, donor.nMGLevels);
  FinalizeIntegration(donorIntegration);
  FinalizeSolver(donor.solver, donor.nMGLevels);
  delete donorIteration;
  for (unsigned short iMesh = 0; iMesh <= donor.nMGLevels; iMesh++) delete donor.geometry[iMesh];
  delete [] donor.geometry;

  /*--- The output continues the history file, its geometry data and convergence monitoring start again. ---*/

  auto* output = output_container[ZONE_0];
  output->ResetMeshDependentData();
  output->ResetConvergenceMonitoring(0);
  output->SetConvergence(false);
  StopCalc = false;

  /*--- Size of the problem for the performance summary. ---*/

  Mpoints = geometry[MESH_0]->GetGlobal_nPoint() / 1.0e6;
  MpointsDomain = geometry[MESH_0]->GetGlobal_nPointDomain() / 1.0e6;
  MDOFs = DOFsPerPoint * Mpoints;
  MDOFsDomain = DOFsPerPoint * MpointsDomain;

  const su2double replaceTime = SU2_MPI::Wtime() - startTime;
  UsedTimePreproc += replaceTime;
  lastReplaceTime = replaceTime;
  lastTransferTime = transferTime;

  if (rank == MASTER_NODE) {
    cout << "The problem uses the new mesh: " << geometry[MESH_0]->GetGlobal_nPointDomain() << " points, "
         << nMGLevels << " multigrid levels." << endl;
    cout << "Mesh replaced in " << replaceTime << " s (solution transfer " << transferTime << " s)." << endl;
  }
}

void CSinglezoneDriver::AdaptMesh(CSolutionTransfer& transfer) {
  SU2_ZONE_SCOPED

  const auto mesh = RemeshFromMetric();
  ReplaceMesh(mesh, transfer);
}

void CSinglezoneDriver::Update() {
  SU2_ZONE_SCOPED

  iteration_container[ZONE_0][INST_0]->Update(output_container[ZONE_0], integration_container, geometry_container,
        solver_container, numerics_container, config_container,
        surface_movement, grid_movement, FFDBox, ZONE_0, INST_0);

}

void CSinglezoneDriver::Output(unsigned long TimeIter) {
  SU2_ZONE_SCOPED

  /*--- Time the output for performance benchmarking. ---*/

  StopTime = SU2_MPI::Wtime();

  UsedTimeCompute += StopTime-StartTime;

  StartTime = SU2_MPI::Wtime();

  bool wrote_files = output_container[ZONE_0]->SetResultFiles(geometry_container[ZONE_0][INST_0][MESH_0],
                                                               config_container[ZONE_0],
                                                               solver_container[ZONE_0][INST_0][MESH_0],
                                                               TimeIter, StopCalc);

  /*--- Save iteration solution for libROM ---*/
  if (config_container[MESH_0]->GetSave_libROM()) {
    solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]->SavelibROM(geometry_container[ZONE_0][INST_0][MESH_0],
                                                                   config_container[ZONE_0], StopCalc);
    wrote_files = true;
  }

  if (wrote_files) {

    StopTime = SU2_MPI::Wtime();

    UsedTimeOutput += StopTime-StartTime;
    OutputCount++;
    BandwidthSum = config_container[ZONE_0]->GetRestart_Bandwidth_Agg();

    StartTime = SU2_MPI::Wtime();
  }

  config_container[ZONE_0]->Set_StartTime(StartTime);
}

void CSinglezoneDriver::DynamicMeshUpdate(unsigned long TimeIter) {
  SU2_ZONE_SCOPED

  auto iteration = iteration_container[ZONE_0][INST_0];

  /*--- Legacy dynamic mesh update - Only if GRID_MOVEMENT = YES ---*/
  if (config_container[ZONE_0]->GetGrid_Movement()) {
    iteration->SetGrid_Movement(geometry_container[ZONE_0][INST_0],surface_movement[ZONE_0],
                                grid_movement[ZONE_0][INST_0], solver_container[ZONE_0][INST_0],
                                config_container[ZONE_0], 0, TimeIter);
  }

  /*--- New solver - all the other routines in SetGrid_Movement should be adapted to this one ---*/
  /*--- Works if DEFORM_MESH = YES ---*/
  iteration->SetMesh_Deformation(geometry_container[ZONE_0][INST_0],
                                 solver_container[ZONE_0][INST_0][MESH_0],
                                 numerics_container[ZONE_0][INST_0][MESH_0],
                                 config_container[ZONE_0], RECORDING::CLEAR_INDICES);

  /*--- Update the wall distances if the mesh was deformed. ---*/
  if (config_container[ZONE_0]->GetGrid_Movement() ||
      config_container[ZONE_0]->GetDeform_Mesh()) {
    CGeometry::ComputeWallDistance(config_container, geometry_container);
  }
}

bool CSinglezoneDriver::Monitor(unsigned long TimeIter){
  SU2_ZONE_SCOPED

  unsigned long nInnerIter, InnerIter, nTimeIter;
  su2double MaxTime, CurTime;
  bool TimeDomain, InnerConvergence, TimeConvergence, FinalTimeReached, MaxIterationsReached;

  nInnerIter = config_container[ZONE_0]->GetnInner_Iter();
  InnerIter  = config_container[ZONE_0]->GetInnerIter();
  nTimeIter  = config_container[ZONE_0]->GetnTime_Iter();
  MaxTime    = config_container[ZONE_0]->GetMax_Time();
  CurTime    = output_container[ZONE_0]->GetHistoryFieldValue("CUR_TIME");

  TimeDomain = config_container[ZONE_0]->GetTime_Domain();


  /*--- Check whether the inner solver has converged --- */

  if (TimeDomain == NO){

    InnerConvergence = output_container[ZONE_0]->GetConvergence();
    MaxIterationsReached = InnerIter+1 >= nInnerIter;

    if ((MaxIterationsReached || InnerConvergence) && (rank == MASTER_NODE)) {
      cout << "\n----------------------------- Solver Exit -------------------------------" << endl;
      if (output_container[ZONE_0]->GetConvergenceInterrupted())
        cout << "Interrupt signal received, exiting before the convergence criteria were satisfied." << endl;
      else if (InnerConvergence) cout << "All convergence criteria satisfied." << endl;
      else cout << "\nMaximum number of iterations reached (ITER = " << nInnerIter << ") before convergence." << endl;
      output_container[ZONE_0]->PrintConvergenceSummary();
      cout << "-------------------------------------------------------------------------" << endl;
    }

    StopCalc = MaxIterationsReached || InnerConvergence;
  }

  if (TimeDomain == YES) {

    /*--- Check whether the outer time integration has reached the final time ---*/

    TimeConvergence = GetTimeConvergence();

    FinalTimeReached = CurTime >= MaxTime;
    MaxIterationsReached = TimeIter+1 >= nTimeIter;

    if ((FinalTimeReached || MaxIterationsReached || TimeConvergence) && (rank == MASTER_NODE)){
      cout << "\n----------------------------- Solver Exit -------------------------------";
      if (TimeConvergence) cout << "\nAll windowed time-averaged convergence criteria are fullfilled." << endl;
      if (FinalTimeReached) cout << "\nMaximum time reached (MAX_TIME = " << MaxTime << "s)." << endl;
      if (MaxIterationsReached) cout << "\nMaximum number of time iterations reached (TIME_ITER = " << nTimeIter << ")." << endl;
      cout << "-------------------------------------------------------------------------" << endl;
    }
    StopCalc = FinalTimeReached || MaxIterationsReached || TimeConvergence;
  }

  /*--- Reset the inner convergence --- */

  output_container[ZONE_0]->SetConvergence(false);

  /*--- Increase the total iteration count --- */

  IterCount += config_container[ZONE_0]->GetInnerIter()+1;

  return StopCalc;
}

bool CSinglezoneDriver::GetTimeConvergence() const{
  SU2_ZONE_SCOPED
  return output_container[ZONE_0]->GetCauchyCorrectedTimeConvergence(config_container[ZONE_0]);
}
