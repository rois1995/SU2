// Original frames and original-P1 stencils, never an adapted-node replacement field.
namespace normalization_test {
std::string Options(const std::string& name,int workers,bool walls=true,bool firstRow=false) {
  std::ostringstream s;s<<"SOLVER= RANS\nKIND_TURB_MODEL= SA\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= .2\nREYNOLDS_NUMBER= 1e6\nREYNOLDS_LENGTH= 1\n"
    <<"MESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\nMARKER_FAR= (far)\nMARKER_HEATFLUX= (wall,0)\n"
    <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\nADAP_ISO_CORNER= NO\n"
    <<"ADAP_HMIN= .1\nADAP_HMAX= 1.0000000000000002\nADAP_ARMAX= 100\nADAP_COMPLEXITY= 20\nADAP_HGRAD= 1.3\n"
    <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nADAP_NATIVE_MAX_WORK= 8192\nWRT_PERFORMANCE= YES\n"
    <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
    <<"TIME_DOMAIN= YES\nTIME_STEP= .001\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
    <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nRESTART_FILENAME= "<<name<<"_restart\nSOLUTION_FILENAME= "<<name<<"_restart\n";
  if(walls)s<<"ADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (.1)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.3)\n";
  if(firstRow)s<<"ADAP_BL_METHOD= FIRST_ROW\n";
  return s.str();
}
std::unique_ptr<TetraDriver> Driver(World& world,const std::string& name,int workers,bool walls=true,bool firstRow=false) {
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(3,1,[](const passivedouble* p){return p[2]==0?"wall":"far";}),name+".su2");
    std::ofstream out(name+".cfg");out<<Options(name,workers,walls,firstRow);
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

namespace {
CSimplexMesh SymmetryAnnulusMesh() {
  CSimplexMesh mesh;mesh.nDim=3;
  const std::array<Point,8> base{{{0,0,0},{1,0,0},{1,0,1},{0,0,1},
      {.4,0,.4},{.6,0,.4},{.6,0,.6},{.4,0,.6}}};
  for(double y:{0.,1.})for(auto p:base)mesh.coord.insert(mesh.coord.end(),{p.x,y,p.z});
  auto point=[&](unsigned long id){return Point{mesh.coord[3*id],mesh.coord[3*id+1],mesh.coord[3*id+2]};};
  std::map<Face,unsigned> incidence;
  for(unsigned i=0;i<4;++i)for(auto triangle:std::array<Face,2>{{{i,(i+1)%4,4+(i+1)%4},{i,4+(i+1)%4,4+i}}}) {
    std::sort(triangle.begin(),triangle.end());const auto a=triangle[0],b=triangle[1],c=triangle[2];
    for(auto tet:std::array<std::array<Id,4>,3>{{{a,b,c,c+8},{a,b,b+8,c+8},{a,a+8,b+8,c+8}}}) {
      const auto sign=Orientation(point(tet[0]),point(tet[1]),point(tet[2]),point(tet[3]));REQUIRE(sign!=0);
      if(sign<0)std::swap(tet[0],tet[1]);mesh.elem.insert(mesh.elem.end(),tet.begin(),tet.end());
      for(unsigned omit=0;omit<4;++omit){Face f{};unsigned k=0;for(unsigned j=0;j<4;++j)if(j!=omit)f[k++]=tet[j];std::sort(f.begin(),f.end());++incidence[f];}
    }
  }
  mesh.elemRef.resize(mesh.GetnElem());std::map<std::string,std::vector<unsigned long>> boundary;
  for(const auto& row:incidence) {
    REQUIRE((row.second==1 || row.second==2));if(row.second==2)continue;
    bool wall=true,symmetry=true;for(auto id:row.first){wall=wall&&point(id).z==0;symmetry=symmetry&&point(id).y==0;}
    auto& f=boundary[wall?"wall":symmetry?"symmetry":"far"];f.insert(f.end(),row.first.begin(),row.first.end());
  }
  for(auto& row:boundary){CSimplexMesh::Marker marker;marker.name=row.first;marker.elem=std::move(row.second);mesh.markers.push_back(std::move(marker));}
  return mesh;
}
void ShearSymmetryMesh(CSimplexMesh& mesh) {
  std::set<unsigned long> boundary;
  for(const auto& marker:mesh.markers)if(marker.name=="symmetry")for(auto id:marker.elem)boundary.insert(id);
  for(size_t p=0;p<mesh.GetnPoint();++p) {
    mesh.coord[3*p+1]+=.37*mesh.coord[3*p]+.61*mesh.coord[3*p+2];
    if(boundary.count(p)) {
      const double error=(p%2?1:-1)*1e-12*(p+1);
      mesh.coord[3*p]-=.37*error;mesh.coord[3*p+1]+=error;mesh.coord[3*p+2]-=.61*error;
    }
  }
}
void AppendSymmetryMesh(CSimplexMesh& mesh,const CSimplexMesh& part) {
  const auto offset=mesh.GetnPoint();mesh.coord.insert(mesh.coord.end(),part.coord.begin(),part.coord.end());
  for(auto id:part.elem)mesh.elem.push_back(id+offset);mesh.elemRef.resize(mesh.GetnElem());
  for(const auto& marker:part.markers) {
    auto found=std::find_if(mesh.markers.begin(),mesh.markers.end(),[&](const auto& m){return m.name==marker.name;});REQUIRE(found!=mesh.markers.end());
    for(auto id:marker.elem)found->elem.push_back(id+offset);
  }
}
void BudgetRuntime3D(bool symmetry) {
  World world;const auto all=std::getenv("SU2_NATIVE_SYMMETRY_ALL_WORKERS");
  const int workers=symmetry && all && std::string(all)=="1"?world.size:std::min(2,world.size);
  const bool annulus=symmetry && std::getenv("SU2_NATIVE_SYMMETRY_ANNULUS");
  const bool oblique=annulus && std::getenv("SU2_NATIVE_SYMMETRY_OBLIQUE");
  const bool mapped=oblique && std::getenv("SU2_NATIVE_SYMMETRY_MAPPED");
  const double budget=GENERATE(3.,20.);if(annulus && budget!=20)return;
  const auto name=std::string(oblique?"affine_annulus_bl_budget_":annulus?"symmetry_annulus_bl_budget_":symmetry?"symmetry_bl_budget_":"bl_budget_")+std::to_string(int(budget))+"_m"+std::to_string(workers);
  if(!world.rank) {
    auto mesh=annulus?SymmetryAnnulusMesh():simplex_test::MakeSimplexMesh(3,1,[&](const passivedouble* p){return p[2]==0?"wall":symmetry&&p[1]==0?"symmetry":"far";});
    if(oblique)ShearSymmetryMesh(mesh); else if(symmetry)for(size_t p=0;p<mesh.GetnPoint();++p)if(mesh.coord[3*p+1]==0)mesh.coord[3*p+1]=(p%2?1:-1)*1e-12*(p+1);
    simplex_test::WriteSU2Mesh(mesh,name+".su2");
    if(symmetry) {
      std::ofstream ref(name+".planes");
      if(oblique)ref<<"SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" 17 1 0 0 0 -.37 1 -.61 1e-10 1e-13\n";
      else ref<<"SU2_NATIVE_SYMMETRY_PLANES 1 1\n\"symmetry\" Y 0 1e-10\n";
      REQUIRE(bool(ref));
    }
    auto text=normalization_test::Options(name,workers);const std::string old="ADAP_COMPLEXITY= 20";
    const auto at=text.find(old);REQUIRE(at!=std::string::npos);text.replace(at,old.size(),"ADAP_COMPLEXITY= "+std::to_string(int(budget)));
    if(symmetry)text+="MARKER_SYM= (symmetry)\nADAP_NATIVE_SYMMETRY_REFERENCE= "+name+".planes\n";
    if(oblique){std::ostringstream angle;angle<<std::setprecision(17)<<"SIDESLIP_ANGLE= "<<std::atan(.37)*180/std::acos(-1.)<<'\n';text+=angle.str();}
    std::ofstream cfg(name+".cfg");cfg<<text<<"ADAP_BL_BUDGET= YES\nADAP_BL_PROTECT_DISTANCE= .1\nADAP_BL_OUTER_GROWTH_MAX= 10\n";REQUIRE(bool(cfg));
    if(mapped){cfg<<"ADAP_NATIVE_MAX_TIME= 120\n";REQUIRE(bool(cfg));}
  }
  SU2_MPI::Barrier(world.comm);auto cfg=name+".cfg";auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  if(symmetry && !oblique) {
    SymmetryReferenceAudit3D report;std::string reason;
    REQUIRE(PrepareSymmetryReference(driver->Config(),driver->Geometry(),false,reason,&report));
    CHECK(report.changed_points==0);CHECK(report.maximum_deviation==0);CHECK(report.junction_points>0);
    auto& mesh=driver->Geometry();std::vector<double> before(mesh.GetnPoint());
    const uint64_t negativeZero=uint64_t(1)<<63;
    for(size_t p=0;p<mesh.GetnPoint();++p)if(mesh.nodes->GetCoord(p,1)==0)
      std::memcpy(mesh.nodes->GetCoord(p)+1,&negativeZero,sizeof(negativeZero));
    REQUIRE(PrepareSymmetryReference(driver->Config(),mesh,false,reason));
    for(size_t p=0;p<mesh.GetnPoint();++p)if(mesh.nodes->GetCoord(p,1)==0) {
      uint64_t bits=0;std::memcpy(&bits,mesh.nodes->GetCoord(p)+1,sizeof(bits));
      CHECK(bits==negativeZero);mesh.nodes->SetCoord(p,1,0.);
    }

    for(size_t p=0;p<mesh.GetnPoint();++p) {
      before[p]=SU2_TYPE::GetValue(mesh.nodes->GetCoord(p,1));
      if(before[p]==0)mesh.nodes->SetCoord(p,1,1e-9);
    }
    CHECK_FALSE(PrepareSymmetryReference(driver->Config(),mesh,false,reason));
    CHECK(reason.find("declared plane deviation")!=std::string::npos);
    for(size_t p=0;p<mesh.GetnPoint();++p) {
      CHECK(mesh.nodes->GetCoord(p,1)==(before[p]==0?1e-9:before[p]));
      if(before[p]==0)mesh.nodes->SetCoord(p,1,1e-12);
    }
    CHECK_FALSE(PrepareSymmetryReference(driver->Config(),mesh,false,reason));
    CHECK(reason.find("Existing state/returned mesh")!=std::string::npos);
    for(size_t p=0;p<mesh.GetnPoint();++p) {
      CHECK(mesh.nodes->GetCoord(p,1)==(before[p]==0?1e-12:before[p]));
      mesh.nodes->SetCoord(p,1,before[p]);
    }
    CHECK_FALSE(PrepareSymmetryReference(driver->Config(),mesh,false,reason,nullptr,world.rank==0?1:SIZE_MAX));
    CHECK(reason.find("storage budget")!=std::string::npos);
    auto declaration=[&](const std::string& value) {
      if(!world.rank){std::ofstream out(name+".planes");out<<value;REQUIRE(bool(out));}
      SU2_MPI::Barrier(world.comm);
    };
    for(const auto& bad:std::vector<std::string>{
        "SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" Y 0 1e-10\n",
        "SU2_NATIVE_SYMMETRY_PLANES 1 1\n\"wall\" Y 0 1e-10\n",
        "SU2_NATIVE_SYMMETRY_PLANES 1 1\n\"symmetry\" Q 0 1e-10\n",
        "SU2_NATIVE_SYMMETRY_PLANES 1 1\n\"symmetry\" Y 0 -1\n",
        "SU2_NATIVE_SYMMETRY_PLANES 1 2\n\"symmetry\" Y 0 1e-10\n\"symmetry\" Y 0 1e-10\n",
        "SU2_NATIVE_SYMMETRY_PLANES 1 1\n\"symmetry\" Y 0 1e-10\ntrailing"}) {
      declaration(bad);CHECK_FALSE(PrepareSymmetryReference(driver->Config(),mesh,false,reason));
    }
    declaration("SU2_NATIVE_SYMMETRY_PLANES 1 1\n\"symmetry\" Y 2 3\n");
    CHECK_FALSE(PrepareSymmetryReference(driver->Config(),mesh,true,reason));
    CHECK(reason.find("degenerates/reverses")!=std::string::npos);
    declaration("SU2_NATIVE_SYMMETRY_PLANES 1 1\n\"symmetry\" Y 0 1e-10\n");
    for(size_t p=0;p<mesh.GetnPoint();++p)CHECK(mesh.nodes->GetCoord(p,1)==before[p]);
    std::stringstream disabledText(normalization_test::Options(name,workers)+"ADAP_NATIVE_SYMMETRY_REFERENCE= NONE\n");
    std::unique_ptr<CConfig> disabled;{Mute mute;disabled=std::make_unique<CConfig>(disabledText,SU2_COMPONENT::SU2_CFD,false);}
    CHECK(PrepareSymmetryReference(*disabled,mesh,false,reason,nullptr,0));

  }
  if(oblique) {
    SymmetryReferenceAudit3D report;std::string reason;auto& geometry=driver->Geometry();
    const auto states=State(geometry,driver->Solver());std::vector<std::array<double,3>> coordinates;
    for(size_t p=0;p<geometry.GetnPoint();++p){const auto* x=geometry.nodes->GetCoord(p);coordinates.push_back({double(x[0]),double(x[1]),double(x[2])});}
    REQUIRE(PrepareSymmetryReference(driver->Config(),geometry,false,reason,&report));
    CHECK(report.changed_points==0);CHECK(report.maximum_mapping_error<=1e-13);CHECK(report.junction_points>0);
    REQUIRE(PrepareSymmetryReference(driver->Config(),geometry,true,reason,&report));CHECK(report.changed_points==0);
    CHECK_FALSE(PrepareSymmetryReference(driver->Config(),geometry,false,reason,nullptr,world.rank==0?1:SIZE_MAX));
    auto declaration=[&](const std::string& data){if(!world.rank){std::ofstream out(name+".planes");out<<data;REQUIRE(bool(out));}SU2_MPI::Barrier(world.comm);};
    for(const auto& invalid:std::vector<std::string>{
        "SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" 17 0 0 0 0 -.37 1 -.61 1e-10 1e-13\n",
        "SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" 17 1 0 0 0 -.37 1 -.61 1e-14 1e-13\n",
        "SU2_NATIVE_SYMMETRY_PLANES 2 2\n\"symmetry\" 17 1 0 0 0 -.37 1 -.61 1e-10 1e-13\n\"symmetry\" 18 1 0 0 0 -.37 1 -.61 1e-10 1e-13\n",
        "SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" 17 1 0 .1 0 -.37 1 -.61 1e-10 1e-13\n"}) {
      declaration(invalid);CHECK_FALSE(PrepareSymmetryReference(driver->Config(),geometry,false,reason));
    }
    declaration("SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" 17 1 0 0 0 -.37 1 -.61 1e-10 1e-13\n");
    for(size_t p=0;p<geometry.GetnPoint();++p)for(unsigned k=0;k<3;++k)CHECK(geometry.nodes->GetCoord(p,k)==coordinates[p][k]);
    CHECK(State(geometry,driver->Solver())==states);
  }
  auto backend=driver->Backend();auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  if(mapped) {
    // Geometry already adopted; declare a matching mapping band before original metric/donors freeze.
    REQUIRE(driver->Reference());auto& reference=*driver->Reference();REQUIRE(reference.original);
    std::map<Id,double> errors;for(auto f:reference.facets)errors.emplace(f.id,1e-13);
    auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(reference.original,errors,7,SIZE_MAX,std::getenv("SU2_NATIVE_MAPPED_COMPONENTS")!=nullptr);
    BoundaryPoints points;std::vector<SurfaceFace> skin;
    for(auto f:reference.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});}
    reference.original=PlanarReference::WithParameters(reference.original,std::make_shared<BoundaryParameterState>(authority,points,skin));
    const auto check=ImportGeometry(driver->Config(),driver->Geometry(),nullptr,reference);
    CHECK(CPassiveComm::Allreduce(uint64_t(check.cells.size()),CPassiveComm::Op::SUM,world.comm)==driver->Geometry().GetGlobal_nElemDomain());
  }
  if(annulus) {
    struct LastTrial {Patch old,fresh;Feature feature;MetricDiagnosis metric;bool candidate=false;std::string reason;};
    auto last=std::make_shared<LastTrial>();const auto reference=driver->Reference();
    const auto prefix=name+"_rejected_rank"+std::to_string(world.rank);
    // Optional complete rejected-trial log (DC-5 production re-run): one JSON line per rejected engine trial.
    std::shared_ptr<std::ofstream> trials;
    if(const char* path=std::getenv("SU2_NATIVE_TRIAL_LOG"))
      trials=std::make_shared<std::ofstream>(std::string(path)+".rank"+std::to_string(world.rank)+".jsonl");
    native->SetRejectedTrialObserver([last,trials](const Patch& old,const Patch& fresh,Feature feature,const MetricDiagnosis& metric,bool candidate,const std::string& reason) {
      if(old.cells.size()>MaximumReplacementCells || fresh.cells.size()>MaximumReplacementCells)
        throw std::runtime_error("Controlled rejected trial exceeds its diagnostic storage bound.");
      *last={old,fresh,feature,metric,candidate,reason};
      if(trials) {
        std::string text;for(char c:reason){if(c=='"'||c=='\\')text+='\\';text+=(c=='\n'?' ':c);}
        *trials<<std::setprecision(17)<<"{\"candidate\":"<<candidate<<",\"failure\":"<<int(metric.failure)
          <<",\"shape_measured\":"<<(metric.shape_measured?"true":"false")<<",\"q\":";
        if(metric.shape_measured)*trials<<metric.minimum_mean_ratio;else *trials<<"null";
        *trials<<",\"J\":";if(metric.shape_measured)*trials<<metric.minimum_scaled_jacobian;else *trials<<"null";
        *trials
          <<",\"size_progress\":"<<metric.size_progress<<",\"quality_progress\":"<<metric.quality_progress
          <<",\"old_cells\":"<<old.cells.size()<<",\"fresh_cells\":"<<fresh.cells.size()
          <<",\"old_surface\":"<<old.surface.size()<<",\"feature\":["<<feature.count<<','<<feature.vertices[0]<<','
          <<feature.vertices[1]<<','<<feature.vertices[2]<<"],\"reason\":\""<<text<<"\"}\n"<<std::flush;
        if(!*trials)throw std::runtime_error("Cannot write the rejected-trial log.");
      }
    });
    native->SetRejectedStateObserver([last,reference,prefix](const Owned& owned,const PlanarReference& active,const std::string& reason) {
      auto rejected=*reference;rejected.original=std::shared_ptr<const PlanarReference>(reference->original,&active); // Borrowed for this call only.
      if(owned.size()>4096)throw std::runtime_error("Controlled rejected state exceeds its diagnostic storage bound.");
      std::vector<OwnedCell> rows;for(const auto& row:owned)rows.push_back(row.second);
      SaveRecords(prefix,rows,{},rejected);
      std::vector<char> bytes;RecordStream encode(bytes);uint64_t count=rows.size();encode(count);for(auto& row:rows)encode(row);
      std::ofstream raw(prefix+".bin",std::ios::binary);raw.write(bytes.data(),bytes.size());if(!raw)throw std::runtime_error("Cannot save rejected native state.");
      std::ofstream authority(prefix+".native_ref");authority<<EncodeReference(rejected);if(!authority)throw std::runtime_error("Cannot save rejected native authority.");
      std::ofstream why(prefix+".txt");why<<reason<<"\nLast observed local trial: "<<last->reason<<'\n';if(!why)throw std::runtime_error("Cannot save rejected native reason.");
      SaveRecords(prefix+"_trial_before",PatchRecords(last->old),{},rejected);
      SaveRecords(prefix+"_trial_after",PatchRecords(last->fresh),{},rejected);
      std::ofstream info(prefix+"_trial.json");info<<std::setprecision(17)<<"{\"count\":"<<last->feature.count
        <<",\"vertices\":["<<last->feature.vertices[0]<<','<<last->feature.vertices[1]<<','<<last->feature.vertices[2]
        <<"],\"candidate\":"<<(last->candidate?"true":"false")<<",\"shape_measured\":"<<(last->metric.shape_measured?"true":"false")<<",\"minimum_quality\":";
      if(last->metric.shape_measured)info<<last->metric.minimum_mean_ratio;else info<<"null";
      info<<",\"minimum_scaled_jacobian\":";if(last->metric.shape_measured)info<<last->metric.minimum_scaled_jacobian;else info<<"null";
      info<<",\"weakest_cell\":"<<last->metric.weakest_cell<<",\"sliver_cell\":"<<last->metric.sliver_cell<<"}\n";
      if(!info)throw std::runtime_error("Cannot save rejected trial diagnosis.");
      for(const auto& trial:std::vector<std::pair<std::string,const Patch*>>{{"before",&last->old},{"after",&last->fresh}}) {
        if(trial.second->parameters.size()>4*MaximumReplacementCells)throw std::runtime_error("Rejected parameter diagnostic exceeds its storage bound.");
        std::vector<char> payload;RecordStream stream(payload);uint64_t count=trial.second->parameters.size();stream(count);
        for(auto row:trial.second->parameters)stream(row.second);
        std::ofstream out(prefix+"_trial_"+trial.first+".parameters.bin",std::ios::binary);out.write(payload.data(),payload.size());
        if(!out)throw std::runtime_error("Cannot save rejected forward geometry parameters.");
      }
    });
  }

  auto& g=driver->Geometry();auto& solver=driver->Solver();
  if(symmetry) {
    auto* flow=solver.GetNodes();auto* turbulence=driver->Turbulence()->GetNodes();
    const double rho=SU2_TYPE::GetValue(flow->GetSolution(0,0)),energy=SU2_TYPE::GetValue(flow->GetSolution(0,4));
    std::array<double,3> momentum;double kinetic=0;
    for(unsigned k=0;k<3;++k){momentum[k]=SU2_TYPE::GetValue(flow->GetSolution(0,k+1));kinetic+=momentum[k]*momentum[k]/(2*rho);}
    const double internal=energy-kinetic,nu=SU2_TYPE::GetValue(turbulence->GetSolution(0,0));REQUIRE(internal>0);REQUIRE(nu>0);
    for(size_t p=0;p<g.GetnPoint();++p) {
      const auto* x=g.nodes->GetCoord(p);const double f[]={1+.05*double(x[2]),1+.05*double(x[2]),.9+.03*double(x[1])};
      for(unsigned level=0;level<3;++level) {
        double u[]={rho,momentum[0]*f[level],momentum[1]*f[level],momentum[2]*f[level],
            internal*(level==2?.96:1.)+kinetic*f[level]*f[level]};
        for(unsigned k=0;k<5;++k) {
          if(!level)flow->SetSolution(p,k,u[k]);else if(level==1)flow->GetSolution_time_n()(p,k)=u[k];else flow->GetSolution_time_n1()(p,k)=u[k];
        }
      }
      turbulence->SetSolution(p,0,nu*(1+.1*double(x[2])));
      turbulence->GetSolution_time_n()(p,0)=nu*(1+.1*double(x[2]));
      turbulence->GetSolution_time_n1()(p,0)=nu*.94*(1+.03*double(x[1]));
    }
    {Mute mute;CMeshOutput::WriteMesh(&driver->Config(),&g,name+"_initial_adopted");}
  }
  const auto before=State(g,solver),beforeTurb=State(g,*driver->Turbulence());
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
  std::vector<FlowRecord> oldTurb;
  if(symmetry) {
    oldTurb=Flow(world,g,*driver->Turbulence());Save(world,name+"_initial",original.cells,original.original,*driver->Reference());
    if(!world.rank){std::ofstream out(name+"_frozen_profile.json");out<<std::setprecision(17)<<"{\"budget\":"<<budget
      <<",\"parameter\":"<<selected.parameter<<",\"version\":"<<selected.version<<",\"core\":"<<family->core
      <<",\"normalization_log_scale\":"<<family->normalization.logScale<<",\"original_field\":true}\n";REQUIRE(bool(out));}
  }

  if(oblique) {
    const auto encoded=EncodeReference(*driver->Reference());CHECK_FALSE(driver->Reference()->original->Affine().empty());
    CHECK(encoded.find(mapped?(std::getenv("SU2_NATIVE_MAPPED_COMPONENTS")?"SU2_NATIVE_REFERENCE_3D 6\n":"SU2_NATIVE_REFERENCE_3D 5\n"):"SU2_NATIVE_REFERENCE_3D 4\n")==0);
    if(!world.rank){std::ofstream out(name+".planes");out<<"SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" 17 2 0 0 0 -.37 1 -.61 1e-10 1e-13\n";REQUIRE(bool(out));}
    SU2_MPI::Barrier(world.comm);ImportedGeometry refused;BridgeStats stats;std::string reason;auto retained=*driver->Reference();
    CHECK_FALSE(ImportGeometry(driver->Config(),g,&solver.GetNodes()->GetMetric(),retained,refused,reason,stats,size_t(128)<<20));
    CHECK(reason.find("differs from its configuration declaration")!=std::string::npos);
    CHECK(refused.cells.empty());CHECK(EncodeReference(retained)==encoded);CHECK(State(g,solver)==before);
    if(!world.rank){std::ofstream out(name+".planes");out<<"SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" 17 1 0 0 0 -.37 1 -.61 1e-10 1e-13\n";REQUIRE(bool(out));}
    SU2_MPI::Barrier(world.comm);
  }
  const auto result=backend->Remesh(driver->Config(),g,solver.GetNodes()->GetMetric());
  INFO(native->Statistics().normalization.reason);
  CHECK(State(g,solver)==before);CHECK(State(g,*driver->Turbulence())==beforeTurb);
  REQUIRE(result.status==CRemeshResult::Status::COMPLETE);
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
  std::vector<FlowRecord> acceptedTurb;
  if(symmetry){acceptedTurb=Flow(world,driver->Geometry(),*driver->Turbulence());SaveStates(world,name+"_turb",oldTurb,acceptedTurb,theta);}

  SaveStates(world,name,oldFlow,fresh,theta);SaveProjection(world,name,transfer);CheckPositiveFlow(fresh);
  for(size_t p=0;p<sensor.rows();++p)for(unsigned k=0;k<6;++k)CHECK(sensor(p,k)==Approx(k==0||k==3||k==5?theta:0.).margin(2e-12));
  auto frozen=original.original;for(auto& d:frozen)d.sensor.fill({theta,0,0,theta,0,theta});
  const auto authority=EncodeReference(reference);Save(world,name,accepted.cells,frozen,reference);
  OutputRoundTrip(world,name,result.slices,accepted.cells,reference,theta,
      [](const su2double*,double t){return Tensor{t,0,0,t,0,t};},&frozen);
  if(symmetry)for(const auto& extension:std::vector<std::pair<std::string,std::string>>{{"SU2",".su2"}
#ifdef HAVE_CGNS
      ,{"CGNS",".cgns"}
#endif
      }) {
    const auto path=name+"_written_"+extension.first+extension.second+".native_ref";
    WriteReference(path,reference);ReferenceState persisted;REQUIRE(LoadReference(path,persisted));
    CHECK(EncodeReference(persisted)==authority);
  }
  {Mute mute;CMeshOutput::WriteMesh(&driver->Config(),&driver->Geometry(),name+"_accepted");}
  CHECK(EncodeReference(*driver->Reference())==authority);
  if(!world.rank) {
    std::ofstream out(name+"_profile.json");out<<std::setprecision(17)<<"{\"budget\":"<<budget<<",\"parameter\":"<<selected.parameter
      <<",\"version\":"<<selected.version<<",\"trials\":"<<family->budget_selection.trials<<",\"workers\":"<<workers
      <<",\"accepted\":true,\"mass\":"<<result.complexity.value<<",\"indicator\":"<<result.complexity.uncertainty
      <<",\"final_sensor_eigenvalue\":"<<theta<<",\"frozen\":true,\"support_changed\":false,\"proven_bound\":false}\n";REQUIRE(bool(out));
  }
  const auto checkpointFlow=State(driver->Geometry(),driver->Solver()),checkpointTurb=State(driver->Geometry(),*driver->Turbulence());
  driver->Config().SetTimeIter(2);driver->WriteTimeHistoryRestarts();
  CHECK(State(driver->Geometry(),driver->Solver())==checkpointFlow);CHECK(State(driver->Geometry(),*driver->Turbulence())==checkpointTurb);
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
  if(symmetry)SaveStates(world,name+"_turb_restart",acceptedTurb,Flow(world,driver->Geometry(),*driver->Turbulence()),theta);
  CHECK(EncodeReference(*driver->Reference())==authority);
  {Mute mute;driver->Finalize();driver.reset();}
}


} // namespace
TEST_CASE("Opt-in BL budget selection stays frozen through native RANS replacement", "[NativeBLBudgetRuntime3D]") {
  BudgetRuntime3D(false);
}
TEST_CASE("Declared symmetry reference preserves wall intersections and original donors through RANS restart", "[NativeSymmetryRuntime3D]") {
  BudgetRuntime3D(true);
}

TEST_CASE("Affine declarations adopt geometry before Euler and RANS states and preserve original sources", "[NativeAffineInitialDriver3D]") {
  World world;const bool rans=GENERATE(false,true);const unsigned components=GENERATE(1u,2u);
  const std::string name=std::string(rans?"affine_initial_rans":"affine_initial_euler")+"_c"+std::to_string(components);
  auto options=normalization_test::Options(name,std::min(2,world.size),false);
  {std::ostringstream angle;angle<<std::setprecision(17)<<"SIDESLIP_ANGLE= "<<std::atan(.37)*180/std::acos(-1.)<<'\n';options+=angle.str();}
  if(!rans) {
    for(const auto& change:std::vector<std::pair<std::string,std::string>>{{"SOLVER= RANS","SOLVER= EULER"},
        {"KIND_TURB_MODEL= SA\n",""},{"MARKER_HEATFLUX= (wall,0)","MARKER_EULER= (wall)"}}) {
      const auto at=options.find(change.first);REQUIRE(at!=std::string::npos);options.replace(at,change.first.size(),change.second);
    }
  }
  if(!world.rank) {
    auto mesh=simplex_test::MakeSimplexMesh(3,1,[](const passivedouble* p){return p[2]==0?"wall":p[1]==0?"symmetry":"far";});
    ShearSymmetryMesh(mesh);
    if(components==2){auto second=mesh;for(size_t p=0;p<second.GetnPoint();++p){second.coord[3*p]+=3;second.coord[3*p+1]+=5;second.coord[3*p+2]+=2;}AppendSymmetryMesh(mesh,second);}
    simplex_test::WriteSU2Mesh(mesh,name+".su2");
    std::ofstream ref(name+".planes");ref<<"SU2_NATIVE_SYMMETRY_PLANES 2 "<<components<<"\n\"symmetry\" 17 1 0 0 0 -.37 1 -.61 1e-10 1e-13\n";
    if(components==2)ref<<"\"symmetry\" 18 1 3 5 2 -.37 1 -.61 1e-10 1e-13\n";REQUIRE(bool(ref));
    std::ofstream cfg(name+".cfg");cfg<<options<<"MARKER_SYM= (symmetry)\nADAP_NATIVE_SYMMETRY_REFERENCE= "<<name<<".planes\n";REQUIRE(bool(cfg));
  }
  SU2_MPI::Barrier(world.comm);const auto cfg=name+".cfg";auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  auto& g=driver->Geometry();const auto states=State(g,driver->Solver());SymmetryReferenceAudit3D audit;std::string reason;
  REQUIRE(PrepareSymmetryReference(driver->Config(),g,false,reason,&audit));CHECK(audit.changed_points==0);
  CHECK(audit.maximum_mapping_error<=1e-13);CHECK(audit.junction_points>0);
  auto backend=driver->Backend();auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  native->PrepareReference(driver->Config(),g);CHECK_FALSE(driver->Reference()->original->Affine().empty());
  const auto encoded=EncodeReference(*driver->Reference());CHECK(encoded.find("SU2_NATIVE_REFERENCE_3D 4\n")==0);
  CHECK(EncodeReference(DecodeReference(encoded))==encoded);CHECK(State(g,driver->Solver())==states);
  su2activematrix metric(g.GetnPointDomain(),6);
  for(size_t p=0;p<metric.rows();++p){const auto* q=g.nodes->GetCoord(p);for(unsigned k=0;k<6;++k)metric(p,k)=0;metric(p,0)=2+.125*q[0];metric(p,3)=3+.125*q[1];metric(p,5)=4+.125*q[2];}
  const auto imported=ImportGeometry(driver->Config(),g,&metric,*driver->Reference());
  Save(world,name,imported.cells,imported.original,*driver->Reference());WriteReference(name+".native_ref",*driver->Reference());
  CHECK(State(g,driver->Solver())==states);CHECK(EncodeReference(*driver->Reference())==encoded);
  {Mute mute;driver->Finalize();driver.reset();}
}

TEST_CASE("Affine initial adoption rejects narrow-gap contact atomically despite positive tetrahedra", "[NativeAffineInitialDriver3D]") {
  World world;const std::string name="affine_narrow_gap";
  auto mesh=simplex_test::MakeSimplexMesh(3,1,[](const passivedouble* p){return p[1]==0?"symmetry":"far";});
  const auto nodes=mesh.GetnPoint();auto second=mesh;
  for(size_t p=0;p<nodes;++p) {
    const double y=mesh.coord[3*p+1];mesh.coord[3*p+1]+=.37*mesh.coord[3*p]+(y==0?1e-11:0);
    second.coord[3*p]+=1+2e-13;second.coord[3*p+1]+=.37*second.coord[3*p]+(y==0?-1e-11:0);
  }
  AppendSymmetryMesh(mesh,second);
  auto options=normalization_test::Options(name,std::min(2,world.size),false);
  const auto wall=options.find("MARKER_HEATFLUX= (wall,0)\n");REQUIRE(wall!=std::string::npos);options.erase(wall,std::string("MARKER_HEATFLUX= (wall,0)\n").size());
  for(const auto& change:std::vector<std::pair<std::string,std::string>>{{"SOLVER= RANS","SOLVER= EULER"},{"KIND_TURB_MODEL= SA\n",""}}) {
    const auto at=options.find(change.first);REQUIRE(at!=std::string::npos);options.replace(at,change.first.size(),change.second);
  }
  options+="MARKER_SYM= (symmetry)\n";
  {std::ostringstream angle;angle<<std::setprecision(17)<<"SIDESLIP_ANGLE= "<<std::atan(.37)*180/std::acos(-1.)<<'\n';options+=angle.str();}
  if(!world.rank){simplex_test::WriteSU2Mesh(mesh,name+".su2");std::ofstream cfg(name+".cfg");cfg<<options;REQUIRE(bool(cfg));}
  SU2_MPI::Barrier(world.comm);const auto cfg=name+".cfg";auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  auto& g=driver->Geometry();const auto states=State(g,driver->Solver());std::vector<std::array<double,3>> coordinates;
  for(size_t p=0;p<g.GetnPoint();++p){const auto* q=g.nodes->GetCoord(p);coordinates.push_back({double(q[0]),double(q[1]),double(q[2])});}
  if(!world.rank){std::ofstream ref(name+".planes");ref<<"SU2_NATIVE_SYMMETRY_PLANES 2 1\n\"symmetry\" 17 1 0 0 0 -.37 1 0 1e-9 1e-13\n";REQUIRE(bool(ref));}
  SU2_MPI::Barrier(world.comm);std::stringstream text(options+"ADAP_NATIVE_SYMMETRY_REFERENCE= "+name+".planes\n");std::unique_ptr<CConfig> declared;
  {Mute mute;declared=std::make_unique<CConfig>(text,SU2_COMPONENT::SU2_CFD,false);}
  declared->SetnMarker_All(g.GetnMarker());
  for(unsigned m=0;m<g.GetnMarker();++m){declared->SetMarker_All_TagBound(m,driver->Config().GetMarker_All_TagBound(m));declared->SetMarker_All_KindBC(m,driver->Config().GetMarker_All_KindBC(m));declared->SetMarker_All_SendRecv(m,driver->Config().GetMarker_All_SendRecv(m));}
  SymmetryReferenceAudit3D report;std::string reason;
  CHECK_FALSE(PrepareSymmetryReference(*declared,g,true,reason,&report));INFO(reason);
  CHECK(reason.find("nonconforming boundary-cell contact")!=std::string::npos);CHECK(report.minimum_volume_ratio>0);CHECK(report.boundary_pair_checks>0);
  for(size_t p=0;p<g.GetnPoint();++p)for(unsigned k=0;k<3;++k)CHECK(g.nodes->GetCoord(p,k)==coordinates[p][k]);
  CHECK(State(g,driver->Solver())==states);
  if(!world.rank) {
    const SU2NativeGeometry::AffineAuthority authority{17,1,3,{0,0,0},{-.37,1,0},1e-13};std::set<unsigned long> boundary;
    for(const auto& marker:mesh.markers)if(marker.name=="symmetry")for(auto id:marker.elem)boundary.insert(id);
    for(auto id:boundary){const auto p=SU2NativeGeometry::ProjectIntersection({mesh.coord[3*id],mesh.coord[3*id+1],mesh.coord[3*id+2]},{authority});mesh.coord[3*id]=p.x;mesh.coord[3*id+1]=p.y;mesh.coord[3*id+2]=p.z;}
    simplex_test::WriteSU2Mesh(mesh,name+"_rejected.su2");
    std::ofstream out(name+"_rejection.json");out<<std::setprecision(17)<<"{\"accepted\":false,\"boundary_witnesses\":"<<report.boundary_witnesses
      <<",\"boundary_pair_checks\":"<<report.boundary_pair_checks<<",\"minimum_volume_ratio\":"<<report.minimum_volume_ratio
      <<",\"maximum_displacement\":"<<report.maximum_displacement<<",\"boundary_seconds\":"<<report.boundary_check_seconds<<"}\n";REQUIRE(bool(out));
  }
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

TEST_CASE("FIRST_ROW solver and candidate normalization retain the construction target", "[NativeBLTargetNormalization]") {
  World world;auto driver=normalization_test::Driver(world,"first_row_family",world.size,true,true);
  auto backend=driver->Backend();auto& g=driver->Geometry();auto& solver=driver->Solver();
  std::vector<su2double> seed(6*g.GetnPointDomain());
  for(size_t i=0;i<g.GetnPointDomain();++i){seed[6*i]=2;seed[6*i+3]=3;seed[6*i+5]=4;}
  {Mute mute;solver.ComputeMetric(&g,&driver->Config(),&seed,true,nullptr,driver->Reference().get());}
  const auto family=solver.GetNativeMetricFamily3D();REQUIRE(family);REQUIRE(family->first_row);
  CHECK(family->Compatible(driver->Config()));CHECK(family->first_row->full==family->layers);
  const auto input=ImportGeometry(driver->Config(),g,&solver.GetNodes()->GetMetric(),*driver->Reference());
  DonorImport source(world);std::string why;REQUIRE(source.Build(input.original,why));
  ComplexityReassessment3D result;BridgeStats stats;std::shared_ptr<const PreparedComplexity3D> cloud;
  REQUIRE(ReassessComplexity(world,input.cells,source,*driver->Reference()->original,family->first_row->Composition(),
      result,why,stats,size_t(256)<<20,0,8192,20261017,{},&cloud));
  REQUIRE(cloud);CNativeCandidateComplexity3D assess(*family,*cloud,input.original,size_t(256)<<20);
  su2double core=family->core;const auto estimate=assess.Assess(family->selected,core);
  CHECK(estimate.value==Approx(result.value).epsilon(2e-12));
  CHECK(std::abs(estimate.value-family->normalization.estimate.value)<=estimate.uncertainty+family->normalization.estimate.uncertainty);
  {Mute mute;driver->Finalize();}
}
