// Explicitly selected actual-state feasibility diagnostic; no production dispatch or historical replay claim.
namespace {
std::vector<char> Captured202Bytes(const std::string& path) {
  std::ifstream file(path,std::ios::binary|std::ios::ate);REQUIRE(bool(file));
  const auto size=file.tellg();REQUIRE(size>=0);REQUIRE(uint64_t(size)<=(size_t(128)<<20));
  std::vector<char> bytes(static_cast<size_t>(size));
  file.seekg(0);file.read(bytes.data(),bytes.size());REQUIRE(bool(file));return bytes;
}
template<class T>std::vector<T> Captured202Rows(const std::string& path,size_t record_bytes) {
  const auto bytes=Captured202Bytes(path);RecordStream input(bytes);uint64_t count=0;input(count);
  REQUIRE(count<=(size_t(128)<<20)/record_bytes);REQUIRE(bytes.size()==8+count*record_bytes);
  std::vector<T> rows(count);for(auto& row:rows)input(row);REQUIRE(input.End());return rows;
}
struct Captured202Edge {
  Edge vertices{};double length=0;
  template<class S>void Fields(S& s){s(vertices,length);}
};
struct Captured202Metric {
  Id id=0;double q=0,J=0;std::array<Captured202Edge,6> edges{};
  template<class S>void Fields(S& s){s(id,q,J,edges);}
};
}

TEST_CASE("Captured M6 feature-star tests existing collapse against reconstructed current authority",
          "[.][NativeCapturedCavity3D]") {
  World world;REQUIRE(world.size==1);
  const auto* cfg=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(cfg);
  const std::string config_path(cfg);const auto slash=config_path.find_last_of('/');REQUIRE(slash!=std::string::npos);
  const auto directory=config_path.substr(0,slash+1);
  REQUIRE_FALSE(bool(std::ifstream("native_captured_cavity_result.jsonl")));
  std::ofstream log("native_captured_cavity_result.jsonl");REQUIRE(bool(log));log<<std::setprecision(17)<<std::boolalpha;
  auto reference_bytes=Captured202Bytes(directory+"m6.su2.native_ref");
  auto original=DecodeReference(std::string(reference_bytes.begin(),reference_bytes.end()));
  REQUIRE((original.marker_names==std::vector<std::string>{"FARFIELD","SYMMETRY","WING3D"}));
  REQUIRE(original.original);REQUIRE(original.original->Parameters());
  auto point_rows=Captured202Rows<BoundaryPoint>(directory+"witness_boundary_points.bin",72);
  auto surface=Captured202Rows<SurfaceFace>(directory+"witness_boundary_surface.bin",32);
  auto cells=Captured202Rows<OwnedCell>(directory+"witness_143424775_owned.bin",416);
  auto expected=Captured202Rows<Captured202Metric>(directory+"witness_expected_metrics.bin",168);
  REQUIRE(point_rows.size()==38752);REQUIRE(surface.size()==77500);REQUIRE(cells.size()==52);REQUIRE(expected.size()==52);
  BoundaryPoints points;for(const auto& p:point_rows)REQUIRE(points.emplace(p.node.id,p).second);
  Owned owned;std::vector<Cell> regions;for(const auto& row:cells){REQUIRE(owned.emplace(row.cell.id,row).second);regions.push_back(row.cell);}
  log<<"{\"stage\":\"loaded\",\"points\":"<<points.size()<<",\"surface\":"<<surface.size()<<",\"cells\":"<<cells.size()<<"}\n";log.flush();

  const size_t ceiling=size_t(12)<<30;
  SU2NativeGeometry::AtlasBudget budget;budget.maximum_bytes=ceiling;
  // Same bounded per-face reservation as production mobility setup, over this complete reconstruction scope.
  budget.maximum_work=transfer_memory::Mul(transfer_memory::Add(original.facets.size(),surface.size()),
      SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  budget.maximum_integer_bits=size_t(1)<<20;
  budget.caller_bytes=transfer_memory::Add(original.original->ResidentBytes(),
      transfer_memory::Mul(points.size(),sizeof(BoundaryPoint)+8*sizeof(void*)),
      transfer_memory::Bytes(point_rows),transfer_memory::Bytes(surface),transfer_memory::Bytes(cells));
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

  CavityMetricSource source(world,owned);const auto identity=std::make_shared<const FrozenMetricIdentity>();
  auto field=[&](const std::vector<Cell>& query,const std::vector<OwnedCell>& cavity) {
    auto imported=source.ImportCellsLocal(query,cavity,ceiling,budget.caller_bytes);INFO(imported.reason);REQUIRE(imported.valid);
    const auto extensions=AssociatedExtensions(imported.cells,*reference);
    return std::make_unique<FrozenField>(std::move(imported.cells),MetricComposition{},false,extensions,
        MetricEdgeBreakpoints{},identity,true,MaximumDonors,true);
  };
  struct RestoreShape {ShapeAdmission value=GetShapeAdmission();~RestoreShape(){SetShapeAdmission(value);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  auto full_field=field(regions,cells);double q_error=0,j_error=0,edge_error=0;
  for(const auto& prior:expected) {
    const auto measured=MeasureMetricCells({owned.at(prior.id).cell},*full_field);
    q_error=std::max(q_error,std::abs(measured.minimum_mean_ratio-prior.q));
    j_error=std::max(j_error,std::abs(measured.minimum_scaled_jacobian-prior.J));
    for(const auto& edge:prior.edges)edge_error=std::max(edge_error,double(std::abs(measured.edges.at(edge.vertices).length-edge.length)));
  }
  log<<"{\"stage\":\"old_metric_comparison\",\"cells\":52,\"maximum_q_absolute_error\":"<<q_error
     <<",\"maximum_J_absolute_error\":"<<j_error<<",\"maximum_edge_absolute_error\":"<<edge_error<<"}\n";log.flush();
  CHECK(q_error<=1e-10);CHECK(j_error<=1e-10);CHECK(edge_error<=1e-10);

  const std::array<Edge,4> trials{{{4502,181199364},{4502,174173910},{181199364,4502},{4502,174150582}}};
  const std::array<std::array<unsigned,3>,4> opposite{{{{1,2,3}},{{0,3,2}},{{0,1,3}},{{0,2,1}}}};
  for(const auto& direction:trials) {
    std::vector<OwnedCell> cavity;Patch old,fresh;
    for(const auto& row:cells)if(std::any_of(row.cell.v.begin(),row.cell.v.end(),[&](Node n){return n.id==direction[0];})) {
      cavity.push_back(row);old.cells.push_back(row.cell);
      for(unsigned slot=0;slot<4;++slot)if(row.physical[slot].present){Face face;
        for(unsigned k=0;k<3;++k)face[k]=row.cell.v[opposite[slot][k]].id;
        old.surface.push_back({face,row.physical[slot].facet});}
    }
    REQUIRE(cavity.size()==(direction[0]==4502?12:16));
    auto scope=reference->WithAtlasReservation(budget);scope->ExportParameters(old);
    auto old_field=field(old.cells,cavity);const auto before=MeasureMetricCells(old.cells,*old_field);
    reason.clear();const bool compatible=scope->ContractionAllowed(direction[0],direction[1]);
    bool constructed=CollapseVertex(old,direction[0],direction[1],200000000,*scope,fresh,reason);
    const auto construction_reason=reason;bool replacement=false,metric_valid=false,guidance_valid=false;
    std::string replacement_reason,metric_reason,guidance_reason;PatchMetric after;MetricDiagnosis diagnosis;
    long double old_geometry=0,new_geometry=0;
    if(constructed)replacement=state->CheckReplacement(old,fresh,replacement_reason,ceiling,budget.caller_bytes,&budget);
    if(constructed&&replacement) {
      auto fresh_field=field(fresh.cells,cavity);after=MeasureMetricCells(fresh.cells,*fresh_field);
      metric_valid=ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),after,MetricChange::Repair,metric_reason,&diagnosis);
      if(metric_valid)try {
        const auto was=GeometryGuidanceExcess(old,*scope,1e-4);old_geometry=was.excess;
        const auto now=GeometryGuidanceExcess(fresh,*scope,1e-4,old_geometry);new_geometry=now.excess;
        guidance_valid=was.complete&&now.complete&&std::isfinite(new_geometry)&&new_geometry<=old_geometry;
      }catch(const std::exception& e){guidance_reason=e.what();}
    }
    log<<"{\"stage\":\"directed_collapse\",\"removed\":"<<direction[0]<<",\"retained\":"<<direction[1]
       <<",\"compatible\":"<<compatible<<",\"constructed\":"<<constructed<<",\"construction_reason\":"<<std::quoted(construction_reason)
       <<",\"replacement\":"<<replacement<<",\"replacement_reason\":"<<std::quoted(replacement_reason)
       <<",\"metric_valid\":"<<metric_valid<<",\"metric_reason\":"<<std::quoted(metric_reason)
       <<",\"guidance_valid\":"<<guidance_valid<<",\"guidance_reason\":"<<std::quoted(guidance_reason)
       <<",\"old_cells\":"<<old.cells.size()<<",\"new_cells\":"<<fresh.cells.size()
       <<",\"old_q_bad\":"<<before.q_bad_cells<<",\"old_J_bad\":"<<before.j_bad_cells
       <<",\"new_q_bad\":"<<after.q_bad_cells<<",\"new_J_bad\":"<<after.j_bad_cells
       <<",\"old_shape_debt\":"<<double(before.shape_debt)<<",\"new_shape_debt\":"<<double(after.shape_debt)
       <<",\"old_geometry_guidance\":"<<double(old_geometry)<<",\"new_geometry_guidance\":"<<double(new_geometry)
       <<",\"work\":"<<budget.work<<",\"global_contact_and_publication_checked\":false,\"accepted_repair\":false}\n";log.flush();
    CHECK_FALSE(budget.work_exhausted);CHECK(budget.geometry_failure.empty());
  }
  log.close();REQUIRE(bool(log));
}
