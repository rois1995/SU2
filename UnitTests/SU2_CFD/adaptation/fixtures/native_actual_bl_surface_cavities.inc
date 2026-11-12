// Actual original mesh/reference geometry trials; identity-metric controls are NOT production target certificates.
TEST_CASE("Actual curved BL surface cavities preserve complete dependencies", "[.][NativeActualBLSurfaceCavity3D]") {
  World world;REQUIRE(world.size==1);
  const auto* cfg=std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  const auto* ref=std::getenv("SU2_NATIVE_BL_REFERENCE");
  const auto* contexts=std::getenv("SU2_NATIVE_BL_CAVITY_CONTEXTS");
  REQUIRE(cfg);REQUIRE(ref);REQUIRE(contexts);
  std::ifstream configFile(cfg);REQUIRE(bool(configFile));CConfig config(configFile,SU2_COMPONENT::SU2_CFD,false);
  const auto ceiling=config.GetAdap_Native_MemoryLimit();REQUIRE(ceiling>0);
  auto reference=std::make_shared<ReferenceState>();bool loaded=false;size_t loadPeak=0;std::string reason;
  REQUIRE(LoadReference(ref,*reference,loaded,reason,ceiling,0,loadPeak));REQUIRE(loaded);
  REQUIRE(reference->original);REQUIRE(reference->original->Mapped());
  struct GeometryOwner {
    CGeometry** levels=nullptr;unsigned count=0;
    ~GeometryOwner(){if(levels){for(unsigned i=0;i<count;++i)delete levels[i];delete[] levels;}}
  } geometryOwner;
  geometryOwner.count=config.GetnMGLevels()+1;
  CDriver::BuildGeometryFVM(&config,new CPhysicalGeometry(&config,0,1),geometryOwner.levels,true);
  const auto& mesh=*geometryOwner.levels[MESH_0];
  CNativeRemesher3D prepare(reference);prepare.PrepareReference(config,mesh);
  ImportedGeometry imported;BridgeStats stats;
  REQUIRE(ImportGeometry(config,mesh,nullptr,*reference,imported,reason,stats,ceiling,ReferenceStorageBytes(*reference)));
  std::map<std::array<Id,4>,Id> byVertices;
  Id nextNode=0,nextCell=0;
  for(const auto& row:imported.cells) {
    std::array<Id,4> key;nextCell=std::max(nextCell,row.first);
    for(unsigned k=0;k<4;++k){key[k]=row.second.cell.v[k].id;nextNode=std::max(nextNode,key[k]);}
    std::sort(key.begin(),key.end());REQUIRE(byVertices.emplace(key,row.first).second);
  }
  REQUIRE(nextNode<UINT64_MAX-16);REQUIRE(nextCell<UINT64_MAX-MaximumReplacementCells);++nextNode;++nextCell;
  SU2NativeGeometry::AtlasBudget budget;budget.maximum_bytes=ceiling;
  budget.caller_bytes=transfer_memory::Add(ReferenceStorageBytes(*reference),OwnedStorageBytes(imported.cells),
      transfer_memory::Bytes(imported.original),transfer_memory::Mul(byVertices.size(),sizeof(std::pair<const std::array<Id,4>,Id>)+4*sizeof(void*)),size_t(128)<<20);
  REQUIRE(budget.caller_bytes<ceiling);
  // Use the engine's existing bounded arithmetic envelope, not AtlasBudget's zero-bit default.
  constexpr size_t atlasScratch=size_t(16)<<20;size_t low=0,high=atlasScratch*8;
  while(low<high){const size_t bits=low+(high-low+1)/2;
    if(transfer_memory::Mul(2,SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits))<=atlasScratch/2)low=bits;else high=bits-1;}
  REQUIRE(low>0);budget.maximum_integer_bits=low;
  const auto work=config.GetAdap_Native_Max_Work()?size_t(config.GetAdap_Native_Max_Work()):std::max(size_t(4096),transfer_memory::Mul(imported.cells.size(),64));
  budget.maximum_work=transfer_memory::Mul(transfer_memory::Add(imported.cells.size(),work),SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  const auto started=std::chrono::steady_clock::now();
  const bool surfaceStage=std::getenv("SU2_NATIVE_BL_FRONT_PROBE")&&
      std::string(std::getenv("SU2_NATIVE_BL_FRONT_PROBE"))=="checkpoint_target_surface";
  // The separate production-stage replay uses the existing event ledger, not the 300s patch diagnostic.
  const double seconds=surfaceStage?SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time()):
      std::min(300.,SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time()));REQUIRE(seconds>0);
  budget.within_time=[&](){return std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()<seconds;};
  auto scope=reference->original->WithAtlasReservation(budget);
  auto directory=std::make_unique<Directory>(world);REQUIRE(directory->Build(imported.cells,reason,ceiling,budget.caller_bytes));
  budget.caller_bytes=transfer_memory::Add(budget.caller_bytes,directory->ResidentBytes());REQUIRE(budget.caller_bytes<ceiling);
  WallHeights heights;heights.tolerance=SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol());
  for(unsigned short w=0;w<config.GetnAdap_BL();++w) {
    const auto& wall=config.GetAdap_BL(w);const auto at=std::find(reference->marker_names.begin(),reference->marker_names.end(),wall.marker);
    REQUIRE(at!=reference->marker_names.end());heights[int(at-reference->marker_names.begin())]=SU2_TYPE::GetValue(wall.firstHeight);
  }
  struct Restore {ShapeAdmission previous=GetShapeAdmission();~Restore(){SetShapeAdmission(previous);}} restore;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  std::ifstream input(contexts);REQUIRE(bool(input));size_t count=0;input>>count;REQUIRE(count>0);REQUIRE(count<=8);
  std::ofstream log("actual_bl_surface_cavities.jsonl");REQUIRE(bool(log));log<<std::setprecision(17)<<std::boolalpha;
  auto patchOf=[&](const std::set<Id>& ids) {
    Patch patch;const std::array<std::array<unsigned,3>,4> slots{{{{1,2,3}},{{0,3,2}},{{0,1,3}},{{0,2,1}}}};
    for(Id id:ids){const auto& row=imported.cells.at(id);patch.cells.push_back(row.cell);
      for(unsigned f=0;f<4;++f)if(row.physical[f].present){Face face;
        for(unsigned k=0;k<3;++k)face[k]=row.cell.v[slots[f][k]].id;patch.surface.push_back({face,row.physical[f].facet});}}
    scope->ExportParameters(patch,&budget);return patch;
  };
  auto describe=[&](const std::string& label,const char* op,const Patch& patch) {
    const Tensor identity{1,0,0,1,0,1};size_t qb=0,jb=0;double q=1,J=1,error=0;size_t gb=0;
    std::map<Id,Point> points;
    for(const auto& cell:patch.cells){Tetrahedron tet;
      for(unsigned k=0;k<4;++k){tet[k]=cell.v[k].p;points.emplace(cell.v[k].id,cell.v[k].p);}
      REQUIRE(Orientation(tet[0],tet[1],tet[2],tet[3])>0);
      const auto m=Measure(tet,identity);q=std::min(q,m.mean_ratio);J=std::min(J,m.minimum_scaled_jacobian);
      qb+=m.mean_ratio<MinimumMeanRatio;jb+=m.minimum_scaled_jacobian<MinimumScaledJacobian;
    }
    auto privateGeometry=scope->PrivateParameters(patch,&budget);const auto& geometry=privateGeometry?*privateGeometry:*scope;
    const double tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());REQUIRE(tolerance>0);
    for(const auto& face:patch.surface){std::array<Point,3> triangle;
      for(unsigned k=0;k<3;++k)triangle[k]=points.at(face.v[k]);
      if(!geometry.ContainsFace(face.facet,triangle))throw std::runtime_error("Candidate physical face is not supported by its reference.");
      const auto excess=geometry.SurfaceError(face.facet,triangle);error=std::max(error,excess);gb+=excess>tolerance;
    }
    FirstHeightReport report;std::string why;const bool height=CheckFirstHeight(patch,heights,geometry,report,why);
    log<<"{\"context\":"<<std::quoted(label)<<",\"operation\":"<<std::quoted(op)<<",\"cells\":"<<patch.cells.size()
       <<",\"points\":"<<points.size()<<",\"euclidean_control_q_bad\":"<<qb<<",\"euclidean_control_J_bad\":"<<jb
       <<",\"euclidean_control_q\":"<<q<<",\"euclidean_control_J\":"<<J<<",\"geometry_bad_faces\":"<<gb
       <<",\"maximum_geometry_error\":"<<error<<",\"requested_first_height_pass\":"<<height
       <<",\"first_height_maximum_relative_error\":"<<report.maximum_relative_error
       <<",\"first_height_maximum_tilt\":"<<report.maximum_tilt
       <<",\"first_height_reference_clearance_failures\":"<<report.reference_clearance_failures
       <<",\"first_height_reason\":"<<std::quoted(why)<<",\"production_metric_certified\":false}"<<std::endl;
  };
  if(const auto* mode=std::getenv("SU2_NATIVE_BL_FRONT_PROBE");mode&&std::string(mode)=="tangential_target") {
#include "native_actual_bl_tangential_target_probe.inc"
    return;
  }
  if(const auto* mode=std::getenv("SU2_NATIVE_BL_FRONT_PROBE");mode&&std::string(mode)=="seed_measurement") {
#include "native_actual_bl_seed_measurement.inc"
    return;
  }
  if(std::getenv("SU2_NATIVE_BL_FRONT_PROBE")) {
#include "native_actual_bl_front_probe.inc"
    return;
  }
  size_t trials=0;
  for(size_t c=0;c<count;++c) {
    std::string label;Edge edge{};size_t n=0;input>>label>>edge[0]>>edge[1]>>n;REQUIRE(bool(input));REQUIRE(edge[0]!=edge[1]);REQUIRE(n>0);REQUIRE(n<=MaximumCavityCells);
    std::set<Id> selected;
    for(size_t i=0;i<n;++i){std::array<Id,4> key;for(auto& id:key)input>>id;REQUIRE(bool(input));std::sort(key.begin(),key.end());REQUIRE(selected.insert(byVertices.at(key)).second);}
    std::set<Id> complete;Point endpoints[2];bool found[2]={false,false};
    for(const auto& row:imported.cells)for(auto node:row.second.cell.v)for(unsigned k=0;k<2;++k)if(node.id==edge[k]){complete.insert(row.first);endpoints[k]=node.p;found[k]=true;}
    REQUIRE(found[0]);REQUIRE(found[1]);
    REQUIRE(complete==selected);Patch old=patchOf(selected);
    describe(label,"unchanged_control",old);
    for(unsigned trial=0;trial<8;++trial) {
      budget.CheckTime();const auto t0=std::chrono::steady_clock::now();const auto w0=budget.work;
      std::set<Id> operationCells,unchangedCells;
      const Id vertex=edge[trial<2?trial:trial<4?trial-2:trial<6?trial-4:0];
      for(Id id:selected) {
        unsigned endpointsPresent=0;bool hasVertex=false;
        for(auto node:imported.cells.at(id).cell.v){endpointsPresent+=node.id==edge[0]||node.id==edge[1];hasVertex=hasVertex||node.id==vertex;}
        const bool use=trial<6?hasVertex:trial==6?endpointsPresent==2:true;
        (use?operationCells:unchangedCells).insert(id);
      }
      REQUIRE_FALSE(operationCells.empty());const auto operationOld=patchOf(operationCells);
      Patch fresh;std::string why;bool built=false,clear=false,measured=false;const char* op="";
      try {
        Point middle{double(((long double)endpoints[0].x+endpoints[1].x)/2),double(((long double)endpoints[0].y+endpoints[1].y)/2),double(((long double)endpoints[0].z+endpoints[1].z)/2)};
        if(trial<2){op="collapse";built=CollapseVertex(operationOld,edge[trial],edge[1-trial],nextCell,*scope,fresh,why);}
        else if(trial<4){op="relocate_midpoint";built=MoveVertex(operationOld,edge[trial-2],{nextNode,middle},nextCell,*scope,fresh,why);}
        else if(trial<6){op="reconnect";built=Reconnect(operationOld,edge[trial-4],nextCell,*scope,fresh,why);}
        else if(trial==6){op="split";built=SplitEdge(operationOld,edge,{nextNode,middle},nextCell,*scope,fresh,why);}
        else {op="surface_strip";built=SplitSurfaceStrip(operationOld,edge,{nextNode,middle},{},nextCell,*scope,fresh,why);}
        if(built){
          clear=true;REQUIRE(directory->Clearance(imported.cells,operationOld,fresh,clear,why,ceiling,budget.caller_bytes));
          const auto rest=patchOf(unchangedCells);Patch affected=fresh;
          affected.cells.insert(affected.cells.end(),rest.cells.begin(),rest.cells.end());
          affected.surface.insert(affected.surface.end(),rest.surface.begin(),rest.surface.end());
          affected.parameters.insert(rest.parameters.begin(),rest.parameters.end());
          affected.coverage.faces.insert(rest.coverage.faces.begin(),rest.coverage.faces.end());
          affected.coverage.traces.insert(rest.coverage.traces.begin(),rest.coverage.traces.end());
          describe(label,op,affected);measured=true;
        }
      }catch(const std::exception& e){why=e.what();}
      log<<"{\"context\":"<<std::quoted(label)<<",\"trial\":"<<trial<<",\"operation\":"<<std::quoted(op)
         <<",\"operator_old_cells\":"<<operationOld.cells.size()<<",\"context_old_cells\":"<<old.cells.size()
         <<",\"constructed\":"<<built<<",\"external_clearance\":"<<clear<<",\"patch_measurement_complete\":"<<measured<<",\"reason\":"<<std::quoted(why)
         <<",\"seconds\":"<<std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count()<<",\"geometry_work\":"<<budget.work-w0
         <<",\"production_admission\":false}"<<std::endl;++trials;
      REQUIRE_FALSE(budget.time_exhausted);REQUIRE_FALSE(budget.work_exhausted);
    }
  }
  input>>std::ws;REQUIRE(input.eof());REQUIRE(trials==8*count);
  log<<"{\"complete\":true,\"contexts\":"<<count<<",\"trials\":"<<trials<<",\"geometry_work\":"<<budget.work
     <<",\"mesh_published\":false,\"production_metric_certified\":false}"<<std::endl;REQUIRE(bool(log));
}
