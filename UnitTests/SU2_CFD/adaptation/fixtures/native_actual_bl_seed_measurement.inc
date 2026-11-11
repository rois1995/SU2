// Bounded actual-context attribution only; no CFD/normalization replay or production acceptance.
SU2NativeBLPipeline3D::detail::PipelineScope pipeline;
constexpr double entryCore=9.9999999999999734e-5; // Preserved362 metric-entry floor, not a production rule.
REQUIRE_FALSE(config.GetAdap_BL_Budget());
auto policy=SU2NativeBLPipeline3D::MakeTargetPolicy(config,*reference,entryCore,-1);
auto lease=policy->ReserveGeometry(budget);
budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,policy->StorageBytes(),
    transfer_memory::Mul(4*MaximumCavityCells,4*MaximumCavityCells*sizeof(BoundaryMetricSample)+1024));
REQUIRE(transfer_memory::Add(budget.caller_bytes,policy->base->GeometryWorkspaceBytes())<=ceiling);
const Tensor control{entryCore,0,0,entryCore,0,entryCore};
std::ofstream costs("actual_bl_seed_measurement.jsonl");REQUIRE(bool(costs));
costs<<std::setprecision(17)<<std::boolalpha;
auto now=[](){return std::chrono::steady_clock::now();};
auto seconds=[&](auto from){return std::chrono::duration<double>(now()-from).count();};
for(size_t context=0;context<count;++context) {
  std::string label;Edge edge{};size_t n=0;input>>label>>edge[0]>>edge[1]>>n;
  REQUIRE(bool(input));REQUIRE(n>0);REQUIRE(n<=MaximumCavityCells);
  std::vector<Id> ids;
  for(size_t i=0;i<n;++i){std::array<Id,4> key;for(auto& id:key)input>>id;std::sort(key.begin(),key.end());ids.push_back(byVertices.at(key));}
  REQUIRE(bool(input));std::sort(ids.begin(),ids.end());REQUIRE(std::adjacent_find(ids.begin(),ids.end())==ids.end());
  std::map<Id,std::vector<BoundaryMetricSample>> priorGuidance;
  std::map<Id,PatchMetric> singleMeasures;
  size_t calls=0,repeated=0,sides=0;double guidanceSeconds=0,patchSeconds=0,plannerSeconds=0,overlaySeconds=0;
  FieldStats totals;const auto workBefore=budget.work;
  const BoundaryMetric guidance=[&](Node node,const PlanarReference& current,Tensor sensor) {
    const auto start=now();auto result=policy->BoundaryGuidance(node,current,sensor,reference->marker_names);
    guidanceSeconds+=seconds(start);++calls;sides+=result.size();
    const auto inserted=priorGuidance.emplace(node.id,result);
    if(!inserted.second) {
      ++repeated;const auto& before=inserted.first->second;REQUIRE(before.size()==result.size());
      for(size_t s=0;s<result.size();++s) {
        CHECK(SameMetric(before[s].tensor,result[s].tensor));
        CHECK(before[s].normal.x==result[s].normal.x);CHECK(before[s].normal.y==result[s].normal.y);CHECK(before[s].normal.z==result[s].normal.z);
        CHECK(before[s].strip_rule.x==result[s].strip_rule.x);CHECK(before[s].strip_rule.y==result[s].strip_rule.y);CHECK(before[s].strip_rule.z==result[s].strip_rule.z);
      }
    }
    return result;
  };
  // Production splits16-cell seed chunks at MaximumCavityCells/6; no publication occurs in a measurement batch.
  for(size_t first=0;first<ids.size();) {
    const size_t end=std::min({first+16-first%16,first+MaximumCavityCells/6,ids.size()});
    std::vector<DonorCell> donors;
    for(size_t i=first;i<end;++i){DonorCell d;d.cell=imported.cells.at(ids[i]).cell;d.sensor.fill(control);donors.push_back(d);}
    auto identity=std::make_shared<const FrozenMetricIdentity>(FrozenMetricIdentity::Scope::SurfacePlanning);
    FrozenField field(std::move(donors),{},true,{},{},identity,true,MaximumDonors,true);
    for(size_t i=first;i<end;++i) {
      budget.CheckTime();auto start=now();const auto patch=patchOf({ids[i]});patchSeconds+=seconds(start);
      start=now();const auto planned=MeasureSurfacePlanning(patch,field,*scope,heights,guidance);plannerSeconds+=seconds(start);
      CHECK_FALSE(planned.shape_measured);
      singleMeasures.emplace(ids[i],planned);
      // Independent context-preparation witness; this time is NOT nested in plannerSeconds.
      start=now();auto overlay=scope->PrivateParameters(patch,&budget);overlaySeconds+=seconds(start);REQUIRE(overlay);
    }
    const auto& s=field.Statistics();totals.build_seconds+=s.build_seconds;totals.query_seconds+=s.query_seconds;
    totals.edge_seconds+=s.edge_seconds;totals.search_seconds+=s.search_seconds;totals.requests+=s.requests;
    first=end;
  }
  costs<<"{\"context\":"<<std::quoted(label)<<",\"cells\":"<<n<<",\"guidance_calls\":"<<calls
       <<",\"unique_nodes\":"<<priorGuidance.size()<<",\"identical_repeated_node_calls\":"<<repeated<<",\"sided_samples\":"<<sides
       <<",\"patch_export_seconds\":"<<patchSeconds<<",\"planner_seconds\":"<<plannerSeconds
       <<",\"guidance_nested_seconds\":"<<guidanceSeconds<<",\"core_query_nested_seconds\":"<<totals.query_seconds
       <<",\"core_edge_nested_seconds\":"<<totals.edge_seconds<<",\"field_build_seconds\":"<<totals.build_seconds
       <<",\"independent_overlay_seconds\":"<<overlaySeconds<<",\"geometry_work\":"<<budget.work-workBefore
       <<",\"seed_core_witness_only\":true,\"production_target_certified\":false}"<<std::endl;
  // Exercise the actual census caller too; one round/work unit prevents an adaptation campaign.
  Owned selected;std::vector<DonorCell> source;
  for(Id id:ids){auto row=imported.cells.at(id);row.sensor_present=true;row.sensor.fill(control);selected.emplace(id,row);
    DonorCell d;d.cell=row.cell;d.sensor=row.sensor;source.push_back(d);}
  EngineOptions options;options.maximum_work=1;options.maximum_rounds=1;options.maximum_seconds=20;
  options.memory_ceiling=ceiling;options.caller_bytes=budget.caller_bytes;options.wall_heights=heights;
  options.surface_tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());options.timings=true;
  // A deliberately exhausted child window must not poison the next independent diagnostic context.
  auto childBudget=budget;const auto childWork=childBudget.work;
  options.shared_geometry_budget=&childBudget;
  options.geometry_workspace_bytes=transfer_memory::Add(std::max(policy->full->GeometryWorkspaceBytes(),policy->base->GeometryWorkspaceBytes()),atlasScratch);
  options.geometry_reservation=[&](auto& admitted){return policy->ReserveGeometry(admitted);};
  options.stage_observer_bytes=size_t(1)<<20;
  options.stage_observer=[&](const StageProgressView& view) {
    for(const auto& row:view.working) {
      const auto expected=singleMeasures.find(row.first);
      if(expected==singleMeasures.end()||row.second.version!=imported.cells.at(row.first).version)continue;
      CellDiagnostic cached;if(!view.cell_diagnostic(row.first,cached))continue;
      CHECK_FALSE(cached.q_measured);CHECK_FALSE(cached.j_measured);
      CHECK(cached.size_bad==(expected->second.maximum_length>MaximumMetricEdge));
      for(unsigned edge=0;edge<6;++edge)CHECK(cached.lengths[edge]==expected->second.edges.at(cached.edges[edge]).length);
    }
  };
  size_t engineCalls=0;double engineGuidanceSeconds=0;
  const auto measured=PrepareSurface(world,selected,source,*scope,
      [&](Node node,const PlanarReference& current,Tensor sensor) {
        const auto start=now();auto result=policy->BoundaryGuidance(node,current,sensor,reference->marker_names);
        engineGuidanceSeconds+=seconds(start);++engineCalls;return result;
      },options);
  REQUIRE(childBudget.work>=childWork);REQUIRE(budget.ChargeWork(childBudget.work-childWork));
  budget.peak_bytes=std::max(budget.peak_bytes,childBudget.peak_bytes);
  const auto& s=measured.stats;
  costs<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"bounded_engine\",\"seed_cells\":"<<s.initial_cells
       <<",\"cached_cells\":"<<s.cached_measured_cells<<",\"seed_seconds\":"<<s.seed_seconds
       <<",\"measurement_cpu_seconds\":"<<s.measurement_cpu_seconds<<",\"field_build_seconds\":"<<s.field_build_seconds
       <<",\"field_query_seconds\":"<<s.field_query_seconds<<",\"field_search_seconds\":"<<s.field_search_seconds
       <<",\"edge_seconds\":"<<s.edge_seconds<<",\"edge_integral_nested_seconds\":"<<s.edge_integral_seconds
       <<",\"collapse_geometry_nested_seconds\":"<<s.collapse_geometry_seconds<<",\"total_seconds\":"<<s.total_seconds
       <<",\"guidance_calls\":"<<engineCalls<<",\"guidance_nested_seconds\":"<<engineGuidanceSeconds
       <<",\"reason\":"<<std::quoted(measured.reason)<<",\"production_acceptance\":false}"<<std::endl;
}
input>>std::ws;REQUIRE(input.eof());
