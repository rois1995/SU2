/*!
 * \file DistributedTransfer_tests.cpp
 * \brief The distributed solution transfers (MPI_TRANSFER_PLAN.md M1, M2): partition equivalence per the requirements
 *        matrix (0.1) against the same code on one rank (MPI_COMM_SELF, computed in the same run) and against the
 *        gathered (MPI-1) reference, canonical location and nearest-face search, admissibility, memory ceiling.
 *        Valid for any number of ranks: mpirun -n 3 test_driver "[DistributedTransfer]".
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
#include <cstdlib>
#include <cstring>
#include <limits>
#include <map>

#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CDistributedLocator.hpp"
#include "../../../SU2_CFD/include/adaptation/CTransferAdmissibility.hpp"
#include "../../../SU2_CFD/include/solvers/CTurbSolver.hpp"

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

/*--- Deterministic pseudo-random numbers (the same sequence on every rank for the same seed). ---*/
struct Random {
  uint64_t state;
  explicit Random(uint64_t seed) : state(seed * 6364136223846793005ull + 1442695040888963407ull) {}
  uint64_t Next() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  }
  double Uniform() { return (Next() >> 11) * (1.0 / 9007199254740992.0); }
  long Int(long lo, long hi) { return lo + static_cast<long>(Next() % static_cast<uint64_t>(hi - lo + 1)); }
};

/*--- Largest value and number of failing ranks (native MPI, independent of the code under test). ---*/
passivedouble WorldMax(passivedouble value) {
  passivedouble result = value;
#ifdef HAVE_MPI
  MPI_Allreduce(&value, &result, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
#endif
  return result;
}
unsigned long FailedRanks(bool ok) {
  unsigned long bad = ok ? 0 : 1, total = bad;
#ifdef HAVE_MPI
  MPI_Allreduce(&bad, &total, 1, MPI_UNSIGNED_LONG, MPI_SUM, SU2_MPI::GetComm());
#endif
  return total;
}

/*--- The transferred arrays (solution, time n, time n-1; flow, turbulence) of the domain points, by global index. ---*/
using PointValues = std::map<uint64_t, std::vector<su2double>>;
PointValues OwnedValues(const MeshSolution& mesh) {
  PointValues values;
  for (auto iPoint = 0ul; iPoint < mesh.Fine().GetnPointDomain(); ++iPoint) {
    auto& row = values[mesh.Fine().nodes->GetGlobalIndex(iPoint)];
    for (const auto iSol : {FLOW_SOL, TURB_SOL}) {
      auto* solver = mesh.solver[MESH_0][iSol];
      if (solver == nullptr) continue;
      auto* nodes = solver->GetNodes();
      for (auto* array : {&nodes->GetSolution(), &nodes->GetSolution_time_n(), &nodes->GetSolution_time_n1()}) {
        if (array->rows() == 0) continue;
        for (unsigned long iVar = 0; iVar < array->cols(); ++iVar) row.push_back((*array)(iPoint, iVar));
      }
    }
  }
  return values;
}

/*--- Field scales S_f = max(range, max |u|) of the reference values (column f of every row). ---*/
std::vector<passivedouble> FieldScales(const PointValues& values) {
  std::vector<passivedouble> lo, hi, mag;
  for (const auto& entry : values) {
    const auto& row = entry.second;
    if (lo.empty()) {
      lo.assign(row.size(), 1e300);
      hi.assign(row.size(), -1e300);
      mag.assign(row.size(), 0.0);
    }
    for (auto f = 0ul; f < row.size(); ++f) {
      const passivedouble v = SU2_TYPE::GetValue(row[f]);
      lo[f] = std::min(lo[f], v);
      hi[f] = std::max(hi[f], v);
      mag[f] = std::max(mag[f], std::fabs(v));
    }
  }
  for (auto f = 0ul; f < lo.size(); ++f) mag[f] = std::max(mag[f], hi[f] - lo[f]);
  return mag;
}

/*--- Largest |difference| / S_f of this rank's owned values to the reference (all ranks), and the number of values
 *    that are not bitwise equal (all ranks). ---*/
void CompareValues(const PointValues& values, const PointValues& reference, passivedouble& worst,
                   unsigned long& nNotBitwise, bool& complete) {
  const auto scale = FieldScales(reference);
  passivedouble diff = 0.0;
  unsigned long n = 0;
  bool ok = true;
  for (const auto& entry : values) {
    const auto it = reference.find(entry.first);
    if (it == reference.end() || it->second.size() != entry.second.size()) {
      ok = false;
      continue;
    }
    for (auto f = 0ul; f < entry.second.size(); ++f) {
      const passivedouble a = SU2_TYPE::GetValue(entry.second[f]), b = SU2_TYPE::GetValue(it->second[f]);
      const passivedouble s = scale[f] > 0.0 ? scale[f] : 1.0;
      diff = std::max(diff, std::fabs(a - b) / s);
      if (!(a == b)) n++;
    }
  }
  worst = WorldMax(diff);
  nNotBitwise = CPassiveComm::AllreduceSum(n);
  complete = FailedRanks(ok) == 0;
}

/*--- Every rank's stencil decisions, sorted by global index (on every rank). ---*/
using Record = CBarycentricTransfer::StencilRecord;
std::vector<Record> AllRecords(const std::vector<Record>& local) {
  std::vector<char> bytes(local.size() * sizeof(Record));
  if (!local.empty()) std::memcpy(bytes.data(), local.data(), bytes.size());
  const auto all = CPassiveComm::AllgathervRounds(bytes.data(), bytes.size(), nullptr);
  std::vector<Record> records(all.size() / sizeof(Record));
  if (!records.empty()) std::memcpy(records.data(), all.data(), all.size());
  std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) { return a.gid < b.gid; });
  return records;
}

/*--- The same decision: element or face, key, donor points (and the marker if both use config positions). ---*/
bool SameDecision(const Record& a, const Record& b, bool compareMarker) {
  bool same = a.gid == b.gid && a.onFace == b.onFace && a.inside == b.inside && a.beyondLimit == b.beyondLimit &&
              a.key == b.key && (!compareMarker || !a.onFace || a.marker == b.marker);
  for (int k = 0; k < 4; ++k) same &= a.point[k] == b.point[k];
  return same;
}
bool SameDecisions(const std::vector<Record>& a, const std::vector<Record>& b, bool compareMarker,
                   unsigned long* nWeightsNotBitwise = nullptr) {
  if (a.size() != b.size()) return false;
  bool same = true;
  unsigned long n = 0;
  for (auto i = 0ul; i < a.size(); ++i) {
    same &= SameDecision(a[i], b[i], compareMarker);
    for (int k = 0; k < 4; ++k) n += !(a[i].weight[k] == b[i].weight[k]);
  }
  if (nWeightsNotBitwise != nullptr) *nWeightsNotBitwise = n;
  return same;
}

/*--- Donor fields: affine or smooth flow for U^n (solution and Solution_time_n) and another field for U^(n-1);
 *    turbulence: SA nu_tilde (sign change for the negative variant), SST k and omega with rho k, rho omega affine. ---*/
enum class Turb { NONE, SA, SA_NEG, SST };
void SetFields(MeshSolution& donor, unsigned short nDim, bool affine, Turb turb) {
  auto* flow = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
  auto* turbNodes = donor.solver[MESH_0][TURB_SOL] ? donor.solver[MESH_0][TURB_SOL]->GetNodes() : nullptr;
  for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
    const auto* x = donor.Fine().nodes->GetCoord(iPoint);
    su2double U[MAXVAR] = {}, Un1[MAXVAR] = {};
    if (affine) {
      AffineFlow(nDim)(x, U);
    } else {
      SmoothFlow(nDim, x, U);
    }
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) Un1[iVar] = U[iVar] * (1.0 + 0.01 * x[0]);
    for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
      flow->GetSolution()(iPoint, iVar) = U[iVar];
      if (flow->GetSolution_time_n().rows() > 0) flow->GetSolution_time_n()(iPoint, iVar) = U[iVar];
      if (flow->GetSolution_time_n1().rows() > 0) flow->GetSolution_time_n1()(iPoint, iVar) = Un1[iVar];
    }
    if (turbNodes == nullptr) continue;
    su2double v[2] = {}, vn1[2] = {};
    switch (turb) {
      case Turb::SA:
        v[0] = 1e-4 + 2e-5 * x[0] - 1e-5 * x[1];
        vn1[0] = 1.1 * v[0];
        break;
      case Turb::SA_NEG:
        v[0] = -5e-5 + 1e-4 * x[0] - 2e-5 * x[1];
        vn1[0] = 0.9 * v[0];
        break;
      case Turb::SST:
        v[0] = (1.0 + 0.1 * x[0] - 0.2 * x[1]) / U[0];
        v[1] = (1e3 + 100.0 * x[0] + 50.0 * x[1]) / U[0];
        vn1[0] = 1.05 * v[0];
        vn1[1] = 0.95 * v[1];
        break;
      default:
        break;
    }
    for (unsigned short iVar = 0; iVar < donor.solver[MESH_0][TURB_SOL]->GetnVar(); ++iVar) {
      turbNodes->GetSolution()(iPoint, iVar) = v[iVar];
      if (turbNodes->GetSolution_time_n().rows() > 0) turbNodes->GetSolution_time_n()(iPoint, iVar) = v[iVar];
      if (turbNodes->GetSolution_time_n1().rows() > 0) turbNodes->GetSolution_time_n1()(iPoint, iVar) = vn1[iVar];
    }
  }
}

/*--- Box mesh scaled about its centre (a target slightly larger than the donor: points outside the donor hull). ---*/
CSimplexMesh Scaled(CSimplexMesh mesh, passivedouble factor) {
  if (factor == 1.0) return mesh;
  const unsigned short nDim = mesh.nDim;
  const passivedouble centre[3] = {nDim == 2 ? 1.0 : 0.5, 0.5, 0.5};
  for (auto iPoint = 0ul; iPoint < mesh.GetnPoint(); ++iPoint)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      auto& x = mesh.coord[iPoint * nDim + iDim];
      x = centre[iDim] + factor * (x - centre[iDim]);
    }
  return mesh;
}

/*--- Barycentric transfer of a case; values of the owned points and the decisions of all points. ---*/
struct BaryCase {
  std::string name;
  unsigned short nDim;
  std::string options;
  bool affine;
  Turb turb;
  unsigned long nDonor, nTarget;
  passivedouble scale;
};

struct BaryResult {
  PointValues values;
  std::vector<Record> records;
  CBarycentricTransfer::Summary summary;
  passivedouble exactness = 0.0;
};

BaryResult RunBarycentric(const BaryCase& test, bool gathered, bool checkCoarse) {
  BaryResult result;
  auto config = MakeConfig(test.nDim, test.options);
  MeshSolution donor(config.get(), BoxMesh(test.nDim, test.nDonor, true), 2);
  SetFields(donor, test.nDim, test.affine, test.turb);
  MeshSolution target(config.get(), Scaled(BoxMesh(test.nDim, test.nTarget, true), test.scale), 2);
  CBarycentricTransfer transfer(true, gathered);
  transfer.KeepStencils(true);
  {
    Mute mute;
    transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
  }
  result.values = OwnedValues(target);
  result.records = AllRecords(transfer.GetStencils());
  result.summary = transfer.GetSummary();
  if (test.affine && test.scale == 1.0) {
    passivedouble maxDiff = 0.0;
    const auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
    for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
      su2double exact[MAXVAR] = {};
      AffineFlow(test.nDim)(target.Fine().nodes->GetCoord(iPoint), exact);
      for (unsigned short iVar = 0; iVar < test.nDim + 2; ++iVar)
        maxDiff = std::max(maxDiff, RelDiff(nodes->GetSolution(iPoint, iVar), exact[iVar]));
    }
    result.exactness = WorldMax(maxDiff);
  }
  if (checkCoarse) {
    CheckCoarseLevels(target, FLOW_SOL);
    if (test.turb != Turb::NONE) CheckCoarseLevels(target, TURB_SOL);
  }
  return result;
}

const std::string kRans = "SOLVER= RANS\nREYNOLDS_NUMBER= 1e6\n";
const std::string kTime2 = "TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n";
const std::string kTime1 = "TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-1ST_ORDER\n";

}  // namespace

TEST_CASE("Distributed barycentric transfer: partition equivalence", "[AdaptationMPI][DistributedTransfer]") {
  /*--- Requirements matrix (0.1), barycentric column: stencil decisions equal for every P (required), values within
   *    1e-12 S_f of P = 1 (bitwise expected, reported), the gathered (MPI-1) reference with the same rules gives the
   *    same decisions and values; affine fields exact; coarse levels the restriction. ---*/
  const std::vector<BaryCase> cases = {
      {"2D Euler affine", 2, "SOLVER= EULER\n", true, Turb::NONE, 4, 6, 1.0},
      {"3D Euler affine", 3, "SOLVER= EULER\n", true, Turb::NONE, 3, 4, 1.0},
      {"2D SA smooth 2nd order", 2, kRans + "KIND_TURB_MODEL= SA\n" + kTime2, false, Turb::SA, 4, 6, 1.0},
      {"2D SA-neg 1st order", 2, kRans + "KIND_TURB_MODEL= SA\nSA_OPTIONS= (NEGATIVE, WITHFT2)\n" + kTime1, false,
       Turb::SA_NEG, 4, 6, 1.0},
      {"2D SST affine 2nd order", 2, kRans + "KIND_TURB_MODEL= SST\n" + kTime2, true, Turb::SST, 4, 6, 1.0},
      {"3D SST smooth", 3, kRans + "KIND_TURB_MODEL= SST\n", false, Turb::SST, 3, 4, 1.0},
      {"2D identity (vertex ties)", 2, "SOLVER= EULER\n", false, Turb::NONE, 4, 4, 1.0},
      {"3D identity (vertex ties)", 3, "SOLVER= EULER\n", false, Turb::NONE, 3, 3, 1.0},
      {"2D coarse donor, fine target, outside the hull", 2, "SOLVER= EULER\n", false, Turb::NONE, 2, 8, 1.02},
      {"2D fine donor, coarse target", 2, "SOLVER= EULER\n", false, Turb::NONE, 8, 2, 1.0},
      {"3D outside the hull (corners)", 3, "SOLVER= EULER\n", false, Turb::NONE, 2, 4, 1.03},
  };
  for (const auto& test : cases) {
    SECTION(test.name) {
      BaryResult serial;
      Serial([&]() { serial = RunBarycentric(test, false, false); });
      const auto parallel = RunBarycentric(test, false, true);
      const auto gathered = RunBarycentric(test, true, false);

      /*--- Decisions: equal for every P; the gathered reference: same elements and faces (its marker positions are
       *    those of the gathered mesh). ---*/
      unsigned long nWeights = 0;
      CHECK(serial.records.size() == parallel.summary.nPoint);
      CHECK(SameDecisions(parallel.records, serial.records, true, &nWeights));
      CHECK(nWeights == 0);
      CHECK(SameDecisions(parallel.records, gathered.records, false));

      passivedouble worst = 0.0, worstGathered = 0.0;
      unsigned long nNotBitwise = 0, nNotBitwiseGathered = 0;
      bool complete = false, completeGathered = false;
      CompareValues(parallel.values, serial.values, worst, nNotBitwise, complete);
      CompareValues(parallel.values, gathered.values, worstGathered, nNotBitwiseGathered, completeGathered);
      CHECK(complete);
      CHECK(completeGathered);
      CHECK(worst <= 1e-12);
      CHECK(worstGathered <= 1e-12);
      /*--- Reported: bitwise equality (expected with the same kernels and inputs). ---*/
      CHECK(nNotBitwise == 0);
      CHECK(nNotBitwiseGathered == 0);
      if (test.affine) CHECK(parallel.exactness < 1e-12);

      /*--- Statistics: the same over the ranks. ---*/
      CHECK(parallel.summary.nPoint == serial.summary.nPoint);
      CHECK(parallel.summary.nOutside == serial.summary.nOutside);
      CHECK(parallel.summary.nFlowFixed == serial.summary.nFlowFixed);
      CHECK(parallel.summary.nTurbLimited == serial.summary.nTurbLimited);
      CHECK(parallel.summary.maxDistance == serial.summary.maxDistance);
      if (test.scale > 1.0) CHECK(parallel.summary.nOutside > 0);
      for (auto f = 0ul; f < serial.summary.newIntegral.size(); ++f)
        CHECK(RelDiff(parallel.summary.newIntegral[f], serial.summary.newIntegral[f], 1e-300) < 1e-13);
      REQUIRE(parallel.summary.roundTripL2.size() == serial.summary.roundTripL2.size());
      for (auto f = 0ul; f < serial.summary.roundTripL2.size(); ++f) {
        CHECK(RelDiff(parallel.summary.roundTripL2[f], serial.summary.roundTripL2[f], 1e-300) < 1e-12);
        CHECK(parallel.summary.roundTripLinf[f] == serial.summary.roundTripLinf[f]);
      }
      if (test.nDonor == test.nTarget && test.scale == 1.0) {
        /*--- Identity: every new point is a donor vertex (weights a unit vector up to round-off; values the donor's). */
        CHECK(parallel.summary.roundTripLinf[0] < 1e-14);
        bool unit = true;
        for (const auto& r : parallel.records) {
          int nOne = 0;
          for (int k = 0; k < 4; ++k) nOne += std::fabs(r.weight[k] - 1.0) < 1e-14;
          unit &= nOne == 1;
        }
        CHECK(unit);
      }
    }
  }
}

TEST_CASE("Distributed barycentric transfer: donor arrays unchanged (read-only round trip)",
          "[AdaptationMPI][DistributedTransfer]") {
  auto config = MakeConfig(2, kRans + "KIND_TURB_MODEL= SST\n" + kTime2);
  MeshSolution donor(config.get(), BoxMesh(2, 4, true), 2);
  SetFields(donor, 2, false, Turb::SST);
  /*--- Stale halo values (FIXED_POINT kept donor): the transfer reads only owned donor values. ---*/
  std::vector<su2activematrix> before;
  for (const auto iSol : {FLOW_SOL, TURB_SOL}) {
    auto* nodes = donor.solver[MESH_0][iSol]->GetNodes();
    for (auto* array : {&nodes->GetSolution(), &nodes->GetSolution_time_n(), &nodes->GetSolution_time_n1()}) {
      for (auto iPoint = donor.Fine().GetnPointDomain(); iPoint < donor.Fine().GetnPoint(); ++iPoint)
        for (unsigned long iVar = 0; iVar < array->cols(); ++iVar) (*array)(iPoint, iVar) = 1e30;
      before.push_back(*array);
    }
  }
  MeshSolution target(config.get(), BoxMesh(2, 6, true), 2);
  CBarycentricTransfer transfer;
  {
    Mute mute;
    transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
  }
  bool unchanged = true;
  size_t i = 0;
  for (const auto iSol : {FLOW_SOL, TURB_SOL}) {
    auto* nodes = donor.solver[MESH_0][iSol]->GetNodes();
    for (auto* array : {&nodes->GetSolution(), &nodes->GetSolution_time_n(), &nodes->GetSolution_time_n1()}) {
      const auto& old = before[i++];
      for (auto k = 0ul; k < array->size(); ++k) unchanged &= array->data()[k] == old.data()[k];
    }
  }
  CHECK(FailedRanks(unchanged) == 0);
  /*--- The stale halos did not reach the new mesh. ---*/
  bool finite = true;
  for (const auto& entry : OwnedValues(target))
    for (const auto& v : entry.second) finite &= std::fabs(SU2_TYPE::GetValue(v)) < 1e10;
  CHECK(FailedRanks(finite) == 0);
  for (const auto r : transfer.GetSummary().roundTripL2) CHECK(std::isfinite(SU2_TYPE::GetValue(r)));
}

TEST_CASE("Distributed barycentric transfer: nonfinite and out-of-bound donor turbulence",
          "[AdaptationMPI][DistributedTransfer]") {
  /*--- A NaN SA nu_tilde, a NaN SST omega, an SST omega above the solver bound at one donor point with otherwise valid
   *    states: interpolated states that use it are rejected by the predicate (NaN) or bounded; the paired fallback
   *    takes another donor point; every transferred value is finite and within the bounds, equal for every P. ---*/
  struct Case {
    std::string name, options;
    Turb turb;
    unsigned short iVar;
    passivedouble value;
  };
  const std::vector<Case> cases = {
      {"SA NaN", kRans + "KIND_TURB_MODEL= SA\n", Turb::SA, 0, std::numeric_limits<passivedouble>::quiet_NaN()},
      {"SST NaN omega", kRans + "KIND_TURB_MODEL= SST\n", Turb::SST, 1, std::numeric_limits<passivedouble>::quiet_NaN()},
      {"SST omega above the bound", kRans + "KIND_TURB_MODEL= SST\n", Turb::SST, 1, 1e20},
  };
  for (const auto& test : cases) {
    SECTION(test.name) {
      auto run = [&](PointValues& values, unsigned long& nFixed) {
        auto config = MakeConfig(2, test.options);
        MeshSolution donor(config.get(), BoxMesh(2, 4, false), 0);
        SetFields(donor, 2, true, test.turb);
        auto* turb = donor.solver[MESH_0][TURB_SOL]->GetNodes();
        for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
          const auto* x = donor.Fine().nodes->GetCoord(iPoint);
          if (std::fabs(SU2_TYPE::GetValue(x[0]) - 1.0) < 1e-12 && std::fabs(SU2_TYPE::GetValue(x[1]) - 0.5) < 1e-12)
            turb->GetSolution()(iPoint, test.iVar) = test.value;
        }
        MeshSolution target(config.get(), BoxMesh(2, 8, true), 0);
        CBarycentricTransfer transfer;
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        values = OwnedValues(target);
        nFixed = transfer.GetSummary().nFlowFixed;
        const auto* turbSolver = dynamic_cast<const CTurbSolver*>(target.solver[MESH_0][TURB_SOL]);
        bool ok = true;
        for (const auto& entry : values) {
          for (const auto& v : entry.second) ok &= std::isfinite(SU2_TYPE::GetValue(v));
        }
        for (auto iPoint = 0ul; iPoint < target.Fine().GetnPointDomain(); ++iPoint)
          for (unsigned short iVar = 0; iVar < turbSolver->GetnVar(); ++iVar) {
            const auto v = turbSolver->GetNodes()->GetSolution(iPoint, iVar);
            ok &= v >= turbSolver->GetLowerLimit(iVar) && v <= turbSolver->GetUpperLimit(iVar);
          }
        CHECK(FailedRanks(ok) == 0);
      };
      PointValues serial, parallel;
      unsigned long nSerial = 0, nParallel = 0;
      Serial([&]() { run(serial, nSerial); });
      run(parallel, nParallel);
      CHECK(nParallel == nSerial);
      if (std::isnan(test.value)) CHECK(nParallel > 0);
      passivedouble worst = 0.0;
      unsigned long nNotBitwise = 0;
      bool complete = false;
      CompareValues(parallel, serial, worst, nNotBitwise, complete);
      CHECK(complete);
      CHECK(nNotBitwise == 0);
    }
  }
}

TEST_CASE("Distributed barycentric transfer: paired fallback at partition interfaces",
          "[AdaptationMPI][DistributedTransfer]") {
  /*--- Inadmissible donor states (zero total energy) at every fifth grid point ((i + 2j) mod 5 = 0: at most one per
   *    element), across the partition interfaces: the interpolated states near them are not admissible and take the
   *    flow state of the admissible donor point of largest weight, equal to P = 1. ---*/
  auto run = [&](PointValues& values, unsigned long& nFixed) {
    auto config = MakeConfig(2, "SOLVER= EULER\n" + kTime2);
    MeshSolution donor(config.get(), BoxMesh(2, 4, false), 0);
    SetFields(donor, 2, true, Turb::NONE);
    auto* flow = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
    for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
      const auto* x = donor.Fine().nodes->GetCoord(iPoint);
      const long i = lround(SU2_TYPE::GetValue(x[0]) * 4), j = lround(SU2_TYPE::GetValue(x[1]) * 4);
      if ((i + 2 * j) % 5 != 0) continue;
      flow->GetSolution()(iPoint, 3) = 0.0;
      flow->GetSolution_time_n()(iPoint, 3) = 0.0;
    }
    MeshSolution target(config.get(), BoxMesh(2, 6, true), 0);
    CBarycentricTransfer transfer;
    {
      Mute mute;
      transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
    }
    values = OwnedValues(target);
    nFixed = transfer.GetSummary().nFlowFixed;
  };
  PointValues serial, parallel;
  unsigned long nSerial = 0, nParallel = 0;
  Serial([&]() { run(serial, nSerial); });
  run(parallel, nParallel);
  CHECK(nSerial > 0);
  CHECK(nParallel == nSerial);
  passivedouble worst = 0.0;
  unsigned long nNotBitwise = 0;
  bool complete = false;
  CompareValues(parallel, serial, worst, nNotBitwise, complete);
  CHECK(complete);
  CHECK(nNotBitwise == 0);
}

TEST_CASE("Canonical nearest face: counterexamples and brute force", "[AdaptationMPI][DistributedTransfer]") {
  auto makeFaces = [](const std::vector<std::vector<passivedouble>>& triangles, const std::vector<uint32_t>& markers) {
    CBoundaryFaces faces;
    faces.nDim = 3;
    uint64_t node = 0;
    for (auto t = 0ul; t < triangles.size(); ++t) {
      faces.marker.push_back(markers[t]);
      for (int k = 0; k < 3; ++k) {
        faces.faceGid.push_back(node);
        faces.nodeGid.push_back(node++);
        for (int d = 0; d < 3; ++d) faces.nodeCoord.push_back(triangles[t][3 * k + d]);
      }
    }
    return faces;
  };
  auto bruteForce = [](const CBoundaryFaces& faces, const passivedouble* x) {
    CFaceHit best;
    for (auto f = 0ul; f < faces.GetnFace(); ++f) {
      const passivedouble* nodes[3];
      for (int k = 0; k < 3; ++k) {
        const auto it = std::find(faces.nodeGid.begin(), faces.nodeGid.end(), faces.faceGid[3 * f + k]);
        nodes[k] = &faces.nodeCoord[3 * (it - faces.nodeGid.begin())];
      }
      CFaceHit hit;
      hit.found = true;
      CanonicalClosestPoint(3, nodes, x, hit.weight, &hit.distance, &hit.faceSize);
      hit.marker = faces.marker[f];
      hit.key = MakeSimplexKey(&faces.faceGid[3 * f], 3);
      if (BetterFace(hit, best)) best = hit;
    }
    return best;
  };

  for (const passivedouble shift : {0.0, 1e6}) {
    SECTION("tolerance halo of the ADT (round 2), shift " + std::to_string(shift)) {
      /*--- A = (0,0,0),(1,0,0),(0,1,0); x = (0.5, 0.5 + 4e-11, 0) is 2.83e-11 outside A's hypotenuse; B lies under x
       *    at z = 1e-11: B is the nearest face, whatever face the ADT starts from. ---*/
      const passivedouble s = shift;
      const std::vector<std::vector<passivedouble>> triangles = {
          {s, s, s, s + 1, s, s, s, s + 1, s},
          {s + 0.4, s + 0.4, s + 1e-11, s + 0.6, s + 0.4, s + 1e-11, s + 0.5, s + 0.7, s + 1e-11}};
      for (const auto& markers : {std::vector<uint32_t>{0, 0}, std::vector<uint32_t>{0, 1}}) {
        const auto faces = makeFaces(triangles, markers);
        CCanonicalBoundary boundary(faces, std::sqrt(3.0));
        const passivedouble x[3] = {s + 0.5, s + 0.5 + 4e-11, s};
        const auto hit = boundary.NearestAny(x);
        const auto expected = bruteForce(faces, x);
        CHECK(hit.found);
        CHECK(hit.key == expected.key);
        CHECK(hit.distance == expected.distance);
        if (shift == 0.0) {
          CHECK(hit.key == CSimplexKey({3, 4, 5, UINT64_MAX}));
          CHECK(hit.distance == Approx(1e-11).epsilon(1e-3));
        }
      }
    }
    SECTION("distant face, query at the origin (round 3), shift " + std::to_string(shift)) {
      /*--- One triangle at x = 1e6, queried at the origin: canonical U = 999999.9999999999 while the inflated box is
       *    1e6 away; the margin 1e-12 (U + max|x| + L) keeps the face a candidate. ---*/
      const passivedouble s = shift;
      const std::vector<std::vector<passivedouble>> triangles = {
          {s + 1e6, s - 1, s - 1, s + 1e6, s + 5, s - 1, s + 1e6, s - 1, s + 5}};
      const auto faces = makeFaces(triangles, {0});
      CCanonicalBoundary boundary(faces, std::sqrt(72.0));
      const passivedouble x[3] = {s, s, s};
      const auto hit = boundary.NearestAny(x);
      CHECK(hit.found);
      CHECK(hit.distance == bruteForce(faces, x).distance);
      CHECK(hit.distance == Approx(1e6).epsilon(1e-12));
    }
  }

  SECTION("random surfaces: disconnected, symmetric ties, thin gap, zero distance, marker junctions") {
    std::vector<std::vector<passivedouble>> triangles;
    std::vector<uint32_t> markers;
    /*--- Two parallel plates (thin gap 1e-9 at z = 0 and z = 1e-9, mirrored copy at z = 2: symmetric ties for points at
     *    z = 1), a disconnected tilted square, plates split over two markers (junction at x = 0.5). ---*/
    const int n = 4;
    for (const passivedouble z : {0.0, 1e-9, 2.0}) {
      for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j) {
          const passivedouble x0 = passivedouble(i) / n, y0 = passivedouble(j) / n, h = 1.0 / n;
          const uint32_t m = (z == 2.0) ? 2 : (x0 < 0.5 ? 0 : 1);
          triangles.push_back({x0, y0, z, x0 + h, y0, z, x0 + h, y0 + h, z});
          markers.push_back(m);
          triangles.push_back({x0, y0, z, x0 + h, y0 + h, z, x0, y0 + h, z});
          markers.push_back(m);
        }
    }
    triangles.push_back({3.0, 3.0, 3.0, 4.0, 3.0, 3.5, 3.0, 4.0, 3.2});
    markers.push_back(3);
    triangles.push_back({4.0, 3.0, 3.5, 4.0, 4.0, 3.7, 3.0, 4.0, 3.2});
    markers.push_back(3);
    for (const passivedouble shift : {0.0, 1e3}) {
      auto shifted = triangles;
      for (auto& t : shifted)
        for (auto& c : t) c += shift;
      const auto faces = makeFaces(shifted, markers);
      CCanonicalBoundary boundary(faces, 8.0);
      Random random(5);
      bool ok = true;
      for (int q = 0; q < 2000; ++q) {
        passivedouble x[3];
        const int kind = q % 4;
        if (kind == 0) {
          for (int d = 0; d < 3; ++d) x[d] = 5.0 * random.Uniform() - 0.5;
        } else if (kind == 1) {
          /*--- On a node (zero distance, several faces) or an edge midpoint. ---*/
          const auto f = static_cast<unsigned long>(random.Int(0, faces.GetnFace() - 1));
          for (int d = 0; d < 3; ++d) x[d] = faces.nodeCoord[(3 * f) * 3 + d] - shift;
        } else if (kind == 2) {
          /*--- Equidistant from z = 1e-9 and z = 2 (the symmetric ties) and in the gap. ---*/
          x[0] = std::round(4.0 * random.Uniform()) / 4.0;
          x[1] = random.Uniform();
          x[2] = (q % 8 == 2) ? 1.0 + 0.5e-9 : 0.5e-9;
        } else {
          x[0] = 0.5;
          x[1] = random.Uniform();
          x[2] = -0.1 * random.Uniform();
        }
        for (int d = 0; d < 3; ++d) x[d] += shift;
        const auto hit = boundary.NearestAny(x);
        const auto expected = bruteForce(faces, x);
        ok &= hit.found && hit.key == expected.key && hit.marker == expected.marker &&
              hit.distance == expected.distance;
        /*--- Restricted to markers 0 and 1 (the junction). ---*/
        const auto restricted = boundary.Nearest(x, {1, 0});
        CFaceHit best;
        for (auto f = 0ul; f < faces.GetnFace(); ++f) {
          if (faces.marker[f] > 1) continue;
          CBoundaryFaces one;
          one.nDim = 3;
          one.marker = {faces.marker[f]};
          one.faceGid.assign(&faces.faceGid[3 * f], &faces.faceGid[3 * f] + 3);
          one.nodeGid = faces.nodeGid;
          one.nodeCoord = faces.nodeCoord;
          const auto h = bruteForce(one, x);
          if (BetterFace(h, best)) best = h;
        }
        ok &= restricted.key == best.key && restricted.distance == best.distance;
      }
      CHECK(ok);
    }
  }
}

TEST_CASE("Canonical nearest face: replicated boundary of a partitioned mesh", "[AdaptationMPI][DistributedTransfer]") {
  /*--- The faces gathered from the ranks (markers absent on some ranks, faces in the partition's node order) answer
   *    every query as the boundary of the complete mesh on one rank. ---*/
  for (const unsigned short nDim : {2, 3}) {
    auto config = MakeConfig(nDim, "SOLVER= EULER\n");
    const auto input = BoxMesh(nDim, nDim == 2 ? 6 : 3, true);
    MeshSolution mesh(config.get(), input, 0);
    double xMin[3], xMax[3], diagonal = 0.0;
    GlobalBoundingBox(mesh.Fine(), xMin, xMax, &diagonal);
    CCanonicalBoundary distributed(CBoundaryFaces::Gather(mesh.Fine(), mesh.markerTags, *config), diagonal);
    std::vector<std::pair<CSimplexKey, passivedouble>> parallel, serial;
    std::vector<passivedouble> queries;
    Random random(9);
    for (int q = 0; q < 500; ++q)
      for (unsigned short d = 0; d < nDim; ++d) queries.push_back((d == 0 && nDim == 2 ? 2.4 : 1.2) * random.Uniform() - 0.1);
    for (int q = 0; q < 500; ++q) {
      const auto hit = distributed.NearestAny(&queries[q * nDim]);
      parallel.emplace_back(hit.key, hit.distance);
    }
    Serial([&]() {
      auto serialConfig = MakeConfig(nDim, "SOLVER= EULER\n");
      MeshSolution whole(serialConfig.get(), input, 0);
      CCanonicalBoundary boundary(CBoundaryFaces::Gather(whole.Fine(), whole.markerTags, *serialConfig), diagonal);
      for (int q = 0; q < 500; ++q) {
        const auto hit = boundary.NearestAny(&queries[q * nDim]);
        serial.emplace_back(hit.key, hit.distance);
      }
    });
    CHECK(FailedRanks(parallel == serial) == 0);
  }
}

TEST_CASE("Distributed point location: query chunks under a memory ceiling", "[AdaptationMPI][DistributedTransfer]") {
  /*--- The location of many points in chunks of bounded size gives the result of one chunk. All ranks query points of
   *    one corner of the donor (an overloaded owner rank, empty peers): the queries a rank receives per chunk are
   *    bounded by the chunk count. ---*/
  for (const unsigned short nDim : {2, 3}) {
    auto config = MakeConfig(nDim, "SOLVER= EULER\n");
    MeshSolution donor(config.get(), BoxMesh(nDim, nDim == 2 ? 8 : 4, true), 0);
    CDistributedLocator locator(donor.Fine(), donor.markerTags, *config, 0.0);
    const int rank = SU2_MPI::GetRank();
    for (const bool corner : {false, true}) {
      std::vector<passivedouble> coord;
      Random random(31 + rank);
      for (int q = 0; q < 400; ++q)
        for (unsigned short d = 0; d < nDim; ++d)
          coord.push_back(corner ? 0.2 * random.Uniform() : (d == 0 && nDim == 2 ? 2.0 : 1.0) * random.Uniform());
      const auto reference = locator.LocateElements(coord);
      const auto chunksDefault = locator.GetLastChunks();
      const auto received = locator.GetLastQueriesReceived();
      const auto saved = GetTransferMemoryCeiling();
      const size_t resident = locator.GetMemory() + 400 * sizeof(CElementHit);
      const size_t ceiling = CPassiveComm::AllreduceMax(resident) + 3000;
      SetTransferMemoryCeiling(ceiling);
      const auto chunked = locator.LocateElements(coord);
      const auto nChunk = locator.GetLastChunks();
      const auto maxPerChunk = locator.GetLastMaxChunkReceived();
      SetTransferMemoryCeiling(saved);
      bool same = reference.size() == chunked.size();
      for (auto i = 0ul; same && i < reference.size(); ++i)
        same = reference[i].found == chunked[i].found && reference[i].key == chunked[i].key &&
               reference[i].minWeight == chunked[i].minWeight;
      CHECK(chunksDefault == 1);
      CHECK(nChunk > 1);
      CHECK(FailedRanks(same) == 0);
      CHECK(maxPerChunk <= received / nChunk + 1 + SU2_MPI::GetSize());
    }
  }
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- Collective errors: hidden tests (they stop the run), run by hand to check the message on 1..n ranks, e.g.   ---*/
/*--- SU2_COLLECTIVE_ERROR_TESTS=1 mpirun -n 3 test_driver "[.CollectiveError][beyond]" (without the variable they do ---*/
/*--- nothing: test specs made of exclusions only also select hidden tests).                                       ---*/
/*------------------------------------------------------------------------------------------------------------------*/

namespace {
bool CollectiveErrorTests() { return std::getenv("SU2_COLLECTIVE_ERROR_TESTS") != nullptr; }
}  // namespace

TEST_CASE("Collective error: barycentric point beyond the distance limit", "[.CollectiveError][beyond]") {
  if (!CollectiveErrorTests()) return;
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(2, 4, true), 0);
  SetFields(donor, 2, true, Turb::NONE);
  MeshSolution target(config.get(), Scaled(BoxMesh(2, 6, true), 1.5), 0);
  CBarycentricTransfer transfer;
  transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
}

TEST_CASE("Collective error: no admissible donor state", "[.CollectiveError][admissible]") {
  if (!CollectiveErrorTests()) return;
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(2, 4, true), 0);
  SetFields(donor, 2, true, Turb::NONE);
  auto* flow = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
  for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) flow->GetSolution()(iPoint, 3) = -1.0;
  MeshSolution target(config.get(), BoxMesh(2, 6, true), 0);
  CBarycentricTransfer transfer;
  transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
}
