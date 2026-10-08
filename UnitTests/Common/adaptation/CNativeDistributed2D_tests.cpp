/*!
 * \file CNativeDistributed2D_tests.cpp
 * \brief Versioned native incidence discovery, explicit wire fields and private publication controls.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include <numeric>
#include "../../../Common/include/adaptation/CNativeDistributed2D.hpp"
#include "../../../Common/include/adaptation/CNativePartition2D.hpp"

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
  const auto before = directory.incident;
  std::vector<Cell> erased, added;
  if (world.rank == 0) {
    erased.push_back(FanCell(0, 700));
    auto replacement = erased.front();
    ++replacement.version;
    added.push_back(replacement);
  }
  const auto prepared = directory.prepare(erased, added, 2 * 1024 * 1024);
  CHECK_FALSE(prepared.valid);
  CHECK(prepared.reason.find("staging cap") != std::string::npos);
  CHECK(SameStars(directory.incident, before));
}

// One malformed rank must fail collectively before any peer array is indexed
// or counts are exchanged; run only in an expected-error MPI subprocess.
TEST_CASE("Native MPI failure: invalid peer buckets on one rank", "[NativeInvalidPeerBuckets][.]") {
  World world;
  std::vector<std::vector<Cell>> to(world.size);
  if (world.rank == 0) to.clear();
  world.exchange(to);
  FAIL("Invalid peer buckets passed native exchange preflight.");
}

TEST_CASE("Native MPI: weighted working partition preserves complete raw cells", "[NativeDistributed2D]") {
  World world;
  const auto layout = GENERATE(0, 1, 2);  // One owner, one empty rank, interleaved owners.
  const auto parts = GENERATE_COPY(1, world.size);
  std::map<Id, Cell> input;
  std::vector<uint32_t> weights;
  std::map<Id, std::vector<char>> original;
  for (int j = 0; j < 12; ++j)
    for (int i = 0; i < 16; ++i) {
      auto node = [&](int di, int dj) { return Node{Id((j + dj) * 17 + i + di), {double(i + di), double(j + dj)}, 0}; };
      const auto a = node(0, 0), b = node(1, 0), c = node(1, 1), d = node(0, 1);
      for (int k = 0; k < 2; ++k) {
        const auto index = 2 * (16 * j + i) + k;
        auto cell = FanCell(index);
        cell.t = k ? triangle(a, c, d) : triangle(a, b, c);
        cell.t.id = 100000 + 13 * index;  // Sparse cell IDs are not graph ordinals.
        cell.t.protected_cell = index % 3 == 0;
        original.emplace(cell.t.id, Bytes(cell));
        const int owners = layout == 0 ? 1 : layout == 1 ? std::max(1, world.size - 1) : world.size;
        if (index % owners == world.rank) input.emplace(cell.t.id, cell);
      }
    }
  for (const auto& entry : input) {
    const auto x = centroid(entry.second.t).x;
    weights.push_back(x < 8 ? 4 : 1);
  }
  const auto before = input;
  PartitionStats rejected;
  CHECK_FALSE(Repartition(world, input, weights, parts, rejected, world.rank == 0 ? 1 : SIZE_MAX));
  REQUIRE(input.size() == before.size());
  for (const auto& entry : input) CHECK(Bytes(entry.second) == Bytes(before.at(entry.first)));

  PartitionStats stats;
  REQUIRE(Repartition(world, input, weights, parts, stats));
  CHECK(std::accumulate(stats.cells_after.begin(), stats.cells_after.end(), uint64_t(0)) == 384);
  CHECK(std::accumulate(stats.work_after.begin(), stats.work_after.end(), uint64_t(0)) == 960);
  CHECK(stats.migration_bytes > 0);
  for (const auto& entry : input) CHECK(Bytes(entry.second) == original.at(entry.first));
  if (parts == 1) CHECK(input.size() == (world.rank == 0 ? 384 : 0));
  else {
    CHECK(*std::max_element(stats.cells_after.begin(), stats.cells_after.end()) <= 1.3 * 384 / parts);
    CHECK(*std::max_element(stats.work_after.begin(), stats.work_after.end()) <= 1.3 * 960 / parts);
  }
  std::vector<Cell> local;
  for (const auto& entry : input) local.push_back(entry.second);
  const auto gathered = world.metadata(local);  // Tiny test fixture only; production never gathers volume cells.
  std::set<Id> seen;
  for (const auto& cell : gathered) CHECK(seen.insert(cell.t.id).second);
  CHECK(seen.size() == original.size());
}

TEST_CASE("Native partition weights keep geometric BL out of raw sensor donors", "[NativeDistributed2D]") {
  Cell cell;
  cell.t = triangle({0, {0, 0}, 0}, {1, {1, 0}, 0}, {2, {0, 1}, 0});
  cell.nodal_target.fill(Tensor{4, 0, 4});
  cell.marker = {7, 0, 0};
  const std::map<Id, Cell> input{{0, cell}};
  const auto before = Bytes(cell);
  int queries = 0, wallQueries = 0, interiorQueries = 0;
  const MetricComposition composition = [&](Point p, Tensor sensor) {
    ++queries;
    if (p.y < 1e-4) {
      ++wallQueries;
      sensor.yy = std::max(sensor.yy, 1e8);
    } else ++interiorQueries;
    return sensor;
  };
  const auto plain = WorkWeights(input, {}, {});
  const auto bl = WorkWeights(input, composition, {{7, 25}});
  REQUIRE(bl.size() == 1);
  CHECK(bl[0] > plain[0]);
  CHECK(bl[0] < 512);  // Fine wall samples do not multiply the entire coarse-cell area.
  CHECK(queries > 0);
  CHECK(wallQueries > 0);
  CHECK(interiorQueries > 0);
  CHECK(Bytes(input.at(0)) == before);
  auto protectedInput = input;
  protectedInput.at(0).t.protected_cell = 1;
  CHECK(WorkWeights(protectedInput, composition, {{7, 25}})[0] < bl[0]);
}

TEST_CASE("Native work sampling associates rounded physical-edge queries", "[NativeDistributed2D]") {
  const Node a{0, {.2, 0}, 1}, b{1, {.2 + .04, .02}, 1}, c{2, {.2, .02}, 1};
  Cell cell;
  cell.t = triangle(a, b, c);
  cell.marker[0] = 7;
  cell.nodal_target.fill(Tensor{4, 0, 8});
  const Point midpoint{double((static_cast<long double>(a.p.x) + b.p.x) / 2), .01};
  CHECK(orient(a.p, b.p, midpoint) < 0);
  const auto before = Bytes(cell);
  const auto weights = WorkWeights({{0, cell}}, {}, {});
  REQUIRE(weights.size() == 1);
  CHECK(weights[0] > 0);
  CHECK(Bytes(cell) == before);
}

TEST_CASE("Native MPI failure: nonmanifold working graph", "[NativeInvalidWorkingGraph][.]") {
  World world;
  std::map<Id, Cell> input;
  if (world.rank == 0)
    for (Id i = 0; i < 6; ++i) {
      Cell cell;
      cell.t = triangle({0, {0, 0}, 0}, {1, {1, 0}, 0}, {i + 2, {.5, double(i + 1)}, 0});
      cell.t.id = i;
      input.emplace(i, cell);
    }
  PartitionStats stats;
  Repartition(world, input, std::vector<uint32_t>(input.size(), 1), world.size, stats);
  FAIL("Nonmanifold native working graph passed preflight.");
}
