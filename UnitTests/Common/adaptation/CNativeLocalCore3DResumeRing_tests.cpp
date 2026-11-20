/*!
 * \file CNativeLocalCore3DResumeRing_tests.cpp
 * \brief Exhaustive triangulation controls for native local edge removal.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeLocalCore3D.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

namespace {
using namespace SU2Native3D;
namespace local = SU2NativeLocal3D;
using Diagonal = std::array<size_t, 2>;

constexpr double NoScore = -std::numeric_limits<double>::infinity();

Owned IrregularRing(size_t n, double scale) {
  constexpr std::array<double, 7> offsets{0., .13, -.09, .05, -.06, .03, -.11};
  const auto turn = 2 * std::acos(-1.);
  std::vector<Node> rim;
  for (size_t i = 0; i < n; ++i) {
    const auto angle = turn * i / n + offsets[i];
    rim.push_back({100 + i, {.68 * std::cos(angle), .68 * std::sin(angle), 0.}});
  }
  // An axis just inside the first side makes one original wedge thin without degenerating the rim.
  const auto x = (rim[0].p.x + rim[1].p.x) * (511. / 1024.);
  const auto y = (rim[0].p.y + rim[1].p.y) * (511. / 1024.);
  const Node a{10, {x, y, -.72}}, b{20, {x, y, .54}};
  Owned rows;
  for (size_t i = 0; i < n; ++i) {
    OwnedCell row;
    row.cell = {1000 + 11 * (n - i), {a, b, rim[i], rim[(i + 1) % n]}};
    REQUIRE(Orientation(a.p, b.p, rim[i].p, rim[(i + 1) % n].p) > 0);
    row.physical[0] = {true, 500 + 2 * i};
    row.physical[1] = {true, 501 + 2 * i};
    row.sensor_present = true;
    for (size_t j = 0; j < row.sensor.size(); ++j) {
      const auto p = row.cell.v[j].p;
      row.sensor[j] = {scale * (1 + .1 * p.x), scale * .05 * p.y, 0,
                       scale * (1 + .1 * p.y), 0, scale * (1 + .1 * p.z)};
    }
    rows.emplace(row.cell.id, row);
  }
  return rows;
}

local::Slot PointSlot(const local::Mesh& mesh, Id id) {
  const auto& points = mesh.Points();
  const auto found = std::find_if(points.begin(), points.end(), [id](const auto& p) { return p.node.id == id; });
  REQUIRE(found != points.end());
  return static_cast<local::Slot>(found - points.begin());
}

bool Crossing(Diagonal a, Diagonal b) {
  return (a[0] < b[0] && b[0] < a[1] && a[1] < b[1]) || (b[0] < a[0] && a[0] < b[1] && b[1] < a[1]);
}

double TrialMargin(const local::Mesh& mesh, local::Vertices cell) {
  const auto p = [&](size_t i) { return mesh.Points()[cell[i]].node.p; };
  return Orientation(p(0), p(1), p(2), p(3)) > 0 ? mesh.Margin(cell) : NoScore;
}

double TriangulationMargin(const local::Mesh& mesh, local::Edge edge, const std::vector<local::Slot>& rim,
                           const std::vector<Diagonal>& diagonals) {
  const auto n = rim.size();
  bool connected[7][7]{};
  connected[0][n - 1] = true;
  for (size_t i = 0; i + 1 < n; ++i) connected[i][i + 1] = true;
  for (const auto& [i, j] : diagonals) {
    if (mesh.Length({rim[i], rim[j]}) > local::LongEdge) return NoScore;
    connected[i][j] = true;
  }
  double result = std::numeric_limits<double>::infinity();
  size_t triangles = 0;
  for (size_t i = 0; i + 2 < n; ++i)
    for (size_t j = i + 1; j + 1 < n; ++j)
      for (size_t k = j + 1; k < n; ++k) {
        if (!connected[i][j] || !connected[j][k] || !connected[i][k]) continue;
        ++triangles;
        result = std::min({result, TrialMargin(mesh, {rim[i], rim[j], rim[k], edge[1]}),
                           TrialMargin(mesh, {rim[j], rim[i], rim[k], edge[0]})});
      }
  CHECK(triangles == n - 2);
  return result;
}

double BruteForce(const local::Mesh& mesh, local::Edge edge, const std::vector<local::Slot>& rim) {
  const auto n = rim.size();
  std::vector<Diagonal> candidates;
  for (size_t i = 0; i + 2 < n; ++i)
    for (size_t j = i + 2; j < n; ++j)
      if (i != 0 || j + 1 != n) candidates.push_back({i, j});

  // In a convex n-gon, every set of n-3 pairwise noncrossing diagonals defines one triangulation.
  constexpr std::array<size_t, 4> expected_count{2, 5, 14, 42};
  size_t triangulations = 0;
  double best = NoScore;
  for (unsigned mask = 0; mask < (1u << candidates.size()); ++mask) {
    const std::bitset<14> chosen(mask);
    if (chosen.count() != n - 3) continue;
    std::vector<Diagonal> diagonals;
    for (size_t i = 0; i < candidates.size(); ++i)
      if (chosen[i]) diagonals.push_back(candidates[i]);
    bool crossing = false;
    for (size_t i = 0; i < diagonals.size(); ++i)
      for (size_t j = i + 1; j < diagonals.size(); ++j) crossing |= Crossing(diagonals[i], diagonals[j]);
    if (crossing) continue;
    ++triangulations;
    best = std::max(best, TriangulationMargin(mesh, edge, rim, diagonals));
  }
  CHECK(triangulations == expected_count[n - 4]);
  return best;
}

double WorstMargin(const local::Mesh& mesh) {
  double worst = std::numeric_limits<double>::infinity();
  for (const auto& cell : mesh.Cells())
    if (cell.live) worst = std::min(worst, mesh.Margin(cell.vertices));
  return worst;
}
}  // namespace

TEST_CASE("Native local edge removal matches exhaustive irregular ring triangulations",
          "[.][NativeLocalCore3DResumeRing]") {
  const size_t n = GENERATE(4u, 5u, 6u, 7u);
  const double scale = GENERATE(1., 9.);
  CAPTURE(n, scale);
  local::Mesh mesh(IrregularRing(n, scale));
  const local::Edge edge{PointSlot(mesh, 10), PointSlot(mesh, 20)};
  REQUIRE(mesh.ClosedRing(edge, 0));
  std::vector<local::Slot> rim;
  for (size_t i = 0; i < n; ++i) rim.push_back(PointSlot(mesh, 100 + i));
  const auto before = WorstMargin(mesh);
  CHECK(before < local::BadMargin);
  const auto best = BruteForce(mesh, edge, rim);
  if (scale == 1.)
    CHECK(best > before);
  else
    CHECK(best == NoScore);

  local::PassStats stats;
  const auto outcome = mesh.RemoveEdge(edge, 0, 7, 64, stats);
  if (best > before) {
    REQUIRE(outcome == local::Refusal::None);
    CHECK(stats.edge_removals == 1);
    CHECK(std::count_if(mesh.Cells().begin(), mesh.Cells().end(), [](const auto& cell) { return cell.live; }) ==
          static_cast<ptrdiff_t>(2 * (n - 2)));
    CHECK(WorstMargin(mesh) == best);
  } else {
    CHECK(outcome != local::Refusal::None);
    CHECK(stats.edge_removals == 0);
    CHECK(WorstMargin(mesh) == before);
  }
  REQUIRE_NOTHROW(mesh.Check());
  std::cout << "FP1_RESUME_RING n=" << n << " tensor_scale=" << scale << " old_margin=" << before
            << " oracle_margin=" << best << " final_margin=" << WorstMargin(mesh) << '\n';
}
