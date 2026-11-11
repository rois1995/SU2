// Included inside the existing actual-M6 loader: selection changes, not geometry or CFD provenance.
using namespace junction_wp0;
SU2NativeBLPipeline3D::detail::PipelineScope pipeline;
auto policy=SU2NativeBLPipeline3D::MakeTargetPolicy(config,*reference,1e-4,-1);
policy->Update(1e-4,SU2_TYPE::GetValue(config.GetAdap_Hgrad()));
auto geometryLease=policy->ReserveGeometry(budget);
dc10::optionA=policy->A;dc10::probeBase=policy->base;dc10::probeFull=policy->full;
dc9::validityOnlyConstruction=true;dc9::markerRoles=policy->roles;
const Tensor control{1e-4,0,0,1e-4,0,1e-4};
auto state=std::make_unique<State>();state->name="actual_curved_front_probe";
state->facets=reference->facets;state->markers=reference->marker_names;
state->heights=heights;for(const auto& height:heights)state->walls.insert(height.first);
state->h0=policy->h0;state->core=policy->core;state->layers=policy->full;state->mapped=true;
state->growth=SU2_TYPE::GetValue(config.GetAdap_BL(0).growth);
state->thickness=SU2_TYPE::GetValue(config.GetAdap_BL(0).thickness);state->memory_ceiling=ceiling;
const auto priorCaller=budget.caller_bytes;
const auto retained=transfer_memory::Add(priorCaller,policy->StorageBytes(),
    transfer_memory::Mul(imported.cells.size(),3*(sizeof(DonorCell)+640)),size_t(256)<<20);
REQUIRE(retained<ceiling);budget.caller_bytes=retained;
std::vector<DonorCell> donors;donors.reserve(imported.cells.size());
for(const auto& row:imported.cells){DonorCell d;d.cell=row.second.cell;d.sensor.fill(control);donors.push_back(d);}
const auto extensions=AssociatedExtensions(donors,*scope);
state->field=std::make_unique<FrozenField>(std::move(donors),policy->Composition(),true,extensions,policy->Edges(),
    std::make_shared<const FrozenMetricIdentity>(),true,std::max(MaximumDonors,imported.cells.size()));
state->retained_bytes=[&]{return retained;};
state->admit_scratch=[&](size_t bytes){if(transfer_memory::Add(retained,bytes)>ceiling)
    throw std::length_error("Actual front probe exceeds working-storage admission.");};
dc12::DirField directions;
struct ResetDirections {~ResetDirections(){dc9::ensureDirectionField={};dc9::directionField=nullptr;}} resetDirections;
dc9::ensureDirectionField=[&](State& current) {
  if(current.direction_revision==current.boundary_revision)return;
  budget.CheckTime();directions=dc12::ComputeDirectionField(current);
  current.direction_revision=current.boundary_revision;dc9::directionField=&directions.d;
};
std::ofstream frontLog("actual_bl_front.jsonl");REQUIRE(bool(frontLog));frontLog<<std::setprecision(17)<<std::boolalpha;
auto pointJSON=[&](Point p){frontLog<<'['<<p.x<<','<<p.y<<','<<p.z<<']';};
size_t built=0,admitted=0;
for(size_t index=0;index<count;++index) {
  std::string label;Edge edge{};size_t n=0;input>>label>>edge[0]>>edge[1]>>n;
  REQUIRE(bool(input));REQUIRE(n>0);REQUIRE(n<=MaximumCavityCells);
  std::set<Id> selected;
  for(size_t i=0;i<n;++i){std::array<Id,4> key;for(auto& id:key)input>>id;std::sort(key.begin(),key.end());selected.insert(byVertices.at(key));}
  REQUIRE(selected.size()==n);
  state->mesh=imported.cells;for(auto& row:state->mesh){row.second.sensor_present=true;row.second.sensor.fill(control);}
  state->live_parameters=std::make_shared<BoundaryParameterState>(*reference->original->Parameters());
  state->reference=scope->WithParameterState(state->live_parameters)->WithAtlasReservation(budget);
  state->overlay.clear();Prepare(*state);dc12::ApplyWallPolicies(*state);state->heights.tolerance=heights.tolerance;
  dc9::DetectCreases(*state,policy->roles);dc9::ensureDirectionField(*state);
  const auto old=PatchOf(*state,std::vector<Id>(selected.begin(),selected.end()));
  std::vector<DonorCell> localDonors;for(const auto& c:old.cells){DonorCell d;d.cell=c;d.sensor.fill(control);localDonors.push_back(d);}
  auto identity=std::make_shared<const FrozenMetricIdentity>(FrozenMetricIdentity::Scope::SurfacePlanning);
  FrozenField core(std::move(localDonors),{},true,{}, {},identity);
  const auto beforeWork=budget.work;const auto t0=std::chrono::steady_clock::now();
  const auto beforeField=dc9::CountersOf(*state);
  const auto beforeRawField=state->field->Statistics();
  try {
    const auto plan=MeasureSurfacePlanning(old,core,*state->reference,heights,
        [&](Node node,const PlanarReference& context,Tensor sensor){return policy->BoundaryGuidance(node,context,sensor,reference->marker_names);});
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_planning\",\"maximum_requested_length\":"<<plan.maximum_length
            <<",\"core_control_only\":true,\"full_target_certified\":false,\"geometry_work\":"<<budget.work-beforeWork<<"}"<<std::endl;
    for(const auto& face:old.surface)if(heights.count(state->reference->Get(face.facet).marker)) {
      const auto& geometry=state->reference->Parameters()->Geometry();const auto vertex=face.v[0];
      const auto p=NodePoint(*state,vertex);const auto* association=state->reference->Parameter(p);REQUIRE(association);
      if(!budget.ChargeWork(1))throw std::length_error("Actual curvature probe exhausted geometry work.");
      const auto physical=geometry.EvaluateSurface(face.facet,association->binding);
      const auto guidance=policy->BoundaryGuidance({vertex,p},*state->reference,control,reference->marker_names);
      const auto direction=dc9::DirectionAt(*state,vertex);
      double tangent1=0,tangent2=0;
      for(const auto& side:guidance) {
        auto density=[&](Point direction) {
          const auto n=Of(side.normal),d=Of(direction);const auto tangent=d-(Dot(d,n)/Dot(n,n))*n;
          return double(MetricNorm({0,0,0},Pt(tangent),side.tensor));
        };
        tangent1=std::max(tangent1,density(physical.direction1));tangent2=std::max(tangent2,density(physical.direction2));
      }
      frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"physical_target_directions\",\"facet\":"<<face.facet
              <<",\"vertex\":"<<vertex<<",\"point\":";pointJSON(p);
      frontLog<<",\"curvatures\":["<<physical.curvature1<<','<<physical.curvature2<<"],\"directions\":[";
      pointJSON(physical.direction1);frontLog<<',';pointJSON(physical.direction2);
      frontLog<<"],\"control_planning_directional_density\":["<<tangent1<<','<<tangent2<<"],\"direction_ok\":"<<direction.ok
              <<",\"carrier_rank\":"<<direction.rank<<",\"smooth_normal_alignment_abs\":"
              <<(direction.ok?std::abs(Dot(direction.d,Of(physical.normal))):0.)<<",\"core_control_only\":true}"<<std::endl;
    }
    dc9::Rows rows;std::set<Id> moving{edge[0]};dc9::Tx tx;bool low=false;
    for(unsigned grow=0;grow<20;++grow) {
      budget.CheckTime();tx=dc9::Transaction(*state,rows,moving,low);
      frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"row_trial\",\"trial\":"<<grow
              <<",\"low_band\":"<<low<<",\"closure_vertices\":"<<moving.size()<<",\"constructed\":"<<tx.ok
              <<",\"old_cells\":"<<tx.old_cells<<",\"fresh_cells\":"<<tx.fresh_cells
              <<",\"class\":"<<std::quoted(tx.cls)<<",\"reason\":"<<std::quoted(tx.witness)<<"}"<<std::endl;
      if(tx.ok)break;
      if(!low&&(tx.cls=="partial-state"||tx.cls=="first-child"||tx.cls=="clearance"||tx.cls=="height")){low=true;continue;}
      bool added=false;
      if(tx.cls=="clearance"){for(Id v:tx.blockers)if(!dc9::Rim(*state,v))added=moving.insert(v).second||added;}
      else if((tx.cls=="partial-state"||tx.cls=="first-child"||tx.cls=="height")&&!tx.failing.empty()) {
        std::sort(tx.failing.begin(),tx.failing.end());for(Id v:tx.failing.front())
          if(!dc9::Rim(*state,v))added=moving.insert(v).second||added;
      }
      if(!added)break;
    }
    bool external=false,rowsPreserved=false;
    std::string externalReason;
    if(tx.ok) {
      ++built;rowsPreserved=PreservePassingFirstHeights(tx.old,tx.fresh,heights,*state->reference,reason);
      external=true;REQUIRE(directory.Clearance(imported.cells,tx.old,tx.fresh,external,externalReason,ceiling,retained));
      std::set<Id> replaced(tx.ids.begin(),tx.ids.end()),unchanged;
      for(Id id:selected)if(!replaced.count(id))unchanged.insert(id);
      const auto surrounding=patchOf(unchanged);
      auto complete=[&](Patch patch) {
        patch.cells.insert(patch.cells.end(),surrounding.cells.begin(),surrounding.cells.end());
        patch.surface.insert(patch.surface.end(),surrounding.surface.begin(),surrounding.surface.end());
        patch.parameters.insert(surrounding.parameters.begin(),surrounding.parameters.end());
        patch.coverage.faces.insert(surrounding.coverage.faces.begin(),surrounding.coverage.faces.end());
        patch.coverage.traces.insert(surrounding.coverage.traces.begin(),surrounding.coverage.traces.end());return patch;
      };
      describe(label,"first_row_complete_old",complete(tx.old));
      describe(label,"first_row_complete_fresh",complete(tx.fresh));
      // Actual physical distance enclosures, not planar altitude or a uniqueness claim for the nearest foot.
      std::map<Id,Point> points;for(const auto& c:tx.fresh.cells)for(auto node:c.v)points.emplace(node.id,node.p);
      for(const auto& apex:tx.apices) {
        Id facet=UINT64_MAX;for(const auto& f:tx.fresh.surface)if(Sorted(f.v)==apex.first)facet=f.facet;
        REQUIRE(facet!=UINT64_MAX);size_t attempted=0;
        const auto& geometry=state->reference->Parameters()->Geometry();const auto h0=heights.at(state->reference->Get(facet).marker);
        SU2NativeGeometry::MappedFacetReference::CurvedClosest query;
        const auto work=std::min(SU2NativeGeometry::MappedFacetReference::ClearanceWork,budget.maximum_work-budget.work);
        try {query=geometry.ClosestCurved(facet,points.at(apex.second),h0*1e-4,work,ceiling-retained,{},&attempted,&budget);}
        catch(...){budget.ChargeWork(attempted);throw;}
        REQUIRE(budget.ChargeWork(attempted));
        frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"physical_first_height\",\"facet\":"<<facet
                <<",\"apex\":"<<apex.second<<",\"requested_h0\":"<<h0<<",\"distance_interval\":["<<query.lower<<','<<query.upper
                <<"],\"clearance_lower\":"<<query.clearance_lower<<",\"strict_height_interval_pass\":"
                <<(query.lower>=h0*(1-heights.tolerance)&&query.upper<=h0*(1+heights.tolerance)&&query.clearance_lower>0)<<"}"<<std::endl;
      }
      if(external&&rowsPreserved)++admitted; // Candidate only; strict whole-patch geometry remains in the companion log.
    }
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"first_row_result\",\"constructed\":"<<tx.ok
            <<",\"closure_vertices\":"<<moving.size()<<",\"old_cells\":"<<tx.old_cells<<",\"fresh_cells\":"<<tx.fresh_cells
            <<",\"external_clearance\":"<<external<<",\"passing_rows_preserved\":"<<rowsPreserved
            <<",\"external_clearance_attempted\":"<<tx.ok<<",\"external_reason\":"<<std::quoted(externalReason)
            <<",\"class\":"<<std::quoted(tx.cls)<<",\"reason\":"<<std::quoted(tx.witness)
            <<",\"geometry_work\":"<<budget.work-beforeWork<<",\"seconds\":"
            <<std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count()<<",\"mesh_published\":false}"<<std::endl;
  } catch(const std::exception& e) {
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"refusal\",\"reason\":"<<std::quoted(e.what())
            <<",\"geometry_work\":"<<budget.work-beforeWork<<",\"seconds\":"
            <<std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count()<<"}"<<std::endl;
  }
  frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"existing_counters\",\"field\":"
          <<dc9::JsonCounters(beforeField,dc9::CountersOf(*state))
          <<",\"edge_failures\":"<<state->field->Statistics().edge_failures-beforeRawField.edge_failures
          <<",\"edge_geometry_seconds\":"<<state->field->Statistics().edge_geometry_seconds-beforeRawField.edge_geometry_seconds
          <<",\"edge_composition_seconds\":"<<state->field->Statistics().edge_composition_seconds-beforeRawField.edge_composition_seconds
          <<",\"edge_seconds\":"<<state->field->Statistics().edge_seconds-beforeRawField.edge_seconds
          <<",\"cumulative_perf\":"<<dc9::JsonPerf()<<"}"<<std::endl;
  REQUIRE_FALSE(budget.time_exhausted);REQUIRE_FALSE(budget.work_exhausted);REQUIRE_FALSE(state->publication_failed);
}
frontLog<<"{\"complete\":true,\"contexts\":"<<count<<",\"constructed\":"<<built<<",\"clear_preserved_candidates\":"<<admitted
        <<",\"production_target_certified\":false,\"mesh_published\":false,\"geometry_work\":"<<budget.work<<"}"<<std::endl;
REQUIRE(bool(frontLog));input>>std::ws;REQUIRE(input.eof());
