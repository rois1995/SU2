/*!
 * \file CReaderSlices.cpp
 * \brief The reader slices of a complete mesh, sent from the rank that holds it, and the metric of the new points.
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

#include "../../include/adaptation/CReaderSlices.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>

#include "../../include/geometry/CGeometry.hpp"
#include "../../include/option_structure.hpp"
#include "../../include/parallelization/CPassiveComm.hpp"
#include "../../include/toolboxes/CLinearPartitioner.hpp"

namespace {

constexpr int SLICE_TAG = 5841;

/*--- Bytes of scalars and arrays, appended in order (memcpy, no aliasing casts). ---*/
class Packer {
 public:
  std::vector<char> bytes;

  template <class T>
  void Put(T value) {
    PutArray(&value, 1);
  }
  template <class T>
  void PutArray(const T* values, size_t n) {
    static_assert(std::is_trivially_copyable<T>::value, "Packer packs trivially copyable data only.");
    const size_t start = bytes.size();
    bytes.resize(start + n * sizeof(T));
    if (n > 0) std::memcpy(bytes.data() + start, values, n * sizeof(T));
  }
  void PutString(const std::string& text) {
    Put<uint64_t>(text.size());
    PutArray(text.data(), text.size());
  }
};

class Unpacker {
 public:
  explicit Unpacker(const std::vector<char>& bytes) : bytes(bytes) {}

  template <class T>
  T Get() {
    T value{};
    GetArray(&value, 1);
    return value;
  }
  template <class T>
  void GetArray(T* values, size_t n) {
    if (n * sizeof(T) > bytes.size() - position) SU2_MPI::Error("Truncated reader slice.", CURRENT_FUNCTION);
    if (n > 0) std::memcpy(values, bytes.data() + position, n * sizeof(T));
    position += n * sizeof(T);
  }
  std::string GetString() {
    std::string text(Get<uint64_t>(), '\0');
    GetArray(&text[0], text.size());
    return text;
  }
  bool AtEnd() const { return position == bytes.size(); }

 private:
  const std::vector<char>& bytes;
  size_t position = 0;
};

/*--- The checks of CMemoryMeshReaderFVM, in its order and with its messages, done once on the root. ---*/
void CheckMesh(const CSimplexMesh& mesh) {
  const auto nDim = mesh.nDim;
  if (nDim != 2 && nDim != 3) {
    SU2_MPI::Error("The mesh in memory must be 2D or 3D.", CURRENT_FUNCTION);
  }
  const unsigned long nPoint = mesh.GetnPoint();
  if (nPoint == 0 || mesh.GetnElem() == 0 || mesh.coord.size() != nPoint * nDim ||
      mesh.elem.size() != mesh.GetnElem() * (nDim + 1)) {
    SU2_MPI::Error("The mesh in memory is empty or its arrays have inconsistent sizes.", CURRENT_FUNCTION);
  }
  for (unsigned long iElem = 0; iElem < mesh.GetnElem(); iElem++) {
    for (unsigned short iNode = 0; iNode <= nDim; iNode++) {
      if (mesh.elem[iElem * (nDim + 1) + iNode] >= nPoint) {
        SU2_MPI::Error("Element " + std::to_string(iElem) + " of the mesh in memory has a point out of range.",
                       CURRENT_FUNCTION);
      }
    }
  }
  for (const auto& marker : mesh.markers) {
    if (marker.elem.size() % nDim != 0) {
      SU2_MPI::Error("Marker " + marker.name + " of the mesh in memory has an inconsistent size.", CURRENT_FUNCTION);
    }
    for (const auto iPoint : marker.elem) {
      if (iPoint >= nPoint) {
        SU2_MPI::Error("Marker " + marker.name + " of the mesh in memory has a point out of range.", CURRENT_FUNCTION);
      }
    }
  }
  if (!mesh.metric.empty() && mesh.metric.size() != nPoint * CSimplexMesh::GetnMetric(nDim)) {
    SU2_MPI::Error("The metric of the mesh in memory has an inconsistent size.", CURRENT_FUNCTION);
  }
}

/*--- The rows of rank q, packed: header, markers (boundary rows on the master rank), coordinates, metric, volume
 *    rows. elems: the indices of the elements with a point in the slice of q, in increasing order. ---*/
std::vector<char> PackSlice(const CSimplexMesh& mesh, const CLinearPartitioner& partitioner, int q,
                            const unsigned long* elems, unsigned long nElemLocal) {
  const unsigned short nDim = mesh.nDim, nNode = nDim + 1;
  const unsigned short nMetric = mesh.metric.empty() ? 0 : CSimplexMesh::GetnMetric(nDim);
  const unsigned long firstPoint = partitioner.GetFirstIndexOnRank(q);
  const unsigned long nPointLocal = partitioner.GetSizeOnRank(q);
  const bool master = (q == MASTER_NODE);

  Packer pack;
  pack.Put<uint64_t>(nDim);
  pack.Put<uint64_t>(mesh.GetnPoint());
  pack.Put<uint64_t>(mesh.GetnElem());
  pack.Put<uint64_t>(firstPoint);
  pack.Put<uint64_t>(nPointLocal);
  pack.Put<uint64_t>(nMetric);
  pack.Put<uint64_t>(nElemLocal);
  pack.Put<uint64_t>(mesh.markers.size());

  /*--- Markers: name, whether it has elements, the boundary rows (master rank only). ---*/
  const unsigned short boundType = (nDim == 2) ? LINE : TRIANGLE;
  for (const auto& marker : mesh.markers) {
    pack.PutString(marker.name);
    pack.Put<uint8_t>(marker.elem.empty() ? 0 : 1);
    const unsigned long nRow = master ? marker.GetnElem(nDim) : 0;
    pack.Put<uint64_t>(nRow);
    for (unsigned long iElem = 0; iElem < nRow; iElem++) {
      unsigned long row[SU2_CONN_SIZE] = {0, boundType};
      for (unsigned short iNode = 0; iNode < nDim; iNode++) row[2 + iNode] = marker.elem[iElem * nDim + iNode];
      pack.PutArray(row, SU2_CONN_SIZE);
    }
  }

  /*--- Points of the slice. ---*/
  pack.PutArray(mesh.coord.data() + firstPoint * nDim, nPointLocal * nDim);
  if (nMetric > 0) pack.PutArray(mesh.metric.data() + firstPoint * nMetric, nPointLocal * nMetric);

  /*--- Volume rows [element index, VTK type, nodes, 0 ...]. ---*/
  const unsigned short vtkType = (nDim == 2) ? TRIANGLE : TETRAHEDRON;
  for (unsigned long i = 0; i < nElemLocal; i++) {
    const auto iElem = elems[i];
    unsigned long row[SU2_CONN_SIZE] = {iElem, vtkType};
    for (unsigned short iNode = 0; iNode < nNode; iNode++) row[2 + iNode] = mesh.elem[iElem * nNode + iNode];
    pack.PutArray(row, SU2_CONN_SIZE);
  }
  return std::move(pack.bytes);
}

CReaderSlices UnpackSlice(const std::vector<char>& bytes) {
  Unpacker unpack(bytes);
  CReaderSlices slices;
  slices.nDim = unpack.Get<uint64_t>();
  slices.nPointGlobal = unpack.Get<uint64_t>();
  slices.nElemGlobal = unpack.Get<uint64_t>();
  slices.firstPoint = unpack.Get<uint64_t>();
  slices.nPointLocal = unpack.Get<uint64_t>();
  slices.nMetric = unpack.Get<uint64_t>();
  slices.nElemLocal = unpack.Get<uint64_t>();
  const auto nMarker = unpack.Get<uint64_t>();

  slices.markerNames.resize(nMarker);
  slices.boundaryRows.resize(nMarker);
  for (uint64_t iMarker = 0; iMarker < nMarker; iMarker++) {
    slices.markerNames[iMarker] = unpack.GetString();
    if (unpack.Get<uint8_t>() != 0) slices.markersWithElements.push_back(slices.markerNames[iMarker]);
    auto& rows = slices.boundaryRows[iMarker];
    rows.resize(unpack.Get<uint64_t>() * SU2_CONN_SIZE);
    unpack.GetArray(rows.data(), rows.size());
  }

  const unsigned short nDim = slices.nDim;
  std::vector<passivedouble> coord(slices.nPointLocal * nDim);
  unpack.GetArray(coord.data(), coord.size());
  slices.coord.assign(nDim, std::vector<passivedouble>(slices.nPointLocal));
  for (unsigned long iPoint = 0; iPoint < slices.nPointLocal; iPoint++)
    for (unsigned short iDim = 0; iDim < nDim; iDim++) slices.coord[iDim][iPoint] = coord[iPoint * nDim + iDim];

  slices.metric.resize(slices.nPointLocal * slices.nMetric);
  unpack.GetArray(slices.metric.data(), slices.metric.size());

  slices.elemRows.resize(slices.nElemLocal * SU2_CONN_SIZE);
  unpack.GetArray(slices.elemRows.data(), slices.elemRows.size());

  if (!unpack.AtEnd()) SU2_MPI::Error("Reader slice with trailing data.", CURRENT_FUNCTION);
  return slices;
}

}  // namespace

CReaderSlices CReaderSlices::FromComplete(const CSimplexMesh& mesh, int root) {
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();

  if (rank != root) return UnpackSlice(CPassiveComm::RecvRounds(root, SLICE_TAG));

  CheckMesh(mesh);
  const unsigned short nNode = mesh.nDim + 1;
  const unsigned long nElem = mesh.GetnElem();
  const CLinearPartitioner partitioner(mesh.GetnPoint(), 0);

  /*--- The elements with a point in each rank's slice, in increasing order (an element is listed once per rank). ---*/
  std::vector<unsigned long> offset(size + 1, 0), elems;
  {
    auto ranksOf = [&](unsigned long iElem, int* ranks) {
      int n = 0;
      for (unsigned short iNode = 0; iNode < nNode; iNode++) {
        const int r = static_cast<int>(partitioner.GetRankContainingIndex(mesh.elem[iElem * nNode + iNode]));
        if (std::find(ranks, ranks + n, r) == ranks + n) ranks[n++] = r;
      }
      return n;
    };
    int ranks[4];
    for (unsigned long iElem = 0; iElem < nElem; iElem++) {
      const int n = ranksOf(iElem, ranks);
      for (int i = 0; i < n; i++) offset[ranks[i] + 1]++;
    }
    for (int q = 0; q < size; q++) offset[q + 1] += offset[q];
    elems.resize(offset[size]);
    auto next = offset;
    for (unsigned long iElem = 0; iElem < nElem; iElem++) {
      const int n = ranksOf(iElem, ranks);
      for (int i = 0; i < n; i++) elems[next[ranks[i]]++] = iElem;
    }
  }

  /*--- One rank at a time: the root holds the rows of one rank only. ---*/
  for (int q = 0; q < size; q++) {
    if (q == root) continue;
    const auto bytes = PackSlice(mesh, partitioner, q, elems.data() + offset[q], offset[q + 1] - offset[q]);
    CPassiveComm::SendRounds(bytes.data(), bytes.size(), q, SLICE_TAG);
  }
  return UnpackSlice(PackSlice(mesh, partitioner, root, elems.data() + offset[root], offset[root + 1] - offset[root]));
}

std::vector<passivedouble> CReaderSlices::FetchPointMetric(const CGeometry& geometry) const {
  std::vector<passivedouble> local;
  if (nMetric == 0) return local;

  const int size = SU2_MPI::GetSize();
  const CLinearPartitioner partitioner(nPointGlobal, 0);
  const unsigned long nPoint = geometry.GetnPoint();

  /*--- Requests: the global index of every local point, grouped by the rank whose slice holds it. ---*/
  std::vector<size_t> count(size, 0);
  std::vector<int> owner(nPoint);
  for (unsigned long iPoint = 0; iPoint < nPoint; iPoint++) {
    const auto id = geometry.nodes->GetGlobalIndex(iPoint);
    if (id >= nPointGlobal) SU2_MPI::Error("A point of the geometry is not in the new mesh.", CURRENT_FUNCTION);
    owner[iPoint] = static_cast<int>(partitioner.GetRankContainingIndex(id));
    count[owner[iPoint]]++;
  }
  std::vector<size_t> start(size + 1, 0);
  for (int q = 0; q < size; q++) start[q + 1] = start[q] + count[q];
  std::vector<uint64_t> request(nPoint);
  std::vector<unsigned long> position(nPoint);
  {
    auto next = start;
    for (unsigned long iPoint = 0; iPoint < nPoint; iPoint++) {
      position[iPoint] = next[owner[iPoint]]++;
      request[position[iPoint]] = geometry.nodes->GetGlobalIndex(iPoint);
    }
  }
  std::vector<size_t> recvCount;
  const auto received = CPassiveComm::Alltoallv(request, count, recvCount);

  /*--- Replies: the metric of each requested point, in the order of the requests. ---*/
  std::vector<passivedouble> reply(received.size() * nMetric);
  for (size_t i = 0; i < received.size(); i++) {
    if (received[i] < firstPoint || received[i] >= firstPoint + nPointLocal) {
      SU2_MPI::Error("Metric requested for a point outside the slice of this rank.", CURRENT_FUNCTION);
    }
    std::copy_n(&metric[(received[i] - firstPoint) * nMetric], nMetric, &reply[i * nMetric]);
  }
  std::vector<size_t> replyCount(size), backCount;
  for (int p = 0; p < size; p++) replyCount[p] = recvCount[p] * nMetric;
  const auto values = CPassiveComm::Alltoallv(reply, replyCount, backCount);

  local.resize(nPoint * nMetric);
  for (unsigned long iPoint = 0; iPoint < nPoint; iPoint++)
    std::copy_n(&values[position[iPoint] * nMetric], nMetric, &local[iPoint * nMetric]);
  return local;
}
