#include "CNativeCurvedReference.hpp"
#include <iostream>
#include <iomanip>
using namespace SU2NativeCurved;
int main(){std::array<std::array<Vec<3>,10>,2> source{{std::array<Vec<3>,10>{{Vec<3>{0.0107187521002556,0.004987338589743502,-0.01353298943882928},Vec<3>{0.005985127462594315,0.006749924753011305,-0.007145077741415979},Vec<3>{0.002380958890740337,-1.062527844e-13,-0.007634829294804406},Vec<3>{0.008876224025965773,0.005731130435994923,-0.011740583689208774},Vec<3>{0.007116715244831473,0.006422449570221775,-0.009522611208878595},Vec<3>{0.0047722882695313724,0.004506621610010489,-0.007314696685367057},Vec<3>{0.002590789684815837,0.0022499749175995997,-0.008016585346009058},Vec<3>{0.0038557028446380387,0.0016624461965103322,-0.010317907490540294},Vec<3>{0.007702020475297107,0.003465103699437577,-0.011869227014413298},Vec<3>{0.005547628727338193,0.004053221050335582,-0.009976586779429326}}},
std::array<Vec<3>,10>{{Vec<3>{-6.134604418e-14,-1.062527844e-13,-2.220446049e-16},Vec<3>{0.002380958890740337,-1.062527844e-13,-0.007634829294804406},Vec<3>{0.005985127462594315,0.006749924753011305,-0.007145077741415979},Vec<3>{-6.134590622944794e-14,-1.0625262339472095e-13,-0.002544943098268283},Vec<3>{0.0010876769410745942,-1.0625293350187024e-13,-0.005375938618652015},Vec<3>{0.002590789684815837,0.0022499749175995997,-0.008016585346009058},Vec<3>{0.0047722882695313724,0.004506621610010489,-0.007314696685367057},Vec<3>{0.003601495673574434,0.004726382603528558,-0.00497952260970985},Vec<3>{-6.132869694524023e-14,0.0022499749175996,-0.0023816925804721555},Vec<3>{0.0016187149166728397,0.0023082510533316354,-0.005190026895249503}}}}};
std::array<Vec<3>,6> queries{{Vec<3>{0.0024597140053985514,0.00014749340572871503,-0.007624127675562078},Vec<3>{0.0024597104012299797,0.00014748665580396192,-0.007624128165313632},Vec<3>{0.002459717609567123,0.00014750015565346815,-0.007624127185810525},Vec<3>{0.002459353588541366,0.0001468184132534033,-0.007624176650717417},Vec<3>{0.002460074422255737,0.00014816839820402678,-0.007624078700406739},Vec<3>{0.0024597500470842698,0.0001475609049762462,-0.007624122778046544}}};
std::cout<<std::setprecision(21);
for(unsigned f=0;f<2;++f)for(unsigned q=0;q<6;++q)for(unsigned cap:{64u,4096u}){
 auto net=source[f];const auto target=detail::NormalizeTriangleProjection(net,queries[q]);
 double u=0,v=0;detail::ClosestOnTriangle(target,net[0],net[1],net[2],u,v);
 unsigned work=1,firstStationary=0;bool converged=false,capped=false,haveDerivative=false;
 double lastU=0,lastV=0;Vec<3> lastSu{},lastSv{};
 auto use=[&](){if(work>=cap)throw 1;++work;};
 auto eval=[&](double a,double b){use();const auto point=detail::TrianglePatch(net,1-a-b,a,b);
  if(haveDerivative && a==lastU && b==lastV){const auto d=Sub<3>(point,target);
   const double gu=Dot<3>(lastSu,d),gv=Dot<3>(lastSv,d),norm2=Dot<3>(lastSu,lastSu)+Dot<3>(lastSv,lastSv);
   const double tolerance=1e-7*std::sqrt(Dot<3>(d,d)*norm2)+64*std::numeric_limits<double>::epsilon()*norm2;
   const double base=-a*gu-b*gv;
   if(!firstStationary && std::isfinite(tolerance) && std::min({base,base+gu,base+gv})>=-tolerance)firstStationary=work;
  }return point;};
 auto deriv=[&](double a,double b,Vec<3>& x,Vec<3>& y){use();detail::TriangleDerivatives(net,a,b,x,y);lastU=a;lastV=b;lastSu=x;lastSv=y;haveDerivative=true;};
 try{converged=detail::MinimizeTriangle(target,u,v,eval,deriv);}catch(int){capped=true;}
 std::cout<<"{\"facet\":"<<(f?35672:30335)<<",\"point\":"<<q<<",\"cap\":"<<cap<<",\"work\":"<<work<<",\"first_stationary_work\":"<<firstStationary<<",\"converged\":"<<converged<<",\"capped\":"<<capped<<",\"u\":"<<u<<",\"v\":"<<v<<"}\n";
}}
