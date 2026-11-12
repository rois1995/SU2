// Diagnostic only: one complete reserved patch, with provisional validity-first steps.
std::set<Edge> admittedEdges;
for(const auto& face:publishedPatch.surface)for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());admittedEdges.insert(e);}
// The saved edge-star union has annular links at these two diagnosed vertices.
std::vector<Feature> queries{{{4218,0,0},1},{{4219,0,0},1}};for(auto e:admittedEdges)queries.push_back({{e[0],e[1],0},2});
const auto dependencies=directory.Lookup(queries,ceiling,held);REQUIRE(dependencies.valid);
const auto support=directory.Fetch(dependencies,trial.mesh,ceiling,held);REQUIRE(support.valid);
REQUIRE(support.cells.size()<=MaximumCavityCells);REQUIRE(directory.Reserve(support,true,reason,ceiling,held));
std::vector<Id> ids;for(const auto& row:support.cells)ids.push_back(row.cell.id);
const auto original=junction_wp0::PatchOf(trial,ids);Patch progressing=original;
for(auto cell:publishedPatch.cells)REQUIRE(std::find(ids.begin(),ids.end(),cell.id)!=ids.end());
auto canonicalSkin=[](const Patch& patch){std::map<Face,Face> faces;for(auto face:Incidence(patch.cells).Boundary()){
  std::rotate(face.begin(),std::min_element(face.begin(),face.end()),face.end());faces.emplace(junction_wp0::Sorted(face),face);}
  std::vector<Face> result;for(auto f:faces)result.push_back(f.second);return result;};
REQUIRE(ValidatePrescribedInterface(original.cells,original.cells,canonicalSkin(original),reason));
log<<"{\"stage\":\"trajectory_input\",\"cells\":"<<original.cells.size()<<",\"native_identity_topology_pass\":true,\"complete_reservation\":true}"<<std::endl;
const Id firstPoint=nextNode+completed,firstCell=nextCell+MaximumReplacementCells;
std::set<Edge> refused;unsigned attempts=0,cuts=0;size_t remainingBad=0;std::string stop;
const auto trajectoryStart=std::chrono::steady_clock::now();const auto trajectoryCpu=std::clock();const auto trajectoryWork=budget.work;
auto rowsOf=[&](const Patch& patch){std::vector<OwnedCell> rows;for(const auto& cell:patch.cells){OwnedCell row;row.cell=cell;rows.push_back(row);}PublishMetrics(support.cells,rows);return rows;};
std::vector<OwnedCell> finalRows;PatchMetric finalPlan;
for(unsigned pass=0;pass<=24;++pass) {
  budget.CheckTime();auto context=trial.reference->PrivateParameters(progressing);REQUIRE(context);const auto& geometry=*context;
  const auto rows=rowsOf(progressing);size_t qb=0,jb=0,bad=0;
  const auto plan=measure(progressing,rows,geometry,qb,jb,bad);remainingBad=bad;finalRows=rows;finalPlan=plan;
  const auto debt=GeometryGuidanceExcess(progressing,geometry,SU2_TYPE::GetValue(config.GetAdap_Hausd()));REQUIRE(debt.complete);
  std::map<Id,Node> points;for(const auto& c:progressing.cells)for(auto n:c.v)points.emplace(n.id,n);
  std::set<Edge> edges;std::vector<SurfaceFace> failing;
  for(const auto& face:progressing.surface)if(heights.count(geometry.Get(face.facet).marker)) {
    const double bound=geometry.SurfaceDeviation(face.facet,{points.at(face.v[0]).p,points.at(face.v[1]).p,points.at(face.v[2]).p},true);
    log<<"{\"stage\":\"trajectory_face\",\"pass\":"<<pass<<",\"facet\":"<<face.facet<<",\"vertices\":["<<face.v[0]<<','<<face.v[1]<<','<<face.v[2]<<"],\"bound\":"<<bound<<",\"final_certificate\":false}"<<std::endl;
    if(bound>SU2_TYPE::GetValue(config.GetAdap_Hausd()))failing.push_back(face);
    for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());edges.insert(e);}
  }
  size_t shortEdges=0;for(auto e:edges){const auto& m=plan.edges.at(e);shortEdges+=m.length<.5;
    log<<"{\"stage\":\"trajectory_edge\",\"pass\":"<<pass<<",\"edge\":["<<e[0]<<','<<e[1]<<"],\"length\":"<<m.length<<",\"a\":["<<m.a.x<<','<<m.a.y<<','<<m.a.z<<"],\"b\":["<<m.b.x<<','<<m.b.y<<','<<m.b.z<<"]}"<<std::endl;}
  log<<"{\"stage\":\"trajectory_state\",\"pass\":"<<pass<<",\"cells\":"<<progressing.cells.size()<<",\"cuts\":"<<cuts<<",\"attempts\":"<<attempts
     <<",\"q_bad\":"<<qb<<",\"J_bad\":"<<jb<<",\"geometry_bad\":"<<bad<<",\"geometry_excess\":"<<debt.excess<<",\"wall_edges\":"<<edges.size()<<",\"below_half\":"<<shortEdges
     <<",\"planning_max\":"<<plan.maximum_length<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-trajectoryStart).count()<<",\"geometry_work\":"<<budget.work-trajectoryWork<<",\"mesh_accepted\":false}"<<std::endl;
  if(failing.empty()){stop="mesh-to-model guidance closed; bidirectional final certification still required";break;}
  if(attempts>=24){stop="unchanged 24-operation bound";break;}
  std::map<Edge,Id> examine;
  for(auto face:failing)for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());
    if(!refused.count(e)&&(admittedEdges.count(e)||e[0]>=firstPoint||e[1]>=firstPoint))examine.emplace(e,face.facet);}
  std::vector<std::pair<double,Edge>> ranked;
  for(auto item:examine) {
    const auto e=item.first;const auto a=points.at(e[0]).p,b=points.at(e[1]).p;
    const Point midpoint{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
    const auto entity=context->Parameters()->Entity(item.second);size_t work=0;bool charged=false;
    double scale=1;for(auto p:{a,b})scale=std::max({scale,std::abs(p.x),std::abs(p.y),std::abs(p.z)});
    const double roundoff=std::nextafter(16*std::numeric_limits<double>::epsilon()*scale,INFINITY);
    try {
      const auto closest=context->Parameters()->Geometry().ClosestCurved(item.second,midpoint,1e-6,
          std::min(SU2NativeGeometry::MappedFacetReference::ClearanceWork,budget.maximum_work-budget.work),budget.maximum_bytes-budget.caller_bytes,
          [&](Id id){return context->Parameters()->Entity(id)==entity;},&work,&budget);
      charged=true;REQUIRE(budget.ChargeWork(work));const bool outside=closest.lower>SU2_TYPE::GetValue(config.GetAdap_Hausd())+roundoff;
      if(outside)ranked.push_back({-closest.lower,e});
      log<<"{\"stage\":\"trajectory_witness\",\"pass\":"<<pass<<",\"edge\":["<<e[0]<<','<<e[1]<<"],\"lower\":"<<closest.lower<<",\"upper\":"<<closest.upper<<",\"outside\":"<<outside<<",\"work\":"<<work<<"}"<<std::endl;
    } catch(const std::exception& x){if(!charged)REQUIRE(budget.ChargeWork(work));log<<"{\"stage\":\"trajectory_uncertain\",\"pass\":"<<pass<<",\"reason\":"<<junction_wp0::Json(x.what())<<"}"<<std::endl;}
  }
  std::sort(ranked.begin(),ranked.end());
  if(ranked.empty()){stop="no remaining admissible demonstrated outside midpoint; face debt unresolved, sampling is not certification";break;}
  const auto edge=ranked.front().second;const auto a=points.at(edge[0]).p,b=points.at(edge[1]).p;
  Node inserted{firstPoint+cuts,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
  auto trim=[](Patch& patch){std::set<Face> faces;std::set<Edge> edges;std::set<Id> vertices;
    for(auto f:patch.surface){std::rotate(f.v.begin(),std::min_element(f.v.begin(),f.v.end()),f.v.end());faces.insert(f.v);vertices.insert(f.v.begin(),f.v.end());
      for(unsigned k=0;k<3;++k){Edge e{f.v[k],f.v[(k+1)%3]};std::sort(e.begin(),e.end());edges.insert(e);}}
    for(auto i=patch.parameters.begin();i!=patch.parameters.end();)if(!vertices.count(i->first))i=patch.parameters.erase(i);else ++i;
    for(auto i=patch.coverage.faces.begin();i!=patch.coverage.faces.end();)if(!faces.count(i->first))i=patch.coverage.faces.erase(i);else ++i;
    for(auto i=patch.coverage.traces.begin();i!=patch.coverage.traces.end();)if(!edges.count(i->first))i=patch.coverage.traces.erase(i);else ++i;};
  Patch local;local.parameters=progressing.parameters;local.coverage=progressing.coverage;std::set<Id> localIds;
  for(auto cell:progressing.cells)if(std::any_of(cell.v.begin(),cell.v.end(),[&](Node n){return n.id==edge[0];})&&
      std::any_of(cell.v.begin(),cell.v.end(),[&](Node n){return n.id==edge[1];})){local.cells.push_back(cell);localIds.insert(cell.id);}
  const auto localIndex=Incidence(local.cells);for(auto face:progressing.surface)if(localIndex.FaceCells(face.v).size()==1)local.surface.push_back(face);
  trim(local);Patch replacement,next;std::string why;bool constructed=false;
  if(local.cells.empty()||local.cells.size()>MaximumCavityCells)why="complete edge-star input exceeds existing bound";
  else if(progressing.cells.size()+local.cells.size()>MaximumReplacementCells)why="complete private patch would exceed 128-cell bound";
  else constructed=SplitSurfaceStrip(local,edge,inserted,{},firstCell+MaximumReplacementCells,geometry,replacement,why);
  ++attempts;
  if(constructed){next=progressing;next.cells.erase(std::remove_if(next.cells.begin(),next.cells.end(),[&](const Cell& c){return localIds.count(c.id);}),next.cells.end());
    next.cells.insert(next.cells.end(),replacement.cells.begin(),replacement.cells.end());std::set<Face> retired;
    for(auto face:local.surface)retired.insert(junction_wp0::Sorted(face.v));
    next.surface.erase(std::remove_if(next.surface.begin(),next.surface.end(),[&](const SurfaceFace& f){return retired.count(junction_wp0::Sorted(f.v));}),next.surface.end());
    next.surface.insert(next.surface.end(),replacement.surface.begin(),replacement.surface.end());
    for(auto row:replacement.parameters)next.parameters.insert_or_assign(row.first,row.second);
    for(auto row:replacement.coverage.faces)next.coverage.faces.insert_or_assign(row.first,row.second);
    for(auto row:replacement.coverage.traces)next.coverage.traces.insert_or_assign(row.first,row.second);
    trim(next);for(size_t k=0;k<next.cells.size();++k)next.cells[k].id=firstCell+k;
    auto artificial=[](const Patch& p){std::set<Face> physical;for(auto f:p.surface)physical.insert(junction_wp0::Sorted(f.v));std::map<Face,Face> out;
      for(auto f:Incidence(p.cells).Boundary())if(!physical.count(junction_wp0::Sorted(f))){std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());out.emplace(junction_wp0::Sorted(f),f);}return out;};
    const auto boundary=artificial(progressing);if(boundary!=artificial(next)){constructed=false;why="complete patch artificial interface changed";}
    auto skin=boundary;for(auto f:next.surface){std::rotate(f.v.begin(),std::min_element(f.v.begin(),f.v.end()),f.v.end());REQUIRE(skin.emplace(junction_wp0::Sorted(f.v),f.v).second);}
    std::vector<Face> expected;for(auto f:skin)expected.push_back(f.second);
    if(constructed)constructed=ValidatePrescribedInterface(progressing.cells,next.cells,expected,why,nullptr,MaximumReplacementCells);
  }
  bool metric=false,parameters=false,contactProtocol=false,contact=true,preserved=false,provisional=false;
  if(constructed) {
    REQUIRE(next.cells.size()<=MaximumReplacementCells);for(auto cell:next.cells)REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
    const auto proposedRows=rowsOf(next);auto nextContext=trial.reference->PrivateParameters(next);REQUIRE(nextContext);
    size_t q=0,j=0,g=0;const auto proposalPlan=measure(next,proposedRows,*nextContext,q,j,g);
    metric=ValidateMetricChange(original.cells.size(),measure(original,support.cells,*trial.reference,q,j,g),next.cells.size(),proposalPlan,MetricChange::GeometryRefine,why);
    parameters=trial.live_parameters->CheckReplacement(original,next,why,ceiling,budget.caller_bytes,&budget);
    contactProtocol=directory.Clearance(trial.mesh,original,next,contact,why,ceiling,held);
    preserved=PreservePassingFirstHeights(original,next,heights,*trial.reference,why);
    FirstHeightReport h;const bool strict=CheckFirstHeight(next,heights,*nextContext,h,why);provisional=strict||(h.measured&&!h.reference_clearance_failures&&!h.missing_cells);
  }
  const bool admitted=constructed&&metric&&parameters&&contactProtocol&&contact&&preserved&&provisional;
  log<<"{\"stage\":\"trajectory_operation\",\"operation\":\"projected_split\",\"attempt\":"<<attempts<<",\"edge\":["<<edge[0]<<','<<edge[1]<<"],\"constructed\":"<<constructed
     <<",\"metric\":"<<metric<<",\"parameters\":"<<parameters<<",\"contact_protocol\":"<<contactProtocol<<",\"contact\":"<<contact<<",\"preserved_heights\":"<<preserved
     <<",\"provisional_height\":"<<provisional<<",\"admitted\":"<<admitted<<",\"reason\":"<<junction_wp0::Json(why)<<"}"<<std::endl;
  if(admitted){progressing=std::move(next);++cuts;refused.clear();}else refused.insert(edge);
}
bool firstRowReached=false,firstRowReady=false;
if(remainingBad==0&&finalPlan.maximum_length<=1.8&&attempts<24) {
  using namespace junction_wp0;
  std::map<Face,Id> physical;for(auto wall:progressing.surface)physical.emplace(Sorted(wall.v),wall.facet);
  for(auto& row:finalRows){const auto faces=CellFaces(row.cell);for(unsigned k=0;k<4;++k){auto at=physical.find(Sorted(faces[k]));if(at!=physical.end())row.physical[k]={true,at->second};}}
  const auto edits=BoundaryParameterState::Changes(original,progressing,trial.live_parameters->Revision());
  REQUIRE(directory.Publish(trial.mesh,dependencies.cells,finalRows,true,reason,ceiling,held,edits,progressing.coverage,&budget));
  trial.reference=trial.reference->WithParameterState(trial.live_parameters)->WithAtlasReservation(budget);
  const auto rowCaller=transfer_memory::Add(budget.caller_bytes,
      transfer_memory::Mul(trial.mesh.size(),6*(sizeof(DonorCell)+640)),
      transfer_memory::Mul(progressing.surface.size(),3*1024));
  REQUIRE(rowCaller<ceiling);struct RestoreCaller {SU2NativeGeometry::AtlasBudget& b;size_t old;~RestoreCaller(){b.caller_bytes=old;}} restoreCaller{budget,budget.caller_bytes};
  budget.caller_bytes=rowCaller;
  auto state=std::make_unique<State>();state->name="actual_connected_patch_trajectory";
  state->mesh=trial.mesh;state->live_parameters=trial.live_parameters;state->reference=trial.reference;
  state->facets=reference->facets;state->markers=reference->marker_names;state->heights=heights;
  for(auto height:heights)state->walls.insert(height.first);
  state->h0=policy.h0;state->core=policy.core;state->layers=policy.full;state->mapped=true;
  state->growth=SU2_TYPE::GetValue(config.GetAdap_BL(0).growth);state->thickness=SU2_TYPE::GetValue(config.GetAdap_BL(0).thickness);state->memory_ceiling=ceiling;
  state->retained_bytes=[&](){return budget.caller_bytes;};state->admit_scratch=[&](size_t n){REQUIRE(transfer_memory::Add(budget.caller_bytes,n)<ceiling);};
  std::vector<DonorCell> donors;donors.reserve(state->mesh.size());for(auto row:state->mesh){DonorCell donor;donor.cell=row.second.cell;donor.sensor=row.second.sensor;donors.push_back(donor);}
  const auto donorCount=donors.size();const auto extensions=AssociatedExtensions(donors,*state->reference);
  state->field=std::make_unique<FrozenField>(std::move(donors),policy.Composition(),true,extensions,policy.Edges(),
      std::make_shared<const FrozenMetricIdentity>(),true,std::max(MaximumDonors,donorCount));
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
  const auto rowResult=dc9::PhaseR2(*state,static_cast<dc9::Rows&>(rows),seconds,rowLog,24-attempts,&affected,&certified);
  size_t passing=0;for(auto key:targets){const auto owner=state->wall_index.find(key);if(owner==state->wall_index.end())continue;
    FirstHeightReport h;std::string why;const bool strict=CheckFirstHeight(PatchOf(*state,{owner->second}),heights,*state->reference,h,why);passing+=strict;
    log<<"{\"stage\":\"trajectory_first_child\",\"face\":["<<key[0]<<','<<key[1]<<','<<key[2]<<"],\"strict_height\":"<<strict<<",\"reason\":"<<Json(why)<<"}"<<std::endl;}
  firstRowReady=!targets.empty()&&passing==targets.size();stop=firstRowReady?"strict local first children reached; full constraints and transition remain":"first-row coverage stalled within unchanged remaining work/deadline";
  std::ofstream("trajectory_first_row.log")<<rowLog.str();
  log<<"{\"stage\":\"trajectory_first_row\",\"work\":"<<rowResult.work<<",\"commits\":"<<rowResult.accepted<<",\"target_faces\":"<<targets.size()<<",\"passing\":"<<passing<<",\"outcome\":"<<Json(rowResult.outcome)<<",\"final_mesh_accepted\":false}"<<std::endl;
}
log<<"{\"stage\":\"trajectory_result\",\"old_cells\":"<<original.cells.size()<<",\"new_cells\":"<<progressing.cells.size()<<",\"attempts\":"<<attempts<<",\"cuts\":"<<cuts
   <<",\"geometry_bad\":"<<remainingBad<<",\"mesh_to_model_guidance_closed\":"<<(remainingBad==0)<<",\"geometry_prerequisite_closed\":"<<(remainingBad==0)
   <<",\"surface_geometry_certified\":false,\"first_row_reached\":"<<firstRowReached<<",\"target_first_height_coverage\":"<<firstRowReady<<",\"stop\":"<<junction_wp0::Json(stop)<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-trajectoryStart).count()
   <<",\"cpu_seconds\":"<<double(std::clock()-trajectoryCpu)/CLOCKS_PER_SEC<<",\"geometry_work\":"<<budget.work-trajectoryWork<<",\"published\":false,\"mesh_accepted\":false}"<<std::endl;
