/*!
 * \file CDistributedSearch.cpp
 * \brief Building blocks of the distributed solution transfer (ownership, keys, box tree, directory, failures, sums).
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

#include "../../include/adaptation/CDistributedSearch.hpp"

#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>
#include <numeric>

#include "../../include/CConfig.hpp"
#include "../../include/adaptation/CAccurateSum.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include "../../include/parallelization/CPassiveComm.hpp"

CSimplexKey MakeSimplexKey(const uint64_t* gids, unsigned short n) {
  CSimplexKey key = {UINT64_MAX, UINT64_MAX, UINT64_MAX, UINT64_MAX};
  for (unsigned short i = 0; i < n; ++i) key[i] = gids[i];
  std::sort(key.begin(), key.begin() + n);
  return key;
}

namespace {

/*--- Whether this rank owns an element: its node of smallest global index is a domain point. ---*/
bool OwnsElement(const CGeometry& geometry, const CPrimalGrid* element, uint64_t* gids) {
  uint64_t smallest = UINT64_MAX;
  unsigned long owner = 0;
  for (unsigned short iNode = 0; iNode < element->GetnNodes(); ++iNode) {
    const auto iPoint = element->GetNode(iNode);
    gids[iNode] = geometry.nodes->GetGlobalIndex(iPoint);
    if (gids[iNode] < smallest) {
      smallest = gids[iNode];
      owner = iPoint;
    }
  }
  return geometry.nodes->GetDomain(owner);
}

std::string TagOf(const std::vector<std::string>& markerTags, unsigned short iMarker) {
  return iMarker < markerTags.size() ? markerTags[iMarker] : std::string();
}

}  // namespace

COwnedSimplices OwnedSimplices(const CGeometry& geometry) {
  COwnedSimplices owned;
  owned.nDim = geometry.GetnDim();
  if (owned.nDim != 2 && owned.nDim != 3) SU2_MPI::Error("The solution transfer needs a 2D or 3D mesh.", CURRENT_FUNCTION);
  owned.nNode = owned.nDim + 1;
  const unsigned short simplex = (owned.nDim == 2) ? TRIANGLE : TETRAHEDRON;
  uint64_t gids[8];
  for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
    const auto* element = geometry.elem[iElem];
    if (element->GetVTK_Type() != simplex) {
      SU2_MPI::Error(std::string("The solution transfer supports only ") + (owned.nDim == 2 ? "triangles" : "tetrahedra") +
                         " (element " + std::to_string(element->GetGlobalIndex()) + ").",
                     CURRENT_FUNCTION);
    }
    if (!OwnsElement(geometry, element, gids)) continue;
    owned.elem.push_back(iElem);
    for (unsigned short iNode = 0; iNode < owned.nNode; ++iNode) owned.nodes.push_back(element->GetNode(iNode));
    owned.keys.push_back(MakeSimplexKey(gids, owned.nNode));
  }
  return owned;
}

uint32_t MarkerConfigId(const CConfig& config, const std::string& name) {
  const auto nMarker = config.GetnMarker_CfgFile();
  if (name.empty()) return nMarker;
  for (unsigned short iMarker = 0; iMarker < nMarker; ++iMarker)
    if (config.GetMarker_CfgFile_TagBound(iMarker) == name) return iMarker;
  SU2_MPI::Error("The configuration file has no marker " + name + ".", CURRENT_FUNCTION);
  return nMarker;
}

COwnedFaces OwnedBoundaryFaces(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                               const CConfig& config) {
  COwnedFaces faces;
  faces.nDim = geometry.GetnDim();
  const unsigned short faceType = (faces.nDim == 2) ? LINE : TRIANGLE;
  uint64_t gids[8];
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    const auto tag = TagOf(markerTags, iMarker);
    if (tag == "SEND_RECEIVE" || geometry.GetnElem_Bound(iMarker) == 0) continue;
    if (!tag.empty() && config.GetMarker_CfgFile_KindBC(tag) == PERIODIC_BOUNDARY) {
      SU2_MPI::Error("The solution transfer does not support periodic markers (" + tag + ").", CURRENT_FUNCTION);
    }
    const auto id = MarkerConfigId(config, tag);
    for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
      const auto* element = geometry.bound[iMarker][iElem];
      if (element->GetVTK_Type() == VERTEX) continue;
      if (element->GetVTK_Type() != faceType) {
        SU2_MPI::Error(std::string("The solution transfer supports only ") + (faces.nDim == 2 ? "line" : "triangle") +
                           " boundary elements (marker " + tag + ").",
                       CURRENT_FUNCTION);
      }
      if (!OwnsElement(geometry, element, gids)) continue;
      faces.markerId.push_back(id);
      for (unsigned short iNode = 0; iNode < faces.nDim; ++iNode) faces.nodes.push_back(element->GetNode(iNode));
      faces.keys.push_back(MakeSimplexKey(gids, faces.nDim));
    }
  }
  return faces;
}

std::vector<std::vector<uint32_t>> PointMarkerIds(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                                                  const CConfig& config) {
  std::vector<std::vector<uint32_t>> ids(geometry.GetnPoint());
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker) {
    const auto tag = TagOf(markerTags, iMarker);
    if (tag == "SEND_RECEIVE" || geometry.GetnElem_Bound(iMarker) == 0) continue;
    const auto id = MarkerConfigId(config, tag);
    for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
      const auto* element = geometry.bound[iMarker][iElem];
      if (element->GetVTK_Type() == VERTEX) continue;
      for (unsigned short iNode = 0; iNode < element->GetnNodes(); ++iNode) ids[element->GetNode(iNode)].push_back(id);
    }
  }
  for (auto& list : ids) {
    std::sort(list.begin(), list.end());
    list.erase(std::unique(list.begin(), list.end()), list.end());
  }
  return ids;
}

void GlobalBoundingBox(const CGeometry& geometry, double* xMin, double* xMax, double* diagonal) {
  const auto nDim = geometry.GetnDim();
  double lo[3], hi[3];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    lo[iDim] = std::numeric_limits<double>::max();
    hi[iDim] = std::numeric_limits<double>::lowest();
  }
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      const double x = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
      lo[iDim] = std::min(lo[iDim], x);
      hi[iDim] = std::max(hi[iDim], x);
    }
  }
  CPassiveComm::Allreduce(lo, xMin, nDim, CPassiveComm::Op::MIN);
  CPassiveComm::Allreduce(hi, xMax, nDim, CPassiveComm::Op::MAX);
  double size = 0.0;
  bool any = true;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    any = any && xMin[iDim] <= xMax[iDim];
    const double extent = xMax[iDim] - xMin[iDim];
    if (any) size += extent * extent;
  }
  *diagonal = any ? std::sqrt(size) : 0.0;
}

std::vector<double> BisectionBoxes(unsigned short nDim, const std::vector<double>& itemBoxes,
                                   const std::vector<double>& centroids, unsigned long maxBoxes,
                                   std::vector<unsigned long>* groupOfItem) {
  const unsigned long nItem = centroids.size() / nDim;
  std::vector<unsigned long> order(nItem);
  std::iota(order.begin(), order.end(), 0ul);
  int depth = 0;
  while ((2ul << depth) <= std::max(maxBoxes, 1ul)) ++depth;

  std::vector<std::pair<unsigned long, unsigned long>> groups;  // ranges of order
  /*--- Recursive bisection (explicit stack, depth-first, lower half first). ---*/
  struct Range {
    unsigned long first, last;
    int depth;
  };
  std::vector<Range> stack;
  if (nItem > 0) stack.push_back({0, nItem, 0});
  while (!stack.empty()) {
    const auto range = stack.back();
    stack.pop_back();
    const auto n = range.last - range.first;
    if (range.depth >= depth || n <= 1) {
      groups.emplace_back(range.first, range.last);
      continue;
    }
    double lo[3], hi[3];
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      lo[iDim] = std::numeric_limits<double>::max();
      hi[iDim] = std::numeric_limits<double>::lowest();
    }
    for (auto i = range.first; i < range.last; ++i) {
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        lo[iDim] = std::min(lo[iDim], centroids[order[i] * nDim + iDim]);
        hi[iDim] = std::max(hi[iDim], centroids[order[i] * nDim + iDim]);
      }
    }
    unsigned short axis = 0;
    for (unsigned short iDim = 1; iDim < nDim; ++iDim)
      if (hi[iDim] - lo[iDim] > hi[axis] - lo[axis]) axis = iDim;
    const auto mid = range.first + n / 2;
    std::nth_element(order.begin() + range.first, order.begin() + mid, order.begin() + range.last,
                     [&](unsigned long a, unsigned long b) {
                       const double xa = centroids[a * nDim + axis], xb = centroids[b * nDim + axis];
                       return xa < xb || (xa == xb && a < b);
                     });
    /*--- Lower half processed first (pushed last). ---*/
    stack.push_back({mid, range.last, range.depth + 1});
    stack.push_back({range.first, mid, range.depth + 1});
  }

  std::vector<double> boxes;
  if (groupOfItem != nullptr) groupOfItem->assign(nItem, 0);
  for (auto iGroup = 0ul; iGroup < groups.size(); ++iGroup) {
    double box[6];
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      box[iDim] = std::numeric_limits<double>::max();
      box[nDim + iDim] = std::numeric_limits<double>::lowest();
    }
    for (auto i = groups[iGroup].first; i < groups[iGroup].second; ++i) {
      const auto item = order[i];
      if (groupOfItem != nullptr) (*groupOfItem)[item] = iGroup;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        box[iDim] = std::min(box[iDim], itemBoxes[item * 2 * nDim + iDim]);
        box[nDim + iDim] = std::max(box[nDim + iDim], itemBoxes[item * 2 * nDim + nDim + iDim]);
      }
    }
    boxes.insert(boxes.end(), box, box + 2 * nDim);
  }
  return boxes;
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- CRankBoxTree                                                                                                 ---*/
/*------------------------------------------------------------------------------------------------------------------*/

void CRankBoxTree::Build(unsigned short nDimIn, const std::vector<double>& localBoxes,
                         CPassiveComm::Communicator comm) {
  nDim = nDimIn;
  std::vector<size_t> counts;
  boxes = CPassiveComm::Allgatherv(localBoxes, &counts, comm);
  const auto size = counts.size();
  firstBox.assign(size + 1, 0);
  rank.clear();
  localIndex.clear();
  for (size_t iRank = 0; iRank < size; ++iRank) {
    const auto n = counts[iRank] / (2 * nDim);
    firstBox[iRank + 1] = firstBox[iRank] + n;
    for (auto i = 0ul; i < n; ++i) {
      rank.push_back(static_cast<int>(iRank));
      localIndex.push_back(i);
    }
  }

  /*--- Bounding volume hierarchy: median split of the box centres along the longest axis, leaves of up to 4 boxes. ---*/
  const auto nBox = rank.size();
  order.resize(nBox);
  std::iota(order.begin(), order.end(), 0ul);
  nodes.clear();
  if (nBox == 0) return;
  struct Task {
    unsigned long node, first, last;
  };
  std::vector<Task> stack;
  nodes.push_back(Node());
  stack.push_back({0, 0, nBox});
  while (!stack.empty()) {
    const auto task = stack.back();
    stack.pop_back();
    Node node;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      node.lo[iDim] = std::numeric_limits<double>::max();
      node.hi[iDim] = std::numeric_limits<double>::lowest();
    }
    for (auto i = task.first; i < task.last; ++i) {
      const double* box = Box(order[i]);
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        node.lo[iDim] = std::min(node.lo[iDim], box[iDim]);
        node.hi[iDim] = std::max(node.hi[iDim], box[nDim + iDim]);
      }
    }
    node.first = task.first;
    node.last = task.last;
    node.child[0] = node.child[1] = -1;
    if (task.last - task.first > 4) {
      unsigned short axis = 0;
      for (unsigned short iDim = 1; iDim < nDim; ++iDim)
        if (node.hi[iDim] - node.lo[iDim] > node.hi[axis] - node.lo[axis]) axis = iDim;
      const auto mid = task.first + (task.last - task.first) / 2;
      auto centre = [&](unsigned long iBox) { return Box(iBox)[axis] + Box(iBox)[nDim + axis]; };
      std::nth_element(order.begin() + task.first, order.begin() + mid, order.begin() + task.last,
                       [&](unsigned long a, unsigned long b) {
                         const double ca = centre(a), cb = centre(b);
                         return ca < cb || (ca == cb && a < b);
                       });
      node.child[0] = nodes.size();
      nodes.push_back(Node());
      node.child[1] = nodes.size();
      nodes.push_back(Node());
      stack.push_back({static_cast<unsigned long>(node.child[0]), task.first, mid});
      stack.push_back({static_cast<unsigned long>(node.child[1]), mid, task.last});
    }
    nodes[task.node] = node;
  }
}

template <class Test>
void CRankBoxTree::Query(const Test& test, std::vector<unsigned long>& result) const {
  result.clear();
  if (nodes.empty()) return;
  std::vector<unsigned long> stack = {0};
  while (!stack.empty()) {
    const auto& node = nodes[stack.back()];
    stack.pop_back();
    if (!test(node.lo, node.hi)) continue;
    if (node.child[0] < 0) {
      for (auto i = node.first; i < node.last; ++i) {
        const double* box = Box(order[i]);
        if (test(box, box + nDim)) result.push_back(order[i]);
      }
    } else {
      stack.push_back(node.child[0]);
      stack.push_back(node.child[1]);
    }
  }
  std::sort(result.begin(), result.end());
}

void CRankBoxTree::BoxesContaining(const double* x, std::vector<unsigned long>& result) const {
  Query(
      [&](const double* lo, const double* hi) {
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          if (x[iDim] < lo[iDim] || x[iDim] > hi[iDim]) return false;
        return true;
      },
      result);
}

void CRankBoxTree::BoxesIntersecting(const double* qLo, const double* qHi, std::vector<unsigned long>& result) const {
  Query(
      [&](const double* lo, const double* hi) {
        for (unsigned short iDim = 0; iDim < nDim; ++iDim)
          if (lo[iDim] > qHi[iDim] || hi[iDim] < qLo[iDim]) return false;
        return true;
      },
      result);
}

void CRankBoxTree::BoxesWithinDistance(const double* x, double r, std::vector<unsigned long>& result) const {
  const double r2 = r * r;
  Query(
      [&](const double* lo, const double* hi) {
        double d2 = 0.0;
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
          const double ds = std::min(0.0, x[iDim] - lo[iDim]) + std::max(0.0, x[iDim] - hi[iDim]);
          d2 += ds * ds;
        }
        return d2 <= r2;
      },
      result);
}

void CRankBoxTree::ToRanks(const std::vector<unsigned long>& boxIds, std::vector<int>& result) const {
  result.clear();
  for (const auto iBox : boxIds) result.push_back(rank[iBox]);
  std::sort(result.begin(), result.end());
  result.erase(std::unique(result.begin(), result.end()), result.end());
}

size_t CRankBoxTree::GetMemory() const {
  return boxes.capacity() * sizeof(double) + rank.capacity() * sizeof(int) +
         (localIndex.capacity() + firstBox.capacity() + order.capacity()) * sizeof(unsigned long) +
         nodes.capacity() * sizeof(Node);
}

size_t CRankBoxTree::QueryBytes() const {
  /*--- Box ids and the traversal stack (at most every box / every node), the ranks of the boxes before they are
   *    made unique (at most one per box). ---*/
  return transfer_memory::Add(transfer_memory::GrowthBound(rank.size(), sizeof(unsigned long)),
                              transfer_memory::GrowthBound(nodes.size() + 1, sizeof(unsigned long)),
                              transfer_memory::GrowthBound(rank.size(), sizeof(int)));
}

void CRankBoxTree::RanksContaining(const double* x, std::vector<int>& result) const {
  std::vector<unsigned long> ids;
  BoxesContaining(x, ids);
  ToRanks(ids, result);
}

void CRankBoxTree::RanksIntersecting(const double* lo, const double* hi, std::vector<int>& result) const {
  std::vector<unsigned long> ids;
  BoxesIntersecting(lo, hi, ids);
  ToRanks(ids, result);
}

void CRankBoxTree::RanksWithinDistance(const double* x, double r, std::vector<int>& result) const {
  std::vector<unsigned long> ids;
  BoxesWithinDistance(x, r, ids);
  ToRanks(ids, result);
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- Memory ceiling                                                                                               ---*/
/*------------------------------------------------------------------------------------------------------------------*/

namespace {
size_t transferMemoryCeiling = size_t(2) << 30;
}

size_t GetTransferMemoryCeiling() { return transferMemoryCeiling; }
void SetTransferMemoryCeiling(size_t bytes) { transferMemoryCeiling = bytes; }

CTransferRoundScope::CTransferRoundScope() : saved(CPassiveComm::GetRoundBytes()) {
  const size_t bytes = std::max<size_t>(std::min(saved, transferMemoryCeiling / 8), 4096 + SU2_MPI::GetSize());
  CPassiveComm::SetRoundBytes(std::min<size_t>(bytes, INT_MAX));
}

CTransferRoundScope::~CTransferRoundScope() { CPassiveComm::SetRoundBytes(saved); }

/*------------------------------------------------------------------------------------------------------------------*/
/*--- Failures                                                                                                     ---*/
/*------------------------------------------------------------------------------------------------------------------*/

CElectedFailure ElectFailure(const CLocalFailure& local, CPassiveComm::Communicator comm) {
  CElectedFailure elected;
  const int rank = CPassiveComm::Rank(comm);
  const unsigned long severity = CPassiveComm::AllreduceMax(local.severity, comm);
  if (severity == 0) return elected;
  const bool candidate = local.severity == severity;
  const unsigned long gid = CPassiveComm::AllreduceMin(candidate ? static_cast<unsigned long>(local.gid) : ULONG_MAX, comm);
  const bool tied = candidate && static_cast<unsigned long>(local.gid) == gid;
  const unsigned long root = CPassiveComm::AllreduceMin(tied ? static_cast<unsigned long>(rank) : ULONG_MAX, comm);
  std::vector<char> text;
  if (static_cast<unsigned long>(rank) == root) text.assign(local.message.begin(), local.message.end());
  CPassiveComm::Bcast(text, static_cast<int>(root), comm);
  elected.any = true;
  elected.rank = static_cast<int>(root);
  elected.severity = static_cast<unsigned short>(severity);
  elected.gid = gid;
  elected.message.assign(text.begin(), text.end());
  return elected;
}

void CollectiveFailure(const CLocalFailure& local, const std::string& function) {
  const auto elected = ElectFailure(local);
  if (elected.any) SU2_MPI::Error(elected.message, function);
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- CPointDirectory                                                                                              ---*/
/*------------------------------------------------------------------------------------------------------------------*/

int CPointDirectory::Owner(uint64_t gid) const {
  const int size = SU2_MPI::GetSize();
  return static_cast<int>(std::min<uint64_t>(size - 1, gid / blockSize));
}

void CPointDirectory::Build(const std::vector<uint64_t>& gids, const std::vector<char>& records, size_t recordBytesIn,
                            const std::string& what) {
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  recordBytes = recordBytesIn;
  if (records.size() != gids.size() * recordBytes) SU2_MPI::Error("Wrong size of the directory records.", CURRENT_FUNCTION);
  nGlobal = CPassiveComm::AllreduceSum(gids.size());
  blockSize = std::max<uint64_t>(1, (nGlobal + size - 1) / size);
  firstGid = std::min<uint64_t>(nGlobal, rank * blockSize);
  const uint64_t lastGid = (rank == size - 1) ? nGlobal : std::min<uint64_t>(nGlobal, (rank + 1) * blockSize);

  CLocalFailure failure;
  auto badIndex = [&](uint64_t gid, const std::string& problem) {
    failure.Set(1, gid,
                "The global indices of the " + what + " points are not a numbering 0 .. N-1 (N = " +
                    std::to_string(nGlobal) + "): index " + std::to_string(gid) + " " + problem + ".");
  };

  /*--- Records (gid, record) by block owner. ---*/
  const size_t stride = sizeof(uint64_t) + recordBytes;
  std::vector<size_t> sendBytes(size, 0);
  std::vector<int> dest(gids.size(), -1);
  for (auto i = 0ul; i < gids.size(); ++i) {
    if (gids[i] >= nGlobal) {
      badIndex(gids[i], "is out of range");
      continue;
    }
    dest[i] = Owner(gids[i]);
    sendBytes[dest[i]] += stride;
  }
  std::vector<size_t> offset(size + 1, 0);
  for (int q = 0; q < size; ++q) offset[q + 1] = offset[q] + sendBytes[q];
  std::vector<char> send(offset[size]);
  auto position = offset;
  for (auto i = 0ul; i < gids.size(); ++i) {
    if (dest[i] < 0) continue;
    std::memcpy(&send[position[dest[i]]], &gids[i], sizeof(uint64_t));
    if (recordBytes > 0) std::memcpy(&send[position[dest[i]] + sizeof(uint64_t)], &records[i * recordBytes], recordBytes);
    position[dest[i]] += stride;
  }
  std::vector<size_t> recvBytes;
  const auto received = CPassiveComm::AlltoallvRounds(send.data(), sendBytes, recvBytes);
  send = std::vector<char>();

  const auto nLocal = lastGid - firstGid;
  block.assign(nLocal * recordBytes, 0);
  std::vector<char> filled(nLocal, 0);
  for (size_t pos = 0; pos + stride <= received.size(); pos += stride) {
    uint64_t gid = 0;
    std::memcpy(&gid, &received[pos], sizeof(uint64_t));
    const auto slot = gid - firstGid;
    if (filled[slot]) {
      badIndex(gid, "appears twice");
      continue;
    }
    filled[slot] = 1;
    if (recordBytes > 0) std::memcpy(&block[slot * recordBytes], &received[pos + sizeof(uint64_t)], recordBytes);
  }
  for (auto slot = 0ul; slot < nLocal; ++slot) {
    if (!filled[slot]) {
      badIndex(firstGid + slot, "is missing");
      break;
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
}

std::vector<char> CPointDirectory::Fetch(const std::vector<uint64_t>& gids) const {
  const int size = SU2_MPI::GetSize();
  std::vector<uint64_t> unique = gids;
  std::sort(unique.begin(), unique.end());
  unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
  if (!unique.empty() && unique.back() >= nGlobal) {
    SU2_MPI::Error("Fetch of global index " + std::to_string(unique.back()) + " of " + std::to_string(nGlobal) + ".",
                   CURRENT_FUNCTION);
  }

  /*--- Requests (sorted, so grouped by owner), answered in the same order. ---*/
  std::vector<size_t> requestBytes(size, 0);
  for (const auto gid : unique) requestBytes[Owner(gid)] += sizeof(uint64_t);
  std::vector<size_t> incomingBytes;
  const auto requests = CPassiveComm::AlltoallvRounds(reinterpret_cast<const char*>(unique.data()), requestBytes,
                                                      incomingBytes);
  const auto nRequest = requests.size() / sizeof(uint64_t);
  std::vector<char> answer(nRequest * recordBytes);
  for (auto i = 0ul; i < nRequest; ++i) {
    uint64_t gid = 0;
    std::memcpy(&gid, &requests[i * sizeof(uint64_t)], sizeof(uint64_t));
    if (gid < firstGid || gid - firstGid >= block.size() / std::max<size_t>(recordBytes, 1)) {
      if (recordBytes > 0) SU2_MPI::Error("Directory request for a point of another block.", CURRENT_FUNCTION);
    }
    if (recordBytes > 0) std::memcpy(&answer[i * recordBytes], &block[(gid - firstGid) * recordBytes], recordBytes);
  }
  std::vector<size_t> answerBytes(size), replyBytes;
  for (int q = 0; q < size; ++q) answerBytes[q] = incomingBytes[q] / sizeof(uint64_t) * recordBytes;
  const auto replies = CPassiveComm::AlltoallvRounds(answer.data(), answerBytes, replyBytes);

  std::vector<char> result(gids.size() * recordBytes);
  for (auto i = 0ul; i < gids.size() && recordBytes > 0; ++i) {
    const auto pos = std::lower_bound(unique.begin(), unique.end(), gids[i]) - unique.begin();
    std::memcpy(&result[i * recordBytes], &replies[pos * recordBytes], recordBytes);
  }
  return result;
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- CAccurateSumBatch                                                                                            ---*/
/*------------------------------------------------------------------------------------------------------------------*/

size_t CAccurateSumBatch::Add(const double* x, size_t n, size_t stride) {
  double triple[3];
  size_t bad = n;
  CAccurateSum::Local(x, n, stride, triple, &bad);
  triples.insert(triples.end(), triple, triple + 3);
  firstBad.push_back(bad);
  derivativeOf.push_back(-1);
  return firstBad.size() - 1;
}

size_t CAccurateSumBatch::Add(const float* x, size_t n, size_t stride) {
  double triple[3];
  size_t bad = n;
  CAccurateSum::Local(x, n, stride, triple, &bad);
  triples.insert(triples.end(), triple, triple + 3);
  firstBad.push_back(bad);
  derivativeOf.push_back(-1);
  return firstBad.size() - 1;
}

size_t CAccurateSumBatch::AddActive(const su2double* x, size_t n, size_t stride) {
  std::vector<double> values(n);
  for (auto i = 0ul; i < n; ++i) values[i] = SU2_TYPE::GetValue(x[i * stride]);
  const auto q = Add(values);
#ifdef CODI_FORWARD_TYPE
  for (auto i = 0ul; i < n; ++i) values[i] = SU2_TYPE::GetDerivative(x[i * stride]);
  const auto qd = Add(values);
  derivativeOf[q] = static_cast<long>(qd);
#endif
  return q;
}

void CAccurateSumBatch::Reduce() {
  const auto nQuantity = firstBad.size();
  std::vector<size_t> counts;
  const auto all = CPassiveComm::Allgatherv(triples, &counts);
  for (const auto count : counts) {
    if (count != 3 * nQuantity) SU2_MPI::Error("Accurate sums: different quantities on the ranks.", CURRENT_FUNCTION);
  }
  const auto nRank = counts.size();
  results.assign(nQuantity, 0.0);
  absSums.assign(nQuantity, 0.0);
  finite.assign(nQuantity, false);
  std::vector<double> rankTriples(3 * nRank);
  for (auto q = 0ul; q < nQuantity; ++q) {
    for (auto r = 0ul; r < nRank; ++r)
      for (int k = 0; k < 3; ++k) rankTriples[3 * r + k] = all[r * 3 * nQuantity + 3 * q + k];
    double result = 0.0, absSum = 0.0;
    finite[q] = CAccurateSum::Merge(rankTriples.data(), nRank, &result, &absSum);
    results[q] = result;
    absSums[q] = absSum;
  }
}

void CAccurateSumBatch::ReduceLocal() {
  const auto nQuantity = firstBad.size();
  results.assign(nQuantity, 0.0);
  absSums.assign(nQuantity, 0.0);
  finite.assign(nQuantity, false);
  for (auto q = 0ul; q < nQuantity; ++q) {
    double result = 0.0, absSum = 0.0;
    finite[q] = CAccurateSum::Merge(&triples[3 * q], 1, &result, &absSum);
    results[q] = result;
    absSums[q] = absSum;
  }
}

su2double CAccurateSumBatch::GetActive(size_t q) const {
  su2double value = results[q];
#ifdef CODI_FORWARD_TYPE
  if (derivativeOf[q] >= 0) SU2_TYPE::SetDerivative(value, results[derivativeOf[q]]);
#endif
  return value;
}

bool CAccurateSumBatch::FiniteActive(size_t q) const {
  bool ok = finite[q];
  if (derivativeOf[q] >= 0) ok = ok && finite[derivativeOf[q]];
  return ok;
}
