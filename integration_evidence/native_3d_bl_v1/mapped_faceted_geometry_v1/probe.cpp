#include "Common/include/adaptation/CNativePointConstruction3D.hpp"
#include <iomanip>
#include <iostream>
#include <limits>
using namespace SU2Native3D;
void PointJson(Point p){std::cout<<'['<<p.x<<','<<p.y<<','<<p.z<<']';}
int main(){
 std::cout<<std::setprecision(17)<<'[';bool first=true;
 for(double s:{1e-300,1e-100,1.,1e100,1e300})for(double t:{.1,.3,.37,.9})for(bool edge:{false,true}) {
  const Point a{3*s,-5*s,7*s},b{-2*s,4*s,-3*s},c{5*s,6*s,-s};
  const ParametricPoint model{a,b,c,t,edge?0:.05};const auto box=ParametricPointBounds(model);
  if(!first)std::cout<<',';first=false;
  std::cout<<"{\"origin\":";PointJson(a);std::cout<<",\"first\":";PointJson(b);std::cout<<",\"second\":";PointJson(c);
  std::cout<<",\"u\":"<<model.u<<",\"v\":"<<model.v<<",\"bounds\":[";PointJson(box[0]);std::cout<<',';PointJson(box[1]);
  std::cout<<"],\"orientation\":[";for(unsigned axis=0;axis<3;++axis){if(axis)std::cout<<',';std::cout<<ParametricOrientation({a},{b},model,axis);}std::cout<<"]}";
 }
 std::cout<<"]\n";
}
