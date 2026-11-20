/*!
 * \file CNativeLocalCore3DResumeBox_tests.cpp
 * \brief Full operator coverage and refusal memory identity for a serial local pass.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"

#include <algorithm>
#include <iostream>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;

Tensor ResumeSensor(Point p) {
  const double scale = 16 + 6 * p.x + 2 * p.y + 4 * p.z;
  return {scale, 3 * p.z, 3 * p.y, .5 * (scale + 1), 3 * p.x, .5 * (scale + 2)};
}

Owned ResumeBox() {
  constexpr int n = 7;
  constexpr std::array<double, n + 1> x{0., .125, .1875, .25, .25 + 1. / 128., .375, .5, 1.};
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
          row.sensor_present = true;
          auto& tet = row.cell.v;
          if (Orientation(tet[0].p, tet[1].p, tet[2].p, tet[3].p) < 0) std::swap(tet[0], tet[1]);
          for (unsigned f = 0; f < 4; ++f) {
            std::array<Point, 3> face{};
            for (unsigned p = 0, q = 0; p < 4; ++p)
              if (p != f) face[q++] = tet[p].p;
            const auto a = face[0], b = face[1], c = face[2];
            const bool physical = (a.x == b.x && a.x == c.x && (a.x == 0 || a.x == x.back())) ||
                                  (a.y == b.y && a.y == c.y && (a.y == 0 || a.y == n / 8.)) ||
                                  (a.z == b.z && a.z == c.z && (a.z == 0 || a.z == n / 8.));
            row.physical[f] = {physical, 1000 + 4 * rows.size() + f};
            row.sensor[f] = ResumeSensor(tet[f].p);
          }
          rows.emplace(id, row);
        }
      }
  return rows;
}

template <class... T>
std::vector<char> WireBytes(T... values) {
  std::vector<char> bytes;
  RecordStream write(bytes);
  write(values...);
  return bytes;
}

std::vector<char> PointBytes(const local::PointRow& p) {
  return WireBytes(p.node, p.tensor, p.incident, p.surface, p.border, p.removed, p.wall_near);
}

std::vector<char> CellBytes(const local::CellRow& c) {
  return WireBytes(c.vertices, c.neighbors, c.id, c.version, c.binding, c.stamp, c.live, c.changed);
}

Id MaximumPointId(const local::Mesh& mesh) {
  Id id = 0;
  for (const auto& point : mesh.Points()) id = std::max(id, point.node.id);
  return id;
}

bool ProbeBothEnds(local::Mesh mesh) {
  mesh.MarkWallNear();
  for (const auto& use : mesh.Edges()) {
    const auto& a = mesh.Points()[use.edge[0]];
    const auto& b = mesh.Points()[use.edge[1]];
    if (!a.Free() || !b.Free() || a.wall_near || b.wall_near || mesh.Length(use.edge) >= local::ShortEdge) continue;
    const auto first = a.node.id, second = b.node.id;
    const auto length = mesh.Length(use.edge);
    local::PassStats stats;
    if (mesh.Collapse(use.edge, 1, stats, true) != local::Refusal::None) continue;
    CHECK(stats.collapses == 1);
    CHECK_NOTHROW(mesh.Check());
    std::cout << "FP1_FIXTURE resume_both_ends a=" << first << " b=" << second << " length=" << length << '\n';
    return true;
  }
  return false;
}

void CheckArrays(const local::Mesh& on, const local::Mesh& off) {
  REQUIRE(on.Points().size() == off.Points().size());
  for (size_t k = 0; k < on.Points().size(); ++k) {
    CAPTURE(k);
    REQUIRE(PointBytes(on.Points()[k]) == PointBytes(off.Points()[k]));
  }
  REQUIRE(on.Cells().size() == off.Cells().size());
  for (size_t k = 0; k < on.Cells().size(); ++k) {
    CAPTURE(k);
    REQUIRE(CellBytes(on.Cells()[k]) == CellBytes(off.Cells()[k]));
    REQUIRE(WireBytes(on.Bindings(k)) == WireBytes(off.Bindings(k)));
  }
}
}  // namespace

TEST_CASE("Local tetrahedral resume box exercises all five operators in one pass", "[.][NativeLocalCore3DResumeA]") {
  const auto rows = ResumeBox();
  local::Mesh mesh(rows);
  CHECK(ProbeBothEnds(mesh));
  local::IdSequence point_ids(MaximumPointId(mesh));
  const auto stats = mesh.RunPass(point_ids, SIZE_MAX, true);
  CHECK(stats.collapses > 0);
  CHECK(stats.splits > 0);
  CHECK(stats.edge_removals > 0);
  CHECK(stats.face_swaps > 0);
  CHECK(stats.moves > 0);
  CHECK(stats.deferred > 0);
  std::cout << "FP1_FIXTURE resume_box cells_in=" << rows.size() << " cells_out=" << stats.cells_out
            << " collapses=" << stats.collapses << " splits=" << stats.splits << " edge_removals=" << stats.edge_removals
            << " face_swaps=" << stats.face_swaps << " moves=" << stats.moves << " deferred=" << stats.deferred << '\n';
}

TEST_CASE("Local tetrahedral refusal memory changes only refusal bookkeeping", "[.][NativeLocalCore3DResumeA]") {
  auto rows_on = ResumeBox();
  auto rows_off = rows_on;
  local::Mesh on(rows_on), off(rows_off);
  const auto base = MaximumPointId(on);
  local::IdSequence points_on(base), points_off(base);
  local::CoreOptions no_memory;
  no_memory.memory = false;
  const auto stats_on = on.RunPass(points_on, SIZE_MAX, true);
  const auto stats_off = off.RunPass(points_off, SIZE_MAX, true, no_memory);
  CHECK(stats_on.skipped_memory > 0);
  CHECK(stats_off.skipped_memory == 0);
  CHECK(points_on.Peek() == points_off.Peek());
  CheckArrays(on, off);
  const auto last_cell = rows_on.rbegin()->first;
  local::IdSequence cells_on(last_cell), cells_off(last_cell);
  on.WriteBack(rows_on, cells_on);
  off.WriteBack(rows_off, cells_off);
  REQUIRE(rows_on.size() == rows_off.size());
  for (const auto& [id, row] : rows_on) {
    CAPTURE(id);
    REQUIRE(rows_off.count(id) == 1);
    REQUIRE(WireBytes(row) == WireBytes(rows_off.at(id)));
  }
  std::cout << "FP1_FIXTURE resume_memory skipped_on=" << stats_on.skipped_memory
            << " skipped_off=" << stats_off.skipped_memory << " bytes_per_cell=" << local::Mesh::BytesPerCell
            << " points=" << on.Points().size() << " cell_slots=" << on.Cells().size() << " rows=" << rows_on.size()
            << '\n';
}
