#include "adaptation/CNativeCurvedReference.hpp"
#include <iostream>
#include <iomanip>
using namespace SU2NativeCurved;
int main(){
 const Vec<3> query{0,.80499999999999994,.5};
 const std::array<Vec<3>,3> faces[2]={{{{0,0,0},{0,.61,1},{0,1.61,1}}},{{{0,0,0},{0,1.61,1},{0,1,0}}}};
 const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
 for(unsigned f=0;f<2;++f){std::array<Vec<3>,10> net;
  for(unsigned k=0;k<10;++k)for(unsigned d=0;d<3;++d){long double value=0;for(unsigned i=0;i<3;++i)value+=powers[k][i]*(long double)faces[f][i][d];net[k][d]=double(value/3);}
  double u=0,v=0;detail::ClosestOnTriangle(query,net[0],net[1],net[2],u,v);
  auto eval=[&](double a,double b){return detail::TrianglePatch(net,1-a-b,a,b);};
  auto derivatives=[&](double a,double b,Vec<3>& first,Vec<3>& second){detail::TriangleDerivatives(net,a,b,first,second);};
  const bool converged=detail::MinimizeTriangle(query,u,v,eval,derivatives);
  std::cout<<std::setprecision(17)<<f<<" converged="<<converged<<" u="<<u<<" v="<<v<<" distance="<<Norm<3>(Sub<3>(eval(u,v),query))<<'\n';
 }
}
