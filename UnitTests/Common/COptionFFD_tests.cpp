/*!
 * \file COptionFFD_tests.cpp
 * \brief Unit tests for the parsing of the FFD_DEFINITION and FFD_DEGREE options.
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

/*--- Tokens of FFD_DEFINITION for nBox boxes (tag and 24 coordinates each), the last one with nLast coordinates. ---*/
std::vector<std::string> DefinitionTokens(unsigned short nBox, unsigned short nLast = 24) {
  std::vector<std::string> tokens;
  for (unsigned short iBox = 0; iBox < nBox; iBox++) {
    if (iBox > 0) tokens.emplace_back(";");
    tokens.push_back("BOX" + std::to_string(iBox));
    const unsigned short nCoord = (iBox == nBox - 1) ? nLast : 24;
    for (unsigned short iCoord = 0; iCoord < nCoord; iCoord++) tokens.emplace_back("0.5");
  }
  return tokens;
}

/*--- Tokens of FFD_DEGREE for nBox boxes, the last one with nLast values. ---*/
std::vector<std::string> DegreeTokens(unsigned short nBox, unsigned short nLast = 3) {
  std::vector<std::string> tokens;
  for (unsigned short iBox = 0; iBox < nBox; iBox++) {
    if (iBox > 0) tokens.emplace_back(";");
    const unsigned short nValue = (iBox == nBox - 1) ? nLast : 3;
    for (unsigned short iValue = 0; iValue < nValue; iValue++) tokens.emplace_back("2");
  }
  return tokens;
}

/*--- Parses FFD_DEFINITION and then FFD_DEGREE (if given) as CConfig does, with a shared number of boxes,
 * returns the first error message. The options free their memory when they go out of scope. ---*/
std::string ParseFFD(const std::vector<std::string>& definition, const std::vector<std::string>& degree) {
  unsigned short nFFDBox = 0;
  su2double** coordFFD = nullptr;
  std::string* tagFFD = nullptr;
  unsigned short** degreeFFD = nullptr;

  COptionFFDDef optionDef("FFD_DEFINITION", nFFDBox, coordFFD, tagFFD);
  COptionFFDDegree optionDegree("FFD_DEGREE", nFFDBox, degreeFFD);

  auto message = optionDef.SetValue(definition);
  if (message.empty() && !degree.empty()) message = optionDegree.SetValue(degree);
  return message;
}

}  // namespace

TEST_CASE("FFD_DEFINITION and FFD_DEGREE checks", "[Config]") {
  CHECK(ParseFFD(DefinitionTokens(2), DegreeTokens(2)).empty());

  /*--- A box with too few coordinates or degrees. ---*/

  CHECK_FALSE(ParseFFD(DefinitionTokens(2, 23), {}).empty());
  CHECK_FALSE(ParseFFD(DefinitionTokens(2), DegreeTokens(2, 2)).empty());

  /*--- Different number of boxes in the two options. ---*/

  CHECK_FALSE(ParseFFD(DefinitionTokens(2), DegreeTokens(1)).empty());

  /*--- More boxes than MAX_NUMBER_FFD. ---*/

  CHECK(ParseFFD(DefinitionTokens(MAX_NUMBER_FFD), {}).empty());
  CHECK_FALSE(ParseFFD(DefinitionTokens(MAX_NUMBER_FFD + 1), {}).empty());
}
