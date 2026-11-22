/*!
 * \file epm_direct_tests.cpp
 * \brief Unit tests for the EPM direct (primal, fixed-amplitude) mode, SST_EPM_DIRECT.
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
#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/CEPMFieldSites.hpp"
#include "epm_test_setup.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <sstream>
#include <string>

/*--- SST_EPM_DIRECT is rejected in builds with OpenMP (the hooks count evaluations in the shared config). ---*/
#ifndef HAVE_OMP
namespace {
using epm_test::FusedFlux;
using epm_test::FusedState;
using epm_test::MakeNumerics;
using epm_test::Options;
using epm_test::PointState;
using Record = CEPMFieldSites::Record;

using OptionMap = std::map<std::string, std::string>;

/*--- Configuration text of the options (each option once: CConfig rejects repeated options). ---*/
std::string Text(const OptionMap& options) {
  std::string text;
  for (const auto& option : options) text += option.first + "= " + option.second + "\n";
  return text;
}

std::string Number(double value) {
  std::stringstream text;
  text << std::setprecision(17) << value;
  return text.str();
}

/*--- Plain SST primal accepted by the EPM direct guards (the numerics of the EPM extraction), with overrides (an
 *    empty value removes the option). ---*/
std::string DirectBase(const OptionMap& overrides = {}) {
  OptionMap options = {{"SOLVER", "RANS"},           {"MATH_PROBLEM", "DIRECT"}, {"REYNOLDS_NUMBER", "1e6"},
                       {"KIND_TURB_MODEL", "SST"},   {"SST_OPTIONS", "V2003m"},  {"SLOPE_LIMITER_FLOW", "NONE"},
                       {"MGLEVEL", "0"},             {"CFL_ADAPT", "NO"},        {"UQ_DELTA_B", "1"},
                       {"UQ_URLX", "1"}};
  for (const auto& option : overrides) {
    if (option.second.empty()) options.erase(option.first);
    else options[option.first] = option.second;
  }
  return Text(options);
}

/*--- Direct mode of the case mode (component mode/2+1, permute mode%2) at the amplitude. ---*/
std::string DirectOptions(int mode, double amplitude) {
  return DirectBase({{"UQ_COMPONENT", std::to_string(mode / 2 + 1)},
                     {"UQ_PERMUTE", mode % 2 ? "YES" : "NO"},
                     {"SST_EPM_DIRECT", "YES"},
                     {"SST_EPM_DIRECT_AMPLITUDE", Number(amplitude)}});
}

/*--- Classical UQ (SST_OPTIONS UQ) of the case mode with the under-relaxation UQ_URLX= urlx. ---*/
std::string ClassicalOptions(int mode, double urlx) {
  return DirectBase({{"SST_OPTIONS", "(V2003m, UQ)"},
                     {"UQ_COMPONENT", std::to_string(mode / 2 + 1)},
                     {"UQ_PERMUTE", mode % 2 ? "YES" : "NO"},
                     {"UQ_URLX", Number(urlx)}});
}

using Values = std::array<double, 5>;

/*--- Residual of the scalar viscous flux (flux = true) or the SST source at the fixed point state. ---*/
Values Residual(bool flux, unsigned short nDim, double omega, const std::string& text) {
  std::stringstream options(text);
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  PointState s(omega, nDim);
  const auto numerics = MakeNumerics(flux, nDim, config, s);
  const auto residual = numerics->ComputeResidual(&config);
  Values values{};
  for (int i = 0; i < (flux ? nDim + 2 : 2); ++i) values[i] = SU2_TYPE::GetValue(residual[i]);
  return values;
}

/*--- Record of the field extraction at the same state: one recording site, as stored by the field hooks. ---*/
Record FieldRecord(bool flux, unsigned short nDim, double omega) {
  std::stringstream options(DirectBase());
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  auto& epm = config.GetEPMExtraction();
  epm.active = true;
  epm.mode = CEPMExtraction::Mode::FIELD;
  epm.field = std::make_shared<CEPMFieldSites>();
  PointState s(omega, nDim);
  const auto numerics = MakeNumerics(flux, nDim, config, s);
  epm.cursor.kind = flux ? CEPMExtraction::SiteKind::EDGE : CEPMExtraction::SiteKind::NODE;
  epm.cursor.a = 3;
  epm.cursor.b = flux ? 7 : 0;
  epm.cursor.set = true;
  numerics->ComputeResidual(&config);
  REQUIRE(epm.field->Size() == 1);
  return epm.field->Data(0);
}

/*--- Raw (unclipped) production of the classical target of the case mode at the point state. ---*/
double TargetProduction(int mode, unsigned short nDim, double omega) {
  PointState s(omega, nDim);
  passivedouble grad[3][3] = {}, target[3][3];
  for (int i = 0; i < nDim; ++i)
    for (int j = 0; j < nDim; ++j) grad[i][j] = SU2_TYPE::GetValue(s.grad[i + 1][j]);
  const su2double density = s.v[nDim + 2], viscosity = s.v[nDim + 6], k = s.scalar[0];
  bool ambiguous = false;
  CNumerics::EPMCaseTarget(nDim, grad, density, viscosity, k, mode / 2 + 1, mode % 2, 0.0, 3, target, ambiguous);
  return CNumerics::ComputeReynoldsStressProduction(nDim, target, grad, SU2_TYPE::GetValue(density));
}

/*--- Tolerance of residual comparisons: round-off of residuals of size scale (nondimensional, O(1..100)). ---*/
double Margin(const Values& a, const Values& b) {
  double scale = 1;
  for (int i = 0; i < 5; ++i) scale = std::max({scale, std::fabs(a[i]), std::fabs(b[i])});
  return 1e-12 * scale;
}
}  // namespace

TEST_CASE("EPM direct options select the fixed-amplitude spec path", "[EPM][EPMDirect]") {
  {
    std::stringstream options(DirectBase());
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    const auto& epm = config.GetEPMExtraction();
    CHECK_FALSE(epm.direct);
    CHECK_FALSE(epm.active);
    CHECK_FALSE(epm.enabled);
    CHECK(epm.amplitude == 0.0);
    CHECK(config.GetEPMDirectError().empty());
  }
  for (int mode = 0; mode < 5; ++mode) {
    std::stringstream options(DirectOptions(mode, .25));
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    const auto& epm = config.GetEPMExtraction();
    CHECK(epm.direct);
    CHECK_FALSE(epm.enabled);
    CHECK_FALSE(epm.extract_only);
    CHECK(epm.active);
    /*--- The scalar hooks (no site cursors, no field store), with the canonical orientation of the case. ---*/
    CHECK(epm.mode == CEPMExtraction::Mode::SCALAR_REPLAY);
    CHECK(epm.field == nullptr);
    CHECK_FALSE(EPMFieldCursorOn(&config));
    CHECK(epm.component == mode / 2 + 1);
    CHECK(epm.permute == (mode % 2 == 1));
    CHECK(epm.angle == 0.0);
    CHECK(epm.probe_pair == 3);
    CHECK(SU2_TYPE::GetValue(epm.amplitude) == .25);
    CHECK(SU2_TYPE::GetValue(epm.direct_amplitude) == .25);
    CHECK_FALSE(config.GetSSTParsedOptions().uq);
    CHECK(config.GetSSTParsedOptions().production == SST_OPTIONS::NONE);
    CHECK(config.GetEPMDirectError().empty());
  }
  /*--- The amplitude bounds are accepted. ---*/
  for (double amplitude : {-1.0, 0.0, 1.0}) {
    std::stringstream options(DirectOptions(1, amplitude));
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    CHECK(SU2_TYPE::GetValue(config.GetEPMExtraction().amplitude) == amplitude);
  }
}

TEST_CASE("EPM direct guards reject unsupported configurations", "[EPM][EPMDirect]") {
  /*--- SU2_MPI::Error aborts, so the guard is checked on configurations that are valid without the direct mode,
   *    with the direct switch set afterwards. The string is the message CConfig raises at postprocessing. ---*/
  const auto error = [](const std::string& text, double amplitude = .25, bool enabled = false) {
    std::stringstream options(text);
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    auto& epm = config.GetEPMExtraction();
    epm.direct = true;
    epm.direct_amplitude = amplitude;
    if (enabled) epm.enabled = true;
    return config.GetEPMDirectError();
  };
  CHECK(error(DirectBase()).empty());

  /*--- Classical UQ in SST_OPTIONS (the case comes from UQ_COMPONENT and UQ_PERMUTE). ---*/
  CHECK(error(DirectBase({{"SST_OPTIONS", "(V2003m, UQ)"}})).find("V2003m") != std::string::npos);
  /*--- Other SST versions and modifications, other turbulence models. ---*/
  for (const char* sst : {"V1994m", "(V2003m, SUSTAINING)", "(V2003m, VORTICITY)", "(V2003m, DIMENSIONLESS_LIMIT)"})
    CHECK_FALSE(error(DirectBase({{"SST_OPTIONS", sst}})).empty());
  CHECK_FALSE(error(DirectBase({{"KIND_TURB_MODEL", "SA"}, {"SST_OPTIONS", ""}})).empty());
#ifdef CODI_REVERSE_TYPE
  /*--- Discrete adjoint (only parsed by a reverse-AD build). ---*/
  CHECK(error(DirectBase({{"MATH_PROBLEM", "DISCRETE_ADJOINT"}})).find("MATH_PROBLEM= DIRECT") != std::string::npos);
#endif
  /*--- Exclusive with the adjoint extraction. ---*/
  CHECK(error(DirectBase(), .25, true).find("exclusive") != std::string::npos);
  CHECK(error(DirectBase({{"SST_EPM_EXTRACTION", "SCALAR_REPLAY"}})).find("SST_EPM_EXTRACTION") != std::string::npos);

  /*--- Full classical target controls; the amplitude is SST_EPM_DIRECT_AMPLITUDE. ---*/
  CHECK(error(DirectBase({{"UQ_URLX", "0.5"}})).find("UQ_URLX") != std::string::npos);
  CHECK(error(DirectBase({{"UQ_DELTA_B", "0.5"}})).find("UQ_DELTA_B") != std::string::npos);
  /*--- UQ_URLX is unused by the direct mode: unset (default 0.1) is accepted. ---*/
  CHECK(error(DirectBase({{"UQ_URLX", ""}})).empty());
  /*--- One of the five cases. ---*/
  CHECK(error(DirectBase({{"UQ_COMPONENT", "3"}, {"UQ_PERMUTE", "YES"}})).find("UQ_COMPONENT") != std::string::npos);
  CHECK(error(DirectBase({{"UQ_COMPONENT", "4"}})).find("UQ_COMPONENT") != std::string::npos);
  CHECK(error(DirectBase({{"UQ_COMPONENT", "0"}})).find("UQ_COMPONENT") != std::string::npos);
  CHECK(error(DirectBase({{"UQ_COMPONENT", "2"}, {"UQ_PERMUTE", "YES"}})).empty());

  /*--- Amplitude in [-1, 1]. ---*/
  for (double amplitude : {1.5, -1.5, std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()})
    CHECK(error(DirectBase(), amplitude).find("SST_EPM_DIRECT_AMPLITUDE") != std::string::npos);

  /*--- Numerics and physics outside the configuration of the EPM extraction. ---*/
  const OptionMap unsupported = {{"USE_VECTORIZATION", "YES"},  {"MGLEVEL", "1"},
                                 {"CFL_ADAPT", "YES"},          {"MUSCL_TURB", "YES"},
                                 {"SLOPE_LIMITER_FLOW", "VENKATAKRISHNAN"}, {"TIME_DISCRE_FLOW", "EULER_EXPLICIT"},
                                 {"BODY_FORCE", "YES"},         {"AXISYMMETRIC", "YES"}};
  for (const auto& option : unsupported) CHECK_FALSE(error(DirectBase({option})).empty());
}

TEST_CASE("EPM direct residual follows the extraction path", "[EPM][EPMDirect]") {
  for (unsigned short nDim : {2, 3})
    for (bool flux : {true, false})
      for (double omega : {.01, 100.0}) {
        const int n = flux ? nDim + 2 : 2;
        const auto base = Residual(flux, nDim, omega, DirectBase());
        for (int mode = 0; mode < 5; ++mode) {
          /*--- Classical endpoint (UQ, UQ_URLX= 1) and the field record of the extraction at the same state. ---*/
          const auto end = Residual(flux, nDim, omega, ClassicalOptions(mode, 1));
          const auto record = FieldRecord(flux, nDim, omega);
          const auto zero = Residual(flux, nDim, omega, DirectOptions(mode, 0));

          /*--- A = 0 is the plain SST residual, bit for bit. ---*/
          for (int i = 0; i < n; ++i) CHECK(zero[i] == base[i]);

          for (double amplitude : {-.5, .25, .5, 1.0}) {
            const auto direct = Residual(flux, nDim, omega, DirectOptions(mode, amplitude));
            const auto margin = Margin(base, end);

            /*--- (a) R(A) = (1-A) R0 + A Rm. ---*/
            for (int i = 0; i < n; ++i)
              CHECK(direct[i] == Approx((1 - amplitude) * base[i] + amplitude * end[i]).margin(margin));

            /*--- (c) dR/dA (secant; R is affine in A) equals the stored dtau / dpk, dpw of the extraction. ---*/
            Values derivative{};
            if (flux) {
              /*--- Projected viscous flux of the stress change: momentum dtau.n, energy (dtau.n).v (no heat). ---*/
              const PointState s(omega, nDim);
              for (int i = 0; i < nDim; ++i)
                for (int j = 0; j < nDim; ++j) {
                  const double t = record.dtau[mode][j * 3 + i] * SU2_TYPE::GetValue(s.normal[j]);
                  derivative[1 + i] += t;
                  derivative[nDim + 1] += t * SU2_TYPE::GetValue(s.v[1 + i]);
                }
            } else {
              /*--- Production changes times the volume (3) of the point state. ---*/
              derivative[0] = record.dpk[mode] * 3;
              derivative[1] = record.dpw[mode] * 3;
            }
            for (int i = 0; i < n; ++i)
              CHECK((direct[i] - zero[i]) / amplitude == Approx(derivative[i]).margin(margin / std::fabs(amplitude)));
          }

          /*--- SWAP: the target production is negative, so the classical solver path at UQ_URLX= A (clip after
           *    the blend) differs from the direct path (blend of the clipped endpoints). ---*/
          if (!flux && mode % 2 == 1) {
            CHECK(TargetProduction(mode, nDim, omega) < 0);
            const auto direct = Residual(flux, nDim, omega, DirectOptions(mode, .5));
            const auto classical = Residual(flux, nDim, omega, ClassicalOptions(mode, .5));
            CHECK(std::fabs(direct[0] - classical[0]) > 1e-3);
            /*--- The direct k production is (1-A) clip(P0): the clipped target production is 0. ---*/
            CHECK(record.dpk[mode] == Approx(-record.pk0));
          }
        }
      }
}

namespace {
template<size_t nDim>
void CheckFusedDirect() {
  const FusedState<nDim> s;
  std::stringstream off_options(DirectBase());
  CConfig off(off_options, SU2_COMPONENT::SU2_CFD, false);
  const auto base = FusedFlux(s, off, false, false);
  for (int mode = 0; mode < 5; ++mode) {
    /*--- Classical endpoint: FusedFlux takes the case of the classical stress from the extraction state. ---*/
    std::stringstream end_options(Options(mode, true));
    CConfig classical(end_options, SU2_COMPONENT::SU2_CFD, false);
    classical.GetEPMExtraction().component = mode / 2 + 1;
    classical.GetEPMExtraction().permute = mode % 2;
    const auto end = FusedFlux(s, classical, true, false);

    /*--- Stored stress change of the case: the correction the field hook stores (CNumerics::EPMFieldStressHook). ---*/
    passivedouble velgrad[nDim][nDim], dtau[3][3];
    for (size_t i = 0; i < nDim; ++i)
      for (size_t j = 0; j < nDim; ++j) velgrad[i][j] = SU2_TYPE::GetValue(s.grad(i+1,j)[0]);
    bool ambiguous = false;
    CNumerics::EPMCaseStressCorrection(nDim, velgrad, s.v.density()[0], s.v.eddyVisc()[0], su2double(1),
                                       mode / 2 + 1, mode % 2, 0.0, 3, dtau, ambiguous);
    MatrixDbl<nDim> dtauMatrix;
    VectorDbl<nDim> noHeat;
    for (size_t i = 0; i < nDim; ++i) {
      noHeat(i) = 0.0;
      for (size_t j = 0; j < nDim; ++j) dtauMatrix(i,j) = dtau[i][j];
    }
    const auto derivative = viscousFlux<nDim+2>(s.v, dtauMatrix, noHeat, s.normal);

    /*--- The fused field hook stores exactly this correction (it records one lane per site: SIMD size 1). ---*/
    if (Double::Size == 1) {
      std::stringstream field_options(Options(0, false));
      CConfig field(field_options, SU2_COMPONENT::SU2_CFD, false);
      auto& epm = field.GetEPMExtraction();
      epm.active = true;
      epm.mode = CEPMExtraction::Mode::FIELD;
      epm.field = std::make_shared<CEPMFieldSites>();
      epm.cursor.kind = CEPMExtraction::SiteKind::EDGE;
      epm.cursor.a = 3;
      epm.cursor.b = 7;
      epm.cursor.set = true;
      FusedFlux(s, field, false, true);
      REQUIRE(epm.field->Size() == 1);
      for (size_t i = 0; i < nDim; ++i)
        for (size_t j = 0; j < nDim; ++j) CHECK(epm.field->Data(0).dtau[mode][i*3+j] == Approx(dtau[i][j]).margin(1e-14));
    }

    std::stringstream zero_options(DirectOptions(mode, 0));
    CConfig zeroConfig(zero_options, SU2_COMPONENT::SU2_CFD, false);
    REQUIRE(zeroConfig.GetEPMExtraction().active);
    const auto zero = FusedFlux(s, zeroConfig, false, true);
    for (size_t i = 0; i < nDim+2; ++i) CHECK(SU2_TYPE::GetValue(zero(i)[0]) == SU2_TYPE::GetValue(base(i)[0]));

    for (double amplitude : {-.5, .25, 1.0}) {
      std::stringstream options(DirectOptions(mode, amplitude));
      CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
      const auto flux = FusedFlux(s, config, false, true);
      for (size_t i = 0; i < nDim+2; ++i) {
        const double b = SU2_TYPE::GetValue(base(i)[0]), e = SU2_TYPE::GetValue(end(i)[0]);
        const double f = SU2_TYPE::GetValue(flux(i)[0]), z = SU2_TYPE::GetValue(zero(i)[0]);
        const double margin = 1e-12 * std::max({1.0, std::fabs(b), std::fabs(e)});
        CHECK(f == Approx((1 - amplitude) * b + amplitude * e).margin(margin));
        CHECK((f - z) / amplitude == Approx(SU2_TYPE::GetValue(derivative(i)[0])).margin(margin / std::fabs(amplitude)));
      }
    }
  }
}
}  // namespace

TEST_CASE("EPM direct fused edge flux follows the extraction path", "[EPM][EPMDirect]") {
  CheckFusedDirect<2>();
  CheckFusedDirect<3>();
}
#endif
