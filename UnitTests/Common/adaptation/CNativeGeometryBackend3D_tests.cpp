/*!
 * \file CNativeGeometryBackend3D_tests.cpp
 * \brief Existing cubic authority exercised through immutable geometry queries.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include <cmath>
#include <limits>
using namespace SU2NativeGeometry;
namespace {
AtlasBudget BackendBudget(){return {size_t(1)<<22,size_t(1)<<28,4096};}
struct BackendFixture {
  std::vector<SU2Native3D::Facet> facets;
  std::shared_ptr<const SU2Native3D::PlanarReference> reference;
  std::shared_ptr<MappedFacetReference> geometry;
  MobilityPolicy policy;
  explicit BackendFixture(bool second_body=false,bool cubic=true) {
    using namespace SU2Native3D;
    const std::array<Face,12> faces{{{0,2,3},{0,3,1},{4,5,7},{4,7,6},{0,4,6},{0,6,2},{1,3,7},{1,7,5},{0,1,5},{0,5,4},{2,6,7},{2,7,3}}};
    for(unsigned body=0;body<(second_body?2u:1u);++body){std::array<Node,8> nodes;
      for(unsigned i=0;i<8;++i)nodes[i]={100*body+i,{double(i&1)+10*body,double((i>>1)&1),double((i>>2)&1)}};
      for(unsigned i=0;i<faces.size();++i){const auto& f=faces[i];facets.push_back({100*body+i,0,{nodes[f[0]],nodes[f[1]],nodes[f[2]]}});}}
    reference=std::make_shared<PlanarReference>(facets);MappedFacetReference::CurvedNets nets;std::map<Id,double> errors;
    const unsigned power[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
    for(const auto& f:facets){errors.emplace(f.id,1e-12);if(!cubic)continue;auto& net=nets[f.id];
      for(unsigned q=0;q<10;++q){long double x=0,y=0,z=0;for(unsigned k=0;k<3;++k){x+=power[q][k]*(long double)f.v[k].p.x;y+=power[q][k]*(long double)f.v[k].p.y;z+=power[q][k]*(long double)f.v[k].p.z;}net[q]={double(x/3),double(y/3),double(z/3)};}
      if(f.id%100<2)net[9].z-=.03;
    }
    geometry=std::make_shared<MappedFacetReference>(reference,errors,57,SIZE_MAX,false,nets);
    auto budget=BackendBudget();std::string reason;REQUIRE(geometry->BuildMobilityPolicy(60,{{0,2}},policy,budget,reason));
  }
};
}
TEST_CASE("Native backend shares sided cubic differentials and separates physical support", "[NativeGeometryBackend3D]") {
  BackendFixture fixture;NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);auto budget=BackendBudget();std::string reason;
  const auto binding=fixture.geometry->FacetPoint(0,.25,.25);GeometryEvaluation position,sided;
  REQUIRE(backend.Evaluate(binding,std::nullopt,position,budget,reason));CHECK_FALSE(position.differential_available);
  REQUIRE(backend.AssociationValid(binding,position.point,reason));REQUIRE(backend.Evaluate(binding,0,sided,budget,reason));
  CHECK(sided.differential_available);CHECK(sided.normal.z<0);CHECK(std::abs(sided.curvature1)+std::abs(sided.curvature2)>0);
  const auto closest=fixture.geometry->ClosestCurved(0,position.point,1e-8,16384);
  CHECK(sided.normal.x==closest.normal.x);CHECK(sided.normal.y==closest.normal.y);CHECK(sided.normal.z==closest.normal.z);
  CHECK(sided.curvature1==closest.curvature1);CHECK(sided.curvature2==closest.curvature2);
  const auto seam=fixture.geometry->EdgePoint({0,3},.5);GeometrySupport support;
  REQUIRE(backend.Support(seam,0,support,budget,reason));CHECK(seam.dimension==1);CHECK(support.mobility==2);CHECK(support.body==0);CHECK(support.sheet==0);
  const auto crease=fixture.geometry->EdgePoint({0,2},.5);GeometryEvaluation bottom,left;
  REQUIRE(backend.Evaluate(crease,0,bottom,budget,reason));REQUIRE(backend.Evaluate(crease,5,left,budget,reason));
  CHECK(bottom.tangent_available);CHECK(left.tangent_available);CHECK(bottom.tangent.y>0);CHECK(left.tangent.y>0);
  CHECK(bottom.normal.x*left.normal.x+bottom.normal.y*left.normal.y+bottom.normal.z*left.normal.z<.5);
  auto saved=sided;CHECK_FALSE(backend.Evaluate(binding,2,sided,budget,reason));CHECK(sided.point.z==saved.point.z);
  auto stale=binding;++stale.version;CHECK_FALSE(backend.AssociationValid(stale,position.point,reason));
  auto limited=BackendBudget();limited.maximum_work=0;CHECK_FALSE(backend.Evaluate(binding,std::nullopt,sided,limited,reason));
}
TEST_CASE("Native projection requires a body and refuses sheet and feature escapes", "[NativeGeometryBackend3D]") {
  BackendFixture fixture(true);NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);auto budget=BackendBudget();std::string reason;
  GeometryProjectionScope scope;scope.body=0;scope.hint=0;scope.previous=fixture.geometry->FacetPoint(0,.25,.25);scope.sheet=fixture.policy.sheets.at(0);scope.absolute_error=1e-6;
  GeometryEvaluation evaluation;REQUIRE(backend.Evaluate(scope.previous,0,evaluation,budget,reason));
  Point query{evaluation.point.x+.001*evaluation.normal.x,evaluation.point.y+.001*evaluation.normal.y,evaluation.point.z+.001*evaluation.normal.z};
  GeometryProjection result;INFO(reason);REQUIRE(backend.Project(query,scope,result,budget,reason));
  auto invalid_query=BackendBudget();const auto saved=result;
  CHECK_FALSE(backend.Project({std::numeric_limits<double>::quiet_NaN(),0,0},scope,result,invalid_query,reason));
  CHECK_FALSE(invalid_query.work_exhausted);CHECK(invalid_query.work<invalid_query.maximum_work);
  CHECK(result.binding.feature==saved.binding.feature);CHECK(result.binding.u==saved.binding.u);CHECK(result.binding.v==saved.binding.v);
  GeometrySupport support;REQUIRE(backend.Support(result.binding,result.surface,support,budget,reason));CHECK(support.body==0);CHECK(support.sheet==*scope.sheet);
  CHECK(result.distance_lower<=.001);CHECK(result.distance_upper>=.001-1e-10);
  auto other_body=scope;other_body.body=100;CHECK_FALSE(backend.Project(query,other_body,result,budget,reason));
  CHECK_FALSE(backend.Project({-.001,.5,.5},scope,result,budget,reason));CHECK(reason.find("sheet")!=std::string::npos);
  auto feature=scope;feature.previous=fixture.geometry->EdgePoint({0,2},.5);REQUIRE(backend.Support(feature.previous,0,support,budget,reason));feature.feature=std::make_pair(support.mobility,support.feature);
  CHECK_FALSE(backend.Project(query,feature,result,budget,reason));CHECK(reason.find("feature")!=std::string::npos);
  auto far_budget=BackendBudget();REQUIRE(backend.Project({10.25,.5,-.001},scope,result,far_budget,reason));
  REQUIRE(backend.Support(result.binding,result.surface,support,far_budget,reason));CHECK(support.body==0);CHECK(result.evaluation.point.x<=1+1e-12);
  BackendFixture faceted(false,false);NativeMappedGeometryBackend3D flat(faceted.geometry,faceted.policy);auto flat_budget=BackendBudget();
  GeometryEvaluation plane;REQUIRE(flat.Evaluate(scope.previous,0,plane,flat_budget,reason));CHECK(plane.curvature1==0);CHECK(plane.curvature2==0);
  CHECK_FALSE(flat.Project(query,scope,result,flat_budget,reason));CHECK(reason.find("unavailable")!=std::string::npos);
}
TEST_CASE("Native backend region certificates and full authority survive existing persistence", "[NativeGeometryBackend3D]") {
  using namespace SU2Native3D;BackendFixture fixture;NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);auto budget=BackendBudget();std::string reason;
  const auto& f=fixture.facets[0];std::array<SurfaceBinding,3> bindings;std::array<Point,3> flat;
  for(unsigned k=0;k<3;++k){bindings[k]=fixture.geometry->Vertex(f.v[k].id);flat[k]=f.v[k].p;}
  FaceCoverage coverage;REQUIRE(fixture.geometry->ResolveSingleChart(0,bindings,coverage,budget,reason));GeometryRegionBounds bounds;
  REQUIRE(backend.RegionErrorBounds(coverage,flat,bounds,budget,reason));CHECK(bounds.maximum_error>1e-3);CHECK(bounds.bounds[0].z<0);
  GeometryEvaluation evaluation;const auto binding=fixture.geometry->FacetPoint(0,.25,.25);REQUIRE(backend.Evaluate(binding,std::nullopt,evaluation,budget,reason));CHECK(bounds.bounds[0].z<=evaluation.point.z);
  BoundaryPoints points;std::vector<SurfaceFace> surface;
  for(const auto& facet:fixture.facets){surface.push_back({{facet.v[0].id,facet.v[1].id,facet.v[2].id},facet.id});for(auto n:facet.v)points.emplace(n.id,BoundaryPoint{n,fixture.geometry->Vertex(n.id)});}
  auto parameters=std::make_shared<BoundaryParameterState>(fixture.geometry,points,surface);
  ReferenceState state{PlanarReference::WithParameters(fixture.reference,parameters),fixture.facets,{"wall"},0};
  const auto encoded=EncodeReference(state);CHECK(encoded.find("SU2_NATIVE_REFERENCE_3D 7\n")==0);const auto restored=DecodeReference(encoded);CHECK(EncodeReference(restored)==encoded);
  NativeMappedGeometryBackend3D reloaded(restored.original->Parameters()->Authority(),restored.original->Parameters()->Mobility());
  CHECK(reloaded.Identity().version==backend.Identity().version);CHECK(reloaded.Identity().association_schema==1);CHECK(reloaded.Identity().declaration!=backend.Identity().declaration);
  GeometryEvaluation again;REQUIRE(reloaded.Evaluate(binding,std::nullopt,again,budget,reason));CHECK(again.point.x==evaluation.point.x);CHECK(again.point.y==evaluation.point.y);CHECK(again.point.z==evaluation.point.z);
  auto changed=fixture.geometry->Curves();changed[0][9].z-=.01;
  auto different=std::make_shared<MappedFacetReference>(fixture.reference,fixture.geometry->Errors(),57,SIZE_MAX,false,changed);
  MobilityPolicy changed_policy;REQUIRE(different->BuildMobilityPolicy(60,{{0,2}},changed_policy,budget,reason));
  NativeMappedGeometryBackend3D changed_backend(different,changed_policy);
  CHECK(changed_backend.Identity().version==backend.Identity().version);CHECK(changed_backend.Identity().declaration!=backend.Identity().declaration);
  CHECK_FALSE(changed_backend.AssociationValid(binding,evaluation.point,reason));
  const auto saved=bounds.maximum_error;CHECK_FALSE(backend.RegionErrorBounds({},flat,bounds,budget,reason));CHECK(bounds.maximum_error==saved);
}

TEST_CASE("Native contraction policy follows physical sheets across original seams", "[NativeGeometryBackend3D][NativeGeometryContraction3D]") {
  BackendFixture fixture(true);NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);const auto& g=*fixture.geometry;
  const auto surface=g.FacetPoint(0,.25,.25),seam=g.EdgePoint({0,3},.5);
  REQUIRE(seam.dimension==1);REQUIRE(g.MobilityDimension(seam,fixture.policy)==2);
  REQUIRE(fixture.policy.sheets.at(0)==fixture.policy.sheets.at(1));
  REQUIRE(fixture.policy.sheets.at(0)!=fixture.policy.sheets.at(2));
  auto check=[&](const SurfaceBinding& removed,const SurfaceBinding& retained,bool expected) {
    auto budget=BackendBudget();std::string reason="previous failure";bool allowed=!expected;
    const bool queried=backend.ContractionAllowed(removed,retained,allowed,budget,reason);INFO(reason);REQUIRE(queried);
    CHECK(allowed==expected);CHECK(reason.empty());
  };
  check(seam,g.FacetPoint(1,.25,.25),true);
  check(surface,g.FacetPoint(1,.25,.25),true);
  // These supports are incident to the same physical sheet across the original diagonal.
  check(surface,g.EdgePoint({1,3},.5),true);
  check(surface,g.Vertex(1),true);
  check(seam,g.EdgePoint({0,2},.5),true);
  check(surface,g.FacetPoint(2,.25,.25),false);
  check(surface,g.Vertex(7),false);
  check(surface,g.FacetPoint(100,.25,.25),false);
  check(seam,g.Vertex(100),false);
  check(g.Vertex(0),surface,false);
  check(g.Vertex(0),g.Vertex(2),false);
}

TEST_CASE("Native contraction policy preserves genuine curve chains and incident corners", "[NativeGeometryBackend3D][NativeGeometryContraction3D]") {
  BackendFixture fixture(true);NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);const auto& g=*fixture.geometry;
  const auto curve=g.EdgePoint({0,2},.5);
  REQUIRE(g.MobilityDimension(curve,fixture.policy)==1);
  REQUIRE(g.MobilityDimension(g.Vertex(0),fixture.policy)==0);
  REQUIRE(g.MobilityDimension(g.Vertex(2),fixture.policy)==0);
  REQUIRE(fixture.policy.features.at(SU2Native3D::Edge{0,2})!=fixture.policy.features.at(SU2Native3D::Edge{2,3}));
  auto check=[&](const SurfaceBinding& retained,bool expected) {
    auto budget=BackendBudget();std::string reason="previous failure";bool allowed=!expected;
    const bool queried=backend.ContractionAllowed(curve,retained,allowed,budget,reason);INFO(reason);REQUIRE(queried);
    CHECK(allowed==expected);CHECK(reason.empty());
  };
  check(g.EdgePoint({0,2},.75),true);
  check(g.Vertex(0),true);check(g.Vertex(2),true);
  check(g.Vertex(7),false);
  check(g.EdgePoint({2,3},.5),false);
  check(g.FacetPoint(0,.25,.25),false);
  check(g.EdgePoint({0,3},.5),false);
  check(g.EdgePoint({100,102},.5),false);
  check(g.Vertex(100),false);
}

TEST_CASE("Native contraction query distinguishes refusal from a topology veto", "[NativeGeometryBackend3D][NativeGeometryContraction3D]") {
  BackendFixture fixture;NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);const auto& g=*fixture.geometry;
  const auto removed=g.EdgePoint({0,3},.5),retained=g.FacetPoint(1,.25,.25);
  for(bool initial:{false,true}) {
    auto exhausted=BackendBudget();exhausted.maximum_work=0;std::string reason;bool allowed=initial;
    CHECK_FALSE(backend.ContractionAllowed(removed,retained,allowed,exhausted,reason));
    CHECK(allowed==initial);CHECK_FALSE(reason.empty());
    auto budget=BackendBudget();auto stale=removed;++stale.version;reason.clear();
    CHECK_FALSE(backend.ContractionAllowed(stale,retained,allowed,budget,reason));
    CHECK(allowed==initial);CHECK_FALSE(reason.empty());
  }
}

TEST_CASE("Certified point separation conservatively screens curved boundary samples", "[NativeGeometryBackend3D]") {
  using namespace SU2Native3D;
  BackendFixture fixture;NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);
  auto budget=BackendBudget();std::string reason;GeometryEvaluation sample;
  REQUIRE(backend.Evaluate(fixture.geometry->FacetPoint(0,.25,.25),std::nullopt,sample,budget,reason));
  const auto& f=fixture.facets[0];const std::array<Point,3> flat{{f.v[0].p,f.v[1].p,f.v[2].p}};
  const std::array<Point,3> point{{sample.point,sample.point,sample.point}};
  const double distance=std::abs(sample.point.z);REQUIRE(distance>0);
  const double low=std::nextafter(distance*.25+sample.realization_error,INFINITY);
  const double high=std::nextafter(distance*2+sample.realization_error,INFINITY);
  CHECK(SourceFacesSeparated(point,flat,low));CHECK_FALSE(SourceFacesSeparated(point,flat,high));
  CHECK_FALSE(SourceFacesSeparated(point,point,low));
  const double margin=std::nextafter(distance+sample.realization_error,INFINITY);
  CHECK_FALSE(SourceFacesSeparated(point,flat,margin));
  auto near=flat;for(auto& p:near)p.z=sample.point.z;
  CHECK_FALSE(SourceFacesSeparated(point,near,low));
  const bool all_far=SourceFacesSeparated(point,flat,low) && SourceFacesSeparated(point,near,low);
  CHECK_FALSE(all_far); // A single nearby face makes the union test inconclusive.
}

TEST_CASE("Backend bounds both directions of a complete unpartitioned surface union", "[NativeGeometryBackend3D]") {
  using namespace SU2Native3D;
  BackendFixture flat(true,false),curved(false,true);std::string reason;
  auto triangles=[](const BackendFixture& fixture,Id body){std::vector<std::array<Point,3>> result;
    for(const auto& facet:fixture.facets)if((facet.id>=100)==(body==100))
      result.push_back({facet.v[0].p,facet.v[1].p,facet.v[2].p});return result;};
  auto square=triangles(flat,0);REQUIRE(square.size()==12);
  // Replace the bottom square's diagonal without changing its oriented union.
  const auto a=flat.facets[0].v[0].p,b=flat.facets[0].v[1].p,c=flat.facets[0].v[2].p,d=flat.facets[1].v[2].p;
  square[0]={a,b,d};square[1]={b,c,d};
  GeometrySupport scope;scope.body=flat.geometry->Body(0);scope.marker=0;scope.sheet=UINT64_MAX;
  NativeMappedGeometryBackend3D backend(flat.geometry,flat.policy);auto budget=BackendBudget();
  MappedFacetReference::SurfaceUnionBounds bounds;
  bool ok=backend.PatchErrorBounds(scope,square,1e-4,bounds,budget,reason);INFO(reason);REQUIRE(ok);
  CHECK(bounds.within_tolerance);CHECK_FALSE(bounds.outside_tolerance);
  CHECK(bounds.model_to_mesh<=1e-4);CHECK(bounds.mesh_to_model<=1e-4);
  CHECK(bounds.model_pieces>0);CHECK(bounds.physical_pieces>0);
  auto missing=square;missing.erase(missing.begin());auto hole_budget=BackendBudget();
  ok=backend.PatchErrorBounds(scope,missing,1e-4,bounds,hole_budget,reason);INFO(reason);REQUIRE(ok);CHECK(bounds.outside_tolerance);CHECK_FALSE(bounds.within_tolerance);CHECK(bounds.model_to_mesh>1e-3);
  auto wrong=triangles(flat,100);auto body_budget=BackendBudget();
  ok=backend.PatchErrorBounds(scope,wrong,1e-4,bounds,body_budget,reason);INFO(reason);REQUIRE(ok);
  CHECK(bounds.outside_tolerance);CHECK_FALSE(bounds.within_tolerance);
  CHECK(bounds.model_to_mesh>1);CHECK(bounds.mesh_to_model>1);
  NativeMappedGeometryBackend3D cubic(curved.geometry,curved.policy);auto bump=triangles(curved,0);auto curved_budget=BackendBudget();
  scope.body=curved.geometry->Body(0);ok=cubic.PatchErrorBounds(scope,bump,1e-4,bounds,curved_budget,reason);INFO(reason);REQUIRE(ok);
  CHECK(bounds.outside_tolerance);CHECK_FALSE(bounds.within_tolerance);
  CHECK(bounds.model_to_mesh>.006);CHECK(bounds.mesh_to_model>0);
  MappedFacetReference::SurfaceUnionBounds sentinel;sentinel.model_to_mesh=17;sentinel.mesh_to_model=19;sentinel.model_pieces=23;sentinel.physical_pieces=29;
  auto unchanged=[&](){CHECK(sentinel.model_to_mesh==17);CHECK(sentinel.mesh_to_model==19);CHECK(sentinel.model_pieces==23);CHECK(sentinel.physical_pieces==29);};
  auto no_work=BackendBudget();no_work.maximum_work=0;
  CHECK_FALSE(cubic.PatchErrorBounds(scope,bump,1e-4,sentinel,no_work,reason));CHECK(no_work.work_exhausted);unchanged();
  auto no_storage=BackendBudget();no_storage.maximum_bytes=0;
  CHECK_FALSE(cubic.PatchErrorBounds(scope,bump,1e-4,sentinel,no_storage,reason));unchanged();
}

TEST_CASE("Physical source tubes preserve conservative shell checks without face correspondence", "[NativeGeometryBackend3D][NativeJointAffineSource3D]") {
  using namespace SU2Native3D;
  const std::array<Point,3> triangle{{{0,0,0},{1,0,0},{0,1,0}}};
  const auto tube=MappedFacetReference::SourcePatch::Tube(triangle,.01);
  const auto box=tube.Bounds();CHECK(box[0].z<=-.01);CHECK(box[1].z>=.01);
  CHECK(tube.OutwardDisplacement()>=.01);uint64_t work=0;
  auto outer=triangle;for(auto& p:outer)p.z=.02;
  CHECK(tube.Separated(outer,work,10000));
  for(auto& p:outer)p.z=.005;
  CHECK_FALSE(tube.Separated(outer,work,10000));
  CHECK_THROWS(MappedFacetReference::SourcePatch::Tube(triangle,-.01));
  CHECK_THROWS(MappedFacetReference::SourcePatch::Tube({triangle[0],triangle[0],triangle[2]},.01));
}

TEST_CASE("Entity geometry preserves feature contracts through reference and payload roundtrips", "[NativeGeometryBackend3D]") {
  using namespace SU2Native3D;BackendFixture fixture;auto budget=BackendBudget();std::string reason;
  BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:fixture.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,fixture.geometry->Vertex(n.id)});}
  auto legacy=std::make_shared<BoundaryParameterState>(fixture.geometry,points,skin,1,SIZE_MAX,0,BoundaryCoverage{},fixture.policy,&budget);
  // Fixture tolerance admits this .03-control bump; it does not qualify a .001 workflow.
  auto entity=legacy->WithEntityGeometry(.01,budget);REQUIRE(entity->EntityGeometry());CHECK(entity->GeometryTolerance()==.01);
  CHECK(entity->Coverage().faces.empty());CHECK(entity->Coverage().traces.size()==fixture.policy.features.size());
  for(const auto& row:entity->GeometryCertificates(budget)){CHECK(row.second.model_to_mesh<=.01);CHECK(row.second.mesh_to_model<=.01);}
  ReferenceState state{PlanarReference::WithParameters(fixture.reference,entity),fixture.facets,{"wall"},0};
  const auto encoded=EncodeReference(state);REQUIRE(encoded.find("SU2_NATIVE_REFERENCE_3D 9\n")==0);
  const auto decoded=DecodeReference(encoded);REQUIRE(decoded.original->Parameters());
  CHECK(decoded.original->Parameters()->Snapshot()==entity->Snapshot());CHECK(decoded.original->Parameters()->GeometryTolerance()==.01);
  const auto payload=entity->Payload();auto restored=BoundaryParameterState::FromPayload(fixture.geometry,payload);
  CHECK(restored->Snapshot()==entity->Snapshot());
  std::map<Id,Id> numbering;for(const auto& row:entity->Records())numbering.emplace(row.first,1000+entity->Records().size()-1-row.first);
  auto rebound=entity->Rebind(numbering);CHECK(rebound->EntityGeometry());CHECK(rebound->GeometryTolerance()==.01);
  for(const auto& row:entity->Records()){const auto* after=rebound->Find(numbering.at(row.first));REQUIRE(after);
    CHECK(after->node.p.x==row.second.point.node.p.x);CHECK(after->node.p.y==row.second.point.node.p.y);CHECK(after->node.p.z==row.second.point.node.p.z);}
  CHECK(rebound->Coverage().traces.size()==entity->Coverage().traces.size());
  for(const auto& row:entity->Surface()){std::array<Point,3> original{},renumbered{};
    for(unsigned k=0;k<3;++k){original[k]=entity->Find(row.second.v[k])->node.p;renumbered[k]=rebound->Find(numbering.at(row.second.v[k]))->node.p;}
    const auto a=MappedFacetReference::SourcePatch::Tube(original,.01).Bounds(),b=MappedFacetReference::SourcePatch::Tube(renumbered,.01).Bounds();
    for(unsigned k=0;k<2;++k){CHECK(a[k].x==b[k].x);CHECK(a[k].y==b[k].y);CHECK(a[k].z==b[k].z);}}
  for(const auto& row:rebound->GeometryCertificates(budget)){CHECK(row.second.model_to_mesh<=.01);CHECK(row.second.mesh_to_model<=.01);}
  auto missing=entity->Coverage();REQUIRE_FALSE(missing.traces.empty());missing.traces.erase(missing.traces.begin());
  CHECK_THROWS(BoundaryParameterState(fixture.geometry,points,skin,1,SIZE_MAX,0,missing,fixture.policy,&budget,.01));
  auto noncanonical=entity->Coverage();auto& trace=noncanonical.traces.begin()->second;REQUIRE(trace.pieces.size()==1);
  auto first=trace.pieces.front(),second=first;ExactRatio two,half;REQUIRE(ExactInteger(2,two,budget,reason));REQUIRE(ExactBinary64(.5,half,budget,reason));
  auto middle=[&](const ExactRatio& a,const ExactRatio& b){ExactRatio sum,out;REQUIRE(ExactAdd(a,b,sum,budget,reason));REQUIRE(ExactDivide(sum,two,out,budget,reason));return out;};
  ExactBary midpoint{middle(first.first.u,first.last.u),middle(first.first.v,first.last.v)};
  first.last=midpoint;first.end=half;second.first=midpoint;trace.pieces={first,second};
  CHECK_THROWS(BoundaryParameterState(fixture.geometry,points,skin,1,SIZE_MAX,0,noncanonical,fixture.policy,&budget,.01));
}
