#include "active_edge_variant.hpp"
#include <iostream>
#include <iomanip>
using namespace SU2NativeCurved;
int main(){std::array<Vec<3>,10> source{{{{0.0107187521002556,0.004987338589743502,-0.01353298943882928}},{{0.005985127462594315,0.006749924753011305,-0.007145077741415979}},{{0.002380958890740337,-1.062527844e-13,-0.007634829294804406}},{{0.008876224025965773,0.005731130435994923,-0.011740583689208774}},{{0.007116715244831473,0.006422449570221775,-0.009522611208878595}},{{0.0047722882695313724,0.004506621610010489,-0.007314696685367057}},{{0.002590789684815837,0.0022499749175995997,-0.008016585346009058}},{{0.0038557028446380387,0.0016624461965103322,-0.010317907490540294}},{{0.007702020475297107,0.003465103699437577,-0.011869227014413298}},{{0.005547628727338193,0.004053221050335582,-0.009976586779429326}}}};Vec<3> query{0.002385697960607164,8.87537971106717e-06,-0.007634185327469742};
std::cout<<std::setprecision(21);
for(unsigned cap:{64u,4096u})for(double perturb:{-1e-6,0.,1e-6}){
auto net=source;const auto target=detail::NormalizeTriangleProjection(net,query);
double u=0,v=0;detail::ClosestOnTriangle(target,net[0],net[1],net[2],u,v);u+=perturb;detail::ClampSimplex(u,v);
unsigned work=1;bool converged=false,capped=false;
auto use=[&](){if(work>=cap)throw 1;++work;};
auto eval=[&](double a,double b){use();return detail::TrianglePatch(net,1-a-b,a,b);};
auto deriv=[&](double a,double b,Vec<3>& x,Vec<3>& y){use();detail::TriangleDerivatives(net,a,b,x,y);};
try{converged=detail::MinimizeTriangle(target,u,v,eval,deriv);}catch(int){capped=true;}
Vec<3> x,y;detail::TriangleDerivatives(source,u,v,x,y);Vec<3> normal{x[1]*y[2]-x[2]*y[1],x[2]*y[0]-x[0]*y[2],x[0]*y[1]-x[1]*y[0]};normal=Unit<3>(normal);
const auto p=detail::TrianglePatch(source,1-u-v,u,v);
std::cout<<"{\"cap\":"<<cap<<",\"perturb\":"<<perturb<<",\"work\":"<<work<<",\"converged\":"<<converged<<",\"capped\":"<<capped<<",\"u\":"<<u<<",\"v\":"<<v<<",\"point\":["<<p[0]<<','<<p[1]<<','<<p[2]<<"],\"normal\":["<<normal[0]<<','<<normal[1]<<','<<normal[2]<<"]}\n";
}}
