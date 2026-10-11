/*!
 * \file CNativeDistributed3D_tests.cpp
 * \brief Complete MPI stars, private coupled edits, atomic rejection and subset migration.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeDistributed3D.hpp"
#include "../../../Common/include/adaptation/CNativeDonor3D.hpp"
#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include <cmath>
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
      if (geometry.Get(face.facet).marker != int(face.facet) + 10) valid = false;
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
       << " rounds=" << s.rounds << " proposed=" << s.proposed << " attempts=" << s.attempted
       << " accepted=" << s.accepted << " rejected=" << s.rejected << " conflicts=" << s.conflicts
       << " collapse_geometry_checks=" << s.collapse_geometry_checks << " collapse_filtered=" << s.collapse_filtered
       << " collapse_geometry_nested_s=" << s.collapse_geometry_seconds
       << " selection_edge_evaluations=" << s.edge_evaluations << " selection_edge_reused=" << s.edge_reused
       << " field_requests=" << s.field_requests
       << " field_hits=" << s.field_hits << " field_edges=" << s.field_edges << " work_bytes=" << s.maximum_work_bytes
       << " total_s=" << s.total_seconds << " init_s=" << s.initialization_seconds << " seed_s=" << s.seed_seconds
       << " selection_s=" << s.selection_seconds << " dependency_s=" << s.dependency_seconds
       << " reservation_s=" << s.reservation_seconds << " donor_s=" << s.donor_seconds
       << " reconstruction_s=" << s.reconstruction_seconds << " metric_s=" << s.metric_seconds
       << " routing_s=" << s.routing_seconds << " publication_s=" << s.publication_seconds
       << " update_s=" << s.update_seconds << " completion_s=" << s.completion_seconds << " other_s=" << s.other_seconds
       << " process_cpu_s=" << s.process_cpu_seconds << " reconstruction_cpu_s=" << s.reconstruction_cpu_seconds
       << " metric_cpu_s=" << s.metric_cpu_seconds << " measurement_cpu_s=" << s.measurement_cpu_seconds
       << " repair_fallbacks=" << s.repair_fallbacks << " fallback_accepted=" << s.fallback_accepted
       << " recovery_starts=" << s.recovery_starts << " repair_proposals=" << s.repair_proposals << " moved=" << s.moved << " reconnected=" << s.reconnected << " inserted=" << s.inserted
       << " batches=" << s.donor_batches << " batch_retries=" << s.batch_retries
       << " query_import_retries=" << s.query_import_retries << " private_query_retries=" << s.private_query_retries
       << " donor_candidates=" << s.donor_candidates << " donor_filtered=" << s.donor_filtered
       << " donor_payloads=" << s.donor_payloads << " donor_maximum_patch=" << s.donor_maximum_patch
       << " donor_search_nested_s=" << s.donor_search_seconds << " donor_filter_nested_s=" << s.donor_filter_seconds
       << " field_build_nested_s=" << s.field_build_seconds << " field_query_nested_s=" << s.field_query_seconds
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
TEST_CASE("Native MPI refinement preserves accepted input on incomplete or invalid events", "[NativeDistributed3D]") {
  World world;
  const auto fixture=Box();const PlanarReference reference(fixture.facets);
  const auto input=Partition(Rows(fixture.patch),world);const auto saved=Bytes(input);
  EngineOptions options;options.maximum_rounds=1;
  auto limited=Refine(world,input,Uniform(input,{4,0,0,4,0,4}),reference,{},options);
  CHECK_FALSE(limited.valid);
  CHECK(limited.reason.find("round limit")!=std::string::npos);
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
  CHECK(Bytes(unchanged.cells)==saved);
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
          if(coarsening) { CHECK(result.splits.empty()); CHECK(result.collapses.size()+result.stats.moved+result.stats.reconnected+result.stats.inserted==result.stats.accepted); }
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
  CHECK(world.sum(int(result.stats.collapse_filtered))>0);
  CHECK(world.sum(int(result.stats.collapse_geometry_checks))>0);
  CHECK(result.stats.collapse_geometry_seconds>=0);CHECK(result.stats.other_seconds>=-1e-12);
  CHECK(Bytes(result.cells)==saved);CHECK(Bytes(input)==saved);
  EngineReport(result,world,"immutable_corner_filter");
  options.maximum_rounds=1;
  const auto rejected=Coarsen(world,input,Uniform(input,Tensor{16,0,0,16,0,16}),reference,{},options);
  CHECK_FALSE(rejected.valid);CHECK(rejected.stats.collapse_filtered==0);
  if (rejected.stats.attempted) CHECK(rejected.reason.find(": Native 3D movement proposal leaves an incident immutable facet.")!=std::string::npos);
  CHECK(Bytes(input)==saved);CHECK(rejected.stats.other_seconds>=-1e-12);
}

TEST_CASE("Native automatic MPI repair selects complete failing features and restores frozen gates", "[NativeDistributed3D][NativeAutomaticRepair3D]") {
  World parent;using Clock=std::chrono::steady_clock;
  auto elapsed=[](Clock::time_point start){return std::chrono::duration<double>(Clock::now()-start).count();};
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
          auto result=mode=="restoreR" ? Refine(worker,owned,source_rows,geometry,{},options) :
              (mode=="restoreC" ? Coarsen(worker,owned,source_rows,geometry,{},options) : Repair(worker,owned,source_rows,geometry,{},options));
          if(!result.valid)std::cerr<<"AUTO_REPAIR_FAILURE "<<label<<" rank="<<worker.rank<<" "<<result.reason<<'\n';
          valid=result.valid;CHECK(valid);CHECK(Bytes(owned)==owned_saved);CHECK(result.stats.other_seconds>=-1e-12);
          CHECK(worker.sum(int(result.stats.accepted))>0);
          CHECK(result.stats.recovery_starts<=1);
          CHECK(result.stats.rounds<=options.maximum_rounds);
          CHECK(result.stats.initial_cells==owned.size());
          CHECK(result.splits.size()+result.collapses.size()+result.stats.moved+result.stats.reconnected+result.stats.inserted==result.stats.accepted);
          if(mode=="auto") {
            CHECK(worker.sum(int(kind=="wall" || kind=="interior" ? result.stats.moved : result.stats.reconnected))>0);
            CHECK(result.splits.empty());CHECK(result.collapses.empty());
          }
          if(mode=="restoreR" && kind!="face") CHECK(result.stats.recovery_starts==1);
          if(mode=="restoreC") CHECK(result.splits.empty());
          if(mode=="restoreC" && kind=="edge4") CHECK(worker.sum(int(result.stats.fallback_accepted))>0);
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
        if(mode=="auto") CHECK(count==(kind=="face" ? 2u : fixture.patch.cells.size()));
        else { CHECK(count>0);CHECK(count<=12);if(mode=="restoreC") CHECK(count<=fixture.patch.cells.size()); }
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
