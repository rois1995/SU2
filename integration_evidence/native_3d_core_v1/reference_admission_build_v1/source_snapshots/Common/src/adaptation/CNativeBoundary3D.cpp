/*!
 * \file CNativeBoundary3D.cpp
 * \brief Exact faceted surface and volume bisection, inverse pairing and private rollback.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <cmath>
#include <type_traits>
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
std::vector<Edge> StraightBorders(const std::vector<Edge>& border, const std::map<Id, Node>& nodes) {
  std::map<Id, Id> next, previous;
  for (auto edge : border)
    if (!next.emplace(edge[0], edge[1]).second || !previous.emplace(edge[1], edge[0]).second) return border;
  std::set<Id> corners;
  for (const auto& edge : next) {
    const auto before = previous.find(edge.first);
    if (before == previous.end()) return border;
    if (!OnEdge(nodes.at(before->second).p, nodes.at(edge.second).p, nodes.at(edge.first).p, nullptr))
      corners.insert(edge.first);
  }
  std::vector<Edge> result;
  size_t walked = 0;
  for (Id first : corners) {
    Id last = first;
    do {
      if (++walked > border.size()) return border;
      last = next.at(last);
    } while (!corners.count(last));
    result.push_back({first, last});
  }
  // Remove only forward-collinear subdivisions: identical oriented half-planes, including separate hole loops.
  return walked == border.size() ? result : border;
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
    f.facet = reference.Canonical(f.facet);
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
PlanarReference::PlanarReference(std::vector<Facet> input, size_t ceiling) {
  const size_t bytes = ConstructionBytes(input.size());
  if (bytes == SIZE_MAX || bytes > ceiling) throw std::invalid_argument("Native 3D reference construction exceeds storage budget.");
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
  std::map<Edge, std::vector<Id>> owners;
  for (const auto& row : facets) {
    canonical.emplace(row.first, row.first);
    for (size_t i = 0; i < 3; ++i) {
      Edge edge{row.second.v[i].id, row.second.v[(i + 1) % 3].id};
      std::sort(edge.begin(), edge.end());
      owners[edge].push_back(row.first);
    }
  }
  std::set<Id> seen;
  for (const auto& root : facets) {
    if (seen.count(root.first)) continue;
    const auto& reference = root.second;
    size_t axis = 0;
    while (Projected(reference.v[0].p, reference.v[1].p, reference.v[2].p, axis, nullptr) == 0) ++axis;
    const auto sign = Projected(reference.v[0].p, reference.v[1].p, reference.v[2].p, axis, nullptr);
    std::vector<Id> component{root.first};
    seen.insert(root.first);
    for (size_t j = 0; j < component.size(); ++j) {
      const auto& f = Get(component[j]);
      for (size_t i = 0; i < 3; ++i) {
        Edge edge{f.v[i].id, f.v[(i + 1) % 3].id};
        std::sort(edge.begin(), edge.end());
        const auto& adjacent = owners.at(edge);
        if (adjacent.size() != 2) continue;
        for (Id id : adjacent) {
          if (seen.count(id)) continue;
          const auto& next = Get(id);
          if (next.marker != reference.marker) continue;
          bool coplanar = true;
          for (auto n : next.v)
            coplanar = coplanar && Orientation(reference.v[0].p, reference.v[1].p, reference.v[2].p, n.p) == 0;
          const auto area = Projected(next.v[0].p, next.v[1].p, next.v[2].p, axis, nullptr);
          if (coplanar && ((sign > 0 && area > 0) || (sign < 0 && area < 0))) {
            component.push_back(id);
            seen.insert(id);
          }
        }
      }
    }
    if (component.size() == 1) continue;
    std::vector<Face> triangles;
    std::map<Edge, std::vector<Edge>> edges;
    std::set<Id> vertices;
    for (Id id : component) {
      const auto& f = Get(id);
      triangles.push_back({f.v[0].id, f.v[1].id, f.v[2].id});
      for (size_t i = 0; i < 3; ++i) {
        vertices.insert(f.v[i].id);
        const Edge oriented{f.v[i].id, f.v[(i + 1) % 3].id};
        auto key = oriented;
        std::sort(key.begin(), key.end());
        edges[key].push_back(oriented);
      }
    }
    bool valid = true;
    try {
      ValidateSurfaceTopology(triangles, false);
    } catch (const std::invalid_argument&) {
      valid = false;
    }
    std::set<std::array<double, 3>> coordinates;
    for (Id id : vertices) {
      const auto p = nodes.at(id).p;
      valid = coordinates.insert({p.x, p.y, p.z}).second && valid;
    }
    std::vector<Edge> border;
    for (const auto& entry : edges) {
      const auto& uses = entry.second;
      if (uses.size() == 1)
        border.push_back(uses[0]);
      else if (uses.size() != 2 || uses[0][0] != uses[1][1] || uses[0][1] != uses[1][0])
        valid = false;
    }
    if (valid) border = StraightBorders(border, nodes);
    for (auto edge : border)
      for (Id id : vertices) {
        const auto side = Projected(nodes.at(edge[0]).p, nodes.at(edge[1]).p, nodes.at(id).p, axis, nullptr);
        if ((sign > 0 && side < 0) || (sign < 0 && side > 0)) valid = false;
      }
    // shortcut: retain raw constraints on nonconvex patches until finite-union triangle/path coverage is available.
    if (!valid) continue;
    boundaries.emplace(root.first, std::move(border));
    for (Id id : component) canonical.at(id) = root.first;
  }
  std::map<Id, std::array<Point, 2>> boxes;
  for (const auto& row : facets) {
    const auto id = Canonical(row.first);
    auto& box = boxes.emplace(id, std::array<Point, 2>{row.second.v[0].p, row.second.v[0].p}).first->second;
    for (auto n : row.second.v) {
      box[0] = {std::min(box[0].x, n.p.x), std::min(box[0].y, n.p.y), std::min(box[0].z, n.p.z)};
      box[1] = {std::max(box[1].x, n.p.x), std::max(box[1].y, n.p.y), std::max(box[1].z, n.p.z)};
    }
  }
  bool multiple = false;
  for (const auto& row : boxes) {
    auto& ids = marker_facets[Get(row.first).marker];
    ids.push_back(row.first);
    multiple = multiple || ids.size() > 1;
    indexed.push_back(row.first);
  }
  if constexpr (std::is_same<su2double, double>::value) {
    if (multiple) {
      std::vector<su2double> coordinates;
      std::vector<unsigned long> connectivity, ids;
      std::vector<unsigned short> types(indexed.size(), LINE), markers(indexed.size(), 0);
      coordinates.reserve(6 * indexed.size());
      connectivity.reserve(2 * indexed.size());
      ids.reserve(indexed.size());
      for (Id id : indexed) {
        for (auto p : boxes.at(id)) {
          coordinates.push_back(p.x);coordinates.push_back(p.y);coordinates.push_back(p.z);
        }
        connectivity.push_back(connectivity.size());connectivity.push_back(connectivity.size());
        ids.push_back(ids.size());
      }
      search = std::make_shared<CADTElemClass>(3, coordinates, connectivity, types, markers, ids, false);
    }
  }
}
size_t PlanarReference::ConstructionBytes(size_t count) {
  if (count > std::numeric_limits<unsigned long>::max() / 2) return SIZE_MAX;
  size_t retained = 0, peak = 0;
  if (count) CADTElemClass::PredictBytes(3, 2 * count, count, 2 * count, &retained, &peak);
  // shortcut: conservative map/component scratch; calibrate allocator requests before larger reference campaigns.
  return transfer_memory::Add(transfer_memory::Mul(count, 4096), retained, peak,
                             transfer_memory::Mul(count, 6 * sizeof(su2double) + 3 * sizeof(unsigned long) + 4 * sizeof(unsigned short)));
}
size_t PlanarReference::ResidentBytes() const {
  auto map = [](const auto& m) {
    return transfer_memory::Mul(m.size(), sizeof(typename std::decay_t<decltype(m)>::value_type) + 4 * sizeof(void*));
  };
  size_t result = transfer_memory::Add(sizeof(*this), map(facets), map(canonical), map(boundaries), map(nodes));
  for (const auto& entry : boundaries) result = transfer_memory::Add(result, transfer_memory::Bytes(entry.second));
  result = transfer_memory::Add(result, map(marker_facets), transfer_memory::Bytes(indexed),
                                search ? search->GetAllocatedBytes() : 0);
  for (const auto& entry : marker_facets) result = transfer_memory::Add(result, transfer_memory::Bytes(entry.second));
  return result;
}
size_t PlanarReference::SearchWorkspaceBytes() const {
  return search ? transfer_memory::Add(transfer_memory::Mul(indexed.size(), 2 * sizeof(unsigned long)),
                                       search->RetainedWorkspaceBound(1)) : 0;
}
bool PlanarReference::FindFace(int marker, const std::array<Point, 3>& triangle, Id& facet, KernelStats* stats) const {
  for (auto p : triangle)
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
      throw std::invalid_argument("Nonfinite native 3D physical binding query.");
  const auto group = marker_facets.find(marker);
  if (group == marker_facets.end()) return false;
  if (group->second.size() == 1 || !search) {
    for (Id id : group->second)
      if (ContainsFace(id, triangle, stats)) {facet = id;return true;}
    return false;
  }
  Point lo = triangle[0], hi = triangle[0];
  for (auto p : triangle) {
    lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
    hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
  }
  const std::array<su2double, 3> lower{lo.x, lo.y, lo.z}, upper{hi.x, hi.y, hi.z};
  std::vector<unsigned long> candidates;
  search->DetermineIntersectingElements(lower.data(), upper.data(), candidates);
  bool found = false;
  Id chosen = 0;
  for (auto index : candidates) {
    const auto id = indexed.at(index);
    if (Get(id).marker == marker && ContainsFace(id, triangle, stats) && (!found || id < chosen)) {
      chosen = id;found = true;
    }
  }
  if (found) facet = chosen;
  return found;
}
Id PlanarReference::Canonical(Id id) const { return canonical.at(id); }
const Facet& PlanarReference::Get(Id id) const { return facets.at(id); }
bool PlanarReference::Contains(Id id, Point p, KernelStats* stats) const {
  id = Canonical(id);
  const auto& v = Get(id).v;
  if (Orientation(v[0].p, v[1].p, v[2].p, p, stats) != 0) return false;
  for (size_t axis = 0; axis < 3; ++axis) {
    const auto area = Projected(v[0].p, v[1].p, v[2].p, axis, stats);
    if (area == 0) continue;
    const auto group = boundaries.find(id);
    if (group == boundaries.end()) {
      for (size_t i = 0; i < 3; ++i) {
        const auto side = Projected(v[i].p, v[(i + 1) % 3].p, p, axis, stats);
        if ((area > 0 && side < 0) || (area < 0 && side > 0)) return false;
      }
    } else
      for (auto edge : group->second) {
        const auto side = Projected(nodes.at(edge[0]).p, nodes.at(edge[1]).p, p, axis, stats);
        if ((area > 0 && side < 0) || (area < 0 && side > 0)) return false;
      }
    return true;
  }
  throw std::logic_error("Degenerate admitted native 3D reference facet.");
}
bool PlanarReference::ContainsFace(Id id, const std::array<Point, 3>& triangle, KernelStats* stats) const {
  for (auto p : triangle)
    if (!Contains(id, p, stats)) return false;
  const auto& f = Get(Canonical(id));
  for (size_t axis = 0; axis < 3; ++axis) {
    const auto old = Projected(f.v[0].p, f.v[1].p, f.v[2].p, axis, stats);
    if (old != 0) {
      const auto fresh = Projected(triangle[0], triangle[1], triangle[2], axis, stats);
      return (old > 0 && fresh > 0) || (old < 0 && fresh < 0);
    }
  }
  return false;
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
bool InsertEdgeCavity(const Patch& old, Edge edge, Node point, Id first, const PlanarReference& reference,
                      Patch& fresh, std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    Endpoints(edge);
    const Incidence index(old.cells, stats);
    const auto before = SkinMap(index.Boundary());
    auto skin = before;
    auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    if (nodes.count(point.id)) throw std::invalid_argument("Native 3D cavity insertion reuses a vertex identity.");
    for (const auto& cell : old.cells) {
      unsigned endpoints = 0;
      for (auto node : cell.v) endpoints += node.id == edge[0] || node.id == edge[1];
      if (endpoints != 2) throw std::invalid_argument("Native 3D insertion input is not a complete edge star.");
    }
    std::vector<Face> wall;
    Id facet = 0;
    for (const auto& row : before) if (Has(row.second, edge[0]) && Has(row.second, edge[1])) {
      const auto found = physical.find(row.first);
      if (found == physical.end()) throw std::invalid_argument("Native 3D incomplete insertion dependency at interface face.");
      if (!wall.empty() && facet != found->second.facet)
        throw std::invalid_argument("Native 3D insertion cannot change an immutable facet or marker junction.");
      facet = found->second.facet;
      wall.push_back(row.second);
    }
    Patch proposal;
    if (wall.empty()) {
      if (!Cone(old.cells, point, first, proposal.cells, reason, stats)) throw std::invalid_argument(reason);
      for (const auto& row : physical) proposal.surface.push_back(row.second);
    } else {
      if (wall.size() != 2 || !reference.Contains(facet, point.p, stats))
        throw std::invalid_argument("Native 3D surface insertion needs two coplanar same-facet triangles.");
      for (auto face : wall) {
        physical.erase(Key(face));
        skin.erase(Key(face));
        for (size_t i = 0; i < 3; ++i) {
          const Id a = face[i], b = face[(i + 1) % 3];
          if ((a == edge[0] && b == edge[1]) || (a == edge[1] && b == edge[0])) continue;
          auto child = Oriented(Face{a, b, point.id});
          if (!reference.ContainsFace(facet, {nodes.at(a).p, nodes.at(b).p, point.p}, stats))
            throw std::invalid_argument("Native 3D surface insertion leaves or reverses its planar fan.");
          skin.emplace(Key(child), child);
          physical.emplace(Key(child), SurfaceFace{child, facet});
        }
      }
      // Positive planar fan children retain the old oriented outer border; exact coplanarity closes volume.
      for (const auto& row : before) {
        if (std::find(wall.begin(), wall.end(), row.second) != wall.end()) continue;
        const auto f = row.second;
        const std::array<Node, 4> v{point, nodes.at(f[0]), nodes.at(f[1]), nodes.at(f[2])};
        if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p, stats) > 0))
          throw std::invalid_argument("Native 3D insertion cannot see its retained cavity skin.");
        Append(proposal.cells, v, first, stats);
      }
      Finish(old, proposal, skin, physical, reference, stats);
    }
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
bool CollapseVertex(const Patch& old, Id point, Id neighbor, Id first, const PlanarReference& reference, Patch& fresh,
                    std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    if (point == neighbor) throw std::invalid_argument("Native 3D collapse repeats its endpoint.");
    const Incidence index(old.cells, stats);
    auto skin = SkinMap(index.Boundary());
    auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    const auto apex = nodes.at(neighbor);
    nodes.at(point);
    for (const auto& f : skin)
      if (Has(f.second, point)) {
        const auto marked = physical.find(f.first);
        if (marked == physical.end())
          throw std::invalid_argument("Native 3D incomplete collapse dependency at interface node " +
                                      std::to_string(point) + ".");
        if (!reference.Contains(marked->second.facet, apex.p, stats))
          throw std::invalid_argument("Native 3D collapse leaves incident reference facet " +
                                      std::to_string(marked->second.facet) + ".");
      }
    Patch proposal;
    size_t removed = 0;
    for (const auto& cell : old.cells) {
      auto v = cell.v;
      bool has_point = false, has_neighbor = false;
      for (auto& n : v) {
        has_neighbor = has_neighbor || n.id == neighbor;
        if (n.id == point) {
          has_point = true;
          n = apex;
        }
      }
      if (!has_point) throw std::invalid_argument("Native 3D collapse input is not a complete vertex star.");
      if (has_neighbor) {
        ++removed;
        continue;
      }
      if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p, stats) > 0))
        throw std::invalid_argument("Native 3D collapse obstructed at cell " + std::to_string(cell.id) + ".");
      Append(proposal.cells, v, first, stats);
    }
    if (!removed || proposal.cells.empty())
      throw std::invalid_argument("Native 3D collapse removes no cells or the entire volume patch.");
    const auto before = skin;
    for (const auto& entry : before)
      if (Has(entry.second, point)) {
        auto found = physical.find(entry.first);
        const auto facet = found->second.facet;
        physical.erase(found);
        skin.erase(entry.first);
        if (Has(entry.second, neighbor)) continue;
        auto face = entry.second;
        for (auto& id : face)
          if (id == point) id = neighbor;
        // All intermediate face areas are affine in the moved point; positive endpoint orientation prevents folding.
        for (size_t axis = 0; axis < 3; ++axis) {
          const auto old_area = Projected(nodes.at(entry.second[0]).p, nodes.at(entry.second[1]).p,
                                          nodes.at(entry.second[2]).p, axis, stats);
          if (old_area == 0) continue;
          const auto area = Projected(nodes.at(face[0]).p, nodes.at(face[1]).p, nodes.at(face[2]).p, axis, stats);
          if ((old_area > 0 && !(area > 0)) || (old_area < 0 && !(area < 0)))
            throw std::invalid_argument("Native 3D collapse folds physical facet " + std::to_string(facet) + ".");
          break;
        }
        face = Oriented(face);
        if (!skin.emplace(Key(face), face).second || !physical.emplace(Key(face), SurfaceFace{face, facet}).second)
          throw std::invalid_argument("Native 3D collapse repeats a physical face.");
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
bool MoveVertex(const Patch& old, Id vertex, Node point, Id first, const PlanarReference& reference, Patch& fresh,
                std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    const Incidence index(old.cells, stats);
    const auto before = SkinMap(index.Boundary());
    const auto physical = CheckSurface(old, before, reference, stats);
    const auto nodes = Nodes(old.cells);
    const auto original = nodes.at(vertex);
    if (nodes.count(point.id) || (original.p.x == point.p.x && original.p.y == point.p.y && original.p.z == point.p.z))
      throw std::invalid_argument("Native 3D movement needs a new identity and a distinct position.");
    for (const auto& face : before)
      if (Has(face.second, vertex)) {
        const auto marked = physical.find(face.first);
        if (marked == physical.end())
          throw std::invalid_argument("Native 3D incomplete movement dependency at interface node " +
                                      std::to_string(vertex) + ".");
        if (!reference.Contains(marked->second.facet, point.p, stats))
          throw std::invalid_argument("Native 3D movement leaves incident reference facet " +
                                      std::to_string(marked->second.facet) + ".");
      }
    Patch proposal;
    for (const auto& cell : old.cells) {
      auto v = cell.v;
      bool found = false;
      for (auto& node : v)
        if (node.id == vertex) {
          found = true;
          node = point;
        }
      if (!found) throw std::invalid_argument("Native 3D movement input is not a complete vertex star.");
      if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p, stats) > 0))
        throw std::invalid_argument("Native 3D movement obstructed at cell " + std::to_string(cell.id) + ".");
      Append(proposal.cells, v, first, stats);
    }
    auto moved_nodes = nodes;
    moved_nodes.emplace(point.id, point);
    Skin skin;
    Physical surface;
    for (const auto& entry : before) {
      auto face = entry.second;
      const bool moving = Has(face, vertex);
      if (moving)
        for (auto& id : face)
          if (id == vertex) id = point.id;
      if (moving)
        for (size_t axis = 0; axis < 3; ++axis) {
          const auto old_area = Projected(nodes.at(entry.second[0]).p, nodes.at(entry.second[1]).p,
                                          nodes.at(entry.second[2]).p, axis, stats);
          if (old_area == 0) continue;
          const auto area =
              Projected(moved_nodes.at(face[0]).p, moved_nodes.at(face[1]).p, moved_nodes.at(face[2]).p, axis, stats);
          if ((old_area > 0 && !(area > 0)) || (old_area < 0 && !(area < 0)))
            throw std::invalid_argument("Native 3D movement folds physical face at node " + std::to_string(vertex) +
                                        ".");
          break;
        }
      face = Oriented(face);
      if (!skin.emplace(Key(face), face).second) throw std::invalid_argument("Native 3D movement repeats a skin face.");
      const auto marked = physical.find(entry.first);
      if (marked != physical.end()) surface.emplace(Key(face), SurfaceFace{face, marked->second.facet});
    }
    // Positive affine determinants/face areas along the facet-contained displacement preserve the old domain.
    Finish(old, proposal, skin, surface, reference, stats);
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool Reconnect(const Patch& old, Id vertex, Id first, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    const auto skin = SkinMap(Incidence(old.cells, stats).Boundary());
    const auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    Patch proposal;
    if (!Cone(old.cells, nodes.at(vertex), first, proposal.cells, reason, stats)) return false;
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
