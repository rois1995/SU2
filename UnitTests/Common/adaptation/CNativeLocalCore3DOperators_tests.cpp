/*!
 * \file CNativeLocalCore3DOperators_tests.cpp
 * \brief Small cavities for native local remeshing and atomic refusal.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {
using namespace SU2Native3D;
using namespace SU2NativeLocal3D;
using FaceKey = std::array<Id, 3>;
using Tet = std::array<size_t, 4>;

constexpr Id PointId(size_t i) { return 100 + i; }

FaceKey OrientedFace(const Cell& cell, unsigned slot) {
  FaceKey face{};
  for (unsigned i = 0, j = 0; i < 4; ++i)
    if (i != slot) face[j++] = cell.v[i].id;
  if (slot % 2) std::swap(face[1], face[2]);
  return face;
}

FaceKey Sorted(FaceKey face) {
  std::sort(face.begin(), face.end());
  return face;
}

bool Odd(const FaceKey& face) {
  return ((face[0] > face[1]) + (face[0] > face[2]) + (face[1] > face[2])) % 2 != 0;
}

Tensor AffineTensor(Point p) {
  return {1 + p.x / 4, p.z / 16, 0, 1 + p.y / 4, p.x / 32, 1 + p.z / 4};
}

std::array<double, 6> Components(Tensor m) { return {m.xx, m.xy, m.xz, m.yy, m.yz, m.zz}; }

Owned BoundMesh(const std::vector<Point>& points, const std::vector<Tet>& cells, bool affine = false) {
  Owned result;
  std::map<FaceKey, std::vector<std::pair<Id, unsigned>>> faces;
  for (const auto& tet : cells) {
    OwnedCell row;
    row.cell.id = 1000 + 7 * result.size();
    row.version = 10 + result.size();
    row.sensor_present = true;
    auto& v = row.cell.v;
    for (unsigned i = 0; i < 4; ++i) v[i] = {PointId(tet[i]), points.at(tet[i])};
    if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) < 0) std::swap(v[0], v[1]);
    REQUIRE(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0);
    for (unsigned i = 0; i < 4; ++i) {
      if (affine) row.sensor[i] = AffineTensor(v[i].p);
      faces[Sorted(OrientedFace(row.cell, i))].emplace_back(row.cell.id, i);
    }
    result.emplace(row.cell.id, row);
  }
  Id mark = 500;
  for (const auto& face : faces) {
    REQUIRE(face.second.size() <= 2);
    if (face.second.size() != 1) continue;
    const auto [row, slot] = face.second.front();
    result.at(row).physical[slot] = {true, mark++};
  }
  return result;
}

Owned Bipyramid(double height, bool ring, bool offset = false, bool affine = false) {
  const double x = offset ? 2 : 0;
  const double y = offset ? 2 : 0;
  const std::vector<Point> points{{.75, 0, 0}, {-.375, .65, 0}, {-.375, -.65, 0},
                                  {x, y, height}, {x, y, -height}};
  if (ring) return BoundMesh(points, {{3, 4, 0, 1}, {3, 4, 1, 2}, {3, 4, 2, 0}}, affine);
  return BoundMesh(points, {{0, 1, 2, 3}, {0, 2, 1, 4}}, affine);
}

std::vector<Tet> FillStar(size_t center, size_t first) {
  std::vector<Tet> cells;
  for (size_t omit = 0; omit < 4; ++omit) {
    Tet tet{center};
    for (size_t i = 0, j = 1; i < 4; ++i)
      if (i != omit) tet[j++] = first + i;
    cells.push_back(tet);
  }
  return cells;
}

Owned LayeredTetra() {
  std::vector<Point> points{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {0, 0, 2}};
  for (size_t i = 0; i < 4; ++i) {
    const auto p = points[i];
    points.push_back({.375 + .25 * p.x, .375 + .25 * p.y, .375 + .25 * p.z});
  }
  points.push_back({.5, .5, .5});
  std::vector<Tet> cells;
  for (size_t omit = 0; omit < 4; ++omit) {
    std::array<size_t, 3> face{};
    for (size_t i = 0, j = 0; i < 4; ++i)
      if (i != omit) face[j++] = i;
    const auto [a, b, c] = face;
    // The same vertex order on each prism gives matching diagonals on its shared sides.
    cells.insert(cells.end(), {{a, b, c, c + 4}, {a, b, b + 4, c + 4}, {a, a + 4, b + 4, c + 4}});
  }
  const auto inner = FillStar(8, 4);
  cells.insert(cells.end(), inner.begin(), inner.end());
  return BoundMesh(points, cells);
}

Slot PointSlot(const Mesh& mesh, Id id) {
  const auto& points = mesh.Points();
  const auto found = std::find_if(points.begin(), points.end(),
                                  [&](const auto& row) { return !row.removed && row.node.id == id; });
  REQUIRE(found != points.end());
  return static_cast<Slot>(found - points.begin());
}

Cell AsCell(const Mesh& mesh, Slot slot) {
  const auto& row = mesh.Cells().at(slot);
  Cell result;
  result.id = row.id;
  for (unsigned i = 0; i < 4; ++i) result.v[i] = mesh.Points().at(row.vertices[i]).node;
  return result;
}

unsigned FaceSlot(const Mesh& mesh, Slot cell, FaceKey key) {
  for (unsigned i = 0; i < 4; ++i)
    if (Sorted(OrientedFace(AsCell(mesh, cell), i)) == key) return i;
  throw std::logic_error("Fixture cannot find the requested face.");
}

Id MaxPoint(const Owned& rows) {
  Id result = 0;
  for (const auto& entry : rows)
    for (const auto& node : entry.second.cell.v) result = std::max(result, node.id);
  return result;
}

std::vector<char> RowBytes(OwnedCell row) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  stream(row);
  return bytes;
}

std::vector<char> Snapshot(const Mesh& mesh) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  auto points = mesh.Points().size();
  auto cells = mesh.Cells().size();
  stream(points, cells);
  for (auto row : mesh.Points())
    stream(row.node, row.tensor, row.surface, row.border, row.removed, row.wall_near, row.incident);
  for (Slot slot = 0; slot < cells; ++slot) {
    auto row = mesh.Cells().at(slot);
    stream(row.vertices, row.neighbors, row.id, row.version, row.live, row.changed, row.stamp, row.binding);
    if (!row.live) continue;
    auto bindings = mesh.Bindings(slot);
    stream(bindings);
  }
  return bytes;
}

using Boundary = std::pair<std::map<FaceKey, std::pair<Id, bool>>, std::map<Id, std::vector<char>>>;

Boundary BoundaryState(const Owned& rows) {
  Boundary result;
  for (const auto& entry : rows) {
    const auto& row = entry.second;
    for (unsigned i = 0; i < 4; ++i) {
      if (!row.physical[i].present) continue;
      const auto face = OrientedFace(row.cell, i);
      REQUIRE(result.first.emplace(Sorted(face), std::make_pair(row.physical[i].facet, Odd(face))).second);
      for (unsigned j = 0; j < 4; ++j) {
        if (i == j) continue;
        auto node = row.cell.v[j];
        auto tensor = row.sensor[j];
        std::vector<char> bytes;
        RecordStream stream(bytes);
        stream(node, tensor);
        const auto [where, inserted] = result.second.emplace(node.id, bytes);
        if (!inserted) CHECK(where->second == bytes);
      }
    }
  }
  return result;
}

void CheckNeighbours(const Mesh& mesh) {
  std::map<FaceKey, std::vector<std::pair<Slot, unsigned>>> rebuilt;
  for (Slot cell = 0; cell < mesh.Cells().size(); ++cell) {
    if (!mesh.Cells()[cell].live) continue;
    for (unsigned i = 0; i < 4; ++i)
      rebuilt[Sorted(OrientedFace(AsCell(mesh, cell), i))].emplace_back(cell, i);
  }
  for (const auto& face : rebuilt) {
    const auto& uses = face.second;
    REQUIRE(uses.size() <= 2);
    const auto [a, i] = uses.front();
    if (uses.size() == 1) {
      CHECK(mesh.Cells()[a].neighbors[i] == Physical);
      CHECK(mesh.Bindings(a)[i].present);
      continue;
    }
    const auto [b, j] = uses.back();
    CHECK(mesh.Cells()[a].neighbors[i] == static_cast<int32_t>(b));
    CHECK(mesh.Cells()[b].neighbors[j] == static_cast<int32_t>(a));
    CHECK_FALSE(mesh.Bindings(a)[i].present);
    CHECK_FALSE(mesh.Bindings(b)[j].present);
    CHECK(Odd(OrientedFace(AsCell(mesh, a), i)) != Odd(OrientedFace(AsCell(mesh, b), j)));
  }
}

Owned CheckChange(const Mesh& mesh, const Owned& original, size_t count) {
  REQUIRE_NOTHROW(mesh.Check());
  CheckNeighbours(mesh);
  Owned result = original;
  IdSequence cell_ids(original.rbegin()->first);
  mesh.WriteBack(result, cell_ids);
  REQUIRE(result.size() == count);
  CHECK(BoundaryState(result) == BoundaryState(original));
  for (auto current = result.begin(); current != result.end(); ++current) {
    const auto& row = current->second;
    const auto& v = row.cell.v;
    CHECK(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0);
    CHECK(row.sensor_present);
    const auto old = original.find(current->first);
    if (old == original.end()) {
      CHECK(row.version == 0);
      CHECK(row.cell.id > original.rbegin()->first);
    } else {
      CHECK(RowBytes(row) == RowBytes(old->second));
    }
    for (auto other = result.begin(); other != current; ++other)
      CHECK(ConformingDisjoint(row.cell, other->second.cell));
  }
  return result;
}

constexpr uint32_t Stamp = 7;
constexpr size_t CellLimit = 64;
}  // namespace

TEST_CASE("Native local operators preserve oriented cavity boundaries and bindings", "[.][NativeLocalCore3D]") {
  PassStats stats;

  SECTION("ReplaceCells keeps the specified positive vertex order") {
    const auto original = BoundMesh({{}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, {{0, 1, 2, 3}});
    Mesh mesh(original);
    const auto v = mesh.Cells().front().vertices;
    const Vertices rotated{v[1], v[2], v[0], v[3]};
    REQUIRE(mesh.ReplaceCells({0}, {rotated}, Stamp) == Refusal::None);
    const auto result = CheckChange(mesh, original, 1);
    for (unsigned i = 0; i < 4; ++i)
      CHECK(result.begin()->second.cell.v[i].id == mesh.Points()[rotated[i]].node.id);
  }

  SECTION("Collapse removes an interior star center while relinking exterior neighbors") {
    const auto original = LayeredTetra();
    Mesh mesh(original);
    const Slot remove = PointSlot(mesh, PointId(8));
    const Slot keep = PointSlot(mesh, PointId(4));
    mesh.MarkWallNear();
    REQUIRE_FALSE(mesh.Points()[remove].wall_near);
    REQUIRE(mesh.Points()[keep].wall_near);
    REQUIRE(mesh.Collapse({keep, remove}, Stamp, stats) == Refusal::None);
    CHECK(mesh.Points()[remove].removed);
    CheckChange(mesh, original, 13);
  }

  SECTION("Split preserves the outer faces of a closed ring between surface points") {
    const auto original = Bipyramid(1.125, true, false, true);
    Mesh mesh(original);
    const SU2NativeLocal3D::Edge edge{PointSlot(mesh, PointId(3)), PointSlot(mesh, PointId(4))};
    REQUIRE(mesh.Points()[edge[0]].surface);
    REQUIRE(mesh.Points()[edge[1]].surface);
    const Id base = MaxPoint(original);
    IdSequence ids(base);
    REQUIRE(mesh.Split(edge, 0, ids, Stamp, CellLimit, stats) == Refusal::None);
    const auto& fresh = mesh.Points()[PointSlot(mesh, base + 1)];
    CHECK(fresh.node.p.x == 0);
    CHECK(fresh.node.p.y == 0);
    CHECK(fresh.node.p.z == 0);
    CHECK(Components(fresh.tensor) == Components(AffineTensor({})));
    CHECK_FALSE(fresh.surface);
    CheckChange(mesh, original, 6);
  }

  SECTION("Edge removal replaces a closed triangular ring with two tetrahedra") {
    const auto original = Bipyramid(.8, true);
    Mesh mesh(original);
    const SU2NativeLocal3D::Edge edge{PointSlot(mesh, PointId(3)), PointSlot(mesh, PointId(4))};
    REQUIRE(mesh.RemoveEdge(edge, 0, Stamp, CellLimit, stats) == Refusal::None);
    CheckChange(mesh, original, 2);
  }

  SECTION("Face swap replaces a thin convex bipyramid pair with three tetrahedra") {
    const auto original = Bipyramid(.05, false);
    Mesh mesh(original);
    const auto shared = FaceSlot(mesh, 0, {PointId(0), PointId(1), PointId(2)});
    REQUIRE(mesh.SwapFace(0, shared, Stamp, CellLimit, stats) == Refusal::None);
    CheckChange(mesh, original, 3);
  }

  SECTION("Point motion transports an affine metric and publishes a fresh id") {
    const auto original = BoundMesh({{}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {.25, .25, .015625}},
                                    FillStar(4, 0), true);
    Mesh mesh(original);
    const Slot center = PointSlot(mesh, PointId(4));
    const Id base = MaxPoint(original);
    IdSequence ids(base);
    REQUIRE(mesh.MovePoint(center, ids, Stamp, stats) == Refusal::None);
    const auto& fresh = mesh.Points()[PointSlot(mesh, base + 1)];
    CHECK(fresh.node.p.x == .25);
    CHECK(fresh.node.p.y == .25);
    CHECK(fresh.node.p.z == .25);
    const auto actual = Components(fresh.tensor);
    const auto expected = Components(AffineTensor({.25, .25, .25}));
    for (unsigned i = 0; i < actual.size(); ++i) CHECK(actual[i] == Approx(expected[i]).margin(1e-14));
    const auto result = CheckChange(mesh, original, 4);
    for (const auto& entry : result) {
      const auto& vertices = entry.second.cell.v;
      CHECK(std::none_of(vertices.begin(), vertices.end(), [&](Node node) { return node.id == PointId(4); }));
    }
  }
}

TEST_CASE("Native local replacements refuse malformed cavities without mutating rows", "[.][NativeLocalCore3D]") {
  Mesh mesh(Bipyramid(.2, false));
  const auto before = Snapshot(mesh);
  const auto vertices = mesh.Cells().front().vertices;

  SECTION("The same cavity boundary cannot be matched twice") {
    CHECK(mesh.ReplaceCells({0}, {vertices, vertices}, Stamp) == Refusal::Topology);
  }
  SECTION("The cavity boundary cannot be omitted") {
    CHECK(mesh.ReplaceCells({0}, {}, Stamp) == Refusal::Topology);
  }
  SECTION("An inverted tetrahedron is refused") {
    auto inverted = vertices;
    std::swap(inverted[0], inverted[1]);
    CHECK(mesh.ReplaceCells({0}, {inverted}, Stamp) != Refusal::None);
  }
  SECTION("A positive cell on the wrong side of the shared face is refused") {
    const Vertices wrong_side{PointSlot(mesh, PointId(4)), PointSlot(mesh, PointId(0)),
                              PointSlot(mesh, PointId(1)), PointSlot(mesh, PointId(2))};
    const auto p = [&](unsigned i) { return mesh.Points()[wrong_side[i]].node.p; };
    REQUIRE(Orientation(p(0), p(1), p(2), p(3)) > 0);
    CHECK(mesh.ReplaceCells({0}, {wrong_side}, Stamp) == Refusal::Topology);
  }
  CHECK(Snapshot(mesh) == before);
}

TEST_CASE("Native local face swap refuses a nonconvex pair atomically", "[.][NativeLocalCore3D]") {
  Mesh mesh(Bipyramid(.2, false, true));
  const auto before = Snapshot(mesh);
  const auto x = PointSlot(mesh, PointId(0));
  const auto y = PointSlot(mesh, PointId(1));
  const auto z = PointSlot(mesh, PointId(2));
  const auto p = PointSlot(mesh, PointId(3));
  const auto q = PointSlot(mesh, PointId(4));
  const auto n = [&](Slot slot) { return mesh.Points()[slot].node.p; };
  REQUIRE(Orientation(n(q), n(p), n(x), n(y)) < 0);
  CHECK(mesh.ReplaceCells({0, 1}, {{q, p, x, y}, {q, p, y, z}, {q, p, z, x}}, Stamp) != Refusal::None);
  CHECK(Snapshot(mesh) == before);
  PassStats stats;
  const auto shared = FaceSlot(mesh, 0, {PointId(0), PointId(1), PointId(2)});
  CHECK(mesh.SwapFace(0, shared, Stamp, CellLimit, stats) != Refusal::None);
  CHECK(Snapshot(mesh) == before);
}
