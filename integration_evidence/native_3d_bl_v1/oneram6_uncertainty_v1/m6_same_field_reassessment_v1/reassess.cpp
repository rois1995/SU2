// Same immutable selected original field; rejected finer geometry is only an integration proposal.
#include "../../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include <fstream>
#include <iomanip>
#include <cstring>
using namespace SU2Native3D;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
class Driver:public CSinglezoneDriver {
 public:using CSinglezoneDriver::CSinglezoneDriver;
 CConfig& Config(){return *config_container[ZONE_0];}
 CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
 std::shared_ptr<ReferenceState> Reference(){auto unused=MakeRemesher();require(bool(nativeReference3D),"Missing original geometry authority");return nativeReference3D;}
};
uint64_t Fingerprint(const std::vector<DonorCell>& source) {
 uint64_t h=1469598103934665603ull;
 auto add=[&](const auto& v){const auto* b=reinterpret_cast<const unsigned char*>(&v);for(size_t i=0;i<sizeof(v);++i)h=(h^b[i])*1099511628211ull;};
 for(const auto& d:source){add(d.cell.id);for(unsigned k=0;k<4;++k){const auto& n=d.cell.v[k];add(n.id);add(n.p.x);add(n.p.y);add(n.p.z);const auto& m=d.sensor[k];for(double x:{m.xx,m.xy,m.xz,m.yy,m.yz,m.zz})add(x);}}return h;
}
Owned Working(World& world) {
 Owned cells;
 for(unsigned piece=0;piece<4;++piece)if(piece%unsigned(world.size)==unsigned(world.rank)) {
  std::ifstream f("working_owner"+std::to_string(piece)+".bin",std::ios::binary);char magic[8];f.read(magic,8);
  require(bool(f)&&std::memcmp(magic,"NATOWN1\0",8)==0,"Bad terminal owner header");uint64_t count;
  auto get=[&](auto& x){f.read(reinterpret_cast<char*>(&x),sizeof(x));};get(count);require(count<=2000000,"Excess terminal owner count");
  for(uint64_t i=0;i<count;++i){OwnedCell row;get(row.cell.id);get(row.version);for(auto& n:row.cell.v){get(n.id);get(n.p.x);get(n.p.y);get(n.p.z);}for(auto& b:row.physical){uint8_t present;get(present);require(present<=1,"Bad physical binding");b.present=present;get(b.facet);}require(bool(f)&&cells.emplace(row.cell.id,row).second,"Truncated/duplicate terminal cell");}
  require(f.peek()==std::char_traits<char>::eof(),"Extra terminal bytes");
 }
 require(world.sum(uint64_t(cells.size()))==541027,"Incomplete terminal working mesh");return cells;
}
int run(char* cfg) {
 World world;Driver driver(cfg,1,world.comm);auto& g=driver.Geometry();auto reference=driver.Reference();
 std::ifstream f("selected_sensor.bin",std::ios::binary);uint64_t count;f.read(reinterpret_cast<char*>(&count),8);require(count==61330,"Changed original selected field");
 struct Record{std::array<double,3>x;std::array<double,9>m;};std::map<Id,Record> records;
 for(uint64_t k=0;k<count;++k){Id id;Record r;f.read(reinterpret_cast<char*>(&id),8);f.read(reinterpret_cast<char*>(r.x.data()),24);f.read(reinterpret_cast<char*>(r.m.data()),72);require(bool(f)&&id<count&&records.emplace(id,r).second,"Invalid selected field");}
 require(f.peek()==std::char_traits<char>::eof(),"Extra selected-field bytes");
 su2activematrix sensor(g.GetnPointDomain(),6);const unsigned upper[]={0,1,2,4,5,8};
 for(unsigned long p=0;p<sensor.rows();++p){const auto& r=records.at(g.nodes->GetGlobalIndex(p));for(unsigned d=0;d<3;++d)require(SU2_TYPE::GetValue(g.nodes->GetCoord(p,d))==r.x[d],"Moved original sensor node");for(unsigned d=0;d<6;++d)sensor(p,d)=r.m[upper[d]];}
 require(world.sum(uint64_t(sensor.rows()))==count,"Incomplete original selected publisher field");
 const auto start=world.seconds();auto input=ImportGeometry(driver.Config(),g,&sensor,*reference);const auto source_before=Fingerprint(input.original);
 auto layers=CBoundaryLayerMetric::FromNativeReference(*reference,driver.Config());layers->ConfigureOuterNormal(SU2_TYPE::GetValue(driver.Config().GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(driver.Config().GetAdap_BL_OuterGrowthMax()));layers->SelectOuterNormal(.875);layers->FreezeOuterNormal();
 const auto version=layers->GetOuterNormal().version;const double core=.003215747252591252;
 MetricComposition composition=[&](Point p,Tensor s){CBoundaryLayerMetric::Tensor m;m.m[0][0]=s.xx;m.m[0][1]=m.m[1][0]=s.xy;m.m[0][2]=m.m[2][0]=s.xz;m.m[1][1]=s.yy;m.m[1][2]=m.m[2][1]=s.yz;m.m[2][2]=s.zz;const su2double x[]={p.x,p.y,p.z};layers->ApplyPoint(x,m,core,nullptr,false);return Tensor{m.m[0][0],m.m[0][1],m.m[0][2],m.m[1][1],m.m[1][2],m.m[2][2]};};
 const size_t ceiling=size_t(2)<<30;const auto caller=transfer_memory::Add(OwnedStorageBytes(input.cells),transfer_memory::Bytes(input.original),ReferenceStorageBytes(*reference),layers->StorageBytes(),size_t(32)<<20);
 std::string reason;DonorImport source(world,true);const auto before_build=world.seconds();const bool built=source.Build(input.original,reason,ceiling,caller);if(!built)throw std::runtime_error(reason);const auto source_seconds=world.seconds()-before_build;
 auto report=[&](const std::string& name,const Owned& cells){
  const auto before=world.seconds();BridgeStats stats;ComplexityReassessment3D result;const auto original_queries=source.Statistics().point_queries;
  const bool ok=ReassessComplexity(world,cells,source,*reference->original,composition,result,reason,stats,ceiling,caller,8192,20261017,[&](){return world.seconds()-before<90;});
  require(Fingerprint(input.original)==source_before,"Reassessment changed original donors");require(layers->GetOuterNormal().version==version,"Reassessment changed selected profile");
  std::ofstream out(name+"_rank"+std::to_string(world.rank)+".json");out<<std::setprecision(17)<<"{\"rank\":"<<world.rank<<",\"ranks\":"<<world.size<<",\"ok\":"<<(ok?"true":"false")<<",\"estimated\":"<<(result.estimated?"true":"false")<<",\"value\":"<<result.value<<",\"uncertainty\":";if(std::isfinite(result.uncertainty))out<<result.uncertainty;else out<<"null";
  out<<",\"point_SE\":"<<result.standard_error<<",\"block_SE\":"<<result.block_standard_error<<",\"centroid\":"<<result.centroid_estimate<<",\"samples\":"<<result.samples<<",\"seconds\":"<<world.seconds()-before<<",\"query_seconds\":"<<stats.query_seconds<<",\"source_point_seconds\":"<<stats.source_point_seconds<<",\"field_build_seconds\":"<<stats.field_build_seconds<<",\"composition_seconds\":"<<stats.composition_seconds<<",\"sampler_preparation_seconds\":"<<stats.cells_seconds<<",\"sampler_draw_seconds\":"<<stats.points_seconds<<",\"reduction_seconds\":"<<stats.reduction_seconds<<",\"tracked_bytes\":"<<stats.tracked_bytes<<",\"tracked_calls\":"<<stats.tracked_calls<<",\"maximum_native_bytes\":"<<stats.maximum_native_bytes<<",\"source_queries\":"<<source.Statistics().point_queries-original_queries<<",\"requests\":"<<stats.requests<<",\"compositions\":"<<stats.compositions<<",\"immutable_source\":true,\"profile_version\":"<<version<<",\"reason\":"<<std::quoted(ok?result.reason:reason)<<"}\n";out.close();require(bool(out),"Cannot save independent assessment stage");
  if(!world.rank)std::cout<<"STAGE "<<name<<" ok="<<ok<<" estimated="<<result.estimated<<" seconds="<<world.seconds()-before<<" reason="<<(ok?result.reason:reason)<<std::endl;
  return ok;
 };
 const bool coarse_ok=report("coarse",input.cells);
 auto fine=Working(world);const bool fine_ok=report("fine",fine);
 std::ofstream out("setup_rank"+std::to_string(world.rank)+".json");out<<std::setprecision(17)<<"{\"source_build_seconds\":"<<source_seconds<<",\"total_seconds\":"<<world.seconds()-start<<",\"source_resident_bytes\":"<<source.ResidentBytes()<<",\"original_cells\":"<<world.sum(uint64_t(input.original.size()))<<",\"fine_cells\":"<<world.sum(uint64_t(fine.size()))<<",\"immutable_source\":true,\"profile_version\":"<<version<<"}\n";out.close();require(bool(out),"Cannot save setup");
 driver.Finalize();return coarse_ok&&fine_ok?0:2;
}
int main(int argc,char** argv){SU2_MPI::Init(&argc,&argv);int code=0;try{require(argc==2,"Expected original config");code=run(argv[1]);}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;code=1;}SU2_MPI::Finalize();return code;}
