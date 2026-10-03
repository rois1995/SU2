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
#include <fstream>
#include <string>

#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"

namespace {

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

TEST_CASE("Time window state, same mesh, Euler with multigrid", "[Adaptation]") {
  /*--- Convected vortex. ---*/
  CheckSameMeshResolve("fp_state_euler", "SOLVER= EULER\nKIND_VERIFICATION_SOLUTION= INVISCID_VORTEX\n"
                                         "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus)\n"
                                         "CONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= YES\n"
                                         "SLOPE_LIMITER_FLOW= VENKATAKRISHNAN\nMGLEVEL= 2\n");
}

TEST_CASE("Time window state, same mesh, RANS", "[Adaptation]") {
  /*--- Boundary layer starting on a wall from the free stream (eddy viscosity, wall distance). ---*/
  CheckSameMeshResolve("fp_state_rans", "SOLVER= RANS\nKIND_TURB_MODEL= SA\nVISCOSITY_MODEL= CONSTANT_VISCOSITY\n"
                                        "MARKER_HEATFLUX= (y_minus, 0.0)\nMARKER_FAR= (x_minus, x_plus, y_plus)\n"
                                        "MU_CONSTANT= 1e-3\nCONV_NUM_METHOD_FLOW= JST\nCONV_NUM_METHOD_TURB= SCALAR_UPWIND\n"
                                        "MUSCL_TURB= NO\nMGLEVEL= 2\n");
}
