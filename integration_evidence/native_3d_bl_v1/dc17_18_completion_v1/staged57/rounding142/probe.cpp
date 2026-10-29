#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <boost/multiprecision/cpp_int.hpp>
struct Point { double x,y,z; };
using Vec=std::array<double,3>;
Vec Values(Point p){return {p.x,p.y,p.z};}
Point ToPoint(Vec p){return {p[0],p[1],p[2]};}
void Finite(Point p){for(auto x:Values(p))if(!std::isfinite(x))throw std::invalid_argument("nonfinite");}
struct AtlasOps {void Work(size_t=1){}};
namespace v141 {
struct Range {long double lo=0,hi=0;};
Range Add(Range a,Range b){
  if(a.lo==0 && a.hi==0)return b;
  if(b.lo==0 && b.hi==0)return a;
  return {std::nextafter(a.lo+b.lo,-INFINITY),std::nextafter(a.hi+b.hi,INFINITY)};
}
Range Multiply(Range a,long double b){
  if(b==1)return a;
  if(b==-1)return {-a.hi,-a.lo};
  if((b==0 || (a.lo==0 && a.hi==0)) && std::isfinite(a.lo) && std::isfinite(a.hi) && std::isfinite(b))return {};
  return b>=0?Range{std::nextafter(a.lo*b,-INFINITY),std::nextafter(a.hi*b,INFINITY)}:
      Range{std::nextafter(a.hi*b,-INFINITY),std::nextafter(a.lo*b,INFINITY)};
}
Range Subtract(double a,double b){if(a==b && std::isfinite(a))return {};
  const auto x=static_cast<long double>(a)-b;return {std::nextafter(x,-INFINITY),std::nextafter(x,INFINITY)};}
using Box=std::array<Range,3>;
constexpr unsigned CubicIndex[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
Range Subtract(Range a,Range b){return Add(a,{-b.hi,-b.lo});}
Range Multiply(Range a,Range b) {
  if(a.lo==a.hi && std::isfinite(a.lo))return Multiply(b,a.lo);
  if(b.lo==b.hi && std::isfinite(b.lo))return Multiply(a,b.lo);
  const long double p[]={a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi};
  return {std::nextafter(*std::min_element(p,p+4),-INFINITY),std::nextafter(*std::max_element(p,p+4),INFINITY)};
}
Range DividePositive(Range a,long double d){return Multiply(a,Range{std::nextafter(1/d,-INFINITY),std::nextafter(1/d,INFINITY)});}
Box Exact(Point p){const auto v=Values(p);return {{{v[0],v[0]},{v[1],v[1]},{v[2],v[2]}}};}
double NormBound(const Box& a) {
  Range sum;
  for(const auto& r:a){const auto bound=std::max(std::abs(r.lo),std::abs(r.hi));sum=Add(sum,Multiply({bound,bound},bound));}
  const auto value=std::nextafter(std::sqrt(sum.hi),INFINITY);
  return std::nextafter(double(std::max(value,(long double)std::numeric_limits<double>::min())),INFINITY);
}
Box Difference(const Box& a,const Box& b){Box c;for(unsigned d=0;d<3;++d)c[d]=Subtract(a[d],b[d]);return c;}
Box CrossBox(const Box& a,const Box& b){Box c;for(unsigned d=0;d<3;++d)c[d]=Subtract(Multiply(a[(d+1)%3],b[(d+2)%3]),Multiply(a[(d+2)%3],b[(d+1)%3]));return c;}
Range DotBox(const Box& a,const Box& b){Range c;for(unsigned d=0;d<3;++d)c=Add(c,Multiply(a[d],b[d]));return c;}
Point BoxMiddle(const Box& box) {
  Vec p;for(unsigned d=0;d<3;++d)p[d]=double(.5L*box[d].lo+.5L*box[d].hi);
  auto out=ToPoint(p);Finite(out);return out;
}
struct TriangleDistanceBounds {double lower=0,upper=INFINITY;};
TriangleDistanceBounds TriangleDistance(const Box& point,const std::array<Point,3>& triangle,AtlasOps& op,bool lower_bound=true) {
  op.Work();const auto p=Values(BoxMiddle(point)),a=Values(triangle[0]),b=Values(triangle[1]),c=Values(triangle[2]);
  using Wide=std::array<long double,3>;Wide ab{},ac{},ap{};
  for(unsigned d=0;d<3;++d){ab[d]=(long double)b[d]-a[d];ac[d]=(long double)c[d]-a[d];ap[d]=(long double)p[d]-a[d];}
  auto dot=[](Wide x,Wide y){return x[0]*y[0]+x[1]*y[1]+x[2]*y[2];};
  long double best_score=INFINITY;double selected_u=0,selected_v=0;
  auto propose=[&](long double wb,long double wc) {
    if(!(wb>=0&&wc>=0&&wb+wc<=1))return;
    const double u=std::clamp(double(wb+wc),0.,1.),v=u?std::clamp(double(wc/(wb+wc)),0.,1.):0.;
    Wide delta;for(unsigned d=0;d<3;++d)delta[d]=ap[d]-u*((1-(long double)v)*ab[d]+v*ac[d]);
    const auto score=dot(delta,delta);
    if(score<best_score){best_score=score;selected_u=u;selected_v=v;}
  };
  propose(0,0);propose(1,0);propose(0,1);
  const auto aa=dot(ab,ab),bb=dot(ab,ac),cc=dot(ac,ac),x=dot(ap,ab),y=dot(ap,ac),det=aa*cc-bb*bb;
  if(det>0)propose((x*cc-y*bb)/det,(y*aa-x*bb)/det);
  if(aa>0)propose(std::clamp(x/aa,0.L,1.L),0);
  if(cc>0)propose(0,std::clamp(y/cc,0.L,1.L));
  Wide bc{},bp{};for(unsigned d=0;d<3;++d){bc[d]=(long double)c[d]-b[d];bp[d]=(long double)p[d]-b[d];}
  if(dot(bc,bc)>0){const auto t=std::clamp(dot(bp,bc)/dot(bc,bc),0.L,1.L);propose(1-t,t);}
  // Floating ranking only chooses a convex witness; certify that witness once before using its bound.
  Box witness;for(unsigned d=0;d<3;++d)witness[d]=Add(Multiply({a[d],a[d]},Subtract({1,1},{selected_u,selected_u})),
      Multiply(Add(Multiply({b[d],b[d]},Subtract({1,1},{selected_v,selected_v})),Multiply({c[d],c[d]},selected_v)),selected_u));
  const auto best=NormBound(Difference(point,witness));
  if(!lower_bound)return {0,best};
  const auto direction=Exact(BoxMiddle(Difference(point,witness)));const auto length=NormBound(direction);
  long double lower=INFINITY;
  for(auto vertex:triangle)lower=std::min(lower,DotBox(Difference(point,Exact(vertex)),direction).lo);
  if(!(length>0&&std::isfinite(length)&&lower>0))lower=0;
  else lower=std::nextafter(lower/length,-INFINITY);
  return {std::max(0.,std::nextafter(double(lower),-INFINITY)),best};
}
TriangleDistanceBounds OldTriangleDistance(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,true);}
TriangleDistanceBounds NewFull(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,true);}
TriangleDistanceBounds NewUpper(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,false);}
using Rational=boost::multiprecision::cpp_rational;
size_t checks=0;
void check(TriangleDistanceBounds b,Rational exact) {
  assert(std::isfinite(b.lower) && std::isfinite(b.upper) && b.lower>=0 && b.upper>=b.lower);
  assert(Rational(b.lower)*Rational(b.lower)<=exact);
  assert(Rational(b.upper)*Rational(b.upper)>=exact);
  ++checks;
}
int distance_check(){
  AtlasOps op;
  // Nearest points respectively face, three edges and three vertices of x+y<=4.
  const std::array<Point,7> queries={{{1,1,2},{2,-1,2},{-1,2,2},{3,3,2},{-1,-1,2},{5,-1,2},{-1,5,2}}};
  const int dist2[]={4,5,5,6,6,6,6};
  for(int perm=0;perm<3;++perm)for(int scale: {1,2,8})for(int shift: {-16,0,16}) {
    auto transform=[&](Point p){double a[]={p.x,p.y,p.z};return Point{double(shift+scale*a[perm]),double(shift+scale*a[(perm+1)%3]),double(shift+scale*a[(perm+2)%3])};};
    std::array<Point,3> tri={transform({0,0,0}),transform({4,0,0}),transform({0,4,0})};
    for(size_t i=0;i<queries.size();++i){auto p=transform(queries[i]);auto box=Exact(p);Rational exact=dist2[i]*scale*scale;
      check(OldTriangleDistance(box,tri,op),exact);check(NewFull(box,tri,op),exact);check(NewUpper(box,tri,op),exact);
      // Every corner is a possible query in the same nonzero-width box.
      for(auto& r:box){r.lo-=.125L*scale;r.hi+=.125L*scale;}
      auto old=OldTriangleDistance(box,tri,op),full=NewFull(box,tri,op),upper=NewUpper(box,tri,op);
      for(unsigned mask=0;mask<8;++mask){
        Rational original[3];for(unsigned d=0;d<3;++d)original[(perm+d)%3]=(Rational(mask&(1u<<d)?box[d].hi:box[d].lo)-shift)/scale;
        auto x=original[0],y=original[1],z=original[2];Rational squared=z*z;
        if(i==1)squared+=y*y; if(i==2)squared+=x*x;
        if(i==3)squared+=(x+y-4)*(x+y-4)/2;
        if(i==4)squared+=x*x+y*y; if(i==5)squared+=(x-4)*(x-4)+y*y; if(i==6)squared+=x*x+(y-4)*(y-4);
        squared*=scale*scale;check(old,squared);check(full,squared);check(upper,squared);
      }
    }
  }
  std::array<Point,3> tri={Point{.58904233025368091,.38581769422093798,-.03112997494669438},Point{.56530953543327178,.39977586224200762,-.03225505132526349},Point{.56375529165422267,.36510764045479471,-.032187514520798287}};
  std::array<Box,64> points;for(size_t i=0;i<points.size();++i){double u=(i%8)/8.,v=(i/8)/8.;points[i]=Exact({tri[0].x+(tri[1].x-tri[0].x)*u+(tri[2].x-tri[0].x)*v,tri[0].y+(tri[1].y-tri[0].y)*u+(tri[2].y-tri[0].y)*v,tri[0].z+(tri[1].z-tri[0].z)*u+(tri[2].z-tri[0].z)*v+.00013});}
  volatile double sink=0;auto time=[&](auto f){auto start=std::chrono::steady_clock::now();for(unsigned repeat=0;repeat<400;++repeat)for(const auto& p:points)sink+=f(p,tri,op).upper;return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();};
  std::cout<<"checks="<<checks<<" old_seconds="<<time(OldTriangleDistance)<<" new_full_seconds="<<time(NewFull)<<" new_upper_seconds="<<time(NewUpper)<<" sink="<<sink<<"\n";return 0;
}

}
namespace v142 {
long double Adjacent(long double value,bool upward) {
#if defined(__x86_64__) && defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__==64
  static_assert(std::numeric_limits<long double>::radix==2 && std::numeric_limits<long double>::max_exponent==16384,
                "Native outward stepping assumes the x87 extended format.");
  struct Bits {uint64_t mantissa;uint16_t sign_exponent;};
  Bits bits{};std::memcpy(&bits,&value,10);const unsigned exponent=bits.sign_exponent&0x7fff;
  // Only ordinary normals: preserve the standard library's exceptional-range behavior below.
  if(exponent>1 && exponent<0x7ffe && (bits.mantissa>>63)) {
    if(upward==!(bits.sign_exponent&0x8000)) {
      if(bits.mantissa==UINT64_MAX){bits.mantissa=uint64_t(1)<<63;++bits.sign_exponent;}
      else ++bits.mantissa;
    } else {
      if(bits.mantissa==(uint64_t(1)<<63)){bits.mantissa=UINT64_MAX;--bits.sign_exponent;}
      else --bits.mantissa;
    }
    std::memcpy(&value,&bits,10);return value;
  }
#endif
  return std::nextafter(value,upward?INFINITY:-INFINITY);
}
struct Range {long double lo=0,hi=0;};
Range Add(Range a,Range b){
  if(a.lo==0 && a.hi==0)return b;
  if(b.lo==0 && b.hi==0)return a;
  return {Adjacent(a.lo+b.lo,false),Adjacent(a.hi+b.hi,true)};
}
Range Multiply(Range a,long double b){
  if(b==1)return a;
  if(b==-1)return {-a.hi,-a.lo};
  if((b==0 || (a.lo==0 && a.hi==0)) && std::isfinite(a.lo) && std::isfinite(a.hi) && std::isfinite(b))return {};
  return b>=0?Range{Adjacent(a.lo*b,false),Adjacent(a.hi*b,true)}:
      Range{Adjacent(a.hi*b,false),Adjacent(a.lo*b,true)};
}
Range Subtract(double a,double b){if(a==b && std::isfinite(a))return {};
  const auto x=static_cast<long double>(a)-b;return {Adjacent(x,false),Adjacent(x,true)};}
using Box=std::array<Range,3>;
constexpr unsigned CubicIndex[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
Range Subtract(Range a,Range b){return Add(a,{-b.hi,-b.lo});}
Range Multiply(Range a,Range b) {
  if(a.lo==a.hi && std::isfinite(a.lo))return Multiply(b,a.lo);
  if(b.lo==b.hi && std::isfinite(b.lo))return Multiply(a,b.lo);
  const long double p[]={a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi};
  return {Adjacent(*std::min_element(p,p+4),false),Adjacent(*std::max_element(p,p+4),true)};
}
Range DividePositive(Range a,long double d){return Multiply(a,Range{Adjacent(1/d,false),Adjacent(1/d,true)});}
Box Exact(Point p){const auto v=Values(p);return {{{v[0],v[0]},{v[1],v[1]},{v[2],v[2]}}};}
double NormBound(const Box& a) {
  Range sum;
  for(const auto& r:a){const auto bound=std::max(std::abs(r.lo),std::abs(r.hi));sum=Add(sum,Multiply({bound,bound},bound));}
  const auto value=std::nextafter(std::sqrt(sum.hi),INFINITY);
  return std::nextafter(double(std::max(value,(long double)std::numeric_limits<double>::min())),INFINITY);
}
Box Difference(const Box& a,const Box& b){Box c;for(unsigned d=0;d<3;++d)c[d]=Subtract(a[d],b[d]);return c;}
Box CrossBox(const Box& a,const Box& b){Box c;for(unsigned d=0;d<3;++d)c[d]=Subtract(Multiply(a[(d+1)%3],b[(d+2)%3]),Multiply(a[(d+2)%3],b[(d+1)%3]));return c;}
Range DotBox(const Box& a,const Box& b){Range c;for(unsigned d=0;d<3;++d)c=Add(c,Multiply(a[d],b[d]));return c;}
Point BoxMiddle(const Box& box) {
  Vec p;for(unsigned d=0;d<3;++d)p[d]=double(.5L*box[d].lo+.5L*box[d].hi);
  auto out=ToPoint(p);Finite(out);return out;
}
struct TriangleDistanceBounds {double lower=0,upper=INFINITY;};
TriangleDistanceBounds TriangleDistance(const Box& point,const std::array<Point,3>& triangle,AtlasOps& op,bool lower_bound=true) {
  op.Work();const auto p=Values(BoxMiddle(point)),a=Values(triangle[0]),b=Values(triangle[1]),c=Values(triangle[2]);
  using Wide=std::array<long double,3>;Wide ab{},ac{},ap{};
  for(unsigned d=0;d<3;++d){ab[d]=(long double)b[d]-a[d];ac[d]=(long double)c[d]-a[d];ap[d]=(long double)p[d]-a[d];}
  auto dot=[](Wide x,Wide y){return x[0]*y[0]+x[1]*y[1]+x[2]*y[2];};
  long double best_score=INFINITY;double selected_u=0,selected_v=0;
  auto propose=[&](long double wb,long double wc) {
    if(!(wb>=0&&wc>=0&&wb+wc<=1))return;
    const double u=std::clamp(double(wb+wc),0.,1.),v=u?std::clamp(double(wc/(wb+wc)),0.,1.):0.;
    Wide delta;for(unsigned d=0;d<3;++d)delta[d]=ap[d]-u*((1-(long double)v)*ab[d]+v*ac[d]);
    const auto score=dot(delta,delta);
    if(score<best_score){best_score=score;selected_u=u;selected_v=v;}
  };
  propose(0,0);propose(1,0);propose(0,1);
  const auto aa=dot(ab,ab),bb=dot(ab,ac),cc=dot(ac,ac),x=dot(ap,ab),y=dot(ap,ac),det=aa*cc-bb*bb;
  if(det>0)propose((x*cc-y*bb)/det,(y*aa-x*bb)/det);
  if(aa>0)propose(std::clamp(x/aa,0.L,1.L),0);
  if(cc>0)propose(0,std::clamp(y/cc,0.L,1.L));
  Wide bc{},bp{};for(unsigned d=0;d<3;++d){bc[d]=(long double)c[d]-b[d];bp[d]=(long double)p[d]-b[d];}
  if(dot(bc,bc)>0){const auto t=std::clamp(dot(bp,bc)/dot(bc,bc),0.L,1.L);propose(1-t,t);}
  // Floating ranking only chooses a convex witness; certify that witness once before using its bound.
  Box witness;for(unsigned d=0;d<3;++d)witness[d]=Add(Multiply({a[d],a[d]},Subtract({1,1},{selected_u,selected_u})),
      Multiply(Add(Multiply({b[d],b[d]},Subtract({1,1},{selected_v,selected_v})),Multiply({c[d],c[d]},selected_v)),selected_u));
  const auto best=NormBound(Difference(point,witness));
  if(!lower_bound)return {0,best};
  const auto direction=Exact(BoxMiddle(Difference(point,witness)));const auto length=NormBound(direction);
  long double lower=INFINITY;
  for(auto vertex:triangle)lower=std::min(lower,DotBox(Difference(point,Exact(vertex)),direction).lo);
  if(!(length>0&&std::isfinite(length)&&lower>0))lower=0;
  else lower=std::nextafter(lower/length,-INFINITY);
  return {std::max(0.,std::nextafter(double(lower),-INFINITY)),best};
}
TriangleDistanceBounds OldTriangleDistance(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,true);}
TriangleDistanceBounds NewFull(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,true);}
TriangleDistanceBounds NewUpper(const Box& p,const std::array<Point,3>& t,AtlasOps& o){return TriangleDistance(p,t,o,false);}
using Rational=boost::multiprecision::cpp_rational;
size_t checks=0;
void check(TriangleDistanceBounds b,Rational exact) {
  assert(std::isfinite(b.lower) && std::isfinite(b.upper) && b.lower>=0 && b.upper>=b.lower);
  assert(Rational(b.lower)*Rational(b.lower)<=exact);
  assert(Rational(b.upper)*Rational(b.upper)>=exact);
  ++checks;
}
int distance_check(){
  AtlasOps op;
  // Nearest points respectively face, three edges and three vertices of x+y<=4.
  const std::array<Point,7> queries={{{1,1,2},{2,-1,2},{-1,2,2},{3,3,2},{-1,-1,2},{5,-1,2},{-1,5,2}}};
  const int dist2[]={4,5,5,6,6,6,6};
  for(int perm=0;perm<3;++perm)for(int scale: {1,2,8})for(int shift: {-16,0,16}) {
    auto transform=[&](Point p){double a[]={p.x,p.y,p.z};return Point{double(shift+scale*a[perm]),double(shift+scale*a[(perm+1)%3]),double(shift+scale*a[(perm+2)%3])};};
    std::array<Point,3> tri={transform({0,0,0}),transform({4,0,0}),transform({0,4,0})};
    for(size_t i=0;i<queries.size();++i){auto p=transform(queries[i]);auto box=Exact(p);Rational exact=dist2[i]*scale*scale;
      check(OldTriangleDistance(box,tri,op),exact);check(NewFull(box,tri,op),exact);check(NewUpper(box,tri,op),exact);
      // Every corner is a possible query in the same nonzero-width box.
      for(auto& r:box){r.lo-=.125L*scale;r.hi+=.125L*scale;}
      auto old=OldTriangleDistance(box,tri,op),full=NewFull(box,tri,op),upper=NewUpper(box,tri,op);
      for(unsigned mask=0;mask<8;++mask){
        Rational original[3];for(unsigned d=0;d<3;++d)original[(perm+d)%3]=(Rational(mask&(1u<<d)?box[d].hi:box[d].lo)-shift)/scale;
        auto x=original[0],y=original[1],z=original[2];Rational squared=z*z;
        if(i==1)squared+=y*y; if(i==2)squared+=x*x;
        if(i==3)squared+=(x+y-4)*(x+y-4)/2;
        if(i==4)squared+=x*x+y*y; if(i==5)squared+=(x-4)*(x-4)+y*y; if(i==6)squared+=x*x+(y-4)*(y-4);
        squared*=scale*scale;check(old,squared);check(full,squared);check(upper,squared);
      }
    }
  }
  std::array<Point,3> tri={Point{.58904233025368091,.38581769422093798,-.03112997494669438},Point{.56530953543327178,.39977586224200762,-.03225505132526349},Point{.56375529165422267,.36510764045479471,-.032187514520798287}};
  std::array<Box,64> points;for(size_t i=0;i<points.size();++i){double u=(i%8)/8.,v=(i/8)/8.;points[i]=Exact({tri[0].x+(tri[1].x-tri[0].x)*u+(tri[2].x-tri[0].x)*v,tri[0].y+(tri[1].y-tri[0].y)*u+(tri[2].y-tri[0].y)*v,tri[0].z+(tri[1].z-tri[0].z)*u+(tri[2].z-tri[0].z)*v+.00013});}
  volatile double sink=0;auto time=[&](auto f){auto start=std::chrono::steady_clock::now();for(unsigned repeat=0;repeat<400;++repeat)for(const auto& p:points)sink+=f(p,tri,op).upper;return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();};
  std::cout<<"checks="<<checks<<" old_seconds="<<time(OldTriangleDistance)<<" new_full_seconds="<<time(NewFull)<<" new_upper_seconds="<<time(NewUpper)<<" sink="<<sink<<"\n";return 0;
}

}
namespace v142 {
using Rational=boost::multiprecision::cpp_rational;
size_t interval_checks=0;
void contains(Range interval,const Rational& exact) {
  assert(!std::isnan(interval.lo) && !std::isnan(interval.hi));
  assert(interval.lo<=interval.hi);
  assert(std::isfinite(interval.lo) ? Rational(interval.lo)<=exact : interval.lo==-INFINITY);
  assert(std::isfinite(interval.hi) ? Rational(interval.hi)>=exact : interval.hi==INFINITY);
  ++interval_checks;
}
void interval_check() {
  const auto tiny=std::numeric_limits<long double>::denorm_min();
  assert(tiny>0 && std::numeric_limits<long double>::is_iec559);
  const auto large=std::numeric_limits<long double>::max()/4;
  const Range intervals[]={{0,0},{-0.L,0.L},{1,1},{-1,-1},{-2,3},{tiny,tiny},{-tiny,tiny},{large,large},{-large,large}};
  for(auto a:intervals)for(auto b:intervals) {
    contains(Add(a,b),Rational(a.lo)+Rational(b.lo));contains(Add(a,b),Rational(a.hi)+Rational(b.hi));
    for(auto x:{a.lo,a.hi})for(auto y:{b.lo,b.hi})contains(Multiply(a,b),Rational(x)*Rational(y));
  }
  for(auto a:intervals)for(auto b:{0.L,-0.L,1.L,-1.L,.5L,tiny,large})
    for(auto x:{a.lo,a.hi})contains(Multiply(a,b),Rational(x)*Rational(b));
  const double points[]={0.,-0.,1.,-1.,std::numeric_limits<double>::denorm_min(),std::numeric_limits<double>::max()};
  for(double a:points)for(double b:points)contains(Subtract(a,b),Rational(a)-Rational(b));
  auto exact_zero=Multiply(Range{0,0},Range{-2,3});assert(exact_zero.lo==0 && exact_zero.hi==0);
  auto underflow=Multiply(Range{tiny,tiny},.5L);
  assert(underflow.lo<=0 && underflow.hi>=tiny);
  assert(!std::isfinite(Multiply(Range{INFINITY,INFINITY},0.L).hi));
  assert(!std::isfinite(Multiply(Range{0,0},Range{INFINITY,INFINITY}).hi));
  assert(!std::isfinite(Multiply(Range{NAN,NAN},0.L).hi));
  std::cout << "exact rational enclosure interval_checks " << interval_checks << "; zero/unit identities, genuine underflow and nonfinite paths pass\n";
}

}
struct Raw {uint64_t m;uint16_t e;};
long double make(uint64_t m,uint16_t e){Raw b{m,e};long double x=0;std::memcpy(&x,&b,10);return x;}
bool same(long double x,long double y){unsigned char a[10],b[10];std::memcpy(a,&x,10);std::memcpy(b,&y,10);return std::memcmp(a,b,10)==0;}
size_t neighbors=0;
void compare(long double x){for(bool up:{false,true}){const auto a=v142::Adjacent(x,up),b=std::nextafter(x,up?INFINITY:-INFINITY);assert((std::isnan(a)&&std::isnan(b))||same(a,b));++neighbors;}}
int main(){
#if defined(__x86_64__) && defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__==64
  const uint64_t mantissas[]={0x8000000000000000ULL,0x8000000000000001ULL,0xaaaaaaaaaaaaaaaaULL,0xfffffffffffffffeULL,0xffffffffffffffffULL};
  for(unsigned exponent=1;exponent<0x7fff;++exponent)for(auto m:mantissas)for(unsigned sign:{0u,0x8000u})compare(make(m,exponent|sign));
  for(auto m:{0ULL,1ULL,0x7fffffffffffffffULL})for(unsigned sign:{0u,0x8000u})compare(make(m,sign));
  std::mt19937_64 random(142);for(unsigned i=0;i<100000;++i){auto exponent=1+random()%0x7ffe;auto m=random()|0x8000000000000000ULL;compare(make(m,exponent|((random()&1)?0x8000:0)));}
#endif
  compare(0.L);compare(-0.L);compare(INFINITY);compare(-INFINITY);compare(std::numeric_limits<long double>::quiet_NaN());
  std::cout<<"adjacent_comparisons="<<neighbors<<"\n";
  v142::interval_check();v141::distance_check();v142::distance_check();
}
