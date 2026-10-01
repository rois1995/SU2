/*!
 * \file CFreeFormBlending_tests.cpp
 * \brief Unit tests for the Bezier and B-spline blending functions of the FFD boxes.
 * \author SU2 Contributors
 * \version 8.5.0 "Harrier"
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
#include "../../../Common/include/grid_movement/CBezierBlending.hpp"
#include "../../../Common/include/grid_movement/CBSplineBlending.hpp"

TEST_CASE("FFD blending functions are a partition of unity", "[FFD]") {
  /*--- Also at the ends of the parameter range, where the first/last basis function is 1. ---*/

  const short nControl = 5;

  for (const short order : {2, 3, 4}) {
    CBSplineBlending bspline(order, nControl);

    for (const double t : {0.0, 0.3, 0.75, 1.0}) {
      su2double sum = 0.0;
      for (short i = 0; i < nControl; i++) sum += bspline.GetBasis(i, t);
      CHECK(SU2_TYPE::GetValue(sum) == Approx(1.0));
    }
    CHECK(SU2_TYPE::GetValue(bspline.GetBasis(0, 0.0)) == Approx(1.0));
    CHECK(SU2_TYPE::GetValue(bspline.GetBasis(nControl - 1, 1.0)) == Approx(1.0));
  }

  for (const short order : {1, 2, 3, 4}) {
    CBezierBlending bezier(order, order);

    for (const double t : {0.0, 0.3, 1.0}) {
      su2double sum = 0.0;
      for (short i = 0; i < order; i++) sum += bezier.GetBasis(i, t);
      CHECK(SU2_TYPE::GetValue(sum) == Approx(1.0));
    }
  }
}

TEST_CASE("Derivatives of the Bezier blending functions", "[FFD]") {
  /*--- Degree 1: B0 = 1 - t, B1 = t. ---*/

  CBezierBlending linear(2, 2);

  for (const double t : {0.0, 0.3, 0.8}) {
    CHECK(SU2_TYPE::GetValue(linear.GetDerivative(0, t, 1)) == Approx(-1.0));
    CHECK(SU2_TYPE::GetValue(linear.GetDerivative(1, t, 1)) == Approx(1.0));
    CHECK(SU2_TYPE::GetValue(linear.GetDerivative(0, t, 2)) == Approx(0.0).margin(1e-12));
    CHECK(SU2_TYPE::GetValue(linear.GetDerivative(1, t, 2)) == Approx(0.0).margin(1e-12));
  }

  /*--- Degree 2: B0 = (1 - t)^2, B1 = 2 t (1 - t), B2 = t^2. ---*/

  CBezierBlending quadratic(3, 3);

  for (const double t : {0.2, 0.7}) {
    CHECK(SU2_TYPE::GetValue(quadratic.GetDerivative(0, t, 1)) == Approx(-2.0 * (1.0 - t)));
    CHECK(SU2_TYPE::GetValue(quadratic.GetDerivative(1, t, 1)) == Approx(2.0 - 4.0 * t));
    CHECK(SU2_TYPE::GetValue(quadratic.GetDerivative(2, t, 1)) == Approx(2.0 * t));
    CHECK(SU2_TYPE::GetValue(quadratic.GetDerivative(0, t, 2)) == Approx(2.0));
    CHECK(SU2_TYPE::GetValue(quadratic.GetDerivative(1, t, 2)) == Approx(-4.0));
    CHECK(SU2_TYPE::GetValue(quadratic.GetDerivative(2, t, 2)) == Approx(2.0));
  }
}
