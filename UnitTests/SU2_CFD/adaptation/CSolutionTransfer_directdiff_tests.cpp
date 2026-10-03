/*!
 * \file CSolutionTransfer_directdiff_tests.cpp
 * \brief Forward-mode (DIRECT_DIFF) derivatives through the solution transfers of the mesh adaptation.
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

#include <cmath>
#include <memory>

#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "TransferTestCase.hpp"

using namespace transfer_test;

namespace {

/*--- Seeded derivative of the variable iVar of a state (any smooth, nonzero function of the value and position). ---*/
passivedouble Seed(const su2double* x, unsigned short iVar, passivedouble value, int level) {
  return 0.01 * value * (1.0 + 0.1 * iVar + 0.05 * level) + 0.3 * SU2_TYPE::GetValue(x[0]) -
         0.2 * SU2_TYPE::GetValue(x[1]) + level;
}

}  // namespace

TEST_CASE("Solution transfer: forward-mode derivatives through an identity transfer", "[Adaptation]") {
  /*--- The same mesh as donor and target (the transfers are the identity there). U^n (solution and Solution_time_n)
   *    and U^(n-1) carry nonzero seeded derivatives (as in a DIRECT_DIFF run, where the solution depends on the
   *    design variable). The transferred values and their derivatives must be the donor's, for the solution and the
   *    history, flow and SST turbulence. ---*/
  const unsigned short nDim = 2;
  for (const bool conservative : {false, true}) {
    SECTION(conservative ? "conservative" : "barycentric") {
      auto config = MakeConfig(nDim,
                               "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nMATH_PROBLEM= DIRECT\n"
                               "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 1e-3\n"
                               "TIME_ITER= 10\n");
      MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 0);
      MeshSolution target(config.get(), BoxMesh(nDim, 4, true), 0);

      /*--- Donor arrays: affine flow (zero momentum on the no-slip walls), SST k, omega; derivatives seeded. ---*/
      for (int level = 0; level < 2; ++level) {
        for (const unsigned short iSol : {FLOW_SOL, TURB_SOL}) {
          auto* nodes = donor.solver[MESH_0][iSol]->GetNodes();
          const auto nVar = donor.solver[MESH_0][iSol]->GetnVar();
          for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
            const auto* x = donor.Fine().nodes->GetCoord(iPoint);
            su2double U[MAXVAR];
            AffineFlow(nDim)(x, U);
            const bool wall = SU2_TYPE::GetValue(x[1]) < 1e-12;
            for (unsigned short iVar = 0; iVar < nVar; ++iVar) {
              passivedouble value = SU2_TYPE::GetValue(U[iVar]) * (1.0 + 0.02 * level);
              if (iSol == TURB_SOL) value = (iVar == 0 ? 1.0 + 0.1 * SU2_TYPE::GetValue(x[0]) : 1e3) * (1.0 + 0.02 * level);
              /*--- No-slip wall: zero momentum, zero derivative (as the solver imposes it). ---*/
              const bool wallMomentum = wall && iSol == FLOW_SOL && iVar >= 1 && iVar <= nDim;
              if (wallMomentum) value = 0.0;
              su2double v = value;
              SU2_TYPE::SetDerivative(v, wallMomentum ? 0.0 : Seed(x, iVar, value, level) * (iSol == TURB_SOL ? 1e-3 : 1.0));
              if (level == 0) {
                nodes->GetSolution()(iPoint, iVar) = v;
                nodes->GetSolution_time_n()(iPoint, iVar) = v;
              } else {
                nodes->GetSolution_time_n1()(iPoint, iVar) = v;
              }
            }
          }
        }
      }

      std::unique_ptr<CSolutionTransfer> transfer;
      if (conservative) {
        transfer = std::make_unique<CConservativeTransfer>();
      } else {
        transfer = std::make_unique<CBarycentricTransfer>();
      }
      {
        Mute mute;
        transfer->Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      }

      /*--- Same mesh, same numbering: compare point by point. ---*/
      REQUIRE(target.Fine().GetnPoint() == donor.Fine().GetnPoint());
      for (const unsigned short iSol : {FLOW_SOL, TURB_SOL}) {
        auto* donorNodes = donor.solver[MESH_0][iSol]->GetNodes();
        auto* nodes = target.solver[MESH_0][iSol]->GetNodes();
        for (int level = 0; level < 3; ++level) {
          INFO("solver " << iSol << ", array " << level);
          auto& donorArray = level == 2 ? donorNodes->GetSolution_time_n1() : donorNodes->GetSolution();
          auto& array = level == 0 ? nodes->GetSolution() : level == 1 ? nodes->GetSolution_time_n() : nodes->GetSolution_time_n1();
          passivedouble maxValue = 0.0, maxDerivative = 0.0;
          unsigned long nSeeded = 0, nValues = 0;
          for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
            REQUIRE(SU2_TYPE::GetValue(target.Fine().nodes->GetCoord(iPoint, 0)) ==
                    SU2_TYPE::GetValue(donor.Fine().nodes->GetCoord(iPoint, 0)));
            for (unsigned short iVar = 0; iVar < array.cols(); ++iVar) {
              const passivedouble d0 = SU2_TYPE::GetDerivative(donorArray(iPoint, iVar));
              const passivedouble d1 = SU2_TYPE::GetDerivative(array(iPoint, iVar));
              const passivedouble v0 = SU2_TYPE::GetValue(donorArray(iPoint, iVar));
              const passivedouble v1 = SU2_TYPE::GetValue(array(iPoint, iVar));
              maxValue = std::max(maxValue, fabs(v1 - v0) / std::max(fabs(v0), 1e-8));
              maxDerivative = std::max(maxDerivative, fabs(d1 - d0) / std::max(fabs(d0), 1e-8));
              nSeeded += (d0 != 0.0);
              nValues++;
            }
          }
          CHECK(nSeeded >= 0.8 * nValues);
          CHECK(maxValue < 1e-10);
          CHECK(maxDerivative < 1e-8);
        }
      }
    }
  }
}

TEST_CASE("Solution transfer: forward-mode derivatives of affine fields between meshes", "[Adaptation]") {
  /*--- Euler, 2nd-order dual time stepping, donor and new mesh different. The values (U^n, U^(n-1)) and their seeded
   *    derivatives are affine in the coordinates: both transfers reproduce affine fields exactly, so the derivatives
   *    on the new mesh are the affine seed functions at the new points (the derivative of the transfer is the transfer
   *    of the derivative). ---*/
  const unsigned short nDim = 2;
  auto seed = [](const su2double* x, unsigned short iVar, int level) {
    return 1.0 + 0.5 * iVar + 0.3 * SU2_TYPE::GetValue(x[0]) - (0.2 + 0.1 * level) * SU2_TYPE::GetValue(x[1]) + level;
  };
  for (const bool conservative : {false, true}) {
    SECTION(conservative ? "conservative" : "barycentric") {
      auto config = MakeConfig(nDim,
                               "SOLVER= EULER\nMATH_PROBLEM= DIRECT\nTIME_DOMAIN= YES\n"
                               "TIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 1e-3\nTIME_ITER= 10\n");
      MeshSolution donor(config.get(), BoxMesh(nDim, 4, true), 0);
      MeshSolution target(config.get(), BoxMesh(nDim, 6, true), 0);
      auto* donorNodes = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
      for (int level = 0; level < 2; ++level) {
        for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
          const auto* x = donor.Fine().nodes->GetCoord(iPoint);
          su2double U[MAXVAR];
          AffineFlow(nDim)(x, U);
          for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
            su2double v = SU2_TYPE::GetValue(U[iVar]) * (1.0 + 0.02 * level);
            SU2_TYPE::SetDerivative(v, seed(x, iVar, level));
            if (level == 0) {
              donorNodes->GetSolution()(iPoint, iVar) = v;
              donorNodes->GetSolution_time_n()(iPoint, iVar) = v;
            } else {
              donorNodes->GetSolution_time_n1()(iPoint, iVar) = v;
            }
          }
        }
      }
      std::unique_ptr<CSolutionTransfer> transfer;
      if (conservative) {
        transfer = std::make_unique<CConservativeTransfer>();
      } else {
        transfer = std::make_unique<CBarycentricTransfer>();
      }
      {
        Mute mute;
        transfer->Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
      }
      auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
      for (int array = 0; array < 3; ++array) {
        INFO("array " << array);
        const int level = (array == 2) ? 1 : 0;
        auto& values = array == 0 ? nodes->GetSolution() : array == 1 ? nodes->GetSolution_time_n() : nodes->GetSolution_time_n1();
        passivedouble maxError = 0.0;
        for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
          const auto* x = target.Fine().nodes->GetCoord(iPoint);
          for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
            const passivedouble exact = seed(x, iVar, level);
            maxError = std::max(maxError, fabs(SU2_TYPE::GetDerivative(values(iPoint, iVar)) - exact) / fabs(exact));
          }
        }
        CHECK(maxError < 1e-9);
      }
    }
  }
}
