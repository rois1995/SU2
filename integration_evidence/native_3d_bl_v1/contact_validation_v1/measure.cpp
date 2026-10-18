// Saved-mesh validation only: no remeshing, metric construction or CFD.
#include "../../../Common/include/adaptation/CNativeMesh3D.hpp"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdexcept>
using namespace SU2Native3D;
struct Box{std::array<double,3> lo,hi;};
Box bounds(const Cell& c){Box b{{c.v[0].p.x,c.v[0].p.y,c.v[0].p.z},{c.v[0].p.x,c.v[0].p.y,c.v[0].p.z}};for(auto n:c.v){const double p[]={n.p.x,n.p.y,n.p.z};for(unsigned k=0;k<3;++k){b.lo[k]=std::min(b.lo[k],p[k]);b.hi[k]=std::max(b.hi[k],p[k]);}}return b;}
unsigned classification(const Cell& a,const Cell& b,const Box& x,const Box& y){
 for(unsigned k=0;k<3;++k)if(x.hi[k]<y.lo[k]||y.hi[k]<x.lo[k])return 0;
 auto shared=[](const Node& p,const Cell& c){for(auto q:c.v)if(p.id==q.id){if(p.p.x!=q.p.x||p.p.y!=q.p.y||p.p.z!=q.p.z)throw std::runtime_error("Shared coordinate mismatch");return true;}return false;};
 auto support=[&](const Cell& c,const Cell& other,unsigned k,double plane){bool any=false;for(auto n:c.v){const double p[]={n.p.x,n.p.y,n.p.z};if(p[k]==plane){any=true;if(!shared(n,other))return false;}}return any;};
 for(unsigned k=0;k<3;++k)if(x.hi[k]==y.lo[k]||y.hi[k]==x.lo[k]){const auto plane=x.hi[k]==y.lo[k]?x.hi[k]:y.hi[k];if(support(a,b,k,plane)||support(b,a,k,plane))return 1;}
 return 2;
}
int main(int argc,char** argv){try{
 if(argc!=3)throw std::runtime_error("Expected saved patch and classify/measure");const bool measure=std::string(argv[2])=="measure";if(!measure&&std::string(argv[2])!="classify")throw std::runtime_error("Unknown mode");
 std::ifstream input(argv[1]);std::string magic;size_t count,faces;input>>magic>>count>>faces;if(magic!="NATIVE_PRIVATE_PATCH_V1"||!count||count>20000)throw std::runtime_error("Bad bounded input");
 std::vector<Cell> cells(count);std::vector<Box> boxes;std::map<Id,Point> nodes;
 for(auto& c:cells){input>>c.id;for(auto& n:c.v){input>>n.id>>n.p.x>>n.p.y>>n.p.z;auto at=nodes.emplace(n.id,n.p);if(!at.second&&(at.first->second.x!=n.p.x||at.first->second.y!=n.p.y||at.first->second.z!=n.p.z))throw std::runtime_error("Conflicting mesh node");}if(!input)throw std::runtime_error("Truncated input");if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0))throw std::runtime_error("Nonpositive input");boxes.push_back(bounds(c));}
 const auto start=std::chrono::steady_clock::now();auto seconds=[&](){return std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();};
 uint64_t categories[3]={},calls=0;KernelStats stats;
 for(size_t i=0;i<count;++i){if(seconds()>90)throw std::runtime_error("Bounded validation replay exhausted90seconds");for(size_t j=i+1;j<count;++j){const auto kind=classification(cells[i],cells[j],boxes[i],boxes[j]);++categories[kind];if(measure&&kind){++calls;if(!ConformingDisjoint(cells[i],cells[j],&stats))throw std::runtime_error("Saved accepted pair rejected");}}}
 std::cout<<std::setprecision(17)<<"{\"cells\":"<<count<<",\"mode\":"<<std::quoted(argv[2])<<",\"pairs\":"<<count*(count-1)/2<<",\"strict_box_separated\":"<<categories[0]<<",\"shared_support_candidates\":"<<categories[1]<<",\"unresolved_candidates\":"<<categories[2]<<",\"predicate_calls\":"<<calls<<",\"exact_orientations\":"<<stats.exact_orientations<<",\"filtered_orientations\":"<<stats.filtered_orientations<<",\"seconds\":"<<seconds()<<"}\n";
 return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
