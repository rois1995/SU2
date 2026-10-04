/*!
 * \file CDistributedLocator.cpp
 * \brief Canonical point location of the solution transfers (canonical boundary, distributed location).
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

#include "../../include/adaptation/CDistributedLocator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/TransferTolerances.hpp"
#include "../../../Common/include/geometry/CGeometry.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"

#if defined(__GNUC__)
#define SU2_TRANSFER_NOINLINE __attribute__((noinline))
#else
#define SU2_TRANSFER_NOINLINE
#endif

namespace {

/*--- Weights of the closest point of the segment [a,b] to p. ---*/
void ClosestPointSegment(unsigned short nDim, const passivedouble* a, const passivedouble* b, const passivedouble* p,
                         passivedouble* weight) {
  passivedouble length2 = 0.0, projection = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    length2 += std::pow(b[iDim] - a[iDim], 2);
    projection += (p[iDim] - a[iDim]) * (b[iDim] - a[iDim]);
  }
  passivedouble t = (length2 > 0.0) ? projection / length2 : passivedouble(0.0);
  t = std::min(std::max(t, passivedouble(0.0)), passivedouble(1.0));
  weight[0] = 1.0 - t;
  weight[1] = t;
}

/*--- Weights of the closest point of the triangle (a,b,c) to p in 3D, by the Voronoi regions of the triangle
 *    (Ericson, Real-Time Collision Detection, 5.1.5). The weights are in [0,1]. ---*/
void ClosestPointTriangle(const passivedouble* a, const passivedouble* b, const passivedouble* c,
                          const passivedouble* p, passivedouble* weight) {
  auto dot = [](const passivedouble* u, const passivedouble* v) { return u[0] * v[0] + u[1] * v[1] + u[2] * v[2]; };
  passivedouble ab[3], ac[3], ap[3], bp[3], cp[3];
  for (int i = 0; i < 3; ++i) {
    ab[i] = b[i] - a[i];
    ac[i] = c[i] - a[i];
    ap[i] = p[i] - a[i];
    bp[i] = p[i] - b[i];
    cp[i] = p[i] - c[i];
  }
  auto set = [weight](passivedouble wa, passivedouble wb, passivedouble wc) {
    weight[0] = wa;
    weight[1] = wb;
    weight[2] = wc;
  };

  const passivedouble d1 = dot(ab, ap), d2 = dot(ac, ap);
  if (d1 <= 0.0 && d2 <= 0.0) return set(1.0, 0.0, 0.0);

  const passivedouble d3 = dot(ab, bp), d4 = dot(ac, bp);
  if (d3 >= 0.0 && d4 <= d3) return set(0.0, 1.0, 0.0);

  const passivedouble vc = d1 * d4 - d3 * d2;
  if (vc <= 0.0 && d1 >= 0.0 && d3 <= 0.0) {
    const passivedouble v = d1 / (d1 - d3);
    return set(1.0 - v, v, 0.0);
  }

  const passivedouble d5 = dot(ab, cp), d6 = dot(ac, cp);
  if (d6 >= 0.0 && d5 <= d6) return set(0.0, 0.0, 1.0);

  const passivedouble vb = d5 * d2 - d1 * d6;
  if (vb <= 0.0 && d2 >= 0.0 && d6 <= 0.0) {
    const passivedouble w = d2 / (d2 - d6);
    return set(1.0 - w, 0.0, w);
  }

  const passivedouble va = d3 * d6 - d5 * d4;
  if (va <= 0.0 && (d4 - d3) >= 0.0 && (d5 - d6) >= 0.0) {
    const passivedouble w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
    return set(0.0, 1.0 - w, w);
  }

  /*--- Inside the face region: weights proportional to va, vb, vc (positive up to round-off). A degenerate (zero-area)
   *    triangle has no such region: closest point of its edges. ---*/
  const passivedouble wa = std::max(passivedouble(0.0), va), wb = std::max(passivedouble(0.0), vb),
                      wc = std::max(passivedouble(0.0), vc);
  const passivedouble sum = wa + wb + wc;
  if (!(sum > 0.0)) {
    const passivedouble* xNode[] = {a, b, c};
    passivedouble best = std::numeric_limits<passivedouble>::max();
    for (int i = 0; i < 3; ++i) {
      const int j = (i + 1) % 3;
      passivedouble w2[2] = {}, dist2 = 0.0;
      ClosestPointSegment(3, xNode[i], xNode[j], p, w2);
      for (int k = 0; k < 3; ++k) dist2 += std::pow(p[k] - w2[0] * xNode[i][k] - w2[1] * xNode[j][k], 2);
      if (dist2 < best) {
        best = dist2;
        weight[0] = weight[1] = weight[2] = 0.0;
        weight[i] = w2[0];
        weight[j] = w2[1];
      }
    }
    return;
  }
  set(wa / sum, wb / sum, wc / sum);
}

}  // namespace

SU2_TRANSFER_NOINLINE void CanonicalClosestPoint(unsigned short nDim, const passivedouble* const* nodes,
                                                 const passivedouble* x, passivedouble* weight,
                                                 passivedouble* distance, passivedouble* faceSize) {
  if (nDim == 2) {
    ClosestPointSegment(nDim, nodes[0], nodes[1], x, weight);
  } else {
    ClosestPointTriangle(nodes[0], nodes[1], nodes[2], x, weight);
  }
  passivedouble closest[3] = {}, dist2 = 0.0;
  for (unsigned short iNode = 0; iNode < nDim; ++iNode)
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) closest[iDim] += weight[iNode] * nodes[iNode][iDim];
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) dist2 += std::pow(x[iDim] - closest[iDim], 2);
  *distance = std::sqrt(dist2);

  passivedouble size = 0.0;
  for (unsigned short iNode = 0; iNode < nDim; ++iNode) {
    for (unsigned short jNode = iNode + 1; jNode < nDim; ++jNode) {
      passivedouble length2 = 0.0;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) length2 += std::pow(nodes[iNode][iDim] - nodes[jNode][iDim], 2);
      size = std::max(size, std::sqrt(length2));
    }
  }
  *faceSize = size;
}

bool BetterFace(const CFaceHit& a, const CFaceHit& b) {
  if (!b.found) return a.found;
  if (!a.found) return false;
  /*--- A nonfinite distance is the worst (a strict order also then). ---*/
  if (std::isnan(a.distance) || std::isnan(b.distance)) {
    if (std::isnan(a.distance) != std::isnan(b.distance)) return std::isnan(b.distance);
    if (a.marker != b.marker) return a.marker < b.marker;
    return a.key < b.key;
  }
  if (a.distance != b.distance) return a.distance < b.distance;
  if (a.marker != b.marker) return a.marker < b.marker;
  return a.key < b.key;
}

bool BetterElement(const CElementHit& a, const CElementHit& b) {
  if (!b.found) return a.found;
  if (!a.found) return false;
  if (std::isnan(a.minWeight) || std::isnan(b.minWeight)) {
    if (std::isnan(a.minWeight) != std::isnan(b.minWeight)) return std::isnan(b.minWeight);
    return a.key < b.key;
  }
  if (a.minWeight != b.minWeight) return a.minWeight > b.minWeight;
  return a.key < b.key;
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- CBoundaryFaces, CCanonicalBoundary                                                                            ---*/
/*------------------------------------------------------------------------------------------------------------------*/

CBoundaryFaces CBoundaryFaces::Gather(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                                      const CConfig& config) {
  const auto nDim = geometry.GetnDim();
  const auto owned = OwnedBoundaryFaces(geometry, markerTags, config);

  /*--- Faces: u32 marker id, nDim u64 global indices (node order of the geometry). ---*/
  std::vector<char> faceBytes;
  for (auto iFace = 0ul; iFace < owned.size(); ++iFace) {
    PutBytes(faceBytes, owned.markerId[iFace]);
    for (unsigned short k = 0; k < nDim; ++k)
      PutBytes(faceBytes, static_cast<uint64_t>(geometry.nodes->GetGlobalIndex(owned.nodes[iFace * nDim + k])));
  }
  /*--- Nodes: the domain points on a physical boundary (sent by their owner, which holds all their faces). ---*/
  const auto pointMarkers = PointMarkerIds(geometry, markerTags, config);
  std::vector<char> nodeBytes;
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    if (pointMarkers[iPoint].empty()) continue;
    PutBytes(nodeBytes, static_cast<uint64_t>(geometry.nodes->GetGlobalIndex(iPoint)));
    for (unsigned short iDim = 0; iDim < nDim; ++iDim)
      PutBytes(nodeBytes, static_cast<double>(SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim))));
  }
  const auto allFaces = CPassiveComm::AllgathervRounds(faceBytes.data(), faceBytes.size(), nullptr);
  const auto allNodes = CPassiveComm::AllgathervRounds(nodeBytes.data(), nodeBytes.size(), nullptr);

  CBoundaryFaces faces;
  faces.nDim = nDim;
  const size_t faceRecord = sizeof(uint32_t) + nDim * sizeof(uint64_t);
  const size_t nodeRecord = sizeof(uint64_t) + nDim * sizeof(double);
  const auto nFace = allFaces.size() / faceRecord, nNode = allNodes.size() / nodeRecord;
  faces.marker.resize(nFace);
  faces.faceGid.resize(nFace * nDim);
  const char* src = allFaces.data();
  for (auto iFace = 0ul; iFace < nFace; ++iFace) {
    faces.marker[iFace] = GetBytes<uint32_t>(src);
    for (unsigned short k = 0; k < nDim; ++k) faces.faceGid[iFace * nDim + k] = GetBytes<uint64_t>(src);
  }
  faces.nodeGid.resize(nNode);
  faces.nodeCoord.resize(nNode * nDim);
  src = allNodes.data();
  for (auto iNode = 0ul; iNode < nNode; ++iNode) {
    faces.nodeGid[iNode] = GetBytes<uint64_t>(src);
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) faces.nodeCoord[iNode * nDim + iDim] = GetBytes<double>(src);
  }
  return faces;
}

CCanonicalBoundary::CCanonicalBoundary(CBoundaryFaces facesIn, passivedouble domainSize)
    : nDim(facesIn.nDim), domainSize(domainSize) {
  if (nDim != 2 && nDim != 3) SU2_MPI::Error("The canonical boundary needs a 2D or 3D mesh.", CURRENT_FUNCTION);
  const auto nFace = facesIn.GetnFace();

  /*--- Faces sorted by (marker, key): the order of the MPI-1 gather; nodes sorted by global index. ---*/
  std::vector<CSimplexKey> keysIn(nFace);
  for (auto iFace = 0ul; iFace < nFace; ++iFace) keysIn[iFace] = MakeSimplexKey(&facesIn.faceGid[iFace * nDim], nDim);
  std::vector<unsigned long> order(nFace);
  std::iota(order.begin(), order.end(), 0ul);
  std::sort(order.begin(), order.end(), [&](unsigned long a, unsigned long b) {
    if (facesIn.marker[a] != facesIn.marker[b]) return facesIn.marker[a] < facesIn.marker[b];
    return keysIn[a] < keysIn[b];
  });
  faces.nDim = nDim;
  faces.marker.resize(nFace);
  faces.faceGid.resize(nFace * nDim);
  keys.resize(nFace);
  for (auto i = 0ul; i < nFace; ++i) {
    faces.marker[i] = facesIn.marker[order[i]];
    keys[i] = keysIn[order[i]];
    std::copy_n(&facesIn.faceGid[order[i] * nDim], nDim, &faces.faceGid[i * nDim]);
  }
  const auto nNode = facesIn.nodeGid.size();
  std::vector<unsigned long> nodeOrder(nNode);
  std::iota(nodeOrder.begin(), nodeOrder.end(), 0ul);
  std::sort(nodeOrder.begin(), nodeOrder.end(),
            [&](unsigned long a, unsigned long b) { return facesIn.nodeGid[a] < facesIn.nodeGid[b]; });
  faces.nodeGid.resize(nNode);
  faces.nodeCoord.resize(nNode * nDim);
  for (auto i = 0ul; i < nNode; ++i) {
    faces.nodeGid[i] = facesIn.nodeGid[nodeOrder[i]];
    std::copy_n(&facesIn.nodeCoord[nodeOrder[i] * nDim], nDim, &faces.nodeCoord[i * nDim]);
  }
  faceNode.resize(nFace * nDim);
  for (auto i = 0ul; i < nFace * nDim; ++i) {
    const auto it = std::lower_bound(faces.nodeGid.begin(), faces.nodeGid.end(), faces.faceGid[i]);
    if (it == faces.nodeGid.end() || *it != faces.faceGid[i]) {
      SU2_MPI::Error("The coordinates of boundary point " + std::to_string(faces.faceGid[i]) + " are missing.",
                     CURRENT_FUNCTION);
    }
    faceNode[i] = it - faces.nodeGid.begin();
  }

  /*--- One ADT per marker over its faces (compact node numbering). ---*/
  const unsigned short faceType = (nDim == 2) ? LINE : TRIANGLE;
  for (auto first = 0ul; first < nFace;) {
    auto last = first;
    while (last < nFace && faces.marker[last] == faces.marker[first]) ++last;
    markerIds.push_back(faces.marker[first]);
    markerFirst.push_back(first);

    std::vector<unsigned long> nodesUsed(faceNode.begin() + first * nDim, faceNode.begin() + last * nDim);
    std::sort(nodesUsed.begin(), nodesUsed.end());
    nodesUsed.erase(std::unique(nodesUsed.begin(), nodesUsed.end()), nodesUsed.end());
    std::vector<su2double> coord(nodesUsed.size() * nDim);
    for (auto i = 0ul; i < nodesUsed.size(); ++i)
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) coord[i * nDim + iDim] = faces.nodeCoord[nodesUsed[i] * nDim + iDim];
    std::vector<unsigned long> conn((last - first) * nDim);
    for (auto i = 0ul; i < conn.size(); ++i)
      conn[i] = std::lower_bound(nodesUsed.begin(), nodesUsed.end(), faceNode[first * nDim + i]) - nodesUsed.begin();
    const auto n = last - first;
    std::vector<unsigned short> types(n, faceType), markers(n, 0);
    std::vector<unsigned long> ids(n);
    std::iota(ids.begin(), ids.end(), 0ul);
    markerADT.emplace_back(new CADTElemClass(nDim, coord, conn, types, markers, ids, false));
    first = last;
  }
  markerFirst.push_back(nFace);
}

bool CCanonicalBoundary::HasMarker(uint32_t id) const {
  return std::binary_search(markerIds.begin(), markerIds.end(), id);
}

size_t CCanonicalBoundary::GetMemory() const {
  const size_t nFace = faces.GetnFace(), nNode = faces.nodeGid.size();
  return nFace * (sizeof(uint32_t) + nDim * (2 * sizeof(uint64_t) + sizeof(unsigned long)) + sizeof(CSimplexKey) +
                  2 * nDim * sizeof(double) * 2 + 64) +
         nNode * (sizeof(uint64_t) + nDim * sizeof(double) * 2);
}

CFaceHit CCanonicalBoundary::Evaluate(unsigned long face, const passivedouble* x) const {
  CFaceHit hit;
  const passivedouble* nodes[3] = {};
  for (unsigned short k = 0; k < nDim; ++k) nodes[k] = &faces.nodeCoord[faceNode[face * nDim + k] * nDim];
  CanonicalClosestPoint(nDim, nodes, x, hit.weight, &hit.distance, &hit.faceSize);
  hit.found = true;
  hit.marker = faces.marker[face];
  hit.key = keys[face];
  hit.face = face;
  return hit;
}

CFaceHit CCanonicalBoundary::Nearest(const passivedouble* x, const std::vector<uint32_t>& markers) {
  std::vector<unsigned long> slots;
  for (const auto id : markers) {
    const auto it = std::lower_bound(markerIds.begin(), markerIds.end(), id);
    if (it != markerIds.end() && *it == id) slots.push_back(it - markerIds.begin());
  }
  std::sort(slots.begin(), slots.end());
  slots.erase(std::unique(slots.begin(), slots.end()), slots.end());
  CFaceHit best;
  if (slots.empty()) return best;

  su2double xs[3] = {0.0, 0.0, 0.0};
  passivedouble normInf = 0.0;
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
    xs[iDim] = x[iDim];
    normInf = std::max(normInf, std::fabs(x[iDim]));
  }

  /*--- U: canonical distance of the ADT's nearest face of each marker (any face is acceptable here; its canonical
   *    distance is achievable), the smallest. ---*/
  passivedouble U = std::numeric_limits<passivedouble>::infinity();
  for (const auto slot : slots) {
    su2double dist = 0.0;
    unsigned short markerID = 0;
    unsigned long id = 0;
    int rankID = 0;
    markerADT[slot]->DetermineNearestElement(xs, dist, markerID, id, rankID);
    U = std::min(U, Evaluate(markerFirst[slot] + id, x).distance);
  }

  /*--- Radius with the margin of the proof (scales with the distance, the coordinates and the domain), then every
   *    face within it competes by the total key. ---*/
  const passivedouble radius = U + DecisionThreshold(1e-12) * (U + normInf + domainSize);
  std::vector<unsigned long> ids;
  for (const auto slot : slots) {
    markerADT[slot]->DetermineElementsWithinDistance(xs, radius, ids);
    for (const auto id : ids) {
      const auto hit = Evaluate(markerFirst[slot] + id, x);
      if (BetterFace(hit, best)) best = hit;
    }
  }
  if (!best.found) SU2_MPI::Error("The canonical nearest-face search found no candidate.", CURRENT_FUNCTION);
  return best;
}

/*------------------------------------------------------------------------------------------------------------------*/
/*--- CDistributedLocator                                                                                           ---*/
/*------------------------------------------------------------------------------------------------------------------*/

CDistributedLocator::CDistributedLocator(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                                         const CConfig& config, passivedouble absoluteLimit)
    : nDim(geometry.GetnDim()), absoluteLimit(absoluteLimit), domainSize(0.0) {
  owned = OwnedSimplices(geometry);
  const unsigned short nNode = nDim + 1;
  const auto nOwned = owned.size();

  /*--- ADT of the owned elements (compact numbering of their nodes). ---*/
  std::vector<long> compact(geometry.GetnPoint(), -1);
  std::vector<unsigned long> conn(nOwned * nNode);
  ownedGid.resize(nOwned * nNode);
  for (auto i = 0ul; i < nOwned * nNode; ++i) {
    const auto iPoint = owned.nodes[i];
    if (compact[iPoint] < 0) {
      compact[iPoint] = adtCoord.size() / nDim;
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) adtCoord.push_back(geometry.nodes->GetCoord(iPoint, iDim));
    }
    conn[i] = compact[iPoint];
    ownedGid[i] = geometry.nodes->GetGlobalIndex(iPoint);
  }

  /*--- Routing boxes: union of the ADT's inflated element boxes over groups of elements (recursive bisection). ---*/
  std::vector<double> elemBoxes(nOwned * 2 * nDim), centroids(nOwned * nDim);
  for (auto iElem = 0ul; iElem < nOwned; ++iElem) {
    double* lo = &elemBoxes[iElem * 2 * nDim];
    double* hi = lo + nDim;
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
      lo[iDim] = std::numeric_limits<double>::max();
      hi[iDim] = std::numeric_limits<double>::lowest();
      centroids[iElem * nDim + iDim] = 0.0;
    }
    for (unsigned short k = 0; k < nNode; ++k) {
      const auto c = conn[iElem * nNode + k];
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) {
        const double x = SU2_TYPE::GetValue(adtCoord[c * nDim + iDim]);
        if (k == 0) {
          lo[iDim] = hi[iDim] = x;
        } else {
          lo[iDim] = std::min(lo[iDim], x);
          hi[iDim] = std::max(hi[iDim], x);
        }
        centroids[iElem * nDim + iDim] += x / nNode;
      }
    }
    CADTElemClass::InflateBox(nDim, lo, hi);
  }
  const unsigned long size = SU2_MPI::GetSize();
  unsigned long maxBoxes = (nDim == 2) ? 32 : 64;
  while (maxBoxes > 1 && size * maxBoxes > (1ul << 20)) maxBoxes /= 2;
  routing.Build(nDim, BisectionBoxes(nDim, elemBoxes, centroids, maxBoxes, nullptr));

  if (nOwned > 0) {
    std::vector<unsigned short> types(nOwned, nDim == 2 ? TRIANGLE : TETRAHEDRON), markers(nOwned, 0);
    std::vector<unsigned long> ids(nOwned);
    std::iota(ids.begin(), ids.end(), 0ul);
    auto coordCopy = adtCoord;
    adt = std::make_unique<CADTElemClass>(nDim, coordCopy, conn, types, markers, ids, false);
  }

  double xMin[3], xMax[3], diagonal = 0.0;
  GlobalBoundingBox(geometry, xMin, xMax, &diagonal);
  domainSize = diagonal;
  boundary = std::make_unique<CCanonicalBoundary>(CBoundaryFaces::Gather(geometry, markerTags, config), domainSize);

  /*--- The replicated boundary is held by every rank: above about 64 MB the distributed nearest-face search
   *    (MPI_TRANSFER_PLAN.md M4b, not implemented yet) is needed. ---*/
  if (boundary->GetMemory() > (size_t(64) << 20) && SU2_MPI::GetRank() == MASTER_NODE) {
    cout << "WARNING: the replicated donor boundary of the solution transfer needs " << (boundary->GetMemory() >> 20)
         << " MB on every rank (distributed nearest-face search not available)." << endl;
  }
}

CDistributedLocator::~CDistributedLocator() = default;

passivedouble CDistributedLocator::GetDistanceLimit(passivedouble faceSize) const {
  return std::max(faceSize, std::max(absoluteLimit, passivedouble(1e-3) * domainSize));
}

size_t CDistributedLocator::GetMemory() const {
  const size_t nOwned = owned.size(), nNode = nDim + 1;
  return nOwned * (2 * nDim * sizeof(double) * 2 + nNode * (3 * sizeof(unsigned long)) + sizeof(CSimplexKey) + 64) +
         adtCoord.size() * sizeof(su2double) * 2 + routing.GetnBox() * (2 * nDim * sizeof(double) + 32) +
         boundary->GetMemory();
}

CElementHit CDistributedLocator::LocalBest(const passivedouble* x) {
  CElementHit best;
  if (!adt) return best;
  const unsigned short nNode = nDim + 1;
  su2double xs[3] = {0.0, 0.0, 0.0};
  for (unsigned short iDim = 0; iDim < nDim; ++iDim) xs[iDim] = x[iDim];
  std::vector<unsigned long> ids;
  std::vector<su2double> weights;
  adt->DetermineContainingElements(xs, ids, weights);
  for (auto i = 0ul; i < ids.size(); ++i) {
    CElementHit hit;
    hit.found = true;
    hit.minWeight = std::numeric_limits<passivedouble>::max();
    for (unsigned short k = 0; k < nNode; ++k) {
      hit.weight[k] = SU2_TYPE::GetValue(weights[8 * i + k]);
      hit.minWeight = std::min(hit.minWeight, hit.weight[k]);
      hit.gid[k] = ownedGid[ids[i] * nNode + k];
    }
    hit.key = owned.keys[ids[i]];
    if (BetterElement(hit, best)) best = hit;
  }
  return best;
}

std::vector<CElementHit> CDistributedLocator::LocateElements(const std::vector<passivedouble>& coord) {
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  const unsigned short nNode = nDim + 1;
  const auto nPoint = coord.size() / nDim;
  std::vector<CElementHit> result(nPoint);

  /*--- Destinations: the ranks whose routing boxes contain the point. ---*/
  std::vector<std::vector<unsigned long>> lists(size);
  std::vector<int> ranks;
  for (auto i = 0ul; i < nPoint; ++i) {
    double x[3] = {0.0, 0.0, 0.0};
    for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = coord[i * nDim + iDim];
    routing.RanksContaining(x, ranks);
    for (const auto r : ranks) lists[r].push_back(i);
  }

  /*--- Plan: total counts of every pair, then the number of chunks so that every rank's outgoing and incoming
   *    queries and replies of one chunk fit the part of the ceiling left after the resident structures. ---*/
  const size_t queryBytes = sizeof(uint64_t) + nDim * sizeof(double);
  const size_t replyBytes = sizeof(uint64_t) * (1 + nNode) + sizeof(double) * (nNode + 1);
  std::vector<uint64_t> outCount(size), inCount(size);
  for (int r = 0; r < size; ++r) outCount[r] = lists[r].size();
  CPassiveComm::Alltoall(outCount.data(), inCount.data(), sizeof(uint64_t));
  size_t nOut = 0, nIn = 0;
  for (int r = 0; r < size; ++r) {
    nOut += outCount[r];
    nIn += inCount[r];
  }
  const size_t ceiling = GetTransferMemoryCeiling();
  const size_t resident = GetMemory() + nPoint * sizeof(CElementHit);
  CLocalFailure failure;
  size_t available = 0;
  if (resident >= ceiling) {
    failure.Set(1, rank,
                "The distributed point location would need " + std::to_string(resident) +
                    " bytes for its resident structures on rank " + std::to_string(rank) +
                    ", more than the memory ceiling of the transfer (" + std::to_string(ceiling) + " bytes).");
  } else {
    available = ceiling - resident;
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  auto chunksFor = [available](size_t bytes) { return bytes == 0 ? 0ul : (bytes - 1) / available + 1; };
  const unsigned long need = std::max({chunksFor(nOut * queryBytes), chunksFor(nIn * queryBytes),
                                       chunksFor(nIn * replyBytes), chunksFor(nOut * replyBytes), 1ul});
  const unsigned long nChunk = CPassiveComm::AllreduceMax(need);
  lastChunks = nChunk;
  lastSent = nOut;
  lastReceived = nIn;
  lastMaxChunkReceived = 0;

  for (unsigned long q = 0; q < nChunk; ++q) {
    /*--- The q-th part of every destination's list. ---*/
    std::vector<size_t> sendBytes(size, 0), first(size), last(size);
    for (int r = 0; r < size; ++r) {
      const auto n = lists[r].size();
      first[r] = n * q / nChunk;
      last[r] = n * (q + 1) / nChunk;
      sendBytes[r] = (last[r] - first[r]) * queryBytes;
    }
    std::vector<char> send;
    send.reserve(std::accumulate(sendBytes.begin(), sendBytes.end(), size_t(0)));
    for (int r = 0; r < size; ++r) {
      for (auto j = first[r]; j < last[r]; ++j) {
        const auto i = lists[r][j];
        PutBytes(send, static_cast<uint64_t>(i));
        for (unsigned short iDim = 0; iDim < nDim; ++iDim) PutBytes(send, static_cast<double>(coord[i * nDim + iDim]));
      }
    }
    std::vector<size_t> recvBytes;
    auto queries = CPassiveComm::AlltoallvRounds(send.data(), sendBytes, recvBytes);
    send = std::vector<char>();

    /*--- Collect-all location in the owned elements; padded reply per query. ---*/
    const auto nQuery = queries.size() / queryBytes;
    lastMaxChunkReceived = std::max<unsigned long>(lastMaxChunkReceived, nQuery);
    std::vector<char> replies;
    replies.reserve(nQuery * replyBytes);
    const char* src = queries.data();
    for (auto k = 0ul; k < nQuery; ++k) {
      GetBytes<uint64_t>(src);
      passivedouble x[3] = {0.0, 0.0, 0.0};
      for (unsigned short iDim = 0; iDim < nDim; ++iDim) x[iDim] = GetBytes<double>(src);
      const auto hit = LocalBest(x);
      PutBytes(replies, static_cast<uint64_t>(hit.found ? 1 : 0));
      for (unsigned short k2 = 0; k2 < nNode; ++k2) PutBytes(replies, hit.gid[k2]);
      PutBytes(replies, static_cast<double>(hit.minWeight));
      for (unsigned short k2 = 0; k2 < nNode; ++k2) PutBytes(replies, static_cast<double>(hit.weight[k2]));
    }
    queries = std::vector<char>();
    std::vector<size_t> replySend(size), replyRecv;
    for (int r = 0; r < size; ++r) replySend[r] = recvBytes[r] / queryBytes * replyBytes;
    const auto answers = CPassiveComm::AlltoallvRounds(replies.data(), replySend, replyRecv);
    replies = std::vector<char>();

    /*--- Merge: the replies of rank r answer the queries sent to it, in order. ---*/
    src = answers.data();
    for (int r = 0; r < size; ++r) {
      for (auto j = first[r]; j < last[r]; ++j) {
        CElementHit hit;
        hit.found = GetBytes<uint64_t>(src) != 0;
        for (unsigned short k = 0; k < nNode; ++k) hit.gid[k] = GetBytes<uint64_t>(src);
        hit.minWeight = GetBytes<double>(src);
        for (unsigned short k = 0; k < nNode; ++k) hit.weight[k] = GetBytes<double>(src);
        if (!hit.found) continue;
        hit.key = MakeSimplexKey(hit.gid, nNode);
        auto& best = result[lists[r][j]];
        if (BetterElement(hit, best)) best = hit;
      }
    }
  }
  return result;
}

CDistributedLocator::Stencil CDistributedLocator::FaceStencil(const CFaceHit& hit) const {
  if (!hit.found) SU2_MPI::Error("The mesh has no boundary faces.", CURRENT_FUNCTION);
  Stencil stencil;
  stencil.inside = false;
  stencil.onFace = true;
  stencil.nPoint = nDim;
  for (unsigned short k = 0; k < nDim; ++k) {
    stencil.gid[k] = boundary->FaceNodeGid(hit.face, k);
    stencil.weight[k] = hit.weight[k];
  }
  stencil.distance = hit.distance;
  stencil.faceSize = hit.faceSize;
  stencil.beyondLimit = !(stencil.distance <= GetDistanceLimit(stencil.faceSize));
  stencil.marker = hit.marker;
  stencil.key = hit.key;
  return stencil;
}

CDistributedLocator::Stencil CDistributedLocator::ElementStencil(const CElementHit& hit, unsigned short nDim) {
  Stencil stencil;
  stencil.nPoint = nDim + 1;
  passivedouble sum = 0.0;
  for (unsigned short k = 0; k <= nDim; ++k) {
    stencil.gid[k] = hit.gid[k];
    stencil.weight[k] = std::max(passivedouble(0.0), hit.weight[k]);
    sum += stencil.weight[k];
  }
  for (unsigned short k = 0; k <= nDim; ++k) stencil.weight[k] /= sum;
  stencil.key = hit.key;
  return stencil;
}

std::vector<CDistributedLocator::Stencil> CDistributedLocator::Locate(
    const std::vector<passivedouble>& coord, const std::vector<std::vector<uint32_t>>& markers) {
  const auto hits = LocateElements(coord);
  const auto nPoint = hits.size();
  std::vector<Stencil> stencils(nPoint);
  std::vector<uint32_t> known;
  for (auto i = 0ul; i < nPoint; ++i) {
    const auto* x = &coord[i * nDim];
    known.clear();
    for (const auto id : markers[i])
      if (boundary->HasMarker(id)) known.push_back(id);
    if (!known.empty()) {
      stencils[i] = FaceStencil(boundary->Nearest(x, known));
      stencils[i].inside = hits[i].found;
    } else if (hits[i].found) {
      stencils[i] = ElementStencil(hits[i], nDim);
    } else {
      stencils[i] = FaceStencil(boundary->NearestAny(x));
    }
  }
  return stencils;
}
