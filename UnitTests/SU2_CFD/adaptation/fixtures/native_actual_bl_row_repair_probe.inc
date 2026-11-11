// One selected actual cavity, not a global BL acceptance or an unconstrained artificial-cut test.
Apply(*state,prepared.ids,prepared.fresh);
directory.reset();directory=std::make_unique<Directory>(world);
REQUIRE(directory->Build(state->mesh,why,ceiling,retained));
auto context=state->reference->PrivateParameters(prepared.fresh,&budget);
const auto& geometry=context?*context:*state->reference;
auto capture=[&](const char* stage,const Patch& patch) {
  const std::string stem=std::string("prepared_repair_")+stage;
  NativeTransactionCapture::Write(stem+".bin",NativeTransactionCapture::PatchBytes(patch));
  std::ofstream cells(stem+"_frames.jsonl");REQUIRE(bool(cells));cells<<std::setprecision(17)<<std::boolalpha;
  for(const auto& cell:patch.cells) {
    const auto p=Centroid(cell);const auto tensor=state->field->Query(p).target;
    const auto metric=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},tensor);
    const su2double x[3]={p.x,p.y,p.z};const auto sample=policy->base->Evaluate(0,x,policy->core);
    cells<<"{\"cell\":"<<cell.id<<",\"centroid\":["<<p.x<<','<<p.y<<','<<p.z<<"],\"metric\":["
         <<tensor.xx<<','<<tensor.xy<<','<<tensor.xz<<','<<tensor.yy<<','<<tensor.yz<<','<<tensor.zz
         <<"],\"q\":"<<metric.mean_ratio<<",\"J\":"<<metric.minimum_scaled_jacobian
         <<",\"wall_child\":"<<layer2d::IsFirstChild(*state,patch,cell)<<",\"wall0_distance\":"<<SU2_TYPE::GetValue(sample.distance)
         <<",\"wall0_normal\":["<<SU2_TYPE::GetValue(sample.normal[0])<<','<<SU2_TYPE::GetValue(sample.normal[1])<<','<<SU2_TYPE::GetValue(sample.normal[2])
         <<"],\"wall0_hn\":"<<SU2_TYPE::GetValue(sample.hn)<<",\"control_target_only\":true}"<<std::endl;
  }
  cells.close();REQUIRE(bool(cells));
};
const auto parameters=state->live_parameters->Payload();REQUIRE(parameters.size()<NativeTransactionCapture::limit);
NativeTransactionCapture::Write("prepared_repair_parameters.bin",parameters);
capture("before",prepared.fresh);
Patch candidate=prepared.fresh;
auto validate=[&](const Patch& proposal,std::string& reason) {
  if(proposal.cells.empty()||proposal.cells.size()>MaximumReplacementCells){reason="replacement bound";return false;}
  auto privateContext=state->reference->PrivateParameters(proposal,&budget);const auto& ref=privateContext?*privateContext:*state->reference;
  std::map<Id,Point> nodes;for(const auto& cell:proposal.cells) {
    if(!dc9::Positive(cell.v)){reason="nonpositive cell";return false;}
    for(auto node:cell.v)nodes.emplace(node.id,node.p);
  }
  for(const auto& f:proposal.surface) {
    const std::array<Point,3> triangle{nodes.at(f.v[0]),nodes.at(f.v[1]),nodes.at(f.v[2])};
    if(!ref.ContainsFace(f.facet,triangle)||ref.SurfaceError(f.facet,triangle)>SU2_TYPE::GetValue(config.GetAdap_Hausd())){reason="geometry";return false;}
  }
  FirstHeightReport measured;
  if(!CheckFirstHeight(proposal,heights,ref,measured,reason))return false;
  std::set<Face> oldPhysical;for(const auto& f:prepared.fresh.surface)oldPhysical.insert(dc9::Sorted(f.v));
  std::map<Face,Face> expected;
  for(auto f:Incidence(prepared.fresh.cells).Boundary())if(!oldPhysical.count(dc9::Sorted(f)))expected.emplace(dc9::Sorted(f),f);
  for(const auto& f:proposal.surface)expected.emplace(dc9::Sorted(f.v),f.v);
  std::vector<Face> skin;for(const auto& f:expected)skin.push_back(f.second);
  if(!ValidatePrescribedInterface(prepared.fresh.cells,proposal.cells,skin,reason))return false;
  bool clear=true;if(!directory->Clearance(state->mesh,prepared.fresh,proposal,clear,reason,ceiling,retained)||!clear)return false;
  return true;
};
const auto repairStart=std::chrono::steady_clock::now();const auto repairWork=budget.work;
Patch repaired;ConnectivityRepairReport report;std::string repairReason;
const bool improved=RepairMetricConnectivity(candidate,*state->field,geometry,heights,state->next_cell,repaired,report,repairReason);
bool admitted=false;if(improved)admitted=validate(repaired,repairReason);
if(admitted)candidate=std::move(repaired);
const auto repairedShape=composedShapes(candidate);
frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"prepared_connectivity_repair\",\"improved\":"<<improved
        <<",\"geometry_height_contact\":"<<admitted<<",\"trials\":"<<report.trials<<",\"flips\":"<<report.flips
        <<",\"moves\":"<<report.moves<<",\"removals\":"<<report.removals<<",\"coordinated\":"<<report.coordinated
        <<",\"height_rejections\":"<<report.move_height_rejected<<",\"geometry_rejections\":"<<report.move_geometry_rejected
        <<",\"loss_rejections\":"<<report.move_loss_rejected<<",\"before_loss\":"<<report.before_loss<<",\"after_loss\":"<<report.after_loss
        <<",\"blocked_vertex\":"<<report.blocked_interior_vertex<<",\"q_bad\":"<<repairedShape[0]<<",\"J_bad\":"<<repairedShape[1]
        <<",\"unknown\":"<<repairedShape[2]<<",\"reason\":"<<std::quoted(repairReason)<<",\"geometry_work\":"<<budget.work-repairWork
        <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-repairStart).count()<<",\"accepted\":false}"<<std::endl;
capture("connectivity",candidate);
if(report.blocked_interior_vertex!=UINT64_MAX) {
  const auto star=dc9::StarOf(*state,report.blocked_interior_vertex);size_t outside=0;
  std::set<Id> localCells;for(const auto& cell:prepared.fresh.cells)localCells.insert(cell.id);
  for(Id cell:star)outside+=!localCells.count(cell);
  frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"repair_interface_hint\",\"vertex\":"<<report.blocked_interior_vertex
          <<",\"global_star_cells\":"<<star.size()<<",\"outside_candidate_cells\":"<<outside<<",\"support_exported\":"
          <<(!star.empty()&&star.size()<=MaximumCavityCells)<<"}"<<std::endl;
  if(!star.empty()&&star.size()<=MaximumCavityCells) {
    const auto neighbors=PatchOf(*state,std::vector<Id>(star.begin(),star.end()));capture("neighbor_star",neighbors);
  }
}

Owned local;std::map<Face,Id> physical;
for(const auto& f:candidate.surface)physical.emplace(dc9::Sorted(f.v),f.facet);
for(const auto& cell:candidate.cells) {
  OwnedCell row;row.cell=cell;row.sensor_present=true;row.sensor.fill(control);const auto faces=CellFaces(cell);
  for(unsigned k=0;k<4;++k){const auto found=physical.find(dc9::Sorted(faces[k]));if(found!=physical.end())row.physical[k]={true,found->second};}
  REQUIRE(local.emplace(cell.id,row).second);
}
auto privateContext=state->reference->PrivateParameters(candidate,&budget);const auto& optimizerGeometry=privateContext?*privateContext:*state->reference;
OptimizeOptions options;options.geometry=&optimizerGeometry;options.wall_heights=heights;
options.surface_tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());options.maximum_sweeps=4;options.maximum_work=64;
options.maximum_seconds=30;options.cache_entries=8*MaximumReplacementCells;options.boundary=false;
// Only the actual cavity interface is fixed; no row-apex, column or first-child registry freeze.
std::map<Face,size_t> incidence;
for(const auto& cell:candidate.cells)for(auto f:CellFaces(cell))++incidence[dc9::Sorted(f)];
for(const auto& f:incidence)if(f.second==1&&!physical.count(f.first))options.frozen_vertices.insert(f.first.begin(),f.first.end());
const RegionMetric target(*state->field);Id nextPoint=state->next_node,nextElement=state->next_cell;
for(const auto& cell:candidate.cells){nextElement=std::max(nextElement,cell.id+1);for(auto node:cell.v)nextPoint=std::max(nextPoint,node.id+1);}
OptimizeReport optimized;const bool changed=OptimizeMetricMesh(local,target,nextPoint,nextElement,options,optimized);
Patch proposal;proposal.parameters=candidate.parameters;proposal.coverage=candidate.coverage;
for(const auto& row:local) {
  proposal.cells.push_back(row.second.cell);const auto faces=CellFaces(row.second.cell);
  for(unsigned k=0;k<4;++k)if(row.second.physical[k].present)proposal.surface.push_back({faces[k],row.second.physical[k].facet});
}
for(const auto& row:optimized.parameters)proposal.parameters.insert_or_assign(row.first,row.second);
for(const auto& row:optimized.coverage.faces)proposal.coverage.faces.insert_or_assign(row.first,row.second);
for(const auto& row:optimized.coverage.traces)proposal.coverage.traces.insert_or_assign(row.first,row.second);
std::set<Face> retainedFaces;std::set<Edge> retainedEdges;
for(auto f:proposal.surface) {
  std::rotate(f.v.begin(),std::min_element(f.v.begin(),f.v.end()),f.v.end());retainedFaces.insert(f.v);
  for(unsigned k=0;k<3;++k){Edge e{f.v[k],f.v[(k+1)%3]};std::sort(e.begin(),e.end());retainedEdges.insert(e);}
}
for(auto i=proposal.coverage.faces.begin();i!=proposal.coverage.faces.end();)if(!retainedFaces.count(i->first))i=proposal.coverage.faces.erase(i);else ++i;
for(auto i=proposal.coverage.traces.begin();i!=proposal.coverage.traces.end();)if(!retainedEdges.count(i->first))i=proposal.coverage.traces.erase(i);else ++i;
auto finalContext=optimizerGeometry.PrivateParameters(proposal,&budget);if(finalContext)finalContext->ExportParameters(proposal);
std::string optimizedReason;const bool finalValid=validate(proposal,optimizedReason);const auto finalShape=composedShapes(proposal);
frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"prepared_optimizer\",\"changed\":"<<changed
        <<",\"geometry_height_contact\":"<<finalValid<<",\"trials\":"<<optimized.trials<<",\"moves\":"<<optimized.moves
        <<",\"swaps\":"<<optimized.edge_removals+optimized.face_swaps<<",\"collapses\":"<<optimized.collapses
        <<",\"insertions\":"<<optimized.shell_insertions+optimized.neighbourhood_insertions+optimized.length_insertions
        <<",\"rejected_frozen\":"<<optimized.rejected_frozen<<",\"rejected_interface\":"<<optimized.rejected_interface
        <<",\"rejected_quality\":"<<optimized.rejected_quality<<",\"rejected_edge\":"<<optimized.rejected_edge
        <<",\"q_bad\":"<<finalShape[0]<<",\"J_bad\":"<<finalShape[1]<<",\"unknown\":"<<finalShape[2]
        <<",\"stop\":"<<std::quoted(optimized.stop)<<",\"reason\":"<<std::quoted(optimizedReason)
        <<",\"seconds\":"<<optimized.seconds<<",\"accepted\":false}"<<std::endl;
capture("optimizer",proposal);
describe(label,"prepared_repair_final",proposal);
