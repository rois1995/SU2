/*!
 * \file CNativeMesh2D_tests.cpp
 * \brief Native geometry, immutable reference and coupled reconstruction controls.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include <limits>
#include "../../../Common/include/adaptation/CNativeReference2D.hpp"

using namespace SU2NativeBoundary2D;

TEST_CASE("Native geometry: exact orientation and contact rejection", "[NativeMesh2D]") {
  CHECK(Orientation(0, 0, 1, 1, 2, 2) == 0);
  /*--- Exact determinant 2^-104, even though both ordinary binary64 products round to one. ---*/
  const double d = std::ldexp(1., -52);
  CHECK(Orientation(0, 0, 1, 1 + d, 1 - d, 1) > 0);
  CHECK(Orientation(0, 0, 1 - d, 1, 1, 1 + d) < 0);
  const double tiny = std::numeric_limits<double>::denorm_min();
  CHECK(Orientation(0, 0, tiny, 0, 0, tiny) > 0);
  CHECK(Orientation(0, 0, 0, tiny, tiny, 0) < 0);
  const double huge = std::numeric_limits<double>::max();
  CHECK(Orientation(-huge, 0, huge, 0, 0, huge) > 0);
  CHECK_THROWS_AS(Orientation(0, 0, 1, 0, 0, std::numeric_limits<double>::infinity()), std::invalid_argument);
  CHECK(SegmentsIntersect({0, 0}, {2, 0}, {1, 0}, {3, 0}));
  CHECK(SegmentsIntersect({0, 0}, {2, 0}, {1, 0}, {1, 1}));
  CHECK_FALSE(SegmentsIntersect({0, 0}, {1, 0}, {2, 0}, {3, 0}));
  std::vector<Node> touching{{1, {0, 0}, 0}, {2, {2, 0}, 0}, {3, {2, 1}, 0}, {4, {1, 0}, 0}, {5, {0, 1}, 0}};
  CHECK_FALSE(simple(touching));
  std::vector<Node> box{{1, {0, 0}, 0}, {2, {2, 0}, 0}, {3, {2, 1}, 0}, {4, {0, 1}, 0}};
  CHECK(simple(box));
  CHECK(inside({1, .5}, box));
  CHECK_FALSE(inside({1, 0}, box));
  std::vector<Triangle> junction{triangle({1, {0, 0}, 0}, {2, {2, 0}, 0}, {3, {0, 2}, 0}),
                                 triangle({4, {1, 0}, 0}, {5, {3, -1}, 0}, {6, {3, 0}, 0})};
  std::string reason;
  CHECK_FALSE(strict_cells(junction, reason));
}

TEST_CASE("Native reference: component association and closed seam", "[NativeMesh2D]") {
  std::vector<PolylineReference::Face> faces;
  for (const double offset : {0., 1.001}) {
    const Id base = offset == 0 ? 0 : 100;
    std::array<Node, 4> nodes{{{base, {0, offset}, 1},
                               {base + 1, {1, offset}, 1},
                               {base + 2, {1, offset + 1}, 1},
                               {base + 3, {0, offset + 1}, 1}}};
    for (size_t i = 0; i < 4; ++i) faces.push_back({nodes[i], nodes[(i + 1) % 4], 7});
  }
  PolylineReference reference(faces, 45);
  REQUIRE(reference.Components().size() == 2);
  const auto first = reference.ComponentOfOriginalFace(0, 1), second = reference.ComponentOfOriginalFace(100, 101);
  CHECK(first != second);
  CHECK(reference.Marker(first) == reference.Marker(second));
  const auto interval = reference.Interval(first, {0, .2}, {0, 0});
  CHECK(interval.first == Approx(3.8));
  CHECK(interval.second == Approx(4.));
  const auto middle = reference.At(first, (interval.first + interval.second) * .5);
  CHECK(middle.x == 0);
  CHECK(middle.y == Approx(.1));
  CHECK(reference.At(second, .5).y == Approx(1.001));
  CHECK(reference.At(first, .5).y == 0);
  CHECK(reference.IsFeature(0));
  CHECK(reference.IsFeature(102));
  Physical chord{{0, {0, 0}, 1}, {2, {1, 1}, 1}, first};
  CHECK(reference.Deviation(chord) == Approx(std::sqrt(.5)));
  const auto policy = reference.Policy({{7, .002}});
  CHECK(policy.height(first) == .002);
  CHECK(policy.height(second) == .002);
  auto duplicate = faces;
  duplicate.push_back(faces[0]);
  CHECK_THROWS_AS(PolylineReference(duplicate, 45), std::invalid_argument);
}

TEST_CASE("Native kernel: boundary and first-height cells change together", "[NativeMesh2D]") {
  for (const double scale : {1e-100, 1e-4, 1., 1e4, 1e100}) {
    for (const double translation : {0., 1e3 * scale}) {
      INFO("scale=" << scale << " translation=" << translation);
      auto point = [&](double x, double y) { return Point{translation + x * scale, translation + y * scale}; };
      const double h = .004 * scale;
      const Node a{0, point(0, 0), 1 | FEATURE}, b{1, point(.04, 0), 1 | FEATURE}, c{2, point(.04, .02), 1 | FEATURE},
          d{3, point(0, .02), 1 | FEATURE};
      const PolylineReference reference({{a, b, 10}, {b, c, 11}, {c, d, 12}, {d, a, 11}}, 45);
      auto policy = reference.Policy({{10, h}});
      const auto wall = reference.ComponentOfOriginalFace(a.id, b.id);
      Cell lower, upper;
      lower.t = triangle(a, b, c);
      lower.t.id = 10;
      lower.marker[0] = wall;
      upper.t = triangle(a, c, d);
      upper.t.id = 11;
      const std::vector<Cell> old{lower, upper};
      const auto metric = checked([&](Point) { return Tensor{625 / (scale * scale), 0, 62500 / (scale * scale)}; });
      std::vector<Cell> split;
      std::string reason;
      const bool accepted =
          reconstruct({Action::SPLIT, a.id, b.id}, old, policy, metric, 1000, split, reason, 1e-10 * scale);
      INFO(reason);
      REQUIRE(accepted);
      REQUIRE(strict_cells(triangles(split), reason));
      CHECK(physical(split).size() == 2);
      for (const auto& cell : split)
        for (int k = 0; k < 3; ++k)
          if (cell.marker[k]) {
            const double height = static_cast<double>(2 * area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p);
            CHECK(height / h == Approx(1.).epsilon(1e-8));
          }
      std::vector<Cell> coarsened;
      CHECK(reconstruct({Action::REMOVE, 1000, 0}, split, policy, metric, 2000, coarsened, reason, 1e-10 * scale));
      CHECK(physical(coarsened).size() == 1);
      auto changed = reference.Policy({{10, 1.25 * h}});
      std::vector<Cell> rebuilt;
      CHECK(
          reconstruct({Action::HEIGHT, a.id, b.id}, coarsened, changed, metric, 3000, rebuilt, reason, 1e-10 * scale));
      for (const auto& cell : rebuilt)
        for (int k = 0; k < 3; ++k)
          if (cell.marker[k]) {
            const double height = static_cast<double>(2 * area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p);
            CHECK(height / (1.25 * h) == Approx(1.).epsilon(1e-8));
          }
      for (const int fault : {1, 2, 3}) {
        std::vector<Cell> rejected;
        Operation operation{Action::SPLIT, a.id, b.id};
        operation.fault = fault;
        CHECK_FALSE(reconstruct(operation, old, policy, metric, 4000, rejected, reason, 1e-10 * scale));
      }
    }
  }
}

TEST_CASE("Native kernel: coupled tangential apex repair opens an inter-wall wedge", "[NativeMesh2D]") {
  // Coordinates of the two close apices that caused the eighth real SU2 BL cycle
  // to fail. Close their complete stars with an unchanged artificial perimeter.
  const Node a{0, {.023815431230468573, 0}, 1}, b{1, {.02941277152096965, 0}, 1},
      c{2, {.04, 0}, 1}, d{3, {.04, .01}, 1}, e{4, {.023815431230468573, .01}, 1},
      left{5, {.029132904506444596, .0045}, 0}, right{6, {.02994213294492117, .0045}, 0},
      top{7, {.02627391247161338, .007985336090842557}, 0};
  std::vector<Triangle> cells{triangle(a, b, left), triangle(b, c, right), triangle(left, b, right),
                            triangle(top, left, right), triangle(a, left, top), triangle(right, c, d),
                            triangle(top, right, d), triangle(a, top, e), triangle(top, d, e)};
  cells[0].protected_cell = cells[1].protected_cell = 1;
  const auto metric = checked([](Point) { return Tensor{10000., 0., 1 / (.0045 * .0045)}; });
  std::string reason;
  REQUIRE(strict_cells(cells, reason));
  REQUIRE(min_quality(cells, metric) < .18);
  const auto perimeter = boundary(cells);
  const double originalLength = max_length(cells, metric);
  relax_wall_apices(cells, metric);
  REQUIRE(strict_cells(cells, reason));
  CHECK(min_quality(cells, metric) >= .18);
  // Fixed artificial edges are outside this repair; their longest length is unchanged.
  CHECK(max_length(cells, metric) <= originalLength);
  const auto after = boundary(cells);
  REQUIRE(after.size() == perimeter.size());
  for (const auto& edge : perimeter) {
    REQUIRE(after.count(edge.first) == 1);
    const auto face = after.at(edge.first);
    CHECK(face.first.p.x == edge.second.first.p.x);
    CHECK(face.first.p.y == edge.second.first.p.y);
    CHECK(face.second.p.x == edge.second.second.p.x);
    CHECK(face.second.p.y == edge.second.second.p.y);
  }
  for (int i = 0; i < 2; ++i) {
    CHECK(static_cast<double>(2 * area(cells[i])) / norm(cells[i].v[1].p - cells[i].v[0].p) ==
          Approx(.0045).epsilon(1e-12));
  }
}

TEST_CASE("Native wall coarsening: a fitting base cannot undo a graded first-layer repair", "[NativeMesh2D]") {
  const Node a{0, {0, 0}, 1 | FEATURE}, mid{1, {.5, 0}, 1}, b{2, {1, 0}, 1 | FEATURE},
      c{3, {1, .8}, 1 | FEATURE}, d{4, {0, .8}, 1 | FEATURE},
      left{5, {.25, .3}, 0}, right{6, {.75, .3}, 0};
  const PolylineReference reference({{a, mid, 10}, {mid, b, 10}, {b, c, 11}, {c, d, 12}, {d, a, 11}}, 45);
  const auto policy = reference.Policy({{10, .3}});
  const auto metric = checked([](Point p) {
    const double density = 1 + 31 * p.y / .3;
    return Tensor{density, 0, density};  // Affine SPD nodal target, increasing away from the wall.
  });
  const std::vector<Triangle> input{triangle(a, mid, left), triangle(mid, b, right), triangle(left, mid, right),
                                  triangle(a, left, d), triangle(left, right, d), triangle(right, c, d),
                                  triangle(b, c, right)};
  const auto perimeter = boundary(input);
  std::vector<Cell> old;
  for (size_t i = 0; i < input.size(); ++i) {
    Cell cell;
    cell.t = input[i];
    cell.t.id = 10 + i;
    cell.t.protected_cell = i < 2;
    for (int k = 0; k < 3; ++k) {
      const auto edge = SU2Native2D::edge(cell.t.v[k].id, cell.t.v[(k + 1) % 3].id);
      if (perimeter.count(edge))
        cell.marker[k] = reference.ComponentOfOriginalFace(edge.first, edge.second);
    }
    old.push_back(cell);
  }
  std::string reason;
  REQUIRE(strict_cells(input, reason));
  REQUIRE(max_length({input[0], input[1]}, metric) < 1.8);
  REQUIRE(length(a, b, metric) < 1.45);  // The old base-only coarsening gate admits the join.
  const auto action = GENERATE(Action::REMOVE, Action::REDISTRIBUTE);
  Operation operation{action, mid.id};
  operation.parameter = .25;  // Redistribution grows the other base to .75, still below the base limit.
  std::vector<Cell> rejected;
  CHECK_FALSE(reconstruct(operation, old, policy, metric, 100, rejected, reason, 1e-10));
  CHECK(reason == "boundary coarsening or movement misses first-layer metric target");
  // Keeping the existing two wall bases and the same height remains feasible.
  std::vector<Cell> repaired;
  REQUIRE(reconstruct({Action::HEIGHT, a.id, mid.id}, old, policy, metric, 100, repaired, reason, 1e-10));
  for (const auto& cell : repaired)
    if (cell.t.protected_cell) {
      CHECK(max_length({cell.t}, metric) <= 1.8);
      CHECK(quality(cell.t, metric) >= .18);
      CHECK(2 * static_cast<double>(area(cell.t)) / norm(cell.t.v[1].p - cell.t.v[0].p) == Approx(.3));
    }
}

TEST_CASE("Native movement: previously admissible edges cannot accumulate length drift", "[NativeMesh2D]") {
  const Node a{1, {-2, -1}, 1}, b{2, {2, -1}, 1}, c{3, {2, 1}, 1}, d{4, {-2, 1}, 1},
      center{5, {.7, .5}, 0};
  const std::vector<Triangle> old{triangle(a, b, center), triangle(b, c, center),
                                  triangle(c, d, center), triangle(d, a, center)};
  const auto metric = checked([](Point) { return Tensor{1, 0, 1}; });
  Point goal;
  REQUIRE(MovementGoal(nodes(old), center.id, metric, goal));
  CHECK(norm(goal - c.p) > 1.8);  // An unrestricted Laplacian would spoil this spoke.
  std::vector<Triangle> fresh;
  std::string reason;
  auto unrestricted = old;
  for (auto& t : unrestricted)
    for (auto& v : t.v)
      if (v.id == center.id) v.p = goal;
  CHECK(min_quality(unrestricted, metric) > min_quality(old, metric));
  CHECK(max_length(unrestricted, metric) <= max_length(old, metric));
  // The old global length test admitted this move, despite the newly bad spoke.
  CHECK_FALSE(MoveStar(old, center.id, metric, fresh, reason));
  CHECK(nodes(fresh).at(center.id).p.x == center.p.x);
  CHECK(nodes(fresh).at(center.id).p.y == center.p.y);
  REQUIRE(validate_replacement(old, fresh, reason));
  const auto before = nodes(old), after = nodes(fresh);
  for (const auto& t : old)
    for (int k = 0; k < 3; ++k) {
      const auto a = t.v[k], b = t.v[(k + 1) % 3];
      CHECK(length(after.at(a.id), after.at(b.id), metric) <=
            std::max(1.8, length(before.at(a.id), before.at(b.id), metric)) + 1e-12);
    }
}


TEST_CASE("Native reconnection: long unchanged perimeter does not block improving local work", "[NativeMesh2D]") {
  const Node a{1, {0, 0}, 1}, b{2, {3, 0}, 1}, c{3, {3, 1}, 1}, d{4, {0, 2}, 1};
  const auto metric = checked([](Point) { return Tensor{1, 0, 1}; });
  std::vector<Triangle> old{triangle(a, b, d), triangle(b, c, d)}, fresh;
  std::string reason;
  REQUIRE(SU2Native2D::reconstruct({Kind::FLIP, b.id, d.id, 1}, old, metric, 100, fresh, reason));
  REQUIRE(validate_replacement(old, fresh, reason));
  CHECK(min_quality(fresh, metric) > min_quality(old, metric));
  CHECK(max_length(fresh, metric) < max_length(old, metric));
  CHECK(max_length(fresh, metric) > 1.8);  // Still incomplete: later work must repair the perimeter.

  const Node corner{5, {0, 3}, 1}, interior{6, {.1, .1}, 0};
  old = {triangle(a, b, interior), triangle(b, corner, interior), triangle(corner, a, interior)};
  fresh.clear();
  REQUIRE(SU2Native2D::reconstruct({Kind::REMOVE, interior.id, 0, 1}, old, metric, 100, fresh, reason));
  REQUIRE(fresh.size() == 1);
  REQUIRE(validate_replacement(old, fresh, reason));
  CHECK(min_quality(fresh, metric) >= .22);
  CHECK(max_length(fresh, metric) == max_length(old, metric));
}


TEST_CASE("Native triangulation: blocked ears never query outside a concave donor domain", "[NativeMesh2D]") {
  const std::vector<Node> polygon{{0, {0, 0}, 1}, {1, {4, 0}, 1}, {2, {4, 1}, 1},
                                  {3, {1, 1}, 1}, {4, {1, 4}, 1}, {5, {0, 4}, 1}};
  // The convex candidate at vertex0 covers the notch and contains vertex3.
  // Its centroid(4/3,4/3) is outside the donor: geometry must reject it first.
  int samples = 0;
  const auto metric = checked([&](Point p) {
    if (!inside(p, polygon)) throw std::runtime_error("Query outside concave donor.");
    ++samples;
    return Tensor{1, 0, 1};
  });
  std::vector<Triangle> cells;
  REQUIRE(ear_clip(polygon, metric, cells));
  REQUIRE(cells.size() == polygon.size() - 2);
  CHECK(samples > 0);
  std::string reason;
  REQUIRE(strict_cells(cells, reason));
  long double total = 0;
  for (const auto& t : cells) total += area(t);
  CHECK(static_cast<double>(total) == Approx(7.));
}


TEST_CASE("Native wall carving: blocked height children are rejected before donor queries", "[NativeMesh2D]") {
  const std::vector<Node> polygon{{0, {0, 0}, 1}, {1, {4, 0}, 1}, {2, {4, 4}, 1}, {3, {3, 4}, 1},
                                  {4, {3, 1}, 1}, {5, {1, 1}, 1}, {6, {1, 4}, 1}, {7, {0, 4}, 1}};
  std::vector<PolylineReference::Face> faces;
  for (size_t k = 0; k < polygon.size(); ++k)
    faces.push_back({polygon[k], polygon[(k + 1) % polygon.size()], k == 0 ? 10 : 11});
  const PolylineReference reference(faces, 45);
  const auto uniform = checked([](Point) { return Tensor{1, 0, 1}; });
  std::vector<Triangle> triangles;
  REQUIRE(ear_clip(polygon, uniform, triangles));
  std::vector<Cell> old;
  const auto perimeter = boundary(triangles);
  for (auto t : triangles) {
    Cell cell;
    cell.t = t;
    cell.t.id = 10 + old.size();
    for (int k = 0; k < 3; ++k) {
      const auto a = t.v[k], b = t.v[(k + 1) % 3];
      if (perimeter.count(edge(a.id, b.id))) cell.marker[k] = reference.ComponentOfOriginalFace(a.id, b.id);
    }
    old.push_back(cell);
  }
  int samples = 0;
  const auto metric = checked([&](Point p) {
    if (!inside(p, polygon)) throw std::runtime_error("Query outside U-shaped donor.");
    ++samples;
    return Tensor{1, 0, 1};
  });
  std::vector<Cell> fresh;
  std::string reason;
  CHECK_FALSE(reconstruct({Action::HEIGHT, 0, 1}, old, reference.Policy({{10, 3.9}}), metric, 100, fresh, reason));
  CHECK(reason == "no admitted first-height ear; enlargement or different surface resolution required");
  CHECK(samples == 0);
  CHECK(fresh.empty());
}

TEST_CASE("Native insertion: bounded cavity growth reconnects poor midpoint children", "[NativeMesh2D]") {
  const Node a{0, {-1, 0}, 0}, b{1, {1, 0}, 0}, c{2, {0, 10}, 1}, d{3, {0, -10}, 1},
      e{4, {-5, 5}, 1}, f{5, {5, 5}, 1}, g{6, {-5, -5}, 1}, h{7, {5, -5}, 1};
  const auto metric = checked([](Point) { return Tensor{1, 0, 1}; });
  std::vector<Triangle> old{triangle(a, b, c), triangle(b, a, d), triangle(a, c, e),
                            triangle(c, b, f), triangle(d, a, g), triangle(b, d, h)};
  const Node midpoint{100, {0, 0}, 0};
  const std::vector<Triangle> bisected{triangle(a, midpoint, c), triangle(midpoint, b, c),
                                      triangle(b, midpoint, d), triangle(midpoint, a, d)};
  REQUIRE(min_quality(std::vector<Triangle>{old[0], old[1]}, metric) > .18);
  REQUIRE(min_quality(bisected, metric) < .18);
  std::vector<Triangle> fresh;
  std::string reason;
  SECTION("admitted neighbours repair the insertion before publication") {
    REQUIRE(SU2Native2D::reconstruct({Kind::SPLIT, a.id, b.id, 1}, old, metric, midpoint.id, fresh, reason));
    REQUIRE(validate_replacement(old, fresh, reason));
    REQUIRE(strict_cells(fresh, reason));
    CHECK(min_quality(fresh, metric) >= .18);
    CHECK(max_length(fresh, metric) <= max_length(old, metric));
    CHECK(nodes(fresh).size() == nodes(old).size() + 1);
  }
  SECTION("a protected neighbour stays intact while other neighbours reconnect") {
    old[2].protected_cell = 1;
    REQUIRE(SU2Native2D::reconstruct({Kind::SPLIT, a.id, b.id, 1}, old, metric, midpoint.id, fresh, reason));
    REQUIRE(validate_replacement(old, fresh, reason));
    REQUIRE(strict_cells(fresh, reason));
    CHECK(std::count_if(fresh.begin(), fresh.end(), [](const auto& t) { return t.protected_cell; }) == 1);
    const auto kept = std::find_if(fresh.begin(), fresh.end(), [](const auto& t) { return t.protected_cell; });
    REQUIRE(kept != fresh.end());
    for (int k = 0; k < 3; ++k) CHECK(kept->v[k].id == old[2].v[k].id);
    CHECK(std::count_if(fresh.begin(), fresh.end(), [&](const auto& t) { return quality(t, metric) < .18; }) <
          std::count_if(bisected.begin(), bisected.end(), [&](const auto& t) { return quality(t, metric) < .18; }));
  }
  SECTION("a split through the protected wall child is rejected") {
    old[0].protected_cell = 1;
    CHECK_FALSE(SU2Native2D::reconstruct({Kind::SPLIT, a.id, b.id, 1}, old, metric, midpoint.id, fresh, reason));
    CHECK(fresh.empty());
    CHECK(reason == "protected first layer");
  }
}
