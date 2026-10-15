/*!
 * \file CNativeRegion3D.cpp
 * \brief Whole-region surface sampling and coupled wall/transition tetrahedral construction.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeRegion3D.hpp"
#include "../../include/adaptation/CNativeMetric3D.hpp"
#include "Eigen/Dense"
#include <algorithm>
#include <cmath>
#include <set>
#include <limits>
#include <stdexcept>

namespace SU2Native3D {
namespace {
Point Add(Point a, Point b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Point Sub(Point a, Point b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Point Mul(Point a, double b) { return {a.x * b, a.y * b, a.z * b}; }
double Dot(Point a, Point b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Point Cross(Point a, Point b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double Norm(Point a) { return std::hypot(a.x, a.y, a.z); }
Point Unit(Point a) {
  const double n = Norm(a);
  if (!(n > 0)) throw std::runtime_error("Region has a degenerate normal.");
  return Mul(a, 1 / n);
}
Edge Key(Id a, Id b) { return a < b ? Edge{a, b} : Edge{b, a}; }
Face Key(Face f) {
  std::sort(f.begin(), f.end());
  return f;
}
struct Use {
  size_t face;
  Id a, b;
};
using Edges = std::map<Edge, std::vector<Use>>;
struct Junction {
  Id node, facet;
  std::array<Id, 4> quad;
  std::vector<size_t> cells;
};
class Construction {
 public:
  const PlanarReference& geometry;
  const std::map<int, RegionWallPolicy>& policy;
  const double tolerance;
  FrozenField& field;
  const RegionLimits& limits;
  RegionReport& report;
  std::map<Id, Point> nodes;
  std::vector<SurfaceFace> faces;
  std::map<Id, std::set<int>> tags;
  std::map<int, Id> caps;
  Id next_node, next_cell;
  Patch fresh;
  std::vector<Junction> junctions;
  std::map<Edge, long double> lengths;
  Construction(const PlanarReference& g, const std::map<int, RegionWallPolicy>& p, double t, FrozenField& f,
               const RegionLimits& l, RegionReport& r, Id n, Id c)
      : geometry(g), policy(p), tolerance(t), field(f), limits(l), report(r), next_node(n), next_cell(c) {}
  void Check() const {
    if (limits.checkpoint) limits.checkpoint();
    if (fresh.cells.size() > limits.maximum_cells || nodes.size() > 4 * limits.maximum_cells ||
        report.surface_trials > limits.maximum_surface_edits)
      throw std::runtime_error("Region construction exceeds admitted cells/surface work.");
  }
  Id New(Point p) {
    Check();
    if (next_node == UINT64_MAX) throw std::overflow_error("Region node IDs exhausted.");
    nodes.emplace(next_node, p);
    return next_node++;
  }
  bool Wall(Id f) const { return policy.count(geometry.Get(f).marker); }
  double Height(Id f) const { return policy.at(geometry.Get(f).marker).height; }
  std::array<Point, 3> Triangle(const SurfaceFace& f) const {
    return {nodes.at(f.v[0]), nodes.at(f.v[1]), nodes.at(f.v[2])};
  }
  Point Normal(const SurfaceFace& f) const {
    const auto p = Triangle(f);
    return Unit(Cross(Sub(p[1], p[0]), Sub(p[2], p[0])));
  }
  Point Center(const SurfaceFace& f) const {
    const auto p = Triangle(f);
    return Mul(Add(Add(p[0], p[1]), p[2]), 1. / 3);
  }
  Edges Links(const std::vector<SurfaceFace>& fs) const {
    Edges edges;
    for (size_t i = 0; i < fs.size(); ++i)
      for (unsigned k = 0; k < 3; ++k) {
        const Id a = fs[i].v[k], b = fs[i].v[(k + 1) % 3];
        edges[Key(a, b)].push_back({i, a, b});
      }
    return edges;
  }
  long double Length(Id a, Id b) {
    const auto k = Key(a, b);
    auto it = lengths.find(k);
    if (it == lengths.end()) it = lengths.emplace(k, field.TargetEdgeLength(nodes.at(a), nodes.at(b))).first;
    return it->second;
  }
  Cell Tet(std::array<Id, 4> ids, Id id = 0) const {
    Cell c;
    c.id = id;
    for (unsigned k = 0; k < 4; ++k) c.v[k] = {ids[k], nodes.at(ids[k])};
    return c;
  }
  MetricMeasures Shape(const Cell& c) {
    return Measure({c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p}, field.Query(Centroid(c)).target);
  }
  double Score(const std::vector<std::array<Id, 4>>& cells, bool measure_edges = true) {
    double score = INFINITY;
    for (auto ids : cells) {
      const auto c = Tet(ids);
      if (!(Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) > 0)) return -INFINITY;
      const auto m = Shape(c);
      score = std::min({score, m.mean_ratio / .22, m.minimum_scaled_jacobian / .055});
      if (measure_edges)
        for (unsigned a = 0; a < 4; ++a)
          for (unsigned b = a + 1; b < 4; ++b)
            score = std::min(score, double(.95 * MaximumMetricEdge / Length(ids[a], ids[b])));
    }
    return score;
  }
  void Append(const std::array<Id, 4>& ids) {
    Check();
    if (next_cell == UINT64_MAX) throw std::overflow_error("Region cell IDs exhausted.");
    fresh.cells.push_back(Tet(ids, next_cell++));
  }
  bool SurfaceValid(const SurfaceFace& f) {
    const auto p = Triangle(f);
    if (!geometry.ContainsFace(f.facet, p)) return false;
    const double tol = Wall(f.facet) ? std::min(tolerance, .5 * Height(f.facet)) : tolerance;
    return geometry.SurfaceError(f.facet, p) <= tol;
  }
  void SplitSurface() {
    const auto edges = Links(faces);
    std::vector<Edge> marked;
    for (const auto& row : edges)
      if (Length(row.first[0], row.first[1]) > MaximumMetricEdge) marked.push_back(row.first);
    for (auto e : marked) {
      Check();
      ++report.surface_trials;
      const auto uses = Links(faces).at(e);
      if (uses.size() != 2) throw std::runtime_error("Region surface is not closed.");
      Point p = Mul(Add(nodes.at(e[0]), nodes.at(e[1])), .5);
      std::vector<Id> charts;
      for (const auto& u : uses) charts.push_back(faces[u.face].facet);
      std::stable_sort(charts.begin(), charts.end(), [&](Id a, Id b) {
        return geometry.Analytic().at(geometry.Canonical(a)).kind == AnalyticSurface::Kind::Disk &&
               geometry.Analytic().at(geometry.Canonical(b)).kind != AnalyticSurface::Kind::Disk;
      });
      for (Id f : charts) p = geometry.Project(f, p);
      for (Id f : charts)
        if (!geometry.Contains(f, p)) throw std::runtime_error("Region split leaves its declared feature.");
      const Id mid = New(p);
      std::vector<SurfaceFace> next;
      for (size_t i = 0; i < faces.size(); ++i) {
        bool hit = false;
        for (auto u : uses)
          if (u.face == i) {
            hit = true;
            Id c = 0;
            for (Id n : faces[i].v)
              if (n != e[0] && n != e[1]) c = n;
            next.push_back({{u.a, mid, c}, faces[i].facet});
            next.push_back({{mid, u.b, c}, faces[i].facet});
          }
        if (!hit) next.push_back(faces[i]);
      }
      faces.swap(next);
      ++report.surface_splits;
    }
  }
  void CoarsenSurface(Point kernel) {
    std::vector<SurfaceFace> wall;
    for (auto f : faces)
      if (Wall(f.facet))
        wall.push_back(f);
      else {
        const int m = geometry.Get(f.facet).marker;
        auto c = caps.emplace(m, f.facet);
        c.first->second = std::min(c.first->second, f.facet);
        for (Id n : f.v) tags[n].insert(m);
      }
    for (;;) {
      Check();
      const auto edges = Links(wall);
      std::map<Id, std::set<Id>> neighbors;
      std::vector<std::pair<long double, Edge>> order;
      for (const auto& e : edges) {
        neighbors[e.first[0]].insert(e.first[1]);
        neighbors[e.first[1]].insert(e.first[0]);
        const auto l = Length(e.first[0], e.first[1]);
        if (l < .9) order.push_back({l, e.first});
      }
      std::sort(order.begin(), order.end());
      bool changed = false;
      for (const auto& entry : order) {
        for (unsigned reverse = 0; reverse < 2; ++reverse) {
          const Id removed = entry.second[reverse], retained = entry.second[1 - reverse];
          if (!std::includes(tags[retained].begin(), tags[retained].end(), tags[removed].begin(), tags[removed].end()))
            continue;
          if (!tags[removed].empty() && !tags[retained].empty() && edges.at(entry.second).size() != 1) continue;
          std::set<Id> opposite, shared;
          for (auto u : edges.at(entry.second))
            for (Id n : wall[u.face].v)
              if (n != removed && n != retained) opposite.insert(n);
          std::set_intersection(neighbors[removed].begin(), neighbors[removed].end(), neighbors[retained].begin(),
                                neighbors[retained].end(), std::inserter(shared, shared.end()));
          if (opposite != shared) continue;
          ++report.surface_trials;
          Check();
          std::vector<SurfaceFace> next;
          std::set<Edge> old_edges;
          for (auto f : wall)
            if (std::find(f.v.begin(), f.v.end(), removed) != f.v.end())
              for (unsigned k = 0; k < 3; ++k) old_edges.insert(Key(f.v[k], f.v[(k + 1) % 3]));
          bool valid = true;
          for (auto f : wall) {
            if (std::find(f.v.begin(), f.v.end(), removed) == f.v.end()) {
              next.push_back(f);
              continue;
            }
            for (auto& n : f.v)
              if (n == removed) n = retained;
            if (Key(f.v)[0] == Key(f.v)[1] || Key(f.v)[1] == Key(f.v)[2]) continue;
            if (!SurfaceValid(f)) {
              valid = false;
              break;
            }
            for (unsigned k = 0; k < 3; ++k)
              if (!old_edges.count(Key(f.v[k], f.v[(k + 1) % 3])) &&
                  Length(f.v[k], f.v[(k + 1) % 3]) > .95 * MaximumMetricEdge)
                valid = false;
            if (!valid) break;
            const Point apex = Sub(Center(f), Mul(Normal(f), Height(f.facet)));
            Cell c;
            c.v[0] = {UINT64_MAX, apex};
            for (unsigned k = 0; k < 3; ++k) c.v[k + 1] = {f.v[k], nodes.at(f.v[k])};
            const auto shape = Shape(c);
            if (shape.mean_ratio < .24 || shape.minimum_scaled_jacobian < .06) {
              valid = false;
              break;
            }
            next.push_back(f);
          }
          if (!valid) continue;
          std::set<Face> keys;
          for (auto f : next)
            if (!keys.insert(Key(f.v)).second) valid = false;
          if (!valid) continue;
          wall.swap(next);
          ++report.surface_collapses;
          changed = true;
          break;
        }
        if (changed) break;
      }
      if (!changed) break;
    }
    faces = wall;
    std::map<int, std::vector<std::pair<Id, Id>>> loops;
    for (const auto& row : Links(wall)) {
      if (row.second.size() > 2) throw std::runtime_error("Region wall is not manifold.");
      if (row.second.size() == 1) {
        std::vector<int> shared;
        std::set_intersection(tags[row.first[0]].begin(), tags[row.first[0]].end(), tags[row.first[1]].begin(),
                              tags[row.first[1]].end(), std::back_inserter(shared));
        if (shared.size() != 1) throw std::runtime_error("Region rim lost its unique cap.");
        const auto use = row.second.front();
        loops[shared.front()].push_back({use.b, use.a});
      }
    }
    for (const auto& loop : loops) {
      const Id facet = caps.at(loop.first), center = New(geometry.Project(facet, kernel));
      VerifyLoop(loop.second);
      for (auto e : loop.second) faces.push_back({{center, e.first, e.second}, facet});
    }
  }
  void VerifyLoop(const std::vector<std::pair<Id, Id>>& edges) const {
    std::map<Id, Id> next;
    std::set<Id> incoming;
    for (auto e : edges)
      if (!next.emplace(e.first, e.second).second || !incoming.insert(e.second).second)
        throw std::runtime_error("Region cap has branched rims.");
    if (next.empty()) throw std::runtime_error("Region cap has an empty rim.");
    Id at = next.begin()->first;
    std::set<Id> walked;
    for (size_t k = 0; k < edges.size(); ++k) {
      if (!walked.insert(at).second || !next.count(at)) throw std::runtime_error("Region cap is not one closed loop.");
      at = next.at(at);
    }
    if (at != next.begin()->first) throw std::runtime_error("Region cap is open.");
  }
  struct Flow {
    Eigen::Matrix<double, 3, 4> displacement;
    Point center;
    double scale;
    Point Apply(Point p, double step = 1) const {
      const Point q = Mul(Sub(p, center), 1 / scale);
      const Eigen::Vector3d d = displacement * Eigen::Vector4d(q.x, q.y, q.z, 1);
      return Add(p, {step * d[0], step * d[1], step * d[2]});
    }
  };
  Flow Fit(const std::vector<SurfaceFace>& fs, double depth) {
    std::set<Id> ids;
    for (auto f : fs)
      for (Id n : f.v) ids.insert(n);
    Point center{};
    for (Id n : ids) center = Add(center, nodes.at(n));
    center = Mul(center, 1. / ids.size());
    double scale = 0;
    for (Id n : ids) scale = std::max(scale, Norm(Sub(nodes.at(n), center)));
    if (!(scale > 0)) throw std::runtime_error("Region has no scale.");
    std::vector<Eigen::Matrix<double, 1, 12>> rows, constraints;
    std::vector<double> rhs;
    for (auto f : fs) {
      const auto p = Triangle(f);
      const auto n = Normal(f);
      const double weight = std::sqrt(Norm(Cross(Sub(p[1], p[0]), Sub(p[2], p[0]))));
      for (Point x : p) {
        const Point q = Mul(Sub(x, center), 1 / scale);
        Eigen::Matrix<double, 1, 12> row;
        const double a[3] = {n.x, n.y, n.z}, b[4] = {q.x, q.y, q.z, 1};
        for (unsigned i = 0; i < 3; ++i)
          for (unsigned j = 0; j < 4; ++j) row[4 * i + j] = a[i] * b[j];
        if (Wall(f.facet)) {
          rows.push_back(weight * row);
          rhs.push_back(-weight * depth);
        } else
          constraints.push_back(row);
      }
    }
    Eigen::MatrixXd c(constraints.size(), 12), a(rows.size(), 12);
    Eigen::VectorXd b(rhs.size());
    for (size_t i = 0; i < constraints.size(); ++i) c.row(i) = constraints[i];
    for (size_t i = 0; i < rows.size(); ++i) {
      a.row(i) = rows[i];
      b[i] = rhs[i];
    }
    Eigen::MatrixXd null = Eigen::MatrixXd::Identity(12, 12);
    if (c.rows()) {
      Eigen::JacobiSVD<Eigen::MatrixXd> svd(c, Eigen::ComputeFullV);
      svd.setThreshold(1e-11);
      null = svd.matrixV().rightCols(12 - svd.rank());
    }
    if (!null.cols()) throw std::runtime_error("Region cap constraints block the inward flow.");
    const Eigen::VectorXd coefficients =
        null * (a * null).jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
    Flow flow;
    flow.center = center;
    flow.scale = scale;
    for (unsigned i = 0; i < 3; ++i)
      for (unsigned j = 0; j < 4; ++j) flow.displacement(i, j) = coefficients[4 * i + j];
    if (c.rows() && (c * coefficients).lpNorm<Eigen::Infinity>() > 1e-11 * depth)
      throw std::runtime_error("Region flow fails planar constraints.");
    if ((Eigen::Matrix3d::Identity() + flow.displacement.leftCols<3>() / scale).determinant() <= 0)
      throw std::runtime_error("Region inward flow folds.");
    return flow;
  }
  std::map<Id, Id> MapFront(const std::vector<SurfaceFace>& fs, const Flow& flow, double step) {
    std::set<Id> ids;
    std::map<Id, std::set<Id>> charts;
    for (auto f : fs)
      for (Id n : f.v) {
        ids.insert(n);
        if (!Wall(f.facet)) charts[n].insert(f.facet);
      }
    std::map<Id, Id> result;
    for (Id n : ids) {
      Point p = flow.Apply(nodes.at(n), step);
      for (Id f : charts[n]) p = geometry.Project(f, p);
      result[n] = New(p);
    }
    return result;
  }
  void Band(const std::vector<SurfaceFace>& outer, const std::map<Id, Id>& inner, const std::vector<Id>& apices,
            const std::map<Edge, Id>& side, bool physical) {
    for (size_t i = 0; i < outer.size(); ++i) {
      const auto f = outer[i];
      const auto a = f.v;
      Append({apices[i], a[0], a[1], a[2]});
      Append({apices[i], inner.at(a[2]), inner.at(a[1]), inner.at(a[0])});
      if (physical) fresh.surface.push_back(f);
    }
    for (const auto& entry : Links(outer)) {
      Check();
      const auto uses = entry.second;
      const auto u = uses.front();
      const Id p = apices[u.face], a = u.a, b = u.b;
      const std::array<Id, 4> q{a, inner.at(a), inner.at(b), b};
      if (uses.size() == 2) {
        const auto v = uses[1];
        if (v.a != b || v.b != a) throw std::runtime_error("Region wall edge orientations disagree.");
        const Id other = apices[v.face];
        std::vector<std::vector<std::array<Id, 4>>> options(3);
        for (unsigned k = 0; k < 4; ++k) options[0].push_back({p, other, q[k], q[(k + 1) % 4]});
        const unsigned diagonals[2][2][3] = {{{0, 1, 2}, {0, 2, 3}}, {{0, 1, 3}, {1, 2, 3}}};
        for (unsigned d = 0; d < 2; ++d)
          for (const auto& tri : diagonals[d]) {
            options[d + 1].push_back({p, q[tri[0]], q[tri[1]], q[tri[2]]});
          }
        for (unsigned d = 0; d < 2; ++d)
          for (const auto& tri : diagonals[d]) {
            options[d + 1].push_back({other, q[tri[2]], q[tri[1]], q[tri[0]]});
          }
        double best = -INFINITY;
        size_t choice = 0;
        for (size_t i = 0; i < options.size(); ++i) {
          const double s = Score(options[i]);
          if (s > best) {
            best = s;
            choice = i;
          }
        }
        if (!std::isfinite(best)) throw std::runtime_error("Region shared interface has no positive triangulation.");
        for (auto t : options[choice]) Append(t);
      } else {
        if (uses.size() != 1 || !side.count(entry.first)) throw std::runtime_error("Region wall rim is not admitted.");
        const Id facet = side.at(entry.first);
        Point center{};
        for (Id n : q) center = Add(center, nodes.at(n));
        const Id other = New(geometry.Project(facet, Mul(center, .25)));
        Junction junction{other, facet, q, {}};
        for (unsigned k = 0; k < 4; ++k) {
          junction.cells.push_back(fresh.cells.size());
          Append({p, other, q[k], q[(k + 1) % 4]});
          fresh.surface.push_back({{other, q[k], q[(k + 1) % 4]}, facet});
        }
        junctions.push_back(junction);
      }
    }
  }
  void PlaceJunctions() {
    for (const auto& j : junctions) {
      Check();
      const Point origin = nodes.at(j.node), u = Unit(Sub(nodes.at(j.quad[1]), nodes.at(j.quad[0]))),
                  normal = Unit(Cross(Sub(nodes.at(j.quad[1]), nodes.at(j.quad[0])),
                                      Sub(nodes.at(j.quad[2]), nodes.at(j.quad[0])))),
                  v = Cross(normal, u);
      double diameter = 0;
      for (Id n : j.quad) diameter = std::max(diameter, Norm(Sub(nodes.at(n), origin)));
      auto evaluate = [&](Point point) {
        ++report.junction_trials;
        if (limits.checkpoint) limits.checkpoint();
        point = geometry.Project(j.facet, point);
        for (unsigned k = 0; k < 4; ++k)
          if (Dot(Cross(Sub(nodes.at(j.quad[(k + 1) % 4]), nodes.at(j.quad[k])), Sub(point, nodes.at(j.quad[k]))),
                  normal) < 0)
            return -std::numeric_limits<double>::infinity();
        double score = INFINITY;
        for (size_t index : j.cells) {
          auto c = fresh.cells[index];
          for (auto& n : c.v)
            if (n.id == j.node) n.p = point;
          if (!(Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) > 0))
            return -std::numeric_limits<double>::infinity();
          const auto m = Shape(c);
          score = std::min({score, m.mean_ratio / .22, m.minimum_scaled_jacobian / .055});
        }
        return score;
      };
      Point best = origin;
      double value = evaluate(best), step = .1 * diameter;
      for (unsigned iteration = 0; iteration < 48 && value < 1.0 && step > 1e-7 * diameter; ++iteration) {
        Point chosen = best;
        double next = value;
        for (int x = -1; x <= 1; ++x)
          for (int y = -1; y <= 1; ++y)
            if (x || y) {
              const Point trial = geometry.Project(j.facet, Add(best, Mul(Add(Mul(u, x), Mul(v, y)), step)));
              const double s = evaluate(trial);
              if (s > next) {
                next = s;
                chosen = trial;
              }
            }
        if (next > value) {
          best = chosen;
          value = next;
        } else
          step *= .5;
      }
      nodes.at(j.node) = best;
      for (size_t index : j.cells)
        for (auto& n : fresh.cells[index].v)
          if (n.id == j.node) n.p = best;
    }
    lengths.clear();
  }
  void Validate() {
    Check();
    const Incidence topology(fresh.cells);
    if (topology.FaceComponents() != 1) throw std::runtime_error("Region construction has disconnected volumes.");
    const auto skin = topology.Boundary();
    ValidateDomainSurfaceTopology(skin);
    std::map<Face, Id> physical;
    for (auto f : fresh.surface) {
      if (!SurfaceValid(f)) throw std::runtime_error("Region physical face fails authority/error.");
      if (!physical.emplace(Key(f.v), f.facet).second) throw std::runtime_error("Region repeats a physical face.");
    }
    if (physical.size() != skin.size()) throw std::runtime_error("Region has a missing physical face.");
    for (auto f : skin)
      if (!physical.count(Key(f))) throw std::runtime_error("Region has a spurious interface face.");
    // Complete vertex links exclude pinched interior stars that face incidence alone cannot detect.
    std::map<Id, std::vector<Face>> links;
    std::set<Id> boundary;
    for (auto f : skin)
      for (Id n : f) boundary.insert(n);
    for (const auto& c : fresh.cells)
      for (unsigned k = 0; k < 4; ++k) {
        Face f;
        unsigned at = 0;
        for (unsigned j = 0; j < 4; ++j)
          if (j != k) f[at++] = c.v[j].id;
        if (k % 2) std::swap(f[1], f[2]);
        links[c.v[k].id].push_back(f);
      }
    for (const auto& row : links)
      ValidateSurfaceTopology(row.second, !boundary.count(row.first), 4 * limits.maximum_cells);
    std::map<int, double> heights;
    for (auto p : policy) heights[p.first] = p.second.height;
    FirstHeightReport height;
    std::string why;
    if (!CheckFirstHeight(fresh, heights, geometry, height, why)) throw std::runtime_error(why);
    struct Box {
      Point lo, hi;
    };
    std::vector<Box> boxes;
    boxes.reserve(fresh.cells.size());
    for (const auto& c : fresh.cells) {
      Check();
      const auto m = Shape(c);
      report.minimum_quality = std::min(report.minimum_quality, m.mean_ratio);
      report.minimum_jacobian = std::min(report.minimum_jacobian, m.minimum_scaled_jacobian);
      if (m.mean_ratio < MinimumMeanRatio || m.minimum_scaled_jacobian < MinimumScaledJacobian)
        throw std::runtime_error("Region fails actual-centroid quality/sliver target.");
      Box box{c.v[0].p, c.v[0].p};
      for (auto n : c.v) {
        box.lo = {std::min(box.lo.x, n.p.x), std::min(box.lo.y, n.p.y), std::min(box.lo.z, n.p.z)};
        box.hi = {std::max(box.hi.x, n.p.x), std::max(box.hi.y, n.p.y), std::max(box.hi.z, n.p.z)};
      }
      boxes.push_back(box);
      for (unsigned a = 0; a < 4; ++a)
        for (unsigned b = a + 1; b < 4; ++b) {
          const auto l = Length(c.v[a].id, c.v[b].id);
          report.maximum_edge = std::max(report.maximum_edge, l);
          if (l > MaximumMetricEdge) throw std::runtime_error("Region fails full composed edge target.");
        }
    }
    for (size_t i = 0; i < fresh.cells.size(); ++i) {
      Check();
      for (size_t j = i + 1; j < fresh.cells.size(); ++j) {
        const auto a = boxes[i], b = boxes[j];
        if (a.hi.x < b.lo.x || b.hi.x < a.lo.x || a.hi.y < b.lo.y || b.hi.y < a.lo.y || a.hi.z < b.lo.z ||
            b.hi.z < a.lo.z)
          continue;
        if (!ConformingDisjoint(fresh.cells[i], fresh.cells[j]))
          throw std::runtime_error("Region has overlapping/nonconforming tetrahedra.");
      }
    }
  }
};
}  // namespace
bool ConstructWallRegion(const std::vector<Facet>& skin, Point kernel, const PlanarReference& geometry,
                         const std::map<int, RegionWallPolicy>& policies, double tolerance, Id first_node,
                         Id first_cell, FrozenField& target, const RegionLimits& limits, Patch& output,
                         RegionReport& report, std::string& reason) {
  report = {};
  try {
    if (policies.size() != 1 || !(tolerance > 0) || !limits.maximum_cells || !limits.maximum_surface_edits)
      throw std::runtime_error("Region construction needs one wall policy and positive admission limits.");
    const auto p = policies.begin()->second;
    if (!(p.height > 0 && p.growth >= 1 && p.thickness > p.height) || !std::isfinite(p.height + p.growth + p.thickness))
      throw std::runtime_error("Invalid region wall policy.");
    Construction c(geometry, policies, tolerance, target, limits, report, first_node, first_cell);
    std::set<Id> wall_authorities;
    for (const auto& f : skin) {
      if (!geometry.CompleteAnalytic(f.id))
        throw std::runtime_error("Region requires complete declared geometry authority.");
      const auto chart = geometry.Canonical(f.id);
      const auto& a = geometry.Analytic().at(chart);
      if (c.Wall(f.id)) {
        if (a.kind != AnalyticSurface::Kind::Cylinder && a.kind != AnalyticSurface::Kind::Sphere)
          throw std::runtime_error("Region wall has unsupported inward geometry.");
        wall_authorities.insert(chart);
      } else if (a.kind != AnalyticSurface::Kind::Disk)
        throw std::runtime_error("Region nonwall boundary must be planar.");
      SurfaceFace face;
      face.facet = f.id;
      for (unsigned k = 0; k < 3; ++k) {
        const auto n = f.v[k];
        if (n.id >= first_node) throw std::runtime_error("Region new ID reservation overlaps retained skin.");
        auto inserted = c.nodes.emplace(n.id, n.p);
        if (!inserted.second && Norm(Sub(inserted.first->second, n.p)) != 0)
          throw std::runtime_error("Region skin repeats inconsistent coordinates.");
        face.v[k] = n.id;
      }
      c.faces.push_back(face);
    }
    if (wall_authorities.size() != 1) throw std::runtime_error("Region needs one connected complete wall authority.");
    std::vector<Face> original_skin;
    for (auto f : c.faces) original_skin.push_back(f.v);
    ValidateDomainSurfaceTopology(original_skin);
    c.SplitSurface();
    c.CoarsenSurface(kernel);
    for (auto f : c.faces)
      if (!(Dot(c.Normal(f), Sub(kernel, c.nodes.at(f.v[0]))) < 0))
        throw std::runtime_error("Region lacks a visible interior kernel.");
    const double requested = .9 * MaximumMetricEdge,
                 depth = p.growth == 1 ? p.height * requested
                                       : p.height *
                                             (.5 * (p.growth - 1) +
                                              .5 * (p.growth + 1) *
                                                  std::expm1((requested - .5) * 2 * (p.growth - 1) / (p.growth + 1))) /
                                             (p.growth - 1);
    if (!(depth > p.height && depth < p.thickness))
      throw std::runtime_error("Region first band does not fit the requested thickness.");
    auto flow = c.Fit(c.faces, depth);
    auto front = c.MapFront(c.faces, flow, 1);
    kernel = flow.Apply(kernel);
    std::vector<SurfaceFace> wall, inner;
    std::vector<Id> apices;
    for (auto f : c.faces)
      if (c.Wall(f.facet)) {
        wall.push_back(f);
        apices.push_back(c.New(Sub(c.Center(f), Mul(c.Normal(f), p.height))));
      }
    std::map<Edge, Id> side;
    std::map<int, std::vector<std::array<Id, 3>>> rim;
    for (const auto& edge : c.Links(c.faces)) {
      if (edge.second.size() != 2) throw std::runtime_error("Region skin is not closed.");
      std::vector<Use> walls, other;
      for (auto u : edge.second) (c.Wall(c.faces[u.face].facet) ? walls : other).push_back(u);
      if (walls.size() == 1) {
        if (other.size() != 1) throw std::runtime_error("Region rim lacks a planar cap.");
        const auto u = other.front();
        const Id f = c.faces[u.face].facet;
        side[edge.first] = f;
        rim[geometry.Get(f).marker].push_back({front.at(u.a), front.at(u.b), f});
      }
    }
    c.Band(wall, front, apices, side, true);
    for (auto f : wall) {
      for (auto& n : f.v) n = front.at(n);
      inner.push_back(f);
    }
    auto close = [&](std::vector<SurfaceFace>& fs, Point k, const std::map<Id, Id>* mapped) {
      for (const auto& loop : rim) {
        const Id facet = loop.second.front()[2], center = c.New(geometry.Project(facet, k));
        std::vector<std::pair<Id, Id>> edges;
        for (auto row : loop.second) {
          const Id a = mapped ? mapped->at(row[0]) : row[0], b = mapped ? mapped->at(row[1]) : row[1];
          edges.push_back({a, b});
          fs.push_back({{center, a, b}, row[2]});
        }
        c.VerifyLoop(edges);
      }
    };
    close(inner, kernel, nullptr);
    flow = c.Fit(inner, p.thickness);
    std::set<Id> ids;
    for (auto f : inner)
      for (Id n : f.v) ids.insert(n);
    auto clearance = [&](double step) {
      double d = INFINITY;
      for (Id n : ids)
        d = std::min(d, geometry.ClosestAnalytic(*wall_authorities.begin(), flow.Apply(c.nodes.at(n), step)).distance);
      return d;
    };
    if (!(clearance(1) > p.thickness * (1 + 1e-10)))
      throw std::runtime_error("Region transition cannot reach outside the original BL fade.");
    double lo = 0, hi = 1;
    for (unsigned k = 0; k < 48; ++k) {
      const double mid = .5 * (lo + hi);
      if (clearance(mid) >= p.thickness * (1 + 1e-10))
        hi = mid;
      else
        lo = mid;
    }
    const auto last = c.MapFront(inner, flow, hi);
    kernel = flow.Apply(kernel, hi);
    wall.clear();
    apices.clear();
    std::map<Edge, Id> side2;
    for (auto f : inner)
      if (c.Wall(f.facet)) {
        wall.push_back(f);
        SurfaceFace mapped = f;
        for (auto& n : mapped.v) n = last.at(n);
        apices.push_back(c.New(Mul(Add(c.Center(f), c.Center(mapped)), .5)));
      }
    for (const auto& loop : rim)
      for (auto row : loop.second) side2[Key(row[0], row[1])] = row[2];
    c.Band(wall, last, apices, side2, false);
    std::vector<SurfaceFace> final;
    for (auto f : wall) {
      for (auto& n : f.v) n = last.at(n);
      final.push_back(f);
    }
    const Id core = c.New(kernel);
    close(final, kernel, &last);
    for (auto f : final) {
      c.Append({core, f.v[0], f.v[1], f.v[2]});
      if (!c.Wall(f.facet)) c.fresh.surface.push_back(f);
    }
    c.PlaceJunctions();
    try {
      c.Validate();
    } catch (const std::exception& e) {
      if (limits.rejected) limits.rejected(c.fresh, e.what());
      throw;
    }
    output = std::move(c.fresh);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
}  // namespace SU2Native3D
