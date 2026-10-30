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
    struct Query {std::vector<SU2Native3D::Id> facets;std::vector<std::array<SU2Native3D::Point,3>> physical;};
    std::vector<Query> queries;
    queries.push_back({{22287,22969,22993,22994,23137,23138,23270,23316,23346,23360,23361,27093,27094,27095,27120,27121,27122,27145,27146,27147,27148,27149,27150,27151,27177,27178,27181,27182,27183,27184,27208,27212,27213,27214,27215,27240,27243,27244,27268,27271,27272,27292,47230,47231,47612,47613,47614,47615,47638,47707,47708,47713,47714,47727,47728,47738,47739,47794,47799,47807,47808,47810,47826,47827,47843,47845,47847,47848,47999,48001,48003,48005,48006,48007,48009},{{{{0.88756244504698567,1.196299999999894,0.02192292083359024},{0.88947001686594007,1.187025766068968,0.02192107412674615},{0.87603773257407591,1.1888021765888801,0.022165855038422111}}}}});
    queries.push_back({{22287,22993,22994,23137,23138,23270,23346,23360,23361,27068,27093,27094,27095,27096,27120,27121,27122,27123,27146,27147,27148,27149,27150,27151,27178,27181,27182,27183,27184,27212,27213,27214,27215,27243,27244,27272,47230,47231,47612,47614,47615,47637,47638,47713,47714,47727,47728,47731,47738,47739,47799,47802,47807,47808,47818,47826,47827,47843,47845,47847,47848,47999,48001,48003,48005,48006,48007,48009,48011},{{{{0.87137596097371595,1.196299999999894,0.022143445513179091},{0.8831760152991136,1.1897712905021169,0.022043895212589419},{0.86735486194659206,1.1886823523008221,0.022256933009470011}}}}});
    queries.push_back({{22969,22993,22994,23137,23138,23316,23346,23360,23361,27093,27095,27120,27121,27122,27145,27146,27147,27148,27149,27150,27177,27178,27181,27182,27183,27184,27208,27212,27213,27214,27215,27240,27243,27244,27268,27271,27272,27292,47230,47231,47612,47613,47614,47615,47707,47708,47713,47714,47727,47728,47738,47794,47799,47807,47808,47810,47826,47827,47843,47845,47847,47999,48001,48003,48005,48006,48009},{{{{0.87956227535359544,1.196299999999894,0.022056737349563309},{0.88756244504698567,1.196299999999894,0.02192292083359024},{0.88947001686594007,1.187025766068968,0.02192107412674615}}}}});
    unsigned failed=0;for(size_t i=0;i<queries.size();++i){
      SU2NativeGeometry::AtlasBudget budget;budget.maximum_work=1000000000;budget.maximum_bytes=ceiling;budget.maximum_integer_bits=209504;budget.caller_bytes=caller;
      const auto start=std::chrono::steady_clock::now();
      try {auto bound=geometry.BoundPhysicalSurface(queries[i].facets,queries[i].physical,1e-4,budget);
        std::cout<<std::setprecision(17)<<"query="<<i<<" result=bound bound="<<bound;
      } catch(const std::exception& e){++failed;std::cout<<"query="<<i<<" result=exception error="<<e.what();}
      std::cout<<" work="<<budget.work<<" caller_bytes="<<caller<<" peak_bytes="<<budget.peak_bytes<<" seconds="<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<"\n"<<std::flush;
    }
    std::cout<<"completed="<<queries.size()<<" exceptions="<<failed<<"\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<"\n";status=1;}
  SU2_MPI::Finalize();return status;
}
