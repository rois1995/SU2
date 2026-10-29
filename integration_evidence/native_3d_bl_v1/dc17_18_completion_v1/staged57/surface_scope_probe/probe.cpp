#include "../../../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../../../Common/include/adaptation/CTransferMemory.hpp"
#include "../../../../../Common/include/parallelization/mpi_structure.hpp"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int status=0;
  try {
    if(argc!=2)throw std::runtime_error("usage: probe reference.native_ref");
    std::ifstream in(argv[1],std::ios::binary);if(!in)throw std::runtime_error("Cannot read reference");
    std::string text((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());if(in.bad())throw std::runtime_error("Cannot read reference");
    constexpr size_t ceiling=size_t(12)<<30;
    auto state=SU2Native3D::DecodeReference(text,ceiling);
    const auto caller=transfer_memory::Add(SU2Native3D::ReferenceStorageBytes(state),text.capacity());
    if(caller>ceiling)throw std::runtime_error("Reference exceeds storage ceiling");
    if(!state.original || !state.original->Mapped() || !state.original->Parameters())throw std::runtime_error("Expected mapped authority");
    const auto& geometry=state.original->Parameters()->Geometry();
    const std::vector<SU2Native3D::Id> facets={41485,41486,41492,41845,43578,43579,43894,43975,43976,44070,44071,44303,44367,44383,44395,44552,44665,45663,45664,45681,45682,45683,45700,45701,45702,45714,45716,45717,45718,45719,45734,45735,45736,45751,45752,45753,45754,45769};
    std::vector<std::array<SU2Native3D::Point,3>> physical={{{{0.55153320451464394,0.38021241273808187,-0.032716074566382473},{0.56375529165422267,0.36510764045479471,-0.032187514520798287},{0.56530953543327178,0.39977586224200762,-0.03225505132526349}}}};
    for(unsigned pass=0;pass<2;++pass){
      if(pass)physical[0][0].x=std::nextafter(physical[0][0].x,INFINITY);
      SU2NativeGeometry::AtlasBudget budget;budget.maximum_work=1000000000;budget.maximum_bytes=ceiling;budget.maximum_integer_bits=209504;budget.caller_bytes=caller;
      const auto start=std::chrono::steady_clock::now();
      try {const auto bound=geometry.BoundPhysicalSurface(facets,physical,1e-4,budget);
        const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
        std::cout<<std::setprecision(17)<<"pass="<<pass<<" bound="<<bound<<" work="<<budget.work<<" caller_bytes="<<caller<<" peak_bytes="<<budget.peak_bytes<<" seconds="<<seconds<<"\n"<<std::flush;
      } catch(const std::exception& e){std::cerr<<"pass="<<pass<<" work="<<budget.work<<" peak_bytes="<<budget.peak_bytes<<" error="<<e.what()<<"\n";throw;}
    }
  }catch(const std::exception& e){std::cerr<<e.what()<<"\n";status=1;}
  SU2_MPI::Finalize();return status;
}
