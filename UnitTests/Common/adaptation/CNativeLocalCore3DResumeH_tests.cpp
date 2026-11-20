/*!
 * \file CNativeLocalCore3DResumeH_tests.cpp
 * \brief Surface priority, work-list identity, and sweep diagnostics for the local 3D core.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <string_view>
#include <utility>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;

constexpr Id PointBase = Id(1) << 56, CellBase = Id(1) << 60;
using NodeEdge = std::array<Id, 2>;

std::vector<double> Coordinates(unsigned n, double spacing) {
  std::vector<double> result;
  for (unsigned i = 0; i <= n; ++i) result.push_back(i * spacing);
  return result;
}

template <class Metric>
Owned Box(const std::vector<double>& x, const std::vector<double>& yz, Metric metric) {
  constexpr std::array<unsigned, 6> ring{1, 2, 3, 7, 4, 5};
  const auto node = [&](size_t i, size_t j, size_t k) {
    return Node{PointBase + Id((k * yz.size() + j) * x.size() + i), {x[i], yz[j], yz[k]}};
  };
  Owned rows;
  for (size_t k = 0; k + 1 < yz.size(); ++k)
    for (size_t j = 0; j + 1 < yz.size(); ++j)
      for (size_t i = 0; i + 1 < x.size(); ++i) {
        const std::array<Node, 8> v{node(i, j, k), node(i + 1, j, k), node(i + 1, j + 1, k), node(i, j + 1, k),
                                    node(i, j, k + 1), node(i + 1, j, k + 1), node(i + 1, j + 1, k + 1),
                                    node(i, j + 1, k + 1)};
        for (size_t t = 0; t < ring.size(); ++t) {
          const Id id = CellBase + 13 * rows.size();
          OwnedCell row;
          row.cell = {id, {v[0], v[6], v[ring[t]], v[ring[(t + 1) % ring.size()]]}};
          row.version = 1 + rows.size() % 17;
          row.sensor_present = true;
          auto& tet = row.cell.v;
          if (Orientation(tet[0].p, tet[1].p, tet[2].p, tet[3].p) < 0) std::swap(tet[0], tet[1]);
          for (unsigned f = 0; f < 4; ++f) {
            std::array<Point, 3> face{};
            for (unsigned p = 0, q = 0; p < 4; ++p)
              if (p != f) face[q++] = tet[p].p;
            const auto a = face[0], b = face[1], c = face[2];
            const bool physical = (a.x == b.x && a.x == c.x && (a.x == 0 || a.x == x.back())) ||
                                  (a.y == b.y && a.y == c.y && (a.y == 0 || a.y == yz.back())) ||
                                  (a.z == b.z && a.z == c.z && (a.z == 0 || a.z == yz.back()));
            row.physical[f] = {physical, 1000 + 4 * rows.size() + f};
            row.sensor[f] = metric(tet[f].p);
          }
          rows.emplace(id, row);
        }
      }
  return rows;
}

// Exact geometry, cell/point ids, tensors, and face bindings of the F4.1 box.
Owned ResumeBox() {
  return Box({0., .125, .1875, .25, .25 + 1. / 128., .375, .5, 1.}, Coordinates(7, 1. / 8.), [](Point p) {
    const double scale = 16 + 6 * p.x + 2 * p.y + 4 * p.z;
    return Tensor{scale, 3 * p.z, 3 * p.y, .5 * (scale + 1), 3 * p.x, .5 * (scale + 2)};
  });
}

Owned SweepBox() {
  const std::vector<double> x{0., .125, 17. / 128., 18. / 128., 19. / 128., 20. / 128.,
                               21. / 128., 22. / 128., .25, .5, .875, 1.};
  return Box(x, Coordinates(7, 1. / 8.), [](Point p) {
    const bool boundary = p.x == 0 || p.x == 1 || p.y == 0 || p.y == .875 || p.z == 0 || p.z == .875;
    if (boundary) return Tensor{.5, 0, 0, .5, 0, .5};
    // Dense short slabs exercise successive collapses. One interior tensor forces repeated bisection.
    const bool high = p.x == .5 && p.y == .5 && p.z == .5;
    // Keep all edges at the hot point above the collapse threshold before splitting begins.
    return Tensor{high ? 16384. : 10., 0, 0, high ? 144. : 12., 0, high ? 144. : 12.};
  });
}

constexpr NodeEdge LongSurfaceEdge{PointBase + 2 * 7 + 2, PointBase + 2 * 7 + 3};

Owned SurfaceBox() {
  const auto coordinates = Coordinates(6, 1.);
  return Box(coordinates, coordinates, [](Point p) {
    const bool long_ring = p.y == 2 && ((p.z == 0 && (p.x == 2 || p.x == 3)) || (p.z == 1 && p.x == 3));
    return Tensor{long_ring ? 4. : .25, 0, 0, .25, 0, .25};
  });
}

bool TouchesLongEdge(const OwnedCell& row) {
  return std::any_of(row.cell.v.begin(), row.cell.v.end(), [](const auto& node) {
    return node.id == LongSurfaceEdge[0] || node.id == LongSurfaceEdge[1];
  });
}

std::vector<char> Wire(const Owned& rows, bool only_long_edge = false) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  for (const auto& entry : rows) {
    if (only_long_edge && !TouchesLongEdge(entry.second)) continue;
    auto id = entry.first;
    auto row = entry.second;
    stream(id, row);
  }
  return bytes;
}

void EqualWire(const std::vector<char>& a, const std::vector<char>& b) {
  REQUIRE(a.size() == b.size());
  const auto where = std::mismatch(a.begin(), a.end(), b.begin()).first;
  CAPTURE(std::distance(a.begin(), where));
  CHECK(where == a.end());
}

std::set<NodeEdge> LongPhysicalEdges(const local::Mesh& mesh) {
  std::set<NodeEdge> result;
  for (local::Slot row = 0; row < mesh.Cells().size(); ++row) {
    const auto& cell = mesh.Cells()[row];
    const auto bindings = mesh.Bindings(row);
    for (unsigned f = 0; f < 4; ++f) {
      if (!bindings[f].present) continue;
      for (unsigned i = 0; i < 4; ++i) {
        if (i == f) continue;
        for (unsigned j = i + 1; j < 4; ++j) {
          if (j == f) continue;
          const local::Edge edge{cell.vertices[i], cell.vertices[j]};
          if (mesh.Length(edge) <= local::LongEdge) continue;
          NodeEdge ids{mesh.Points()[edge[0]].node.id, mesh.Points()[edge[1]].node.id};
          std::sort(ids.begin(), ids.end());
          result.insert(ids);
        }
      }
    }
  }
  return result;
}

bool Unresolved(const local::Mesh& mesh, Id id) {
  const auto& points = mesh.Points();
  const auto found = std::find_if(points.begin(), points.end(), [&](const auto& p) { return p.node.id == id; });
  return found != points.end() && found->unresolved;
}

Id MaximumPointId(const Owned& rows) {
  Id id = 0;
  for (const auto& entry : rows)
    for (const auto& point : entry.second.cell.v) id = std::max(id, point.id);
  return id;
}

local::PassStats Adapt(local::Mesh& mesh, Owned& rows, local::IdSequence& point_ids, local::IdSequence& cell_ids,
                       const local::CoreOptions& options, local::PassStats stats = {}) {
  auto measured = mesh.RunPass(point_ids, SIZE_MAX, std::move(stats), options);
  mesh.Check();
  mesh.WriteBack(rows, cell_ids);
  return measured;
}

struct Pass {
  local::PassStats stats;
  std::vector<char> bytes;
};

Pass Run(const Owned& initial, const local::CoreOptions& options, local::PassStats stats = {}) {
  auto rows = initial;
  local::Mesh mesh(rows, false, options);
  local::IdSequence point_ids(MaximumPointId(rows)), cell_ids(rows.rbegin()->first);
  return {Adapt(mesh, rows, point_ids, cell_ids, options, std::move(stats)), Wire(rows)};
}

template <class Counts>
std::array<size_t, 5> Edits(const Counts& row) {
  return {row.collapses, row.splits, row.edge_removals, row.face_swaps, row.moves};
}

template <class Counts>
std::array<size_t, 18> Counters(const Counts& row) {
  return {row.evaluated_collapse, row.evaluated_split, row.evaluated_quality, row.deferred, row.skipped_memory,
          row.blocked,           row.collapses,       row.splits,            row.edge_removals, row.face_swaps,
          row.moves,             row.refused_valid,  row.refused_size,      row.refused_topology,
          row.refused_memory,    row.refused_numeric, row.refused_shape,     row.refused_exact};
}

void CheckSweepRows(const std::vector<local::SweepRow>& rows, const local::PassStats& stats) {
  std::map<std::string_view, unsigned> counts{{"collapse", 0}, {"split", 0}, {"quality", 0}};
  std::array<size_t, 18> sum{};
  unsigned previous_round = 0;
  for (const auto& row : rows) {
    const std::string_view kind(row.kind);
    CAPTURE(kind, row.round, row.sweep);
    REQUIRE(counts.count(kind) == 1);
    CHECK(row.sweep == ++counts[kind]);
    if (kind == "quality") {
      CHECK(row.round == 0);
    } else {
      CHECK(row.round > 0);
      CHECK(row.round >= previous_round);
      CHECK(row.round <= stats.rounds);
      CHECK(row.bad_listed == 0);
      previous_round = row.round;
    }
    CHECK(std::isfinite(row.seconds));
    CHECK(row.seconds >= 0.);
    const auto delta = Counters(row.counters);
    for (size_t i = 0; i < sum.size(); ++i) sum[i] += delta[i];
  }
  CHECK(counts.at("collapse") == stats.sweeps_collapse);
  CHECK(counts.at("split") == stats.sweeps_split);
  CHECK(counts.at("quality") == stats.sweeps_quality);
  CHECK(rows.size() == stats.sweeps_collapse + stats.sweeps_split + stats.sweeps_quality);
  CHECK(sum == Counters(stats));
  REQUIRE_FALSE(rows.empty());
  CHECK(rows.back().live_cells == stats.cells_out);
}

unsigned MaximumSweepsInRound(const std::vector<local::SweepRow>& rows, std::string_view kind) {
  std::map<unsigned, unsigned> per_round;
  unsigned maximum = 0;
  for (const auto& row : rows)
    if (row.kind == kind) maximum = std::max(maximum, ++per_round[row.round]);
  return maximum;
}

void CheckWorkList(const Owned& initial, bool many_sweeps) {
  local::CoreOptions work_list, full_scan;
  work_list.work_list = true;
  full_scan.work_list = false;
  local::PassStats request;
  std::vector<local::SweepRow> rows;
  request.sink = [&](const local::SweepRow& row) { rows.push_back(row); };
  const auto work = Run(initial, work_list, std::move(request));
  const auto scan = Run(initial, full_scan);
  EqualWire(work.bytes, scan.bytes);
  CHECK(Edits(work.stats) == Edits(scan.stats));
  CheckSweepRows(rows, work.stats);
  const auto collapse = MaximumSweepsInRound(rows, "collapse");
  const auto split = MaximumSweepsInRound(rows, "split");
  std::cout << "FP1_H_FIXTURE work_list cells=" << initial.size() << " collapse_sweeps_in_one_call=" << collapse
            << " split_sweeps_in_one_call=" << split << " bytes=" << work.bytes.size() << std::endl;
  if (many_sweeps) {
    CHECK(collapse >= 5);
    CHECK(split >= 5);
  }
}
}  // namespace

TEST_CASE("Local core protects the cavity of the unresolved surface edge", "[.][NativeLocalCoreH]") {
  auto rows = SurfaceBox();
  const auto initial = Wire(rows, true);
  REQUIRE_FALSE(initial.empty());
  local::Mesh mesh(rows);
  const std::set<NodeEdge> expected{LongSurfaceEdge};
  REQUIRE(LongPhysicalEdges(mesh) == expected);
  for (auto point : LongSurfaceEdge) CHECK(Unresolved(mesh, point));
  local::IdSequence points(MaximumPointId(rows)), cells(rows.rbegin()->first);
  const auto stats = Adapt(mesh, rows, points, cells, {});
  CHECK(stats.blocked > 0);
  EqualWire(Wire(rows, true), initial);
  std::cout << "FP1_H_FIXTURE unresolved_surface blocked=" << stats.blocked << std::endl;
}

TEST_CASE("Local core propagates the unresolved mark where only a peer sees the long edge",
          "[.][NativeLocalCoreHBorder]") {
  World world;
  REQUIRE(world.size == 2);
  const auto box = SurfaceBox();
  Owned rows;
  for (const auto& entry : box)
    if (int(Centroid(entry.second.cell).x >= 2.125) == world.rank) rows.emplace(entry);
  const auto initial = Wire(rows, true);
  REQUIRE_FALSE(initial.empty());
  local::Mesh mesh(rows, true);
  const std::set<NodeEdge> expected = world.rank ? std::set<NodeEdge>{LongSurfaceEdge} : std::set<NodeEdge>{};
  CHECK(LongPhysicalEdges(mesh) == expected);
  CHECK(Unresolved(mesh, LongSurfaceEdge[0]) == (world.rank == 1));
  local::MemoryBudget memory{SIZE_MAX, 0, sizeof(Owned::value_type) + 4 * sizeof(void*)};
  const auto meeting = local::MeetBorderPoints(world, rows, memory);
  const auto* point = meeting.Find(LongSurfaceEdge[0]);
  CHECK(point != nullptr);
  if (point) CHECK(point->unresolved);
  mesh.MarkBorderSurface(meeting);
  CHECK(Unresolved(mesh, LongSurfaceEdge[0]));
  local::IdSequence points(MaximumPointId(box), world.rank, world.size);
  local::IdSequence cells(box.rbegin()->first, world.rank, world.size);
  local::PassStats stats;
  CLocalFailure failure;
  try {
    stats = Adapt(mesh, rows, points, cells, {});
  } catch (const std::exception& error) {
    failure.Set(1, 0, error.what());
  }
  const auto elected = world.elect(failure);
  INFO(elected.message);
  REQUIRE_FALSE(elected.any);
  const auto blocked = world.reduce<uint64_t>(stats.blocked, CPassiveComm::Op::SUM);
  CHECK(blocked > 0);
  EqualWire(Wire(rows, true), initial);
  if (!world.rank) std::cout << "FP1_H_FIXTURE unresolved_border blocked=" << blocked << std::endl;
}

TEST_CASE("Local core work lists preserve the full-scan edits and sweep rows account for the pass",
          "[.][NativeLocalCoreH]") {
  CheckWorkList(ResumeBox(), false);
  CheckWorkList(SweepBox(), true);
}

TEST_CASE("Local core with the four H switches disabled reproduces the G F4.1 Owned rows",
          "[.][NativeLocalCoreH]") {
  const char* file = std::getenv("SU2_FP1_G_OWNED_BASELINE");
  REQUIRE(file != nullptr);
  std::ifstream input(file, std::ios::binary);
  REQUIRE(input.good());
  const std::istreambuf_iterator<char> first(input), end;
  const std::vector<char> baseline(first, end);
  REQUIRE_FALSE(baseline.empty());

  local::CoreOptions g;
  g.shape_test = g.float_trials = g.surface_first = g.work_list = false;
  g.split_progress = false;
  const auto pass = Run(ResumeBox(), g);
  const std::array<size_t, 5> edits{74, 169, 380, 12, 317};
  CHECK(Edits(pass.stats) == edits);
  CHECK(pass.stats.cells_in == 2058);
  CHECK(pass.stats.cells_out == 2600);
  CHECK(pass.stats.deferred == 2104);
  CHECK(pass.stats.skipped_memory == 1134);
  EqualWire(pass.bytes, baseline);
  std::cout << "FP1_H_FIXTURE exact_g_compatibility bytes=" << pass.bytes.size() << std::endl;
}
