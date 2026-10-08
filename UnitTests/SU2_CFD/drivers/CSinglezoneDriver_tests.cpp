/*!
 * \file CSinglezoneDriver_tests.cpp
 * \brief Unit tests of the single-zone driver: state of a time window (fixed-point mesh adaptation).
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

#include "catch.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>

#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../Common/adaptation/SimplexMeshTestCase.hpp"

namespace {

/*--- Access to the protected parts of the driver that the adaptation loops use. ---*/
class TestDriver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::CSinglezoneDriver;
  using CSinglezoneDriver::SolveTimeWindow;
  CConfig* Config() { return config_container[ZONE_0]; }
  COutput* Output() { return output_container[ZONE_0]; }
  CGeometry* Geometry() { return geometry_container[ZONE_0][INST_0][MESH_0]; }
  CSolver* FlowSolver() { return solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]; }
};

/*--- Number of entries of two containers that differ (bit for bit). ---*/
template <class Container>
unsigned long CountDifferent(const Container& a, const Container& b) {
  if (a.size() != b.size()) return std::max<unsigned long>(a.size(), b.size()) + 1;
  unsigned long n = 0;
  for (auto i = 0ul; i < a.size(); ++i) n += (a.data()[i] != b.data()[i]);
  return n;
}

/*--- Solve a few time steps, save the state, solve a window, restore the state and solve the
 *    window again: the second solve must give the same state as the first, bit for bit. ---*/
void CheckSameMeshResolve(const std::string& name, const std::string& options) {
  const std::string cfgName = name + ".cfg";
  {
    std::ofstream cfg(cfgName);
    cfg << "MACH_NUMBER= 0.5\nAOA= 0.0\n"
           "FREESTREAM_DENSITY= 1.0\nFREESTREAM_PRESSURE= 1.0\nFREESTREAM_TEMPERATURE= 1.0\n"
           "FLUID_MODEL= IDEAL_GAS\nGAMMA_VALUE= 1.4\nGAS_CONSTANT= 1.0\nREF_DIMENSIONALIZATION= DIMENSIONAL\n"
           "INIT_OPTION= TD_CONDITIONS\n"
           "MESH_FORMAT= RECTANGLE\nMESH_BOX_SIZE= (17, 17, 0)\nMESH_BOX_LENGTH= (1.0, 1.0, 0.0)\n"
           "MESH_BOX_OFFSET= (-1.0, -0.5, 0.0)\n"
           "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 0.01\nTIME_ITER= 10\n"
           "INNER_ITER= 4\nNUM_METHOD_GRAD= GREEN_GAUSS\nCFL_NUMBER= 20\nMGCYCLE= V_CYCLE\nMG_MIN_MESHSIZE= 10\n"
           "LINEAR_SOLVER= FGMRES\nLINEAR_SOLVER_PREC= ILU\nLINEAR_SOLVER_ITER= 5\nCONV_RESIDUAL_MINVAL= -14\n"
           "OUTPUT_FILES= (RESTART)\nSCREEN_WRT_FREQ_INNER= 1000\nCONV_FILENAME= " << name << "_history\n"
        << options;
  }

  auto* origBuf = std::cout.rdbuf(nullptr);
  {
    CSinglezoneDriver driver(const_cast<char*>(cfgName.c_str()), 1, SU2_MPI::GetComm());
    auto step = [&](unsigned long timeIter) {
      driver.Preprocess(timeIter);
      driver.Run();
      driver.Postprocess();
      driver.Update();
    };
    for (unsigned long timeIter = 0; timeIter < 2; timeIter++) step(timeIter);
    const auto start = driver.SaveTimeWindowState();
    for (unsigned long timeIter = 2; timeIter < 5; timeIter++) step(timeIter);
    const auto first = driver.SaveTimeWindowState();
    driver.RestoreTimeWindowState(start);
    for (unsigned long timeIter = 2; timeIter < 5; timeIter++) step(timeIter);
    const auto second = driver.SaveTimeWindowState();
    std::cout.rdbuf(origBuf);

    /*--- Coarse levels and the solvers of the case are in the state. ---*/
    REQUIRE(first.solvers.size() == second.solvers.size());
    REQUIRE(first.solvers.size() == start.solvers.size());
    unsigned short nLevel = 0;
    for (const auto& arrays : start.solvers) nLevel = std::max<unsigned short>(nLevel, arrays.iMesh + 1);
    CHECK(nLevel == 3);

    for (auto i = 0ul; i < first.solvers.size(); ++i) {
      const auto &a = first.solvers[i], &b = second.solvers[i];
      INFO("level " << a.iMesh << ", solver " << a.iSol);
      CHECK(CountDifferent(a.solution, b.solution) == 0);
      CHECK(CountDifferent(a.timeN, b.timeN) == 0);
      CHECK(CountDifferent(a.timeN1, b.timeN1) == 0);
      CHECK(CountDifferent(a.solutionOld, b.solutionOld) == 0);
      CHECK(CountDifferent(a.primitive, b.primitive) == 0);
      CHECK(CountDifferent(a.localCFL, b.localCFL) == 0);
      CHECK(CountDifferent(a.muT, b.muT) == 0);
      /*--- The window changed the solution (the test is not trivial). ---*/
      if (a.iMesh == 0) CHECK(CountDifferent(a.solution, start.solvers[i].solution) > 0);
    }
    CHECK(first.timeIter == 4);
    CHECK(second.timeIter == 4);
    CHECK(CountDifferent(first.CFL, second.CFL) == 0);
    CHECK(first.dampResRestric == second.dampResRestric);
    CHECK(first.dampCorrecProlong == second.dampCorrecProlong);

    origBuf = std::cout.rdbuf(nullptr);
    driver.Finalize();
  }
  std::cout.rdbuf(origBuf);
  std::remove(cfgName.c_str());
  std::remove((name + "_history.csv").c_str());
}

}  // namespace

TEST_CASE("Steady adaptation consumes its completed metric; public remeshing refreshes it",
          "[Adaptation][SteadyMetricReuse]") {
  class CountingDriver : public TestDriver {
   public:
    using TestDriver::TestDriver;
    unsigned metricCalls = 0;
    void ComputeMetric() override {
      ++metricCalls;
      const su2double metric[3][3] = {{4, 0, 0}, {0, 4, 0}, {0, 0, 4}};
      for (auto p = 0ul; p < Geometry()->GetnPoint(); ++p) FlowSolver()->GetNodes()->SetMetricMat(p, metric);
    }
  };
  const std::string name = "steady_metric_reuse";
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(2, 4, simplex_test::Marker2D), name + ".su2");
    std::ofstream cfg(name + ".cfg");
    cfg << "SOLVER= EULER\nMACH_NUMBER= 0.5\nAOA= 0\nMGLEVEL= 0\n"
           "MESH_FORMAT= SU2\nMESH_FILENAME= " << name << ".su2\n"
           "MARKER_FAR= (left, right, upper, lower_a, lower_b)\n"
           "CONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\nITER= 1\nCFL_NUMBER= 1\n"
           "COMPUTE_METRIC= YES\nADAP_SENSOR= MACH\nADAP_LOOP= YES\nADAP_REMESHER= NATIVE_CAVITY\n"
           "ADAP_SIZES= (8)\nADAP_SUBITER= (1)\nADAP_FLOW_ITER= (1)\nADAP_FLOW_CFL= (1)\n"
           "ADAP_HMIN= 0.01\nADAP_HMAX= 2\nADAP_HAUSD= 1e-8\nADAP_TRANSFER= BARYCENTRIC\n"
           "OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nRESTART_FILENAME= " << name << "_solution\n"
           "MESH_OUT_FILENAME= " << name << "_mesh\nCONV_FILENAME= " << name << "_history\n";
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  {
    CountingDriver driver(const_cast<char*>((name + ".cfg").c_str()), 1, SU2_MPI::GetComm());
    driver.StartSolver();
    CHECK(driver.metricCalls == 2);  // One computation on each mesh; the old loop makes three.
    const auto result = driver.RemeshFromMetric();
    CHECK(driver.metricCalls == 3);  // Explicit remeshing must recompute from the current solution.
    CHECK(result.status == CRemeshResult::Status::COMPLETE);
    driver.Finalize();
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  if (SU2_MPI::GetRank() == MASTER_NODE)
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::current_path()))
      if (entry.path().filename().string().find(name) == 0) std::filesystem::remove(entry.path());
}

TEST_CASE("Time window state, same mesh, Euler with multigrid", "[Adaptation]") {
  /*--- Convected vortex. ---*/
  CheckSameMeshResolve("fp_state_euler", "SOLVER= EULER\nKIND_VERIFICATION_SOLUTION= INVISCID_VORTEX\n"
                                         "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus)\n"
                                         "CONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= YES\n"
                                         "SLOPE_LIMITER_FLOW= VENKATAKRISHNAN\nMGLEVEL= 2\n");
}

TEST_CASE("Physical time step of UNST_CFL_NUMBER kept by a mesh replacement and the window state", "[Adaptation]") {
  /*--- Dual time stepping with the time step computed from UNST_CFL_NUMBER (TIME_STEP= 0 is valid then): it is
   *    computed at the first time step and must stay for the rest of the run, also on a new mesh. ---*/
  const std::string name = "unst_cfl_dt";
  simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(2, 8, simplex_test::Marker2D), name + ".su2");
  {
    std::ofstream cfg(name + ".cfg");
    cfg << "SOLVER= EULER\nMACH_NUMBER= 0.5\nAOA= 5.0\n"
           "FREESTREAM_DENSITY= 1.0\nFREESTREAM_PRESSURE= 1.0\nFREESTREAM_TEMPERATURE= 1.0\n"
           "FLUID_MODEL= IDEAL_GAS\nGAMMA_VALUE= 1.4\nGAS_CONSTANT= 1.0\nREF_DIMENSIONALIZATION= DIMENSIONAL\n"
           "INIT_OPTION= TD_CONDITIONS\nMESH_FORMAT= SU2\nMESH_FILENAME= " << name << ".su2\n"
           "MARKER_FAR= (left, right, upper)\nMARKER_EULER= (lower_a, lower_b)\n"
           "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 0.0\nUNST_CFL_NUMBER= 2.0\n"
           "TIME_ITER= 10\nINNER_ITER= 3\nNUM_METHOD_GRAD= GREEN_GAUSS\nCFL_NUMBER= 10\nMGLEVEL= 0\n"
           "CONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
           "LINEAR_SOLVER= FGMRES\nLINEAR_SOLVER_PREC= ILU\nLINEAR_SOLVER_ITER= 5\nCONV_RESIDUAL_MINVAL= -14\n"
           "OUTPUT_FILES= (RESTART)\nSCREEN_WRT_FREQ_INNER= 1000\nCONV_FILENAME= " << name << "_history\n";
  }

  auto* origBuf = std::cout.rdbuf(nullptr);
  {
    TestDriver driver(const_cast<char*>((name + ".cfg").c_str()), 1, SU2_MPI::GetComm());
    auto* config = driver.Config();
    auto step = [&](unsigned long timeIter) {
      driver.Preprocess(timeIter);
      driver.Run();
      driver.Postprocess();
      driver.Update();
    };
    for (unsigned long timeIter = 0; timeIter < 2; timeIter++) step(timeIter);
    const auto dt = config->GetDelta_UnstTimeND();

    /*--- The window state keeps it. ---*/
    const auto state = driver.SaveTimeWindowState();
    config->SetDelta_UnstTimeND(0.5 * dt);
    driver.RestoreTimeWindowState(state);
    const auto dtRestored = config->GetDelta_UnstTimeND();

    /*--- A new mesh (the same mesh, extracted and rebuilt) keeps it. ---*/
    const auto nPoint = driver.Geometry()->GetnPoint();
    su2activematrix metric(nPoint, 3);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      metric(iPoint, 0) = 1.0;
      metric(iPoint, 1) = 0.0;
      metric(iPoint, 2) = 1.0;
    }
    const auto mesh = CMMGInterface::ExtractMesh(*config, *driver.Geometry(), metric);
    CBarycentricTransfer transfer;
    driver.ReplaceMesh(mesh, transfer);
    const auto dtNewMesh = config->GetDelta_UnstTimeND();
    std::cout.rdbuf(origBuf);

    CHECK(dt > 0.0);
    CHECK(dtRestored == dt);
    CHECK(dtNewMesh == dt);
    REQUIRE(dtNewMesh > 0.0);

    /*--- The next time steps run with it (finite residuals) and do not recompute it. ---*/
    origBuf = std::cout.rdbuf(nullptr);
    for (unsigned long timeIter = 2; timeIter < 4; timeIter++) step(timeIter);
    const auto dtAfter = config->GetDelta_UnstTimeND();
    const auto resRho = driver.FlowSolver()->GetRes_RMS(0);
    std::cout.rdbuf(origBuf);
    CHECK(dtAfter == dt);
    CHECK(std::isfinite(SU2_TYPE::GetValue(resRho)));

    origBuf = std::cout.rdbuf(nullptr);
    driver.Finalize();
  }
  std::cout.rdbuf(origBuf);
  for (const auto& suffix : {".cfg", ".su2", "_history.csv"}) std::remove((name + suffix).c_str());
}

TEST_CASE("Discarded window solve writes no file (fixed CL entering finite differences)", "[Adaptation]") {
  /*--- A solve that the fixed-point loop discards runs Preprocess/Run/Postprocess/Update with the file output off.
   *    Fixed-CL mode writes the result files and the meta data when it starts its finite differences (inside Run), at
   *    inner iteration INNER_ITER - ITER_DCL_DALPHA of the first time step here: nothing may be written or changed. ---*/
  namespace fs = std::filesystem;
  const std::string name = "discarded_files";
  const auto dir = fs::current_path() / (name + "_dir");
  fs::remove_all(dir);
  fs::create_directory(dir);
  const auto cwd = fs::current_path();
  fs::current_path(dir);
  {
    std::ofstream cfg(name + ".cfg");
    cfg << "SOLVER= EULER\nMACH_NUMBER= 0.5\nAOA= 2.0\n"
           "FREESTREAM_DENSITY= 1.0\nFREESTREAM_PRESSURE= 1.0\nFREESTREAM_TEMPERATURE= 1.0\n"
           "FLUID_MODEL= IDEAL_GAS\nGAMMA_VALUE= 1.4\nGAS_CONSTANT= 1.0\nREF_DIMENSIONALIZATION= DIMENSIONAL\n"
           "INIT_OPTION= TD_CONDITIONS\nREF_AREA= 1.0\n"
           "MESH_FORMAT= RECTANGLE\nMESH_BOX_SIZE= (9, 9, 0)\nMESH_BOX_LENGTH= (1.0, 1.0, 0.0)\n"
           "MARKER_FAR= (x_minus, x_plus, y_plus)\nMARKER_EULER= (y_minus)\nMARKER_MONITORING= (y_minus)\n"
           "FIXED_CL_MODE= YES\nTARGET_CL= 0.1\nDCL_DALPHA= 0.1\nITER_DCL_DALPHA= 2\n"
           "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 0.01\nTIME_ITER= 10\n"
           "INNER_ITER= 5\nNUM_METHOD_GRAD= GREEN_GAUSS\nCFL_NUMBER= 10\nMGLEVEL= 0\n"
           "CONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
           "LINEAR_SOLVER= FGMRES\nLINEAR_SOLVER_PREC= ILU\nLINEAR_SOLVER_ITER= 5\nCONV_RESIDUAL_MINVAL= -14\n"
           "OUTPUT_FILES= (RESTART, PARAVIEW)\nWRT_FORCES_BREAKDOWN= YES\nSCREEN_WRT_FREQ_INNER= 1\n"
           "CONV_FILENAME= history\n";
  }

  /*--- Name, size and modification time of every file of the directory. ---*/
  auto listing = [&]() {
    std::map<std::string, std::pair<std::uintmax_t, fs::file_time_type>> files;
    for (const auto& entry : fs::directory_iterator(dir))
      files[entry.path().filename().string()] = {fs::file_size(entry.path()), fs::last_write_time(entry.path())};
    return files;
  };

  auto* origBuf = std::cout.rdbuf(nullptr);
  bool finiteDifferences = false;
  decltype(listing()) before, after;
  {
    TestDriver driver(const_cast<char*>((name + ".cfg").c_str()), 1, SU2_MPI::GetComm());
    before = listing();
    driver.Output()->SetFileWriting(false);
    driver.SolveTimeWindow(0, 1, false);
    driver.Output()->SetFileWriting(true);
    finiteDifferences = driver.Config()->GetFinite_Difference_Mode();
    after = listing();
    driver.Finalize();
  }
  std::cout.rdbuf(origBuf);
  fs::current_path(cwd);
  fs::remove_all(dir);

  CHECK(finiteDifferences);  // the case reaches the fixed-CL file output
  CHECK(after.size() == before.size());
  for (const auto& file : after) {
    INFO("file " << file.first);
    REQUIRE(before.count(file.first) == 1);
    CHECK(before.at(file.first) == file.second);
  }
}

TEST_CASE("Time window state, same mesh, RANS", "[Adaptation]") {
  /*--- Boundary layer starting on a wall from the free stream (eddy viscosity, wall distance). ---*/
  CheckSameMeshResolve("fp_state_rans", "SOLVER= RANS\nKIND_TURB_MODEL= SA\nVISCOSITY_MODEL= CONSTANT_VISCOSITY\n"
                                        "MARKER_HEATFLUX= (y_minus, 0.0)\nMARKER_FAR= (x_minus, x_plus, y_plus)\n"
                                        "MU_CONSTANT= 1e-3\nCONV_NUM_METHOD_FLOW= JST\nCONV_NUM_METHOD_TURB= SCALAR_UPWIND\n"
                                        "MUSCL_TURB= NO\nMGLEVEL= 2\n");
}
