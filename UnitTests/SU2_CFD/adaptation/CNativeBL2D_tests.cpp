/*!
 * \file CNativeBL2D_tests.cpp
 * \brief Repeated coupled wall/first-altitude adaptation of a real distributed viscous SU2 geometry.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeField2D.hpp"
#include "../../../Common/include/adaptation/CNativeEngine2D.hpp"
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../../SU2_CFD/include/solvers/CTurbSolver.hpp"
#include <cstdlib>
#include <fstream>
#include <iomanip>

using namespace transfer_test;
using namespace SU2NativeBoundary2D;
namespace {
class WallDriver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::CSinglezoneDriver;
  using CSinglezoneDriver::AdaptedMeshName;
  using CSinglezoneDriver::WriteAdaptedMesh;
  CConfig& Config() { return *config_container[ZONE_0]; }
  CGeometry& Geometry() { return *geometry_container[ZONE_0][INST_0][MESH_0]; }
  CSolver& Flow() { return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]; }
  CTurbSolver* Turbulence() {
    return static_cast<CTurbSolver*>(solver_container[ZONE_0][INST_0][MESH_0][TURB_SOL]);
  }
  std::unique_ptr<CRemesher> Backend() { return MakeRemesher(); }
  const ReferenceState& Reference() const { return *nativeReference; }
  void ProduceMetric() {
    Flow().Preprocessing(&Geometry(), solver_container[ZONE_0][INST_0][MESH_0], &Config(), MESH_0, 0,
                         RUNTIME_FLOW_SYS, false);
    ComputeMetric();
  }
  void Snapshot() {
    const auto stopped = StopCalc;
    StopCalc = true;
    Output(0);
    StopCalc = stopped;
  }
};
std::string WallOptions(double height, const std::string& name, bool opposing = false) {
  std::ostringstream out;
  out << std::setprecision(17)
      << "SOLVER= NAVIER_STOKES\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\n"
         "REYNOLDS_NUMBER= 1000\nREYNOLDS_LENGTH= 0.04\n"
         "MESH_FORMAT= SU2\nMESH_FILENAME= "
      << name
      << ".su2\n"
      << (opposing ? "MARKER_FAR= (left, right)\nMARKER_HEATFLUX= (lower_a, 0, lower_b, 0, upper, 0)\n"
                   : "MARKER_FAR= (left, right, upper)\nMARKER_HEATFLUX= (lower_a, 0, lower_b, 0)\n")
      << "COMPUTE_METRIC= YES\nADAP_SENSOR= MACH\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\n"
         "ADAP_HMIN= 1e-4\nADAP_HMAX= 0.04\nADAP_COMPLEXITY= 20\nADAP_HAUSD= 1e-8\n"
      << (opposing ? "ADAP_BL_MARKER= (lower_a, lower_b, upper)\n" : "ADAP_BL_MARKER= (lower_a, lower_b)\n")
      << "ADAP_BL_FIRST_HEIGHT= ("
      << height
      << ")\n"
         "ADAP_BL_GROWTH= 1.2\nADAP_BL_THICKNESS= 0.016\n"
         "MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\n"
         "MUSCL_FLOW= NO\nCFL_NUMBER= 1\nITER= 2\nWRT_ADAP_MESH= NO\nOUTPUT_FILES= (RESTART)\n"
         "CONV_FILENAME= "
      << name << "_history\n";
  return out.str();
}

std::vector<std::string> Tags(const CConfig& config, const CGeometry& geometry) {
  std::vector<std::string> result;
  for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) result.push_back(config.GetMarker_All_TagBound(m));
  return result;
}
}  // namespace

static void CheckChangingBL(bool stepMetric) {
  World world;
  const bool opposing = GENERATE(false, true);
  const bool conservative = GENERATE(false, true);
  const std::string name = std::string(opposing ? "native_bl_opposing_" : "native_bl_") +
                           (conservative ? "conservative" : "barycentric");
  if (world.rank == 0) {
    auto mesh = BoxMesh(2, 4, false);
    for (auto& x : mesh.coord) x *= .02;
    simplex_test::WriteSU2Mesh(mesh, name + ".su2");
    std::ofstream cfg(name + ".cfg");
    cfg << WallOptions(.004, name, opposing);
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  const auto cfg = name + ".cfg";
  auto driver = std::make_unique<WallDriver>(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
  // Admissible affine conservative fields, with exactly zero momentum on the no-slip wall.
  auto expected = [opposing](Point p) {
    return std::array<double, 4>{1.2 + .1 * p.x, opposing ? 0. : 20 * p.y, 0., 3 + .01 * p.x};
  };
  for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p) {
    const auto values = expected({SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p, 0)),
                                  SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p, 1))});
    for (unsigned short k = 0; k < 4; ++k) driver->Flow().GetNodes()->SetSolution(p, k, values[k]);
  }
  auto backend = driver->Backend();
  const auto original = driver->Reference().original;
  CBarycentricTransfer barycentric;
  CConservativeTransfer projection;
  CSolutionTransfer& transfer = conservative ? static_cast<CSolutionTransfer&>(projection) : barycentric;
  const std::array<double, 8> heights{.004, .004, .003, .003, .005, .005, .004, .0045};
  const std::array<double, 8> centers{.01, .03, .01, .03, .02, .03, .01, .02};
  size_t completed = 0;
  for (size_t cycle = 0; cycle < heights.size(); ++cycle) {
    const auto height = heights[cycle], center = centers[cycle];
    auto& geometry = driver->Geometry();
    // A fresh request changes h0 without changing geometry/solution ownership or the retained original reference.
    std::stringstream options(WallOptions(height, name, opposing));
    std::unique_ptr<CConfig> requested;
    {
      Mute mute;
      requested = std::make_unique<CConfig>(options, SU2_COMPONENT::SU2_CFD, false);
    }
    requested->SetnMarker_All(geometry.GetnMarker());
    for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) {
      requested->SetMarker_All_TagBound(m, driver->Config().GetMarker_All_TagBound(m));
      requested->SetMarker_All_KindBC(m, driver->Config().GetMarker_All_KindBC(m));
    }
    su2activematrix metric(geometry.GetnPointDomain(), 3);
    for (unsigned long p = 0; p < metric.rows(); ++p) {
      const auto x = SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 0));
      const auto normalized = (x - center) / .006;
      // A nodal step remains a continuous P1 target inside each frozen donor cell.
      // Both amplitudes match the smooth control; only the spatial variation changes.
      const auto tangential = stepMetric ? (x < center ? .004 : .014)
                                         : .014 - .010 * std::exp(-.5 * normalized * normalized);
      metric(p, 0) = 1 / (tangential * tangential);
      metric(p, 1) = 0;
      metric(p, 2) = 1 / (height * height);
    }
    const auto artifact = name + "_cycle_" + std::to_string(cycle);
    if (std::getenv("SU2_NATIVE_SAVE_AUDIT")) {
      CMeshGather gather(geometry);
      const auto donor = gather.GatherMesh(driver->Config(), Tags(driver->Config(), geometry), false);
      const auto tensors = gather.Gather(metric.data(), 3);
      if (world.rank == 0) {
        simplex_test::WriteSU2Mesh(donor, artifact + "_donor.su2");
        std::ofstream csv(artifact + "_metric.csv");
        csv << std::setprecision(17) << "xx,xy,yy\n";
        for (size_t p = 0; p < donor.GetnPoint(); ++p)
          csv << tensors[3 * p] << ',' << tensors[3 * p + 1] << ',' << tensors[3 * p + 2] << '\n';
      }
    }
    INFO("cycle=" << cycle << ", h0=" << height << ", refinement center=" << center << ", step=" << stepMetric);
    const auto acceptedReference = EncodeReference(driver->Reference());
    const auto acceptedCount = geometry.GetnPoint();
    std::vector<double> acceptedSolution;
    for (unsigned long p = 0; p < acceptedCount; ++p)
      for (unsigned short k = 0; k < 4; ++k)
        acceptedSolution.push_back(SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, k)));
    const auto result = backend->Remesh(*requested, geometry, metric);
    CHECK(result.status == CRemeshResult::Status::COMPLETE);
    if (result.status != CRemeshResult::Status::COMPLETE) {
      CHECK(EncodeReference(driver->Reference()) == acceptedReference);
      REQUIRE(geometry.GetnPoint() == acceptedCount);
      for (unsigned long p = 0; p < acceptedCount; ++p)
        for (unsigned short k = 0; k < 4; ++k)
          CHECK(SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, k)) == acceptedSolution[4 * p + k]);
      // A one-rank diagnostic writes the rejected reader candidate without publishing
      // it to the driver: the old CFD mesh and solution remain the accepted state.
      if (world.size == 1 && std::getenv("SU2_NATIVE_SAVE_AUDIT")) {
        CSimplexMesh rejected;
        rejected.nDim = 2;
        for (size_t p = 0; p < result.slices.nPointLocal; ++p)
          for (int d = 0; d < 2; ++d) rejected.coord.push_back(result.slices.coord[d][p]);
        for (size_t e = 0; e < result.slices.nElemLocal; ++e)
          for (int k = 0; k < 3; ++k) rejected.elem.push_back(result.slices.elemRows[e * SU2_CONN_SIZE + 2 + k]);
        for (size_t m = 0; m < result.slices.markerNames.size(); ++m) {
          CSimplexMesh::Marker marker;
          marker.name = result.slices.markerNames[m];
          for (size_t row = 0; row < result.slices.boundaryRows[m].size(); row += SU2_CONN_SIZE)
            for (int k = 0; k < 2; ++k) marker.elem.push_back(result.slices.boundaryRows[m][row + 2 + k]);
          rejected.markers.push_back(std::move(marker));
        }
        simplex_test::WriteSU2Mesh(rejected, artifact + "_rejected.su2");
      }
      break;
    }
    driver->ReplaceMesh(result, transfer);
    CHECK(driver->Reference().original == original);
    CMeshGather gather(driver->Geometry());
    const auto mesh = gather.GatherMesh(driver->Config(), Tags(driver->Config(), driver->Geometry()), false);
    if (world.rank == 0) {
      if (std::getenv("SU2_NATIVE_SAVE_AUDIT")) simplex_test::WriteSU2Mesh(mesh, artifact + "_adapted.su2");
      double maxHeightError = 0;
      size_t walls = 0, upperWalls = 0;
      for (const auto& marker : mesh.markers)
        if (marker.name == "lower_a" || marker.name == "lower_b" || (opposing && marker.name == "upper"))
          for (size_t f = 0; f < marker.elem.size(); f += 2) {
            const auto a = marker.elem[f], b = marker.elem[f + 1];
            for (size_t e = 0; e < mesh.elem.size(); e += 3) {
              bool hasA = false, hasB = false;
              unsigned long opposite = 0;
              for (int k = 0; k < 3; ++k) {
                const auto p = mesh.elem[e + k];
                hasA |= p == a;
                hasB |= p == b;
                if (p != a && p != b) opposite = p;
              }
              if (hasA && hasB) {
                const double ax = mesh.coord[2 * a], ay = mesh.coord[2 * a + 1], bx = mesh.coord[2 * b],
                             by = mesh.coord[2 * b + 1], cx = mesh.coord[2 * opposite],
                             cy = mesh.coord[2 * opposite + 1];
                const auto altitude =
                    std::abs((bx - ax) * (cy - ay) - (by - ay) * (cx - ax)) / std::hypot(bx - ax, by - ay);
                maxHeightError = std::max(maxHeightError, std::abs(altitude / height - 1));
                ++walls;
                upperWalls += marker.name == "upper";
              }
            }
          }
      CHECK(walls >= 2);
      if (opposing) CHECK(upperWalls >= 2);
      CHECK(maxHeightError < 1e-8);
      std::cout << "[native BL] cycle=" << cycle << " h0=" << height << " center=" << center
                << " points=" << mesh.GetnPoint() << " wall faces=" << walls << " upper faces=" << upperWalls
                << " altitude error=" << maxHeightError
                << '\n';
    }
    double fieldError = 0;
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p) {
      const auto values = expected({SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p, 0)),
                                    SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p, 1))});
      for (unsigned short k = 0; k < 4; ++k)
        fieldError = std::max(fieldError, RelDiff(driver->Flow().GetNodes()->GetSolution(p, k), values[k]));
    }
    fieldError = CPassiveComm::Allreduce(fieldError, CPassiveComm::Op::MAX);
    CHECK(fieldError < 2e-8);
    if (conservative)
      for (const auto defect : projection.GetSummary().relativeDefect) CHECK(std::abs(defect) < 1e-10);
    ++completed;
  }
  CHECK(completed == heights.size());
  driver->Finalize();
  driver.reset();
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  if (world.rank == 0) {
    std::remove((name + ".cfg").c_str());
    std::remove((name + ".su2").c_str());
    std::remove((name + "_history.csv").c_str());
  }
}

TEST_CASE("Native SU2 BL: moving tangential refinement, coarsening and changed first altitude", "[NativeBL2D]") {
  CheckChangingBL(false);
}

// Opt-in stress: identical heights/transfers/geometry, abrupt moving nodal target.
TEST_CASE("Native SU2 BL: moving nodal step with changing first altitude", "[NativeBLStep2D][.]") {
  CheckChangingBL(true);
}

TEST_CASE("Native SU2 BL: SU2-produced sensor/BL metric and resumed viscous solve", "[NativeProducedBL2D]") {
  World world;
  const std::string turbulence = GENERATE("NONE", "SA", "SST");
#ifdef HAVE_CGNS
  const bool cgns = GENERATE(false, true);
#else
  const bool cgns = false;
#endif
  const std::string name = std::string(cgns ? "native_produced_bl_cgns" : "native_produced_bl") +
                           (turbulence == "NONE" ? "" : "_" + turbulence);
  if (world.rank == 0) {
    auto mesh = BoxMesh(2, 4, false);
    for (auto& x : mesh.coord) x *= .02;
    simplex_test::WriteSU2Mesh(mesh, name + ".su2");
    auto options = WallOptions(.004, name);
    const std::string oldMinimum = "ADAP_HMIN= 1e-4";
    options.replace(options.find(oldMinimum), oldMinimum.size(), "ADAP_HMIN= 0.004");
    if (turbulence != "NONE") {
      const std::string solver = "SOLVER= NAVIER_STOKES";
      options.replace(options.find(solver), solver.size(), "SOLVER= RANS");
      options += "KIND_TURB_MODEL= " + turbulence + "\n";
    }
    std::ofstream cfg(name + ".cfg");
    cfg << options << "ADAP_LOOP= YES\nADAP_SIZES= (20, 35, 12)\nADAP_SUBITER= (1)\n"
                   << "RESTART_FILENAME= " << name << "_restart\nSOLUTION_FILENAME= " << name
                   << "_restart\nMESH_OUT_FILENAME= " << name << "_mesh\nOUTPUT_WRT_FREQ= 1\n"
                   << "MESH_OUT_FORMAT= " << (cgns ? "CGNS" : "SU2") << '\n';
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  const auto cfg = name + ".cfg";
  auto driver = std::make_unique<WallDriver>(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
  auto backend = driver->Backend();
  const auto original = driver->Reference().original;
  CConservativeTransfer transfer;
  size_t completed = 0;
  const unsigned short nTurb = driver->Turbulence() ? driver->Turbulence()->GetnVar() : 0;
  auto checkConservation = [&]() {
    const auto& summary = transfer.GetSummary();
    REQUIRE(summary.relativeDefect.size() == 4 + nTurb);
    for (unsigned short k = 0; k < 4; ++k) CHECK(std::abs(summary.relativeDefect[k]) < 1e-10);
    if (nTurb) REQUIRE(summary.turbulenceChange.size() == nTurb);
    for (unsigned short k = 0; k < nTurb; ++k) {
      CHECK(std::isfinite(summary.relativeDefect[4 + k]));
      CHECK(std::isfinite(summary.turbulenceChange[k]));
      if (world.rank == 0)
        std::cout << "[native RANS] " << turbulence << " field=" << k
                  << " relative integral change=" << summary.relativeDefect[4 + k]
                  << " bounds-step change=" << summary.turbulenceChange[k] << '\n';
    }
  };
  auto checkTurbulence = [&](bool transferred = false) {
    if (!nTurb) return;
    CPointDirectory owners;
    std::vector<uint64_t> gids;
    std::vector<char> records;
    bool admissible = true;
    double maximum = 0;
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p) {
      const bool owned = p < driver->Geometry().GetnPointDomain();
      if (owned) gids.push_back(driver->Geometry().nodes->GetGlobalIndex(p));
      for (unsigned short k = 0; k < nTurb; ++k) {
        auto value = SU2_TYPE::GetValue(driver->Turbulence()->GetNodes()->GetSolution(p, k));
        admissible &= std::isfinite(value) && value >= 0 && (k == 0 || value > 0);
        if (transferred)
          admissible &= value >= SU2_TYPE::GetValue(driver->Turbulence()->GetLowerLimit(k)) &&
                        value <= SU2_TYPE::GetValue(driver->Turbulence()->GetUpperLimit(k));
        if (owned) {
          RecordStream encode(records);
          encode(value);
          if (k == 0) maximum = std::max(maximum, value);
        }
      }
    }
    CHECK(world.sum(!admissible) == 0);
    CHECK(CPassiveComm::Allreduce(maximum, CPassiveComm::Op::MAX) > 0);
    owners.Build(gids, records, 8 * nTurb, "native transferred turbulence owners");
    gids.clear();
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p)
      gids.push_back(driver->Geometry().nodes->GetGlobalIndex(p));
    const auto fetched = owners.Fetch(gids);
    double error = 0;
    for (unsigned long p = 0; p < gids.size(); ++p) {
      const char* data = fetched.data() + 8 * nTurb * p;
      for (unsigned short k = 0; k < nTurb; ++k)
        error = std::max(error, RelDiff(driver->Turbulence()->GetNodes()->GetSolution(p, k), GetBytes<double>(data)));
    }
    CHECK(CPassiveComm::Allreduce(error, CPassiveComm::Op::MAX) < 1e-12);
  };
  for (unsigned short cycle = 0; cycle < 3; ++cycle) {
    driver->Config().SetAdap_MetricLevel(cycle);
    auto& geometry = driver->Geometry();
    const auto center = cycle == 1 ? .03 : .01;
    // Prescribe an admissible varying no-slip field, then use the actual SU2
    // primitive/sensor/Hessian/complexity/BL pipeline to produce every tensor.
    for (unsigned long p = 0; p < geometry.GetnPoint(); ++p) {
      const double x = SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 0));
      const double y = SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 1));
      const auto s = (x - center) / .006;
      std::array<double, 4> field{1.2 + .1 * x, 20 * y * (1 + .25 * std::exp(-.5 * s * s)), 0., 3 + .01 * x};
      if (auto* turb = driver->Turbulence()) {
        const double value = (turbulence == "SST" ? .05 : .001) * y;
        turb->GetNodes()->SetSolution(p, 0, value);
        if (turbulence == "SST") {
          turb->GetNodes()->SetSolution(p, 1, 100 + 10 * x);
          field[3] += field[0] * value;
        }
      }
      for (unsigned short k = 0; k < 4; ++k) driver->Flow().GetNodes()->SetSolution(p, k, field[k]);
    }
    driver->ProduceMetric();
    const auto& metric = driver->Flow().GetNodes()->GetMetric();
    const auto artifact = name + "_cycle_" + std::to_string(cycle);
    if (std::getenv("SU2_NATIVE_SAVE_AUDIT")) {
      CMeshGather gather(geometry);
      const auto donor = gather.GatherMesh(driver->Config(), Tags(driver->Config(), geometry), false);
      const auto tensors = gather.Gather(metric.data(), 3);
      if (world.rank == 0) {
        simplex_test::WriteSU2Mesh(donor, artifact + "_donor.su2");
        std::ofstream csv(artifact + "_metric.csv");
        csv << std::setprecision(17) << "xx,xy,yy\n";
        for (size_t p = 0; p < donor.GetnPoint(); ++p)
          csv << tensors[3 * p] << ',' << tensors[3 * p + 1] << ',' << tensors[3 * p + 2] << '\n';
      }
    }
    INFO("SU2-produced metric cycle=" << cycle);
    const auto result = backend->Remesh(driver->Config(), geometry, metric);
    CHECK(result.status == CRemeshResult::Status::COMPLETE);
    if (result.status != CRemeshResult::Status::COMPLETE) break;
    driver->ReplaceMesh(result, transfer);
    CHECK(driver->Reference().original == original);
    checkConservation();
    checkTurbulence(true);
    if (std::getenv("SU2_NATIVE_SAVE_AUDIT")) {
      CMeshGather gather(driver->Geometry());
      const auto adapted = gather.GatherMesh(driver->Config(), Tags(driver->Config(), driver->Geometry()), false);
      if (world.rank == 0) simplex_test::WriteSU2Mesh(adapted, artifact + "_adapted.su2");
    }
    std::vector<double> before, turbBefore;
    for (unsigned long p = 0; p < driver->Geometry().GetnPointDomain(); ++p)
      for (unsigned short k = 0; k < 4; ++k)
        before.push_back(SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, k)));
    for (unsigned long p = 0; p < driver->Geometry().GetnPointDomain(); ++p)
      for (unsigned short k = 0; k < nTurb; ++k)
        turbBefore.push_back(SU2_TYPE::GetValue(driver->Turbulence()->GetNodes()->GetSolution(p, k)));
    driver->Run();
    driver->Postprocess();
    driver->Update();
    bool admissible = true;
    double change = 0, turbChange = 0;
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p) {
      const auto rho = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, 0));
      const auto mx = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, 1));
      const auto my = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, 2));
      const auto energy = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, 3));
      const double rhoK = turbulence == "SST"
                              ? rho * SU2_TYPE::GetValue(driver->Turbulence()->GetNodes()->GetSolution(p, 0))
                              : 0.;
      admissible &= std::isfinite(rho) && std::isfinite(mx) && std::isfinite(my) && std::isfinite(energy) && rho > 0 &&
                    energy > (mx * mx + my * my) / (2 * rho) + rhoK;
      if (p < driver->Geometry().GetnPointDomain())
        for (unsigned short k = 0; k < 4; ++k)
          change = std::max(change, std::abs(SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, k)) -
                                           before[4 * p + k]));
      if (p < driver->Geometry().GetnPointDomain())
        for (unsigned short k = 0; k < nTurb; ++k)
          turbChange = std::max(
              turbChange, std::abs(SU2_TYPE::GetValue(driver->Turbulence()->GetNodes()->GetSolution(p, k)) -
                                   turbBefore[nTurb * p + k]));
    }
    CHECK(world.sum(!admissible) == 0);
    CHECK(std::isfinite(SU2_TYPE::GetValue(driver->Flow().GetRes_RMS(0))));
    CHECK(CPassiveComm::Allreduce(change, CPassiveComm::Op::MAX) > 1e-12);
    checkTurbulence();
    if (nTurb) CHECK(CPassiveComm::Allreduce(turbChange, CPassiveComm::Op::MAX) > 1e-12);
    ++completed;
  }
  CHECK(completed == 3);
  CPointDirectory snapshot;
  std::string outputMesh, savedReference;
  if (completed == 3) {
    std::vector<uint64_t> gids;
    std::vector<char> records;
    for (unsigned long p = 0; p < driver->Geometry().GetnPointDomain(); ++p) {
      gids.push_back(driver->Geometry().nodes->GetGlobalIndex(p));
      RecordStream encode(records);
      for (unsigned short k = 0; k < 4; ++k) {
        auto value = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, k));
        encode(value);
      }
      for (unsigned short k = 0; k < nTurb; ++k) {
        auto value = SU2_TYPE::GetValue(driver->Turbulence()->GetNodes()->GetSolution(p, k));
        encode(value);
      }
    }
    snapshot.Build(gids, records, 8 * (4 + nTurb), "native BL restart solution");
    driver->WriteAdaptedMesh(3);
    outputMesh = driver->AdaptedMeshName(3, 0) + driver->Config().GetMesh_Out_FileExtension();
    savedReference = EncodeReference(driver->Reference());
    driver->Snapshot();
  }
  driver->Finalize();
  driver.reset();
  if (!outputMesh.empty()) {
    if (world.rank == 0) {
      std::ifstream source(name + ".cfg");
      std::stringstream text;
      text << source.rdbuf();
      auto options = text.str();
      options.replace(options.find(name + ".su2"), name.size() + 4, outputMesh);
      if (cgns) options.replace(options.find("MESH_FORMAT= SU2"), 16, "MESH_FORMAT= CGNS");
      std::ofstream restart(name + "_reload.cfg");
      restart << options << "RESTART_SOL= YES\n";
    }
    SU2_MPI::Barrier(SU2_MPI::GetComm());
    const auto restart = name + "_reload.cfg";
    driver = std::make_unique<WallDriver>(const_cast<char*>(restart.c_str()), 1, SU2_MPI::GetComm());
    auto restoredBackend = driver->Backend();
    CHECK(EncodeReference(driver->Reference()) == savedReference);
    std::vector<uint64_t> gids;
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p)
      gids.push_back(driver->Geometry().nodes->GetGlobalIndex(p));
    const auto records = snapshot.Fetch(gids);
    double error = 0;
    for (unsigned long p = 0; p < gids.size(); ++p) {
      const char* data = records.data() + 8 * (4 + nTurb) * p;
      for (unsigned short k = 0; k < 4; ++k)
        error = std::max(error, RelDiff(driver->Flow().GetNodes()->GetSolution(p, k), GetBytes<double>(data)));
      for (unsigned short k = 0; k < nTurb; ++k)
        error = std::max(error, RelDiff(driver->Turbulence()->GetNodes()->GetSolution(p, k), GetBytes<double>(data)));
    }
    CHECK(CPassiveComm::Allreduce(error, CPassiveComm::Op::MAX) < 1e-12);
    checkTurbulence();
    if (cgns) {
      // Remesh the restored viscous CGNS geometry too: physical wall labels,
      // retained reference and first-height protection must survive this reader.
      const auto reloadedOriginal = driver->Reference().original;
      driver->Config().SetAdap_MetricLevel(2);
      driver->ProduceMetric();
      const auto result = restoredBackend->Remesh(driver->Config(), driver->Geometry(),
                                                driver->Flow().GetNodes()->GetMetric());
      CHECK(result.status == CRemeshResult::Status::COMPLETE);
      if (result.status == CRemeshResult::Status::COMPLETE) {
        driver->ReplaceMesh(result, transfer);
        CHECK(driver->Reference().original == reloadedOriginal);
        checkConservation();
        checkTurbulence(true);
      }
    }
    driver->Finalize();
    driver.reset();
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  if (world.rank == 0) {
    std::remove((name + ".cfg").c_str());
    std::remove((name + ".su2").c_str());
    std::remove((name + "_history.csv").c_str());
    std::remove((name + "_reload.cfg").c_str());
    if (!std::getenv("SU2_NATIVE_SAVE_AUDIT")) {
      std::remove((name + "_restart.dat").c_str());
      std::remove((name + "_restart.meta").c_str());
      std::remove(outputMesh.c_str());
      std::remove((outputMesh + ".native_ref").c_str());
    }
  }
}

namespace {
MetricComposition ReplayBLTarget(const PolylineReference& reference,int marker,double h,double g,double thickness,
                                 double core,double angle) {
  CBoundaryLayerMetric::Wall wall;
  wall.layer.firstHeight = h;
  wall.layer.growth = g;
  wall.layer.thickness = thickness;
  wall.layer.marker = "captured_wall";
  std::map<Id, unsigned long> points;
  for (const auto& component : reference.Components())
    if (component.marker == marker)
      for (size_t k = 1; k < component.nodes.size(); ++k)
        for (const auto node : {component.nodes[k - 1], component.nodes[k]}) {
          const auto insertion = points.emplace(node.id, points.size());
          if (insertion.second) {
            wall.coord.push_back(node.p.x);
            wall.coord.push_back(node.p.y);
          }
          wall.conn.push_back(insertion.first->second);
        }
  auto layers=std::make_shared<CBoundaryLayerMetric>(2, std::vector<CBoundaryLayerMetric::Wall>{wall}, angle);
  return [layers,core](Point p, SU2Native2D::Tensor m) {
    const su2double coord[] = {p.x, p.y};
    CBoundaryLayerMetric::Tensor target;
    target.m[0][0] = m.xx;
    target.m[0][1] = target.m[1][0] = m.xy;
    target.m[1][1] = m.yy;
    layers->ApplyPoint(coord, target, core, nullptr, false);
    return SU2Native2D::Tensor{SU2_TYPE::GetValue(target.m[0][0]), SU2_TYPE::GetValue(target.m[0][1]),
                             SU2_TYPE::GetValue(target.m[1][1])};
  };
}
}
namespace {
struct CapturedState {
  std::map<Id,Cell> current,original;
  std::vector<PolylineReference::Face> faces;
  double core=0,h=0,g=0,thickness=0,angle=0,tolerance=0;
  int marker=0;
};
CapturedState ReadCapturedState(const char* folder) {
  CapturedState state;
  auto& owned=state.current;auto& original=state.original;auto& faces=state.faces;
  auto& core=state.core;auto& h=state.h;auto& g=state.g;auto& thickness=state.thickness;
  auto& angle=state.angle;auto& tolerance=state.tolerance;auto& marker=state.marker;
  int ranks=1;
  for(int rank=0;rank<ranks;++rank) {
    const auto filename=std::string(folder)+"/native_rejected_state_rank"+std::to_string(rank)+".bin";
    std::ifstream in(filename,std::ios::binary);REQUIRE(in.good());
    in.seekg(0,std::ios::end);REQUIRE(in.tellg()<=64*1024*1024);in.seekg(0);
    const std::vector<char> bytes((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    RecordStream read(bytes);
    uint64_t version;int savedRank,savedRanks,m;
    double c,h0,g0,t,a,tol;
    read(version,savedRank,savedRanks,c,m,h0,g0,t,a,tol);
    REQUIRE(version==1);REQUIRE(savedRank==rank);REQUIRE(savedRanks>=1);REQUIRE(savedRanks<=4);
    if(!rank) {ranks=savedRanks;core=c;marker=m;h=h0;g=g0;thickness=t;angle=a;tolerance=tol;}
    else {REQUIRE(savedRanks==ranks);REQUIRE(c==core);REQUIRE(m==marker);REQUIRE(h0==h);REQUIRE(g0==g);REQUIRE(t==thickness);REQUIRE(a==angle);REQUIRE(tol==tolerance);}
    auto records=[&](auto& values) {
      uint64_t count;read(count);
      typename std::decay_t<decltype(values)>::value_type value{};
      RecordStream measure;measure(value);
      REQUIRE(count<=(bytes.size()-read.Size())/measure.Size());
      values.resize(count);for(auto& entry:values) read(entry);
    };
    std::vector<Cell> current;std::vector<DonorCell> donors;std::vector<PolylineReference::Face> reference;
    records(current);records(donors);records(reference);REQUIRE(read.End());
    for(const auto& cell:current) REQUIRE(owned.emplace(cell.t.id,cell).second);
    for(const auto& d:donors) {Cell cell;cell.t=d.triangle;cell.marker=d.marker;cell.nodal_target=d.metric;REQUIRE(original.emplace(cell.t.id,cell).second);}
    if(!rank) faces=std::move(reference);
  }
  return state;
}
}

// Explicit captured-fixture regression. All ranks read the bounded local evidence;
// donor import and replacement publication then use the actual MPI protocol.
TEST_CASE("Captured blocked cavity publishes a two-point repair across owners", "[.][NativeRAEJointMPI]") {
  World world;
  const char* folder=std::getenv("NATIVE_RAE_STATE_REPLAY");REQUIRE(folder!=nullptr);
  const auto state=ReadCapturedState(folder);
  PolylineReference geometry(state.faces,state.angle);
  const auto policy=geometry.Policy({{state.marker,state.h}});
  Operation op{Action::BULK_SPLIT};double largest=1.8;
  for(const auto& entry:state.current) {
    const auto& cell=entry.second;
    for(int k=0;k<3;++k)
      if(!cell.t.protected_cell && !cell.marker[k] && cell.target_cache[k+1]>largest) {
        largest=cell.target_cache[k+1];op.a=cell.t.v[k].id;op.b=cell.t.v[(k+1)%3].id;
      }
  }
  REQUIRE(largest>1.8);
  std::set<Id> seeds{op.a,op.b};std::vector<Cell> old;
  for(int ring=0;ring<2;++ring) {
    old.clear();
    for(const auto& entry:state.current)
      if((seeds.count(entry.second.t.v[0].id)||seeds.count(entry.second.t.v[1].id)||seeds.count(entry.second.t.v[2].id)))
        old.push_back(entry.second);
    if(!ring) for(const auto& cell:old) for(const auto& node:cell.t.v) seeds.insert(node.id);
  }
  REQUIRE(old.size()>MAX_CAVITY);REQUIRE(old.size()<=PATCH_LIMIT);
  std::map<Id,Cell> original,owned;
  size_t k=0;
  for(const auto& entry:state.original) if(int(k++%world.size)==world.rank) original.emplace(entry);
  for(size_t j=0;j<old.size();++j) if(int(j%world.size)==world.rank) owned.emplace(old[j].t.id,old[j]);
  DonorField donor(world,original,ReplayBLTarget(geometry,state.marker,state.h,state.g,state.thickness,state.core,state.angle));
  bool active=true;size_t maximum=0;int rejected=0;
  auto field=donor.import(old,active,2*1024*1024,state.tolerance,maximum,rejected);
  REQUIRE(active);REQUIRE(rejected==0);
  for(const auto& cell:old) for(int v=0;v<3;++v) {
    const auto p=cell.t.v[v].p;
    field->cache.emplace(std::array<double,2>{p.x,p.y},cell.nodal_target[v]);
  }
  const auto metric=checked([field](Point p){return field->evaluate(p);});
  const auto before=triangles(old);
  EngineOptions options;options.geometry_tolerance=state.tolerance;
  // Keep the actual original sensor/BL target. Engine's current-patch donor
  // provides transaction coverage; this fixture's preimported field supplies
  // the immutable original target, including every proposed interior point.
  options.metric_composition=[field](Point p,Tensor){return field->evaluate(p);};
  Engine engine(world,owned,policy,options);
  Choice choice{op,world.rank==0?1.:-1.,world.rank};
  SECTION("accepted two-point publication") {
    CHECK(world.sum(engine.round(choice,true))==1);
    CHECK(world.sum(engine.stats.joint_commits)==1);
    CHECK(world.sum(engine.stats.cross_rank)==(world.size>1?1:0));
    std::vector<std::vector<Cell>> to(world.size);
    for(const auto& cell:engine.owned) to[0].push_back(cell.second);
    const auto fresh=world.exchange(to);
    if(world.rank==0) {
      const auto after=triangles(fresh);std::string reason;
      REQUIRE(after.size()==before.size()+4);
      CHECK(nodes(after).size()==nodes(before).size()+2);
      CHECK(strict_cells(after,reason));CHECK(validate_replacement(before,after,reason));
      CHECK(min_quality(after,metric)>=.18);
      CHECK(max_length(after,metric)<=max_length(before,metric)*(1+1e-12));
      CHECK(SizeDeficit(after,metric)<SizeDeficit(before,metric)*(1-1e-8));
      for(const auto& cell:old) if(cell.t.protected_cell) {
        // Publication allocates replacement cell IDs; protected geometry is
        // matched by its unchanged vertex identities and coordinates instead.
        CHECK(std::count_if(after.begin(),after.end(),[&](const Triangle& t){
          for(int v=0;v<3;++v) if(t.v[v].id!=cell.t.v[v].id || t.v[v].p.x!=cell.t.v[v].p.x || t.v[v].p.y!=cell.t.v[v].p.y) return false;
          return t.protected_cell != 0;
        })==1);
      }
      std::cout<<"JOINT MPI ranks="<<world.size<<" old="<<before.size()<<" fresh="<<after.size()
               <<" q="<<min_quality(after,metric)<<" L="<<max_length(after,metric)<<std::endl;
    }
  }
  SECTION("participant rejection retains the original records") {
    std::vector<char> bytes;RecordStream snapshot(bytes);for(auto entry:engine.owned) snapshot(entry.second);
    choice.op.fault=4;
    CHECK_FALSE(engine.round(choice,true));CHECK(world.sum(engine.stats.joint_commits)==0);
    std::vector<char> output;RecordStream after(output);for(auto entry:engine.owned) after(entry.second);
    CHECK(output==bytes);
  }
}
