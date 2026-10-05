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
