/*!
 * \file CNativeGeometryBackend3D_tests.cpp
 * \brief Existing cubic authority exercised through immutable geometry queries.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedReference.hpp"
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
  explicit BackendFixture(bool second_body=false,bool cubic=true,bool shear=false) {
    using namespace SU2Native3D;
    const std::array<Face,12> faces{{{0,2,3},{0,3,1},{4,5,7},{4,7,6},{0,4,6},{0,6,2},{1,3,7},{1,7,5},{0,1,5},{0,5,4},{2,6,7},{2,7,3}}};
    for(unsigned body=0;body<(second_body?2u:1u);++body){std::array<Node,8> nodes;
      for(unsigned i=0;i<8;++i)nodes[i]={100*body+i,{double(i&1)+10*body,double((i>>1)&1),double((i>>2)&1)}};
      if(shear)for(auto& node:nodes)node.p.y+=.37*node.p.x+.61*node.p.z;
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
  auto invalid_query=BackendBudget();const auto saved=result;GeometryProjectionFailure failure;
  CHECK_FALSE(backend.Project({std::numeric_limits<double>::quiet_NaN(),0,0},scope,result,invalid_query,reason,&failure));CHECK(failure==GeometryProjectionFailure::Failure);
  CHECK_FALSE(invalid_query.work_exhausted);CHECK(invalid_query.work<invalid_query.maximum_work);
  CHECK(result.binding.feature==saved.binding.feature);CHECK(result.binding.u==saved.binding.u);CHECK(result.binding.v==saved.binding.v);
  GeometrySupport support;REQUIRE(backend.Support(result.binding,result.surface,support,budget,reason));CHECK(support.body==0);CHECK(support.sheet==*scope.sheet);
  CHECK(result.distance_lower<=.001);CHECK(result.distance_upper>=.001-1e-10);
  auto other_body=scope;other_body.body=100;CHECK_FALSE(backend.Project(query,other_body,result,budget,reason));
  REQUIRE(backend.Project({-.001,.5,.5},scope,result,budget,reason));
  REQUIRE(backend.Support(result.binding,result.surface,support,budget,reason));CHECK(support.sheet==*scope.sheet);
  auto feature=scope;feature.previous=fixture.geometry->EdgePoint({0,2},.5);REQUIRE(backend.Support(feature.previous,0,support,budget,reason));feature.feature=std::make_pair(support.mobility,support.feature);
  CHECK_FALSE(backend.Project(query,feature,result,budget,reason,&failure));CHECK(failure==GeometryProjectionFailure::Unavailable);CHECK(reason.find("feature")!=std::string::npos);
  REQUIRE(backend.Project(query,scope,result,budget,reason,&failure));CHECK(failure==GeometryProjectionFailure::None);CHECK(budget.geometry_failure.empty());
  auto far_budget=BackendBudget();REQUIRE(backend.Project({10.25,.5,-.001},scope,result,far_budget,reason));
  REQUIRE(backend.Support(result.binding,result.surface,support,far_budget,reason));CHECK(support.body==0);CHECK(result.evaluation.point.x<=1+1e-12);
  auto failed_budget=BackendBudget();const auto before_failure=result;const auto huge=std::numeric_limits<double>::max();
  CHECK_FALSE(backend.Project({huge,huge,huge},scope,result,failed_budget,reason));
  CHECK(failed_budget.work>0);CHECK(failed_budget.work<failed_budget.maximum_work/2);
  CHECK(result.binding.feature==before_failure.binding.feature);CHECK(result.binding.u==before_failure.binding.u);CHECK(result.binding.v==before_failure.binding.v);
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
  // A source enclosure may ignore an extra distant mesh face; final acceptance must detect it.
  auto extra=square;auto distant=square.front();for(auto& p:distant)p.x+=5;extra.push_back(distant);
  auto enclosure_budget=BackendBudget();
  REQUIRE(backend.PatchErrorBounds(scope,extra,1e-4,bounds,enclosure_budget,reason,GeometryErrorDirection::ModelToMesh));
  CHECK(bounds.model_to_mesh<=1e-4);CHECK(bounds.model_pieces==0);CHECK_FALSE(bounds.within_tolerance);
  auto final_budget=BackendBudget();REQUIRE(backend.PatchErrorBounds(scope,extra,1e-4,bounds,final_budget,reason));
  CHECK(bounds.outside_tolerance);CHECK_FALSE(bounds.within_tolerance);CHECK(bounds.mesh_to_model>1);
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

TEST_CASE("Prepared curved footprints jointly cover original facet seams", "[NativeGeometryBackend3D][NativeSurfaceMemo3D]") {
  BackendFixture fixture;
  // The top charts are planar cubic maps; the working diagonal crosses their original seam.
  std::vector<std::array<Point,3>> physical{{{{0,0,1},{1,0,1},{0,1,1}}},{{{1,0,1},{1,1,1},{0,1,1}}}};
  auto budget=BackendBudget();budget.maximum_work=16384;
  const auto bounds=fixture.geometry->BoundSurfaceUnion({2,3},physical,1e-4,budget);
  CHECK(bounds.within_tolerance);CHECK_FALSE(bounds.outside_tolerance);
  CHECK(bounds.model_to_mesh<=1e-4);CHECK(bounds.mesh_to_model<=1e-4);
  for(auto& triangle:physical)for(auto& p:triangle)p.z+=2e-4;
  auto outside=BackendBudget();outside.maximum_work=16384;
  const auto separated=fixture.geometry->BoundSurfaceUnion({2,3},physical,1e-4,outside);
  CHECK(separated.outside_tolerance);CHECK_FALSE(separated.within_tolerance);
}

TEST_CASE("Curved target uncertainty refines without relaxing surface tolerance", "[NativeGeometryBackend3D][NativeSurfaceMemo3D]") {
  BackendFixture fixture;
  const auto center=fixture.geometry->Realize(fixture.geometry->FacetPoint(0,1./3,1./3));
  constexpr double tolerance=1e-4;
  auto physical=[&](double gap){return std::vector<std::array<Point,3>>{{{{center.x,center.y,center.z-gap},
      {center.x+1e-6,center.y,center.z-gap},{center.x,center.y+1e-6,center.z-gap}}}};};
  // The known center witnesses the whole small triangle; it is also the cubic's minimum z.
  auto inside=BackendBudget();
  CHECK(fixture.geometry->BoundPhysicalSurface({0},physical(.99*tolerance),tolerance,inside)<=tolerance);
  auto outside=BackendBudget();
  CHECK(fixture.geometry->BoundPhysicalSurface({0},physical(1.01*tolerance),tolerance,outside)>tolerance);
  BackendFixture limited;auto budget=BackendBudget();budget.maximum_work=inside.work-1;
  CHECK_THROWS(limited.geometry->BoundPhysicalSurface({0},physical(.99*tolerance),tolerance,budget));
  CHECK(budget.work_exhausted);
}

TEST_CASE("Physical surface bounds reuse only identical immutable queries", "[NativeGeometryBackend3D][NativeSurfaceMemo3D]") {
  BackendFixture fixture(true,true);const auto& f=fixture.facets.front();
  const std::vector<SU2Native3D::Id> facets{0,1};
  const std::vector<std::array<Point,3>> physical{{{f.v[0].p,f.v[1].p,f.v[2].p}}};
  auto budget=BackendBudget();const auto bound=fixture.geometry->BoundPhysicalSurface(facets,physical,.01,budget);
  REQUIRE(std::isfinite(bound));REQUIRE(bound<=.01);REQUIRE(budget.work>0);
  const auto work=budget.work;const auto key_work=3+facets.size()+9*physical.size();budget.maximum_work=work+key_work;
  CHECK(fixture.geometry->BoundPhysicalSurface(facets,physical,.01,budget)==bound);
  CHECK(budget.work==work+key_work);CHECK_FALSE(budget.work_exhausted);
  auto none=budget;CHECK_THROWS(fixture.geometry->BoundPhysicalSurface(facets,physical,.01,none));CHECK(none.work_exhausted);
  budget.maximum_work+=key_work;
  auto sticky=budget;sticky.work_exhausted=true;
  CHECK_THROWS(fixture.geometry->BoundPhysicalSurface(facets,physical,.01,sticky));CHECK(sticky.work==budget.work);
  auto exceeded=budget;exceeded.work=exceeded.maximum_work+1;
  CHECK_THROWS(fixture.geometry->BoundPhysicalSurface(facets,physical,.01,exceeded));CHECK(exceeded.work_exhausted);
  auto changed=physical;changed[0][0].x=std::nextafter(changed[0][0].x,-INFINITY);
  auto coordinates=budget;CHECK_THROWS(fixture.geometry->BoundPhysicalSurface(facets,changed,.01,coordinates));CHECK(coordinates.work_exhausted);
  auto tolerance=budget;CHECK_THROWS(fixture.geometry->BoundPhysicalSurface(facets,physical,.02,tolerance));CHECK(tolerance.work_exhausted);
  auto support=budget;CHECK_THROWS(fixture.geometry->BoundPhysicalSurface({0},physical,.01,support));CHECK(support.work_exhausted);
  auto body=budget;CHECK_THROWS(fixture.geometry->BoundPhysicalSurface({100,101},physical,.01,body));CHECK(body.work_exhausted);
  auto storage=budget;storage.maximum_bytes=0;
  CHECK_THROWS(fixture.geometry->BoundPhysicalSurface(facets,physical,.01,storage));
  auto fresh=BackendBudget();CHECK(fixture.geometry->BoundPhysicalSurface({100,101},physical,.01,fresh)>9);
}

TEST_CASE("Prepared surface geometry is reused across distinct physical queries", "[NativeGeometryBackend3D][NativeSurfaceMemo3D]") {
  BackendFixture warm, fresh, overlapping, different_tolerance, tighter_tolerance;
  const auto& f=warm.facets.front();
  const std::vector<std::array<Point,3>> physical{{{f.v[0].p,f.v[1].p,f.v[2].p}}};
  auto shifted=physical;for(auto& point:shifted[0])point.z+=.001;
  auto warmup=BackendBudget();warm.geometry->BoundPhysicalSurface({0,1},physical,.01,warmup);
  auto reused=BackendBudget(), cold=BackendBudget();
  const auto actual=warm.geometry->BoundPhysicalSurface({0,1},shifted,.01,reused);
  const auto expected=fresh.geometry->BoundPhysicalSurface({0,1},shifted,.01,cold);
  CHECK(actual==expected);CHECK(reused.work<cold.work);

  auto other=physical;for(auto& point:other[0])point.z+=.002;
  auto reused_overlap=BackendBudget(), cold_overlap=BackendBudget();
  const auto actual_overlap=warm.geometry->BoundPhysicalSurface({0,1,2},other,.01,reused_overlap);
  const auto expected_overlap=overlapping.geometry->BoundPhysicalSurface({0,1,2},other,.01,cold_overlap);
  CHECK(actual_overlap==expected_overlap);CHECK(reused_overlap.work<cold_overlap.work);

  auto reused_tolerance=BackendBudget(), cold_tolerance=BackendBudget();
  const auto actual_tolerance=warm.geometry->BoundPhysicalSurface({0,1},shifted,.02,reused_tolerance);
  const auto expected_tolerance=different_tolerance.geometry->BoundPhysicalSurface({0,1},shifted,.02,cold_tolerance);
  CHECK(actual_tolerance==expected_tolerance);

  auto tight=physical;for(auto& point:tight[0])point.z-=.001;
  auto reused_tight=BackendBudget(), cold_tight=BackendBudget();
  const auto actual_tight=warm.geometry->BoundPhysicalSurface({0,1},tight,.005,reused_tight);
  const auto expected_tight=tighter_tolerance.geometry->BoundPhysicalSurface({0,1},tight,.005,cold_tight);
  CHECK(actual_tight==expected_tight);
}

TEST_CASE("Surface union bounds span a noncoplanar triangulated graph without filling holes", "[NativeGeometryBackend3D]") {
  const bool shear=GENERATE(false,true);BackendFixture fixture(false,true,shear);
  std::vector<std::array<Point,3>> physical;
  constexpr unsigned n=8;
  for(Id facet:{Id(0),Id(1)}) {
    auto point=[&](unsigned u,unsigned v){
      const auto& f=fixture.reference->Get(facet);const std::array<unsigned,3> weights{n-u-v,u,v};
      std::vector<unsigned> nonzero;for(unsigned k=0;k<3;++k)if(weights[k])nonzero.push_back(k);
      if(nonzero.size()==1)return fixture.geometry->Realize(fixture.geometry->Vertex(f.v[nonzero[0]].id));
      if(nonzero.size()==2){unsigned a=nonzero[0],b=nonzero[1];if(f.v[a].id>f.v[b].id)std::swap(a,b);
        return fixture.geometry->Realize(fixture.geometry->EdgePoint({f.v[a].id,f.v[b].id},double(weights[b])/n));}
      return fixture.geometry->Realize(fixture.geometry->FacetPoint(facet,double(u)/n,double(v)/n));};
    for(unsigned u=0;u<n;++u)for(unsigned v=0;v<n-u;++v) {
      physical.push_back({point(u,v),point(u+1,v),point(u,v+1)});
      if(u+v+2<=n)physical.push_back({point(u+1,v),point(u+1,v+1),point(u,v+1)});
    }
  }
  SECTION("complete graph") {
    auto budget=BackendBudget();budget.maximum_work=size_t(1)<<20;
    const auto bounds=fixture.geometry->BoundSurfaceUnion({0,1},physical,.001,budget);
    CHECK(bounds.within_tolerance);CHECK_FALSE(bounds.outside_tolerance);
    CHECK(bounds.model_to_mesh<=.001);CHECK(bounds.mesh_to_model<=.001);
  }
  SECTION("missing physical coverage") {
    physical.erase(physical.begin()+23);auto budget=BackendBudget();
    const auto bounds=fixture.geometry->BoundSurfaceUnion({0,1},physical,.001,budget);
    CHECK(bounds.outside_tolerance);CHECK_FALSE(bounds.within_tolerance);CHECK(bounds.model_to_mesh>.001);
  }
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
  for(const auto& row:entity->GeometryCertificates(budget)){CHECK(row.second.model_to_mesh<=.01);CHECK_FALSE(row.second.within_tolerance);}
  for(const auto& row:entity->GeometryCertificates(budget,true)){CHECK(row.second.within_tolerance);CHECK(row.second.mesh_to_model<=.01);}
  ReferenceState state{PlanarReference::WithParameters(fixture.reference,entity),fixture.facets,{"wall"},0};
  Patch support;support.surface=skin;support.parameters=points;
  auto insertion=state.original->PrivateParameters(support,&budget);
  const auto inserted=insertion->BindNewPoint({1000,{.25,.5,0}},{0});
  CHECK(inserted.p.z<0);REQUIRE(insertion->Parameter(inserted.p));
  CHECK(fixture.geometry->Admissible(insertion->Parameter(inserted.p)->binding,inserted.p));
  CHECK(state.original->Parameter(inserted.p)==nullptr);
  auto guidance=state.original->WithAtlasReservation(budget);
  const std::array<Point,3> diagonal{{points.at(0).node.p,points.at(2).node.p,points.at(1).node.p}};
  REQUIRE_FALSE(fixture.geometry->Supports(0,fixture.geometry->Vertex(1)));
  const auto before_guidance=budget.work;
  const auto deviation=guidance->SurfaceDeviation(0,diagonal,true);
  CHECK(std::isfinite(deviation));CHECK(deviation>0);CHECK(deviation<.1);
  CHECK(budget.work>before_guidance);
  CHECK(entity->Snapshot()==guidance->Parameters()->Snapshot());
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
  for(const auto& row:rebound->GeometryCertificates(budget,true)){CHECK(row.second.model_to_mesh<=.01);CHECK(row.second.mesh_to_model<=.01);CHECK(row.second.within_tolerance);}
  auto missing=entity->Coverage();REQUIRE_FALSE(missing.traces.empty());missing.traces.erase(missing.traces.begin());
  CHECK_THROWS(BoundaryParameterState(fixture.geometry,points,skin,1,SIZE_MAX,0,missing,fixture.policy,&budget,.01));
  auto noncanonical=entity->Coverage();auto& trace=noncanonical.traces.begin()->second;REQUIRE(trace.pieces.size()==1);
  auto first=trace.pieces.front(),second=first;ExactRatio two,half;REQUIRE(ExactInteger(2,two,budget,reason));REQUIRE(ExactBinary64(.5,half,budget,reason));
  auto middle=[&](const ExactRatio& a,const ExactRatio& b){ExactRatio sum,out;REQUIRE(ExactAdd(a,b,sum,budget,reason));REQUIRE(ExactDivide(sum,two,out,budget,reason));return out;};
  ExactBary midpoint{middle(first.first.u,first.last.u),middle(first.first.v,first.last.v)};
  first.last=midpoint;first.end=half;second.first=midpoint;trace.pieces={first,second};
  CHECK_THROWS(BoundaryParameterState(fixture.geometry,points,skin,1,SIZE_MAX,0,noncanonical,fixture.policy,&budget,.01));
}
TEST_CASE("Native curved projection restricts closest surface and reports attempted work", "[NativeGeometryBackend3D]") {
  BackendFixture fixture;
  const Point query{.25, .25, 1.1};
  const auto unrestricted = fixture.geometry->ClosestCurved(0, query, 1e-6, 16384);
  CHECK(unrestricted.facet != 0);
  size_t work = 0;
  const auto only_bottom = [](Id facet) { return facet == 0; };
  const auto restricted = fixture.geometry->ClosestCurved(0, query, 1e-6, 16384, SIZE_MAX, only_bottom, &work);
  CHECK(restricted.facet == 0);
  CHECK(restricted.lower > unrestricted.upper);
  CHECK(work == restricted.work);
  work = 99;
  CHECK_THROWS(fixture.geometry->ClosestCurved(0, query, 1e-6, 1, SIZE_MAX, only_bottom, &work));
  CHECK(work == 1);
  work = 99;
  CHECK_THROWS(fixture.geometry->ClosestCurved(0, query, 1e-6, 16384, SIZE_MAX,
                                               [](Id) { return false; }, &work));
  CHECK(work == 0);
}

TEST_CASE("Numerical model projection reuses fixed cubic nets with bounded work", "[NativeGeometryBackend3D]") {
  BackendFixture fixture;NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);std::string reason;
  GeometryProjectionScope scope;scope.body=0;scope.hint=0;scope.previous=fixture.geometry->FacetPoint(0,.25,.25);
  scope.sheet=fixture.policy.sheets.at(0);scope.absolute_error=1e-8;
  auto initial=BackendBudget();GeometryEvaluation source;
  REQUIRE(backend.Evaluate(scope.previous,0,source,initial,reason));
  for(double offset:{.001,.002,.003}) {
    const Point query{source.point.x+offset*source.normal.x,source.point.y+offset*source.normal.y,source.point.z+offset*source.normal.z};
    auto budget=BackendBudget();budget.maximum_work=4096;GeometryProjection foot;
    const bool projected=backend.Project(query,scope,foot,budget,reason);INFO(reason);REQUIRE(projected);
    CHECK(budget.work<budget.maximum_work);CHECK(foot.distance_lower==0);
    const auto certified=fixture.geometry->ClosestCurved(0,query,1e-8,16384);
    CHECK(foot.distance_upper>=certified.lower);CHECK(foot.distance_upper<=certified.upper+1e-7);
    REQUIRE(backend.AssociationValid(foot.binding,foot.evaluation.point,reason));
    const auto held=foot;auto limited=BackendBudget();limited.maximum_work=8;
    CHECK_FALSE(backend.Project(query,scope,foot,limited,reason));CHECK(limited.work_exhausted);
    CHECK(foot.binding.feature==held.binding.feature);CHECK(foot.binding.u==held.binding.u);CHECK(foot.binding.v==held.binding.v);
  }
}

TEST_CASE("Model projection accepts a stationary shared-edge foot under roundoff", "[NativeGeometryBackend3D]") {
  using namespace SU2NativeCurved;
  BackendFixture fixture(false,true,true);NativeMappedGeometryBackend3D backend(fixture.geometry,fixture.policy);
  const Point query{0,.80499999999999994,.5};const Vec<3> target{query.x,query.y,query.z};std::string reason;
  for(Id hint:{Id(4),Id(5)}) {
    std::array<Vec<3>,10> net;const auto& source=fixture.geometry->Curves().at(hint);
    for(unsigned k=0;k<10;++k)net[k]={source[k].x,source[k].y,source[k].z};
    double u=0,v=0;SU2NativeCurved::detail::ClosestOnTriangle(target,net[0],net[1],net[2],u,v);
    auto evaluate=[&](double a,double b){return SU2NativeCurved::detail::TrianglePatch(net,1-a-b,a,b);};
    auto derivatives=[&](double a,double b,Vec<3>& first,Vec<3>& second){SU2NativeCurved::detail::TriangleDerivatives(net,a,b,first,second);};
    REQUIRE(SU2NativeCurved::detail::MinimizeTriangle(target,u,v,evaluate,derivatives));
    CHECK(Norm<3>(Sub<3>(evaluate(u,v),target))<1e-12);
    GeometryProjectionScope scope;scope.body=0;scope.hint=hint;scope.previous=fixture.geometry->FacetPoint(hint,.25,.25);
    scope.sheet=fixture.policy.sheets.at(hint);scope.feature=std::make_pair(2u,*scope.sheet);scope.absolute_error=.000125;
    auto budget=BackendBudget();budget.maximum_work=4096;GeometryProjection projected;
    const bool ok=backend.Project(query,scope,projected,budget,reason);INFO(reason);REQUIRE(ok);
    CHECK(projected.distance_upper<1e-10);REQUIRE(backend.AssociationValid(projected.binding,projected.evaluation.point,reason));
  }
}

TEST_CASE("Captured M6 projection is stable in local model coordinates", "[NativeGeometryBackend3D]") {
  const std::array<Point,10> net{{{1.1399865803007889,1.1827308358735329,0.00034308959017548718},
      {1.1402067706316801,1.184155973197023,0.00036539223957894449},
      {1.1403759768693289,1.1841081489400951,0.00034278444612927833},
      {1.1400599770663551,1.1832058816512316,0.00035052371456477052},
      {1.1401333738445938,1.1836809274253701,0.00035795794133556039},
      {1.1402631727007879,1.1841400317808868,0.00035785622676684427},
      {1.1403195747787203,1.1841240903622716,0.00035032028524237301},
      {1.140246178013149,1.1836490445845744,0.00034288616081168605},
      {1.1401163791569688,1.1831899402290535,0.00034298787549307696},
      {1.1401897759231776,1.1836649860065718,0.0003504220050729595}}};
  using namespace SU2NativeCurved;
  const Vec<3> query{1.1401517230489573,1.1837996888661504,.00035981657722808017};
  std::array<Vec<3>,10> local;for(unsigned k=0;k<10;++k)local[k]={net[k].x,net[k].y,net[k].z};
  const auto target=SU2NativeCurved::detail::NormalizeTriangleProjection(local,query);
  double u=0,v=0;SU2NativeCurved::detail::ClosestOnTriangle(target,local[0],local[1],local[2],u,v);
  auto value=[&](double a,double b){return SU2NativeCurved::detail::TrianglePatch(local,1-a-b,a,b);};
  auto derivative=[&](double a,double b,Vec<3>& first,Vec<3>& second){SU2NativeCurved::detail::TriangleDerivatives(local,a,b,first,second);};
  REQUIRE(SU2NativeCurved::detail::MinimizeTriangle(target,u,v,value,derivative));
  std::array<Vec<3>,10> original;for(unsigned k=0;k<10;++k)original[k]={net[k].x,net[k].y,net[k].z};
  const auto foot=SU2NativeCurved::detail::TrianglePatch(original,1-u-v,u,v);
  CHECK(Norm<3>(Sub<3>(foot,query))<1e-8);
}


TEST_CASE("Captured M6 surface foot within projection accuracy is available", "[NativeGeometryBackend3D]") {
  using namespace SU2Native3D;
  const std::array<Point,10> captured{{{0.79605229720550796,0.01789019128901161,-0.0024093783025947819},
      {0.79711543123895912,0.01594789472874154,-0.0022102837192061989},
      {0.79755222208546239,0.017466265292026831,-0.002209017201393812},
      {0.79640667538443455,0.017242759055121466,-0.0023430147972096902},
      {0.79676105346660331,0.016595326848413357,-0.0022766505098608669},
      {0.79726102819182265,0.016454018248704738,-0.0022098615791582353},
      {0.79740662513663074,0.016960141770930939,-0.0022094393740207845},
      {0.79705224735474389,0.017607573893280451,-0.0022758060876628973},
      {0.79655227233329329,0.017748882576209733,-0.0023425926248020301},
      {0.79690665037856045,0.017101450379868509,-0.0022762280393128272}}};
  const Point a=captured[0], b=captured[1], c=captured[2];
  const Point u{b.x-a.x,b.y-a.y,b.z-a.z},v{c.x-a.x,c.y-a.y,c.z-a.z};
  const Point d{a.x-(u.y*v.z-u.z*v.y)*1000,a.y-(u.z*v.x-u.x*v.z)*1000,a.z-(u.x*v.y-u.y*v.x)*1000};
  const std::array<Node,4> nodes{{{0,a},{1,b},{2,c},{3,d}}};
  const std::array<Face,4> faces{{{0,1,2},{1,0,3},{2,1,3},{0,2,3}}};
  const unsigned power[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
  std::vector<Facet> facets;MappedFacetReference::CurvedNets nets;std::map<Id,double> errors;
  for(Id i=0;i<4;++i) {
    const auto f=faces[i];facets.push_back({i,0,{nodes[f[0]],nodes[f[1]],nodes[f[2]]}});errors[i]=1e-13;
    auto& net=nets[i];
    for(unsigned k=0;k<10;++k) {
      Point p{};for(unsigned j=0;j<3;++j){p.x+=power[k][j]*nodes[f[j]].p.x/3;p.y+=power[k][j]*nodes[f[j]].p.y/3;p.z+=power[k][j]*nodes[f[j]].p.z/3;}
      net[k]=p;
      for(unsigned t=0;t<10;++t) {
        bool same=true;for(unsigned j=0;j<3;++j) {
          unsigned weight=0;for(unsigned h=0;h<3;++h)if(f[j]==h)weight=power[t][h];
          same=same && power[k][j]==weight;
        }
        if(same)net[k]=captured[t];
      }
    }
  }
  nets[0]=captured;
  auto reference=std::make_shared<PlanarReference>(facets);
  auto geometry=std::make_shared<MappedFacetReference>(reference,errors,18,SIZE_MAX,false,nets);
  auto budget=BackendBudget();MobilityPolicy policy;std::string reason;
  REQUIRE(geometry->BuildMobilityPolicy(45,{{0,2}},policy,budget,reason));
  for(Id i=1;i<4;++i)REQUIRE(policy.sheets.at(i)!=policy.sheets.at(0));
  NativeMappedGeometryBackend3D backend(geometry,policy);
  GeometryProjectionScope scope;scope.body=geometry->Body(0);scope.hint=0;scope.sheet=policy.sheets.at(0);
  scope.previous=geometry->FacetPoint(0,1./3,1./3);scope.absolute_error=1.25e-5;
  GeometryProjection result;GeometryProjectionFailure failure;
  const bool projected=backend.Project({0.7965838642222336,0.016919043008876575,-0.0023098310109004906},scope,result,budget,reason,&failure);
  INFO(reason);REQUIRE(projected);CHECK(failure==GeometryProjectionFailure::None);
  CHECK(result.distance_upper<=scope.absolute_error);
  REQUIRE(backend.AssociationValid(result.binding,result.evaluation.point,reason));
  GeometrySupport support;REQUIRE(backend.Support(result.binding,result.surface,support,budget,reason));
  CHECK(support.body==scope.body);CHECK(support.sheet==*scope.sheet);
}
