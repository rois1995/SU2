/*!
 * \file CNativeCavity3D_tests.cpp
 * \brief Oriented private insertion, removal, movement and reconnection controls.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeCavity3D.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
using namespace SU2Native3D;
namespace {
Cell Make(Id id, std::array<Node, 4> v) {
  if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) < 0) std::swap(v[0], v[1]);
  return {id, v};
}
std::vector<Cell> Single(double scale = 1) {
  return {Make(0, {{{0, {0, 0, 0}}, {1, {scale, 0, 0}}, {2, {0, scale, 0}}, {3, {0, 0, scale}}}})};
}
}  // namespace
TEST_CASE("Native 3D cone inserts, moves and removes an interior vertex privately", "[NativeMesh3D]") {
  std::string reason;
  for (double scale : {1e-100, 1., 1e100}) {
    const auto old = Single(scale);
    std::vector<Cell> inserted, moved, removed;
    const Node center{4, {.25 * scale, .25 * scale, .25 * scale}};
    REQUIRE(Cone(old, center, 10, inserted, reason));
    CHECK(reason.empty());
    CHECK(inserted.size() == 4);
    CHECK(ValidateFixedInterface(old, inserted, reason));
    REQUIRE(Cone(inserted, {4, {.125 * scale, .25 * scale, .25 * scale}}, 20, moved, reason));
    CHECK(moved.size() == 4);
    CHECK(ValidateFixedInterface(old, moved, reason));
    REQUIRE(Cone(moved, old.front().v[0], 30, removed, reason));
    CHECK(removed.size() == 1);
    CHECK(ValidateFixedInterface(old, removed, reason));
    CHECK(old.front().id == 0);
  }
}
TEST_CASE("Native 3D cone reconnects two tetrahedra to three", "[NativeMesh3D]") {
  const Node a{0, {0, 0, 0}}, b{1, {1, 0, 0}}, c{2, {0, 1, 0}}, top{3, {.25, .25, 1}}, bottom{4, {.25, .25, -1}};
  const std::vector<Cell> old{Make(0, {a, b, c, top}), Make(1, {a, b, c, bottom})};
  std::vector<Cell> fresh;
  std::string reason;
  REQUIRE(Cone(old, top, 10, fresh, reason));
  CHECK(fresh.size() == 3);
  CHECK(Incidence(fresh).EdgeCells({top.id, bottom.id}).size() == 3);
  CHECK(ValidateFixedInterface(old, fresh, reason));
  std::vector<Cell> back;
  REQUIRE(Cone(fresh, a, 20, back, reason));
  CHECK(back.size() == 2);
  CHECK(ValidateFixedInterface(old, back, reason));
}
TEST_CASE("Native 3D cone rejects obstruction, interface movement and budget without changing output",
          "[NativeMesh3D]") {
  const auto old = Single();
  std::vector<Cell> fresh = old;
  std::string reason;
  CHECK_FALSE(Cone(old, {4, {2, 2, 2}}, 10, fresh, reason));
  CHECK_FALSE(reason.empty());
  CHECK(fresh.size() == 1);
  CHECK(fresh.front().id == 0);
  CHECK_FALSE(Cone(old, {0, {.125, .125, .125}}, 10, fresh, reason));
  CHECK(fresh.front().v[0].p.x == 0);
  CHECK_FALSE(Cone(old, {4, {.25, .25, .25}}, std::numeric_limits<Id>::max() - 2, fresh, reason));
  CHECK(fresh.front().id == 0);
  CHECK_FALSE(Cone({}, {4, {}}, 10, fresh, reason));
  CHECK_FALSE(Cone(std::vector<Cell>(MaximumCavityCells + 1, old.front()), {4, {}}, 10, fresh, reason));
  CHECK_FALSE(ValidateFixedInterface(old, std::vector<Cell>(MaximumReplacementCells + 1, old.front()), reason));
  const Node a{0, {0, 0, 0}}, b{1, {1, 0, 0}}, c{2, {0, 1, 0}}, top{3, {2, 2, 1}}, bottom{4, {2, 2, -1}};
  const std::vector<Cell> nonconvex{Make(0, {a, b, c, top}), Make(1, {a, b, c, bottom})};
  CHECK_FALSE(Cone(nonconvex, top, 10, fresh, reason));
  CHECK(fresh.front().id == 0);
}
TEST_CASE("Native 3D fixed interface rejects changed skins, duplicates and disconnected cavities", "[NativeMesh3D]") {
  const auto old = Single();
  auto changed = old;
  std::string reason;
  changed.front().v[0].p.x = .125;
  CHECK_FALSE(ValidateFixedInterface(old, changed, reason));
  changed = old;
  changed.front().v[0].id = 10;
  CHECK_FALSE(ValidateFixedInterface(old, changed, reason));
  changed = old;
  std::swap(changed.front().v[0], changed.front().v[1]);
  CHECK_FALSE(ValidateFixedInterface(old, changed, reason));
  changed = old;
  changed.push_back(changed.front());
  changed.back().id = 1;
  CHECK_FALSE(ValidateFixedInterface(old, changed, reason));
  changed = old;
  auto other = old.front();
  other.id = 1;
  for (auto& node : other.v) {
    node.id += 10;
    node.p.x += 2;
  }
  changed.push_back(other);
  CHECK(Incidence(changed).FaceComponents() == 2);
  CHECK_FALSE(ValidateFixedInterface(changed, changed, reason));
}
TEST_CASE("Native 3D cavity rejects face-connected genus-one skin", "[NativeMesh3D]") {
  std::vector<Cell> torus;
  for (int x = 0; x < 3; ++x)
    for (int y = 0; y < 3; ++y) {
      if (x == 1 && y == 1) continue;
      const std::array<Point, 8> positions{{{double(x), double(y), 0},
                                            {double(x + 1), double(y), 0},
                                            {double(x + 1), double(y + 1), 0},
                                            {double(x), double(y + 1), 0},
                                            {double(x), double(y), 1},
                                            {double(x + 1), double(y), 1},
                                            {double(x + 1), double(y + 1), 1},
                                            {double(x), double(y + 1), 1}}};
      std::array<Node, 8> v;
      for (size_t i = 0; i < 8; ++i)
        v[i] = {Id(positions[i].x + 4 * positions[i].y + 16 * positions[i].z), positions[i]};
      const std::array<Id, 6> ring{1, 2, 3, 7, 4, 5};
      for (size_t i = 0; i < 6; ++i)
        torus.push_back(Make(torus.size(), {v[0], v[6], v[ring[i]], v[ring[(i + 1) % 6]]}));
    }
  CHECK(Incidence(torus).FaceComponents() == 1);
  std::string reason;
  CHECK_FALSE(ValidateFixedInterface(torus, torus, reason));
  CHECK(reason.find("sphere/disk") != std::string::npos);
}

TEST_CASE("Native 3D cone closure survives rotated thin cavities", "[NativeMesh3D]") {
  for (double aspect : {1e4, 1e6, 1e8, 1e10}) {
    auto old = Single();
    const double c = std::cos(.37), s = std::sin(.37);
    auto transform = [&](Point p) { return Point{c * p.x + s * p.z / aspect, p.y, -s * p.x + c * p.z / aspect}; };
    for (auto& cell : old)
      for (auto& node : cell.v) node.p = transform(node.p);
    std::vector<Cell> fresh;
    std::string reason;
    const bool result = Cone(old, {4, transform({.25, .25, .25})}, 10, fresh, reason);
    INFO("aspect=" << aspect << "; rejection=" << reason);
    CHECK(result);
    if (result) CHECK(ValidateFixedInterface(old, fresh, reason));
  }
}
