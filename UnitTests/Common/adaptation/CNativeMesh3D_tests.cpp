/*!
 * \file CNativeMesh3D_tests.cpp
 * \brief Predicate and metric-shape controls for the native tetrahedral kernel.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeMesh3D.hpp"
#include "../../../Common/include/adaptation/CNativeSupport3D.hpp"
#include "../../../Common/include/adaptation/CNativePointConstruction3D.hpp"
#include "../../../Common/include/adaptation/CNativeCellSampling3D.hpp"
#include "../../../Common/src/adaptation/CNativePredicates3D_internal.hpp"
#include <random>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
using namespace SU2Native3D;

namespace {
Tetrahedron Regular() {
  return {{{0,0,0},{1,0,0},{.5,std::sqrt(3.)/2,0},{.5,std::sqrt(3.)/6,std::sqrt(2./3)}}};
}
}
TEST_CASE("Native 3D orientation: exact signs and bounded fallback", "[NativeMesh3D]") {
  KernelStats stats;
  CHECK(Orientation({}, {1,0,0}, {0,1,0}, {0,0,1}, &stats)==1);
  CHECK(stats.filtered_orientations==1);
  const double epsilon = std::ldexp(1.,-52);
  CHECK(Orientation({}, {1,1+epsilon,0}, {1-epsilon,1,0}, {0,0,1}, &stats)>0);
  CHECK(stats.exact_orientations==1);
  CHECK(Orientation({}, {1-epsilon,1,0}, {1,1+epsilon,0}, {0,0,1})<0);
  CHECK(Orientation({}, {1,0,0}, {0,1,0}, {1,1,0})==0);
  const auto tiny = std::numeric_limits<double>::denorm_min();
  CHECK(Orientation({}, {tiny,0,0}, {0,tiny,0}, {0,0,tiny})>0);
  const auto huge = std::numeric_limits<double>::max();
  CHECK(Orientation({-huge,0,0}, {huge,0,0}, {0,huge,0}, {0,0,huge})>0);
  CHECK_THROWS_AS(Orientation({}, {1,0,0}, {0,1,0},
                             {0,0,std::numeric_limits<double>::infinity()}),std::invalid_argument);
}
TEST_CASE("Native 3D metric measures: regular shape and scaling", "[NativeMesh3D]") {
  for (double scale : {1e-100,1.,1e100}) {
    auto tet = Regular();
    for (auto& p : tet) { p.x = scale*(p.x+1000);p.y = scale*(p.y+1000);p.z = scale*(p.z+1000); }
    const double m = 1/(scale*scale);
    const auto result = Measure(tet,{m,0,0,m,0,m});
    CHECK(result.mean_ratio==Approx(1).epsilon(1e-10));
    CHECK(result.minimum_scaled_jacobian==Approx(1).epsilon(1e-10));
    CHECK(result.rms_edge==Approx(1).epsilon(1e-10));
    CHECK(result.maximum_edge==Approx(1).epsilon(1e-10));
  }
}
TEST_CASE("Native 3D metric measures: aligned and rotated strong anisotropy", "[NativeMesh3D]") {
  const double angle = .37, c = std::cos(angle), s = std::sin(angle), aspect = 1e4;
  for (bool rotated : {false,true}) {
    auto tet = Regular();
    for (auto& p : tet) {
      p.z /= aspect;
      if (rotated) { const auto x = p.x;p.x = c*x+s*p.z;p.z = -s*x+c*p.z; }
    }
    const double a = aspect*aspect;
    const Tensor metric = rotated ? Tensor{c*c+a*s*s,0,(a-1)*s*c,1,0,s*s+a*c*c} :
                                    Tensor{1,0,0,1,0,a};
    const auto result = Measure(tet,metric);
    CHECK(result.mean_ratio==Approx(1).epsilon(1e-8));
    CHECK(result.minimum_scaled_jacobian==Approx(1).epsilon(1e-8));
    CHECK(result.rms_edge==Approx(1).epsilon(1e-8));
  }
  auto tet = Regular();
  for (auto& p : tet) { p.x *= 1e8;p.y *= 1e8; }
  CHECK(Measure(tet,{1e-16,0,0,1e-16,0,1}).mean_ratio==Approx(1).epsilon(1e-10));
}
TEST_CASE("Native 3D slivers: acceptable edge sizes do not imply acceptable shape", "[NativeMesh3D]") {
  const Tetrahedron tet{{{.5,0,0},{-.5,0,0},{0,-.5,1e-8},{0,.5,1e-8}}};
  const auto result = Measure(tet,{});
  CHECK(result.maximum_edge<1.8);
  CHECK(result.mean_ratio>0);
  CHECK(result.mean_ratio<MinimumMeanRatio);
  CHECK(result.minimum_scaled_jacobian<MinimumScaledJacobian);
}
TEST_CASE("Native 3D metric measures: permutation signs and degeneracy", "[NativeMesh3D]") {
  auto tet = Regular();
  std::swap(tet[0],tet[1]);
  const auto result = Measure(tet,{});
  CHECK(result.mean_ratio==Approx(-1));
  CHECK(result.minimum_scaled_jacobian==Approx(-1));
  tet[3]=tet[2];
  CHECK(Measure(tet,{}).mean_ratio==0);
  CHECK(Measure(tet,{}).minimum_scaled_jacobian==0);
  CHECK(Measure(Tetrahedron{},{}).maximum_edge==0);
}
TEST_CASE("Native 3D metric admission: singular indefinite and nonfinite tensors", "[NativeMesh3D]") {
  const auto tet = Regular();
  CHECK_THROWS_AS(Measure(tet,{1,1,0,1,0,1}),std::invalid_argument);
  CHECK_THROWS_AS(Measure(tet,{1,0,0,1,0,-1}),std::invalid_argument);
  CHECK_THROWS_AS(Measure(tet,{1,0,0,1,0,std::numeric_limits<double>::quiet_NaN()}),std::invalid_argument);
}

TEST_CASE("Native 3D exact segment clipping covers cells and conformal ties", "[NativeMesh3D]") {
  Cell up{0, {{{0, {0, 0, 0}}, {1, {1, 0, 0}}, {2, {0, 1, 0}}, {3, {0, 0, 1}}}}};
  Cell down{1, {{{0, {0, 0, 0}}, {2, {0, 1, 0}}, {1, {1, 0, 0}}, {4, {0, 0, -1}}}}};
  const Point a{.125, .125, -.25}, b{.125, .125, .25};
  const auto pieces = TraceSegment({&up, &down}, a, b);
  REQUIRE(pieces.size() == 2);
  CHECK(pieces[0].donor == 1);
  CHECK(pieces[1].donor == 0);
  CHECK(pieces[0].begin == 0);
  CHECK(pieces[0].end == .5L);
  CHECK(pieces[0].width == .5L);
  CHECK(pieces[1].end == 1);
  CHECK(pieces[1].begin_weights[3] == 0);
  CHECK(pieces[1].end_weights[3] == .25L);
  Cell separated{2, {{{8,{1,1,0}},{2,{0,1,0}},{1,{1,0,0}},{9,{1,1,1}}}}};
  KernelStats pruned;
  const auto filtered=TraceSegment({&up,&down,&separated},a,b,&pruned);
  REQUIRE(filtered.size()==pieces.size());CHECK(pruned.segment_separated==1);
  for(size_t i=0;i<pieces.size();++i) {
    CHECK(filtered[i].donor==pieces[i].donor);CHECK(filtered[i].begin==pieces[i].begin);
    CHECK(filtered[i].end==pieces[i].end);CHECK(filtered[i].width==pieces[i].width);
    CHECK(filtered[i].begin_weights==pieces[i].begin_weights);CHECK(filtered[i].end_weights==pieces[i].end_weights);
  }
  KernelStats touching;
  CHECK(TraceSegment({&up,&down,&separated},{0,0,0},{1,0,0},&touching).size()==1);
  CHECK(touching.segment_separated==0);
  std::swap(separated.v[0],separated.v[1]);
  CHECK_THROWS_AS(TraceSegment({&up,&down,&separated},a,b),std::invalid_argument);
  const auto face = TraceSegment({&up, &down}, {.125, .125, 0}, {.5, .125, 0});
  REQUIRE(face.size() == 1);
  CHECK(face[0].donor == 0);
  CHECK(face[0].width == 1);
  const auto reverse = TraceSegment({&up, &down}, b, a);
  REQUIRE(reverse.size() == 2);
  CHECK(reverse[0].donor == 0);
  CHECK(reverse[0].begin_weights == pieces[1].end_weights);
  const auto edge = TraceSegment({&up, &down}, {0, 0, 0}, {1, 0, 0});
  REQUIRE(edge.size() == 1);
  CHECK(edge[0].begin_weights[0] == 1);
  CHECK(edge[0].end_weights[1] == 1);
  CHECK_THROWS_AS(TraceSegment({&up}, a, b), std::runtime_error);
  CHECK_THROWS_AS(TraceSegment({&up, &up}, {.1, .1, .1}, {.2, .1, .1}), std::runtime_error);
  Cell unrelated = down;
  for (auto& n : unrelated.v) n.id += 10;
  CHECK_THROWS_AS(TraceSegment({&up, &unrelated}, {.125, .125, 0}, {.5, .125, 0}), std::runtime_error);
  CHECK_THROWS_AS(TraceSegment({}, a, b), std::runtime_error);
  CHECK_THROWS_AS(TraceSegment({nullptr}, a, b), std::invalid_argument);
  CHECK_THROWS_AS(TraceSegment({&up}, b, b), std::invalid_argument);
  CHECK_THROWS_AS(TraceSegment(std::vector<const Cell*>(MaximumSegmentDonors + 1, &up), a, b), std::invalid_argument);
  std::swap(up.v[0], up.v[1]);
  CHECK_THROWS_AS(TraceSegment({&up}, b, {.2, .1, .1}), std::invalid_argument);
}
TEST_CASE("Native exact segment vertices reuse unit weights without snapping nearby points or hiding invalid donors", "[NativeMesh3D]") {
  for(int power:{-20,0,300}) {
    const double scale=std::ldexp(1.,power);
    Cell cell{0, {{{0,{0,0,0}},{1,{scale,0,0}},{2,{0,scale,0}},{3,{0,0,scale}}}}};
    for(size_t a=0;a<4;++a)for(size_t b=0;b<4;++b)if(a!=b) {
      const auto pieces=TraceSegment({&cell},cell.v[a].p,cell.v[b].p);
      REQUIRE(pieces.size()==1);CHECK(pieces[0].begin==0);CHECK(pieces[0].end==1);CHECK(pieces[0].width==1);
      for(size_t i=0;i<4;++i){CHECK(pieces[0].begin_weights[i]==(i==a?1.L:0.L));CHECK(pieces[0].end_weights[i]==(i==b?1.L:0.L));}
    }
    const auto near=std::nextafter(scale,0.);
    const auto pieces=TraceSegment({&cell},{0,0,0},{near,0,0});REQUIRE(pieces.size()==1);
    CHECK(pieces[0].end_weights[1]==static_cast<long double>(near)/scale);CHECK(pieces[0].end_weights[0]>0);
    auto invalid=cell;std::swap(invalid.v[0],invalid.v[1]);
    CHECK_THROWS_AS(TraceSegment({&cell,&invalid},cell.v[0].p,cell.v[1].p),std::invalid_argument);
  }
}
TEST_CASE("Native 3D chord norm preserves small positive cancellation and large components", "[NativeMesh3D]") {
  KernelStats stats;
  CHECK(MetricNorm({}, {1, 2, 2}, {}, &stats) == 3);
  CHECK(stats.filtered_norms == 1);
  const auto next = std::nextafter(1., 2.);
  CHECK(double(MetricNorm({}, {1, -1, 0}, {1, 1, 0, next, 0, 1}, &stats)) ==
        Approx(std::sqrt(next - 1)).epsilon(1e-14));
  CHECK(stats.exact_norms == 1);
  const Tensor large{1e308, 9e307, 0, 1e308, 0, 1};
  CHECK(double(MetricNorm({}, {1, -1, 0}, large)) ==
        Approx(std::sqrt(2e308L - 2 * static_cast<long double>(large.xy))).epsilon(1e-14));
  CHECK(MetricNorm({}, {}, {}) == 0);
  CHECK_THROWS_AS(MetricNorm({}, {1, 0, 0}, {1, 1, 0, 1, 0, 1}), std::invalid_argument);
  CHECK_THROWS_AS(MetricNorm({}, {std::numeric_limits<double>::infinity(), 0, 0}, {}), std::invalid_argument);
}
TEST_CASE("Native 3D exact chord magnitude never substitutes an underflow sign sentinel", "[NativeMesh3D]") {
  const auto tiny = std::numeric_limits<double>::denorm_min();
  const Tensor metric{tiny, 0, 0, 1, 0, 1};
  if (std::numeric_limits<long double>::min_exponent - std::numeric_limits<long double>::digits > -3222)
    CHECK_THROWS_AS(MetricNorm({}, {tiny, 0, 0}, metric), std::runtime_error);
  else {
    const auto expected = static_cast<long double>(tiny) * std::sqrt(static_cast<long double>(tiny));
    CHECK(double(MetricNorm({}, {tiny, 0, 0}, metric) / expected) == Approx(1).epsilon(1e-14));
  }
}

TEST_CASE("Native 3D donor footprint uses exact closed segments including point-only contact", "[NativeMesh3D]") {
  const Tetrahedron t{{{0,0,0},{1,0,0},{0,1,0},{0,0,1}}};
  CHECK(IntersectsSegment(t, {-.5,.5,0}, {.5,-.5,0}));
  CHECK(IntersectsSegment(t, {-1,0,0}, {}));
  CHECK(IntersectsSegment(t, {}, {}));
  CHECK(IntersectsSegment(t, {.25,.25,.25}, {.25,.25,.25}));
  CHECK_FALSE(IntersectsSegment(t, {-1,.2,.2}, {.2,-1,.2}));
  CHECK_FALSE(IntersectsSegment(t, {0,0,-std::numeric_limits<double>::denorm_min()}, {0,0,-1}));
  CHECK_FALSE(IntersectsSegment(t, {1,1,1}, {1,1,1}));
  CHECK(IntersectsSegment(t, {-1e100,.25,.25}, {1e100,.25,.25}));
  CHECK_FALSE(IntersectsSegment(t, {-1e100,1,1}, {1e100,1,1}));
  auto negative=t;std::swap(negative[0],negative[1]);
  CHECK_THROWS_AS(IntersectsSegment(negative, {}, {1,0,0}), std::invalid_argument);
  CHECK_THROWS_AS(IntersectsSegment(t, {NAN,0,0}, {}), std::invalid_argument);
}

TEST_CASE("Native closed-query filters settle containment without weakening crossing or tiny signs", "[NativeMesh3D]") {
  const Tetrahedron t{{{0,0,0},{1,0,0},{0,1,0},{0,0,1}}};KernelStats stats;
  CHECK(IntersectsSegment(t,{.2,.2,.2},{.2,.2,.2},&stats));
  CHECK(stats.exact_orientations==0);
  CHECK(IntersectsSegment(t,{.2,.2,-1},{.2,.2,1}));
  CHECK(IntersectsSegment(t,{.2,.2,1},{.2,.2,-1}));
  const double tiny=std::numeric_limits<double>::denorm_min();
  const Tetrahedron small{{{0,0,0},{tiny,0,0},{0,tiny,0},{0,0,tiny}}};
  CHECK(IntersectsSegment(small,{},{}));
  CHECK_FALSE(IntersectsSegment(small,{-tiny,0,0},{-tiny,0,0}));
  CHECK_FALSE(IntersectsSegment(t,{-1,.2,.2},{.2,-1,.2}));
}

TEST_CASE("Metric segment footprints keep every positive interval but exclude isolated contact", "[NativeMetricFootprint3D]") {
  for(double s:{std::numeric_limits<double>::denorm_min(),1e-100,1.,1e100}) {
    const Tetrahedron t{{{0,0,0},{s,0,0},{0,s,0},{0,0,s}}};
    CHECK(IntersectsSegment(t,{}, {-s,0,0}));CHECK_FALSE(IntersectsSegmentPositiveLength(t,{}, {-s,0,0}));
    CHECK_FALSE(IntersectsSegmentPositiveLength(t,{-s,0,0},{}));
    CHECK(IntersectsSegmentPositiveLength(t,{}, {s,0,0}));
    CHECK(IntersectsSegmentPositiveLength(t,{s,0,0},{}));
    CHECK_FALSE(IntersectsSegmentPositiveLength(t,{},{}));
  }
  const Tetrahedron t{{{0,0,0},{1,0,0},{0,1,0},{0,0,1}}};
  CHECK(IntersectsSegmentPositiveLength(t,{.2,.2,-1},{.2,.2,1}));
  CHECK_FALSE(IntersectsSegmentPositiveLength(t,{-1,.2,.2},{.2,-1,.2}));
  CHECK(IntersectsSegmentPositiveLength(t,{std::nextafter(1.,-INFINITY),0,0},{2,0,0}));
  CHECK_FALSE(IntersectsSegmentPositiveLength(t,{std::nextafter(1.,INFINITY),0,0},{2,0,0}));
  CHECK(IntersectsSegment(t,{1,0,0},{2,0,0}));CHECK_FALSE(IntersectsSegmentPositiveLength(t,{1,0,0},{2,0,0}));
  CHECK_THROWS(IntersectsSegmentPositiveLength(t,{NAN,0,0},{}));
  auto negative=t;std::swap(negative[0],negative[1]);CHECK_THROWS(IntersectsSegmentPositiveLength(negative,{}, {1,0,0}));
}

TEST_CASE("Native3D exact contact admits only the common mesh simplex", "[NativeContact3D]") {
  for(double scale:{1.,1e-100,1e100}) {
    Cell a{0,{{{0,{0,0,0}},{1,{scale,0,0}},{2,{0,scale,0}},{3,{0,0,scale}}}}};
    Cell b{1,{{a.v[0],a.v[2],a.v[1],{4,{0,0,-scale}}}}};
    CHECK(ConformingDisjoint(a,b));CHECK(ConformingDisjoint(b,a));
    CHECK_FALSE(ConformingDisjoint(a,a));
    auto edge=b;edge.v={a.v[0],a.v[1],Node{4,{0,-scale,0}},Node{5,{0,0,-scale}}};
    CHECK(ConformingDisjoint(a,edge));
    auto unshared=b;unshared.v[1].id=100;CHECK_FALSE(ConformingDisjoint(a,unshared));
    unshared=b;for(auto& n:unshared.v)n.id+=100;CHECK_FALSE(ConformingDisjoint(a,unshared));
    Cell vertex{2,{{a.v[1],{100,{2*scale,0,0}},{101,{scale,scale,0}},{102,{scale,0,scale}}}}};
    CHECK(ConformingDisjoint(a,vertex));
    vertex.v[0].id=103;CHECK_FALSE(ConformingDisjoint(a,vertex));
    for(auto& n:vertex.v)n.p.x=std::nextafter(n.p.x,INFINITY);
    CHECK(ConformingDisjoint(a,vertex));
    auto overlap=a;for(auto& n:overlap.v){n.id+=200;n.p.x+=scale/4;n.p.y+=scale/4;n.p.z+=scale/4;}
    CHECK_FALSE(ConformingDisjoint(a,overlap));
  }
}


TEST_CASE("DC-17 shared-simplex contact proofs agree with the exact contact test", "[NativeContact3D]") {
  // Grid points in [-2,2]^3 (exact coplanar, collinear and tangent configurations are frequent); b shares k = 0..3
  // vertices (ids and coordinates) with a, its other vertices are fresh ids (possibly at a's coordinates).
  std::mt19937_64 rng(1729);std::uniform_int_distribution<int> c(-2,2),pick(0,3),kk(0,3);
  auto random=[&]{return Point{double(c(rng)),double(c(rng)),double(c(rng))};};
  auto positive=[](Cell& t){const auto o=Orientation(t.v[0].p,t.v[1].p,t.v[2].p,t.v[3].p);if(o<0)std::swap(t.v[0],t.v[1]);return o!=0;};
  size_t cases=0,agree=0,proved=0,yes=0;const size_t proofs0=SharedSimplexProofs();
  for(int it=0;it<200000&&cases<40000;++it) {
    Cell a{0,{{{0,random()},{1,random()},{2,random()},{3,random()}}}};if(!positive(a))continue;
    const int k=kk(rng);Cell b{1,{{{10,random()},{11,random()},{12,random()},{13,random()}}}};
    std::array<bool,4> used{};for(int i=0;i<k;++i){int j=pick(rng);while(used[j])j=(j+1)%4;used[j]=true;b.v[i]=a.v[j];}
    if(!positive(b))continue;
    ++cases;const bool full=ConformingDisjointExactPath(a,b),fast=ConformingDisjoint(a,b);
    agree+=full==fast;yes+=full;
    if(full!=fast)INFO("disagreement at case "<<cases<<" k="<<k);
    REQUIRE(full==fast);
    proved+=fast;
  }
  const size_t shortcut=SharedSimplexProofs()-proofs0;
  CHECK(cases>=40000);CHECK(agree==cases);CHECK(yes>1000);CHECK(shortcut>yes/2);CHECK(shortcut<=proved);
}

TEST_CASE("Native3D supporting-plane contact preserves exact fallback and input checks", "[NativeContact3D]") {
  auto positive=[](Cell c) {
    if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[1],c.v[2]);
    return c;
  };
  for(double scale:{std::numeric_limits<double>::denorm_min(),std::ldexp(1.,-500),1.,std::ldexp(1.,500)}) {
    Cell a{0,{{{0,{0,0,0}},{1,{scale,0,0}},{2,{0,scale,0}},{3,{0,0,scale}}}}};
    auto b=positive(Cell{1,{{a.v[0],{4,{-scale,0,0}},{5,{0,-scale,0}},{6,{0,0,-scale}}}}});
    CHECK(ConformingDisjoint(a,b));CHECK(ConformingDisjoint(b,a));
    auto unshared=b;for(auto& n:unshared.v)n.id+=100;
    CHECK_FALSE(ConformingDisjoint(a,unshared));
  }
  Cell a{0,{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}};
  auto vertex=positive(Cell{1,{{a.v[1],{4,{1,-1,0}},{5,{1,0,-1}},{6,{2,0,0}}}}});
  // One extremal face has extra vertices; only the other tet's common vertex is needed.
  KernelStats stats;CHECK(ConformingDisjoint(a,vertex,&stats));CHECK(stats.exact_orientations==0);
  CHECK(ConformingDisjoint(vertex,a));
  auto edge=positive(Cell{1,{{a.v[0],a.v[1],{4,{0,-1,0}},{5,{0,0,-1}}}}});
  auto edge_a=a;
  for(auto* c:{&edge_a,&edge})for(auto& n:c->v){const auto p=n.p;n.p={-p.y-p.z,p.x,p.z};}
  stats={};CHECK(ConformingDisjoint(edge_a,edge,&stats));CHECK(stats.exact_orientations==0);
  auto face=positive(Cell{1,{{a.v[0],a.v[1],a.v[2],{4,{0,0,-1}}}}});
  stats={};CHECK(ConformingDisjoint(a,face,&stats));CHECK(stats.exact_orientations==0);
  auto partial=positive(Cell{1,{{a.v[0],{4,{1,.5,0}},{5,{.5,1,0}},{6,{0,0,-1}}}}});
  CHECK_FALSE(ConformingDisjoint(a,partial));CHECK_FALSE(ConformingDisjoint(partial,a));
  // Non-axis-aligned supporting faces retain the original exact SAT/clipping path.
  Cell rotated=a;for(auto& n:rotated.v){n.p.x+=n.p.z;n.p.y+=n.p.z;}
  auto reverse=positive(Cell{1,{{rotated.v[1],rotated.v[2],rotated.v[3],{4,{2,2,2}}}}});
  // DC-17: the shared-face proof settles it; the full exact SAT/clipping path keeps its own coverage
  CHECK(ConformingDisjoint(rotated,reverse));
  stats={};CHECK(ConformingDisjointExactPath(rotated,reverse,&stats));CHECK(stats.exact_orientations>0);
  reverse.v[0].id=100;CHECK_FALSE(ConformingDisjoint(rotated,reverse));
  auto touching=positive(Cell{1,{{a.v[1],{4,{2,0,0}},{5,{1,1,0}},{6,{1,0,1}}}}});
  for(auto& n:touching.v)n.id+=100;
  auto gap=touching;for(auto& n:gap.v)n.p.x=std::nextafter(n.p.x,INFINITY);
  CHECK(ConformingDisjoint(a,gap));
  auto overlap=touching;for(auto& n:overlap.v)n.p.x=std::nextafter(n.p.x,-INFINITY);
  CHECK_FALSE(ConformingDisjoint(a,overlap));
  auto mismatch=gap;mismatch.v[0].id=a.v[0].id;
  CHECK_THROWS_AS(ConformingDisjoint(a,mismatch),std::invalid_argument);
  auto negative=gap;std::swap(negative.v[0],negative.v[1]);
  CHECK_THROWS_AS(ConformingDisjoint(a,negative),std::invalid_argument);
  auto nonfinite=gap;nonfinite.v[0].p.x=INFINITY;
  CHECK_THROWS_AS(ConformingDisjoint(a,nonfinite),std::invalid_argument);
}


namespace {
SupportWall3D SupportSquare() {
  return {{{0,0,0},{1,0,0},{1,1,0},{0,1,0}},{{0,1,2},{0,2,3}},.1,1,.2};
}
}
TEST_CASE("Native support mixture covers face edge and vertex tubes with stable coplanar frames", "[NativeSupport3D]") {
  const auto wall=SupportSquare();SupportSampler3D sampler({wall},size_t(32)<<20,1000000);
  CHECK((sampler.Statistics().components==std::array<size_t,3>{{2,4,4}}));
  const auto points=sampler.Draw(4096,449);const double r=.2,volume=2*r+2*std::acos(-1.)*r*r+4*std::acos(-1.)*r*r*r/3;
  std::array<size_t,3> families{};
  for(const auto& q:points){CHECK(q.inverse_pdf==Approx(volume).epsilon(1e-11));++families[q.kind];}
  for(auto n:families)CHECK(n>0);
  CHECK(sampler.Density({10,10,10})==0);
  SupportSampler3D prefix({wall},size_t(32)<<20,1000000);const auto first=prefix.Draw(16,91),second=prefix.Draw(16,91,16),all=prefix.Draw(32,91);
  for(size_t i=0;i<32;++i) {
    const auto& a=i<16?first[i]:second[i-16];const auto& b=all[i];
    CHECK(a.point.x==b.point.x);CHECK(a.point.y==b.point.y);CHECK(a.point.z==b.point.z);CHECK(a.inverse_pdf==b.inverse_pdf);
  }
}
TEST_CASE("Native support PDFs add overlapping walls without omitting components", "[NativeSupport3D]") {
  SupportWall3D a{{{0,0,0},{1,0,0},{0,1,0}},{{0,1,2}},.01,1,.1},b=a;
  for(auto& p:b.vertices)p.z+=.05;
  SupportSampler3D sampler({a,b},size_t(32)<<20,1000000);
  CHECK(sampler.Density({.25,.25,.025})==Approx(2*sampler.Density({.25,.25,.12})).epsilon(1e-13));
  for(auto growth:{1.,1.+1e-12,1.2,2.}) {
    a.growth=growth;SupportSampler3D varying({a},size_t(32)<<20,1000000);
    for(const auto& q:varying.Draw(512,192)){CHECK(q.inverse_pdf>0);CHECK(std::isfinite(q.inverse_pdf));}
  }
}
TEST_CASE("Native support work storage and invalid geometry fail explicitly", "[NativeSupport3D]") {
  auto a=SupportSquare();
  CHECK_THROWS(SupportSampler3D({a},1,10000));CHECK_THROWS(SupportSampler3D({a},size_t(32)<<20,1));
  a.vertices[0].x=NAN;CHECK_THROWS(SupportSampler3D({a},size_t(32)<<20,10000));a=SupportSquare();
  a.triangles[0]={0,0,1};CHECK_THROWS(SupportSampler3D({a},size_t(32)<<20,10000));a=SupportSquare();
  a.triangles.push_back({0,2,1});CHECK_THROWS(SupportSampler3D({a},size_t(32)<<20,10000));a=SupportSquare();
  a.first_height=.3;CHECK_THROWS(SupportSampler3D({a},size_t(32)<<20,10000));a=SupportSquare();
  SupportSampler3D sampler({a},size_t(32)<<20,10000);CHECK_THROWS(sampler.Draw(1048577,1));CHECK_THROWS(sampler.Density({NAN,0,0}));
}


TEST_CASE("Native density and volume keep determinant magnitude under strong cancellation", "[NativeMesh3D][NativeDensity3D]") {
  for(double scale:{1e-200,1.,1e200}) {
    long double condition=0;
    CHECK(MetricDensity({scale,0,0,scale,0,scale},nullptr,&condition)==Approx(std::pow(scale,1.5)).epsilon(1e-13));
    CHECK(double(condition)==Approx(3.).epsilon(1e-13));
  }
  long double diagonalCondition=0;MetricDensity({1,0,0,2,0,4},nullptr,&diagonalCondition);
  CHECK(double(diagonalCondition)==Approx(7.).epsilon(1e-14));
  const double a=std::ldexp(1.,40);KernelStats stats;
  CHECK(MetricDensity({a+1,a,0,a+1,0,4},&stats)==Approx(2*std::sqrt(2*a+1)).epsilon(1e-14));
  CHECK(stats.exact_orientations>0);
  const Tetrahedron tet{{{0,0,0},{1,1,0},{1,1+std::ldexp(1.,-40),0},{0,0,1}}};
  CHECK(TetrahedronVolume(tet)==Approx(std::ldexp(1.,-40)/6).epsilon(1e-14));
  CHECK_THROWS(MetricDensity({-1,0,0,1,0,1}));CHECK_THROWS(MetricDensity({NAN,0,0,1,0,1}));
  auto bad=tet;std::swap(bad[0],bad[1]);CHECK_THROWS(TetrahedronVolume(bad));
}

TEST_CASE("Candidate importance stream is ownership independent with complete positive coverage", "[NativeCellSampling3D]") {
  World world;
  auto cells=[&](unsigned shift) {
    std::vector<WeightedCell3D> rows;
    for(unsigned i=0;i<32;++i)if((i+shift)%world.size==unsigned(world.rank)) {
      WeightedCell3D c;c.points={{{4.*i,0,0},{4.*i+1,0,0},{4.*i,1,0},{4.*i,0,1}}};c.importance=1+.5*i;c.id=i;rows.push_back(c);
    }return rows;
  };
  CellSampler3D first(world,cells(0),size_t(32)<<20),second(world,cells(1),size_t(32)<<20);
  const auto a=first.Draw(2048,7349),b=second.Draw(2048,7349);
  REQUIRE(a.size()==b.size());CHECK(first.CentroidEstimate()==Approx((32+31*32*.25)/6.).epsilon(1e-14));
  for(size_t i=0;i<a.size();++i) {
    CHECK(a[i].row==b[i].row);CHECK(a[i].point.x==b[i].point.x);CHECK(a[i].point.y==b[i].point.y);CHECK(a[i].point.z==b[i].point.z);
    CHECK(a[i].inverse_pdf==b[i].inverse_pdf);CHECK(a[i].cell==b[i].cell);
    CHECK(a[i].importance*a[i].inverse_pdf==Approx(double(first.CentroidEstimate())).epsilon(1e-12));
  }
  if(std::getenv("SU2_NATIVE_CELL_SAMPLER_TRACE")) {
    std::ofstream out("cell_sampler_rank"+std::to_string(world.rank)+".csv");out<<std::setprecision(17)<<"row,x,y,z,inverse_pdf,cell,importance,volume\n";
    for(const auto& p:a)out<<p.row<<','<<p.point.x<<','<<p.point.y<<','<<p.point.z<<','<<p.inverse_pdf<<','<<p.cell<<','<<p.importance<<','<<p.volume<<'\n';
    REQUIRE(bool(out));
  }
}

TEST_CASE("Candidate importance rejects invalid metadata and collective resource/control faults", "[NativeCellSampling3D]") {
  World world;WeightedCell3D cell;cell.points={{{0,0,0},{1,0,0},{0,1,0},{0,0,1}}};cell.importance=1;
  auto one=world.rank==0?std::vector<WeightedCell3D>{cell}:std::vector<WeightedCell3D>{};
  CellSampler3D sampler(world,one,size_t(32)<<20);
  CHECK_THROWS(sampler.Draw(world.rank==0?0:32,1));
  CHECK_THROWS_AS(CellSampler3D(world,one,4096),CellSamplingLimit3D);
  auto tiny=one;if(world.rank==0){auto extra=cell;for(auto& p:extra.points)p.x+=2;extra.importance=1e-30;tiny.push_back(extra);}
  CHECK_THROWS_AS(CellSampler3D(world,tiny,size_t(32)<<20),CellSamplingLimit3D);
  auto invalid=one;if(world.rank==0)invalid.front().importance=0;
  CHECK_THROWS(CellSampler3D(world,invalid,size_t(32)<<20));
  if(world.rank==0)one.push_back(cell);CHECK_THROWS(CellSampler3D(world,one,size_t(32)<<20));
  CHECK_THROWS(CellSampler3D(world,{},size_t(32)<<20));
}

TEST_CASE("Exact binary64 segment freedom is conservative across binades and subnormals", "[NativeSegmentFreedom3D]") {
  Point mid{9,8,7};const auto tiny=std::numeric_limits<double>::denorm_min();
  const double x=.5,y=std::nextafter(x,INFINITY),z=std::nextafter(y,INFINITY);
  CHECK(Binary64SegmentFreedom({x,x,x},{y,x,x},mid)==SegmentFreedom::None);CHECK(mid.x==9);
  CHECK(Binary64SegmentFreedom({x,x,x},{z,x,x},mid)==SegmentFreedom::MidpointOnly);CHECK(mid.x==y);
  CHECK(Binary64SegmentFreedom({0,0,0},{2*tiny,0,0},mid)==SegmentFreedom::MidpointOnly);CHECK(mid.x==tiny);
  CHECK(Binary64SegmentFreedom({-tiny,0,0},{tiny,0,0},mid)==SegmentFreedom::MidpointOnly);CHECK(mid.x==0);
  CHECK(Binary64SegmentFreedom({0,0,0},{1,1,1},mid)==SegmentFreedom::Unresolved);
  const double h=std::ldexp(1.,-53);CHECK(Binary64SegmentFreedom({1-2*h,.5,0},{1+4*h,.5+2*h,0},mid)==SegmentFreedom::None);
  CHECK_THROWS(Binary64SegmentFreedom({0,0,0},{0,0,0},mid));CHECK_THROWS(Binary64SegmentFreedom({INFINITY,0,0},{1,0,0},mid));
}

TEST_CASE("Native centroid orientation uses the mathematical point even when it is not binary64", "[NativeMesh3D][NativePlanarUnion3D]") {
  const auto tiny=std::numeric_limits<double>::denorm_min();
  for(double h:{tiny,1.,1e100}) {
    const std::array<Point,3> triangle{{{0,0,0},{h,0,0},{0,h,0}}};
    CHECK(OrientationCentroidSign({0,0,0},{h,0,0},triangle,{0,0,1})>0);
    CHECK(OrientationCentroidSign({0,0,0},{0,h,0},triangle,{0,0,1})<0);
    CHECK(OrientationCentroidSign({h,0,0},{0,h,0},triangle,{0,0,1})>0);
  }
  const double x=.5,y=std::nextafter(x,1.);
  const std::array<Point,3> thin{{{x,x,0},{y,x,0},{x,y,0}}};
  CHECK(OrientationCentroidSign({x,x,0},{y,x,0},thin,{0,0,1})>0);
  CHECK(OrientationCentroidSign({y,x,0},{x,y,0},thin,{0,0,1})>0);
  const std::array<Point,3> on{{{0,-1,0},{1,0,0},{2,1,0}}};
  CHECK(OrientationCentroidSign({}, {1,0,0},on,{0,0,1})==0);
  auto invalid=on;invalid[1].x=INFINITY;CHECK_THROWS(OrientationCentroidSign({}, {1,0,0},invalid,{0,0,1}));
}
