/*!
 * \file epm_extraction_tests.cpp
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
#include "../../../SU2_CFD/include/numerics/flow/flow_diffusion.hpp"
#include "../../../SU2_CFD/include/variables/CEulerVariable.hpp"

namespace {
struct Result {
  std::array<double, 4> value{};
  double parameter = 0, state = 0;
};

Result Evaluate(bool flux, int mode, bool legacy, bool active, double amplitude, double omega = 100) {
  std::stringstream options;
  options << "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= (V2003m" << (legacy ? ", UQ" : "")
          << ")\nUQ_COMPONENT= " << mode / 2 + 1 << "\nUQ_PERMUTE= " << (mode % 2 ? "YES" : "NO")
          << "\nUQ_DELTA_B= 1\nUQ_URLX= 1\n";
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  auto& epm = config.GetEPMExtraction();
  epm.active = active;
  epm.component = mode / 2 + 1;
  epm.permute = mode % 2;
  epm.amplitude = amplitude;
  su2double v[11] = {1, .7, .2, 1, 2, 10, 10, .01, .3, .2, 1};
  su2double scalar[2] = {1, omega}, grad[3][2] = {{.1, 0}, {0, 2}, {0, 0}}, sg[2][2] = {};
  su2double vort[3] = {0, 0, -2}, x[2] = {0, 0}, y[2] = {1, 0}, normal[2] = {1, .4};
  const su2double constants[] = {.85, 1, .5, .856, .075, .0828, .09, .31, 5.0 / 9, .44, 10};
  std::unique_ptr<CNumerics> source;
  if (flux)
    source.reset(new CAvgGrad_Flow(2, 4, false, &config));
  else
    source.reset(
        new CSourcePieceWise_TurbSST<CEulerVariable::CIndices<unsigned short>>(2, 2, constants, 0, 0, &config));
  source->SetPrimitive(v, v);
  source->SetScalarVar(scalar, scalar);
  source->SetPrimVarGradient({&grad[0][0], 2}, {&grad[0][0], 2});
  source->SetScalarVarGradient({&sg[0][0], 2}, {&sg[0][0], 2});
  source->SetVorticity(vort, vort);
  source->SetStrainMag(2, 2);
  source->SetF1blending(1, 1);
  source->SetF2blending(1);
  source->SetCrossDiff(0);
  source->SetDistance(1, 1);
  source->SetVolume(3);
  source->SetCoord(x, y);
  source->SetNormal(normal);
  source->SetTurbKineticEnergy(1, 1);
  AD::Reset();
#ifdef CODI_REVERSE_TYPE
  AD::PreaccEnabled = true;
#endif
  AD::StartRecording();
  AD::RegisterInput(epm.amplitude);
  auto& state = flux ? v[1] : scalar[0];
  AD::RegisterInput(state);
  const auto residual = source->ComputeResidual(&config);
  Result result;
  su2double objective = 0;
  for (int i = 0; i < (flux ? 4 : 2); ++i) {
    result.value[i] = SU2_TYPE::GetValue(residual[i]);
    objective += (i + 1) * residual[i];
  }
  AD::RegisterOutput(objective);
  AD::StopRecording();
  SU2_TYPE::SetDerivative(objective, 1);
  AD::ComputeAdjoint();
  result.parameter = SU2_TYPE::GetDerivative(epm.amplitude);
  result.state = SU2_TYPE::GetDerivative(state);
  AD::ClearAdjoints();
  return result;
}
}  // namespace

TEST_CASE("EPM extraction preserves baseline and classical local endpoints", "[EPM][EPMExtraction]") {
  for (bool flux : {false, true})
    for (double omega : {.01, 100.0}) {
      const auto base = Evaluate(flux, 0, false, false, 0, omega);
      for (int mode = 0; mode < 5; ++mode) {
        const auto zero = Evaluate(flux, mode, false, true, 0, omega);
        const auto end = Evaluate(flux, mode, false, true, 1, omega);
        const auto classical = Evaluate(flux, mode, true, false, 0, omega);
        const auto half = Evaluate(flux, mode, false, true, .5, omega);
        for (int i = 0; i < (flux ? 4 : 2); ++i) {
          CHECK(zero.value[i] == Approx(base.value[i]).margin(1e-12));
          CHECK(end.value[i] == Approx(classical.value[i]).margin(1e-12));
          CHECK(half.value[i] == Approx(.5 * (base.value[i] + classical.value[i])).margin(1e-12));
        }
#ifdef CODI_REVERSE_TYPE
        CHECK(zero.state == Approx(base.state).margin(1e-12));
        const double h = 1e-5;
        const auto step = Evaluate(flux, mode, false, true, h, omega);
        double fd = 0;
        for (int i = 0; i < (flux ? 4 : 2); ++i) fd += (i + 1) * (step.value[i] - zero.value[i]) / h;
        CHECK(zero.parameter == Approx(fd).margin(1e-7));
        CHECK(base.parameter == 0);
#endif
      }
    }
}

TEST_CASE("EPM tie probes retain full strength", "[EPM][EPMExtraction]") {
  const double grad[3][3] = {};
  for (int component : {1, 2, 3}) {
    for (int pair = 0; pair < 3; ++pair)
      for (double angle : {0., 1.5707963267948966}) {
        double r[3][3], v[3][3], eig[3], work[3];
        bool ambiguous = false;
        CNumerics::ComputePerturbedRSM(3, component, false, 1., 1., grad, 1., 1., 1., r, angle,
                                       CEPMExtraction::gap_tolerance, &ambiguous, pair);
        CBlasStructure::EigenDecomposition(r, v, eig, 3, work);
        CHECK(ambiguous == (component != 3));
        const double expected[3][3] = {{0, 0, 2}, {0, 1, 1}, {2. / 3, 2. / 3, 2. / 3}};
        for (int i = 0; i < 3; ++i) CHECK(eig[i] == Approx(expected[component - 1][i]).margin(1e-12));
      }
  }
}

#if defined(CODI_REVERSE_TYPE) && !defined(HAVE_OMP)
TEST_CASE("EPM configuration accepts the constrained coupled adjoint", "[EPMExtraction]") {
  std::stringstream options;
  options << "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= V2003m\n"
          << "MATH_PROBLEM= DISCRETE_ADJOINT\nOBJECTIVE_FUNCTION= DRAG\n"
          << "FROZEN_VISC_DISC= NO\nFROZEN_LIMITER_DISC= NO\nINCONSISTENT_DISC= NO\n"
          << "SLOPE_LIMITER_FLOW= NONE\nMGLEVEL= 0\nCFL_ADAPT= NO\n"
          << "UQ_DELTA_B= 1\nUQ_URLX= 1\nSST_EPM_POSTPROCESS= YES\n";
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  CHECK(config.GetEPMExtraction().enabled);
  CHECK_FALSE(config.GetEPMExtraction().active);
  CHECK_FALSE(config.GetSSTParsedOptions().uq);
}
#endif
