/*!
 * \file CMixingPlaneInterface.cpp
 * \brief Declaration and inlines of the class to transfer average solver variables
 *        needed for MixingPlane computation from a generic zone into another one.
 * \author J. Kelly, S. Vitale
 * \version 8.4.0 "Harrier"
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

#include "../../../include/interfaces/cfd/CMixingPlaneInterface.hpp"
#include "../../../include/solvers/MixingMeanFlux.hpp"
#include "../../../Common/include/interface_interpolation/CInterpolator.hpp"
#include "../../../../Common/include/CConfig.hpp"
#include "../../../../Common/include/geometry/CGeometry.hpp"
#include "../../../include/solvers/CSolver.hpp"

CMixingPlaneInterface::CMixingPlaneInterface(unsigned short val_nVar, unsigned short val_nConst){
  nVar = val_nVar; // Solver vars
  nMixingVars = 8; // 8 solver vars in turbo MP
  Donor_Variable     = new su2double[nMixingVars]();
  Target_Variable    = new su2double[nMixingVars]();
  InterfaceType      = ENUM_TRANSFER::MIXING_PLANE;
}

void CMixingPlaneInterface::SetSpanWiseLevels(const CConfig *donor_config, const CConfig *target_config){

  unsigned short iSpan;
  nSpanMaxAllZones = donor_config->GetnSpanMaxAllZones();


  SpanValueCoeffTarget = new su2double[target_config->GetnSpanWiseSections() + 1];
  SpanLevelDonor       = new unsigned short[target_config->GetnSpanWiseSections() + 1];


  for (iSpan = 0; iSpan < target_config->GetnSpanWiseSections() + 1;iSpan++){
    SpanValueCoeffTarget[iSpan] = 0.0;
    SpanLevelDonor[iSpan]       = 1;
  }
}

void CMixingPlaneInterface::BroadcastData_MixingPlane(const CInterpolator& interpolator,
                               CSolver *donor_solution, CSolver *target_solution,
                               CGeometry *donor_geometry, CGeometry *target_geometry,
                               const CConfig *donor_config, const CConfig *target_config) {
  static_assert(su2activematrix::Storage == StorageType::RowMajor,"");

  /*--- Loop over interface markers. ---*/
  const auto nMarkerInt = donor_config->GetnMarker_MixingPlaneInterface() / 2;
  for (auto iMarkerInt = 1; iMarkerInt < nMarkerInt + 1; iMarkerInt++) {

    /*--- Find the markers containing the interface ---*/
    short markDonor = donor_config->FindMixingPlaneInterfaceMarker(donor_geometry->GetnMarker(), iMarkerInt);
    short markTarget= target_config->FindMixingPlaneInterfaceMarker(target_geometry->GetnMarker(), iMarkerInt);

    /*--- Check if this interface connects the two zones, if not continue. ---*/
    if(!CInterpolator::CheckInterfaceBoundary(markDonor, markTarget)) continue;

    if (donor_config->GetConservativeMixingPlane() != target_config->GetConservativeMixingPlane())
      SU2_MPI::Error("CONSERVATIVE_MIXINGPLANE must agree across a mixing interface", CURRENT_FUNCTION);
    if (target_config->GetConservativeMixingPlane())
      TransferConservativeFlux(donor_solution,target_solution,donor_geometry,target_geometry,
                               donor_config,target_config,markDonor,markTarget);

    // The number of spans is available on every rank
    const auto nSpanDonor = donor_config->GetnSpanWiseSections();

    /*--- Fill send buffers. ---*/

    su2vector<short> sendDonorMarker(nSpanDonor + 1);
    sendDonorMarker.setConstant(-1); // Initialize to -1 to identify ranks that do not have the marker
    su2activevector sendDonorVar(static_cast<size_t>(nSpanDonor + 1) * nMixingVars);

    if (markDonor != -1) {
      for (auto iSpan = 0; iSpan < nSpanDonor + 1; iSpan++) {
        GetDonor_Variable(donor_solution, donor_geometry, donor_config, markDonor, iSpan, 0);
        for (auto iVar = 0u; iVar < nMixingVars; iVar++) sendDonorVar[iSpan * nMixingVars + iVar] = Donor_Variable[iVar];
        sendDonorMarker[iSpan] = markDonor;
      }
    }
#ifdef HAVE_MPI
    /*--- Gather data. ---*/
    const size_t nTotalDonors = static_cast<size_t>(nSpanDonor + 1) * size; // Number of donor spans across all ranks
    const size_t nSpanDonorVars = static_cast<size_t>(nSpanDonor + 1) * nMixingVars; // Number of variables to be transferred on each rank
    su2vector<short> buffDonorMarker(nTotalDonors);
    su2activevector buffDonorVar(static_cast<unsigned long>(nTotalDonors) * nMixingVars); // Total number of variables to be transferred on all ranks

    SU2_MPI::Allgather(sendDonorMarker.data(), nSpanDonor + 1, MPI_SHORT,
                      buffDonorMarker.data(), nSpanDonor + 1, MPI_SHORT,
                      SU2_MPI::GetComm());

    SU2_MPI::Allgather(sendDonorVar.data(), nSpanDonorVars, MPI_DOUBLE,
                      buffDonorVar.data(), nSpanDonorVars, MPI_DOUBLE,
                      SU2_MPI::GetComm());

    for (auto iSize = 0; iSize < size; iSize++){
      if (buffDonorMarker[static_cast<size_t>(iSize) * static_cast<unsigned long>(nSpanDonor + 1)] != -1) {
        for (auto iSpan = 0; iSpan < nSpanDonor + 1; iSpan++){
          const size_t spanOffset = static_cast<size_t>(iSpan) * nMixingVars;
          const size_t donorOffset = static_cast<size_t>(iSize) * nSpanDonorVars;
          for (size_t iVar = 0u; iVar < nMixingVars; iVar++) sendDonorVar[spanOffset + iVar] = buffDonorVar[donorOffset + spanOffset + iVar];
        }
        markDonor = buffDonorMarker[static_cast<unsigned long>(iSize) * static_cast<unsigned long>(nSpanDonor + 1)];
        break; // Avoid overwriting
      }
    }
#endif

    /*--- This rank does not need to do more work. ---*/
    if (!(markTarget != -1 && markDonor != -1)) continue;

    /*--- Loop over target spans. ---*/
    unsigned long nTargetSpan = target_config->GetnSpanWiseSections() + 1;

    for (auto iTargetSpan = 0ul; iTargetSpan < nTargetSpan; iTargetSpan++) {

      auto& targetSpan = interpolator.targetSpans[iMarkerInt][iTargetSpan];

      /*--- Get the global index of the donor span. ---*/
      const auto donorSpan = targetSpan.donorSpan;

      if ((iTargetSpan == 0) || (iTargetSpan == nTargetSpan) || (iTargetSpan == nTargetSpan - 1)) {
        /*--- Transfer values at hub, shroud and 1D values ---*/
        RecoverTarget_SpanEndwall(sendDonorVar, donorSpan);

        SetTarget_Variable(target_solution, target_geometry, target_config, markTarget, iTargetSpan, 0);
      }
      else {
        /*--- Get the global index of interpolation coefficient. ---*/
        const auto donorCoeff = targetSpan.coefficient;

        /*--- Recover the Target_Variable from the buffer of variables. ---*/
        RecoverTarget_Span(sendDonorVar, donorSpan, donorCoeff);

        SetTarget_Variable(target_solution, target_geometry, target_config, markTarget, iTargetSpan, 0);
      }
    }
  }
}

void CMixingPlaneInterface::GetDonor_Variable(CSolver *donor_solution, CGeometry *donor_geometry,
                                          const CConfig *donor_config, unsigned long Marker_Donor,
                                          unsigned long Span_Donor, unsigned long Point_Donor) {

  Donor_Variable[0] = donor_solution->GetAverageDensity(Marker_Donor, Span_Donor);
  Donor_Variable[1] = donor_solution->GetAveragePressure(Marker_Donor, Span_Donor);
  Donor_Variable[2] = donor_solution->GetAverageTurboVelocity(Marker_Donor, Span_Donor)[0];
  Donor_Variable[3] = donor_solution->GetAverageTurboVelocity(Marker_Donor, Span_Donor)[1];

  if(donor_geometry->GetnDim() == 3){
    Donor_Variable[4] = donor_solution->GetAverageTurboVelocity(Marker_Donor, Span_Donor)[2];
  }
  else{
    Donor_Variable[4] = -1.0;
  }

  if(donor_config->GetKind_Turb_Model() != TURB_MODEL::NONE){
    Donor_Variable[5] = donor_solution->GetAverageNu(Marker_Donor, Span_Donor);
    Donor_Variable[6] = donor_solution->GetAverageKine(Marker_Donor, Span_Donor);
    Donor_Variable[7] = donor_solution->GetAverageOmega(Marker_Donor, Span_Donor);
  }
  else{
    Donor_Variable[5] = -1.0;
    Donor_Variable[6] = -1.0;
    Donor_Variable[7] = -1.0;
  }
}

void CMixingPlaneInterface::SetTarget_Variable(CSolver *target_solution, CGeometry *target_geometry,
                                           const CConfig *target_config, unsigned long Marker_Target,
                                           unsigned long Span_Target, unsigned long Point_Target) {
  /*--- Set the mixing plane solution with the value of the Target Variable ---*/
  for (unsigned short iVar = 0; iVar < nMixingVars; iVar++) {
    target_solution->SetMixingState(Marker_Target, Span_Target, iVar, Target_Variable[iVar]);
  }
}

void CMixingPlaneInterface::TransferConservativeFlux(CSolver* donor, CSolver* target,
    CGeometry* dg, CGeometry* tg, const CConfig* dc, const CConfig* tc, short dm, short tm) {
  // Freeze both sides before the lower-numbered zone solves; both multizone drivers visit targets in ascending order.
  if (dc->GetiZone() < tc->GetiZone()) return;
  auto gather = [](CSolver* solver, const CConfig* config, short marker) {
    const auto spans=config->GetnSpanWiseSections();
    su2activevector local(spans*6), global(spans*6); local.setConstant(0.0);
    if (marker>=0) for(auto span=0u;span<spans;++span) {
      const auto* flux=solver->GetMixingNumericalFlux(marker,span);
      if (flux) for(auto v=0u;v<6;++v)local[span*6+v]=flux[v];
    }
    SU2_MPI::Allreduce(local.data(),global.data(),spans*6,MPI_DOUBLE,MPI_SUM,SU2_MPI::GetComm());
    return global;
  };
  const auto donor_flux=gather(donor,dc,dm), local=gather(target,tc,tm);
  const auto nd=dc->GetnSpanWiseSections(), nt=tc->GetnSpanWiseSections();
  bool ready=true;
  for(auto span=0u;span<nd;++span)ready=ready && donor_flux[span*6]>0;
  for(auto span=0u;span<nt;++span)ready=ready && local[span*6]>0;
  if (!ready) {
    if (tm>=0)for(auto span=0u;span<nt;++span)target->SetConservativeMixingFlux(tm,span,nullptr);
    if (dm>=0)for(auto span=0u;span<nd;++span)donor->SetConservativeMixingFlux(dm,span,nullptr);
    return;
  }
  unsigned short df=dm>=0?dc->GetMarker_All_TurbomachineryFlag(dm):0;
  unsigned short tf=tm>=0?tc->GetMarker_All_TurbomachineryFlag(tm):0, dflag=0,tflag=0;
  SU2_MPI::Allreduce(&df,&dflag,1,MPI_UNSIGNED_SHORT,MPI_MAX,SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&tf,&tflag,1,MPI_UNSIGNED_SHORT,MPI_MAX,SU2_MPI::GetComm());
  if (dflag==tflag || !dflag || !tflag || dc->GetDensity_Ref()!=tc->GetDensity_Ref() ||
      dc->GetVelocity_Ref()!=tc->GetVelocity_Ref() || dc->GetPressure_Ref()!=tc->GetPressure_Ref())
    SU2_MPI::Error("Conservative mixing requires opposite sides and common nondimensional references",CURRENT_FUNCTION);
  std::vector<double> rd(nd),rt(nt);std::vector<MixingMeanFlux::Flux> fd(nd),ft(nt);
  for(auto i=0u;i<nd;++i) {
    rd[i]=SU2_TYPE::GetValue(dg->GetSpanWiseValue(dflag)[i]);
    for(auto v=0u;v<5;++v)fd[i][v]=SU2_TYPE::GetValue(donor_flux[i*6+v+1]);
  }
  for(auto i=0u;i<nt;++i) {
    rt[i]=SU2_TYPE::GetValue(tg->GetSpanWiseValue(tflag)[i]);
    for(auto v=0u;v<5;++v)ft[i][v]=SU2_TYPE::GetValue(local[i*6+v+1]);
  }
  try {
    const auto common=MixingMeanFlux::Common(rt,ft,SU2_TYPE::GetValue(tc->GetnBlades(tc->GetiZone())),
                                           rd,fd,SU2_TYPE::GetValue(dc->GetnBlades(dc->GetiZone())));
    const auto opposite=MixingMeanFlux::Common(rd,fd,SU2_TYPE::GetValue(dc->GetnBlades(dc->GetiZone())),
                                             rt,ft,SU2_TYPE::GetValue(tc->GetnBlades(tc->GetiZone())));
    if (tm>=0)for(auto span=0u;span<nt;++span)target->SetConservativeMixingFlux(tm,span,common[span].data());
    if (dm>=0)for(auto span=0u;span<nd;++span)donor->SetConservativeMixingFlux(dm,span,opposite[span].data());
  } catch (const std::exception& error) { SU2_MPI::Error(error.what(),CURRENT_FUNCTION); }
}
