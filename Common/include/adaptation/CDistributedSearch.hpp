/*!
 * \file CDistributedSearch.hpp
 * \brief Building blocks of the distributed solution transfer (MPI_TRANSFER_PLAN.md, milestone M0): ownership and keys
 *        of simplices and boundary faces, the replicated tree of per-rank boxes, the rendezvous directory of point
 *        records, the collective failure protocol, accurate global sums, packed field records.
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

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "../basic_types/datatype_structure.hpp"

class CConfig;
class CGeometry;

/*!
 * \brief Key of a simplex or a face: its global node indices sorted (unused entries UINT64_MAX). Partition
 *        independent and unique in a valid mesh; compared lexicographically.
 */
using CSimplexKey = std::array<uint64_t, 4>;

/*! \brief Key of the n global indices given (any order). */
CSimplexKey MakeSimplexKey(const uint64_t* gids, unsigned short n);

/*!
 * \brief The volume elements of a partitioned geometry that this rank owns: those whose node of smallest global index
 *        is a domain point of this rank (each element is owned by exactly one rank; the rule of CMeshGather). Triangles
 *        (2D) or tetrahedra (3D) only (error otherwise).
 */
struct COwnedSimplices {
  unsigned short nDim = 0, nNode = 0;
  std::vector<unsigned long> elem;  /*!< \brief Local index of each owned element in the geometry. */
  std::vector<unsigned long> nodes; /*!< \brief Local point indices, in the element's node order (nNode each). */
  std::vector<CSimplexKey> keys;    /*!< \brief Key of each owned element. */
  unsigned long size() const { return elem.size(); }
};
COwnedSimplices OwnedSimplices(const CGeometry& geometry);

/*!
 * \brief Position of a marker name in the marker list of the config file (the same on every rank); the empty name
 *        (unnamed donor markers, one rank only) is one past the end. Error for an unknown name.
 */
uint32_t MarkerConfigId(const CConfig& config, const std::string& name);

/*!
 * \brief The boundary faces (lines in 2D, triangles in 3D) of the physical markers of a partitioned geometry that this
 *        rank owns (ownership as for the volume elements). SEND_RECEIVE markers and vertex elements are skipped,
 *        periodic markers are an error, markers without elements contribute nothing.
 * \param[in] markerTags - Name of each marker of the geometry (by iMarker); an empty name is an unnamed marker.
 */
struct COwnedFaces {
  unsigned short nDim = 0;
  std::vector<uint32_t> markerId;   /*!< \brief MarkerConfigId of the face's marker. */
  std::vector<unsigned long> nodes; /*!< \brief Local point indices in the face's node order (nDim each). */
  std::vector<CSimplexKey> keys;    /*!< \brief Key of each face. */
  unsigned long size() const { return markerId.size(); }
};
COwnedFaces OwnedBoundaryFaces(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                               const CConfig& config);

/*!
 * \brief MarkerConfigId of the physical markers of each local point (from the local boundary elements: complete for
 *        the domain points, whose boundary elements are all local), sorted, without duplicates.
 */
std::vector<std::vector<uint32_t>> PointMarkerIds(const CGeometry& geometry, const std::vector<std::string>& markerTags,
                                                  const CConfig& config);

/*!
 * \brief Collective: smallest and largest coordinate of the domain points of all ranks (exact), and the diagonal of
 *        that box computed as the serial locator does.
 */
void GlobalBoundingBox(const CGeometry& geometry, double* xMin, double* xMax, double* diagonal);

/*!
 * \brief Boxes by recursive coordinate bisection: the items (boxes with 2 nDim doubles, min then max, and a centroid)
 *        are split at the median of the centroids along the longest extent of the centroids, until at most maxBoxes
 *        groups; each group's box is the union of its items' boxes.
 * \param[out] groupOfItem - If not null: the group of each item.
 * \return The boxes (2 nDim doubles each) of the non-empty groups, in the order of the bisection.
 */
std::vector<double> BisectionBoxes(unsigned short nDim, const std::vector<double>& itemBoxes,
                                   const std::vector<double>& centroids, unsigned long maxBoxes,
                                   std::vector<unsigned long>* groupOfItem);

/*!
 * \class CRankBoxTree
 * \brief The boxes of all ranks (2 nDim doubles: min corner, then max corner), replicated on every rank, in a bounding
 *        volume hierarchy. Point and box tests include the bounds (a point on a box face is inside), as the ADT does.
 */
class CRankBoxTree {
 public:
  /*!
   * \brief Collective: the boxes of every rank (Allgatherv), in rank order, then the tree.
   * \param[in] localBoxes - Boxes of this rank (2 nDim doubles each, may be empty).
   */
  void Build(unsigned short nDim, const std::vector<double>& localBoxes);

  unsigned long GetnBox() const { return rank.size(); }
  int BoxRank(unsigned long iBox) const { return rank[iBox]; }
  unsigned long BoxLocalIndex(unsigned long iBox) const { return localIndex[iBox]; }
  const double* Box(unsigned long iBox) const { return &boxes[2 * nDim * iBox]; }
  /*! \brief Index of the first box of a rank (the boxes of rank r are firstBox[r] .. firstBox[r+1]-1). */
  unsigned long FirstBox(int iRank) const { return firstBox[iRank]; }

  /*! \brief Boxes that contain the point (sorted). */
  void BoxesContaining(const double* x, std::vector<unsigned long>& result) const;
  /*! \brief Boxes that intersect the box [lo, hi] (sorted). */
  void BoxesIntersecting(const double* lo, const double* hi, std::vector<unsigned long>& result) const;
  /*! \brief Boxes whose possible (smallest) distance to x is at most r (sorted). */
  void BoxesWithinDistance(const double* x, double r, std::vector<unsigned long>& result) const;

  /*! \brief Ranks of the boxes above (sorted, unique). */
  void RanksContaining(const double* x, std::vector<int>& result) const;
  void RanksIntersecting(const double* lo, const double* hi, std::vector<int>& result) const;
  void RanksWithinDistance(const double* x, double r, std::vector<int>& result) const;

 private:
  struct Node {
    double lo[3], hi[3];
    long child[2];             /*!< \brief Children (-1: leaf). */
    unsigned long first, last; /*!< \brief Leaf: range in order. */
  };
  template <class Test>
  void Query(const Test& test, std::vector<unsigned long>& result) const;
  void ToRanks(const std::vector<unsigned long>& boxIds, std::vector<int>& result) const;

  unsigned short nDim = 0;
  std::vector<double> boxes;
  std::vector<int> rank;
  std::vector<unsigned long> localIndex, firstBox, order;
  std::vector<Node> nodes;
};

/*!
 * \brief Absolute memory ceiling per rank of the distributed solution transfer (MPI_TRANSFER_PLAN.md 5.17): resident
 *        structures, planned imports and query chunks are admitted against it (default 2 GiB; the same value must be
 *        set on every rank, tests force small values). The transfer's passive round size is min(1 GiB, ceiling / 8).
 */
size_t GetTransferMemoryCeiling();
void SetTransferMemoryCeiling(size_t bytes);

/*!
 * \brief Scope that sets the round size of CPassiveComm to min(current, ceiling / 8) and restores it (all ranks).
 */
class CTransferRoundScope {
 public:
  CTransferRoundScope();
  ~CTransferRoundScope();
  CTransferRoundScope(const CTransferRoundScope&) = delete;
  CTransferRoundScope& operator=(const CTransferRoundScope&) = delete;

 private:
  size_t saved;
};

/*!
 * \brief Local failure of a pipeline step (the worst one of this rank) for CollectiveFailure.
 */
struct CLocalFailure {
  unsigned short severity = 0; /*!< \brief 0: no failure; larger is worse. */
  uint64_t gid = UINT64_MAX;   /*!< \brief Global index the failure is about (ties: smallest wins). */
  std::string message;         /*!< \brief Complete message (printed if this failure is elected). */

  /*! \brief Keep the worse of this and the given failure (higher severity, then smaller gid). */
  void Set(unsigned short sev, uint64_t id, const std::string& text) {
    if (sev == 0) return;
    if (severity == 0 || sev > severity || (sev == severity && id < gid)) {
      severity = sev;
      gid = id;
      message = text;
    }
  }
  bool Failed() const { return severity > 0; }
};

/*!
 * \brief The failure elected over the ranks by (severity descending, gid, rank).
 */
struct CElectedFailure {
  bool any = false;
  int rank = -1;
  unsigned short severity = 0;
  uint64_t gid = UINT64_MAX;
  std::string message;
};

/*!
 * \brief Collective: elect one failure over the ranks (three reductions) and broadcast its message from its rank.
 */
CElectedFailure ElectFailure(const CLocalFailure& local);

/*!
 * \brief Collective: if any rank failed, every rank calls SU2_MPI::Error with the message of the elected failure
 *        (a collective error call: the MPI_Ibarrier of the error completes on all ranks, rank 0 prints).
 * \param[in] function - Name of the calling function for the message.
 */
void CollectiveFailure(const CLocalFailure& local, const std::string& function);

/*!
 * \brief Size in bytes of one serialized field value: the value, and in forward mode (DIRECT_DIFF) its derivative.
 */
inline constexpr size_t FieldValueBytes() {
#ifdef CODI_FORWARD_TYPE
  return 2 * sizeof(double);
#else
  return sizeof(double);
#endif
}

/*! \brief Serialize n field values (value, and derivative in forward mode) at dst. */
inline void PackFieldValues(char* dst, const su2double* values, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    const double value = SU2_TYPE::GetValue(values[i]);
    std::memcpy(dst + i * FieldValueBytes(), &value, sizeof(double));
#ifdef CODI_FORWARD_TYPE
    const double derivative = SU2_TYPE::GetDerivative(values[i]);
    std::memcpy(dst + i * FieldValueBytes() + sizeof(double), &derivative, sizeof(double));
#endif
  }
}

/*! \brief Restore n field values serialized by PackFieldValues. */
inline void UnpackFieldValues(const char* src, su2double* values, size_t n) {
  for (size_t i = 0; i < n; ++i) {
    double value = 0.0;
    std::memcpy(&value, src + i * FieldValueBytes(), sizeof(double));
    values[i] = value;
#ifdef CODI_FORWARD_TYPE
    double derivative = 0.0;
    std::memcpy(&derivative, src + i * FieldValueBytes() + sizeof(double), sizeof(double));
    SU2_TYPE::SetDerivative(values[i], derivative);
#endif
  }
}

/*! \brief Append a trivially copyable value to a byte buffer, and read one (memcpy, no aliasing casts). */
template <class T>
inline void PutBytes(std::vector<char>& buffer, const T& value) {
  const auto* p = reinterpret_cast<const char*>(&value);
  buffer.insert(buffer.end(), p, p + sizeof(T));
}
template <class T>
inline T GetBytes(const char*& src) {
  T value;
  std::memcpy(&value, src, sizeof(T));
  src += sizeof(T);
  return value;
}

/*!
 * \class CPointDirectory
 * \brief Rendezvous directory of fixed-size point records by global index: the record of point g lives on the block
 *        owner min(P-1, g / ceil(N/P)) (blocks may be empty when N < P). Built from the records of the owned points of
 *        every rank (each global index 0..N-1 exactly once, checked: collective error naming the index otherwise);
 *        Fetch returns records bit for bit.
 */
class CPointDirectory {
 public:
  CPointDirectory() = default;

  /*!
   * \brief Collective build.
   * \param[in] gids - Global indices of this rank's owned points.
   * \param[in] records - recordBytes bytes per point, in the order of gids.
   * \param[in] what - Name of the mesh for the error messages.
   */
  void Build(const std::vector<uint64_t>& gids, const std::vector<char>& records, size_t recordBytes,
             const std::string& what);

  /*!
   * \brief Collective: the records of the given global indices (any order, repetitions allowed), recordBytes each in
   *        the order of the request.
   */
  std::vector<char> Fetch(const std::vector<uint64_t>& gids) const;

  uint64_t GetnGlobal() const { return nGlobal; }
  size_t GetRecordBytes() const { return recordBytes; }
  /*! \brief Bytes held by this rank. */
  size_t GetMemory() const { return block.size(); }

 private:
  int Owner(uint64_t gid) const;

  uint64_t nGlobal = 0, blockSize = 1, firstGid = 0;
  size_t recordBytes = 0;
  std::vector<char> block;
};

/*!
 * \class CAccurateSumBatch
 * \brief Collective accurate sums of several quantities with one Allgatherv (CAccurateSum: Neumaier per rank, second
 *        Neumaier pass over the rank partials in rank order). Quantities are registered with their local terms, then
 *        Reduce is called by every rank with the same number of quantities.
 * \note Active quantities (AddActive) are summed as values and, in forward mode, as derivatives (an independent second
 *       sum); Get returns the value, GetActive the su2double with its derivative.
 */
class CAccurateSumBatch {
 public:
  /*! \brief Register the terms x[0], x[stride], ... of a quantity; returns its index. */
  size_t Add(const double* x, size_t n, size_t stride = 1);
  size_t Add(const std::vector<double>& x) { return Add(x.data(), x.size(), 1); }
  /*! \brief Register a quantity of one local term. */
  size_t AddValue(double x) { return Add(&x, 1, 1); }
  /*! \brief Register active terms (value, and the derivative in forward mode). */
  size_t AddActive(const su2double* x, size_t n, size_t stride = 1);
  size_t AddActive(const std::vector<su2double>& x) { return AddActive(x.data(), x.size(), 1); }

  /*! \brief Collective: the global sums of all registered quantities. */
  void Reduce();

  /*! \brief Not collective: the sums of this rank's terms only (serial code on one rank of a communicator). */
  void ReduceLocal();

  double Get(size_t q) const { return results[q]; }
  double GetAbs(size_t q) const { return absSums[q]; }
  /*! \brief Whether every term of every rank and the merge were finite (the same on all ranks). */
  bool Finite(size_t q) const { return finite[q]; }
  /*! \brief Index of the first nonfinite local term of this rank (n if none). */
  size_t LocalFirstBad(size_t q) const { return firstBad[q]; }
  su2double GetActive(size_t q) const;
  bool FiniteActive(size_t q) const;

  size_t Size() const { return firstBad.size(); }

 private:
  std::vector<double> triples, results, absSums;
  std::vector<bool> finite;
  std::vector<size_t> firstBad;
  std::vector<long> derivativeOf; /*!< \brief Quantity holding the derivative sum of an active quantity (-1: none). */
};
