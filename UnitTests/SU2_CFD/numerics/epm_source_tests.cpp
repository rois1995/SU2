/*!
 * \file epm_source_tests.cpp
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
#include "../../../SU2_CFD/include/numerics/turbulent/turb_sources.hpp"
#include "../../../SU2_CFD/include/variables/CEulerVariable.hpp"

TEST_CASE("SST2003 EPM limits signed production in both source equations", "[EPM]") {
  for (bool uq : {false, true})
    for (bool swap : {false, true})
      for (double omega : {0.01, 100.0}) {
        std::stringstream options;
        options << "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= (V2003m"
                << (uq ? ", UQ" : "") << ")\nUQ_COMPONENT= 1\nUQ_PERMUTE= " << (swap ? "YES" : "NO")
                << "\nUQ_DELTA_B= 1\nUQ_URLX= 1\n";
        CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
        const su2double constants[] = {0.85, 1.0, 0.5, 0.856, 0.075, 0.0828, 0.09, 0.31, 5.0 / 9, 0.44, 10};
        CSourcePieceWise_TurbSST<CEulerVariable::CIndices<unsigned short>> source(2, 2, constants, 0, 0, &config);
        su2double v[11] = {};
        v[4] = 2;
        v[6] = 10;
        v[7] = 0.01;
        v[8] = 0.3;
        su2double scalar[2] = {1, omega}, grad[3][2] = {{0, 0}, {0, 2}, {0, 0}}, sg[2][2] = {};
        su2double vort[3] = {0, 0, -2};
        source.SetPrimitive(v, v);
        source.SetScalarVar(scalar, scalar);
        source.SetPrimVarGradient({&grad[0][0], 2}, {&grad[0][0], 2});
        source.SetScalarVarGradient({&sg[0][0], 2}, {&sg[0][0], 2});
        source.SetVorticity(vort, vort);
        source.SetStrainMag(2, 2);
        source.SetF1blending(1, 1);
        source.SetF2blending(1);
        source.SetCrossDiff(0);
        source.SetDistance(1, 1);
        source.SetVolume(3);
        const double raw = uq ? (swap ? -4.0 : 4.0) : 1.2;
        const double pk = std::max(0.0, std::min(raw, 10 * 0.09 * 2 * omega));
        const auto residual = source.ComputeResidual(&config);
        REQUIRE(SU2_TYPE::GetValue(residual[0]) == Approx(3 * (pk - 0.09 * 2 * omega)).margin(1e-12));
        REQUIRE(SU2_TYPE::GetValue(residual[1]) ==
                Approx(3 * ((5.0 / 9 * 2 / 0.3) * pk - 0.075 * 2 * omega * omega)).margin(1e-10));
        if (uq) {
          v[8] = 0.0;
          const auto zero_visc = source.ComputeResidual(&config);
          REQUIRE(SU2_TYPE::GetValue(zero_visc[1]) == Approx(-3 * 0.075 * 2 * omega * omega));
        }
      }
}
