/*!
 * \file CPassiveComm.hpp
 * \brief Transport of passive (not differentiated) data as packed bytes with the native MPI functions, in rounds of
 *        bounded size, and checked conversions of sizes to int.
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

#include <climits>
#include <limits>
#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#include "mpi_structure.hpp"

class CGeometry;

/*!
 * \brief Convert a size or an index to int (MPI counts and displacements, MMG indices), with an error that names the
 *        quantity if it does not fit.
 * \param[in] value - Value to convert (any integer type).
 * \param[in] quantity - What the value is, for the error message.
 */
template <class T>
int CheckedInt(T value, const char* quantity) {
  static_assert(std::is_integral<T>::value, "CheckedInt converts integers only.");
  bool fits = true;
  if constexpr (std::is_signed<T>::value) {
    fits = static_cast<long long>(value) >= static_cast<long long>(INT_MIN) &&
           static_cast<long long>(value) <= static_cast<long long>(INT_MAX);
  } else {
    fits = static_cast<unsigned long long>(value) <= static_cast<unsigned long long>(INT_MAX);
  }
  if (!fits) {
    SU2_MPI::Error(std::string(quantity) + " (" + std::to_string(value) + ") does not fit in an int (MPI and MMG " +
                       "counts are int).",
                   CURRENT_FUNCTION);
  }
  return static_cast<int>(value);
}

/*--- Passive doubles are moved as bytes or as the library's double (NativeMpiDouble): IEEE binary64 is assumed. ---*/
static_assert(std::numeric_limits<double>::is_iec559 && std::numeric_limits<double>::digits == 53,
              "CPassiveComm assumes IEEE binary64 doubles.");

/*!
 * \class CPassiveComm
 * \brief Collective and point-to-point transport of passive data (coordinates, metrics, indices, flags) packed as bytes.
 * \note The calls are the native MPI functions with MPI_BYTE on the supplied raw communicator (default SU2_MPI::GetComm()) (an MPI_Comm
 *       in every MPI build) and native MPI_Request handles. They do not go through the SU2_MPI wrapper: in the
 *       reverse (AD) and forward (DD) builds that is the MeDiPack wrapper, which has no conversion for MPI_BYTE and
 *       treats MPI_DOUBLE as the active type. Data sent here never carry derivatives (passivedouble, integers,
 *       characters; the typed helpers reject su2double in the AD builds). A message pair is always native on both
 *       sides. Without MPI there is one rank: the collectives copy locally and point-to-point calls are an error.
 *
 *       Size limits: every MPI count and displacement is an int. The collectives therefore run in rounds that bound,
 *       per round, the aggregate bytes each rank sends and the aggregate bytes each rank receives by the round size
 *       (GetRoundBytes, 1 GiB by default). Each round packs a fresh send buffer and receives into a fresh buffer that
 *       is copied to its place in the result. All ranks compute the same number of rounds (the largest need of any
 *       rank, MPI_Allreduce) and take part in every round, also with nothing to send or receive. The data of a rank
 *       for itself are copied directly (never through MPI). Messages are byte streams cut at any byte, so records of
 *       any size can be sent.
 *
 *       All collective calls must be made by every rank of the supplied communicator in the same order. The round size must
 *       be the same on all ranks (SetRoundBytes, for tests).
 *
 *       Used by the mesh adaptation (reader slices of the remeshed mesh, the metric of the new points) and meant as
 *       the one passive transport of the distributed adaptation (also the distributed solution transfers).
 */
class CPassiveComm {
 public:
  using Communicator = SU2_MPI::Comm;
  static int Rank(Communicator comm = SU2_MPI::GetComm()) {
#ifdef HAVE_MPI
    int rank = 0;
    MPI_Comm_rank(comm, &rank);
    return rank;
#else
    return 0;
#endif
  }
  static int Size(Communicator comm = SU2_MPI::GetComm()) {
#ifdef HAVE_MPI
    int size = 1;
    MPI_Comm_size(comm, &size);
    return size;
#else
    return 1;
#endif
  }
#ifdef HAVE_MPI
  using Request = MPI_Request;
#else
  using Request = int;
#endif

  static constexpr size_t DEFAULT_ROUND_BYTES = size_t(1) << 30; /*!< \brief Default round size, 1 GiB < INT_MAX. */

  /*! \brief Largest aggregate number of bytes a rank sends or receives in one round of a collective. */
  static size_t GetRoundBytes() { return roundBytes; }

  /*!
   * \brief Set the round size (all ranks the same value; tests force small rounds). Must not exceed INT_MAX.
   */
  static void SetRoundBytes(size_t bytes);

  /*!
   * \brief Statistics of the last AlltoallvRounds, GathervRounds or AllgathervRounds of this rank (for tests): the
   *        number of rounds and the largest aggregate number of bytes this rank sent or received through MPI in one
   *        round.
   */
  static unsigned long GetLastRounds() { return lastRounds; }
  static size_t GetLastPeakRoundBytes() { return lastPeakRoundBytes; }

  /*!
   * \brief Collective: every rank sends blockBytes bytes to every rank (block q of send to rank q) and receives
   *        blockBytes bytes from every rank (block p of recv from rank p). blockBytes must be the same on all ranks.
   */
  static void Alltoall(const void* send, void* recv, size_t blockBytes, Communicator comm = SU2_MPI::GetComm());

  /*!
   * \brief Collective: personalized exchange of byte streams in rounds.
   * \param[in] send - The bytes for all ranks, those for rank q at offset sum(sendBytes[0..q-1]).
   * \param[in] sendBytes - Number of bytes for each rank (size = number of ranks).
   * \param[out] recvBytes - Number of bytes received from each rank.
   * \return The bytes received, those from rank p at offset sum(recvBytes[0..p-1]).
   */
  static std::vector<char> AlltoallvRounds(const char* send, const std::vector<size_t>& sendBytes,
                                           std::vector<size_t>& recvBytes, Communicator comm = SU2_MPI::GetComm());

  /*!
   * \brief Collective: the bytes of every rank on the root, in rank order, in rounds.
   * \param[out] recvBytes - Root: number of bytes received from each rank (may be nullptr).
   * \return Root: the bytes of all ranks; other ranks: empty.
   */
  static std::vector<char> GathervRounds(const char* send, size_t nBytes, int root, std::vector<size_t>* recvBytes);

  /*!
   * \brief Collective: the bytes of every rank on every rank, in rank order, in rounds.
   */
  static std::vector<char> AllgathervRounds(const char* send, size_t nBytes, std::vector<size_t>* recvBytes);

  /*!
   * \brief Collective: data of the root copied to all ranks (size included), in rounds.
   */
  static void BcastRounds(std::vector<char>& data, int root, Communicator comm = SU2_MPI::GetComm());

  /*!
   * \brief Point-to-point: send nBytes bytes to rank dest (not this rank), as the size followed by chunks of at most
   *        the round size. Blocking; matched by RecvRounds on dest with the same tag.
   */
  static void SendRounds(const char* data, size_t nBytes, int dest, int tag);

  /*!
   * \brief Point-to-point: receive what SendRounds of rank source (not this rank) sent with this tag.
   */
  static std::vector<char> RecvRounds(int source, int tag);

  /*!
   * \brief Non-blocking point-to-point (halo exchanges): native requests, count checked to fit an int.
   */
  static void Isend(const void* data, size_t nBytes, int dest, int tag, Request* request);
  static void Irecv(void* data, size_t nBytes, int source, int tag, Request* request);
  static void Waitall(std::vector<Request>& requests);

  /*!
   * \brief Collective: exchange of packed per-point records with the halo points of a partitioned geometry (its P2P
   *        send and receive lists, as CGeometry::InitiateComms): the record of every halo point is replaced by the
   *        record of the rank that owns the point.
   * \param[in] geometry - Partitioned geometry (any multigrid level).
   * \param[in,out] records - recordBytes bytes per local point (records + iPoint * recordBytes).
   * \param[in] recordBytes - Size of one record (the same on all ranks).
   */
  static void ExchangeHalo(const CGeometry& geometry, void* records, size_t recordBytes);

  /*! \brief Collective: exclusive prefix sum over the ranks; rank 0 gets 0 (MPI_Exscan leaves it undefined). */
  static unsigned long ExscanSum(unsigned long value, Communicator comm = SU2_MPI::GetComm());

  /*! \brief Collective reductions of integers over the ranks. */
  static unsigned long AllreduceMax(unsigned long value, Communicator comm = SU2_MPI::GetComm());
  static unsigned long AllreduceSum(unsigned long value);
  static unsigned long AllreduceMin(unsigned long value, Communicator comm = SU2_MPI::GetComm());

  /*! \brief Reduction operations of Allreduce. */
  enum class Op { SUM, MIN, MAX };

  /*!
   * \brief Collective: element-wise reduction of n passive scalars over the ranks with the native MPI type of T
   *        (integers, characters, double, float); never through MeDiPack.
   */
  template <class T>
  static void Allreduce(const T* send, T* recv, size_t n, Op op, Communicator comm = SU2_MPI::GetComm()) {
    CheckPassive<T>();
#ifdef HAVE_MPI
    MPI_Allreduce(const_cast<T*>(send), recv, CheckedInt(n, "Allreduce count"), NativeType<T>(), NativeOp(op),
                  comm);
#else
    if (n > 0) std::memcpy(recv, send, n * sizeof(T));
#endif
  }

  /*! \brief Allreduce of one value. */
  template <class T>
  static T Allreduce(T value, Op op, Communicator comm = SU2_MPI::GetComm()) {
    T result = value;
    Allreduce(&value, &result, 1, op, comm);
    return result;
  }

  /*!
   * \brief Typed AlltoallvRounds: counts in elements of T (trivially copyable passive data).
   */
  template <class T>
  static std::vector<T> Alltoallv(const std::vector<T>& send, const std::vector<size_t>& sendCount,
                                  std::vector<size_t>& recvCount) {
    CheckPassive<T>();
    std::vector<size_t> sendBytes(sendCount.size()), recvBytes;
    size_t total = 0;
    for (size_t q = 0; q < sendCount.size(); ++q) {
      sendBytes[q] = sendCount[q] * sizeof(T);
      total += sendCount[q];
    }
    if (total != send.size()) SU2_MPI::Error("The send counts do not add up to the send buffer.", CURRENT_FUNCTION);
    const auto bytes = AlltoallvRounds(reinterpret_cast<const char*>(send.data()), sendBytes, recvBytes);
    recvCount.resize(recvBytes.size());
    for (size_t p = 0; p < recvBytes.size(); ++p) recvCount[p] = recvBytes[p] / sizeof(T);
    return FromBytes<T>(bytes);
  }

  /*!
   * \brief Typed GathervRounds (counts in elements of T).
   */
  template <class T>
  static std::vector<T> Gatherv(const std::vector<T>& send, int root, std::vector<size_t>* recvCount) {
    CheckPassive<T>();
    std::vector<size_t> recvBytes;
    const auto bytes = GathervRounds(reinterpret_cast<const char*>(send.data()), send.size() * sizeof(T), root,
                                     &recvBytes);
    if (recvCount != nullptr) {
      recvCount->resize(recvBytes.size());
      for (size_t p = 0; p < recvBytes.size(); ++p) (*recvCount)[p] = recvBytes[p] / sizeof(T);
    }
    return FromBytes<T>(bytes);
  }

  /*!
   * \brief Typed AllgathervRounds (counts in elements of T).
   */
  template <class T>
  static std::vector<T> Allgatherv(const std::vector<T>& send, std::vector<size_t>* recvCount) {
    CheckPassive<T>();
    std::vector<size_t> recvBytes;
    const auto bytes = AllgathervRounds(reinterpret_cast<const char*>(send.data()), send.size() * sizeof(T),
                                        &recvBytes);
    if (recvCount != nullptr) {
      recvCount->resize(recvBytes.size());
      for (size_t p = 0; p < recvBytes.size(); ++p) (*recvCount)[p] = recvBytes[p] / sizeof(T);
    }
    return FromBytes<T>(bytes);
  }

  /*!
   * \brief Typed BcastRounds.
   */
  template <class T>
  static void Bcast(std::vector<T>& data, int root, Communicator comm = SU2_MPI::GetComm()) {
    CheckPassive<T>();
    std::vector<char> bytes;
    if (Rank(comm) == root) bytes.assign(reinterpret_cast<const char*>(data.data()),
                                                 reinterpret_cast<const char*>(data.data()) + data.size() * sizeof(T));
    BcastRounds(bytes, root, comm);
    data = FromBytes<T>(bytes);
  }

  /*!
   * \brief Copy bytes into a vector of T (the size must be a multiple of sizeof(T)).
   */
  template <class T>
  static std::vector<T> FromBytes(const std::vector<char>& bytes) {
    CheckPassive<T>();
    if (bytes.size() % sizeof(T) != 0) SU2_MPI::Error("Received a partial record.", CURRENT_FUNCTION);
    std::vector<T> values(bytes.size() / sizeof(T));
    if (!values.empty()) std::memcpy(values.data(), bytes.data(), bytes.size());
    return values;
  }

 private:
  static size_t roundBytes; /*!< \brief Current round size. */
  static unsigned long lastRounds;  /*!< \brief Rounds of the last collective in rounds. */
  static size_t lastPeakRoundBytes; /*!< \brief Largest aggregate bytes of a round of the last collective. */

#ifdef HAVE_MPI
  /*--- Native MPI type of a passive scalar. ---*/
  template <class T>
  static MPI_Datatype NativeType() {
    if constexpr (std::is_same<T, double>::value) {
      /*--- The library's double, also in single precision builds (where MPI_DOUBLE is redefined as MPI_FLOAT). ---*/
      return NativeMpiDouble();
    } else if constexpr (std::is_same<T, float>::value) return MPI_FLOAT;
    else if constexpr (std::is_same<T, int>::value) return MPI_INT;
    else if constexpr (std::is_same<T, unsigned int>::value) return MPI_UNSIGNED;
    else if constexpr (std::is_same<T, long>::value) return MPI_LONG;
    else if constexpr (std::is_same<T, unsigned long>::value) return MPI_UNSIGNED_LONG;
    else if constexpr (std::is_same<T, long long>::value) return MPI_LONG_LONG;
    else if constexpr (std::is_same<T, unsigned long long>::value) return MPI_UNSIGNED_LONG_LONG;
    else if constexpr (std::is_same<T, short>::value) return MPI_SHORT;
    else if constexpr (std::is_same<T, unsigned short>::value) return MPI_UNSIGNED_SHORT;
    else if constexpr (std::is_same<T, char>::value) return MPI_CHAR;
    else if constexpr (std::is_same<T, signed char>::value) return MPI_SIGNED_CHAR;
    else if constexpr (std::is_same<T, unsigned char>::value) return MPI_UNSIGNED_CHAR;
    else static_assert(sizeof(T) == 0, "CPassiveComm::Allreduce: no native MPI type for this type.");
  }
  static MPI_Op NativeOp(Op op) { return op == Op::SUM ? MPI_SUM : (op == Op::MIN ? MPI_MIN : MPI_MAX); }
#endif

  template <class T>
  static void CheckPassive() {
    static_assert(std::is_trivially_copyable<T>::value, "CPassiveComm sends trivially copyable data only.");
    static_assert(std::is_same<su2double, passivedouble>::value || !std::is_same<T, su2double>::value,
                  "CPassiveComm sends passive data only (su2double is active in this build).");
  }
};
