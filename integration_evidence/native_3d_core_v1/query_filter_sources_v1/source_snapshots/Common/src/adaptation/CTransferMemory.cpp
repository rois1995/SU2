/*!
 * \file CTransferMemory.cpp
 * \brief Memory model of the distributed solution transfers (see CTransferMemory.hpp).
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

#include "../../include/adaptation/CTransferMemory.hpp"

#include <algorithm>

namespace transfer_memory {

size_t ContainmentQueryBound(size_t nElem, size_t activeBytes) {
  return Add(ContainmentQueryTransientBound(nElem, activeBytes),
             Mul(4, Mul(std::max<size_t>(nElem, 200), sizeof(unsigned long))));
}

size_t ContainmentQueryTransientBound(size_t nElem, size_t activeBytes) {
  const size_t candidates = Add(GrowthBound(nElem, sizeof(unsigned long)), GrowthBound(nElem, Mul(8, activeBytes)));
  const size_t fronts = Mul(2, Mul(std::max<size_t>(nElem, 200), sizeof(unsigned long)));
  return Add(candidates, fronts, Mul(64, activeBytes));
}

size_t IntersectionQueryBound(size_t nElem) {
  return Add(GrowthBound(nElem, sizeof(unsigned long)),
             Mul(2, GrowthBound(std::max<size_t>(nElem, 200), sizeof(unsigned long))));
}

/*--------------------------------------------------------------------------------------------------------------------*/

size_t CLocatorChunkModel::MaxChunks() const {
  uint64_t largest = 1;
  for (const auto n : out) largest = std::max(largest, n);
  for (const auto n : in) largest = std::max(largest, n);
  return largest;
}

size_t CLocatorChunkModel::Peak(size_t nChunk) const {
  size_t uOut = 0, uIn = 0;
  for (const auto n : out) uOut = Add(uOut, CeilDiv(n, nChunk));
  for (const auto n : in) uIn = Add(uIn, CeilDiv(n, nChunk));
  const size_t self = CeilDiv(out[rank], nChunk);
  const size_t q = queryBytes, r = replyBytes;
  /*--- Retained per chunk: first, last, send bytes, receive bytes, reply send and receive bytes (one word per peer).
   * ---*/
  const size_t meta = Mul(48, nRank);
  /*--- A: query exchange (send buffer, result, staging); B: location (queries, replies, one search); C: reply
   *    exchange (replies, result, staging). ---*/
  const size_t a = Add(Mul(q, uOut), Mul(q, uIn), Staging(roundBytes, Mul(q, uOut - self), Mul(q, uIn - self)),
                       TransportBytes(nRank));
  const size_t b = Add(Mul(q, uIn), Mul(r, uIn), searchBytes);
  const size_t c = Add(Mul(r, uIn), Mul(r, uOut), Staging(roundBytes, Mul(r, uIn - self), Mul(r, uOut - self)),
                       TransportBytes(nRank));
  return Add(baseline, meta, std::max({a, b, c}));
}

size_t CLocatorChunkModel::SmallestChunks(size_t ceiling) const {
  size_t hi = MaxChunks();
  if (Peak(hi) > ceiling) return 0;
  size_t lo = 1;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (Peak(mid) <= ceiling) {
      hi = mid;
    } else {
      lo = mid + 1;
    }
  }
  return lo;
}

/*--------------------------------------------------------------------------------------------------------------------*/

size_t CImportRoundModel::MaxRounds() const {
  uint64_t largest = 1;
  for (const auto n : sendElems) largest = std::max(largest, n);
  for (const auto n : recvElems) largest = std::max(largest, n);
  return largest;
}

size_t CImportRoundModel::Wire(const std::vector<uint64_t>& elems, size_t nRound, bool remoteOnly) const {
  const size_t perElem = Add(elemBytes, Mul(nNode, nodeBytes));
  size_t bytes = 0;
  for (size_t p = 0; p < elems.size(); ++p) {
    if (remoteOnly && static_cast<int>(p) == rank) continue;
    /*--- Element and node counts of every stream (also empty), then the records. ---*/
    bytes = Add(bytes, 2 * sizeof(uint64_t), Mul(CeilDiv(elems[p], nRound), perElem));
  }
  return bytes;
}

size_t CImportRoundModel::Peak(size_t nRound) const {
  const size_t send = Wire(sendElems, nRound, false), recv = Wire(recvElems, nRound, false);
  const size_t staging = Staging(roundBytes, Wire(sendElems, nRound, true), Wire(recvElems, nRound, true));
  /*--- Packing scratch: the distinct nodes of one stream (at most nNode per element of the largest list). ---*/
  uint64_t largest = 0;
  for (const auto n : sendElems) largest = std::max<uint64_t>(largest, CeilDiv(n, nRound));
  const size_t scratch = Mul(Mul(largest, nNode), sizeof(unsigned long));
  /*--- Sub-round: send buffer and packing scratch, then send buffer, receive buffer and staging, then the receive
   *    buffer while it is appended to the reserved sub-mesh (in the baseline). Per-peer counts and offsets. ---*/
  const size_t meta = Mul(32, nRank);
  const size_t exchange = Add(send, recv, staging, TransportBytes(nRank));
  return Add(baseline, meta, std::max(Add(send, scratch), exchange));
}

size_t CImportRoundModel::SmallestRounds(size_t ceiling) const {
  size_t hi = MaxRounds();
  if (Peak(hi) > ceiling) return 0;
  size_t lo = 1;
  while (lo < hi) {
    const size_t mid = lo + (hi - lo) / 2;
    if (Peak(mid) <= ceiling) {
      hi = mid;
    } else {
      lo = mid + 1;
    }
  }
  return lo;
}

/*--------------------------------------------------------------------------------------------------------------------*/

namespace {
TransferMemoryHook memoryHook = nullptr;
}

void SetTransferMemoryHook(TransferMemoryHook hook) { memoryHook = hook; }

void TransferMemoryEvent(TransferPhase phase, bool begin, size_t predicted) {
  if (memoryHook != nullptr) memoryHook(phase, begin, predicted);
}

}  // namespace transfer_memory
