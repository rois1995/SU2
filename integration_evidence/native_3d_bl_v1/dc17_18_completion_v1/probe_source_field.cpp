#include "Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "Common/include/adaptation/CNativeDistributed3D.hpp"
#include "Common/include/adaptation/CNativeField3D.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int status=1;
  try {
    using namespace SU2Native3D;
    World world;if(world.size!=1 || argc<4 || argc>5)
      throw std::runtime_error("usage: probe sidecar admitted-donors receipt.jsonl [first-facet] (serial)");
    std::ifstream sidecar(argv[1]);std::ostringstream text;text<<sidecar.rdbuf();if(!sidecar)throw std::runtime_error("Cannot read reference.");
    const auto state=DecodeReference(text.str(),size_t(2)<<30);const auto& g=state.original->Parameters()->Geometry();
    std::ifstream input(argv[2],std::ios::binary|std::ios::ate);const auto size=input.tellg();
    if(!input || size<16 || size>(std::streamoff(1)<<30))throw std::runtime_error("Invalid diagnostic source snapshot size.");
    std::vector<char> bytes(size);input.seekg(0);input.read(bytes.data(),bytes.size());if(!input)throw std::runtime_error("Truncated source snapshot.");
    const auto& encoded=bytes;RecordStream decode(encoded);uint64_t version=0,count=0;decode(version,count);
    if(version!=1 || !count || count>2000000)throw std::runtime_error("Invalid source snapshot header.");
    std::vector<DonorCell> source(count);size_t original=0,extension=0;
    for(auto& row:source){decode(row);if(row.extension_component==UINT64_MAX)++original;else ++extension;}
    if(!decode.End() || !original || !extension)throw std::runtime_error("Incomplete admitted source snapshot.");
    std::vector<char>().swap(bytes);const auto components=AssociatedExtensions(source,*state.original);
    FrozenField field(std::move(source),{},false,components,{}, {},true,count);
    const size_t first=argc==5?std::stoull(argv[4]):0;
    if(first>=state.facets.size())throw std::runtime_error("Resume index leaves planned surface.");
    std::ofstream out(argv[3]);out<<std::setprecision(17);const auto start=world.seconds();
    size_t queries=0,chords=0;double reverseError=0;
    for(size_t index=first;index<state.facets.size();++index) {
      if(world.seconds()-start>180)throw std::runtime_error("Field diagnostic time exhausted; completed facets preserved.");
      const auto& f=state.facets[index];
      try {
        for(const auto& node:f.v) {
          const auto sample=field.Query(node.p);++queries;
          if(sample.extension_component!=UINT64_MAX)throw std::runtime_error("Original boundary point lost P1 priority.");
        }
        std::array<Point,7> points;
        for(unsigned k=0;k<3;++k){Edge e{f.v[k].id,f.v[(k+1)%3].id};std::sort(e.begin(),e.end());points[k]=g.Realize(g.EdgePoint(e,.5));}
        const std::array<std::array<double,2>,4> uv{{{.25,.25},{.5,.25},{.25,.5},{1./3,1./3}}};
        for(unsigned k=0;k<uv.size();++k)points[3+k]=g.Realize(g.FacetPoint(f.id,uv[k][0],uv[k][1]));
        for(auto p:points){field.Query(p);++queries;}
        for(unsigned k=0;k<3;++k) {
          const auto a=field.SensorEdgeLength(points[k],points[(k+1)%3]);
          const auto b=field.SensorEdgeLength(points[(k+1)%3],points[k]);chords+=2;
          if(!(std::isfinite(a)&&std::isfinite(b)&&a>0&&b>0))throw std::runtime_error("Invalid curved-child chord length.");
          const double relative=std::abs(a-b)/std::max(a,b);reverseError=std::max(reverseError,relative);
          if(relative>1e-12)throw std::runtime_error("Curved-child chord reversal disagrees.");
        }
      }catch(const std::exception& e) {
        out<<"{\"index\":"<<index<<",\"facet\":"<<f.id<<",\"passed\":false,\"reason\":"<<std::quoted(e.what())<<"}\n";out.flush();throw;
      }
      out<<"{\"index\":"<<index<<",\"facet\":"<<f.id<<",\"passed\":true,\"queries\":10,\"chords\":6}\n";out.flush();
      if(!out)throw std::runtime_error("Cannot preserve field receipt.");
      if(index%1024==0)std::cout<<"completed_facet="<<index<<" queries="<<queries<<" chords="<<chords<<std::endl;
    }
    const auto& stats=field.Statistics();
    std::cout<<std::setprecision(17)<<"complete=true first="<<first<<" facets="<<state.facets.size()-first<<" queries="<<queries<<" chords="<<chords
      <<" reverse_relative_error="<<reverseError<<" extended_queries="<<stats.extended_queries<<" extended_chord_pieces="<<stats.extended_edge_pieces
      <<" seconds="<<world.seconds()-start<<" mesh_constructed=false"<<std::endl;status=0;
  }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;}
  SU2_MPI::Finalize();return status;
}
