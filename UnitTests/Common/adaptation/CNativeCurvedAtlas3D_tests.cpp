/*!
 * \file CNativeCurvedAtlas3D_tests.cpp
 * \brief Connected curved correspondence across original vertices and physical curve samples.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../Common/include/adaptation/CNativeTraceCoverage3D.hpp"
#include "../../../Common/include/adaptation/CNativeDistributed3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include "../../../Common/include/adaptation/CNativeRuntimeProfile3D.hpp"
using namespace SU2Native3D;
using namespace SU2NativeGeometry;
namespace {
AtlasBudget RemappingBudget(){return {size_t(1)<<29,size_t(1)<<28,4096};}
struct RemappingFixture {
  std::vector<Facet> facets;
  std::shared_ptr<PlanarReference> definition;
  std::shared_ptr<MappedFacetReference> geometry;
  std::shared_ptr<BoundaryParameterState> state;
  std::shared_ptr<const PlanarReference> reference;
  BoundaryPoints points;
  std::vector<SurfaceFace> surface;
  Patch patch;
  MobilityPolicy policy;
  explicit RemappingFixture(bool crease) {
    if(crease){std::array<Node,8> c;for(unsigned i=0;i<8;++i)c[i]={10+10*i,{double(i&1),double((i>>1)&1),double((i>>2)&1)}};
      const Node p{0,{.5,0,0}};
      facets={{0,0,{c[0],c[2],p}},{1,0,{p,c[2],c[3]}},{2,0,{p,c[3],c[1]}},
              {3,0,{c[0],p,c[4]}},{4,0,{p,c[1],c[5]}},{5,0,{p,c[5],c[4]}}};
      const unsigned q[4][4]={{4,5,7,6},{2,6,7,3},{0,4,6,2},{1,3,7,5}};
      for(const auto& f:q){facets.push_back({Id(facets.size()),0,{c[f[0]],c[f[1]],c[f[2]]}});facets.push_back({Id(facets.size()),0,{c[f[0]],c[f[2]],c[f[3]]}});}
    }else{const Node a{10,{0,0,0}},b{20,{1,0,0}},c{30,{1,1,0}},d{40,{0,1,0}},p{0,{.5,.5,0}},top{50,{.5,.5,1}};
      facets={{0,0,{a,p,b}},{1,0,{b,p,c}},{2,0,{c,p,d}},{3,0,{d,p,a}},
              {4,0,{a,b,top}},{5,0,{b,c,top}},{6,0,{c,d,top}},{7,0,{d,a,top}}};}
    definition=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;MappedFacetReference::CurvedNets nets;
    const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
    for(const auto& f:facets){errors.emplace(f.id,1e-12);
      if(!crease){auto& net=nets[f.id];for(unsigned q=0;q<10;++q){long double x=0,y=0,z=0;
          for(unsigned k=0;k<3;++k){x+=powers[q][k]*(long double)f.v[k].p.x;y+=powers[q][k]*(long double)f.v[k].p.y;z+=powers[q][k]*(long double)f.v[k].p.z;}
          net[q]={double(x/3),double(y/3),double(z/3)};}if(f.id<4)net[9].z=-.02;}}
    geometry=std::make_shared<MappedFacetReference>(definition,errors,71,SIZE_MAX,false,nets);
    auto budget=RemappingBudget();std::string reason;REQUIRE(geometry->BuildMobilityPolicy(45,{{0,2}},policy,budget,reason));
    for(const auto& f:facets){surface.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
      for(const auto& n:f.v)points.emplace(n.id,BoundaryPoint{n,geometry->Vertex(n.id)});
      if(std::find(surface.back().v.begin(),surface.back().v.end(),Id(0))!=surface.back().v.end())patch.surface.push_back(surface.back());}
    state=std::make_shared<BoundaryParameterState>(geometry,points,surface,1,SIZE_MAX,0,BoundaryCoverage{},policy,&budget);
    reference=PlanarReference::WithParameters(definition,state);
    // Intentionally leave patch.parameters empty: the accepted reference owns the original bindings.
  }
  std::vector<SurfaceFace> Moved(Id old,Id fresh,const std::vector<SurfaceFace>& faces)const {
    auto result=faces;for(auto& face:result)for(auto& id:face.v)if(id==old)id=fresh;return result;
  }
};
}

TEST_CASE("Convex curved atlas releases an original vertex and remaps a second accepted event", "[NativeCurvedAtlasRemap3D][NativeCurvedAtlas3D]") {
  RemappingFixture fixture(false);auto budget=RemappingBudget();std::string reason;CurvedAtlas atlas;
  REQUIRE(fixture.geometry->MobilityDimension(fixture.geometry->Vertex(0),fixture.policy)==2);
  REQUIRE(PrepareCurvedAtlas(fixture.patch,*fixture.reference,Id(0),atlas,budget,reason));
  BoundaryPoint moved;INFO(reason);REQUIRE(ProposeAtlasBinding(atlas,0,{100,{.62,.54,0}},moved,budget,reason));
  CHECK(moved.binding.dimension==2);CHECK(moved.node.p.x!=fixture.points.at(0).node.p.x);
  auto first=fixture.Moved(0,100,fixture.patch.surface);BoundaryPoints parameters=fixture.points;parameters.erase(0);parameters.emplace(100,moved);
  BoundaryCoverage coverage;REQUIRE(RemapAtlasSurface(atlas,first,parameters,coverage,budget,reason));CHECK(coverage.faces.size()==first.size());
  CHECK(std::any_of(coverage.traces.begin(),coverage.traces.end(),[](const auto& row){return row.second.pieces.size()>1;}));
  auto all=fixture.Moved(0,100,fixture.surface);auto saved=fixture.state->Snapshot();
  auto accepted=std::make_shared<BoundaryParameterState>(fixture.geometry,parameters,all,2,SIZE_MAX,0,coverage,fixture.policy,&budget);
  auto reference=PlanarReference::WithParameters(fixture.definition,accepted);CHECK(fixture.state->Snapshot()==saved);
  Patch next;next.surface=first;next.coverage=coverage;CurvedAtlas second;
  REQUIRE(PrepareCurvedAtlas(next,*reference,Id(100),second,budget,reason));
  BoundaryPoint again;REQUIRE(ProposeAtlasBinding(second,100,{1,{.38,.58,0}},again,budget,reason));
  parameters.erase(100);parameters.emplace(1,again);auto faces=fixture.Moved(100,1,first);BoundaryCoverage twice;
  const bool remapped=RemapAtlasSurface(second,faces,parameters,twice,budget,reason);INFO(reason);
  REQUIRE(remapped);CHECK(twice.faces.size()==faces.size());
  for(const auto& edge:coverage.traces)if(edge.first[0]!=100&&edge.first[1]!=100){REQUIRE(twice.traces.count(edge.first));
    REQUIRE(fixture.geometry->EqualTraces(edge.second,twice.traces.at(edge.first),budget,reason));}
  CHECK(CurvedAtlasWorkspaceBytes(SIZE_MAX)==SIZE_MAX);CHECK(atlas.ResidentBytes()>0);
}

TEST_CASE("Curved atlas moves a true curve sample with the same parameter on both sheets", "[NativeCurvedAtlasRemap3D][NativeCurvedAtlas3D]") {
  RemappingFixture fixture(true);auto budget=RemappingBudget();std::string reason;CurvedAtlas atlas;
  REQUIRE(fixture.geometry->MobilityDimension(fixture.geometry->Vertex(0),fixture.policy)==1);
  REQUIRE(PrepareCurvedAtlas(fixture.patch,*fixture.reference,Id(0),atlas,budget,reason));
  BoundaryPoint moved;INFO(reason);REQUIRE(ProposeAtlasBinding(atlas,0,{100,{.65,0,0}},moved,budget,reason));
  CHECK(moved.node.p.x==Approx(.65));CHECK(moved.node.p.y==0);CHECK(moved.node.p.z==0);
  CHECK(fixture.geometry->MobilityDimension(moved.binding,fixture.policy)==1);
  auto faces=fixture.Moved(0,100,fixture.patch.surface);auto points=fixture.points;points.erase(0);points.emplace(100,moved);BoundaryCoverage coverage;
  REQUIRE(RemapAtlasSurface(atlas,faces,points,coverage,budget,reason));CHECK(coverage.faces.size()==faces.size());
  REQUIRE(coverage.traces.count(Edge{10,100}));CHECK(coverage.traces.at(Edge{10,100}).pieces.size()>=2);
  auto all=fixture.Moved(0,100,fixture.surface);
  CHECK_NOTHROW(BoundaryParameterState(fixture.geometry,points,all,2,SIZE_MAX,0,coverage,fixture.policy,&budget));
}

TEST_CASE("Curved atlas refuses holes changed retained nodes and exhausted resources atomically", "[NativeCurvedAtlasRemap3D][NativeCurvedAtlas3D]") {
  RemappingFixture fixture(false);std::string reason;auto budget=RemappingBudget();CurvedAtlas atlas;
  REQUIRE(PrepareCurvedAtlas(fixture.patch,*fixture.reference,std::nullopt,atlas,budget,reason));const auto saved=atlas.data;
  auto limited=RemappingBudget();limited.maximum_bytes=1;
  CHECK_FALSE(PrepareCurvedAtlas(fixture.patch,*fixture.reference,std::nullopt,atlas,limited,reason));CHECK(atlas.data==saved);
  auto partial=fixture.patch;partial.surface.erase(partial.surface.begin()+1);auto refusal=RemappingBudget();
  CHECK_FALSE(PrepareCurvedAtlas(partial,*fixture.reference,Id(0),atlas,refusal,reason));CHECK(atlas.data==saved);
  auto overridden=fixture.patch;const auto& face=overridden.surface.front();Edge edge{face.v[0],face.v[1]};std::sort(edge.begin(),edge.end());
  EdgeTrace malformed;auto override_work=RemappingBudget();REQUIRE(fixture.reference->ResolveTrace(edge,face,malformed,override_work,reason));
  REQUIRE_FALSE(malformed.pieces.empty());malformed.pieces.back().end=ExactRatio{};overridden.coverage.traces.emplace(edge,malformed);
  auto trace_view=fixture.reference->PrivateParameters(overridden,&override_work);
  CHECK_FALSE(PrepareCurvedAtlas(overridden,*trace_view,Id(0),atlas,override_work,reason));CHECK(atlas.data==saved);
  BoundaryCoverage output;REQUIRE(RemapAtlasSurface(atlas,fixture.patch.surface,fixture.points,output,budget,reason));
  const auto words=EncodeBoundaryAtlas(output,fixture.policy);
  auto work=RemappingBudget();work.maximum_work=0;CHECK_FALSE(RemapAtlasSurface(atlas,fixture.patch.surface,fixture.points,output,work,reason));
  CHECK(EncodeBoundaryAtlas(output,fixture.policy)==words);
  auto corrupt=fixture.points;corrupt.at(10)=fixture.points.at(20);corrupt.at(10).node.id=10;auto changed=RemappingBudget();
  CHECK_FALSE(RemapAtlasSurface(atlas,fixture.patch.surface,corrupt,output,changed,reason));CHECK(EncodeBoundaryAtlas(output,fixture.policy)==words);
}

TEST_CASE("Validated legacy parameters activate equivalent mobility metadata atomically", "[NativeCurvedAtlasRemap3D][NativeCurvedAtlas3D][NativeAtlasPolicyMigration3D]") {
  RemappingFixture fixture(false);
  const auto revision=fixture.state->Revision();const auto geometry_version=fixture.geometry->Version();
  const auto legacy=std::make_shared<BoundaryParameterState>(fixture.geometry,fixture.points,fixture.surface,revision);
  const auto before=legacy->Snapshot();REQUIRE(legacy->Mobility().identity==0);
  auto budget=RemappingBudget();
  const auto migrated=legacy->WithMobilityPolicy(45,{{0,2}},budget);
  REQUIRE(migrated);CHECK(migrated->Snapshot()==fixture.state->Snapshot());
  CHECK(migrated->Authority().get()==fixture.geometry.get());CHECK(migrated->Revision()==revision);
  CHECK(migrated->Geometry().Version()==geometry_version);CHECK(legacy->Snapshot()==before);
  const auto reference=PlanarReference::WithParameters(fixture.definition,migrated);
  REQUIRE(reference->AtlasReservation()==nullptr);
  for(const auto& face:migrated->Surface()) {
    std::array<Point,3> triangle;for(unsigned k=0;k<3;++k)triangle[k]=migrated->Find(face.second.v[k])->node.p;
    Id found=UINT64_MAX;CHECK(reference->FindFace(0,triangle,found));CHECK(found==face.second.facet);
    CHECK(reference->ContainsFace(face.second.facet,triangle));
    auto reversed=triangle;std::swap(reversed[0],reversed[1]);CHECK_FALSE(reference->ContainsFace(face.second.facet,reversed));
    auto altered=triangle;altered[0].x+=.001;CHECK_FALSE(reference->ContainsFace(face.second.facet,altered));
    CHECK_FALSE(reference->FindFace(1,triangle,found));
  }
  const auto active=migrated->Snapshot();
  auto work=RemappingBudget();work.maximum_work=0;
  CHECK_THROWS(legacy->WithMobilityPolicy(45,{{0,2}},work));CHECK(legacy->Snapshot()==before);
  auto memory=RemappingBudget();memory.maximum_bytes=1;
  CHECK_THROWS(legacy->WithMobilityPolicy(45,{{0,2}},memory));CHECK(legacy->Snapshot()==before);
  auto ceiling=RemappingBudget();
  CHECK_THROWS(legacy->WithMobilityPolicy(45,{{0,2}},ceiling,1));CHECK(legacy->Snapshot()==before);
  auto angle=RemappingBudget();CHECK_THROWS(migrated->WithMobilityPolicy(44,{{0,2}},angle));CHECK(migrated->Snapshot()==active);
  auto roles=RemappingBudget();CHECK_THROWS(migrated->WithMobilityPolicy(45,{{0,0}},roles));CHECK(migrated->Snapshot()==active);
  CHECK(fixture.geometry->Version()==geometry_version);CHECK(legacy->Authority().get()==fixture.geometry.get());
}

TEST_CASE("Curved boundary vertex movement crosses seams on successive accepted states", "[NativeCurvedAtlasRemap3D][NativeCurvedAtlas3D]") {
  RemappingFixture fixture(false);auto budget=RemappingBudget();std::string reason;
  auto reference=fixture.reference->WithAtlasReservation(budget);
  Patch old=fixture.patch;const Node apex=fixture.points.at(50).node;
  for(const auto& face:old.surface) {
    Cell cell;cell.id=old.cells.size();
    for(unsigned k=0;k<3;++k)cell.v[k]=fixture.points.at(face.v[k]).node;
    cell.v[3]=apex;
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
    REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
    old.cells.push_back(cell);
  }
  auto preview=[&](const Patch& patch,Id old_id,Node target,const PlanarReference& base) {
    BoundaryPoint point;auto context=base.PrivateParameters(patch);REQUIRE(context);CurvedAtlas atlas;
    const bool prepared=PrepareCurvedAtlas(patch,*context,old_id,atlas,budget,reason);INFO(reason);REQUIRE(prepared);
    const bool proposed=ProposeAtlasBinding(atlas,old_id,target,point,budget,reason);INFO(reason);REQUIRE(proposed);return point;
  };
  auto compare=[&](const Patch& before,const Patch& after,Id old_id,const BoundaryPoint& proposed) {
    const auto& actual=after.parameters.at(proposed.node.id);CHECK(actual.node.p.x==proposed.node.p.x);CHECK(actual.node.p.y==proposed.node.p.y);CHECK(actual.node.p.z==proposed.node.p.z);
    CHECK(actual.binding.feature==proposed.binding.feature);CHECK(actual.binding.version==proposed.binding.version);CHECK(actual.binding.dimension==proposed.binding.dimension);
    CHECK(actual.binding.u==proposed.binding.u);CHECK(actual.binding.v==proposed.binding.v);
    for(const auto& cell:before.cells) {
      auto predicted=cell.v;for(auto& node:predicted)if(node.id==old_id)node=proposed.node;
      std::array<Id,4> key;for(unsigned k=0;k<4;++k)key[k]=predicted[k].id;std::sort(key.begin(),key.end());
      auto found=std::find_if(after.cells.begin(),after.cells.end(),[&](const Cell& candidate){std::array<Id,4> ids;for(unsigned k=0;k<4;++k)ids[k]=candidate.v[k].id;std::sort(ids.begin(),ids.end());return ids==key;});
      REQUIRE(found!=after.cells.end());const Tensor metric{};
      const auto a=Measure({predicted[0].p,predicted[1].p,predicted[2].p,predicted[3].p},metric);
      const auto b=Measure({found->v[0].p,found->v[1].p,found->v[2].p,found->v[3].p},metric);
      CHECK(a.mean_ratio==Approx(b.mean_ratio));CHECK(a.minimum_scaled_jacobian==Approx(b.minimum_scaled_jacobian));
    }
  };
  const auto snapshot=fixture.state->Snapshot();Patch first;
  const auto first_preview=preview(old,0,{100,{.6,.5,-.01}},*reference);
  const bool first_move=MoveVertex(old,0,{100,{.6,.5,-.01}},1000,*reference,first,reason);INFO(reason);REQUIRE(first_move);
  compare(old,first,0,first_preview);
  REQUIRE(first.cells.size()==4);CHECK(first.coverage.faces.size()==4);
  for(const auto& cell:first.cells)CHECK(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
  CHECK(fixture.state->Snapshot()==snapshot);
  auto points=fixture.points;points.erase(0);points.insert_or_assign(100,first.parameters.at(100));
  auto all=fixture.Moved(0,100,fixture.surface);
  auto accepted=std::make_shared<BoundaryParameterState>(fixture.geometry,points,all,2,SIZE_MAX,0,first.coverage,fixture.policy,&budget);
  auto next_reference=PlanarReference::WithParameters(fixture.definition,accepted)->WithAtlasReservation(budget);
  const auto accepted_snapshot=accepted->Snapshot();Patch second;
  const auto second_preview=preview(first,100,{101,{.38,.58,-.01}},*next_reference);
  const bool second_move=MoveVertex(first,100,{101,{.38,.58,-.01}},2000,*next_reference,second,reason);INFO(reason);REQUIRE(second_move);
  compare(first,second,100,second_preview);
  CHECK(first.parameters.at(100).binding.dimension==2);CHECK(second.parameters.at(101).binding.dimension==2);
  CHECK(first.parameters.at(100).binding.feature!=second.parameters.at(101).binding.feature);
  REQUIRE(second.cells.size()==4);CHECK(second.coverage.faces.size()==4);
  for(const auto& cell:second.cells)CHECK(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
  CHECK(accepted->Snapshot()==accepted_snapshot);CHECK(fixture.state->Snapshot()==snapshot);
}

TEST_CASE("Fresh curved working patches forget interior history over repeated accepted events", "[NativeCurvedAtlasRemap3D][NativeCurvedAtlas3D]") {
  RemappingFixture fixture(false);auto accepted=fixture.state;auto parameters=fixture.points;
  auto surface=fixture.surface,local=fixture.patch.surface;BoundaryCoverage previous;
  std::map<unsigned,std::vector<char>> repeated;std::map<unsigned,std::pair<size_t,size_t>> resources;Id moving=0;
  for(unsigned event=0;event<16;++event) {
    auto budget=RemappingBudget();std::string reason;const auto snapshot=accepted->Snapshot();
    auto reference=PlanarReference::WithParameters(fixture.definition,accepted);
    Patch patch;patch.surface=local;patch.parameters=parameters;patch.coverage=previous;CurvedAtlas atlas;
    const bool prepared=PrepareCurvedAtlas(patch,*reference,moving,atlas,budget,reason);INFO(event);INFO(reason);REQUIRE(prepared);
    // Four original charts and at most two triangles per current constraint point.
    CHECK(atlas.ReferenceTriangleCount()<=4+2*5);
    const unsigned phase=event%2;const Id next=100+event;
    const auto binding=fixture.geometry->FacetPoint(phase?3:1,.2,.3);
    const BoundaryPoint point{{next,fixture.geometry->Realize(binding)},binding};
    parameters.erase(moving);parameters.emplace(next,point);
    auto candidate=fixture.Moved(moving,next,local);BoundaryCoverage fresh;
    const bool remapped=RemapAtlasSurface(atlas,candidate,parameters,fresh,budget,reason);INFO(reason);REQUIRE(remapped);
    size_t pieces=0;for(const auto& face:fresh.faces)pieces+=face.second.pieces.size();
    // Each pair of triangles intersects in a convex polygon with at most six vertices.
    CHECK(pieces<=4*candidate.size()*atlas.ReferenceTriangleCount());
    BoundaryCoverage normalized;
    for(const auto& face:fresh.faces){auto key=face.first;for(auto& id:key)if(id==next)id=10000;normalized.faces.emplace(key,face.second);}
    for(const auto& edge:fresh.traces){auto key=edge.first;for(auto& id:key)if(id==next)id=10000;normalized.traces.emplace(key,edge.second);}
    const auto encoded=EncodeBoundaryAtlas(normalized,{});
    if(event>=2){const auto found=repeated.find(phase);if(found==repeated.end())repeated.emplace(phase,encoded);else CHECK(found->second==encoded);}
    surface=fixture.Moved(moving,next,surface);
    auto next_state=std::make_shared<BoundaryParameterState>(fixture.geometry,parameters,surface,accepted->Revision()+1,
        SIZE_MAX,0,fresh,fixture.policy,&budget);
    if(event>=2){const auto found=resources.find(phase);INFO(event);INFO(budget.work);INFO(budget.peak_bytes);
      if(found==resources.end())resources.emplace(phase,std::make_pair(budget.work,budget.peak_bytes));
      else {CHECK(budget.work<=found->second.first);CHECK(budget.peak_bytes<=found->second.second);}}
    CHECK(accepted->Snapshot()==snapshot);CHECK(next_state->Authority()==fixture.geometry);
    previous=std::move(fresh);local=std::move(candidate);accepted=std::move(next_state);moving=next;
  }
}

TEST_CASE("A nonseparating crease retains two local sheets during a vertex move", "[NativeCurvedAtlas3D]") {
  const Node a{10,{0,0,-1}},b{20,{0,0,1}},c{30,{1,.1,0}},d{40,{1,-.1,0}},p{0,{0,0,0}};
  std::vector<Facet> facets{{0,0,{a,p,c}},{1,0,{p,b,c}},{2,0,{a,d,p}},
                            {3,0,{p,d,b}},{4,0,{a,c,d}},{5,0,{b,d,c}}};
  auto definition=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;
  for(const auto& face:facets)errors.emplace(face.id,1e-12);
  auto geometry=std::make_shared<MappedFacetReference>(definition,errors,91);
  auto budget=RemappingBudget();MobilityPolicy policy;std::string reason;
  const bool policy_ok=geometry->BuildMobilityPolicy(120,{{0,2}},policy,budget,reason);INFO(reason);REQUIRE(policy_ok);
  REQUIRE(policy.sheets.at(0)==policy.sheets.at(2));
  BoundaryPoints points;for(const Node node:{a,b,c,d,p})points.emplace(node.id,BoundaryPoint{node,geometry->Vertex(node.id)});
  for(const auto edge:std::array<Edge,2>{{{0,10},{0,20}}}) {
    const auto binding=geometry->EdgePoint(edge,.5);const Id id=edge[1]==10?50:60;
    points.emplace(id,BoundaryPoint{{id,geometry->Realize(binding)},binding});
  }
  std::vector<SurfaceFace> surface{{{10,50,30},0},{{50,0,30},0},{{0,60,30},1},{{60,20,30},1},
                                   {{10,40,50},2},{{50,40,0},2},{{0,40,60},3},{{60,40,20},3},
                                   {{10,30,40},4},{{20,40,30},5}};
  auto state=std::make_shared<BoundaryParameterState>(geometry,points,surface,1,SIZE_MAX,0,BoundaryCoverage{},policy,&budget);
  auto reference=PlanarReference::WithParameters(definition,state)->WithAtlasReservation(budget);
  Patch old;old.surface={surface[1],surface[2],surface[5],surface[6]};
  for(const auto ids:std::array<std::array<Id,4>,2>{{{{50,30,0,40}},{{0,30,60,40}}}}) {
    Cell cell;cell.id=old.cells.size();for(unsigned k=0;k<4;++k)cell.v[k]=points.at(ids[k]).node;
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
    REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);old.cells.push_back(cell);
  }
  const auto snapshot=state->Snapshot();Patch fresh;
  const bool moved=MoveVertex(old,0,{100,{0,0,.2}},1000,*reference,fresh,reason);INFO(reason);REQUIRE(moved);
  REQUIRE(fresh.cells.size()==2);CHECK(fresh.coverage.faces.size()==4);
  REQUIRE(fresh.parameters.count(100));CHECK(geometry->MobilityDimension(fresh.parameters.at(100).binding,policy)==1);
  for(const auto& cell:fresh.cells)CHECK(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
  CHECK(state->Snapshot()==snapshot);
}

TEST_CASE("Overlapping curved stars retain bounded correspondence across alternating moves", "[NativeCurvedAtlas3D][NativeOverlappingAtlas3D]") {
  RemappingFixture fixture(false);auto setup=RemappingBudget();const auto qb=fixture.geometry->EdgePoint({0,20},.5);
  auto points=fixture.points;points.emplace(60,BoundaryPoint{{60,fixture.geometry->Realize(qb)},qb});
  auto surface=fixture.surface;surface.erase(surface.begin(),surface.begin()+2);
  surface.insert(surface.begin(),{{{10,0,60},0},{{10,60,20},0},{{20,60,30},1},{{60,0,30},1}});
  auto accepted=std::make_shared<BoundaryParameterState>(fixture.geometry,points,surface,1,SIZE_MAX,0,BoundaryCoverage{},fixture.policy,&setup);
  surface.clear();for(const auto& row:accepted->Surface())surface.push_back(row.second);
  const auto original=fixture.state->Snapshot();Id moving[2]={0,60};
  using Measures=std::array<size_t,5>;std::map<unsigned,Measures> baseline;
  for(unsigned event=0;event<16;++event) {
    const auto started=std::chrono::steady_clock::now();
    const unsigned phase=event%4,slot=event%2;const Id old=moving[slot],next=100+event;
    auto budget=RemappingBudget();std::string reason;const auto snapshot=accepted->Snapshot();
    auto reference=PlanarReference::WithParameters(fixture.definition,accepted);
    Patch patch;for(const auto& face:surface)if(std::find(face.v.begin(),face.v.end(),old)!=face.v.end())patch.surface.push_back(face);
    CurvedAtlas atlas;const bool prepared=PrepareCurvedAtlas(patch,*reference,old,atlas,budget,reason);INFO(event);INFO(reason);REQUIRE(prepared);
    const std::array<SurfaceBinding,4> bindings{{fixture.geometry->FacetPoint(1,.75,.125),fixture.geometry->FacetPoint(0,.5,.375),
                                                  fixture.geometry->FacetPoint(3,.75,.125),fixture.geometry->FacetPoint(1,.5,.125)}};
    const auto binding=bindings[phase];points.erase(old);points.emplace(next,BoundaryPoint{{next,fixture.geometry->Realize(binding)},binding});
    auto local=fixture.Moved(old,next,patch.surface);BoundaryCoverage fresh;
    const bool remapped=RemapAtlasSurface(atlas,local,points,fresh,budget,reason);INFO(reason);REQUIRE(remapped);
    BoundaryCoverage coverage=accepted->Coverage();for(const auto& face:patch.surface)coverage.faces.erase(face.v);
    for(const auto& row:fresh.faces)coverage.faces.insert_or_assign(row.first,row.second);
    surface=fixture.Moved(old,next,surface);std::set<Edge> edges;
    for(const auto& face:surface)for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());edges.insert(edge);}
    for(auto it=coverage.traces.begin();it!=coverage.traces.end();)if(!edges.count(it->first))it=coverage.traces.erase(it);else ++it;
    for(const auto& row:fresh.traces)coverage.traces.insert_or_assign(row.first,row.second);
    auto next_state=std::make_shared<BoundaryParameterState>(fixture.geometry,points,surface,accepted->Revision()+1,SIZE_MAX,0,coverage,fixture.policy,&budget);
    CHECK(accepted->Snapshot()==snapshot);CHECK(fixture.state->Snapshot()==original);CHECK(next_state->Authority()==fixture.geometry);
    CHECK(next_state->Mobility().bodies==fixture.policy.bodies);
    size_t face_pieces=0,trace_pieces=0,bits=0;
    auto ratio=[&](const ExactRatio& r){for(const auto* limbs:{&r.numerator,&r.denominator})if(!limbs->empty()){
      size_t width=32*(limbs->size()-1);for(uint32_t top=limbs->back();top;top>>=1)++width;bits=std::max(bits,width);}};
    auto bary=[&](const ExactBary& b){ratio(b.u);ratio(b.v);};
    for(const auto& row:coverage.faces)for(const auto& piece:row.second.pieces){++face_pieces;for(const auto& b:piece.reference)bary(b);for(const auto& b:piece.active)bary(b);}
    for(const auto& row:coverage.traces)for(const auto& piece:row.second.pieces){++trace_pieces;bary(piece.first);bary(piece.last);ratio(piece.end);}
    const Measures measures{{face_pieces,trace_pieces,bits,budget.work,budget.peak_bytes}};INFO(phase);INFO(budget.work);INFO(budget.peak_bytes);
    if(event>=4){const auto found=baseline.find(phase);if(found==baseline.end())baseline.emplace(phase,measures);
      else for(unsigned k=0;k<measures.size();++k)CHECK(measures[k]<=found->second[k]);}
    std::ostringstream cost;cost<<std::setprecision(17)<<"NATIVE_REFERENCE_EVENT {\"scope\":\"private_overlapping_surface_moves\",\"rank\":"<<SU2_MPI::GetRank()
      <<",\"event\":"<<event<<",\"current_points\":"<<points.size()<<",\"current_faces\":"<<surface.size()
      <<",\"reference_triangles\":"<<atlas.ReferenceTriangleCount()<<",\"face_pieces\":"<<face_pieces<<",\"trace_pieces\":"<<trace_pieces
      <<",\"maximum_integer_bits\":"<<bits<<",\"arithmetic_work\":"<<budget.work<<",\"admitted_peak_bytes\":"<<budget.peak_bytes
      <<",\"accepted_resident_bytes\":"<<next_state->ResidentBytes()<<",\"payload_bytes\":"<<next_state->PayloadBytes()
      <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()
      <<",\"lifetime_process_peak_rss_bytes\":"<<ProcessPeakRSS3D()<<"}\n";
    std::cout<<cost.str();
    accepted=std::move(next_state);surface.clear();for(const auto& row:accepted->Surface())surface.push_back(row.second);moving[slot]=next;
  }
}

TEST_CASE("Neighboring curved moves publish and reload complete boundary state", "[NativeCurvedAtlas3D][NativeOverlappingPublication3D]") {
  const unsigned variant=GENERATE(0u,1u,2u,3u);const bool legacy_initial=variant==1||variant==3,entity_initial=variant>=2;INFO(variant);
  World world;RemappingFixture fixture(false);auto setup=RemappingBudget();std::string reason;
  struct Restore {ShapeAdmission old=GetShapeAdmission();~Restore(){SetShapeAdmission(old);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  const auto qb=fixture.geometry->EdgePoint({0,20},.5);auto points=fixture.points;
  points.emplace(60,BoundaryPoint{{60,fixture.geometry->Realize(qb)},qb});
  auto surface=fixture.surface;surface.erase(surface.begin(),surface.begin()+2);
  surface.insert(surface.begin(),{{{10,0,60},0},{{10,60,20},0},{{20,60,30},1},{{60,0,30},1}});
  BoundaryCoverage legacy;
  if(legacy_initial) {
    const Edge shared{0,60};
    auto mix=[&](const ExactBary& a,const ExactBary& b,double t){ExactBary out;ExactRatio left,right;
      REQUIRE(ExactBinary64(1-t,left,setup,reason));REQUIRE(ExactBinary64(t,right,setup,reason));
      auto component=[&](const ExactRatio& x,const ExactRatio& y){ExactRatio u,v,r;REQUIRE(ExactMultiply(x,left,u,setup,reason));
        REQUIRE(ExactMultiply(y,right,v,setup,reason));REQUIRE(ExactAdd(u,v,r,setup,reason));return r;};
      out.u=component(a.u,b.u);out.v=component(a.v,b.v);return out;};
    for(auto face:surface)if(std::find(face.v.begin(),face.v.end(),shared[0])!=face.v.end() &&
                              std::find(face.v.begin(),face.v.end(),shared[1])!=face.v.end()) {
      std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());
      std::array<SurfaceBinding,3> binding;for(unsigned k=0;k<3;++k)binding[k]=points.at(face.v[k]).binding;
      FaceCoverage identity;REQUIRE(fixture.geometry->ResolveSingleChart(face.facet,binding,identity,setup,reason));
      const auto& original=identity.pieces.front();
      const auto rc=mix(mix(original.reference[0],original.reference[1],.5),original.reference[2],.25);
      const auto ac=mix(mix(original.active[0],original.active[1],.5),original.active[2],.25);
      FaceCoverage cover;
      for(unsigned k=0;k<3;++k){const unsigned j=(k+1)%3;Edge edge{face.v[k],face.v[j]},key=edge;std::sort(key.begin(),key.end());
        auto append=[&](const ExactBary& a,const ExactBary& b,const ExactBary& x,const ExactBary& y){CoveragePiece piece;piece.facet=face.facet;
          piece.reference={rc,a,b};piece.active={ac,x,y};cover.pieces.push_back(std::move(piece));};
        if(key==shared){const auto midpoint=mix(original.reference[k],original.reference[j],.5);
          const auto parameter=mix(original.active[k],original.active[j],edge[0]<edge[1]?.25:.75);
          append(original.reference[k],midpoint,original.active[k],parameter);append(midpoint,original.reference[j],parameter,original.active[j]);}
        else append(original.reference[k],original.reference[j],original.active[k],original.active[j]);}
      std::array<EdgeTrace,3> traces;REQUIRE(fixture.geometry->CoverageTraces(cover,traces,setup,reason));
      for(unsigned k=0;k<3;++k)if(face.v[k]==shared[0]&&face.v[(k+1)%3]==shared[1])legacy.traces.emplace(shared,traces[k]);
      legacy.faces.emplace(face.v,std::move(cover));
    }
    REQUIRE(legacy.faces.size()==2);REQUIRE(legacy.traces.at(shared).pieces.size()==2);
  }
  auto state=std::make_shared<BoundaryParameterState>(fixture.geometry,points,surface,1,SIZE_MAX,0,legacy,fixture.policy,&setup);
  auto reference=PlanarReference::WithParameters(fixture.definition,state);
  std::vector<Cell> global;for(const auto& face:surface)if(face.facet<4){Cell cell;cell.id=global.size();
    for(unsigned k=0;k<3;++k)cell.v[k]=points.at(face.v[k]).node;cell.v[3]=points.at(50).node;
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
    REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);global.push_back(cell);}
  REQUIRE(global.size()==6);
  auto cell_faces=[](const Cell& c){return std::array<Face,4>{{{c.v[1].id,c.v[2].id,c.v[3].id},{c.v[0].id,c.v[3].id,c.v[2].id},
      {c.v[0].id,c.v[1].id,c.v[3].id},{c.v[0].id,c.v[2].id,c.v[1].id}}};};
  auto rows=[&](const std::vector<Cell>& cells,const std::vector<SurfaceFace>& faces){std::vector<OwnedCell> out;
    for(const auto& cell:cells){OwnedCell row;row.cell=cell;const auto boundary=cell_faces(cell);
      for(unsigned k=0;k<4;++k)for(const auto& face:faces){auto a=boundary[k],b=face.v;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());
        if(a==b)row.physical[k]={true,face.facet};}out.push_back(row);}return out;};
  Owned owned;for(auto row:rows(global,surface))if(row.cell.id%world.size==unsigned(world.rank))owned.emplace(row.cell.id,row);
  std::shared_ptr<BoundaryParameterState> frozen_source;std::vector<char> frozen_snapshot;std::vector<double> frozen_bounds;
  auto source_bounds=[&](const BoundaryParameterState& source){std::vector<double> values;auto work=RemappingBudget();
    for(const auto& row:source.Surface()){FaceCoverage cover;REQUIRE(source.ResolveCoverage(row.second,cover,work,reason));
      std::array<Point,3> chord;for(unsigned k=0;k<3;++k)chord[k]=source.Find(row.second.v[k])->node.p;
      for(const auto& p:fixture.geometry->PrepareSourcePatch(cover,chord,work).Bounds()){values.push_back(p.x);values.push_back(p.y);values.push_back(p.z);}}
    return values;};
  if(legacy_initial){frozen_source=std::make_shared<BoundaryParameterState>(*state);frozen_snapshot=frozen_source->Snapshot();frozen_bounds=source_bounds(*frozen_source);}
  if(entity_initial){state=state->WithEntityGeometry(.01,setup);reference=PlanarReference::WithParameters(fixture.definition,state);CHECK(state->Coverage().faces.empty());}
  if(legacy_initial && !entity_initial) {
    const auto frozen=state->Snapshot();BoundaryCoverage normalized;auto normalize=RemappingBudget();
    REQUIRE(RebuildWorkingSurface(*state,normalized,normalize,reason));
    const Edge shared{0,60};REQUIRE(normalized.traces.count(shared));CHECK(normalized.traces.at(shared).pieces.size()==1);
    for(const auto& face:legacy.faces)CHECK(normalized.faces.count(face.first));
    std::set<Face> affected;for(const auto& item:normalized.faces){auto key=item.first;std::sort(key.begin(),key.end());affected.insert(key);}
    std::vector<Dependency> removed;std::vector<OwnedCell> added;
    for(const auto& row:owned){bool touches=false;const auto faces=cell_faces(row.second.cell);
      for(unsigned k=0;k<4;++k)if(row.second.physical[k].present){auto key=faces[k];std::sort(key.begin(),key.end());touches|=affected.count(key);}
      if(touches){removed.push_back({row.first,row.second.version,world.rank});auto replacement=row.second;++replacement.version;added.push_back(replacement);}}
    REQUIRE(CPassiveComm::Allreduce(uint64_t(removed.size()),CPassiveComm::Op::SUM,world.comm)>=2);
    for(const auto& face:affected){uint64_t local=0;for(const auto& row:added){const auto faces=cell_faces(row.cell);
      for(unsigned k=0;k<4;++k)if(row.physical[k].present){auto key=faces[k];std::sort(key.begin(),key.end());local+=key==face;}}
      CHECK(CPassiveComm::Allreduce(local,CPassiveComm::Op::SUM,world.comm)==1);}
    Directory directory(world,true,state.get());REQUIRE(directory.Build(owned,reason));const BoundaryCoverage empty;auto publication=RemappingBudget();
    const bool committed=directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{},world.rank?empty:normalized,&publication);INFO(reason);REQUIRE(committed);
    CHECK(state->Snapshot()!=frozen);CHECK(state->Geometry().Version()==fixture.geometry->Version());
    CHECK(state->Coverage().traces.at(shared).pieces.size()==1);
    BoundaryCoverage again;auto repeat=RemappingBudget();REQUIRE(RebuildWorkingSurface(*state,again,repeat,reason));
    CHECK(again.faces.empty());CHECK(again.traces.empty());
  }
  Id moving[2]={0,60};using Measures=std::array<size_t,5>;std::map<unsigned,Measures> baseline;
  for(unsigned event=0;event<16;++event){const auto started=std::chrono::steady_clock::now();auto budget=RemappingBudget();const unsigned phase=event%4,slot=event%2;const Id old=moving[slot],next=100+event;
    auto scoped=reference->WithAtlasReservation(budget);Patch patch;
    for(const auto& cell:global)if(std::any_of(cell.v.begin(),cell.v.end(),[&](const Node& n){return n.id==old;}))patch.cells.push_back(cell);
    surface.clear();for(const auto& row:reference->Parameters()->Surface())surface.push_back(row.second);
    for(const auto& cell:patch.cells)for(const auto& boundary:cell_faces(cell))for(const auto& face:surface){auto a=boundary,b=face.v;
      std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());if(a==b)patch.surface.push_back(face);}
    const std::array<SurfaceBinding,4> target{{fixture.geometry->FacetPoint(1,.75,.125),fixture.geometry->FacetPoint(0,.5,.375),
                                               fixture.geometry->FacetPoint(3,.75,.125),fixture.geometry->FacetPoint(1,.5,.125)}};
    const auto raw=fixture.geometry->Realize(target[phase]);const auto prior=reference->Parameters()->Find(old)->node.p;
    scoped->ExportParameters(patch);
    Patch fresh;const bool moved=MoveVertex(patch,old,{next,raw},1000+event*10,*scoped,fresh,reason);INFO(event);INFO(reason);REQUIRE(moved);
    REQUIRE(fresh.parameters.count(next));const auto moved_point=fresh.parameters.at(next).node.p;
    CHECK((moved_point.x!=prior.x||moved_point.y!=prior.y||moved_point.z!=prior.z));
    for(const auto& cell:fresh.cells)CHECK(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
    Directory directory(world,true,reference->Parameters());REQUIRE(directory.Build(owned,reason));
    const std::vector<Feature> features=world.rank?std::vector<Feature>{}:std::vector<Feature>{{{old,0,0},1}};
    const auto dependencies=directory.Lookup(features);INFO(dependencies.reason);REQUIRE(dependencies.valid);
    const auto imported=directory.Fetch(dependencies,owned);INFO(imported.reason);REQUIRE(imported.valid);
    const bool reserved=directory.Reserve(imported,world.rank==0,reason);INFO(reason);if(!world.rank)REQUIRE(reserved);
    if(!world.rank){std::set<Id> expected,actual;for(const auto& c:patch.cells)expected.insert(c.id);for(const auto& row:imported.cells)actual.insert(row.cell.id);CHECK(actual==expected);}
    const auto next_surface=fixture.Moved(old,next,surface);
    std::vector<std::vector<Dependency>> removals(world.size);
    std::vector<std::vector<OwnedCell>> additions(world.size);
    if(!world.rank) {
      std::map<Id,int> owners;for(const auto& dep:dependencies.cells){removals[dep.owner].push_back(dep);owners.emplace(dep.cell,dep.owner);}
      const auto replacement=rows(fresh.cells,next_surface);REQUIRE(replacement.size()==patch.cells.size());
      for(size_t i=0;i<replacement.size();++i)additions[owners.at(patch.cells[i].id)].push_back(replacement[i]);
    }
    const auto removed=world.exchange(removals);const auto added=world.exchange(additions);
    const auto edits=world.rank?std::vector<BoundaryParameterEdit>{}:BoundaryParameterState::Changes(patch,fresh,reference->Parameters()->Revision());
    auto publication_budget=RemappingBudget();const BoundaryCoverage no_coverage;const bool published=directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,edits,world.rank?no_coverage:fresh.coverage,&publication_budget);INFO(reason);REQUIRE(published);
    for(const auto& cell:patch.cells)global.erase(std::remove_if(global.begin(),global.end(),[&](const Cell& c){return c.id==cell.id;}),global.end());
    global.insert(global.end(),fresh.cells.begin(),fresh.cells.end());CHECK(global.size()==6);
    CHECK(reference->Parameters()->Records().size()==7);CHECK(reference->Parameters()->Surface().size()==10);
    ReferenceState archive;archive.original=reference;archive.facets=fixture.facets;archive.marker_names={"body"};
    const auto encoded=EncodeReference(archive);CHECK(encoded.find(entity_initial?"SU2_NATIVE_REFERENCE_3D 9\n":"SU2_NATIVE_REFERENCE_3D 8\n")==0);
    const std::string filename="neighboring_curved_working_state.native_ref";WriteReference(filename,archive);
    auto loaded=archive;const auto before_load=EncodeReference(loaded);bool found=false;size_t load_peak=0;
    const auto load_caller=ReferenceStorageBytes(loaded);
    CHECK_FALSE(LoadReference(filename,loaded,found,reason,1,load_caller,load_peak));CHECK_FALSE(found);CHECK(EncodeReference(loaded)==before_load);
    const bool reloaded=LoadReference(filename,loaded,found,reason,ReferenceStorageLimit3D,load_caller,load_peak);INFO(reason);REQUIRE(reloaded);REQUIRE(found);
    REQUIRE(loaded.original->Parameters());CHECK(EncodeReference(loaded)==encoded);
    if(event==1) {
      const auto saved=loaded.original;const auto saved_parameters=saved->Parameters()->Snapshot();
      const auto deep_caller=ReferenceStorageBytes(loaded);
      const auto deep_ceiling=transfer_memory::Add(load_peak-load_caller,deep_caller)-1;
      size_t refused_peak=0;
      CHECK_FALSE(LoadReference(filename,loaded,found,reason,world.rank?ReferenceStorageLimit3D:deep_ceiling,deep_caller,refused_peak));
      CHECK_FALSE(found);CHECK_FALSE(reason.empty());CHECK(loaded.original==saved);CHECK(saved->Parameters()->Snapshot()==saved_parameters);
      if(!world.rank)CHECK(refused_peak>deep_ceiling);
    }
    reference=loaded.original;moving[slot]=next;
    auto source_budget=RemappingBudget();
    for(const auto& row:reference->Parameters()->Surface()) {
      std::array<Point,3> chord;for(unsigned k=0;k<3;++k)chord[k]=reference->Parameters()->Find(row.second.v[k])->node.p;
      auto source=[&](){if(entity_initial){const auto& certs=reference->Parameters()->GeometryCertificates(source_budget);
          const auto& certificate=certs.at(reference->Parameters()->Entity(row.second.facet));CHECK_FALSE(certificate.within_tolerance);
          return MappedFacetReference::SourcePatch::Tube(chord,certificate.model_to_mesh);}
        FaceCoverage source_cover;REQUIRE(reference->Parameters()->ResolveCoverage(row.second,source_cover,source_budget,reason));
        return reference->Parameters()->Geometry().PrepareSourcePatch(source_cover,chord,source_budget);}();
      const auto bounds=source.Bounds();
      for(const auto& point:chord){CHECK(point.x>=bounds[0].x);CHECK(point.x<=bounds[1].x);
        CHECK(point.y>=bounds[0].y);CHECK(point.y<=bounds[1].y);CHECK(point.z>=bounds[0].z);CHECK(point.z<=bounds[1].z);}
    }
    if(entity_initial && event==15){auto complete_budget=RemappingBudget();
      for(const auto& row:reference->Parameters()->GeometryCertificates(complete_budget,true))CHECK(row.second.within_tolerance);}
    const auto& coverage=reference->Parameters()->Coverage();if(entity_initial){CHECK(coverage.faces.empty());
      for(const auto& row:coverage.traces){auto trace=row.second;std::optional<Id> chain;
        REQUIRE(NormalizeFeatureTrace(reference->Parameters()->Geometry(),reference->Parameters()->Mobility(),trace,chain,source_budget,reason));CHECK(chain);}}
    size_t face_pieces=0,trace_pieces=0,bits=0;
    auto ratio=[&](const ExactRatio& r){for(const auto* limbs:{&r.numerator,&r.denominator})if(!limbs->empty()){
      size_t width=32*(limbs->size()-1);for(uint32_t top=limbs->back();top;top>>=1)++width;bits=std::max(bits,width);}};
    auto bary=[&](const ExactBary& p){ratio(p.u);ratio(p.v);};
    for(const auto& row:coverage.faces)for(const auto& piece:row.second.pieces){++face_pieces;for(const auto& p:piece.reference)bary(p);for(const auto& p:piece.active)bary(p);}
    for(const auto& row:coverage.traces)for(const auto& piece:row.second.pieces){++trace_pieces;bary(piece.first);bary(piece.last);ratio(piece.end);}
    const auto local_peak=std::max({budget.peak_bytes,publication_budget.peak_bytes,source_budget.peak_bytes});
    // Hash rendezvous changes rank shares as IDs change; aggregate workspace tests history growth.
    const auto aggregate_peak=CPassiveComm::Allreduce(uint64_t(local_peak),CPassiveComm::Op::SUM,world.comm);
    const Measures measures{{face_pieces,trace_pieces,bits,budget.work+publication_budget.work+source_budget.work,size_t(aggregate_peak)}};
    if(!world.rank)std::cout<<std::setprecision(17)<<"NATIVE_PUBLICATION_EVENT {\"scope\":\"private_neighbor_moves_publication_source_schema"<<(entity_initial?9:8)<<"\",\"event\":"<<event
      <<",\"legacy_initial\":"<<(legacy_initial?"true":"false")<<",\"entity_initial\":"<<(entity_initial?"true":"false")<<",\"operator_work\":"<<budget.work<<",\"operator_peak_bytes\":"<<budget.peak_bytes
      <<",\"publication_work\":"<<publication_budget.work<<",\"publication_peak_bytes\":"<<publication_budget.peak_bytes
      <<",\"source_envelope_work\":"<<source_budget.work<<",\"source_envelope_peak_bytes\":"<<source_budget.peak_bytes
      <<",\"arithmetic_work_excludes_decode\":"<<measures[3]<<",\"face_pieces\":"<<face_pieces<<",\"trace_pieces\":"<<trace_pieces
      <<",\"maximum_integer_bits\":"<<bits<<",\"payload_bytes\":"<<reference->Parameters()->PayloadBytes()
      <<",\"resident_bytes\":"<<reference->Parameters()->ResidentBytes()<<",\"ascii_bytes\":"<<encoded.size()
      <<",\"archive_load_peak_bytes\":"<<load_peak
      <<",\"aggregate_admitted_peak_bytes\":"<<aggregate_peak
      <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()
      <<",\"lifetime_process_peak_rss_bytes\":"<<ProcessPeakRSS3D()<<"}\n";
    if(event>=4){auto it=baseline.find(phase);if(it==baseline.end())baseline.emplace(phase,measures);else for(unsigned k=0;k<measures.size();++k)CHECK(measures[k]<=it->second[k]);}
  }
  if(frozen_source){CHECK(frozen_source->Snapshot()==frozen_snapshot);CHECK(source_bounds(*frozen_source)==frozen_bounds);}
}

TEST_CASE("Legacy shared trace normalization publishes both owners and is idempotent", "[NativeCurvedAtlas3D][NativeWorkingSurfaceNormalization3D]") {
  World world;const unsigned variant=GENERATE(0u,1u,2u);INFO(variant);RemappingFixture fixture(false);auto budget=RemappingBudget();std::string reason;
  struct RestoreAdmission {ShapeAdmission old=GetShapeAdmission();~RestoreAdmission(){SetShapeAdmission(old);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  const Edge shared=variant==2?Edge{10,20}:Edge{0,20};BoundaryCoverage legacy;
  if(variant!=1) {
    auto mix=[&](const ExactBary& a,const ExactBary& b,double t){ExactBary out;ExactRatio left,right;
      REQUIRE(ExactBinary64(1-t,left,budget,reason));REQUIRE(ExactBinary64(t,right,budget,reason));
      auto component=[&](const ExactRatio& x,const ExactRatio& y){ExactRatio u,v,r;REQUIRE(ExactMultiply(x,left,u,budget,reason));
        REQUIRE(ExactMultiply(y,right,v,budget,reason));REQUIRE(ExactAdd(u,v,r,budget,reason));return r;};
      out.u=component(a.u,b.u);out.v=component(a.v,b.v);return out;};
    for(auto face:fixture.surface)if(std::find(face.v.begin(),face.v.end(),shared[0])!=face.v.end()&&
                                     std::find(face.v.begin(),face.v.end(),shared[1])!=face.v.end()) {
      std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());
      std::array<SurfaceBinding,3> bindings;for(unsigned k=0;k<3;++k)bindings[k]=fixture.points.at(face.v[k]).binding;
      FaceCoverage identity;REQUIRE(fixture.geometry->ResolveSingleChart(face.facet,bindings,identity,budget,reason));
      const auto& original=identity.pieces.front();ExactBary reference_center=original.reference[0],active_center=original.active[0];
      // Binary-exact interior fan center: weights 3/8, 3/8, 1/4.
      reference_center=mix(mix(original.reference[0],original.reference[1],.5),original.reference[2],.25);
      active_center=mix(mix(original.active[0],original.active[1],.5),original.active[2],.25);
      FaceCoverage cover;
      for(unsigned k=0;k<3;++k){const unsigned j=(k+1)%3;Edge edge{face.v[k],face.v[j]},key=edge;std::sort(key.begin(),key.end());
        auto append=[&](const ExactBary& a,const ExactBary& b,const ExactBary& x,const ExactBary& y){CoveragePiece piece;piece.facet=face.facet;
          piece.reference={reference_center,a,b};piece.active={active_center,x,y};cover.pieces.push_back(std::move(piece));};
        if(key==shared){const auto midpoint=mix(original.reference[k],original.reference[j],.5);
          const auto parameter=mix(original.active[k],original.active[j],edge[0]<edge[1]?.25:.75);
          append(original.reference[k],midpoint,original.active[k],parameter);append(midpoint,original.reference[j],parameter,original.active[j]);}
        else append(original.reference[k],original.reference[j],original.active[k],original.active[j]);}
      std::array<EdgeTrace,3> traces;REQUIRE(fixture.geometry->CoverageTraces(cover,traces,budget,reason));
      for(unsigned k=0;k<3;++k)if(face.v[k]==shared[0]&&face.v[(k+1)%3]==shared[1])legacy.traces.emplace(shared,traces[k]);
      Face key=face.v;std::rotate(key.begin(),std::min_element(key.begin(),key.end()),key.end());REQUIRE(key==face.v);
      legacy.faces.emplace(key,std::move(cover));
    }
    REQUIRE(legacy.faces.size()==2);REQUIRE(legacy.traces.at(shared).pieces.size()==2);
    for(const auto& face:legacy.faces)REQUIRE(face.second.pieces.size()==4);
  }
  if(variant==1) {
    std::vector<CoveragePiece> region;std::vector<SurfaceFace> selected;
    std::map<Id,SurfaceBinding> bindings;std::map<Edge,EdgeTrace> traces;
    for(auto face:fixture.surface)if(std::find(face.v.begin(),face.v.end(),shared[0])!=face.v.end()&&
                                     std::find(face.v.begin(),face.v.end(),shared[1])!=face.v.end()) {
      std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());selected.push_back(face);
      std::array<SurfaceBinding,3> local;for(unsigned k=0;k<3;++k){local[k]=fixture.points.at(face.v[k]).binding;bindings.emplace(face.v[k],local[k]);}
      FaceCoverage cover;REQUIRE(fixture.geometry->ResolveSingleChart(face.facet,local,cover,budget,reason));
      region.insert(region.end(),cover.pieces.begin(),cover.pieces.end());
      for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());if(traces.count(edge))continue;
        EdgeTrace trace;REQUIRE(fixture.state->ResolveTrace(edge,face,trace,budget,reason));traces.emplace(edge,std::move(trace));}
    }
    REQUIRE(selected.size()==2);ExactBary first,last,middle;REQUIRE(fixture.geometry->BindingParameters(0,bindings.at(0),first,budget,reason));
    REQUIRE(fixture.geometry->BindingParameters(0,bindings.at(20),last,budget,reason));
    REQUIRE(ExactBinary64(.25,middle.u,budget,reason));REQUIRE(ExactBinary64(.25,middle.v,budget,reason));
    ExactRatio half,one;REQUIRE(ExactBinary64(.5,half,budget,reason));REQUIRE(ExactInteger(1,one,budget,reason));
    traces.at(shared).pieces={{0,first,middle,half},{0,middle,last,one}};
    const bool built=RebuildTraceCoverage(*fixture.geometry,region,selected,bindings,traces,legacy.faces,budget,reason);INFO(reason);REQUIRE(built);
    legacy.traces=std::move(traces);REQUIRE(legacy.traces.at(shared).pieces.size()==2);
  }
  auto state=std::make_shared<BoundaryParameterState>(fixture.geometry,fixture.points,fixture.surface,1,SIZE_MAX,0,legacy,fixture.policy,&budget);
  auto check_indices=[&](const BoundaryParameterState& current){
    std::map<Edge,std::set<Face>> edges;std::map<Id,std::set<Face>> support;auto proof=RemappingBudget();
    for(const auto& row:current.Surface()){const auto& face=row.second;FaceCoverage cover;
      REQUIRE(current.ResolveCoverage(face,cover,proof,reason));
      for(const auto& piece:cover.pieces)support[current.Geometry().Canonical(piece.facet)].insert(row.first);
      for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());edges[edge].insert(row.first);}}
    for(const auto& row:edges){std::set<Face> found;for(const auto* face:current.IncidentFaces(row.first))if(face){auto key=face->v;std::sort(key.begin(),key.end());found.insert(key);}CHECK(found==row.second);}
    for(const auto& facet:fixture.facets){std::set<Face> found;const auto range=current.FacesOnOriginalFacet(facet.id);
      for(auto it=range.first;it!=range.second;++it)found.insert(it->second);CHECK(found==support[current.Geometry().Canonical(facet.id)]);}
  };check_indices(*state);
  const auto before=state->Snapshot();auto incident=state->IncidentFaces(shared);REQUIRE(incident[0]);REQUIRE(incident[1]);
  for(const auto* face:incident){const auto range=state->FacesOnOriginalFacet(face->facet);const auto key=[&](){auto f=face->v;std::sort(f.begin(),f.end());return f;}();
    CHECK(std::any_of(range.first,range.second,[&](const auto& entry){return entry.second==key;}));}
  if(variant==2)REQUIRE(fixture.policy.features.count(shared));
  auto refused=legacy;const auto refused_before=EncodeBoundaryAtlas(refused,fixture.policy);
  auto no_work=RemappingBudget();no_work.maximum_work=0;
  CHECK_FALSE(RebuildWorkingSurface(*state,refused,no_work,reason));CHECK(EncodeBoundaryAtlas(refused,fixture.policy)==refused_before);
  auto no_memory=RemappingBudget();no_memory.maximum_bytes=0;
  CHECK_FALSE(RebuildWorkingSurface(*state,refused,no_memory,reason));CHECK(EncodeBoundaryAtlas(refused,fixture.policy)==refused_before);
  BoundaryCoverage normalized;auto work=RemappingBudget();const bool rebuilt=RebuildWorkingSurface(*state,normalized,work,reason);INFO(reason);REQUIRE(rebuilt);
  REQUIRE(normalized.traces.count(shared));CHECK(normalized.traces.at(shared).pieces.size()==1);
  for(const auto* face:incident){REQUIRE(normalized.faces.count(face->v));CHECK(normalized.faces.at(face->v).pieces.size()==1);}
  CHECK(state->Snapshot()==before);
  std::array<SurfaceFace,2> old_incident{{*incident[0],*incident[1]}};
  // Two incident cone cells own the changed faces; all owner rows must participate.
  auto cell_faces=[](const Cell& c){return std::array<Face,4>{{{c.v[1].id,c.v[2].id,c.v[3].id},{c.v[0].id,c.v[3].id,c.v[2].id},
      {c.v[0].id,c.v[1].id,c.v[3].id},{c.v[0].id,c.v[2].id,c.v[1].id}}};};
  Owned owned;for(const auto& face:fixture.patch.surface){Cell cell;cell.id=face.facet;
    for(unsigned k=0;k<3;++k)cell.v[k]=fixture.points.at(face.v[k]).node;cell.v[3]=fixture.points.at(50).node;
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);
    OwnedCell row;row.cell=cell;const auto faces=cell_faces(cell);
    for(unsigned k=0;k<4;++k)for(const auto& physical:fixture.surface){auto a=faces[k],b=physical.v;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());if(a==b)row.physical[k]={true,physical.facet};}
    if(cell.id%world.size==unsigned(world.rank))REQUIRE(owned.emplace(cell.id,row).second);}
  const auto frozen_source_state=std::make_shared<BoundaryParameterState>(*state);
  const auto frozen_reference=frozen_source_state->Snapshot();
  auto original_bounds=[&](){std::vector<double> values;auto source_work=RemappingBudget();
    for(const auto& row:frozen_source_state->Surface()){FaceCoverage cover;REQUIRE(frozen_source_state->ResolveCoverage(row.second,cover,source_work,reason));
      std::array<Point,3> chord;for(unsigned k=0;k<3;++k)chord[k]=frozen_source_state->Find(row.second.v[k])->node.p;
      const auto bounds=fixture.geometry->PrepareSourcePatch(cover,chord,source_work).Bounds();
      for(const auto& point:bounds){values.push_back(point.x);values.push_back(point.y);values.push_back(point.z);}}
    return values;};
  const auto frozen_bounds=original_bounds();
  Directory directory(world,true,state.get());REQUIRE(directory.Build(owned,reason));auto reject=RemappingBudget();
  const BoundaryCoverage empty;const auto& delta=world.rank?empty:normalized;
  CHECK_FALSE(directory.Publish(owned,{}, {},true,reason,SIZE_MAX,0,{},delta,&reject));CHECK(state->Snapshot()==before);
  std::set<Face> affected;for(const auto& row:normalized.faces){auto key=row.first;std::sort(key.begin(),key.end());affected.insert(key);}
  std::vector<Dependency> removed;std::vector<OwnedCell> added;
  for(const auto& row:owned){bool changed=false;const auto faces=cell_faces(row.second.cell);
    for(unsigned k=0;k<4;++k)if(row.second.physical[k].present){auto key=faces[k];std::sort(key.begin(),key.end());changed|=affected.count(key);}
    if(changed){removed.push_back({row.first,row.second.version,world.rank});auto next=row.second;++next.version;added.push_back(next);}}
  REQUIRE(CPassiveComm::Allreduce(uint64_t(removed.size()),CPassiveComm::Op::SUM,world.comm)>=2);auto partial=normalized;partial.faces.erase(old_incident[0].v);auto partial_budget=RemappingBudget();
  CHECK_FALSE(directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{},world.rank?empty:partial,&partial_budget));
  CHECK(state->Snapshot()==before);for(const auto& row:removed)CHECK(owned.at(row.cell).version==row.version);
  auto publish=RemappingBudget();const bool done=directory.Publish(owned,removed,added,true,reason,SIZE_MAX,0,{},delta,&publish);INFO(reason);REQUIRE(done);
  REQUIRE(state->Coverage().traces.count(shared));CHECK(state->Coverage().traces.at(shared).pieces.size()==1);
  for(const auto& face:old_incident){REQUIRE(state->Coverage().faces.count(face.v));CHECK(state->Coverage().faces.at(face.v).pieces.size()==1);}
  check_indices(*state);const auto after=state->Snapshot();BoundaryCoverage again;auto repeat=RemappingBudget();const bool rebuilt_again=RebuildWorkingSurface(*state,again,repeat,reason);INFO(reason);REQUIRE(rebuilt_again);
  CHECK(again.faces.empty());CHECK(again.traces.empty());CHECK(state->Snapshot()==after);
  const auto now=state->IncidentFaces(shared);REQUIRE(now[0]);REQUIRE(now[1]);
  for(const auto* face:now){const auto range=state->FacesOnOriginalFacet(face->facet);auto key=face->v;std::sort(key.begin(),key.end());
    CHECK(std::any_of(range.first,range.second,[&](const auto& entry){return entry.second==key;}));}
  CHECK(frozen_source_state->Snapshot()==frozen_reference);CHECK(original_bounds()==frozen_bounds);
  ReferenceState archive;archive.original=PlanarReference::WithParameters(fixture.definition,state);
  archive.facets=fixture.facets;archive.marker_names={"body"};
  const auto encoded=EncodeReference(archive);REQUIRE(encoded.find("SU2_NATIVE_REFERENCE_3D 8\n")==0);
  auto decoded=DecodeReference(encoded);REQUIRE(decoded.original->Parameters());check_indices(*decoded.original->Parameters());CHECK(EncodeReference(decoded)==encoded);
  // Rebuild a complete replicated coordinate star; owned rows are still partitioned.
  Patch old;for(const auto& face:fixture.patch.surface){Cell cell;cell.id=face.facet;
    for(unsigned k=0;k<3;++k)cell.v[k]=fixture.points.at(face.v[k]).node;cell.v[3]=fixture.points.at(50).node;
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0)std::swap(cell.v[0],cell.v[1]);old.cells.push_back(cell);}
  for(const auto& cell:old.cells)for(const auto& f:cell_faces(cell))for(const auto& physical:fixture.surface){auto a=f,b=physical.v;
    std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());if(a==b)old.surface.push_back(physical);}
  auto movement=RemappingBudget();auto scoped=decoded.original->WithAtlasReservation(movement);scoped->ExportParameters(old);
  Patch fresh;const bool moved=MoveVertex(old,0,{100,{.6,.5,-.01}},1000,*scoped,fresh,reason);INFO(reason);REQUIRE(moved);
  for(const auto& c:fresh.cells)CHECK(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
  Directory next_directory(world,true,decoded.original->Parameters());REQUIRE(next_directory.Build(owned,reason));
  const std::vector<Feature> requests=world.rank?std::vector<Feature>{}:std::vector<Feature>{{{0,0,0},1}};
  const auto dependencies=next_directory.Lookup(requests);REQUIRE(dependencies.valid);
  const auto fetched=next_directory.Fetch(dependencies,owned);REQUIRE(fetched.valid);
  const bool reserved=next_directory.Reserve(fetched,world.rank==0,reason);if(!world.rank)REQUIRE(reserved);
  std::vector<std::vector<Dependency>> owner_removed(world.size);std::vector<std::vector<OwnedCell>> owner_added(world.size);
  if(!world.rank){std::map<Id,int> owners;for(const auto& d:dependencies.cells){owner_removed[d.owner].push_back(d);owners.emplace(d.cell,d.owner);}
    REQUIRE(fresh.cells.size()==old.cells.size());for(size_t i=0;i<fresh.cells.size();++i){OwnedCell row;row.cell=fresh.cells[i];const auto faces=cell_faces(row.cell);
      for(unsigned k=0;k<4;++k)for(const auto& physical:fresh.surface){auto a=faces[k],b=physical.v;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());if(a==b)row.physical[k]={true,physical.facet};}
      owner_added[owners.at(old.cells[i].id)].push_back(row);}}
  const auto removed_move=world.exchange(owner_removed);const auto added_move=world.exchange(owner_added);
  const auto edits=world.rank?std::vector<BoundaryParameterEdit>{}:BoundaryParameterState::Changes(old,fresh,decoded.original->Parameters()->Revision());
  const BoundaryCoverage no_delta;auto publish_move=RemappingBudget();
  const bool continued=next_directory.Publish(owned,removed_move,added_move,true,reason,SIZE_MAX,0,edits,world.rank?no_delta:fresh.coverage,&publish_move);
  INFO(reason);REQUIRE(continued);CHECK(frozen_source_state->Snapshot()==frozen_reference);CHECK(original_bounds()==frozen_bounds);
}

TEST_CASE("Curved collapse removes smooth and curve samples across original seams", "[NativeCurvedAtlasRemap3D][NativeCurvedAtlas3D]") {
  const bool crease=GENERATE(false,true);const bool unpublished=GENERATE(false,true);RemappingFixture fixture(crease);
  auto budget=RemappingBudget();auto reference=fixture.reference->WithAtlasReservation(budget);
  Patch old;const Node apex=crease?Node{90,{.5,.5,.5}}:fixture.points.at(50).node;
  for(const auto& f:fixture.facets){
    if(!crease&&f.id>=4)continue;
    if(crease&&std::none_of(f.v.begin(),f.v.end(),[](Node n){return n.id==0;}))continue;
    auto v=std::array<Node,4>{f.v[0],f.v[1],f.v[2],apex};
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);
    old.cells.push_back({Id(old.cells.size()),v});
  }
  old.surface=fixture.patch.surface;
  const Id original=crease?10:20,retained=unpublished?original+100:original;
  if(unpublished){auto point=fixture.points.at(original);point.node.id=retained;old.parameters.emplace(retained,point);
    for(auto& c:old.cells)for(auto& n:c.v)if(n.id==original)n.id=retained;
    for(auto& f:old.surface)for(auto& id:f.v)if(id==original)id=retained;
  }
  CHECK_FALSE(fixture.geometry->Supports(crease?1:2,fixture.geometry->Vertex(original)));
  Patch fresh;std::string reason;const bool ok=CollapseVertex(old,0,retained,100,*reference,fresh,reason);
  INFO(reason);REQUIRE(ok);CHECK(fresh.cells.size()==(crease?4:2));
  CHECK_FALSE(fresh.coverage.faces.empty());
  for(const auto& c:fresh.cells){CHECK(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
    for(const auto& n:c.v)CHECK(n.id!=0);}
  CHECK(fixture.state->Find(0)!=nullptr); // Private proposal cannot mutate accepted associations.
}

TEST_CASE("Feature trace normalization validates intervals and preserves failed outputs", "[NativeCurvedAtlas3D]") {
  RemappingFixture crease(true);auto budget=RemappingBudget();std::string reason;const Edge feature{0,10};
  const auto incident=crease.state->IncidentFaces(feature);REQUIRE(incident[0]);
  EdgeTrace genuine;REQUIRE(crease.state->ResolveTrace(feature,*incident[0],genuine,budget,reason));
  std::optional<Id> chain;REQUIRE(NormalizeFeatureTrace(*crease.geometry,crease.policy,genuine,chain,budget,reason));REQUIRE(chain);
  const auto canonical=genuine;const auto saved_chain=chain;
  auto generous=RemappingBudget();generous.maximum_integer_bits=size_t(1)<<34;generous.maximum_bytes=size_t(16)<<20;
  auto tiny=canonical;std::optional<Id> tiny_chain;REQUIRE(NormalizeFeatureTrace(*crease.geometry,crease.policy,tiny,tiny_chain,generous,reason));
  CHECK(tiny_chain==saved_chain);CHECK(generous.maximum_integer_bits==(size_t(1)<<34));CHECK(generous.peak_bytes<=generous.maximum_bytes);
  auto encode=[&](const EdgeTrace& trace){BoundaryCoverage c;c.traces.emplace(feature,trace);return EncodeBoundaryAtlas(c,crease.policy);};
  auto no_work=RemappingBudget();no_work.maximum_work=0;
  CHECK_FALSE(NormalizeFeatureTrace(*crease.geometry,crease.policy,genuine,chain,no_work,reason));
  CHECK(encode(genuine)==encode(canonical));CHECK(chain==saved_chain);
  auto malformed=canonical;REQUIRE_FALSE(malformed.pieces.empty());REQUIRE(ExactInteger(0,malformed.pieces.back().end,budget,reason));
  const auto broken=encode(malformed);CHECK_FALSE(NormalizeFeatureTrace(*crease.geometry,crease.policy,malformed,chain,budget,reason));
  CHECK(encode(malformed)==broken);CHECK(chain==saved_chain);
  RemappingFixture smooth(false);const Edge smooth_edge{0,20};const auto smooth_incident=smooth.state->IncidentFaces(smooth_edge);REQUIRE(smooth_incident[0]);
  EdgeTrace plain;REQUIRE(smooth.state->ResolveTrace(smooth_edge,*smooth_incident[0],plain,budget,reason));
  BoundaryCoverage before;before.traces.emplace(smooth_edge,plain);const auto encoded=EncodeBoundaryAtlas(before,smooth.policy);
  chain=saved_chain;REQUIRE(NormalizeFeatureTrace(*smooth.geometry,smooth.policy,plain,chain,budget,reason));CHECK_FALSE(chain);
  BoundaryCoverage after;after.traces.emplace(smooth_edge,plain);CHECK(EncodeBoundaryAtlas(after,smooth.policy)==encoded);
}

TEST_CASE("Entity feature reconstruction accepts empty interior cavities only", "[NativeCurvedAtlas3D]") {
  RemappingFixture fixture(false);auto budget=RemappingBudget();std::string reason;
  auto entity=fixture.state->WithEntityGeometry(.01,budget);auto reference=PlanarReference::WithParameters(fixture.definition,entity);
  Patch interior;std::vector<SurfaceFace> no_faces;BoundaryPoints no_points;std::map<Edge,EdgeTrace> result;
  REQUIRE(RebuildFeatureTraces(interior,no_faces,no_points,*reference,result,budget,reason));CHECK(result.empty());
  Patch physical;physical.surface.push_back(fixture.surface.front());
  EdgeTrace sentinel;ExactRatio one;REQUIRE(ExactInteger(1,one,budget,reason));sentinel.pieces.push_back({0,{},{},one});
  const Edge key{10,20};result.emplace(key,sentinel);const auto saved=result.size();
  CHECK_FALSE(RebuildFeatureTraces(physical,no_faces,no_points,*reference,result,budget,reason));
  CHECK(result.size()==saved);REQUIRE(result.count(key));REQUIRE(result.at(key).pieces.size()==1);
  CHECK(result.at(key).pieces[0].facet==sentinel.pieces[0].facet);CHECK(result.at(key).pieces[0].end.numerator==one.numerator);
  CHECK(result.at(key).pieces[0].end.denominator==one.denominator);
}

TEST_CASE("Entity feature reconstruction handles ordinary and genuine routes", "[NativeCurvedAtlas3D]") {
  const bool crease=GENERATE(false,true);RemappingFixture fixture(crease);auto budget=RemappingBudget();std::string reason;
  auto entity=fixture.state->WithEntityGeometry(.01,budget);auto reference=PlanarReference::WithParameters(fixture.definition,entity);
  Patch old=fixture.patch;old.parameters=fixture.points;
  if(crease)for(const Edge edge:{Edge{0,10},Edge{0,20}}) {
    const auto incident=fixture.state->IncidentFaces(edge);REQUIRE(incident[0]);EdgeTrace trace;
    REQUIRE(fixture.state->ResolveTrace(edge,*incident[0],trace,budget,reason));old.coverage.traces.emplace(edge,std::move(trace));
  }
  auto faces=old.surface;auto points=fixture.points;
  if(crease){faces=fixture.Moved(0,100,faces);points.erase(0);
    points.emplace(100,BoundaryPoint{{100,{.65,0,0}},fixture.geometry->EdgePoint({0,20},.3)});}
  std::map<Edge,EdgeTrace> output;INFO(reason);
  REQUIRE(RebuildFeatureTraces(old,faces,points,*reference,output,budget,reason));
  CHECK(output.size()==(crease?2:0));
  if(crease)for(const Edge edge:{Edge{10,100},Edge{20,100}}){REQUIRE(output.count(edge));
    auto trace=output.at(edge);std::optional<Id> chain;
    REQUIRE(NormalizeFeatureTrace(*fixture.geometry,fixture.policy,trace,chain,budget,reason));CHECK(chain);}
  auto invalid=points;invalid.at(crease?100:0).binding.version++;
  const auto saved=output.size();CHECK_FALSE(RebuildFeatureTraces(old,faces,invalid,*reference,output,budget,reason));CHECK(output.size()==saved);
}

TEST_CASE("Entity collapse precheck agrees with the complete curved cavity", "[NativeCurvedAtlas3D][NativeCollapseFilter3D]") {
  RemappingFixture fixture(false);auto budget=RemappingBudget();std::string reason;
  auto state=fixture.state->WithEntityGeometry(.01,budget);const auto snapshot=state->Snapshot();
  auto reference=PlanarReference::WithParameters(fixture.definition,state);auto scoped=reference->WithAtlasReservation(budget);
  struct Restore {ShapeAdmission old=GetShapeAdmission();~Restore(){SetShapeAdmission(old);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);Patch old;
  for(const auto& f:fixture.facets)if(f.id<4){std::array<Node,4> nodes{{f.v[0],f.v[1],f.v[2],fixture.points.at(50).node}};
    if(Orientation(nodes[0].p,nodes[1].p,nodes[2].p,nodes[3].p)<0)std::swap(nodes[0],nodes[1]);
    old.cells.push_back({Id(old.cells.size()),nodes});}
  old.surface=fixture.surface;scoped->ExportParameters(old);
  Patch fresh;const bool collapsed=CollapseVertex(old,0,10,100,*scoped,fresh,reason);INFO(reason);REQUIRE(collapsed);
  REQUIRE(fresh.cells.size()==2);for(const auto& c:fresh.cells)CHECK(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
  auto current=scoped->PrivateParameters(fresh);double error=0;
  for(const auto& face:fresh.surface){std::array<Point,3> p;
    for(unsigned k=0;k<3;++k)p[k]=fixture.points.at(face.v[k]).node.p;
    error=std::max(error,current->SurfaceErrorForValidatedFace(face.facet,p));}
  CHECK(error<.01);CHECK(error>1e-4);
  bool tight_veto=false;size_t survivors=0;
  for(const auto& c:old.cells)if(std::none_of(c.v.begin(),c.v.end(),[](const Node& n){return n.id==10;})) {
    ++survivors;OwnedCell row;row.cell=c;const auto& v=c.v;
    const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
    for(unsigned k=0;k<4;++k)for(const auto& physical:fixture.surface){auto a=faces[k],b=physical.v;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());
      if(a==b)row.physical[k]={true,physical.facet};}
    CHECK_FALSE(CollapseConstraintViolation(row,0,fixture.points.at(10).node,*scoped,.01,{}));
    tight_veto|=CollapseConstraintViolation(row,0,fixture.points.at(10).node,*scoped,1e-4,{});
  }
  CHECK(survivors==2);CHECK(tight_veto);CHECK(state->Snapshot()==snapshot);
}
