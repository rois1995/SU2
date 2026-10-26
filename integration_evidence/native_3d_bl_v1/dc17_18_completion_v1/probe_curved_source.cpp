// Exercise the existing internal source-shell admission without enabling curved import.
#include "Common/src/adaptation/CNativeImport3D.cpp"
#include "Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include <iomanip>
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int status=1;
  try {
    using namespace SU2Native3D;
    World world;if(argc<4 || argc>6 || (argc>=5 && (world.size!=1 || std::string(argv[4])!="--surface")))
      throw std::runtime_error("usage: probe sidecar donors output [--surface [first-facet]] (surface sampling is serial)");
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
    if(argc>=5) {
      const size_t first=argc==6?std::stoull(argv[5]):0;
      if(first>=state.facets.size())throw std::runtime_error("Resume index leaves the planned surface.");
      Owned().swap(owned);FrozenField field(std::move(source),{},false,{}, {}, {},true,count);
      const auto& g=state.original->Parameters()->Geometry();std::ofstream out(argv[3]);out<<std::setprecision(17);
      size_t missing=0,checked=0;const auto start=world.seconds();
      for(size_t index=first;index<state.facets.size();++index) {
        if(world.seconds()-start>180)throw std::runtime_error("Surface diagnostic work time exhausted; completed facets preserved.");
        const auto& f=state.facets[index];std::vector<SU2NativeGeometry::SurfaceBinding> samples;
        for(unsigned k=0;k<3;++k){Edge e{f.v[k].id,f.v[(k+1)%3].id};std::sort(e.begin(),e.end());samples.push_back(g.EdgePoint(e,.5));}
        for(const auto& uv:std::array<std::array<double,2>,4>{{{.25,.25},{.5,.25},{.25,.5},{1./3,1./3}}})samples.push_back(g.FacetPoint(f.id,uv[0],uv[1]));
        out<<"{\"index\":"<<index<<",\"facet\":"<<f.id<<",\"marker\":"<<f.marker<<",\"samples\":7,\"outside\":[";bool separator=false;
        for(size_t k=0;k<samples.size();++k) {
          const auto p=g.Realize(samples[k]);++checked;
          try {const auto value=field.Query(p);if(value.extension_component!=UINT64_MAX)throw std::runtime_error("Unexpected generated source.");}
          catch(const std::runtime_error& e) {
            if(std::string(e.what()).find("Query outside native 3D immutable original donors:")!=0)throw;
            ++missing;if(separator)out<<',';separator=true;
            out<<"{\"sample\":"<<k<<",\"point\":["<<p.x<<','<<p.y<<','<<p.z<<"]}";
          }
        }
        out<<"]}\n";out.flush();if(!out)throw std::runtime_error("Cannot preserve surface coverage receipt.");
        if(index%1024==0)std::cout<<"completed_facet="<<index<<" checked="<<checked<<" outside="<<missing<<std::endl;
      }
      std::cout<<"surface_samples="<<checked<<" outside="<<missing<<" first="<<first<<" mesh_constructed=false"<<std::endl;
      SU2_MPI::Finalize();return 0;
    }
    double radius=0;for(const auto& row:state.original->Parameters()->Geometry().Errors())
      radius=std::max(radius,4*state.original->Parameters()->Geometry().EnvelopeError(row.first));
    source.erase(std::remove_if(source.begin(),source.end(),[&](const DonorCell& d){return d.cell.id%world.size!=Id(world.rank);}),source.end());
    for(auto i=owned.begin();i!=owned.end();)if(i->first%world.size!=Id(world.rank))i=owned.erase(i);else ++i;
    const size_t local_count=source.size();
    const std::string output=std::string(argv[3])+(world.size==1?"":".rank"+std::to_string(world.rank));
    BridgeStats stats;std::string reason;
    const auto live=transfer_memory::Add(OwnedStorageBytes(owned),ReferenceStorageBytes(state),transfer_memory::Bytes(source));
    const bool accepted=Coverage(world,owned,source,*state.original,1e-4,live,ceiling,stats,reason,180,true);
    std::ofstream out(output);out<<std::setprecision(17)<<"{\"admitted\":"<<(accepted?"true":"false")
      <<",\"mesh_constructed\":false,\"radius\":"<<radius<<",\"rank\":"<<world.rank<<",\"ranks\":"<<world.size
      <<",\"global_original_cells\":"<<count<<",\"source_cells_before\":"<<local_count
      <<",\"source_cells_after\":"<<source.size()<<",\"shell_nodes\":"<<stats.source_envelope_nodes
      <<",\"shell_cells\":"<<stats.source_envelope_cells<<",\"solves\":"<<stats.source_envelope_solves
      <<",\"contacts\":"<<stats.source_envelope_contacts<<",\"distances\":"<<stats.source_envelope_distances
      <<",\"seconds\":"<<stats.source_envelope_seconds<<",\"maximum_requested_bytes\":"<<stats.maximum_native_bytes
      <<",\"reason\":"<<std::quoted(reason)<<"}\n";
    if(!out)throw std::runtime_error("Cannot preserve source admission receipt.");
    out.close();
    if(accepted) {
      uint64_t version=1,records=source.size();RecordStream measure;measure(version,records);
      for(auto& row:source)measure(row);
      if(measure.Size()>(size_t(1)<<30))throw std::runtime_error("Diagnostic source snapshot exceeds 1 GiB.");
      std::vector<char> bytes;bytes.reserve(measure.Size());RecordStream encode(bytes);encode(version,records);
      for(auto& row:source)encode(row);
      if(bytes.size()!=measure.Size())throw std::runtime_error("Source snapshot measurement mismatch.");
      const auto& input=bytes;RecordStream decode(input);uint64_t read_version=0,read_count=0;decode(read_version,read_count);
      if(read_version!=version || read_count!=records)throw std::runtime_error("Source snapshot header mismatch.");
      for(size_t k=0;k<source.size();++k){DonorCell row;decode(row);std::vector<char> a,b;RecordStream ea(a),eb(b);ea(row);eb(source[k]);
        if(a!=b)throw std::runtime_error("Source snapshot round-trip mismatch.");}
      if(!decode.End())throw std::runtime_error("Source snapshot trailing data.");
      std::ofstream saved(output+".donors",std::ios::binary);saved.write(bytes.data(),bytes.size());saved.close();
      if(!saved)throw std::runtime_error("Cannot preserve admitted source snapshot.");
    }
    std::cout<<"source_admitted="<<accepted<<" reason="<<reason<<std::endl;status=accepted?0:2;
  }catch(const std::exception& e){std::cerr<<e.what()<<std::endl;}
  SU2_MPI::Finalize();return status;
}
