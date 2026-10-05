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
                     std::vector<Triangle>& fresh, std::string& reason) {
  const auto all = nodes(old);
  const auto origin = all.at(id).p;
  double best = min_quality(old, metric);
  std::map<Edge, double> limits;
  for (const auto& t : old)
    for (int k = 0; k < 3; ++k) {
      const auto a = t.v[k], b = t.v[(k + 1) % 3];
      limits.emplace(edge(a.id, b.id), std::max(1.8, length(a, b, metric)));
    }
  fresh = old;
  Point chosen = origin;
  bool improved = false;
  auto trial = [&](Point position) {
    auto candidate = old;
    for (auto& t : candidate)
      for (auto& v : t.v)
        if (v.id == id) v.p = position;
    if (!validate_replacement(old, candidate, reason)) return;
    const double q = min_quality(candidate, metric);
    if (!(q > best + 1e-4)) return;
    for (const auto& t : candidate)
      for (int k = 0; k < 3; ++k) {
        const auto a = t.v[k], b = t.v[(k + 1) % 3];
        const auto cap = limits.at(edge(a.id, b.id));
        if (length(a, b, metric) > cap + 1e-12 * cap) return;
      }
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
  if (best < .22) {
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
    for (double step = .5; step >= 1. / 512 && best < .22; step *= .5)
      for (int sweep = 0; sweep < 2 && best < .22; ++sweep) {
        const auto center = chosen;
        const auto before = best;
        for (const auto direction : directions) trial(center + direction * step);
        if (best == before) break;
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
                        std::vector<Triangle>& fresh, std::string& reason) {
  if (old.empty() || old.size() > MAX_CAVITY) {
    reason = "cavity size limit";
    return false;
  }
  for (const auto& t : old)
    if (t.protected_cell) {
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
    if (min_quality(fresh, metric) < .22) {
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
    if (old.size() != 2 || !all.count(r.b)) {
      reason = "not a complete interior edge star";
      return false;
    }
    const Node a = all.at(r.a), b = all.at(r.b), mid{new_point, (a.p + b.p) * .5, 0};
    for (const auto& t : old) {
      Node opposite;
      for (auto v : t.v)
        if (v.id != r.a && v.id != r.b) opposite = v;
      fresh.push_back(triangle(a, mid, opposite));
      fresh.push_back(triangle(mid, b, opposite));
    }
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
    if (min_quality(fresh, metric) <= min_quality(old, metric) + 1e-7 ||
        length(c, d, metric) > std::max(1.8, length(a, b, metric))) {
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
        Request r{kind, a.id, 0, 1 - quality(t, metric)};
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
}  // namespace SU2Native2D
