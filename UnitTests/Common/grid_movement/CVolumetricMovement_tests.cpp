/*!
 * \file CVolumetricMovement_tests.cpp
 * \brief Unit tests for the element volumes used by the mesh deformation.
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
#include <vector>
#include "../../../Common/include/grid_movement/CVolumetricMovement.hpp"

namespace {

/*--- Gives access to the volume functions without a geometry. ---*/
struct CTestVolumetricMovement : public CVolumetricMovement {
  explicit CTestVolumetricMovement(unsigned short val_nDim) { nDim = val_nDim; }
};

/*--- Fills the corners from a list of points; mirror = -1 mirrors x, which inverts the element. ---*/
void SetCorners(su2double CoordCorners[8][3], const std::vector<std::vector<double>>& points, double mirror) {
  for (size_t iNode = 0; iNode < points.size(); iNode++) {
    for (size_t iDim = 0; iDim < points[iNode].size(); iDim++) {
      CoordCorners[iNode][iDim] = (iDim == 0 ? mirror : 1.0) * points[iNode][iDim];
    }
  }
}

}  // namespace

TEST_CASE("Detection of inverted elements in the mesh deformation", "[Deformation]") {
  /*--- Reference elements with the orientation enforced by CPhysicalGeometry::Check_IntElem_Orientation have
   positive corner Jacobians, the mirrored (inverted) ones negative corner Jacobians. ---*/

  su2double CoordCorners[8][3] = {};

  const CTestVolumetricMovement movement2D(2);

  for (const double mirror : {1.0, -1.0}) {
    SetCorners(CoordCorners, {{0, 0}, {1, 0}, {0, 1}}, mirror);
    CHECK(SU2_TYPE::GetValue(movement2D.GetMinCornerJacobian(3, CoordCorners)) == Approx(mirror));

    SetCorners(CoordCorners, {{0, 0}, {1, 0}, {1, 1}, {0, 1}}, mirror);
    CHECK(SU2_TYPE::GetValue(movement2D.GetMinCornerJacobian(4, CoordCorners)) == Approx(mirror));
  }

  const CTestVolumetricMovement movement3D(3);

  for (const double mirror : {1.0, -1.0}) {
    SetCorners(CoordCorners, {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, mirror);
    CHECK(SU2_TYPE::GetValue(movement3D.GetMinCornerJacobian(4, CoordCorners)) == Approx(mirror));

    SetCorners(CoordCorners, {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.5, 0.5, 1}}, mirror);
    CHECK(SU2_TYPE::GetValue(movement3D.GetMinCornerJacobian(5, CoordCorners)) == Approx(mirror));

    SetCorners(CoordCorners, {{0, 0, 0}, {0, 1, 0}, {1, 0, 0}, {0, 0, 1}, {0, 1, 1}, {1, 0, 1}}, mirror);
    CHECK(SU2_TYPE::GetValue(movement3D.GetMinCornerJacobian(6, CoordCorners)) == Approx(mirror));

    SetCorners(CoordCorners, {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}},
               mirror);
    CHECK(SU2_TYPE::GetValue(movement3D.GetMinCornerJacobian(8, CoordCorners)) == Approx(mirror));
  }

  /*--- Hexahedron folded at one corner: the volume (sum of absolute values) stays positive. ---*/

  SetCorners(CoordCorners, {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, -0.5}, {0, 1, 1}},
             1.0);
  CHECK(movement3D.GetHexa_Volume(CoordCorners) > 0.0);
  CHECK(movement3D.GetMinCornerJacobian(8, CoordCorners) < 0.0);

  /*--- Thin and warped, but valid, boundary layer cell of the deformed brick_hex_rans test case: the signed sum of
   its tetrahedral decomposition would be negative. ---*/

  SetCorners(CoordCorners,
             {{1.641025641025641, 0.1578947368421052, 0.018689568960886069},
              {1.641025641025641, 0.21052631578947359, 0.030149698537023321},
              {1.5897435897435901, 0.21052631578947359, 0.037206957328457177},
              {1.5897435897435901, 0.157894736842105, 0.023064310044792879},
              {1.641025725218126, 0.15789452936859219, 0.018690568964974171},
              {1.6410257768416501, 0.21052609236238609, 0.030150698536001701},
              {1.589743728550705, 0.2105260400643249, 0.037207957326707619},
              {1.589743675790291, 0.15789448080478949, 0.023065310049577441}},
             1.0);
  CHECK(movement3D.GetMinCornerJacobian(8, CoordCorners) > 0.0);
}
