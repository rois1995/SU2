// Diagnostic only: actual captured volume cavities, not a whole mesh certificate.
#include "../../../../../../../Common/include/adaptation/CNativeBoundary3D.hpp"
#include "../../../../../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../../../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../../../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "../../../../../../../Common/include/parallelization/mpi_structure.hpp"
#include "../../../../../../../Common/include/CConfig.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <algorithm>
using namespace SU2Native3D;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void writeMesh(const Patch& p,const std::string& name){
  std::map<Id,Node> nodes;for(const auto& c:p.cells)for(auto n:c.v)nodes.emplace(n.id,n);
  std::map<Id,size_t> ids;for(const auto& n:nodes)ids[n.first]=ids.size();
  std::map<Face,size_t> counts;std::map<Face,Face> faces;
  for(const auto& c:p.cells){const auto& v=c.v;const std::array<Face,4> f{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};for(auto q:f){auto key=q;std::sort(key.begin(),key.end());++counts[key];faces[key]=q;}}
  std::ofstream o(name);o<<std::setprecision(17)<<"NDIME= 3\nNELEM= "<<p.cells.size()<<'\n';
  for(const auto& c:p.cells){o<<10;for(auto n:c.v)o<<' '<<ids.at(n.id);o<<'\n';}
  o<<"NPOIN= "<<nodes.size()<<'\n';for(auto n:nodes)o<<n.second.p.x<<' '<<n.second.p.y<<' '<<n.second.p.z<<' '<<ids.at(n.first)<<'\n';
  size_t skin=0;for(auto q:counts)skin+=q.second==1;o<<"NMARK= 1\nMARKER_TAG= ARTIFICIAL_PATCH_INTERFACE\nMARKER_ELEMS= "<<skin<<'\n';
  for(auto q:counts)if(q.second==1){o<<5;for(auto id:faces.at(q.first))o<<' '<<ids.at(id);o<<'\n';}require(bool(o),"Cannot save private diagnostic mesh");
}
int run(const std::string& root){
  std::ifstream cfg("serial.cfg");require(bool(cfg),"Missing config");CConfig config(cfg,SU2_COMPONENT::SU2_CFD,false);
  ReferenceState state;require(LoadReference(root+"/Coarse.su2.native_ref",state)&&bool(state.original),"Missing geometry reference");
  auto layers=CBoundaryLayerMetric::FromNativeReference(state,config);
  layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config.GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config.GetAdap_BL_OuterGrowthMax()));
  layers->SelectOuterNormal(.875);layers->FreezeOuterNormal();const double core=.003215747252591252;
  const MetricComposition compose=[&](Point p,Tensor s){CBoundaryLayerMetric::Tensor m;
    m.m[0][0]=s.xx;m.m[1][1]=s.yy;m.m[2][2]=s.zz;m.m[0][1]=m.m[1][0]=s.xy;m.m[0][2]=m.m[2][0]=s.xz;m.m[1][2]=m.m[2][1]=s.yz;
    const su2double x[]={p.x,p.y,p.z};layers->ApplyPoint(x,m,core,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};};
  const MetricEdgeBreakpoints bands=[&](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
  std::ifstream input("donors.dat");std::string tag;size_t count;input>>tag>>count;require(tag=="ORIGINAL_CAVITY_V1"&&count<=MaximumDonors,"Invalid donor fixture");
  Patch old;std::vector<DonorCell> donors;std::map<Id,Node> nodes;
  for(size_t k=0;k<count;++k){DonorCell d;int selected;input>>d.cell.id>>selected;
    for(unsigned j=0;j<4;++j){auto& n=d.cell.v[j];auto& m=d.sensor[j];input>>n.id>>n.p.x>>n.p.y>>n.p.z>>m.xx>>m.xy>>m.xz>>m.yy>>m.yz>>m.zz;nodes.emplace(n.id,n);}
    require(bool(input),"Truncated fixture");const auto& v=d.cell.v;if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0){std::swap(d.cell.v[0],d.cell.v[1]);std::swap(d.sensor[0],d.sensor[1]);}
    if(selected)old.cells.push_back(d.cell);donors.push_back(d);
  }
  std::ifstream working("working.dat");size_t nc,nf;working>>tag>>nc>>nf;
  require(tag=="NATIVE_PRIVATE_PATCH_V1"&&nc>0&&nc<=MaximumCavityCells&&nf<=4*MaximumCavityCells,"Expected captured interior cavity");
  old={};nodes.clear();for(size_t k=0;k<nc;++k){Cell c;working>>c.id;for(auto& n:c.v){working>>n.id>>n.p.x>>n.p.y>>n.p.z;nodes.emplace(n.id,n);}old.cells.push_back(c);}
  for(size_t k=0;k<nf;++k){SurfaceFace f;working>>f.v[0]>>f.v[1]>>f.v[2]>>f.facet;old.surface.push_back(f);}
  require(bool(working)&&nodes.rbegin()->first<1000000000,"Invalid captured node range");
  Edge edge;std::ifstream edgeFile("edge.dat");edgeFile>>edge[0]>>edge[1];require(bool(edgeFile),"Missing captured feature");
  FrozenField field(std::move(donors),compose,true,{},bands);const Point a=nodes.at(edge[0]).p,b=nodes.at(edge[1]).p;
  const auto before=MeasureMetricCells(old.cells,field);const auto length=field.TargetEdgeLength(a,b);
  std::ofstream report("trials.jsonl");report<<std::setprecision(17);
  report<<"{\"kind\":\"initial\",\"donors\":"<<count<<",\"q\":"<<before.minimum_mean_ratio<<",\"J\":"<<before.minimum_scaled_jacobian<<",\"max_length\":"<<double(before.maximum_length)<<",\"edge_length\":"<<double(length)<<"}\n";
  const Point mid=field.SegmentPoint(a,b,.5);
  report<<"{\"kind\":\"crease\",\"length\":"<<double(length)<<",\"left\":"<<double(field.TargetEdgeLength(a,mid))<<",\"right\":"<<double(field.TargetEdgeLength(mid,b))<<",\"midpoint\":["<<mid.x<<','<<mid.y<<','<<mid.z<<"],\"edge_limit\":1.8,\"canonical_facets\":[";
  for(size_t k=0;k<old.surface.size();++k){if(k)report<<',';report<<state.original->Canonical(old.surface[k].facet);}report<<"],\"analytic_authorities\":"<<state.original->Analytic().size()<<"}\n";
  require(bool(report),"Cannot save measurement");return 0;
}
int main(int argc,char** argv){SU2_MPI::Init(&argc,&argv);int code=0;try{require(argc==2,"Expected frozen input folder");code=run(argv[1]);}catch(const std::exception& e){std::cerr<<e.what()<<'\n';code=1;}SU2_MPI::Finalize();return code;}
