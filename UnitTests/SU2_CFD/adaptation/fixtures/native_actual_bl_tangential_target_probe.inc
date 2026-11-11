// Same saved production inputs; centroid/vertex tensor diagnostics are not exact cell quadrature or BL acceptance.
auto policy=SU2NativeBLPipeline3D::MakeTargetPolicy(config,*reference,1e-4,-1);
const bool checkpointTarget=false;
auto savedSensorAt=[&](Point){return Tensor{policy->core,0,0,policy->core,0,policy->core};};
auto lease=policy->ReserveGeometry(budget);
std::ofstream out("actual_bl_tangential_target.jsonl");REQUIRE(bool(out));out<<std::setprecision(17);
for(size_t index=0;index<count;++index) {
  std::string label;Edge edge{};size_t n=0;input>>label>>edge[0]>>edge[1]>>n;
  REQUIRE(bool(input));REQUIRE(n>0);REQUIRE(n<=MaximumCavityCells);
  std::set<Id> selected;
  for(size_t i=0;i<n;++i){std::array<Id,4> key;for(auto& id:key)input>>id;std::sort(key.begin(),key.end());selected.insert(byVertices.at(key));}
  REQUIRE(selected.size()==n);const auto patch=patchOf(selected);
  struct Query {Point p;std::vector<CBoundaryLayerMetric::PointSample> full,base;long double weight;uint64_t fullVersion=0,baseVersion=0;};
  std::vector<Query> queries;queries.reserve(patch.cells.size());
  policy->full->SelectTangentialScale(1);policy->Update(1e-4,SU2_TYPE::GetValue(config.GetAdap_Hgrad()));
  const auto workBefore=budget.work;
  for(const auto& cell:patch.cells) {
    Point p{};for(auto node:cell.v){p.x+=node.p.x/4;p.y+=node.p.y/4;p.z+=node.p.z/4;}
    queries.push_back({p,policy->full->SamplePoints({p.x,p.y,p.z}),policy->base->SamplePoints({p.x,p.y,p.z}),policy->A->Weight(junction_wp0::Of(p)),policy->full->GetGeometrySampleVersion(),policy->base->GetGeometrySampleVersion()});
  }
  for(double scale:{1.,.75,.5,.25}) {
    policy->full->SelectTangentialScale(scale);policy->Update(1e-4,SU2_TYPE::GetValue(config.GetAdap_Hgrad()));
    size_t qb=0,jb=0;double normalRatio=1,minEdge=INFINITY,maxEdge=0;long double density=0;
    for(size_t c=0;c<queries.size();++c) {
      auto& q=queries[c];
      if(q.fullVersion!=policy->full->GetGeometrySampleVersion())q.full=policy->full->SamplePoints({q.p.x,q.p.y,q.p.z});
      if(q.baseVersion!=policy->base->GetGeometrySampleVersion())q.base=policy->base->SamplePoints({q.p.x,q.p.y,q.p.z});
      q.fullVersion=policy->full->GetGeometrySampleVersion();q.baseVersion=policy->base->GetGeometrySampleVersion();
      q.weight=policy->A->Weight(junction_wp0::Of(q.p));
      for(auto& v:q.full){const auto hn=v.sample.hn;policy->full->RecomposeTangentialSample(v.wall,v.sample,1.);
        if(v.sample.distance<=.009)normalRatio=std::max(normalRatio,double(std::max(v.sample.hn/hn,hn/v.sample.hn)));}
      for(auto& v:q.base)policy->base->RecomposeTangentialSample(v.wall,v.sample,1.);
      CBoundaryLayerMetric::Tensor m;for(unsigned j=0;j<3;++j)m.m[j][j]=1e-4;
      auto direct=m;policy->ApplySamples(q.full,q.base,q.weight,m);policy->ApplyPoint(q.p,direct);
      for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)CHECK(m.m[i][j]==direct.m[i][j]);
      Tetrahedron tet;for(unsigned j=0;j<4;++j)tet[j]=patch.cells[c].v[j].p;
      const auto tensor=SU2NativeBLPipeline3D::TargetPolicy::Pack(m);const auto measured=Measure(tet,tensor);
      qb+=measured.mean_ratio<MinimumMeanRatio;jb+=measured.minimum_scaled_jacobian<MinimumScaledJacobian;
      for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
        const auto d=junction_wp0::Of(tet[j])-junction_wp0::Of(tet[i]);
        const auto length=std::sqrt(d.x*(tensor.xx*d.x+tensor.xy*d.y+tensor.xz*d.z)+d.y*(tensor.xy*d.x+tensor.yy*d.y+tensor.yz*d.z)+d.z*(tensor.xz*d.x+tensor.yz*d.y+tensor.zz*d.z));
        minEdge=std::min(minEdge,double(length));maxEdge=std::max(maxEdge,double(length));
      }
      const auto& a=m.m;density+=std::sqrt(std::max(0.,double(a[0][0]*(a[1][1]*a[2][2]-a[1][2]*a[2][1])-a[0][1]*(a[1][0]*a[2][2]-a[1][2]*a[2][0])+a[0][2]*(a[1][0]*a[2][1]-a[1][1]*a[2][0]))));
    }
    CHECK(normalRatio==Approx(1.).epsilon(1e-12));
    out<<"{\"context\":"<<std::quoted(label)<<",\"tangent_eigenvalue_scale\":"<<scale<<",\"cells\":"<<queries.size()
       <<",\"centroid_control_q_bad\":"<<qb<<",\"centroid_control_J_bad\":"<<jb<<",\"centroid_density_sum\":"<<double(density)
       <<",\"centroid_control_min_edge\":"<<minEdge<<",\"centroid_control_max_edge\":"<<maxEdge<<",\"normal_hn_ratio_in_full_band\":"<<normalRatio
       <<",\"geometry_work\":"<<budget.work-workBefore<<",\"exact_edge_quadrature\":false,\"production_acceptance\":false}"<<std::endl;
  }
}

#include "native_actual_bl_saved_tangent_probe.inc"
