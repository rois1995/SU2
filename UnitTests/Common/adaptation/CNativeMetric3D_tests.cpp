/*!
 * \file CNativeMetric3D_tests.cpp
 * \brief Frozen metric progress versus final completion and composed-target rejection.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include "../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include <cmath>
#include <limits>
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

TEST_CASE("Native3D edge-only construction ignores metric q/J and rejects invalid orientation", "[NativeMesh3D][NativeShapeAdmission3D]") {
  DonorCell donor=Donor({1,0,0,1,0,1e-12});
  size_t centroid_queries=0;
  FrozenField edge_target({donor},[](Point p,Tensor m) {
    if(p.x==.25 && p.y==.25 && p.z==.25) throw std::runtime_error("centroid was queried during construction");
    return m;
  },false,{},[](Point,Point){return std::vector<double>{0,1};});
  auto edges=MeasureMetricEdges({Single()},edge_target);
  CHECK_FALSE(edges.shape_measured);
  CHECK(std::isnan(edges.minimum_mean_ratio));CHECK(std::isnan(edges.minimum_scaled_jacobian));
  CHECK(edges.edges.size()==6);
  auto reversed=Single();std::swap(reversed.v[0],reversed.v[1]);
  CHECK_THROWS(MeasureMetricEdges({reversed},edge_target));
  auto degenerate=Single();degenerate.v[3].p.z=0;
  CHECK_THROWS(MeasureMetricEdges({degenerate},edge_target));
  FrozenField final_target({donor});
  PatchMetric final;std::string reason;
  CHECK_FALSE(ValidateMetricPatch({Single()},final_target,final,reason));
  CHECK(final.shape_measured);CHECK(final.minimum_scaled_jacobian<MinimumScaledJacobian);
  CHECK(reason.find("shape/sliver")!=std::string::npos);
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
  {  // The split edge is admissible (<= 1.8) but longer than SplitInsertionLength: StrictExcess requires total
     // excess reduction, while SplitInsertion requires only a shorter changed-edge frontier for net-new resolution.
    struct Restore { RefineAcceptance a=GetRefineAcceptance(); ~Restore(){SetRefineAcceptance(a);} } restore;
    SetRefineAcceptance(RefineAcceptance::StrictExcess);
    CHECK_FALSE(ValidateMetricChange(old.cells, fresh.cells, MetricChange::Refine, coarse, measured, reason));
    SetRefineAcceptance(RefineAcceptance::SplitInsertion);
    MetricDiagnosis diagnosis;
    CHECK(ValidateMetricChange(old.cells, fresh.cells, MetricChange::Refine, coarse, measured, reason, nullptr, nullptr, &diagnosis));
    CHECK(diagnosis.size_insertion);
    CHECK_FALSE(diagnosis.vertex_removal);
    CHECK(diagnosis.longest_removed > SplitInsertionLength);
    CHECK(diagnosis.longest_removed <= MaximumMetricEdge);
  }
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

TEST_CASE("Native 3D frozen scalar reuse checks coordinates and preserves composed rejection", "[NativeMesh3D]") {
  FrozenField target({Donor({})});
  PatchMetric original, measured;
  std::string reason;
  REQUIRE(ValidateMetricPatch({Single()}, target, original, reason));
  MetricStats identical;
  CHECK(ValidateMetricPatch({Single()}, target, measured, reason, &identical, &original.edges));
  CHECK(identical.edge_evaluations == 0);
  CHECK(identical.edge_reused == 6);
  CHECK(measured.maximum_length == original.maximum_length);
  auto moved = Single();
  moved.v[1].p.x = .5;
  MetricStats changed;
  CHECK(ValidateMetricPatch({moved}, target, measured, reason, &changed, &original.edges));
  CHECK(changed.edge_evaluations == 3);
  CHECK(changed.edge_reused == 3);
  CHECK(measured.edges.at({0, 1}).length == .5L);
  auto malformed = original.edges;
  malformed.at({0, 1}).length = std::numeric_limits<long double>::quiet_NaN();
  MetricStats invalid;
  CHECK(ValidateMetricPatch({Single()}, target, measured, reason, &invalid, &malformed));
  CHECK(invalid.edge_evaluations == 1);
  CHECK(invalid.edge_reused == 5);
  size_t compositions = 0;
  FrozenField composed({Donor({})}, [&](Point, Tensor sensor) { ++compositions; return sensor; });
  CHECK_FALSE(ValidateMetricPatch({Single()}, composed, measured, reason, nullptr, &original.edges));
  CHECK(reason.find("geometry-resolved") != std::string::npos);
  CHECK(compositions == 1);
}

TEST_CASE("Composed edge reuse requires an immutable field identity and exact endpoints", "[NativeMetricReuse3D]") {
  const auto identity=std::make_shared<const FrozenMetricIdentity>();size_t compositions=0;
  const MetricComposition compose=[&](Point p,Tensor sensor) {
    ++compositions;sensor.xx+=.25*p.x;return sensor;
  };
  const MetricEdgeBreakpoints bands=[](Point,Point){return std::vector<double>{0,1};};
  FrozenField first({Donor({})},compose,true,{},bands,identity);
  const auto before=MeasureMetricCells({Single()},first);CHECK(first.Statistics().edge_requests==6);
  // A fresh imported index may share scalar results only under the explicitly retained full-field authority.
  FrozenField imported({Donor({})},compose,true,{},bands,identity);MetricStats same;
  auto measured=MeasureMetricCells({Single()},imported,&same,&before.edges);
  CHECK(same.edge_reused==6);CHECK(same.edge_evaluations==0);CHECK(imported.Statistics().edge_requests==0);
  CHECK(measured.maximum_length==before.maximum_length);CHECK(measured.minimum_mean_ratio==before.minimum_mean_ratio);
  CHECK(compositions==first.Statistics().compositions+1); // The actual centroid is still composed.
  auto moved=Single();moved.v[1].p.x=.5;MetricStats changed;
  measured=MeasureMetricCells({moved},imported,&changed,&before.edges);
  CHECK(changed.edge_reused==3);CHECK(changed.edge_evaluations==3);
  const auto cold=MeasureMetricCells({moved},imported);
  for(const auto& edge:measured.edges)CHECK(edge.second.length==cold.edges.at(edge.first).length);
  FrozenField fresh({Donor({4,0,0,4,0,4})},compose,false,{},bands);MetricStats new_stage;
  const auto refined=MeasureMetricCells({Single()},fresh,&new_stage,&before.edges);
  CHECK(new_stage.edge_reused==0);CHECK(new_stage.edge_evaluations==6);
  CHECK(refined.maximum_length>before.maximum_length);CHECK(fresh.Identity()!=identity);
  auto invalid=before.edges;invalid.at({0,1}).length=std::numeric_limits<long double>::quiet_NaN();
  MetricStats malformed;measured=MeasureMetricCells({Single()},imported,&malformed,&invalid);
  CHECK(malformed.edge_reused==5);CHECK(malformed.edge_evaluations==1);
  // Even an explicitly shared token cannot authorize unsupported composed-edge integration.
  FrozenField unresolved({Donor({})},compose,false,{}, {},identity);
  CHECK_THROWS_WITH(MeasureMetricCells({Single()},unresolved,nullptr,&before.edges),
                    "Native 3D composed target requires geometry-resolved edge integration.");
  auto outside=Single();outside.v[1].p.x=1.5;
  CHECK_THROWS(MeasureMetricCells({outside},imported,nullptr,&before.edges));
  // Sensor-only reuse now rejects unrelated fields too, rather than trusting edge coordinates alone.
  FrozenField sensor({Donor({4,0,0,4,0,4})});MetricStats other;
  MeasureMetricCells({Single()},sensor,&other,&before.edges);CHECK(other.edge_reused==0);
}

TEST_CASE("Native general wall collapse retains fine demands through the frozen metric gate", "[NativeMesh3D]") {
  const auto tetra=Single();
  Patch original;original.cells={tetra};
  std::vector<Facet> facets;
  for(auto face:Incidence(original.cells).Boundary()) {
    std::array<Node,3> nodes;
    for(size_t i=0;i<3;++i) for(auto n:tetra.v) if(n.id==face[i]) nodes[i]=n;
    const Id id=facets.size();facets.push_back({id,int(id)+10,nodes});original.surface.push_back({face,id});
  }
  const PlanarReference geometry(facets);const Node point{4,{.25,.25,0}};
  Patch old;old.cells={{10,{point,tetra.v[1],tetra.v[2],tetra.v[3]}},
      {11,{tetra.v[0],point,tetra.v[2],tetra.v[3]}},{12,{tetra.v[0],tetra.v[1],point,tetra.v[3]}}};
  for(auto face:Incidence(old.cells).Boundary()) for(auto facet:facets) {
    bool contains=true;
    for(auto id:face) {
      const auto p=id==point.id ? point.p : tetra.v[id].p;
      contains=contains && geometry.Contains(facet.id,p);
    }
    if(contains){old.surface.push_back({face,facet.id});break;}
  }
  Patch fresh;std::string reason;
  REQUIRE(CollapseVertex(old,4,0,100,geometry,fresh,reason));
  PatchMetric measured;
  FrozenField coarse({Donor({})});
  CHECK(ValidateMetricChange(old.cells,fresh.cells,MetricChange::Coarsen,coarse,measured,reason));
  FrozenField fine({Donor({4,0,0,4,0,4})});
  CHECK(ValidateMetricChange(old.cells,fresh.cells,MetricChange::Coarsen,fine,measured,reason));
  CHECK_FALSE(ValidateMetricPatch(fresh.cells,fine,measured,reason));
  CHECK(reason.find("oversized")!=std::string::npos);
}

TEST_CASE("Native private shape rejection avoids edge tracing while preserving actual composition", "[NativeMesh3D]") {
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::Absolute);
  std::vector<Cell> fresh;std::string reason;PatchMetric metric;MetricStats stats;
  REQUIRE(Cone({Single()},{4,{.03125,.03125,.03125}},100,fresh,reason));
  size_t compositions=0;
  FrozenField target({Donor({})},[&](Point,Tensor sensor){++compositions;return sensor;});
  CHECK_FALSE(ValidateMetricChange({Single()},fresh,MetricChange::Repair,target,metric,reason,&stats));
  CHECK(reason.find("shape/sliver")!=std::string::npos);
  CHECK(stats.cells==4);CHECK(stats.edge_evaluations==0);CHECK(target.Statistics().edge_requests==0);
  CHECK(compositions==4);CHECK(target.Statistics().requests==4);
}

TEST_CASE("Native 3D streamed metric measurements retain the full frozen progress gate", "[NativeMesh3D]") {
  const auto donor=Donor({4,0,0,4,0,4});
  Patch old; old.cells={Single()};
  Patch fresh; std::string reason;
  std::vector<Facet> facets;
  for (auto face:Incidence(old.cells).Boundary()) {
    std::array<Node,3> nodes;
    for (size_t i=0;i<3;++i) nodes[i]=Single().v[face[i]];
    const Id id=facets.size();facets.push_back({id,int(id),nodes});old.surface.push_back({face,id});
  }
  REQUIRE(SplitEdge(old,{1,2},{4,{.5,.5,0}},100,PlanarReference(facets),fresh,reason));
  FrozenField target({donor});
  const auto before=MeasureMetricCells(old.cells,target);
  const auto together=MeasureMetricCells(fresh.cells,target);
  PatchMetric streamed;
  streamed.shape_measured=true;streamed.minimum_mean_ratio=streamed.minimum_scaled_jacobian=std::numeric_limits<double>::infinity();
  streamed.weakest_cell=streamed.sliver_cell=fresh.cells.front().id;
  MetricStats stats;
  for (const auto& cell:fresh.cells) {
    const auto part=MeasureMetricCells({cell},target,&stats,&streamed.edges);
    streamed.minimum_mean_ratio=std::min(streamed.minimum_mean_ratio,part.minimum_mean_ratio);
    streamed.minimum_scaled_jacobian=std::min(streamed.minimum_scaled_jacobian,part.minimum_scaled_jacobian);
    streamed.maximum_length=std::max(streamed.maximum_length,part.maximum_length);
    streamed.edges.insert(part.edges.begin(),part.edges.end());
  }
  CHECK(streamed.minimum_mean_ratio==together.minimum_mean_ratio);
  CHECK(streamed.minimum_scaled_jacobian==together.minimum_scaled_jacobian);
  CHECK(streamed.maximum_length==together.maximum_length);
  REQUIRE(streamed.edges.size()==together.edges.size());
  for (const auto& edge:together.edges) CHECK(streamed.edges.at(edge.first).length==edge.second.length);
  CHECK(stats.edge_reused>0);
  const bool valid=ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),streamed,MetricChange::Refine,reason);
  REQUIRE(valid);
  PatchMetric checked;std::string full_reason;
  CHECK(valid==ValidateMetricChange(old.cells,fresh.cells,MetricChange::Refine,target,checked,full_reason));
  CHECK(reason==full_reason);
  CHECK_FALSE(ValidateMetricChange(0,before,fresh.cells.size(),streamed,MetricChange::Refine,reason));
  CHECK_FALSE(ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),{},MetricChange::Coarsen,reason));
  streamed.edges.begin()->second.length=std::numeric_limits<long double>::quiet_NaN();
  CHECK_FALSE(ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),streamed,MetricChange::Coarsen,reason));
  streamed=together;streamed.minimum_mean_ratio=std::numeric_limits<double>::quiet_NaN();
  CHECK_FALSE(ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),streamed,MetricChange::Repair,reason));
  FrozenField composed({donor},[](Point,Tensor sensor){return sensor;});
  CHECK_THROWS(MeasureMetricCells(fresh.cells,composed,nullptr,&together.edges));
}

TEST_CASE("Native metric diagnosis separates shape, measurement and frozen progress failures", "[NativeMesh3D][NativePolicy3D]") {
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::Absolute);
  FrozenField target({Donor({})});
  PatchMetric metric; MetricDiagnosis diagnosis; std::string reason;
  const auto cell=Single();
  const auto before=MeasureMetricCells({cell},target,nullptr,nullptr,false,&diagnosis);
  CHECK(diagnosis.failure==MetricFailure::None);
  CHECK_FALSE(ValidateMetricChange(1,before,1,before,MetricChange::Repair,reason,&diagnosis));
  CHECK(diagnosis.failure==MetricFailure::Progress);
  CHECK_FALSE(diagnosis.size_progress); CHECK_FALSE(diagnosis.quality_progress);
  auto unshaped=MeasureMetricEdges({cell},target);
  CHECK_FALSE(ValidateMetricChange(1,before,1,unshaped,MetricChange::Repair,reason,&diagnosis));
  CHECK(reason.find("lacks measured shape")!=std::string::npos);
  auto corrupt=before;corrupt.edges.begin()->second.length=-1;
  CHECK_FALSE(ValidateMetricChange(1,before,1,corrupt,MetricChange::Repair,reason,&diagnosis));
  CHECK(diagnosis.failure==MetricFailure::Measurement);
  auto sliver=cell;sliver.id=100;sliver.v[3].p.z=1e-12;
  CHECK_FALSE(ValidateMetricChange({cell},{sliver},MetricChange::Repair,target,metric,reason,nullptr,nullptr,&diagnosis));
  CHECK(diagnosis.failure==MetricFailure::Shape);
  CHECK(diagnosis.weakest_cell==100);CHECK(diagnosis.sliver_cell==100);
  CHECK(diagnosis.minimum_mean_ratio<MinimumMeanRatio);
  CHECK(diagnosis.minimum_scaled_jacobian<MinimumScaledJacobian);
  // Diagnostics must not replace a failed shape query by an integrated sensor surrogate.
  CHECK(target.Statistics().edge_requests==12); // before plus explicit edge-only measurement; failed shape query adds none
}

TEST_CASE("Coordinate-only metric repair can measure a large unchanged star", "[NativeMesh3D][NativePolicy3D]") {
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  constexpr size_t cells=130;
  PatchMetric before,after;
  before.shape_measured=after.shape_measured=true;
  before.minimum_mean_ratio=.10;before.minimum_scaled_jacobian=.10;
  after.minimum_mean_ratio=.12;after.minimum_scaled_jacobian=.11;
  // 40 vertices provide 780 distinct edges, beyond the ordinary 128-cell edge allowance (768).
  for(Id a=0;a<40;++a)for(Id b=a+1;b<40;++b) {
    EdgeMeasure measure;measure.length=1;
    before.edges.emplace(Edge{a,b},measure);after.edges.emplace(Edge{a,b},measure);
  }
  REQUIRE(before.edges.size()==780);
  std::string reason;MetricDiagnosis diagnosis;
  CHECK(ValidateMetricChange(cells,before,cells,after,MetricChange::Repair,reason,&diagnosis,0,true));
  CHECK(diagnosis.quality_progress);
  CHECK_FALSE(ValidateMetricChange(cells,before,cells,after,MetricChange::Repair,reason));
  CHECK_FALSE(ValidateMetricChange(cells,before,cells+1,after,MetricChange::Repair,reason,nullptr,0,true));
  CHECK_FALSE(ValidateMetricChange(cells,before,cells,after,MetricChange::Refine,reason,nullptr,0,true));
}

TEST_CASE("Geometry-only repairs retain strict shape and changed-edge excess gates", "[NativeSurfaceError3D]") {
  FrozenField field({Donor({})});const auto cell=Single();
  const auto before=MeasureMetricCells({cell},field);
  std::string reason;
  CHECK_FALSE(ValidateMetricChange(1,before,1,before,MetricChange::Repair,reason));
  CHECK(ValidateMetricChange(1,before,1,before,MetricChange::GeometryRepair,reason));
  CHECK_FALSE(ValidateMetricChange(1,before,1,before,MetricChange::GeometryRefine,reason));
  auto bad=before;bad.minimum_mean_ratio=.19;
  CHECK_FALSE(ValidateMetricChange(1,before,1,bad,MetricChange::GeometryRepair,reason));
  bad=before;bad.minimum_scaled_jacobian=.04;
  CHECK_FALSE(ValidateMetricChange(1,before,1,bad,MetricChange::GeometryRepair,reason));
  bad=before;bad.edges.begin()->second.length=2;bad.edges.begin()->second.a.x-=1;
  CHECK_FALSE(ValidateMetricChange(1,before,1,bad,MetricChange::GeometryRepair,reason));
}

TEST_CASE("Geometric refinement may defer metric excess to final validation", "[NativeMesh3D][NativeShapeAdmission3D][NativePolicy3D]") {
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  // This exercises the metric subgate only; production callers separately certify strict geometry-error reduction.
  const auto old=Single();std::vector<Cell> fresh;std::string reason;
  REQUIRE(Cone({old},{4,{.25,.25,.25}},100,fresh,reason));
  for(const auto& cell:fresh)CHECK(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
  const auto donor=Donor({9,0,0,9,0,9});
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  FrozenField construction({donor});PatchMetric edge_only;
  CHECK(ValidateMetricChange({old},fresh,MetricChange::GeometryRefine,construction,edge_only,reason));
  CHECK_FALSE(edge_only.shape_measured);CHECK(construction.Statistics().requests==0);
  PatchMetric final_metric;
  CHECK_FALSE(ValidateMetricPatch(fresh,construction,final_metric,reason));
  CHECK(final_metric.shape_measured);CHECK(reason.find("oversized")!=std::string::npos);

  SetShapeAdmission(ShapeAdmission::Absolute);
  FrozenField diagnostic({donor});PatchMetric measured;MetricDiagnosis diagnosis;
  CHECK_FALSE(ValidateMetricChange({old},fresh,MetricChange::GeometryRefine,diagnostic,measured,reason,
                                   nullptr,nullptr,&diagnosis));
  CHECK(diagnosis.failure==MetricFailure::Progress);
  CHECK(reason.find("adds frozen metric excess")!=std::string::npos);
}

TEST_CASE("Native3D metric movement honours directional stiffness without changing its target", "[NativeMetricMove3D]") {
  const Point origin{0,0,0};
  std::vector<std::pair<Point,Tensor>> neighbors{{{1,0,0},{}},{{0,1,0},{}},{{0,0,1},{}}};
  const auto euclidean=MetricMeanPoint(origin,neighbors);
  CHECK(euclidean.x==Approx(1./3));CHECK(euclidean.y==Approx(1./3));CHECK(euclidean.z==Approx(1./3));
  neighbors.back().second.zz=30.25;const auto graded=MetricMeanPoint(origin,neighbors);
  CHECK(graded.x==Approx(1./3));CHECK(graded.y==Approx(1./3));CHECK(graded.z==Approx(30.25/32.25));
  CHECK(neighbors.back().second.zz==30.25);CHECK_THROWS(MetricMeanPoint(origin,{}));
  std::vector<std::pair<Point,Tensor>> large(257,{{1,2,3},{}});
  CHECK_THROWS(MetricMeanPoint(origin,large));
  const auto admitted=MetricMeanPoint(origin,large,large.size());
  CHECK(admitted.x==Approx(1));CHECK(admitted.y==Approx(2));CHECK(admitted.z==Approx(3));
  large[0].second.xx=-1;CHECK_THROWS(MetricMeanPoint(origin,large,large.size()));
  large[0].second.xx=std::numeric_limits<double>::quiet_NaN();
  CHECK_THROWS(MetricMeanPoint(origin,large,large.size()));
}

TEST_CASE("Native actual-centroid q/J Jacobian matches anisotropic spatial finite differences", "[NativeMetricJacobian3D]") {
  for(double scale:{.01,1.,100.})for(double aspect:{1.,10.,100.})for(bool varying:{false,true}) {
    Tetrahedron tet{{{2.3*scale,-1.4*scale,.7*scale},{3.4*scale,-1.36*scale,.7*scale},
                     {2.45*scale,-.5*scale,.7*scale},{2.43*scale,-1.18*scale,(.7+.005/aspect)*scale}}};
    const Tensor metric{4,.6,.2*aspect,3,.3*aspect,2*aspect*aspect};
    std::array<Tensor,3> spatial{{{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}}};
    if(varying) {
      spatial[0].xx=.05/scale;spatial[1].yy=.07/scale;spatial[1].xz=.03*aspect/scale;
      spatial[2].zz=.09*aspect*aspect*aspect/scale;
    }
    const auto linear=LinearizeMetricShape(tet,metric,&spatial);
    CHECK(linear.residual[0]>0);CHECK(linear.residual[1]>0);
    for(unsigned vertex=0;vertex<4;++vertex)for(unsigned axis=0;axis<3;++axis) {
      const double h=1e-7*scale/(axis==2?aspect:1);auto plus=tet,minus=tet;
      std::array<double*,3> high{{&plus[vertex].x,&plus[vertex].y,&plus[vertex].z}},low{{&minus[vertex].x,&minus[vertex].y,&minus[vertex].z}};
      *high[axis]+=h;*low[axis]-=h;
      const std::array<double,3> original{{tet[vertex].x,tet[vertex].y,tet[vertex].z}};
      auto sample=[&](const Tetrahedron& t,double coordinate) {
        const auto& d=spatial[axis];const long double a=(static_cast<long double>(coordinate)-original[axis])/4;
        const Tensor m{double(metric.xx+a*d.xx),double(metric.xy+a*d.xy),double(metric.xz+a*d.xz),
                       double(metric.yy+a*d.yy),double(metric.yz+a*d.yz),double(metric.zz+a*d.zz)};
        return LinearizeMetricShape(t,m).residual;
      };
      const auto a=sample(plus,*high[axis]),b=sample(minus,*low[axis]);
      for(unsigned row=0;row<2;++row) {
        const auto fd=(a[row]-b[row])/(*high[axis]-*low[axis]);
        INFO(scale);INFO(aspect);INFO(varying);INFO(vertex);INFO(axis);INFO(row);
        CHECK(std::abs(fd-linear.derivative[row][vertex][axis])<=3e-5*std::max(1.,std::abs(fd)));
      }
    }
  }
  auto inverted=Single().v;std::swap(inverted[0],inverted[1]);
  CHECK_THROWS(LinearizeMetricShape({inverted[0].p,inverted[1].p,inverted[2].p,inverted[3].p},{}));
  const auto c=Single();CHECK_THROWS(LinearizeMetricShape({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},{-1,0,0,1,0,1}));
}


TEST_CASE("Metric shape ranking shares exact centroid arithmetic without certifying edges", "[NativeShapeRanking3D]") {
  size_t compositions=0;
  const MetricComposition compose=[&](Point p,Tensor m){++compositions;m.xx+=p.x;return m;};
  FrozenField target({Donor({})},compose,false,{},[](Point,Point){return std::vector<double>{0,1};});
  const auto shape=MetricShapeQuality({Single()},target);CHECK(compositions==1);CHECK(target.Statistics().edge_requests==0);
  const auto full=MeasureMetricCells({Single()},target);
  CHECK(shape==std::min(full.minimum_mean_ratio/MinimumMeanRatio,full.minimum_scaled_jacobian/MinimumScaledJacobian));
  auto weak=Single();weak.v[3].p.z=.01;
  CHECK(MetricShapeQuality({weak},target)<shape);
  CHECK_THROWS(MetricShapeQuality({},target));CHECK_THROWS(MetricShapeQuality({Single(),Single()},target));
  auto outside=Single();outside.v[3].p.z=10;CHECK_THROWS(MetricShapeQuality({outside},target));
  // A shape score cannot authorize a composed target whose edge policy is missing.
  FrozenField unresolved({Donor({})},compose);CHECK(std::isfinite(MetricShapeQuality({Single()},unresolved)));
  PatchMetric measured;std::string reason;CHECK_FALSE(ValidateMetricPatch({Single()},unresolved,measured,reason));
  CHECK(reason.find("geometry-resolved")!=std::string::npos);
}

TEST_CASE("Repair campaign edge certificates require exact coordinates and immutable full field", "[NativeCampaignEdge3D]") {
  const auto identity=std::make_shared<const FrozenMetricIdentity>();
  const MetricComposition composition=[](Point p,Tensor m){m.xx+=.25*p.x;return m;};
  const MetricEdgeBreakpoints cuts=[](Point,Point){return std::vector<double>{0,1};};
  FrozenField first({Donor({})},composition,false,{},cuts,identity);
  std::map<Edge,EdgeMeasure> campaign;MetricStats cold;
  const auto original=MeasureMetricCells({Single()},first,&cold,nullptr,false,nullptr,&campaign);
  CHECK(cold.edge_evaluations==6);CHECK(campaign.size()==6);
  FrozenField imported({Donor({})},composition,false,{},cuts,identity);MetricStats warm;
  const auto repeated=MeasureMetricCells({Single()},imported,&warm,nullptr,false,nullptr,&campaign);
  CHECK(warm.edge_evaluations==0);CHECK(warm.edge_campaign_reused==6);
  CHECK(imported.Statistics().edge_requests==0);CHECK(imported.Statistics().compositions==1);
  CHECK(repeated.minimum_mean_ratio==original.minimum_mean_ratio);
  for(const auto& e:repeated.edges)CHECK(e.second.length==original.edges.at(e.first).length);
  auto moved=Single();moved.v[1].p.x=.5;MetricStats changed;
  const auto result=MeasureMetricCells({moved},imported,&changed,nullptr,false,nullptr,&campaign);
  CHECK(changed.edge_evaluations==3);CHECK(changed.edge_campaign_reused==3);
  const auto reference=MeasureMetricCells({moved},imported);
  for(const auto& e:result.edges)CHECK(e.second.length==reference.edges.at(e.first).length);
  campaign.at({0,2}).length=std::numeric_limits<long double>::quiet_NaN();MetricStats malformed;
  MeasureMetricCells({moved},imported,&malformed,nullptr,false,nullptr,&campaign);
  CHECK(malformed.edge_evaluations==1);CHECK(malformed.edge_campaign_reused==5);
  FrozenField stage({Donor({4,0,0,4,0,4})},composition,false,{},cuts);MetricStats refreshed;
  MeasureMetricCells({moved},stage,&refreshed,nullptr,false,nullptr,&campaign);
  CHECK(refreshed.edge_evaluations==6);CHECK(refreshed.edge_campaign_reused==0);
  FrozenField unsupported({Donor({})},composition,false,{}, {},identity);
  CHECK_THROWS(MeasureMetricCells({moved},unsupported,nullptr,nullptr,false,nullptr,&campaign));
  campaign.clear();auto flat=Single();flat.v[3].p.z=1e-8;
  CHECK_THROWS(MeasureMetricCells({flat},first,nullptr,nullptr,true,nullptr,&campaign));
  CHECK(campaign.empty()); // Shape rejection never creates edge certificates.
  const EdgeMeasure stale{{0,0,0},{0,0,0},-1,{}};
  for(size_t i=0;i<MaximumCampaignEdges;++i)campaign.emplace(Edge{100+2*i,101+2*i},stale);
  MeasureMetricCells({Single()},first,nullptr,nullptr,false,nullptr,&campaign);
  CHECK(campaign.size()==MaximumCampaignEdges);
}

TEST_CASE("Metric query planning shares exact reuse and protects bounded campaign certificates", "[NativeMetricPlan3D]") {
  auto identity=std::make_shared<const FrozenMetricIdentity>();FrozenField field({Donor({})},{},false,{}, {},identity);
  const auto reference=MeasureMetricCells({Single()},field);auto reuse=reference.edges;
  CHECK(MetricEdgeRequests({Single()},reuse,identity)==std::vector<unsigned char>{0});
  std::map<Edge,EdgeMeasure> empty;CHECK(MetricEdgeRequests({Single()},empty,identity)==std::vector<unsigned char>{63});
  auto stage=std::make_shared<const FrozenMetricIdentity>();reuse=reference.edges;
  CHECK(MetricEdgeRequests({Single()},reuse,stage)==std::vector<unsigned char>{63});
  auto moved=Single();moved.v[1].p.x=.875;reuse=reference.edges;
  CHECK(MetricEdgeRequests({moved},reuse,identity)==std::vector<unsigned char>{25});
  reuse=reference.edges;reuse.at({0,1}).length=NAN;reuse.at({0,2}).length=0;
  CHECK(MetricEdgeRequests({Single()},reuse,identity)==std::vector<unsigned char>{3});
  auto reversed=Single();std::swap(reversed.v[0],reversed.v[1]);reuse=reference.edges;
  CHECK(MetricEdgeRequests({reversed},reuse,identity)==std::vector<unsigned char>{0});
  auto campaign=reference.edges;reuse.clear();
  CHECK(MetricEdgeRequests({Single()},reuse,identity,&campaign)==std::vector<unsigned char>{0});
  campaign.clear(); // Eviction after planning cannot invalidate the protected batch.
  MetricStats counted;const auto measured=MeasureMetricCells({Single()},field,&counted,&reuse);
  CHECK(counted.edge_evaluations==0);CHECK(counted.edge_reused==6);
  CHECK(measured.maximum_length==reference.maximum_length);
  CHECK_THROWS(MetricEdgeRequests({Single()},reuse,{}));
  CHECK_THROWS(MetricEdgeRequests(std::vector<Cell>(MaximumReplacementCells+1),reuse,identity));
}

TEST_CASE("Native 3D SplitInsertion requires a descending edge frontier", "[NativeMetricSplitRule3D]") {
  struct Restore { RefineAcceptance a=GetRefineAcceptance(); ~Restore(){SetRefineAcceptance(a);} } restore;
  struct RestoreShape {ShapeAdmission a=GetShapeAdmission();~RestoreShape(){SetShapeAdmission(a);}} restore_shape;
  SetShapeAdmission(ShapeAdmission::Absolute);
  auto metric=[](std::vector<std::pair<Edge,long double>> rows,double q) {
    PatchMetric m;m.shape_measured=true;m.minimum_mean_ratio=q;m.minimum_scaled_jacobian=q;
    for(const auto& r:rows){EdgeMeasure e;e.a={double(r.first[0]),0,0};e.b={double(r.first[1]),1,0};e.length=r.second;m.edges.emplace(r.first,e);}
    return m;};
  // a long edge (1,2) split at 4; one replacement edge is longer than the removed frontier
  const auto before=metric({{{1,2},3.0L},{{1,3},1.0L},{{2,3},1.0L}},.5);
  const auto split=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,4},1.5L},{{2,4},1.5L},{{3,4},3.5L}},.5);
  // Every replacement edge is shorter, although their aggregate excess is greater than the removed edge's.
  const auto debt=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,4},2.9L},{{2,4},2.9L},{{3,4},2.9L}},.5);
  std::string reason;MetricDiagnosis d;
  SetRefineAcceptance(RefineAcceptance::StrictExcess);
  CHECK_FALSE(ValidateMetricChange(1,before,2,split,MetricChange::Refine,reason,&d));
  CHECK_FALSE(ValidateMetricChange(1,before,2,debt,MetricChange::Refine,reason,&d));
  SetRefineAcceptance(RefineAcceptance::SplitInsertion);
  CHECK_FALSE(ValidateMetricChange(1,before,2,split,MetricChange::Refine,reason,&d));
  CHECK(reason.find("longest changed-edge frontier")!=std::string::npos);
  CHECK(ValidateMetricChange(1,before,2,debt,MetricChange::Refine,reason,&d));
  CHECK(d.size_insertion);CHECK_FALSE(d.vertex_removal);CHECK(d.added_excess>d.removed_excess);CHECK(d.size_progress);
  // no cell-count condition for a size insertion (a refit may keep the tetrahedron count)
  CHECK(ValidateMetricChange(2,before,2,debt,MetricChange::Refine,reason,&d));
  // strict shape is unchanged
  auto sliver=split;sliver.minimum_mean_ratio=.1;
  CHECK_FALSE(ValidateMetricChange(1,before,2,sliver,MetricChange::Refine,reason,&d));
  // no net vertex gain (absorption/collapse): strict excess decrease is required
  const auto before4=metric({{{1,2},3.0L},{{1,3},1.0L},{{2,3},1.0L},{{1,4},1.0L},{{3,4},1.0L}},.5);
  const auto worse=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,5},1.5L},{{2,5},1.5L},{{3,5},3.5L}},.5);
  const auto better=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,5},1.5L},{{2,5},1.5L},{{3,5},1.5L}},.5);
  CHECK_FALSE(ValidateMetricChange(2,before4,3,worse,MetricChange::Refine,reason,&d));CHECK(d.vertex_removal);
  CHECK(ValidateMetricChange(2,before4,3,better,MetricChange::Refine,reason,&d));
  // non-size insertion (split edge <= 1.6): no added oversized edge and a shape improvement
  const auto shortBefore=metric({{{1,2},1.5L},{{1,3},1.0L},{{2,3},1.0L}},.4);
  const auto shortSame=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,4},.75L},{{2,4},.75L},{{3,4},1.0L}},.4);
  const auto shortBetter=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,4},.75L},{{2,4},.75L},{{3,4},1.0L}},.6);
  const auto shortLong=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,4},.75L},{{2,4},.75L},{{3,4},2.0L}},.6);
  CHECK_FALSE(ValidateMetricChange(1,shortBefore,2,shortSame,MetricChange::Refine,reason,&d));
  CHECK(ValidateMetricChange(1,shortBefore,2,shortBetter,MetricChange::Refine,reason,&d));CHECK_FALSE(d.size_insertion);
  CHECK_FALSE(ValidateMetricChange(1,shortBefore,2,shortLong,MetricChange::Refine,reason,&d));
  // Validity-only construction still requires a descending edge frontier but never requires centroid shape data.
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  auto edge_before=before,edge_debt=debt;edge_before.shape_measured=edge_debt.shape_measured=false;
  edge_before.minimum_mean_ratio=edge_before.minimum_scaled_jacobian=std::numeric_limits<double>::quiet_NaN();
  edge_debt.minimum_mean_ratio=edge_debt.minimum_scaled_jacobian=std::numeric_limits<double>::quiet_NaN();
  CHECK(ValidateMetricChange(1,edge_before,2,edge_debt,MetricChange::Refine,reason,&d));
  CHECK_FALSE(d.quality_progress);CHECK(d.size_insertion);CHECK(d.added_excess>d.removed_excess);
  // other intents are unchanged by the policy
  SetShapeAdmission(ShapeAdmission::Absolute);
  CHECK_FALSE(ValidateMetricChange(1,before,2,split,MetricChange::Repair,reason,&d));
  CHECK_FALSE(ValidateMetricChange(1,before,2,split,MetricChange::GeometryRefine,reason,&d));
}

TEST_CASE("Native 3D split rule counts only caller-certified relocations as moves", "[NativeMetricSplitRule3D]") {
  struct Restore { RefineAcceptance a=GetRefineAcceptance(); ~Restore(){SetRefineAcceptance(a);} } restore;
  struct RestoreShape {ShapeAdmission a=GetShapeAdmission();~RestoreShape(){SetShapeAdmission(a);}} restore_shape;
  SetRefineAcceptance(RefineAcceptance::SplitInsertion);
  SetShapeAdmission(ShapeAdmission::Absolute);
  auto metric=[](std::vector<std::pair<Edge,long double>> rows) {
    PatchMetric m;m.shape_measured=true;m.minimum_mean_ratio=.5;m.minimum_scaled_jacobian=.5;
    for(const auto& r:rows){EdgeMeasure e;e.a={double(r.first[0]),0,0};e.b={double(r.first[1]),1,0};e.length=r.second;m.edges.emplace(r.first,e);}
    return m;};
  // long edge (1,2) split at 5; apex 4 relocated to 6; aggregate excess grows while the longest new edge shrinks
  const auto before=metric({{{1,2},3.0L},{{1,3},1.0L},{{2,3},1.0L},{{1,4},1.0L},{{2,4},1.0L},{{3,4},1.0L}});
  const auto fresh=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,5},2.9L},{{2,5},2.9L},{{3,5},2.9L},{{1,6},1.0L},{{2,6},1.0L},{{5,6},1.0L},{{3,6},1.0L}});
  std::string reason;MetricDiagnosis d;
  CHECK_FALSE(ValidateMetricChange(2,before,4,fresh,MetricChange::Refine,reason,&d));CHECK(d.vertex_removal);
  CHECK(ValidateMetricChange(2,before,4,fresh,MetricChange::Refine,reason,&d,1));CHECK(d.size_insertion);
  // a certified relocation cannot hide an absorption without net vertex gain
  const auto absorbed=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,6},1.0L},{{2,6},1.0L},{{3,6},3.5L}});
  CHECK_FALSE(ValidateMetricChange(2,before,2,absorbed,MetricChange::Refine,reason,&d,1));CHECK(d.vertex_removal);
}

// ---- DC-12 (OPTIMIZE_PLAN): per-operation shape admission and the mesh-wide metric optimizer ----------------------------
TEST_CASE("Native 3D shape admission: validity-only generation keeps the final gate absolute", "[NativeShapeAdmission3D]") {
  struct Restore { ShapeAdmission a=GetShapeAdmission(); ~Restore(){SetShapeAdmission(a);} } restore;
  CHECK(GetShapeAdmission()==ShapeAdmission::Absolute);  // production default
  auto metric=[](double q,double J) {
    PatchMetric m;m.shape_measured=true;m.minimum_mean_ratio=q;m.minimum_scaled_jacobian=J;
    // the long edge (1,2) is split at 4 (a size insertion): it is not among the fresh edges
    for(const auto& r:std::vector<std::pair<Edge,long double>>{{{1,3},1.0L},{{2,3},1.0L},{{1,4},1.5L},{{2,4},1.5L},{{3,4},1.2L}}){
      EdgeMeasure e;e.a={double(r.first[0]),0,0};e.b={double(r.first[1]),1,0};e.length=r.second;m.edges.emplace(r.first,e);}
    return m;};
  PatchMetric before;before.shape_measured=true;before.minimum_mean_ratio=.5;before.minimum_scaled_jacobian=.5;
  for(const auto& r:std::vector<std::pair<Edge,long double>>{{{1,2},3.0L},{{1,3},1.0L},{{2,3},1.0L}}){EdgeMeasure e;e.a={double(r.first[0]),0,0};e.b={double(r.first[1]),1,0};e.length=r.second;before.edges.emplace(r.first,e);}
  std::string reason;MetricDiagnosis d;
  const auto poor=metric(.1,.03),sliver=metric(.1,5e-4),flat=metric(0,0);
  SetShapeAdmission(ShapeAdmission::Absolute);
  CHECK_FALSE(ValidateMetricChange(1,before,2,poor,MetricChange::Refine,reason,&d));CHECK(d.failure==MetricFailure::Shape);
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  CHECK(ValidateMetricChange(1,before,2,poor,MetricChange::Refine,reason,&d));  // a valid size insertion
  CHECK(ValidateMetricChange(1,before,2,sliver,MetricChange::Refine,reason,&d));
  CHECK(ValidateMetricChange(1,before,2,flat,MetricChange::Refine,reason,&d));
  // progress rules are unchanged: a coarsening that adds cells is still refused
  CHECK_FALSE(ValidateMetricChange(1,before,2,poor,MetricChange::Coarsen,reason,&d));CHECK(d.failure==MetricFailure::Progress);
  // Construction repair uses the optimizer's worst normalized quality; spare Jacobian margin may repair q.
  const auto weak_q=metric(.19,.1),repaired_q=metric(.21,.08),weaker_J=metric(.22,.051);
  CHECK(ValidateMetricChange(1,weak_q,1,repaired_q,MetricChange::Repair,reason,&d));CHECK(d.quality_progress);
  CHECK_FALSE(ValidateMetricChange(1,repaired_q,1,weak_q,MetricChange::Repair,reason,&d));
  CHECK_FALSE(ValidateMetricChange(1,repaired_q,1,weaker_J,MetricChange::Repair,reason,&d));
  // Explicit full measurements report poor final shape; construction edge measurements do not query centroids.
  DonorCell donor;donor.cell={0,{{{0,{-10,-10,-10}},{1,{40,-10,-10}},{2,{-10,40,-10}},{3,{-10,-10,40}}}}};donor.sensor.fill(Tensor{1,0,0,1,0,.0004});  // z compressed by 50: q ~ .06, J ~ .01
  FrozenField field({donor});
  const Cell cell{7,{{{10,{0,0,0}},{11,{1,0,0}},{12,{0,1,0}},{13,{0,0,1}}}}};
  PatchMetric measured;
  const auto shape=MeasureMetricCells({cell},field,nullptr,nullptr,false);
  REQUIRE(shape.minimum_mean_ratio<MinimumMeanRatio);REQUIRE(shape.minimum_scaled_jacobian>=ValiditySliverJacobian);
  SetShapeAdmission(ShapeAdmission::Absolute);CHECK_THROWS(MeasureMetricCells({cell},field,nullptr,nullptr,true));
  SetShapeAdmission(ShapeAdmission::ValidityOnly);CHECK_NOTHROW(MeasureMetricCells({cell},field,nullptr,nullptr,true));
  CHECK_FALSE(ValidateMetricPatch({cell},field,measured,reason));CHECK(reason.find("shape/sliver")!=std::string::npos);
  SetShapeAdmission(ShapeAdmission::Absolute);
}
namespace {
// Constant-metric target covering [-1,3]^3 (sensor only: exact analytic edge lengths).
DonorCell CoveringDonor(Tensor metric){DonorCell d;d.cell={0,{{{0,{-10,-10,-10}},{1,{40,-10,-10}},{2,{-10,40,-10}},{3,{-10,-10,40}}}}};d.sensor.fill(metric);return d;}
Owned MeshOf(const std::vector<std::array<Node,4>>& cells) {
  Owned out;Id id=100;
  for(auto v:cells){Cell c{id,v};if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[0],c.v[1]);OwnedCell row;row.cell=c;out.emplace(id++,row);}
  // skin faces carry a physical binding (facet = 1000 + running index), as an accepted mesh's boundary
  std::map<Face,std::pair<Id,unsigned>> once;std::map<Face,int> uses;
  for(const auto& r:out){const auto& v=r.second.cell.v;const std::array<Face,4> f{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
    for(unsigned k=0;k<4;++k){auto s=f[k];std::sort(s.begin(),s.end());++uses[s];once[s]={r.first,k};}}
  Id facet=1000;for(const auto& u:uses)if(u.second==1){auto& row=out.at(once.at(u.first).first);row.physical[once.at(u.first).second]={true,facet++};}
  return out;
}
struct MeshFacts { size_t cells=0,bad=0;double min=1e300;long double volume=0;std::map<Face,Id> skin;bool positive=true; };
MeshFacts Facts(const Owned& mesh,FrozenField& field) {
  MeshFacts f;f.cells=mesh.size();
  for(const auto& r:mesh){const auto& c=r.second.cell;const auto o=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p);f.positive=f.positive&&o>0;f.volume+=o;
    const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},field.Query(Centroid(c)).target);const double s=std::min(m.mean_ratio/MinimumMeanRatio,m.minimum_scaled_jacobian/MinimumScaledJacobian);
    f.min=std::min(f.min,s);f.bad+=s<1;
    const auto& v=c.v;const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
    for(unsigned k=0;k<4;++k)if(r.second.physical[k].present){auto s2=faces[k];std::sort(s2.begin(),s2.end());f.skin[s2]=r.second.physical[k].facet;}}
  return f;
}
std::array<Node,4> Tet(Node a,Node b,Node c,Node d){return {a,b,c,d};}
}  // namespace
TEST_CASE("Native 3D optimizer: edge removal 3-2 and n -> 2n-4 with the optimal ring triangulation", "[NativeOptimize3D]") {
  FrozenField field({CoveringDonor({1,0,0,1,0,1})});const RegionMetric target(field);
  const double pi=std::acos(-1.);
  auto ringOf=[&](size_t n,double R,Id first,double scale){std::vector<Node> r;for(size_t i=0;i<n;++i){const double t=2*pi*double(i)/double(n);r.push_back({first+i,{scale*R*std::cos(t),scale*R*std::sin(t),0}});}return r;};
  OptimizeOptions options;options.smoothing=false;options.collapses=false;options.insertions=false;
  // bipyramid: three needle cells around the long axis a-b (score .81) -> two cells (2.5)
  {const auto r=ringOf(3,.25,10,1);const Node a{1,{0,0,1.2}},b{2,{0,0,-1.2}};
   std::vector<std::array<Node,4>> cells;for(int i=0;i<3;++i)cells.push_back(Tet(a,b,r[i],r[(i+1)%3]));
   Owned mesh=MeshOf(cells);const auto before=Facts(mesh,field);REQUIRE(before.bad==3);
   OptimizeReport report;Id nn=100,nc=1000;
   CHECK(OptimizeMetricMesh(mesh,target,nn,nc,options,report));const auto after=Facts(mesh,field);
   CHECK(report.edge_removals==1);CHECK(after.cells==2);CHECK(after.positive);CHECK(after.bad==0);
   CHECK(after.min>before.min);CHECK(after.skin==before.skin);CHECK(std::abs(double(after.volume-before.volume))<=1e-12*std::abs(double(before.volume)));}
  // pentagon ring around a long axis: 5 -> 6 cells (best triangulation by dynamic programming)
  {const auto r=ringOf(5,.25,20,1);const Node a{3,{0,0,1.2}},b{4,{0,0,-1.2}};
   std::vector<std::array<Node,4>> cells;for(int i=0;i<5;++i)cells.push_back(Tet(a,b,r[i],r[(i+1)%5]));
   Owned mesh=MeshOf(cells);const auto before=Facts(mesh,field);REQUIRE(before.bad>0);
   OptimizeReport report;Id nn=100,nc=1000;
   CHECK(OptimizeMetricMesh(mesh,target,nn,nc,options,report));const auto after=Facts(mesh,field);
   CHECK(report.edge_removals==1);CHECK(after.cells==6);CHECK(after.positive);CHECK(after.min>before.min);CHECK(after.skin==before.skin);}
  // edge guard: the same pentagon three times larger; a target whose exact length of every ring diagonal is 10 (above
  // max(1.8, axis 7.2)) refuses the removal, the plain target accepts it
  {const auto r=ringOf(5,.25,20,3);const Node a{3,{0,0,3.6}},b{4,{0,0,-3.6}};
   std::vector<std::array<Node,4>> cells;for(int i=0;i<5;++i)cells.push_back(Tet(a,b,r[i],r[(i+1)%5]));
   const RegionMetric guarded([&field](Point p){return field.Query(p).target;},
     [&field](Point x,Point y){return (std::abs(x.z)<1e-9&&std::abs(y.z)<1e-9)?10.L:field.TargetEdgeLength(x,y);});
   Owned mesh=MeshOf(cells);OptimizeReport report;Id nn=100,nc=1000;
   CHECK_FALSE(OptimizeMetricMesh(mesh,guarded,nn,nc,options,report));CHECK(report.edge_removals==0);CHECK(report.rejected_edge>0);CHECK(mesh.size()==5);
   Owned plain=MeshOf(cells);OptimizeReport r2;nn=100;nc=1000;
   CHECK(OptimizeMetricMesh(plain,target,nn,nc,options,r2));CHECK(r2.edge_removals==1);CHECK(plain.size()==6);}
  // face removal 2-3: two cells sharing a face (bad flat cell .68) -> three cells around the new edge (1.81)
  {const Node r0{40,{0,0,0}},r1{41,{1,0,0}},r2{42,{.5,.866,0}},d{43,{1.15,.73,.89}},e{44,{.48,.44,-.023}};
   Owned mesh=MeshOf({Tet(d,r0,r1,r2),Tet(e,r0,r2,r1)});const auto before=Facts(mesh,field);REQUIRE(before.bad>0);
   OptimizeReport report;Id nn=100,nc=1000;
   CHECK(OptimizeMetricMesh(mesh,target,nn,nc,options,report));const auto after=Facts(mesh,field);
   CHECK(report.face_swaps==1);CHECK(after.cells==3);CHECK(after.positive);CHECK(after.min>before.min);CHECK(after.skin==before.skin);}
}
TEST_CASE("Native 3D optimizer: smoothing, collapse, frozen sets, skin and determinism", "[NativeOptimize3D]") {
  FrozenField field({CoveringDonor({1,0,0,1,0,1})});const RegionMetric target(field);
  // unit cube, 8 corners + one interior vertex pushed towards a corner (score .77): 12 boundary triangles coned from it
  std::array<Node,8> c;for(int i=0;i<8;++i)c[i]={Id(1+i),{double(i&1),double((i>>1)&1),double((i>>2)&1)}};
  const std::array<std::array<int,4>,6> quads{{{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}}};
  auto cone=[&](Node x){std::vector<std::array<Node,4>> cells;for(const auto& f:quads){cells.push_back(Tet(x,c[f[0]],c[f[1]],c[f[2]]));cells.push_back(Tet(x,c[f[0]],c[f[2]],c[f[3]]));}return cells;};
  const auto cells=cone({20,{.95,.93,.9}});
  OptimizeOptions options;options.swaps=false;options.collapses=false;options.insertions=false;
  {Owned mesh=MeshOf(cells);const auto before=Facts(mesh,field);REQUIRE(before.bad>0);
   OptimizeReport report;Id nn=100,nc=1000;
   CHECK(OptimizeMetricMesh(mesh,target,nn,nc,options,report));const auto after=Facts(mesh,field);
   CHECK(report.moves>=1);CHECK(after.min>before.min);CHECK(after.skin==before.skin);CHECK(after.positive);CHECK(report.relabel.count(20)==1);
   CHECK(std::abs(double(after.volume-before.volume))<=1e-12*std::abs(double(before.volume)));}
  // Every admitted star gets a coordinate pass before any enrichment, with all operators enabled.
  {auto pair=cells;for(auto tet:cells){for(auto& v:tet){v.id+=100;v.p.x+=3;}pair.push_back(tet);}
   Owned mesh=MeshOf(pair);const auto before=Facts(mesh,field);OptimizeOptions staged;staged.maximum_sweeps=1;staged.maximum_work=160;
   OptimizeReport report;Id nn=1000,nc=10000;
   REQUIRE(OptimizeMetricMesh(mesh,target,nn,nc,staged,report));const auto after=Facts(mesh,field);
   CHECK(report.moves==2);CHECK(report.relabel.count(20)==1);CHECK(report.relabel.count(120)==1);
   CHECK(report.shell_insertions+report.neighbourhood_insertions+report.length_insertions==0);
   CHECK(report.collapses==0);CHECK(after.cells==before.cells);CHECK(after.bad==0);CHECK(after.positive);CHECK(after.skin==before.skin);
   CHECK(report.trials<=staged.maximum_work);}
  // A refused coordinate pass exhausts only its share; enrichment still gets its reserved trial.
  {auto scaled=cells;for(auto& tet:scaled)for(auto& v:tet){v.p.x*=3;v.p.y*=3;v.p.z*=3;}
   const RegionMetric refusing([&field](Point p){return field.Query(p).target;},[](Point,Point){return 100.L;});
   Owned mesh=MeshOf(scaled);const auto before=Facts(mesh,field);OptimizeOptions staged;
   staged.swaps=staged.collapses=false;staged.maximum_sweeps=1;staged.maximum_work=2;
   OptimizeReport report;Id nn=100,nc=1000;
   CHECK_FALSE(OptimizeMetricMesh(mesh,refusing,nn,nc,staged,report));
   CHECK(report.trials==2);CHECK(report.placements==2);CHECK(report.rejected_edge>0);CHECK(report.moves==0);CHECK(report.stop=="limit");
   CHECK(mesh.size()==before.cells);CHECK(Facts(mesh,field).skin==before.skin);CHECK(nn==100);CHECK(nc==1000);}
  // a frozen vertex is never moved; frozen cells are never replaced
  {Owned mesh=MeshOf(cells);OptimizeOptions frozen=options;frozen.frozen_vertices={20};OptimizeReport report;Id nn=100,nc=1000;
   CHECK_FALSE(OptimizeMetricMesh(mesh,target,nn,nc,frozen,report));CHECK(report.moves==0);}
  {Owned mesh=MeshOf(cells);OptimizeOptions all;for(const auto& row:mesh)all.frozen_cells.insert(row.first);OptimizeReport report;Id nn=100,nc=1000;
   const Owned copy=mesh;CHECK_FALSE(OptimizeMetricMesh(mesh,target,nn,nc,all,report));CHECK(mesh.size()==copy.size());
   for(const auto& row:copy)CHECK(mesh.count(row.first)==1);}
  // collapse: a second interior vertex 1e-3 above the centre creates four slivers that a contraction removes
  auto sliverMesh=[&]{const Node y{21,{.5,.5,.5}},z{22,{.5,.5,.501}};auto m=cone(y);
    m[2]=Tet(z,c[4],c[6],c[7]);m[3]=Tet(z,c[4],c[7],c[5]);
    m.push_back(Tet(y,z,c[4],c[6]));m.push_back(Tet(y,z,c[6],c[7]));m.push_back(Tet(y,z,c[7],c[5]));m.push_back(Tet(y,z,c[5],c[4]));return m;};
  {Owned mesh=MeshOf(sliverMesh());const auto before=Facts(mesh,field);REQUIRE(before.positive);REQUIRE(before.bad>0);
   OptimizeOptions col;col.swaps=false;col.smoothing=false;col.insertions=false;OptimizeReport report;Id nn=100,nc=1000;
   CHECK(OptimizeMetricMesh(mesh,target,nn,nc,col,report));const auto after=Facts(mesh,field);
   CHECK(report.collapses>=1);CHECK(after.min>before.min);CHECK(after.skin==before.skin);CHECK(after.positive);CHECK(after.cells==12);}
  // determinism: the same input gives the same mesh (all operations)
  {Owned m1=MeshOf(sliverMesh()),m2=MeshOf(sliverMesh());OptimizeReport q1,q2;Id a1=100,b1=1000,a2=100,b2=1000;const OptimizeOptions every;
   OptimizeMetricMesh(m1,target,a1,b1,every,q1);OptimizeMetricMesh(m2,target,a2,b2,every,q2);
   REQUIRE(m1.size()==m2.size());bool same=true;auto i1=m1.begin();auto i2=m2.begin();
   for(;i1!=m1.end();++i1,++i2)for(unsigned k=0;k<4;++k)same=same&&i1->first==i2->first&&i1->second.cell.v[k].id==i2->second.cell.v[k].id&&i1->second.cell.v[k].p.x==i2->second.cell.v[k].p.x;
   CHECK(same);}
}
TEST_CASE("Native 3D optimizer: boundary edge flip inside one facet", "[NativeOptimize3D]") {
  FrozenField field({CoveringDonor({1,0,0,1,0,1})});const RegionMetric target(field);
  // two cells over a planar boundary quad a-r0-b-rm split along a-b (score .78); the flip to r0-rm gives 2.48
  const Node a{1,{0,0,0}},b{2,{1,.05,0}},r0{3,{.25,-.065,0}},rm{4,{.55,.55,0}},x{5,{.056,.382,.282}};
  auto build=[&](bool sameFacet){Owned mesh=MeshOf({Tet(a,b,r0,x),Tet(a,b,x,rm)});
    if(sameFacet)for(auto& row:mesh){const auto& v=row.second.cell.v;
      for(unsigned k=0;k<4;++k)if(row.second.physical[k].present&&v[k].id==x.id)row.second.physical[k].facet=7;}  // z = 0 faces: one facet
    return mesh;};
  OptimizeOptions options;options.smoothing=false;options.collapses=false;options.insertions=false;
  {Owned mesh=build(true);const auto before=Facts(mesh,field);REQUIRE(before.bad>0);OptimizeReport report;Id nn=100,nc=1000;
   CHECK(OptimizeMetricMesh(mesh,target,nn,nc,options,report));const auto after=Facts(mesh,field);
   CHECK(report.boundary_flips==1);CHECK(after.cells==2);CHECK(after.positive);CHECK(after.min>before.min);
   CHECK(std::abs(double(after.volume-before.volume))<=1e-12*std::abs(double(before.volume)));
   size_t bottom=0;for(const auto& f:after.skin)if(f.second==7)++bottom;CHECK(bottom==2);
   CHECK(after.skin.count(Face{3,4,1})+after.skin.count(Face{1,3,4})==1);}
  // two different facets: the boundary is never flipped across them
  {Owned mesh=build(false);OptimizeReport report;Id nn=100,nc=1000;OptimizeMetricMesh(mesh,target,nn,nc,options,report);CHECK(report.boundary_flips==0);}
}
TEST_CASE("Native 3D optimizer: length-driven edge removal of an interior edge longer than 1.8", "[NativeOptimize3D]") {
  FrozenField field({CoveringDonor({1,0,0,1,0,1})});const RegionMetric target(field);
  // four good cells (score 3.8) around an axis of length 2.0: the 4-4 swap to a ring diagonal of length 1.6 (score 4.4)
  const double pi=std::acos(-1.);std::vector<Node> r;for(int i=0;i<4;++i)r.push_back({Id(10+i),{.8*std::cos(pi*i/2),.8*std::sin(pi*i/2),0}});
  const Node a{1,{0,0,1}},b{2,{0,0,-1}};std::vector<std::array<Node,4>> cells;for(int i=0;i<4;++i)cells.push_back(Tet(a,b,r[i],r[(i+1)%4]));
  Owned mesh=MeshOf(cells);const auto before=Facts(mesh,field);REQUIRE(before.bad==0);
  OptimizeOptions options;OptimizeReport report;Id nn=100,nc=1000;
  CHECK(OptimizeMetricMesh(mesh,target,nn,nc,options,report));const auto after=Facts(mesh,field);
  CHECK(report.length_removals==1);CHECK(after.cells==4);CHECK(after.positive);CHECK(after.bad==0);CHECK(after.skin==before.skin);
  bool axis=false;for(const auto& row:mesh){bool ha=false,hb=false;for(const auto& v:row.second.cell.v){ha=ha||v.id==1;hb=hb||v.id==2;}axis=axis||(ha&&hb);}CHECK_FALSE(axis);
  // a frozen cell in the shell keeps the long edge
  Owned frozenMesh=MeshOf(cells);OptimizeOptions frozen;frozen.frozen_cells={frozenMesh.begin()->first};OptimizeReport r2;nn=100;nc=1000;
  OptimizeMetricMesh(frozenMesh,target,nn,nc,frozen,r2);CHECK(r2.length_removals==0);
}
TEST_CASE("Native 3D optimizer: sliver removal by vertex insertion (O7/O8) and length-driven insertion (O7L)", "[NativeOptimize3D]") {
  FrozenField field({CoveringDonor({1,0,0,1,0,1})});const RegionMetric target(field);
  // classic sliver (a,b,c,d) (crossing edges ab/cd, height .02) inside an octahedron: its upper faces coned to p, its
  // lower faces to q; swaps, collapse and smoothing disabled: only a vertex insertion can remove it
  const Node a{1,{1,0,0}},b{2,{-1,0,0}},cc{3,{0,1,.02}},d{4,{0,-1,.02}},p{5,{0,0,1}},q{6,{0,0,-1}};
  auto sliverMesh=[&]{return std::vector<std::array<Node,4>>{Tet(a,b,cc,d),Tet(p,a,cc,d),Tet(p,b,cc,d),Tet(q,a,b,cc),Tet(q,a,b,d)};};
  // Pinned skin points do not consume the relaxation share or suppress necessary insertion.
  {Owned mesh=MeshOf(sliverMesh());OptimizeOptions staged;staged.swaps=staged.collapses=false;staged.maximum_work=160;
   OptimizeReport report;Id nn=100,nc=1000;
   REQUIRE(OptimizeMetricMesh(mesh,target,nn,nc,staged,report));CHECK(report.moves==0);
   CHECK(report.shell_insertions+report.neighbourhood_insertions+report.length_insertions>0);CHECK(report.trials<=staged.maximum_work);
   for(size_t budget:{size_t(0),size_t(1)}) {
     Owned limited=MeshOf(sliverMesh());staged.maximum_work=budget;OptimizeReport bounded;nn=100;nc=1000;
     OptimizeMetricMesh(limited,target,nn,nc,staged,bounded);CHECK(bounded.trials==budget);CHECK(bounded.stop=="limit");
     CHECK(bounded.moves==0);CHECK(bounded.placements==budget);
   }}
  {Owned mesh=MeshOf(sliverMesh());const auto before=Facts(mesh,field);REQUIRE(before.positive);REQUIRE(before.bad>0);
   OptimizeOptions ins;ins.swaps=false;ins.collapses=false;ins.smoothing=false;OptimizeReport report;Id nn=100,nc=1000;
   CHECK(OptimizeMetricMesh(mesh,target,nn,nc,ins,report));const auto after=Facts(mesh,field);
   CHECK(report.shell_insertions+report.neighbourhood_insertions>=1);CHECK(report.placements>=1);CHECK(report.placement_evaluations>0);
   CHECK(after.min>before.min);CHECK(after.skin==before.skin);CHECK(after.positive);
   CHECK(std::abs(double(after.volume-before.volume))<=1e-12*std::abs(double(before.volume)));
   std::set<Id> ids;for(const auto& row:mesh)for(const auto& v:row.second.cell.v)ids.insert(v.id);for(Id i=1;i<=6;++i)CHECK(ids.count(i)==1);
   // frozen cells everywhere: nothing is inserted
   Owned again=MeshOf(sliverMesh());OptimizeOptions all=ins;for(const auto& row:again)all.frozen_cells.insert(row.first);OptimizeReport r2;nn=100;nc=1000;
   CHECK_FALSE(OptimizeMetricMesh(again,target,nn,nc,all,r2));CHECK(r2.shell_insertions+r2.neighbourhood_insertions==0);}
  // O7L: four cells around an axis of length 2.4 whose 4-4 swap would create a ring diagonal of 2.4 > 1.8: the shell
  // is re-coned from a vertex near the axis midpoint (both halves <= .6 x 2.4), the long edge leaves the mesh
  {const double pi=std::acos(-1.);std::vector<Node> r;for(int i=0;i<4;++i)r.push_back({Id(10+i),{1.2*std::cos(pi*i/2),1.2*std::sin(pi*i/2),0}});
   const Node a{1,{0,0,1.2}},b{2,{0,0,-1.2}};std::vector<std::array<Node,4>> cells;for(int i=0;i<4;++i)cells.push_back(Tet(a,b,r[i],r[(i+1)%4]));
   Owned mesh=MeshOf(cells);const auto before=Facts(mesh,field);
   OptimizeOptions options;options.smoothing=false;options.collapses=false;OptimizeReport report;Id nn=100,nc=1000;
   CHECK(OptimizeMetricMesh(mesh,target,nn,nc,options,report));const auto after=Facts(mesh,field);
   CHECK(report.length_removals==0);CHECK(report.length_insertions==1);CHECK(after.cells==8);CHECK(after.positive);CHECK(after.skin==before.skin);
   bool axis=false;long double longest=0;for(const auto& row:mesh){const auto& v=row.second.cell.v;bool ha=false,hb=false;for(const auto& x:v){ha=ha||x.id==1;hb=hb||x.id==2;}axis=axis||(ha&&hb);
     for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j)longest=std::max(longest,field.TargetEdgeLength(v[i].p,v[j].p));}
   CHECK_FALSE(axis);CHECK(longest<=MaximumMetricEdge);
   CHECK(std::abs(double(after.volume-before.volume))<=1e-12*std::abs(double(before.volume)));}
  // the public placement helper: the shell of the long axis re-coned with the size-split restriction
  {const double pi=std::acos(-1.);std::vector<Node> r;for(int i=0;i<3;++i)r.push_back({Id(10+i),{1.2*std::cos(2*pi*i/3),1.2*std::sin(2*pi*i/3),0}});
   const Node a{1,{0,0,1.2}},b{2,{0,0,-1.2}};std::vector<Cell> cells;
   for(int i=0;i<3;++i){Cell x{Id(i),Tet(a,b,r[i],r[(i+1)%3])};if(Orientation(x.v[0].p,x.v[1].p,x.v[2].p,x.v[3].p)<0)std::swap(x.v[0],x.v[1]);cells.push_back(x);}
   std::vector<Cell> fresh;double score=0;const Edge e{1,2};
   REQUIRE(PlaceConeInsertion(cells,{0,0,0},50,500,target,fresh,score,&e,.6));CHECK(fresh.size()==6);CHECK(score>0);
   for(const auto& x:fresh){CHECK(Orientation(x.v[0].p,x.v[1].p,x.v[2].p,x.v[3].p)>0);CHECK(x.v[0].id==50);}}
}

TEST_CASE("Captured curved collapse reduces size debt during construction", "[NativeShapeAdmission3D][NativeMetricSplitRule3D]") {
  struct Restore {ShapeAdmission value=GetShapeAdmission();~Restore(){SetShapeAdmission(value);}} restore;
  auto metric=[](const std::vector<std::pair<Edge,long double>>& edges) {
    PatchMetric m;m.shape_measured=true;m.minimum_mean_ratio=.5;m.minimum_scaled_jacobian=.5;
    for(const auto& row:edges){EdgeMeasure e;e.a={double(row.first[0]),0,0};e.b={double(row.first[1]),1,0};e.length=row.second;m.edges.emplace(row.first,e);}return m;
  };
  const auto before=metric({{{5192,115352},2.055327113L},{{27224,115352},2.049412153L},
      {{7,115352},2.049412153L},{{7,5192},4.094093306L}});
  auto after=metric({{{5192,104984},1.972715426L},{{51848,104984},1.626658203L},{{7,5192},4.094093306L}});
  std::string reason;MetricDiagnosis diagnosis;
  SetShapeAdmission(ShapeAdmission::Absolute);
  CHECK_FALSE(ValidateMetricChange(10,before,6,after,MetricChange::Coarsen,reason,&diagnosis));
  SetShapeAdmission(ShapeAdmission::ValidityOnly);auto unmeasured=before;
  unmeasured.shape_measured=after.shape_measured=false;
  CHECK(ValidateMetricChange(10,unmeasured,6,after,MetricChange::Coarsen,reason,&diagnosis));
  CHECK(diagnosis.cell_removal);CHECK(diagnosis.size_progress);CHECK_FALSE(diagnosis.shape_measured);
  CHECK(diagnosis.added_excess>0);CHECK(diagnosis.added_excess<diagnosis.removed_excess);
  CHECK_FALSE(ValidateMetricChange(6,after,10,unmeasured,MetricChange::Coarsen,reason));
  CHECK_FALSE(ValidateMetricChange(10,after,6,unmeasured,MetricChange::Coarsen,reason));
}

TEST_CASE("Quality repair cannot exchange unacceptable shape for size progress", "[NativeShapeAdmission3D][NativeMetricSplitRule3D]") {
  struct Restore {ShapeAdmission value=GetShapeAdmission();~Restore(){SetShapeAdmission(value);}} restore;
  auto metric=[](Edge key,long double length,double q,double j){
    PatchMetric m;m.shape_measured=true;m.minimum_mean_ratio=q;m.minimum_scaled_jacobian=j;
    EdgeMeasure edge;edge.a={0,0,0};edge.b={1,0,0};edge.length=length;m.edges.emplace(key,edge);return m;
  };
  const auto before=metric({1,2},3,.5,.5),poor=metric({1,3},2.5,.1,.02),good=metric({1,3},2.5,.4,.4);
  std::string reason;MetricDiagnosis diagnosis;
  SetShapeAdmission(ShapeAdmission::Absolute);
  CHECK_FALSE(ValidateMetricChange(4,before,4,poor,MetricChange::Repair,reason,&diagnosis));
  CHECK(diagnosis.failure==MetricFailure::Shape);CHECK(diagnosis.shape_measured);
  CHECK(ValidateMetricChange(4,before,4,good,MetricChange::Repair,reason));
  const auto weak=metric({1,2},1,.19,.1),closed=metric({1,3},1,.21,.08);
  CHECK(ValidateMetricChange(4,weak,4,closed,MetricChange::Repair,reason,&diagnosis));CHECK(diagnosis.quality_progress);
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  CHECK(ValidateMetricChange(4,before,4,poor,MetricChange::Repair,reason)); // size progress remains an independent alternative
}

TEST_CASE("Validity-only repair requires complete decreasing measured q/J hinge debt", "[NativeShapeAdmission3D][NativeMetricSplitRule3D]") {
  struct Restore {ShapeAdmission value=GetShapeAdmission();~Restore(){SetShapeAdmission(value);}} restore;
  auto tet=[](Id first,double x,double h) {
    return Cell{first,{{{first,{x,0,0}},{first+1,{x+1,0,0}},{first+2,{x,1,0}},{first+3,{x,0,h}}}}};
  };
  DonorCell donor;donor.cell={0,{{{0,{-10,-10,-10}},{1,{40,-10,-10}},{2,{-10,40,-10}},{3,{-10,-10,40}}}}};
  donor.sensor.fill({1,0,0,1,0,1});FrozenField field({donor});
  const std::vector<Cell> old{tet(10,0,.02),tet(20,2,.05)};
  const std::vector<Cell> worse_total{tet(30,0,.021),tet(40,2,.04)};
  PatchMetric before,after;std::string reason;MetricDiagnosis diagnosis;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  REQUIRE_NOTHROW(before=MeasureMetricCells(old,field));
  REQUIRE_NOTHROW(after=MeasureMetricCells(worse_total,field,nullptr,&before.edges));
  const auto score=[](const PatchMetric& m){return std::min(m.minimum_mean_ratio/MinimumMeanRatio,m.minimum_scaled_jacobian/MinimumScaledJacobian);};
  REQUIRE(score(after)>score(before));REQUIRE(after.shape_debt>before.shape_debt);
  CHECK_FALSE(ValidateMetricChange(old.size(),before,worse_total.size(),after,MetricChange::Repair,reason,&diagnosis));
  CHECK(diagnosis.old_shape_cells==old.size());CHECK(diagnosis.new_shape_cells==worse_total.size());
  CHECK(diagnosis.new_shape_debt>diagnosis.old_shape_debt);

  const std::vector<Cell> one_old{tet(50,0,.02)},improved{tet(60,0,.03)};
  auto one_before=MeasureMetricCells(one_old,field);
  auto one_after=MeasureMetricCells(improved,field,nullptr,&one_before.edges);
  REQUIRE(one_after.shape_debt<one_before.shape_debt);
  CHECK(ValidateMetricChange(1,one_before,1,one_after,MetricChange::Repair,reason,&diagnosis));
  CHECK(diagnosis.quality_progress);

  const std::vector<Cell> regular{tet(70,0,1)},regular_changed{tet(80,0,1.1)};
  auto above=MeasureMetricCells(regular,field),still_above=MeasureMetricCells(regular_changed,field,nullptr,&above.edges);
  REQUIRE(above.shape_debt==0);REQUIRE(still_above.shape_debt==0);
  CHECK_FALSE(ValidateMetricChange(1,above,1,still_above,MetricChange::Repair,reason,&diagnosis));

  auto missing=one_before;missing.shape_cells=0;
  CHECK_FALSE(ValidateMetricChange(1,missing,1,one_after,MetricChange::Repair,reason,&diagnosis));
  auto incomplete=one_before;incomplete.shape_cells=2;
  CHECK_FALSE(ValidateMetricChange(1,incomplete,1,one_after,MetricChange::Repair,reason,&diagnosis));
  auto edge_only=MeasureMetricEdges(one_old,field);
  CHECK_FALSE(edge_only.shape_measured);CHECK(edge_only.shape_cells==0);CHECK(edge_only.shape_debt==0);
}
