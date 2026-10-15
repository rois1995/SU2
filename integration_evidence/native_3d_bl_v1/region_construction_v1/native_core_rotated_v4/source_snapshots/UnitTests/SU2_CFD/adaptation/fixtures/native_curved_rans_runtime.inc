namespace {
struct SARecord {
    Point point;std::array<double,3> values{};
    template<class S> void Fields(S& s){s(point.x,point.y,point.z,values);}
  };
}
/* Test-only end-to-end curved RANS/SA evolution, native remeshing and BDF restart. */
TEST_CASE("Solved curved RANS survives native BL construction removal and restart",
          "[.][NativeCurvedRANS3D]") {
  World world;const int workers=GENERATE(1,2,4);if(workers>world.size)return;
  const auto* selected=std::getenv("SU2_NATIVE_POLICY_WORKERS");const int only=selected?std::stoi(selected):0;
  REQUIRE((only==0 || only==1 || only==2 || only==4));REQUIRE(only<=world.size);
  if(only && workers!=only)return;
  const std::string name="curved_rans_m"+std::to_string(workers);
  auto configText=[&](bool layers) {
    std::ostringstream cfg;
    cfg<<"SOLVER= RANS\nKIND_TURB_MODEL= SA\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= .2\nAOA= 90\n"
       <<"REYNOLDS_NUMBER= 1000\nREYNOLDS_LENGTH= 1\nMESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\n"
       <<"MARKER_FAR= (cap0,cap1)\nMARKER_HEATFLUX= (cylinder,0)\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\n"
       <<"ADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\nADAP_ISO_CORNER= NO\n"
       <<"ADAP_HMIN= .025\nADAP_HMAX= 1.0000000000000002\nADAP_ARMAX= 100\nADAP_COMPLEXITY= 1\nADAP_HGRAD= 1.3\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nADAP_NATIVE_MAX_WORK= 4096\nADAP_NATIVE_MAX_TIME= 300\nADAP_HAUSD= .04\n"
       <<"WRT_PERFORMANCE= YES\nMGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\nCFL_NUMBER= .5\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 3\n"
       <<"OUTPUT_FILES= (RESTART, PARAVIEW)\nOUTPUT_WRT_FREQ= 1\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= SU2\n"
       <<"MESH_OUT_FILENAME= "<<name<<"_mesh\nVOLUME_FILENAME= "<<name<<"_solution\nCONV_FILENAME= "<<name<<"_history\n"
       <<"RESTART_FILENAME= "<<name<<"_flow\nSOLUTION_FILENAME= "<<name<<"_flow\nWRT_RESTART_COMPACT= YES\n";
    if(layers)cfg<<"ADAP_BL_MARKER= (cylinder)\nADAP_BL_FIRST_HEIGHT= (.05)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.2)\n";
    return cfg.str();
  };
  if(!world.rank){simplex_test::WriteSU2Mesh(PrimitiveMesh(false,false),name+".su2");
    std::ofstream cfg(name+".cfg");cfg<<configText(true);std::ofstream next(name+"_sensor_only.cfg");next<<configText(false);}
  SU2_MPI::Barrier(world.comm);const auto cfg=name+".cfg";
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  ReferenceState declared;ImportGeometry(driver->Config(),driver->Geometry(),nullptr,declared);DeclarePrimitive(declared,false,false);
  WriteReference(name+".su2.native_ref",declared);const auto authority=EncodeReference(declared);
  auto* flow=driver->Solver().GetNodes();auto* turb=driver->Turbulence()->GetNodes();
  std::array<double,5> freestream;for(unsigned k=0;k<5;++k)freestream[k]=SU2_TYPE::GetValue(flow->GetSolution(0,k));
  const double rho=freestream[0],kinetic=(freestream[1]*freestream[1]+freestream[2]*freestream[2]+freestream[3]*freestream[3])/(2*rho);
  const double viscosity=SU2_TYPE::GetValue(turb->GetSolution(0,0));REQUIRE(viscosity>0);
  for(unsigned long p=0;p<driver->Geometry().GetnPoint();++p) {
    const auto* x=driver->Geometry().nodes->GetCoord(p);const double profile=std::max(0.,1-SU2_TYPE::GetValue(x[0]*x[0]+x[1]*x[1]));
    auto state=freestream;for(unsigned k=1;k<4;++k)state[k]*=profile;state[4]=freestream[4]-kinetic+kinetic*profile*profile;
    for(unsigned k=0;k<5;++k){flow->SetSolution(p,k,state[k]);flow->GetSolution_time_n()(p,k)=state[k];flow->GetSolution_time_n1()(p,k)=state[k]*.96875;}
    turb->SetSolution(p,0,viscosity*profile);turb->GetSolution_time_n()(p,0)=viscosity*profile;turb->GetSolution_time_n1()(p,0)=viscosity*profile*.9375;
  }
  auto captureSA=[&]() {
    std::vector<SARecord> records;const auto* nodes=driver->Turbulence()->GetNodes();
    const double lower=SU2_TYPE::GetValue(driver->Turbulence()->GetLowerLimit(0));
    for(unsigned long p=0;p<driver->Geometry().GetnPointDomain();++p){const auto* x=driver->Geometry().nodes->GetCoord(p);
      SARecord row{{SU2_TYPE::GetValue(x[0]),SU2_TYPE::GetValue(x[1]),SU2_TYPE::GetValue(x[2])},
        {SU2_TYPE::GetValue(nodes->GetSolution(p,0)),SU2_TYPE::GetValue(nodes->GetSolution_time_n(p,0)),SU2_TYPE::GetValue(nodes->GetSolution_time_n1(p,0))}};
      for(double v:row.values){CHECK(std::isfinite(v));CHECK(v>=lower);}records.push_back(row);}
    return world.metadata(records);
  };
  auto saveSA=[&](const std::string& event,const auto& before,const auto& after) {
    if(world.rank)return;std::ofstream out(event+"_SA.json");out<<std::setprecision(17)<<"{\"donor_scale\":"<<viscosity;
    auto rows=[&](const char* key,const auto& records){out<<",\""<<key<<"\":[";bool comma=false;
      for(auto row:records){if(comma)out<<',';comma=true;out<<'['<<row.point.x<<','<<row.point.y<<','<<row.point.z;
        for(double v:row.values)out<<','<<v;out<<']';}out<<']';};
    rows("before",before);rows("after",after);out<<"}\n";CHECK(out.good());
  };
  auto backend=driver->Backend();CConservativeTransfer transfer;std::vector<uint64_t> counts;
  for(unsigned cycle=0;cycle<2;++cycle) {
    const auto event=name+"_s"+std::to_string(cycle);const auto initial=Flow(world,driver->Geometry(),driver->Solver());
    const double solveStart=world.seconds();
    for(unsigned long step=cycle?3:0;step<(cycle?5:2);++step){driver->Preprocess(step);driver->Run();driver->Postprocess();driver->Update();}
    const double solveTime=world.seconds()-solveStart;
    const auto solved=Flow(world,driver->Geometry(),driver->Solver());const auto oldSA=captureSA();CheckPositiveFlow(solved);
    double change=0;for(size_t p=0;p<initial.size();++p)for(unsigned k=0;k<5;++k)change=std::max(change,std::abs(initial[p].values[k]-solved[p].values[k]));
    CHECK(change>1e-10);driver->Output(cycle?4:1);
    const double metricStart=world.seconds();driver->ComputeMetric();const double metricTime=world.seconds()-metricStart;
    const auto& metric=driver->Solver().GetNodes()->GetMetric();auto observed=declared;
    const auto donor=ImportGeometry(driver->Config(),driver->Geometry(),&metric,observed);
    Save(world,event+"_initial",donor.cells,donor.original,observed);
    const auto before=State(driver->Geometry(),driver->Solver()),beforeSA=State(driver->Geometry(),*driver->Turbulence());
    const double remeshStart=world.seconds();const auto result=backend->Remesh(driver->Config(),driver->Geometry(),metric);
    const double remeshTime=world.seconds()-remeshStart;auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
    INFO(event<<" stop="<<EngineStopName(native->Statistics().refine.stop));
    CHECK(State(driver->Geometry(),driver->Solver())==before);CHECK(State(driver->Geometry(),*driver->Turbulence())==beforeSA);
    REQUIRE(result.status==CRemeshResult::Status::COMPLETE);CHECK(native->Statistics().complete);CHECK(native->Statistics().workers==workers);
    if(native->Statistics().refine.region_cells)CHECK(native->Statistics().coarsen.global_work==0);
    const double replaceStart=world.seconds();driver->ReplaceMesh(result,transfer);const double replaceTime=world.seconds()-replaceStart;
    CHECK(EncodeReference(*driver->Reference())==authority);SaveProjection(world,event,transfer);
    CHECK(transfer.GetSummary().nVarFlow==5);CHECK(transfer.GetSummary().projection.nField==12);
    const auto transported=result.slices.FetchPointMetric(driver->Geometry());const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);
    CheckPositiveFlow(fresh);SaveStates(world,event,solved,fresh,0);const auto freshSA=captureSA();saveSA(event,oldSA,freshSA);
    DonorImport source(world);std::string reason;REQUIRE(source.Build(donor.original,reason));
    RegionFieldService verifier(world,source,*observed.original,{}, {},false,SIZE_MAX,0);double sensorResidual=0;CLocalFailure failure;
    try{verifier.Run([&](const RegionMetric& field){for(const auto& row:fresh){const auto tensor=field.point(row.point);
      const std::array<double,6> expected{tensor.xx,tensor.xy,tensor.xz,tensor.yy,tensor.yz,tensor.zz};
      for(unsigned k=0;k<6;++k)sensorResidual=std::max(sensorResidual,std::abs(row.metric[k]-expected[k])/(1+std::abs(expected[k])));
    }});}catch(const std::exception& e){failure.Set(1,0,e.what());}
    const auto rejected=world.elect(failure);INFO(rejected.message);REQUIRE_FALSE(rejected.any);if(!world.rank)CHECK(sensorResidual<2e-12);
    su2activematrix acceptedMetric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<acceptedMetric.rows();++p)for(unsigned k=0;k<6;++k)acceptedMetric(p,k)=transported[6*p+k];
    auto retained=*driver->Reference();const auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&acceptedMetric,retained);
    Save(world,event+"_accepted",accepted.cells,donor.original,retained);counts.push_back(result.slices.nElemGlobal);
    const double outputStart=world.seconds();OutputRoundTrip(world,event,result.slices,accepted.cells,retained,0,Sensor,&donor.original);
    driver->WriteAdaptedMesh(cycle+1,cycle+1);const double outputTime=world.seconds()-outputStart;
    if(!world.rank){std::ofstream out(event+"_summary.json");out<<std::setprecision(17)<<"{\"cells\":"<<counts.back()
      <<",\"workers\":"<<workers<<",\"sensor_residual\":"<<sensorResidual<<",\"flow_change\":"<<change
      <<",\"solve_seconds\":"<<solveTime<<",\"metric_seconds\":"<<metricTime<<",\"remesh_seconds\":"<<remeshTime
      <<",\"replace_seconds\":"<<replaceTime<<",\"output_seconds\":"<<outputTime<<"}\n";CHECK(out.good());}
    if(!cycle) {
#ifdef HAVE_CGNS
      const std::string restartMesh=event+"_written_CGNS.cgns",format="CGNS";
#else
      const std::string restartMesh=event+"_written_SU2.su2",format="SU2";
#endif
      WriteReference(restartMesh+".native_ref",retained);backend.reset();
      driver=ReloadCurved(world,std::move(driver),name,restartMesh,format,configText(false));
      const auto reloaded=captureSA();saveSA(name+"_restart",freshSA,reloaded);
      std::map<std::array<double,3>,std::array<double,3>> expected;
      for(auto row:freshSA)REQUIRE(expected.emplace(std::array<double,3>{row.point.x,row.point.y,row.point.z},row.values).second);
      REQUIRE(reloaded.size()==expected.size());
      for(auto row:reloaded)for(unsigned k=0;k<3;++k)CHECK(RelDiff(row.values[k],expected.at({row.point.x,row.point.y,row.point.z})[k],viscosity)<2e-10);
      backend=driver->Backend();
    }
  }
  REQUIRE(counts.size()==2);CHECK(counts[0]>24);CHECK(counts[1]<counts[0]);
  driver->Preprocess(5);driver->Run();driver->Postprocess();driver->Update();
  CheckPositiveFlow(Flow(world,driver->Geometry(),driver->Solver()));captureSA();driver->Output(5);
  {Mute mute;driver->Finalize();driver.reset();}
}
