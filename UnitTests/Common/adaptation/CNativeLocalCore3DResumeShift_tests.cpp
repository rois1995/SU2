/*!
 * \file CNativeLocalCore3DResumeShift_tests.cpp
 * \brief Two nontrivial local core passes across slab and block partitions.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;

constexpr int BoxX = 8, BoxYZ = 4;
constexpr Id PointBase = Id(1) << 56, CellBase = Id(1) << 60;

Node GridPoint(int x, int y, int z) {
  return {PointBase + Id((z * (BoxYZ + 1) + y) * (BoxX + 1) + x), {double(x), double(y), double(z)}};
}

Tensor Metric(Point p) {
  // Field 1 uses a 0.5I boundary, a y ramp from x=4 to 7, a z ridge at
  // (x,z)=(4,3), and relative shear magnitudes (0.38, 0.35, 0.45).
  if (p.x == 0 || p.x == BoxX || p.y == 0 || p.y == BoxYZ || p.z == 0 || p.z == BoxYZ)
    return {.5, 0, 0, .5, 0, .5};
  const double y = p.y / BoxYZ, z = p.z / BoxYZ;
  const double fine_y = std::clamp((p.x - 4) / 3, 0., 1.);
  const double fine_z = std::max(0., 1 - std::abs(p.z - 3)) * std::max(0., 1 - std::abs(p.x - 4) / 3);
  const double weak_x = std::clamp(p.x - 2, 0., 1.) * std::clamp((BoxX - p.x) / 2, 0., 1.);
  const double weak_scale = .0015 + .0065 * y * y + .001 * z;
  const double xx = (1 - weak_x) * (.20 + .15 * y) + weak_x * weak_scale;
  const double yy = .5 + 4.5 * fine_y, zz = .5 + 4.5 * fine_z;
  // Vary the weak direction across the ridge. The normalized shear is diagonally dominant.
  const double xy = .38 * (z == .25 ? -1 : 1);
  const double xz = .35 * (y == .75 ? -1 : 1);
  return {xx, xy * std::sqrt(xx * yy), xz * std::sqrt(xx * zz), yy, .45 * std::sqrt(yy * zz), zz};
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
            row.sensor[f] = Metric(tet[f].p);
          }
          rows.emplace(id, row);
        }
      }
  return rows;
}

Owned Layout(const Owned& box, const World& world) {
  Owned rows;
  for (const auto& entry : box) {
    const auto p = Centroid(entry.second.cell);
    const int owner = int(p.x >= 3.625) + (world.size == 4 ? 2 * int(p.y >= 3.625) : 0);
    if (owner == world.rank) rows.emplace(entry);
  }
  return rows;
}

template <class... T>
std::vector<char> Payload(T... values) {
  std::vector<char> bytes;
  RecordStream writer(bytes);
  writer(values...);
  return bytes;
}

struct RankedRow {
  int owner = 0;
  OwnedCell row;
};
using GlobalRows = std::map<Id, RankedRow>;

GlobalRows Gather(const Owned& local_rows, const World& world) {
  std::vector<char> send;
  RecordStream send_rows(send);
  for (const auto& entry : local_rows) {
    auto row = entry.second;
    int owner = world.rank;
    send_rows(owner, row);
  }
  const auto received = CPassiveComm::Allgatherv(send, nullptr, world.comm);
  RecordStream read(received);
  GlobalRows rows;
  while (!read.End()) {
    RankedRow decoded;
    read(decoded.owner, decoded.row);
    const Id id = decoded.row.cell.id;
    REQUIRE(rows.emplace(id, std::move(decoded)).second);
  }
  return rows;
}

struct NodeUse {
  std::set<int> owners;
  std::vector<char> payload;
};

struct FaceUse {
  FacetBinding binding;
  bool even;
};

struct MeshAudit {
  std::map<Id, NodeUse> vertices;
  std::map<Face, std::pair<Id, bool>> skin;
};

MeshAudit Audit(const GlobalRows& rows) {
  MeshAudit audit;
  std::map<Face, std::vector<FaceUse>> incidence;
  std::set<std::array<Id, 4>> simplices;
  for (const auto& [id, owned] : rows) {
    const auto& row = owned.row;
    const auto& v = row.cell.v;
    CAPTURE(id);
    REQUIRE(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0);
    REQUIRE(row.sensor_present);
    std::array<Id, 4> simplex{};
    for (unsigned f = 0; f < 4; ++f) {
      simplex[f] = v[f].id;
      auto& use = audit.vertices[v[f].id];
      const auto bytes = Payload(v[f], row.sensor[f]);
      if (use.owners.empty()) use.payload = bytes;
      REQUIRE(use.payload == bytes);
      use.owners.insert(owned.owner);
      const auto face = FaceNodes(row.cell, f);
      Face key{face[0].id, face[1].id, face[2].id};
      const bool even = ((key[0] > key[1]) + (key[0] > key[2]) + (key[1] > key[2])) % 2 == 0;
      std::sort(key.begin(), key.end());
      incidence[key].push_back({row.physical[f], even});
    }
    std::sort(simplex.begin(), simplex.end());
    REQUIRE(simplices.insert(simplex).second);
  }
  for (const auto& [face, uses] : incidence) {
    CAPTURE(face);
    const auto& a = uses.front();
    if (uses.size() == 1) {
      REQUIRE(a.binding.present);
      audit.skin.emplace(face, std::make_pair(a.binding.facet, a.even));
    } else {
      REQUIRE(uses.size() == 2);
      REQUIRE_FALSE((a.binding.present || uses.back().binding.present));
      REQUIRE(a.even != uses.back().even);
    }
  }
  return audit;
}

size_t PersistentBorder(const MeshAudit& before, const MeshAudit& after, const World& world) {
  size_t count = 0;
  for (const auto& [id, point] : before.vertices) {
    if (id % world.size != static_cast<unsigned>(world.rank) || point.owners.size() < 2) continue;
    const auto found = after.vertices.find(id);
    count += found != after.vertices.end() && found->second.owners.size() > 1;
  }
  return count;
}

std::array<uint64_t, 5> Totals(const local::PassStats& stats, World& world) {
  std::array<uint64_t, 5> values{stats.collapses, stats.splits, stats.edge_removals, stats.face_swaps, stats.moves};
  for (auto& value : values) value = world.reduce<uint64_t>(value, CPassiveComm::Op::SUM);
  return values;
}

local::PassStats Adapt(Owned& rows, local::IdSequence& points, local::IdSequence& cells,
                       const local::BorderMeeting& meeting, const World& world, unsigned pass) {
  local::PassStats stats;
  CLocalFailure failure;
  try {
    local::Mesh mesh(rows, true);
    mesh.MarkBorderSurface(meeting);
    stats = mesh.RunPass(points, SIZE_MAX, true);
    mesh.Check();
    mesh.WriteBack(rows, cells);
  } catch (const std::exception& error) {
    failure.Set(1, 0, error.what());
  }
  const auto elected = world.elect(failure);
  CAPTURE(pass);
  INFO(elected.message);
  REQUIRE_FALSE(elected.any);
  return stats;
}

void ReportPass(const World& world, unsigned pass, const std::array<uint64_t, 5>& counts) {
  if (world.rank) return;
  std::cout << "FP1_F4_SHIFT_PASS ranks=" << world.size << " pass=" << pass << " collapses=" << counts[0]
            << " splits=" << counts[1] << " edge_removals=" << counts[2] << " face_swaps=" << counts[3]
            << " moves=" << counts[4] << std::endl;
}

Owned TwoPasses(const Owned& box, World& world) {
  auto rows = Layout(box, world);
  const auto before = Audit(Gather(rows, world));
  const Id last_cell = box.rbegin()->first;
  local::IdSequence cells(last_cell, world.rank, world.size);
  local::IdSequence points(before.vertices.rbegin()->first, world.rank, world.size);
  local::MemoryBudget memory{SIZE_MAX, 0, sizeof(Owned::value_type) + 4 * sizeof(void*)};

  const auto three = std::count_if(before.vertices.begin(), before.vertices.end(),
                                  [](const auto& entry) { return entry.second.owners.size() == 3; });
  const auto four = std::count_if(before.vertices.begin(), before.vertices.end(),
                                 [](const auto& entry) { return entry.second.owners.size() == 4; });
  if (world.size == 4) {
    CHECK(three > 0);
    CHECK(four > 0);
  }

  const auto first = local::MeetBorderPoints(world, rows, memory);
  const auto pass_one = Totals(Adapt(rows, points, cells, first, world, 1), world);
  ReportPass(world, 1, pass_one);
  size_t changed_at_first_border = 0;
  for (const auto& [id, row] : rows) {
    if (id <= last_cell) continue;
    changed_at_first_border += std::any_of(row.cell.v.begin(), row.cell.v.end(),
                                           [&](const auto& p) { return first.Find(p.id) != nullptr; });
  }
  const auto changed_border = world.reduce<uint64_t>(changed_at_first_border, CPassiveComm::Op::SUM);
  local::ShiftBorders(world, rows, first, local::Direction::Lower, memory);
  const auto after_lower = Audit(Gather(rows, world));
  const auto second = local::MeetBorderPoints(world, rows, memory);
  const auto pass_two = Totals(Adapt(rows, points, cells, second, world, 2), world);
  ReportPass(world, 2, pass_two);
  const auto higher = local::ShiftBorders(world, rows, second, local::Direction::Higher, memory, &first);
  const auto final_rows = Gather(rows, world);
  const auto final = Audit(final_rows);
  const auto frozen = world.reduce<uint64_t>(higher.frozen_in_both, CPassiveComm::Op::SUM);
  CHECK(higher.frozen_in_both == PersistentBorder(before, after_lower, world));
  if (world.size == 4) CHECK(frozen > 0);
  CHECK(after_lower.skin == before.skin);
  CHECK(final.skin == before.skin);
  CHECK(final_rows.size() != box.size());
  INFO("First and second pass counts are collapse, split, edge removal, face swap, move");
  CAPTURE(pass_one, pass_two);
  for (auto count : pass_one) CHECK(count > 0);
  CHECK(pass_two[0] > 0);
  CHECK(pass_two[1] > 0);
  CHECK(pass_two[2] + pass_two[3] + pass_two[4] > 0);
  CHECK(changed_border > 0);

  if (!world.rank) {
    auto print = [](const auto& counts) { for (auto count : counts) std::cout << ',' << count; };
    std::cout << "FP1_F4_SHIFT ranks=" << world.size << " three_rank_points=" << three << " four_rank_points=" << four
              << " frozen_in_both=" << frozen << " changed_at_border=" << changed_border
              << " box_cells=" << box.size() << " final_cells=" << final_rows.size() << " pass1=";
    print(pass_one);
    std::cout << " pass2=";
    print(pass_two);
    std::cout << std::endl;
  }
  return rows;
}

std::vector<Facet> Reference(const Owned& box) {
  std::vector<Facet> facets;
  for (const auto& entry : box) {
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

TEST_CASE("Local tetrahedral passes and shifts edit a slab and a multiway block partition",
          "[.][NativeLocalCore3DResumeShift]") {
  World world;
  REQUIRE((world.size == 2 || world.size == 4));
  if (!world.rank)
    std::cout << "FP1_F4_SHIFT_FIELD ranks=" << world.size << " variant=1" << std::endl;
  const auto box = Box();
  const auto rows = TwoPasses(box, world);
  const auto repeat = TwoPasses(box, world);
  REQUIRE(rows.size() == repeat.size());
  for (const auto& [id, row] : rows) {
    CAPTURE(id);
    REQUIRE(repeat.count(id) == 1);
    CHECK(Payload(row) == Payload(repeat.at(id)));
  }
  const PlanarReference reference(Reference(box));
  std::vector<DonorCell> donors;
  for (const auto& entry : rows) donors.push_back({entry.second.cell, entry.second.sensor});
  if (!world.rank)
    std::cout << "FP1_F4_SHIFT_REFINE_BEGIN ranks=" << world.size << std::endl;
  const auto result = Refine(world, rows, donors, reference, {}, {});
  INFO(result.reason);
  CHECK_FALSE(world.maximum(!result.valid));
  if (!world.rank)
    std::cout << "FP1_F4_SHIFT_REFINE ranks=" << world.size << " valid=" << result.valid
              << " rounds=" << result.stats.rounds << " accepted=" << result.stats.accepted << std::endl;
}
