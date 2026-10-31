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
    struct Value { Point point; Tensor sensor; bool present = false; };
    CLocalFailure failure;
    std::map<Id, Value> values;
    std::vector<Id> missing;
    const auto temporary = transfer_memory::Add(
        transfer_memory::Mul(original.size(), sizeof(Id) + sizeof(void*) * 5),
        transfer_memory::Mul(current.size(), 4 * (sizeof(Id) + sizeof(Value) + 4 * sizeof(void*) + sizeof(Id))),
        transfer_memory::Mul(current.size(), sizeof(DonorCell)));
    try {
      if (transfer_memory::Add(caller_bytes, ResidentBytes(), temporary) > ceiling || temporary == SIZE_MAX)
        throw std::invalid_argument("Cavity metric bootstrap exceeds working-storage budget.");
      std::map<Id, const DonorCell*> by_id;
      for (const auto& d : original)
        if (!by_id.emplace(d.cell.id, &d).second) throw std::invalid_argument("Duplicate source cell identity.");
      for (const auto& entry : current) {
        const auto& cell = entry.second.cell;
        const auto found = by_id.find(entry.first);
        std::array<int, 4> slots{{-1, -1, -1, -1}};
        if (found != by_id.end()) {
          for (size_t i = 0; i < 4; ++i) for (size_t j = 0; j < 4; ++j) {
            const auto a = cell.v[i], b = found->second->cell.v[j];
            if (a.id == b.id && a.p.x == b.p.x && a.p.y == b.p.y && a.p.z == b.p.z) slots[i] = int(j);
          }
        }
        const bool matched = std::all_of(slots.begin(), slots.end(), [](int slot) { return slot >= 0; });
        for (size_t i = 0; i < 4; ++i) {
          const auto n = cell.v[i];
          if (!std::isfinite(n.p.x) || !std::isfinite(n.p.y) || !std::isfinite(n.p.z))
            throw std::invalid_argument("Nonfinite current metric node.");
          const auto inserted = values.emplace(n.id, Value{n.p, {}, false});
          auto& value = inserted.first->second;
          if (value.point.x != n.p.x || value.point.y != n.p.y || value.point.z != n.p.z)
            throw std::invalid_argument("Inconsistent current metric node coordinates.");
          if (matched) {
            const auto sensor = found->second->sensor[slots[i]]; ValidateTensor(sensor);
            if (value.present && !SameMetric(value.sensor, sensor))
              throw std::invalid_argument("Inconsistent source nodal sensor metrics.");
            value.sensor = sensor; value.present = true;
          }
        }
      }
      for (const auto& value : values) if (!value.second.present) missing.push_back(value.first);
    } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
    auto elected = world.elect(failure); reason = elected.message;
    if (elected.any) return false;
    if (world.maximum(!missing.empty())) {
      DonorImport source(world);
      const auto live = transfer_memory::Add(caller_bytes, ResidentBytes(), temporary);
      if (!source.Build(original, reason, ceiling, live)) return false;
      const auto local_batches = transfer_memory::CeilDiv(missing.size(), MaximumCavityCells);
      const auto batches = CPassiveComm::Allreduce(uint64_t(local_batches), CPassiveComm::Op::MAX, world.comm);
      for (uint64_t batch = 0; batch < batches; ++batch) {
        std::vector<Point> points; CLocalFailure pack_failure;
        const auto begin = std::min(missing.size(), size_t(batch) * MaximumCavityCells);
        const auto end = std::min(missing.size(), begin + MaximumCavityCells);
        try { points.reserve(end-begin); for (size_t i = begin; i < end; ++i) points.push_back(values.at(missing[i]).point); }
        catch (const std::exception& e) { pack_failure.Set(2, 0, e.what()); }
        elected = world.elect(pack_failure); reason = elected.message;
        if (elected.any) return false;
        auto sampled = source.SamplePoints(points, {}, ceiling, transfer_memory::Add(live,
            transfer_memory::Mul(MaximumCavityCells, sizeof(Point) + sizeof(DonorSample))));
        if (!sampled.valid) { reason = sampled.reason; return false; }
        CLocalFailure sample_failure;
        try {
          if (sampled.samples.size() != end-begin) throw std::invalid_argument("Incomplete current metric bootstrap samples.");
          for (size_t i = begin; i < end; ++i) {
            auto& value = values.at(missing[i]); value.sensor = sampled.samples[i-begin].sensor;
            ValidateTensor(value.sensor); value.present = true;
          }
        } catch (const std::exception& e) { sample_failure.Set(2, 0, e.what()); }
        elected = world.elect(sample_failure); reason = elected.message;
        if (elected.any) return false;
      }
    }
    CLocalFailure snapshot_failure;
    try {
      stage_snapshot.clear(); stage_snapshot.reserve(current.size());
      for (const auto& entry : current) {
        DonorCell donor; donor.cell = entry.second.cell;
        for (size_t i = 0; i < 4; ++i) {
          const auto& value = values.at(donor.cell.v[i].id);
          if (!value.present) throw std::invalid_argument("Missing current nodal sensor.");
          donor.sensor[i] = value.sensor;
        }
        stage_snapshot.push_back(donor);
      }
    } catch (const std::exception& e) { snapshot_failure.Set(2, 0, e.what()); }
    elected = world.elect(snapshot_failure); reason = elected.message;
    if (elected.any) return false;
    const auto retained = transfer_memory::Add(transfer_memory::Bytes(stage_snapshot),
        transfer_memory::Mul(values.size(), sizeof(Id) + sizeof(Value) + 4 * sizeof(void*)),
        transfer_memory::Bytes(missing));
    if (!bootstrap.Build(stage_snapshot, reason, ceiling, transfer_memory::Add(caller_bytes, retained))) return false;
    stats = bootstrap.Statistics();
    for (auto& entry : current) {
      for (size_t i = 0; i < 4; ++i) entry.second.sensor[i] = values.at(entry.second.cell.v[i].id).sensor;
      entry.second.sensor_present = true;
    }
    return true;
  }
  DonorPatch Import(const std::vector<DonorRegion>& regions, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    auto result = bootstrap.Import(regions, ceiling, transfer_memory::Add(caller_bytes, transfer_memory::Bytes(stage_snapshot)));
    ++stats.imports;
    return result;
  }
  DonorPatch ImportCells(const std::vector<Cell>& cells, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    ++stats.imports;
    auto result=ImportCellsFrom(cells,cavity,true,ceiling,caller_bytes);
    CLocalFailure failure;if(!result.valid)failure.Set(2,0,result.reason);
    const auto elected=world.elect(failure);result.reason=elected.message;result.valid=!elected.any;
    AccountImport(result);return result;
  }
  /*! Local-only import from the caller's complete current cavity; never falls back to current or bootstrap rows. */
  DonorPatch ImportCellsLocal(const std::vector<Cell>& cells,const std::vector<OwnedCell>& complete_cavity,
                              size_t ceiling=SIZE_MAX,size_t caller_bytes=0) {
    ++stats.imports;auto result=ImportCellsFrom(cells,&complete_cavity,false,ceiling,caller_bytes);AccountImport(result);return result;
  }
  DonorPatch ImportMetricCells(const std::vector<Cell>& cells, size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    return ImportCells(cells, ceiling, caller_bytes);
  }
  DonorPatch ImportMetricSamples(const std::vector<Cell>& cells, const std::vector<Id>& = {},
                                 size_t ceiling = SIZE_MAX, size_t caller_bytes = 0,
                                 const std::vector<unsigned char>& = {}, bool = true) {
    return ImportCells(cells, ceiling, caller_bytes);
  }
  DonorPatch ImportMetricCellsLocal(const std::vector<Cell>& cells,const std::vector<OwnedCell>& complete_cavity,
                                    size_t ceiling=SIZE_MAX,size_t caller_bytes=0) {
    return ImportCellsLocal(cells,complete_cavity,ceiling,caller_bytes);
  }
  DonorPatch ImportMetricSamplesLocal(const std::vector<Cell>& cells,const std::vector<OwnedCell>& complete_cavity,
                                      const std::vector<Id>& = {},size_t ceiling=SIZE_MAX,size_t caller_bytes=0,
                                      const std::vector<unsigned char>& = {},bool = true) {
    return ImportCellsLocal(cells,complete_cavity,ceiling,caller_bytes);
  }
  DonorSamples SamplePoints(const std::vector<Point>& points, const std::vector<Id>& = {},
                            size_t ceiling = SIZE_MAX, size_t caller_bytes = 0) {
    ++stats.point_batches;stats.point_queries+=points.size();
    const std::vector<OwnedCell> empty;auto result=SamplePointsFrom(points,cavity?*cavity:empty,ceiling,caller_bytes);
    CLocalFailure failure;if(!result.valid)failure.Set(2,0,result.reason);
    const auto elected=world.elect(failure);result.reason=elected.message;result.valid=!elected.any;
    AccountSamples(result);return result;
  }
  /*! Local-only samples from an explicitly supplied complete current cavity. */
  DonorSamples SamplePointsLocal(const std::vector<Point>& points,const std::vector<OwnedCell>& complete_cavity,
                                 const std::vector<Id>& = {},size_t ceiling=SIZE_MAX,size_t caller_bytes=0) {
    ++stats.point_batches;stats.point_queries+=points.size();
    auto result=SamplePointsFrom(points,complete_cavity,ceiling,caller_bytes);AccountSamples(result);return result;
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
  DonorSamples SampleMidpointsLocal(const std::vector<std::array<Point,2>>& endpoints,const std::vector<OwnedCell>& complete_cavity,
                                    const std::vector<Id>& extensions={},size_t ceiling=SIZE_MAX,size_t caller_bytes=0) {
    std::vector<Point> points;const auto point_bytes=transfer_memory::Mul(endpoints.size(),sizeof(Point));
    try {
      Admit(0,complete_cavity.size(),endpoints.size(),ceiling,transfer_memory::Add(caller_bytes,point_bytes));
      points.reserve(endpoints.size());
      for(const auto& e:endpoints)points.push_back({double(((long double)e[0].x+e[1].x)/2),
        double(((long double)e[0].y+e[1].y)/2),double(((long double)e[0].z+e[1].z)/2)});
    } catch(const std::exception& e) {
      ++stats.rejected;DonorSamples result;result.reason=e.what();return result;
    }
    return SamplePointsLocal(points,complete_cavity,extensions,ceiling,transfer_memory::Add(caller_bytes,point_bytes));
  }
  /*! Sample one current sensor row only when exact positive-tet barycentrics prove closed containment. */
  bool SampleContainedCurrentMetric(const OwnedCell& row,Point point,Tensor& sensor) const {
    if(!row.sensor_present||!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z))
      throw std::invalid_argument("Missing current metric row or nonfinite sample coordinate.");
    DonorCell donor;donor.cell=row.cell;donor.sensor=row.sensor;
    for(size_t i=0;i<4;++i) {
      const auto p=row.cell.v[i].p;
      if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))
        throw std::invalid_argument("Nonfinite current metric row coordinate.");
      ValidateTensor(row.sensor[i]);
    }
    std::array<long double,4> weights;
    const Tetrahedron tet{row.cell.v[0].p,row.cell.v[1].p,row.cell.v[2].p,row.cell.v[3].p};
    if(!Barycentric(tet,point,weights))return false;
    const Tensor sampled=InterpolateSensor(donor,weights);ValidateTensor(sampled);sensor=sampled;return true;
  }
  size_t ResidentBytes() const { return transfer_memory::Add(bootstrap.ResidentBytes(), transfer_memory::Bytes(stage_snapshot)); }
  const DonorImportStats& Statistics() const { return stats; }
 private:
  DonorPatch ImportCellsFrom(const std::vector<Cell>& cells,const std::vector<OwnedCell>* rows,bool use_current,
                             size_t ceiling,size_t caller_bytes) {
    DonorPatch result;
    try {
      Admit(cells.size(),rows?rows->size():0,0,ceiling,caller_bytes);
      if(!cells.empty()) {
        if((!rows||rows->empty())&&!use_current)throw std::invalid_argument("Missing explicit complete current metric cavity.");
        std::map<Id,Tensor> metrics;std::vector<DonorCell> donors;
        if(rows)for(const auto& row:*rows)Add(row,metrics,donors);
        if(use_current)for(const auto& cell:cells) {
          const auto found=current.find(cell.id);
          if(found!=current.end()&&std::none_of(donors.begin(),donors.end(),[&](const DonorCell& donor){return donor.cell.id==cell.id;}))
            Add(found->second,metrics,donors);
        }
        if(donors.empty())throw std::invalid_argument("Missing current metric cavity.");
        std::vector<Node> nodes;for(const auto& cell:cells)for(const auto& node:cell.v)nodes.push_back(node);
        metrics=TransportNodalMetrics(donors,nodes,metrics);result.cells=MetricDonors(cells,metrics);
      }
      result.valid=true;
    } catch(const std::exception& e){result.reason=e.what();}
    return result;
  }
  DonorSamples SamplePointsFrom(const std::vector<Point>& points,const std::vector<OwnedCell>& rows,
                                size_t ceiling,size_t caller_bytes) {
    DonorSamples result;
    try {
      Admit(0,rows.size(),points.size(),ceiling,caller_bytes);
      if(!points.empty()) {
        if(rows.empty())throw std::invalid_argument("Missing explicit complete current metric cavity.");
        std::map<Id,Tensor> metrics;std::vector<DonorCell> donors;
        for(const auto& row:rows)Add(row,metrics,donors);
        for(const auto& point:points)result.samples.push_back(EvaluateCavitySample(donors,point));
      }
      result.valid=true;
    }catch(const std::exception& e){result.reason=e.what();}
    return result;
  }
  void AccountImport(DonorPatch& result) {
    if(!result.valid){++stats.rejected;result.cells.clear();}
    else {stats.payloads+=result.cells.size();stats.maximum_patch=std::max(stats.maximum_patch,result.cells.size());}
  }
  void AccountSamples(DonorSamples& result) {
    if(!result.valid){++stats.rejected;result.samples.clear();}
    else stats.point_payloads+=result.samples.size();
  }
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
