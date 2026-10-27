#include "SU2_CFD/include/adaptation/ConvexClipping.hpp"
#include <iomanip>
#include <limits>
int main(){
  const double tiny=std::numeric_limits<double>::denorm_min();
  double v[4][3]={{.25,.5,-tiny},{.75,.5,0},{.25,.75,0},{.25,.5,1}};
  const double* p[4]={v[0],v[1],v[2],v[3]};
  convex_clip::Polyhedron poly;poly.InitTetrahedron(p);double before,after,c[3];poly.Moments(before,c);
  poly.Clip(0,0,1,0);poly.Clip(1,0,0,-.1);poly.Moments(after,c);
  std::cout<<std::setprecision(17)<<before<<" "<<after<<"\n";
  for(int i=0;i<poly.n;++i)std::cout<<poly.x[i][0]<<" "<<poly.x[i][1]<<" "<<poly.x[i][2]<<"\n";
}
