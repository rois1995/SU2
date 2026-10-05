/*!
 * \file CNativeBoundary2D.hpp
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
// Coupled physical-boundary and volume reconstruction. Reuses the native kernel.
#include "CNativeMesh2D.hpp"

namespace SU2NativeBoundary2D {
using namespace SU2Native2D;
constexpr int FEATURE = 32;
constexpr std::size_t PATCH_LIMIT = 128;
struct Cell {
  Triangle t;
  std::array<int, 3> marker{{0, 0, 0}};
  std::uint64_t version = 0;
  std::array<double, 4> target_cache{};  // Actual frozen-target quality and three edge lengths; priority only.
  std::array<Tensor, 3> nodal_target{};
  std::uint64_t target_epoch = 0;
  template <class Stream>
  void Fields(Stream& s) {
    s(t, marker, version, target_cache, nodal_target, target_epoch);
  }
};
struct Physical {
  Node a, b;
  int marker = 0;
};
enum class Action : int { HEIGHT, SPLIT, REMOVE, REDISTRIBUTE, BULK_REMOVE, BULK_SPLIT, BULK_FLIP, BULK_MOVE };
struct Operation {
  Action action = Action::HEIGHT;
  Id a = 0, b = 0;
  double parameter = std::numeric_limits<double>::quiet_NaN();
  int fault = 0;
  template <class S>
  void Fields(S& s) {
    int code = static_cast<int>(action);
    s(code, a, b, parameter, fault);
    action = static_cast<Action>(code);
  }
};

/*! Geometry/height policy supplied by the adapter. Intervals follow each directed reference component,
 * including closed-component seam unwrapping. There are no analytic fixture geometries in this kernel. */
struct Reference {
  std::function<Point(int, double)> point;
  std::function<std::pair<double, double>(int, Point, Point)> interval;
  std::function<double(Physical)> deviation;
  std::function<double(int)> height;
  bool is_wall(int marker) const { return height(marker) > 0; }
};

inline Metric checked(Metric metric) {
  return [=](Point p) {
    const Tensor m = metric(p);
    if (!(NormalizedDeterminant(m.xx, m.xy, m.yy) > 1e-14L))
      throw std::runtime_error("nonfinite or numerically singular target metric");
    return m;
  };
}
inline std::vector<Triangle> triangles(const std::vector<Cell>& cells) {
  std::vector<Triangle> result;
  for (auto c : cells) result.push_back(c.t);
  return result;
}
inline std::map<Edge, Physical> physical(const std::vector<Cell>& cells) {
  std::map<Edge, Physical> result;
  for (auto c : cells)
    for (int i = 0; i < 3; ++i)
      if (c.marker[i]) {
        auto a = c.t.v[i], b = c.t.v[(i + 1) % 3];
        auto key = edge(a.id, b.id);
        if (!result.emplace(key, Physical{a, b, c.marker[i]}).second)
          throw std::runtime_error("duplicate physical face");
      }
  return result;
}
inline long double polygon_area(const std::vector<Node>& p) {
  long double result = 0;
  for (std::size_t i = 1; i + 1 < p.size(); ++i) result += orient(p[0].p, p[i].p, p[i + 1].p) / 2;
  return result;
}
inline bool crosses(Point a, Point b, Point c, Point d) { return SegmentsIntersect(a, b, c, d); }
inline bool simple(const std::vector<Node>& p) {
  if (p.size() < 3) return false;
  std::set<Id> ids;
  std::set<std::pair<double, double>> positions;
  for (auto a : p)
    if (!std::isfinite(a.p.x) || !std::isfinite(a.p.y) || !ids.insert(a.id).second ||
        !positions.emplace(a.p.x, a.p.y).second)
      return false;
  if (!(polygon_area(p) > 0)) return false;
  for (std::size_t i = 0; i < p.size(); ++i)
    for (std::size_t j = i + 1; j < p.size(); ++j) {
      if (j == i + 1 || (i == 0 && j == p.size() - 1)) continue;
      if (crosses(p[i].p, p[(i + 1) % p.size()].p, p[j].p, p[(j + 1) % p.size()].p)) return false;
    }
  return true;
}
inline bool inside(Point x, const std::vector<Node>& p) {
  int winding = 0;
  for (std::size_t i = 0; i < p.size(); ++i) {
    auto a = p[i].p, b = p[(i + 1) % p.size()].p;
    if (OnSegment(a, b, x)) return false;
    if (a.y <= x.y && b.y > x.y && orient(a, b, x) > 0) ++winding;
    if (a.y > x.y && b.y <= x.y && orient(a, b, x) < 0) --winding;
  }
  return winding != 0;
}
inline bool polygon(const std::vector<Cell>& cells, std::vector<Node>& output) {
  auto all = nodes(triangles(cells));
  std::map<Id, Id> successor;
  for (auto f : boundary(triangles(cells)))
    if (!successor.emplace(f.second.first.id, f.second.second.id).second) return false;
  if (successor.empty()) return false;
  Id first = successor.begin()->first, current = first;
  do {
    if (!successor.count(current) || output.size() >= successor.size()) return false;
    output.push_back(all.at(current));
    current = successor.at(current);
  } while (current != first);
  return output.size() == successor.size() && simple(output);
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

/*! Repair wedges between newly carved wall children while their complete stars
 * are still private. Tangential apex motion preserves the requested altitude;
 * the cavity perimeter and frozen donor metric remain unchanged. */
inline void relax_wall_apices(std::vector<Triangle>& cells, const Metric& metric) {
  for (int sweep = 0; sweep < 3; ++sweep) {
    bool improved = false;
    for (size_t child = 0; child < cells.size(); ++child) {
      if (!cells[child].protected_cell) continue;
      const auto wall = cells[child];  // Carving orders each child as base endpoints, apex.
      const auto apex = wall.v[2];
      std::vector<size_t> star;
      for (size_t i = 0; i < cells.size(); ++i)
        for (const auto& v : cells[i].v)
          if (v.id == apex.id) star.push_back(i);
      auto penalty = [&](const std::vector<Triangle>& candidate) {
        double value = 0;
        for (const auto i : star) {
          const auto& t = candidate[i];
          const double deficit = std::max(0., 1 - quality(t, metric) / .22);
          value += deficit * deficit;
          if (t.protected_cell) {
            if (quality(t, metric) < .18) return std::numeric_limits<double>::infinity();
            for (int k = 1; k < 3; ++k) {
              const double excess = std::max(0., length(t.v[k], t.v[(k + 1) % 3], metric) / 1.8 - 1);
              value += excess * excess;
            }
          }
        }
        return value;
      };
      double best = penalty(cells);
      if (best == 0) continue;
      const auto base = wall.v[1].p - wall.v[0].p;
      const double current = dot(apex.p - wall.v[0].p, base) / dot(base, base);
      const Point offset = apex.p - (wall.v[0].p + base * current);
      auto choice = cells;
      for (const double parameter : {.05, .2, .35, .5, .65, .8, .95}) {
        auto trial = cells;
        const Point position = wall.v[0].p + base * parameter + offset;
        for (const auto i : star)
          for (auto& v : trial[i].v)
            if (v.id == apex.id) v.p = position;
        std::string reason;
        // Geometry admission precedes donor queries, including inverted/star-crossing trials.
        if (!strict_cells(trial, reason)) continue;
        const double score = penalty(trial);
        if (score + 1e-12 < best) {
          best = score;
          choice = std::move(trial);
        }
      }
      if (best + 1e-12 < penalty(cells)) {
        cells.swap(choice);
        improved = true;
      }
    }
    if (!improved) break;
  }
}

inline bool reconstruct(Operation op, const std::vector<Cell>& old, const Reference& reference, const Metric& metric,
                        Id next, std::vector<Cell>& fresh, std::string& reason,
                        double geometry_tolerance = std::numeric_limits<double>::infinity()) {
  if (old.empty() || old.size() > PATCH_LIMIT) {
    reason = "dependency cap";
    return false;
  }
  if ((int)op.action >= 4) {
    std::vector<Triangle> output;
    Request request{(Kind)((int)op.action - 4), op.a, op.b, 1};
    if (op.action == Action::BULK_MOVE) {
      auto input = triangles(old);
      auto all = nodes(input);
      if (!all.count(op.a) || all.at(op.a).fixed ||
          std::any_of(input.begin(), input.end(), [](auto t) { return t.protected_cell; })) {
        reason = "fixed node or protected first layer";
        return false;
      }
      if (!MoveStar(input, op.a, metric, output, reason)) return false;
    } else if (!SU2Native2D::reconstruct(request, triangles(old), metric, next, output, reason))
      return false;
    if (!strict_cells(output, reason)) return false;
    auto faces = physical(old);
    for (auto t : output) {
      Cell c;
      c.t = t;
      for (int k = 0; k < 3; ++k) {
        auto e = edge(t.v[k].id, t.v[(k + 1) % 3].id);
        if (faces.count(e)) c.marker[k] = faces.at(e).marker;
      }
      fresh.push_back(c);
    }
    return true;
  }
  std::vector<Node> loop;
  if (!polygon(old, loop)) {
    reason = "cavity is not a simple disk";
    return false;
  }
  auto faces = physical(old);
  auto before = boundary(triangles(old));
  for (auto f : faces)
    if (!before.count(f.first)) {
      reason = "physical face inside old cavity";
      return false;
    }
  auto locate = [&](Id id) {
    for (std::size_t i = 0; i < loop.size(); ++i)
      if (loop[i].id == id) return i;
    return loop.size();
  };
  if (op.action == Action::SPLIT) {
    auto found = faces.find(edge(op.a, op.b));
    if (found == faces.end()) {
      reason = "missing split face";
      return false;
    }
    auto f = found->second;
    auto bounds = reference.interval(f.marker, f.a.p, f.b.p);
    double ua = bounds.first, ub = bounds.second;
    Node p{next++, reference.point(f.marker, (ua + ub) * .5), 1};
    if (op.fault == 1) p.p = {std::numeric_limits<double>::quiet_NaN(), 0};
    std::size_t i = locate(f.a.id);
    if (i == loop.size() || loop[(i + 1) % loop.size()].id != f.b.id) {
      reason = "split orientation";
      return false;
    }
    loop.insert(loop.begin() + i + 1, p);
    faces.erase(found);
    faces[edge(f.a.id, p.id)] = {f.a, p, f.marker};
    faces[edge(p.id, f.b.id)] = {p, f.b, f.marker};
  } else if (op.action == Action::REMOVE || op.action == Action::REDISTRIBUTE) {
    const auto i = locate(op.a);
    if (i == loop.size() || (loop[i].fixed & FEATURE)) {
      reason = "feature or missing node";
      return false;
    }
    Node a = loop[(i + loop.size() - 1) % loop.size()], b = loop[(i + 1) % loop.size()], v = loop[i];
    auto left = faces.find(edge(a.id, v.id)), right = faces.find(edge(v.id, b.id));
    if (left == faces.end() || right == faces.end() || left->second.marker != right->second.marker) {
      reason = "marker junction";
      return false;
    }
    int marker = left->second.marker;
    faces.erase(edge(a.id, v.id));
    faces.erase(edge(v.id, b.id));
    if (op.action == Action::REMOVE) {
      loop.erase(loop.begin() + i);
      faces[edge(a.id, b.id)] = {a, b, marker};
    } else {
      double parameter = op.parameter;
      if (!std::isfinite(parameter)) {
        auto bounds = reference.interval(marker, a.p, b.p);
        double lo = bounds.first, hi = bounds.second;
        Node left{0, reference.point(marker, lo), 1}, right{0, reference.point(marker, hi), 1};
        for (int step = 0; step < 40; ++step) {
          parameter = (lo + hi) * .5;
          Node mid{0, reference.point(marker, parameter), 1};
          if (length(left, mid, metric) > length(mid, right, metric))
            hi = parameter;
          else
            lo = parameter;
        }
      }
      const auto moved = reference.point(marker, parameter);
      if (moved.x == v.p.x && moved.y == v.p.y) {
        reason = "no representable tangential redistribution";
        return false;
      }
      v.p = reference.point(marker, parameter);
      loop[i] = v;
      faces[edge(a.id, v.id)] = {a, v, marker};
      faces[edge(v.id, b.id)] = {v, b, marker};
    }
  }
  if (!simple(loop)) {
    reason = "failed projection or nonsimple boundary";
    return false;
  }
  for (const auto& entry : faces) {
    if (!(reference.deviation(entry.second) <= geometry_tolerance)) {
      reason = "physical face misses declared reference tolerance";
      return false;
    }
  }
  if (op.action == Action::REMOVE || op.action == Action::REDISTRIBUTE) {
    auto original = physical(old);
    for (auto f : faces) {
      bool changed = !original.count(f.first);
      if (!changed) {
        auto g = original.at(f.first);
        changed = f.second.a.p.x != g.a.p.x || f.second.a.p.y != g.a.p.y || f.second.b.p.x != g.b.p.x ||
                  f.second.b.p.y != g.b.p.y;
      }
      if (changed && (1.02 * reference.deviation(f.second) > geometry_tolerance ||
                      length(f.second.a, f.second.b, metric) > 1.45)) {
        reason = "boundary coarsening or movement misses geometry/metric target";
        return false;
      }
    }
  }
  const auto expected_loop = loop;
  std::vector<Triangle> accepted;
  // Carve wall children from the polygon, then triangulate the remaining disk.
  // This releases old anchors rather than retaining old first-layer topology.
  for (auto entry : faces) {
    auto f = entry.second;
    if (!reference.is_wall(f.marker)) continue;
    const double h0 = reference.height(f.marker);
    if (!(std::isfinite(h0) && h0 > 0)) {
      reason = "invalid first height";
      return false;
    }
    std::size_t i = locate(f.a.id);
    if (i == loop.size() || loop[(i + 1) % loop.size()].id != f.b.id) {
      reason = "wall edge no longer exposed";
      return false;
    }
    Point tangent = (f.b.p - f.a.p) * (1 / norm(f.b.p - f.a.p)), normal{-tangent.y, tangent.x};
    double best = -1;
    Node choice;
    std::vector<Node> best_loop;
    for (double beta : {0., -.15, .15, -.3, .3, -.45, .45}) {
      // The complete-star bulk guard protects this anchor through its wall cell.
      // Keep it movable/removable after a later wall rebuild releases that cell.
      Node p{next, (f.a.p + f.b.p) * .5 + normal * h0 + (f.b.p - f.a.p) * beta, 0};
      auto child = triangle(f.a, f.b, p);
      if (!inside(p.p, loop)) continue;
      bool blocked = false;
      for (auto v : loop)
        if (v.id != f.a.id && v.id != f.b.id && orient(f.a.p, f.b.p, v.p) >= 0 && orient(f.b.p, p.p, v.p) >= 0 &&
            orient(p.p, f.a.p, v.p) >= 0)
          blocked = true;
      if (blocked) continue;
      auto remainder = loop;
      remainder.insert(remainder.begin() + i + 1, p);
      if (!simple(remainder)) continue;
      if (quality(child, metric) < .18) continue;
      std::vector<Triangle> test;
      if (!ear_clip(remainder, metric, test)) continue;
      double q = std::min(quality(child, metric), min_quality(test, metric));
      if (q > best) {
        best = q;
        choice = p;
        best_loop = std::move(remainder);
      }
    }
    if (best < 0) {
      reason = "no admitted first-height ear; enlargement or different surface resolution required";
      return false;
    }
    auto child = triangle(f.a, f.b, choice);
    child.protected_cell = 1;
    accepted.push_back(child);
    loop = std::move(best_loop);
    ++next;
  }
  if (!ear_clip(loop, metric, accepted)) {
    reason = "remaining polygon triangulation failed";
    return false;
  }
  relax_wall_apices(accepted, metric);
  if (op.fault == 2 && !accepted.empty()) std::swap(accepted[0].v[1], accepted[0].v[2]);
  if (!strict_cells(accepted, reason)) return false;
  long double total = 0;
  for (auto t : accepted) total += area(t);
  if (std::abs(total - polygon_area(expected_loop)) > 1e-12L * polygon_area(expected_loop)) {
    reason = "area differs from new physical perimeter";
    return false;
  }
  auto after = boundary(accepted);
  if (after.size() != expected_loop.size()) {
    reason = "unexpected exposed edge";
    return false;
  }
  for (std::size_t i = 0; i < expected_loop.size(); ++i) {
    auto a = expected_loop[i], b = expected_loop[(i + 1) % expected_loop.size()];
    auto found = after.find(edge(a.id, b.id));
    if (found == after.end() || found->second.first.id != a.id || found->second.second.id != b.id ||
        found->second.first.p.x != a.p.x || found->second.first.p.y != a.p.y || found->second.second.p.x != b.p.x ||
        found->second.second.p.y != b.p.y) {
      reason = "cavity perimeter mismatch";
      return false;
    }
  }
  for (auto f : before)
    if (!physical(old).count(f.first)) {
      auto found = after.find(f.first);
      if (found == after.end() || found->second.first.id != f.second.first.id ||
          found->second.first.p.x != f.second.first.p.x || found->second.first.p.y != f.second.first.p.y ||
          found->second.second.p.x != f.second.second.p.x || found->second.second.p.y != f.second.second.p.y) {
        reason = "artificial interface changed";
        return false;
      }
    }
  for (auto t : accepted) {
    Cell c;
    c.t = t;
    for (int i = 0; i < 3; ++i) {
      auto key = edge(t.v[i].id, t.v[(i + 1) % 3].id);
      if (faces.count(key)) {
        c.marker[i] = faces.at(key).marker;
        if (reference.is_wall(c.marker[i])) {
          const double height =
              (double)(2 * area(t)) / norm(t.v[(i + 1) % 3].p - t.v[i].p) / reference.height(c.marker[i]);
          if (op.fault == 3 || std::abs(height - 1) > 1e-8) {
            reason = "first-height contract";
            return false;
          }
        }
      }
    }
    fresh.push_back(c);
  }
  return true;
}
}  // namespace SU2NativeBoundary2D
