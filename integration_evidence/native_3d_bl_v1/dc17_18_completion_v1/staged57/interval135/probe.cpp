#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <boost/multiprecision/cpp_int.hpp>
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

using Rational=boost::multiprecision::cpp_rational;
size_t checks=0;
void contains(Range interval,const Rational& exact) {
  assert(!std::isnan(interval.lo) && !std::isnan(interval.hi));
  assert(interval.lo<=interval.hi);
  assert(std::isfinite(interval.lo) ? Rational(interval.lo)<=exact : interval.lo==-INFINITY);
  assert(std::isfinite(interval.hi) ? Rational(interval.hi)>=exact : interval.hi==INFINITY);
  ++checks;
}
int main() {
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
  std::cout << "exact rational enclosure checks " << checks << "; zero/unit identities, genuine underflow and nonfinite paths pass\n";
}
