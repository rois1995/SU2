// Actual corrected-target residuals: existing operators only, never historical replay or acceptance.
static void CapturedCurrentResidual(bool compound) {
  World world;REQUIRE(world.size==1);
  const auto* cfg=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(cfg);
  const std::string config(cfg);const auto slash=config.find_last_of('/');REQUIRE(slash!=std::string::npos);
  const auto directory=config.substr(0,slash+1);
  std::ifstream current_cases(directory+"current_residual_cases.tsv");
  const bool current_capture=bool(current_cases);
  REQUIRE_FALSE(bool(std::ifstream("native_current_residual_result.jsonl")));
  std::ofstream log("native_current_residual_result.jsonl");REQUIRE(bool(log));log<<std::setprecision(17)<<std::boolalpha;
  auto bytes=Captured202Bytes(directory+"m6.su2.native_ref");
  auto original=DecodeReference(std::string(bytes.begin(),bytes.end()));
  REQUIRE(original.original);REQUIRE(original.original->Parameters());
  REQUIRE((original.marker_names==std::vector<std::string>{"FARFIELD","SYMMETRY","WING3D"}));
  auto point_rows=Captured202Rows<BoundaryPoint>(directory+"witness_boundary_points.bin",72);
  auto surface=Captured202Rows<SurfaceFace>(directory+"witness_boundary_surface.bin",32);
  if(current_capture){REQUIRE_FALSE(point_rows.empty());REQUIRE_FALSE(surface.empty());}
  else {REQUIRE(point_rows.size()==32784);REQUIRE(surface.size()==65564);}
  BoundaryPoints points;for(const auto& p:point_rows)REQUIRE(points.emplace(p.node.id,p).second);
  const size_t ceiling=size_t(12)<<30;
  SU2NativeGeometry::AtlasBudget budget;budget.maximum_bytes=ceiling;
  // Same bounded per-face reservation as production mobility setup, over this complete reconstruction scope.
  budget.maximum_work=transfer_memory::Mul(transfer_memory::Add(original.facets.size(),surface.size()),
      SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  budget.maximum_integer_bits=size_t(1)<<20;
  budget.caller_bytes=transfer_memory::Add(original.original->ResidentBytes(),
      transfer_memory::Mul(points.size(),sizeof(BoundaryPoint)+8*sizeof(void*)),
      transfer_memory::Bytes(point_rows),transfer_memory::Bytes(surface),size_t(64)<<20);
  const auto* initial=original.original->Parameters();std::map<int,int> roles{{0,2},{1,0},{2,2}};
  auto mobile=initial->Mobility().identity?std::shared_ptr<BoundaryParameterState>{}:
      initial->WithMobilityPolicy(60,roles,budget,ceiling,budget.caller_bytes);
  const auto* active=mobile?mobile.get():initial;
  REQUIRE(active->Mobility().angle==60);REQUIRE(active->Mobility().roles==roles);
  log<<"{\"stage\":\"mobility_ready\",\"work\":"<<budget.work<<",\"work_limit\":"<<budget.maximum_work<<"}\n";log.flush();
  auto entity=active->WithEntityGeometry(1e-4,budget,ceiling,budget.caller_bytes);
  log<<"{\"stage\":\"entity_ready\",\"work\":"<<budget.work<<"}\n";log.flush();
  auto model=original.original->WithParameterState(entity);
  Patch initial_surface;initial_surface.coverage=entity->Coverage();
  for(const auto& row:entity->Records())initial_surface.parameters.emplace(row.first,row.second.point);
  for(const auto& row:entity->Surface())initial_surface.surface.push_back(row.second);
  SU2NativeGeometry::BoundaryCoverage coverage;std::string reason;
  const bool rebuilt=SU2NativeGeometry::RebuildFeatureTraces(initial_surface,surface,points,*model,coverage.traces,budget,reason);
  INFO(reason);REQUIRE(rebuilt);
  log<<"{\"stage\":\"feature_traces_rebuilt\",\"traces\":"<<coverage.traces.size()<<",\"work\":"<<budget.work<<"}\n";log.flush();
  auto state=std::make_shared<BoundaryParameterState>(entity->Authority(),points,surface,1,ceiling,
      budget.caller_bytes,coverage,entity->Mobility(),&budget,1e-4);
  auto reference=original.original->WithParameterState(state);
  log<<"{\"stage\":\"current_boundary_validated\",\"traces\":"<<state->Coverage().traces.size()
     <<",\"work\":"<<budget.work<<",\"fresh_revision\":1,\"historical_trace_identity_proved\":false}\n";log.flush();


  struct RestoreShape {ShapeAdmission value=GetShapeAdmission();~RestoreShape(){SetShapeAdmission(value);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  std::vector<std::pair<std::string,Id>> cases{
    {"root_leading_surface",224633219},{"blocked_midspan_surface",218068745},
    {"feature_tip_surface",224695705},{"interior_control_candidate",223413526}};
  std::map<Id,size_t> expected_unions{{224633219,59},{218068745,67},{224695705,55},{223413526,91}};
  std::map<Id,size_t> expected_stars{{273716432,13},{272681898,30},{272822191,12},{268453858,20},
      {18577,19},{18578,23},{274021962,24},{273819624,17},{284057860,28},{272378148,18},
      {179548258,8},{281903910,19},{282435106,26},{278469346,42},{282108514,30},{188151106,18}};
  if(current_capture) {
    cases.clear();expected_unions.clear();expected_stars.clear();
    std::string label;Id core;size_t count;
    while(current_cases>>label>>core>>count) {
      REQUIRE(cases.size()<8);REQUIRE(count>0);REQUIRE(count<=MaximumReplacementCells);
      REQUIRE(expected_unions.emplace(core,count).second);cases.emplace_back(label,core);
      for(unsigned k=0;k<4;++k){Id vertex;size_t star;REQUIRE(bool(current_cases>>vertex>>star));
        const auto prior=expected_stars.emplace(vertex,star);REQUIRE(prior.first->second==star);}
    }
    REQUIRE(current_cases.eof());REQUIRE_FALSE(cases.empty());
  }
  const std::array<std::array<unsigned,3>,4> opposite{{{{1,2,3}},{{0,3,2}},{{0,1,3}},{{0,2,1}}}};
  auto patch_from=[&](const Owned& rows) {
    Patch p;for(const auto& entry:rows){const auto& row=entry.second;p.cells.push_back(row.cell);
      for(unsigned slot=0;slot<4;++slot)if(row.physical[slot].present){Face face;
        for(unsigned k=0;k<3;++k)face[k]=row.cell.v[opposite[slot][k]].id;
        p.surface.push_back({face,row.physical[slot].facet});}}
    return p;
  };
  auto retain_coverage=[](Patch& p) {
    std::set<Face> faces;std::set<Edge> edges;
    for(auto f:p.surface){std::rotate(f.v.begin(),std::min_element(f.v.begin(),f.v.end()),f.v.end());faces.insert(f.v);
      for(unsigned k=0;k<3;++k){Edge e{f.v[k],f.v[(k+1)%3]};std::sort(e.begin(),e.end());edges.insert(e);}}
    for(auto i=p.coverage.faces.begin();i!=p.coverage.faces.end();)if(!faces.count(i->first))i=p.coverage.faces.erase(i);else ++i;
    for(auto i=p.coverage.traces.begin();i!=p.coverage.traces.end();)if(!edges.count(i->first))i=p.coverage.traces.erase(i);else ++i;
  };
  const auto identity=std::make_shared<const FrozenMetricIdentity>();
  budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,state->ResidentBytes(),reference->ResidentBytes(),
      2*MaximumFrozenFieldBytes,size_t(64)<<20);
  REQUIRE(budget.caller_bytes<ceiling);
  auto scope=reference->WithAtlasReservation(budget);
  for(const auto& test:cases) {
    if(compound) {
      if(current_capture) {
        const auto rows=Captured202Rows<OwnedCell>(directory+test.first+"_owned.bin",416);
        const auto core=std::find_if(rows.begin(),rows.end(),[&](const OwnedCell& row){return row.cell.id==test.second;});
        REQUIRE(core!=rows.end());
        if(std::count_if(core->physical.begin(),core->physical.end(),[](const auto& f){return f.present;})!=2)continue;
      } else if(test.second!=224633219 && test.second!=218068745)continue;
    }
    INFO(test.first);
    auto rows=Captured202Rows<OwnedCell>(directory+test.first+"_owned.bin",416);
    auto expected=Captured202Rows<Captured202Metric>(directory+test.first+"_expected_metrics.bin",168);
    REQUIRE(rows.size()==expected.size());REQUIRE(rows.size()==expected_unions.at(test.second));
    REQUIRE(rows.size()<=MaximumReplacementCells);
    Owned owned;std::vector<DonorCell> donors;
    for(const auto& row:rows){REQUIRE(row.sensor_present);REQUIRE(owned.emplace(row.cell.id,row).second);
      DonorCell d;d.cell=row.cell;d.sensor=row.sensor;donors.push_back(d);}
    // Context-only current nodal field; active operator inputs below remain bounded complete single-vertex stars.
    REQUIRE(donors.size()<=MaximumDonors);
    auto extensions=AssociatedExtensions(donors,*scope);
    FrozenField field(std::move(donors),MetricComposition{},false,extensions,MetricEdgeBreakpoints{},identity,true,MaximumDonors,true);
    auto complete=patch_from(owned);scope->ExportParameters(complete);
    const auto before=MeasureMetricCells(complete.cells,field);
    double q_error=0,j_error=0,edge_error=0;
    for(const auto& prior:expected){const auto measured=MeasureMetricCells({owned.at(prior.id).cell},field);
      q_error=std::max(q_error,std::abs(measured.minimum_mean_ratio-prior.q));
      j_error=std::max(j_error,std::abs(measured.minimum_scaled_jacobian-prior.J));
      for(const auto& e:prior.edges) {
        if(current_capture)REQUIRE(std::isnan(e.length));
        else edge_error=std::max(edge_error,double(std::abs(measured.edges.at(e.vertices).length-e.length)));
      }}
    REQUIRE(q_error<=1e-10);REQUIRE(j_error<=1e-10);REQUIRE(edge_error<=1e-10);
    log<<"{\"stage\":\"current_metric_match\",\"case\":"<<std::quoted(test.first)<<",\"cells\":"<<rows.size()
       <<",\"q_error\":"<<q_error<<",\"J_error\":"<<j_error<<",\"edge_error\":"<<edge_error
       <<",\"captured_edges_available\":"<<!current_capture<<"}\n";log.flush();
    auto assess=[&](Owned local,const std::set<Id>& retired,const Patch& seed,Patch input,Id vertex,const char* kind,bool optimize=true) {
      REQUIRE_FALSE(seed.cells.empty());REQUIRE(seed.cells.size()<=MaximumCavityCells);
      auto context=scope->PrivateParameters(input);REQUIRE(context);context->ExportParameters(input);
      const auto stats0=state->Geometry().GeometryStatsSnapshot();
      const auto started=std::chrono::steady_clock::now();
      OptimizeOptions options;options.maximum_work=160;options.maximum_seconds=30;options.boundary=false;
      options.cache_entries=8*MaximumReplacementCells;options.geometry=context.get();options.surface_tolerance=1e-4;
      Id node=600000000,cell=700000000;OptimizeReport report;
      const bool changed=optimize&&OptimizeMetricMesh(local,RegionMetric(field),node,cell,options,report);
      const auto optimized_stats=state->Geometry().GeometryStatsSnapshot();
      REQUIRE(report.trials<=160);REQUIRE(local.size()<=MaximumReplacementCells);
      auto result=patch_from(local);result.parameters=input.parameters;result.coverage=input.coverage;
      for(const auto& p:report.parameters)result.parameters.insert_or_assign(p.first,p.second);
      for(const auto& p:report.coverage.faces)result.coverage.faces.insert_or_assign(p.first,p.second);
      for(const auto& p:report.coverage.traces)result.coverage.traces.insert_or_assign(p.first,p.second);
      retain_coverage(result);
      auto final_context=context->PrivateParameters(result);REQUIRE(final_context);final_context->ExportParameters(result);
      std::string replacement_reason,interface_reason,metric_reason;
      const bool replacement=state->CheckReplacement(seed,result,replacement_reason,ceiling,budget.caller_bytes,&budget);
      std::set<Face> physical;for(auto f:seed.surface){std::sort(f.v.begin(),f.v.end());physical.insert(f.v);}
      std::map<Face,Face> skin;
      for(auto f:Incidence(seed.cells).Boundary()){auto key=f;std::sort(key.begin(),key.end());if(!physical.count(key))skin.emplace(key,f);}
      for(auto f:result.surface){std::rotate(f.v.begin(),std::min_element(f.v.begin(),f.v.end()),f.v.end());auto key=f.v;
        std::sort(key.begin(),key.end());skin.emplace(key,f.v);}
      std::vector<Face> prescribed;for(const auto& f:skin)prescribed.push_back(f.second);
      const bool interface_valid=ValidatePrescribedInterface(seed.cells,result.cells,prescribed,interface_reason);
      INFO(interface_reason);CHECK(interface_valid);INFO(replacement_reason);CHECK(replacement);
      Owned joined=owned;for(Id id:retired)joined.erase(id);
      for(const auto& row:local)REQUIRE(joined.emplace(row).second);
      REQUIRE(joined.size()<=MaximumReplacementCells);
      auto all=patch_from(joined);all.parameters=complete.parameters;all.coverage=complete.coverage;
      for(const auto& p:result.parameters)all.parameters.insert_or_assign(p.first,p.second);
      for(const auto& p:result.coverage.faces)all.coverage.faces.insert_or_assign(p.first,p.second);
      for(const auto& p:result.coverage.traces)all.coverage.traces.insert_or_assign(p.first,p.second);
      retain_coverage(all);
      auto all_context=scope->PrivateParameters(all);REQUIRE(all_context);all_context->ExportParameters(all);
      const auto after=MeasureMetricCells(all.cells,field);
      const auto old_active=MeasureMetricCells(seed.cells,field),new_active=MeasureMetricCells(result.cells,field);
      MetricDiagnosis diagnosis;
      const bool primitive=ValidateMetricChange(seed.cells.size(),old_active,result.cells.size(),new_active,MetricChange::Repair,metric_reason,&diagnosis);
      const bool metric_valid=primitive&&diagnosis.added_excess<=diagnosis.removed_excess&&
          (diagnosis.quality_progress||(new_active.minimum_mean_ratio>=MinimumMeanRatio&&new_active.minimum_scaled_jacobian>=MinimumScaledJacobian));
      const auto old_geometry=GeometryGuidanceExcess(complete,*scope,1e-4);
      const auto new_geometry=GeometryGuidanceExcess(all,*all_context,1e-4);
      REQUIRE(old_geometry.complete);REQUIRE(new_geometry.complete);
      auto geometry_bad=[&](const Owned& cells,const PlanarReference& geometry) {
        std::set<Id> bad_cells;
        for(const auto& row:cells){bool bad=false;
          for(unsigned slot=0;slot<4;++slot)if(row.second.physical[slot].present){std::array<Point,3> triangle;
            for(unsigned k=0;k<3;++k)triangle[k]=row.second.cell.v[opposite[slot][k]].p;
            const auto error=geometry.SurfaceDeviation(row.second.physical[slot].facet,triangle);
            REQUIRE(std::isfinite(error));bad=bad||error>1e-4;}
          if(bad)bad_cells.insert(row.first);}
        return bad_cells;
      };
      const auto old_geometry_bad=geometry_bad(owned,*scope),new_geometry_bad=geometry_bad(joined,*all_context);
      std::array<std::set<Id>,2> old_bad,new_bad;
      for(const auto& row:expected){if(row.q<MinimumMeanRatio)old_bad[0].insert(row.id);if(row.J<MinimumScaledJacobian)old_bad[1].insert(row.id);}
      for(const auto& row:joined){const auto m=MeasureMetricCells({row.second.cell},field);
        if(m.minimum_mean_ratio<MinimumMeanRatio)new_bad[0].insert(row.first);
        if(m.minimum_scaled_jacobian<MinimumScaledJacobian)new_bad[1].insert(row.first);}
      auto absent=[](const std::set<Id>& a,const std::set<Id>& b){size_t count=0;for(Id id:a)count+=!b.count(id);return count;};
      REQUIRE(old_bad[0].size()==before.q_bad_cells);REQUIRE(old_bad[1].size()==before.j_bad_cells);
      REQUIRE(new_bad[0].size()==after.q_bad_cells);REQUIRE(new_bad[1].size()==after.j_bad_cells);
      const auto stats1=state->Geometry().GeometryStatsSnapshot();
      const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
      log<<"{\"stage\":\"bounded_star_result\",\"case\":"<<std::quoted(test.first)<<",\"vertex\":"<<vertex
         <<",\"kind\":"<<std::quoted(kind)<<",\"active_cells\":"<<seed.cells.size()<<",\"context_cells\":"<<complete.cells.size()<<",\"new_context_cells\":"<<all.cells.size()
         <<",\"changed\":"<<changed<<",\"trials\":"<<report.trials<<",\"seconds\":"<<seconds<<",\"optimizer_seconds\":"<<report.seconds
         <<",\"stop\":"<<std::quoted(report.stop)<<",\"old_q_bad\":"<<before.q_bad_cells<<",\"new_q_bad\":"<<after.q_bad_cells
         <<",\"old_J_bad\":"<<before.j_bad_cells<<",\"new_J_bad\":"<<after.j_bad_cells
         <<",\"old_geometry_bad\":"<<old_geometry_bad.size()<<",\"new_geometry_bad\":"<<new_geometry_bad.size()
         <<",\"q_retired\":"<<absent(old_bad[0],new_bad[0])<<",\"q_introduced\":"<<absent(new_bad[0],old_bad[0])
         <<",\"J_retired\":"<<absent(old_bad[1],new_bad[1])<<",\"J_introduced\":"<<absent(new_bad[1],old_bad[1])
         <<",\"geometry_retired\":"<<absent(old_geometry_bad,new_geometry_bad)<<",\"geometry_introduced\":"<<absent(new_geometry_bad,old_geometry_bad)
         <<",\"old_geometry_excess\":"<<double(old_geometry.excess)<<",\"new_geometry_excess\":"<<double(new_geometry.excess)
         <<",\"old_maximum_edge\":"<<double(before.maximum_length)<<",\"new_maximum_edge\":"<<double(after.maximum_length)
         <<",\"added_size_excess\":"<<double(diagnosis.added_excess)<<",\"removed_size_excess\":"<<double(diagnosis.removed_excess)
         <<",\"metric_valid\":"<<metric_valid<<",\"metric_reason\":"<<std::quoted(metric_reason)
         <<",\"replacement\":"<<replacement<<",\"replacement_reason\":"<<std::quoted(replacement_reason)
         <<",\"interface_valid\":"<<interface_valid<<",\"interface_reason\":"<<std::quoted(interface_reason)
         <<",\"boundary_candidates\":"<<report.boundary_candidates<<",\"boundary_moves\":"<<report.boundary_moves
         <<",\"boundary_shape_rejected\":"<<report.boundary_rejected_shape<<",\"boundary_geometry_rejected\":"<<report.boundary_rejected_geometry
         <<",\"interior_candidates\":"<<report.interior_candidates<<",\"interior_moves\":"<<report.interior_moves
         <<",\"collapses\":"<<report.collapses<<",\"edge_removals\":"<<report.edge_removals<<",\"face_swaps\":"<<report.face_swaps
         <<",\"shell_insertions\":"<<report.shell_insertions<<",\"boundary_flips\":"<<report.boundary_flips
         <<",\"optimizer_upper_calls\":"<<optimized_stats.upper_calls-stats0.upper_calls
         <<",\"optimizer_traversal_work\":"<<optimized_stats.traversal_work-stats0.traversal_work
         <<",\"upper_calls\":"<<stats1.upper_calls-stats0.upper_calls<<",\"traversal_work\":"<<stats1.traversal_work-stats0.traversal_work
         <<",\"requested_heights\":0,\"nonlocal_contact_and_publication_checked\":false,\"accepted_mesh\":false}\n";log.flush();
      CHECK_FALSE(budget.work_exhausted);CHECK(budget.geometry_failure.empty());
      return metric_valid && replacement && interface_valid && after.q_bad_cells<=before.q_bad_cells &&
          after.j_bad_cells<=before.j_bad_cells && (after.q_bad_cells<before.q_bad_cells || after.j_bad_cells<before.j_bad_cells) &&
          new_geometry.excess<=old_geometry.excess;
    };
    auto subset=[&](Id a,Id b){Owned out;for(const auto& row:owned)
      if(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return n.id==a||n.id==b;}))out.insert(row);
      if(a==b)REQUIRE(out.size()==expected_stars.at(a));return out;};
    bool useful_star=false;
    for(const auto& vertex:owned.at(test.second).cell.v) {
      auto local=subset(vertex.id,vertex.id);std::set<Id> retired;for(const auto& row:local)retired.insert(row.first);
      if(local.size()>MaximumCavityCells) {
        log<<"{\"stage\":\"existing_star_limit\",\"case\":"<<std::quoted(test.first)
           <<",\"vertex\":"<<vertex.id<<",\"cells\":"<<local.size()<<"}\n";log.flush();continue;
      }
      auto seed=patch_from(local);scope->ExportParameters(seed);
      useful_star=assess(local,retired,seed,seed,vertex.id,"original_star")||useful_star;
    }
    if(compound && !current_capture && test.second==224633219)CHECK(useful_star);
    if(current_capture && !compound && test.second==219735950) {
      REQUIRE(complete.cells.size()<=MaximumCavityCells);
      std::set<Id> retired;for(const auto& row:owned)retired.insert(row.first);
      CHECK(assess(owned,retired,complete,complete,test.second,"production_complete_context"));
    }
    if(compound)for(const auto& removed:owned.at(test.second).cell.v)for(const auto& retained:owned.at(test.second).cell.v) {
      if(removed.id==retained.id)continue;
      auto removed_rows=subset(removed.id,removed.id);auto old=patch_from(removed_rows);scope->ExportParameters(old);
      Patch fresh;std::string why;
      const auto started=std::chrono::steady_clock::now();
      const bool constructed=CollapseVertex(old,removed.id,retained.id,800000000,*scope,fresh,why);
      log<<"{\"stage\":\"current_directed_collapse\",\"case\":"<<std::quoted(test.first)
         <<",\"removed\":"<<removed.id<<",\"retained\":"<<retained.id<<",\"constructed\":"<<constructed
         <<",\"reason\":"<<std::quoted(why)<<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<"}\n";log.flush();
      if(!constructed)continue;
      auto prior=subset(removed.id,retained.id);REQUIRE(prior.size()<=MaximumCavityCells);
      auto seed=patch_from(prior);scope->ExportParameters(seed);std::set<Id> retired;
      for(const auto& row:prior)retired.insert(row.first);
      for(const auto& row:removed_rows)prior.erase(row.first);
      std::map<Face,Id> physical;for(auto f:fresh.surface){std::sort(f.v.begin(),f.v.end());physical.emplace(f.v,f.facet);}
      for(const auto& c:fresh.cells){OwnedCell row;row.cell=c;
        for(unsigned slot=0;slot<4;++slot){Face key;for(unsigned k=0;k<3;++k)key[k]=c.v[opposite[slot][k]].id;
          std::sort(key.begin(),key.end());auto f=physical.find(key);if(f!=physical.end())row.physical[slot]={true,f->second};}
        REQUIRE(prior.emplace(c.id,row).second);}
      REQUIRE(prior.size()<=MaximumCavityCells);
      auto input=patch_from(prior);input.parameters=seed.parameters;input.coverage=seed.coverage;
      for(const auto& row:fresh.parameters)input.parameters.insert_or_assign(row.first,row.second);
      for(const auto& row:fresh.coverage.faces)input.coverage.faces.insert_or_assign(row.first,row.second);
      for(const auto& row:fresh.coverage.traces)input.coverage.traces.insert_or_assign(row.first,row.second);
      retain_coverage(input);
      assess(prior,retired,seed,input,retained.id,"collapse_then_retained_star_optimizer");
    }
    if(compound) {
      const auto& core=owned.at(test.second);std::vector<unsigned> faces;
      for(unsigned slot=0;slot<4;++slot)if(core.physical[slot].present)faces.push_back(slot);
      REQUIRE(faces.size()==2);
      if(!current_capture)REQUIRE(state->Entity(core.physical[faces[0]].facet)==state->Entity(core.physical[faces[1]].facet));
      const Id a=core.cell.v[faces[0]].id,b=core.cell.v[faces[1]].id;
      Owned shell;
      for(const auto& row:owned){bool has_a=false,has_b=false;for(Node n:row.second.cell.v){has_a=has_a||n.id==a;has_b=has_b||n.id==b;}
        if(has_a&&has_b)shell.insert(row);}
      if(!current_capture)REQUIRE(shell.size()==(test.second==224633219?4:6));
      REQUIRE_FALSE(shell.empty());REQUIRE(shell.size()<=MaximumCavityCells);
      auto old=patch_from(shell);scope->ExportParameters(old);Patch fresh;std::string reason;
      const auto started=std::chrono::steady_clock::now();
      const bool constructed=ShellBoundaryCell(old,test.second,800000000,*scope,fresh,reason);
      log<<"{\"stage\":\"current_shell_boundary\",\"case\":"<<std::quoted(test.first)<<",\"core\":"<<test.second
         <<",\"ring_cells\":"<<shell.size()<<",\"constructed\":"<<constructed<<",\"reason\":"<<std::quoted(reason)
         <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<"}\n";log.flush();
      if(constructed) {
        std::set<Id> retired;for(const auto& row:shell)retired.insert(row.first);
        Owned local;std::map<Face,Id> physical;
        for(auto f:fresh.surface){std::sort(f.v.begin(),f.v.end());physical.emplace(f.v,f.facet);}
        for(const auto& c:fresh.cells){OwnedCell row;row.cell=c;
          for(unsigned slot=0;slot<4;++slot){Face key;for(unsigned k=0;k<3;++k)key[k]=c.v[opposite[slot][k]].id;
            std::sort(key.begin(),key.end());auto f=physical.find(key);if(f!=physical.end())row.physical[slot]={true,f->second};}
          REQUIRE(local.emplace(c.id,row).second);}
        assess(local,retired,old,fresh,test.second,"existing_shell_without_optimizer",false);
        if(current_capture && complete.cells.size()<=MaximumCavityCells) {
          // Complete existing stars permit relocation after shelling; final merit is against the original patch.
          Owned joined=owned;for(Id id:retired)joined.erase(id);
          for(const auto& row:local)REQUIRE(joined.emplace(row).second);
          auto input=patch_from(joined);input.parameters=complete.parameters;input.coverage=complete.coverage;
          for(const auto& p:fresh.parameters)input.parameters.insert_or_assign(p.first,p.second);
          for(const auto& p:fresh.coverage.faces)input.coverage.faces.insert_or_assign(p.first,p.second);
          for(const auto& p:fresh.coverage.traces)input.coverage.traces.insert_or_assign(p.first,p.second);
          retain_coverage(input);
          std::set<Id> all_retired;for(const auto& row:owned)all_retired.insert(row.first);
          assess(joined,all_retired,complete,input,test.second,"existing_shell_then_complete_context_optimizer");
          const Point pa=core.cell.v[faces[0]].p,pb=core.cell.v[faces[1]].p;
          const Node midpoint{900000000,{double(((long double)pa.x+pb.x)/2),double(((long double)pa.y+pb.y)/2),double(((long double)pa.z+pb.z)/2)}};
          auto raw_context=scope->PrivateParameters(fresh);REQUIRE(raw_context);Patch refined;std::string split_reason;
          const bool split=SplitEdge(fresh,{a,b},midpoint,900000001,*raw_context,refined,split_reason);
          log<<"{\"stage\":\"existing_shell_geometry_split\",\"case\":"<<std::quoted(test.first)
             <<",\"constructed\":"<<split<<",\"reason\":"<<std::quoted(split_reason)<<"}\n";log.flush();
          if(split) {
            Owned combined=owned;for(Id id:retired)combined.erase(id);
            std::map<Face,Id> refined_physical;
            for(auto f:refined.surface){std::sort(f.v.begin(),f.v.end());refined_physical.emplace(f.v,f.facet);}
            for(const auto& c:refined.cells){OwnedCell row;row.cell=c;
              for(unsigned slot=0;slot<4;++slot){Face key;for(unsigned k=0;k<3;++k)key[k]=c.v[opposite[slot][k]].id;
                std::sort(key.begin(),key.end());auto f=refined_physical.find(key);if(f!=refined_physical.end())row.physical[slot]={true,f->second};}
              REQUIRE(combined.emplace(c.id,row).second);}
            auto proposed=patch_from(combined);proposed.parameters=complete.parameters;proposed.coverage=complete.coverage;
            for(const auto& p:refined.parameters)proposed.parameters.insert_or_assign(p.first,p.second);
            for(const auto& p:refined.coverage.faces)proposed.coverage.faces.insert_or_assign(p.first,p.second);
            for(const auto& p:refined.coverage.traces)proposed.coverage.traces.insert_or_assign(p.first,p.second);
            retain_coverage(proposed);
            assess(combined,all_retired,complete,proposed,test.second,"existing_shell_geometry_split_then_optimizer");
          }
        }
      }
    }
  }
  log.close();REQUIRE(bool(log));
}
TEST_CASE("Current M6 residual stars assess existing coupled repair and complete neighbors",
          "[.][NativeCurrentResidual3D]") {CapturedCurrentResidual(false);}
TEST_CASE("Current M6 boundary residuals assess existing collapse and retained-star optimization",
          "[.][NativeCurrentCompound3D]") {CapturedCurrentResidual(true);}
