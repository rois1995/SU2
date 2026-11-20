/*!
 * \file CNativeLocalCore3DResumeHOperators_tests.cpp
 * \brief Shape protection, exact publication and float sampling for local tetrahedral trials.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetricTransport3D.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;
using Tet = std::array<size_t, 4>;
using FaceKey = std::array<Id, 3>;

Owned BoundMesh(const std::vector<Point>& points, const std::vector<Tet>& cells, Tensor metric = {}) {
  Owned result;
  std::map<FaceKey, std::vector<std::pair<Id, unsigned>>> faces;
  for (const auto& tet : cells) {
    OwnedCell row;
    row.cell.id = result.size() + 1;
    row.sensor_present = true;
    row.sensor.fill(metric);
    auto& v = row.cell.v;
    for (unsigned i = 0; i < 4; ++i) v[i] = {tet[i], points.at(tet[i])};
    if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) < 0) std::swap(v[0], v[1]);
    REQUIRE(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0);
    for (unsigned f = 0; f < 4; ++f) {
      FaceKey face{};
      for (unsigned i = 0, j = 0; i < 4; ++i)
        if (i != f) face[j++] = v[i].id;
      std::sort(face.begin(), face.end());
      faces[face].emplace_back(row.cell.id, f);
    }
    result.emplace(row.cell.id, row);
  }
  Id facet = 1;
  for (const auto& entry : faces) {
    const auto& uses = entry.second;
    REQUIRE(uses.size() <= 2);
    if (uses.size() != 1) continue;
    const auto [id, face] = uses.front();
    result.at(id).physical[face] = {true, facet++};
  }
  return result;
}

local::Slot SlotOf(const local::Mesh& mesh, Id id) {
  const auto& points = mesh.Points();
  const auto found = std::find_if(points.begin(), points.end(), [&](const auto& point) { return point.node.id == id; });
  REQUIRE(found != points.end());
  return static_cast<local::Slot>(found - points.begin());
}

Owned LayeredBipyramid(double height) {
  std::vector<Point> points{{.625, 0, 0}, {-.25, .5, 0}, {-.25, -.5, 0}, {0, 0, .75},
                            {0, 0, -1. / 64.}, {0, 0, height}};
  constexpr unsigned outer = 6;
  for (size_t i = 0; i < 5; ++i) {
    const auto p = points[i];
    points.push_back({2 * p.x, 2 * p.y, 2 * p.z - .25});
  }
  std::vector<Tet> cells;
  for (const size_t tip : {3U, 4U})
    for (size_t i = 0; i < 3; ++i) {
      const size_t a = i, b = (i + 1) % 3;
      cells.push_back({5, a, b, tip});
      const size_t lo = std::min(a, b), hi = std::max(a, b);
      // Ordering the radial frusta by point id makes their diagonals agree on every radial face.
      cells.push_back({lo, hi, tip, tip + outer});
      cells.push_back({lo, hi, hi + outer, tip + outer});
      cells.push_back({lo, lo + outer, hi + outer, tip + outer});
    }
  return BoundMesh(points, cells);
}

double Worst(const local::Mesh& mesh, local::Slot point) {
  double result = std::numeric_limits<double>::infinity();
  for (const auto& row : mesh.Cells())
    if (row.live && std::find(row.vertices.begin(), row.vertices.end(), point) != row.vertices.end())
      result = std::min(result, mesh.Margin(row.vertices));
  return result;
}

std::vector<char> Snapshot(const local::Mesh& mesh) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  auto points = mesh.Points().size(), cells = mesh.Cells().size();
  stream(points, cells);
  for (auto p : mesh.Points())
    stream(p.node, p.tensor, p.incident, p.surface, p.border, p.removed, p.wall_near, p.unresolved, p.changed_at);
  for (local::Slot i = 0; i < cells; ++i) {
    auto row = mesh.Cells()[i];
    auto bindings = mesh.Bindings(i);
    stream(row.vertices, row.neighbors, row.id, row.version, row.binding, row.stamp, row.live, row.changed,
           row.failed_at, bindings);
  }
  return bytes;
}

double TrialVolume(Point a, Point b, Point c, Point d) {
  const Point u{b.x - a.x, b.y - a.y, b.z - a.z}, v{c.x - a.x, c.y - a.y, c.z - a.z},
      w{d.x - a.x, d.y - a.y, d.z - a.z};
  // Isolate each binary64 rounding so the test's compiler flags cannot contract the witness determinant.
  const volatile double yz = v.y * w.z, zy = v.z * w.y, zx = v.z * w.x, xz = v.x * w.z, xy = v.x * w.y,
                        yx = v.y * w.x;
  const volatile double x = u.x * (yz - zy), y = u.y * (zx - xz), z = u.z * (xy - yx);
  const volatile double sum = x + y;
  return sum + z;
}

Owned RoundedSplitRing() {
  constexpr double ulp = 0x1p-52;
  constexpr Point a{30. / 16. + ulp, 27. / 16. + ulp, 17. / 16.};
  constexpr Point b{17. / 16. + ulp, 23. / 16., 29. / 16.};
  constexpr Point c{19. / 16., 19. / 16., 21. / 16.}, e{24. / 16., 30. / 16., 27. / 16.};
  const Point m{.5 * (a.x + b.x), .5 * (a.y + b.y), .5 * (a.z + b.z)};
  for (int perturb = 0; perturb <= 4; ++perturb) {
    const Point d{29. / 16., 29. / 16., 21. / 16. + perturb * ulp};
    const std::array<Point, 3> rim{c, d, e};
    bool trials_positive = true;
    for (size_t i = 0; i < rim.size(); ++i) {
      const auto p = rim[i], q = rim[(i + 1) % rim.size()];
      trials_positive &= Orientation(a, b, p, q) > 0 && TrialVolume(m, b, p, q) > 0 && TrialVolume(a, m, p, q) > 0;
    }
    if (trials_positive && Orientation(m, b, c, d) < 0) {
      std::cout << "FP1_FIXTURE h_split perturb_ulps=" << perturb << " float=" << TrialVolume(m, b, c, d)
                << " exact=" << Orientation(m, b, c, d) << '\n';
      return BoundMesh({a, b, c, d, e}, {{0, 1, 2, 3}, {0, 1, 3, 4}, {0, 1, 4, 2}}, {4, 0, 0, 4, 0, 4});
    }
  }
  FAIL("No rounded midpoint entered the opposite side of the trial plane.");
  return {};
}

Owned RegularBox() {
  constexpr size_t n = 4;
  constexpr auto index = [](size_t x, size_t y, size_t z) { return (z * (n + 1) + y) * (n + 1) + x; };
  constexpr std::array<size_t, 6> ring{1, 2, 3, 7, 4, 5};
  constexpr auto a = index(2, 2, 2), b = index(2, 3, 2);
  std::vector<Point> points;
  for (size_t z = 0; z <= n; ++z)
    for (size_t y = 0; y <= n; ++y)
      for (size_t x = 0; x <= n; ++x) points.push_back({double(x), double(y), double(z)});
  const auto middle = points.size();
  points.push_back({2, 2 + 3. / 16., 2});
  std::vector<Tet> cells;
  for (size_t z = 0; z < n; ++z)
    for (size_t y = 0; y < n; ++y)
      for (size_t x = 0; x < n; ++x) {
        const std::array<size_t, 8> v{index(x, y, z), index(x + 1, y, z), index(x + 1, y + 1, z), index(x, y + 1, z),
                                       index(x, y, z + 1), index(x + 1, y, z + 1), index(x + 1, y + 1, z + 1),
                                       index(x, y + 1, z + 1)};
        for (size_t i = 0; i < ring.size(); ++i) {
          const Tet tet{v[0], v[6], v[ring[i]], v[ring[(i + 1) % ring.size()]]};
          if (std::find(tet.begin(), tet.end(), a) == tet.end() || std::find(tet.begin(), tet.end(), b) == tet.end()) {
            cells.push_back(tet);
            continue;
          }
          for (const auto end : {a, b}) {
            auto child = tet;
            *std::find(child.begin(), child.end(), end) = middle;
            cells.push_back(child);
          }
        }
      }
  return BoundMesh(points, cells);
}

std::array<double, 6> Components(Tensor m) { return {m.xx, m.xy, m.xz, m.yy, m.yz, m.zz}; }

std::vector<DonorCell> DonorStar(Point center) {
  const auto owned = BoundMesh({{}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, center},
                               {{4, 0, 1, 2}, {4, 0, 1, 3}, {4, 0, 2, 3}, {4, 1, 2, 3}});
  std::vector<DonorCell> donors;
  for (const auto& entry : owned) {
    DonorCell donor{entry.second.cell, {}};
    for (unsigned i = 0; i < 4; ++i) {
      const auto p = donor.cell.v[i].p;
      donor.sensor[i] = {2 + p.x / 4, p.x / 64, p.y / 64, 3 + p.y / 4, p.z / 32, 4 + p.z / 4};
    }
    donors.push_back(donor);
  }
  return donors;
}
}  // namespace

TEST_CASE("Local collapse refuses newly worse bad shape and can improve already bad shape", "[.][NativeLocalCoreHOperators]") {
  local::CoreOptions options;
  options.surface_first = false;
  options.float_trials = false;
  for (const bool good_start : {true, false}) {
    CAPTURE(good_start);
    const auto rows = LayeredBipyramid(good_start ? .25 : -1. / 64. + 1. / 8192.);
    local::Mesh guarded(rows, false, options);
    auto unguarded_options = options;
    unguarded_options.shape_test = false;
    local::Mesh unguarded(rows, false, unguarded_options);
    guarded.MarkWallNear();
    unguarded.MarkWallNear();
    const auto a = SlotOf(guarded, 5), b = SlotOf(guarded, 0);
    REQUIRE(guarded.Points()[a].Free());
    REQUIRE_FALSE(guarded.Points()[a].wall_near);
    REQUIRE(guarded.Points()[b].wall_near);
    REQUIRE(guarded.Length({a, b}) < local::ShortEdge);
    const auto old_margin = Worst(guarded, a);
    local::PassStats legacy_stats;
    REQUIRE(unguarded.Collapse({a, b}, 1, legacy_stats) == local::Refusal::None);
    REQUIRE_NOTHROW(unguarded.Check());
    double new_margin = std::numeric_limits<double>::infinity();
    for (const auto& row : unguarded.Cells())
      if (row.live && row.changed) new_margin = std::min(new_margin, unguarded.Margin(row.vertices));
    REQUIRE(new_margin < local::BadMargin);

    local::PassStats stats;
    if (good_start) {
      REQUIRE(old_margin >= local::BadMargin);
      const auto before = Snapshot(guarded);
      CHECK(guarded.Collapse({a, b}, 1, stats) == local::Refusal::Shape);
      CHECK(stats.refused_shape == 1);
      CHECK(Snapshot(guarded) == before);
    } else {
      REQUIRE(old_margin < new_margin);
      REQUIRE(guarded.Collapse({a, b}, 1, stats) == local::Refusal::None);
      CHECK(stats.refused_shape == 0);
      CHECK(Snapshot(guarded) == Snapshot(unguarded));
      CHECK_NOTHROW(guarded.Check());
    }
    std::cout << "FP1_FIXTURE h_shape good_start=" << good_start << " old=" << old_margin << " fresh=" << new_margin
              << " refused_shape=" << stats.refused_shape << '\n';
  }
}

TEST_CASE("Local split rejects a rounded midpoint that passes the float trial", "[.][NativeLocalCoreHOperators]") {
  const auto rows = RoundedSplitRing();
  local::CoreOptions options;
  options.surface_first = false;
  local::Mesh mesh(rows, false, options);
  const local::Edge edge{SlotOf(mesh, 0), SlotOf(mesh, 1)};
  REQUIRE(mesh.ClosedRing(edge, 0));
  REQUIRE(mesh.Length(edge) > local::LongEdge);
  const auto before = Snapshot(mesh);
  local::IdSequence ids(4);
  const auto next = ids.Peek();
  local::PassStats stats;
  CHECK(mesh.Split(edge, 0, ids, 1, SIZE_MAX, stats) == local::Refusal::Validity);
  CHECK(stats.refused_exact == 1);
  CHECK(stats.refused_valid == 1);
  CHECK(stats.splits == 0);
  CHECK(ids.Peek() == next);
  CHECK(Snapshot(mesh) == before);
  CHECK_NOTHROW(mesh.Check());
}

TEST_CASE("Local split bounds every new frontier edge by the split edge", "[.][NativeLocalCoreH6]") {
  const std::array<Tensor, 5> tensors{{{4, 0, -1.75, .25, 0, .875}, {4, 0, 1.75, .25, 0, .875},
                                      {.25, 0, 0, .25, 0, .25}, {.25, 0, 0, .25, 0, .25},
                                      {.25, 0, 0, .25, 0, .25}}};
  for (const double radius : {1.5, .75}) {
    CAPTURE(radius);
    auto rows = BoundMesh({{0, 0, -1}, {0, 0, 1}, {radius, 0, 0}, {-.375, .75, 0}, {-.375, -.75, 0}},
                          {{0, 1, 2, 3}, {0, 1, 3, 4}, {0, 1, 4, 2}});
    for (auto& entry : rows)
      for (unsigned i = 0; i < 4; ++i) entry.second.sensor[i] = tensors[entry.second.cell.v[i].id];

    local::Mesh mesh(rows);
    REQUIRE(std::none_of(mesh.Points().begin(), mesh.Points().end(), [](const auto& p) { return p.unresolved; }));
    const local::Edge edge{SlotOf(mesh, 0), SlotOf(mesh, 1)};
    REQUIRE(mesh.ClosedRing(edge, 0));
    const auto old_length = mesh.Length(edge);
    REQUIRE(old_length > local::LongEdge);
    // The midpoint and rim endpoint norms are 2r and r/2, so their integrated length is 7r/5.
    const auto radial_length = 1.4 * radius;
    const auto before = Snapshot(mesh);
    local::IdSequence ids(4);
    const auto next = ids.Peek();
    local::PassStats stats;
    if (radius > 1) {
      REQUIRE(radial_length > old_length);
      CHECK(mesh.Split(edge, 0, ids, 1, SIZE_MAX, stats) == local::Refusal::Size);
      CHECK(stats.refused_size == 1);
      CHECK(stats.splits == 0);
      CHECK(ids.Peek() == next);
      CHECK(Snapshot(mesh) == before);
    } else {
      REQUIRE(mesh.Split(edge, 0, ids, 1, SIZE_MAX, stats) == local::Refusal::None);
      CHECK(stats.refused_size == 0);
      CHECK(stats.splits == 1);
      CHECK(ids.Peek() == next + 1);
      const auto midpoint = SlotOf(mesh, next);
      double longest = 0;
      size_t new_edges = 0;
      for (const auto& use : mesh.Edges()) {
        if (use.edge[0] != midpoint && use.edge[1] != midpoint) continue;
        longest = std::max(longest, mesh.Length(use.edge));
        ++new_edges;
      }
      CHECK(new_edges == 5);
      CHECK(longest == Approx(radial_length));
      CHECK(longest < old_length * (1 - 1e-12));
    }
    CHECK_NOTHROW(mesh.Check());
    std::cout << "FP1_FIXTURE h6_split radius=" << radius << " old_length=" << old_length
              << " radial_length=" << radial_length << " refused_size=" << stats.refused_size
              << " splits=" << stats.splits << '\n';
  }
}

TEST_CASE("Float local box pass passes strict measures and permutation signs", "[.][NativeLocalCoreHOperators]") {
  auto rows = RegularBox();
  local::Mesh mesh(rows);
  Id last_point = 0;
  for (const auto& point : mesh.Points()) last_point = std::max(last_point, point.node.id);
  local::IdSequence points(last_point), cells(rows.rbegin()->first);
  const auto stats = mesh.RunPass(points, SIZE_MAX, true);
  REQUIRE(stats.collapses > 0);
  CHECK_NOTHROW(mesh.Check());
  size_t signs = 0;
  for (const auto& row : mesh.Cells()) {
    if (!row.live) continue;
    auto order = row.vertices;
    std::sort(order.begin(), order.end());
    do {
      const auto p = [&](unsigned i) { return mesh.Points()[order[i]].node.p; };
      CHECK(local::EvenOrder(row.vertices, order) == (Orientation(p(0), p(1), p(2), p(3)) > 0));
      ++signs;
    } while (std::next_permutation(order.begin(), order.end()));
  }
  mesh.WriteBack(rows, cells);
  CHECK(signs == 24 * rows.size());
  std::vector<DonorCell> donors;
  for (const auto& entry : rows) donors.push_back({entry.second.cell, entry.second.sensor});
  FrozenField field(std::move(donors), {}, false, {}, {}, {}, true, rows.size(), true);
  for (auto it = rows.begin(); it != rows.end();) {
    std::vector<Cell> batch;
    for (; it != rows.end() && batch.size() < MaximumReplacementCells; ++it) batch.push_back(it->second.cell);
    const auto measured = MeasureMetricCells(batch, field);
    CHECK(measured.shape_cells == batch.size());
    CHECK(measured.q_bad_cells == 0);
    CHECK(measured.j_bad_cells == 0);
    CHECK(measured.maximum_length <= MaximumMetricEdge);
  }
  std::cout << "FP1_FIXTURE h_float_box cells=" << rows.size() << " signs=" << signs << " collapses=" << stats.collapses
            << '\n';
}

TEST_CASE("Float moved-tensor samples agree with the production cavity sampler", "[.][NativeLocalCoreHOperators]") {
  size_t separated = 0, samples = 0;
  double largest_relative_error = 0;
  for (const auto center : {Point{.125, .125, .125}, Point{.25, .125, .375}, Point{.375, .25, .125}}) {
    const auto donors = DonorStar(center);
    std::vector<Point> queries{center, {0, 0, 0}, {-.125, .25, .375}, {.25, .375, .625}};
    for (const auto& donor : donors) queries.push_back(Centroid(donor.cell));
    for (const auto point : queries) {
      CAPTURE(point.x, point.y, point.z);
      std::vector<long double> minima;
      for (const auto& donor : donors) {
        const auto weights = MetricAffineWeights(donor.cell, point);
        minima.push_back(*std::min_element(weights.begin(), weights.end()));
      }
      std::sort(minima.rbegin(), minima.rend());
      const auto expected = EvaluateCavitySample(donors, point);
      Tensor actual;
      size_t chosen = SIZE_MAX;
      REQUIRE(local::SampleMovedTensorFloat(donors, point, actual, &chosen));
      REQUIRE(chosen < donors.size());
      if (minima[0] - minima[1] > 1e-9L) {
        CHECK(donors[chosen].cell.id == expected.cell);
        ++separated;
      }
      const auto wanted = Components(expected.sensor), observed = Components(actual);
      double scale = 0;
      for (const auto component : wanted) scale = std::max(scale, std::abs(component));
      for (size_t i = 0; i < wanted.size(); ++i) {
        const auto error = std::abs(wanted[i] - observed[i]);
        CHECK(error <= 1e-12 * scale);
        largest_relative_error = std::max(largest_relative_error, error / scale);
      }
      ++samples;
    }
  }
  CHECK(separated > 0);
  CHECK(separated < samples);
  std::cout << "FP1_FIXTURE h_float_sampler samples=" << samples << " separated=" << separated
            << " max_relative_error=" << largest_relative_error << '\n';
}
