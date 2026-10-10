/*!
 * \file CNativeDistributed3D_tests.cpp
 * \brief Complete MPI stars, private coupled edits, atomic rejection and subset migration.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeDistributed3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <set>
using namespace SU2Native3D;
namespace {
Face Key(Face face) { std::sort(face.begin(), face.end()); return face; }
std::array<Face, 4> Faces(const Cell& c) {
  const auto& v = c.v;
  return {{{v[1].id, v[2].id, v[3].id}, {v[0].id, v[3].id, v[2].id},
           {v[0].id, v[1].id, v[3].id}, {v[0].id, v[2].id, v[1].id}}};
}
struct Fixture { Patch patch; std::vector<Facet> facets; };
Fixture Box() {
  const std::array<Node, 8> nodes{{{0, {0, 0, 0}}, {1, {1, 0, 0}}, {2, {1, 1, 0}}, {3, {0, 1, 0}},
                                 {4, {0, 0, 1}}, {5, {1, 0, 1}}, {6, {1, 1, 1}}, {7, {0, 1, 1}}}};
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
  Patch fresh;
  if (world.rank == 0 && approved) try {
    const auto old = PatchOf(imported.cells);
    const PlanarReference reference(geometry.facets);
    approved = reverse ? UndoEdgeSplit(old, 8, {0, 6}, 200, reference, fresh, reason) :
                         SplitEdge(old, {0, 6}, {8, {.5, .5, .5}}, 100, reference, fresh, reason);
    std::vector<DonorCell> donors;
    for (const auto& cell : old.cells) {
      DonorCell donor;
      donor.cell = cell;
      donor.sensor.fill(reverse ? Tensor{} : Tensor{4, 0, 0, 4, 0, 4});
      donors.push_back(donor);
    }
    FrozenField target(donors);
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
