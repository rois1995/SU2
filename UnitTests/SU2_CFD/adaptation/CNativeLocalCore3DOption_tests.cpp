/*!
 * \file CNativeLocalCore3DOption_tests.cpp
 * \brief Config, publication and wall-height controls for the native local core.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {
using namespace SU2Native3D;
using namespace transfer_test;

struct CoreRows {
  size_t applicable = 0, skipped = 0;
  uint64_t changes = 0;
};

CoreRows ReadCoreRows(const std::string& log) {
  CoreRows result;
  std::istringstream lines(log);
  for (std::string line; std::getline(lines, line);) {
    if (line.rfind("NATIVE3D_LOCAL_CORE ", 0) != 0) continue;
    std::cout << line << std::endl;
    std::istringstream fields(line);
    for (std::string field; fields >> field;) {
      if (field == "applicable=1") ++result.applicable;
      if (field == "applicable=0") ++result.skipped;
      const auto separator = field.find('=');
      const auto name = field.substr(0, separator);
      if (name == "collapses" || name == "splits" || name == "edge_removals" || name == "face_swaps" || name == "moves")
        result.changes += std::stoull(field.substr(separator + 1));
    }
  }
  return result;
}

MetricComposition WallPolicy(const CConfig& config, const ReferenceState& reference, double, EngineOptions& options) {
  const auto& request = config.GetAdap_BL(0);
  const auto marker = std::find(reference.marker_names.begin(), reference.marker_names.end(), request.marker);
  if (marker == reference.marker_names.end()) throw std::logic_error("Missing fixture wall marker.");
  const auto height = SU2_TYPE::GetValue(request.firstHeight);
  options.wall_heights[static_cast<int>(marker - reference.marker_names.begin())] = height;
  options.edge_breakpoints = [](Point, Point) { return std::vector<double>{0, 1}; };
  // This wall control already has the requested first height and needs no refinement.
  return [height](Point, Tensor) {
    const auto scale = 1 / (height * height);
    return Tensor{scale, 0, 0, scale, 0, scale};
  };
}

struct RunResult {
  CRemeshResult mesh;
  std::string reference;
  CoreRows rows;
};

RunResult RunRemesh(const CConfig& config, const CGeometry& donor, const su2activematrix& sensor, bool wall,
                     const World& world) {
  auto reference = std::make_shared<ReferenceState>();
  CNativeRemesher3D::CompositionFactory policy;
  if (wall) policy = WallPolicy;
  CNativeRemesher3D remesher(reference, {}, std::move(policy));
  CRemeshResult result;
  std::ostringstream captured;
  {
    struct Capture {
      std::streambuf* previous;
      explicit Capture(std::ostream& sink) : previous(std::cout.rdbuf(sink.rdbuf())) {}
      ~Capture() { std::cout.rdbuf(previous); }
    } capture(captured);
    result = remesher.Remesh(config, donor, sensor);
  }
  const auto log = captured.str();
  const auto rows = ReadCoreRows(log);
  CAPTURE(world.rank, world.size, wall);
  INFO(log);
  CHECK(remesher.Statistics().workers == world.size);
  REQUIRE(result.status == CRemeshResult::Status::COMPLETE);
  REQUIRE(result.onAccepted);
  result.onAccepted();
  REQUIRE(reference->original);
  return {std::move(result), EncodeReference(*reference), rows};
}

bool SameBits(const std::vector<passivedouble>& a, const std::vector<passivedouble>& b) {
  return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(a.front())) == 0);
}

void CheckSameMesh(const RunResult& first, const RunResult& second) {
  const auto& a = first.mesh.slices;
  const auto& b = second.mesh.slices;
  CHECK(a.nDim == b.nDim);
  CHECK(a.nPointGlobal == b.nPointGlobal);
  CHECK(a.nElemGlobal == b.nElemGlobal);
  CHECK(a.firstPoint == b.firstPoint);
  CHECK(a.nPointLocal == b.nPointLocal);
  CHECK(a.nElemLocal == b.nElemLocal);
  CHECK(a.nMetric == b.nMetric);
  REQUIRE(a.coord.size() == b.coord.size());
  for (size_t d = 0; d < a.coord.size(); ++d) CHECK(SameBits(a.coord[d], b.coord[d]));
  CHECK(SameBits(a.metric, b.metric));
  CHECK(a.elemRows == b.elemRows);
  CHECK(a.boundaryRows == b.boundaryRows);
  CHECK(a.markerNames == b.markerNames);
  CHECK(a.markersWithElements == b.markersWithElements);
  CHECK(first.mesh.markers == second.mesh.markers);
  CHECK(first.reference == second.reference);
}
}  // namespace

TEST_CASE("Native local core option applies to the interior and skips requested wall heights",
          "[.][NativeLocalCore3DOption]") {
  World world;
  const bool wall = GENERATE(false, true);
  std::string options = "SOLVER= EULER\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\n"
                        "ADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
                        "ADAP_NATIVE_LOCAL_CORE= YES\nADAP_NATIVE_REPARTITION= NO\nWRT_PERFORMANCE= YES\n";
  if (wall) options += "ADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (.5)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.6)\n";
  auto config = MakeRoundConfig(options, "MARKER_EULER= (wall)\nMARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n");
  REQUIRE(config->GetAdap_Native_Local_Core());
  const auto mesh = simplex_test::MakeSimplexMesh(3, 2, [](const passivedouble* p) {
    return p[2] == 0 ? std::string("wall") : simplex_test::Marker3D(p);
  });
  MeshSolution donor(config.get(), mesh, 0);
  su2activematrix sensor(donor.Fine().GetnPointDomain(), 6);
  // Only cube body diagonals are long, with complete rings under smallest-point ownership.
  for (size_t p = 0; p < sensor.rows(); ++p)
    for (unsigned k = 0; k < 6; ++k) sensor(p, k) = (k == 0 || k == 3 || k == 5) ? 5.0625 : 0.;

  const auto first = RunRemesh(*config, donor.Fine(), sensor, wall, world);
  if (wall) {
    CHECK(first.rows.skipped == 1);
    CHECK(first.rows.applicable == 0);
    CHECK(first.rows.changes == 0);
    return;
  }
  const auto second = RunRemesh(*config, donor.Fine(), sensor, wall, world);
  for (const auto* rows : {&first.rows, &second.rows}) {
    CHECK(rows->applicable > 0);
    CHECK(rows->skipped == 0);
    CHECK(CPassiveComm::Allreduce(rows->changes, CPassiveComm::Op::SUM, world.comm) > 0);
  }
  CheckSameMesh(first, second);
}

TEST_CASE("Native local core is disabled by default", "[.][NativeLocalCore3DOption]") {
  const auto config = MakeConfig(3, "SOLVER= EULER\n");
  CHECK_FALSE(config->GetAdap_Native_Local_Core());
}
