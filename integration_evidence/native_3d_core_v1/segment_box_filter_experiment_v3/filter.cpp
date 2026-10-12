#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
using Point=std::array<double,3>;
using Box=std::array<double,6>;
int Side(Point a,Point b,double x,double y,size_t i,size_t j) {
  const long double p=(static_cast<long double>(b[i])-a[i])*(static_cast<long double>(y)-a[j]);
  const long double q=(static_cast<long double>(b[j])-a[j])*(static_cast<long double>(x)-a[i]);
  const auto determinant=p-q,permanent=std::abs(p)+std::abs(q);
  const auto epsilon=std::numeric_limits<long double>::epsilon();
  if(std::isfinite(determinant) && std::isfinite(permanent) &&
     permanent>std::numeric_limits<long double>::min()/epsilon &&
     std::abs(determinant)>64*epsilon*permanent) return determinant>0 ? 1 : -1;
  return 0; // Unresolved signs retain the exact downstream predicate.
}
bool MayIntersect(Point a,Point b,Box box) {
  for(size_t i=0;i<3;++i) {
    if(!std::isfinite(a[i]) || !std::isfinite(b[i]) || !std::isfinite(box[i]) || !std::isfinite(box[i+3]) || box[i]>box[i+3])
      return true;
  }
  for(size_t i=0;i<3;++i)
    if(std::min(a[i],b[i])>box[i+3] || std::max(a[i],b[i])<box[i])return false;
  for(size_t i=0;i<3;++i)for(size_t j=i+1;j<3;++j) {
    const auto di=static_cast<long double>(b[i])-a[i],dj=static_cast<long double>(b[j])-a[j];
    if(!std::isfinite(di) || !std::isfinite(dj))return true;
    const double xmin=box[i+(dj<=0 ? 0 : 3)],ymin=box[j+(di>=0 ? 0 : 3)];
    const double xmax=box[i+(dj<=0 ? 3 : 0)],ymax=box[j+(di>=0 ? 3 : 0)];
    if(Side(a,b,xmin,ymin,i,j)>0 || Side(a,b,xmax,ymax,i,j)<0)return false;
  }
  return true;
}
int main() {
  uint64_t id,bits;std::array<double,12> v;
  while(std::cin>>id) {
    for(auto& x:v) {if(!(std::cin>>bits))return 2;std::memcpy(&x,&bits,sizeof x);}
    Point a{v[0],v[1],v[2]},b{v[3],v[4],v[5]};Box box{v[6],v[7],v[8],v[9],v[10],v[11]};
    std::cout<<id<<' '<<MayIntersect(a,b,box)<<char(10);
  }
}
