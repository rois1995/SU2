/*!
 * \file CNativeCavity3D.cpp
 * \brief Oriented cavity closure, manifold links and bounded point reconnection.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeCavity3D.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace SU2Native3D {
namespace {
using Graph = std::map<Id, std::vector<Id>>;
std::map<Id, Node> Nodes(const std::vector<Cell>& cells) {
  std::map<Id, Node> nodes;
  std::map<std::array<double, 3>, Id> positions;
  for (const auto& cell : cells)
    for (const auto& v : cell.v) {
      const auto where = positions.emplace(std::array<double, 3>{v.p.x, v.p.y, v.p.z}, v.id);
      if (!where.second && where.first->second != v.id)
        throw std::invalid_argument("Native 3D cavity has coincident distinct vertices.");
      nodes.emplace(v.id, v);
    }
  return nodes;
}
bool Connected(const Graph& graph) {
  if (graph.empty()) return false;
  std::set<Id> seen;
  std::vector<Id> pending{graph.begin()->first};
  seen.insert(pending.front());
  for (size_t i = 0; i < pending.size(); ++i)
    for (Id neighbor : graph.at(pending[i]))
      if (seen.insert(neighbor).second) pending.push_back(neighbor);
  return seen.size() == graph.size();
}
void Add(Graph& graph, Id a, Id b) {
  graph[a].push_back(b);
  graph[b].push_back(a);
}
void Curve(const Graph& graph, bool closed) {
  size_t ends = 0;
  for (const auto& entry : graph) {
    if (entry.second.size() == 1)
      ++ends;
    else if (entry.second.size() != 2)
      throw std::invalid_argument("Native 3D cavity has a branched link.");
  }
  if (ends != (closed ? 0u : 2u) || !Connected(graph))
    throw std::invalid_argument("Native 3D cavity has a disconnected or unclosed link.");
}
/*! Connected triangular sphere/disk with manifold vertex links, not just an Euler-characteristic test. */
void Surface(const std::vector<Face>& triangles, bool closed) {
  if (triangles.empty()) throw std::invalid_argument("Native 3D cavity has an empty link.");
  std::map<Edge, std::vector<Id>> edges;
  std::map<Id, Graph> links;
  Graph adjacency, border;
  for (size_t i = 0; i < triangles.size(); ++i) {
    adjacency[i];
    const auto& t = triangles[i];
    for (size_t j = 0; j < 3; ++j) {
      const Id a = t[j], b = t[(j + 1) % 3], c = t[(j + 2) % 3];
      edges[Edge{std::min(a, b), std::max(a, b)}].push_back(i);
      Add(links[a], b, c);
    }
  }
  for (const auto& entry : edges) {
    const auto& owners = entry.second;
    if (owners.size() == 2)
      Add(adjacency, owners[0], owners[1]);
    else if (owners.size() == 1)
      Add(border, entry.first[0], entry.first[1]);
    else
      throw std::invalid_argument("Native 3D cavity link has a nonmanifold edge.");
  }
  if (!Connected(adjacency) || static_cast<int64_t>(links.size()) - static_cast<int64_t>(edges.size()) +
                                       static_cast<int64_t>(triangles.size()) !=
                                   (closed ? 2 : 1))
    throw std::invalid_argument("Native 3D cavity link is not a connected sphere/disk.");
  if (closed) {
    if (!border.empty()) throw std::invalid_argument("Native 3D cavity sphere has a boundary.");
  } else
    Curve(border, true);
  for (const auto& entry : links) Curve(entry.second, !border.count(entry.first));
}
void Ball(const std::vector<Cell>& cells, const Incidence& index, const std::vector<Face>& boundary) {
  if (index.FaceComponents() != 1) throw std::invalid_argument("Native 3D cavity is not face connected.");
  Surface(boundary, true);
  std::set<Id> boundary_nodes;
  for (const auto& face : boundary) boundary_nodes.insert(face.begin(), face.end());
  std::map<Id, std::vector<Face>> links;
  for (const auto& cell : cells)
    for (size_t i = 0; i < 4; ++i) {
      Face opposite;
      size_t j = 0;
      for (size_t k = 0; k < 4; ++k)
        if (k != i) opposite[j++] = cell.v[k].id;
      links[cell.v[i].id].push_back(opposite);
    }
  for (const auto& entry : links) Surface(entry.second, !boundary_nodes.count(entry.first));
}
void Bounds(const std::vector<Cell>& cells, size_t maximum) {
  if (cells.empty() || cells.size() > maximum)
    throw std::invalid_argument("Native 3D cavity cell budget/empty-input rejection.");
}
void Validate(const std::vector<Cell>& old, const Incidence& before, const std::vector<Cell>& fresh,
              KernelStats* stats) {
  Bounds(fresh, MaximumReplacementCells);
  const Incidence after(fresh, stats);
  const auto old_skin = before.Boundary(), new_skin = after.Boundary();
  if (old_skin != new_skin) throw std::invalid_argument("Native 3D cavity oriented interface changed.");
  const auto a = Nodes(old), b = Nodes(fresh);
  for (const auto& face : old_skin)
    for (Id id : face) {
      const auto p = a.at(id).p, q = b.at(id).p;
      if (p.x != q.x || p.y != q.y || p.z != q.z)
        throw std::invalid_argument("Native 3D cavity interface coordinates changed.");
    }
  Ball(old, before, old_skin);
  Ball(fresh, after, new_skin);
  // Exact oriented-face cancellation and unchanged coordinates close signed volume algebraically.
  // A sum of rounded determinants can falsely reject rotated thin cavities.
}
}  // namespace

bool ValidateFixedInterface(const std::vector<Cell>& old, const std::vector<Cell>& fresh, std::string& reason,
                            KernelStats* stats) {
  try {
    Bounds(old, MaximumCavityCells);
    const Incidence before(old, stats);
    Validate(old, before, fresh, stats);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool Cone(const std::vector<Cell>& old, Node apex, Id first_cell, std::vector<Cell>& fresh, std::string& reason,
          KernelStats* stats) {
  try {
    Bounds(old, MaximumCavityCells);
    const Incidence before(old, stats);
    const auto nodes = Nodes(old);
    std::vector<Cell> proposal;
    const auto skin = before.Boundary();
    for (const auto& face : skin) {
      if (std::find(face.begin(), face.end(), apex.id) != face.end()) continue;
      if (proposal.size() >= MaximumReplacementCells || first_cell > std::numeric_limits<Id>::max() - proposal.size())
        throw std::invalid_argument("Native 3D cone exceeds cell/ID budget.");
      Cell cell{first_cell + proposal.size(), {apex, nodes.at(face[0]), nodes.at(face[1]), nodes.at(face[2])}};
      if (!(Orientation(cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p, stats) > 0))
        throw std::invalid_argument("Native 3D cone obstructed by interface face " + std::to_string(face[0]) + "," +
                                    std::to_string(face[1]) + "," + std::to_string(face[2]) + " at apex " +
                                    std::to_string(apex.id) + ".");
      proposal.push_back(cell);
    }
    Validate(old, before, proposal, stats);
    fresh.swap(proposal);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
}  // namespace SU2Native3D
