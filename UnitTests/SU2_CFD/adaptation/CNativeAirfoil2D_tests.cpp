/*!
 * \file CNativeAirfoil2D_tests.cpp
 * \brief Opt-in actual SU2 sensor/BL adaptation of a realistic triangular airfoil seed.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher.hpp"
#include "../../../Common/include/adaptation/CNativeImport2D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include <array>
#include <cstdlib>
#include <fstream>
#include <iomanip>

using namespace transfer_test;
using namespace SU2NativeBoundary2D;
namespace {
class AirfoilDriver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::CSinglezoneDriver;
  CConfig& Config() { return *config_container[ZONE_0]; }
  CGeometry& Geometry() { return *geometry_container[ZONE_0][INST_0][MESH_0]; }
  CSolver& Flow() { return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]; }
  std::unique_ptr<CRemesher> Backend() { return MakeRemesher(); }
  const ReferenceState& Reference() const { return *nativeReference; }
  double TransferSeconds() const { return SU2_TYPE::GetValue(lastTransferTime); }
  double ReplaceSeconds() const { return SU2_TYPE::GetValue(lastReplaceTime); }
  void ExportAdaptedMesh(unsigned long cycle) {
    CheckAdaptedMeshNames();
    WriteAdaptedMesh(cycle);
  }
  void ProduceMetric() {
    Flow().Preprocessing(&Geometry(), solver_container[ZONE_0][INST_0][MESH_0], &Config(), MESH_0, 0,
                         RUNTIME_FLOW_SYS, false);
    ComputeMetric();
  }
};
std::vector<std::string> Tags(const CConfig& config, const CGeometry& geometry) {
  std::vector<std::string> tags;
  for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) tags.push_back(config.GetMarker_All_TagBound(m));
  return tags;
}
// Test artifact only: generation still returns distributed reader slices. Each
// duplicated element row is selected by the slice containing its smallest node.
CSimplexMesh GatherCandidate(World& world, const CReaderSlices& slices) {
  REQUIRE(slices.nDim == 2);
  REQUIRE(slices.coord.size() == 2);
  std::vector<std::vector<Node>> points(world.size);
  std::vector<std::vector<std::array<Id, 4>>> elements(world.size);
  for (unsigned long p = 0; p < slices.nPointLocal; ++p)
    points[0].push_back({slices.firstPoint + p, {slices.coord[0][p], slices.coord[1][p]}, 0});
  for (unsigned long e = 0; e < slices.nElemLocal; ++e) {
    const auto* row = slices.elemRows.data() + e * SU2_CONN_SIZE;
    const auto smallest = std::min({row[2], row[3], row[4]});
    if (smallest >= slices.firstPoint && smallest < slices.firstPoint + slices.nPointLocal)
      elements[0].push_back({row[0], row[2], row[3], row[4]});
  }
  CHECK(CPassiveComm::Allreduce(uint64_t(points[0].size()), CPassiveComm::Op::SUM) == slices.nPointGlobal);
  CHECK(CPassiveComm::Allreduce(uint64_t(elements[0].size()), CPassiveComm::Op::SUM) == slices.nElemGlobal);
  const auto allPoints = world.exchange(points);
  const auto allElements = world.exchange(elements);
  CSimplexMesh mesh;
  mesh.nDim = 2;
  if (world.rank == 0) {
    mesh.coord.resize(2 * slices.nPointGlobal);
    mesh.elem.resize(3 * slices.nElemGlobal);
    for (const auto& p : allPoints) {
      mesh.coord[2 * p.id] = p.p.x;
      mesh.coord[2 * p.id + 1] = p.p.y;
    }
    for (const auto& e : allElements)
      for (int k = 0; k < 3; ++k) mesh.elem[3 * e[0] + k] = e[k + 1];
    for (size_t m = 0; m < slices.markerNames.size(); ++m) {
      CSimplexMesh::Marker marker;
      marker.name = slices.markerNames[m];
      for (size_t row = 0; row < slices.boundaryRows[m].size(); row += SU2_CONN_SIZE)
        for (int k = 0; k < 2; ++k) marker.elem.push_back(slices.boundaryRows[m][row + 2 + k]);
      mesh.markers.push_back(std::move(marker));
    }
  }
  return mesh;
}
}  // namespace

TEST_CASE("Native airfoil artifacts: distributed candidate slices gather exactly once", "[NativeAirfoilSlices2D]") {
  World world;
  const auto expected = BoxMesh(2, 4, true);
  const auto slices = CReaderSlices::FromComplete(expected);
  const auto actual = GatherCandidate(world, slices);
  if (world.rank == 0) {
    CHECK(actual.coord == expected.coord);
    CHECK(actual.elem == expected.elem);
    REQUIRE(actual.markers.size() == expected.markers.size());
    for (size_t m = 0; m < actual.markers.size(); ++m) {
      CHECK(actual.markers[m].name == expected.markers[m].name);
      CHECK(actual.markers[m].elem == expected.markers[m].elem);
    }
  }
}

// Hidden from the default unit suite: this runs a real airfoil and needs an
// explicitly supplied configuration. The integration runner records its inputs.
TEST_CASE("Native realistic airfoil: actual sensor/BL targets and three changed levels", "[NativeAirfoil2D][.]") {
  World world;
  const auto input = std::getenv("SU2_NATIVE_AIRFOIL_CONFIG");
  REQUIRE(input != nullptr);
  std::ifstream source(input);
  REQUIRE(source.good());
  std::stringstream options;
  options << source.rdbuf();
  const std::string cfg = "native_airfoil.cfg";
  if (world.rank == 0) {
    std::ofstream output(cfg);
    output << options.str();
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  auto driver = std::make_unique<AirfoilDriver>(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
  REQUIRE(driver->Config().GetnAdap_Levels() >= 3);
  REQUIRE(driver->Config().GetnAdap_BL() == 1);
  auto backend = driver->Backend();
  const auto original = driver->Reference().original;
  const auto wallTag = driver->Config().GetAdap_BL(0).marker;
  const double height = SU2_TYPE::GetValue(driver->Config().GetAdap_BL(0).firstHeight);
  CConservativeTransfer transfer;
  size_t completed = 0;
  auto checkFlow = [&]() {
    bool admissible = true;
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p) {
      const double rho = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, 0));
      const double mx = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, 1));
      const double my = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, 2));
      const double energy = SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, 3));
      admissible &= std::isfinite(rho) && std::isfinite(mx) && std::isfinite(my) && std::isfinite(energy) && rho > 0 &&
                    energy > (mx * mx + my * my) / (2 * rho);
    }
    CHECK(world.sum(!admissible) == 0);
    CHECK(std::isfinite(SU2_TYPE::GetValue(driver->Flow().GetRes_RMS(0))));
  };
  // Inspectable field snapshots in the same global point order as GatherMesh.
  // Outside phase timers; these are conserved flow fields, not SU2 restart files.
  auto saveSolution = [&](const CMeshGather& gather, const CSimplexMesh& mesh, const std::string& artifact) {
    const auto& geometry = driver->Geometry();
    std::vector<su2double> local(4 * geometry.GetnPointDomain());
    for (unsigned long p = 0; p < geometry.GetnPointDomain(); ++p)
      for (unsigned short k = 0; k < 4; ++k) local[4 * p + k] = driver->Flow().GetNodes()->GetSolution(p, k);
    const auto values = gather.Gather(local.data(), 4);
    if (world.rank == 0) {
      REQUIRE(values.size() == 4 * mesh.GetnPoint());
      std::ofstream csv(artifact + "_solution.csv");
      csv << std::setprecision(17) << "point_id,x,y,density,momentum_x,momentum_y,energy_density\n";
      for (unsigned long p = 0; p < mesh.GetnPoint(); ++p) {
        csv << p << ',' << mesh.coord[2 * p] << ',' << mesh.coord[2 * p + 1];
        for (unsigned short k = 0; k < 4; ++k) csv << ',' << SU2_TYPE::GetValue(values[4 * p + k]);
        csv << '\n';
      }
      REQUIRE(csv.good());
    }
  };
  for (unsigned short cycle = 0; cycle < 3; ++cycle) {
    driver->Config().SetAdap_MetricLevel(cycle);
    driver->Config().SetAdap_FlowLevel(cycle);
    // Real driver phases, separate from the test-only snapshot gathers/audits.
    // Solve includes the existing inner-iteration monitoring and file output.
    std::array<double, 7> seconds{};
    auto phaseStart = SU2_MPI::Wtime();
    driver->Run();
    seconds[0] = SU2_MPI::Wtime() - phaseStart;
    checkFlow();
    phaseStart = SU2_MPI::Wtime();
    driver->ProduceMetric();
    seconds[1] = SU2_MPI::Wtime() - phaseStart;
    auto writeTiming = [&](bool accepted) {
      std::array<double, 7> maximum{};
      for (size_t k = 0; k < seconds.size(); ++k) {
        CHECK(std::isfinite(seconds[k]));
        CHECK(seconds[k] >= 0);
        maximum[k] = CPassiveComm::Allreduce(seconds[k], CPassiveComm::Op::MAX);
      }
      // Cumulative whole-process peak, including startup and earlier test snapshots.
      // Per-rank values expose imbalance; this is not a remesher-only allocation peak.
      uint64_t hwmKiB = 0;
      std::ifstream processStatus("/proc/self/status");
      std::string statusLine;
      while (std::getline(processStatus, statusLine))
        if (statusLine.compare(0, 6, "VmHWM:") == 0) hwmKiB = std::stoull(statusLine.substr(6));
      CHECK(hwmKiB > 0);
      std::ofstream localCsv("native_airfoil_cycle_" + std::to_string(cycle) + "_timing_rank_" +
                             std::to_string(world.rank) + ".csv");
      localCsv << std::setprecision(17)
               << "ranks,rank,accepted,owned_points,total_points,local_elements,rss_hwm_kib,"
                  "solve_with_inner_output_s,metric_s,remesh_s,replace_s,transfer_s,"
                  "replace_without_transfer_s,mesh_output_s\n"
               << world.size << ',' << world.rank << ',' << accepted << ','
               << driver->Geometry().GetnPointDomain() << ',' << driver->Geometry().GetnPoint() << ','
               << driver->Geometry().GetnElem() << ',' << hwmKiB;
      for (const auto value : seconds) localCsv << ',' << value;
      localCsv << '\n';
      REQUIRE(localCsv.good());
      if (world.rank == 0) {
        std::ofstream csv("native_airfoil_cycle_" + std::to_string(cycle) + "_timing.csv");
        csv << std::setprecision(17)
            << "ranks,accepted,solve_with_inner_output_max_s,metric_max_s,remesh_max_s,replace_max_s,"
               "transfer_max_s,replace_without_transfer_max_s,mesh_output_max_s\n"
            << world.size << ',' << accepted;
        for (const auto value : maximum) csv << ',' << value;
        csv << '\n';
        REQUIRE(csv.good());
      }
    };
    auto& geometry = driver->Geometry();
    const auto& metric = driver->Flow().GetNodes()->GetMetric();
    const auto artifact = "native_airfoil_cycle_" + std::to_string(cycle);
    {
      CMeshGather gather(geometry);
      const auto donor = gather.GatherMesh(driver->Config(), Tags(driver->Config(), geometry), false);
      const auto tensors = gather.Gather(metric.data(), 3);
      saveSolution(gather, donor, artifact + "_donor");
      if (world.rank == 0) {
        simplex_test::WriteSU2Mesh(donor, artifact + "_donor.su2");
        std::ofstream csv(artifact + "_metric.csv");
        csv << std::setprecision(17) << "xx,xy,yy\n";
        for (size_t p = 0; p < donor.GetnPoint(); ++p)
          csv << tensors[3 * p] << ',' << tensors[3 * p + 1] << ',' << tensors[3 * p + 2] << '\n';
      }
    }
    INFO("realistic airfoil cycle=" << cycle);
    const auto acceptedReference = EncodeReference(driver->Reference());
    const auto acceptedCount = geometry.GetnPoint();
    std::vector<double> acceptedSolution;
    for (unsigned long p = 0; p < acceptedCount; ++p)
      for (unsigned short k = 0; k < 4; ++k)
        acceptedSolution.push_back(SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, k)));
    SU2_MPI::Barrier(SU2_MPI::GetComm());
    phaseStart = SU2_MPI::Wtime();
    const auto result = backend->Remesh(driver->Config(), geometry, metric);
    seconds[2] = SU2_MPI::Wtime() - phaseStart;
    CHECK(result.status == CRemeshResult::Status::COMPLETE);
    if (result.status != CRemeshResult::Status::COMPLETE) {
      writeTiming(false);
      CHECK(EncodeReference(driver->Reference()) == acceptedReference);
      REQUIRE(geometry.GetnPoint() == acceptedCount);
      for (unsigned long p = 0; p < acceptedCount; ++p)
        for (unsigned short k = 0; k < 4; ++k)
          CHECK(SU2_TYPE::GetValue(driver->Flow().GetNodes()->GetSolution(p, k)) == acceptedSolution[4 * p + k]);
      if (result.slices.nDim == 2 && result.slices.nPointGlobal) {
        const auto rejected = GatherCandidate(world, result.slices);
        if (world.rank == 0) simplex_test::WriteSU2Mesh(rejected, artifact + "_rejected.su2");
      }
      break;
    }
    driver->ReplaceMesh(result, transfer);
    seconds[3] = driver->ReplaceSeconds();
    seconds[4] = driver->TransferSeconds();
    seconds[5] = seconds[3] - seconds[4];
    phaseStart = SU2_MPI::Wtime();
    driver->ExportAdaptedMesh(cycle + 1);
    seconds[6] = SU2_MPI::Wtime() - phaseStart;
    writeTiming(true);
    checkFlow();
    CHECK(driver->Reference().original == original);
    CHECK(driver->Reference().marker_names.size() == 2);
    const auto& summary = transfer.GetSummary();
    const auto& projection = summary.projection;
    REQUIRE(projection.targetTotal.size() == 4);
    REQUIRE(projection.donorTotal.size() == 4);
    REQUIRE(projection.fillOpen.size() == 4);
    REQUIRE(projection.sliverOpen.size() == 4);
    REQUIRE(projection.scale.size() == 4);
    REQUIRE(summary.newIntegral.size() == 4);
    std::ofstream conservation;
    if (world.rank == 0) {
      conservation.open(artifact + "_transfer.csv");
      conservation << std::setprecision(17)
                   << "field,donor_total,fill_open,sliver_open,target_total,new_integral,scale,corrected_defect,raw_relative_defect\n";
    }
    for (unsigned short k = 0; k < 4; ++k) {
      // The existing CLOSED policy retains wall content but follows open-boundary
      // domain changes. Check that contract at the original tolerance rather than
      // demanding unchanged donor totals after physical boundary resampling.
      const auto expected = projection.donorTotal[k] + projection.fillOpen[k] - projection.sliverOpen[k];
      const auto scale = std::max(projection.scale[k], 1e-300);
      CHECK(std::abs(projection.targetTotal[k] - expected) / scale < 1e-10);
      const auto correctedDefect = (summary.newIntegral[k] - expected) / scale;
      CHECK(std::abs(correctedDefect) < 1e-10);
      if (world.rank == 0)
        conservation << k << ',' << projection.donorTotal[k] << ',' << projection.fillOpen[k] << ','
                     << projection.sliverOpen[k] << ',' << projection.targetTotal[k] << ',' << summary.newIntegral[k]
                     << ',' << projection.scale[k] << ',' << correctedDefect << ',' << summary.relativeDefect[k] << '\n';
    }
    CMeshGather gather(driver->Geometry());
    const auto mesh = gather.GatherMesh(driver->Config(), Tags(driver->Config(), driver->Geometry()), false);
    saveSolution(gather, mesh, artifact + "_adapted");
    if (world.rank == 0) {
      simplex_test::WriteSU2Mesh(mesh, artifact + "_adapted.su2");
      const auto wall = mesh.FindMarker(wallTag);
      CHECK(wall != nullptr);
      size_t incidences = 0;
      double error = 0;
      for (size_t f = 0; wall && f < wall->elem.size(); f += 2) {
        const auto a = wall->elem[f], b = wall->elem[f + 1];
        for (size_t e = 0; e < mesh.elem.size(); e += 3) {
          bool hasA = false, hasB = false;
          unsigned long other = 0;
          for (int k = 0; k < 3; ++k) {
            const auto p = mesh.elem[e + k];
            hasA |= p == a;
            hasB |= p == b;
            if (p != a && p != b) other = p;
          }
          if (!(hasA && hasB)) continue;
          const double ax = mesh.coord[2 * a], ay = mesh.coord[2 * a + 1], bx = mesh.coord[2 * b],
                       by = mesh.coord[2 * b + 1], cx = mesh.coord[2 * other], cy = mesh.coord[2 * other + 1];
          const double altitude = std::abs((bx - ax) * (cy - ay) - (by - ay) * (cx - ax)) / std::hypot(bx - ax, by - ay);
          error = std::max(error, std::abs(altitude / height - 1));
          ++incidences;
        }
      }
      CHECK(incidences == (wall ? wall->elem.size() / 2 : 0));
      CHECK(error < 1e-8);
      std::cout << "[native airfoil] cycle=" << cycle << " points=" << mesh.GetnPoint()
                << " wall faces=" << incidences << " altitude error=" << error << '\n';
    }
    ++completed;
  }
  CHECK(completed == 3);
  if (completed == 3) {
    driver->Run();
    driver->Postprocess();
    driver->Update();
    checkFlow();
  }
  driver->Finalize();
  driver.reset();
  SU2_MPI::Barrier(SU2_MPI::GetComm());
}

// Separate CFD convergence from remesher scaling: every rank count imports the
// same original sensor samples and lets the production backend compose geometry.
TEST_CASE("Native frozen airfoil: identical input metric for MPI scaling", "[NativeFrozenAirfoil2D][.]") {
  World world;
  const auto configPath = std::getenv("SU2_NATIVE_AIRFOIL_CONFIG");
  const auto metricPath = std::getenv("SU2_NATIVE_FROZEN_METRIC");
  REQUIRE(configPath != nullptr);
  REQUIRE(metricPath != nullptr);
  auto driver = std::make_unique<AirfoilDriver>(const_cast<char*>(configPath), 1, SU2_MPI::GetComm());
  auto& geometry = driver->Geometry();
  std::ifstream input(metricPath);
  REQUIRE(input.good());
  std::string header;
  std::getline(input, header);
  REQUIRE(header == "x,y,xx,xy,yy");
  // Test-only replicated lookup, outside measured remeshing and its memory
  // admission; the production backend still receives only owned sensor rows.
  std::map<std::array<double, 2>, Tensor> frozen;
  double x, y, xx, xy, yy;
  char comma;
  while (input >> x >> comma >> y >> comma >> xx >> comma >> xy >> comma >> yy) {
    REQUIRE(frozen.emplace(std::array<double, 2>{x, y}, Tensor{xx, xy, yy}).second);
  }
  REQUIRE(input.eof());
  su2activematrix metric(geometry.GetnPointDomain(), 3);
  size_t missing = 0;
  for (unsigned long p = 0; p < metric.rows(); ++p) {
    const auto coord = geometry.nodes->GetCoord(p);
    const auto found = frozen.find({SU2_TYPE::GetValue(coord[0]), SU2_TYPE::GetValue(coord[1])});
    if (found == frozen.end()) { ++missing; continue; }
    metric(p, 0) = found->second.xx;
    metric(p, 1) = found->second.xy;
    metric(p, 2) = found->second.yy;
  }
  REQUIRE(world.sum(missing) == 0);
  REQUIRE(world.sum(metric.rows()) == frozen.size());
  auto backend = driver->Backend();
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  const auto start = SU2_MPI::Wtime();
  auto result = backend->Remesh(driver->Config(), geometry, metric);
  const auto elapsed = SU2_MPI::Wtime() - start;
  const auto maximum = CPassiveComm::Allreduce(elapsed, CPassiveComm::Op::MAX);
  // Whole-process peak through remeshing, including driver/fixture setup;
  // artifact gathering below is excluded, as in the existing airfoil campaign.
  uint64_t hwmKiB = 0;
  std::ifstream processStatus("/proc/self/status");
  std::string statusLine;
  while (std::getline(processStatus, statusLine))
    if (statusLine.compare(0, 6, "VmHWM:") == 0) hwmKiB = std::stoull(statusLine.substr(6));
  REQUIRE(hwmKiB > 0);
  std::ofstream timing("native_frozen_timing_rank_" + std::to_string(world.rank) + ".csv");
  timing << std::setprecision(17) << "ranks,rank,owned_points,remesh_seconds,remesh_max_seconds,accepted,rss_hwm_kib\n"
         << world.size << ',' << world.rank << ',' << metric.rows() << ',' << elapsed << ',' << maximum << ','
         << (result.status == CRemeshResult::Status::COMPLETE) << ',' << hwmKiB << '\n';
  REQUIRE(timing.good());
  REQUIRE(result.status == CRemeshResult::Status::COMPLETE);
  const auto candidate = GatherCandidate(world, result.slices);
  if (world.rank == 0) simplex_test::WriteSU2Mesh(candidate, "native_frozen_adapted.su2");
}
