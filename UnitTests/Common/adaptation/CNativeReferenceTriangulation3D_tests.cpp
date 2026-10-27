/*!
 * \file CNativeReferenceTriangulation3D_tests.cpp
 * \brief History-independent reference constraints, nonconvex corridors and per-chart holes.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceTriangulation3D.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include <algorithm>
using namespace SU2NativeGeometry;
namespace {
AtlasBudget Budget(){return {size_t(1)<<28,size_t(1)<<28,4096};}
ExactRatio Ratio(double value,AtlasBudget& budget){ExactRatio r;std::string why;REQUIRE(ExactBinary64(value,r,budget,why));return r;}
ExactBary RefPoint(double u,double v,AtlasBudget& budget){return {Ratio(u,budget),Ratio(v,budget)};}
CoveragePiece Piece(ExactBary a,ExactBary b,ExactBary c,AtlasBudget& budget){return {0,{std::move(a),std::move(b),std::move(c)},
  {RefPoint(0,0,budget),RefPoint(1,0,budget),RefPoint(0,1,budget)}};}
std::shared_ptr<MappedFacetReference> Geometry(){using namespace SU2Native3D;
  const Node a{0,{0,0,0}},b{1,{1,0,0}},c{2,{0,1,0}},d{3,{0,0,1}};
  std::vector<Facet> f{{0,0,{a,c,b}},{1,0,{a,b,d}},{2,0,{b,c,d}},{3,0,{a,d,c}}};std::map<Id,double> errors;
  for(const auto& face:f)errors.emplace(face.id,1e-12);return std::make_shared<MappedFacetReference>(std::make_shared<PlanarReference>(f),errors,93);}
std::vector<uint64_t> Words(const std::vector<CoveragePiece>& pieces){auto budget=Budget();std::string why;std::vector<uint64_t> result;
  for(const auto& p:pieces){result.push_back(p.facet);for(const auto* points:{&p.reference,&p.active})for(const auto& point:*points)for(const auto* ratio:{&point.u,&point.v}){
    std::vector<uint64_t> words;REQUIRE(ExactEncode(*ratio,words,budget,why));result.insert(result.end(),words.begin(),words.end());}}return result;}
bool Same(const ExactBary& a,const ExactBary& b){return a.u.negative==b.u.negative&&a.v.negative==b.v.negative&&a.u.numerator==b.u.numerator&&a.u.denominator==b.u.denominator&&a.v.numerator==b.v.numerator&&a.v.denominator==b.v.denominator;}
bool Has(const std::vector<CoveragePiece>& pieces,const ExactBary& point){for(const auto& p:pieces)for(const auto& v:p.reference)if(Same(v,point))return true;return false;}
ExactBary Center(const std::array<ExactBary,3>& triangle,AtlasBudget& budget){std::string why;ExactBary center;const auto three=Ratio(3,budget);
  for(unsigned d=0;d<2;++d){ExactRatio a,b;auto coordinate=[&](unsigned k)->const ExactRatio&{return d?triangle[k].v:triangle[k].u;};
    REQUIRE(ExactAdd(coordinate(0),coordinate(1),a,budget,why));REQUIRE(ExactAdd(a,coordinate(2),b,budget,why));REQUIRE(ExactDivide(b,three,d?center.v:center.u,budget,why));}return center;}
std::vector<CoveragePiece> Refine(const std::vector<CoveragePiece>& pieces,AtlasBudget& budget){std::vector<CoveragePiece> out;
  for(const auto& p:pieces){const auto c=Center(p.reference,budget);for(unsigned k=0;k<3;++k)out.push_back(Piece(p.reference[k],p.reference[(k+1)%3],c,budget));}return out;}
double TwiceArea(const std::vector<CoveragePiece>& pieces){auto budget=Budget();std::string why;ExactRatio sum=Ratio(0,budget);
  for(const auto& p:pieces){ExactRatio x,y,z,w,first,last,area;
    REQUIRE(ExactSubtract(p.reference[1].u,p.reference[0].u,x,budget,why));REQUIRE(ExactSubtract(p.reference[2].v,p.reference[0].v,y,budget,why));
    REQUIRE(ExactSubtract(p.reference[1].v,p.reference[0].v,z,budget,why));REQUIRE(ExactSubtract(p.reference[2].u,p.reference[0].u,w,budget,why));
    REQUIRE(ExactMultiply(x,y,first,budget,why));REQUIRE(ExactMultiply(z,w,last,budget,why));REQUIRE(ExactSubtract(first,last,area,budget,why));
    CHECK_FALSE(area.negative);CHECK_FALSE(area.numerator.empty());REQUIRE(ExactAdd(sum,area,sum,budget,why));}
  std::array<double,2> bounds;REQUIRE(ExactBounds(sum,bounds,budget,why));CHECK(bounds[0]==bounds[1]);return bounds[0];}
}

TEST_CASE("Fresh reference triangulation discards historical interior cuts and preserves trace t", "[NativeReferenceTriangulation3D]") {
  auto geometry=Geometry();auto budget=Budget();std::string why;
  const std::vector<CoveragePiece> original{Piece(RefPoint(0,0,budget),RefPoint(1,0,budget),RefPoint(0,1,budget),budget)};
  const auto historical=Refine(Refine(original,budget),budget);REQUIRE(historical.size()==9);
  std::map<SU2Native3D::Id,SurfaceBinding> current{{10,geometry->Vertex(0)},{20,geometry->Vertex(2)},{30,geometry->Vertex(1)},
    {40,geometry->FacetPoint(0,.25,.25)},{50,geometry->FacetPoint(0,.5,.125)}};
  const auto p=RefPoint(.25,.25,budget),middle=RefPoint(.375,.1875,budget),q=RefPoint(.5,.125,budget);ExactRatio third;
  REQUIRE(ExactDivide(Ratio(1,budget),Ratio(3,budget),third,budget,why));
  EdgeTrace trace;trace.pieces={{0,p,middle,third},{0,middle,q,Ratio(1,budget)}};const std::vector<EdgeTrace> traces{trace};
  std::vector<uint64_t> t_before;REQUIRE(ExactEncode(traces[0].pieces[0].end,t_before,budget,why));
  std::vector<CoveragePiece> first,second;INFO(why);
  REQUIRE(RebuildReferenceTriangulation(*geometry,original,current,traces,first,budget,why));
  REQUIRE(RebuildReferenceTriangulation(*geometry,historical,current,traces,second,budget,why));
  CHECK(Words(first)==Words(second));CHECK(first.size()==7);CHECK(TwiceArea(first)==1);CHECK(Has(first,middle));
  CHECK_FALSE(Has(first,Center(original[0].reference,budget)));
  auto repeated=first;
  for(unsigned event=0;event<5;++event){auto pass=Budget();std::vector<CoveragePiece> rebuilt;
    REQUIRE(RebuildReferenceTriangulation(*geometry,repeated,current,traces,rebuilt,pass,why));CHECK(Words(rebuilt)==Words(first));repeated=std::move(rebuilt);}
  std::vector<uint64_t> t_after;REQUIRE(ExactEncode(traces[0].pieces[0].end,t_after,budget,why));CHECK(t_before==t_after);
}

TEST_CASE("Fresh reference constraints recover a concave polygon without metric-ranked ears", "[NativeReferenceTriangulation3D]") {
  auto geometry=Geometry();auto budget=Budget();std::string why;
  const auto a=RefPoint(.125,.125,budget),b=RefPoint(.625,.125,budget),c=RefPoint(.625,.25,budget),d=RefPoint(.25,.25,budget),e=RefPoint(.25,.625,budget),f=RefPoint(.125,.625,budget);
  std::vector<CoveragePiece> region{Piece(a,b,d,budget),Piece(b,c,d,budget),Piece(a,d,f,budget),Piece(d,e,f,budget)},fresh;
  REQUIRE(RebuildReferenceTriangulation(*geometry,region,{}, {},fresh,budget,why));CHECK(fresh.size()==4);CHECK(TwiceArea(fresh)==.21875);
  for(const auto& point:{a,b,c,d,e,f})CHECK(Has(fresh,point));
}

TEST_CASE("Fresh per-chart winding keeps holes and disconnected components", "[NativeReferenceTriangulation3D]") {
  auto geometry=Geometry();auto budget=Budget();std::string why;
  const std::array<ExactBary,3> outer{RefPoint(.0625,.0625,budget),RefPoint(.8125,.0625,budget),RefPoint(.0625,.8125,budget)};
  const std::array<ExactBary,3> inner{RefPoint(.25,.25,budget),RefPoint(.375,.25,budget),RefPoint(.25,.375,budget)};
  std::vector<CoveragePiece> region;for(unsigned k=0;k<3;++k){const auto next=(k+1)%3;
    region.push_back(Piece(outer[k],outer[next],inner[next],budget));region.push_back(Piece(outer[k],inner[next],inner[k],budget));}
  region.push_back(Piece(RefPoint(.8125,.125,budget),RefPoint(.84375,.125,budget),RefPoint(.8125,.15625,budget),budget));
  std::vector<CoveragePiece> fresh;INFO(why);REQUIRE(RebuildReferenceTriangulation(*geometry,region,{}, {},fresh,budget,why));
  CHECK(fresh.size()==7);CHECK(TwiceArea(fresh)==.5478515625);
  const auto saved=Words(fresh);EdgeTrace crossing;crossing.pieces={{0,RefPoint(.1875,.3125,budget),RefPoint(.4375,.3125,budget),Ratio(1,budget)}};
  auto refusal=Budget();CHECK_FALSE(RebuildReferenceTriangulation(*geometry,region,{}, {crossing},fresh,refusal,why));CHECK(Words(fresh)==saved);
}

TEST_CASE("Fresh reference reconstruction refuses resource and malformed-region failures atomically", "[NativeReferenceTriangulation3D]") {
  auto geometry=Geometry();auto budget=Budget();std::string why;const std::vector<CoveragePiece> region{Piece(RefPoint(0,0,budget),RefPoint(1,0,budget),RefPoint(0,1,budget),budget)};
  std::vector<CoveragePiece> output=region;const auto saved=Words(output);
  auto work=Budget();work.maximum_work=0;CHECK_FALSE(RebuildReferenceTriangulation(*geometry,region,{}, {},output,work,why));CHECK(Words(output)==saved);
  auto bytes=Budget();bytes.maximum_bytes=1;CHECK_FALSE(RebuildReferenceTriangulation(*geometry,region,{}, {},output,bytes,why));CHECK(Words(output)==saved);
  auto corrupt=region;std::swap(corrupt[0].reference[0],corrupt[0].reference[1]);auto invalid=Budget();
  CHECK_FALSE(RebuildReferenceTriangulation(*geometry,corrupt,{}, {},output,invalid,why));CHECK(Words(output)==saved);
  auto duplicate=Budget();const std::map<SU2Native3D::Id,SurfaceBinding> repeated{{1,geometry->Vertex(0)},{2,geometry->Vertex(0)}};
  CHECK_FALSE(RebuildReferenceTriangulation(*geometry,region,repeated,{},output,duplicate,why));CHECK(Words(output)==saved);
}
