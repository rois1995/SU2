/*!
 * \file epm_field_tests.cpp
 * \brief Unit tests for the EPM field-sensitivity options and extraction state.
 * \author SU2 Contributors
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

#include "catch.hpp"
#include "../../../Common/include/CConfig.hpp"

#include <sstream>
#include <string>

#if defined(CODI_REVERSE_TYPE) && !defined(HAVE_OMP)
namespace {
/*--- Options of the constrained coupled adjoint accepted by CConfig for SST_EPM_POSTPROCESS. ---*/
std::string EPMBaseOptions() {
  return "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= V2003m\n"
         "MATH_PROBLEM= DISCRETE_ADJOINT\nOBJECTIVE_FUNCTION= DRAG\n"
         "FROZEN_VISC_DISC= NO\nFROZEN_LIMITER_DISC= NO\nINCONSISTENT_DISC= NO\n"
         "SLOPE_LIMITER_FLOW= NONE\nMGLEVEL= 0\nCFL_ADAPT= NO\n"
         "UQ_DELTA_B= 1\nUQ_URLX= 1\nSST_EPM_POSTPROCESS= YES\n";
}
}  // namespace

TEST_CASE("EPM field options parse", "[EPMField]") {
  {
    std::stringstream options(EPMBaseOptions());
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    const auto& epm = config.GetEPMExtraction();
    CHECK(epm.enabled);
    CHECK(epm.mode == CEPMExtraction::Mode::FIELD);
    CHECK_FALSE(epm.extract_only);
    CHECK_FALSE(epm.cursor.set);
    CHECK(epm.field == nullptr);
  }
  {
    std::stringstream options(EPMBaseOptions() + "SST_EPM_EXTRACTION= SCALAR_REPLAY\n");
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    CHECK(config.GetEPMExtraction().mode == CEPMExtraction::Mode::SCALAR_REPLAY);
    CHECK_FALSE(config.GetEPMExtraction().extract_only);
  }
}

TEST_CASE("EPM extract-only configuration is accepted", "[EPMField]") {
  /*--- Rejections (no restart, ITER != 1, non-residual criterion, equal adjoint file names) call
   *    SU2_MPI::Error, which aborts the process; they are covered by the Python smoke runner. ---*/
  std::stringstream options(EPMBaseOptions() +
                            "SST_EPM_EXTRACT_ONLY= YES\nRESTART_SOL= YES\nITER= 1\n"
                            "CONV_FIELD= RMS_ADJ_DENSITY\n"
                            "SOLUTION_ADJ_FILENAME= solution_adj\nRESTART_ADJ_FILENAME= restart_adj\n");
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  const auto& epm = config.GetEPMExtraction();
  CHECK(epm.enabled);
  CHECK(epm.extract_only);
  CHECK(epm.mode == CEPMExtraction::Mode::FIELD);
  CHECK(config.GetRestart());
  CHECK(config.GetnInner_Iter() == 1);
}
#endif
