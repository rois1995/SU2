// Actual corrected-target residuals: existing operators only, never historical replay or acceptance.
static void CapturedCurrentResidual(bool compound,bool distributed=false,bool production_source=false,bool metric_cleanup=false) {
  World world;if(!distributed)REQUIRE(world.size==1);
  const auto* cfg=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(cfg);
  const std::string config(cfg);const auto slash=config.find_last_of('/');REQUIRE(slash!=std::string::npos);
  const auto directory=config.substr(0,slash+1);
  std::ifstream current_cases(directory+"current_residual_cases.tsv");
  const bool current_capture=bool(current_cases);
  if(production_source)REQUIRE(current_capture);
  const auto logfile=distributed?"native_current_residual_rank"+std::to_string(world.rank)+".jsonl":"native_current_residual_result.jsonl";
  REQUIRE_FALSE(bool(std::ifstream(logfile)));
  std::ofstream log(logfile);REQUIRE(bool(log));log<<std::setprecision(17)<<std::boolalpha;
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
  std::map<std::string,std::vector<Edge>> construction_edges;
  std::ifstream edge_cases(directory+"current_geometry_edges.tsv");
  if(edge_cases) {
    std::string label;Edge edge;size_t count=0;
    while(edge_cases>>label>>edge[0]>>edge[1]) {
      REQUIRE(++count<=12);REQUIRE(edge[0]<edge[1]);construction_edges[label].push_back(edge);
    }
    REQUIRE(edge_cases.eof());REQUIRE(current_capture);
  }
  if(metric_cleanup){REQUIRE(production_source);REQUIRE(compound);REQUIRE_FALSE(distributed);
    REQUIRE(cases.size()==4);REQUIRE(construction_edges.size()==cases.size());
    for(const auto& c:cases)REQUIRE(construction_edges.at(c.first).size()==1);}
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
      (production_source?4:2)*MaximumFrozenFieldBytes,size_t(64)<<20);
  REQUIRE(budget.caller_bytes<ceiling);
  auto scope=reference->WithAtlasReservation(budget);
  size_t source_geometry_candidates=0;
  for(const auto& test:cases) {
    if(compound && !current_capture && test.second!=224633219 && test.second!=218068745)continue;
    INFO(test.first);
    auto rows=Captured202Rows<OwnedCell>(directory+test.first+"_owned.bin",416);
    auto expected=Captured202Rows<Captured202Metric>(directory+test.first+"_expected_metrics.bin",168);
    REQUIRE(rows.size()==expected.size());REQUIRE(rows.size()==expected_unions.at(test.second));
    REQUIRE(rows.size()<=MaximumReplacementCells);
    Owned owned;std::vector<DonorCell> donors;
    for(const auto& row:rows){REQUIRE(row.sensor_present);REQUIRE(owned.emplace(row.cell.id,row).second);
      DonorCell d;d.cell=row.cell;d.sensor=row.sensor;donors.push_back(d);}
    // Current nodal field for bounded complete-star and complete-context operator comparisons.
    REQUIRE(donors.size()<=MaximumDonors);
    auto extensions=AssociatedExtensions(donors,*scope);
    FrozenField field(std::move(donors),MetricComposition{},false,extensions,MetricEdgeBreakpoints{},identity,true,MaximumDonors,true);
    CavityMetricSource metric_source(world,owned);
    auto complete=patch_from(owned);scope->ExportParameters(complete);
    const auto before=MeasureMetricCells(complete.cells,field);
    double q_error=0,j_error=0,edge_error=0;
    size_t known_q=0,known_j=0,unknown_q=0,unknown_j=0;
    for(const auto& prior:expected){const auto measured=MeasureMetricCells({owned.at(prior.id).cell},field);
      if(std::isfinite(prior.q)){++known_q;q_error=std::max(q_error,std::abs(measured.minimum_mean_ratio-prior.q));}
      else {REQUIRE(production_source);REQUIRE(std::isnan(prior.q));++unknown_q;}
      if(std::isfinite(prior.J)){++known_j;j_error=std::max(j_error,std::abs(measured.minimum_scaled_jacobian-prior.J));}
      else {REQUIRE(production_source);REQUIRE(std::isnan(prior.J));++unknown_j;}
      for(const auto& e:prior.edges) {
        if(current_capture&&!production_source)REQUIRE(std::isnan(e.length));
        else {REQUIRE(std::isfinite(e.length));edge_error=std::max(edge_error,double(std::abs(measured.edges.at(e.vertices).length-e.length)));}
      }}
    REQUIRE(known_q+unknown_q==rows.size());REQUIRE(known_j+unknown_j==rows.size());
    REQUIRE(known_q>0);REQUIRE(known_j>0);
    REQUIRE(q_error<=1e-10);REQUIRE(j_error<=1e-10);REQUIRE(edge_error<=1e-10);
    log<<"{\"stage\":\"current_metric_match\",\"case\":"<<std::quoted(test.first)<<",\"cells\":"<<rows.size()
       <<",\"q_error\":"<<q_error<<",\"J_error\":"<<j_error<<",\"edge_error\":"<<edge_error
       <<",\"captured_edges_available\":"<<(!current_capture||production_source)
       <<",\"known_q_oracles\":"<<known_q<<",\"known_J_oracles\":"<<known_j
       <<",\"unknown_q_oracles\":"<<unknown_q<<",\"unknown_J_oracles\":"<<unknown_j
       <<",\"production_current_source\":"<<production_source<<"}\n";log.flush();
    auto assess=[&](Owned local,const std::set<Id>& retired,const Patch& seed,Patch input,Id vertex,const char* kind,bool optimize=true,size_t relocated=0,
                    const GeometrySplitPrediction* prediction=nullptr,bool* needs_reclosure=nullptr) {
      REQUIRE_FALSE(seed.cells.empty());REQUIRE(seed.cells.size()<=MaximumCavityCells);
      auto context=scope->PrivateParameters(input);REQUIRE(context);context->ExportParameters(input);
      std::vector<OwnedCell> source_rows;
      std::unique_ptr<FrozenField> proposal_field;
      if(production_source) {
        for(Id id:retired)source_rows.push_back(owned.at(id));
        REQUIRE(source_rows.size()==seed.cells.size());
        auto imported=metric_source.ImportCellsLocal(seed.cells,source_rows,ceiling,budget.caller_bytes);
        INFO(imported.reason);REQUIRE(imported.valid);
        const auto components=AssociatedExtensions(imported.cells,*scope);
        proposal_field=std::make_unique<FrozenField>(std::move(imported.cells),MetricComposition{},false,
            components,MetricEdgeBreakpoints{},identity,true,MaximumDonors,true);
      }
      const auto stats0=state->Geometry().GeometryStatsSnapshot();
      const auto started=std::chrono::steady_clock::now();
      OptimizeOptions options;options.maximum_work=160;options.maximum_seconds=30;options.boundary=false;
      options.cache_entries=8*MaximumReplacementCells;options.geometry=context.get();options.surface_tolerance=1e-4;
      Id node=600000000,cell=700000000;OptimizeReport report;
      const bool changed=optimize&&OptimizeMetricMesh(local,RegionMetric(proposal_field?*proposal_field:field),node,cell,options,report);
      const auto optimized_stats=state->Geometry().GeometryStatsSnapshot();
      REQUIRE(report.trials<=160);REQUIRE(local.size()<=MaximumReplacementCells);
      auto result=patch_from(local);result.parameters=input.parameters;result.coverage=input.coverage;
      for(const auto& p:report.parameters)result.parameters.insert_or_assign(p.first,p.second);
      for(const auto& p:report.coverage.faces)result.coverage.faces.insert_or_assign(p.first,p.second);
      for(const auto& p:report.coverage.traces)result.coverage.traces.insert_or_assign(p.first,p.second);
      retain_coverage(result);
      auto final_context=context->PrivateParameters(result);REQUIRE(final_context);final_context->ExportParameters(result);
      std::unique_ptr<FrozenField> gate_field;
      if(production_source) {
        std::map<Id,Point> previous;for(const auto& row:source_rows)for(Node n:row.cell.v)previous.emplace(n.id,n.p);
        for(const auto& cell:result.cells)for(Node n:cell.v)if(previous.count(n.id)) {
          const auto p=previous.at(n.id);REQUIRE(n.p.x==p.x);REQUIRE(n.p.y==p.y);REQUIRE(n.p.z==p.z);
        }
        auto imported=metric_source.ImportMetricSamplesLocal(result.cells,source_rows,{},ceiling,budget.caller_bytes);
        INFO(imported.reason);REQUIRE(imported.valid);
        std::vector<OwnedCell> published;for(const auto& row:local)published.push_back(row.second);
        PublishMetrics(source_rows,published);
        std::map<Id,const DonorCell*> sampled;for(const auto& row:imported.cells)REQUIRE(sampled.emplace(row.cell.id,&row).second);
        for(const auto& row:published) {
          const auto& donor=*sampled.at(row.cell.id);
          for(unsigned k=0;k<4;++k)REQUIRE(SameMetric(row.sensor[k],donor.sensor[k]));
          local.at(row.cell.id)=row;
        }
        const auto components=AssociatedExtensions(imported.cells,*scope);
        gate_field=std::make_unique<FrozenField>(std::move(imported.cells),MetricComposition{},false,
            components,MetricEdgeBreakpoints{},identity,true,MaximumDonors,true);
      }
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
      std::unique_ptr<FrozenField> joined_field;
      if(production_source) {
        std::vector<DonorCell> donors;
        for(const auto& row:joined){REQUIRE(row.second.sensor_present);DonorCell donor;
          donor.cell=row.second.cell;donor.sensor=row.second.sensor;donors.push_back(donor);}
        const auto components=AssociatedExtensions(donors,*scope);
        joined_field=std::make_unique<FrozenField>(std::move(donors),MetricComposition{},false,
            components,MetricEdgeBreakpoints{},identity,true,MaximumDonors,true);
      }
      auto& after_field=joined_field?*joined_field:field;
      const auto after=MeasureMetricCells(all.cells,after_field);
      const auto old_active=MeasureMetricCells(seed.cells,proposal_field?*proposal_field:field);
      const auto new_active=MeasureMetricCells(result.cells,gate_field?*gate_field:field);
      const auto proposal_metric=production_source?MeasureMetricCells(result.cells,*proposal_field):new_active;
      MetricDiagnosis diagnosis;
      const bool primitive=ValidateMetricChange(seed.cells.size(),old_active,result.cells.size(),new_active,MetricChange::Repair,metric_reason,&diagnosis);
      MetricDiagnosis geometry_diagnosis;std::string geometry_metric_reason;
      const bool geometry_metric=ValidateMetricChange(seed.cells.size(),old_active,result.cells.size(),new_active,
          MetricChange::GeometryRefine,geometry_metric_reason,&geometry_diagnosis,relocated);
      MetricDiagnosis geometry_repair_diagnosis;std::string geometry_repair_reason;
      const bool geometry_repair_metric=ValidateMetricChange(seed.cells.size(),old_active,result.cells.size(),new_active,
          MetricChange::GeometryRepair,geometry_repair_reason,&geometry_repair_diagnosis,relocated);
      const bool metric_valid=primitive&&diagnosis.added_excess<=diagnosis.removed_excess&&
          (diagnosis.quality_progress||(new_active.minimum_mean_ratio>=MinimumMeanRatio&&new_active.minimum_scaled_jacobian>=MinimumScaledJacobian));
      const auto old_geometry=GeometryGuidanceExcess(complete,*scope,1e-4);
      const auto new_geometry=GeometryGuidanceExcess(all,*all_context,1e-4);
      REQUIRE(old_geometry.complete);REQUIRE(new_geometry.complete);
      if(prediction&&prediction->complete) {
        const long double actual=new_geometry.excess-old_geometry.excess,predicted=prediction->after-prediction->before;
        const long double scale=std::max({old_geometry.excess,new_geometry.excess,prediction->before,prediction->after,1e-24L});
        CHECK(std::abs(actual-predicted)<=1e-10L*scale);
      }
      struct FaceErrors {std::set<Id> bad_cells;size_t bad_faces=0;long double excess_squared=0;double maximum_error=0;};
      auto geometry_errors=[&](const Owned& cells,const PlanarReference& geometry) {
        FaceErrors errors;
        for(const auto& row:cells){bool bad=false;
          for(unsigned slot=0;slot<4;++slot)if(row.second.physical[slot].present){std::array<Point,3> triangle;
            for(unsigned k=0;k<3;++k)triangle[k]=row.second.cell.v[opposite[slot][k]].p;
            const auto error=geometry.SurfaceDeviation(row.second.physical[slot].facet,triangle);
            REQUIRE(std::isfinite(error));bad=bad||error>1e-4;
            errors.bad_faces+=error>1e-4;errors.maximum_error=std::max(errors.maximum_error,error);
            const long double excess=std::max(0.L,static_cast<long double>(error)-1e-4);
            errors.excess_squared+=excess*excess;}
          if(bad)errors.bad_cells.insert(row.first);}
        return errors;
      };
      const auto old_face_errors=geometry_errors(owned,*scope),new_face_errors=geometry_errors(joined,*all_context);
      const auto& old_geometry_bad=old_face_errors.bad_cells;const auto& new_geometry_bad=new_face_errors.bad_cells;
      auto vertex_count=[](const Owned& cells){std::set<Id> vertices;for(const auto& row:cells)
        for(const auto& node:row.second.cell.v)vertices.insert(node.id);return vertices.size();};
      std::array<std::set<Id>,2> old_bad,new_bad;
      if(production_source)for(const auto& row:owned){const auto m=MeasureMetricCells({row.second.cell},field);
        if(m.minimum_mean_ratio<MinimumMeanRatio)old_bad[0].insert(row.first);
        if(m.minimum_scaled_jacobian<MinimumScaledJacobian)old_bad[1].insert(row.first);}
      else for(const auto& row:expected){if(row.q<MinimumMeanRatio)old_bad[0].insert(row.id);if(row.J<MinimumScaledJacobian)old_bad[1].insert(row.id);}
      for(const auto& row:joined){const auto m=MeasureMetricCells({row.second.cell},after_field);
        if(m.minimum_mean_ratio<MinimumMeanRatio)new_bad[0].insert(row.first);
        if(m.minimum_scaled_jacobian<MinimumScaledJacobian)new_bad[1].insert(row.first);}
      auto absent=[](const std::set<Id>& a,const std::set<Id>& b){size_t count=0;for(Id id:a)count+=!b.count(id);return count;};
      REQUIRE(old_bad[0].size()==before.q_bad_cells);REQUIRE(old_bad[1].size()==before.j_bad_cells);
      REQUIRE(new_bad[0].size()==after.q_bad_cells);REQUIRE(new_bad[1].size()==after.j_bad_cells);
      const auto stats1=state->Geometry().GeometryStatsSnapshot();
      const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
      const bool geometry_progress=new_geometry.excess<old_geometry.excess*(1-1e-12L);
      const bool geometry_admissible=geometry_repair_metric&&replacement&&interface_valid&&
          after.q_bad_cells==0&&after.j_bad_cells==0&&after.maximum_length<=MaximumMetricEdge&&geometry_progress&&
          (!production_source || (new_geometry_bad.size()<=old_geometry_bad.size()&&
              new_face_errors.bad_faces<old_face_errors.bad_faces));
      auto short_deficit=[](const PatchMetric& metric){long double sum=0;
        for(const auto& edge:metric.edges){const auto d=std::max(0.L,CoarseningMetricEdge-edge.second.length);sum+=d*d;}return sum;};
      bool cleanup_metric=false;std::string cleanup_reason;
      if(metric_cleanup)cleanup_metric=ValidateMetricChange(seed.cells.size(),old_active,result.cells.size(),new_active,
          MetricChange::Coarsen,cleanup_reason);
      const bool cleanup_feasible=metric_cleanup&&cleanup_metric&&replacement&&interface_valid&&
          before.q_bad_cells==0&&before.j_bad_cells==0&&after.q_bad_cells==0&&after.j_bad_cells==0&&
          old_face_errors.bad_cells.empty()&&new_face_errors.bad_cells.empty()&&after.maximum_length<=MaximumMetricEdge&&
          vertex_count(joined)<vertex_count(owned)&&short_deficit(after)<short_deficit(before);
      source_geometry_candidates+=production_source&&geometry_admissible;
      if(needs_reclosure)*needs_reclosure=production_source&&geometry_repair_metric&&replacement&&interface_valid&&
          geometry_progress&&after.maximum_length<=MaximumMetricEdge&&(after.q_bad_cells||after.j_bad_cells);
      log<<"{\"stage\":\"bounded_star_result\",\"case\":"<<std::quoted(test.first)<<",\"vertex\":"<<vertex
         <<",\"kind\":"<<std::quoted(kind)<<",\"active_cells\":"<<seed.cells.size()<<",\"context_cells\":"<<complete.cells.size()<<",\"new_context_cells\":"<<all.cells.size()
         <<",\"changed\":"<<changed<<",\"trials\":"<<report.trials<<",\"seconds\":"<<seconds<<",\"optimizer_seconds\":"<<report.seconds
         <<",\"stop\":"<<std::quoted(report.stop)<<",\"old_q_bad\":"<<before.q_bad_cells<<",\"new_q_bad\":"<<after.q_bad_cells
         <<",\"old_J_bad\":"<<before.j_bad_cells<<",\"new_J_bad\":"<<after.j_bad_cells
         <<",\"old_geometry_bad\":"<<old_geometry_bad.size()<<",\"new_geometry_bad\":"<<new_geometry_bad.size()
         <<",\"q_retired\":"<<absent(old_bad[0],new_bad[0])<<",\"q_introduced\":"<<absent(new_bad[0],old_bad[0])
         <<",\"J_retired\":"<<absent(old_bad[1],new_bad[1])<<",\"J_introduced\":"<<absent(new_bad[1],old_bad[1])
         <<",\"geometry_retired\":"<<absent(old_geometry_bad,new_geometry_bad)<<",\"geometry_introduced\":"<<absent(new_geometry_bad,old_geometry_bad)
         <<",\"old_bad_physical_faces\":"<<old_face_errors.bad_faces<<",\"new_bad_physical_faces\":"<<new_face_errors.bad_faces
         <<",\"old_face_excess_squared\":"<<double(old_face_errors.excess_squared)<<",\"new_face_excess_squared\":"<<double(new_face_errors.excess_squared)
         <<",\"old_maximum_face_error\":"<<old_face_errors.maximum_error<<",\"new_maximum_face_error\":"<<new_face_errors.maximum_error
         <<",\"old_context_vertices\":"<<vertex_count(owned)<<",\"new_context_vertices\":"<<vertex_count(joined)
         <<",\"old_geometry_excess\":"<<double(old_geometry.excess)<<",\"new_geometry_excess\":"<<double(new_geometry.excess)
         <<",\"old_maximum_edge\":"<<double(before.maximum_length)<<",\"new_maximum_edge\":"<<double(after.maximum_length)
         <<",\"added_size_excess\":"<<double(diagnosis.added_excess)<<",\"removed_size_excess\":"<<double(diagnosis.removed_excess)
         <<",\"metric_valid\":"<<metric_valid<<",\"metric_reason\":"<<std::quoted(metric_reason)
         <<",\"geometry_refine_valid\":"<<geometry_metric<<",\"geometry_refine_reason\":"<<std::quoted(geometry_metric_reason)
         <<",\"geometry_repair_metric_valid\":"<<geometry_repair_metric<<",\"geometry_repair_metric_reason\":"<<std::quoted(geometry_repair_reason)
         <<",\"production_current_source\":"<<production_source<<",\"production_source_rows\":"<<source_rows.size()
         <<",\"proposal_q_bad\":"<<proposal_metric.q_bad_cells<<",\"source_gate_q_bad\":"<<new_active.q_bad_cells
         <<",\"proposal_J_bad\":"<<proposal_metric.j_bad_cells<<",\"source_gate_J_bad\":"<<new_active.j_bad_cells
         <<",\"production_geometry_admissible\":"<<(production_source&&geometry_admissible)
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
         <<",\"native_geometry_progress\":"<<geometry_progress;
      if(metric_cleanup) {
        log<<",\"diagnostic_cleanup_feasible\":"<<cleanup_feasible<<",\"coarsen_metric_valid\":"<<cleanup_metric
           <<",\"coarsen_metric_reason\":"<<std::quoted(cleanup_reason)
           <<",\"old_short_edge_deficit\":"<<double(short_deficit(before))
           <<",\"new_short_edge_deficit\":"<<double(short_deficit(after));
        auto distributions=[&](const char* name,const Patch& patch,const PatchMetric& metric) {
          std::map<Id,Point> coordinates;for(const auto& cell:patch.cells)for(Node n:cell.v)coordinates.emplace(n.id,n.p);
          std::vector<double> lengths,span_lengths,nose_lengths,ratios;size_t short09=0,short05=0;
          for(const auto& edge:metric.edges){lengths.push_back(double(edge.second.length));short09+=edge.second.length<.9L;short05+=edge.second.length<.5L;}
          for(const auto& face:patch.surface) {
            std::array<Point,3> p;for(unsigned k=0;k<3;++k)p[k]=coordinates.at(face.v[k]);
            const double u[3]={p[1].x-p[0].x,p[1].y-p[0].y,p[1].z-p[0].z},v[3]={p[2].x-p[0].x,p[2].y-p[0].y,p[2].z-p[0].z};
            double n[3]={u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
            const auto norm=std::hypot(n[0],n[1],n[2]);REQUIRE(norm>0);for(auto& x:n)x/=norm;
            double span[3]={1/std::sqrt(3.),1,0};const double dot=span[0]*n[0]+span[1]*n[1];
            for(unsigned k=0;k<3;++k)span[k]-=dot*n[k];const auto sn=std::hypot(span[0],span[1],span[2]);REQUIRE(sn>0);for(auto& x:span)x/=sn;
            const double nose[3]={n[1]*span[2]-n[2]*span[1],n[2]*span[0]-n[0]*span[2],n[0]*span[1]-n[1]*span[0]};
            double sl=INFINITY,sh=-INFINITY,nl=INFINITY,nh=-INFINITY;
            for(Point x:p){const auto s=x.x*span[0]+x.y*span[1]+x.z*span[2],t=x.x*nose[0]+x.y*nose[1]+x.z*nose[2];sl=std::min(sl,s);sh=std::max(sh,s);nl=std::min(nl,t);nh=std::max(nh,t);}
            REQUIRE(nh>nl);ratios.push_back((sh-sl)/(nh-nl));
            for(unsigned k=0;k<3;++k){const unsigned j=(k+1)%3;Edge e{face.v[k],face.v[j]};std::sort(e.begin(),e.end());
              const double dx=p[j].x-p[k].x,dy=p[j].y-p[k].y,dz=p[j].z-p[k].z;
              const bool along=std::abs(dx*span[0]+dy*span[1]+dz*span[2])>std::abs(dx*nose[0]+dy*nose[1]+dz*nose[2]);
              (along?span_lengths:nose_lengths).push_back(double(metric.edges.at(e).length));}
          }
          auto quantiles=[&](std::vector<double> values){log<<'[';if(!values.empty()){std::sort(values.begin(),values.end());bool comma=false;
            for(double p:{0.,.1,.5,.9,1.}){const auto at=p*(values.size()-1);const size_t lo=size_t(at),hi=std::min(lo+1,values.size()-1);
              if(comma)log<<',';comma=true;log<<values[lo]+(values[hi]-values[lo])*(at-lo);}}log<<']';};
          log<<",\""<<name<<"\":{\"unique_edges\":"<<metric.edges.size()<<",\"short09\":"<<short09<<",\"short05\":"<<short05<<",\"edge_quantiles\":";quantiles(lengths);
          log<<",\"span_face_edge_quantiles\":";quantiles(span_lengths);log<<",\"nose_face_edge_quantiles\":";quantiles(nose_lengths);
          log<<",\"span_over_nose_extent_quantiles\":";quantiles(ratios);log<<'}';
        };
        distributions("old_directional_conformity",complete,before);distributions("new_directional_conformity",all,after);
      }
      if(production_source) {
        std::map<Id,Point> prior_nodes,fresh_nodes;
        for(const auto& cell:input.cells)for(Node n:cell.v)prior_nodes.emplace(n.id,n.p);
        for(const auto& cell:result.cells)for(Node n:cell.v)fresh_nodes.emplace(n.id,n.p);
        auto changed_nodes=[&](const auto& from,const auto& to) {
          bool comma=false;for(const auto& row:from)if(!to.count(row.first)) {
            if(comma)log<<',';comma=true;log<<'['<<row.first<<','<<row.second.x<<','<<row.second.y<<','<<row.second.z<<']';
          }
        };
        log<<",\"optimizer_removed_nodes\":[";changed_nodes(prior_nodes,fresh_nodes);
        log<<"],\"optimizer_added_nodes\":[";changed_nodes(fresh_nodes,prior_nodes);log<<']';
      }
      log<<",\"requested_heights\":0,\"nonlocal_contact_and_publication_checked\":false,\"accepted_mesh\":false}\n";log.flush();
      CHECK_FALSE(budget.work_exhausted);CHECK(budget.geometry_failure.empty());
      if(metric_cleanup)return cleanup_feasible;
      if(production_source)return geometry_admissible&&new_geometry_bad.size()<old_geometry_bad.size();
      return metric_valid && replacement && interface_valid && after.q_bad_cells<=before.q_bad_cells &&
          after.j_bad_cells<=before.j_bad_cells && (after.q_bad_cells<before.q_bad_cells || after.j_bad_cells<before.j_bad_cells) &&
          new_geometry.excess<=old_geometry.excess;
    };
    auto subset=[&](Id a,Id b){Owned out;for(const auto& row:owned)
      if(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return n.id==a||n.id==b;}))out.insert(row);
      if(a==b)REQUIRE(out.size()==expected_stars.at(a));return out;};
    if(!metric_cleanup&&!construction_edges.empty()) {
      const auto selected=construction_edges.find(test.first);REQUIRE(selected!=construction_edges.end());
      for(const auto& edge:selected->second) {
        Owned ring;std::map<Id,Node> nodes;
        for(const auto& row:owned) {
          unsigned count=0;for(Node n:row.second.cell.v){nodes.emplace(n.id,n);count+=n.id==edge[0]||n.id==edge[1];}
          if(count==2)ring.insert(row);
        }
        REQUIRE_FALSE(ring.empty());const auto a=nodes.at(edge[0]).p,b=nodes.at(edge[1]).p;
        const Node midpoint{900000000,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
        for(unsigned kind=distributed?8:0;kind<9;++kind) {
          Owned motion;std::vector<Id> apices;
          if(kind==8) {
            auto edge_patch=patch_from(ring);scope->ExportParameters(edge_patch);apices=SplitPatchApices(edge_patch,edge);
            REQUIRE_FALSE(apices.empty());
            for(Id apex:apices){REQUIRE(expected_stars.count(apex));auto star=subset(apex,apex);motion.insert(star.begin(),star.end());}
            for(const auto& row:ring)REQUIRE(motion.count(row.first));
          }
          const auto& active=kind==8?motion:kind>=3?owned:ring;auto seed=patch_from(active);scope->ExportParameters(seed);
          Patch fresh;Face obstruction{};std::string reason;bool constructed=false;JointSplitReport placement;
          const auto started=std::chrono::steady_clock::now();
          if(kind==0)constructed=SplitEdge(seed,edge,midpoint,800000000,*scope,fresh,reason);
          if(kind==1)constructed=InsertEdgeCavity(seed,edge,midpoint,800000000,*scope,fresh,reason);
          if(kind==2 || kind==3)constructed=ReconstructSplitPatch(seed,edge,midpoint,{900000001,{}},
              field.Query(Centroid(seed.cells.front())).target,800000000,*scope,fresh,obstruction,reason);
          if(kind>=4 && kind<8) {
            auto edge_patch=patch_from(ring);scope->ExportParameters(edge_patch);
            const auto apices=SplitPatchApices(edge_patch,edge);REQUIRE_FALSE(apices.empty());
            std::vector<SurfaceFace> incident;
            for(const auto& f:edge_patch.surface)if(std::find(f.v.begin(),f.v.end(),edge[0])!=f.v.end() &&
                std::find(f.v.begin(),f.v.end(),edge[1])!=f.v.end())incident.push_back(f);
            REQUIRE(incident.size()==2);
            REQUIRE(state->Entity(incident[0].facet)==state->Entity(incident[1].facet));
            const auto p=nodes.at(incident[0].v[0]).p,q=nodes.at(incident[0].v[1]).p,t=nodes.at(incident[0].v[2]).p;
            const long double u[3]={(long double)q.x-p.x,(long double)q.y-p.y,(long double)q.z-p.z};
            const long double v[3]={(long double)t.x-p.x,(long double)t.y-p.y,(long double)t.z-p.z};
            std::array<long double,3> normal{{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]}};
            const auto norm=std::hypot(normal[0],normal[1],normal[2]);REQUIRE(norm>0);for(auto& x:normal)x/=norm;
            const double alpha=std::array<double,4>{{1.,.5,.25,.125}}[kind-4];
            std::vector<std::pair<Id,Node>> moves;
            for(Id apex:apices) {
              REQUIRE(expected_stars.count(apex));REQUIRE(subset(apex,apex).size()==expected_stars.at(apex));
              const auto origin=nodes.at(apex).p;const auto metric=field.Query(origin).target;
              const auto side=normal[0]*((long double)origin.x-p.x)+normal[1]*((long double)origin.y-p.y)+normal[2]*((long double)origin.z-p.z);
              const auto quadratic=metric.xx*normal[0]*normal[0]+metric.yy*normal[1]*normal[1]+metric.zz*normal[2]*normal[2]+
                  2*(metric.xy*normal[0]*normal[1]+metric.xz*normal[0]*normal[2]+metric.yz*normal[1]*normal[2]);
              REQUIRE(quadratic>0);REQUIRE(std::isfinite(quadratic));REQUIRE(side!=0);
              const auto step=alpha*std::copysign(.25L/std::sqrt(quadratic),side);
              Node proposed{900000001+moves.size(),{double(origin.x+step*normal[0]),double(origin.y+step*normal[1]),double(origin.z+step*normal[2])}};
              moves.emplace_back(apex,ConstrainMovePoint(seed,apex,proposed,{},*scope));
            }
            constructed=SplitSurfaceStrip(seed,edge,midpoint,moves,800000000,*scope,fresh,reason);
          }
          if(kind==8) {
            std::vector<std::pair<Id,Id>> reserved;for(Id apex:apices)reserved.emplace_back(apex,900000001+reserved.size());
            constructed=PlaceSplitPatch(seed,edge,midpoint,reserved,800000000,field,*scope,{},fresh,placement,reason,256);
          }
          log<<"{\"stage\":\"current_geometry_construction\",\"case\":"<<std::quoted(test.first)
             <<",\"kind\":"<<kind<<",\"edge\":["<<edge[0]<<','<<edge[1]<<"],\"active_cells\":"<<active.size()
             <<",\"constructed\":"<<constructed<<",\"reason\":"<<std::quoted(reason)
             <<",\"placement_trials\":"<<placement.trials<<",\"placement_moves\":"<<placement.moves
             <<",\"obstruction\":["<<obstruction[0]<<','<<obstruction[1]<<','<<obstruction[2]
             <<"],\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<"}\n";log.flush();
          if(distributed)REQUIRE(constructed);
          if(!constructed)continue;
          Owned local;std::set<Id> retired;for(const auto& row:active)retired.insert(row.first);
          std::map<Face,Id> physical;for(auto f:fresh.surface){std::sort(f.v.begin(),f.v.end());physical.emplace(f.v,f.facet);}
          for(const auto& c:fresh.cells){OwnedCell row;row.cell=c;
            for(unsigned slot=0;slot<4;++slot){Face key;for(unsigned k=0;k<3;++k)key[k]=c.v[opposite[slot][k]].id;
              std::sort(key.begin(),key.end());auto f=physical.find(key);if(f!=physical.end())row.physical[slot]={true,f->second};}
            REQUIRE(local.emplace(c.id,row).second);}
          if(distributed) {
            REQUIRE(kind==8);REQUIRE(constructed);REQUIRE(placement.moves>0);
            // Empty owners still participate in every lookup/reservation/publication collective.
            Owned published;if(!world.rank)published=active;
            Directory service(world);REQUIRE(service.Build(published,reason));const auto metadata_before=service.Snapshot();
            const std::vector<Feature> feature{{{{apices.front(),0,0}},1}};
            const auto dependencies=service.Lookup(feature);REQUIRE(dependencies.valid);REQUIRE(dependencies.cells.size()==active.size());
            auto stale=dependencies;if(!world.rank)++stale.cells.front().version;
            CHECK_FALSE(service.Fetch(stale,published).valid);CHECK(service.Snapshot()==metadata_before);
            const auto imported=service.Fetch(dependencies,published);REQUIRE(imported.valid);
            const bool chosen=service.Reserve(imported,true,reason);CHECK(chosen==(!world.rank));
            REQUIRE(world.sum(int(chosen))==1);
            std::vector<std::vector<Dependency>> remove_to(world.size);
            std::vector<std::vector<OwnedCell>> add_to(world.size);
            if(chosen) {
              auto current=scope->PrivateParameters(fresh);REQUIRE(current);current->ExportParameters(fresh);
              REQUIRE(state->CheckReplacement(seed,fresh,reason,ceiling,budget.caller_bytes,&budget));
              for(const auto& dep:dependencies.cells)remove_to[dep.owner].push_back(dep);
              for(const auto& row:local)add_to[row.first%world.size].push_back(row.second);
            }
            const auto removed=world.exchange(remove_to);const auto added=world.exchange(add_to);
            CHECK_FALSE(service.Publish(published,removed,added,world.rank!=0,reason));
            CHECK(service.Snapshot()==metadata_before);CHECK(world.sum(int(published.size()))==int(active.size()));
            REQUIRE(service.Publish(published,removed,added,true,reason));
            CHECK(world.sum(int(published.size()))==int(fresh.cells.size()));
            const auto inserted=service.Lookup({{{{midpoint.id,0,0}},1}});REQUIRE(inserted.valid);
            const auto returned=service.Fetch(inserted,published);REQUIRE(returned.valid);
            for(const auto& row:returned.cells)for(Node n:row.cell.v)if(n.id==midpoint.id) {
              bool matched=false;for(const auto& c:fresh.cells)for(Node expected:c.v)if(expected.id==n.id)
                matched=matched||(n.p.x==expected.p.x&&n.p.y==expected.p.y&&n.p.z==expected.p.z);
              CHECK(matched);
            }
            log<<"{\"stage\":\"current_placement_publication\",\"rank\":"<<world.rank
               <<",\"initially_empty\":"<<bool(world.rank)<<",\"winner\":"<<chosen
               <<",\"published_cells\":"<<published.size()<<",\"moves\":"<<placement.moves
               <<",\"engine_path_exercised\":false}\n";log.flush();
          }
          const std::string name="geometry_construction_"+std::to_string(kind);
          assess(local,retired,seed,fresh,midpoint.id,(name+"_raw").c_str(),false,placement.moves);
          assess(local,retired,seed,fresh,midpoint.id,(name+"_optimized").c_str());
        }
      }
      continue;
    }
    bool useful_star=false;
    if(metric_cleanup){std::set<Id> retired;for(const auto& row:owned)retired.insert(row.first);
      assess(owned,retired,complete,complete,test.second,"clean_context_optimizer");}
    if(!metric_cleanup)for(const auto& vertex:owned.at(test.second).cell.v) {
      auto local=subset(vertex.id,vertex.id);std::set<Id> retired;for(const auto& row:local)retired.insert(row.first);
      if(local.size()>MaximumCavityCells) {
        log<<"{\"stage\":\"existing_star_limit\",\"case\":"<<std::quoted(test.first)
           <<",\"vertex\":"<<vertex.id<<",\"cells\":"<<local.size()<<"}\n";log.flush();continue;
      }
      auto seed=patch_from(local);scope->ExportParameters(seed);
      useful_star=assess(local,retired,seed,seed,vertex.id,"original_star")||useful_star;
    }
    if(compound && !current_capture && test.second==224633219)CHECK(useful_star);
    if(current_capture && !compound) {
      REQUIRE(complete.cells.size()<=MaximumCavityCells);
      std::set<Id> retired;for(const auto& row:owned)retired.insert(row.first);
      const bool useful=assess(owned,retired,complete,complete,test.second,"production_complete_context");
      if(test.second==219735950)CHECK(useful);
    }
    bool useful_compound=false;
    if(compound)for(const auto& removed:owned.at(test.second).cell.v)for(const auto& retained:owned.at(test.second).cell.v) {
      if(removed.id==retained.id)continue;
      if(metric_cleanup){Edge edge{removed.id,retained.id};std::sort(edge.begin(),edge.end());
        const auto& chosen=construction_edges.at(test.first);if(std::find(chosen.begin(),chosen.end(),edge)==chosen.end())continue;}
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
      useful_compound=assess(prior,retired,seed,input,retained.id,"collapse_before_retained_star_optimizer",false)||useful_compound;
      useful_compound=assess(prior,retired,seed,input,retained.id,"collapse_then_retained_star_optimizer")||useful_compound;
    }
    // Actual wing-adjacent residual: area weighting vetoed every otherwise useful complete-context contraction.
    if(compound && current_capture && test.second==256755094)CHECK(useful_compound);
    if(metric_cleanup)continue;
    if(compound) {
      const auto& core=owned.at(test.second);std::vector<unsigned> faces;
      for(unsigned slot=0;slot<4;++slot)if(core.physical[slot].present)faces.push_back(slot);
      if(current_capture && !distributed) {
        // Complete core vertex stars also contain every dependency of their boundary edges.
        std::set<Edge> edges;
        for(unsigned slot:faces)for(unsigned k=0;k<3;++k) {
          Edge e{core.cell.v[opposite[slot][k]].id,core.cell.v[opposite[slot][(k+1)%3]].id};
          std::sort(e.begin(),e.end());edges.insert(e);
        }
        if(production_source) {
          std::set<Id> bad_cells;
          for(const auto& row:owned) {
            Owned one;one.insert(row);auto patch=patch_from(one);scope->ExportParameters(patch);
            const auto demand=GeometryGuidanceExcess(patch,*scope,1e-4);REQUIRE(demand.complete);
            if(demand.excess>0)bad_cells.insert(row.first);
          }
          std::set<Id> common;bool first=true;
          for(const auto& row:owned)if(bad_cells.count(row.first)) {
            std::set<Id> nodes;for(Node n:row.second.cell.v)nodes.insert(n.id);
            if(first){common=nodes;first=false;}
            else for(auto it=common.begin();it!=common.end();)if(!nodes.count(*it))it=common.erase(it);else ++it;
          }
          // A globally complete common vertex star proves every incident edge ring is present.
          for(const auto& row:owned)if(bad_cells.count(row.first))for(unsigned slot=0;slot<4;++slot)if(row.second.physical[slot].present)
            for(unsigned k=0;k<3;++k) {
              Edge edge{row.second.cell.v[opposite[slot][k]].id,row.second.cell.v[opposite[slot][(k+1)%3]].id};
              if((common.count(edge[0])&&expected_stars.count(edge[0]))||(common.count(edge[1])&&expected_stars.count(edge[1]))) {
                std::sort(edge.begin(),edge.end());edges.insert(edge);
              }
            }
        }
        REQUIRE(edges.size()<=6);
        for(const auto& edge:edges) {
          Owned ring;std::map<Id,Node> nodes;
          for(const auto& row:owned) {
            unsigned count=0;for(Node n:row.second.cell.v){nodes.emplace(n.id,n);count+=n.id==edge[0]||n.id==edge[1];}
            if(count==2)ring.insert(row);
          }
          REQUIRE_FALSE(ring.empty());REQUIRE(ring.size()<=MaximumCavityCells);
          auto seed=patch_from(ring);scope->ExportParameters(seed);
          const auto a=nodes.at(edge[0]).p,b=nodes.at(edge[1]).p;
          const Node midpoint{900000000,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
          CHECK_FALSE(PredictGeometrySplit(edge,{a,b},*original.original,1e-4).complete);
          const auto prediction=PredictGeometrySplit(edge,{a,b},*scope,1e-4);
          if(edge==Edge{18795,202090700}) {
            REQUIRE(prediction.complete);CHECK(prediction.after<prediction.before);
            auto changed=std::array<Point,2>{a,b};changed[0].x=std::nextafter(changed[0].x,INFINITY);
            const auto before_work=budget.work;
            CHECK_FALSE(PredictGeometrySplit(edge,changed,*scope,1e-4).complete);CHECK(budget.work==before_work);
            const auto limit=budget.maximum_bytes;budget.maximum_bytes=budget.caller_bytes;
            CHECK_THROWS_AS(PredictGeometrySplit(edge,{a,b},*scope,1e-4),std::length_error);budget.maximum_bytes=limit;
            budget.work_exhausted=true;
            CHECK_THROWS(PredictGeometrySplit(edge,{a,b},*scope,1e-4));CHECK(budget.work_exhausted);
            budget.work_exhausted=false;
          }
          Patch fresh;std::string why;const auto started=std::chrono::steady_clock::now();
          const bool constructed=SplitEdge(seed,edge,midpoint,800000000,*scope,fresh,why);
          log<<"{\"stage\":\"complete_boundary_edge_split\",\"case\":"<<std::quoted(test.first)
             <<",\"edge\":["<<edge[0]<<','<<edge[1]<<"],\"ring_cells\":"<<ring.size()
             <<",\"constructed\":"<<constructed<<",\"reason\":"<<std::quoted(why)
             <<",\"prediction_complete\":"<<prediction.complete<<",\"predicted_before\":"<<prediction.before
             <<",\"predicted_after\":"<<prediction.after<<",\"prediction_faces\":"<<prediction.evaluated_faces
             <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<<"}\n";log.flush();
          if(!constructed)continue;
          Owned joined=owned;for(const auto& row:ring)joined.erase(row.first);
          std::map<Face,Id> physical;for(auto f:fresh.surface){std::sort(f.v.begin(),f.v.end());physical.emplace(f.v,f.facet);}
          for(const auto& c:fresh.cells){OwnedCell row;row.cell=c;
            for(unsigned slot=0;slot<4;++slot){Face key;for(unsigned k=0;k<3;++k)key[k]=c.v[opposite[slot][k]].id;
              std::sort(key.begin(),key.end());auto f=physical.find(key);if(f!=physical.end())row.physical[slot]={true,f->second};}
            REQUIRE(joined.emplace(c.id,row).second);}
          REQUIRE(joined.size()<=MaximumReplacementCells);
          auto proposed=patch_from(joined);proposed.parameters=complete.parameters;proposed.coverage=complete.coverage;
          for(const auto& p:fresh.parameters)proposed.parameters.insert_or_assign(p.first,p.second);
          for(const auto& p:fresh.coverage.faces)proposed.coverage.faces.insert_or_assign(p.first,p.second);
          for(const auto& p:fresh.coverage.traces)proposed.coverage.traces.insert_or_assign(p.first,p.second);
          retain_coverage(proposed);std::set<Id> retired;for(const auto& row:owned)retired.insert(row.first);
          if(production_source) {
            Patch strip;REQUIRE(SplitSurfaceStrip(complete,edge,midpoint,{},800000000,*scope,strip,why));
            auto connectivity=[](const Patch& patch) {
              std::set<std::array<Id,4>> cells;for(const auto& cell:patch.cells) {
                std::array<Id,4> key;for(unsigned k=0;k<4;++k)key[k]=cell.v[k].id;
                std::sort(key.begin(),key.end());REQUIRE(cells.insert(key).second);
              }return cells;
            };
            REQUIRE(connectivity(strip)==connectivity(proposed));
            std::map<Id,Point> positions;for(const auto& cell:proposed.cells)for(Node n:cell.v)positions.emplace(n.id,n.p);
            for(const auto& cell:strip.cells)for(Node n:cell.v) {
              const auto p=positions.at(n.id);REQUIRE(n.p.x==p.x);REQUIRE(n.p.y==p.y);REQUIRE(n.p.z==p.z);
            }
            joined.clear();std::map<Face,Id> physical;
            for(auto f:strip.surface){std::sort(f.v.begin(),f.v.end());REQUIRE(physical.emplace(f.v,f.facet).second);}
            for(const auto& cell:strip.cells){OwnedCell row;row.cell=cell;
              for(unsigned slot=0;slot<4;++slot){Face key;for(unsigned k=0;k<3;++k)key[k]=cell.v[opposite[slot][k]].id;
                std::sort(key.begin(),key.end());const auto f=physical.find(key);if(f!=physical.end())row.physical[slot]={true,f->second};}
              REQUIRE(joined.emplace(cell.id,row).second);
            }
            proposed=std::move(strip);
          }
          const std::string label="complete_edge_split_"+std::to_string(edge[0])+"_"+std::to_string(edge[1]);
          bool reclosure=false;
          assess(joined,retired,complete,proposed,midpoint.id,label.c_str(),false,0,&prediction,&reclosure);
          if(reclosure) {
            const std::string combined=label+"_then_optimizer";
            assess(joined,retired,complete,proposed,midpoint.id,combined.c_str());
          }
        }
      }
      // Collapse/retained-star trials above apply to all cores; shelling requires two physical faces.
      if(faces.size()!=2)continue;
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
  if(production_source&&!metric_cleanup)CHECK(source_geometry_candidates>0);
  log.close();REQUIRE(bool(log));
}
TEST_CASE("Current M6 residual stars assess existing coupled repair and complete neighbors",
          "[.][NativeCurrentResidual3D]") {CapturedCurrentResidual(false);}
TEST_CASE("Current M6 boundary residuals assess existing collapse and retained-star optimization",
          "[.][NativeCurrentCompound3D]") {CapturedCurrentResidual(true);}

TEST_CASE("Actual current M6 candidates use production metric transport and explicit unknown oracles",
          "[.][NativeCurrentProduction3D]") {CapturedCurrentResidual(true,false,true);}

TEST_CASE("Actual clean M6 stars assess metric cleanup without artificial shape debt",
          "[.][NativeMetricCleanup3D]") {CapturedCurrentResidual(true,false,true,true);}

TEST_CASE("Actual M6 geometry placement publishes complete curved stars with empty owners",
          "[.][NativeCurrentPlacementMPI3D]") {CapturedCurrentResidual(true,true);}

TEST_CASE("Captured residual wing faces reproduce bounded certificates without a current volume gather",
          "[.][NativeGeometryResidualFaces3D]") {
  World world;REQUIRE(world.size==1);
  const auto* cfg=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(cfg);
  const std::string config(cfg);const auto slash=config.find_last_of('/');REQUIRE(slash!=std::string::npos);
  const auto directory=config.substr(0,slash+1);
  auto bytes=Captured202Bytes(directory+"m6.su2.native_ref");
  auto original=DecodeReference(std::string(bytes.begin(),bytes.end()));
  REQUIRE(original.original);REQUIRE(original.original->Parameters());
  auto cells=Captured202Rows<OwnedCell>(directory+"geometry_faces_owned.bin",416);
  auto rows=Captured202Rows<BoundaryPoint>(directory+"witness_boundary_points.bin",72);
  REQUIRE(cells.size()==78);
  BoundaryPoints points;for(auto p:rows)REQUIRE(points.emplace(p.node.id,p).second);
  std::map<Id,double> expected;Id id;double excess;
  std::ifstream input(directory+"geometry_faces_expected.tsv");REQUIRE(bool(input));
  while(input>>id>>excess){REQUIRE(std::isfinite(excess));REQUIRE(excess>0);REQUIRE(expected.emplace(id,excess).second);}
  REQUIRE(expected.size()==cells.size());
  std::set<Id> probes;std::ifstream selected(directory+"geometry_faces_probe_ids.txt");REQUIRE(bool(selected));
  while(selected>>id){REQUIRE(expected.count(id));REQUIRE(probes.insert(id).second);}REQUIRE(probes.size()==8);
  const size_t ceiling=size_t(2)<<30;
  SU2NativeGeometry::AtlasBudget budget;budget.maximum_bytes=ceiling;
  budget.maximum_work=transfer_memory::Mul(original.facets.size(),SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  budget.maximum_integer_bits=size_t(1)<<20;
  budget.caller_bytes=transfer_memory::Add(original.original->ResidentBytes(),transfer_memory::Bytes(cells),
      transfer_memory::Bytes(rows),size_t(64)<<20);
  const auto* initial=original.original->Parameters();std::map<int,int> roles{{0,2},{1,0},{2,2}};
  auto mobile=initial->Mobility().identity?std::shared_ptr<BoundaryParameterState>{}:
      initial->WithMobilityPolicy(60,roles,budget,ceiling,budget.caller_bytes);
  const auto* active=mobile?mobile.get():initial;
  REQUIRE(active->Mobility().angle==60);REQUIRE(active->Mobility().roles==roles);
  auto entity=active->WithEntityGeometry(1e-4,budget,ceiling,budget.caller_bytes);
  auto reference=original.original->WithParameterState(entity);
  budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,entity->ResidentBytes(),reference->ResidentBytes());
  REQUIRE(budget.caller_bytes<ceiling);
  auto scope=reference->WithAtlasReservation(budget);REQUIRE(scope);
  REQUIRE_FALSE(bool(std::ifstream("geometry_faces_result.jsonl")));
  std::ofstream log("geometry_faces_result.jsonl");REQUIRE(bool(log));log<<std::setprecision(17)<<std::boolalpha;
  const unsigned opposite[4][3]={{1,2,3},{0,3,2},{0,1,3},{0,2,1}};
  for(const auto& row:cells) {
    Patch patch;patch.cells.push_back(row.cell);std::array<Point,3> xyz{};SurfaceFace face;unsigned count=0;
    for(unsigned slot=0;slot<4;++slot)if(row.physical[slot].present) {
      ++count;face.facet=row.physical[slot].facet;
      for(unsigned k=0;k<3;++k){const auto n=row.cell.v[opposite[slot][k]];face.v[k]=n.id;xyz[k]=n.p;
        const auto p=points.at(n.id);REQUIRE(p.node.p.x==n.p.x);REQUIRE(p.node.p.y==n.p.y);REQUIRE(p.node.p.z==n.p.z);
        patch.parameters.emplace(n.id,p);}
    }
    REQUIRE(count==1);REQUIRE(reference->Get(face.facet).marker==2);patch.surface.push_back(face);
    const auto work=budget.work;const auto stats0=entity->Geometry().GeometryStatsSnapshot();
    try {
      // This overlay reconstructs face coverage only; it cannot admit a volume replacement.
      auto context=scope->PrivateParameters(patch);REQUIRE(context);context->ExportParameters(patch);
      const bool contained=context->ContainsFace(face.facet,xyz);CHECK(contained);
      if(!contained){log<<"{\"stage\":\"baseline\",\"cell\":"<<row.cell.id<<",\"contained\":false}\n";continue;}
      const double bound=context->SurfaceDeviation(face.facet,xyz,true),actual=std::max(0.,bound-1e-4);
      REQUIRE(std::isfinite(bound));CHECK(std::abs(actual-expected.at(row.cell.id))<=1e-10);
      const auto stats1=entity->Geometry().GeometryStatsSnapshot();
      log<<"{\"stage\":\"baseline\",\"cell\":"<<row.cell.id<<",\"facet\":"<<face.facet
         <<",\"contained\":true,\"bound\":"<<bound<<",\"excess\":"<<actual<<",\"captured_excess\":"<<expected.at(row.cell.id)
         <<",\"work\":"<<budget.work-work<<",\"upper_work\":"<<stats1.upper_work-stats0.upper_work<<"}\n";log.flush();
      if(!probes.count(row.cell.id))continue;
      for(unsigned edge=0;edge<3;++edge)try {
        auto trial=scope->PrivateParameters(patch);REQUIRE(trial);
        const unsigned a=edge,b=(edge+1)%3;const Edge endpoints{face.v[a],face.v[b]};
        Node midpoint{900000000+row.cell.id*3+edge,{double(((long double)xyz[a].x+xyz[b].x)/2),
            double(((long double)xyz[a].y+xyz[b].y)/2),double(((long double)xyz[a].z+xyz[b].z)/2)}};
        REQUIRE_FALSE(points.count(midpoint.id));
        midpoint=trial->BindNewPoint(midpoint,{face.facet},&endpoints);
        auto left=xyz,right=xyz;left[a]=midpoint.p;right[b]=midpoint.p;
        const bool valid=trial->ContainsFace(face.facet,left)&&trial->ContainsFace(face.facet,right);
        const long double dx=(long double)xyz[a].x-xyz[b].x,dy=(long double)xyz[a].y-xyz[b].y,dz=(long double)xyz[a].z-xyz[b].z;
        std::ostringstream entry;entry<<std::setprecision(17)<<std::boolalpha;
        entry<<"{\"stage\":\"face_midpoint_probe\",\"cell\":"<<row.cell.id<<",\"edge\":"<<edge
           <<",\"contains_children\":"<<valid<<",\"length_squared\":"<<dx*dx+dy*dy+dz*dz;
        if(valid){const double l=trial->SurfaceDeviation(face.facet,left,true),r=trial->SurfaceDeviation(face.facet,right,true);
          REQUIRE(std::isfinite(l));REQUIRE(std::isfinite(r));
          const long double dl=std::max(0.L,(long double)l-1e-4),dr=std::max(0.L,(long double)r-1e-4);
          entry<<",\"child_maximum_bound\":"<<std::max(l,r)<<",\"child_squared_excess\":"<<dl*dl+dr*dr;}
        entry<<",\"scope\":\"single_face_only_not_complete_edge_star_or_candidate_admission\"}\n";log<<entry.str();log.flush();
      }catch(const std::exception& e){log<<"{\"stage\":\"probe_unresolved\",\"cell\":"<<row.cell.id
          <<",\"edge\":"<<edge<<",\"reason\":"<<std::quoted(e.what())<<"}\n";log.flush();}
    }catch(const std::exception& e){log<<"{\"stage\":\"baseline_unresolved\",\"cell\":"<<row.cell.id
        <<",\"reason\":"<<std::quoted(e.what())<<"}\n";log.flush();CHECK(false);}
  }
  log.close();REQUIRE(bool(log));CHECK(budget.work<=budget.maximum_work);CHECK(budget.peak_bytes<=ceiling);
}
