// Real saved-state sensor/Hessian/metric initialization, without Solve or a CFD time advance.
TEST_CASE("Native3D selects a budget profile from a saved CFD state without advancing it", "[.][NativeSavedMetric3D]") {
  World world;const auto* configFile=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(configFile);
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>(configFile),1,world.comm);
  auto backend=driver->Backend();driver->Preprocess(0);
  REQUIRE_FALSE(driver->Config().GetTime_Domain());
  auto current=[](const CGeometry& geometry,const CSolver& solver) {
    std::vector<char> bytes;RecordStream encode(bytes);const auto* nodes=solver.GetNodes();
    for(unsigned long p=0;p<geometry.GetnPoint();++p)for(unsigned short k=0;k<solver.GetnVar();++k) {
      auto value=SU2_TYPE::GetValue(nodes->GetSolution(p,k));encode(value);
    }return bytes;
  };
  const auto flow=current(driver->Geometry(),driver->Solver()),turb=current(driver->Geometry(),*driver->Turbulence());
  const auto authority=EncodeReference(*driver->Reference());const auto begin=world.seconds();
  driver->ComputeMetric();const auto seconds=world.seconds()-begin;
  CHECK(current(driver->Geometry(),driver->Solver())==flow);CHECK(current(driver->Geometry(),*driver->Turbulence())==turb);
  CHECK(EncodeReference(*driver->Reference())==authority);
  const auto family=driver->Solver().GetNativeMetricFamily3D();REQUIRE(family);REQUIRE(family->layers);
  const auto profile=family->layers->GetOuterNormal();CHECK(profile.frozen);CHECK(family->budget_selection.refinements<=2);CHECK(family->budget_selection.trials<=9*(1+family->budget_selection.refinements));
  if(family->budget_selection.status==SU2Complexity::BudgetStatus::IntegrationLimited) {
    CHECK(family->normalization.status==SU2Complexity::Status::IntegrationLimited);
    CHECK(family->normalization.logScale==family->lower);CHECK(family->normalization.trials==1);
  }
  CHECK(CPassiveComm::Allreduce(profile.parameter,CPassiveComm::Op::MIN)==CPassiveComm::Allreduce(profile.parameter,CPassiveComm::Op::MAX));
  CHECK(CPassiveComm::Allreduce(profile.version,CPassiveComm::Op::MIN)==CPassiveComm::Allreduce(profile.version,CPassiveComm::Op::MAX));
  if(!world.rank) {
    std::ofstream out("saved_metric_budget.json");out<<std::setprecision(17)<<"{\"CFD_steps\":0,\"mesh_construction_attempted\":false,\"accepted_mesh\":false"
      <<",\"parameter\":"<<profile.parameter<<",\"version\":"<<profile.version<<",\"trials\":"<<family->budget_selection.trials<<",\"evidence_stages\":"<<family->budget_selection.refinements
      <<",\"normalization_status\":"<<unsigned(family->normalization.status)<<",\"estimate\":"<<family->normalization.estimate.value
      <<",\"uncertainty\":";if(std::isfinite(family->normalization.estimate.uncertainty))out<<family->normalization.estimate.uncertainty;else out<<"null";
    out<<",\"seconds\":"<<seconds<<",\"frozen\":true,\"original_family_bytes\":"<<family->ResidentBytes()
      <<",\"flow_and_turbulence_unchanged\":true,\"reference_unchanged\":true,\"support_changed\":false,\"proven_bound\":false}\n";REQUIRE(bool(out));
  }
  if(std::getenv("SU2_NATIVE_SAVED_CONSTRUCTION")) {
    auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
    // Save owned original values before construction; no refit or full-mesh gather.
    std::ofstream snapshot("selected_sensor_rank"+std::to_string(world.rank)+".bin",std::ios::binary);
    const uint64_t count=driver->Geometry().GetnPointDomain();snapshot.write(reinterpret_cast<const char*>(&count),sizeof(count));
    for(unsigned long p=0;p<count;++p) {
      const uint64_t id=driver->Geometry().nodes->GetGlobalIndex(p);snapshot.write(reinterpret_cast<const char*>(&id),sizeof(id));
      for(unsigned k=0;k<3;++k){const double x=SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p,k));snapshot.write(reinterpret_cast<const char*>(&x),sizeof(x));}
      const auto& m=family->selected.at(p);
      for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b){const double x=SU2_TYPE::GetValue(m.m[a][b]);snapshot.write(reinterpret_cast<const char*>(&x),sizeof(x));}
    }
    snapshot.close();REQUIRE(bool(snapshot));
    const auto start=world.seconds();const auto result=backend->Remesh(driver->Config(),driver->Geometry(),driver->Solver().GetNodes()->GetMetric());
    const double remeshSeconds=CPassiveComm::Allreduce(world.seconds()-start,CPassiveComm::Op::MAX);
    const auto& stats=native->Statistics();const bool complete=result.status==CRemeshResult::Status::COMPLETE;
    CHECK(stats.complete==complete);CHECK(current(driver->Geometry(),driver->Solver())==flow);
    CHECK(current(driver->Geometry(),*driver->Turbulence())==turb);CHECK(EncodeReference(*driver->Reference())==authority);
    CHECK(family->layers->GetOuterNormal().version==profile.version);CHECK(family->layers->GetOuterNormal().frozen);
    double transferSeconds=0;
    if(complete) {
      const auto begin=world.seconds();CConservativeTransfer transfer;driver->ReplaceMesh(result,transfer);
      transferSeconds=CPassiveComm::Allreduce(world.seconds()-begin,CPassiveComm::Op::MAX);
      SaveProjection(world,"selected_profile",transfer);driver->WriteAdaptedMesh(1,1);
      const auto* flowNodes=driver->Solver().GetNodes();const auto* turbNodes=driver->Turbulence()->GetNodes();
      for(unsigned long p=0;p<driver->Geometry().GetnPointDomain();++p) {
        const auto rho=flowNodes->GetSolution(p,0);CHECK(rho>0);su2double kinetic=0;
        for(unsigned k=1;k<=3;++k)kinetic+=flowNodes->GetSolution(p,k)*flowNodes->GetSolution(p,k)/(2*rho);
        CHECK(flowNodes->GetSolution(p,4)>kinetic);CHECK(turbNodes->GetSolution(p,0)>=0);
      }
    }else CHECK_FALSE(result.onAccepted);
    if(!world.rank) {
      std::ofstream out("selected_construction.json");out<<std::setprecision(17)
        <<"{\"CFD_steps\":0,\"construction_attempted\":true,\"accepted_mesh\":"<<(complete?"true":"false")
        <<",\"transfer_performed\":"<<(complete?"true":"false")<<",\"remesh_seconds\":"<<remeshSeconds
        <<",\"transfer_seconds\":"<<transferSeconds<<",\"parameter\":"<<profile.parameter<<",\"version\":"<<profile.version
        <<",\"initial_cells\":"<<stats.initial_cells<<",\"final_cells\":"<<stats.final_cells
        <<",\"workers\":"<<stats.workers<<",\"normalization_stages\":"<<stats.normalization_stages
        <<",\"private_edits\":"<<stats.refine.accepted<<",\"rounds\":"<<stats.refine.rounds
        <<",\"stop\":"<<std::quoted(EngineStopName(stats.refine.stop))
        <<",\"atomic_original_state_check\":true,\"profile_frozen\":true,\"proven_bound\":false}\n";REQUIRE(bool(out));
    }
  }
  backend.reset();{Mute mute;driver->Finalize();driver.reset();}
}
