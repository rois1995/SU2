TEST_CASE("Saved M6 visibility correction publishes across actual owners", "[.][NativeActualCavityCorrectionMPI3D]") {
  World world;REQUIRE(world.size==6);
  struct ShapeRestore {ShapeAdmission prior=GetShapeAdmission();~ShapeRestore(){SetShapeAdmission(prior);}} restoreShape;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  const auto* configPath=std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  const auto* referencePath=std::getenv("SU2_NATIVE_BL_REFERENCE");
  const auto* targetPath=std::getenv("SU2_NATIVE_TARGET_CHECKPOINT3D");
  const auto* capture=std::getenv("SU2_NATIVE_BL_PLANNING_CAPTURE");
  REQUIRE(configPath);REQUIRE(referencePath);REQUIRE(targetPath);REQUIRE(capture);
  std::ifstream configFile(configPath);REQUIRE(bool(configFile));CConfig config(configFile,SU2_COMPONENT::SU2_CFD,false);
  const size_t ceiling=config.GetAdap_Native_MemoryLimit();REQUIRE(ceiling>0);
  auto reference=std::make_shared<ReferenceState>();bool loaded=false;std::string reason;size_t loadPeak=0;
  REQUIRE(LoadReference(referencePath,*reference,loaded,reason,ceiling,0,loadPeak));REQUIRE(loaded);REQUIRE(reference->original);
  struct GeometryOwner { CGeometry** levels=nullptr;unsigned count=0;
    ~GeometryOwner(){if(levels){for(unsigned i=0;i<count;++i)delete levels[i];delete[] levels;}} } geometryOwner;
  geometryOwner.count=config.GetnMGLevels()+1;
  CDriver::BuildGeometryFVM(&config,new CPhysicalGeometry(&config,world.rank,world.size),geometryOwner.levels,true);
  CNativeRemesher3D prepare(reference);prepare.PrepareReference(config,*geometryOwner.levels[MESH_0]);
  SU2NativeBLPipeline3D::detail::PipelineScope pipeline;
  const std::string targetRoot(targetPath);const auto slash=targetRoot.find_last_of('/');REQUIRE(slash!=std::string::npos);
  const auto restored=SU2NativeTargetCheckpoint3D::Read(targetRoot.substr(0,slash+1)+"target_rank"+std::to_string(world.rank)+".bin",
      *reference,config,ceiling,size_t(128)<<20);
  const auto& policy=*restored.family->first_row;
  auto faceKey=[](Face face){std::sort(face.begin(),face.end());return face;};

  Owned owned;{
    const auto bytes=TransactionRead(std::string(capture)+"/stage_surface_rank"+std::to_string(world.rank)+"_owned.bin");
    RecordStream input(bytes);auto rows=TransactionVector<OwnedCell>(input,1000000);REQUIRE(input.End());
    std::vector<std::vector<OwnedCell>> outgoing(world.size);
    for(auto& row:rows)outgoing.at(row.cell.id%5).push_back(std::move(row));
    bool admitted=false;const auto incoming=world.exchange(outgoing,&admitted,ceiling,
        transfer_memory::Add(ReferenceStorageBytes(*reference),restored.family->ResidentBytes(),transfer_memory::Bytes(rows)));
    REQUIRE(admitted);for(const auto& row:incoming)REQUIRE(owned.emplace(row.cell.id,row).second);
  }
  for(const auto& row:owned)REQUIRE(row.second.sensor_present);
  const auto payload=TransactionRead(std::string(capture)+"/stage_surface_rank0_parameters.bin");
  auto parameters=BoundaryParameterState::FromPayload(reference->original->Parameters()->Authority(),payload,ceiling,
      transfer_memory::Add(ReferenceStorageBytes(*reference),restored.family->ResidentBytes(),OwnedStorageBytes(owned)));
  SU2NativeGeometry::AtlasBudget budget;budget.maximum_bytes=ceiling;
  const auto globalCells=CPassiveComm::Allreduce(uint64_t(owned.size()),CPassiveComm::Op::SUM,world.comm);
  REQUIRE(globalCells==300958);REQUIRE_FALSE(world.maximum(world.rank==5&&!owned.empty()));
  budget.caller_bytes=transfer_memory::Add(ReferenceStorageBytes(*reference),restored.family->ResidentBytes(),
      OwnedStorageBytes(owned),size_t(256)<<20);
  budget.maximum_work=transfer_memory::Mul(globalCells,SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  constexpr size_t atlasScratch=size_t(16)<<20;size_t low=0,high=atlasScratch*8;
  while(low<high){const size_t bits=low+(high-low+1)/2;
    if(transfer_memory::Mul(2,SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits))<=atlasScratch/2)low=bits;else high=bits-1;}
  REQUIRE(low>0);budget.maximum_integer_bits=low;
  const auto started=std::chrono::steady_clock::now();budget.within_time=[&](){return std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<300.;};
  auto lease=policy.ReserveGeometry(budget);
  auto geometry=reference->original->WithParameterState(parameters)->WithAtlasReservation(budget);
  Directory directory(world,true,parameters.get());REQUIRE(directory.Build(owned,reason,ceiling,budget.caller_bytes));
  budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,directory.ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
  CavityMetricSource source(world,owned,true);
  auto identity=std::make_shared<const FrozenMetricIdentity>(FrozenMetricIdentity::Scope::SurfacePlanning);
  WallHeights heights;heights.tolerance=SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol());
  for(unsigned short w=0;w<config.GetnAdap_BL();++w){const auto& spec=config.GetAdap_BL(w);
    const auto marker=std::find(reference->marker_names.begin(),reference->marker_names.end(),spec.marker);
    REQUIRE(marker!=reference->marker_names.end());heights[int(marker-reference->marker_names.begin())]=SU2_TYPE::GetValue(spec.firstHeight);}

  constexpr Id point=11990,neighbor=11886;const bool active=world.rank==0;
  auto seed=directory.Lookup(active?std::vector<Feature>{{{point,0,0},1}}:std::vector<Feature>{},ceiling,budget.caller_bytes);
  REQUIRE(seed.valid);auto seedRows=directory.Fetch(seed,owned,ceiling,budget.caller_bytes);REQUIRE(seedRows.valid);
  const auto initialOwned=([&](){std::vector<char> bytes;RecordStream out(bytes);uint64_t n=owned.size();out(n);for(const auto& row:owned)out(row.second);return bytes;})();
  const auto initialParameters=parameters->Payload();
  const auto refused=directory.CorrectCollapse(seed,seedRows,owned,point,neighbor,active,*geometry,MaximumCavityCells,
      budget.within_time,1,budget.caller_bytes);
  CHECK_FALSE(refused.valid);
  const auto afterRefusal=([&](){std::vector<char> bytes;RecordStream out(bytes);uint64_t n=owned.size();out(n);for(const auto& row:owned)out(row.second);return bytes;})();
  CHECK(afterRefusal==initialOwned);CHECK(parameters->Payload()==initialParameters);
  {
    const bool simultaneous=active||world.rank==2;
    const Id removed=world.rank==2?13785:point,kept=world.rank==2?13788:neighbor;
    const auto requests=simultaneous?std::vector<Feature>{{{removed,0,0},1}}:std::vector<Feature>{};
    const auto dependencies=directory.Lookup(requests,ceiling,budget.caller_bytes);REQUIRE(dependencies.valid);
    const auto imported=directory.Fetch(dependencies,owned,ceiling,budget.caller_bytes);REQUIRE(imported.valid);
    const auto parallel=directory.CorrectCollapse(dependencies,imported,owned,removed,kept,simultaneous,*geometry,
        2*MaximumCavityCells,budget.within_time,ceiling,budget.caller_bytes);
    REQUIRE_FALSE(world.maximum(!parallel.valid||parallel.corrected!=simultaneous));
    REQUIRE_FALSE(world.maximum(world.rank==2&&parallel.imported.cells.size()!=8));
    CHECK(parameters->Payload()==initialParameters);
  }
  auto correction=directory.CorrectCollapse(seed,seedRows,owned,point,neighbor,active,*geometry,MaximumCavityCells,
      budget.within_time,ceiling,budget.caller_bytes);
  REQUIRE_FALSE(world.maximum(!correction.valid||correction.corrected!=active));
  source.SetCavity(&correction.imported.cells);
  std::string localReason;CLocalFailure localFailure;
  if(active)try {
    if(correction.apex!=27313||seedRows.cells.size()!=7||correction.imported.cells.size()!=16||
       correction.apex==point||correction.apex==neighbor)throw std::runtime_error("Saved 11990 correction changed its witnessed apex or complete support.");
    std::set<int> owners;for(const auto& d:correction.dependencies.cells)owners.insert(d.owner);
    if(owners.size()<2)throw std::runtime_error("Actual correction did not cross migrated owners.");
    Patch seedPatch,direct;for(const auto& row:seedRows.cells){seedPatch.cells.push_back(row.cell);const auto& v=row.cell.v;
      const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
      for(unsigned k=0;k<4;++k)if(row.physical[k].present)seedPatch.surface.push_back({faces[k],row.physical[k].facet});}
    geometry->ExportParameters(seedPatch);
    if(CollapseVertex(seedPatch,point,neighbor,0,*geometry,direct,localReason))
      throw std::runtime_error("The saved 11990 seed unexpectedly admits direct collapse.");
  }catch(const std::exception& e){localFailure.Set(2,point,e.what());}
  const auto election=world.elect(localFailure);REQUIRE_FALSE(election.any);

  std::set<Id> seedVertices;for(const auto& row:seedRows.cells)for(const auto& n:row.cell.v)seedVertices.insert(n.id);
  Id peerCell=UINT64_MAX;if(active)for(const auto& row:correction.imported.cells) {
    if(std::any_of(seedRows.cells.begin(),seedRows.cells.end(),[&](const OwnedCell& seedRow){return seedRow.cell.id==row.cell.id;}))continue;
    const bool disjoint=std::none_of(row.cell.v.begin(),row.cell.v.end(),[&](const Node& n){return seedVertices.count(n.id);});
    if(disjoint){peerCell=row.cell.id;break;}}
  peerCell=CPassiveComm::Allreduce(peerCell,CPassiveComm::Op::MIN,world.comm);REQUIRE(peerCell!=UINT64_MAX);
  std::vector<std::vector<OwnedCell>> peerOutgoing(world.size);
  if(active){const auto row=std::find_if(correction.imported.cells.begin(),correction.imported.cells.end(),[&](const OwnedCell& c){return c.cell.id==peerCell;});
    if(row==correction.imported.cells.end())localFailure.Set(2,point,"Growth-only peer row is missing.");else peerOutgoing[1].push_back(*row);}
  const auto peerElection=world.elect(localFailure);REQUIRE_FALSE(peerElection.any);
  bool admitted=false;const auto peerRows=world.exchange(peerOutgoing,&admitted,ceiling,budget.caller_bytes);REQUIRE(admitted);
  Imported peer;peer.valid=world.rank==1;for(const auto& row:peerRows)peer.cells.push_back(row);
  std::string reservationReason;
  Imported competingSeed=active?seedRows:peer;const bool competing=active||world.rank==1;
  const bool seedClaim=directory.Reserve(competingSeed,competing,reservationReason,ceiling,budget.caller_bytes,active?1:0);
  CHECK(world.maximum(competing&&!seedClaim)==false);
  Imported competingFull=active?correction.imported:peer;
  const bool fullConflict=directory.Reserve(competingFull,competing,reservationReason,ceiling,budget.caller_bytes,active?1:0);
  CHECK(world.maximum(active&&!fullConflict));CHECK(world.maximum(world.rank==1&&fullConflict));
  const bool reserved=directory.Reserve(correction.imported,active,reservationReason,ceiling,budget.caller_bytes,0);
  CHECK(world.maximum(active&&!reserved)==false);

  Patch old,fresh;std::vector<BoundaryParameterEdit> edits;PatchMetric before,after,transported;MetricDiagnosis diagnosis;
  std::vector<OwnedCell> metricRows;
  bool metricOK=false,heightOK=false,geometryOK=false,parametersOK=false,heightStrict=false;size_t qBad=0,jBad=0;
  std::string metricReason,heightReason,geometryReason,parameterReason;
  Id nextCell=CPassiveComm::Allreduce(owned.empty()?Id(0):owned.rbegin()->first,CPassiveComm::Op::MAX,world.comm)+1;
  if(active)try {
    for(const auto& row:correction.imported.cells){old.cells.push_back(row.cell);const auto& v=row.cell.v;
      const std::array<Face,4> faces{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
      for(unsigned k=0;k<4;++k)if(row.physical[k].present)old.surface.push_back({faces[k],row.physical[k].facet});}
    geometry->ExportParameters(old);if(!ReconnectCollapse(old,point,neighbor,correction.apex,nextCell,*geometry,fresh,localReason))
      throw std::runtime_error(localReason);
    metricRows.resize(fresh.cells.size());for(size_t i=0;i<fresh.cells.size();++i)metricRows[i].cell=fresh.cells[i];
    PublishMetrics(correction.imported.cells,metricRows);
    auto metric=[&](const Patch& patch,const std::vector<OwnedCell>& rows) {auto donors=source.ImportMetricSamplesLocal(patch.cells,rows,{},ceiling,budget.caller_bytes);
      if(!donors.valid)throw std::runtime_error(donors.reason);const auto extensions=AssociatedExtensions(donors.cells,*geometry);
      FrozenField field(std::move(donors.cells),{},true,extensions,{},identity,true,MaximumDonors,true);
      const BoundaryMetric guidance=[&](Node node,const PlanarReference& g,Tensor tensor){return policy.BoundaryGuidance(node,g,tensor,reference->marker_names);};
      return MeasureSurfacePlanning(patch,field,*geometry,heights,guidance);};
    before=metric(old,correction.imported.cells);after=metric(fresh,correction.imported.cells);
    metricOK=ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),after,MetricChange::Coarsen,metricReason,&diagnosis);
    transported=metric(fresh,metricRows);
    auto shapeDonors=source.ImportMetricSamplesLocal(fresh.cells,metricRows,{},ceiling,budget.caller_bytes);
    if(!shapeDonors.valid)throw std::runtime_error(shapeDonors.reason);
    const auto extensions=AssociatedExtensions(shapeDonors.cells,*geometry);
    FrozenField shapeField(std::move(shapeDonors.cells),{},true,extensions,{},identity,true,MaximumDonors,true);
    const auto compose=policy.Composition();
    for(const auto& cell:fresh.cells){const auto center=Centroid(cell);const auto tensor=compose(center,shapeField.Sensor(center));
      const auto quality=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},tensor);
      if(!std::isfinite(quality.mean_ratio)||!std::isfinite(quality.minimum_scaled_jacobian))
        throw std::runtime_error("Post-transport shape observation is unresolved.");
      qBad+=quality.mean_ratio<MinimumMeanRatio;jBad+=quality.minimum_scaled_jacobian<MinimumScaledJacobian;}
    heightOK=PreservePassingFirstHeights(old,fresh,heights,*geometry,heightReason);
    FirstHeightReport report;heightStrict=CheckFirstHeight(fresh,heights,*geometry,report,heightReason);
    heightOK=heightOK&&(heightStrict||(GetShapeAdmission()==ShapeAdmission::ValidityOnly&&report.measured&&
        !report.reference_clearance_failures&&!report.missing_cells));
    const auto oldGeometry=GeometryGuidanceExcess(old,*geometry,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
    const auto newGeometry=GeometryGuidanceExcess(fresh,*geometry,SU2_TYPE::GetValue(config.GetAdap_Hausd()));
    geometryOK=oldGeometry.complete&&newGeometry.complete&&newGeometry.excess==0;
    if(!geometryOK)geometryReason="Corrected patch fails strict SurfacePlanning geometry guidance.";
    parametersOK=parameters->CheckReplacement(old,fresh,parameterReason,ceiling,budget.caller_bytes,&budget);
    if(parametersOK)edits=BoundaryParameterState::Changes(old,fresh,parameters->Revision());
    const auto recomputed=metric(fresh,metricRows);bool same=transported.edges.size()==recomputed.edges.size();
    for(const auto& edge:transported.edges){const auto row=recomputed.edges.find(edge.first);
      same=same&&row!=recomputed.edges.end()&&row->second.length==edge.second.length;}
    if(!same)throw std::runtime_error("Cached/recomputed post-transport SurfacePlanning edges disagree.");
    std::map<Id,Tensor> nodal;for(const auto& c:correction.imported.cells)for(unsigned k=0;k<4;++k)nodal.emplace(c.cell.v[k].id,c.sensor[k]);
    for(const auto& c:metricRows)for(unsigned k=0;k<4;++k)if(!SameMetric(nodal.at(c.cell.v[k].id),c.sensor[k]))
      throw std::runtime_error("Metric transport changed a retained nodal tensor.");
    std::set<Id> oldIds;for(const auto& c:old.cells)oldIds.insert(c.id);
    if(correction.dependencies.cells.size()!=old.cells.size()||oldIds.size()!=old.cells.size())
      throw std::runtime_error("Final correction dependencies do not exactly cover the old support.");
    for(const auto& dep:correction.dependencies.cells){const auto cell=std::find_if(correction.imported.cells.begin(),correction.imported.cells.end(),
        [&](const OwnedCell& row){return row.cell.id==dep.cell;});
      if(!oldIds.erase(dep.cell)||cell==correction.imported.cells.end()||cell->version!=dep.version)
        throw std::runtime_error("Final correction dependency version differs from its fetched support.");}
    if(std::any_of(fresh.cells.begin(),fresh.cells.end(),[](const Cell& cell){return !(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)>0);}))
      throw std::runtime_error("Corrected replacement contains a nonpositive tetrahedron.");
    if(!metricOK)localFailure.Set(1,point,metricReason);if(!heightOK)localFailure.Set(1,point,heightReason);
    if(!geometryOK)localFailure.Set(1,point,geometryReason);if(!parametersOK)localFailure.Set(1,point,parameterReason);
  }catch(const std::exception& e){localFailure.Set(2,point,e.what());}
  const auto metricVote=world.elect(localFailure);
  INFO(metricVote.message);REQUIRE_FALSE(metricVote.any);
  bool contact=active;std::string contactReason;
  REQUIRE(directory.Clearance(owned,old,fresh,contact,contactReason,ceiling,budget.caller_bytes));
  REQUIRE_FALSE(world.maximum(active&&!contact));
  const auto ownedBefore=([&](){std::vector<char> bytes;RecordStream out(bytes);uint64_t n=owned.size();out(n);for(const auto& row:owned)out(row.second);return bytes;})();
  const auto parametersBefore=parameters->Payload();
  // Stale versions must reject before any owner or boundary state changes.
  std::vector<std::vector<Dependency>> staleOutgoing(world.size);
  if(active)for(const auto& dep:correction.dependencies.cells)staleOutgoing.at(dep.owner).push_back(dep);
  bool staleRouted=false;auto staleRemoved=world.exchange(staleOutgoing,&staleRouted,ceiling,budget.caller_bytes);REQUIRE(staleRouted);
  auto stale=std::find_if(staleRemoved.begin(),staleRemoved.end(),[](const Dependency& dep){return dep.owner>=0;});
  if(stale!=staleRemoved.end())++stale->version;
  const bool stalePublished=directory.Publish(owned,staleRemoved,{},true,reservationReason,ceiling,budget.caller_bytes);
  CHECK_FALSE(stalePublished);
  const auto ownedAfterStale=([&](){std::vector<char> bytes;RecordStream out(bytes);uint64_t n=owned.size();out(n);for(const auto& row:owned)out(row.second);return bytes;})();
  CHECK(ownedAfterStale==ownedBefore);CHECK(parameters->Payload()==parametersBefore);
  const bool finalReserved=directory.Reserve(correction.imported,active,reservationReason,ceiling,budget.caller_bytes);
  CHECK(world.maximum(active&&!finalReserved)==false);

  std::vector<std::vector<Dependency>> removals(world.size);std::vector<std::vector<OwnedCell>> additions(world.size);
  if(active) {
    std::map<Face,Id> physical;for(const auto& f:fresh.surface)physical.emplace(faceKey(f.v),f.facet);
    for(size_t i=0;i<correction.dependencies.cells.size();++i) {
      const auto& dep=correction.dependencies.cells[i];removals.at(dep.owner).push_back(dep);
    }
    for(size_t i=0;i<metricRows.size();++i) {
      auto& row=metricRows[i];const auto& dep=correction.dependencies.cells[i%correction.dependencies.cells.size()];
      for(unsigned f=0;f<4;++f){Face face;const auto& v=row.cell.v;
        static const unsigned slots[4][3]={{1,2,3},{0,3,2},{0,1,3},{0,2,1}};
        for(unsigned k=0;k<3;++k)face[k]=v[slots[f][k]].id;auto found=physical.find(faceKey(face));
        if(found!=physical.end())row.physical[f]={true,found->second};}
      row.version=0;additions.at(dep.owner).push_back(row);
    }
  }
  bool routed=false;const auto removed=world.exchange(removals,&routed,ceiling,budget.caller_bytes);REQUIRE(routed);
  const auto added=world.exchange(additions,&routed,ceiling,budget.caller_bytes);REQUIRE(routed);
  REQUIRE(directory.Publish(owned,removed,added,true,reservationReason,ceiling,budget.caller_bytes,edits,
      active?fresh.coverage:SU2NativeGeometry::BoundaryCoverage{},&budget));
  CHECK(world.maximum(!directory.ChangedVertices().empty()));
  CHECK(CPassiveComm::Allreduce(uint64_t(owned.size()),CPassiveComm::Op::SUM,world.comm)==globalCells-2);
  bool invalid=false;for(const auto& row:owned) {
    const auto& v=row.second.cell.v;invalid=invalid||Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<=0;
    for(auto node:v)invalid=invalid||node.id==point;
  }
  CHECK_FALSE(world.maximum(invalid));CHECK_FALSE(world.maximum(world.rank==5&&!owned.empty()));
  for(const auto& row:owned)if(std::any_of(added.begin(),added.end(),[&](const OwnedCell& freshRow){return freshRow.cell.id==row.first;}))
    CHECK(row.second.sensor_present);
  std::ofstream log("native_actual_cavity_correction_mpi_rank"+std::to_string(world.rank)+".json");REQUIRE(bool(log));
  log<<std::setprecision(17)<<std::boolalpha<<"{\"rank\":"<<world.rank<<",\"owned_after\":"<<owned.size()
     <<",\"candidate_owner\":"<<active<<",\"support_cells\":"<<(active?correction.imported.cells.size():0)
     <<",\"replacement_cells\":"<<(active?fresh.cells.size():0)<<",\"apex\":"<<(active?correction.apex:0)
     <<",\"metric_accepted\":"<<(!active||metricOK)<<",\"height_accepted\":"<<(!active||heightOK)
     <<",\"height_strict\":"<<(!active||heightStrict)<<",\"geometry_accepted\":"<<(!active||geometryOK)
     <<",\"parameters_accepted\":"<<(!active||parametersOK)<<",\"q_bad_observed\":"<<qBad<<",\"j_bad_observed\":"<<jBad
     <<",\"concurrent_discovery_verified\":true,\"empty_rank_verified\":true,\"cached_recomputed_agree\":true"
     <<",\"stale_publish_rejected\":"<<(!stalePublished)<<",\"published\":true}\n";
}
