/*!
 * \file CNativeMetric3D_tests.cpp
 * \brief Frozen metric progress versus final completion and composed-target rejection.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include <cmath>
using namespace SU2Native3D;
namespace {
Cell Single() { return {0, {{{0, {0, 0, 0}}, {1, {1, 0, 0}}, {2, {0, 1, 0}}, {3, {0, 0, 1}}}}}; }
DonorCell Donor(Tensor metric) {
  DonorCell d;
  d.cell = Single();
  d.sensor.fill(metric);
  return d;
}
}  // namespace
TEST_CASE("Native 3D final metric gate requires length, shape and sliver criteria", "[NativeMesh3D]") {
  PatchMetric measured;
  MetricStats stats;
  std::string reason;
  FrozenField unit({Donor({})});
  CHECK(ValidateMetricPatch({Single()}, unit, measured, reason, &stats));
  CHECK(measured.maximum_length == Approx(std::sqrt(2.)));
  CHECK(measured.edges.size() == 6);
  FrozenField large({Donor({4, 0, 0, 4, 0, 4})});
  CHECK_FALSE(ValidateMetricPatch({Single()}, large, measured, reason, &stats));
  CHECK(reason.find("oversized") != std::string::npos);
  CHECK(measured.maximum_length > MaximumMetricEdge);
  FrozenField flattened({Donor({1, 0, 0, 1, 0, 1e-16})});
  CHECK_FALSE(ValidateMetricPatch({Single()}, flattened, measured, reason, &stats));
  CHECK(reason.find("shape/sliver") != std::string::npos);
  CHECK(measured.minimum_mean_ratio < MinimumMeanRatio);
  CHECK(stats.patches == 3);
  CHECK(stats.failures == 2);
  CHECK(stats.maximum_nodes == 4);
  CHECK(stats.maximum_edges == 6);
  CHECK_FALSE(ValidateMetricPatch({}, unit, measured, reason));
}
TEST_CASE("Native 3D private refinement can progress before the final global size gate", "[NativeMesh3D]") {
  const std::array<Node, 8> nodes{{{0, {0, 0, 0}},
                                   {1, {1, 0, 0}},
                                   {2, {1, 1, 0}},
                                   {3, {0, 1, 0}},
                                   {4, {0, 0, 1}},
                                   {5, {1, 0, 1}},
                                   {6, {1, 1, 1}},
                                   {7, {0, 1, 1}}}};
  Patch old;
  std::vector<DonorCell> original;
  const std::array<Id, 6> ring{1, 2, 3, 7, 4, 5};
  for (size_t i = 0; i < 6; ++i) {
    Cell c{Id(i), {nodes[0], nodes[6], nodes[ring[i]], nodes[ring[(i + 1) % 6]]}};
    if (Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) < 0) std::swap(c.v[0], c.v[1]);
    old.cells.push_back(c);
    DonorCell donor;
    donor.cell = c;
    donor.sensor.fill({4, 0, 0, 4, 0, 4});
    original.push_back(donor);
  }
  Patch fresh;
  std::string reason;
  REQUIRE(SplitEdge(old, {0, 6}, {8, {.5, .5, .5}}, 100, PlanarReference(std::vector<Facet>{}), fresh, reason));
  FrozenField target(original);
  PatchMetric measured;
  MetricStats stats;
  REQUIRE(ValidateMetricChange(old.cells, fresh.cells, MetricChange::Refine, target, measured, reason, &stats));
  CHECK(measured.maximum_length > MaximumMetricEdge);
  CHECK_FALSE(ValidateMetricPatch(fresh.cells, target, measured, reason));
  CHECK(stats.edge_reused > 0);
  CHECK(stats.edges == stats.edge_evaluations + stats.edge_reused);
  CHECK(stats.edge_evaluations == target.Statistics().edge_requests - (measured.edges.size()));
  CHECK_FALSE(ValidateMetricChange(fresh.cells, old.cells, MetricChange::Coarsen, target, measured, reason));
  for (auto& d : original) d.sensor.fill({});
  FrozenField coarse(original);
  CHECK(ValidateMetricChange(fresh.cells, old.cells, MetricChange::Coarsen, coarse, measured, reason));
  CHECK(ValidateMetricPatch(old.cells, coarse, measured, reason));
  CHECK_FALSE(ValidateMetricChange(old.cells, fresh.cells, MetricChange::Refine, coarse, measured, reason));
}
TEST_CASE("Native 3D metric gate never uses sensor integration to qualify a composed target", "[NativeMesh3D]") {
  size_t queries = 0;
  FrozenField target({Donor({})}, [&](Point p, Tensor sensor) {
    ++queries;
    CHECK(p.x == .25);
    sensor.xx += 1;
    return sensor;
  });
  PatchMetric measured;
  std::string reason;
  CHECK_FALSE(ValidateMetricPatch({Single()}, target, measured, reason));
  CHECK(reason.find("geometry-resolved") != std::string::npos);
  CHECK(queries == 1);
  CHECK(target.SensorEdgeLength({0, 0, 0}, {1, 0, 0}) == 1);
  CHECK(queries == 1);
}
TEST_CASE("Native 3D metric repair accepts improving interior movement and rejects regression", "[NativeMesh3D]") {
  std::vector<Cell> old, fresh;
  std::string reason;
  PatchMetric measured;
  REQUIRE(Cone({Single()}, {4, {.05, .25, .25}}, 10, old, reason));
  REQUIRE(Cone(old, {4, {.25, .25, .25}}, 20, fresh, reason));
  FrozenField target({Donor({})});
  CHECK(ValidateMetricChange(old, fresh, MetricChange::Repair, target, measured, reason));
  CHECK(ValidateMetricPatch(fresh, target, measured, reason));
  CHECK_FALSE(ValidateMetricChange(fresh, old, MetricChange::Repair, target, measured, reason));
  CHECK_FALSE(ValidateMetricChange(fresh, fresh, MetricChange::Repair, target, measured, reason));
  CHECK_FALSE(ValidateMetricChange(fresh, old, static_cast<MetricChange>(99), target, measured, reason));
  CHECK(reason.find("Unknown") != std::string::npos);
}
TEST_CASE("Native 3D metric repair couples face reconnection to actual frozen quality", "[NativeMesh3D]") {
  const Node a{0, {0, 0, 0}}, b{1, {1, 0, 0}}, c{2, {.5, std::sqrt(3.) / 2, 0}}, top{3, {.5, std::sqrt(3.) / 6, .1}},
      bottom{4, {.5, std::sqrt(3.) / 6, -.1}};
  std::vector<Cell> old{{0, {a, b, c, top}}, {1, {b, a, c, bottom}}}, fresh, back;
  std::vector<DonorCell> donors;
  for (const auto& cell : old) {
    DonorCell d;
    d.cell = cell;
    d.sensor.fill({});
    donors.push_back(d);
  }
  FrozenField target(donors);
  std::string reason;
  PatchMetric measured;
  REQUIRE(Cone(old, top, 10, fresh, reason));
  REQUIRE(fresh.size() == 3);
  CHECK(ValidateMetricChange(old, fresh, MetricChange::Repair, target, measured, reason));
  CHECK(ValidateMetricPatch(fresh, target, measured, reason));
  REQUIRE(Cone(fresh, a, 20, back, reason));
  REQUIRE(back.size() == 2);
  CHECK(ValidateMetricChange(fresh, back, MetricChange::Coarsen, target, measured, reason));
}
