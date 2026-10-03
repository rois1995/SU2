/*!
 * \file CSolutionTransfer.cpp
 * \brief Parts shared by the solution transfers to a new mesh (mesh adaptation).
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

#include "../../include/adaptation/CSolutionTransfer.hpp"

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../include/solvers/CSolver.hpp"

CSolutionTransfer::TransferArrays CSolutionTransfer::CheckProblem(const std::string& name, CConfig* config,
                                                                  const CMeshDonor& donor, CGeometry** geometry,
                                                                  CSolver*** solver) {
  TransferArrays arrays;

  /*--- Supported problems: compressible flow, SA or SST. ---*/

  const auto kindSolver = config->GetKind_Solver();
  if (kindSolver != MAIN_SOLVER::EULER && kindSolver != MAIN_SOLVER::NAVIER_STOKES && kindSolver != MAIN_SOLVER::RANS) {
    SU2_MPI::Error(
        "The " + name + " solution transfer is only available for compressible EULER, NAVIER_STOKES or RANS.",
        CURRENT_FUNCTION);
  }
  if (config->GetTime_Domain() && config->GetGrid_Movement()) {
    SU2_MPI::Error("The " + name + " solution transfer is not available for moving meshes.", CURRENT_FUNCTION);
  }
  const auto turbFamily = TurbModelFamily(config->GetKind_Turb_Model());
  if (config->GetKind_Turb_Model() != TURB_MODEL::NONE && turbFamily != TURB_FAMILY::SA &&
      turbFamily != TURB_FAMILY::KW) {
    SU2_MPI::Error("The " + name + " solution transfer is only available for the SA and SST turbulence models.",
                   CURRENT_FUNCTION);
  }
  if (config->GetKind_Trans_Model() != TURB_TRANS_MODEL::NONE ||
      config->GetKind_Species_Model() != SPECIES_MODEL::NONE) {
    SU2_MPI::Error("The " + name + " solution transfer is not available with transition or species models.",
                   CURRENT_FUNCTION);
  }

  for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
    const auto* newSolver = solver[MESH_0][iSol];
    const auto* donorSolver = donor.solver[MESH_0][iSol];
    if (newSolver == nullptr && donorSolver == nullptr) continue;
    if (iSol != FLOW_SOL && iSol != TURB_SOL) {
      SU2_MPI::Error("The " + name + " solution transfer is not available for solver " + std::to_string(iSol) + ".",
                     CURRENT_FUNCTION);
    }
    if (newSolver == nullptr || donorSolver == nullptr || newSolver->GetnVar() != donorSolver->GetnVar()) {
      SU2_MPI::Error("The solvers of the donor and of the new mesh differ.", CURRENT_FUNCTION);
    }
    arrays.solverIndices.push_back(iSol);
  }
  if (solver[MESH_0][FLOW_SOL] == nullptr) SU2_MPI::Error("No flow solver.", CURRENT_FUNCTION);

  const auto* donorGeometry = donor.geometry[MESH_0];
  const auto* newGeometry = geometry[MESH_0];
  if (donorGeometry->GetnDim() != newGeometry->GetnDim()) {
    SU2_MPI::Error("The donor and the new mesh have different dimensions.", CURRENT_FUNCTION);
  }

  /*--- The arrays to set: the solution and, in the time domain, its history (Solution_time_n, and Solution_time_n1,
   *    which the variables allocate for every time marching). The transfer is done at the end of a time step, after
   *    the dual-time update, where the solution and Solution_time_n hold the same state U^n: it is transferred once
   *    into both. U^(n-1) is transferred only for 2nd-order dual time stepping; otherwise Solution_time_n1 is set to
   *    U^n (not used, but no stale data). Both meshes have the same config, so the same arrays. ---*/

  const ArrayGetter getSolution = [](CVariable* nodes) -> su2activematrix& { return nodes->GetSolution(); };
  const ArrayGetter getTimeN = [](CVariable* nodes) -> su2activematrix& { return nodes->GetSolution_time_n(); };
  const ArrayGetter getTimeN1 = [](CVariable* nodes) -> su2activematrix& { return nodes->GetSolution_time_n1(); };

  /*--- The history arrays the variables allocate (CVariable: Solution_time_n in the time domain, Solution_time_n1 for
   *    every time marching but STEADY): from the config, not from the local array sizes (a rank may have no points). */
  auto* flowSolver = solver[MESH_0][FLOW_SOL];
  if (config->GetTime_Domain()) {
    arrays.hasTimeN = true;
    arrays.hasTimeN1 = config->GetTime_Marching() != TIME_MARCHING::STEADY;
  }
  arrays.interpolateTimeN1 = arrays.hasTimeN1 && config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_2ND;

  /*--- Layout of the fields (R8): from the config and the solver objects; the same on every rank. ---*/
  arrays.nDim = newGeometry->GetnDim();
  arrays.nVarFlow = flowSolver->GetnVar();
  arrays.nVarTurb = solver[MESH_0][TURB_SOL] ? solver[MESH_0][TURB_SOL]->GetnVar() : 0;
  arrays.nLevel = arrays.interpolateTimeN1 ? 2 : 1;
  arrays.sst = arrays.nVarTurb > 0 && turbFamily == TURB_FAMILY::KW;
  {
    unsigned long layout[] = {arrays.nDim, arrays.nVarFlow, arrays.nVarTurb, arrays.nLevel, arrays.hasTimeN,
                              arrays.hasTimeN1, arrays.interpolateTimeN1, arrays.sst, arrays.solverIndices.size()};
    constexpr size_t n = sizeof(layout) / sizeof(layout[0]);
    unsigned long lo[n], hi[n];
    CPassiveComm::Allreduce(layout, lo, n, CPassiveComm::Op::MIN);
    CPassiveComm::Allreduce(layout, hi, n, CPassiveComm::Op::MAX);
    for (size_t i = 0; i < n; ++i) {
      if (lo[i] != hi[i]) {
        SU2_MPI::Error("The " + name + " solution transfer: the layout of the fields (dimensions, variables, time " +
                           "levels) differs between the ranks.",
                       CURRENT_FUNCTION);
      }
    }
  }

  std::vector<ArrayGetter> all = {getSolution};
  if (arrays.hasTimeN) {
    all.push_back(getTimeN);
    arrays.history.push_back(getTimeN);
    arrays.historyComms.push_back(MPI_QUANTITIES::SOLUTION_TIME_N);
  }
  if (arrays.hasTimeN1) {
    all.push_back(getTimeN1);
    arrays.history.push_back(getTimeN1);
    arrays.historyComms.push_back(MPI_QUANTITIES::SOLUTION_TIME_N1);
  }

  for (const auto iSol : arrays.solverIndices) {
    for (const auto getter : all) {
      auto& newArray = getter(solver[MESH_0][iSol]->GetNodes());
      auto& donorArray = getter(donor.solver[MESH_0][iSol]->GetNodes());
      if (newArray.rows() != newGeometry->GetnPoint() || donorArray.rows() != donorGeometry->GetnPoint() ||
          newArray.cols() != donorArray.cols()) {
        SU2_MPI::Error("The solution arrays of the donor and of the new mesh differ.", CURRENT_FUNCTION);
      }
    }
  }

  /*--- In the time domain U^n is transferred once into the solution and Solution_time_n: they must be the same state
   *    (bitwise) on the domain points of the donor; a difference on any rank stops all ranks. ---*/
  CLocalFailure failure;
  for (const auto iSol : arrays.solverIndices) {
    if (!arrays.hasTimeN) break;
    auto* nodes = donor.solver[MESH_0][iSol]->GetNodes();
    const auto& solution = nodes->GetSolution();
    const auto& timeN = nodes->GetSolution_time_n();
    for (auto iPoint = 0ul; iPoint < donorGeometry->GetnPointDomain(); ++iPoint) {
      bool same = true;
      for (auto iVar = 0ul; iVar < solution.cols(); ++iVar) same &= solution(iPoint, iVar) == timeN(iPoint, iVar);
      if (!same) {
        failure.Set(1, donorGeometry->nodes->GetGlobalIndex(iPoint),
                    "In the time domain the solution is transferred at the end of a time step (after the dual-time "
                    "update), where the solution and Solution_time_n are the same state U^n; they differ here.");
        break;
      }
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  return arrays;
}

std::vector<std::string> CSolutionTransfer::DonorMarkerTags(const std::string& name, const CMeshDonor& donor) {
  const auto nMarker = donor.geometry[MESH_0]->GetnMarker();
  auto tags = donor.markerTags;
  if (tags.size() < nMarker) {
    if (SU2_MPI::GetSize() > 1) {
      const std::string counts = "the donor geometry has " + std::to_string(nMarker) +
                                 " markers, CMeshDonor::markerTags names " + std::to_string(tags.size()) + ". ";
      SU2_MPI::Error("The " + name + " solution transfer: " + counts +
                         "With more than one rank every marker of the donor needs its name: the names identify the "
                         "markers across the ranks.",
                     CURRENT_FUNCTION);
    }
    /*--- One rank: the markers without a name are unnamed donor boundaries (as before the transfers ran on gathered
     *    meshes): their faces only take part in the search of the nearest boundary face. ---*/
    tags.resize(nMarker, "");
  }
  if (SU2_MPI::GetSize() > 1) {
    for (unsigned short iMarker = 0; iMarker < nMarker; ++iMarker) {
      if (tags[iMarker].empty()) {
        SU2_MPI::Error("The " + name + " solution transfer: marker " + std::to_string(iMarker) +
                           " of the donor has no name. With more than one rank every marker of the donor needs its "
                           "name: the names identify the markers across the ranks.",
                       CURRENT_FUNCTION);
      }
    }
  }
  return tags;
}

void CSolutionTransfer::FinishTransfer(CConfig* config, CGeometry** geometry, CSolver*** solver,
                                       const TransferArrays& arrays) {
  /*--- As after loading a restart file: communication, primitive variables, eddy viscosity, coarse levels (in the
   *    order of the restart, the turbulence solver updates the flow primitives). The old solution is the new one, the
   *    flow preprocessing would otherwise reset non-physical points to the free-stream state of the constructor. ---*/

  /*--- The transfers set the domain points; the halo points get their values before the old solution is set. ---*/
  for (const auto iSol : arrays.solverIndices) {
    solver[MESH_0][iSol]->InitiateComms(geometry[MESH_0], config, MPI_QUANTITIES::SOLUTION);
    solver[MESH_0][iSol]->CompleteComms(geometry[MESH_0], config, MPI_QUANTITIES::SOLUTION);
    solver[MESH_0][iSol]->Set_OldSolution();
  }

  for (const auto iSol : arrays.solverIndices) {
    auto* sol = solver[MESH_0][iSol];
    SU2_OMP_PARALLEL_(if (sol->GetHasHybridParallel()))
    sol->UpdateLoadedSolution(geometry, solver, config);
    END_SU2_OMP_PARALLEL
  }

  for (unsigned short iMesh = 1; iMesh <= config->GetnMGLevels(); ++iMesh)
    for (const auto iSol : arrays.solverIndices) solver[iMesh][iSol]->Set_OldSolution();

  /*--- The time history on the coarse levels is its restriction, as after a restart (PushSolutionBackInTime sets it
   *    from the restricted solution). ---*/

  for (const auto iSol : arrays.solverIndices) {
    for (unsigned short iLevel = 0; iLevel < arrays.history.size(); ++iLevel) {
      const auto getter = arrays.history[iLevel];
      const auto comm = arrays.historyComms[iLevel];
      solver[MESH_0][iSol]->InitiateComms(geometry[MESH_0], config, comm);
      solver[MESH_0][iSol]->CompleteComms(geometry[MESH_0], config, comm);
      for (unsigned short iMesh = 1; iMesh <= config->GetnMGLevels(); ++iMesh) {
        CSolver::MultigridRestriction(*geometry[iMesh - 1], getter(solver[iMesh - 1][iSol]->GetNodes()),
                                      *geometry[iMesh], getter(solver[iMesh][iSol]->GetNodes()));
        solver[iMesh][iSol]->InitiateComms(geometry[iMesh], config, comm);
        solver[iMesh][iSol]->CompleteComms(geometry[iMesh], config, comm);
      }
    }
  }
}
