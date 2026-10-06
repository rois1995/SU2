/*!
 * \file CAdjointTransfer.cpp
 * \brief Transfer of a discrete adjoint problem (primal and adjoint solutions) to a new mesh (goal-oriented adaptation).
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

#include "../../include/adaptation/CAdjointTransfer.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../include/adaptation/CDistributedLocator.hpp"
#include "../../include/solvers/CSolver.hpp"

std::vector<unsigned short> CDiscAdjTransfer::AdjointSolvers(const CMeshDonor& donor, CSolver*** solver) {

  /*--- Per slot: the adjoint solver of the donor and of the new mesh (presence, variables), no extra adjoint
   *    variables; the layout of the slots the same on every rank (it comes from the config and the solver objects). ---*/
  std::vector<unsigned short> adjoints;
  std::vector<unsigned long> layout(MAX_SOLS, 0);
  CLocalFailure failure;
  for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
    const auto* newSolver = solver[MESH_0][iSol];
    const auto* donorSolver = donor.solver[MESH_0][iSol];
    const bool newAdjoint = newSolver != nullptr && newSolver->GetAdjoint();
    const bool donorAdjoint = donorSolver != nullptr && donorSolver->GetAdjoint();
    if (!newAdjoint && !donorAdjoint) continue;
    if (newAdjoint != donorAdjoint || newSolver->GetnVar() != donorSolver->GetnVar()) {
      failure.Set(2, iSol, "The adjoint solvers of the donor and of the new mesh differ (solver " +
                               std::to_string(iSol) + ").");
      continue;
    }
    if (newSolver->GetNodes()->GetSolutionExtra().size() != 0 ||
        donorSolver->GetNodes()->GetSolutionExtra().size() != 0) {
      failure.Set(1, iSol, "The adjoint transfer does not support extra adjoint variables (solver " +
                               std::to_string(iSol) + ", turbulence adjoint, G8).");
      continue;
    }
    adjoints.push_back(iSol);
    layout[iSol] = 1 + newSolver->GetnVar();
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);

  std::vector<unsigned long> lo(MAX_SOLS), hi(MAX_SOLS);
  CPassiveComm::Allreduce(layout.data(), lo.data(), MAX_SOLS, CPassiveComm::Op::MIN);
  CPassiveComm::Allreduce(layout.data(), hi.data(), MAX_SOLS, CPassiveComm::Op::MAX);
  if (lo != hi) SU2_MPI::Error("The adjoint transfer: the adjoint solvers differ between the ranks.", CURRENT_FUNCTION);
  return adjoints;
}

void CDiscAdjTransfer::Transfer(CConfig* config, const CMeshDonor& donor, CGeometry** geometry, CSolver*** solver) {
  SU2_ZONE_SCOPED

  if (!primal) SU2_MPI::Error("The adjoint transfer needs a primal transfer.", CURRENT_FUNCTION);

  /*--- The adjoint solvers are checked before the primal transfer communicates. ---*/
  AdjointSolvers(donor, solver);

  /*--- The primal transfer sets the flow (and turbulence) solvers, as after a restart. ---*/
  primal->Transfer(config, donor, geometry, solver);

  TransferAdjoint(config, donor, geometry, solver);
}

void CDiscAdjTransfer::TransferAdjoint(CConfig* config, const CMeshDonor& donor, CGeometry** geometry,
                                       CSolver*** solver) {
  SU2_ZONE_SCOPED

  const auto startTime = SU2_MPI::Wtime();
  const int rank = SU2_MPI::GetRank();
  summary = Summary();

  auto* donorGeometry = donor.geometry[MESH_0];
  auto* newGeometry = geometry[MESH_0];
  const auto nDim = newGeometry->GetnDim();
  if (donorGeometry->GetnDim() != nDim) {
    SU2_MPI::Error("The donor and the new mesh have different dimensions.", CURRENT_FUNCTION);
  }

  const auto adjoints = AdjointSolvers(donor, solver);
  unsigned short nField = 0;
  for (const auto iSol : adjoints) nField += solver[MESH_0][iSol]->GetnVar();
  summary.nSolver = adjoints.size();
  if (adjoints.empty()) return;

  /*--- Largest |psi| per variable on the owned donor points (reported; P1 weights keep the new values within). ---*/
  auto maxAbs = [&](CGeometry* geo, CSolver*** solvers) {
    std::vector<passivedouble> local(nField, 0.0), global(nField, 0.0);
    unsigned short offset = 0;
    for (const auto iSol : adjoints) {
      const auto& psi = solvers[MESH_0][iSol]->GetNodes()->GetSolution();
      for (auto iPoint = 0ul; iPoint < geo->GetnPointDomain(); ++iPoint)
        for (unsigned long iVar = 0; iVar < psi.cols(); ++iVar)
          local[offset + iVar] = std::max(local[offset + iVar], std::fabs(SU2_TYPE::GetValue(psi(iPoint, iVar))));
      offset += psi.cols();
    }
    CPassiveComm::Allreduce(local.data(), global.data(), nField, CPassiveComm::Op::MAX);
    return global;
  };
  summary.donorMax = maxAbs(donorGeometry, donor.solver);

  const auto nPoint = newGeometry->GetnPointDomain();
  summary.nPoint = CPassiveComm::AllreduceSum(nPoint);

  if (warmStart) {
    CTransferRoundScope rounds;
    const size_t recordBytes = nField * FieldValueBytes();

    /*--- Donor: psi of the owned points in the rendezvous directory. ---*/
    CPointDirectory directory;
    {
      const auto nOwned = donorGeometry->GetnPointDomain();
      std::vector<uint64_t> gids(nOwned);
      std::vector<char> records(nOwned * recordBytes);
      std::vector<su2double> values(nField);
      for (auto iPoint = 0ul; iPoint < nOwned; ++iPoint) {
        gids[iPoint] = donorGeometry->nodes->GetGlobalIndex(iPoint);
        unsigned short offset = 0;
        for (const auto iSol : adjoints) {
          const auto& psi = donor.solver[MESH_0][iSol]->GetNodes()->GetSolution();
          for (unsigned long iVar = 0; iVar < psi.cols(); ++iVar) values[offset + iVar] = psi(iPoint, iVar);
          offset += psi.cols();
        }
        PackFieldValues(&records[iPoint * recordBytes], values.data(), nField);
      }
      directory.Build(gids, records, recordBytes, "adjoint donor");
    }

    /*--- Stencils of the owned points of the new mesh (the rules of the barycentric transfer). ---*/
    std::vector<std::string> newTags;
    for (unsigned short iMarker = 0; iMarker < newGeometry->GetnMarker(); ++iMarker)
      newTags.push_back(config->GetMarker_All_TagBound(iMarker));
    const auto donorTags = DonorMarkerTags("adjoint", donor);
    std::vector<passivedouble> coord(nPoint * nDim);
    auto pointMarkers = PointMarkerIds(*newGeometry, newTags, *config);
    pointMarkers.resize(nPoint);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        coord[iPoint * nDim + iDim] = SU2_TYPE::GetValue(newGeometry->nodes->GetCoord(iPoint, iDim));

    std::vector<CDistributedLocator::Stencil> stencils;
    {
      CDistributedLocator locator(*donorGeometry, donorTags, *config, 2.0 * SU2_TYPE::GetValue(config->GetAdap_Hausd()));
      stencils = locator.Locate(coord, pointMarkers, directory.GetMemory());

      /*--- Points beyond the distance limit: the farthest one (ratio, ties by global index) stops all ranks. ---*/
      unsigned long nOutside = 0, nBeyond = 0;
      passivedouble maxDistance = 0.0, worstRatio = 0.0;
      unsigned long worstPoint = 0;
      uint64_t worstGid = UINT64_MAX;
      for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
        const auto& stencil = stencils[iPoint];
        nOutside += !stencil.inside;
        maxDistance = std::max(maxDistance, stencil.distance);
        if (!stencil.beyondLimit) continue;
        nBeyond++;
        passivedouble ratio = stencil.distance / locator.GetDistanceLimit(stencil.faceSize);
        if (!std::isfinite(ratio)) ratio = std::numeric_limits<passivedouble>::infinity();
        const uint64_t gid = newGeometry->nodes->GetGlobalIndex(iPoint);
        if (ratio > worstRatio || (ratio == worstRatio && gid < worstGid)) {
          worstRatio = ratio;
          worstPoint = iPoint;
          worstGid = gid;
        }
      }
      summary.nOutside = CPassiveComm::AllreduceSum(nOutside);
      summary.maxDistance = CPassiveComm::Allreduce(maxDistance, CPassiveComm::Op::MAX);
      const auto nBeyondGlobal = CPassiveComm::AllreduceSum(nBeyond);
      if (nBeyondGlobal > 0) {
        const double globalWorst = CPassiveComm::Allreduce(static_cast<double>(worstRatio), CPassiveComm::Op::MAX);
        CLocalFailure failure;
        if (nBeyond > 0 && worstRatio == globalWorst) {
          std::ostringstream text;
          text << std::setprecision(10) << nBeyondGlobal
               << " points of the new mesh are farther from the donor mesh than accepted (adjoint transfer), the "
                  "farthest is (";
          for (unsigned short iDim = 0; iDim < nDim; ++iDim) text << (iDim ? ", " : "") << coord[worstPoint * nDim + iDim];
          text << ") at " << stencils[worstPoint].distance << " from the donor boundary.";
          failure.Set(1, worstGid, text.str());
        }
        CollectiveFailure(failure, CURRENT_FUNCTION);
      }
    }

    /*--- Values of the stencil points from the directory, then the weighted sums. ---*/
    std::vector<uint64_t> unique;
    for (const auto& stencil : stencils) unique.insert(unique.end(), stencil.gid, stencil.gid + stencil.nPoint);
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    std::vector<su2double> donorValues(unique.size() * nField);
    {
      const auto fetched = directory.Fetch(unique);
      for (auto i = 0ul; i < unique.size(); ++i)
        UnpackFieldValues(&fetched[i * recordBytes], &donorValues[i * nField], nField);
    }

    std::vector<su2double> values(nField);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
      const auto& stencil = stencils[iPoint];
      for (auto& value : values) value = 0.0;
      for (unsigned short k = 0; k < stencil.nPoint; ++k) {
        const auto index = std::lower_bound(unique.begin(), unique.end(), stencil.gid[k]) - unique.begin();
        for (unsigned short iField = 0; iField < nField; ++iField)
          values[iField] += stencil.weight[k] * donorValues[index * nField + iField];
      }
      unsigned short offset = 0;
      for (const auto iSol : adjoints) {
        auto& psi = solver[MESH_0][iSol]->GetNodes()->GetSolution();
        for (unsigned long iVar = 0; iVar < psi.cols(); ++iVar) psi(iPoint, iVar) = values[offset + iVar];
        offset += psi.cols();
      }
    }

    /*--- As after loading an adjoint restart: halo values, old solution, coarse levels. ---*/
    for (const auto iSol : adjoints) {
      auto* adjoint = solver[MESH_0][iSol];
      adjoint->InitiateComms(newGeometry, config, MPI_QUANTITIES::SOLUTION);
      adjoint->CompleteComms(newGeometry, config, MPI_QUANTITIES::SOLUTION);
      adjoint->GetNodes()->Set_OldSolution();
      for (unsigned short iMesh = 1; iMesh <= config->GetnMGLevels(); ++iMesh) {
        CSolver::MultigridRestriction(*geometry[iMesh - 1], solver[iMesh - 1][iSol]->GetNodes()->GetSolution(),
                                      *geometry[iMesh], solver[iMesh][iSol]->GetNodes()->GetSolution());
        solver[iMesh][iSol]->InitiateComms(geometry[iMesh], config, MPI_QUANTITIES::SOLUTION);
        solver[iMesh][iSol]->CompleteComms(geometry[iMesh], config, MPI_QUANTITIES::SOLUTION);
        solver[iMesh][iSol]->GetNodes()->Set_OldSolution();
      }
    }
  }

  summary.newMax = maxAbs(newGeometry, solver);
  summary.time = SU2_MPI::Wtime() - startTime;

  if (rank == MASTER_NODE) {
    std::cout << "Adjoint transfer (" << (warmStart ? "P1 interpolation of psi" : "cold start, psi not transferred")
              << "): " << summary.nPoint << " points";
    if (warmStart) {
      std::cout << ", " << summary.nOutside << " outside the donor (largest distance " << summary.maxDistance << ")";
    }
    std::cout << ", " << summary.time << " s." << std::endl;
    std::cout << "  max |psi| per variable, donor -> new:" << std::scientific << std::setprecision(4);
    for (unsigned short iField = 0; iField < nField; ++iField)
      std::cout << " " << summary.donorMax[iField] << " -> " << summary.newMax[iField];
    std::cout << std::endl;
    std::cout.unsetf(std::ios_base::floatfield);
    std::cout << std::setprecision(6);
  }
}
