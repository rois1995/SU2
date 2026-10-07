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
    // Reproduce a previous donor-edge sample rounded one ulp differently.
    // Its unchanged vertex must retain that authoritative sample on all owners.
    const double cachedXX = std::nextafter(1., 2.);
    for (auto& entry : engine.owned)
      for (int k = 0; k < 3; ++k)
        if (entry.second.t.v[k].id == a.id) entry.second.nodal_target[k].xx = cachedXX;
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
      for (const auto& cell : output)
        for (int k = 0; k < 3; ++k)
          if (cell.t.v[k].id == a.id) CHECK(cell.nodal_target[k].xx == cachedXX);
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

TEST_CASE("Native MPI quality: a fitting free-vertex star requests insertion repair", "[NativeEngine2D][NativeSurfaceShape2D]") {
  World world;
  const std::vector<Node> rim{{0,{0,0},1|FEATURE},{1,{2,0},1|FEATURE},
                              {2,{2,2},1|FEATURE},{3,{0,2},1|FEATURE}};
  const Node center{4,{1,.001},0};
  std::vector<PolylineReference::Face> faces;
  for (size_t k=0;k<rim.size();++k) faces.push_back({rim[k],rim[(k+1)%rim.size()],10});
  const PolylineReference reference(faces,45);
  std::map<Id,Cell> owned;
  for (size_t k=0;k<rim.size();++k) {
    Cell c; c.t=triangle(rim[k],rim[(k+1)%rim.size()],center);c.t.id=10+k;
    c.marker[0]=reference.ComponentOfOriginalFace(rim[k].id,rim[(k+1)%rim.size()].id);
    for (auto& m:c.nodal_target) m={.36,0,.36};
    if (int(k%world.size)==world.rank) owned.emplace(c.t.id,c);
  }
  EngineOptions options;options.geometry_tolerance=1e-10;
  Engine engine(world,owned,reference.Policy({}),options);
  auto choice=engine.choose(Action::BULK_SPLIT,{});
  if (world.rank==0) REQUIRE(choice.score>0);
  else choice.score=-1;
  const bool accepted=engine.round(choice);
  CHECK(accepted==(world.rank==0));
  auto output=Gather(world,engine);
  if (world.rank==0) {
    const auto metric=checked([](Point){return Tensor{.36,0,.36};});
    CHECK(min_quality(triangles(output),metric)>=.18);
    CHECK(max_length(triangles(output),metric)<=1.8);
  }
}

TEST_CASE("Native MPI first-layer apex retains altitude through a cross-owner tangential move", "[NativeEngine2D]") {
  World world;
  const Node a{0,{0,0},1},b{1,{1,0},1},c{2,{1,.25},1},d{3,{0,.25},1},center{4,{.8,.15},0};
  const PolylineReference reference({{a,b,10},{b,c,11},{c,d,11},{d,a,11}},45);
  const auto policy=reference.Policy({{10,.15}});
  const std::array<Node,4> perimeter{{a,b,c,d}};
  std::map<Id,Cell> owned;
  std::vector<Triangle> before;
  for (int k=0;k<4;++k) {
    Cell cell;cell.t=triangle(perimeter[k],perimeter[(k+1)%4],center);cell.t.id=k;
    cell.marker[0]=reference.ComponentOfOriginalFace(perimeter[k].id,perimeter[(k+1)%4].id);
    cell.t.protected_cell=policy.is_wall(cell.marker[0]);
    for (auto& tensor:cell.nodal_target) tensor={1,0,100};
    before.push_back(cell.t);
    if (k%world.size==world.rank) owned.emplace(k,cell);
  }
  EngineOptions options;options.geometry_tolerance=1e-10;
  Engine engine(world,owned,policy,options);
  const bool accepted=engine.round({{Action::BULK_MOVE,center.id},world.rank==0 ? 1. : -1.,world.rank});
  CHECK(world.sum(accepted)==1);
  const auto output=Gather(world,engine);
  if (world.rank==0) {
    CHECK(engine.stats.cross_rank==(world.size>1 ? 1 : 0));
    const auto fresh=triangles(output);
    CHECK(nodes(fresh).at(center.id).p.y==center.p.y);
    CHECK(nodes(fresh).at(center.id).p.x<center.p.x);
    const auto target=checked([](Point){return Tensor{1,0,100};});
    CHECK(min_quality(fresh,target)>min_quality(before,target));
    for (const auto& cell:output) if (cell.t.protected_cell)
      CHECK(2*static_cast<double>(area(cell.t))/norm(cell.t.v[1].p-cell.t.v[0].p)==Approx(.15).epsilon(1e-8));
    std::string reason;REQUIRE(validate_replacement(before,fresh,reason));
  }
}

TEST_CASE("Native MPI movement spends a bounded phase on size repair before legal-shape smoothing", "[NativeEngine2D]") {
  World world;
  std::vector<PolylineReference::Face> faces;std::map<Id,Cell> owned;
  const Node legal{4,{.6,.6},0}, oversized{9,{2.48554697,.51848817},0};
  for (int group=0;group<2;++group) {
    const double x=2*group;const Id base=5*group;
    const std::array<Node,4> ring{{{base,{x,0},1},{base+1,{x+1,0},1},{base+2,{x+1,1},1},{base+3,{x,1},1}}};
    for (int k=0;k<4;++k) {
      faces.push_back({ring[k],ring[(k+1)%4],10});
      Cell cell;cell.t=triangle(ring[k],ring[(k+1)%4],group==0 ? legal : oversized);cell.t.id=4*group+k;
      for (auto& tensor:cell.nodal_target) tensor={1,0,1};
      if ((4*group+k)%world.size==world.rank) owned.emplace(cell.t.id,cell);
    }
  }
  const PolylineReference reference(faces,45);
  for (auto& entry:owned) entry.second.marker[0]=reference.ComponentOfOriginalFace(entry.second.t.v[0].id,entry.second.t.v[1].id);
  EngineOptions options;options.geometry_tolerance=1e-10;options.phase_rounds=1;options.ordered=true;
  options.metric_composition=[](Point p,Tensor) {
    const double m=p.x<1.5 ? 1 : 1+40*(3-p.x)*(3-p.x);return Tensor{m,0,m};
  };
  Engine engine(world,owned,reference.Policy({}),options);
  engine.phase(Action::BULK_MOVE);
  CHECK(world.sum(engine.stats.commits)==1);
  const auto output=Gather(world,engine);
  if (world.rank==0) {
    const auto points=nodes(triangles(output));
    CHECK(points.at(legal.id).p.x==legal.p.x);CHECK(points.at(legal.id).p.y==legal.p.y);
    CHECK(points.at(oversized.id).p.x<oversized.p.x);
    const auto target=checked([&](Point p){return options.metric_composition(p,{1,0,1});});
    const std::array<Node,4> ring{{{5,{2,0},1},{6,{3,0},1},{7,{3,1},1},{8,{2,1},1}}};
    double oldCost=0,newCost=0;
    for (const auto v:ring) {
      oldCost+=std::pow(std::max(0.,length(v,oversized,target)-1.8),2);
      newCost+=std::pow(std::max(0.,length(v,points.at(oversized.id),target)-1.8),2);
    }
    CHECK(newCost<oldCost*.9);
  }
}

TEST_CASE("Native MPI insertion reconnects a graded private fan across owners", "[NativeEngine2D]") {
  World world;
  const Node a{0,{0,.0015818079676982433},1},b{1,{.84964659833923584,.07768100341278919},1},
      c{2,{1.064159052583336,.78732623101466725},1},d{3,{-.026534076052650249,.65153299715349611},1};
  const std::vector<Triangle> input{triangle(a,b,c),triangle(a,c,d)};
  const auto perimeter=boundary(input);std::vector<PolylineReference::Face> faces;
  for (const auto& face:perimeter) faces.push_back({face.second.first,face.second.second,10});
  const PolylineReference reference(faces,45);std::map<Id,Cell> owned;
  for (size_t i=0;i<input.size();++i) {
    Cell cell;cell.t=input[i];cell.t.id=i;
    for (auto& m:cell.nodal_target) m={1,0,1};
    for (int k=0;k<3;++k) if (perimeter.count(edge(cell.t.v[k].id,cell.t.v[(k+1)%3].id)))
      cell.marker[k]=reference.ComponentOfOriginalFace(cell.t.v[k].id,cell.t.v[(k+1)%3].id);
    if (int(i%world.size)==world.rank) owned.emplace(i,cell);
  }
  EngineOptions options;options.geometry_tolerance=1e-10;
  options.metric_composition=[](Point p,Tensor) {
    const double angle=.33058031613373484*p.x,cs=std::cos(angle),sn=std::sin(angle),
      t=1187.576041139243*(1+9.1311707809219804*p.x*p.x),n=1/std::pow(.0003+.2*p.y,2);
    return Tensor{t*cs*cs+n*sn*sn,(t-n)*cs*sn,t*sn*sn+n*cs*cs};
  };
  Engine engine(world,owned,reference.Policy({}),options);
  CHECK(world.sum(engine.round({{Action::BULK_SPLIT,a.id,c.id},world.rank==0 ? 1. : -1.,world.rank}))==1);
  const auto output=Gather(world,engine);
  if (world.rank==0) {
    CHECK(engine.stats.cross_rank==(world.size>1 ? 1 : 0));
    const auto fresh=triangles(output);
    const auto target=checked([&](Point p){return options.metric_composition(p,{1,0,1});});
    CHECK(min_quality(fresh,target)>=.18);CHECK(max_length(fresh,target)<length(a,c,target));
    CHECK(nodes(fresh).size()==nodes(input).size()+1);
    for (const auto& cell:fresh) for (int k=0;k<3;++k)
      CHECK(edge(cell.v[k].id,cell.v[(k+1)%3].id)!=edge(a.id,c.id));
    std::string reason;REQUIRE(validate_replacement(input,fresh,reason));
  }
}

TEST_CASE("Native joint repair: complete dependency publication preserves wall cells and rejects faults",
          "[NativeEngine2D][NativeJoint2D]") {
  World world;
  std::vector<Triangle> input;
  auto node = [](int x, int y) {
    return Node{Id(5 * y + x), {double(x), double(y)}, int(x == 0 || x == 4 || y == 0 || y == 4)};
  };
  for (int y = 0; y < 4; ++y)
    for (int x = 0; x < 4; ++x) {
      auto lower = triangle(node(x, y), node(x + 1, y), node(x + 1, y + 1));
      auto upper = triangle(node(x, y), node(x + 1, y + 1), node(x, y + 1));
      lower.protected_cell = upper.protected_cell = (y == 0);
      input.push_back(lower);
      input.push_back(upper);
    }
  const auto perimeter = boundary(input);
  std::vector<PolylineReference::Face> faces;
  for (const auto& face : perimeter)
    faces.push_back(
        {face.second.first, face.second.second, face.second.first.p.y == 0 && face.second.second.p.y == 0 ? 10 : 11});
  PolylineReference reference(faces, 45);
  const auto policy = reference.Policy({{10, 1.}});
  const auto target = checked([](Point) { return Tensor{2, 0, 2}; });
  std::map<Id, Cell> owned;
  for (size_t i = 0; i < input.size(); ++i) {
    Cell cell;
    cell.t = input[i];
    cell.t.id = i;
    for (auto& tensor : cell.nodal_target) tensor = {2, 0, 2};
    for (int k = 0; k < 3; ++k)
      if (perimeter.count(edge(cell.t.v[k].id, cell.t.v[(k + 1) % 3].id)))
        cell.marker[k] = reference.ComponentOfOriginalFace(cell.t.v[k].id, cell.t.v[(k + 1) % 3].id);
    if (int(i % world.size) == world.rank) owned.emplace(cell.t.id, cell);
  }
  EngineOptions options;
  options.geometry_tolerance = 1e-10;
  SECTION("private two-point reconstruction retains every protected triangle") {
    if (world.rank == 0) {
      const auto fresh = JointPatch(input, target, 100, {1.5, 1.5}, edge(6, 12));
      REQUIRE_FALSE(fresh.empty());
      CHECK(nodes(fresh).size() == nodes(input).size() + 2);
      CHECK(min_quality(fresh, target) >= .18);
      CHECK(max_length(fresh, target) <= max_length(input, target) * (1 + 1e-12));
      CHECK(SizeDeficit(fresh, target) < SizeDeficit(input, target) * (1 - 1e-8));
      std::string reason;
      CHECK(strict_cells(fresh, reason));
      CHECK(validate_replacement(input, fresh, reason));
      for (const auto& old : input)
        if (old.protected_cell) {
          const auto matches = std::count_if(fresh.begin(), fresh.end(), [&](const Triangle& t) {
            return t.protected_cell && t.v[0].id == old.v[0].id && t.v[1].id == old.v[1].id &&
                   t.v[2].id == old.v[2].id && t.v[0].p.x == old.v[0].p.x && t.v[0].p.y == old.v[0].p.y &&
                   t.v[1].p.x == old.v[1].p.x && t.v[1].p.y == old.v[1].p.y && t.v[2].p.x == old.v[2].p.x &&
                   t.v[2].p.y == old.v[2].p.y;
          });
          CHECK(matches == 1);
        }
      CHECK(JointPatch(input, target, 100, {.5, .5}, edge(0, 6)).empty());
    }
  }
  SECTION("second-ring closure publishes consistently across owners") {
    Engine engine(world, owned, policy, options);
    CHECK(world.sum(engine.round({{Action::BULK_SPLIT, 6, 12}, world.rank == 0 ? 1. : -1., world.rank}, true)) == 1);
    CHECK(world.sum(engine.stats.joint_commits) == 1);
    const auto output = Gather(world, engine);
    if (world.rank == 0) {
      CHECK(engine.stats.max_patch > 12);
      CHECK(engine.stats.cross_rank == (world.size > 1 ? 1 : 0));
      const auto fresh = triangles(output);
      std::string reason;
      CHECK(strict_cells(fresh, reason));
      CHECK(validate_replacement(input, fresh, reason));
      CHECK(min_quality(fresh, target) >= .18);
      CHECK(SizeDeficit(fresh, target) < SizeDeficit(input, target));
      CHECK(physical(output).size() == perimeter.size());
      for (const auto& cell : output)
        for (int k = 0; k < 3; ++k)
          if (cell.marker[k] && policy.is_wall(cell.marker[k]))
            CHECK(double(2 * area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p) == Approx(1.));
    }
  }
  SECTION("memory rejection leaves every owned record unchanged") {
    options.dependency_bytes = 64;
    Engine engine(world, owned, policy, options);
    const auto before = Snapshot(engine.owned);
    CHECK_FALSE(engine.round({{Action::BULK_SPLIT, 6, 12}, world.rank == 0 ? 1. : -1., world.rank}, true));
    CHECK(Snapshot(engine.owned) == before);
    CHECK(world.sum(engine.stats.joint_commits) == 0);
    CHECK(world.sum(engine.stats.memory_rejected) > 0);
  }
  SECTION("participant certificate rejection leaves every owned record unchanged") {
    Engine engine(world, owned, policy, options);
    const auto before = Snapshot(engine.owned);
    Choice choice{{Action::BULK_SPLIT, 6, 12}, world.rank == 0 ? 1. : -1., world.rank};
    choice.op.fault = 4;
    CHECK_FALSE(engine.round(choice, true));
    CHECK(Snapshot(engine.owned) == before);
    CHECK(world.sum(engine.stats.joint_commits) == 0);
  }
}

TEST_CASE("Native sweep completion checks geometry and wall height as well as q and length", "[NativeEngine2D]") {
  World world;Case input;
  auto owned=input.Owned(world);
  for(auto& entry:owned)for(auto& metric:entry.second.nodal_target)metric={100,0,100};
  Engine unconstrained(world,owned,input.geometry.Policy({}),input.Options());
  CHECK(unconstrained.satisfied());
  Engine wall(world,owned,input.geometry.Policy({{10,input.height}}),input.Options());
  CHECK_FALSE(wall.satisfied());
  wall.phase(Action::HEIGHT);
  CHECK(wall.satisfied()==(world.sum(wall.stats.commits)>0));
}

TEST_CASE("Native compound surface repair publishes several boundary changes atomically", "[NativeEngine2D][NativeJoint2D]") {
  World world;
  const bool sizeOnly = GENERATE(false, true);
  const double height = sizeOnly ? .6 : .12;
  const double coefficient = sizeOnly ? 1. : .64;
  const Node a{0, {0, 0}, 1 | FEATURE}, b{1, {2, 0}, 1 | FEATURE},
      c{2, {2, height}, 1 | FEATURE}, d{3, {0, height}, 1 | FEATURE};
  const PolylineReference geometry({{a, b, 10}, {b, c, 11}, {c, d, 12}, {d, a, 13}}, 45);
  const auto policy = geometry.Policy({});
  const auto metric = checked([=](Point) { return Tensor{coefficient, 0, coefficient}; });
  std::vector<Cell> input(2);
  input[0].t = triangle(a, b, c);
  input[1].t = triangle(a, c, d);
  input[0].marker = {geometry.ComponentOfOriginalFace(a.id, b.id),
                     geometry.ComponentOfOriginalFace(b.id, c.id), 0};
  input[1].marker = {0, geometry.ComponentOfOriginalFace(c.id, d.id),
                     geometry.ComponentOfOriginalFace(d.id, a.id)};
  std::map<Id, Cell> owned;
  for (size_t i = 0; i < input.size(); ++i) {
    input[i].t.id = i;
    for (auto& tensor : input[i].nodal_target) tensor = {coefficient, 0, coefficient};
    if (int(i % world.size) == world.rank) owned.emplace(i, input[i]);
  }
  EngineOptions options;
  options.geometry_tolerance = 1e-10;
  options.ordered = true;
  SECTION("shape and size stagnation repair imports both owners and splits both walls") {
    std::vector<Cell> single;
    std::string reason;
    REQUIRE(reconstruct({Action::SPLIT, a.id, b.id}, input, policy, metric, 10, single, reason, 1e-10));
    CHECK((min_quality(triangles(single), metric) < .18 || max_length(triangles(single), metric) > 1.8));
    Engine engine(world, owned, policy, options);
    CHECK_FALSE(engine.satisfied());
    CHECK(world.sum(engine.choose(Action::SPLIT, {}, true).score >= 0) > 0);
    engine.repair();
    CHECK(engine.satisfied());
    const int commits = world.sum(engine.stats.joint_commits);
    if (sizeOnly) CHECK(commits >= 2);  // Bulk and surface fallback may cooperate.
    else CHECK(commits == 1);
    if (world.size > 1) CHECK(world.sum(engine.stats.cross_rank) >= 1);
    else CHECK(world.sum(engine.stats.cross_rank) == 0);
    const auto output = Gather(world, engine);
    if (world.rank == 0) {
      const auto fresh = triangles(output);
      CHECK(strict_cells(fresh, reason));
      if (sizeOnly) {
        CHECK(fresh.size() >= 4);
        CHECK(nodes(fresh).size() >= 6);
      } else {
        CHECK(fresh.size() == 4);
        CHECK(nodes(fresh).size() == 6);
      }
      CHECK(min_quality(fresh, metric) >= .18);
      CHECK(max_length(fresh, metric) <= 1.8);
      std::map<int, double> before, after;
      for (const auto& face : physical(input)) before[face.second.marker] += norm(face.second.b.p - face.second.a.p);
      for (const auto& face : physical(output)) {
        after[face.second.marker] += norm(face.second.b.p - face.second.a.p);
        CHECK(policy.deviation(face.second) <= options.geometry_tolerance);
      }
      CHECK(before.size() == after.size());
      for (const auto& marker : before) CHECK(after.at(marker.first) == Approx(marker.second));
      for (const auto& old : nodes(triangles(input))) {
        const auto retained = nodes(fresh).at(old.first);
        CHECK(retained.p.x == old.second.p.x);
        CHECK(retained.p.y == old.second.p.y);
        CHECK(retained.fixed == old.second.fixed);
      }
    }
  }
  SECTION("geometry and participant rejection preserve all owned records") {
    for (const int fault : {2, 4}) {
      Engine engine(world, owned, policy, options);
      const auto before = Snapshot(engine.owned);
      Choice choice{{Action::SPLIT, a.id, b.id}, world.rank == 0 ? 1. : -1., world.rank};
      choice.op.fault = fault;
      CHECK_FALSE(engine.round(choice, true));
      CHECK(Snapshot(engine.owned) == before);
      CHECK(world.sum(engine.stats.joint_commits) == 0);
    }
  }
  SECTION("dependency budget and prescribed-height constraints remain mandatory") {
    options.dependency_bytes = 64;
    Engine engine(world, owned, policy, options);
    const auto before = Snapshot(engine.owned);
    CHECK_FALSE(engine.round({{Action::SPLIT, a.id, b.id}, world.rank == 0 ? 1. : -1., world.rank}, true));
    CHECK(Snapshot(engine.owned) == before);
    CHECK(world.sum(engine.stats.memory_rejected) > 0);
    std::vector<Cell> fresh;
    std::string reason;
    CHECK_FALSE(reconstruct({Action::SPLIT, a.id, b.id}, input, geometry.Policy({{10, .12}}), metric, 10,
                            fresh, reason, 1e-10, true));
    CHECK(fresh.empty());
  }
  SECTION("fixed boundaries do not select compound surface proposals") {
    options.fixed_boundary = true;
    Engine engine(world, owned, policy, options);
    CHECK(world.sum(engine.choose(Action::SPLIT, {}, true).score >= 0) == 0);
  }
}
