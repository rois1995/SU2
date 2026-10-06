/*!
 * \file CNativeScaling2D_tests.cpp
 * \brief Explicit engine scaling probe; incomplete adaptation is a measured outcome.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeEngine2D.hpp"
#include "../../../Common/include/adaptation/CNativeReference2D.hpp"
#include <cstdlib>
#include <fstream>
#include <iomanip>

using namespace SU2NativeBoundary2D;

TEST_CASE("Native engine size and partition envelope", "[NativeScaling2D][.]") {
  World world;
  auto parameter = [](const char* name, int fallback, int upper) {
    const auto value = std::getenv(name);
    if (!value) return fallback;
    char* end = nullptr;
    const auto number = std::strtol(value, &end, 10);
    REQUIRE(end != value);
    REQUIRE(*end == '\0');
    REQUIRE(number >= 1);
    REQUIRE(number <= upper);
    return int(number);
  };
  const int tiles = parameter("SU2_NATIVE_SCALING_TILES", 1, 4096);
  const int layout = parameter("SU2_NATIVE_SCALING_LAYOUT", 1, 3);
  const int anisotropy = parameter("SU2_NATIVE_SCALING_AR", 10, 1000);
  const int matched = parameter("SU2_NATIVE_SCALING_MATCHED", 1, 2);
  const int nx = 2 * tiles, ny = 4;
  const double normalSize = .04 / anisotropy;
  const double yScale = matched == 1 ? 10. / anisotropy : 1.;
  const double h0 = matched == 1 ? normalSize : .004;
  const double dy = .005 * yScale, yMax = ny * dy;
  const Tensor tensor{625., 0., 1 / (normalSize * normalSize)};
  auto node = [nx, ny, dy](int i, int j) {
    const bool onBoundary = i == 0 || i == nx || j == 0 || j == ny;
    const bool corner = (i == 0 || i == nx) && (j == 0 || j == ny);
    return Node{Id(j * (nx + 1) + i), {.02 * i, dy * j}, onBoundary ? (corner ? 1 | FEATURE : 1) : 0};
  };
  std::vector<Triangle> original;
  for (int j = 0; j < ny; ++j)
    for (int i = 0; i < nx; ++i) {
      original.push_back(triangle(node(i, j), node(i + 1, j), node(i + 1, j + 1)));
      original.push_back(triangle(node(i, j), node(i + 1, j + 1), node(i, j + 1)));
    }
  const auto perimeter = boundary(original);
  std::vector<PolylineReference::Face> faces;
  for (const auto& face : perimeter) {
    const auto a = face.second.first, b = face.second.second;
    const int marker = a.p.y == 0 && b.p.y == 0 ? 10 : a.p.y == yMax && b.p.y == yMax ? 12 : 11;
    faces.push_back({a, b, marker});
  }
  const PolylineReference reference(faces, 45);
  const auto policy = reference.Policy({{10, h0}, {12, h0}});
  auto ownerOf = [&](size_t index) {
    if (layout == 1) return int(index % world.size);
    if (layout == 2) return std::min(world.size - 1, int(index * world.size / original.size()));
    return std::min(world.size - 1, int((index / 2 % nx) * world.size / nx)); // Vertical geometric strips.
  };
  uint64_t inputCutEdges = 0;
  for (int j = 0; j < ny; ++j)
    for (int i = 0; i < nx; ++i) {
      const size_t cell = 2 * (j * nx + i);
      inputCutEdges += ownerOf(cell) != ownerOf(cell + 1);
      if (i + 1 < nx) inputCutEdges += ownerOf(cell) != ownerOf(cell + 3);
      if (j + 1 < ny) inputCutEdges += ownerOf(cell + 1) != ownerOf(cell + 2 * nx);
    }
  std::map<Id, Cell> local;
  for (size_t index = 0; index < original.size(); ++index) {
    const int owner = ownerOf(index);
    if (owner != world.rank) continue;
    Cell cell;
    cell.t = original[index];
    cell.t.id = index;
    for (auto& metric : cell.nodal_target) metric = tensor;
    for (int k = 0; k < 3; ++k) {
      const auto a = cell.t.v[k], b = cell.t.v[(k + 1) % 3];
      if (perimeter.count(edge(a.id, b.id))) cell.marker[k] = reference.ComponentOfOriginalFace(a.id, b.id);
    }
    local.emplace(index, cell);
  }
  EngineOptions options;
  options.geometry_tolerance = 1e-10;
  options.phase_rounds = parameter("SU2_NATIVE_SCALING_ROUNDS",
                                    std::max(300, int(original.size() / (4 * world.size))), 100000);
  options.sweeps = parameter("SU2_NATIVE_SCALING_SWEEPS", 6, 48);
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  const auto started = world.seconds();
  Engine engine(world, std::move(local), policy, options);
  const auto initialized = world.seconds();
  engine.adapt();
  const auto ended = world.seconds();
  const auto initSeconds = CPassiveComm::Allreduce(initialized - started, CPassiveComm::Op::MAX);
  const auto adaptSeconds = CPassiveComm::Allreduce(ended - initialized, CPassiveComm::Op::MAX);
  const auto traffic = CPassiveComm::Allreduce(uint64_t(world.bytes_sent), CPassiveComm::Op::SUM);
  const auto peakExchange = CPassiveComm::Allreduce(uint64_t(world.max_exchange_work_bytes), CPassiveComm::Op::MAX);
  const auto collectives = CPassiveComm::Allreduce(world.collective_calls, CPassiveComm::Op::MAX);
  const auto collectiveSeconds = CPassiveComm::Allreduce(world.collective_seconds, CPassiveComm::Op::MAX);
  const auto commits = world.sum(engine.stats.commits);
  const auto crossRank = world.sum(engine.stats.cross_rank);
  const auto conflicts = world.sum(engine.stats.conflicts);
  const auto memoryRejected = world.sum(engine.stats.memory_rejected);
  const auto maxPatch = CPassiveComm::Allreduce(uint64_t(engine.stats.max_patch), CPassiveComm::Op::MAX);
  const auto maxDonors = CPassiveComm::Allreduce(uint64_t(engine.stats.max_donors), CPassiveComm::Op::MAX);
  uint64_t scans = 0;
  double selection = 0;
  for (int k = 0; k < 8; ++k) { scans += engine.stats.selection_scans[k]; selection += engine.stats.choice_seconds[k]; }
  scans = CPassiveComm::Allreduce(scans, CPassiveComm::Op::SUM);
  selection = CPassiveComm::Allreduce(selection, CPassiveComm::Op::MAX);
  std::ifstream processStatus("/proc/self/status");
  std::string statusLine;
  uint64_t hwmKiB = 0;
  while (std::getline(processStatus, statusLine))
    if (statusLine.compare(0, 6, "VmHWM:") == 0) hwmKiB = std::stoull(statusLine.substr(6));
  hwmKiB = CPassiveComm::Allreduce(hwmKiB, CPassiveComm::Op::MAX);
  const auto maxOwned = CPassiveComm::Allreduce(uint64_t(engine.owned.size()), CPassiveComm::Op::MAX);
  // Per-rank rejection evidence is written outside all measured engine timing/traffic.
  std::ofstream rejectionFile("rejections_rank_" + std::to_string(world.rank) + ".csv");
  rejectionFile << "reason,count\n";
  for (const auto& rejection : engine.stats.rejected) rejectionFile << std::quoted(rejection.first) << ',' << rejection.second << '\n';
  REQUIRE(rejectionFile.good());
  std::vector<std::vector<Cell>> outgoing(world.size);
  for (const auto& cell : engine.owned) outgoing[0].push_back(cell.second);
  const auto output = world.exchange(outgoing); // Outside all timing/traffic metrics; test-only audit gather.
  if (world.rank == 0) {
    std::string reason;
    const auto fresh = triangles(output);
    REQUIRE(strict_cells(fresh, reason));
    // Physical sampling may change: verify area and every topological boundary label, not fixed edge IDs.
    long double oldArea = 0, newArea = 0;
    for (const auto& cell : original) oldArea += area(cell);
    for (const auto& cell : fresh) newArea += area(cell);
    CHECK(std::abs(newArea - oldArea) <= 1e-10L * oldArea);
    CHECK(physical(output).size() == boundary(fresh).size());
    const auto target = checked([tensor](Point) { return tensor; });
    const auto qmin = min_quality(fresh, target), lmax = max_length(fresh, target);
    uint64_t missedHeight = 0, missedGeometry = 0;
    double heightError = 0;
    for (const auto& cell : output)
      for (int k = 0; k < 3; ++k) if (cell.marker[k]) {
        const auto a = cell.t.v[k], b = cell.t.v[(k + 1) % 3];
        missedGeometry += policy.deviation({a, b, cell.marker[k]}) > options.geometry_tolerance;
        const auto h = policy.height(cell.marker[k]);
        if (h > 0) {
          const auto error = std::abs(double(2 * area(cell.t)) / norm(b.p - a.p) / h - 1);
          heightError = std::max(heightError, error);
          missedHeight += error > 1e-8;
        }
      }
    // Raw records permit a checker independent of the production quality/length routines.
    std::ofstream pointFile("points.csv"), cellFile("cells.csv"), faceFile("faces.csv");
    pointFile << std::setprecision(17) << "id,x,y\n";
    cellFile << "id,a,b,c\n";
    faceFile << "a,b,marker\n";
    for (const auto& point : nodes(fresh))
      pointFile << point.first << ',' << point.second.p.x << ',' << point.second.p.y << '\n';
    for (const auto& cell : output) {
      cellFile << cell.t.id << ',' << cell.t.v[0].id << ',' << cell.t.v[1].id << ',' << cell.t.v[2].id << '\n';
      for (int k = 0; k < 3; ++k) if (cell.marker[k])
        faceFile << cell.t.v[k].id << ',' << cell.t.v[(k + 1) % 3].id << ','
                 << reference.Marker(cell.marker[k]) << '\n';
    }
    REQUIRE(pointFile.good());
    REQUIRE(cellFile.good());
    REQUIRE(faceFile.good());
    const bool complete = qmin >= .18 && lmax <= 1.8 && !missedHeight && !missedGeometry;
    std::ofstream report("native_scaling.json");
    report << std::setprecision(17) << "{\"scope\":\"engine only; audit gather excluded\",\"tiles\":" << tiles
           << ",\"layout\":" << layout << ",\"matched\":" << matched << ",\"metric_height\":" << h0 / normalSize << ",\"anisotropy\":" << anisotropy << ",\"ranks\":" << world.size
           << ",\"h0\":" << h0 << ",\"xmax\":" << nx * .02 << ",\"ymax\":" << yMax << ",\"input_cut_edges\":" << inputCutEdges << ",\"input_cells\":" << original.size() << ",\"output_cells\":" << output.size()
           << ",\"complete\":" << (complete ? "true" : "false") << ",\"init_seconds\":" << initSeconds
           << ",\"adapt_seconds\":" << adaptSeconds << ",\"selection_seconds_max\":" << selection
           << ",\"collective_seconds_max\":" << collectiveSeconds << ",\"collective_calls_max\":" << collectives
           << ",\"traffic_bytes_sum\":" << traffic << ",\"exchange_work_bytes_max\":" << peakExchange
           << ",\"selection_scans_sum\":" << scans << ",\"commits\":" << commits << ",\"cross_rank\":" << crossRank
           << ",\"conflicts\":" << conflicts << ",\"memory_rejected\":" << memoryRejected
           << ",\"phase_rounds\":" << options.phase_rounds << ",\"sweeps\":" << options.sweeps
           << ",\"rounds\":" << engine.stats.rounds << ",\"max_patch\":" << maxPatch << ",\"max_donors\":" << maxDonors
           << ",\"rss_hwm_kib_max\":" << hwmKiB << ",\"max_owned\":" << maxOwned << ",\"qmin\":" << qmin << ",\"lmax\":" << lmax
           << ",\"height_error\":" << heightError << ",\"missed_height\":" << missedHeight
           << ",\"missed_geometry\":" << missedGeometry << "}\n";
    REQUIRE(report.good());
  }
}
