// Terminal saved state, not an exact replay of an unavailable private trial payload.
const auto acceptedParameters=current.live_parameters->Payload();
Directory directory(world,false,current.live_parameters.get());
REQUIRE(directory.Build(current.mesh,reason,ceiling,retained));
budget.caller_bytes=transfer_memory::Add(retained,directory.ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
struct ShapeRestore {ShapeAdmission old=GetShapeAdmission();~ShapeRestore(){SetShapeAdmission(old);}} restore;
SetShapeAdmission(ShapeAdmission::ValidityOnly);
std::vector<std::pair<std::string,Edge>> edges{{"recorded_edge_r5",{7907,7908}}};
auto wallEdges=[&](Id vertex) {
  std::set<Edge> result;const auto patch=junction_wp0::PatchOf(current,star(vertex));
  for(const auto& face:patch.surface)if(heights.count(current.reference->Get(face.facet).marker))
    for(unsigned k=0;k<3;++k) {
      Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());
      if(e[0]==vertex||e[1]==vertex)result.insert(e);
    }
  REQUIRE_FALSE(result.empty());return result;
};
// R4's log retains only its first endpoint: inspect two actual adjacent edges, without claiming identity.
const auto r4=wallEdges(23829);size_t selected=0;
for(auto edge:r4){edges.push_back({"r4_adjacent_candidate_"+std::to_string(selected),edge});if(++selected==2)break;}
edges.push_back({"smooth_control",*wallEdges(smooth).begin()});
edges.push_back({"leading_edge_control",*wallEdges(4219).begin()});
Id nextNode=0,nextCell=0;
for(const auto& row:current.mesh){nextCell=std::max(nextCell,row.first);for(auto n:row.second.cell.v)nextNode=std::max(nextNode,n.id);}
++nextNode;++nextCell;
for(const auto& entry:edges) {
  const auto edge=entry.second;
  auto dependencies=directory.Lookup({Feature{{edge[0],edge[1],0},2}},ceiling,retained);REQUIRE(dependencies.valid);
  auto imported=directory.Fetch(dependencies,current.mesh,ceiling,retained);REQUIRE(imported.valid);
  std::vector<Id> edgeIds;for(const auto& row:imported.cells)edgeIds.push_back(row.cell.id);
  REQUIRE_FALSE(edgeIds.empty());const auto edgePatch=junction_wp0::PatchOf(current,edgeIds);
  std::map<Id,Node> nodes;for(const auto& cell:edgePatch.cells)for(auto n:cell.v)nodes.emplace(n.id,n);
  const auto a=nodes.at(edge[0]).p,b=nodes.at(edge[1]).p;
  Node point{nextNode,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
  // Discover every wall-apex star before reserving any coupled trial.
  std::vector<Feature> features{{{edge[0],edge[1],0},2}};
  for(const auto& face:edgePatch.surface)if(heights.count(current.reference->Get(face.facet).marker)&&
      std::find(face.v.begin(),face.v.end(),edge[0])!=face.v.end()&&std::find(face.v.begin(),face.v.end(),edge[1])!=face.v.end())
    for(const auto& cell:edgePatch.cells) {
      unsigned count=0;Id apex=0;for(auto n:cell.v)if(std::find(face.v.begin(),face.v.end(),n.id)!=face.v.end())++count;else apex=n.id;
      if(count==3)features.push_back({{apex,0,0},1});
    }
  const auto joined=directory.Lookup(features,ceiling,retained);
  Imported complete;if(joined.valid)complete=directory.Fetch(joined,current.mesh,ceiling,retained);
  auto observe=[&](const Patch& patch,const std::vector<OwnedCell>& rows,const char* operation,const char* side) {
    const auto start=std::chrono::steady_clock::now();const auto cpu=std::clock();const auto work=budget.work;
    auto privateGeometry=current.reference->PrivateParameters(patch);const auto& geometry=privateGeometry?*privateGeometry:*current.reference;
    FirstHeightReport height;std::string why;const bool strict=CheckFirstHeight(patch,heights,geometry,height,why);
    log<<"{\"stage\":\"height\",\"context\":"<<std::quoted(entry.first)<<",\"edge\":["<<edge[0]<<','<<edge[1]
       <<"],\"operator\":"<<std::quoted(operation)<<",\"side\":"<<std::quoted(side)<<",\"cells\":"<<patch.cells.size()
       <<",\"strict_height\":"<<strict<<",\"measured\":"<<height.measured<<",\"height_failures\":"<<height.height_failures
       <<",\"clearance_failures\":"<<height.reference_clearance_failures<<",\"minimum_clearance_ratio\":"<<(std::isfinite(height.minimum_reference_clearance_ratio)?std::to_string(height.minimum_reference_clearance_ratio):"null")
       <<",\"facet\":"<<height.reference_clearance_facet<<",\"cell\":"<<height.reference_clearance_cell
       <<",\"apex\":["<<height.reference_clearance_apex.x<<','<<height.reference_clearance_apex.y<<','<<height.reference_clearance_apex.z
       <<"],\"geometry_work\":"<<budget.work-work<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()
       <<",\"cpu_seconds\":"<<double(std::clock()-cpu)/CLOCKS_PER_SEC<<",\"reason\":"<<junction_wp0::Json(why)<<"}"<<std::endl;
    if(height.reference_clearance_failures)for(const double ratio:{1e-4,1e-6,1e-8}) {
      const auto facet=height.reference_clearance_facet;const auto query=height.reference_clearance_apex;
      const double h=heights.at(geometry.Get(facet).marker);const auto queryStart=std::chrono::steady_clock::now();
      size_t attempted=0;bool charged=false;
      try {
        const auto cap=std::min(SU2NativeGeometry::MappedFacetReference::ClearanceWork,budget.maximum_work-budget.work);
        const auto result=geometry.Parameters()->Geometry().ClosestCurved(facet,query,h*ratio,cap,
            budget.maximum_bytes-budget.caller_bytes,{},&attempted,&budget);
        charged=true;REQUIRE(budget.ChargeWork(attempted));
        const long double signedCandidate=-((long double)(query.x-result.point.x)*result.normal.x+
            (long double)(query.y-result.point.y)*result.normal.y+(long double)(query.z-result.point.z)*result.normal.z);
        log<<"{\"stage\":\"clearance_refinement\",\"context\":"<<std::quoted(entry.first)<<",\"operator\":"<<std::quoted(operation)
           <<",\"side\":"<<std::quoted(side)<<",\"precision_over_h\":"<<ratio<<",\"lower\":"<<result.lower<<",\"upper\":"<<result.upper
           <<",\"clearance_lower_over_h\":"<<result.clearance_lower/h<<",\"signed_candidate_over_h\":"<<signedCandidate/h
           <<",\"candidate_is_sign_certificate\":false,\"work\":"<<attempted<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-queryStart).count()<<"}"<<std::endl;
      }catch(const std::exception& e) {
        if(!charged)budget.ChargeWork(attempted);
        log<<"{\"stage\":\"clearance_refinement_refused\",\"context\":"<<std::quoted(entry.first)<<",\"operator\":"<<std::quoted(operation)
           <<",\"side\":"<<std::quoted(side)<<",\"reason\":"<<junction_wp0::Json(e.what())<<"}"<<std::endl;break;
      }
    }
    auto donors=source.ImportCellsLocal(patch.cells,rows,ceiling,budget.caller_bytes);REQUIRE(donors.valid);
    const auto extensions=AssociatedExtensions(donors.cells,geometry);
    FrozenField core(std::move(donors.cells),{},true,extensions,{},identity,true,MaximumDonors,true);
    const BoundaryMetric boundary=[&](Node node,const PlanarReference& g,Tensor m){return policy.BoundaryGuidance(node,g,m,reference->marker_names);};
    const auto measured=MeasureSurfacePlanning(patch,core,geometry,heights,boundary);
    const auto guidance=GeometryGuidanceExcess(patch,geometry,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
    size_t qBad=0,jBad=0;const auto compose=policy.Composition();
    for(const auto& cell:patch.cells) {
      const auto x=Centroid(cell);const auto m=compose(x,core.Sensor(x));
      const auto shape=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},m);
      qBad+=shape.mean_ratio<MinimumMeanRatio;jBad+=shape.minimum_scaled_jacobian<MinimumScaledJacobian;
    }
    size_t shortEdges=0,tinyEdges=0,longEdges=0;long double minimum=INFINITY,maximum=0;
    for(const auto& e:measured.edges){minimum=std::min(minimum,e.second.length);maximum=std::max(maximum,e.second.length);shortEdges+=e.second.length<.9;tinyEdges+=e.second.length<.5;longEdges+=e.second.length>MaximumMetricEdge;}
    log<<"{\"stage\":\"patch_measurement\",\"context\":"<<std::quoted(entry.first)<<",\"operator\":"<<std::quoted(operation)
       <<",\"side\":"<<std::quoted(side)<<",\"geometry_complete\":"<<guidance.complete<<",\"geometry_excess\":"<<guidance.excess
       <<",\"planning_min_edge\":"<<minimum<<",\"planning_max_edge\":"<<maximum<<",\"short_edges\":"<<shortEdges<<",\"tiny_edges\":"<<tinyEdges
       <<",\"centroid_q_bad\":"<<qBad<<",\"centroid_J_bad\":"<<jBad<<",\"long_edges\":"<<longEdges<<",\"full_target_certificate\":false,\"mesh_accepted\":false}"<<std::endl;
  };
  observe(edgePatch,imported.cells,"input","old");
  if(complete.valid&&complete.cells.size()<=MaximumCavityCells) {
#include "native_actual_surface_apex_probe.inc"
  }
  for(unsigned method=0;method<5;++method) {
    const bool coupled=method>=2;const auto& support=coupled?complete:imported;
    const char* name=method==0?"SplitEdge":method==1?"InsertEdgeCavity":method==2?"RefitWallSplit":method==3?"ReconstructWallSplit":"ReconstructSplitPatch";
    if(!support.valid||support.cells.size()>MaximumCavityCells) {
      log<<"{\"stage\":\"support_refused\",\"context\":"<<std::quoted(entry.first)<<",\"operator\":"<<std::quoted(name)<<"}"<<std::endl;continue;
    }
    std::vector<Id> ids;for(const auto& row:support.cells)ids.push_back(row.cell.id);const auto old=junction_wp0::PatchOf(current,ids);
    REQUIRE(directory.Reserve(support,true,reason,ceiling,retained));
    Patch fresh;Face obstruction{};Feature blocked;std::string why;bool built=false;const auto start=std::chrono::steady_clock::now();
    if(method==0)built=SplitEdge(old,edge,point,nextCell,*current.reference,fresh,why);
    if(method==1)built=InsertEdgeCavity(old,edge,point,nextCell,*current.reference,fresh,why);
    if(method==2)built=RefitWallSplit(old,edge,point,nextNode+1,nextCell,heights,*current.reference,fresh,blocked,why);
    if(method==3)built=ReconstructWallSplit(old,edge,point,nextNode+1,nextCell,heights,*current.reference,fresh,obstruction,why,nullptr,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
    if(method==4) {
      auto donors=source.ImportCellsLocal(old.cells,support.cells,ceiling,budget.caller_bytes);REQUIRE(donors.valid);
      const auto extensions=AssociatedExtensions(donors.cells,*current.reference);
      FrozenField core(std::move(donors.cells),{},true,extensions,{},identity,true,MaximumDonors,true);
      const auto placement=core.Sensor(Centroid(old.cells.front()));
      built=ReconstructSplitPatch(old,edge,point,{nextNode+1,{}},placement,nextCell,*current.reference,fresh,obstruction,why,nullptr,heights);
    }
    log<<"{\"stage\":\"operator\",\"context\":"<<std::quoted(entry.first)<<",\"edge\":["<<edge[0]<<','<<edge[1]<<"],\"operator\":"<<std::quoted(name)
       <<",\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<fresh.cells.size()<<",\"constructed\":"<<built
       <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<",\"reason\":"<<junction_wp0::Json(why)<<"}"<<std::endl;
    if(built){for(const auto& c:fresh.cells)REQUIRE(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);observe(fresh,support.cells,name,"fresh");}
    REQUIRE(current.live_parameters->Payload()==acceptedParameters);
    nextNode+=MaximumReplacementCells+2;point.id=nextNode;nextCell+=MaximumReplacementCells;
  }
}
NativeStageCapture::WriteOwned("correction_unchanged_owned.bin",current.mesh);
