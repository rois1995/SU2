/*!
 * \file CNativeCollision2D_tests.cpp
 * \brief Remote physical snapshot collision controls beyond a locally valid cavity.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeEngine2D.hpp"
#include "../../../Common/include/adaptation/CNativeReference2D.hpp"

using namespace SU2NativeBoundary2D;

TEST_CASE("Native MPI collision: locally valid physical edits cannot cross or swallow a remote component",
          "[NativeCollision]") {
  World world;
  const int scenario = GENERATE(0, 1, 2);
  const double depth = scenario == 0 ? .008 : .02;
  const double x0 = scenario == 2 ? .018 : .012, x1 = scenario == 2 ? .022 : .028;
  const double y0 = scenario == 2 ? -.011 : -.018, y1 = scenario == 2 ? -.010 : -.012;
  const Node a{0, {0, 0}, 1 | FEATURE}, b{1, {.04, 0}, 1 | FEATURE}, c{2, {.04, .02}, 1 | FEATURE},
      d{3, {0, .02}, 1 | FEATURE}, e{10, {x0, y0}, 1 | FEATURE}, f{11, {x1, y0}, 1 | FEATURE},
      g{12, {x1, y1}, 1 | FEATURE}, h{13, {x0, y1}, 1 | FEATURE};
  const std::vector<PolylineReference::Face> faces{{a, b, 0}, {b, c, 1}, {c, d, 2}, {d, a, 1},
                                                   {e, f, 3}, {f, g, 3}, {g, h, 3}, {h, e, 3}};
  const PolylineReference reference(faces, 45);
  std::map<Id, Cell> local;
  size_t index = 0;
  for (const auto t : {triangle(a, b, c), triangle(a, c, d), triangle(e, f, g), triangle(e, g, h)}) {
    Cell cell;
    cell.t = t;
    cell.t.id = 100 + index;
    for (auto& m : cell.nodal_target) m = {625, 0, 625};
    for (int k = 0; k < 3; ++k)
      for (const auto& face : faces)
        if (edge(face.a.id, face.b.id) == edge(t.v[k].id, t.v[(k + 1) % 3].id))
          cell.marker[k] = reference.ComponentOfOriginalFace(face.a.id, face.b.id);
    // First cavity is on rank 0; unrelated physical component is on the last rank.
    if ((index < 2 ? 0 : world.size - 1) == world.rank) local.emplace(cell.t.id, cell);
    ++index;
  }
  auto policy = reference.Policy({});
  const auto originalPoint = policy.point;
  const int wall = reference.ComponentOfOriginalFace(0, 1);
  // Deliberately bad projection policy: test the snapshot guard independently of reference construction.
  policy.point = [=](int component, double u) {
    auto point = originalPoint(component, u);
    if (component == wall && u > 0 && u < .04) point.y = -depth;
    return point;
  };
  EngineOptions options;
  options.geometry_tolerance = .1;
  Engine engine(world, local, policy, options);
  const auto before = engine.owned;
  std::vector<char> beforeBytes;
  RecordStream snapshot(beforeBytes);
  for (auto cell : before) snapshot(cell.second);
  Choice choice{{Action::SPLIT, 0, 1}, world.rank == 0 ? 1. : -1., world.rank};
  const bool committed = engine.round(choice);
  CHECK(world.sum(committed) == (scenario == 0 ? 1 : 0));
  if (scenario != 0) {
    std::vector<char> afterBytes;
    RecordStream after(afterBytes);
    for (auto cell : engine.owned) after(cell.second);
    CHECK(afterBytes == beforeBytes);
    if (world.rank == 0) CHECK(engine.stats.rejected["physical collision against current snapshot"] == 1);
  }
}
