/*!
 * \file CNativeReferenceIO.hpp
 * \brief Versioned immutable geometry and accepted component bindings for adapted native mesh restarts.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once
#include "CNativeImport2D.hpp"
#include "CNativeGeometryValidation2D.hpp"
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>

namespace SU2NativeBoundary2D {

// Geometry/reference storage is replicated O(B), outside the per-cavity dependency budget. The sidecar records
// geometry and component identity, not boundary conditions. Decimal binary64 round trips use the classic locale.
constexpr size_t REFERENCE_FILE_LIMIT = 256 * 1024 * 1024;

inline std::string EncodeReference(const ReferenceState& state) {
  if (!state.original || state.marker_names.empty() || state.accepted_edges.empty())
    throw std::invalid_argument("Cannot persist an uninitialized native geometry reference.");
  size_t nFaces = 0;
  for (const auto& component : state.original->Components()) nFaces += component.nodes.size() - 1;
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::setprecision(std::numeric_limits<double>::max_digits10);
  out << "SU2_NATIVE_REFERENCE 1\n"
      << state.marker_names.size() << ' ' << nFaces << ' ' << state.accepted_edges.size() << '\n';
  for (const auto& name : state.marker_names) out << std::quoted(name) << '\n';
  for (const auto& component : state.original->Components())
    for (size_t i = 1; i < component.nodes.size(); ++i) {
      out << component.marker;
      for (const auto node : {component.nodes[i - 1], component.nodes[i]})
        out << ' ' << node.id << ' ' << node.p.x << ' ' << node.p.y << ' ' << state.original->IsFeature(node.p);
      out << '\n';
    }
  for (const auto& binding : state.accepted_edges) {
    for (const auto x : binding.first) out << x << ' ';
    out << binding.second << '\n';
  }
  if (!out || out.tellp() > std::streamoff(REFERENCE_FILE_LIMIT))
    throw std::runtime_error("Native reference sidecar exceeds its replicated file limit.");
  return out.str();
}

inline ReferenceState DecodeReference(const std::string& bytes) {
  if (bytes.size() > REFERENCE_FILE_LIMIT) throw std::invalid_argument("Native reference sidecar exceeds file limit.");
  std::istringstream in(bytes);
  in.imbue(std::locale::classic());
  std::string magic;
  unsigned version = 0;
  uint64_t nMarkers = 0, nFaces = 0, nBindings = 0;
  if (!(in >> magic >> version >> nMarkers >> nFaces >> nBindings) || magic != "SU2_NATIVE_REFERENCE" || version != 1 ||
      !nMarkers || nMarkers > 65535 || !nFaces || nFaces > bytes.size() / 14 || !nBindings ||
      nBindings > bytes.size() / 9)
    throw std::invalid_argument("Invalid native reference sidecar header/counts/version.");
  ReferenceState result;
  std::set<std::string> names;
  for (uint64_t m = 0; m < nMarkers; ++m) {
    std::string name;
    if (!(in >> std::quoted(name)) || name.empty() || name.size() > 1024 || !names.insert(name).second)
      throw std::invalid_argument("Invalid native reference marker name.");
    result.marker_names.push_back(std::move(name));
  }
  std::vector<PolylineReference::Face> faces;
  for (uint64_t f = 0; f < nFaces; ++f) {
    PolylineReference::Face face;
    if (!(in >> face.marker) || face.marker < 0 || uint64_t(face.marker) >= nMarkers)
      throw std::invalid_argument("Invalid native reference original marker.");
    for (auto* node : {&face.a, &face.b}) {
      int feature = 0;
      if (!(in >> node->id >> node->p.x >> node->p.y >> feature) || (feature != 0 && feature != 1))
        throw std::invalid_argument("Invalid native reference original point.");
      node->fixed = feature ? FEATURE : 0;
    }
    faces.push_back(face);
  }
  // Explicit original features are persisted; rebuilding the same graph retains deterministic component IDs.
  ValidatePhysicalGraph(faces);
  result.original = std::make_shared<PolylineReference>(faces, 180);
  for (uint64_t f = 0; f < nBindings; ++f) {
    FaceCoordinates coordinates;
    int component = 0;
    for (auto& x : coordinates)
      if (!(in >> x) || !std::isfinite(x)) throw std::invalid_argument("Invalid native reference accepted coordinate.");
    if (!(in >> component)) throw std::invalid_argument("Invalid native reference component binding.");
    result.original->Marker(component);  // Validate the component before storing anything.
    const Point a{coordinates[0], coordinates[1]}, b{coordinates[2], coordinates[3]};
    if (CoordinateKey(a, b) != coordinates || (a.x == b.x && a.y == b.y) ||
        !result.accepted_edges.emplace(coordinates, component).second)
      throw std::invalid_argument("Duplicate or noncanonical native reference accepted edge.");
  }
  in >> std::ws;
  if (!in.eof()) throw std::invalid_argument("Trailing data in native reference sidecar.");
  return result;
}

// Collective root I/O, passive byte broadcast, and publication only after all ranks have decoded successfully.
inline bool LoadReference(const std::string& filename, ReferenceState& state) {
  CLocalFailure failure;
  std::vector<char> bytes;
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    try {
      if (std::filesystem::exists(filename)) {
        const auto length = std::filesystem::file_size(filename);
        if (!length || length > REFERENCE_FILE_LIMIT) throw std::runtime_error("Invalid native reference file size.");
        bytes.resize(length);
        std::ifstream input(filename, std::ios::binary);
        if (!input.read(bytes.data(), bytes.size())) throw std::runtime_error("Cannot read native reference sidecar.");
      }
    } catch (const std::exception& error) {
      failure.Set(1, 0, filename + ": " + error.what());
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  CPassiveComm::BcastRounds(bytes, MASTER_NODE);
  if (bytes.empty()) return false;
  ReferenceState staged;
  try {
    staged = DecodeReference(std::string(bytes.begin(), bytes.end()));
  } catch (const std::exception& error) {
    failure.Set(1, 0, filename + ": " + error.what());
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  state = std::move(staged);
  return true;
}

inline void WriteReference(const std::string& filename, const ReferenceState& state) {
  CLocalFailure failure;
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    try {
      const auto bytes = EncodeReference(state);
      const auto temporary = filename + ".tmp";
      {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output.write(bytes.data(), bytes.size()))
          throw std::runtime_error("Cannot write native reference sidecar.");
        output.close();
        if (!output) throw std::runtime_error("Cannot close native reference sidecar.");
      }
      if (std::rename(temporary.c_str(), filename.c_str()))
        throw std::runtime_error("Cannot publish native reference sidecar.");
    } catch (const std::exception& error) {
      failure.Set(1, 0, filename + ": " + error.what());
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
}

}  // namespace SU2NativeBoundary2D
