/*!
 * \file CNativeField2D_tests.cpp
 * \brief Frozen nodal targets, associated extension and pre-payload admission controls.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeField2D.hpp"

using namespace SU2NativeBoundary2D;

namespace {
Tensor Affine(Point p) { return {4 + p.x + 2 * p.y, .1 + p.x * .05, 8 + 3 * p.x + p.y}; }
std::vector<Cell> Square() {
  const Node a{0, {0, 0}, 1}, b{1, {1, 0}, 1}, c{2, {1, 1}, 1}, d{3, {0, 1}, 1};
  Cell lower, upper;
  lower.t = triangle(a, b, c);
  lower.t.id = 42;
  lower.marker = {1, 4, 0};
  upper.t = triangle(a, c, d);
  upper.t.id = 7;
  upper.marker = {0, 3, 2};
  for (auto* cell : {&lower, &upper})
    for (int k = 0; k < 3; ++k) cell->nodal_target[k] = Affine(cell->t.v[k].p);
  return {lower, upper};
}
}  // namespace

TEST_CASE("Native MPI: frozen P1 target and associated thin-band extension", "[NativeField2D]") {
  World world;
  const auto original = Square();
  std::map<Id, Cell> owned;
  const int owners = std::min(2, world.size);
  for (size_t i = 0; i < original.size(); ++i)
    if (int(i % owners) == world.rank) owned.emplace(original[i].t.id, original[i]);
  DonorField field(world, owned);
  // Changing the working mesh's nodal values cannot mutate the retained donor.
  for (auto& cell : owned)
    for (auto& m : cell.second.nodal_target) m = {100, 0, 100};
  size_t discovered = 0;
  int rejected = 0;
  bool active = world.rank == 0;
  const auto patch = field.import(active ? std::vector<Cell>{original[0]} : std::vector<Cell>{}, active,
                                  2 * 1024 * 1024, .002, discovered, rejected);
  CHECK(rejected == 0);
  CHECK(active == (world.rank == 0));
  if (world.rank == 0) {
    REQUIRE(patch->cells.size() == 2);
    CHECK(patch->cells[0].Key() < patch->cells[1].Key());
    for (const Point query : std::vector<Point>{{.25, .375}, {.5, .5}, {0, 0}, {1, 1}, {.9, .1}}) {
      const auto value = patch->evaluate(query), expected = Affine(query);
      CHECK(value.xx == Approx(expected.xx));
      CHECK(value.xy == Approx(expected.xy));
      CHECK(value.yy == Approx(expected.yy));
    }
    const auto extended = patch->evaluate({.5, -.001}), expected = Affine({.5, 0});
    CHECK(extended.xx == Approx(expected.xx));
    CHECK(extended.xy == Approx(expected.xy));
    CHECK(extended.yy == Approx(expected.yy));
    CHECK(patch->extensions == 1);
    CHECK(patch->maximum_extension == Approx(.001));
    CHECK_THROWS_AS(patch->evaluate({.5, -.003}), std::runtime_error);
    CHECK_THROWS_AS(patch->evaluate({.5, 1.001}),
                    std::runtime_error);  // Component 3 is not associated with this cavity.
    CHECK_THROWS_AS(patch->evaluate({std::numeric_limits<double>::infinity(), 0}), std::runtime_error);
    auto sampled = original[0];
    CacheTarget(sampled, checked([patch](Point p) { return patch->evaluate(p); }), 99);
    CHECK(sampled.target_epoch == 99);
    CHECK(sampled.target_cache[0] > 0);
    for (int k = 0; k < 3; ++k) CHECK(sampled.nodal_target[k].xx == Approx(Affine(sampled.t.v[k].p).xx));
  } else
    CHECK(patch->cells.empty());
  active = world.rank == 0;
  rejected = 0;
  const auto no = field.import(active ? std::vector<Cell>{original[0]} : std::vector<Cell>{}, active, 64, .002,
                               discovered, rejected);
  CHECK_FALSE(active);
  CHECK(no->cells.empty());
  CHECK(rejected == (world.rank == 0 ? 1 : 0));
}

TEST_CASE("Native MPI: receive-buffer admission precedes record exchange", "[NativeField2D]") {
  World world;
  std::vector<std::vector<Cell>> to(world.size);
  to[(world.rank + 1) % world.size] = Square();
  const auto before = world.bytes_sent;
  bool admitted = true;
  const auto no = world.exchange(to, &admitted, 128);
  CHECK_FALSE(admitted);
  CHECK(no.empty());
  CHECK(world.bytes_sent == before);
  CHECK(world.max_exchange_work_bytes > 128);
  const auto yes = world.exchange(to, &admitted, 2 * 1024 * 1024);
  CHECK(admitted);
  CHECK(yes.size() == 2);
  CHECK(world.bytes_sent > before);
}

TEST_CASE("Native MPI: a thin cavity imports its donor cover instead of its enclosing box", "[NativeField2D]") {
  World world;
  const double offset = GENERATE(0., 1e6);
  auto node = [&](Id id, double x, double y) { return Node{id, {offset + x, offset + y}, 0}; };
  std::map<Id, Cell> owned;
  // A valid800-triangle donor square; the narrow diagonal cavity's enclosing
  // box overlaps every donor, but its actual padded cover fits FIELD_LIMIT.
  for (int j = 0; j < 20; ++j)
    for (int i = 0; i < 20; ++i) {
      const Node a = node(j * 21 + i, i / 20., j / 20.), b = node(j * 21 + i + 1, (i + 1) / 20., j / 20.),
                 c = node((j + 1) * 21 + i + 1, (i + 1) / 20., (j + 1) / 20.),
                 d = node((j + 1) * 21 + i, i / 20., (j + 1) / 20.);
      for (int k = 0; k < 2; ++k) {
        Cell cell;
        cell.t = k ? triangle(a, c, d) : triangle(a, b, c);
        cell.t.id = 2 * (j * 20 + i) + k;
        for (int slot = 0; slot < 3; ++slot) cell.nodal_target[slot] = Affine(cell.t.v[slot].p);
        if (int(cell.t.id % world.size) == world.rank) owned.emplace(cell.t.id, cell);
      }
    }
  DonorField field(world, owned);
  Cell lower, upper;
  lower.t = triangle(node(1000, 0, 0), node(1001, 1, .99), node(1002, 1, 1));
  upper.t = triangle(node(1000, 0, 0), node(1002, 1, 1), node(1003, 0, .01));
  bool active = world.rank == 0;
  size_t discovered = 0;
  int rejected = 0;
  const auto patch = field.import(active ? std::vector<Cell>{lower, upper} : std::vector<Cell>{}, active,
                                   2 * 1024 * 1024, .002, discovered, rejected);
  CHECK(rejected == 0);
  CHECK(active == (world.rank == 0));
  if (world.rank == 0) {
    REQUIRE_FALSE(patch->cells.empty());
    CHECK(patch->cells.size() <= FIELD_LIMIT);
    CHECK(std::adjacent_find(patch->cells.begin(), patch->cells.end(),
                            [](const auto& a, const auto& b) { return a.Key() == b.Key(); }) == patch->cells.end());
    for (const Point p : std::vector<Point>{{0, 0}, {1, 1}, {.5, .5}, {.9, .895}, {.5, .507}}) {
      // Last query is outside the thin cavity, inside its padded neighborhood.
      const Point query{offset + p.x, offset + p.y};
      const auto actual = patch->evaluate(query), expected = Affine(query);
      CHECK(actual.xx == Approx(expected.xx));
      CHECK(actual.xy == Approx(expected.xy));
      CHECK(actual.yy == Approx(expected.yy));
    }
    CHECK_THROWS_AS(patch->evaluate({offset + .2, offset + .8}), std::runtime_error);
  } else
    CHECK(patch->cells.empty());
}

TEST_CASE("Native field: roundoff-only containment follows exact donor search", "[NativeField2D]") {
  const Node a{0, {.2, 0}, 1}, c{2, {.2 + .04, .02}, 1}, d{3, {.2, .02}, 1};
  DonorCell upper;
  upper.triangle = triangle(a, c, d);
  upper.metric = {Tensor{4, 0, 8}, Tensor{4, 0, 8}, Tensor{4, 0, 8}};
  const Point rounded{static_cast<double>((static_cast<long double>(a.p.x) + c.p.x) / 2), .01};
  CHECK(orient(a.p, c.p, rounded) < 0);
  FieldPatch patch;
  patch.cells = {upper};
  CHECK(patch.evaluate(rounded).xx == 4);
  CHECK(patch.roundoff_queries == 1);
  CHECK(patch.extensions == 0);
  // Just outside the donor coordinate box must still reach the numerical
  // containment fallback; a box shortcut must not become a domain gate.
  CHECK(patch.evaluate({std::nextafter(a.p.x, -std::numeric_limits<double>::infinity()), .01}).xx == 4);
  CHECK(patch.roundoff_queries == 2);
  CHECK_THROWS_AS(patch.evaluate({rounded.x + 1e-10, rounded.y}), std::runtime_error);
  patch.cache.clear();
  patch.recent.clear();
  patch.cells[0].marker[0] = 7;
  CHECK_THROWS_AS(patch.evaluate(rounded), std::runtime_error);
  patch.extension_components.insert(7);
  CHECK(patch.evaluate(rounded).yy == 8);
}

TEST_CASE("Native MPI: too many original donors reject before target payload", "[NativeField2D]") {
  World world;
  std::map<Id, Cell> owned;
  // Many cells overlap the probe box: the metadata phase must return a bounded sentinel.
  // These controls exercise admission only; this overlapping donor is not a valid CFD input.
  for (Id i = 0; i < FIELD_LIMIT + 1; ++i)
    if (int(i % world.size) == world.rank) {
      auto cell = Square()[0];
      cell.t.id = 1000 + i;
      for (auto& node : cell.t.v) node.id += 3 * i;
      owned.emplace(cell.t.id, cell);
    }
  DonorField field(world, owned);
  bool active = world.rank == 0;
  size_t discovered = 0;
  int rejected = 0;
  const auto patch =
      field.import(active ? Square() : std::vector<Cell>{}, active, 2 * 1024 * 1024, 0, discovered, rejected);
  CHECK_FALSE(active);
  CHECK(patch->cells.empty());
  CHECK(rejected == (world.rank == 0 ? 1 : 0));
}

TEST_CASE("Native MPI: geometric constraints apply after donor interpolation", "[NativeField2D][NativeComposite2D]") {
  World world;
  const auto original = Square();
  std::map<Id, Cell> owned;
  for (size_t k = 0; k < original.size(); ++k)
    if (int(k % world.size) == world.rank) owned.emplace(original[k].t.id, original[k]);
  const MetricComposition compose = [](Point p, Tensor sensor) {
    const double h = 1e-5 + .2 * std::max(0., p.y);
    sensor.yy = std::max(sensor.yy, 1 / (h * h));
    return sensor;
  };
  DonorField donor(world, owned, compose);
  bool active = world.rank == 0;
  size_t discovered = 0;
  int rejected = 0;
  auto patch = donor.import(active ? original : std::vector<Cell>{}, active, 2 * 1024 * 1024,
                             .002, discovered, rejected);
  REQUIRE(rejected == 0);
  if (world.rank == 0) {
    REQUIRE(active);
    for (const Point p : std::vector<Point>{{.5, 0}, {.5, .01}, {.5, .2}, {.5, 1}}) {
      const auto expected = compose(p, Affine(p));
      const auto actual = patch->evaluate(p);
      CHECK(actual.xx == Approx(expected.xx));
      CHECK(actual.xy == Approx(expected.xy));
      CHECK(actual.yy == Approx(expected.yy));
      CHECK(patch->evaluate(p).yy == actual.yy); // Cached samples retain the composed field.
    }
    CHECK(patch->evaluate({.5, .2}).yy < 1000); // No coarse-cell spreading of the wall's 1e10 value.
    auto sample = original[0];
    CacheTarget(sample, checked([patch](Point p) { return patch->evaluate(p); }), 123);
    CHECK(sample.target_cache[0] == Approx(quality(sample.t, checked([patch](Point p) {return patch->evaluate(p);} ))));
  }
}

TEST_CASE("Native field: saturated query cache retains recent points and authoritative vertices", "[NativeField2D]") {
  FieldPatch patch;
  for (const auto& cell : Square()) {
    DonorCell donor;
    donor.triangle = cell.t;
    donor.metric = cell.nodal_target;
    patch.cells.push_back(donor);
  }
  patch.Sort();
  const std::array<double, 2> anchor{0, 0};
  const Tensor authoritative{std::nextafter(4., 5.), .1, 8};
  patch.cache.emplace(anchor, authoritative);
  Point last;
  for (size_t i = 1; i <= 2 * QUERY_CACHE_LIMIT; ++i) {
    last = {double(i) / (2 * QUERY_CACHE_LIMIT + 1), .25};
    CHECK(patch.evaluate(last).xx == Approx(Affine(last).xx));
    REQUIRE(patch.cache.size() <= QUERY_CACHE_LIMIT);
  }
  const auto evaluated = patch.samples;
  CHECK(patch.evaluate(last).xx == Approx(Affine(last).xx));
  CHECK(patch.samples == evaluated);
  CHECK(patch.evaluate({0, 0}).xx == authoritative.xx);
  CHECK(patch.samples == evaluated);
  CHECK(patch.evaluate({-0., 0.}).xx == authoritative.xx);
  CHECK(patch.samples == evaluated);
  CHECK(patch.evictions > 0);
  CHECK(patch.recent.size() == QUERY_CACHE_LIMIT - 1);
}

TEST_CASE("Native MPI: indexed discovery agrees with exact donor cover", "[NativeField2D]") {
  World world;
  const double offset = GENERATE(0., 1e6);
  std::map<Id, Cell> owned;
  std::vector<Cell> original;
  for (int j = 0; j < 12; ++j)
    for (int i = 0; i < 12; ++i) {
      auto node = [&](int di, int dj) {
        return Node{Id((j + dj) * 13 + i + di), {offset + (i + di) / 12., offset + (j + dj) / 12.}, 0};
      };
      const auto a = node(0, 0), b = node(1, 0), c = node(1, 1), d = node(0, 1);
      for (int k = 0; k < 2; ++k) {
        Cell cell;
        cell.t = k ? triangle(a, c, d) : triangle(a, b, c);
        cell.t.id = 10000 - 2 * (j * 12 + i) - k;  // IDs oppose spatial/insertion order.
        for (int v = 0; v < 3; ++v) cell.nodal_target[v] = Affine(cell.t.v[v].p);
        original.push_back(cell);
        if (int(cell.t.id % world.size) == world.rank) owned.emplace(cell.t.id, cell);
      }
    }
  DonorField field(world, owned);
  for (int q = 0; q < 12; ++q) {
    const double x = offset + .1 + .05 * q, y = offset + .1 + .04 * ((q + world.rank) % 12);
    Cell cavity;
    cavity.t = triangle(Node{900, {x, y}, 0}, Node{901, {x + .1, y}, 0}, Node{902, {x, y + .1}, 0});
    FieldRegion region;
    region.lo = {x, y};
    region.hi = {x + .1, y + .1};
    for (int k = 0; k < 3; ++k) region.triangle[k] = cavity.t.v[k].p;
    const double padding = .004;
    const double boxPadding = padding + 32 * std::numeric_limits<double>::epsilon() *
                                           std::max({1., std::abs(x), std::abs(y), std::abs(x + .1), std::abs(y + .1)});
    for (int k = 0; k < 2; ++k) {
      region.lo[k] = std::nextafter(region.lo[k] - boxPadding, -std::numeric_limits<double>::infinity());
      region.hi[k] = std::nextafter(region.hi[k] + boxPadding, std::numeric_limits<double>::infinity());
    }
    std::set<std::array<Id, 3>> expected;
    for (const auto& cell : original) {
      const std::array<Point, 3> points{cell.t.v[0].p, cell.t.v[1].p, cell.t.v[2].p};
      if (region.Overlaps(points, padding)) {
        DonorCell donor;
        donor.triangle = cell.t;
        expected.insert(donor.Key());
      }
    }
    bool active = true;
    size_t discovered = 0;
    int rejected = 0;
    const auto patch = field.import({cavity}, active, 2 * 1024 * 1024, .002, discovered, rejected);
    CHECK(active);
    CHECK(rejected == 0);
    std::set<std::array<Id, 3>> actual;
    for (const auto& cell : patch->cells) actual.insert(cell.Key());
    CHECK(actual == expected);
    CHECK(patch->cells.size() == actual.size());
    CHECK(std::is_sorted(patch->cells.begin(), patch->cells.end(),
                         [](const auto& a, const auto& b) { return a.Key() < b.Key(); }));
    const auto value = patch->evaluate({x + .025, y + .025}), affine = Affine({x + .025, y + .025});
    CHECK(value.xx == Approx(affine.xx));
    CHECK(value.xy == Approx(affine.xy));
    CHECK(value.yy == Approx(affine.yy));
  }
  if constexpr (std::is_same<su2double, double>::value) {
    CHECK(field.search_candidates < field.full_scan_equivalent);
    CHECK(field.SearchBytes() > 0);
  }
}
