/*!
 * \file CNativeMarkerTolerance3D.hpp
 * \brief Event-local native geometry tolerances indexed by reference marker.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once

#include "CTransferMemory.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace SU2Native3D {

class MarkerTolerance3D {
  struct Row {
    std::string name;
    double value;
    int index_class;
  };
  std::vector<Row> rows;
  double strict;

 public:
  using Configured = std::map<std::string, double>;

  static size_t ConstructionBytes(const std::vector<std::string>& markers, size_t configured) {
    auto bytes = transfer_memory::Add(sizeof(MarkerTolerance3D), transfer_memory::Mul(markers.size(), sizeof(Row)),
                                      transfer_memory::Mul(configured, sizeof(double)));
    for (const auto& name : markers) bytes = transfer_memory::Add(bytes, name.size() + 1);
    return bytes;
  }

  MarkerTolerance3D(const Configured& configured, const std::vector<std::string>& marker_names, double max_coordinate) {
    if (configured.empty()) throw std::invalid_argument("Native marker tolerance table requires a list.");
    for (const auto& name : marker_names)
      if (!configured.count(name))
        throw std::invalid_argument("ADAP_HAUSD: with ADAP_REMESHER= NATIVE_CAVITY every boundary marker needs a value; missing: " + name);
    std::vector<double> classes;
    classes.reserve(configured.size());
    for (const auto& entry : configured) classes.push_back(entry.second);
    std::sort(classes.begin(), classes.end());
    classes.erase(std::unique(classes.begin(), classes.end()), classes.end());
    strict = classes.front();
    const auto floor = std::max(1e-12 * max_coordinate,
        16 * std::nextafter(std::numeric_limits<double>::min(), std::numeric_limits<double>::infinity()));
    if (strict < floor) {
      std::ostringstream message;
      message << "ADAP_HAUSD: the smallest value is below the native floor " << std::setprecision(17) << floor << '.';
      throw std::invalid_argument(message.str());
    }
    rows.reserve(marker_names.size());
    for (const auto& marker : marker_names) {
      const auto& entry = *configured.find(marker);
      const auto offset = std::lower_bound(classes.begin(), classes.end(), entry.second) - classes.begin();
      if (offset > std::numeric_limits<int>::max() / 4)
        throw std::length_error("Native tolerance role is outside the integer domain.");
      rows.push_back({entry.first, entry.second, static_cast<int>(offset)});
    }
  }

  size_t Bytes() const {
    auto bytes = transfer_memory::Add(sizeof(*this), transfer_memory::Bytes(rows));
    for (const auto& row : rows) bytes = transfer_memory::Add(bytes, transfer_memory::Bytes(row.name));
    return bytes;
  }
  double Strict() const { return strict; }
  double Marker(int marker) const { return rows.at(marker).value; }
  int Class(int marker) const { return rows.at(marker).index_class; }

  void WriteMarkers(std::ostream& stream) const {
    for (size_t i = 0; i < rows.size(); ++i) {
      const auto& row = rows[i];
      if (i) stream << ',';
      stream << row.name << ':' << i << ':' << row.value << ':' << row.index_class;
    }
  }

  // Configured is sorted by name. Include the field terminator and all value bits.
  static uint64_t Identity(const Configured& configured) {
    if (configured.empty()) return 0;
    uint64_t word = 14695981039346656037ULL;
    const auto add = [&](unsigned char byte) { word = (word ^ byte) * 1099511628211ULL; };
    for (const auto& entry : configured) {
      for (unsigned char byte : entry.first) add(byte);
      add(0);
      uint64_t value;
      static_assert(sizeof(value) == sizeof(entry.second));
      std::memcpy(&value, &entry.second, sizeof(value));
      for (unsigned byte = 0; byte < sizeof(value); ++byte) add((value >> (8 * byte)) & 255);
    }
    return word ? word : 1;
  }
};

inline thread_local const MarkerTolerance3D* active_marker_tolerance = nullptr;

inline double MarkerTolerance(int marker, double scalar) {
  return active_marker_tolerance ? active_marker_tolerance->Marker(marker) : scalar;
}

template <class Reference>
double FaceTolerance(const Reference& geometry, uint64_t facet, double scalar) {
  return active_marker_tolerance ? active_marker_tolerance->Marker(geometry.Get(facet).marker) : scalar;
}

}  // namespace SU2Native3D
