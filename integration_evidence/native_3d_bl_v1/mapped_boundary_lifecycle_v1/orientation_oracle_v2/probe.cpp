#include "Common/src/adaptation/CNativePredicates3D_internal.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
int main(){using namespace SU2Native3D;std::string word;while(std::cin>>word){std::array<Point,3>a{},b{};
for(auto* face:{&a,&b})for(auto& p:*face)for(double* x:{&p.x,&p.y,&p.z}){*x=std::strtod(word.c_str(),nullptr);if(!(face==&b && &p==&b[2] && x==&p.z))if(!(std::cin>>word))return 2;}
KernelStats stats;const int sign=SurfaceOrientationSign(a,b,&stats);std::cout<<sign<<' '<<stats.filtered_orientations<<' '<<stats.exact_orientations<<'\n';}}
