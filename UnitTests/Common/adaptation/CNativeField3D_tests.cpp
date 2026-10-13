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

TEST_CASE("Curved projection requires explicit frozen metric coverage outside the original mesh", "[NativeCurvedCoverage3D]") {
  auto original = Single();
  for (size_t i = 0; i < 4; ++i) {
    const auto p = original.cell.v[i].p;
    const double size = 1 + p.x + p.y + p.z;
    original.sensor[i] = {size, 0, 0, size, 0, size};
  }
  FrozenField interior({original});
  const Point chord{.5, .5, 0}, projected{std::sqrt(.5), std::sqrt(.5), 0};
  CHECK(projected.x * projected.x + projected.y * projected.y == Approx(1).margin(1e-12));
  CHECK(interior.Query(chord).sensor.xx == 2);
  CHECK_THROWS_AS(interior.Query(projected), std::runtime_error);
  CHECK_THROWS_AS(interior.SensorEdgeLength(chord, projected), std::runtime_error);

  // Study only: three exterior tetrahedra fill the octant shell 1<=x+y+z<=2.
  // Freeze the boundary trace (2I) there; the original P1 field remains authoritative inside.
  const Node a{1, {1, 0, 0}}, b{2, {0, 1, 0}}, c{3, {0, 0, 1}};
  const Node outer_a{4, {2, 0, 0}}, outer_b{5, {0, 2, 0}}, outer_c{6, {0, 0, 2}};
  std::vector<DonorCell> admitted{original, Donor(1, {a, b, c, outer_c}),
                                Donor(2, {a, b, outer_b, outer_c}), Donor(3, {a, outer_a, outer_b, outer_c})};
  for (size_t i = 1; i < admitted.size(); ++i)
    for (auto& sensor : admitted[i].sensor) sensor = {2, 0, 0, 2, 0, 2};
  FrozenField extended(admitted);
  const auto expected = interior.Query({.25, .25, .25}).sensor;
  const auto retained = extended.Query({.25, .25, .25}).sensor;
  CHECK(retained.xx == expected.xx);
  CHECK(retained.yy == expected.yy);
  CHECK(retained.zz == expected.zz);
  CHECK(retained.xy == expected.xy);
  CHECK(retained.xz == expected.xz);
  CHECK(retained.yz == expected.yz);
  CHECK(extended.Query(projected).sensor.xx == 2);
  CHECK(extended.Query(projected).sensor.yy == 2);
  CHECK(extended.Query(projected).sensor.zz == 2);
  CHECK(double(extended.SensorEdgeLength(chord, projected)) == Approx(2 * (projected.x - .5)).epsilon(1e-12));
  CHECK_THROWS_AS(extended.Query({2.1, 0, 0}), std::runtime_error);
  admitted[1].sensor[0].xx = 99;
  CHECK(extended.Query(projected).sensor.xx == 2);
}

namespace {
std::vector<DonorCell> Layers() {
  auto original=Single();original.cell.id=20;
  for(size_t i=0;i<4;++i){original.cell.v[i].id+=100;original.sensor[i]={1+3*original.cell.v[i].p.x,0,0,1,0,1};}
  auto extension=Single();extension.cell.id=21;extension.extension_component=42;
  for(size_t i=0;i<4;++i){auto& p=extension.cell.v[i].p;p.x*=2;p.y*=2;p.z*=2;extension.sensor[i]={9,0,0,9,0,9};}
  return {extension,original}; // Extension keys are lower; roles, not delivery/ID order, must choose original P1.
}
}
TEST_CASE("Explicit frozen coverage keeps original P1 priority and consistent original exterior edge integrals",
          "[NativeLayeredField3D]") {
  auto input=Layers();std::vector<Id> scope{42};
  FrozenField field(input,{},false,scope);scope[0]=43; // Association and source are frozen, not live mutable options.
  const auto old=FrozenField({input[1]}).Query({.125,.125,.125});
  const auto sample=field.Query({.125,.125,.125});
  CHECK(sample.donor==old.donor);CHECK(sample.weights==old.weights);
  CHECK(sample.sensor.xx==old.sensor.xx);CHECK(sample.extension_component==UINT64_MAX);
  CHECK(field.Query({1,0,0}).sensor.xx==4);CHECK(field.Query({1.25,0,0}).sensor.xx==9);
  CHECK(field.Query({1.25,0,0}).extension_component==42);
  CHECK(double(field.SensorEdgeLength({0,0,0},{2,0,0}))==Approx(41./9).epsilon(1e-14));
  CHECK(double(field.SensorEdgeLength({2,0,0},{0,0,0}))==Approx(41./9).epsilon(1e-14));
  CHECK(double(field.SensorEdgeLength({0,0,0},{1,0,0}))==Approx(14./9).epsilon(1e-14));
  CHECK(field.Statistics().extended_queries==2);CHECK(field.Statistics().extended_edge_pieces>=2);
  input[0].sensor[0].xx=99;CHECK(field.Query({1.25,0,0}).sensor.xx==9);
  FrozenField disabled(Layers()),wrong(Layers(),{},false,{43});
  CHECK_THROWS(disabled.Query({1.25,0,0}));CHECK_THROWS(wrong.Query({1.25,0,0}));
  CHECK_THROWS(disabled.SensorEdgeLength({0,0,0},{2,0,0}));
  CHECK_THROWS(field.Query({2.01,0,0}));CHECK_THROWS(field.SensorEdgeLength({0,0,0},{2.01,0,0}));
  size_t calls=0;
  FrozenField composed(Layers(),[&](Point p,Tensor m){++calls;if(p.x>1)m.zz=std::max(m.zz,1e8);return m;},false,{42});
  CHECK(composed.Query({1.25,0,0}).target.zz==1e8);CHECK(composed.Query({1.25,0,0}).target.zz==1e8);
  CHECK(calls==2);CHECK(composed.Statistics().hits==1);
  CHECK(double(composed.SensorEdgeLength({0,0,0},{2,0,0}))==Approx(41./9).epsilon(1e-14));
  CHECK(calls==2);CHECK_THROWS(composed.TargetEdgeLength({0,0,0},{2,0,0}));
}
TEST_CASE("Layered exact coverage preserves thin original sensor demand and nonzero coincident-cut widths",
          "[NativeLayeredField3D]") {
  for(bool extreme:{false,true}) {
    auto original=extreme ? Slabs({-1e100,0,.5,1,1e100},1e100,1e-100) : Slabs({0,.3,.3000005,.300001,1},4e12);
    auto shell=Donor(1000,{{{1000,{-2e100,0,0}},{1001,{2e100,0,0}},{1002,{-2e100,4,0}},{1003,{-2e100,0,4}}}});
    shell.extension_component=42;for(auto& m:shell.sensor)m={1,0,0,1,0,1};
    FrozenField baseline(original);const Point a{extreme?-1e100:0,.25,.25},b{extreme?1e100:1,.25,.25};
    const auto expected=baseline.SensorEdgeLength(a,b);original.push_back(shell);
    FrozenField field(original,{},false,{42});
    CHECK(double(field.SensorEdgeLength(a,b))==Approx(double(expected)).epsilon(1e-12));
    CHECK(double(field.SensorEdgeLength(b,a))==Approx(double(expected)).epsilon(1e-12));
    CHECK(field.Statistics().extended_edge_pieces==0); // Coarse exterior tensors cannot override covered original demand.
    CHECK(field.Query({extreme?.5:.3000005,.25,.25}).sensor.xx>=1e12);
  }
}
TEST_CASE("Declared layered fields reject invalid priority records and ambiguous exterior coverage",
          "[NativeLayeredField3D]") {
  auto input=Layers();auto duplicate=input[0];duplicate.cell.id=22;
  for(auto& n:duplicate.cell.v)n.id+=1000;for(auto& m:duplicate.sensor)m.xx=16;
  input.push_back(duplicate);FrozenField field(input,{},false,{42});
  CHECK_THROWS(field.SensorEdgeLength({1.25,0,0},{1.5,0,0}));
  auto invalid=Layers();invalid[0].sensor[0].zz=0;
  CHECK_THROWS(FrozenField(invalid)); // Unassociated records still undergo admission validation.
  CHECK_THROWS(FrozenField(Layers(),{},false,{UINT64_MAX}));
  const auto c=Single().cell;
  CHECK_THROWS(TraceSegment({&c},{0,0,0},{1,0,0},nullptr,2));
  CHECK_THROWS(FrozenField(std::vector<DonorCell>(MaximumDonors+1,Layers()[0]),{},false,{42}));
}

TEST_CASE("Native composed edge integration rejects malformed bands", "[NativeComposite3D]") {
  const Point a{.125,.125,0},b{.125,.125,.5};
  const auto compose=[](Point,Tensor m) {return m;};
  for(const std::vector<double> cuts : {std::vector<double>{}, {0.,.5}, {0.,.7,.3,1.},
                                      {0.,.5,.5,1.}, {0.,std::numeric_limits<double>::quiet_NaN(),1.}}) {
    FrozenField field({Single()},compose,false,{},[cuts](Point,Point) {return cuts;});
    CHECK_THROWS(field.TargetEdgeLength(a,b));
    CHECK(field.Statistics().edge_failures==1);
  }
  FrozenField field({Single()},compose,false,{},[](Point,Point){return std::vector<double>{0.,.001,.01,.5,1.};});
  CHECK(double(field.TargetEdgeLength(a,b))==Approx(double(field.SensorEdgeLength(a,b))).epsilon(1e-10));
}

TEST_CASE("Native3D split placement balances full target and preserves constant-field midpoint", "[NativeBalancedSplit3D]") {
  auto donor=Single();donor.sensor.fill({1,0,0,1,0,1});
  const Point a{0,0,0},b{0,0,1};
  FrozenField constant({donor});CHECK(constant.BalancedSplitPoint(a,b).z==.5);
  FrozenField graded({donor},[](Point p,Tensor sensor){sensor.zz=std::pow(1+9*p.z,2);return sensor;},false,{},
      [](Point,Point){return std::vector<double>{0,1};});
  const auto point=graded.BalancedSplitPoint(a,b);
  CHECK(point.x==0);CHECK(point.y==0);
  CHECK(point.z==Approx((-1+std::sqrt(50.5))/9).epsilon(1e-8));
  CHECK(double(graded.TargetEdgeLength(a,point))==Approx(double(graded.TargetEdgeLength(point,b))).epsilon(3e-8));
  CHECK(graded.Query(point).sensor.zz==1);
  FrozenField missing({donor},[](Point,Tensor sensor){return sensor;});CHECK_THROWS(missing.BalancedSplitPoint(a,b));
}
