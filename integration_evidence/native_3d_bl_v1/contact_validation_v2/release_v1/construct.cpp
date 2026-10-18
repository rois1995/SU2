// Complete closed curved BL construction probe. No CFD advance or target fitting.
#include "../../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include <fstream>
#include <iomanip>
using namespace SU2Native3D;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
class Driver:public CSinglezoneDriver {
 public:using CSinglezoneDriver::CSinglezoneDriver;
 CConfig& Config(){return *config_container[ZONE_0];}
 CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
 std::shared_ptr<ReferenceState> Reference(){auto unused=MakeRemesher();require(bool(nativeReference3D),"Missing declared geometry");return nativeReference3D;}
};
void savePatch(const Patch& patch,const std::string& name) {
 std::ofstream out(name+".dat");out<<std::setprecision(17)<<"NATIVE_PRIVATE_PATCH_V1 "<<patch.cells.size()<<' '<<patch.surface.size()<<'\n';
 for(const auto& c:patch.cells){out<<c.id<<'\n';for(auto n:c.v)out<<n.id<<' '<<n.p.x<<' '<<n.p.y<<' '<<n.p.z<<'\n';}
 for(auto f:patch.surface)out<<f.v[0]<<' '<<f.v[1]<<' '<<f.v[2]<<' '<<f.facet<<'\n';out.close();require(bool(out),"Cannot save exact private patch");
}
Patch patchOf(const Owned& cells) {
 Patch p;for(const auto& entry:cells){const auto& c=entry.second;p.cells.push_back(c.cell);const auto& v=c.cell.v;const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};for(unsigned k=0;k<4;++k)if(c.physical[k].present)p.surface.push_back({faces[k],c.physical[k].facet});}return p;
}
void mesh(const Patch& patch,const std::string& name,const ReferenceState& reference) {
 std::map<Id,Node> nodes;for(const auto& c:patch.cells)for(auto n:c.v){auto it=nodes.emplace(n.id,n);require(it.second||(it.first->second.p.x==n.p.x&&it.first->second.p.y==n.p.y&&it.first->second.p.z==n.p.z),"Conflicting output node");}
 std::map<Id,size_t> index;for(auto n:nodes)index[n.first]=index.size();
 std::ofstream out(name+".su2");out<<std::setprecision(17)<<"NDIME= 3\nNELEM= "<<patch.cells.size()<<'\n';for(const auto& c:patch.cells){out<<10;for(auto n:c.v)out<<' '<<index.at(n.id);out<<'\n';}out<<"NPOIN= "<<nodes.size()<<'\n';for(auto n:nodes)out<<n.second.p.x<<' '<<n.second.p.y<<' '<<n.second.p.z<<' '<<index.at(n.first)<<'\n';out<<"NMARK= 1\nMARKER_TAG= sphere\nMARKER_ELEMS= "<<patch.surface.size()<<'\n';for(auto f:patch.surface){out<<5;for(auto n:f.v)out<<' '<<index.at(n);out<<'\n';}out.close();require(bool(out),"Cannot save inspectable grid");WriteReference(name+".su2.native_ref",reference);
}
int run(char* cfg) {
 World world;require(world.size==1,"Serial construction diagnosis first");Driver driver(cfg,1,world.comm);auto reference=std::make_shared<ReferenceState>();require(LoadReference("sphere.su2.native_ref",*reference),"Missing explicit saved sphere reference");auto& g=driver.Geometry();require(reference->marker_names.size()==1&&reference->marker_names[0]=="sphere","Wrong declared sphere reference");
 su2activematrix sensor(g.GetnPointDomain(),6);for(unsigned long p=0;p<sensor.rows();++p)for(unsigned k=0;k<6;++k)sensor(p,k)=k==0||k==3||k==5?1.:0.;
 auto input=ImportGeometry(driver.Config(),g,&sensor,*reference);require(input.cells.size()==8,"Wrong original sphere mesh");const auto authority=EncodeReference(*reference);
 for(const auto& d:input.original)for(const auto& m:d.sensor)require(m.xx==1&&m.xy==0&&m.xz==0&&m.yy==1&&m.yz==0&&m.zz==1,"Nonconstant original sensor");
 MetricComposition composition;
 EngineOptions options;options.surface_tolerance=.04;options.maximum_work=driver.Config().GetAdap_Native_Max_Work();options.maximum_seconds=SU2_TYPE::GetValue(driver.Config().GetAdap_Native_Max_Time());options.timings=true;
 options.rejected_trial=[&](const Patch& old,const Patch& fresh,Feature,const MetricDiagnosis&,bool,const std::string& why){savePatch(old,"last_old");savePatch(fresh,"last_private");std::ofstream out("last_rejection.txt");out<<why<<'\n';};
 options.rejected_state=[&](const Owned& cells,const std::string& why){savePatch(patchOf(cells),"terminal_rejected");std::ofstream out("terminal_rejection.txt");out<<why<<'\n';};
 std::vector<char> snapshot;RecordStream snapshot_stream(snapshot);for(auto row:input.cells)snapshot_stream(row.second);Owned working;std::ifstream saved("accepted_input.dat");std::string magic;size_t nc,nf;saved>>magic>>nc>>nf;require(magic=="NATIVE_PRIVATE_PATCH_V1"&&nc==5440,"Wrong saved accepted input");
 for(size_t i=0;i<nc;++i){OwnedCell row;row.version=1;saved>>row.cell.id;for(auto& n:row.cell.v)saved>>n.id>>n.p.x>>n.p.y>>n.p.z;require(bool(saved)&&working.emplace(row.cell.id,row).second,"Truncated/duplicate accepted cell");}
 std::map<Face,Id> physical;for(size_t i=0;i<nf;++i){Face f;Id facet;saved>>f[0]>>f[1]>>f[2]>>facet;std::sort(f.begin(),f.end());require(bool(saved)&&physical.emplace(f,facet).second,"Invalid accepted physical face");}
 for(auto& entry:working){auto& row=entry.second;const auto& v=row.cell.v;const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};for(unsigned i=0;i<4;++i){auto f=faces[i];std::sort(f.begin(),f.end());const auto found=physical.find(f);if(found!=physical.end())row.physical[i]={true,found->second};}}
 std::vector<char> working_snapshot;RecordStream work_stream(working_snapshot);for(auto row:working)work_stream(row.second);
 const auto before=patchOf(working);savePatch(before,"original");
 const auto start=world.seconds();auto result=Coarsen(world,working,input.original,*reference->original,composition,options);const auto elapsed=world.seconds()-start;
 savePatch(patchOf(result.cells),result.valid?"accepted":"retained");mesh(patchOf(result.cells),result.valid?"accepted":"rejected",*reference);require(EncodeReference(*reference)==authority,"Changed geometry authority");
 std::vector<char> current;RecordStream current_stream(current);for(auto row:input.cells)current_stream(row.second);require(current==snapshot,"Changed original CFD import");std::vector<char> working_current;RecordStream current_work(working_current);for(auto row:working)current_work(row.second);require(working_current==working_snapshot,"Changed saved accepted input");
 std::ofstream original("source.json");original<<std::setprecision(17)<<'[';bool comma=false;
 for(const auto& d:input.original){if(comma)original<<',';comma=true;original<<"{\"id\":"<<d.cell.id<<",\"component\":"<<d.extension_component<<",\"parent\":"<<d.parent_cell<<",\"vertices\":[";for(unsigned k=0;k<4;++k){if(k)original<<',';const auto n=d.cell.v[k];const auto m=d.sensor[k];original<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<",["<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<"]]";}original<<"]}";}original<<"]\n";original.close();require(bool(original),"Cannot save immutable original source");
 if(result.valid) {
  FrozenField field(input.original,composition,false,AssociatedExtensions(input.original,*reference->original),options.edge_breakpoints);
  std::ofstream samples("samples.csv"),edgeFile("edges.csv");samples<<std::setprecision(17)<<"cell,q,j,xx,xy,xz,yy,yz,zz\n";edgeFile<<std::setprecision(20)<<"a,b,length\n";std::map<Edge,std::pair<Point,Point>> edges;
  for(const auto& row:result.cells){const auto& c=row.second.cell;const auto m=field.Query(Centroid(c)).target;const auto q=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},m);samples<<c.id<<','<<q.mean_ratio<<','<<q.minimum_scaled_jacobian<<','<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<'\n';
   for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){auto a=c.v[i],b=c.v[j];if(a.id>b.id)std::swap(a,b);edges.emplace(Edge{a.id,b.id},std::make_pair(a.p,b.p));}}
  for(const auto& edge:edges)edgeFile<<edge.first[0]<<','<<edge.first[1]<<','<<field.TargetEdgeLength(edge.second.first,edge.second.second)<<'\n';samples.close();edgeFile.close();require(bool(samples)&&bool(edgeFile),"Cannot save complete accepted field audit");
 }
 std::ofstream out("result.json");out<<std::setprecision(17)<<"{\"valid\":"<<(result.valid?"true":"false")<<",\"seconds\":"<<elapsed<<",\"cells\":"<<result.cells.size()<<",\"source_donors\":"<<input.original.size()<<",\"work\":"<<result.stats.global_work<<",\"stop\":"<<std::quoted(EngineStopName(result.stats.stop))<<",\"region_attempts\":"<<result.stats.region_attempts<<",\"region_cells\":"<<result.stats.region_cells<<",\"region_fronts\":"<<result.stats.region_transition_fronts<<",\"surface_trials\":"<<result.stats.region_surface_trials<<",\"junction_trials\":"<<result.stats.region_junction_trials<<",\"core_trials\":"<<result.stats.region_core_trials<<",\"front_trials\":"<<result.stats.region_front_trials<<",\"sampling_seconds\":"<<result.stats.region_sampling_seconds<<",\"first_band_seconds\":"<<result.stats.region_first_band_seconds<<",\"core_seconds\":"<<result.stats.region_core_seconds<<",\"validation_seconds\":"<<result.stats.region_validation_seconds<<",\"transition_seconds\":"<<result.stats.region_transition_seconds<<",\"reconstruction_seconds\":"<<result.stats.reconstruction_seconds<<",\"work_limit\":"<<result.stats.work_limit<<",\"region_field_points\":"<<result.stats.region_field_points<<",\"region_field_edges\":"<<result.stats.region_field_edges<<",\"reason\":"<<std::quoted(result.reason)<<",\"immutable_input\":true,\"unchanged_authority\":true,\"CFD_steps\":0}\n";out.close();require(bool(out),"Cannot save construction receipt");driver.Finalize();return result.valid?0:2;
}
int main(int argc,char** argv){SU2_MPI::Init(&argc,&argv);int code=0;try{require(argc==2,"Expected config");code=run(argv[1]);}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;code=1;}SU2_MPI::Finalize();return code;}
