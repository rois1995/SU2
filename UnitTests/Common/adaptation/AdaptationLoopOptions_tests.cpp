/*!
 * \file AdaptationLoopOptions_tests.cpp
 * \brief Unit tests for the options of the mesh adaptation loop (lists, defaults, cycles).
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

#include <sstream>
#include <string>

#include "../../../Common/include/CConfig.hpp"

namespace {

const std::string baseOptions =
    "SOLVER= EULER\nMESH_FORMAT= BOX\nINIT_OPTION= TD_CONDITIONS\nMGLEVEL= 2\n"
    "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n"
    "ITER= 150\nCFL_NUMBER= 7\n"
    "COMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\n"
    "ADAP_HMAX= 3\nADAP_HMIN= 1e-3\nADAP_NORM= 2\nADAP_ARMAX= 100\nADAP_COMPLEXITY= 777\n";

std::unique_ptr<CConfig> MakeConfig(const std::string& options) {
  std::stringstream ss(baseOptions + options);
  auto* origBuf = std::cout.rdbuf(nullptr);
  auto config = std::make_unique<CConfig>(ss, SU2_COMPONENT::SU2_CFD, false);
  std::cout.rdbuf(origBuf);
  return config;
}

}  // namespace

TEST_CASE("Adaptation loop options, scalar defaults", "[Adaptation]") {
  const auto config = MakeConfig("ADAP_LOOP= YES\nADAP_SIZES= (5000, 10000)\nADAP_SUBITER= (2, 3)\n");

  REQUIRE(config->GetAdap_Loop());
  REQUIRE(config->GetnAdap_Levels() == 2);
  CHECK(config->GetKind_Adap_Transfer() == ADAP_TRANSFER::BARYCENTRIC);

  for (unsigned short iLevel = 0; iLevel < 2; iLevel++) {
    const auto& level = config->GetAdap_Level(iLevel);
    CHECK(level.complexity == (iLevel == 0 ? 5000ul : 10000ul));
    CHECK(level.subIter == (iLevel == 0 ? 2ul : 3ul));
    CHECK(level.hmax == 3.0);
    CHECK(level.hmin == 1e-3);
    CHECK(level.norm == 2.0);
    CHECK(level.armax == 100.0);
    CHECK(level.flowIter == 150ul);
    CHECK(level.flowCFL == 7.0);
    CHECK(level.residualReduction == 0.0);
  }

  /*--- Cycles: 0 is the input mesh, then 2 meshes of level 0 and 3 of level 1. ---*/
  REQUIRE(config->GetnAdap_Cycles() == 5);
  const unsigned short levels[] = {0, 0, 1, 1, 1};
  for (unsigned long iCycle = 1; iCycle <= 5; iCycle++) CHECK(config->GetAdap_CycleLevel(iCycle) == levels[iCycle - 1]);

  /*--- The metric options of a level replace ADAP_COMPLEXITY, ADAP_HMAX, ... ---*/
  CHECK(config->GetAdap_Complexity() == 777);
  config->SetAdap_MetricLevel(1);
  CHECK(config->GetAdap_Complexity() == 10000);
  CHECK(config->GetAdap_Hmax() == 3.0);
  CHECK(config->GetAdap_Hmin() == 1e-3);
  CHECK(config->GetAdap_Norm() == 2.0);
  CHECK(config->GetAdap_ARmax() == 100.0);

  config->SetAdap_FlowLevel(1);
  CHECK(config->GetnInner_Iter() == 150);
}

TEST_CASE("Adaptation loop options, per-level lists", "[Adaptation]") {
  const auto config = MakeConfig(
      "ADAP_LOOP= YES\nADAP_SIZES= (1000, 2000, 4000)\nADAP_HMAXS= (4, 2, 1)\nADAP_HMINS= (1e-4)\n"
      "ADAP_NORMS= (1, 2, 4)\nADAP_ARMAXS= (10)\nADAP_FLOW_ITER= (100)\nADAP_FLOW_CFL= (5, 10, 20)\n"
      "ADAP_RESIDUAL_REDUCTION= (3)\nADAP_TRANSFER= FREESTREAM\n");

  REQUIRE(config->GetnAdap_Levels() == 3);
  CHECK(config->GetKind_Adap_Transfer() == ADAP_TRANSFER::FREESTREAM);

  /*--- Without ADAP_SUBITER, one adaptation per level. ---*/
  REQUIRE(config->GetnAdap_Cycles() == 3);

  const su2double hmax[] = {4, 2, 1}, norm[] = {1, 2, 4}, cfl[] = {5, 10, 20};
  for (unsigned short iLevel = 0; iLevel < 3; iLevel++) {
    const auto& level = config->GetAdap_Level(iLevel);
    CHECK(config->GetAdap_CycleLevel(iLevel + 1) == iLevel);
    CHECK(level.subIter == 1ul);
    CHECK(level.hmax == hmax[iLevel]);
    CHECK(level.hmin == 1e-4);
    CHECK(level.norm == norm[iLevel]);
    CHECK(level.armax == 10.0);
    CHECK(level.flowIter == 100ul);
    CHECK(level.flowCFL == cfl[iLevel]);
    CHECK(level.residualReduction == 3.0);
  }

  config->SetAdap_MetricLevel(2);
  CHECK(config->GetAdap_Complexity() == 4000);
  CHECK(config->GetAdap_Hmax() == 1.0);
  CHECK(config->GetAdap_Hmin() == 1e-4);
  CHECK(config->GetAdap_Norm() == 4.0);
  CHECK(config->GetAdap_ARmax() == 10.0);

  /*--- The solve on the input mesh keeps ITER; the adapted meshes use ADAP_FLOW_ITER. ---*/
  CHECK(config->GetnInner_Iter() == 150);
  config->SetAdap_FlowLevel(0);
  CHECK(config->GetnInner_Iter() == 100);
}

TEST_CASE("Adaptation loop options, fixed CL", "[Adaptation]") {
  /*--- ITER excludes the iterations of the CL derivative, which fixed-CL mode adds to every solve. ---*/
  const std::string fixedCL = "FIXED_CL_MODE= YES\nTARGET_CL= 0.3\nITER_DCL_DALPHA= 50\nADAP_LOOP= YES\n";

  const auto config = MakeConfig(fixedCL + "ADAP_SIZES= (1000)\n");
  CHECK(config->GetnInner_Iter() == 200);
  CHECK(config->GetAdap_Level(0).flowIter == 150ul);
  config->SetAdap_FlowLevel(0);
  CHECK(config->GetnInner_Iter() == 200);

  const auto config2 = MakeConfig(fixedCL + "ADAP_SIZES= (1000)\nADAP_FLOW_ITER= (30)\n");
  config2->SetAdap_FlowLevel(0);
  CHECK(config2->GetnInner_Iter() == 80);
}

TEST_CASE("Adaptation loop options, loop off", "[Adaptation]") {
  /*--- The lists parse (config files of the Python adaptation script) but define no level without ADAP_LOOP. ---*/
  const auto config = MakeConfig("ADAP_SIZES= (1000, 2000)\nADAP_SUBITER= (1, 3, 4)\nADAP_ADJ_ITER= (10, 20)\n");
  CHECK_FALSE(config->GetAdap_Loop());
  CHECK(config->GetnAdap_Levels() == 0);
  CHECK(config->GetnAdap_Cycles() == 0);
}

TEST_CASE("Adaptation loop, solution in memory and file names", "[Adaptation]") {
  /*--- A restart turns W_CYCLE into V_CYCLE; the adapted meshes read no restart and use the cycle of the file. ---*/
  const auto config = MakeConfig("RESTART_SOL= YES\nMGCYCLE= W_CYCLE\n");
  CHECK(config->GetRestart());
  CHECK(config->GetMGCycle() == MG_CYCLE::V);
  config->SetSolutionInMemory();
  CHECK_FALSE(config->GetRestart());
  CHECK(config->GetMGCycle() == MG_CYCLE::W);
  CHECK(config->GetFinestMesh() == MESH_0);

  /*--- The full multigrid starts from the coarsest level, an adapted mesh has a solution already. ---*/
  const auto config2 = MakeConfig("MGCYCLE= FULLMG_CYCLE\n");
  CHECK(config2->GetMGCycle() == MG_CYCLE::FULL);
  config2->SetSolutionInMemory();
  CHECK(config2->GetMGCycle() == MG_CYCLE::V);
  CHECK(config2->GetFinestMesh() == MESH_0);

  CHECK(CConfig::GetAdap_FileName("flow", 0) == "flow_adap_00000");
  CHECK(CConfig::GetAdap_FileName("restart_flow", 12) == "restart_flow_adap_00012");
}

TEST_CASE("Adapted mesh output options", "[Adaptation]") {
  /*--- Options with another input format than the BOX of the base options. ---*/
  auto makeConfig = [](const std::string& meshFormat, const std::string& options) {
    std::string base = baseOptions;
    base.replace(base.find("MESH_FORMAT= BOX"), 16, "MESH_FORMAT= " + meshFormat);
    std::stringstream ss(base + "ADAP_LOOP= YES\nADAP_SIZES= (5000)\n" + options);
    auto* origBuf = std::cout.rdbuf(nullptr);
    auto config = std::make_unique<CConfig>(ss, SU2_COMPONENT::SU2_CFD, false);
    std::cout.rdbuf(origBuf);
    return config;
  };

  /*--- Off by default, the default output format (SU2) is not changed then. ---*/
  auto config = makeConfig("SU2B", "");
  CHECK_FALSE(config->GetWrt_Adap_Mesh());
  CHECK(config->GetMesh_Out_FileFormat() == ENUM_GRID::SU2);

  /*--- Without MESH_OUT_FORMAT the adapted meshes have the format of the input mesh. ---*/
  config = makeConfig("SU2B", "WRT_ADAP_MESH= YES\n");
  CHECK(config->GetWrt_Adap_Mesh());
  CHECK(config->GetMesh_Out_FileFormat() == ENUM_GRID::SU2_BIN);
  CHECK(config->GetMesh_Out_FileExtension() == ".su2b");

  config = makeConfig("BOX", "WRT_ADAP_MESH= YES\n");
  CHECK(config->GetMesh_Out_FileFormat() == ENUM_GRID::SU2);
  CHECK(config->GetMesh_Out_FileExtension() == ".su2");

  /*--- MESH_OUT_FORMAT in the config file wins, also when it is the default value. ---*/
  config = makeConfig("SU2B", "WRT_ADAP_MESH= YES\nMESH_OUT_FORMAT= SU2\n");
  CHECK(config->GetMesh_Out_FileFormat() == ENUM_GRID::SU2);

  /*--- Names of the cycles. ---*/
  config = makeConfig("SU2", "WRT_ADAP_MESH= YES\nMESH_OUT_FILENAME= adapted.su2\n");
  CHECK(CConfig::GetAdap_FileName(config->GetMesh_Out_FileName(), 3) + config->GetMesh_Out_FileExtension() ==
        "adapted_adap_00003.su2");

#ifdef HAVE_CGNS
  config = makeConfig("CGNS", "WRT_ADAP_MESH= YES\n");
  CHECK(config->GetMesh_Out_FileFormat() == ENUM_GRID::CGNS_GRID);
  CHECK(config->GetMesh_Out_FileExtension() == ".cgns");

  config = makeConfig("CGNS", "WRT_ADAP_MESH= YES\nMESH_OUT_FORMAT= SU2\n");
  CHECK(config->GetMesh_Out_FileFormat() == ENUM_GRID::SU2);

  config = makeConfig("SU2", "WRT_ADAP_MESH= YES\nMESH_OUT_FORMAT= CGNS\n");
  CHECK(config->GetMesh_Out_FileFormat() == ENUM_GRID::CGNS_GRID);
#endif
}

TEST_CASE("Adaptation loop options, time domain", "[Adaptation]") {
  /*--- Time-domain runs take INNER_ITER, not ITER (removed from the base options). ---*/
  auto MakeTimeConfig = [](const std::string& options) {
    std::string base = baseOptions;
    base.replace(base.find("ITER= 150\n"), 10, "");
    std::stringstream ss(base + options);
    auto* origBuf = std::cout.rdbuf(nullptr);
    auto config = std::make_unique<CConfig>(ss, SU2_COMPONENT::SU2_CFD, false);
    std::cout.rdbuf(origBuf);
    return config;
  };
  const std::string timeOptions =
      "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 1e-3\nTIME_ITER= 100\n"
      "INNER_ITER= 20\nADAP_LOOP= YES\nADAP_SIZES= (4000)\nADAP_FREQ= 10\n";

  SECTION("Defaults") {
    const auto config = MakeTimeConfig(timeOptions);
    REQUIRE(config->GetAdap_Loop());
    CHECK(config->GetAdap_Freq() == 10ul);
    REQUIRE(config->GetnAdap_Levels() == 1);
    CHECK(config->GetAdap_Level(0).complexity == 4000ul);

    /*--- Default transfer of a time-domain run: barycentric until the conservative transfer exists. ---*/
    CHECK(config->GetAdap_Transfer_Default());
    CHECK(config->GetKind_Adap_Transfer() == ADAP_TRANSFER::BARYCENTRIC);

    /*--- Windows of 10 steps from step 0: adapted after the steps 9, 19, ... ---*/
    for (unsigned long timeIter = 0; timeIter < 100; timeIter++)
      CHECK(config->GetAdap_TimeWindowEnd(timeIter) == ((timeIter + 1) % 10 == 0));
  }

  SECTION("Explicit transfer") {
    const auto config = MakeTimeConfig(timeOptions + "ADAP_TRANSFER= BARYCENTRIC\n");
    CHECK_FALSE(config->GetAdap_Transfer_Default());
    CHECK(config->GetKind_Adap_Transfer() == ADAP_TRANSFER::BARYCENTRIC);
  }

  SECTION("Steady run, no window") {
    const auto config = MakeConfig("ADAP_LOOP= YES\nADAP_SIZES= (4000)\n");
    CHECK(config->GetAdap_Transfer_Default());
    CHECK(config->GetAdap_Freq() == 0ul);
    CHECK_FALSE(config->GetAdap_TimeWindowEnd(9));
  }
}
