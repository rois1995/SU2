const auto* capture=std::getenv("SU2_NATIVE_BL_PLANNING_CAPTURE");REQUIRE(capture);
const std::string root=capture;constexpr size_t maxRows=1000000;
junction_wp0::State current;
{
 for(unsigned shard=0;shard<(cavityCorrection?6u:1u);++shard) {
  const auto bytes=TransactionRead(root+"/stage_surface_rank"+std::to_string(shard)+"_owned.bin");RecordStream input(bytes);
  auto rows=TransactionVector<OwnedCell>(input,maxRows);REQUIRE(input.End());
  REQUIRE(transfer_memory::Add(reference->original->ResidentBytes(),restored.family->ResidentBytes(),
      transfer_memory::Mul(current.mesh.size()+rows.size(),6*(sizeof(DonorCell)+640)),size_t(256)<<20)<ceiling);
  for(auto& row:rows)REQUIRE(current.mesh.emplace(row.cell.id,std::move(row)).second);
 }
}
const auto retained=transfer_memory::Add(ReferenceStorageBytes(*reference),restored.family->ResidentBytes(),
    transfer_memory::Mul(current.mesh.size(),6*(sizeof(DonorCell)+640)),size_t(256)<<20);
current.live_parameters=BoundaryParameterState::FromPayload(reference->original->Parameters()->Authority(),
    TransactionRead(root+"/stage_surface_rank0_parameters.bin"),ceiling,retained);
SU2NativeGeometry::AtlasBudget budget;budget.maximum_bytes=ceiling;budget.caller_bytes=retained;
budget.maximum_work=transfer_memory::Mul(current.mesh.size(),SU2NativeGeometry::MappedFacetReference::ClearanceWork);
constexpr size_t atlasScratch=size_t(16)<<20;size_t low=0,high=atlasScratch*8;
while(low<high){const size_t bits=low+(high-low+1)/2;
  if(transfer_memory::Mul(2,SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits))<=atlasScratch/2)low=bits;else high=bits-1;}
REQUIRE(low>0);budget.maximum_integer_bits=low;
const auto begun=std::chrono::steady_clock::now();
budget.within_time=[&](){return std::chrono::duration<double>(std::chrono::steady_clock::now()-begun).count()<300;};
auto lease=policy.ReserveGeometry(budget);
current.reference=reference->original->WithParameterState(current.live_parameters)->WithAtlasReservation(budget);
CavityMetricSource source(world,current.mesh,true); // Never bootstrap evolved cells from the original P1 donors.
for(const auto& row:current.mesh)REQUIRE(row.second.sensor_present);
auto identity=std::make_shared<const FrozenMetricIdentity>(FrozenMetricIdentity::Scope::SurfacePlanning);
WallHeights heights;heights.tolerance=SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol());
for(unsigned short w=0;w<config.GetnAdap_BL();++w) {
  const auto& spec=config.GetAdap_BL(w);const auto marker=std::find(reference->marker_names.begin(),reference->marker_names.end(),spec.marker);
  REQUIRE(marker!=reference->marker_names.end());heights[int(marker-reference->marker_names.begin())]=SU2_TYPE::GetValue(spec.firstHeight);
}
auto star=[&](Id vertex) {std::vector<Id> ids;
  INFO("Current complete star vertex "<<vertex);
  for(const auto& row:current.mesh)for(auto node:row.second.cell.v)if(node.id==vertex){ids.push_back(row.first);break;}
  REQUIRE_FALSE(ids.empty());REQUIRE(ids.size()<=MaximumCavityCells);return ids;
};
std::vector<std::pair<std::string,std::vector<Id>>> contexts;
contexts.push_back({"recorded_geometry_refusal",star(18775)});
if(cavityCorrection){contexts.push_back({"recorded_visibility_obstruction",star(11990)});contexts.push_back({"recorded_visibility_obstruction_b",star(13785)});}
std::set<Id> constrained=current.live_parameters->Mobility().corners;
for(const auto& edge:current.live_parameters->Mobility().features)constrained.insert(edge.first.begin(),edge.first.end());
Id smooth=0;long double nearest=INFINITY;
for(const auto& row:current.live_parameters->Records()) {
  const auto& p=row.second.point;if(constrained.count(row.first))continue;
  bool requested=false;current.live_parameters->Geometry().ForFacets(p.binding,[&](Id facet) {
    requested=requested||heights.count(reference->original->Get(facet).marker);
  });if(!requested)continue;
  const auto x=p.node.p.x-.7L,y=p.node.p.y-.5L,z=p.node.p.z-.04L;const auto distance=x*x+y*y+z*z;
  if(distance<nearest){nearest=distance;smooth=row.first;}
}
REQUIRE(smooth!=0);contexts.push_back({"unchanged_wall_control",star(smooth)});
for(const auto& row:current.mesh)if(std::none_of(row.second.physical.begin(),row.second.physical.end(),[](FacetBinding f){return f.present;})) {
  contexts.push_back({"unchanged_interior_control",{row.first}});break;
}
contexts.push_back({"leading_edge",star(4219)});
REQUIRE(contexts.size()==(cavityCorrection?6:4));
std::ofstream log("actual_bl_cavity_planning.jsonl");REQUIRE(bool(log));log<<std::setprecision(17)<<std::boolalpha;
auto samePoint=[](Point a,Point b){return a.x==b.x&&a.y==b.y&&a.z==b.z;};
auto sameTensor=[](Tensor a,Tensor b){return a.xx==b.xx&&a.xy==b.xy&&a.xz==b.xz&&a.yy==b.yy&&a.yz==b.yz&&a.zz==b.zz;};
if(!cavityCorrection)for(const auto& entry:contexts) {
  const auto patch=junction_wp0::PatchOf(current,entry.second);
  std::vector<OwnedCell> currentRows;for(Id id:entry.second)currentRows.push_back(current.mesh.at(id));
  for(const auto& c:patch.cells)REQUIRE(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0);
  std::map<Id,std::vector<BoundaryMetricSample>> witness;std::map<Edge,EdgeMeasure> baseline;
  // ABBA retains shared query-cache effects in both directions; no geometry result is a mesh certificate.
  for(unsigned repeat=0;repeat<4;++repeat) {
    const bool bulk=repeat==1||repeat==2;size_t calls=0;double guidanceSeconds=0;
    const BoundaryMetric target=[&](Node node,const PlanarReference& geometry,Tensor sensor) {
      const auto t=std::chrono::steady_clock::now();auto result=policy.BoundaryGuidance(node,geometry,sensor,reference->marker_names);
      guidanceSeconds+=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();++calls;
      const auto inserted=witness.emplace(node.id,result);
      if(!inserted.second) {
        REQUIRE(inserted.first->second.size()==result.size());
        for(size_t i=0;i<result.size();++i) {
          CHECK(sameTensor(inserted.first->second[i].tensor,result[i].tensor));
          CHECK(samePoint(inserted.first->second[i].normal,result[i].normal));
          CHECK(samePoint(inserted.first->second[i].strip_rule,result[i].strip_rule));
        }
      }
      return result;
    };
    const auto workBefore=budget.work;MetricStats counted;uint64_t requests=0;
    const auto t=std::chrono::steady_clock::now();std::map<Edge,EdgeMeasure> measured;
    const auto merge=[&](const PatchMetric& result) {
      for(const auto& edge:result.edges){const auto inserted=measured.emplace(edge);if(!inserted.second)CHECK(inserted.first->second.length==edge.second.length);}
    };
    // Match measure_private's admitted <=10-cell source batch in both alternatives.
    for(size_t first=0;first<patch.cells.size();first+=MaximumCavityCells/6) {
      Patch batch;batch.parameters=patch.parameters;batch.coverage=patch.coverage;
      std::set<Face> faces;
      for(size_t i=first;i<std::min(first+MaximumCavityCells/6,patch.cells.size());++i) {
        batch.cells.push_back(patch.cells[i]);for(auto f:junction_wp0::CellFaces(patch.cells[i])){std::sort(f.begin(),f.end());faces.insert(f);}
      }
      for(const auto& face:patch.surface){auto key=face.v;std::sort(key.begin(),key.end());if(faces.count(key))batch.surface.push_back(face);}
      auto imported=source.ImportCellsLocal(batch.cells,currentRows,ceiling,retained);REQUIRE(imported.valid);
      const auto components=AssociatedExtensions(imported.cells,*current.reference);
      FrozenField field(std::move(imported.cells),{},true,components,{},identity,true,MaximumDonors,true);
      if(bulk)merge(MeasureSurfacePlanning(batch,field,*current.reference,heights,target,&counted));
      else for(const auto& cell:batch.cells) {
        Patch part;part.cells={cell};part.parameters=patch.parameters;part.coverage=patch.coverage;
        const auto cellFaces=junction_wp0::CellFaces(cell);
        for(const auto& face:batch.surface)if(std::any_of(cellFaces.begin(),cellFaces.end(),[&](Face f){std::sort(f.begin(),f.end());auto other=face.v;std::sort(other.begin(),other.end());return f==other;}))part.surface.push_back(face);
        merge(MeasureSurfacePlanning(part,field,*current.reference,heights,target,&counted));
      }
      requests+=field.Statistics().requests;
    }
    const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count();
    if(!repeat)baseline=measured;
    REQUIRE(measured.size()==baseline.size());
    for(const auto& edge:measured) {
      REQUIRE(baseline.count(edge.first));const auto& old=baseline.at(edge.first);
      CHECK(edge.second.length==old.length);CHECK(samePoint(edge.second.a,old.a));CHECK(samePoint(edge.second.b,old.b));
    }
    log<<"{\"context\":"<<std::quoted(entry.first)<<",\"cells\":"<<patch.cells.size()<<",\"surface_faces\":"<<patch.surface.size()
       <<",\"mode\":"<<std::quoted(bulk?"complete_patch":"per_cell")<<",\"repeat\":"<<repeat<<",\"edges\":"<<measured.size()
       <<",\"guidance_calls\":"<<calls<<",\"unique_nodes\":"<<witness.size()<<",\"seconds\":"<<seconds
       <<",\"guidance_seconds_nested\":"<<guidanceSeconds<<",\"geometry_work\":"<<budget.work-workBefore
       <<",\"core_edge_evaluations\":"<<counted.edge_evaluations<<",\"field_requests\":"<<requests
       <<",\"current_nodal_field\":true,\"exact_edge_and_sample_agreement\":true,\"mesh_accepted\":false}"<<std::endl;REQUIRE(bool(log));
  }
}
// Existing private operators only: no publication, ordering change or q/J construction veto.
Directory directory(world,false,current.live_parameters.get());
std::string directoryReason;REQUIRE(directory.Build(current.mesh,directoryReason,ceiling,retained));
budget.caller_bytes=transfer_memory::Add(retained,directory.ResidentBytes());
REQUIRE(budget.caller_bytes<ceiling);
Id nextCell=0,nextNode=0;
for(const auto& row:current.mesh){nextCell=std::max(nextCell,row.first);for(auto n:row.second.cell.v)nextNode=std::max(nextNode,n.id);}
++nextCell;++nextNode;
const BoundaryMetric guidance=[&](Node n,const PlanarReference& g,Tensor m){return policy.BoundaryGuidance(n,g,m,reference->marker_names);};
struct ShapeModeRestore {ShapeAdmission old=GetShapeAdmission();~ShapeModeRestore(){SetShapeAdmission(old);}} shapeMode;
SetShapeAdmission(ShapeAdmission::ValidityOnly);
auto measurePatch=[&](const Patch& patch,const std::vector<OwnedCell>& rows) {
  PatchMetric measured;
  for(size_t first=0;first<patch.cells.size();first+=MaximumCavityCells/6) {
    Patch batch;batch.parameters=patch.parameters;batch.coverage=patch.coverage;std::set<Face> faces;
    for(size_t i=first;i<std::min(first+MaximumCavityCells/6,patch.cells.size());++i) {
      batch.cells.push_back(patch.cells[i]);for(auto f:junction_wp0::CellFaces(patch.cells[i])){std::sort(f.begin(),f.end());faces.insert(f);}
    }
    for(const auto& face:patch.surface){auto f=face.v;std::sort(f.begin(),f.end());if(faces.count(f))batch.surface.push_back(face);}
    auto imported=source.ImportCellsLocal(batch.cells,rows,ceiling,transfer_memory::Add(retained,directory.ResidentBytes()));
    if(!imported.valid)throw std::runtime_error(imported.reason);
    const auto components=AssociatedExtensions(imported.cells,*current.reference);
    FrozenField field(std::move(imported.cells),{},true,components,{},identity,true,MaximumDonors,true);
    const auto result=MeasureSurfacePlanning(batch,field,*current.reference,heights,guidance);
    for(const auto& edge:result.edges){auto inserted=measured.edges.emplace(edge);if(!inserted.second)REQUIRE(inserted.first->second.length==edge.second.length);}
  }
  std::set<Edge> requested;
  for(const auto& face:patch.surface)if(heights.count(current.reference->Get(face.facet).marker))for(unsigned k=0;k<3;++k){Edge e{face.v[k],face.v[(k+1)%3]};std::sort(e.begin(),e.end());requested.insert(e);}
  for(auto& edge:measured.edges) {
    if(!requested.count(edge.first))edge.second.length=1;
    if(edge.second.length>measured.maximum_length){measured.maximum_length=edge.second.length;measured.longest_edge=edge.first;}
  }
  return measured;
};
struct CorrectionAudit {std::string context,operation;Patch old,fresh;std::vector<OwnedCell> rows;};
std::vector<CorrectionAudit> correctionAudits,heightAudits;
for(const auto& entry:contexts) {
  if(entry.first=="unchanged_interior_control")continue;
  const Id vertex=entry.first=="recorded_geometry_refusal"?18775:entry.first=="recorded_visibility_obstruction"?11990:entry.first=="recorded_visibility_obstruction_b"?13785:entry.first=="leading_edge"?4219:smooth;
  const auto vertexPatch=junction_wp0::PatchOf(current,entry.second);
  std::map<Id,Node> nodes;for(const auto& cell:vertexPatch.cells)for(auto n:cell.v)nodes.emplace(n.id,n);
  std::vector<OwnedCell> vertexRows;for(Id id:entry.second)vertexRows.push_back(current.mesh.at(id));
  const auto planning=measurePatch(vertexPatch,vertexRows);
  Edge shortest{},longest{};long double shortLength=INFINITY,longLength=-1;
  for(const auto& edge:planning.edges)if(edge.first[0]==vertex||edge.first[1]==vertex) {
    bool wall=false;for(const auto& face:vertexPatch.surface)if(heights.count(current.reference->Get(face.facet).marker)&&
        std::find(face.v.begin(),face.v.end(),edge.first[0])!=face.v.end()&&std::find(face.v.begin(),face.v.end(),edge.first[1])!=face.v.end())wall=true;
    if(!wall)continue;
    if(edge.second.length<shortLength){shortLength=edge.second.length;shortest=edge.first;}
    if(edge.second.length>longLength){longLength=edge.second.length;longest=edge.first;}
  }
  REQUIRE(std::isfinite(shortLength));REQUIRE(longLength>0);
  auto test=[&](const char* name,const std::vector<Id>& ids,MetricChange kind,auto construct) {
    const auto old=junction_wp0::PatchOf(current,ids);std::vector<OwnedCell> rows;for(Id id:ids)rows.push_back(current.mesh.at(id));
    const auto acceptedParameters=current.live_parameters->Payload();
    const auto cpu=std::clock();const long rssBefore=dc16::RssKB();
    Patch fresh;std::string why;const auto workBefore=budget.work;const auto time=std::chrono::steady_clock::now();
    bool built=false,preserved=false,heightMeasured=false,heightStrict=false,heightProvisional=false,geometryOK=false,metricOK=false,parameterOK=false,contactProtocol=false,contactOK=false,completeStars=true,reserved=false;
    long double oldExcess=-1,newExcess=-1,oldMaximum=-1,newMaximum=-1;size_t oldQ=0,newQ=0,oldJ=0,newJ=0,oldGeometryBad=0,newGeometryBad=0;bool shapeMeasured=false,geometryCountsMeasured=false;
    try {
      built=construct(old,fresh,why);nextCell+=MaximumReplacementCells;++nextNode;
      if(built) {
        REQUIRE_FALSE(fresh.cells.empty());for(const auto& cell:fresh.cells)REQUIRE(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);
        if(cavityCorrection) {
          std::set<Id> before,after;for(const auto& c:old.cells)for(auto n:c.v)before.insert(n.id);
          for(const auto& c:fresh.cells)for(auto n:c.v)after.insert(n.id);
          std::vector<Feature> features;for(Id id:before)if(!after.count(id))features.push_back({{id,0,0},1});
          const auto required=directory.Lookup(features,ceiling,retained);
          completeStars=required.valid;std::set<Id> oldIds(ids.begin(),ids.end());
          for(const auto& dependency:required.cells)completeStars=completeStars&&oldIds.count(dependency.cell);
          if(!completeStars)why="Correction omits complete removed-vertex support: "+required.reason;
          Imported finalSupport;finalSupport.cells=rows;finalSupport.valid=true;
          reserved=directory.Reserve(finalSupport,completeStars,reason,ceiling,retained);
          if(why.empty()&&!reserved)why=reason;
        } else reserved=true;
        preserved=PreservePassingFirstHeights(old,fresh,heights,*current.reference,why);
        FirstHeightReport height;std::string heightReason;heightStrict=CheckFirstHeight(fresh,heights,*current.reference,height,heightReason);
        heightMeasured=height.measured;heightProvisional=heightStrict||(height.measured&&!height.reference_clearance_failures&&!height.missing_cells);
        if(why.empty()&&!heightProvisional)why=heightReason;
        const auto before=GeometryGuidanceExcess(old,*current.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
        const auto after=GeometryGuidanceExcess(fresh,*current.reference,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
        oldExcess=before.excess;newExcess=after.excess;
        if(cavityCorrection) {
          auto badCells=[&](const Patch& p) {
            auto privateGeometry=current.reference->PrivateParameters(p);const auto& g=privateGeometry?*privateGeometry:*current.reference;
            std::map<Id,Point> points;std::set<Id> bad;for(const auto& c:p.cells)for(auto n:c.v)points.emplace(n.id,n.p);
            for(const auto& f:p.surface)if(g.SurfaceDeviation(f.facet,{points.at(f.v[0]),points.at(f.v[1]),points.at(f.v[2])},true)>SU2_TYPE::GetValue(config.GetAdap_Hausd())) {
              auto key=f.v;std::sort(key.begin(),key.end());
              for(const auto& c:p.cells)for(auto face:junction_wp0::CellFaces(c)){std::sort(face.begin(),face.end());if(face==key)bad.insert(c.id);}
            }return bad.size();
          };
          oldGeometryBad=badCells(old);newGeometryBad=badCells(fresh);geometryCountsMeasured=true;
        }
        geometryOK=before.complete&&after.complete&&std::isfinite(newExcess)&&
            (kind==MetricChange::GeometryRefine ? oldExcess>0&&newExcess<oldExcess*(1-1e-12L) :
             kind==MetricChange::Coarsen ? newExcess==0 : newExcess<=oldExcess);
        if(why.empty()&&!geometryOK)why="Complete frozen geometry guidance fails this construction intent.";
        const auto oldMetric=measurePatch(old,rows);const auto newMetric=measurePatch(fresh,rows);
        oldMaximum=oldMetric.maximum_length;newMaximum=newMetric.maximum_length;std::string metricReason;
        MetricDiagnosis diagnosis;
        metricOK=ValidateMetricChange(old.cells.size(),oldMetric,fresh.cells.size(),newMetric,kind,metricReason,&diagnosis);
        if(metricOK&&kind==MetricChange::GeometryRepair&&
            !(oldExcess>0&&newExcess<oldExcess*(1-1e-12L))&&!diagnosis.size_progress) {
          metricOK=false;metricReason="Surface preparation repair removes neither tangential nor geometry demand.";
        }
        if(why.empty()&&!metricOK)why=metricReason;
        std::string parameterReason;parameterOK=current.live_parameters->CheckReplacement(old,fresh,parameterReason,ceiling,
            transfer_memory::Add(retained,directory.ResidentBytes()),&budget);
        if(why.empty()&&!parameterOK)why=parameterReason;
        std::string contactReason;contactOK=true;
        contactProtocol=directory.Clearance(current.mesh,old,fresh,contactOK,contactReason,ceiling,retained);
        if(why.empty()&&(!contactProtocol||!contactOK))why=contactReason;
        // Diagnostic production centroid q/J on the complete affected patch; never an admission veto.
        auto countShape=[&](const Patch& p,size_t& q,size_t& j,const char* side) {
          auto donors=source.ImportCellsLocal(p.cells,rows,ceiling,transfer_memory::Add(retained,directory.ResidentBytes()));
          if(!donors.valid)throw std::runtime_error(donors.reason);
          const auto components=AssociatedExtensions(donors.cells,*current.reference);
          FrozenField field(std::move(donors.cells),{},true,components,{},identity,true,MaximumDonors,true);
          const auto compose=policy.Composition();
          for(const auto& cell:p.cells){const auto point=Centroid(cell);const auto tensor=compose(point,field.Sensor(point));
            const auto m=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},tensor);
            q+=m.mean_ratio<MinimumMeanRatio;j+=m.minimum_scaled_jacobian<MinimumScaledJacobian;
            if(cavityCorrection){
              log<<"{\"stage\":\"centroid_directional_witness\",\"context\":"<<std::quoted(entry.first)<<",\"operator\":"<<std::quoted(name)
                 <<",\"side\":"<<std::quoted(side)<<",\"cell\":"<<cell.id<<",\"q\":"<<m.mean_ratio<<",\"J\":"<<m.minimum_scaled_jacobian
                 <<",\"tensor\":["<<tensor.xx<<','<<tensor.xy<<','<<tensor.xz<<','<<tensor.yy<<','<<tensor.yz<<','<<tensor.zz<<"],\"vertices\":[";
              for(unsigned k=0;k<4;++k){const auto n=cell.v[k];if(k)log<<',';log<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<']';}log<<"]}"<<std::endl;}
          }
        };
        countShape(old,oldQ,oldJ,"old");countShape(fresh,newQ,newJ,"fresh");shapeMeasured=true;
      }
    }catch(const std::exception& error){why=error.what();}
    REQUIRE(current.live_parameters->Payload()==acceptedParameters);
    const bool feasible=built&&completeStars&&reserved&&preserved&&heightProvisional&&geometryOK&&metricOK&&parameterOK&&contactProtocol&&contactOK;
    if(heightContinuation&&built&&std::none_of(heightAudits.begin(),heightAudits.end(),[&](const CorrectionAudit& a){return a.context==entry.first;}))
      heightAudits.push_back({entry.first,name,old,fresh,rows});
    if(cavityCorrection&&feasible) {
      const auto saved=std::find_if(correctionAudits.begin(),correctionAudits.end(),[&](const CorrectionAudit& a){return a.context==entry.first;});
      CorrectionAudit audit{entry.first,name,old,fresh,rows};
      if(saved==correctionAudits.end())correctionAudits.push_back(std::move(audit));
      else if(std::string(name).find("ReconnectCollapse")==0||std::string(name).find("PrescribedCollapseCone")==0)*saved=std::move(audit);
    }
    log<<"{\"context\":"<<std::quoted(entry.first)<<",\"stage\":\"operator_preflight\",\"operator\":"<<std::quoted(name)
       <<",\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<fresh.cells.size()<<",\"constructed\":"<<built
       <<",\"complete_removed_stars\":"<<completeStars<<",\"final_support_reserved\":"<<reserved
       <<",\"passing_heights_preserved\":"<<preserved<<",\"height_measured\":"<<heightMeasured<<",\"strict_height\":"<<heightStrict<<",\"provisional_height\":"<<heightProvisional
       <<",\"geometry_guidance_pass\":"<<geometryOK<<",\"old_geometry_excess\":"<<oldExcess<<",\"new_geometry_excess\":"<<newExcess
       <<",\"geometry_counts_measured\":"<<geometryCountsMeasured<<",\"old_geometry_bad_cells\":"<<oldGeometryBad<<",\"new_geometry_bad_cells\":"<<newGeometryBad
       <<",\"metric_admission\":"<<metricOK<<",\"old_planning_max\":"<<oldMaximum<<",\"new_planning_max\":"<<newMaximum
       <<",\"parameter_preflight\":"<<parameterOK<<",\"contact_protocol\":"<<contactProtocol<<",\"contact_pass\":"<<contactOK
       <<",\"centroid_shape_measured\":"<<shapeMeasured<<",\"old_q_bad\":"<<oldQ<<",\"new_q_bad\":"<<newQ<<",\"old_J_bad\":"<<oldJ<<",\"new_J_bad\":"<<newJ
       <<",\"private_feasible\":"<<feasible<<",\"reason\":"<<junction_wp0::Json(why)<<",\"geometry_work\":"<<budget.work-workBefore
       <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-time).count()
       <<",\"cpu_seconds\":"<<double(std::clock()-cpu)/CLOCKS_PER_SEC<<",\"rss_kB_before\":"<<rssBefore<<",\"rss_kB_after\":"<<dc16::RssKB()
       <<",\"admitted_peak_bytes\":"<<budget.peak_bytes
       <<",\"current_nodal_field\":true,\"published\":false,\"mesh_accepted\":false}"<<std::endl;REQUIRE(bool(log));
    REQUIRE(budget.geometry_failure.empty());
    return feasible;
  };
  const Id neighbor=entry.first=="recorded_geometry_refusal"?18783:entry.first=="recorded_visibility_obstruction"?11886:entry.first=="recorded_visibility_obstruction_b"?13788:shortest[0]==vertex?shortest[1]:shortest[0];
  if(cavityCorrection&&!heightContinuation) {
    const auto dependencies=directory.Lookup({Feature{{vertex,0,0},1}},ceiling,retained);REQUIRE(dependencies.valid);
    const auto rows=directory.Fetch(dependencies,current.mesh,ceiling,retained);REQUIRE(rows.valid);
    const auto plan=directory.CorrectCollapse(dependencies,rows,current.mesh,vertex,neighbor,true,*current.reference,
        2*MaximumCavityCells,budget.within_time,ceiling,retained);
    INFO(plan.reason);REQUIRE(plan.valid);
    log<<"{\"stage\":\"production_correction_support\",\"context\":"<<std::quoted(entry.first)
       <<",\"corrected\":"<<plan.corrected<<",\"apex\":"<<plan.apex<<",\"old_cells\":"<<plan.imported.cells.size()
       <<",\"work\":"<<plan.work<<"}"<<std::endl;
    if(entry.first=="recorded_visibility_obstruction")REQUIRE(plan.corrected);
    if(plan.corrected) {
      std::vector<Id> ids;for(const auto& c:plan.imported.cells)ids.push_back(c.cell.id);
      test("ProductionReconnectCollapse",ids,MetricChange::Coarsen,[&](const Patch& old,Patch& fresh,std::string& why){
        return ReconnectCollapse(old,vertex,neighbor,plan.apex,nextCell,*current.reference,fresh,why);
      });
    }
  }
  test("CollapseVertex",entry.second,MetricChange::Coarsen,[&](const Patch& old,Patch& fresh,std::string& why){
    return CollapseVertex(old,vertex,entry.first=="recorded_geometry_refusal"?18783:neighbor,nextCell,*current.reference,fresh,why);
  });
  if(cavityCorrection) {
#include "native_actual_cavity_correction.inc"
  }
  if(entry.first=="leading_edge") {
    std::vector<Id> edgeStar;for(Id id:entry.second){const auto& cell=current.mesh.at(id).cell;unsigned found=0;for(auto n:cell.v)found+=n.id==longest[0]||n.id==longest[1];if(found==2)edgeStar.push_back(id);}
    REQUIRE_FALSE(edgeStar.empty());REQUIRE(edgeStar.size()<=MaximumCavityCells);
    const auto a=nodes.at(longest[0]).p,b=nodes.at(longest[1]).p;
    auto midpoint=[&](){return Node{nextNode,{double((static_cast<long double>(a.x)+b.x)/2),double((static_cast<long double>(a.y)+b.y)/2),double((static_cast<long double>(a.z)+b.z)/2)}};};
    test("SplitEdge",edgeStar,MetricChange::GeometryRefine,[&](const Patch& old,Patch& fresh,std::string& why){return SplitEdge(old,longest,midpoint(),nextCell,*current.reference,fresh,why);});
    // Availability test, independent of whether production reaches this fallback after SplitEdge.
    test("InsertEdgeCavity_independent",edgeStar,MetricChange::GeometryRefine,[&](const Patch& old,Patch& fresh,std::string& why){return InsertEdgeCavity(old,longest,midpoint(),nextCell,*current.reference,fresh,why);});
  } else if(entry.first=="unchanged_wall_control") {
    Point average{};for(const auto& node:nodes)if(node.first!=vertex){average.x+=node.second.p.x;average.y+=node.second.p.y;average.z+=node.second.p.z;}
    const auto original=nodes.at(vertex).p;const double count=nodes.size()-1;
    Node proposal{nextNode,{.9*original.x+.1*average.x/count,.9*original.y+.1*average.y/count,.9*original.z+.1*average.z/count}};
    proposal=ConstrainMovePoint(vertexPatch,vertex,proposal,heights,*current.reference);
    test("MoveVertex",entry.second,MetricChange::GeometryRepair,[&](const Patch& old,Patch& fresh,std::string& why){proposal.id=nextNode;return MoveVertex(old,vertex,proposal,nextCell,*current.reference,fresh,why);});
    test("Reconnect",entry.second,MetricChange::GeometryRepair,[&](const Patch& old,Patch& fresh,std::string& why){return Reconnect(old,neighbor,nextCell,*current.reference,fresh,why);});
  }
}
if(heightContinuation) {
#include "native_actual_cavity_height_continuation.inc"
}
if(cavityCorrection&&!heightContinuation&&!productionCorrection) {
  for(const auto& audit:correctionAudits)for(unsigned side=0;side<2;++side) {
    const auto& patch=side?audit.fresh:audit.old;std::string why;PatchMetric metric;bool complete=false;
    const auto t=std::chrono::steady_clock::now();const auto cpu=std::clock();const auto workBefore=budget.work;
    try {
      auto donors=source.ImportCellsLocal(patch.cells,audit.rows,ceiling,retained);if(!donors.valid)throw std::runtime_error(donors.reason);
      const auto components=AssociatedExtensions(donors.cells,*current.reference);
      FrozenField field(std::move(donors.cells),policy.Composition(),true,components,policy.Edges(),std::make_shared<const FrozenMetricIdentity>(),true,MaximumDonors,true);
      std::map<Edge,std::array<Point,2>> edges;
      for(const auto& c:patch.cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){auto a=c.v[i],b=c.v[j];if(a.id>b.id)std::swap(a,b);edges.emplace(Edge{a.id,b.id},std::array<Point,2>{a.p,b.p});}
      complete=true;
      for(const auto& e:edges) {
        if(budget.time_exhausted){complete=false;why="Existing diagnostic deadline exhausted; remaining exact edges unknown.";break;}
        try {
          const auto length=field.TargetEdgeLength(e.second[0],e.second[1]);
          metric.edges.emplace(e.first,EdgeMeasure{e.second[0],e.second[1],length,{}});metric.maximum_length=std::max(metric.maximum_length,length);
          log<<"{\"stage\":\"completed_exact_edge\",\"context\":"<<std::quoted(audit.context)<<",\"operator\":"<<std::quoted(audit.operation)
             <<",\"side\":"<<std::quoted(side?"fresh":"old")<<",\"a\":"<<e.first[0]<<",\"b\":"<<e.first[1]<<",\"length\":"<<length<<"}"<<std::endl;
        }catch(const std::exception& e){complete=false;why=e.what();break;}
      }
    }catch(const std::exception& e){why=e.what();}
    log<<"{\"stage\":\"full_target_edge_audit\",\"context\":"<<std::quoted(audit.context)<<",\"operator\":"<<std::quoted(audit.operation)
       <<",\"side\":"<<std::quoted(side?"fresh":"old")<<",\"complete\":"<<complete<<",\"maximum\":"<<metric.maximum_length
       <<",\"reason\":"<<junction_wp0::Json(why)<<",\"geometry_work\":"<<budget.work-workBefore<<",\"cpu_seconds\":"<<double(std::clock()-cpu)/CLOCKS_PER_SEC
       <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-t).count()<<",\"edges\":[";
    bool first=true;for(const auto& e:metric.edges){if(!first)log<<',';first=false;log<<'['<<e.first[0]<<','<<e.first[1]<<','<<e.second.length<<']';}log<<"]}"<<std::endl;
  }
}

if(cavityCorrection)NativeStageCapture::WriteOwned("correction_unchanged_owned.bin",current.mesh);
