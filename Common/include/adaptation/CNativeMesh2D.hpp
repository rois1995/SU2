/*!
 * \file CNativeMesh2D.hpp
 * \brief The part of a new mesh that one rank reads (the arrays of a mesh reader on that rank), built without
 *        giving every rank the complete mesh.
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once
// Bounded native bulk reconstruction, ported from the audited standalone experiment.
// Orientation is evaluated by a separately compiled strict-arithmetic predicate.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace SU2Native2D {
using Id = std::uint64_t;
struct Point {
  double x = 0, y = 0;
  template <class Stream>
  void Fields(Stream& s) {
    s(x, y);
  }
};
inline Point operator+(Point a, Point b) { return {a.x + b.x, a.y + b.y}; }
inline Point operator-(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
inline Point operator*(Point a, double s) { return {a.x * s, a.y * s}; }
inline double dot(Point a, Point b) { return a.x * b.x + a.y * b.y; }
inline double norm(Point a) { return std::sqrt(dot(a, a)); }
/*! Sign is exact for finite binary64 inputs; returned magnitude is rounded. */
long double Orientation(double ax, double ay, double bx, double by, double cx, double cy);
/*! Scale-normalized determinant evaluated in the strict-arithmetic unit; invalid tensors return -1. */
long double NormalizedDeterminant(double xx, double xy, double yy);
inline long double orient(Point a, Point b, Point c) { return Orientation(a.x, a.y, b.x, b.y, c.x, c.y); }
inline bool OnSegment(Point a, Point b, Point p) {
  return orient(a, b, p) == 0 && p.x >= std::min(a.x, b.x) && p.x <= std::max(a.x, b.x) && p.y >= std::min(a.y, b.y) &&
         p.y <= std::max(a.y, b.y);
}
inline bool Opposite(long double a, long double b) { return (a > 0 && b < 0) || (a < 0 && b > 0); }
inline bool SegmentsIntersect(Point a, Point b, Point c, Point d) {
  return (Opposite(orient(a, b, c), orient(a, b, d)) && Opposite(orient(c, d, a), orient(c, d, b))) ||
         OnSegment(a, b, c) || OnSegment(a, b, d) || OnSegment(c, d, a) || OnSegment(c, d, b);
}
struct Node {
  Id id = 0;
  Point p;
  int fixed = 0;
  template <class Stream>
  void Fields(Stream& s) {
    s(id, p, fixed);
  }
};
struct Triangle {
  Id id = 0;
  std::array<Node, 3> v;
  int protected_cell = 0;
  template <class Stream>
  void Fields(Stream& s) {
    s(id, v, protected_cell);
  }
};
struct Tensor {
  double xx, xy, yy;
  double quadratic(Point e) const { return xx * e.x * e.x + 2 * xy * e.x * e.y + yy * e.y * e.y; }
  double determinant() const { return xx * yy - xy * xy; }
  template <class Stream>
  void Fields(Stream& s) {
    s(xx, xy, yy);
  }
};
using Metric = std::function<Tensor(Point)>;
// Optional geometric constraints are applied AFTER interpolation of the frozen
// sensor tensor. Interpolating their nodal samples would spread a thin layer
// across a coarse seed simplex.
using MetricComposition = std::function<Tensor(Point, Tensor)>;
using Edge = std::pair<Id, Id>;
inline Edge edge(Id a, Id b) { return std::minmax(a, b); }
inline Point centroid(const Triangle& t) {
  return t.v[0].p + ((t.v[1].p - t.v[0].p) + (t.v[2].p - t.v[0].p)) * (1. / 3);
}
inline long double area(const Triangle& t) { return orient(t.v[0].p, t.v[1].p, t.v[2].p) / 2; }
inline Triangle triangle(Node a, Node b, Node c) {
  Triangle t{0, {{a, b, c}}, 0};
  if (area(t) < 0) std::swap(t.v[1], t.v[2]);
  return t;
}
inline double quality(const Triangle& t, const Metric& metric) {
  const auto raw = metric(centroid(t));
  const double scale = std::max({std::abs(raw.xx), std::abs(raw.xy), std::abs(raw.yy)});
  if (!(std::isfinite(scale) && std::isfinite(raw.xx) && std::isfinite(raw.xy) && std::isfinite(raw.yy) && scale > 0))
    return -1;
  const long double xx = static_cast<long double>(raw.xx) / scale, xy = static_cast<long double>(raw.xy) / scale,
                    yy = static_cast<long double>(raw.yy) / scale;
  const long double determinant = NormalizedDeterminant(raw.xx, raw.xy, raw.yy);
  if (!(xx > 0 && yy > 0 && determinant > 0)) return -1;
  long double sum = 0;
  for (int i = 0; i < 3; ++i) {
    const long double x = static_cast<long double>(t.v[(i + 1) % 3].p.x) - t.v[i].p.x;
    const long double y = static_cast<long double>(t.v[(i + 1) % 3].p.y) - t.v[i].p.y;
    sum += xx * x * x + 2 * xy * x * y + yy * y * y;
  }
  const auto q = 4 * std::sqrt(3.L) * area(t) * std::sqrt(determinant) / sum;
  return std::isfinite(q) && q >= 0 && q <= 1 + 1e-12L ? static_cast<double>(q) : -1;
}
inline double length(Node a, Node b, const Metric& metric) {
  const long double x = static_cast<long double>(b.p.x) - a.p.x, y = static_cast<long double>(b.p.y) - a.p.y;
  const Point middle{static_cast<double>((static_cast<long double>(a.p.x) + b.p.x) / 2),
                     static_cast<double>((static_cast<long double>(a.p.y) + b.p.y) / 2)};
  auto at = [&](Point point) {
    const auto m = metric(point);
    const long double squared = static_cast<long double>(m.xx) * x * x + 2 * static_cast<long double>(m.xy) * x * y +
                                static_cast<long double>(m.yy) * y * y;
    if (!(std::isfinite(squared) && squared >= 0)) throw std::runtime_error("Unusable metric edge length.");
    return std::sqrt(squared);
  };
  return static_cast<double>((at(a.p) + 4 * at(middle) + at(b.p)) / 6);
}
inline double min_quality(const std::vector<Triangle>& cells, const Metric& metric) {
  double q = 1;
  for (const auto& t : cells) q = std::min(q, quality(t, metric));
  return q;
}
inline double max_length(const std::vector<Triangle>& cells, const Metric& metric) {
  double result = 0;
  for (const auto& t : cells)
    for (int i = 0; i < 3; ++i) result = std::max(result, length(t.v[i], t.v[(i + 1) % 3], metric));
  return result;
}
inline std::map<Id, Node> nodes(const std::vector<Triangle>& cells) {
  std::map<Id, Node> result;
  for (const auto& t : cells)
    for (auto v : t.v) {
      auto old = result.find(v.id);
      if (old != result.end() && (old->second.p.x != v.p.x || old->second.p.y != v.p.y || old->second.fixed != v.fixed))
        throw std::runtime_error("Inconsistent shared node");
      result[v.id] = v;
    }
  return result;
}
inline std::map<Edge, std::pair<Node, Node>> boundary(const std::vector<Triangle>& cells) {
  std::map<Edge, int> counts;
  std::map<Edge, std::pair<Node, Node>> ends;
  for (const auto& t : cells)
    for (int i = 0; i < 3; ++i) {
      Node a = t.v[i], b = t.v[(i + 1) % 3];
      auto e = edge(a.id, b.id);
      ++counts[e];
      ends[e] = {a, b};
    }
  std::map<Edge, std::pair<Node, Node>> result;
  for (auto entry : counts) {
    if (entry.second > 2) throw std::runtime_error("Nonmanifold cavity");
    if (entry.second == 1) result[entry.first] = ends[entry.first];
  }
  return result;
}
inline bool validate_replacement(const std::vector<Triangle>& old, const std::vector<Triangle>& fresh,
                                 std::string& reason) {
  if (fresh.empty()) {
    reason = "empty replacement";
    return false;
  }
  long double a = 0, b = 0;
  for (const auto& t : old) a += area(t);
  for (const auto& t : fresh) {
    if (!(area(t) > 0)) {
      reason = "nonpositive cell";
      return false;
    }
    for (auto v : t.v)
      if (!std::isfinite(v.p.x) || !std::isfinite(v.p.y)) {
        reason = "nonfinite node";
        return false;
      }
    b += area(t);
  }
  if (std::abs(b - a) > 1e-10L * a) {
    reason = "cavity area changed";
    return false;
  }
  try {
    nodes(fresh);
    auto before = boundary(old), after = boundary(fresh);
    if (before.size() != after.size()) {
      reason = "cavity boundary size changed";
      return false;
    }
    for (auto e : before) {
      auto f = after.find(e.first);
      if (f == after.end()) {
        reason = "cavity boundary edge changed";
        return false;
      }
      const auto& u = e.second;
      const auto& v = f->second;
      if (u.first.id != v.first.id || u.second.id != v.second.id || u.first.p.x != v.first.p.x ||
          u.first.p.y != v.first.p.y || u.second.p.x != v.second.p.x || u.second.p.y != v.second.p.y) {
        reason = "cavity boundary geometry or orientation changed";
        return false;
      }
    }
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
  return true;
}

inline bool star_polygon(const std::vector<Triangle>& star, Id removed, std::vector<Node>& polygon) {
  std::map<Id, Node> successor;
  for (auto e : boundary(star)) {
    const auto a = e.second.first, b = e.second.second;
    if (a.id == removed || b.id == removed || successor.count(a.id)) return false;
    successor[a.id] = b;
  }
  if (successor.size() < 3) return false;
  const auto all = nodes(star);
  Id first = successor.begin()->first, current = first;
  do {
    if (!successor.count(current) || polygon.size() >= successor.size()) return false;
    polygon.push_back(all.at(current));
    current = successor.at(current).id;
  } while (current != first);
  return polygon.size() == successor.size();
}
inline bool ear_clip(std::vector<Node> polygon, const Metric& metric, std::vector<Triangle>& result) {
  while (polygon.size() > 3) {
    int selected = -1;
    double best = -1;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
      const std::size_t left = (i + polygon.size() - 1) % polygon.size(), right = (i + 1) % polygon.size();
      Triangle t{0, {{polygon[left], polygon[i], polygon[right]}}, 0};
      if (!(area(t) > 0)) continue;
      bool blocked = false;
      for (std::size_t j = 0; j < polygon.size(); ++j) {
        if (j == left || j == i || j == right) continue;
        const auto p = polygon[j].p;
        if (orient(t.v[0].p, t.v[1].p, p) >= 0 && orient(t.v[1].p, t.v[2].p, p) >= 0 &&
            orient(t.v[2].p, t.v[0].p, p) >= 0) {
          blocked = true;
          break;
        }
      }
      // A blocked ear is outside the admitted triangulation. It may cross a
      // physical hole and must never query the donor metric there.
      if (blocked) continue;
      double q = quality(t, metric);
      if (q > best) {
        best = q;
        selected = (int)i;
      }
    }
    if (selected < 0) return false;
    const auto i = (std::size_t)selected;
    result.push_back(
        {0, {{polygon[(i + polygon.size() - 1) % polygon.size()], polygon[i], polygon[(i + 1) % polygon.size()]}}, 0});
    polygon.erase(polygon.begin() + selected);
  }
  Triangle final{0, {{polygon[0], polygon[1], polygon[2]}}, 0};
  if (!(area(final) > 0)) return false;
  result.push_back(final);
  return true;
}

enum class Kind : int { REMOVE = 0, SPLIT = 1, FLIP = 2, MOVE = 3 };
inline bool MovementGoal(const std::map<Id, Node>& all, Id id, const Metric& metric, Point& goal) {
  const auto origin = all.at(id).p;
  std::vector<std::pair<Point, Tensor>> neighbors;
  double scale = 0;
  for (const auto& entry : all)
    if (entry.first != id) {
      const auto offset = entry.second.p - origin;
      const auto m = metric(origin + offset * .5);
      scale = std::max({scale, std::abs(m.xx), std::abs(m.xy), std::abs(m.yy)});
      neighbors.emplace_back(offset, m);
    }
  if (!(scale > 0 && std::isfinite(scale))) return false;
  long double xx = 0, xy = 0, yy = 0, rx = 0, ry = 0;
  for (const auto& entry : neighbors) {
    const auto& m = entry.second;
    const long double a = static_cast<long double>(m.xx) / scale, b = static_cast<long double>(m.xy) / scale,
                      c = static_cast<long double>(m.yy) / scale;
    xx += a;
    xy += b;
    yy += c;
    rx += a * entry.first.x + b * entry.first.y;
    ry += b * entry.first.x + c * entry.first.y;
  }
  const auto det = xx * yy - xy * xy;
  if (!(det > 0 && std::isfinite(det))) return false;
  goal = {static_cast<double>(origin.x + (yy * rx - xy * ry) / det),
          static_cast<double>(origin.y + (xx * ry - xy * rx) / det)};
  return std::isfinite(goal.x) && std::isfinite(goal.y);
}

/*! Improve a complete movable star without making an admitted edge inadmissible.
 * A metric Laplacian is only a proposal: sharp P1 transitions need a bounded
 * search in metric coordinates. Every trial keeps the old perimeter and orientation. */
inline bool MoveStar(const std::vector<Triangle>& old, Id id, const Metric& metric,
                     std::vector<Triangle>& fresh, std::string& reason, const Point* tangent = nullptr) {
  const auto all = nodes(old);
  const auto origin = all.at(id).p;
  double best = min_quality(old, metric);
  std::map<Edge, double> limits;
  double bestCost = 0;
  for (const auto& t : old)
    for (int k = 0; k < 3; ++k) {
      const auto a = t.v[k], b = t.v[(k + 1) % 3];
      const double l = length(a, b, metric);
      if (limits.emplace(edge(a.id, b.id), std::max(1.8, l)).second && (a.id == id || b.id == id))
        bestCost += std::pow(std::max(0., l - 1.8), 2);
    }
  // For an already legal shape, reduce oversized spokes without requiring
  // further shape improvement. Fixed topology makes this deficit comparable.
  const bool sizeRepair = best >= .18 && bestCost > 0;
  fresh = old;
  Point chosen = origin;
  bool improved = false;
  auto trial = [&](Point position) {
    if (tangent) position = origin + *tangent * dot(position - origin, *tangent);
    auto candidate = old;
    for (auto& t : candidate)
      for (auto& v : t.v)
        if (v.id == id) v.p = position;
    if (!validate_replacement(old, candidate, reason)) return;
    const double q = min_quality(candidate, metric);
    if (sizeRepair ? q < .18 : !(q > best + 1e-4)) return;
    for (size_t i = 0; i < old.size(); ++i)
      if (old[i].protected_cell && quality(candidate[i], metric) < std::min(.18, quality(old[i], metric))) return;
    const auto proposed = nodes(candidate);
    double cost = 0;
    for (const auto& entry : limits) {
      const auto a = proposed.at(entry.first.first), b = proposed.at(entry.first.second);
      const auto l = length(a, b, metric), cap = entry.second;
      if (l > cap + 1e-12 * cap) return;
      if (a.id == id || b.id == id) cost += std::pow(std::max(0., l - 1.8), 2);
    }
    if (sizeRepair && !(cost < bestCost * (1 - 1e-8))) return;
    bestCost = cost;
    best = q;
    chosen = position;
    fresh.swap(candidate);
    improved = true;
  };
  Point goal;
  if (MovementGoal(all, id, metric, goal))
    for (double fraction = 1; fraction >= 1. / 64; fraction *= .5) {
      trial(origin + (goal - origin) * fraction);
      if (improved) break;
    }
  auto needsSearch = [&] { return sizeRepair ? bestCost > 0 : best < .22; };
  if (needsSearch()) {
    const auto m = metric(origin);
    const double scale = std::max({std::abs(m.xx), std::abs(m.xy), std::abs(m.yy)});
    const double xx = m.xx / scale, xy = m.xy / scale, yy = m.yy / scale;
    const double largest = .5 * (xx + yy + std::hypot(xx - yy, 2 * xy));
    const double smallest = static_cast<double>(NormalizedDeterminant(m.xx, m.xy, m.yy)) / largest;
    const double angle = .5 * std::atan2(2 * xy, xx - yy);
    const double factor = 1 / std::sqrt(scale);
    const Point major{std::cos(angle) * factor / std::sqrt(largest),
                      std::sin(angle) * factor / std::sqrt(largest)};
    const Point minor{-std::sin(angle) * factor / std::sqrt(smallest),
                       std::cos(angle) * factor / std::sqrt(smallest)};
    const std::array<Point, 8> directions{{major, major * -1, minor, minor * -1,
                                         (major + minor) * std::sqrt(.5), (major - minor) * std::sqrt(.5),
                                         (minor - major) * std::sqrt(.5), (major + minor) * -std::sqrt(.5)}};
    for (double step = .5; step >= 1. / 512 && needsSearch(); step *= .5)
      for (int sweep = 0; sweep < 2 && needsSearch(); ++sweep) {
        const auto center = chosen;
        const auto before = chosen;
        for (const auto direction : directions) trial(center + direction * step);
        if (chosen.x == before.x && chosen.y == before.y) break;
      }
  }
  if (!improved) reason = "no improving valid movement";
  return improved;
}

struct Request {
  Kind kind = Kind::REMOVE;
  Id a = 0, b = 0;
  double score = -1;
};
inline bool affected(const Triangle& t, Request r) {
  bool a = false, b = false;
  for (auto v : t.v) {
    a |= v.id == r.a;
    b |= v.id == r.b;
  }
  return a && ((r.kind == Kind::REMOVE || r.kind == Kind::MOVE) || b);
}
constexpr std::size_t MAX_CAVITY = 32;

inline bool reconstruct(Request r, const std::vector<Triangle>& old, const Metric& metric, Id new_point,
                        std::vector<Triangle>& fresh, std::string& reason);

/*! Reconnect a private triangulation before enforcing its final shape floor.
 * Local deficit reduction can repair tied bad cells without worsening other
 * cells. A size insertion must not restore the edge it was asked to refine. */
inline void RepairShape(std::vector<Triangle>& cells, const Metric& metric, double floor,
                        Edge forbidden = {0, 0}) {
  auto deficit = [&](const std::vector<Triangle>& input) {
    double value = 0;
    for (const auto& t : input) value += std::max(0., floor - quality(t, metric));
    return value;
  };
  for (size_t pass = 0; pass < 2 * cells.size() && min_quality(cells, metric) < floor; ++pass) {
    std::map<Edge, std::vector<size_t>> incidence;
    for (size_t i = 0; i < cells.size(); ++i)
      for (int k = 0; k < 3; ++k) incidence[edge(cells[i].v[k].id, cells[i].v[(k + 1) % 3].id)].push_back(i);
    bool improved = false;
    for (const auto& entry : incidence) {
      if (entry.second.size() != 2) continue;
      const auto left = entry.second[0], right = entry.second[1];
      const std::vector<Triangle> old{cells[left], cells[right]};
      std::vector<Triangle> fresh;
      std::string reason;
      if (!reconstruct({Kind::FLIP, entry.first.first, entry.first.second, 1}, old, metric, 0, fresh, reason)) continue;
      bool restores = false;
      for (const auto& t : fresh)
        for (int k = 0; k < 3; ++k) restores |= edge(t.v[k].id, t.v[(k + 1) % 3].id) == forbidden;
      if (restores || !(deficit(fresh) < deficit(old) - 1e-10)) continue;
      cells[left] = fresh[0];
      cells[right] = fresh[1];
      improved = true;
      break;
    }
    if (!improved) break;
  }
}

/*! Insert in an edge star, then absorb admitted adjacent cells from the bounded
 * dependency patch. Reconnection happens before publication, so midpoint
 * children need not survive until a later global flip phase. Unabsorbed cells,
 * protected wall children and the patch perimeter retain their geometry. */
inline bool SplitPatch(Request r, const std::vector<Triangle>& old, const Metric& metric, Id new_point,
                       std::vector<Triangle>& fresh, std::string& reason) {
  const auto all = nodes(old);
  if (!all.count(r.a) || !all.count(r.b)) {
    reason = "missing requested edge";
    return false;
  }
  const auto a = all.at(r.a), b = all.at(r.b);
  Node point{new_point, a.p + (b.p - a.p) * .5, 0};
  std::set<size_t> selected;
  for (size_t k = 0; k < old.size(); ++k)
    if (affected(old[k], r)) {
      if (old[k].protected_cell) {
        reason = "protected first layer";
        return false;
      }
      selected.insert(k);
    }
  if (selected.size() != 2) {
    reason = "not a complete interior edge star";
    return false;
  }
  auto fan = [&](const std::set<size_t>& members, std::vector<Triangle>& output) {
    std::vector<Triangle> input;
    for (const auto k : members) input.push_back(old[k]);
    output.clear();
    for (const auto& face : boundary(input)) {
      const auto u = face.second.first, v = face.second.second;
      if (!(orient(u.p, v.p, point.p) > 0)) return false;
      output.push_back({0, {{u, v, point}}, 0});
    }
    // Geometric admission, including orientation and unchanged area/perimeter,
    // precedes all donor queries in this enlarged cavity.
    return validate_replacement(input, output, reason);
  };
  std::vector<Triangle> current;
  if (!fan(selected, current)) return false;
  const bool sizeInsertion = length(a, b, metric) > 1.6;
  double fraction = .5;
  if (sizeInsertion) {
    // Equal Euclidean halves need not fit a graded metric. Use the same
    // Simpson-length balance as surface splitting, retaining the edge geometry.
    double lo = 0, hi = 1;
    for (int iteration = 0; iteration < 32; ++iteration) {
      fraction = .5 * (lo + hi);
      const Node trial{new_point, a.p + (b.p - a.p) * fraction, 0};
      const double left = length(a, trial, metric), right = length(trial, b, metric);
      if (left == right) break;
      if (left < right) lo = fraction;
      else hi = fraction;
    }
  }
  point.p = a.p + (b.p - a.p) * fraction;
  if (!fan(selected, current)) return false;
  auto sizeDeficit = [&](const std::vector<Triangle>& cells) {
    std::set<Edge> seen;
    long double value = 0;
    for (const auto& t : cells)
      for (int k = 0; k < 3; ++k) {
        const auto u = t.v[k], v = t.v[(k + 1) % 3];
        if (!seen.insert(edge(u.id, v.id)).second) continue;
        const long double excess = std::max(0., length(u, v, metric) - 1.8);
        value += excess * excess;
      }
    return value;
  };
  auto sizeProgress = [&](const std::vector<Triangle>& before, const std::vector<Triangle>& after) {
    if (sizeInsertion) {
      // Ordinary refinement adds resolution. Only absorption of an existing
      // free vertex can undo it; require deficit progress for that replacement.
      const auto previous = nodes(before), next = nodes(after);
      const bool removed = std::any_of(previous.begin(), previous.end(),
                                      [&](const auto& node) { return !next.count(node.first); });
      if (!removed) return true;
      const auto oldCost = sizeDeficit(before), newCost = sizeDeficit(after);
      return oldCost > 0 ? newCost < oldCost : newCost == 0;
    }
    // Pure shape repair may retain a long artificial perimeter, but cannot
    // create a new oversized interior edge while replacing a free vertex.
    const auto perimeter = boundary(before);
    for (const auto& t : after)
      for (int k = 0; k < 3; ++k)
        if (!perimeter.count(edge(t.v[k].id, t.v[(k + 1) % 3].id)) &&
            length(t.v[k], t.v[(k + 1) % 3], metric) > 1.8) return false;
    return true;
  };
  for (size_t iteration = 0; iteration < old.size(); ++iteration) {
    const double current_min = min_quality(current, metric);
    if (current_min >= .22) break;
    double penalty = 0;
    for (const auto& t : current) penalty += std::max(.22 - quality(t, metric), 0.);
    const auto perimeter = boundary(current);
    const double current_max = max_length(current, metric);
    double best_gain = 1e-5;
    std::set<size_t> chosen;
    std::vector<Triangle> best;
    auto consider = [&](const std::set<size_t>& members, bool optimize) {
      std::vector<Triangle> trial, added;
      for (const auto k : members)
        if (!selected.count(k)) added.push_back(old[k]);
      if (added.empty() || !fan(members, trial)) return;
      if (optimize && min_quality(trial, metric) < .22) {
        std::vector<Triangle> moved;
        std::string why;
        if (MoveStar(trial, new_point, metric, moved, why)) trial.swap(moved);
      }
      const double old_q = min_quality(added, metric);
      if (min_quality(trial, metric) + 1e-7 < std::min(current_min, old_q)) return;
      double next_penalty = 0, added_penalty = 0;
      for (const auto& t : trial) next_penalty += std::max(.22 - quality(t, metric), 0.);
      for (const auto& t : added) added_penalty += std::max(.22 - quality(t, metric), 0.);
      const double gain = penalty + added_penalty - next_penalty;
      if (!(gain > best_gain)) return;
      std::vector<Triangle> replaced;
      for (const auto k : members) replaced.push_back(old[k]);
      if (!sizeProgress(replaced, trial)) return;
      const double cap = std::max({1.8, current_max, max_length(added, metric)});
      if (max_length(trial, metric) > cap + 1e-12 * cap) return;
      chosen = members;
      best_gain = gain;
      best = std::move(trial);
    };
    for (size_t k = 0; k < old.size(); ++k) {
      if (selected.count(k) || old[k].protected_cell) continue;
      bool adjacent = false;
      for (int slot = 0; slot < 3; ++slot)
        adjacent |= perimeter.count(edge(old[k].v[slot].id, old[k].v[(slot + 1) % 3].id));
      if (!adjacent) continue;
      auto members = selected;
      members.insert(k);
      consider(members, false);
    }
    // A free perimeter vertex can hide a complete star behind a concave
    // intermediate fan. Absorb its COMPLETE imported star in one proposal,
    // then optimize the new point privately. Every removed vertex must be
    // interior to this dependency patch; protected cells remain untouched.
    const auto patchBoundary = boundary(old);
    std::set<Id> exposed;
    for (const auto& face : patchBoundary) {
      exposed.insert(face.first.first);
      exposed.insert(face.first.second);
    }
    std::set<Id> closurePoints;
    for (const auto& face : perimeter) {
      closurePoints.insert(face.first.first);
      closurePoints.insert(face.first.second);
    }
    for (const auto id : closurePoints) {
      if (all.at(id).fixed || exposed.count(id)) continue;
      auto members = selected;
      bool protectedStar = false;
      for (size_t k = 0; k < old.size(); ++k)
        for (const auto& v : old[k].v)
          if (v.id == id) {
            members.insert(k);
            protectedStar |= old[k].protected_cell != 0;
          }
      if (!protectedStar) consider(members, true);
    }
    if (chosen.empty()) break;
    selected = std::move(chosen);
    current = std::move(best);
  }
  // A new free point also repairs shallow ears with three fixed boundary
  // vertices. Improve it while its complete fan is still private, using the
  // same geometry/per-edge admission as ordinary star movement.
  if (min_quality(current, metric) < .22) {
    std::vector<Triangle> moved;
    std::string movementReason;
    if (MoveStar(current, new_point, metric, moved, movementReason)) current.swap(moved);
  }
  std::vector<Triangle> replaced;
  for (const auto k : selected) replaced.push_back(old[k]);
  const double before = min_quality(replaced, metric);
  RepairShape(current, metric, std::min(.18, before), sizeInsertion ? edge(r.a, r.b) : Edge{0, 0});
  const double after = min_quality(current, metric);
  if (after < std::min(.18, before) * (1 - 1e-12)) {
    reason = "insertion worsens sub-threshold shape";
    return false;
  }
  // A fitting edge is inserted only to repair shape. Without this progress
  // check a failed repair repeatedly bisects the same shallow ear.
  if (!sizeProgress(replaced, current)) {
    reason = "insertion does not reduce length deficits";
    return false;
  }
  if (!sizeInsertion && after <= before + 1e-7) {
    reason = "insertion does not improve shape";
    return false;
  }
  fresh = std::move(current);
  for (size_t k = 0; k < old.size(); ++k)
    if (!selected.count(k)) fresh.push_back(old[k]);
  return validate_replacement(old, fresh, reason);
}

inline bool reconstruct(Request r, const std::vector<Triangle>& old, const Metric& metric, Id new_point,
                        std::vector<Triangle>& fresh, std::string& reason) {
  if (old.empty() || old.size() > MAX_CAVITY) {
    reason = "cavity size limit";
    return false;
  }
  for (const auto& t : old)
    if (t.protected_cell && r.kind != Kind::SPLIT) {
      reason = "protected first layer";
      return false;
    }
  const auto all = nodes(old);
  if (!all.count(r.a)) {
    reason = "missing requested node";
    return false;
  }
  if (r.kind == Kind::REMOVE) {
    if (all.at(r.a).fixed) {
      reason = "fixed node";
      return false;
    }
    std::vector<Node> polygon;
    if (!star_polygon(old, r.a, polygon) || !ear_clip(polygon, metric, fresh)) {
      reason = "non-simple star or ear failure";
      return false;
    }
    // Keep the normal coarsening margin. A poor seed needs progressive
    // deletion of thin rows: require nondegradation of its worst quality,
    // then let subsequent cavities repair it. Final acceptance still needs
    // 0.18 everywhere. Requiring 0.22 here traps the original BL topology.
    const double before = min_quality(old, metric);
    const double requiredQuality = before < .18 ? before * (1 - 1e-12) : .22;
    if (min_quality(fresh, metric) < requiredQuality) {
      reason = "coarsening misses target";
      return false;
    }
    // An unchanged artificial perimeter is repaired by adjacent cavities. Only
    // newly introduced edges must meet the coarsening length bound here.
    const auto perimeter = boundary(old);
    for (const auto& t : fresh)
      for (int k = 0; k < 3; ++k)
        if (!perimeter.count(edge(t.v[k].id, t.v[(k + 1) % 3].id)) &&
            length(t.v[k], t.v[(k + 1) % 3], metric) > 1.45) {
          reason = "coarsening misses target";
          return false;
        }
  } else if (r.kind == Kind::SPLIT) {
    return SplitPatch(r, old, metric, new_point, fresh, reason);
  } else if (r.kind == Kind::FLIP) {
    if (old.size() != 2 || !all.count(r.b)) {
      reason = "not a complete interior edge star";
      return false;
    }
    std::vector<Node> other;
    for (auto v : all)
      if (v.first != r.a && v.first != r.b) other.push_back(v.second);
    if (other.size() != 2) {
      reason = "degenerate edge star";
      return false;
    }
    const Node a = all.at(r.a), b = all.at(r.b), c = other[0], d = other[1];
    if (!((orient(c.p, d.p, a.p) > 0 && orient(c.p, d.p, b.p) < 0) ||
          (orient(c.p, d.p, a.p) < 0 && orient(c.p, d.p, b.p) > 0))) {
      reason = "nonconvex quadrilateral";
      return false;
    }
    fresh = {triangle(c, d, a), triangle(d, c, b)};
    // Long unchanged perimeter edges must not prevent an improving reconnection.
    // The replaced diagonal cannot introduce a new length deficit or worsen its old one.
    const double before = min_quality(old, metric), after = min_quality(fresh, metric),
                 oldLength = length(a, b, metric), newLength = length(c, d, metric);
    // A legal shape may still carry an oversized diagonal. Repair its size
    // without requiring an additional shape gain; the reverse flip cannot
    // recreate a long edge once the diagonal fits. Poor shapes cannot worsen.
    const bool sizeRepair = oldLength > 1.8 && newLength < oldLength * (1 - 1e-12) &&
                            after >= std::min(.18, before) * (1 - 1e-12);
    if ((!sizeRepair && after <= before + 1e-7) || newLength > std::max(1.8, oldLength)) {
      reason = "flip does not improve quality";
      return false;
    }
  } else {
    if (all.at(r.a).fixed) {
      reason = "fixed node";
      return false;
    }
    if (!MoveStar(old, r.a, metric, fresh, reason)) return false;
  }
  return validate_replacement(old, fresh, reason);
}

inline std::vector<Request> candidates(const std::vector<Triangle>& local, Kind kind, const Metric& metric) {
  std::map<std::pair<Id, Id>, Request> result;
  for (const auto& t : local) {
    if (t.protected_cell) continue;
    for (int i = 0; i < 3; ++i) {
      const Node a = t.v[i], b = t.v[(i + 1) % 3];
      if (kind == Kind::REMOVE) {
        const double l = length(a, b, metric);
        if (l < .65 && !a.fixed) {
          const auto key = std::make_pair(a.id, Id(0));
          Request r{kind, a.id, 0, .65 - l};
          if (!result.count(key) || result[key].score < r.score) result[key] = r;
        }
        if (l < .65 && !b.fixed) {
          const auto key = std::make_pair(b.id, Id(0));
          Request r{kind, b.id, 0, .65 - l};
          if (!result.count(key) || result[key].score < r.score) result[key] = r;
        }
      } else if (kind == Kind::MOVE) {
        const auto key = std::make_pair(a.id, Id(0));
        Request r{kind, a.id, 0, std::max(1 - quality(t, metric), length(a, b, metric) - 1.8)};
        if (!a.fixed && (!result.count(key) || result[key].score < r.score)) result[key] = r;
      } else {
        const auto e = edge(a.id, b.id);
        const double l = length(a, b, metric);
        if (kind == Kind::SPLIT && l > 1.6) result[e] = {kind, e.first, e.second, l - 1.6};
        if (kind == Kind::FLIP) {
          Request r{kind, e.first, e.second, 1 - quality(t, metric)};
          if (!result.count(e) || result[e].score < r.score) result[e] = r;
        }
      }
    }
  }
  std::vector<Request> output;
  for (auto entry : result) output.push_back(entry.second);
  return output;
}
inline bool strict_cells(const std::vector<Triangle>& cells, std::string& reason) {
  std::set<std::array<Id, 3>> unique;
  std::map<Edge, std::vector<std::pair<Id, Id>>> incidence;
  for (auto t : cells) {
    if (!(area(t) > 0)) {
      reason = "nonpositive cell";
      return false;
    }
    std::array<Id, 3> ids{{t.v[0].id, t.v[1].id, t.v[2].id}};
    std::sort(ids.begin(), ids.end());
    if (!unique.insert(ids).second) {
      reason = "duplicate triangle";
      return false;
    }
    for (int k = 0; k < 3; ++k)
      incidence[edge(t.v[k].id, t.v[(k + 1) % 3].id)].push_back({t.v[k].id, t.v[(k + 1) % 3].id});
  }
  for (auto f : incidence)
    if (f.second.size() > 2 || (f.second.size() == 2 && !(f.second[0].first == f.second[1].second &&
                                                          f.second[0].second == f.second[1].first))) {
      reason = "nonmanifold or equally oriented interior edge";
      return false;
    }
  const auto all = nodes(cells);
  std::set<std::pair<double, double>> positions;
  for (const auto& entry : all)
    if (!positions.emplace(entry.second.p.x, entry.second.p.y).second) {
      reason = "different node identities at the same position";
      return false;
    }
  for (const auto& f : incidence) {
    const auto a = all.at(f.first.first).p, b = all.at(f.first.second).p;
    for (const auto& v : all)
      if (v.first != f.first.first && v.first != f.first.second && OnSegment(a, b, v.second.p)) {
        reason = "nonincident node on edge";
        return false;
      }
    for (const auto& g : incidence)
      if (f.first < g.first && f.first.first != g.first.first && f.first.first != g.first.second &&
          f.first.second != g.first.first && f.first.second != g.first.second &&
          SegmentsIntersect(a, b, all.at(g.first.first).p, all.at(g.first.second).p)) {
        reason = "intersecting nonincident edges";
        return false;
      }
  }
  return true;
}

// Stagnation-only reconstruction. The ordinary 32-cell primitive remains bounded
// separately; MPI imports and reserves this complete patch before private search.
constexpr std::size_t MAX_JOINT_CAVITY = 128;
// ponytail: bounded coordinate descent with two insertions; profile before adding a general optimizer.
// Scoped to one immutable target and one private search. Ordered coordinates retain
// the original arithmetic; changed coordinates or collisions require fresh evaluation.
class PatchScores {
 public:
  static constexpr size_t LIMIT = 1024;
  explicit PatchScores(const Metric& target) : metric(target) {}
  double Length(Node a, Node b) {
    return Get({a.p.x, a.p.y, b.p.x, b.p.y, 0, 0}, 1, [&] { return length(a, b, metric); });
  }
  double Quality(const Triangle& t) {
    return Get({t.v[0].p.x, t.v[0].p.y, t.v[1].p.x, t.v[1].p.y, t.v[2].p.x, t.v[2].p.y}, 2,
               [&] { return quality(t, metric); });
  }

 private:
  struct Entry {
    std::array<double, 6> coordinates{};
    double value = 0;
    int kind = 0;
  };
  std::array<Entry, LIMIT> cache{};
  const Metric& metric;
  template <class Evaluate>
  double Get(const std::array<double, 6>& coordinates, int kind, Evaluate evaluate) {
    size_t hash = kind;
    for (const auto x : coordinates)
      hash ^= std::hash<double>{}(x) + size_t(0x9e3779b9) + (hash << 6) + (hash >> 2);
    auto& entry = cache[hash % LIMIT];
    if (entry.kind == kind && entry.coordinates == coordinates) return entry.value;
    const auto value = evaluate();
    entry = {coordinates, value, kind};
    return value;
  }
};

constexpr size_t PATCH_SCORE_BYTES = sizeof(PatchScores) + (MAX_JOINT_CAVITY + 4) * sizeof(size_t);

inline double SizeDeficit(const std::vector<Triangle>& cells, const Metric& metric, PatchScores* scores = nullptr) {
  std::set<Edge> seen;
  double value = 0;
  for (const auto& t : cells)
    for (int k = 0; k < 3; ++k) {
      const auto a = t.v[k], b = t.v[(k + 1) % 3];
      if (!seen.insert(edge(a.id, b.id)).second) continue;
      const double excess = std::max(0., (scores ? scores->Length(a, b) : length(a, b, metric)) - 1.8);
      value += excess * excess;
    }
  return value;
}
inline Point SplitSeed(const std::vector<Triangle>& old, const Metric& metric, Edge request, Id id) {
  std::vector<Triangle> pair;
  for (const auto& t : old)
    if (affected(t, {Kind::SPLIT, request.first, request.second, 1})) pair.push_back(t);
  if (pair.size() != 2) throw std::runtime_error("not a complete interior edge star");
  const auto all = nodes(pair);
  Point chosen = (all.at(request.first).p + all.at(request.second).p) * .5;
  const auto perimeter = boundary(pair);
  double best = 1e100;
  PatchScores scores(metric);
  auto consider = [&](Point p) {
    std::vector<Triangle> fan;
    for (const auto& e : perimeter) {
      if (orient(e.second.first.p, e.second.second.p, p) <= 0) return;
      fan.push_back({0, {{e.second.first, e.second.second, {id, p, 0}}}, 0});
    }
    double cost = SizeDeficit(fan, metric, &scores);
    for (const auto& t : fan) cost += 1000 * std::max(0., .1 - scores.Quality(t));
    if (cost < best) {
      best = cost;
      chosen = p;
    }
  };
  consider(chosen);
  for (const auto& parent : pair)
    for (int i = 1; i < 40; ++i)
      for (int j = 1; j < 40 - i; ++j)
        consider(parent.v[0].p +
                 ((parent.v[1].p - parent.v[0].p) * i + (parent.v[2].p - parent.v[0].p) * j) * (1. / 40));
  return chosen;
}
inline std::vector<Triangle> JointPatch(const std::vector<Triangle>& old, const Metric& metric, Id first, Point seed,
                                        Edge forbidden) {
  if (old.empty() || old.size() > MAX_JOINT_CAVITY) return {};
  std::vector<Triangle> pair, rest;
  for (const auto& t : old) {
    if (affected(t, {Kind::SPLIT, forbidden.first, forbidden.second, 1}))
      pair.push_back(t);
    else
      rest.push_back(t);
  }
  if (pair.size() != 2 ||
      std::any_of(pair.begin(), pair.end(), [](const Triangle& t) { return t.protected_cell != 0; }))
    return {};
  const auto perimeter = boundary(pair);
  std::set<Id> locked;
  for (const auto& face : boundary(old)) {
    locked.insert(face.first.first);
    locked.insert(face.first.second);
  }
  for (const auto& t : old)
    for (const auto& v : t.v)
      if (t.protected_cell || v.fixed) locked.insert(v.id);
  std::vector<Triangle> fan;
  for (const auto& face : perimeter) {
    if (orient(face.second.first.p, face.second.second.p, seed) <= 0) return {};
    fan.push_back({0, {{face.second.first, face.second.second, {first, seed, 0}}}, 0});
  }
  PatchScores scores(metric);
  const auto sizeCost = [&](const std::vector<Triangle>& ts) { return SizeDeficit(ts, metric, &scores); };
  const double oldCost = sizeCost(old), oldMax = max_length(old, metric);
  std::vector<Triangle> best;
  for (const double weight : {10., 100., 1000.})
    for (size_t slot = 0; slot < fan.size(); ++slot)
      for (const double f : {1. / 3, .6}) {
        auto state = fan;
        state.insert(state.end(), rest.begin(), rest.end());
        const auto parent = state[slot];
        const Node inserted{first + 1, parent.v[2].p * f + (parent.v[0].p + parent.v[1].p) * (.5 * (1 - f)), 0};
        state[slot] = triangle(parent.v[0], parent.v[1], inserted);
        state.push_back(triangle(parent.v[1], parent.v[2], inserted));
        state.push_back(triangle(parent.v[2], parent.v[0], inserted));
        auto score = [&](const std::vector<Triangle>& ts) {
          double value = sizeCost(ts);
          for (const auto& t : ts) {
            const auto q = scores.Quality(t);
            if (!(q > 0)) return 1e100;
            value += weight * std::max(0., .2 - q) + 1e-4 * (1 - q) * (1 - q);
          }
          return value;
        };
        std::array<size_t, MAX_JOINT_CAVITY + 4> scored_indices{};
        size_t scored_count = 0;
        double previous_score = 0;
        bool previous_valid = false;
        auto consider = [&](const std::vector<size_t>& indices, std::vector<Triangle> trial) {
          if (indices.size() > scored_indices.size()) throw std::runtime_error("joint score dependency cap");
          std::vector<Triangle> previous;
          for (const auto index : indices) previous.push_back(state[index]);
          std::string why;
          if (!validate_replacement(previous, trial, why)) return false;
          // Unchanged cell and edge terms cancel. The complete point star or flip
          // pair contains every changed term; its unchanged perimeter also cancels.
          const double trial_score = score(trial);
          if (!previous_valid || scored_count != indices.size() ||
              !std::equal(indices.begin(), indices.end(), scored_indices.begin())) {
            previous_score = score(previous);
            std::copy(indices.begin(), indices.end(), scored_indices.begin());
            scored_count = indices.size();
            previous_valid = true;
          }
          if (!(trial_score < previous_score - 1e-10)) return false;
          for (size_t k = 0; k < indices.size(); ++k) state[indices[k]] = trial[k];
          previous_valid = false;
          return true;
        };
        std::vector<Id> movable;
        for (const auto& entry : nodes(state))
          if (!locked.count(entry.first)) movable.push_back(entry.first);
        for (double step = 1; step >= 1. / 4096; step *= .5)
          for (int sweep = 0; sweep < 8; ++sweep) {
            bool changed = false;
            for (const Id id : movable) {
              const auto all = nodes(state);
              const auto origin = all.at(id).p;
              const auto m = metric(origin);
              const double scale = std::max({std::abs(m.xx), std::abs(m.xy), std::abs(m.yy)});
              const double large = .5 * ((m.xx + m.yy) / scale + std::hypot((m.xx - m.yy) / scale, 2 * m.xy / scale));
              const double small = double(NormalizedDeterminant(m.xx, m.xy, m.yy)) / large;
              const double angle = .5 * std::atan2(2 * m.xy, m.xx - m.yy), factor = 1 / std::sqrt(scale);
              const Point major{std::cos(angle) * factor / std::sqrt(large),
                                std::sin(angle) * factor / std::sqrt(large)};
              const Point minor{-std::sin(angle) * factor / std::sqrt(small),
                                std::cos(angle) * factor / std::sqrt(small)};
              std::vector<Point> directions{major,
                                            major * -1,
                                            minor,
                                            minor * -1,
                                            (major + minor) * std::sqrt(.5),
                                            (major - minor) * std::sqrt(.5),
                                            (minor - major) * std::sqrt(.5),
                                            (major + minor) * -std::sqrt(.5)};
              std::vector<Triangle> star;
              std::vector<size_t> indices;
              for (size_t index = 0; index < state.size(); ++index)
                for (const auto& v : state[index].v)
                  if (v.id == id) {
                    star.push_back(state[index]);
                    indices.push_back(index);
                    break;
                  }
              Point goal;
              if (MovementGoal(nodes(star), id, metric, goal)) directions.push_back(goal - origin);
              for (const auto direction : directions) {
                std::vector<Triangle> trial;
                for (const auto index : indices) trial.push_back(state[index]);
                const auto position = origin + direction * step;
                for (auto& t : trial)
                  for (auto& v : t.v)
                    if (v.id == id) v.p = position;
                changed |= consider(indices, std::move(trial));
              }
            }
            std::map<Edge, std::vector<size_t>> incidence;
            for (size_t i = 0; i < state.size(); ++i)
              for (int k = 0; k < 3; ++k) incidence[edge(state[i].v[k].id, state[i].v[(k + 1) % 3].id)].push_back(i);
            for (const auto& e : incidence) {
              if (e.second.size() != 2) continue;
              const auto l = e.second[0], r = e.second[1];
              if (state[l].protected_cell || state[r].protected_cell) continue;
              const auto local = nodes({state[l], state[r]});
              std::vector<Node> opposite;
              for (const auto& v : local)
                if (v.first != e.first.first && v.first != e.first.second) opposite.push_back(v.second);
              if (opposite.size() != 2 || edge(opposite[0].id, opposite[1].id) == forbidden) continue;
              const auto a = local.at(e.first.first), b = local.at(e.first.second), c = opposite[0], d = opposite[1];
              if (!((orient(c.p, d.p, a.p) > 0 && orient(c.p, d.p, b.p) < 0) ||
                    (orient(c.p, d.p, a.p) < 0 && orient(c.p, d.p, b.p) > 0)))
                continue;
              if (consider({l, r}, {triangle(c, d, a), triangle(d, c, b)})) {
                changed = true;
                break;
              }
            }
            if (!changed) break;
          }
        const double q = min_quality(state, metric), cost = sizeCost(state), L = max_length(state, metric);
        if (q >= .18 && cost < oldCost * (1 - 1e-8) && L <= oldMax * (1 + 1e-12)) {
          best = state;
          std::string why;
          if (!strict_cells(best, why) || !validate_replacement(old, best, why)) throw std::runtime_error(why);
          return best;
        }
      }
  return {};
}

inline bool JointSplitPatch(Request request, const std::vector<Triangle>& old, const Metric& metric, Id first,
                            std::vector<Triangle>& fresh, std::string& reason) {
  fresh.clear();
  if (request.kind != Kind::SPLIT || old.empty() || old.size() > MAX_JOINT_CAVITY) {
    reason = "joint dependency cap or unsupported action";
    return false;
  }
  const double before = SizeDeficit(old, metric), maximum = max_length(old, metric);
  if (!(before > 0)) {
    reason = "joint patch has no length deficit";
    return false;
  }
  if (SplitPatch(request, old, metric, first, fresh, reason) && min_quality(fresh, metric) >= .18 &&
      max_length(fresh, metric) <= maximum * (1 + 1e-12) && SizeDeficit(fresh, metric) < before * (1 - 1e-8) &&
      strict_cells(fresh, reason) && validate_replacement(old, fresh, reason))
    return true;
  fresh = JointPatch(old, metric, first, SplitSeed(old, metric, edge(request.a, request.b), first),
                     edge(request.a, request.b));
  if (fresh.empty()) {
    reason = "joint reconstruction misses shape or size progress";
    return false;
  }
  return true;
}
}  // namespace SU2Native2D
