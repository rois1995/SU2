/*!
 * \file CNativeTraceCoverage3D_tests.cpp
 * \brief Fixed shared paths, fresh face disks and bounded sparse correspondence.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeTraceCoverage3D.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include <algorithm>
using namespace SU2NativeGeometry;
using namespace SU2Native3D;
namespace {
AtlasBudget TraceBudget(){return {size_t(1)<<28,size_t(1)<<28,4096};}
ExactRatio TraceRatio(double x,AtlasBudget& budget){ExactRatio value;std::string reason;REQUIRE(ExactBinary64(x,value,budget,reason));return value;}
ExactBary TracePoint(double u,double v,AtlasBudget& budget){return {TraceRatio(u,budget),TraceRatio(v,budget)};}
std::shared_ptr<MappedFacetReference> TraceGeometry(bool bowed=false){
  std::vector<Facet> facets;
  if(!bowed){const Node a{0,{0,0,0}},b{1,{1,0,0}},c{2,{0,1,0}},d{3,{0,0,1}};
    facets={{0,0,{a,c,b}},{1,0,{a,b,d}},{2,0,{b,c,d}},{3,0,{a,d,c}}};
  }else{const Node a{0,{0,0,0}},m{1,{.5,-.25,0}},b{2,{1,0,0}},c{3,{.5,1,0}},top{4,{.5,.25,1}};
    facets={{0,0,{a,c,b}},{1,0,{a,b,m}},{2,0,{c,a,top}},{3,0,{b,c,top}},{4,0,{m,b,top}},{5,0,{a,m,top}}};}
  std::map<Id,double> errors;for(const auto& f:facets)errors.emplace(f.id,1e-12);
  return std::make_shared<MappedFacetReference>(std::make_shared<PlanarReference>(facets),errors,101);
}
CoveragePiece Identity(const MappedFacetReference& geometry,Id facet,AtlasBudget& budget){std::array<SurfaceBinding,3> bindings;
  const auto& face=geometry.Reference().Get(facet);for(unsigned k=0;k<3;++k)bindings[k]=geometry.Vertex(face.v[k].id);
  FaceCoverage out;std::string reason;REQUIRE(geometry.ResolveSingleChart(facet,bindings,out,budget,reason));REQUIRE(out.pieces.size()==1);return out.pieces.front();}
EdgeTrace Reversed(const EdgeTrace& source,AtlasBudget& budget){EdgeTrace out;std::string reason;const auto one=TraceRatio(1,budget),zero=TraceRatio(0,budget);
  for(size_t i=source.pieces.size();i;--i){auto p=source.pieces[i-1];std::swap(p.first,p.last);
    REQUIRE(ExactSubtract(one,i==1?zero:source.pieces[i-2].end,p.end,budget,reason));out.pieces.push_back(std::move(p));}return out;}
std::array<EdgeTrace,3> Oriented(const SurfaceFace& face,const std::map<Edge,EdgeTrace>& traces,AtlasBudget& budget){std::array<EdgeTrace,3> out;
  for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};const bool forward=edge[0]<edge[1];std::sort(edge.begin(),edge.end());out[k]=forward?traces.at(edge):Reversed(traces.at(edge),budget);}return out;}
void FaceTraces(const MappedFacetReference& geometry,const SurfaceFace& face,const std::map<Id,SurfaceBinding>& bindings,
    std::map<Edge,EdgeTrace>& out,AtlasBudget& budget){std::array<SurfaceBinding,3> b;for(unsigned k=0;k<3;++k)b[k]=bindings.at(face.v[k]);
  FaceCoverage cover;std::array<EdgeTrace,3> traces;std::string reason;REQUIRE(geometry.ResolveSingleChart(face.facet,b,cover,budget,reason));REQUIRE(geometry.CoverageTraces(cover,traces,budget,reason));
  for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};const bool forward=edge[0]<edge[1];std::sort(edge.begin(),edge.end());auto trace=forward?traces[k]:Reversed(traces[k],budget);
    const auto old=out.find(edge);if(old==out.end())out.emplace(edge,std::move(trace));else REQUIRE(geometry.EqualTraces(old->second,trace,budget,reason));}}
std::vector<char> TraceWords(const std::map<Face,FaceCoverage>& faces,const std::map<Edge,EdgeTrace>& traces={}){BoundaryCoverage coverage;coverage.faces=faces;coverage.traces=traces;return EncodeBoundaryAtlas(coverage,{});}
void ValidateFaces(const MappedFacetReference& geometry,const std::vector<SurfaceFace>& faces,const std::map<Id,SurfaceBinding>& bindings,
    const std::map<Edge,EdgeTrace>& traces,const std::map<Face,FaceCoverage>& out,AtlasBudget& budget){std::string reason;
  for(auto face:faces){std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());REQUIRE(out.count(face.v));std::array<SurfaceBinding,3> b;std::array<Point,3> actual;
    for(unsigned k=0;k<3;++k){b[k]=bindings.at(face.v[k]);actual[k]=geometry.Realize(b[k]);}
    const auto t=Oriented(face,traces,budget);const bool valid=geometry.ValidateCoverage(face.facet,b,actual,out.at(face.v),t,budget,reason);INFO(reason);CHECK(valid);}
}
}

TEST_CASE("Trace coverage labels two current faces and discards prior interior subdivisions", "[NativeTraceCoverage3D]") {
  auto geometry=TraceGeometry();auto construction=TraceBudget();std::string reason;
  std::vector<CoveragePiece> region{Identity(*geometry,0,construction)};
  std::map<Id,SurfaceBinding> bindings{{100,geometry->Vertex(0)},{200,geometry->Vertex(2)},{300,geometry->Vertex(1)},{400,geometry->EdgePoint({1,2},.5)}};
  const std::vector<SurfaceFace> faces{{{400,100,200},0},{{100,400,300},0}};std::map<Edge,EdgeTrace> traces;
  for(const auto& face:faces)FaceTraces(*geometry,face,bindings,traces,construction);
  const auto input=TraceWords({},traces);std::map<Face,FaceCoverage> output;auto work=TraceBudget();
  const bool built=RebuildTraceCoverage(*geometry,region,faces,bindings,traces,output,work,reason);INFO(reason);REQUIRE(built);REQUIRE(output.size()==2);
  ValidateFaces(*geometry,faces,bindings,traces,output,work);CHECK(TraceWords({},traces)==input);const auto saved=TraceWords(output);
  for(unsigned event=0;event<3;++event){region.clear();for(const auto& cover:output)region.insert(region.end(),cover.second.pieces.begin(),cover.second.pieces.end());auto next=TraceBudget();
    REQUIRE(RebuildTraceCoverage(*geometry,region,faces,bindings,traces,output,next,reason));CHECK(TraceWords(output)==saved);CHECK(next.work<=next.maximum_work);}
  const std::vector<SurfaceFace> partial{faces.back()};auto partial_bindings=bindings;partial_bindings.erase(200);
  std::map<Edge,EdgeTrace> partial_traces;FaceTraces(*geometry,partial.front(),partial_bindings,partial_traces,construction);auto missing_face=TraceBudget();
  CHECK_FALSE(RebuildTraceCoverage(*geometry,region,partial,partial_bindings,partial_traces,output,missing_face,reason));CHECK(TraceWords(output)==saved);
}

TEST_CASE("Trace coverage preserves a nonlinear multichart boundary and frees flat-boundary chords", "[NativeTraceCoverage3D]") {
  auto geometry=TraceGeometry(true);auto construction=TraceBudget();std::string reason;
  const std::vector<CoveragePiece> region{Identity(*geometry,0,construction),Identity(*geometry,1,construction)};
  const std::map<Id,SurfaceBinding> bindings{{100,geometry->Vertex(0)},{200,geometry->Vertex(3)},{300,geometry->Vertex(2)}};
  const std::vector<SurfaceFace> faces{{{100,200,300},0}};std::map<Edge,EdgeTrace> traces;FaceTraces(*geometry,faces.front(),bindings,traces,construction);
  // One shared contract replaces the straight original A-B chord by the accepted A-M-B path.
  traces.at(Edge{100,300}).pieces={{1,TracePoint(0,0,construction),TracePoint(0,1,construction),TraceRatio(.375,construction)},
                                 {1,TracePoint(0,1,construction),TracePoint(1,0,construction),TraceRatio(1,construction)}};
  const auto prescribed=TraceWords({},traces);std::map<Face,FaceCoverage> output;auto work=TraceBudget();
  const bool built=RebuildTraceCoverage(*geometry,region,faces,bindings,traces,output,work,reason);INFO(reason);REQUIRE(built);
  REQUIRE(output.size()==1);CHECK(output.begin()->second.pieces.size()==4);ValidateFaces(*geometry,faces,bindings,traces,output,work);
  CHECK(TraceWords({},traces)==prescribed);
  std::vector<uint64_t> midpoint;REQUIRE(geometry->ReferencePointKey(0,TracePoint(0,.5,construction),midpoint,construction,reason));bool free_midpoint=false;
  for(const auto& p:output.begin()->second.pieces)for(const auto& r:p.reference){std::vector<uint64_t> key;REQUIRE(geometry->ReferencePointKey(p.facet,r,key,construction,reason));free_midpoint=free_midpoint||key==midpoint;}
  CHECK(free_midpoint);CHECK(work.work<=work.maximum_work);
  const auto saved=TraceWords(output);auto missing=traces;missing.erase(missing.begin());auto absent=TraceBudget();
  CHECK_FALSE(RebuildTraceCoverage(*geometry,region,faces,bindings,missing,output,absent,reason));CHECK(TraceWords(output)==saved);
  auto mismatch=traces;mismatch.begin()->second.pieces.front().first=TracePoint(.125,.125,construction);auto invalid=TraceBudget();
  CHECK_FALSE(RebuildTraceCoverage(*geometry,region,faces,bindings,mismatch,output,invalid,reason));CHECK(TraceWords(output)==saved);
  auto no_work=TraceBudget();no_work.maximum_work=0;CHECK_FALSE(RebuildTraceCoverage(*geometry,region,faces,bindings,traces,output,no_work,reason));CHECK(TraceWords(output)==saved);CHECK(no_work.work==0);
  auto no_storage=TraceBudget();no_storage.maximum_bytes=1;CHECK_FALSE(RebuildTraceCoverage(*geometry,region,faces,bindings,traces,output,no_storage,reason));CHECK(TraceWords(output)==saved);
}

TEST_CASE("Trace coverage admits a shared embedding and atomically rejects crossed edge proposals", "[NativeTraceCoverage3D]") {
  auto geometry=TraceGeometry();auto construction=TraceBudget();std::string reason;
  const std::vector<CoveragePiece> region{Identity(*geometry,0,construction)};
  std::map<Id,SurfaceBinding> bindings{{100,geometry->Vertex(0)},{200,geometry->Vertex(2)},
      {300,geometry->Vertex(1)},{400,geometry->FacetPoint(0,.25,.25)},
      {500,geometry->FacetPoint(0,.5,.125)}};
  const std::vector<SurfaceFace> faces{{{100,200,500},0},{{200,400,500},0},
      {{400,100,500},0},{{200,300,400},0},{{300,100,400},0}};
  auto edge_proposals=[&] {
    std::map<Edge,EdgeTrace> result;
    for(const auto& face:faces)for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());
      if(result.count(edge))continue;ExactBary a,b;
      REQUIRE(geometry->BindingParameters(0,bindings.at(edge[0]),a,construction,reason));
      REQUIRE(geometry->BindingParameters(0,bindings.at(edge[1]),b,construction,reason));
      result[edge].pieces.push_back({0,a,b,TraceRatio(1,construction)});
    }
    return result;
  };
  auto traces=edge_proposals();std::map<Face,FaceCoverage> output;auto work=TraceBudget();
  const bool accepted=RebuildTraceCoverage(*geometry,region,faces,bindings,traces,output,work,reason);
  INFO(reason);REQUIRE(accepted);ValidateFaces(*geometry,faces,bindings,traces,output,work);
  const auto saved=TraceWords(output);
  // Every proposed segment still lies in the original facet; collectively they cross.
  std::swap(bindings.at(400),bindings.at(500));traces=edge_proposals();
  const auto unchanged=TraceWords({},traces);auto invalid=TraceBudget();
  CHECK_FALSE(RebuildTraceCoverage(*geometry,region,faces,bindings,traces,output,invalid,reason));
  CHECK(TraceWords(output)==saved);CHECK(TraceWords({},traces)==unchanged);
}
