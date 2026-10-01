/*!
 * \file COptionDVParam_tests.cpp
 * \brief Unit tests for the parsing of the DV_PARAM option.
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
#include <string>
#include <vector>
#define ENABLE_MAPS
#include "../../Common/include/option_structure.hpp"
#undef ENABLE_MAPS

namespace {

/*--- Parses the DV_PARAM tokens for the DV_KIND = HICKS_HENNE, TRANSLATION, returns the error message. ---*/
std::string ParseDVParam(const std::vector<std::string>& tokens) {
  unsigned short design_variable[] = {HICKS_HENNE, TRANSLATION};
  unsigned short* design_variable_ptr = design_variable;
  unsigned short nDV = 2;
  su2double** paramDV = nullptr;
  std::string* FFDTag = nullptr;

  COptionDVParam option("DV_PARAM", nDV, paramDV, FFDTag, design_variable_ptr);
  const auto message = option.SetValue(tokens);

  if (paramDV != nullptr) {
    for (unsigned short iDV = 0; iDV < nDV; iDV++) delete[] paramDV[iDV];
    delete[] paramDV;
  }
  delete[] FFDTag;
  return message;
}

}  // namespace

TEST_CASE("DV_PARAM number of parameters", "[Config]") {
  /*--- HICKS_HENNE takes 2 parameters, TRANSLATION takes 3. ---*/

  CHECK(ParseDVParam({"1", "0.5", ";", "1.0", "0.0", "0.0"}).empty());

  /*--- Too few parameters for the last or for the first design variable. ---*/

  CHECK_FALSE(ParseDVParam({"1", "0.5", ";", "1.0", "0.0"}).empty());
  CHECK_FALSE(ParseDVParam({"1", ";", "1.0", "0.0", "0.0"}).empty());

  /*--- More design variables than entries in DV_KIND. ---*/

  CHECK_FALSE(ParseDVParam({"1", "0.5", ";", "1.0", "0.0", "0.0", ";", "1", "0.5"}).empty());
}
