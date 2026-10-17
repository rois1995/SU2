// Geometry-only certificate helper; reuses the native exact predicates, never initializes CFD.
#include "Common/include/adaptation/CNativeMesh3D.hpp"
#include <cmath>
#include <chrono>
#include <map>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
using namespace SU2Native3D;
int main(int argc,char** argv) {
  try {
    if(argc!=3)throw std::invalid_argument("usage: support_planes INPUT OUTPUT");
    std::ifstream in(argv[1],std::ios::binary);uint64_t points=0,candidates=0;
    in.read(reinterpret_cast<char*>(&points),8);in.read(reinterpret_cast<char*>(&candidates),8);
    if(!in || !points || points>1000000 || candidates>1000000)throw std::runtime_error("Invalid support certificate header.");
    double stopLower=0,maximumSeconds=0;
    in.read(reinterpret_cast<char*>(&stopLower),8);in.read(reinterpret_cast<char*>(&maximumSeconds),8);
    if(!in || !std::isfinite(stopLower) || !(stopLower>0) || !(maximumSeconds>0 && maximumSeconds<=300))
      throw std::runtime_error("Invalid support certificate work control.");
    auto readPoint=[&]() {
      Point p;double x[3];in.read(reinterpret_cast<char*>(x),sizeof(x));
      if(!in || !std::isfinite(x[0]) || !std::isfinite(x[1]) || !std::isfinite(x[2]))throw std::runtime_error("Invalid certificate point.");
      p={x[0],x[1],x[2]};return p;
    };
    std::vector<Point> wall;wall.reserve(points);for(uint64_t i=0;i<points;++i)wall.push_back(readPoint());
    struct Record {uint64_t face,cell;Point a,b,c,apex,cone;double lower;};
    std::vector<Record> records;records.reserve(candidates);
    for(uint64_t i=0;i<candidates;++i) {
      Record r;in.read(reinterpret_cast<char*>(&r.face),8);in.read(reinterpret_cast<char*>(&r.cell),8);
      r.a=readPoint();r.b=readPoint();r.c=readPoint();r.apex=readPoint();r.cone=readPoint();
      in.read(reinterpret_cast<char*>(&r.lower),8);
      if(!in || !std::isfinite(r.lower) || r.lower<0)throw std::runtime_error("Invalid lower contribution.");
      records.push_back(r);
    }
    if(in.peek()!=std::char_traits<char>::eof())throw std::runtime_error("Trailing support certificate data.");
    std::ofstream out(argv[2]);out<<"face,cell,supporting,inside,tests,filtered,exact,lower_sum\n";KernelStats stats;
    std::map<uint64_t,double> selected;long double lowerSum=0;uint64_t completed=0;
    const auto start=std::chrono::steady_clock::now();std::string stop="complete";bool timed=false;
    for(const auto& r:records) {
      const auto face=r.face,cell=r.cell;const auto a=r.a,b=r.b,c=r.c,apex=r.apex,cone=r.cone;
      const auto filtered=stats.filtered_orientations,exact=stats.exact_orientations;
      const auto fluid=Orientation(a,b,c,apex,&stats);if(fluid==0)throw std::runtime_error("Degenerate original boundary cell.");
      bool supporting=true;uint64_t tests=0;
      for(const auto& q:wall) {
        if((q.x==a.x && q.y==a.y && q.z==a.z) || (q.x==b.x && q.y==b.y && q.z==b.z) ||
           (q.x==c.x && q.y==c.y && q.z==c.z))continue;
        if((tests%1024)==0 && std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()>maximumSeconds){timed=true;break;}
        ++tests;const auto side=Orientation(a,b,c,q,&stats);
        if(side!=0 && (side>0)==(fluid>0)){supporting=false;break;}
      }
      if(timed){stop="time_budget";break;}
      Tetrahedron source{a,b,c,apex};if(fluid<0)std::swap(source[0],source[1]);
      std::array<long double,4> weights{};const bool inside=Barycentric(source,cone,weights,&stats);
      if(supporting && inside){auto& old=selected[cell];if(r.lower>old){lowerSum+=r.lower-old;old=r.lower;}}
      out<<face<<','<<cell<<','<<supporting<<','<<inside<<','<<tests<<','<<stats.filtered_orientations-filtered
         <<','<<stats.exact_orientations-exact<<','<<lowerSum<<std::endl;
      if(!out)throw std::runtime_error("Cannot save support certificate row.");
      ++completed;if(lowerSum>stopLower){stop="lower_threshold";break;}
    }
    std::cout<<"completed="<<completed<<" stop="<<stop<<" lower="<<lowerSum<<" filtered="<<stats.filtered_orientations
             <<" exact="<<stats.exact_orientations<<'\n';
  }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
