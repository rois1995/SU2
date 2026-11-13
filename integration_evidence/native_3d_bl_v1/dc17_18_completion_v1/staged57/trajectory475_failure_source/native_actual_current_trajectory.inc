// Current captured mesh, no legacy construction precursor or replay recipe.
#include "native_actual_surface_trial_context.inc"
const size_t captureBytes=NativeStageCapture::limit+NativeStageCapture::streaming_reserve;
budget.caller_bytes=transfer_memory::Add(held,captureBytes);REQUIRE(budget.caller_bytes<ceiling);
unsigned trajectoryRemaining=24;
auto saveWorking=[&]() {
  NativeStageCapture::CheckPayloadBound(trial.live_parameters->PayloadBytes(),captureBytes);
  const auto payload=trial.live_parameters->Payload();
  NativeStageCapture::WriteOwned("trajectory_working_owned.bin",trial.mesh);
  NativeStageCapture::WriteBytes("trajectory_working_parameters.bin",payload);
  NativeStageCapture::WriteOwned("correction_unchanged_owned.bin",current.mesh);
  REQUIRE(current.live_parameters->Payload()==acceptedParameters);
  log<<"{\"stage\":\"current_trajectory_capture\",\"cells\":"<<trial.mesh.size()
     <<",\"remaining_shared_calls\":"<<trajectoryRemaining<<",\"geometry_work\":"<<budget.work
     <<",\"time_exhausted\":"<<budget.time_exhausted<<",\"work_exhausted\":"<<budget.work_exhausted
     <<",\"final_mesh_accepted\":false}"<<std::endl;
};
try {
  std::ifstream replay("trajectory_replay.dat");REQUIRE_FALSE(bool(replay));
  const auto* path=std::getenv("SU2_NATIVE_BL_CAVITY_CONTEXTS");REQUIRE(path);
  std::ifstream input(path);REQUIRE(bool(input));size_t count=0;input>>count;
  REQUIRE((count>0&&count<=2));
  struct Seed {std::string label;Edge edge;std::set<std::array<Id,4>> cells;};
  std::vector<Seed> seeds;std::set<Edge> unique;
  for(size_t i=0;i<count;++i) {
    Seed seed;size_t n=0;input>>std::setw(128)>>seed.label>>seed.edge[0]>>seed.edge[1]>>n;
    REQUIRE(bool(input));REQUIRE(seed.edge[0]!=seed.edge[1]);REQUIRE((n>0&&n<=MaximumCavityCells));
    std::sort(seed.edge.begin(),seed.edge.end());REQUIRE(unique.insert(seed.edge).second);
    for(size_t k=0;k<n;++k){std::array<Id,4> key;for(auto& id:key)input>>id;REQUIRE(bool(input));
      std::sort(key.begin(),key.end());REQUIRE(std::adjacent_find(key.begin(),key.end())==key.end());REQUIRE(seed.cells.insert(key).second);}
    seeds.push_back(std::move(seed));
  }
  input>>std::ws;REQUIRE(input.eof());
  {
    Directory initial(world,false,trial.live_parameters.get());
    REQUIRE(initial.Build(trial.mesh,reason,ceiling,budget.caller_bytes));
    for(const auto& seed:seeds) {
      const auto deps=initial.Lookup({Feature{{seed.edge[0],seed.edge[1],0},2}},ceiling,budget.caller_bytes);REQUIRE(deps.valid);
      const auto rows=initial.Fetch(deps,trial.mesh,ceiling,budget.caller_bytes);REQUIRE(rows.valid);
      std::set<std::array<Id,4>> actual;
      for(const auto& row:rows.cells){std::array<Id,4> key;for(unsigned k=0;k<4;++k)key[k]=row.cell.v[k].id;
        std::sort(key.begin(),key.end());REQUIRE(actual.insert(key).second);}
      REQUIRE(actual==seed.cells);
    }
  }
  const auto control=junction_wp0::PatchOf(trial,star(smooth));
  const auto controlGeometry=GeometryGuidanceExcess(control,*trial.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
  REQUIRE(controlGeometry.complete);REQUIRE(controlGeometry.excess==0);
  log<<"{\"stage\":\"current_trajectory_control\",\"vertex\":"<<smooth<<",\"cells\":"<<control.cells.size()
     <<",\"geometry_excess\":"<<controlGeometry.excess<<",\"geometry_only_control\":true,\"first_row_qualified\":false}"<<std::endl;
  for(const auto& seed:seeds) {
    budget.CheckTime();
    const size_t caller=transfer_memory::Add(held,captureBytes);
    budget.caller_bytes=caller;
    Directory directory(world,false,trial.live_parameters.get());REQUIRE(directory.Build(trial.mesh,reason,ceiling,caller));
    budget.caller_bytes=transfer_memory::Add(caller,directory.ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
    const auto deps=directory.Lookup({Feature{{seed.edge[0],seed.edge[1],0},2}},ceiling,budget.caller_bytes);REQUIRE(deps.valid);
    const auto rows=directory.Fetch(deps,trial.mesh,ceiling,budget.caller_bytes);REQUIRE(rows.valid);
    if(rows.cells.empty()) {
      log<<"{\"stage\":\"current_trajectory_unreached\",\"context\":"<<std::quoted(seed.label)<<",\"reason\":\"seed edge no longer exists\"}"<<std::endl;continue;
    }
    std::vector<Id> ids;for(const auto& row:rows.cells)ids.push_back(row.cell.id);
    const auto seedPatch=junction_wp0::PatchOf(trial,ids);
    const std::vector<Feature> seedQueries{{{seed.edge[0],0,0},1},{{seed.edge[1],0,0},1}};
    Id trajectoryNode=0,trajectoryCell=0;
    for(const auto& row:trial.mesh){trajectoryCell=std::max(trajectoryCell,row.first);for(auto node:row.second.cell.v)trajectoryNode=std::max(trajectoryNode,node.id);}
    REQUIRE(trajectoryNode<UINT64_MAX-4*24*MaximumReplacementCells);REQUIRE(trajectoryCell<UINT64_MAX-24*MaximumReplacementCells);
    ++trajectoryNode;++trajectoryCell;
    log<<"{\"stage\":\"current_trajectory_context\",\"context\":"<<std::quoted(seed.label)<<",\"edge\":["<<seed.edge[0]<<','<<seed.edge[1]
       <<"],\"current_seed_cells\":"<<rows.cells.size()<<",\"remaining_shared_calls\":"<<trajectoryRemaining<<",\"legacy_precursor\":false}"<<std::endl;
#include "native_actual_surface_patch_trajectory.inc"
    budget.caller_bytes=caller;
  }
} catch(...) {saveWorking();throw;}
saveWorking();
