// Resume exact transported state through the public production stage; do not re-import or refit its metric.
REQUIRE(coherentCapture);REQUIRE(restored.stage.event==0);REQUIRE(config.GetnAdap_BL()>0);
REQUIRE(policy.h0==SU2_TYPE::GetValue(config.GetAdap_BL(0).firstHeight));
const auto payloadBytes=current.live_parameters->PayloadBytes();
const auto donorBytes=transfer_memory::GrowthBound(current.mesh.size(),sizeof(DonorCell));
const auto outsideState=retained-OwnedStorageBytes(current.mesh);
const auto captureCaller=transfer_memory::Add(outsideState,transfer_memory::Mul(2,payloadBytes),NativeStageCapture::streaming_reserve);
REQUIRE_FALSE(world.maximum(transfer_memory::Add(SU2NativeBLPipeline3D::detail::StateBytes(current,captureCaller),donorBytes)>ceiling));
const auto originalParameters=current.live_parameters->Payload();
std::vector<DonorCell> donors;donors.reserve(current.mesh.size());
for(const auto& row:current.mesh) {
  DonorCell donor;donor.cell=row.second.cell;donor.sensor=row.second.sensor;donors.push_back(donor);
}
const auto fullStamp=SU2NativeTargetCheckpoint3D::detail::State(*policy.full);
const auto baseStamp=SU2NativeTargetCheckpoint3D::detail::State(*policy.base);
const auto initialCore=policy.core,initialTangent=policy.inherited_tangent_scale;
const auto& authority=current.live_parameters->Geometry();const auto geometryBefore=authority.GeometryStatsSnapshot();
EngineOptions options;const double eventSeconds=SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time());
const double reserve=std::min(.5*eventSeconds,std::max(120.,.1*eventSeconds));
options.maximum_seconds=.08*(eventSeconds-reserve);
// Retain the established production-stage bounds, independently of the diagnostic prefix's 24-call cap.
options.maximum_work=config.GetAdap_Native_Max_Work()?std::min<size_t>(233220,config.GetAdap_Native_Max_Work()):233220;
options.memory_ceiling=ceiling;
options.caller_bytes=transfer_memory::Add(SU2NativeBLPipeline3D::detail::StateBytes(current,
    transfer_memory::Add(outsideState,transfer_memory::Mul(2,originalParameters.capacity()),NativeStageCapture::streaming_reserve)),transfer_memory::Bytes(donors));
options.surface_tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());options.wall_heights=heights;
for(unsigned short w=0;w<config.GetnAdap_BL();++w) {
  const auto& spec=config.GetAdap_BL(w);const auto marker=std::find(reference->marker_names.begin(),reference->marker_names.end(),spec.marker);
  REQUIRE(marker!=reference->marker_names.end());options.region_walls[int(marker-reference->marker_names.begin())]=
      {SU2_TYPE::GetValue(spec.firstHeight),SU2_TYPE::GetValue(spec.growth),SU2_TYPE::GetValue(spec.thickness)};
}
options.shared_geometry_budget=&budget;
options.geometry_workspace_bytes=transfer_memory::Add(std::max(policy.full->GeometryWorkspaceBytes(),policy.base->GeometryWorkspaceBytes()),atlasScratch);
options.geometry_reservation=[&](SU2NativeGeometry::AtlasBudget& currentBudget){return policy.ReserveGeometry(currentBudget);};
options.timings=true;options.stage_observer_bytes=NativeStageCapture::limit;
NativeStageCapture stageCapture;bool captured=false;
options.stage_observer=[&](const StageProgressView& view) {
  const bool atExit=std::string(view.stage)=="exit";
  if(atExit)stageCapture.rejected_saved=true; // Keep the existing census trace, with one explicitly named final stage capture.
  stageCapture.Observe(view,world.rank);
  if(atExit){stageCapture.Save(view,world.rank,"resumed_surface");captured=true;}
};
#include "native_actual_surface_target_observation.inc"
if(nosePath)options.caller_bytes=transfer_memory::Add(options.caller_bytes,noseBytes);
REQUIRE_FALSE(world.maximum(!(options.maximum_seconds>0)||options.caller_bytes>ceiling));
noseObserve(current.mesh,*current.reference,"before");
if(nosePath)options.rejected_trial=[&](const Patch& old,const Patch& fresh,Feature feature,const MetricDiagnosis& diagnosis,bool candidate,const std::string& why) {
  noseLog<<"{\"stage\":\"refusal\",\"feature\":["<<feature.vertices[0]<<','<<feature.vertices[1]<<','<<feature.vertices[2]
      <<"],\"feature_count\":"<<feature.count<<",\"old_cells\":"<<old.cells.size()<<",\"fresh_cells\":"<<fresh.cells.size()
      <<",\"candidate\":"<<candidate<<",\"failure\":"<<unsigned(diagnosis.failure)<<",\"reason\":"<<std::quoted(why)<<"}"<<std::endl;
};
const auto start=std::chrono::steady_clock::now();const auto beforeWork=budget.work;
const auto prepared=PrepareSurface(world,current.mesh,donors,*current.reference,
    [&](Node node,const PlanarReference& geometry,Tensor sensor){return policy.BoundaryGuidance(node,geometry,sensor,reference->marker_names);},options);
const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
if(nosePath) {
  // Published split records preserve parent endpoints even when a later edit retires them.
  std::map<Id,Point> positions;for(const auto& row:current.mesh)for(auto n:row.second.cell.v)positions.emplace(n.id,n.p);
  for(const auto& split:prepared.splits) {
    const auto a=positions.find(split.edge[0]),b=positions.find(split.edge[1]);
    noseLog<<"{\"stage\":\"accepted_split\",\"edge\":["<<split.edge[0]<<','<<split.edge[1]<<"],\"point\":["<<split.point.id<<','<<split.point.p.x<<','<<split.point.p.y<<','<<split.point.p.z<<"],\"endpoints_known\":"<<(a!=positions.end()&&b!=positions.end());
    if(a!=positions.end()&&b!=positions.end())noseLog<<",\"a\":["<<a->second.x<<','<<a->second.y<<','<<a->second.z<<"],\"b\":["<<b->second.x<<','<<b->second.y<<','<<b->second.z<<']';
    noseLog<<"}"<<std::endl;positions[split.point.id]=split.point.p;
  }
  noseLog<<"{\"stage\":\"split_summary\",\"accepted_records\":"<<prepared.splits.size()<<",\"accepted_stats\":"<<prepared.stats.length_splits<<",\"total_rejected\":"<<prepared.stats.rejected<<"}"<<std::endl;
  positions.clear();noseObserve(prepared.cells,*prepared.reference,"after");
}
const bool samePolicy=SU2NativeTargetCheckpoint3D::detail::State(*policy.full)==fullStamp&&
    SU2NativeTargetCheckpoint3D::detail::State(*policy.base)==baseStamp&&policy.core==initialCore&&policy.inherited_tangent_scale==initialTangent;
const bool inputParametersUnchanged=current.live_parameters->Payload()==originalParameters;
CHECK(samePolicy);CHECK(inputParametersUnchanged);CHECK_FALSE(prepared.valid);
const auto geometryAfter=authority.GeometryStatsSnapshot();
size_t positive=0,sensors=0;
for(const auto& row:prepared.cells) {
  const auto& v=row.second.cell.v;REQUIRE(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);++positive;
  sensors+=row.second.sensor_present;
}
const auto globalCells=CPassiveComm::Allreduce(uint64_t(prepared.cells.size()),CPassiveComm::Op::SUM,world.comm);
std::ofstream output("actual_saved_surface_stage_rank"+std::to_string(world.rank)+".json");REQUIRE(bool(output));
output<<std::setprecision(17)<<std::boolalpha<<"{\"rank\":"<<world.rank<<",\"ranks\":"<<world.size
    <<",\"input_cells\":"<<current.mesh.size()<<",\"donor_cells\":"<<donors.size()<<",\"output_cells\":"<<prepared.cells.size()
    <<",\"global_output_cells\":"<<globalCells<<",\"positive_cells\":"<<positive<<",\"sensor_cells\":"<<sensors
    <<",\"surface_prepared\":"<<prepared.surface_prepared<<",\"engine_valid\":"<<prepared.valid<<",\"final_mesh_accepted\":false,\"same_target_policy\":"<<samePolicy
    <<",\"input_parameters_unchanged\":"<<inputParametersUnchanged
    <<",\"exact_transported_input\":true,\"captured\":"<<captured<<",\"stage_height\":"<<policy.h0<<",\"seconds\":"<<seconds
    <<",\"maximum_seconds\":"<<options.maximum_seconds<<",\"maximum_work\":"<<options.maximum_work
    <<",\"stop\":"<<std::quoted(EngineStopName(prepared.stats.stop))<<",\"reason\":"<<std::quoted(prepared.reason)
    <<",\"work\":"<<prepared.stats.global_work<<",\"commits\":"<<prepared.stats.accepted<<",\"moved\":"<<prepared.stats.moved
    <<",\"reconnected\":"<<prepared.stats.reconnected<<",\"splits\":"<<prepared.stats.length_splits<<",\"collapses\":"<<prepared.stats.length_collapses
    <<",\"geometry_work\":"<<budget.work-beforeWork<<",\"process_cpu_seconds\":"<<prepared.stats.process_cpu_seconds
    <<",\"ownership_evaluations\":"<<prepared.stats.ownership_evaluations<<",\"ownership_changes\":"<<prepared.stats.ownership_changes
    <<",\"ownership_moved_cells\":"<<prepared.stats.ownership_moved_cells<<",\"geometry_guidance_trials\":"<<prepared.stats.geometry_guidance_trials
    <<",\"geometry_guidance_rejected\":"<<prepared.stats.geometry_guidance_rejected
    <<",\"surface_required\":"<<prepared.stats.surface_demand_required_cells<<",\"surface_measured\":"<<prepared.stats.surface_demand_measured_cells
    <<",\"surface_census_complete\":"<<prepared.stats.surface_demand_census_complete<<",\"surface_geometry_complete\":"<<prepared.stats.surface_demand_geometry_complete
    <<",\"upper_calls\":"<<geometryAfter.upper_calls-geometryBefore.upper_calls<<",\"upper_work\":"<<geometryAfter.upper_work-geometryBefore.upper_work
    <<",\"upper_seconds\":"<<geometryAfter.upper_seconds-geometryBefore.upper_seconds<<",\"scope_work\":"<<geometryAfter.scope_build_work-geometryBefore.scope_build_work
    <<",\"scope_seconds\":"<<geometryAfter.scope_build_seconds-geometryBefore.scope_build_seconds
    <<",\"facet_work\":"<<geometryAfter.facet_build_work-geometryBefore.facet_build_work<<",\"traversal_work\":"<<geometryAfter.traversal_work-geometryBefore.traversal_work
    <<",\"tracked_collective_seconds\":"<<prepared.stats.tracked_collective_seconds<<",\"ownership_seconds\":"<<prepared.stats.ownership_seconds
    <<",\"maximum_work_bytes\":"<<prepared.stats.maximum_work_bytes<<",\"timers_overlap\":true,\"cpu_includes_MPI_polling\":true}"<<std::endl;
REQUIRE(bool(output));REQUIRE_FALSE(world.maximum(!captured||sensors!=prepared.cells.size()||!prepared.parameters||!prepared.reference));
