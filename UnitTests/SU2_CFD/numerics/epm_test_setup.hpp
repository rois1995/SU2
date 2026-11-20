/*!
 * \file epm_test_setup.hpp
 * \brief Fixed-state setup shared by the EPM extraction and EPM field-sensitivity unit tests.
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

#pragma once

#include <array>
#include <memory>
#include <sstream>
#include <string>

#include "../../../SU2_CFD/include/numerics/turbulent/turb_sources.hpp"
#include "../../../SU2_CFD/include/numerics/flow/flow_diffusion.hpp"
#include "../../../SU2_CFD/include/variables/CEulerVariable.hpp"
#include "../../../SU2_CFD/include/numerics_simd/flow/diffusion/common.hpp"

namespace epm_test {

/*! \brief RANS SST options; legacy selects the classical UQ production of the case mode. */
inline std::string Options(int mode, bool legacy) {
  std::stringstream options;
  options << "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= (V2003m" << (legacy ? ", UQ" : "")
          << ")\nUQ_COMPONENT= " << mode / 2 + 1 << "\nUQ_PERMUTE= " << (mode % 2 ? "YES" : "NO")
          << "\nUQ_DELTA_B= 1\nUQ_URLX= 1\n";
  return options.str();
}

/*! \brief Point state of the scalar viscous flux and of the SST source. The numerics keep pointers into it. */
struct PointState {
  su2double v[12] = {1, .7, .2, 1, 2, 10, 10, .01, .3, .2, 1};
  su2double scalar[2] = {1, 100};
  su2double grad[4][3] = {{.1, 0, .02}, {0, 2, .4}, {0, 0, 0}, {.3, 0, 0}};
  su2double sg[2][3] = {}, vort[3] = {0, .1, -2}, x[3] = {}, y[3] = {1, 0, 0}, normal[3] = {1, .4, .2};

  /*!
   * \param[in] degenerate - Velocity gradient with tied baseline anisotropy eigenvalues, so that the EPM cases are
   *            ambiguous and the orientation probes act: in 2D a pure rotation (zero strain, all three tied), in 3D
   *            a rotation plus the axisymmetric strain diag(.5, .5, -1) (the two lowest tied).
   */
  PointState(double omega, unsigned short nDim, bool degenerate = false) {
    scalar[1] = omega;
    if (nDim == 3) {
      for (int i = 11; i > 3; --i) v[i] = v[i-1];
      v[3] = .1;
    }
    if (degenerate) {
      const su2double rotation[3][3] = {{0, 2, 0}, {-2, 0, 0}, {0, 0, 0}};
      const su2double axisymmetric[3][3] = {{.5, .3, 0}, {-.3, .5, 0}, {0, 0, -1}};
      for (int i = 0; i < nDim; ++i)
        for (int j = 0; j < 3; ++j) grad[i+1][j] = nDim == 2 ? rotation[i][j] : axisymmetric[i][j];
    }
  }
  PointState(const PointState&) = delete;
  PointState& operator=(const PointState&) = delete;
};

/*! \brief Scalar viscous flux (flux = true) or SST source, set to the state. */
inline std::unique_ptr<CNumerics> MakeNumerics(bool flux, unsigned short nDim, const CConfig& config,
                                               PointState& s) {
  const su2double constants[] = {.85, 1, .5, .856, .075, .0828, .09, .31, 5.0 / 9, .44, 10};
  std::unique_ptr<CNumerics> numerics;
  if (flux)
    numerics.reset(new CAvgGrad_Flow(nDim, nDim + 2, false, &config));
  else
    numerics.reset(
        new CSourcePieceWise_TurbSST<CEulerVariable::CIndices<unsigned short>>(nDim, 2, constants, 0, 0, &config));
  numerics->SetPrimitive(s.v, s.v);
  numerics->SetScalarVar(s.scalar, s.scalar);
  numerics->SetPrimVarGradient({&s.grad[0][0], 3}, {&s.grad[0][0], 3});
  numerics->SetScalarVarGradient({&s.sg[0][0], 3}, {&s.sg[0][0], 3});
  numerics->SetVorticity(s.vort, s.vort);
  numerics->SetStrainMag(2, 2);
  numerics->SetF1blending(1, 1);
  numerics->SetF2blending(1);
  numerics->SetCrossDiff(0);
  numerics->SetDistance(1, 1);
  numerics->SetVolume(3);
  numerics->SetCoord(s.x, s.y);
  numerics->SetNormal(s.normal);
  numerics->SetTurbKineticEnergy(1, 1);
  return numerics;
}

/*! \brief Residual values and reverse derivatives of the scalar-amplitude replay. */
struct Result {
  std::array<double, 5> value{};
  double parameter = 0, state = 0;
  unsigned long ambiguous = 0; /*!< \brief Ambiguous evaluations counted by the replay. */
};

/*!
 * \brief Scalar-amplitude replay of the scalar flux (flux = true) or the SST source at the fixed state.
 * \return The residual and, in reverse AD, the derivatives of sum((i+1)*residual[i]) with respect to the
 *         amplitude and to one state variable.
 */
inline Result Evaluate(bool flux, int mode, bool legacy, bool active, double amplitude, double omega = 100,
                       unsigned short nDim = 2, double angle = 0.0, unsigned short probe_pair = 3,
                       bool degenerate = false) {
  std::stringstream options(Options(mode, legacy));
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  auto& epm = config.GetEPMExtraction();
  epm.mode = CEPMExtraction::Mode::SCALAR_REPLAY;
  epm.active = active;
  epm.component = mode / 2 + 1;
  epm.permute = mode % 2;
  epm.angle = angle;
  epm.probe_pair = probe_pair;
  epm.amplitude = amplitude;
  PointState s(omega, nDim, degenerate);
  const auto source = MakeNumerics(flux, nDim, config, s);
  AD::Reset();
#ifdef CODI_REVERSE_TYPE
  AD::PreaccEnabled = true;
#endif
  AD::StartRecording();
  AD::RegisterInput(epm.amplitude);
  auto& state = flux ? s.v[1] : s.scalar[0];
  AD::RegisterInput(state);
  const auto residual = source->ComputeResidual(&config);
  Result result;
  su2double objective = 0;
  for (int i = 0; i < (flux ? nDim + 2 : 2); ++i) {
    result.value[i] = SU2_TYPE::GetValue(residual[i]);
    objective += (i + 1) * residual[i];
  }
  AD::RegisterOutput(objective);
  AD::StopRecording();
  SU2_TYPE::SetDerivative(objective, 1);
  AD::ComputeAdjoint();
  result.parameter = SU2_TYPE::GetDerivative(epm.amplitude);
  result.state = SU2_TYPE::GetDerivative(state);
  result.ambiguous = epm.ambiguous;
  AD::ClearAdjoints();
  return result;
}

/*! \brief Edge state of the fused viscous flux. */
template<size_t nDim>
struct FusedState {
  CCompressiblePrimitives<nDim,nDim+9> v;
  MatrixDbl<nDim+1,nDim> grad;
  VectorDbl<nDim> normal, heat;

  FusedState() {
    for (size_t i = 0; i < v.nVar; ++i) v.all(i) = 0;
    v.density() = 2;
    v.laminarVisc() = .01;
    v.eddyVisc() = .3;
    for (size_t i = 0; i < nDim; ++i) {
      v.velocity(i) = .7 / (i+1);
      normal(i) = 1.0 / (i+1);
      heat(i) = 0;
      for (size_t j = 0; j <= nDim; ++j) grad(j,i) = 0;
    }
    grad(1,1) = 2;
    if (nDim == 3) { grad(1,2) = .4; grad(3,0) = .3; }
  }
};

/*!
 * \brief Fused viscous flux at the edge state.
 * \param[in] legacy - Add the classical UQ stress of the configured case.
 * \param[in] active - Add the EPM extraction stress (scalar amplitude or field inputs, after the mode).
 */
template<size_t nDim>
VectorDbl<nDim+2> FusedFlux(const FusedState<nDim>& s, const CConfig& config, bool legacy, bool active) {
  const auto& epm = config.GetEPMExtraction();
  auto tau = stressTensor(s.v.laminarVisc() + (legacy ? Double(0.0) : s.v.eddyVisc()), s.grad);
  if (legacy) addPerturbedRSM(s.v, s.grad, Double(1), tau, epm.component, epm.permute, su2double(1), su2double(1));
  if (active) addEPMExtractionStress(s.v, s.grad, Double(1), tau, config, s.normal, Double(1));
  return viscousFlux<nDim+2>(s.v, tau, s.heat, s.normal);
}

/*! \brief Scalar-amplitude replay of the fused flux, see Evaluate. */
template<size_t nDim>
Result EvaluateFused(int mode, bool legacy, bool active, double amplitude) {
  std::stringstream options("SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= V2003m\n");
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  auto& epm = config.GetEPMExtraction();
  epm.mode = CEPMExtraction::Mode::SCALAR_REPLAY;
  epm.component = mode / 2 + 1;
  epm.permute = mode % 2;
  epm.amplitude = amplitude;
  FusedState<nDim> s;
  AD::Reset();
#ifdef CODI_REVERSE_TYPE
  AD::PreaccEnabled = true;
#endif
  AD::StartRecording();
  AD::RegisterInput(epm.amplitude);
  AD::RegisterInput(s.v.eddyVisc()[0]);
  AD::StartPreacc();
  AD::SetPreaccIn(s.v.eddyVisc()[0]);
  auto flux = FusedFlux(s, config, legacy, active);
  for (size_t i = 0; i < nDim+2; ++i)
    for (size_t lane = 0; lane < Double::Size; ++lane) AD::SetPreaccOut(flux(i)[lane]);
  AD::EndPreacc();
  Result result;
  su2double objective = 0;
  for (size_t i = 0; i < nDim+2; ++i) {
    result.value[i] = SU2_TYPE::GetValue(flux(i)[0]);
    objective += (i+1) * flux(i)[0];
  }
  AD::RegisterOutput(objective);
  AD::StopRecording();
  SU2_TYPE::SetDerivative(objective, 1);
  AD::ComputeAdjoint();
  result.parameter = SU2_TYPE::GetDerivative(epm.amplitude);
  result.state = SU2_TYPE::GetDerivative(s.v.eddyVisc()[0]);
  AD::ClearAdjoints();
  return result;
}

}  // namespace epm_test
