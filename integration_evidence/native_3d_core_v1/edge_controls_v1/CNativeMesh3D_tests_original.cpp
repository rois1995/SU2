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
  return {{{0, 0, 0}, {1, 0, 0}, {.5, std::sqrt(3.) / 2, 0}, {.5, std::sqrt(3.) / 6, std::sqrt(2. / 3)}}};
}
}  // namespace
TEST_CASE("Native 3D orientation: exact signs and bounded fallback", "[NativeMesh3D]") {
  KernelStats stats;
  CHECK(Orientation({}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, &stats) == 1);
  CHECK(stats.filtered_orientations == 1);
  const double epsilon = std::ldexp(1., -52);
  CHECK(Orientation({}, {1, 1 + epsilon, 0}, {1 - epsilon, 1, 0}, {0, 0, 1}, &stats) > 0);
  CHECK(stats.exact_orientations == 1);
  CHECK(Orientation({}, {1 - epsilon, 1, 0}, {1, 1 + epsilon, 0}, {0, 0, 1}) < 0);
  CHECK(Orientation({}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}) == 0);
  const auto tiny = std::numeric_limits<double>::denorm_min();
  CHECK(Orientation({}, {tiny, 0, 0}, {0, tiny, 0}, {0, 0, tiny}) > 0);
  const auto huge = std::numeric_limits<double>::max();
  CHECK(Orientation({-huge, 0, 0}, {huge, 0, 0}, {0, huge, 0}, {0, 0, huge}) > 0);
  CHECK_THROWS_AS(Orientation({}, {1, 0, 0}, {0, 1, 0}, {0, 0, std::numeric_limits<double>::infinity()}),
                  std::invalid_argument);
}
TEST_CASE("Native 3D metric measures: regular shape and scaling", "[NativeMesh3D]") {
  for (double scale : {1e-100, 1., 1e100}) {
    auto tet = Regular();
    for (auto& p : tet) {
      p.x = scale * (p.x + 1000);
      p.y = scale * (p.y + 1000);
      p.z = scale * (p.z + 1000);
    }
    const double m = 1 / (scale * scale);
    const auto result = Measure(tet, {m, 0, 0, m, 0, m});
    CHECK(result.mean_ratio == Approx(1).epsilon(1e-10));
    CHECK(result.minimum_scaled_jacobian == Approx(1).epsilon(1e-10));
    CHECK(result.rms_edge == Approx(1).epsilon(1e-10));
    CHECK(result.maximum_edge == Approx(1).epsilon(1e-10));
  }
}
TEST_CASE("Native 3D metric measures: aligned and rotated strong anisotropy", "[NativeMesh3D]") {
  const double angle = .37, c = std::cos(angle), s = std::sin(angle), aspect = 1e4;
  for (bool rotated : {false, true}) {
    auto tet = Regular();
    for (auto& p : tet) {
      p.z /= aspect;
      if (rotated) {
        const auto x = p.x;
        p.x = c * x + s * p.z;
        p.z = -s * x + c * p.z;
      }
    }
    const double a = aspect * aspect;
    const Tensor metric =
        rotated ? Tensor{c * c + a * s * s, 0, (a - 1) * s * c, 1, 0, s * s + a * c * c} : Tensor{1, 0, 0, 1, 0, a};
    const auto result = Measure(tet, metric);
    CHECK(result.mean_ratio == Approx(1).epsilon(1e-8));
    CHECK(result.minimum_scaled_jacobian == Approx(1).epsilon(1e-8));
    CHECK(result.rms_edge == Approx(1).epsilon(1e-8));
  }
  auto tet = Regular();
  for (auto& p : tet) {
    p.x *= 1e8;
    p.y *= 1e8;
  }
  CHECK(Measure(tet, {1e-16, 0, 0, 1e-16, 0, 1}).mean_ratio == Approx(1).epsilon(1e-10));
}
TEST_CASE("Native 3D slivers: acceptable edge sizes do not imply acceptable shape", "[NativeMesh3D]") {
  const Tetrahedron tet{{{.5, 0, 0}, {-.5, 0, 0}, {0, -.5, 1e-8}, {0, .5, 1e-8}}};
  const auto result = Measure(tet, {});
  CHECK(result.maximum_edge < 1.8);
  CHECK(result.mean_ratio > 0);
  CHECK(result.mean_ratio < MinimumMeanRatio);
  CHECK(result.minimum_scaled_jacobian < MinimumScaledJacobian);
}
TEST_CASE("Native 3D metric measures: permutation signs and degeneracy", "[NativeMesh3D]") {
  auto tet = Regular();
  std::swap(tet[0], tet[1]);
  const auto result = Measure(tet, {});
  CHECK(result.mean_ratio == Approx(-1));
  CHECK(result.minimum_scaled_jacobian == Approx(-1));
  tet[3] = tet[2];
  CHECK(Measure(tet, {}).mean_ratio == 0);
  CHECK(Measure(tet, {}).minimum_scaled_jacobian == 0);
  CHECK(Measure(Tetrahedron{}, {}).maximum_edge == 0);
}
TEST_CASE("Native 3D metric admission: singular indefinite and nonfinite tensors", "[NativeMesh3D]") {
  const auto tet = Regular();
  CHECK_THROWS_AS(Measure(tet, {1, 1, 0, 1, 0, 1}), std::invalid_argument);
  CHECK_THROWS_AS(Measure(tet, {1, 0, 0, 1, 0, -1}), std::invalid_argument);
  CHECK_THROWS_AS(Measure(tet, {1, 0, 0, 1, 0, std::numeric_limits<double>::quiet_NaN()}), std::invalid_argument);
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
