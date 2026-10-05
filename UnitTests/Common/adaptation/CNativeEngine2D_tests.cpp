/*!
 * \file CNativeEngine2D_tests.cpp
 * \brief Actual MPI coupled transactions with frozen nodal fields and old-state rejection controls.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeEngine2D.hpp"
#include "../../../Common/include/adaptation/CNativeReference2D.hpp"

using namespace SU2NativeBoundary2D;

namespace {
std::vector<char> Snapshot(const std::map<Id, Cell>& cells) {
  std::vector<char> result;
  RecordStream stream(result);
  for (auto cell : cells) stream(cell.second);
  return result;
}
struct Case {
  Node a{0, {0, 0}, 1 | FEATURE}, b{1, {.04, 0}, 1 | FEATURE}, c{2, {.04, .02}, 1 | FEATURE},
      d{3, {0, .02}, 1 | FEATURE};
  PolylineReference geometry{{{a, b, 10}, {b, c, 11}, {c, d, 12}, {d, a, 11}}, 45};
  double height = .004;
  std::map<Id, Cell> Owned(World& world) const {
    Cell lower, upper;
    lower.t = triangle(a, b, c);
    lower.t.id = 10;
    lower.marker = {geometry.ComponentOfOriginalFace(a.id, b.id), geometry.ComponentOfOriginalFace(b.id, c.id), 0};
    upper.t = triangle(a, c, d);
    upper.t.id = 11;
    upper.marker = {0, geometry.ComponentOfOriginalFace(c.id, d.id), geometry.ComponentOfOriginalFace(d.id, a.id)};
    std::map<Id, Cell> result;
    int index = 0;
    for (auto cell : {lower, upper}) {
      for (auto& target : cell.nodal_target) target = {625, 0, 62500};
      if (index++ % std::min(world.size, 2) == world.rank) result.emplace(cell.t.id, cell);
    }
    return result;
  }
  EngineOptions Options() const {
    EngineOptions result;
    result.geometry_tolerance = 1e-10;
    return result;
  }
};
std::vector<Cell> Gather(World& world, const Engine& engine) {
  std::vector<std::vector<Cell>> to(world.size);
  for (const auto& cell : engine.owned) to[0].push_back(cell.second);
  return world.exchange(to);
}
}  // namespace

TEST_CASE("Native MPI engine: coupled boundary refinement and admitted publication", "[NativeEngine2D]") {
  World world;
  Case input;
  Engine engine(world, input.Owned(world), input.geometry.Policy({{10, input.height}}), input.Options());
  Choice choice{{Action::SPLIT, 0, 1}, 1, world.rank};
  const bool accepted = engine.round(choice);
  CHECK(accepted == (world.rank == 0));
  CHECK(world.sum(accepted) == 1);
  CHECK(engine.stats.conflicts == (world.rank == 0 ? 0 : 1));
  if (world.rank == 0) CHECK(engine.stats.cross_rank == (world.size > 1 ? 1 : 0));
  const auto after = Gather(world, engine);
  if (world.rank == 0) {
    std::string reason;
    REQUIRE(strict_cells(triangles(after), reason));
    const auto faces = physical(after);
    CHECK(faces.size() == 5);
    const auto wall = input.geometry.ComponentOfOriginalFace(0, 1);
    int wallFaces = 0;
    for (const auto& cell : after)
      for (int k = 0; k < 3; ++k)
        if (cell.marker[k] == wall) {
          ++wallFaces;
          const auto altitude = static_cast<double>(2 * area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p);
          CHECK(altitude == Approx(input.height));
          CHECK(cell.t.protected_cell == 1);
          CHECK(cell.t.v[(k + 2) % 3].fixed == 0);  // Can be released during a subsequent wall reconstruction.
        }
    CHECK(wallFaces == 2);
    for (const auto& cell : after) {
      CHECK(cell.target_epoch == 1);
      CHECK(cell.version == 1);
      for (const auto& m : cell.nodal_target) {
        CHECK(m.xx == Approx(625));
        CHECK(m.yy == Approx(62500));
      }
    }
  }
  bool overflow = false, stale = false;
  const auto refs = engine.directory.lookup({0}, overflow);
  CHECK_FALSE(overflow);
  CHECK_FALSE(engine.directory.fetch(refs, engine.owned, true, stale).empty());
  CHECK_FALSE(stale);
}

TEST_CASE("Native MPI engine: rejected edits keep accepted records exact", "[NativeEngine2D]") {
  World world;
  Case input;
  for (const int fault : {1, 2, 3, 4}) {
    Engine engine(world, input.Owned(world), input.geometry.Policy({{10, input.height}}), input.Options());
    const auto before = Snapshot(engine.owned);
    const auto incidence = engine.directory.incident;
    Choice choice{{Action::SPLIT, 0, 1}, world.rank == 0 ? 1. : -1., world.rank};
    choice.op.fault = fault;
    CHECK_FALSE(engine.round(choice));
    CHECK(Snapshot(engine.owned) == before);
    CHECK(engine.stats.commits == 0);
    // Query still obtains exactly the original versioned payload after a rejected preparation.
    bool overflow = false, stale = false;
    const auto refs = engine.directory.lookup({0, 1}, overflow, true);
    const auto old = engine.directory.fetch(refs, engine.owned, true, stale);
    REQUIRE(old.size() == 1);
    CHECK(old.front().t.id == 10);
    CHECK(old.front().version == 0);
    CHECK_FALSE(stale);
    CHECK_FALSE(overflow);
  }
  auto controls = input.Options();
  controls.dependency_bytes = 64;
  Engine engine(world, input.Owned(world), input.geometry.Policy({{10, input.height}}), controls);
  const auto before = Snapshot(engine.owned);
  Choice choice{{Action::SPLIT, 0, 1}, world.rank == 0 ? 1. : -1., world.rank};
  CHECK_FALSE(engine.round(choice));
  CHECK(Snapshot(engine.owned) == before);
  CHECK(engine.stats.commits == 0);
  if (world.rank == 0) CHECK(engine.stats.memory_rejected > 0);
}

TEST_CASE("Native MPI engine: cached priorities use original nodal targets", "[NativeEngine2D]") {
  World world;
  Case input;
  auto cells = input.Owned(world);
  for (auto& cell : cells)
    for (auto& target : cell.second.nodal_target) target.yy = 625;
  // The physical edge has unit length in this nodal target, so it is not a split candidate.
  Engine coarse(world, cells, input.geometry.Policy({{10, input.height}}), input.Options());
  CHECK(coarse.choose(Action::SPLIT, {}).score < 0);
  // Refine tangential sizes only through the actual nodal arrays, without an analytic priority callback.
  for (auto& cell : cells)
    for (auto& target : cell.second.nodal_target) target.xx = 10000;
  Engine fine(world, cells, input.geometry.Policy({{10, input.height}}), input.Options());
  const auto split = fine.choose(Action::SPLIT, {});
  if (world.rank == 0) {
    CHECK(split.score > 0);
    CHECK(split.op.a == 0);
    CHECK(split.op.b == 1);
  }
  if (world.rank >= 2) CHECK(split.score < 0);
}

TEST_CASE("Native MPI engine: independent wall cavities publish in the same round", "[NativeEngine2D]") {
  World world;
  std::vector<PolylineReference::Face> faces;
  std::map<Id, Cell> local;
  for (int rank = 0; rank < world.size; ++rank) {
    const Id base = 32 * rank;
    const double offset = .1 * rank;
    const Node a{base, {offset, 0}, 1 | FEATURE}, b{base + 1, {offset + .04, 0}, 1 | FEATURE},
        c{base + 2, {offset + .04, .02}, 1 | FEATURE}, d{base + 3, {offset, .02}, 1 | FEATURE};
    faces.insert(faces.end(), {{a, b, 10}, {b, c, 11}, {c, d, 12}, {d, a, 11}});
    if (rank == world.rank) {
      Cell lower, upper;
      lower.t = triangle(a, b, c);
      lower.t.id = base + 10;
      upper.t = triangle(a, c, d);
      upper.t.id = base + 11;
      local.emplace(lower.t.id, lower);
      local.emplace(upper.t.id, upper);
    }
  }
  const PolylineReference reference(faces, 45);
  for (auto& entry : local) {
    auto& cell = entry.second;
    for (auto& m : cell.nodal_target) m = {625, 0, 62500};
    const Id base = 32 * world.rank;
    if (cell.t.id == base + 10)
      cell.marker = {reference.ComponentOfOriginalFace(base, base + 1),
                     reference.ComponentOfOriginalFace(base + 1, base + 2), 0};
    else
      cell.marker = {0, reference.ComponentOfOriginalFace(base + 2, base + 3),
                     reference.ComponentOfOriginalFace(base + 3, base)};
  }
  EngineOptions controls;
  controls.geometry_tolerance = 1e-10;
  Engine engine(world, local, reference.Policy({{10, .004}}), controls);
  const Id base = 32 * world.rank;
  CHECK(engine.round({{Action::SPLIT, base, base + 1}, 1., world.rank}));
  CHECK(world.sum(engine.stats.commits) == world.size);
  CHECK(engine.stats.conflicts == 0);
  CHECK(engine.stats.cross_rank == 0);
  const auto cells = Gather(world, engine);
  if (world.rank == 0) {
    std::string reason;
    CHECK(strict_cells(triangles(cells), reason));
    CHECK(physical(cells).size() == 5 * world.size);
    int walls = 0;
    for (const auto& cell : cells)
      for (int k = 0; k < 3; ++k)
        if (cell.marker[k] && reference.Marker(cell.marker[k]) == 10) {
          ++walls;
          const auto altitude = static_cast<double>(2 * area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p);
          CHECK(altitude == Approx(.004));
        }
    CHECK(walls == 2 * world.size);
  }
}
