/*!
 * \file CNativeCurvedAtlas3D_tests.cpp
 * \brief Connected curved correspondence across original vertices and physical curve samples.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include <algorithm>
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
  const auto snapshot=fixture.state->Snapshot();Patch first;
  const bool first_move=MoveVertex(old,0,{100,{.6,.5,-.01}},1000,*reference,first,reason);INFO(reason);REQUIRE(first_move);
  REQUIRE(first.cells.size()==4);CHECK(first.coverage.faces.size()==4);
  for(const auto& cell:first.cells)CHECK(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
  CHECK(fixture.state->Snapshot()==snapshot);
  auto points=fixture.points;points.erase(0);points.insert_or_assign(100,first.parameters.at(100));
  auto all=fixture.Moved(0,100,fixture.surface);
  auto accepted=std::make_shared<BoundaryParameterState>(fixture.geometry,points,all,2,SIZE_MAX,0,first.coverage,fixture.policy,&budget);
  auto next_reference=PlanarReference::WithParameters(fixture.definition,accepted)->WithAtlasReservation(budget);
  const auto accepted_snapshot=accepted->Snapshot();Patch second;
  const bool second_move=MoveVertex(first,100,{101,{.38,.58,-.01}},2000,*next_reference,second,reason);INFO(reason);REQUIRE(second_move);
  CHECK(first.parameters.at(100).binding.dimension==2);CHECK(second.parameters.at(101).binding.dimension==2);
  CHECK(first.parameters.at(100).binding.feature!=second.parameters.at(101).binding.feature);
  REQUIRE(second.cells.size()==4);CHECK(second.coverage.faces.size()==4);
  for(const auto& cell:second.cells)CHECK(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
  CHECK(accepted->Snapshot()==accepted_snapshot);CHECK(fixture.state->Snapshot()==snapshot);
}

TEST_CASE("Fresh curved working patches forget interior history over repeated accepted events", "[NativeCurvedAtlasRemap3D][NativeCurvedAtlas3D]") {
  RemappingFixture fixture(false);auto accepted=fixture.state;auto parameters=fixture.points;
  auto surface=fixture.surface,local=fixture.patch.surface;BoundaryCoverage previous;
  std::map<unsigned,std::vector<char>> repeated;Id moving=0;
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
