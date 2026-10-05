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
