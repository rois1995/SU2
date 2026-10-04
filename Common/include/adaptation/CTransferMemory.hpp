/*!
 * \file CTransferMemory.hpp
 * \brief Memory model of the distributed solution transfers (MPI_TRANSFER_PLAN.md 5.17, REVIEW5_FIX_PLAN.md S1-S3):
 *        exact byte counts of containers, saturating arithmetic, the peak models of the gated phases (query chunks
 *        of the point location, import sub-rounds of the conservative projection) and the phase hook of the tests.
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

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

/*!
 * \brief Byte counts are requested bytes (what operator new is asked for), the quantity the memory ceiling of the
 *        transfer bounds. Every count saturates at SIZE_MAX (an overflow means: does not fit).
 */
namespace transfer_memory {

constexpr size_t kMax = std::numeric_limits<size_t>::max();

inline size_t Add(size_t a, size_t b) { return (a > kMax - b) ? kMax : a + b; }
inline size_t Mul(size_t a, size_t b) { return (a != 0 && b > kMax / a) ? kMax : a * b; }
template <class... T>
size_t Add(size_t a, size_t b, T... rest) {
  return Add(Add(a, b), rest...);
}
inline size_t CeilDiv(size_t a, size_t b) { return a == 0 ? 0 : (a - 1) / b + 1; }

/*! \brief Heap bytes of a vector (its capacity). */
template <class T>
size_t Bytes(const std::vector<T>& v) {
  return v.capacity() * sizeof(T);
}
/*! \brief Heap bytes of a vector of vectors (the outer array and every inner capacity). */
template <class T>
size_t Bytes(const std::vector<std::vector<T>>& v) {
  size_t bytes = v.capacity() * sizeof(std::vector<T>);
  for (const auto& inner : v) bytes += inner.capacity() * sizeof(T);
  return bytes;
}
/*! \brief Heap bytes of a string (beyond the small-string buffer of 15 characters, with the terminator). */
inline size_t Bytes(const std::string& s) { return s.capacity() > 15 ? s.capacity() + 1 : 0; }
/*! \brief Heap bytes of a vector of strings. */
inline size_t Bytes(const std::vector<std::string>& v) {
  size_t bytes = v.capacity() * sizeof(std::string);
  for (const auto& s : v) bytes += Bytes(s);
  return bytes;
}
/*! \brief The vector<bool> bit array (rounded up to whole 64-bit words). */
inline size_t Bytes(const std::vector<bool>& v) { return CeilDiv(v.capacity(), 64) * 8; }

/*!
 * \brief Bound of the live bytes of a vector filled by push_back/insert from empty to at most n elements: the
 *        capacity doubles, and while it grows the old and the new array coexist (c + 2c < 3n), so 3 n sizeof(T).
 */
inline size_t GrowthBound(size_t n, size_t elementBytes) { return Mul(Mul(3, n), elementBytes); }

/*!
 * \brief Staging buffers of one CPassiveComm::AlltoallvRounds round: a send and a receive buffer, each at most the
 *        round size B (the rounds plan B - P bytes, plus at most one byte per peer from the per-peer ceilings) and at
 *        most the remote (not self) bytes of the exchange.
 */
inline size_t Staging(size_t roundBytes, size_t remoteSend, size_t remoteRecv) {
  return Add(remoteSend < roundBytes ? remoteSend : roundBytes, remoteRecv < roundBytes ? remoteRecv : roundBytes);
}

/*!
 * \brief Transport-only temporaries of one AlltoallvRounds (offsets, 64-bit counts, chunks, int counts and
 *        displacements, the count Alltoall): at most 12 words per peer plus a constant.
 */
inline size_t TransportBytes(size_t nRank) { return Add(Mul(96, nRank), 256); }

/*!
 * \brief Bound of the transient bytes of one collect-all containment query of a CADTElemClass with nElem elements
 *        (element ids and 8 weights per candidate, candidates <= nElem, vectors grown by push_back) plus the growth of
 *        the two traversal fronts of one thread (at most nElem leaves each, reserved at 200).
 */
size_t ContainmentQueryBound(size_t nElem, size_t activeBytes);

/*!
 * \brief Containment scratch when the retained workspace envelope is already in the baseline: candidates, weights,
 *        and the old front arrays during growth (the new arrays are covered by the retained envelope).
 */
size_t ContainmentQueryTransientBound(size_t nElem, size_t activeBytes);

/*!
 * \brief The same for an intersecting-elements box query (element ids only) and the fronts.
 */
size_t IntersectionQueryBound(size_t nElem);

/*!
 * \brief Point location in query chunks (CDistributedLocator::LocateElements): per-peer query counts and the
 *        live baseline; Peak(Q) bounds the live bytes of the chunk phases for Q chunks and does not increase with Q.
 */
struct CLocatorChunkModel {
  size_t nRank = 1;
  int rank = 0;
  size_t queryBytes = 0, replyBytes = 0;
  size_t roundBytes = 0;         /*!< \brief Effective CPassiveComm round size. */
  size_t baseline = 0;           /*!< \brief Live bytes during every chunk (resident, arguments, caller, results). */
  size_t searchBytes = 0;        /*!< \brief Transient bytes of the local location of one query. */
  std::vector<uint64_t> out, in; /*!< \brief Total queries to / from each rank. */

  /*! \brief Largest number of chunks worth testing: beyond it every nonempty list has one query per chunk. */
  size_t MaxChunks() const;
  /*! \brief Bound of the live bytes of the chunk phases (query exchange, location, reply exchange) for Q chunks. */
  size_t Peak(size_t nChunk) const;
  /*! \brief Smallest Q in [1, MaxChunks()] with Peak(Q) <= ceiling, or 0 if none. */
  size_t SmallestChunks(size_t ceiling) const;
};

/*!
 * \brief Import of donor elements in sub-rounds (CDistributedProjection::Supermesh, one import group): the k-th
 *        part (at most ceil(n/K)) of every destination's element list per sub-round. The envelope counts every element
 *        with all its nodes (actual nodes are deduplicated per destination and sub-round, so the wire is smaller);
 *        Peak(K) does not increase with K.
 */
struct CImportRoundModel {
  size_t nRank = 1;
  int rank = 0;
  size_t elemBytes = 0; /*!< \brief Wire bytes of an element record. */
  size_t nodeBytes = 0; /*!< \brief Wire bytes of a node record. */
  unsigned short nNode = 0;
  size_t roundBytes = 0;
  size_t baseline = 0;             /*!< \brief Live bytes during the sub-rounds (incl. the reserved sub-mesh). */
  std::vector<uint64_t> sendElems; /*!< \brief Elements this rank sends to each rank in the group. */
  std::vector<uint64_t> recvElems; /*!< \brief Elements this rank receives from each rank in the group. */

  size_t MaxRounds() const;
  /*! \brief Envelope wire bytes of the lists to (send) or from (receive) every rank for K sub-rounds. */
  size_t Wire(const std::vector<uint64_t>& elems, size_t nRound, bool remoteOnly) const;
  size_t Peak(size_t nRound) const;
  size_t SmallestRounds(size_t ceiling) const;
};

/*!
 * \brief Gated phases of the transfers (the tests observe the live bytes of each with an allocation probe).
 */
enum class TransferPhase { LOCATE = 0, IMPORT = 1, SLIVERS = 2 };

/*!
 * \brief Called at the start (begin = true, predicted 0) and at the end of a gated phase on every rank. At the end,
 *        predicted is the transfer's bound of the live bytes during the phase of everything it and its caller hold
 *        (resident structures, the caller's declared bytes and arguments, the phase's own allocations). Tests only.
 */
using TransferMemoryHook = void (*)(TransferPhase phase, bool begin, size_t predicted);
void SetTransferMemoryHook(TransferMemoryHook hook);
void TransferMemoryEvent(TransferPhase phase, bool begin, size_t predicted);

}  // namespace transfer_memory
