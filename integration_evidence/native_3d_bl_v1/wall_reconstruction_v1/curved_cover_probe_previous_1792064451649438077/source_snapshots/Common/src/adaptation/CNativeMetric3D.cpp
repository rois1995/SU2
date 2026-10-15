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
Point MetricMeanPoint(Point origin,const std::vector<std::pair<Point,Tensor>>& neighbors) {
  if(neighbors.empty() || neighbors.size()>4*MaximumCavityCells)
    throw std::invalid_argument("Native3D movement neighbor count exceeds bounds.");
  long double scale=0;
  for(const auto& row:neighbors) {
    ValidateTensor(row.second);
    const auto& m=row.second;scale=std::max({scale,std::abs((long double)m.xx),std::abs((long double)m.xy),
        std::abs((long double)m.xz),std::abs((long double)m.yy),std::abs((long double)m.yz),std::abs((long double)m.zz)});
  }
  long double a[3][3]{},rhs[3]{},l[3][3]{},work[3]{},delta[3]{};
  for(const auto& row:neighbors) {
    const auto& m=row.second;const long double matrix[3][3]={{m.xx,m.xy,m.xz},{m.xy,m.yy,m.yz},{m.xz,m.yz,m.zz}};
    const long double d[3]={(long double)row.first.x-origin.x,(long double)row.first.y-origin.y,(long double)row.first.z-origin.z};
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j){a[i][j]+=matrix[i][j]/scale;rhs[i]+=matrix[i][j]/scale*d[j];}
  }
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<=i;++j) {
    auto value=a[i][j];for(unsigned k=0;k<j;++k)value-=l[i][k]*l[j][k];
    if(i==j){if(!(value>0))throw std::runtime_error("Unrepresentable native3D movement frame.");l[i][j]=std::sqrt(value);}
    else l[i][j]=value/l[j][j];
  }
  for(unsigned i=0;i<3;++i){auto value=rhs[i];for(unsigned j=0;j<i;++j)value-=l[i][j]*work[j];work[i]=value/l[i][i];}
  for(int i=2;i>=0;--i){auto value=work[i];for(unsigned j=i+1;j<3;++j)value-=l[j][i]*delta[j];delta[i]=value/l[i][i];}
  const Point result{double(origin.x+delta[0]),double(origin.y+delta[1]),double(origin.z+delta[2])};
  if(!std::isfinite(result.x) || !std::isfinite(result.y) || !std::isfinite(result.z))
    throw std::runtime_error("Nonfinite native3D movement proposal.");
  return result;
}

namespace {
bool Same(Point a, Point b) { return a.x == b.x && a.y == b.y && a.z == b.z; }
void Shape(const PatchMetric& m);
}  // namespace
PatchMetric MeasureMetricCells(const std::vector<Cell>& cells, FrozenField& target, MetricStats* stats,
                               const std::map<Edge, EdgeMeasure>* reuse, bool require_shape, MetricDiagnosis* diagnosis) {
  if (diagnosis) { *diagnosis={}; diagnosis->failure=MetricFailure::Measurement; }
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
      m = Measure({cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p}, target.Query(Centroid(cell)).target);
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
  // Reject inadmissible private shapes before spending source-resolved edge integration work.
  if (diagnosis) {
    diagnosis->minimum_mean_ratio=result.minimum_mean_ratio;
    diagnosis->minimum_scaled_jacobian=result.minimum_scaled_jacobian;
    diagnosis->weakest_cell=result.weakest_cell; diagnosis->sliver_cell=result.sliver_cell;
    if (require_shape && (result.minimum_mean_ratio<MinimumMeanRatio || result.minimum_scaled_jacobian<MinimumScaledJacobian))
      diagnosis->failure=MetricFailure::Shape;
  }
  if (require_shape) Shape(result);
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
    if (reuse && target.SensorOnly()) {
      const auto prior = reuse->find(edge.first);
      if (prior != reuse->end() && Same(prior->second.a, m.a) && Same(prior->second.b, m.b) &&
          std::isfinite(prior->second.length) && prior->second.length > 0) {
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
  if (diagnosis) diagnosis->failure=MetricFailure::None;
  return result;
}
namespace {
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
                         std::string& reason, MetricStats* stats, const std::map<Edge, EdgeMeasure>* frozen_reuse) {
  try {
    measured = MeasureMetricCells(cells, target, stats, frozen_reuse);
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
bool ValidateMetricChange(size_t old_count, const PatchMetric& before, size_t fresh_count,
                          const PatchMetric& measured, MetricChange kind, std::string& reason, MetricDiagnosis* diagnosis) {
  if (diagnosis) { *diagnosis={}; diagnosis->failure=MetricFailure::Measurement; }
  try {
    if (!old_count || !fresh_count || old_count > MaximumReplacementCells || fresh_count > MaximumReplacementCells)
      throw std::invalid_argument("Native 3D metric-change cell count exceeds budget or is empty.");
    if (kind != MetricChange::Refine && kind != MetricChange::Coarsen && kind != MetricChange::Repair && kind != MetricChange::GeometryRefine && kind != MetricChange::GeometryRepair)
      throw std::invalid_argument("Unknown native 3D metric-change intent.");
    for (const auto* metric : {&before, &measured}) {
      if (!std::isfinite(metric->minimum_mean_ratio) || !std::isfinite(metric->minimum_scaled_jacobian) ||
          metric->minimum_mean_ratio < 0 || metric->minimum_scaled_jacobian < 0 || metric->edges.empty() ||
          metric->edges.size() > 6 * MaximumReplacementCells)
        throw std::invalid_argument("Invalid native 3D metric-change measurement.");
      for (const auto& edge : metric->edges)
        if (!std::isfinite(edge.second.length) || edge.second.length <= 0)
          throw std::invalid_argument("Invalid native 3D metric-change edge measurement.");
    }
    if (diagnosis) {
      diagnosis->minimum_mean_ratio=measured.minimum_mean_ratio;
      diagnosis->minimum_scaled_jacobian=measured.minimum_scaled_jacobian;
      diagnosis->weakest_cell=measured.weakest_cell; diagnosis->sliver_cell=measured.sliver_cell;
      diagnosis->failure=MetricFailure::Shape;
    }
    Shape(measured);
    long double removed = 0, added = 0;
    for (const auto& edge : before.edges)
      if (!Shared(measured.edges, edge)) removed += Excess(edge.second.length);
    for (const auto& edge : measured.edges)
      if (!Shared(before.edges, edge)) added += Excess(edge.second.length);
    const bool size_progress = removed > 0 && added < removed * (1 - 1e-12L);
    const bool quality_progress =
        (measured.minimum_mean_ratio > before.minimum_mean_ratio + 1e-7 &&
         measured.minimum_scaled_jacobian >= before.minimum_scaled_jacobian * (1 - 1e-12)) ||
        (measured.minimum_scaled_jacobian > before.minimum_scaled_jacobian + 1e-7 &&
         measured.minimum_mean_ratio >= before.minimum_mean_ratio * (1 - 1e-12));
    if (diagnosis) {
      diagnosis->failure=MetricFailure::Progress;
      diagnosis->removed_excess=removed; diagnosis->added_excess=added;
      diagnosis->size_progress=size_progress;
      diagnosis->quality_progress=quality_progress && added<=removed;
      diagnosis->cell_removal=fresh_count<old_count;
    }
    if (kind == MetricChange::Refine) {
      if (fresh_count <= old_count || !(removed > 0 && added < removed * (1 - 1e-12L)))
        throw std::invalid_argument("Native 3D refinement does not reduce frozen changed-edge excess.");
    } else if (kind == MetricChange::GeometryRefine) {
      if (fresh_count <= old_count || added > removed)
        throw std::invalid_argument("Native3D geometric refinement adds frozen metric excess or removes no geometric freedom.");
    } else if (kind == MetricChange::GeometryRepair) {
      if (added>removed)
        throw std::invalid_argument("Native3D geometric repair adds frozen metric excess.");
    } else if (kind == MetricChange::Coarsen) {
      if (fresh_count >= old_count || added > 0)
        throw std::invalid_argument("Native 3D coarsening does not remove cells or introduces an oversized edge.");
    } else {
      if (!(size_progress || (quality_progress && added <= removed)))
        throw std::invalid_argument("Native 3D repair improves neither frozen size excess nor quality/sliver bounds.");
    }
    if (diagnosis) diagnosis->failure=MetricFailure::None;
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool ValidateMetricChange(const std::vector<Cell>& old, const std::vector<Cell>& fresh, MetricChange kind,
                          FrozenField& target, PatchMetric& measured, std::string& reason, MetricStats* stats, const std::map<Edge, EdgeMeasure>* frozen_reuse, MetricDiagnosis* diagnosis) {
  if (diagnosis) { *diagnosis={}; diagnosis->failure=MetricFailure::Measurement; }
  try {
    if (stats) ++stats->changes;
    if (kind != MetricChange::Refine && kind != MetricChange::Coarsen && kind != MetricChange::Repair && kind != MetricChange::GeometryRefine && kind != MetricChange::GeometryRepair)
      throw std::invalid_argument("Unknown native 3D metric-change intent.");
    measured = MeasureMetricCells(fresh, target, stats, frozen_reuse, true, diagnosis);
    Shape(measured);
    if (diagnosis) diagnosis->failure=MetricFailure::Measurement;
    const auto before = MeasureMetricCells(old, target, stats, &measured.edges);
    const bool valid = ValidateMetricChange(old.size(), before, fresh.size(), measured, kind, reason, diagnosis);
    if (!valid && stats) ++stats->failures;
    return valid;
  } catch (const std::exception& e) {
    if (stats) ++stats->failures;
    reason = e.what();
    return false;
  }
}
}  // namespace SU2Native3D
