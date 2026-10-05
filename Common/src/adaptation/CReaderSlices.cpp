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
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>

#include "../../include/adaptation/CDistributedSearch.hpp"
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

/*--- Explicit scalar packing, using the existing passive transport. No struct layout is transmitted. ---*/
std::vector<char> ExchangePacks(const std::vector<Packer>& packs) {
  std::vector<size_t> sendBytes(packs.size()), recvBytes;
  size_t total = 0;
  for (size_t r = 0; r < packs.size(); ++r) {
    sendBytes[r] = packs[r].bytes.size();
    total += sendBytes[r];
  }
  std::vector<char> send;
  send.reserve(total);
  for (const auto& pack : packs) send.insert(send.end(), pack.bytes.begin(), pack.bytes.end());
  return CPassiveComm::AlltoallvRounds(send.data(), sendBytes, recvBytes);
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

CReaderSlices CReaderSlices::FromDistributed(const CSimplexMesh& mesh, const std::vector<uint64_t>& pointKeys) {
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
  CLocalFailure failure;
  auto fail = [&](uint64_t id, const std::string& reason) {
    failure.Set(1, id, "Distributed reader slices: " + reason);
  };
  const auto nDim = mesh.nDim;
  const auto minDim = CPassiveComm::AllreduceMin(nDim), maxDim = CPassiveComm::AllreduceMax(nDim);
  if ((nDim != 2 && nDim != 3) || minDim != maxDim) fail(0, "inconsistent or unsupported dimension.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const unsigned short nNode = nDim + 1;
  const unsigned short nMetric = CPassiveComm::AllreduceMax(mesh.metric.empty() ? 0 : CSimplexMesh::GetnMetric(nDim));
  const auto nPoint = mesh.GetnPoint(), nElem = mesh.GetnElem();
  if (mesh.coord.size() != nPoint * nDim || pointKeys.size() != nPoint || mesh.elem.size() != nElem * nNode ||
      mesh.metric.size() != nPoint * nMetric) fail(0, "inconsistent local array sizes.");
  CollectiveFailure(failure, CURRENT_FUNCTION);

  Packer names;
  names.Put<uint64_t>(mesh.markers.size());
  std::set<std::string> uniqueNames;
  for (const auto& marker : mesh.markers) {
    names.PutString(marker.name);
    if (marker.name.empty() || !uniqueNames.insert(marker.name).second) fail(0, "empty or repeated marker name.");
  }
  auto rootNames = rank == MASTER_NODE ? names.bytes : std::vector<char>{};
  CPassiveComm::BcastRounds(rootNames, MASTER_NODE);
  if (names.bytes != rootNames) fail(0, "marker names/order differ between ranks.");

  std::set<uint64_t> uniqueKeys;
  std::vector<uint8_t> used(nPoint, 0);
  for (unsigned long i = 0; i < nPoint; ++i) {
    if (!uniqueKeys.insert(pointKeys[i]).second) fail(pointKeys[i], "repeated local point identity.");
    for (unsigned short d = 0; d < nDim; ++d)
      if (!std::isfinite(mesh.coord[i * nDim + d])) fail(pointKeys[i], "nonfinite point coordinate.");
    for (unsigned short m = 0; m < nMetric; ++m)
      if (!std::isfinite(mesh.metric[i * nMetric + m])) fail(pointKeys[i], "nonfinite point metric.");
  }
  for (unsigned long e = 0; e < nElem; ++e) {
    std::set<unsigned long> nodes;
    for (unsigned short k = 0; k < nNode; ++k) {
      const auto p = mesh.elem[e * nNode + k];
      if (p >= nPoint) fail(e, "volume point index out of range.");
      else { used[p] = 1; nodes.insert(p); }
    }
    if (nodes.size() != nNode) fail(e, "repeated node in a volume element.");
  }
  for (const auto& marker : mesh.markers) {
    if (marker.elem.size() % nDim) fail(0, "inconsistent boundary connectivity size.");
    for (size_t e = 0; e < marker.elem.size() / nDim; ++e) {
      std::set<unsigned long> nodes;
      for (unsigned short k = 0; k < nDim; ++k) {
        const auto p = marker.elem[e * nDim + k];
        if (p >= nPoint) fail(e, "boundary point index out of range.");
        else nodes.insert(p);
      }
      if (nodes.size() != nDim) fail(e, "repeated node in a boundary element.");
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);

  struct PointRecord {
    std::array<passivedouble, 3> coord{};
    std::array<passivedouble, 6> metric{};
    std::vector<std::pair<int, uint64_t>> requesters;
    bool used = false;
  };
  std::map<uint64_t, PointRecord> points;
  {
    std::vector<Packer> to(size);
    for (unsigned long i = 0; i < nPoint; ++i) {
      auto& pack = to[pointKeys[i] % size];
      pack.Put<uint64_t>(pointKeys[i]);
      pack.Put<uint64_t>(rank);
      pack.Put<uint64_t>(i);
      pack.Put<uint8_t>(used[i]);
      pack.PutArray(&mesh.coord[i * nDim], nDim);
      if (nMetric) pack.PutArray(&mesh.metric[i * nMetric], nMetric);
    }
    const auto bytes = ExchangePacks(to);
    Unpacker in(bytes);
    while (!in.AtEnd()) {
      const auto key = in.Get<uint64_t>(), source = in.Get<uint64_t>(), index = in.Get<uint64_t>();
      PointRecord record;
      record.used = in.Get<uint8_t>() != 0;
      in.GetArray(record.coord.data(), nDim);
      in.GetArray(record.metric.data(), nMetric);
      auto inserted = points.emplace(key, record);
      auto& point = inserted.first->second;
      if (!inserted.second && (point.coord != record.coord || point.metric != record.metric))
        fail(key, "shared coordinates or metric differ.");
      point.used |= record.used;
      point.requesters.emplace_back(static_cast<int>(source), index);
    }
  }
  for (const auto& entry : points)
    if (!entry.second.used) fail(entry.first, "point is not used by any volume element.");
  CollectiveFailure(failure, CURRENT_FUNCTION);

  /*--- Reject duplicate volume/physical-face ownership at distributed key owners. A physical face with two
   *    different markers is also a duplicate; marker identity is not part of the face's uniqueness key. ---*/
  {
    std::vector<Packer> to(size);
    auto signature = [&](const unsigned long* localNodes, unsigned short count) {
      std::array<uint64_t, 4> key{};
      for (unsigned short k = 0; k < count; ++k) key[k] = pointKeys[localNodes[k]];
      std::sort(key.begin(), key.begin() + count);
      auto& pack = to[key[0] % size];
      pack.Put<uint64_t>(count);
      pack.PutArray(key.data(), key.size());
    };
    for (unsigned long e = 0; e < nElem; ++e) signature(&mesh.elem[e * nNode], nNode);
    for (const auto& marker : mesh.markers)
      for (unsigned long e = 0; e < marker.GetnElem(nDim); ++e) signature(&marker.elem[e * nDim], nDim);
    const auto bytes = ExchangePacks(to);
    Unpacker in(bytes);
    std::set<std::pair<uint64_t, std::array<uint64_t, 4>>> seen;
    while (!in.AtEnd()) {
      const auto count = in.Get<uint64_t>();
      std::array<uint64_t, 4> key;
      in.GetArray(key.data(), key.size());
      if (!seen.emplace(count, key).second) fail(key[0], "duplicate volume or physical-face ownership.");
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);

  CReaderSlices slices;
  slices.nDim = nDim;
  slices.nMetric = nMetric;
  slices.nPointGlobal = CPassiveComm::AllreduceSum(points.size());
  slices.nElemGlobal = CPassiveComm::AllreduceSum(nElem);
  if (!slices.nPointGlobal || !slices.nElemGlobal) fail(0, "globally empty mesh.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const CLinearPartitioner partitioner(slices.nPointGlobal, 0);
  slices.firstPoint = partitioner.GetFirstIndexOnRank(rank);
  slices.nPointLocal = partitioner.GetSizeOnRank(rank);
  slices.coord.assign(nDim, std::vector<passivedouble>(slices.nPointLocal));
  slices.metric.resize(slices.nPointLocal * nMetric);
  std::vector<unsigned long> dense(nPoint);
  {
    std::vector<Packer> replies(size), coords(size);
    auto id = CPassiveComm::ExscanSum(points.size());
    for (const auto& entry : points) {
      const auto& point = entry.second;
      for (const auto& source : point.requesters) {
        replies[source.first].Put<uint64_t>(source.second);
        replies[source.first].Put<uint64_t>(id);
      }
      auto& pack = coords[partitioner.GetRankContainingIndex(id)];
      pack.Put<uint64_t>(id++);
      pack.PutArray(point.coord.data(), nDim);
      pack.PutArray(point.metric.data(), nMetric);
    }
    const auto replyBytes = ExchangePacks(replies);
    Unpacker reply(replyBytes);
    std::vector<uint8_t> found(nPoint, 0);
    while (!reply.AtEnd()) {
      const auto index = reply.Get<uint64_t>(), newId = reply.Get<uint64_t>();
      if (index >= nPoint || newId >= slices.nPointGlobal || (index < nPoint && found[index]++))
        fail(index, "invalid dense identity reply.");
      else dense[index] = newId;
    }
    if (std::find(found.begin(), found.end(), 0) != found.end()) fail(0, "missing dense identity reply.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const auto coordBytes = ExchangePacks(coords);
    Unpacker coord(coordBytes);
    std::vector<uint8_t> present(slices.nPointLocal, 0);
    while (!coord.AtEnd()) {
      const auto newId = coord.Get<uint64_t>();
      PointRecord record;
      coord.GetArray(record.coord.data(), nDim);
      coord.GetArray(record.metric.data(), nMetric);
      if (newId < slices.firstPoint || newId >= slices.firstPoint + slices.nPointLocal) {
        fail(newId, "point routed outside linear slice.");
        continue;
      }
      const auto i = newId - slices.firstPoint;
      if (present[i]++) fail(newId, "duplicate linear-slice point.");
      for (unsigned short d = 0; d < nDim; ++d) slices.coord[d][i] = record.coord[d];
      for (unsigned short m = 0; m < nMetric; ++m) slices.metric[i * nMetric + m] = record.metric[m];
    }
    if (std::find(present.begin(), present.end(), 0) != present.end()) fail(0, "missing linear-slice point.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
  }

  {
    std::vector<Packer> to(size);
    const auto firstElement = CPassiveComm::ExscanSum(nElem);
    for (unsigned long e = 0; e < nElem; ++e) {
      unsigned long row[SU2_CONN_SIZE] = {firstElement + e, nDim == 2 ? TRIANGLE : TETRAHEDRON};
      std::set<int> destinations;
      for (unsigned short k = 0; k < nNode; ++k) {
        row[k + 2] = dense[mesh.elem[e * nNode + k]];
        destinations.insert(partitioner.GetRankContainingIndex(row[k + 2]));
      }
      for (const int destination : destinations) to[destination].PutArray(row, SU2_CONN_SIZE);
    }
    const auto bytes = ExchangePacks(to);
    slices.elemRows = CPassiveComm::FromBytes<unsigned long>(bytes);
    slices.nElemLocal = slices.elemRows.size() / SU2_CONN_SIZE;
    /*--- Source-rank concatenation is already ordered by prefix-allocated element identity. ---*/
  }

  std::vector<unsigned long> localBoundaryCounts(mesh.markers.size(), 0), boundaryCounts(mesh.markers.size(), 0);
  Packer boundary;
  for (size_t m = 0; m < mesh.markers.size(); ++m) {
    const auto& marker = mesh.markers[m];
    slices.markerNames.push_back(marker.name);
    localBoundaryCounts[m] = marker.GetnElem(nDim);
    for (unsigned long e = 0; e < marker.GetnElem(nDim); ++e) {
      unsigned long row[SU2_CONN_SIZE] = {0, nDim == 2 ? LINE : TRIANGLE};
      for (unsigned short k = 0; k < nDim; ++k) row[k + 2] = dense[marker.elem[e * nDim + k]];
      boundary.Put<uint64_t>(m);
      boundary.PutArray(row, SU2_CONN_SIZE);
    }
  }
  CPassiveComm::Allreduce(localBoundaryCounts.data(), boundaryCounts.data(), boundaryCounts.size(), CPassiveComm::Op::SUM);
  for (size_t m = 0; m < boundaryCounts.size(); ++m)
    if (boundaryCounts[m]) slices.markersWithElements.push_back(slices.markerNames[m]);
  const auto boundaryBytes = CPassiveComm::GathervRounds(boundary.bytes.data(), boundary.bytes.size(), MASTER_NODE, nullptr);
  slices.boundaryRows.resize(mesh.markers.size());
  if (rank == MASTER_NODE) {
    Unpacker in(boundaryBytes);
    while (!in.AtEnd()) {
      const auto m = in.Get<uint64_t>();
      unsigned long row[SU2_CONN_SIZE];
      in.GetArray(row, SU2_CONN_SIZE);
      slices.boundaryRows[m].insert(slices.boundaryRows[m].end(), row, row + SU2_CONN_SIZE);
    }
  }
  return slices;
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
