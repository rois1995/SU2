/*!
 * \file CConfig_tests.cpp
 * \brief Unit tests for option defaults that depend on other options.
 * \author SU2 Contributors
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
#include "../../Common/include/CConfig.hpp"

TEST_CASE("SST Newton named options are parsed with an inactive legacy default", "[globalization][config]") {
  std::stringstream input("SOLVER= RANS\nKIND_TURB_MODEL= SST\nMESH_FORMAT= BOX\nMACH_NUMBER= 0.15\nREYNOLDS_NUMBER= 6e6\n"
      "SST_NEWTON_RETRIES= 3\nSST_NEWTON_CFL_MIN= 0.01\nSST_NEWTON_ARMIJO= 0.0002\n"
      "SST_NEWTON_ADAPTIVE_FORCING= NO\n");
  CConfig config(input, SU2_COMPONENT::SU2_CFD, false);
  const auto options = config.GetSSTNewtonOptions().Passive([](const su2double& x) { return SU2_TYPE::GetValue(x); });
  REQUIRE_FALSE(options.enabled);
  REQUIRE(options.recovery);
  REQUIRE(options.lineSearch);
  REQUIRE(options.pseudoTimeMerit);
  REQUIRE(options.maxRetries == 3);
  REQUIRE(options.cflMin == 0.01);
  REQUIRE(options.armijo == 0.0002);
  REQUIRE_FALSE(options.adaptiveForcing);
}

TEST_CASE("Common Newton selection grouping and controls are independent", "[globalization][config]") {
  for (bool coupled : {false, true}) {
    std::stringstream input;
    input << "SOLVER= RANS\nKIND_TURB_MODEL= SST\nKIND_TRANS_MODEL= LM\n"
             "MESH_FORMAT= BOX\nMGLEVEL= 0\nMACH_NUMBER= 0.15\nREYNOLDS_NUMBER= 6e6\n"
             "NEWTON_EQUATIONS= (TRANS,TURB)\nNEWTON_RETRIES= 3\nNEWTON_CFL_MIN= 0.01\n"
             "NEWTON_ARMIJO= 0.0002\nNEWTON_ADAPTIVE_FORCING= NO\nNEWTON_BLOCK_LOWER= YES\n"
             "NEWTON_COUPLED= " << (coupled ? "YES" : "NO") << "\n";
    CConfig config(input, SU2_COMPONENT::SU2_CFD, false);
    const std::vector<unsigned short> expected{RUNTIME_TRANS_SYS, RUNTIME_TURB_SYS};
    CHECK(config.GetNewtonEquations() == expected);
    CHECK(config.GetNewtonCoupled() == coupled);
    CHECK(config.GetNewtonBlockLower());
    CHECK_FALSE(config.GetNewtonKrylov());
    const auto options = config.GetNewtonOptions().Passive([](const su2double& x) { return SU2_TYPE::GetValue(x); });
    CHECK(options.maxRetries == 3);
    CHECK(options.cflMin == 0.01);
    CHECK(options.armijo == 0.0002);
    CHECK_FALSE(options.adaptiveForcing);
    CHECK(config.GetSSTNewtonOptions().maxRetries == 16);
    CHECK_FALSE(config.GetSSTNewtonOptions().enabled);
  }
}

TEST_CASE("SST production and compressibility options remain independent", "[Config][SST]") {
  for (const std::string production : {"VORTICITY", "KATO-LAUNDER"}) {
    for (const std::string correction : {"COMPRESSIBILITY-WILCOX", "COMPRESSIBILITY-SARKAR"}) {
      for (const bool reverse : {false, true}) {
        std::stringstream input;
        input << "SOLVER= RANS\nKIND_TURB_MODEL= SST\nREYNOLDS_NUMBER= 1e6\n";
        input << "SST_OPTIONS= (" << (reverse ? correction : production) << ", "
              << (reverse ? production : correction) << ")\n";
        CConfig config(input, SU2_COMPONENT::SU2_CFD, false);
        const auto options = config.GetSSTParsedOptions();
        CHECK(options.production == (production == "VORTICITY" ? SST_OPTIONS::V : SST_OPTIONS::KL));
        CHECK(options.compWilcox == (correction == "COMPRESSIBILITY-WILCOX"));
        CHECK(options.compSarkar == (correction == "COMPRESSIBILITY-SARKAR"));
      }
    }
  }
}

namespace {

/*--- Minimal incompressible case on a box mesh, the options under test are appended to it. ---*/
const std::string base_options =
    "SOLVER= INC_NAVIER_STOKES\n"
    "MESH_FORMAT= BOX\n"
    "MESH_BOX_SIZE= 5,5,5\n"
    "MESH_BOX_LENGTH= 1,1,1\n"
    "MESH_BOX_OFFSET= 0,0,0\n"
    "MARKER_HEATFLUX= (y_minus, 0.0, y_plus, 0.0)\n"
    "MARKER_CUSTOM= (x_minus, x_plus, z_plus, z_minus)\n"
    "INC_DENSITY_INIT= 1.1766\n"
    "INC_TEMPERATURE_INIT= 300.0\n"
    "VISCOSITY_MODEL= CONSTANT_VISCOSITY\n";

INIT_OPTION_INC GetInitOptionInc(const std::string& options) {
  std::stringstream ss(base_options + options);
  auto* orig_buf = std::cout.rdbuf(nullptr);
  CConfig config(ss, SU2_COMPONENT::SU2_CFD, false);
  std::cout.rdbuf(orig_buf);
  return config.GetKind_InitOption_Inc();
}

const std::string ideal_gas_options =
    "INC_DENSITY_MODEL= VARIABLE\n"
    "INC_ENERGY_EQUATION= YES\n"
    "FLUID_MODEL= INC_IDEAL_GAS\n";

const std::string mixture_options =
    "INC_DENSITY_MODEL= VARIABLE\n"
    "INC_ENERGY_EQUATION= YES\n"
    "FLUID_MODEL= FLUID_MIXTURE\n"
    "KIND_SCALAR_MODEL= SPECIES_TRANSPORT\n"
    "SPECIES_INIT= 1.0\n"
    "MOLECULAR_WEIGHT= 28.96, 16.043\n"
    "SPECIFIC_HEAT_CP= 1009.39, 2225.0\n"
    "MARKER_SPECIES_STRONG_BC= (x_minus)\n"
    "MU_CONSTANT= 1.8551e-05, 1.1e-05\n"
    "PRANDTL_LAM= 0.72, 0.72\n"
    "PRANDTL_TURB= 0.9, 0.9\n"
    "DIFFUSIVITY_CONSTANT= 1.0e-05\n";

}  // namespace

TEST_CASE("INIT_OPTION_INC defaults", "[Config]") {
  /*--- Without the option each fluid model keeps the density it used before INIT_OPTION_INC existed. ---*/

  CHECK(GetInitOptionInc(ideal_gas_options) == INIT_OPTION_INC::DENSITY_INIT);
  CHECK(GetInitOptionInc(mixture_options) == INIT_OPTION_INC::OPERATING_PRESSURE);
  CHECK(GetInitOptionInc("INC_DENSITY_MODEL= CONSTANT\n") == INIT_OPTION_INC::DENSITY_INIT);

  /*--- An explicit setting overrides the default. ---*/

  CHECK(GetInitOptionInc(ideal_gas_options + "INIT_OPTION_INC= OPERATING_PRESSURE\n") ==
        INIT_OPTION_INC::OPERATING_PRESSURE);
  CHECK(GetInitOptionInc(ideal_gas_options + "INIT_OPTION_INC= DENSITY_INIT\n") == INIT_OPTION_INC::DENSITY_INIT);
}
