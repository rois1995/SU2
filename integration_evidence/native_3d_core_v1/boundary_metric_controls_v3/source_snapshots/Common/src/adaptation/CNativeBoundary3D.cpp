/*!
 * \file CNativeBoundary3D.cpp
 * \brief Exact faceted surface and volume bisection, inverse pairing and private rollback.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
namespace SU2Native3D {
namespace {
bool Has(Face f, Id id) { return std::find(f.begin(), f.end(), id) != f.end(); }
Face Key(Face f) {
  std::sort(f.begin(), f.end());
  return f;
}
Face Oriented(Face f) {
  std::rotate(f.begin(), std::min_element(f.begin(), f.end()), f.end());
  return f;
}
long double Projected(Point a, Point b, Point c, size_t axis, KernelStats* stats) {
  auto project = [axis](Point p) {
    return axis == 0 ? Point{p.y, p.z, 0} : axis == 1 ? Point{p.x, p.z, 0} : Point{p.x, p.y, 0};
  };
  return Orientation(project(a), project(b), project(c), {0, 0, 1}, stats);
}
bool OnEdge(Point a, Point b, Point p, KernelStats* stats) {
  for (size_t axis = 0; axis < 3; ++axis)
    if (Projected(a, b, p, axis, stats) != 0) return false;
  return p.x >= std::min(a.x, b.x) && p.x <= std::max(a.x, b.x) && p.y >= std::min(a.y, b.y) &&
         p.y <= std::max(a.y, b.y) && p.z >= std::min(a.z, b.z) && p.z <= std::max(a.z, b.z);
}
std::map<Id, Node> Nodes(const std::vector<Cell>& cells) {
  std::map<Id, Node> result;
  for (const auto& c : cells)
    for (auto n : c.v) result.emplace(n.id, n);
  return result;
}
using Skin = std::map<Face, Face>;
Skin SkinMap(const std::vector<Face>& faces) {
  Skin skin;
  for (auto f : faces) skin.emplace(Key(f), Oriented(f));
  return skin;
}
std::vector<Face> SkinVector(const Skin& skin) {
  std::vector<Face> result;
  result.reserve(skin.size());
  for (const auto& f : skin) result.push_back(f.second);
  return result;
}
using Physical = std::map<Face, SurfaceFace>;
Physical CheckSurface(const Patch& patch, const Skin& skin, const PlanarReference& reference, KernelStats* stats) {
  if (patch.surface.size() > 4 * MaximumReplacementCells)
    throw std::invalid_argument("Native 3D physical skin exceeds budget.");
  const auto nodes = Nodes(patch.cells);
  Physical faces;
  for (auto f : patch.surface) {
    f.v = Oriented(f.v);
    const auto key = Key(f.v);
    const auto found = skin.find(key);
    if (found == skin.end() || found->second != f.v || !faces.emplace(key, f).second)
      throw std::invalid_argument("Native 3D physical face is duplicate or not the oriented patch skin.");
    for (auto id : f.v)
      if (!reference.Contains(f.facet, nodes.at(id).p, stats))
        throw std::invalid_argument("Native 3D physical face leaves original facet " + std::to_string(f.facet) +
                                    " at node " + std::to_string(id) + ".");
  }
  return faces;
}
void Bounds(const Patch& patch) {
  if (patch.cells.empty() || patch.cells.size() > MaximumCavityCells)
    throw std::invalid_argument("Native 3D edge patch is empty or exceeds cell budget.");
}
void Append(std::vector<Cell>& cells, std::array<Node, 4> nodes, Id first, KernelStats* stats) {
  if (cells.size() >= MaximumReplacementCells || first > std::numeric_limits<Id>::max() - cells.size())
    throw std::invalid_argument("Native 3D edge proposal exceeds cell/ID budget.");
  const auto sign = Orientation(nodes[0].p, nodes[1].p, nodes[2].p, nodes[3].p, stats);
  if (sign == 0) throw std::invalid_argument("Native 3D edge proposal has a degenerate tetrahedron.");
  if (sign < 0) std::swap(nodes[0], nodes[1]);
  cells.push_back({first + cells.size(), nodes});
}
void Finish(const Patch& old, Patch& proposal, const Skin& expected, const Physical& physical,
            const PlanarReference& reference, KernelStats* stats) {
  for (const auto& f : physical) proposal.surface.push_back(f.second);
  std::string reason;
  if (!ValidatePrescribedInterface(old.cells, proposal.cells, SkinVector(expected), reason, stats))
    throw std::invalid_argument(reason);
  CheckSurface(proposal, expected, reference, stats);
}
void Endpoints(Edge& edge) {
  if (edge[0] == edge[1]) throw std::invalid_argument("Native 3D edge repeats an endpoint.");
  std::sort(edge.begin(), edge.end());
}
}  // namespace
PlanarReference::PlanarReference(std::vector<Facet> input) {
  if (input.size() > 4 * MaximumCavityCells) throw std::invalid_argument("Native 3D reference facet budget exceeded.");
  std::map<Id, Node> nodes;
  for (const auto& f : input) {
    std::set<Id> ids;
    for (auto n : f.v) {
      ids.insert(n.id);
      const auto old = nodes.emplace(n.id, n);
      if (!old.second &&
          (old.first->second.p.x != n.p.x || old.first->second.p.y != n.p.y || old.first->second.p.z != n.p.z))
        throw std::invalid_argument("Inconsistent native 3D reference node coordinates.");
    }
    bool area = false;
    for (size_t axis = 0; axis < 3; ++axis) area = Projected(f.v[0].p, f.v[1].p, f.v[2].p, axis, nullptr) != 0 || area;
    if (ids.size() != 3 || !area || !facets.emplace(f.id, f).second)
      throw std::invalid_argument("Native 3D reference has a duplicate or degenerate facet.");
  }
}
const Facet& PlanarReference::Get(Id id) const { return facets.at(id); }
bool PlanarReference::Contains(Id id, Point p, KernelStats* stats) const {
  const auto& v = Get(id).v;
  if (Orientation(v[0].p, v[1].p, v[2].p, p, stats) != 0) return false;
  for (size_t axis = 0; axis < 3; ++axis) {
    const auto area = Projected(v[0].p, v[1].p, v[2].p, axis, stats);
    if (area == 0) continue;
    for (size_t i = 0; i < 3; ++i) {
      const auto side = Projected(v[i].p, v[(i + 1) % 3].p, p, axis, stats);
      if ((area > 0 && side < 0) || (area < 0 && side > 0)) return false;
    }
    return true;
  }
  throw std::logic_error("Degenerate admitted native 3D reference facet.");
}
bool SplitEdge(const Patch& old, Edge edge, Node point, Id first, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    Endpoints(edge);
    const Incidence index(old.cells, stats);
    auto skin = SkinMap(index.Boundary());
    auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    const auto a = nodes.at(edge[0]), b = nodes.at(edge[1]);
    if (nodes.count(point.id) || !OnEdge(a.p, b.p, point.p, stats) ||
        (point.p.x == a.p.x && point.p.y == a.p.y && point.p.z == a.p.z) ||
        (point.p.x == b.p.x && point.p.y == b.p.y && point.p.z == b.p.z))
      throw std::invalid_argument("Native 3D split point is not a distinct exact on-edge point.");
    Patch proposal;
    for (const auto& c : old.cells) {
      auto left = c.v, right = c.v;
      bool has_a = false, has_b = false;
      for (size_t i = 0; i < 4; ++i) {
        if (c.v[i].id == edge[0]) {
          right[i] = point;
          has_a = true;
        }
        if (c.v[i].id == edge[1]) {
          left[i] = point;
          has_b = true;
        }
      }
      if (!(has_a && has_b)) throw std::invalid_argument("Native 3D split input is not an edge star.");
      Append(proposal.cells, left, first, stats);
      Append(proposal.cells, right, first, stats);
    }
    const auto before = skin;
    for (const auto& entry : before)
      if (Has(entry.second, edge[0]) && Has(entry.second, edge[1])) {
        const auto found = physical.find(entry.first);
        if (found == physical.end())
          throw std::invalid_argument("Native 3D incomplete split dependency at interface face " +
                                      std::to_string(entry.first[0]) + "," + std::to_string(entry.first[1]) + "," +
                                      std::to_string(entry.first[2]) + ".");
        const auto facet = found->second.facet;
        physical.erase(found);
        skin.erase(entry.first);
        const auto f = entry.second;
        for (size_t i = 0; i < 3; ++i)
          if ((f[i] == edge[0] && f[(i + 1) % 3] == edge[1]) || (f[i] == edge[1] && f[(i + 1) % 3] == edge[0])) {
            for (auto child : {Face{f[i], point.id, f[(i + 2) % 3]}, Face{point.id, f[(i + 1) % 3], f[(i + 2) % 3]}}) {
              child = Oriented(child);
              skin.emplace(Key(child), child);
              physical.emplace(Key(child), SurfaceFace{child, facet});
            }
            break;
          }
      }
    Finish(old, proposal, skin, physical, reference, stats);
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool UndoEdgeSplit(const Patch& old, Id point, Edge edge, Id first, const PlanarReference& reference, Patch& fresh,
                   std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    Endpoints(edge);
    if (point == edge[0] || point == edge[1])
      throw std::invalid_argument("Native 3D inverse split repeats an endpoint.");
    const Incidence index(old.cells, stats);
    auto skin = SkinMap(index.Boundary());
    auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    const auto a = nodes.at(edge[0]), b = nodes.at(edge[1]), p = nodes.at(point);
    if (!OnEdge(a.p, b.p, p.p, stats))
      throw std::invalid_argument("Native 3D inverse split point is off its original edge.");
    std::map<Edge, std::array<bool, 2>> pairs;
    for (const auto& c : old.cells) {
      bool has_p = false, has_a = false, has_b = false;
      Edge others{};
      size_t count = 0;
      for (auto n : c.v) {
        if (n.id == point)
          has_p = true;
        else if (n.id == edge[0])
          has_a = true;
        else if (n.id == edge[1])
          has_b = true;
        else {
          if (count >= 2) throw std::invalid_argument("Native 3D inverse split is not a paired vertex star.");
          others[count++] = n.id;
        }
      }
      if (!has_p || has_a == has_b || count != 2)
        throw std::invalid_argument("Native 3D inverse split is not a paired vertex star.");
      std::sort(others.begin(), others.end());
      auto& pair = pairs[others];
      if (pair[has_b]) throw std::invalid_argument("Native 3D inverse split repeats a child.");
      pair[has_b] = true;
    }
    Patch proposal;
    for (const auto& pair : pairs) {
      if (!(pair.second[0] && pair.second[1]))
        throw std::invalid_argument("Native 3D inverse split has a missing volume sibling.");
      Append(proposal.cells, {a, b, nodes.at(pair.first[0]), nodes.at(pair.first[1])}, first, stats);
    }
    struct SurfacePair {
      Id facet;
      Face face;
      bool side;
    };
    std::map<std::pair<Id, Id>, std::vector<SurfacePair>> surface_pairs;
    const auto before = skin;
    for (const auto& entry : before)
      if (Has(entry.second, point)) {
        const auto found = physical.find(entry.first);
        if (found == physical.end())
          throw std::invalid_argument("Native 3D incomplete inverse split dependency at interface node " +
                                      std::to_string(point) + ".");
        const auto f = entry.second;
        const bool has_a = Has(f, edge[0]), has_b = Has(f, edge[1]);
        if (has_a == has_b) throw std::invalid_argument("Native 3D inverse split has an unpaired physical face.");
        Id other = 0;
        for (auto id : f)
          if (id != point && id != edge[has_b]) other = id;
        const auto facet = found->second.facet;
        surface_pairs[{facet, other}].push_back({facet, f, has_b});
        physical.erase(found);
        skin.erase(entry.first);
      }
    for (const auto& entry : surface_pairs) {
      const auto& pair = entry.second;
      if (pair.size() != 2 || pair[0].side == pair[1].side)
        throw std::invalid_argument("Native 3D inverse split has a missing physical sibling.");
      auto parent = pair[0].face;
      for (auto& id : parent)
        if (id == point) id = edge[!pair[0].side];
      parent = Oriented(parent);
      const auto key = Key(parent);
      if (!skin.emplace(key, parent).second || !physical.emplace(key, SurfaceFace{parent, pair[0].facet}).second)
        throw std::invalid_argument("Native 3D inverse split repeats a parent face.");
    }
    Finish(old, proposal, skin, physical, reference, stats);
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
}  // namespace SU2Native3D
