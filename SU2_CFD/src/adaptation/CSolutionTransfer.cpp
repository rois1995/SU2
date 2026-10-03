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
#include "../../../Common/include/geometry/CGeometry.hpp"
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

  auto* flowSolver = solver[MESH_0][FLOW_SOL];
  if (config->GetTime_Domain()) {
    arrays.hasTimeN = flowSolver->GetNodes()->GetSolution_time_n().size() > 0;
    arrays.hasTimeN1 = flowSolver->GetNodes()->GetSolution_time_n1().size() > 0;
  }
  arrays.interpolateTimeN1 = arrays.hasTimeN1 && config->GetTime_Marching() == TIME_MARCHING::DT_STEPPING_2ND;

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
    if (arrays.hasTimeN) {
      auto* nodes = donor.solver[MESH_0][iSol]->GetNodes();
      const auto& solution = nodes->GetSolution();
      const auto& timeN = nodes->GetSolution_time_n();
      for (auto i = 0ul; i < solution.size(); ++i) {
        if (solution.data()[i] != timeN.data()[i]) {
          SU2_MPI::Error(
              "In the time domain the solution is transferred at the end of a time step (after the dual-time "
              "update), where the solution and Solution_time_n are the same state U^n; they differ here.",
              CURRENT_FUNCTION);
        }
      }
    }
  }
  return arrays;
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
