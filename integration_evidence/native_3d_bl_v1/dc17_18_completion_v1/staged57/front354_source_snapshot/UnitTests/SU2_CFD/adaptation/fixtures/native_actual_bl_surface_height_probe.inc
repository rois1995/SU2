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
  const auto ownerAt=state->wall_index.find(worst);REQUIRE(ownerAt!=state->wall_index.end());
  auto parent=PatchOf(*state,{ownerAt->second});auto priv=state->reference->PrivateParameters(parent,&budget);
  const auto& context=priv?*priv:*state->reference;
  const auto parentFace=std::find_if(parent.surface.begin(),parent.surface.end(),[&](const SurfaceFace& f){return dc9::Sorted(f.v)==worst;});
  REQUIRE(parentFace!=parent.surface.end());
  std::vector<std::pair<double,Edge>> candidates;
  for(unsigned k=0;k<3;++k) {
    Edge split{worst[k],worst[(k+1)%3]};std::sort(split.begin(),split.end());
    const auto midpoint=dc10::Mid(NodePoint(*state,split[0]),NodePoint(*state,split[1]));
    const auto projected=context.Project(parentFace->facet,midpoint);REQUIRE(budget.ChargeWork(1));
    candidates.emplace_back(-double(Norm(Of(projected)-Of(midpoint))),split);
  }
  std::sort(candidates.begin(),candidates.end());bool changed=false;
  for(const auto& ranked:candidates) {
    budget.CheckTime();const auto split=ranked.second;const auto ids=Star(*state,split);
    const auto workAt=budget.work;const auto splitStart=std::chrono::steady_clock::now();
    const auto newNode=state->next_node;Patch before,after;std::string why;bool made=false,contact=false,geometryPass=false,preserved=false;
    std::set<Face> expected=descendants;
    try {
      if(ids.empty()||ids.size()>MaximumCavityCells)throw std::length_error("Complete split edge star exceeds existing cavity bounds.");
      before=PatchOf(*state,ids);
      made=SplitEdge(before,split,{newNode,dc10::Mid(NodePoint(*state,split[0]),NodePoint(*state,split[1]))},
          state->next_cell,*state->reference,after,why);
      if(!made)throw std::invalid_argument(why);
      auto freshContext=state->reference->PrivateParameters(after,&budget);const auto& freshReference=freshContext?*freshContext:*state->reference;
      std::map<Id,Point> coordinates;for(const auto& cell:after.cells) {
        if(!dc9::Positive(cell.v))throw std::invalid_argument("Split has a nonpositive cell.");
        for(auto node:cell.v)coordinates.emplace(node.id,node.p);
      }
      const auto tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());geometryPass=true;
      for(const auto& f:after.surface) {
        const std::array<Point,3> points{coordinates.at(f.v[0]),coordinates.at(f.v[1]),coordinates.at(f.v[2])};
        geometryPass=geometryPass&&freshReference.ContainsFace(f.facet,points)&&freshReference.SurfaceError(f.facet,points)<=tolerance;
      }
      if(!geometryPass)throw std::invalid_argument("Split lacks strict whole-patch geometry certification.");
      preserved=PreservePassingFirstHeights(before,after,heights,*state->reference,why);
      if(!preserved)throw std::invalid_argument(why);
      for(const auto& child:descendants)if(std::find(child.begin(),child.end(),split[0])!=child.end()&&
          std::find(child.begin(),child.end(),split[1])!=child.end()) {
        expected.erase(child);auto left=child,right=child;
        std::replace(left.begin(),left.end(),split[0],newNode);std::replace(right.begin(),right.end(),split[1],newNode);
        left=dc9::Sorted(left);right=dc9::Sorted(right);expected.insert(left);expected.insert(right);
        for(const auto& required:{left,right}) {
          const auto actual=std::find_if(after.surface.begin(),after.surface.end(),[&](const SurfaceFace& f){return dc9::Sorted(f.v)==required;});
          if(actual==after.surface.end())throw std::invalid_argument("Split lost expected descendant surface coverage.");
          if(freshReference.Canonical(actual->facet)!=context.Canonical(parentFace->facet)||
              freshReference.Get(actual->facet).marker!=context.Get(parentFace->facet).marker)
            throw std::invalid_argument("Split changed descendant geometry authority or marker.");
        }
      }
      contact=true;if(!directory->Clearance(state->mesh,before,after,contact,why,ceiling,retained)||!contact)
        throw std::invalid_argument("Split external clearance: "+why);
      describe(label,"surface_height_split_old",before);describe(label,"surface_height_split_fresh",after);
      const auto oldShape=composedShapes(before),freshShape=composedShapes(after);const auto revision=state->boundary_revision;
      Apply(*state,ids,after);descendants=std::move(expected);++surfaceEdits;changed=true;
      REQUIRE(state->boundary_revision>revision);dc9::ensureDirectionField(*state);
      REQUIRE(state->direction_revision==state->boundary_revision);
      // Legacy private Apply does not publish additions to Directory's collision index.
      directory.reset();directory=std::make_unique<Directory>(world);
      REQUIRE(directory->Build(state->mesh,why,ceiling,retained));
      for(const auto& child:descendants)REQUIRE(state->wall_index.count(child));
      frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_height_split_shapes\",\"generation\":"<<generation
              <<",\"old_q_bad\":"<<oldShape[0]<<",\"old_J_bad\":"<<oldShape[1]<<",\"old_unknown\":"<<oldShape[2]
              <<",\"fresh_q_bad\":"<<freshShape[0]<<",\"fresh_J_bad\":"<<freshShape[1]<<",\"fresh_unknown\":"<<freshShape[2]
              <<",\"composed_control_target_only\":true,\"shape_used_for_admission\":false}"<<std::endl;
    }catch(const std::exception& e){if(changed)throw;why=e.what();}
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_height_split\",\"generation\":"<<generation
            <<",\"edge\":["<<split[0]<<','<<split[1]<<"],\"constructed\":"<<made<<",\"applied_private\":"<<changed
            <<",\"geometry_pass\":"<<geometryPass<<",\"passing_rows_preserved\":"<<preserved<<",\"external_clearance\":"<<contact
            <<",\"old_cells\":"<<before.cells.size()<<",\"fresh_cells\":"<<after.cells.size()<<",\"reason\":"<<std::quoted(why)
            <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-splitStart).count()
            <<",\"geometry_work\":"<<budget.work-workAt<<",\"mesh_published\":false}"<<std::endl;
    if(state->publication_failed)throw std::runtime_error("Surface-height diagnostic private publication failed.");
    if(changed)break;
  }
  if(!changed){surfaceOutcome="all_existing_splits_refused";break;}
}
frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_height_result\",\"splits\":"<<surfaceEdits
        <<",\"descendants\":"<<descendants.size()<<",\"feasible_children\":"<<readyFaces<<",\"outcome\":"<<std::quoted(surfaceOutcome)
        <<",\"coherent_front_certified\":false,\"production_target_certified\":false,\"mesh_published\":false}"<<std::endl;
#include "native_actual_bl_prepared_row_probe.inc"
