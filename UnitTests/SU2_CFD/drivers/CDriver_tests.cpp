/*!
 * \file CDriver_tests.cpp
 * \brief Unit tests for the run-time state of the config saved by the driver.
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

#include "../../../Common/include/CConfig.hpp"
#include "../../../SU2_CFD/include/drivers/CDriver.hpp"

namespace {

void CheckRunState(bool unsteady) {
  const string timeOptions = unsteady ? "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n"
                                        "TIME_STEP= 0.1\nTIME_ITER= 20\nINNER_ITER= 5\n"
                                      : "ITER= 100\n";
  stringstream options("SOLVER= EULER\nMESH_FORMAT= BOX\nINIT_OPTION= TD_CONDITIONS\nMGLEVEL= 2\nCFL_NUMBER= 5\n"
                       "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n" + timeOptions);
  auto origBuf = cout.rdbuf();
  cout.rdbuf(nullptr);
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  cout.rdbuf(origBuf);

  CDriver::CConfigRunState state;
  state.Save(config);
  const auto timeIter = config.GetTimeIter(), offset = config.GetExtIter_OffSet();
  const auto nMGLevels = config.GetnMGLevels();
  const auto CFL = config.GetCFL(0);

  /*--- Changes made by a run on the first mesh. ---*/
  config.SetMGLevels(0);
  config.SetCFL(0, 2 * CFL);
  config.SetTimeIter(timeIter + 7);
  config.SetExtIter_OffSet(offset + 11);
  config.SetInnerIter(3);
  config.SetOuterIter(2);

  state.Restore(config);
  CHECK(nMGLevels == 2);
  CHECK(config.GetnMGLevels() == nMGLevels);
  CHECK(config.GetCFL(0) == CFL);
  CHECK(config.GetInnerIter() == 0);
  CHECK(config.GetOuterIter() == 0);

  /*--- A time-domain run keeps its time counters, a steady run starts again. ---*/
  CHECK(config.GetTimeIter() == (unsteady ? timeIter + 7 : timeIter));
  CHECK(config.GetExtIter_OffSet() == (unsteady ? offset + 11 : offset));
}

}  // namespace

TEST_CASE("Config run state, steady", "[Adaptation]") { CheckRunState(false); }

TEST_CASE("Config run state, time domain", "[Adaptation]") { CheckRunState(true); }
