/*!
 * \file flow_sources_tests.cpp
 * \brief Axisymmetric source residual and its conservative Jacobian.
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
#include <array>
#include <sstream>
#include "../../../SU2_CFD/include/numerics/flow/flow_sources.hpp"

TEST_CASE("Axisymmetric source includes SST k and differentiates the energy flux", "[FlowSources]") {
  std::stringstream options("SOLVER= RANS\nKIND_TURB_MODEL= SST\nREYNOLDS_NUMBER= 1e6\n");
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  CSourceAxisymmetric_Flow numerics(2, 4, &config);
  const su2double coord[2] = {0.0, 0.7};
  su2double primitive[11] = {};
  su2activematrix gradient(4, 2), auxiliary(3, 2);
  gradient = 0.0;
  auxiliary = 0.0;
  numerics.SetCoord(coord, coord);
  numerics.SetVolume(0.3);
  numerics.SetPrimitive(primitive, primitive);
  numerics.SetPrimVarGradient(CMatrixView<const su2double>(gradient), CMatrixView<const su2double>(gradient));
  numerics.SetAuxVarGrad(CMatrixView<const su2double>(auxiliary), CMatrixView<const su2double>(auxiliary));
  for (const su2double k : {0.0, 0.15}) {
    const std::array<su2double, 4> state = {1.2, 0.6, 0.24, 3.0 + 1.2 * k};
    numerics.SetTurbKineticEnergy(k, k);
    numerics.SetConservative(state.data(), state.data());
    const auto reference = numerics.ComputeResidual(&config);
    su2double jacobian[4][4];
    for (auto row = 0u; row < 4; ++row)
      for (auto col = 0u; col < 4; ++col) jacobian[row][col] = reference.jacobian_i[row][col];
    const auto pressure =
        0.4 * (state[3] - 0.5 * (state[1] * state[1] + state[2] * state[2]) / state[0] - state[0] * k);
    CHECK(reference.residual[3] == Approx(0.3 / coord[1] * state[2] / state[0] * (state[3] + pressure)));
    for (auto col = 0u; col < 4; ++col) {
      auto plus = state, minus = state;
      const su2double h = 1e-6;
      plus[col] += h;
      minus[col] -= h;
      numerics.SetConservative(plus.data(), plus.data());
      const auto fp = numerics.ComputeResidual(&config);
      std::array<su2double, 4> positive;
      for (auto row = 0u; row < 4; ++row) positive[row] = fp.residual[row];
      numerics.SetConservative(minus.data(), minus.data());
      const auto fm = numerics.ComputeResidual(&config);
      for (auto row = 0u; row < 4; ++row)
        CHECK(jacobian[row][col] == Approx((positive[row] - fm.residual[row]) / (2 * h)).margin(1e-8));
    }
  }
}
