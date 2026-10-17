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
  const auto& profile=family->layers->GetOuterNormal();CHECK(profile.frozen);CHECK(family->budget_selection.trials<=9);
  if(family->budget_selection.status==SU2Complexity::BudgetStatus::IntegrationLimited) {
    CHECK(family->normalization.status==SU2Complexity::Status::IntegrationLimited);
    CHECK(family->normalization.logScale==family->lower);CHECK(family->normalization.trials==1);
  }
  CHECK(CPassiveComm::Allreduce(profile.parameter,CPassiveComm::Op::MIN)==CPassiveComm::Allreduce(profile.parameter,CPassiveComm::Op::MAX));
  CHECK(CPassiveComm::Allreduce(profile.version,CPassiveComm::Op::MIN)==CPassiveComm::Allreduce(profile.version,CPassiveComm::Op::MAX));
  if(!world.rank) {
    std::ofstream out("saved_metric_budget.json");out<<std::setprecision(17)<<"{\"CFD_steps\":0,\"mesh_construction_attempted\":false,\"accepted_mesh\":false"
      <<",\"parameter\":"<<profile.parameter<<",\"version\":"<<profile.version<<",\"trials\":"<<family->budget_selection.trials
      <<",\"normalization_status\":"<<unsigned(family->normalization.status)<<",\"estimate\":"<<family->normalization.estimate.value
      <<",\"uncertainty\":";if(std::isfinite(family->normalization.estimate.uncertainty))out<<family->normalization.estimate.uncertainty;else out<<"null";
    out<<",\"seconds\":"<<seconds<<",\"frozen\":true,\"original_family_bytes\":"<<family->ResidentBytes()
      <<",\"flow_and_turbulence_unchanged\":true,\"reference_unchanged\":true,\"support_changed\":false,\"proven_bound\":false}\n";REQUIRE(bool(out));
  }
  backend.reset();{Mute mute;driver->Finalize();driver.reset();}
}
