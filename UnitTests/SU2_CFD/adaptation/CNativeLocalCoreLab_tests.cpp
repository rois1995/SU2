/*!
 * \file CNativeLocalCoreLab_tests.cpp
 * \brief One-rank local-core measurements on the frozen ONERA M6 inputs.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../Common/adaptation/native_local_fixture_access.hpp"
#include "../../Common/adaptation/native_local_surface_probe.hpp"
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
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
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

template <class Action>
void CheckLabStep(const World& world, Action action) {
  CLocalFailure failure;
  try {
    action();
  } catch (const std::exception& error) {
    failure.Set(2, 0, error.what());
  }
  const auto elected = world.elect(failure);
  INFO(elected.message);
  REQUIRE_FALSE(elected.any);
}

su2activematrix ReadFrozenSensor(CGeometry& geometry, const char* path) {
  std::ifstream input(path, std::ios::binary);
  uint64_t count = 0;
  input.read(reinterpret_cast<char*>(&count), sizeof(count));
  if (!input || count != geometry.GetGlobal_nPointDomain())
    throw std::runtime_error("Frozen lab sensor size differs from the global CFD point count.");
  std::vector<std::array<double, 12>> rows(count);
  for (uint64_t i = 0; i < count; ++i) {
    uint64_t id = count;
    input.read(reinterpret_cast<char*>(&id), sizeof(id));
    input.read(reinterpret_cast<char*>(rows[i].data()), sizeof(rows[i]));
    if (!input || id != i) throw std::runtime_error("Invalid lab frozen sensor record.");
  }
  if (input.peek() != std::char_traits<char>::eof()) throw std::runtime_error("Trailing data in the frozen lab sensor.");
  constexpr std::array<size_t, 6> upper{0, 1, 2, 4, 5, 8};
  const auto owned = geometry.GetnPointDomain();
  su2activematrix metric(owned, upper.size());
  for (size_t i = 0; i < owned; ++i) {
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

LabInput LoadMesh(ReferenceState* kept = nullptr, bool parallel = false) {
  const auto ranks = SU2_MPI::GetSize();
  REQUIRE((parallel ? (ranks == 2 || ranks == 6) : ranks == 1));
  const char* config = std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  const char* sensor = std::getenv("SU2_NATIVE_FROZEN_SENSOR3D");
  REQUIRE(config);
  REQUIRE(sensor);
  std::string config_path(config);
  LabDriver driver(config_path.data(), 1, SU2_MPI::GetComm());
  auto& geometry = driver.Geometry();
  su2activematrix metric;
  if (parallel) {
    const World world;
    CheckLabStep(world, [&] { metric = ReadFrozenSensor(geometry, sensor); });
  } else {
    metric = ReadFrozenSensor(geometry, sensor);
  }
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

struct ExactCounts {
  size_t nonpositive = 0;
  Id first = 0;
};

ExactCounts CountExact(const Owned& owned) {
  ExactCounts result;
  for (const auto& entry : owned) {
    const auto& v = entry.second.cell.v;
    if (Orientation(v[0].p, v[1].p, v[2].p, v[3].p) > 0) continue;
    if (result.nonpositive++ == 0) result.first = entry.first;
  }
  return result;
}

void ReportExact(const Owned& owned) {
  const auto exact = CountExact(owned);
  std::cout << "FP1_LAB exact cells=" << owned.size() << " nonpositive=" << exact.nonpositive
            << " first_nonpositive=" << exact.first << std::endl;
  REQUIRE(exact.nonpositive == 0);
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

using SurfaceReasons = std::map<Face, local::SurfaceRemaining>;

struct LabEntityCensus {
  size_t faces = 0, above = 0, left_refused = 0, left_short = 0, left_border = 0, left_limit = 0;
  size_t left_neighbor_locked = 0, unexplained = 0;
};

LabEntityCensus ClassifyLabSurface(const BoundaryParameterState& state, SU2NativeGeometry::AtlasBudget& budget,
                                   const SurfaceReasons& remaining) {
  LabEntityCensus census;
  for (const auto& [key, face] : state.Surface()) {
    std::array<SU2NativeGeometry::SurfaceBinding, 3> bindings;
    std::array<Point, 3> points;
    for (size_t k = 0; k < 3; ++k) {
      const auto& point = state.Records().at(face.v[k]).point;
      bindings[k] = point.binding;
      points[k] = point.node.p;
    }
    const double bound = state.EntityFaceError(face.facet, bindings, points, &budget,
                                               SU2NativeGeometry::SurfaceQueryNeed::Classify());
    ++census.faces;
    if (!(bound > state.GeometryTolerance())) continue;
    ++census.above;
    const auto reason = remaining.find(key);
    if (reason == remaining.end()) {
      ++census.unexplained;
      continue;
    }
    switch (reason->second) {
      case local::SurfaceRemaining::Refused: ++census.left_refused; break;
      case local::SurfaceRemaining::Short: ++census.left_short; break;
      case local::SurfaceRemaining::Border: ++census.left_border; break;
      case local::SurfaceRemaining::Limit: ++census.left_limit; break;
      case local::SurfaceRemaining::NeighborLocked: ++census.left_neighbor_locked; break;
    }
  }
  return census;
}

void ProveLabSurface(const BoundaryParameterState& state, size_t cells, size_t ceiling, size_t caller,
                     Clock::time_point deadline_start, const SurfaceReasons& remaining) {
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
  const auto census = ClassifyLabSurface(state, budget, remaining);
  const bool reason_accounting = census.above == census.left_refused + census.left_short + census.left_border +
                                                 census.left_limit + census.left_neighbor_locked;
  const bool qualified = (accepted || reason_accounting) && census.unexplained == 0;
  std::cout << "FP2_A LAB_S_PROOF entities=" << keys.size() << " classified=" << census.faces
            << " classify_above_tol=" << census.above << " accepted=" << accepted << " qualified=" << qualified
            << " classify_left_refused=" << census.left_refused << " classify_left_short=" << census.left_short
            << " classify_left_border=" << census.left_border << " classify_left_limit=" << census.left_limit
            << " classify_left_neighbor_locked=" << census.left_neighbor_locked
            << " classify_unexplained=" << census.unexplained
            << " seconds_mesh_to_model=" << reverse_seconds << " seconds_model_to_mesh=" << forward_seconds
            << " seconds_classify=" << Seconds(classify_start) << " work=" << budget.work
            << " modeled_peak=" << budget.peak_bytes << std::endl;
  REQUIRE(census.faces == state.Surface().size());
  REQUIRE(census.unexplained == 0);
  REQUIRE(qualified);
}

void ReportRankSweep(const local::Mesh& mesh, const local::SweepRow& sweep, int rank, unsigned pass, bool first) {
  const auto& points = mesh.Points();
  std::vector<size_t> incident(points.size());
  size_t blocked = 0;
  for (const auto& cell : mesh.Cells()) {
    if (!cell.live) continue;
    bool unresolved = false;
    for (auto point : cell.vertices) {
      ++incident[point];
      unresolved |= points[point].unresolved;
    }
    blocked += unresolved;
  }
  size_t largest = 0, over100 = 0, over1000 = 0, unresolved = 0;
  for (size_t i = 0; i < points.size(); ++i) {
    if (points[i].removed) continue;
    largest = std::max(largest, incident[i]);
    over100 += incident[i] > 100;
    over1000 += incident[i] > 1000;
    unresolved += points[i].unresolved;
  }
  std::ostringstream row;
  row << std::setprecision(17) << "NATIVE3D_LOCAL_SWEEP rank=" << rank << " stage=0 pass=" << pass;
  local::PrintSweepFields(row, sweep);
  local::PrintSurfaceSweepFields(row, sweep);
  row << " max_star=" << largest << " star_gt100=" << over100 << " star_gt1000=" << over1000
      << " unresolved_points=" << unresolved << " blocked_cells=" << blocked << " first_volume_sweep=" << first
      << '\n';
  std::cout << row.str() << std::flush;
}

struct RankRefusedEdge {
  const char* kind;
  local::SurfaceEdge edge;
  local::SurfaceRefusal reason;
};

const char* RankRefusalName(local::SurfaceRefusal reason) {
  switch (reason) {
    case local::SurfaceRefusal::Border: return "border";
    case local::SurfaceRefusal::Ineligible: return "ineligible";
    case local::SurfaceRefusal::Walk: return "walk";
    case local::SurfaceRefusal::Protected: return "protected";
    case local::SurfaceRefusal::Feature: return "feature";
    case local::SurfaceRefusal::Trace: return "trace";
    case local::SurfaceRefusal::Duplicate: return "duplicate";
    case local::SurfaceRefusal::Outward: return "outward";
    case local::SurfaceRefusal::Size: return "size";
    case local::SurfaceRefusal::Validity: return "validity";
    case local::SurfaceRefusal::Topology: return "topology";
    case local::SurfaceRefusal::Memory: return "memory";
    case local::SurfaceRefusal::Entity: return "entity";
    case local::SurfaceRefusal::None: return "none";
  }
  throw std::logic_error("Unknown surface refusal in the rank check.");
}

void ReportRankSurface(const World& world, unsigned pass, bool active, const local::SurfaceCounters& surface,
                       const local::Mesh& mesh, const std::vector<RankRefusedEdge>& refused) {
  std::ostringstream row;
  row << std::setprecision(17) << "NATIVE3D_LOCAL_SURFACE rank=" << world.rank << " stage=0 pass=" << pass
      << " applicable=" << active;
  local::PrintSurfaceFields(row, surface);
  if (active) {
    // Keep the post-surface data even if a later sweep or pass reaches the wrapper limit.
    const auto& points = mesh.Points();
    std::vector<size_t> incident(points.size());
    size_t blocked = 0, unresolved = 0;
    for (const auto& cell : mesh.Cells()) {
      if (!cell.live) continue;
      bool touches_unresolved = false;
      for (auto slot : cell.vertices) {
        ++incident[slot];
        touches_unresolved |= points[slot].unresolved;
      }
      blocked += touches_unresolved;
    }
    std::vector<size_t> slots;
    for (size_t slot = 0; slot < points.size(); ++slot) {
      if (points[slot].removed) continue;
      unresolved += points[slot].unresolved;
      if (incident[slot]) slots.push_back(slot);
    }
    row << " unresolved_points=" << unresolved << " blocked_cells=" << blocked << '\n';
    const auto count = std::min(size_t{5}, slots.size());
    std::partial_sort(slots.begin(), slots.begin() + count, slots.end(), [&](size_t a, size_t b) {
      if (incident[a] != incident[b]) return incident[a] > incident[b];
      return points[a].node.id < points[b].node.id;
    });
    for (size_t i = 0; i < count; ++i) {
      const auto slot = slots[i];
      const auto& point = points[slot];
      const auto id = point.node.id;
      const auto& position = point.node.p;
      row << "FP2_A RANK_STAR rank=" << world.rank << " pass=" << pass << " place=" << i + 1
          << " point=" << id << " incident=" << incident[slot]
          << " x=" << position.x << " y=" << position.y << " z=" << position.z
          << " surface=" << point.surface << " border=" << point.border << '\n';
      for (const auto& refusal : refused) {
        const auto& edge = refusal.edge;
        if (edge[0] != id && edge[1] != id) continue;
        row << "FP2_A RANK_STAR_EDGE rank=" << world.rank << " pass=" << pass << " point=" << id
            << " endpoint_a=" << edge[0] << " endpoint_b=" << edge[1]
            << " kind=" << refusal.kind << " reason=" << RankRefusalName(refusal.reason) << '\n';
      }
    }
  } else {
    row << '\n';
  }
  std::cout << row.str() << std::flush;
}

void ReportRankPass(const World& world, unsigned pass, bool surface, const local::PassStats& stats,
                    double elapsed, double run_seconds, double meeting_seconds) {
  std::ostringstream row;
  row << std::setprecision(17) << "FP2_A RANK_PASS rank=" << world.rank << " pass=" << pass << " surface=" << surface
      << " cells_in=" << stats.cells_in << " cells_out=" << stats.cells_out
      << " surface_splits=" << stats.surface.splits_length + stats.surface.splits_distance
      << " seconds_pass=" << elapsed << " seconds_run_pass=" << run_seconds
      << " seconds_surface_length=" << stats.surface.seconds_length
      << " seconds_surface_distance=" << stats.surface.seconds_distance
      << " seconds_length_work=" << stats.seconds_collapse + stats.seconds_split
      << " seconds_quality_work=" << stats.seconds_quality << " seconds_meeting=" << meeting_seconds
      << " seconds_build=" << stats.seconds_build << " seconds_write=" << stats.seconds_write << '\n';
  std::cout << row.str() << std::flush;
}

double RunSurfaceWorkerPasses(World& world, LabInput& input, local::StateSurfaceGeometry* surface,
                              local::MemoryBudget& memory, size_t reference_bytes) {
  constexpr auto max = CPassiveComm::Op::MAX;
  local::IdSequence point_ids(world.reduce<uint64_t>(input.point_base, max), world.rank, world.size);
  local::IdSequence cell_ids(world.reduce<uint64_t>(input.cell_base, max), world.rank, world.size);
  auto options = Variant("V2");
  options.surface_splits = surface != nullptr;
  local::BorderMeeting previous;
  double exchange_seconds = 0;
  for (unsigned pass = 1; pass <= local::PassCount; ++pass) {
    const auto pass_start = Clock::now();
    memory.caller_bytes = transfer_memory::Add(reference_bytes, surface ? surface->ResidentBytes() : 0);
    auto meeting = local::MeetBorderPoints(world, input.cells, memory, previous.Bytes());
    const auto meeting_seconds = Seconds(pass_start);
    const auto retained = transfer_memory::Add(reference_bytes, previous.Bytes(), meeting.Bytes());
    const auto owned_bytes = OwnedStorageBytes(input.cells);
    const auto extra_bytes = surface ? local::Mesh::SurfaceArrayBytes(input.cells.size()) : 0;
    const auto mesh_bytes = transfer_memory::Add(local::Mesh::ArrayBytes(input.cells.size()), extra_bytes);
    const auto bytes = transfer_memory::Add(retained, owned_bytes, mesh_bytes, surface ? surface->ResidentBytes() : 0);
    memory.Require(world, bytes, "FP2-A rank check core admission.");
    const auto bytes_per_cell = memory.owned_row_bytes + local::Mesh::BytesPerCell +
                                (surface ? local::Mesh::SurfaceArrayBytes(1) : 0);
    const auto cell_limit = (input.ceiling - memory.caller_bytes) / bytes_per_cell;
    CheckLabStep(world, [&] {
      const auto build_start = Clock::now();
      local::Mesh mesh(input.cells, true, options);
      const auto build_seconds = Seconds(build_start);
      mesh.MarkBorderSurface(meeting);
      if (surface) surface->SetCallerBytes(transfer_memory::Add(reference_bytes, owned_bytes));
      bool first = true;
      std::vector<RankRefusedEdge> refused;
      local::PassStats stats;
      stats.report = true;
      stats.sink = [&](const local::SweepRow& sweep) {
        const bool volume = std::strcmp(sweep.kind, "surface_length") && std::strcmp(sweep.kind, "surface_distance");
        ReportRankSweep(mesh, sweep, world.rank, pass, volume && std::exchange(first, false));
      };
      stats.surface_refusal_sink = [&](const char* kind, const local::SurfaceEdge& edge, local::SurfaceRefusal reason) {
        refused.push_back({kind, edge, reason});
      };
      stats.surface_sink = [&](const local::SurfaceCounters& counts) {
        ReportRankSurface(world, pass, true, counts, mesh, refused);
      };
      const auto run_start = Clock::now();
      stats = mesh.RunPass(point_ids, cell_limit, std::move(stats), options, surface);
      const auto run_seconds = Seconds(run_start);
      const auto write_start = Clock::now();
      mesh.WriteBack(input.cells, cell_ids);
      stats.seconds_build = build_seconds;
      stats.seconds_write = Seconds(write_start);
      // Keep local completion evidence even when another rank has not reached the vote.
      if (!surface) ReportRankSurface(world, pass, false, stats.surface, mesh, refused);
      ReportRankPass(world, pass, surface != nullptr, stats, Seconds(pass_start), run_seconds, meeting_seconds);
    });
    if (surface) {
      memory.maximum_bytes = std::max(memory.maximum_bytes, surface->PeakBytes());
      const auto exchange_start = Clock::now();
      surface->Exchange(world, memory, transfer_memory::Add(retained, OwnedStorageBytes(input.cells)));
      exchange_seconds += Seconds(exchange_start);
      memory.caller_bytes = transfer_memory::Add(reference_bytes, surface->ResidentBytes());
    }
    const bool lower = pass == 1;
    const auto direction = lower ? local::Direction::Lower : local::Direction::Higher;
    local::ShiftBorders(world, input.cells, meeting, direction, memory, lower ? nullptr : &previous);
    if (lower) previous.rendezvous = std::move(meeting.rendezvous);
  }
  return exchange_seconds;
}

size_t CheckRankOwnedGeometry(const Owned& owned, const BoundaryParameterState& state, bool checked_by_builder) {
  size_t count = 0;
  for (const auto& entry : owned) {
    const auto& row = entry.second;
    for (unsigned k = 0; k < row.physical.size(); ++k) {
      if (!row.physical[k].present) continue;
      ++count;
      if (checked_by_builder) continue;
      const auto order = local::OrientedFace({0, 1, 2, 3}, k);
      Face face{row.cell.v[order[0]].id, row.cell.v[order[1]].id, row.cell.v[order[2]].id};
      auto key = face;
      std::sort(key.begin(), key.end());
      std::rotate(face.begin(), std::min_element(face.begin(), face.end()), face.end());
      const auto& stored = state.Surface().at(key);
      if (stored.v != face || stored.facet != state.Geometry().Canonical(row.physical[k].facet))
        throw std::runtime_error("Rank check found a different owned face orientation or anchor.");
      for (auto index : order) {
        const auto& node = row.cell.v[index];
        const auto* point = state.Find(node.id);
        if (!point || point->node.id != node.id || std::memcmp(&node.p.x, &point->node.p.x, sizeof(double)) ||
            std::memcmp(&node.p.y, &point->node.p.y, sizeof(double)) ||
            std::memcmp(&node.p.z, &point->node.p.z, sizeof(double)))
          throw std::runtime_error("Rank check found a different owned boundary point.");
      }
    }
  }
  return count;
}

size_t CheckSnapshotAgreement(World& world, const BoundaryParameterState& state, local::MemoryBudget& memory,
                             size_t caller) {
  memory.Require(world, transfer_memory::Add(caller, transfer_memory::Mul(4, state.SnapshotBytes()),
                                            transfer_memory::TransportBytes(world.size)),
                 "FP2-A rank check snapshot exceeds storage admission.");
  std::vector<char> snapshot, root;
  CheckLabStep(world, [&] {
    snapshot = state.Snapshot();
    if (!world.rank) root = snapshot;
  });
  CPassiveComm::BcastRounds(root, 0, world.comm);
  REQUIRE_FALSE(world.maximum(snapshot != root));
  return snapshot.size();
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
  SurfaceReasons remaining;
  local::PassStats stats;
  stats.surface_remaining_sink = [&](Face face, local::SurfaceRemaining reason) {
    std::sort(face.begin(), face.end());
    REQUIRE(remaining.emplace(face, reason).second);
  };
  double build_seconds = 0, pass_seconds = 0, write_seconds = 0;
  {
    const auto build_start = Clock::now();
    local::Mesh mesh(input.cells, false, options);
    build_seconds = Seconds(build_start);
    const auto pass_start = Clock::now();
    stats = mesh.RunPass(point_ids, cell_limit, std::move(stats), options, &surface);
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
  const auto reasons_bytes =
      transfer_memory::Mul(remaining.size(), sizeof(SurfaceReasons::value_type) + 4 * sizeof(void*));
  const auto proof_live =
      transfer_memory::Add(caller, surface.ResidentBytes(), result.state->ResidentBytes(), reasons_bytes,
                           transfer_memory::Mul(result.state->Surface().size(), sizeof(SurfaceFace)));
  ProveLabSurface(*result.state, stats.cells_in, input.ceiling, proof_live, start, remaining);
}

TEST_CASE("FP2-A rank check on the frozen M6 event 1 volume", "[.][NativeLocalCoreSurfaceRankCheck]") {
  const char* setting = std::getenv("SU2_FP2_A_RANK_SURFACE");
  REQUIRE(setting);
  const std::string mode(setting);
  REQUIRE((mode == "0" || mode == "1"));
  const bool active = mode == "1";
  const auto start = Clock::now();
  World world;
  ReferenceState reference;
  auto input = LoadMesh(&reference, true);
  const auto global_cells = world.reduce<uint64_t>(input.cells.size(), CPassiveComm::Op::SUM);
  REQUIRE(global_cells == 303359);
  REQUIRE((reference.marker_names == std::vector<std::string>{"FARFIELD", "SYMMETRY", "WING3D"}));
  REQUIRE(reference.original->Parameters()->Mobility().identity == 0);
  CheckLabStep(world, [&] { native_fp2_fixture::PrepareIN1(reference, input.ceiling, OwnedStorageBytes(input.cells)); });
  const auto& original = *reference.original->Parameters();
  REQUIRE(original.Surface().size() == 48954);
  REQUIRE(original.Records().size() == 24479);
  const auto reference_bytes = reference.original->ResidentBytes();

  PartitionStats routing;
  std::string reason;
  const auto partition_start = Clock::now();
  const std::vector<uint32_t> weights(input.cells.size(), 1);
  const auto partitioned = Repartition(world, input.cells, weights, world.size, routing, reason, input.ceiling,
                                       transfer_memory::Add(reference_bytes, transfer_memory::Bytes(weights)));
  const auto partition_seconds = Seconds(partition_start);
  INFO(reason);
  REQUIRE(partitioned);
  const auto input_cells = input.cells.size();
  REQUIRE_FALSE(world.maximum(input_cells == 0));

  constexpr auto owned_row_bytes = sizeof(Owned::value_type) + 4 * sizeof(void*);
  local::MemoryBudget memory{input.ceiling, reference_bytes, owned_row_bytes, routing.maximum_native_bytes};
  const auto caller = [&] { return transfer_memory::Add(reference_bytes, OwnedStorageBytes(input.cells)); };
  std::unique_ptr<local::StateSurfaceGeometry> surface;
  if (active) {
    CheckLabStep(world, [&] {
      auto budget = native_fp2_fixture::Budget(global_cells, caller(), start, 5400, input.ceiling);
      surface = std::make_unique<local::StateSurfaceGeometry>(reference.original, std::move(budget));
    });
  }
  double exchange_seconds = 0;
  local::SurfaceStateResult result;
  try {
    exchange_seconds = RunSurfaceWorkerPasses(world, input, surface.get(), memory, reference_bytes);
    const auto exact = CountExact(input.cells);
    std::ostringstream row;
    row << "FP2_A RANK_EXACT rank=" << world.rank << " cells=" << input.cells.size()
        << " nonpositive=" << exact.nonpositive << " first_nonpositive=" << exact.first << '\n';
    std::cout << row.str() << std::flush;
    REQUIRE_FALSE(world.maximum(exact.nonpositive != 0));
    if (surface) result = local::BuildSurfaceState(world, input.cells, *surface, memory, caller());
  } catch (const CElectedFailure& error) {
    FAIL(error.message);
  }

  const bool rebuilt = result.state != nullptr;
  REQUIRE_FALSE(world.maximum(rebuilt != (result.splits != 0)));
  std::shared_ptr<const PlanarReference> retained = reference.original;
  if (rebuilt) CheckLabStep(world, [&] { reference.original = retained->WithParameterState(result.state); });
  const auto& state = *reference.original->Parameters();
  const auto check_start = Clock::now();
  size_t owned_faces = 0;
  CheckLabStep(world, [&] { owned_faces = CheckRankOwnedGeometry(input.cells, state, rebuilt); });
  const auto total_owned_faces = world.reduce<uint64_t>(owned_faces, CPassiveComm::Op::SUM);
  const auto check_seconds = result.seconds_check + Seconds(check_start);
  const auto surface_bytes = surface ? surface->ResidentBytes() : 0;
  const auto snapshot_bytes = CheckSnapshotAgreement(
      world, state, memory, transfer_memory::Add(caller(), surface_bytes, rebuilt ? state.ResidentBytes() : 0));
  REQUIRE(total_owned_faces == state.Surface().size());
  REQUIRE(state.Revision() == original.Revision() + size_t(rebuilt));
  REQUIRE(state.Surface().size() == original.Surface().size() + 2 * result.splits);
  REQUIRE(state.Records().size() == original.Records().size() + result.splits);
  if (!active) REQUIRE(result.splits == 0);

  std::ostringstream row;
  row << std::setprecision(17) << "FP2_A RANK_CHECK rank=" << world.rank << " ranks=" << world.size
      << " surface=" << active << " input_cells=" << input_cells << " output_cells=" << input.cells.size()
      << " splits=" << result.splits << " owned_faces=" << owned_faces << " total_owned_faces=" << total_owned_faces
      << " faces=" << state.Surface().size() << " points=" << state.Records().size()
      << " traces=" << state.Coverage().traces.size() << " revision=" << state.Revision()
      << " snapshot_bytes=" << snapshot_bytes << " snapshot_equal=1 exact_nonpositive=0"
      << " owned_geometry_equal=1 state_rebuilt=" << rebuilt << " seconds_partition=" << partition_seconds
      << " seconds_exchange=" << exchange_seconds << " seconds_gather=" << result.seconds_gather
      << " seconds_build=" << result.seconds_build << " seconds_check=" << check_seconds
      << " modeled_peak=" << std::max(result.modeled_peak, memory.maximum_bytes)
      << " peak_rss=" << ProcessPeakRSS3D() << '\n';
  std::cout << row.str() << std::flush;
}

#include "fixtures/native_handover.inc"
#include "fixtures/native_handover_quality.inc"
#include "fixtures/native_handover_volume.inc"
#include "fixtures/native_volume_handover.inc"
