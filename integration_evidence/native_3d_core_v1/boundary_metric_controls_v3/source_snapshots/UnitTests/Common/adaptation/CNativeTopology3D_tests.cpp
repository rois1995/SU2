/*!
 * \file CNativeTopology3D_tests.cpp
 * \brief Complete supplied-cell stars and tetrahedral face cancellation controls.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeTopology3D.hpp"
#include <algorithm>
#include <stdexcept>
using namespace SU2Native3D;
namespace {
Cell Make(Id id, std::array<Node, 4> v) {
  if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) < 0) std::swap(v[0], v[1]);
  return {id, v};
}
std::vector<Cell> Cube() {
  const std::array<Node, 8> v{{{0, {0, 0, 0}},
                               {1, {1, 0, 0}},
                               {2, {1, 1, 0}},
                               {3, {0, 1, 0}},
                               {4, {0, 0, 1}},
                               {5, {1, 0, 1}},
                               {6, {1, 1, 1}},
                               {7, {0, 1, 1}}}};
  const std::array<Id, 6> ring{1, 2, 3, 7, 4, 5};
  std::vector<Cell> cells;
  for (size_t i = 0; i < 6; ++i) cells.push_back(Make(20 + i, {v[0], v[6], v[ring[i]], v[ring[(i + 1) % 6]]}));
  return cells;
}
}  // namespace
TEST_CASE("Native 3D incidence: full edge and vertex stars exceed a face pair", "[NativeMesh3D]") {
  const auto cells = Cube();
  const Incidence index(cells);
  CHECK(index.FaceEntries() == 24);
  CHECK(index.EdgeEntries() == 36);
  CHECK(index.VertexEntries() == 24);
  CHECK(index.FaceComponents() == 1);
  CHECK(index.Boundary().size() == 12);
  CHECK(index.EdgeCells({6, 0}) == std::vector<Id>{20, 21, 22, 23, 24, 25});
  CHECK(index.VertexCells(0) == index.EdgeCells({0, 6}));
  CHECK(index.FaceCells({6, 2, 0}) == std::vector<Id>{20, 21});
  CHECK(index.FaceCells({1, 2, 6}) == std::vector<Id>{20});
  CHECK(index.VertexCells(8).empty());
  CHECK(index.EdgeCells({2, 4}).empty());
  CHECK(index.FaceCells({0, 2, 4}).empty());
  CHECK_THROWS_AS(index.FaceCells({1, 1, 2}), std::invalid_argument);
  CHECK_THROWS_AS(index.EdgeCells({1, 1}), std::invalid_argument);
  for (const auto& face : index.Boundary()) {
    const Cell* owner = nullptr;
    for (const auto& cell : cells)
      if (cell.id == index.FaceCells(face).front()) owner = &cell;
    REQUIRE(owner);
    std::array<Point, 3> p;
    Point interior;
    for (const auto& v : owner->v) {
      const auto match = std::find(face.begin(), face.end(), v.id);
      if (match != face.end())
        p[match - face.begin()] = v.p;
      else
        interior = v.p;
    }
    CHECK(Orientation(p[0], p[1], p[2], interior) < 0);
  }
  auto shuffled = cells;
  std::reverse(shuffled.begin(), shuffled.end());
  for (auto& cell : shuffled) {
    std::swap(cell.v[0], cell.v[1]);
    std::swap(cell.v[2], cell.v[3]);
  }
  const Incidence reordered(shuffled);
  CHECK(reordered.Boundary() == index.Boundary());
  CHECK(reordered.EdgeCells({6, 0}) == index.EdgeCells({0, 6}));
}
TEST_CASE("Native 3D incidence rejects duplicate and inconsistent records", "[NativeMesh3D]") {
  auto cells = Cube();
  cells[1].id = cells[0].id;
  CHECK_THROWS_AS(Incidence(cells), std::invalid_argument);
  cells = Cube();
  cells[1].v = cells[0].v;
  CHECK_THROWS_AS(Incidence(cells), std::invalid_argument);
  cells = Cube();
  cells[0].v[0].p.x += .125;
  CHECK_THROWS_AS(Incidence(cells), std::invalid_argument);
  cells = Cube();
  cells[0].v[1].id = cells[0].v[0].id;
  CHECK_THROWS_AS(Incidence(cells), std::invalid_argument);
  cells = Cube();
  std::swap(cells[0].v[0], cells[0].v[1]);
  CHECK_THROWS_AS(Incidence(cells), std::invalid_argument);
}
TEST_CASE("Native 3D incidence rejects same-side and nonmanifold shared faces", "[NativeMesh3D]") {
  const Node a{0, {0, 0, 0}}, b{1, {1, 0, 0}}, c{2, {0, 1, 0}};
  const Cell top = Make(0, {a, b, c, {3, {0, 0, 1}}});
  const Cell bottom = Make(1, {a, b, c, {4, {0, 0, -1}}});
  const Cell top2 = Make(2, {a, b, c, {5, {.125, .125, 2}}});
  CHECK(Incidence({top, bottom}).Boundary().size() == 6);
  CHECK_THROWS_AS(Incidence({top, top2}), std::invalid_argument);
  CHECK_THROWS_AS(Incidence({top, bottom, top2}), std::invalid_argument);
}
TEST_CASE("Native 3D incidence reports disconnected and vertex-pinched patches", "[NativeMesh3D]") {
  const auto cells = Cube();
  const Incidence partial({cells[0], cells[3]});
  CHECK(partial.FaceComponents() == 2);
  CHECK(partial.EdgeCells({0, 6}).size() == 2);
  CHECK(Incidence({}).FaceComponents() == 0);
  CHECK(Incidence({}).Boundary().empty());
  const Cell a = Make(0, {{{0, {0, 0, 0}}, {1, {1, 0, 0}}, {2, {0, 1, 0}}, {3, {0, 0, 1}}}});
  const Cell b = Make(1, {{{0, {0, 0, 0}}, {4, {-1, 0, 0}}, {5, {0, -1, 0}}, {6, {0, 0, -1}}}});
  CHECK(Incidence({a, b}).FaceComponents() == 2);
}
