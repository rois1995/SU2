/*!
 * \file CNativeLocalCore3DPass.cpp
 * \brief Deterministic length and quality sweeps over the local tetrahedral arrays.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include <algorithm>
#include <ostream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

namespace SU2NativeLocal3D {
namespace {
constexpr unsigned LengthSweepLimit = 50, LengthRoundLimit = 8, QualitySweepLimit = 20;

double ReportClock(const PassStats& stats) { return stats.report ? SU2_MPI::Wtime() : 0.0; }

constexpr std::pair<const char*, size_t SweepCounters::*> SweepFields[] = {
    {"evaluated_collapse", &SweepCounters::evaluated_collapse},
    {"evaluated_split", &SweepCounters::evaluated_split},
    {"evaluated_quality", &SweepCounters::evaluated_quality},
    {"deferred", &SweepCounters::deferred}, {"skipped_memory", &SweepCounters::skipped_memory},
    {"blocked", &SweepCounters::blocked}, {"collapses", &SweepCounters::collapses}, {"splits", &SweepCounters::splits},
    {"edge_removals", &SweepCounters::edge_removals}, {"face_swaps", &SweepCounters::face_swaps},
    {"moves", &SweepCounters::moves}, {"refused_valid", &SweepCounters::refused_valid},
    {"refused_size", &SweepCounters::refused_size}, {"refused_topology", &SweepCounters::refused_topology},
    {"refused_memory", &SweepCounters::refused_memory}, {"refused_numeric", &SweepCounters::refused_numeric},
    {"refused_shape", &SweepCounters::refused_shape}, {"refused_exact", &SweepCounters::refused_exact}};

struct SweepWatch {
  explicit SweepWatch(const PassStats& stats) {
    if (stats.report && stats.sink) {
      before = stats;
      start = ReportClock(stats);
    }
  }

  void Emit(const PassStats& stats, const char* kind, unsigned round, unsigned sweep, size_t candidates,
            size_t live_cells, size_t bad_listed = 0) const {
    if (!stats.report || !stats.sink) return;
    SweepRow row{kind, round, sweep, candidates, live_cells, bad_listed, ReportClock(stats) - start, {}};
    for (const auto& field : SweepFields) row.counters.*field.second = stats.*field.second - before.*field.second;
    stats.sink(row);
  }

  SweepCounters before;
  double start = 0;
};

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

void PrintSweepFields(std::ostream& output, const SweepRow& row) {
  output << " kind=" << row.kind << " round=" << row.round << " sweep=" << row.sweep
         << " candidates=" << row.candidates << " live_cells=" << row.live_cells << " bad_listed=" << row.bad_listed;
  for (const auto& field : SweepFields) output << ' ' << field.first << '=' << row.counters.*field.second;
  output << " seconds=" << row.seconds;
}

// Four points, six edge uses and four faces per cell bound even a rank owning disjoint tetrahedra.
// Include sort candidates, reversible cavity links and node/bucket overhead of the temporary lookup tables.
const size_t Mesh::BytesPerCell = transfer_memory::GrowthBound(
    1, sizeof(CellRow) + sizeof(std::array<FacetBinding, 4>) + sizeof(Id) + 3 * sizeof(Slot) +
           4 * (sizeof(PointRow) + sizeof(FrozenPoint) + sizeof(Id) + 5 * sizeof(void*) + sizeof(uint32_t)) +
           6 * (sizeof(EdgeUse) + sizeof(LengthCandidate) + sizeof(uint64_t) + 4 * sizeof(void*) +
                sizeof(Edge) + sizeof(Slot)) + 4 * sizeof(Slot) +
           6 * (2 * sizeof(Id) + sizeof(uint64_t) + 4 * sizeof(void*)) +
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

void Mesh::RememberWorkPoints(const Plan& plan) {
  if (!work_start_) return;
  std::vector<Slot> touched;
  const auto add = [&](Vertices vertices) {
    for (auto point : vertices)
      if (points_[point].changed_at <= work_start_) touched.push_back(point);
  };
  for (auto slot : plan.old) add(cells_[slot].vertices);
  for (const auto& row : plan.fresh) add(row.vertices);
  std::sort(touched.begin(), touched.end());
  work_points_.insert(work_points_.end(), touched.begin(), std::unique(touched.begin(), touched.end()));
}

std::vector<EdgeUse> Mesh::NextEdges(const std::vector<Edge>& deferred) {
  // Untouched refusals remain unchanged. Within a split call, available capacity only decreases.
  std::unordered_map<uint64_t, EdgeUse> uses;
  const auto add = [&](Edge edge, Slot cell) {
    const uint64_t key = (uint64_t(std::min(edge[0], edge[1])) << 32) | std::max(edge[0], edge[1]);
    const auto insertion = uses.emplace(key, EdgeUse{ById(edge), cell});
    insertion.first->second.cell = std::min(insertion.first->second.cell, cell);
  };
  for (auto p : work_points_) {
    if (points_[p].removed) continue;
    for (auto cell : Star(p))
      for (auto q : cells_[cell].vertices)
        if (p != q) add({p, q}, cell);
  }
  for (auto edge : deferred) {
    if (points_[edge[0]].removed || points_[edge[1]].removed) continue;
    for (auto cell : Star(edge[0])) {
      const auto& vertices = cells_[cell].vertices;
      if (std::find(vertices.begin(), vertices.end(), edge[1]) != vertices.end()) add(edge, cell);
    }
  }
  std::vector<EdgeUse> result;
  result.reserve(uses.size());
  for (const auto& entry : uses) result.push_back(entry.second);
  return result;
}

size_t Mesh::CollapseSweeps(PassStats& stats) {
  size_t total = 0;
  const bool work_list = options_.work_list && options_.memory;
  std::vector<Edge> deferred;
  for (unsigned sweep = 0; sweep < LengthSweepLimit; ++sweep) {
    const SweepWatch watch(stats);
    stats.Increment(stats.sweeps_collapse);
    ++sweep_;
    MarkWallNear();
    size_t accepted = 0;
    std::vector<LengthCandidate> candidates;
    for (const auto& use : work_list && sweep ? NextEdges(deferred) : Edges()) {
      const auto edge = use.edge;
      if (!CanCollapse(edge[0]) && !CanCollapse(edge[1])) continue;
      const auto length = Length(edge);
      if (length < options_.short_edge)
        candidates.push_back({length, {points_[edge[0]].node.id, points_[edge[1]].node.id}, use});
    }
    deferred.clear();
    work_points_.clear();
    work_start_ = work_list ? change_ : 0;
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
        if (work_list) deferred.push_back(edge);
        continue;
      }
      // An edit to an incident cell or one of its point values stamps an endpoint.
      // This includes changes to wall-near marks, so an untouched star still refuses.
      if (options_.memory) {
        const auto remembered = collapse_refusals_.find(candidate.ids);
        if (remembered != collapse_refusals_.end() && remembered->second >= points_[edge[0]].changed_at &&
            remembered->second >= points_[edge[1]].changed_at) {
          stats.Increment(stats.skipped_memory);
          continue;
        }
      }
      stats.Increment(stats.evaluated_collapse);
      if (Collapse(edge, sweep_, stats) == Refusal::None) {
        ++accepted;
      } else if (options_.memory) {
        collapse_refusals_.insert_or_assign(candidate.ids, change_);
        if (collapse_refusals_.size() > 6 * live_cells_) collapse_refusals_.clear();
      }
    }
    work_start_ = 0;
    watch.Emit(stats, "collapse", stats.rounds, stats.sweeps_collapse, candidates.size(), live_cells_);
    if (!accepted) return total;
    total += accepted;
  }
  if (stats.report) stats.limit = true;
  return total;
}

size_t Mesh::SplitSweeps(IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  size_t total = 0;
  const bool work_list = options_.work_list && options_.memory;
  std::vector<Edge> deferred;
  for (unsigned sweep = 0; sweep < LengthSweepLimit; ++sweep) {
    const SweepWatch watch(stats);
    stats.Increment(stats.sweeps_split);
    ++sweep_;
    size_t accepted = 0;
    std::vector<LengthCandidate> candidates;
    for (const auto& use : work_list && sweep ? NextEdges(deferred) : Edges()) {
      const auto edge = use.edge;
      const auto length = Length(edge);
      if (length <= LongEdge) continue;
      const auto ring = EdgeRing(edge, use.cell);
      if (!ring.Closed()) continue;
      if (Blocked(ring.cells)) {
        stats.Increment(stats.blocked);
        continue;
      }
      candidates.push_back({-length, {points_[edge[0]].node.id, points_[edge[1]].node.id}, use});
    }
    deferred.clear();
    work_points_.clear();
    work_start_ = work_list ? change_ : 0;
    std::sort(candidates.begin(), candidates.end());
    for (const auto& candidate : candidates) {
      const auto& [edge, seed] = candidate.use;
      const auto& row = cells_[seed];
      if (!row.live || row.stamp == sweep_ || std::any_of(edge.begin(), edge.end(), [&](Slot point) {
            return std::find(row.vertices.begin(), row.vertices.end(), point) == row.vertices.end();
          })) {
        stats.Increment(stats.deferred);
        if (work_list) deferred.push_back(edge);
        continue;
      }
      const auto ring = EdgeRing(edge, seed);
      if (std::any_of(ring.cells.begin(), ring.cells.end(), [this](Slot c) { return cells_[c].stamp == sweep_; })) {
        stats.Increment(stats.deferred);
        if (work_list) deferred.push_back(edge);
        continue;
      }
      stats.Increment(stats.evaluated_split);
      if (Split(edge, seed, point_ids, sweep_, cell_limit, stats) == Refusal::None) ++accepted;
    }
    work_start_ = 0;
    watch.Emit(stats, "split", stats.rounds, stats.sweeps_split, candidates.size(), live_cells_);
    if (!accepted) return total;
    total += accepted;
  }
  if (stats.report) stats.limit = true;
  return total;
}

void Mesh::LengthWork(IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  for (unsigned round = 0; round < LengthRoundLimit; ++round) {
    stats.Increment(stats.rounds);
    const auto collapse_sweeps = stats.sweeps_collapse;
    const auto split_sweeps = stats.sweeps_split;
    const auto collapse_start = ReportClock(stats);
    const auto collapses = CollapseSweeps(stats);
    const auto split_start = ReportClock(stats);
    const auto splits = SplitSweeps(point_ids, cell_limit, stats);
    if (stats.report) {
      stats.seconds_collapse += split_start - collapse_start;
      stats.seconds_split += ReportClock(stats) - split_start;
      if (!stats.round_changes.empty()) stats.round_changes += ',';
      stats.round_changes += "c" + std::to_string(collapses) + "(" +
                             std::to_string(stats.sweeps_collapse - collapse_sweeps) + ")s" +
                             std::to_string(splits) + "(" + std::to_string(stats.sweeps_split - split_sweeps) + ")";
    }
    if (!collapses && !splits) return;
  }
  if (stats.report) stats.limit = true;
}

bool Mesh::QualitySweep(IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  const SweepWatch watch(stats);
  stats.Increment(stats.sweeps_quality);
  ++sweep_;
  bool changed = false;
  std::vector<QualityCandidate> candidates;
  for (Slot cell = 0; cell < cells_.size(); ++cell) {
    if (!cells_[cell].live) continue;
    const auto vertices = cells_[cell].vertices;
    const auto margin = Margin(vertices);
    if (margin >= BadMargin) continue;
    if (Blocked(vertices)) {
      stats.Increment(stats.blocked);
      continue;
    }
    candidates.push_back({margin, Key(vertices), cell});
  }
  std::sort(candidates.begin(), candidates.end());
  for (const auto& candidate : candidates) {
    const auto cell = candidate.cell;
    auto& row = cells_[cell];
    if (!row.live || row.stamp == sweep_) continue;
    // Incident-cell edits stamp their points, including point values read by adjacent options.
    // Capacity refusals also depend on the live cell count and cannot be remembered.
    if (options_.memory && row.failed_at != 0 && std::all_of(row.vertices.begin(), row.vertices.end(), [&](Slot p) {
          return row.failed_at >= points_[p].changed_at;
        })) {
      stats.Increment(stats.skipped_memory);
      continue;
    }
    stats.Increment(stats.evaluated_quality);
    const auto stable_order = [this](Slot a, Slot b) { return points_[a].node.id < points_[b].node.id; };
    auto vertices = row.vertices;
    std::sort(vertices.begin(), vertices.end(), stable_order);
    Plan best;
    bool best_edge = false, refused_capacity = false;
    EachEdge(vertices, [&](Edge edge) {
      auto plan = RemoveEdgePlan(edge, cell, cell_limit);
      stats.Count(plan.refusal);
      refused_capacity |= plan.refusal == Refusal::Memory;
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
      refused_capacity |= plan.refusal == Refusal::Memory;
      if (plan.Accepted() && (!best.Accepted() || plan.score > best.score)) {
        best = std::move(plan);
        best_edge = false;
      }
    }
    if (best.Accepted()) {
      if (options_.float_trials && !ExactlyPositive(best)) {
        stats.CountExact();
        if (options_.memory) row.failed_at = refused_capacity ? 0 : change_;
        continue;
      }
      Commit(best, sweep_);
      changed = true;
      if (best_edge)
        stats.Increment(stats.edge_removals);
      else
        stats.Increment(stats.face_swaps);
    } else {
      bool moved = false;
      for (auto p : vertices) {
        bool exact_refusal = false;
        moved = MovePoint(p, point_ids, sweep_, stats, &exact_refusal) == Refusal::None;
        if (moved || exact_refusal) break;
      }
      changed |= moved;
      if (options_.memory && !moved) row.failed_at = refused_capacity ? 0 : change_;
    }
  }
  watch.Emit(stats, "quality", 0, stats.sweeps_quality, candidates.size(), live_cells_, candidates.size());
  return changed;
}

void Mesh::QualityWork(IdSequence& point_ids, size_t cell_limit, PassStats& stats) {
  const auto accepted = [&stats] { return stats.edge_removals + stats.face_swaps + stats.moves; };
  for (unsigned sweep = 0; sweep < QualitySweepLimit; ++sweep) {
    const auto before = stats.report ? accepted() : 0;
    const bool changed = QualitySweep(point_ids, cell_limit, stats);
    if (stats.report) {
      if (!stats.quality_changes.empty()) stats.quality_changes += ',';
      stats.quality_changes += std::to_string(accepted() - before);
    }
    if (!changed) return;
  }
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
    stats.short_out += length < options_.short_edge;
    if (length > LongEdge) {
      const auto ring = EdgeRing(use.edge, use.cell);
      stats.long_left_surface += ring.surface;
      stats.long_left_border += ring.border;
    }
  }
}

PassStats Mesh::RunPass(IdSequence& point_ids, size_t cell_limit, bool report, CoreOptions options) {
  PassStats stats;
  stats.report = report;
  return RunPass(point_ids, cell_limit, std::move(stats), options);
}

PassStats Mesh::RunPass(IdSequence& point_ids, size_t cell_limit, PassStats stats, CoreOptions options) {
  options_ = options;
  if (stats.report) {
    stats.cells_in = live_cells_;
    stats.points_in = points_.size();
    for (const auto& row : cells_)
      if (row.live && Margin(row.vertices) < BadMargin) ++stats.bad_in;
  }
  LengthWork(point_ids, cell_limit, stats);
  const auto quality_start = ReportClock(stats);
  QualityWork(point_ids, cell_limit, stats);
  const auto check_start = ReportClock(stats);
  Check();
  if (stats.report) {
    MarkWallNear();
    ReportResult(stats);
    stats.seconds_quality = check_start - quality_start;
    stats.seconds_check = ReportClock(stats) - check_start;
  }
  return stats;
}
}  // namespace SU2NativeLocal3D
