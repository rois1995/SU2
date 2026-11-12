// Diagnostic only: fixed current core/tangents isolate h0 continuation, not a renormalized target or generated BL.
std::ifstream stageInput(cfg);std::string line,stageText;
while(std::getline(stageInput,line)) {
  if(line.find("ADAP_SUBITER=")!=std::string::npos)line="ADAP_SUBITER= ( 7 )";
  REQUIRE(line.find("ADAP_BL_H0_RAMP=")==std::string::npos);
  stageText+=line+'\n';
}
// Existing x32 continuation: five halvings, plus two final-height events in a complete workflow.
stageText+="ADAP_BL_H0_RAMP= 5\n";
std::stringstream stageStream(stageText);std::unique_ptr<CConfig> staged;
{Mute mute;staged=std::make_unique<CConfig>(stageStream,SU2_COMPONENT::SU2_CFD,false);}
REQUIRE(staged->GetAdap_BL_H0Ramp()==5);
std::map<Id,Point> edgePoints;
for(const auto& row:current.mesh)for(auto n:row.second.cell.v)if(n.id==6032||n.id==6072)edgePoints.emplace(n.id,n.p);
REQUIRE(edgePoints.size()==2);
for(unsigned long k=0;k<=5;++k) {
  staged->SetAdap_BL_RampEvent(k);
  const double h=SU2_TYPE::GetValue(staged->GetAdap_BL(0).firstHeight);
  REQUIRE(h==std::ldexp(SU2_TYPE::GetValue(config.GetAdap_BL_True(0).firstHeight),int(5-k)));
  REQUIRE(staged->GetAdap_BL(0).growth==config.GetAdap_BL_True(0).growth);
  REQUIRE(staged->GetAdap_BL(0).thickness==config.GetAdap_BL_True(0).thickness);
  const auto start=std::chrono::steady_clock::now();const auto cpu=std::clock();
  REQUIRE(transfer_memory::Add(retained,directory.ResidentBytes(),transfer_memory::Mul(policy.StorageBytes(),3))<ceiling);
  auto full=CBoundaryLayerMetric::FromNativeReference(*reference,*staged);
  full->ReplayProfile(policy.full->GetProfileHistory());
  auto target=SU2NativeBLPipeline3D::MakeTargetPolicy(*staged,*reference,policy.core,policy.outer_parameter,full);
  REQUIRE(target->h0==h);REQUIRE(target->core==policy.core);
  REQUIRE(target->full->GetTangentialScale()==policy.full->GetTangentialScale());
  const size_t stageBytes=transfer_memory::Add(retained,directory.ResidentBytes(),target->StorageBytes(),policy.StorageBytes());
  REQUIRE(stageBytes<ceiling);
  SU2NativeGeometry::AtlasBudget stageBudget;
  stageBudget.maximum_bytes=ceiling;stageBudget.caller_bytes=stageBytes;
  REQUIRE(budget.work<=budget.maximum_work);stageBudget.maximum_work=budget.maximum_work-budget.work;
  stageBudget.maximum_integer_bits=budget.maximum_integer_bits;
  stageBudget.within_time=budget.within_time;
  auto stageLease=target->ReserveGeometry(stageBudget);
  WallHeights stageHeights=heights;
  for(unsigned short w=0;w<staged->GetnAdap_BL();++w) {
    const auto& layer=staged->GetAdap_BL(w);const auto marker=std::find(reference->marker_names.begin(),reference->marker_names.end(),layer.marker);
    REQUIRE(marker!=reference->marker_names.end());stageHeights[int(marker-reference->marker_names.begin())]=SU2_TYPE::GetValue(layer.firstHeight);
  }
  const double preparation=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
  for(const auto& audit:heightAudits)for(unsigned side=0;side<2;++side) {
    const auto& patch=side?audit.fresh:audit.old;std::string why;FirstHeightReport height;
    const auto work=budget.work;const auto t=std::chrono::steady_clock::now();bool strict=false,measured=false;
    size_t q=0,j=0;double minimumQ=INFINITY,minimumJ=INFINITY,finalTensorDifference=0;
    try {
      strict=CheckFirstHeight(patch,stageHeights,*current.reference,height,why);
      auto imported=source.ImportCellsLocal(patch.cells,audit.rows,ceiling,stageBytes);
      if(!imported.valid)throw std::runtime_error(imported.reason);
      const auto extensions=AssociatedExtensions(imported.cells,*current.reference);
      FrozenField field(std::move(imported.cells),{},true,extensions,{},identity,true,MaximumDonors,true);
      const BoundaryMetric boundary=[&](Node node,const PlanarReference& geometry,Tensor sensor) {
        return target->BoundaryGuidance(node,geometry,sensor,reference->marker_names);
      };
      MetricStats planningStats;const auto planning=MeasureSurfacePlanning(patch,field,*current.reference,stageHeights,boundary,&planningStats);
      std::set<Edge> requested;
      for(const auto& face:patch.surface)if(stageHeights.count(current.reference->Get(face.facet).marker))for(unsigned e=0;e<3;++e) {
        Edge key{face.v[e],face.v[(e+1)%3]};std::sort(key.begin(),key.end());requested.insert(key);
      }
      log<<"{\"stage\":\"height_continuation_tangential_edges\",\"k\":"<<k<<",\"context\":"<<std::quoted(audit.context)
         <<",\"side\":"<<std::quoted(side?"fresh":"old")<<",\"edge_evaluations\":"<<planningStats.edge_evaluations<<",\"edges\":[";
      bool first=true;for(const auto& edge:planning.edges)if(requested.count(edge.first)) {
        if(!first)log<<',';first=false;log<<'['<<edge.first[0]<<','<<edge.first[1]<<','<<edge.second.length<<']';
      }log<<"],\"full_combined_edge_certificate\":false}"<<std::endl;
      const auto compose=target->Composition();
      for(const auto& cell:patch.cells) {
        const auto x=Centroid(cell);const auto sensor=field.Sensor(x);const auto tensor=compose(x,sensor);
        if(k==5) {
          const auto baseline=policy.Composition()(x,sensor);
          const std::array<double,6> a{tensor.xx,tensor.xy,tensor.xz,tensor.yy,tensor.yz,tensor.zz};
          const std::array<double,6> b{baseline.xx,baseline.xy,baseline.xz,baseline.yy,baseline.yz,baseline.zz};
          for(unsigned i=0;i<6;++i)finalTensorDifference=std::max(finalTensorDifference,std::abs(a[i]-b[i])/std::max(1.,std::abs(b[i])));
          REQUIRE(finalTensorDifference<=1e-12);
        }
        const auto m=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},tensor);
        q+=m.mean_ratio<MinimumMeanRatio;j+=m.minimum_scaled_jacobian<MinimumScaledJacobian;
        minimumQ=std::min(minimumQ,double(m.mean_ratio));minimumJ=std::min(minimumJ,double(m.minimum_scaled_jacobian));
      }measured=true;
    }catch(const std::exception& e){why=e.what();}
    const double g=SU2_TYPE::GetValue(staged->GetAdap_BL(0).growth),thickness=SU2_TYPE::GetValue(staged->GetAdap_BL(0).thickness);
    const auto idealRows=std::ceil(std::log1p((g-1)*thickness/h)/std::log(g));
    log<<"{\"stage\":\"height_continuation_patch\",\"k\":"<<k<<",\"N\":5,\"factor\":"<<staged->GetAdap_BL_RampFactor()
       <<",\"h0\":"<<h<<",\"growth\":"<<g<<",\"thickness\":"<<thickness<<",\"ideal_geometric_rows_to_T\":"<<idealRows
       <<",\"context\":"<<std::quoted(audit.context)<<",\"operator\":"<<std::quoted(audit.operation)<<",\"side\":"<<std::quoted(side?"fresh":"old")
       <<",\"strict_height\":"<<strict<<",\"height_measured\":"<<height.measured<<",\"shape_measured\":"<<measured
       <<",\"q_bad\":"<<q<<",\"J_bad\":"<<j<<",\"minimum_q\":"<<(measured?minimumQ:-1)<<",\"minimum_J\":"<<(measured?minimumJ:-1)
       <<",\"final_tensor_checked\":"<<(k==5&&measured)<<",\"final_tensor_relative_difference\":"<<finalTensorDifference<<",\"geometry_work\":"<<budget.work-work<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count()
       <<",\"reason\":"<<junction_wp0::Json(why)<<",\"fixed_core_tangents\":true,\"generated_BL\":false,\"mesh_accepted\":false}"<<std::endl;
  }
  // Same difficult control edge at every stage. Independent short diagnostic window cannot poison later stages.
  const auto edgeStart=std::chrono::steady_clock::now();
  stageBudget.within_time=[&,edgeStart](){return budget.within_time()&&std::chrono::duration<double>(std::chrono::steady_clock::now()-edgeStart).count()<20;};
  bool complete=false;long double length=-1;std::string why;FieldStats stats;
  try {
    std::vector<Cell> seed;std::vector<OwnedCell> rows;
    for(const auto& audit:heightAudits)if(audit.context=="unchanged_wall_control"){seed=audit.old.cells;rows=audit.rows;break;}
    REQUIRE_FALSE(seed.empty());auto imported=source.ImportCellsLocal(seed,rows,ceiling,stageBytes);
    if(!imported.valid)throw std::runtime_error(imported.reason);
    const auto extensions=AssociatedExtensions(imported.cells,*current.reference);
    FrozenField field(std::move(imported.cells),target->Composition(),true,extensions,target->Edges(),std::make_shared<const FrozenMetricIdentity>(),true,MaximumDonors,true);
    try {length=field.TargetEdgeLength(edgePoints.at(6032),edgePoints.at(6072));complete=true;}
    catch(const std::exception& e){why=e.what();}stats=field.Statistics();
  }catch(const std::exception& e){why=e.what();}
  log<<"{\"stage\":\"height_continuation_edge\",\"k\":"<<k<<",\"N\":5,\"h0\":"<<h<<",\"a\":6032,\"b\":6072,\"complete\":"<<complete
     <<",\"length\":"<<length<<",\"profile_preparation_seconds\":"<<preparation<<",\"edge_seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-edgeStart).count()
     <<",\"edge_geometry_seconds_nested\":"<<stats.edge_geometry_seconds<<",\"edge_integral_seconds\":"<<stats.edge_integral_seconds
     <<",\"edge_composition_seconds_nested\":"<<stats.edge_composition_seconds<<",\"compositions\":"<<stats.edge_compositions
     <<",\"stage_geometry_work\":"<<stageBudget.work<<",\"cpu_seconds_total\":"<<double(std::clock()-cpu)/CLOCKS_PER_SEC
     <<",\"reason\":"<<junction_wp0::Json(why)<<",\"fixed_core_tangents\":true,\"renormalized\":false}"<<std::endl;
  budget.work=transfer_memory::Add(budget.work,stageBudget.work);budget.peak_bytes=std::max(budget.peak_bytes,stageBudget.peak_bytes);
  REQUIRE(budget.work<=budget.maximum_work);
}
