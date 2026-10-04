/*!
 * \file CDistributedLocator.hpp
 * \brief Canonical point location of the solution transfers: canonical closest point of boundary faces, canonical
 *        nearest-face search (replicated boundary), and the distributed location of points in a partitioned mesh.
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

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../../../Common/include/adaptation/CDistributedSearch.hpp"
#include "../../../Common/include/adaptation/CTransferMemory.hpp"
#include "../../../Common/include/adt/CADTElemClass.hpp"

class CConfig;
class CGeometry;

/*!
 * \brief Canonical closest point of a boundary face (segment in 2D, triangle in 3D, Ericson's Voronoi regions): the
 *        weights in the face's node order (in [0,1]), the distance d = |x - sum w_k y_k| and the face size (longest
 *        edge). One compiled function (not inline) used by every mode of the transfers and by the serial locator, so
 *        the same face and point give the same bits everywhere.
 * \param[in] nodes - Coordinates of the nDim nodes of the face, in its node order.
 */
void CanonicalClosestPoint(unsigned short nDim, const passivedouble* const* nodes, const passivedouble* x,
                           passivedouble* weight, passivedouble* distance, passivedouble* faceSize);

/*!
 * \brief Result of a canonical nearest-face search.
 */
struct CFaceHit {
  bool found = false;
  passivedouble distance = 0.0;  /*!< \brief Canonical distance. */
  uint32_t marker = 0;           /*!< \brief Marker id (config position; serial locator: mesh marker position). */
  CSimplexKey key = {};          /*!< \brief Face key (sorted global indices). */
  unsigned long face = 0;        /*!< \brief Index of the face in the boundary. */
  passivedouble weight[3] = {};  /*!< \brief Weights of the closest point in the face's node order. */
  passivedouble faceSize = 0.0;  /*!< \brief Longest edge of the face. */
};

/*! \brief The total order of the canonical search: (distance, marker, face key). */
bool BetterFace(const CFaceHit& a, const CFaceHit& b);

/*!
 * \brief Boundary faces given as arrays (all ranks' faces in the replicated mode, or a serial mesh).
 */
struct CBoundaryFaces {
  unsigned short nDim = 0;
  std::vector<uint32_t> marker;        /*!< \brief Marker id of each face. */
  std::vector<uint64_t> faceGid;       /*!< \brief Global indices of the face nodes, in the face's node order. */
  std::vector<uint64_t> nodeGid;       /*!< \brief Global index of each node (any order, unique). */
  std::vector<passivedouble> nodeCoord; /*!< \brief Coordinates of each node. */

  unsigned long GetnFace() const { return marker.size(); }

  /*!
   * \brief Collective: the owned boundary faces of the physical markers of all ranks (COwnedFaces: marker id = config
   *        position, unnamed one past the end) with the coordinates of their nodes (sent by the owners of the points).
   */
  static CBoundaryFaces Gather(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                               const CConfig& config);
};

/*!
 * \class CCanonicalBoundary
 * \brief Canonical nearest-face search on a set of boundary faces held completely by this rank (replicated mode of
 *        MPI_TRANSFER_PLAN.md 3.4, and the serial locator).
 * \note For a point x and a set of markers: U = canonical distance of the ADT's initial face of each marker (the
 *       smallest), R = U + 1e-12 (U + max|x_k| + L) (L: diagonal of the donor box), candidates = every face whose
 *       inflated box has a possible distance <= R (ADT range query), winner = smallest (distance, marker, face key) of
 *       the canonical kernel over the candidates. The winner is the global minimum of that key over the faces of the
 *       markers (proof in the plan, 3.4), independent of the ADT's arithmetic and of the order of the faces.
 */
class CCanonicalBoundary {
 public:
  /*!
   * \param[in] faces - The faces (sorted here by (marker, key)).
   * \param[in] domainSize - L, the diagonal of the bounding box of the mesh (global).
   */
  CCanonicalBoundary(CBoundaryFaces faces, passivedouble domainSize);

  unsigned short GetnDim() const { return nDim; }
  /*! \brief Marker ids with faces, sorted. */
  const std::vector<uint32_t>& GetMarkers() const { return markerIds; }
  bool HasMarker(uint32_t id) const;
  unsigned long GetnFace() const { return faces.GetnFace(); }

  /*!
   * \brief Canonical nearest face over the faces of the given markers (ids without faces are ignored; none known:
   *        CFaceHit::found false).
   */
  CFaceHit Nearest(const passivedouble* x, const std::vector<uint32_t>& markers);

  /*! \brief Canonical nearest face over every marker. */
  CFaceHit NearestAny(const passivedouble* x) { return Nearest(x, markerIds); }

  /*! \brief Global index of node k of a face. */
  uint64_t FaceNodeGid(unsigned long face, unsigned short k) const { return faces.faceGid[face * nDim + k]; }

  /*! \brief Heap bytes held by this rank (all arrays at their capacity, the ADTs included). */
  size_t GetMemory() const;

  /*! \brief Bound of the transient (and workspace growth) bytes of one Nearest query. */
  size_t QueryBytes() const;

 private:
  unsigned short nDim;
  passivedouble domainSize;
  CBoundaryFaces faces;                 /*!< \brief Sorted by (marker, key). */
  std::vector<CSimplexKey> keys;
  std::vector<unsigned long> faceNode;  /*!< \brief Node index (in faces.node*) of each face node. */
  std::vector<uint32_t> markerIds;      /*!< \brief Markers with faces. */
  std::vector<unsigned long> markerFirst; /*!< \brief First face of each marker (+ end). */
  std::vector<std::unique_ptr<CADTElemClass>> markerADT;

  CFaceHit Evaluate(unsigned long face, const passivedouble* x) const;
};

/*!
 * \brief Canonical containing element (MPI_TRANSFER_PLAN.md 3.5, 4.4): of every element that the ADT accepts, the one
 *        with the largest minimum raw barycentric weight, ties (bitwise equal minima) by the smallest element key.
 */
struct CElementHit {
  bool found = false;
  passivedouble minWeight = 0.0;
  CSimplexKey key = {};
  uint64_t gid[4] = {};          /*!< \brief Global indices of the nodes in the element's node order. */
  passivedouble weight[4] = {};  /*!< \brief Raw weights (ADT) in that order. */
};
bool BetterElement(const CElementHit& a, const CElementHit& b);

/*!
 * \class CDistributedLocator
 * \brief Collective location of points in a partitioned simplex mesh (MPI_TRANSFER_PLAN.md 4.2-4.5): an ADT of the
 *        elements this rank owns (COwnedSimplices), the replicated tree of per-rank routing boxes (union of the ADT's
 *        inflated element boxes, CRankBoxTree), the replicated canonical boundary, and the rules of the serial locator:
 *        a point on markers that the mesh has takes the canonical nearest face of those markers; otherwise the
 *        canonical containing element (weights clipped at 0 and renormalized), else the canonical nearest face of any
 *        marker. Distance limit max(face size, absoluteLimit, 1e-3 x the diagonal of the mesh) per face stencil.
 * \note Queries go to the ranks whose routing boxes contain the point (every element the ADT could accept for it is
 *       owned by one of them), in chunks of bounded size (memory ceiling of the transfer, 5.17); replies are merged by
 *       the canonical rule, so the stencils do not depend on the number of ranks or on the partition.
 */
class CDistributedLocator {
 public:
  /*!
   * \brief Stencil of a located point: global indices of the mesh points and weights.
   */
  struct Stencil {
    unsigned short nPoint = 0;
    uint64_t gid[4] = {};
    passivedouble weight[4] = {};
    bool inside = true;            /*!< \brief Inside an element of the mesh. */
    bool onFace = false;           /*!< \brief Closest point of a boundary face. */
    passivedouble distance = 0.0, faceSize = 0.0;
    bool beyondLimit = false;
    uint32_t marker = 0;           /*!< \brief Face stencils: marker id of the face. */
    CSimplexKey key = {};          /*!< \brief Key of the element or face (the stencil decision). */
  };

  /*!
   * \brief Collective: search structures of the mesh.
   * \param[in] geometry - Finest grid, partitioned.
   * \param[in] markerTags - Name of each marker of the geometry (empty names: unnamed markers, one rank only).
   * \param[in] absoluteLimit - Distance accepted outside the mesh at any face size.
   */
  CDistributedLocator(const CGeometry& geometry, const std::vector<std::string>& markerTags, const CConfig& config,
                      passivedouble absoluteLimit);
  ~CDistributedLocator();

  /*!
   * \brief Collective: stencils of the points of this rank (any number, also none).
   * \param[in] coord - Coordinates (nDim per point).
   * \param[in] markers - Marker ids (config positions) of each point.
   * \param[in] callerBytes - Live heap bytes of the caller during the call, without the arguments (memory ceiling,
   *            MPI_TRANSFER_PLAN.md 5.17: query chunks are chosen so that everything fits).
   */
  std::vector<Stencil> Locate(const std::vector<passivedouble>& coord,
                              const std::vector<std::vector<uint32_t>>& markers, size_t callerBytes = 0);

  /*!
   * \brief Collective: the canonical containing element of each point (CElementHit::found false outside the mesh).
   * \param[in] callerBytes - As for Locate.
   */
  std::vector<CElementHit> LocateElements(const std::vector<passivedouble>& coord, size_t callerBytes = 0);

  /*! \brief Canonical boundary (replicated). */
  CCanonicalBoundary& Boundary() { return *boundary; }

  passivedouble GetDomainSize() const { return domainSize; }
  passivedouble GetDistanceLimit(passivedouble faceSize) const;

  /*! \brief Face stencil from a canonical face hit. */
  Stencil FaceStencil(const CFaceHit& hit) const;

  /*! \brief Element stencil (weights clipped at 0 and renormalized) from a canonical element hit. */
  static Stencil ElementStencil(const CElementHit& hit, unsigned short nDim);

  /*! \brief Statistics of the last Locate on this rank: query chunks, queries sent and received. */
  unsigned long GetLastChunks() const { return lastChunks; }
  unsigned long GetLastQueriesSent() const { return lastSent; }
  unsigned long GetLastQueriesReceived() const { return lastReceived; }
  /*! \brief Largest number of queries this rank received in one chunk of the last Locate. */
  unsigned long GetLastMaxChunkReceived() const { return lastMaxChunkReceived; }
  /*! \brief Smallest memory ceiling that admits the last location (largest over the ranks). */
  size_t GetLastMinimumCeiling() const { return lastMinimumCeiling; }

  /*! \brief Heap bytes held by the search structures on this rank (all arrays at their capacity). */
  size_t GetMemory() const;

 private:
  unsigned short nDim;
  passivedouble absoluteLimit, domainSize;
  COwnedSimplices owned;
  std::vector<uint64_t> ownedGid;         /*!< \brief Global node indices of the owned elements (element order). */
  std::vector<su2double> adtCoord;        /*!< \brief Coordinates of the nodes of the owned elements. */
  std::unique_ptr<CADTElemClass> adt;     /*!< \brief Owned elements (null if none). */
  CRankBoxTree routing;
  std::unique_ptr<CCanonicalBoundary> boundary;
  unsigned long lastChunks = 0, lastSent = 0, lastReceived = 0, lastMaxChunkReceived = 0;
  size_t lastMinimumCeiling = 0;

  CElementHit LocalBest(const passivedouble* x);

  /*!
   * \brief LocateElements with the memory admission: callerBytes are live bytes of the caller (not the arguments,
   *        counted here), afterBytes are allocated after the location while the results live (Locate's stencils).
   * \param[out] predicted - Bound of the live bytes of the phase (resident, caller, arguments, own allocations).
   */
  std::vector<CElementHit> LocateElementsImpl(const std::vector<passivedouble>& coord, size_t callerBytes,
                                              size_t afterBytes, size_t* predicted);
};
