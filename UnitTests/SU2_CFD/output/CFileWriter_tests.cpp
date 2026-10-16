/*!
 * \file CFileWriter_tests.cpp
 * \brief Tests for collective file writes with empty partitions.
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
#include "../../../SU2_CFD/include/output/filewriter/CFileWriter.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace {
class CollectiveTestWriter : public CFileWriter {
 public:
  CollectiveTestWriter() : CFileWriter(".bin") {}
  using CFileWriter::OpenMPIFile;
  using CFileWriter::WriteMPIBinaryDataAll;
  using CFileWriter::GetRankOffset;
  using CFileWriter::CloseMPIFile;
};
}  // namespace

TEST_CASE("Collective file writes preserve empty ranks and byte offsets", "[FileWriter]") {
  const auto rank = SU2_MPI::GetRank();
  const auto ranks = SU2_MPI::GetSize();
  const unsigned long blockBytes = (1ul << 20) + 17;
  const int owners[] = {-1, 0, ranks - 1, -2};
  unsigned scenario = 0;
  for (const auto owner : owners) {
    CAPTURE(owner, rank, ranks);
    unsigned long long stamp = 0;
    if (rank == MASTER_NODE) stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    SU2_MPI::Bcast(&stamp, 1, MPI_UNSIGNED_LONG_LONG, MASTER_NODE, SU2_MPI::GetComm());
    const std::string prefix = "collective_file_writer_" + std::to_string(stamp) + "_" + std::to_string(scenario++);
    const bool active = owner == -1 || owner == rank;
    const unsigned long localBytes = active ? blockBytes + rank : 0;
    std::vector<unsigned char> data(localBytes, static_cast<unsigned char>(0x40 + rank % 128));
    CollectiveTestWriter writer;
    const auto offset = writer.GetRankOffset(localBytes);
    REQUIRE(writer.OpenMPIFile(prefix));

    /*--- Empty ranks must join the same collective calls as ranks with surface data. ---*/
    const bool written = writer.WriteMPIBinaryDataAll(data.data(), localBytes, offset.second, offset.first);
    const bool closed = writer.CloseMPIFile();
    int valid = written && closed, allValid = 0;
    SU2_MPI::Allreduce(&valid, &allValid, 1, MPI_INT, MPI_MIN, SU2_MPI::GetComm());
    REQUIRE(allValid == 1);
    CHECK(SU2_TYPE::GetValue(writer.GetFilesize()) == static_cast<double>(offset.second));

    if (rank == MASTER_NODE) {
      std::ifstream input(prefix + ".bin", std::ios::binary);
      valid = input.good();
      std::vector<unsigned char> actual((std::istreambuf_iterator<char>(input)), {});
      unsigned long cursor = 0;
      for (auto producer = 0; producer < ranks; ++producer) {
        const unsigned long bytes = owner == -1 || owner == producer ? blockBytes + producer : 0;
        if (cursor + bytes > actual.size()) {
          valid = 0;
          break;
        }
        for (auto i = 0ul; i < bytes; ++i)
          if (actual[cursor + i] != static_cast<unsigned char>(0x40 + producer % 128)) valid = 0;
        cursor += bytes;
      }
      if (cursor != actual.size()) valid = 0;
    }
    SU2_MPI::Bcast(&valid, 1, MPI_INT, MASTER_NODE, SU2_MPI::GetComm());
    REQUIRE(valid == 1);
    if (rank == MASTER_NODE) std::filesystem::remove(prefix + ".bin");
  }
}
