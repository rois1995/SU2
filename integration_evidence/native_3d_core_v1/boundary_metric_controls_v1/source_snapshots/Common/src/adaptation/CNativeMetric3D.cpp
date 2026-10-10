/*!
 * \file CNativeMetric3D.cpp
 * \brief Actual-query metric shape, unique edges and frozen-target private progress.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeMetric3D.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
namespace SU2Native3D {
namespace {
bool Same(Point a, Point b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
Point Center(const Cell& c) {
  long double x = 0, y = 0, z = 0;
  for (auto n : c.v) {
    x += static_cast<long double>(n.p.x) / 4;
    y += static_cast<long double>(n.p.y) / 4;
    z += static_cast<long double>(n.p.z) / 4;
  }
  return {double(x), double(y), double(z)};
}
PatchMetric MeasurePatch(const std::vector<Cell>& cells, FrozenField& target, MetricStats* stats,
                         const std::map<Edge, EdgeMeasure>* reuse = nullptr) {
  if (cells.empty() || cells.size() > MaximumReplacementCells)
    throw std::invalid_argument("Native 3D metric patch exceeds budget or is empty.");
  if (stats) {
    ++stats->patches;
    stats->maximum_cells = std::max(stats->maximum_cells, cells.size());
  }
  PatchMetric result;
  result.weakest_cell = result.sliver_cell = cells.front().id;
  std::map<Id, Node> nodes;
  std::set<Id> cell_ids;
  std::set<std::array<Id, 4>> simplices;
  for (const auto& cell : cells) {
    if (!cell_ids.insert(cell.id).second) throw std::invalid_argument("Duplicate native 3D metric cell ID.");
    std::array<Id, 4> key;
    for (size_t i = 0; i < 4; ++i) {
      auto n = cell.v[i];
      key[i] = n.id;
      const auto old = nodes.emplace(n.id, n);
      if (!old.second && !Same(old.first->second.p, n.p))
        throw std::invalid_argument("Inconsistent native 3D metric node coordinates.");
    }
    std::sort(key.begin(), key.end());
    if (std::adjacent_find(key.begin(), key.end()) != key.end() || !simplices.insert(key).second)
      throw std::invalid_argument("Repeated native 3D metric simplex/node.");
    if (stats) ++stats->cells;
    MetricMeasures m;
    try {
      m = Measure({cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p}, target.Query(Center(cell)).target);
    } catch (const std::exception& e) {
      throw std::runtime_error(std::string(e.what()) + " at native 3D metric cell " + std::to_string(cell.id) + ".");
    }
    if (m.mean_ratio < result.minimum_mean_ratio) {
      result.minimum_mean_ratio = m.mean_ratio;
      result.weakest_cell = cell.id;
    }
    if (m.minimum_scaled_jacobian < result.minimum_scaled_jacobian) {
      result.minimum_scaled_jacobian = m.minimum_scaled_jacobian;
      result.sliver_cell = cell.id;
    }
    for (size_t i = 0; i < 4; ++i)
      for (size_t j = i + 1; j < 4; ++j) result.edges.emplace(Edge{key[i], key[j]}, EdgeMeasure{});
  }
  if (stats) {
    stats->maximum_nodes = std::max(stats->maximum_nodes, nodes.size());
    stats->maximum_edges = std::max(stats->maximum_edges, result.edges.size());
  }
  for (auto& edge : result.edges) {
    auto& m = edge.second;
    m.a = nodes.at(edge.first[0]).p;
    m.b = nodes.at(edge.first[1]).p;
    if (stats) ++stats->edges;
    bool reused = false;
    if (reuse) {
      const auto prior = reuse->find(edge.first);
      if (prior != reuse->end() && Same(prior->second.a, m.a) && Same(prior->second.b, m.b)) {
        m.length = prior->second.length;
        reused = true;
        if (stats) ++stats->edge_reused;
      }
    }
    if (!reused) {
      if (stats) ++stats->edge_evaluations;
      try {
        m.length = target.TargetEdgeLength(m.a, m.b);
      } catch (const std::exception& e) {
        throw std::runtime_error(std::string(e.what()) + " at native 3D metric edge " + std::to_string(edge.first[0]) +
                                 "," + std::to_string(edge.first[1]) + ".");
      }
    }
    if (m.length > result.maximum_length) {
      result.maximum_length = m.length;
      result.longest_edge = edge.first;
    }
  }
  return result;
}
void Shape(const PatchMetric& m) {
  if (m.minimum_mean_ratio < MinimumMeanRatio || m.minimum_scaled_jacobian < MinimumScaledJacobian) {
    std::ostringstream reason;
    reason << std::setprecision(17) << "Native 3D metric shape/sliver failure: q=" << m.minimum_mean_ratio
           << " at cell " << m.weakest_cell << ", J=" << m.minimum_scaled_jacobian << " at cell " << m.sliver_cell
           << '.';
    throw std::invalid_argument(reason.str());
  }
}
long double Excess(long double length) {
  const auto e = std::max(0.L, length - MaximumMetricEdge);
  return e * e;
}
bool Shared(const std::map<Edge, EdgeMeasure>& others, const std::pair<const Edge, EdgeMeasure>& edge) {
  const auto old = others.find(edge.first);
  return old != others.end() && Same(old->second.a, edge.second.a) && Same(old->second.b, edge.second.b);
}
}  // namespace
bool ValidateMetricPatch(const std::vector<Cell>& cells, FrozenField& target, PatchMetric& measured,
                         std::string& reason, MetricStats* stats) {
  try {
    measured = MeasurePatch(cells, target, stats);
    Shape(measured);
    if (measured.maximum_length > MaximumMetricEdge) {
      std::ostringstream message;
      message << std::setprecision(21) << "Native 3D oversized metric edge " << measured.longest_edge[0] << ','
              << measured.longest_edge[1] << ": length=" << measured.maximum_length << '.';
      throw std::invalid_argument(message.str());
    }
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    if (stats) ++stats->failures;
    reason = e.what();
    return false;
  }
}
bool ValidateMetricChange(const std::vector<Cell>& old, const std::vector<Cell>& fresh, MetricChange kind,
                          FrozenField& target, PatchMetric& measured, std::string& reason, MetricStats* stats) {
  try {
    if (stats) ++stats->changes;
    measured = MeasurePatch(fresh, target, stats);
    Shape(measured);
    const auto before = MeasurePatch(old, target, stats, &measured.edges);
    long double removed = 0, added = 0;
    for (const auto& edge : before.edges)
      if (!Shared(measured.edges, edge)) removed += Excess(edge.second.length);
    for (const auto& edge : measured.edges)
      if (!Shared(before.edges, edge)) added += Excess(edge.second.length);
    if (kind == MetricChange::Refine) {
      if (fresh.size() <= old.size() || !(removed > 0 && added < removed * (1 - 1e-12L)))
        throw std::invalid_argument("Native 3D refinement does not reduce frozen changed-edge excess.");
    } else if (fresh.size() >= old.size() || added > 0)
      throw std::invalid_argument("Native 3D coarsening does not remove cells or introduces an oversized edge.");
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    if (stats) ++stats->failures;
    reason = e.what();
    return false;
  }
}
}  // namespace SU2Native3D
