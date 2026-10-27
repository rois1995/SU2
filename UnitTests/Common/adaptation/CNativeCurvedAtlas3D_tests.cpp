/*!
 * \file CNativeCurvedAtlas3D_tests.cpp
 * \brief Connected curved correspondence across original vertices and physical curve samples.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../Common/include/adaptation/CNativeDistributed3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
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
  World world;RemappingFixture fixture(false);auto setup=RemappingBudget();std::string reason;
  struct Restore {ShapeAdmission old=GetShapeAdmission();~Restore(){SetShapeAdmission(old);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  const auto qb=fixture.geometry->EdgePoint({0,20},.5);auto points=fixture.points;
  points.emplace(60,BoundaryPoint{{60,fixture.geometry->Realize(qb)},qb});
  auto surface=fixture.surface;surface.erase(surface.begin(),surface.begin()+2);
  surface.insert(surface.begin(),{{{10,0,60},0},{{10,60,20},0},{{20,60,30},1},{{60,0,30},1}});
  auto state=std::make_shared<BoundaryParameterState>(fixture.geometry,points,surface,1,SIZE_MAX,0,BoundaryCoverage{},fixture.policy,&setup);
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
    const auto encoded=EncodeReference(archive);CHECK(encoded.find("SU2_NATIVE_REFERENCE_3D 8\n")==0);
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
      FaceCoverage source_cover;REQUIRE(reference->Parameters()->ResolveCoverage(row.second,source_cover,source_budget,reason));
      std::array<Point,3> chord;for(unsigned k=0;k<3;++k)chord[k]=reference->Parameters()->Find(row.second.v[k])->node.p;
      const auto source=reference->Parameters()->Geometry().PrepareSourcePatch(source_cover,chord,source_budget);
      const auto bounds=source.Bounds();
      for(const auto& point:chord){CHECK(point.x>=bounds[0].x);CHECK(point.x<=bounds[1].x);
        CHECK(point.y>=bounds[0].y);CHECK(point.y<=bounds[1].y);CHECK(point.z>=bounds[0].z);CHECK(point.z<=bounds[1].z);}
    }
    const auto& coverage=reference->Parameters()->Coverage();size_t face_pieces=0,trace_pieces=0,bits=0;
    auto ratio=[&](const ExactRatio& r){for(const auto* limbs:{&r.numerator,&r.denominator})if(!limbs->empty()){
      size_t width=32*(limbs->size()-1);for(uint32_t top=limbs->back();top;top>>=1)++width;bits=std::max(bits,width);}};
    auto bary=[&](const ExactBary& p){ratio(p.u);ratio(p.v);};
    for(const auto& row:coverage.faces)for(const auto& piece:row.second.pieces){++face_pieces;for(const auto& p:piece.reference)bary(p);for(const auto& p:piece.active)bary(p);}
    for(const auto& row:coverage.traces)for(const auto& piece:row.second.pieces){++trace_pieces;bary(piece.first);bary(piece.last);ratio(piece.end);}
    const auto local_peak=std::max({budget.peak_bytes,publication_budget.peak_bytes,source_budget.peak_bytes});
    // Hash rendezvous changes rank shares as IDs change; aggregate workspace tests history growth.
    const auto aggregate_peak=CPassiveComm::Allreduce(uint64_t(local_peak),CPassiveComm::Op::SUM,world.comm);
    const Measures measures{{face_pieces,trace_pieces,bits,budget.work+publication_budget.work+source_budget.work,size_t(aggregate_peak)}};
    if(!world.rank)std::cout<<std::setprecision(17)<<"NATIVE_PUBLICATION_EVENT {\"scope\":\"private_neighbor_moves_publication_source_schema8\",\"event\":"<<event
      <<",\"operator_work\":"<<budget.work<<",\"operator_peak_bytes\":"<<budget.peak_bytes
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
}
