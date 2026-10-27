#include "/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/Common/include/adaptation/CNativeMesh3D.hpp"
#include <iostream>
#include <iomanip>
#include <stdexcept>
using namespace SU2Native3D;
int main(){try{size_t n;if(!(std::cin>>n)||n>4096)throw std::runtime_error("invalid count");std::cout<<std::setprecision(17);
for(size_t i=0;i<n;++i){Id id;Tetrahedron t;if(!(std::cin>>id))throw std::runtime_error("missing id");for(auto& p:t)if(!(std::cin>>p.x>>p.y>>p.z))throw std::runtime_error("missing point");
const auto orientation=Orientation(t[0],t[1],t[2],t[3]);const auto m=Measure(t,Tensor{4,0,0,4,0,4});std::cout<<id<<' '<<orientation<<' '<<m.mean_ratio<<' '<<m.minimum_scaled_jacobian<<' '<<m.maximum_edge<<'\n';}
return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
