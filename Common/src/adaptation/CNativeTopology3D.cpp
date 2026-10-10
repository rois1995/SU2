/*!
 * \file CNativeTopology3D.cpp
 * \brief Sorted face, edge and vertex incidence with exact orientation admission.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeTopology3D.hpp"
#include <algorithm>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <utility>

namespace SU2Native3D {
namespace {
template <class Use>
void SortUses(std::vector<Use>& uses) {
  std::sort(uses.begin(), uses.end(),
            [](const Use& a, const Use& b) { return std::make_pair(a.key, a.cell) < std::make_pair(b.key, b.cell); });
}
template <class Use, class Key>
std::vector<Id> FindCells(const std::vector<Use>& uses, const Key& key) {
  auto first =
      std::lower_bound(uses.begin(), uses.end(), key, [](const Use& use, const Key& k) { return use.key < k; });
  std::vector<Id> cells;
  while (first != uses.end() && first->key == key) cells.push_back((first++)->cell);
  return cells;
}
template <size_t N>
std::array<Id, N> Canonical(std::array<Id, N> key) {
  std::sort(key.begin(), key.end());
  if (std::adjacent_find(key.begin(), key.end()) != key.end())
    throw std::invalid_argument("Native 3D incidence key repeats a vertex.");
  return key;
}
int Parity(const Face& face) {
  int inversions = 0;
  for (size_t i = 0; i < face.size(); ++i)
    for (size_t j = i + 1; j < face.size(); ++j) inversions += face[i] > face[j];
  return inversions % 2 ? -1 : 1;
}
}  // namespace

Incidence::Incidence(const std::vector<Cell>& cells, KernelStats* stats) : components(cells.size()) {
  if (cells.size() > std::numeric_limits<size_t>::max() / 6)
    throw std::length_error("Native 3D incidence size overflow.");
  faces.reserve(4 * cells.size());
  edges.reserve(6 * cells.size());
  vertices.reserve(4 * cells.size());
  std::vector<Node> nodes;
  nodes.reserve(4 * cells.size());
  std::vector<Id> ids;
  ids.reserve(cells.size());
  std::vector<std::array<Id, 4>> simplices;
  simplices.reserve(cells.size());
  for (const auto& cell : cells) {
    ids.push_back(cell.id);
    std::array<Id, 4> key;
    for (size_t i = 0; i < 4; ++i) key[i] = cell.v[i].id;
    simplices.push_back(Canonical(key));
    if (!(Orientation(cell.v[0].p, cell.v[1].p, cell.v[2].p, cell.v[3].p, stats) > 0))
      throw std::invalid_argument("Native 3D incidence requires positive tetrahedra.");
    for (const auto& node : cell.v) {
      nodes.push_back(node);
      vertices.push_back({node.id, cell.id});
    }
    for (size_t i = 0; i < 4; ++i)
      for (size_t j = i + 1; j < 4; ++j) edges.push_back({Canonical(Edge{key[i], key[j]}), cell.id});
    const std::array<Face, 4> outward{
        {{key[1], key[2], key[3]}, {key[0], key[3], key[2]}, {key[0], key[1], key[3]}, {key[0], key[2], key[1]}}};
    for (const auto& face : outward) faces.push_back({Canonical(face), cell.id, Parity(face)});
  }
  std::sort(ids.begin(), ids.end());
  if (std::adjacent_find(ids.begin(), ids.end()) != ids.end())
    throw std::invalid_argument("Native 3D incidence repeats a cell ID.");
  std::sort(simplices.begin(), simplices.end());
  if (std::adjacent_find(simplices.begin(), simplices.end()) != simplices.end())
    throw std::invalid_argument("Native 3D incidence repeats a tetrahedron.");
  std::sort(nodes.begin(), nodes.end(), [](const Node& a, const Node& b) { return a.id < b.id; });
  for (size_t i = 1; i < nodes.size(); ++i) {
    const auto& a = nodes[i - 1];
    const auto& b = nodes[i];
    if (a.id == b.id && (a.p.x != b.p.x || a.p.y != b.p.y || a.p.z != b.p.z))
      throw std::invalid_argument("Native 3D incidence has inconsistent vertex coordinates.");
  }
  SortUses(faces);
  SortUses(edges);
  SortUses(vertices);
  std::vector<size_t> parent(cells.size()), weight(cells.size(), 1);
  std::iota(parent.begin(), parent.end(), 0);
  auto root = [&parent](size_t i) {
    while (parent[i] != i) {
      parent[i] = parent[parent[i]];
      i = parent[i];
    }
    return i;
  };
  for (size_t first = 0; first < faces.size();) {
    size_t last = first + 1;
    while (last < faces.size() && faces[last].key == faces[first].key) ++last;
    if (last - first > 2) throw std::invalid_argument("Native 3D incidence has a nonmanifold face.");
    if (last - first == 2) {
      if (faces[first].orientation == faces[first + 1].orientation)
        throw std::invalid_argument("Native 3D internal face orientations do not cancel.");
      size_t a = root(std::lower_bound(ids.begin(), ids.end(), faces[first].cell) - ids.begin());
      size_t b = root(std::lower_bound(ids.begin(), ids.end(), faces[first + 1].cell) - ids.begin());
      if (a != b) {
        if (weight[a] < weight[b]) std::swap(a, b);
        parent[b] = a;
        weight[a] += weight[b];
        --components;
      }
    }
    first = last;
  }
}
std::vector<Id> Incidence::FaceCells(Face face) const { return FindCells(faces, Canonical(face)); }
std::vector<Id> Incidence::EdgeCells(Edge edge) const { return FindCells(edges, Canonical(edge)); }
std::vector<Id> Incidence::VertexCells(Id vertex) const { return FindCells(vertices, vertex); }
std::vector<Face> Incidence::Boundary() const {
  std::vector<Face> boundary;
  for (size_t first = 0; first < faces.size();) {
    size_t last = first + 1;
    while (last < faces.size() && faces[last].key == faces[first].key) ++last;
    if (last == first + 1) {
      auto face = faces[first].key;
      if (faces[first].orientation < 0) std::swap(face[1], face[2]);
      boundary.push_back(face);
    }
    first = last;
  }
  return boundary;
}
}  // namespace SU2Native3D
