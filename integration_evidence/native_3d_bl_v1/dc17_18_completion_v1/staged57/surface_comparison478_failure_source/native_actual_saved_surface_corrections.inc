// Fixed coverage compares existing surface freedoms on one saved state, before one publication.
#include "native_actual_surface_trial_context.inc"
const size_t captureBytes=NativeStageCapture::limit+NativeStageCapture::streaming_reserve;
const size_t caller=transfer_memory::Add(held,captureBytes,size_t(32)<<20);
REQUIRE(caller<ceiling);budget.caller_bytes=caller;
unsigned slots=0,calls=0,completed=0;
auto saveWorking=[&]() {
  NativeStageCapture::CheckPayloadBound(trial.live_parameters->PayloadBytes(),captureBytes);
  NativeStageCapture::WriteOwned("trajectory_working_owned.bin",trial.mesh);
  NativeStageCapture::WriteBytes("trajectory_working_parameters.bin",trial.live_parameters->Payload());
  NativeStageCapture::WriteOwned("correction_unchanged_owned.bin",current.mesh);
  REQUIRE(current.live_parameters->Payload()==acceptedParameters);
  log<<"{\"stage\":\"saved_correction_capture\",\"slots\":"<<slots<<",\"constructor_calls\":"<<calls<<",\"contexts_completed\":"<<completed<<",\"geometry_work\":"<<budget.work<<",\"time_exhausted\":"<<budget.time_exhausted<<",\"work_exhausted\":"<<budget.work_exhausted<<",\"mesh_accepted\":false}"<<std::endl;
};
try {
  REQUIRE(coherentCapture);REQUIRE(current.mesh.size()==304521);
  REQUIRE(current.live_parameters->Payload()==TransactionRead(root+"/trajectory_working_parameters.bin"));
  REQUIRE(restored.stage.event==0);REQUIRE(policy.h0==SU2_TYPE::GetValue(config.GetAdap_BL(0).firstHeight));
  log<<"{\"stage\":\"saved_correction_input\",\"cells\":"<<current.mesh.size()<<",\"stage_height\":"<<policy.h0
     <<",\"prepared_authority_reused\":true,\"transported_current_field\":true,\"slots_per_context\":8,\"total_slots\":24}"<<std::endl;
  Directory directory(world,false,trial.live_parameters.get());REQUIRE(directory.Build(trial.mesh,reason,ceiling,caller));
  budget.caller_bytes=transfer_memory::Add(caller,directory.ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
  struct Seed {std::string label;Edge vertices;std::set<std::array<Id,4>> expected;};
  std::vector<Seed> seeds;const auto* path=std::getenv("SU2_NATIVE_BL_CAVITY_CONTEXTS");REQUIRE(path);
  std::ifstream input(path);REQUIRE(bool(input));size_t count=0;input>>count;REQUIRE(count==3);
  for(size_t i=0;i<count;++i) {
    Seed seed;size_t n=0;input>>std::setw(128)>>seed.label>>seed.vertices[0]>>seed.vertices[1]>>n;
    REQUIRE(bool(input));REQUIRE(seed.vertices[0]!=seed.vertices[1]);REQUIRE((n>0&&n<=MaximumCavityCells));
    for(size_t k=0;k<n;++k){std::array<Id,4> key;for(auto& id:key)input>>id;REQUIRE(bool(input));std::sort(key.begin(),key.end());
      REQUIRE(std::adjacent_find(key.begin(),key.end())==key.end());REQUIRE(seed.expected.insert(key).second);}
    seeds.push_back(std::move(seed));
  }
  input>>std::ws;REQUIRE(input.eof());
  for(const auto& seed:seeds) {
    const auto dependencies=directory.Lookup({Feature{{seed.vertices[0],0,0},1},Feature{{seed.vertices[1],0,0},1}},ceiling,caller);REQUIRE(dependencies.valid);
    const auto imported=directory.Fetch(dependencies,trial.mesh,ceiling,caller);REQUIRE(imported.valid);
    std::set<std::array<Id,4>> actual;for(const auto& row:imported.cells){std::array<Id,4> key;for(unsigned k=0;k<4;++k)key[k]=row.cell.v[k].id;std::sort(key.begin(),key.end());actual.insert(key);}
    REQUIRE(actual==seed.expected);
  }
  auto planning=[&](const Patch& patch,const std::vector<OwnedCell>& rows,const PlanarReference& geometry,size_t& q,size_t& j,size_t& bad) {
    auto result=measure(patch,rows,geometry,q,j,bad);std::set<Edge> requested;
    for(const auto& face:patch.surface)if(heights.count(geometry.Get(face.facet).marker))for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());requested.insert(e);}
    result.maximum_length=0;for(auto& edge:result.edges){if(!requested.count(edge.first))edge.second.length=1;
      if(edge.second.length>result.maximum_length){result.maximum_length=edge.second.length;result.longest_edge=edge.first;}}
    return result;
  };
  auto edges=[&](const Patch& patch,const PatchMetric& measured,const std::string& label,const char* side,unsigned slot) {
    std::set<Edge> requested;for(auto face:patch.surface)if(heights.count(trial.reference->Get(face.facet).marker))for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());requested.insert(e);}
    for(auto e:requested){const auto& m=measured.edges.at(e);log<<"{\"stage\":\"saved_correction_edge\",\"context\":"<<std::quoted(label)<<",\"slot\":"<<slot<<",\"side\":"<<std::quoted(side)
      <<",\"edge\":["<<e[0]<<','<<e[1]<<"],\"length\":"<<m.length<<",\"a\":["<<m.a.x<<','<<m.a.y<<','<<m.a.z<<"],\"b\":["<<m.b.x<<','<<m.b.y<<','<<m.b.z<<"]}"<<std::endl;}
  };
  auto targetWitness=[&](const Patch& patch,const std::vector<OwnedCell>& rows,const PlanarReference& geometry,const std::string& label,const char* side,unsigned slot) {
    std::set<Id> wall;for(auto f:patch.surface)if(heights.count(geometry.Get(f.facet).marker))wall.insert(f.v.begin(),f.v.end());
    for(auto row:rows)for(unsigned k=0;k<4;++k)if(wall.erase(row.cell.v[k].id)) {
      const auto node=row.cell.v[k];const auto samples=policy.BoundaryGuidance(node,geometry,row.sensor[k],reference->marker_names);
      for(size_t n=0;n<samples.size();++n){const auto& sample=samples[n];const auto m=sample.tensor;
        log<<"{\"stage\":\"saved_correction_target\",\"context\":"<<std::quoted(label)<<",\"side\":"<<std::quoted(side)<<",\"slot\":"<<slot<<",\"vertex\":"<<node.id<<",\"sample\":"<<n
           <<",\"point\":["<<node.p.x<<','<<node.p.y<<','<<node.p.z<<"],\"tensor\":["<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<"],\"normal\":["<<sample.normal.x<<','<<sample.normal.y<<','<<sample.normal.z
           <<"],\"strip_rule\":["<<sample.strip_rule.x<<','<<sample.strip_rule.y<<','<<sample.strip_rule.z<<"]}"<<std::endl;}
    }
  };
  struct Winner {Patch old,fresh;std::vector<OwnedCell> rows;Feature support;std::string label,operation;MetricChange kind;long double gain=0;bool active=false;};
  std::unique_ptr<Winner> winner;
  for(size_t context=0;context<seeds.size();++context) {
    const auto& seed=seeds[context];budget.caller_bytes=transfer_memory::Add(caller,directory.ResidentBytes());budget.CheckTime();
    const auto unionDeps=directory.Lookup({Feature{{seed.vertices[0],0,0},1},Feature{{seed.vertices[1],0,0},1}},ceiling,caller);REQUIRE(unionDeps.valid);
    const auto unionRows=directory.Fetch(unionDeps,trial.mesh,ceiling,caller);REQUIRE(unionRows.valid);
    std::vector<Id> unionIds;std::set<Id> haloVertices;for(const auto& row:unionRows.cells){unionIds.push_back(row.cell.id);for(auto n:row.cell.v)haloVertices.insert(n.id);}
    std::vector<Id> halo;for(const auto& row:trial.mesh)if(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return haloVertices.count(n.id);}))halo.push_back(row.first);
    size_t haloQ=0,haloJ=0,haloBad=0;
    for(size_t start=0;start<halo.size();start+=MaximumCavityCells) {
      const auto end=std::min(halo.size(),start+MaximumCavityCells);std::vector<Id> ids(halo.begin()+start,halo.begin()+end);std::vector<OwnedCell> rows;for(Id id:ids)rows.push_back(trial.mesh.at(id));
      size_t bad=0;planning(junction_wp0::PatchOf(trial,ids),rows,*trial.reference,haloQ,haloJ,bad);haloBad+=bad;
    }
    log<<"{\"stage\":\"saved_correction_context\",\"context\":"<<std::quoted(seed.label)<<",\"endpoint_union_cells\":"<<unionIds.size()<<",\"halo_cells\":"<<halo.size()
       <<",\"halo_q_bad\":"<<haloQ<<",\"halo_J_bad\":"<<haloJ<<",\"halo_geometry_bad\":"<<haloBad<<",\"reserved_slots\":8}"<<std::endl;
    for(unsigned local=0;local<8;++local) {
      const auto started=std::chrono::steady_clock::now();const auto cpu=std::clock();const auto work=budget.work;
      ++slots;REQUIRE(slots<=24);budget.caller_bytes=transfer_memory::Add(caller,directory.ResidentBytes());budget.CheckTime();
      const Id vertex=seed.vertices[local<3?0:local<6?1:0];const char* operation=local<6?"MoveVertex":local==6?"CollapseVertex":"Reconnect";
      const auto deps=directory.Lookup({Feature{{vertex,0,0},1}},ceiling,caller);REQUIRE(deps.valid);
      const auto imported=directory.Fetch(deps,trial.mesh,ceiling,caller);REQUIRE(imported.valid);REQUIRE((!imported.cells.empty()&&imported.cells.size()<=MaximumCavityCells));
      std::vector<Id> ids;for(auto row:imported.cells)ids.push_back(row.cell.id);const auto old=junction_wp0::PatchOf(trial,ids);
      auto oldContext=trial.reference->PrivateParameters(old);REQUIRE(oldContext);
      std::map<Id,Node> nodes;for(auto cell:old.cells)for(auto n:cell.v)nodes.emplace(n.id,n);
      std::set<Id> neighbors;for(auto face:old.surface)if(std::find(face.v.begin(),face.v.end(),vertex)!=face.v.end())neighbors.insert(face.v.begin(),face.v.end());neighbors.erase(vertex);
      std::string topologyWhy;auto skin=Incidence(old.cells).Boundary();for(auto& f:skin)std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());
      std::sort(skin.begin(),skin.end(),[](Face a,Face b){return junction_wp0::Sorted(a)<junction_wp0::Sorted(b);});REQUIRE(ValidatePrescribedInterface(old.cells,old.cells,skin,topologyWhy));
      const auto* binding=oldContext->Parameter(nodes.at(vertex).p);REQUIRE(binding);
      const bool reserved=directory.Reserve(imported,true,reason,ceiling,caller);REQUIRE(reserved);
      Patch fresh;std::vector<OwnedCell> freshRows;size_t q0=0,q1=0,j0=0,j1=0,b0=0,b1=0;
      bool built=false,complete=false,geometryOK=false,metricOK=false,parameters=false,protocol=false,contact=true,preserve=false,heightOK=false,strictHeight=false,stars=true,measured=false;
      std::string constructorWhy,certificationWhy,supportWhy,metricWhy,parameterWhy,contactWhy,preserveWhy,heightWhy;long double before=-1,after=-1;Id retainedNode=0;double displacement=0;
      PatchMetric oldPlan,newPlan;MetricDiagnosis diagnosis;const auto kind=local==6?MetricChange::Coarsen:MetricChange::GeometryRepair;
      try {
        oldPlan=planning(old,imported.cells,*oldContext,q0,j0,b0);
        REQUIRE(nextCell<UINT64_MAX-25*MaximumReplacementCells);REQUIRE(nextNode<UINT64_MAX-25);
        const Id first=nextCell+slots*MaximumReplacementCells;Node proposal{nextNode+slots,{}};
        if(local<6) {
          REQUIRE_FALSE(neighbors.empty());long double x=0,y=0,z=0;for(Id id:neighbors){const auto p=nodes.at(id).p;x+=p.x;y+=p.y;z+=p.z;}
          const long double step=local%3==0?1.L:local%3==1?.5L:.25L;const auto origin=nodes.at(vertex).p;
          proposal.p={double(origin.x+step*(x/neighbors.size()-origin.x)),double(origin.y+step*(y/neighbors.size()-origin.y)),double(origin.z+step*(z/neighbors.size()-origin.z))};
          ++calls;built=MoveVertex(old,vertex,proposal,first,*oldContext,fresh,constructorWhy);
          if(built)for(auto cell:fresh.cells)for(auto n:cell.v)if(n.id==proposal.id)displacement=std::hypot(n.p.x-origin.x,n.p.y-origin.y,n.p.z-origin.z);
        } else if(local==6) {
          long double shortest=INFINITY;for(auto edge:oldPlan.edges)if((edge.first[0]==vertex||edge.first[1]==vertex)&&neighbors.count(edge.first[0]==vertex?edge.first[1]:edge.first[0])) {
            bool wall=false;for(auto face:old.surface)if(heights.count(oldContext->Get(face.facet).marker)&&std::find(face.v.begin(),face.v.end(),edge.first[0])!=face.v.end()&&std::find(face.v.begin(),face.v.end(),edge.first[1])!=face.v.end())wall=true;
            if(wall&&edge.second.length<shortest){shortest=edge.second.length;retainedNode=edge.first[0]==vertex?edge.first[1]:edge.first[0];}}
          if(retainedNode){++calls;built=CollapseVertex(old,vertex,retainedNode,first,*oldContext,fresh,constructorWhy);}else constructorWhy="No incident requested-wall collapse edge.";
        } else {++calls;built=Reconnect(old,vertex,first,*oldContext,fresh,constructorWhy);}
        if(built) {
          REQUIRE((!fresh.cells.empty()&&fresh.cells.size()<=MaximumReplacementCells));std::set<Id> kept;
          for(auto c:fresh.cells){REQUIRE(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);for(auto n:c.v)kept.insert(n.id);OwnedCell row;row.cell=c;freshRows.push_back(row);}
          std::vector<Feature> removed;for(auto node:nodes)if(!kept.count(node.first))removed.push_back({{node.first,0,0},1});
          const auto needed=directory.Lookup(removed,ceiling,caller);stars=needed.valid;std::set<Id> oldIds(ids.begin(),ids.end());for(auto d:needed.cells)stars=stars&&oldIds.count(d.cell);if(!stars)supportWhy="Candidate omits a removed vertex complete current star: "+needed.reason;
          PublishMetrics(imported.cells,freshRows);
          std::map<Id,std::pair<Point,Tensor>> oldNodal;for(auto row:imported.cells)for(unsigned k=0;k<4;++k)oldNodal.emplace(row.cell.v[k].id,std::make_pair(row.cell.v[k].p,row.sensor[k]));
          for(auto row:freshRows)for(unsigned k=0;k<4;++k){const auto at=oldNodal.find(row.cell.v[k].id);if(at!=oldNodal.end()){REQUIRE(samePoint(at->second.first,row.cell.v[k].p));REQUIRE(sameTensor(at->second.second,row.sensor[k]));}}
          auto newContext=trial.reference->PrivateParameters(fresh);REQUIRE(newContext);
          const auto a=GeometryGuidanceExcess(old,*oldContext,SU2_TYPE::GetValue(config.GetAdap_Hausd())),b=GeometryGuidanceExcess(fresh,*newContext,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
          before=a.excess;after=b.excess;complete=a.complete&&b.complete;
          newPlan=planning(fresh,freshRows,*newContext,q1,j1,b1);measured=true;
          metricOK=ValidateMetricChange(old.cells.size(),oldPlan,fresh.cells.size(),newPlan,kind,metricWhy,&diagnosis);
          const bool progress=before>0&&after<before*(1-1e-12L);
          geometryOK=complete&&std::isfinite(after)&&(local==6?after==0:after<=before&&(progress||diagnosis.size_progress));
          parameters=trial.live_parameters->CheckReplacement(old,fresh,parameterWhy,ceiling,budget.caller_bytes,&budget);
          protocol=directory.Clearance(trial.mesh,old,fresh,contact,contactWhy,ceiling,caller);
          budget.caller_bytes=transfer_memory::Add(caller,directory.ResidentBytes());
          preserve=PreservePassingFirstHeights(old,fresh,heights,*oldContext,preserveWhy);FirstHeightReport h;
          strictHeight=CheckFirstHeight(fresh,heights,*newContext,h,heightWhy);heightOK=strictHeight||(h.measured&&!h.reference_clearance_failures&&!h.missing_cells);
          edges(old,oldPlan,seed.label,"old",slots);edges(fresh,newPlan,seed.label,"fresh",slots);
          const bool feasible=stars&&geometryOK&&metricOK&&parameters&&protocol&&contact&&preserve&&heightOK;
          const bool active=context<2;const auto gain=before-after;
          if(feasible){targetWitness(old,imported.cells,*oldContext,seed.label,"old",slots);targetWitness(fresh,freshRows,*newContext,seed.label,"fresh",slots);}
          if(feasible&&(!active||progress)&&(!winner||(active&&!winner->active)||(active==winner->active&&gain>winner->gain)))winner=std::make_unique<Winner>(Winner{old,fresh,freshRows,Feature{{vertex,0,0},1},seed.label,operation,kind,gain,active});
        }
      }catch(const std::exception& error){certificationWhy=error.what();}
      REQUIRE(trial.live_parameters->Payload()==acceptedParameters);
      const bool feasible=built&&measured&&complete&&stars&&geometryOK&&metricOK&&parameters&&protocol&&contact&&preserve&&heightOK;
      log<<"{\"stage\":\"saved_correction_trial\",\"context\":"<<std::quoted(seed.label)<<",\"slot\":"<<slots<<",\"local_slot\":"<<local<<",\"operator\":"<<std::quoted(operation)<<",\"vertex\":"<<vertex<<",\"retained\":"<<retainedNode
         <<",\"binding_dimension\":"<<binding->binding.dimension<<",\"genuine_corner\":"<<trial.live_parameters->Mobility().corners.count(vertex)<<",\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<fresh.cells.size()<<",\"displacement\":"<<displacement
         <<",\"reserved\":"<<reserved<<",\"constructed\":"<<built<<",\"complete_removed_stars\":"<<stars<<",\"measured\":"<<measured<<",\"geometry_complete\":"<<complete<<",\"geometry_pass\":"<<geometryOK<<",\"metric_pass\":"<<metricOK
         <<",\"parameter_pass\":"<<parameters<<",\"contact_protocol\":"<<protocol<<",\"contact_pass\":"<<contact<<",\"passing_heights_preserved\":"<<preserve<<",\"strict_height\":"<<strictHeight<<",\"provisional_height\":"<<heightOK<<",\"private_feasible\":"<<feasible
         <<",\"active_geometry_progress\":"<<(complete&&before>0&&after<before*(1-1e-12L))<<",\"unchanged_halo_by_complete_stars\":"<<stars<<",\"old_geometry_excess\":"<<before<<",\"new_geometry_excess\":"<<after<<",\"old_geometry_bad\":"<<b0<<",\"new_geometry_bad\":"<<b1<<",\"old_q_bad\":"<<q0<<",\"new_q_bad\":"<<q1<<",\"old_J_bad\":"<<j0<<",\"new_J_bad\":"<<j1
         <<",\"halo_q_delta\":"<<(measured?long(q1)-long(q0):0)<<",\"halo_J_delta\":"<<(measured?long(j1)-long(j0):0)<<",\"halo_geometry_delta\":"<<(measured?long(b1)-long(b0):0)<<",\"unchanged_halo_cells\":"<<halo.size()-old.cells.size()
         <<",\"support_reason\":"<<junction_wp0::Json(supportWhy)<<",\"constructor_reason\":"<<junction_wp0::Json(constructorWhy)<<",\"certification_reason\":"<<junction_wp0::Json(certificationWhy)<<",\"metric_reason\":"<<junction_wp0::Json(metricWhy)<<",\"parameter_reason\":"<<junction_wp0::Json(parameterWhy)
         <<",\"contact_reason\":"<<junction_wp0::Json(contactWhy)<<",\"preserve_reason\":"<<junction_wp0::Json(preserveWhy)<<",\"height_reason\":"<<junction_wp0::Json(heightWhy)<<",\"geometry_work\":"<<budget.work-work<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<",\"cpu_seconds\":"<<double(std::clock()-cpu)/CLOCKS_PER_SEC<<",\"mesh_accepted\":false}"<<std::endl;
    }
    ++completed;
  }
  REQUIRE(slots==24);REQUIRE(calls<=24);bool published=false;
  if(winner) {
    auto& publication=directory;budget.caller_bytes=transfer_memory::Add(caller,directory.ResidentBytes());
    auto originalContext=trial.reference->PrivateParameters(winner->old);REQUIRE(originalContext);
    const auto deps=publication.Lookup({winner->support},ceiling,caller);REQUIRE(deps.valid);
    const auto rows=publication.Fetch(deps,trial.mesh,ceiling,caller);REQUIRE(rows.valid);
    std::set<Id> expected,actual;for(auto c:winner->old.cells)expected.insert(c.id);for(auto c:rows.cells){actual.insert(c.cell.id);REQUIRE(c.version==current.mesh.at(c.cell.id).version);}REQUIRE(actual==expected);
    REQUIRE(publication.Reserve(rows,true,reason,ceiling,caller));
    for(auto& row:winner->rows){const auto faces=junction_wp0::CellFaces(row.cell);for(unsigned k=0;k<4;++k)for(auto f:winner->fresh.surface)if(junction_wp0::Sorted(faces[k])==junction_wp0::Sorted(f.v))row.physical[k]={true,f.facet};}
    const auto changes=BoundaryParameterState::Changes(winner->old,winner->fresh,trial.live_parameters->Revision());
    published=publication.Publish(trial.mesh,deps.cells,winner->rows,true,reason,ceiling,caller,changes,winner->fresh.coverage,&budget);REQUIRE(published);
    trial.reference=trial.reference->WithParameterState(trial.live_parameters)->WithAtlasReservation(budget);
    std::vector<Id> ids;for(auto row:winner->rows)ids.push_back(row.cell.id);const auto patch=junction_wp0::PatchOf(trial,ids);
    size_t q=0,j=0,bad=0,oq=0,oj=0,ob=0;const auto before=planning(winner->old,rows.cells,*originalContext,oq,oj,ob),after=planning(patch,winner->rows,*trial.reference,q,j,bad);
    std::string why;MetricDiagnosis diagnosis;REQUIRE(ValidateMetricChange(winner->old.cells.size(),before,patch.cells.size(),after,winner->kind,why,&diagnosis));
    const auto oldGeometry=GeometryGuidanceExcess(winner->old,*originalContext,SU2_TYPE::GetValue(config.GetAdap_Hausd())),newGeometry=GeometryGuidanceExcess(patch,*trial.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
    REQUIRE(oldGeometry.complete);REQUIRE(newGeometry.complete);
    const bool progress=oldGeometry.excess>0&&newGeometry.excess<oldGeometry.excess*(1-1e-12L);
    if(winner->active)REQUIRE(progress);
    REQUIRE((winner->kind==MetricChange::Coarsen?newGeometry.excess==0:newGeometry.excess<=oldGeometry.excess&&(progress||diagnosis.size_progress)));
    REQUIRE(PreservePassingFirstHeights(winner->old,patch,heights,*originalContext,why));FirstHeightReport height;
    const bool strict=CheckFirstHeight(patch,heights,*trial.reference,height,why);REQUIRE((strict||(height.measured&&!height.reference_clearance_failures&&!height.missing_cells)));
    log<<"{\"stage\":\"saved_correction_publication_support\",\"removed_cell_ids\":[";bool comma=false;for(auto c:winner->old.cells){if(comma)log<<',';log<<c.id;comma=true;}log<<"],\"added_cell_ids\":[";comma=false;for(auto c:winner->rows){if(comma)log<<',';log<<c.cell.id;comma=true;}log<<"]}"<<std::endl;
    log<<"{\"stage\":\"saved_correction_publication\",\"context\":"<<std::quoted(winner->label)<<",\"operator\":"<<std::quoted(winner->operation)<<",\"active_context\":"<<winner->active<<",\"published\":"<<published<<",\"q_bad\":"<<q<<",\"J_bad\":"<<j<<",\"geometry_bad\":"<<bad<<",\"final_mesh_accepted\":false}"<<std::endl;
  }
  log<<"{\"stage\":\"saved_correction_result\",\"contexts_completed\":"<<completed<<",\"slots\":"<<slots<<",\"constructor_calls\":"<<calls<<",\"active_feasible\":"<<(winner&&winner->active)<<",\"published\":"<<published<<",\"geometry_work\":"<<budget.work<<",\"stage_height\":"<<policy.h0<<",\"first_row_ready\":false,\"mesh_accepted\":false}"<<std::endl;
} catch(...) {saveWorking();throw;}
saveWorking();
