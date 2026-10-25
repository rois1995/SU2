#include "Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "Common/include/parallelization/mpi_structure.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int status=0;
  try {
    if(argc!=3 && argc!=4)throw std::runtime_error("usage: probe sidecar output.jsonl [first-query]");
    const size_t first=argc==4?std::stoull(argv[3]):0;
    std::ifstream in(argv[1]);std::ostringstream buffer;buffer<<in.rdbuf();if(!in)throw std::runtime_error("Cannot read sidecar.");
    auto state=SU2Native3D::DecodeReference(buffer.str(),size_t(2)<<30);
    if(!state.original->Mapped())throw std::runtime_error("Expected mapped reference.");
    const auto& geometry=state.original->Parameters()->Geometry();
    std::ofstream out(argv[2]);out<<std::setprecision(17);
    const size_t stride=std::max(size_t(1),state.facets.size()/64),planned=5*((state.facets.size()+stride-1)/stride);
    if(first>=planned)throw std::runtime_error("Resume index leaves the planned query set.");
    size_t completed=0,work=0,index=0;double seconds=0;
    for(size_t i=0;i<state.facets.size();i+=stride) {
      if(index+5<=first){index+=5;continue;}
      const auto& f=state.facets[i];const auto binding=geometry.FacetPoint(f.id,.25,.25);const auto point=geometry.Realize(binding);
      std::cout<<"preparing facet="<<f.id<<std::endl;
      const auto on=geometry.ClosestCurved(f.id,point,1e-8,16384,size_t(16)<<20);
      for(double offset:{0.,-1e-6,1e-6,-1e-3,1e-3}) {
        const size_t queryIndex=index++;if(queryIndex<first)continue;
        std::cout<<"query="<<queryIndex<<" facet="<<f.id<<" offset="<<offset<<std::endl;
        const SU2Native3D::Point query{point.x+offset*on.normal.x,point.y+offset*on.normal.y,point.z+offset*on.normal.z};
        const auto start=std::chrono::steady_clock::now();
        const double precision=std::abs(offset)==1e-6?1e-10:1e-8;
        const auto result=geometry.ClosestCurved(f.id,query,precision,16384,size_t(16)<<20);
        const auto duration=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        if(result.upper-result.lower>precision || result.lower>std::abs(offset)+geometry.MaximumError(binding) ||
            result.clearance_lower < -result.upper || result.clearance_lower > result.upper ||
            !geometry.Admissible(result.binding,result.point) || state.original->Get(result.facet).marker!=f.marker)
          throw std::runtime_error("Curved query violated its distance/body/mapping contract.");
        out<<"{\"query\":"<<queryIndex<<",\"facet\":"<<f.id<<",\"marker\":"<<f.marker<<",\"offset\":"<<offset<<",\"lower\":"<<result.lower
          <<",\"upper\":"<<result.upper<<",\"work\":"<<result.work<<",\"seconds\":"<<duration
          <<",\"precision\":"<<precision<<",\"clearance_lower\":"<<result.clearance_lower
          <<",\"curvature1\":"<<result.curvature1<<",\"curvature2\":"<<result.curvature2<<"}\n";out.flush();
        if(!out)throw std::runtime_error("Cannot preserve independent query receipt.");
        if(offset==-1e-6 && !(result.clearance_lower>0))throw std::runtime_error("Curved fluid-side offset has no positive clearance certificate.");
        ++completed;work+=result.work;seconds+=duration;
      }
    }
    std::cout<<"planned="<<planned<<" first="<<first<<" completed="<<completed<<" work="<<work<<" seconds="<<seconds<<" resident="<<geometry.ResidentBytes()<<" mesh_constructed=false\n";
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';status=1;}
  SU2_MPI::Finalize();return status;
}
