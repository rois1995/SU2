#include <chrono>
#include "../../../../Common/include/adaptation/CNativeOptimize3D.hpp"
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

TEST_CASE("Captured M6 current nodal cells recover independent centroid shape diagnostics",
          "[.][NativeCapturedShapeCensus3D]") {
  World world;REQUIRE(world.size==1);
  const auto* cfg=std::getenv("SU2_NATIVE_FROZEN_CONFIG");REQUIRE(cfg);
  const std::string config(cfg);const auto slash=config.find_last_of('/');REQUIRE(slash!=std::string::npos);
  const auto folder=config.substr(0,slash+1);
  REQUIRE_FALSE(bool(std::ifstream("captured_shape_census.tsv")));
  std::ofstream output("captured_shape_census.tsv");REQUIRE(bool(output));
  output<<std::setprecision(17)<<"rank\tid\tversion\tq\tJ\n";
  size_t total=0,q_bad=0,j_bad=0;
  std::vector<DonorCell> donor(1);
  for(unsigned rank=0;rank<6;++rank) {
    const auto rows=Captured202Rows<OwnedCell>(folder+"adapted_mesh_inspection_rank"+
        std::to_string(rank)+"_owned.bin",416);
    for(const auto& row:rows) {
      REQUIRE(row.sensor_present);
      donor[0].cell=row.cell;donor[0].sensor=row.sensor;
      const auto target=EvaluateCavitySample(donor,Centroid(row.cell)).sensor;
      const auto quality=Measure({row.cell.v[0].p,row.cell.v[1].p,row.cell.v[2].p,row.cell.v[3].p},target);
      REQUIRE(std::isfinite(quality.mean_ratio));REQUIRE(std::isfinite(quality.minimum_scaled_jacobian));
      q_bad+=quality.mean_ratio<MinimumMeanRatio;j_bad+=quality.minimum_scaled_jacobian<MinimumScaledJacobian;
      output<<rank<<'\t'<<row.cell.id<<'\t'<<row.version<<'\t'<<quality.mean_ratio<<'\t'
            <<quality.minimum_scaled_jacobian<<'\n';
    }
    total+=rows.size();output.flush();REQUIRE(bool(output));
  }
  // Each sealed shard is consumed to its declared count and exact end, independent of mesh size.
  REQUIRE(total>0);
  INFO("Diagnostic only: no geometry, edge, transfer or final acceptance certification");
  std::cout<<"CAPTURED_SHAPE_DIAG cells="<<total<<" q_bad="<<q_bad<<" J_bad="<<j_bad<<'\n';
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

  // Saved single-face census excesses; a correspondence bound is not a distance violation proof.
  struct GeometryProbe {Id cell,facet;Face vertices;double excess;};
  const std::array<GeometryProbe,4> geometry_probes{{
    {152256924,10699,{8075,174384024,191931538},2.3465772417086435e-5},
    {151610623,16737,{7849,191559586,7844},0.00030667155886447245},
    {152331796,16677,{8370,173784300,7322},0.0004730775988526313},
    {152256925,10699,{191931538,174384024,181694598},0.}
  }};
  {
    const auto& authority=state->Geometry();const auto retained=authority.GeometryStatsSnapshot();
    const auto cache_bytes=transfer_memory::Add(retained.prepared_limit,retained.memo_limit);
    const auto caller=transfer_memory::Add(budget.caller_bytes,state->ResidentBytes(),reference->ResidentBytes(),cache_bytes);
    REQUIRE(transfer_memory::Add(caller,cache_bytes)<ceiling);
    auto immutable=std::shared_ptr<const PlanarReference>(state->Authority(),&authority.Reference());
    SU2NativeGeometry::MappedFacetReference recomputed(immutable,authority.Errors(),authority.Version(),
        ceiling-caller-cache_bytes,authority.ReferenceComponents(),authority.Curves(),cache_bytes);
    const auto query_caller=transfer_memory::Add(caller,recomputed.ResidentBytes(),cache_bytes);
    REQUIRE(query_caller<ceiling);
    for(const auto& probe:geometry_probes) {
      auto key=probe.vertices;std::sort(key.begin(),key.end());
      const auto found=std::find_if(surface.begin(),surface.end(),[&](const SurfaceFace& f){
        auto vertices=f.v;std::sort(vertices.begin(),vertices.end());return f.facet==probe.facet&&vertices==key;
      });
      REQUIRE(found!=surface.end());std::array<Point,3> physical;std::array<SU2NativeGeometry::SurfaceBinding,3> bindings;
      for(unsigned k=0;k<3;++k){const auto& p=points.at(found->v[k]);physical[k]=p.node.p;bindings[k]=p.binding;
        REQUIRE(authority.Supports(probe.facet,p.binding));}
      const double direct=authority.SurfaceError(probe.facet,bindings,physical);
      const double production=reference->SurfaceDeviation(probe.facet, physical, SU2NativeGeometry::SurfaceQueryNeed::Magnitude());
      CHECK(direct==production);CHECK(std::abs(std::max(0.,production-1e-4)-probe.excess)<=1e-12);
      auto query=budget;query.work=0;query.maximum_work=64*SU2NativeGeometry::MappedFacetReference::ClearanceWork;
      query.caller_bytes=query_caller;query.work_exhausted=false;query.geometry_failure.clear();
      auto fresh_budget=query;const auto before=authority.GeometryStatsSnapshot();
      {
        using namespace SU2NativeGeometry;
        auto partition_budget=query;partition_budget.maximum_integer_bits=4096;
        // Admit both coverage generations, traces and bounded exact-coordinate temporaries before allocation.
        const auto scratch=size_t(8)<<20;
        REQUIRE(transfer_memory::Add(partition_budget.caller_bytes,scratch)<partition_budget.maximum_bytes);
        partition_budget.caller_bytes+=scratch;
        FaceCoverage partition;std::string why;
        REQUIRE(authority.ResolveSingleChart(probe.facet,bindings,partition,partition_budget,why));
        REQUIRE(partition.pieces.size()==1);
        std::array<EdgeTrace,3> original_traces;
        REQUIRE(authority.CoverageTraces(partition,original_traces,partition_budget,why));
        ExactRatio half;REQUIRE(ExactBinary64(.5,half,partition_budget,why));
        double previous=direct;
        for(unsigned depth=0;depth<=2;++depth) {
          const auto work=partition_budget.work;const auto started=std::chrono::steady_clock::now();
          const double upper=authority.SurfaceError(partition,physical,partition_budget);
          const auto bound_work=partition_budget.work-work;
          const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
          auto repeated_budget=partition_budget;repeated_budget.work=0;
          CHECK(upper==authority.SurfaceError(partition,physical,repeated_budget));
          CHECK(upper==recomputed.SurfaceError(partition,physical,repeated_budget));
          CHECK(std::isfinite(upper));CHECK(upper<=previous+1e-12);
          if(depth==0)CHECK(std::abs(upper-direct)<=1e-12);
          log<<"{\"stage\":\"geometry_partition_probe\",\"cell\":"<<probe.cell<<",\"depth\":"<<depth
             <<",\"pieces\":"<<partition.pieces.size()<<",\"upper\":"<<upper<<",\"bound_work\":"<<bound_work
             <<",\"preparation_work\":"<<work<<",\"seconds\":"<<seconds
             <<",\"directed_within\":"<<(upper<=1e-4)<<"}\n";log.flush();
          budget.work=transfer_memory::Add(budget.work,repeated_budget.work);
          previous=upper;if(depth==2)break;
          auto midpoint=[&](const ExactBary& a,const ExactBary& b) {
            ExactBary out;
            REQUIRE(ExactAdd(a.u,b.u,out.u,partition_budget,why));
            REQUIRE(ExactMultiply(out.u,half,out.u,partition_budget,why));
            REQUIRE(ExactAdd(a.v,b.v,out.v,partition_budget,why));
            REQUIRE(ExactMultiply(out.v,half,out.v,partition_budget,why));return out;
          };
          FaceCoverage divided;divided.pieces.reserve(4*partition.pieces.size());
          const std::array<std::array<unsigned,3>,4> children{{{{0,3,5}},{{3,1,4}},{{5,4,2}},{{3,4,5}}}};
          for(const auto& piece:partition.pieces) {
            auto split=[&](const std::array<ExactBary,3>& p) {
              return std::array<ExactBary,6>{{p[0],p[1],p[2],midpoint(p[0],p[1]),midpoint(p[1],p[2]),midpoint(p[2],p[0])}};
            };
            const auto reference_points=split(piece.reference),active_points=split(piece.active);
            for(const auto& child:children){CoveragePiece next;next.facet=piece.facet;
              for(unsigned k=0;k<3;++k){next.reference[k]=reference_points[child[k]];next.active[k]=active_points[child[k]];}
              divided.pieces.push_back(std::move(next));}
          }
          partition=std::move(divided);
          std::array<EdgeTrace,3> traces;
          bool verified=authority.CoverageTraces(partition,traces,partition_budget,why);
          for(unsigned k=0;k<3&&verified;++k)verified=authority.EqualTraces(original_traces[k],traces[k],partition_budget,why);
          if(!verified) {
            log<<"{\"stage\":\"geometry_partition_refusal\",\"cell\":"<<probe.cell<<",\"depth\":"<<depth+1
               <<",\"work\":"<<partition_budget.work<<",\"work_exhausted\":"<<partition_budget.work_exhausted
               <<",\"reason\":"<<std::quoted(why)<<"}\n";log.flush();
            INFO(why);REQUIRE(partition_budget.work_exhausted);break;
          }
        }
        auto exhausted=partition_budget;exhausted.maximum_work=exhausted.work=0;
        CHECK_THROWS(authority.SurfaceError(partition,physical,exhausted));CHECK(exhausted.work_exhausted);
        auto tight=partition_budget;tight.maximum_bytes=tight.caller_bytes;
        CHECK_THROWS_AS(authority.SurfaceError(partition,physical,tight),std::length_error);
        budget.work=transfer_memory::Add(budget.work,partition_budget.work);REQUIRE(budget.work<=budget.maximum_work);
      }
      log<<"{\"stage\":\"geometry_probe_start\",\"cell\":"<<probe.cell<<",\"facet\":"<<probe.facet
         <<",\"direct_upper\":"<<direct<<",\"recorded_excess\":"<<probe.excess
         <<",\"query_work_limit\":"<<query.maximum_work<<"}\n";log.flush();
      try {
        const std::vector<Id> support{probe.facet};const std::vector<std::array<Point,3>> triangles{physical};
        const double bound=authority.BoundPhysicalSurface(support,triangles,1e-4,query);const auto first_work=query.work;
        const double cached=authority.BoundPhysicalSurface(support,triangles,1e-4,query);
        const double fresh=recomputed.BoundPhysicalSurface(support,triangles,1e-4,fresh_budget);
        REQUIRE(std::isfinite(bound));CHECK(bound==cached);CHECK(bound==fresh);
        const auto after=authority.GeometryStatsSnapshot();CHECK(after.memo_union_hits>before.memo_union_hits);
        log<<"{\"stage\":\"geometry_probe_result\",\"cell\":"<<probe.cell<<",\"query_completed\":true"
           <<",\"union_upper\":"<<bound<<",\"cached_upper\":"<<cached<<",\"recomputed_upper\":"<<fresh
           <<",\"directed_within\":"<<(bound<=1e-4)<<",\"first_work\":"<<first_work
           <<",\"cached_work\":"<<query.work-first_work<<",\"recomputed_work\":"<<fresh_budget.work
           <<",\"traversal_work\":"<<after.traversal_work-before.traversal_work
           <<",\"scope_build_work\":"<<after.scope_build_work-before.scope_build_work<<"}\n";
        auto scoped=reference->WithAtlasReservation(query);
        const auto scoped_before=authority.GeometryStatsSnapshot();
        double expected=direct<=1e-4?direct:std::min(direct,bound);
        if(expected>1e-4)expected=std::min(expected,authority.BoundPhysicalSurface(support,triangles,1e-4,query,
            SU2NativeGeometry::MappedFacetReference::SurfaceBoundPurpose::ErrorMagnitude));
        const auto affine=reference->Affine().find(probe.facet);
        if(affine!=reference->Affine().end())for(auto point:physical)
          expected=std::max(expected,SU2NativeGeometry::AffineChart(affine->second).ErrorBound(point));
        const auto measured=scoped->SurfaceDeviation(probe.facet, physical, SU2NativeGeometry::SurfaceQueryNeed::Magnitude());
        CHECK(measured==Approx(expected).margin(1e-14));CHECK((measured<=1e-4)==(expected<=1e-4));
        const auto scoped_after=authority.GeometryStatsSnapshot();
        if(direct<=1e-4)CHECK(scoped_after.upper_calls==scoped_before.upper_calls);
        if(probe.cell==152256924) {
          CHECK(scoped->SurfaceDeviation(probe.facet, physical, SU2NativeGeometry::SurfaceQueryNeed::Classify())<=1e-4);
          const auto saved_caller=query.caller_bytes,saved_ceiling=query.maximum_bytes;
          auto exhausted=query;auto exhausted_scope=reference->WithAtlasReservation(exhausted);
          exhausted.work=0;exhausted.maximum_work=0;
          CHECK_THROWS(exhausted_scope->SurfaceDeviation(probe.facet, physical, SU2NativeGeometry::SurfaceQueryNeed::Magnitude()));CHECK(exhausted.work_exhausted);
          CHECK(exhausted.caller_bytes==saved_caller);CHECK(exhausted.maximum_bytes==saved_ceiling);
          auto tight=query;auto tight_scope=reference->WithAtlasReservation(tight);tight.maximum_bytes=tight.caller_bytes;
          CHECK_THROWS_AS(tight_scope->SurfaceDeviation(probe.facet, physical, SU2NativeGeometry::SurfaceQueryNeed::Magnitude()),std::length_error);
          CHECK(tight.caller_bytes==saved_caller);CHECK(tight.maximum_bytes==saved_caller);
          CHECK(reference->SurfaceDeviation(probe.facet, physical, SU2NativeGeometry::SurfaceQueryNeed::Magnitude())==direct);
        }
      }catch(const std::exception& error) {
        WARN(error.what());
        log<<"{\"stage\":\"geometry_probe_result\",\"cell\":"<<probe.cell<<",\"query_completed\":false"
           <<",\"work_exhausted\":"<<(query.work_exhausted||fresh_budget.work_exhausted)
           <<",\"query_work\":"<<query.work<<",\"recomputed_work\":"<<fresh_budget.work<<"}\n";
      }
      budget.work=transfer_memory::Add(budget.work,query.work,fresh_budget.work);REQUIRE(budget.work<=budget.maximum_work);
      log.flush();
    }
  }

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
  Patch full_patch;full_patch.cells=regions;
  for(const auto& row:cells)for(unsigned slot=0;slot<4;++slot)if(row.physical[slot].present) {
    Face face;for(unsigned k=0;k<3;++k)face[k]=row.cell.v[opposite[slot][k]].id;
    full_patch.surface.push_back({face,row.physical[slot].facet});
  }
  reference->WithAtlasReservation(budget)->ExportParameters(full_patch);
  auto assess_optimizer=[&](const Patch& seed,const char* label,const std::set<Id>& frozen=std::set<Id>{}) {
    struct RestoreCaller {SU2NativeGeometry::AtlasBudget& b;size_t caller;~RestoreCaller(){b.caller_bytes=caller;}} restore{budget,budget.caller_bytes};
    budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,state->ResidentBytes(),reference->ResidentBytes(),
        2*MaximumFrozenFieldBytes,size_t(64)<<20);
    REQUIRE(budget.caller_bytes<ceiling);
    auto scope=reference->WithAtlasReservation(budget);auto context=scope->PrivateParameters(seed);REQUIRE(context);
    Owned local;std::map<Face,Id> physical;
    for(const auto& f:seed.surface){auto key=f.v;std::sort(key.begin(),key.end());physical.emplace(key,f.facet);}
    for(const auto& cell:seed.cells){OwnedCell row;row.cell=cell;
      for(unsigned slot=0;slot<4;++slot){Face face;for(unsigned k=0;k<3;++k)face[k]=cell.v[opposite[slot][k]].id;
        std::sort(face.begin(),face.end());const auto found=physical.find(face);
        if(found!=physical.end())row.physical[slot]={true,found->second};}
      REQUIRE(local.emplace(cell.id,std::move(row)).second);}
    OptimizeOptions options;options.maximum_work=160;options.maximum_seconds=30;
    options.cache_entries=8*MaximumReplacementCells;options.boundary=false;options.geometry=context.get();options.surface_tolerance=1e-4;
    options.frozen_cells=frozen;
    Id next_node=300000000,next_cell=400000000;OptimizeReport report;
    const bool changed=OptimizeMetricMesh(local,RegionMetric(*full_field),next_node,next_cell,options,report);
    CHECK(report.trials<=160);CHECK(local.size()<=MaximumReplacementCells);
    Patch result=seed;result.cells.clear();result.surface.clear();
    for(const auto& row:local){const auto& cell=row.second.cell;result.cells.push_back(cell);
      for(unsigned slot=0;slot<4;++slot)if(row.second.physical[slot].present){Face face;
        for(unsigned k=0;k<3;++k)face[k]=cell.v[opposite[slot][k]].id;
        result.surface.push_back({face,row.second.physical[slot].facet});}}
    for(const auto& p:report.parameters)result.parameters.insert_or_assign(p.first,p.second);
    for(const auto& p:report.coverage.faces)result.coverage.faces.insert_or_assign(p.first,p.second);
    for(const auto& p:report.coverage.traces)result.coverage.traces.insert_or_assign(p.first,p.second);
    std::set<Face> remaining_faces;std::set<Edge> remaining_edges;
    for(auto face:result.surface){std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());remaining_faces.insert(face.v);
      for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());remaining_edges.insert(edge);}}
    for(auto it=result.coverage.faces.begin();it!=result.coverage.faces.end();)if(!remaining_faces.count(it->first))it=result.coverage.faces.erase(it);else ++it;
    for(auto it=result.coverage.traces.begin();it!=result.coverage.traces.end();)if(!remaining_edges.count(it->first))it=result.coverage.traces.erase(it);else ++it;
    auto final_context=context->PrivateParameters(result);REQUIRE(final_context);final_context->ExportParameters(result);
    const auto before=MeasureMetricCells(full_patch.cells,*full_field),after=MeasureMetricCells(result.cells,*full_field);
    std::string metric_reason,replacement_reason;
    MetricDiagnosis diagnosis;
    const bool metric_primitive=ValidateMetricChange(full_patch.cells.size(),before,result.cells.size(),after,MetricChange::Repair,metric_reason,&diagnosis);
    const bool metric_valid=metric_primitive&&diagnosis.added_excess<=diagnosis.removed_excess&&
        (diagnosis.quality_progress||(after.shape_measured&&after.minimum_mean_ratio>=MinimumMeanRatio&&after.minimum_scaled_jacobian>=MinimumScaledJacobian));
    const bool replacement=state->CheckReplacement(full_patch,result,replacement_reason,ceiling,budget.caller_bytes,&budget);
    std::set<Face> old_physical;for(auto face:full_patch.surface){std::sort(face.v.begin(),face.v.end());old_physical.insert(face.v);}
    std::map<Face,Face> expected_skin;
    for(auto face:Incidence(full_patch.cells).Boundary()){auto key=face;std::sort(key.begin(),key.end());if(!old_physical.count(key))expected_skin.emplace(key,face);}
    for(auto face:result.surface){std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());auto key=face.v;std::sort(key.begin(),key.end());expected_skin.emplace(key,face.v);}
    std::vector<Face> prescribed;for(const auto& face:expected_skin)prescribed.push_back(face.second);
    std::string interface_reason;const bool interface_valid=ValidatePrescribedInterface(full_patch.cells,result.cells,prescribed,interface_reason);
    INFO(interface_reason);CHECK(interface_valid);INFO(replacement_reason);CHECK(replacement);
    const auto old_guidance=GeometryGuidanceExcess(full_patch,*scope,1e-4);
    const auto new_guidance=GeometryGuidanceExcess(result,*final_context,1e-4,old_guidance.excess);
    log<<"{\"stage\":\"complete_support_optimizer\",\"seed\":"<<std::quoted(label)<<",\"changed\":"<<changed
       <<",\"trials\":"<<report.trials<<",\"seconds\":"<<report.seconds<<",\"stop\":"<<std::quoted(report.stop)
       <<",\"old_cells\":"<<full_patch.cells.size()<<",\"new_cells\":"<<result.cells.size()
       <<",\"old_q_bad\":"<<before.q_bad_cells<<",\"old_J_bad\":"<<before.j_bad_cells
       <<",\"new_q_bad\":"<<after.q_bad_cells<<",\"new_J_bad\":"<<after.j_bad_cells
       <<",\"old_debt\":"<<double(before.shape_debt)<<",\"new_debt\":"<<double(after.shape_debt)
       <<",\"frozen_cells\":"<<frozen.size()<<",\"old_maximum_edge\":"<<double(before.maximum_length)<<",\"new_maximum_edge\":"<<double(after.maximum_length)
       <<",\"removed_size_excess\":"<<double(diagnosis.removed_excess)<<",\"added_size_excess\":"<<double(diagnosis.added_excess)
       <<",\"metric_primitive\":"<<metric_primitive
       <<",\"metric_valid\":"<<metric_valid<<",\"metric_reason\":"<<std::quoted(metric_reason)
       <<",\"replacement\":"<<replacement<<",\"replacement_reason\":"<<std::quoted(replacement_reason)
       <<",\"interface_valid\":"<<interface_valid<<",\"interface_reason\":"<<std::quoted(interface_reason)
       <<",\"old_geometry\":"<<double(old_guidance.excess)<<",\"new_geometry\":"<<double(new_guidance.excess)
       <<",\"geometry_complete\":"<<(old_guidance.complete&&new_guidance.complete)
       <<",\"boundary_candidates\":"<<report.boundary_candidates<<",\"boundary_moves\":"<<report.boundary_moves
       <<",\"boundary_shape_rejected\":"<<report.boundary_rejected_shape<<",\"boundary_geometry_rejected\":"<<report.boundary_rejected_geometry
       <<",\"boundary_validity_rejected\":"<<report.boundary_rejected_validity<<",\"boundary_edge_rejected\":"<<report.boundary_rejected_edge
       <<",\"interior_candidates\":"<<report.interior_candidates<<",\"interior_moves\":"<<report.interior_moves
       <<",\"interior_shape_rejected\":"<<report.interior_rejected_shape<<",\"interior_geometry_rejected\":"<<report.interior_rejected_geometry
       <<",\"global_contact_and_publication_checked\":false,\"accepted_repair\":false}\n";log.flush();
    CHECK_FALSE(budget.work_exhausted);CHECK(budget.geometry_failure.empty());
  };
  assess_optimizer(full_patch,"original_complete_52");
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
    if(direction==Edge{4502,181199364}) {
      CHECK(constructed);CHECK(replacement);
      const Edge retained_edge{174173910,181199364};
      REQUIRE_FALSE(state->Coverage().traces.count(retained_edge));REQUIRE(fresh.coverage.traces.count(retained_edge));
      auto lost=fresh;lost.coverage.traces.erase(retained_edge);std::string rejected;
      CHECK_FALSE(state->CheckReplacement(old,lost,rejected,ceiling,budget.caller_bytes,&budget));
    }
    if(constructed&&replacement) {
      auto fresh_field=field(fresh.cells,cavity);after=MeasureMetricCells(fresh.cells,*fresh_field);
      metric_valid=ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),after,MetricChange::Repair,metric_reason,&diagnosis);
      if(metric_valid)try {
        const auto was=GeometryGuidanceExcess(old,*scope,1e-4);old_geometry=was.excess;
        const auto now=GeometryGuidanceExcess(fresh,*scope,1e-4,old_geometry);new_geometry=now.excess;
        guidance_valid=was.complete&&now.complete&&std::isfinite(new_geometry)&&new_geometry<=old_geometry;
      }catch(const std::exception& e){guidance_reason=e.what();}
    }
    if(direction==Edge{4502,181199364}&&constructed&&replacement) {
      Patch joined=full_patch;std::set<Id> retired;for(const auto& c:old.cells)retired.insert(c.id);
      joined.cells.erase(std::remove_if(joined.cells.begin(),joined.cells.end(),[&](const Cell& c){return retired.count(c.id);}),joined.cells.end());
      joined.cells.insert(joined.cells.end(),fresh.cells.begin(),fresh.cells.end());
      std::set<Face> retired_faces;for(const auto& f:old.surface){auto key=f.v;std::sort(key.begin(),key.end());retired_faces.insert(key);}
      joined.surface.erase(std::remove_if(joined.surface.begin(),joined.surface.end(),[&](const SurfaceFace& f){auto key=f.v;std::sort(key.begin(),key.end());return retired_faces.count(key);}),joined.surface.end());
      joined.surface.insert(joined.surface.end(),fresh.surface.begin(),fresh.surface.end());
      joined.parameters=fresh.parameters;joined.coverage=fresh.coverage;
      assess_optimizer(joined,"collapse_with_retained_star_49");
      std::set<Id> bad_vertices;const Cell* obstruction=nullptr;long double largest_debt=0;
      for(const auto& cell:joined.cells){const auto measured=MeasureMetricCells({cell},*full_field);
        if(measured.shape_debt>largest_debt){largest_debt=measured.shape_debt;obstruction=&cell;}}
      REQUIRE(obstruction);for(const auto& node:obstruction->v)bad_vertices.insert(node.id);REQUIRE(bad_vertices.size()==4);
      for(Id vertex:bad_vertices){std::set<Id> frozen;
        for(const auto& cell:joined.cells)if(std::none_of(cell.v.begin(),cell.v.end(),[&](const Node& n){return n.id==vertex;}))frozen.insert(cell.id);
        const auto label="collapsed_bad_vertex_"+std::to_string(vertex)+"_star";
        assess_optimizer(joined,label.c_str(),frozen);
      }
    }
    log<<"{\"stage\":\"directed_collapse\",\"removed\":"<<direction[0]<<",\"retained\":"<<direction[1]
       <<",\"compatible\":"<<compatible<<",\"constructed\":"<<constructed<<",\"construction_reason\":"<<std::quoted(construction_reason)
       <<",\"replacement\":"<<replacement<<",\"replacement_reason\":"<<std::quoted(replacement_reason)
       <<",\"metric_measured\":"<<after.shape_measured<<",\"metric_valid\":"<<metric_valid<<",\"metric_reason\":"<<std::quoted(metric_reason)
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
