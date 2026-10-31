/*!
 * \file CNativeDistributed3D_tests.cpp
 * \brief Complete MPI stars, private coupled edits, atomic rejection and subset migration.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeDistributed3D.hpp"
#include "../../../Common/include/adaptation/CNativeDonor3D.hpp"
#include "../../../Common/include/adaptation/CNativeRegionField3D.hpp"
#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "SimplexMeshTestCase.hpp"
#include <cmath>
#include <fstream>
#include <chrono>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
using namespace SU2Native3D;
namespace {
void Log(World& world, const std::string& text) {
  // One writer avoids MPI stdout packet interleaving; this is fixture output, outside engine timings.
  std::cout<<std::flush;
  const std::vector<char> send(text.begin(),text.end());
  const auto rows=CPassiveComm::Allgatherv(send,nullptr,world.comm);
  if (!world.rank && !rows.empty()) std::cout.write(rows.data(),rows.size())<<std::flush;
}
Face Key(Face face) { std::sort(face.begin(), face.end()); return face; }
std::array<Face, 4> Faces(const Cell& c) {
  const auto& v = c.v;
  return {{{v[1].id, v[2].id, v[3].id}, {v[0].id, v[3].id, v[2].id},
           {v[0].id, v[1].id, v[3].id}, {v[0].id, v[2].id, v[1].id}}};
}
struct Fixture { Patch patch; std::vector<Facet> facets; };
Fixture Box(double height=1, bool shear=false) {
  std::array<Node, 8> nodes{{{0, {0, 0, 0}}, {1, {1, 0, 0}}, {2, {1, 1, 0}}, {3, {0, 1, 0}},
                                 {4, {0, 0, height}}, {5, {1, 0, height}}, {6, {1, 1, height}}, {7, {0, 1, height}}}};
  if (shear) for (auto& node : nodes) node.p.x += node.p.y;
  Fixture result;
  const std::array<Id, 6> ring{1, 2, 3, 7, 4, 5};
  for (size_t i = 0; i < ring.size(); ++i) {
    Cell c{Id(i), {nodes[0], nodes[6], nodes[ring[i]], nodes[ring[(i + 1) % 6]]}};
    if (Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) < 0) std::swap(c.v[0], c.v[1]);
    result.patch.cells.push_back(c);
  }
  for (auto face : Incidence(result.patch.cells).Boundary()) {
    const auto id = Id(result.facets.size());
    result.facets.push_back({id, int(id) + 10, {nodes[face[0]], nodes[face[1]], nodes[face[2]]}});
    result.patch.surface.push_back({face, id});
  }
  return result;
}
std::vector<OwnedCell> Rows(const Patch& patch) {
  std::map<Face, Id> physical;
  for (auto face : patch.surface)
    if (!physical.emplace(Key(face.v), face.facet).second) throw std::runtime_error("Duplicate fixture physical face.");
  std::vector<OwnedCell> rows;
  for (auto cell : patch.cells) {
    OwnedCell row;
    row.cell = cell;
    row.version = 17 + cell.id;
    const auto faces = Faces(cell);
    for (size_t i = 0; i < faces.size(); ++i) {
      const auto found = physical.find(Key(faces[i]));
      if (found != physical.end()) row.physical[i] = {true, found->second};
    }
    rows.push_back(row);
  }
  return rows;
}
Owned Partition(const std::vector<OwnedCell>& rows, const World& world) {
  Owned result;
  for (auto row : rows) if (row.cell.id % world.size == unsigned(world.rank)) result.emplace(row.cell.id, row);
  return result;
}
Patch PatchOf(const std::vector<OwnedCell>& rows) {
  Patch patch;
  for (const auto& row : rows) {
    patch.cells.push_back(row.cell);
    const auto faces = Faces(row.cell);
    for (size_t i = 0; i < faces.size(); ++i)
      if (row.physical[i].present) patch.surface.push_back({faces[i], row.physical[i].facet});
  }
  return patch;
}
std::vector<char> Bytes(OwnedCell cell) {
  std::vector<char> result;
  RecordStream stream(result);
  stream(cell);
  return result;
}
std::vector<char> Bytes(const Owned& owned) {
  std::vector<char> result;
  RecordStream stream(result);
  for (auto row : owned) { auto key = row.first; stream(key, row.second); }
  return result;
}
Owned WithUniformSensor(Owned owned, Tensor sensor) {
  for (auto& row : owned) { row.second.sensor_present = true; row.second.sensor.fill(sensor); }
  return owned;
}
std::vector<Feature> Query(const World& world, Feature feature, bool all = false) {
  return world.rank == 0 || all ? std::vector<Feature>{feature} : std::vector<Feature>{};
}
std::vector<OwnedCell> Gather(World& world, const Owned& owned) {
  std::vector<std::vector<OwnedCell>> to(world.size);
  for (const auto& row : owned) to[0].push_back(row.second);
  auto result = world.exchange(to);
  std::sort(result.begin(), result.end(), [](const auto& a, const auto& b) { return a.cell.id < b.cell.id; });
  return result;
}
bool Audit(World& world, const Owned& owned, const Fixture& reference, size_t count) {
  const auto rows = Gather(world, owned);
  bool valid = true;
  if (world.rank == 0) try {
    const auto patch = PatchOf(rows);
    const Incidence incidence(patch.cells);
    if (rows.size() != count || incidence.Boundary().size() != patch.surface.size()) valid = false;
    const PlanarReference geometry(reference.facets);
    std::map<Face, Id> physical;
    std::map<Id, Point> nodes;
    for (const auto& cell : patch.cells) for (auto node : cell.v) nodes.emplace(node.id, node.p);
    for (auto face : patch.surface) {
      if (!physical.emplace(Key(face.v), face.facet).second) valid = false;
      const auto source=std::find_if(reference.facets.begin(),reference.facets.end(),[&](const auto& f){return f.id==face.facet;});
      if (source==reference.facets.end() || geometry.Get(face.facet).marker!=source->marker) valid=false;
      for (auto id : face.v) if (!geometry.Contains(face.facet, nodes.at(id))) valid = false;
    }
    for (auto face : incidence.Boundary()) if (!physical.count(Key(face))) valid = false;
  } catch (const std::exception&) { valid = false; }
  return !world.maximum(!valid);
}
// One coupled, fully imported private refinement and its inverse; not a complete remeshing sweep.
bool Edit(World& world, Directory& directory, Owned& owned, const Fixture& geometry, bool reverse) {
  auto dependencies = directory.Lookup(Query(world, reverse ? Feature{{8, 0, 0}, 1} : Feature{{0, 6, 0}, 2}));
  auto imported = directory.Fetch(dependencies, owned);
  if (!dependencies.valid || !imported.valid) return false;
  std::string reason;
  const bool reserved = directory.Reserve(imported, world.rank == 0, reason);
  bool approved = world.rank != 0 || reserved;
  std::vector<DonorCell> original;
  for (const auto& row : owned) {
    DonorCell donor; donor.cell = row.second.cell;
    donor.sensor.fill(reverse ? Tensor{} : Tensor{4, 0, 0, 4, 0, 4});
    original.push_back(donor);
  }
  DonorImport donor_service(world);
  const bool frozen = donor_service.Build(original, reason);
  std::vector<DonorRegion> regions;
  for (const auto& row : imported.cells) {
    const auto p = row.cell.v[0].p;
    DonorRegion box{p.x, p.y, p.z, p.x, p.y, p.z};
    for (auto node : row.cell.v) {
      const double xyz[3]{node.p.x, node.p.y, node.p.z};
      for (size_t a = 0; a < 3; ++a) { box[a] = std::min(box[a], xyz[a]); box[a + 3] = std::max(box[a + 3], xyz[a]); }
    }
    regions.push_back(box);
  }
  auto sensor_patch = donor_service.Import(regions);
  approved = approved && frozen && sensor_patch.valid;
  Patch fresh;
  if (world.rank == 0 && approved) try {
    const auto old = PatchOf(imported.cells);
    const PlanarReference reference(geometry.facets);
    approved = reverse ? UndoEdgeSplit(old, 8, {0, 6}, 200, reference, fresh, reason) :
                         SplitEdge(old, {0, 6}, {8, {.5, .5, .5}}, 100, reference, fresh, reason);
    FrozenField target(std::move(sensor_patch.cells));
    PatchMetric measured;
    approved = approved && ValidateMetricChange(old.cells, fresh.cells,
        reverse ? MetricChange::Coarsen : MetricChange::Refine, target, measured, reason);
  } catch (const std::exception& e) { approved = false; reason = e.what(); }
  if (!approved) std::cerr << "Private edit rejected rank " << world.rank << ": " << reason << '\n';
  std::vector<std::vector<Dependency>> remove(world.size);
  std::vector<std::vector<OwnedCell>> add(world.size);
  if (world.rank == 0 && approved) {
    for (auto row : dependencies.cells) remove[row.owner].push_back(row);
    for (auto row : Rows(fresh)) add[row.cell.id % world.size].push_back(row);
  }
  const auto removed = world.exchange(remove);
  const auto added = world.exchange(add);
  return directory.Publish(owned, removed, added, approved, reason);
}
void MeshRecords(const std::string& label, World& world, const Owned& owned) {
  const auto rows = Gather(world, owned);
  if (world.rank == 0) for (const auto& row : rows) {
    std::ostringstream line;
    line << std::setprecision(17) << "MESH " << label << ' ' << row.cell.id << ' ' << row.version;
    for (auto n : row.cell.v) line << ' ' << n.id << ' ' << n.p.x << ' ' << n.p.y << ' ' << n.p.z;
    for (auto f : row.physical) line << ' ' << f.present << ' ' << f.facet;
    std::cout << line.str() << '\n';
  }
}
void Report(const char* label, World& world, const Directory& directory) {
  const auto& s = directory.Statistics();
  std::cout << "DIRECTORY " << label << " rank=" << world.rank << " ranks=" << world.size
            << " builds=" << s.builds << " lookups=" << s.lookups << " fetches=" << s.fetches
            << " reservations=" << s.reservations << " publications=" << s.publications << " rejected=" << s.rejected
            << " scanned=" << s.metadata_scanned << " payloads=" << s.payloads << " elections=" << s.elections
            << " max_dependencies=" << s.maximum_dependencies << " resident_bytes=" << directory.ResidentBytes()
            << " max_directory_work_bytes=" << s.maximum_work_bytes << " max_exchange_work_bytes=" << world.max_exchange_work_bytes
            << " bytes_sent=" << world.bytes_sent << " collective_calls=" << world.collective_calls
            << " build_s=" << s.build_seconds << " lookup_s=" << s.lookup_seconds << " fetch_s=" << s.fetch_seconds
            << " reserve_s=" << s.reservation_seconds << " publish_s=" << s.publication_seconds
            << " election_nested_s=" << s.election_seconds << " world_transport_partial_s=" << world.collective_seconds << '\n';
}
}  // namespace
TEST_CASE("Native tetrahedral MPI scalar records preserve physical bindings and high IDs", "[NativeDistributed3D]") {
  auto row = Rows(Box().patch).front();
  row.cell.id = UINT64_MAX;
  row.version = UINT64_MAX - 1;
  row.physical[0] = {true, 0};
  row.sensor_present = true;
  row.sensor[2] = {2, .1, .2, 3, .3, 4};
  const auto bytes = Bytes(row);
  RecordStream measured;
  measured(row);
  CHECK(bytes.size() == measured.Size());
  OwnedCell restored;
  RecordStream read(bytes);
  read(restored);
  CHECK(read.End());
  CHECK(Bytes(restored) == bytes);
  CHECK(restored.physical[0].present);
  CHECK(restored.physical[0].facet == 0);
  CHECK(restored.sensor_present);
  CHECK(restored.sensor[2].xz == .2);
  CHECK(restored.sensor[2].zz == 4);
  auto legacy = bytes;
  legacy[0] ^= 1;
  RecordStream legacy_read(static_cast<const std::vector<char>&>(legacy));
  CHECK_THROWS_AS(legacy_read(restored), std::invalid_argument);
  auto truncated = bytes;
  truncated.pop_back();
  RecordStream short_read(static_cast<const std::vector<char>&>(truncated));
  CHECK_THROWS_AS(short_read(restored), std::runtime_error);
  World world;
  const auto before = CPassiveComm::GetRoundBytes();
  CPassiveComm::SetRoundBytes(127);
  std::vector<std::vector<OwnedCell>> to(world.size);
  to[(world.rank + 1) % world.size].push_back(row);
  const auto imported = world.exchange(to);
  CPassiveComm::SetRoundBytes(before);
  CHECK(imported.size() == 1);
  if (imported.size() == 1) CHECK(Bytes(imported.front()) == bytes);
}
TEST_CASE("Native tetrahedral MPI admits complete vertex edge face stars and fresh authoritative payloads", "[NativeDistributed3D]") {
  World world;
  Directory directory(world);
  const auto owned = Partition(Rows(Box().patch), world);
  std::string reason;
  REQUIRE(directory.Build(owned, reason));
  for (auto feature : {Feature{{0, 0, 0}, 1}, Feature{{6, 0, 0}, 2}, Feature{{6, 1, 0}, 3}}) {
    const auto dependencies = directory.Lookup(Query(world, feature));
    CHECK(dependencies.valid);
    CHECK(dependencies.cells.size() == (world.rank ? 0 : feature.count == 3 ? 2 : 6));
    const auto imported = directory.Fetch(dependencies, owned);
    CHECK(imported.valid);
    CHECK(imported.cells.size() == dependencies.cells.size());
    for (const auto& row : imported.cells) CHECK(row.version == 17 + row.cell.id);
  }
  const auto saved = directory.Snapshot();
  CHECK_FALSE(directory.Build({}, reason));
  CHECK(directory.Snapshot() == saved);
  auto missing = directory.Lookup(Query(world, {{UINT64_MAX, 0, 0}, 1}));
  CHECK_FALSE(missing.valid);
  CHECK(missing.cells.empty());
  CHECK(directory.Snapshot() == saved);
  auto repeated = directory.Lookup(Query(world, {{0, 0, 0}, 2}));
  CHECK_FALSE(repeated.valid);
  CHECK(repeated.cells.empty());
  auto oversized = directory.Lookup(world.rank ? std::vector<Feature>{} : std::vector<Feature>(MaximumDirectoryFeatures + 1, {{0, 0, 0}, 1}));
  CHECK_FALSE(oversized.valid);
  CHECK(oversized.cells.empty());
  CHECK(directory.Snapshot() == saved);
  Report("discovery", world, directory);
}
TEST_CASE("Native tetrahedral MPI claims elect one nonconflicting proposal", "[NativeDistributed3D]") {
  World world;
  Directory directory(world);
  auto owned = Partition(Rows(Box().patch), world);
  std::string reason;
  REQUIRE(directory.Build(owned, reason));
  const auto saved = directory.Snapshot();
  const auto mesh = Bytes(owned);
  const auto dependencies = directory.Lookup(Query(world, {{0, 6, 0}, 2}, true));
  const auto imported = directory.Fetch(dependencies, owned);
  const bool chosen = directory.Reserve(imported, true, reason);
  CHECK(chosen == (world.rank == 0));
  CHECK(world.sum(chosen) == 1);
  const bool prioritized=directory.Reserve(imported,true,reason,SIZE_MAX,0,unsigned(world.size-1-world.rank));
  CHECK(prioritized==(world.rank==world.size-1));CHECK(world.sum(prioritized)==1);
  CHECK(directory.Snapshot() == saved);
  CHECK(Bytes(owned) == mesh);
  Report("conflict", world, directory);
}
TEST_CASE("Native tetrahedral MPI split and coarsening publish mesh and directory together", "[NativeDistributed3D]") {
  World world;
  Directory directory(world);
  const auto fixture = Box();
  auto owned = Partition(Rows(fixture.patch), world);
  std::string reason;
  REQUIRE(directory.Build(owned, reason));
  MeshRecords("coupled_initial", world, owned);
  CHECK(Edit(world, directory, owned, fixture, false));
  CHECK(Audit(world, owned, fixture, 12));
  MeshRecords("coupled_refined", world, owned);
  const auto midpoint = directory.Lookup(Query(world, {{8, 0, 0}, 1}));
  CHECK(midpoint.valid);
  CHECK(midpoint.cells.size() == (world.rank ? 0 : 12));
  CHECK(Edit(world, directory, owned, fixture, true));
  CHECK(Audit(world, owned, fixture, 6));
  MeshRecords("coupled_coarsened", world, owned);
  CHECK_FALSE(directory.Lookup(Query(world, {{8, 0, 0}, 1})).valid);
  CHECK(directory.Statistics().publications == 2);
  Report("coupled", world, directory);
}
TEST_CASE("Native tetrahedral MPI rejected versions rank veto budgets and shared coordinates preserve both stores", "[NativeDistributed3D]") {
  World world;
  Directory directory(world);
  auto owned = Partition(Rows(Box().patch), world);
  std::string reason;
  REQUIRE(directory.Build(owned, reason));
  const auto saved = directory.Snapshot();
  const auto mesh = Bytes(owned);
  const auto deps = directory.Lookup(Query(world, {{0, 6, 0}, 2}));
  auto stale = deps;
  for (auto& d : stale.cells) ++d.version;
  const auto imported_stale = directory.Fetch(stale, owned);
  CHECK_FALSE(imported_stale.valid);
  CHECK(imported_stale.cells.empty());
  CHECK_FALSE(directory.Publish(owned, {}, {}, world.rank != 0, reason));
  CHECK_FALSE(directory.Publish(owned, {}, {}, true, reason, world.rank ? SIZE_MAX : 1));
  CHECK_FALSE(directory.Lookup(Query(world, {{0, 0, 0}, 1}), world.rank ? SIZE_MAX : 1).valid);
  CHECK_FALSE(directory.Lookup(Query(world, {{0, 0, 0}, 1}), SIZE_MAX, world.rank ? 0 : SIZE_MAX).valid);
  std::vector<Dependency> removed;
  std::vector<OwnedCell> added;
  if (owned.count(0)) {
    const auto original = owned.at(0);
    removed.push_back({0, original.version, world.rank});
    added.push_back(original);
  }
  CHECK_FALSE(directory.Publish(owned, removed, added, true, reason));
  for (auto& row : added) { ++row.version; for (auto& node : row.cell.v) if (node.id == 0) node.p.x = .125; }
  CHECK_FALSE(directory.Publish(owned, removed, added, true, reason));
  for (auto& row : added) std::swap(row.cell.v[0], row.cell.v[1]);
  CHECK_FALSE(directory.Publish(owned, removed, added, true, reason));
  CHECK(directory.Snapshot() == saved);
  CHECK(Bytes(owned) == mesh);
  const auto fresh = directory.Fetch(deps, owned);
  CHECK(fresh.valid);
  CHECK_FALSE(directory.Fetch(deps, owned, world.rank ? SIZE_MAX : 1).valid);
  CHECK_FALSE(directory.Reserve(fresh, world.rank == 0, reason, world.rank ? SIZE_MAX : 1));
  CHECK(directory.Snapshot() == saved);
  CHECK(Bytes(owned) == mesh);
  Report("rollback", world, directory);
}
TEST_CASE("Native tetrahedral MPI rejects duplicate global identities even with disjoint node IDs", "[NativeDistributed3D]") {
  World world;
  Directory directory(world);
  auto owned = Partition(Rows(Box().patch), world);
  std::string reason;
  REQUIRE(directory.Build(owned, reason));
  const auto saved = directory.Snapshot();
  auto extra = Rows(Box().patch).front();
  for (auto& node : extra.cell.v) { node.id += 100; node.p.x += 10; }
  extra.physical = {};
  CHECK_FALSE(directory.Publish(owned, {}, world.rank == world.size - 1 ? std::vector<OwnedCell>{extra} : std::vector<OwnedCell>{}, true, reason));
  CHECK(directory.Snapshot() == saved);
}
TEST_CASE("Native tetrahedral MPI never admits a partial over-budget star", "[NativeDistributed3D]") {
  World world;
  Directory directory(world);
  Owned owned;
  const size_t count = MaximumCavityCells + 1;
  for (size_t i = 0; i < count; ++i) {
    const double a = 2 * std::acos(-1.) * i / count, b = 2 * std::acos(-1.) * ((i + 1) % count) / count;
    OwnedCell row;
    row.cell = {Id(i), {{{0, {0, 0, 0}}, {1, {0, 0, 1}},
                         {Id(i + 2), {std::cos(a), std::sin(a), .5}},
                         {Id((i + 1) % count + 2), {std::cos(b), std::sin(b), .5}}}}};
    if (Orientation(row.cell.v[0].p, row.cell.v[1].p, row.cell.v[2].p, row.cell.v[3].p) < 0)
      std::swap(row.cell.v[0], row.cell.v[1]);
    if (i % world.size == unsigned(world.rank)) owned.emplace(row.cell.id, row);
  }
  std::string reason;
  REQUIRE(directory.Build(owned, reason));
  const auto saved = directory.Snapshot();
  const auto deps = directory.Lookup(Query(world, {{0, 1, 0}, 2}));
  CHECK_FALSE(deps.valid);
  CHECK(deps.cells.empty());
  CHECK(directory.Snapshot() == saved);
  CHECK(reason.empty());
  Report("overbudget", world, directory);
}
#ifdef HAVE_MPI
TEST_CASE("Native tetrahedral MPI migrates to M workers and returns accepted records to N ranks", "[NativeDistributed3D]") {
  World parent;
  const auto fixture = Box();
  const std::set<int> workers{parent.size, std::max(1, parent.size - 1), std::max(1, parent.size / 2)};
  for (int m : workers) {
    auto owned = Partition(Rows(fixture.patch), parent);
    std::vector<std::vector<OwnedCell>> to(parent.size);
    for (auto row : owned) to[row.first % m].push_back(row.second);
    const auto imported = parent.exchange(to);
    MPI_Comm subset = MPI_COMM_NULL;
    MPI_Comm_split(parent.comm, parent.rank < m ? 0 : MPI_UNDEFINED, parent.rank, &subset);
    Owned remeshed;
    bool valid = true;
    if (subset != MPI_COMM_NULL) {
      World world(subset);
      for (auto row : imported) remeshed.emplace(row.cell.id, row);
      Directory directory(world);
      std::string reason;
      valid = directory.Build(remeshed, reason);
      if (valid) valid = Edit(world, directory, remeshed, fixture, false);
      if (valid) valid = Audit(world, remeshed, fixture, 12);
      Report("subset", world, directory);
      MPI_Comm_free(&subset);
    } else CHECK(imported.empty());
    CHECK_FALSE(parent.maximum(!valid));
    std::vector<std::vector<OwnedCell>> back(parent.size);
    for (auto row : remeshed) back[row.first % parent.size].push_back(row.second);
    const auto accepted = parent.exchange(back);
    Owned returned;
    for (auto row : accepted) CHECK(returned.emplace(row.cell.id, row).second);
    CHECK(Audit(parent, returned, fixture, 12));
    MeshRecords("returned_m" + std::to_string(m), parent, returned);
    std::cout << "SUBSET_RETURN N=" << parent.size << " M=" << m << " rank=" << parent.rank
              << " local_cells=" << returned.size() << " valid=" << valid << '\n';
  }
}
#endif

#ifdef HAVE_MPI
TEST_CASE("Native worker rank boxes and passive gathers stay in their explicit communicator", "[NativeDistributed3D]") {
  World parent;
  const auto saved = CPassiveComm::GetRoundBytes();
  CPassiveComm::SetRoundBytes(127);
  const std::set<int> sizes{parent.size, std::max(1, parent.size - 1), std::max(1, parent.size / 2)};
  for (int m : sizes) {
    MPI_Comm subset = MPI_COMM_NULL;
    MPI_Comm_split(parent.comm, parent.rank < m ? 0 : MPI_UNDEFINED, parent.rank, &subset);
    bool valid = true;
    if (subset != MPI_COMM_NULL) {
      World worker(subset);
      std::vector<uint64_t> local(worker.rank, uint64_t(worker.rank + 10));
      std::vector<size_t> counts;
      const auto imported = CPassiveComm::Allgatherv(local, &counts, worker.comm);
      size_t cursor = 0;
      valid = counts.size() == unsigned(m);
      for (int r = 0; r < m && valid; ++r) {
        valid = counts[r] == unsigned(r);
        for (int i = 0; i < r && valid; ++i) valid = cursor < imported.size() && imported[cursor++] == unsigned(r + 10);
      }
      valid = valid && cursor == imported.size();
      for (unsigned short dimension : {2, 3}) {
        std::vector<double> box;
        // Empty partitions and empty rank-zero sends still participate in bounded rounds.
        if (m < 3 || worker.rank != 1) {
          for (unsigned short axis = 0; axis < dimension; ++axis) box.push_back(axis ? 0. : double(worker.rank));
          for (unsigned short axis = 0; axis < dimension; ++axis) box.push_back(axis ? .5 : worker.rank + .5);
        }
        CRankBoxTree tree;
        tree.Build(dimension, box, worker.comm);
        valid = valid && tree.GetnBox() == unsigned(m - (m >= 3));
        for (int r = 0; r < m; ++r) {
          const double point[3]{double(r), 0., 0.};
          std::vector<int> ranks;
          tree.RanksContaining(point, ranks);
          valid = valid && ranks == (m >= 3 && r == 1 ? std::vector<int>{} : std::vector<int>{r});
        }
      }
      MPI_Comm_free(&subset);
    }
    CHECK_FALSE(parent.maximum(!valid));
  }
  // Existing callers omit comm; verify their full CFD communicator behavior is preserved.
  std::vector<double> box{double(parent.rank), 0., 0., parent.rank + .5, .5, .5};
  CRankBoxTree default_tree;
  default_tree.Build(3, box);
  CHECK(default_tree.GetnBox() == unsigned(parent.size));
  const double point[3]{double(parent.size - 1), 0., 0.};
  std::vector<int> ranks;
  default_tree.RanksContaining(point, ranks);
  CHECK(ranks == std::vector<int>{parent.size - 1});
  CPassiveComm::SetRoundBytes(saved);
}
#endif

#ifdef HAVE_MPI
namespace {
Tensor Sensor(Point p) { return {2 + p.x, .2, .1, 3 + p.y, .15, 4 + p.z}; }
std::array<double, 6> TensorValues(Tensor m) { return {m.xx, m.xy, m.xz, m.yy, m.yz, m.zz}; }
std::vector<DonorCell> Source(const std::vector<Cell>& cells, const World& world, bool all = false) {
  std::vector<DonorCell> result;
  for (const auto& cell : cells) if (all || cell.id % world.size == unsigned(world.rank)) {
    DonorCell donor; donor.cell = cell;
    for (size_t i = 0; i < 4; ++i) donor.sensor[i] = Sensor(cell.v[i].p);
    result.push_back(donor);
  }
  return result;
}
}
TEST_CASE("Native MPI donor imports preserve original P1 sensors and actual-query composition", "[NativeDistributed3D]") {
  World world;
  const auto geometry = Box();
  auto original = Source(geometry.patch.cells, world);
  DonorImport service(world, true);
  std::string reason;
  REQUIRE(service.Build(original, reason));
  const auto resident = service.ResidentBytes();
  CHECK_FALSE(service.Build(original, reason));
  CHECK(service.ResidentBytes() == resident);
  // Changing the caller's working copy cannot modify the accepted immutable source.
  for (auto& donor : original) for (auto& sensor : donor.sensor) sensor.xx += 100;
  const std::vector<DonorRegion> regions{{0, 0, 0, 1, 1, 1}, {0, 0, 0, 1, 1, 1}};
  auto patch = service.Import(regions);
  REQUIRE(patch.valid);
  REQUIRE(patch.cells.size() == 6);
  for (const auto& donor : patch.cells) {
    std::ostringstream line;
    line << std::setprecision(17) << "DONOR_SOURCE " << world.rank << ' ' << donor.cell.id;
    for (size_t i = 0; i < 4; ++i) {
      const auto node = donor.cell.v[i];
      line << ' ' << node.id << ' ' << node.p.x << ' ' << node.p.y << ' ' << node.p.z;
      for (auto value : TensorValues(donor.sensor[i])) line << ' ' << value;
    }
    std::cout << line.str() << '\n';
  }
  FrozenField oracle(Source(geometry.patch.cells, world, true));
  uint64_t composed = 0;
  FrozenField imported(patch.cells, [&](Point p, Tensor sensor) {
    ++composed;
    // Manufactured thin geometric region: no fine wall tensor is placed in the donor record.
    if (p.z < .0625) sensor.zz += 100 * (1 - p.z / .0625);
    return sensor;
  }, true);
  double max_residual = 0;
  for (Point p : {Point{.125, .25, .375}, Point{.5, .5, .5}, Point{0, 0, 0}, Point{.25, .375, .03125}}) {
    const auto expected = TensorValues(Sensor(p));
    const auto local = oracle.Query(p);
    for (int repeat = 0; repeat < 2; ++repeat) {
      const auto sample = imported.Query(p);
      CHECK(sample.donor == local.donor);
      const auto values = TensorValues(sample.sensor);
      for (size_t i = 0; i < 6; ++i) max_residual = std::max(max_residual, std::abs(values[i] - expected[i]));
      CHECK(TensorValues(sample.sensor) == TensorValues(local.sensor));
      CHECK(sample.target.zz == Approx(expected[5] + (p.z < .0625 ? 100 * (1 - p.z / .0625) : 0)));
      if (!repeat) {
        std::ostringstream line;
        line << std::setprecision(17) << "DONOR_SAMPLE " << world.rank << ' ' << p.x << ' ' << p.y << ' ' << p.z;
        for (auto value : TensorValues(sample.sensor)) line << ' ' << value;
        for (auto value : TensorValues(sample.target)) line << ' ' << value;
        for (auto id : sample.donor) line << ' ' << id;
        std::cout << line.str() << '\n';
      }
    }
  }
  CHECK(max_residual <= 2e-14);
  CHECK(composed == 8);
  CHECK(imported.Statistics().hits == 4);
  CHECK(imported.Statistics().interpolations == 4);
  CHECK(imported.Statistics().compositions == 8);
  CHECK(imported.SensorEdgeLength({0, 0, 0}, {1, 1, 1}) == Approx(double(oracle.SensorEdgeLength({0, 0, 0}, {1, 1, 1}))));
  CHECK_THROWS(imported.TargetEdgeLength({0, 0, 0}, {1, 1, 1}));
  auto inactive = service.Import({});
  CHECK(inactive.valid);
  CHECK(inactive.cells.empty());
  auto root_only = service.Import(world.rank ? std::vector<DonorRegion>{} : regions);
  CHECK(root_only.valid);
  CHECK(root_only.cells.size() == (world.rank ? 0 : 6));
  auto reject = service.Import(world.rank ? std::vector<DonorRegion>{} : std::vector<DonorRegion>{{2, 2, 2, 3, 3, 3}});
  CHECK_FALSE(reject.valid);
  CHECK(reject.cells.empty());
  CHECK_FALSE(service.Import(regions, 1).valid);
  CHECK_FALSE(service.Import(regions, SIZE_MAX, SIZE_MAX).valid);
  CHECK_FALSE(service.Import(std::vector<DonorRegion>(MaximumCavityCells + 1)).valid);
  CHECK_FALSE(service.Import({{NAN, 0, 0, 1, 1, 1}}).valid);
  CHECK(service.Import(regions).valid);
  std::ostringstream report;
  report << "DONOR rank=" << world.rank << " residual=" << std::setprecision(17) << max_residual
         << " requests=" << imported.Statistics().requests << " hits=" << imported.Statistics().hits
         << " compositions=" << composed << " imported=" << service.Statistics().payloads
         << " candidates=" << service.Statistics().index_candidates << " resident=" << service.ResidentBytes()
         << " build_s=" << service.Statistics().build_seconds << " import_s=" << service.Statistics().import_seconds
         << " routing_nested_s=" << service.Statistics().routing_seconds
         << " search_nested_s=" << service.Statistics().search_seconds
         << " exchange_nested_s=" << service.Statistics().exchange_seconds << '\n';
  std::cout << report.str() << std::flush;
}
TEST_CASE("Native MPI donor authority rejects inconsistent or duplicate source without freezing it", "[NativeDistributed3D]") {
  World world;
  const auto geometry = Box();
  const auto good = Source(geometry.patch.cells, world);
  for (int problem = 0; problem < 5; ++problem) {
    auto source = good;
    if (world.rank == world.size - 1) {
      if (problem < 2) {
        auto& row = source.front();
        for (size_t i = 0; i < 4; ++i) if (row.cell.v[i].id == 0) {
          if (problem == 0) row.sensor[i].xx += 1;
          else row.cell.v[i].p.x += .125;
        }
      } else if (problem == 2) {
        auto duplicate = Source(geometry.patch.cells, world, true).front();
        duplicate.cell.id = 1000;
        source.push_back(duplicate);
      } else if (problem == 3) {
        source[world.size == 1 ? 1 : 0].cell.id = 0;
      } else source.front().sensor.front().zz = -1;
    }
    DonorImport service(world);
    std::string reason;
    CHECK_FALSE(service.Build(source, reason));
    CHECK_FALSE(reason.empty());
    CHECK(service.ResidentBytes() == 0);
    CHECK_FALSE(service.Import({}).valid);
    REQUIRE(service.Build(good, reason));
    CHECK(service.Import({{0, 0, 0, 1, 1, 1}}).valid);
  }
  DonorImport empty(world);
  std::string reason;
  CHECK_FALSE(empty.Build({}, reason));
  CHECK_FALSE(empty.Build(good, reason, 1));
  REQUIRE(empty.Build(good, reason));
}
TEST_CASE("Native MPI donor import admits large immutable owners but rejects oversized patch unions", "[NativeDistributed3D]") {
  World world;
  std::vector<Cell> cells;
  for (Id i = 0; i <= MaximumDonors; ++i) {
    const double x = 2 * i;
    cells.push_back({i, {{{4 * i, {x, 0, 0}}, {4 * i + 1, {x + 1, 0, 0}},
                         {4 * i + 2, {x, 1, 0}}, {4 * i + 3, {x, 0, 1}}}}});
  }
  const auto original = Source(cells, world);
  DonorImport service(world);
  std::string reason;
  REQUIRE(service.Build(original, reason));
  auto patch = service.Import({{0, 0, 0, 1, 1, 1}});
  REQUIRE(patch.valid);
  REQUIRE(patch.cells.size() == 1);
  FrozenField field(patch.cells);
  CHECK(field.Query({.25, .25, .25}).sensor.xx == Approx(2.25));
  const auto resident = service.ResidentBytes();
  const double last = 2 * MaximumDonors + 1;
  auto oversized = service.Import({{0, 0, 0, last, 1, 1}});
  CHECK_FALSE(oversized.valid);
  CHECK(oversized.cells.empty());
  CHECK(service.ResidentBytes() == resident);
  CHECK(service.Import({{0, 0, 0, 1, 1, 1}}).valid);
}
TEST_CASE("Native MPI donor unions reuse exact admission without changing payload or rejecting invalid duplicates", "[NativeDistributed3D][NativeDonorReuse3D]") {
  World world;std::vector<Cell> cells;
  for (Id i=0;i<=MaximumDonors;++i) {
    const double x=2*i;
    cells.push_back({i,{{{4*i,{x,0,0}},{4*i+1,{x+1,0,0}},{4*i+2,{x,1,0}},{4*i+3,{x,0,1}}}}});
  }
  DonorImport service(world);std::string reason;
  REQUIRE(service.Build(Source(cells,world),reason));const auto resident=service.ResidentBytes();
  for (bool metric:{false,true}) {
    const auto query=world.rank ? std::vector<Cell>{} : std::vector<Cell>{cells.front()};
    const auto single=metric ? service.ImportMetricCells(query) : service.ImportCells(query);REQUIRE(single.valid);
    const auto reused_before=service.Statistics().reused_candidates, filtered_before=service.Statistics().filter_candidates;
    const auto duplicate=world.rank ? std::vector<Cell>{} : std::vector<Cell>{cells.front(),cells.front()};
    const auto twice=metric ? service.ImportMetricCells(duplicate) : service.ImportCells(duplicate);REQUIRE(twice.valid);
    REQUIRE(twice.cells.size()==single.cells.size());
    if(!world.rank) {
      REQUIRE(twice.cells.size()==1);CHECK(twice.cells.front().Key()==single.cells.front().Key());
      for(size_t j=0;j<4;++j) CHECK(TensorValues(twice.cells.front().sensor[j])==TensorValues(single.cells.front().sensor[j]));
      FrozenField field(twice.cells);CHECK(field.Query({.25,.25,.25}).sensor.xx==Approx(2.25));
    }
    CHECK(world.sum(int(service.Statistics().reused_candidates-reused_before))>0);
    CHECK(world.sum(int(service.Statistics().filter_candidates-filtered_before))==1);
    auto invalid=duplicate;if(!invalid.empty())std::swap(invalid.back().v[0],invalid.back().v[1]);
    const auto rejected=metric ? service.ImportMetricCells(invalid) : service.ImportCells(invalid);
    CHECK_FALSE(rejected.valid);CHECK(rejected.cells.empty());CHECK(service.ResidentBytes()==resident);
  }
}

TEST_CASE("Native immutable sensor import works with fewer workers than CFD ranks", "[NativeDistributed3D]") {
  World parent;
  const auto geometry = Box();
  for (int m : std::set<int>{parent.size, std::max(1, parent.size - 1), std::max(1, parent.size / 2)}) {
    MPI_Comm subset = MPI_COMM_NULL;
    MPI_Comm_split(parent.comm, parent.rank < m ? 0 : MPI_UNDEFINED, parent.rank, &subset);
    bool valid = true;
    if (subset != MPI_COMM_NULL) {
      World worker(subset);
      DonorImport service(worker);
      std::string reason;
      valid = service.Build(Source(geometry.patch.cells, worker), reason);
      // Calls match even after a rejected build; Build result must be common to this worker group.
      if (valid) {
        auto patch = service.Import({{0, 0, 0, 1, 1, 1}});
        valid = patch.valid && patch.cells.size() == 6;
        if (valid) try {
          FrozenField field(patch.cells);
          valid = TensorValues(field.Query({.125, .25, .375}).sensor) == TensorValues(Sensor({.125, .25, .375}));
        } catch (const std::exception&) { valid = false; }
      }
      if (valid) {
        // Only worker zero owns this source; other workers have genuinely empty local indices.
        DonorImport sparse(worker);
        const auto single = Source({geometry.patch.cells.front()}, worker);
        valid = sparse.Build(single, reason);
        if (valid) {
          auto patch = sparse.Import({{0, 0, 0, 1, 1, 1}});
          valid = patch.valid && patch.cells.size() == 1;
          if (valid) try {
            FrozenField field(patch.cells);
            valid = TensorValues(field.Query({.75, .5, .25}).sensor) == TensorValues(Sensor({.75, .5, .25}));
          } catch (const std::exception&) { valid = false; }
        }
      }
      MPI_Comm_free(&subset);
    }
    CHECK_FALSE(parent.maximum(!valid));
  }
}
#endif

#ifdef HAVE_MPI
namespace {
std::vector<DonorCell> Uniform(const Owned& owned, Tensor tensor) {
  std::vector<DonorCell> source;
  for (const auto& row : owned) { DonorCell donor; donor.cell=row.second.cell; donor.sensor.fill(tensor); source.push_back(donor); }
  return source;
}
void EngineReport(const EngineResult& result, World& world, const std::string& label="box") {
  const auto& s=result.stats;
  std::ostringstream line;
  line << std::setprecision(17) << "ENGINE rank=" << world.rank << " ranks=" << world.size << " valid=" << result.valid
       << " case=" << label << " initial=" << s.initial_cells << " updated=" << s.updated_cells << " final=" << result.cells.size()
       << " seed_batches=" << s.seed_batches << " unmeasured_retired=" << s.unmeasured_retired
       << " rounds=" << s.rounds << " proposed=" << s.proposed << " attempts=" << s.attempted
       << " accepted=" << s.accepted << " rejected=" << s.rejected << " conflicts=" << s.conflicts
       << " collapse_geometry_checks=" << s.collapse_geometry_checks << " collapse_filtered=" << s.collapse_filtered
             << " collapse_orientation_checks=" << s.collapse_orientation_checks << " collapse_orientation_filtered=" << s.collapse_orientation_filtered
             << " collapse_reactivated=" << s.collapse_reactivated << " changed_star_scans=" << s.changed_star_scans
             << " changed_vertices_sent=" << s.changed_vertices_sent << " changed_vertices_received=" << s.changed_vertices_received
             << " nested_change_notifications=" << s.change_notification_seconds << " nested_collapse_orientation=" << s.collapse_orientation_seconds
       << " collapse_geometry_nested_s=" << s.collapse_geometry_seconds
       << " selection_edge_evaluations=" << s.edge_evaluations << " selection_edge_reused=" << s.edge_reused
       << " wall_reconstruction_trials=" << s.wall_reconstruction_trials << " wall_reconstruction_accepted=" << s.wall_reconstruction_accepted
       << " wall_joint_trials=" << s.wall_joint_trials << " wall_joint_moves=" << s.wall_joint_moves << " wall_joint_removals=" << s.wall_joint_removals << " wall_joint_accepted=" << s.wall_joint_accepted
       << " placement_connectivity_trials=" << s.placement_connectivity_trials << " placement_flips=" << s.placement_flips << " placement_moves=" << s.placement_moves
       << " placement_dependency_expansions=" << s.placement_dependency_expansions
       << " placement_classified_vertices=" << s.placement_classified_vertices
             << " placement_trials=" << s.placement_trials << " placement_validated=" << s.placement_validated
       << " wall_topology_trials=" << s.wall_topology_trials << " wall_topology_accepted=" << s.wall_topology_accepted
       << " field_requests=" << s.field_requests
       << " field_hits=" << s.field_hits << " field_misses=" << s.field_misses
       << " field_invalid_queries=" << s.field_invalid_queries << " field_failures=" << s.field_failures
       << " field_evictions=" << s.field_evictions << " field_edges=" << s.field_edges << " work_bytes=" << s.maximum_work_bytes
       << " total_s=" << s.total_seconds << " init_s=" << s.initialization_seconds << " seed_s=" << s.seed_seconds
       << " selection_s=" << s.selection_seconds << " dependency_s=" << s.dependency_seconds
       << " reservation_s=" << s.reservation_seconds << " donor_s=" << s.donor_seconds
       << " reconstruction_s=" << s.reconstruction_seconds << " metric_s=" << s.metric_seconds
       << " routing_s=" << s.routing_seconds << " publication_s=" << s.publication_seconds
       << " update_s=" << s.update_seconds << " completion_s=" << s.completion_seconds << " other_s=" << s.other_seconds
       << " tracked_bytes=" << s.tracked_bytes << " tracked_calls=" << s.tracked_calls
       << " tracked_collectives_nested_s=" << s.tracked_collective_seconds
       << " directory_elections=" << s.directory_elections << " directory_election_nested_s=" << s.directory_election_seconds
       << " process_cpu_s=" << s.process_cpu_seconds << " reconstruction_cpu_s=" << s.reconstruction_cpu_seconds
       << " metric_cpu_s=" << s.metric_cpu_seconds << " measurement_cpu_s=" << s.measurement_cpu_seconds
       << " repair_fallbacks=" << s.repair_fallbacks << " fallback_accepted=" << s.fallback_accepted
       << " recovery_starts=" << s.recovery_starts << " repair_proposals=" << s.repair_proposals << " moved=" << s.moved << " reconnected=" << s.reconnected << " inserted=" << s.inserted
       << " batches=" << s.donor_batches << " batch_retries=" << s.batch_retries
       << " query_import_retries=" << s.query_import_retries << " private_query_retries=" << s.private_query_retries
       << " donor_reused=" << s.donor_reused << " edge_separated=" << s.edge_separated
       << " donor_candidates=" << s.donor_candidates << " donor_filtered=" << s.donor_filtered << " donor_box_filtered=" << s.donor_box_filtered
       << " donor_payloads=" << s.donor_payloads << " donor_maximum_patch=" << s.donor_maximum_patch
       << " donor_search_nested_s=" << s.donor_search_seconds << " donor_filter_nested_s=" << s.donor_filter_seconds
       << " planning_nested_s=" << s.planning_seconds
       << " batch_considered=" << s.batch_considered << " batch_deferred=" << s.batch_deferred
       << " proposal_duplicates=" << s.proposal_duplicates
             << " batch_rounds=" << s.batch_rounds << " batch_candidates=" << s.batch_candidates
             << " stop=" << EngineStopName(s.stop) << " global_work=" << s.global_work << " work_limit=" << s.work_limit
             << " progress_windows=" << s.progress_windows << " stagnant_windows=" << s.stagnant_windows << " search_windows=" << s.search_windows
             << " bidirectional=" << s.bidirectional << " length_split_passes=" << s.length_split_passes
         << " length_collapse_passes=" << s.length_collapse_passes << " length_splits=" << s.length_splits
         << " length_collapses=" << s.length_collapses << " length_pass_deferred=" << s.length_pass_deferred
         << " size_progress=" << s.size_progress << " quality_progress=" << s.quality_progress << " cell_removals=" << s.cell_removals
             << " geometry_trials=" << s.geometry_trials << " geometry_rejected=" << s.geometry_rejected << " metric_trials=" << s.metric_trials
             << " metric_shape_rejected=" << s.metric_shape_rejected << " metric_progress_rejected=" << s.metric_progress_rejected
             << " metric_measurement_rejected=" << s.metric_measurement_rejected
             << " sample_metric_present=" << (s.last_metric_rejection.failure!=MetricFailure::None)
             << " sample_shape_measured=" << s.last_metric_rejection.shape_measured << " sample_q=";
  if(s.last_metric_rejection.shape_measured)line << s.last_metric_rejection.minimum_mean_ratio;else line << "unavailable";
  line << " sample_j=";
  if(s.last_metric_rejection.shape_measured)line << s.last_metric_rejection.minimum_scaled_jacobian;else line << "unavailable";
  line << " sample_cell=" << s.last_metric_rejection.weakest_cell << " sample_sliver=" << s.last_metric_rejection.sliver_cell
             << " sample_feature=" << s.last_rejected_feature.count << " sample_v0=" << s.last_rejected_feature.vertices[0]
             << " sample_v1=" << s.last_rejected_feature.vertices[1] << " sample_v2=" << s.last_rejected_feature.vertices[2]
             << " sample_x=" << s.last_rejected_cavity_centroid.x << " sample_y=" << s.last_rejected_cavity_centroid.y << " sample_z=" << s.last_rejected_cavity_centroid.z
       << " coarsen_checkpoints=" << s.coarsen_checkpoints << " coarsen_committed_passes=" << s.coarsen_committed_passes
       << " coarsen_rolled_back_passes=" << s.coarsen_rolled_back_passes << " coarsen_retired_directions=" << s.coarsen_retired_directions
       << " coarsen_restored_cells=" << s.coarsen_restored_cells << " coarsen_rolled_back_collapses=" << s.coarsen_rolled_back_collapses
       << " quality_stages=" << s.quality_stages << " construction_returns=" << s.construction_returns
       << " shape_measured=" << s.last_metric_rejection.shape_measured
       << " batch_reserved=" << s.batch_reserved
       << " concurrent_rounds=" << s.concurrent_rounds << " final_reservations=" << s.final_reservations
       << " proposal_deferred=" << s.proposal_deferred << " maximum_parallel_commits=" << s.maximum_parallel_commits << " field_build_nested_s=" << s.field_build_seconds << " field_query_nested_s=" << s.field_query_seconds
       << " cache_nested_s=" << s.field_cache_seconds << " search_nested_s=" << s.field_search_seconds
       << " interpolation_nested_s=" << s.field_interpolation_seconds << " composition_nested_s=" << s.field_composition_seconds
       << " edge_nested_s=" << s.edge_seconds << " edge_search_nested_s=" << s.edge_search_seconds
       << " edge_trace_nested_s=" << s.edge_trace_seconds << " edge_integral_nested_s=" << s.edge_integral_seconds << '\n';
  Log(world,line.str());
}
}
TEST_CASE("Native incremental MPI refinement reaches complete frozen metric gates", "[NativeDistributed3D]") {
  World world;
  const auto fixture=Box();
  const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);
  const auto saved=Bytes(input);
  EngineOptions options; options.timings=true;
  auto result=Refine(world,input,Uniform(input,{4,0,0,4,0,4}),reference,{},options);
  if (!result.valid) std::cerr<<"Engine rejected rank "<<world.rank<<": "<<result.reason<<'\n';
  REQUIRE(result.valid);
  CHECK(Bytes(input)==saved);
  CHECK(world.sum(int(result.splits.size()))>1);
  CHECK(result.stats.initial_cells==input.size());
  CHECK(result.stats.updated_cells>=result.cells.size()-input.size());
  CHECK(result.stats.other_seconds>=-1e-12);
  CHECK(result.stats.edge_evaluations+result.stats.edge_reused==6*(result.stats.initial_cells+result.stats.updated_cells));
  CHECK(result.stats.edge_reused>0);
  const auto total=CPassiveComm::Allreduce(result.cells.size(),CPassiveComm::Op::SUM,world.comm);
  CHECK(total>6);
  CHECK(total<=128);
  CHECK(Audit(world,result.cells,fixture,total));
  const auto final=Gather(world,result.cells);
  if (!world.rank) {
    auto cells=PatchOf(final).cells;
    // Root needs the full original source for its independent final whole-mesh check.
    std::vector<DonorCell> all;
    for(auto cell:fixture.patch.cells){DonorCell d;d.cell=cell;d.sensor.fill({4,0,0,4,0,4});all.push_back(d);}
    FrozenField target(all);
    PatchMetric metric;std::string reason;
    CHECK(ValidateMetricPatch(cells,target,metric,reason));
  }
  MeshRecords("engine_full",world,result.cells);
  EngineReport(result,world);
  if (world.size==1) {
    options.timings=false;
    auto untimed=Refine(world,input,Uniform(input,{4,0,0,4,0,4}),reference,{},options);
    CHECK(untimed.valid);
    CHECK(Bytes(untimed.cells)==Bytes(result.cells));
    CHECK(untimed.stats.accepted==result.stats.accepted);
    CHECK(untimed.stats.updated_cells==result.stats.updated_cells);
    CHECK(untimed.stats.total_seconds==0);
  }
}
TEST_CASE("Native construction skips centroid shape queries and requires a fresh quality census", "[NativeStagedConstruction3D]") {
  World world;const auto fixture=Box();const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  std::set<std::array<double,3>> centroids;
  auto remember=[&](const std::vector<Cell>& cells) {
    for(const auto& c:cells){const auto p=Centroid(c);centroids.insert({p.x,p.y,p.z});}
  };
  remember(fixture.patch.cells);
  unsigned centroid_queries=0,construction_metrics=0;
  EngineOptions options;options.maximum_rounds=1;options.candidate_batch=1;
  options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  options.capture_feature={{0,6,0},2};
  options.transaction_observer=[&](const TransactionView& view) {
    if(std::string(view.stage)=="SplitEdge" && view.passed)remember(view.fresh.cells);
    if(std::string(view.stage)=="construction_end" && view.passed) {
      ++construction_metrics;CHECK_FALSE(view.metric.shape_measured);
      CHECK(std::isnan(view.metric.minimum_mean_ratio));CHECK(std::isnan(view.metric.minimum_scaled_jacobian));
    }
  };
  auto compose=[&](Point p,Tensor sensor) {
    centroid_queries+=centroids.count({p.x,p.y,p.z});return sensor;
  };
  const auto limited=Refine(world,input,Uniform(input,{4,0,0,4,0,4}),reference,compose,options);
  INFO(limited.reason);CHECK_FALSE(limited.valid);CHECK(limited.stats.stop==EngineStop::RoundBudget);
  CHECK(world.sum(int(limited.stats.accepted))>0);CHECK(world.sum(int(construction_metrics))>0);
  CHECK(world.sum(int(centroid_queries))==0);CHECK(limited.stats.quality_stages==0);
  CHECK(Bytes(input)==saved);CHECK_FALSE(limited.rebuilt_domain);
  for(const auto& row:limited.cells) {
    const auto& v=row.second.cell.v;CHECK(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);
  }
  // Size/geometry already fit, so failure at a centroid can only occur after construction hands off.
  options.maximum_rounds=0;options.transaction_observer={};centroid_queries=0;
  auto reject_centroids=[&](Point p,Tensor sensor) {
    if(centroids.count({p.x,p.y,p.z})) {++centroid_queries;throw std::runtime_error("strict quality census sentinel");}
    return sensor;
  };
  const auto censused=Refine(world,input,Uniform(input,Tensor{}),reference,reject_centroids,options);
  INFO(censused.reason);CHECK_FALSE(censused.valid);CHECK(censused.stats.quality_stages==1);
  CHECK(world.sum(int(centroid_queries))>0);CHECK(Bytes(input)==saved);CHECK_FALSE(censused.rebuilt_domain);
}

TEST_CASE("Native composed measurements interleave complete cavities and certify every survivor", "[NativeProgressiveMeasurement3D]") {
  World world;const bool concentrated=GENERATE(false,true);Fixture fixture;
  const Id components=8*world.size+1;
  for(Id i=0;i<components;++i) {
    auto box=Box();
    for(auto& cell:box.patch.cells) {cell.id+=100*i;for(auto& n:cell.v){n.id+=16*i;n.p.x+=2*i;}}
    for(auto& face:box.patch.surface){face.facet+=12*i;for(auto& n:face.v)n+=16*i;}
    for(auto& f:box.facets){f.id+=12*i;for(auto& n:f.v){n.id+=16*i;n.p.x+=2*i;}}
    fixture.patch.cells.insert(fixture.patch.cells.end(),box.patch.cells.begin(),box.patch.cells.end());
    fixture.patch.surface.insert(fixture.patch.surface.end(),box.patch.surface.begin(),box.patch.surface.end());
    fixture.facets.insert(fixture.facets.end(),box.facets.begin(),box.facets.end());
  }
  Owned input;for(const auto& row:Rows(fixture.patch))
    if(concentrated ? world.rank==0 : row.cell.id%world.size==unsigned(world.rank))input.emplace(row.cell.id,row);
  const auto saved=Bytes(input);const PlanarReference geometry(fixture.facets);
  // The original field is independently distributed, including empty working owners.
  const auto original=Partition(Rows(fixture.patch),world);
  const auto source=Uniform(original,{4,0,0,4,0,4});
  const MetricComposition compose=[](Point,Tensor m){return m;};
  EngineOptions options;options.timings=true;options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  options.maximum_work=0;
  const auto limited=Refine(world,input,source,geometry,compose,options);
  CHECK_FALSE(limited.valid);CHECK(limited.stats.stop==EngineStop::WorkBudget);
  CHECK(world.sum(int(limited.stats.cached_measured_cells))<int(fixture.patch.cells.size()));CHECK(Bytes(input)==saved);
  options.maximum_work=4096;
  const auto result=Refine(world,input,source,geometry,compose,options);INFO(result.reason);REQUIRE(result.valid);
  CHECK(Bytes(input)==saved);CHECK(result.stats.cached_measured_cells==result.cells.size());CHECK(result.stats.cached_invalid_cells==0);
  CHECK(world.sum(int(result.stats.unmeasured_retired))>0);
  CHECK(world.sum(int(result.stats.initial_cells))<int(fixture.patch.cells.size()));
  const auto total=CPassiveComm::Allreduce(result.cells.size(),CPassiveComm::Op::SUM,world.comm);
  CHECK(Audit(world,result.cells,fixture,total));
  const auto final=Gather(world,result.cells);
  if(!world.rank) {
    Owned all;for(const auto& row:Rows(fixture.patch))all.emplace(row.cell.id,row);
    FrozenField full(Uniform(all,{4,0,0,4,0,4}));PatchMetric metric;std::string reason;
    const auto cells=PatchOf(final).cells;
    for(size_t at=0;at<cells.size();at+=MaximumReplacementCells)
      CHECK(ValidateMetricPatch(std::vector<Cell>(cells.begin()+at,cells.begin()+std::min(cells.size(),at+MaximumReplacementCells)),full,metric,reason));
  }
  MeshRecords(concentrated?"progressive_concentrated_initial":"progressive_distributed_initial",world,input);
  MeshRecords(concentrated?"progressive_concentrated_accepted":"progressive_distributed_accepted",world,result.cells);
  EngineReport(result,world,concentrated?"progressive_concentrated":"progressive_distributed");
  // An admissible prefix must not hide either ordinary remaining cells or an invalid tail field.
  const auto unit=Uniform(original,{1,0,0,1,0,1});
  const auto unchanged=Refine(world,input,unit,geometry,compose,options);INFO(unchanged.reason);REQUIRE(unchanged.valid);
  CHECK(unchanged.stats.rounds==0);CHECK(world.sum(int(unchanged.stats.initial_cells))==int(fixture.patch.cells.size()));
  CHECK(unchanged.stats.cached_measured_cells==unchanged.cells.size());CHECK(unchanged.stats.seed_batches>1);
  const MetricComposition invalid=[=](Point p,Tensor m){return p.x>2*double(components-1)-.5 ? Tensor{-1,0,0,1,0,1}:m;};
  CHECK_FALSE(Refine(world,input,unit,geometry,invalid,options).valid);CHECK(Bytes(input)==saved);
}

TEST_CASE("Productive construction services descendant generations before quality", "[NativeConstructionSweep3D]") {
  const bool bidirectional=GENERATE(false,true);
  World world;const auto fixture=Box();const PlanarReference geometry(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  std::set<Edge> entry;
  for(const auto& c:fixture.patch.cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
    Edge edge{c.v[i].id,c.v[j].id};std::sort(edge.begin(),edge.end());entry.insert(edge);
  }
  EngineOptions options;options.candidate_batch=1;options.maximum_rounds=entry.size()+1;options.maximum_work=4096;
  const Tensor tensor{64,0,0,64,0,64};bool construction_unmeasured=true;
  options.stage_observer=[&](const StageProgressView& view) {
    if(view.phase==EnginePhase::Construction)construction_unmeasured &=
        view.progress.shape_measured_cells==0 && view.progress.q_measured_cells==0 && view.progress.j_measured_cells==0;
  };
  const auto result=bidirectional?AdaptMetric(world,input,Uniform(input,tensor),geometry,{},options):
      Refine(world,input,Uniform(input,tensor),geometry,{},options);INFO(result.reason);
  CHECK_FALSE(result.valid);CHECK(result.stats.quality_stages==0);CHECK(world.sum(result.stats.accepted)>0);
  CHECK(world.sum(result.stats.attempted)>entry.size());CHECK(construction_unmeasured);
  bool descendant_debt=false;
  for(const auto& row:result.cells) {
    const auto& v=row.second.cell.v;CHECK(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
      Edge edge{v[i].id,v[j].id};std::sort(edge.begin(),edge.end());
      descendant_debt=descendant_debt || (!entry.count(edge) && MetricNorm(v[i].p,v[j].p,tensor)>MaximumMetricEdge);
    }
  }
  CHECK(world.maximum(descendant_debt));CHECK(Bytes(input)==saved);
  CHECK(result.stats.global_work<=options.maximum_work);
  if(bidirectional) {
    CHECK(result.stats.length_collapse_passes==1);CHECK(result.stats.length_split_passes>=2);
    EngineOptions complete;complete.maximum_work=4096;
    const auto admissible=AdaptMetric(world,input,Uniform(input,Tensor{}),geometry,{},complete);INFO(admissible.reason);
    CHECK(admissible.valid);CHECK(admissible.stats.quality_stages>0);
    CHECK(admissible.stats.length_collapse_passes==1);CHECK(admissible.stats.length_split_passes==1);
  }
}

TEST_CASE("Interior split lane services neighboring entry edges without construction claims", "[NativeConstructionSweep3D][NativeSplitLanes3D]") {
  const auto batch=GENERATE(size_t(1),MaximumCandidateBatch);
  World world;const auto seed=Box();const PlanarReference geometry(seed.facets);Fixture fixture;fixture.facets=seed.facets;
  constexpr unsigned n=3,side=n+1;Id next=0;
  for(unsigned z=0;z<n;++z)for(unsigned y=0;y<n;++y)for(unsigned x=0;x<n;++x)for(const auto& base:seed.patch.cells) {
    Cell cell;cell.id=next++;
    for(unsigned k=0;k<4;++k){const auto p=base.v[k].p;const unsigned i=x+unsigned(p.x),j=y+unsigned(p.y),l=z+unsigned(p.z);
      cell.v[k]={i+side*(j+side*l),{double(i)/n,double(j)/n,double(l)/n}};}
    fixture.patch.cells.push_back(cell);
  }
  const Incidence incidence(fixture.patch.cells);std::map<Id,Point> points;
  for(const auto& cell:fixture.patch.cells)for(const auto& node:cell.v)points.emplace(node.id,node.p);
  std::set<Edge> boundary_edges;
  for(const auto& face:incidence.Boundary()) {
    Id facet=UINT64_MAX;for(const auto& f:fixture.facets)if(geometry.Contains(f.id,points.at(face[0]))&&geometry.Contains(f.id,points.at(face[1]))&&geometry.Contains(f.id,points.at(face[2]))){facet=f.id;break;}
    REQUIRE(facet!=UINT64_MAX);fixture.patch.surface.push_back({face,facet});
    for(unsigned k=0;k<3;++k){Edge edge{face[k],face[(k+1)%3]};std::sort(edge.begin(),edge.end());boundary_edges.insert(edge);}
  }
  std::set<Edge> interior;for(const auto& cell:fixture.patch.cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){Edge e{cell.v[i].id,cell.v[j].id};std::sort(e.begin(),e.end());if(!boundary_edges.count(e))interior.insert(e);}
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);std::set<Edge> before,after;
  bool entry_seen=false,exit_seen=false,unmeasured=true;
  auto collect=[](const Owned& cells){std::set<Edge> edges;for(const auto& row:cells){const auto& v=row.second.cell.v;
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){Edge e{v[i].id,v[j].id};std::sort(e.begin(),e.end());edges.insert(e);}}return edges;};
  EngineOptions options;options.maximum_work=4096;options.maximum_rounds=512;options.candidate_batch=batch;
  options.phase_ownership=PhaseOwnership::Disabled;
  options.stage_observer_bytes=size_t(1)<<20;
  options.stage_observer=[&](const StageProgressView& view){
    if(view.phase==EnginePhase::Construction)unmeasured &= view.progress.q_measured_cells==0 && view.progress.j_measured_cells==0;
    if(!entry_seen&&view.stats.split_generation==1 && std::string(view.stage)=="split_interior_entry"){before=collect(view.working);entry_seen=true;}
    if(view.stats.split_generation==1 && std::string(view.stage)=="length_split_exit"){after=collect(view.working);exit_seen=true;}
  };
  const auto result=AdaptMetric(world,input,Uniform(input,{100,0,0,100,0,100}),geometry,{},options);
  INFO(result.reason);CHECK(world.maximum(entry_seen));CHECK(world.maximum(exit_seen));CHECK(unmeasured);CHECK(Bytes(input)==saved);
  std::vector<Edge> serviced;
  for(const auto& edge:interior)if(world.maximum(before.count(edge)!=0) && !world.maximum(after.count(edge)!=0))serviced.push_back(edge);
  bool neighboring=false;for(size_t i=0;i<serviced.size();++i)for(size_t j=0;j<i;++j)
    neighboring=neighboring||serviced[i][0]==serviced[j][0]||serviced[i][0]==serviced[j][1]||serviced[i][1]==serviced[j][0]||serviced[i][1]==serviced[j][1];
  CHECK(neighboring);CHECK(result.stats.interior_rounds>0);
  if(batch>1 && world.size>=3)CHECK(result.stats.maximum_parallel_split_commits>=2);
}

TEST_CASE("Native MPI refinement preserves accepted input on incomplete or invalid events", "[NativeDistributed3D]") {
  World world;
  const auto fixture=Box();const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  EngineOptions options;options.maximum_rounds=1;
  auto limited=Refine(world,input,Uniform(input,{4,0,0,4,0,4}),reference,{},options);
  CHECK_FALSE(limited.valid);
  CHECK(limited.reason.find("round limit")!=std::string::npos);
  CHECK(limited.reason.find("cached metric size/shape gates")!=std::string::npos);
  CHECK(world.maximum(limited.stats.cached_invalid_cells!=0));
  CHECK(Bytes(input)==saved);
  CHECK(Audit(world,limited.cells,fixture,12));
  MeshRecords("engine_rejected",world,limited.cells);
  options.maximum_rounds=512;options.memory_ceiling=1;
  auto budget=Refine(world,input,Uniform(input,{4,0,0,4,0,4}),reference,{},options);
  CHECK_FALSE(budget.valid);
  CHECK(Bytes(input)==saved);
  options.memory_ceiling=size_t(2)<<30;
  options.maximum_rounds=world.rank ? 2 : 1;
  if (world.size>1) CHECK_FALSE(Refine(world,input,Uniform(input,{4,0,0,4,0,4}),reference,{},options).valid);
  options.maximum_rounds=512;
  auto unchanged=Refine(world,input,Uniform(input,{}),reference,{},options);
  CHECK(unchanged.valid);
  CHECK(Bytes(unchanged.cells)==Bytes(WithUniformSensor(input,{})));
  CHECK(unchanged.stats.accepted==0);
  auto exhausted=input;
  if (exhausted.count(0)) {
    auto row=exhausted.at(0);exhausted.erase(0);
    row.cell.id=UINT64_MAX-64;exhausted.emplace(row.cell.id,row);
  }
  const auto exhausted_bytes=Bytes(exhausted);
  auto identity=Refine(world,exhausted,Uniform(exhausted,{4,0,0,4,0,4}),reference,{},options);
  CHECK_FALSE(identity.valid);
  CHECK(identity.reason.find("identity range")!=std::string::npos);
  CHECK(identity.stats.accepted==0);
  CHECK(Bytes(exhausted)==exhausted_bytes);
}
TEST_CASE("Native full MPI refinement returns from fewer workers with accepted input preserved", "[NativeDistributed3D]") {
  World parent;const auto fixture=Box();const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),parent);const auto saved=Bytes(input);
  for(int m:std::set<int>{parent.size,std::max(1,parent.size-1),std::max(1,parent.size/2)}) {
    std::vector<std::vector<OwnedCell>> to(parent.size);
    for(const auto& row:input) to[row.first%m].push_back(row.second);
    const auto migrated=parent.exchange(to);
    MPI_Comm subset=MPI_COMM_NULL;
    MPI_Comm_split(parent.comm,parent.rank<m ? 0 : MPI_UNDEFINED,parent.rank,&subset);
    Owned refined;bool valid=true;
    if(subset!=MPI_COMM_NULL) {
      World worker(subset);Owned owned;
      for(auto row:migrated) owned.emplace(row.cell.id,row);
      auto result=Refine(worker,owned,Uniform(owned,{4,0,0,4,0,4}),reference);
      valid=result.valid;refined.swap(result.cells);
      MPI_Comm_free(&subset);
    }
    CHECK_FALSE(parent.maximum(!valid));
    to.assign(parent.size,{});
    for(const auto& row:refined) to[row.first%parent.size].push_back(row.second);
    const auto returned=parent.exchange(to);Owned output;
    for(auto row:returned) output.emplace(row.cell.id,row);
    const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);
    CHECK(count>6);CHECK(count<=128);
    CHECK(Audit(parent,output,fixture,count));
    CHECK(Bytes(input)==saved);
    MeshRecords("engine_m"+std::to_string(m),parent,output);
  }
}

namespace {
struct AnisotropicCase { const char* name; double height; bool shear; unsigned spatial; };
Tensor AnisotropicSensor(Point p, const AnisotropicCase& spec) {
  if (spec.shear) return {4,-4,0,8,0,4/(spec.height*spec.height)};
  if (!spec.spatial) return {4,0,0,4,0,4/(spec.height*spec.height)};
  const double angle=spec.spatial==2 ? .2+.6*p.x : .37;
  const double c=std::cos(angle),s=std::sin(angle),x=4+.4*p.x,y=2+.4*p.y,z=4+.4*p.z;
  return {x*c*c+y*s*s,(x-y)*c*s,0,x*s*s+y*c*c,0,z};
}
void AnisotropicSourceRecords(const char* kind,const std::string& label,World& world,const std::vector<DonorCell>& source) {
  std::ostringstream output;
  for (const auto& donor:source) {
    std::ostringstream line;
    line<<std::setprecision(17)<<kind<<' '<<label<<' '<<world.rank<<' '<<donor.cell.id;
    for(size_t i=0;i<4;++i) {
      const auto n=donor.cell.v[i];line<<' '<<n.id<<' '<<n.p.x<<' '<<n.p.y<<' '<<n.p.z;
      for(auto value:TensorValues(donor.sensor[i])) line<<' '<<value;
    }
    output<<line.str()<<'\n';
  }
  Log(world,output.str());
}
}
TEST_CASE("Native full MPI refinement handles thin sheared and spatial anisotropic targets", "[NativeDistributed3D]") {
  World parent;
  using Clock=std::chrono::steady_clock;
  auto elapsed=[](Clock::time_point start){return std::chrono::duration<double>(Clock::now()-start).count();};
  // Frozen local-first matrix; no geometry association/metric/quality relaxation on failure.
  for(const auto spec:std::array<AnisotropicCase,5>{{{"thin4",1e-4,false,false},
      {"thin8",1e-8,false,false},{"shear4",1e-4,true,false},{"spatial",1,false,1},{"turning",1,false,2}}}) {
    const std::string name="aniso_"+std::string(spec.name);
    const auto fixture=Box(spec.height,spec.shear);const PlanarReference geometry(fixture.facets);
    const auto input=Partition(Rows(fixture.patch),parent);const auto saved=Bytes(input);
    const auto preparation_start=Clock::now();
    std::vector<DonorCell> original;
    for(const auto& row:input) {
      DonorCell d;d.cell=row.second.cell;
      for(size_t i=0;i<4;++i) d.sensor[i]=AnisotropicSensor(d.cell.v[i].p,spec);
      original.push_back(d);
    }
    const auto preparation_seconds=elapsed(preparation_start);
    std::ostringstream prep;prep<<std::setprecision(17)<<"ANISO_PREP case="<<name<<" rank="<<parent.rank
        <<" ranks="<<parent.size<<" synthetic_metric_s="<<preparation_seconds<<" cells="<<original.size()<<'\n';
    Log(parent,prep.str());
    MeshRecords(name+"_initial",parent,input);
    AnisotropicSourceRecords("ANISO_INPUT",name,parent,original);
    for(int m:std::set<int>{parent.size,std::max(1,parent.size-1),std::max(1,parent.size/2)}) {
      const auto label=name+"_m"+std::to_string(m);
      const auto cycle_start=Clock::now(),migration_start=cycle_start;const auto cpu_start=std::clock();
      std::vector<std::vector<OwnedCell>> to(parent.size);
      std::vector<std::vector<DonorCell>> sensors(parent.size);
      for(const auto& row:input) to[row.first%m].push_back(row.second);
      for(auto d:original) sensors[d.cell.id%m].push_back(d);
      const auto migrated=parent.exchange(to);const auto sensor_rows=parent.exchange(sensors);
      const auto migration_seconds=elapsed(migration_start);const auto split_start=Clock::now();
      MPI_Comm subset=MPI_COMM_NULL;
      MPI_Comm_split(parent.comm,parent.rank<m ? 0 : MPI_UNDEFINED,parent.rank,&subset);
      const auto split_seconds=elapsed(split_start);const auto worker_start=Clock::now();
      double source_audit_seconds=0,engine_external_seconds=0;
      Owned refined;bool valid=true;
      if(subset!=MPI_COMM_NULL) {
        World worker(subset);Owned owned;
        for(auto row:migrated) owned.emplace(row.cell.id,row);
        const auto source_audit_start=Clock::now();
        // Audit actual transported records and canonical P1 samples before the event.
        DonorImport service(worker);std::string reason;
        bool imported=service.Build(sensor_rows,reason);
        if(imported) {
          auto patch=service.Import({{0,0,0,spec.shear ? 2. : 1.,1.,spec.height}});
          imported=patch.valid && patch.cells.size()==6;
          if(imported) {
            AnisotropicSourceRecords("ANISO_SOURCE",label,worker,patch.cells);
            FrozenField field(patch.cells);
            for(auto p:std::array<Point,4>{{{.125,.25,.375*spec.height},{.5,.5,.5*spec.height},
                {0,0,0},{.25,.375,.03125*spec.height}}}) {
              if(spec.shear) p.x+=p.y;
              const auto sample=field.Query(p);
              std::ostringstream line;line<<std::setprecision(17)<<"ANISO_SAMPLE "<<label<<' '<<worker.rank
                  <<' '<<p.x<<' '<<p.y<<' '<<p.z;
              for(auto value:TensorValues(sample.sensor)) line<<' '<<value;
              for(auto id:sample.donor) line<<' '<<id;
              Log(worker,line.str()+"\n");
            }
          }
        }
        source_audit_seconds=elapsed(source_audit_start);
        CHECK(imported);
        EngineOptions options;options.timings=true;
        const auto engine_start=Clock::now();
        auto result=Refine(worker,owned,sensor_rows,geometry,{},options);
        engine_external_seconds=elapsed(engine_start);
        valid=imported && result.valid;
        if(!valid) std::cerr<<"ANISO_FAILURE "<<label<<" rank="<<worker.rank<<" "<<result.reason<<'\n';
        CHECK(result.stats.edge_evaluations+result.stats.edge_reused==6*(result.stats.initial_cells+result.stats.updated_cells));
        CHECK(result.stats.other_seconds>=-1e-12);
        EngineReport(result,worker,label);refined.swap(result.cells);
        MPI_Comm_free(&subset);
      }
      const bool failed=parent.maximum(!valid);const auto worker_seconds=elapsed(worker_start);
      const auto return_start=Clock::now();
      CHECK_FALSE(failed);
      to.assign(parent.size,{});
      for(const auto& row:refined) to[row.first%parent.size].push_back(row.second);
      const auto returned=parent.exchange(to);Owned output;
      for(auto row:returned) output.emplace(row.cell.id,row);
      const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);
      const auto return_seconds=elapsed(return_start);const auto audit_start=Clock::now();
      CHECK(count>6);CHECK(count<=128);CHECK(Audit(parent,output,fixture,count));
      CHECK(Bytes(input)==saved);
      const auto audit_seconds=elapsed(audit_start);const auto export_start=Clock::now();
      MeshRecords(label+(failed ? "_rejected" : ""),parent,output);
      const auto export_seconds=elapsed(export_start),total_seconds=elapsed(cycle_start);
      const auto other_seconds=total_seconds-migration_seconds-split_seconds-worker_seconds-return_seconds-audit_seconds-export_seconds;
      CHECK(other_seconds>=-1e-12);
      std::ostringstream cycle;
      cycle<<std::setprecision(17)<<"ANISO_CYCLE case="<<label<<" rank="<<parent.rank<<" ranks="<<parent.size
           <<" workers="<<m<<" parent_s="<<total_seconds<<" migration_s="<<migration_seconds
           <<" worker_comm_s="<<split_seconds<<" worker_s="<<worker_seconds<<" return_s="<<return_seconds
           <<" test_audit_s="<<audit_seconds<<" record_export_s="<<export_seconds<<" other_s="<<other_seconds
           <<" process_cpu_s="<<double(std::clock()-cpu_start)/CLOCKS_PER_SEC
           <<" inactive_worker_wait_s="<<(parent.rank>=m ? worker_seconds : 0)
           <<" source_audit_nested_s="<<source_audit_seconds<<" engine_external_nested_s="<<engine_external_seconds<<'\n';
      Log(parent,cycle.str());
    }
  }
}
TEST_CASE("Native MPI general coarsening supports repeated planar refine coarsen cycles", "[NativeDistributed3D][NativeCycles3D]") {
  World parent;
  using Clock=std::chrono::steady_clock;
  auto elapsed=[](Clock::time_point start){return std::chrono::duration<double>(Clock::now()-start).count();};
  for(const auto spec:std::array<AnisotropicCase,5>{{{"box",1,false,0},{"thin8",1e-8,false,0},{"shear4",1e-4,true,0},
      {"spatial",1,false,1},{"turning",1,false,2}}}) {
    const std::string name="cycle_"+std::string(spec.name);
    const auto fixture=Box(spec.height,spec.shear);const PlanarReference reference(fixture.facets);
    const auto initial=Partition(Rows(fixture.patch),parent);
    MeshRecords(name+"_initial",parent,initial);
    for(int m:std::set<int>{parent.size,std::max(1,parent.size-1),std::max(1,parent.size/2)}) {
      Owned accepted=initial;
      for(unsigned step=0;step<4;++step) {
        const bool coarsening=step%2;
        const std::string label=name+"_m"+std::to_string(m)+"_s"+std::to_string(step);
        const auto started=Clock::now();const auto cpu=std::clock();
        const auto saved=Bytes(accepted);
        const auto old_count=CPassiveComm::Allreduce(accepted.size(),CPassiveComm::Op::SUM,parent.comm);
        const auto preparation=Clock::now();
        auto original=Uniform(accepted,{});
        for(auto& donor:original) for(size_t i=0;i<4;++i) {
          const auto tensor=AnisotropicSensor(donor.cell.v[i].p,spec);
          donor.sensor[i]=coarsening ? Tensor{tensor.xx/4,tensor.xy/4,tensor.xz/4,tensor.yy/4,tensor.yz/4,tensor.zz/4} : tensor;
        }
        const double preparation_s=elapsed(preparation);
        AnisotropicSourceRecords("CYCLE_INPUT",label,parent,original);
        const auto migration=Clock::now();
        std::vector<std::vector<OwnedCell>> to(parent.size);
        std::vector<std::vector<DonorCell>> sensors(parent.size);
        for(const auto& row:accepted) to[row.first%m].push_back(row.second);
        for(const auto& row:original) sensors[row.cell.id%m].push_back(row);
        const auto migrated=parent.exchange(to);const auto sensor_rows=parent.exchange(sensors);
        const double migration_s=elapsed(migration);const auto comm_start=Clock::now();
        MPI_Comm subset=MPI_COMM_NULL;
        MPI_Comm_split(parent.comm,parent.rank<m ? 0 : MPI_UNDEFINED,parent.rank,&subset);
        const double comm_s=elapsed(comm_start);const auto worker_start=Clock::now();
        Owned adapted;bool valid=true;double source_s=0,engine_s=0,control_s=0;
        if(subset!=MPI_COMM_NULL) {
          World worker(subset);Owned owned;
          for(auto row:migrated) owned.emplace(row.cell.id,row);
          const auto worker_saved=Bytes(owned);const auto source_start=Clock::now();
          DonorImport donors(worker);std::string reason;
          bool imported=donors.Build(sensor_rows,reason);
          if(imported) {
            auto patch=donors.Import({{0,0,0,spec.shear ? 2. : 1.,1.,spec.height}});
            imported=patch.valid && patch.cells.size()==old_count;
            if(imported) {
              AnisotropicSourceRecords("CYCLE_SOURCE",label,worker,patch.cells);
              FrozenField field(patch.cells);
              for(auto p:std::array<Point,4>{{{.125,.25,.375*spec.height},{.5,.5,.5*spec.height},
                  {0,0,0},{.25,.375,.03125*spec.height}}}) {
                if(spec.shear) p.x+=p.y;
                const auto sample=field.Query(p);
                std::ostringstream line;line<<std::setprecision(17)<<"CYCLE_SAMPLE "<<label<<' '<<worker.rank
                    <<' '<<p.x<<' '<<p.y<<' '<<p.z;
                for(auto value:TensorValues(sample.sensor)) line<<' '<<value;
                for(auto id:sample.donor) line<<' '<<id;
                Log(worker,line.str()+"\n");
              }
            }
          }
          source_s=elapsed(source_start);CHECK(imported);
          if(std::string(spec.name)=="box" && step==1 && m==parent.size) {
            const auto control_start=Clock::now();
            EngineOptions control;control.maximum_rounds=1;
            const auto limited=Coarsen(worker,owned,sensor_rows,reference,{},control);
            CHECK_FALSE(limited.valid);
            CHECK(limited.reason.find("round limit")!=std::string::npos);
            CHECK(limited.reason.find("valid cached metric cells and pending collapse candidates")!=std::string::npos);
            CHECK_FALSE(worker.maximum(limited.stats.cached_invalid_cells!=0));
            CHECK(worker.maximum(limited.stats.remaining_edges!=0));
            CHECK(Bytes(owned)==worker_saved);
            control.maximum_rounds=512;control.memory_ceiling=1;
            CHECK_FALSE(Coarsen(worker,owned,sensor_rows,reference,{},control).valid);
            CHECK(Bytes(owned)==worker_saved);
            if(worker.size>1) {
              const auto mixed=worker.rank ? Coarsen(worker,owned,sensor_rows,reference) :
                                            Refine(worker,owned,sensor_rows,reference);
              CHECK_FALSE(mixed.valid);
              CHECK(mixed.reason.find("intent")!=std::string::npos);
              CHECK(Bytes(owned)==worker_saved);
            }
            control_s=elapsed(control_start);
          }
          EngineOptions options;options.timings=true;
          const auto engine_start=Clock::now();
          auto result=coarsening ? Coarsen(worker,owned,sensor_rows,reference,{},options) :
                                  Refine(worker,owned,sensor_rows,reference,{},options);
          engine_s=elapsed(engine_start);valid=imported && result.valid;
          CHECK(Bytes(owned)==worker_saved);
          CHECK(result.stats.edge_evaluations+result.stats.edge_reused==6*(result.stats.initial_cells+result.stats.updated_cells));
          CHECK(result.stats.other_seconds>=-1e-12);
          if(coarsening) { CHECK(result.splits.empty()); CHECK(result.collapses.size()+result.stats.coarsen_rolled_back_collapses+result.stats.moved+result.stats.reconnected+result.stats.inserted==result.stats.accepted); }
          else { CHECK(result.collapses.empty()); CHECK(result.splits.size()+result.stats.moved+result.stats.reconnected+result.stats.inserted==result.stats.accepted); }
          if(!valid) std::cerr<<"CYCLE_FAILURE "<<label<<" rank="<<worker.rank<<" "<<result.reason<<'\n';
          EngineReport(result,worker,label);adapted.swap(result.cells);
          MPI_Comm_free(&subset);
        }
        const bool failed=parent.maximum(!valid);const double worker_s=elapsed(worker_start);
        CHECK_FALSE(failed);const auto returning=Clock::now();
        to.assign(parent.size,{});
        for(const auto& row:adapted) to[row.first%parent.size].push_back(row.second);
        const auto returned=parent.exchange(to);Owned output;
        for(auto row:returned) output.emplace(row.cell.id,row);
        const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);
        const double return_s=elapsed(returning);const auto audit_start=Clock::now();
        CHECK(count>=6);CHECK(count<=128);CHECK(Audit(parent,output,fixture,count));
        CHECK(Bytes(accepted)==saved);
        if(!failed) { CHECK((coarsening ? count<old_count : count>old_count)); }
        const double audit_s=elapsed(audit_start);const auto exporting=Clock::now();
        MeshRecords(label+(failed ? "_rejected" : ""),parent,output);
        const double export_s=elapsed(exporting);const double total_s=elapsed(started);
        // Input observation output is intentionally charged to other, not to metric preparation.
        const double other_s=total_s-preparation_s-migration_s-comm_s-worker_s-return_s-audit_s-export_s;
        CHECK(other_s>=-1e-12);
        std::ostringstream profile;
        profile<<std::setprecision(17)<<"CYCLE_EVENT case="<<label<<" rank="<<parent.rank<<" ranks="<<parent.size
            <<" workers="<<m<<" step="<<step<<" coarsening="<<coarsening<<" valid="<<!failed
            <<" initial="<<old_count<<" final="<<count<<" parent_s="<<total_s<<" preparation_s="<<preparation_s
            <<" migration_s="<<migration_s<<" worker_comm_s="<<comm_s<<" worker_s="<<worker_s
            <<" return_s="<<return_s<<" test_audit_s="<<audit_s<<" record_export_s="<<export_s<<" other_s="<<other_s
            <<" process_cpu_s="<<double(std::clock()-cpu)/CLOCKS_PER_SEC
            <<" inactive_worker_wait_s="<<(parent.rank>=m ? worker_s : 0)
            <<" source_audit_nested_s="<<source_s<<" rejection_control_nested_s="<<control_s<<" engine_external_nested_s="<<engine_s<<'\n';
        Log(parent,profile.str());
        if(failed) break;
        accepted.swap(output);
      }
    }
  }
}
namespace {
Fixture RepairGeometry(const std::string& kind,double height,bool shear,double placement=.0625) {
  auto transform=[&](Point p){return Point{p.x+(shear ? p.y : 0),p.y,p.z*height};};
  Fixture result;
  if(kind=="face") {
    const Node a{0,transform({0,0,0})},b{1,transform({1,0,0})},c{2,transform({.5,std::sqrt(3.)/2,0})},
        d{3,transform({.5,std::sqrt(3.)/6,.25})},e{4,transform({.5,std::sqrt(3.)/6,-.25})};
    result.patch.cells={{0,{a,b,c,d}},{1,{a,c,b,e}}};
  } else if(kind=="edge4") {
    const std::array<Node,6> n{{{0,transform({.5,0,0})},{1,transform({0,1,0})},
        {2,transform({-.5,0,0})},{3,transform({0,-1,0})},{4,transform({0,0,-.75})},{5,transform({0,0,.75})}}};
    for(size_t i=0;i<4;++i) {
      Cell c{Id(i),{n[4],n[5],n[i],n[(i+1)%4]}};
      if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0) std::swap(c.v[0],c.v[1]);
      result.patch.cells.push_back(c);
    }
  } else {
    result.patch.cells={{0,{{{0,transform({0,0,0})},{1,transform({1,0,0})},
        {2,transform({0,1,0})},{3,transform({0,0,1})}}}}};
  }
  std::map<Id,Node> nodes;
  for(auto cell:result.patch.cells) for(auto node:cell.v) nodes.emplace(node.id,node);
  for(auto face:Incidence(result.patch.cells).Boundary()) {
    const Id id=result.facets.size();result.facets.push_back({id,int(id)+10,{nodes.at(face[0]),nodes.at(face[1]),nodes.at(face[2])}});
    result.patch.surface.push_back({face,id});
  }
  if(kind=="wall" || kind=="interior") {
    const auto cell=result.patch.cells.front();const PlanarReference reference(result.facets);
    const Node p{4,transform({placement,placement,kind=="wall" ? 0 : placement})};
    if(kind=="wall") result.patch.cells={{0,{p,cell.v[1],cell.v[2],cell.v[3]}},
        {1,{cell.v[0],p,cell.v[2],cell.v[3]}},{2,{cell.v[0],cell.v[1],p,cell.v[3]}}};
    else {std::string reason;REQUIRE(Cone({cell},p,0,result.patch.cells,reason));}
    result.patch.surface.clear();nodes.emplace(p.id,p);
    for(auto face:Incidence(result.patch.cells).Boundary()) {
      bool found=false;
      for(auto facet:result.facets) {
        bool contains=true;
        for(auto id:face) contains=contains && reference.Contains(facet.id,nodes.at(id).p);
        if(contains){result.patch.surface.push_back({face,facet.id});found=true;break;}
      }
      REQUIRE(found);
    }
  }
  return result;
}
void RepairFacetRecords(const std::string& label,World& world,const std::vector<Facet>& facets) {
  std::ostringstream text;
  if(!world.rank) for(auto f:facets) {
    text<<std::setprecision(17)<<"REPAIR_FACET "<<label<<' '<<f.id<<' '<<f.marker;
    for(auto n:f.v) text<<' '<<n.id<<' '<<n.p.x<<' '<<n.p.y<<' '<<n.p.z;
    text<<'\n';
  }
  Log(world,text.str());
}
}
TEST_CASE("Native MPI planar movement and face edge reconnection publish full private metric repairs", "[NativeDistributed3D][NativeRepair3D]") {
  World parent;using Clock=std::chrono::steady_clock;
  auto elapsed=[](Clock::time_point start){return std::chrono::duration<double>(Clock::now()-start).count();};
  for(const auto spec:std::array<AnisotropicCase,3>{{{"unit",1,false,0},{"thin8",1e-8,false,0},{"shear4",1e-4,true,0}}})
    for(const std::string kind:{"face","edge4","wall","interior"}) {
      const std::string name="repair_"+kind+"_"+spec.name;
      const auto fixture=RepairGeometry(kind,spec.height,spec.shear);const PlanarReference geometry(fixture.facets);
      const auto input=Partition(Rows(fixture.patch),parent);const auto initial_bytes=Bytes(input);
      MeshRecords(name+"_initial",parent,input);RepairFacetRecords(name,parent,fixture.facets);
      for(int m:std::set<int>{parent.size,std::max(1,parent.size-1),std::max(1,parent.size/2)}) {
        Owned accepted=input;
        for(unsigned step=0;step<(kind=="face" ? 3u : 1u);++step) {
          const std::string label=name+"_m"+std::to_string(m)+"_s"+std::to_string(step);
          const auto started=Clock::now();const auto preparation=Clock::now();const auto saved=Bytes(accepted);
          Tensor tensor{1,spec.shear ? -1. : 0,0,spec.shear ? 2. : 1.,0,(step==1 ? 9. : 1.)/(spec.height*spec.height)};
          auto original=Uniform(accepted,tensor);
          for(auto& donor:original) for(size_t i=0;i<4;++i) {
            const double scale=1+.1*donor.cell.v[i].p.x;
            auto& t=donor.sensor[i];t={t.xx*scale,t.xy*scale,t.xz*scale,t.yy*scale,t.yz*scale,t.zz*scale};
          }
          const auto preparation_s=elapsed(preparation);
          AnisotropicSourceRecords("REPAIR_INPUT",label,parent,original);
          const auto migration=Clock::now();std::vector<std::vector<OwnedCell>> to(parent.size);
          std::vector<std::vector<DonorCell>> sensor_to(parent.size);
          for(const auto& row:accepted) to[row.first%m].push_back(row.second);
          for(auto row:original) sensor_to[row.cell.id%m].push_back(row);
          const auto migrated=parent.exchange(to);const auto source_rows=parent.exchange(sensor_to);
          const auto migration_s=elapsed(migration);const auto comm_start=Clock::now();
          MPI_Comm subset=MPI_COMM_NULL;MPI_Comm_split(parent.comm,parent.rank<m ? 0 : MPI_UNDEFINED,parent.rank,&subset);
          const auto comm_s=elapsed(comm_start);const auto worker_start=Clock::now();
          Owned repaired;bool valid=true;
          if(subset!=MPI_COMM_NULL) {
            World worker(subset);Owned owned;for(auto row:migrated) owned.emplace(row.cell.id,row);
            const auto owned_bytes=Bytes(owned);Directory directory(worker);std::string reason;
            valid=directory.Build(owned,reason);const auto directory_before=directory.Snapshot();
            const Feature feature=(kind=="face" && step!=1) ? Feature{{0,1,2},3} :
                (kind=="face" ? Feature{{3,4,0},2} : (kind=="edge4" ? Feature{{4,5,0},2} : Feature{{4,0,0},1}));
            const auto dependencies=directory.Lookup(Query(worker,feature,true));
            const auto imported=directory.Fetch(dependencies,owned);
            valid=valid && dependencies.valid && imported.valid;
            CHECK(valid);CHECK(imported.cells.size()==(kind=="face" ? (step==1 ? 3u : 2u) : fixture.patch.cells.size()));
            const bool chosen=directory.Reserve(imported,true,reason);
            CHECK(chosen==(!worker.rank));
            DonorImport donors(worker,true);const bool source_built=donors.Build(source_rows,reason);valid=valid && source_built;
            std::vector<DonorRegion> regions;
            for(auto row:imported.cells) {
              const auto p=row.cell.v[0].p;DonorRegion box{p.x,p.y,p.z,p.x,p.y,p.z};
              for(auto node:row.cell.v) {
                const double xyz[3]{node.p.x,node.p.y,node.p.z};
                for(size_t a=0;a<3;++a){box[a]=std::min(box[a],xyz[a]);box[a+3]=std::max(box[a+3],xyz[a]);}
              }
              regions.push_back(box);
            }
            auto sensors=donors.Import(regions);valid=valid && sensors.valid;
            AnisotropicSourceRecords("REPAIR_SOURCE",label,worker,sensors.cells);
            FrozenField field(sensors.cells,{},true);
            const auto& sample_cell=std::min_element(sensors.cells.begin(),sensors.cells.end(),
                [](const DonorCell& a,const DonorCell& b){return a.Key()<b.Key();})->cell;
            std::array<Point,4> queries;
            queries[0]=sample_cell.v[0].p;
            for(size_t sample=1;sample<4;++sample) {
              long double x=0,y=0,z=0;
              for(size_t i=0;i<4;++i) {
                const long double w=sample==1 ? .25L : (sample==2 ? (i==0 ? .625L : .125L) : (i==1 ? .625L : .125L));
                x+=w*sample_cell.v[i].p.x;y+=w*sample_cell.v[i].p.y;z+=w*sample_cell.v[i].p.z;
              }
              queries[sample]={double(x),double(y),double(z)};
            }
            for(auto point:queries) {
              const auto value=field.Query(point);std::ostringstream line;
              line<<std::setprecision(17)<<"REPAIR_SAMPLE "<<label<<' '<<worker.rank<<' '<<point.x<<' '<<point.y<<' '<<point.z;
              for(auto x:TensorValues(value.sensor)) line<<' '<<x;
              for(auto id:value.donor) line<<' '<<id;
              Log(worker,line.str()+"\n");
            }
            const auto reconstruction_start=Clock::now();Patch old=PatchOf(imported.cells),fresh;
            bool approved=!chosen;
            if(chosen) {
              if(kind=="wall" || kind=="interior") {
                Point p{.25,.25,kind=="wall" ? 0 : .25*spec.height};if(spec.shear)p.x+=p.y;
                approved=MoveVertex(old,4,{5,p},1000,geometry,fresh,reason);
              } else approved=Reconnect(old,kind=="edge4" || step==1 ? 0 : 3,1000*(step+1),geometry,fresh,reason);
            }
            const auto reconstruction_s=elapsed(reconstruction_start);const auto metric_start=Clock::now();
            PatchMetric measured;MetricStats metric_stats;
            if(chosen && approved) approved=ValidateMetricChange(old.cells,fresh.cells,MetricChange::Repair,field,measured,reason,&metric_stats);
            const auto metric_s=elapsed(metric_start);
            if(!approved) std::cerr<<"REPAIR_FAILURE "<<label<<" rank="<<worker.rank<<" "<<reason<<'\n';
            valid=valid && approved;CHECK(valid);
            std::vector<std::vector<Dependency>> removes(worker.size);std::vector<std::vector<OwnedCell>> adds(worker.size);
            if(chosen && approved) {
              for(auto row:dependencies.cells) removes[row.owner].push_back(row);
              for(auto row:Rows(fresh)) adds[row.cell.id%worker.size].push_back(row);
            }
            const auto removed=worker.exchange(removes);const auto added=worker.exchange(adds);
            // Actual shared-coordinate motion/reconnection staged with new IDs; rank veto must preserve both stores.
            CHECK_FALSE(directory.Publish(owned,removed,added,worker.rank!=0,reason));
            CHECK(Bytes(owned)==owned_bytes);CHECK(directory.Snapshot()==directory_before);
            CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,1));
            CHECK(Bytes(owned)==owned_bytes);CHECK(directory.Snapshot()==directory_before);
            valid=directory.Publish(owned,removed,added,valid,reason) && valid;
            repaired.swap(owned);const auto& f=field.Statistics();const auto& d=directory.Statistics();
            std::ostringstream report;report<<std::setprecision(17)<<"REPAIR_WORK case="<<label<<" rank="<<worker.rank
                <<" ranks="<<worker.size<<" valid="<<valid<<" chosen="<<chosen<<" metric_cells="<<metric_stats.cells
                <<" metric_edges="<<metric_stats.edge_evaluations<<" reconstruction_s="<<reconstruction_s<<" metric_s="<<metric_s
                <<" field_requests="<<f.requests<<" field_hits="<<f.hits<<" field_edges="<<f.edge_requests
                <<" search_nested_s="<<f.search_seconds<<" interpolation_nested_s="<<f.interpolation_seconds
                <<" trace_nested_s="<<f.edge_trace_seconds<<" directory_work_bytes="<<d.maximum_work_bytes
                <<" donor_work_bytes="<<donors.Statistics().maximum_work_bytes<<" bytes_sent="<<worker.bytes_sent
                <<" collective_calls="<<worker.collective_calls<<'\n';
            Log(worker,report.str());MPI_Comm_free(&subset);
          }
          const bool failed=parent.maximum(!valid);const auto worker_s=elapsed(worker_start);CHECK_FALSE(failed);
          const auto returning=Clock::now();to.assign(parent.size,{});
          for(const auto& row:repaired) to[row.first%parent.size].push_back(row.second);
          const auto returned=parent.exchange(to);Owned output;for(auto row:returned) output.emplace(row.cell.id,row);
          const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);
          const auto return_s=elapsed(returning);const auto audit_start=Clock::now();
          CHECK(count==(kind=="face" ? (step==1 ? 2u : 3u) : fixture.patch.cells.size()));
          CHECK(Audit(parent,output,fixture,count));CHECK(Bytes(accepted)==saved);CHECK(Bytes(input)==initial_bytes);
          const auto audit_s=elapsed(audit_start);const auto exporting=Clock::now();
          MeshRecords(label+(failed ? "_rejected" : ""),parent,output);
          const auto export_s=elapsed(exporting),total_s=elapsed(started);
          const auto other_s=total_s-preparation_s-migration_s-comm_s-worker_s-return_s-audit_s-export_s;
          CHECK(other_s>=-1e-12);
          std::ostringstream report;report<<std::setprecision(17)<<"REPAIR_EVENT case="<<label<<" rank="<<parent.rank
              <<" ranks="<<parent.size<<" workers="<<m<<" step="<<step<<" valid="<<!failed<<" final="<<count
              <<" parent_s="<<total_s<<" preparation_s="<<preparation_s<<" migration_s="<<migration_s
              <<" worker_comm_s="<<comm_s<<" worker_s="<<worker_s<<" return_s="<<return_s
              <<" test_audit_s="<<audit_s<<" record_export_s="<<export_s<<" other_s="<<other_s
              <<" inactive_worker_wait_s="<<(parent.rank>=m ? worker_s : 0)<<'\n';
          Log(parent,report.str());if(failed)break;accepted.swap(output);
        }
      }
    }
}

TEST_CASE("Native MPI admissible coarsening prunes exact immutable-facet counterexamples", "[NativeDistributed3D][NativeCollapseFilter3D]") {
  World world;
  const auto fixture=RepairGeometry("corner",1.,false);
  const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  const auto original=Uniform(input,Tensor{.25,0,0,.25,0,.25});
  EngineOptions options;options.timings=true;
  const auto result=Coarsen(world,input,original,reference,{},options);
  CHECK(result.valid);CHECK(result.reason.empty());CHECK(result.stats.rounds==0);
  CHECK(result.stats.final_owned_cells==result.cells.size());
  CHECK(result.stats.cached_measured_cells==result.cells.size());
  CHECK(result.stats.cached_invalid_cells==0);
  CHECK(result.stats.remaining_edges==0);CHECK(result.stats.remaining_repairs==0);
  CHECK(world.sum(int(result.stats.collapse_filtered))>0);
  CHECK(world.sum(int(result.stats.collapse_geometry_checks))>0);
  CHECK(result.stats.collapse_geometry_seconds>=0);CHECK(result.stats.other_seconds>=-1e-12);
  CHECK(Bytes(result.cells)==Bytes(WithUniformSensor(input,{.25,0,0,.25,0,.25})));CHECK(Bytes(input)==saved);
  EngineReport(result,world,"immutable_corner_filter");
  options.maximum_rounds=1;
  const auto rejected=Coarsen(world,input,Uniform(input,Tensor{16,0,0,16,0,16}),reference,{},options);
  CHECK_FALSE(rejected.valid);CHECK(rejected.stats.collapse_filtered==0);CHECK(rejected.stats.collapse_orientation_filtered==0);
  CHECK(rejected.stats.final_owned_cells==rejected.cells.size());
  CHECK(rejected.stats.cached_measured_cells==rejected.cells.size());
  CHECK(world.sum(int(rejected.stats.cached_invalid_cells))>0);
  INFO(rejected.reason);
  CHECK(rejected.stats.stop==EngineStop::RoundBudget);
  CHECK(rejected.reason.find("round limit")!=std::string::npos);
  CHECK(Bytes(input)==saved);CHECK(rejected.stats.other_seconds>=-1e-12);
}

TEST_CASE("Native automatic MPI repair selects complete failing features and restores frozen gates", "[NativeDistributed3D][NativeAutomaticRepair3D]") {
  World parent;using Clock=std::chrono::steady_clock;
  auto elapsed=[](Clock::time_point start){return std::chrono::duration<double>(Clock::now()-start).count();};
  {
    const auto fixture=Box();const PlanarReference geometry(fixture.facets);
    const auto input=Partition(Rows(fixture.patch),parent);const auto saved=Bytes(input);
    const auto original=Uniform(input,{4,0,0,4,0,4});
    for(bool detail : {false,true}) {
      EngineOptions options;options.timings=detail;options.maximum_rounds=1;
      options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
      uint64_t calls=0;
      const auto bytes_before=parent.bytes_sent;const auto calls_before=parent.collective_calls;
      const double seconds_before=parent.collective_seconds;
      auto rejected=Refine(parent,input,original,geometry,[&](Point,Tensor)->Tensor {
        ++calls;throw std::runtime_error("Injected query composition failure.");
      },options);
      CHECK(rejected.stats.tracked_bytes==parent.bytes_sent-bytes_before);
      CHECK(rejected.stats.tracked_calls==parent.collective_calls-calls_before);
      CHECK(rejected.stats.tracked_collective_seconds==Approx(parent.collective_seconds-seconds_before).margin(1e-12));
      CHECK(rejected.stats.directory_elections>0);
      CHECK(rejected.stats.directory_election_seconds>=0);
      CHECK_FALSE(rejected.valid);CHECK(rejected.stats.accepted==0);CHECK(Bytes(input)==saved);
      CHECK(rejected.reason.find("Injected query composition failure.")!=std::string::npos);
      CHECK(Bytes(rejected.cells)==Bytes(WithUniformSensor(input,{4,0,0,4,0,4})));
      CHECK(rejected.stats.field_requests==rejected.stats.field_hits+rejected.stats.field_misses+rejected.stats.field_invalid_queries);
      CHECK(rejected.stats.field_invalid_queries==0);CHECK(rejected.stats.field_failures==calls);
      CHECK(rejected.stats.field_compositions==calls);CHECK(rejected.stats.field_requests+rejected.stats.field_edges==calls);
      CHECK(parent.sum(int(calls))>0);
    }
  }
  for(const std::string mode:{"auto","restoreR","restoreC"})
  for(const auto spec:std::array<AnisotropicCase,3>{{{"unit",1,false,0},{"thin8",1e-8,false,0},{"shear4",1e-4,true,0}}})
    for(const std::string kind:{"face","edge4","wall","interior"}) {
      auto fixture=RepairGeometry(kind,spec.height,spec.shear,.03125);const PlanarReference geometry(fixture.facets);
      if(kind=="face") { Patch next;std::string reason;REQUIRE(Reconnect(fixture.patch,3,0,geometry,next,reason));fixture.patch=std::move(next); }
      const std::string name=mode+"_"+kind+"_"+spec.name;
      const auto input=Partition(Rows(fixture.patch),parent);const auto saved=Bytes(input);
      Tensor tensor{1,spec.shear ? -1. : 0,0,spec.shear ? 2. : 1.,0,(kind=="face" ? 16. : 1.)/(spec.height*spec.height)};
      if(kind=="edge4") tensor={.04,spec.shear ? -.04 : 0,0,spec.shear ? .08 : .04,0,1./(spec.height*spec.height)};
      auto original=Uniform(input,tensor);
      for(auto& donor:original) for(size_t i=0;i<4;++i) {
        const double scale=1+.1*donor.cell.v[i].p.x;auto& t=donor.sensor[i];
        t={t.xx*scale,t.xy*scale,t.xz*scale,t.yy*scale,t.yz*scale,t.zz*scale};
      }
      MeshRecords(name+"_initial",parent,input);RepairFacetRecords(name,parent,fixture.facets);
      AnisotropicSourceRecords("AUTO_INPUT",name,parent,original);
      for(int m:std::set<int>{parent.size,std::max(1,parent.size-1),std::max(1,parent.size/2)}) {
        const std::string label=name+"_m"+std::to_string(m);const auto started=Clock::now();
        const auto migration=Clock::now();std::vector<std::vector<OwnedCell>> to(parent.size);
        std::vector<std::vector<DonorCell>> sensor_to(parent.size);
        for(const auto& row:input) to[row.first%m].push_back(row.second);
        for(auto row:original) sensor_to[row.cell.id%m].push_back(row);
        const auto migrated=parent.exchange(to);const auto source_rows=parent.exchange(sensor_to);const auto migration_s=elapsed(migration);
        const auto comm_start=Clock::now();MPI_Comm subset=MPI_COMM_NULL;
        MPI_Comm_split(parent.comm,parent.rank<m ? 0 : MPI_UNDEFINED,parent.rank,&subset);const auto comm_s=elapsed(comm_start);
        const auto worker_start=Clock::now();Owned repaired;bool valid=true;
        if(subset!=MPI_COMM_NULL) {
          World worker(subset);Owned owned;for(auto row:migrated)owned.emplace(row.cell.id,row);
          const auto owned_saved=Bytes(owned);EngineOptions options;options.timings=true;
          const auto bytes_before=worker.bytes_sent;const auto calls_before=worker.collective_calls;
          const double seconds_before=worker.collective_seconds;
          auto result=mode=="restoreR" ? Refine(worker,owned,source_rows,geometry,{},options) :
              (mode=="restoreC" ? Coarsen(worker,owned,source_rows,geometry,{},options) : Repair(worker,owned,source_rows,geometry,{},options));
          CHECK(result.stats.tracked_bytes==worker.bytes_sent-bytes_before);
          CHECK(result.stats.tracked_calls==worker.collective_calls-calls_before);
          CHECK(result.stats.tracked_collective_seconds==Approx(worker.collective_seconds-seconds_before).margin(1e-12));
          CHECK(result.stats.directory_elections>0);
          if(!result.valid)std::cerr<<"AUTO_REPAIR_FAILURE "<<label<<" rank="<<worker.rank<<" "<<result.reason<<'\n';
          valid=result.valid;CHECK(valid);CHECK(Bytes(owned)==owned_saved);CHECK(result.stats.other_seconds>=-1e-12);
          CHECK(worker.sum(int(result.stats.accepted))>0);
          CHECK(result.stats.recovery_starts<=1);
          CHECK((!options.maximum_rounds || result.stats.rounds<=options.maximum_rounds));
          CHECK(result.stats.global_work<=result.stats.work_limit);
          CHECK(result.stats.initial_cells==owned.size());
          CHECK(result.stats.field_requests==result.stats.field_hits+result.stats.field_misses+result.stats.field_invalid_queries);
          CHECK(result.stats.field_failures<=result.stats.field_requests+result.stats.field_edges);
          CHECK(result.stats.field_evictions<=result.stats.field_misses);
          CHECK(result.splits.size()+result.collapses.size()+result.stats.coarsen_rolled_back_collapses+result.stats.moved+result.stats.reconnected+result.stats.inserted==result.stats.accepted);
          CHECK(result.stats.quality_stages>=1);
          if(mode=="auto") {
            if(kind!="face") {
              CHECK(worker.sum(int(kind=="wall" || kind=="interior" ? result.stats.moved : result.stats.reconnected))>0);
              CHECK(result.splits.empty());
            }
            CHECK(worker.maximum(Bytes(result.cells)!=owned_saved));CHECK(result.collapses.empty());
          }
          if(mode=="restoreR" && kind!="face") CHECK(worker.sum(int(result.stats.quality_progress))>0);
          if(mode=="restoreC") CHECK(result.splits.empty());
          if(mode=="restoreC" && kind=="edge4") {
            CHECK(worker.sum(int(result.stats.quality_progress))>0);
            CHECK(result.stats.quality_stages>=2);CHECK(result.stats.construction_returns>0);
          }
          EngineReport(result,worker,label);repaired.swap(result.cells);
          if(mode=="auto" && spec.name==std::string("unit") && kind=="interior") {
            options.maximum_rounds=1;const auto rejected=Repair(worker,owned,source_rows,geometry,{},options);
            CHECK_FALSE(rejected.valid);CHECK(Bytes(owned)==owned_saved);
            options.maximum_rounds=512;options.memory_ceiling=1;
            const auto budget=Repair(worker,owned,source_rows,geometry,{},options);
            CHECK_FALSE(budget.valid);CHECK(Bytes(owned)==owned_saved);
          }
          MPI_Comm_free(&subset);
        }
        const bool failed=parent.maximum(!valid);const auto worker_s=elapsed(worker_start);CHECK_FALSE(failed);
        const auto returning=Clock::now();to.assign(parent.size,{});
        for(const auto& row:repaired)to[row.first%parent.size].push_back(row.second);
        const auto returned=parent.exchange(to);Owned output;for(auto row:returned)output.emplace(row.cell.id,row);
        const auto return_s=elapsed(returning);const auto audit_start=Clock::now();
        const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);
        CHECK(count>0);CHECK(count<=12);
        if(mode=="auto" && kind!="face") CHECK(count==fixture.patch.cells.size());
        if(mode=="restoreC") CHECK(count<=fixture.patch.cells.size());
        CHECK(Audit(parent,output,fixture,count));CHECK(Bytes(input)==saved);
        const auto all=Gather(parent,output),all_source=Gather(parent,input);
        if(!parent.rank) {
          const auto cells=PatchOf(all).cells;std::vector<DonorCell> donors;
          // Full-source final check on the small manufactured fixture; the engine itself imports distributed donors.
          for(auto row:all_source) { DonorCell d;d.cell=row.cell;d.sensor.fill(tensor);
            for(size_t i=0;i<4;++i) {auto& t=d.sensor[i];const auto scale=1+.1*d.cell.v[i].p.x;
              t={t.xx*scale,t.xy*scale,t.xz*scale,t.yy*scale,t.yz*scale,t.zz*scale};} donors.push_back(d); }
          FrozenField field(donors);PatchMetric metric;std::string reason;
          CHECK(ValidateMetricPatch(cells,field,metric,reason));
          std::cout<<std::setprecision(17)<<"AUTO_FINAL "<<label<<' '<<metric.minimum_mean_ratio<<' '
              <<metric.minimum_scaled_jacobian<<' '<<double(metric.maximum_length)<<'\n';
        }
        const auto audit_s=elapsed(audit_start);const auto exporting=Clock::now();
        MeshRecords(label+(failed ? "_rejected" : ""),parent,output);const auto export_s=elapsed(exporting),total_s=elapsed(started);
        const auto other_s=total_s-migration_s-comm_s-worker_s-return_s-audit_s-export_s;CHECK(other_s>=-1e-12);
        std::ostringstream report;report<<std::setprecision(17)<<"AUTO_EVENT case="<<label<<" rank="<<parent.rank
            <<" ranks="<<parent.size<<" workers="<<m<<" mode="<<mode<<" valid="<<!failed<<" final="<<count<<" parent_s="<<total_s
            <<" migration_s="<<migration_s<<" worker_comm_s="<<comm_s<<" worker_s="<<worker_s<<" return_s="<<return_s
            <<" test_audit_s="<<audit_s<<" record_export_s="<<export_s<<" other_s="<<other_s
            <<" inactive_worker_wait_s="<<(parent.rank>=m ? worker_s : 0)<<'\n';Log(parent,report.str());
      }
    }
}
#endif

TEST_CASE("Native exact cell donor filters retain closed boundaries and reject invalid requests atomically", "[NativeDistributed3D]") {
  World world; const auto geometry=Box();
  DonorImport service(world); std::string reason;
  std::vector<DonorCell> original;
  for(auto cell:geometry.patch.cells) if(cell.id%world.size==unsigned(world.rank)) {
    DonorCell donor; donor.cell=cell;
    for(size_t i=0;i<4;++i) donor.sensor[i].xx=2+cell.v[i].p.x;
    original.push_back(donor);
  }
  REQUIRE(service.Build(original,reason));
  const auto resident=service.ResidentBytes();
  auto cell=geometry.patch.cells.front();
  auto imported=service.ImportCells(world.rank ? std::vector<Cell>{} : std::vector<Cell>{cell});
  REQUIRE(imported.valid);
  if (!world.rank) {
    FrozenField field(imported.cells);
    for(auto node:cell.v) CHECK(field.Query(node.p).sensor.xx==Approx(2+node.p.x));
    CHECK(field.SensorEdgeLength(cell.v[0].p,cell.v[1].p)>0);
  } else CHECK(imported.cells.empty());
  std::swap(cell.v[0],cell.v[1]);
  CHECK_FALSE(service.ImportCells(world.rank ? std::vector<Cell>{} : std::vector<Cell>{cell}).valid);
  CHECK(service.ResidentBytes()==resident);
  std::swap(cell.v[0],cell.v[1]); cell.v[0].p.x=NAN;
  CHECK_FALSE(service.ImportCells(world.rank ? std::vector<Cell>{} : std::vector<Cell>{cell}).valid);
  CHECK_FALSE(service.ImportCells(std::vector<Cell>(MaximumCavityCells+1)).valid);
  CHECK_FALSE(service.ImportCells(geometry.patch.cells,1).valid);
  CHECK(service.ImportCells({}).valid);
  CHECK(service.ImportCells({geometry.patch.cells.front()}).valid);
  CHECK(service.Statistics().filter_candidates==0);
}

TEST_CASE("Native metric-query imports cover centroids and complete edges with atomic invalid rejection", "[NativeDistributed3D]") {
  World world; const auto geometry=Box(); std::vector<DonorCell> original;
  for(auto cell:geometry.patch.cells) if(cell.id%world.size==unsigned(world.rank)) {
    DonorCell donor;donor.cell=cell;
    for(size_t i=0;i<4;++i) donor.sensor[i].xx=2+cell.v[i].p.x;
    original.push_back(donor);
  }
  DonorImport service(world);std::string reason;REQUIRE(service.Build(original,reason));
  const auto resident=service.ResidentBytes();auto cell=geometry.patch.cells.front();
  auto imported=service.ImportMetricCells(world.rank ? std::vector<Cell>{} : std::vector<Cell>{cell});
  REQUIRE(imported.valid);
  if(!world.rank) {
    FrozenField field(imported.cells);
    CHECK(field.Query(Centroid(cell)).sensor.xx==Approx(2+Centroid(cell).x));
    for(size_t i=0;i<4;++i) for(size_t j=i+1;j<4;++j) CHECK(field.SensorEdgeLength(cell.v[i].p,cell.v[j].p)>0);
  } else CHECK(imported.cells.empty());
  std::swap(cell.v[0],cell.v[1]);
  CHECK_FALSE(service.ImportMetricCells(world.rank ? std::vector<Cell>{} : std::vector<Cell>{cell}).valid);
  std::swap(cell.v[0],cell.v[1]);cell.v[0].p.x=NAN;
  CHECK_FALSE(service.ImportMetricCells(world.rank ? std::vector<Cell>{} : std::vector<Cell>{cell}).valid);
  CHECK_FALSE(service.ImportMetricCells(std::vector<Cell>(MaximumCavityCells+1)).valid);
  CHECK_FALSE(service.ImportMetricCells(geometry.patch.cells,1).valid);
  CHECK(service.ImportMetricCells({}).valid);
  CHECK(service.ResidentBytes()==resident);
  CHECK(service.Statistics().filter_candidates==0);
}

#ifdef HAVE_MPI
TEST_CASE("Native MPI coarsens across original planar seams and repeats refinement on worker subsets", "[NativeDistributed3D][NativeCoplanar3D]") {
  World parent;
  using Clock=std::chrono::steady_clock;
  auto elapsed=[](Clock::time_point start){return std::chrono::duration<double>(Clock::now()-start).count();};
  const std::array<Node,6> v{{{0,{0,0,0}}, {1,{1,0,0}}, {2,{1,1,0}}, {3,{0,1,0}},
                             {4,{.5,.5,0}}, {5,{.5,.5,1}}}};
  Fixture fixture;
  for (size_t i=0;i<4;++i) {
    Cell cell{Id(i),{v[i],v[(i+1)%4],v[4],v[5]}};
    if (Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0) std::swap(cell.v[0],cell.v[1]);
    fixture.patch.cells.push_back(cell);
  }
  for (auto face:Incidence(fixture.patch.cells).Boundary()) {
    const Id id=fixture.facets.size();
    const bool base=v[face[0]].p.z==0 && v[face[1]].p.z==0 && v[face[2]].p.z==0;
    fixture.facets.push_back({id,base ? 70 : int(id)+10,{v[face[0]],v[face[1]],v[face[2]]}});
    fixture.patch.surface.push_back({face,id});
  }
  const auto geometry_start=Clock::now();
  const PlanarReference reference(fixture.facets);
  const double geometry_s=elapsed(geometry_start);
  std::ostringstream geometry_profile;
  geometry_profile<<std::setprecision(17)<<"COPLANAR_GEOMETRY rank="<<parent.rank<<" ranks="<<parent.size
      <<" facets="<<fixture.facets.size()<<" build_s="<<geometry_s<<'\n';
  Log(parent,geometry_profile.str());
  const auto input=Partition(Rows(fixture.patch),parent);const auto input_bytes=Bytes(input);
  MeshRecords("coplanar_initial",parent,input);
  for (int m:std::set<int>{parent.size,std::max(1,parent.size-1),std::max(1,parent.size/2)}) {
    Owned accepted=input;
    for (unsigned step=0;step<3;++step) {
      const bool coarsen=step!=1;
      const std::string label="coplanar_m"+std::to_string(m)+"_s"+std::to_string(step);
      const auto started=Clock::now();const auto saved=Bytes(accepted);
      const auto preparation=Clock::now();
      auto sensors=Uniform(input,coarsen ? Tensor{.25,0,0,.25,0,.25} : Tensor{4,0,0,4,0,4});
      const double preparation_s=elapsed(preparation);
      AnisotropicSourceRecords("COPLANAR_SOURCE",label,parent,sensors);
      const auto migration=Clock::now();
      std::vector<std::vector<OwnedCell>> to(parent.size);
      std::vector<std::vector<DonorCell>> sources(parent.size);
      for (const auto& row:accepted) to[row.first%m].push_back(row.second);
      for (auto donor:sensors) sources[donor.cell.id%m].push_back(donor);
      const auto migrated=parent.exchange(to);const auto original=parent.exchange(sources);
      const double migration_s=elapsed(migration);const auto communicator=Clock::now();
      MPI_Comm subset=MPI_COMM_NULL;
      MPI_Comm_split(parent.comm,parent.rank<m ? 0 : MPI_UNDEFINED,parent.rank,&subset);
      const double communicator_s=elapsed(communicator);const auto worker_start=Clock::now();
      Owned updated;bool valid=true;
      if (subset!=MPI_COMM_NULL) {
        World worker(subset);Owned owned;
        for (auto row:migrated) owned.emplace(row.cell.id,row);
        EngineOptions options;options.maximum_rounds=256;options.timings=true;
        auto result=coarsen ? Coarsen(worker,owned,original,reference,{},options) :
                             Refine(worker,owned,original,reference,{},options);
        EngineReport(result,worker,label);
        valid=result.valid;updated.swap(result.cells);
        MPI_Comm_free(&subset);
      }
      const bool failed=parent.maximum(!valid);CHECK_FALSE(failed);
      const double worker_s=elapsed(worker_start);const auto returning=Clock::now();
      to.assign(parent.size,{});
      for (const auto& row:updated) to[row.first%parent.size].push_back(row.second);
      const auto returned=parent.exchange(to);Owned output;
      for (auto row:returned) output.emplace(row.cell.id,row);
      const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);
      const double return_s=elapsed(returning);const auto exporting=Clock::now();
      if (step==0) CHECK(count==2);
      CHECK(count>=2);CHECK(count<=128);
      CHECK(Bytes(accepted)==saved);CHECK(Bytes(input)==input_bytes);
      MeshRecords(label+(failed ? "_rejected" : ""),parent,output);
      const double export_s=elapsed(exporting),total_s=elapsed(started);
      const double other_s=total_s-preparation_s-migration_s-communicator_s-worker_s-return_s-export_s;
      CHECK(other_s>=-1e-12);
      std::ostringstream profile;
      profile<<std::setprecision(17)<<"COPLANAR_EVENT case="<<label<<" rank="<<parent.rank<<" ranks="<<parent.size
          <<" workers="<<m<<" step="<<step<<" valid="<<!failed<<" final="<<count<<" parent_s="<<total_s
          <<" preparation_s="<<preparation_s<<" migration_s="<<migration_s<<" worker_comm_s="<<communicator_s
          <<" worker_s="<<worker_s<<" return_s="<<return_s<<" record_export_s="<<export_s<<" other_s="<<other_s
          <<" inactive_worker_wait_s="<<(parent.rank>=m ? worker_s : 0)<<'\n';
      Log(parent,profile.str());
      if (failed) break;
      accepted.swap(output);
    }
  }
}
#endif

TEST_CASE("Native MPI publication notifies complete changed stars beyond unchanged candidate edges", "[NativeDistributed3D][NativeCollapseFilter3D]") {
  World world;
  const std::array<Node,6> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}},{4,{.25,.25,.25}},{5,{1,1,1}}}};
  const std::array<std::array<Id,4>,5> layout{{{4,0,1,2},{4,1,2,3},{4,0,2,3},{4,0,3,1},{5,1,2,3}}};
  Fixture fixture;
  for (size_t i=0;i<layout.size();++i) {
    Cell cell; cell.id=10+i;
    for (size_t j=0;j<4;++j) cell.v[j]=nodes[layout[i][j]];
    if (Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0) std::swap(cell.v[0],cell.v[1]);
    fixture.patch.cells.push_back(cell);
  }
  for (auto face:Incidence(fixture.patch.cells).Boundary()) {
    const Id id=fixture.facets.size();
    fixture.facets.push_back({id,int(id)+10,{nodes[face[0]],nodes[face[1]],nodes[face[2]]}});
    fixture.patch.surface.push_back({face,id});
  }
  const PlanarReference geometry(fixture.facets);
  const auto initial_rows=Rows(fixture.patch);
  auto owned=Partition(initial_rows,world);
  const auto saved=Bytes(owned);
  Directory directory(world,true);
  std::string reason;
  REQUIRE(directory.Build(owned,reason));
  const auto before_edge=directory.Lookup(Query(world,{{0,4,0},2}));
  REQUIRE(before_edge.valid);
  const auto dependencies=directory.Lookup(Query(world,{{1,2,3},3}));
  REQUIRE(dependencies.valid);
  const auto imported=directory.Fetch(dependencies,owned);
  REQUIRE(imported.valid);
  const bool reserved=directory.Reserve(imported,world.rank==0,reason);
  Patch fresh;
  bool approved=world.rank!=0;
  if (!world.rank) {
    approved=reserved && Reconnect(PatchOf(imported.cells),4,1000,geometry,fresh,reason);
    Owned all; for (auto row:initial_rows) all.emplace(row.cell.id,row);
    auto donor_rows=Uniform(all,{.25,0,0,.25,0,.25});
    for (auto& donor:donor_rows) for (size_t i=0;i<4;++i) if (donor.cell.v[i].id==4) donor.sensor[i].xx=7;
    FrozenField field(std::move(donor_rows));
    PatchMetric measured;
    approved=approved && ValidateMetricChange(PatchOf(imported.cells).cells,fresh.cells,MetricChange::Repair,field,measured,reason);
  }
  REQUIRE(!world.maximum(!approved));
  std::vector<std::vector<Dependency>> removes(world.size);
  std::vector<std::vector<OwnedCell>> adds(world.size);
  if (!world.rank) {
    for (auto row:dependencies.cells) removes[row.owner].push_back(row);
    adds.back()=Rows(fresh);
  }
  const auto removed=world.exchange(removes);
  const auto added=world.exchange(adds);
  REQUIRE(directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{}, {},nullptr,true));
  const auto changes=directory.ChangedVertices();
  const auto supports=directory.ChangedRepairSupports();
  CHECK(std::is_sorted(supports.begin(),supports.end()));
  CHECK(std::adjacent_find(supports.begin(),supports.end())==supports.end());
  // Cell10 is unchanged, but face {1,2,4}'s healthy support cell11 changed.
  // Its owner must reopen that face; the unchanged edge {0,4} must stay blocked.
  if(world.rank==10%world.size)CHECK(std::binary_search(supports.begin(),supports.end(),std::array<Id,4>{{1,2,3,4}}));
  for(const auto& support:supports)CHECK(std::find(support.begin(),support.end(),0)==support.end());
  CHECK(std::is_sorted(changes.begin(),changes.end()));
  CHECK(std::adjacent_find(changes.begin(),changes.end())==changes.end());
  std::vector<Id> expected;
  for (Id vertex:{1u,2u,3u,4u,5u}) {
    bool owner=world.rank==world.size-1;
    for (const auto& row:initial_rows) if (row.cell.id%world.size==unsigned(world.rank))
      for (auto node:row.cell.v) owner=owner || node.id==vertex;
    if (owner) expected.push_back(vertex);
  }
  CHECK(changes==expected);
  CHECK(std::find(changes.begin(),changes.end(),0)==changes.end());
  const auto after_edge=directory.Lookup(Query(world,{{0,4,0},2}));
  REQUIRE(after_edge.valid);
  CHECK(before_edge.cells.size()==after_edge.cells.size());
  for (size_t i=0;i<before_edge.cells.size();++i) {
    CHECK(before_edge.cells[i].cell==after_edge.cells[i].cell);
    CHECK(before_edge.cells[i].version==after_edge.cells[i].version);
    CHECK(before_edge.cells[i].owner==after_edge.cells[i].owner);
  }
  const auto after_vertex=directory.Lookup(Query(world,{{4,0,0},1}));
  REQUIRE(after_vertex.valid);
  if (!world.rank) CHECK(after_vertex.cells.size()==6);
  if (world.size==4 && world.rank==0) CHECK(Bytes(owned)==saved);
  const auto directory_saved=directory.Snapshot(), cells_saved=Bytes(owned);
  CHECK_FALSE(directory.Publish(owned,{}, {},world.rank!=0,reason));
  CHECK(directory.Snapshot()==directory_saved); CHECK(Bytes(owned)==cells_saved); CHECK(directory.ChangedVertices()==changes);
  CHECK(directory.ChangedRepairSupports()==supports);
  CHECK_FALSE(directory.Publish(owned,{}, {},true,reason,1));
  CHECK(directory.Snapshot()==directory_saved); CHECK(Bytes(owned)==cells_saved); CHECK(directory.ChangedVertices()==changes);
  CHECK(directory.ChangedRepairSupports()==supports);
  CHECK(world.sum(int(directory.Statistics().change_sent))>0);
  if(world.size>1) {
    CHECK_FALSE(directory.Publish(owned,{}, {},true,reason,SIZE_MAX,0,{}, {},nullptr,world.rank==0));
    CHECK(reason.find("mode differs")!=std::string::npos);
    CHECK(directory.Snapshot()==directory_saved);CHECK(Bytes(owned)==cells_saved);
    CHECK(directory.ChangedRepairSupports()==supports);
  }
  CHECK(directory.Statistics().change_seconds>=0);
  Directory mismatched(world,world.rank==0);const auto uninitialized=mismatched.Snapshot();
  if (world.size>1) {
    CHECK_FALSE(mismatched.Build(owned,reason));
    CHECK(reason.find("mode differs")!=std::string::npos); CHECK(mismatched.Snapshot()==uninitialized);
  }
}

TEST_CASE("Native MPI early collapse filtering rejects exact zero-volume witnesses and preserves invalid-field repair", "[NativeDistributed3D][NativeCollapseFilter3D]") {
  World world;
  auto fixture=Box();
  auto rename=[](Id id){return id==0 ? Id(1) : (id==1 ? Id(0) : id);};
  for (auto& cell:fixture.patch.cells) for (auto& node:cell.v) node.id=rename(node.id);
  for (auto& face:fixture.patch.surface) for (auto& id:face.v) id=rename(id);
  for (auto& facet:fixture.facets) for (auto& node:facet.v) node.id=rename(node.id);
  const PlanarReference geometry(fixture.facets);
  Patch split; std::string reason;
  REQUIRE(SplitEdge(fixture.patch,{1,6},{8,{.5,.5,.5}},100,geometry,split,reason));
  // Keep the candidate and its zero-volume witness on the same owner; extra ranks still participate in the workflow.
  Owned input;for(const auto& row:Rows(split))
    if(row.cell.id%std::min(world.size,2)==unsigned(world.rank))input.emplace(row.cell.id,row);
  const auto saved=Bytes(input);
  EngineOptions options;options.timings=true;
  const auto result=Coarsen(world,input,Uniform(input,{.25,0,0,.25,0,.25}),geometry,{},options);
  CHECK(result.valid);CHECK(result.reason.empty());
  CHECK(world.sum(int(result.stats.collapse_orientation_filtered))>0);
  CHECK(world.sum(int(result.stats.collapse_orientation_checks))>0);
  CHECK(result.stats.collapse_orientation_seconds>=0);
  CHECK(result.stats.change_notification_seconds>=0);
  CHECK(result.stats.cached_invalid_cells==0);CHECK(result.stats.remaining_edges==0);
  CHECK(Audit(world,result.cells,fixture,6));CHECK(Bytes(input)==saved);
  EngineReport(result,world,"exact_orientation_filter");
  options.maximum_rounds=1;
  const auto rejected=Coarsen(world,input,Uniform(input,{16,0,0,16,0,16}),geometry,{},options);
  CHECK_FALSE(rejected.valid);CHECK(rejected.stats.collapse_orientation_filtered==0);CHECK(Bytes(input)==saved);
}

TEST_CASE("Native MPI full vertex-star facet filtering protects corners beyond candidate edge cells", "[NativeDistributed3D][NativeCollapseFilter3D]") {
  World world;
  const auto fixture=Box();const PlanarReference geometry(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  Directory directory(world);std::string reason;
  REQUIRE(directory.Build(input,reason));
  const auto dependencies=directory.Lookup(Query(world,{{0,0,0},1}));
  REQUIRE(dependencies.valid);
  const auto imported=directory.Fetch(dependencies,input);
  REQUIRE(imported.valid);
  if (!world.rank) {
    Patch proposal;
    CHECK_FALSE(CollapseVertex(PatchOf(imported.cells),0,1,100,geometry,proposal,reason));
    CHECK(reason.find("incident reference facet")!=std::string::npos);
  }
  EngineOptions options;options.timings=true;
  const auto result=Coarsen(world,input,Uniform(input,{.25,0,0,.25,0,.25}),geometry,{},options);
  CHECK(result.valid);CHECK(result.reason.empty());CHECK(result.stats.accepted==0);
  CHECK(result.stats.cached_invalid_cells==0);CHECK(result.stats.remaining_edges==0);
  CHECK(Audit(world,result.cells,fixture,6));CHECK(Bytes(input)==saved);
  CHECK(world.sum(int(result.stats.collapse_filtered))>0);
  // All serial witnesses are local; MPI owners without a witness still need the complete private star.
  if (world.size==1) CHECK(result.stats.attempted==0);
  EngineReport(result,world,"full_star_facet_filter");
}

TEST_CASE("Native MPI narrow metric-query routing preserves exhaustive closed donor footprints", "[NativeDistributed3D][NativeDonorReuse3D][NativeMetricSamples3D]") {
  World world;bool recovered_centroid=false;
  for (bool thin : {false,true}) {
    auto transform = [=](Point p) { return thin ? Point{.5*(p.x+p.y),.5*(p.y-p.x),std::ldexp(p.z,-20)} : p; };
    std::vector<Cell> cells;
    const auto tile=Box().patch.cells;
    for (Id z=0;z<4;++z) for (Id y=0;y<4;++y) for (Id x=0;x<4;++x) for (auto cell:tile) {
      cell.id=cells.size();
      for (auto& node:cell.v) {
        const Id i=x+Id(node.p.x),j=y+Id(node.p.y),k=z+Id(node.p.z);
        node.id=i+5*(j+5*k);node.p=transform({i/4.,j/4.,k/4.});
      }
      cells.push_back(cell);
    }
    REQUIRE(cells.size()>MaximumDonors);
    auto original=thin ? (world.rank ? std::vector<DonorCell>{} : Source(cells,world,true)) : Source(cells,world);
    DonorImport service(world);std::string reason;
    REQUIRE(service.Build(original,reason));const auto resident=service.ResidentBytes();
    const std::array<Tetrahedron,4> probes{{
      {{{.125,.125,.125},{.875,.125,.125},{.125,.875,.125},{.125,.125,.875}}},
      {{{0,0,0},{.75,0,0},{0,.75,0},{0,0,.125}}},
      {{{.25,.25,.25},{.5,.25,.25},{.25,.5,.25},{.25,.25,.5}}},
      {{{.11,.13,.17},{.83,.19,.23},{.17,.79,.29},{.19,.23,.81}}}
    }};
    for (auto probe:probes) {
      Cell cell;
      for(size_t i=0;i<4;++i) cell.v[i]={1000+i,transform(probe[i])};
      std::set<Id> expected;
      const auto center=Centroid(cell);
      for (const auto& donor:cells) {
        const Tetrahedron t{donor.v[0].p,donor.v[1].p,donor.v[2].p,donor.v[3].p};
        bool needed=IntersectsSegment(t,center,center);
        for(size_t i=0;i<4 && !needed;++i) for(size_t j=i+1;j<4 && !needed;++j)
          needed=IntersectsSegment(t,cell.v[i].p,cell.v[j].p);
        if(needed) expected.insert(donor.id);
      }
      REQUIRE_FALSE(expected.empty());REQUIRE(expected.size()<=MaximumDonors);
      const auto query=world.rank==world.size-1 ? std::vector<Cell>{cell} : std::vector<Cell>{};
      const auto requests_before=service.Statistics().owner_requests;
      const auto imported=service.ImportMetricCells(query);REQUIRE(imported.valid);
      const auto requests=CPassiveComm::Allreduce(service.Statistics().owner_requests-requests_before,CPassiveComm::Op::SUM,world.comm);
      CHECK(requests>0);CHECK(requests<=static_cast<uint64_t>(world.size));
      if(thin)CHECK(requests==1);
      if(world.rank==world.size-1) {
        std::set<Id> actual;for(const auto& donor:imported.cells)actual.insert(donor.cell.id);
        CHECK(actual==expected);
        FrozenField field(imported.cells);
        const auto value=TensorValues(field.Query(center).sensor),analytic=TensorValues(Sensor(center));
        for(size_t i=0;i<6;++i)CHECK(value[i]==Approx(analytic[i]).epsilon(1e-13));
        for(size_t i=0;i<4;++i)for(size_t j=i+1;j<4;++j)CHECK(field.SensorEdgeLength(cell.v[i].p,cell.v[j].p)>0);
      } else CHECK(imported.cells.empty());
      std::vector<std::array<Point,2>> segments;
      if(world.rank==world.size-1)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j)segments.push_back({cell.v[i].p,cell.v[j].p});
      const auto edge_only=service.ImportEdgeSegments(segments);REQUIRE(edge_only.valid);
      const auto canonical=service.SamplePoints(world.rank==world.size-1?std::vector<Point>{center}:std::vector<Point>{});REQUIRE(canonical.valid);
      const auto exact_samples=service.ImportMetricSamples(query);INFO(exact_samples.reason);REQUIRE(exact_samples.valid);
      if(world.rank==world.size-1) {
        const auto key=canonical.samples.at(0).Key();
        recovered_centroid=recovered_centroid || std::none_of(edge_only.cells.begin(),edge_only.cells.end(),[&](const DonorCell& d){return d.Key()==key;});
        CHECK(std::any_of(exact_samples.cells.begin(),exact_samples.cells.end(),[&](const DonorCell& d){return d.Key()==key;}));
        FrozenField exact(exact_samples.cells);const auto actual=TensorValues(exact.Query(center).sensor),analytic=TensorValues(Sensor(center));
        for(unsigned k=0;k<6;++k)CHECK(actual[k]==Approx(analytic[k]).epsilon(1e-13));
        for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j)CHECK(exact.SensorEdgeLength(cell.v[i].p,cell.v[j].p)>0);
      }
      CHECK_FALSE(service.ImportMetricCells(query,1).valid);CHECK(service.ResidentBytes()==resident);
      auto invalid=query;if(!invalid.empty())std::swap(invalid.front().v[0],invalid.front().v[1]);
      CHECK_FALSE(service.ImportMetricCells(invalid).valid);CHECK(service.ResidentBytes()==resident);
    }
    const auto& counters=service.Statistics();
    CHECK(counters.box_filtered<=counters.filtered);
    const auto rejected=CPassiveComm::Allreduce(counters.box_filtered,CPassiveComm::Op::SUM,world.comm);
    if constexpr (std::numeric_limits<long double>::digits>=std::numeric_limits<double>::digits &&
                  std::numeric_limits<long double>::max_exponent>=2*std::numeric_limits<double>::max_exponent+4 &&
                  std::numeric_limits<long double>::min_exponent<=2*(std::numeric_limits<double>::min_exponent-std::numeric_limits<double>::digits)-4)
      CHECK(rejected>0);
    else CHECK(rejected==0);
  }
  CHECK(world.maximum(recovered_centroid));
}

TEST_CASE("Native MPI owner publication admits aggregated independent cavities", "[NativeDistributed3D][NativeStaticFix3D]") {
  World world;
  Owned owned;
  std::vector<Dependency> removed;
  std::vector<OwnedCell> added;
  std::string reason;
  if (world.rank == 0) for (Id i = 0; i <= MaximumCavityCells; ++i) {
    const double x = 2.*i;
    const Cell cell{i, {{{4*i,{x,0,0}}, {4*i+1,{x+1,0,0}}, {4*i+2,{x,1,0}}, {4*i+3,{x,0,1}}}}};
    OwnedCell old; old.cell = cell;
    owned.emplace(i,old);
    removed.push_back({i,0,0});
    std::vector<Cell> fresh;
    REQUIRE(Cone({cell},{10000+i,{x+.25,.25,.25}},100000+4*i,fresh,reason));
    for (auto c : fresh) { OwnedCell row; row.cell=c; added.push_back(row); }
  }
  Directory directory(world,true);
  REQUIRE(directory.Build(owned,reason));
  const auto before = Bytes(owned), metadata = directory.Snapshot();
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,world.rank == 0 ? 1 : SIZE_MAX));
  CHECK(Bytes(owned)==before); CHECK(directory.Snapshot()==metadata);
  CHECK_FALSE(directory.Publish(owned,removed,added,world.rank != 0,reason));
  CHECK(Bytes(owned)==before); CHECK(directory.Snapshot()==metadata);
  REQUIRE(directory.Publish(owned,removed,added,true,reason));
  CHECK(owned.size()==(world.rank == 0 ? 4*(MaximumCavityCells+1) : 0));
  const auto dependencies = directory.Lookup(Query(world,{{10000,0,0},1}));
  REQUIRE(dependencies.valid);
  CHECK(dependencies.cells.size()==(world.rank == 0 ? 4 : 0));
  CHECK(directory.Statistics().publications==1);
}

TEST_CASE("Batch cavity reservation honors explicit priority before ownership", "[NativeRepairBatch3D][NativeDistributed3D]") {
  World world;const auto fixture=Box();auto owned=Partition(Rows(fixture.patch),world);
  Directory directory(world);std::string reason;REQUIRE(directory.Build(owned,reason));
  const auto mesh=Bytes(owned),snapshot=directory.Snapshot();
  const std::vector<Feature> features{{{0,0,0},1},{{6,0,0},1}};
  const auto dependencies=directory.LookupBatch(features);REQUIRE(dependencies.valid);
  const auto imported=directory.FetchBatch(dependencies,owned);REQUIRE(imported.valid);
  const auto ordinary=directory.ReserveBatch(imported);REQUIRE(ordinary.valid);
  REQUIRE(ordinary.chosen.size()==2);
  CHECK(ordinary.chosen[0]==(world.rank==0));CHECK_FALSE(ordinary.chosen[1]);
  const std::vector<unsigned> priorities{unsigned(100+world.rank),unsigned(world.size-1-world.rank)};
  const auto prioritized=directory.ReserveBatch(imported,SIZE_MAX,0,priorities);REQUIRE(prioritized.valid);
  REQUIRE(prioritized.chosen.size()==2);
  CHECK_FALSE(prioritized.chosen[0]);CHECK(prioritized.chosen[1]==(world.rank==world.size-1));
  CHECK_FALSE(directory.ReserveBatch(imported,world.rank?SIZE_MAX:1,0,priorities).valid);
  CHECK_FALSE(directory.ReserveBatch(imported,SIZE_MAX,0,{0}).valid);
  CHECK(Bytes(owned)==mesh);CHECK(directory.Snapshot()==snapshot);
}

TEST_CASE("Native MPI batch lookup preserves per-candidate complete stars and refusals", "[NativeDistributed3D][NativeBatchLookup3D]") {
  World world;
  Owned owned;
  for (Id i=0;i<=MaximumCavityCells;++i) {
    OwnedCell row; row.cell={i, {{{4*i,{2.*i,0,0}}, {4*i+1,{2.*i+1,0,0}},
                                {4*i+2,{2.*i,1,0}}, {4*i+3,{2.*i,0,1}}}}};
    if (i % world.size == unsigned(world.rank)) owned.emplace(i,row);
    row.cell={1000+i, {{{500,{0,100,0}}, {2000+i,{1,100.+i,0}},
                         {2001+i,{1,101.+i,0}}, {501,{0,100,1}}}}};
    if (i % world.size == unsigned(world.rank)) owned.emplace(row.cell.id,row);
  }
  Directory directory(world);
  std::string reason;
  REQUIRE(directory.Build(owned,reason));
  const auto snapshot=directory.Snapshot(), mesh=Bytes(owned);
  auto pack=[](const Dependencies& result) {
    std::vector<char> bytes;RecordStream out(bytes);
    for (auto row:result.cells) out(row);
    return bytes;
  };
  std::vector<Feature> features;
  if (!world.rank) for (Id i=0;i<=MaximumCavityCells;++i) features.push_back({{4*i,0,0},1});
  const auto large=directory.LookupBatch(features);
  REQUIRE(large.valid);
  REQUIRE(large.candidates.size()==features.size());
  for (size_t i=0;i<large.candidates.size();++i) {
    CHECK(large.candidates[i].valid);
    REQUIRE(large.candidates[i].cells.size()==1);
    CHECK(large.candidates[i].cells.front().cell==i);
  }
  CHECK_FALSE(directory.Lookup(features).valid);  // Legacy union still has its original64-cell limit.
  const auto large_payload=directory.FetchBatch(large,owned);
  REQUIRE(large_payload.valid);
  const auto large_reservation=directory.ReserveBatch(large_payload);
  REQUIRE(large_reservation.valid);
  CHECK(large_reservation.chosen.size()==features.size());
  for (size_t i=0;i<large_payload.candidates.size();++i) {
    REQUIRE(large_payload.candidates[i].valid);
    REQUIRE(large_payload.candidates[i].cells.size()==1);
    CHECK(large_payload.candidates[i].cells[0].cell.id==i);
    CHECK(large_reservation.chosen[i]);
  }

  std::vector<Dependencies> serial;
  const auto baseline_begin=world.collective_calls, baseline_bytes=world.bytes_sent;
  for (Id i=0;i<5;++i) {
    const auto result=directory.Lookup(Query(world,{{4*i,0,0},1}));
    REQUIRE(result.valid);serial.push_back(result);
  }
  const auto serial_calls=world.collective_calls-baseline_begin, serial_bytes=world.bytes_sent-baseline_bytes;
  if (!world.rank) features.resize(5);
  const auto batch_begin=world.collective_calls, batch_bytes_begin=world.bytes_sent;
  const auto batch=directory.LookupBatch(features);
  const auto batch_calls=world.collective_calls-batch_begin, batch_bytes=world.bytes_sent-batch_bytes_begin;
  REQUIRE(batch.valid);REQUIRE(batch.candidates.size()==features.size());
  for (size_t i=0;i<features.size();++i) CHECK(pack(batch.candidates[i])==pack(serial[i]));
  CHECK(batch_calls<serial_calls);  // Wrapper calls only; not measured MPI wait or speed.
  CHECK(batch_bytes==serial_bytes);
  std::ostringstream counts;
  counts << "BATCH_LOOKUP rank=" << world.rank << " ranks=" << world.size
         << " features=" << features.size() << " serial_wrapped_calls=" << serial_calls
         << " batch_wrapped_calls=" << batch_calls << " serial_payload_bytes=" << serial_bytes
         << " batch_payload_bytes=" << batch_bytes << '\n';
  Log(world,counts.str());
  const auto mixed=directory.LookupBatch(world.rank ? std::vector<Feature>{} :
      std::vector<Feature>{{{0,0,0},1},{{999999,0,0},1},{{500,0,0},1},{{0,0,0},1}});
  REQUIRE(mixed.valid);
  if (!world.rank) {
    REQUIRE(mixed.candidates.size()==4);
    CHECK(mixed.candidates[0].valid);CHECK(mixed.candidates[3].valid);
    CHECK(pack(mixed.candidates[0])==pack(mixed.candidates[3]));
    CHECK_FALSE(mixed.candidates[1].valid);CHECK(mixed.candidates[1].cells.empty());
    CHECK(mixed.candidates[1].reason.find("missing")!=std::string::npos);
    CHECK_FALSE(mixed.candidates[2].valid);CHECK(mixed.candidates[2].cells.empty());
    CHECK(mixed.candidates[2].reason.find("budget")!=std::string::npos);
  }
  const auto dimensions=directory.LookupBatch(world.rank ? std::vector<Feature>{} :
      std::vector<Feature>{{{0,1,0},2},{{2,0,1},3}});
  REQUIRE(dimensions.valid);
  if (!world.rank) {
    REQUIRE(dimensions.candidates.size()==2);
    CHECK(dimensions.candidates[0].valid);CHECK(dimensions.candidates[1].valid);
    CHECK(pack(dimensions.candidates[0])==pack(serial[0]));
    CHECK(pack(dimensions.candidates[1])==pack(serial[0]));
  }
  const auto repeated=directory.LookupBatch(world.rank ? std::vector<Feature>{} :
      std::vector<Feature>{{{0,0,0},1},{{4,0,0},1},{{0,0,0},1}});
  const auto payload_begin=directory.Statistics().payloads;
  const auto repeated_payload=directory.FetchBatch(repeated,owned);
  REQUIRE(repeated_payload.valid);
  CHECK(directory.Statistics().payloads-payload_begin==(world.rank ? 0 : 2));
  if (!world.rank) {
    REQUIRE(repeated_payload.candidates.size()==3);
    CHECK(Bytes(repeated_payload.candidates[0].cells[0])==Bytes(repeated_payload.candidates[2].cells[0]));
  }
  for (size_t i=0;i<3;++i) {
    const auto serial_payload=directory.Fetch(world.rank ? Dependencies{{},true,{}} : repeated.candidates[i],owned);
    REQUIRE(serial_payload.valid);
    if (!world.rank) CHECK(Bytes(serial_payload.cells[0])==Bytes(repeated_payload.candidates[i].cells[0]));
  }
  auto stale=directory.LookupBatch(world.rank ? std::vector<Feature>{} :
      std::vector<Feature>{{{0,0,0},1},{{4,0,0},1}});
  if (!world.rank) ++stale.candidates[1].cells[0].version;
  const auto stale_payload=directory.FetchBatch(stale,owned);
  REQUIRE(stale_payload.valid);
  if (!world.rank) {
    CHECK(stale_payload.candidates[0].valid);CHECK_FALSE(stale_payload.candidates[1].valid);
    CHECK(stale_payload.candidates[1].cells.empty());
  }
  const auto partial=directory.ReserveBatch(stale_payload);
  REQUIRE(partial.valid);
  if (!world.rank) { CHECK(partial.chosen[0]);CHECK_FALSE(partial.chosen[1]); }
  auto inconsistent=repeated;
  if (!world.rank) ++inconsistent.candidates[2].cells[0].version;
  CHECK_FALSE(directory.FetchBatch(inconsistent,owned).valid);
  const auto shared=directory.LookupBatch({{{0,0,0},1},{{4,0,0},1},{{0,0,0},1}});
  REQUIRE(shared.valid);
  const auto shared_payload=directory.FetchBatch(shared,owned);
  REQUIRE(shared_payload.valid);
  const auto reservation=directory.ReserveBatch(shared_payload);
  REQUIRE(reservation.valid);REQUIRE(reservation.chosen.size()==3);
  CHECK(reservation.chosen[0]==(world.rank==0));CHECK(reservation.chosen[1]==(world.rank==0));
  CHECK_FALSE(reservation.chosen[2]);  // Same caller candidates also conflict on their complete stars.
  const auto mixed_payload=directory.FetchBatch(mixed,owned);
  REQUIRE(mixed_payload.valid);
  if (!world.rank) {
    CHECK(mixed_payload.candidates[0].valid);CHECK(mixed_payload.candidates[3].valid);
    CHECK_FALSE(mixed_payload.candidates[1].valid);CHECK_FALSE(mixed_payload.candidates[2].valid);
  }
  CHECK_FALSE(directory.FetchBatch(repeated,owned,world.rank ? SIZE_MAX : 1).valid);
  CHECK_FALSE(directory.ReserveBatch(shared_payload,world.rank ? SIZE_MAX : 1).valid);
  auto malformed=shared_payload;
  if (!world.rank) malformed.candidates[0].cells.clear();
  CHECK_FALSE(directory.ReserveBatch(malformed).valid);
  const auto idle=directory.LookupBatch({});
  CHECK(idle.valid);CHECK(idle.candidates.empty());
  const auto idle_payload=directory.FetchBatch(idle,owned);
  REQUIRE(idle_payload.valid);CHECK(idle_payload.candidates.empty());
  const auto idle_reserved=directory.ReserveBatch(idle_payload);
  CHECK(idle_reserved.valid);CHECK(idle_reserved.chosen.empty());
  CHECK_FALSE(directory.LookupBatch(world.rank ? std::vector<Feature>{} :
      std::vector<Feature>(MaximumDirectoryFeatures+1,{{0,0,0},1})).valid);
  const auto refused=directory.LookupBatch(features,world.rank ? SIZE_MAX : 1);
  CHECK_FALSE(refused.valid);CHECK(refused.candidates.empty());
  CHECK_FALSE(directory.LookupBatch(Query(world,{{0,0,0},2})).valid);
  CHECK(directory.Snapshot()==snapshot);CHECK(Bytes(owned)==mesh);
}

TEST_CASE("Native metadata planning filters remote-star conflicts before payloads and retains final proofs", "[NativeDistributed3D][NativeBatchPlanning3D]") {
  World world;Patch patch;
  patch.cells={{0,{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}},
               {1,{{{4,{2,0,0}},{5,{2,1,0}},{6,{2,0,1}},{3,{0,0,1}}}}},
               {2,{{{8,{5,0,0}},{9,{6,0,0}},{10,{5,1,0}},{11,{5,0,1}}}}}};
  for(auto& cell:patch.cells)if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
  auto owned=Partition(Rows(patch),world);Directory directory(world);std::string reason;
  REQUIRE(directory.Build(owned,reason));const auto saved=directory.Snapshot(),mesh=Bytes(owned);
  const std::vector<Feature> features{{{0,0,0},1},{{4,0,0},1},{{8,0,0},1}};
  std::vector<size_t> slots;const auto payload_begin=directory.Statistics().payloads;
  const auto plan=directory.PlanBatch(features,slots);INFO(plan.reason);REQUIRE(plan.valid);
  CHECK(directory.Statistics().payloads==payload_begin);
  CHECK(slots==(world.rank?std::vector<size_t>{}:std::vector<size_t>{0,2}));
  CHECK(plan.candidates.size()==slots.size());
  const auto fetched=directory.FetchBatch(plan,owned);REQUIRE(fetched.valid);
  const auto reserved=directory.ReserveBatch(fetched);REQUIRE(reserved.valid);
  for(bool chosen:reserved.chosen)CHECK(chosen);
  CHECK(world.sum(int(directory.Statistics().payloads-payload_begin))==2);
  // The distinct feature at4 shares only vertex3 with the first complete star, on a different cell/owner.
  auto changed=owned;for(auto& row:changed)if(row.first==0)++row.second.version;
  const auto stale=directory.FetchBatch(plan,changed);REQUIRE(stale.valid);
  if(!world.rank){REQUIRE(stale.candidates.size()==2);CHECK_FALSE(stale.candidates[0].valid);CHECK(stale.candidates[1].valid);}
  CHECK_FALSE(directory.PlanBatch(features,slots,world.rank?SIZE_MAX:1).valid);CHECK(slots.empty());
  CHECK_FALSE(directory.PlanBatch(world.rank?std::vector<Feature>{}:std::vector<Feature>{{{UINT64_MAX,0,0},1}},slots).valid);
  CHECK(slots.empty());
  const auto idle=directory.PlanBatch({},slots);CHECK(idle.valid);CHECK(idle.candidates.empty());CHECK(slots.empty());
  CHECK(directory.Snapshot()==saved);CHECK(Bytes(owned)==mesh);
}

TEST_CASE("Native batched coarsening completes disjoint planar edits in one epoch", "[NativeDistributed3D][NativeBatchEngine3D]") {
  World world;
  Fixture fixture;
  Owned input;
  std::vector<DonorCell> source;
  for (Id i=0;i<MaximumCandidateBatch;++i) {
    auto box=Box();
    for (auto& cell:box.patch.cells) {
      cell.id+=100*i;
      for (auto& node:cell.v) { node.id+=16*i;node.p.x+=2*i; }
    }
    for (auto& face:box.patch.surface) { face.facet+=12*i; for (auto& id:face.v) id+=16*i; }
    for (auto& facet:box.facets) {
      facet.id+=12*i;
      for (auto& node:facet.v) { node.id+=16*i;node.p.x+=2*i; }
    }
    const PlanarReference geometry(box.facets);
    Patch split;std::string reason;
    REQUIRE(SplitEdge(box.patch,{16*i,16*i+6},{16*i+8,{2.*i+.5,.5,.5}},100*i+20,geometry,split,reason));
    const auto owned=Partition(Rows(split),world);
    input.insert(owned.begin(),owned.end());
    auto donors=Uniform(Partition(Rows(box.patch),world),{1,0,0,1,0,1});
    source.insert(source.end(),donors.begin(),donors.end());
    fixture.patch.cells.insert(fixture.patch.cells.end(),box.patch.cells.begin(),box.patch.cells.end());
    fixture.facets.insert(fixture.facets.end(),box.facets.begin(),box.facets.end());
    fixture.patch.surface.insert(fixture.patch.surface.end(),box.patch.surface.begin(),box.patch.surface.end());
  }
  const PlanarReference geometry(fixture.facets);
  const auto saved=Bytes(input);
  EngineOptions options;options.maximum_rounds=1;options.timings=true;
  const auto result=Coarsen(world,input,source,geometry,{},options);
  INFO(result.reason);
  REQUIRE(result.valid);
  CHECK(Bytes(input)==saved);
  CHECK(result.stats.rounds==1);CHECK(result.stats.batch_rounds==1);
  CHECK(world.sum(int(result.stats.accepted))==int(MaximumCandidateBatch));
  CHECK(result.stats.remaining_edges==0);CHECK(result.stats.cached_invalid_cells==0);
  CHECK(result.stats.other_seconds>=-1e-12);
  const auto final=Gather(world,result.cells);
  if (!world.rank) {
    REQUIRE(final.size()==6*MaximumCandidateBatch);
    const auto cells=PatchOf(final).cells;
    CHECK_NOTHROW(ValidateDomainSurfaceTopology(Incidence(cells).Boundary()));
    // Root's independent metric field must cover every disconnected component.
    std::vector<DonorCell> all;
    for (const auto& cell:fixture.patch.cells) { DonorCell donor;donor.cell=cell;donor.sensor.fill({1,0,0,1,0,1});all.push_back(donor); }
    FrozenField full(all);PatchMetric metric;std::string reason;
    CHECK(ValidateMetricPatch(cells,full,metric,reason));
    long double volume=0;
    for (const auto& cell:cells) volume+=Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)/6;
    CHECK(std::abs(volume-MaximumCandidateBatch)<1e-12L);
  }
  MeshRecords("batched_coarsen_initial",world,input);
  MeshRecords("batched_coarsen_final",world,result.cells);
  EngineReport(result,world);
  options.candidate_batch=1;
  const auto serial=Coarsen(world,input,source,geometry,{},options);
  CHECK_FALSE(serial.valid);CHECK(serial.stats.cached_invalid_cells==0);
  CHECK(world.sum(int(serial.stats.accepted))<int(MaximumCandidateBatch));
  CHECK(Bytes(input)==saved);
  options.candidate_batch=world.rank ? 1 : MaximumCandidateBatch;
  if (world.size>1) CHECK_FALSE(Coarsen(world,input,source,geometry,{},options).valid);
  options.candidate_batch=MaximumCandidateBatch+1;
  CHECK_FALSE(Coarsen(world,input,source,geometry,{},options).valid);
  options.candidate_batch=MaximumCandidateBatch;options.maximum_rounds=512;
  const auto coarse=Partition(Rows(fixture.patch),world);
  const auto coarse_saved=Bytes(coarse);
  const auto refined=Refine(world,coarse,Uniform(coarse,{4,0,0,4,0,4}),geometry,{},options);
  INFO(refined.reason);
  REQUIRE(refined.valid);CHECK(Bytes(coarse)==coarse_saved);
  CHECK(refined.stats.batch_rounds>0);
  CHECK(world.sum(int(refined.stats.accepted))>int(refined.stats.rounds));
  CHECK(refined.stats.remaining_edges==0);CHECK(refined.stats.cached_invalid_cells==0);
  CHECK(refined.stats.other_seconds>=-1e-12);
  MeshRecords("batched_refine_initial",world,coarse);
  MeshRecords("batched_refine_final",world,refined.cells);
  EngineReport(refined,world);
}

TEST_CASE("Native proposal and clock budgets reject collectively without publishing input", "[NativeDistributed3D][NativePolicy3D]") {
  World world;const auto box=Box();const PlanarReference geometry(box.facets);
  Patch split;std::string reason;
  REQUIRE(SplitEdge(box.patch,{0,6},{8,{.5,.5,.5}},20,geometry,split,reason));
  const auto input=Partition(Rows(split),world);const auto saved=Bytes(input);
  const auto donors=Uniform(Partition(Rows(box.patch),world),{1,0,0,1,0,1});
  EngineOptions options;options.maximum_work=0;
  const auto refused=Coarsen(world,input,donors,geometry,{},options);
  CHECK_FALSE(refused.valid);CHECK(refused.stats.stop==EngineStop::WorkBudget);
  CHECK(refused.stats.global_work==0);CHECK(refused.stats.accepted==0);
  CHECK_FALSE(world.maximum(refused.stats.cached_invalid_cells!=0));
  CHECK(Bytes(refused.cells)==Bytes(WithUniformSensor(input,{1,0,0,1,0,1})));CHECK(Bytes(input)==saved);
  options.maximum_work=64;options.maximum_seconds=1e-12;
  const auto timed=Coarsen(world,input,donors,geometry,{},options);
  CHECK_FALSE(timed.valid);CHECK(timed.stats.stop==EngineStop::TimeBudget);
  CHECK(timed.stats.global_work==0);CHECK(Bytes(timed.cells)==Bytes(WithUniformSensor(input,{1,0,0,1,0,1})));CHECK(Bytes(input)==saved);
  CHECK(timed.stats.initial_cells==0);CHECK(timed.reason.find("metric seeding")!=std::string::npos);
  options.maximum_seconds=0;
  const auto completed=Coarsen(world,input,donors,geometry,{},options);
  INFO(completed.reason);REQUIRE(completed.valid);
  CHECK(completed.stats.stop==EngineStop::Complete);
  CHECK(completed.stats.global_work<=64);
  CHECK(world.sum(int(completed.stats.cell_removals))==1);
  if(world.size>1)CHECK(world.sum(int(completed.stats.proposal_duplicates+completed.stats.batch_deferred))>0);
  CHECK(world.sum(int(completed.stats.metric_trials))>=world.sum(int(completed.stats.accepted)));
  CHECK(Bytes(input)==saved);
  if (world.size>1) {
    options.maximum_work=world.rank ? 64 : 65;
    const auto mismatch=Coarsen(world,input,donors,geometry,{},options);
    CHECK_FALSE(mismatch.valid);CHECK(mismatch.stats.stop==EngineStop::Failure);
    CHECK(Bytes(input)==saved);
  }
}

TEST_CASE("MPI imports preserve declared exterior component and original source sensor semantics",
          "[NativeLayeredDistributed3D]") {
  World world;const auto geometry=Box();auto original=Source(geometry.patch.cells,world);
  DonorCell extension;extension.cell={1000000,{{{1000000,{1,0,0}},{1000001,{0,0,0}},
                                               {1000002,{0,1,0}},{1000003,{0,0,-.125}}}}};
  extension.extension_component=77;for(auto& m:extension.sensor)m={9,0,0,9,0,9};
  if(world.rank==world.size-1)original.push_back(extension);
  DonorImport service(world);std::string reason;REQUIRE(service.Build(original,reason));
  auto imported=service.Import({DonorRegion{0,0,-.125,1,1,1}});REQUIRE(imported.valid);
  REQUIRE(imported.cells.size()==7);size_t count=0;
  for(const auto& d:imported.cells)count+=d.extension_component==77;
  CHECK(count==1);
  auto complete=Source(geometry.patch.cells,world,true);complete.push_back(extension);
  FrozenField oracle(complete,{},false,{77}),field(imported.cells,{},false,{77}),wrong(imported.cells,{},false,{78});
  const Point outside{.1,.1,-.06},inside{.1,.1,.01};
  CHECK(field.Query(outside).sensor.xx==9);CHECK(field.Query(outside).extension_component==77);
  CHECK(field.Query(inside).sensor.xx==oracle.Query(inside).sensor.xx);
  CHECK(field.Query(inside).extension_component==UINT64_MAX);CHECK_THROWS(wrong.Query(outside));
  CHECK(field.SensorEdgeLength(inside,outside)==oracle.SensorEdgeLength(inside,outside));
  CHECK_THROWS(wrong.SensorEdgeLength(inside,outside));
  auto gap=extension;
  gap.cell.v={{{1000000,{0,0,-.125}},{1000001,{1,0,-.125}},
               {1000002,{0,1,-.125}},{1000003,{0,0,0}}}};
  auto broken=Source(geometry.patch.cells,world,true);broken.push_back(gap);
  FrozenField gapped(broken,{},false,{77});
  CHECK(gapped.Query(outside).sensor.xx==9);
  CHECK_THROWS(gapped.SensorEdgeLength(inside,outside)); // Covered vertices do not establish coverage between them.
}

TEST_CASE("Curved partition-interface publication preserves frozen source on MPI veto", "[NativeCurvedTransaction3D]") {
  World parent;const int m=GENERATE(1,2,4);if(m>parent.size)return;
  MPI_Comm subset=MPI_COMM_NULL;MPI_Comm_split(parent.comm,parent.rank<m?0:MPI_UNDEFINED,parent.rank,&subset);
  bool valid=true;
  if(subset!=MPI_COMM_NULL) {
    World world(subset);const std::array<Node,5> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{.5,.5,-1}},{4,{1,1,0}}}};
    Patch old;
    for(auto v:{std::array<Node,4>{nodes[0],nodes[1],nodes[2],nodes[3]},std::array<Node,4>{nodes[1],nodes[4],nodes[2],nodes[3]}}) {
      if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);old.cells.push_back({Id(old.cells.size()),v});
    }
    std::vector<Facet> facets;std::map<Id,AnalyticSurface> curves;
    AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.extent={1,1,.1};
    for(auto f:Incidence(old.cells).Boundary()) {
      const auto id=Id(facets.size());const bool roof=std::find(f.begin(),f.end(),3)==f.end();
      facets.push_back({id,roof?0:1,{nodes[f[0]],nodes[f[1]],nodes[f[2]]}});old.surface.push_back({f,id});if(roof)curves.emplace(id,g);
    }
    const PlanarReference geometry(facets,SIZE_MAX,curves);Owned owned=Partition(Rows(old),world);
    const Tensor sensor{.5,0,0,.5,0,.5};std::vector<DonorCell> complete;
    for(const auto& c:old.cells){DonorCell d;d.cell=c;d.sensor.fill(sensor);complete.push_back(d);}
    // Declare the finite graph extension before constructing or freezing any private proposal.
    for(const auto& f:old.surface)if(curves.count(f.facet)) {
      std::array<Node,3> a{{nodes[f.v[0]],nodes[f.v[1]],nodes[f.v[2]]}};std::sort(a.begin(),a.end(),[](Node x,Node y){return x.id<y.id;});auto b=a;
      for(auto& n:b){n.id+=100;n.p.z=std::nextafter(.1/16+2*g.tolerance,INFINITY);}
      for(auto v:std::array<std::array<Node,4>,3>{{{a[0],a[1],a[2],b[2]},{a[0],a[1],b[1],b[2]},{a[0],b[0],b[1],b[2]}}}) {
        if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);DonorCell d;d.cell={Id(1000+complete.size()),v};d.sensor.fill(sensor);d.extension_component=geometry.Canonical(f.facet);complete.push_back(d);
      }
    }
    std::vector<DonorCell> local;for(auto d:complete)if(d.cell.id%world.size==unsigned(world.rank))local.push_back(d);
    auto source_bytes=[&](){std::vector<char> bytes;RecordStream s(bytes);for(auto d:local)s(d);return bytes;};const auto frozen=source_bytes();
    Directory directory(world);std::string reason;REQUIRE(directory.Build(owned,reason));const auto mesh_before=Bytes(owned),directory_before=directory.Snapshot();
    const auto deps=directory.Lookup(Query(world,{{1,2,0},2},true));REQUIRE(deps.valid);REQUIRE(deps.cells.size()==2);
    if(world.size>1){std::set<int> owners;for(auto d:deps.cells)owners.insert(d.owner);CHECK(owners.size()==2);}
    auto stale=deps;if(!world.rank)++stale.cells.front().version;CHECK_FALSE(directory.Fetch(stale,owned).valid);
    CHECK(Bytes(owned)==mesh_before);CHECK(directory.Snapshot()==directory_before);CHECK(source_bytes()==frozen);
    const auto imported=directory.Fetch(deps,owned);REQUIRE(imported.valid);const bool chosen=directory.Reserve(imported,true,reason);CHECK(chosen==(!world.rank));
    DonorImport service(world);REQUIRE(service.Build(local,reason));const auto payload=service.Import({DonorRegion{0,0,-1,1,1,.00626}});REQUIRE(payload.valid);
    FrozenField field(payload.cells,{},false,AssociatedExtensions(payload.cells,geometry));Patch fresh;bool approved=!chosen;
    if(chosen) {
      approved=SplitEdge(PatchOf(imported.cells),{1,2},{5,{.5,.5,0}},10000,geometry,fresh,reason);
      PatchMetric measured;if(approved)approved=ValidateMetricChange(old.cells,fresh.cells,MetricChange::GeometryRefine,field,measured,reason);
      auto error=[&](const Patch& p){long double sum=0;for(auto f:p.surface){std::array<Point,3> q{};for(size_t k=0;k<3;++k)for(auto c:p.cells)for(auto n:c.v)if(n.id==f.v[k])q[k]=n.p;const auto e=std::max(0.,geometry.SurfaceError(f.facet,q)-.003);const auto a=q[0],b=q[1],c=q[2];const auto area=std::hypot((b.y-a.y)*(c.z-a.z)-(b.z-a.z)*(c.y-a.y),(b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z),(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x));sum+=e*e*area;}return sum;};
      if(approved)approved=error(fresh)<error(old);INFO(reason);CHECK(approved);
    }
    valid=!world.maximum(!approved);REQUIRE(valid);
    std::vector<std::vector<Dependency>> remove_to(world.size);std::vector<std::vector<OwnedCell>> add_to(world.size);
    if(chosen){for(auto d:deps.cells)remove_to[d.owner].push_back(d);for(auto c:Rows(fresh))add_to[c.cell.id%world.size].push_back(c);}
    const auto removed=world.exchange(remove_to);const auto added=world.exchange(add_to);
    CHECK_FALSE(directory.Publish(owned,removed,added,world.rank!=0,reason));CHECK(Bytes(owned)==mesh_before);CHECK(directory.Snapshot()==directory_before);CHECK(source_bytes()==frozen);
    CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,world.rank?SIZE_MAX:1));CHECK(Bytes(owned)==mesh_before);CHECK(directory.Snapshot()==directory_before);CHECK(source_bytes()==frozen);
    REQUIRE(directory.Publish(owned,removed,added,true,reason));CHECK(source_bytes()==frozen);
    const auto final=directory.Lookup(Query(world,{{5,0,0},1},true));REQUIRE(final.valid);CHECK(final.cells.size()==4);
    const auto returned=directory.Fetch(final,owned);REQUIRE(returned.valid);CHECK(returned.cells.size()==4);
    for(const auto& c:returned.cells)for(auto n:c.cell.v)if(n.id==5)CHECK(n.p.z==Approx(.00625).epsilon(1e-12));
    CHECK(field.Query({.5,.5,.00625}).sensor.xx==.5);CHECK(source_bytes()==frozen);
  }
  CHECK_FALSE(parent.maximum(!valid));if(subset!=MPI_COMM_NULL)MPI_Comm_free(&subset);
}

TEST_CASE("Native original support preserves affine P1 and falls back at inserted source-edge points", "[NativeOriginalSupport3D]") {
  World world;const auto box=Box();auto current=box;const PlanarReference reference(box.facets);
  const int kind=GENERATE(0,1,2);
  if(kind==1) {Patch fresh;std::string reason;REQUIRE(SplitEdge(box.patch,{0,6},{50,{.5,.5,.5}},100,reference,fresh,reason));current.patch=std::move(fresh);}
  if(kind==2) {
    current.patch.cells.clear();
    for(const auto& c:box.patch.cells) {std::vector<Cell> children;std::string reason;
      REQUIRE(Cone({c},{50+c.id,Centroid(c)},100+4*c.id,children,reason));
      current.patch.cells.insert(current.patch.cells.end(),children.begin(),children.end());
    }
  }
  const auto input=Partition(Rows(current.patch),world);const auto saved=Bytes(input);
  auto donors=Uniform(Partition(Rows(box.patch),World(MPI_COMM_SELF)),{});if(world.rank)donors.clear();
  for(auto& d:donors)for(unsigned k=0;k<4;++k){const auto p=d.cell.v[k].p;d.sensor[k]={1+p.x,0,0,1+p.y,0,16+p.z};}
  size_t observed=0;
  const MetricComposition check=[&](Point p,Tensor m) {
    CHECK(std::abs(m.xx-(1+p.x))<1e-12);CHECK(std::abs(m.yy-(1+p.y))<1e-12);CHECK(std::abs(m.zz-(16+p.z))<1e-12);
    CHECK(m.xy==0);CHECK(m.xz==0);CHECK(m.yz==0);++observed;return m;
  };
  EngineOptions options;options.maximum_work=0;
  options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  const auto result=Refine(world,input,donors,reference,check,options);
  INFO(result.reason);CHECK_FALSE(result.valid);CHECK(result.stats.stop==EngineStop::WorkBudget);
  CHECK(Bytes(input)==saved);CHECK(Bytes(result.cells)==saved);CHECK(world.maximum(observed>0));
  CHECK(result.stats.seed_batches>0);
  CHECK(result.stats.donor_batches==result.stats.seed_batches);
  CHECK(result.stats.metric_support_cells==result.stats.initial_cells);
  CHECK((result.stats.metric_support_edges>0 || input.empty()));
}

TEST_CASE("Native retained first-row insertion reaches full-field measurement without background searches", "[NativeFirstLayerRanking3D]") {
  World world;const bool varying=GENERATE(false,true);const double height=varying?.03:.1;
  const Cell cell{0,{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}};
  Patch old;old.cells={cell};std::vector<Facet> facets;
  const auto skin=Incidence(old.cells).Boundary();
  for(size_t k=0;k<skin.size();++k) {
    std::array<Node,3> nodes;for(unsigned j=0;j<3;++j)for(auto n:cell.v)if(n.id==skin[k][j])nodes[j]=n;
    const bool wall=std::all_of(nodes.begin(),nodes.end(),[](Node n){return n.p.z==0;});
    facets.push_back({k,wall?10:20,nodes});old.surface.push_back({skin[k],k});
  }
  const PlanarReference reference(facets);const Tensor sensor{1,0,0,1,0,100};
  const MetricComposition target=[&](Point p,Tensor m) {
    if(!varying)return m;
    const double dx=p.x-1./3,dy=p.y-1./3,dz=p.z-height/4;
    if(dx*dx+dy*dy+dz*dz<height*height/10000)return Tensor{4,0,0,4,0,1e-8};
    const double x=p.x-.625,y=p.y-.125,z=p.z-.125;
    const double scale=4+1e4*std::exp(-(x*x+y*y+z*z)/.0004);return Tensor{scale,0,0,scale,0,scale};
  };
  const MetricEdgeBreakpoints cuts=[](Point,Point){return std::vector<double>{0,1};};
  Patch candidate;FirstHeightReport report;std::string reason;
  REQUIRE(InsertFirstWallPoint(old,101,200,{{10,height}},reference,candidate,report,reason));
  REQUIRE(candidate.cells.size()==4);CHECK(report.maximum_relative_error<1e-8);
  Owned input;if(!world.rank)for(const auto& row:Rows(old))input.emplace(row.cell.id,row);
  EngineOptions options;options.maximum_work=1;options.wall_heights={{10,height}};options.edge_breakpoints=cuts;
  const auto saved=Bytes(input);auto result=Refine(world,input,Uniform(input,sensor),reference,target,options);
  CHECK(Bytes(input)==saved);CHECK_FALSE(result.valid);CHECK(result.stats.stop==EngineStop::WorkBudget);
  using Tet=std::array<std::array<double,3>,4>;
  auto signature=[](const std::vector<Cell>& cells) {std::vector<Tet> result;for(const auto& c:cells) {
    Tet t;for(unsigned j=0;j<4;++j)t[j]={c.v[j].p.x,c.v[j].p.y,c.v[j].p.z};std::sort(t.begin(),t.end());result.push_back(t);
  }std::sort(result.begin(),result.end());return result;};
  std::vector<Cell> actual;for(const auto& row:result.cells)actual.push_back(row.second.cell);
  if(!world.rank)CHECK(signature(actual)==signature(candidate.cells));
  CHECK(world.maximum(result.stats.edge_evaluations>0));
  CHECK(result.stats.layer_edge_evaluations==0);CHECK(result.stats.layer_donor_batches==0);
}

TEST_CASE("Native first-row owner retains coupled-wall reconstruction with empty MPI ranks", "[NativeFirstLayerEngine3D]") {
  World world;Cell cell;cell.id=0;cell.v[0]={0,{0,0,0}};
  cell.v[1]={1,{1,1.5075567228888185,-.7196119662144681}};
  cell.v[2]={2,{0,1.809068067466582,1.7893054295062427}};
  cell.v[3]={3,{0,0,1.0696934632917758}};
  Patch old;old.cells={cell};std::vector<Facet> facets;const auto skin=Incidence(old.cells).Boundary();
  for(size_t k=0;k<skin.size();++k) {
    std::array<Node,3> nodes;for(unsigned j=0;j<3;++j)for(auto n:cell.v)if(n.id==skin[k][j])nodes[j]=n;
    const bool wall=std::find(skin[k].begin(),skin[k].end(),Id(1))==skin[k].end() || std::find(skin[k].begin(),skin[k].end(),Id(2))==skin[k].end();
    facets.push_back({k,wall?10:20,nodes});old.surface.push_back({skin[k],k});
  }
  const PlanarReference reference(facets);Owned input;if(!world.rank)for(const auto& row:Rows(old))input.emplace(row.cell.id,row);
  const auto saved=Bytes(input);EngineOptions options;options.maximum_work=1;options.wall_heights={{10,.1}};
  auto result=Refine(world,input,Uniform(input,{1,0,0,1,0,1}),reference,{},options);
  INFO(result.reason);CHECK(Bytes(input)==saved);CHECK_FALSE(result.valid);CHECK(result.stats.stop==EngineStop::WorkBudget);
  CHECK(world.sum(int(result.stats.layer_published))==1);CHECK(world.maximum(result.stats.layer_donor_batches>0));
  CHECK(result.stats.layer_shape_screens==0);  // Construction defers q/J to improvement.
  for(const auto& row:result.cells) {FirstHeightReport report;std::string reason;
    CHECK(CheckFirstHeight(PatchOf({row.second}),options.wall_heights,reference,report,reason));
  }
}

TEST_CASE("Native3D engine initializes wall heights through owner transactions and protects edits", "[NativeFirstLayerEngine3D]") {
  World parent;const auto fixture=Box();const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),parent);const auto saved=Bytes(input);
  WallHeights heights;
  for(const auto& f:fixture.facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights[f.marker]=.1;
  for(int m:std::set<int>{1,parent.size,std::max(1,parent.size/2)}) {
    std::vector<std::vector<OwnedCell>> to(parent.size);
    for(const auto& row:input)to[row.first%m].push_back(row.second);
    const auto migrated=parent.exchange(to);MPI_Comm subset=MPI_COMM_NULL;
    MPI_Comm_split(parent.comm,parent.rank<m?0:MPI_UNDEFINED,parent.rank,&subset);
    Owned privateResult;bool consistent=true;int published=0;
    if(subset!=MPI_COMM_NULL) {
      World worker(subset);Owned owned;for(const auto& row:migrated)owned.emplace(row.cell.id,row);
      const auto originalBytes=Bytes(owned);
      EngineOptions options;options.wall_heights=heights;options.maximum_rounds=1;options.timings=true;
      auto result=Refine(worker,owned,Uniform(owned,{1,0,0,1,0,100}),reference,{},options);
      CHECK(Bytes(owned)==originalBytes);published=worker.sum(int(result.stats.layer_published));
      CHECK(published==2);CHECK(result.stats.layer_seconds>0);CHECK(worker.sum(int(result.stats.height_checks))>0);
      CHECK_FALSE((result.cells.empty() && !owned.empty()));
      for(const auto& row:result.cells) {
        FirstHeightReport report;std::string reason;
        if(!CheckFirstHeight(PatchOf({row.second}),heights,reference,report,reason))consistent=false;
      }
      privateResult=std::move(result.cells);MPI_Comm_free(&subset);
    }
    CHECK_FALSE(parent.maximum(!consistent));
    to.assign(parent.size,{});for(const auto& row:privateResult)to[row.first%parent.size].push_back(row.second);
    const auto returned=parent.exchange(to);Owned output;for(const auto& row:returned)output.emplace(row.cell.id,row);
    const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);
    CHECK(count>6);CHECK(Audit(parent,output,fixture,count));CHECK(Bytes(input)==saved);
  }
}

TEST_CASE("Native3D engine elects inconsistent wall-height policies before changing input", "[NativeFirstLayerEngine3D]") {
  World world;const auto fixture=Box();const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  EngineOptions options;options.maximum_rounds=1;
  if(world.size>1) {
    options.wall_heights[10]=world.rank?.1:.2;
    const auto rejected=Refine(world,input,Uniform(input,{}),reference,{},options);
    CHECK_FALSE(rejected.valid);CHECK(rejected.reason.find("policies differ")!=std::string::npos);
    CHECK(Bytes(input)==saved);
  }
  options.wall_heights[10]=std::numeric_limits<double>::quiet_NaN();
  const auto invalid=Refine(world,input,Uniform(input,{}),reference,{},options);
  CHECK_FALSE(invalid.valid);CHECK(invalid.reason.find("nonfinite")!=std::string::npos);CHECK(Bytes(input)==saved);
  options.wall_heights.clear();
  for(const auto& f:fixture.facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))options.wall_heights[f.marker]=2.;
  const auto impossible=Refine(world,input,Uniform(input,{}),reference,{},options);
  INFO(impossible.reason);
  CHECK_FALSE(impossible.valid);CHECK(impossible.reason.find("geometry-height incompatibility")!=std::string::npos);
  CHECK(world.sum(int(impossible.stats.layer_published))==0);CHECK(Bytes(input)==saved);
  auto unchanged=impossible.cells;
  for(auto& row:unchanged) {
    REQUIRE(input.count(row.first));const auto& prior=input.at(row.first);
    CHECK(row.second.sensor_present);
    for(const auto& sensor:row.second.sensor) {
      CHECK(sensor.xx==1);CHECK(sensor.yy==1);CHECK(sensor.zz==1);
      CHECK(sensor.xy==0);CHECK(sensor.xz==0);CHECK(sensor.yz==0);
    }
    // Bootstrap fills private sensor attributes before the height refusal; the mesh stays exact.
    row.second.sensor_present=prior.sensor_present;row.second.sensor=prior.sensor;
  }
  CHECK(Bytes(unchanged)==saved);
  for(auto& h:options.wall_heights)h.second=.1;
  options.maximum_work=1;
  const auto bounded=Refine(world,input,Uniform(input,{1,0,0,1,0,100}),reference,{},options);
  CHECK_FALSE(bounded.valid);CHECK(bounded.stats.stop==EngineStop::WorkBudget);
  CHECK(world.sum(int(bounded.stats.layer_published))==1);CHECK(Bytes(input)==saved);
  const auto count=CPassiveComm::Allreduce(bounded.cells.size(),CPassiveComm::Op::SUM,world.comm);
  CHECK(Audit(world,bounded.cells,fixture,count));
}

namespace {
#include "fixtures/native_bl_corner.inc"
#include "fixtures/native_bl_cold.inc"
#include "fixtures/native_bl_planar_wall.inc"
Patch SavedPrism(const char* filename) {
  std::ifstream in(filename);std::string magic;unsigned version=0;size_t cells=0,faces=0;
  in>>magic>>version>>cells>>faces;
  if(magic!="NATIVE_SAVED_PATCH_3D" || version!=1 || !cells || cells>4096 || faces>4*cells)
    throw std::runtime_error("Invalid saved prismatic patch header.");
  Patch patch;patch.cells.resize(cells);patch.surface.resize(faces);
  for(auto& c:patch.cells){in>>c.id;for(auto& n:c.v)in>>n.id>>n.p.x>>n.p.y>>n.p.z;}
  for(auto& f:patch.surface)in>>f.facet>>f.v[0]>>f.v[1]>>f.v[2];
  if(!in)throw std::runtime_error("Truncated saved prismatic patch.");
  in>>std::ws;if(!in.eof())throw std::runtime_error("Trailing saved prismatic patch records.");
  return patch;
}
void CoupledBL(bool grouped,unsigned replay=0,double strength=1,bool opposing=false,bool regional=false) {
  World world;auto fixture=Box();const auto donorFixture=fixture;
  const char* saved_patch=opposing?std::getenv("SU2_NATIVE_OPPOSING_PATCH"):nullptr;
  if(saved_patch)fixture.patch=SavedPrism(saved_patch);
  const char* selected_sensor=regional?std::getenv("SU2_NATIVE_PRISM_SENSOR"):nullptr;
  const std::string sensor=selected_sensor?selected_sensor:"SCALAR";
  const bool crossing=sensor=="CROSSING",tangential=sensor=="TANGENTIAL",rotating=sensor=="ROTATING";
  if(replay)fixture.patch=replay==2?BLColdObstruction():BLCornerObstruction();
  if(grouped)for(auto& f:fixture.facets)f.marker=std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;})?10:
      opposing && std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==1;})?12:11;
  const PlanarReference reference(fixture.facets);const std::string prefix=std::string(regional?"regional_":"")+std::string(crossing?"crossing_":tangential?"tangential_":rotating?"rotating_":"")+std::string(opposing?"opposing_":"")+std::string(strength>1?"finer_":"")+(replay==2?"cold_replay_":replay?"replay_":"")+std::string(grouped?"grouped_":"");
  auto input=Partition(Rows(fixture.patch),world);
  if(saved_patch)for(auto& row:input)row.second.version=0;
  const auto saved=input;
  auto original=Uniform(Partition(Rows(donorFixture.patch),world),{strength,0,0,strength,0,strength});
  auto allOriginal=Uniform(Partition(Rows(donorFixture.patch),World(MPI_COMM_SELF)),{strength,0,0,strength,0,strength});
  if(strength>1)for(auto* source:{&original,&allOriginal})for(auto& donor:*source)for(size_t k=0;k<4;++k) {
    const auto p=donor.cell.v[k].p;
    const auto value=strength*(1+3*p.x);
    donor.sensor[k]=crossing?Tensor{4,0,0,4,0,4+96*(1-std::abs(p.x-p.z))}:Tensor{value,0,0,value,0,value};
    auto rotated=[](double high,double sign){return Tensor{.75*high+1,sign*.25*std::sqrt(3.)*(high-4),0,.25*high+3,0,4};};
    if(tangential)donor.sensor[k]=rotated(256,1);
    if(rotating){const auto a=rotated(64,-1),b=rotated(256,1);donor.sensor[k]={(1-p.x)*a.xx+p.x*b.xx,(1-p.x)*a.xy+p.x*b.xy,0,(1-p.x)*a.yy+p.x*b.yy,0,4};}
  }
  const double first=opposing?.02:.1,thickness=opposing?.6:.3;
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",first,1.2,thickness};
  wall.coord={0,0,0,1,0,0,1,1,0,0,1,0};wall.conn={0,1,2,2,0,2,3,3};
  std::vector<CBoundaryLayerMetric::Wall> walls{wall};
  if(opposing){auto second=wall;second.layer.marker="opposite";for(size_t k=2;k<second.coord.size();k+=3)second.coord[k]=1;
    second.conn={0,2,1,1,0,3,2,2};walls.push_back(std::move(second));}
  CBoundaryLayerMetric layers(3,std::move(walls));
  const auto compose=[&](Point p,Tensor sensor) {
    CBoundaryLayerMetric::Tensor m;
    m.m[0][0]=sensor.xx;m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;
    m.m[1][1]=sensor.yy;m.m[1][2]=m.m[2][1]=sensor.yz;m.m[2][2]=sensor.zz;
    const su2double x[3]={p.x,p.y,p.z};layers.ApplyPoint(x,m,strength,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
        SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  EngineOptions options;options.timings=true;options.maximum_seconds=300;options.maximum_work=4096;
  options.edge_breakpoints=[&](Point a,Point b){return layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
  for(const auto& face:fixture.facets)
    if(std::all_of(face.v.begin(),face.v.end(),[](Node n){return n.p.z==0;}) ||
       (opposing && std::all_of(face.v.begin(),face.v.end(),[](Node n){return n.p.z==1;}))) {
      options.wall_heights[face.marker]=first;
      if(opposing || regional)options.region_walls[face.marker]={first,1.2,thickness};
    }
  auto exportMesh=[&](const Owned& owned,const std::string& name) {
    const auto rows=Gather(world,owned);if(world.rank)return;
    CSimplexMesh mesh;mesh.nDim=3;std::map<Id,unsigned long> nodes;std::map<int,size_t> markers;
    for(const auto& row:rows)for(const auto& node:row.cell.v) {
      const auto inserted=nodes.emplace(node.id,nodes.size());
      if(inserted.second){mesh.coord.push_back(node.p.x);mesh.coord.push_back(node.p.y);mesh.coord.push_back(node.p.z);}
      mesh.elem.push_back(inserted.first->second);
    }
    for(const auto& face:PatchOf(rows).surface) {
      const int marker=reference.Get(face.facet).marker;
      const auto inserted=markers.emplace(marker,markers.size());
      if(inserted.second)mesh.markers.push_back({"marker_"+std::to_string(marker),marker,{}});
      for(auto id:face.v)mesh.markers[inserted.first->second].elem.push_back(nodes.at(id));
    }
    simplex_test::WriteSU2Mesh(mesh,prefix+name+"_n"+std::to_string(world.size)+".su2");
  };
  Patch rejectedRegion;
  if(opposing || regional)options.rejected_trial=[&](const Patch&,const Patch& candidate,Feature,const MetricDiagnosis&,bool,const std::string&) {rejectedRegion=candidate;};
  exportMesh(input,saved_patch?"accepted_BL_input":replay?"saved_BL_input":"initial_euler");
  const auto refined=Refine(world,input,original,reference,compose,options);
  if(opposing || regional) {
    Owned rejected;for(auto row:Rows(rejectedRegion))rejected.emplace(row.cell.id,row);
    if(world.maximum(!rejected.empty())){exportMesh(rejected,"private_region_rejected");MeshRecords(prefix+"private_region_rejected",world,rejected);}
    const auto attempts=world.maximum(refined.stats.region_attempts),fronts=world.maximum(refined.stats.region_transition_fronts);
    const auto capTrials=world.maximum(refined.stats.region_surface_trials);const double capSeconds=CPassiveComm::Allreduce(refined.stats.region_cap_seconds,CPassiveComm::Op::MAX,world.comm);
    if(!world.rank)std::cout<<"PRISMATIC_RESULT valid="<<refined.valid<<" region_attempts="<<attempts<<" fronts="<<fronts<<" surface_trials="<<capTrials<<" cap_nested_s="<<capSeconds<<std::endl;
    CHECK(attempts==(saved_patch?0:1));
  }
  EngineReport(refined,world,prefix+"coupled_BL");Log(world,"BL result: "+refined.reason+"\n");
  exportMesh(refined.cells,refined.valid?"constructed_BL":"constructed_BL_rejected");
  CHECK(Bytes(input)==Bytes(saved));
  const auto count=CPassiveComm::Allreduce(refined.cells.size(),CPassiveComm::Op::SUM,world.comm);
  CHECK(Audit(world,refined.cells,fixture,count));
  FrozenField target(allOriginal,compose,false,{},options.edge_breakpoints);
  bool metric=true,height=true;std::string reason;
  for(const auto& row:refined.cells) {
    PatchMetric measured;FirstHeightReport report;
    metric=ValidateMetricPatch({row.second.cell},target,measured,reason) && metric;
    height=CheckFirstHeight(PatchOf({row.second}),options.wall_heights,reference,report,reason) && height;
  }
  MeshRecords(prefix+(refined.valid?"coupled_BL":"coupled_BL_rejected"),world,refined.cells);
  if(!refined.valid) {
    for(const auto& row:refined.cells) {
      const auto measured=MeasureMetricCells({row.second.cell},target);
      if(measured.minimum_mean_ratio<MinimumMeanRatio || measured.minimum_scaled_jacobian<MinimumScaledJacobian || measured.maximum_length>MaximumMetricEdge) {
        const auto center=Centroid(row.second.cell);std::ostringstream line;
        line << std::setprecision(17) << "BL_VIOLATION rank=" << world.rank << " cell=" << row.first
             << " q=" << measured.minimum_mean_ratio << " j=" << measured.minimum_scaled_jacobian
             << " length=" << double(measured.maximum_length) << " x=" << center.x << " y=" << center.y << " z=" << center.z << '\n';
        std::cout << line.str();
      }
    }
  }
  CHECK_FALSE(world.maximum(!height));
  INFO(refined.reason);REQUIRE(refined.valid);CHECK_FALSE(world.maximum(!metric));
  if(strength>1 && !opposing && !regional) {
    CHECK(world.sum(int(refined.stats.wall_reconstruction_trials))>0);
    CHECK(world.sum(int(refined.stats.wall_reconstruction_accepted))>0);
  }
  options.wall_heights.clear();options.region_walls.clear();options.edge_breakpoints={};
  const auto coarse=Coarsen(world,refined.cells,original,reference,{},options);
  EngineReport(coarse,world,prefix+"released_Euler");Log(world,"Euler result: "+coarse.reason+"\n");
  exportMesh(coarse.cells,coarse.valid?"released_euler":"released_euler_rejected");
  MeshRecords(prefix+(coarse.valid?"released_Euler":"released_Euler_rejected"),world,coarse.cells);
  INFO(coarse.reason);REQUIRE(coarse.valid);
  const auto coarseCount=CPassiveComm::Allreduce(coarse.cells.size(),CPassiveComm::Op::SUM,world.comm);
  CHECK(coarseCount<count);CHECK(Audit(world,coarse.cells,fixture,coarseCount));
  FrozenField euler(allOriginal);metric=true;
  for(const auto& row:coarse.cells){PatchMetric measured;metric=ValidateMetricPatch({row.second.cell},euler,measured,reason) && metric;}
  CHECK_FALSE(world.maximum(!metric));CHECK(Bytes(input)==Bytes(saved));
}
}
TEST_CASE("Native3D coupled geometric BL construction and Euler release reach complete gates", "[NativeCoupledBL3D]") {
  CoupledBL(false);
}
TEST_CASE("Native3D coarse cube with physical wall and far markers constructs and releases BL", "[NativeCoupledBLGrouped3D]") {
  CoupledBL(true);
}

TEST_CASE("Native engine discovers the saved ordinary-marker joint repair from immutable coarse donors", "[NativeBLCornerEngine3D]") {
  CoupledBL(true,true);
}

TEST_CASE("Native engine repairs the exact cold-start BL residual using immutable coarse donors", "[NativeBLColdEngine3D]") {
  CoupledBL(true,2);
}

TEST_CASE("Native composed BL startup preserves original geometry across MPI worker subsets", "[NativeCoupledBLBudget3D]") {
  World parent;const auto fixture=Box();const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),parent),saved=input;
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.1,1.2,.3};
  wall.coord={0,0,0,1,0,0,1,1,0,0,1,0};wall.conn={0,1,2,2,0,2,3,3};CBoundaryLayerMetric layers(3,{wall});
  const auto compose=[&](Point p,Tensor sensor) {
    CBoundaryLayerMetric::Tensor m;
    m.m[0][0]=sensor.xx;m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;
    m.m[1][1]=sensor.yy;m.m[1][2]=m.m[2][1]=sensor.yz;m.m[2][2]=sensor.zz;
    const su2double x[3]={p.x,p.y,p.z};layers.ApplyPoint(x,m,1.,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
        SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  for(int m:std::set<int>{1,parent.size,std::max(1,parent.size/2)}) {
    std::vector<std::vector<OwnedCell>> to(parent.size);for(const auto& row:input)to[row.first%m].push_back(row.second);
    const auto migrated=parent.exchange(to);MPI_Comm subset=MPI_COMM_NULL;
    MPI_Comm_split(parent.comm,parent.rank<m?0:MPI_UNDEFINED,parent.rank,&subset);
    Owned output;bool height=true,unchanged=true,stopped=true;int published=0;
    if(subset!=MPI_COMM_NULL) {
      World worker(subset);Owned owned;for(const auto& row:migrated)owned.emplace(row.cell.id,row);
      const auto initial=Bytes(owned);EngineOptions options;options.maximum_rounds=1;
      options.edge_breakpoints=[&](Point a,Point b){return layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
      for(const auto& f:fixture.facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))options.wall_heights[f.marker]=.1;
      const auto result=Refine(worker,owned,Uniform(owned,{1,0,0,1,0,1}),reference,compose,options);
      unchanged=Bytes(owned)==initial;published=worker.sum(int(result.stats.layer_published));
      stopped=result.valid || result.stats.stop==EngineStop::RoundBudget;
      for(const auto& row:result.cells){FirstHeightReport report;std::string reason;
        height=CheckFirstHeight(PatchOf({row.second}),options.wall_heights,reference,report,reason) && height;}
      CHECK(published==2);output=result.cells;MPI_Comm_free(&subset);
    }
    CHECK_FALSE(parent.maximum(!height));CHECK_FALSE(parent.maximum(!unchanged));CHECK_FALSE(parent.maximum(!stopped));
    to.assign(parent.size,{});for(const auto& row:output)to[row.first%parent.size].push_back(row.second);
    const auto returned=parent.exchange(to);output.clear();for(const auto& row:returned)output.emplace(row.cell.id,row);
    const auto count=CPassiveComm::Allreduce(output.size(),CPassiveComm::Op::SUM,parent.comm);
    CHECK(count>6);CHECK(Audit(parent,output,fixture,count));CHECK(Bytes(input)==Bytes(saved));
  }
}


TEST_CASE("Native3D finer original sensor reconstructs wall children and releases the layer", "[.][NativeFinerSensorBL3D]") {
  CoupledBL(true,0,4.);
}

TEST_CASE("Native3D overlapping opposing wall layers reach a complete finer-sensor mesh", "[.][NativeOpposingBL3D]") {
  CoupledBL(true,0,4.,true);
}

TEST_CASE("Native3D regional single-wall fade retains finer and crossing normal sensors", "[.][NativePrismaticSingleWall3D]") {
  CoupledBL(true,0,4.,false,true);
}

TEST_CASE("Prismatic admission certifies opposite caps and rejects slanted sides and wall junctions", "[NativePrismaticAdmission3D]") {
  auto fixture=Box();
  for(auto& f:fixture.facets)f.marker=std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;})?10:
      std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==1;})?12:11;
  const std::map<int,RegionWallPolicy> walls{{10,{.02,1.2,.6}},{12,{.02,1.2,.6}}};
  const PlanarReference geometry(fixture.facets);CHECK(PrismaticWallRegion(geometry,walls));
  CHECK_FALSE(PrismaticWallRegion(geometry,{}));
  auto junction=walls;junction.emplace(11,RegionWallPolicy{.02,1.2,.6});CHECK_FALSE(PrismaticWallRegion(geometry,junction));
  auto shear=fixture;for(auto& f:shear.facets)for(auto& n:f.v)n.p.x+=n.p.y;
  CHECK(PrismaticWallRegion(PlanarReference(shear.facets),walls));
  auto slanted=fixture;for(auto& f:slanted.facets)for(auto& n:f.v)n.p.x+=.25*n.p.z;
  CHECK_FALSE(PrismaticWallRegion(PlanarReference(slanted.facets),walls));
  Patch sentinel;sentinel.cells.push_back({123,{}});RegionReport report;RegionLimits limits;limits.maximum_cells=4096;limits.maximum_work=4096;
  auto incompatible=walls;for(auto& p:incompatible)p.second.height=.6;
  const RegionMetric metric([](Point){return Tensor{1,0,0,1,0,1};},[](Point a,Point b){return (long double)std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);});
  std::string reason;CHECK_FALSE(ConstructWallRegion(fixture.facets,{.5,.5,.5},geometry,incompatible,.04,SU2NativeFirstHeight::DefaultTolerance,8,6,metric,limits,sentinel,report,reason));
  CHECK(reason.find("geometry-height incompatibility")!=std::string::npos);CHECK(sentinel.cells.size()==1);CHECK(sentinel.cells.front().id==123);
  Patch retained;
  REQUIRE(ConstructWallRegion(fixture.facets,{.5,.5,.5},geometry,{{10,{.1,1.2,.3}}},.04,SU2NativeFirstHeight::DefaultTolerance,8,6,metric,limits,retained,report,reason));
  CHECK(retained.cells.size()==12);
  CHECK(report.surface_trials==1);  // Complete original cap/front fits; no lattice or boundary resampling.

}

TEST_CASE("Native first-row owners construct disjoint wall cells concurrently", "[NativeFirstLayerConcurrency3D][NativeFirstLayerBatch3D]") {
  World world;auto fixture=Box();
  // Disconnected boxes have independent complete vertex reservations even with the same wall marker policy.
  for(unsigned copy=1;copy<16;++copy) {
    auto extra=Box();
    for(auto& c:extra.patch.cells) {c.id+=100*copy;for(auto& n:c.v){n.id+=100*copy;n.p.x+=3*copy;}}
    for(auto& f:extra.patch.surface){f.facet+=100*copy;for(auto& id:f.v)id+=100*copy;}
    for(auto& f:extra.facets){f.id+=100*copy;for(auto& n:f.v){n.id+=100*copy;n.p.x+=3*copy;}}
    fixture.patch.cells.insert(fixture.patch.cells.end(),extra.patch.cells.begin(),extra.patch.cells.end());
    fixture.patch.surface.insert(fixture.patch.surface.end(),extra.patch.surface.begin(),extra.patch.surface.end());
    fixture.facets.insert(fixture.facets.end(),extra.facets.begin(),extra.facets.end());
  }
  const PlanarReference reference(fixture.facets);Owned input;
  for(const auto& row:Rows(fixture.patch))if((row.cell.id/100)%world.size==unsigned(world.rank))input.emplace(row.cell.id,row);
  const auto saved=Bytes(input);EngineOptions options;options.maximum_rounds=1;
  for(const auto& f:fixture.facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))options.wall_heights[f.marker]=.1;
  using Tet=std::array<std::array<double,3>,4>;std::vector<Tet> baseline;uint64_t baseline_batches=0;
  for(size_t batch:{size_t(1),MaximumCandidateBatch}) {
    options.candidate_batch=batch;options.maximum_work=32;options.timings=true;
    const auto result=Refine(world,input,Uniform(input,{1,0,0,1,0,100}),reference,{},options);
    CHECK(Bytes(input)==saved);CHECK_FALSE(result.valid);CHECK(result.stats.stop==EngineStop::WorkBudget);
    CHECK(result.stats.global_work==32);CHECK(world.sum(int(result.stats.layer_published))==32);
    CHECK(result.stats.layer_donor_batches==0);CHECK(result.stats.layer_shape_screens==0);
    CHECK(world.maximum(result.stats.edge_evaluations>0));
    std::vector<Tet> signature;
    for(const auto& row:result.cells) {FirstHeightReport report;std::string reason;
      CHECK(CheckFirstHeight(PatchOf({row.second}),options.wall_heights,reference,report,reason));
      Tet tet;for(unsigned j=0;j<4;++j){const auto p=row.second.cell.v[j].p;tet[j]={p.x,p.y,p.z};}
      std::sort(tet.begin(),tet.end());signature.push_back(tet);
    }
    std::sort(signature.begin(),signature.end());
    const auto batches=CPassiveComm::Allreduce(result.stats.layer_batches,CPassiveComm::Op::MAX,world.comm);
    if(batch==1){baseline=signature;baseline_batches=batches;}else {
      CHECK(signature==baseline);CHECK(batches<baseline_batches);
      CHECK(result.stats.layer_maximum_batch<=MaximumCandidateBatch);
    }
  }
}

TEST_CASE("Exact terminal planar wall patch diagnoses retained reconstruction", "[.][NativePlanarWallReplay3D]") {
  World world;REQUIRE(world.size==1);auto fixture=Box();
  for(auto& f:fixture.facets)f.marker=std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;})?10:11;
  const PlanarReference reference(fixture.facets);const auto old=BLPlanarWallObstruction();REQUIRE(old.cells.size()==18);
  auto donors=Uniform(Partition(Rows(fixture.patch),world),{4,0,0,4,0,4});
  for(auto& d:donors)for(size_t i=0;i<4;++i){const auto q=4+12*d.cell.v[i].p.x;d.sensor[i]={q,0,0,q,0,q};}
  CBoundaryLayerMetric::Wall wall;wall.layer={"wall",.1,1.2,.3};wall.coord={0,0,0,1,0,0,1,1,0,0,1,0};wall.conn={0,1,2,2,0,2,3,3};
  CBoundaryLayerMetric layers(3,{wall});
  MetricComposition compose=[&](Point p,Tensor sensor) {
    CBoundaryLayerMetric::Tensor m;m.m[0][0]=sensor.xx;m.m[1][1]=sensor.yy;m.m[2][2]=sensor.zz;
    const su2double x[3]={p.x,p.y,p.z};layers.ApplyPoint(x,m,4.,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
        SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  FrozenField target(donors,compose,false,{},[&](Point a,Point b){return layers.EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});});
  const auto working=BLPlanarWallWorking();
  const Edge edge{3,8399};std::map<Id,Node> nodes;Id maximum_node=0,maximum_cell=0;
  for(const auto& c:working.cells){maximum_cell=std::max(maximum_cell,c.id);for(auto n:c.v){nodes.emplace(n.id,n);maximum_node=std::max(maximum_node,n.id);}}
  const Node point{maximum_node+1,target.BalancedSplitPoint(nodes.at(edge[0]).p,nodes.at(edge[1]).p)};
  const WallHeights heights{{10,.1}};FirstHeightReport height;std::string reason;
  REQUIRE(CheckFirstHeight(old,heights,reference,height,reason));
  MeshRecords("planar_wall_replay_old",world,Partition(Rows(old),world));
  bool reconstructed=false;
  for(unsigned frame=0;frame<2;++frame) {
    Patch admitted=old;
    for(unsigned growth=0;growth<12;++growth) {
      Patch fresh;Face obstruction{};PatchMetric metric;MetricDiagnosis diagnosis;Id blocked=UINT64_MAX;
      const bool geometric=frame==0?ReconstructWallSplit(admitted,edge,point,maximum_node+2,maximum_cell+1,heights,reference,fresh,obstruction,reason):
          ReconstructSplitPatch(admitted,edge,point,{maximum_node+2,{}},target.Query(Centroid(admitted.cells.front())).target,maximum_cell+1,reference,fresh,obstruction,reason,nullptr,heights);
      bool valid=geometric && ValidateMetricChange(admitted.cells,fresh.cells,MetricChange::Refine,target,metric,reason,nullptr,nullptr,&diagnosis);
      std::cout<<std::setprecision(17)<<"PLANAR_WALL_REPLAY frame="<<frame<<" growth="<<growth<<" old="<<admitted.cells.size()<<" geometry="<<geometric<<" valid="<<valid
          <<" cells="<<fresh.cells.size()<<" q="<<metric.minimum_mean_ratio<<" j="<<metric.minimum_scaled_jacobian
          <<" failure="<<unsigned(diagnosis.failure)<<" removed="<<diagnosis.removed_excess<<" added="<<diagnosis.added_excess
          <<" obstruction="<<obstruction[0]<<','<<obstruction[1]<<','<<obstruction[2]<<" reason="<<reason<<std::endl;
      if(geometric && !valid && diagnosis.failure==MetricFailure::Shape) {
        Patch repaired;ConnectivityRepairReport report;
        RepairMetricConnectivity(fresh,target,reference,heights,maximum_cell+1,repaired,report,reason,0);blocked=report.blocked_interior_vertex;
        if(blocked==UINT64_MAX) {
          const bool progress=RepairMetricConnectivity(fresh,target,reference,heights,maximum_cell+1,repaired,report,reason,256);
          blocked=report.blocked_interior_vertex;
          std::cout<<"PLANAR_WALL_REPAIR frame="<<frame<<" growth="<<growth<<" progress="<<progress<<" complete="<<report.complete_shape<<" trials="<<report.trials
              <<" removals="<<report.removals<<" flips="<<report.flips<<" moves="<<report.moves<<" blocked="<<blocked<<" reason="<<reason<<std::endl;
          if(progress) {
            valid=ValidateMetricChange(admitted.cells,repaired.cells,MetricChange::Refine,target,metric,reason,nullptr,nullptr,&diagnosis);
            std::cout<<"PLANAR_WALL_REPAIRED valid="<<valid<<" q="<<metric.minimum_mean_ratio<<" j="<<metric.minimum_scaled_jacobian
                <<" failure="<<unsigned(diagnosis.failure)<<" removed="<<diagnosis.removed_excess<<" added="<<diagnosis.added_excess<<" reason="<<reason<<std::endl;
            if(!valid && report.complete_shape && report.removals && diagnosis.failure==MetricFailure::Progress && report.trials<256) {
              Patch preserved;ConnectivityRepairReport alternate;
              const bool retried=RepairMetricConnectivity(fresh,target,reference,heights,maximum_cell+1,preserved,alternate,reason,256-report.trials,false);
              std::cout<<"PLANAR_WALL_PRESERVE progress="<<retried<<" complete="<<alternate.complete_shape<<" trials="<<alternate.trials<<" flips="<<alternate.flips<<" moves="<<alternate.moves<<" reason="<<reason<<std::endl;
              if(retried){repaired=std::move(preserved);valid=ValidateMetricChange(admitted.cells,repaired.cells,MetricChange::Refine,target,metric,reason,nullptr,nullptr,&diagnosis);}
              std::cout<<"PLANAR_WALL_ALTERNATE valid="<<valid<<" q="<<metric.minimum_mean_ratio<<" j="<<metric.minimum_scaled_jacobian
                  <<" failure="<<unsigned(diagnosis.failure)<<" removed="<<diagnosis.removed_excess<<" added="<<diagnosis.added_excess<<" reason="<<reason<<std::endl;
              blocked=alternate.blocked_interior_vertex;
            }
            fresh=std::move(repaired);
          }
        } else std::cout<<"PLANAR_WALL_HINT vertex="<<blocked<<std::endl;
      }
      if(geometric)MeshRecords("planar_wall_replay_frame"+std::to_string(frame)+"_growth"+std::to_string(growth),world,Partition(Rows(fresh),world));
      if(valid){reconstructed=true;std::cout<<"PLANAR_WALL_SUCCESS frame="<<frame<<" growth="<<growth<<std::endl;break;}
      std::set<Id> ids;for(const auto& c:admitted.cells)ids.insert(c.id);const auto previous=ids.size();
      for(const auto& c:working.cells)if(!ids.count(c.id)) {
        unsigned count=0;for(auto n:c.v)count+=std::find(obstruction.begin(),obstruction.end(),n.id)!=obstruction.end();
        const bool vertex=blocked!=UINT64_MAX && std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==blocked;});
        if(vertex || (blocked==UINT64_MAX && obstruction!=Face{} && count==3)){ids.insert(c.id);admitted.cells.push_back(c);}
      }
      if(ids.size()==previous || ids.size()>MaximumCavityCells)break;
      const auto boundary=Incidence(admitted.cells).Boundary();admitted.surface.clear();
      for(const auto& f:working.surface)if(std::any_of(boundary.begin(),boundary.end(),[&](Face q){return Key(q)==Key(f.v);}))admitted.surface.push_back(f);
    }
  }
  REQUIRE(reconstructed);
}


TEST_CASE("Regional field service resolves large distributed sources and closes after refusal", "[NativeRegionField3D]") {
  World world;const auto seed=Box();const unsigned n=5,side=n+1;
  std::vector<DonorCell> original;Id next=0;
  for(unsigned z=0;z<n;++z)for(unsigned y=0;y<n;++y)for(unsigned x=0;x<n;++x)
    for(const auto& base:seed.patch.cells){
      DonorCell d;d.cell.id=next++;
      for(unsigned k=0;k<4;++k){const auto p=base.v[k].p;
        const unsigned i=x+unsigned(p.x),j=y+unsigned(p.y),l=z+unsigned(p.z);
        d.cell.v[k]={i+side*(j+side*l),{double(i)/n,double(j)/n,double(l)/n}};
        const double value=1+d.cell.v[k].p.z;d.sensor[k]={value,0,0,value,0,value};
      }
      if(d.cell.id%world.size==unsigned(world.rank))original.push_back(d);
    }
  REQUIRE(next>MaximumDonors);DonorImport source(world);std::string reason;REQUIRE(source.Build(original,reason));
  PlanarReference geometry(seed.facets);size_t compositions=0;
  auto compose=[&](Point p,Tensor sensor){++compositions;sensor.xx+=4*p.y*p.y;return sensor;};
  RegionFieldService points(world,source,geometry,compose,{},false,SIZE_MAX,0);
  CLocalFailure failure;
  try{points.Run([&](const RegionMetric& metric){
    const Point p{.18,.22,.34};const auto a=metric.point(p),b=metric.point(p);
    if(std::abs(a.xx-(1+p.z+4*p.y*p.y))>1e-13 || a.xx!=b.xx || std::abs(a.zz-(1+p.z))>1e-13 || compositions!=2)
      throw std::runtime_error("Regional point memo changed sensor interpolation or skipped actual composition.");
  });}catch(const std::exception& e){failure.Set(1,0,e.what());}
  const auto point_error=world.elect(failure);INFO(point_error.message);REQUIRE_FALSE(point_error.any);
  if(!world.rank){CHECK(points.Statistics().sensor_hits==1);CHECK(points.Statistics().maximum_resident<=MaximumDonors);}
  RegionFieldService edges(world,source,geometry,{}, {},false,SIZE_MAX,0);
  failure={};
  try{edges.Run([&](const RegionMetric& metric){
    const auto length=metric.edge({0,0,0},{1,1,1});const auto expected=std::sqrt(3.L)*2/3*(2*std::sqrt(2.L)-1);
    if(std::abs(length-expected)>1e-12L || metric.edge({1,1,1},{0,0,0})!=length)
      throw std::runtime_error("Regional exact segment source does not match the analytic affine sensor integral.");
  });}catch(const std::exception& e){failure.Set(1,0,e.what());}
  const auto edge_error=world.elect(failure);INFO(edge_error.message);REQUIRE_FALSE(edge_error.any);
  if(!world.rank){CHECK(edges.Statistics().edge_hits==1);CHECK(edges.Statistics().maximum_resident<=MaximumDonors);}
  CHECK(source.Statistics().maximum_patch<=MaximumDonors);
  RegionFieldService denied(world,source,geometry,{}, {},false,SIZE_MAX,0);failure={};
  try{denied.Run([&](const RegionMetric& metric){metric.point({-1,-1,-1});});}
  catch(const std::exception& e){failure.Set(1,0,e.what());}
  CHECK(world.elect(failure).any);
  RegionFieldService resumed(world,source,geometry,{}, {},false,SIZE_MAX,0);failure={};
  try{resumed.Run([&](const RegionMetric& metric){if(std::abs(metric.point({.2,.3,.4}).yy-1.4)>1e-13)throw std::runtime_error("Protocol did not recover after refusal.");});}
  catch(const std::exception& e){failure.Set(1,0,e.what());}
  CHECK_FALSE(world.elect(failure).any);
}

TEST_CASE("Native MPI point probes distinguish complete outside searches from failed imports", "[NativeDistributed3D][NativeDonorPointProbe3D]") {
  World world(SU2_MPI::GetComm());const auto fixture=Box();std::vector<DonorCell> original;
  for(const auto& cell:fixture.patch.cells)if((cell.id+1)%world.size==unsigned(world.rank)) {
    DonorCell row;row.cell=cell;row.sensor.fill({1,0,0,1,0,1});original.push_back(row);
  }
  DonorImport source(world);std::string reason;REQUIRE(source.Build(original,reason));
  const auto outside=source.ProbePoints(world.rank==0?std::vector<Point>{{2,2,2}}:std::vector<Point>{});
  REQUIRE(outside.valid);CHECK(outside.cells.empty());
  const auto strict=source.ImportSegments(world.rank==0?std::vector<std::array<Point,2>>{{Point{2,2,2},Point{2,2,2}}}:std::vector<std::array<Point,2>>{});
  CHECK_FALSE(strict.valid);
  const Point point{.23,.31,.41};
  const auto inside=source.ProbePoints(world.rank==world.size-1?std::vector<Point>{point}:std::vector<Point>{});
  REQUIRE(inside.valid);REQUIRE(inside.cells.size()==inside.source_rank.size());
  for(size_t i=0;i<inside.cells.size();++i)CHECK(inside.source_rank[i]==int((inside.cells[i].cell.id+1)%world.size));
  if(world.rank==world.size-1) {
    CHECK_FALSE(inside.cells.empty());
    for(const auto& row:inside.cells){std::array<long double,4> weights{};const auto& v=row.cell.v;
      CHECK(Barycentric({v[0].p,v[1].p,v[2].p,v[3].p},point,weights));}
  }else CHECK(inside.cells.empty());
  const auto refused=source.ProbePoints(world.rank==0?std::vector<Point>{{2,2,2}}:std::vector<Point>{},1);
  CHECK_FALSE(refused.valid);
  const auto invalid=source.ProbePoints(world.rank==world.size-1?std::vector<Point>{{NAN,0,0}}:std::vector<Point>{});
  CHECK_FALSE(invalid.valid);
}


TEST_CASE("Owner-routed point samples preserve canonical physical and extension priorities", "[NativePointSamples3D]") {
  World world;const auto geometry=Box();std::vector<DonorCell> all,local;
  for(auto cell:geometry.patch.cells) {
    DonorCell row;row.cell=cell;
    for(unsigned j=0;j<4;++j) {
      row.cell.v[j].id+=100;const auto p=row.cell.v[j].p;
      row.sensor[j]={2+.25*p.x,.03125*p.x,0,3+.25*p.y,0,4+.25*p.z};
    }all.push_back(row);
  }
  for(unsigned k=0;k<2;++k) {
    DonorCell row;row.cell=all.front().cell;row.cell.id=1000+k;row.extension_component=777;
    for(unsigned j=0;j<4;++j){row.cell.v[j].id=4*k+j;if(k)row.cell.v[j].p.x+=2;row.sensor[j]={16,0,0,16,0,16};}
    all.push_back(row);
  }
  for(const auto& row:all)if((row.cell.id+1)%world.size==unsigned(world.rank))local.push_back(row);
  DonorImport service(world,true);std::string reason;REQUIRE(service.Build(local,reason));
  const auto bytes=service.ResidentBytes();
  const auto& ext=all.back().cell;const Point outside=Centroid(ext);
  const std::vector<Point> points{{0,0,0},{.5,.5,.5},{.23,.31,.41},outside};
  const auto result=service.SamplePoints(world.rank==0?points:std::vector<Point>{},world.rank==0?std::vector<Id>{777}:std::vector<Id>{});
  INFO(result.reason);REQUIRE(result.valid);
  if(!world.rank) {
    FrozenField reference(all,{},false,{777});REQUIRE(result.samples.size()==points.size());
    for(size_t i=0;i<points.size();++i) {
      const auto expected=reference.Query(points[i]);const auto& actual=result.samples[i];
      CHECK(actual.Key()==expected.donor);CHECK(actual.weights==expected.weights);
      CHECK(actual.extension_component==expected.extension_component);
      CHECK(actual.source_rank==int((actual.cell+1)%world.size));
      const std::array<double,6> a{actual.sensor.xx,actual.sensor.xy,actual.sensor.xz,actual.sensor.yy,actual.sensor.yz,actual.sensor.zz},
          b{expected.sensor.xx,expected.sensor.xy,expected.sensor.xz,expected.sensor.yy,expected.sensor.yz,expected.sensor.zz};CHECK(a==b);
    }
    CHECK(result.samples[0].extension_component==UINT64_MAX);
    CHECK(result.samples.back().extension_component==777);
  }else CHECK(result.samples.empty());
  const auto missing=service.SamplePoints(world.rank==world.size-1?std::vector<Point>{outside}:std::vector<Point>{});
  CHECK_FALSE(missing.valid);CHECK(missing.samples.empty());
  const auto invalid=service.SamplePoints(world.rank==0?std::vector<Point>{{NAN,0,0}}:std::vector<Point>{});
  CHECK_FALSE(invalid.valid);CHECK(invalid.samples.empty());
  const auto refused=service.SamplePoints({}, {},world.rank==0?1:SIZE_MAX);
  CHECK_FALSE(refused.valid);CHECK(refused.samples.empty());CHECK(service.ResidentBytes()==bytes);
}

TEST_CASE("Known metric support never fabricates missing centroid or edge coverage", "[NativeMetricSamples3D]") {
  World world;std::vector<DonorCell> local;
  for(unsigned k=0;k<2;++k) {
    DonorCell d;d.cell={k,{{{4*k,{3.*k,0,0}},{4*k+1,{3.*k+1,0,0}},{4*k+2,{3.*k,1,0}},{4*k+3,{3.*k,0,1}}}}};
    d.sensor.fill({1,0,0,1,0,1});if((k+1)%world.size==unsigned(world.rank))local.push_back(d);
  }
  DonorImport source(world);std::string reason;REQUIRE(source.Build(local,reason));const auto held=source.ResidentBytes();
  const Cell query{9,{{{100,{0,0,0}},{101,{3,0,0}},{102,{0,1,0}},{103,{0,0,1}}}}};
  const auto cells=world.rank==world.size-1?std::vector<Cell>{query}:std::vector<Cell>{};
  const auto sampled=source.ImportMetricSamples(cells);INFO(sampled.reason);REQUIRE(sampled.valid);
  if(world.rank==world.size-1) {
    FrozenField field(sampled.cells);CHECK_THROWS(field.Query(Centroid(query)));
    CHECK_THROWS(MeasureMetricCells({query},field));CHECK_THROWS(field.TargetEdgeLength(query.v[0].p,query.v[1].p));
  }
  const auto strict=source.SamplePoints(world.rank==world.size-1?std::vector<Point>{Centroid(query)}:std::vector<Point>{});
  CHECK_FALSE(strict.valid);CHECK(strict.samples.empty());CHECK(source.ResidentBytes()==held);
}

TEST_CASE("Surviving collapse-cell constraints follow shape admission policy", "[NativeMetricPlan3D][NativeShapeAdmission3D]") {
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::Absolute);
  const auto fixture=Box(.1);const PlanarReference geometry(fixture.facets);auto row=Rows(fixture.patch).front();
  WallHeights heights;bool wall=false;Node apex;Point center{};
  const auto faces=Faces(row.cell);
  for(unsigned f=0;f<4;++f) {
    bool bottom=true;for(auto id:faces[f])for(auto n:row.cell.v)if(n.id==id)bottom=bottom && n.p.z==0;
    if(!bottom){row.physical[f].present=false;continue;}
    REQUIRE(row.physical[f].present);wall=true;heights[geometry.Get(row.physical[f].facet).marker]=.1;
    apex=row.cell.v[f];for(auto id:faces[f])for(auto n:row.cell.v)if(n.id==id){center.x+=n.p.x/3;center.y+=n.p.y/3;}
  }
  REQUIRE(wall);Node retained{900,{center.x,center.y,.1}};
  CHECK_FALSE(CollapseConstraintViolation(row,apex.id,retained,geometry,0,heights));
  retained.p.z=.2;CHECK(CollapseConstraintViolation(row,apex.id,retained,geometry,0,heights));
  CHECK_FALSE(CollapseConstraintViolation(row,apex.id,retained,geometry,0,{}));
  retained.p={-1,-1,.1};CHECK(CollapseConstraintViolation(row,apex.id,retained,geometry,0,heights));
  CHECK_FALSE(CollapseConstraintViolation(row,99999,retained,geometry,0,heights));
  for(auto n:row.cell.v)if(n.id!=apex.id){CHECK_FALSE(CollapseConstraintViolation(row,apex.id,n,geometry,0,heights));break;}

  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  retained.p={center.x,center.y,.2};CHECK_FALSE(CollapseConstraintViolation(row,apex.id,retained,geometry,0,heights));
  retained.p={center.x+2,center.y,.1};CHECK_FALSE(CollapseConstraintViolation(row,apex.id,retained,geometry,0,heights));
  retained.p={center.x,center.y,0};CHECK(CollapseConstraintViolation(row,apex.id,retained,geometry,0,heights));
  auto malformed=heights;malformed.tolerance=std::numeric_limits<double>::quiet_NaN();
  retained.p={center.x,center.y,.1};CHECK(CollapseConstraintViolation(row,apex.id,retained,geometry,0,malformed));
  malformed=heights;malformed.begin()->second=std::numeric_limits<double>::quiet_NaN();
  CHECK(CollapseConstraintViolation(row,apex.id,retained,geometry,0,malformed));
}

TEST_CASE("Masked metric imports retain actual point support and validate request boundaries", "[NativeMetricPlan3D]") {
  World world;const auto fixture=Box();const auto rows=Partition(Rows(fixture.patch),world);DonorImport source(world);std::string reason;
  REQUIRE(source.Build(Uniform(rows,{1,0,0,1,0,1}),reason));auto cell=fixture.patch.cells.front();
  const auto queries=world.rank==world.size-1?std::vector<Cell>{cell}:std::vector<Cell>{};
  const auto masks=world.rank==world.size-1?std::vector<unsigned char>{0}:std::vector<unsigned char>{};
  const auto before=source.Statistics();const auto support=source.ImportMetricSamples(queries,{},SIZE_MAX,0,masks);
  INFO(support.reason);REQUIRE(support.valid);CHECK(source.Statistics().metric_edge_queries==before.metric_edge_queries);
  CHECK(source.Statistics().metric_edge_skipped-before.metric_edge_skipped==(world.rank==world.size-1?6:0));
  if(world.rank==world.size-1) {
    FrozenField point(support.cells);CHECK(point.Query(Centroid(cell)).sensor.xx==1);
    DonorCell d;d.cell=cell;d.sensor.fill({1,0,0,1,0,1});auto id=std::make_shared<const FrozenMetricIdentity>();
    FrozenField full({d},{},false,{}, {},id);const auto measured=MeasureMetricCells({cell},full);
    FrozenField narrow(support.cells,{},false,{}, {},id);MetricStats counted;
    MeasureMetricCells({cell},narrow,&counted,&measured.edges);CHECK(counted.edge_reused==6);CHECK(counted.edge_evaluations==0);
  }
  const auto duplicated=world.rank==world.size-1?std::vector<Cell>{cell,cell}:std::vector<Cell>{};
  const auto full_before=source.Statistics();REQUIRE(source.ImportMetricSamples(duplicated).valid);
  CHECK(source.Statistics().metric_edge_queries-full_before.metric_edge_queries==(world.rank==world.size-1?6:0));
  CHECK(source.Statistics().metric_edge_duplicates-full_before.metric_edge_duplicates==(world.rank==world.size-1?6:0));
  CHECK(source.Statistics().metric_edge_queries+source.Statistics().metric_edge_skipped+source.Statistics().metric_edge_duplicates==6*source.Statistics().metric_sample_cells);
  const auto invalid=world.rank==0?std::vector<unsigned char>{64}:std::vector<unsigned char>{};
  CHECK_FALSE(source.ImportMetricSamples(world.rank==0?std::vector<Cell>{cell}:std::vector<Cell>{},{},SIZE_MAX,0,invalid).valid);
  CHECK_FALSE(source.ImportMetricSamples(queries,{},SIZE_MAX,0,world.rank==world.size-1?std::vector<unsigned char>{0,0}:std::vector<unsigned char>{}).valid);
}

TEST_CASE("Canonical ideal-midpoint samples retain physical priority and exact boundary weights", "[NativeMidpointSamples3D]") {
  World world;DonorCell physical;physical.cell.id=74753;
  physical.cell.v={{{6758,{.5683152985314954,.07855169036323718,.02781832897848278}},
      {6804,{.5539589887874311,.06659188433438007,.02873117516934826}},
      {39146,{.5661287222999969,.08810439336809825,.05224913251574465}},
      {60068,{.573893966318129,.06987809295852927,.04422988413910242}}}};
  const auto a=physical.cell.v[0].p,b=physical.cell.v[1].p;
  if(Orientation(physical.cell.v[0].p,physical.cell.v[1].p,physical.cell.v[2].p,physical.cell.v[3].p)<0)std::swap(physical.cell.v[0],physical.cell.v[1]);
  for(unsigned j=0;j<4;++j){const auto p=physical.cell.v[j].p;physical.sensor[j]={2+p.x,.01,0,3+p.y,0,4+p.z};}
  DonorCell extension=physical;extension.cell.id=0;extension.extension_component=777;
  for(unsigned j=0;j<4;++j){extension.cell.v[j].id=j;extension.sensor[j]={32,0,0,32,0,32};}
  std::vector<DonorCell> all{physical,extension},local;
  for(const auto& d:all)if((d.cell.id+1)%world.size==unsigned(world.rank))local.push_back(d);
  DonorImport source(world,true);std::string reason;REQUIRE(source.Build(local,reason));const auto held=source.ResidentBytes();
  const std::vector<std::array<Point,2>> query{{a,b},{b,a},{a,a}};
  const auto result=source.SampleMidpoints(world.rank==0?query:std::vector<std::array<Point,2>>{},world.rank==0?std::vector<Id>{777}:std::vector<Id>{});
  INFO(result.reason);REQUIRE(result.valid);
  if(!world.rank) {
    FrozenField expected(all,{},false,{777});REQUIRE(result.samples.size()==query.size());
    for(size_t i=0;i<query.size();++i) {
      const auto ref=expected.QueryMidpoint(query[i][0],query[i][1]);const auto& actual=result.samples[i];
      CHECK(actual.Key()==ref.donor);CHECK(actual.weights==ref.weights);CHECK(actual.extension_component==UINT64_MAX);
      CHECK(actual.sensor.xx==ref.sensor.xx);CHECK(actual.sensor.yy==ref.sensor.yy);CHECK(actual.sensor.zz==ref.sensor.zz);
    }
    const Point rounded{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
    FrozenField strict({physical});CHECK_THROWS(strict.Query(rounded));
  }else CHECK(result.samples.empty());
  const auto bad=source.SampleMidpoints(world.rank==world.size-1?std::vector<std::array<Point,2>>{{a,{NAN,0,0}}}:std::vector<std::array<Point,2>>{});
  CHECK_FALSE(bad.valid);CHECK(bad.samples.empty());
  CHECK_FALSE(source.SampleMidpoints(std::vector<std::array<Point,2>>(MaximumCavityCells+1)).valid);
  CHECK_FALSE(source.SampleMidpoints({}, {},world.rank==0?1:SIZE_MAX).valid);
  CHECK(source.ResidentBytes()==held);
  const auto missing=source.SampleMidpoints(world.rank==0?std::vector<std::array<Point,2>>{{Point{4,0,0},Point{6,0,0}}}:std::vector<std::array<Point,2>>{});
  CHECK_FALSE(missing.valid);CHECK(missing.samples.empty());
}

TEST_CASE("Canonical owner point samples do not import large incident donor stars", "[NativePointSamples3D]") {
  World world;std::vector<DonorCell> local;Id next=0;
  std::array<std::array<Node,13>,13> grid;
  for(unsigned j=0;j<=12;++j)for(unsigned i=0;i<=12;++i)grid[j][i]={1+j*13+i,{double(i)/12,double(j)/12,1}};
  auto add=[&](std::array<Node,4> nodes) {
    DonorCell row;row.cell={next++,nodes};row.sensor.fill({2,0,0,2,0,2});
    if(Orientation(nodes[0].p,nodes[1].p,nodes[2].p,nodes[3].p)<0)std::swap(row.cell.v[0],row.cell.v[1]);
    if((row.cell.id+1)%world.size==unsigned(world.rank))local.push_back(row);
  };
  for(unsigned j=0;j<12;++j)for(unsigned i=0;i<12;++i) {
    const Node apex{0,{0,0,0}};
    add({apex,grid[j][i],grid[j][i+1],grid[j+1][i]});
    add({apex,grid[j+1][i+1],grid[j+1][i],grid[j][i+1]});
  }
  REQUIRE(next>MaximumDonors);DonorImport service(world,true);std::string reason;REQUIRE(service.Build(local,reason));
  const auto points=world.rank==0?std::vector<Point>{{0,0,0}}:std::vector<Point>{};
  const auto full=service.ProbePoints(points);CHECK_FALSE(full.valid);
  const auto sampled=service.SamplePoints(points);INFO(sampled.reason);REQUIRE(sampled.valid);
  if(!world.rank) {
    REQUIRE(sampled.samples.size()==1);CHECK(sampled.samples[0].cell==0);
    CHECK(sampled.samples[0].sensor.xx==2);CHECK(sampled.samples[0].source_rank==int(1%world.size));
  }else CHECK(sampled.samples.empty());
  const Point origin{},end{.01,.02,.5};
  const auto segments=world.rank==0?std::vector<std::array<Point,2>>{{origin,end}}:std::vector<std::array<Point,2>>{};
  CHECK_FALSE(service.ImportSegments(segments).valid);
  const auto integral=service.ImportEdgeSegments(segments);INFO(integral.reason);REQUIRE(integral.valid);
  if(!world.rank) {
    REQUIRE(integral.cells.size()==1);FrozenField field(integral.cells);
    CHECK(field.TargetEdgeLength(origin,end)==Approx(std::sqrt(2*(.01*.01+.02*.02+.5*.5))).epsilon(1e-12));
  }
  Cell query{9000,{{{9001,origin},{9002,{.01,0,.5}},{9003,{0,.01,.5}},{9004,{.01,.01,.5}}}}};
  if(Orientation(query.v[0].p,query.v[1].p,query.v[2].p,query.v[3].p)<0)std::swap(query.v[0],query.v[1]);
  const auto known=world.rank==0?std::vector<Cell>{query}:std::vector<Cell>{};
  CHECK_FALSE(service.ImportMetricCells(known).valid);
  const auto measured=service.ImportMetricSamples(known);INFO(measured.reason);REQUIRE(measured.valid);
  if(!world.rank) {
    REQUIRE(measured.cells.size()<MaximumDonors);FrozenField field(measured.cells);DonorCell d;d.cell=query;d.sensor.fill({2,0,0,2,0,2});FrozenField reference({d});
    const auto actual=MeasureMetricCells({query},field),expected=MeasureMetricCells({query},reference);
    CHECK(actual.minimum_mean_ratio==expected.minimum_mean_ratio);CHECK(actual.minimum_scaled_jacobian==expected.minimum_scaled_jacobian);
    CHECK(actual.edges.size()==expected.edges.size());for(const auto& edge:actual.edges)CHECK(edge.second.length==Approx(expected.edges.at(edge.first).length).epsilon(1e-12));
  }
  CHECK_FALSE(service.ImportMetricSamples(known,{},1).valid);
  CHECK_FALSE(service.ImportMetricSamples(std::vector<Cell>(MaximumCavityCells/6+1)).valid);
  CHECK(service.ImportMetricSamples({}).valid);
  CHECK_FALSE(service.ImportEdgeSegments(world.rank==0?std::vector<std::array<Point,2>>{{origin,origin}}:std::vector<std::array<Point,2>>{}).valid);
  CHECK_FALSE(service.ImportEdgeSegments(world.rank==0?std::vector<std::array<Point,2>>{{origin,Point{-1,-1,-1}}}:std::vector<std::array<Point,2>>{}).valid);
  PlanarReference geometry(std::vector<Facet>{});size_t composed=0;
  RegionFieldService regional(world,service,geometry,[&](Point p,Tensor sensor){++composed;sensor.xx+=p.z*p.z;return sensor;},{},false,SIZE_MAX,0);
  CLocalFailure failure;
  try{regional.Run([&](const RegionMetric& metric){
    if(metric.point(origin).xx!=2 || metric.point(origin).xx!=2 || composed!=2 || std::abs(metric.point(end).xx-2.25)>1e-14)
      throw std::runtime_error("Large-star canonical point or actual-query composition changed.");
  });}catch(const std::exception& e){failure.Set(1,0,e.what());}
  INFO(failure.message);CHECK_FALSE(world.elect(failure).any);
  if(!world.rank){CHECK(regional.Statistics().sampled_points==2);CHECK(regional.Statistics().sensor_hits==1);}
  RegionFieldService edges(world,service,geometry,[](Point p,Tensor sensor){sensor.zz+=p.z;return sensor;},
      [](Point,Point){return std::vector<double>{0,.25,.5,.75,1};},false,SIZE_MAX,0);failure={};
  try{edges.Run([&](const RegionMetric& metric){
    const auto value=metric.edge(origin,end);const long double a=.501L,b=.125L;
    const auto expected=2/(3*b)*(std::pow(a+b,1.5L)-std::pow(a,1.5L));
    if(std::abs(value-expected)>1e-12L || metric.edge(end,origin)!=value)
      throw std::runtime_error("Large-star integral changed or reversed edge was not reused.");
  });}catch(const std::exception& e){failure.Set(1,0,e.what());}
  const auto edge_error=world.elect(failure);INFO(edge_error.message);CHECK_FALSE(edge_error.any);
  RegionFieldService denied(world,service,geometry,{}, {},false,world.rank==0?1:SIZE_MAX,0);failure={};
  try{denied.Run([&](const RegionMetric& metric){metric.point(origin);});}catch(const std::exception& e){failure.Set(1,0,e.what());}
  CHECK(world.elect(failure).any);
}

TEST_CASE("Metric donor intervals still reject genuinely oversized positive footprints", "[NativeMetricFootprint3D]") {
  World world;std::vector<DonorCell> local;const Id count=MaximumDonors+1;
  for(Id i=0;i<count;++i) {
    DonorCell d;d.cell={i,{{{i,{double(i),0,0}},{i+1,{double(i+1),0,0}},{count+1+2*i,{double(i),1,0}},{count+2+2*i,{double(i),0,1}}}}};
    d.sensor.fill({1,0,0,1,0,1});if(i%world.size==unsigned(world.rank))local.push_back(d);
  }
  DonorImport source(world);std::string reason;REQUIRE(source.Build(local,reason));
  const auto result=source.ImportEdgeSegments(world.rank==0?std::vector<std::array<Point,2>>{{Point{},Point{double(count),0,0}}}:std::vector<std::array<Point,2>>{});
  CHECK_FALSE(result.valid);CHECK(result.cells.empty());CHECK_FALSE(result.reason.empty());
  Cell query{count+7,{{{count+10,{0,0,0}},{count+11,{double(count),0,0}},{count+12,{0,1,0}},{count+13,{0,0,1}}}}};
  const auto sampled=source.ImportMetricSamples(world.rank==0?std::vector<Cell>{query}:std::vector<Cell>{});
  CHECK_FALSE(sampled.valid);CHECK(sampled.cells.empty());CHECK(sampled.reason.find("patch budget")!=std::string::npos);
}


TEST_CASE("Original point sample transport retains full binary barycentric precision", "[NativePointSamples3D]") {
  DonorSample original;original.weights={std::nextafter(1.L,0.L),std::numeric_limits<long double>::denorm_min(),1.L/3,0};
  auto expected=original;std::vector<char> bytes;RecordStream encode(bytes);encode(original);
  DonorSample decoded;const auto& input=bytes;RecordStream decode(input);decode(decoded);
  CHECK(decode.End());CHECK(original.weights==expected.weights);CHECK(decoded.weights==expected.weights);
}


TEST_CASE("Interior reservations retain shared interfaces and reject incomplete cells", "[NativeInteriorBatch3D]") {
  World world;const auto fixture=Box();const auto input=Partition(Rows(fixture.patch),world);
  Directory directory(world);std::string reason;REQUIRE(directory.Build(input,reason));const auto saved=directory.Snapshot();
  std::vector<Feature> features;
  for(const auto& row:input)for(unsigned f=0;f<4;++f)if(row.second.physical[f].present){features.push_back({Faces(row.second.cell)[f],3});break;}
  const auto dependencies=directory.LookupBatch(features);REQUIRE(dependencies.valid);
  const auto imported=directory.FetchBatch(dependencies,input);REQUIRE(imported.valid);
  const auto general=directory.ReserveBatch(imported);REQUIRE(general.valid);
  CHECK(world.sum(int(std::count(general.chosen.begin(),general.chosen.end(),true)))==1);
  const auto interior=directory.ReserveInteriors(imported);REQUIRE(interior.valid);
  CHECK(world.sum(int(std::count(interior.chosen.begin(),interior.chosen.end(),true)))==6);
  ImportedBatch duplicates;duplicates.valid=true;duplicates.candidates.push_back({{Rows(fixture.patch).front()},true,{}});
  const auto unique=directory.ReserveInteriors(duplicates);REQUIRE(unique.valid);
  CHECK(world.sum(int(std::count(unique.chosen.begin(),unique.chosen.end(),true)))==1);
  auto invalid=duplicates;if(!world.rank)invalid.candidates[0].cells.push_back(Rows(fixture.patch)[1]);
  const auto refused=directory.ReserveInteriors(invalid);CHECK_FALSE(refused.valid);CHECK(refused.chosen.empty());
  const auto budget=directory.ReserveInteriors(imported,world.rank==0?1:SIZE_MAX);
  CHECK_FALSE(budget.valid);CHECK(budget.chosen.empty());CHECK(directory.Snapshot()==saved);
  const auto recovered=directory.ReserveInteriors(imported);REQUIRE(recovered.valid);
  CHECK(world.sum(int(std::count(recovered.chosen.begin(),recovered.chosen.end(),true)))==6);
}

TEST_CASE("A failed interior candidate vetoes the whole first-row publication batch", "[NativeFirstLayerBatch3D]") {
  World world;auto fixture=Box();auto other=Box();
  for(auto& f:fixture.facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))f.marker=100;
  for(auto& c:other.patch.cells){c.id+=100;for(auto& n:c.v){n.id+=100;n.p.x+=3;}}
  for(auto& f:other.patch.surface){f.facet+=100;for(auto& id:f.v)id+=100;}
  for(auto& f:other.facets){f.id+=100;for(auto& n:f.v){n.id+=100;n.p.x+=3;}
    if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))f.marker=200;}
  fixture.patch.cells.insert(fixture.patch.cells.end(),other.patch.cells.begin(),other.patch.cells.end());
  fixture.patch.surface.insert(fixture.patch.surface.end(),other.patch.surface.begin(),other.patch.surface.end());
  fixture.facets.insert(fixture.facets.end(),other.facets.begin(),other.facets.end());
  const PlanarReference reference(fixture.facets);const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  EngineOptions options;options.wall_heights={{100,.1},{200,2}};options.timings=true;
  const auto result=Refine(world,input,Uniform(input,{1,0,0,1,0,100}),reference,{},options);
  CHECK_FALSE(result.valid);CHECK(result.reason.find("geometry-height incompatibility")!=std::string::npos);
  CHECK(world.sum(uint64_t(result.stats.layer_published))==0);CHECK(Bytes(result.cells)==saved);CHECK(Bytes(input)==saved);
}


TEST_CASE("Faceted composed proposals publish independent complete cavities concurrently", "[NativeConcurrentComposed3D]") {
  World world;Fixture fixture;Owned input;
  for(int copy=0;copy<world.size;++copy) {
    auto box=Box();
    for(auto& c:box.patch.cells){c.id+=100*copy;for(auto& n:c.v){n.id+=16*copy;n.p.x+=3*copy;}}
    for(auto& f:box.patch.surface){f.facet+=12*copy;for(auto& id:f.v)id+=16*copy;}
    for(auto& f:box.facets){f.id+=12*copy;for(auto& n:f.v){n.id+=16*copy;n.p.x+=3*copy;}}
    if(copy==world.rank)for(const auto& row:Rows(box.patch))input.emplace(row.cell.id,row);
    fixture.patch.cells.insert(fixture.patch.cells.end(),box.patch.cells.begin(),box.patch.cells.end());
    fixture.patch.surface.insert(fixture.patch.surface.end(),box.patch.surface.begin(),box.patch.surface.end());
    fixture.facets.insert(fixture.facets.end(),box.facets.begin(),box.facets.end());
  }
  const auto original=Uniform(Partition(Rows(fixture.patch),world),{1.21,0,0,1.21,0,1.21});
  const auto frozen=Bytes(input);const PlanarReference geometry(fixture.facets);
  const MetricComposition compose=[](Point,Tensor m){m.xx*=1.03125;return m;};
  REQUIRE(std::sqrt(3*1.21)>MaximumMetricEdge);REQUIRE(std::sqrt(2*1.21*1.03125)<MaximumMetricEdge);
  EngineOptions options;options.timings=true;options.maximum_rounds=1;
  options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  auto first=Refine(world,input,original,geometry,compose,options);
  CHECK(Bytes(input)==frozen);CHECK(world.sum(int(first.stats.accepted))==world.size);
  CHECK(Audit(world,first.cells,fixture,CPassiveComm::Allreduce(first.cells.size(),CPassiveComm::Op::SUM,world.comm)));
  if(world.size>1){CHECK(first.stats.maximum_parallel_commits==unsigned(world.size));CHECK(first.stats.final_reservations==1);}
  else CHECK(first.stats.concurrent_rounds==0);
  EngineReport(first,world,"concurrent_composed_first_epoch");
  options.candidate_batch=1;
  const auto serial=Refine(world,input,original,geometry,compose,options);
  CHECK(world.sum(int(serial.stats.accepted))==1);CHECK(serial.stats.concurrent_rounds==0);CHECK(Bytes(input)==frozen);
  options.candidate_batch=MaximumCandidateBatch;options.maximum_work=world.size-1;
  const auto limited=Refine(world,input,original,geometry,compose,options);
  CHECK_FALSE(limited.valid);CHECK(limited.stats.stop==EngineStop::WorkBudget);
  CHECK(limited.stats.global_work<=options.maximum_work);CHECK(Bytes(input)==frozen);
  options.maximum_work=4096;options.maximum_rounds=512;
  const auto result=Refine(world,input,original,geometry,compose,options);INFO(result.reason);REQUIRE(result.valid);
  CHECK(Bytes(input)==frozen);CHECK(result.stats.global_work<=options.maximum_work);
  CHECK(Audit(world,result.cells,fixture,CPassiveComm::Allreduce(result.cells.size(),CPassiveComm::Op::SUM,world.comm)));
  const auto all=Gather(world,result.cells);
  if(!world.rank) {
    std::vector<DonorCell> full;for(auto c:fixture.patch.cells){DonorCell d;d.cell=c;d.sensor.fill({1.21,0,0,1.21,0,1.21});full.push_back(d);}
    FrozenField field(full,compose,false,{},options.edge_breakpoints);PatchMetric metric;std::string reason;const auto cells=PatchOf(all).cells;
    for(size_t at=0;at<cells.size();at+=MaximumReplacementCells)
      CHECK(ValidateMetricPatch(std::vector<Cell>(cells.begin()+at,cells.begin()+std::min(cells.size(),at+MaximumReplacementCells)),field,metric,reason));
  }
  EngineReport(result,world,"concurrent_composed_complete");
  MeshRecords("concurrent_composed_initial",world,input);MeshRecords("concurrent_composed_accepted",world,result.cells);
  if(world.size>1) {
    auto mixed=original;for(auto& donor:mixed)if(donor.cell.id<100)donor.sensor.fill({1e8,0,0,1.21,0,1.21});
    EngineOptions observed;observed.maximum_rounds=1;observed.edge_breakpoints=options.edge_breakpoints;
    observed.capture_feature={{{0,6,0}},2};bool constructed=false,ended=false;
    observed.transaction_observer=[&](const TransactionView& view) {
      if(!view.selected)return;
      if(std::string(view.stage)=="construction_end") {
        constructed=true;CHECK(view.passed);CHECK_FALSE(view.metric.shape_measured);
      }
      if(std::string(view.stage)=="transaction_end") {ended=true;CHECK(view.passed);}
    };
    // Anisotropic low shape is provisional construction progress; strict shape belongs to the next stage.
    const auto provisional=Refine(world,input,mixed,geometry,compose,observed);
    CHECK(world.sum(int(provisional.stats.accepted))==world.size);CHECK(Bytes(input)==frozen);
    CHECK_FALSE(provisional.valid);CHECK(provisional.stats.quality_stages==0);
    if(!world.rank){REQUIRE(constructed);REQUIRE(ended);}
  }

}

TEST_CASE("Conflicting composed refinements prioritize the global metric demand before rank", "[NativeConcurrentComposed3D]") {
  World world;auto fixture=Box();
  for(auto& c:fixture.patch.cells)for(auto& n:c.v)if(n.id==3)n.p.x=-1;
  for(auto& f:fixture.facets)for(auto& n:f.v)if(n.id==3)n.p.x=-1;
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  const PlanarReference geometry(fixture.facets);const auto source=Uniform(input,{4,0,0,4,0,4});
  const MetricComposition compose=[](Point,Tensor m){return m;};
  EngineOptions options;options.maximum_rounds=1;options.timings=true;
  options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  const auto result=Refine(world,input,source,geometry,compose,options);
  INFO(result.reason);CHECK(world.sum(int(result.stats.accepted))==1);
  CHECK(world.sum(int(result.splits.size()))==1);
  for(const auto& split:result.splits)CHECK((split.edge==Edge{3,6}));
  CHECK(Bytes(input)==saved);CHECK(Audit(world,result.cells,fixture,8));
  EngineReport(result,world,"global_priority_composed_refine");
}

TEST_CASE("Replicated composed refinement consumes one unique edge proposal", "[NativeConcurrentComposed3D]") {
  World world;const auto fixture=Box();const auto input=Partition(Rows(fixture.patch),world);
  const auto saved=Bytes(input);const PlanarReference geometry(fixture.facets);
  const auto source=Uniform(input,{1.21,0,0,1.21,0,1.21});
  const MetricComposition compose=[](Point,Tensor m){m.xx*=1.03125;return m;};
  EngineOptions options;options.maximum_work=1;options.timings=true;
  options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  const auto result=Refine(world,input,source,geometry,compose,options);
  INFO(result.reason);REQUIRE(result.valid);CHECK(result.stats.global_work==1);
  CHECK(world.sum(int(result.stats.proposed))==1);CHECK(world.sum(int(result.stats.accepted))==1);
  CHECK(world.sum(int(result.stats.proposal_duplicates))==std::min(world.size,6)-1);
  CHECK(Bytes(input)==saved);CHECK(result.stats.cached_invalid_cells==0);
  CHECK(Audit(world,result.cells,fixture,12));
  EngineReport(result,world,"unique_composed_refine");
}

TEST_CASE("Final reservations include expanded candidates and completed peers without mutation", "[NativeConcurrentComposed3D]") {
  World world;if(world.size<2)return;auto left=Box(),right=Box();
  for(auto& c:right.patch.cells){c.id+=100;for(auto& n:c.v){n.id+=16;n.p.x+=3;}}
  for(auto& f:right.patch.surface){f.facet+=12;for(auto& id:f.v)id+=16;}
  left.patch.cells.insert(left.patch.cells.end(),right.patch.cells.begin(),right.patch.cells.end());
  left.patch.surface.insert(left.patch.surface.end(),right.patch.surface.begin(),right.patch.surface.end());
  const auto input=Partition(Rows(left.patch),world);Directory directory(world);std::string reason;
  REQUIRE(directory.Build(input,reason));const auto before=Bytes(input),metadata=directory.Snapshot();
  const bool active=world.rank<2;const Feature own{{world.rank==0?Id(0):Id(16),0,0},1};
  const auto base=directory.Lookup(active?std::vector<Feature>{own}:std::vector<Feature>{});REQUIRE(base.valid);
  const auto initial=directory.Fetch(base,input);REQUIRE(initial.valid);CHECK(directory.Reserve(initial,active,reason)==active);
  const auto enlarged=directory.Lookup(world.rank==0?std::vector<Feature>{own,{{16,0,0},1}}:
      active?std::vector<Feature>{own}:std::vector<Feature>{});REQUIRE(enlarged.valid);
  const auto final=directory.Fetch(enlarged,input);REQUIRE(final.valid);
  CHECK(directory.Reserve(final,active,reason)==(world.rank==0));
  CHECK(Bytes(input)==before);CHECK(directory.Snapshot()==metadata);
  if(world.rank==1)CHECK(reason=="Native 3D conflicting dependency reservation.");
  CHECK(directory.Reserve(initial,active,reason,SIZE_MAX,0,world.rank==1?0u:1u)==active);
  CHECK(directory.Reserve(final,active,reason,SIZE_MAX,0,world.rank==1?0u:1u)==(world.rank==1));
  if(world.rank==0)CHECK(reason=="Native 3D conflicting dependency reservation.");
  CHECK(Bytes(input)==before);CHECK(directory.Snapshot()==metadata);
}

TEST_CASE("Rejected working states are observable without changing accepted inputs", "[NativeRejectedState3D]") {
  World world;const auto fixture=Box();const PlanarReference geometry(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  const auto source=Uniform(input,{4,0,0,4,0,4});EngineOptions options;options.maximum_rounds=1;
  unsigned observations=0;std::vector<char> observed;
  options.rejected_state=[&](const Owned& cells,const PlanarReference& active,const std::string& reason){++observations;observed=Bytes(cells);CHECK(&active==&geometry);CHECK_FALSE(reason.empty());};
  const auto rejected=Refine(world,input,source,geometry,{},options);
  CHECK_FALSE(rejected.valid);CHECK(observations==1);CHECK(observed==Bytes(rejected.cells));CHECK(Bytes(input)==saved);
  options.rejected_state={};
  if(!world.rank)options.rejected_state=[](const Owned&,const PlanarReference&,const std::string&){throw std::runtime_error("deliberate diagnostic refusal");};
  const auto refused=Refine(world,input,source,geometry,{},options);
  CHECK_FALSE(refused.valid);CHECK(refused.stats.stop==EngineStop::Failure);
  CHECK(refused.reason.find("Rejected-state diagnostic failed:")!=std::string::npos);
  CHECK(refused.reason.find("deliberate diagnostic refusal")!=std::string::npos);CHECK(Bytes(input)==saved);
  observations=0;options.rejected_state=[&](const Owned&,const PlanarReference&,const std::string&){++observations;};
  const auto accepted=Refine(world,input,Uniform(input,Tensor{}),geometry,{},options);
  CHECK(accepted.valid);CHECK(observations==0);CHECK(Bytes(input)==saved);
  auto root=std::make_shared<PlanarReference>(fixture.facets);std::map<Id,double> errors;
  for(auto f:fixture.facets)errors.emplace(f.id,1e-13);
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(root,errors,7);
  BoundaryPoints records;for(auto f:fixture.facets)for(auto n:f.v)records.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});
  auto parameters=std::make_shared<BoundaryParameterState>(authority,records,fixture.patch.surface);
  auto mapped=PlanarReference::WithParameters(root,parameters);const auto original_parameters=parameters->Snapshot();
  std::vector<char> observed_parameters;observations=0;
  options.rejected_state=[&](const Owned& cells,const PlanarReference& active,const std::string& reason) {
    ++observations;observed=Bytes(cells);REQUIRE(active.Parameters());CHECK(active.Parameters()!=parameters.get());
    observed_parameters=active.Parameters()->Snapshot();CHECK_FALSE(reason.empty());
  };
  const auto mapped_rejected=Refine(world,input,source,*mapped,{},options);
  CHECK_FALSE(mapped_rejected.valid);CHECK(observations==1);REQUIRE(mapped_rejected.reference);
  CHECK(observed==Bytes(mapped_rejected.cells));CHECK(observed_parameters==mapped_rejected.reference->Parameters()->Snapshot());
  CHECK(parameters->Snapshot()==original_parameters);CHECK(Bytes(input)==saved);

}


TEST_CASE("Saved original metric edge replays its high-valence footprint without CFD", "[.][NativeRegionFieldReplay3D]") {
  World world;const char* path=std::getenv("SU2_NATIVE_REGION_FIELD_REPLAY");REQUIRE(path);
  std::ifstream input(path);std::string magic;size_t count=0;Id aid=0,bid=0;Point a,b;
  input>>magic>>count>>aid>>bid>>a.x>>a.y>>a.z>>b.x>>b.y>>b.z;
  REQUIRE(magic=="NATIVE_METRIC_EDGE_WITNESS_V1");REQUIRE(count>MaximumDonors);REQUIRE(count<=8192);
  std::vector<DonorCell> local,all;std::set<Id> expected;
  for(size_t i=0;i<count;++i) {
    DonorCell d;input>>d.cell.id;bool first=false,last=false;
    for(unsigned k=0;k<4;++k){auto& n=d.cell.v[k];auto& m=d.sensor[k];input>>n.id>>n.p.x>>n.p.y>>n.p.z>>m.xx>>m.xy>>m.xz>>m.yy>>m.yz>>m.zz;first=first||n.id==aid;last=last||n.id==bid;}
    REQUIRE(bool(input));REQUIRE(first);if(last)expected.insert(d.cell.id);
    all.push_back(d);
    if(d.cell.id%world.size==unsigned(world.rank))local.push_back(d);
  }
  std::vector<char> bytes;RecordStream encode(bytes);for(auto d:local)encode(d);DonorImport source(world);std::string reason;REQUIRE(source.Build(local,reason));
  const auto query=world.rank==0?std::vector<std::array<Point,2>>{{a,b}}:std::vector<std::array<Point,2>>{};
  CHECK_FALSE(source.ImportSegments(query).valid);
  const auto edge=source.ImportEdgeSegments(query);INFO(edge.reason);REQUIRE(edge.valid);
  if(!world.rank){std::set<Id> actual;for(const auto& d:edge.cells)actual.insert(d.cell.id);CHECK(actual==expected);}
  PlanarReference geometry(std::vector<Facet>{});RegionFieldService service(world,source,geometry,{}, {},false,SIZE_MAX,0);CLocalFailure failure;
  try{service.Run([&](const RegionMetric& field){
    const auto m=field.point(a);if(m.xx!=1 || m.yy!=1 || m.zz!=1)throw std::runtime_error("Saved canonical source changed.");
    if(std::abs(field.edge(a,b)-std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z))>1e-12L)throw std::runtime_error("Saved full identity edge changed.");
  });}catch(const std::exception& e){failure.Set(1,0,e.what());}
  const auto failed=world.elect(failure);INFO(failed.message);CHECK_FALSE(failed.any);std::vector<char> after;RecordStream encodeAfter(after);for(auto d:local)encodeAfter(d);CHECK(after==bytes);
  Fixture fixture;std::map<Id,Node> nodes;
  for(const auto& d:all){fixture.patch.cells.push_back(d.cell);for(auto n:d.cell.v)nodes.emplace(n.id,n);}
  for(const auto face:Incidence(fixture.patch.cells).Boundary()) {
    const Id id=fixture.facets.size();fixture.patch.surface.push_back({face,id});
    fixture.facets.push_back({id,0,{nodes.at(face[0]),nodes.at(face[1]),nodes.at(face[2])}});
  }
  const PlanarReference authority(fixture.facets);const auto owned=Partition(Rows(fixture.patch),world);const auto before=Bytes(owned);
  EngineOptions options;options.maximum_work=4096;options.maximum_seconds=30;options.timings=true;
  options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  const auto reduced=Coarsen(world,owned,Uniform(owned,{.25,0,0,.25,0,.25}),authority,[](Point,Tensor m){return m;},options);
  INFO(reduced.reason);REQUIRE(reduced.valid);CHECK(reduced.stats.cached_invalid_cells==0);
  CHECK(reduced.stats.cached_measured_cells==reduced.cells.size());CHECK(reduced.stats.remaining_edges==0);
  CHECK(world.sum(int(reduced.stats.rejected))>0);CHECK(Bytes(owned)==before);
  CHECK(world.sum(int(reduced.stats.blocked_queue_skips))>0);
  CHECK(Audit(world,reduced.cells,fixture,CPassiveComm::Allreduce(reduced.cells.size(),CPassiveComm::Op::SUM,world.comm)));
  if(!world.rank)std::cout<<"STAR_ADMISSION_CONTROL cells="<<count<<" cap="<<MaximumCavityCells<<" valid="<<reduced.valid<<" work="<<reduced.stats.global_work<<" blocked_queue_skips="<<reduced.stats.blocked_queue_skips<<" blocked_unmeasured="<<reduced.stats.blocked_unmeasured<<std::endl;
  if(!world.rank)std::cout<<"EDGE_WITNESS ranks="<<world.size<<" original="<<count<<" positive="<<expected.size()<<" valid="<<!failed.any<<std::endl;
}

namespace {
// Convex star whose center belongs to more cells than one cavity admits; every other vertex is a physical corner.
Fixture IcosphereStar(unsigned levels=1) {
  const double phi=(1+std::sqrt(5.))/2;
  std::vector<std::array<double,3>> points{{-1,phi,0},{1,phi,0},{-1,-phi,0},{1,-phi,0},{0,-1,phi},{0,1,phi},
                                           {0,-1,-phi},{0,1,-phi},{phi,0,-1},{phi,0,1},{-phi,0,-1},{-phi,0,1}};
  std::vector<std::array<size_t,3>> faces{{0,11,5},{0,5,1},{0,1,7},{0,7,10},{0,10,11},{1,5,9},{5,11,4},{11,10,2},{10,7,6},{7,1,8},
                                          {3,9,4},{3,4,2},{3,2,6},{3,6,8},{3,8,9},{4,9,5},{2,4,11},{6,2,10},{8,6,7},{9,8,1}};
  auto unit=[](std::array<double,3> p){const double n=std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);for(auto& x:p)x/=n;return p;};
  for(auto& p:points)p=unit(p);
  std::map<std::pair<size_t,size_t>,size_t> middle;
  auto mid=[&](size_t a,size_t b){
    const auto key=std::minmax(a,b);const auto found=middle.find(key);if(found!=middle.end())return found->second;
    points.push_back(unit({(points[a][0]+points[b][0])/2,(points[a][1]+points[b][1])/2,(points[a][2]+points[b][2])/2}));
    middle.emplace(key,points.size()-1);return points.size()-1;
  };
  std::vector<std::array<size_t,3>> fine=faces;
  for(unsigned level=0;level<levels;++level) {
    auto coarse=std::move(fine);fine.clear();
    for(auto f:coarse){const auto a=mid(f[0],f[1]),b=mid(f[1],f[2]),c=mid(f[2],f[0]);fine.insert(fine.end(),{{f[0],a,c},{f[1],b,a},{f[2],c,b},{a,b,c}});}
  }
  Fixture result;const Node center{1000,{0,0,0}};
  auto node=[&](size_t i){return Node{Id(i),{points[i][0],points[i][1],points[i][2]}};};
  for(auto f:fine) {
    std::array<Node,3> tri{node(f[0]),node(f[1]),node(f[2])};
    if(Orientation(center.p,tri[0].p,tri[1].p,tri[2].p)<0)std::swap(tri[1],tri[2]);
    Cell cell{Id(result.patch.cells.size()),{center,tri[0],tri[1],tri[2]}};REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
    result.patch.cells.push_back(cell);
  }
  std::map<Id,Node> nodes;for(const auto& c:result.patch.cells)for(auto n:c.v)nodes.emplace(n.id,n);
  for(auto face:Incidence(result.patch.cells).Boundary()) {
    const auto id=Id(result.facets.size());
    result.facets.push_back({id,1,{nodes.at(face[0]),nodes.at(face[1]),nodes.at(face[2])}});result.patch.surface.push_back({face,id});
  }
  return result;
}
// One axis edge shared by more cells than one cavity admits.
Fixture AxisWheel(unsigned sectors) {
  const double pi=std::acos(-1.);Fixture result;const Node top{1000,{0,0,1}},bottom{1001,{0,0,-1}};
  for(unsigned i=0;i<sectors;++i) {
    const Node a{i,{std::cos(2*pi*i/sectors),std::sin(2*pi*i/sectors),0}},b{(i+1)%sectors,{std::cos(2*pi*((i+1)%sectors)/sectors),std::sin(2*pi*((i+1)%sectors)/sectors),0}};
    Cell cell{Id(i),{top,bottom,a,b}};if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
    result.patch.cells.push_back(cell);
  }
  std::map<Id,Node> nodes;for(const auto& c:result.patch.cells)for(auto n:c.v)nodes.emplace(n.id,n);
  for(auto face:Incidence(result.patch.cells).Boundary()) {
    const auto id=Id(result.facets.size());
    result.facets.push_back({id,1,{nodes.at(face[0]),nodes.at(face[1]),nodes.at(face[2])}});result.patch.surface.push_back({face,id});
  }
  return result;
}
}  // namespace

TEST_CASE("Complete coordinate stars cross payload pages and publish atomically", "[NativeDistributed3D][NativeCoordinateStar3D]") {
  World world;
  for(unsigned levels:{1u,2u}) {
  const auto fixture=IcosphereStar(levels);const size_t count=fixture.patch.cells.size();const PlanarReference geometry(fixture.facets);
  auto owned=Partition(Rows(fixture.patch),world);const auto original=Bytes(owned);
  Directory directory(world,true);std::string reason;REQUIRE(directory.Build(owned,reason));
  const auto original_directory=directory.Snapshot();
  const Feature center{{1000,0,0},1};
  auto bounded=directory.LookupBatch(Query(world,center));REQUIRE(bounded.valid);
  if(!world.rank){CHECK_FALSE(bounded.candidates.front().valid);CHECK(bounded.candidates.front().oversized);}
  CHECK_FALSE(directory.LookupBatch(Query(world,center),world.rank?SIZE_MAX:1,0,true).valid);
  auto batch=directory.LookupBatch(Query(world,center),SIZE_MAX,0,true);REQUIRE(batch.valid);
  auto payload_batch=directory.FetchBatch(batch,owned);REQUIRE(payload_batch.valid);
  if(!world.rank) {
    REQUIRE(payload_batch.candidates.size()==1);
    CHECK(payload_batch.candidates.front().cells.size()==count);
    CHECK(payload_batch.candidates.front().coordinate_star.count==1);
  }
  const auto batch_reservation=directory.ReserveBatch(payload_batch);REQUIRE(batch_reservation.valid);
  CHECK(world.sum(std::count(batch_reservation.chosen.begin(),batch_reservation.chosen.end(),true))==1);
  CHECK_FALSE(directory.FetchBatch(batch,owned,world.rank?SIZE_MAX:1).valid);
  CHECK_FALSE(directory.ReserveBatch(payload_batch,world.rank?SIZE_MAX:1).valid);
  auto malformed=payload_batch;
  if(!world.rank)malformed.candidates.front().coordinate_star.vertices[0]=UINT64_MAX;
  CHECK_FALSE(directory.ReserveBatch(malformed).valid);
  Dependencies dependencies;dependencies.valid=true;
  if(!world.rank){dependencies=batch.candidates.front();REQUIRE(dependencies.valid);CHECK(dependencies.cells.size()==count);}
  auto imported=directory.Fetch(dependencies,owned);REQUIRE(imported.valid);
  if(!world.rank){CHECK(imported.cells.size()==count);CHECK(imported.coordinate_star.count==1);}
  REQUIRE(world.sum(directory.Reserve(imported,world.rank==0,reason))==1);
  Patch moved;REQUIRE(MoveVertex(fixture.patch,1000,{2000,{.01,0,0}},10000,geometry,moved,reason,nullptr,fixture.patch.cells.size()));
  bool candidate=world.rank==0;
  REQUIRE(directory.Clearance(owned,fixture.patch,moved,candidate,reason,SIZE_MAX,0,nullptr,0,center));
  REQUIRE(world.sum(candidate)==1);
  auto corrupt=moved;corrupt.cells.front().v[1].p.x+=.001;candidate=world.rank==0;
  CHECK_FALSE(directory.Clearance(owned,fixture.patch,corrupt,candidate,reason,SIZE_MAX,0,nullptr,0,center));
  auto added=Partition(Rows(moved),world);std::vector<Dependency> removed;std::vector<OwnedCell> replacements;
  for(const auto& row:owned){removed.push_back({row.first,row.second.version,world.rank});auto next=added.at(row.first);next.version=row.second.version+1;replacements.push_back(next);}
  CHECK_FALSE(directory.Publish(owned,removed,replacements,true,reason,world.rank?SIZE_MAX:1));
  CHECK(Bytes(owned)==original);CHECK(directory.Snapshot()==original_directory);
  REQUIRE(directory.Publish(owned,removed,replacements,true,reason));
  CHECK(Audit(world,owned,fixture,count));
  for(const auto& row:owned){CHECK(row.second.version==18+row.first);CHECK(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[](Node n){return n.id==2000;}));}
  const auto current=directory.LookupBatch(Query(world,{{2000,0,0},1}),SIZE_MAX,0,true);REQUIRE(current.valid);
  if(!world.rank){REQUIRE(current.candidates.front().valid);CHECK(current.candidates.front().cells.size()==count);}
  CHECK_FALSE(directory.Lookup(Query(world,center)).valid);
  }
}

TEST_CASE("Distributed repair moves complete large stars without changing cell ownership or identities", "[NativeCoordinateStar3D][NativeStageProgress3D]") {
  World world;
  for(unsigned levels:{1u,2u}) {
    auto fixture=IcosphereStar(levels);
    const auto& face=fixture.patch.cells.front().v;
    const Point displaced{.999*(face[1].p.x+face[2].p.x+face[3].p.x)/3,
                          .999*(face[1].p.y+face[2].p.y+face[3].p.y)/3,
                          .999*(face[1].p.z+face[2].p.z+face[3].p.z)/3};
    for(auto& c:fixture.patch.cells)for(auto& n:c.v) {
      if(n.id==1000){n.id=0;n.p=displaced;}else ++n.id;
    }
    for(auto& f:fixture.patch.surface)for(auto& id:f.v)++id;
    for(auto& f:fixture.facets)for(auto& n:f.v)++n.id;
    const PlanarReference geometry(fixture.facets);const auto input=Partition(Rows(fixture.patch),world);
    const auto saved=Bytes(input);const Tensor tensor{.81,0,0,.81,0,.81};unsigned initial_bad=0;
    // Approach a face from the interior to guarantee a flat cell at either refinement.
    // Both endpoints stay in the unit ball, so every metric edge remains below 1.8.
    for(const auto& row:input) {
      const auto& v=row.second.cell.v;REQUIRE(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);
      const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
      REQUIRE(m.maximum_edge<=MaximumMetricEdge);
      initial_bad+=m.mean_ratio<MinimumMeanRatio || m.minimum_scaled_jacobian<MinimumScaledJacobian;
    }
    REQUIRE(world.sum(initial_bad)>0);
    EngineOptions options;options.phase_ownership=PhaseOwnership::Disabled;options.candidate_batch=1;options.maximum_rounds=1;options.maximum_work=4096;
    const auto source=Uniform(input,tensor);
    const auto baseline=Repair(world,input,source,geometry,{},options);
    size_t quality_records=0,exit_records=0;bool diagnostics_match=true,construction_unmeasured=true;
    options.stage_observer=[&](const StageProgressView& view) {
      const auto& progress=view.progress;
      diagnostics_match &= progress.owned_cells==view.working.size() &&
          progress.measured_cells+progress.unmeasured_cells==view.working.size() &&
          progress.shape_measured_cells+progress.shape_unmeasured_cells==view.working.size();
      if(view.phase==EnginePhase::Construction)
        construction_unmeasured &= progress.shape_measured_cells==0 && progress.q_hinge_squared==0 && progress.j_hinge_squared==0;
      quality_records+=std::string(view.stage)=="quality_entry";
      exit_records+=std::string(view.stage)=="exit";
      if(!progress.complete)return;
      long double q_debt=0,j_debt=0,size_debt=0;size_t q_bad=0,j_bad=0,size_bad=0;
      for(const auto& row:view.working) {
        const auto& v=row.second.cell.v;
        const auto measured=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
        const long double q=std::max(0.,1-measured.mean_ratio/MinimumMeanRatio);
        const long double j=std::max(0.,1-measured.minimum_scaled_jacobian/MinimumScaledJacobian);
        q_debt+=q*q;j_debt+=j*j;q_bad+=q>0;j_bad+=j>0;
        size_bad+=measured.maximum_edge>MaximumMetricEdge;
        for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b) {
          const auto excess=std::max(0.L,MetricNorm(v[a].p,v[b].p,tensor)/MaximumMetricEdge-1);
          size_debt+=excess*excess;
        }
      }
      const auto close=[](long double a,long double b){return std::abs(a-b)<=1e-11L*std::max(1.L,std::abs(b));};
      diagnostics_match &= progress.q_bad_cells==q_bad && progress.j_bad_cells==j_bad && progress.size_bad_cells==size_bad &&
          close(progress.q_hinge_squared,q_debt) && close(progress.j_hinge_squared,j_debt) && close(progress.size_hinge_squared,size_debt);
    };
    const auto result=Repair(world,input,source,geometry,{},options);INFO(result.reason);
    CHECK(diagnostics_match);CHECK(construction_unmeasured);CHECK(quality_records==1);CHECK(exit_records==1);
    CHECK(result.valid==baseline.valid);CHECK(Bytes(result.cells)==Bytes(baseline.cells));
    CHECK(result.stats.field_requests==baseline.stats.field_requests);CHECK(result.stats.field_edges==baseline.stats.field_edges);
    CHECK(result.stats.global_work==baseline.stats.global_work);CHECK(result.stats.accepted==baseline.stats.accepted);
    CHECK(result.stats.quality_stages==1);CHECK(world.sum(result.stats.moved)==1);
    CHECK(result.cells.size()==input.size());CHECK(Bytes(input)==saved);
    for(const auto& row:result.cells) {
      REQUIRE(input.count(row.first));CHECK(row.second.version==input.at(row.first).version+1);
      CHECK(std::none_of(row.second.cell.v.begin(),row.second.cell.v.end(),[](Node n){return n.id==0;}));
      const auto& v=row.second.cell.v;CHECK(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);
      if(result.valid) {
        const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
        CHECK(m.mean_ratio>=MinimumMeanRatio);CHECK(m.minimum_scaled_jacobian>=MinimumScaledJacobian);CHECK(m.maximum_edge<=MaximumMetricEdge);
      }
    }
    CHECK(Audit(world,result.cells,fixture,fixture.patch.cells.size()));
    CHECK(result.stats.global_work<=options.maximum_work);
  }
}

TEST_CASE("Stage diagnostics elect callback failures and admit their storage before invocation", "[NativeStageProgress3D]") {
  World world;const auto fixture=Box();const PlanarReference geometry(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world),saved=input;
  const auto source=Uniform(input,{1,0,0,1,0,1});
  EngineOptions options;options.maximum_work=64;options.maximum_rounds=4;
  bool observed=false;
  options.stage_observer=[&](const StageProgressView& view) {
    observed=true;
    if(!world.rank && std::string(view.stage)=="quality_entry")throw std::runtime_error("stage observer test failure");
  };
  const auto rejected=Repair(world,input,source,geometry,{},options);
  CHECK(observed);CHECK_FALSE(rejected.valid);CHECK(rejected.stats.stop==EngineStop::Failure);
  CHECK(rejected.reason.find("stage observer test failure")!=std::string::npos);CHECK(Bytes(input)==Bytes(saved));
  observed=false;options.stage_observer_bytes=SIZE_MAX;
  const auto refused=Repair(world,input,source,geometry,{},options);
  CHECK_FALSE(refused.valid);CHECK_FALSE(observed);CHECK(Bytes(input)==Bytes(saved));
}

TEST_CASE("Oversized complete stars block batched and planned proposals without failing the event", "[NativeDistributed3D][NativeOversizedStar3D]") {
  // T5 (audit 3D #1): MPI1 batched LookupBatch, MPI>1 PlanBatch and the one-candidate path all block the star.
  World world;const auto sphere=IcosphereStar();REQUIRE(sphere.patch.cells.size()==80);
  const PlanarReference geometry(sphere.facets);const auto owned=Partition(Rows(sphere.patch),world);const auto saved=Bytes(owned);
  for(size_t batch:{size_t(1),MaximumCandidateBatch}) {
    EngineOptions options;options.candidate_batch=batch;options.maximum_work=4096;
    const auto result=Coarsen(world,owned,Uniform(owned,{.01,0,0,.01,0,.01}),geometry,{},options);
    if(!world.rank)std::cout<<"OVERSIZED_STAR coarsen ranks="<<world.size<<" batch="<<batch<<" valid="<<result.valid
        <<" stop="<<EngineStopName(result.stats.stop)<<" rounds="<<result.stats.rounds<<" oversized="<<result.stats.oversized_stars
        <<" rejected="<<result.stats.rejected<<" batch_rounds="<<result.stats.batch_rounds<<" reason="<<result.reason<<std::endl;
    INFO(result.reason);REQUIRE(result.valid);CHECK(result.stats.stop==EngineStop::Complete);
    CHECK(world.sum(int(result.stats.oversized_stars))>0);CHECK(world.sum(int(result.stats.accepted))==0);
    CHECK(CPassiveComm::Allreduce(result.cells.size(),CPassiveComm::Op::SUM,world.comm)==80);
    CHECK(Bytes(owned)==saved);
  }
  const auto wheel=AxisWheel(70);const PlanarReference axis(wheel.facets);const auto spokes=Partition(Rows(wheel.patch),world);
  const auto spokes_saved=Bytes(spokes);
  for(size_t batch:{size_t(1),MaximumCandidateBatch}) {
    EngineOptions options;options.candidate_batch=batch;options.maximum_rounds=1;
    const auto result=Refine(world,spokes,Uniform(spokes,{1,0,0,1,0,1}),axis,{},options);
    if(!world.rank)std::cout<<"OVERSIZED_STAR refine ranks="<<world.size<<" batch="<<batch<<" valid="<<result.valid
        <<" stop="<<EngineStopName(result.stats.stop)<<" oversized="<<result.stats.oversized_stars<<" reason="<<result.reason<<std::endl;
    INFO(result.reason);CHECK_FALSE(result.valid);CHECK(result.stats.stop==EngineStop::RoundBudget);
    CHECK(world.sum(int(result.stats.oversized_stars))>0);
    CHECK(world.maximum(result.stats.last_rejected_feature.count==2 &&
        ((result.stats.last_rejected_feature.vertices[0]==1000 && result.stats.last_rejected_feature.vertices[1]==1001) ||
         (result.stats.last_rejected_feature.vertices[0]==1001 && result.stats.last_rejected_feature.vertices[1]==1000))));
    CHECK(CPassiveComm::Allreduce(result.cells.size(),CPassiveComm::Op::SUM,world.comm)==70);
    CHECK(Bytes(spokes)==spokes_saved);
  }
}

TEST_CASE("Native current collision index follows remote publication and retains exact contact vetoes", "[NativeCollision3D]") {
  World world;const int owner=world.size-1;
  auto cell=[](Id id,Id first,double x,double y,double z,double h) {
    OwnedCell row;row.cell={id,{{{first,{x,y,z}},{first+1,{x+h,y,z}},
        {first+2,{x,y+h,z}},{first+3,{x,y,z+h}}}}};return row;
  };
  auto obstacle=cell(10,100,2,0,0,.5);Owned owned;if(world.rank==owner)owned.emplace(10,obstacle);
  Directory directory(world);std::string reason;REQUIRE(directory.Build(owned,reason));
  const Patch query{{cell(20,0,0,0,0,1).cell},{}};
  auto check=[&](bool expected,const Patch& old=Patch{}) {
    bool candidate=world.rank==0;
    const bool cleared=directory.Clearance(owned,old,query,candidate,reason);INFO(reason);
    REQUIRE(cleared);
    CHECK(world.sum(candidate)==int(expected));
  };
  check(true);check(true);CHECK(directory.Statistics().collision_builds==1);
  auto moved=cell(10,100,.2,.2,.2,.5);moved.version=1;
  std::vector<Dependency> remove;std::vector<OwnedCell> add;
  if(world.rank==owner){remove.push_back({10,0,owner});add.push_back(moved);}
  const auto before=Bytes(owned),metadata=directory.Snapshot();
  CHECK_FALSE(directory.Publish(owned,remove,add,world.rank!=0,reason));
  CHECK(Bytes(owned)==before);CHECK(directory.Snapshot()==metadata);check(true);
  REQUIRE(directory.Publish(owned,remove,add,true,reason));check(false);
  CHECK(directory.Statistics().collision_builds==1);
  check(true,Patch{{moved.cell},{}}); // The complete removed cavity is excluded on its actual owner.
  if(world.rank==owner)remove[0].version=1;add.clear();
  REQUIRE(directory.Publish(owned,remove,add,true,reason));check(true);remove.clear();
  if(world.rank==owner)for(Id i=0;i<70;++i)add.push_back(cell(100+i,1000+4*i,i?3+double(i):.2,i?0:.2,i?0:.2,.5));
  REQUIRE(directory.Publish(owned,remove,add,true,reason));check(false);
  CHECK(world.maximum(directory.Statistics().collision_builds==2));
  add.clear();if(world.rank==owner)remove.push_back({100,0,owner});
  REQUIRE(directory.Publish(owned,remove,add,true,reason));check(true); // Delete an obstacle already in the rebuilt tree.
  bool candidate=world.rank==0;const auto saved=Bytes(owned),savedMetadata=directory.Snapshot();
  CHECK_FALSE(directory.Clearance(owned,{},query,candidate,reason,world.rank?SIZE_MAX:1));
  CHECK(Bytes(owned)==saved);CHECK(directory.Snapshot()==savedMetadata);check(true);
  auto duplicate=query;duplicate.cells.push_back(query.cells[0]);duplicate.cells.back().id=21;
  candidate=world.rank==0;REQUIRE(directory.Clearance(owned,{},duplicate,candidate,reason));CHECK(world.sum(candidate)==0);
  {
    const Patch empty,overlap{{cell(21,200,.2,.2,.2,.5).cell},{}},disjoint{{cell(22,300,2,0,0,.5).cell},{}};
    auto batch=[&](bool conflicting,bool remote,bool reverse_priority=false) {
      std::vector<ClearanceCandidate> proposals;
      if(!world.rank) {
        proposals.push_back({&empty,&query,true,false,reverse_priority?1u:0u,{},{}});
        proposals.push_back({&empty,conflicting?&overlap:&disjoint,true,false,0,{},{}});
      } else if(remote&&world.rank==1)proposals.push_back({&empty,&overlap,true,false,0,{},{}});
      REQUIRE(directory.ClearanceBatch(owned,proposals,reason));
      CHECK(Bytes(owned)==saved);CHECK(directory.Snapshot()==savedMetadata);
      int accepted=0,deferred=0;for(const auto& proposal:proposals){accepted+=proposal.candidate;deferred+=proposal.deferred;}
      CHECK(world.sum(accepted)==(conflicting?1:2));
      CHECK(world.sum(deferred)==int(conflicting)+int(remote&&world.size>1));
      if(!world.rank&&conflicting){CHECK(proposals[0].candidate==!reverse_priority);CHECK(proposals[1].candidate==reverse_priority);}
    };
    batch(false,false);batch(true,false);batch(true,false,true);batch(false,true);
    std::vector<ClearanceCandidate> refused;
    if(!world.rank)refused.push_back({&empty,&query,true,false,0,{},{}});
    CHECK_FALSE(directory.ClearanceBatch(owned,refused,reason,world.rank?SIZE_MAX:1));
    CHECK(Bytes(owned)==saved);CHECK(directory.Snapshot()==savedMetadata);
    // Violating an admitted coordinate-star contract is a round failure, not a peer contact veto.
    if(!world.rank)refused.push_back({&empty,&query,true,false,0,{{0,0,0},1},{}});
    CHECK_FALSE(directory.ClearanceBatch(owned,refused,reason));
    CHECK(Bytes(owned)==saved);CHECK(directory.Snapshot()==savedMetadata);
  }
  if(world.size>1) {
    auto peers=[&](const Patch& first,const Patch& second,int accepted,bool expectDeferred,bool prefer_second=false) {
      bool active=world.rank<2,deferred=false;
      REQUIRE(directory.Clearance(owned,{},world.rank?second:first,active,reason,SIZE_MAX,0,&deferred,
          prefer_second && world.rank!=1?1u:0u));
      INFO(reason);CHECK(world.sum(active)==accepted);CHECK(world.sum(deferred)==int(expectDeferred));
      CHECK(Bytes(owned)==saved);CHECK(directory.Snapshot()==savedMetadata);
      return active;
    };
    const Patch overlap{{cell(21,200,.2,.2,.2,.5).cell},{}};
    const bool selected=peers(query,overlap,1,true);CHECK(selected==(world.rank==0));
    CHECK(peers(query,overlap,1,true,true)==(world.rank==1));
    const Patch disjoint{{cell(22,300,2,0,0,.5).cell},{}};
    CHECK(peers(query,disjoint,2,false)==(world.rank<2));
    CHECK(peers(query,disjoint,2,false,true)==(world.rank<2));
    // The first proposal hits the published obstacle at x=4; its peer only intersects that rejected proposal.
    const Patch obstructed{{cell(23,400,3,0,0,2).cell},{}};
    const Patch free{{cell(24,500,3.1,.05,.05,.1).cell},{}};
    CHECK(peers(obstructed,free,1,false)==(world.rank==1));
    CHECK(peers(free,obstructed,1,false,true)==(world.rank==0));
  }
}

TEST_CASE("Native MPI interior placement repairs a complete star through ordinary transactions", "[NativeInteriorPlacement3D]") {
  World world;
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  for(bool sheared:{false,true})for(bool mapped:{false,true})for(bool height:{false,true}) {
    auto fixture=Box(1.,sheared);fixture.patch.cells.clear();
    const Node center=height?Node{8,{.5+(sheared?.5:0),.5,.65}}:Node{8,{.95+(sheared?.93:0),.93,.9}};
    for(const auto& f:fixture.facets) {
      Cell c{fixture.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
      if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
      fixture.patch.cells.push_back(c);
    }
    const Tensor tensor=sheared?Tensor{1,-1,0,2,0,1}:Tensor{1,0,0,1,0,1};
    std::shared_ptr<const PlanarReference> reference=std::make_shared<PlanarReference>(fixture.facets);
    if(mapped) {
      std::map<Id,double> errors;for(const auto& f:fixture.facets)errors.emplace(f.id,1e-13);
      auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(reference,errors,7);
      BoundaryPoints points;for(const auto& f:fixture.facets)for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});
      reference=PlanarReference::WithParameters(reference,std::make_shared<BoundaryParameterState>(authority,points,fixture.patch.surface));
    }
    const auto parameters=mapped?reference->Parameters()->Snapshot():std::vector<char>{};
    const auto input=Partition(Rows(fixture.patch),world),saved=input;
    unsigned bad=0;for(const auto& row:input) {
      const auto& v=row.second.cell.v;const auto q=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
      bad+=q.mean_ratio<MinimumMeanRatio || q.minimum_scaled_jacobian<MinimumScaledJacobian;
    }
    if(!height)REQUIRE(world.sum(bad)>0);
    EngineOptions options;options.maximum_rounds=128;options.timings=true;options.surface_tolerance=1e-12;
    if(height) {
      for(const auto& f:fixture.facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))options.wall_heights.emplace(f.marker,.35);
      FirstHeightReport report;std::string reason;
      CHECK_FALSE(CheckFirstHeight(fixture.patch,options.wall_heights,*reference,report,reason));
      CHECK(report.measured);CHECK(report.height_failures>0);
      auto invalid=fixture.patch;invalid.cells.clear();
      CHECK_FALSE(CheckFirstHeight(invalid,options.wall_heights,*reference,report,reason));CHECK_FALSE(report.measured);
    }
    const auto result=Repair(world,input,Uniform(input,tensor),*reference,{},options);INFO(result.reason);
    REQUIRE(result.valid);
    if(height)CHECK(world.maximum(Bytes(result.cells)!=Bytes(input)));
    else CHECK(world.sum(result.stats.moved+result.stats.optimization_validated)>0);
    CHECK(result.stats.cached_invalid_cells==0);CHECK(Bytes(input)==Bytes(saved));
    if(mapped)CHECK(reference->Parameters()->Snapshot()==parameters);
    CHECK(Audit(world,result.cells,fixture,world.sum(int(result.cells.size()))));
    if(height)for(const auto& row:result.cells) {
      Patch patch;patch.cells.push_back(row.second.cell);const auto faces=Faces(row.second.cell);
      for(unsigned k=0;k<4;++k)if(row.second.physical[k].present)patch.surface.push_back({faces[k],row.second.physical[k].facet});
      FirstHeightReport report;std::string reason;
      CHECK(CheckFirstHeight(patch,options.wall_heights,result.reference?*result.reference:*reference,report,reason));
    }
    long double volume=0;
    for(const auto& row:result.cells) {
      const auto& v=row.second.cell.v;const auto q=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
      CHECK(q.mean_ratio>=MinimumMeanRatio);CHECK(q.minimum_scaled_jacobian>=MinimumScaledJacobian);
      CHECK(q.maximum_edge<=MaximumMetricEdge);
      volume+=TetrahedronVolume({v[0].p,v[1].p,v[2].p,v[3].p});
    }
    CHECK(CPassiveComm::Allreduce(double(volume),CPassiveComm::Op::SUM,world.comm)==Approx(1.).margin(1e-12));
    EngineReport(result,world,sheared?"interior_placement_sheared":"interior_placement_uniform");
  }
}

TEST_CASE("Global repair age precedes ownership and reactivated low IDs", "[NativeRepairFairness3D]") {
  World world;
  for(bool reversed:{false,true}) {
    Fixture fixture;Owned input;
    for(int copy=0;copy<2;++copy) {
      auto box=Box();box.patch.cells.clear();const Node center{8,{.95,.93,.9}};
      for(const auto& f:box.facets) {
        Cell c{box.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
        if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
        box.patch.cells.push_back(c);
      }
      for(auto& c:box.patch.cells){c.id+=100*copy;for(auto& n:c.v){n.id+=16*copy;n.p.x+=3*copy;}}
      for(auto& f:box.patch.surface){f.facet+=12*copy;for(auto& id:f.v)id+=16*copy;}
      for(auto& f:box.facets){f.id+=12*copy;for(auto& n:f.v){n.id+=16*copy;n.p.x+=3*copy;}}
      const int owner=(copy==int(reversed)) ? 0 : world.size-1;
      if(owner==world.rank)for(const auto& row:Rows(box.patch))input.emplace(row.cell.id,row);
      fixture.patch.cells.insert(fixture.patch.cells.end(),box.patch.cells.begin(),box.patch.cells.end());
      fixture.patch.surface.insert(fixture.patch.surface.end(),box.patch.surface.begin(),box.patch.surface.end());
      fixture.facets.insert(fixture.facets.end(),box.facets.begin(),box.facets.end());
    }
    const auto saved=Bytes(input);const PlanarReference geometry(fixture.facets);
    EngineOptions options;options.candidate_batch=1;options.maximum_rounds=9;options.maximum_work=4096;
    options.capture_feature={{8,0,0},1};unsigned serviced=0;uint64_t service_round=UINT64_MAX;
    options.transaction_observer=[&](const TransactionView& view) {
      if(view.selected && std::string(view.stage)=="transaction_end") {
        ++serviced;service_round=view.round;
        CHECK(std::all_of(view.old.cells.begin(),view.old.cells.end(),[](const Cell& c) {
          return std::any_of(c.v.begin(),c.v.end(),[](Node n){return n.id==8;});
        }));
      }
    };
    const auto result=Repair(world,input,Uniform(input,Tensor{}),geometry,{},options);INFO(result.reason);
    // At most the eight original corner demands precede this waiting interior star. A newly
    // generated/reactivated corner gets a later epoch, regardless of which worker owns either box.
    CHECK(world.sum(serviced)==1);CHECK(world.maximum(service_round<9));
    CHECK(result.stats.quality_stages>0);CHECK(result.stats.global_work<=options.maximum_work);
    CHECK(Bytes(input)==saved);
    for(const auto& row:result.cells) {
      const auto& v=row.second.cell.v;CHECK(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);
    }
  }
}

TEST_CASE("Quality batch serves old demand and severe independent stars in one round", "[NativeRepairBatch3D]") {
  World world;Fixture fixture;Owned input;const int copies=2*world.size;
  for(int copy=0;copy<copies;++copy) {
    auto box=Box();box.patch.cells.clear();const double x=.95+.049*copy/(copies-1);
    const Node center{8,{x,x,x}};
    for(const auto& f:box.facets) {
      Cell c{box.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
      if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
      box.patch.cells.push_back(c);
    }
    // Give the movable interior star the lowest ID; both independent boxes have real debt.
    auto rename=[](Id id){return id==0?Id(8):id==8?Id(0):id;};
    for(auto& c:box.patch.cells)for(auto& n:c.v)n.id=rename(n.id);
    for(auto& f:box.patch.surface)for(auto& id:f.v)id=rename(id);
    for(auto& f:box.facets)for(auto& n:f.v)n.id=rename(n.id);
    for(auto& c:box.patch.cells){c.id+=100*copy;for(auto& n:c.v){n.id+=16*copy;n.p.x+=3*copy;}}
    for(auto& f:box.patch.surface){f.facet+=12*copy;for(auto& id:f.v)id+=16*copy;}
    for(auto& f:box.facets){f.id+=12*copy;for(auto& n:f.v){n.id+=16*copy;n.p.x+=3*copy;}}
    if(copy%world.size==world.rank)for(const auto& row:Rows(box.patch))input.emplace(row.cell.id,row);
    fixture.facets.insert(fixture.facets.end(),box.facets.begin(),box.facets.end());
  }
  const auto saved=Bytes(input);const PlanarReference geometry(fixture.facets);
  EngineOptions options;options.maximum_rounds=1;options.maximum_work=4096*copies;
  options.capture_feature={{Id(16*(copies-1)),0,0},1};
  unsigned serviced=0;uint64_t round=UINT64_MAX;
  options.transaction_observer=[&](const TransactionView& view) {
    if(view.selected&&std::string(view.stage)=="transaction_end"){++serviced;round=view.round;}
  };
  bool refreshed=false,consistent=true,entered=false;
  size_t entry_q=0,entry_j=0,exit_q=0,exit_j=0;
  options.stage_observer=[&](const StageProgressView& view) {
    if(std::string(view.stage)=="quality_entry"&&!entered) {
      entered=true;entry_q=view.progress.q_bad_cells;entry_j=view.progress.j_bad_cells;
    }
    if(std::string(view.stage)!="exit")return;
    refreshed=true;long double q=0,j=0;size_t qb=0,jb=0;
    for(const auto& row:view.working) {
      const auto& v=row.second.cell.v;const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},Tensor{});
      const auto dq=std::max(0.L,1.L-static_cast<long double>(m.mean_ratio)/MinimumMeanRatio);
      const auto dj=std::max(0.L,1.L-static_cast<long double>(m.minimum_scaled_jacobian)/MinimumScaledJacobian);
      q+=dq*dq;j+=dj*dj;qb+=m.mean_ratio<MinimumMeanRatio;jb+=m.minimum_scaled_jacobian<MinimumScaledJacobian;
    }
    const auto& p=view.progress;
    exit_q=p.q_bad_cells;exit_j=p.j_bad_cells;
    consistent=consistent&&p.cache_consistent&&p.census_complete&&p.shape_complete&&p.geometry_complete&&
        p.q_bad_cells==qb&&p.j_bad_cells==jb&&std::abs(p.q_hinge_squared-q)<1e-10L&&std::abs(p.j_hinge_squared-j)<1e-10L;
  };
  const auto result=Repair(world,input,Uniform(input,Tensor{}),geometry,{},options);INFO(result.reason);
  CHECK(world.sum(serviced)==1);CHECK(world.maximum(round==0));
  CHECK(world.sum(result.stats.batch_candidates)>unsigned(world.size));
  CHECK(world.sum(result.stats.batch_reserved)>unsigned(world.size));
  CHECK(world.sum(result.stats.quality_progress)>0);
  CHECK(entered);CHECK(result.stats.quality.unmeasured_churn==0);
  CHECK(result.stats.quality.private_tracked_collectives==0);CHECK(result.stats.final_reservations==1);
  CHECK(result.stats.maximum_parallel_commits>unsigned(world.size));
  const auto q_removed=CPassiveComm::Allreduce(int64_t(result.stats.quality.q_removed)-int64_t(result.stats.quality.q_created),CPassiveComm::Op::SUM,world.comm);
  const auto j_removed=CPassiveComm::Allreduce(int64_t(result.stats.quality.j_removed)-int64_t(result.stats.quality.j_created),CPassiveComm::Op::SUM,world.comm);
  CHECK(q_removed==CPassiveComm::Allreduce(int64_t(entry_q)-int64_t(exit_q),CPassiveComm::Op::SUM,world.comm));
  CHECK(j_removed==CPassiveComm::Allreduce(int64_t(entry_j)-int64_t(exit_j),CPassiveComm::Op::SUM,world.comm));
  CHECK(refreshed);CHECK(consistent);
  CHECK(result.stats.global_work<=options.maximum_work);CHECK(Bytes(input)==saved);
  for(const auto& row:result.cells){const auto& v=row.second.cell.v;CHECK(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);}
}

TEST_CASE("Quality repair completes a useful pass before revisiting construction debt", "[NativeQualityPass3D]") {
  World world;Fixture fixture;Owned input;
  const int copies=std::max(8,2*world.size);
  auto append=[&](Fixture piece,Id cells,Id nodes,Id facets,double shift,int owner) {
    for(auto& c:piece.patch.cells){c.id+=cells;for(auto& n:c.v){n.id+=nodes;n.p.x+=shift;}}
    for(auto& f:piece.patch.surface){f.facet+=facets;for(auto& id:f.v)id+=nodes;}
    for(auto& f:piece.facets){f.id+=facets;for(auto& n:f.v){n.id+=nodes;n.p.x+=shift;}}
    if(owner==world.rank)for(const auto& row:Rows(piece.patch))input.emplace(row.cell.id,row);
    fixture.patch.cells.insert(fixture.patch.cells.end(),piece.patch.cells.begin(),piece.patch.cells.end());
    fixture.patch.surface.insert(fixture.patch.surface.end(),piece.patch.surface.begin(),piece.patch.surface.end());
    fixture.facets.insert(fixture.facets.end(),piece.facets.begin(),piece.facets.end());
  };
  for(int copy=1;copy<=copies;++copy) {
    auto box=Box();box.patch.cells.clear();const Node center{8,{.95,.93,.9}};
    for(const auto& f:box.facets) {
      Cell c{box.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
      if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
      box.patch.cells.push_back(c);
    }
    append(std::move(box),100*copy,16*copy,12*copy,3*copy,(copy-1)%world.size);
  }
  // This independent long edge cannot be split within the complete-star admission bound.
  append(AxisWheel(70),1000000,1000000,1000000,0,0);
  auto good=Box();const double good_shift=3*(copies+1);
  std::set<std::array<double,3>> unchanged_centroids;
  for(const auto& c:good.patch.cells){const auto p=Centroid(c);unchanged_centroids.insert({p.x+good_shift,p.y,p.z});}
  append(std::move(good),100*(copies+1),16*(copies+1),12*(copies+1),good_shift,0);
  input=WithUniformSensor(std::move(input),Tensor{});
  const auto saved=Bytes(input);const PlanarReference reference(fixture.facets);
  unsigned unchanged_queries=0;
  auto compose=[&](Point p,Tensor tensor){unchanged_queries+=unchanged_centroids.count({p.x,p.y,p.z});return tensor;};
  EngineOptions options;options.maximum_rounds=128*(copies+1);options.maximum_work=4096*copies;
  options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  const auto result=Repair(world,input,Uniform(input,Tensor{}),reference,compose,options);INFO(result.reason);
  CHECK(world.sum(result.stats.moved+result.stats.optimization_validated)>=unsigned(copies));
  CHECK(result.stats.construction_returns<=1);CHECK(result.stats.quality_stages>=1);CHECK(result.stats.quality_stages<=2);
  // The frozen-field census retains unchanged versioned rows across quality stages.
  CHECK(world.sum(unchanged_queries)==unchanged_centroids.size());
  Owned retained;
  for(const auto& row:result.cells)if(row.first>=Id(100*(copies+1)) && row.first<Id(100*(copies+1)+100))retained.emplace(row);
  CHECK(world.sum(int(retained.size()))==int(unchanged_centroids.size()));
  const auto gathered=Gather(world,retained);retained.clear();
  for(const auto& row:gathered)CHECK(retained.emplace(row.cell.id,row).second);
  for(const auto& row:input)if(row.first>=Id(100*(copies+1)) && row.first<Id(100*(copies+1)+100)) {
    const auto found=retained.find(row.first);CHECK(found!=retained.end());
    if(found==retained.end())continue;
    CHECK(found->second.version==row.second.version);
    Owned before{{row.first,row.second}},after{{found->first,found->second}};CHECK(Bytes(before)==Bytes(after));
  }
  CHECK(result.stats.global_work<=options.maximum_work);CHECK(Bytes(input)==saved);
  std::set<int> checked;
  for(const auto& row:result.cells) {
    const auto& c=row.second.cell;
    if(c.v[0].p.x<3 || c.v[0].p.x>3*copies+1)continue;
    const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},Tensor{});
    CHECK(m.mean_ratio>=MinimumMeanRatio);CHECK(m.minimum_scaled_jacobian>=MinimumScaledJacobian);checked.insert(int(std::floor(c.v[0].p.x/3)));
  }
  for(int copy=1;copy<=copies;++copy)CHECK(world.maximum(checked.count(copy)!=0));
  EngineReport(result,world,"quality_pass_with_construction_debt");
}

TEST_CASE("Coarsening revisits short edges created by quality repair", "[NativeCoarsenQualityReturn3D]") {
  World world;const double length=1.272;auto box=Box();Fixture fixture;
  std::map<Id,Node> nodes;
  for(auto f:box.facets)for(auto n:f.v){n.p={length*n.p.x,length*n.p.y,length*n.p.z};nodes.emplace(n.id,n);}
  const Node roof{8,{length/2,length/2,length+.02}},center{9,{length/2,length/2,.05}};
  auto append=[&](std::array<Node,3> triangle) {
    if(Orientation(center.p,triangle[0].p,triangle[1].p,triangle[2].p)<0)std::swap(triangle[1],triangle[2]);
    const Id id=fixture.facets.size();fixture.facets.push_back({id,int(id)+10,triangle});
    fixture.patch.surface.push_back({{triangle[0].id,triangle[1].id,triangle[2].id},id});
    fixture.patch.cells.push_back({id,{center,triangle[0],triangle[1],triangle[2]}});
  };
  for(const auto& f:box.facets) {
    if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==1;}))continue;
    append({nodes.at(f.v[0].id),nodes.at(f.v[1].id),nodes.at(f.v[2].id)});
  }
  const std::array<Id,4> rim{4,5,6,7};
  for(unsigned k=0;k<4;++k)append({nodes.at(rim[k]),nodes.at(rim[(k+1)%4]),roof});
  size_t bad=0,short_edges=0;std::set<Edge> seen;
  for(const auto& c:fixture.patch.cells) {
    const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},Tensor{});
    bad+=m.mean_ratio<MinimumMeanRatio || m.minimum_scaled_jacobian<MinimumScaledJacobian;
    REQUIRE(m.maximum_edge<=MaximumMetricEdge);
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
      Edge e{c.v[i].id,c.v[j].id};std::sort(e.begin(),e.end());if(!seen.insert(e).second)continue;
      const auto a=c.v[i].p,b=c.v[j].p;const double distance=std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);
      if(e[1]==center.id)CHECK(distance>=CoarseningMetricEdge);
      if(distance<CoarseningMetricEdge) {++short_edges;CHECK(e[1]==roof.id);CHECK(e[0]>=4);CHECK(e[0]<=7);}
    }
  }
  REQUIRE(bad>0);REQUIRE(short_edges==4);
  const PlanarReference reference(fixture.facets);const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  EngineOptions options;options.maximum_rounds=256;options.maximum_work=4096;
  const auto result=Coarsen(world,input,Uniform(input,Tensor{}),reference,{},options);INFO(result.reason);
  REQUIRE(result.valid);CHECK(result.stats.construction_returns>=1);
  CHECK(world.sum(result.stats.moved+result.stats.optimization_validated)>0);
  CHECK(world.sum(result.stats.collapse_filtered+result.stats.rejected)>0);CHECK(world.sum(result.collapses.size())>0);
  bool roof_collapse=false;for(const auto& e:result.collapses)roof_collapse=roof_collapse || e[0]==roof.id;
  CHECK(world.maximum(roof_collapse));CHECK(Bytes(input)==saved);
  CHECK(Audit(world,result.cells,fixture,world.sum(result.cells.size())));
  for(const auto& row:result.cells) {
    const auto& c=row.second.cell;const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},Tensor{});
    CHECK(m.mean_ratio>=MinimumMeanRatio);CHECK(m.minimum_scaled_jacobian>=MinimumScaledJacobian);CHECK(m.maximum_edge<=MaximumMetricEdge);
  }
  EngineReport(result,world,"coarsen_after_quality_publication");
}

TEST_CASE("Unproductive coarsening closures restore strict ownership and revisit collateral directions", "[NativeCoarsenQualityReturn3D][NativeCoarsenClosure3D]") {
  World world;
  for(bool stretched:{false,true})for(bool mapped:{false,true}) {
    const double height=stretched?2.:1.;
    // Orthogonal stretch preserves the face-center normal footprints in both triangles.
    auto fixture=Box(height);fixture.patch.cells.clear();const Node center{8,{.5,.5,height/2}};
    for(const auto& f:fixture.facets) {
      Cell c{fixture.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
      if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
      fixture.patch.cells.push_back(c);
    }
    std::shared_ptr<const PlanarReference> reference=std::make_shared<PlanarReference>(fixture.facets);
    if(mapped) {
      std::map<Id,double> errors;for(const auto& f:fixture.facets)errors.emplace(f.id,1e-13);
      auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(reference,errors,7);
      BoundaryPoints points;for(const auto& f:fixture.facets)for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});
      reference=PlanarReference::WithParameters(reference,std::make_shared<BoundaryParameterState>(authority,points,fixture.patch.surface));
    }
    const auto parameters=mapped?reference->Parameters()->Snapshot():std::vector<char>{};
    const Tensor tensor=stretched?Tensor{1,0,0,1,0,.25}:Tensor{};
    const auto input=WithUniformSensor(Partition(Rows(fixture.patch),world),tensor);const auto saved=Bytes(input);
    size_t feasible=0;
    for(Id corner=0;corner<8;++corner) {
      Patch collapsed;std::string reason;
      feasible+=CollapseVertex(fixture.patch,center.id,corner,10000,*reference,collapsed,reason);
    }
    REQUIRE(feasible>=2);
    EngineOptions options;options.candidate_batch=1;options.maximum_work=4096*feasible;
    options.maximum_rounds=128*(feasible+1);options.surface_tolerance=1e-12;
    for(const auto& f:fixture.facets) {
      const auto a=f.v[0].p,b=f.v[1].p,c=f.v[2].p;
      const long double x=((long double)b.y-a.y)*(c.z-a.z)-((long double)b.z-a.z)*(c.y-a.y);
      const long double y=((long double)b.z-a.z)*(c.x-a.x)-((long double)b.x-a.x)*(c.z-a.z);
      const long double z=((long double)b.x-a.x)*(c.y-a.y)-((long double)b.y-a.y)*(c.x-a.x);
      options.wall_heights[f.marker]=double(std::abs(Orientation(a,b,c,center.p))/std::sqrt(x*x+y*y+z*z));
      const bool horizontal=a.z==b.z && b.z==c.z;
      REQUIRE(options.wall_heights.at(f.marker)==Approx(horizontal?height/2:.5));
    }
    FirstHeightReport initial_height;std::string reason;
    REQUIRE(CheckFirstHeight(fixture.patch,options.wall_heights,*reference,initial_height,reason));
    FrozenField initial(Uniform(input,tensor));
    for(const auto& row:input) {
      PatchMetric measured;REQUIRE(ValidateMetricPatch({row.second.cell},initial,measured,reason));
    }
    bool height_diagnostics_match=true,saw_height_debt=false;
    options.stage_observer=[&](const StageProgressView& view) {
      const auto& progress=view.progress;
      if(view.phase==EnginePhase::Construction) {
        if(progress.height_measured_cells==0)return;
        if(progress.height_measured_cells<view.working.size()) {
          height_diagnostics_match &= progress.height_measured_cells+progress.height_unmeasured_cells==view.working.size() &&
              !progress.height_complete && !progress.complete;
          return;
        }
      } else if(!progress.complete)return;
      size_t bad=0;long double debt=0;
      for(const auto& row:view.working) {
        Patch patch;patch.cells.push_back(row.second.cell);const auto faces=Faces(row.second.cell);
        for(unsigned k=0;k<4;++k)if(row.second.physical[k].present)
          patch.surface.push_back({faces[k],row.second.physical[k].facet});
        view.geometry.ExportParameters(patch);
        FirstHeightReport report;std::string why;
        const bool valid=CheckFirstHeight(patch,view.wall_heights,view.geometry,report,why);
        bad+=!valid;
        const auto h=std::max(0.L,static_cast<long double>(report.maximum_relative_error)/view.wall_heights.tolerance-1);
        const auto f=std::max(0.L,-3.L*report.minimum_barycentric);debt+=h*h+f*f;
      }
      height_diagnostics_match &= progress.height_measured_cells==view.working.size() && progress.height_bad_cells==bad &&
          std::abs(progress.height_hinge_squared-debt)<=1e-11L*std::max(1.L,debt);
      saw_height_debt|=bad>0;
    };
    const auto result=Coarsen(world,input,Uniform(input,tensor),*reference,{},options);INFO(result.reason);
    CHECK(height_diagnostics_match);CHECK(world.maximum(saw_height_debt));
    options.stage_observer={};
    REQUIRE(result.valid);CHECK(result.stats.coarsen_checkpoints>0);
    // Every geometrically feasible center removal destroys a requested first height. The
    // retained twelve corner triangles each need their own height-valid incident cell.
    // Each failed trial removes center8, making the other entry directions disappear
    // collaterally; later feasible removals must reappear rather than being silently retired.
    CHECK(result.stats.coarsen_rolled_back_passes==feasible);
    CHECK(world.sum(result.stats.coarsen_rolled_back_collapses)==feasible);
    CHECK(world.sum(result.stats.coarsen_retired_directions)>=feasible);
    CHECK(result.collapses.empty());CHECK(Bytes(result.cells)==saved);CHECK(Bytes(input)==saved);
    CHECK(result.stats.global_work<=options.maximum_work);CHECK(result.stats.cached_invalid_cells==0);
    CHECK(Audit(world,result.cells,fixture,fixture.patch.cells.size()));
    if(mapped) {
      CHECK(reference->Parameters()->Snapshot()==parameters);REQUIRE(result.reference);
      CHECK(result.reference->Parameters()->Snapshot()==parameters);
    }
    for(auto ceiling:{size_t(0),size_t(1)}) {
      auto limited=options;limited.maximum_work=ceiling;
      const auto refused=Coarsen(world,input,Uniform(input,tensor),*reference,{},limited);INFO(refused.reason);
      CHECK_FALSE(refused.valid);CHECK(refused.stats.stop==EngineStop::WorkBudget);
      CHECK(refused.stats.coarsen_rolled_back_passes==0);CHECK(Bytes(input)==saved);
      if(ceiling==1){CHECK(refused.stats.coarsen_checkpoints>0);CHECK(world.sum(refused.stats.accepted)>0);}
      if(mapped)CHECK(reference->Parameters()->Snapshot()==parameters);
    }
    auto limited=options;limited.maximum_rounds=1;
    const auto refused=Coarsen(world,input,Uniform(input,tensor),*reference,{},limited);
    CHECK_FALSE(refused.valid);CHECK(refused.stats.stop==EngineStop::RoundBudget);CHECK(Bytes(input)==saved);
    limited=options;limited.memory_ceiling=1;
    const auto no_storage=Coarsen(world,input,Uniform(input,tensor),*reference,{},limited);
    CHECK_FALSE(no_storage.valid);CHECK(no_storage.stats.coarsen_rolled_back_passes==0);CHECK(Bytes(input)==saved);
    limited=options;limited.maximum_seconds=1e-12;
    const auto timed=Coarsen(world,input,Uniform(input,tensor),*reference,{},limited);
    CHECK_FALSE(timed.valid);CHECK(timed.stats.stop==EngineStop::TimeBudget);CHECK(timed.stats.coarsen_rolled_back_passes==0);
    CHECK(Bytes(input)==saved);
    if(mapped)CHECK(reference->Parameters()->Snapshot()==parameters);
    EngineReport(result,world,"strict_coarsen_closure");
  }
}

TEST_CASE("Metric optimizer moves an interior apex to satisfy requested first height", "[NativeHeightOptimization3D]") {
  auto fixture=Box();fixture.patch.cells.clear();const Node center{8,{.5,.5,.65}};
  for(const auto& f:fixture.facets) {
    Cell c{fixture.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
    if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
    fixture.patch.cells.push_back(c);
  }
  const PlanarReference reference(fixture.facets);Owned mesh;
  for(const auto& row:Rows(fixture.patch))mesh.emplace(row.cell.id,row);
  const auto saved=Bytes(mesh);WallHeights heights;
  for(const auto& f:fixture.facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights.emplace(f.marker,.35);
  FirstHeightReport height;std::string reason;CHECK_FALSE(CheckFirstHeight(fixture.patch,heights,reference,height,reason));
  FrozenField field(Uniform(mesh,Tensor{}));RegionMetric target(field);Id node=9,cell=12;
  OptimizeOptions options;options.geometry=&reference;options.wall_heights=heights;options.maximum_work=1024;
  SECTION("coordinate relaxation alone") {options.swaps=options.collapses=options.boundary=options.insertions=false;}
  SECTION("coordinate relaxation precedes enabled enrichment") {}
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);OptimizeReport report;
  REQUIRE(OptimizeMetricMesh(mesh,target,node,cell,options,report));CHECK(report.moves>0);CHECK(Bytes(mesh)!=saved);
  CHECK(report.shell_insertions+report.neighbourhood_insertions+report.length_insertions==0);CHECK(mesh.size()==fixture.patch.cells.size());
  std::vector<OwnedCell> rows;for(const auto& row:mesh)rows.push_back(row.second);
  const auto final=PatchOf(rows);CHECK(CheckFirstHeight(final,heights,reference,height,reason));
  PatchMetric metric;CHECK(ValidateMetricPatch(final.cells,field,metric,reason));
}

TEST_CASE("Debt optimization preserves admitted first height during shape movement", "[NativeHeightOptimization3D]") {
  auto fixture=Box();fixture.patch.cells.clear();const Node center{8,{.98,.98,.35}};
  for(const auto& f:fixture.facets) {
    Cell c{fixture.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
    if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
    fixture.patch.cells.push_back(c);
  }
  const PlanarReference reference(fixture.facets);Owned mesh;
  for(const auto& row:Rows(fixture.patch))mesh.emplace(row.cell.id,row);
  WallHeights heights;for(const auto& f:fixture.facets)
    if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))heights.emplace(f.marker,.35);
  FirstHeightReport height;std::string reason;
  REQUIRE(CheckFirstHeight(fixture.patch,heights,reference,height,reason));
  FrozenField field(Uniform(mesh,Tensor{}));RegionMetric target(field);
  const auto before=MeasureMetricCells(fixture.patch.cells,field);REQUIRE(before.shape_debt>0);
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);Id node=9,cell=12;OptimizeReport report;
  OptimizeOptions options;options.geometry=&reference;options.wall_heights=heights;options.maximum_work=1024;
  options.swaps=options.collapses=options.boundary=options.insertions=false;
  REQUIRE(OptimizeMetricMesh(mesh,target,node,cell,options,report));CHECK(report.moves>0);
  std::vector<OwnedCell> rows;for(const auto& row:mesh)rows.push_back(row.second);
  const auto final=PatchOf(rows);const auto after=MeasureMetricCells(final.cells,field);
  CHECK(after.shape_debt<before.shape_debt);CHECK(CheckFirstHeight(final,heights,reference,height,reason));
  CHECK(mesh.size()==fixture.patch.cells.size());
  for(const auto& c:final.cells)CHECK(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
}

TEST_CASE("Independent quality stars commit concurrently with final strict acceptance", "[NativeConcurrentRepair3D]") {
  World world;
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  Fixture fixture;Owned input;
  for(int copy=0;copy<world.size;++copy) {
    auto box=Box();box.patch.cells.clear();const Node center{8,{.95,.93,.9}};
    for(const auto& f:box.facets) {
      Cell c{box.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
      if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
      box.patch.cells.push_back(c);
    }
    for(auto& c:box.patch.cells){c.id+=100*copy;for(auto& n:c.v){n.id+=16*copy;n.p.x+=3*copy;}}
    for(auto& f:box.patch.surface){f.facet+=12*copy;for(auto& id:f.v)id+=16*copy;}
    for(auto& f:box.facets){f.id+=12*copy;for(auto& n:f.v){n.id+=16*copy;n.p.x+=3*copy;}}
    if(copy==world.rank)for(const auto& row:Rows(box.patch))input.emplace(row.cell.id,row);
    fixture.patch.cells.insert(fixture.patch.cells.end(),box.patch.cells.begin(),box.patch.cells.end());
    fixture.patch.surface.insert(fixture.patch.surface.end(),box.patch.surface.begin(),box.patch.surface.end());
    fixture.facets.insert(fixture.facets.end(),box.facets.begin(),box.facets.end());
  }
  const auto saved=Bytes(input);const PlanarReference geometry(fixture.facets);
  const Tensor tensor{1,0,0,1,0,1};EngineOptions options;options.maximum_rounds=128;
  Id first_bad_vertex=UINT64_MAX;
  for(const auto& cell:fixture.patch.cells) {
    const auto& v=cell.v;const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
    if(m.mean_ratio<MinimumMeanRatio || m.minimum_scaled_jacobian<MinimumScaledJacobian)
      for(const auto& node:v)first_bad_vertex=std::min(first_bad_vertex,node.id);
  }
  REQUIRE(first_bad_vertex!=UINT64_MAX);
  options.capture_feature={{first_bad_vertex,0,0},1};
  bool optimizer_seen=false,topology_before_optimizer=false;
  options.transaction_observer=[&](const TransactionView& view) {
    if(!view.selected)return;
    const std::string stage=view.stage;
    if(stage=="OptimizeMetricMesh")optimizer_seen=true;
    if(stage=="CollapseVertex" || stage=="Reconnect")topology_before_optimizer|=!optimizer_seen;
    CHECK(view.reason.find("collapse repeats its endpoint")==std::string::npos);
  };
  const auto result=Repair(world,input,Uniform(input,tensor),geometry,{},options);INFO(result.reason);
  CHECK(world.sum(optimizer_seen)==1);CHECK_FALSE(world.maximum(topology_before_optimizer));
  REQUIRE(result.valid);CHECK(result.stats.cached_invalid_cells==0);CHECK(Bytes(input)==saved);
  CHECK(Audit(world,result.cells,fixture,world.sum(int(result.cells.size()))));
  if(world.size>1){CHECK(result.stats.maximum_parallel_commits==unsigned(world.size));CHECK(result.stats.final_reservations>0);}
  for(const auto& row:result.cells) {
    const auto& v=row.second.cell.v;const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
    CHECK(m.mean_ratio>=MinimumMeanRatio);CHECK(m.minimum_scaled_jacobian>=MinimumScaledJacobian);CHECK(m.maximum_edge<=MaximumMetricEdge);
  }
}

TEST_CASE("Metric adaptation resolves opposing length demands before shape census", "[NativeBidirectionalLength3D][NativePhaseOwnership3D]") {
  const bool composed=GENERATE(false,true);
  const bool migrate=GENERATE(false,true);
  World world; Fixture fixture; Owned input;
  const bool reject_transition=migrate&&world.size>1?GENERATE(false,true):false;
  std::vector<char> staged_cells;bool staged_rejection=false;
  auto append=[&](Fixture piece,Id offset,double shift,int owner) {
    for(auto& c:piece.patch.cells){c.id+=offset;for(auto& n:c.v){n.id+=offset;n.p.x+=shift;}}
    for(auto& f:piece.patch.surface){f.facet+=offset;for(auto& id:f.v)id+=offset;}
    for(auto& f:piece.facets){f.id+=offset;for(auto& n:f.v){n.id+=offset;n.p.x+=shift;}}
    if(owner==world.rank)for(const auto& row:Rows(piece.patch))input.emplace(row.cell.id,row);
    fixture.patch.cells.insert(fixture.patch.cells.end(),piece.patch.cells.begin(),piece.patch.cells.end());
    fixture.patch.surface.insert(fixture.patch.surface.end(),piece.patch.surface.begin(),piece.patch.surface.end());
    fixture.facets.insert(fixture.facets.end(),piece.facets.begin(),piece.facets.end());
  };
  auto fan=Box();fan.patch.cells.clear();const Node center{8,{.5,.5,.5}};
  for(const auto& f:fan.facets){Cell c{fan.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
    if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[2],c.v[3]);
    fan.patch.cells.push_back(c);}
  auto second_fan=fan;append(Box(),100,0,0);append(std::move(fan),1000,3,world.size>1?1:0);
  append(std::move(second_fan),2000,6,world.size>2?2:world.size>1?1:0);
  const PlanarReference geometry(fixture.facets);const auto saved=Bytes(input);
  auto source=Uniform(input,Tensor{});
  for(auto& donor:source)for(auto& metric:donor.sensor)
    metric=donor.cell.id<1000?Tensor{4,0,0,4,0,4}:Tensor{.25,0,0,.25,0,.25};
  const auto source_saved=source; // Compare serialized donor records below.
  auto donorBytes=[](std::vector<DonorCell> rows){std::vector<char> b;RecordStream stream(b);for(auto& row:rows)stream(row);return b;};
  bool saw_split=false,saw_collapse=false,construction_unmeasured=true,quality_seen=false;
  uint64_t splits_before_quality=0,collapses_before_quality=0,parallel_before_quality=0;
  bool quality_after_pair=true;std::vector<std::string> length_passes;
  EngineOptions options;options.maximum_work=4096;options.stagnation_window=1;
  options.phase_ownership=migrate?PhaseOwnership::DiagnosticAlways:PhaseOwnership::Disabled;
  unsigned failure_captures=0;
  options.transaction_observer=[&](const TransactionView& view) {
    ++failure_captures;CHECK(view.selected);CHECK_FALSE(view.passed);CHECK_FALSE(view.old.cells.empty());
    CHECK(std::string(view.stage).find("first_")==0);
  };
  if(composed)options.edge_breakpoints=[](Point,Point){return std::vector<double>{0,1};};
  options.stage_observer_bytes=size_t(1)<<20;
  options.stage_observer=[&](const StageProgressView& view){
    if(reject_transition && std::string(view.stage)=="ownership_staged") {
      staged_cells=Bytes(view.working);staged_rejection=true;throw std::runtime_error("injected staged ownership refusal");
    }
    if(std::string(view.stage)=="quality_entry") {
      CHECK(view.progress.census_complete);CHECK(view.progress.shape_complete);
      CHECK(view.progress.measured_cells==view.working.size());
      CHECK(view.progress.shape_unmeasured_cells==0);
    }
    if(view.phase==EnginePhase::Improvement && !quality_seen) {
      quality_after_pair=view.stats.length_split_passes>=1 && view.stats.length_collapse_passes==1;
      quality_seen=true;splits_before_quality=view.stats.length_splits;collapses_before_quality=view.stats.length_collapses;
      parallel_before_quality=view.stats.maximum_parallel_commits;
    }
    if(view.phase!=EnginePhase::Construction || view.stats.quality_stages)return;
    construction_unmeasured &= view.progress.q_measured_cells==0 && view.progress.j_measured_cells==0 && view.progress.shape_measured_cells==0;
    if(std::string(view.stage)=="length_split_exit") {
      saw_split=true;length_passes.emplace_back("split");
    }
    if(std::string(view.stage)=="length_collapse_exit") {
      saw_collapse=true;length_passes.emplace_back("collapse");
    }
  };
  MetricComposition composition;if(composed)composition=[](Point,Tensor sensor){return sensor;};
  const auto result=AdaptMetric(world,input,source,geometry,composition,options);INFO(result.reason);
  if(reject_transition) {
    CHECK_FALSE(result.valid);CHECK(staged_rejection);CHECK(result.stats.ownership_changes==0);
    CHECK(result.reason.find("injected staged ownership refusal")!=std::string::npos);
    CHECK(Bytes(result.cells)==staged_cells);CHECK(Bytes(input)==saved);CHECK(donorBytes(source)==donorBytes(source_saved));return;
  }
  CHECK(world.maximum(!quality_after_pair)==0);
  CHECK(result.stats.bidirectional);CHECK(result.stats.length_split_passes>0);CHECK(result.stats.length_collapse_passes>0);
  CHECK(world.sum(result.stats.length_splits)>0);CHECK(world.sum(result.stats.length_collapses)>0);
  CHECK(world.sum(result.stats.length_pass_deferred)>0);
  CHECK(world.maximum(saw_split));CHECK(world.maximum(saw_collapse));CHECK(world.maximum(!construction_unmeasured)==0);
  REQUIRE_FALSE(length_passes.empty());CHECK(length_passes.front()=="collapse");
  for(size_t k=1;k<length_passes.size();++k)CHECK(length_passes[k]=="split");
  if(world.size>=3)CHECK(world.maximum(parallel_before_quality)>=2);
  CHECK(result.stats.global_work<=options.maximum_work);
  CHECK(failure_captures<=1);if(world.sum(result.stats.rejected)>0)CHECK(world.sum(failure_captures)>0);
  CHECK(world.sum(splits_before_quality)>0);CHECK(world.sum(collapses_before_quality)>0);
  CHECK(Bytes(input)==saved);CHECK(donorBytes(source)==donorBytes(source_saved));
  EngineReport(result,world,"bidirectional_lengths");
  REQUIRE(result.valid);
  if(migrate && world.size>1) {
    CHECK(result.stats.ownership_evaluations==2);CHECK(result.stats.ownership_changes==2);
    CHECK(world.sum(result.stats.ownership_moved_cells)>0);
  } else CHECK(result.stats.ownership_changes==0);
  for(const auto& row:result.cells){const auto& c=row.second.cell;
    CHECK(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
    const Tensor m=c.v[0].p.x<2?Tensor{4,0,0,4,0,4}:Tensor{.25,0,0,.25,0,.25};
    const auto quality=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},m);
    CHECK(quality.mean_ratio>=MinimumMeanRatio);CHECK(quality.minimum_scaled_jacobian>=MinimumScaledJacobian);
    CHECK(quality.maximum_edge<=MaximumMetricEdge);
  }
}

TEST_CASE("Phase ownership preserves a complete census without a subsequent edit", "[NativePhaseOwnership3D]") {
  World world;const auto fixture=Box();const PlanarReference geometry(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  const auto source=Uniform(input,Tensor{});
  bool quality_seen=false;
  EngineOptions options;options.phase_ownership=PhaseOwnership::DiagnosticAlways;
  options.maximum_work=64;options.maximum_rounds=1;
  options.stage_observer=[&](const StageProgressView& view) {
    if(std::string(view.stage)!="quality_entry")return;
    quality_seen=true;
    CHECK(view.progress.census_complete);CHECK(view.progress.shape_complete);
    CHECK(view.progress.geometry_complete);CHECK(view.progress.unmeasured_cells==0);
    CHECK(view.progress.shape_unmeasured_cells==0);
    CHECK(view.stats.ownership_changes==(world.size>1?2:0));
  };
  const auto result=AdaptMetric(world,input,source,geometry,{},options);INFO(result.reason);
  CHECK(world.maximum(quality_seen));CHECK(result.stats.attempted==0);
  CHECK(result.stats.ownership_evaluations==(world.size>1?2:0));
  CHECK(result.stats.ownership_changes==(world.size>1?2:0));
  CHECK(Bytes(input)==saved);REQUIRE(result.valid);
}

TEST_CASE("Completed metric size hands residual curved geometry to a quality census", "[NativeConstructionHandoff3D]") {
  World world;auto fixture=Box();fixture.patch.cells.clear();const Node center{8,{.5,.5,.5}};
  for(const auto& f:fixture.facets) {
    Cell cell{fixture.patch.cells.size(),{center,f.v[0],f.v[1],f.v[2]}};
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[2],cell.v[3]);
    fixture.patch.cells.push_back(cell);
  }
  auto root=std::make_shared<PlanarReference>(fixture.facets);
  SU2NativeGeometry::MappedFacetReference::CurvedNets controls;std::map<Id,double> errors;
  const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
  for(const auto& f:fixture.facets) {
    errors.emplace(f.id,1e-12);auto& net=controls[f.id];
    for(unsigned q=0;q<10;++q) {
      long double x=0,y=0,z=0;
      for(unsigned k=0;k<3;++k){x+=powers[q][k]*(long double)f.v[k].p.x;y+=powers[q][k]*(long double)f.v[k].p.y;z+=powers[q][k]*(long double)f.v[k].p.z;}
      net[q]={double(x/3),double(y/3),double(z/3)};
    }
    if(f.id==0) {
      net[9].x+=.04*(net[9].x-center.p.x);
      net[9].y+=.04*(net[9].y-center.p.y);
      net[9].z+=.04*(net[9].z-center.p.z);
    }
  }
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(root,errors,29,SIZE_MAX,false,controls);
  BoundaryPoints points;for(const auto& f:fixture.facets)for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});
  auto parameters=std::make_shared<BoundaryParameterState>(authority,points,fixture.patch.surface);
  auto geometry=PlanarReference::WithParameters(root,parameters);const auto snapshot=parameters->Snapshot();
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  bool geometry_only=false;
  SECTION("Complete quality-entry census"){ }
  SECTION("Geometry demand waits for closed shape and size"){geometry_only=true;}
  bool split_exit=false,quality_seen=false,geometry_seen=false;size_t geometry_at_exit=0;
  EngineOptions options;options.maximum_work=4096;options.maximum_rounds=256;options.surface_tolerance=1e-4;
  options.phase_ownership=PhaseOwnership::DiagnosticAlways;
  options.stage_observer=[&](const StageProgressView& view) {
    if(view.phase==EnginePhase::Construction && !view.stats.quality_stages) {
      CHECK(view.progress.q_measured_cells==0);CHECK(view.progress.j_measured_cells==0);
    }
    if(std::string(view.stage)=="length_split_exit") {
      split_exit=true;geometry_at_exit=view.progress.geometry_bad_cells;
      CHECK(view.stats.length_split_passes==1);CHECK(view.progress.census_complete);
      CHECK(view.progress.cache_consistent);CHECK(view.progress.geometry_complete);
      CHECK(view.progress.size_bad_cells==0);
    }
    if(std::string(view.stage)=="geometry_repair_entry") {
      geometry_seen=true;
      CHECK(view.phase==EnginePhase::Improvement);CHECK(view.progress.shape_complete);
      CHECK(view.progress.q_bad_cells==0);CHECK(view.progress.j_bad_cells==0);
      CHECK(view.progress.size_bad_cells==0);CHECK(view.progress.geometry_complete);
      CHECK(world.maximum(view.progress.geometry_bad_cells!=0));
      CHECK(world.maximum(view.progress.remaining_repairs!=0));
      throw std::runtime_error("bounded geometry-only repair witness");
    }
    if(std::string(view.stage)!="quality_entry")return;
    quality_seen=true;CHECK(view.stats.length_split_passes==1);
    CHECK(view.stats.ownership_changes==(world.size>1?2:0));
    CHECK(view.progress.census_complete);CHECK(view.progress.shape_complete);
    CHECK(view.progress.geometry_complete);CHECK_FALSE(view.valid);
    if(!geometry_only)throw std::runtime_error("bounded residual geometry handoff witness");
  };
  const auto result=AdaptMetric(world,input,Uniform(input,Tensor{}),*geometry,{},options);INFO(result.reason);
  CHECK(world.maximum(split_exit));CHECK(world.sum(geometry_at_exit)>0);CHECK(world.maximum(quality_seen));
  CHECK(world.sum(result.stats.length_splits)>0);CHECK_FALSE(result.valid);
  CHECK(world.maximum(geometry_seen)==geometry_only);
  CHECK(result.reason.find(geometry_only?"bounded geometry-only repair witness":"bounded residual geometry handoff witness")!=std::string::npos);
  CHECK(Bytes(input)==saved);CHECK(parameters->Snapshot()==snapshot);
  CHECK(result.stats.global_work<=options.maximum_work);
  for(const auto& row:result.cells){const auto& v=row.second.cell.v;CHECK(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);}
}

TEST_CASE("Metric improvement isolates remaining shape debt after construction", "[NativeBidirectionalLength3D]") {
  World world;Fixture fixture;Owned input;
  auto append=[&](Fixture piece,Id offset,double shift,int owner) {
    for(auto& c:piece.patch.cells){c.id+=offset;for(auto& n:c.v){n.id+=offset;n.p.x+=shift;}}
    for(auto& f:piece.patch.surface){f.facet+=offset;for(auto& id:f.v)id+=offset;}
    for(auto& f:piece.facets){f.id+=offset;for(auto& n:f.v){n.id+=offset;n.p.x+=shift;}}
    if(owner==world.rank)for(const auto& row:Rows(piece.patch))input.emplace(row.cell.id,row);
    fixture.facets.insert(fixture.facets.end(),piece.facets.begin(),piece.facets.end());
  };
  append(Box(),0,0,0);append(Box(.02),1000,3,world.size>1?1:0);
  const PlanarReference geometry(fixture.facets);const auto saved=Bytes(input);
  auto source=Uniform(input,Tensor{});
  for(auto& donor:source)for(auto& metric:donor.sensor)
    metric=donor.cell.id<1000?Tensor{4,0,0,4,0,4}:Tensor{.25,0,0,.25,0,.25};
  bool seen_quality=false,good_shape=false,good_size_bad=false,bad_shape=false;unsigned good_root_selected=0;
  EngineOptions options;options.maximum_work=256;options.maximum_rounds=64;options.capture_feature={{0,0,0},1};
  options.transaction_observer=[&](const TransactionView& view){if(view.selected)++good_root_selected;};
  options.stage_observer=[&](const StageProgressView& view){
    if(seen_quality || std::string(view.stage)!="quality_entry")return;seen_quality=true;
    for(const auto& row:view.working){const auto& v=row.second.cell.v;
      const bool good=v[0].p.x<2;const Tensor tensor=good?Tensor{4,0,0,4,0,4}:Tensor{.25,0,0,.25,0,.25};
      const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
      const bool shape=m.mean_ratio>=MinimumMeanRatio && m.minimum_scaled_jacobian>=MinimumScaledJacobian;
      good_shape|=good && shape;good_size_bad|=good && m.maximum_edge>MaximumMetricEdge;bad_shape|=!good && !shape;
    }
  };
  const auto result=AdaptMetric(world,input,source,geometry,{},options);INFO(result.reason);
  CHECK(world.maximum(seen_quality));CHECK(world.maximum(good_shape));CHECK_FALSE(world.maximum(good_size_bad));CHECK(world.maximum(bad_shape));
  CHECK(world.sum(good_root_selected)==0);CHECK(Bytes(input)==saved);
  if(result.valid)for(const auto& row:result.cells){const auto& v=row.second.cell.v;
    const Tensor tensor=v[0].p.x<2?Tensor{4,0,0,4,0,4}:Tensor{.25,0,0,.25,0,.25};
    const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
    CHECK(m.mean_ratio>=MinimumMeanRatio);CHECK(m.minimum_scaled_jacobian>=MinimumScaledJacobian);CHECK(m.maximum_edge<=MaximumMetricEdge);
  }
}

TEST_CASE("Metric size closes in construction or residual repair", "[NativeBidirectionalLength3D]") {
  World world;const auto fixture=Box();const PlanarReference geometry(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  const Tensor tensor{4,0,0,4,0,4};const auto source=Uniform(input,tensor);
  bool quality_seen=false,residual_size=false,size_seen=false;
  EngineOptions options;options.maximum_work=4096;
  options.stage_observer=[&](const StageProgressView& view) {
    if(std::string(view.stage)=="quality_entry") {
      quality_seen=true;residual_size|=world.maximum(view.progress.size_bad_cells!=0);
    }
    if(std::string(view.stage)!="size_repair_entry")return;
    size_seen=true;CHECK(view.phase==EnginePhase::Improvement);
    CHECK(view.progress.shape_complete);CHECK(view.progress.q_bad_cells==0);CHECK(view.progress.j_bad_cells==0);
    CHECK(world.maximum(view.progress.size_bad_cells!=0));
    CHECK(world.maximum(view.progress.remaining_repairs!=0));
  };
  const auto result=AdaptMetric(world,input,source,geometry,{},options);INFO(result.reason);
  CHECK(world.maximum(quality_seen));CHECK_FALSE((world.maximum(residual_size)&&!world.maximum(size_seen)));CHECK(Bytes(input)==saved);
  REQUIRE(result.valid);CHECK(result.stats.global_work<=options.maximum_work);
  for(const auto& row:result.cells) {
    const auto& v=row.second.cell.v;const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},tensor);
    CHECK(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);
    CHECK(m.mean_ratio>=MinimumMeanRatio);CHECK(m.minimum_scaled_jacobian>=MinimumScaledJacobian);
    CHECK(m.maximum_edge<=MaximumMetricEdge);
  }
}

TEST_CASE("Construction pass claims only committed cavity vertices", "[NativeConstructionPassClaims3D]") {
  World world;auto rows=Rows(Box().patch),other=rows;
  for(auto& row:other){row.cell.id+=100;for(auto& node:row.cell.v){node.id+=100;node.p.x+=3;}
    for(auto& face:row.physical)if(face.present)face.facet+=100;rows.push_back(row);}
  auto owned=Partition(rows,world);Directory directory(world);std::string reason;
  REQUIRE(directory.Build(owned,reason));
  std::vector<Dependency> removed;std::vector<OwnedCell> added;
  if(owned.count(0)){auto row=owned.at(0);removed.push_back({row.cell.id,row.version,world.rank});++row.version;added.push_back(row);}
  const auto feature=Feature{{0,0,0},1};
  directory.BeginConstructionPass();
  const auto initial=directory.Snapshot(),mesh=Bytes(owned);
  CHECK_FALSE(directory.Publish(owned,removed,added,world.rank!=0,reason));
  CHECK(directory.Snapshot()==initial);CHECK(Bytes(owned)==mesh);
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,world.rank?SIZE_MAX:1));
  CHECK(directory.Snapshot()==initial);CHECK(Bytes(owned)==mesh);
  auto deps=directory.Lookup(Query(world,feature));REQUIRE(deps.valid);
  auto imported=directory.Fetch(deps,owned);REQUIRE(imported.valid);
  const bool before=directory.Reserve(imported,world.rank==0,reason);
  CHECK((world.rank!=0 || before));CHECK_FALSE(directory.PassDeferred());
  REQUIRE(directory.Publish(owned,removed,added,true,reason));
  deps=directory.Lookup(Query(world,feature));REQUIRE(deps.valid);
  imported=directory.Fetch(deps,owned);REQUIRE(imported.valid);
  const bool held=directory.Reserve(imported,world.rank==0,reason);
  if(!world.rank){CHECK_FALSE(held);CHECK(directory.PassDeferred());}
  auto separate=directory.Lookup(Query(world,Feature{{100,0,0},1}));REQUIRE(separate.valid);
  auto enlarged=directory.Fetch(separate,owned);REQUIRE(enlarged.valid);
  CHECK(directory.Reserve(enlarged,world.rank==0,reason)==(world.rank==0));CHECK_FALSE(directory.PassDeferred());
  if(!world.rank)enlarged.cells.insert(enlarged.cells.end(),imported.cells.begin(),imported.cells.end());
  const bool expanded=directory.Reserve(enlarged,world.rank==0,reason);
  if(!world.rank){CHECK_FALSE(expanded);CHECK(directory.PassDeferred());}
  const auto batch=directory.LookupBatch(Query(world,feature));REQUIRE(batch.valid);
  const auto payload=directory.FetchBatch(batch,owned);REQUIRE(payload.valid);
  const auto reserved=directory.ReserveBatch(payload);REQUIRE(reserved.valid);
  if(!world.rank){CHECK(reserved.chosen.size()==1);if(reserved.chosen.size()==1){CHECK_FALSE(reserved.chosen[0]);CHECK(reserved.pass_deferred[0]);}}
  std::vector<size_t> slots;const auto plan=directory.PlanBatch(Query(world,feature),slots);REQUIRE(plan.valid);
  if(!world.rank){CHECK(plan.pass_deferred==std::vector<size_t>{0});CHECK(slots.empty());}
  ImportedBatch interiors;interiors.valid=true;
  if(!world.rank){Imported one;one.valid=true;one.cells.push_back(owned.at(0));interiors.candidates.push_back(one);}
  const auto interior=directory.ReserveInteriors(interiors);REQUIRE(interior.valid);
  if(!world.rank){CHECK(interior.chosen.size()==1);if(interior.chosen.size()==1){CHECK(interior.chosen[0]);CHECK_FALSE(interior.pass_deferred[0]);}}
  directory.BeginConstructionPass();
  CHECK(directory.Reserve(imported,world.rank==0,reason)==(world.rank==0));CHECK_FALSE(directory.PassDeferred());
  const auto fresh=directory.Snapshot();
  CHECK_FALSE(directory.Publish(owned,{}, {},world.rank!=0,reason));CHECK(directory.Snapshot()==fresh);
  CHECK(directory.Reserve(imported,world.rank==0,reason)==(world.rank==0));CHECK_FALSE(directory.PassDeferred());
  directory.EndConstructionPass();
  // A fresh full-star reservation after the claim lane releases previous pass claims,
  // but overlapping owners must still elect exactly one winner.
  const auto released=directory.Snapshot();
  const auto shared_dependencies=directory.Lookup(Query(world,feature,true));REQUIRE(shared_dependencies.valid);
  const auto shared_star=directory.Fetch(shared_dependencies,owned);REQUIRE(shared_star.valid);
  const bool shared=directory.Reserve(shared_star,true,reason);
  CHECK(world.sum(shared)==1);CHECK(shared==(world.rank==0));CHECK_FALSE(directory.PassDeferred());
  CHECK(directory.Snapshot()==released);
  const bool after=directory.Reserve(imported,world.rank==0,reason);
  CHECK((world.rank!=0 || after));CHECK_FALSE(directory.PassDeferred());
}

TEST_CASE("Metric lifecycle disagreement is refused collectively", "[NativeBidirectionalLength3D]") {
  World world;if(world.size<2){SUCCEED("Mixed worker entry points require multiple ranks.");return;}
  const auto fixture=Box();const auto owned=Partition(Rows(fixture.patch),world);const auto saved=Bytes(owned);
  const PlanarReference geometry(fixture.facets);const auto donors=Uniform(owned,Tensor{});
  const auto result=world.rank?Refine(world,owned,donors,geometry):AdaptMetric(world,owned,donors,geometry);
  CHECK_FALSE(result.valid);CHECK(result.reason.find("intent differs across workers")!=std::string::npos);
  CHECK(Bytes(owned)==saved);
}

TEST_CASE("Cumulative atlas and backend failure stop all workers before construction", "[NativeStageCapture3D][NativeConstructionSweep3D][NativeAtlasWorkStop3D]") {
  const bool diagnostic_error=GENERATE(false,true);
  const bool geometry_error=GENERATE(false,true);
  World world;const auto fixture=Box();auto root=std::make_shared<PlanarReference>(fixture.facets);
  std::map<Id,double> errors;std::map<int,int> roles;
  for(const auto& f:fixture.facets){errors.emplace(f.id,1e-13);roles[f.marker]=2;}
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(root,errors,7);
  SU2NativeGeometry::AtlasBudget setup{size_t(1)<<28,size_t(1)<<29,4096};
  SU2NativeGeometry::MobilityPolicy policy;std::string reason;
  REQUIRE(authority->BuildMobilityPolicy(45,roles,policy,setup,reason));
  BoundaryPoints points;for(const auto& f:fixture.facets)for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});
  auto parameters=std::make_shared<BoundaryParameterState>(authority,points,fixture.patch.surface,1,SIZE_MAX,0,
      SU2NativeGeometry::BoundaryCoverage{},policy,&setup);
  auto reference=PlanarReference::WithParameters(root,parameters);const auto snapshot=parameters->Snapshot();
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);const auto source=Uniform(input,{4,0,0,4,0,4});
  unsigned fired=0,later=0;EngineOptions options;options.candidate_batch=1;options.surface_tolerance=1e-3;options.capture_feature={{0,6,0},2};
  options.transaction_observer=[&](const TransactionView& view){
    if(!view.selected)return;
    if(std::string(view.stage)=="begin") {
      ++fired;auto* budget=view.geometry.AtlasReservation();REQUIRE(budget);REQUIRE(budget->work<budget->maximum_work);
      if(geometry_error)budget->geometry_failure="deliberate geometry backend failure";
      else {CHECK_FALSE(budget->ChargeWork(budget->maximum_work-budget->work+1));CHECK(budget->work<budget->maximum_work);}
      if(diagnostic_error)throw std::runtime_error("deliberate concurrent diagnostic failure");
    } else if(std::string(view.stage)=="SplitEdge" || std::string(view.stage)=="construction_end" || std::string(view.stage)=="transaction_end")++later;
  };
  const auto result=Refine(world,input,source,*reference,{},options);INFO(result.reason);
  CHECK_FALSE(result.valid);CHECK(result.stats.stop==(geometry_error?EngineStop::Failure:EngineStop::WorkBudget));
  if(geometry_error)CHECK(result.reason.find("deliberate geometry backend failure")!=std::string::npos);
  if(diagnostic_error)CHECK(result.reason.find("deliberate concurrent diagnostic failure")!=std::string::npos);
  CHECK(world.sum(fired)==1);CHECK(world.sum(later)==0);CHECK(world.sum(result.stats.accepted)==0);
  CHECK(result.stats.quality_stages==0);CHECK(Bytes(input)==saved);CHECK(Bytes(result.cells)==Bytes(WithUniformSensor(input,{4,0,0,4,0,4})));CHECK(parameters->Snapshot()==snapshot);
}
