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
