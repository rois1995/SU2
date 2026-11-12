// Saved complete target -> existing production W; this is provisional stage evidence, never CFD acceptance.
state->field.reset();state->mesh=imported.cells;
std::vector<DonorCell> surfaceDonors;surfaceDonors.reserve(state->mesh.size());
for(auto& row:state->mesh) {
  row.second.sensor_present=true;for(unsigned k=0;k<4;++k)row.second.sensor[k]=coreAt(row.second.cell.v[k]);
  DonorCell donor;donor.cell=row.second.cell;donor.sensor=row.second.sensor;surfaceDonors.push_back(donor);
}
state->live_parameters=std::make_shared<BoundaryParameterState>(*reference->original->Parameters());
state->reference=scope->WithParameterState(state->live_parameters)->WithAtlasReservation(budget);
Prepare(*state);dc12::ApplyWallPolicies(*state);state->heights.tolerance=heights.tolerance;
dc9::DetectCreases(*state,policy->roles);
std::set<Edge> wallEdges;
for(const auto& face:WallFaces(*state))for(unsigned k=0;k<3;++k) {
  Edge edge{face.second[k],face.second[(k+1)%3]};std::sort(edge.begin(),edge.end());wallEdges.insert(edge);
}
const double eventSeconds=SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time());
const double reserve=std::min(.5*eventSeconds,std::max(120.,.1*eventSeconds));
EngineOptions options;options.maximum_seconds=std::min(.08*(eventSeconds-reserve),eventSeconds-
    std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count());
REQUIRE(options.maximum_seconds>0);
options.maximum_work=std::min(work,std::max<size_t>(8192,4*wallEdges.size()));
options.memory_ceiling=ceiling;options.caller_bytes=retained;
options.surface_tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());options.wall_heights=state->heights;
options.shared_geometry_budget=&budget;
options.geometry_workspace_bytes=transfer_memory::Add(std::max(policy->full->GeometryWorkspaceBytes(),
    policy->base->GeometryWorkspaceBytes()),atlasScratch);
options.geometry_reservation=[&](SU2NativeGeometry::AtlasBudget& current){return policy->ReserveGeometry(current);};
options.timings=true;NativeStageCapture capture;bool captured=false;
options.stage_observer_bytes=NativeStageCapture::limit;
options.stage_observer=[&](const StageProgressView& view) {
  if(std::string(view.stage)=="exit") {capture.Save(view,world.rank,"surface");captured=true;}
};
std::ofstream output("actual_bl_surface_stage.json");REQUIRE(bool(output));output<<std::setprecision(17)<<std::boolalpha;
const auto stageStart=std::chrono::steady_clock::now();const auto workBefore=budget.work;
const auto prepared=PrepareSurface(world,state->mesh,surfaceDonors,*state->reference,
    [&](Node node,const PlanarReference& geometry,Tensor sensor) {
      return policy->BoundaryGuidance(node,geometry,sensor,reference->marker_names);
    },options);
const auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-stageStart).count();
CHECK(SU2NativeTargetCheckpoint3D::detail::State(*policy->full)==initialFullProfile);
CHECK(SU2NativeTargetCheckpoint3D::detail::State(*policy->base)==initialBaseProfile);
CHECK(policy->A->radius==initialRadius);CHECK(policy->core==selectedCore);
CHECK(policy->inherited_tangent_scale==tangentScale);CHECK_FALSE(prepared.valid);
std::set<Id> points;size_t positive=0,sensors=0;
for(const auto& row:prepared.cells) {
  const auto& v=row.second.cell.v;
  REQUIRE(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0);++positive;
  sensors+=row.second.sensor_present;for(auto node:v)points.insert(node.id);
}
output<<"{\"surface_prepared\":"<<prepared.surface_prepared<<",\"final_mesh_accepted\":false,\"fresh_reload_qualified\":false"
      <<",\"reason\":"<<std::quoted(prepared.reason)<<",\"seconds\":"<<elapsed
      <<",\"maximum_seconds\":"<<options.maximum_seconds<<",\"maximum_work\":"<<options.maximum_work
      <<",\"captured\":"<<captured<<",\"nodes\":"<<points.size()<<",\"tets\":"<<prepared.cells.size()
      <<",\"positive_cells\":"<<positive<<",\"transported_core_cells\":"<<sensors
      <<",\"work\":"<<prepared.stats.global_work<<",\"commits\":"<<prepared.stats.accepted
      <<",\"geometry_work\":"<<budget.work-workBefore<<",\"seed_seconds\":"<<prepared.stats.seed_seconds
      <<",\"measurement_cpu_seconds\":"<<prepared.stats.measurement_cpu_seconds
      <<",\"cached_measured_cells\":"<<prepared.stats.cached_measured_cells
      <<",\"edge_evaluations\":"<<prepared.stats.edge_evaluations<<",\"edge_reused\":"<<prepared.stats.edge_reused
      <<",\"field_requests\":"<<prepared.stats.field_requests<<",\"field_compositions\":"<<prepared.stats.field_compositions
      <<",\"target_policy_unchanged\":true,\"source_checkpoint_required\":true}"<<std::endl;
REQUIRE(bool(output));REQUIRE(captured);REQUIRE(sensors==prepared.cells.size());
