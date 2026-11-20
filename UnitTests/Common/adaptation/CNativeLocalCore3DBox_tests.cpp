/*!
 * \file CNativeLocalCore3DBox_tests.cpp
 * \brief Serial measures, sweeps and publication checks for the native tetrahedral local core.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <utility>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;

Tensor BoxMetric(Point p) {
  const double scale = 16 + 6 * p.x + 2 * p.y + 4 * p.z;
  return {scale, .125 * p.z, .25 * p.y, scale + 1, .375 * p.x, scale + 2};
}

Owned Box() {
  constexpr int n = 6;
  constexpr std::array<double, n + 1> x{0., .125, .25, .25 + 1. / 128., .375, .5, 1.};
  constexpr std::array<unsigned, 6> ring{1, 2, 3, 7, 4, 5};
  auto node = [&](int i, int j, int k) {
    return Node{(Id(1) << 56) + Id((k * (n + 1) + j) * (n + 1) + i), {x[i], j / 8., k / 8.}};
  };
  Owned rows;
  for (int k = 0; k < n; ++k)
    for (int j = 0; j < n; ++j)
      for (int i = 0; i < n; ++i) {
        const std::array<Node, 8> v{node(i, j, k), node(i + 1, j, k), node(i + 1, j + 1, k), node(i, j + 1, k),
                                    node(i, j, k + 1), node(i + 1, j, k + 1), node(i + 1, j + 1, k + 1),
                                    node(i, j + 1, k + 1)};
        for (size_t t = 0; t < ring.size(); ++t) {
          const Id id = (Id(1) << 60) + 13 * rows.size();
          OwnedCell row;
          row.cell = {id, {v[0], v[6], v[ring[t]], v[ring[(t + 1) % ring.size()]]}};
          row.version = 1 + rows.size() % 17;
          auto& tet = row.cell.v;
          if (Orientation(tet[0].p, tet[1].p, tet[2].p, tet[3].p) < 0) std::swap(tet[0], tet[1]);
          for (size_t f = 0; f < 4; ++f) {
            std::array<Point, 3> face{};
            for (size_t p = 0, q = 0; p < 4; ++p)
              if (p != f) face[q++] = tet[p].p;
            const auto a = face[0], b = face[1], c = face[2];
            const bool physical = (a.x == b.x && a.x == c.x && (a.x == 0 || a.x == 1)) ||
                                  (a.y == b.y && a.y == c.y && (a.y == 0 || a.y == n / 8.)) ||
                                  (a.z == b.z && a.z == c.z && (a.z == 0 || a.z == n / 8.));
            row.physical[f] = {physical, 1000 + 4 * rows.size() + f};
            row.sensor[f] = BoxMetric(tet[f].p);
          }
          row.sensor_present = true;
          rows.emplace(id, row);
        }
      }
  return rows;
}

std::vector<char> Payload(OwnedCell row) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  stream(row);
  return bytes;
}

std::vector<char> Payload(const Owned& rows) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  for (const auto& entry : rows) {
    Id key = entry.first;
    auto row = entry.second;
    stream(key, row);
  }
  return bytes;
}

using NodalPayloads = std::map<Id, std::vector<char>>;

std::vector<char> Payload(Node node, Tensor tensor) {
  std::vector<char> bytes;
  RecordStream stream(bytes);
  stream(node, tensor);
  return bytes;
}

NodalPayloads Nodes(const Owned& rows) {
  NodalPayloads nodes;
  for (const auto& [id, row] : rows) {
    REQUIRE(id == row.cell.id);
    REQUIRE(row.sensor_present);
    for (size_t i = 0; i < 4; ++i) {
      const auto node = row.cell.v[i];
      const auto bytes = Payload(node, row.sensor[i]);
      const auto stored = nodes.emplace(node.id, bytes).first;
      REQUIRE(stored->second == bytes);
    }
  }
  return nodes;
}

std::map<Face, Id> Surface(const Owned& rows) {
  constexpr unsigned order[4][3] = {{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};
  std::map<Face, Id> faces;
  for (const auto& entry : rows) {
    const auto& row = entry.second;
    for (size_t f = 0; f < 4; ++f) {
      if (!row.physical[f].present) continue;
      Face face{};
      for (size_t p = 0; p < 3; ++p) face[p] = row.cell.v[order[f][p]].id;
      std::rotate(face.begin(), std::min_element(face.begin(), face.end()), face.end());
      REQUIRE(faces.emplace(face, row.physical[f].facet).second);
    }
  }
  return faces;
}

std::set<std::array<Id, 4>> Simplices(const Owned& rows) {
  std::set<std::array<Id, 4>> keys;
  for (const auto& entry : rows) {
    const auto& v = entry.second.cell.v;
    std::array<Id, 4> key{v[0].id, v[1].id, v[2].id, v[3].id};
    std::sort(key.begin(), key.end());
    REQUIRE(keys.insert(key).second);
  }
  return keys;
}

std::vector<DonorCell> Donors(const Owned& rows) {
  std::vector<DonorCell> donors;
  donors.reserve(rows.size());
  for (const auto& entry : rows) donors.push_back({entry.second.cell, entry.second.sensor});
  return donors;
}

Edge EdgeKey(Id a, Id b) { return {std::min(a, b), std::max(a, b)}; }

struct EdgeIncidence {
  local::Edge slots;
  local::Slot cell;
  bool closed = true;
};

std::map<Edge, EdgeIncidence> Edges(const local::Mesh& mesh) {
  std::map<Edge, EdgeIncidence> edges;
  const auto& points = mesh.Points();
  const auto& cells = mesh.Cells();
  for (local::Slot c = 0; c < cells.size(); ++c) {
    const auto& cell = cells[c];
    if (!cell.live) continue;
    for (size_t i = 0; i < 4; ++i)
      for (size_t j = i + 1; j < 4; ++j) {
        local::Edge slots{cell.vertices[i], cell.vertices[j]};
        const Id a = points[slots[0]].node.id, b = points[slots[1]].node.id;
        if (a > b) std::swap(slots[0], slots[1]);
        auto& edge = edges.emplace(EdgeKey(a, b), EdgeIncidence{slots, c}).first->second;
        for (size_t f = 0; f < 4; ++f)
          if (f != i && f != j && cell.neighbors[f] < 0) edge.closed = false;
      }
  }
  return edges;
}

std::set<Edge> LongEdges(const local::Mesh& mesh) {
  std::set<Edge> edges;
  for (const auto& [key, edge] : Edges(mesh))
    if (mesh.Length(edge.slots) > local::LongEdge) edges.insert(key);
  return edges;
}

double MinimumMargin(const local::Mesh& mesh) {
  double minimum = std::numeric_limits<double>::infinity();
  for (const auto& cell : mesh.Cells())
    if (cell.live) minimum = std::min(minimum, mesh.Margin(cell.vertices));
  return minimum;
}

Id LastPoint(const Owned& rows) {
  Id last = 0;
  for (const auto& entry : rows)
    for (const auto& node : entry.second.cell.v) last = std::max(last, node.id);
  return last;
}

void CheckLengthWork(local::Mesh& mesh, const local::IdSequence& ids, const local::PassStats& stats) {
  constexpr auto probe_stamp = std::numeric_limits<uint32_t>::max();
  size_t closed_long = 0, long_validity = 0, long_limit = 0, long_blocked = 0, admissible_short = 0;
  mesh.MarkWallNear();
  for (const auto& [key, edge] : Edges(mesh)) {
    const auto length = mesh.Length(edge.slots);
    CAPTURE(key[0], key[1], length);
    if (length > local::LongEdge) {
      CHECK(mesh.ClosedRing(edge.slots, edge.cell) == edge.closed);
      if (!edge.closed) continue;
      ++closed_long;
      local::Mesh trial = mesh;
      auto point_ids = ids;
      local::PassStats split;
      const auto reason = trial.Split(edge.slots, edge.cell, point_ids, probe_stamp, SIZE_MAX, split);
      if (reason == local::Refusal::Validity) {
        CHECK((stats.refused_valid > 0 || stats.limit));
        ++long_validity;
      } else if (reason == local::Refusal::Blocked) {
        CHECK(split.blocked == 1);
        CHECK(stats.blocked > 0);
        ++long_blocked;
      } else {
        CHECK((reason == local::Refusal::None && stats.limit));
        ++long_limit;
      }
    }
    if (length < local::ShortEdge) {
      local::PassStats collapse;
      if (mesh.Collapse(edge.slots, probe_stamp, collapse, false) == local::Refusal::None) ++admissible_short;
    }
  }
  INFO("closed long edges: " << closed_long << ", admissible short edges: " << admissible_short);
  CHECK((admissible_short == 0 || stats.limit));
  std::cout << "FP1_FIXTURE length closed_long=" << closed_long << " admissible_short=" << admissible_short
            << " long_validity=" << long_validity << " long_limit=" << long_limit
            << " long_blocked=" << long_blocked
            << " refused_valid=" << stats.refused_valid << " refused_memory=" << stats.refused_memory
            << " limit=" << stats.limit << '\n';
}

struct MetricCounts {
  size_t q = 0, j = 0, long_cells = 0;
};

MetricCounts ProductionCounts(const Owned& rows) {
  FrozenField field(Donors(rows), {}, false, {}, {}, {}, true, rows.size(), true);
  MetricCounts counts;
  for (auto entry = rows.begin(); entry != rows.end();) {
    std::vector<Cell> batch;
    for (; batch.size() < MaximumReplacementCells && entry != rows.end(); ++entry)
      batch.push_back(entry->second.cell);
    const auto patch = MeasureMetricCells(batch, field);
    counts.q += patch.q_bad_cells;
    counts.j += patch.j_bad_cells;
    for (const auto& cell : batch) {
      bool long_edge = false;
      for (size_t i = 0; i < 4; ++i)
        for (size_t j = i + 1; j < 4; ++j)
          long_edge |= patch.edges.at(EdgeKey(cell.v[i].id, cell.v[j].id)).length > MaximumMetricEdge;
      counts.long_cells += long_edge;
    }
  }
  return counts;
}

void CheckGateCounts(const local::Mesh& mesh, const Owned& rows) {
  MetricCounts counts;
  size_t margin_cells = 0, margin_edges = 0;
  for (const auto& cell : mesh.Cells()) {
    if (!cell.live) continue;
    const auto shape = mesh.Shape(cell.vertices);
    counts.q += shape.mean_ratio < MinimumMeanRatio;
    counts.j += shape.minimum_scaled_jacobian < MinimumScaledJacobian;
    const auto margin = mesh.Margin(cell.vertices);
    margin_cells += margin >= 1 && margin < local::BadMargin;
    bool long_edge = false;
    for (size_t i = 0; i < 4; ++i)
      for (size_t j = i + 1; j < 4; ++j)
        long_edge |= mesh.Length({cell.vertices[i], cell.vertices[j]}) > MaximumMetricEdge;
    counts.long_cells += long_edge;
  }
  for (const auto& entry : Edges(mesh)) {
    const auto length = mesh.Length(entry.second.slots);
    margin_edges += length > local::LongEdge && length <= MaximumMetricEdge;
  }
  const auto production = ProductionCounts(rows);
  CHECK(counts.q == production.q);
  CHECK(counts.j == production.j);
  CHECK(counts.long_cells == production.long_cells);
  std::cout << "FP1_FIXTURE strict q_bad=" << counts.q << " j_bad=" << counts.j << " long_cells=" << counts.long_cells
            << " margin_cells=" << margin_cells << " margin_edges=" << margin_edges << '\n';
}

Owned RunPass(Owned rows) {
  local::Mesh mesh(rows);
  local::IdSequence point_ids{LastPoint(rows)}, cell_ids{rows.rbegin()->first};
  local::PassStats stats;
  mesh.LengthWork(point_ids, SIZE_MAX, stats);
  mesh.QualityWork(point_ids, SIZE_MAX, stats);
  mesh.Check();
  mesh.WriteBack(rows, cell_ids);
  return rows;
}
}  // namespace

TEST_CASE("Local tetrahedral box measures agree with production current nodal measures",
          "[.][NativeLocalCore3D][NativeLocalCore3DBox]") {
  const auto rows = Box();
  const auto donors = Donors(rows);
  const local::Mesh mesh(rows);
  FrozenField field(donors, {}, false, {}, {}, {}, true, donors.size(), true);
  long double length_error = 0;
  size_t short_edges = 0, long_edges = 0, bad_cells = 0;
  for (const auto& entry : Edges(mesh)) {
    const auto edge = entry.second.slots;
    const auto length = mesh.Length(edge);
    const auto& points = mesh.Points();
    const auto strict = field.TargetEdgeLength(points[edge[0]].node.p, points[edge[1]].node.p);
    REQUIRE(std::isfinite(length));
    length_error = std::max(length_error, std::abs(strict - length));
    short_edges += length < local::ShortEdge;
    long_edges += length > local::LongEdge;
  }
  double q_error = 0, j_error = 0;
  for (const auto& cell : mesh.Cells()) {
    const auto& row = rows.at(cell.id);
    const auto& v = row.cell.v;
    const auto tensor = InterpolateSensor({row.cell, row.sensor}, {.25L, .25L, .25L, .25L});
    const auto strict = Measure({v[0].p, v[1].p, v[2].p, v[3].p}, tensor);
    const auto fast = mesh.Shape(cell.vertices);
    REQUIRE((std::isfinite(fast.mean_ratio) && std::isfinite(fast.minimum_scaled_jacobian)));
    q_error = std::max(q_error, std::abs(strict.mean_ratio - fast.mean_ratio));
    j_error = std::max(j_error, std::abs(strict.minimum_scaled_jacobian - fast.minimum_scaled_jacobian));
    bad_cells += mesh.Margin(cell.vertices) < local::BadMargin;
  }
  CHECK(length_error < 1e-12L);
  CHECK(q_error < 1e-12);
  CHECK(j_error < 1e-12);
  REQUIRE(short_edges > 0);
  REQUIRE(long_edges > 0);
  REQUIRE(bad_cells > 0);
  std::cout << "FP1_FIXTURE measures length_error=" << length_error << " q_error=" << q_error << " j_error=" << j_error
            << " short_edges=" << short_edges << " long_edges=" << long_edges << " bad_cells=" << bad_cells << '\n';
}

TEST_CASE("Local tetrahedral box length and quality sweeps preserve the frozen surface",
          "[.][NativeLocalCore3D][NativeLocalCore3DBox]") {
  const auto initial = Box();
  auto rows = initial;
  local::Mesh mesh(rows);
  local::IdSequence point_ids{LastPoint(rows)}, cell_ids{rows.rbegin()->first};
  local::PassStats stats;
  mesh.LengthWork(point_ids, SIZE_MAX, stats);
  REQUIRE(stats.collapses > 0);
  REQUIRE(stats.splits > 0);
  CheckLengthWork(mesh, point_ids, stats);
  const auto weakest = MinimumMargin(mesh);
  const auto long_before = LongEdges(mesh);
  mesh.QualityWork(point_ids, SIZE_MAX, stats);
  const auto long_after = LongEdges(mesh);
  CHECK(MinimumMargin(mesh) >= weakest);
  CHECK(std::includes(long_before.begin(), long_before.end(), long_after.begin(), long_after.end()));
  REQUIRE_NOTHROW(mesh.Check());
  mesh.WriteBack(rows, cell_ids);
  const auto initial_nodes = Nodes(initial), final_nodes = Nodes(rows);
  for (const auto& [id, bytes] : final_nodes) {
    const auto before = initial_nodes.find(id);
    if (before == initial_nodes.end())
      CHECK(id > initial_nodes.rbegin()->first);
    else
      CHECK(bytes == before->second);
  }
  for (const auto& [id, row] : rows) {
    const auto before = initial.find(id);
    if (before == initial.end()) {
      CHECK(id > initial.rbegin()->first);
      CHECK(row.version == 0);
    } else {
      CHECK(Payload(row) == Payload(before->second));
    }
  }
  const auto skin = Surface(initial);
  CHECK(Surface(rows) == skin);
  for (const auto& entry : skin)
    for (const auto id : entry.first) CHECK(final_nodes.at(id) == initial_nodes.at(id));
  CHECK(Simplices(rows).size() == rows.size());
  CheckGateCounts(mesh, rows);
  std::cout << "FP1_FIXTURE pass cells_in=" << initial.size() << " cells_out=" << rows.size()
            << " collapses=" << stats.collapses << " splits=" << stats.splits << " edge_removals=" << stats.edge_removals
            << " face_swaps=" << stats.face_swaps << " moves=" << stats.moves << '\n';
}

TEST_CASE("Local tetrahedral box round trip and fresh ids are deterministic",
          "[.][NativeLocalCore3D][NativeLocalCore3DBox]") {
  const auto rows = Box();
  SECTION("Untouched owned rows retain the entire serialized payload") {
    auto result = rows;
    const local::Mesh mesh(result);
    local::IdSequence cell_ids{rows.rbegin()->first};
    mesh.WriteBack(result, cell_ids);
    CHECK(Payload(result) == Payload(rows));
  }
  SECTION("Changing cell identities reverses build order while keeping point identities") {
    const auto first = RunPass(rows);
    CHECK(Payload(RunPass(rows)) == Payload(first));
    Owned reversed;
    Id id = Id(1) << 62;
    for (auto entry = rows.rbegin(); entry != rows.rend(); ++entry) {
      auto row = entry->second;
      row.cell.id = id++;
      reversed.emplace(row.cell.id, row);
    }
    const auto reordered = RunPass(std::move(reversed));
    CHECK(Simplices(first) == Simplices(reordered));
    CHECK(Nodes(first) == Nodes(reordered));
  }
}

TEST_CASE("Local tetrahedral box collapse does not change cells touching the wall",
          "[.][NativeLocalCore3D][NativeLocalCore3DBox]") {
  const auto initial = Box();
  auto rows = initial;
  local::Mesh mesh(rows);
  local::PassStats stats;
  mesh.CollapseSweeps(stats);
  REQUIRE(stats.collapses > 0);
  REQUIRE_NOTHROW(mesh.Check());
  local::IdSequence cell_ids{rows.rbegin()->first};
  mesh.WriteBack(rows, cell_ids);
  std::set<Id> surface_points;
  for (const auto& entry : Surface(initial)) surface_points.insert(entry.first.begin(), entry.first.end());
  for (const auto& [id, row] : initial) {
    if (!std::any_of(row.cell.v.begin(), row.cell.v.end(), [&](const auto& p) { return surface_points.count(p.id); }))
      continue;
    REQUIRE(rows.count(id) == 1);
    CHECK(Payload(rows.at(id)) == Payload(row));
  }
}
