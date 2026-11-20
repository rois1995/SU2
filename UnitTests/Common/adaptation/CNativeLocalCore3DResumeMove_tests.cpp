/*!
 * \file CNativeLocalCore3DResumeMove_tests.cpp
 * \brief Moved tensor selection when an exactly positive donor has unusable affine weights.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetricTransport3D.hpp"

#include <utility>

namespace {
using namespace SU2Native3D;

std::vector<char> TensorBytes(Tensor tensor) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  stream(tensor);
  return bytes;
}
}  // namespace

TEST_CASE("Local tetrahedral moves skip exactly positive donors with unusable weights",
          "[.][NativeLocalCore3DResumeMove]") {
  constexpr double epsilon = 0x1p-52;
  const Node a{0, {0, 0, 0}}, b{1, {1, 0, 0}}, y{2, {0, 1, 0}}, below{3, {0, 0, -1}}, above{6, {0, 0, 1}};
  const DonorCell normal{{10, {a, b, below, y}},
                         {{{2, .125, 0, 3, -.125, 4}, {6, -.125, .25, 7, .125, 8},
                           {10, .25, -.25, 11, -.25, 12}, {14, .25, .5, 15, .25, 16}}}};
  DonorCell flat{{11, {a, b, {4, {0, 1, 1 + epsilon}}, {5, {0, 1 - epsilon, 1}}}},
                 {normal.sensor[0], normal.sensor[1], Tensor{}, Tensor{}}};
  auto& v = flat.cell.v;
  if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) < 0) std::swap(v[2], v[3]);
  REQUIRE(Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0);

  const Point sample{.25, .25, -.25};
  REQUIRE_THROWS_MATCHES(MetricAffineWeights(flat.cell, sample), std::invalid_argument,
                         Catch::Matchers::Message("Degenerate metric donor."));
  const Tensor sentinel{99, -0.0, .25, 100, -.5, 101};
  Tensor observed = sentinel;

  SECTION("The remaining donor supplies its interior P1 value") {
    const Tensor expected{8, .125, .125, 9, 0, 10};
    REQUIRE(SU2NativeLocal3D::SampleMovedTensor({flat, normal}, sample, observed));
    CHECK(TensorBytes(observed) == TensorBytes(expected));
  }

  SECTION("No usable donor leaves the output untouched") {
    CHECK_FALSE(SU2NativeLocal3D::SampleMovedTensor({flat}, sample, observed));
    CHECK(TensorBytes(observed) == TensorBytes(sentinel));
  }

  SECTION("All usable donors reproduce the existing cavity sample bit for bit") {
    const DonorCell opposite{{12, {a, b, y, above}},
                             {normal.sensor[0], normal.sensor[1], normal.sensor[3], Tensor{18, 0, 0, 19, 0, 20}}};
    const std::vector<DonorCell> donors{opposite, normal};
    REQUIRE(SU2NativeLocal3D::SampleMovedTensor(donors, sample, observed));
    CHECK(TensorBytes(observed) == TensorBytes(EvaluateCavitySample(donors, sample).sensor));
  }
}
