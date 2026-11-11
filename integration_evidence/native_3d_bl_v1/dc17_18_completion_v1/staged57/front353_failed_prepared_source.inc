// Reuse the existing coordinated transaction only where every incident wall face has a current feasible child.
Id seed=UINT64_MAX;size_t seedCells=SIZE_MAX;std::set<Id> vertices;
for(const auto& f:descendants)vertices.insert(f.begin(),f.end());
for(Id vertex:vertices) {
  if(vertex<nextNode||dc9::Rim(*state,vertex))continue;
  const auto star=StarOf(*state,vertex);if(star.empty()||star.size()>MaximumCavityCells)continue;
  const auto walls=PhysicalOf(*state,star,true);bool ready=!walls.empty();
  for(const auto& wall:walls)if(std::find(wall.key.begin(),wall.key.end(),vertex)!=wall.key.end())
    ready=ready&&feasibleFaces.count(wall.key);
  if(ready&&std::make_pair(star.size(),vertex)<std::make_pair(seedCells,seed)){seed=vertex;seedCells=star.size();}
}
if(seed==UINT64_MAX) {
  frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"prepared_row_result\",\"attempted\":false,"
          <<"\"reason\":\"No new vertex has a completely feasible incident wall star.\",\"mesh_published\":false}"<<std::endl;
} else {
  const auto start=std::chrono::steady_clock::now();const auto workAt=budget.work;
  dc9::Rows preparedRows;std::set<Id> advancing{seed};dc9::Tx prepared;bool low=false;
  for(unsigned trial=0;trial<20;++trial) {
    budget.CheckTime();prepared=dc9::Transaction(*state,preparedRows,advancing,low);
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"prepared_row_trial\",\"trial\":"<<trial
            <<",\"seed\":"<<seed<<",\"low_band\":"<<low<<",\"closure_vertices\":"<<advancing.size()
            <<",\"constructed\":"<<prepared.ok<<",\"old_cells\":"<<prepared.old_cells<<",\"fresh_cells\":"<<prepared.fresh_cells
            <<",\"class\":"<<std::quoted(prepared.cls)<<",\"reason\":"<<std::quoted(prepared.witness)<<"}"<<std::endl;
    if(prepared.ok)break;
    if(!low&&(prepared.cls=="partial-state"||prepared.cls=="first-child"||prepared.cls=="clearance"||prepared.cls=="height")){low=true;continue;}
    bool added=false;
    if(prepared.cls=="clearance")for(Id v:prepared.blockers)if(!dc9::Rim(*state,v))added=advancing.insert(v).second||added;
    if((prepared.cls=="partial-state"||prepared.cls=="first-child"||prepared.cls=="height")&&!prepared.failing.empty()) {
      std::sort(prepared.failing.begin(),prepared.failing.end());for(Id v:prepared.failing.front())
        if(!dc9::Rim(*state,v))added=advancing.insert(v).second||added;
    }
    if(!added)break;
  }
  bool contact=false,geometryPass=false,preserved=false,completeHeight=false;size_t columns=0,passingColumns=0;
  FirstHeightReport height;std::string why;std::array<size_t,3> before{},after{};
  if(prepared.ok) {
    auto freshContext=state->reference->PrivateParameters(prepared.fresh,&budget);const auto& ref=freshContext?*freshContext:*state->reference;
    std::map<Id,Point> coordinates;for(const auto& cell:prepared.fresh.cells)for(auto node:cell.v)coordinates.emplace(node.id,node.p);
    geometryPass=true;
    for(const auto& f:prepared.fresh.surface) {
      const std::array<Point,3> triangle{coordinates.at(f.v[0]),coordinates.at(f.v[1]),coordinates.at(f.v[2])};
      geometryPass=geometryPass&&ref.ContainsFace(f.facet,triangle)&&ref.SurfaceError(f.facet,triangle)<=SU2_TYPE::GetValue(config.GetAdap_Hausd());
    }
    preserved=PreservePassingFirstHeights(prepared.old,prepared.fresh,heights,*state->reference,why);
    completeHeight=CheckFirstHeight(prepared.fresh,heights,ref,height,why);
    for(const auto& entry:prepared.apices) {
      ++columns;Patch child;child.parameters=prepared.fresh.parameters;
      for(const auto& f:prepared.fresh.surface)if(dc9::Sorted(f.v)==entry.first)child.surface.push_back(f);
      for(const auto& cell:prepared.fresh.cells) {
        unsigned count=0;for(auto node:cell.v)count+=std::find(entry.first.begin(),entry.first.end(),node.id)!=entry.first.end();
        if(count==3)child.cells.push_back(cell);
      }
      FirstHeightReport measured;std::string reason;passingColumns+=CheckFirstHeight(child,heights,ref,measured,reason);
    }
    contact=true;REQUIRE(directory->Clearance(state->mesh,prepared.old,prepared.fresh,contact,why,ceiling,retained));
    before=composedShapes(prepared.old);after=composedShapes(prepared.fresh);
    describe(label,"prepared_row_old",prepared.old);describe(label,"prepared_row_fresh",prepared.fresh);
  }
  frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"prepared_row_result\",\"attempted\":true,\"seed\":"<<seed
          <<",\"constructed\":"<<prepared.ok<<",\"geometry_pass\":"<<geometryPass<<",\"passing_rows_preserved\":"<<preserved
          <<",\"external_clearance\":"<<contact<<",\"complete_patch_height\":"<<completeHeight<<",\"columns\":"<<columns
          <<",\"passing_columns\":"<<passingColumns<<",\"old_q_bad\":"<<before[0]<<",\"old_J_bad\":"<<before[1]<<",\"old_unknown\":"<<before[2]
          <<",\"fresh_q_bad\":"<<after[0]<<",\"fresh_J_bad\":"<<after[1]<<",\"fresh_unknown\":"<<after[2]
          <<",\"shape_measured\":"<<prepared.ok<<",\"composed_control_target_only\":true,\"class\":"<<std::quoted(prepared.cls)
          <<",\"reason\":"<<std::quoted(prepared.witness)<<",\"check_reason\":"<<std::quoted(why)
          <<",\"geometry_work\":"<<budget.work-workAt<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
          <<",\"accepted\":false,\"production_target_certified\":false,\"mesh_published\":false}"<<std::endl;
}
