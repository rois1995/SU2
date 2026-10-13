/*!
 * \file CNEMOUpwind_tests.cpp
 * \brief Unit tests for the upwind numerics classes of the NEMO solver.
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
#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "../../../SU2_CFD/include/fluid/CSU2TCLib.hpp"
#include "../../../SU2_CFD/include/numerics/NEMO/convection/ausm_slau.hpp"
#include "../../../SU2_CFD/include/variables/CNEMOEulerVariable.hpp"

namespace {

/*!
 * \brief Left or right state of a face, given with the variables that are natural to choose.
 */
struct NEMOFaceState {
  su2double pressure, temperature, temperature_ve;
  std::vector<su2double> velocity;
};

/*!
 * \brief Two points of a five species air mixture and what is needed to evaluate an upwind
 * numerics class of the NEMO solver from their conservative variables. The primitive variables,
 * the vib.-el. energies, dPdU and gamma are recomputed from the conservative variables the same
 * way the solver does it, so that a flux can be differentiated with finite differences.
 */
struct NEMOUpwindFixture {
  static constexpr unsigned short nSpecies = 5;
  const unsigned short nDim, nVar, nPrimVar, nPrimVarGrad;

  /*--- Dissociated air (N2, O2, NO, N, O), so that gamma depends on the composition. ---*/
  const su2double massFrac[nSpecies] = {0.70, 0.15, 0.03, 0.02, 0.10};

  std::unique_ptr<CConfig> config;
  std::unique_ptr<CSU2TCLib> fluidModel;
  std::unique_ptr<CNEMOEulerVariable> nodes;
  std::vector<su2double> normal;
  su2double sensor = 1.0;
  su2double Tve_start[2] = {0.0, 0.0};

  NEMOUpwindFixture(unsigned short val_nDim, const std::string& mach)
      : nDim(val_nDim), nVar(nSpecies + nDim + 2), nPrimVar(nSpecies + nDim + 10), nPrimVarGrad(nSpecies + nDim + 8) {
    std::stringstream options;
    options << "SOLVER= NEMO_EULER\n"
            << "GAS_MODEL= AIR-5\n"
            << "GAS_COMPOSITION= (0.70, 0.15, 0.03, 0.02, 0.10)\n"
            << "FLUID_MODEL= SU2_NONEQ\n"
            << "MACH_NUMBER= " << mach << "\n"
            << "TIME_DISCRE_FLOW= EULER_IMPLICIT\n";
    config.reset(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
    fluidModel.reset(new CSU2TCLib(config.get(), nDim, false));

    const su2double zeroMach[3] = {0.0};
    nodes.reset(new CNEMOEulerVariable(1000.0, massFrac, zeroMach, 300.0, 300.0, 2, nDim, nVar, nPrimVar, nPrimVarGrad,
                                       config.get(), fluidModel.get()));

    /*--- A face that is not aligned with the axes. ---*/
    if (nDim == 2)
      normal = {0.3, 0.4};
    else
      normal = {0.2, 0.3, 0.6};
  }

  /*!
   * \brief Velocity with the given components normal and tangential to the face.
   */
  std::vector<su2double> Velocity(su2double normalVel, su2double tangentVel) const {
    const su2double area = GeometryToolbox::Norm(nDim, normal.data());
    std::vector<su2double> unitNormal(nDim), tangent(nDim, 0.0), velocity(nDim);
    for (auto iDim = 0u; iDim < nDim; iDim++) unitNormal[iDim] = normal[iDim] / area;
    tangent[0] = -unitNormal[1];
    tangent[1] = unitNormal[0];
    const su2double tangentNorm = GeometryToolbox::Norm(nDim, tangent.data());
    for (auto iDim = 0u; iDim < nDim; iDim++)
      velocity[iDim] = normalVel * unitNormal[iDim] + tangentVel * tangent[iDim] / tangentNorm;
    return velocity;
  }

  /*!
   * \brief Conservative variables of a state.
   */
  std::vector<su2double> Conservatives(const NEMOFaceState& state) {
    fluidModel->SetTDStatePTTv(state.pressure, massFrac, state.temperature, state.temperature_ve);
    const su2double rho = fluidModel->GetDensity();
    const auto& energies = fluidModel->ComputeMixtureEnergies();
    const su2double sqvel = GeometryToolbox::SquaredNorm(nDim, state.velocity.data());

    std::vector<su2double> U(nVar);
    for (auto iSpecies = 0u; iSpecies < nSpecies; iSpecies++) U[iSpecies] = rho * massFrac[iSpecies];
    for (auto iDim = 0u; iDim < nDim; iDim++) U[nSpecies + iDim] = rho * state.velocity[iDim];
    U[nSpecies + nDim] = rho * (energies[0] + 0.5 * sqvel);
    U[nSpecies + nDim + 1] = rho * energies[1];
    return U;
  }

  /*!
   * \brief Evaluate the numerics for the given conservative variables of the two points.
   */
  CNumerics::ResidualType<> Evaluate(CNumerics& numerics, const std::vector<su2double>& U_i,
                                     const std::vector<su2double>& U_j) {
    nodes->SetSolution(0, U_i.data());
    nodes->SetSolution(1, U_j.data());
    for (auto iPoint = 0ul; iPoint < 2; iPoint++) {
      /*--- The vib.-el. temperature is found iteratively, always start from the same value. ---*/
      nodes->GetPrimitive(iPoint)[nodes->GetTveIndex()] = Tve_start[iPoint];
      REQUIRE(!nodes->SetPrimVar(iPoint, fluidModel.get()));
    }
    numerics.SetNormal(normal.data());
    numerics.SetConservative(nodes->GetSolution(0), nodes->GetSolution(1));
    numerics.SetPrimitive(nodes->GetPrimitive(0), nodes->GetPrimitive(1));
    numerics.SetdPdU(nodes->GetdPdU(0), nodes->GetdPdU(1));
    numerics.SetdTdU(nodes->GetdTdU(0), nodes->GetdTdU(1));
    numerics.SetdTvedU(nodes->GetdTvedU(0), nodes->GetdTvedU(1));
    numerics.SetEve(nodes->GetEve(0), nodes->GetEve(1));
    numerics.SetCvve(nodes->GetCvve(0), nodes->GetCvve(1));
    numerics.SetGamma(nodes->GetGamma(0), nodes->GetGamma(1));
    numerics.SetSensor(sensor, sensor);
    return numerics.ComputeResidual(config.get());
  }

  /*!
   * \brief Largest difference between the Jacobians of the numerics and central finite differences
   * of its flux. Each column is scaled with the size of its conservative variable, and the
   * difference is then divided by the largest entry of its row, so that all equations count.
   */
  su2double JacobianError(CNumerics& numerics, const NEMOFaceState& state_i, const NEMOFaceState& state_j,
                          su2double relStep) {
    const auto U_i = Conservatives(state_i);
    const auto U_j = Conservatives(state_j);
    Tve_start[0] = state_i.temperature_ve;
    Tve_start[1] = state_j.temperature_ve;

    /*--- Copy the Jacobians, the next evaluations reuse the storage of the numerics. ---*/
    std::vector<std::vector<su2double>> analytic(nVar, std::vector<su2double>(2 * nVar));
    std::vector<su2double> scale(2 * nVar);
    {
      const auto residual = Evaluate(numerics, U_i, U_j);
      for (auto iVar = 0u; iVar < nVar; iVar++) {
        for (auto jVar = 0u; jVar < nVar; jVar++) {
          analytic[iVar][jVar] = residual.jacobian_i[iVar][jVar];
          analytic[iVar][nVar + jVar] = residual.jacobian_j[iVar][jVar];
        }
      }
      /*--- Size of the conservative variables: rho, rho*a and rho*a^2. ---*/
      for (auto iPoint = 0ul; iPoint < 2; iPoint++) {
        const su2double rho = nodes->GetDensity(iPoint), a = nodes->GetSoundSpeed(iPoint);
        for (auto jVar = 0u; jVar < nVar; jVar++) {
          scale[iPoint * nVar + jVar] = (jVar < nSpecies) ? rho : (jVar < nVar - 2u) ? rho * a : rho * a * a;
        }
      }
    }

    std::vector<std::vector<su2double>> numeric(nVar, std::vector<su2double>(2 * nVar));
    for (auto jVar = 0u; jVar < 2 * nVar; jVar++) {
      const su2double step = relStep * scale[jVar];
      auto Up_i = U_i, Up_j = U_j, Um_i = U_i, Um_j = U_j;
      if (jVar < nVar) {
        Up_i[jVar] += step;
        Um_i[jVar] -= step;
      } else {
        Up_j[jVar - nVar] += step;
        Um_j[jVar - nVar] -= step;
      }
      std::vector<su2double> fluxPlus(nVar);
      {
        const auto residual = Evaluate(numerics, Up_i, Up_j);
        for (auto iVar = 0u; iVar < nVar; iVar++) fluxPlus[iVar] = residual.residual[iVar];
      }
      const auto residual = Evaluate(numerics, Um_i, Um_j);
      for (auto iVar = 0u; iVar < nVar; iVar++)
        numeric[iVar][jVar] = (fluxPlus[iVar] - residual.residual[iVar]) / (2.0 * step);
    }

    su2double error = 0.0;
    for (auto iVar = 0u; iVar < nVar; iVar++) {
      su2double rowScale = 0.0, rowError = 0.0;
      for (auto jVar = 0u; jVar < 2 * nVar; jVar++) {
        rowScale = std::max(rowScale, std::fabs(numeric[iVar][jVar]) * scale[jVar]);
        rowError = std::max(rowError, std::fabs(analytic[iVar][jVar] - numeric[iVar][jVar]) * scale[jVar]);
      }
      REQUIRE(rowScale > 0.0);
      error = std::max(error, rowError / rowScale);
    }
    return error;
  }
};

/*!
 * \brief A pair of states with a name for the report of the test.
 */
struct NEMOFaceCase {
  std::string name;
  NEMOFaceState state_i, state_j;
};

/*!
 * \brief States that cover the branches of the AUSM-type schemes. Velocities are given as the
 * components normal and tangential to the face.
 */
std::vector<NEMOFaceCase> FaceCases(const NEMOUpwindFixture& f) {
  return {
      {"subsonic, from i to j",
       {20000.0, 2500.0, 2300.0, f.Velocity(450.0, 200.0)},
       {18000.0, 2400.0, 2350.0, f.Velocity(520.0, 150.0)}},
      {"subsonic, from j to i",
       {20000.0, 2500.0, 2300.0, f.Velocity(-450.0, 200.0)},
       {18000.0, 2400.0, 2350.0, f.Velocity(-520.0, -150.0)}},
      {"supersonic, from i to j",
       {5000.0, 900.0, 1200.0, f.Velocity(2200.0, 500.0)},
       {5600.0, 950.0, 1150.0, f.Velocity(2100.0, 650.0)}},
      {"supersonic, from j to i",
       {5000.0, 900.0, 1200.0, f.Velocity(-2200.0, 500.0)},
       {5600.0, 950.0, 1150.0, f.Velocity(-2100.0, 650.0)}},
      {"transonic, from i to j",
       {9000.0, 1500.0, 1400.0, f.Velocity(900.0, -300.0)},
       {11000.0, 1650.0, 1450.0, f.Velocity(650.0, -250.0)}},
      {"transonic, from j to i",
       {11000.0, 1650.0, 1450.0, f.Velocity(-650.0, 250.0)},
       {9000.0, 1500.0, 1400.0, f.Velocity(-900.0, 300.0)}},
      {"strong shock, from i to j",
       {1000.0, 300.0, 300.0, f.Velocity(1900.0, 400.0)},
       {33000.0, 1900.0, 800.0, f.Velocity(350.0, 400.0)}},
      {"strong shock, from j to i",
       {33000.0, 1900.0, 800.0, f.Velocity(-350.0, -400.0)},
       {1000.0, 300.0, 300.0, f.Velocity(-1900.0, -400.0)}},
      {"strong expansion",
       {30000.0, 2000.0, 1800.0, f.Velocity(-300.0, 100.0)},
       {3000.0, 1000.0, 1500.0, f.Velocity(500.0, -200.0)}},
  };
}

}  // namespace

TEST_CASE("NEMO AUSM, AUSM+-up2 and AUSM+M return the Jacobians of their flux", "[NEMO][Upwind][Jacobian]") {
  /*--- The finite differences are limited by the tolerance of the iterative solution for the vib.-el.
   * temperature, which is why the step is rather large. With this step they agree with exact Jacobians
   * to about 3e-7, whereas the Jacobians of plain AUSM, used before for these two schemes, differ from
   * them by 0.5 or more. For plain AUSM the truncation error of the finite differences is larger
   * (3e-5 for the strong shock, it decreases with the square of the step), hence the wider tolerance. ---*/
  const su2double relStep = 1e-4, tolerance = 1e-5, toleranceAusm = 5e-5;

  for (const unsigned short nDim : {2, 3}) {
    /*--- The low reference Mach number activates the low-speed scaling of AUSM+-up2 and AUSM+M. ---*/
    for (const std::string mach : {"5.0", "0.3"}) {
      NEMOUpwindFixture fixture(nDim, mach);
      /*--- An intermediate value of the pressure sensor keeps all the terms of AUSM+M active. ---*/
      fixture.sensor = 0.6;

      CUpwAUSM_NEMO ausm(nDim, fixture.nVar, fixture.nPrimVar, fixture.nPrimVarGrad, fixture.config.get());
      CUpwAUSMPLUSUP2_NEMO ausmPlusUp2(nDim, fixture.nVar, fixture.nPrimVar, fixture.nPrimVarGrad,
                                       fixture.config.get());
      CUpwAUSMPLUSM_NEMO ausmPlusM(nDim, fixture.nVar, fixture.nPrimVar, fixture.nPrimVarGrad, fixture.config.get());

      for (const auto& faceCase : FaceCases(fixture)) {
        INFO("nDim = " << nDim << ", reference Mach = " << mach << ", " << faceCase.name);
        CHECK(fixture.JacobianError(ausm, faceCase.state_i, faceCase.state_j, relStep) < toleranceAusm);
        CHECK(fixture.JacobianError(ausmPlusUp2, faceCase.state_i, faceCase.state_j, relStep) < tolerance);
        CHECK(fixture.JacobianError(ausmPlusM, faceCase.state_i, faceCase.state_j, relStep) < tolerance);
      }
    }
  }
}
