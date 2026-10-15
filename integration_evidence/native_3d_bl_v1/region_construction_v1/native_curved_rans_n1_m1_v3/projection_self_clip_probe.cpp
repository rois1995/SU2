#include <algorithm>
#include <cmath>
#include <iomanip>
#include "SU2_CFD/include/adaptation/ConvexClipping.hpp"
int main(){std::cout<<std::setprecision(20);for(unsigned i=0;i<8;++i){
 const double angle=i*std::acos(-1.)/4,next=(i+1)*std::acos(-1.)/4;
 double a[2]={0,0},b[2]={std::sin(angle),-std::cos(angle)},c[2]={std::sin(next),-std::cos(next)};
 convex_clip::Polygon source;source.InitTriangle(a,b,c);auto copy=source;
 for(int k=0;k<3;++k){const auto* x=source.x[k];const auto* y=source.x[(k+1)%3];copy.Clip(x[1]-y[1],y[0]-x[0],(y[1]-x[1])*x[0]-(y[0]-x[0])*x[1]);}
 bool same=copy.n==source.n;for(int k=0;k<std::min(copy.n,source.n);++k)for(int d=0;d<2;++d)same=same && copy.x[k][d]==source.x[k][d];
 std::cout<<"sector="<<i<<" same="<<same<<" n="<<copy.n<<'\n';if(!same){for(int k=0;k<copy.n;++k)std::cout<<copy.x[k][0]<<' '<<copy.x[k][1]<<'\n';}
}}
