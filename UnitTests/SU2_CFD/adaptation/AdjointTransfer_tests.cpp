/*!
 * \file AdjointTransfer_tests.cpp
 * \brief Unit tests of the transfer of a discrete adjoint problem to a new mesh (CDiscAdjTransfer, stage G2).
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
#include <limits>
#include <map>

#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/TransferTolerances.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../SU2_CFD/include/adaptation/CAdjointTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "../../../SU2_CFD/include/solvers/CDiscAdjSolver.hpp"

using namespace transfer_test;

namespace {

/*--- Run f with every rank alone (MPI_COMM_SELF): each rank computes the whole problem. ---*/
template <class F>
void Serial(F f) {
#ifdef HAVE_MPI
  const auto world = SU2_MPI::GetComm();
  SU2_MPI::SetComm(MPI_COMM_SELF);
  f();
  SU2_MPI::SetComm(world);
#else
  f();
#endif
}

passivedouble WorldMax(passivedouble value) { return CPassiveComm::Allreduce(value, CPassiveComm::Op::MAX); }

/*--- The adjoint flow solver of a mesh on every level (as CSolverFactory builds it for a discrete adjoint problem). ---*/
void AddAdjoint(MeshSolution& mesh, CConfig* config, unsigned short slot = ADJFLOW_SOL) {
  Mute mute;
  for (unsigned short iMesh = 0; iMesh <= mesh.nMGLevels; ++iMesh) {
    mesh.solver[iMesh][slot] =
        new CDiscAdjSolver(mesh.geometry[iMesh], config, mesh.solver[iMesh][FLOW_SOL], RUNTIME_FLOW_SYS, iMesh);
  }
}

/*--- Affine adjoint field, a different plane and scale for each variable. ---*/
void AffinePsi(unsigned short nDim, const su2double* x, su2double* psi) {
  const su2double z = (nDim == 3) ? x[2] : su2double(0.0);
  for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
    const passivedouble s = std::pow(10.0, iVar - 2.0);
    psi[iVar] = s * (0.3 + 1.1 * iVar - (0.7 + iVar) * x[0] + 0.4 * (iVar % 2 ? 1 : -1) * x[1] + 0.25 * iVar * z);
  }
}

/*--- Donor fields: affine flow and psi on the domain points; the halo psi poisoned (only owned rows are read). ---*/
void SetDonorFields(MeshSolution& donor, unsigned short nDim, unsigned short slot = ADJFLOW_SOL) {
  donor.SetField(FLOW_SOL, AffineFlow(nDim));
  auto* psi = donor.solver[MESH_0][slot]->GetNodes();
  const auto nPointDomain = donor.Fine().GetnPointDomain();
  for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
    su2double values[MAXVAR] = {};
    AffinePsi(nDim, donor.Fine().nodes->GetCoord(iPoint), values);
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar)
      psi->SetSolution(iPoint, iVar, iPoint < nPointDomain ? values[iVar]
                                                           : su2double(std::numeric_limits<passivedouble>::quiet_NaN()));
  }
}

using PointValues = std::map<uint64_t, std::vector<passivedouble>>;

/*--- psi (and optionally the flow solution) of the domain points by global index. ---*/
PointValues Values(const MeshSolution& mesh, unsigned short iSol) {
  PointValues values;
  const auto& array = mesh.solver[MESH_0][iSol]->GetNodes()->GetSolution();
  for (auto iPoint = 0ul; iPoint < mesh.Fine().GetnPointDomain(); ++iPoint) {
    auto& row = values[mesh.Fine().nodes->GetGlobalIndex(iPoint)];
    for (unsigned long iVar = 0; iVar < array.cols(); ++iVar) row.push_back(SU2_TYPE::GetValue(array(iPoint, iVar)));
  }
  return values;
}

/*--- Largest |a - b| / scale of the field (all ranks), scale = max |b| of the reference per column. ---*/
passivedouble MaxDifference(const PointValues& values, const PointValues& reference, bool& complete) {
  std::vector<passivedouble> scale;
  for (const auto& entry : reference) {
    if (scale.empty()) scale.assign(entry.second.size(), 0.0);
    for (auto f = 0ul; f < entry.second.size(); ++f) scale[f] = std::max(scale[f], std::fabs(entry.second[f]));
  }
  passivedouble diff = 0.0;
  bool ok = true;
  for (const auto& entry : values) {
    const auto it = reference.find(entry.first);
    if (it == reference.end() || it->second.size() != entry.second.size()) {
      ok = false;
      continue;
    }
    for (auto f = 0ul; f < entry.second.size(); ++f) {
      const passivedouble d = std::fabs(entry.second[f] - it->second[f]) / (scale[f] > 0.0 ? scale[f] : 1.0);
      diff = std::max(diff, std::isfinite(d) ? d : std::numeric_limits<passivedouble>::infinity());
    }
  }
  complete = CPassiveComm::AllreduceSum(ok ? 0ul : 1ul) == 0;
  return WorldMax(diff);
}

/*--- Largest |psi - exact| / max |exact| over ALL points (domain and halo) of the fine level, all ranks. ---*/
passivedouble AffineError(const MeshSolution& mesh, unsigned short nDim, unsigned short slot = ADJFLOW_SOL) {
  const auto* psi = mesh.solver[MESH_0][slot]->GetNodes();
  std::vector<passivedouble> scale(nDim + 2, 0.0), diff(nDim + 2, 0.0);
  for (auto iPoint = 0ul; iPoint < mesh.Fine().GetnPoint(); ++iPoint) {
    su2double exact[MAXVAR] = {};
    AffinePsi(nDim, mesh.Fine().nodes->GetCoord(iPoint), exact);
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
      scale[iVar] = std::max(scale[iVar], std::fabs(SU2_TYPE::GetValue(exact[iVar])));
      const passivedouble d = std::fabs(SU2_TYPE::GetValue(psi->GetSolution(iPoint, iVar) - exact[iVar]));
      diff[iVar] = std::max(diff[iVar], std::isfinite(d) ? d : std::numeric_limits<passivedouble>::infinity());
    }
  }
  passivedouble worst = 0.0;
  for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar)
    worst = std::max(worst, diff[iVar] / std::max(WorldMax(scale[iVar]), 1e-300));
  return WorldMax(worst);
}

/*--- The coarse levels of psi are the restriction of the fine one, the old solution equals it (domain points). ---*/
bool CoarseLevelsRestricted(const MeshSolution& mesh, unsigned short slot) {
  bool ok = true;
  for (unsigned short iMesh = 1; iMesh <= mesh.nMGLevels; ++iMesh) {
    auto* nodes = mesh.solver[iMesh][slot]->GetNodes();
    su2activematrix restricted = nodes->GetSolution();
    CSolver::MultigridRestriction(*mesh.geometry[iMesh - 1], mesh.solver[iMesh - 1][slot]->GetNodes()->GetSolution(),
                                  *mesh.geometry[iMesh], restricted);
    for (auto iPoint = 0ul; iPoint < mesh.geometry[iMesh]->GetnPointDomain(); ++iPoint) {
      for (unsigned short iVar = 0; iVar < mesh.solver[iMesh][slot]->GetnVar(); ++iVar) {
        ok &= RelDiff(nodes->GetSolution(iPoint, iVar), restricted(iPoint, iVar)) < 1e-14;
        ok &= nodes->GetSolution(iPoint, iVar) == nodes->GetSolution_Old(iPoint, iVar);
      }
    }
  }
  return CPassiveComm::AllreduceSum(ok ? 0ul : 1ul) == 0;
}

std::unique_ptr<CSolutionTransfer> Primal(ADAP_TRANSFER kind) {
  switch (kind) {
    case ADAP_TRANSFER::BARYCENTRIC: return std::make_unique<CBarycentricTransfer>(false);
    case ADAP_TRANSFER::CONSERVATIVE: return std::make_unique<CConservativeTransfer>();
    default: return std::make_unique<CFreeStreamTransfer>();
  }
}

struct AdjointResult {
  PointValues psi, flow, flowPrimalOnly;
  passivedouble affineError = 0.0;
  bool coarse = false;
  CDiscAdjTransfer::Summary summary;
  unsigned long minOwned = 0;
};

/*--- Donor box with n1 cells, target box with n2 cells (different triangulations of the same box). ---*/
AdjointResult RunAdjoint(unsigned short nDim, unsigned long n1, unsigned long n2, ADAP_TRANSFER kind, bool warm,
                         bool samePerturbation = false) {
  AdjointResult result;
  auto config = MakeConfig(nDim, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(nDim, n1, true), 2);
  AddAdjoint(donor, config.get());
  SetDonorFields(donor, nDim);
  MeshSolution target(config.get(), BoxMesh(nDim, n2, !samePerturbation), 2);
  AddAdjoint(target, config.get());
  CDiscAdjTransfer transfer(Primal(kind), warm);
  {
    Mute mute;
    transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
  }
  result.psi = Values(target, ADJFLOW_SOL);
  result.flow = Values(target, FLOW_SOL);
  result.affineError = AffineError(target, nDim);
  result.coarse = CoarseLevelsRestricted(target, ADJFLOW_SOL);
  result.summary = transfer.GetSummary();
  result.minOwned = CPassiveComm::AllreduceMin(target.Fine().GetnPointDomain());

  /*--- The primal transfer alone on another copy of the target: the flow must be the same, bit for bit. ---*/
  MeshSolution alone(config.get(), BoxMesh(nDim, n2, !samePerturbation), 2);
  {
    Mute mute;
    Primal(kind)->Transfer(config.get(), donor.Donor(), alone.geometry, alone.solver);
  }
  result.flowPrimalOnly = Values(alone, FLOW_SOL);
  return result;
}

bool Bitwise(const PointValues& a, const PointValues& b) {
  unsigned long n = 0;
  if (a.size() != b.size()) n = 1;
  for (const auto& entry : a) {
    const auto it = b.find(entry.first);
    if (it == b.end() || it->second != entry.second) n++;
  }
  return CPassiveComm::AllreduceSum(n) == 0;
}

}  // namespace

TEST_CASE("Adjoint transfer: affine psi exact for every primal policy, partition equivalence",
          "[AdjointTransfer][AdaptationMPI]") {
  if (transfer_tol::kSinglePrecision) return;
  struct Case {
    unsigned short nDim;
    unsigned long n1, n2;
  };
  for (const auto test : {Case{2, 4, 6}, Case{3, 3, 4}, Case{2, 6, 2}}) {
    for (const auto kind : {ADAP_TRANSFER::BARYCENTRIC, ADAP_TRANSFER::CONSERVATIVE, ADAP_TRANSFER::FREESTREAM}) {
      INFO("dim " << test.nDim << ", donor " << test.n1 << ", target " << test.n2 << ", policy "
                  << static_cast<int>(kind));
      AdjointResult serial;
      Serial([&]() { serial = RunAdjoint(test.nDim, test.n1, test.n2, kind, true); });
      const auto parallel = RunAdjoint(test.nDim, test.n1, test.n2, kind, true);

      /*--- Exact on every point (domain and halo), coarse levels restricted, flow not touched by the adjoint part. ---*/
      CHECK(parallel.affineError <= 1e-12);
      CHECK(serial.affineError <= 1e-12);
      CHECK(parallel.coarse);
      CHECK(Bitwise(parallel.flow, parallel.flowPrimalOnly));
      CHECK(parallel.summary.nSolver == 1);
      CHECK(parallel.summary.nPoint > 0);

      /*--- Partition independence (by global index). ---*/
      bool complete = false;
      CHECK(MaxDifference(parallel.psi, serial.psi, complete) <= 1e-12);
      CHECK(complete);
      if (SU2_MPI::GetRank() == MASTER_NODE && kind == ADAP_TRANSFER::BARYCENTRIC) {
        WARN("dim " << test.nDim << " target " << test.n2 << ": smallest number of owned target points "
                    << parallel.minOwned);
      }
    }
  }
}

TEST_CASE("Adjoint transfer: cold start and same mesh", "[AdjointTransfer][AdaptationMPI]") {
  if (transfer_tol::kSinglePrecision) return;
  for (const unsigned short nDim : {2, 3}) {
    INFO("dim " << nDim);
    /*--- Cold start: psi keeps the constructor state of a new adjoint solver (1e-16), the flow is transferred. ---*/
    const auto cold = RunAdjoint(nDim, 4, 4, ADAP_TRANSFER::BARYCENTRIC, false);
    bool allInitial = true;
    for (const auto& entry : cold.psi)
      for (const auto value : entry.second) allInitial &= value == 1e-16;
    CHECK(CPassiveComm::AllreduceSum(allInitial ? 0ul : 1ul) == 0);
    CHECK(Bitwise(cold.flow, cold.flowPrimalOnly));

    /*--- Same mesh as donor and target: psi reproduced to round-off. ---*/
    const auto same = RunAdjoint(nDim, 4, 4, ADAP_TRANSFER::BARYCENTRIC, true, true);
    CHECK(same.affineError <= 1e-15);
  }
}

TEST_CASE("Adjoint transfer: curved boundary and marker order", "[AdjointTransfer][AdaptationMPI]") {
  if (transfer_tol::kSinglePrecision) return;

  /*--- Disk: target boundary points are projected on the donor faces of the same marker; the values are convex
   *    combinations of donor values (within the donor range per variable). ---*/
  {
    auto config = MakeRoundConfig("SOLVER= EULER\n", "MARKER_FAR= (round_a, round_b)\n");
    MeshSolution donor(config.get(), simplex_test::MakeRoundMesh(2, 6, 1.0), 0);
    AddAdjoint(donor, config.get());
    SetDonorFields(donor, 2);
    MeshSolution target(config.get(), simplex_test::MakeRoundMesh(2, 10, 1.0), 0);
    AddAdjoint(target, config.get());
    CDiscAdjTransfer transfer(Primal(ADAP_TRANSFER::BARYCENTRIC), true);
    {
      Mute mute;
      transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
    }
    const auto& summary = transfer.GetSummary();
    CHECK(summary.nOutside > 0);
    bool within = true;
    for (unsigned short iVar = 0; iVar < 4; ++iVar) within &= summary.newMax[iVar] <= summary.donorMax[iVar] * (1 + 1e-14);
    CHECK(within);
    /*--- Interior points are exact (affine field); boundary points are within the donor bounds only. ---*/
    passivedouble interior = 0.0, scale = 0.0;
    auto* psi = target.solver[MESH_0][ADJFLOW_SOL]->GetNodes();
    for (auto iPoint = 0ul; iPoint < target.Fine().GetnPointDomain(); ++iPoint) {
      if (target.Fine().nodes->GetBoundary(iPoint)) continue;
      su2double exact[MAXVAR] = {};
      AffinePsi(2, target.Fine().nodes->GetCoord(iPoint), exact);
      for (unsigned short iVar = 0; iVar < 4; ++iVar) {
        scale = std::max(scale, std::fabs(SU2_TYPE::GetValue(exact[iVar])));
        interior = std::max(interior, std::fabs(SU2_TYPE::GetValue(psi->GetSolution(iPoint, iVar) - exact[iVar])));
      }
    }
    /*--- Interior target points can lie outside the donor polygon near the circle (nearest face): report. ---*/
    WARN("disk: interior max |psi - exact| / scale = " << WorldMax(interior) / WorldMax(scale));
  }

  /*--- The target config lists the markers in another order than the donor config. ---*/
  {
    auto configA = MakeConfig(2, "SOLVER= EULER\n");
    stringstream options("SOLVER= EULER\nMACH_NUMBER= 0.5\nMESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\n"
                         "MARKER_FAR= (lower_b, upper, lower_a, right, left)\nMGLEVEL= 2\nMG_MIN_MESHSIZE= 4\n");
    std::unique_ptr<CConfig> configB;
    {
      Mute mute;
      configB.reset(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
    }
    MeshSolution donor(configA.get(), BoxMesh(2, 4, true), 2);
    AddAdjoint(donor, configA.get());
    SetDonorFields(donor, 2);
    MeshSolution target(configB.get(), BoxMesh(2, 6, true), 2);
    AddAdjoint(target, configB.get());
    CDiscAdjTransfer transfer(Primal(ADAP_TRANSFER::BARYCENTRIC), true);
    {
      Mute mute;
      transfer.Transfer(configB.get(), donor.Donor(), target.geometry, target.solver);
    }
    CHECK(AffineError(target, 2) <= 1e-12);
  }
}

/*--- Errors (run by hand, each must stop all ranks with its message): ---*/

TEST_CASE("Collective error: adjoint solver only on the donor", "[.CollectiveError][adjoint_donor_only]") {
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(2, 4, true), 0);
  AddAdjoint(donor, config.get());
  SetDonorFields(donor, 2);
  MeshSolution target(config.get(), BoxMesh(2, 6, true), 0);
  CDiscAdjTransfer transfer(Primal(ADAP_TRANSFER::BARYCENTRIC), true);
  transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
}

TEST_CASE("Collective error: adjoint solver in another slot on rank 1", "[.CollectiveError][adjoint_rank_layout]") {
  for (const bool warm : {true, false}) {
    auto config = MakeConfig(2, "SOLVER= EULER\n");
    const unsigned short slot = SU2_MPI::GetRank() == 1 ? ADJTURB_SOL : ADJFLOW_SOL;
    MeshSolution donor(config.get(), BoxMesh(2, 4, true), 0);
    AddAdjoint(donor, config.get(), slot);
    MeshSolution target(config.get(), BoxMesh(2, 6, true), 0);
    AddAdjoint(target, config.get(), slot);
    CDiscAdjTransfer transfer(Primal(ADAP_TRANSFER::BARYCENTRIC), warm);
    transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
  }
}

TEST_CASE("Collective error: adjoint target point beyond the distance limit", "[.CollectiveError][adjoint_beyond]") {
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(2, 4, true), 0);
  AddAdjoint(donor, config.get());
  SetDonorFields(donor, 2);
  /*--- Target twice as large: its far points are beyond the limit (the adjoint part alone, the primal one would stop
   *    first). ---*/
  auto mesh = BoxMesh(2, 6, true);
  for (auto& x : mesh.coord) x *= 2.0;
  MeshSolution target(config.get(), mesh, 0);
  AddAdjoint(target, config.get());
  CDiscAdjTransfer transfer(Primal(ADAP_TRANSFER::BARYCENTRIC), true);
  transfer.TransferAdjoint(config.get(), donor.Donor(), target.geometry, target.solver);
}
