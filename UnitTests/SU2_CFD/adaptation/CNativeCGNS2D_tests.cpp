/*!
 * \file CNativeCGNS2D_tests.cpp
 * \brief Native production adaptation loop, configurable CGNS output and adapted CGNS input.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#ifdef HAVE_CGNS
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include <cstdlib>
#include <cstdio>
#include <fstream>

using namespace transfer_test;
using namespace SU2NativeBoundary2D;
namespace {
class CGNSDriver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::CSinglezoneDriver;
  using CSinglezoneDriver::AdaptedMeshName;
  CConfig& Config() { return *config_container[ZONE_0]; }
  CGeometry& Geometry() { return *geometry_container[ZONE_0][INST_0][MESH_0]; }
  CSolver& Flow() { return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]; }
  const ReferenceState& Reference() const { return *nativeReference; }
  std::unique_ptr<CRemesher> Backend() { return MakeRemesher(); }
};
std::string Options(const std::string& name, bool output, const std::string& input = "") {
  std::ostringstream text;
  text << "SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\n"
       << "MESH_FORMAT= " << (input.empty() ? "SU2" : "CGNS") << "\nMESH_FILENAME= "
       << (input.empty() ? name + ".su2" : input) << "\n"
       << "MARKER_FAR= (left, right, upper, lower_a, lower_b)\n"
          "COMPUTE_METRIC= YES\nADAP_SENSOR= MACH\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\n"
          "ADAP_HAUSD= 1e-8\nADAP_HMIN= 0.1\nADAP_HMAX= 1\nADAP_LOOP= YES\n"
          "ADAP_SIZES= (30, 12)\nADAP_SUBITER= (1)\nADAP_TRANSFER= CONSERVATIVE\n"
          "MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
          "CFL_NUMBER= 0.1\nITER= 2\nADAP_FLOW_ITER= (2)\nOUTPUT_FILES= (RESTART)\n"
       << "WRT_ADAP_MESH= " << (output ? "YES" : "NO") << "\n"
       << "CONV_FILENAME= " << name << "_history\nRESTART_FILENAME= " << name << "_restart\n"
       << "MESH_OUT_FILENAME= " << name << "_mesh\n";
  // Initial SU2 input explicitly requests CGNS output. For CGNS input, test the
  // configuration default that preserves the input format when output is enabled.
  if (input.empty()) text << "MESH_OUT_FORMAT= CGNS\n";
  return text.str();
}
}  // namespace

TEST_CASE("Native production loop: configured CGNS output, disabled output and CGNS input", "[NativeCGNS2D]") {
  World world;
  const bool output = GENERATE(false, true);
  const std::string name = output ? "native_cgns_enabled" : "native_cgns_disabled";
  if (world.rank == 0) {
    // Rank-count runs can share a working directory: stale outputs must never
    // satisfy the enabled-output checks or contaminate the disabled-output control.
    for (const auto& prefix : {name, name + "_reload"})
      for (unsigned long cycle = 0; cycle <= 2; ++cycle) {
        const auto file = CConfig::GetAdap_FileName(prefix + "_mesh", cycle) + ".cgns";
        std::remove(file.c_str());
        std::remove((file + ".native_ref").c_str());
      }
    simplex_test::WriteSU2Mesh(BoxMesh(2, 6, true), name + ".su2");
    std::ofstream cfg(name + ".cfg");
    cfg << Options(name, output);
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  const auto cfg = name + ".cfg";
  auto driver = std::make_unique<CGNSDriver>(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
  auto backend = driver->Backend();
  const auto original = driver->Reference().original;
  std::array<double, 4> uniform;
  for (unsigned short k = 0; k < 4; ++k) {
    const auto local = driver->Geometry().GetnPointDomain()
                           ? SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(0, k))
                           : -std::numeric_limits<double>::infinity();
    uniform[k] = CPassiveComm::Allreduce(local, CPassiveComm::Op::MAX);
  }
  auto uniformError = [&]() {
    double error = 0;
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p)
      for (unsigned short k = 0; k < 4; ++k)
        // Near-zero transverse momentum needs an absolute unit scale: the
        // generic affine-field helper's 1e-8 floor magnifies roundoff by 1e8.
        error = std::max(error, RelDiff(driver->Flow().GetNodes()->GetSolution(p, k), uniform[k], 1.));
    return CPassiveComm::Allreduce(error, CPassiveComm::Op::MAX);
  };
  CHECK(driver->Config().GetAdap_Mesh_Output() == output);
  driver->StartSolver();
  CHECK(uniformError() < 1e-9);
  CHECK(driver->Reference().original == original);
  const auto finalCount = driver->Geometry().GetGlobal_nPointDomain();
  CHECK(finalCount > 0);
  const auto finalMesh = driver->AdaptedMeshName(2, 0) + ".cgns";
  const auto encoded = EncodeReference(driver->Reference());
  if (world.rank == 0) {
    CHECK(std::ifstream(finalMesh).good() == output);
    CHECK(std::ifstream(finalMesh + ".native_ref").good() == output);
  }
  driver->Finalize();
  driver.reset();
  if (output) {
    if (world.rank == 0) {
      std::ofstream cfg(name + "_reload.cfg");
      cfg << Options(name + "_reload", true, finalMesh);
    }
    SU2_MPI::Barrier(SU2_MPI::GetComm());
    const auto cfg = name + "_reload.cfg";
    driver = std::make_unique<CGNSDriver>(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
    auto reloadedBackend = driver->Backend();
    CHECK(driver->Config().GetMesh_Out_FileFormat() == ENUM_GRID::CGNS_GRID);
    CHECK(driver->Geometry().GetGlobal_nPointDomain() == finalCount);
    CHECK(EncodeReference(driver->Reference()) == encoded);
    driver->StartSolver();
    CHECK(uniformError() < 1e-9);
    const auto reloadedMesh = driver->AdaptedMeshName(2, 0) + ".cgns";
    ReferenceState sidecar;
    CHECK(LoadReference(reloadedMesh + ".native_ref", sidecar));
    CHECK(EncodeReference(sidecar) == EncodeReference(driver->Reference()));
    driver->Finalize();
    driver.reset();
  }
  // Retain input/config/mesh/restart artifacts for independent CGNS auditing.
  SU2_MPI::Barrier(SU2_MPI::GetComm());
}
#endif
