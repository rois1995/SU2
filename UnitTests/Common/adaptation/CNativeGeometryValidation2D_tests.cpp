/*!
 * \file CNativeGeometryValidation2D_tests.cpp
 * \brief Global physical graph controls: contact, nesting, holes, and disconnected fluid components.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeGeometryValidation2D.hpp"

using namespace SU2NativeBoundary2D;
namespace {
using Faces = std::vector<PolylineReference::Face>;
Faces Loop(const std::vector<Point>& vertices, Id base) {
  Faces result;
  for (size_t k = 0; k < vertices.size(); ++k)
    result.push_back({{base + k, vertices[k], 0},
                      {base + (k + 1) % vertices.size(), vertices[(k + 1) % vertices.size()], 0},
                      int(k % 2)});
  return result;
}
Faces Join(Faces a, const Faces& b) {
  a.insert(a.end(), b.begin(), b.end());
  return a;
}
}  // namespace

TEST_CASE("Native physical graph: fluid-left closure and exact global contacts", "[NativeGeometry]") {
  const auto square = Loop({{0, 0}, {1, 0}, {1, 1}, {0, 1}}, 0);
  CHECK_NOTHROW(ValidatePhysicalGraph(square));
  CHECK_NOTHROW(ValidatePhysicalGraph(Join(square, Loop({{2, 0}, {3, 0}, {3, 1}, {2, 1}}, 10))));
  CHECK_NOTHROW(ValidatePhysicalGraph(Join(square, Loop({{.2, .2}, {.2, .8}, {.8, .8}, {.8, .2}}, 10))));
  CHECK_THROWS(ValidatePhysicalGraph(Join(square, Loop({{.2, .2}, {.8, .2}, {.8, .8}, {.2, .8}}, 10))));
  CHECK_THROWS(ValidatePhysicalGraph(Loop({{0, 0}, {0, 1}, {1, 1}, {1, 0}}, 0)));
  CHECK_THROWS(ValidatePhysicalGraph(Loop({{0, 0}, {1, 1}, {0, 1}, {1, 0}}, 0)));
  CHECK_THROWS(ValidatePhysicalGraph(Join(square, Loop({{.5, .5}, {1.5, .5}, {1.5, 1.5}, {.5, 1.5}}, 10))));
  CHECK_THROWS(ValidatePhysicalGraph(Join(square, Loop({{1, 1}, {2, 1}, {2, 2}, {1, 2}}, 10))));
  CHECK_THROWS(ValidatePhysicalGraph(Loop({{0, 0}, {1, 0}, {.5, 0}, {1, 1}, {0, 1}}, 0)));
  auto open = square;
  open.pop_back();
  CHECK_THROWS(ValidatePhysicalGraph(open));
  auto branch = Join(square, Loop({{0, 0}, {-1, 0}, {-1, -1}, {0, -1}}, 10));
  branch[4].a.id = branch[7].b.id = 0;
  CHECK_THROWS(ValidatePhysicalGraph(branch));
  auto island = Join(square, Loop({{.2, .2}, {.2, .8}, {.8, .8}, {.8, .2}}, 10));
  island = Join(island, Loop({{.3, .3}, {.7, .3}, {.7, .7}, {.3, .7}}, 20));
  CHECK_NOTHROW(ValidatePhysicalGraph(island));
  // Collinear samples at a lexicographic extreme do not change orientation or create contacts.
  CHECK_NOTHROW(ValidatePhysicalGraph(Loop({{0, 0}, {.5, 0}, {1, 0}, {1, 1}, {0, 1}, {0, .5}}, 0)));
}
