/*!
 * \file CDiscAdjSinglezoneDriver_tests.cpp
 * \brief Unit tests of the mesh replacement inside the discrete adjoint driver (stage G2, reverse-mode AD build).
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

#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <fstream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "../../../Common/include/adaptation/CMMGInterface.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../SU2_CFD/include/adaptation/CAdjointTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/drivers/CDiscAdjSinglezoneDriver.hpp"
#include "../../../SU2_CFD/include/output/COutput.hpp"
#include "../../Common/adaptation/SimplexMeshTestCase.hpp"

#ifdef CODI_REVERSE_TYPE

namespace {

/*--- Access to the protected parts of the discrete adjoint driver. ---*/
class TestDADriver : public CDiscAdjSinglezoneDriver {
 public:
  using CDiscAdjSinglezoneDriver::CDiscAdjSinglezoneDriver;
  using CDiscAdjSinglezoneDriver::RunPrimalPhase;
  using CDiscAdjSinglezoneDriver::PreprocessGoalAdjoint;
  CConfig* Config() { return config_container[ZONE_0]; }
  CGeometry* Geometry() { return geometry_container[ZONE_0][INST_0][MESH_0]; }
  CSolver* Flow() { return solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]; }
  CSolver* Adjoint() { return solver_container[ZONE_0][INST_0][MESH_0][ADJFLOW_SOL]; }
  COutput* AdjointOutput() { return output_container[ZONE_0]; }
  void SetAdjointIterations(unsigned long n) { nAdjoint_Iter = n; }
  passivedouble Objective() const { return SU2_TYPE::GetValue(ObjFunc); }
  void Capture() { CaptureResidualAdjoint(); }
  void GoalMetric() { ComputeGoalMetric(); }
  void LoadAdjointRestart() {
    Adjoint()->LoadRestart(geometry_container[ZONE_0][INST_0], solver_container[ZONE_0][INST_0], Config(), 0, true);
  }
  CSolver** Solvers() { return solver_container[ZONE_0][INST_0][MESH_0]; }
};

struct Mute {
  std::streambuf* buffer = std::cout.rdbuf();
  Mute() { std::cout.rdbuf(nullptr); }
  ~Mute() { std::cout.rdbuf(buffer); }
};

const std::string kName = "da_swap";

std::string CommonOptions(bool upwind = false) {
  return "SOLVER= EULER\nMACH_NUMBER= 0.5\nAOA= 5.0\n"
         "FREESTREAM_PRESSURE= 101325.0\nFREESTREAM_TEMPERATURE= 288.15\nREF_DIMENSIONALIZATION= DIMENSIONAL\n"
         "MESH_FORMAT= SU2\nMESH_FILENAME= " + kName + ".su2\n"
         "MARKER_FAR= (left, right, upper)\nMARKER_EULER= (lower_a, lower_b)\nMARKER_MONITORING= (lower_a, lower_b)\n"
         "REF_AREA= 1.0\nREF_LENGTH= 1.0\n" +
         (upwind ? "CONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= YES\nSLOPE_LIMITER_FLOW= VENKATAKRISHNAN\n"
                   "LIMITER_ITER= 1\nFROZEN_LIMITER_DISC= NO\n"
                 : "CONV_NUM_METHOD_FLOW= JST\nJST_SENSOR_COEFF= (0.5, 0.02)\n") +
         "TIME_DISCRE_FLOW= EULER_IMPLICIT\n"
         "NUM_METHOD_GRAD= GREEN_GAUSS\nCFL_NUMBER= 20\nMGLEVEL= 0\n"
         "LINEAR_SOLVER= FGMRES\nLINEAR_SOLVER_PREC= ILU\nLINEAR_SOLVER_ITER= 20\nLINEAR_SOLVER_ERROR= 1e-10\n"
         "DISCADJ_LIN_SOLVER= FGMRES\nDISCADJ_LIN_PREC= ILU\n"
         "SCREEN_WRT_FREQ_INNER= 100000\nHISTORY_WRT_FREQ_INNER= 100000\nOUTPUT_WRT_FREQ= 100000\n"
         "RESTART_FILENAME= " + kName + "_flow\nSOLUTION_FILENAME= " + kName + "_flow\n"
         "RESTART_ADJ_FILENAME= " + kName + "_adj\nSOLUTION_ADJ_FILENAME= " + kName + "_adj\n"
         "CONV_FILENAME= " + kName + "_history\nVOLUME_FILENAME= " + kName + "_vol\n"
         "VOLUME_ADJ_FILENAME= " + kName + "_voladj\nTABULAR_FORMAT= CSV\n";
}

/*--- The mesh (triangulated rectangle with an Euler wall) and a primal restart made by the DIRECT driver. ---*/
void PreparePrimal() {
  static bool done = false;
  if (done) return;
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(2, 32, simplex_test::Marker2D), kName + ".su2");
    std::ofstream cfg(kName + "_primal.cfg");
    cfg << CommonOptions() << "MATH_PROBLEM= DIRECT\nITER= 400\nCONV_RESIDUAL_MINVAL= -13\nOUTPUT_FILES= (RESTART)\n";
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  {
    Mute mute;
    CSinglezoneDriver driver(const_cast<char*>((kName + "_primal.cfg").c_str()), 1, SU2_MPI::GetComm());
    driver.StartSolver();
    driver.Finalize();
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  done = true;
}

std::string WriteAdjointConfig(const std::string& suffix, bool compact, bool upwind = false) {
  const std::string name = kName + "_" + suffix + ".cfg";
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    std::ofstream cfg(name);
    cfg << CommonOptions(upwind) << "MATH_PROBLEM= DISCRETE_ADJOINT\nOBJECTIVE_FUNCTION= DRAG\nITER= 1\n"
        << "QUASI_NEWTON_NUM_SAMPLES= 0\nRELAXATION_FACTOR_ADJOINT= 1.0\nCONV_RESIDUAL_MINVAL= -30\n"
        << "COMPUTE_METRIC= YES\nADAP_SENSOR= (GOAL)\nADAP_COMPLEXITY= 1500\nADAP_HMIN= 1e-4\nADAP_HMAX= 1.0\n"
        << "NUM_METHOD_HESS= GREEN_GAUSS\nOUTPUT_FILES= (RESTART)\nWRT_RESTART_COMPACT= " << (compact ? "YES" : "NO")
        << "\n";
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  return name;
}

using PointValues = std::map<uint64_t, std::vector<passivedouble>>;

PointValues Values(CGeometry* geometry, const su2activematrix& array) {
  PointValues values;
  for (auto iPoint = 0ul; iPoint < geometry->GetnPointDomain(); ++iPoint) {
    auto& row = values[geometry->nodes->GetGlobalIndex(iPoint)];
    for (unsigned long iVar = 0; iVar < array.cols(); ++iVar) row.push_back(SU2_TYPE::GetValue(array(iPoint, iVar)));
  }
  return values;
}

PointValues DirectValues(TestDADriver& driver) {
  PointValues values;
  auto* nodes = driver.Adjoint()->GetNodes();
  for (auto iPoint = 0ul; iPoint < driver.Geometry()->GetnPointDomain(); ++iPoint) {
    auto& row = values[driver.Geometry()->nodes->GetGlobalIndex(iPoint)];
    for (unsigned short iVar = 0; iVar < driver.Flow()->GetnVar(); ++iVar)
      row.push_back(SU2_TYPE::GetValue(nodes->GetSolution_Direct(iPoint)[iVar]));
  }
  return values;
}

/*--- Values of all ranks on every rank (global index, row), for comparisons across runs with other partitions. ---*/
PointValues Gathered(const PointValues& local) {
  std::vector<passivedouble> flat;
  for (const auto& entry : local) {
    flat.push_back(static_cast<passivedouble>(entry.first));
    flat.push_back(static_cast<passivedouble>(entry.second.size()));
    flat.insert(flat.end(), entry.second.begin(), entry.second.end());
  }
  const auto bytes = CPassiveComm::AllgathervRounds(reinterpret_cast<const char*>(flat.data()),
                                                    flat.size() * sizeof(passivedouble), nullptr);
  std::vector<passivedouble> all(bytes.size() / sizeof(passivedouble));
  if (!all.empty()) std::memcpy(all.data(), bytes.data(), bytes.size());
  PointValues result;
  for (auto i = 0ul; i < all.size();) {
    const auto gid = static_cast<uint64_t>(all[i]);
    const auto n = static_cast<unsigned long>(all[i + 1]);
    result[gid].assign(all.begin() + i + 2, all.begin() + i + 2 + n);
    i += 2 + n;
  }
  return result;
}

/*--- Largest |a - b| / (max |b| of the column), over the points of both (gathered) maps; infinity if they differ. ---*/
passivedouble MaxDifference(const PointValues& a, const PointValues& b) {
  if (a.size() != b.size() || a.empty()) return std::numeric_limits<passivedouble>::infinity();
  std::vector<passivedouble> scale(b.begin()->second.size(), 0.0);
  for (const auto& entry : b)
    for (auto f = 0ul; f < entry.second.size(); ++f) scale[f] = std::max(scale[f], std::fabs(entry.second[f]));
  passivedouble diff = 0.0;
  for (const auto& entry : a) {
    const auto it = b.find(entry.first);
    if (it == b.end() || it->second.size() != entry.second.size()) return std::numeric_limits<passivedouble>::infinity();
    for (auto f = 0ul; f < entry.second.size(); ++f) {
      const passivedouble d = std::fabs(entry.second[f] - it->second[f]) / (scale[f] > 0.0 ? scale[f] : 1.0);
      diff = std::max(diff, std::isfinite(d) ? d : std::numeric_limits<passivedouble>::infinity());
    }
  }
  return diff;
}

/*--- Halos are recorded primal outputs too; their seeds must not duplicate the owner solution. ---*/
passivedouble HaloSeedMax(TestDADriver& driver) {
  passivedouble maximum = 0.0;
  const auto& psi = driver.Adjoint()->GetNodes()->GetSolution();
  for (auto i = driver.Geometry()->GetnPointDomain(); i < driver.Geometry()->GetnPoint(); ++i)
    for (auto v = 0ul; v < psi.cols(); ++v)
      maximum = std::max(maximum, std::fabs(SU2_TYPE::GetValue(psi(i, v))));
  return CPassiveComm::Allreduce(maximum, CPassiveComm::Op::MAX);
}

/*--- RMS of every adjoint variable after one adjoint iteration (Run with one iteration). ---*/
std::vector<passivedouble> AdjointIteration(TestDADriver& driver) {
  driver.Run();
  std::vector<passivedouble> rms;
  for (unsigned short iVar = 0; iVar < driver.Adjoint()->GetnVar(); ++iVar)
    rms.push_back(SU2_TYPE::GetValue(driver.Adjoint()->GetRes_RMS(iVar)));
  return rms;
}

/*--- The same mesh, extracted (gathered on the master rank) with a unit metric. ---*/
CSimplexMesh SameMesh(TestDADriver& driver) {
  const auto nPoint = driver.Geometry()->GetnPoint();
  su2activematrix metric(nPoint, 3);
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint) {
    metric(iPoint, 0) = 1.0;
    metric(iPoint, 1) = 0.0;
    metric(iPoint, 2) = 1.0;
  }
  return CMMGInterface::ExtractMesh(*driver.Config(), *driver.Geometry(), metric);
}

std::unique_ptr<CDiscAdjTransfer> MakeTransfer() {
  return std::make_unique<CDiscAdjTransfer>(std::make_unique<CBarycentricTransfer>(false), true);
}

/*--- The adjoint restart written by the output is read back by CDiscAdjSolver::LoadRestart: psi bitwise. ---*/
bool RestartRoundTrip(TestDADriver& driver, const std::vector<string>& exclusions) {
  auto* output = driver.AdjointOutput();
  output->SetVolumeOutputExclusions(exclusions);
  auto& psi = driver.Adjoint()->GetNodes()->GetSolution();
  const auto saved = Values(driver.Geometry(), psi);
  {
    Mute mute;
    output->SetResultFiles(driver.Geometry(), driver.Config(), driver.Solvers(), 0, true);
  }
  for (auto iPoint = 0ul; iPoint < psi.rows(); ++iPoint)
    for (unsigned long iVar = 0; iVar < psi.cols(); ++iVar) psi(iPoint, iVar) = std::numeric_limits<passivedouble>::quiet_NaN();
  {
    Mute mute;
    driver.LoadAdjointRestart();
  }
  const auto loaded = Values(driver.Geometry(), psi);
  unsigned long nDiff = (loaded.size() == saved.size()) ? 0 : 1;
  for (const auto& entry : saved) {
    const auto it = loaded.find(entry.first);
    if (it == loaded.end() || it->second != entry.second) nDiff++;
  }
  return CPassiveComm::AllreduceSum(nDiff) == 0;
}

}  // namespace

TEST_CASE("Goal recording after LIMITER_ITER matches a fresh discrete adjoint recording", "[GoalSwap]") {
  PreparePrimal();
  PointValues primalState;
  passivedouble objectiveGoal = 0.0, objectiveFresh = 0.0;
  std::vector<passivedouble> rmsGoal, rmsFresh;
  unsigned long primalIters = 0, primalInnerIter = 0, recordingInnerIter = 1, recordingOuterIter = 1;
  {
    const auto cfg = WriteAdjointConfig("limiter_goal", true, true);
    Mute mute;
    TestDADriver driver(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
    /*--- Run the actual goal-loop primal phase past the limiter freeze. Save U_in, not the recording's U_out. ---*/
    const auto phase = driver.RunPrimalPhase(6);
    primalIters = phase.nIter;
    primalInnerIter = driver.Config()->GetInnerIter();
    primalState = Gathered(Values(driver.Geometry(), driver.Flow()->GetNodes()->GetSolution()));
    driver.SetAdjointIterations(1);
    driver.Config()->SetnInner_Iter(1);
    /*--- Exercise the recording setup used by RunGoalAdaptationLoop. ---*/
    driver.PreprocessGoalAdjoint();
    recordingInnerIter = driver.Config()->GetInnerIter();
    recordingOuterIter = driver.Config()->GetOuterIter();
    objectiveGoal = driver.Objective();
    rmsGoal = AdjointIteration(driver);
    AD::Reset();
    driver.Finalize();
  }
  {
    const auto cfg = WriteAdjointConfig("limiter_fresh", true, true);
    Mute mute;
    TestDADriver driver(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
    /*--- A fresh DA driver records the same primal state, including halo points, at its initial counters. ---*/
    for (auto iPoint = 0ul; iPoint < driver.Geometry()->GetnPoint(); ++iPoint) {
      const auto& row = primalState.at(driver.Geometry()->nodes->GetGlobalIndex(iPoint));
      for (unsigned short iVar = 0; iVar < driver.Flow()->GetnVar(); ++iVar)
        driver.Flow()->GetNodes()->SetSolution(iPoint, iVar, row[iVar]);
    }
    driver.Flow()->GetNodes()->Set_OldSolution();
    driver.SetAdjointIterations(1);
    driver.Preprocess(0);
    objectiveFresh = driver.Objective();
    rmsFresh = AdjointIteration(driver);
    AD::Reset();
    driver.Finalize();
  }
  CHECK(primalIters == 6);
  CHECK(primalInnerIter > 1);
  CHECK(recordingInnerIter == 0);
  CHECK(recordingOuterIter == 0);
  CHECK(std::fabs(objectiveGoal - objectiveFresh) <= 1e-12);
  REQUIRE(rmsGoal.size() == rmsFresh.size());
  for (auto iVar = 0ul; iVar < rmsGoal.size(); ++iVar)
    CHECK(std::fabs(rmsGoal[iVar] - rmsFresh[iVar]) <= 1e-12);
}

TEST_CASE("Discrete adjoint mesh swap: same mesh, next iterations as uninterrupted", "[GoalSwap]") {
  PreparePrimal();
  const unsigned long k = 4;

  /*--- Run A: 2k adjoint iterations on one recording. ---*/
  std::vector<std::vector<passivedouble>> rmsA;
  passivedouble objectiveA = 0.0, linearResidualA = 0.0, linearResidualB = 0.0;
  PointValues psiA;
  {
    const auto cfg = WriteAdjointConfig("a", true);
    Mute mute;
    TestDADriver driver(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
    driver.SetAdjointIterations(1);
    driver.Preprocess(0);
    objectiveA = driver.Objective();
    linearResidualA = SU2_TYPE::GetValue(driver.Flow()->GetResLinSolver());
    for (unsigned long i = 0; i < 2 * k; ++i) rmsA.push_back(AdjointIteration(driver));
    psiA = Gathered(Values(driver.Geometry(), driver.Adjoint()->GetNodes()->GetSolution()));
    AD::Reset();
    driver.Finalize();
  }

  /*--- Run B: k iterations, same-mesh swap, re-recording, k iterations. ---*/
  std::vector<std::vector<passivedouble>> rmsB;
  passivedouble objectiveB = 0.0, flowDiff = 0.0, psiDiff = 0.0;
  bool restartCompact = false, restartFull = false;
  PointValues psiB;
  unsigned long changedOwners = 0;
  passivedouble haloSeedBefore = 0.0, haloSeedAfter = 0.0;
  {
    const auto cfg = WriteAdjointConfig("b", true);
    Mute mute;
    TestDADriver driver(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
    driver.SetAdjointIterations(1);
    driver.Preprocess(0);
    for (unsigned long i = 0; i < k; ++i) rmsB.push_back(AdjointIteration(driver));
    const auto direct = Gathered(DirectValues(driver));
    const auto psiBefore = Gathered(Values(driver.Geometry(), driver.Adjoint()->GetNodes()->GetSolution()));
    haloSeedBefore = HaloSeedMax(driver);

    PointValues owners;
    for (auto i = 0ul; i < driver.Geometry()->GetnPointDomain(); ++i)
      owners[driver.Geometry()->nodes->GetGlobalIndex(i)] = {static_cast<passivedouble>(SU2_MPI::GetRank())};
    owners = Gathered(owners);
    const auto mesh = SameMesh(driver);
    auto transfer = MakeTransfer();
    driver.SwapMesh(mesh, *transfer);
    haloSeedAfter = HaloSeedMax(driver);

    PointValues newOwners;
    for (auto i = 0ul; i < driver.Geometry()->GetnPointDomain(); ++i)
      newOwners[driver.Geometry()->nodes->GetGlobalIndex(i)] = {static_cast<passivedouble>(SU2_MPI::GetRank())};
    newOwners = Gathered(newOwners);
    for (const auto& point : owners)
      if (point.second != newOwners.at(point.first)) ++changedOwners;

    flowDiff = MaxDifference(Gathered(Values(driver.Geometry(), driver.Flow()->GetNodes()->GetSolution())), direct);
    psiDiff = MaxDifference(Gathered(Values(driver.Geometry(), driver.Adjoint()->GetNodes()->GetSolution())), psiBefore);

    driver.SetAdjointIterations(1);
    driver.Preprocess(0);
    objectiveB = driver.Objective();
    linearResidualB = SU2_TYPE::GetValue(driver.Flow()->GetResLinSolver());
    for (unsigned long i = 0; i < k; ++i) rmsB.push_back(AdjointIteration(driver));
    psiB = Gathered(Values(driver.Geometry(), driver.Adjoint()->GetNodes()->GetSolution()));

    /*--- Restart usability (compact restart): with and without the sensitivity fields. ---*/
    restartCompact = RestartRoundTrip(driver, {"SENSITIVITY"});
    restartFull = RestartRoundTrip(driver, {});
    AD::Reset();
    driver.Finalize();
  }

  CHECK(haloSeedBefore <= 1e-12);
  CHECK(haloSeedAfter == 0.0);
  CHECK(flowDiff <= 1e-12);
  CHECK(psiDiff <= 1e-12);
  CHECK(std::fabs(objectiveB - objectiveA) <= 1e-12 * std::fabs(objectiveA));
  REQUIRE(rmsA.size() == rmsB.size());
  passivedouble worst = 0.0;
  for (auto i = 0ul; i < rmsA.size(); ++i)
    for (auto v = 0ul; v < rmsA[i].size(); ++v)
      worst = std::max(worst, std::fabs(rmsB[i][v] - rmsA[i][v]) / std::max(std::fabs(rmsA[i][v]), 1e-300));
  CHECK(worst <= 1e-10);
  CHECK(MaxDifference(psiB, psiA) <= 1e-10);
  /*--- Not trivial: the iterations change the adjoint (the residuals move) and psi is not zero. ---*/
  CHECK(rmsA.front()[0] != rmsA.back()[0]);
  passivedouble psiMax = 0.0;
  for (const auto& entry : psiA)
    for (const auto value : entry.second) psiMax = std::max(psiMax, std::fabs(value));
  CHECK(psiMax > 0.0);
  CHECK(objectiveA != 0.0);
  CHECK(restartCompact);
  CHECK(restartFull);
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    WARN("same-mesh swap: changed owners " << changedOwners << ", recorded linear residuals "
         << linearResidualA << " / " << linearResidualB << ", halo seeds " << haloSeedBefore << " / " << haloSeedAfter
         << ", flow " << flowDiff << ", psi " << psiDiff << ", objective "
                                 << std::fabs(objectiveB - objectiveA) / std::fabs(objectiveA)
                                 << ", worst per-iteration RMS " << worst << ", final psi " << MaxDifference(psiB, psiA));
  }
}

TEST_CASE("Discrete adjoint mesh swap: memory over full lifecycles", "[GoalSwap]") {
  PreparePrimal();
  std::vector<passivedouble> rss;
  bool restart = false;
  {
    const auto cfg = WriteAdjointConfig("rss", false);
    Mute mute;
    TestDADriver driver(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
    auto transfer = MakeTransfer();
    for (int cycle = 0; cycle < 10; ++cycle) {
      driver.SetAdjointIterations(2);
      driver.Preprocess(0);
      driver.Run();
      driver.Capture();
      driver.GoalMetric();
      driver.AdjointOutput()->SetResultFiles(driver.Geometry(), driver.Config(), driver.Solvers(), 0, true);
      const auto mesh = SameMesh(driver);
      driver.SwapMesh(mesh, *transfer);
      std::ifstream status("/proc/self/status");
      std::string line;
      passivedouble value = 0.0;
      while (std::getline(status, line))
        if (line.compare(0, 6, "VmRSS:") == 0) value = std::stod(line.substr(6)) / 1024.0;
      rss.push_back(CPassiveComm::Allreduce(value, CPassiveComm::Op::MAX));
    }
    /*--- Restart usability with the non-compact restart. ---*/
    driver.SetAdjointIterations(2);
    driver.Preprocess(0);
    driver.Run();
    restart = RestartRoundTrip(driver, {"SENSITIVITY"}) && RestartRoundTrip(driver, {});
    AD::Reset();
    driver.Finalize();
  }
  REQUIRE(rss.size() == 10);
  CHECK(rss[9] <= 1.05 * rss[2]);
  CHECK(restart);
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    std::ostringstream text;
    for (const auto value : rss) text << " " << value;
    WARN("RSS after each swap [MB]:" << text.str());
  }
}

#endif
