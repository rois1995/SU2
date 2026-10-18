#include "Common/include/adaptation/CNativeMesh3D.hpp"
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace SU2Native3D;
int main(int argc,char** argv) {
 try {
  if(argc!=2)throw std::runtime_error("Expected saved edge witness");
  std::ifstream input(argv[1]);std::string magic;size_t count=0;Id aid=0,bid=0;Point a,b;
  input>>magic>>count>>aid>>bid>>a.x>>a.y>>a.z>>b.x>>b.y>>b.z;
  if(magic!="NATIVE_METRIC_EDGE_WITNESS_V1" || !count || count>8192)throw std::runtime_error("Invalid witness");
  std::vector<Tetrahedron> donors(count);size_t expected=0;
  for(auto& tet:donors) {
   Id cell;input>>cell;bool first=false,last=false;
   for(auto& p:tet){Id id;input>>id>>p.x>>p.y>>p.z;first|=id==aid;last|=id==bid;for(unsigned j=0;j<6;++j){double unused;input>>unused;}}
   if(!input || !first)throw std::runtime_error("Truncated or inconsistent star witness");expected+=last;
  }
  size_t closed=0;for(const auto& t:donors)closed+=IntersectsSegment(t,a,b);
  constexpr unsigned repeats=32;KernelStats stats;size_t positive=0;const auto begin=std::chrono::steady_clock::now();
  for(unsigned j=0;j<repeats;++j)for(const auto& t:donors)positive+=IntersectsSegmentPositiveLength(t,a,b,&stats);
  const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
  if(closed!=count || positive!=repeats*expected)throw std::runtime_error("Metric support changed");
  std::cout<<std::setprecision(17)<<"{\"source_cells\":"<<count<<",\"closed\":"<<closed<<",\"positive_per_pass\":"<<expected<<",\"repeats\":"<<repeats<<",\"seconds\":"<<seconds<<",\"exact_orientations\":"<<stats.exact_orientations<<",\"filtered_orientations\":"<<stats.filtered_orientations<<"}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
