/*!
 * \file CNativeField3D_tests.cpp
 * \brief Original P1 sensors, conservative ADT queries and actual-point composition controls.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeField3D.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace SU2Native3D;
namespace {
Tensor Sensor(Point p) { return {2 + p.x, .1, 0, 3 + p.y, 0, 4 + p.z}; }
DonorCell Donor(Id id, std::array<Node, 4> nodes) {
  if (Orientation(nodes[0].p, nodes[1].p, nodes[2].p, nodes[3].p) < 0) std::swap(nodes[0], nodes[1]);
  DonorCell donor;
  donor.cell = {id, nodes};
  for (size_t i = 0; i < 4; ++i) donor.sensor[i] = Sensor(nodes[i].p);
  return donor;
}
DonorCell Single() { return Donor(0, {{{0, {0, 0, 0}}, {1, {1, 0, 0}}, {2, {0, 1, 0}}, {3, {0, 0, 1}}}}); }
}  // namespace
TEST_CASE("Native 3D P1 weights are magnitude-controlled in rotated thin donors", "[NativeMesh3D]") {
  for (double aspect : {1., 1e4, 1e6, 1e8, 1e10}) {
    const double c = std::cos(.37), s = std::sin(.37);
    auto transform = [&](Point p) { return Point{c * p.x + s * p.z / aspect, p.y, -s * p.x + c * p.z / aspect}; };
    const Tetrahedron donor{transform({0, 0, 0}), transform({1, 0, 0}), transform({0, 1, 0}), transform({0, 0, 1})};
    std::array<long double, 4> weights;
    KernelStats stats;
    REQUIRE(Barycentric(donor, transform({.125, .25, .125}), weights, &stats));
    long double sum = 0;
    for (auto w : weights) {
      CHECK(w >= 0);
      sum += w;
    }
    CHECK(double(sum) == Approx(1).epsilon(1e-14));
    // Binary64 rounding of the rotated query changes its true weights at extreme aspect.
    CHECK(double(weights[2]) == Approx(.25).epsilon(1e-12));
    if (aspect >= 1e6) CHECK(stats.exact_orientations > 0);
  }
  std::array<long double, 4> unchanged{9, 8, 7, 6};
  const auto d = Single().cell;
  const Tetrahedron t{d.v[0].p, d.v[1].p, d.v[2].p, d.v[3].p};
  CHECK_FALSE(Barycentric(t, {1, 1, 1}, unchanged));
  CHECK(unchanged[0] == 9);
}
TEST_CASE("Native 3D original sensor cache recomposes thin geometric demand at every query", "[NativeMesh3D]") {
  auto donor = Single();
  size_t composed = 0;
  const auto combine = [&](Point p, Tensor sensor) {
    ++composed;
    if (p.z < .001) sensor.zz = std::max(sensor.zz, 1e8);
    return sensor;
  };
  FrozenField field({donor}, combine, true);
  donor.sensor[0].zz = 99;  // Original records are copied; evolving data cannot overwrite the frozen sensor.
  const auto wall = field.Query({.125, .125, .0005});
  CHECK(wall.sensor.zz == Approx(4.0005));
  CHECK(wall.target.zz == 1e8);
  CHECK(field.Query({.125, .125, .0005}).target.zz == 1e8);
  CHECK(field.Query({.125, .125, .25}).target.zz == Approx(4.25));
  CHECK(composed == 3);
  CHECK(field.Statistics().hits == 1);
  CHECK(field.Statistics().interpolations == 2);
  CHECK(field.Statistics().compositions == 3);
  CHECK(field.Statistics().index_bytes == 0);
  const auto& s = field.Statistics();
  CHECK(s.query_seconds >= s.cache_seconds + s.search_seconds + s.interpolation_seconds + s.composition_seconds);
  CHECK(s.build_seconds > 0);
  auto fine = Single();
  for (auto& m : fine.sensor) m.zz = 1e10;
  FrozenField finer({fine}, combine);
  CHECK(finer.Query({.125, .125, .0005}).target.zz == 1e10);
  CHECK(finer.Statistics().query_seconds == 0);
}
TEST_CASE("Native 3D original donor ties use original node keys independently of cell IDs and delivery",
          "[NativeMesh3D]") {
  const Node a{0, {0, 0, 0}}, b{1, {1, 0, 0}}, c{2, {0, 1, 0}}, top{3, {.25, .25, 1}}, bottom{4, {.25, .25, -1}};
  const auto up = Donor(99, {a, b, c, top}), down = Donor(0, {a, b, c, bottom});
  FrozenField forward({up, down}), reverse({down, up});
  const auto u = forward.Query({.25, .25, 0}), v = reverse.Query({.25, .25, 0});
  CHECK(u.donor == up.Key());
  CHECK(v.donor == u.donor);
  CHECK(v.weights == u.weights);
  CHECK(v.sensor.zz == u.sensor.zz);
  CHECK(u.sensor.xx == Approx(2.25));
}
TEST_CASE("Native 3D ADT avoids scanning spatially separated original donors", "[NativeMesh3D]") {
  std::vector<DonorCell> donors;
  for (Id i = 0; i < MaximumDonors; ++i) {
    auto d = Single();
    d.cell.id = i;
    for (size_t j = 0; j < 4; ++j) {
      d.cell.v[j].id = 4 * i + j;
      d.cell.v[j].p.x += 3 * i;
      d.sensor[j] = Sensor(d.cell.v[j].p);
    }
    donors.push_back(d);
  }
  std::reverse(donors.begin(), donors.end());
  FrozenField field(donors);
  CHECK(field.Query({3 * 123 + .125, .125, .125}).sensor.xx == Approx(2 + 3 * 123 + .125));
  CHECK(field.Statistics().box_candidates == 1);
  CHECK(field.Statistics().containment_tests == 1);
  CHECK(field.Statistics().maximum_candidates == 1);
  CHECK(field.Statistics().index_bytes > 0);
}
TEST_CASE("Native 3D sensor reuse is bounded and FIFO eviction does not change values", "[NativeMesh3D]") {
  FrozenField field({Single()});
  for (size_t i = 0; i <= MaximumSensorSamples; ++i) {
    const double x = double(i + 1) / 4096;
    CHECK(field.Query({x, .125, .125}).sensor.xx == 2 + x);
  }
  CHECK(field.Statistics().maximum_cache_entries == MaximumSensorSamples);
  CHECK(field.Statistics().evictions == 1);
  CHECK(field.Query({1. / 4096, .125, .125}).sensor.xx == 2 + 1. / 4096);
  CHECK(field.Statistics().evictions == 2);
  CHECK(field.Query({double(MaximumSensorSamples + 1) / 4096, .125, .125}).sensor.xx ==
        2 + double(MaximumSensorSamples + 1) / 4096);
  CHECK(field.Statistics().hits == 1);
}
TEST_CASE("Native 3D original sensor rejects invalid admission, extrapolation and composition", "[NativeMesh3D]") {
  auto donor = Single();
  CHECK_THROWS_AS(FrozenField({donor, donor}), std::invalid_argument);
  CHECK_THROWS_AS(FrozenField(std::vector<DonorCell>(MaximumDonors + 1, donor)), std::invalid_argument);
  donor.sensor[0] = {1, 0, 0, 1, 0, 0};
  CHECK_THROWS_AS(FrozenField({donor}), std::invalid_argument);
  donor = Single();
  std::swap(donor.cell.v[0], donor.cell.v[1]);
  CHECK_THROWS_AS(FrozenField({donor}), std::invalid_argument);
  FrozenField field({Single()});
  CHECK_THROWS_AS(field.Query({1, 1, 1}), std::runtime_error);
  CHECK_THROWS_AS(field.Query({std::numeric_limits<double>::infinity(), 0, 0}), std::invalid_argument);
  CHECK(field.Statistics().failures == 2);
  CHECK(field.Statistics().requests ==
        field.Statistics().hits + field.Statistics().misses + field.Statistics().invalid_queries);
  FrozenField bad({Single()}, [](Point, Tensor) { return Tensor{1, 0, 0, 1, 0, 0}; });
  CHECK_THROWS_AS(bad.Query({.125, .125, .125}), std::invalid_argument);
  CHECK_THROWS_AS(bad.Query({.125, .125, .125}), std::invalid_argument);
  CHECK(bad.Statistics().hits == 1);
  CHECK(bad.Statistics().failures == 2);
  CHECK(bad.Statistics().compositions == 2);
}

namespace {
std::vector<DonorCell> Slabs(std::array<double, 5> x, double peak, double coarse = 1) {
  const std::array<std::array<size_t, 4>, 6> tets{
      {{0, 1, 2, 6}, {0, 2, 3, 6}, {0, 3, 7, 6}, {0, 7, 4, 6}, {0, 4, 5, 6}, {0, 5, 1, 6}}};
  std::vector<DonorCell> result;
  for (size_t plane = 0; plane + 1 < x.size(); ++plane) {
    const std::array<Node, 8> cube{{{4 * plane, {x[plane], 0, 0}},
                                    {4 * (plane + 1), {x[plane + 1], 0, 0}},
                                    {4 * (plane + 1) + 1, {x[plane + 1], 1, 0}},
                                    {4 * plane + 1, {x[plane], 1, 0}},
                                    {4 * plane + 2, {x[plane], 0, 1}},
                                    {4 * (plane + 1) + 2, {x[plane + 1], 0, 1}},
                                    {4 * (plane + 1) + 3, {x[plane + 1], 1, 1}},
                                    {4 * plane + 3, {x[plane], 1, 1}}}};
    for (const auto& indices : tets) {
      auto d = Donor(result.size(), {cube[indices[0]], cube[indices[1]], cube[indices[2]], cube[indices[3]]});
      for (size_t i = 0; i < 4; ++i) d.sensor[i] = {d.cell.v[i].p.x == x[2] ? peak : coarse, 0, 0, 1, 0, 1};
      result.push_back(d);
    }
  }
  return result;
}
}  // namespace
TEST_CASE("Native 3D sensor edge integration resolves thin source demand without coarse spreading", "[NativeMesh3D]") {
  for (bool adjacent : {false, true}) {
    const double middle = adjacent ? std::nextafter(.3, 1.) : .3000005;
    const std::array<double, 5> x{0, .3, middle, adjacent ? std::nextafter(middle, 1.) : .300001, 1};
    const double peak = adjacent ? 4e32 : 4e12;
    auto donors = Slabs(x, peak);
    size_t composed = 0;
    FrozenField field(
        donors,
        [&](Point, Tensor sensor) {
          ++composed;
          sensor.xx += 10;
          return sensor;
        },
        true);
    const Point a{0, .25, .25}, b{1, .25, .25};
    const auto length = field.SensorEdgeLength(a, b);
    const auto height = std::sqrt(static_cast<long double>(peak));
    const auto width = static_cast<long double>(x[3]) - x[1];
    const auto expected = 1 - width + width * (2.L / 3) * (peak + height + 1) / (height + 1);
    CHECK(double(length) == Approx(double(expected)).epsilon(1e-12));
    CHECK(double(field.SensorEdgeLength(b, a)) == Approx(double(length)).epsilon(1e-14));
    CHECK(composed == 0);  // Sensor audit must not be mistaken for a composed-target acceptance test.
    const auto three_point =
        (std::sqrt(field.Query(a).sensor.xx) + 4 * std::sqrt(field.Query({.5, .25, .25}).sensor.xx) +
         std::sqrt(field.Query(b).sensor.xx)) /
        6;
    CHECK(three_point == 1);
    CHECK(length > 1.8L);
    std::reverse(donors.begin(), donors.end());
    FrozenField reverse(donors);
    CHECK(reverse.SensorEdgeLength(a, b) == length);
    const auto& s = field.Statistics();
    CHECK(s.edge_requests == 2);
    CHECK(s.edge_failures == 0);
    CHECK(s.maximum_edge_candidates <= 24);
    CHECK(s.maximum_edge_pieces <= 24);
    CHECK(s.maximum_edge_pieces >= 4);
    CHECK(s.edge_seconds >= s.edge_search_seconds + s.edge_trace_seconds + s.edge_integral_seconds);
    CHECK(s.edge_trace_seconds > 0);
    CHECK(reverse.Statistics().edge_seconds == 0);
  }
}
TEST_CASE("Native 3D sensor edge length integrates affine donors and rejects incomplete coverage", "[NativeMesh3D]") {
  auto d = Single();
  for (size_t i = 0; i < 4; ++i) d.sensor[i] = {1 + 3 * d.cell.v[i].p.x, 0, 0, 1, 0, 1};
  FrozenField field({d});
  CHECK(double(field.SensorEdgeLength({0, 0, 0}, {1, 0, 0})) == Approx(14. / 9).epsilon(1e-14));
  CHECK_THROWS_AS(field.SensorEdgeLength({0, 0, 0}, {2, 0, 0}), std::runtime_error);
  CHECK_THROWS_AS(field.SensorEdgeLength({0, 0, 0}, {0, 0, 0}), std::invalid_argument);
  CHECK_THROWS_AS(field.SensorEdgeLength({0, 0, 0}, {std::numeric_limits<double>::quiet_NaN(), 0, 0}),
                  std::invalid_argument);
  CHECK(field.Statistics().edge_requests == 4);
  CHECK(field.Statistics().edge_failures == 3);
  CHECK(field.Statistics().requests == 0);
  CHECK(field.Statistics().edge_pieces == 1);
  CHECK(field.Statistics().edge_direct_intervals == 1);
  CHECK(field.Statistics().edge_containment_tests == 4);
}

TEST_CASE("Native 3D sensor intervals retain nonzero width when rounded cuts coincide", "[NativeMesh3D]") {
  auto donors = Slabs({-1e100, 0, .5, 1, 1e100}, 1e100, 1e-100);
  FrozenField field(donors);
  const auto length = field.SensorEdgeLength({-1e100, .25, .25}, {1e100, .25, .25});
  const auto baseline = 2 * static_cast<long double>(1e100) * std::sqrt(static_cast<long double>(1e-100));
  const auto expected = baseline + (2.L / 3) * std::sqrt(static_cast<long double>(1e100));
  CHECK(double(length) == Approx(double(expected)).epsilon(1e-12));
  std::vector<const Cell*> cells;
  for (const auto& d : donors) cells.push_back(&d.cell);
  const auto pieces = TraceSegment(cells, {-1e100, .25, .25}, {1e100, .25, .25});
  size_t coincident = 0;
  for (const auto& piece : pieces)
    if (piece.begin == piece.end) {
      CHECK(piece.width > 0);
      ++coincident;
    }
  CHECK(coincident >= 2);
}
