/*!
 * \file CNativeReferenceIO3D.cpp
 * \brief Bounded passive I/O and validated planar authority publication.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/adaptation/CNativeReferenceIO3D.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
namespace SU2Native3D {
std::string EncodeReference(const ReferenceState& state) {
  if (!state.original || state.marker_names.empty() || state.facets.empty())
    throw std::invalid_argument("Cannot persist an uninitialized native3D reference.");
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(17);
  out << "SU2_NATIVE_REFERENCE_3D 1\n" << state.marker_names.size() << ' ' << state.facets.size() << '\n';
  for (const auto& name : state.marker_names) out << std::quoted(name) << '\n';
  for (const auto& f : state.facets) {
    out << f.id << ' ' << f.marker;
    for (auto n : f.v) out << ' ' << n.id << ' ' << n.p.x << ' ' << n.p.y << ' ' << n.p.z;
    out << '\n';
  }
  if (!out || out.tellp() > std::streamoff(ReferenceFileLimit3D))
    throw std::runtime_error("Native3D reference exceeds file limit.");
  auto bytes = out.str();
  DecodeReference(bytes);
  return bytes;
}
ReferenceState DecodeReference(const std::string& bytes) {
  if (bytes.size() > ReferenceFileLimit3D) throw std::invalid_argument("Native3D reference exceeds file limit.");
  std::istringstream in(bytes);
  in.imbue(std::locale::classic());
  std::string magic;
  unsigned version = 0;
  uint64_t markers = 0, facets = 0;
  if (!(in >> magic >> version >> markers >> facets) || magic != "SU2_NATIVE_REFERENCE_3D" || version != 1 || !facets ||
      facets > 4 * MaximumCavityCells || !markers || markers > facets)
    throw std::invalid_argument("Invalid native3D reference header/counts/version.");
  ReferenceState result;
  std::set<std::string> names;
  std::set<int> used;
  std::vector<Face> skin;
  for (uint64_t i = 0; i < markers; ++i) {
    std::string name;
    if (!(in >> std::quoted(name)) || name.empty() || name.size() > 1024 || !names.insert(name).second)
      throw std::invalid_argument("Invalid native3D reference marker name.");
    result.marker_names.push_back(std::move(name));
  }
  if (!std::is_sorted(result.marker_names.begin(), result.marker_names.end()))
    throw std::invalid_argument("Noncanonical native3D marker ordering.");
  for (uint64_t i = 0; i < facets; ++i) {
    Facet f;
    if (!(in >> f.id >> f.marker) || f.marker < 0 || uint64_t(f.marker) >= markers)
      throw std::invalid_argument("Invalid native3D reference facet marker.");
    for (auto& n : f.v)
      if (!(in >> n.id >> n.p.x >> n.p.y >> n.p.z)) throw std::invalid_argument("Invalid native3D reference point.");
    used.insert(f.marker);
    skin.push_back({f.v[0].id, f.v[1].id, f.v[2].id});
    result.facets.push_back(f);
  }
  in >> std::ws;
  if (!in.eof() || used.size() != markers) throw std::invalid_argument("Trailing data or unused native3D markers.");
  ValidateSurfaceTopology(skin, true);
  result.original = std::make_shared<PlanarReference>(result.facets);
  return result;
}
bool LoadReference(const std::string& filename, ReferenceState& state) {
  CLocalFailure failure;
  std::vector<char> bytes;
  if (!SU2_MPI::GetRank()) try {
      if (std::filesystem::exists(filename)) {
        const auto n = std::filesystem::file_size(filename);
        if (!n || n > ReferenceFileLimit3D) throw std::runtime_error("Invalid native3D reference file size.");
        bytes.resize(n);
        std::ifstream input(filename, std::ios::binary);
        if (!input.read(bytes.data(), bytes.size())) throw std::runtime_error("Cannot read native3D reference.");
      }
    } catch (const std::exception& e) {
      failure.Set(1, 0, filename + ": " + e.what());
    }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  CPassiveComm::BcastRounds(bytes, 0);
  if (bytes.empty()) return false;
  ReferenceState pending;
  try {
    pending = DecodeReference(std::string(bytes.begin(), bytes.end()));
  } catch (const std::exception& e) {
    failure.Set(1, 0, filename + ": " + e.what());
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  state = std::move(pending);
  return true;
}
void WriteReference(const std::string& filename, const ReferenceState& state) {
  CLocalFailure failure;
  if (!SU2_MPI::GetRank()) try {
      const auto bytes = EncodeReference(state), temporary = filename + ".tmp";
      {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output.write(bytes.data(), bytes.size())) throw std::runtime_error("Cannot write native3D reference.");
        output.close();
        if (!output) throw std::runtime_error("Cannot close native3D reference.");
      }
      if (std::rename(temporary.c_str(), filename.c_str()))
        throw std::runtime_error("Cannot publish native3D reference.");
    } catch (const std::exception& e) {
      failure.Set(1, 0, filename + ": " + e.what());
    }
  CollectiveFailure(failure, CURRENT_FUNCTION);
}
}  // namespace SU2Native3D
