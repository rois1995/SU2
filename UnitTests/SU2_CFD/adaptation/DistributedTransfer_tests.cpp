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
#include "../../../Common/include/adaptation/TransferTolerances.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CDistributedLocator.hpp"
#include "../../../SU2_CFD/include/adaptation/CDistributedProjection.hpp"
#include "../../../SU2_CFD/include/adaptation/CTransferAdmissibility.hpp"
#include "../../../SU2_CFD/include/solvers/CTurbSolver.hpp"

using namespace transfer_test;

/*--- Tests whose tolerances are those of double precision (1e-10..1e-14 of the field scale, bitwise equality with the
 *    gathered reference, exact affine fields): not run in single precision builds, where the single precision contract
 *    is tested instead ("Single precision transfer contract"). ---*/
#define SKIP_IN_SINGLE_PRECISION()                                                      \
  if (transfer_tol::kSinglePrecision) {                                                 \
    WARN("Skipped in single precision: the tolerances are those of double precision."); \
    return;                                                                             \
  }

namespace {

/*--- Arrays of passive values from double expressions (no narrowing in single precision builds). ---*/
std::vector<std::vector<passivedouble>> Passive(const std::vector<std::vector<double>>& values) {
  std::vector<std::vector<passivedouble>> result;
  for (const auto& row : values) result.emplace_back(row.begin(), row.end());
  return result;
}

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
  const passivedouble centre[3] = {passivedouble(nDim == 2 ? 1.0 : 0.5), 0.5, 0.5};
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
  SKIP_IN_SINGLE_PRECISION();
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
  SKIP_IN_SINGLE_PRECISION();
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
      const double s = shift;
      const auto triangles =
          Passive({{s, s, s, s + 1, s, s, s, s + 1, s},
                   {s + 0.4, s + 0.4, s + 1e-11, s + 0.6, s + 0.4, s + 1e-11, s + 0.5, s + 0.7, s + 1e-11}});
      for (const auto& markers : {std::vector<uint32_t>{0, 0}, std::vector<uint32_t>{0, 1}}) {
        const auto faces = makeFaces(triangles, markers);
        CCanonicalBoundary boundary(faces, std::sqrt(3.0));
        const passivedouble x[3] = {passivedouble(s + 0.5), passivedouble(s + 0.5 + 4e-11), passivedouble(s)};
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
      const double s = shift;
      const auto triangles = Passive({{s + 1e6, s - 1, s - 1, s + 1e6, s + 5, s - 1, s + 1e6, s - 1, s + 5}});
      const auto faces = makeFaces(triangles, {0});
      CCanonicalBoundary boundary(faces, std::sqrt(72.0));
      const passivedouble x[3] = {passivedouble(s), passivedouble(s), passivedouble(s)};
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
      /*--- The smallest ceiling the admission accepts (one query per peer and chunk at its limit). ---*/
      const size_t ceiling = locator.GetLastMinimumCeiling();
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
/*--- M2: distributed conservative projection and transfer                                                          ---*/
/*------------------------------------------------------------------------------------------------------------------*/

namespace {

using ValueMap = std::map<uint64_t, std::vector<passivedouble>>;
using FieldFunction = std::function<void(const passivedouble* x, passivedouble* v)>;

/*--- Fields of the projection tests: affine flow, a constant, a smooth field. ---*/
FieldFunction ProjectionFields(unsigned short nDim) {
  return [nDim](const passivedouble* x, passivedouble* v) {
    su2double xs[3] = {x[0], x[1], nDim == 3 ? x[2] : passivedouble(0.0)}, U[MAXVAR];
    AffineFlow(nDim)(xs, U);
    for (unsigned short f = 0; f < nDim + 2; ++f) v[f] = SU2_TYPE::GetValue(U[f]);
    v[nDim + 2] = 2.0;
    v[nDim + 3] = std::sin(3.0 * x[0]) * std::cos(2.0 * x[1]) + (nDim == 3 ? x[2] * x[2] : 0.0);
  };
}

struct ProjectionResult {
  ValueMap values;
  CConservativeProjection::Summary summary;
  std::vector<passivedouble> targetTotal, scale;  /*!< \brief Per field (from the summary). */
  std::vector<passivedouble> newTotal;            /*!< \brief Per field: sum over the ranks of value x CV. */
  passivedouble affineError = 0.0;                /*!< \brief Largest relative error of the affine fields. */
  unsigned long localImports = 0;
};

/*--- The distributed projection (or the serial one on the gathered meshes, oracle = true, run on every rank). ---*/
ProjectionResult RunProjection(unsigned short nDim, const CSimplexMesh& donorMesh, const CSimplexMesh& targetMesh,
                               const std::string& markers, const CConservativeProjection::Options& options,
                               bool oracle, bool roundMesh) {
  ProjectionResult result;
  auto config = roundMesh ? MakeRoundConfig("SOLVER= EULER\n", markers) : MakeConfig(nDim, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), donorMesh, 0), target(config.get(), targetMesh, 0);
  const unsigned short nField = nDim + 4;
  const auto field = ProjectionFields(nDim);
  if (oracle) {
    /*--- Serial projection on the complete meshes (each rank alone: run inside Serial()). ---*/
    CConservativeProjection projection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags, options);
    std::vector<su2double> values(donor.Fine().GetnPoint() * nField), out;
    for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
      passivedouble x[3] = {}, v[MAXVAR] = {};
      for (unsigned short d = 0; d < nDim; ++d) x[d] = SU2_TYPE::GetValue(donor.Fine().nodes->GetCoord(iPoint, d));
      field(x, v);
      for (unsigned short f = 0; f < nField; ++f) values[iPoint * nField + f] = v[f];
    }
    projection.Project(nField, values, out);
    for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
      auto& row = result.values[target.Fine().nodes->GetGlobalIndex(iPoint)];
      for (unsigned short f = 0; f < nField; ++f) row.push_back(SU2_TYPE::GetValue(out[iPoint * nField + f]));
    }
    result.summary = projection.GetSummary();
  } else {
    CDistributedProjection projection(donor.Fine(), donor.markerTags, target.Fine(), target.markerTags, *config,
                                      options);
    const auto nOwned = donor.Fine().GetnPointDomain();
    std::vector<su2double> values(nOwned * nField), out;
    for (auto iPoint = 0ul; iPoint < nOwned; ++iPoint) {
      passivedouble x[3] = {}, v[MAXVAR] = {};
      for (unsigned short d = 0; d < nDim; ++d) x[d] = SU2_TYPE::GetValue(donor.Fine().nodes->GetCoord(iPoint, d));
      field(x, v);
      for (unsigned short f = 0; f < nField; ++f) values[iPoint * nField + f] = v[f];
    }
    projection.Project(nField, values, out);
    const auto nRow = target.Fine().GetnPointDomain();
    passivedouble affine = 0.0;
    std::vector<passivedouble> totals(nField, 0.0);
    for (auto iPoint = 0ul; iPoint < nRow; ++iPoint) {
      auto& row = result.values[target.Fine().nodes->GetGlobalIndex(iPoint)];
      passivedouble x[3] = {}, v[MAXVAR] = {};
      for (unsigned short d = 0; d < nDim; ++d) x[d] = SU2_TYPE::GetValue(target.Fine().nodes->GetCoord(iPoint, d));
      field(x, v);
      for (unsigned short f = 0; f < nField; ++f) {
        const passivedouble value = SU2_TYPE::GetValue(out[iPoint * nField + f]);
        row.push_back(value);
        totals[f] += value * SU2_TYPE::GetValue(target.Fine().nodes->GetVolume(iPoint));
        if (f < nDim + 2) affine = std::max(affine, std::fabs(value - v[f]) / std::max(std::fabs(v[f]), 1e-8));
      }
    }
    result.newTotal.resize(nField);
#ifdef HAVE_MPI
    MPI_Allreduce(totals.data(), result.newTotal.data(), nField, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
#else
    result.newTotal = totals;
#endif
    result.affineError = WorldMax(affine);
    result.summary = projection.GetSummary();
    result.localImports = projection.GetLocalImports();
  }
  return result;
}

/*--- Largest |difference| / S_f of this rank's values to a reference (all ranks), S_f from the reference. ---*/
passivedouble ProjectionDifference(const ValueMap& values, const ValueMap& reference, bool& complete) {
  std::vector<passivedouble> lo, hi, mag;
  for (const auto& entry : reference) {
    if (lo.empty()) {
      lo.assign(entry.second.size(), 1e300);
      hi.assign(entry.second.size(), -1e300);
      mag.assign(entry.second.size(), 0.0);
    }
    for (auto f = 0ul; f < entry.second.size(); ++f) {
      lo[f] = std::min(lo[f], entry.second[f]);
      hi[f] = std::max(hi[f], entry.second[f]);
      mag[f] = std::max(mag[f], std::fabs(entry.second[f]));
    }
  }
  passivedouble worst = 0.0;
  bool ok = true;
  for (const auto& entry : values) {
    const auto it = reference.find(entry.first);
    if (it == reference.end()) {
      ok = false;
      continue;
    }
    for (auto f = 0ul; f < entry.second.size(); ++f) {
      const passivedouble s = std::max(mag[f], hi[f] - lo[f]);
      worst = std::max(worst, std::fabs(entry.second[f] - it->second[f]) / (s > 0.0 ? s : 1.0));
    }
  }
  complete = FailedRanks(ok) == 0;
  return WorldMax(worst);
}

}  // namespace

TEST_CASE("Distributed conservative projection: partition equivalence and exact totals",
          "[AdaptationMPI][DistributedTransfer]") {
  SKIP_IN_SINGLE_PRECISION();
  /*--- Requirements matrix (0.1), conservative column: values within 1e-10 S_f of P = 1 and of the serial projection
   *    on the gathered meshes, totals exact (1e-12 sum |u| V) at every P, measures consistent; affine fields exact
   *    where the domains coincide; slivers and fill with the CLOSED, GLOBAL and NONE rules on the disk and ball. ---*/
  using Rule = CConservativeProjection::SliverRule;
  struct Case {
    std::string name;
    unsigned short nDim;
    bool round;
    unsigned long nDonor, nTarget;
    Rule rule;
    bool open;
  };
  const std::vector<Case> cases = {
      {"2D box", 2, false, 4, 6, Rule::CLOSED, false},
      {"2D box, coarse target", 2, false, 8, 3, Rule::CLOSED, false},
      {"3D box", 3, false, 3, 4, Rule::CLOSED, false},
      {"disk, closed walls", 2, true, 4, 6, Rule::CLOSED, false},
      {"disk, closed open markers", 2, true, 4, 6, Rule::CLOSED, true},
      {"disk, global", 2, true, 6, 4, Rule::GLOBAL, false},
      {"disk, none", 2, true, 4, 6, Rule::NONE, false},
      {"ball, closed walls", 3, true, 3, 4, Rule::CLOSED, false},
      {"ball, none", 3, true, 4, 3, Rule::NONE, false},
  };
  for (const auto& test : cases) {
    SECTION(test.name) {
      auto makeMesh = [&](unsigned long n) {
        return test.round ? simplex_test::MakeRoundMesh(test.nDim, n, 1.0) : BoxMesh(test.nDim, n, true);
      };
      const std::string markers = "MARKER_EULER= (round_a, round_b)\n";
      CConservativeProjection::Options options;
      options.sliverRule = test.rule;
      options.absoluteLimit = 0.02;
      if (test.open) options.openMarkers = {"round_a", "round_b"};
      ProjectionResult serial, oracle;
      Serial([&]() {
        serial = RunProjection(test.nDim, makeMesh(test.nDonor), makeMesh(test.nTarget), markers, options, false,
                               test.round);
        oracle = RunProjection(test.nDim, makeMesh(test.nDonor), makeMesh(test.nTarget), markers, options, true,
                               test.round);
      });
      const auto parallel =
          RunProjection(test.nDim, makeMesh(test.nDonor), makeMesh(test.nTarget), markers, options, false, test.round);
      bool complete = false, completeOracle = false;
      const auto diff = ProjectionDifference(parallel.values, serial.values, complete);
      const auto diffOracle = ProjectionDifference(parallel.values, oracle.values, completeOracle);
      INFO("difference to P = 1: " << diff << ", to the serial oracle: " << diffOracle);
      CHECK(complete);
      CHECK(completeOracle);
      CHECK(diff <= 1e-10);
      CHECK(diffOracle <= 1e-10);

      const auto& s = parallel.summary;
      for (auto f = 0ul; f < s.targetTotal.size(); ++f) {
        CHECK(std::fabs(parallel.newTotal[f] - s.targetTotal[f]) <= 1e-12 * s.scale[f]);
        CHECK(std::fabs(s.targetTotal[f] - serial.summary.targetTotal[f]) <= 1e-12 * s.scale[f]);
      }
      CHECK(s.nPairs == serial.summary.nPairs);
      CHECK(s.nPairs == oracle.summary.nPairs);
      CHECK(s.nFillPieces == serial.summary.nFillPieces);
      CHECK(s.nFillPieces == oracle.summary.nFillPieces);
      CHECK(s.nSliverElems == oracle.summary.nSliverElems);
      CHECK(std::fabs(s.overlapVolume + s.fillVolume - s.targetVolume) < 1e-12 * s.targetVolume);
      CHECK(std::fabs(s.overlapVolume + s.sliverVolume - s.donorVolume) < 1e-12 * s.donorVolume);
      if (test.round) {
        CHECK(s.nFillPieces > 0);
        CHECK(s.nSliverElems > 0);
      } else {
        CHECK(s.nFillPieces == 0);
        CHECK(s.nSliverElems == 0);
        CHECK(parallel.affineError < 1e-11);
      }
    }
  }
}

TEST_CASE("Distributed conservative projection: brute-force supermesh and exact dense solve",
          "[AdaptationMPI][DistributedTransfer]") {
  SKIP_IN_SINGLE_PRECISION();
  /*--- Independent reference on small meshes: S by all-pairs clipping (no search structure), the mass matrix from the
   *    element formula, the dense system solved by Gaussian elimination in long double. The distributed projection
   *    without limiter (the domains coincide, no slivers) must agree to 1e-10 of the field scale at every P. ---*/
  for (const unsigned short nDim : {2, 3}) {
    const auto donorMesh = BoxMesh(nDim, nDim == 2 ? 3 : 2, true), targetMesh = BoxMesh(nDim, nDim == 2 ? 4 : 3, true);
    const unsigned short nNode = nDim + 1, nField = nDim + 4;
    const auto field = ProjectionFields(nDim);
    const auto nD = donorMesh.GetnPoint(), nT = targetMesh.GetnPoint();
    std::vector<passivedouble> U(nD * nField);
    for (auto i = 0ul; i < nD; ++i) field(&donorMesh.coord[i * nDim], &U[i * nField]);

    /*--- Right-hand side by all pairs. ---*/
    struct Context {
      const std::vector<unsigned long>* elem;
      const std::vector<passivedouble>* U;
      unsigned long t, e;
      unsigned short nNode, nField;
      std::vector<long double>* S;
      const std::vector<unsigned long>* telem;
    } ctx;
    std::vector<long double> S(nT * nField, 0.0L);
    ctx.elem = &donorMesh.elem;
    ctx.telem = &targetMesh.elem;
    ctx.U = &U;
    ctx.nNode = nNode;
    ctx.nField = nField;
    ctx.S = &S;
    auto piece = [](void* c, unsigned short i, passivedouble volume, const passivedouble*, const passivedouble* mu) {
      auto& x = *static_cast<Context*>(c);
      const auto row = (*x.telem)[x.t * x.nNode + i];
      for (unsigned short f = 0; f < x.nField; ++f) {
        long double value = 0.0L;
        for (unsigned short k = 0; k < x.nNode; ++k) value += mu[k] * (*x.U)[(*x.elem)[x.e * x.nNode + k] * x.nField + f];
        (*x.S)[row * x.nField + f] += volume * value;
      }
    };
    std::vector<long double> M(nT * nT, 0.0L);
    for (auto t = 0ul; t < targetMesh.GetnElem(); ++t) {
      const passivedouble* nodes[4] = {};
      for (unsigned short k = 0; k < nNode; ++k) nodes[k] = &targetMesh.coord[targetMesh.elem[t * nNode + k] * nDim];
      conservative::Frame frame;
      conservative::SetFrame(nDim, nodes, frame);
      const long double vol = frame.volume;
      const long double diag = (nDim == 2) ? 11.0L / 54.0L : 25.0L / 192.0L;
      const long double off = (1.0L / nNode - diag) / nDim;
      for (unsigned short k = 0; k < nNode; ++k)
        for (unsigned short l = 0; l < nNode; ++l)
          M[targetMesh.elem[t * nNode + k] * nT + targetMesh.elem[t * nNode + l]] += (k == l ? diag : off) * vol;
      ctx.t = t;
      for (auto e = 0ul; e < donorMesh.GetnElem(); ++e) {
        const passivedouble* donorNodes[4] = {};
        for (unsigned short k = 0; k < nNode; ++k) donorNodes[k] = &donorMesh.coord[donorMesh.elem[e * nNode + k] * nDim];
        ctx.e = e;
        bool clipped = false;
        conservative::Overlap(nDim, frame, donorNodes, conservative::SimplexMeasure(nDim, donorNodes), piece, &ctx,
                              &clipped);
      }
    }
    /*--- Dense solve (partial pivoting), all fields at once. ---*/
    std::vector<long double> X = S;
    for (auto c = 0ul; c < nT; ++c) {
      auto pivot = c;
      for (auto r = c + 1; r < nT; ++r)
        if (std::fabs(M[r * nT + c]) > std::fabs(M[pivot * nT + c])) pivot = r;
      for (auto k = 0ul; k < nT; ++k) std::swap(M[c * nT + k], M[pivot * nT + k]);
      for (unsigned short f = 0; f < nField; ++f) std::swap(X[c * nField + f], X[pivot * nField + f]);
      for (auto r = c + 1; r < nT; ++r) {
        const long double factor = M[r * nT + c] / M[c * nT + c];
        if (factor == 0.0L) continue;
        for (auto k = c; k < nT; ++k) M[r * nT + k] -= factor * M[c * nT + k];
        for (unsigned short f = 0; f < nField; ++f) X[r * nField + f] -= factor * X[c * nField + f];
      }
    }
    for (long r = nT - 1; r >= 0; --r)
      for (unsigned short f = 0; f < nField; ++f) {
        long double sum = X[r * nField + f];
        for (auto k = r + 1; k < static_cast<long>(nT); ++k) sum -= M[r * nT + k] * X[k * nField + f];
        X[r * nField + f] = sum / M[r * nT + r];
      }
    CConservativeProjection::Options options;
    options.limiter = false;
    const auto result = RunProjection(nDim, donorMesh, targetMesh, "", options, false, false);

    /*--- The projection keeps the donor's lumped totals (sum u |C|): the exact projection shifted uniformly by the
     *    correction (M 1 = row volumes) over the measure of the domain. ---*/
    long double measure = 0.0L;
    for (auto t = 0ul; t < targetMesh.GetnElem(); ++t) {
      const passivedouble* nodes[4] = {};
      for (unsigned short k = 0; k < nNode; ++k) nodes[k] = &targetMesh.coord[targetMesh.elem[t * nNode + k] * nDim];
      measure += conservative::SimplexMeasure(nDim, nodes);
    }
    ValueMap exact;
    for (auto i = 0ul; i < nT; ++i)
      for (unsigned short f = 0; f < nField; ++f)
        exact[i].push_back(static_cast<passivedouble>(X[i * nField + f] + result.summary.correction[f] / measure));
    bool complete = false;
    const auto diff = ProjectionDifference(result.values, exact, complete);
    INFO("nDim " << nDim << ": difference to the exact dense projection " << diff);
    CHECK(complete);
    CHECK(diff < 1e-10);
  }
}

TEST_CASE("Distributed conservative projection: import groups under a memory ceiling",
          "[AdaptationMPI][DistributedTransfer]") {
  SKIP_IN_SINGLE_PRECISION();
  /*--- A forced small ceiling splits the import into several groups (results within the tolerance of one group:
   *    only the order of the right-hand side sums changes). ---*/
  for (const unsigned short nDim : {2, 3}) {
    CConservativeProjection::Options options;
    const auto donorMesh = BoxMesh(nDim, nDim == 2 ? 8 : 4, true), targetMesh = BoxMesh(nDim, nDim == 2 ? 10 : 5, true);
    const auto reference = RunProjection(nDim, donorMesh, targetMesh, "", options, false, false);
    CHECK(reference.summary.nImportGroups == 1);
    const auto saved = GetTransferMemoryCeiling();
    const size_t ceiling = reference.summary.memoryResident +
                           std::max(reference.summary.memoryLargestBox, reference.summary.memoryImport / 3) + 1;
    SetTransferMemoryCeiling(ceiling);
    const auto grouped = RunProjection(nDim, donorMesh, targetMesh, "", options, false, false);
    SetTransferMemoryCeiling(saved);
    CHECK(grouped.summary.nImportGroups > 1);
    bool complete = false;
    CHECK(ProjectionDifference(grouped.values, reference.values, complete) <= 1e-10);
    CHECK(complete);
    CHECK(grouped.summary.nPairs == reference.summary.nPairs);
    for (auto f = 0ul; f < grouped.newTotal.size(); ++f)
      CHECK(std::fabs(grouped.newTotal[f] - grouped.summary.targetTotal[f]) <= 1e-12 * grouped.summary.scale[f]);
  }
}

TEST_CASE("Conservative kernels: enumeration fixtures", "[AdaptationMPI][DistributedTransfer]") {
  SKIP_IN_SINGLE_PRECISION();
  SECTION("tiny target element inside a donor element (separating-plane counterexample)") {
    /*--- E = (1, 0.79645933750706 45), (0.5, 0.39822966908697405), (0, 0); T a 1e-17-sized triangle at
     *    (0.1365068111787508, 0.10872212439662993), inside E (exact barycentric coordinates 2e-7..4e-7, computed
     *    ones negative): the clip finds the overlap, T fully covered. ---*/
    const passivedouble e[3][2] = {{1.0, 0.7964593375070645}, {0.5, 0.39822966908697405}, {0.0, 0.0}};
    const passivedouble c[2] = {0.1365068111787508, 0.10872212439662993};
    /*--- T: a few ulps (about 1e-16) in size (1e-17 is below the spacing of the doubles there). ---*/
    passivedouble x1 = c[0], y2 = c[1];
    for (int k = 0; k < 4; ++k) {
      x1 = std::nextafter(x1, 1.0);
      y2 = std::nextafter(y2, 1.0);
    }
    const passivedouble t[3][2] = {{c[0], c[1]}, {x1, c[1]}, {c[0], y2}};
    const passivedouble* tNodes[3] = {t[0], t[1], t[2]};
    const passivedouble* eNodes[3] = {e[0], e[1], e[2]};
    conservative::Frame frame;
    conservative::SetFrame(2, tNodes, frame);
    struct Sum {
      passivedouble volume = 0.0;
    } sum;
    auto piece = [](void* context, unsigned short, passivedouble volume, const passivedouble*, const passivedouble*) {
      static_cast<Sum*>(context)->volume += volume;
    };
    bool clipped = false;
    const bool overlapped =
        conservative::Overlap(2, frame, eNodes, conservative::SimplexMeasure(2, eNodes), piece, &sum, &clipped);
    INFO("T measure " << frame.volume << ", overlap " << sum.volume);
    CHECK(frame.volume > 0.0);
    CHECK(clipped);
    CHECK(overlapped);
    /*--- The overlap is T up to the round-off of clipping a unit-sized E by planes of a 1e-16-sized T. ---*/
    CHECK(sum.volume > 0.5 * frame.volume);
    CHECK(sum.volume < 1.5 * frame.volume);
  }
  SECTION("target element over two disconnected donor components (no advancing front)") {
    /*--- Donor: two triangles with a gap between them; target: one triangle covering both. The exhaustive
     *    enumeration clips both (an advancing front from one component would miss the other). ---*/
    CSimplexMesh donor, target;
    donor.nDim = target.nDim = 2;
    donor.coord = {0.1, 0.1, 0.4, 0.1, 0.1, 0.4, 0.6, 0.1, 0.9, 0.1, 0.6, 0.3};
    donor.elem = {0, 1, 2, 3, 4, 5};
    donor.volume = {0.03, 0.03, 0.03, 0.02, 0.02, 0.02};
    donor.markers.resize(1);
    donor.markers[0].name = "wall";
    donor.markers[0].elem = {0, 1, 1, 2, 2, 0, 3, 4, 4, 5, 5, 3};
    target.coord = {0.0, 0.0, 2.0, 0.0, 0.0, 2.0};
    target.elem = {0, 1, 2};
    target.volume = {2.0 / 3, 2.0 / 3, 2.0 / 3};
    target.markers.resize(1);
    target.markers[0].name = "wall";
    target.markers[0].elem = {0, 1, 1, 2, 2, 0};
    CConservativeProjection::Options options;
    options.sliverRule = CConservativeProjection::SliverRule::NONE;
    options.absoluteLimit = 10.0;
    CConservativeProjection projection(donor, target, options);
    std::vector<su2double> values(6, 1.0), out;
    projection.Project(1, values, out);
    const auto& s = projection.GetSummary();
    CHECK(s.nPairs == 2);
    CHECK(s.overlapVolume == Approx(0.045 + 0.03).epsilon(1e-12));
    CHECK(s.nSliverElems == 0);
  }
}

TEST_CASE("Distributed conservative kernels: guarded CG and bounded redistribution",
          "[AdaptationMPI][DistributedTransfer]") {
  SKIP_IN_SINGLE_PRECISION();
  /*--- Mass matrix of the owned rows of a partitioned box. ---*/
  for (const unsigned short nDim : {2, 3}) {
    auto config = MakeConfig(nDim, "SOLVER= EULER\n");
    MeshSolution mesh(config.get(), BoxMesh(nDim, nDim == 2 ? 6 : 3, true), 0);
    const auto& geometry = mesh.Fine();
    const unsigned short nNode = nDim + 1;
    std::vector<unsigned long> nodes;
    std::vector<passivedouble> volumes;
    for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
      passivedouble x[4][3] = {};
      const passivedouble* p[4] = {};
      for (unsigned short k = 0; k < nNode; ++k) {
        const auto iPoint = geometry.elem[iElem]->GetNode(k);
        nodes.push_back(iPoint);
        for (unsigned short d = 0; d < nDim; ++d) x[k][d] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, d));
        p[k] = x[k];
      }
      volumes.push_back(conservative::SimplexMeasure(nDim, p));
    }
    const auto nRow = geometry.GetnPointDomain();
    conservative::MassMatrix mass;
    mass.Assemble(nDim, nRow, geometry.GetnPoint(), nodes, volumes);
    const conservative::MassSolver solver(mass, &geometry, 1e-13, 2000);
    const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();

    SECTION("nDim " + std::to_string(nDim) + ": zero, huge and nonfinite right-hand sides, non-convergence") {
      std::vector<passivedouble> b(nRow, 0.0), x;
      auto result = solver.Solve(b, x);
      CHECK_FALSE(result.failed);
      CHECK(result.iterations == 0);
      bool zero = true;
      for (const auto v : x) zero &= v == 0.0;
      CHECK(FailedRanks(zero) == 0);

      /*--- b = 1e200 x rowVolume (M 1): x = 1e200 everywhere; the unscaled inner products would overflow. ---*/
      for (auto i = 0ul; i < nRow; ++i) b[i] = 1e200 * mass.rowVolume[i];
      result = solver.Solve(b, x);
      CHECK_FALSE(result.failed);
      passivedouble worst = 0.0;
      for (const auto v : x) worst = std::max(worst, std::fabs(v / 1e200 - 1.0));
      CHECK(WorldMax(worst) < 1e-10);

      /*--- A NaN on the last rank (zeros elsewhere): flagged on every rank, never the zero shortcut. ---*/
      std::fill(b.begin(), b.end(), 0.0);
      if (rank == size - 1 && nRow > 0) b[0] = std::numeric_limits<passivedouble>::quiet_NaN();
      result = solver.Solve(b, x);
      CHECK(result.nonfiniteRHS);
      CHECK(result.failed);

      /*--- Non-convergence: one iteration only, the true residual is far above 1e-10. ---*/
      const conservative::MassSolver short1(mass, &geometry, 1e-13, 1);
      Random random(3 + rank);
      for (auto i = 0ul; i < nRow; ++i) b[i] = random.Uniform() - 0.5;
      result = short1.Solve(b, x);
      CHECK(result.failed);
      CHECK(result.iterations == 1);

      /*--- Converged solve: true residual within the tolerance. ---*/
      result = solver.Solve(b, x);
      CHECK_FALSE(result.failed);
      CHECK(result.trueResidual <= 1e-13);
    }

    SECTION("nDim " + std::to_string(nDim) + ": bounded redistribution, precedence of the totals") {
      std::vector<passivedouble> cv(nRow);
      for (auto i = 0ul; i < nRow; ++i) cv[i] = SU2_TYPE::GetValue(geometry.nodes->GetVolume(i));
      passivedouble cvLocal = 0.0;
      for (const auto v : cv) cvLocal += v;
      const passivedouble volume = CPassiveComm::Allreduce(cvLocal, CPassiveComm::Op::SUM);
      auto total = [&](const std::vector<su2double>& v) {
        passivedouble local = 0.0;
        for (auto i = 0ul; i < nRow; ++i) local += SU2_TYPE::GetValue(v[i]) * cv[i];
        return CPassiveComm::Allreduce(local, CPassiveComm::Op::SUM);
      };
      /*--- Room enough: the total is restored within the bounds. ---*/
      std::vector<su2double> v(nRow, 1.0), lo(nRow, 0.0), hi(nRow, 2.0);
      auto result = conservative::BoundedRedistribute(v, cv, 1.5 * volume, lo, hi, nullptr, 1e-12, 2.0, true);
      CHECK_FALSE(result.error);
      CHECK_FALSE(result.relaxed);
      CHECK(result.totalExact);
      CHECK(std::fabs(total(v) - 1.5 * volume) < 1e-13 * volume);
      /*--- Exhausted capacity: bounds relaxed (reported), the total exact. ---*/
      std::fill(v.begin(), v.end(), 1.0);
      std::fill(hi.begin(), hi.end(), 1.0);
      result = conservative::BoundedRedistribute(v, cv, 1.5 * volume, lo, hi, nullptr, 1e-12, 1.0, true);
      CHECK_FALSE(result.error);
      CHECK(result.relaxed);
      CHECK(result.nViolations > 0);
      CHECK(result.maxViolation == Approx(0.5).epsilon(1e-10));
      CHECK(std::fabs(total(v) - 1.5 * volume) < 1e-13 * volume);
      /*--- Everything frozen: a satisfied total is success, a nonzero correction an error (not an abort here). ---*/
      std::vector<bool> frozen(nRow, true);
      std::fill(v.begin(), v.end(), 1.0);
      result = conservative::BoundedRedistribute(v, cv, volume, lo, hi, &frozen, 1e-12, 1.0, true);
      CHECK_FALSE(result.error);
      result = conservative::BoundedRedistribute(v, cv, 2.0 * volume, lo, hi, &frozen, 1e-12, 1.0, true);
      CHECK(result.error);
    }
  }
}

TEST_CASE("Transfer admissibility: two-stage recovery predicate", "[AdaptationMPI][DistributedTransfer]") {
  /*--- The review's infeasible SST pair (densities -1 and 3, rho omega -1e-3 and 3e-4): the first state fails stage 1;
   *    their volume mean (rho 1, rho omega -3.5e-4) passes stage 1 (omega only needs to be finite) but not the full
   *    predicate; after the stage-2 bounds (omega clipped) it passes the full predicate. NaN fails every stage. ---*/
  auto config = MakeConfig(2, kRans + "KIND_TURB_MODEL= SST\n");
  MeshSolution mesh(config.get(), BoxMesh(2, 2, false), 0);
  auto* fluid = mesh.solver[MESH_0][FLOW_SOL]->GetFluidModel();
  const auto* turb = dynamic_cast<const CTurbSolver*>(mesh.solver[MESH_0][TURB_SOL]);
  const CTransferAdmissibility admissibility(*fluid, 2, turb, true);
  const su2double first[6] = {-1.0, 0.0, 0.0, 2.5e5 * -1.0, -1.0 * 1.0, -1e-3};
  const su2double second[6] = {3.0, 0.0, 0.0, 2.5e5 * 3.0, 3.0 * 1.0, 3e-4};
  su2double mean[6];
  for (int i = 0; i < 6; ++i) mean[i] = 0.5 * (first[i] + second[i]);
  CHECK_FALSE(admissibility.AdmissibleStage1(first));
  CHECK(admissibility.AdmissibleStage1(mean));
  CHECK_FALSE(admissibility.AdmissibleConservative(mean));
  su2double bounded[6];
  std::copy(mean, mean + 6, bounded);
  for (unsigned short iVar = 0; iVar < 2; ++iVar)
    bounded[4 + iVar] = mean[0] * admissibility.Bounded(iVar, mean[4 + iVar] / mean[0]);
  CHECK(admissibility.AdmissibleConservative(bounded));
  su2double withNaN[6];
  std::copy(mean, mean + 6, withNaN);
  withNaN[5] = std::numeric_limits<passivedouble>::quiet_NaN();
  CHECK_FALSE(admissibility.AdmissibleStage1(withNaN));
  CHECK_FALSE(admissibility.AdmissibleConservative(withNaN));
  /*--- Stage 1 subtracts max(rho k, k_lo rho): a k below its lower bound counts as k_lo. ---*/
  su2double lowK[6] = {1.0, 0.0, 0.0, 2.5e5, -1.0, 1e3};
  CHECK(admissibility.AdmissibleStage1(lowK));
}

TEST_CASE("Distributed conservative transfer: partition equivalence, walls, history, recovery",
          "[AdaptationMPI][DistributedTransfer]") {
  SKIP_IN_SINGLE_PRECISION();
  /*--- The complete transfer (wall fix, bounds, recovery, final checks, FinishTransfer) against P = 1 and against the
   *    gathered (MPI-1) reference: values within 1e-10 of the field scale for the fixtures without decisions at a
   *    margin, totals of the flow variables exact at every P, invariants (admissible states, zero wall momentum). ---*/
  struct Case {
    std::string name;
    unsigned short nDim;
    std::string options;
    bool affine;
    Turb turb;
    unsigned long nDonor, nTarget;
    bool marginFree;
  };
  const std::vector<Case> cases = {
      {"2D Euler affine", 2, "SOLVER= EULER\n", true, Turb::NONE, 4, 6, true},
      {"3D Euler smooth", 3, "SOLVER= EULER\n", false, Turb::NONE, 3, 4, false},
      {"2D SA walls, 2nd order", 2, kRans + "KIND_TURB_MODEL= SA\n" + kTime2, false, Turb::SA, 4, 6, false},
      {"2D SST walls", 2, kRans + "KIND_TURB_MODEL= SST\n", false, Turb::SST, 5, 4, false},
  };
  for (const auto& test : cases) {
    SECTION(test.name) {
      auto run = [&](bool gathered, PointValues& values, CConservativeTransfer::Summary& summary, passivedouble* wall) {
        auto config = MakeConfig(test.nDim, test.options);
        MeshSolution donor(config.get(), BoxMesh(test.nDim, test.nDonor, true), 2);
        SetFields(donor, test.nDim, test.affine, test.turb);
        MeshSolution target(config.get(), BoxMesh(test.nDim, test.nTarget, true), 2);
        CConservativeTransfer transfer(CConservativeProjection::Options(), gathered);
        {
          Mute mute;
          transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
        }
        values = OwnedValues(target);
        summary = transfer.GetSummary();
        if (wall == nullptr) return;
        /*--- Largest momentum on the owned no-slip wall points. ---*/
        passivedouble largest = 0.0;
        for (unsigned short iMarker = 0; iMarker < target.Fine().GetnMarker(); ++iMarker) {
          if (config->GetMarker_All_KindBC(iMarker) != HEAT_FLUX) continue;
          for (auto iElem = 0ul; iElem < target.Fine().GetnElem_Bound(iMarker); ++iElem)
            for (unsigned short k = 0; k < target.Fine().bound[iMarker][iElem]->GetnNodes(); ++k) {
              const auto iPoint = target.Fine().bound[iMarker][iElem]->GetNode(k);
              if (iPoint >= target.Fine().GetnPointDomain()) continue;
              for (unsigned short d = 0; d < test.nDim; ++d)
                largest = std::max(largest, std::fabs(SU2_TYPE::GetValue(
                                                target.solver[MESH_0][FLOW_SOL]->GetNodes()->GetSolution(iPoint, 1 + d))));
            }
        }
        *wall = WorldMax(largest);
        if (test.affine) CheckCoarseLevels(target, FLOW_SOL);
      };
      PointValues serial, parallel, gathered;
      CConservativeTransfer::Summary serialSummary, summary, gatheredSummary;
      passivedouble wall = 1.0;
      Serial([&]() { run(false, serial, serialSummary, nullptr); });
      run(false, parallel, summary, &wall);
      run(true, gathered, gatheredSummary, nullptr);
      passivedouble worst = 0.0, worstGathered = 0.0;
      unsigned long nNotBitwise = 0, nNotBitwiseGathered = 0;
      bool complete = false, completeGathered = false;
      CompareValues(parallel, serial, worst, nNotBitwise, complete);
      CompareValues(parallel, gathered, worstGathered, nNotBitwiseGathered, completeGathered);
      INFO("difference to P = 1: " << worst << ", to the gathered transfer: " << worstGathered
                                   << "; limited " << summary.projection.nLimited[0] << ", recovered "
                                   << summary.nFlowFixed);
      CHECK(complete);
      CHECK(completeGathered);
      if (test.marginFree) {
        CHECK(worst <= 1e-10);
        CHECK(worstGathered <= 1e-10);
      } else {
        /*--- Decisions at a margin (limiter clipping) may differ: reported, a loose sanity bound only. ---*/
        CHECK(worst <= 1e-6);
        CHECK(worstGathered <= 1e-6);
      }
      CHECK(wall == 0.0);
      for (unsigned short f = 0; f < test.nDim + 2; ++f) CHECK(std::fabs(summary.relativeDefect[f]) < 1e-12);
      CHECK(summary.nPoint == serialSummary.nPoint);
      CHECK(summary.nWallPoints == serialSummary.nWallPoints);
    }
  }
}

TEST_CASE("Distributed conservative transfer: recovery on the gathered level", "[AdaptationMPI][DistributedTransfer]") {
  SKIP_IN_SINGLE_PRECISION();
  /*--- High-speed states (momentum jumps, small internal energy): the projection gives inadmissible states, the
   *    recovery (stage 1 on the master rank until M3) repairs them; every state admissible, totals exact, at every P. ---*/
  const unsigned short nDim = 2;
  auto run = [&](CConservativeTransfer::Summary& summary, unsigned long& nBad) {
    auto config = MakeConfig(nDim, "SOLVER= EULER\n" + kTime2);
    auto field = [](passivedouble shift) {
      return Field([shift](const su2double* x, su2double* U) {
        const su2double rho = 1.0 + 0.3 * x[1];
        const su2double u = tanh((x[0] - 1.0 - shift) / 0.005) - 0.5 * tanh((x[0] - 0.4 + shift) / 0.005);
        const su2double v = 0.5 * tanh((x[1] - 0.5 - shift) / 0.005);
        U[0] = rho;
        U[1] = rho * u;
        U[2] = rho * v;
        U[3] = rho * (0.5 * (u * u + v * v) + 1e-5);
      });
    };
    MeshSolution donor(config.get(), BoxMesh(nDim, 16, true), 0);
    auto* donorNodes = donor.solver[MESH_0][FLOW_SOL]->GetNodes();
    for (auto iPoint = 0ul; iPoint < donor.Fine().GetnPoint(); ++iPoint) {
      su2double Un[MAXVAR], Un1[MAXVAR];
      field(0.0)(donor.Fine().nodes->GetCoord(iPoint), Un);
      field(0.03)(donor.Fine().nodes->GetCoord(iPoint), Un1);
      for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
        donorNodes->GetSolution()(iPoint, iVar) = Un[iVar];
        donorNodes->GetSolution_time_n()(iPoint, iVar) = Un[iVar];
        donorNodes->GetSolution_time_n1()(iPoint, iVar) = Un1[iVar];
      }
    }
    MeshSolution target(config.get(), BoxMesh(nDim, 9, true), 0);
    CConservativeTransfer transfer;
    {
      Mute mute;
      transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
    }
    summary = transfer.GetSummary();
    auto* fluidModel = target.solver[MESH_0][FLOW_SOL]->GetFluidModel();
    auto* nodes = target.solver[MESH_0][FLOW_SOL]->GetNodes();
    unsigned long bad = 0;
    for (auto iPoint = 0ul; iPoint < target.Fine().GetnPoint(); ++iPoint) {
      su2double U[MAXVAR], V[MAXVAR];
      for (unsigned short iVar = 0; iVar < nDim + 2; ++iVar) {
        U[iVar] = nodes->GetSolution(iPoint, iVar);
        V[iVar] = nodes->GetSolution_time_n1()(iPoint, iVar);
      }
      bad += !CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, U);
      bad += !CBarycentricTransfer::AdmissibleState(*fluidModel, nDim, V);
    }
    nBad = CPassiveComm::AllreduceSum(bad);
  };
  CConservativeTransfer::Summary serial, parallel;
  unsigned long nBadSerial = 0, nBad = 0;
  Serial([&]() { run(serial, nBadSerial); });
  run(parallel, nBad);
  INFO("fixed " << parallel.nFlowFixed << " / " << parallel.nHistoryFixed << " (P = 1: " << serial.nFlowFixed << " / "
                << serial.nHistoryFixed << ")");
  CHECK(parallel.nFlowFixed + parallel.nHistoryFixed > 0);
  CHECK(parallel.recoveryGathered);
  CHECK(nBad == 0);
  CHECK(nBadSerial == 0);
  for (unsigned short f = 0; f < 2 * (nDim + 2); ++f) CHECK(std::fabs(parallel.relativeDefect[f]) < 1e-12);
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- Single precision contract (REVIEW5_FIX_PLAN.md F1.4), run in every build ---*/
/*------------------------------------------------------------------------------------------------------------------*/

TEST_CASE("Single precision transfer contract", "[AdaptationMPI][DistributedTransfer][SinglePrecision]") {
  /*--- Finite states, conservation within the acceptance gate of the build, stencil decisions equal for every P. ---*/
  SECTION("barycentric: decisions equal to one rank, finite values") {
    for (const auto& test : {BaryCase{"2D Euler", 2, "SOLVER= EULER\n", false, Turb::NONE, 4, 6, 1.0},
                             BaryCase{"2D SA walls", 2, kRans + "KIND_TURB_MODEL= SA\n", false, Turb::SA, 4, 6, 1.0},
                             BaryCase{"3D Euler", 3, "SOLVER= EULER\n", false, Turb::NONE, 3, 4, 1.0}}) {
      BaryResult serial;
      Serial([&]() { serial = RunBarycentric(test, false, false); });
      const auto parallel = RunBarycentric(test, false, false);
      CHECK(SameDecisions(parallel.records, serial.records, true));
      bool finite = true;
      for (const auto& entry : parallel.values)
        for (const auto& v : entry.second) finite &= std::isfinite(SU2_TYPE::GetValue(v));
      CHECK(FailedRanks(finite) == 0);
    }
  }
  SECTION("conservative: totals within the acceptance gate, finite values") {
    for (const unsigned short nDim : {2, 3}) {
      CConservativeProjection::Options options;
      const auto donorMesh = BoxMesh(nDim, nDim == 2 ? 6 : 3, true),
                 targetMesh = BoxMesh(nDim, nDim == 2 ? 8 : 4, true);
      const auto result = RunProjection(nDim, donorMesh, targetMesh, "", options, false, false);
      for (auto f = 0ul; f < result.newTotal.size(); ++f)
        CHECK(std::fabs(result.newTotal[f] - result.summary.targetTotal[f]) <=
              TransferTol(1e-12, 1e-5) * result.summary.scale[f]);
      bool finite = true;
      for (const auto& entry : result.values)
        for (const auto v : entry.second) finite &= std::isfinite(v);
      CHECK(FailedRanks(finite) == 0);
    }
  }
  SECTION("conservative: a partial gap between the noise floor and the gap threshold") {
    /*--- The new domain ends at x = 2 - d with d / h = 0.01: the donor elements of the last column lose 2e-2 (edge on
     *    x = 2) or about 1e-4 (vertex on x = 2) of their measure, far above the round-off floor of either precision:
     *    the same 2 n_y slivers in single and double precision, the strip's area as their missing measure. ---*/
    const unsigned long n = 4;
    const passivedouble h = 1.0 / n, d = 0.01 * h;
    auto targetMesh = BoxMesh(2, 6, true);
    for (auto iPoint = 0ul; iPoint < targetMesh.GetnPoint(); ++iPoint) targetMesh.coord[2 * iPoint] *= (2.0 - d) / 2.0;
    CConservativeProjection::Options options;
    options.sliverRule = CConservativeProjection::SliverRule::GLOBAL;
    const auto result = RunProjection(2, BoxMesh(2, n, true), targetMesh, "", options, false, false);
    CHECK(result.summary.nSliverElems == 2 * n);
    CHECK(std::fabs(result.summary.sliverVolume - d) <= TransferTol(1e-12, 1e-3) * d);
    for (auto f = 0ul; f < result.newTotal.size(); ++f)
      CHECK(std::fabs(result.newTotal[f] - result.summary.targetTotal[f]) <=
            TransferTol(1e-12, 1e-5) * result.summary.scale[f]);
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

TEST_CASE("Collective error: conservative nonfinite donor value", "[.CollectiveError][nonfinite]") {
  if (!CollectiveErrorTests()) return;
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(2, 4, true), 0);
  SetFields(donor, 2, true, Turb::NONE);
  if (SU2_MPI::GetRank() == SU2_MPI::GetSize() - 1 && donor.Fine().GetnPointDomain() > 0)
    donor.solver[MESH_0][FLOW_SOL]->GetNodes()->GetSolution()(0, 0) = std::numeric_limits<passivedouble>::quiet_NaN();
  MeshSolution target(config.get(), BoxMesh(2, 6, true), 0);
  CConservativeTransfer transfer;
  transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
}

TEST_CASE("Collective error: conservative memory ceiling", "[.CollectiveError][ceiling]") {
  if (!CollectiveErrorTests()) return;
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(2, 4, true), 0);
  SetFields(donor, 2, true, Turb::NONE);
  MeshSolution target(config.get(), BoxMesh(2, 6, true), 0);
  SetTransferMemoryCeiling(20000);
  CConservativeTransfer transfer;
  transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
}

TEST_CASE("Collective error: BOUNDARY sliver rule with more than one rank", "[.CollectiveError][boundary]") {
  if (!CollectiveErrorTests()) return;
  auto config = MakeConfig(2, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(2, 4, true), 0);
  SetFields(donor, 2, true, Turb::NONE);
  MeshSolution target(config.get(), BoxMesh(2, 6, true), 0);
  CConservativeProjection::Options options;
  options.sliverRule = CConservativeProjection::SliverRule::BOUNDARY;
  CConservativeTransfer transfer(options);
  transfer.Transfer(config.get(), donor.Donor(), target.geometry, target.solver);
}
