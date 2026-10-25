#define main dc18_standalone_main
#include "integration_evidence/native_3d_bl_v1/dc18_prep/test_curved_reference.cpp"
#undef main
#undef CHECK
// Frozen original M6 field diagnostic; no mesh construction or acceptance claim.
#include "SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "SU2_CFD/include/adaptation/CNativeComplexity3D.hpp"
#include "Common/include/adaptation/CNativeField3D.hpp"
#include "Common/include/adaptation/CNativeImport3D.hpp"
#include <iomanip>
#include "Common/include/adaptation/CNativeSupport3D.hpp"
#include "Common/include/linear_algebra/blas_structure.hpp"
#include "Common/include/CConfig.hpp"
using BL=CBoundaryLayerMetric;
using namespace SU2Native3D;
static BL::Tensor Unpack(const Tensor& m) {
  BL::Tensor t;t.m[0][0]=m.xx;t.m[1][1]=m.yy;t.m[2][2]=m.zz;
  t.m[0][1]=t.m[1][0]=m.xy;t.m[0][2]=t.m[2][0]=m.xz;t.m[1][2]=t.m[2][1]=m.yz;return t;
}
static double Ratio(const BL::Tensor& a,const BL::Tensor& b,const std::array<double,3>& x,const std::array<double,3>& y,double growth) {
  su2double vec[3][3],val[3],work[3],inv[3][3],test[3][3]={},distance2=0;
  CBlasStructure::EigenDecomposition(a.m,vec,val,3,work);
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
    inv[i][j]=vec[i][j]/sqrt(val[j]);distance2+=(x[i]-y[i])*b.m[i][j]*(x[j]-y[j]);
  }
  const auto factor=1/pow(1+growth*sqrt(fmax(0.,distance2)),2);
  for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)for(unsigned l=0;l<3;++l)
    test[i][j]+=inv[k][i]*factor*b.m[k][l]*inv[l][j];
  CBlasStructure::EigenDecomposition(test,vec,val,3,work);return *std::max_element(val,val+3);
}
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int exitCode=0;
  try {
    if(argc!=5)throw std::runtime_error("usage: probe mesh config donors output.json");
    std::ifstream cf(argv[2]);std::stringstream input;std::string line;
    while(std::getline(cf,line))if(line.rfind("ADAP_LOOP=",0)!=0)input<<line<<"\n";
    input<<"\nADAP_BL_METHOD= FIRST_ROW\nADAP_BL_H0_RAMP= 5\nADAP_LOOP= YES\nADAP_SIZES= (500000,500000,500000,500000,500000,500000)\n";
    CConfig config(input,SU2_COMPONENT::SU2_CFD,false);config.SetAdap_BL_RampEvent(1);
    const auto mesh=ReadSu2(argv[1]);ReferenceState state;
    for(const auto& marker:mesh.markers) {
      const int m=state.marker_names.size();state.marker_names.push_back(marker.first);
      for(const auto& face:marker.second) {
        if(face.size()!=3)throw std::runtime_error("Probe requires tetrahedral surface triangles.");
        Facet f;f.id=state.facets.size();f.marker=m;
        for(unsigned k=0;k<3;++k){const auto& p=mesh.p.at(face[k]);f.v[k]={Id(face[k]),{p[0],p[1],p[2]}};}
        state.facets.push_back(f);
      }
    }
    std::vector<DonorCell> donors;double core=INFINITY;
    {std::ifstream in(argv[3],std::ios::binary);uint64_t count=0;in.read(reinterpret_cast<char*>(&count),8);
     if(!in || !count || count>20000000)throw std::runtime_error("Invalid donor count.");donors.resize(count);
     for(size_t r=0;r<count;++r){auto& d=donors[r];d.cell.id=r;
       for(unsigned k=0;k<4;++k){uint64_t id;double x[3],m[6];in.read(reinterpret_cast<char*>(&id),8);in.read(reinterpret_cast<char*>(x),24);in.read(reinterpret_cast<char*>(m),48);
         d.cell.v[k]={id,{x[0],x[1],x[2]}};d.sensor[k]={m[0],m[1],m[2],m[3],m[4],m[5]};
         const auto a=Unpack(d.sensor[k]);su2double v[3][3],l[3],w[3];CBlasStructure::EigenDecomposition(a.m,v,l,3,w);core=std::min(core,double(*std::min_element(l,l+3)));
       }
       if(Orientation(d.cell.v[0].p,d.cell.v[1].p,d.cell.v[2].p,d.cell.v[3].p)<0){std::swap(d.cell.v[0],d.cell.v[1]);std::swap(d.sensor[0],d.sensor[1]);}
     }
     if(!in || !(core>0))throw std::runtime_error("Incomplete or non-SPD donors.");}
    std::map<std::array<Id,3>,std::pair<size_t,unsigned>> surface;
    for(size_t f=0;f<state.facets.size();++f) {
      auto key=std::array<Id,3>{state.facets[f].v[0].id,state.facets[f].v[1].id,state.facets[f].v[2].id};std::sort(key.begin(),key.end());
      if(!surface.emplace(key,std::make_pair(f,0)).second)throw std::runtime_error("Duplicate boundary facet.");
    }
    for(const auto& d:donors)for(unsigned k=0;k<4;++k) {
      std::array<Id,3> key;unsigned next=0;for(unsigned j=0;j<4;++j)if(j!=k)key[next++]=d.cell.v[j].id;std::sort(key.begin(),key.end());
      const auto found=surface.find(key);if(found==surface.end())continue;
      auto& f=state.facets[found->second.first];++found->second.second;
      if(Orientation(f.v[0].p,f.v[1].p,f.v[2].p,d.cell.v[k].p)>0)std::swap(f.v[1],f.v[2]);
    }
    for(const auto& f:surface)if(f.second.second!=1)throw std::runtime_error("Boundary does not bind to exactly one original donor.");
    state.original=std::make_shared<PlanarReference>(state.facets);
    std::vector<CNativeComplexity3D::SourceCell> source;source.reserve(donors.size());
    std::vector<BL::Tensor> original(mesh.p.size());std::vector<bool> seen(mesh.p.size());
    for(const auto& d:donors) {
      CNativeComplexity3D::SourceCell row;row.cell=d.cell;
      for(unsigned k=0;k<4;++k) {
        const auto id=d.cell.v[k].id;if(id>=original.size())throw std::runtime_error("Original node identity outside mesh.");
        row.nodes[k]=id;const auto m=Unpack(d.sensor[k]);
        if(seen[id])for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(original[id].m[i][j]!=m.m[i][j])throw std::runtime_error("Unequal sensor values for one original node.");
        original[id]=m;seen[id]=true;
      }
      source.push_back(row);
    }
    const size_t count=donors.size();FrozenField frozen(std::move(donors),{},false,{},{},{},true,count);
    auto old=BL::FromNativeReference(state,config),now=BL::FromNativeReference(state,config);
    auto base=BL::FromNativeReference(state,config,179.999);
    for(auto* layer:{old.get(),now.get(),base.get()})if(config.GetAdap_BL_Budget()) {
      layer->ConfigureOuterNormal(config.GetAdap_BL_ProtectDistance(),config.GetAdap_BL_OuterGrowthMax());layer->SelectOuterNormal(.75);layer->FreezeOuterNormal();
    }
    now->ConfigureOuterTransition(core,config.GetAdap_Hgrad());base->ConfigureOuterTransition(core,config.GetAdap_Hgrad());
    std::ofstream detail(std::string(argv[4])+".rays");detail<<"facet marker distance old_ratio new_ratio sensor_ratio normal_change foot_motion distance_delta base_ratio\n"<<std::setprecision(12);
    std::map<std::string,size_t> failureReasons;
    size_t rays=0,failed=0,oldBad=0,newBad=0,sensorBad=0;double oldWorst=1,newWorst=1,sensorWorst=1,seconds=0;
    std::vector<const Facet*> walls;
    for(const auto& f:state.facets)for(unsigned b=0;b<config.GetnAdap_BL();++b)if(state.marker_names[f.marker]==config.GetAdap_BL(b).marker)walls.push_back(&f);
    const auto started=std::chrono::steady_clock::now();
    for(size_t at=0;at<walls.size();at+=std::max<size_t>(1,walls.size()/1024)) {
      const auto& f=*walls[at];auto a=f.v[0].p,b=f.v[1].p,c=f.v[2].p;
      std::array<double,3> g{(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3,(a.z+b.z+c.z)/3};
      auto n=SU2NativeCurved::Unit<3>(SU2NativeCurved::Cross({b.x-a.x,b.y-a.y,b.z-a.z},{c.x-a.x,c.y-a.y,c.z-a.z}));
      double thickness=0;for(unsigned w=0;w<config.GetnAdap_BL();++w)if(config.GetAdap_BL(w).marker==state.marker_names[f.marker])thickness=config.GetAdap_BL(w).thickness;
      BL::Tensor previousOld,previousNew,previousSensor,previousBase;std::array<double,3> previousPoint;bool previous=false; BL::Sample previousWall; unsigned wallIndex=0;
      for(unsigned w=0;w<now->GetnWall();++w)if(now->GetLayer(w).marker==state.marker_names[f.marker])wallIndex=w;
      for(double offset:{.899,.9,.925,.95,.975,1.,1.025,1.1,1.5,2.})try {
        std::array<double,3> x;for(unsigned j=0;j<3;++j)x[j]=g[j]-offset*thickness*n[j];
        const auto sensor=frozen.Query({x[0],x[1],x[2]}).sensor;auto before=Unpack(sensor),after=before,withoutFeatures=before;
        base->ApplyPoint(x.data(),withoutFeatures,core,nullptr,false);
        old->ApplyPoint(x.data(),before,core,nullptr,false);now->ApplyPoint(x.data(),after,core,nullptr,false); const auto wallSample=now->Evaluate(wallIndex,x.data(),core);
        if(previous){const auto h=std::log(double(config.GetAdap_Hgrad()));const double ro=std::max(Ratio(before,previousOld,x,previousPoint,h),Ratio(previousOld,before,previousPoint,x,h));
          const double rn=std::max(Ratio(after,previousNew,x,previousPoint,h),Ratio(previousNew,after,previousPoint,x,h));
          const auto rb=std::max(Ratio(withoutFeatures,previousBase,x,previousPoint,h),Ratio(previousBase,withoutFeatures,previousPoint,x,h));
          const auto rs=std::max(Ratio(Unpack(sensor),previousSensor,x,previousPoint,h),Ratio(previousSensor,Unpack(sensor),previousPoint,x,h));
          sensorBad+=rs>1.01;sensorWorst=std::max(sensorWorst,rs);
          oldWorst=std::max(oldWorst,ro);newWorst=std::max(newWorst,rn);oldBad+=ro>1.01;newBad+=rn>1.01;++rays;
          double normalChange=0,footMotion=0;
          for(unsigned j=0;j<3;++j){normalChange+=pow(wallSample.normal[j]-previousWall.normal[j],2);
            footMotion+=pow(x[j]-wallSample.distance*wallSample.normal[j]-previousPoint[j]+previousWall.distance*previousWall.normal[j],2);}
          detail<<f.id<<' '<<f.marker<<' '<<offset*thickness<<' '<<ro<<' '<<rn<<' '<<rs<<' '<<sqrt(normalChange)<<' '<<sqrt(footMotion)<<' '<<(wallSample.distance-previousWall.distance)<<' '<<rb<<'\n';}
        previousBase=withoutFeatures;previousWall=wallSample;previousOld=before;previousNew=after;previousSensor=Unpack(sensor);previousPoint=x;previous=true;
      }catch(const std::exception& e){++failed;++failureReasons[e.what()];previous=false;}
    }
    for(const auto& e:failureReasons)std::cerr<<"query refusal "<<e.second<<": "<<e.first<<"\n";
    seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    std::string sampling="not attempted";size_t inside=0;
    try{auto sampler=now->PrepareCorrectionSupport3D(size_t(2)<<30,80000000);const auto draws=sampler->Draw(64,4817);
      for(const auto& p:draws)try{frozen.Query(p.point);++inside;}catch(const std::exception&){}
      sampling="completed";
    }catch(const std::exception& e){sampling=e.what();}
    std::ofstream out(argv[4]);out<<std::setprecision(15)<<"{\"probe_completed\":true,\"mesh_constructed\":false,\"policy\":\"full D1, original P1 sensor\",\"core\":"<<core<<",\"pairs\":"<<rays<<",\"failed_queries\":"<<failed<<",\"old_bad\":"<<oldBad<<",\"new_bad\":"<<newBad<<",\"old_worst\":"<<oldWorst<<",\"new_worst\":"<<newWorst<<",\"sensor_bad\":"<<sensorBad<<",\"sensor_worst\":"<<sensorWorst<<",\"query_seconds\":"<<seconds<<",\"sampling\":\""<<sampling<<"\",\"sample_inside_64\":"<<inside<<"}\n";
    out.close();
    if(!rays)throw std::runtime_error("No completed field probe rays.");
    const auto preparationStart=std::chrono::steady_clock::now();
    CNativeComplexity3D estimate(*now,std::move(source),original.size(),size_t(2)<<30,8192,7519,true);
    auto result=estimate.Assess(original,core,true);
    std::ofstream stages(std::string(argv[4])+".complexity.stages");
    std::string reason;
    do { stages<<result.samples<<' '<<result.global.value<<' '<<result.global.uncertainty<<'\n'<<std::flush;
      if(!estimate.Refine(reason))break; result=estimate.Assess(original,core,true);
    } while(true);
    stages<<"refinement stopped: "<<reason<<'\n';
    const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-preparationStart).count();
    const auto& stats=estimate.GetStatistics();
    std::ofstream complexity(std::string(argv[4])+".complexity.json");
    complexity<<std::setprecision(15)<<"{\"completed\":true,\"value\":"<<result.global.value<<",\"uncertainty\":"<<result.global.uncertainty<<",\"samples\":"<<result.samples<<",\"inside\":"<<result.inside<<",\"nonzero\":"<<result.nonzero<<",\"seconds\":"<<elapsed<<",\"retained_bytes\":"<<stats.retainedBytes<<",\"support_candidates\":"<<stats.supportCandidates<<"}\n";
  }catch(const std::exception& e){std::cerr<<e.what()<<'\n';exitCode=1;}
  SU2_MPI::Finalize();return exitCode;
}
