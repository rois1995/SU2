// Stage2 diagnostic: load the saved CFD state and construct its production target without advancing CFD.
TEST_CASE("Saved CFD stage constructs the complete native BL target without a flow step", "[.][NativeSavedCFDMetricStage3D]") {
  const auto* path=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(path);
  class MetricStageDriver : public CSinglezoneDriver {
   public:
    using CSinglezoneDriver::CSinglezoneDriver;
    CConfig& Config(){return *config_container[ZONE_0];}
    CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
    CSolver& Flow(){return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];}
  };
  std::string config(path);MetricStageDriver driver(config.data(),1,SU2_MPI::GetComm());
  auto& flow=driver.Flow();auto& geometry=driver.Geometry();auto* nodes=flow.GetNodes();
  REQUIRE(geometry.GetnDim()==3);REQUIRE(driver.Config().GetnAdap_Sensor()==1);
  REQUIRE(driver.Config().GetAdap_Sensor(0)=="MACH");
  std::vector<std::array<double,5>> before(geometry.GetnPointDomain());
  for(size_t p=0;p<before.size();++p)for(unsigned k=0;k<5;++k)before[p][k]=SU2_TYPE::GetValue(nodes->GetSolution(p,k));
  driver.ComputeMetric();
  const auto family=flow.GetNativeMetricFamily3D();REQUIRE(family);REQUIRE(family->first_row);
  REQUIRE(family->Compatible(driver.Config()));
  double maximumMachError=0;
  for(size_t p=0;p<before.size();++p) {
    for(unsigned k=0;k<5;++k)REQUIRE(nodes->GetSolution(p,k)==before[p][k]);
    const auto& u=before[p];const double momentum2=u[1]*u[1]+u[2]*u[2]+u[3]*u[3];
    const double pressure=(SU2_TYPE::GetValue(driver.Config().GetGamma())-1)*(u[4]-.5*momentum2/u[0]);
    REQUIRE(u[0]>0);REQUIRE(pressure>0);
    const double mach=std::sqrt(momentum2/(u[0]*SU2_TYPE::GetValue(driver.Config().GetGamma())*pressure));
    maximumMachError=std::max(maximumMachError,std::abs(mach-SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(p,0))));
  }
  maximumMachError=CPassiveComm::Allreduce(maximumMachError,CPassiveComm::Op::MAX);
  CHECK(maximumMachError<1e-9);
  std::ofstream out("saved_cfd_metric_rank"+std::to_string(CPassiveComm::Rank())+".jsonl");
  out<<std::setprecision(17)<<"{\"stage\":\"metric_only\",\"provisional\":true,\"complete_checkpoint\":false,\"cfd_advanced\":false"
     <<",\"requested_complexity\":"<<driver.Config().GetAdap_Complexity()<<",\"estimated_complexity\":"<<flow.GetMetricComplexityFinal()
     <<",\"indicator\":"<<family->normalization.estimate.uncertainty<<",\"core\":"<<family->core
     <<",\"tangential_scale\":"<<family->inherited_tangent_scale<<",\"anchor\":"<<family->tangential_anchor
     <<",\"mach_error\":"<<maximumMachError<<",\"owned_nodes\":"<<family->owned<<"}\n";
  for(const auto& node:family->nodes) {
    if(node.local>=family->owned)continue;
    out<<"{\"id\":"<<node.id<<",\"x\":["<<node.point.x<<","<<node.point.y<<","<<node.point.z<<"],\"core_tensor\":[";
    const auto& m=family->selected.at(node.local);
    for(unsigned i=0,k=0;i<3;++i)for(unsigned j=i;j<3;++j)out<<(k++?",":"")<<m.m[i][j];
    out<<"],\"mach\":"<<nodes->GetAuxVar_Adapt(node.local,0)<<"}\n";
  }
  out.close();REQUIRE(bool(out));driver.Finalize();
}
