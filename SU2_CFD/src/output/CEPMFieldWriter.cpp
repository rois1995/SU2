/*!
 * \file CEPMFieldWriter.cpp
 * \brief Writer of the per-rank EPM field files and their JSON layout.
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

#include "../../include/output/CEPMFieldWriter.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/CEPMFieldSites.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"

#define EPM_STRINGIZE_(x) #x
#define EPM_STRINGIZE(x) EPM_STRINGIZE_(x)

namespace {
using Record = CEPMFieldSites::Record;
constexpr size_t kColumns = sizeof(Record) / sizeof(passivedouble);

/*--- The file is the raw memory of the records: no padding, only passivedouble, and the columns named below
 *    follow the member order. ---*/
static_assert(std::is_standard_layout<Record>::value, "Record is written raw.");
static_assert(std::is_same<decltype(Record::kind), passivedouble>::value, "Record holds only passivedouble.");
static_assert(sizeof(Record) % sizeof(passivedouble) == 0, "Record must have no padding.");
static_assert(kColumns == 100, "Update the column names of the EPM field writer.");
static_assert(offsetof(Record, coord_a) == 3 * sizeof(passivedouble), "Column order changed.");
static_assert(offsetof(Record, grad) == 12 * sizeof(passivedouble), "Column order changed.");
static_assert(offsetof(Record, normal) == 21 * sizeof(passivedouble), "Column order changed.");
static_assert(offsetof(Record, S_tau) == 33 * sizeof(passivedouble), "Column order changed.");
static_assert(offsetof(Record, dtau) == 44 * sizeof(passivedouble), "Column order changed.");
static_assert(offsetof(Record, dpk) == 89 * sizeof(passivedouble), "Column order changed.");
static_assert(offsetof(Record, dpw) == 94 * sizeof(passivedouble), "Column order changed.");
static_assert(offsetof(Record, uses) == 99 * sizeof(passivedouble), "Column order changed.");

std::vector<std::string> ColumnNames() {
  std::vector<std::string> names;
  const auto add = [&names](const std::string& name) { names.push_back(name); };
  const auto add_array = [&add](const std::string& name, int n) {
    for (int i = 0; i < n; ++i) add(name + "_" + std::to_string(i));
  };
  add("kind"); add("key_a"); add("key_b");
  add_array("coord_a", 3);
  add_array("coord_b", 3);
  add("rho"); add("mu_t"); add("k");
  add_array("grad", 9);
  add_array("normal", 3);
  add("area"); add("volume");
  add("prod_limit"); add("pk0"); add("pw0"); add("p_raw0"); add("alfa_blended");
  add("floor_active"); add("cap_active");
  add_array("S_tau", CEPMFieldSites::kTau);
  add("S_pk"); add("S_pw");
  for (int c = 0; c < CEPMFieldSites::kCases; ++c)
    for (int j = 0; j < CEPMFieldSites::kTau; ++j) add("dtau_" + std::to_string(c) + "_" + std::to_string(j));
  for (int c = 0; c < CEPMFieldSites::kCases; ++c) add("dpk_" + std::to_string(c));
  for (int c = 0; c < CEPMFieldSites::kCases; ++c) add("dpw_" + std::to_string(c));
  add("uses");
  return names;
}

const char* NonDimName(unsigned short value) {
  switch (value) {
    case DIMENSIONAL: return "DIMENSIONAL";
    case FREESTREAM_PRESS_EQ_ONE: return "FREESTREAM_PRESS_EQ_ONE";
    case FREESTREAM_VEL_EQ_MACH: return "FREESTREAM_VEL_EQ_MACH";
    case FREESTREAM_VEL_EQ_ONE: return "FREESTREAM_VEL_EQ_ONE";
    case INITIAL_VALUES: return "INITIAL_VALUES";
    case REFERENCE_VALUES: return "REFERENCE_VALUES";
    default: return "UNKNOWN";
  }
}

/*! \brief Byte order of this host, as written in the JSON ("little" or "big"). */
const char* HostByteOrder() {
  const std::uint16_t one = 1;
  return *reinterpret_cast<const unsigned char*>(&one) == 1 ? "little" : "big";
}

/*! \brief JSON string body of s (without the enclosing quotes): quotes, backslashes and control characters escaped. */
std::string JsonEscaped(const std::string& s) {
  std::string out;
  for (const char c : s) {
    if (c == '"' || c == '\\') {
      out += '\\';
      out += c;
    } else if (static_cast<unsigned char>(c) < 0x20) {
      char code[16];
      std::snprintf(code, sizeof(code), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
      out += code;
    } else {
      out += c;
    }
  }
  return out;
}

std::string Trimmed(const std::string& s) {
  const auto first = s.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
}  // namespace

void WriteEPMField(const CEPMFieldSites& field, const CConfig& config, const CGeometry& geometry,
                   const std::string& objective, const std::string& extras_json) {
  using PassiveMPI = SelectMPIWrapper<passivedouble>::W;
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
  const std::string base = "sst_epm_field_" + objective;

  /*--- Own records: raw rows, all sites, in the registry order. ---*/
  const std::string own_name = base + "_rank" + std::to_string(rank) + ".bin";
  {
    std::ofstream output(own_name, std::ios::binary | std::ios::trunc);
    for (size_t site = 0; site < field.Size(); ++site)
      output.write(reinterpret_cast<const char*>(&field.Data(site)), sizeof(Record));
    output.close();
    if (!output) SU2_MPI::Error("Could not write " + own_name + ".", CURRENT_FUNCTION);
  }

  /*--- The master learns only the record counts. ---*/
  const unsigned long local_count = field.Size();
  std::vector<unsigned long> counts(size, 0);
  PassiveMPI::Gather(&local_count, 1, MPI_UNSIGNED_LONG, counts.data(), 1, MPI_UNSIGNED_LONG, MASTER_NODE,
                     SU2_MPI::GetComm());
  if (rank != MASTER_NODE) return;

  /*--- Build identity, e.g. -Dcpp_args=-DSU2_EPM_COMMIT=<sha> at meson setup. A value given as a string literal
   *    (-DSU2_EPM_COMMIT='"<sha>"') loses its quotes; anything else is escaped, so the JSON stays valid. ---*/
#ifdef SU2_EPM_COMMIT
  std::string commit = EPM_STRINGIZE(SU2_EPM_COMMIT);
  if (commit.size() >= 2 && commit.front() == '"' && commit.back() == '"') commit = commit.substr(1, commit.size() - 2);
#else
  std::string commit = "unknown";
#endif

  std::ostringstream json;
  json << std::setprecision(17);
  json << "{\n  \"columns\": [";
  const auto columns = ColumnNames();
  for (size_t i = 0; i < columns.size(); ++i) json << (i ? ", " : "") << '"' << columns[i] << '"';
  json << "],\n"
       << "  \"dtype\": \"float64\",\n"
       << "  \"byte_order\": \"" << HostByteOrder() << "\",\n"
       << "  \"record_bytes\": " << sizeof(Record) << ",\n"
       << "  \"ranks\": [";
  for (int r = 0; r < size; ++r)
    json << (r ? ", " : "") << "{\"file\": \"" << base << "_rank" << r << ".bin\", \"count\": " << counts[r] << '}';
  json << "],\n"
       << "  \"nDim\": " << geometry.GetnDim() << ",\n"
       << "  \"objective\": \"" << objective << "\",\n"
       << "  \"delta_b\": 1,\n"
       << "  \"urlx\": 1,\n"
       << "  \"gap_tolerance\": " << CEPMExtraction::gap_tolerance << ",\n";
  /*--- UQ_FADE_GAP: only with the fade, so that the file without it is unchanged (as the CSV column). ---*/
  const passivedouble fade_gap = SU2_TYPE::GetValue(config.GetUQ_FadeGap());
  if (fade_gap > 0.0) {
    json << "  \"uq_fade_gap\": " << fade_gap << ",\n"
         << "  \"fade_note\": \"dtau_*, dpk_* and dpw_* are weighted by the fade factor f = g^2/(g^2+c^2) of the "
            "case at the site (c = uq_fade_gap, g: anisotropy eigenvalue gap of the case direction)\",\n";
  }
  json << "  \"mpi_ranks\": " << size << ",\n"
       << "  \"su2_version\": \"8.6.0 Harrier\",\n"
       << "  \"source_commit\": \"" << JsonEscaped(commit) << "\",\n"
       << "  \"ref_dimensionalization\": \"" << NonDimName(config.GetRef_NonDim()) << "\",\n"
       << "  \"case_names\": [\"1C_KEEP\", \"1C_SWAP\", \"2C_KEEP\", \"2C_SWAP\", \"3C\"],\n"
       << "  \"kinds\": {\"EDGE\": " << static_cast<int>(CEPMExtraction::SiteKind::EDGE)
       << ", \"BOUNDARY\": " << static_cast<int>(CEPMExtraction::SiteKind::BOUNDARY)
       << ", \"NODE\": " << static_cast<int>(CEPMExtraction::SiteKind::NODE) << "},\n"
       << "  \"S_tau_note\": \"symmetrized over nDim block; only sym part meaningful\",\n"
       << "  \"normal_note\": \"EDGE: unit normal from key_a to key_b; BOUNDARY: outward unit normal\",\n"
       << "  \"extract_only_residuals\": " << (Trimmed(extras_json).empty() ? "null" : Trimmed(extras_json))
       << "\n}\n";

  const std::string json_name = base + ".json";
  std::ofstream output(json_name, std::ios::trunc);
  output << json.str();
  output.close();
  if (!output) SU2_MPI::Error("Could not write " + json_name + ".", CURRENT_FUNCTION);
}
