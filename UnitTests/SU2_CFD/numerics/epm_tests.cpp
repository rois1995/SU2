/*!
 * \file epm_tests.cpp
 * \brief Unit tests for the numerics classes.
 * \author SU2 Contributors
 * \version 8.6.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "catch.hpp"
#include "../../../SU2_CFD/include/numerics/CNumerics.hpp"

namespace {
void Rotate(const su2double (&a)[3][3], su2double (&b)[3][3]) {
  const su2double q[3][3] = {{0.36, -0.8, 0.48}, {0.48, 0.6, 0.64}, {-0.8, 0.0, 0.6}};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      b[i][j] = 0;
      for (int k = 0; k < 3; ++k)
        for (int l = 0; l < 3; ++l) b[i][j] += q[i][k] * a[k][l] * q[j][l];
    }
}
void Equal(const su2double (&a)[3][3], const su2double (&b)[3][3]) {
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      REQUIRE(SU2_TYPE::GetValue(a[i][j]) == Approx(SU2_TYPE::GetValue(b[i][j])).margin(2e-12));
}
}  // namespace

TEST_CASE("EPM swaps eigenvectors and is rotation covariant", "[EPM]") {
  const su2double grad[3][3] = {{-0.2, 0, 0}, {0, -0.1, 0}, {0, 0, 0.3}};
  su2double rotated[3][3], r[3][3], rr[3][3], expected[3][3];
  Rotate(grad, rotated);
  for (int component = 1; component <= 3; ++component)
    for (bool swap : {false, true}) {
      CNumerics::ComputePerturbedRSM(3, component, swap, 1.0, 1.0, grad, 1.0, 1.0, 1.0, r);
      CNumerics::ComputePerturbedRSM(3, component, swap, 1.0, 1.0, rotated, 1.0, 1.0, 1.0, rr);
      Rotate(r, expected);
      Equal(rr, expected);
      REQUIRE(SU2_TYPE::GetValue(r[0][0] + r[1][1] + r[2][2]) == Approx(2.0));
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) REQUIRE(r[i][j] == Approx(SU2_TYPE::GetValue(r[j][i])).margin(2e-12));
      if (component == 1) REQUIRE(SU2_TYPE::GetValue(r[swap ? 2 : 0][swap ? 2 : 0]) == Approx(2.0));
    }
  CNumerics::ComputeStressTensor(3, expected, rotated, 1.0, 1.0, 1.0, true);
  for (auto& row : expected)
    for (auto& x : row) x = -x;
  CNumerics::ComputePerturbedRSM(3, 1, false, 0.0, 1.0, rotated, 1.0, 1.0, 1.0, r);
  Equal(r, expected);
  for (int component : {1, 2}) {
    CNumerics::ComputePerturbedRSM(3, component, false, 1.0, 1.0, rotated, 1.0, 1.0, 1.0, r);
    CNumerics::ComputePerturbedRSM(3, component, true, 1.0, 1.0, rotated, 1.0, 1.0, 1.0, rr);
    REQUIRE(CNumerics::ComputeReynoldsStressProduction(3, r, rotated, 1.0) >
            CNumerics::ComputeReynoldsStressProduction(3, rr, rotated, 1.0));
  }
}

TEST_CASE("EPM production preserves sign and tensor blend", "[EPM]") {
  const su2double grad[2][2] = {{0, 2}, {0, 0}};
  su2double r[3][3];
  for (double factor : {0.0, 0.25, 1.0}) {
    CNumerics::ComputePerturbedRSM(2, 3, false, 1.0, factor, grad, 2.0, 0.3, 1.0, r);
    const auto p = CNumerics::ComputeReynoldsStressProduction(2, r, grad, 2.0);
    REQUIRE(SU2_TYPE::GetValue(p) == Approx(1.2 * (1.0 - factor)).margin(2e-12));
    REQUIRE(SU2_TYPE::GetValue(r[0][0] + r[1][1] + r[2][2]) == Approx(2.0));
  }
  CNumerics::ComputePerturbedRSM(2, 1, true, 1.0, 1.0, grad, 2.0, 0.3, 1.0, r);
  REQUIRE(CNumerics::ComputeReynoldsStressProduction(2, r, grad, 2.0) < 0.0);
  CNumerics::ComputePerturbedRSM(2, 1, true, 1.0, 1.0, grad, 2.0, 0.0, 0.0, r);
  for (auto& row : r)
    for (auto x : row) REQUIRE(x == 0.0);
}

TEST_CASE("EPM anisotropic targets have no unique isotropic-state limit", "[EPM]") {
  for (double epsilon : {1e-4, 1e-8, 1e-12}) {
    su2double grad[3][3] = {{-epsilon, 0, 0}, {0, 0, 0}, {0, 0, epsilon}};
    su2double rotated[3][3], r[3][3], rr[3][3];
    Rotate(grad, rotated);
    for (int component : {1, 2}) {
      CNumerics::ComputePerturbedRSM(3, component, false, 1.0, 1.0, grad, 1.0, 1.0, 1.0, r);
      CNumerics::ComputePerturbedRSM(3, component, false, 1.0, 1.0, rotated, 1.0, 1.0, 1.0, rr);
      REQUIRE(fabs(SU2_TYPE::GetValue(r[0][0] - rr[0][0])) > 0.1);
    }
  }
}
