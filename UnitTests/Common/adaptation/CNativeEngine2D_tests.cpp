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

TEST_CASE("Native MPI insertion: endpoint-star import reconnects before collective publication", "[NativeEngine2D]") {
  World world;
  const Node a{0, {-1, 0}, 1}, b{1, {1, 0}, 1}, c{2, {0, 10}, 1}, d{3, {0, -10}, 1},
      e{4, {-5, 5}, 1}, f{5, {5, 5}, 1}, g{6, {-5, -5}, 1}, h{7, {5, -5}, 1};
  const std::vector<Triangle> input{triangle(a, b, c), triangle(b, a, d), triangle(a, c, e),
                                    triangle(c, b, f), triangle(d, a, g), triangle(b, d, h)};
  std::vector<PolylineReference::Face> faces;
  const auto perimeter = boundary(input);
  for (const auto& face : perimeter) faces.push_back({face.second.first, face.second.second, 10});
  const PolylineReference reference(faces, 45);
  std::map<Id, Cell> owned;
  for (size_t k = 0; k < input.size(); ++k) {
    Cell cell;
    cell.t = input[k];
    cell.t.id = 10 + k;
    for (auto& tensor : cell.nodal_target) tensor = {1, 0, 1};
    for (int slot = 0; slot < 3; ++slot)
      if (perimeter.count(edge(cell.t.v[slot].id, cell.t.v[(slot + 1) % 3].id)))
        cell.marker[slot] = reference.ComponentOfOriginalFace(cell.t.v[slot].id, cell.t.v[(slot + 1) % 3].id);
    if (int(k % world.size) == world.rank) owned.emplace(cell.t.id, cell);
  }
  EngineOptions options;
  options.geometry_tolerance = 1e-10;
  SECTION("full closure spans owners and publishes the repaired insertion") {
    Engine engine(world, owned, reference.Policy({}), options);
    const bool accepted = engine.round({{Action::BULK_SPLIT, a.id, b.id}, world.rank == 0 ? 1. : -1., world.rank});
    CHECK(accepted == (world.rank == 0));
    CHECK(world.sum(accepted) == 1);
    if (world.rank == 0) {
      CHECK(engine.stats.max_patch == input.size());
      CHECK(engine.stats.cross_rank == (world.size > 1 ? 1 : 0));
    }
    const auto output = Gather(world, engine);
    if (world.rank == 0) {
      const auto fresh = triangles(output);
      const auto target = checked([](Point) { return Tensor{1, 0, 1}; });
      std::string reason;
      REQUIRE(validate_replacement(input, fresh, reason));
      REQUIRE(strict_cells(fresh, reason));
      CHECK(min_quality(fresh, target) >= .18);
      CHECK(nodes(fresh).size() == nodes(input).size() + 1);
      CHECK(physical(output).size() == perimeter.size());
    }
  }
  SECTION("an inadmissible dependency budget preserves every owned record") {
    options.dependency_bytes = 64;
    Engine engine(world, owned, reference.Policy({}), options);
    const auto before = Snapshot(engine.owned);
    CHECK_FALSE(engine.round({{Action::BULK_SPLIT, a.id, b.id}, world.rank == 0 ? 1. : -1., world.rank}));
    CHECK(Snapshot(engine.owned) == before);
    CHECK(world.sum(engine.stats.commits) == 0);
    CHECK(world.sum(engine.stats.memory_rejected) > 0);
  }
}

TEST_CASE("Native MPI quality: short interior diagonal repairs an all-boundary ear", "[NativeEngine2D]") {
  World world;
  const Node a{0, {0, 0}, 1}, b{1, {1, -.1}, 1}, c{2, {2, 0}, 1}, d{3, {1, 1}, 1};
  const std::vector<Triangle> input{triangle(a, b, c), triangle(a, c, d)};
  const auto target = checked([](Point) { return Tensor{.36, 0, .36}; });
  REQUIRE(min_quality(input, target) < .18);
  REQUIRE(max_length(input, target) < 1.6);
  std::vector<PolylineReference::Face> faces;
  const auto perimeter = boundary(input);
  for (const auto& face : perimeter) faces.push_back({face.second.first, face.second.second, 10});
  const PolylineReference reference(faces, 45);
  std::map<Id, Cell> owned;
  for (size_t k = 0; k < input.size(); ++k) {
    Cell cell;
    cell.t = input[k];
    cell.t.id = 10 + k;
    for (auto& tensor : cell.nodal_target) tensor = {.36, 0, .36};
    for (int slot = 0; slot < 3; ++slot)
      if (perimeter.count(edge(cell.t.v[slot].id, cell.t.v[(slot + 1) % 3].id)))
        cell.marker[slot] = reference.ComponentOfOriginalFace(cell.t.v[slot].id, cell.t.v[(slot + 1) % 3].id);
    if (int(k % world.size) == world.rank) owned.emplace(cell.t.id, cell);
  }
  EngineOptions options;
  options.geometry_tolerance = 1e-10;
  options.fixed_boundary = true;  // Repair requires no physical boundary motion.
  Engine engine(world, owned, reference.Policy({}), options);
  const auto choice = engine.choose(Action::BULK_SPLIT, {});
  CHECK(world.sum(choice.score >= 0) == 1);
  engine.phase(Action::BULK_SPLIT);
  const auto output = Gather(world, engine);
  if (world.rank == 0) {
    const auto fresh = triangles(output);
    std::string reason;
    REQUIRE(validate_replacement(input, fresh, reason));
    REQUIRE(strict_cells(fresh, reason));
    CHECK(min_quality(fresh, target) >= .18);
    CHECK(max_length(fresh, target) <= 1.8);
    CHECK(nodes(fresh).size() == 5);
    CHECK(physical(output).size() == perimeter.size());
  }
}

TEST_CASE("Native MPI wall repair: a fitting base still requests repair of an oversized side", "[NativeEngine2D]") {
  World world;
  const Node a{0, {0, 0}, 1 | FEATURE}, b{1, {1, 0}, 1 | FEATURE}, c{2, {3, .2}, 1 | FEATURE},
      d{3, {0, 1}, 1 | FEATURE}, apex{4, {2.4, .15}, 0};
  const PolylineReference geometry({{a, b, 10}, {b, c, 11}, {c, d, 11}, {d, a, 11}}, 45);
  std::map<Id, Cell> owned;
  const std::array<Node, 4> ring{a, b, c, d};
  for (int k = 0; k < 4; ++k) {
    Cell cell;
    cell.t = triangle(ring[k], ring[(k + 1) % 4], apex);
    cell.t.id = 10 + k;
    cell.t.protected_cell = k == 0;
    cell.marker[0] = geometry.ComponentOfOriginalFace(ring[k].id, ring[(k + 1) % 4].id);
    for (auto& tensor : cell.nodal_target) tensor = {1, 0, 1 / (.15 * .15)};
    if (k % world.size == world.rank) owned.emplace(cell.t.id, cell);
  }
  EngineOptions options;
  options.geometry_tolerance = 1e-10;
  Engine engine(world, owned, geometry.Policy({{10, .15}}), options);
  Choice choice{{Action::SPLIT, a.id, b.id}, -1, world.rank};
  if (world.rank == 0) {
    const auto& wall = engine.owned.at(10);
    REQUIRE(wall.target_cache[1] < 1.6);
    REQUIRE(wall.target_cache[0] >= .18);
    REQUIRE(*std::max_element(wall.target_cache.begin() + 1, wall.target_cache.end()) > 1.8);
    choice = engine.CellChoice(wall, Action::SPLIT, {});
    CHECK(choice.score > 0);
    CHECK(edge(choice.op.a, choice.op.b) == edge(a.id, b.id));
  }
  CHECK(engine.round(choice) == (world.rank == 0));
  const auto output = Gather(world, engine);
  if (world.rank == 0) {
    const auto marker = geometry.ComponentOfOriginalFace(a.id, b.id);
    int walls = 0;
    for (const auto& cell : output)
      for (int k = 0; k < 3; ++k)
        if (cell.marker[k] == marker) {
          ++walls;
          CHECK(cell.t.protected_cell == 1);
          CHECK(2 * static_cast<double>(area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p) == Approx(.15));
          CHECK(cell.target_cache[0] >= .18);
          for (int j = 1; j < 4; ++j) CHECK(cell.target_cache[j] <= 1.8);
        }
    CHECK(walls == 2);
  }
}

TEST_CASE("Native MPI selection: bounded cache preserves exhaustive choices after publication", "[NativeEngine2D][NativeSelection2D]") {
  World world;
  const auto action = GENERATE(Action::BULK_REMOVE, Action::BULK_SPLIT, Action::BULK_FLIP, Action::BULK_MOVE);
  const bool ordered = GENERATE(false, true);
  std::map<Id, Cell> owned;
  std::vector<PolylineReference::Face> faces;
  // More than64 owned cells even at four ranks; tied proposals, distinct
  // priorities, cross-owner stars and normal rejected operations share a phase.
  for (int group = 0; group < 96; ++group) {
    const Id base = 5 * group;
    const double x = 2 * group;
    const Node center{base, {x + .25, .2}, 0};
    const std::array<Node, 4> ring{{{base + 1, {x - .4, -.4}, 1}, {base + 2, {x + .4, -.4}, 1},
                                  {base + 3, {x + .4, .4}, 1}, {base + 4, {x - .4, .4}, 1}}};
    const double density = group % 3 == 0 ? .25 : group % 3 == 1 ? 16. : 4.;
    for (int k = 0; k < 4; ++k) {
      faces.push_back({ring[k], ring[(k + 1) % 4], 10});
      Cell cell;
      cell.t = triangle(center, ring[k], ring[(k + 1) % 4]);
      cell.t.id = 4 * group + k;
      for (auto& tensor : cell.nodal_target) tensor = {density, 0, density};
      if ((4 * group + k) % world.size == world.rank) owned.emplace(cell.t.id, cell);
    }
  }
  const PolylineReference reference(faces, 45);
  for (auto& entry : owned)
    entry.second.marker[1] = reference.ComponentOfOriginalFace(entry.second.t.v[1].id, entry.second.t.v[2].id);
  EngineOptions options;
  options.geometry_tolerance = 1e-10;
  options.ordered = ordered;
  Engine engine(world, owned, reference.Policy({}), options);
  Engine::SelectionCache cache;
  std::set<Edge> attempted;
  int selections = 0;
  for (int iteration = 0; iteration < 96; ++iteration) {
    const auto exhaustive = engine.choose(action, attempted);
    const auto cached = engine.chooseCached(action, attempted, cache);
    CHECK(cached.score == exhaustive.score);
    CHECK(cached.op.a == exhaustive.op.a);
    CHECK(cached.op.b == exhaustive.op.b);
    CHECK(cached.op.action == exhaustive.op.action);
    if (!world.sum(cached.score >= 0)) break;
    ++selections;
    const bool accepted = engine.round(cached);
    if (ordered) {
      if (!engine.last_committed)
        attempted.insert({engine.last_selected.op.a, engine.last_selected.op.b});
    } else if (cached.score >= 0 && !accepted && !engine.last_deferred) {
      attempted.insert({cached.op.a, cached.op.b});
    }
  }
  CHECK(selections > 0);
  CHECK(engine.stats.selection_scans[int(action)] > 0);
  if (world.rank == 0)
    std::cout << "[native selection] action=" << int(action) << " ordered=" << ordered
              << " selections=" << selections << " full local scans=" << engine.stats.selection_scans[int(action)]
              << '\n';
}

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

TEST_CASE("Native MPI policy: adaptive sampling repairs a frozen-boundary length obstruction", "[NativeFixedAdaptive2D]") {
  World world;
  Case input;
  const bool fixed = GENERATE(false, true);
  auto cells = input.Owned(world);
  for (auto& cell : cells)
    for (auto& target : cell.second.nodal_target) target.xx = 10000;
  auto options = input.Options();
  options.fixed_boundary = fixed;
  Engine engine(world, cells, input.geometry.Policy({{10, input.height}}), options);
  engine.adapt();
  const auto output = Gather(world, engine);
  if (world.rank == 0) {
    const auto target = checked([](Point) { return Tensor{10000, 0, 62500}; });
    const auto fresh = triangles(output);
    const auto faces = physical(output);
    std::string reason;
    REQUIRE(strict_cells(fresh, reason));
    if (fixed) {
      CHECK(faces.size() == 4);
      // The retained base is four metric units long, independently of how the
      // volume is repaired. This proves the fixed policy cannot meet L<=1.8.
      CHECK(max_length(fresh, target) >= 4.);
    } else {
      CHECK(faces.size() > 4);
      CHECK(min_quality(fresh, target) >= .18);
      CHECK(max_length(fresh, target) <= 1.8);
    }
    const auto wall = input.geometry.ComponentOfOriginalFace(0, 1);
    for (const auto& cell : output)
      for (int k = 0; k < 3; ++k)
        if (cell.marker[k] == wall)
          CHECK(static_cast<double>(2 * area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p) ==
                Approx(input.height));
  }
}

TEST_CASE("Native MPI engine: independent wall cavities publish in the same round", "[NativeEngine2D]") {
  World world;
  const bool crossOwner = GENERATE(false, true);
  INFO("cross-owner disjoint cavities: " << crossOwner);
  std::vector<PolylineReference::Face> faces;
  std::map<Id, Cell> local;
  for (int rank = 0; rank < world.size; ++rank) {
    const Id base = 32 * rank;
    const double offset = .1 * rank;
    const Node a{base, {offset, 0}, 1 | FEATURE}, b{base + 1, {offset + .04, 0}, 1 | FEATURE},
        c{base + 2, {offset + .04, .02}, 1 | FEATURE}, d{base + 3, {offset, .02}, 1 | FEATURE};
    faces.insert(faces.end(), {{a, b, 10}, {b, c, 11}, {c, d, 12}, {d, a, 11}});
    {
      Cell lower, upper;
      lower.t = triangle(a, b, c);
      lower.t.id = base + 10;
      upper.t = triangle(a, c, d);
      upper.t.id = base + 11;
      if (rank == world.rank) local.emplace(lower.t.id, lower);
      if ((crossOwner ? (rank + 1) % world.size : rank) == world.rank) local.emplace(upper.t.id, upper);
    }
  }
  const PolylineReference reference(faces, 45);
  for (auto& entry : local) {
    auto& cell = entry.second;
    for (auto& m : cell.nodal_target) m = {625, 0, 62500};
    const Id base = 32 * (cell.t.id / 32);
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
  CHECK(engine.stats.cross_rank == int(crossOwner && world.size > 1));
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
