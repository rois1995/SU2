// Exercise the existing internal source-shell admission without enabling curved import.
#include "Common/src/adaptation/CNativeImport3D.cpp"
#include "Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include <iomanip>
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int status=1;
  try {
    using namespace SU2Native3D;
    World world;if(world.size!=1 || argc!=4)throw std::runtime_error("usage: probe sidecar donors output.json (serial)");
    const size_t ceiling=size_t(12)<<30;
    std::ifstream file(argv[1]);std::ostringstream text;text<<file.rdbuf();if(!file)throw std::runtime_error("Cannot read curved sidecar.");
    auto state=DecodeReference(text.str(),size_t(2)<<30);
    std::ifstream in(argv[2],std::ios::binary);uint64_t count=0;in.read(reinterpret_cast<char*>(&count),8);
    if(!in || !count || count>1000000)throw std::runtime_error("Invalid or oversized diagnostic donor count.");
    std::vector<DonorCell> source(count);Owned owned;
    std::map<Face,std::pair<Id,unsigned>> boundary;
    for(const auto& f:state.facets) {
      Face key{{f.v[0].id,f.v[1].id,f.v[2].id}};std::sort(key.begin(),key.end());
      if(!boundary.emplace(key,std::make_pair(f.id,0)).second)throw std::runtime_error("Duplicate source boundary facet.");
    }
    for(size_t r=0;r<count;++r) {
      auto& d=source[r];d.cell.id=r;
      for(unsigned k=0;k<4;++k) {
        uint64_t id;double x[3],m[6];in.read(reinterpret_cast<char*>(&id),8);in.read(reinterpret_cast<char*>(x),24);in.read(reinterpret_cast<char*>(m),48);
        if(!in)throw std::runtime_error("Incomplete donor record.");
        d.cell.v[k]={id,{x[0],x[1],x[2]}};d.sensor[k]={m[0],m[1],m[2],m[3],m[4],m[5]};
      }
      auto& v=d.cell.v;const auto sign=Orientation(v[0].p,v[1].p,v[2].p,v[3].p);
      if(!sign)throw std::runtime_error("Degenerate original donor.");
      if(sign<0){std::swap(v[0],v[1]);std::swap(d.sensor[0],d.sensor[1]);}
      OwnedCell row;row.cell=d.cell;
      for(unsigned slot=0;slot<4;++slot) {
        Face key;unsigned at=0;for(unsigned k=0;k<4;++k)if(k!=slot)key[at++]=v[k].id;std::sort(key.begin(),key.end());
        const auto found=boundary.find(key);if(found==boundary.end())continue;
        ++found->second.second;const auto& f=state.original->Get(found->second.first);
        if(!(Orientation(f.v[0].p,f.v[1].p,f.v[2].p,v[slot].p)<0))throw std::runtime_error("Source/reference orientation mismatch.");
        for(const auto& n:f.v)for(const auto& node:v)if(n.id==node.id && !(n.p.x==node.p.x&&n.p.y==node.p.y&&n.p.z==node.p.z))
          throw std::runtime_error("Source/reference coordinates differ.");
        row.physical[slot]={true,f.id};
      }
      owned.emplace(r,row);
    }
    if(in.peek()!=EOF)throw std::runtime_error("Unexpected donor trailing data.");
    for(const auto& row:boundary)if(row.second.second!=1)throw std::runtime_error("Incomplete source boundary incidence.");
    std::map<Face,std::pair<Id,unsigned>>().swap(boundary);
    double radius=0;for(const auto& row:state.original->Parameters()->Geometry().Errors())
      radius=std::max(radius,4*state.original->Parameters()->Geometry().EnvelopeError(row.first));
    BridgeStats stats;std::string reason;
    const auto live=transfer_memory::Add(OwnedStorageBytes(owned),ReferenceStorageBytes(state),transfer_memory::Bytes(source));
    const bool accepted=Coverage(world,owned,source,*state.original,1e-4,live,ceiling,stats,reason,180,true);
    std::ofstream out(argv[3]);out<<std::setprecision(17)<<"{\"admitted\":"<<(accepted?"true":"false")
      <<",\"mesh_constructed\":false,\"radius\":"<<radius<<",\"source_cells_before\":"<<count
      <<",\"source_cells_after\":"<<source.size()<<",\"shell_nodes\":"<<stats.source_envelope_nodes
      <<",\"shell_cells\":"<<stats.source_envelope_cells<<",\"solves\":"<<stats.source_envelope_solves
      <<",\"contacts\":"<<stats.source_envelope_contacts<<",\"distances\":"<<stats.source_envelope_distances
      <<",\"seconds\":"<<stats.source_envelope_seconds<<",\"maximum_requested_bytes\":"<<stats.maximum_native_bytes
      <<",\"reason\":"<<std::quoted(reason)<<"}\n";
    if(!out)throw std::runtime_error("Cannot preserve source admission receipt.");
    std::cout<<"source_admitted="<<accepted<<" reason="<<reason<<std::endl;status=accepted?0:2;
  }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;}
  SU2_MPI::Finalize();return status;
}
