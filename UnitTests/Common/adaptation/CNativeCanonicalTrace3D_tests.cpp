/*!
 * \file CNativeCanonicalTrace3D_tests.cpp
 * \brief Immutable original-chart paths, independent of working mesh history.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeCanonicalTrace3D.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include <algorithm>
using namespace SU2Native3D;
using namespace SU2NativeGeometry;
namespace {
AtlasBudget TraceBudget(){return {size_t(1)<<27,size_t(1)<<26,4096};}
std::vector<uint64_t> TraceWords(const EdgeTrace& trace) {
  auto budget=TraceBudget();std::string reason;std::vector<uint64_t> words{uint64_t(trace.pieces.size())};
  for(const auto& piece:trace.pieces){words.push_back(piece.facet);
    for(const auto* value:{&piece.first.u,&piece.first.v,&piece.last.u,&piece.last.v,&piece.end}){
      std::vector<uint64_t> encoded;REQUIRE(ExactEncode(*value,encoded,budget,reason));words.insert(words.end(),encoded.begin(),encoded.end());}}
  return words;
}
EdgeTrace ReverseTrace(const EdgeTrace& trace) {
  EdgeTrace result;auto budget=TraceBudget();std::string reason;ExactRatio zero,one;
  REQUIRE(ExactInteger(0,zero,budget,reason));REQUIRE(ExactInteger(1,one,budget,reason));
  for(size_t i=trace.pieces.size();i;--i){const auto& p=trace.pieces[i-1];ExactRatio end;
    REQUIRE(ExactSubtract(one,i>1?trace.pieces[i-2].end:zero,end,budget,reason));result.pieces.push_back({p.facet,p.last,p.first,end});}
  return result;
}
struct TraceFixture {
  std::shared_ptr<PlanarReference> original;
  std::shared_ptr<MappedFacetReference> geometry;
  MobilityPolicy policy;
  TraceFixture(std::vector<Facet> facets,double angle=45,bool cubic=false) {
    original=std::make_shared<PlanarReference>(facets);std::map<Id,double> errors;MappedFacetReference::CurvedNets nets;
    const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,1,2},{0,2,1},{1,0,2},{2,0,1},{1,1,1}};
    for(const auto& f:facets){errors.emplace(f.id,1e-12);if(cubic){auto& net=nets[f.id];
        for(unsigned q=0;q<10;++q){long double x=0,y=0,z=0;for(unsigned k=0;k<3;++k){
            x+=powers[q][k]*(long double)f.v[k].p.x;y+=powers[q][k]*(long double)f.v[k].p.y;z+=powers[q][k]*(long double)f.v[k].p.z;}
          net[q]={double(x/3),double(y/3),double(z/3)};}if(f.id%100<4)net[9].z-=.02;}}
    geometry=std::make_shared<MappedFacetReference>(original,errors,71,SIZE_MAX,false,nets);
    auto budget=TraceBudget();std::string reason;const bool built=geometry->BuildMobilityPolicy(angle,{{0,2}},policy,budget,reason);INFO(reason);REQUIRE(built);
  }
};
std::vector<Facet> Pyramid(Id offset=0,double x=0) {
  const Node a{offset+10,{x,0,0}},b{offset+20,{x+1,0,0}},c{offset+30,{x+1,1,0}},d{offset+40,{x,1,0}},p{offset,{x+.5,.5,0}},top{offset+50,{x+.5,.5,1}};
  return {{offset+0,0,{a,p,b}},{offset+1,0,{b,p,c}},{offset+2,0,{c,p,d}},{offset+3,0,{d,p,a}},
          {offset+4,0,{a,b,top}},{offset+5,0,{b,c,top}},{offset+6,0,{c,d,top}},{offset+7,0,{d,a,top}}};
}
std::vector<Facet> LongBox() {
  std::vector<Facet> facets;std::map<Edge,unsigned> incidence;std::map<Edge,Edge> directed;std::map<Id,Node> nodes;
  for(unsigned j=0;j<2;++j)for(unsigned i=0;i<5;++i){const Id id=i+5*j;nodes.emplace(id,Node{id,{double(i),double(j),0}});nodes.emplace(id+100,Node{id+100,{double(i),double(j),1}});}
  for(unsigned i=0;i<4;++i){const Node a=nodes.at(i),b=nodes.at(i+1),c=nodes.at(i+6),d=nodes.at(i+5);
    facets.push_back({Id(facets.size()),0,{a,d,c}});facets.push_back({Id(facets.size()),0,{a,c,b}});}
  const auto bottom=facets;
  for(const auto& f:bottom){for(unsigned k=0;k<3;++k){const Edge edge{f.v[k].id,f.v[(k+1)%3].id};auto key=edge;std::sort(key.begin(),key.end());++incidence[key];directed[key]=edge;}
    facets.push_back({Id(facets.size()),0,{nodes.at(f.v[0].id+100),nodes.at(f.v[2].id+100),nodes.at(f.v[1].id+100)}});}
  for(const auto& row:incidence)if(row.second==1){const auto e=directed.at(row.first);
    facets.push_back({Id(facets.size()),0,{nodes.at(e[1]),nodes.at(e[0]),nodes.at(e[0]+100)}});
    facets.push_back({Id(facets.size()),0,{nodes.at(e[1]),nodes.at(e[0]+100),nodes.at(e[1]+100)}});}
  return facets;
}
}

TEST_CASE("Canonical surface paths cross an original vertex without mesh history", "[NativeCanonicalTrace3D]") {
  TraceFixture fixture(Pyramid(),45,true);auto& g=*fixture.geometry;const Id sheet=fixture.policy.sheets.at(0);
  const auto first=g.FacetPoint(0,.5,.25),last=g.FacetPoint(2,.5,.25);EdgeTrace forward;auto budget=TraceBudget();std::string reason;
  const bool built=CanonicalSurfaceTrace(g,fixture.policy,sheet,first,last,forward,budget,reason);INFO(reason);REQUIRE(built);
  const auto work=budget.work,peak=budget.peak_bytes;
  REQUIRE(forward.pieces.size()==2);std::vector<uint64_t> crossing;
  REQUIRE(g.ReferencePointKey(forward.pieces.front().facet,forward.pieces.front().last,crossing,budget,reason));
  const std::vector<uint64_t> original_vertex{0,0};CHECK(crossing==original_vertex);
  const auto words=TraceWords(forward);
  for(unsigned repeat=0;repeat<4;++repeat){EdgeTrace again;auto fresh=TraceBudget();
    REQUIRE(CanonicalSurfaceTrace(g,fixture.policy,sheet,first,last,again,fresh,reason));CHECK(TraceWords(again)==words);CHECK(fresh.work==work);CHECK(fresh.peak_bytes==peak);}
  EdgeTrace reverse;auto backward=TraceBudget();REQUIRE(CanonicalSurfaceTrace(g,fixture.policy,sheet,last,first,reverse,backward,reason));
  CHECK(TraceWords(reverse)==TraceWords(ReverseTrace(forward)));
}

TEST_CASE("Canonical surface paths use an original strip when endpoint stars do not overlap", "[NativeCanonicalTrace3D]") {
  TraceFixture fixture(LongBox());auto& g=*fixture.geometry;const Id sheet=fixture.policy.sheets.at(0);EdgeTrace trace;std::string reason;auto budget=TraceBudget();
  const auto first=g.FacetPoint(0,.25,.25),last=g.FacetPoint(6,.25,.25);
  const bool built=CanonicalSurfaceTrace(g,fixture.policy,sheet,first,last,trace,budget,reason);INFO(reason);REQUIRE(built);
  CHECK(trace.pieces.size()>2);CHECK(trace.pieces.size()<=8);
  for(const auto& p:trace.pieces)CHECK(p.facet<8);
  EdgeTrace reverse;auto backward=TraceBudget();REQUIRE(CanonicalSurfaceTrace(g,fixture.policy,sheet,last,first,reverse,backward,reason));
  CHECK(TraceWords(reverse)==TraceWords(ReverseTrace(trace)));
}

TEST_CASE("Canonical surface paths cut a nonseparating protected crease", "[NativeCanonicalTrace3D]") {
  const std::array<Node,4> n{{{1,{1,.37,-8.80388993699896e-19}},{9,{1,1.37,0}},{21,{0,.5,1}},{40,{2,.5,1}}}};
  TraceFixture fixture({{100,0,{n[1],n[2],n[3]}},{101,0,{n[0],n[3],n[2]}},{102,0,{n[0],n[1],n[3]}},{103,0,{n[0],n[2],n[1]}}},125);
  auto& g=*fixture.geometry;REQUIRE(fixture.policy.features.count(Edge{21,40}));REQUIRE(fixture.policy.sheets.at(100)==fixture.policy.sheets.at(101));
  EdgeTrace trace;auto budget=TraceBudget();std::string reason;const bool built=CanonicalSurfaceTrace(g,fixture.policy,fixture.policy.sheets.at(100),g.FacetPoint(100,.25,.25),g.FacetPoint(101,.25,.25),trace,budget,reason);
  INFO(reason);REQUIRE(built);CHECK(trace.pieces.size()>=3);
  for(size_t i=1;i<trace.pieces.size();++i){std::vector<uint64_t> key;
    REQUIRE(g.ReferencePointKey(trace.pieces[i].facet,trace.pieces[i].first,key,budget,reason));REQUIRE(!key.empty());
    if(key[0]==1){REQUIRE(key.size()>=3);CHECK_FALSE(fixture.policy.features.count(Edge{key[1],key[2]}));}}
}

TEST_CASE("Canonical surface trace refuses body sheet and resource violations atomically", "[NativeCanonicalTrace3D]") {
  auto facets=Pyramid();const auto second=Pyramid(100,10);facets.insert(facets.end(),second.begin(),second.end());TraceFixture fixture(facets);
  auto& g=*fixture.geometry;const Id sheet=fixture.policy.sheets.at(0);const auto first=g.FacetPoint(0,.5,.25),last=g.FacetPoint(2,.5,.25);
  EdgeTrace trace;auto budget=TraceBudget();std::string reason;REQUIRE(CanonicalSurfaceTrace(g,fixture.policy,sheet,first,last,trace,budget,reason));const auto saved=TraceWords(trace);
  auto body=TraceBudget();CHECK_FALSE(CanonicalSurfaceTrace(g,fixture.policy,sheet,first,g.FacetPoint(102,.5,.25),trace,body,reason));CHECK(TraceWords(trace)==saved);
  auto wrong_sheet=TraceBudget();CHECK_FALSE(CanonicalSurfaceTrace(g,fixture.policy,fixture.policy.sheets.at(4),first,last,trace,wrong_sheet,reason));CHECK(TraceWords(trace)==saved);
  auto work=TraceBudget();work.maximum_work=0;CHECK_FALSE(CanonicalSurfaceTrace(g,fixture.policy,sheet,first,last,trace,work,reason));CHECK(TraceWords(trace)==saved);CHECK(work.caller_bytes==0);
  auto memory=TraceBudget();memory.maximum_bytes=1;CHECK_FALSE(CanonicalSurfaceTrace(g,fixture.policy,sheet,first,last,trace,memory,reason));CHECK(TraceWords(trace)==saved);CHECK(memory.caller_bytes==0);
}
