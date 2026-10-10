/*!
 * \file CNativeField3D.cpp
 * \brief Canonical indexed P1 sensors; actual-query composition and exclusive query profiling.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeField3D.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <stdexcept>
#include <type_traits>

namespace SU2Native3D {
namespace {
class Timer {
 public:
  Timer(double& value, bool enabled) : value(value), enabled(enabled) {
    if (enabled) start = std::chrono::steady_clock::now();
  }
  ~Timer() {
    if (enabled) value += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
  }

 private:
  double& value;
  bool enabled;
  std::chrono::steady_clock::time_point start;
};
std::array<double, 6> Components(Tensor m) { return {m.xx, m.xy, m.xz, m.yy, m.yz, m.zz}; }
void Canonical(std::vector<DonorCell>& original) {
  if (original.size() > MaximumDonors) throw std::invalid_argument("Native 3D original donor patch exceeds budget.");
  std::sort(original.begin(), original.end(), [](const DonorCell& a, const DonorCell& b) { return a.Key() < b.Key(); });
  for (size_t i = 1; i < original.size(); ++i)
    if (original[i].Key() == original[i - 1].Key())
      throw std::invalid_argument("Duplicate native 3D original donor simplex.");
}
Tensor Blend(const DonorCell& donor, const std::array<long double, 4>& weights) {
  std::array<long double, 6> total{};
  for (size_t i = 0; i < 4; ++i) {
    const auto m = Components(donor.sensor[i]);
    for (size_t j = 0; j < 6; ++j) total[j] += weights[i] * m[j];
  }
  return {double(total[0]), double(total[1]), double(total[2]), double(total[3]), double(total[4]), double(total[5])};
}
}  // namespace
std::array<Id, 4> DonorCell::Key() const {
  std::array<Id, 4> key;
  for (size_t i = 0; i < 4; ++i) key[i] = cell.v[i].id;
  std::sort(key.begin(), key.end());
  return key;
}
size_t FrozenField::Hash::operator()(const std::array<double, 3>& point) const {
  size_t value = 0;
  for (auto x : point) value ^= std::hash<double>{}(x) + size_t(0x9e3779b9) + (value << 6) + (value >> 2);
  return value;
}
FrozenField::FrozenField(std::vector<DonorCell> original, MetricComposition combine, bool enabled)
    : donors(std::move(original)), composition(std::move(combine)), timings(enabled) {
  Timer timer(stats.build_seconds, timings);
  Canonical(donors);
  std::map<Id, std::pair<Point, std::array<double, 6>>> nodes;
  for (const auto& donor : donors) {
    if (!(Orientation(donor.cell.v[0].p, donor.cell.v[1].p, donor.cell.v[2].p, donor.cell.v[3].p, &stats.kernel) > 0))
      throw std::invalid_argument("Nonpositive native 3D original donor.");
    const auto key = donor.Key();
    if (std::adjacent_find(key.begin(), key.end()) != key.end())
      throw std::invalid_argument("Repeated native donor vertex.");
    for (size_t i = 0; i < 4; ++i) {
      const auto& n = donor.cell.v[i];
      const auto inserted = nodes.emplace(n.id, std::make_pair(n.p, Components(donor.sensor[i])));
      if (inserted.second)
        ValidateTensor(donor.sensor[i], &stats.kernel);
      else {
        const auto& old = inserted.first->second;
        if (old.first.x != n.p.x || old.first.y != n.p.y || old.first.z != n.p.z ||
            old.second != Components(donor.sensor[i]))
          throw std::invalid_argument("Inconsistent shared native 3D original sensor/coordinates.");
      }
    }
  }
  if (donors.size() <= 1) return;  // One donor needs no tree or spatial traversal buffers.
  if constexpr (std::is_same<su2double, double>::value) {
    std::vector<su2double> coordinates;
    std::vector<unsigned long> connectivity, ids;
    std::vector<unsigned short> types(donors.size(), LINE), markers(donors.size(), 0);
    coordinates.reserve(6 * donors.size());
    connectivity.reserve(2 * donors.size());
    ids.reserve(donors.size());
    for (const auto& donor : donors) {
      for (int side = 0; side < 2; ++side)
        for (int axis = 0; axis < 3; ++axis) {
          double bound = axis == 0 ? donor.cell.v[0].p.x : axis == 1 ? donor.cell.v[0].p.y : donor.cell.v[0].p.z;
          for (const auto& n : donor.cell.v) {
            const double p = axis == 0 ? n.p.x : axis == 1 ? n.p.y : n.p.z;
            bound = side ? std::max(bound, p) : std::min(bound, p);
          }
          coordinates.push_back(bound);
        }
      connectivity.push_back(connectivity.size());
      connectivity.push_back(connectivity.size());
      ids.push_back(ids.size());
    }
    // Reuse the 2D import's two-corner LINE box representation; no ADT element-containment tolerance is used.
    search = std::make_unique<CADTElemClass>(3, coordinates, connectivity, types, markers, ids, false);
    stats.index_bytes = search->GetAllocatedBytes();
  }
}
FrozenField::~FrozenField() = default;
void FrozenField::FindCandidates(Point lo, Point hi) {
  if (search) {
    const std::array<su2double, 3> lower{lo.x, lo.y, lo.z}, upper{hi.x, hi.y, hi.z};
    search->DetermineIntersectingElements(lower.data(), upper.data(), candidates);
  } else {
    candidates.resize(donors.size());
    for (size_t i = 0; i < donors.size(); ++i) candidates[i] = i;
  }
  std::sort(candidates.begin(), candidates.end());
}
long double FrozenField::SensorEdgeLength(Point a, Point b) {
  Timer edge_time(stats.edge_seconds, timings);
  ++stats.edge_requests;
  try {
    if (!(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z) && std::isfinite(b.x) && std::isfinite(b.y) &&
          std::isfinite(b.z)))
      throw std::invalid_argument("Nonfinite native 3D sensor edge.");
    if (a.x == b.x && a.y == b.y && a.z == b.z) throw std::invalid_argument("Native 3D sensor edge must be nonzero.");
    std::vector<const Cell*> cells;
    {
      Timer timer(stats.edge_search_seconds, timings);
      FindCandidates({std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z)},
                     {std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z)});
      stats.edge_box_candidates += candidates.size();
      stats.maximum_edge_candidates = std::max(stats.maximum_edge_candidates, candidates.size());
      cells.reserve(candidates.size());
      for (auto i : candidates) cells.push_back(&donors.at(i).cell);
    }
    std::vector<SegmentPiece> pieces;
    {
      Timer timer(stats.edge_trace_seconds, timings);
      pieces = TraceSegment(cells, a, b, &stats.kernel);
    }
    stats.edge_pieces += pieces.size();
    stats.maximum_edge_pieces = std::max(stats.maximum_edge_pieces, pieces.size());
    Timer timer(stats.edge_integral_seconds, timings);
    long double total = 0;
    for (const auto& piece : pieces) {
      const auto& donor = donors[candidates[piece.donor]];
      const auto first = MetricNorm(a, b, Blend(donor, piece.begin_weights), &stats.kernel);
      const auto last = MetricNorm(a, b, Blend(donor, piece.end_weights), &stats.kernel);
      const auto largest = std::max(first, last), r = std::min(first, last) / largest;
      // Integral of sqrt(affine quadratic form), avoiding subtraction of nearly equal cubes.
      total += piece.width * largest * (2.L / 3) * (1 + r + r * r) / (1 + r);
    }
    if (!(total > 0 && std::isfinite(total)))
      throw std::runtime_error("Unrepresentable native 3D original sensor edge length.");
    return total;
  } catch (...) {
    ++stats.edge_failures;
    throw;
  }
}
FieldSample FrozenField::Query(Point point) {
  Timer query_time(stats.query_seconds, timings);
  ++stats.requests;
  try {
    if (!(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z))) {
      ++stats.invalid_queries;
      throw std::invalid_argument("Nonfinite native 3D sensor query.");
    }
    const std::array<double, 3> key{point.x, point.y, point.z};
    SensorValue value;
    bool found = false;
    {
      Timer timer(stats.cache_seconds, timings);
      const auto hit = cache.find(key);
      if (hit != cache.end()) {
        value = hit->second;
        found = true;
        ++stats.hits;
      } else
        ++stats.misses;
    }
    if (!found) {
      {
        Timer timer(stats.search_seconds, timings);
        FindCandidates(point, point);
        stats.box_candidates += candidates.size();
        stats.maximum_candidates = std::max(stats.maximum_candidates, candidates.size());
        for (auto index : candidates) {
          const auto& c = donors.at(index).cell;
          const Tetrahedron t{c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p};
          // Inflated broad-phase boxes never authorize containment.
          bool boxed = true;
          for (size_t axis = 0; axis < 3; ++axis) {
            const bool vertex_lo = std::any_of(
                t.begin(), t.end(), [&](Point p) { return (axis == 0 ? p.x : axis == 1 ? p.y : p.z) <= key[axis]; });
            const bool vertex_hi = std::any_of(
                t.begin(), t.end(), [&](Point p) { return (axis == 0 ? p.x : axis == 1 ? p.y : p.z) >= key[axis]; });
            boxed = boxed && vertex_lo && vertex_hi;
          }
          if (!boxed) continue;
          ++stats.containment_tests;
          if (Barycentric(t, point, value.weights, &stats.kernel)) {
            value.donor = index;
            found = true;
            break;
          }
        }
        if (!found) throw std::runtime_error("Query outside native 3D immutable original donors.");
      }
      {
        Timer timer(stats.interpolation_seconds, timings);
        ++stats.interpolations;
        value.tensor = Blend(donors[value.donor], value.weights);
        ValidateTensor(value.tensor, &stats.kernel);
      }
      {
        Timer timer(stats.cache_seconds, timings);
        if (cache.size() == MaximumSensorSamples) {
          cache.erase(recent.front());
          recent.pop_front();
          ++stats.evictions;
        }
        recent.push_back(key);
        try {
          cache.emplace(key, value);
        } catch (...) {
          recent.pop_back();
          throw;
        }
        stats.maximum_cache_entries = std::max(stats.maximum_cache_entries, cache.size());
      }
    }
    FieldSample sample{value.tensor, value.tensor, donors[value.donor].Key(), value.weights};
    if (composition) {
      Timer timer(stats.composition_seconds, timings);
      ++stats.compositions;
      sample.target = composition(point, sample.sensor);
      ValidateTensor(sample.target, &stats.kernel);
    }
    return sample;
  } catch (...) {
    ++stats.failures;
    throw;
  }
}
}  // namespace SU2Native3D
