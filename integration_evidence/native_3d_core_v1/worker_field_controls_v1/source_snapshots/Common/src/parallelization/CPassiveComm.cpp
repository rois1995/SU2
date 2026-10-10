/*!
 * \file CPassiveComm.cpp
 * \brief Transport of passive data as packed bytes with the native MPI functions, in rounds of bounded size.
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

#include "../../include/parallelization/CPassiveComm.hpp"

#include <algorithm>
#include <cstdint>

#include "../../include/geometry/CGeometry.hpp"

size_t CPassiveComm::roundBytes = CPassiveComm::DEFAULT_ROUND_BYTES;
unsigned long CPassiveComm::lastRounds = 0;
size_t CPassiveComm::lastPeakRoundBytes = 0;

namespace {

/*--- Offsets of consecutive blocks (size n + 1). ---*/
std::vector<size_t> Offsets(const std::vector<size_t>& counts) {
  std::vector<size_t> offsets(counts.size() + 1, 0);
  for (size_t i = 0; i < counts.size(); ++i) offsets[i + 1] = offsets[i] + counts[i];
  return offsets;
}

size_t CeilDiv(size_t a, size_t b) { return a == 0 ? 0 : (a - 1) / b + 1; }

/*--- Bytes of the stream of n bytes that go in round r when the stream is cut into rounds of chunk bytes. ---*/
size_t ChunkOfRound(size_t n, size_t chunk, size_t r) {
  const size_t start = std::min(n, r * chunk);
  return std::min(chunk, n - start);
}

#ifdef HAVE_MPI
MPI_Comm Comm() { return SU2_MPI::GetComm(); }

/*--- The usable round size: chunks are rounded up per peer, at most one byte each, so the aggregate stays within the
 *    round size if the rounds are planned with this much less. ---*/
size_t PlanningBytes(size_t roundBytes, int size) {
  if (roundBytes <= static_cast<size_t>(size)) {
    SU2_MPI::Error("The round size of the passive transport (" + std::to_string(roundBytes) +
                       " bytes) must exceed the number of ranks.",
                   CURRENT_FUNCTION);
  }
  return roundBytes - size;
}

/*--- Non-null pointer for MPI calls with empty buffers. ---*/
template <class T>
T* Data(std::vector<T>& v) {
  static T dummy{};
  return v.empty() ? &dummy : v.data();
}
#endif

}  // namespace

void CPassiveComm::SetRoundBytes(size_t bytes) {
  if (bytes == 0 || bytes > static_cast<size_t>(INT_MAX)) {
    SU2_MPI::Error("The round size of the passive transport must be in 1 .. INT_MAX bytes.", CURRENT_FUNCTION);
  }
  roundBytes = bytes;
}

void CPassiveComm::Alltoall(const void* send, void* recv, size_t blockBytes, Communicator comm) {
#ifdef HAVE_MPI
  const int size = Size(comm);
  const int count = CheckedInt(blockBytes, "Block size of CPassiveComm::Alltoall");
  CheckedInt(blockBytes * size, "Total size of CPassiveComm::Alltoall");
  MPI_Alltoall(const_cast<void*>(send), count, MPI_BYTE, recv, count, MPI_BYTE, comm);
#else
  if (blockBytes > 0) std::memcpy(recv, send, blockBytes);
#endif
}

std::vector<char> CPassiveComm::AlltoallvRounds(const char* send, const std::vector<size_t>& sendBytes,
                                                std::vector<size_t>& recvBytes, Communicator comm) {
  const int size = Size(comm), rank = Rank(comm);
  if (sendBytes.size() != static_cast<size_t>(size)) {
    SU2_MPI::Error("AlltoallvRounds needs one send count per rank.", CURRENT_FUNCTION);
  }
  const auto sendOffset = Offsets(sendBytes);

  /*--- Byte counts of every pair. ---*/
  std::vector<uint64_t> sendCount64(size), recvCount64(size);
  for (int q = 0; q < size; ++q) sendCount64[q] = sendBytes[q];
  Alltoall(sendCount64.data(), recvCount64.data(), sizeof(uint64_t), comm);
  recvBytes.assign(recvCount64.begin(), recvCount64.end());
  const auto recvOffset = Offsets(recvBytes);

  std::vector<char> result(recvOffset[size]);
  lastRounds = 0;
  lastPeakRoundBytes = 0;

  /*--- Own data, directly. ---*/
  if (recvBytes[rank] != sendBytes[rank]) SU2_MPI::Error("Inconsistent self message.", CURRENT_FUNCTION);
  if (sendBytes[rank] > 0) std::copy_n(send + sendOffset[rank], sendBytes[rank], result.data() + recvOffset[rank]);

#ifdef HAVE_MPI
  /*--- Rounds: enough that the aggregate of every rank's sends and receives fits one round. ---*/
  const size_t planning = PlanningBytes(roundBytes, size);
  const size_t sendTotal = sendOffset[size] - sendBytes[rank], recvTotal = recvOffset[size] - recvBytes[rank];
  const unsigned long nRound =
      AllreduceMax(std::max(CeilDiv(sendTotal, planning), CeilDiv(recvTotal, planning)), comm);
  lastRounds = nRound;

  std::vector<size_t> sendChunk(size, 0), recvChunk(size, 0);
  for (int q = 0; q < size; ++q) {
    if (q == rank || nRound == 0) continue;
    sendChunk[q] = CeilDiv(sendBytes[q], nRound);
    recvChunk[q] = CeilDiv(recvBytes[q], nRound);
  }

  std::vector<int> sendCounts(size), sendDispl(size), recvCounts(size), recvDispl(size);
  for (unsigned long r = 0; r < nRound; ++r) {
    size_t nSend = 0, nRecv = 0;
    for (int q = 0; q < size; ++q) {
      const size_t s = (q == rank) ? 0 : ChunkOfRound(sendBytes[q], sendChunk[q], r);
      const size_t v = (q == rank) ? 0 : ChunkOfRound(recvBytes[q], recvChunk[q], r);
      sendCounts[q] = CheckedInt(s, "Send count of a round");
      sendDispl[q] = CheckedInt(nSend, "Send displacement of a round");
      recvCounts[q] = CheckedInt(v, "Receive count of a round");
      recvDispl[q] = CheckedInt(nRecv, "Receive displacement of a round");
      nSend += s;
      nRecv += v;
    }
    CheckedInt(nSend, "Send bytes of a round");
    CheckedInt(nRecv, "Receive bytes of a round");
    lastPeakRoundBytes = std::max({lastPeakRoundBytes, nSend, nRecv});

    /*--- Fresh buffers for each round. ---*/
    std::vector<char> sendBuffer(nSend), recvBuffer(nRecv);
    for (int q = 0; q < size; ++q) {
      if (sendCounts[q] > 0)
        std::copy_n(send + sendOffset[q] + r * sendChunk[q], sendCounts[q], sendBuffer.data() + sendDispl[q]);
    }
    MPI_Alltoallv(Data(sendBuffer), sendCounts.data(), sendDispl.data(), MPI_BYTE, Data(recvBuffer),
                  recvCounts.data(), recvDispl.data(), MPI_BYTE, comm);
    for (int p = 0; p < size; ++p) {
      if (recvCounts[p] > 0)
        std::copy_n(recvBuffer.data() + recvDispl[p], recvCounts[p], result.data() + recvOffset[p] + r * recvChunk[p]);
    }
  }
#endif
  return result;
}

std::vector<char> CPassiveComm::GathervRounds(const char* send, size_t nBytes, int root,
                                              std::vector<size_t>* recvBytes) {
  const int size = SU2_MPI::GetSize(), rank = SU2_MPI::GetRank();
  const bool isRoot = (rank == root);

#ifdef HAVE_MPI
  /*--- Byte counts on the root. ---*/
  uint64_t count64 = nBytes;
  std::vector<uint64_t> counts64(isRoot ? size : 0);
  MPI_Gather(&count64, sizeof(uint64_t), MPI_BYTE, Data(counts64), sizeof(uint64_t), MPI_BYTE, root, Comm());
  std::vector<size_t> counts(counts64.begin(), counts64.end());
#else
  std::vector<size_t> counts(1, nBytes);
#endif
  const auto offsets = Offsets(counts);
  std::vector<char> result(isRoot ? offsets[counts.size()] : 0);
  if (isRoot && nBytes > 0) std::copy_n(send, nBytes, result.data() + offsets[rank]);
  if (recvBytes != nullptr) *recvBytes = isRoot ? counts : std::vector<size_t>();
  lastRounds = 0;
  lastPeakRoundBytes = 0;

#ifdef HAVE_MPI
  /*--- Rounds: the sends of every rank and the aggregate receive of the root fit one round. ---*/
  const size_t planning = PlanningBytes(roundBytes, size);
  const size_t ownSend = isRoot ? 0 : nBytes;
  const size_t rootRecv = isRoot ? offsets[size] - nBytes : 0;
  const unsigned long nRound = AllreduceMax(std::max(CeilDiv(ownSend, planning), CeilDiv(rootRecv, planning)));
  lastRounds = nRound;

  const size_t sendChunk = (isRoot || nRound == 0) ? 0 : CeilDiv(nBytes, nRound);
  std::vector<size_t> recvChunk(isRoot ? size : 0, 0);
  for (int p = 0; isRoot && nRound > 0 && p < size; ++p) recvChunk[p] = (p == root) ? 0 : CeilDiv(counts[p], nRound);

  std::vector<int> recvCounts(isRoot ? size : 0), recvDispl(isRoot ? size : 0);
  for (unsigned long r = 0; r < nRound; ++r) {
    const size_t s = isRoot ? 0 : ChunkOfRound(nBytes, sendChunk, r);
    std::vector<char> sendBuffer(send + std::min(nBytes, r * sendChunk), send + std::min(nBytes, r * sendChunk) + s);
    size_t nRecv = 0;
    for (int p = 0; isRoot && p < size; ++p) {
      const size_t v = (p == root) ? 0 : ChunkOfRound(counts[p], recvChunk[p], r);
      recvCounts[p] = CheckedInt(v, "Receive count of a round");
      recvDispl[p] = CheckedInt(nRecv, "Receive displacement of a round");
      nRecv += v;
    }
    lastPeakRoundBytes = std::max({lastPeakRoundBytes, s, nRecv});
    std::vector<char> recvBuffer(CheckedInt(nRecv, "Receive bytes of a round"));
    MPI_Gatherv(Data(sendBuffer), CheckedInt(s, "Send count of a round"), MPI_BYTE, Data(recvBuffer),
                Data(recvCounts), Data(recvDispl), MPI_BYTE, root, Comm());
    for (int p = 0; isRoot && p < size; ++p) {
      if (recvCounts[p] > 0)
        std::copy_n(recvBuffer.data() + recvDispl[p], recvCounts[p], result.data() + offsets[p] + r * recvChunk[p]);
    }
  }
#endif
  return result;
}

std::vector<char> CPassiveComm::AllgathervRounds(const char* send, size_t nBytes, std::vector<size_t>* recvBytes, Communicator comm) {
  const int size = Size(comm);
#ifdef HAVE_MPI
  uint64_t count64 = nBytes;
  std::vector<uint64_t> counts64(size);
  MPI_Allgather(&count64, sizeof(uint64_t), MPI_BYTE, counts64.data(), sizeof(uint64_t), MPI_BYTE, comm);
  std::vector<size_t> counts(counts64.begin(), counts64.end());
#else
  std::vector<size_t> counts(1, nBytes);
#endif
  const auto offsets = Offsets(counts);
  std::vector<char> result(offsets[size]);
  if (recvBytes != nullptr) *recvBytes = counts;
  lastRounds = 0;
  lastPeakRoundBytes = 0;

#ifdef HAVE_MPI
  /*--- Every rank receives every stream (its own included, through MPI): the rounds follow from the total, which all
   *    ranks know. ---*/
  const size_t planning = PlanningBytes(roundBytes, size);
  const size_t nRound = CeilDiv(offsets[size], planning);
  lastRounds = nRound;
  std::vector<size_t> chunk(size, 0);
  for (int p = 0; nRound > 0 && p < size; ++p) chunk[p] = CeilDiv(counts[p], nRound);
  const int rank = Rank(comm);

  std::vector<int> recvCounts(size), recvDispl(size);
  for (size_t r = 0; r < nRound; ++r) {
    size_t nRecv = 0;
    for (int p = 0; p < size; ++p) {
      const size_t v = ChunkOfRound(counts[p], chunk[p], r);
      recvCounts[p] = CheckedInt(v, "Receive count of a round");
      recvDispl[p] = CheckedInt(nRecv, "Receive displacement of a round");
      nRecv += v;
    }
    const size_t start = std::min(nBytes, r * chunk[rank]);
    std::vector<char> sendBuffer(recvCounts[rank]);
    if (recvCounts[rank] > 0) std::copy_n(send + start, recvCounts[rank], sendBuffer.data());
    lastPeakRoundBytes = std::max(lastPeakRoundBytes, nRecv);
    std::vector<char> recvBuffer(CheckedInt(nRecv, "Receive bytes of a round"));
    MPI_Allgatherv(Data(sendBuffer), recvCounts[rank], MPI_BYTE, Data(recvBuffer), recvCounts.data(),
                   recvDispl.data(), MPI_BYTE, comm);
    for (int p = 0; p < size; ++p) {
      if (recvCounts[p] > 0)
        std::copy_n(recvBuffer.data() + recvDispl[p], recvCounts[p], result.data() + offsets[p] + r * chunk[p]);
    }
  }
#else
  if (nBytes > 0) std::copy_n(send, nBytes, result.data());
#endif
  return result;
}

void CPassiveComm::BcastRounds(std::vector<char>& data, int root, Communicator comm) {
#ifdef HAVE_MPI
  uint64_t n = data.size();
  MPI_Bcast(&n, sizeof(uint64_t), MPI_BYTE, root, comm);
  data.resize(n);
  for (size_t start = 0; start < n; start += roundBytes) {
    const size_t chunk = std::min(roundBytes, n - start);
    MPI_Bcast(data.data() + start, CheckedInt(chunk, "Broadcast chunk"), MPI_BYTE, root, comm);
  }
#endif
}

void CPassiveComm::SendRounds(const char* data, size_t nBytes, int dest, int tag) {
#ifdef HAVE_MPI
  if (dest == SU2_MPI::GetRank()) SU2_MPI::Error("SendRounds to this rank.", CURRENT_FUNCTION);
  uint64_t n = nBytes;
  MPI_Send(&n, sizeof(uint64_t), MPI_BYTE, dest, tag, Comm());
  for (size_t start = 0; start < nBytes; start += roundBytes) {
    const size_t chunk = std::min(roundBytes, nBytes - start);
    MPI_Send(const_cast<char*>(data) + start, CheckedInt(chunk, "Message chunk"), MPI_BYTE, dest, tag, Comm());
  }
#else
  SU2_MPI::Error("Point-to-point messages need MPI.", CURRENT_FUNCTION);
#endif
}

std::vector<char> CPassiveComm::RecvRounds(int source, int tag) {
  std::vector<char> data;
#ifdef HAVE_MPI
  if (source == SU2_MPI::GetRank()) SU2_MPI::Error("RecvRounds from this rank.", CURRENT_FUNCTION);
  uint64_t n = 0;
  MPI_Recv(&n, sizeof(uint64_t), MPI_BYTE, source, tag, Comm(), MPI_STATUS_IGNORE);
  data.resize(n);
  for (size_t start = 0; start < n; start += roundBytes) {
    const size_t chunk = std::min(roundBytes, static_cast<size_t>(n) - start);
    MPI_Recv(data.data() + start, CheckedInt(chunk, "Message chunk"), MPI_BYTE, source, tag, Comm(),
             MPI_STATUS_IGNORE);
  }
#else
  SU2_MPI::Error("Point-to-point messages need MPI.", CURRENT_FUNCTION);
#endif
  return data;
}

void CPassiveComm::Isend(const void* data, size_t nBytes, int dest, int tag, Request* request) {
#ifdef HAVE_MPI
  MPI_Isend(const_cast<void*>(data), CheckedInt(nBytes, "Isend count"), MPI_BYTE, dest, tag, Comm(), request);
#else
  SU2_MPI::Error("Point-to-point messages need MPI.", CURRENT_FUNCTION);
#endif
}

void CPassiveComm::Irecv(void* data, size_t nBytes, int source, int tag, Request* request) {
#ifdef HAVE_MPI
  MPI_Irecv(data, CheckedInt(nBytes, "Irecv count"), MPI_BYTE, source, tag, Comm(), request);
#else
  SU2_MPI::Error("Point-to-point messages need MPI.", CURRENT_FUNCTION);
#endif
}

void CPassiveComm::Waitall(std::vector<Request>& requests) {
#ifdef HAVE_MPI
  if (!requests.empty())
    MPI_Waitall(CheckedInt(requests.size(), "Number of requests"), requests.data(), MPI_STATUSES_IGNORE);
#endif
}

void CPassiveComm::ExchangeHalo(const CGeometry& geometry, void* records, size_t recordBytes) {
  const int nSend = geometry.nP2PSend, nRecv = geometry.nP2PRecv;
  if ((nSend == 0 && nRecv == 0) || recordBytes == 0) return;
#ifdef HAVE_MPI
  constexpr int tag = 5840;
  auto* bytes = static_cast<char*>(records);

  std::vector<char> sendBuffer(geometry.nPoint_P2PSend[nSend] * recordBytes);
  std::vector<char> recvBuffer(geometry.nPoint_P2PRecv[nRecv] * recordBytes);
  std::vector<Request> requests(nSend + nRecv);

  for (int iRecv = 0; iRecv < nRecv; ++iRecv) {
    const size_t offset = geometry.nPoint_P2PRecv[iRecv], end = geometry.nPoint_P2PRecv[iRecv + 1];
    Irecv(recvBuffer.data() + offset * recordBytes, (end - offset) * recordBytes, geometry.Neighbors_P2PRecv[iRecv],
          tag, &requests[iRecv]);
  }
  for (int iSend = 0; iSend < nSend; ++iSend) {
    const size_t offset = geometry.nPoint_P2PSend[iSend], end = geometry.nPoint_P2PSend[iSend + 1];
    for (size_t i = offset; i < end; ++i) {
      const auto iPoint = geometry.Local_Point_P2PSend[i];
      std::memcpy(sendBuffer.data() + i * recordBytes, bytes + iPoint * recordBytes, recordBytes);
    }
    Isend(sendBuffer.data() + offset * recordBytes, (end - offset) * recordBytes, geometry.Neighbors_P2PSend[iSend],
          tag, &requests[nRecv + iSend]);
  }
  Waitall(requests);

  for (size_t i = 0; i < static_cast<size_t>(geometry.nPoint_P2PRecv[nRecv]); ++i) {
    const auto iPoint = geometry.Local_Point_P2PRecv[i];
    std::memcpy(bytes + iPoint * recordBytes, recvBuffer.data() + i * recordBytes, recordBytes);
  }
#else
  SU2_MPI::Error("A geometry with halo points needs MPI.", CURRENT_FUNCTION);
#endif
}

unsigned long CPassiveComm::ExscanSum(unsigned long value, Communicator comm) {
#ifdef HAVE_MPI
  unsigned long result = 0;
  MPI_Exscan(&value, &result, 1, MPI_UNSIGNED_LONG, MPI_SUM, comm);
  /*--- MPI_Exscan leaves the result of rank 0 undefined. ---*/
  if (Rank(comm) == 0) result = 0;
  return result;
#else
  return 0;
#endif
}

unsigned long CPassiveComm::AllreduceMax(unsigned long value, Communicator comm) {
#ifdef HAVE_MPI
  unsigned long result = 0;
  MPI_Allreduce(&value, &result, 1, MPI_UNSIGNED_LONG, MPI_MAX, comm);
  return result;
#else
  return value;
#endif
}

unsigned long CPassiveComm::AllreduceSum(unsigned long value) {
#ifdef HAVE_MPI
  unsigned long result = 0;
  MPI_Allreduce(&value, &result, 1, MPI_UNSIGNED_LONG, MPI_SUM, Comm());
  return result;
#else
  return value;
#endif
}

unsigned long CPassiveComm::AllreduceMin(unsigned long value, Communicator comm) {
#ifdef HAVE_MPI
  unsigned long result = 0;
  MPI_Allreduce(&value, &result, 1, MPI_UNSIGNED_LONG, MPI_MIN, comm);
  return result;
#else
  return value;
#endif
}
