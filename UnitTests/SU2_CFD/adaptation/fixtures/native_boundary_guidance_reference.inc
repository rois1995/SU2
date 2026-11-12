// Frozen pre-reuse implementation: independent exact-result oracle for scoped strip reuse.
namespace native_boundary_guidance_test {
std::vector<BoundaryMetricSample> BeforeStripReuse(const SU2NativeBLPipeline3D::TargetPolicy& policy,
    Node node,const PlanarReference& context,Tensor sensor,const std::vector<std::string>& marker_names) {
  const auto& authority=policy.authority;const auto& base=policy.base;const auto& A=policy.A;
  const auto& roles=policy.roles;const auto core=policy.core;
  using SurfaceGuidance=SU2NativeBLPipeline3D::TargetPolicy::SurfaceGuidance;
  auto Unpack=[](Tensor input,CBoundaryLayerMetric::Tensor& metric){SU2NativeBLPipeline3D::TargetPolicy::Unpack(input,metric);};
  auto Pack=[](const CBoundaryLayerMetric::Tensor& metric){return SU2NativeBLPipeline3D::TargetPolicy::Pack(metric);};
  auto IntrinsicSurfaceGuidance=[&](unsigned short wall,Id facet,Node vertex,const PlanarReference& geometry,Tensor value) {
    return policy.IntrinsicSurfaceGuidance(wall,facet,vertex,geometry,value);
  };

    if(!authority||!base||!A)throw std::invalid_argument("Boundary guidance needs an initialized native target.");
    ValidateTensor(sensor);
    std::vector<BoundaryMetricSample> result;
    auto append=[&](BoundaryMetricSample sample) {
      if(result.size()==4*MaximumCavityCells)throw std::length_error("Requested-wall sided guidance exceeds cavity admission.");
      result.push_back(sample);
    };
    auto side=[&](Id facet) {
      const auto& face=authority->Get(facet);const auto role=roles.find(face.marker);
      if(role==roles.end()||role->second!=1)return;
      bool found=false;
      for(unsigned short wall=0;wall<base->GetnWall();++wall) {
        if(!base->OwnsNativeFacet(wall,facet,marker_names.at(face.marker)))continue;
        if(found)throw std::logic_error("Requested-wall facet has ambiguous component ownership.");
        found=true;SurfaceGuidance guidance;
        if(authority->Mapped()&&authority->Parameters()->Geometry().Curves().count(facet))
          guidance=IntrinsicSurfaceGuidance(wall,facet,node,context,sensor);
        else {
          const su2double x[3]={node.p.x,node.p.y,node.p.z};
          const auto own=base->Evaluate(wall,x,core);CBoundaryLayerMetric::Tensor metric;Unpack(sensor,metric);
          for(unsigned short w=0;w<base->GetnWall();++w)
            base->ApplySample(w,w==wall?own:base->Evaluate(w,x,core),metric,core,nullptr,false);
          guidance={Pack(metric),{SU2_TYPE::GetValue(own.normal[0]),SU2_TYPE::GetValue(own.normal[1]),SU2_TYPE::GetValue(own.normal[2])}};
        }
        BoundaryMetricSample sample{guidance.tensor,guidance.normal};append(sample);
        for(const auto& strip:A->strips) {
          long double width=0;dc10::V direction;
          if(!A->InStrip(strip,dc10::Of(node.p),width,direction))continue;
          // The existing four-cells-across strip rule is construction demand, not a changed flow/BL target.
          const auto scale=4*MaximumMetricEdge/(1.05L*width);
          sample.strip_rule={double(scale*direction.x),double(scale*direction.y),double(scale*direction.z)};append(sample);
        }
      }
      if(!found)throw std::invalid_argument("Requested-wall binding has no retained metric component.");
    };
    if(context.Parameters()) {
      if(authority->Mapped()&&authority->Parameters()->Authority()!=context.Parameters()->Authority())
        throw std::invalid_argument("Boundary guidance changed its immutable geometry authority.");
      const auto* point=context.Parameter(node.p);
      if(!point)return result;
      if(point->node.id!=node.id)throw std::invalid_argument("Boundary guidance coordinate has a different node identity.");
      context.Parameters()->Geometry().ForFacets(point->binding,side);
    } else {
      std::set<Id> components;
      for(const auto& face:authority->Facets()) {
        const auto role=roles.find(face.second.marker);
        if(role==roles.end()||role->second!=1)continue;
        const auto component=authority->Canonical(face.first);
        if(!components.count(component)&&context.Contains(face.first,node.p)){side(face.first);components.insert(component);}
      }
    }
    return result;

}
}

TEST_CASE("Boundary strip reuse preserves side order, changing positions and admission", "[NativeBoundaryStripReuse3D]") {
  using namespace native_target_checkpoint_test;
  auto config=Config();auto reference=Reference();
  for(auto& facet:reference.facets)facet.marker=0;
  auto original=std::make_shared<PlanarReference>(reference.facets);
  std::map<Id,double> errors;for(const auto& facet:reference.facets)errors.emplace(facet.id,1e-13);
  auto mapped=std::make_shared<SU2NativeGeometry::MappedFacetReference>(original,errors,7,SIZE_MAX,true);
  BoundaryPoints points;std::vector<SurfaceFace> surface;
  for(const auto& facet:reference.facets) {
    Face face;for(unsigned i=0;i<3;++i){face[i]=facet.v[i].id;points.emplace(face[i],BoundaryPoint{facet.v[i],mapped->Vertex(face[i])});}
    surface.push_back({face,facet.id});
  }
  auto parameters=std::make_shared<BoundaryParameterState>(mapped,points,surface);
  reference.original=PlanarReference::WithParameters(original,parameters);
  auto policy=SU2NativeBLPipeline3D::MakeTargetPolicy(*config,reference,1.,-1.);
  dc10::WedgeCrease a;a.a={0,0,0};a.b={1,0,0};a.t={1,0,0};a.L=1;
  auto b=a;b.a.y=b.b.y=1;policy->A->creases={a,b};
  const Tensor sensor{2.,.1,0.,3.,0.,4.};
  World world;
  for(const size_t strips:{size_t(0),size_t(2),size_t(80),size_t(100)}) {
    policy->A->strips.assign(strips,dc10::StripPair{0,1,{0,0,1}});
    // Every rank constructs the same authority; the last worker may have no local queries.
    if(world.size>1&&world.rank==world.size-1)continue;
    for(const auto& row:points) {
      const auto node=row.second.node;const bool exhausted=strips==100&&node.p.z==0;
      if(exhausted) {
        CHECK_THROWS_AS(policy->BoundaryGuidance(node,*reference.original,sensor,reference.marker_names),std::length_error);
        CHECK_THROWS_AS(native_boundary_guidance_test::BeforeStripReuse(*policy,node,*reference.original,sensor,reference.marker_names),std::length_error);
        continue;
      }
      const auto expected=native_boundary_guidance_test::BeforeStripReuse(*policy,node,*reference.original,sensor,reference.marker_names);
      const auto actual=policy->BoundaryGuidance(node,*reference.original,sensor,reference.marker_names);
      REQUIRE(actual.size()==expected.size());REQUIRE(actual.size()==3*(1+(node.p.z==0?strips:0)));
      for(size_t i=0;i<actual.size();++i) {
        CHECK(actual[i].tensor.xx==expected[i].tensor.xx);CHECK(actual[i].tensor.xy==expected[i].tensor.xy);
        CHECK(actual[i].tensor.xz==expected[i].tensor.xz);CHECK(actual[i].tensor.yy==expected[i].tensor.yy);
        CHECK(actual[i].tensor.yz==expected[i].tensor.yz);CHECK(actual[i].tensor.zz==expected[i].tensor.zz);
        CHECK(actual[i].normal.x==expected[i].normal.x);CHECK(actual[i].normal.y==expected[i].normal.y);CHECK(actual[i].normal.z==expected[i].normal.z);
        CHECK(actual[i].strip_rule.x==expected[i].strip_rule.x);CHECK(actual[i].strip_rule.y==expected[i].strip_rule.y);CHECK(actual[i].strip_rule.z==expected[i].strip_rule.z);
      }
    }
  }
  CHECK(world.maximum(1)==1);
}
