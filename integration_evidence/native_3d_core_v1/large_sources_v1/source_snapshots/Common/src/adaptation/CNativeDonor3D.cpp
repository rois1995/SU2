/*!
 * \file CNativeDonor3D.cpp
 * \brief Indexed original sensor imports without composed-tensor transport or global candidate scans.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeDonor3D.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <chrono>
#include <cmath>
#include <stdexcept>
namespace SU2Native3D {
namespace {
class Timer {
 public:
  Timer(double& value, bool enabled) : value(value), enabled(enabled) {
    if (enabled) start = std::chrono::steady_clock::now();
  }
  ~Timer() { if (enabled) value += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); }
 private:
  double& value;
  bool enabled;
  std::chrono::steady_clock::time_point start;
};
std::array<double, 6> Components(Tensor m) { return {m.xx, m.xy, m.xz, m.yy, m.yz, m.zz}; }
struct Authority {
  unsigned kind = 0;  // 0 shared node, 1 cell ID, 2 canonical simplex.
  std::array<Id, 4> key{};
  Point point;
  Tensor sensor;
  template <class S> void Fields(S& s) { s(kind, key, point, sensor); }
};
int Server(const Authority& row, int size) {
  uint64_t hash = row.kind;
  for (auto id : row.key) hash ^= id + uint64_t(0x9e3779b97f4a7c15ULL) + (hash << 6) + (hash >> 2);
  return hash % size;
}
struct Request {
  DonorRegion region{};
  int caller = 0;
  template <class S> void Fields(S& s) { s(region, caller); }
};
DonorRegion Bounds(const Cell& cell) {
  const auto p = cell.v[0].p;
  DonorRegion result{p.x, p.y, p.z, p.x, p.y, p.z};
  for (const auto& n : cell.v) {
    const double v[3]{n.p.x, n.p.y, n.p.z};
    for (size_t a = 0; a < 3; ++a) {
      result[a] = std::min(result[a], v[a]); result[a + 3] = std::max(result[a + 3], v[a]);
    }
  }
  return result;
}
bool Intersects(const DonorRegion& region, const double* box) {
  for (size_t a = 0; a < 3; ++a) if (region[a] > box[a + 3] || region[a + 3] < box[a]) return false;
  return true;
}
size_t MapBytes(size_t count, size_t value) { return transfer_memory::Mul(count, value + 4 * sizeof(void*)); }
}  // namespace
DonorImport::DonorImport(World& group, bool enabled) : world(group), timings(enabled) {}
DonorImport::~DonorImport() = default;
bool DonorImport::Vote(const CLocalFailure& failure, std::string& reason) {
  const auto elected = world.elect(failure);
  reason = elected.message;
  return !elected.any;
}
bool DonorImport::Admit(size_t bytes, size_t ceiling, std::string& reason) {
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, bytes);
  CLocalFailure failure;
  if (bytes == SIZE_MAX || bytes > ceiling) failure.Set(3, 0, "Native 3D donor import exceeds working-storage budget.");
  return Vote(failure, reason);
}
size_t DonorImport::ResidentBytes() const {
  return transfer_memory::Add(transfer_memory::Bytes(donors), transfer_memory::Bytes(boxes),
      index ? transfer_memory::Add(index->GetAllocatedBytes(), index->RetainedWorkspaceBound(1)) : 0,
      ranks ? ranks->GetMemory() : 0);
}
bool DonorImport::Build(const std::vector<DonorCell>& original, std::string& reason,
                        size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.build_seconds, timings);
  ++stats.builds;
  CLocalFailure failure;
  if (initialized) failure.Set(1, 0, "Native 3D donor source is already frozen.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  const auto n = original.size();
  // Guard ordinary arithmetic inside the house predictor before calling it.
  const auto envelope = transfer_memory::Mul(transfer_memory::Mul(4096, transfer_memory::Add(n, 1)),
      size_t(omp_get_max_threads()));
  if (!Admit(transfer_memory::Add(caller_bytes, transfer_memory::Bytes(original), envelope), ceiling, reason)) {
    ++stats.rejected; return false;
  }
  size_t retained = 0, peak = 0;
  if (n) CADTElemClass::PredictBytes(3, 2 * n, n, 2 * n, &retained, &peak);
  const auto baseline = transfer_memory::Add(caller_bytes, transfer_memory::Bytes(original));
  const auto initial_peak = transfer_memory::Add(baseline, transfer_memory::Mul(n, sizeof(DonorCell)),
      transfer_memory::Mul(6 * n, sizeof(double)), transfer_memory::Mul(n, 256), peak,
      transfer_memory::Mul(world.size, 32768), transfer_memory::GrowthBound(6 * n, sizeof(Authority)));
  if (!Admit(initial_peak, ceiling, reason)) { ++stats.rejected; return false; }
  std::vector<DonorCell> staged;
  std::vector<double> staged_boxes;
  std::vector<std::vector<Authority>> to;
  std::unique_ptr<CADTElemClass> staged_index;
  std::unique_ptr<CRankBoxTree> staged_ranks;
  std::vector<double> owner_boxes;
  try {
    staged = original;
    std::sort(staged.begin(), staged.end(), [](const auto& a, const auto& b) { return a.Key() < b.Key(); });
    to.resize(world.size);
    staged_boxes.reserve(6 * n);
    std::vector<double> centroids;
    centroids.reserve(3 * n);
    for (const auto& donor : staged) {
      const auto key = donor.Key();
      if (std::adjacent_find(key.begin(), key.end()) != key.end()) throw std::invalid_argument("Repeated donor node.");
      const auto& v = donor.cell.v;
      if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0)) throw std::invalid_argument("Nonpositive donor cell.");
      for (size_t i = 0; i < 4; ++i) {
        ValidateTensor(donor.sensor[i]);
        Authority row{0, {v[i].id, 0, 0, 0}, v[i].p, donor.sensor[i]};
        to[Server(row, world.size)].push_back(row);
      }
      for (unsigned kind : {1, 2}) {
        Authority row{kind, kind == 1 ? std::array<Id, 4>{donor.cell.id, 0, 0, 0} : key, {}, {}};
        to[Server(row, world.size)].push_back(row);
      }
      const auto box = Bounds(donor.cell);
      staged_boxes.insert(staged_boxes.end(), box.begin(), box.end());
      for (size_t a = 0; a < 3; ++a) centroids.push_back(.5 * box[a] + .5 * box[a + 3]);
    }
    owner_boxes = BisectionBoxes(3, staged_boxes, centroids, 8, nullptr);
    staged_ranks = std::make_unique<CRankBoxTree>();
    if (n) {
      std::vector<su2double> coordinates(staged_boxes.begin(), staged_boxes.end());
      std::vector<unsigned long> connectivity(2 * n), ids(n);
      for (size_t i = 0; i < connectivity.size(); ++i) connectivity[i] = i;
      for (size_t i = 0; i < n; ++i) ids[i] = i;
      std::vector<unsigned short> types(n, LINE), markers(n, 0);
      staged_index = std::make_unique<CADTElemClass>(3, coordinates, connectivity, types, markers, ids, false);
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  bool admitted = false;
  const auto records = world.exchange(to, &admitted, ceiling, initial_peak);
  if (!admitted) failure.Set(3, 0, "Original donor authority transport exceeds budget.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  const auto check_peak = transfer_memory::Add(initial_peak, transfer_memory::Bytes(to), transfer_memory::Bytes(records),
      MapBytes(records.size(), sizeof(std::pair<const std::pair<unsigned, std::array<Id, 4>>, Authority>)));
  if (!Admit(check_peak, ceiling, reason)) { ++stats.rejected; return false; }
  try {
    std::map<std::pair<unsigned, std::array<Id, 4>>, Authority> authority;
    for (const auto& row : records) {
      const auto inserted = authority.emplace(std::make_pair(row.kind, row.key), row);
      if (inserted.second) continue;
      const auto& old = inserted.first->second;
      if (row.kind != 0) throw std::invalid_argument("Duplicate original donor cell ID or simplex across workers.");
      if (old.point.x != row.point.x || old.point.y != row.point.y || old.point.z != row.point.z ||
          Components(old.sensor) != Components(row.sensor))
        throw std::invalid_argument("Inconsistent original donor sensor/coordinate across workers.");
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  if (!world.maximum(n != 0)) failure.Set(1, 0, "Empty original donor source.");
  if (!Vote(failure, reason)) { ++stats.rejected; return false; }
  staged_ranks->Build(3, owner_boxes, world.comm);
  const auto resident = transfer_memory::Add(baseline, transfer_memory::Bytes(staged), transfer_memory::Bytes(staged_boxes),
      staged_index ? transfer_memory::Add(staged_index->GetAllocatedBytes(), staged_index->RetainedWorkspaceBound(1)) : 0,
      staged_ranks->GetMemory());
  if (!Admit(transfer_memory::Add(check_peak, resident), ceiling, reason)) { ++stats.rejected; return false; }
  donors.swap(staged); boxes.swap(staged_boxes); index.swap(staged_index); ranks.swap(staged_ranks);
  initialized = true;
  return true;
}
DonorPatch DonorImport::Import(const std::vector<DonorRegion>& regions, size_t ceiling, size_t caller_bytes) {
  Timer timer(stats.import_seconds, timings);
  ++stats.imports;
  DonorPatch result;
  CLocalFailure failure;
  if (!initialized) failure.Set(1, 0, "Original donor source has not been frozen.");
  if (regions.size() > MaximumCavityCells) failure.Set(1, 0, "Too many original donor import regions.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  const auto baseline = transfer_memory::Add(caller_bytes, ResidentBytes(), transfer_memory::Bytes(regions));
  const auto route_peak = transfer_memory::Add(baseline, ranks->QueryBytes(),
      transfer_memory::Mul(world.size, sizeof(std::vector<Request>)),
      transfer_memory::GrowthBound(transfer_memory::Mul(regions.size(), world.size), sizeof(Request)));
  if (!Admit(route_peak, ceiling, result.reason)) { ++stats.rejected; return result; }
  std::vector<std::vector<Request>> to;
  try {
    Timer routing(stats.routing_seconds, timings);
    to.resize(world.size);
    std::vector<int> owners;
    for (const auto& region : regions) {
      for (size_t a = 0; a < 3; ++a)
        if (!(std::isfinite(region[a]) && std::isfinite(region[a + 3]) && region[a] <= region[a + 3]))
          throw std::invalid_argument("Invalid original donor import region.");
      ranks->RanksIntersecting(region.data(), region.data() + 3, owners);
      for (auto owner : owners) to[owner].push_back({region, world.rank});
    }
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  bool admitted = false;
  std::vector<Request> requests;
  {
    Timer exchange(stats.exchange_seconds, timings);
    requests = world.exchange(to, &admitted, ceiling, route_peak);
  }
  if (!admitted) failure.Set(3, 0, "Original donor request transport exceeds budget.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  const auto search_peak = transfer_memory::Add(route_peak, transfer_memory::Bytes(to), transfer_memory::Bytes(requests),
      transfer_memory::IntersectionQueryBound(donors.size()), transfer_memory::Mul(world.size, sizeof(std::map<Id, size_t>)),
      MapBytes(transfer_memory::Mul(world.size, MaximumDonors + 1), sizeof(std::pair<const Id, size_t>)),
      transfer_memory::Mul(world.size, sizeof(std::vector<DonorCell>)),
      transfer_memory::GrowthBound(transfer_memory::Mul(world.size, MaximumDonors), sizeof(DonorCell)));
  if (!Admit(search_peak, ceiling, result.reason)) { ++stats.rejected; return result; }
  std::vector<std::vector<DonorCell>> payload;
  try {
    Timer search_time(stats.search_seconds, timings);
    std::vector<std::map<Id, size_t>> selected(world.size);
    std::vector<unsigned long> candidates;
    for (const auto& request : requests) {
      ++stats.owner_requests;
      if (request.caller < 0 || request.caller >= world.size) throw std::invalid_argument("Invalid donor request owner.");
      if (!index) continue;
      const auto& b = request.region;
      const su2double lo[3]{b[0], b[1], b[2]}, hi[3]{b[3], b[4], b[5]};
      index->DetermineIntersectingElements(lo, hi, candidates);
      stats.index_candidates += candidates.size();
      for (auto i : candidates) {
        if (!Intersects(b, boxes.data() + 6 * i)) continue;
        auto& set = selected[request.caller];
        set.emplace(donors.at(i).cell.id, i);
        if (set.size() > MaximumDonors) throw std::runtime_error("Original donor import exceeds patch budget.");
      }
    }
    payload.resize(world.size);
    for (int peer = 0; peer < world.size; ++peer)
      for (auto entry : selected[peer]) payload[peer].push_back(donors[entry.second]);
  } catch (const std::exception& e) { failure.Set(2, 0, e.what()); }
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  std::vector<DonorCell> received;
  {
    Timer exchange(stats.exchange_seconds, timings);
    received = world.exchange(payload, &admitted, ceiling, search_peak);
  }
  if (!admitted) failure.Set(3, 0, "Original donor payload transport exceeds budget.");
  if (received.size() > MaximumDonors) failure.Set(1, 0, "Original donor union exceeds patch budget.");
  if (!regions.empty() && received.empty()) failure.Set(1, 0, "No original donor intersects requested regions.");
  if (!Vote(failure, result.reason)) { ++stats.rejected; return result; }
  std::sort(received.begin(), received.end(), [](const auto& a, const auto& b) { return a.Key() < b.Key(); });
  stats.payloads += received.size();
  stats.maximum_patch = std::max(stats.maximum_patch, received.size());
  result.cells.swap(received);
  result.valid = true;
  return result;
}
}  // namespace SU2Native3D
