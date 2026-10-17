// Frozen original-field diagnostic; ordered marginal densities telescope across interacting walls.
#pragma once
class CostDiagnostic3D {
  std::ofstream output;
  double coarse=0;
 public:
  CostDiagnostic3D(int rank) {
    const char* hmax=std::getenv("SU2_NATIVE_BL_COST_HMAX");
    if(!hmax)return;
    const auto h=std::stod(hmax);
    if(!(h>0) || !std::isfinite(h) || !(1/(h*h)>0))throw std::runtime_error("Invalid cost-diagnostic HMAX.");
    coarse=1/(h*h);
    output.open("bl_cost_rank"+std::to_string(rank)+".csv");
    output<<std::setprecision(17)<<"row,wall,distance,hn,weight,ht_min,ht_max,foot_x,foot_y,foot_z,original_weighted_marginal,coarsest_weighted_marginal\n";
    if(!output)throw std::runtime_error("Cannot open BL-cost diagnostic.");
  }
  void Record(CBoundaryLayerMetric& layers,const Request& request,const Metric& original,double core) {
    if(!coarse)return;
    auto flow=original;Metric background;for(unsigned i=0;i<3;++i)background.m[i][i]=coarse;
    auto beforeFlow=LogDensity(flow),beforeBackground=LogDensity(background);
    const auto p=request.point;
    for(const auto& point:layers.SamplePoints({p.x,p.y,p.z})) {
      const auto& s=point.sample;const auto wall=point.wall;
      layers.ApplySample(wall,s,flow,core,nullptr,false);
      layers.ApplySample(wall,s,background,coarse,nullptr,false);
      const auto afterFlow=LogDensity(flow),afterBackground=LogDensity(background);
      const auto flowDelta=std::exp(beforeFlow)*std::expm1(afterFlow-beforeFlow)*request.inversePDF;
      const auto backgroundDelta=std::exp(beforeBackground)*std::expm1(afterBackground-beforeBackground)*request.inversePDF;
      std::array<long double,3> n{},u{},v{};
      for(unsigned i=0;i<3;++i)n[i]=SU2_TYPE::GetValue(s.normal[i]);
      const auto axis=std::min_element(n.begin(),n.end(),[](auto a,auto b){return std::abs(a)<std::abs(b);})-n.begin();
      u[(axis+1)%3]=n[(axis+2)%3];u[(axis+2)%3]=-n[(axis+1)%3];
      const auto norm=std::sqrt(u[0]*u[0]+u[1]*u[1]+u[2]*u[2]);for(auto& x:u)x/=norm;
      v={n[1]*u[2]-n[2]*u[1],n[2]*u[0]-n[0]*u[2],n[0]*u[1]-n[1]*u[0]};
      long double a=0,b=0,c=0;
      for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j) {
        const long double m=SU2_TYPE::GetValue(s.full.m[i][j]);a+=u[i]*m*u[j];b+=u[i]*m*v[j];c+=v[i]*m*v[j];
      }
      const auto large=(a+c+std::hypot(a-c,2*b))/2,small=std::fma(a,c,-b*b)/large;
      if(!(small>0 && large>0) || !std::isfinite(flowDelta) || !std::isfinite(backgroundDelta))
        throw std::runtime_error("Invalid BL-cost diagnostic tensor/contribution.");
      output<<request.row<<','<<wall<<','<<s.distance<<','<<s.hn<<','<<s.weight<<','<<1/std::sqrt(large)<<','<<1/std::sqrt(small)
        <<','<<p.x-s.distance*n[0]<<','<<p.y-s.distance*n[1]<<','<<p.z-s.distance*n[2]<<','<<flowDelta<<','<<backgroundDelta<<'\n';
      if(!output)throw std::runtime_error("Cannot write BL-cost diagnostic.");
      beforeFlow=afterFlow;beforeBackground=afterBackground;
    }
  }
  void Close() {
    if(!coarse)return;output.close();if(!output)throw std::runtime_error("Cannot close BL-cost diagnostic.");
  }
};
