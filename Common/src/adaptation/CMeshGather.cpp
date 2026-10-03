/*!
 * \file CMeshGather.cpp
 * \brief Gather of the partitioned mesh and of point values on one rank in the global numbering (mesh adaptation with
 *        MPI), the scatter back, the broadcast of a mesh and the exchange of halo values.
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

#include "../../include/adaptation/CMeshGather.hpp"

#include <algorithm>
#include <array>
#include <climits>

#include "../../include/CConfig.hpp"
#include "../../include/adaptation/CMMGInterface.hpp"
#include "../../include/geometry/CGeometry.hpp"

namespace {

/*--- Send buffers of all ranks to the root (counts in entries of the datatype), received in rank order. ---*/
template <class T>
std::vector<T> ToRoot(const std::vector<T>& send, MPI_Datatype type, int root, std::vector<int>* counts = nullptr) {
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  if (send.size() > static_cast<size_t>(INT_MAX)) SU2_MPI::Error("Message too large for MPI.", CURRENT_FUNCTION);
  const int nSend = static_cast<int>(send.size());
  std::vector<int> nRecv(size, 0);
  SU2_MPI::Gather(&nSend, 1, MPI_INT, nRecv.data(), 1, MPI_INT, root, SU2_MPI::GetComm());

  std::vector<int> sendCounts(size, 0), sendDispl(size, 0), recvCounts(size, 0), recvDispl(size + 1, 0);
  sendCounts[root] = nSend;
  if (rank == root) {
    for (int iRank = 0; iRank < size; ++iRank) {
      recvCounts[iRank] = nRecv[iRank];
      if (static_cast<long>(recvDispl[iRank]) + nRecv[iRank] > INT_MAX)
        SU2_MPI::Error("Message too large for MPI.", CURRENT_FUNCTION);
      recvDispl[iRank + 1] = recvDispl[iRank] + nRecv[iRank];
    }
  }
  std::vector<T> recv(rank == root ? recvDispl[size] : 0);
  /*--- Non-empty buffers for the pointers. ---*/
  T dummySend{}, dummyRecv{};
  SU2_MPI::Alltoallv(send.empty() ? &dummySend : send.data(), sendCounts.data(), sendDispl.data(), type,
                     recv.empty() ? &dummyRecv : recv.data(), recvCounts.data(), recvDispl.data(), type,
                     SU2_MPI::GetComm());
  if (counts != nullptr) *counts = nRecv;
  return recv;
}

/*--- Broadcast a vector from the root (its size too). ---*/
template <class T>
void BcastVector(std::vector<T>& values, MPI_Datatype type, int root) {
  unsigned long n = values.size();
  SU2_MPI::Bcast(&n, 1, MPI_UNSIGNED_LONG, root, SU2_MPI::GetComm());
  values.resize(n);
  if (n > static_cast<unsigned long>(INT_MAX)) SU2_MPI::Error("Message too large for MPI.", CURRENT_FUNCTION);
  if (n > 0) SU2_MPI::Bcast(values.data(), static_cast<int>(n), type, root, SU2_MPI::GetComm());
}

/*--- Passive doubles: MPI_DOUBLE is the active type of the MeDiPack wrapper in the AD builds, so they travel as
 *    su2double there. ---*/
void BcastPassive(std::vector<passivedouble>& values, int root) {
#if defined CODI_REVERSE_TYPE || defined CODI_FORWARD_TYPE
  std::vector<su2double> active(values.begin(), values.end());
  BcastVector(active, MPI_DOUBLE, root);
  values.resize(active.size());
  for (auto i = 0ul; i < active.size(); ++i) values[i] = SU2_TYPE::GetValue(active[i]);
#else
  BcastVector(values, MPI_DOUBLE, root);
#endif
}

using Key = std::array<unsigned long, 3>;

/*--- Sorted nodes of a boundary element (unused entries ULONG_MAX). ---*/
Key SortedKey(const unsigned long* nodes, unsigned short nNode) {
  Key key = {ULONG_MAX, ULONG_MAX, ULONG_MAX};
  for (unsigned short i = 0; i < nNode; ++i) key[i] = nodes[i];
  std::sort(key.begin(), key.begin() + nNode);
  return key;
}

}  // namespace

CMeshGather::CMeshGather(const CGeometry& geometry, int root)
    : geometry(geometry), root(root), nPointDomain(geometry.GetnPointDomain()),
      nPointGlobal(geometry.GetGlobal_nPointDomain()) {
  /*--- SU2 stores the domain points first. ---*/
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
    if (geometry.nodes->GetDomain(iPoint) != (iPoint < nPointDomain)) {
      SU2_MPI::Error("The domain points of the geometry are not stored first.", CURRENT_FUNCTION);
    }
  }
  std::vector<unsigned long> ids(nPointDomain);
  for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) ids[iPoint] = geometry.nodes->GetGlobalIndex(iPoint);
  globalId = ToRoot(ids, MPI_UNSIGNED_LONG, root, &rankPoints);

  if (!IsRoot()) return;
  const int size = SU2_MPI::GetSize();
  rankOffset.assign(size + 1, 0);
  for (int iRank = 0; iRank < size; ++iRank) rankOffset[iRank + 1] = rankOffset[iRank] + rankPoints[iRank];

  /*--- The global indices of the domain points must be 0 .. N-1, each once. ---*/
  std::vector<bool> seen(nPointGlobal, false);
  bool valid = (globalId.size() == nPointGlobal);
  for (auto i = 0ul; valid && i < globalId.size(); ++i) {
    valid = globalId[i] < nPointGlobal && !seen[globalId[i]];
    if (valid) seen[globalId[i]] = true;
  }
  if (!valid) {
    SU2_MPI::Error("The global indices of the mesh points are not a numbering 0 .. N-1 of the domain points.",
                   CURRENT_FUNCTION);
  }
}

std::vector<su2double> CMeshGather::GatherRankOrder(const su2double* local, unsigned short n) const {
  std::vector<su2double> send(local, local + nPointDomain * n);
  return ToRoot(send, MPI_DOUBLE, root);
}

std::vector<su2double> CMeshGather::Gather(const su2double* local, unsigned short n) const {
  const auto received = GatherRankOrder(local, n);
  std::vector<su2double> global;
  if (!IsRoot()) return global;
  global.resize(nPointGlobal * n);
  for (auto i = 0ul; i < globalId.size(); ++i)
    for (unsigned short k = 0; k < n; ++k) global[globalId[i] * n + k] = received[i * n + k];
  return global;
}

void CMeshGather::Scatter(const std::vector<su2double>& global, unsigned short n, su2double* local) const {
  const int size = SU2_MPI::GetSize();
  std::vector<int> sendCounts(size, 0), sendDispl(size, 0), recvCounts(size, 0), recvDispl(size, 0);
  std::vector<su2double> send;
  if (IsRoot()) {
    if (global.size() != nPointGlobal * n) SU2_MPI::Error("Wrong size of the values to scatter.", CURRENT_FUNCTION);
    send.resize(globalId.size() * n);
    for (auto i = 0ul; i < globalId.size(); ++i)
      for (unsigned short k = 0; k < n; ++k) send[i * n + k] = global[globalId[i] * n + k];
    for (int iRank = 0; iRank < size; ++iRank) {
      if (static_cast<long>(rankOffset[iRank + 1]) * n > INT_MAX)
        SU2_MPI::Error("Message too large for MPI.", CURRENT_FUNCTION);
      sendCounts[iRank] = rankPoints[iRank] * n;
      sendDispl[iRank] = rankOffset[iRank] * n;
    }
  }
  recvCounts[root] = static_cast<int>(nPointDomain * n);
  std::vector<su2double> recv(nPointDomain * n);
  su2double dummySend = 0.0, dummyRecv = 0.0;
  SU2_MPI::Alltoallv(send.empty() ? &dummySend : send.data(), sendCounts.data(), sendDispl.data(), MPI_DOUBLE,
                     recv.empty() ? &dummyRecv : recv.data(), recvCounts.data(), recvDispl.data(), MPI_DOUBLE,
                     SU2_MPI::GetComm());
  std::copy(recv.begin(), recv.end(), local);
}

std::vector<std::string> CMeshGather::GatherMarkerNames(const CConfig& config,
                                                        const std::vector<std::string>& markerTags,
                                                        const CGeometry& geometry) {
  /*--- Names of this rank, separated by newlines, gathered on all ranks. ---*/
  if (markerTags.size() < geometry.GetnMarker()) {
    SU2_MPI::Error("A marker of the geometry has no name.", CURRENT_FUNCTION);
  }
  std::string localNames;
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    if (markerTags[iMarker] == "SEND_RECEIVE" || geometry.GetnElem_Bound(iMarker) == 0) continue;
    if (geometry.bound[iMarker][0]->GetVTK_Type() == VERTEX) continue;
    localNames += markerTags[iMarker] + "\n";
  }
  const int size = SU2_MPI::GetSize();
  const int nLocal = static_cast<int>(localNames.size());
  std::vector<int> counts(size, 0), displ(size + 1, 0);
  SU2_MPI::Allgather(&nLocal, 1, MPI_INT, counts.data(), 1, MPI_INT, SU2_MPI::GetComm());
  for (int iRank = 0; iRank < size; ++iRank) displ[iRank + 1] = displ[iRank] + counts[iRank];
  std::vector<char> all(std::max(displ[size], 1));
  std::vector<char> send(localNames.begin(), localNames.end());
  send.push_back('\0');
  SU2_MPI::Allgatherv(send.data(), nLocal, MPI_CHAR, all.data(), counts.data(), displ.data(), MPI_CHAR,
                      SU2_MPI::GetComm());

  std::vector<std::string> names;
  std::string name;
  for (int i = 0; i < displ[size]; ++i) {
    if (all[i] == '\n') {
      if (std::find(names.begin(), names.end(), name) == names.end()) names.push_back(name);
      name.clear();
    } else {
      name += all[i];
    }
  }
  /*--- In the order of the config file; the unnamed markers (empty name) last. ---*/
  auto position = [&config](const std::string& name) -> int {
    return name.empty() ? INT_MAX : config.GetMarker_CfgFile_TagBound(name);
  };
  std::stable_sort(names.begin(), names.end(),
                   [&position](const std::string& a, const std::string& b) { return position(a) < position(b); });
  return names;
}

CSimplexMesh CMeshGather::GatherMesh(const CConfig& config, const std::vector<std::string>& markerTags,
                                     bool controlVolumes) const {
  const auto nDim = geometry.GetnDim();
  if (nDim != 2 && nDim != 3) SU2_MPI::Error("Mesh adaptation needs a 2D or 3D mesh.", CURRENT_FUNCTION);
  const unsigned short nNode = nDim + 1;
  const unsigned short simplex = (nDim == 2) ? TRIANGLE : TETRAHEDRON;
  const unsigned short boundType = (nDim == 2) ? LINE : TRIANGLE;

  CSimplexMesh mesh;
  mesh.nDim = nDim;

  /*--- Markers (the same list on all ranks). ---*/
  const auto names = GatherMarkerNames(config, markerTags, geometry);
  for (const auto& name : names) {
    CSimplexMesh::Marker marker;
    marker.name = name;
    marker.ref = name.empty() ? 0 : CMMGInterface::GetMarkerReference(config, name);
    mesh.markers.push_back(std::move(marker));
  }

  /*--- Points and control volumes. ---*/
  {
    std::vector<su2double> local(nPointDomain * nDim);
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim)
        local[iPoint * nDim + iDim] = geometry.nodes->GetCoord(iPoint, iDim);
    const auto coord = Gather(local.data(), nDim);
    mesh.coord.resize(coord.size());
    for (auto i = 0ul; i < coord.size(); ++i) mesh.coord[i] = SU2_TYPE::GetValue(coord[i]);
  }
  if (controlVolumes) {
    std::vector<su2double> local(nPointDomain);
    for (auto iPoint = 0ul; iPoint < nPointDomain; ++iPoint) local[iPoint] = geometry.nodes->GetVolume(iPoint);
    const auto volume = Gather(local.data(), 1);
    mesh.volume.resize(volume.size());
    for (auto i = 0ul; i < volume.size(); ++i) mesh.volume[i] = SU2_TYPE::GetValue(volume[i]);
  }

  /*--- An element (volume or boundary) is sent by the rank that owns its node of smallest global index. ---*/
  auto owned = [&](const CPrimalGrid* element) {
    unsigned long smallest = ULONG_MAX, owner = 0;
    for (unsigned short iNode = 0; iNode < element->GetnNodes(); ++iNode) {
      const auto iPoint = element->GetNode(iNode);
      const auto id = geometry.nodes->GetGlobalIndex(iPoint);
      if (id < smallest) {
        smallest = id;
        owner = iPoint;
      }
    }
    return geometry.nodes->GetDomain(owner);
  };

  /*--- Volume elements: global index and global nodes, ordered by their sorted global nodes (not by their global
   *    index: that is the order of the mesh file or of the mesh in memory, and the adapted mesh is written in the order
   *    of the output, which depends on the ranks; with the nodes as the key a run restarted from an exported mesh gives
   *    the remesher the same input as the run that wrote it). ---*/
  {
    std::vector<unsigned long> send;
    for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
      const auto* element = geometry.elem[iElem];
      if (element->GetVTK_Type() != simplex) {
        SU2_MPI::Error(string("Mesh adaptation supports only ") + (nDim == 2 ? "triangles" : "tetrahedra") +
                           " (element " + std::to_string(element->GetGlobalIndex()) + " has VTK type " +
                           std::to_string(element->GetVTK_Type()) + ").",
                       CURRENT_FUNCTION);
      }
      if (!owned(element)) continue;
      send.push_back(element->GetGlobalIndex());
      for (unsigned short iNode = 0; iNode < nNode; ++iNode)
        send.push_back(geometry.nodes->GetGlobalIndex(element->GetNode(iNode)));
    }
    const auto received = ToRoot(send, MPI_UNSIGNED_LONG, root);
    if (IsRoot()) {
      const unsigned long stride = nNode + 1, nElem = received.size() / stride;
      using ElemKey = std::array<unsigned long, 4>;
      std::vector<std::pair<ElemKey, unsigned long>> order(nElem);
      for (auto i = 0ul; i < nElem; ++i) {
        ElemKey key = {0, 0, 0, 0};
        for (unsigned short iNode = 0; iNode < nNode; ++iNode) key[iNode] = received[i * stride + 1 + iNode];
        std::sort(key.begin(), key.begin() + nNode);
        order[i] = {key, i};
      }
      std::sort(order.begin(), order.end());
      for (auto i = 1ul; i < nElem; ++i) {
        if (order[i].first == order[i - 1].first) {
          SU2_MPI::Error("Element " + std::to_string(received[order[i].second * stride]) + " was gathered twice.",
                         CURRENT_FUNCTION);
        }
      }
      if (nElem != geometry.GetGlobal_nElemDomain()) {
        SU2_MPI::Error("Gathered " + std::to_string(nElem) + " elements, the mesh has " +
                           std::to_string(geometry.GetGlobal_nElemDomain()) + ".",
                       CURRENT_FUNCTION);
      }
      mesh.elem.resize(nElem * nNode);
      mesh.elemRef.assign(nElem, 0);
      for (auto i = 0ul; i < nElem; ++i)
        for (unsigned short iNode = 0; iNode < nNode; ++iNode)
          mesh.elem[i * nNode + iNode] = received[order[i].second * stride + 1 + iNode];
    }
  }

  /*--- Boundary elements: position of the marker name in the list, global nodes. ---*/
  {
    std::vector<unsigned long> send;
    for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
      const auto& tag = markerTags[iMarker];
      if (tag == "SEND_RECEIVE") continue;
      const auto it = std::find(names.begin(), names.end(), tag);
      for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
        const auto* element = geometry.bound[iMarker][iElem];
        if (element->GetVTK_Type() == VERTEX) continue;
        if (element->GetVTK_Type() != boundType) {
          SU2_MPI::Error(string("Mesh adaptation supports only ") + (nDim == 2 ? "line" : "triangle") +
                             " boundary elements (marker " + tag + ").",
                         CURRENT_FUNCTION);
        }
        if (it == names.end()) SU2_MPI::Error("Unknown marker " + tag + ".", CURRENT_FUNCTION);
        if (!owned(element)) continue;
        send.push_back(it - names.begin());
        for (unsigned short iNode = 0; iNode < nDim; ++iNode)
          send.push_back(geometry.nodes->GetGlobalIndex(element->GetNode(iNode)));
      }
    }
    const auto received = ToRoot(send, MPI_UNSIGNED_LONG, root);
    if (IsRoot()) {
      const unsigned long stride = nDim + 1, nFace = received.size() / stride;
      std::vector<std::vector<std::pair<Key, unsigned long>>> faces(names.size());
      for (auto i = 0ul; i < nFace; ++i) {
        const auto* entry = &received[i * stride];
        faces[entry[0]].emplace_back(SortedKey(entry + 1, nDim), i);
      }
      for (auto iMarker = 0ul; iMarker < names.size(); ++iMarker) {
        auto& list = faces[iMarker];
        std::sort(list.begin(), list.end());
        auto& elem = mesh.markers[iMarker].elem;
        elem.reserve(list.size() * nDim);
        for (auto i = 0ul; i < list.size(); ++i) {
          if (i > 0 && list[i].first == list[i - 1].first) {
            SU2_MPI::Error("A boundary element of marker " + names[iMarker] + " was gathered twice.",
                           CURRENT_FUNCTION);
          }
          for (unsigned short iNode = 0; iNode < nDim; ++iNode)
            elem.push_back(received[list[i].second * stride + 1 + iNode]);
        }
      }
    }
  }
  return mesh;
}

void CMeshGather::Broadcast(CSimplexMesh& mesh, int root) {
  unsigned short nDim = mesh.nDim;
  SU2_MPI::Bcast(&nDim, 1, MPI_UNSIGNED_SHORT, root, SU2_MPI::GetComm());
  mesh.nDim = nDim;
  BcastPassive(mesh.coord, root);
  BcastPassive(mesh.metric, root);
  BcastPassive(mesh.volume, root);
  BcastVector(mesh.elem, MPI_UNSIGNED_LONG, root);
  BcastVector(mesh.elemRef, MPI_INT, root);

  unsigned long nMarker = mesh.markers.size();
  SU2_MPI::Bcast(&nMarker, 1, MPI_UNSIGNED_LONG, root, SU2_MPI::GetComm());
  mesh.markers.resize(nMarker);
  for (auto& marker : mesh.markers) {
    std::vector<char> name(marker.name.begin(), marker.name.end());
    BcastVector(name, MPI_CHAR, root);
    marker.name.assign(name.begin(), name.end());
    SU2_MPI::Bcast(&marker.ref, 1, MPI_INT, root, SU2_MPI::GetComm());
    BcastVector(marker.elem, MPI_UNSIGNED_LONG, root);
  }
}

void CMeshGather::ExchangeHalo(const CGeometry& geometry, su2double* values, unsigned short n) {
  const int nSend = geometry.nP2PSend, nRecv = geometry.nP2PRecv;
  if (nSend == 0 && nRecv == 0) return;
  constexpr int tag = 5839;

  std::vector<su2double> sendBuffer(geometry.nPoint_P2PSend[nSend] * n);
  std::vector<su2double> recvBuffer(geometry.nPoint_P2PRecv[nRecv] * n);
  std::vector<SU2_MPI::Request> requests(nSend + nRecv);

  for (int iRecv = 0; iRecv < nRecv; ++iRecv) {
    const auto offset = geometry.nPoint_P2PRecv[iRecv];
    const int count = (geometry.nPoint_P2PRecv[iRecv + 1] - offset) * n;
    SU2_MPI::Irecv(&recvBuffer[offset * n], count, MPI_DOUBLE, geometry.Neighbors_P2PRecv[iRecv], tag,
                   SU2_MPI::GetComm(), &requests[iRecv]);
  }
  for (int iSend = 0; iSend < nSend; ++iSend) {
    const auto offset = geometry.nPoint_P2PSend[iSend];
    const auto end = geometry.nPoint_P2PSend[iSend + 1];
    for (auto i = offset; i < end; ++i) {
      const auto iPoint = geometry.Local_Point_P2PSend[i];
      for (unsigned short k = 0; k < n; ++k) sendBuffer[i * n + k] = values[iPoint * n + k];
    }
    SU2_MPI::Isend(&sendBuffer[offset * n], (end - offset) * n, MPI_DOUBLE, geometry.Neighbors_P2PSend[iSend], tag,
                   SU2_MPI::GetComm(), &requests[nRecv + iSend]);
  }
  SU2_MPI::Waitall(nSend + nRecv, requests.data(), MPI_STATUS_IGNORE);

  for (int i = 0; i < geometry.nPoint_P2PRecv[nRecv]; ++i) {
    const auto iPoint = geometry.Local_Point_P2PRecv[i];
    for (unsigned short k = 0; k < n; ++k) values[iPoint * n + k] = recvBuffer[i * n + k];
  }
}

CSimplexMesh CMeshGather::LocalMesh(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                                    bool controlVolumes) {
  const auto nDim = geometry.GetnDim();
  if (nDim != 2 && nDim != 3) SU2_MPI::Error("The mesh must be 2D or 3D.", CURRENT_FUNCTION);
  const unsigned short nNode = nDim + 1;
  const unsigned short simplex = (nDim == 2) ? TRIANGLE : TETRAHEDRON;
  const unsigned short boundType = (nDim == 2) ? LINE : TRIANGLE;

  CSimplexMesh mesh;
  mesh.nDim = nDim;
  const auto nPoint = geometry.GetnPoint();
  mesh.coord.resize(nPoint * nDim);
  for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      mesh.coord[iPoint * nDim + iDim] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
  if (controlVolumes) {
    mesh.volume.resize(nPoint);
    for (auto iPoint = 0ul; iPoint < nPoint; ++iPoint)
      mesh.volume[iPoint] = SU2_TYPE::GetValue(geometry.nodes->GetVolume(iPoint));
  }

  mesh.elem.resize(geometry.GetnElem() * nNode);
  mesh.elemRef.assign(geometry.GetnElem(), 0);
  for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
    const auto* element = geometry.elem[iElem];
    if (element->GetVTK_Type() != simplex) {
      SU2_MPI::Error("The mesh must consist of triangles (2D) or tetrahedra (3D).", CURRENT_FUNCTION);
    }
    for (unsigned short iNode = 0; iNode < nNode; ++iNode) mesh.elem[iElem * nNode + iNode] = element->GetNode(iNode);
  }

  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker() && iMarker < markerTags.size(); ++iMarker) {
    const auto& tag = markerTags[iMarker];
    if (tag == "SEND_RECEIVE") continue;
    auto it = std::find_if(mesh.markers.begin(), mesh.markers.end(),
                           [&tag](const CSimplexMesh::Marker& marker) { return marker.name == tag; });
    if (it == mesh.markers.end()) {
      mesh.markers.emplace_back();
      mesh.markers.back().name = tag;
      it = mesh.markers.end() - 1;
    }
    for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
      const auto* element = geometry.bound[iMarker][iElem];
      if (element->GetVTK_Type() == VERTEX) continue;
      if (element->GetVTK_Type() != boundType) {
        SU2_MPI::Error("The boundary elements must be lines (2D) or triangles (3D).", CURRENT_FUNCTION);
      }
      for (unsigned short iNode = 0; iNode < nDim; ++iNode) it->elem.push_back(element->GetNode(iNode));
    }
  }
  return mesh;
}
