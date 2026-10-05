/*!
 * \file CNativeMemory2D_tests.cpp
 * \brief Measured requested heap bytes of native transactions, including map/tree allocations.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "catch.hpp"
#include "../memory/AllocationProbe.hpp"
#include "../../../Common/include/adaptation/CNativeEngine2D.hpp"
#include "../../../Common/include/adaptation/CNativeReference2D.hpp"

using namespace SU2NativeBoundary2D;

TEST_CASE("Native dependency memory: measured complete transaction including tree nodes", "[NativeMemory]") {
  World world;
  const Node a{0, {0, 0}, 1 | FEATURE}, b{1, {.04, 0}, 1 | FEATURE}, c{2, {.04, .02}, 1 | FEATURE},
      d{3, {0, .02}, 1 | FEATURE};
  const PolylineReference reference({{a, b, 0}, {b, c, 1}, {c, d, 2}, {d, a, 1}}, 45);
  std::map<Id, Cell> input;
  Cell lower, upper;
  lower.t = triangle(a, b, c);
  lower.t.id = 10;
  lower.marker = {reference.ComponentOfOriginalFace(0, 1), reference.ComponentOfOriginalFace(1, 2), 0};
  upper.t = triangle(a, c, d);
  upper.t.id = 11;
  upper.marker = {0, reference.ComponentOfOriginalFace(2, 3), reference.ComponentOfOriginalFace(3, 0)};
  int owner = 0;
  for (auto cell : {lower, upper}) {
    for (auto& metric : cell.nodal_target) metric = {625, 0, 62500};
    if (owner++ % std::min(2, world.size) == world.rank) input.emplace(cell.t.id, cell);
  }
  EngineOptions options;
  options.geometry_tolerance = 1e-10;
  Engine engine(world, input, reference.Policy({{0, .004}}), options);
  Choice split{{Action::SPLIT, 0, 1}, 1, world.rank};
  // Accepted mesh, original donor, directory, reference and MPI residents precede the dependency window.
  // Everything allocated by round(), including incoming/outgoing metadata and map nodes, is measured.
  const auto base = alloc_probe::Live();
  alloc_probe::ResetPeak();
  const bool accepted = engine.round(split);
  const auto peak = alloc_probe::Peak();
  const auto measured = peak > base ? peak - base : 0;
  const auto maxMeasured = CPassiveComm::Allreduce(uint64_t(measured), CPassiveComm::Op::MAX);
  CHECK(world.sum(accepted) == 1);
  CHECK(maxMeasured <= options.dependency_bytes);
  if (world.rank == 0)
    std::cout << "[native memory] ranks=" << world.size << " complete round requested heap peak=" << maxMeasured
              << " bytes, dependency budget=" << options.dependency_bytes << '\n';
  // Named limit: this is a measured fixture gate, not a proof for all stars/process counts; MPI malloc/RSS and
  // allocator rounding are outside operator-new requested bytes. High-valence staging has independent controls.
}

TEST_CASE("Native dependency memory: complete movement and insertion patches with rejection", "[NativeMemory]") {
  World world;
  const int count = GENERATE(6, 32, 64, 128);
  const auto action = GENERATE(Action::BULK_MOVE, Action::BULK_SPLIT);
  std::vector<Node> ring;
  for (int k = 0; k < count; ++k) {
    const double angle = 2 * std::acos(-1.) * k / count;
    ring.push_back({Id(k + 1), {std::cos(angle), std::sin(angle)}, 1});
  }
  std::vector<PolylineReference::Face> faces;
  for (int k = 0; k < count; ++k) faces.push_back({ring[k], ring[(k + 1) % count], 0});
  const PolylineReference reference(faces, 45);
  const Node center{0, {.7, .5}, 0};
  std::map<Id, Cell> input;
  for (int k = 0; k < count; ++k) {
    Cell cell;
    cell.t = triangle(center, ring[k], ring[(k + 1) % count]);
    cell.t.id = 10 + k;
    cell.marker[1] = reference.ComponentOfOriginalFace(ring[k].id, ring[(k + 1) % count].id);
    for (auto& m : cell.nodal_target) m = {1, 0, 1};
    if (k % world.size == world.rank) input.emplace(cell.t.id, cell);
  }
  auto snapshot = [](const std::map<Id, Cell>& cells) {
    std::vector<char> bytes;
    RecordStream stream(bytes);
    for (const auto& entry : cells) {
      auto cell = entry.second;
      stream(cell);
    }
    return bytes;
  };
  for (const size_t budget : {size_t(2 * 1024 * 1024), size_t(64)}) {
    EngineOptions options;
    options.geometry_tolerance = 1e-10;
    options.dependency_bytes = budget;
    Engine engine(world, input, reference.Policy({}), options);
    const auto before = snapshot(engine.owned);
    Engine::SelectionCache cache;
    const std::set<Edge> attempted;
    engine.chooseCached(action, attempted, cache);
    Choice move{{action, center.id, action == Action::BULK_SPLIT ? ring.front().id : 0},
                world.rank == 0 ? 1. : -1., world.rank};
    const auto base = alloc_probe::Live();
    alloc_probe::ResetPeak();
    const bool accepted = engine.round(move);
    const auto peak = alloc_probe::Peak();
    const auto measured = peak > base ? peak - base : 0;
    const auto maxMeasured = CPassiveComm::Allreduce(uint64_t(measured), CPassiveComm::Op::MAX);
    const auto selectionBase = alloc_probe::Live();
    alloc_probe::ResetPeak();
    const auto cached = engine.chooseCached(action, attempted, cache);
    const auto selectionPeak = alloc_probe::Peak();
    const auto exhaustive = engine.choose(action, attempted);
    CHECK(selectionPeak == selectionBase);
    CHECK(cached.score == exhaustive.score);
    CHECK(cached.op.a == exhaustive.op.a);
    CHECK(cached.op.b == exhaustive.op.b);
    INFO("complete star cells=" << count << ", action=" << int(action) << ", budget=" << budget);
    if (budget == 64) {
      CHECK(world.sum(accepted) == 0);
      CHECK(snapshot(engine.owned) == before);
      CHECK(world.sum(engine.stats.memory_rejected) > 0);
      // The deliberately tiny payload budget excludes the fixed control plane.
    } else {
      const bool oversizedSplit = action == Action::BULK_SPLIT && count > int(MAX_CAVITY);
      CHECK(world.sum(accepted) == (oversizedSplit ? 0 : 1));
      if (oversizedSplit) {
        CHECK(snapshot(engine.owned) == before);
        if (world.rank == 0) CHECK(engine.stats.rejected.at("cavity size limit") == 1);
      }
      CHECK(maxMeasured <= budget);
    }
    if (world.rank == 0)
      std::cout << "[native star memory] ranks=" << world.size << " action=" << int(action) << " star cells=" << count
                << " complete round requested heap peak=" << maxMeasured << " budget=" << budget << '\n';
  }
  // The 64/128 fans intentionally exceed the final shape target at fixed valence;
  // this measures one movement transaction, not a completed remesh or RSS/MPI malloc.
}
