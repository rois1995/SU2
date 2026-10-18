// Hidden complete RANS lifecycle on the declared sphere; one resumable event per invocation.
namespace {
struct SphereRuntimeTurbulence {
  Id id;Point p;std::array<double,3> value;
  template<class S> void Fields(S& s){s(id,p.x,p.y,p.z,value);}
};
}
TEST_CASE("Native closed-sphere BL follows actual RANS replacement and restart", "[.][NativeSphereBLRuntime3D]") {
  World world;const char* cfg=std::getenv("SU2_NATIVE_SPHERE_BL_CFG");REQUIRE(cfg);
  const auto begin=world.seconds();auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg),1,world.comm);
  const bool restart=driver->Config().GetRestart();const bool walls=driver->Config().GetnAdap_BL()!=0;
  if(restart)driver->Preprocess(driver->Config().GetRestart_Iter());
  else {
    auto* flow=driver->Solver().GetNodes();auto* turb=driver->Turbulence()->GetNodes();
    const double rho=SU2_TYPE::GetValue(driver->Solver().GetDensity_Inf()),energy=SU2_TYPE::GetValue(driver->Solver().GetDensity_Energy_Inf());
    const double nu=SU2_TYPE::GetValue(driver->Turbulence()->GetNuTilde_Inf());REQUIRE(rho>0);REQUIRE(energy>0);REQUIRE(nu>0);
    for(unsigned long p=0;p<driver->Geometry().GetnPoint();++p) {
      const auto* x=driver->Geometry().nodes->GetCoord(p);
      const double value=nu*std::max(0.,1.-SU2_TYPE::GetValue(x[0]*x[0]+x[1]*x[1]+x[2]*x[2]));
      for(unsigned k=0;k<5;++k){const double u=k==0?rho:k==4?energy:0.;flow->SetSolution(p,k,u);flow->GetSolution_time_n()(p,k)=u;flow->GetSolution_time_n1()(p,k)=u*.96875;}
      turb->SetSolution(p,0,value);turb->GetSolution_time_n()(p,0)=value;turb->GetSolution_time_n1()(p,0)=value*.9375;
    }
  }
  auto saveTurb=[&](const char* name) {
    std::vector<SphereRuntimeTurbulence> rows;const auto* n=driver->Turbulence()->GetNodes();
    for(unsigned long p=0;p<driver->Geometry().GetnPointDomain();++p) {
      const auto* x=driver->Geometry().nodes->GetCoord(p);
      rows.push_back({driver->Geometry().nodes->GetGlobalIndex(p),{SU2_TYPE::GetValue(x[0]),SU2_TYPE::GetValue(x[1]),SU2_TYPE::GetValue(x[2])},
          {SU2_TYPE::GetValue(n->GetSolution(p,0)),SU2_TYPE::GetValue(n->GetSolution_time_n(p,0)),SU2_TYPE::GetValue(n->GetSolution_time_n1(p,0))}});
    }
    const auto all=world.metadata(rows);if(world.rank)return;
    std::ofstream out(name);out<<std::setprecision(17)<<"id,x,y,z,current,n,n1\n";
    for(const auto& r:all)out<<r.id<<','<<r.p.x<<','<<r.p.y<<','<<r.p.z<<','<<r.value[0]<<','<<r.value[1]<<','<<r.value[2]<<'\n';REQUIRE(bool(out));
  };
  ReferenceState declared;REQUIRE(LoadReference(driver->Config().GetMesh_FileName()+".native_ref",declared));
  const auto authority=EncodeReference(declared);auto backend=driver->Backend();auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  auto& geometry=driver->Geometry();su2activematrix sensor(geometry.GetnPointDomain(),6);
  for(unsigned long p=0;p<sensor.rows();++p)for(unsigned k=0;k<6;++k)sensor(p,k)=k==0||k==3||k==5?1.:0.;
  const auto input=ImportGeometry(driver->Config(),geometry,&sensor,declared);
  REQUIRE(world.sum(input.cells.size())<=6144);Save(world,"event_initial",input.cells,input.original,declared);
  const auto old=Flow(world,geometry,driver->Solver());SaveStates(world,"before",{},old,1.);saveTurb("before_turb.csv");
  const auto before=State(geometry,driver->Solver()),beforeTurb=State(geometry,*driver->Turbulence());
  const double prepared=world.seconds();const auto result=backend->Remesh(driver->Config(),geometry,sensor);const double remeshed=world.seconds();
  CHECK(State(geometry,driver->Solver())==before);CHECK(State(geometry,*driver->Turbulence())==beforeTurb);
  const auto stats=native->Statistics();
  {
    std::ofstream out("status_rank"+std::to_string(world.rank)+".json");out<<std::setprecision(17)
      <<"{\"complete\":"<<(result.status==CRemeshResult::Status::COMPLETE?"true":"false")<<",\"remesh_seconds\":"<<remeshed-prepared
      <<",\"work\":"<<stats.cumulative_work<<",\"unchanged_input\":true}\n";REQUIRE(bool(out));
  }
  REQUIRE(result.status==CRemeshResult::Status::COMPLETE);CHECK(stats.complete);CHECK(stats.cumulative_work<=6144);
  CConservativeTransfer transfer;driver->ReplaceMesh(result,transfer);const double replaced=world.seconds();
  CHECK(EncodeReference(*driver->Reference())==authority);SaveProjection(world,"event",transfer);
  const auto transported=result.slices.FetchPointMetric(driver->Geometry());const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);
  SaveStates(world,"event",old,fresh,1.);CheckPositiveFlow(fresh);saveTurb("accepted_turb.csv");
  su2activematrix acceptedMetric(driver->Geometry().GetnPointDomain(),6);
  for(unsigned long p=0;p<acceptedMetric.rows();++p)for(unsigned k=0;k<6;++k) {
    acceptedMetric(p,k)=transported[6*p+k];CHECK(acceptedMetric(p,k)==Approx(k==0||k==3||k==5?1.:0.).margin(2e-14));
  }
  auto* turb=driver->Turbulence()->GetNodes();const double lower=SU2_TYPE::GetValue(driver->Turbulence()->GetLowerLimit(0));
  for(unsigned long p=0;p<acceptedMetric.rows();++p)for(double value:{SU2_TYPE::GetValue(turb->GetSolution(p,0)),SU2_TYPE::GetValue(turb->GetSolution_time_n(p,0)),SU2_TYPE::GetValue(turb->GetSolution_time_n1(p,0))}) {
    CHECK(std::isfinite(value));CHECK(value>=lower);
  }
  auto retained=*driver->Reference();const auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&acceptedMetric,retained);
  Save(world,"event_accepted",accepted.cells,input.original,retained);
  OutputRoundTrip(world,"event",result.slices,accepted.cells,retained,1.,[](const su2double*,double){return Tensor{1,0,0,1,0,1};},&input.original);
  for(const auto& format:std::vector<std::string>{"SU2"
#ifdef HAVE_CGNS
      ,"CGNS"
#endif
  })WriteReference("event_written_"+format+(format=="CGNS"?".cgns":".su2")+".native_ref",retained);
  const double written=world.seconds();driver->Config().SetTimeIter(2);
  const auto saved=State(driver->Geometry(),driver->Solver()),savedTurb=State(driver->Geometry(),*driver->Turbulence());
  driver->WriteTimeHistoryRestarts();CHECK(State(driver->Geometry(),driver->Solver())==saved);CHECK(State(driver->Geometry(),*driver->Turbulence())==savedTurb);
  const double ended=world.seconds();
  std::ofstream out("lifecycle_rank"+std::to_string(world.rank)+".json");out<<std::setprecision(17)
    <<"{\"ranks\":"<<world.size<<",\"workers\":"<<stats.workers<<",\"BL\":"<<(walls?"true":"false")
    <<",\"restart_input\":"<<(restart?"true":"false")<<",\"initial_cells\":"<<stats.initial_cells<<",\"final_cells\":"<<stats.final_cells
    <<",\"work\":"<<stats.cumulative_work<<",\"prepare_seconds\":"<<prepared-begin<<",\"remesh_seconds\":"<<remeshed-prepared
    <<",\"replace_seconds\":"<<replaced-remeshed<<",\"audit_output_seconds\":"<<written-replaced<<",\"history_write_seconds\":"<<ended-written
    <<",\"total_seconds\":"<<ended-begin<<",\"native_seconds\":"<<stats.total_seconds<<",\"native_other_seconds\":"<<stats.other_seconds
    <<",\"native_bytes\":"<<stats.maximum_native_bytes<<",\"process_peak_rss_bytes\":"<<stats.process_peak_rss_bytes
    <<",\"communication_bytes\":"<<stats.communication_bytes<<",\"communication_calls\":"<<stats.communication_calls
    <<",\"communication_seconds\":"<<stats.communication_seconds<<",\"reconstruction_seconds\":"<<stats.refine.reconstruction_seconds+stats.coarsen.reconstruction_seconds
    <<",\"validation_seconds\":"<<stats.refine.region_validation_seconds+stats.coarsen.region_validation_seconds
    <<",\"CFD_steps\":0,\"original_sensor_only\":true,\"phases\":{";
  for(size_t k=0;k<stats.phases.size();++k){if(k)out<<',';out<<'"'<<RemeshStats::names[k]<<"\":"<<stats.phases[k];}out<<"}}\n";REQUIRE(bool(out));
  {Mute mute;driver->Finalize();driver.reset();}
}
