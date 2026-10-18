// Complete closed curved BL construction probe. No CFD advance or target fitting.
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
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
 World world;require(world.size==1,"Serial construction diagnosis first");Driver driver(cfg,1,world.comm);auto reference=driver.Reference();auto& g=driver.Geometry();require(reference->marker_names.size()==1&&reference->marker_names[0]=="sphere","Wrong declared sphere reference");
 su2activematrix sensor(g.GetnPointDomain(),6);for(unsigned long p=0;p<sensor.rows();++p)for(unsigned k=0;k<6;++k)sensor(p,k)=k==0||k==3||k==5?1.:0.;
 auto input=ImportGeometry(driver.Config(),g,&sensor,*reference);require(input.cells.size()==8,"Wrong original sphere mesh");const auto authority=EncodeReference(*reference);
 for(const auto& d:input.original)for(const auto& m:d.sensor)require(m.xx==1&&m.xy==0&&m.xz==0&&m.yy==1&&m.yz==0&&m.zz==1,"Nonconstant original sensor");
 auto layers=CBoundaryLayerMetric::FromNativeReference(*reference,driver.Config());
 MetricComposition composition=[&](Point p,Tensor s){CBoundaryLayerMetric::Tensor m;m.m[0][0]=s.xx;m.m[0][1]=m.m[1][0]=s.xy;m.m[0][2]=m.m[2][0]=s.xz;m.m[1][1]=s.yy;m.m[1][2]=m.m[2][1]=s.yz;m.m[2][2]=s.zz;const su2double x[]={p.x,p.y,p.z};layers->ApplyPoint(x,m,1.,nullptr,false);return Tensor{m.m[0][0],m.m[0][1],m.m[0][2],m.m[1][1],m.m[1][2],m.m[2][2]};};
 EngineOptions options;options.surface_tolerance=.04;options.maximum_work=4096;options.maximum_seconds=300;options.timings=true;options.wall_heights[0]=.05;options.region_walls[0]={.05,1.2,.2};options.edge_breakpoints=[&](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
 options.rejected_trial=[&](const Patch& old,const Patch& fresh,Feature,const MetricDiagnosis&,bool,const std::string& why){savePatch(old,"last_old");savePatch(fresh,"last_private");std::ofstream out("last_rejection.txt");out<<why<<'\n';};
 options.rejected_state=[&](const Owned& cells,const std::string& why){savePatch(patchOf(cells),"terminal_rejected");std::ofstream out("terminal_rejection.txt");out<<why<<'\n';};
 const auto before=patchOf(input.cells);savePatch(before,"original");const auto start=world.seconds();auto result=Refine(world,input.cells,input.original,*reference->original,composition,options);const auto elapsed=world.seconds()-start;
 savePatch(patchOf(result.cells),result.valid?"accepted":"retained");mesh(patchOf(result.cells),result.valid?"accepted":"rejected",*reference);require(EncodeReference(*reference)==authority,"Changed geometry authority");
 std::vector<char> a,b;RecordStream sa(a),sb(b);for(auto row:input.cells)sa(row.second);for(auto c:before.cells){OwnedCell row=input.cells.at(c.id);row.cell=c;sb(row);}require(a==b,"Changed accepted input");
 std::ofstream out("result.json");out<<std::setprecision(17)<<"{\"valid\":"<<(result.valid?"true":"false")<<",\"seconds\":"<<elapsed<<",\"cells\":"<<result.cells.size()<<",\"source_donors\":"<<input.original.size()<<",\"work\":"<<result.stats.global_work<<",\"stop\":"<<std::quoted(EngineStopName(result.stats.stop))<<",\"region_attempts\":"<<result.stats.region_attempts<<",\"region_cells\":"<<result.stats.region_cells<<",\"region_fronts\":"<<result.stats.region_transition_fronts<<",\"region_field_points\":"<<result.stats.region_field_points<<",\"region_field_edges\":"<<result.stats.region_field_edges<<",\"reason\":"<<std::quoted(result.reason)<<",\"immutable_input\":true,\"unchanged_authority\":true,\"CFD_steps\":0}\n";out.close();require(bool(out),"Cannot save construction receipt");driver.Finalize();return 0;
}
int main(int argc,char** argv){SU2_MPI::Init(&argc,&argv);int code=0;try{require(argc==2,"Expected config");code=run(argv[1]);}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;code=1;}SU2_MPI::Finalize();return code;}
