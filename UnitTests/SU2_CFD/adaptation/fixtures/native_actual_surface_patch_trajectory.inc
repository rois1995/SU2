// The connected observation region is not a transaction cavity. Reacquire each local transaction.
std::set<Edge> initialEdges;for(auto f:seedPatch.surface)for(unsigned k=0;k<3;++k){Edge e{f.v[k],f.v[(k+1)%3]};std::sort(e.begin(),e.end());initialEdges.insert(e);}
std::vector<Feature> initialQueries=seedQueries;for(auto e:initialEdges)initialQueries.push_back({{e[0],e[1],0},2});
const auto initialDependencies=directory.Lookup(initialQueries,ceiling,budget.caller_bytes);REQUIRE(initialDependencies.valid);
const auto initialRows=directory.Fetch(initialDependencies,trial.mesh,ceiling,budget.caller_bytes);REQUIRE(initialRows.valid);
std::set<Id> observed;for(auto row:initialRows.cells)observed.insert(row.cell.id);
auto canonicalGuidancePatch=[](Patch patch) {
  for(auto& face:patch.surface)std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());
  std::sort(patch.surface.begin(),patch.surface.end(),[](const auto& a,const auto& b){return a.facet<b.facet||(a.facet==b.facet&&a.v<b.v);});
  return patch;
};
struct ReplayStep {Edge edge;Node point;};std::vector<ReplayStep> replay;Edge replayFailure{};
std::set<std::array<Id,4>> replayCells;std::map<Id,Point> replayNodes;std::map<Face,Id> replayPhysical;bool replayFailureChecked=false;
std::ifstream replayInput("trajectory_replay.dat");
if(replayInput) {size_t count=0;replayInput>>count;REQUIRE(bool(replayInput));REQUIRE(count>0);REQUIRE(count<24);
  for(size_t k=0;k<count;++k){ReplayStep step;replayInput>>step.edge[0]>>step.edge[1]>>step.point.id>>step.point.p.x>>step.point.p.y>>step.point.p.z;REQUIRE(bool(replayInput));REQUIRE(step.edge[0]!=step.edge[1]);replay.push_back(step);}
  replayInput>>replayFailure[0]>>replayFailure[1]>>count;REQUIRE(bool(replayInput));REQUIRE(count>0);REQUIRE(count<=MaximumCavityCells);
  for(size_t k=0;k<count;++k){std::array<Id,4> cell;for(auto& id:cell)replayInput>>id;REQUIRE(bool(replayInput));std::sort(cell.begin(),cell.end());REQUIRE(replayCells.insert(cell).second);}
  replayInput>>count;REQUIRE(bool(replayInput));REQUIRE(count>0);REQUIRE(count<=4*MaximumCavityCells);
  for(size_t k=0;k<count;++k){Id id=0;Point point{};replayInput>>id>>point.x>>point.y>>point.z;REQUIRE(bool(replayInput));REQUIRE((std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z)));REQUIRE(replayNodes.emplace(id,point).second);}
  replayInput>>count;REQUIRE(bool(replayInput));REQUIRE(count<=4*MaximumCavityCells);for(size_t k=0;k<count;++k){Id facet;Face face;replayInput>>facet;for(auto& id:face)replayInput>>id;REQUIRE(bool(replayInput));std::sort(face.begin(),face.end());REQUIRE(replayPhysical.emplace(face,facet).second);}
  REQUIRE(bool(replayInput));replayInput>>std::ws;REQUIRE(replayInput.eof());
}
const auto initialCount=observed.size();const auto trajectoryStart=std::chrono::steady_clock::now();const auto trajectoryCpu=std::clock();const auto trajectoryWork=budget.work;
const size_t baseCaller=budget.caller_bytes;unsigned attempts=0,commits=0,cacheHits=0,scopeAdded=0;
long long netQ=0,netJ=0,netGeometry=0;std::string stop;
const unsigned attemptLimit=trajectoryRemaining;
Id nodeCursor=trajectoryNode,cellCursor=trajectoryCell;
struct Observation {Patch patch;PatchMetric metric;size_t q=0,j=0,bad=0;};
auto observe=[&]() {
  Observation result;std::vector<Id> all(observed.begin(),observed.end());result.patch=junction_wp0::PatchOf(trial,all);
  for(size_t first=0;first<all.size();first+=MaximumCavityCells) {
    const auto end=std::min(all.size(),first+MaximumCavityCells);std::vector<Id> ids(all.begin()+first,all.begin()+end);
    const auto patch=junction_wp0::PatchOf(trial,ids);std::vector<OwnedCell> rows;for(Id id:ids)rows.push_back(trial.mesh.at(id));
    size_t partBad=0;const auto part=measure(patch,rows,*trial.reference,result.q,result.j,partBad);result.bad+=partBad;
    result.metric.maximum_length=std::max(result.metric.maximum_length,part.maximum_length);
    for(auto edge:part.edges){auto at=result.metric.edges.find(edge.first);if(at==result.metric.edges.end()||edge.second.length>at->second.length)result.metric.edges.insert_or_assign(edge.first,edge.second);}
  }
  return result;
};
auto contextBytes=[&](const Imported& rows,const Patch& patch){
  const auto encoding=transfer_memory::Add(transfer_memory::Mul(patch.cells.size(),2048),transfer_memory::Mul(patch.parameters.size(),1024),BoundaryCoverageBytes(patch.coverage));
  if(encoding>(size_t(4)<<20))return std::vector<char>{}; // No rejection reuse when a complete bounded key is unavailable.
  REQUIRE(transfer_memory::Add(budget.caller_bytes,transfer_memory::Mul(4,encoding))<ceiling);
  auto bytes=NativeTransactionCapture::PatchBytes(patch);RecordStream out(bytes);NativeTransactionCapture::Vector(out,rows.cells);return bytes;
};
struct RefusedContext {std::vector<Feature> features;std::vector<char> bytes;};std::map<Edge,RefusedContext> refused;
auto cacheBytes=[&](){size_t n=0;for(const auto& r:refused)n=transfer_memory::Add(n,r.second.bytes.capacity(),transfer_memory::Bytes(r.second.features),sizeof(RefusedContext)+sizeof(Edge)+8*sizeof(void*));return n;};
log<<"{\"stage\":\"transaction_input\",\"initial_observation_cells\":"<<initialCount<<",\"observation_is_transaction\":false,\"requested_height_unchanged\":true,\"replay_steps\":"<<replay.size()<<"}"<<std::endl;
Observation last;unsigned observedCommits=0;
for(unsigned pass=0;pass<=attemptLimit;++pass) {
  budget.CheckTime();last=observe();observedCommits=commits;const auto debt=GeometryGuidanceExcess(last.patch,*trial.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));REQUIRE(debt.complete);
  std::map<Id,Node> points;for(auto c:last.patch.cells)for(auto n:c.v)points.emplace(n.id,n);
  std::map<Edge,Id> examine;size_t shortEdges=0,wallEdges=0;
  std::set<Edge> edges;
  for(auto f:last.patch.surface)if(heights.count(trial.reference->Get(f.facet).marker)) {
    const double bound=trial.reference->SurfaceDeviation(f.facet, {points.at(f.v[0]).p, points.at(f.v[1]).p, points.at(f.v[2]).p}, SU2NativeGeometry::SurfaceQueryNeed::Magnitude(), true);
    log<<"{\"stage\":\"transaction_face\",\"pass\":"<<pass<<",\"facet\":"<<f.facet<<",\"vertices\":["<<f.v[0]<<','<<f.v[1]<<','<<f.v[2]<<"],\"bound\":"<<bound<<",\"final_certificate\":false}"<<std::endl;
    for(unsigned k=0;k<3;++k){Edge e{f.v[k],f.v[(k+1)%3]};std::sort(e.begin(),e.end());edges.insert(e);if(bound>SU2_TYPE::GetValue(config.GetAdap_Hausd()))examine.emplace(e,f.facet);}
  }
  for(auto e:edges){const auto& m=last.metric.edges.at(e);++wallEdges;shortEdges+=m.length<.5;
    log<<"{\"stage\":\"transaction_edge\",\"pass\":"<<pass<<",\"edge\":["<<e[0]<<','<<e[1]<<"],\"length\":"<<m.length<<",\"a\":["<<m.a.x<<','<<m.a.y<<','<<m.a.z<<"],\"b\":["<<m.b.x<<','<<m.b.y<<','<<m.b.z<<"]}"<<std::endl;}
  log<<"{\"stage\":\"transaction_state\",\"pass\":"<<pass<<",\"cells\":"<<observed.size()<<",\"commits\":"<<commits<<",\"attempts\":"<<attempts
     <<",\"q_bad\":"<<last.q<<",\"J_bad\":"<<last.j<<",\"geometry_bad\":"<<last.bad<<",\"geometry_excess\":"<<debt.excess<<",\"wall_edges\":"<<wallEdges<<",\"below_half\":"<<shortEdges
     <<",\"planning_max\":"<<last.metric.maximum_length<<",\"scope_added_old_cells\":"<<scopeAdded<<",\"sum_local_q_delta\":"<<netQ<<",\"sum_local_J_delta\":"<<netJ<<",\"sum_local_geometry_delta\":"<<netGeometry
     <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-trajectoryStart).count()<<",\"geometry_work\":"<<budget.work-trajectoryWork<<",\"mesh_accepted\":false}"<<std::endl;
  if(!last.bad){stop="observed mesh-to-model guidance closed; final bidirectional geometry and tangential suitability remain";break;}
  if(attempts>=attemptLimit){stop="unchanged 24-constructor work bound";break;}
  std::vector<std::pair<double,Edge>> ranked;
  for(auto item:examine) {
    const auto e=item.first;const auto a=points.at(e[0]).p,b=points.at(e[1]).p;const Point mid{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
    const auto entity=trial.live_parameters->Entity(item.second);size_t work=0;bool charged=false;double scale=1;
    for(auto p:{a,b})scale=std::max({scale,std::abs(p.x),std::abs(p.y),std::abs(p.z)});const double roundoff=std::nextafter(16*std::numeric_limits<double>::epsilon()*scale,INFINITY);
    try {const auto nearest=trial.live_parameters->Geometry().ClosestCurved(item.second,mid,1e-6,
        std::min(SU2NativeGeometry::MappedFacetReference::ClearanceWork,budget.maximum_work-budget.work),budget.maximum_bytes-budget.caller_bytes,
        [&](Id id){return trial.live_parameters->Entity(id)==entity;},&work,&budget);
      charged=true;REQUIRE(budget.ChargeWork(work));const bool outside=nearest.lower>SU2_TYPE::GetValue(config.GetAdap_Hausd())+roundoff;
      if(outside)ranked.push_back({-nearest.lower,e});
      log<<"{\"stage\":\"transaction_witness\",\"pass\":"<<pass<<",\"edge\":["<<e[0]<<','<<e[1]<<"],\"lower\":"<<nearest.lower<<",\"upper\":"<<nearest.upper<<",\"outside\":"<<outside<<",\"work\":"<<work<<"}"<<std::endl;
    }catch(const std::exception& x){if(!charged)REQUIRE(budget.ChargeWork(work));log<<"{\"stage\":\"transaction_uncertain\",\"reason\":"<<junction_wp0::Json(x.what())<<"}"<<std::endl;}
  }
  std::sort(ranked.begin(),ranked.end());
  if(commits<replay.size()){const auto edge=replay[commits].edge;ranked={{0,edge}};}
  else if(!replay.empty()&&!replayFailureChecked){ranked.erase(std::remove_if(ranked.begin(),ranked.end(),[&](const auto& r){return r.second==replayFailure;}),ranked.end());ranked.insert(ranked.begin(),{0,replayFailure});}
  if(ranked.empty()){stop="no demonstrated outside midpoint; complete-face debt unresolved, sampling is not certification";break;}
  Directory transaction(world,false,trial.live_parameters.get());REQUIRE(transaction.Build(trial.mesh,reason,ceiling,baseCaller));
  struct CallerRestore {SU2NativeGeometry::AtlasBudget& b;size_t old;~CallerRestore(){b.caller_bytes=old;}} callerRestore{budget,budget.caller_bytes};
  budget.caller_bytes=transfer_memory::Add(baseCaller,transaction.ResidentBytes(),SurfacePlanningScratchBytes(MaximumReplacementCells),size_t(32)<<20);REQUIRE(budget.caller_bytes<ceiling);
  bool published=false;
  for(auto selected:ranked) {
    const Edge edge=selected.second;std::vector<Feature> features{{{edge[0],edge[1],0},2}};
    const auto cached=refused.find(edge);
    if(cached!=refused.end()) {
      const auto deps=transaction.Lookup(cached->second.features,ceiling,budget.caller_bytes);REQUIRE(deps.valid);
      const auto rows=transaction.Fetch(deps,trial.mesh,ceiling,budget.caller_bytes);REQUIRE(rows.valid);
      std::vector<Id> ids;for(auto row:rows.cells)ids.push_back(row.cell.id);const auto old=junction_wp0::PatchOf(trial,ids);
      const auto currentKey=contextBytes(rows,old);if(!currentKey.empty()&&currentKey==cached->second.bytes){++cacheHits;continue;}refused.erase(cached);
    }
    bool reconstruct=false;std::set<Face> grown;RefusedContext lastFailure;
    while(attempts<attemptLimit) {
      const auto deps=transaction.Lookup(features,ceiling,budget.caller_bytes);
      if(!deps.valid){stop=deps.reason;break;}
      const auto rows=transaction.Fetch(deps,trial.mesh,ceiling,budget.caller_bytes);
      if(!rows.valid){stop=rows.reason;break;}
      REQUIRE(rows.cells.size()<=MaximumCavityCells);std::vector<Id> ids;for(auto row:rows.cells)ids.push_back(row.cell.id);
      const auto old=junction_wp0::PatchOf(trial,ids);std::string why;const auto skin=Incidence(old.cells).Boundary();
      auto expected=skin;for(auto& f:expected)std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());std::sort(expected.begin(),expected.end(),[](Face a,Face b){return junction_wp0::Sorted(a)<junction_wp0::Sorted(b);});
      const bool topology=ValidatePrescribedInterface(old.cells,old.cells,expected,why);if(!topology){stop="current local transaction input topology: "+why;break;}
      if(!reconstruct&&!replay.empty()&&commits==replay.size()&&!replayFailureChecked&&edge==replayFailure) {
        std::set<std::array<Id,4>> actualCells;std::map<Id,Point> actualNodes;std::map<Face,Id> actualPhysical;
        for(auto c:old.cells){std::array<Id,4> ids;for(unsigned k=0;k<4;++k){ids[k]=c.v[k].id;actualNodes.emplace(c.v[k].id,c.v[k].p);}std::sort(ids.begin(),ids.end());actualCells.insert(ids);}
        REQUIRE(actualCells==replayCells);REQUIRE(actualNodes.size()==replayNodes.size());
        for(auto row:replayNodes){const auto p=actualNodes.at(row.first);REQUIRE((p.x==row.second.x&&p.y==row.second.y&&p.z==row.second.z));}
        for(auto f:old.surface)actualPhysical.emplace(junction_wp0::Sorted(f.v),f.facet);REQUIRE(actualPhysical==replayPhysical);
        log<<"{\"stage\":\"transaction_replay_checkpoint\",\"prefix_commits\":"<<commits<<",\"cells\":"<<old.cells.size()<<",\"exact_topology_coordinates_physical_support\":true}"<<std::endl;
        replayFailureChecked=true;
      }
      const bool reserved=transaction.Reserve(rows,true,reason,ceiling,budget.caller_bytes);
      if(!reserved){stop="complete current reservation refused: "+reason;break;}
      const auto a=points.at(edge[0]).p,b=points.at(edge[1]).p;Node point{nodeCursor,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
      auto context=trial.reference->PrivateParameters(old);REQUIRE(context);Patch candidate;Face obstruction{};bool constructed=false;
      REQUIRE(trajectoryRemaining>0);--trajectoryRemaining;++attempts;
      if(!reconstruct)constructed=SplitSurfaceStrip(old,edge,point,{},cellCursor,*context,candidate,why);
      else {
        auto donors=trialSource.ImportCellsLocal(old.cells,rows.cells,ceiling,budget.caller_bytes);REQUIRE(donors.valid);const auto extensions=AssociatedExtensions(donors.cells,*context);
        FrozenField field(std::move(donors.cells),policy.Composition(),true,extensions,policy.Edges(),std::make_shared<const FrozenMetricIdentity>(),true,MaximumDonors,true);
        const auto placement=field.Query(Centroid(old.cells.front())).target;
        constructed=ReconstructSplitPatch(old,edge,point,{nodeCursor+1,{}},placement,cellCursor,*context,candidate,obstruction,why,nullptr,heights);
      }
      if(!reconstruct&&replayFailureChecked&&commits==replay.size()&&edge==replayFailure){REQUIRE_FALSE(constructed);REQUIRE(why.find("inverts inherited volume connectivity")!=std::string::npos);}
      bool metric=false,parameters=false,protocol=false,contact=true,preserved=false,provisional=false,geometryProgress=false;
      std::string metricWhy,parameterWhy,contactWhy,preserveWhy,heightWhy;
      size_t q0=0,q1=0,j0=0,j1=0,g0=0,g1=0;std::vector<OwnedCell> nextRows;long double excess0=-1,excess1=-1;
      if(constructed) {
        REQUIRE(candidate.cells.size()<=MaximumReplacementCells);for(auto c:candidate.cells){REQUIRE(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);OwnedCell row;row.cell=c;nextRows.push_back(row);}
        PublishMetrics(rows.cells,nextRows);auto nextContext=trial.reference->PrivateParameters(candidate);REQUIRE(nextContext);
        const auto before=measure(old,rows.cells,*context,q0,j0,g0),after=measure(candidate,nextRows,*nextContext,q1,j1,g1);
        const auto oldGuidance=GeometryGuidanceExcess(old,*context,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
        const auto newGuidance=GeometryGuidanceExcess(candidate,*nextContext,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
        excess0=oldGuidance.excess;excess1=newGuidance.excess;
        geometryProgress=oldGuidance.complete&&newGuidance.complete&&std::isfinite(excess1)&&excess0>0&&excess1<excess0*(1-1e-12L);
        metric=ValidateMetricChange(old.cells.size(),before,candidate.cells.size(),after,MetricChange::GeometryRefine,metricWhy);
        parameters=trial.live_parameters->CheckReplacement(old,candidate,parameterWhy,ceiling,budget.caller_bytes,&budget);
        protocol=transaction.Clearance(trial.mesh,old,candidate,contact,contactWhy,ceiling,budget.caller_bytes);
        preserved=PreservePassingFirstHeights(old,candidate,heights,*context,preserveWhy);FirstHeightReport h;
        const bool strict=CheckFirstHeight(candidate,heights,*nextContext,h,heightWhy);provisional=strict||(h.measured&&!h.reference_clearance_failures&&!h.missing_cells);
        // Keep the legacy replay unchanged; current-input admission matches production GeometryRefine.
        if(metric&&parameters&&protocol&&contact&&preserved&&provisional&&(!currentResidual||geometryProgress)) {
          const auto expectedPublishedGeometry=GeometryGuidanceExcess(canonicalGuidancePatch(candidate),*nextContext,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
          REQUIRE(expectedPublishedGeometry.complete);
          {std::map<Face,Id> physical;for(auto f:candidate.surface)physical.emplace(junction_wp0::Sorted(f.v),f.facet);
            for(auto& row:nextRows){const auto faces=junction_wp0::CellFaces(row.cell);for(unsigned k=0;k<4;++k){auto f=physical.find(junction_wp0::Sorted(faces[k]));if(f!=physical.end())row.physical[k]={true,f->second};}}
            const auto edits=BoundaryParameterState::Changes(old,candidate,trial.live_parameters->Revision());
            published=transaction.Publish(trial.mesh,deps.cells,nextRows,true,reason,ceiling,budget.caller_bytes,edits,candidate.coverage,&budget);
            if(published){for(auto row:rows.cells){scopeAdded+=!observed.count(row.cell.id);observed.erase(row.cell.id);}for(auto row:nextRows){observed.insert(row.cell.id);cellCursor=std::max(cellCursor,row.cell.id+1);for(auto n:row.cell.v)nodeCursor=std::max(nodeCursor,n.id+1);}
              if(commits<replay.size()){const auto expected=replay[commits].point;bool found=false;
                for(auto row:nextRows)for(auto n:row.cell.v)if(n.id==expected.id){REQUIRE((n.p.x==expected.p.x&&n.p.y==expected.p.y&&n.p.z==expected.p.z));found=true;}REQUIRE(found);
                log<<"{\"stage\":\"transaction_replay_step\",\"step\":"<<commits<<",\"point\":"<<expected.id<<",\"exact_coordinate_match\":true}"<<std::endl;}
              trial.reference=trial.reference->WithParameterState(trial.live_parameters)->WithAtlasReservation(budget);++commits;netQ+=long(q1)-long(q0);netJ+=long(j1)-long(j0);netGeometry+=long(g1)-long(g0);refused.erase(edge);
              std::vector<Id> publishedIds;for(const auto& row:nextRows){publishedIds.push_back(row.cell.id);
                for(unsigned k=0;k<4;++k)REQUIRE(sameTensor(trial.mesh.at(row.cell.id).sensor[k],row.sensor[k]));}
              const auto publishedPatch=junction_wp0::PatchOf(trial,publishedIds);
              size_t pq=0,pj=0,pg=0;const auto publishedMetric=measure(publishedPatch,nextRows,*trial.reference,pq,pj,pg);
              REQUIRE(pq==q1);REQUIRE(pj==j1);REQUIRE(pg==g1);
              REQUIRE(publishedMetric.maximum_length==after.maximum_length);REQUIRE(publishedMetric.edges.size()==after.edges.size());
              for(const auto& e:after.edges)REQUIRE(publishedMetric.edges.at(e.first).length==e.second.length);
              const auto publishedGeometry=GeometryGuidanceExcess(canonicalGuidancePatch(publishedPatch),*trial.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
              REQUIRE(publishedGeometry.complete);REQUIRE(publishedGeometry.excess==expectedPublishedGeometry.excess);
            }
          }
        }
      }
      log<<"{\"stage\":\"transaction_operation\",\"attempt\":"<<attempts<<",\"operator\":"<<std::quoted(reconstruct?"ReconstructSplitPatch":"SplitSurfaceStrip")<<",\"edge\":["<<edge[0]<<','<<edge[1]<<"],\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<(constructed?candidate.cells.size():0)
         <<",\"constructed\":"<<constructed<<",\"native_input_topology\":"<<topology<<",\"metric\":"<<metric<<",\"parameters\":"<<parameters<<",\"contact_protocol\":"<<protocol<<",\"contact\":"<<contact<<",\"preserved_heights\":"<<preserved<<",\"provisional_height\":"<<provisional<<",\"reserved\":"<<reserved<<",\"published\":"<<published
         <<",\"geometry_progress\":"<<geometryProgress<<",\"production_geometry_guard_required\":"<<currentResidual<<",\"metric_reason\":"<<junction_wp0::Json(metricWhy)<<",\"parameter_reason\":"<<junction_wp0::Json(parameterWhy)<<",\"collision_peak_admitted_bytes\":"<<transaction.Statistics().maximum_work_bytes<<",\"collision_ceiling_bytes\":"<<ceiling<<",\"contact_reason\":"<<junction_wp0::Json(contactWhy)<<",\"preserve_reason\":"<<junction_wp0::Json(preserveWhy)<<",\"height_reason\":"<<junction_wp0::Json(heightWhy)<<",\"measurements_known\":"<<constructed<<",\"old_q_bad\":"<<q0<<",\"new_q_bad\":"<<q1<<",\"old_J_bad\":"<<j0<<",\"new_J_bad\":"<<j1<<",\"old_geometry_bad\":"<<g0<<",\"new_geometry_bad\":"<<g1<<",\"old_geometry_excess\":"<<excess0<<",\"new_geometry_excess\":"<<excess1<<",\"reason\":"<<junction_wp0::Json(why)<<",\"publication_reason\":"<<junction_wp0::Json(reason)<<",\"obstruction\":["<<obstruction[0]<<','<<obstruction[1]<<','<<obstruction[2]<<"]}"<<std::endl;
      if(published)break;
      if(constructed){break;} // Admission failure is not permission to change a physical constraint.
      const bool deterministic=budget.geometry_failure.empty()&&!budget.time_exhausted&&!budget.work_exhausted&&
          (why=="Native3D surface strip inverts inherited volume connectivity."||why=="Native3D joint point cannot see its retained skin.");
      lastFailure=deterministic?RefusedContext{features,contextBytes(rows,old)}:RefusedContext{};
      if(!reconstruct&&why.find("inverts inherited volume connectivity")!=std::string::npos){reconstruct=true;continue;}
      if(reconstruct&&obstruction!=Face{}&&grown.insert(junction_wp0::Sorted(obstruction)).second){features.push_back({obstruction,3});continue;}
      break;
    }
    if(published)break;
    if(!lastFailure.bytes.empty()){const auto retainedKey=transfer_memory::Add(cacheBytes(),lastFailure.bytes.capacity(),transfer_memory::Bytes(lastFailure.features),sizeof(RefusedContext)+sizeof(Edge)+8*sizeof(void*));
      if(retainedKey<(size_t(4)<<20))refused.insert_or_assign(edge,std::move(lastFailure));}
    if(attempts>=attemptLimit||!stop.empty())break;
  }
  if(!published){if(stop.empty())stop=attempts>=attemptLimit?"unchanged 24-constructor work bound":"all current supported constructor contexts refused";break;}
}
if(observedCommits!=commits)last=observe();
Patch progressing=last.patch;PatchMetric finalPlan=last.metric;size_t remainingBad=last.bad;
bool firstRowReached=false,firstRowReady=false;size_t firstRowWork=0;
if(remainingBad==0&&finalPlan.maximum_length<=1.8&&attempts<attemptLimit) {
  using namespace junction_wp0;
  const auto rowCaller=transfer_memory::Add(budget.caller_bytes,
      transfer_memory::Mul(trial.mesh.size(),6*(sizeof(DonorCell)+640)),
      transfer_memory::Mul(progressing.surface.size(),3*1024));
  REQUIRE(rowCaller<ceiling);struct RestoreCaller {SU2NativeGeometry::AtlasBudget& b;size_t old;~RestoreCaller(){b.caller_bytes=old;}} restoreCaller{budget,budget.caller_bytes};
  budget.caller_bytes=rowCaller;
  auto state=std::make_unique<State>();state->name="actual_connected_patch_trajectory";
  state->mesh=trial.mesh;state->live_parameters=trial.live_parameters;state->reference=trial.reference;
  struct ReturnState {State& source;State& destination;
    ~ReturnState(){destination.mesh.swap(source.mesh);destination.live_parameters=source.live_parameters;destination.reference=source.reference;}
  } returnState{*state,trial};
  state->facets=reference->facets;state->markers=reference->marker_names;state->heights=heights;
  for(auto height:heights)state->walls.insert(height.first);
  state->h0=policy.h0;state->core=policy.core;state->layers=policy.full;state->mapped=true;
  state->growth=SU2_TYPE::GetValue(config.GetAdap_BL(0).growth);state->thickness=SU2_TYPE::GetValue(config.GetAdap_BL(0).thickness);state->memory_ceiling=ceiling;
  state->retained_bytes=[&](){return budget.caller_bytes;};state->admit_scratch=[&](size_t n){REQUIRE(transfer_memory::Add(budget.caller_bytes,n)<ceiling);};
  std::vector<DonorCell> donors;donors.reserve(state->mesh.size());for(auto row:state->mesh){DonorCell donor;donor.cell=row.second.cell;donor.sensor=row.second.sensor;donors.push_back(donor);}
  const auto donorCount=donors.size();const auto extensions=AssociatedExtensions(donors,*state->reference);
  state->field=std::make_unique<FrozenField>(std::move(donors),policy.Composition(),true,extensions,policy.Edges(),
      std::make_shared<const FrozenMetricIdentity>(),true,std::max(MaximumDonors,donorCount),true);
  dc10::optionA=policy.A;dc10::probeBase=policy.base;dc10::probeFull=policy.full;dc9::validityOnlyConstruction=true;dc9::markerRoles=policy.roles;
  Prepare(*state);dc12::ApplyWallPolicies(*state);state->heights.tolerance=heights.tolerance;dc9::DetectCreases(*state,policy.roles);
  dc12::DirField directions;struct ResetDirections {~ResetDirections(){dc9::ensureDirectionField={};dc9::directionField=nullptr;}} resetDirections;
  dc9::ensureDirectionField=[&](State& current){if(current.direction_revision==current.boundary_revision)return;budget.CheckTime();size_t stars=0;
    for(const auto& face:current.wall_index)for(Id v:face.first){auto at=current.vertex_cells.find(v);if(at!=current.vertex_cells.end())stars=transfer_memory::Add(stars,at->second.size());}
    current.admit_scratch(transfer_memory::Add(transfer_memory::Mul(current.wall_index.size(),3*1024),transfer_memory::Mul(stars,1024)));
    directions=dc12::ComputeDirectionField(current);current.direction_revision=current.boundary_revision;dc9::directionField=&directions.d;};
  dc9::ensureDirectionField(*state);std::set<Id> affected;std::set<Face> targets;
  for(auto face:progressing.surface)if(heights.count(state->reference->Get(face.facet).marker)){targets.insert(Sorted(face.v));for(auto id:face.v)affected.insert(id);}
  for(auto vertex:affected){const auto direction=dc9::DirectionAt(*state,vertex);log<<"{\"stage\":\"trajectory_direction\",\"vertex\":"<<vertex<<",\"ok\":"<<direction.ok<<",\"carrier_rank\":"<<direction.rank<<"}"<<std::endl;}
  firstRowReached=true;dc10::Rows10 rows;std::set<Face> certified;std::ostringstream rowLog;
  const double seconds=std::max(0.,300-std::chrono::duration<double>(std::chrono::steady_clock::now()-begun).count());
  const unsigned rowAllowance=trajectoryRemaining;trajectoryRemaining=0;
  const auto rowResult=dc9::PhaseR2(*state,static_cast<dc9::Rows&>(rows),seconds,rowLog,attemptLimit-attempts,&affected,&certified);
  REQUIRE(rowResult.work<=rowAllowance);firstRowWork=rowResult.work;trajectoryRemaining=rowAllowance-firstRowWork;
  size_t passing=0;for(auto key:targets){const auto owner=state->wall_index.find(key);if(owner==state->wall_index.end())continue;
    FirstHeightReport h;std::string why;const bool strict=CheckFirstHeight(PatchOf(*state,{owner->second}),heights,*state->reference,h,why);passing+=strict;
    log<<"{\"stage\":\"trajectory_first_child\",\"face\":["<<key[0]<<','<<key[1]<<','<<key[2]<<"],\"strict_height\":"<<strict<<",\"reason\":"<<Json(why)<<"}"<<std::endl;}
  REQUIRE_FALSE(state->publication_failed);
  for(auto row:trial.mesh)if(!state->mesh.count(row.first))observed.erase(row.first);
  for(auto row:state->mesh)if(!trial.mesh.count(row.first))observed.insert(row.first);
  firstRowReady=!targets.empty()&&passing==targets.size();stop=firstRowReady?"strict local first children reached; full constraints and transition remain":"first-row coverage stalled within unchanged remaining work/deadline";
  std::ofstream("trajectory_first_row.log")<<rowLog.str();
  log<<"{\"stage\":\"trajectory_first_row\",\"work\":"<<rowResult.work<<",\"commits\":"<<rowResult.accepted<<",\"target_faces\":"<<targets.size()<<",\"passing\":"<<passing<<",\"outcome\":"<<Json(rowResult.outcome)<<",\"final_mesh_accepted\":false}"<<std::endl;
}
if(firstRowReached){last=observe();remainingBad=last.bad;}
log<<"{\"stage\":\"transaction_final_observation\",\"cells\":"<<observed.size()<<",\"q_bad\":"<<last.q<<",\"J_bad\":"<<last.j<<",\"geometry_bad\":"<<last.bad<<",\"planning_max\":"<<last.metric.maximum_length<<",\"after_first_row\":"<<firstRowReached<<"}"<<std::endl;
log<<"{\"stage\":\"transaction_result\",\"initial_observation_cells\":"<<initialCount<<",\"final_observation_cells\":"<<observed.size()<<",\"attempts\":"<<attempts<<",\"commits\":"<<commits
   <<",\"geometry_bad\":"<<remainingBad<<",\"mesh_to_model_guidance_closed\":"<<(remainingBad==0)<<",\"surface_geometry_certified\":false,\"first_row_reached\":"<<firstRowReached<<",\"target_first_height_coverage\":"<<firstRowReady
   <<",\"replay_checkpoint_matched\":"<<replayFailureChecked<<",\"rejected_context_hits\":"<<cacheHits<<",\"sum_local_q_delta\":"<<netQ<<",\"sum_local_J_delta\":"<<netJ<<",\"sum_local_geometry_delta\":"<<netGeometry<<",\"scope_added_old_cells\":"<<scopeAdded
   <<",\"stop\":"<<junction_wp0::Json(stop)<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-trajectoryStart).count()<<",\"cpu_seconds\":"<<double(std::clock()-trajectoryCpu)/CLOCKS_PER_SEC
   <<",\"geometry_work\":"<<budget.work-trajectoryWork<<",\"original_mesh_accepted\":false}"<<std::endl;

REQUIRE(attempts+firstRowWork<=attemptLimit);
REQUIRE(trajectoryRemaining==attemptLimit-attempts-firstRowWork);
log<<"{\"stage\":\"trajectory_budget\",\"geometry_constructor_calls\":"<<attempts
   <<",\"first_row_work\":"<<firstRowWork<<",\"remaining_shared_calls\":"<<trajectoryRemaining<<"}"<<std::endl;
