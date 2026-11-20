/*!
 * \file epm_field_tests.cpp
 * \brief Unit tests for the EPM field-sensitivity options and extraction state.
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

#include <cmath>
#include <memory>
#include <sstream>
#include <string>

#if defined(CODI_REVERSE_TYPE) && !defined(HAVE_OMP)
namespace {
/*--- Options of the constrained coupled adjoint accepted by CConfig for SST_EPM_POSTPROCESS. ---*/
std::string EPMBaseOptions() {
  return "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\nKIND_TURB_MODEL= SST\nSST_OPTIONS= V2003m\n"
         "MATH_PROBLEM= DISCRETE_ADJOINT\nOBJECTIVE_FUNCTION= DRAG\n"
         "FROZEN_VISC_DISC= NO\nFROZEN_LIMITER_DISC= NO\nINCONSISTENT_DISC= NO\n"
         "SLOPE_LIMITER_FLOW= NONE\nMGLEVEL= 0\nCFL_ADAPT= NO\n"
         "UQ_DELTA_B= 1\nUQ_URLX= 1\nSST_EPM_POSTPROCESS= YES\n";
}
}  // namespace

TEST_CASE("EPM field options parse", "[EPMField]") {
  {
    std::stringstream options(EPMBaseOptions());
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    const auto& epm = config.GetEPMExtraction();
    CHECK(epm.enabled);
    CHECK(epm.mode == CEPMExtraction::Mode::FIELD);
    CHECK_FALSE(epm.extract_only);
    CHECK_FALSE(epm.cursor.set);
    CHECK(epm.field == nullptr);
  }
  {
    std::stringstream options(EPMBaseOptions() + "SST_EPM_EXTRACTION= SCALAR_REPLAY\n");
    CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
    CHECK(config.GetEPMExtraction().mode == CEPMExtraction::Mode::SCALAR_REPLAY);
    CHECK_FALSE(config.GetEPMExtraction().extract_only);
  }
}

TEST_CASE("EPM extract-only configuration is accepted", "[EPMField]") {
  /*--- Rejections (no restart, ITER != 1, non-residual criterion, equal adjoint file names) call
   *    SU2_MPI::Error, which aborts the process; they are covered by the Python smoke runner. ---*/
  std::stringstream options(EPMBaseOptions() +
                            "SST_EPM_EXTRACT_ONLY= YES\nRESTART_SOL= YES\nITER= 1\n"
                            "CONV_FIELD= RMS_ADJ_DENSITY\n"
                            "SOLUTION_ADJ_FILENAME= solution_adj\nRESTART_ADJ_FILENAME= restart_adj\n");
  CConfig config(options, SU2_COMPONENT::SU2_CFD, false);
  const auto& epm = config.GetEPMExtraction();
  CHECK(epm.enabled);
  CHECK(epm.extract_only);
  CHECK(epm.mode == CEPMExtraction::Mode::FIELD);
  CHECK(config.GetRestart());
  CHECK(config.GetnInner_Iter() == 1);
}
#endif

using EPMSiteKind = CEPMExtraction::SiteKind;

TEST_CASE("Site keys are unique and edge order independent", "[EPMField]") {
  CEPMFieldSites sites;
  const auto edge = sites.Site(EPMSiteKind::EDGE, 7, 3);
  CHECK(sites.Site(EPMSiteKind::EDGE, 3, 7) == edge);
  CHECK(sites.Site(EPMSiteKind::NODE, 3, 0) != edge);
  CHECK(sites.Size() == 2);
  /*--- Edge keys are stored as (min, max) of the global ids. ---*/
  CHECK(sites.Data(edge).key_a == 3);
  CHECK(sites.Data(edge).key_b == 7);
  CHECK(sites.Data(edge).kind == static_cast<int>(EPMSiteKind::EDGE));
}

TEST_CASE("Freeze rejects new sites", "[EPMField]") {
  CEPMFieldSites sites;
  const auto edge = sites.Site(EPMSiteKind::EDGE, 3, 7);
  sites.Freeze();
  CHECK(sites.Site(EPMSiteKind::NODE, 99, 0) == CEPMFieldSites::npos);
  CHECK(sites.Site(EPMSiteKind::EDGE, 7, 3) == edge);
  CHECK(sites.Size() == 1);
}

#ifdef CODI_REVERSE_TYPE
TEST_CASE("Repeated site accumulates", "[EPMField]") {
  CEPMFieldSites sites;
  const auto s = sites.Site(EPMSiteKind::NODE, 5, 0);
  sites.Freeze();
  sites.Data(s).uses = 2;

  AD::Reset();
  AD::StartRecording();
  sites.RegisterInputs();
  su2double y = 2 * sites.PkInput(s) + 3 * sites.PkInput(s);
  AD::RegisterOutput(y);
  AD::StopRecording();
  SU2_TYPE::SetDerivative(y, 1);
  AD::ComputeAdjoint();
  sites.ExtractAdjoints();
  const auto record = sites.Data(s);

  /*--- Leave the tape clean for the other tests before checking. ---*/
  AD::ClearAdjoints();
  sites.Clear();
  AD::Reset();

  CHECK(record.S_pk == Approx(5.0));
  CHECK(record.S_pw == 0.0);
  CHECK(record.uses == 2.0);
}
#endif

TEST_CASE("Reset records keeps keys and inputs, clears the rest", "[EPMField]") {
  CEPMFieldSites sites;
  const auto s = sites.Site(EPMSiteKind::EDGE, 7, 3);
  sites.Freeze();
  auto* input = sites.TauInput(s);
  {
    auto& record = sites.Data(s);
    record.uses = 3;
    record.rho = 2;
    record.grad[4] = 1;
    record.cap_active = 1;
    record.S_tau[1] = 4;
    record.S_pw = 5;
    record.dtau[2][4] = 6;
    record.dpk[1] = 7;
  }
  sites.ResetRecords();
  const auto& record = sites.Data(s);
  CHECK(record.uses == 0);
  CHECK(std::isnan(record.rho));
  CHECK(std::isnan(record.grad[4]));
  CHECK(std::isnan(record.cap_active));
  CHECK(record.S_tau[1] == 0);
  CHECK(record.S_pw == 0);
  CHECK(record.dtau[2][4] == 0);
  CHECK(record.dpk[1] == 0);
  CHECK(record.kind == static_cast<int>(EPMSiteKind::EDGE));
  CHECK(record.key_a == 3);
  CHECK(record.key_b == 7);
  CHECK(sites.Size() == 1);
  CHECK(sites.Frozen());
  CHECK(sites.Site(EPMSiteKind::EDGE, 3, 7) == s);
  CHECK(sites.TauInput(s) == input);
}

namespace {
using epm_test::Evaluate;
using epm_test::EvaluateFused;
using epm_test::FusedFlux;
using epm_test::FusedState;
using epm_test::MakeNumerics;
using epm_test::Options;
using epm_test::PointState;
using Record = CEPMFieldSites::Record;

/*--- FIELD-mode extraction with an empty site store, as the driver sets it before the discovery pass. ---*/
struct FieldCase {
  std::stringstream options{Options(0, false)};
  CConfig config{options, SU2_COMPONENT::SU2_CFD, false};
  CEPMExtraction& epm = config.GetEPMExtraction();
  FieldCase() {
    epm.active = true;
    epm.mode = CEPMExtraction::Mode::FIELD;
    epm.field = std::make_shared<CEPMFieldSites>();
  }
};

void SetCursor(CEPMExtraction& epm, EPMSiteKind kind, unsigned long a, unsigned long b) {
  epm.cursor.kind = kind;
  epm.cursor.a = a;
  epm.cursor.b = b;
  epm.cursor.set = true;
}

/*--- Edge (3,7) for the fluxes, node 3 for the source. ---*/
void SetCursor(CEPMExtraction& epm, bool flux) {
  if (flux) SetCursor(epm, EPMSiteKind::EDGE, 3, 7);
  else SetCursor(epm, EPMSiteKind::NODE, 3, 0);
}

std::array<double, 5> Values(const CNumerics::ResidualType<>& residual, int n) {
  std::array<double, 5> values{};
  for (int i = 0; i < n; ++i) values[i] = SU2_TYPE::GetValue(residual[i]);
  return values;
}

template<size_t nDim>
void CheckFusedZero() {
  std::stringstream options(Options(0, false));
  CConfig off(options, SU2_COMPONENT::SU2_CFD, false);
  const FusedState<nDim> s;
  const auto base = FusedFlux(s, off, false, false);

  FieldCase field;
  SetCursor(field.epm, true);
  const auto flux = FusedFlux(s, field.config, false, true);
  for (size_t i = 0; i < nDim+2; ++i) CHECK(SU2_TYPE::GetValue(flux(i)[0]) == SU2_TYPE::GetValue(base(i)[0]));
  CHECK_FALSE(field.epm.cursor.set);
  CHECK(field.epm.field->Size() == 1);
  const auto& record = field.epm.field->Data(0);
  CHECK(record.uses == 1);
  CHECK(record.rho == Approx(2));
  CHECK(record.mu_t == Approx(.3));
  CHECK(record.area == Approx(1));
  CHECK(field.epm.evaluations == 1);
  CHECK(field.epm.stress_evaluations == 1);
}
}  // namespace

TEST_CASE("Field stress hook: zero input leaves flux unchanged", "[EPMField]") {
  for (unsigned short nDim : {2, 3})
    for (bool flux : {true, false})
      for (double omega : {.01, 100.0}) {
        const int n = flux ? nDim + 2 : 2;

        /*--- Feature off. ---*/
        std::stringstream off_options(Options(0, false));
        CConfig off(off_options, SU2_COMPONENT::SU2_CFD, false);
        PointState off_state(omega, nDim);
        const auto base = Values(MakeNumerics(flux, nDim, off, off_state)->ComputeResidual(&off), n);

        /*--- Scalar replay at zero amplitude, for the counters. ---*/
        std::stringstream replay_options(Options(0, false));
        CConfig replay(replay_options, SU2_COMPONENT::SU2_CFD, false);
        replay.GetEPMExtraction().active = true;
        replay.GetEPMExtraction().mode = CEPMExtraction::Mode::SCALAR_REPLAY;
        PointState replay_state(omega, nDim);
        MakeNumerics(flux, nDim, replay, replay_state)->ComputeResidual(&replay);

        FieldCase field;
        PointState state(omega, nDim);
        const auto numerics = MakeNumerics(flux, nDim, field.config, state);
        SetCursor(field.epm, flux);
        const auto values = Values(numerics->ComputeResidual(&field.config), n);

        for (int i = 0; i < n; ++i) CHECK(values[i] == base[i]);
        CHECK_FALSE(field.epm.cursor.set);
        CHECK(field.epm.evaluations == replay.GetEPMExtraction().evaluations);
        CHECK(field.epm.stress_evaluations == replay.GetEPMExtraction().stress_evaluations);
        CHECK(field.epm.evaluations == 1);
        CHECK(field.epm.stress_evaluations == (flux ? 1u : 0u));
        CHECK(field.epm.field->Size() == 1);
        const auto& record = field.epm.field->Data(0);
        CHECK(record.kind == static_cast<int>(flux ? EPMSiteKind::EDGE : EPMSiteKind::NODE));
        CHECK(record.uses == 1);
        CHECK(record.rho == Approx(2));
        CHECK(record.mu_t == Approx(.3));
        CHECK(record.k == Approx(1));
        if (flux) {
          CHECK(std::isfinite(record.area));
          CHECK(std::isnan(record.volume));
        } else {
          CHECK(record.volume == Approx(3));
          CHECK(record.cap_active == (omega < 1 ? 1.0 : 0.0));
          CHECK(record.floor_active == 0.0);
        }
      }

  /*--- The fused kernel records one lane per site (reverse-AD SIMD size). ---*/
  if (Double::Size == 1) {
    CheckFusedZero<2>();
    CheckFusedZero<3>();
  }
}

TEST_CASE("Stale cursor is an error", "[EPMField]") {
  /*--- The hooks call SU2_MPI::Error (an abort) when the site index is npos. ---*/
  FieldCase field;
  CHECK(CNumerics::EPMFieldSiteIndex(&field.config) == CEPMFieldSites::npos);
  CHECK(field.epm.field->Size() == 0);

  SetCursor(field.epm, EPMSiteKind::EDGE, 7, 3);
  const auto site = CNumerics::EPMFieldSiteIndex(&field.config);
  CHECK(site == 0);
  CHECK_FALSE(field.epm.cursor.set);
  /*--- A cursor serves one call. ---*/
  CHECK(CNumerics::EPMFieldSiteIndex(&field.config) == CEPMFieldSites::npos);

  field.epm.field->Freeze();
  SetCursor(field.epm, EPMSiteKind::NODE, 9, 0);
  CHECK(CNumerics::EPMFieldSiteIndex(&field.config) == CEPMFieldSites::npos);
  CHECK_FALSE(field.epm.cursor.set);
  SetCursor(field.epm, EPMSiteKind::EDGE, 3, 7);
  CHECK(CNumerics::EPMFieldSiteIndex(&field.config) == site);

  field.epm.field.reset();
  SetCursor(field.epm, EPMSiteKind::EDGE, 3, 7);
  CHECK(CNumerics::EPMFieldSiteIndex(&field.config) == CEPMFieldSites::npos);
  CHECK_FALSE(field.epm.cursor.set);
}

#ifdef CODI_REVERSE_TYPE
namespace {
/*--- Driver order: discovery (passive), Freeze, ResetRecords, RegisterInputs in the recording, evaluation,
 *    reverse sweep, ExtractAdjoints. Objective: sum((i+1)*residual[i]), as in the scalar replay. ---*/
Record FieldAdjoint(bool flux, unsigned short nDim, double omega) {
  FieldCase field;
  auto& sites = *field.epm.field;
  PointState state(omega, nDim);
  const auto numerics = MakeNumerics(flux, nDim, field.config, state);
  SetCursor(field.epm, flux);
  numerics->ComputeResidual(&field.config);
  sites.Freeze();
  sites.ResetRecords();

  AD::Reset();
  AD::PreaccEnabled = true;
  AD::StartRecording();
  sites.RegisterInputs();
  SetCursor(field.epm, flux);
  const auto residual = numerics->ComputeResidual(&field.config);
  su2double objective = 0;
  for (int i = 0; i < (flux ? nDim + 2 : 2); ++i) objective += (i + 1) * residual[i];
  AD::RegisterOutput(objective);
  AD::StopRecording();
  SU2_TYPE::SetDerivative(objective, 1);
  AD::ComputeAdjoint();
  sites.ExtractAdjoints();
  const auto record = sites.Data(0);

  AD::ClearAdjoints();
  sites.Clear();
  AD::Reset();
  return record;
}

template<size_t nDim>
Record FusedFieldAdjoint() {
  FieldCase field;
  auto& sites = *field.epm.field;
  const FusedState<nDim> s;
  SetCursor(field.epm, true);
  FusedFlux(s, field.config, false, true);
  sites.Freeze();
  sites.ResetRecords();

  AD::Reset();
  AD::PreaccEnabled = true;
  AD::StartRecording();
  sites.RegisterInputs();
  AD::StartPreacc();
  SetCursor(field.epm, true);
  auto flux = FusedFlux(s, field.config, false, true);
  for (size_t i = 0; i < nDim+2; ++i) AD::SetPreaccOut(flux(i)[0]);
  AD::EndPreacc();
  su2double objective = 0;
  for (size_t i = 0; i < nDim+2; ++i) objective += (i+1) * flux(i)[0];
  AD::RegisterOutput(objective);
  AD::StopRecording();
  SU2_TYPE::SetDerivative(objective, 1);
  AD::ComputeAdjoint();
  sites.ExtractAdjoints();
  const auto record = sites.Data(0);

  AD::ClearAdjoints();
  sites.Clear();
  AD::Reset();
  return record;
}

double StressDot(const Record& record, int m) {
  double dot = 0;
  for (int i = 0; i < CEPMFieldSites::kTau; ++i) dot += record.S_tau[i] * record.dtau[m][i];
  return dot;
}

double SourceDot(const Record& record, int m) { return record.S_pk * record.dpk[m] + record.S_pw * record.dpw[m]; }

template<size_t nDim>
void CheckFusedDot() {
  const auto record = FusedFieldAdjoint<nDim>();
  CHECK(record.uses == 1);
  bool nonzero = false;
  for (int m = 0; m < CEPMFieldSites::kCases; ++m) {
    const auto reference = EvaluateFused<nDim>(m, false, true, 0).parameter;
    nonzero |= reference != 0;
    CHECK(StressDot(record, m) == Approx(reference).epsilon(1e-12));
  }
  CHECK(nonzero);
}
}  // namespace

TEST_CASE("Field stress hook: input FD matches adjoint", "[EPMField]") {
  for (unsigned short nDim : {2, 3}) {
    FieldCase field;
    auto& sites = *field.epm.field;
    PointState state(100, nDim);
    const auto numerics = MakeNumerics(true, nDim, field.config, state);
    /*--- Seeded output: x-momentum flux, which depends on tau[0][1] through the normal. ---*/
    const auto output = [&]() {
      SetCursor(field.epm, true);
      return SU2_TYPE::GetValue(numerics->ComputeResidual(&field.config)[1]);
    };
    output();
    sites.Freeze();
    const size_t site = 0;
    const double h = 1e-6;
    sites.TauInput(site)[1] = h;
    const double plus = output();
    sites.TauInput(site)[1] = -h;
    const double minus = output();
    sites.TauInput(site)[1] = 0.0;
    const double fd = (plus - minus) / (2 * h);
    sites.ResetRecords();

    AD::Reset();
    AD::PreaccEnabled = true;
    AD::StartRecording();
    sites.RegisterInputs();
    SetCursor(field.epm, true);
    su2double objective = numerics->ComputeResidual(&field.config)[1];
    AD::RegisterOutput(objective);
    AD::StopRecording();
    SU2_TYPE::SetDerivative(objective, 1);
    AD::ComputeAdjoint();
    sites.ExtractAdjoints();
    const auto record = sites.Data(site);
    AD::ClearAdjoints();
    sites.Clear();
    AD::Reset();

    CHECK(fd != 0);
    CHECK(record.S_tau[0 * 3 + 1] == Approx(fd).epsilon(1e-7));
    CHECK(record.uses == 1);
  }
}

TEST_CASE("Field dot product equals amplitude derivative", "[EPMField]") {
  for (unsigned short nDim : {2, 3}) {
    /*--- Scalar viscous flux. ---*/
    {
      const auto record = FieldAdjoint(true, nDim, 100);
      CHECK(record.uses == 1);
      bool nonzero = false;
      for (int m = 0; m < CEPMFieldSites::kCases; ++m) {
        const auto reference = Evaluate(true, m, false, true, 0, 100, nDim).parameter;
        nonzero |= reference != 0;
        CHECK(StressDot(record, m) == Approx(reference).epsilon(1e-12));
      }
      CHECK(nonzero);
    }
    /*--- SST source, with the production cap active (small omega) and inactive. ---*/
    for (double omega : {.01, 100.0}) {
      const auto record = FieldAdjoint(false, nDim, omega);
      CHECK(record.uses == 1);
      CHECK(record.cap_active == (omega < 1 ? 1.0 : 0.0));
      bool nonzero = false;
      for (int m = 0; m < CEPMFieldSites::kCases; ++m) {
        const auto reference = Evaluate(false, m, false, true, 0, omega, nDim).parameter;
        nonzero |= reference != 0;
        CHECK(SourceDot(record, m) == Approx(reference).epsilon(1e-12));
      }
      CHECK(nonzero);
    }
  }
  /*--- Fused edge flux. ---*/
  CheckFusedDot<2>();
  CheckFusedDot<3>();
}
#endif
