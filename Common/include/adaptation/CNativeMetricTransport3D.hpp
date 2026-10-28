/*!
 * \file CNativeMetricTransport3D.hpp
 * \brief Cavity-local transport of nodal sensor metrics.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include "CNativeDonor3D.hpp"
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
#include <stdexcept>

namespace SU2Native3D {
inline std::array<long double, 4> MetricAffineWeights(const Cell& cell, Point p) {
  long double a[3][3], r[3];
  const auto origin = cell.v[0].p;
  for (int i = 0; i < 3; ++i) {
    const auto q = cell.v[i + 1].p;
    a[0][i] = (long double)q.x - origin.x;
    a[1][i] = (long double)q.y - origin.y;
    a[2][i] = (long double)q.z - origin.z;
  }
  r[0] = (long double)p.x - origin.x; r[1] = (long double)p.y - origin.y; r[2] = (long double)p.z - origin.z;
  const auto det = [](const long double m[3][3]) {
    return m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1]) -
           m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0]) +
           m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
  };
  const auto d = det(a);
  if (!std::isfinite(d) || d == 0) throw std::invalid_argument("Degenerate metric donor.");
  long double x[3];
  for (int c = 0; c < 3; ++c) {
    long double b[3][3];
    for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) b[i][j] = j == c ? r[i] : a[i][j];
    x[c] = det(b) / d;
  }
  return {1-x[0]-x[1]-x[2], x[0], x[1], x[2]};
}
inline bool SameMetric(const Tensor& a, const Tensor& b) {
  return a.xx == b.xx && a.xy == b.xy && a.xz == b.xz && a.yy == b.yy && a.yz == b.yz && a.zz == b.zz;
}
inline std::vector<DonorCell> MetricDonors(const std::vector<Cell>& cells, const std::map<Id, Tensor>& metrics) {
  if (cells.empty() || metrics.empty()) throw std::invalid_argument("Empty metric transport cavity.");
  std::vector<DonorCell> donors;
  donors.reserve(cells.size());
  for (const auto& cell : cells) {
    DonorCell donor;
    donor.cell = cell;
    for (size_t i = 0; i < 4; ++i) {
      const auto p = cell.v[i].p;
      if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
        throw std::invalid_argument("Nonfinite metric donor coordinate.");
      const auto found = metrics.find(cell.v[i].id);
      if (found == metrics.end()) throw std::invalid_argument("Missing nodal sensor metric.");
      ValidateTensor(found->second);
      donor.sensor[i] = found->second;
    }
    const auto volume = Orientation(cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p);
    if (!std::isfinite(volume) || volume == 0) throw std::invalid_argument("Degenerate metric donor.");
    donors.push_back(donor);
  }
  return donors;
}

/*! Local best-parent sensor evaluation, including points outside the old cavity. */
inline DonorSample EvaluateCavitySample(const std::vector<DonorCell>& donors, Point p) {
  if (donors.empty() || !std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
    throw std::invalid_argument("Empty cavity or nonfinite metric sample coordinate.");
  for(const auto& donor:donors)for(unsigned k=0;k<4;++k) {
    const auto q=donor.cell.v[k].p;
    if(p.x==q.x && p.y==q.y && p.z==q.z) {
      DonorSample sample;sample.cell=donor.cell.id;sample.sensor=donor.sensor[k];sample.weights[k]=1;
      for(unsigned j=0;j<4;++j)sample.nodes[j]=donor.cell.v[j].id;
      ValidateTensor(sample.sensor);return sample;
    }
  }
  size_t best = 0; long double minimum = -std::numeric_limits<long double>::infinity();
  std::array<long double, 4> selected{};
  for (size_t i = 0; i < donors.size(); ++i) {
    for (const auto& tensor : donors[i].sensor) ValidateTensor(tensor);
    const auto w = MetricAffineWeights(donors[i].cell, p);
    for (auto value : w) if (!std::isfinite(value)) throw std::invalid_argument("Nonfinite metric sample weights.");
    const auto score = *std::min_element(w.begin(), w.end());
    if (score > minimum || (score == minimum && donors[i].Key() < donors[best].Key())) {
      minimum = score; best = i; selected = w;
    }
  }
  long double sum = 0; for (auto& w : selected) { w = std::max(0.L, w); sum += w; }
  if (!std::isfinite(sum) || sum <= 0) throw std::invalid_argument("Invalid metric sample weights.");
  for (auto& w : selected) w /= sum;
  DonorSample sample; sample.weights = selected; sample.cell = donors[best].cell.id;
  for (size_t i = 0; i < 4; ++i) sample.nodes[i] = donors[best].cell.v[i].id;
  sample.sensor = InterpolateSensor(donors[best], selected); ValidateTensor(sample.sensor);
  return sample;
}
inline Tensor EvaluateCavitySensor(const std::vector<DonorCell>& donors, Point p) {
  return EvaluateCavitySample(donors, p).sensor;
}

/*! Preserve existing values; interpolate new nodes from the best old cavity tetrahedron. */
inline std::map<Id, Tensor> TransportNodalMetrics(const std::vector<DonorCell>& donors,
                                                  const std::vector<Node>& nodes,
                                                  const std::map<Id, Tensor>& metrics) {
  if (donors.empty() || nodes.empty() || metrics.empty()) throw std::invalid_argument("Empty metric transport input.");
  std::vector<Cell> cells;
  std::map<Id, Tensor> donor_metrics;
  for (const auto& donor : donors) {
    cells.push_back(donor.cell);
    for (size_t i = 0; i < 4; ++i) {
      ValidateTensor(donor.sensor[i]);
      const auto inserted = donor_metrics.emplace(donor.cell.v[i].id, donor.sensor[i]);
      if (!inserted.second && !SameMetric(inserted.first->second, donor.sensor[i]))
        throw std::invalid_argument("Inconsistent shared nodal sensor metrics.");
    }
  }
  MetricDonors(cells, donor_metrics);
  auto result = metrics;
  for (const auto& entry : metrics) ValidateTensor(entry.second);
  for (const auto& node : nodes) {
    const auto p = node.p;
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
      throw std::invalid_argument("Nonfinite metric transport coordinate.");
    if (result.count(node.id)) continue;
    result.emplace(node.id, EvaluateCavitySensor(donors, p));
  }
  return result;
}
/*! Assign replacement sensor attributes from the complete old cavity. */
inline void PublishMetrics(const std::vector<OwnedCell>& old, std::vector<OwnedCell>& fresh) {
  if (fresh.empty()) return;
  std::vector<Cell> cells;
  std::map<Id, Tensor> metrics;
  for (const auto& c : old) {
    if (!c.sensor_present) throw std::invalid_argument("Missing current nodal sensor metrics.");
    cells.push_back(c.cell);
    for (size_t i = 0; i < 4; ++i) {
      ValidateTensor(c.sensor[i]);
      const auto added = metrics.emplace(c.cell.v[i].id, c.sensor[i]);
      if (!added.second && !SameMetric(added.first->second, c.sensor[i]))
        throw std::invalid_argument("Inconsistent shared nodal sensor metrics.");
    }
  }
  const auto donors = MetricDonors(cells, metrics);
  std::vector<Node> nodes;
  for (const auto& c : fresh) for (const auto& n : c.cell.v) nodes.push_back(n);
  const auto transported = TransportNodalMetrics(donors, nodes, metrics);
  for (auto& c : fresh) {
    for (size_t i = 0; i < 4; ++i) c.sensor[i] = transported.at(c.cell.v[i].id);
    c.sensor_present = true;
  }
}

/*! Bootstrap imports remain collective; cavity evaluation uses only the current dependency context. */
class CavityMetricSource {
 public:
  CavityMetricSource(World& world, Owned& current, bool timings = false)
      : world(world), current(current), bootstrap(world, timings) {}
  void SetCavity(const std::vector<OwnedCell>* cells) { cavity = cells; }
  DonorImport& Bootstrap() { return bootstrap; }
  bool Build(const std::vector<DonorCell>& original, std::string& reason,
             size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    CLocalFailure failure;
    std::map<Id, std::array<Tensor, 4>> pending;
    try {
      const auto temporary = transfer_memory::Add(
          transfer_memory::Mul(original.size(), sizeof(Id) + sizeof(void*) * 5),
          transfer_memory::Mul(current.size(), sizeof(Id) + sizeof(std::array<Tensor, 4>) + 4 * sizeof(void*)),
          transfer_memory::Mul(current.size(), sizeof(DonorCell)));
      if (transfer_memory::Add(caller_bytes, ResidentBytes(), temporary) > ceiling || temporary == SIZE_MAX)
        throw std::invalid_argument("Cavity metric bootstrap exceeds working-storage budget.");
      std::map<Id, const DonorCell*> by_id;
      for (const auto& d : original) by_id.emplace(d.cell.id, &d);
      for (const auto& entry : current) {
        const auto& c = entry.second;
        if (c.sensor_present) { for (const auto& m : c.sensor) ValidateTensor(m); continue; }
        const auto found = by_id.find(entry.first);
        if (found == by_id.end()) throw std::invalid_argument("Missing local original sensor for current cell.");
        std::array<Tensor, 4> sensor;
        for (size_t i = 0; i < 4; ++i) {
          bool matched = false;
          for (size_t j = 0; j < 4; ++j) if (c.cell.v[i].id == found->second->cell.v[j].id) {
            sensor[i] = found->second->sensor[j]; ValidateTensor(sensor[i]); matched = true; break;
          }
          if (!matched) throw std::invalid_argument("Original sensor cell connectivity mismatch.");
        }
        pending.emplace(entry.first, sensor);
      }
    } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
    const auto elected = world.elect(failure); reason = elected.message;
    if (elected.any) return false;
    CLocalFailure snapshot_failure;
    try {
      stage_snapshot.clear(); stage_snapshot.reserve(current.size());
      for (const auto& entry : current) {
        DonorCell donor; donor.cell = entry.second.cell;
        const auto found = pending.find(entry.first);
        donor.sensor = found == pending.end() ? entry.second.sensor : found->second;
        stage_snapshot.push_back(donor);
      }
    } catch (const std::exception& e) { snapshot_failure.Set(2, 0, e.what()); }
    const auto snapshot_elected = world.elect(snapshot_failure); reason = snapshot_elected.message;
    if (snapshot_elected.any) return false;
    const auto retained = transfer_memory::Add(transfer_memory::Bytes(stage_snapshot),
        transfer_memory::Mul(pending.size(), sizeof(Id) + sizeof(std::array<Tensor, 4>) + 4 * sizeof(void*)));
    if (!bootstrap.Build(stage_snapshot, reason, ceiling, transfer_memory::Add(caller_bytes, retained))) return false;
    stats = bootstrap.Statistics();
    for (const auto& p : pending) { current.at(p.first).sensor = p.second; current.at(p.first).sensor_present = true; }
    return true;
  }
  DonorPatch Import(const std::vector<DonorRegion>& regions, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    auto result = bootstrap.Import(regions, ceiling, transfer_memory::Add(caller_bytes, transfer_memory::Bytes(stage_snapshot)));
    ++stats.imports;
    return result;
  }
  DonorPatch ImportCells(const std::vector<Cell>& cells, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    DonorPatch result; CLocalFailure failure; ++stats.imports;
    try {
      Admit(cells.size(), cavity ? cavity->size() : 0, 0, ceiling, caller_bytes);
      if (!cells.empty()) {
        std::map<Id, Tensor> metrics;
        std::vector<DonorCell> donors;
        if (cavity) for (const auto& c : *cavity) Add(c, metrics, donors);
        for (const auto& cell : cells) {
          const auto found = current.find(cell.id);
          if (found != current.end() && std::none_of(donors.begin(), donors.end(),
              [&](const DonorCell& donor) { return donor.cell.id == cell.id; })) Add(found->second, metrics, donors);
        }
        if (donors.empty()) throw std::invalid_argument("Missing current metric cavity.");
        std::vector<Node> nodes;
        for (const auto& cell : cells) for (const auto& node : cell.v) nodes.push_back(node);
        metrics = TransportNodalMetrics(donors, nodes, metrics);
        result.cells = MetricDonors(cells, metrics);
      }
    } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
    const auto elected = world.elect(failure); result.reason = elected.message; result.valid = !elected.any;
    if (!result.valid) { ++stats.rejected; result.cells.clear(); }
    else { stats.payloads += result.cells.size(); stats.maximum_patch = std::max(stats.maximum_patch, result.cells.size()); }
    return result;
  }
  DonorPatch ImportMetricCells(const std::vector<Cell>& cells, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    return ImportCells(cells, ceiling, caller_bytes);
  }
  DonorPatch ImportMetricSamples(const std::vector<Cell>& cells, const std::vector<Id>& = {},
                                 size_t ceiling = SIZE_MAX, size_t caller_bytes = 0,
                                 const std::vector<unsigned char>& = {}, bool = true) {
    return ImportCells(cells, ceiling, caller_bytes);
  }
  DonorSamples SamplePoints(const std::vector<Point>& points, const std::vector<Id>& = {},
                            size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    DonorSamples result; CLocalFailure failure; ++stats.point_batches; stats.point_queries += points.size();
    try {
      Admit(0, cavity ? cavity->size() : 0, points.size(), ceiling, caller_bytes);
      if (!points.empty()) {
        if (!cavity || cavity->empty()) throw std::invalid_argument("Missing current metric cavity.");
        std::map<Id, Tensor> metrics; std::vector<DonorCell> donors;
        for (const auto& c : *cavity) Add(c, metrics, donors);
        for (const auto& p : points) {
          result.samples.push_back(EvaluateCavitySample(donors, p));
        }
      }
    } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
    const auto elected = world.elect(failure); result.reason = elected.message; result.valid = !elected.any;
    if (!result.valid) { ++stats.rejected; result.samples.clear(); }
    else stats.point_payloads += result.samples.size();
    return result;
  }
  DonorSamples SampleMidpoints(const std::vector<std::array<Point, 2>>& endpoints, const std::vector<Id>& extensions = {},
                               size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    std::vector<Point> points; CLocalFailure failure;
    const auto point_bytes = transfer_memory::Mul(endpoints.size(), sizeof(Point));
    try {
      Admit(0, cavity ? cavity->size() : 0, endpoints.size(), ceiling,
            transfer_memory::Add(caller_bytes, point_bytes));
      points.reserve(endpoints.size());
      // These are represented midpoints, unlike the exact midpoint original-source query.
      for (const auto& e : endpoints) points.push_back({double(((long double)e[0].x+e[1].x)/2),
        double(((long double)e[0].y+e[1].y)/2), double(((long double)e[0].z+e[1].z)/2)});
    } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
    const auto elected = world.elect(failure);
    if (elected.any) { ++stats.rejected; DonorSamples result; result.reason = elected.message; return result; }
    return SamplePoints(points, extensions, ceiling, transfer_memory::Add(caller_bytes, point_bytes));
  }
  size_t ResidentBytes() const { return transfer_memory::Add(bootstrap.ResidentBytes(), transfer_memory::Bytes(stage_snapshot)); }
  const DonorImportStats& Statistics() const { return stats; }
 private:
  void Admit(size_t query_cells, size_t cavity_cells, size_t points, size_t ceiling, size_t caller_bytes) {
    const auto count = transfer_memory::Add(query_cells, cavity_cells);
    // Include transport's donor/cell copies and simultaneous node maps, with map-node overhead.
    const auto per_cell = 4 * (sizeof(DonorCell) + sizeof(Cell)) +
                          32 * (sizeof(Id) + sizeof(Tensor) + 4 * sizeof(void*)) + 4 * sizeof(Node);
    const auto bytes = transfer_memory::Add(caller_bytes, ResidentBytes(),
        transfer_memory::Mul(count, per_cell), transfer_memory::Mul(points, sizeof(DonorSample)));
    stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, bytes);
    if (bytes == SIZE_MAX || bytes > ceiling) throw std::invalid_argument("Cavity metric transport exceeds working-storage budget.");
  }
  static void Add(const OwnedCell& cell, std::map<Id, Tensor>& metrics, std::vector<DonorCell>& donors) {
    if (!cell.sensor_present) throw std::invalid_argument("Missing current nodal sensor metrics.");
    DonorCell donor; donor.cell = cell.cell; donor.sensor = cell.sensor;
    for (size_t i = 0; i < 4; ++i) {
      ValidateTensor(cell.sensor[i]);
      const auto added = metrics.emplace(cell.cell.v[i].id, cell.sensor[i]);
      if (!added.second && !SameMetric(added.first->second, cell.sensor[i]))
        throw std::invalid_argument("Inconsistent shared nodal sensor metrics.");
    }
    donors.push_back(donor);
  }
  World& world; Owned& current; DonorImport bootstrap; DonorImportStats stats;
  std::vector<DonorCell> stage_snapshot;
  const std::vector<OwnedCell>* cavity = nullptr;
};
}  // namespace SU2Native3D
