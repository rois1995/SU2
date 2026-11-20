/*!
 * \file CNativeLocalCore3DShift_tests.cpp
 * \brief Local tetrahedral core border exchange and two-pass MPI fixtures.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <set>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;

constexpr int BoxX = 8, BoxYZ = 4;
constexpr Id PointBase = Id(1) << 56, CellBase = Id(1) << 60;
constexpr Id PassOnePoint = PointBase + 1000, InterfacePoint = PointBase + 1001;

Node GridPoint(int x, int y, int z) {
  return {PointBase + Id((z * (BoxYZ + 1) + y) * (BoxX + 1) + x), {double(x), double(y), double(z)}};
}

Tensor BoxSensor(Point p) {
  return {1 + p.x / 1024, p.z / 4096, 0, 1 + p.y / 512, p.x / 4096, 1 + p.z / 512};
}

std::array<Node, 3> FaceNodes(const Cell& cell, unsigned face) {
  constexpr unsigned order[4][3] = {{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};
  return {cell.v[order[face][0]], cell.v[order[face][1]], cell.v[order[face][2]]};
}

int BoundaryMarker(const std::array<Node, 3>& face) {
  const auto a = face[0].p, b = face[1].p, c = face[2].p;
  if (a.x == b.x && a.x == c.x && (a.x == 0 || a.x == BoxX)) return a.x == 0 ? 0 : 1;
  if (a.y == b.y && a.y == c.y && (a.y == 0 || a.y == BoxYZ)) return a.y == 0 ? 2 : 3;
  if (a.z == b.z && a.z == c.z && (a.z == 0 || a.z == BoxYZ)) return a.z == 0 ? 4 : 5;
  return -1;
}

Owned Box() {
  constexpr std::array<unsigned, 6> ring{1, 2, 3, 7, 4, 5};
  Owned rows;
  for (int z = 0; z < BoxYZ; ++z)
    for (int y = 0; y < BoxYZ; ++y)
      for (int x = 0; x < BoxX; ++x) {
        const std::array<Node, 8> v{GridPoint(x, y, z), GridPoint(x + 1, y, z), GridPoint(x + 1, y + 1, z),
                                    GridPoint(x, y + 1, z), GridPoint(x, y, z + 1), GridPoint(x + 1, y, z + 1),
                                    GridPoint(x + 1, y + 1, z + 1), GridPoint(x, y + 1, z + 1)};
        for (size_t k = 0; k < ring.size(); ++k) {
          const auto id = CellBase + 13 * rows.size();
          OwnedCell row;
          row.cell = {id, {v[0], v[6], v[ring[k]], v[ring[(k + 1) % ring.size()]]}};
          auto& tet = row.cell.v;
          if (Orientation(tet[0].p, tet[1].p, tet[2].p, tet[3].p) < 0) std::swap(tet[0], tet[1]);
          row.version = 1 + rows.size() % 17;
          row.sensor_present = true;
          for (unsigned f = 0; f < 4; ++f) {
            row.physical[f] = {BoundaryMarker(FaceNodes(row.cell, f)) >= 0, 1000 + 4 * rows.size() + f};
            row.sensor[f] = BoxSensor(tet[f].p);
          }
          rows.emplace(id, row);
        }
      }
  return rows;
}

void AddShortStar(Owned& rows, int x, Id point) {
  const auto a = GridPoint(x, 2, 2), b = GridPoint(x, 3, 2);
  const Node split{point, {double(x), 2. + 3. / 16., 2.}};
  auto next = rows.rbegin()->first + 1;
  std::vector<OwnedCell> children;
  for (auto entry = rows.begin(); entry != rows.end();) {
    const auto& row = entry->second;
    const auto& tet = row.cell.v;
    const auto ca = std::find_if(tet.begin(), tet.end(), [&](Node p) { return p.id == a.id; });
    const auto cb = std::find_if(tet.begin(), tet.end(), [&](Node p) { return p.id == b.id; });
    if (ca == tet.end() || cb == tet.end()) {
      ++entry;
      continue;
    }
    REQUIRE(std::none_of(row.physical.begin(), row.physical.end(), [](FacetBinding face) { return face.present; }));
    for (const auto k : {ca - tet.begin(), cb - tet.begin()}) {
      auto child = row;
      child.cell.id = next++;
      child.cell.v[k] = split;
      child.sensor[k] = BoxSensor(split.p);
      children.push_back(child);
    }
    entry = rows.erase(entry);
  }
  REQUIRE_FALSE(children.empty());
  for (const auto& child : children) REQUIRE(rows.emplace(child.cell.id, child).second);
}

Owned Slab(const Owned& rows, World& world, double first_boundary) {
  Owned owned;
  const auto width = double(BoxX) / world.size;
  for (const auto& entry : rows) {
    const auto x = Centroid(entry.second.cell).x;
    int owner = 0;
    while (owner + 1 < world.size && x >= first_boundary + owner * width) ++owner;
    if (owner == world.rank) owned.emplace(entry);
  }
  return owned;
}

template <class... T>
std::vector<char> Bytes(T... values) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  stream(values...);
  return bytes;
}

struct RankedRow {
  int rank = 0;
  OwnedCell row;
};
using GlobalRows = std::map<Id, RankedRow>;

GlobalRows Gather(const Owned& owned, World& world) {
  std::vector<char> bytes;
  RecordStream write(bytes);
  for (auto entry : owned) {
    int rank = world.rank;
    write(rank, entry.second);
  }
  const auto all = CPassiveComm::Allgatherv(bytes, nullptr, world.comm);
  RecordStream read(all);
  GlobalRows rows;
  while (!read.End()) {
    RankedRow row;
    read(row.rank, row.row);
    const auto id = row.row.cell.id;
    REQUIRE(rows.emplace(id, std::move(row)).second);
  }
  return rows;
}

Face Key(std::array<Node, 3> face) {
  Face key{face[0].id, face[1].id, face[2].id};
  std::sort(key.begin(), key.end());
  return key;
}

struct FaceUse {
  int rank;
  FacetBinding binding;
  bool even;
};

struct NodeUse {
  std::set<int> owners, surface_owners;
  std::vector<char> bytes;
};

struct MeshAudit {
  std::map<Id, NodeUse> nodes;
  std::map<Face, std::pair<Id, bool>> skin;
  std::vector<std::set<Face>> borders;
};

MeshAudit Audit(const GlobalRows& rows, int ranks) {
  MeshAudit audit;
  audit.borders.resize(ranks);
  std::map<Face, std::vector<FaceUse>> faces;
  std::set<std::array<Id, 4>> simplices;
  for (const auto& [id, owned] : rows) {
    const auto& cell = owned.row.cell;
    const auto& v = cell.v;
    REQUIRE(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0);
    REQUIRE(owned.row.sensor_present);
    std::array<Id, 4> key{};
    for (unsigned f = 0; f < 4; ++f) {
      key[f] = v[f].id;
      auto& node = audit.nodes[v[f].id];
      const auto bytes = Bytes(v[f], owned.row.sensor[f]);
      if (node.owners.empty()) node.bytes = bytes;
      REQUIRE(node.bytes == bytes);
      node.owners.insert(owned.rank);
      const auto face = FaceNodes(cell, f);
      const bool even = ((face[0].id > face[1].id) + (face[0].id > face[2].id) + (face[1].id > face[2].id)) % 2 == 0;
      const auto binding = owned.row.physical[f];
      faces[Key(face)].push_back({owned.rank, binding, even});
      if (binding.present)
        for (const auto& p : face) audit.nodes[p.id].surface_owners.insert(owned.rank);
    }
    std::sort(key.begin(), key.end());
    CAPTURE(id, key);
    REQUIRE(simplices.insert(key).second);
  }
  for (const auto& [face, uses] : faces) {
    CAPTURE(face);
    const auto& a = uses.front();
    if (uses.size() == 1) {
      REQUIRE(a.binding.present);
      audit.skin.emplace(face, std::make_pair(a.binding.facet, a.even));
      continue;
    }
    REQUIRE(uses.size() == 2);
    const auto& b = uses.back();
    REQUIRE((a.even != b.even && !a.binding.present && !b.binding.present));
    if (a.rank != b.rank) {
      audit.borders[a.rank].insert(face);
      audit.borders[b.rank].insert(face);
    }
  }
  return audit;
}

void CheckBorderFaces(const local::Mesh& mesh, const MeshAudit& audit, int rank) {
  std::set<Face> observed;
  for (const auto& cell : mesh.Cells()) {
    Cell keyed;
    for (unsigned k = 0; k < 4; ++k) keyed.v[k] = mesh.Points()[cell.vertices[k]].node;
    for (unsigned f = 0; f < 4; ++f)
      if (cell.neighbors[f] == local::Border) CHECK(observed.insert(Key(FaceNodes(keyed, f))).second);
  }
  CHECK(observed == audit.borders[rank]);
}

size_t CheckMeeting(local::Mesh& mesh, const MeshAudit& audit, const local::BorderMeeting& meeting, World& world) {
  std::vector<Id> expected;
  for (const auto& [id, node] : audit.nodes)
    if (node.owners.size() > 1 && id % world.size == unsigned(world.rank)) expected.push_back(id);
  CHECK(meeting.rendezvous == expected);
  mesh.MarkBorderSurface(meeting);
  size_t count = 0, remote_only = 0;
  for (const auto& point : mesh.Points()) {
    const auto id = point.node.id;
    const auto& node = audit.nodes.at(id);
    const auto* record = meeting.Find(id);
    const bool border = node.owners.size() > 1;
    CAPTURE(world.rank, id);
    CHECK(point.border == border);
    CHECK(point.surface == !node.surface_owners.empty());
    CHECK(point.Free() == !(border || !node.surface_owners.empty()));
    CHECK((record != nullptr) == border);
    if (record) {
      ++count;
      CHECK(record->lower == *node.owners.begin());
      CHECK(record->upper == *node.owners.rbegin());
      CHECK(record->surface == !node.surface_owners.empty());
    }
    remote_only += !node.surface_owners.empty() && !node.surface_owners.count(world.rank);
  }
  CHECK(meeting.points.size() == count);
  return remote_only;
}

std::set<Id> FreedByLower(const GlobalRows& rows, const MeshAudit& audit) {
  std::set<Id> freed;
  for (const auto& [id, node] : audit.nodes)
    if (node.owners.size() == 2 && node.surface_owners.empty()) freed.insert(id);
  for (const auto& entry : rows) {
    const auto& v = entry.second.row.cell.v;
    int lower = entry.second.rank;
    for (const auto& p : v) lower = std::min(lower, *audit.nodes.at(p.id).owners.begin());
    for (const auto& p : v)
      if (lower < *audit.nodes.at(p.id).owners.begin()) freed.erase(p.id);
  }
  return freed;
}

size_t BothMeetings(const MeshAudit& first, const MeshAudit& second, World& world) {
  size_t count = 0;
  for (const auto& [id, node] : first.nodes) {
    if (node.owners.size() < 2 || id % world.size != unsigned(world.rank)) continue;
    const auto other = second.nodes.find(id);
    count += other != second.nodes.end() && other->second.owners.size() > 1;
  }
  return count;
}

void CheckShift(const GlobalRows& before, const GlobalRows& after, const MeshAudit& audit, local::Direction direction,
                const local::ShiftStats& stats, World& world) {
  REQUIRE(before.size() == after.size());
  size_t sent = 0, received = 0, cells_before = 0, cells_after = 0;
  for (const auto& [id, prior] : before) {
    CAPTURE(id);
    REQUIRE(after.count(id) == 1);
    const auto& current = after.at(id);
    CHECK(Bytes(prior.row) == Bytes(current.row));
    int destination = prior.rank;
    for (const auto& p : prior.row.cell.v) {
      const auto& owners = audit.nodes.at(p.id).owners;
      destination = direction == local::Direction::Lower ? std::min(destination, *owners.begin())
                                                        : std::max(destination, *owners.rbegin());
    }
    CHECK(current.rank == destination);
    cells_before += prior.rank == world.rank;
    cells_after += current.rank == world.rank;
    sent += prior.rank == world.rank && current.rank != world.rank;
    received += prior.rank != world.rank && current.rank == world.rank;
  }
  CHECK(stats.sent == sent);
  CHECK(stats.received == received);
  CHECK(stats.cells_before == cells_before);
  CHECK(stats.cells_after == cells_after);
}

std::set<Edge> CheckLengths(local::Mesh& mesh, const local::IdSequence& ids, const local::PassStats& stats) {
  std::set<Edge> long_edges;
  mesh.MarkWallNear();
  constexpr uint32_t stamp = UINT32_MAX;
  for (const auto& use : mesh.Edges()) {
    const auto edge = use.edge;
    const auto length = mesh.Length(edge);
    if (length > local::LongEdge) {
      Edge key{mesh.Points()[edge[0]].node.id, mesh.Points()[edge[1]].node.id};
      std::sort(key.begin(), key.end());
      long_edges.insert(key);
      if (mesh.ClosedRing(edge, use.cell)) {
        auto trial = mesh;
        auto point_ids = ids;
        local::PassStats probe;
        const auto reason = trial.Split(edge, use.cell, point_ids, stamp, SIZE_MAX, probe);
        CHECK(((reason == local::Refusal::Validity && (stats.refused_valid || stats.limit)) ||
               (reason == local::Refusal::Size && probe.refused_size == 1) ||
               (reason == local::Refusal::Memory && probe.refused_memory == 1) ||
               (reason == local::Refusal::Blocked && probe.blocked == 1)));
      }
    }
    if (length < local::ShortEdge) {
      local::PassStats probe;
      CHECK((mesh.Collapse(edge, stamp, probe, false) != local::Refusal::None || stats.limit));
    }
  }
  return long_edges;
}

double WorstMargin(const local::Mesh& mesh) {
  double worst = std::numeric_limits<double>::infinity();
  for (const auto& cell : mesh.Cells())
    if (cell.live) worst = std::min(worst, mesh.Margin(cell.vertices));
  return worst;
}

local::PassStats AdaptPass(local::Mesh& mesh, Owned& rows, local::IdSequence& points, local::IdSequence& cells) {
  local::PassStats stats;
  mesh.LengthWork(points, SIZE_MAX, stats);
  const auto allowed_long = CheckLengths(mesh, points, stats);
  const auto margin = WorstMargin(mesh);
  mesh.QualityWork(points, SIZE_MAX, stats);
  CHECK(WorstMargin(mesh) >= margin);
  for (const auto& use : mesh.Edges()) {
    if (mesh.Length(use.edge) <= local::LongEdge) continue;
    Edge key{mesh.Points()[use.edge[0]].node.id, mesh.Points()[use.edge[1]].node.id};
    std::sort(key.begin(), key.end());
    CHECK(allowed_long.count(key) == 1);
  }
  CHECK_NOTHROW(mesh.Check());
  mesh.WriteBack(rows, cells);
  return stats;
}

void CheckFrozen(const MeshAudit& before, const MeshAudit& after) {
  CHECK(after.skin == before.skin);
  for (const auto& [id, node] : before.nodes) {
    if (node.owners.size() == 1 && node.surface_owners.empty()) continue;
    CAPTURE(id);
    const auto found = after.nodes.find(id);
    REQUIRE(found != after.nodes.end());
    CHECK(found->second.bytes == node.bytes);
    CHECK(found->second.owners == node.owners);
  }
}

Owned TwoPasses(const Owned& box, World& world) {
  auto rows = Slab(box, world, double(BoxX) / world.size);
  local::MemoryBudget memory{SIZE_MAX, 0, sizeof(Owned::value_type) + 4 * sizeof(void*)};
  local::IdSequence point_ids(InterfacePoint, world.rank, world.size);
  local::IdSequence cell_ids(box.rbegin()->first, world.rank, world.size);

  const auto initial_rows = Gather(rows, world);
  const auto initial = Audit(initial_rows, world.size);
  const auto first = local::MeetBorderPoints(world, rows, memory);
  local::PassStats pass_one;
  {
    local::Mesh mesh(rows, true);
    CheckBorderFaces(mesh, initial, world.rank);
    CheckMeeting(mesh, initial, first, world);
    pass_one = AdaptPass(mesh, rows, point_ids, cell_ids);
  }
  const auto before_lower = Gather(rows, world);
  const auto after_one = Audit(before_lower, world.size);
  CheckFrozen(initial, after_one);
  const auto freed = FreedByLower(before_lower, after_one);
  REQUIRE(freed.count(InterfacePoint) == 1);
  const auto lower = local::ShiftBorders(world, rows, first, local::Direction::Lower, memory);
  const auto after_lower = Gather(rows, world);
  const auto second_mesh = Audit(after_lower, world.size);
  CheckShift(before_lower, after_lower, after_one, local::Direction::Lower, lower, world);

  const auto second = local::MeetBorderPoints(world, rows, memory);
  local::PassStats pass_two;
  {
    local::Mesh mesh(rows, true);
    CheckBorderFaces(mesh, second_mesh, world.rank);
    CheckMeeting(mesh, second_mesh, second, world);
    for (const auto id : freed) {
      CHECK(second_mesh.nodes.at(id).owners == std::set<int>{*after_one.nodes.at(id).owners.begin()});
    }
    for (const auto& point : mesh.Points())
      if (freed.count(point.node.id)) CHECK(point.Free());
    pass_two = AdaptPass(mesh, rows, point_ids, cell_ids);
  }
  const auto before_higher = Gather(rows, world);
  const auto after_two = Audit(before_higher, world.size);
  CheckFrozen(second_mesh, after_two);
  const auto higher = local::ShiftBorders(world, rows, second, local::Direction::Higher, memory, &first);
  const auto final_rows = Gather(rows, world);
  const auto final = Audit(final_rows, world.size);
  CheckShift(before_higher, final_rows, after_two, local::Direction::Higher, higher, world);
  CheckBorderFaces(local::Mesh(rows, true), final, world.rank);
  CHECK(higher.frozen_in_both == BothMeetings(initial, second_mesh, world));
  CHECK(final.nodes.count(PassOnePoint) == 0);
  CHECK(final.nodes.count(InterfacePoint) == 0);
  CHECK(final.skin == initial.skin);

  const auto c1 = world.reduce<uint64_t>(pass_one.collapses, CPassiveComm::Op::SUM);
  const auto c2 = world.reduce<uint64_t>(pass_two.collapses, CPassiveComm::Op::SUM);
  const auto sent_lower = world.reduce<uint64_t>(lower.sent, CPassiveComm::Op::SUM);
  const auto sent_higher = world.reduce<uint64_t>(higher.sent, CPassiveComm::Op::SUM);
  const auto frozen = world.reduce<uint64_t>(higher.frozen_in_both, CPassiveComm::Op::SUM);
  CHECK(c1 > 0);
  CHECK(c2 > 0);
  CHECK(sent_lower > 0);
  CHECK(sent_higher > 0);
  if (!world.rank)
    std::cout << "FP1_FIXTURE shifts ranks=" << world.size << " pass1_collapses=" << c1 << " pass2_collapses=" << c2
              << " lower_sent=" << sent_lower << " higher_sent=" << sent_higher << " freed=" << freed.size()
              << " frozen_in_both=" << frozen << " rows=" << final_rows.size() << '\n';
  return rows;
}

std::vector<Facet> Reference(const Owned& rows) {
  std::vector<Facet> facets;
  for (const auto& entry : rows) {
    const auto& row = entry.second;
    for (unsigned f = 0; f < 4; ++f) {
      if (!row.physical[f].present) continue;
      const auto face = FaceNodes(row.cell, f);
      facets.push_back({row.physical[f].facet, BoundaryMarker(face), face});
    }
  }
  return facets;
}
}  // namespace

TEST_CASE("Local tetrahedral border meetings propagate remote physical surface marks", "[.][NativeLocalCore3DShift]") {
  World world;
  REQUIRE((world.size == 2 || world.size == 4));
  const auto rows = Slab(Box(), world, .625);
  const auto audit = Audit(Gather(rows, world), world.size);
  local::MemoryBudget memory{SIZE_MAX, 0, sizeof(Owned::value_type) + 4 * sizeof(void*)};
  const auto meeting = local::MeetBorderPoints(world, rows, memory);
  local::Mesh mesh(rows, true);
  CheckBorderFaces(mesh, audit, world.rank);
  const auto remote = world.reduce<uint64_t>(CheckMeeting(mesh, audit, meeting, world), CPassiveComm::Op::SUM);
  CHECK(remote == 9);
  if (!world.rank) std::cout << "FP1_FIXTURE remote_surface ranks=" << world.size << " points=" << remote << '\n';
}

TEST_CASE("Local tetrahedral slab passes and shifts keep exact mesh rows and reach the engine",
          "[.][NativeLocalCore3DShift]") {
  World world;
  REQUIRE((world.size == 2 || world.size == 4));
  auto box = Box();
  AddShortStar(box, 3, PassOnePoint);
  AddShortStar(box, 4, InterfacePoint);
  const auto rows = TwoPasses(box, world);
  const auto repeat = TwoPasses(box, world);
  CHECK(rows.size() == repeat.size());
  for (const auto& [id, row] : rows) {
    CAPTURE(id);
    CHECK(repeat.count(id) == 1);
    if (repeat.count(id)) CHECK(Bytes(row) == Bytes(repeat.at(id)));
  }
  const PlanarReference reference(Reference(box));
  std::vector<DonorCell> donors;
  for (const auto& entry : rows) donors.push_back({entry.second.cell, entry.second.sensor});
  const auto result = Refine(world, rows, donors, reference, {}, {});
  INFO(result.reason);
  CHECK_FALSE(world.maximum(!result.valid));
  if (!world.rank)
    std::cout << "FP1_FIXTURE refine ranks=" << world.size << " valid=" << result.valid << " rounds=" << result.stats.rounds
              << " accepted=" << result.stats.accepted << '\n';
}
