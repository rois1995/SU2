/*!
 * \file CNativeMesh3D_tests.cpp
 * \brief Predicate and metric-shape controls for the native tetrahedral kernel.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeMesh3D.hpp"
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
