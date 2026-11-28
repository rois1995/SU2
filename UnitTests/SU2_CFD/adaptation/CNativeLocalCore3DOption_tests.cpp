/*!
 * \file CNativeLocalCore3DOption_tests.cpp
 * \brief Config, publication and wall-height controls for the native local core.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher.hpp"
#include "../../../Common/include/adaptation/CNativeMarkerTolerance3D.hpp"

#include <algorithm>
#include <cstdlib>
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

namespace {
void CheckConstructionCollapseTrigger(bool mapped) {
  World world;
  REQUIRE(world.size == 1);
  const auto config = MakeRoundConfig(
      "SOLVER= EULER\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\n"
      "ADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\nADAP_NATIVE_LOCAL_CORE= YES\nADAP_NATIVE_LOCAL_SURFACE= YES\n"
      "ADAP_NATIVE_REPARTITION= NO\nWRT_PERFORMANCE= YES\n",
      "MARKER_EULER= (wall)\nMARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n");
  const auto mesh = simplex_test::MakeSimplexMesh(3, 1, [](const passivedouble* p) {
    return p[2] == 0 ? std::string("wall") : simplex_test::Marker3D(p);
  });
  MeshSolution donor(config.get(), mesh, 0);
  su2activematrix sensor(donor.Fine().GetnPointDomain(), 6);
  // The vertical edges are short, but every surface vertex is a feature corner.
  for (size_t p = 0; p < sensor.rows(); ++p) {
    for (unsigned k = 0; k < 6; ++k) sensor(p, k) = k == 0 || k == 3 ? 1. : k == 5 ? .49 : 0.;
  }
  auto reference = std::make_shared<ReferenceState>();
  if (mapped) {
    ImportGeometry(*config, donor.Fine(), &sensor, *reference);
    using SU2NativeGeometry::MappedFacetReference;
    MappedFacetReference::CurvedNets nets;
    std::map<Id, double> errors;
    constexpr unsigned powers[10][3] = {{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},
                                        {0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
    for (const auto& face : reference->facets) {
      errors.emplace(face.id, 1e-12);
      auto& net = nets[face.id];
      for (unsigned q = 0; q < 10; ++q)
        for (unsigned k = 0; k < 3; ++k) {
          net[q].x += powers[q][k] * face.v[k].p.x / 3;
          net[q].y += powers[q][k] * face.v[k].p.y / 3;
          net[q].z += powers[q][k] * face.v[k].p.z / 3;
        }
    }
    auto authority = std::make_shared<MappedFacetReference>(reference->original, errors, 303, SIZE_MAX, false, nets, 0);
    BoundaryPoints points;
    std::vector<SurfaceFace> skin;
    for (const auto& face : reference->facets) {
      skin.push_back({{face.v[0].id, face.v[1].id, face.v[2].id}, face.id});
      for (const auto& node : face.v) points.emplace(node.id, BoundaryPoint{node, authority->Vertex(node.id)});
    }
    reference->original = PlanarReference::WithParameters(
        reference->original, std::make_shared<BoundaryParameterState>(authority, points, skin, 1));
  }
  bool initial_exit = false;
  EngineOptions options;
  options.stage_observer = [&](const StageProgressView& view) {
    if (initial_exit || std::string(view.stage) != "length_collapse_exit") return;
    initial_exit = true;
    if (mapped) {
      CHECK(view.round == 0);
      CHECK(view.stats.attempted == 0);
      CHECK(view.stats.collapse_filtered == 0);
    } else {
      CHECK(view.stats.attempted + view.stats.collapse_filtered > 0);
    }
  };
  CNativeRemesher3D remesher(reference, options);
  std::ostringstream captured;
  struct Capture {
    std::streambuf* previous;
    explicit Capture(std::ostream& sink) : previous(std::cout.rdbuf(sink.rdbuf())) {}
    ~Capture() { std::cout.rdbuf(previous); }
  };
  CRemeshResult result;
  {
    const Capture capture(captured);
    result = remesher.Remesh(*config, donor.Fine(), sensor);
  }
  const auto log = captured.str();
  INFO(log);
  REQUIRE(result.status == CRemeshResult::Status::COMPLETE);
  CHECK(initial_exit);
  CHECK(log.find("NATIVE3D_LOCAL_CORE rank=0 stage=0 pass=1 applicable=1") != std::string::npos);
  if (mapped) {
    CHECK(log.find("NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=0 removals_all=0") != std::string::npos);
  } else {
    CHECK(log.find("NATIVE3D_LOCAL_SURFACE rank=0 applicable=0") != std::string::npos);
  }
}
}  // namespace

TEST_CASE("Step C with zero edits closes construction collapse", "[.][FP3A]") {
  CheckConstructionCollapseTrigger(true);
}

TEST_CASE("Volume-only core leaves construction collapse available", "[.][FP3A]") {
  CheckConstructionCollapseTrigger(false);
}

namespace {
const std::string nativeToleranceOptions =
    "SOLVER= EULER\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\n"
    "ADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n";
const std::string allNativeTolerances =
    "ADAP_HAUSD= (x_minus, 1e-4, x_plus, 1e-4, y_minus, 1e-4, y_plus, 1e-4, "
    "z_plus, 1e-4, z_minus_a, 1e-4, z_minus_b, 1e-4)\n";

void MapToleranceReference(ReferenceState& state) {
  using SU2NativeGeometry::MappedFacetReference;
  MappedFacetReference::CurvedNets nets;
  std::map<Id, double> errors;
  constexpr unsigned powers[10][3] = {{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},
                                      {0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
  for (const auto& face : state.facets) {
    errors.emplace(face.id, 1e-12);
    auto& net = nets[face.id];
    for (unsigned q = 0; q < 10; ++q)
      for (unsigned k = 0; k < 3; ++k) {
        net[q].x += powers[q][k] * face.v[k].p.x / 3;
        net[q].y += powers[q][k] * face.v[k].p.y / 3;
        net[q].z += powers[q][k] * face.v[k].p.z / 3;
      }
  }
  auto authority = std::make_shared<MappedFacetReference>(state.original, errors, 304, SIZE_MAX, false, nets, 0);
  BoundaryPoints points;
  std::vector<SurfaceFace> skin;
  for (const auto& face : state.facets) {
    skin.push_back({{face.v[0].id, face.v[1].id, face.v[2].id}, face.id});
    for (const auto& node : face.v) points.emplace(node.id, BoundaryPoint{node, authority->Vertex(node.id)});
  }
  state.original = PlanarReference::WithParameters(
      state.original, std::make_shared<BoundaryParameterState>(authority, points, skin, 1));
}
}  // namespace

TEST_CASE("TOL1 projected marker inventory keeps declared absent values and canonical identity", "[.][TOL1]") {
  const MarkerTolerance3D::Configured configured{{"z", 1.}, {"y", 1e-4}, {"absent", 1e-12}};
  const MarkerTolerance3D table(configured, {"z", "y"}, 1.);
  CHECK(table.Strict() == 1e-12);
  CHECK(table.Marker(0) == 1.);
  CHECK(table.Marker(1) == 1e-4);
  CHECK(table.Class(0) == 2);
  CHECK(table.Class(1) == 1);
  CHECK(MarkerTolerance3D::Identity({}) == 0);
  auto different = configured;
  different["z"] = 2.;
  CHECK(MarkerTolerance3D::Identity(configured) != MarkerTolerance3D::Identity(different));
}

TEST_CASE("TOL1 fatal native geometry and parent agreement subprocess", "[.][tol1_fatal]") {
  const auto* name = std::getenv("SU2_TOL1_FATAL_CASE");
  REQUIRE(name != nullptr);
  const std::string which(name);
  const World world;
  if (which == "tiny_floor") {
    try {
      const MarkerTolerance3D table({{"x_minus", 2e-310}}, {"x_minus"}, 1e-298);
    } catch (const std::invalid_argument& error) {
      SU2_MPI::Error(error.what(), CURRENT_FUNCTION);
    }
    FAIL("The tiny-coordinate setup passed its tolerance floor.");
  }
  if (which == "native2d") {
    const auto config = MakeConfig(2, nativeToleranceOptions + "ADAP_HAUSD= (left, 1e-4)\n");
    MeshSolution donor(config.get(), BoxMesh(2, 1, false), 0);
    CNativeRemesher::CheckSupport(*config, donor.Fine());
    FAIL("The native 2D list passed support admission.");
  }

  auto options = allNativeTolerances;
  if (which == "missing") options = "ADAP_HAUSD= (x_plus, 1e-4)\n";
  if (which == "unit_floor")
    options.replace(options.find("x_minus, 1e-4"), std::string("x_minus, 1e-4").size(),
                    "x_minus, 9.9999999999999978e-13");
  if (which == "mpi_value") {
    REQUIRE(world.size == 2);
    options.replace(options.find("x_minus, 1e-4"), std::string("x_minus, 1e-4").size(),
                    world.rank == 0 ? "x_minus, 1" : "x_minus, 2");
  }
  if (which == "mpi_form") {
    REQUIRE(world.size == 2);
    if (world.rank == 0) options = "ADAP_HAUSD= 1E-4\n";
  }
  const auto config = MakeConfig(3, nativeToleranceOptions + options);
  MeshSolution donor(config.get(), BoxMesh(3, 1, false), 0);
  auto reference = std::make_shared<ReferenceState>();
  CNativeRemesher3D remesher(reference);
  if (which == "mpi_value" || which == "mpi_form") {
    const su2activematrix sensor;
    (void)remesher.Remesh(*config, donor.Fine(), sensor);
  } else {
    ImportGeometry(*config, donor.Fine(), nullptr, *reference);
    if (which == "missing" || which == "unit_floor") MapToleranceReference(*reference);
    remesher.PrepareReference(*config, donor.Fine());
  }
  FAIL("The requested native tolerance error returned instead of aborting.");
}
