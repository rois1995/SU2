// Capture bounded exact private contexts through the existing observer; never changes native policy.
#include "../../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include <fstream>
#include <iomanip>
#include <deque>
using namespace SU2Native3D;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
struct Rejected {Patch old,fresh;Feature feature;MetricDiagnosis metric;bool geometric;std::string reason;uint64_t sequence;};
void savePatch(const Patch& p,const std::string& path){
  std::ofstream o(path);o<<std::setprecision(17)<<"NATIVE_PRIVATE_PATCH_V1 "<<p.cells.size()<<' '<<p.surface.size()<<'\n';
  for(const auto& c:p.cells){o<<c.id<<'\n';for(const auto n:c.v)o<<n.id<<' '<<n.p.x<<' '<<n.p.y<<' '<<n.p.z<<'\n';}
  for(auto f:p.surface)o<<f.v[0]<<' '<<f.v[1]<<' '<<f.v[2]<<' '<<f.facet<<'\n';require(bool(o),"Cannot retain exact rejected context");
}
class Driver:public CSinglezoneDriver{
 public:using CSinglezoneDriver::CSinglezoneDriver;
 CConfig& Config(){return *config_container[ZONE_0];}
 CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
 std::deque<Rejected> ring;uint64_t observations=0;
 std::vector<double> Snapshot(){
   std::vector<double> values;const auto& g=Geometry();const auto* flow=solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL]->GetNodes();
   for(unsigned long p=0;p<g.GetnPoint();++p){values.push_back(g.nodes->GetGlobalIndex(p));for(unsigned d=0;d<3;++d)values.push_back(SU2_TYPE::GetValue(g.nodes->GetCoord(p,d)));for(unsigned d=0;d<5;++d)values.push_back(SU2_TYPE::GetValue(flow->GetSolution(p,d)));}
   for(unsigned long e=0;e<g.GetnElem();++e)for(unsigned d=0;d<4;++d)values.push_back(g.elem[e]->GetNode(d));return values;
 }
 std::unique_ptr<CNativeRemesher3D> Backend(){
   auto unused=MakeRemesher();require(bool(nativeReference3D),"Missing driver geometry authority");
   auto factory=[this](const CConfig& cfg,const ReferenceState& state,double core,EngineOptions& control)->MetricComposition{
     auto layers=CBoundaryLayerMetric::FromNativeReference(state,cfg);
     layers->ConfigureOuterNormal(SU2_TYPE::GetValue(cfg.GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(cfg.GetAdap_BL_OuterGrowthMax()));layers->SelectOuterNormal(.875);layers->FreezeOuterNormal();
     require(core==.003215747252591252,"Frozen core changed");
     for(unsigned short b=0;b<cfg.GetnAdap_BL();++b){const auto& layer=cfg.GetAdap_BL(b);const auto marker=std::find(state.marker_names.begin(),state.marker_names.end(),layer.marker);require(marker!=state.marker_names.end(),"Missing BL marker");const int id=int(marker-state.marker_names.begin());control.wall_heights[id]=SU2_TYPE::GetValue(layer.firstHeight);control.region_walls[id]={SU2_TYPE::GetValue(layer.firstHeight),SU2_TYPE::GetValue(layer.growth),SU2_TYPE::GetValue(layer.thickness)};}
     control.caller_bytes=transfer_memory::Add(control.caller_bytes,layers->StorageBytes(),size_t(4)<<20); // Explicit bounded diagnostic storage.
     control.edge_breakpoints=[layers](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
     control.rejected_trial=[this](const Patch& old,const Patch& fresh,Feature f,const MetricDiagnosis& m,bool geometry,const std::string& why){
       require(old.cells.size()<=MaximumReplacementCells&&fresh.cells.size()<=MaximumReplacementCells,"Diagnostic patch bound");
       ++observations;if(ring.size()==16)ring.pop_front();ring.push_back({old,fresh,f,m,geometry,why,observations});
     };
     std::ofstream o("profile_rank"+std::to_string(CPassiveComm::Rank())+".json");o<<std::setprecision(17)<<"{\"parameter\":0.875,\"archive_version\":22,\"runtime_version\":"<<layers->GetOuterNormal().version<<",\"core\":"<<core<<"}\n";require(bool(o),"Cannot retain profile");
     return [layers,core](Point p,Tensor s){CBoundaryLayerMetric::Tensor m;m.m[0][0]=s.xx;m.m[1][1]=s.yy;m.m[2][2]=s.zz;m.m[0][1]=m.m[1][0]=s.xy;m.m[0][2]=m.m[2][0]=s.xz;m.m[1][2]=m.m[2][1]=s.yz;const su2double x[]={p.x,p.y,p.z};layers->ApplyPoint(x,m,core,nullptr,false);return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};};
   };
   return std::make_unique<CNativeRemesher3D>(nativeReference3D,EngineOptions{},std::move(factory));
 }
};
int main(int argc,char** argv){
 SU2_MPI::Init(&argc,&argv);int code=0;
 try{require(argc==3,"Expected config and selected sensor file");Driver driver(argv[1],1,SU2_MPI::GetComm());World world;auto& g=driver.Geometry();require(g.GetnDim()==3,"Expected tetrahedral 3D input");
   std::ifstream f(argv[2],std::ios::binary);uint64_t count;f.read(reinterpret_cast<char*>(&count),8);require(count==61330,"Changed frozen sensor count");
   struct Record{std::array<double,3>x;std::array<double,9>m;};std::map<Id,Record> records;
   for(uint64_t k=0;k<count;++k){Id id;Record r;f.read(reinterpret_cast<char*>(&id),8);f.read(reinterpret_cast<char*>(r.x.data()),24);f.read(reinterpret_cast<char*>(r.m.data()),72);require(bool(f)&&records.emplace(id,r).second,"Invalid original field");}require(f.peek()==std::char_traits<char>::eof(),"Extra sensor bytes");
   su2activematrix sensor(g.GetnPointDomain(),6);for(unsigned long p=0;p<sensor.rows();++p){const auto& r=records.at(g.nodes->GetGlobalIndex(p));for(unsigned d=0;d<3;++d)require(SU2_TYPE::GetValue(g.nodes->GetCoord(p,d))==r.x[d],"Moved original donor");const unsigned upper[]={0,1,2,4,5,8};for(unsigned d=0;d<6;++d)sensor(p,d)=r.m[upper[d]];}require(world.sum(uint64_t(sensor.rows()))==count,"Incomplete owned sensor input");
   const auto before=driver.Snapshot();auto backend=driver.Backend();const auto start=SU2_MPI::Wtime();auto result=backend->Remesh(driver.Config(),g,sensor);const auto& s=backend->Statistics();require(before==driver.Snapshot(),"Backend mutated input CFD geometry/flow");
   const auto elapsed=CPassiveComm::Allreduce(SU2_MPI::Wtime()-start,CPassiveComm::Op::MAX);
   const std::string rank=std::to_string(world.rank);std::ofstream out("contexts_rank"+rank+".jsonl");out<<std::setprecision(17);
   for(const auto& r:driver.ring){const auto stem="rank"+rank+"_trial"+std::to_string(r.sequence);savePatch(r.old,stem+"_old.dat");savePatch(r.fresh,stem+"_private.dat");
     out<<"{\"stem\":"<<std::quoted(stem)<<",\"sequence\":"<<r.sequence<<",\"feature_count\":"<<r.feature.count<<",\"feature\":["<<r.feature.vertices[0]<<','<<r.feature.vertices[1]<<','<<r.feature.vertices[2]<<"],\"candidate_geometry\":"<<(r.geometric?"true":"false")<<",\"metric_failure\":"<<int(r.metric.failure)<<",\"q\":"<<r.metric.minimum_mean_ratio<<",\"J\":"<<r.metric.minimum_scaled_jacobian<<",\"reason\":"<<std::quoted(r.reason)<<"}\n";}
   require(bool(out),"Cannot save context index");std::ofstream receipt("result_rank"+rank+".json");receipt<<std::setprecision(17)<<"{\"accepted\":"<<(s.complete?"true":"false")<<",\"seconds\":"<<elapsed<<",\"observations\":"<<driver.observations<<",\"retained\":"<<driver.ring.size()<<",\"atomic_input\":true,\"CFD_steps\":0,\"initial_cells\":"<<s.initial_cells<<",\"final_cells\":"<<s.final_cells<<",\"last_feature_count\":"<<s.refine.last_rejected_feature.count<<",\"last_feature\":["<<s.refine.last_rejected_feature.vertices[0]<<','<<s.refine.last_rejected_feature.vertices[1]<<','<<s.refine.last_rejected_feature.vertices[2]<<"]}\n";require(bool(receipt),"Cannot retain result");driver.Finalize();
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';code=1;}SU2_MPI::Finalize();return code;
}
