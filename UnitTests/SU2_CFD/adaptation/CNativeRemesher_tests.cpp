/*!
 * \file CNativeRemesher_tests.cpp
 * \brief Real SU2 native backend, mesh replacement and distributed transfer across repeated cycles.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include <fstream>
#include <cstdio>

using namespace transfer_test;
using namespace SU2NativeBoundary2D;

namespace {
class NativeDriver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::AdaptedMeshName;
  using CSinglezoneDriver::CSinglezoneDriver;
  using CSinglezoneDriver::WriteAdaptedMesh;
  CConfig& Config() { return *config_container[ZONE_0]; }
  CGeometry& Geometry() { return *geometry_container[ZONE_0][INST_0][MESH_0]; }
  CSolver& Flow() { return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]; }
  std::unique_ptr<CRemesher> Backend() { return MakeRemesher(); }
  const ReferenceState& Reference() const { return *nativeReference; }
};
}  // namespace

TEST_CASE("Native SU2 backend: eight replacements preserve reference and affine solution", "[NativeRemesher]") {
  World world;
  const bool conservative = GENERATE(false, true);
  const std::string name = conservative ? "native_driver_conservative" : "native_driver_barycentric";
  if (world.rank == 0) {
    simplex_test::WriteSU2Mesh(BoxMesh(2, 6, true), name + ".su2");
    std::ofstream cfg(name + ".cfg");
    cfg << "SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\n"
           "MESH_FORMAT= SU2\nMESH_FILENAME= "
        << name
        << ".su2\n"
           "MARKER_FAR= (left, right, upper, lower_a, lower_b)\n"
           "COMPUTE_METRIC= YES\nADAP_SENSOR= MACH\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\n"
           "ADAP_HAUSD= 1e-8\nMGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\n"
           "CONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\nITER= 2\n"
           "OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nCONV_FILENAME= "
        << name << "_history\nMESH_OUT_FILENAME= " << name << "_mesh\n";
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  std::unique_ptr<NativeDriver> driver;
  {
    const auto cfg = name + ".cfg";
    driver = std::make_unique<NativeDriver>(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
  }
  const auto field = AffineFlow(2);
  auto& flow = driver->Flow();
  for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p) {
    su2double values[MAXVAR] = {};
    field(driver->Geometry().nodes->GetCoord(p), values);
    for (unsigned short k = 0; k < flow.GetnVar(); ++k) flow.GetNodes()->SetSolution(p, k, values[k]);
  }
  auto remesher = driver->Backend();
  CHECK(dynamic_cast<CNativeRemesher*>(remesher.get()) != nullptr);
  CBarycentricTransfer barycentric;
  CConservativeTransfer projection;
  CSolutionTransfer& transfer = conservative ? static_cast<CSolutionTransfer&>(projection) : barycentric;
  const std::array<double, 8> sizes{.20, .30, .16, .28, .18, .32, .14, .26};
  std::shared_ptr<const PolylineReference> original;
  std::vector<uint64_t> counts;
  for (const auto h : sizes) {
    auto& geometry = driver->Geometry();
    su2activematrix metric(geometry.GetnPointDomain(), 3);
    for (unsigned long p = 0; p < metric.rows(); ++p) {
      metric(p, 0) = metric(p, 2) = 1 / (h * h);
      metric(p, 1) = 0;
    }
    INFO("target size " << h << ", conservative=" << conservative);
    auto result = remesher->Remesh(driver->Config(), geometry, metric);
    CHECK(result.status == CRemeshResult::Status::COMPLETE);
    if (result.status != CRemeshResult::Status::COMPLETE) break;
    if (!original) original = driver->Reference().original;
    CHECK(driver->Reference().original == original);
    const auto previous = driver->Reference().accepted_edges;
    CHECK(previous.size() > 0);
    auto notificationCopy = result.onAccepted;
    driver->ReplaceMesh(result, transfer);
    CHECK(driver->Reference().original == original);
    counts.push_back(result.slices.nPointGlobal);
    // The reference publication is idempotent even if a developer repeats the acceptance notification.
    const auto accepted = driver->Reference().accepted_edges;
    result.onAccepted();
    notificationCopy();
    CHECK(driver->Reference().accepted_edges == accepted);
    double maxError = 0;
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p) {
      su2double expected[MAXVAR] = {};
      field(driver->Geometry().nodes->GetCoord(p), expected);
      for (unsigned short k = 0; k < driver->Flow().GetnVar(); ++k)
        maxError = std::max(maxError, RelDiff(driver->Flow().GetNodes()->GetSolution(p, k), expected[k]));
    }
    maxError = CPassiveComm::Allreduce(maxError, CPassiveComm::Op::MAX);
    CHECK(maxError < (conservative ? 1e-9 : 2e-10));
    if (conservative)
      for (const auto defect : projection.GetSummary().relativeDefect) CHECK(std::abs(defect) < 1e-10);
  }
  CHECK(counts.size() == sizes.size());
  if (!counts.empty())
    CHECK(*std::min_element(counts.begin(), counts.end()) < *std::max_element(counts.begin(), counts.end()));
  std::string outputMesh, savedReference;
  if (counts.size() == sizes.size()) {
    outputMesh = driver->AdaptedMeshName(8, 0) + driver->Config().GetMesh_Out_FileExtension();
    driver->WriteAdaptedMesh(8);
    savedReference = EncodeReference(driver->Reference());
    ReferenceState sidecar;
    CHECK(LoadReference(outputMesh + ".native_ref", sidecar));
    CHECK(EncodeReference(sidecar) == savedReference);
  }
  {
    Mute mute;
    driver->Finalize();
    driver.reset();
  }
  if (!outputMesh.empty()) {
    if (world.rank == 0) {
      std::ifstream source(name + ".cfg");
      std::stringstream text;
      text << source.rdbuf();
      auto options = text.str();
      const auto originalName = name + ".su2";
      options.replace(options.find(originalName), originalName.size(), outputMesh);
      std::ofstream replacement(name + "_reload.cfg");
      replacement << options;
    }
    SU2_MPI::Barrier(SU2_MPI::GetComm());
    const auto cfg = name + "_reload.cfg";
    driver = std::make_unique<NativeDriver>(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
    auto reloadedBackend = driver->Backend();  // Must validate the persisted original against this adapted mesh.
    CHECK(EncodeReference(driver->Reference()) == savedReference);
    CHECK(driver->Geometry().GetGlobal_nPointDomain() == counts.back());
    {
      Mute mute;
      driver->Finalize();
      driver.reset();
    }
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  if (world.rank == 0) {
    std::remove((name + ".cfg").c_str());
    std::remove((name + ".su2").c_str());
    std::remove((name + "_history.csv").c_str());
    std::remove((name + "_reload.cfg").c_str());
    if (!outputMesh.empty()) {
      std::remove(outputMesh.c_str());
      std::remove((outputMesh + ".native_ref").c_str());
    }
  }
}
