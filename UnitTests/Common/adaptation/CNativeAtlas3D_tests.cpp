/*!
 * \file CNativeAtlas3D_tests.cpp
 * \brief Exact clipping coordinates, outward bounds and bounded malformed-input refusal.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "catch.hpp"
#include "../../../Common/include/adaptation/CNativeAtlas3D.hpp"
#include "../../../Common/include/adaptation/CNativeAffineReference.hpp"
#include "../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include <cmath>
#include <cstring>
#include <limits>
using namespace SU2NativeGeometry;
namespace {
AtlasBudget Budget(){return {100000000,16*1024*1024,4096};}
bool Same(const ExactRatio& a,const ExactRatio& b) {
  return a.negative==b.negative && a.numerator==b.numerator && a.denominator==b.denominator;
}
ExactRatio Integer(int64_t value,AtlasBudget& budget) {
  ExactRatio result;std::string reason;REQUIRE(ExactInteger(value,result,budget,reason));return result;
}
ExactRatio Fraction(int64_t numerator,int64_t denominator,AtlasBudget& budget) {
  auto n=Integer(numerator,budget),d=Integer(denominator,budget);ExactRatio result;std::string reason;
  REQUIRE(ExactDivide(n,d,result,budget,reason));return result;
}
}

TEST_CASE("Atlas rationals retain non-dyadic intersections independently of operation order", "[NativeAtlas3D]") {
  auto budget=Budget();std::string reason;
  const auto zero=Integer(0,budget),one=Integer(1,budget),two=Integer(2,budget),three=Integer(3,budget);
  const auto half=Fraction(1,2,budget);ExactRatio divisor,t,x,y,other,sum;
  // Intersect y=x with y=(1-x)/2: the active and original coordinates both contain 1/3.
  REQUIRE(ExactAdd(one,half,divisor,budget,reason));
  REQUIRE(ExactDivide(half,divisor,t,budget,reason));
  CHECK(t.numerator==std::vector<uint32_t>{1});CHECK(t.denominator==std::vector<uint32_t>{3});
  REQUIRE(ExactSubtract(one,t,x,budget,reason));REQUIRE(ExactDivide(x,two,y,budget,reason));CHECK(Same(t,y));
  REQUIRE(ExactDivide(two,three,other,budget,reason));REQUIRE(ExactDivide(other,two,x,budget,reason));CHECK(Same(t,x));
  const auto sixth=Fraction(1,6,budget);REQUIRE(ExactAdd(sixth,sixth,sum,budget,reason));CHECK(Same(t,sum));
  CHECK(Same(Fraction(6,15,budget),Fraction(2,5,budget)));
  CHECK(Same(Fraction(-6,-15,budget),Fraction(2,5,budget)));
  REQUIRE(ExactSubtract(t,t,x,budget,reason));CHECK(Same(x,zero));
  int order=9;REQUIRE(ExactCompare(Fraction(-1,3,budget),zero,order,budget,reason));CHECK(order==-1);
  REQUIRE(ExactCompare(t,sum,order,budget,reason));CHECK(order==0);
  REQUIRE(ExactCompare(half,t,order,budget,reason));CHECK(order==1);
  REQUIRE(ExactCompare(Fraction(-2,3,budget),Fraction(-1,3,budget),order,budget,reason));CHECK(order==-1);
  const auto word=Integer(UINT32_MAX,budget);ExactRatio square,carry;
  REQUIRE(ExactMultiply(word,word,square,budget,reason));
  CHECK(square.numerator==std::vector<uint32_t>{1,UINT32_MAX-1});CHECK(square.denominator==std::vector<uint32_t>{1});
  REQUIRE(ExactDivide(square,word,carry,budget,reason));CHECK(Same(carry,word));
  REQUIRE(ExactAdd(word,one,carry,budget,reason));CHECK(carry.numerator==std::vector<uint32_t>{0,1});
  REQUIRE(ExactSubtract(carry,one,carry,budget,reason));CHECK(Same(carry,word));
  CHECK(budget.work>0);CHECK(budget.peak_bytes<=budget.maximum_bytes);
}

TEST_CASE("Atlas binary64 conversion and bounds preserve subnormals and cancellation", "[NativeAtlas3D]") {
  auto budget=Budget();std::string reason;const double tiny=std::numeric_limits<double>::denorm_min();
  for(double value:{0.,-0.,1.,-17.,.1,tiny,-tiny,std::numeric_limits<double>::min(),std::numeric_limits<double>::max()}) {
    ExactRatio exact;REQUIRE(ExactBinary64(value,exact,budget,reason));
    REQUIRE(ExactValidate(exact,budget,reason));std::array<double,2> bounds;
    REQUIRE(ExactBounds(exact,bounds,budget,reason));CHECK(bounds[0]==value);CHECK(bounds[1]==value);
  }
  ExactRatio dyadic;REQUIRE(ExactBinary64(.1,dyadic,budget,reason));
  CHECK(Same(dyadic,Fraction(INT64_C(3602879701896397),INT64_C(36028797018963968),budget)));
  ExactRatio subnormal;REQUIRE(ExactBinary64(tiny,subnormal,budget,reason));
  CHECK(subnormal.numerator==std::vector<uint32_t>{1});REQUIRE(subnormal.denominator.size()==34);
  CHECK(subnormal.denominator.back()==(uint32_t(1)<<18));
  for(size_t i=0;i+1<subnormal.denominator.size();++i)CHECK(subnormal.denominator[i]==0);
  const auto one=Integer(1,budget),two=Integer(2,budget);ExactRatio sum,cancelled,below;
  REQUIRE(ExactAdd(one,subnormal,sum,budget,reason));REQUIRE(ExactSubtract(sum,one,cancelled,budget,reason));CHECK(Same(cancelled,subnormal));
  REQUIRE(ExactDivide(subnormal,two,below,budget,reason));std::array<double,2> bounds;
  REQUIRE(ExactBounds(below,bounds,budget,reason));CHECK(bounds[0]==0);CHECK(bounds[1]==tiny);
  REQUIRE(ExactSubtract(Integer(0,budget),below,cancelled,budget,reason));
  REQUIRE(ExactBounds(cancelled,bounds,budget,reason));CHECK(bounds[0]==-tiny);CHECK(bounds[1]==0);
  const auto third=Fraction(1,3,budget);REQUIRE(ExactBounds(third,bounds,budget,reason));
  CHECK(std::nextafter(bounds[0],std::numeric_limits<double>::infinity())==bounds[1]);
  ExactRatio endpoint;int order=0;
  REQUIRE(ExactBinary64(bounds[0],endpoint,budget,reason));REQUIRE(ExactCompare(endpoint,third,order,budget,reason));CHECK(order<0);
  REQUIRE(ExactBinary64(bounds[1],endpoint,budget,reason));REQUIRE(ExactCompare(endpoint,third,order,budget,reason));CHECK(order>0);
  REQUIRE(ExactBinary64(std::numeric_limits<double>::max(),endpoint,budget,reason));
  REQUIRE(ExactMultiply(endpoint,two,sum,budget,reason));REQUIRE(ExactBounds(sum,bounds,budget,reason));
  CHECK(bounds[0]==std::numeric_limits<double>::max());CHECK(std::isinf(bounds[1]));
  CHECK(Same(Integer(std::numeric_limits<int64_t>::min(),budget),Fraction(std::numeric_limits<int64_t>::min(),1,budget)));
}

TEST_CASE("Atlas rational word streams reject noncanonical and truncated payloads atomically", "[NativeAtlas3D]") {
  auto budget=Budget();std::string reason;const auto value=Fraction(-2,5,budget),sentinel=Integer(7,budget);
  std::vector<uint64_t> words;REQUIRE(ExactEncode(value,words,budget,reason));
  CHECK(words==std::vector<uint64_t>{1,1,1,2,5});
  words.insert(words.begin(),99);size_t offset=1;ExactRatio decoded=sentinel;
  REQUIRE(ExactDecode(words,offset,decoded,budget,reason));CHECK(Same(decoded,value));CHECK(offset==words.size());
  const std::vector<std::vector<uint64_t>> invalid{
    {},{0,0},{0,0,0},{2,1,1,1,1},{0,1,1,2,4},{1,0,1,1},
    {0,1,1,1,0},{0,2,1,1,0,1},{0,1,1,UINT64_C(1)<<32,1},
    {0,UINT64_MAX,1},{0,1,UINT64_MAX},{0,1,1,1}
  };
  for(const auto& payload:invalid){offset=0;decoded=sentinel;
    CHECK_FALSE(ExactDecode(payload,offset,decoded,budget,reason));CHECK(offset==0);CHECK(Same(decoded,sentinel));CHECK_FALSE(reason.empty());}
  for(const ExactRatio malformed:{ExactRatio{false,{1},{}},ExactRatio{false,{2},{4}},ExactRatio{true,{}, {1}},ExactRatio{false,{0},{1}}}) {
    CHECK_FALSE(ExactValidate(malformed,budget,reason));std::vector<uint64_t> saved{123};
    CHECK_FALSE(ExactEncode(malformed,saved,budget,reason));CHECK(saved==std::vector<uint64_t>{123});
  }
}

TEST_CASE("Atlas arithmetic refuses exhausted budgets and preserves outputs", "[NativeAtlas3D]") {
  auto prepared=Budget();std::string reason;const auto a=Fraction(1,3,prepared),b=Fraction(2,5,prepared),sentinel=Integer(7,prepared);
  auto assert_refusal=[&](AtlasBudget budget){ExactRatio out=sentinel;
    CHECK_FALSE(ExactAdd(a,b,out,budget,reason));CHECK(Same(out,sentinel));CHECK_FALSE(reason.empty());
    CHECK(budget.work<=budget.maximum_work);CHECK(budget.peak_bytes<=budget.maximum_bytes);};
  auto work=Budget();work.maximum_work=0;assert_refusal(work);
  auto partial=Budget();partial.maximum_work=20;assert_refusal(partial);
  auto storage=Budget();storage.maximum_bytes=1;assert_refusal(storage);
  auto bits=Budget();bits.maximum_integer_bits=2;assert_refusal(bits);
  auto overflowing=Budget();overflowing.caller_bytes=SIZE_MAX;assert_refusal(overflowing);
  auto full=Budget();full.work=full.maximum_work;assert_refusal(full);
  auto budget=Budget();ExactRatio out=sentinel;
  CHECK_FALSE(ExactDivide(a,Integer(0,prepared),out,budget,reason));CHECK(Same(out,sentinel));
  CHECK_FALSE(ExactBinary64(std::numeric_limits<double>::infinity(),out,budget,reason));CHECK(Same(out,sentinel));
  auto large=Integer(INT64_C(1)<<30,prepared);auto product_budget=Budget();product_budget.maximum_integer_bits=40;
  CHECK_FALSE(ExactMultiply(large,large,out,product_budget,reason));CHECK(Same(out,sentinel));
  std::array<double,2> bounds{17.,19.};auto bound_budget=Budget();bound_budget.maximum_work=0;
  CHECK_FALSE(ExactBounds(a,bounds,bound_budget,reason));CHECK(bounds==std::array<double,2>{17.,19.});
  std::vector<uint64_t> words{0,1,1,1,3};size_t offset=0;auto decode_budget=Budget();decode_budget.maximum_bytes=1;
  CHECK_FALSE(ExactDecode(words,offset,out,decode_budget,reason));CHECK(offset==0);CHECK(Same(out,sentinel));
}

TEST_CASE("Exact atlas arithmetic retains historical peaks across admitted storage windows", "[NativeAtlas3D]") {
  auto budget=Budget();std::string reason;ExactRatio first,second;
  budget.caller_bytes=1u<<20;
  REQUIRE(ExactInteger(1,first,budget,reason));
  const auto previous=budget.peak_bytes;
  budget.caller_bytes=0;budget.maximum_bytes=4096;
  REQUIRE(ExactInteger(2,second,budget,reason));
  CHECK(second.numerator==std::vector<uint32_t>{2});
  CHECK(budget.peak_bytes==previous);
  budget.maximum_bytes=1;
  CHECK_FALSE(ExactInteger(3,second,budget,reason));
  CHECK(second.numerator==std::vector<uint32_t>{2});
}

TEST_CASE("Exact atlas bounds certify nearby floating proposals within bounded work", "[NativeAtlas3D]") {
  auto prepared=Budget();const auto ratio=Fraction(1,3,prepared);
  auto budget=Budget();budget.maximum_work=10000;std::array<double,2> bounds;std::string reason;
  REQUIRE(ExactBounds(ratio,bounds,budget,reason));
  CHECK(bounds[0]<=1./3.);CHECK(bounds[1]>=1./3.);
  CHECK(std::nextafter(bounds[0],INFINITY)==bounds[1]);
}

TEST_CASE("Canonical dyadic validation avoids repeated binary GCD reduction", "[NativeAtlas3D]") {
  auto prepared=Budget();ExactRatio value;std::string reason;
  REQUIRE(ExactBinary64(.3,value,prepared,reason));
  auto budget=Budget();budget.maximum_work=1200;
  for(unsigned i=0;i<100;++i)REQUIRE(ExactValidate(value,budget,reason));
  CHECK(budget.work<=budget.maximum_work);
}

TEST_CASE("Coverage boundary cancellation preserves internal paths and hanging subdivisions", "[NativeAtlas3D]") {
  using namespace SU2Native3D;
  const Node a{0,{0,0,0}},b{1,{1,0,0}},c{2,{0,1,0}},d{3,{0,0,-1}};
  auto base=std::make_shared<PlanarReference>(std::vector<Facet>{{0,0,{a,b,c}},{1,0,{a,d,b}},{2,0,{b,d,c}},{3,0,{c,d,a}}});
  MappedFacetReference geometry(base,{{0,1e-13},{1,1e-13},{2,1e-13},{3,1e-13}},19);
  auto budget=Budget();std::string reason;const std::array<SurfaceBinding,3> bindings{geometry.Vertex(0),geometry.Vertex(1),geometry.Vertex(2)};
  const std::array<Point,3> flat{a.p,b.p,c.p};FaceCoverage single;
  REQUIRE(geometry.ResolveSingleChart(0,bindings,single,budget,reason));std::array<EdgeTrace,3> traces;
  REQUIRE(geometry.CoverageTraces(single,traces,budget,reason));
  auto point=[&](int u,int v,int den){return ExactBary{Fraction(u,den,budget),Fraction(v,den,budget)};};
  const auto center=point(1,1,4),middle=point(1,1,8);
  const auto v0=point(0,0,1),v1=point(1,0,1),v2=point(0,1,1);
  auto piece=[](std::array<ExactBary,3> triangle){return CoveragePiece{0,triangle,triangle};};
  FaceCoverage fan;fan.pieces={piece({v0,v1,center}),piece({v1,v2,center}),piece({v2,v0,center})};
  INFO(reason);REQUIRE(geometry.ValidateCoverage(0,bindings,flat,fan,traces,budget,reason));
  auto hanging=fan;hanging.pieces[0]=piece({v0,v1,middle});hanging.pieces.push_back(piece({v1,center,middle}));
  REQUIRE(geometry.ValidateCoverage(0,bindings,flat,hanging,traces,budget,reason));
  auto broken=fan;broken.pieces[0].reference[2]=point(3,2,10);
  CHECK_FALSE(geometry.ValidateCoverage(0,bindings,flat,broken,traces,budget,reason));
  CHECK(reason.find("internal reference path")!=std::string::npos);
  auto missing=fan;missing.pieces.pop_back();CHECK_FALSE(geometry.ValidateCoverage(0,bindings,flat,missing,traces,budget,reason));
  auto duplicate=fan;duplicate.pieces.insert(duplicate.pieces.end(),fan.pieces.begin(),fan.pieces.end());
  CHECK_FALSE(geometry.ValidateCoverage(0,bindings,flat,duplicate,traces,budget,reason));
  std::array<size_t,2> work{};unsigned sample=0;
  for(int n:{8,16}) {
    FaceCoverage grid;auto run=Budget();
    auto coordinate=[&](int i,int j){return ExactBary{Fraction(i,n,run),Fraction(j,n,run)};};
    for(int i=0;i<n;++i)for(int j=0;j<n-i;++j) {
      grid.pieces.push_back(piece({coordinate(i,j),coordinate(i+1,j),coordinate(i,j+1)}));
      if(i+j+1<n)grid.pieces.push_back(piece({coordinate(i+1,j),coordinate(i+1,j+1),coordinate(i,j+1)}));
    }
    REQUIRE(grid.pieces.size()==size_t(n*n));const auto before=run.work;
    REQUIRE(geometry.ValidateCoverage(0,bindings,flat,grid,traces,run,reason));work[sample++]=run.work-before;
    CHECK(run.peak_bytes<=run.maximum_bytes);
  }
  // Four times the current cells must not trigger a quadratic full-interior arrangement.
  CHECK(work[1]<8*work[0]);
  auto limited=Budget();limited.maximum_bytes=1;
  CHECK_FALSE(geometry.ValidateCoverage(0,bindings,flat,fan,traces,limited,reason));
}

TEST_CASE("Atlas cumulative work denial is sticky and preserves output", "[NativeAtlas3D]") {
  auto budget=Budget();budget.maximum_work=7;budget.work=5;
  CHECK_FALSE(budget.ChargeWork(3));CHECK(budget.work==5);CHECK(budget.work_exhausted);
  CHECK_FALSE(budget.ChargeWork(0));CHECK_FALSE(budget.ChargeWork(1));CHECK(budget.work==5);
  ExactRatio output;output.numerator={77};output.denominator={1};const auto before=output;std::string reason;
  CHECK_FALSE(ExactInteger(3,output,budget,reason));CHECK(Same(output,before));CHECK_FALSE(reason.empty());
  auto invalid=Budget();ExactRatio numerator,zero,result;REQUIRE(ExactInteger(1,numerator,invalid,reason));
  REQUIRE(ExactInteger(0,zero,invalid,reason));CHECK_FALSE(ExactDivide(numerator,zero,result,invalid,reason));
  CHECK_FALSE(invalid.work_exhausted);
  auto malformed=Budget();malformed.work=malformed.maximum_work+1;CHECK_FALSE(malformed.ChargeWork(0));CHECK(malformed.work_exhausted);
}
