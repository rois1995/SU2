/*!
 * \file CNativeRegionField3D.cpp
 * \brief Original sensor and full-segment queries without a whole-region donor gather.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeRegionField3D.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <set>
#include <stdexcept>
namespace SU2Native3D {
RegionFieldService::RegionFieldService(World& w, DonorImport& s, const PlanarReference& g, MetricComposition c,
                                       MetricEdgeBreakpoints b, bool t, size_t cap, size_t held,
                                       std::function<void(const FrozenField&)> observe, std::function<void()> tick)
    : world(w),
      source(s),
      geometry(g),
      composition(std::move(c)),
      bands(std::move(b)),
      timings(t),
      ceiling(cap),
      caller_bytes(held),
      retired(std::move(observe)),
      checkpoint(std::move(tick)) {}
void RegionFieldService::Retire() {
  if (field && retired) retired(*field);
  field.reset();
}
void RegionFieldService::Load(std::vector<DonorCell> records) {
  std::set<std::array<Id, 4>> keys;
  for (const auto& row : records) keys.insert(row.Key());
  // Keep the complete certified new footprint; old records only supply local interior hits.
  for (const auto& row : resident)
    if (records.size() < MaximumDonors && keys.insert(row.Key()).second) records.push_back(row);
  const auto components = AssociatedExtensions(records, geometry);
  auto next = std::make_unique<FrozenField>(records, composition, timings, components, bands);
  Retire();
  resident.swap(records);
  field.swap(next);
  stats.maximum_resident = std::max(stats.maximum_resident, resident.size());
}
DonorPatch RegionFieldService::Fetch(const Segment* segment) {
  CLocalFailure failure;
  std::vector<Segment> query;
  try {
    if (segment) query.push_back(*segment);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  const auto rejected = world.elect(failure);
  if (rejected.any) return {{}, false, rejected.message};
  ++stats.imports;
  return source.ImportEdgeSegments(query, ceiling, transfer_memory::Add(caller_bytes, RegionFieldWorkspaceBytes));
}
Tensor RegionFieldService::PointMetric(Point point) {
  if (checkpoint) checkpoint();
  ++stats.point_calls;
  for (double x : {point.x, point.y, point.z})
    if (!std::isfinite(x)) throw std::invalid_argument("Nonfinite regional point query.");
  const Key key{point.x, point.y, point.z};
  auto found = sensors.find(key);
  if (found != sensors.end()) {
    ++stats.sensor_hits;
    return composition ? composition(point, found->second) : found->second;
  }
  FieldSample sample;
  bool certified = false;
  if (field) try {
      sample = field->Query(point);
      // A strict interior physical donor is unique in the admitted embedded original source.
      // Boundary ties and extensions need a full query footprint: a missing primary must never lose priority.
      certified = sample.extension_component == UINT64_MAX &&
                  std::all_of(sample.weights.begin(), sample.weights.end(), [](long double w) { return w > 0; });
      if (certified) ++stats.interior_hits;
    } catch (const std::exception&) {
      certified = false;
    }
  if (!certified) {
    const auto preparation=transfer_memory::Add(caller_bytes,RegionFieldWorkspaceBytes,
        transfer_memory::Mul(geometry.ExtensionComponents().size(),sizeof(Id)),sizeof(Point));
    if(preparation>ceiling)throw std::runtime_error("Regional point preparation exceeds storage.");
    const auto& extensions=geometry.ExtensionComponents();
    const std::vector<Point> query{point};
    world.maximum(1);
    const auto answer=source.SamplePoints(query,extensions,ceiling,
        transfer_memory::Add(caller_bytes,RegionFieldWorkspaceBytes));
    if(!answer.valid)throw std::runtime_error(answer.reason);
    if(answer.samples.size()!=1)throw std::runtime_error("Invalid regional canonical point reply.");
    ++stats.sampled_points;
    sample.sensor=answer.samples.front().sensor;
    sample.target=composition ? composition(point,sample.sensor) : sample.sensor;
    ValidateTensor(sample.target);
  }
  if (sensors.size() == MaximumSensorSamples) {
    sensors.erase(sensor_order.front());
    sensor_order.pop_front();
  }
  sensors.emplace(key, sample.sensor);
  sensor_order.push_back(key);
  return sample.target;
}
long double RegionFieldService::EdgeLength(Point a, Point b) {
  if (checkpoint) checkpoint();
  ++stats.edge_calls;
  for (Point p : {a, b})
    for (double x : {p.x, p.y, p.z})
      if (!std::isfinite(x)) throw std::invalid_argument("Nonfinite regional edge query.");
  EdgeKey key{{{a.x, a.y, a.z}, {b.x, b.y, b.z}}};
  if (key[1] < key[0]) std::swap(key[0], key[1]);
  auto found = edges.find(key);
  if (found != edges.end()) {
    ++stats.edge_hits;
    return found->second;
  }
  // Preserve every positive-length interval and its priority; isolated contacts carry no metric-edge mass.
  world.maximum(2);
  const Segment query{a, b};
  auto patch = Fetch(&query);
  if (!patch.valid) throw std::runtime_error(patch.reason);
  Load(std::move(patch.cells));
  const auto length = field->TargetEdgeLength(a, b);
  if (edges.size() == MaximumSensorSamples) {
    edges.erase(edge_order.front());
    edge_order.pop_front();
  }
  edges.emplace(key, length);
  edge_order.push_back(key);
  return length;
}
void RegionFieldService::Run(const std::function<void(const RegionMetric&)>& root, const std::vector<DonorCell>& seed) {
  std::exception_ptr failure;
  if (!world.rank) {
    try {
      if (!seed.empty()) {
        Load(seed);
        RegionMetric metric(*field);
        root(metric);
      } else {
        RegionMetric metric([&](Point p) { return PointMetric(p); },
                            [&](Point a, Point b) { return EdgeLength(a, b); });
        root(metric);
      }
    } catch (...) {
      failure = std::current_exception();
    }
    world.maximum(0);  // Close after success or any constructor/cache/field exception.
    Retire();
  } else {
    while (const auto command=world.maximum(0)) {
      // Rank0 owns the constructor exception and closes after either bounded query kind fails.
      try {
        if(command==1)(void)source.SamplePoints({}, {},ceiling,transfer_memory::Add(caller_bytes,RegionFieldWorkspaceBytes));
        else (void)Fetch(nullptr);
      } catch (...) {
        failure = std::current_exception();
      }
    }
  }
  if (failure) std::rethrow_exception(failure);
}
}  // namespace SU2Native3D
