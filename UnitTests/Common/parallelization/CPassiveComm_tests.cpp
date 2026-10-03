/*!
 * \file CPassiveComm_tests.cpp
 * \brief Passive transport (native MPI with MPI_BYTE, rounds of bounded size): every call with uneven sizes, an empty
 *        rank in the middle of the communicator, forced small rounds, MPI_COMM_SELF; the checked conversions. Built in
 *        the normal, reverse (AD) and forward (DD) test drivers; valid with any number of ranks, e.g.
 *        mpirun -n 3 test_driver_AD "[PassiveComm]".
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

#include "catch.hpp"

#include <algorithm>
#include <climits>
#include <cstdint>

#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../SU2_CFD/adaptation/TransferTestCase.hpp"

namespace {

/*--- Largest value over the ranks (native, independent of the transport under test). ---*/
unsigned long WorstOverRanks(bool ok) {
  unsigned long bad = ok ? 0 : 1, worst = 0;
#ifdef HAVE_MPI
  MPI_Allreduce(&bad, &worst, 1, MPI_UNSIGNED_LONG, MPI_MAX, SU2_MPI::GetComm());
#else
  worst = bad;
#endif
  return worst;
}

/*--- The middle rank of three or more sends and receives nothing. ---*/
bool IsEmptyRank(int rank, int size) { return size >= 3 && rank == size / 2; }

/*--- Bytes from p to q (uneven; none from or to the empty rank; a self message on every other rank). ---*/
size_t PairBytes(int p, int q, int size) {
  if (IsEmptyRank(p, size) || IsEmptyRank(q, size)) return 0;
  return 150 + 37 * ((p * 7 + q * 13) % 5) + (p == q ? 11 : 0);
}
char PairByte(int p, int q, size_t k) { return static_cast<char>((p * 31 + q * 17 + k * 7) % 251); }

/*--- Bytes of rank p for the gathers. ---*/
size_t RankBytes(int p, int size) { return IsEmptyRank(p, size) ? 0 : 300 + 53 * p; }
char RankByte(int p, size_t k) { return static_cast<char>((p * 41 + k * 3) % 127); }

/*--- Run f with the round size bytes (all ranks), then restore the default. ---*/
template <class F>
void WithRoundBytes(size_t bytes, F f) {
  const auto saved = CPassiveComm::GetRoundBytes();
  CPassiveComm::SetRoundBytes(bytes);
  f();
  CPassiveComm::SetRoundBytes(saved);
}

}  // namespace

TEST_CASE("Passive transport: checked conversions", "[AdaptationMPI][PassiveComm]") {
  CHECK(CheckedInt(size_t(INT_MAX), "test") == INT_MAX);
  CHECK(CheckedInt(0ul, "test") == 0);
  CHECK(CheckedInt(-5l, "test") == -5);
  CHECK(CheckedInt(static_cast<long long>(INT_MIN), "test") == INT_MIN);
  CHECK(CheckedInt(static_cast<unsigned short>(65535), "test") == 65535);
}

TEST_CASE("Passive transport: all-to-all in rounds", "[AdaptationMPI][PassiveComm]") {
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
  /*--- Default rounds (one round), and rounds of 200 bytes: several peers whose aggregate exceeds a round. ---*/
  for (const size_t roundBytes : {CPassiveComm::DEFAULT_ROUND_BYTES, size_t(200)}) {
    SECTION("round size " + std::to_string(roundBytes)) {
      WithRoundBytes(roundBytes, [&]() {
        std::vector<size_t> sendBytes(size), recvBytes;
        std::vector<char> send;
        for (int q = 0; q < size; ++q) {
          sendBytes[q] = PairBytes(rank, q, size);
          for (size_t k = 0; k < sendBytes[q]; ++k) send.push_back(PairByte(rank, q, k));
        }
        const auto recv = CPassiveComm::AlltoallvRounds(send.data(), sendBytes, recvBytes);

        bool ok = recvBytes.size() == static_cast<size_t>(size);
        size_t offset = 0, others = 0;
        for (int p = 0; ok && p < size; ++p) {
          ok = recvBytes[p] == PairBytes(p, rank, size);
          for (size_t k = 0; ok && k < recvBytes[p]; ++k) ok = recv[offset + k] == PairByte(p, rank, k);
          offset += recvBytes[p];
          if (p != rank) others += recvBytes[p] + sendBytes[p];
        }
        ok = ok && offset == recv.size();
        CHECK(WorstOverRanks(ok) == 0);

        /*--- Every round within the bound; several rounds when the data exceed one (also on the empty rank). ---*/
        CHECK(CPassiveComm::GetLastPeakRoundBytes() <= roundBytes);
        if (roundBytes == 200 && size > 1) CHECK(CPassiveComm::GetLastRounds() > 1);
        if (size == 1) CHECK(CPassiveComm::GetLastRounds() == 0);
        (void)others;

        /*--- Typed: passive doubles and 64-bit integers. ---*/
        std::vector<size_t> counts(size), recvCount;
        std::vector<passivedouble> values;
        std::vector<uint64_t> ids;
        for (int q = 0; q < size; ++q) {
          counts[q] = PairBytes(rank, q, size) / 8;
          for (size_t k = 0; k < counts[q]; ++k) {
            values.push_back(1.0 / (1 + rank) + q + 1e-3 * k);
            ids.push_back((uint64_t(rank) << 40) + (uint64_t(q) << 20) + k);
          }
        }
        const auto gotValues = CPassiveComm::Alltoallv(values, counts, recvCount);
        std::vector<size_t> idCount;
        const auto gotIds = CPassiveComm::Alltoallv(ids, counts, idCount);
        bool typed = (recvCount == idCount);
        size_t i = 0;
        for (int p = 0; typed && p < size; ++p) {
          typed = recvCount[p] == PairBytes(p, rank, size) / 8;
          for (size_t k = 0; typed && k < recvCount[p]; ++k, ++i) {
            typed = gotValues[i] == 1.0 / (1 + p) + rank + 1e-3 * k &&
                    gotIds[i] == (uint64_t(p) << 40) + (uint64_t(rank) << 20) + k;
          }
        }
        CHECK(WorstOverRanks(typed && i == gotValues.size()) == 0);
      });
    }
  }
}

TEST_CASE("Passive transport: gathers and broadcast in rounds", "[AdaptationMPI][PassiveComm]") {
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();
  for (const size_t roundBytes : {CPassiveComm::DEFAULT_ROUND_BYTES, size_t(256)}) {
    SECTION("round size " + std::to_string(roundBytes)) {
      WithRoundBytes(roundBytes, [&]() {
        std::vector<char> mine(RankBytes(rank, size));
        for (size_t k = 0; k < mine.size(); ++k) mine[k] = RankByte(rank, k);

        auto expectAll = [&](const std::vector<char>& all, const std::vector<size_t>& counts) {
          bool ok = counts.size() == static_cast<size_t>(size);
          size_t offset = 0;
          for (int p = 0; ok && p < size; ++p) {
            ok = counts[p] == RankBytes(p, size);
            for (size_t k = 0; ok && k < counts[p]; ++k) ok = all[offset + k] == RankByte(p, k);
            offset += counts[p];
          }
          return ok && offset == all.size();
        };

        /*--- Gatherv to the first and to the last rank (the root's aggregate receive exceeds a round). ---*/
        for (const int root : {0, size - 1}) {
          std::vector<size_t> counts;
          const auto all = CPassiveComm::GathervRounds(mine.data(), mine.size(), root, &counts);
          CHECK(WorstOverRanks(rank == root ? expectAll(all, counts) : (all.empty() && counts.empty())) == 0);
          CHECK(CPassiveComm::GetLastPeakRoundBytes() <= roundBytes);
          if (roundBytes == 256 && size > 1) CHECK(CPassiveComm::GetLastRounds() > 1);
        }

        /*--- Allgatherv. ---*/
        std::vector<size_t> counts;
        const auto all = CPassiveComm::AllgathervRounds(mine.data(), mine.size(), &counts);
        CHECK(WorstOverRanks(expectAll(all, counts)) == 0);
        CHECK(CPassiveComm::GetLastPeakRoundBytes() <= roundBytes);

        /*--- Typed gathers. ---*/
        std::vector<passivedouble> values(RankBytes(rank, size) / 8);
        for (size_t k = 0; k < values.size(); ++k) values[k] = rank + 0.25 * k;
        std::vector<size_t> valueCount;
        const auto gathered = CPassiveComm::Gatherv(values, 0, &valueCount);
        const auto everywhere = CPassiveComm::Allgatherv(values, nullptr);
        bool typed = true;
        size_t i = 0;
        for (int p = 0; p < size; ++p) {
          for (size_t k = 0; typed && k < RankBytes(p, size) / 8; ++k, ++i) {
            typed = everywhere[i] == p + 0.25 * k && (rank != 0 || gathered[i] == p + 0.25 * k);
          }
        }
        CHECK(WorstOverRanks(typed && i == everywhere.size()) == 0);

        /*--- Broadcast from the last rank, larger than a round. ---*/
        std::vector<char> data;
        if (rank == size - 1) data.assign(700, 'x');
        if (rank == size - 1) for (size_t k = 0; k < data.size(); ++k) data[k] = RankByte(size - 1, k);
        CPassiveComm::BcastRounds(data, size - 1);
        bool same = data.size() == 700;
        for (size_t k = 0; same && k < data.size(); ++k) same = data[k] == RankByte(size - 1, k);
        CHECK(WorstOverRanks(same) == 0);

        std::vector<unsigned long> ids;
        if (rank == 0) ids = {3, 1, 4, 1, 5, 9, 2, 6};
        CPassiveComm::Bcast(ids, 0);
        CHECK(WorstOverRanks(ids == std::vector<unsigned long>({3, 1, 4, 1, 5, 9, 2, 6})) == 0);
      });
    }
  }
}

TEST_CASE("Passive transport: point-to-point in rounds and reductions", "[AdaptationMPI][PassiveComm]") {
  const int rank = SU2_MPI::GetRank(), size = SU2_MPI::GetSize();

  /*--- Rank 0 sends every other rank a message of several rounds, which sends it back reversed. ---*/
  WithRoundBytes(100, [&]() {
    constexpr int tag = 77;
    bool ok = true;
    auto message = [](int q) {
      std::vector<char> m(250 + 10 * q);
      for (size_t k = 0; k < m.size(); ++k) m[k] = RankByte(q, k);
      return m;
    };
    if (size > 1) {
      if (rank == 0) {
        for (int q = 1; q < size; ++q) {
          const auto m = message(q);
          CPassiveComm::SendRounds(m.data(), m.size(), q, tag);
        }
        for (int q = 1; q < size; ++q) {
          auto back = CPassiveComm::RecvRounds(q, tag);
          std::reverse(back.begin(), back.end());
          ok &= back == message(q);
        }
      } else {
        auto m = CPassiveComm::RecvRounds(0, tag);
        ok &= m == message(rank);
        std::reverse(m.begin(), m.end());
        CPassiveComm::SendRounds(m.data(), m.size(), 0, tag);
      }
    }
    CHECK(WorstOverRanks(ok) == 0);
  });

  /*--- Exclusive prefix sum: 0 on rank 0. ---*/
  CHECK(CPassiveComm::ExscanSum(rank + 1ul) == static_cast<unsigned long>(rank) * (rank + 1) / 2);

  /*--- Reductions on native types. ---*/
  CHECK(CPassiveComm::AllreduceMax(rank + 3ul) == size + 2ul);
  CHECK(CPassiveComm::AllreduceMin(rank + 3ul) == 3ul);
  CHECK(CPassiveComm::AllreduceSum(1ul) == static_cast<unsigned long>(size));
  const passivedouble values[3] = {0.5 + rank, -1.0 * rank, 2.0};
  passivedouble sum[3], minimum[3], maximum[3];
  CPassiveComm::Allreduce(values, sum, 3, CPassiveComm::Op::SUM);
  CPassiveComm::Allreduce(values, minimum, 3, CPassiveComm::Op::MIN);
  CPassiveComm::Allreduce(values, maximum, 3, CPassiveComm::Op::MAX);
  CHECK(sum[0] == 0.5 * size + 0.5 * size * (size - 1));
  CHECK(sum[2] == 2.0 * size);
  CHECK(minimum[1] == -1.0 * (size - 1));
  CHECK(maximum[0] == 0.5 + (size - 1));
  CHECK(CPassiveComm::Allreduce(static_cast<int>(-rank), CPassiveComm::Op::MIN) == 1 - size);
  CHECK(CPassiveComm::Allreduce(static_cast<long>(rank), CPassiveComm::Op::SUM) == long(size) * (size - 1) / 2);
}

TEST_CASE("Passive transport: halo exchange of packed records", "[AdaptationMPI][PassiveComm]") {
  using namespace transfer_test;
  for (const unsigned short nDim : {2, 3}) {
    SECTION("nDim " + std::to_string(nDim)) {
      auto config = MakeConfig(nDim, "SOLVER= EULER\n");
      config->SetMGLevels(0);
      CGeometry** geometry = nullptr;
      {
        Mute mute;
        CMemoryMeshReaderFVM reader(config.get(), BoxMesh(nDim, 4, true), 0, 1);
        CDriver::BuildGeometryFVM(config.get(), new CPhysicalGeometry(config.get(), reader, 1), geometry, true);
      }
      const auto& fine = *geometry[MESH_0];

      /*--- Record: global index and the first coordinate; halo records start as garbage. ---*/
      struct Record {
        uint64_t id;
        passivedouble x;
        char flag;
      };
      std::vector<Record> records(fine.GetnPoint(), Record{UINT64_MAX, -1.0, 'h'});
      for (auto iPoint = 0ul; iPoint < fine.GetnPointDomain(); ++iPoint)
        records[iPoint] = {fine.nodes->GetGlobalIndex(iPoint), SU2_TYPE::GetValue(fine.nodes->GetCoord(iPoint, 0)), 'd'};
      CPassiveComm::ExchangeHalo(fine, records.data(), sizeof(Record));
      bool ok = true;
      for (auto iPoint = 0ul; iPoint < fine.GetnPoint(); ++iPoint) {
        ok &= records[iPoint].id == fine.nodes->GetGlobalIndex(iPoint) &&
              records[iPoint].x == SU2_TYPE::GetValue(fine.nodes->GetCoord(iPoint, 0)) && records[iPoint].flag == 'd';
      }
      CHECK(WorstOverRanks(ok) == 0);

      for (unsigned short iMesh = 0; iMesh <= config->GetnMGLevels(); ++iMesh) delete geometry[iMesh];
      delete[] geometry;
    }
  }
}

#ifdef HAVE_MPI
TEST_CASE("Passive transport: one rank alone (MPI_COMM_SELF)", "[AdaptationMPI][PassiveComm]") {
  const int worldRank = SU2_MPI::GetRank();
  const auto world = SU2_MPI::GetComm();
  SU2_MPI::SetComm(MPI_COMM_SELF);
  bool ok = SU2_MPI::GetSize() == 1;
  WithRoundBytes(64, [&]() {
    std::vector<char> send(500);
    for (size_t k = 0; k < send.size(); ++k) send[k] = RankByte(worldRank, k);
    std::vector<size_t> recvBytes;
    ok &= CPassiveComm::AlltoallvRounds(send.data(), {send.size()}, recvBytes) == send;
    ok &= recvBytes == std::vector<size_t>({send.size()}) && CPassiveComm::GetLastRounds() == 0;
    ok &= CPassiveComm::GathervRounds(send.data(), send.size(), 0, nullptr) == send;
    ok &= CPassiveComm::AllgathervRounds(send.data(), send.size(), nullptr) == send;
    auto copy = send;
    CPassiveComm::BcastRounds(copy, 0);
    ok &= copy == send;
    ok &= CPassiveComm::ExscanSum(5) == 0 && CPassiveComm::AllreduceSum(5) == 5;
  });
  SU2_MPI::SetComm(world);
  CHECK(WorstOverRanks(ok) == 0);
}
#endif
