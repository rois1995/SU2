// Included inside the existing actual-M6 loader; saved-target mode reads exact371 core values, no CFD/generation.
using namespace junction_wp0;
SU2NativeBLPipeline3D::detail::PipelineScope pipeline;
const bool savedTarget=std::string(std::getenv("SU2_NATIVE_BL_FRONT_PROBE"))=="saved_target_geometry";
double selectedCore=1e-4,tangentScale=1;
std::map<Id,std::pair<Point,Tensor>> savedCore;
if(savedTarget) {
  const auto* path=std::getenv("SU2_NATIVE_FROZEN_SENSOR3D");REQUIRE(path);
  const auto* coreText=std::getenv("SU2_NATIVE_FROZEN_CORE3D");REQUIRE(coreText);
  const auto* scaleText=std::getenv("SU2_NATIVE_FROZEN_TANGENT_SCALE3D");REQUIRE(scaleText);
  std::istringstream coreStream(coreText),scaleStream(scaleText);coreStream>>selectedCore;scaleStream>>tangentScale;
  REQUIRE(bool(coreStream));REQUIRE(bool(scaleStream));coreStream>>std::ws;scaleStream>>std::ws;
  REQUIRE(coreStream.eof());REQUIRE(scaleStream.eof());REQUIRE(std::isfinite(selectedCore));REQUIRE(selectedCore>0);
  REQUIRE(std::isfinite(tangentScale));REQUIRE(tangentScale>0);REQUIRE(tangentScale<=1);
  std::ifstream source(path,std::ios::binary);uint64_t records=0;source.read(reinterpret_cast<char*>(&records),8);
  REQUIRE(bool(source));REQUIRE(records>0);REQUIRE(records<=10000000);
  const auto bytes=transfer_memory::Mul(records,sizeof(decltype(savedCore)::value_type)+4*sizeof(void*));
  REQUIRE(transfer_memory::Add(budget.caller_bytes,bytes)<ceiling);
  source.seekg(0,std::ios::end);REQUIRE(source.tellg()==std::streamoff(8+104*records));source.seekg(8,std::ios::beg);
  for(uint64_t k=0;k<records;++k) {
    Id id=0;Point p{};std::array<double,9> m;
    source.read(reinterpret_cast<char*>(&id),8);source.read(reinterpret_cast<char*>(&p.x),8);
    source.read(reinterpret_cast<char*>(&p.y),8);source.read(reinterpret_cast<char*>(&p.z),8);
    source.read(reinterpret_cast<char*>(m.data()),72);REQUIRE(bool(source));
    REQUIRE(std::isfinite(p.x));REQUIRE(std::isfinite(p.y));REQUIRE(std::isfinite(p.z));
    REQUIRE(m[1]==m[3]);REQUIRE(m[2]==m[6]);REQUIRE(m[5]==m[7]);
    const Tensor t{m[0],m[1],m[2],m[4],m[5],m[8]};ValidateTensor(t);
    REQUIRE(savedCore.emplace(id,std::make_pair(p,t)).second);
  }
  REQUIRE(source.peek()==std::char_traits<char>::eof());
}
auto policy=SU2NativeBLPipeline3D::MakeTargetPolicy(config,*reference,1e-4,-1);
if(savedTarget)policy->full->SelectTangentialScale(tangentScale);
policy->Update(selectedCore,SU2_TYPE::GetValue(config.GetAdap_Hgrad()));
auto geometryLease=policy->ReserveGeometry(budget);
dc10::optionA=policy->A;dc10::probeBase=policy->base;dc10::probeFull=policy->full;
dc9::validityOnlyConstruction=true;dc9::markerRoles=policy->roles;
const Tensor control{selectedCore,0,0,selectedCore,0,selectedCore};
auto coreAt=[&](Node node) {
  if(!savedTarget)return control;
  const auto& row=savedCore.at(node.id);REQUIRE(row.first.x==node.p.x);REQUIRE(row.first.y==node.p.y);REQUIRE(row.first.z==node.p.z);
  return row.second;
};
auto state=std::make_unique<State>();state->name="actual_curved_front_probe";
state->facets=reference->facets;state->markers=reference->marker_names;
state->heights=heights;for(const auto& height:heights)state->walls.insert(height.first);
state->h0=policy->h0;state->core=policy->core;state->layers=policy->full;state->mapped=true;
state->growth=SU2_TYPE::GetValue(config.GetAdap_BL(0).growth);
state->thickness=SU2_TYPE::GetValue(config.GetAdap_BL(0).thickness);state->memory_ceiling=ceiling;
const auto priorCaller=budget.caller_bytes;
const auto retained=transfer_memory::Add(priorCaller,policy->StorageBytes(),
    transfer_memory::Mul(imported.cells.size(),3*(sizeof(DonorCell)+640)),
    transfer_memory::Mul(savedCore.size(),sizeof(decltype(savedCore)::value_type)+4*sizeof(void*)),size_t(256)<<20);
REQUIRE(retained<ceiling);budget.caller_bytes=retained;
std::vector<DonorCell> donors;donors.reserve(imported.cells.size());
for(const auto& row:imported.cells){DonorCell d;d.cell=row.second.cell;for(unsigned k=0;k<4;++k)d.sensor[k]=coreAt(d.cell.v[k]);donors.push_back(d);}
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
  state->mesh=imported.cells;for(auto& row:state->mesh){row.second.sensor_present=true;for(unsigned k=0;k<4;++k)row.second.sensor[k]=coreAt(row.second.cell.v[k]);}
  state->live_parameters=std::make_shared<BoundaryParameterState>(*reference->original->Parameters());
  state->reference=scope->WithParameterState(state->live_parameters)->WithAtlasReservation(budget);
  state->overlay.clear();Prepare(*state);dc12::ApplyWallPolicies(*state);state->heights.tolerance=heights.tolerance;
  dc9::DetectCreases(*state,policy->roles);dc9::ensureDirectionField(*state);
  const auto old=PatchOf(*state,std::vector<Id>(selected.begin(),selected.end()));
  std::vector<DonorCell> localDonors;for(const auto& c:old.cells){DonorCell d;d.cell=c;for(unsigned k=0;k<4;++k)d.sensor[k]=coreAt(d.cell.v[k]);localDonors.push_back(d);}
  auto identity=std::make_shared<const FrozenMetricIdentity>(FrozenMetricIdentity::Scope::SurfacePlanning);
  FrozenField core(std::move(localDonors),{},true,{}, {},identity);
  const auto beforeWork=budget.work;const auto t0=std::chrono::steady_clock::now();
  const auto beforeField=dc9::CountersOf(*state);
  const auto beforeRawField=state->field->Statistics();
  try {
    state->admit_scratch(SurfacePlanningScratchBytes(old.cells.size()));
    const auto plan=MeasureSurfacePlanning(old,core,*state->reference,heights,
        [&](Node node,const PlanarReference& context,Tensor sensor){return policy->BoundaryGuidance(node,context,sensor,reference->marker_names);});
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"surface_planning\",\"maximum_requested_length\":"<<plan.maximum_length
            <<",\"core_control_only\":"<<!savedTarget<<",\"full_target_certified\":false,\"geometry_work\":"<<budget.work-beforeWork<<"}"<<std::endl;
    for(const auto& face:old.surface)if(heights.count(state->reference->Get(face.facet).marker)) {
      const auto& geometry=state->reference->Parameters()->Geometry();const auto vertex=face.v[0];
      const auto p=NodePoint(*state,vertex);const auto* association=state->reference->Parameter(p);REQUIRE(association);
      if(!budget.ChargeWork(1))throw std::length_error("Actual curvature probe exhausted geometry work.");
      const auto physical=geometry.EvaluateSurface(face.facet,association->binding);
      const auto guidance=policy->BoundaryGuidance({vertex,p},*state->reference,coreAt({vertex,p}),reference->marker_names);
      const auto direction=dc9::DirectionAt(*state,vertex);
      if(savedTarget) {
        const auto flow=coreAt({vertex,p});const auto combined=policy->Composition()(p,flow);
        const auto repeated=policy->Composition()(p,flow);
        CHECK(combined.xx==repeated.xx);CHECK(combined.xy==repeated.xy);CHECK(combined.xz==repeated.xz);
        CHECK(combined.yy==repeated.yy);CHECK(combined.yz==repeated.yz);CHECK(combined.zz==repeated.zz);
        const auto wall=policy->BoundaryGuidance({vertex,p},*state->reference,control,reference->marker_names);
        auto density=[&](Tensor m,Point d){return MetricNorm({0,0,0},d,m);};
        frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"saved_target_contributions\",\"vertex\":"<<vertex
                <<",\"flow_principal_density\":["<<density(flow,physical.direction1)<<','<<density(flow,physical.direction2)
                <<"],\"combined_principal_density\":["<<density(combined,physical.direction1)<<','<<density(combined,physical.direction2)
                <<"],\"combined_normal_density\":"<<density(combined,physical.normal)<<",\"wall_profile_with_core_floor_principal_density\":[";
        long double d0=0,d1=0;for(const auto& side:wall){d0=std::max(d0,density(side.tensor,physical.direction1));d1=std::max(d1,density(side.tensor,physical.direction2));}
        frontLog<<d0<<','<<d1<<"],\"components_additive\":false,\"physical_height_placement_verified\":false}"<<std::endl;
      }
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
              <<(direction.ok?std::abs(Dot(direction.d,Of(physical.normal))):0.)<<",\"core_control_only\":"<<!savedTarget<<"}"<<std::endl;
    }
    if(savedTarget) {
      size_t qb=0,jb=0;double minq=1,minj=1;
      for(const auto& cell:old.cells) {
        Tetrahedron tet;Point center{};for(unsigned k=0;k<4;++k){tet[k]=cell.v[k].p;center.x+=tet[k].x/4;center.y+=tet[k].y/4;center.z+=tet[k].z/4;}
        const auto m=Measure(tet,state->field->evaluate(center));qb+=m.mean_ratio<MinimumMeanRatio;jb+=m.minimum_scaled_jacobian<MinimumScaledJacobian;
        minq=std::min(minq,m.mean_ratio);minj=std::min(minj,m.minimum_scaled_jacobian);
      }
      frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"saved_target_centroids_complete\",\"q_bad_centroid\":"<<qb
              <<",\"J_bad_centroid\":"<<jb<<",\"q_centroid_min\":"<<minq<<",\"J_centroid_min\":"<<minj<<"}"<<std::endl;
      const auto a=NodePoint(*state,edge[0]),b=NodePoint(*state,edge[1]);
      const auto sensorLength=state->field->SensorEdgeLength(a,b);
      frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"saved_target_chord_begin\",\"a\":";pointJSON(a);
      frontLog<<",\"b\":";pointJSON(b);frontLog<<",\"sensor_edge_length\":"<<sensorLength<<"}"<<std::endl;
      const auto fullLength=state->field->TargetEdgeLength(a,b);
      REQUIRE(sensorLength>0);REQUIRE(fullLength>0);REQUIRE(std::isfinite(fullLength));
      frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"saved_target_patch\",\"core\":"<<selectedCore
              <<",\"tangent_scale\":"<<tangentScale<<",\"q_bad_centroid\":"<<qb<<",\"J_bad_centroid\":"<<jb
              <<",\"q_centroid_min\":"<<minq<<",\"J_centroid_min\":"<<minj
              <<",\"sensor_edge_length\":"<<sensorLength<<",\"composed_edge_length\":"<<fullLength
              <<",\"construction_attempted\":false,\"complete_checkpoint\":false}"<<std::endl;
    }
    if(savedTarget||std::string(std::getenv("SU2_NATIVE_BL_FRONT_PROBE"))=="target_geometry_only") {
      budget.CheckTime();REQUIRE_FALSE(budget.work_exhausted);
      frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"target_geometry_only_complete\",\"geometry_work\":"<<budget.work-beforeWork
              <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count()
              <<",\"construction_attempted\":false,\"production_target_certified\":false}"<<std::endl;
      continue;
    }
    dc9::Rows rows;std::set<Id> moving{edge[0]};dc9::Tx tx;bool low=false;
    Face firstFail{};bool haveFail=false;
    for(unsigned grow=0;grow<20;++grow) {
      budget.CheckTime();tx=dc9::Transaction(*state,rows,moving,low);
      if(!haveFail&&!tx.failing.empty()){firstFail=dc9::Sorted(tx.failing.front());haveFail=true;}
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
      external=true;REQUIRE(directory->Clearance(imported.cells,tx.old,tx.fresh,external,externalReason,ceiling,retained));
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
    if(!tx.ok) {
      // Test existing completion on the original failed face before attributing refusal to surface resolution.
      const auto face=std::find_if(old.surface.begin(),old.surface.end(),[&](const SurfaceFace& f){
        return heights.count(state->reference->Get(f.facet).marker)&&(haveFail?dc9::Sorted(f.v)==firstFail:
            std::find(f.v.begin(),f.v.end(),edge[0])!=f.v.end());});
      REQUIRE(face!=old.surface.end());const auto key=dc9::Sorted(face->v);
      REQUIRE(state->wall_index.count(key));const Id owner=state->wall_index.at(key);
      dc10::Rows10 completionRows;dc12::FirstRowStats completion;std::set<Face> certified;
      const auto completionWork=budget.work;const auto completionStart=std::chrono::steady_clock::now();
      bool inserted=dc12::FirstRowInsert(*state,completionRows,owner,key,completion,certified);
      std::string cavityReason;bool cavity=false;
      if(!inserted)cavity=dc15::CavityFirstChild(*state,completionRows,owner,key,certified,cavityReason);
      bool strict=false,external=false;std::string heightReason,externalReason;FirstHeightReport height;
      Patch removed,fresh;
      if(inserted||cavity) {
        std::set<Id> retired;
        for(const auto& row:imported.cells)if(!state->mesh.count(row.first))retired.insert(row.first);
        removed=patchOf(retired);std::vector<Id> created;
        for(const auto& row:state->mesh)if(!imported.cells.count(row.first))created.push_back(row.first);
        fresh=PatchOf(*state,created);REQUIRE_FALSE(removed.cells.empty());REQUIRE_FALSE(fresh.cells.empty());
        strict=CheckFirstHeight(fresh,heights,*state->reference,height,heightReason);
        external=true;REQUIRE(directory->Clearance(imported.cells,removed,fresh,external,externalReason,ceiling,retained));
        describe(label,"existing_completion_old",removed);describe(label,"existing_completion_fresh",fresh);
      }
      frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"existing_completion\",\"face\":["<<key[0]<<','<<key[1]<<','<<key[2]
              <<"],\"first_row_route_succeeded\":"<<inserted<<",\"cavity\":"<<cavity<<",\"strict_height\":"<<strict
              <<",\"external_clearance\":"<<external<<",\"external_clearance_attempted\":"<<(inserted||cavity)
              <<",\"old_cells\":"<<removed.cells.size()<<",\"fresh_cells\":"<<fresh.cells.size()
              <<",\"height_reason\":"<<std::quoted(heightReason)<<",\"external_reason\":"<<std::quoted(externalReason)
              <<",\"cavity_reason\":"<<std::quoted(cavityReason)<<",\"rejections\":{";
      bool comma=false;for(const auto& rejection:completion.why){if(comma)frontLog<<',';comma=true;
        frontLog<<std::quoted(rejection.first)<<':'<<rejection.second;}
      frontLog<<"},\"geometry_work\":"<<budget.work-completionWork<<",\"seconds\":"
              <<std::chrono::duration<double>(std::chrono::steady_clock::now()-completionStart).count()
              <<",\"production_target_certified\":false,\"mesh_published\":false}"<<std::endl;
      if(!inserted&&!cavity) {
        auto physicalProposals=[&](const Face& target,unsigned generation) {
        const auto at=state->wall_index.find(target);REQUIRE(at!=state->wall_index.end());
        const Patch originalChild=PatchOf(*state,{at->second});
        const auto targetFace=std::find_if(originalChild.surface.begin(),originalChild.surface.end(),[&](const SurfaceFace& f){return dc9::Sorted(f.v)==target;});
        REQUIRE(targetFace!=originalChild.surface.end());const auto* face=&*targetFace;
        bool feasible=false;double best=std::numeric_limits<double>::infinity();
        auto privateWall=state->reference->PrivateParameters(originalChild,&budget);
        const auto& wallContext=privateWall?*privateWall:*state->reference;
        const auto& geometry=wallContext.Parameters()->Geometry();
        std::array<Point,3> corners;for(unsigned k=0;k<3;++k)corners[k]=NodePoint(*state,face->v[k]);
        const auto outward=dc9::Outward(Of(corners[0]),Of(corners[1]),Of(corners[2]));
        const auto h0=heights.at(wallContext.Get(face->facet).marker);
        const std::array<std::array<long double,3>,7> feet{{{1.L/3,1.L/3,1.L/3},{.6L,.2L,.2L},{.2L,.6L,.2L},{.2L,.2L,.6L},
            {.45L,.45L,.1L},{.45L,.1L,.45L},{.1L,.45L,.45L}}};
        for(size_t foot=0;foot<feet.size();++foot) {
          budget.CheckTime();std::string why;bool positive=false,heightPass=false,contact=false;FirstHeightReport measured;
          double planeRatio=0;const auto workAt=budget.work;
          try {
            const auto w=feet[foot];const auto flat=w[0]*Of(corners[0])+w[1]*Of(corners[1])+w[2]*Of(corners[2]);
            const auto projected=wallContext.Project(face->facet,Pt(flat));const auto* binding=wallContext.Parameter(projected);REQUIRE(binding);
            REQUIRE(budget.ChargeWork(1));const auto surface=geometry.EvaluateSurface(face->facet,binding->binding);
            auto normal=Unit(Of(surface.normal));if(Dot(normal,outward)>0)normal=-1.L*normal;
            const Node apex{state->next_node,Pt(Of(projected)+h0*normal)};
            planeRatio=double(-Dot(Of(apex.p)-Of(corners[0]),outward)/h0);
            Patch candidate;candidate.surface=originalChild.surface;candidate.parameters=originalChild.parameters;
            candidate.coverage=originalChild.coverage;
            positive=Cone(originalChild.cells,apex,state->next_cell,candidate.cells,why);
            if(positive) {
              heightPass=CheckFirstHeight(candidate,heights,wallContext,measured,why);
              if(heightPass) {
                contact=true;REQUIRE(directory->Clearance(state->mesh,originalChild,candidate,contact,why,ceiling,retained));
                describe(label,"physical_foot_old",originalChild);describe(label,"physical_foot_candidate",candidate);
              }
            }
          }catch(const std::exception& e){why=e.what();}
          frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"physical_foot_proposal\",\"face\":["<<target[0]<<','<<target[1]<<','<<target[2]
                  <<"],\"generation\":"<<generation<<",\"foot\":"<<foot<<",\"positive_cone\":"<<positive<<",\"strict_height\":"<<heightPass
                  <<",\"external_clearance\":"<<contact<<",\"plane_height_ratio\":"<<planeRatio
                  <<",\"height_relative_error\":"<<measured.maximum_relative_error<<",\"height_measured\":"<<measured.measured
                  <<",\"reason\":"<<std::quoted(why)<<",\"geometry_work\":"<<budget.work-workAt
                  <<",\"production_target_certified\":false,\"mesh_published\":false}"<<std::endl;
          if(positive&&measured.measured)best=std::min(best,std::abs(planeRatio-1));
          feasible=feasible||(positive&&heightPass&&contact);
        }
        return std::make_pair(feasible,best);
        };
#include "native_actual_bl_surface_height_probe.inc"
      }
    }
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
  if(savedTarget) {
    const auto stats=state->reference->Parameters()->Geometry().GeometryStatsSnapshot();
    frontLog<<"{\"context\":"<<std::quoted(label)<<",\"stage\":\"existing_geometry_counters\",\"closest_region_hits\":"<<stats.closest_region_hits
            <<",\"closest_region_misses\":"<<stats.closest_region_misses<<",\"closest_region_build_seconds\":"<<stats.closest_region_build_seconds
            <<",\"prepared_bytes\":"<<stats.prepared_bytes<<",\"memo_direct_hits\":"<<stats.memo_direct_hits
            <<",\"memo_direct_misses\":"<<stats.memo_direct_misses<<"}"<<std::endl;
  }
  REQUIRE_FALSE(budget.time_exhausted);REQUIRE_FALSE(budget.work_exhausted);REQUIRE_FALSE(state->publication_failed);
}
frontLog<<"{\"complete\":true,\"contexts\":"<<count<<",\"constructed\":"<<built<<",\"clear_preserved_candidates\":"<<admitted
        <<",\"production_target_certified\":false,\"mesh_published\":false,\"geometry_work\":"<<budget.work<<"}"<<std::endl;
REQUIRE(bool(frontLog));input>>std::ws;REQUIRE(input.eof());
