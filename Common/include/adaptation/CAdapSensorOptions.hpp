/*!
 * \file CAdapSensorOptions.hpp
 * \brief Names and ordered definitions of custom adaptation sensors.
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

#pragma once

#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace AdapSensorOptions {

inline std::string Trim(const std::string& text) {
  const auto first = text.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

inline bool IsReserved(const std::string& name) {
  static const std::set<std::string> reserved = {
      "MACH", "PRESSURE", "TEMPERATURE", "ENERGY", "DENSITY", "TOTALPRESSURE", "GOAL",
      "ENTHALPY", "SOUND_SPEED", "LAMINAR_VISCOSITY", "EDDY_VISCOSITY", "THERMAL_CONDUCTIVITY",
      "CP_TOTAL", "VELOCITY_X", "VELOCITY_Y", "VELOCITY_Z"};
  if (reserved.count(name)) return true;
  for (const auto* prefix : {"GRAD_", "TURB", "SPECIES", "SCALAR"})
    if (name.find(prefix) == 0) return true;
  return false;
}

inline std::vector<std::pair<std::string, std::string>> Parse(const std::string& text) {
  std::vector<std::pair<std::string, std::string>> result;
  std::set<std::string> names;
  if (Trim(text).empty()) return result;
  for (size_t first = 0; first <= text.size();) {
    const auto last = text.find(';', first);
    const auto definition = Trim(text.substr(first, last == std::string::npos ? last : last - first));
    const auto colon = definition.find(':');
    if (colon == std::string::npos) throw std::invalid_argument("Expected NAME : expression in ADAP_CUSTOM_SENSORS.");
    const auto name = Trim(definition.substr(0, colon));
    const auto expression = Trim(definition.substr(colon + 1));
    const auto letter = [](char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'); };
    if (name.empty() || !letter(name[0]) ||
        !std::all_of(name.begin(), name.end(), [&](char c) { return letter(c) || (c >= '0' && c <= '9') || c == '_'; }))
      throw std::invalid_argument("Invalid custom adaptation sensor name: " + name);
    if (IsReserved(name)) throw std::invalid_argument("Reserved custom adaptation sensor name: " + name);
    if (!names.insert(name).second) throw std::invalid_argument("Repeated custom adaptation sensor name: " + name);
    if (expression.empty()) throw std::invalid_argument("Empty custom adaptation sensor: " + name);
    result.emplace_back(name, expression);
    if (last == std::string::npos) break;
    first = last + 1;
  }
  return result;
}

}  // namespace AdapSensorOptions
