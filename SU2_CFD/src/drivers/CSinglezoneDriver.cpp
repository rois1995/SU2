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
#include "../../include/integration/CMultiGridIntegration.hpp"
#include "../../include/adaptation/CSolutionTransfer.hpp"
#include "../../include/adaptation/CBarycentricTransfer.hpp"
#include "../../include/adaptation/CConservativeTransfer.hpp"
#include "../../include/adaptation/CMetricPredictor.hpp"
#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"

#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>

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
  if (config->GetAdap_Mesh_Output()) CheckAdaptedMeshNames();

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

    if (config->GetAdap_Mesh_Output()) WriteAdaptedMesh(iCycle + 1);

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

  if (config->GetKind_Adap_Unsteady_Metric() == ADAP_UNSTEADY_METRIC::PREDICT) {
    PredictWindowMetric();
    windowMetricTime += SU2_MPI::Wtime() - startTime;
    return;
  }

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

  if (windowSamples == 0) {
    windowHessian.assign(nPoint * nSensor * nMet, 0.0);
    windowMetricDone = false;
  }

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

  /*--- End of the window: metric of the mean |H| of its time steps, with the options of the loop. A fixed-point
   *    window ends at its last step (also the last, possibly shorter, window of the run). ---*/

  const bool windowEnd = fixedPointWindow ? TimeIter == fixedPointLastStep : config->GetAdap_TimeWindowEnd(TimeIter);
  if (windowEnd) {
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
    windowMetricDone = true;
  }

  windowMetricTime += SU2_MPI::Wtime() - startTime;
}

void CSinglezoneDriver::PredictWindowMetric() {
  SU2_ZONE_SCOPED

  auto* config = config_container[ZONE_0];
  auto* geometry = geometry_container[ZONE_0][INST_0][MESH_0];
  auto* solver_flow = solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];
  auto* nodes = solver_flow->GetNodes();

  const auto freq = config->GetAdap_Freq();
  const auto windowEnd = (TimeIter / freq + 1) * freq - 1;
  const auto snapshotStep = max(windowEnd - config->GetAdap_Predict_Separation(), windowFirstStep);
  const bool last = (TimeIter == windowEnd);
  if (TimeIter != snapshotStep && !last) return;

  const unsigned short nMet = nDim * (nDim + 1) / 2;
  const auto nPointDomain = geometry->GetnPointDomain();

  /*--- Metric of this time step (sensors, Hessians, Lp metric, complexity, bounds), without the boundary-layer
   *    metric, which is added to the final metric. ---*/

  if (rank == MASTER_NODE) {
    cout << endl << "----------------------------- Compute Metric ----------------------------" << endl;
    cout << "Metric snapshot of time step " << TimeIter << " for the prediction (ADAP_UNSTEADY_METRIC= PREDICT)."
         << endl;
  }
  solver_flow->SetAuxVar_Adapt(geometry, config);
  solver_flow->SetHessian_Adapt(geometry, config);
  solver_flow->ComputeMetric(geometry, config, nullptr, false);

  vector<su2double> snapshot(nPointDomain * nMet);
  for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint)
    for (unsigned short iMet = 0; iMet < nMet; ++iMet) snapshot[iPoint * nMet + iMet] = nodes->GetMetric(iPoint, iMet);

  if (!last) {
    predictSnapshot = std::move(snapshot);
    predictSnapshotStep = TimeIter;
    predictSnapshotValid = true;
    return;
  }

  /*--- End of the window: motion of the features between the two snapshots, metric moved over the horizon. The mesh,
   *    the snapshots, the flow velocity and the no-slip wall points are gathered on the master rank (global numbering
   *    of the mesh, CMeshGather), where the serial predictor runs; the predicted metric and the motion go back to the
   *    ranks that own the points. ---*/

  const auto startTime = SU2_MPI::Wtime();
  const bool twoSnapshots = predictSnapshotValid && predictSnapshotStep < TimeIter;
  const su2double separation = twoSnapshots ? su2double(TimeIter - predictSnapshotStep) : su2double(0.0);
  const su2double dt = config->GetDelta_UnstTimeND();

  vector<string> markerTags;
  for (unsigned short iMarker = 0; iMarker < geometry->GetnMarker(); ++iMarker)
    markerTags.push_back(config->GetMarker_All_TagBound(iMarker));
  const CMeshGather gather(*geometry);
  const auto mesh = gather.GatherMesh(*config, markerTags, false);

  /*--- Per point: snapshot j, snapshot k, flow velocity x dt, no-slip wall (no motion: the features there, boundary
   *    layers, are attached to the static wall). ---*/
  const unsigned short nIn = 2 * nMet + nDim + 1;
  vector<su2double> local(nPointDomain * nIn, 0.0);
  for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
    auto* row = &local[iPoint * nIn];
    for (unsigned short iMet = 0; iMet < nMet; ++iMet) {
      row[iMet] = twoSnapshots ? predictSnapshot[iPoint * nMet + iMet] : snapshot[iPoint * nMet + iMet];
      row[nMet + iMet] = snapshot[iPoint * nMet + iMet];
    }
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) row[2 * nMet + iDim] = nodes->GetVelocity(iPoint, iDim) * dt;
  }
  for (unsigned short iMarker = 0; iMarker < geometry->GetnMarker(); ++iMarker) {
    if (!config->GetViscous_Wall(iMarker)) continue;
    for (auto iVertex = 0ul; iVertex < geometry->GetnVertex(iMarker); ++iVertex) {
      const auto iPoint = geometry->vertex[iMarker][iVertex]->GetNode();
      if (iPoint < nPointDomain) local[iPoint * nIn + nIn - 1] = 1.0;
    }
  }
  const auto global = gather.Gather(local.data(), nIn);

  CMetricPredictor::MotionReport motionReport;
  CMetricPredictor::PredictionReport predictionReport;
  CMetricPredictor::Options options;
  options.anisoThreshold = config->GetAdap_Predict_Aniso();
  options.hmin = config->GetAdap_Hmin();
  options.hmax = config->GetAdap_Hmax();

  /*--- Instants of the horizon: 0, step, 2 step, ..., horizon. ---*/
  vector<su2double> instants;
  const auto horizon = config->GetAdap_Predict_Horizon(), step = config->GetAdap_Predict_Step();
  for (unsigned long t = 0; t < horizon; t += step) instants.push_back(t);
  instants.push_back(horizon);

  const unsigned short nOut = nMet + nDim;
  vector<su2double> result;  // per point: predicted metric, motion
  passivedouble motionTime = 0.0;
  if (gather.IsRoot()) {
    const auto nPointGlobal = gather.GetnPointGlobal();
    CMetricPredictor predictor(mesh);

    vector<su2double> metricK(nPointGlobal * nMet), motion(nPointGlobal * nDim, 0.0);
    for (auto iPoint = 0ul; iPoint < nPointGlobal; ++iPoint)
      for (unsigned short iMet = 0; iMet < nMet; ++iMet)
        metricK[iPoint * nMet + iMet] = global[iPoint * nIn + nMet + iMet];

    if (twoSnapshots) {
      vector<su2double> sj(nPointGlobal), sk(nPointGlobal), guess(nPointGlobal * nDim);
      vector<bool> wall(nPointGlobal, false);
      for (auto iPoint = 0ul; iPoint < nPointGlobal; ++iPoint) {
        const auto* row = &global[iPoint * nIn];
        sj[iPoint] = CMetricPredictor::Invariant(nDim, row);
        sk[iPoint] = CMetricPredictor::Invariant(nDim, row + nMet);
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) guess[iPoint * nDim + iDim] = row[2 * nMet + iDim];
        wall[iPoint] = row[nIn - 1] > 0.5;
      }
      motion = predictor.MotionField(sj, sk, separation, &guess, config->GetAdap_Predict_Regularization(),
                                     motionReport, &wall);
    }
    motionTime = SU2_TYPE::GetValue(SU2_MPI::Wtime() - startTime);

    const auto predicted = predictor.Predict(metricK, motion, instants, options, predictionReport);
    result.resize(nPointGlobal * nOut);
    for (auto iPoint = 0ul; iPoint < nPointGlobal; ++iPoint) {
      for (unsigned short iMet = 0; iMet < nMet; ++iMet) result[iPoint * nOut + iMet] = predicted[iPoint * nMet + iMet];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        result[iPoint * nOut + nMet + iDim] = motion[iPoint * nDim + iDim];
    }
  }

  vector<su2double> localResult(nPointDomain * nOut), predicted(nPointDomain * nMet);
  gather.Scatter(result, nOut, localResult.data());
  auto& motionOut = nodes->GetMetric_Motion();
  for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) {
    for (unsigned short iMet = 0; iMet < nMet; ++iMet)
      predicted[iPoint * nMet + iMet] = localResult[iPoint * nOut + iMet];
    if (motionOut.size() == 0) continue;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      motionOut(iPoint, iDim) = localResult[iPoint * nOut + nMet + iDim];
  }
  const auto predictTime = SU2_MPI::Wtime() - startTime;

  if (rank == MASTER_NODE) {
    cout << endl << "----------------------------- Compute Metric ----------------------------" << endl;
    if (twoSnapshots) {
      cout << "Predicted metric: motion of the metric features from time step " << predictSnapshotStep << " to "
           << TimeIter << " (optical flow on the mesh): feature width " << motionReport.featureLength
           << ", smoothing length " << motionReport.smoothLength << ", mismatch " << motionReport.mismatchBefore
           << " -> " << motionReport.mismatchAfter << " (started from "
           << (motionReport.guessKept ? "the flow velocity" : "zero motion");
      if (motionReport.mismatchOther >= 0.0) {
        cout << "; from " << (motionReport.guessKept ? "zero motion" : "the flow velocity") << ": "
             << motionReport.mismatchOther;
      }
      cout << "), speed of the features " << motionReport.meanSpeed << " per time step (largest "
           << motionReport.maxSpeed << "), " << motionReport.linearIterations << " CG iterations"
           << (motionReport.converged ? "" : " (some solves did not converge)") << "." << endl;
      if (motionReport.noMotion) {
        cout << "No motion found that matches the snapshots better than none: the metric of time step " << TimeIter
             << " is kept over the horizon." << endl;
      }
    } else {
      cout << "Predicted metric: only one time step of the window on this mesh, no motion (the metric of time step "
           << TimeIter << " is kept over the horizon)." << endl;
    }
    cout << "Metric of time step " << TimeIter << " moved over the next " << horizon << " time steps ("
         << predictionReport.nInstant << " instants, every " << step << "), reoriented where its anisotropy ratio > "
         << options.anisoThreshold << " (" << predictionReport.nCongruence << " point-instants), "
         << predictionReport.nOutside << " points traced out of the domain; intersection of the instants. Time "
         << predictTime << " s (motion " << motionTime << " s)." << endl;
  }

  /*--- Complexity, bounds, corner and boundary-layer metrics. ---*/
  solver_flow->ComputeMetric(geometry, config, &predicted);
  predictSnapshotValid = false;
}

void CSinglezoneDriver::PrepareTimeAdaptationRestart() {
  auto* config = config_container[ZONE_0];
  if (!config->GetRestart()) return;

  /*--- The adapted meshes of the run that wrote the files used MGCYCLE (a restart turned W_CYCLE into V_CYCLE). ---*/
  config->SetMGCycle_Adapted();

  if (config->GetUnst_CFL() == 0.0) return;

  /*--- The dual-time step of that run, from the meta data written with its restart files. ---*/
  const auto fileName = config->GetFilename("flow", ".meta", static_cast<int>(config->GetRestart_Iter()) - 1);
  const string key = "PHYSICAL_TIME_STEP_ND=";
  passivedouble timeStep = 0.0;
  bool found = false;
  if (rank == MASTER_NODE) {
    std::ifstream file(fileName);
    string line;
    while (std::getline(file, line)) {
      const auto position = line.find(key);
      if (position == string::npos) continue;
      timeStep = std::stod(line.substr(position + key.size()));
      found = timeStep > 0.0;
    }
  }
  unsigned short foundShort = found;
  SU2_MPI::Bcast(&foundShort, 1, MPI_UNSIGNED_SHORT, MASTER_NODE, SU2_MPI::GetComm());
  SU2_MPI::Bcast(&timeStep, 1, MPI_DOUBLE, MASTER_NODE, SU2_MPI::GetComm());

  if (foundShort) {
    config->SetDelta_UnstTimeND(timeStep);
    config->SetUnst_TimeStep_Kept(true);
    if (rank == MASTER_NODE) {
      cout << "Restart: dual-time step of the run " << timeStep * config->GetTime_Ref() << " s (from " << fileName
           << "), kept for every mesh." << endl;
    }
  } else if (rank == MASTER_NODE) {
    cout << "WARNING: no dual-time step (PHYSICAL_TIME_STEP_ND) in " << fileName << ": it is computed again from "
            "UNST_CFL_NUMBER on the restart mesh, so the restarted run does not continue with the time step of the run "
            "that wrote the files." << endl;
  }
}

void CSinglezoneDriver::KeepTimeStep() {
  auto* config = config_container[ZONE_0];
  if (config->GetUnst_CFL() == 0.0 || config->GetUnst_TimeStep_Kept()) return;
  config->SetUnst_TimeStep_Kept(true);
  if (rank == MASTER_NODE) {
    cout << "Dual-time step from UNST_CFL_NUMBER: " << config->GetDelta_UnstTimeND() * config->GetTime_Ref()
         << " s, kept for every mesh of the run (and written to the meta data of the restart files)." << endl;
  }
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
  output->WriteRestartMetaData(config);

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

  if (config->GetKind_Adap_Unsteady_Metric() == ADAP_UNSTEADY_METRIC::FIXED_POINT) {
    RunTimeFixedPointLoop();
    return;
  }

  /*--- Stop before the first time step if the problem or the build cannot be adapted. ---*/

  CheckMeshAdaptation();
  if (config->GetAdap_Mesh_Output()) CheckAdaptedMeshNames();

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
  PrepareTimeAdaptationRestart();

  StartTime = SU2_MPI::Wtime();
  config->Set_StartTime(StartTime);

  const unsigned long firstTimeIter = config->GetRestart() ? config->GetRestart_Iter() : 0;
  const auto nTimeIter = config->GetnTime_Iter();
  const auto freq = config->GetAdap_Freq();

  if (rank == MASTER_NODE) {
    cout << endl << "------------------------------ Begin Solver -----------------------------" << endl;
    cout << endl << "Simulation Run using the Single-zone Driver, time-domain mesh adaptation" << endl;
    cout << "The simulation will run for " << nTimeIter - firstTimeIter << " time steps." << endl;
    const bool predict = config->GetKind_Adap_Unsteady_Metric() == ADAP_UNSTEADY_METRIC::PREDICT;
    cout << "The mesh is adapted after every " << freq << " time steps (after the time steps n with (n+1) % " << freq
         << " == 0), from "
         << (predict ? "the metric of the last step moved over the next time steps with the motion of its features "
                       "(ADAP_UNSTEADY_METRIC= PREDICT)"
                     : "the mean |Hessian| of the sensors over those steps")
         << ": complexity " << config->GetAdap_Complexity() << ", sizes " << config->GetAdap_Hmin() << " to "
         << config->GetAdap_Hmax() << ", norm " << config->GetAdap_Norm() << ", aspect ratio up to "
         << config->GetAdap_ARmax() << "." << endl;
    if (predict) {
      cout << "Prediction: snapshots " << config->GetAdap_Predict_Separation() << " time steps apart (the second at "
           << "the end of the window), horizon " << config->GetAdap_Predict_Horizon() << " time steps every "
           << config->GetAdap_Predict_Step() << ", reoriented where the anisotropy ratio > "
           << config->GetAdap_Predict_Aniso() << ", regularization " << config->GetAdap_Predict_Regularization()
           << "." << endl;
    }
    if (conservative) {
      cout << "Solution transfer: conservative P1 projection of the solution and of its time history (the integrals "
              "of the conservative variables are kept)";
    } else {
      cout << "Solution transfer: barycentric (P1) interpolation of the solution and of its time history (the "
              "integrals of the conservative variables are not conserved)";
    }
    cout << (config->GetAdap_Transfer_Default() ? ", the default of time-domain runs." : ".") << endl;
    if (config->GetAdap_Mesh_Output() && !config->GetWrt_Adap_Mesh()) {
      cout << "The restart files of the steps n (and n-1) are written again on each new mesh, so each mesh is written "
              "too (needed to restart from them), although WRT_ADAP_MESH= NO." << endl;
    }
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
  windowFirstStep = firstTimeIter;
  predictSnapshotValid = false;
  auto cycleStart = SU2_MPI::Wtime();

  /*--- The time loop of RunTimeLoop, with the adaptation after the steps that end a time window. ---*/

  TimeIter = firstTimeIter;
  while (TimeIter < nTimeIter) {

    Preprocess(TimeIter);
    Run();
    KeepTimeStep();
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

      /*--- Files of the new mesh: the mesh (named for its first time step), then the restart files of the
       *    transferred steps n and n-1, which replace those of the previous mesh. The mesh is written first and always
       *    when restart files are (CConfig::GetAdap_Mesh_Output): the rewritten restart files can only be used with
       *    it, and the previous checkpoint (the old mesh with its files) is no longer complete once they are. ---*/

      const auto outputStart = SU2_MPI::Wtime();
      if (config->GetAdap_Mesh_Output()) WriteAdaptedMesh(iCycle, TimeIter + 1);
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
      windowFirstStep = TimeIter + 1;
      predictSnapshotValid = false;
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

CSinglezoneDriver::CTimeWindowState CSinglezoneDriver::SaveTimeWindowState() const {
  SU2_ZONE_SCOPED

  const auto* config = config_container[ZONE_0];
  const auto nMGLevels = config->GetnMGLevels();
  CTimeWindowState state;
  state.timeIter = config->GetTimeIter();

  for (unsigned short iMesh = 0; iMesh <= nMGLevels; iMesh++) {
    for (unsigned short iSol = 0; iSol < MAX_SOLS; iSol++) {
      auto* solver = solver_container[ZONE_0][INST_0][iMesh][iSol];
      if (solver == nullptr || solver->GetNodes() == nullptr) continue;
      auto* nodes = solver->GetNodes();
      CTimeWindowState::Arrays arrays;
      arrays.iMesh = iMesh;
      arrays.iSol = iSol;
      arrays.solution = nodes->GetSolution();
      arrays.timeN = nodes->GetSolution_time_n();
      arrays.timeN1 = nodes->GetSolution_time_n1();
      const auto nPoint = arrays.solution.rows(), nVar = arrays.solution.cols();
      arrays.solutionOld.resize(nPoint, nVar);
      arrays.localCFL.resize(nPoint);
      for (auto iPoint = 0ul; iPoint < nPoint; iPoint++) {
        for (auto iVar = 0ul; iVar < nVar; iVar++) arrays.solutionOld(iPoint, iVar) = nodes->GetSolution_Old(iPoint, iVar);
        arrays.localCFL(iPoint) = nodes->GetLocalCFL(iPoint);
      }
      if (iSol == FLOW_SOL) arrays.primitive = nodes->GetPrimitive();
      if (iSol == TURB_SOL) {
        arrays.muT.resize(nPoint);
        for (auto iPoint = 0ul; iPoint < nPoint; iPoint++) arrays.muT(iPoint) = nodes->GetmuT(iPoint);
      }
      state.solvers.push_back(std::move(arrays));
    }
  }

  for (unsigned short iSol = 0; iSol < MAX_SOLS; iSol++) {
    const auto* integration = dynamic_cast<const CMultiGridIntegration*>(integration_container[ZONE_0][INST_0][iSol]);
    state.integration.push_back(integration ? std::make_shared<CMultiGridIntegration>(*integration) : nullptr);
  }

  for (unsigned short iMesh = 0; iMesh <= nMGLevels; iMesh++) state.CFL.push_back(config->GetCFL(iMesh));
  state.dampResRestric = config->GetDamp_Res_Restric();
  state.dampCorrecProlong = config->GetDamp_Correc_Prolong();
  state.AoA = config->GetAoA();
  state.AoS = config->GetAoS();
  state.finestMesh = config->GetFinestMesh();
  state.timeStepND = config->GetDelta_UnstTimeND();
  state.timeStepKept = config->GetUnst_TimeStep_Kept();
  state.output = output_container[ZONE_0]->GetTimeState();
  return state;
}

void CSinglezoneDriver::RestoreTimeWindowState(const CTimeWindowState& state) {
  SU2_ZONE_SCOPED

  auto* config = config_container[ZONE_0];
  const auto nMGLevels = config->GetnMGLevels();
  if (state.CFL.size() != nMGLevels + 1u) {
    SU2_MPI::Error("The saved state of the time window belongs to another mesh.", CURRENT_FUNCTION);
  }

  for (const auto& arrays : state.solvers) {
    auto* solver = solver_container[ZONE_0][INST_0][arrays.iMesh][arrays.iSol];
    if (solver == nullptr || solver->GetNodes() == nullptr ||
        solver->GetNodes()->GetSolution().rows() != arrays.solution.rows() ||
        solver->GetNodes()->GetSolution().cols() != arrays.solution.cols()) {
      SU2_MPI::Error("The saved state of the time window belongs to another mesh.", CURRENT_FUNCTION);
    }
    auto* nodes = solver->GetNodes();
    nodes->GetSolution() = arrays.solution;
    nodes->GetSolution_time_n() = arrays.timeN;
    nodes->GetSolution_time_n1() = arrays.timeN1;
    const auto nPoint = arrays.solution.rows(), nVar = arrays.solution.cols();
    for (auto iPoint = 0ul; iPoint < nPoint; iPoint++) {
      for (auto iVar = 0ul; iVar < nVar; iVar++) nodes->SetSolution_Old(iPoint, iVar, arrays.solutionOld(iPoint, iVar));
      nodes->SetLocalCFL(iPoint, arrays.localCFL(iPoint));
    }
    for (auto iPoint = 0ul; iPoint < arrays.primitive.rows(); iPoint++)
      for (auto iVar = 0ul; iVar < arrays.primitive.cols(); iVar++)
        nodes->SetPrimitive(iPoint, iVar, arrays.primitive(iPoint, iVar));
    for (auto iPoint = 0ul; iPoint < arrays.muT.size(); iPoint++) nodes->SetmuT(iPoint, arrays.muT(iPoint));
  }

  for (unsigned short iSol = 0; iSol < MAX_SOLS && iSol < state.integration.size(); iSol++) {
    auto* integration = dynamic_cast<CMultiGridIntegration*>(integration_container[ZONE_0][INST_0][iSol]);
    if ((integration == nullptr) != (state.integration[iSol] == nullptr)) {
      SU2_MPI::Error("The saved state of the time window belongs to other solvers.", CURRENT_FUNCTION);
    }
    if (integration != nullptr) *integration = *state.integration[iSol];
  }

  for (unsigned short iMesh = 0; iMesh <= nMGLevels; iMesh++) config->SetCFL(iMesh, state.CFL[iMesh]);
  config->SetDamp_Res_Restric(state.dampResRestric);
  config->SetDamp_Correc_Prolong(state.dampCorrecProlong);
  config->SetAoA(state.AoA);
  config->SetAoS(state.AoS);
  config->SetFinestMesh(state.finestMesh);
  /*--- The dual-time step of the run (a state saved before it was established keeps the current one). ---*/
  if (state.timeStepKept || !config->GetUnst_TimeStep_Kept()) config->SetDelta_UnstTimeND(state.timeStepND);
  config->SetTimeIter(state.timeIter);
  TimeIter = state.timeIter;
  output_container[ZONE_0]->SetTimeState(state.output);
}

unsigned long CSinglezoneDriver::SolveTimeWindow(unsigned long first, unsigned long last, bool accepted) {
  SU2_ZONE_SCOPED

  auto* output = output_container[ZONE_0];

  /*--- The window metric (mean |H|) is accumulated over these steps and computed at the last one. ---*/
  windowSamples = 0;
  windowMetricDone = false;
  fixedPointWindow = true;
  fixedPointLastStep = last;

  /*--- A discarded solve writes no file at all (not even the files that solver controllers write during the inner
   *    iterations); its screen output stays. ---*/
  output->SetFileWriting(accepted);

  auto lastSolved = last;
  for (auto timeIter = first; timeIter <= last; timeIter++) {
    Preprocess(timeIter);
    Run();
    KeepTimeStep();
    Postprocess();
    Update();
    if (accepted) {
      Monitor(timeIter);
      Output(timeIter);
      if (StopCalc) {
        lastSolved = timeIter;
        break;
      }
    } else {
      output->SetConvergence(false);
    }
  }
  output->SetFileWriting(true);
  return lastSolved;
}

std::pair<passivedouble, passivedouble> CSinglezoneDriver::MetricChange(const vector<passivedouble>& meshMetric) const {
  const auto* geometry = geometry_container[ZONE_0][INST_0][MESH_0];
  const auto* nodes = solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]->GetNodes();
  const unsigned short nMet = nDim * (nDim + 1) / 2;

  passivedouble sums[3] = {0.0, 0.0, 0.0};  // |log ratio|, edges changed by more than 2x, edges
  /*--- The global indices are those of the domain points of all ranks (GetGlobal_nPoint counts the halos too). ---*/
  bool fits = meshMetric.size() == geometry->GetGlobal_nPointDomain() * nMet;
  for (auto iPoint = 0ul; fits && iPoint < geometry->GetnPoint(); iPoint++)
    fits = geometry->nodes->GetGlobalIndex(iPoint) < geometry->GetGlobal_nPointDomain();

  if (fits) {
    /*--- Length of the edge e in the metric at a point: sqrt(e^T M e), M stored as its upper triangle. ---*/
    auto length = [&](const su2double* e, auto metric) {
      passivedouble q = 0.0;
      for (unsigned short iDim = 0, iMet = 0; iDim < nDim; iDim++) {
        for (unsigned short jDim = iDim; jDim < nDim; jDim++, iMet++) {
          q += (iDim == jDim ? 1.0 : 2.0) * metric(iMet) * SU2_TYPE::GetValue(e[iDim] * e[jDim]);
        }
      }
      return sqrt(std::max(q, 0.0));
    };
    for (auto iEdge = 0ul; iEdge < geometry->GetnEdge(); iEdge++) {
      const auto iPoint = geometry->edges->GetNode(iEdge, 0), jPoint = geometry->edges->GetNode(iEdge, 1);
      /*--- Each edge once over the ranks: on the rank that owns its point of smaller global index. ---*/
      const bool iFirst = geometry->nodes->GetGlobalIndex(iPoint) < geometry->nodes->GetGlobalIndex(jPoint);
      const auto owner = iFirst ? iPoint : jPoint;
      if (!geometry->nodes->GetDomain(owner)) continue;
      su2double e[3] = {0.0};
      for (unsigned short iDim = 0; iDim < nDim; iDim++)
        e[iDim] = geometry->nodes->GetCoord(jPoint, iDim) - geometry->nodes->GetCoord(iPoint, iDim);
      passivedouble lengthNew = 0.0, lengthOld = 0.0;
      for (const auto point : {iPoint, jPoint}) {
        const auto* old = &meshMetric[geometry->nodes->GetGlobalIndex(point) * nMet];
        lengthNew += 0.5 * length(e, [&](unsigned short iMet) { return SU2_TYPE::GetValue(nodes->GetMetric(point, iMet)); });
        lengthOld += 0.5 * length(e, [&](unsigned short iMet) { return old[iMet]; });
      }
      if (!(lengthNew > 0.0) || !(lengthOld > 0.0)) continue;
      const auto change = fabs(log(lengthNew / lengthOld));
      sums[0] += change;
      if (change > log(2.0)) sums[1] += 1.0;
      sums[2] += 1.0;
    }
  }
  /*--- Reduced as su2double: MPI_DOUBLE is the active type of the MeDiPack wrapper in the AD and DD builds. ---*/
  su2double local[3] = {sums[0], sums[1], sums[2]}, reduced[3] = {0.0, 0.0, 0.0};
  SU2_MPI::Allreduce(local, reduced, 3, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
  const passivedouble global[3] = {SU2_TYPE::GetValue(reduced[0]), SU2_TYPE::GetValue(reduced[1]),
                                   SU2_TYPE::GetValue(reduced[2])};
  unsigned short allFit = fits, globalFit = 0;
  SU2_MPI::Allreduce(&allFit, &globalFit, 1, MPI_UNSIGNED_SHORT, MPI_MIN, SU2_MPI::GetComm());
  if (!globalFit || global[2] == 0.0) return {-1.0, 0.0};
  return {global[0] / global[2], global[1] / global[2]};
}

void CSinglezoneDriver::RunTimeFixedPointLoop() {
  SU2_ZONE_SCOPED

  auto* config = config_container[ZONE_0];
  auto* output = output_container[ZONE_0];

  /*--- Stop before the first time step if the problem or the build cannot be adapted. ---*/

  CheckMeshAdaptation();
  if (config->GetAdap_Mesh_Output()) CheckAdaptedMeshNames();

  CMMGRemesher remesher;

  const bool conservative = config->GetKind_Adap_Transfer() == ADAP_TRANSFER::CONSERVATIVE;
  std::unique_ptr<CSolutionTransfer> transferPtr;
  if (conservative) {
    transferPtr = std::make_unique<CConservativeTransfer>();
  } else {
    transferPtr = std::make_unique<CBarycentricTransfer>();
  }
  CFreeStreamTransfer initialCondition;

  config->SetAdap_MetricLevel(0);
  PrepareTimeAdaptationRestart();

  StartTime = SU2_MPI::Wtime();
  config->Set_StartTime(StartTime);

  const bool restart = config->GetRestart();
  const unsigned long firstTimeIter = restart ? config->GetRestart_Iter() : 0;
  const auto nTimeIter = config->GetnTime_Iter();
  const auto freq = config->GetAdap_Freq();
  const auto nIterFP = config->GetAdap_FP_Iter();
  const auto tolFP = SU2_TYPE::GetValue(config->GetAdap_FP_Tol());

  if (rank == MASTER_NODE) {
    cout << endl << "------------------------------ Begin Solver -----------------------------" << endl;
    cout << endl << "Simulation Run using the Single-zone Driver, time-domain mesh adaptation (fixed point)" << endl;
    cout << "The simulation will run for " << nTimeIter - firstTimeIter << " time steps." << endl;
    cout << "Fixed point per time window of " << freq << " time steps (from step 0): the window is solved on the current "
            "mesh, remeshed from the mean |Hessian| of the sensors over its steps (complexity "
         << config->GetAdap_Complexity() << ", sizes " << config->GetAdap_Hmin() << " to " << config->GetAdap_Hmax()
         << ", norm " << config->GetAdap_Norm() << ", aspect ratio up to " << config->GetAdap_ARmax()
         << ") and solved again from its start state on the new mesh, " << nIterFP << " time(s) (ADAP_FP_ITER), or "
         << "until the metric of a re-solve differs from the one before by less than " << tolFP << " (ADAP_FP_TOL). "
         << "Only the last solve of each window writes files and history rows." << endl;
    if (restart) {
      cout << "Restart: the first window (from time step " << firstTimeIter << ") is solved once on the restart mesh, "
              "the accepted mesh of that window." << endl;
    } else {
      cout << "The first window starts every mesh from the initial condition of the run." << endl;
    }
    if (conservative) {
      cout << "Solution transfer of the start state of a window: conservative P1 projection of the solution and of its "
              "time history";
    } else {
      cout << "Solution transfer of the start state of a window: barycentric (P1) interpolation of the solution and of "
              "its time history";
    }
    cout << (config->GetAdap_Transfer_Default() ? ", the default of time-domain runs." : ".") << endl;
  }

  struct WindowSummary {
    unsigned long cycle = 0, firstStep = 0, lastStep = 0, nSolve = 0, nPoint = 0;
    vector<unsigned long> points;
    vector<passivedouble> changes, solveTimes;
    passivedouble finalChange = -1.0, remeshTime = 0.0, replaceTime = 0.0, transferTime = 0.0, outputTime = 0.0,
                  metricTime = 0.0, defect = -1.0;
  };
  vector<WindowSummary> summary;

  /*--- Metric the current mesh was built from (MMG interpolates it to its points), by global point index; empty for the
   *    input mesh and a restart mesh. ---*/
  vector<passivedouble> meshMetric;
  auto nPointGlobal = [&]() { return geometry_container[ZONE_0][INST_0][MESH_0]->GetGlobal_nPointDomain(); };
  auto changeString = [](passivedouble change) {
    if (change < 0.0) return string("-");
    std::ostringstream out;
    out << std::setprecision(3) << change;
    return out.str();
  };

  bool firstWindow = true;
  TimeIter = firstTimeIter;
  while (TimeIter < nTimeIter) {

    const auto first = TimeIter;
    const auto last = std::min((first / freq + 1) * freq - 1, nTimeIter - 1);
    const auto iCycle = first / freq;
    output->SetAdaptationCycle(iCycle, 0, false);

    WindowSummary row;
    row.cycle = iCycle;
    row.firstStep = first;

    /*--- First window of a restart: the restart mesh is the accepted mesh of this window. ---*/

    if (firstWindow && restart) {
      firstWindow = false;
      if (rank == MASTER_NODE) {
        cout << endl << "------------------------- Fixed-Point Iteration -------------------------" << endl;
        cout << "Time steps " << first << " to " << last << " (cycle " << iCycle << ") on the restart mesh ("
             << nPointGlobal() << " points), solved once: accepted (files and history)." << endl;
      }
      const auto solveStart = SU2_MPI::Wtime();
      windowMetricTime = 0.0;
      row.lastStep = SolveTimeWindow(first, last, true);
      row.nSolve = 1;
      row.nPoint = nPointGlobal();
      row.points.push_back(row.nPoint);
      row.solveTimes.push_back(SU2_TYPE::GetValue(SU2_MPI::Wtime() - solveStart - windowMetricTime));
      row.metricTime = SU2_TYPE::GetValue(windowMetricTime);
      summary.push_back(row);
      if (StopCalc) break;
      TimeIter = last + 1;
      continue;
    }
    firstWindow = false;

    /*--- The first window of a new run starts each mesh from the initial condition (as a run on that mesh would), the
     *    other windows from their start state, saved here and kept on this mesh as the donor of the transfers. ---*/

    const bool initial = (first == 0 && !restart);
    const auto start = SaveTimeWindowState();
    CMeshDonor kept;
    bool haveKept = false;
    CSimplexMesh nextMesh;
    passivedouble previousChange = -1.0;

    for (unsigned long iIter = 0;; iIter++) {

      /*--- This solve is the accepted one after ADAP_FP_ITER remeshes, or when the metric of the previous re-solve
       *    differed from the one before it by less than ADAP_FP_TOL (its mesh barely changes). ---*/
      const bool accepted = (iIter == nIterFP) || (iIter >= 2 && previousChange >= 0.0 && previousChange < tolFP);

      if (iIter > 0) {
        const auto adaptStart = SU2_MPI::Wtime();
        TimeIter = (first > 0) ? first - 1 : 0;
        config->SetTimeIter(TimeIter);
        config->SetSolutionInMemory();

        if (initial) {
          ReplaceMesh(nextMesh, initialCondition, nullptr, nullptr);
        } else if (!haveKept) {
          /*--- The window start state goes back into the solvers of this mesh, which become the donor. ---*/
          RestoreTimeWindowState(start);
          ReplaceMesh(nextMesh, *transferPtr, nullptr, &kept);
          haveKept = true;
        } else {
          ReplaceMesh(nextMesh, *transferPtr, &kept, nullptr);
        }
        meshMetric = std::move(nextMesh.metric);
        nextMesh = CSimplexMesh();
        output->SetVolumeAverageStart(first, config);
        row.replaceTime += SU2_TYPE::GetValue(lastReplaceTime);
        row.transferTime += SU2_TYPE::GetValue(lastTransferTime);
        if (!initial) row.defect = transferPtr->GetReport().conservationDefect;

        /*--- Files of the accepted mesh: the mesh (named for its first time step), then the restart files of the
         *    transferred steps n and n-1 (they replace those of the previous mesh), as in the WINDOW_AVERAGE loop. ---*/
        if (accepted) {
          const auto outputStart = SU2_MPI::Wtime();
          if (config->GetAdap_Mesh_Output()) WriteAdaptedMesh(iCycle, first);
          if (first > 0) WriteTimeHistoryRestarts();
          row.outputTime = SU2_TYPE::GetValue(SU2_MPI::Wtime() - outputStart);
          UsedTimeOutput += row.outputTime;
        }
        UsedTimePreproc += SU2_MPI::Wtime() - adaptStart - row.outputTime;
      }

      if (rank == MASTER_NODE) {
        cout << endl << "------------------------- Fixed-Point Iteration -------------------------" << endl;
        cout << "Time steps " << first << " to " << last << " (cycle " << iCycle << "), solve " << iIter + 1 << " on "
             << (iIter == 0 ? (meshMetric.empty() ? "the current mesh" : "the mesh of the previous window")
                            : "the mesh of solve " + to_string(iIter))
             << " (" << nPointGlobal() << " points): "
             << (accepted ? "accepted (files and history)." : "discarded (screen output only).") << endl;
      }

      windowMetricTime = 0.0;
      StartTime = SU2_MPI::Wtime();
      config->Set_StartTime(StartTime);
      const auto solveStart = StartTime;
      const auto lastSolved = SolveTimeWindow(first, last, accepted);
      if (!accepted) UsedTimeCompute += SU2_MPI::Wtime() - StartTime;

      row.nSolve++;
      row.points.push_back(nPointGlobal());
      row.solveTimes.push_back(SU2_TYPE::GetValue(SU2_MPI::Wtime() - solveStart - windowMetricTime));
      row.metricTime += SU2_TYPE::GetValue(windowMetricTime);

      /*--- Change of the metric of this solve from the metric this mesh was built from. ---*/
      const auto change = windowMetricDone ? MetricChange(meshMetric) : std::make_pair(-1.0, 0.0);
      if (rank == MASTER_NODE && change.first >= 0.0) {
        cout << "Metric change from the metric of the mesh: mean |log(edge length ratio)| " << change.first << ", "
             << 100.0 * change.second << "% of the edges change by more than a factor 2." << endl;
      }

      if (accepted) {
        row.lastStep = lastSolved;
        row.finalChange = change.first;
        break;
      }
      row.changes.push_back(change.first);
      if (iIter >= 1) previousChange = change.first;

      /*--- Discarded: back to the output state of the window start, new mesh from the metric of this solve. ---*/
      output->SetTimeState(start.output);
      const auto remeshStart = SU2_MPI::Wtime();
      nextMesh = remesher.Remesh(*config, *geometry_container[ZONE_0][INST_0][MESH_0],
                                 solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]->GetNodes()->GetMetric());
      row.remeshTime += SU2_TYPE::GetValue(SU2_MPI::Wtime() - remeshStart);
      UsedTimePreproc += SU2_MPI::Wtime() - remeshStart;
    }

    if (haveKept) ReleaseMesh(kept);

    row.nPoint = nPointGlobal();
    if (rank == MASTER_NODE) {
      cout << "Fixed point of time steps " << first << " to " << row.lastStep << ": " << row.nSolve << " solves on";
      for (const auto n : row.points) cout << " " << n;
      cout << " points; metric change";
      for (const auto c : row.changes) cout << " " << changeString(c);
      cout << "; after the accepted solve " << changeString(row.finalChange) << "." << endl;
    }
    summary.push_back(row);

    if (StopCalc) break;
    TimeIter = last + 1;
  }
  fixedPointWindow = false;

  if (rank == MASTER_NODE) {
    cout << endl << "------------------------ Mesh Adaptation Summary ------------------------" << endl;
    PrintingToolbox::CTablePrinter table(&cout);
    table.AddColumn("Cycle", 6);
    table.AddColumn("Time steps", 12);
    table.AddColumn("Solves", 7);
    table.AddColumn("Points", 8);
    table.AddColumn("Change", 14);
    table.AddColumn("Final", 7);
    table.AddColumn("Solve [s]", 10);
    table.AddColumn("Accepted [s]", 12);
    table.AddColumn("Remesh [s]", 10);
    table.AddColumn("Replace [s]", 11);
    table.AddColumn("Output [s]", 10);
    table.AddColumn("Cons. defect", 12);
    table.PrintHeader();
    passivedouble total[6] = {0.0};
    unsigned long nSolve = 0;
    for (const auto& row : summary) {
      string changes;
      for (const auto c : row.changes) changes += (changes.empty() ? "" : "/") + changeString(c);
      passivedouble solveTime = 0.0;
      for (const auto t : row.solveTimes) solveTime += t;
      table << row.cycle << to_string(row.firstStep) + "-" + to_string(row.lastStep) << row.nSolve << row.nPoint
            << (changes.empty() ? string("-") : changes) << changeString(row.finalChange) << solveTime
            << row.solveTimes.back() << row.remeshTime << row.replaceTime << row.outputTime;
      if (row.defect >= 0.0) table << row.defect;
      else table << "-";
      const passivedouble values[] = {solveTime, row.solveTimes.back(), row.metricTime, row.remeshTime,
                                      row.replaceTime, row.outputTime};
      for (int k = 0; k < 6; ++k) total[k] += values[k];
      nSolve += row.nSolve;
    }
    table.PrintFooter();
    cout << "Total: " << nSolve << " window solves over " << summary.size() << " windows, solve " << total[0]
         << " s (accepted " << total[1] << " s), metric " << total[2] << " s, remesh " << total[3] << " s, replace "
         << total[4] << " s, adapted mesh and restart output " << total[5] << " s." << endl;
    cout << "Solves: solves of the window (the last one accepted). Change: metric of each discarded solve against the "
            "metric its mesh was built from (mean |log| of the edge length ratio; - unknown: input or restart mesh). "
            "Final: the same after the accepted solve. Solve: all solves of the window with their output, without the "
            "metric; Accepted: the last one. Remesh: extraction, MMG and validation. Replace: new geometry and solvers "
            "with the transfer. Output: adapted mesh and restart files of the transferred start. Cons. defect: largest "
            "relative change of the integrals of the conservative flow variables in the transfer." << endl;
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
    /*--- The fixed-point loop also adapts the mesh of the first window of a new run. ---*/
    if (!config->GetRestart() && config->GetKind_Adap_Unsteady_Metric() == ADAP_UNSTEADY_METRIC::FIXED_POINT) check(0, 0);
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
  ReplaceMesh(mesh, transfer, nullptr, nullptr);
}

void CSinglezoneDriver::ReleaseMesh(CMeshDonor& mesh) {
  if (mesh.solver != nullptr) FinalizeSolver(mesh.solver, mesh.nMGLevels);
  if (mesh.geometry != nullptr) {
    for (unsigned short iMesh = 0; iMesh <= mesh.nMGLevels; iMesh++) delete mesh.geometry[iMesh];
    delete [] mesh.geometry;
    mesh.geometry = nullptr;
  }
}

void CSinglezoneDriver::ReplaceMesh(const CSimplexMesh& mesh, CSolutionTransfer& transfer,
                                    const CMeshDonor* keptDonor, CMeshDonor* keepCurrent) {
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

  /*--- The objects of the current mesh (the donor, unless another one is given) stay in use until the new ones are
   *    complete. The config describes the current mesh only until the new geometry is built, keep what the donor
   *    needs from it. ---*/

  CMeshDonor current;
  current.geometry = geometry_container[ZONE_0][INST_0];
  current.solver = solver_container[ZONE_0][INST_0];
  current.nMGLevels = config->GetnMGLevels();
  for (unsigned short iMarker = 0; iMarker < current.geometry[MESH_0]->GetnMarker(); iMarker++)
    current.markerTags.push_back(config->GetMarker_All_TagBound(iMarker));
  const CMeshDonor& donor = (keptDonor != nullptr) ? *keptDonor : current;

  /*--- Start from the config state of a fresh run (requested multigrid levels, their CFL, damping, ...). ---*/

  initialRunState.Restore(*config);

  /*--- Geometry, as in InitializeGeometry. The orientation of the remeshed elements is always checked. ---*/

  CGeometry** geometry = nullptr;
  {
    CMemoryMeshReaderFVM reader(config, mesh, ZONE_0, nZone);
    BuildGeometryFVM(config, new CPhysicalGeometry(config, reader, nZone), geometry, true);
  }
  const auto nMGLevels = config->GetnMGLevels();

  /*--- Every marker of the new mesh with boundary elements is held, under its name, by some rank. Each rank holds the
   *    markers of its partition only, in an order of its own (Marker_All_*), so everything that crosses the ranks
   *    (the gathers of the solution transfers and of the next remesh, the mesh output) goes by marker name. ---*/
  {
    vector<string> tags, expected;
    for (unsigned short iMarker = 0; iMarker < geometry[MESH_0]->GetnMarker(); iMarker++)
      tags.push_back(config->GetMarker_All_TagBound(iMarker));
    auto names = CMeshGather::GatherMarkerNames(*config, tags, *geometry[MESH_0]);
    for (const auto& marker : mesh.markers)
      if (!marker.elem.empty()) expected.push_back(marker.name);
    std::sort(names.begin(), names.end());
    std::sort(expected.begin(), expected.end());
    if (names != expected) {
      string list;
      for (const auto& name : expected) list += " " + name;
      SU2_MPI::Error("The markers of the new geometry differ from those of the new mesh (" + list + ").",
                     CURRENT_FUNCTION);
    }
  }

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

  /*--- The new flow solvers set the dual-time step from TIME_STEP: the run keeps its own (computed from
   *    UNST_CFL_NUMBER at its first time step, then kept). ---*/
  const auto timeStepND = config->GetDelta_UnstTimeND();
  const bool timeDomain = config->GetTime_Domain();
  if (timeDomain && config->GetUnst_CFL() != 0.0) config->SetUnst_TimeStep_Kept(true);

  CSolver*** solver = nullptr;
  InitializeSolver(config, geometry, solver, false);
  if (timeDomain) config->SetDelta_UnstTimeND(timeStepND);

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

  /*--- Release the previous mesh, with its own number of levels (its geometry and solvers may be kept as the donor
   *    of later transfers). ---*/

  FinalizeNumerics(donorNumerics, current.nMGLevels);
  FinalizeIntegration(donorIntegration);
  delete donorIteration;
  if (keepCurrent != nullptr) *keepCurrent = current;
  else ReleaseMesh(current);

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
