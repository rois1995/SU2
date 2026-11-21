/*!
 * \file CNativeLocalCoreLab_tests.cpp
 * \brief One-rank local-core measurements on the frozen ONERA M6 inputs.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../Common/include/adaptation/CNativeLocalSurface3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeRuntimeProfile3D.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "fixtures/native_fast_path_stage2_setup.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;
using Clock = std::chrono::steady_clock;

double Seconds(Clock::time_point from) { return std::chrono::duration<double>(Clock::now() - from).count(); }

class LabDriver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::CSinglezoneDriver;
  CConfig& Config() { return *config_container[ZONE_0]; }
  CGeometry& Geometry() { return *geometry_container[ZONE_0][INST_0][MESH_0]; }
};

su2activematrix ReadFrozenSensor(CGeometry& geometry, const char* path) {
  std::ifstream input(path, std::ios::binary);
  uint64_t count = 0;
  input.read(reinterpret_cast<char*>(&count), sizeof(count));
  REQUIRE(bool(input));
  REQUIRE(count == geometry.GetnPointDomain());
  std::vector<std::array<double, 12>> rows(count);
  for (uint64_t i = 0; i < count; ++i) {
    uint64_t id = count;
    input.read(reinterpret_cast<char*>(&id), sizeof(id));
    input.read(reinterpret_cast<char*>(rows[i].data()), sizeof(rows[i]));
    if (!input || id != i) throw std::runtime_error("Invalid lab frozen sensor record.");
  }
  REQUIRE(input.peek() == std::char_traits<char>::eof());
  constexpr std::array<size_t, 6> upper{0, 1, 2, 4, 5, 8};
  su2activematrix metric(count, upper.size());
  for (size_t i = 0; i < count; ++i) {
    const auto& row = rows.at(geometry.nodes->GetGlobalIndex(i));
    for (size_t d = 0; d < 3; ++d)
      if (row[d] != SU2_TYPE::GetValue(geometry.nodes->GetCoord(i, d)))
        throw std::runtime_error("Lab frozen coordinate differs from the CFD point.");
    for (size_t k = 0; k < upper.size(); ++k) metric(i, k) = row[3 + upper[k]];
  }
  return metric;
}

struct LabInput {
  Owned cells;
  size_t ceiling;
  Id point_base, cell_base;
};

LabInput LoadMesh(ReferenceState* kept = nullptr) {
  REQUIRE(SU2_MPI::GetSize() == 1);
  const char* config = std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  const char* sensor = std::getenv("SU2_NATIVE_FROZEN_SENSOR3D");
  REQUIRE(config);
  REQUIRE(sensor);
  std::string config_path(config);
  LabDriver driver(config_path.data(), 1, SU2_MPI::GetComm());
  auto& geometry = driver.Geometry();
  auto metric = ReadFrozenSensor(geometry, sensor);
  ImportedGeometry imported;
  ReferenceState reference;
  std::string reason;
  bool loaded = false;
  size_t reference_bytes = 0;
  const auto ceiling = driver.Config().GetAdap_Native_MemoryLimit();
  const auto path = driver.Config().GetMesh_FileName() + ".native_ref";
  const auto restored = LoadReference(path, reference, loaded, reason, ceiling, 0, reference_bytes);
  INFO(reason);
  REQUIRE(restored);
  REQUIRE(loaded);
  REQUIRE(reference.original);
  BridgeStats stats;
  const auto valid = ImportGeometry(driver.Config(), geometry, &metric, reference, imported, reason, stats, ceiling);
  INFO(reason);
  REQUIRE(valid);
  REQUIRE_FALSE(imported.cells.empty());

  Id point_base = 0, cell_base = 0;
  const auto visit = [&](const Cell& cell) {
    cell_base = std::max(cell_base, cell.id);
    for (const auto& v : cell.v) point_base = std::max(point_base, v.id);
  };
  for (const auto& row : imported.cells) visit(row.second.cell);
  for (const auto& donor : imported.original) visit(donor.cell);
  driver.Finalize();
  if (kept) *kept = std::move(reference);
  return {std::move(imported.cells), ceiling, point_base, cell_base};
}

local::CoreOptions Variant(const std::string& variant) {
  local::CoreOptions options;
  if (variant == "V0") {
    options.shape_test = options.float_trials = options.surface_first = options.work_list = false;
    options.split_progress = false;
  } else if (variant == "V1") {
    options.surface_first = false;
    options.split_progress = false;
  } else if (variant == "V5") {
    options.surface_first = false;
  } else if (variant == "V3") {
    options.short_edge = local::ShortEdge / std::sqrt(2.);
  } else {
    REQUIRE(variant == "V2");
  }
  return options;
}

void ReportMesh(const char* state, const local::Mesh& mesh, double short_edge) {
  size_t points = 0, cells = 0, bad = 0, short_edges = 0, short_edges_09 = 0, long_edges = 0;
  for (const auto& point : mesh.Points()) points += !point.removed;
  for (const auto& cell : mesh.Cells()) {
    if (!cell.live) continue;
    ++cells;
    bad += mesh.Margin(cell.vertices) < local::BadMargin;
  }
  for (const auto& use : mesh.Edges()) {
    const auto length = mesh.Length(use.edge);
    short_edges += length < short_edge;
    short_edges_09 += length < local::ShortEdge;
    long_edges += length > local::LongEdge;
  }
  std::cout << "FP1_LAB " << state << " cells=" << cells << " points=" << points << " bad_cells=" << bad
            << " short_edges=" << short_edges << " short_edges_09=" << short_edges_09 << " long_edges=" << long_edges
            << " short_edge=" << short_edge << std::endl;
}

void ReportExact(const Owned& owned) {
  size_t nonpositive = 0;
  Id first = 0;
  for (const auto& entry : owned) {
    const auto& v = entry.second.cell.v;
    if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0) continue;
    if (nonpositive++ == 0) first = entry.first;
  }
  std::cout << "FP1_LAB exact cells=" << owned.size() << " nonpositive=" << nonpositive
            << " first_nonpositive=" << first << std::endl;
  REQUIRE(nonpositive == 0);
}

void ReportStrict(const Owned& owned) {
  const auto start = Clock::now();
  size_t measured = 0, q_bad = 0, j_bad = 0, long_cells = 0;
  for (auto it = owned.begin(); it != owned.end();) {
    std::vector<Cell> batch;
    std::vector<DonorCell> donors;
    batch.reserve(MaximumReplacementCells);
    donors.reserve(MaximumReplacementCells);
    while (it != owned.end() && batch.size() < MaximumReplacementCells) {
      const auto& row = it++->second;
      batch.push_back(row.cell);
      donors.push_back({row.cell, row.sensor});
    }
    FrozenField field(std::move(donors), {}, false, {}, {}, {}, true, MaximumDonors, true);
    const auto result = MeasureMetricCells(batch, field);
    measured += result.shape_cells;
    q_bad += result.q_bad_cells;
    j_bad += result.j_bad_cells;
    for (const auto& cell : batch) {
      bool has_long = false;
      for (size_t a = 0; a < 4; ++a)
        for (size_t b = a + 1; b < 4; ++b) {
          const auto u = cell.v[a].id, v = cell.v[b].id;
          has_long |= result.edges.at(Edge{std::min(u, v), std::max(u, v)}).length > MaximumMetricEdge;
        }
      long_cells += has_long;
    }
  }
  std::cout << "FP1_LAB strict cells=" << measured << " q_bad=" << q_bad << " j_bad=" << j_bad
            << " long_cells=" << long_cells << " seconds=" << Seconds(start) << std::endl;
  REQUIRE(measured == owned.size());
}

struct LabEntityCensus {
  size_t faces = 0, above = 0;
};

LabEntityCensus ClassifyLabSurface(const BoundaryParameterState& state, SU2NativeGeometry::AtlasBudget& budget) {
  LabEntityCensus census;
  for (const auto& [key, face] : state.Surface()) {
    (void)key;
    std::array<SU2NativeGeometry::SurfaceBinding, 3> bindings;
    std::array<Point, 3> points;
    for (size_t k = 0; k < 3; ++k) {
      const auto& point = state.Records().at(face.v[k]).point;
      bindings[k] = point.binding;
      points[k] = point.node.p;
    }
    const double bound = state.EntityFaceError(face.facet, bindings, points, &budget,
                                               SU2NativeGeometry::SurfaceQueryNeed::Classify());
    census.above += bound > state.GeometryTolerance();
    ++census.faces;
  }
  return census;
}

void ProveLabSurface(const BoundaryParameterState& state, size_t cells, size_t ceiling, size_t caller,
                     Clock::time_point deadline_start) {
  auto budget = native_fp2_fixture::Budget(cells, caller, deadline_start, 5400, ceiling);
  std::vector<SurfaceFace> faces;
  faces.reserve(state.Surface().size());
  for (const auto& row : state.Surface()) faces.push_back(row.second);
  std::set<BoundaryParameterState::EntityKey> keys;
  for (const auto& row : state.Geometry().Reference().Facets()) keys.insert(state.Entity(row.first));

  const auto reverse_start = Clock::now();
  const auto reverse = state.OwnedFaceReverseBounds(faces, budget);
  const auto reverse_seconds = Seconds(reverse_start);
  const auto forward_start = Clock::now();
  const auto& forward = state.GeometryCertificates(budget, false);
  const auto forward_seconds = Seconds(forward_start);
  REQUIRE(reverse.size() == keys.size());
  REQUIRE(forward.size() == keys.size());
  REQUIRE_FALSE(keys.empty());
  bool accepted = state.ReverseProofComplete(reverse);
  const auto tolerance = state.GeometryTolerance();
  size_t i = 0;
  for (const auto& key : keys) {
    const auto& [body, sheet, marker] = key;
    const auto mesh_to_model = reverse[i++];
    const auto& certificate = forward.at(key);
    const auto model_to_mesh = certificate.model_to_mesh;
    const bool valid = !certificate.outside_tolerance && mesh_to_model >= 0 && mesh_to_model <= tolerance &&
                       model_to_mesh >= 0 && model_to_mesh <= tolerance;
    std::cout << "FP2_A LAB_S_ENTITY body=" << body << " sheet=" << sheet << " marker=" << marker
              << " mesh_to_model=" << mesh_to_model << " model_to_mesh=" << model_to_mesh
              << " accepted=" << valid << std::endl;
    accepted &= valid;
  }
  const auto classify_start = Clock::now();
  const auto census = ClassifyLabSurface(state, budget);
  std::cout << "FP2_A LAB_S_PROOF entities=" << keys.size() << " classified=" << census.faces
            << " classify_above_tol=" << census.above << " accepted=" << accepted
            << " seconds_mesh_to_model=" << reverse_seconds << " seconds_model_to_mesh=" << forward_seconds
            << " seconds_classify=" << Seconds(classify_start) << " work=" << budget.work
            << " modeled_peak=" << budget.peak_bytes << std::endl;
  REQUIRE(census.faces == state.Surface().size());
  REQUIRE(accepted);
}
}  // namespace

TEST_CASE("FP1 local-core lab on frozen M6 inputs", "[.][NativeLocalCoreLab]") {
  const char* variant = std::getenv("SU2_NATIVE_LOCAL_LAB_VARIANT");
  REQUIRE(variant);
  const auto options = Variant(variant);
  const auto loading = Clock::now();
  auto input = LoadMesh();
  std::cout << std::setprecision(17) << "FP1_LAB load variant=" << variant << " seconds=" << Seconds(loading)
            << std::endl;
  const auto bytes = transfer_memory::Add(OwnedStorageBytes(input.cells), local::Mesh::ArrayBytes(input.cells.size()));
  REQUIRE(bytes <= input.ceiling);
  const auto cell_limit = input.ceiling / (sizeof(Owned::value_type) + 4 * sizeof(void*) + local::Mesh::BytesPerCell);
  ReportMesh("before", local::Mesh(input.cells, false, options), options.short_edge);
  local::IdSequence point_ids(input.point_base), cell_ids(input.cell_base);
  local::PassStats stats;
  stats.report = true;
  stats.sink = [](const local::SweepRow& row) {
    std::cout << "NATIVE3D_LOCAL_SWEEP rank=0 stage=0 pass=1";
    local::PrintSweepFields(std::cout, row);
    std::cout << std::endl;
  };

  const auto start = Clock::now();
  local::Mesh mesh(input.cells, false, options);
  const auto build_seconds = Seconds(start);
  stats = mesh.RunPass(point_ids, cell_limit, std::move(stats), options);
  const auto write = Clock::now();
  mesh.WriteBack(input.cells, cell_ids);
  const auto pass_seconds = Seconds(start);
  const auto write_seconds = Seconds(write);
  std::cout << "FP1_LAB pass variant=" << variant << " seconds=" << pass_seconds << " self_check=1 cells_in="
            << stats.cells_in << " cells_out=" << stats.cells_out << " points_in=" << stats.points_in
            << " points_out=" << stats.points_out << " collapses=" << stats.collapses << " splits=" << stats.splits
            << " edge_removals=" << stats.edge_removals << " face_swaps=" << stats.face_swaps << " moves=" << stats.moves
            << " deferred=" << stats.deferred << " skipped_memory=" << stats.skipped_memory << " blocked=" << stats.blocked
            << " refused_valid=" << stats.refused_valid << " refused_exact=" << stats.refused_exact
            << " refused_size=" << stats.refused_size << " refused_topology=" << stats.refused_topology
            << " refused_memory=" << stats.refused_memory << " refused_numeric=" << stats.refused_numeric
            << " refused_shape=" << stats.refused_shape << " evaluated_collapse=" << stats.evaluated_collapse
            << " evaluated_split=" << stats.evaluated_split << " evaluated_quality=" << stats.evaluated_quality
            << " sweeps_collapse=" << stats.sweeps_collapse << " sweeps_split=" << stats.sweeps_split
            << " sweeps_quality=" << stats.sweeps_quality << " rounds=" << stats.rounds << " limit=" << stats.limit
            << " round_changes=" << stats.round_changes << " quality_changes=" << stats.quality_changes
            << " seconds_build=" << build_seconds << " seconds_collapse=" << stats.seconds_collapse
            << " seconds_split=" << stats.seconds_split << " seconds_quality=" << stats.seconds_quality
            << " seconds_check=" << stats.seconds_check << " seconds_write=" << write_seconds
            << " memory_admission_bytes=" << bytes << " cell_limit=" << cell_limit << std::endl;
  ReportExact(input.cells);
  ReportMesh("after", mesh, options.short_edge);
  ReportStrict(input.cells);
}

TEST_CASE("FP2-A LAB-S on the frozen M6 event 1 volume", "[.][NativeLocalCoreSurfaceLab]") {
  const auto start = Clock::now();
  ReferenceState reference;
  auto input = LoadMesh(&reference);
  const auto load_seconds = Seconds(start);
  REQUIRE(input.cells.size() == 303359);
  REQUIRE((reference.marker_names == std::vector<std::string>{"FARFIELD", "SYMMETRY", "WING3D"}));
  REQUIRE(reference.original->Parameters()->Mobility().identity == 0);
  REQUIRE_FALSE(reference.original->Parameters()->EntityGeometry());
  const auto setup_start = Clock::now();
  const auto owned_bytes = OwnedStorageBytes(input.cells);
  native_fp2_fixture::PrepareIN1(reference, input.ceiling, owned_bytes);
  const auto setup_seconds = Seconds(setup_start);
  const auto* before = reference.original->Parameters();
  REQUIRE(before->Surface().size() == 48954);
  REQUIRE(before->Records().size() == 24479);

  const auto reference_bytes = reference.original->ResidentBytes();
  const auto main_caller = transfer_memory::Add(reference_bytes, owned_bytes);
  const auto mesh_bytes = local::Mesh::ArrayBytes(input.cells.size());
  const auto geometry_caller = transfer_memory::Add(main_caller, mesh_bytes);
  auto budget = native_fp2_fixture::Budget(input.cells.size(), geometry_caller, start, 5400, input.ceiling);
  local::StateSurfaceGeometry surface(reference.original, std::move(budget));
  const auto baseline = transfer_memory::Add(reference_bytes, surface.ResidentBytes());
  constexpr auto owned_row_bytes = sizeof(Owned::value_type) + 4 * sizeof(void*);
  World world;
  local::MemoryBudget memory{input.ceiling, reference_bytes, owned_row_bytes};
  memory.Require(world, transfer_memory::Add(baseline, owned_bytes, mesh_bytes), "FP2-A LAB-S initial core admission.");
  const auto cell_limit = (input.ceiling - baseline) / (owned_row_bytes + local::Mesh::BytesPerCell);
  auto options = Variant("V2");
  options.surface_splits = true;
  local::IdSequence point_ids(input.point_base), cell_ids(input.cell_base);
  local::PassStats stats;
  double build_seconds = 0, pass_seconds = 0, write_seconds = 0;
  {
    const auto build_start = Clock::now();
    local::Mesh mesh(input.cells, false, options);
    build_seconds = Seconds(build_start);
    const auto pass_start = Clock::now();
    stats = mesh.RunPass(point_ids, cell_limit, true, options, &surface);
    pass_seconds = Seconds(pass_start);
    std::cout << std::setprecision(17) << "NATIVE3D_LOCAL_SURFACE rank=0 stage=0 pass=1 applicable=1";
    local::PrintSurfaceFields(std::cout, stats.surface);
    std::cout << std::endl;
    const auto write_start = Clock::now();
    mesh.WriteBack(input.cells, cell_ids);
    write_seconds = Seconds(write_start);
  }
  std::cout << "FP2_A LAB_S cells_in=" << stats.cells_in << " cells_out=" << stats.cells_out
            << " bad_in=" << stats.bad_in << " bad_out=" << stats.bad_out << " seconds_load=" << load_seconds
            << " seconds_setup=" << setup_seconds << " seconds_build=" << build_seconds
            << " seconds_pass=" << pass_seconds << " seconds_write=" << write_seconds << std::endl;
  const auto exact_start = Clock::now();
  ReportExact(input.cells);
  const auto exact_seconds = Seconds(exact_start);
  const auto caller = transfer_memory::Add(reference_bytes, OwnedStorageBytes(input.cells));
  const auto rss_before = CurrentRSS3D();
  const auto result = local::BuildSurfaceState(world, input.cells, surface, memory, caller);
  const auto rss_after = CurrentRSS3D();
  REQUIRE(result.state);
  REQUIRE(result.splits > 0);
  REQUIRE(result.state->Revision() == before->Revision() + 1);
  REQUIRE(result.state->Records().size() == before->Records().size() + result.splits);
  REQUIRE(result.state->Surface().size() == before->Surface().size() + 2 * result.splits);
  std::cout << "NATIVE3D_LOCAL_SURFACE_STATE rank=0 stage=0 splits_all=" << result.splits
            << " faces=" << result.state->Surface().size() << " points=" << result.state->Records().size()
            << " traces=" << result.state->Coverage().traces.size() << " revision=" << result.state->Revision()
            << " seconds_exchange=0 seconds_gather=" << result.seconds_gather
            << " seconds_build=" << result.seconds_build << " seconds_check=" << result.seconds_check
            << " modeled_peak=" << result.modeled_peak << " rss_before=" << rss_before << " rss_after=" << rss_after
            << std::endl;
  std::cout << "FP2_A LAB_S_EXACT seconds=" << exact_seconds << std::endl;
  const auto proof_live =
      transfer_memory::Add(caller, surface.ResidentBytes(), result.state->ResidentBytes(),
                           transfer_memory::Mul(result.state->Surface().size(), sizeof(SurfaceFace)));
  ProveLabSurface(*result.state, stats.cells_in, input.ceiling, proof_live, start);
}
