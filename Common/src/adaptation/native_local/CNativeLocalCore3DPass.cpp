/*!
 * \file CNativeLocalCore3DPass.cpp
 * \brief Deterministic length and quality sweeps over the local tetrahedral arrays.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include <algorithm>
#include <tuple>
#include <unordered_set>

namespace SU2NativeLocal3D {
namespace {
constexpr unsigned LengthSweepLimit = 50, LengthRoundLimit = 8, QualitySweepLimit = 20;

struct LengthCandidate {
  double order;
  std::array<Id, 2> ids;
  EdgeUse use;

  bool operator<(const LengthCandidate& other) const { return std::tie(order, ids) < std::tie(other.order, other.ids); }
};

struct QualityCandidate {
  double margin;
  std::array<Id, 4> ids;
  Slot cell;

  bool operator<(const QualityCandidate& other) const {
    return std::tie(margin, ids) < std::tie(other.margin, other.ids);
  }
};

template <class Action>
void EachEdge(const Vertices& vertices, Action action) {
  for (unsigned i = 0; i < 4; ++i)
    for (unsigned j = i + 1; j < 4; ++j) action(Edge{vertices[i], vertices[j]});
}
}  // namespace

// Four points, six edge uses and four faces per cell bound even a rank owning disjoint tetrahedra.
// Include sort candidates, reversible cavity links and node/bucket overhead of the temporary lookup tables.
const size_t Mesh::BytesPerCell = transfer_memory::GrowthBound(
    1, sizeof(CellRow) + sizeof(std::array<FacetBinding, 4>) + sizeof(Id) + 3 * sizeof(Slot) +
           4 * (sizeof(PointRow) + sizeof(FrozenPoint) + sizeof(Id) + 5 * sizeof(void*) + sizeof(uint32_t)) +
           6 * (sizeof(EdgeUse) + sizeof(LengthCandidate) + sizeof(uint64_t) + 4 * sizeof(void*)) +
           8 * (sizeof(FreshCell) + sizeof(Face) + 6 * sizeof(void*)) + sizeof(QualityCandidate));

size_t Mesh::ArrayBytes(size_t cells) { return transfer_memory::Mul(cells, BytesPerCell); }

std::vector<EdgeUse> Mesh::Edges() const {
  std::unordered_set<uint64_t> seen;
  std::vector<EdgeUse> result;
  for (Slot cell = 0; cell < cells_.size(); ++cell) {
    const auto& row = cells_[cell];
    if (!row.live) continue;
    EachEdge(row.vertices, [&](Edge edge) {
      const uint64_t key = (uint64_t(std::min(edge[0], edge[1])) << 32) | std::max(edge[0], edge[1]);
      if (seen.insert(key).second) result.push_back({ById(edge), cell});
    });
  }
  return result;
}

size_t Mesh::CollapseSweeps(PassStats& stats) {
  size_t total = 0;
  for (unsigned sweep = 0; sweep < LengthSweepLimit; ++sweep) {
    stats.Increment(stats.sweeps_collapse);
    ++sweep_;
    MarkWallNear();
    size_t accepted = 0;
    std::vector<LengthCandidate> candidates;
    for (const auto& use : Edges()) {
      const auto edge = use.edge;
      if (!CanCollapse(edge[0]) && !CanCollapse(edge[1])) continue;
      const auto length = Length(edge);
      if (length < ShortEdge) candidates.push_back({length, {points_[edge[0]].node.id, points_[edge[1]].node.id}, use});
    }
    std::sort(candidates.begin(), candidates.end());
    for (const auto& candidate : candidates) {
      const auto edge = candidate.use.edge;
      if (points_[edge[0]].removed || points_[edge[1]].removed) continue;
      bool stamped = false;
      for (auto p : edge) {
        if (!CanCollapse(p)) continue;
        const auto star = Star(p);
        stamped |= std::any_of(star.begin(), star.end(), [this](Slot c) { return cells_[c].stamp == sweep_; });
      }
      if (stamped) {
        stats.Increment(stats.deferred);
      } else if (Collapse(edge, sweep_, stats) == Refusal::None) {
        ++accepted;
      }
    }
    if (!accepted) return total;
    total += accepted;
  }
  if (stats.report) stats.limit = true;
  return total;
}

size_t Mesh::SplitSweeps(IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  size_t total = 0;
  for (unsigned sweep = 0; sweep < LengthSweepLimit; ++sweep) {
    stats.Increment(stats.sweeps_split);
    ++sweep_;
    size_t accepted = 0;
    std::vector<LengthCandidate> candidates;
    for (const auto& use : Edges()) {
      const auto edge = use.edge;
      const auto length = Length(edge);
      if (length > LongEdge && ClosedRing(edge, use.cell))
        candidates.push_back({-length, {points_[edge[0]].node.id, points_[edge[1]].node.id}, use});
    }
    std::sort(candidates.begin(), candidates.end());
    std::vector<bool> stamped(points_.size());
    for (const auto& candidate : candidates) {
      const auto edge = candidate.use.edge;
      if (stamped[edge[0]] || stamped[edge[1]]) {
        stats.Increment(stats.deferred);
        continue;
      }
      const auto ring = EdgeRing(edge, candidate.use.cell);
      std::vector<Slot> touched;
      for (auto c : ring.cells) touched.insert(touched.end(), cells_[c].vertices.begin(), cells_[c].vertices.end());
      if (Split(edge, candidate.use.cell, point_ids, sweep_, cell_limit, stats) == Refusal::None) {
        ++accepted;
        for (auto p : touched) stamped[p] = true;
      }
    }
    if (!accepted) return total;
    total += accepted;
  }
  if (stats.report) stats.limit = true;
  return total;
}

void Mesh::LengthWork(IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  for (unsigned round = 0; round < LengthRoundLimit; ++round) {
    stats.Increment(stats.rounds);
    const auto collapses = CollapseSweeps(stats);
    const auto splits = SplitSweeps(point_ids, cell_limit, stats);
    if (!collapses && !splits) return;
  }
  if (stats.report) stats.limit = true;
}

bool Mesh::QualitySweep(IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  stats.Increment(stats.sweeps_quality);
  ++sweep_;
  bool changed = false;
  std::vector<QualityCandidate> candidates;
  for (Slot cell = 0; cell < cells_.size(); ++cell) {
    if (!cells_[cell].live) continue;
    const auto vertices = cells_[cell].vertices;
    const auto margin = Margin(vertices);
    if (margin < BadMargin) candidates.push_back({margin, Key(vertices), cell});
  }
  std::sort(candidates.begin(), candidates.end());
  for (const auto& candidate : candidates) {
    const auto cell = candidate.cell;
    const auto& row = cells_[cell];
    if (!row.live || row.stamp == sweep_) continue;
    const auto stable_order = [this](Slot a, Slot b) { return points_[a].node.id < points_[b].node.id; };
    auto vertices = row.vertices;
    std::sort(vertices.begin(), vertices.end(), stable_order);
    Plan best;
    bool best_edge = false;
    EachEdge(vertices, [&](Edge edge) {
      auto plan = RemoveEdgePlan(edge, cell, cell_limit);
      stats.Count(plan.refusal);
      if (plan.Accepted() && (!best.Accepted() || plan.score > best.score)) {
        best = std::move(plan);
        best_edge = true;
      }
    });
    // The faces opposite vertices in reverse id order enumerate sorted face keys lexicographically.
    for (auto it = vertices.rbegin(); it != vertices.rend(); ++it) {
      const unsigned face = std::find(row.vertices.begin(), row.vertices.end(), *it) - row.vertices.begin();
      auto plan = SwapFacePlan(cell, face, cell_limit);
      stats.Count(plan.refusal);
      if (plan.Accepted() && (!best.Accepted() || plan.score > best.score)) {
        best = std::move(plan);
        best_edge = false;
      }
    }
    if (best.Accepted()) {
      Commit(best, sweep_);
      changed = true;
      if (best_edge)
        stats.Increment(stats.edge_removals);
      else
        stats.Increment(stats.face_swaps);
    } else {
      for (auto p : vertices) {
        if (MovePoint(p, point_ids, sweep_, stats) != Refusal::None) continue;
        changed = true;
        break;
      }
    }
  }
  return changed;
}

void Mesh::QualityWork(IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  for (unsigned sweep = 0; sweep < QualitySweepLimit; ++sweep)
    if (!QualitySweep(point_ids, cell_limit, stats)) return;
  if (stats.report) stats.limit = true;
}

void Mesh::ReportResult(PassStats& stats) const {
  stats.cells_out = live_cells_;
  for (const auto& point : points_) {
    if (point.removed) continue;
    ++stats.points_out;
    stats.surface_points += point.surface;
    stats.border_points += point.border;
    stats.wall_near_points += point.wall_near;
  }
  for (const auto& row : cells_) {
    if (!row.live || Margin(row.vertices) >= BadMargin) continue;
    ++stats.bad_out;
    if (std::all_of(row.vertices.begin(), row.vertices.end(), [this](Slot p) { return points_[p].Free(); }))
      ++stats.bad_out_free;
  }
  for (const auto& use : Edges()) {
    ++stats.edges_out;
    const auto length = Length(use.edge);
    stats.short_out += length < ShortEdge;
    if (length > LongEdge) {
      const auto ring = EdgeRing(use.edge, use.cell);
      stats.long_left_surface += ring.surface;
      stats.long_left_border += ring.border;
    }
  }
}

PassStats Mesh::RunPass(IdSequence& point_ids, size_t cell_limit, bool report) {
  PassStats stats;
  stats.report = report;
  if (report) {
    stats.cells_in = live_cells_;
    stats.points_in = points_.size();
    for (const auto& row : cells_)
      if (row.live && Margin(row.vertices) < BadMargin) ++stats.bad_in;
  }
  LengthWork(point_ids, cell_limit, stats);
  QualityWork(point_ids, cell_limit, stats);
  Check();
  if (report) {
    MarkWallNear();
    ReportResult(stats);
  }
  return stats;
}
}  // namespace SU2NativeLocal3D
