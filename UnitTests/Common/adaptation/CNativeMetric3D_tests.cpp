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
  {  // The split edge is admissible (<= 1.8) but longer than SplitInsertionLength: the former rule (StrictExcess)
     // refuses the split for lack of excess progress; the 2D split rule (SplitInsertion, production default since
     // 2026-10-23) admits it as a size insertion without vertex removal.
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
  FrozenField target({Donor({})});
  PatchMetric metric; MetricDiagnosis diagnosis; std::string reason;
  const auto cell=Single();
  const auto before=MeasureMetricCells({cell},target,nullptr,nullptr,false,&diagnosis);
  CHECK(diagnosis.failure==MetricFailure::None);
  CHECK_FALSE(ValidateMetricChange(1,before,1,before,MetricChange::Repair,reason,&diagnosis));
  CHECK(diagnosis.failure==MetricFailure::Progress);
  CHECK_FALSE(diagnosis.size_progress); CHECK_FALSE(diagnosis.quality_progress);
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
  CHECK(target.Statistics().edge_requests==6);
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

TEST_CASE("Native3D metric movement honours directional stiffness without changing its target", "[NativeMetricMove3D]") {
  const Point origin{0,0,0};
  std::vector<std::pair<Point,Tensor>> neighbors{{{1,0,0},{}},{{0,1,0},{}},{{0,0,1},{}}};
  const auto euclidean=MetricMeanPoint(origin,neighbors);
  CHECK(euclidean.x==Approx(1./3));CHECK(euclidean.y==Approx(1./3));CHECK(euclidean.z==Approx(1./3));
  neighbors.back().second.zz=30.25;const auto graded=MetricMeanPoint(origin,neighbors);
  CHECK(graded.x==Approx(1./3));CHECK(graded.y==Approx(1./3));CHECK(graded.z==Approx(30.25/32.25));
  CHECK(neighbors.back().second.zz==30.25);CHECK_THROWS(MetricMeanPoint(origin,{}));
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

TEST_CASE("Native 3D refine admission follows the 2D split rule under SplitInsertion", "[NativeMetricSplitRule3D]") {
  struct Restore { RefineAcceptance a=GetRefineAcceptance(); ~Restore(){SetRefineAcceptance(a);} } restore;
  auto metric=[](std::vector<std::pair<Edge,long double>> rows,double q) {
    PatchMetric m;m.minimum_mean_ratio=q;m.minimum_scaled_jacobian=q;
    for(const auto& r:rows){EdgeMeasure e;e.a={double(r.first[0]),0,0};e.b={double(r.first[1]),1,0};e.length=r.second;m.edges.emplace(r.first,e);}
    return m;};
  // a long edge (1,2) split at 4; the new edge (3,4) adds more excess than the split edge removes
  const auto before=metric({{{1,2},3.0L},{{1,3},1.0L},{{2,3},1.0L}},.5);
  const auto split=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,4},1.5L},{{2,4},1.5L},{{3,4},3.5L}},.5);
  std::string reason;MetricDiagnosis d;
  SetRefineAcceptance(RefineAcceptance::StrictExcess);
  CHECK_FALSE(ValidateMetricChange(1,before,2,split,MetricChange::Refine,reason,&d));
  SetRefineAcceptance(RefineAcceptance::SplitInsertion);
  CHECK(ValidateMetricChange(1,before,2,split,MetricChange::Refine,reason,&d));
  CHECK(d.size_insertion);CHECK_FALSE(d.vertex_removal);CHECK(d.added_excess>d.removed_excess);
  // no cell-count condition for a size insertion (a refit may keep the tetrahedron count)
  CHECK(ValidateMetricChange(2,before,2,split,MetricChange::Refine,reason,&d));
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
  // other intents are unchanged by the policy
  CHECK_FALSE(ValidateMetricChange(1,before,2,split,MetricChange::Repair,reason,&d));
  CHECK_FALSE(ValidateMetricChange(1,before,2,split,MetricChange::GeometryRefine,reason,&d));
}

TEST_CASE("Native 3D split rule counts only caller-certified relocations as moves", "[NativeMetricSplitRule3D]") {
  struct Restore { RefineAcceptance a=GetRefineAcceptance(); ~Restore(){SetRefineAcceptance(a);} } restore;
  SetRefineAcceptance(RefineAcceptance::SplitInsertion);
  auto metric=[](std::vector<std::pair<Edge,long double>> rows) {
    PatchMetric m;m.minimum_mean_ratio=.5;m.minimum_scaled_jacobian=.5;
    for(const auto& r:rows){EdgeMeasure e;e.a={double(r.first[0]),0,0};e.b={double(r.first[1]),1,0};e.length=r.second;m.edges.emplace(r.first,e);}
    return m;};
  // long edge (1,2) split at 5; apex 4 relocated to the reserved identity 6; the split adds more excess than it removes
  const auto before=metric({{{1,2},3.0L},{{1,3},1.0L},{{2,3},1.0L},{{1,4},1.0L},{{2,4},1.0L},{{3,4},1.0L}});
  const auto fresh=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,5},1.5L},{{2,5},1.5L},{{3,5},3.5L},{{1,6},1.0L},{{2,6},1.0L},{{5,6},1.0L},{{3,6},1.0L}});
  std::string reason;MetricDiagnosis d;
  CHECK_FALSE(ValidateMetricChange(2,before,4,fresh,MetricChange::Refine,reason,&d));CHECK(d.vertex_removal);
  CHECK(ValidateMetricChange(2,before,4,fresh,MetricChange::Refine,reason,&d,1));CHECK(d.size_insertion);
  // a certified relocation cannot hide an absorption without net vertex gain
  const auto absorbed=metric({{{1,3},1.0L},{{2,3},1.0L},{{1,6},1.0L},{{2,6},1.0L},{{3,6},3.5L}});
  CHECK_FALSE(ValidateMetricChange(2,before,2,absorbed,MetricChange::Refine,reason,&d,1));CHECK(d.vertex_removal);
}
