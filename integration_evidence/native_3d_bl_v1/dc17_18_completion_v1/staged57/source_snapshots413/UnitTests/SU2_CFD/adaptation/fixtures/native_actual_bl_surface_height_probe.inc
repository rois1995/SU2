// Diagnostic only: complete-star edits of one actual wall region, never a production readiness certificate.
std::set<Face> descendants{key},feasibleFaces;size_t surfaceEdits=0,readyFaces=0;
auto composedShapes=[&](const Patch& patch) {
  std::array<size_t,3> count{};for(const auto& cell:patch.cells) {
    const auto shape=dc9::Shape(*state,cell);
    if(!(shape.q>0&&shape.J>0)){++count[2];continue;}
    count[0]+=shape.q<MinimumMeanRatio;count[1]+=shape.J<MinimumScaledJacobian;
  }return count;
};
std::string surfaceOutcome="split_limit";
for(unsigned generation=0;generation<=16;++generation) {
  budget.CheckTime();readyFaces=0;feasibleFaces.clear();Face worst{};double worstMiss=-1;
  for(const auto& child:descendants) {
    const auto result=physicalProposals(child,generation);readyFaces+=result.first;
    if(result.first)feasibleFaces.insert(child);
    if(!result.first&&result.second>worstMiss){worst=child;worstMiss=result.second;}
  }
  frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_height_window\",\"generation\":"<<generation
          <<",\"descendants\":"<<descendants.size()<<",\"feasible_children\":"<<readyFaces
          <<",\"has_measured_plane_miss\":"<<std::isfinite(worstMiss)<<",\"worst_sampled_plane_miss\":"
          <<(std::isfinite(worstMiss)?worstMiss:-1)<<",\"mesh_published\":false}"<<std::endl;
  if(readyFaces==descendants.size()){surfaceOutcome="sampled_children_feasible";break;}
  if(generation==16)break;
  const auto workAt=budget.work;const auto revision=state->boundary_revision;
  directory.reset(); // Shared production feedback builds contact against the current private mesh.
  auto feedback=dc15::PrepareFirstRowSurface(*state,completionRows,world,worst,
      SU2_TYPE::GetValue(config.GetAdap_Hausd()),std::max(0.,300.-std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count()),3);
  const bool changed=feedback.changed;const auto split=feedback.split.edge;const auto newNode=feedback.split.point.id;
  if(changed) {
    std::set<Face> expected=descendants;
    for(const auto& child:descendants)if(std::find(child.begin(),child.end(),split[0])!=child.end()&&
        std::find(child.begin(),child.end(),split[1])!=child.end()) {
      expected.erase(child);auto left=child,right=child;
      std::replace(left.begin(),left.end(),split[0],newNode);std::replace(right.begin(),right.end(),split[1],newNode);
      expected.insert(dc9::Sorted(left));expected.insert(dc9::Sorted(right));
    }
    descendants=std::move(expected);++surfaceEdits;
    REQUIRE(state->boundary_revision>revision);REQUIRE(state->direction_revision==state->boundary_revision);
    for(const auto& child:descendants)REQUIRE(state->wall_index.count(child));
    const auto reportBytes=transfer_memory::Add(transfer_memory::Mul(feedback.old.cells.size()+feedback.fresh.cells.size(),32768),
        transfer_memory::Mul(6,transfer_memory::Add(BoundaryCoverageBytes(feedback.old.coverage),BoundaryCoverageBytes(feedback.fresh.coverage))));
    state->admit_scratch(reportBytes);budget.caller_bytes=transfer_memory::Add(retained,reportBytes);
    describe(label,"surface_height_split_old",feedback.old);describe(label,"surface_height_split_fresh",feedback.fresh);
    const auto oldShape=composedShapes(feedback.old),freshShape=composedShapes(feedback.fresh);
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_height_split_shapes\",\"generation\":"<<generation
            <<",\"old_q_bad\":"<<oldShape[0]<<",\"old_J_bad\":"<<oldShape[1]<<",\"old_unknown\":"<<oldShape[2]
            <<",\"fresh_q_bad\":"<<freshShape[0]<<",\"fresh_J_bad\":"<<freshShape[1]<<",\"fresh_unknown\":"<<freshShape[2]
            <<",\"composed_control_target_only\":true,\"shape_used_for_admission\":false}"<<std::endl;
  }
  frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_height_split\",\"generation\":"<<generation
          <<",\"edge\":["<<split[0]<<','<<split[1]<<"],\"constructed\":"<<changed<<",\"applied_private\":"<<changed
          <<",\"geometry_pass\":"<<changed<<",\"passing_rows_preserved\":"<<changed<<",\"external_clearance\":"<<changed
          <<",\"old_cells\":"<<feedback.old.cells.size()<<",\"fresh_cells\":"<<feedback.fresh.cells.size()<<",\"reason\":"<<std::quoted(feedback.why)
          <<",\"seconds\":"<<feedback.seconds<<",\"geometry_work\":"<<budget.work-workAt<<",\"native_work\":"<<feedback.work
          <<",\"geometry_guidance_before\":"<<double(feedback.geometry_before)<<",\"geometry_guidance_after\":"<<double(feedback.geometry_after)
          <<",\"shared_production_feedback\":true,\"mesh_published\":false}"<<std::endl;
  feedback.old={};feedback.fresh={};budget.caller_bytes=retained;
  directory=std::make_unique<Directory>(world);std::string rebuildReason;
  REQUIRE(directory->Build(state->mesh,rebuildReason,ceiling,retained));
  budget.caller_bytes=transfer_memory::Add(retained,directory->ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
  if(state->publication_failed)throw std::runtime_error("Surface-height diagnostic private publication failed.");
  if(!changed){surfaceOutcome="all_existing_splits_refused";break;}
}
frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_height_result\",\"splits\":"<<surfaceEdits
        <<",\"descendants\":"<<descendants.size()<<",\"feasible_children\":"<<readyFaces<<",\"outcome\":"<<std::quoted(surfaceOutcome)
        <<",\"coherent_front_certified\":false,\"production_target_certified\":false,\"mesh_published\":false}"<<std::endl;
#include "native_actual_bl_prepared_row_probe.inc"
