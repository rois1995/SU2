/*!
 * \file CNativeAffineReference.cpp
 * \brief Strict arithmetic and finite chart geometry; no solver or target-metric dispatch.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeAffineReference.hpp"
#include "CNativePredicates3D_internal.hpp"
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CNativeGeometryValidation2D.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <queue>
#include <type_traits>
namespace SU2NativeGeometry {
namespace {
using Vec=std::array<double,3>;
Vec Values(Point p){return {p.x,p.y,p.z};}
Point ToPoint(Vec p){return {p[0],p[1],p[2]};}
void Finite(Point p){for(auto x:Values(p))if(!std::isfinite(x))throw std::invalid_argument("Nonfinite affine geometry coordinate.");}
// Each operation encloses the exact real result, including underflow. Compile without fast math/contraction.
struct Range {long double lo=0,hi=0;};
Range Add(Range a,Range b){return {std::nextafter(a.lo+b.lo,-INFINITY),std::nextafter(a.hi+b.hi,INFINITY)};}
Range Multiply(Range a,long double b){return b>=0?Range{std::nextafter(a.lo*b,-INFINITY),std::nextafter(a.hi*b,INFINITY)}:
    Range{std::nextafter(a.hi*b,-INFINITY),std::nextafter(a.lo*b,INFINITY)};}
Range Subtract(double a,double b){const auto x=static_cast<long double>(a)-b;return {std::nextafter(x,-INFINITY),std::nextafter(x,INFINITY)};}
using Box=std::array<Range,3>;
constexpr unsigned CubicIndex[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
Range Subtract(Range a,Range b){return Add(a,{-b.hi,-b.lo});}
Range Multiply(Range a,Range b) {
  const long double p[]={a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi};
  return {std::nextafter(*std::min_element(p,p+4),-INFINITY),std::nextafter(*std::max_element(p,p+4),INFINITY)};
}
Range DividePositive(Range a,long double d){return Multiply(a,Range{std::nextafter(1/d,-INFINITY),std::nextafter(1/d,INFINITY)});}
Box Exact(Point p){const auto v=Values(p);return {{{v[0],v[0]},{v[1],v[1]},{v[2],v[2]}}};}
Box ParameterBox(const SU2Native3D::ParametricPoint& p) {
  const auto b=SU2Native3D::ParametricPointBounds(p);const auto lo=Values(b[0]),hi=Values(b[1]);
  return {{{lo[0],hi[0]},{lo[1],hi[1]},{lo[2],hi[2]}}};
}
unsigned PolynomialIndex(unsigned degree,unsigned i,unsigned j){return i*(degree+1)-i*(i-1)/2+j;}
Box Blossom(const MappedFacetReference::CubicNet& net,const std::array<Box,3>& arguments) {
  std::array<Box,10> level{};
  for(unsigned q=0;q<10;++q)level[PolynomialIndex(3,CubicIndex[q][0],CubicIndex[q][1])]=Exact(net[q]);
  for(unsigned degree=3;degree>0;--degree) {
    std::array<Box,10> next{};const auto& a=arguments[3-degree];
    for(unsigned i=0;i<degree;++i)for(unsigned j=0;i+j<degree;++j)for(unsigned d=0;d<3;++d)
      next[PolynomialIndex(degree-1,i,j)][d]=Add(Add(Multiply(a[0],level[PolynomialIndex(degree,i+1,j)][d]),
          Multiply(a[1],level[PolynomialIndex(degree,i,j+1)][d])),Multiply(a[2],level[PolynomialIndex(degree,i,j)][d]));
    level=next;
  }
  return level[0];
}
Box LinearPoint(const std::array<Point,3>& source,const Box& parameter) {
  Box out{};for(unsigned d=0;d<3;++d)for(unsigned k=0;k<3;++k)out[d]=Add(out[d],Multiply(parameter[k],Values(source[k])[d]));
  return out;
}
double NormBound(const Box& a) {
  Range sum;
  for(const auto& r:a){const auto bound=std::max(std::abs(r.lo),std::abs(r.hi));sum=Add(sum,Multiply({bound,bound},bound));}
  const auto value=std::nextafter(std::sqrt(sum.hi),INFINITY);
  return std::nextafter(double(std::max(value,(long double)std::numeric_limits<double>::min())),INFINITY);
}
Box Difference(const Box& a,const Box& b){Box c;for(unsigned d=0;d<3;++d)c[d]=Subtract(a[d],b[d]);return c;}
Box CrossBox(const Box& a,const Box& b){Box c;for(unsigned d=0;d<3;++d)c[d]=Subtract(Multiply(a[(d+1)%3],b[(d+2)%3]),Multiply(a[(d+2)%3],b[(d+1)%3]));return c;}
Range DotBox(const Box& a,const Box& b){Range c;for(unsigned d=0;d<3;++d)c=Add(c,Multiply(a[d],b[d]));return c;}
std::array<Box,15> CubicNormalControls(const std::array<Box,10>& net) {
  std::array<Box,10> b{};for(unsigned q=0;q<10;++q)b[PolynomialIndex(3,CubicIndex[q][0],CubicIndex[q][1])]=net[q];
  std::array<Box,6> u{},v{};
  for(unsigned i=0;i<=2;++i)for(unsigned j=0;i+j<=2;++j) {
    const auto k=PolynomialIndex(2,i,j);
    u[k]=Difference(b[PolynomialIndex(3,i,j+1)],b[PolynomialIndex(3,i+1,j)]);
    v[k]=Difference(b[PolynomialIndex(3,i,j)],b[PolynomialIndex(3,i+1,j)]);
  }
  std::array<Box,15> coefficients{};
  const unsigned factorial[]={1,1,2,6,24};
  for(unsigned i=0;i<=2;++i)for(unsigned j=0;i+j<=2;++j)
    for(unsigned k=0;k<=2;++k)for(unsigned l=0;k+l<=2;++l) {
      const auto numerator=4*factorial[i+k]*factorial[j+l]*factorial[4-i-j-k-l];
      const auto denominator=24*factorial[i]*factorial[j]*factorial[2-i-j]*factorial[k]*factorial[l]*factorial[2-k-l];
      const long double weight=(long double)numerator/denominator;
      const Range w{std::nextafter(weight,-INFINITY),std::nextafter(weight,INFINITY)};
      auto& c=coefficients[PolynomialIndex(4,i+k,j+l)];
      const auto normal=CrossBox(u[PolynomialIndex(2,i,j)],v[PolynomialIndex(2,k,l)]);
      for(unsigned d=0;d<3;++d)c[d]=Add(c[d],Multiply(normal[d],w));
    }
  return coefficients;
}
double PositiveNormalClearance(const std::array<Box,10>& net,Point query) {
  long double projection=-INFINITY;double length=0;
  // Keep each control point intact: a coordinate box loses tilted-plane cancellation.
  for(const auto& normal:CubicNormalControls(net)) {
    length=std::max(length,NormBound(normal));
    for(const auto& point:net)projection=std::max(projection,DotBox(Difference(Exact(query),point),normal).hi);
  }
  if(!(projection<0 && std::isfinite(length) && length>0))return 0;
  return std::max(0.,std::nextafter(double(std::nextafter(-projection/length,-INFINITY)),-INFINITY));
}
/* Positive Bernstein coefficients certify a positive projected Jacobian over the entire patch. */
bool PositiveCubicJacobian(const MappedFacetReference::CubicNet& net) {
  std::array<Box,10> controls;for(unsigned q=0;q<10;++q)controls[q]=Exact(net[q]);
  const auto n=CrossBox(Difference(controls[1],controls[0]),Difference(controls[2],controls[0]));
  for(const auto& normal:CubicNormalControls(controls)) {
    const auto r=DotBox(normal,n);if(!(r.lo>0&&std::isfinite(r.hi)))return false;
  }
  return true;
}
double CubicDeviation(const MappedFacetReference::CubicNet& net,const std::array<Box,3>& parameters,
                      const std::array<Point,3>& realized) {
  double bound=0;
  for(unsigned q=0;q<10;++q) {
    std::array<Box,3> arguments;unsigned at=0;
    for(unsigned k=0;k<3;++k)for(unsigned n=0;n<CubicIndex[q][k];++n)arguments[at++]=parameters[k];
    const auto curved=Blossom(net,arguments);Box flat{};
    for(unsigned k=0;k<3;++k)for(unsigned d=0;d<3;++d) {
      const auto weight=(long double)CubicIndex[q][k]/3;
      flat[d]=Add(flat[d],Multiply({std::nextafter(weight,-INFINITY),std::nextafter(weight,INFINITY)},Values(realized[k])[d]));
    }
    bound=std::max(bound,NormBound(Difference(curved,flat)));
  }
  return bound;
}
struct AtlasOps {
  AtlasBudget& budget;std::string reason;
  const char* name;size_t start=0,children=0;AtlasOps* previous=nullptr;
  static AtlasOps*& Active(){static thread_local AtlasOps* active=nullptr;return active;}
  static bool Trace(){static const bool enabled=[](){const char* value=std::getenv("SU2_NATIVE_GEOMETRY_WORK_TRACE");return value&&std::strcmp(value,"1")==0;}();return enabled;}
  AtlasOps(AtlasBudget& b,const char* operation):budget(b),name(operation){if(Trace()){start=budget.work;previous=Active();Active()=this;}}
  AtlasOps(const AtlasOps&)=delete;AtlasOps& operator=(const AtlasOps&)=delete;
  ~AtlasOps() noexcept {if(!Trace())return;Active()=previous;const size_t total=budget.work>=start?budget.work-start:0;
    for(auto* parent=previous;parent;parent=parent->previous)if(&parent->budget==&budget){parent->children+=total;break;}
    std::fprintf(stderr,"NATIVE_GEOMETRY_WORK operation=%s total=%zu exclusive=%zu\n",name,total,total>=children?total-children:0);}
  void Check(bool ok){if(!ok)throw std::runtime_error(reason);}
  void Work(){if(!budget.ChargeWork())throw std::length_error("Curved atlas work exhausted.");}
  void Storage(size_t bytes) {
    const auto total=transfer_memory::Add(budget.caller_bytes,bytes);budget.peak_bytes=std::max(budget.peak_bytes,total);
    if(total==SIZE_MAX || total>budget.maximum_bytes)throw std::length_error("Curved atlas storage exhausted: requested "+std::to_string(total)+", ceiling "+std::to_string(budget.maximum_bytes)+", operation="+name+", caller="+std::to_string(budget.caller_bytes)+", scratch="+std::to_string(bytes)+".");
  }
  ExactRatio Integer(int64_t x){ExactRatio r;Check(ExactInteger(x,r,budget,reason));return r;}
  ExactRatio Binary(double x){ExactRatio r;Check(ExactBinary64(x,r,budget,reason));return r;}
  ExactRatio Add(const ExactRatio& a,const ExactRatio& b){ExactRatio r;Check(ExactAdd(a,b,r,budget,reason));return r;}
  ExactRatio Sub(const ExactRatio& a,const ExactRatio& b){ExactRatio r;Check(ExactSubtract(a,b,r,budget,reason));return r;}
  ExactRatio Mul(const ExactRatio& a,const ExactRatio& b){ExactRatio r;Check(ExactMultiply(a,b,r,budget,reason));return r;}
  ExactRatio Div(const ExactRatio& a,const ExactRatio& b){ExactRatio r;Check(ExactDivide(a,b,r,budget,reason));return r;}
  int Compare(const ExactRatio& a,const ExactRatio& b){int r;Check(ExactCompare(a,b,r,budget,reason));return r;}
  bool Equal(const ExactBary& a,const ExactBary& b){return !Compare(a.u,b.u)&&!Compare(a.v,b.v);}
  ExactRatio Weight(const ExactBary& p,unsigned k){return k==1?p.u:k==2?p.v:Sub(Sub(Integer(1),p.u),p.v);}
  void Bary(const ExactBary& p) {
    Check(ExactValidate(p.u,budget,reason));Check(ExactValidate(p.v,budget,reason));
    if(Compare(p.u,Integer(0))<0 || Compare(p.v,Integer(0))<0 || Compare(Add(p.u,p.v),Integer(1))>0)
      throw std::invalid_argument("Curved coverage parameter leaves its simplex.");
  }
  ExactRatio Orient(const ExactBary& a,const ExactBary& b,const ExactBary& c) {
    return Sub(Mul(Sub(b.u,a.u),Sub(c.v,a.v)),Mul(Sub(b.v,a.v),Sub(c.u,a.u)));
  }
  ExactBary Lerp(const ExactBary& a,const ExactBary& b,const ExactRatio& t) {
    return {Add(a.u,Mul(t,Sub(b.u,a.u))),Add(a.v,Mul(t,Sub(b.v,a.v)))};
  }
  std::vector<uint64_t> Key(const ExactBary& p) {
    std::vector<uint64_t> a,b;Check(ExactEncode(p.u,a,budget,reason));Check(ExactEncode(p.v,b,budget,reason));a.insert(a.end(),b.begin(),b.end());return a;
  }
  Box Parameter(const ExactBary& p) {
    Box r;for(unsigned k=0;k<3;++k){std::array<double,2> b;const auto w=Weight(p,k);Check(ExactBounds(w,b,budget,reason));r[k]={b[0],b[1]};}
    return r;
  }
};
// Floating projections propose a point; interval evaluation certifies that convex witness.
Point BoxMiddle(const Box& box) {
  Vec p;for(unsigned d=0;d<3;++d)p[d]=double(.5L*box[d].lo+.5L*box[d].hi);
  auto out=ToPoint(p);Finite(out);return out;
}
struct TriangleDistanceBounds {double lower=0,upper=INFINITY;};
TriangleDistanceBounds TriangleDistance(const Box& point,const std::array<Point,3>& triangle,AtlasOps& op) {
  op.Work();const auto p=Values(BoxMiddle(point)),a=Values(triangle[0]),b=Values(triangle[1]),c=Values(triangle[2]);
  using Wide=std::array<long double,3>;Wide ab{},ac{},ap{};
  for(unsigned d=0;d<3;++d){ab[d]=(long double)b[d]-a[d];ac[d]=(long double)c[d]-a[d];ap[d]=(long double)p[d]-a[d];}
  auto dot=[](Wide x,Wide y){return x[0]*y[0]+x[1]*y[1]+x[2]*y[2];};
  double best=INFINITY;Box witness{};
  auto propose=[&](long double wb,long double wc) {
    if(!(wb>=0&&wc>=0&&wb+wc<=1))return;
    const double u=std::clamp(double(wb+wc),0.,1.),v=u?std::clamp(double(wc/(wb+wc)),0.,1.):0.;
    Box q;for(unsigned d=0;d<3;++d)q[d]=Add(Multiply({a[d],a[d]},Subtract({1,1},{u,u})),
        Multiply(Add(Multiply({b[d],b[d]},Subtract({1,1},{v,v})),Multiply({c[d],c[d]},v)),u));
    const auto bound=NormBound(Difference(point,q));if(bound<best){best=bound;witness=q;}
  };
  propose(0,0);propose(1,0);propose(0,1);
  const auto aa=dot(ab,ab),bb=dot(ab,ac),cc=dot(ac,ac),x=dot(ap,ab),y=dot(ap,ac),det=aa*cc-bb*bb;
  if(det>0)propose((x*cc-y*bb)/det,(y*aa-x*bb)/det);
  if(aa>0)propose(std::clamp(x/aa,0.L,1.L),0);
  if(cc>0)propose(0,std::clamp(y/cc,0.L,1.L));
  Wide bc{},bp{};for(unsigned d=0;d<3;++d){bc[d]=(long double)c[d]-b[d];bp[d]=(long double)p[d]-b[d];}
  if(dot(bc,bc)>0){const auto t=std::clamp(dot(bp,bc)/dot(bc,bc),0.L,1.L);propose(1-t,t);}
  const auto direction=Exact(BoxMiddle(Difference(point,witness)));const auto length=NormBound(direction);
  long double lower=INFINITY;
  for(auto vertex:triangle)lower=std::min(lower,DotBox(Difference(point,Exact(vertex)),direction).lo);
  if(!(length>0&&std::isfinite(length)&&lower>0))lower=0;
  else lower=std::nextafter(lower/length,-INFINITY);
  return {std::max(0.,std::nextafter(double(lower),-INFINITY)),best};
}
struct UnionTriangle {std::array<Point,3> points;double error=0;};
class TriangleUnion {
 public:
  TriangleUnion(const std::vector<UnionTriangle>& triangles,AtlasOps& op,double resolution):triangles(triangles),op(op),accuracy(resolution) {
    if(triangles.empty())throw std::invalid_argument("Empty geometry triangle union.");
    const auto count=triangles.size();size_t retained=0,peak=0;
    if(count>16)CADTElemClass::PredictBytes(3,2*count,count,2*count,&retained,&peak);
    const auto workspace=transfer_memory::Mul(2*std::max<size_t>(200,count+1),sizeof(CBBoxTargetClass)+2*sizeof(unsigned long));
    const auto scratch=transfer_memory::Add(std::max(retained,peak),workspace,transfer_memory::Mul(count,160));
    op.Storage(scratch);candidates.reserve(count);
    if(count>16) {
      std::vector<su2double> coordinates;std::vector<unsigned long> connectivity,ids;
      coordinates.reserve(6*count);connectivity.reserve(2*count);ids.reserve(count);
      for(const auto& t:triangles) {
        Vec lo=Values(t.points[0]),hi=lo;
        for(auto p:t.points)for(unsigned d=0;d<3;++d){lo[d]=std::min(lo[d],Values(p)[d]);hi[d]=std::max(hi[d],Values(p)[d]);}
        for(unsigned d=0;d<3;++d){lo[d]=std::nextafter(double(Subtract(lo[d],t.error).lo),-INFINITY);hi[d]=std::nextafter(double(Add({hi[d],hi[d]},{t.error,t.error}).hi),INFINITY);}
        Finite(ToPoint(lo));Finite(ToPoint(hi));coordinates.insert(coordinates.end(),{lo[0],lo[1],lo[2],hi[0],hi[1],hi[2]});
        connectivity.push_back(connectivity.size());connectivity.push_back(connectivity.size());ids.push_back(ids.size());
      }
      std::vector<unsigned short> types(count,LINE),markers(count,0);
      index=std::make_unique<CADTElemClass>(3,coordinates,connectivity,types,markers,ids,false);
    }
    op.budget.caller_bytes=transfer_memory::Add(op.budget.caller_bytes,scratch);BuildPlanes();
  }
  TriangleDistanceBounds Distance(const Box& point) {
    const size_t seed=Seed(point);auto bound=Distance(point,seed);Candidates(point,bound.upper);
    for(auto i:candidates){const auto b=Distance(point,i);bound.lower=std::min(bound.lower,b.lower);bound.upper=std::min(bound.upper,b.upper);}
    return bound;
  }
  template<size_t N> double Upper(const std::array<Box,N>& hull) {
    Box box;for(unsigned d=0;d<3;++d){box[d]={INFINITY,-INFINITY};for(const auto& p:hull){box[d].lo=std::min(box[d].lo,p[d].lo);box[d].hi=std::max(box[d].hi,p[d].hi);}}
    auto witness=[&](size_t i){double b=0;for(const auto& p:hull)b=std::max(b,Distance(p,i).upper);return b;};
    if(++visit==0){std::fill(visited.begin(),visited.end(),0);visit=1;}
    auto plane_bound=[&](size_t i)->double{const auto group=triangle_plane[i];if(group==SIZE_MAX||visited[group]==visit)return INFINITY;
      visited[group]=visit;op.Work();return planes[group].Upper(hull);};
    const auto seed=Seed(box);double best=plane_bound(seed);if(best<=accuracy/4)return best;
    best=std::min(best,witness(seed));Candidates(box,best);
    for(auto i:candidates){best=std::min(best,plane_bound(i));if(best<=accuracy/4)return best;best=std::min(best,witness(i));}return best;
  }
 private:
  struct PlaneUnion {
    std::shared_ptr<FinitePlaneReference> footprint;
    Box normal;Point origin;unsigned axis=0;double error=0;
    std::array<double,2> lo{},hi{};bool positive=true;
    template<size_t N> double Upper(const std::array<Box,N>& hull)const {
      const unsigned x=(axis==0?1:0),y=(axis==2?1:2);
      const auto denominator=std::max(normal[axis].lo,-normal[axis].hi);
      if(!(denominator>0))return INFINITY;
      long double vertical=0,dx=0,dy=0;std::array<long double,2> lower{{INFINITY,INFINITY}},upper{{-INFINITY,-INFINITY}};
      for(const auto& p:hull) {
        const auto r=DotBox(Difference(p,Exact(origin)),normal);
        vertical=std::max(vertical,std::max(std::abs(r.lo),std::abs(r.hi)));
        for(unsigned k=0;k<2;++k){const auto d=k?y:x;lower[k]=std::min(lower[k],p[d].lo);upper[k]=std::max(upper[k],p[d].hi);}
      }
      std::array<double,2> a,b;
      for(unsigned k=0;k<2;++k){a[k]=std::clamp(std::nextafter(double(lower[k]),-INFINITY),lo[k],hi[k]);
        b[k]=std::clamp(std::nextafter(double(upper[k]),INFINITY),lo[k],hi[k]);
        const auto shift=std::max({0.L,Subtract({lo[k],lo[k]},{lower[k],lower[k]}).hi,Subtract({upper[k],upper[k]},{hi[k],hi[k]}).hi});
        (k?dy:dx)=shift;}
      const Point p{a[0],a[1],0},q{b[0],a[1],0},r{b[0],b[1],0},s{a[0],b[1],0};
      if(a[0]==b[0]||a[1]==b[1]) {if(!footprint->ContainsSegment(p,r))return INFINITY;}
      else if(positive ? (!footprint->ContainsFace({p,q,r})||!footprint->ContainsFace({p,r,s})) :
          (!footprint->ContainsFace({p,r,q})||!footprint->ContainsFace({p,s,r})))return INFINITY;
      const auto nx=std::max(std::abs(normal[x].lo),std::abs(normal[x].hi)),ny=std::max(std::abs(normal[y].lo),std::abs(normal[y].hi));
      const auto dz=DividePositive(Add(Multiply({dx,dx},nx),Multiply({dy,dy},ny)),denominator);
      const Box shift{{{0,dx},{0,dy},{0,std::max(std::abs(dz.lo),std::abs(dz.hi))}}};
      return std::nextafter(double(Add(Add(DividePositive({0,vertical},denominator),{error,error}),{0,NormBound(shift)}).hi),INFINITY);
    }
  };
  std::vector<PlaneUnion> planes;std::vector<size_t> triangle_plane;
  std::vector<uint64_t> visited;uint64_t visit=0;double accuracy;
  void BuildPlanes() {
    using namespace SU2Native3D;
    const size_t count=triangles.size(),none=SIZE_MAX;op.Storage(transfer_memory::Mul(count,1024));
    std::map<Vec,Id> vertices;std::map<std::array<Id,2>,size_t> edges;std::vector<size_t> roots(count);
    std::vector<std::array<Id,3>> ids(count);std::iota(roots.begin(),roots.end(),0);
    auto root=[&](size_t i){while(roots[i]!=i)i=roots[i];return i;};
    for(size_t i=0;i<count;++i) {
      for(unsigned k=0;k<3;++k){const auto p=Values(triangles[i].points[k]);const auto result=vertices.emplace(p,vertices.size());ids[i][k]=result.first->second;}
      for(unsigned k=0;k<3;++k){std::array<Id,2> edge{ids[i][k],ids[i][(k+1)%3]};std::sort(edge.begin(),edge.end());
        const auto old=edges.emplace(edge,i);if(old.second)continue;const auto j=old.first->second;
        op.Work();const auto& t=triangles[j].points;bool coplanar=SurfaceOrientationSign(t,t,nullptr)>0;
        for(auto p:triangles[i].points)coplanar=coplanar&&Orientation(t[0],t[1],t[2],p)==0;
        if(coplanar){const auto a=root(i),b=root(j);roots[std::max(a,b)]=std::min(a,b);}
      }
    }
    std::map<size_t,std::vector<size_t>> groups;for(size_t i=0;i<count;++i)groups[root(i)].push_back(i);
    triangle_plane.assign(count,none);size_t retained=transfer_memory::Mul(count,1024);
    for(const auto& group:groups) {
      if(group.second.size()<2)continue;
      PlaneUnion plane;const auto& first=triangles[group.first].points;plane.origin=first[0];
      plane.normal=CrossBox(Difference(Exact(first[1]),Exact(first[0])),Difference(Exact(first[2]),Exact(first[0])));
      for(unsigned d=1;d<3;++d)if(std::abs(plane.normal[d].lo+plane.normal[d].hi)>std::abs(plane.normal[plane.axis].lo+plane.normal[plane.axis].hi))plane.axis=d;
      if(std::max(plane.normal[plane.axis].lo,-plane.normal[plane.axis].hi)<=0)continue;
      const unsigned x=(plane.axis==0?1:0),y=(plane.axis==2?1:2);
      auto project=[&](Point p){const auto v=Values(p);return Point{v[x],v[y],0};};
      plane.lo={{INFINITY,INFINITY}};plane.hi={{-INFINITY,-INFINITY}};
      std::vector<Facet> facets;facets.reserve(group.second.size());
      for(auto i:group.second){Facet f;f.id=i;plane.error=std::max(plane.error,triangles[i].error);
        for(unsigned k=0;k<3;++k){f.v[k]={ids[i][k],project(triangles[i].points[k])};const auto p=Values(f.v[k].p);
          for(unsigned d=0;d<2;++d){plane.lo[d]=std::min(plane.lo[d],p[d]);plane.hi[d]=std::max(plane.hi[d],p[d]);}}
        facets.push_back(f);}
      AffineAuthority authority;authority.origin={};authority.normal={0,0,1};authority.dimension=3;authority.version=1;authority.maximum_error=4*std::numeric_limits<double>::min();
      const auto temporary=transfer_memory::Add(PlanarReference::ConstructionBytes(facets.size()),transfer_memory::Mul(facets.size(),4096));
      op.Storage(transfer_memory::Add(retained,temporary));
      plane.footprint=std::make_shared<FinitePlaneReference>(authority,facets,temporary);
      const auto& t=facets.front().v;plane.positive=SurfaceNormalSign({0,0,1},{t[0].p,t[1].p,t[2].p},nullptr)>0;
      retained=transfer_memory::Add(retained,plane.footprint->ResidentBytes());
      for(auto i:group.second)triangle_plane[i]=planes.size();planes.push_back(std::move(plane));
    }
    visited.resize(planes.size());op.budget.caller_bytes=transfer_memory::Add(op.budget.caller_bytes,retained);
  }
  const std::vector<UnionTriangle>& triangles;AtlasOps& op;
  std::unique_ptr<CADTElemClass> index;std::vector<unsigned long> candidates;
  TriangleDistanceBounds Distance(const Box& p,size_t i) {
    auto b=TriangleDistance(p,triangles[i].points,op);const double e=triangles[i].error;
    b.lower=std::max(0.,std::nextafter(double(Subtract(b.lower,e).lo),-INFINITY));
    b.upper=std::nextafter(double(Add({b.upper,b.upper},{e,e}).hi),INFINITY);return b;
  }
  size_t Seed(const Box& box) {
    if(!index)return 0;
    op.Work();const auto p=Values(BoxMiddle(box));const su2double x[]={p[0],p[1],p[2]};su2double distance=0;unsigned short marker=0;unsigned long id=0;int rank=0;
    index->DetermineNearestElement(x,distance,marker,id,rank);return id;
  }
  void Candidates(const Box& box,double distance) {
    candidates.clear();if(!index){for(size_t i=0;i<triangles.size();++i)candidates.push_back(i);return;}
    op.Work();if(!std::isfinite(distance))throw std::invalid_argument("Unrepresentable surface-union distance.");
    su2double lo[3],hi[3];for(unsigned d=0;d<3;++d){lo[d]=std::nextafter(double(Subtract(box[d],{distance,distance}).lo),-INFINITY);hi[d]=std::nextafter(double(Add(box[d],{distance,distance}).hi),INFINITY);}
    index->DetermineIntersectingElements(lo,hi,candidates);
  }
};
struct UnionDomain {Id facet=0;std::array<Point,3> parameter{{{1,0,0},{0,1,0},{0,0,1}}};unsigned depth=0;};
std::array<UnionDomain,4> UnionChildren(const UnionDomain& parent) {
  if(parent.depth>=48)throw std::length_error("Surface-union parameters exhausted exact dyadic resolution; bounds unresolved.");
  std::array<Point,3> mid;
  for(unsigned k=0;k<3;++k){Vec p;const auto a=Values(parent.parameter[k]),b=Values(parent.parameter[(k+1)%3]);for(unsigned d=0;d<3;++d)p[d]=.5*a[d]+.5*b[d];mid[k]=ToPoint(p);}
  std::array<UnionDomain,4> out;for(unsigned k=0;k<3;++k)out[k]={parent.facet,{parent.parameter[k],mid[k],mid[(k+2)%3]},parent.depth+1};
  out[3]={parent.facet,{mid[0],mid[1],mid[2]},parent.depth+1};return out;
}
using AtlasKey=std::vector<uint64_t>;
AtlasKey ReferenceKey(const SU2Native3D::PlanarReference& reference,Id facet,const ExactBary& p,AtlasOps& op) {
  op.Bary(p);const auto& f=reference.Get(facet);std::array<ExactRatio,3> w;
  std::vector<unsigned> nonzero;for(unsigned k=0;k<3;++k){w[k]=op.Weight(p,k);if(op.Compare(w[k],op.Integer(0)))nonzero.push_back(k);}
  if(nonzero.size()==1)return {0,f.v[nonzero[0]].id};
  if(nonzero.size()==2) {
    auto a=nonzero[0],b=nonzero[1];if(f.v[a].id>f.v[b].id)std::swap(a,b);
    std::vector<uint64_t> t;op.Check(ExactEncode(w[b],t,op.budget,op.reason));
    AtlasKey key{1,f.v[a].id,f.v[b].id};key.insert(key.end(),t.begin(),t.end());return key;
  }
  auto key=op.Key(p);key.insert(key.begin(),{2,facet});return key;
}
struct AtlasEdge {
  Id plane=0,facet=0;ExactBary a,b,ra,rb;int sign=1;size_t triangle=0;
};
std::vector<AtlasEdge> SplitAtlasEdges(const std::vector<AtlasEdge>& edges,AtlasOps& op) {
  // Split exact collinear subdivisions before incidence/cancellation; no rounded intersections.
  op.Storage(transfer_memory::Mul(edges.size(),sizeof(AtlasEdge)+128));
  std::map<Id,std::vector<const AtlasEdge*>> planes;size_t requested=transfer_memory::Mul(edges.size(),sizeof(AtlasEdge)+128);
  for(const auto& e:edges)planes[e.plane].push_back(&e);
  for(const auto& row:planes) {
    size_t limbs=1;for(const auto* e:row.second)for(const auto* p:{&e->a,&e->b,&e->ra,&e->rb})for(const auto* r:{&p->u,&p->v})
      limbs=std::max({limbs,r->numerator.size(),r->denominator.size()});
    // Lerp after one exact quotient needs at most eight input bit widths plus carries.
    const auto bits=std::min(op.budget.maximum_integer_bits,transfer_memory::Add(transfer_memory::Mul(limbs,256),32));
    const auto ratio=transfer_memory::Add(sizeof(ExactRatio),transfer_memory::Mul(transfer_memory::Add(bits,31)/32,8));
    const auto unit=transfer_memory::Add(sizeof(AtlasEdge),transfer_memory::Mul(ratio,16),256);
    requested=transfer_memory::Add(requested,transfer_memory::Mul(row.second.size()+1,transfer_memory::Mul(row.second.size()+1,unit)));
  }
  op.Storage(requested);
  std::vector<AtlasEdge> split;
  for(const auto& e:edges) {
    const auto du=op.Sub(e.b.u,e.a.u),dv=op.Sub(e.b.v,e.a.v);const bool use_u=op.Compare(du,op.Integer(0));
    if(!use_u&&!op.Compare(dv,op.Integer(0)))throw std::invalid_argument("Collapsed curved coverage edge.");
    std::vector<ExactRatio> cuts{op.Integer(0),op.Integer(1)};
    for(const auto* other:planes.at(e.plane))for(const auto* p:{&other->a,&other->b}) {
      op.Work();if(op.Compare(op.Orient(e.a,e.b,*p),op.Integer(0)))continue;
      const auto t=op.Div(op.Sub(use_u?p->u:p->v,use_u?e.a.u:e.a.v),use_u?du:dv);
      if(op.Compare(t,op.Integer(0))>0&&op.Compare(t,op.Integer(1))<0)cuts.push_back(t);
    }
    std::sort(cuts.begin(),cuts.end(),[&](const auto& a,const auto& b){return op.Compare(a,b)<0;});
    cuts.erase(std::unique(cuts.begin(),cuts.end(),[&](const auto& a,const auto& b){return !op.Compare(a,b);}),cuts.end());
    for(size_t k=1;k<cuts.size();++k) {
      auto q=e;q.a=op.Lerp(e.a,e.b,cuts[k-1]);q.b=op.Lerp(e.a,e.b,cuts[k]);
      q.ra=op.Lerp(e.ra,e.rb,cuts[k-1]);q.rb=op.Lerp(e.ra,e.rb,cuts[k]);split.push_back(std::move(q));
    }
  }
  return split;
}
using AtlasEdgeKey=std::pair<AtlasKey,AtlasKey>;
AtlasEdgeKey EdgeKeys(AtlasKey a,AtlasKey b,int& sign){if(b<a){std::swap(a,b);sign=-sign;}return {std::move(a),std::move(b)};}
void AddChainMultiplicity(int& value,int delta) {
  const int limit=std::numeric_limits<int>::max();
  if((delta>0&&value>limit-delta)||(delta<0&&value<-limit-delta))throw std::length_error("Reference coverage edge multiplicity overflow.");
  value+=delta;
}
size_t ReferenceEdgeBytes(const std::vector<AtlasEdge>& edges) {
  size_t bytes=transfer_memory::GrowthBound(edges.size(),sizeof(AtlasEdge));
  for(const auto& e:edges)for(const auto* p:{&e.a,&e.b,&e.ra,&e.rb})for(const auto* q:{&p->u,&p->v})bytes=transfer_memory::Add(bytes,transfer_memory::Bytes(q->numerator),transfer_memory::Bytes(q->denominator));
  return bytes;
}
std::vector<AtlasEdge> CancelReferenceEdges(Id facet,const std::vector<std::pair<const CoveragePiece*,int>>& pieces,AtlasOps& op) {
  auto& budget=op.budget;struct Restore {AtlasBudget& budget;size_t caller;~Restore(){budget.caller_bytes=caller;}} restore{budget,budget.caller_bytes};
  const auto baseline=budget.caller_bytes;size_t retained=0;
  struct ChainEdge {int count;const ExactBary* a;const ExactBary* b;size_t bytes;};std::map<AtlasEdgeKey,ChainEdge> chain;
  for(const auto& record:pieces) {
    const auto& p=*record.first;
    for(unsigned k=0;k<3;++k) {
      op.Work();const auto* a=&p.reference[k];const auto* b=&p.reference[(k+1)%3];
      size_t words=12;for(const auto* point:{a,b})for(const auto* q:{&point->u,&point->v})words=transfer_memory::Add(words,q->numerator.size(),q->denominator.size());
      const auto bytes=transfer_memory::Add(sizeof(decltype(chain)::value_type),4*sizeof(void*),transfer_memory::GrowthBound(words,sizeof(uint64_t)));
      op.Storage(bytes);int sign=record.second;auto ka=op.Key(*a),kb=op.Key(*b);if(kb<ka)std::swap(a,b);
      auto key=EdgeKeys(std::move(ka),std::move(kb),sign);const auto found=chain.find(key);
      if(found==chain.end()){chain.emplace(std::move(key),ChainEdge{sign,a,b,bytes});retained=transfer_memory::Add(retained,bytes);}
      else {AddChainMultiplicity(found->second.count,sign);if(!found->second.count){retained-=found->second.bytes;chain.erase(found);}}
      budget.caller_bytes=transfer_memory::Add(baseline,retained);
    }
  }
  size_t residual_bytes=transfer_memory::GrowthBound(chain.size(),sizeof(AtlasEdge));
  for(const auto& row:chain)for(const auto* p:{row.second.a,row.second.b})for(const auto* q:{&p->u,&p->v})residual_bytes=transfer_memory::Add(residual_bytes,transfer_memory::Bytes(q->numerator),transfer_memory::Bytes(q->denominator));
  residual_bytes=transfer_memory::Add(residual_bytes,transfer_memory::Mul(chain.size(),4*sizeof(uint32_t)));
  op.Storage(residual_bytes);std::vector<AtlasEdge> edges;edges.reserve(chain.size());
  for(const auto& row:chain){const auto& e=row.second;edges.push_back({facet,facet,*e.a,*e.b,{}, {},e.count,0});}
  return edges;
}
// Positive pieces tile the simplex iff their oriented boundary equals its boundary.
// Check shared original paths before canceling coincident interior edges.
std::vector<AtlasEdge> CoverageBoundaryEdges(const std::vector<AtlasEdge>& input,
    const SU2Native3D::PlanarReference& reference,AtlasOps& op) {
  auto& budget=op.budget;const auto saved=budget.caller_bytes;
  struct Restore {AtlasBudget& budget;size_t caller;~Restore(){budget.caller_bytes=caller;}} restore{budget,saved};
  const auto input_bytes=ReferenceEdgeBytes(input);op.Storage(input_bytes);
  budget.caller_bytes=transfer_memory::Add(saved,input_bytes);
  struct Use {int count=0;const AtlasEdge* edge=nullptr;const AtlasEdge* model=nullptr;bool reversed=false;};
  std::map<AtlasEdgeKey,Use> chain;size_t retained=0,maximum_words=32;
  auto words=[](const AtlasEdge& e){size_t n=32;
    for(const auto* p:{&e.a,&e.b,&e.ra,&e.rb})for(const auto* q:{&p->u,&p->v})
      n=transfer_memory::Add(n,q->numerator.size(),q->denominator.size());return n;};
  for(const auto& e:input)maximum_words=std::max(maximum_words,words(e));
  const auto temporary=transfer_memory::Mul(8,transfer_memory::GrowthBound(maximum_words,sizeof(uint64_t)));
  for(const auto& e:input) {
    op.Work();const auto bytes=transfer_memory::Add(sizeof(decltype(chain)::value_type),4*sizeof(void*),
        transfer_memory::Mul(2,transfer_memory::GrowthBound(words(e),sizeof(uint64_t))));
    op.Storage(transfer_memory::Add(bytes,temporary));
    int sign=e.sign;auto a=op.Key(e.a),b=op.Key(e.b);const bool reversed=b<a;
    auto key=EdgeKeys(std::move(a),std::move(b),sign);const auto found=chain.find(key);
    auto* use=found==chain.end()?nullptr:&found->second;
    if(use && use->model && e.triangle!=SIZE_MAX) {
      auto first=ReferenceKey(reference,e.facet,reversed?e.rb:e.ra,op);
      auto last=ReferenceKey(reference,e.facet,reversed?e.ra:e.rb,op);
      const auto& previous=*use->model;
      if(first!=ReferenceKey(reference,previous.facet,use->reversed?previous.rb:previous.ra,op) ||
         last!=ReferenceKey(reference,previous.facet,use->reversed?previous.ra:previous.rb,op))
        throw std::invalid_argument("Curved face pieces disagree on an internal reference path.");
    }
    if(!use) {
      use=&chain.emplace(std::move(key),Use{}).first->second;
      use->edge=&e;retained=transfer_memory::Add(retained,bytes);
      budget.caller_bytes=transfer_memory::Add(saved,input_bytes,retained);
    }
    if(e.triangle!=SIZE_MAX && !use->model){use->model=&e;use->reversed=reversed;}
    AddChainMultiplicity(use->count,sign);
  }
  // Keep zero-count records until every use has passed the path check above.
  size_t residual=0;for(const auto& row:chain)residual+=row.second.count!=0;
  auto output_bytes=transfer_memory::GrowthBound(residual,sizeof(AtlasEdge));
  for(const auto& row:chain)if(row.second.count) {
    const auto& e=*(row.second.model?row.second.model:row.second.edge);
    for(const auto* p:{&e.a,&e.b,&e.ra,&e.rb})for(const auto* q:{&p->u,&p->v})
      output_bytes=transfer_memory::Add(output_bytes,transfer_memory::Bytes(q->numerator),transfer_memory::Bytes(q->denominator));
  }
  op.Storage(transfer_memory::Add(output_bytes,temporary));std::vector<AtlasEdge> out;out.reserve(residual);
  for(const auto& row:chain)if(row.second.count) {
    auto edge=*(row.second.model?row.second.model:row.second.edge);
    if(op.Key(edge.b)<op.Key(edge.a)){std::swap(edge.a,edge.b);std::swap(edge.ra,edge.rb);}
    edge.sign=row.second.count;out.push_back(std::move(edge));
  }
  return out;
}
std::array<Box,10> RestrictNet(const MappedFacetReference& geometry,Id facet,const std::array<ExactBary,3>& r,AtlasOps& op) {
  std::array<Box,3> parameter;for(unsigned k=0;k<3;++k){op.Bary(r[k]);parameter[k]=op.Parameter(r[k]);}
  const auto found=geometry.Curves().find(facet);const auto& f=geometry.Reference().Get(facet);
  std::array<Box,10> out;
  for(unsigned q=0;q<10;++q){std::array<Box,3> a;unsigned at=0;for(unsigned k=0;k<3;++k)for(unsigned n=0;n<CubicIndex[q][k];++n)a[at++]=parameter[k];
    if(found!=geometry.Curves().end())out[q]=Blossom(found->second,a);
    else {
      Box p{};for(const auto& argument:a)for(unsigned d=0;d<3;++d)p[d]=Add(p[d],Multiply(argument[d],Range{std::nextafter(1.L/3,-INFINITY),std::nextafter(1.L/3,INFINITY)}));
      out[q]=LinearPoint({f.v[0].p,f.v[1].p,f.v[2].p},p);
    }
  }
  return out;
}
Range SqrtRange(Range a){if(a.hi<0)throw std::invalid_argument("Negative atlas norm enclosure.");return {std::nextafter(std::sqrt(std::max(0.L,a.lo)),-INFINITY),std::nextafter(std::sqrt(std::max(0.L,a.hi)),INFINITY)};}
Range CosineDegrees(double angle) {
  // Taylor remainder on [0,pi], with every coefficient/operation enclosed.
  const long double pi=3.141592653589793238462643383279502884L;
  const auto x=DividePositive(Multiply(Range{std::nextafter(pi,-INFINITY),std::nextafter(pi,INFINITY)},angle),180);
  const auto square=Multiply(x,x);Range sum{1,1},term{1,1};
  for(unsigned k=1;k<=24;++k){term=DividePositive(Multiply(term,square),(2*k-1)*(2*k));sum=k%2?Subtract(sum,term):Add(sum,term);}
  term=DividePositive(Multiply(term,square),49*50);return Add(sum,{-term.hi,term.hi});
}
Box EdgeNormal(const MappedFacetReference& geometry,Id facet,SU2Native3D::Edge edge,Range t) {
  const auto& f=geometry.Reference().Get(facet);
  const auto found=geometry.Curves().find(facet);
  if(found==geometry.Curves().end())return CrossBox(Difference(Exact(f.v[1].p),Exact(f.v[0].p)),Difference(Exact(f.v[2].p),Exact(f.v[0].p)));
  std::array<Box,10> controls;for(unsigned q=0;q<10;++q)controls[q]=Exact(found->second[q]);
  const auto normal=CubicNormalControls(controls);std::array<Box,5> boundary;
  unsigned first=0,last=0;for(unsigned k=0;k<3;++k){if(f.v[k].id==edge[0])first=k;if(f.v[k].id==edge[1])last=k;}
  for(unsigned q=0;q<5;++q){unsigned power[3]={};power[first]=4-q;power[last]=q;boundary[q]=normal[PolynomialIndex(4,power[0],power[1])];}
  // Restrict the quartic normal itself: independent boxes for t and 1-t lose the edge's correlations.
  Box result;for(auto& d:result)d={INFINITY,-INFINITY};
  for(unsigned q=0;q<5;++q) {
    auto level=boundary;
    for(unsigned step=0;step<4;++step){const long double s=step<4-q?t.lo:t.hi;
      for(unsigned k=0;k<4-step;++k)for(unsigned d=0;d<3;++d)level[k][d]=Add(Multiply(level[k][d],Subtract({1,1},{s,s})),Multiply(level[k+1][d],s));}
    for(unsigned d=0;d<3;++d){result[d].lo=std::min(result[d].lo,level[0][d].lo);result[d].hi=std::max(result[d].hi,level[0][d].hi);}
  }
  return result;
}
bool NormalAngleBound(const Box& a,const Box& b,Range cosine) {
  const auto aa=DotBox(a,a),bb=DotBox(b,b);if(!(aa.lo>0&&bb.lo>0))return false;
  const auto bound=Multiply(cosine,SqrtRange(Multiply(aa,bb)));return DotBox(a,b).lo>=bound.hi;
}
std::array<Box,10> RestrictBoxes(const std::array<Box,10>& net,const std::array<Box,3>& arguments) {
  std::array<Box,10> result;
  auto blossom=[&](const std::array<Box,3>& a){std::array<Box,10> level{};
    for(unsigned q=0;q<10;++q)level[PolynomialIndex(3,CubicIndex[q][0],CubicIndex[q][1])]=net[q];
    for(unsigned degree=3;degree;--degree){std::array<Box,10> next{};const auto& p=a[3-degree];
      for(unsigned i=0;i<degree;++i)for(unsigned j=0;i+j<degree;++j)for(unsigned d=0;d<3;++d)
        next[PolynomialIndex(degree-1,i,j)][d]=Add(Add(Multiply(p[0],level[PolynomialIndex(degree,i+1,j)][d]),Multiply(p[1],level[PolynomialIndex(degree,i,j+1)][d])),Multiply(p[2],level[PolynomialIndex(degree,i,j)][d]));level=next;}
    return level[0];};
  for(unsigned q=0;q<10;++q){std::array<Box,3> a;unsigned at=0;for(unsigned k=0;k<3;++k)for(unsigned n=0;n<CubicIndex[q][k];++n)a[at++]=arguments[k];result[q]=blossom(a);}
  return result;
}
void PositiveProjectedNet(const std::array<Box,10>& net,const Box& normal,AtlasOps& op) {
  using Part=std::array<Box,10>;std::vector<Part> pending;
  auto reserve=[&](size_t needed) {
    if(needed>pending.capacity()) {
      const auto next=std::max(needed,transfer_memory::Mul(pending.capacity(),2));
      op.Storage(transfer_memory::Mul(transfer_memory::Add(pending.capacity(),next),sizeof(Part)));
      pending.reserve(next);
    }
    op.Storage(transfer_memory::Mul(pending.capacity(),sizeof(Part)));
  };
  reserve(1);pending.push_back(net);
  const std::array<Box,3> left{Exact({1,0,0}),Exact({.5,.5,0}),Exact({.5,0,.5})},
      right{Exact({.5,.5,0}),Exact({0,1,0}),Exact({0,.5,.5})},
      top{Exact({.5,0,.5}),Exact({0,.5,.5}),Exact({0,0,1})},
      middle{Exact({.5,.5,0}),Exact({0,.5,.5}),Exact({.5,0,.5})};
  while(!pending.empty()) {
    op.Work();const auto part=pending.back();pending.pop_back();bool positive=true,nonpositive=true;
    const auto controls=CubicNormalControls(part);
    for(const auto& n:controls){const auto r=DotBox(n,normal);positive=positive&&r.lo>0;nonpositive=nonpositive&&r.hi<=0;}
    // Quartic vertex controls are actual corner normals: one nonpositive corner refutes positivity.
    for(unsigned k=0;k<3;++k)if(DotBox(controls[PolynomialIndex(4,k==0?4:0,k==1?4:0)],normal).hi<=0)
      throw std::invalid_argument("Curved region has nonpositive projected Jacobian.");
    if(positive)continue;if(nonpositive)throw std::invalid_argument("Curved region has nonpositive projected Jacobian.");
    reserve(transfer_memory::Add(pending.size(),4));
    pending.push_back(RestrictBoxes(part,left));pending.push_back(RestrictBoxes(part,right));pending.push_back(RestrictBoxes(part,top));pending.push_back(RestrictBoxes(part,middle));
  }
}
using CurveBoxes=std::array<Box,4>;
std::pair<CurveBoxes,CurveBoxes> BisectCurve(const CurveBoxes& curve) {
  auto average=[](const Box& a,const Box& b){Box out;for(unsigned d=0;d<3;++d)out[d]=Multiply(Add(a[d],b[d]),.5L);return out;};
  const auto a=average(curve[0],curve[1]),b=average(curve[1],curve[2]),c=average(curve[2],curve[3]);
  const auto d=average(a,b),e=average(b,c),f=average(d,e);return {{curve[0],a,d,f},{f,e,c,curve[3]}};
}
Box MidpointDirection(const Box& box) {Box out;for(unsigned d=0;d<3;++d){const double x=double((box[d].lo+box[d].hi)/2);out[d]={x,x};}return out;}
bool MonotoneCurve(const CurveBoxes& curve) {
  const auto direction=MidpointDirection(Difference(curve[3],curve[0]));
  for(unsigned k=0;k<3;++k)if(!(DotBox(Difference(curve[k+1],curve[k]),direction).lo>0))return false;
  return true;
}
bool CurveHullsSeparated(const CurveBoxes& a,const CurveBoxes& b) {
  for(unsigned axis=0;axis<2;++axis) {
    long double alo=INFINITY,ahi=-INFINITY,blo=INFINITY,bhi=-INFINITY;
    for(const auto& p:a){alo=std::min(alo,p[axis].lo);ahi=std::max(ahi,p[axis].hi);}
    for(const auto& p:b){blo=std::min(blo,p[axis].lo);bhi=std::max(bhi,p[axis].hi);}
    if(ahi<blo||bhi<alo)return true;
  }
  return false;
}
bool EndpointConesSeparated(const CurveBoxes& a,const CurveBoxes& b) {
  auto unit=[](const Box& p){auto out=MidpointDirection(p);const double norm=std::hypot(double(out[0].lo),double(out[1].lo));
    if(!(norm>0&&std::isfinite(norm)))return Box{};for(unsigned d=0;d<2;++d){const double x=double(out[d].lo)/norm;out[d]={x,x};}return out;};
  const auto first=unit(Difference(a[2],a[3])),second=unit(Difference(b[1],b[0]));
  const auto line=MidpointDirection(Difference(first,second));
  // Factoring endpoint displacement leaves positive combinations of these three vectors.
  for(unsigned k=0;k<3;++k)if(!(DotBox(Difference(a[k],a[3]),line).lo>0))return false;
  for(unsigned k=1;k<4;++k)if(!(DotBox(Difference(b[k],b[0]),line).hi<0))return false;
  return true;
}
void SeparateCurvePair(const CurveBoxes& a,const CurveBoxes& b,bool adjacent,AtlasOps& op) {
  struct Pair {CurveBoxes a,b;bool adjacent;};std::vector<Pair> pending{{a,b,adjacent}};
  while(!pending.empty()) {
    op.Work();auto p=pending.back();pending.pop_back();
    if(p.adjacent?EndpointConesSeparated(p.a,p.b):CurveHullsSeparated(p.a,p.b))continue;
    auto width=[](const CurveBoxes& c){long double result=0;for(unsigned d=0;d<2;++d){long double lo=INFINITY,hi=-INFINITY;for(const auto& p:c){lo=std::min(lo,p[d].lo);hi=std::max(hi,p[d].hi);}result=std::max(result,hi-lo);}return result;};
    op.Storage(transfer_memory::Mul(pending.size()+2,sizeof(Pair)));
    if(width(p.a)>=width(p.b)) {
      const auto split=BisectCurve(p.a);pending.push_back({split.first,p.b,false});pending.push_back({split.second,p.b,p.adjacent});
    } else {
      const auto split=BisectCurve(p.b);pending.push_back({p.a,split.first,p.adjacent});pending.push_back({p.a,split.second,false});
    }
  }
}
SU2Native2D::Point Flat(Point p){return {p.x,p.y};}
}
AffineChart::AffineChart(AffineAuthority value):authority(value) {
  Finite(value.origin);Finite(value.normal);
  if((value.dimension!=2 && value.dimension!=3) || !value.version ||
     !(std::isfinite(value.maximum_error)&&value.maximum_error>=std::numeric_limits<double>::min()))throw std::invalid_argument("Invalid affine authority version/dimension/error contract.");
  const auto n=Values(value.normal);
  for(unsigned k=1;k<value.dimension;++k)if(std::abs(n[k])>std::abs(n[dependent]))dependent=k;
  if(n[dependent]==0 || (value.dimension==2 && (value.normal.z!=0 || value.origin.z!=0)))
    throw std::invalid_argument("Invalid affine normal or 2D chart embedding.");
  if(!std::numeric_limits<long double>::is_iec559 || std::numeric_limits<long double>::round_style!=std::round_to_nearest)
    throw std::invalid_argument("Affine geometry requires IEEE round-to-nearest wide arithmetic.");
}
Point AffineChart::Coordinates(Point p)const {
  Finite(p);if(authority.dimension==2 && p.z!=0)throw std::invalid_argument("2D affine query leaves its embedding.");
  const auto q=Values(p);Vec flat{};unsigned j=0;
  for(unsigned k=0;k<authority.dimension;++k)if(k!=dependent)flat[j++]=q[k];return ToPoint(flat);
}
double AffineChart::ErrorBound(Point p)const {
  Finite(p);if(authority.dimension==2 && p.z!=0)return INFINITY;
  const auto q=Values(p),o=Values(authority.origin),n=Values(authority.normal);Range residual;
  for(unsigned k=0;k<authority.dimension;++k)residual=Add(residual,Multiply(Subtract(q[k],o[k]),n[k]));
  // ||n||_infinity <= ||n||_2, so this is a conservative physical-distance bound.
  const auto bound=std::nextafter(std::max(std::abs(residual.lo),std::abs(residual.hi))/std::abs(n[dependent]),INFINITY);
  if(!std::isfinite(bound))return INFINITY;
  // A normal output floor remains conservative even when the process flushes subnormals.
  const auto result=std::nextafter(double(std::max(bound,static_cast<long double>(std::numeric_limits<double>::min()))),INFINITY);
  return std::isfinite(result)?result:INFINITY;
}
Point AffineChart::Lift(Point flat)const {
  Finite(flat);if(flat.z!=0 || (authority.dimension==2 && flat.y!=0))throw std::invalid_argument("Invalid affine chart coordinate.");
  const auto f=Values(flat),o=Values(authority.origin),n=Values(authority.normal);Vec q=o;unsigned j=0;long double sum=0;
  for(unsigned k=0;k<authority.dimension;++k)if(k!=dependent){q[k]=f[j++];sum+=static_cast<long double>(n[k])*(static_cast<long double>(q[k])-o[k]);}
  q[dependent]=double(static_cast<long double>(o[dependent])-sum/n[dependent]);const auto p=ToPoint(q);
  if(ErrorBound(p)>authority.maximum_error)throw std::invalid_argument("Affine lift cannot satisfy its declared geometry error contract.");
  return p;
}
Point AffineChart::Project(Point p)const {
  Finite(p);const auto q=Values(p),o=Values(authority.origin),n=Values(authority.normal);
  if(authority.dimension==2 && p.z!=0)throw std::invalid_argument("2D affine projection leaves its embedding.");
  if(ErrorBound(p)<=authority.maximum_error)return p;
  long double residual=0,square=0;const long double scale=std::abs(n[dependent]);
  for(unsigned k=0;k<authority.dimension;++k){const long double a=n[k]/scale;residual+=a*(static_cast<long double>(q[k])-o[k]);square+=a*a;}
  Vec foot=q;for(unsigned k=0;k<authority.dimension;++k)foot[k]=double(static_cast<long double>(q[k])-(n[k]/scale)*residual/square);
  return Lift(Coordinates(ToPoint(foot)));
}
double DisplacementBound(Point a,Point b) {
  Finite(a);Finite(b);const auto x=Values(a),y=Values(b);Range bound;
  if(x==y)return 0;
  for(unsigned k=0;k<3;++k) {
    const auto difference=Subtract(x[k],y[k]);
    bound=Add(bound,{0,std::max(std::abs(difference.lo),std::abs(difference.hi))});
  }
  return std::nextafter(double(bound.hi),INFINITY);
}
Point ProjectIntersection(Point point,const std::vector<AffineAuthority>& constraints) {
  Finite(point);if(constraints.empty())return point;
  std::vector<AffineChart> charts;bool satisfied=true;
  for(const auto& g:constraints) {
    charts.emplace_back(g);
    if(g.dimension!=constraints.front().dimension)throw std::invalid_argument("Conflicting affine constraint dimensions.");
    satisfied=satisfied && charts.back().ErrorBound(point)<=g.maximum_error;
  }
  if(satisfied)return point;
  if(charts.size()==1)return charts.front().Project(point);
  if(charts.size()>constraints.front().dimension)
    throw std::invalid_argument("Unsupported dependent affine junction constraints; no coordinates changed.");
  using Wide=std::array<long double,3>;std::array<Wide,3> basis{};std::array<long double,3> offset{};size_t rank=0;
  auto dot=[](Wide a,Wide b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
  for(const auto& g:constraints) {
    const auto n=Values(g.normal),o=Values(g.origin),p=Values(point);Wide row{};
    const auto scale=std::max({std::abs(n[0]),std::abs(n[1]),std::abs(n[2])});long double value=0;
    for(unsigned j=0;j<3;++j){row[j]=static_cast<long double>(n[j])/scale;value+=row[j]*(static_cast<long double>(o[j])-p[j]);}
    const auto length=std::sqrt(dot(row,row));for(auto& x:row)x/=length;value/=length;
    for(unsigned pass=0;pass<2;++pass)for(size_t k=0;k<rank;++k) {
      const auto factor=dot(row,basis[k]);value-=factor*offset[k];
      for(unsigned j=0;j<3;++j)row[j]-=factor*basis[k][j];
    }
    const auto independent=std::sqrt(dot(row,row));
    // Reject an unresolved intersection; preserving each authority is different from merging nearby planes.
    if(!(independent>1e-12L))throw std::invalid_argument("Conflicting/dependent or ill-conditioned affine junction constraints.");
    for(auto& x:row)x/=independent;basis[rank]=row;offset[rank++]=value/independent;
  }
  auto p=Values(point);Wide shift{};
  for(size_t k=0;k<rank;++k)for(unsigned j=0;j<3;++j)shift[j]+=basis[k][j]*offset[k];
  for(unsigned j=0;j<3;++j)p[j]=double(static_cast<long double>(p[j])+shift[j]);
  const auto candidate=ToPoint(p);
  for(size_t k=0;k<charts.size();++k)if(charts[k].ErrorBound(candidate)>constraints[k].maximum_error)
    throw std::invalid_argument("Simultaneous affine projection cannot meet its declared mapping errors.");
  return candidate;
}
Point SourceColumn(Point origin,const std::vector<std::array<Point,3>>& faces,double radius,
                   uint64_t& work,uint64_t maximum_work) {
  Finite(origin);
  if(!(std::isfinite(radius)&&radius>=std::numeric_limits<double>::min()) || faces.empty())
    throw std::invalid_argument("Invalid original-source column radius/skin.");
  using Wide=std::array<long double,3>;std::vector<Wide> rows;
  auto dot=[](Wide a,Wide b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
  for(const auto& f:faces) {
    for(auto p:f)Finite(p);
    const auto a=Values(f[0]),b=Values(f[1]),c=Values(f[2]);Wide u{},v{};
    for(unsigned k=0;k<3;++k){u[k]=static_cast<long double>(b[k])-a[k];v[k]=static_cast<long double>(c[k])-a[k];}
    Wide n{{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]}};
    const auto length=std::sqrt(dot(n,n));if(!(length>0 && std::isfinite(length)))
      throw std::invalid_argument("Degenerate original-source boundary face.");
    for(auto& x:n)x/=length;
    // Identical solver rows are redundant; no facets/authorities are merged.
    if(std::find(rows.begin(),rows.end(),n)==rows.end())rows.push_back(n);
    if(rows.size()>64)throw std::length_error("Original-source normal-cone constraint resource ceiling exceeded.");
  }
  Wide best{};long double square=INFINITY;
  auto trial=[&](std::array<size_t,3> active,size_t count) {
    if(work>=maximum_work)throw std::length_error("Original-source column work ceiling exceeded.");++work;
    std::array<Wide,3> basis{};std::array<long double,3> offset{};
    for(size_t i=0;i<count;++i) {
      Wide row=rows[active[i]];long double value=1;
      for(unsigned pass=0;pass<2;++pass)for(size_t j=0;j<i;++j) {
        const auto factor=dot(row,basis[j]);value-=factor*offset[j];
        for(unsigned k=0;k<3;++k)row[k]-=factor*basis[j][k];
      }
      const auto length=std::sqrt(dot(row,row));if(!(length>1e-12L))return;
      for(auto& x:row)x/=length;basis[i]=row;offset[i]=value/length;
    }
    Wide proposal{};for(size_t i=0;i<count;++i)for(unsigned k=0;k<3;++k)proposal[k]+=basis[i][k]*offset[i];
    for(auto row:rows)if(dot(row,proposal)<1-256*std::numeric_limits<long double>::epsilon())return;
    const auto norm=dot(proposal,proposal);if(std::isfinite(norm)&&norm<square){best=proposal;square=norm;}
  };
  for(size_t i=0;i<rows.size();++i) {
    trial({i,0,0},1);
    for(size_t j=i+1;j<rows.size();++j) {
      trial({i,j,0},2);
      for(size_t k=j+1;k<rows.size();++k)trial({i,j,k},3);
    }
  }
  if(!std::isfinite(square))throw std::invalid_argument("No admitted original-source outward normal cone.");
  auto result=Values(origin);
  for(unsigned k=0;k<3;++k)result[k]=double(static_cast<long double>(result[k])+4*static_cast<long double>(radius)*best[k]);
  const auto p=ToPoint(result);Finite(p);if(Values(p)==Values(origin))
    throw std::invalid_argument("Unrepresentable original-source column.");
  return p;
}
bool SourceFacesSeparated(const std::array<Point,3>& a,const std::array<Point,3>& b,double distance) {
  if(!(std::isfinite(distance)&&distance>0))throw std::invalid_argument("Invalid original-source separation distance.");
  for(auto p:a)Finite(p);for(auto p:b)Finite(p);
  using Box=std::array<Range,3>;
  auto subtract=[](Range a,Range b){return Add(a,{-b.hi,-b.lo});};
  auto multiply=[](Range a,Range b) {
    const std::array<long double,4> p{{a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi}};
    return Range{std::nextafter(*std::min_element(p.begin(),p.end()),-INFINITY),
                 std::nextafter(*std::max_element(p.begin(),p.end()),INFINITY)};
  };
  auto delta=[](Point a,Point b){const auto x=Values(a),y=Values(b);Box v;
    for(unsigned k=0;k<3;++k)v[k]=Subtract(x[k],y[k]);return v;};
  auto cross=[&](Box a,Box b){return Box{{subtract(multiply(a[1],b[2]),multiply(a[2],b[1])),
      subtract(multiply(a[2],b[0]),multiply(a[0],b[2])),subtract(multiply(a[0],b[1]),multiply(a[1],b[0]))}};};
  auto dot=[&](Box a,Box b){Range sum;for(unsigned k=0;k<3;++k)sum=Add(sum,multiply(a[k],b[k]));return sum;};
  auto separated=[&](Box axis) {
    long double norm=0;for(auto x:axis)norm=std::nextafter(norm+std::max(std::abs(x.lo),std::abs(x.hi)),INFINITY);
    if(!(norm>0 && std::isfinite(norm)))return false;
    long double alo=INFINITY,ahi=-INFINITY,blo=INFINITY,bhi=-INFINITY;
    for(auto p:a){const auto q=dot(axis,delta(p,a[0]));alo=std::min(alo,q.lo);ahi=std::max(ahi,q.hi);}
    for(auto p:b){const auto q=dot(axis,delta(p,a[0]));blo=std::min(blo,q.lo);bhi=std::max(bhi,q.hi);}
    const auto gap=std::max(std::nextafter(blo-ahi,-INFINITY),std::nextafter(alo-bhi,-INFINITY));
    // ||axis||_2 <= ||axis||_1; every operation encloses its real value.
    const auto required=std::nextafter(static_cast<long double>(distance)*norm,INFINITY);
    return std::isfinite(gap)&&std::isfinite(required)&&gap>=required;
  };
  for(unsigned k=0;k<3;++k){Box axis{};axis[k]={1,1};if(separated(axis))return true;}
  std::array<Box,3> ea,eb;
  for(unsigned k=0;k<3;++k){ea[k]=delta(a[(k+1)%3],a[k]);eb[k]=delta(b[(k+1)%3],b[k]);}
  if(separated(cross(ea[0],ea[1])) || separated(cross(eb[0],eb[1])))return true;
  for(auto x:ea)for(auto y:eb)if(separated(cross(x,y)))return true;
  return false;
}
namespace {
double PointModelError(const SU2Native3D::ParametricPoint& model,Point p) {
  Finite(p);const auto box=SU2Native3D::ParametricPointBounds(model);
  const auto q=Values(p),lo=Values(box[0]),hi=Values(box[1]);
  if(q==lo && q==hi)return 0;
  Range total;
  for(unsigned k=0;k<3;++k) {
    const auto a=Subtract(q[k],lo[k]),b=Subtract(q[k],hi[k]);
    total=Add(total,{0,std::max({std::abs(a.lo),std::abs(a.hi),std::abs(b.lo),std::abs(b.hi)})});
  }
  return std::nextafter(double(std::max(total.hi,static_cast<long double>(std::numeric_limits<double>::min()))),INFINITY);
}
}
struct MappedFacetReference::ComponentData {
  struct Entry {
    std::vector<Id> facets;
    std::vector<SU2Native3D::Edge> border;
    std::vector<std::array<Id,3>> corners;
    std::map<Id,bool> features;
    std::shared_ptr<CADTElemClass> search;
    unsigned axis=0;
    int orientation=0;
    bool convex=false;
    double limit=0;
  };
  std::map<Id,Entry> entries;
};
struct MappedFacetReference::CurvedSearch {
  struct Entry {std::vector<Id> facets;std::shared_ptr<CADTElemClass> index;};
  std::map<Id,Id> root;
  std::map<Id,Entry> entries;
  std::mutex mutex; // House ADT traversal buffers are shared by concurrent non-OpenMP callers.
};
namespace {
struct CurvedQueryRegion {Id facet;std::array<Point,3> parameter;double lower,clearance;unsigned depth;};
Point ComponentFlat(Point p,unsigned axis){return axis==0?Point{p.y,p.z,0}:axis==1?Point{p.x,p.z,0}:Point{p.x,p.y,0};}
SU2Native3D::ParametricPoint ComponentFlat(SU2Native3D::ParametricPoint p,unsigned axis) {
  p.origin=ComponentFlat(p.origin,axis);p.first=ComponentFlat(p.first,axis);p.second=ComponentFlat(p.second,axis);return p;
}
double PlaneModelOffset(const SU2Native3D::ParametricPoint& model,const AffineAuthority& plane) {
  if(!SU2Native3D::ParametricPlaneSide(model,plane.origin,plane.normal))return 0;
  const auto bounds=SU2Native3D::ParametricPointBounds(model);const auto lo=Values(bounds[0]),hi=Values(bounds[1]),o=Values(plane.origin),n=Values(plane.normal);
  Range residual;unsigned axis=0;
  for(unsigned k=0;k<3;++k){residual=Add(residual,Multiply({Subtract(lo[k],o[k]).lo,Subtract(hi[k],o[k]).hi},n[k]));if(std::abs(n[k])>std::abs(n[axis]))axis=k;}
  const auto bound=std::nextafter(std::max(std::abs(residual.lo),std::abs(residual.hi))/std::abs(n[axis]),INFINITY);
  return std::nextafter(double(std::max(bound,static_cast<long double>(std::numeric_limits<double>::min()))),INFINITY);
}
}
struct MappedFacetReference::FaceMemo {
  static constexpr size_t Entries=size_t(1)<<14;
  using Key=std::array<uint64_t,25>;
  struct Hash{size_t operator()(const Key& k)const{uint64_t h=1469598103934665603ULL;for(auto x:k){h^=x+0x9e3779b97f4a7c15ULL+(h<<6)+(h>>2);}return size_t(h);}};
  static Key Query(Id facet,const std::array<SurfaceBinding,3>& binding,const std::array<Point,3>& realized) {
    auto bits=[](double x){uint64_t b;std::memcpy(&b,&x,sizeof b);return b;};
    Key key{};size_t at=0;key[at++]=facet;
    for(unsigned k=0;k<3;++k){key[at++]=binding[k].feature;key[at++]=binding[k].version;key[at++]=binding[k].dimension;key[at++]=bits(binding[k].u);key[at++]=bits(binding[k].v);}
    for(unsigned k=0;k<3;++k){key[at++]=bits(realized[k].x);key[at++]=bits(realized[k].y);key[at++]=bits(realized[k].z);}
    return key;
  }
  // Components and curved nets are mutually exclusive: store containment or surface error respectively.
  std::mutex lock;std::unordered_map<Key,double,Hash> values;
};
MappedFacetReference::MappedFacetReference(std::shared_ptr<const SU2Native3D::PlanarReference> input,
    const std::map<Id,double>& mapping_errors,uint64_t definition_version,size_t ceiling,bool reference_components,const CurvedNets& curved_nets)
    :reference(std::move(input)),version(definition_version) {
  using namespace SU2Native3D;
  if(!reference || !version || mapping_errors.empty())throw std::invalid_argument("Invalid mapped-facet reference/version/declaration.");
  const auto bound=transfer_memory::Mul(reference->Facets().size(),4096);
  if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Mapped-facet preparation exceeds storage admission.");
  for(const auto& row:mapping_errors)
    if(!reference->Facets().count(row.first) || !(std::isfinite(row.second)&&row.second>=std::numeric_limits<double>::min()))
      throw std::invalid_argument("Invalid mapped-facet identity/error declaration.");
  if(!reference->Analytic().empty())throw std::invalid_argument("Mixed analytic/mapped-facet authority is not supported.");
  if(!curved_nets.empty() && reference_components)throw std::invalid_argument("Curved controls require facet-local parameter topology.");
  for(const auto& row:curved_nets) {
    if(!reference->Facets().count(row.first) || !mapping_errors.count(row.first))throw std::invalid_argument("Undeclared curved facet.");
    const auto& f=reference->Get(row.first);
    for(Point p:row.second) {
      Finite(p);const auto plane=reference->Affine().find(row.first);
      if(plane!=reference->Affine().end() && AffineChart(plane->second).ErrorBound(p)>plane->second.maximum_error)
        throw std::invalid_argument("Curved controls leave their declared affine carrier.");
    }
    for(unsigned k=0;k<3;++k)if(Values(row.second[k])!=Values(f.v[k].p))throw std::invalid_argument("Curved control corner differs from immutable facet.");
    if(!PositiveCubicJacobian(row.second))throw std::invalid_argument("Curved patch "+std::to_string(row.first)+" has no certified positive projected Jacobian.");
  }
  curves=curved_nets;
  errors=mapping_errors;
  std::map<Edge,std::vector<Id>> incident;std::vector<Face> skin;
  for(const auto& row:reference->Facets()) {
    const auto& f=row.second;skin.push_back({f.v[0].id,f.v[1].id,f.v[2].id});
    for(unsigned k=0;k<3;++k) {
      nodes.emplace(f.v[k].id,f.v[k].p);vertex_facets[f.v[k].id].push_back(f.id);
      Edge edge{f.v[k].id,f.v[(k+1)%3].id};std::sort(edge.begin(),edge.end());incident[edge].push_back(f.id);
    }
  }
  ValidateDomainSurfaceTopology(skin);
  edges.reserve(incident.size());edge_facets.reserve(incident.size());
  for(auto& row:incident){edges.push_back(row.first);edge_facets.push_back(std::move(row.second));}
  for(const auto& row:reference->Facets())body_roots.emplace(row.first,row.first);
  auto body_root=[&](Id id){while(body_roots.at(id)!=id){body_roots[id]=body_roots.at(body_roots.at(id));id=body_roots.at(id);}return id;};
  for(const auto& row:edge_facets)if(row.size()==2){const Id a=body_root(row[0]),b=body_root(row[1]);body_roots[std::max(a,b)]=std::min(a,b);}
  for(auto& row:body_roots)row.second=body_root(row.first);
  for(size_t e=0;e<edges.size();++e) {
    std::array<Point,2> expected{};bool seen=false,curved=false,flat=false;
    for(Id id:edge_facets[e]) {
      const auto found=curves.find(id);if(found==curves.end()){flat=true;continue;}curved=true;
      const auto& f=reference->Get(id);unsigned k=0;
      while(k<3 && !(std::minmax(f.v[k].id,f.v[(k+1)%3].id)==std::minmax(edges[e][0],edges[e][1])))++k;
      const unsigned controls[3][2]={{3,4},{5,6},{7,8}};
      auto a=found->second[controls[k][0]],b=found->second[controls[k][1]];
      if(f.v[k].id>f.v[(k+1)%3].id)std::swap(a,b);
      if(seen && (Values(a)!=Values(expected[0]) || Values(b)!=Values(expected[1])))throw std::invalid_argument("Curved adjacent patches disagree on shared edge controls.");
      expected={a,b};seen=true;
    }
    if(curved&&flat)throw std::invalid_argument("Curved control net must cover every adjacent facet, including planar patches.");
  }
  if(!curves.empty()) {
    auto data=std::make_shared<CurvedSearch>();
    for(const auto& row:curves)data->root.emplace(row.first,row.first);
    auto root=[&](Id id){while(data->root.at(id)!=id){data->root[id]=data->root.at(data->root.at(id));id=data->root.at(id);}return id;};
    for(const auto& row:edge_facets)if(row.size()==2 && curves.count(row[0]) && curves.count(row[1]) &&
        reference->Get(row[0]).marker==reference->Get(row[1]).marker) {
      const auto a=root(row[0]),b=root(row[1]);data->root[std::max(a,b)]=std::min(a,b);
    }
    for(auto& row:data->root){row.second=root(row.first);data->entries[row.second].facets.push_back(row.first);}
    size_t prepared=bound;
    for(auto& row:data->entries) {
      auto& entry=row.second;const size_t count=entry.facets.size();if(count<=16)continue;
      size_t retained=0,peak=0;CADTElemClass::PredictBytes(3,2*count,count,2*count,&retained,&peak);
      const auto workspace=transfer_memory::Mul(2*std::max<size_t>(200,count+1),sizeof(CBBoxTargetClass)+2*sizeof(unsigned long));
      const auto requested=transfer_memory::Add(prepared,std::max(retained,peak),workspace,transfer_memory::Mul(count,128));
      if(requested==SIZE_MAX||requested>ceiling)throw std::length_error("Curved component index exceeds storage admission.");
      prepared=transfer_memory::Add(prepared,retained,workspace);
      std::vector<su2double> coordinates;std::vector<unsigned long> connectivity,ids;
      coordinates.reserve(6*count);connectivity.reserve(2*count);ids.reserve(count);
      for(Id id:entry.facets) {
        auto lo=Values(curves.at(id)[0]),hi=lo;
        for(Point p:curves.at(id))for(unsigned d=0;d<3;++d){lo[d]=std::min(lo[d],Values(p)[d]);hi[d]=std::max(hi[d],Values(p)[d]);}
        coordinates.insert(coordinates.end(),{lo[0],lo[1],lo[2],hi[0],hi[1],hi[2]});
        connectivity.push_back(connectivity.size());connectivity.push_back(connectivity.size());ids.push_back(ids.size());
      }
      std::vector<unsigned short> types(count,LINE),markers(count,0);
      entry.index=std::make_shared<CADTElemClass>(3,coordinates,connectivity,types,markers,ids,false);
    }
    curved_search=std::move(data);
    if(ResidentBytes()>prepared)throw std::length_error("Curved component storage model exhausted before publication.");
  }
  if(reference_components) {
    auto data=std::make_shared<ComponentData>();size_t prepared=bound;
    for(const auto& row:reference->Facets())data->entries[reference->Canonical(row.first)].facets.push_back(row.first);
    for(auto& row:data->entries) {
      auto& c=row.second;const auto& root=reference->Get(row.first);std::map<Edge,std::vector<Edge>> uses;
      c.limit=INFINITY;
      for(Id id:c.facets) {
        const auto error=errors.find(id);c.limit=std::min(c.limit,error==errors.end()?0.:error->second);
        const auto& f=reference->Get(id);
        for(unsigned k=0;k<3;++k){Edge e{f.v[k].id,f.v[(k+1)%3].id},key=e;std::sort(key.begin(),key.end());uses[key].push_back(e);}
      }
      const auto plane=reference->Affine().find(row.first);
      if(plane!=reference->Affine().end()) {
        const auto n=Values(plane->second.normal);for(unsigned k=1;k<3;++k)if(std::abs(n[k])>std::abs(n[c.axis]))c.axis=k;
        c.limit=std::min(c.limit,plane->second.maximum_error);
        if(!(c.limit>0))throw std::invalid_argument("Affine component consolidation requires an explicit mapping bound on every member facet.");
      } else {
        const auto a=Values(root.v[0].p),b=Values(root.v[1].p),d=Values(root.v[2].p);std::array<long double,3> n{};
        for(unsigned k=0;k<3;++k){const unsigned i=(k+1)%3,j=(k+2)%3;n[k]=(static_cast<long double>(b[i])-a[i])*(static_cast<long double>(d[j])-a[j])-(static_cast<long double>(b[j])-a[j])*(static_cast<long double>(d[i])-a[i]);}
        for(unsigned k=1;k<3;++k)if(std::abs(n[k])>std::abs(n[c.axis]))c.axis=k;
      }
      c.orientation=ParametricOrientation({root.v[0].p},{root.v[1].p},{root.v[2].p},c.axis);
      if(!c.orientation)throw std::invalid_argument("Degenerate mapped component chart.");
      std::map<Id,Id> next,previous;
      for(const auto& use:uses) {
        if(use.second.size()==1){const auto e=use.second.front();c.border.push_back(e);
          if(!next.emplace(e[0],e[1]).second||!previous.emplace(e[1],e[0]).second)throw std::invalid_argument("Branched mapped component border.");}
        else if(use.second.size()!=2||use.second[0][0]!=use.second[1][1]||use.second[0][1]!=use.second[1][0])throw std::invalid_argument("Inconsistent mapped component orientation.");
      }
      if(next.empty()||next.size()!=previous.size())throw std::invalid_argument("Mapped planar component has no complete border.");
      for(auto e:c.border){const bool forward=e[0]<e[1];std::sort(e.begin(),e.end());c.features.emplace(EdgeIdentity(e),forward);}
      std::set<Id> visited;unsigned loops=0;c.convex=true;
      for(const auto& edge:next) {
        const Id v=edge.first;const auto p=previous.find(v);if(p==previous.end())throw std::invalid_argument("Open mapped component border.");
        c.corners.push_back({p->second,v,edge.second});
        c.convex=c.convex && c.orientation*ParametricOrientation({nodes.at(p->second)},{nodes.at(v)},{nodes.at(edge.second)},c.axis)>=0;
        if(visited.count(v))continue;++loops;Id walk=v;
        do{if(!visited.insert(walk).second)throw std::invalid_argument("Mapped component loops intersect.");walk=next.at(walk);}while(walk!=v);
      }
      c.convex=c.convex && loops==1;
      // Small certified components need no per-component tree or fixed ADT workspace.
      if(c.facets.size()>16) {
        const auto count=c.facets.size();size_t retained=0,peak=0;
        CADTElemClass::PredictBytes(2,2*count,count,2*count,&retained,&peak);
        const auto workspace=transfer_memory::Mul(2*std::max<size_t>(200,count+1),sizeof(CBBoxTargetClass)+2*sizeof(unsigned long));
        const auto results=transfer_memory::Mul(count,3*sizeof(Id));
        const auto requested=transfer_memory::Add(prepared,std::max(retained,peak),workspace,results);
        if(requested==SIZE_MAX||requested>ceiling)throw std::length_error("Mapped component index exceeds storage admission.");
        prepared=transfer_memory::Add(prepared,retained,workspace,results);
        std::vector<su2double> coordinates;std::vector<unsigned long> connectivity,ids;
        for(Id id:c.facets) {
          const auto& f=reference->Get(id);auto low=ComponentFlat(f.v[0].p,c.axis),high=low;
          for(auto n:f.v){const auto p=ComponentFlat(n.p,c.axis);low.x=std::min(low.x,p.x);low.y=std::min(low.y,p.y);high.x=std::max(high.x,p.x);high.y=std::max(high.y,p.y);}
          coordinates.insert(coordinates.end(),{low.x,low.y,high.x,high.y});connectivity.push_back(connectivity.size());connectivity.push_back(connectivity.size());ids.push_back(ids.size());
        }
        std::vector<unsigned short> types(ids.size(),LINE),markers(ids.size(),0);
        c.search=std::make_shared<CADTElemClass>(2,coordinates,connectivity,types,markers,ids,false);
      }
    }
    components=std::move(data);
    // The ContainsFace memo is admitted with its full bound (entries, keys, nodes, buckets) or not created at all.
    if(transfer_memory::Add(prepared,FaceMemoBytes())<=ceiling){face_memo=std::make_shared<FaceMemo>();prepared=transfer_memory::Add(prepared,FaceMemoBytes());}
    if(ResidentBytes()>prepared)throw std::length_error("Mapped component storage model exhausted before publication.");
  }
  if(!curves.empty() && transfer_memory::Add(ResidentBytes(),FaceMemoBytes())<=ceiling)
    face_memo=std::make_shared<FaceMemo>();
}
Id MappedFacetReference::Canonical(Id id)const {reference->Get(id);return components?reference->Canonical(id):id;}
const std::vector<Id>& MappedFacetReference::ComponentFacets(Id id)const {
  if(!components)throw std::logic_error("Facet-local mapped authority has no component inventory.");
  return components->entries.at(Canonical(id)).facets;
}
const std::vector<SU2Native3D::Edge>& MappedFacetReference::ComponentBorder(Id id)const {return components->entries.at(Canonical(id)).border;}
unsigned MappedFacetReference::ComponentAxis(Id id)const {return components->entries.at(Canonical(id)).axis;}
std::vector<Id> MappedFacetReference::CandidateFacets(Id id,Point p)const {
  Finite(p);const auto& c=components->entries.at(Canonical(id));if(!c.search)return c.facets;const auto q=ComponentFlat(p,c.axis);
  std::vector<unsigned long> found;const su2double x[2]={q.x,q.y};c.search->DetermineIntersectingElements(x,x,found);
  std::vector<Id> result;result.reserve(found.size());for(auto index:found)result.push_back(c.facets.at(index));std::sort(result.begin(),result.end());return result;
}
bool MappedFacetReference::PhysicalSupport(const SurfaceBinding& binding)const {
  CheckBinding(binding);if(!components)return binding.dimension<2;
  std::set<Id> roots;ForFacets(binding,[&](Id id){roots.insert(Canonical(id));});return roots.size()>1;
}
bool MappedFacetReference::BuildMobilityPolicy(double angle,const std::map<int,int>& roles,MobilityPolicy& result,
    AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};op.Storage(transfer_memory::Mul(reference->Facets().size()+edges.size(),512));
    if(!(std::isfinite(angle)&&angle>=0&&angle<=180))throw std::invalid_argument("Invalid geometry feature angle.");
    MobilityPolicy policy;policy.angle=angle;policy.roles=roles;policy.bodies=body_roots;
    for(const auto& f:reference->Facets()) {
      const auto role=roles.find(f.second.marker);if(role==roles.end()||(role->second!=0&&role->second!=2))throw std::invalid_argument("Incomplete/invalid geometry marker roles.");
      policy.sheets.emplace(f.first,f.first);
    }
    const auto cosine=CosineDegrees(angle);
    auto root=[](auto& roots,Id id){while(roots.at(id)!=id){roots[id]=roots.at(roots.at(id));id=roots.at(id);}return id;};
    std::map<Id,std::vector<size_t>> feature_vertices;
    for(size_t e=0;e<edges.size();++e) {
      op.Work();const auto& incident=edge_facets[e];bool protect=incident.size()!=2;
      if(!protect)protect=reference->Get(incident[0]).marker!=reference->Get(incident[1]).marker;
      if(!protect) {
        std::vector<Range> pending{{0,1}};size_t edge_work=0;
        while(!pending.empty()&&edge_work<64) {
          op.Work();++edge_work;const auto t=pending.back();pending.pop_back();
          if(NormalAngleBound(EdgeNormal(*this,incident[0],edges[e],t),EdgeNormal(*this,incident[1],edges[e],t),cosine))continue;
          const auto middle=(t.lo+t.hi)/2;
          if(!(middle>t.lo&&middle<t.hi)){protect=true;break;}
          pending.push_back({t.lo,middle});pending.push_back({middle,t.hi});
        }
        protect=protect||!pending.empty();
      }
      if(protect) {
        policy.features.emplace(edges[e],Id(e));for(Id v:edges[e])feature_vertices[v].push_back(e);
      } else {
        const auto a=root(policy.sheets,incident[0]),b=root(policy.sheets,incident[1]);policy.sheets[std::max(a,b)]=std::min(a,b);
      }
    }
    auto tangent=[&](size_t index,Id vertex) {
      const auto edge=edges[index];const Id facet=*std::min_element(edge_facets[index].begin(),edge_facets[index].end());
      const auto curve=curves.find(facet);
      if(curve==curves.end())return Difference(Exact(nodes.at(edge[0]==vertex?edge[1]:edge[0])),Exact(nodes.at(vertex)));
      const auto& f=reference->Get(facet);unsigned k=0;while(k<3&&!(std::minmax(f.v[k].id,f.v[(k+1)%3].id)==std::minmax(edge[0],edge[1])))++k;
      const unsigned controls[3][2]={{3,4},{5,6},{7,8}};const bool forward=f.v[k].id==vertex;
      return Difference(Exact(curve->second[controls[k][forward?0:1]]),Exact(nodes.at(vertex)));
    };
    auto signature=[&](size_t index){std::vector<std::pair<int,int>> labels;for(Id f:edge_facets[index]){const int m=reference->Get(f).marker;labels.push_back({m,roles.at(m)});}std::sort(labels.begin(),labels.end());return labels;};
    for(const auto& vertex:feature_vertices) {
      op.Work();bool corner=vertex.second.size()!=2;
      if(!corner) {
        const auto a=vertex.second[0],b=vertex.second[1];auto incoming=tangent(a,vertex.first);for(auto& d:incoming)d={-d.hi,-d.lo};
        corner=signature(a)!=signature(b)||!NormalAngleBound(incoming,tangent(b,vertex.first),cosine);
      }
      if(corner)policy.corners.insert(vertex.first);
    }
    std::map<Id,Id> chains;for(const auto& edge:policy.features)chains.emplace(edge.second,edge.second);
    for(const auto& vertex:feature_vertices)if(!policy.corners.count(vertex.first)) {
      const auto a=root(chains,Id(vertex.second[0])),b=root(chains,Id(vertex.second[1]));chains[std::max(a,b)]=std::min(a,b);
    }
    for(auto& row:policy.features)row.second=root(chains,row.second);
    for(auto& row:policy.sheets)row.second=root(policy.sheets,row.first);
    uint64_t hash=1469598103934665603ull;auto word=[&](uint64_t x){for(unsigned i=0;i<8;++i){hash^=(x>>(8*i))&255;hash*=1099511628211ull;}};
    auto real=[&](double x){uint64_t bits;std::memcpy(&bits,&x,8);word(bits);};
    word(policy.version);word(version);real(angle);
    for(const auto& row:roles){word(uint64_t(row.first));word(uint64_t(row.second));}
    for(const auto& row:reference->Facets()){word(row.first);word(uint64_t(row.second.marker));for(auto n:row.second.v){word(n.id);real(n.p.x);real(n.p.y);real(n.p.z);}}
    for(const auto& row:curves){word(row.first);for(auto p:row.second){real(p.x);real(p.y);real(p.z);}}
    for(const auto& row:reference->Affine()){const auto& p=row.second;word(row.first);word(p.identity);word(p.version);word(p.dimension);real(p.origin.x);real(p.origin.y);real(p.origin.z);real(p.normal.x);real(p.normal.y);real(p.normal.z);real(p.maximum_error);}
    for(const auto& row:policy.features){word(row.first[0]);word(row.first[1]);word(row.second);}
    for(Id v:policy.corners)word(v);for(const auto& row:policy.sheets){word(row.first);word(row.second);}
    policy.identity=hash?hash:1;result=std::move(policy);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
unsigned MappedFacetReference::MobilityDimension(const SurfaceBinding& binding,const MobilityPolicy& policy)const {
  CheckBinding(binding);if(!policy.identity)return binding.dimension;
  if(policy.version!=1)throw std::invalid_argument("Unsupported geometry mobility policy.");
  if(binding.dimension==2)return 2;
  if(binding.dimension==1)return policy.features.count(edges.at(binding.feature))?1:2;
  if(policy.corners.count(binding.feature))return 0;
  for(Id id:vertex_facets.at(binding.feature)) {
    const auto& f=reference->Get(id);for(unsigned k=0;k<3;++k) {
      SU2Native3D::Edge edge{f.v[k].id,f.v[(k+1)%3].id};if(edge[0]!=binding.feature&&edge[1]!=binding.feature)continue;
      std::sort(edge.begin(),edge.end());if(policy.features.count(edge))return 1;
    }
  }
  return 2;
}
double MappedFacetReference::ComponentPointError(const SurfaceBinding& binding,Point p)const {
  const auto model=Ideal(binding);const double original=PointModelError(model,p);double maximum=original;
  ForFacets(binding,[&](Id id){const auto plane=reference->Affine().find(Canonical(id));if(plane==reference->Affine().end())return;
    const auto offset=PlaneModelOffset(model,plane->second);if(!offset)return;
    maximum=std::max(maximum,std::nextafter(double(std::nextafter(static_cast<long double>(original)+offset,INFINITY)),INFINITY));});
  return maximum;
}
Id MappedFacetReference::EdgeIdentity(SU2Native3D::Edge edge)const {
  const auto found=std::lower_bound(edges.begin(),edges.end(),edge);
  if(found==edges.end() || *found!=edge)throw std::invalid_argument("Missing canonical original mapped edge.");
  return Id(found-edges.begin());
}
void MappedFacetReference::CheckBinding(const SurfaceBinding& b)const {
  if(b.version!=version || !std::isfinite(b.u) || !std::isfinite(b.v))
    throw std::invalid_argument("Mapped binding version/nonfinite parameter conflict.");
  if(b.dimension==0) {
    if(!nodes.count(b.feature) || b.u!=0 || b.v!=0)throw std::invalid_argument("Invalid mapped vertex binding.");
  } else if(b.dimension==1) {
    if(b.feature>=edges.size() || !(b.u>0 && b.u<1) || b.v!=0)throw std::invalid_argument("Invalid mapped edge binding.");
  } else if(b.dimension==2) {
    if(!reference->Facets().count(b.feature) || !(b.u>0 && b.v>0) ||
        SU2Native3D::Orientation({1,0,0},{0,1,0},{b.u,b.v,0},{0,0,1})<=0)
      throw std::invalid_argument("Invalid mapped facet interior binding.");
  } else throw std::invalid_argument("Invalid mapped support dimension.");
}
SurfaceBinding MappedFacetReference::Vertex(Id vertex)const {
  SurfaceBinding out{vertex,version,0,0,0};CheckBinding(out);return out;
}
SurfaceBinding MappedFacetReference::EdgePoint(SU2Native3D::Edge edge,double t)const {
  if(edge[0]>=edge[1])throw std::invalid_argument("Mapped edge parameters require canonical endpoint order.");
  const auto feature=EdgeIdentity(edge);
  if(t==0)return Vertex(edge[0]);if(t==1)return Vertex(edge[1]);
  SurfaceBinding out{feature,version,1,t,0};CheckBinding(out);return out;
}
SurfaceBinding MappedFacetReference::FacetPoint(Id facet,double u,double v)const {
  SurfaceBinding out{facet,version,2,u,v};CheckBinding(out);return out;
}
SU2Native3D::ParametricPoint MappedFacetReference::Ideal(const SurfaceBinding& b)const {
  CheckBinding(b);
  if(b.dimension==0){const auto p=nodes.at(b.feature);return {p,p,p,0,0};}
  if(b.dimension==1){const auto edge=edges.at(b.feature);return {nodes.at(edge[0]),nodes.at(edge[1]),nodes.at(edge[0]),b.u,0};}
  const auto& f=reference->Get(b.feature);return {f.v[0].p,f.v[1].p,f.v[2].p,b.u,b.v};
}
SU2Native3D::ParametricPoint MappedFacetReference::FacetParameters(Id facet,const SurfaceBinding& b)const {
  if(!Supports(facet,b))throw std::invalid_argument("Curved parameter binding leaves its facet.");
  const auto& f=reference->Get(facet);
  auto corner=[&](Id id){for(unsigned k=0;k<3;++k)if(f.v[k].id==id){Point p{};if(k==0)p.x=1;if(k==1)p.y=1;if(k==2)p.z=1;return p;}
    throw std::invalid_argument("Curved binding has no facet corner.");};
  if(b.dimension==0){const auto p=corner(b.feature);return {p,p,p,0,0};}
  if(b.dimension==1){const auto e=edges.at(b.feature);return {corner(e[0]),corner(e[1]),corner(e[0]),b.u,0};}
  return {{1,0,0},{0,1,0},{0,0,1},b.u,b.v};
}
std::array<Point,2> MappedFacetReference::CurvedBounds(const SurfaceBinding& b)const {
  CheckBinding(b);Id facet=UINT64_MAX;
  ForFacets(b,[&](Id f){facet=std::min(facet,f);});
  const auto found=curves.find(facet);
  if(found==curves.end())return SU2Native3D::ParametricPointBounds(Ideal(b));
  if(b.dimension==0){const auto p=nodes.at(b.feature);return {p,p};}
  const auto r=ParameterBox(FacetParameters(facet,b));const auto box=Blossom(found->second,{r,r,r});
  Vec lo{},hi{};
  for(unsigned d=0;d<3;++d){lo[d]=std::nextafter(double(box[d].lo),-INFINITY);hi[d]=std::nextafter(double(box[d].hi),INFINITY);}
  const auto a=ToPoint(lo),c=ToPoint(hi);Finite(a);Finite(c);return {a,c};
}
size_t MappedFacetReference::CurvedQueryWorkspaceBytes(size_t maximum_work)const {
  if(!curved_search)return 0;
  size_t facets=0;for(const auto& row:curved_search->entries)facets=std::max(facets,row.second.facets.size());
  return transfer_memory::Add(transfer_memory::Mul(facets,2*sizeof(unsigned long)),
      transfer_memory::Mul(maximum_work,4*sizeof(CurvedQueryRegion)));
}
MappedFacetReference::CurvedClosest MappedFacetReference::ClosestCurved(
    Id hint,Point query,double tolerance,size_t maximum_work,size_t ceiling)const {
  Finite(query);
  if(!curved_search || !curves.count(hint) || !(std::isfinite(tolerance)&&tolerance>=std::numeric_limits<double>::min()) || !maximum_work)
    throw std::invalid_argument("Invalid curved component/distance query contract.");
  const auto& entry=curved_search->entries.at(curved_search->root.at(hint));
  using Region=CurvedQueryRegion;
  const auto storage=CurvedQueryWorkspaceBytes(maximum_work);
  if(storage==SIZE_MAX||storage>ceiling)throw std::length_error("Curved query exceeds storage admission.");
  CurvedClosest result;result.upper=INFINITY;
  auto consume=[&](){if(result.work>=maximum_work)throw std::length_error("Curved distance certification work exhausted.");++result.work;};
  auto binding=[&](Id id,Point r) {
    const auto& f=reference->Get(id);const auto v=Values(r);
    for(unsigned k=0;k<3;++k)if(v[k]==1)return Vertex(f.v[k].id);
    for(unsigned k=0;k<3;++k)if(v[k]==0) {
      const unsigned a=(k+1)%3,b=(k+2)%3;
      return f.v[a].id<f.v[b].id?EdgePoint({f.v[a].id,f.v[b].id},v[b]):EdgePoint({f.v[b].id,f.v[a].id},v[a]);
    }
    return FacetPoint(id,r.y,r.z);
  };
  auto candidate=[&](Id id,Point r) {
    const auto a=Exact(r);const auto box=Blossom(curves.at(id),{a,a,a});
    const auto upper=NormBound(Difference(box,Exact(query)));
    if(upper<result.upper) {
      result.facet=id;result.binding=binding(id,r);result.point=Realize(result.binding);result.upper=upper;
    }
  };
  consume();candidate(hint,{1,0,0});candidate(hint,{0,1,0});candidate(hint,{0,0,1});candidate(hint,{.5,.25,.25});
  if(!std::isfinite(result.upper))throw std::invalid_argument("Unrepresentable curved query distance.");
  // The house nearest-line routine uses squared binary64 distances; it is only an optional seed.
  if(entry.index && result.upper>tolerance && result.upper<std::sqrt(std::numeric_limits<double>::max())/4) {
    std::lock_guard<std::mutex> lock(curved_search->mutex);
    const su2double x[3]={query.x,query.y,query.z};su2double distance=0;unsigned short marker=0;unsigned long id=0;int rank=0;
    entry.index->DetermineNearestElement(x,distance,marker,id,rank);const Id seed=entry.facets.at(id);
    if(seed!=hint){consume();candidate(seed,{1,0,0});candidate(seed,{0,1,0});candidate(seed,{0,0,1});candidate(seed,{.5,.25,.25});}
  }
  if(!std::isfinite(result.upper))throw std::invalid_argument("Unrepresentable curved query distance.");
  std::vector<unsigned long> candidates;
  if(entry.index) {
    const auto x=Values(query);su2double lo[3],hi[3];
    for(unsigned d=0;d<3;++d) {
      lo[d]=std::nextafter(double(Subtract(x[d],result.upper).lo),-INFINITY);
      hi[d]=std::nextafter(double(Add({x[d],x[d]},{result.upper,result.upper}).hi),INFINITY);
    }
    std::lock_guard<std::mutex> lock(curved_search->mutex);entry.index->DetermineIntersectingElements(lo,hi,candidates);
    std::sort(candidates.begin(),candidates.end());
  } else {candidates.resize(entry.facets.size());std::iota(candidates.begin(),candidates.end(),0);}
  auto compare=[](const Region& a,const Region& b){return a.lower>b.lower;};
  std::priority_queue<Region,std::vector<Region>,decltype(compare)> pending(compare);
  auto prepare=[&](Id id,const std::array<Point,3>& parameter,unsigned depth) {
    consume();std::array<Box,10> net;Box hull{};
    for(unsigned d=0;d<3;++d)hull[d]={INFINITY,-INFINITY};
    for(unsigned q=0;q<10;++q) {
      std::array<Box,3> args;unsigned at=0;
      for(unsigned k=0;k<3;++k)for(unsigned n=0;n<CubicIndex[q][k];++n)args[at++]=Exact(parameter[k]);
      net[q]=Blossom(curves.at(id),args);
      for(unsigned d=0;d<3;++d){hull[d].lo=std::min(hull[d].lo,net[q][d].lo);hull[d].hi=std::max(hull[d].hi,net[q][d].hi);}
    }
    Point r{};auto v=Values(r);
    for(unsigned d=0;d<3;++d)v[d]=.25*Values(parameter[0])[d]+.25*Values(parameter[1])[d]+.5*Values(parameter[2])[d];
    candidate(id,ToPoint(v));
    Range squared;
    for(unsigned d=0;d<3;++d) {
      const auto diff=Subtract({Values(query)[d],Values(query)[d]},hull[d]);
      const auto length=std::max({0.L,diff.lo,-diff.hi});squared=Add(squared,Multiply({length,length},length));
    }
    long double lower=std::nextafter(std::sqrt(std::max(0.L,squared.lo)),-INFINITY);
    // Any separating direction gives a valid convex-hull distance bound; a normal resolves flat interior queries.
    Box direction=CrossBox(Difference(net[1],net[0]),Difference(net[2],net[0]));
    for(unsigned attempt=0;attempt<2;++attempt) {
      Vec n{};
      for(unsigned d=0;d<3;++d)n[d]=double((direction[d].lo+direction[d].hi)/2);
      const auto exact=Exact(ToPoint(n));const auto length=NormBound(exact);
      if(std::isfinite(length)&&length>std::numeric_limits<double>::min()) {
        long double lo=INFINITY,hi=-INFINITY;
        for(const auto& control:net){const auto projection=DotBox(Difference(Exact(query),control),exact);lo=std::min(lo,projection.lo);hi=std::max(hi,projection.hi);}
        lower=std::max(lower,std::nextafter(std::max({0.L,lo,-hi})/length,-INFINITY));
      }
      direction=Difference(Exact(query),Blossom(curves.at(id),{Exact(ToPoint(v)),Exact(ToPoint(v)),Exact(ToPoint(v))}));
    }
    const double bound=std::max(0.,std::nextafter(double(lower),-INFINITY));
    pending.push({id,parameter,bound,PositiveNormalClearance(net,query),depth});
  };
  for(auto index:candidates)prepare(entry.facets.at(index),{{{1,0,0},{0,1,0},{0,0,1}}},0);
  if(pending.empty())throw std::logic_error("Curved component broad phase lost its seed.");
  while(!pending.empty()) {
    result.lower=std::min(result.upper,pending.top().lower);
    if(Subtract(result.upper,result.lower).hi<=tolerance)break;
    const auto parent=pending.top();pending.pop();
    if(parent.depth>=48)throw std::length_error("Curved distance parameters exhausted their exact dyadic resolution.");
    std::array<Point,3> mid;
    for(unsigned k=0;k<3;++k){const auto a=Values(parent.parameter[k]),b=Values(parent.parameter[(k+1)%3]);Vec r{};
      for(unsigned d=0;d<3;++d)r[d]=.5*a[d]+.5*b[d];mid[k]=ToPoint(r);}
    for(unsigned k=0;k<3;++k)prepare(parent.facet,{parent.parameter[k],mid[k],mid[(k+2)%3]},parent.depth+1);
    prepare(parent.facet,{mid[0],mid[1],mid[2]},parent.depth+1);
  }
  result.clearance_lower=INFINITY;
  while(!pending.empty()) {
    const auto region=pending.top();pending.pop();
    if(region.lower<=result.upper)result.clearance_lower=std::min(result.clearance_lower,
        region.clearance>0?region.clearance:-result.upper);
  }
  if(!std::isfinite(result.clearance_lower))throw std::logic_error("Curved clearance lost all possible nearest points.");
  const auto evaluation=EvaluateSurface(result.facet,result.binding);result.normal=evaluation.normal;
  result.curvature1=evaluation.curvature1;result.curvature2=evaluation.curvature2;
  result.direction1=evaluation.direction1;result.direction2=evaluation.direction2;return result;
}
std::vector<Id> MappedFacetReference::NearbySurfaceFacets(Id hint,const std::array<Point,2>& box)const {
  for(auto p:box)Finite(p);std::vector<Id> result;
  if(!curved_search||!curved_search->root.count(hint)){result.push_back(hint);return result;}
  const auto& entry=curved_search->entries.at(curved_search->root.at(hint));
  if(!entry.index)return entry.facets;
  const auto a=Values(box[0]),b=Values(box[1]);su2double lo[3],hi[3];for(unsigned d=0;d<3;++d){if(a[d]>b[d])throw std::invalid_argument("Reversed model broad-phase box.");lo[d]=a[d];hi[d]=b[d];}
  std::vector<unsigned long> selected;{std::lock_guard<std::mutex> lock(curved_search->mutex);entry.index->DetermineIntersectingElements(lo,hi,selected);}
  result.reserve(selected.size());for(auto i:selected)result.push_back(entry.facets.at(i));std::sort(result.begin(),result.end());result.erase(std::unique(result.begin(),result.end()),result.end());return result;
}
bool MappedFacetReference::OrientedFace(Id side,const SurfaceBinding& binding,
    const std::array<Point,3>& physical)const {
  if(!Supports(side,binding))return false;for(auto p:physical)Finite(p);
  const auto& f=reference->Get(side);const auto curve=curves.find(side);Box normal;
  if(curve==curves.end())normal=CrossBox(Difference(Exact(f.v[1].p),Exact(f.v[0].p)),Difference(Exact(f.v[2].p),Exact(f.v[0].p)));
  else {
    const auto r=ParameterBox(FacetParameters(side,binding));const auto base=Blossom(curve->second,{Exact({1,0,0}),r,r});
    normal=CrossBox(Difference(Blossom(curve->second,{Exact({0,1,0}),r,r}),base),
        Difference(Blossom(curve->second,{Exact({0,0,1}),r,r}),base));
  }
  const auto actual=CrossBox(Difference(Exact(physical[1]),Exact(physical[0])),Difference(Exact(physical[2]),Exact(physical[0])));
  return DotBox(normal,actual).lo>0;
}
MappedFacetReference::SurfaceEvaluation MappedFacetReference::EvaluateSurface(Id side,const SurfaceBinding& binding)const {
  bool incident=false;ForFacets(binding,[&](Id facet){incident=incident||facet==side;});
  if(!incident)throw std::invalid_argument("Differential side does not support its forward binding.");
  SurfaceEvaluation result;result.point=Realize(binding);const auto curve=curves.find(side);
  if(curve==curves.end()) {
    const auto& f=reference->Get(side);Vec first{},second{},n{};
    for(unsigned d=0;d<3;++d){first[d]=double((long double)Values(f.v[1].p)[d]-Values(f.v[0].p)[d]);second[d]=double((long double)Values(f.v[2].p)[d]-Values(f.v[0].p)[d]);}
    const auto plane=reference->Affine().find(side);
    if(plane!=reference->Affine().end()) {
      n=Values(plane->second.normal);const int orientation=SU2Native3D::SurfaceNormalSign(plane->second.normal,{f.v[0].p,f.v[1].p,f.v[2].p},nullptr);
      if(!orientation)throw std::invalid_argument("Mapped planar side has unresolved orientation.");if(orientation<0)for(auto& x:n)x=-x;
    } else for(unsigned d=0;d<3;++d)n[d]=first[(d+1)%3]*second[(d+2)%3]-first[(d+2)%3]*second[(d+1)%3];
    const auto length=std::hypot(n[0],n[1],n[2]);if(!(length>0&&std::isfinite(length)))throw std::invalid_argument("Unrepresentable mapped planar normal.");
    for(auto& x:n)x/=length;Vec t1=first;long double projection=0;for(unsigned d=0;d<3;++d)projection+=(long double)t1[d]*n[d];
    for(unsigned d=0;d<3;++d)t1[d]=double(t1[d]-projection*n[d]);const auto tangent=std::hypot(t1[0],t1[1],t1[2]);
    if(!(tangent>0&&std::isfinite(tangent)))throw std::invalid_argument("Unrepresentable mapped planar tangent.");
    for(auto& x:t1)x/=tangent;Vec t2{};for(unsigned d=0;d<3;++d)t2[d]=n[(d+1)%3]*t1[(d+2)%3]-n[(d+2)%3]*t1[(d+1)%3];
    result.normal=ToPoint(n);result.first=ToPoint(first);result.second=ToPoint(second);result.direction1=ToPoint(t1);result.direction2=ToPoint(t2);
    Finite(result.first);Finite(result.second);Finite(result.direction1);Finite(result.direction2);return result;
  }
  const auto r=ParameterBox(FacetParameters(side,binding));const auto& net=curve->second;
  const auto base=Blossom(net,{Exact({1,0,0}),r,r});
  const auto normal=CrossBox(Difference(Blossom(net,{Exact({0,1,0}),r,r}),base),
                            Difference(Blossom(net,{Exact({0,0,1}),r,r}),base));
  Vec n{};for(unsigned d=0;d<3;++d)n[d]=double((normal[d].lo+normal[d].hi)/2);
  const double length=std::hypot(n[0],n[1],n[2]);
  if(!(length>0&&std::isfinite(length)))throw std::invalid_argument("Unrepresentable curved normal.");
  for(auto& x:n)x/=length;result.normal=ToPoint(n);
  using Wide=std::array<long double,3>;
  auto midpoint=[](const Box& box,long double scale){Wide out;for(unsigned d=0;d<3;++d)out[d]=scale*(box[d].lo+box[d].hi)/2;return out;};
  auto dot=[](const Wide& a,const Wide& b){long double out=0;for(unsigned d=0;d<3;++d)out+=a[d]*b[d];return out;};
  const auto u=midpoint(Difference(Blossom(net,{Exact({0,1,0}),r,r}),base),3);
  const auto v=midpoint(Difference(Blossom(net,{Exact({0,0,1}),r,r}),base),3);
  Vec first{},next{};for(unsigned d=0;d<3;++d){first[d]=double(u[d]);next[d]=double(v[d]);}
  result.first=ToPoint(first);result.second=ToPoint(next);Finite(result.first);Finite(result.second);
  const auto e0=Exact({1,0,0}),e1=Exact({0,1,0}),e2=Exact({0,0,1});
  const auto zero=Blossom(net,{e0,e0,r});
  auto second=[&](const Box& a,const Box& b){return midpoint(Difference(
      Difference(Blossom(net,{a,b,r}),Blossom(net,{a,e0,r})),Difference(Blossom(net,{e0,b,r}),zero)),6);};
  const Wide unit{n[0],n[1],n[2]};const auto a=std::sqrt(dot(u,u));
  Wide t1{},t2{};for(unsigned d=0;d<3;++d)t1[d]=u[d]/a;
  for(unsigned d=0;d<3;++d)t2[d]=unit[(d+1)%3]*t1[(d+2)%3]-unit[(d+2)%3]*t1[(d+1)%3];
  const auto b=dot(v,t1),c=dot(v,t2);
  if(!(a>0&&c>0&&std::isfinite(a)&&std::isfinite(c)))throw std::invalid_argument("Unrepresentable curved tangent frame.");
  const auto k11=dot(unit,second(e1,e1))/(a*a);
  const auto k12=(dot(unit,second(e1,e2))/a-b*k11)/c;
  const auto k22=(dot(unit,second(e2,e2))-b*b*k11-2*b*c*k12)/(c*c);
  const auto mean=(k11+k22)/2,radius=std::hypot((k11-k22)/2,k12),angle=std::atan2(2*k12,k11-k22)/2;
  result.curvature1=double(mean+radius);result.curvature2=double(mean-radius);
  Vec d1{},d2{};for(unsigned d=0;d<3;++d){d1[d]=double(std::cos(angle)*t1[d]+std::sin(angle)*t2[d]);d2[d]=double(-std::sin(angle)*t1[d]+std::cos(angle)*t2[d]);}
  result.direction1=ToPoint(d1);result.direction2=ToPoint(d2);Finite(result.direction1);Finite(result.direction2);
  if(!std::isfinite(result.curvature1)||!std::isfinite(result.curvature2))throw std::invalid_argument("Unrepresentable curved principal curvature.");
  return result;
}
MappedFacetReference::SourcePatch MappedFacetReference::PrepareSourcePatch(Id facet,
    const std::array<SurfaceBinding,3>& binding,const std::array<Point,3>& source)const {
  if(!ContainsFace(facet,binding,source))throw std::invalid_argument("Source face leaves its mapped parameter patch.");
  SourcePatch out;out.source=source;out.pieces.resize(1);auto& piece=out.pieces.front();
  piece.source_lower=piece.source_upper=source;
  const auto error=errors.find(facet);piece.margin=error==errors.end()?0:error->second;
  std::array<Box,10> controls;
  const auto curve=curves.find(facet);
  if(curve!=curves.end()) {
    // Any future flat child may span current source-face seams within this immutable patch.
    for(unsigned q=0;q<10;++q)controls[q]=Exact(curve->second[q]);
  } else {
    for(unsigned q=0;q<10;++q) {
      Box parameter;for(unsigned d=0;d<3;++d){const auto r=(long double)CubicIndex[q][d]/3;parameter[d]={std::nextafter(r,-INFINITY),std::nextafter(r,INFINITY)};}
      controls[q]=LinearPoint(source,parameter);
    }
  }
  for(unsigned q=0;q<10;++q) {
    Vec lo,hi;for(unsigned d=0;d<3;++d){lo[d]=std::nextafter(double(controls[q][d].lo),-INFINITY);hi[d]=std::nextafter(double(controls[q][d].hi),INFINITY);}
    piece.lower[q]=ToPoint(lo);piece.upper[q]=ToPoint(hi);Finite(piece.lower[q]);Finite(piece.upper[q]);
  }
  return out;
}
MappedFacetReference::SurfaceUnionBounds MappedFacetReference::BoundSurfaceUnion(
    const std::vector<Id>& facets,const std::vector<std::array<Point,3>>& physical,
    double tolerance,AtlasBudget& budget)const {return QuerySurfaceUnion(facets,physical,tolerance,budget,true,true);}
MappedFacetReference::SurfaceUnionBounds MappedFacetReference::BoundModelSurface(
    const std::vector<Id>& facets,const std::vector<std::array<Point,3>>& physical,
    double tolerance,AtlasBudget& budget)const {return QuerySurfaceUnion(facets,physical,tolerance,budget,true,false);}
double MappedFacetReference::BoundPhysicalSurface(const std::vector<Id>& facets,
    const std::vector<std::array<Point,3>>& physical,double tolerance,AtlasBudget& budget)const {
  return QuerySurfaceUnion(facets,physical,tolerance,budget,false,true).mesh_to_model;
}
MappedFacetReference::SurfaceUnionBounds MappedFacetReference::QuerySurfaceUnion(
    const std::vector<Id>& facets,const std::vector<std::array<Point,3>>& physical,
    double tolerance,AtlasBudget& budget,bool include_model,bool include_mesh)const {
  AtlasOps op{budget,__func__};
  struct Restore {AtlasBudget& b;size_t caller;~Restore(){b.caller_bytes=caller;}} restore{budget,budget.caller_bytes};
  if(facets.empty()||physical.empty()||!std::isfinite(tolerance)||tolerance<std::numeric_limits<double>::min())
    throw std::invalid_argument("Invalid surface-union geometry/tolerance.");
  for(size_t i=0;i<facets.size();++i){reference->Get(facets[i]);if(i&&facets[i]<=facets[i-1])throw std::invalid_argument("Surface-union supports must be sorted and unique.");}
  op.Storage(8192);budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,8192);
  const auto physical_bytes=transfer_memory::Mul(physical.size(),sizeof(UnionTriangle));op.Storage(physical_bytes);
  std::vector<UnionTriangle> flat;flat.reserve(physical.size());
  for(const auto& t:physical){op.Work();for(auto p:t)Finite(p);
    if(SU2Native3D::SurfaceOrientationSign(t,t,nullptr)<=0)throw std::invalid_argument("Degenerate physical surface-union triangle.");
    flat.push_back({t,0});}
  budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,transfer_memory::Mul(flat.capacity(),sizeof(UnionTriangle)));
  auto net=[&](const UnionDomain& domain) {
    op.Work();std::array<Box,10> out;const auto curve=curves.find(domain.facet);const auto& f=reference->Get(domain.facet);
    const std::array<Point,3> original{f.v[0].p,f.v[1].p,f.v[2].p};
    for(unsigned q=0;q<10;++q){std::array<Box,3> args;unsigned at=0;
      for(unsigned k=0;k<3;++k)for(unsigned n=0;n<CubicIndex[q][k];++n)args[at++]=Exact(domain.parameter[k]);
      if(curve!=curves.end())out[q]=Blossom(curve->second,args);
      else {Box parameter{};for(const auto& arg:args)for(unsigned d=0;d<3;++d)parameter[d]=Add(parameter[d],DividePositive(arg[d],3));out[q]=LinearPoint(original,parameter);}
    }
    return out;
  };
  auto sample=[&](const UnionDomain& domain,bool model) {
    Vec p{};for(unsigned d=0;d<3;++d)p[d]=.25*Values(domain.parameter[0])[d]+.25*Values(domain.parameter[1])[d]+.5*Values(domain.parameter[2])[d];
    const auto parameter=Exact(ToPoint(p));
    if(!model)return LinearPoint(physical.at(domain.facet),parameter);
    const auto curve=curves.find(domain.facet);if(curve!=curves.end())return Blossom(curve->second,{parameter,parameter,parameter});
    const auto& f=reference->Get(domain.facet);return LinearPoint({f.v[0].p,f.v[1].p,f.v[2].p},parameter);
  };
  std::vector<UnionTriangle> model;std::vector<UnionDomain> pending;
  if(include_mesh)for(Id facet:facets) {
    pending.push_back({facet});
    while(!pending.empty()) {
      const auto domain=pending.back();pending.pop_back();const auto controls=net(domain);UnionTriangle piece;
      for(unsigned k=0;k<3;++k)piece.points[k]=BoxMiddle(controls[k]);
      for(unsigned q=0;q<10;++q){Box linear{};for(unsigned k=0;k<3;++k)for(unsigned d=0;d<3;++d)
          linear[d]=Add(linear[d],Multiply({Values(piece.points[k])[d],Values(piece.points[k])[d]},
              DividePositive({(long double)CubicIndex[q][k],(long double)CubicIndex[q][k]},3)));
        piece.error=std::max(piece.error,NormBound(Difference(controls[q],linear)));}
      op.Storage(transfer_memory::Add(transfer_memory::GrowthBound(model.size()+1,sizeof(UnionTriangle)),
          transfer_memory::GrowthBound(pending.size()+4,sizeof(UnionDomain))));
      if(piece.error<=tolerance/16)model.push_back(piece);
      else {const auto children=UnionChildren(domain);pending.insert(pending.end(),children.begin(),children.end());}
    }
  }
  std::vector<UnionDomain>().swap(pending);
  budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,transfer_memory::Mul(model.capacity(),sizeof(UnionTriangle)));
  // Source enclosure needs only the physical target; approximating the complete model serves the reverse query.
  std::unique_ptr<TriangleUnion> mesh_union,model_union;
  if(include_model)mesh_union=std::make_unique<TriangleUnion>(flat,op,tolerance);
  if(include_mesh)model_union=std::make_unique<TriangleUnion>(model,op,tolerance);
  struct NodeBound {UnionDomain domain;double upper;};
  auto compare=[](const NodeBound& a,const NodeBound& b){return a.upper<b.upper;};
  auto directed=[&](bool from_model,TriangleUnion& target) {
    std::vector<NodeBound> queue;double lower=0,finished=0;size_t leaves=0;
    auto prepare=[&](const UnionDomain& domain) {
      op.Work();double upper=0;
      if(from_model)upper=target.Upper(net(domain));
      else {std::array<Box,3> points;for(unsigned k=0;k<3;++k)points[k]=LinearPoint(physical.at(domain.facet),Exact(domain.parameter[k]));upper=target.Upper(points);}
      lower=std::max(lower,target.Distance(sample(domain,from_model)).lower);
      if(!std::isfinite(upper))throw std::invalid_argument("Unrepresentable surface-union upper bound.");
      op.Storage(transfer_memory::GrowthBound(queue.size()+1,sizeof(NodeBound)));
      queue.push_back({domain,upper});std::push_heap(queue.begin(),queue.end(),compare);
    };
    if(from_model)for(Id facet:facets)prepare({facet});else for(size_t i=0;i<physical.size();++i)prepare({i});
    const double initial_upper=queue.front().upper;
    while(!queue.empty()) {
      if(lower>tolerance)return std::make_tuple(initial_upper,leaves,true);
      std::pop_heap(queue.begin(),queue.end(),compare);const auto node=queue.back();queue.pop_back();
      if(node.upper<=tolerance){finished=std::max(finished,node.upper);++leaves;continue;}
      const auto children=UnionChildren(node.domain);for(const auto& child:children)prepare(child);
    }
    return std::make_tuple(finished,leaves,false);
  };
  SurfaceUnionBounds result;const auto forward=include_model?directed(true,*mesh_union):std::make_tuple(0.,size_t(0),false);
  const auto reverse=include_mesh?directed(false,*model_union):std::make_tuple(0.,size_t(0),false);
  result.model_to_mesh=std::get<0>(forward);result.mesh_to_model=std::get<0>(reverse);
  result.model_pieces=model.size();result.physical_pieces=std::get<1>(reverse);
  result.outside_tolerance=std::get<2>(forward)||std::get<2>(reverse);
  result.within_tolerance=include_model&&include_mesh&&!result.outside_tolerance;return result;
}
bool MappedFacetReference::BindingParameters(Id facet,const SurfaceBinding& binding,ExactBary& result,
    AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};const auto p=FacetParameters(facet,binding);
    auto coordinate=[&](unsigned d){const auto a=Values(p.origin)[d],b=Values(p.first)[d],c=Values(p.second)[d];
      return op.Add(op.Binary(a),op.Add(op.Mul(op.Binary(p.u),op.Sub(op.Binary(b),op.Binary(a))),op.Mul(op.Binary(p.v),op.Sub(op.Binary(c),op.Binary(a)))));};
    ExactBary out{coordinate(1),coordinate(2)};op.Bary(out);result=std::move(out);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::ReferencePointKey(Id facet,const ExactBary& point,std::vector<uint64_t>& result,
    AtlasBudget& budget,std::string& reason)const {
  try {AtlasOps op{budget,__func__};auto out=ReferenceKey(*reference,facet,point,op);result=std::move(out);reason.clear();return true;}
  catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::ReferenceParameters(Id facet,const std::vector<uint64_t>& key,ExactBary& result,
    AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};const auto& f=reference->Get(facet);ExactBary out{op.Integer(0),op.Integer(0)};
    auto has=[&](Id id){for(auto n:f.v)if(n.id==id)return true;return false;};
    if(key.size()==2&&key[0]==0) {
      if(!nodes.count(key[1]))throw std::invalid_argument("Unknown original vertex key.");
      if(!has(key[1])){reason.clear();return false;}
      out.u=op.Integer(f.v[1].id==key[1]);out.v=op.Integer(f.v[2].id==key[1]);
    } else if(key.size()>=4&&key[0]==1) {
      if(key[1]>=key[2])throw std::invalid_argument("Noncanonical original edge key.");
      EdgeIdentity({key[1],key[2]});size_t offset=3;ExactRatio t;op.Check(ExactDecode(key,offset,t,budget,op.reason));
      if(offset!=key.size()||op.Compare(t,op.Integer(0))<=0||op.Compare(t,op.Integer(1))>=0)throw std::invalid_argument("Invalid original edge key parameter.");
      if(!has(key[1])||!has(key[2])){reason.clear();return false;}
      auto weight=[&](Id id){return id==key[1]?op.Sub(op.Integer(1),t):id==key[2]?t:op.Integer(0);};
      out={weight(f.v[1].id),weight(f.v[2].id)};
    } else if(key.size()>=3&&key[0]==2) {
      reference->Get(key[1]);size_t offset=2;op.Check(ExactDecode(key,offset,out.u,budget,op.reason));op.Check(ExactDecode(key,offset,out.v,budget,op.reason));op.Bary(out);
      if(offset!=key.size()||op.Compare(out.u,op.Integer(0))<=0||op.Compare(out.v,op.Integer(0))<=0||op.Compare(op.Add(out.u,out.v),op.Integer(1))>=0)throw std::invalid_argument("Noncanonical original facet point key.");
      if(facet!=key[1]){reason.clear();return false;}
    } else throw std::invalid_argument("Invalid original reference point key.");
    result=std::move(out);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::ResolveSingleChart(Id facet,const std::array<SurfaceBinding,3>& binding,
    FaceCoverage& result,AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};op.Storage(sizeof(FaceCoverage)+sizeof(CoveragePiece)+4096);
    CoveragePiece piece;piece.facet=facet;
    for(unsigned k=0;k<3;++k) {
      op.Check(BindingParameters(facet,binding[k],piece.reference[k],budget,op.reason));
      piece.active[k]={op.Integer(k==1),op.Integer(k==2)};
    }
    if(op.Compare(op.Orient(piece.reference[0],piece.reference[1],piece.reference[2]),op.Integer(0))<=0)
      throw std::invalid_argument("Single-chart coverage is degenerate or reversed.");
    FaceCoverage out;out.pieces.push_back(std::move(piece));result=std::move(out);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::CoverageTraces(const FaceCoverage& coverage,std::array<EdgeTrace,3>& result,
    AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};op.Storage(transfer_memory::Mul(coverage.pieces.size(),3*(sizeof(TracePiece)+1024)));
    auto spell=[&](TracePiece& trace) {
      for(unsigned z=0;z<3;++z)if(!op.Compare(op.Weight(trace.first,z),op.Integer(0))&&!op.Compare(op.Weight(trace.last,z),op.Integer(0))) {
        const auto& f=reference->Get(trace.facet);SU2Native3D::Edge edge{f.v[(z+1)%3].id,f.v[(z+2)%3].id};std::sort(edge.begin(),edge.end());
        const auto& incident=edge_facets.at(EdgeIdentity(edge));const Id canonical=*std::min_element(incident.begin(),incident.end());
        if(canonical!=trace.facet) {
          auto remap=[&](const ExactBary& p){std::map<Id,ExactRatio> w;for(unsigned j=0;j<3;++j)w.emplace(f.v[j].id,op.Weight(p,j));const auto& g=reference->Get(canonical);
            return ExactBary{w.count(g.v[1].id)?w.at(g.v[1].id):op.Integer(0),w.count(g.v[2].id)?w.at(g.v[2].id):op.Integer(0)};};
          trace.first=remap(trace.first);trace.last=remap(trace.last);trace.facet=canonical;
        }break;
      }
    };
    struct Segment {ExactRatio start;TracePiece piece;};std::array<std::vector<Segment>,3> boundary;
    if(coverage.pieces.empty())throw std::invalid_argument("Empty curved face coverage.");
    if(coverage.pieces.size()==1) {
      const auto& piece=coverage.pieces.front();reference->Get(piece.facet);bool identity=true;
      for(unsigned k=0;k<3;++k){op.Bary(piece.reference[k]);op.Bary(piece.active[k]);
        identity=identity&&!op.Compare(piece.active[k].u,op.Integer(k==1))&&!op.Compare(piece.active[k].v,op.Integer(k==2));}
      if(identity) {
        if(op.Compare(op.Orient(piece.reference[0],piece.reference[1],piece.reference[2]),op.Integer(0))<=0)
          throw std::invalid_argument("Curved coverage reverses its reference simplex.");
        std::array<EdgeTrace,3> out;for(unsigned k=0;k<3;++k){TracePiece trace{piece.facet,piece.reference[k],piece.reference[(k+1)%3],op.Integer(1)};
          spell(trace);out[k].pieces.push_back(std::move(trace));}
        result=std::move(out);reason.clear();return true;
      }
    }
    for(const auto& piece:coverage.pieces) {
      reference->Get(piece.facet);
      for(unsigned k=0;k<3;++k){op.Bary(piece.reference[k]);op.Bary(piece.active[k]);}
      if(op.Compare(op.Orient(piece.active[0],piece.active[1],piece.active[2]),op.Integer(0))<=0)
        throw std::invalid_argument("Curved coverage reverses its active simplex.");
      if(op.Compare(op.Orient(piece.reference[0],piece.reference[1],piece.reference[2]),op.Integer(0))<=0)
        throw std::invalid_argument("Curved coverage reverses its reference simplex.");
      for(unsigned k=0;k<3;++k)for(unsigned side=0;side<3;++side) {
        const unsigned next=(k+1)%3,zero=side==0?2:side==1?0:1;
        if(op.Compare(op.Weight(piece.active[k],zero),op.Integer(0)) || op.Compare(op.Weight(piece.active[next],zero),op.Integer(0)))continue;
        auto t=[&](unsigned at){return side==0?piece.active[at].u:side==1?piece.active[at].v:op.Sub(op.Integer(1),piece.active[at].v);};
        const auto first=t(k),last=t(next);
        if(op.Compare(first,last)>=0)throw std::invalid_argument("Curved coverage boundary reverses or collapses.");
        TracePiece trace{piece.facet,piece.reference[k],piece.reference[next],last};
        spell(trace);
        boundary[side].push_back({first,std::move(trace)});
      }
    }
    std::array<EdgeTrace,3> out;
    for(unsigned side=0;side<3;++side) {
      auto& segments=boundary[side];std::sort(segments.begin(),segments.end(),[&](const auto& a,const auto& b){return op.Compare(a.start,b.start)<0;});
      auto end=op.Integer(0);AtlasKey previous;bool first=true;
      for(auto& segment:segments) {
        if(op.Compare(segment.start,end))throw std::invalid_argument("Curved coverage edge has a gap or overlap.");
        const auto begin=ReferenceKey(*reference,segment.piece.facet,segment.piece.first,op);
        if(!first&&begin!=previous)throw std::invalid_argument("Curved coverage edge jumps between reference paths.");
        end=segment.piece.end;previous=ReferenceKey(*reference,segment.piece.facet,segment.piece.last,op);first=false;
        out[side].pieces.push_back(std::move(segment.piece));
      }
      if(first||op.Compare(end,op.Integer(1)))throw std::invalid_argument("Curved coverage edge is incomplete.");
    }
    result=std::move(out);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::CompareTraces(const EdgeTrace& a,const EdgeTrace& b,bool& equal,AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};
    auto check=[&](const EdgeTrace& trace){auto last=op.Integer(0);AtlasKey previous,start;bool first=true;
      for(const auto& p:trace.pieces){op.Bary(p.first);op.Bary(p.last);if(op.Compare(p.end,last)<=0||op.Compare(p.end,op.Integer(1))>0)throw std::invalid_argument("Invalid curved trace interval.");
        auto key=ReferenceKey(*reference,p.facet,p.first,op),end_key=ReferenceKey(*reference,p.facet,p.last,op);
        if(key==end_key)throw std::invalid_argument("Collapsed curved trace path.");
        if(first)start=key;if(!first&&key!=previous)throw std::invalid_argument("Discontinuous curved edge trace.");
        previous=std::move(end_key);last=p.end;first=false;}
      if(first||op.Compare(last,op.Integer(1)))throw std::invalid_argument("Incomplete curved edge trace.");return std::make_pair(std::move(start),std::move(previous));};
    const auto ak=check(a),bk=check(b);if(a.pieces.size()==1&&b.pieces.size()==1){equal=ak==bk;reason.clear();return true;}
    size_t i=0,j=0;auto begin=op.Integer(0),sa=begin,sb=begin;
    while(i<a.pieces.size()&&j<b.pieces.size()) {
      op.Work();const auto& x=a.pieces[i];const auto& y=b.pieces[j];const auto end=op.Compare(x.end,y.end)<0?x.end:y.end;
      auto key=[&](const TracePiece& p,const ExactRatio& start,const ExactRatio& t){return ReferenceKey(*reference,p.facet,op.Lerp(p.first,p.last,op.Div(op.Sub(t,start),op.Sub(p.end,start))),op);};
      if(key(x,sa,begin)!=key(y,sb,begin)||key(x,sa,end)!=key(y,sb,end)){equal=false;reason.clear();return true;}
      if(!op.Compare(x.end,end)){sa=end;++i;}if(!op.Compare(y.end,end)){sb=end;++j;}begin=end;
    }
    equal=i==a.pieces.size()&&j==b.pieces.size();reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::EqualTraces(const EdgeTrace& a,const EdgeTrace& b,AtlasBudget& budget,std::string& reason)const {
  bool equal=false;if(!CompareTraces(a,b,equal,budget,reason))return false;
  if(!equal)reason="Curved edge reference paths or active parameters differ.";return equal;
}
bool MappedFacetReference::ValidateCoverage(Id anchor,const std::array<SurfaceBinding,3>& binding,
    const std::array<Point,3>& realized,const FaceCoverage& coverage,const std::array<EdgeTrace,3>& traces,
    AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};const auto& anchored=reference->Get(anchor);std::vector<AtlasEdge> edges;
    if(coverage.pieces.empty())throw std::invalid_argument("Empty curved face coverage.");
    op.Storage(transfer_memory::Mul(coverage.pieces.size()+1,3*(sizeof(AtlasEdge)+1024)));
    std::array<AtlasKey,3> corners;
    for(unsigned k=0;k<3;++k) {
      if(!Admissible(binding[k],realized[k]))throw std::invalid_argument("Curved face endpoint leaves its forward binding.");
      Id f=UINT64_MAX;ForFacets(binding[k],[&](Id id){f=std::min(f,id);});ExactBary point;
      op.Check(BindingParameters(f,binding[k],point,budget,op.reason));corners[k]=ReferenceKey(*reference,f,point,op);
    }
    const auto flat_normal=CrossBox(Difference(Exact(realized[1]),Exact(realized[0])),Difference(Exact(realized[2]),Exact(realized[0])));
    for(size_t i=0;i<coverage.pieces.size();++i) {
      const auto& p=coverage.pieces[i];const auto& f=reference->Get(p.facet);
      if(f.marker!=anchored.marker||body_roots.at(p.facet)!=body_roots.at(anchor))throw std::invalid_argument("Curved face coverage crosses a marker or body.");
      for(unsigned k=0;k<3;++k){op.Bary(p.active[k]);op.Bary(p.reference[k]);
        for(unsigned c=0;c<3;++c)if(!op.Compare(op.Weight(p.active[k],c),op.Integer(1))&&ReferenceKey(*reference,p.facet,p.reference[k],op)!=corners[c])
          throw std::invalid_argument("Curved coverage endpoint differs from its realized binding.");}
      if(op.Compare(op.Orient(p.active[0],p.active[1],p.active[2]),op.Integer(0))<=0||op.Compare(op.Orient(p.reference[0],p.reference[1],p.reference[2]),op.Integer(0))<=0)
        throw std::invalid_argument("Curved coverage piece reverses or degenerates.");
      PositiveProjectedNet(RestrictNet(*this,p.facet,p.reference,op),flat_normal,op);
      for(unsigned k=0;k<3;++k)edges.push_back({0,p.facet,p.active[k],p.active[(k+1)%3],p.reference[k],p.reference[(k+1)%3],1,i});
    }
    if(coverage.pieces.size()==1) {
      const auto& p=coverage.pieces.front();bool identity=true;
      for(unsigned k=0;k<3;++k)identity=identity&&!op.Compare(p.active[k].u,op.Integer(k==1))&&!op.Compare(p.active[k].v,op.Integer(k==2));
      if(identity){std::array<EdgeTrace,3> derived;op.Check(CoverageTraces(coverage,derived,budget,op.reason));
        for(unsigned k=0;k<3;++k)op.Check(EqualTraces(derived[k],traces[k],budget,op.reason));reason.clear();return true;}
    }
    const std::array<ExactBary,3> unit{{{op.Integer(0),op.Integer(0)},{op.Integer(1),op.Integer(0)},{op.Integer(0),op.Integer(1)}}};
    for(unsigned k=0;k<3;++k)edges.push_back({0,anchor,unit[k],unit[(k+1)%3],{}, {},-1,SIZE_MAX});
    auto boundary=CoverageBoundaryEdges(edges,*reference,op);
    // Raw input may have hanging subdivisions; retain exact splitting on its residual chain.
    const auto boundary_bytes=transfer_memory::Add(ReferenceEdgeBytes(edges),ReferenceEdgeBytes(boundary));
    struct RestoreBoundary {AtlasBudget& budget;size_t caller;~RestoreBoundary(){budget.caller_bytes=caller;}} restore_boundary{budget,budget.caller_bytes};
    op.Storage(boundary_bytes);budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,boundary_bytes);
    const auto split=SplitAtlasEdges(boundary,op);std::map<AtlasEdgeKey,int> count;
    std::map<AtlasEdgeKey,std::pair<AtlasKey,AtlasKey>> paths;
    for(const auto& e:split) {
      int sign=e.sign;auto a=op.Key(e.a),b=op.Key(e.b);const bool reverse=b<a;const auto key=EdgeKeys(std::move(a),std::move(b),sign);count[key]+=sign;
      if(e.triangle==SIZE_MAX)continue;
      auto ra=ReferenceKey(*reference,e.facet,e.ra,op),rb=ReferenceKey(*reference,e.facet,e.rb,op);if(reverse)std::swap(ra,rb);
      const auto inserted=paths.emplace(key,std::make_pair(ra,rb));if(!inserted.second&&inserted.first->second!=std::make_pair(ra,rb))throw std::invalid_argument("Curved face pieces disagree on an internal reference path.");
    }
    for(const auto& row:count)if(row.second)throw std::invalid_argument("Curved coverage does not tile the active simplex exactly.");
    std::array<EdgeTrace,3> derived;op.Check(CoverageTraces(coverage,derived,budget,op.reason));
    for(unsigned k=0;k<3;++k)op.Check(EqualTraces(derived[k],traces[k],budget,op.reason));
    reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::ValidateFeatureReplacement(
    const std::vector<std::pair<const EdgeTrace*,int>>& uses,const MobilityPolicy& policy,
    bool directed,AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};struct Restore {AtlasBudget& b;size_t n;~Restore(){b.caller_bytes=n;}} restore{budget,budget.caller_bytes};
    std::map<std::pair<SU2Native3D::Edge,AtlasKey>,int> events;size_t storage=0;
    for(const auto& use:uses) {
      if(!use.first||!use.second)throw std::invalid_argument("Invalid curve coverage use.");
      std::optional<Id> chain;
      for(const auto& piece:use.first->pieces) {
        op.Work();op.Bary(piece.first);op.Bary(piece.last);const auto& facet=reference->Get(piece.facet);bool found=false;
        for(unsigned k=0;k<3;++k)if(!op.Compare(op.Weight(piece.first,k),op.Integer(0))&&!op.Compare(op.Weight(piece.last,k),op.Integer(0))) {
          SU2Native3D::Edge edge{facet.v[(k+1)%3].id,facet.v[(k+2)%3].id};std::sort(edge.begin(),edge.end());
          const auto feature=policy.features.find(edge);if(feature==policy.features.end()||(chain&&*chain!=feature->second))
            throw std::invalid_argument("Curve coverage leaves its genuine feature identity.");
          chain=feature->second;unsigned axis=0;while(facet.v[axis].id!=edge[1])++axis;
          auto first=op.Weight(piece.first,axis),last=op.Weight(piece.last,axis);const auto sign=op.Compare(first,last);
          if(!sign)throw std::invalid_argument("Degenerate feature coverage interval.");
          if(!directed&&sign>0)std::swap(first,last);
          for(unsigned endpoint=0;endpoint<2;++endpoint){AtlasKey key;op.Check(ExactEncode(endpoint?last:first,key,budget,op.reason));
            const auto bytes=transfer_memory::Add(sizeof(decltype(events)::value_type),4*sizeof(void*),transfer_memory::Bytes(key));
            op.Storage(bytes);const auto item=events.emplace(std::make_pair(edge,std::move(key)),0);
            if(item.second){storage=transfer_memory::Add(storage,bytes);budget.caller_bytes=transfer_memory::Add(restore.n,storage);}
            AddChainMultiplicity(item.first->second,endpoint?-use.second:use.second);}
          found=true;break;
        }
        if(!found)throw std::invalid_argument("Feature coverage has no original curve segment.");
      }
      if(!chain)throw std::invalid_argument("Empty feature coverage trace.");
    }
    for(const auto& event:events)if(event.second)throw std::invalid_argument("Replacement changes genuine feature coverage.");
    reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::ValidateCoverageReplacement(const std::vector<FaceCoverage>& old,
    const std::vector<FaceCoverage>& fresh,AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};size_t pieces=0;for(const auto* covers:{&old,&fresh})for(const auto& cover:*covers)pieces=transfer_memory::Add(pieces,cover.pieces.size());
    const auto index_bytes=transfer_memory::Mul(pieces,128);op.Storage(index_bytes);
    struct Restore {AtlasBudget& budget;size_t caller;~Restore(){budget.caller_bytes=caller;}} restore{budget,budget.caller_bytes};
    budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,index_bytes);
    std::map<Id,std::vector<std::pair<const CoveragePiece*,int>>> groups;
    auto append=[&](const auto& covers,int sign){for(const auto& cover:covers)for(const auto& p:cover.pieces){reference->Get(p.facet);
      for(const auto& r:p.reference)op.Bary(r);
      if(op.Compare(op.Orient(p.reference[0],p.reference[1],p.reference[2]),op.Integer(0))<=0)throw std::invalid_argument("Reference coverage reverses or degenerates.");
      groups[p.facet].push_back({&p,sign});}};
    append(old,-1);append(fresh,1);if(groups.empty())throw std::invalid_argument("Empty reference coverage replacement.");
    // Only one original chart's exact arrangement is resident at a time, including full restart audits.
    for(const auto& group:groups) {
      // The boundary operator is linear: cancel exact interior diagonals before arranging unequal subdivisions.
      Restore chain_restore{budget,budget.caller_bytes};auto edges=CancelReferenceEdges(group.first,group.second,op);
      if(edges.empty())continue;const auto bytes=ReferenceEdgeBytes(edges);op.Storage(bytes);budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,bytes);
      std::map<AtlasEdgeKey,int> count;
      for(const auto& e:SplitAtlasEdges(edges,op)){int sign=e.sign;auto key=EdgeKeys(op.Key(e.a),op.Key(e.b),sign);AddChainMultiplicity(count[std::move(key)],sign);}
      for(const auto& row:count)if(row.second)throw std::invalid_argument("Replacement changes exact original-chart coverage.");
    }
    reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::RegionBoundary(const std::vector<CoveragePiece>& region,
    const std::set<std::vector<uint64_t>>& mandatory,std::vector<RegionSegment>& result,
    AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};if(region.empty())throw std::invalid_argument("Empty accepted reference region.");
    const auto index_bytes=transfer_memory::Mul(region.size(),128);op.Storage(index_bytes);
    struct Restore {AtlasBudget& budget;size_t caller;~Restore(){budget.caller_bytes=caller;}} restore{budget,budget.caller_bytes};
    const auto indexed=transfer_memory::Add(budget.caller_bytes,index_bytes);budget.caller_bytes=indexed;
    std::map<Id,std::vector<std::pair<const CoveragePiece*,int>>> groups;
    for(const auto& p:region) {
      reference->Get(p.facet);for(const auto& r:p.reference)op.Bary(r);
      if(op.Compare(op.Orient(p.reference[0],p.reference[1],p.reference[2]),op.Integer(0))<=0)throw std::invalid_argument("Accepted reference region reverses or degenerates.");
      groups[p.facet].push_back({&p,1});
    }
    std::vector<RegionSegment> out;size_t output_bytes=0;
    for(const auto& group:groups) {
      {
        Restore local{budget,budget.caller_bytes};auto residual=CancelReferenceEdges(group.first,group.second,op);
        const auto residual_bytes=ReferenceEdgeBytes(residual);op.Storage(residual_bytes);budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,residual_bytes);
        auto split=SplitAtlasEdges(residual,op);const auto split_bytes=ReferenceEdgeBytes(split);op.Storage(split_bytes);budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,split_bytes);
        struct BoundaryEdge {int count=0;const ExactBary* first=nullptr;const ExactBary* last=nullptr;};
        std::map<AtlasEdgeKey,BoundaryEdge> boundary;size_t boundary_bytes=0;
        for(const auto& e:split) {
          size_t words=12;for(const auto* p:{&e.a,&e.b})for(const auto* q:{&p->u,&p->v})words=transfer_memory::Add(words,q->numerator.size(),q->denominator.size());
          boundary_bytes=transfer_memory::Add(boundary_bytes,sizeof(decltype(boundary)::value_type),4*sizeof(void*),transfer_memory::GrowthBound(words,sizeof(uint64_t)));
        }
        op.Storage(boundary_bytes);budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,boundary_bytes);
        for(const auto& e:split) {
          int sign=e.sign;const auto* a=&e.a;const auto* b=&e.b;auto ka=op.Key(*a),kb=op.Key(*b);if(kb<ka)std::swap(a,b);
          auto& value=boundary[EdgeKeys(std::move(ka),std::move(kb),sign)];AddChainMultiplicity(value.count,sign);value.first=a;value.last=b;
        }
        struct Link {const ExactBary* point=nullptr;size_t in=0,out=0,incoming=SIZE_MAX,outgoing=SIZE_MAX;bool fixed=false;};
        std::map<AtlasKey,Link> links;
        struct Segment {decltype(links)::iterator first,last;bool alive=true;};std::vector<Segment> segments;
        size_t n=0,link_bytes=0;
        for(const auto& row:boundary)if(row.second.count) {
          if(row.second.count!=1&&row.second.count!=-1)throw std::invalid_argument("Accepted reference region has nonunit boundary multiplicity.");
          ++n;link_bytes=transfer_memory::Add(link_bytes,2*(sizeof(decltype(links)::value_type)+4*sizeof(void*)),
              transfer_memory::Bytes(row.first.first),transfer_memory::Bytes(row.first.second));
        }
        link_bytes=transfer_memory::Add(link_bytes,transfer_memory::GrowthBound(n,sizeof(Segment)),
            transfer_memory::GrowthBound(transfer_memory::Mul(n,4),sizeof(decltype(links)::iterator)),transfer_memory::GrowthBound(n,sizeof(size_t)));
        op.Storage(link_bytes);budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,link_bytes);segments.reserve(n);
        for(const auto& row:boundary)if(row.second.count) {
          const bool forward=row.second.count>0;const auto& a=forward?row.first.first:row.first.second;const auto& b=forward?row.first.second:row.first.first;
          auto first=links.emplace(a,Link{}).first,last=links.emplace(b,Link{}).first;const size_t index=segments.size();
          first->second.point=forward?row.second.first:row.second.last;last->second.point=forward?row.second.last:row.second.first;
          ++first->second.out;first->second.outgoing=index;++last->second.in;last->second.incoming=index;segments.push_back({first,last,true});
        }
        std::vector<decltype(links)::iterator> queue;queue.reserve(links.size());
        for(auto it=links.begin();it!=links.end();++it) {
          if(it->second.in!=it->second.out)throw std::invalid_argument("Accepted reference region has an open boundary.");
          it->second.fixed=mandatory.count(ReferenceKey(*reference,group.first,*it->second.point,op));queue.push_back(it);
        }
        while(!queue.empty()) {
          op.Work();auto point=queue.back();queue.pop_back();auto& link=point->second;
          if(link.fixed||link.in!=1||link.out!=1)continue;
          const size_t a=link.incoming,b=link.outgoing;if(a==b||!segments[a].alive||!segments[b].alive)continue;
          auto first=segments[a].first,last=segments[b].last;if(first==last)continue;
          const auto& p=*first->second.point;const auto& q=*point->second.point;const auto& r=*last->second.point;
          if(op.Compare(op.Orient(p,q,r),op.Integer(0)))continue;
          const bool use_u=op.Compare(p.u,r.u);const auto& x=use_u?p.u:p.v;const auto& y=use_u?q.u:q.v;const auto& z=use_u?r.u:r.v;
          const int direction=op.Compare(x,z);if(!direction||op.Compare(x,y)!=direction||op.Compare(y,z)!=direction)continue;
          segments[a].last=last;segments[b].alive=false;link.in=link.out=0;
          if(last->second.in==1)last->second.incoming=a;queue.push_back(first);queue.push_back(last);
        }
        std::vector<size_t> order;order.reserve(n);size_t added_bytes=0;
        for(size_t i=0;i<segments.size();++i)if(segments[i].alive) {
          order.push_back(i);added_bytes=transfer_memory::Add(added_bytes,3*sizeof(RegionSegment));
          for(const auto* p:{segments[i].first->second.point,segments[i].last->second.point})for(const auto* q:{&p->u,&p->v})added_bytes=transfer_memory::Add(added_bytes,transfer_memory::Bytes(q->numerator),transfer_memory::Bytes(q->denominator));
        }
        std::sort(order.begin(),order.end(),[&](size_t a,size_t b){if(segments[a].first->first!=segments[b].first->first)return segments[a].first->first<segments[b].first->first;return segments[a].last->first<segments[b].last->first;});
        op.Storage(added_bytes);out.reserve(transfer_memory::Add(out.size(),order.size()));
        for(size_t i:order)out.push_back({group.first,*segments[i].first->second.point,*segments[i].last->second.point});
        output_bytes=transfer_memory::Add(output_bytes,added_bytes);
      }
      budget.caller_bytes=transfer_memory::Add(indexed,output_bytes);
    }
    result=std::move(out);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool MappedFacetReference::CertifyEmbedding(const std::vector<CoveragePiece>& region,Point normal,
    AtlasBudget& budget,std::string& reason)const {
  try {
    AtlasOps op{budget,__func__};Finite(normal);const auto n=Values(normal);double scale=0;for(double x:n)scale=std::max(scale,std::abs(x));
    if(!(scale>0)||region.empty())throw std::invalid_argument("Curved embedding needs a nonempty disk and projection normal.");
    Vec direction{};for(unsigned d=0;d<3;++d)direction[d]=n[d]/scale;
    unsigned axis=0;for(unsigned d=1;d<3;++d)if(std::abs(direction[d])<std::abs(direction[axis]))axis=d;
    Vec u{},v{};u[(axis+1)%3]=direction[(axis+2)%3];u[(axis+2)%3]=-direction[(axis+1)%3];
    for(unsigned d=0;d<3;++d)v[d]=direction[(d+1)%3]*u[(d+2)%3]-direction[(d+2)%3]*u[(d+1)%3];
    const auto first=Exact(ToPoint(u)),second=Exact(ToPoint(v)),projection_normal=CrossBox(first,second);
    op.Storage(transfer_memory::Mul(region.size(),3*(sizeof(AtlasEdge)+4096)));
    std::vector<AtlasEdge> edges;
    for(size_t i=0;i<region.size();++i) {
      const auto& p=region[i];for(const auto& r:p.reference)op.Bary(r);
      if(op.Compare(op.Orient(p.reference[0],p.reference[1],p.reference[2]),op.Integer(0))<=0)throw std::invalid_argument("Curved embedding reference piece reverses or collapses.");
      PositiveProjectedNet(RestrictNet(*this,p.facet,p.reference,op),projection_normal,op);
      for(unsigned k=0;k<3;++k)edges.push_back({p.facet,p.facet,p.reference[k],p.reference[(k+1)%3],{}, {},1,i});
    }
    auto split=SplitAtlasEdges(edges,op);
    std::map<AtlasEdgeKey,std::vector<AtlasEdge>> incidence;std::set<AtlasKey> vertices;
    std::vector<std::vector<AtlasKey>> polygons(region.size());
    std::vector<std::vector<size_t>> adjacency(region.size());
    for(auto& e:split) {
      auto a=ReferenceKey(*reference,e.facet,e.a,op),b=ReferenceKey(*reference,e.facet,e.b,op);vertices.insert(a);vertices.insert(b);
      polygons[e.triangle].push_back(a);
      int sign=1;const auto key=EdgeKeys(a,b,sign);e.sign=sign;incidence[key].push_back(std::move(e));
    }
    std::map<AtlasKey,AtlasKey> next;std::map<AtlasKey,AtlasEdge> boundary;
    for(const auto& entry:incidence) {
      const auto& uses=entry.second;
      if(uses.size()==2&&uses[0].sign==-uses[1].sign) {
        adjacency[uses[0].triangle].push_back(uses[1].triangle);adjacency[uses[1].triangle].push_back(uses[0].triangle);
      } else if(uses.size()==1) {
        const auto& e=uses[0];const auto a=ReferenceKey(*reference,e.facet,e.a,op),b=ReferenceKey(*reference,e.facet,e.b,op);
        if(!next.emplace(a,b).second||!boundary.emplace(a,e).second)throw std::invalid_argument("Branched curved embedding perimeter.");
      } else throw std::invalid_argument("Curved reference disk has inconsistent/nonmanifold incidence.");
    }
    if(boundary.size()<3||vertices.size()+region.size()!=incidence.size()+1)throw std::invalid_argument("Curved reference region is not a disk.");
    std::set<size_t> reached;std::vector<size_t> pending{0};
    while(!pending.empty()){op.Work();const auto i=pending.back();pending.pop_back();if(!reached.insert(i).second)continue;for(auto j:adjacency[i])pending.push_back(j);}
    if(reached.size()!=region.size())throw std::invalid_argument("Curved reference disk is disconnected.");
    std::set<AtlasKey> boundary_vertices;std::vector<AtlasEdge> perimeter;auto at=next.begin()->first;const auto start=at;
    do {
      op.Work();if(!boundary_vertices.insert(at).second||!next.count(at))throw std::invalid_argument("Open or repeated curved reference perimeter.");
      perimeter.push_back(boundary.at(at));at=next.at(at);
    }while(at!=start);
    if(perimeter.size()!=boundary.size())throw std::invalid_argument("Curved reference disk has multiple boundary loops.");
    std::map<AtlasKey,std::vector<std::pair<AtlasKey,AtlasKey>>> links;
    for(const auto& polygon:polygons) {
      if(polygon.size()<3)throw std::invalid_argument("Collapsed curved reference cell boundary.");
      for(size_t k=0;k<polygon.size();++k)links[polygon[k]].push_back({polygon[(k+polygon.size()-1)%polygon.size()],polygon[(k+1)%polygon.size()]});
    }
    for(const auto& row:links) {
      std::map<AtlasKey,std::vector<AtlasKey>> link;
      for(const auto& pair:row.second){link[pair.first].push_back(pair.second);link[pair.second].push_back(pair.first);}
      unsigned ends=0;for(const auto& vertex:link){if(vertex.second.size()==1)++ends;else if(vertex.second.size()!=2)throw std::invalid_argument("Nonmanifold curved reference vertex link.");}
      if(ends!=(boundary_vertices.count(row.first)?2u:0u))throw std::invalid_argument("Curved reference vertex link is not a disk neighborhood.");
      std::set<AtlasKey> visited;std::vector<AtlasKey> queue{link.begin()->first};
      while(!queue.empty()){op.Work();auto v=queue.back();queue.pop_back();if(!visited.insert(v).second)continue;for(const auto& w:link.at(v))queue.push_back(w);}
      if(visited.size()!=link.size())throw std::invalid_argument("Disconnected curved reference vertex link.");
    }
    // At C0 original vertices, positive chart Jacobians alone allow a multiple-winding fan.
    for(const auto& vertex:vertices)if(vertex.size()==2&&vertex[0]==0&&!boundary_vertices.count(vertex)) {
      const Id id=vertex[1];unsigned crossings=0,sectors=0;
      for(const auto& p:region)for(unsigned k=0;k<3;++k)if(ReferenceKey(*reference,p.facet,p.reference[k],op)==vertex) {
        const auto& f=reference->Get(p.facet);unsigned corner=0;while(corner<3&&f.v[corner].id!=id)++corner;
        if(corner==3)throw std::invalid_argument("Missing original vertex in curved reference fan.");
        auto tangent=[&](const ExactBary& to) {
          const unsigned near[3][3]={{0,3,8},{4,1,5},{7,6,2}};std::array<ExactRatio,3> xyz;
          const auto curve=curves.find(p.facet);
          for(unsigned d=0;d<3;++d) {
            xyz[d]=op.Integer(0);
            for(unsigned j=0;j<3;++j)if(j!=corner) {
              const double control=curve==curves.end()?Values(f.v[j].p)[d]:Values(curve->second[near[corner][j]])[d];
              auto value=op.Mul(op.Weight(to,j),op.Sub(op.Binary(control),op.Binary(Values(f.v[corner].p)[d])));
              if(curve!=curves.end())value=op.Mul(op.Integer(3),value);xyz[d]=op.Add(xyz[d],value);
            }
          }
          ExactBary projected{op.Integer(0),op.Integer(0)};
          for(unsigned d=0;d<3;++d){projected.u=op.Add(projected.u,op.Mul(xyz[d],op.Binary(u[d])));projected.v=op.Add(projected.v,op.Mul(xyz[d],op.Binary(v[d])));}return projected;
        };
        const auto a=tangent(p.reference[(k+1)%3]),b=tangent(p.reference[(k+2)%3]);const ExactBary zero{op.Integer(0),op.Integer(0)};
        if(op.Compare(op.Orient(zero,a,b),op.Integer(0))<=0)throw std::invalid_argument("Curved C0 vertex has an unresolved/reversed tangent sector.");
        if(op.Compare(a.v,op.Integer(0))<=0&&op.Compare(b.v,op.Integer(0))>0)++crossings;++sectors;
      }
      if(!sectors||crossings!=1)throw std::invalid_argument("Curved C0 vertex fan does not have local degree one.");
    }
    auto curve=[&](const AtlasEdge& e) {
      const auto a=op.Parameter(e.a),b=op.Parameter(e.b);CurveBoxes out;const auto net=curves.find(e.facet);const auto& f=reference->Get(e.facet);
      for(unsigned q=0;q<4;++q) {
        std::array<Box,3> args;for(unsigned k=0;k<3;++k)args[k]=k<3-q?a:b;Box p;
        if(net!=curves.end())p=Blossom(net->second,args);
        else {Box parameter{};for(const auto& arg:args)for(unsigned d=0;d<3;++d)parameter[d]=Add(parameter[d],DividePositive(arg[d],3));p=LinearPoint({f.v[0].p,f.v[1].p,f.v[2].p},parameter);}
        out[q][0]=DotBox(p,first);out[q][1]=DotBox(p,second);out[q][2]={0,0};
      }
      return out;
    };
    std::vector<CurveBoxes> arcs;
    for(const auto& e:perimeter) {
      std::vector<CurveBoxes> queue{curve(e)};
      while(!queue.empty()) {
        op.Work();const auto p=queue.back();queue.pop_back();
        if(MonotoneCurve(p)){op.Storage(transfer_memory::Mul(arcs.size()+queue.size()+1,sizeof(CurveBoxes)));arcs.push_back(p);continue;}
        const auto halves=BisectCurve(p);op.Storage(transfer_memory::Mul(arcs.size()+queue.size()+2,sizeof(CurveBoxes)));queue.push_back(halves.second);queue.push_back(halves.first);
      }
    }
    for(size_t i=0;i<arcs.size();++i)for(size_t j=i+1;j<arcs.size();++j) {
      if(i==0&&j+1==arcs.size())SeparateCurvePair(arcs[j],arcs[i],true,op);
      else SeparateCurvePair(arcs[i],arcs[j],j==i+1,op);
    }
    // C0 seam controls agree by authority construction. Positive one-sided projected Jacobians,
    // the degree-one vertex fans and a Jordan perimeter give a degree-one disk map.
    reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
size_t MappedFacetReference::SourcePreparationBytes(const FaceCoverage& coverage) {
  return SourcePreparationBytes(coverage.pieces.size());
}
size_t MappedFacetReference::SourcePreparationBytes(size_t pieces) {
  return transfer_memory::Add(sizeof(SourcePatch),transfer_memory::Mul(pieces,sizeof(SourcePatch::Piece)),SourcePatch::WorkspaceBytes());
}
MappedFacetReference::SourcePatch MappedFacetReference::PrepareSourcePatch(const FaceCoverage& coverage,
    const std::array<Point,3>& source,AtlasBudget& budget)const {
  AtlasOps op{budget,__func__};op.Storage(SourcePreparationBytes(coverage));if(coverage.pieces.empty())throw std::invalid_argument("Empty curved source coverage.");
  for(Point p:source)Finite(p);SourcePatch out;out.source=source;out.pieces.reserve(coverage.pieces.size());
  auto bound=[](const Box& p){Vec lo,hi;for(unsigned d=0;d<3;++d){lo[d]=std::nextafter(double(p[d].lo),-INFINITY);hi[d]=std::nextafter(double(p[d].hi),INFINITY);}Finite(ToPoint(lo));Finite(ToPoint(hi));return std::array<Point,2>{ToPoint(lo),ToPoint(hi)};};
  for(const auto& p:coverage.pieces) {
    op.Work();if(op.Compare(op.Orient(p.reference[0],p.reference[1],p.reference[2]),op.Integer(0))<=0||op.Compare(op.Orient(p.active[0],p.active[1],p.active[2]),op.Integer(0))<=0)throw std::invalid_argument("Invalid curved source coverage orientation.");
    SourcePatch::Piece piece;const auto controls=RestrictNet(*this,p.facet,p.reference,op);const auto error=errors.find(p.facet);piece.margin=error==errors.end()?0:error->second;
    for(unsigned q=0;q<10;++q){const auto b=bound(controls[q]);piece.lower[q]=b[0];piece.upper[q]=b[1];}
    for(unsigned k=0;k<3;++k){op.Bary(p.active[k]);const auto b=bound(LinearPoint(source,op.Parameter(p.active[k])));piece.source_lower[k]=b[0];piece.source_upper[k]=b[1];}
    out.pieces.push_back(std::move(piece));
  }
  return out;
}
double MappedFacetReference::SurfaceError(const FaceCoverage& coverage,const std::array<Point,3>& realized,
    AtlasBudget& budget)const {
  AtlasOps op{budget,__func__};op.Storage(SourcePreparationBytes(coverage));if(coverage.pieces.empty())throw std::invalid_argument("Empty curved face error coverage.");
  for(Point p:realized)Finite(p);double maximum=0;
  for(const auto& p:coverage.pieces) {
    op.Work();const auto controls=RestrictNet(*this,p.facet,p.reference,op);std::array<Box,3> flat;
    for(unsigned k=0;k<3;++k){op.Bary(p.active[k]);flat[k]=LinearPoint(realized,op.Parameter(p.active[k]));}
    for(unsigned q=0;q<10;++q) {
      Box chord{};for(unsigned k=0;k<3;++k)for(unsigned d=0;d<3;++d){const long double w=(long double)CubicIndex[q][k]/3;chord[d]=Add(chord[d],Multiply(flat[k][d],Range{std::nextafter(w,-INFINITY),std::nextafter(w,INFINITY)}));}
      maximum=std::max(maximum,NormBound(Difference(controls[q],chord)));
    }
  }
  return maximum;
}
MappedFacetReference::SourcePatch MappedFacetReference::SourcePatch::Tube(
    const std::array<Point,3>& triangle,double radius) {
  for(auto p:triangle)Finite(p);
  if(!std::isfinite(radius)||radius<0||SU2Native3D::SurfaceOrientationSign(triangle,triangle,nullptr)<=0)
    throw std::invalid_argument("Invalid physical source tube.");
  SourcePatch out;out.source=triangle;out.pieces.resize(1);auto& piece=out.pieces.front();
  piece.margin=radius;piece.source_lower=piece.source_upper=triangle;
  for(unsigned q=0;q<10;++q)piece.lower[q]=piece.upper[q]=triangle[q%3];
  return out;
}
std::array<Point,2> MappedFacetReference::SourcePatch::Bounds()const {
  Vec lo{{INFINITY,INFINITY,INFINITY}},hi{{-INFINITY,-INFINITY,-INFINITY}};
  for(const auto& piece:pieces)for(unsigned d=0;d<3;++d) {
    long double low=INFINITY,high=-INFINITY;
    for(unsigned q=0;q<10;++q){low=std::min(low,(long double)Values(piece.lower[q])[d]);high=std::max(high,(long double)Values(piece.upper[q])[d]);}
    for(unsigned q=0;q<3;++q){low=std::min(low,(long double)Values(piece.source_lower[q])[d]);high=std::max(high,(long double)Values(piece.source_upper[q])[d]);}
    lo[d]=std::min(lo[d],std::nextafter(double(Subtract({low,low},{piece.margin,piece.margin}).lo),-INFINITY));
    hi[d]=std::max(hi[d],std::nextafter(double(Add({high,high},{piece.margin,piece.margin}).hi),INFINITY));
  }
  const auto a=ToPoint(lo),b=ToPoint(hi);Finite(a);Finite(b);return {a,b};
}
double MappedFacetReference::SourcePatch::OutwardDisplacement()const {
  const auto origin=Exact(source[0]);const auto normal=CrossBox(Difference(Exact(source[1]),origin),Difference(Exact(source[2]),origin));
  long double length=0,projection=0;
  for(const auto& r:normal)length=std::max({length,r.lo,-r.hi});
  if(!(length>0 && std::isfinite(length)))throw std::invalid_argument("Unrepresentable source-face normal.");
  long double bound=0;
  for(const auto& piece:pieces) {
    projection=0;
    for(unsigned q=0;q<10;++q) {
      Box point;for(unsigned d=0;d<3;++d)point[d]={Values(piece.lower[q])[d],Values(piece.upper[q])[d]};
      projection=std::max(projection,DotBox(Difference(point,origin),normal).hi);
    }
    bound=std::max(bound,Add({0,std::nextafter(projection/length,INFINITY)},{piece.margin,piece.margin}).hi);
  }
  const double result=std::nextafter(double(bound),INFINITY);
  if(!std::isfinite(result))throw std::invalid_argument("Unrepresentable source-face displacement.");
  return result;
}
size_t MappedFacetReference::SourcePatch::WorkspaceBytes(){return 32768;}
size_t MappedFacetReference::SourcePatch::ResidentBytes()const{return transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(pieces));}
bool MappedFacetReference::SourcePatch::Separated(const std::array<Point,3>& outer,uint64_t& work,uint64_t maximum_work)const {
  for(auto p:outer)Finite(p);
  for(const auto& piece:pieces) {
  if(work>=maximum_work)throw std::length_error("Curved source separation work exhausted.");++work;
  std::array<Box,13> hull;
  for(unsigned q=0;q<10;++q)for(unsigned d=0;d<3;++d)hull[q][d]={Values(piece.lower[q])[d],Values(piece.upper[q])[d]};
  for(unsigned k=0;k<3;++k)for(unsigned d=0;d<3;++d)hull[10+k][d]={Values(piece.source_lower[k])[d],Values(piece.source_upper[k])[d]};
  auto separated=[&](const Box& direction) {
    Vec n;for(unsigned d=0;d<3;++d)n[d]=double((direction[d].lo+direction[d].hi)/2);
    const auto axis=Exact(ToPoint(n));const auto length=NormBound(axis);
    if(!(std::isfinite(length)&&length>std::numeric_limits<double>::min()))return false;
    long double lo=INFINITY,hi=-INFINITY,olo=INFINITY,ohi=-INFINITY;
    for(const auto& p:hull){const auto r=DotBox(Difference(p,Exact(outer[0])),axis);lo=std::min(lo,r.lo);hi=std::max(hi,r.hi);}
    for(auto p:outer){const auto r=DotBox(Difference(Exact(p),Exact(outer[0])),axis);olo=std::min(olo,r.lo);ohi=std::max(ohi,r.hi);}
    const auto gap=std::max(Subtract({olo,olo},{hi,hi}).lo,Subtract({lo,lo},{ohi,ohi}).lo);
    const auto required=Multiply({piece.margin,piece.margin},length).hi;
    return std::isfinite(gap)&&std::isfinite(required)&&gap>=required;
  };
  std::array<Box,3> edge,flat;
  for(unsigned k=0;k<3;++k){edge[k]=Difference(Exact(outer[(k+1)%3]),Exact(outer[k]));flat[k]=Difference(hull[10+(k+1)%3],hull[10+k]);}
  bool proved=separated(CrossBox(edge[0],edge[1])) || separated(CrossBox(flat[0],flat[1]));
  for(unsigned d=0;d<3&&!proved;++d){Box axis{};axis[d]={1,1};proved=separated(axis);}
  for(unsigned i=0;i<3&&!proved;++i)for(unsigned j=0;j<3&&!proved;++j)proved=separated(CrossBox(flat[i],edge[j]));
  // Curved controls form a polytope, whose faces/edges need not follow the source triangle.
  for(unsigned i=0;i<hull.size()&&!proved;++i)for(unsigned j=i+1;j<hull.size()&&!proved;++j) {
    const auto tangent=Difference(hull[j],hull[i]);
    for(unsigned k=j+1;k<hull.size()&&!proved;++k)proved=separated(CrossBox(tangent,Difference(hull[k],hull[i])));
    for(const auto& e:edge)if(!proved)proved=separated(CrossBox(tangent,e));
  }
  if(!proved)return false;
  }
  return !pieces.empty();
}
double MappedFacetReference::EnvelopeError(Id facet)const {
  const auto e=errors.find(facet);const double numerical=e==errors.end()?0.:e->second;
  const auto found=curves.find(facet);if(found==curves.end())return numerical;
  const auto& f=reference->Get(facet);
  const auto deviation=CubicDeviation(found->second,{Exact({1,0,0}),Exact({0,1,0}),Exact({0,0,1})},
                                      {f.v[0].p,f.v[1].p,f.v[2].p});
  return std::nextafter(deviation+numerical,INFINITY);
}
double MappedFacetReference::SurfaceError(Id facet,const std::array<SurfaceBinding,3>& binding,
                                         const std::array<Point,3>& realized)const {
  const auto found=curves.find(facet);
  if(found==curves.end()){double result=0;for(unsigned k=0;k<3;++k)result=std::max(result,ErrorBound(binding[k],realized[k]));return result;}
  const auto key=FaceMemo::Query(facet,binding,realized);
  if(face_memo) {
    std::lock_guard<std::mutex> guard(face_memo->lock);const auto cached=face_memo->values.find(key);
    if(cached!=face_memo->values.end())return cached->second;
  }
  std::array<Box,3> parameters;
  for(unsigned k=0;k<3;++k){Finite(realized[k]);parameters[k]=ParameterBox(FacetParameters(facet,binding[k]));}
  const double result=CubicDeviation(found->second,parameters,realized);
  if(face_memo) {
    std::lock_guard<std::mutex> guard(face_memo->lock);
    if(face_memo->values.size()>=FaceMemo::Entries)std::unordered_map<FaceMemo::Key,double,FaceMemo::Hash>().swap(face_memo->values);
    face_memo->values.emplace(key,result);
  }
  return result;
}
double MappedFacetReference::MaximumError(const SurfaceBinding& b)const {
  CheckBinding(b);
  if(components){double bound=INFINITY;ForFacets(b,[&](Id id){bound=std::min(bound,components->entries.at(Canonical(id)).limit);});return bound;}
  auto error=[&](Id id){const auto found=errors.find(id);return found==errors.end()?0.:found->second;};
  if(b.dimension==2)return error(b.feature);
  const auto& incident=b.dimension==0?vertex_facets.at(b.feature):edge_facets.at(b.feature);
  double bound=INFINITY;for(auto id:incident)bound=std::min(bound,error(id));return bound;
}
bool MappedFacetReference::Admissible(const SurfaceBinding& b,Point p)const {
  if(ErrorBound(b,p)>MaximumError(b))return false;
  auto plane=[&](Id id){const auto g=reference->Affine().find(id);return g==reference->Affine().end() || AffineChart(g->second).ErrorBound(p)<=g->second.maximum_error;};
  if(b.dimension==2)return plane(b.feature);
  const auto& rows=b.dimension==0?vertex_facets.at(b.feature):edge_facets.at(b.feature);
  return std::all_of(rows.begin(),rows.end(),plane);
}
bool MappedFacetReference::FeatureSegment(const SurfaceBinding& a,const SurfaceBinding& b,Id& feature,double& ta,double& tb)const {
  CheckBinding(a);CheckBinding(b);if(a.dimension>1 || b.dimension>1)return false;
  if(a.dimension==0 && b.dimension==0) {
    SU2Native3D::Edge edge{a.feature,b.feature};std::sort(edge.begin(),edge.end());
    const auto found=std::lower_bound(edges.begin(),edges.end(),edge);if(found==edges.end() || *found!=edge)return false;
    feature=Id(found-edges.begin());
  }else {feature=a.dimension==1?a.feature:b.feature;if(a.dimension==1&&b.dimension==1&&a.feature!=b.feature)return false;}
  const auto edge=edges.at(feature);
  auto parameter=[&](const SurfaceBinding& p,double& t){if(p.dimension==1){t=p.u;return true;}
    if(p.feature==edge[0] || p.feature==edge[1]){t=p.feature==edge[0]?0.:1.;return true;}return false;};
  return parameter(a,ta)&&parameter(b,tb)&&ta!=tb;
}
bool MappedFacetReference::BoundarySegment(Id facet,const SurfaceBinding& a,const SurfaceBinding& b,Id& feature,double& ta,double& tb)const {
  if(!Supports(facet,a)||!Supports(facet,b)||!FeatureSegment(a,b,feature,ta,tb))return false;
  return !components || components->entries.at(Canonical(facet)).features.count(feature);
}
double MappedFacetReference::ErrorBound(const SurfaceBinding& b,Point p)const {
  bool curved=false;if(!curves.empty())ForFacets(b,[&](Id f){curved=curved||curves.count(f);});
  if(!curved)return components?ComponentPointError(b,p):PointModelError(Ideal(b),p);
  Finite(p);const auto bounds=CurvedBounds(b);const auto lo=Values(bounds[0]),hi=Values(bounds[1]),actual=Values(p);Box error;
  for(unsigned d=0;d<3;++d)error[d]=Subtract(Range{lo[d],hi[d]},Range{actual[d],actual[d]});
  return NormBound(error);
}
Point MappedFacetReference::Realize(const SurfaceBinding& b)const {
  const auto model=Ideal(b);auto point=curves.empty()?SU2Native3D::ParametricPointBounds(model)[0]:CurvedBounds(b)[0];
  std::map<Id,AffineAuthority> equations;
  ForFacets(b,[&](Id id){const auto row=reference->Affine().find(id);if(row!=reference->Affine().end())equations.emplace(row->second.identity,row->second);});
  if(!equations.empty()){std::vector<AffineAuthority> constraints;for(const auto& row:equations)constraints.push_back(row.second);point=ProjectIntersection(point,constraints);}
  if(!Admissible(b,point))throw std::invalid_argument("Mapped realization cannot satisfy all incident geometry error contracts.");
  return point;
}
bool MappedFacetReference::Supports(Id facet,const SurfaceBinding& b)const {
  CheckBinding(b);
  if(components){const Id root=Canonical(facet);bool found=false;ForFacets(b,[&](Id id){found=found||Canonical(id)==root;});return found;}
  const auto& f=reference->Get(facet);
  auto has=[&](Id id){return std::any_of(f.v.begin(),f.v.end(),[&](const auto& n){return n.id==id;});};
  return b.dimension==0?has(b.feature):b.dimension==1?(has(edges.at(b.feature)[0])&&has(edges.at(b.feature)[1])):b.feature==facet;
}
bool MappedFacetReference::ContainsFace(Id facet,const std::array<SurfaceBinding,3>& binding,
    const std::array<Point,3>& realized,SU2Native3D::KernelStats* stats)const {
  using namespace SU2Native3D;
  if(components) {
    // Component containment scans the whole component border with exact predicates; identical queries (the same
    // physical faces re-checked by every kernel and admission) reuse their verdict. Statistics callers recompute.
    if(stats || !face_memo)return ComponentContainsFace(facet,binding,realized,stats);
    const auto key=FaceMemo::Query(facet,binding,realized);
    {std::lock_guard<std::mutex> guard(face_memo->lock);const auto found=face_memo->values.find(key);if(found!=face_memo->values.end())return found->second;}
    const bool verdict=ComponentContainsFace(facet,binding,realized,nullptr);
    std::lock_guard<std::mutex> guard(face_memo->lock);
    if(face_memo->values.size()>=FaceMemo::Entries)std::unordered_map<FaceMemo::Key,double,FaceMemo::Hash>().swap(face_memo->values);
    face_memo->values.emplace(key,verdict);return verdict;
  }
  std::array<ParametricPoint,3> ideal;
  for(unsigned i=0;i<3;++i) {
    if(!Supports(facet,binding[i]) || !Admissible(binding[i],realized[i]))return false;
    ideal[i]=Ideal(binding[i]);
  }
  const auto& f=reference->Get(facet);
  const ParametricPoint a{f.v[0].p},b{f.v[1].p},c{f.v[2].p};
  for(unsigned axis=0;axis<3;++axis) {
    const int old=ParametricOrientation(a,b,c,axis,stats);
    if(old)return ParametricOrientation(ideal[0],ideal[1],ideal[2],axis,stats)==old &&
        SurfaceOrientationSign({f.v[0].p,f.v[1].p,f.v[2].p},realized,stats)>0;
  }
  throw std::logic_error("Degenerate admitted mapped facet.");
}
bool MappedFacetReference::ContainsSegment(Id facet,const SurfaceBinding& a,const SurfaceBinding& b,SU2Native3D::KernelStats* stats)const {
  using namespace SU2Native3D;
  if(!Supports(facet,a)||!Supports(facet,b))return false;
  if(!components)return true;
  const auto& c=components->entries.at(Canonical(facet));if(c.convex)return true;
  const auto x=ComponentFlat(Ideal(a),c.axis),y=ComponentFlat(Ideal(b),c.axis);
  auto side=[&](const ParametricPoint& a,const ParametricPoint& b,const ParametricPoint& p){return ParametricOrientation(a,b,p,2,stats);};
  auto same=[](const ParametricPoint& a,const ParametricPoint& b){return ParametricOnProjectedSegment(a,a,b,2);};
  for(auto edge:c.border) {
    const ParametricPoint p{ComponentFlat(nodes.at(edge[0]),c.axis)},q{ComponentFlat(nodes.at(edge[1]),c.axis)};
    if(side(x,y,p)*side(x,y,q)<0 && side(p,q,x)*side(p,q,y)<0)return false;
    if(!same(x,p)&&!same(x,q)&&ParametricOnProjectedSegment(p,q,x,2,stats)&&c.orientation*side(p,q,y)<0)return false;
    if(!same(y,p)&&!same(y,q)&&ParametricOnProjectedSegment(p,q,y,2,stats)&&c.orientation*side(p,q,x)<0)return false;
  }
  for(auto corner:c.corners) {
    const ParametricPoint p{ComponentFlat(nodes.at(corner[0]),c.axis)},v{ComponentFlat(nodes.at(corner[1]),c.axis)},q{ComponentFlat(nodes.at(corner[2]),c.axis)};
    if(!ParametricOnProjectedSegment(x,y,v,2,stats))continue;
    const int turn=c.orientation*side(p,v,q);
    for(const auto& direction:{x,y})if(!same(direction,v)) {
      const int left=c.orientation*side(p,v,direction),right=c.orientation*side(v,q,direction);
      if(turn>=0?left<0||right<0:left<0&&right<0)return false;
    }
  }
  return true;
}
bool MappedFacetReference::ComponentContainsFace(Id facet,const std::array<SurfaceBinding,3>& binding,
    const std::array<Point,3>& realized,SU2Native3D::KernelStats* stats)const {
  using namespace SU2Native3D;
  const auto root=Canonical(facet);const auto& c=components->entries.at(root);std::array<ParametricPoint,3> t;
  for(unsigned k=0;k<3;++k){if(!Supports(root,binding[k])||!Admissible(binding[k],realized[k]))return false;t[k]=ComponentFlat(Ideal(binding[k]),c.axis);}
  if(ParametricOrientation(t[0],t[1],t[2],2,stats)!=c.orientation)return false;
  const auto plane=reference->Affine().find(root);
  if(plane!=reference->Affine().end()) {
    const auto n=Values(plane->second.normal);const int outward=c.orientation*(c.axis==1?-1:1)*(n[c.axis]>0?1:-1);
    if(SurfaceNormalSign(plane->second.normal,realized,stats)!=outward)return false;
  }else {
    const auto& f=reference->Get(root);if(SurfaceOrientationSign({f.v[0].p,f.v[1].p,f.v[2].p},realized,stats)<=0)return false;
  }
  if(c.convex)return true;
  // DC-17: exact prefilter of the border loop (same verdict). Rigorous binary64 bounds of the ideal triangle
  // (ParametricPointBounds) give its box and an enclosure of its centroid height; a border edge whose box is strictly
  // apart from the triangle box cannot enter it, and an edge with both ends strictly on one side of the centroid
  // enclosure adds no winding, so the exact predicates run only for the few border edges near the face.
  double tlo[2]={INFINITY,INFINITY},thi[2]={-INFINITY,-INFINITY},clo=0,chi=0;
  for(unsigned k=0;k<3;++k){const auto bnd=ParametricPointBounds(t[k]);
    tlo[0]=std::min(tlo[0],bnd[0].x);tlo[1]=std::min(tlo[1],bnd[0].y);thi[0]=std::max(thi[0],bnd[1].x);thi[1]=std::max(thi[1],bnd[1].y);
    clo+=bnd[0].y;chi+=bnd[1].y;}
  clo=std::nextafter(std::nextafter(clo,-INFINITY)/3,-INFINITY);chi=std::nextafter(std::nextafter(chi,INFINITY)/3,INFINITY);
  const bool boxed=std::isfinite(tlo[0])&&std::isfinite(tlo[1])&&std::isfinite(thi[0])&&std::isfinite(thi[1])&&std::isfinite(clo)&&std::isfinite(chi);
  int winding=0;
  for(auto edge:c.border) {
    const auto a=ComponentFlat(nodes.at(edge[0]),c.axis),b=ComponentFlat(nodes.at(edge[1]),c.axis);
    if(boxed) {
      const bool apart=std::max(a.x,b.x)<tlo[0]||std::min(a.x,b.x)>thi[0]||std::max(a.y,b.y)<tlo[1]||std::min(a.y,b.y)>thi[1];
      const bool oneSide=(a.y<clo&&b.y<clo)||(a.y>chi&&b.y>chi);
      if(apart&&oneSide)continue;
    }
    const int ay=ParametricCentroidSide({{0,a.y,0}},{{1,a.y,0}},t,2,stats),by=ParametricCentroidSide({{0,b.y,0}},{{1,b.y,0}},t,2,stats);
    const int side=ParametricCentroidSide({a},{b},t,2,stats);
    if(ay>=0&&by<0&&side>0)++winding;if(ay<0&&by>=0&&side<0)--winding;
    if(ParametricSegmentEntersTriangle({a},{b},t,2,stats))return false;
  }
  return winding==c.orientation;
}
size_t MappedFacetReference::FaceMemoBytes() {
  // Per entry: key + value + hash-node links/hash + one bucket pointer (generous), plus the container itself.
  return transfer_memory::Add(sizeof(FaceMemo),transfer_memory::Mul(FaceMemo::Entries,sizeof(FaceMemo::Key)+sizeof(double)+4*sizeof(void*)+2*sizeof(size_t)));
}
size_t MappedFacetReference::ResidentBytes()const {
  auto bytes=[](const auto& map){return transfer_memory::Mul(map.size(),sizeof(typename std::decay_t<decltype(map)>::value_type)+4*sizeof(void*));};
  size_t out=transfer_memory::Add(sizeof(*this),bytes(errors),bytes(curves),bytes(nodes),bytes(vertex_facets),bytes(body_roots),transfer_memory::Bytes(edges),transfer_memory::Bytes(edge_facets));
  for(const auto& row:vertex_facets)out=transfer_memory::Add(out,transfer_memory::Bytes(row.second));
  for(const auto& row:edge_facets)out=transfer_memory::Add(out,transfer_memory::Bytes(row));
  if(components){out=transfer_memory::Add(out,sizeof(ComponentData),bytes(components->entries));
    for(const auto& row:components->entries){const auto& c=row.second;out=transfer_memory::Add(out,transfer_memory::Bytes(c.facets),transfer_memory::Bytes(c.border),
        transfer_memory::Bytes(c.corners),bytes(c.features),c.search?c.search->GetAllocatedBytes():0,c.search?c.search->RetainedWorkspaceBound(1):0,transfer_memory::Mul(c.facets.size(),3*sizeof(Id)));}}
  if(face_memo)out=transfer_memory::Add(out,FaceMemoBytes());
  if(curved_search) {
    out=transfer_memory::Add(out,sizeof(CurvedSearch),bytes(curved_search->root),bytes(curved_search->entries));
    for(const auto& row:curved_search->entries){const auto& c=row.second;out=transfer_memory::Add(out,transfer_memory::Bytes(c.facets),
        c.index?c.index->GetAllocatedBytes():0,c.index?c.index->RetainedWorkspaceBound(1):0);}
  }
  return out;
}
bool MappedFacetReference::ValidateSurface(const std::vector<SU2Native3D::SurfaceFace>& surface,
    const std::map<Id,SurfaceBinding>& bindings,const std::map<Id,Point>& realized,MappedSurfaceAudit& report,
    std::string& reason,size_t maximum_faces,size_t ceiling)const {
  using namespace SU2Native3D;
  try {
    if(surface.empty() || surface.size()>maximum_faces)throw std::length_error("Mapped surface face work admission exhausted.");
    MappedSurfaceAudit audit;
    audit.requested_bytes=transfer_memory::Add(ResidentBytes(),transfer_memory::Mul(surface.size(),4096),transfer_memory::Mul(bindings.size(),256));
    if(audit.requested_bytes==SIZE_MAX || audit.requested_bytes>ceiling)throw std::length_error("Mapped surface audit storage admission exhausted.");
    std::map<Id,std::vector<Face>> groups;std::vector<Face> skin;std::set<Id> used;
    for(const auto& face:surface) {
      std::array<SurfaceBinding,3> b;std::array<Point,3> p;
      for(unsigned k=0;k<3;++k){b[k]=bindings.at(face.v[k]);p[k]=realized.at(face.v[k]);used.insert(face.v[k]);
        audit.maximum_mapping_error=std::max(audit.maximum_mapping_error,ErrorBound(b[k],p[k]));}
      if(!ContainsFace(face.facet,b,p))throw std::invalid_argument("Mapped face leaves its finite support, reverses, or exceeds mapping error.");
      groups[Canonical(face.facet)].push_back(face.v);skin.push_back(face.v);
    }
    if(used.size()!=bindings.size() || used.size()!=realized.size() || groups.size()!=(components?components->entries.size():reference->Facets().size()))
      throw std::invalid_argument("Mapped boundary bindings or original facet coverage are incomplete.");
    ValidateDomainSurfaceTopology(skin);
    // Canonical original support and scalar parameter identify one current shared node, independent of rank IDs.
    std::map<Id,Id> vertices;std::map<std::pair<Id,double>,Id> edge_nodes;
    for(const auto& row:bindings) {
      const auto& b=row.second;
      if(b.dimension==0 && !vertices.emplace(b.feature,row.first).second)throw std::invalid_argument("Duplicate mapped original corner.");
      if(b.dimension==1 && !edge_nodes.emplace(std::make_pair(b.feature,b.u),row.first).second)throw std::invalid_argument("Duplicate mapped feature parameter.");
    }
    for(const auto& group:groups) {
      ValidateSurfaceTopology(group.second,false,maximum_faces,nullptr,bool(components));
      std::map<Id,bool> local_features;const std::map<Id,bool>* expected=nullptr;
      if(components)expected=&components->entries.at(group.first).features;
      else {const auto& original=reference->Get(group.first);
        for(unsigned k=0;k<3;++k){Edge e{original.v[k].id,original.v[(k+1)%3].id};const bool forward=e[0]<e[1];std::sort(e.begin(),e.end());local_features.emplace(EdgeIdentity(e),forward);}expected=&local_features;}
      std::map<Edge,std::vector<Edge>> uses;
      for(const auto& face:group.second)for(unsigned k=0;k<3;++k){Edge e{face[k],face[(k+1)%3]},key=e;std::sort(key.begin(),key.end());uses[key].push_back(e);}
      std::map<Id,std::vector<std::pair<double,double>>> chains;
      for(const auto& row:uses)if(row.second.size()==1) {
        const auto directed=row.second.front();Id feature;double a,b;
        if(!BoundarySegment(group.first,bindings.at(directed[0]),bindings.at(directed[1]),feature,a,b))
          throw std::invalid_argument("Mapped component border leaves its physical feature.");
        const auto found=expected->find(feature);
        if(found==expected->end() || !(found->second?a<b:b<a))throw std::invalid_argument("Mapped feature chain reverses or collapses.");
        chains[feature].push_back({std::min(a,b),std::max(a,b)});++audit.feature_segments;
      }
      if(chains.size()!=expected->size())throw std::invalid_argument("Mapped component has missing border features.");
      for(auto& row:chains) {
        auto& chain=row.second;std::sort(chain.begin(),chain.end());double end=0;
        for(const auto& interval:chain){if(interval.first!=end)throw std::invalid_argument("Mapped feature chain has a gap or overlap.");end=interval.second;}
        if(end!=1)throw std::invalid_argument("Mapped feature chain does not cover its complete finite interval.");
      }
    }
    audit.nodes=used.size();audit.faces=surface.size();report=audit;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
FiniteLineReference::FiniteLineReference(AffineAuthority authority,const std::vector<LineEdge>& edges,size_t ceiling):chart(authority) {
  if(authority.dimension!=2 || edges.empty())throw std::invalid_argument("Finite line reference requires 2D intervals.");
  const auto preparation=transfer_memory::Mul(edges.size(),1024);
  if(preparation==SIZE_MAX || preparation>ceiling)throw std::invalid_argument("Finite line preparation exceeds storage budget.");
  std::map<Id,SU2Native3D::Node> points;std::map<Id,LineEdge> next;std::set<Id> previous,identities;
  std::map<double,Id> positions;
  for(auto edge:edges) {
    if(!identities.insert(edge.identity).second || edge.a.id==edge.b.id || !next.emplace(edge.a.id,edge).second || !previous.insert(edge.b.id).second)
      throw std::invalid_argument("Duplicate/branched finite line topology.");
    for(auto node:{edge.a,edge.b}) {
      if(chart.ErrorBound(node.p)>authority.maximum_error)throw std::invalid_argument("Finite line vertex leaves its declared authority.");
      const auto point=points.emplace(node.id,node);
      if(!point.second && Values(point.first->second.p)!=Values(node.p))throw std::invalid_argument("Inconsistent shared finite line vertex.");
      const auto position=positions.emplace(chart.Coordinates(node.p).x,node.id);
      if(!position.second && position.first->second!=node.id)throw std::invalid_argument("Distinct finite line features have coincident chart coordinates.");
    }
  }
  std::vector<Id> starts;for(const auto& row:next)if(!previous.count(row.first))starts.push_back(row.first);
  for(Id start:starts) {
    const double first=chart.Coordinates(points.at(start).p).x;double last=first;Id at=start,root=UINT64_MAX;int direction=0;
    while(next.count(at)) {
      const auto edge=next.at(at);next.erase(at);root=std::min(root,edge.identity);
      const auto value=chart.Coordinates(edge.b.p).x;const int sign=value>last?1:value<last?-1:0;
      if(!sign || (direction && direction!=sign))throw std::invalid_argument("Degenerate/reversed finite line interval.");
      direction=sign;last=value;at=edge.b.id;
    }
    intervals.push_back({std::min(first,last),std::max(first,last),root,first<last?start:at,first<last?at:start});
  }
  if(!next.empty())throw std::invalid_argument("Cyclic finite line topology.");
  std::sort(intervals.begin(),intervals.end(),[](const auto& a,const auto& b){return a.lo<b.lo;});
  for(size_t i=1;i<intervals.size();++i)if(intervals[i].lo<=intervals[i-1].hi)throw std::invalid_argument("Overlapping/touching disconnected finite line intervals.");
}
bool FiniteLineReference::Contains(Point p)const {
  if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;const auto x=chart.Coordinates(p).x;
  const auto it=std::upper_bound(intervals.begin(),intervals.end(),x,[](double x,const auto& interval){return x<interval.lo;});
  return it!=intervals.begin() && x<=std::prev(it)->hi;
}
bool FiniteLineReference::ContainsSegment(Point a,Point b)const {
  if(!Contains(a)||!Contains(b))return false;const auto x=chart.Coordinates(a).x,y=chart.Coordinates(b).x;
  for(const auto& interval:intervals)if(std::min(x,y)>=interval.lo && std::max(x,y)<=interval.hi)return true;return false;
}
Point FiniteLineReference::Project(Point p)const {p=chart.Project(p);if(!Contains(p))throw std::invalid_argument("Projection leaves finite line intervals.");return p;}
std::vector<Id> FiniteLineReference::FeaturesIntersecting(Point a,Point b)const {
  if(chart.ErrorBound(a)>chart.Authority().maximum_error || chart.ErrorBound(b)>chart.Authority().maximum_error)
    throw std::invalid_argument("Finite line feature query leaves its authority.");
  const auto x=chart.Coordinates(a).x,y=chart.Coordinates(b).x;std::vector<Id> found;
  for(const auto& interval:intervals){if(interval.lo>=std::min(x,y)&&interval.lo<=std::max(x,y))found.push_back(interval.first);
    if(interval.hi>=std::min(x,y)&&interval.hi<=std::max(x,y))found.push_back(interval.last);}
  std::sort(found.begin(),found.end());return found;
}
size_t FiniteLineReference::ResidentBytes()const{return transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(intervals));}
FinitePlaneReference::FinitePlaneReference(AffineAuthority authority,const std::vector<SU2Native3D::Facet>& facets,size_t ceiling):chart(authority) {
  using namespace SU2Native3D;
  if(authority.dimension!=3 || facets.empty())throw std::invalid_argument("Finite plane reference requires 3D facets.");
  const auto preparation=transfer_memory::Add(PlanarReference::ConstructionBytes(facets.size()),transfer_memory::Mul(facets.size(),4096));
  if(preparation==SIZE_MAX || preparation>ceiling)throw std::invalid_argument("Finite plane preparation exceeds storage budget.");
  auto flat=facets;std::map<Edge,std::vector<Edge>> uses;std::map<Id,Point> physical;
  for(auto& f:flat) {
    f.marker=0; // Geometry components do not depend on boundary-condition labels.
    for(auto& n:f.v){if(chart.ErrorBound(n.p)>authority.maximum_error)throw std::invalid_argument("Finite plane vertex leaves its declared authority.");
      const auto original=physical.emplace(n.id,n.p);
      if(!original.second && Values(original.first->second)!=Values(n.p))throw std::invalid_argument("Inconsistent shared finite plane vertex coordinates.");
      n.p=chart.Coordinates(n.p);const auto old=nodes.emplace(n.id,n.p);if(!old.second && Values(old.first->second)!=Values(n.p))throw std::invalid_argument("Inconsistent finite plane vertex.");}
    if(Orientation(f.v[0].p,f.v[1].p,f.v[2].p,{0,0,1})==0)throw std::invalid_argument("Degenerate finite plane facet.");
    for(unsigned k=0;k<3;++k){Edge edge{f.v[k].id,f.v[(k+1)%3].id},key=edge;std::sort(key.begin(),key.end());uses[key].push_back(edge);}
  }
  std::vector<SU2NativeBoundary2D::PolylineReference::Face> boundary;
  int orientation=0;
  for(const auto& f:flat){const int sign=Orientation(f.v[0].p,f.v[1].p,f.v[2].p,{0,0,1})>0?1:-1;
    if(orientation && sign!=orientation)throw std::invalid_argument("Inconsistent finite plane facet orientation.");orientation=sign;}
  for(const auto& row:uses) {
    if(row.second.size()==1){auto edge=row.second[0];borders.push_back(edge);if(orientation<0)std::swap(edge[0],edge[1]);
      boundary.push_back({{edge[0],Flat(nodes.at(edge[0]))},{edge[1],Flat(nodes.at(edge[1]))},0});}
    else if(row.second.size()!=2 || row.second[0][0]!=row.second[1][1] || row.second[0][1]!=row.second[1][0])
      throw std::invalid_argument("Nonmanifold/inconsistently oriented finite plane edge.");
  }
  SU2NativeBoundary2D::ValidatePhysicalGraph(boundary);
  reference=std::make_shared<PlanarReference>(std::move(flat),ceiling);
  std::set<Id> roots;for(const auto& f:facets)roots.insert(reference->Canonical(f.id));components.assign(roots.begin(),roots.end());
}
Id FinitePlaneReference::Component(Id facet)const {return reference->Canonical(facet);}
bool FinitePlaneReference::Contains(Id facet,Point p,SU2Native3D::KernelStats* stats)const {
  if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;
  return reference->Contains(facet,chart.Coordinates(p),stats);
}
bool FinitePlaneReference::ContainsSegment(Id facet,Point a,Point b,SU2Native3D::KernelStats* stats)const {
  if(!Contains(facet,a,stats)||!Contains(facet,b,stats))return false;
  return reference->ContainsSegment(facet,chart.Coordinates(a),chart.Coordinates(b),stats);
}
bool FinitePlaneReference::ContainsFace(Id facet,const std::array<Point,3>& face,SU2Native3D::KernelStats* stats)const {
  auto flat=face;for(auto& p:flat){if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;p=chart.Coordinates(p);}
  return reference->ContainsFace(facet,flat,stats);
}
bool FinitePlaneReference::Contains(Point p)const {
  if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;const auto flat=chart.Coordinates(p);
  for(Id root:components)if(reference->Contains(root,flat))return true;return false;
}
bool FinitePlaneReference::ContainsSegment(Point a,Point b)const {
  if(!Contains(a)||!Contains(b))return false;a=chart.Coordinates(a);b=chart.Coordinates(b);
  for(Id root:components)if(reference->ContainsSegment(root,a,b))return true;return false;
}
bool FinitePlaneReference::ContainsFace(const std::array<Point,3>& face)const {
  auto flat=face;for(auto& p:flat){if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;p=chart.Coordinates(p);}
  for(Id root:components)if(reference->ContainsFace(root,flat))return true;return false;
}
Point FinitePlaneReference::Project(Point p)const {p=chart.Project(p);if(!Contains(p))throw std::invalid_argument("Projection leaves finite plane regions.");return p;}
std::vector<SU2Native3D::Edge> FinitePlaneReference::FeaturesIntersecting(Point a,Point b)const {
  if(chart.ErrorBound(a)>chart.Authority().maximum_error || chart.ErrorBound(b)>chart.Authority().maximum_error)
    throw std::invalid_argument("Finite plane feature query leaves its authority.");
  const auto x=Flat(chart.Coordinates(a)),y=Flat(chart.Coordinates(b));std::vector<SU2Native3D::Edge> found;
  for(auto edge:borders)if(SU2Native2D::SegmentsIntersect(x,y,Flat(nodes.at(edge[0])),Flat(nodes.at(edge[1]))))found.push_back(edge);
  return found;
}
size_t FinitePlaneReference::ResidentBytes()const {
  return transfer_memory::Add(sizeof(*this),reference->ResidentBytes(),transfer_memory::Bytes(components),transfer_memory::Bytes(borders),
      transfer_memory::Mul(nodes.size(),sizeof(decltype(nodes)::value_type)+4*sizeof(void*)));
}
} // namespace SU2NativeGeometry
