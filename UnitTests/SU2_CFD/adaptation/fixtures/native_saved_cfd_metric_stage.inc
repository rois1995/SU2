// Stage2 diagnostics: saved CFD -> actual complete target -> independently reusable checkpoint. No flow step.
namespace {
class SavedNativeMetricDriver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::CSinglezoneDriver;
  CConfig& Config(){return *config_container[ZONE_0];}
  CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
  CSolver& Flow(){return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];}
  ReferenceState& Reference(){if(!nativeReference3D || !nativeReference3D->original){auto setup=MakeRemesher();}return *nativeReference3D;}
};
void CheckSavedProfileAgreement(const CNativeMetricFamily3D& family) {
  std::vector<char> bytes;SU2NativeBoundary2D::RecordStream stream(bytes);
  auto full=family.first_row->full->GetProfileHistory(),base=family.first_row->base->GetProfileHistory();
  auto a=SU2NativeTargetCheckpoint3D::detail::State(*family.first_row->full),b=SU2NativeTargetCheckpoint3D::detail::State(*family.first_row->base);
  double core=family.first_row->core,radius=double(family.first_row->A->radius);stream(a,b,core,radius);
  SU2NativeTargetCheckpoint3D::detail::History(stream,full,false,SIZE_MAX);SU2NativeTargetCheckpoint3D::detail::History(stream,base,false,SIZE_MAX);
  uint64_t count=bytes.size()/8;
  REQUIRE(CPassiveComm::Allreduce(count,CPassiveComm::Op::MIN)==CPassiveComm::Allreduce(count,CPassiveComm::Op::MAX));
  REQUIRE(count<=INT_MAX);std::vector<uint64_t> words(count),low(count),high(count);
  std::memcpy(words.data(),bytes.data(),bytes.size());
  CPassiveComm::Allreduce(words.data(),low.data(),int(count),CPassiveComm::Op::MIN);
  CPassiveComm::Allreduce(words.data(),high.data(),int(count),CPassiveComm::Op::MAX);
  REQUIRE(low==high);
}
SU2NativeGeometry::AtlasBudget SavedTargetBudget(SavedNativeMetricDriver& driver,const CNativeMetricFamily3D& family) {
  auto& config=driver.Config();auto& mesh=driver.Geometry();World world;SU2NativeGeometry::AtlasBudget budget;budget.maximum_bytes=config.GetAdap_Native_MemoryLimit();
  budget.caller_bytes=transfer_memory::Add(ReferenceStorageBytes(driver.Reference()),family.ResidentBytes(),size_t(128)<<20);
  constexpr size_t scratch=size_t(16)<<20;size_t low=0,high=scratch*8;
  while(low<high){const auto bits=low+(high-low+1)/2;if(transfer_memory::Mul(2,SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits))<=scratch/2)low=bits;else high=bits-1;}
  budget.maximum_integer_bits=low;
  if(!low || budget.caller_bytes>=budget.maximum_bytes)throw std::length_error("Checkpoint chord scratch exceeds admission.");
  const auto cells=size_t(mesh.GetGlobal_nElem());const auto maximum=config.GetAdap_Native_Max_Work()?size_t(config.GetAdap_Native_Max_Work()):std::max(size_t(4096),transfer_memory::Mul(cells,64));
  budget.maximum_work=transfer_memory::Mul(transfer_memory::Add(cells,maximum),SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  const auto started=world.seconds();const double seconds=std::min(300.,SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time()));if(!(seconds>0))throw std::invalid_argument("Checkpoint chord requires a positive deadline.");
  budget.within_time=[started,seconds](){return SU2_MPI::Wtime()-started<seconds;};
  return budget;
}
double SavedTargetChord(SavedNativeMetricDriver& driver,const CNativeMetricFamily3D& family,std::ostream& out) {
  const auto* context=std::getenv("SU2_NATIVE_BL_CAVITY_CONTEXTS");if(!context)return 0;
  std::ifstream input(context);size_t count=0,cavity=0;std::string label;Id first=0,last=0;
  input>>count>>label>>first>>last>>cavity;REQUIRE(bool(input));REQUIRE(count>0);REQUIRE(first!=last);
  auto& mesh=driver.Geometry();World world;size_t localCell=SIZE_MAX;
  for(size_t e=0;e<mesh.GetnElem();++e) {
    const auto* cell=mesh.elem[e];if(cell->GetVTK_Type()!=TETRAHEDRON)continue;
    bool a=false,b=false;for(unsigned k=0;k<4;++k){const auto id=mesh.nodes->GetGlobalIndex(cell->GetNode(k));a|=id==first;b|=id==last;}
    if(a&&b){localCell=e;break;}
  }
  const int owner=CPassiveComm::Allreduce(localCell==SIZE_MAX?INT_MAX:world.rank,CPassiveComm::Op::MIN);
  REQUIRE(owner!=INT_MAX);double length=0,busy=0;uint64_t evaluations=0,work=0;CLocalFailure failure;
  if(world.rank==owner)try {
    DonorCell donor;donor.cell.id=0;Point a{},b{};
    for(unsigned k=0;k<4;++k) {
      const auto local=mesh.elem[localCell]->GetNode(k);const auto id=mesh.nodes->GetGlobalIndex(local);const auto* p=mesh.nodes->GetCoord(local);
      donor.cell.v[k]={id,{SU2_TYPE::GetValue(p[0]),SU2_TYPE::GetValue(p[1]),SU2_TYPE::GetValue(p[2])}};
      donor.sensor[k]=SU2NativeBLPipeline3D::TargetPolicy::Pack(family.selected.at(local));
      if(id==first)a=donor.cell.v[k].p;if(id==last)b=donor.cell.v[k].p;
    }
    if(Orientation(donor.cell.v[0].p,donor.cell.v[1].p,donor.cell.v[2].p,donor.cell.v[3].p)<0) {
      std::swap(donor.cell.v[1],donor.cell.v[2]);std::swap(donor.sensor[1],donor.sensor[2]);
    }
    auto budget=SavedTargetBudget(driver,family);const auto started=world.seconds();
    auto lease=family.first_row->ReserveGeometry(budget);
    FrozenField target({donor},family.first_row->Composition(),true,{},family.first_row->Edges());
    length=double(target.TargetEdgeLength(a,b));evaluations=target.Statistics().edge_compositions;work=budget.work;busy=world.seconds()-started;
  }catch(const std::exception& e){failure.Set(3,0,e.what());}
  const auto elected=world.elect(failure);if(elected.any)throw std::runtime_error(elected.message);
  const auto join=world.seconds();length=CPassiveComm::Allreduce(length,CPassiveComm::Op::MAX);
  out<<"{\"stage\":\"checkpoint_chord\",\"owner\":"<<owner<<",\"length\":"<<length<<",\"local_evaluations\":"<<evaluations
     <<",\"local_geometry_work\":"<<work<<",\"local_busy_seconds\":"<<busy<<",\"result_join_seconds\":"<<world.seconds()-join<<"}\n";
  return length;
}
}

TEST_CASE("Saved CFD stage constructs the complete native BL target without a flow step", "[.][NativeSavedCFDMetricStage3D]") {
  const auto* path=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(path);
  std::string config(path);SavedNativeMetricDriver driver(config.data(),1,SU2_MPI::GetComm());
  auto& flow=driver.Flow();auto& geometry=driver.Geometry();auto* nodes=flow.GetNodes();
  REQUIRE(geometry.GetnDim()==3);REQUIRE(driver.Config().GetnAdap_Sensor()==1);REQUIRE(driver.Config().GetAdap_Sensor(0)=="MACH");
  std::vector<std::array<double,5>> before(geometry.GetnPointDomain());
  for(size_t p=0;p<before.size();++p)for(unsigned k=0;k<5;++k)before[p][k]=SU2_TYPE::GetValue(nodes->GetSolution(p,k));
  driver.ComputeMetric();const auto family=flow.GetNativeMetricFamily3D();REQUIRE(family);REQUIRE(family->first_row);REQUIRE(family->Compatible(driver.Config()));
  double maximumMachError=0;
  for(size_t p=0;p<before.size();++p) {
    for(unsigned k=0;k<5;++k)REQUIRE(nodes->GetSolution(p,k)==before[p][k]);
    const auto& u=before[p];const double momentum2=u[1]*u[1]+u[2]*u[2]+u[3]*u[3];
    const double pressure=(SU2_TYPE::GetValue(driver.Config().GetGamma())-1)*(u[4]-.5*momentum2/u[0]);REQUIRE(u[0]>0);REQUIRE(pressure>0);
    const double mach=std::sqrt(momentum2/(u[0]*SU2_TYPE::GetValue(driver.Config().GetGamma())*pressure));
    maximumMachError=std::max(maximumMachError,std::abs(mach-SU2_TYPE::GetValue(nodes->GetAuxVar_Adapt(p,0))));
  }
  maximumMachError=CPassiveComm::Allreduce(maximumMachError,CPassiveComm::Op::MAX);CHECK(maximumMachError<1e-9);
  CheckSavedProfileAgreement(*family);
  std::ofstream out("saved_cfd_metric_rank"+std::to_string(CPassiveComm::Rank())+".jsonl");out<<std::setprecision(17);
  const auto started=SU2_MPI::Wtime();const auto ceiling=driver.Config().GetAdap_Native_MemoryLimit();CLocalFailure failure;size_t bytes=0;
  try {
    auto checkpoint=SU2NativeTargetCheckpoint3D::Encode(*family,driver.Reference(),driver.Config(),flow.GetMetricComplexityPreBL(),flow.GetMetricComplexityFinal(),flow.GetMetricComplexityBracketed(),ceiling);
    bytes=checkpoint.size();SU2NativeTargetCheckpoint3D::Write("target_rank"+std::to_string(CPassiveComm::Rank())+".bin",checkpoint);
  }catch(const std::exception& e){failure.Set(3,0,e.what());}
  World world;const auto elected=world.elect(failure);if(elected.any)throw std::runtime_error(elected.message);
  out<<"{\"stage\":\"metric_only\",\"provisional\":true,\"complete_checkpoint\":true,\"checkpoint_qualified\":false,\"cfd_advanced\":false"
     <<",\"requested_complexity\":"<<driver.Config().GetAdap_Complexity()<<",\"estimated_complexity\":"<<flow.GetMetricComplexityFinal()
     <<",\"indicator\":"<<family->normalization.estimate.uncertainty<<",\"core\":"<<family->core
     <<",\"tangential_scale\":"<<family->inherited_tangent_scale<<",\"anchor\":"<<family->tangential_anchor
     <<",\"mach_error\":"<<maximumMachError<<",\"owned_nodes\":"<<family->owned<<",\"checkpoint_bytes\":"<<bytes
     <<",\"checkpoint_write_and_join_seconds\":"<<SU2_MPI::Wtime()-started<<"}\n";
  for(const auto& node:family->nodes) {
    if(node.local>=family->owned)continue;
    out<<"{\"id\":"<<node.id<<",\"x\":["<<node.point.x<<","<<node.point.y<<","<<node.point.z<<"],\"core_tensor\":[";
    const auto& m=family->selected.at(node.local);for(unsigned i=0,k=0;i<3;++i)for(unsigned j=i;j<3;++j)out<<(k++?",":"")<<m.m[i][j];
    out<<"],\"mach\":"<<nodes->GetAuxVar_Adapt(node.local,0)<<"}\n";
  }
  // Save the complete target BEFORE this independently bounded diagnostic so failures cannot discard the metric stage.
  out.flush();REQUIRE(bool(out));const auto chord=SavedTargetChord(driver,*family,out);
  std::ofstream witness("target_witness_rank"+std::to_string(CPassiveComm::Rank())+".txt");witness<<std::setprecision(17)<<chord<<'\n';witness.close();REQUIRE(bool(witness));
  out.close();REQUIRE(bool(out));driver.Finalize();
}

TEST_CASE("Saved native target reloads in a fresh process without recomputing CFD or metric", "[.][NativeSavedTargetReload3D]") {
  const auto* path=std::getenv("SU2_NATIVE_FROZEN_CONFIG");const auto* directory=std::getenv("SU2_NATIVE_FROZEN_SENSOR3D");REQUIRE(path);REQUIRE(directory);
  std::string config(path);SavedNativeMetricDriver driver(config.data(),1,SU2_MPI::GetComm());World world;
  auto& flow=driver.Flow();auto& geometry=driver.Geometry();auto& reference=driver.Reference();auto* nodes=flow.GetNodes();
  std::vector<std::array<double,5>> before(geometry.GetnPointDomain());
  for(size_t p=0;p<before.size();++p)for(unsigned k=0;k<5;++k)before[p][k]=SU2_TYPE::GetValue(nodes->GetSolution(p,k));
  const auto prefix=std::string(directory)+"/target_rank"+std::to_string(world.rank);SU2NativeTargetCheckpoint3D::Checkpoint checkpoint;CLocalFailure failure;
  const auto loadStart=world.seconds();
  try {checkpoint=SU2NativeTargetCheckpoint3D::Read(prefix+".bin",reference,driver.Config(),driver.Config().GetAdap_Native_MemoryLimit());}
  catch(const std::exception& e){failure.Set(3,0,e.what());}
  const auto elected=world.elect(failure);if(elected.any)throw std::runtime_error(elected.message);
  const double loadSeconds=world.seconds()-loadStart;
  flow.RestoreNativeMetricFamily3D(&geometry,driver.Config(),reference,checkpoint.family,checkpoint.pre_complexity,checkpoint.final_complexity,checkpoint.bracketed);
  const auto family=flow.GetNativeMetricFamily3D();REQUIRE(family);CheckSavedProfileAgreement(*family);
  double maximumRelative=0;uint64_t exact=0,count=0;const auto queryStart=world.seconds();
  uint64_t geometryWork=0;failure=CLocalFailure{};
  try {
    auto budget=SavedTargetBudget(driver,*family);auto lease=family->first_row->ReserveGeometry(budget);
  for(const auto& node:family->nodes)if(node.local<family->owned) {
    const auto actual=family->first_row->Composition()(node.point,SU2NativeBLPipeline3D::TargetPolicy::Pack(family->selected.at(node.local)));
    const auto expected=SU2NativeBLPipeline3D::TargetPolicy::Pack(family->composed_witnesses.at(node.local));
    const double a[]={actual.xx,actual.xy,actual.xz,actual.yy,actual.yz,actual.zz},b[]={expected.xx,expected.xy,expected.xz,expected.yy,expected.yz,expected.zz};
    long double difference=0,norm=0;bool same=true;for(unsigned j=0;j<6;++j){difference+=std::pow((long double)a[j]-b[j],2);norm+=std::pow((long double)b[j],2);same&=a[j]==b[j];}
    maximumRelative=std::max(maximumRelative,double(std::sqrt(difference/norm)));exact+=same;++count;
  }
    geometryWork=budget.work;
  }catch(const std::exception& e){failure.Set(3,0,e.what());}
  const auto queryFailure=world.elect(failure);if(queryFailure.any)throw std::runtime_error(queryFailure.message);
  const auto querySeconds=world.seconds()-queryStart;const double globalRelative=CPassiveComm::Allreduce(maximumRelative,CPassiveComm::Op::MAX);
  CHECK(globalRelative<1e-9);
  for(size_t p=0;p<before.size();++p)for(unsigned k=0;k<5;++k)REQUIRE(nodes->GetSolution(p,k)==before[p][k]);
  std::ofstream out("saved_target_reload_rank"+std::to_string(world.rank)+".jsonl");out<<std::setprecision(17);
  out<<"{\"stage\":\"fresh_process_reload\",\"complete_checkpoint\":true,\"cfd_advanced\":false,\"metric_recomputed\":false"
     <<",\"load_seconds\":"<<loadSeconds<<",\"owned_queries\":"<<count<<",\"bit_identical\":"<<exact
     <<",\"maximum_tensor_relative_difference\":"<<maximumRelative<<",\"geometry_work\":"<<geometryWork<<",\"query_and_join_seconds\":"<<querySeconds<<"}\n";
  double expected=0;std::ifstream witness(std::string(directory)+"/target_witness_rank"+std::to_string(world.rank)+".txt");witness>>expected;REQUIRE(bool(witness));REQUIRE(expected>=0);
  const auto actual=SavedTargetChord(driver,*family,out);
  CHECK(std::abs(actual-expected)<=2e-9*expected+2e-12);
  out.close();REQUIRE(bool(out));driver.Finalize();
}

TEST_CASE("Saved complete target drives the existing native MPI surface stage without CFD advancement", "[.][NativeSavedTargetSurfaceStage3D]") {
  const auto* path=std::getenv("SU2_NATIVE_FROZEN_CONFIG");const auto* directory=std::getenv("SU2_NATIVE_FROZEN_SENSOR3D");REQUIRE(path);REQUIRE(directory);
  std::string configPath(path);SavedNativeMetricDriver driver(configPath.data(),1,SU2_MPI::GetComm());World world;
  auto& config=driver.Config();auto& flow=driver.Flow();auto& geometry=driver.Geometry();auto& reference=driver.Reference();
  const auto ceiling=config.GetAdap_Native_MemoryLimit();CLocalFailure failure;SU2NativeTargetCheckpoint3D::Checkpoint checkpoint;
  std::vector<std::array<double,5>> before(geometry.GetnPointDomain());
  for(size_t p=0;p<before.size();++p)for(unsigned k=0;k<5;++k)before[p][k]=SU2_TYPE::GetValue(flow.GetNodes()->GetSolution(p,k));
  const auto loadStart=world.seconds();
  try {checkpoint=SU2NativeTargetCheckpoint3D::Read(std::string(directory)+"/target_rank"+std::to_string(world.rank)+".bin",reference,config,ceiling);}
  catch(const std::exception& e){failure.Set(3,0,e.what());}
  auto elected=world.elect(failure);if(elected.any)throw std::runtime_error(elected.message);
  flow.RestoreNativeMetricFamily3D(&geometry,config,reference,checkpoint.family,checkpoint.pre_complexity,checkpoint.final_complexity,checkpoint.bracketed);
  const auto family=flow.GetNativeMetricFamily3D();REQUIRE(family);CheckSavedProfileAgreement(*family);
  const double loadSeconds=world.seconds()-loadStart;const auto policy=family->first_row;
  const auto fullProfile=SU2NativeTargetCheckpoint3D::detail::State(*policy->full),baseProfile=SU2NativeTargetCheckpoint3D::detail::State(*policy->base);
  const double radius=double(policy->A->radius),core=policy->core,tangentScale=policy->inherited_tangent_scale;
  const auto caller=transfer_memory::Add(ReferenceStorageBytes(reference),family->ResidentBytes(),size_t(128)<<20);
  ImportedGeometry imported;BridgeStats importStats;std::string reason;
  const auto importStart=world.seconds();
  const bool importedOK=ImportGeometry(config,geometry,&flow.GetNodes()->GetMetric(),reference,imported,reason,importStats,ceiling,caller);
  if(!importedOK)throw std::runtime_error(reason); // ImportGeometry has already elected its collective failure.
  const double importSeconds=world.seconds()-importStart;
  const auto retained=transfer_memory::Add(caller,OwnedStorageBytes(imported.cells),transfer_memory::Bytes(imported.original));
  EngineOptions options;options.memory_ceiling=ceiling;options.caller_bytes=retained;options.timings=true;
  options.surface_tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());options.wall_heights.tolerance=SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol());
  for(unsigned short w=0;w<config.GetnAdap_BL();++w) {
    const auto& spec=config.GetAdap_BL(w);auto marker=std::find(reference.marker_names.begin(),reference.marker_names.end(),spec.marker);
    if(marker==reference.marker_names.end())failure.Set(3,0,"Saved surface stage lacks requested wall marker.");
    else options.wall_heights[int(marker-reference.marker_names.begin())]=SU2_TYPE::GetValue(spec.firstHeight);
  }
  std::set<Edge> wallEdges;
  for(const auto& face:reference.facets)if(options.wall_heights.count(face.marker))for(unsigned k=0;k<3;++k) {
    Edge edge{face.v[k].id,face.v[(k+1)%3].id};std::sort(edge.begin(),edge.end());wallEdges.insert(edge);
  }
  // Same global surface work/time policy as RunEvent; replicated immutable facets avoid halo-edge overcounting.
  const auto globalCells=CPassiveComm::Allreduce(uint64_t(imported.cells.size()),CPassiveComm::Op::SUM,world.comm);
  const auto configuredWork=size_t(config.GetAdap_Native_Max_Work());
  const auto eventWork=configuredWork?configuredWork:std::max(size_t(4096),transfer_memory::Mul(size_t(globalCells),64));
  options.maximum_work=std::min(eventWork,std::max(size_t(8192),transfer_memory::Mul(wallEdges.size(),4)));
  const double eventSeconds=SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time());
  const double reserve=std::min(.5*eventSeconds,std::max(120.,.1*eventSeconds));options.maximum_seconds=.08*(eventSeconds-reserve);
  options.geometry_workspace_bytes=transfer_memory::Add(std::max(policy->full->GeometryWorkspaceBytes(),policy->base->GeometryWorkspaceBytes()),size_t(16)<<20);
  options.geometry_reservation=[&](SU2NativeGeometry::AtlasBudget& budget){return policy->ReserveGeometry(budget);};
  NativeStageCapture capture;bool captured=false;options.stage_observer_bytes=NativeStageCapture::limit;
  options.stage_observer=[&](const StageProgressView& view){if(std::string(view.stage)=="exit"){capture.Save(view,world.rank,"surface");captured=true;}};
  elected=world.elect(failure);if(elected.any)throw std::runtime_error(elected.message);
  const auto started=world.seconds();
  const auto prepared=PrepareSurface(world,imported.cells,imported.original,*reference.original,
      [&](Node node,const PlanarReference& g,Tensor sensor){return policy->BoundaryGuidance(node,g,sensor,reference.marker_names);},options);
  const double stageSeconds=world.seconds()-started;std::set<Id> points;uint64_t positive=0,sensors=0;
  failure=CLocalFailure{};
  for(const auto& row:prepared.cells) {
    const auto& v=row.second.cell.v;
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0)++positive;
    sensors+=row.second.sensor_present;for(auto n:v)points.insert(n.id);
  }
  if(!captured||positive!=prepared.cells.size()||sensors!=prepared.cells.size())failure.Set(3,0,"Surface stage lacks complete valid captured current nodal state.");
  for(size_t p=0;p<before.size();++p)for(unsigned k=0;k<5;++k)if(SU2_TYPE::GetValue(flow.GetNodes()->GetSolution(p,k))!=before[p][k])failure.Set(3,0,"Surface stage changed the saved CFD seed.");
  if(!(SU2NativeTargetCheckpoint3D::detail::State(*policy->full)==fullProfile)||!(SU2NativeTargetCheckpoint3D::detail::State(*policy->base)==baseProfile)||double(policy->A->radius)!=radius||policy->core!=core||policy->inherited_tangent_scale!=tangentScale)
    failure.Set(3,0,"Surface stage changed the saved target profile.");
  elected=world.elect(failure);if(elected.any)throw std::runtime_error(elected.message);
  const auto& s=prepared.stats;std::ofstream out("saved_target_surface_rank"+std::to_string(world.rank)+".jsonl");out<<std::setprecision(17)<<std::boolalpha;
  out<<"{\"rank\":"<<world.rank<<",\"ranks\":"<<world.size<<",\"surface_prepared\":"<<prepared.surface_prepared<<",\"final_mesh_accepted\":false,\"fresh_reload_qualified\":false"
     <<",\"cfd_advanced\":false,\"metric_recomputed\":false,\"target_policy_unchanged\":true,\"load_seconds\":"<<loadSeconds<<",\"import_seconds\":"<<importSeconds
     <<",\"stage_seconds\":"<<stageSeconds<<",\"maximum_seconds\":"<<options.maximum_seconds<<",\"maximum_work\":"<<options.maximum_work
     <<",\"local_nodes\":"<<points.size()<<",\"local_tets\":"<<prepared.cells.size()<<",\"positive_cells\":"<<positive<<",\"transported_core_cells\":"<<sensors
     <<",\"global_work\":"<<s.global_work<<",\"local_commits\":"<<s.accepted<<",\"collapse_passes\":"<<s.length_collapse_passes<<",\"split_passes\":"<<s.length_split_passes
     <<",\"geometry_work\":"<<s.atlas_work<<",\"cached_measured_cells\":"<<s.cached_measured_cells<<",\"field_requests\":"<<s.field_requests<<",\"field_compositions\":"<<s.field_compositions
     <<",\"surface_demand_required\":"<<s.surface_demand_required_cells<<",\"surface_demand_measured\":"<<s.surface_demand_measured_cells
     <<",\"surface_demand_complete\":"<<s.surface_demand_census_complete<<",\"surface_geometry_complete\":"<<s.surface_demand_geometry_complete
     <<",\"seed_cell_classes\":["<<s.seed_cell_classes[0]<<','<<s.seed_cell_classes[1]<<','<<s.seed_cell_classes[2]
     <<"],\"updated_cell_classes\":["<<s.updated_cell_classes[0]<<','<<s.updated_cell_classes[1]<<','<<s.updated_cell_classes[2]<<']'
     <<",\"process_cpu_seconds\":"<<s.process_cpu_seconds<<",\"metric_cpu_seconds\":"<<s.metric_cpu_seconds<<",\"reconstruction_cpu_seconds\":"<<s.reconstruction_cpu_seconds
     <<",\"seed_seconds\":"<<s.seed_seconds<<",\"metric_seconds\":"<<s.metric_seconds<<",\"reconstruction_seconds\":"<<s.reconstruction_seconds
     <<",\"tracked_collective_seconds\":"<<s.tracked_collective_seconds<<",\"tracked_calls\":"<<s.tracked_calls<<",\"maximum_work_bytes\":"<<s.maximum_work_bytes
     <<",\"surface_measurement_skipped\":"<<s.surface_measurement_skipped
     <<",\"updated_cells\":"<<s.updated_cells
     <<",\"update_seconds\":"<<s.update_seconds
     <<",\"measurement_cpu_seconds\":"<<s.measurement_cpu_seconds
     <<",\"donor_seconds\":"<<s.donor_seconds
     <<",\"edge_evaluations\":"<<s.edge_evaluations
     <<",\"edge_reused\":"<<s.edge_reused
     <<",\"directory_elections\":"<<s.directory_elections<<",\"directory_election_seconds\":"<<s.directory_election_seconds
     <<",\"geometry_guidance_trials\":"<<s.geometry_guidance_trials
     <<",\"geometry_guidance_seconds\":"<<s.geometry_guidance_seconds
     <<",\"geometry_guidance_faces\":"<<s.geometry_guidance_faces
     <<",\"geometry_guidance_rejected\":"<<s.geometry_guidance_rejected
     <<",\"geometry_guidance_early_rejections\":"<<s.geometry_guidance_early_rejections
     <<",\"collapse_corrections\":"<<s.collapse_corrections
     <<",\"collapse_correction_cells\":"<<s.collapse_correction_cells
     <<",\"collapse_correction_work\":"<<s.collapse_correction_work
     <<",\"collapse_correction_seconds\":"<<s.collapse_correction_seconds
     <<",\"reason\":"<<std::quoted(prepared.reason)<<"}\n";out.close();
  failure=CLocalFailure{};if(!out)failure.Set(3,0,"Cannot save nativeMPI surface stage receipt.");
  elected=world.elect(failure);if(elected.any)throw std::runtime_error(elected.message);
  driver.Finalize();
}
