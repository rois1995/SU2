// Same immutable selected original field; rejected finer geometry is only an integration proposal.
#include "../../../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "../../../../../SU2_CFD/include/adaptation/CNativeComplexity3D.hpp"
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
int run(char* cfg) {
 World world;Driver driver(cfg,1,world.comm);auto& g=driver.Geometry();auto reference=driver.Reference();
 std::ifstream f("selected_sensor.bin",std::ios::binary);uint64_t count;f.read(reinterpret_cast<char*>(&count),8);require(count==61330,"Changed original selected field");
 struct Record{std::array<double,3>x;std::array<double,9>m;};std::map<Id,Record> records;std::vector<CBoundaryLayerMetric::Tensor> selected(count);
 for(uint64_t k=0;k<count;++k){Id id;Record r;f.read(reinterpret_cast<char*>(&id),8);f.read(reinterpret_cast<char*>(r.x.data()),24);f.read(reinterpret_cast<char*>(r.m.data()),72);require(bool(f)&&id<count&&records.emplace(id,r).second,"Invalid selected field");auto& m=selected[id];for(unsigned a=0;a<3;++a)for(unsigned b=a;b<3;++b)m.m[a][b]=m.m[b][a]=r.m[3*a+b];}
 require(f.peek()==std::char_traits<char>::eof(),"Extra selected-field bytes");
 su2activematrix sensor(g.GetnPointDomain(),6);const unsigned upper[]={0,1,2,4,5,8};
 for(unsigned long p=0;p<sensor.rows();++p){const auto& r=records.at(g.nodes->GetGlobalIndex(p));for(unsigned d=0;d<3;++d)require(SU2_TYPE::GetValue(g.nodes->GetCoord(p,d))==r.x[d],"Moved original sensor node");for(unsigned d=0;d<6;++d)sensor(p,d)=r.m[upper[d]];}
 require(world.sum(uint64_t(sensor.rows()))==count,"Incomplete original selected publisher field");
 const auto start=world.seconds();auto input=ImportGeometry(driver.Config(),g,&sensor,*reference);const auto source_before=Fingerprint(input.original);
 auto layers=CBoundaryLayerMetric::FromNativeReference(*reference,driver.Config());layers->ConfigureOuterNormal(SU2_TYPE::GetValue(driver.Config().GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(driver.Config().GetAdap_BL_OuterGrowthMax()));layers->SelectOuterNormal(.875);layers->FreezeOuterNormal();
 const auto version=layers->GetOuterNormal().version;const double core=.003215747252591252;
 std::vector<CNativeComplexity3D::SourceCell> original;
 for(const auto& d:input.original){CNativeComplexity3D::SourceCell row;row.cell=d.cell;for(unsigned k=0;k<4;++k){require(d.cell.v[k].id<count,"Unexpected generated source alias");row.nodes[k]=d.cell.v[k].id;const auto& m=selected[row.nodes[k]];require(d.sensor[k].xx==m.m[0][0]&&d.sensor[k].xy==m.m[0][1]&&d.sensor[k].xz==m.m[0][2]&&d.sensor[k].yy==m.m[1][1]&&d.sensor[k].yz==m.m[1][2]&&d.sensor[k].zz==m.m[2][2],"Reference changed upper-six frozen donors");}original.push_back(row);}
 const auto before=world.seconds();CNativeComplexity3D estimator(*layers,std::move(original),count,size_t(2)<<30,32768,9919,false);const auto assessment=estimator.Assess(selected,core);const auto& st=estimator.GetStatistics();
 require(Fingerprint(input.original)==source_before&&layers->GetOuterNormal().version==version,"Reference changed original field/profile");
 std::ofstream out("reference_rank"+std::to_string(world.rank)+".json");out<<std::setprecision(17)<<"{\"rank\":"<<world.rank<<",\"ranks\":"<<world.size<<",\"value\":"<<assessment.global.value<<",\"uncertainty\":"<<assessment.global.uncertainty<<",\"sensor_lower\":"<<assessment.sensorLower<<",\"sensor_upper\":"<<assessment.sensorUpper<<",\"sensor_quadrature\":"<<assessment.sensorQuadrature<<",\"correction\":"<<assessment.correction<<",\"standard_error\":"<<assessment.standardError<<",\"samples\":"<<assessment.samples<<",\"inside\":"<<assessment.inside<<",\"nonzero\":"<<assessment.nonzero<<",\"preparation_seconds\":"<<st.preparationSeconds<<",\"assessment_seconds\":"<<st.assessmentSeconds<<",\"background_seconds\":"<<st.backgroundSeconds<<",\"correction_seconds\":"<<st.correctionSeconds<<",\"reduction_seconds\":"<<st.reductionSeconds<<",\"retained_bytes\":"<<st.retainedBytes<<",\"preparation_bytes\":"<<st.preparationBytes<<",\"support_work\":"<<st.supportWork<<",\"support_candidates\":"<<st.supportCandidates<<",\"seconds\":"<<world.seconds()-before<<",\"total_seconds\":"<<world.seconds()-start<<",\"immutable_source\":true,\"profile_version\":"<<version<<"}\n";out.close();require(bool(out),"Cannot save reference assessment");
 driver.Finalize();return 0;
}
int main(int argc,char** argv){SU2_MPI::Init(&argc,&argv);int code=0;try{require(argc==2,"Expected original config");code=run(argv[1]);}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;code=1;}SU2_MPI::Finalize();return code;}
