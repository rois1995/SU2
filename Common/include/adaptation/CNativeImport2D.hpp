/*!
 * \file CNativeImport2D.hpp
 * \brief Physical-face/volume matching and reference association independent of repartitioned point IDs.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once

#include "CNativeField2D.hpp"
#include "CNativeReference2D.hpp"
#include "CNativeGeometryValidation2D.hpp"
#include "CReaderSlices.hpp"

namespace SU2NativeBoundary2D {

using FaceCoordinates = std::array<double, 4>;
inline FaceCoordinates CoordinateKey(Point a, Point b) {
  if (std::tie(b.x, b.y) < std::tie(a.x, a.y)) std::swap(a, b);
  return {a.x, a.y, b.x, b.y};
}
struct ReferenceState {
  std::shared_ptr<const PolylineReference> original;
  std::vector<std::string> marker_names;
  std::map<FaceCoordinates, int> accepted_edges;
};
struct BoundaryLabel {
  Id a = 0, b = 0;
  int marker = 0;  // Position in canonical marker-name list, independent of local/config marker order.
  template <class S>
  void Fields(S& s) {
    s(a, b, marker);
  }
};
struct VolumeEdge {
  Node a, b;
  Tensor ma{}, mb{};
  Id cell = 0;
  int owner = 0, slot = 0;
  template <class S>
  void Fields(S& s) {
    s(a, b, ma, mb, cell, owner, slot);
  }
};
struct MarkerReply {
  Id cell = 0;
  int slot = 0, marker = 0;
  template <class S>
  void Fields(S& s) {
    s(cell, slot, marker);
  }
};

/*--- Bootstrap exchanges the distributed volume edges only with their hash owners. Physical reference storage
 *    is replicated and separately accounted O(B); no rank receives a complete volume mesh. ---*/
inline void Associate(World& world, std::map<Id, Cell>& owned, const std::vector<BoundaryLabel>& labels,
                      const std::vector<std::string>& names, double featureAngle, ReferenceState& state) {
  Directory hash(world);
  auto bucket = [&](Id a, Id b) {
    const auto key = edge(a, b);
    return hash.rendezvous(key.first ^ (key.second + 0x9e3779b97f4a7c15ULL + (key.first << 6) + (key.first >> 2)));
  };
  std::vector<std::vector<VolumeEdge>> edgesTo(world.size);
  std::vector<std::vector<BoundaryLabel>> labelsTo(world.size);
  CLocalFailure failure;
  for (const auto& entry : owned) {
    const auto& cell = entry.second;
    try {
      if (!(area(cell.t) > 0) || entry.first != cell.t.id) failure.Set(1, entry.first, "Invalid native import cell.");
      for (int k = 0; k < 3; ++k) {
        const int j = (k + 1) % 3;
        edgesTo[bucket(cell.t.v[k].id, cell.t.v[j].id)].push_back(
            {cell.t.v[k], cell.t.v[j], cell.nodal_target[k], cell.nodal_target[j], cell.t.id, world.rank, k});
      }
    } catch (const std::exception& error) {
      failure.Set(1, entry.first, error.what());
    }
  }
  for (const auto& label : labels) {
    if (label.a == label.b || label.marker < 0 || size_t(label.marker) >= names.size())
      failure.Set(1, label.a, "Invalid native physical label.");
    else
      labelsTo[bucket(label.a, label.b)].push_back(label);
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const auto edges = world.exchange(edgesTo);
  const auto physicalLabels = world.exchange(labelsTo);
  std::map<Edge, std::vector<VolumeEdge>> adjacency;
  std::map<Edge, int> physicalMarker;
  for (const auto& edgeRecord : edges) adjacency[edge(edgeRecord.a.id, edgeRecord.b.id)].push_back(edgeRecord);
  for (const auto& label : physicalLabels)
    if (!physicalMarker.emplace(edge(label.a, label.b), label.marker).second)
      failure.Set(1, label.a, "Duplicate native physical-face ownership.");
  std::vector<PolylineReference::Face> localFaces;
  std::vector<std::vector<MarkerReply>> replyTo(world.size);
  auto same = [](Node a, Node b, Tensor ma, Tensor mb) {
    return a.id == b.id && a.p.x == b.p.x && a.p.y == b.p.y && ma.xx == mb.xx && ma.xy == mb.xy && ma.yy == mb.yy;
  };
  for (const auto& entry : adjacency) {
    const auto& rows = entry.second;
    const auto physical = physicalMarker.find(entry.first);
    if (rows.size() == 1 && physical != physicalMarker.end()) {
      const auto& row = rows.front();
      localFaces.push_back({row.a, row.b, physical->second});
      replyTo[row.owner].push_back({row.cell, row.slot, physical->second});
    } else if (rows.size() == 2 && physical == physicalMarker.end()) {
      const auto& a = rows[0];
      const auto& b = rows[1];
      if (a.cell == b.cell || !same(a.a, b.b, a.ma, b.mb) || !same(a.b, b.a, a.mb, b.ma))
        failure.Set(1, entry.first.first, "Nonmatching native shared edge/point target.");
    } else
      failure.Set(1, entry.first.first, "Native volume/physical-face coverage is nonmanifold or incomplete.");
  }
  for (const auto& entry : physicalMarker)
    if (!adjacency.count(entry.first))
      failure.Set(1, entry.first.first, "Native physical face has no incident volume cell.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const auto faces = world.metadata(localFaces);
  const auto replies = world.exchange(replyTo);
  if (!state.original) {
    try {
      ValidatePhysicalGraph(faces);
      auto original = std::make_shared<PolylineReference>(faces, featureAngle);
      state.original = std::move(original);
      state.marker_names = names;
      for (const auto& face : faces)
        state.accepted_edges.emplace(CoordinateKey(face.a.p, face.b.p),
                                     state.original->ComponentOfOriginalFace(face.a.id, face.b.id));
    } catch (const std::exception& error) {
      failure.Set(1, 0, error.what());
    }
  }
  if (state.marker_names != names) failure.Set(1, 0, "Native reference marker names changed.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  std::map<Edge, int> components;
  std::set<Id> boundaryNodes;
  for (const auto& face : faces) {
    const auto association = state.accepted_edges.find(CoordinateKey(face.a.p, face.b.p));
    if (association == state.accepted_edges.end()) {
      failure.Set(1, face.a.id, "Physical face is missing from the accepted immutable native reference bindings.");
      continue;
    }
    const auto component = association->second;
    if (state.original->Marker(component) != face.marker)
      failure.Set(1, face.a.id, "Native physical marker/component association changed.");
    components.emplace(edge(face.a.id, face.b.id), component);
    boundaryNodes.insert(face.a.id);
    boundaryNodes.insert(face.b.id);
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  for (auto& entry : owned) {
    auto& cell = entry.second;
    cell.marker.fill(0);
    for (auto& node : cell.t.v) {
      node.fixed = boundaryNodes.count(node.id) ? 1 : 0;
      if (node.fixed && state.original->IsFeature(node.p)) node.fixed |= FEATURE;
    }
  }
  for (const auto& reply : replies) {
    auto& cell = owned.at(reply.cell);
    cell.marker[reply.slot] = components.at(edge(cell.t.v[reply.slot].id, cell.t.v[(reply.slot + 1) % 3].id));
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
}

inline std::map<FaceCoordinates, int> Bindings(World& world, const std::map<Id, Cell>& owned) {
  std::vector<PolylineReference::Face> local;
  for (const auto& cell : owned)
    for (int k = 0; k < 3; ++k)
      if (cell.second.marker[k])
        local.push_back({cell.second.t.v[k], cell.second.t.v[(k + 1) % 3], cell.second.marker[k]});
  const auto faces = world.metadata(local);
  std::map<FaceCoordinates, int> result;
  CLocalFailure failure;
  for (const auto& face : faces)
    if (!result.emplace(CoordinateKey(face.a.p, face.b.p), face.marker).second)
      failure.Set(1, face.a.id, "Duplicate physical coordinates in native reference bindings.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  return result;
}

inline CReaderSlices ReaderSlices(const std::map<Id, Cell>& owned, const ReferenceState& state) {
  CSimplexMesh mesh;
  mesh.nDim = 2;
  for (size_t k = 0; k < state.marker_names.size(); ++k) {
    CSimplexMesh::Marker marker;
    marker.name = state.marker_names[k];
    marker.ref = k + 1;
    mesh.markers.push_back(std::move(marker));
  }
  std::map<Id, std::pair<Node, Tensor>> points;
  CLocalFailure failure;
  for (const auto& entry : owned)
    for (int k = 0; k < 3; ++k) {
      const auto node = entry.second.t.v[k];
      const auto target = entry.second.nodal_target[k];
      const auto found = points.emplace(node.id, std::make_pair(node, target));
      if (!found.second) {
        const auto& other = found.first->second;
        if (other.first.p.x != node.p.x || other.first.p.y != node.p.y || other.second.xx != target.xx ||
            other.second.xy != target.xy || other.second.yy != target.yy)
          failure.Set(1, node.id, "Inconsistent native output point/target cache.");
      }
    }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  std::map<Id, unsigned long> index;
  std::vector<uint64_t> keys;
  keys.reserve(points.size());
  for (const auto& entry : points) {
    index.emplace(entry.first, keys.size());
    keys.push_back(entry.first);
    mesh.coord.push_back(entry.second.first.p.x);
    mesh.coord.push_back(entry.second.first.p.y);
    mesh.metric.push_back(entry.second.second.xx);
    mesh.metric.push_back(entry.second.second.xy);
    mesh.metric.push_back(entry.second.second.yy);
  }
  for (const auto& entry : owned) {
    const auto& cell = entry.second;
    for (const auto& node : cell.t.v) mesh.elem.push_back(index.at(node.id));
    mesh.elemRef.push_back(1);
    for (int k = 0; k < 3; ++k)
      if (cell.marker[k]) {
        auto& marker = mesh.markers.at(state.original->Marker(cell.marker[k]));
        marker.elem.push_back(index.at(cell.t.v[k].id));
        marker.elem.push_back(index.at(cell.t.v[(k + 1) % 3].id));
      }
  }
  return CReaderSlices::FromDistributed(mesh, keys);
}

}  // namespace SU2NativeBoundary2D
