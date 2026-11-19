/*!
 * \file CNativeLocalCore3DChanges.cpp
 * \brief Fixed-cavity collapse, split, reconnection and vertex movement.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../include/adaptation/CNativeMetricTransport3D.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace SU2NativeLocal3D {
namespace {
bool Contains(Vertices v, Slot p) { return std::find(v.begin(), v.end(), p) != v.end(); }

Vertices Replace(Vertices vertices, Slot from, Slot to) {
  *std::find(vertices.begin(), vertices.end(), from) = to;
  return vertices;
}

Tensor Midpoint(const Tensor& a, const Tensor& b) {
  return {0.5 * (a.xx + b.xx), 0.5 * (a.xy + b.xy), 0.5 * (a.xz + b.xz),
          0.5 * (a.yy + b.yy), 0.5 * (a.yz + b.yz), 0.5 * (a.zz + b.zz)};
}
}  // namespace

void PassStats::Count(Refusal refusal) {
  if (!report) return;
  switch (refusal) {
    case Refusal::Validity:
      ++refused_valid;
      break;
    case Refusal::Size:
      ++refused_size;
      break;
    case Refusal::Topology:
      ++refused_topology;
      break;
    case Refusal::Memory:
      ++refused_memory;
      break;
    case Refusal::None:
    case Refusal::Ineligible:
      break;
  }
}

Edge Mesh::ById(Edge edge) const {
  if (points_[edge[1]].node.id < points_[edge[0]].node.id) std::swap(edge[0], edge[1]);
  return edge;
}

std::array<Id, 4> Mesh::Key(Vertices vertices) const {
  std::array<Id, 4> ids;
  for (unsigned k = 0; k < 4; ++k) ids[k] = points_[vertices[k]].node.id;
  std::sort(ids.begin(), ids.end());
  return ids;
}

std::vector<Slot> Mesh::Star(Slot point) const {
  const auto start = points_[point].incident;
  if (start == NoSlot) return {};
  std::vector<Slot> star{start};
  for (size_t i = 0; i < star.size(); ++i) {
    const auto& cell = cells_[star[i]];
    for (unsigned k = 0; k < 4; ++k) {
      const auto n = cell.neighbors[k];
      if (cell.vertices[k] == point || n < 0) continue;
      const auto neighbor = static_cast<Slot>(n);
      if (std::find(star.begin(), star.end(), neighbor) == star.end()) star.push_back(neighbor);
    }
  }
  std::sort(star.begin(), star.end(),
            [this](Slot a, Slot b) { return Key(cells_[a].vertices) < Key(cells_[b].vertices); });
  return star;
}

Mesh::Ring Mesh::EdgeRing(Edge edge, Slot seed) const {
  Ring ring;
  const auto& start = cells_[seed];
  if (!start.live || !Contains(start.vertices, edge[0]) || !Contains(start.vertices, edge[1])) return ring;
  ring.cells.push_back(seed);
  for (size_t i = 0; i < ring.cells.size(); ++i) {
    const auto& cell = cells_[ring.cells[i]];
    for (unsigned k = 0; k < 4; ++k) {
      if (cell.vertices[k] == edge[0] || cell.vertices[k] == edge[1]) continue;
      const auto n = cell.neighbors[k];
      if (n == Physical) ring.surface = true;
      if (n == Border) ring.border = true;
      if (n < 0) continue;
      const auto neighbor = static_cast<Slot>(n);
      if (std::find(ring.cells.begin(), ring.cells.end(), neighbor) == ring.cells.end()) ring.cells.push_back(neighbor);
    }
  }
  return ring;
}

bool Mesh::ClosedRing(Edge edge, Slot seed) const { return EdgeRing(edge, seed).Closed(); }

std::vector<Slot> Mesh::OrderedRim(Edge edge, const Ring& ring) const {
  std::vector<Edge> rim_edges;
  Slot first = NoSlot;
  for (auto slot : ring.cells) {
    Edge rim{};
    unsigned n = 0;
    for (auto p : cells_[slot].vertices) {
      if (p == edge[0] || p == edge[1]) continue;
      rim[n++] = p;
      if (first == NoSlot || points_[p].node.id < points_[first].node.id) first = p;
    }
    if (!Positive({edge[0], edge[1], rim[0], rim[1]})) std::swap(rim[0], rim[1]);
    rim_edges.push_back(rim);
  }
  std::vector<Slot> result{first};
  for (size_t i = 0; i < rim_edges.size(); ++i) {
    const auto next = std::find_if(rim_edges.begin(), rim_edges.end(), [&](Edge e) { return e[0] == result.back(); });
    if (next == rim_edges.end()) return {};
    if (next->at(1) == first) return result.size() == rim_edges.size() ? result : std::vector<Slot>{};
    result.push_back(next->at(1));
  }
  return {};
}

void Mesh::MarkWallNear() {
  for (auto& point : points_) point.wall_near = false;
  for (const auto& row : cells_) {
    if (!row.live ||
        std::none_of(row.vertices.begin(), row.vertices.end(), [this](Slot p) { return points_[p].surface; }))
      continue;
    for (auto p : row.vertices)
      if (points_[p].Free()) points_[p].wall_near = true;
  }
}

bool Mesh::CanCollapse(Slot point) const { return points_[point].Free() && !points_[point].wall_near; }

bool Mesh::Fits(size_t old, size_t fresh, size_t cell_limit) const {
  return fresh <= old || (live_cells_ <= cell_limit && fresh - old <= cell_limit - live_cells_);
}

Mesh::Plan Mesh::CollapsePlan(Slot remove, Slot keep) const {
  if (!CanCollapse(remove)) return {};
  const auto old = Star(remove);
  std::vector<Vertices> fresh;
  std::vector<Slot> incident;
  for (auto c : old) {
    const auto vertices = cells_[c].vertices;
    if (Contains(vertices, keep)) {
      incident.insert(incident.end(), vertices.begin(), vertices.end());
    } else {
      fresh.push_back(Replace(vertices, remove, keep));
    }
  }
  if (incident.empty()) return {};
  auto plan = Prepare(old, fresh);
  if (!plan.Accepted()) return plan;

  // An existing edge used by a valid replacement is in the old cavity or on its fixed boundary.
  for (const auto& vertices : fresh) {
    for (auto p : vertices) {
      if (std::find(incident.begin(), incident.end(), p) != incident.end()) continue;
      plan.score = std::max(plan.score, Length({keep, p}));
    }
  }
  if (plan.score > LongEdge) plan.refusal = Refusal::Size;
  return plan;
}

Refusal Mesh::Collapse(Edge edge, uint32_t stamp, PassStats& stats, bool apply) {
  edge = ById(edge);
  if (points_[edge[0]].removed || points_[edge[1]].removed || Length(edge) >= ShortEdge) return Refusal::Ineligible;
  auto lower = CollapsePlan(edge[0], edge[1]);
  auto higher = CollapsePlan(edge[1], edge[0]);
  stats.Count(lower.refusal);
  stats.Count(higher.refusal);
  const auto& best = lower.Accepted() && (!higher.Accepted() || lower.score < higher.score) ? lower : higher;
  if (!best.Accepted()) return best.refusal == Refusal::Ineligible ? lower.refusal : best.refusal;
  if (apply) {
    Commit(best, stamp);
    stats.Increment(stats.collapses);
  }
  return Refusal::None;
}

Refusal Mesh::Split(Edge edge, Slot seed, IdSequence& point_ids, uint32_t stamp, size_t cell_limit, PassStats& stats) {
  edge = ById(edge);
  auto ring = EdgeRing(edge, seed);
  if (!ring.Closed() || Length(edge) <= LongEdge) return Refusal::Ineligible;
  if (!Fits(ring.cells.size(), 2 * ring.cells.size(), cell_limit)) {
    stats.Count(Refusal::Memory);
    return Refusal::Memory;
  }
  std::sort(ring.cells.begin(), ring.cells.end(),
            [this](Slot a, Slot b) { return Key(cells_[a].vertices) < Key(cells_[b].vertices); });
  const auto& a = points_[edge[0]];
  const auto& b = points_[edge[1]];
  const Point p{0.5 * (a.node.p.x + b.node.p.x), 0.5 * (a.node.p.y + b.node.p.y), 0.5 * (a.node.p.z + b.node.p.z)};
  const PointRow middle{{point_ids.Peek(), p}, Midpoint(a.tensor, b.tensor)};
  if (free_points_.empty() && points_.size() == NoSlot) throw std::length_error("Too many native local core points.");
  const bool append = free_points_.empty();
  const auto slot = append ? static_cast<Slot>(points_.size()) : free_points_.back();
  std::vector<Vertices> fresh;
  fresh.reserve(2 * ring.cells.size());
  for (auto c : ring.cells) {
    fresh.push_back(Replace(cells_[c].vertices, edge[0], slot));
    fresh.push_back(Replace(cells_[c].vertices, edge[1], slot));
  }
  PointRow previous;
  if (append) {
    points_.push_back(middle);
  } else {
    previous = points_[slot];
    points_[slot] = middle;
  }
  const auto plan = Prepare(ring.cells, fresh);
  stats.Count(plan.refusal);
  if (!plan.Accepted()) {
    if (append)
      points_.pop_back();
    else
      points_[slot] = previous;
    return plan.refusal;
  }
  if (!append) free_points_.pop_back();
  point_ids.Take();
  Commit(plan, stamp);
  stats.Increment(stats.splits);
  return Refusal::None;
}

double Mesh::Worst(const std::vector<Slot>& cells) const {
  double margin = std::numeric_limits<double>::infinity();
  for (auto slot : cells) margin = std::min(margin, Margin(cells_[slot].vertices));
  return margin;
}

Mesh::Plan Mesh::RemoveEdgePlan(Edge edge, Slot seed, size_t cell_limit) const {
  edge = ById(edge);
  const auto ring = EdgeRing(edge, seed);
  const auto n = ring.cells.size();
  if (!ring.Closed() || n < 3 || n > 7) return {};
  Plan refused;
  if (!Fits(n, 2 * (n - 2), cell_limit)) {
    refused.refusal = Refusal::Memory;
    return refused;
  }
  const auto rim = OrderedRim(edge, ring);
  if (rim.size() != n) {
    refused.refusal = Refusal::Topology;
    return refused;
  }
  const auto pair = [&](size_t i, size_t k, size_t j) -> std::array<Vertices, 2> {
    return {{{rim[i], rim[k], rim[j], edge[1]}, {rim[k], rim[i], rim[j], edge[0]}}};
  };
  const double infinity = std::numeric_limits<double>::infinity();
  double score[7][7]{};
  size_t middle[7][7]{};
  bool too_long[7][7]{};
  for (size_t i = 0; i + 1 < n; ++i) {
    score[i][i + 1] = infinity;
    for (size_t j = i + 2; j < n; ++j)
      if (i != 0 || j != n - 1) too_long[i][j] = Length({rim[i], rim[j]}) > LongEdge;
  }
  bool invalid = false, long_diagonal = false;
  for (size_t width = 2; width < n; ++width) {
    for (size_t i = 0, j = width; j < n; ++i, ++j) {
      score[i][j] = -infinity;
      for (size_t k = i + 1; k < j; ++k) {
        if (too_long[i][k] || too_long[k][j] || too_long[i][j]) {
          long_diagonal = true;
          continue;
        }
        const auto trial = pair(i, k, j);
        if (!Positive(trial[0]) || !Positive(trial[1])) {
          invalid = true;
          continue;
        }
        const auto quality = std::min({score[i][k], score[k][j], Margin(trial[0]), Margin(trial[1])});
        if (quality <= score[i][j]) continue;
        score[i][j] = quality;
        middle[i][j] = k;
      }
    }
  }
  if (score[0][n - 1] == -infinity) {
    refused.refusal = invalid ? Refusal::Validity : (long_diagonal ? Refusal::Size : Refusal::Topology);
    return refused;
  }
  if (score[0][n - 1] <= Worst(ring.cells)) return {};
  std::vector<Vertices> fresh;
  const auto collect = [&](auto&& self, size_t i, size_t j) -> void {
    if (j - i < 2) return;
    const auto k = middle[i][j];
    const auto cells = pair(i, k, j);
    fresh.insert(fresh.end(), cells.begin(), cells.end());
    self(self, i, k);
    self(self, k, j);
  };
  collect(collect, 0, n - 1);
  auto plan = Prepare(ring.cells, fresh);
  plan.score = score[0][n - 1];
  return plan;
}

Mesh::Plan Mesh::SwapFacePlan(Slot cell, unsigned face, size_t cell_limit) const {
  const auto& a = cells_[cell];
  if (!a.live || a.neighbors[face] < 0) return {};
  const auto neighbor = static_cast<Slot>(a.neighbors[face]);
  const auto& b = cells_[neighbor];
  std::array<Slot, 3> rim{};
  for (unsigned k = 0, l = 0; k < 4; ++k)
    if (k != face) rim[l++] = a.vertices[k];
  std::sort(rim.begin(), rim.end(), [this](Slot x, Slot y) { return points_[x].node.id < points_[y].node.id; });
  auto p = a.vertices[face];
  auto q =
      b.vertices[std::find(b.neighbors.begin(), b.neighbors.end(), static_cast<int32_t>(cell)) - b.neighbors.begin()];
  if (!Positive({rim[0], rim[1], rim[2], p})) std::swap(p, q);
  Plan refused;
  if (Length({p, q}) > LongEdge) {
    refused.refusal = Refusal::Size;
    return refused;
  }
  if (!Fits(2, 3, cell_limit)) {
    refused.refusal = Refusal::Memory;
    return refused;
  }
  const std::vector<Vertices> fresh{{q, p, rim[0], rim[1]}, {q, p, rim[1], rim[2]}, {q, p, rim[2], rim[0]}};
  const std::vector<Slot> old{cell, neighbor};
  auto plan = Prepare(old, fresh);
  if (!plan.Accepted()) return plan;
  plan.score = std::min({Margin(fresh[0]), Margin(fresh[1]), Margin(fresh[2])});
  if (plan.score <= Worst(old)) return {};
  return plan;
}

Refusal Mesh::RemoveEdge(Edge edge, Slot seed, uint32_t stamp, size_t cell_limit, PassStats& stats) {
  const auto plan = RemoveEdgePlan(edge, seed, cell_limit);
  stats.Count(plan.refusal);
  if (plan.Accepted()) {
    Commit(plan, stamp);
    stats.Increment(stats.edge_removals);
  }
  return plan.refusal;
}

Refusal Mesh::SwapFace(Slot cell, unsigned face, uint32_t stamp, size_t cell_limit, PassStats& stats) {
  const auto plan = SwapFacePlan(cell, face, cell_limit);
  stats.Count(plan.refusal);
  if (plan.Accepted()) {
    Commit(plan, stamp);
    stats.Increment(stats.face_swaps);
  }
  return plan.refusal;
}

Refusal Mesh::MovePoint(Slot point, IdSequence& ids, uint32_t stamp, PassStats& stats) {
  if (!points_[point].Free()) return Refusal::Ineligible;
  const auto star = Star(point);
  if (star.empty()) return Refusal::Ineligible;
  const double before = Worst(star);
  std::vector<Slot> neighbors;
  std::vector<SU2Native3D::DonorCell> donors;
  donors.reserve(star.size());
  for (auto c : star) {
    SU2Native3D::DonorCell donor;
    donor.cell.id = cells_[c].id;
    for (unsigned k = 0; k < 4; ++k) {
      const auto p = cells_[c].vertices[k];
      donor.cell.v[k] = points_[p].node;
      donor.sensor[k] = points_[p].tensor;
      if (p != point) neighbors.push_back(p);
    }
    donors.push_back(donor);
  }
  std::sort(neighbors.begin(), neighbors.end(),
            [this](Slot a, Slot b) { return points_[a].node.id < points_[b].node.id; });
  neighbors.erase(std::unique(neighbors.begin(), neighbors.end()), neighbors.end());
  std::array<long double, 3> mean{};
  for (auto n : neighbors) {
    const auto p = points_[n].node.p;
    mean[0] += p.x;
    mean[1] += p.y;
    mean[2] += p.z;
  }
  for (auto& value : mean) value /= neighbors.size();

  const auto original = points_[point];
  const auto p = original.node.p;
  auto refusal = Refusal::Ineligible;
  for (long double step : {1.L, .5L, .25L}) {
    auto candidate = original;
    candidate.node.p = {double(p.x + step * (mean[0] - p.x)), double(p.y + step * (mean[1] - p.y)),
                        double(p.z + step * (mean[2] - p.z))};
    candidate.tensor = SU2Native3D::EvaluateCavitySample(donors, candidate.node.p).sensor;
    points_[point] = candidate;
    try {
      if (std::any_of(star.begin(), star.end(), [&](Slot c) { return !Positive(cells_[c].vertices); }))
        refusal = Refusal::Validity;
      else if (std::any_of(neighbors.begin(), neighbors.end(), [&](Slot n) { return Length({point, n}) > LongEdge; }))
        refusal = Refusal::Size;
      else
        refusal = Worst(star) > before ? Refusal::None : Refusal::Ineligible;
    } catch (...) {
      points_[point] = original;
      throw;
    }
    points_[point] = original;
    if (refusal == Refusal::None) {
      candidate.node.id = ids.Take();
      points_[point] = candidate;
      for (auto c : star) MarkChanged(cells_[c], stamp);
      stats.Increment(stats.moves);
      return Refusal::None;
    }
    stats.Count(refusal);
  }
  return refusal;
}
}  // namespace SU2NativeLocal3D
