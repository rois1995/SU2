/*!
 * \file CNativeDistributed2D_tests.cpp
 * \brief Versioned native incidence discovery, explicit wire fields and private publication controls.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeDistributed2D.hpp"

using namespace SU2NativeBoundary2D;

namespace {
std::vector<char> Bytes(Cell cell) {
  std::vector<char> result;
  RecordStream stream(result);
  stream(cell);
  return result;
}

Cell FanCell(Id index, Id center = 0) {
  Cell result;
  result.t =
      triangle({center, {0, 0}, 0}, {2 * index + 1, {1, double(index)}, 1}, {2 * index + 2, {1, double(index + 1)}, 1});
  result.t.id = 1000 + index;
  result.version = 17 + index;
  result.marker = {int(index + 1), 0, 0};
  result.target_cache = {.7, 1., 2., 3.};
  result.nodal_target = {Tensor{4., .2, 8.}, Tensor{7., -.1, 10.}, Tensor{12., 0, 20.}};
  result.target_epoch = 29;
  return result;
}

bool SameStars(const Directory::Stars& a, const Directory::Stars& b) {
  if (a.size() != b.size()) return false;
  for (const auto& node : a) {
    const auto found = b.find(node.first);
    if (found == b.end() || node.second.size() != found->second.size()) return false;
    for (const auto& cell : node.second) {
      const auto other = found->second.find(cell.first);
      if (other == found->second.end()) return false;
      const auto& x = cell.second;
      const auto& y = other->second;
      if (x.node != y.node || x.cell != y.cell || x.version != y.version || x.owner != y.owner || x.erase != y.erase)
        return false;
    }
  }
  return true;
}
}  // namespace

TEST_CASE("Native MPI: explicit scalar record round trip", "[NativeDistributed2D]") {
  auto original = FanCell(3);
  original.t.id = UINT64_MAX;
  original.t.v[0].fixed = -3;
  original.version = UINT64_MAX - 1;
  original.target_epoch = UINT64_MAX - 2;
  RecordStream measure;
  measure(original);
  const auto bytes = Bytes(original);
  CHECK(measure.Size() == bytes.size());
  Cell restored;
  RecordStream reader(bytes);
  reader(restored);
  CHECK(reader.End());
  CHECK(Bytes(restored) == bytes);
  CHECK(restored.t.id == UINT64_MAX);
  CHECK(restored.t.v[0].fixed == -3);
  auto shortBytes = bytes;
  shortBytes.pop_back();
  RecordStream truncated(static_cast<const std::vector<char>&>(shortBytes));
  CHECK_THROWS_AS(truncated(restored), std::runtime_error);
  Incidence sentinel{UINT64_MAX, 0, UINT64_MAX, -1, 0};
  std::vector<char> encoded;
  RecordStream encode(encoded);
  encode(sentinel);
  RecordStream decode(static_cast<const std::vector<char>&>(encoded));
  Incidence decoded;
  decode(decoded);
  CHECK(decoded.node == UINT64_MAX);
  CHECK(decoded.owner == -1);
  CHECK(decoded.version == UINT64_MAX);

  World world;
  std::vector<std::vector<Cell>> to(world.size);
  to[(world.rank + 1) % world.size].push_back(original);
  const auto savedRound = CPassiveComm::GetRoundBytes();
  CPassiveComm::SetRoundBytes(127);
  const auto received = world.exchange(to);
  CPassiveComm::SetRoundBytes(savedRound);
  REQUIRE(received.size() == 1);
  CHECK(Bytes(received.front()) == bytes);
  CHECK(world.bytes_sent == measure.Size());
}

TEST_CASE("Native MPI: complete stars, reservations and rejected staging", "[NativeDistributed2D]") {
  World world;
  Directory directory(world);
  std::map<Id, Cell> owned;
  const int owners = std::max(1, world.size - 1);  // Deliberately leave the final rank empty.
  for (Id i = 0; i < 8; ++i)
    if (int(i % owners) == world.rank) {
      const auto cell = FanCell(i);
      owned.emplace(cell.t.id, cell);
    }
  std::vector<Cell> local;
  for (const auto& cell : owned) local.push_back(cell.second);
  directory.update({}, local);
  bool overflow = false;
  const auto refs = directory.lookup({0}, overflow);
  CHECK_FALSE(overflow);
  CHECK(refs.size() == 8);
  const auto edgeRefs = directory.lookup({0, 1}, overflow, true);
  REQUIRE(edgeRefs.size() == 1);
  CHECK(edgeRefs.front().cell == 1000);
  bool stale = false;
  const auto patch = directory.fetch(refs, owned, true, stale);
  CHECK_FALSE(stale);
  REQUIRE(patch.size() == 8);
  for (size_t i = 0; i < patch.size(); ++i) CHECK(Bytes(patch[i]) == Bytes(FanCell(i)));
  const auto deferred = directory.fetch(refs, owned, false, stale);
  CHECK(deferred.empty());
  auto badRefs = refs;
  badRefs.front().version++;
  const auto partial = directory.fetch(badRefs, owned, true, stale);
  CHECK(stale);
  CHECK(partial.size() == 7);
  int conflicting = 0;
  CHECK(directory.reserve(patch, true, conflicting) == (world.rank == 0));
  CHECK(conflicting == (world.rank == 0 ? 0 : 1));
  CHECK_FALSE(directory.reserve({}, false, conflicting));

  const auto before = directory.incident;
  std::vector<Cell> erased, added;
  if (world.rank == 0) {
    erased.push_back(FanCell(0));
    auto replacement = erased.front();
    replacement.version++;
    replacement.t.v[2].id = 5000;
    added.push_back(replacement);
  }
  auto prepared = directory.prepare(erased, added);
  CHECK(prepared.valid);
  CHECK(SameStars(directory.incident, before));
  // Destroy a valid prepared operation: rejection retains the exact accepted map.
  prepared = Directory::Prepared{};
  CHECK(SameStars(directory.incident, before));
  auto staleErase = erased;
  if (!staleErase.empty()) staleErase.front().version++;
  const auto rejected = directory.prepare(staleErase, added);
  CHECK_FALSE(rejected.valid);
  CHECK(rejected.reason.find("stale") != std::string::npos);
  CHECK(SameStars(directory.incident, before));
  prepared = directory.prepare(erased, added);
  CHECK(prepared.valid);
  directory.commit(std::move(prepared));
  overflow = false;
  const auto replacement = directory.lookup({5000}, overflow);
  REQUIRE(replacement.size() == 1);
  CHECK(replacement.front().version == 18);
  CHECK(replacement.front().owner == 0);
  CHECK(directory.lookup({2}, overflow).empty());
}

TEST_CASE("Native MPI: oversized star rejects before cell payload", "[NativeDistributed2D]") {
  World world;
  Directory directory(world);
  std::vector<Cell> local;
  std::map<Id, Cell> owned;
  for (Id i = 0; i < PATCH_LIMIT + 1; ++i)
    if (int(i % world.size) == world.rank) {
      const auto cell = FanCell(i, 700);
      local.push_back(cell);
      owned.emplace(cell.t.id, cell);
    }
  directory.update({}, local);
  bool overflow = false, stale = false;
  const auto refs = directory.lookup({700}, overflow);
  CHECK(overflow);
  CHECK(refs.empty());
  CHECK(directory.fetch(refs, owned, !overflow, stale).empty());
  CHECK_FALSE(stale);
}
