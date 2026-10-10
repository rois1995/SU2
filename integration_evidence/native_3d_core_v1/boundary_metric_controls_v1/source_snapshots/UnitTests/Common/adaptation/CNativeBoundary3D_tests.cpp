/*!
 * \file CNativeBoundary3D_tests.cpp
 * \brief Coupled private planar boundary refinement/coarsening and incomplete-star rollback.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
using namespace SU2Native3D;
namespace {
Patch Cube(double height = 1) {
  std::array<Node, 8> v{{{0, {0, 0, 0}},
                         {1, {1, 0, 0}},
                         {2, {1, 1, 0}},
                         {3, {0, 1, 0}},
                         {4, {0, 0, height}},
                         {5, {1, 0, height}},
                         {6, {1, 1, height}},
                         {7, {0, 1, height}}}};
  Patch patch;
  const std::array<Id, 6> ring{1, 2, 3, 7, 4, 5};
  for (size_t i = 0; i < 6; ++i) {
    Cell c{Id(i), {v[0], v[6], v[ring[i]], v[ring[(i + 1) % 6]]}};
    if (Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) < 0) std::swap(c.v[0], c.v[1]);
    patch.cells.push_back(c);
  }
  const auto faces = Incidence(patch.cells).Boundary();
  for (size_t i = 0; i < faces.size(); ++i) patch.surface.push_back({faces[i], Id(i)});
  return patch;
}
std::vector<Facet> Facets(const Patch& patch) {
  std::map<Id, Node> nodes;
  for (const auto& c : patch.cells)
    for (auto n : c.v) nodes.emplace(n.id, n);
  std::vector<Facet> facets;
  for (const auto& f : patch.surface)
    facets.push_back({f.facet, int(f.facet + 10), {nodes.at(f.v[0]), nodes.at(f.v[1]), nodes.at(f.v[2])}});
  return facets;
}
Patch EdgeStar(const Patch& patch, Edge edge) {
  Patch star;
  for (const auto& c : patch.cells) {
    size_t found = 0;
    for (auto n : c.v) found += n.id == edge[0] || n.id == edge[1];
    if (found == 2) star.cells.push_back(c);
  }
  const auto skin = Incidence(star.cells).Boundary();
  for (const auto& f : patch.surface)
    if (std::find(skin.begin(), skin.end(), f.v) != skin.end()) star.surface.push_back(f);
  return star;
}
std::set<std::array<Id, 4>> Connectivity(const Patch& patch) {
  std::set<std::array<Id, 4>> result;
  for (const auto& c : patch.cells) {
    std::array<Id, 4> key;
    for (size_t i = 0; i < 4; ++i) key[i] = c.v[i].id;
    std::sort(key.begin(), key.end());
    result.insert(key);
  }
  return result;
}
}  // namespace
TEST_CASE("Native 3D complete internal edge star splits and reverses with fixed skin", "[NativeMesh3D]") {
  const auto old = Cube();
  const PlanarReference reference(Facets(old));
  Patch split, coarsened;
  std::string reason;
  REQUIRE(SplitEdge(old, {0, 6}, {8, {.5, .5, .5}}, 100, reference, split, reason));
  CHECK(split.cells.size() == 12);
  CHECK(split.surface.size() == 12);
  CHECK(ValidateFixedInterface(old.cells, split.cells, reason));
  REQUIRE(UndoEdgeSplit(split, 8, {6, 0}, 200, reference, coarsened, reason));
  CHECK(Connectivity(coarsened) == Connectivity(old));
  CHECK(ValidateFixedInterface(old.cells, coarsened.cells, reason));
}
TEST_CASE("Native 3D planar physical edge edits retain facets and marker junctions", "[NativeMesh3D]") {
  for (double height : {1., 1e-4, 1e-8}) {
    const auto cube = Cube(height);
    const PlanarReference reference(Facets(cube));
    const auto old = EdgeStar(cube, {0, 1});
    Patch current = old;
    std::string reason;
    for (Id step = 0; step < 3; ++step) {
      Patch split, coarsened;
      REQUIRE(SplitEdge(current, {1, 0}, {8, {.5, 0, 0}}, 100 + 40 * step, reference, split, reason));
      CHECK(split.cells.size() == 4);
      CHECK(split.surface.size() == old.surface.size() + 2);
      CHECK_FALSE(ValidateFixedInterface(old.cells, split.cells, reason));
      std::map<Id, size_t> counts;
      for (const auto& f : split.surface) {
        ++counts[f.facet];
        CHECK(reference.Get(f.facet).marker == int(f.facet + 10));
      }
      size_t changed = 0;
      for (const auto& count : counts) changed += count.second == 2;
      CHECK(changed == 2);  // Two original physical facets/markers meet at the edited feature edge.
      REQUIRE(UndoEdgeSplit(split, 8, {0, 1}, 120 + 40 * step, reference, coarsened, reason));
      CHECK(Connectivity(coarsened) == Connectivity(old));
      CHECK(coarsened.surface.size() == old.surface.size());
      for (size_t i = 0; i < old.surface.size(); ++i) {
        CHECK(coarsened.surface[i].v == old.surface[i].v);
        CHECK(coarsened.surface[i].facet == old.surface[i].facet);
      }
      current = std::move(coarsened);
    }
  }
}
TEST_CASE("Native 3D surface edits reject missing dependencies and preserve both outputs", "[NativeMesh3D]") {
  const auto cube = Cube();
  const PlanarReference reference(Facets(cube));
  const auto old = EdgeStar(cube, {0, 1});
  Patch output = old;
  std::string reason;
  auto incomplete = old;
  incomplete.cells.pop_back();
  const auto skin = Incidence(incomplete.cells).Boundary();
  incomplete.surface.erase(
      std::remove_if(incomplete.surface.begin(), incomplete.surface.end(),
                     [&](SurfaceFace f) { return std::find(skin.begin(), skin.end(), f.v) == skin.end(); }),
      incomplete.surface.end());
  CHECK_FALSE(SplitEdge(incomplete, {0, 1}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  CHECK(reason.find("incomplete") != std::string::npos);
  CHECK(Connectivity(output) == Connectivity(old));
  CHECK(output.surface.size() == old.surface.size());
  CHECK_FALSE(SplitEdge(old, {0, 1}, {8, {.5, 1e-16, 0}}, 100, reference, output, reason));
  CHECK_FALSE(SplitEdge(old, {0, 1}, {0, {.5, 0, 0}}, 100, reference, output, reason));
  CHECK_FALSE(SplitEdge(old, {0, 1}, {8, {0, 0, 0}}, 100, reference, output, reason));
  CHECK_FALSE(SplitEdge(old, {0, 0}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  CHECK_FALSE(SplitEdge(old, {0, 1}, {8, {.5, 0, 0}}, std::numeric_limits<Id>::max(), reference, output, reason));
  CHECK_FALSE(SplitEdge(cube, {0, 1}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  auto unmarked = old;
  unmarked.surface.clear();
  CHECK_FALSE(SplitEdge(unmarked, {0, 1}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  auto wrong = old;
  wrong.surface[0].facet = 999;
  CHECK_FALSE(SplitEdge(wrong, {0, 1}, {8, {.5, 0, 0}}, 100, reference, output, reason));
  Patch split;
  REQUIRE(SplitEdge(old, {0, 1}, {8, {.5, 0, 0}}, 100, reference, split, reason));
  auto missing = split;
  missing.cells.pop_back();
  CHECK_FALSE(UndoEdgeSplit(missing, 8, {0, 1}, 200, reference, output, reason));
  CHECK_FALSE(UndoEdgeSplit(split, 8, {0, 6}, 200, reference, output, reason));
  CHECK_FALSE(UndoEdgeSplit(split, 8, {0, 8}, 200, reference, output, reason));
  CHECK(Connectivity(output) == Connectivity(old));
  CHECK(output.surface.size() == old.surface.size());
}
TEST_CASE("Native 3D immutable planar facet authority checks exact membership and invalid input", "[NativeMesh3D]") {
  auto facets = Facets(Cube());
  PlanarReference reference(facets);
  const auto f = facets.front();
  for (auto n : f.v) CHECK(reference.Contains(f.id, n.p));
  const Point center{(f.v[0].p.x + f.v[1].p.x + f.v[2].p.x) / 3, (f.v[0].p.y + f.v[1].p.y + f.v[2].p.y) / 3,
                     (f.v[0].p.z + f.v[1].p.z + f.v[2].p.z) / 3};
  CHECK(reference.Contains(f.id, center));
  CHECK_FALSE(reference.Contains(f.id, {2, 2, 2}));
  facets.front().marker = 999;
  CHECK(reference.Get(f.id).marker == f.marker);
  facets = Facets(Cube());
  facets.push_back(facets.front());
  CHECK_THROWS_AS(PlanarReference(facets), std::invalid_argument);
  facets = {f};
  facets.front().v[2] = facets.front().v[1];
  CHECK_THROWS_AS(PlanarReference(facets), std::invalid_argument);
  CHECK_THROWS_AS(PlanarReference(std::vector<Facet>(4 * MaximumCavityCells + 1, f)), std::invalid_argument);
}
