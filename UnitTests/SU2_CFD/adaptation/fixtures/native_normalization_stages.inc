// Original frames and original-P1 stencils, never an adapted-node replacement field.
namespace normalization_test {
std::string Options(const std::string& name,int workers,bool walls=true) {
  std::ostringstream s;s<<"SOLVER= RANS\nKIND_TURB_MODEL= SA\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= .2\nREYNOLDS_NUMBER= 1e6\nREYNOLDS_LENGTH= 1\n"
    <<"MESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\nMARKER_FAR= (far)\nMARKER_HEATFLUX= (wall,0)\n"
    <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\nADAP_ISO_CORNER= NO\n"
    <<"ADAP_HMIN= .1\nADAP_HMAX= 1.0000000000000002\nADAP_ARMAX= 100\nADAP_COMPLEXITY= 20\nADAP_HGRAD= 1.3\n"
    <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nADAP_NATIVE_MAX_WORK= 8192\nWRT_PERFORMANCE= YES\n"
    <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
    <<"TIME_DOMAIN= YES\nTIME_STEP= .001\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
    <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nRESTART_FILENAME= "<<name<<"_restart\nSOLUTION_FILENAME= "<<name<<"_restart\n";
  if(walls)s<<"ADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (.1)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.3)\n";
  return s.str();
}
std::unique_ptr<TetraDriver> Driver(World& world,const std::string& name,int workers,bool walls=true) {
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(3,1,[](const passivedouble* p){return p[2]==0?"wall":"far";}),name+".su2");
    std::ofstream out(name+".cfg");out<<Options(name,workers,walls);
  }
  SU2_MPI::Barrier(world.comm);auto cfg=name+".cfg";
  return std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
}
std::vector<double> Raw(const CNativeMetricFamily3D& f) {
  std::vector<double> v;for(auto x:f.eigenvalues)v.push_back(SU2_TYPE::GetValue(x));
  for(const auto& m:f.frames)for(const auto& row:m.m)for(auto x:row)v.push_back(SU2_TYPE::GetValue(x));return v;
}
std::vector<double> Hessian(CSolver& solver,const CGeometry& g,bool poison=false) {
  std::vector<double> data;for(size_t i=0;i<g.GetnPoint();++i)for(unsigned k=0;k<6;++k) {
    if(poison)solver.GetNodes()->GetHessian()(i,0,k)=42+.125*i+k;
    data.push_back(SU2_TYPE::GetValue(solver.GetNodes()->GetHessian(i,0,k)));
  }return data;
}
}

TEST_CASE("Original family replay recovers clipped demands without Hessian recovery or CFD metric publication", "[NativeMetricFamily3D]") {
  World world;auto driver=normalization_test::Driver(world,"original_family",world.size,false);auto backend=driver->Backend();
  auto& g=driver->Geometry();auto& solver=driver->Solver();const auto hessian=normalization_test::Hessian(solver,g,true);
  std::vector<su2double> raw(6*g.GetnPointDomain());
  for(size_t i=0;i<g.GetnPointDomain();++i) {
    const auto* p=g.nodes->GetCoord(i);raw[6*i]=std::exp(16*(2*SU2_TYPE::GetValue(p[0])-1));
    raw[6*i+3]=2+.25*p[1];raw[6*i+5]=3+p[2];
  }
  {Mute mute;solver.ComputeMetric(&g,&driver->Config(),&raw,true,nullptr,driver->Reference().get());}
  const auto family=solver.GetNativeMetricFamily3D();REQUIRE(family);const auto original_frames=normalization_test::Raw(*family);
  REQUIRE(family->frames.size()==g.GetnPointDomain());CHECK(normalization_test::Hessian(solver,g)==hessian);
  const auto before=State(g,solver);const auto before_turb=State(g,*driver->Turbulence());
  const auto input=ImportGeometry(driver->Config(),g,&solver.GetNodes()->GetMetric(),*driver->Reference());
  const auto source_bytes=Bytes(input.original);DonorImport source(world,true);std::string reason;REQUIRE(source.Build(input.original,reason));
  Owned fine;const auto mesh=simplex_test::MakeSimplexMesh(3,4,[](const passivedouble*){return "far";});
  for(size_t i=0;i<mesh.GetnElem();++i)if(i%world.size==unsigned(world.rank)) {
    OwnedCell row;row.cell.id=i;
    for(unsigned k=0;k<4;++k){const auto n=mesh.elem[4*i+k];row.cell.v[k]={n,{mesh.coord[3*n],mesh.coord[3*n+1],mesh.coord[3*n+2]}};}
    if(Orientation(row.cell.v[0].p,row.cell.v[1].p,row.cell.v[2].p,row.cell.v[3].p)<0)std::swap(row.cell.v[0],row.cell.v[1]);fine.emplace(i,row);
  }
  ComplexityReassessment3D first;BridgeStats stats;std::shared_ptr<const PreparedComplexity3D> cloud;
  const auto ok=ReassessComplexity(world,fine,source,*driver->Reference()->original,{},first,reason,stats,size_t(128)<<20,0,8192,20261017,{},&cloud);
  INFO(reason);REQUIRE(ok);REQUIRE(cloud);REQUIRE(first.estimated);
  CNativeCandidateComplexity3D assess(*family,*cloud,input.original,size_t(128)<<20);
  su2double core=family->core;const auto reference=assess.Assess(family->selected,core);
  CHECK(reference.value==Approx(first.value).epsilon(2e-12));const auto searches=source.Statistics().point_queries;
  CNativeMetricEvaluation3D evaluation;evaluation.family=family;
  evaluation.assess=[&](const std::vector<CBoundaryLayerMetric::Tensor>& current,su2double& c){return assess.Assess(current,c);};
  const auto replayed=solver.RecomputeMetricFamily3D(&g,&driver->Config(),evaluation,driver->Reference().get(),reason);
  INFO(reason);REQUIRE(replayed);REQUIRE(evaluation.normalized.size()==g.GetnPoint());
  CHECK(State(g,solver)==before);CHECK(State(g,*driver->Turbulence())==before_turb);
  CHECK(normalization_test::Hessian(solver,g)==hessian);CHECK(normalization_test::Raw(*family)==original_frames);
  for(size_t i=0;i<g.GetnPoint();++i) { CBoundaryLayerMetric::Tensor m;solver.GetNodes()->GetMetricMat(i,m.m);
    for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b)CHECK(m.m[a][b]==family->selected[i].m[a][b]); }
  CHECK(source.Statistics().point_queries==searches);CHECK(assess.stats.geometry_queries==0);CHECK(Bytes(input.original)==source_bytes);
  double local_nonhomogeneous=0;const double ratio=std::exp(evaluation.normalization.logScale-family->normalization.logScale);
  auto beta=input.original;
  for(auto& d:beta)for(unsigned k=0;k<4;++k) {
    const auto index=family->LocalNode(d.cell.v[k].id,assess.source_span);const auto& m=evaluation.normalized.at(index);
    const Tensor corrected{m.m[0][0],m.m[0][1],m.m[0][2],m.m[1][1],m.m[1][2],m.m[2][2]};
    local_nonhomogeneous=std::max(local_nonhomogeneous,std::abs(corrected.xx-ratio*d.sensor[k].xx));d.sensor[k]=corrected;
  }
  CHECK(CPassiveComm::Allreduce(local_nonhomogeneous,CPassiveComm::Op::MAX)>1e-3);
  const auto restored=assess.Assess(family->selected,core);CHECK(restored.value==reference.value);CHECK(restored.uncertainty==reference.uncertainty);
  Save(world,"original_family_beta",input.cells,beta,*driver->Reference());
  if(!world.rank) {std::ofstream out("original_family_result.json");out<<std::setprecision(17)<<"{\"initial_mass\":"<<first.value<<",\"initial_indicator\":"<<first.uncertainty
    <<",\"corrected_mass\":"<<evaluation.normalization.estimate.value<<",\"corrected_indicator\":"<<evaluation.normalization.estimate.uncertainty
    <<",\"scale_before\":"<<family->normalization.logScale<<",\"scale_after\":"<<evaluation.normalization.logScale<<",\"trials\":"<<evaluation.normalization.trials
    <<",\"source_searches_during_trials\":0,\"geometry_queries_during_trials\":0,\"raw_frames_and_Hessian_unchanged\":true,\"CFD_metric_restored\":true}\n";REQUIRE(bool(out));}
  auto invalid=evaluation;invalid.assess=world.rank==0?decltype(invalid.assess){}:evaluation.assess;
  CHECK_FALSE(solver.RecomputeMetricFamily3D(&g,&driver->Config(),invalid,driver->Reference().get(),reason));CHECK(State(g,solver)==before);
  auto bad=family->selected;if(!world.rank)bad.clear();CHECK_THROWS(assess.Assess(bad,core));
  auto mutableFamily=std::const_pointer_cast<CNativeMetricFamily3D>(family);
  const auto normalization=mutableFamily->normalization;
  mutableFamily->normalization.status=SU2Complexity::Status::IntegrationLimited;
  mutableFamily->normalization.logScale=(family->lower+family->upper)/2;
  auto predicted=solver.GetNodes()->GetMetric();if(!world.rank && predicted.rows())predicted(0,0)*=1.01;
  CHECK_FALSE(NativeNormalizationStage3D(solver,g,driver->Config(),*driver->Reference(),predicted).propose);
  const auto job=NativeNormalizationStage3D(solver,g,driver->Config(),*driver->Reference(),solver.GetNodes()->GetMetric());
  REQUIRE(bool(job.propose));ComplexityReassessment3D material=first;
  material.value=2*family->controls[4];material.uncertainty=0;
  CHECK_THROWS(job.propose(material,*cloud,input.original,family->core,[&](){return world.rank!=0;},size_t(128)<<20,0));
  mutableFamily->normalization=normalization;
  CHECK(State(g,solver)==before);CHECK(normalization_test::Hessian(solver,g)==hessian);
  {Mute mute;driver->Finalize();driver.reset();}
}

TEST_CASE("Original collar aliases use the physical source span with empty publishers", "[NativeMetricFamily3D]") {
  World world;CNativeMetricFamily3D family;family.node_span=6;family.selected.resize(6);
  for(size_t i=0;i<6;++i) {
    family.nodes.push_back({i,{},i});auto& m=family.selected[i];m.m[0][0]=i<4?1+i:1000;m.m[1][1]=m.m[2][2]=1;
  }
  std::vector<DonorCell> source;PreparedComplexity3D cloud;cloud.count=32;
  if(!world.rank) {
    DonorCell physical;physical.cell.id=7;
    const Point points[]={{0,0,0},{1,0,0},{0,1,0},{0,0,1}};
    for(unsigned k=0;k<4;++k){physical.cell.v[k]={k,points[k]};physical.sensor[k]={double(1+k),0,0,1,0,1};}
    source.push_back(physical);auto extension=physical;extension.cell.id=8;extension.extension_component=0;
    for(auto& v:extension.cell.v){v.id+=4;v.p.z+=2;}source.push_back(extension);
    for(unsigned i=0;i<32;++i) {
      OriginalComplexityPoint3D p;p.row=i;p.point={.25,.25,2.25};p.inverse_pdf=1;p.cell=8;p.extension_component=0;p.publisher=0;
      p.nodes={{4,5,6,7}};p.weights.fill(.25L);cloud.points.push_back(p);
    }
  }
  CNativeCandidateComplexity3D assess(family,cloud,source,size_t(16)<<20);CHECK(assess.source_span==4);
  su2double core=1;const auto result=assess.Assess(family.selected,core);CHECK(result.value==Approx(std::sqrt(2.5)).epsilon(1e-14));
  auto changed=family.selected;for(unsigned i=0;i<4;++i)changed[i].m[0][0]*=2;
  CHECK(assess.Assess(changed,core).value==Approx(std::sqrt(5.)).epsilon(1e-14));
  CHECK(assess.Assess(family.selected,core).value==result.value);
  auto bad=cloud;if(!world.rank)bad.points[0].nodes[0]=42;
  CHECK_THROWS(CNativeCandidateComplexity3D(family,bad,source,size_t(16)<<20));
  CHECK_THROWS(CNativeCandidateComplexity3D(family,cloud,source,0));
}

TEST_CASE("Bounded frozen normalization stages preserve one original source and transfer RANS histories once", "[NativeNormalizationRuntime3D]") {
  World world;const int workers=GENERATE(1,2,4);if(workers>world.size)return;
  const auto* only=std::getenv("SU2_NATIVE_NORMALIZATION_WORKERS");if(only && workers!=std::stoi(only))return;
  const double alpha=GENERATE(1.,10.);const auto name="normalization_m"+std::to_string(workers)+"_a"+std::to_string(int(alpha));
  auto driver=normalization_test::Driver(world,name,workers);auto backend=driver->Backend();auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  auto* flow=driver->Solver().GetNodes();auto* turb=driver->Turbulence()->GetNodes();
  const double rho=SU2_TYPE::GetValue(flow->GetSolution(0,0)),momentum=SU2_TYPE::GetValue(flow->GetSolution(0,1));
  const double kinetic=momentum*momentum/(2*rho),internal=SU2_TYPE::GetValue(flow->GetSolution(0,4))-kinetic;
  const double viscosity=SU2_TYPE::GetValue(turb->GetSolution(0,0));REQUIRE(viscosity>0);
  for(size_t p=0;p<driver->Geometry().GetnPoint();++p) {
    const double z=SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p,2));const double values[]={rho,momentum*z,0,0,internal+kinetic*z};
    for(unsigned k=0;k<5;++k){flow->SetSolution(p,k,values[k]);flow->GetSolution_time_n()(p,k)=values[k];flow->GetSolution_time_n1()(p,k)=values[k]*.96875;}
    turb->SetSolution(p,0,viscosity*z);turb->GetSolution_time_n()(p,0)=viscosity*z;turb->GetSolution_time_n1()(p,0)=viscosity*z*.9375;
  }
  const auto hessian=normalization_test::Hessian(driver->Solver(),driver->Geometry(),true);
  std::vector<su2double> raw(6*driver->Geometry().GetnPointDomain());for(size_t p=0;p<raw.size()/6;++p)for(unsigned k:{0u,3u,5u})raw[6*p+k]=1;
  {Mute mute;driver->Solver().ComputeMetric(&driver->Geometry(),&driver->Config(),&raw,true,nullptr,driver->Reference().get());}
  auto family=std::const_pointer_cast<CNativeMetricFamily3D>(driver->Solver().GetNativeMetricFamily3D());REQUIRE(family);
  const auto original_frames=normalization_test::Raw(*family);
  // Controlled initialization error: retain the actual unclipped family, but inject an inaccurate provisional scale.
  family->normalization.logScale=std::log(alpha);family->normalization.status=SU2Complexity::Status::IntegrationLimited;
  family->normalization.estimate={20,1};
  for(size_t p=0;p<family->selected.size();++p) {
    family->selected[p]={};for(unsigned k=0;k<3;++k)family->selected[p].m[k][k]=alpha;
    flow->SetMetricMat(p,family->selected[p].m);
  }
  const auto before=State(driver->Geometry(),driver->Solver()),before_turb=State(driver->Geometry(),*driver->Turbulence());
  const auto old=Flow(world,driver->Geometry(),driver->Solver());const auto authority=EncodeReference(*driver->Reference());
  const auto input=ImportGeometry(driver->Config(),driver->Geometry(),&flow->GetMetric(),*driver->Reference());
  CConservativeTransfer transfer;const auto result=backend->Remesh(driver->Config(),driver->Geometry(),flow->GetMetric());
  INFO(native->Statistics().normalization.reason);REQUIRE(result.status==CRemeshResult::Status::COMPLETE);
  const auto stats=native->Statistics();CHECK(stats.normalization_stages==2);CHECK(stats.normalization.changed);
  CHECK(stats.cumulative_work<=8192);CHECK(stats.normalization_stage_work[0]+stats.normalization_stage_work[1]==stats.cumulative_work);CHECK(stats.normalization.trials<=24);CHECK(CPassiveComm::Allreduce(stats.normalization.geometry_queries,CPassiveComm::Op::SUM)>0);
  CHECK(State(driver->Geometry(),driver->Solver())==before);CHECK(State(driver->Geometry(),*driver->Turbulence())==before_turb);
  CHECK(normalization_test::Hessian(driver->Solver(),driver->Geometry())==hessian);CHECK(normalization_test::Raw(*family)==original_frames);
  CHECK(EncodeReference(*driver->Reference())==authority);
  const double theta=std::clamp(std::exp(stats.normalization.log_scale),family->controls[1]>0?1/(family->controls[1]*family->controls[1]):0.,1/(family->controls[0]*family->controls[0]));
  auto beta=input.original;for(auto& d:beta)d.sensor.fill({theta,0,0,theta,0,theta});
  if(alpha==1 && stats.normalization_stage_work[1]>0 && stats.normalization_stage_work[0]>0) {
    auto options=normalization_test::Options(name,workers);const auto at=options.find("ADAP_NATIVE_MAX_WORK= 8192");REQUIRE(at!=std::string::npos);
    options.replace(at,std::string("ADAP_NATIVE_MAX_WORK= 8192").size(),"ADAP_NATIVE_MAX_WORK= "+std::to_string(stats.normalization_stage_work[0]));
    std::stringstream text(options);std::unique_ptr<CConfig> limited;
    {Mute mute;limited=std::make_unique<CConfig>(text,SU2_COMPONENT::SU2_CFD,false);}
    // A fresh parsed config has no mesh-bound marker order, including MPI send/receive markers.
    REQUIRE(limited->GetnMarker_Max()>=driver->Geometry().GetnMarker());
    limited->SetnMarker_All(driver->Geometry().GetnMarker());
    for(unsigned m=0;m<driver->Geometry().GetnMarker();++m) {
      limited->SetMarker_All_TagBound(m,driver->Config().GetMarker_All_TagBound(m));
      limited->SetMarker_All_KindBC(m,driver->Config().GetMarker_All_KindBC(m));
      limited->SetMarker_All_SendRecv(m,driver->Config().GetMarker_All_SendRecv(m));
    }
    const auto rejected=backend->Remesh(*limited,driver->Geometry(),flow->GetMetric());
    CHECK(rejected.status!=CRemeshResult::Status::COMPLETE);CHECK_FALSE(rejected.onAccepted);
    CHECK(native->Statistics().cumulative_work<=stats.normalization_stage_work[0]);
    CHECK(native->Statistics().normalization_stage_work[0]>0);
    CHECK(State(driver->Geometry(),driver->Solver())==before);CHECK(State(driver->Geometry(),*driver->Turbulence())==before_turb);
    CHECK(EncodeReference(*driver->Reference())==authority);
  }
  driver->ReplaceMesh(result,transfer);const auto transported=result.slices.FetchPointMetric(driver->Geometry());
  const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);SaveStates(world,name,old,fresh,1.);SaveProjection(world,name,transfer);CheckPositiveFlow(fresh);
  su2activematrix matrix(driver->Geometry().GetnPointDomain(),6);
  for(size_t p=0;p<matrix.rows();++p)for(unsigned k=0;k<6;++k){matrix(p,k)=transported[6*p+k];CHECK(matrix(p,k)==Approx(k==0||k==3||k==5?theta:0.).margin(2e-13));}
  auto reference=*driver->Reference();const auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&matrix,reference);
  Save(world,name,accepted.cells,beta,reference);
  OutputRoundTrip(world,name,result.slices,accepted.cells,reference,theta,[](const su2double*,double s){return Tensor{s,0,0,s,0,s};},&beta);
  CHECK(EncodeReference(*driver->Reference())==authority);
  for(const auto& row:fresh)for(unsigned k=0;k<5;++k) {
    const double expected=k==0?rho:k==1?momentum*row.point.z:k==4?internal+kinetic*row.point.z:0;
    CHECK(RelDiff(row.values[k],expected)<2e-10);CHECK(RelDiff(row.values[5+k],expected)<2e-10);
    CHECK(RelDiff(row.values[10+k],expected*.96875)<2e-10);
  }
  turb=driver->Turbulence()->GetNodes();
  for(size_t p=0;p<driver->Geometry().GetnPointDomain();++p) {
    const double expected=viscosity*SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p,2));
    const double lower=SU2_TYPE::GetValue(driver->Turbulence()->GetLowerLimit(0));
    CHECK(turb->GetSolution(p,0)>=lower);CHECK(RelDiff(turb->GetSolution(p,0),std::max(lower,expected),viscosity)<2e-10);
    CHECK(RelDiff(turb->GetSolution_time_n(p,0),std::max(lower,expected),viscosity)<2e-10);
    CHECK(RelDiff(turb->GetSolution_time_n1(p,0),std::max(lower,expected*.9375),viscosity*.9375)<2e-10);
  }
  driver->Config().SetTimeIter(2);driver->WriteTimeHistoryRestarts();
#ifdef HAVE_CGNS
  const std::string restartMesh=name+"_written_CGNS.cgns",format="CGNS";
#else
  const std::string restartMesh=name+"_written_SU2.su2",format="SU2";
#endif
  WriteReference(restartMesh+".native_ref",reference);
  if(!world.rank) {
    auto text=normalization_test::Options(name,workers);const auto at=text.find("MESH_FILENAME= "+name+".su2");REQUIRE(at!=std::string::npos);
    text.replace(at,std::string("MESH_FILENAME= "+name+".su2").size(),"MESH_FILENAME= "+restartMesh);
    const auto f=text.find("MESH_FORMAT= SU2");text.replace(f,std::string("MESH_FORMAT= SU2").size(),"MESH_FORMAT= "+format);
    std::ofstream cfg(name+"_reload.cfg");cfg<<text<<"RESTART_SOL= YES\nRESTART_ITER= 3\n";
  }
  backend.reset();{Mute mute;driver->Finalize();driver.reset();}SU2_MPI::Barrier(world.comm);
  const auto restartCfg=name+"_reload.cfg";driver=std::make_unique<TetraDriver>(const_cast<char*>(restartCfg.c_str()),1,world.comm);
  driver->Preprocess(3);auto reloadedBackend=driver->Backend();
  SaveStates(world,name+"_restart",fresh,Flow(world,driver->Geometry(),driver->Solver()),theta);
  CHECK(EncodeReference(*driver->Reference())==authority);
  if(!world.rank){std::ofstream out(name+"_normalization.json");out<<std::setprecision(17)<<"{\"controlled_initialization_error\":true,\"initial_scale\":"<<alpha
    <<",\"final_sensor_eigenvalue\":"<<theta<<",\"stages\":"<<stats.normalization_stages<<",\"work\":"<<stats.cumulative_work
    <<",\"root_trials\":"<<stats.normalization.trials<<",\"mass\":"<<result.complexity.value<<",\"indicator\":"<<result.complexity.uncertainty
    <<",\"normalization_seconds\":"<<stats.phases[14]<<",\"original_raw_frames_unchanged\":true,\"one_final_transfer\":true}\n";REQUIRE(bool(out));}
  {Mute mute;driver->Finalize();driver.reset();}
}

TEST_CASE("Opt-in BL budget selection stays frozen through native RANS replacement", "[NativeBLBudgetRuntime3D]") {
  World world;const int workers=std::min(2,world.size);
  const double budget=GENERATE(3.,20.);
  const auto name="bl_budget_"+std::to_string(int(budget))+"_m"+std::to_string(workers);
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(3,1,[](const passivedouble* p){return p[2]==0?"wall":"far";}),name+".su2");
    auto text=normalization_test::Options(name,workers);const std::string old="ADAP_COMPLEXITY= 20";
    const auto at=text.find(old);REQUIRE(at!=std::string::npos);text.replace(at,old.size(),"ADAP_COMPLEXITY= "+std::to_string(int(budget)));
    std::ofstream cfg(name+".cfg");cfg<<text<<"ADAP_BL_BUDGET= YES\nADAP_BL_PROTECT_DISTANCE= .1\nADAP_BL_OUTER_GROWTH_MAX= 10\n";REQUIRE(bool(cfg));
  }
  SU2_MPI::Barrier(world.comm);auto cfg=name+".cfg";auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  auto backend=driver->Backend();auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  auto& g=driver->Geometry();auto& solver=driver->Solver();const auto before=State(g,solver),beforeTurb=State(g,*driver->Turbulence());
  std::vector<su2double> raw(6*g.GetnPointDomain());for(size_t p=0;p<raw.size()/6;++p)for(unsigned k:{0u,3u,5u})raw[6*p+k]=1;
  solver.ComputeMetric(&g,&driver->Config(),&raw,false,nullptr,driver->Reference().get());
  REQUIRE(solver.GetNativeMetricFamily3D());CHECK_FALSE(solver.GetNativeMetricFamily3D()->layers);
  solver.ComputeMetric(&g,&driver->Config(),&raw,true,nullptr,driver->Reference().get());
  const auto family=solver.GetNativeMetricFamily3D();REQUIRE(family);REQUIRE(family->layers);
  const auto selected=family->layers->GetOuterNormal();CHECK(selected.frozen);CHECK(selected.parameter>=0);CHECK(selected.parameter<=1);
  CHECK(family->budget_selection.refinements<=2);CHECK(family->budget_selection.trials<=9*(1+family->budget_selection.refinements));
  if(budget==20)CHECK(selected.parameter==0);
  if(budget==3)CHECK(selected.parameter>0);
  CHECK(CPassiveComm::Allreduce(selected.parameter,CPassiveComm::Op::MIN)==CPassiveComm::Allreduce(selected.parameter,CPassiveComm::Op::MAX));
  CHECK(CPassiveComm::Allreduce(selected.version,CPassiveComm::Op::MIN)==CPassiveComm::Allreduce(selected.version,CPassiveComm::Op::MAX));
  CHECK_THROWS(family->layers->SelectOuterNormal(0));
  const auto frames=normalization_test::Raw(*family);
  const auto original=ImportGeometry(driver->Config(),g,&solver.GetNodes()->GetMetric(),*driver->Reference());
  const auto oldFlow=Flow(world,g,solver);
  const auto result=backend->Remesh(driver->Config(),g,solver.GetNodes()->GetMetric());
  INFO(native->Statistics().normalization.reason);REQUIRE(result.status==CRemeshResult::Status::COMPLETE);
  CHECK(State(g,solver)==before);CHECK(State(g,*driver->Turbulence())==beforeTurb);
  CHECK(normalization_test::Raw(*family)==frames);CHECK(family->layers->GetOuterNormal().version==selected.version);
  const auto authorityBefore=EncodeReference(*driver->Reference());
  std::ifstream savedConfig(name+".cfg");std::stringstream configText;configText<<savedConfig.rdbuf();auto text=configText.str();
  const std::string work="ADAP_NATIVE_MAX_WORK= 8192";const auto at=text.find(work);REQUIRE(at!=std::string::npos);
  text.replace(at,work.size(),"ADAP_NATIVE_MAX_WORK= 1");std::stringstream limitedText(text);std::unique_ptr<CConfig> limited;
  {Mute mute;limited=std::make_unique<CConfig>(limitedText,SU2_COMPONENT::SU2_CFD,false);}
  REQUIRE(limited->GetnMarker_Max()>=g.GetnMarker());limited->SetnMarker_All(g.GetnMarker());
  for(unsigned m=0;m<g.GetnMarker();++m) {
    limited->SetMarker_All_TagBound(m,driver->Config().GetMarker_All_TagBound(m));
    limited->SetMarker_All_KindBC(m,driver->Config().GetMarker_All_KindBC(m));
    limited->SetMarker_All_SendRecv(m,driver->Config().GetMarker_All_SendRecv(m));
  }
  const auto refused=backend->Remesh(*limited,g,solver.GetNodes()->GetMetric());
  CHECK(refused.status!=CRemeshResult::Status::COMPLETE);CHECK_FALSE(refused.onAccepted);
  CHECK(State(g,solver)==before);CHECK(State(g,*driver->Turbulence())==beforeTurb);
  CHECK(EncodeReference(*driver->Reference())==authorityBefore);CHECK(family->layers->GetOuterNormal().version==selected.version);
  CConservativeTransfer transfer;driver->ReplaceMesh(result,transfer);
  const auto transported=result.slices.FetchPointMetric(driver->Geometry());su2activematrix sensor(driver->Geometry().GetnPointDomain(),6);
  for(size_t p=0;p<sensor.rows();++p)for(unsigned k=0;k<6;++k)sensor(p,k)=transported[6*p+k];
  auto reference=*driver->Reference();const auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&sensor,reference);
  double local_theta=0;for(size_t p=0;p<sensor.rows();++p)local_theta=std::max(local_theta,SU2_TYPE::GetValue(sensor(p,0)));
  const auto theta=CPassiveComm::Allreduce(local_theta,CPassiveComm::Op::MAX);
  const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);
  SaveStates(world,name,oldFlow,fresh,theta);SaveProjection(world,name,transfer);CheckPositiveFlow(fresh);
  for(size_t p=0;p<sensor.rows();++p)for(unsigned k=0;k<6;++k)CHECK(sensor(p,k)==Approx(k==0||k==3||k==5?theta:0.).margin(2e-12));
  auto frozen=original.original;for(auto& d:frozen)d.sensor.fill({theta,0,0,theta,0,theta});
  const auto authority=EncodeReference(reference);Save(world,name,accepted.cells,frozen,reference);
  OutputRoundTrip(world,name,result.slices,accepted.cells,reference,theta,
      [](const su2double*,double t){return Tensor{t,0,0,t,0,t};},&frozen);
  {Mute mute;CMeshOutput::WriteMesh(&driver->Config(),&driver->Geometry(),name+"_accepted");}
  CHECK(EncodeReference(*driver->Reference())==authority);
  if(!world.rank) {
    std::ofstream out(name+"_profile.json");out<<std::setprecision(17)<<"{\"budget\":"<<budget<<",\"parameter\":"<<selected.parameter
      <<",\"version\":"<<selected.version<<",\"trials\":"<<family->budget_selection.trials<<",\"workers\":"<<workers
      <<",\"accepted\":true,\"mass\":"<<result.complexity.value<<",\"indicator\":"<<result.complexity.uncertainty
      <<",\"final_sensor_eigenvalue\":"<<theta<<",\"frozen\":true,\"support_changed\":false,\"proven_bound\":false}\n";REQUIRE(bool(out));
  }
  driver->Config().SetTimeIter(2);driver->WriteTimeHistoryRestarts();
#ifdef HAVE_CGNS
  const std::string restartMesh=name+"_written_CGNS.cgns",format="CGNS";
#else
  const std::string restartMesh=name+"_written_SU2.su2",format="SU2";
#endif
  WriteReference(restartMesh+".native_ref",reference);
  if(!world.rank) {
    auto reload=configText.str();const auto mesh=reload.find("MESH_FILENAME= "+name+".su2");REQUIRE(mesh!=std::string::npos);
    reload.replace(mesh,std::string("MESH_FILENAME= "+name+".su2").size(),"MESH_FILENAME= "+restartMesh);
    const auto type=reload.find("MESH_FORMAT= SU2");REQUIRE(type!=std::string::npos);
    reload.replace(type,std::string("MESH_FORMAT= SU2").size(),"MESH_FORMAT= "+format);
    std::ofstream cfg(name+"_reload.cfg");cfg<<reload<<"RESTART_SOL= YES\nRESTART_ITER= 3\n";REQUIRE(bool(cfg));
  }
  backend.reset();{Mute mute;driver->Finalize();driver.reset();}SU2_MPI::Barrier(world.comm);
  const auto reload=name+"_reload.cfg";driver=std::make_unique<TetraDriver>(const_cast<char*>(reload.c_str()),1,world.comm);
  driver->Preprocess(3);auto reloadedBackend=driver->Backend();REQUIRE(driver->Reference());
  SaveStates(world,name+"_restart",fresh,Flow(world,driver->Geometry(),driver->Solver()),theta);
  CHECK(EncodeReference(*driver->Reference())==authority);
  {Mute mute;driver->Finalize();driver.reset();}
}


TEST_CASE("An uncertain BL budget freezes the coarsest original field without scale trials", "[NativeBLBudgetUncertainty3D]") {
  World world;const std::string name="budget_uncertainty";
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(3,1,[](const passivedouble* p){return p[2]==0?"wall":"far";}),name+".su2");
    auto text=normalization_test::Options(name,std::min(2,world.size));const std::string old="ADAP_COMPLEXITY= 20";
    const auto at=text.find(old);REQUIRE(at!=std::string::npos);text.replace(at,old.size(),"ADAP_COMPLEXITY= 4");
    // Independent one-wall integral is 4 at this coarsest sensor spacing; ADAP_COMPLEXITY is integral-valued.
    const std::string maximum="ADAP_HMAX= 1.0000000000000002";const auto h=text.find(maximum);REQUIRE(h!=std::string::npos);
    text.replace(h,maximum.size(),"ADAP_HMAX= 0.9452825167897142");
    std::ofstream cfg(name+".cfg");cfg<<text<<"ADAP_BL_BUDGET= YES\nADAP_BL_PROTECT_DISTANCE= .1\nADAP_BL_OUTER_GROWTH_MAX= 10\n";REQUIRE(bool(cfg));
  }
  SU2_MPI::Barrier(world.comm);auto cfg=name+".cfg";auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  auto& geometry=driver->Geometry();auto& solver=driver->Solver();const auto before=State(geometry,solver);
  std::vector<su2double> raw(6*geometry.GetnPointDomain());for(size_t p=0;p<raw.size()/6;++p)for(unsigned k:{0u,3u,5u})raw[6*p+k]=1;
  solver.ComputeMetric(&geometry,&driver->Config(),&raw,true,nullptr,driver->Reference().get());
  const auto family=solver.GetNativeMetricFamily3D();REQUIRE(family);REQUIRE(family->layers);
  CHECK(family->budget_selection.status==SU2Complexity::BudgetStatus::IntegrationLimited);
  CHECK(family->normalization.status==SU2Complexity::Status::IntegrationLimited);
  CHECK(family->normalization.logScale==family->lower);CHECK(family->normalization.trials==1);
  CHECK(family->layers->GetOuterNormal().frozen);CHECK(family->layers->GetOuterNormal().parameter==0);
  CHECK(State(geometry,solver)==before);
  {Mute mute;driver->Finalize();driver.reset();}
}
