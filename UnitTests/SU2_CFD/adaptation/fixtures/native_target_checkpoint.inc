// Tiny complete composed authority: the saved sensor is only one input to the replayed live policy.
namespace native_target_checkpoint_test {
std::unique_ptr<CConfig> Config(double hmin=1e-6,const std::string& extra={}) {
  std::ostringstream text;
  text<<"SOLVER= NAVIER_STOKES\nMESH_FORMAT= BOX\nINIT_OPTION= TD_CONDITIONS\nREYNOLDS_NUMBER= 1e6\n"
      <<"MARKER_HEATFLUX= (wall,0)\nMARKER_FAR= (far)\nCOMPUTE_METRIC= YES\nADAP_SENSOR= MACH\n"
      <<"ADAP_REMESHER= NATIVE_CAVITY\nADAP_BL_METHOD= FIRST_ROW\nADAP_HMIN= "<<hmin<<"\nADAP_HMAX= 10\n"
      <<"ADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (.01)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.1)\nADAP_ANGLE= 60\n"<<extra;
  std::stringstream input(text.str());Mute mute;
  return std::make_unique<CConfig>(input,SU2_COMPONENT::SU2_CFD,false);
}
ReferenceState Reference() {
  const std::array<SU2Native3D::Node,4> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}};
  ReferenceState reference;reference.marker_names={"wall","far"};
  reference.facets={ {0,0,{{nodes[0],nodes[2],nodes[1]}}}, {1,1,{{nodes[0],nodes[1],nodes[3]}}},
                     {2,1,{{nodes[0],nodes[3],nodes[2]}}}, {3,1,{{nodes[1],nodes[2],nodes[3]}}} };
  reference.original=std::make_shared<PlanarReference>(reference.facets);return reference;
}
CNativeMetricFamily3D Family(const CConfig& config,const ReferenceState& reference) {
  CNativeMetricFamily3D f;f.first_row=SU2NativeBLPipeline3D::MakeTargetPolicy(config,reference,1.,-1.);
  // Exercise profile history after initial construction, including inherited tangential scaling.
  f.first_row->full->SelectTangentialScale(.8);f.first_row->Update(1.,config.GetAdap_Hgrad());
  f.layers=f.first_row->full;f.authority=reference.original;f.boundary_layer=true;
  f.layer_controls.push_back(config.GetAdap_BL(0));
  f.controls={{SU2_TYPE::GetValue(config.GetAdap_Hmin()),SU2_TYPE::GetValue(config.GetAdap_Hmax()),
    SU2_TYPE::GetValue(config.GetAdap_ARmax()),SU2_TYPE::GetValue(config.GetAdap_Hgrad()),SU2_TYPE::GetValue(config.GetAdap_Complexity()),
    double(config.GetAdap_BL_Budget()),SU2_TYPE::GetValue(config.GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(config.GetAdap_BL_OuterGrowthMax())}};
  f.surface_tolerance=config.GetAdap_Hausd();f.feature_angle=config.GetAdap_Angle();f.feature_identity=f.layers->GetFeatures().identity;
  f.inherited_tangent_scale=f.layers->GetTangentialScale();f.core=1.;f.normalization.estimate={2.,.01,1.};
  f.node_span=4;f.owned=1;f.nodes.push_back({0,{.2,.2,.02},0});
  CBoundaryLayerMetric::Tensor sensor,frame;
  SU2NativeBLPipeline3D::TargetPolicy::Unpack({2.,.1,0.,3.,0.,4.},sensor);
  SU2NativeBLPipeline3D::TargetPolicy::Unpack({1.,0.,0.,1.,0.,1.},frame);
  f.selected={sensor};f.frames={frame};f.eigenvalues={2.,3.,4.};
  CBoundaryLayerMetric::Tensor composed;
  SU2NativeBLPipeline3D::TargetPolicy::Unpack(f.first_row->Composition()(f.nodes[0].point,
      SU2NativeBLPipeline3D::TargetPolicy::Pack(sensor)),composed);
  f.composed_witnesses={composed};f.witness_versions={{f.layers->GetSampleVersion(),f.first_row->base->GetSampleVersion()}};
  f.witness_core=f.first_row->core;f.witness_radius=double(f.first_row->A->radius);
  f.target_extra_bytes=f.first_row->StorageBytes()-f.layers->StorageBytes();return f;
}
void Rechecksum(std::vector<char>& bytes) {
  const auto sum=SU2NativeTargetCheckpoint3D::detail::Checksum(bytes,bytes.size()-8);
  std::memcpy(bytes.data()+bytes.size()-8,&sum,8);
}
std::vector<char> Version1(const std::vector<char>& bytes) {
  SU2NativeTargetCheckpoint3D::detail::Header h;
  SU2NativeBoundary2D::RecordStream input(static_cast<const std::vector<char>&>(bytes));input(h);
  const auto payload=input.Size();h.version=1;
  std::vector<char> legacy;SU2NativeBoundary2D::RecordStream header(legacy);header(h);
  legacy.insert(legacy.end(),bytes.begin()+payload,bytes.end()-8);legacy.resize(legacy.size()+8);
  Rechecksum(legacy);return legacy;
}
}

TEST_CASE("Complete native target checkpoint preserves live composition and rejects incompatible authority", "[NativeTargetCheckpoint3D]") {
  using namespace native_target_checkpoint_test;
  using SU2NativeTargetCheckpoint3D::Encode;using SU2NativeTargetCheckpoint3D::Decode;
  auto config=Config();const auto reference=Reference();auto family=Family(*config,reference);
  const auto bytes=Encode(family,reference,*config,2.,3.,true);
  auto restored=Decode(bytes,reference,*config);REQUIRE(restored.family);
  CHECK(restored.pre_complexity==2.);CHECK(restored.final_complexity==3.);CHECK(restored.bracketed);
  CHECK(restored.family->eigenvalues==family.eigenvalues);
  CHECK(SU2NativeTargetCheckpoint3D::detail::State(*restored.family->layers)==SU2NativeTargetCheckpoint3D::detail::State(*family.layers));
  CHECK(SU2NativeTargetCheckpoint3D::detail::State(*restored.family->first_row->base)==SU2NativeTargetCheckpoint3D::detail::State(*family.first_row->base));
  const Tensor sensor{2.,.1,0.,3.,0.,4.};
  for(Point point: {Point{.2,.2,0},Point{.2,.2,.02},Point{.2,.2,.2}}) {
    CBoundaryLayerMetric::Tensor a,b;
    SU2NativeBLPipeline3D::TargetPolicy::Unpack(family.first_row->Composition()(point,sensor),a);
    SU2NativeBLPipeline3D::TargetPolicy::Unpack(restored.family->first_row->Composition()(point,sensor),b);
    for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)CHECK(a.m[i][j]==Approx(SU2_TYPE::GetValue(b.m[i][j])).epsilon(1e-12));
  }
  CHECK(Encode(*restored.family,reference,*config,2.,3.,true)==bytes);
  CHECK_THROWS(Decode({},reference,*config));
  auto truncated=bytes;truncated.pop_back();CHECK_THROWS(Decode(truncated,reference,*config));
  auto damaged=bytes;damaged[bytes.size()/2]^=1;CHECK_THROWS(Decode(damaged,reference,*config));
  auto wrongConfig=Config(2e-6);CHECK_THROWS(Decode(bytes,reference,*wrongConfig));
  auto wrongGeometry=reference;
  for(auto& facet:wrongGeometry.facets)for(auto& node:facet.v)node.p.x+=.125;
  wrongGeometry.original=std::make_shared<PlanarReference>(wrongGeometry.facets);
  CHECK_THROWS(Decode(bytes,wrongGeometry,*config));
  CHECK_THROWS_AS(Encode(family,reference,*config,2.,3.,true,bytes.size()),std::length_error);
  CHECK_THROWS_AS(Decode(bytes,reference,*config,bytes.size()),std::length_error);
  // A valid checksum does not authorize semantically invalid state.
  auto semantic=bytes;SU2NativeTargetCheckpoint3D::detail::Header h;
  SU2NativeBoundary2D::RecordStream input(static_cast<const std::vector<char>&>(semantic));input(h);h.owned=h.nodes+1;
  std::vector<char> header;SU2NativeBoundary2D::RecordStream output(header);output(h);
  std::copy(header.begin(),header.end(),semantic.begin());Rechecksum(semantic);
  CHECK_THROWS(Decode(semantic,reference,*config));
  semantic=bytes;SU2NativeBoundary2D::RecordStream coreInput(static_cast<const std::vector<char>&>(semantic));coreInput(h);
  h.family[4]=2*h.policy[0];header.clear();SU2NativeBoundary2D::RecordStream coreOutput(header);coreOutput(h);
  std::copy(header.begin(),header.end(),semantic.begin());Rechecksum(semantic);
  CHECK_THROWS(Decode(semantic,reference,*config));
  auto invalid=family;invalid.selected[0].m[0][0]=-1.;
  CHECK_THROWS(Encode(invalid,reference,*config,2.,3.,true));
  family.owned=0;family.frames.clear();family.eigenvalues.clear();
  const auto emptyOwned=Encode(family,reference,*config,2.,3.,false);
  const auto emptyRestored=Decode(emptyOwned,reference,*config);
  CHECK(emptyRestored.family->owned==0);CHECK(emptyRestored.family->frames.empty());CHECK_FALSE(emptyRestored.bracketed);
  const auto legacy=Version1(bytes);CHECK(Decode(legacy,reference,*config).family!=nullptr);
}

TEST_CASE("Native target checkpoint restores the exact ramp event and rejects legacy ramp state", "[NativeTargetRampCheckpoint]") {
  using namespace native_target_checkpoint_test;
  const std::string schedule="ADAP_LOOP= YES\nADAP_SIZES= (100, 100, 100)\nADAP_BL_H0_RAMP= 2\n";
  auto config=Config(1e-6,schedule);config->SetAdap_BL_RampEvent(3);
  auto reference=Reference();auto family=Family(*config,reference);
  const auto bytes=SU2NativeTargetCheckpoint3D::Encode(family,reference,*config,2.,3.,true);
  auto resumed=Config(1e-6,schedule);REQUIRE(resumed->GetAdap_BL_RampEvent()==0);
  const auto restored=SU2NativeTargetCheckpoint3D::DecodeRestoringStage(bytes,reference,*resumed);
  CHECK(resumed->GetAdap_BL_RampEvent()==3);CHECK(restored.stage.event==3);
  auto changed=Config(1e-6,"ADAP_LOOP= YES\nADAP_SIZES= (100, 100, 100)\nADAP_SUBITER= (2, 1, 1)\nADAP_BL_H0_RAMP= 2\n");
  CHECK_THROWS(SU2NativeTargetCheckpoint3D::DecodeRestoringStage(bytes,reference,*changed));
  CHECK(changed->GetAdap_BL_RampEvent()==0);
  auto wrongGeometry=reference;for(auto& facet:wrongGeometry.facets)for(auto& node:facet.v)node.p.y+=.125;
  wrongGeometry.original=std::make_shared<PlanarReference>(wrongGeometry.facets);
  auto rollback=Config(1e-6,schedule);
  CHECK_THROWS(SU2NativeTargetCheckpoint3D::DecodeRestoringStage(bytes,wrongGeometry,*rollback));
  CHECK(rollback->GetAdap_BL_RampEvent()==0);
  auto noRamp=Config();const auto legacy=Version1(SU2NativeTargetCheckpoint3D::Encode(Family(*noRamp,reference),reference,*noRamp,2.,3.,true));
  CHECK_THROWS(SU2NativeTargetCheckpoint3D::Decode(legacy,reference,*resumed));
}

TEST_CASE("Saved actual BL target isolates storage-accounting cost", "[.][NativeActualBLStorageAccounting3D]") {
  World world;REQUIRE(world.size==1);
  const auto* cfg=std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  const auto* ref=std::getenv("SU2_NATIVE_BL_REFERENCE");
  const auto* target=std::getenv("SU2_NATIVE_TARGET_CHECKPOINT3D");REQUIRE(cfg);REQUIRE(ref);REQUIRE(target);
  std::ifstream configFile(cfg);REQUIRE(bool(configFile));CConfig config(configFile,SU2_COMPONENT::SU2_CFD,false);
  const auto ceiling=config.GetAdap_Native_MemoryLimit();
  auto reference=std::make_shared<ReferenceState>();bool loaded=false;std::string reason;size_t peak=0;
  REQUIRE(LoadReference(ref,*reference,loaded,reason,ceiling,0,peak));REQUIRE(loaded);
  struct GeometryOwner {
    CGeometry** levels=nullptr;unsigned count=0;
    ~GeometryOwner(){if(levels){for(unsigned i=0;i<count;++i)delete levels[i];delete[] levels;}}
  } geometryOwner;
  geometryOwner.count=config.GetnMGLevels()+1;
  CDriver::BuildGeometryFVM(&config,new CPhysicalGeometry(&config,0,1),geometryOwner.levels,true);
  const auto& mesh=*geometryOwner.levels[MESH_0];
  CNativeRemesher3D prepare(reference);prepare.PrepareReference(config,mesh);
  SU2NativeBLPipeline3D::detail::PipelineScope pipeline;
  const auto restored=SU2NativeTargetCheckpoint3D::Read(target,*reference,config,ceiling,size_t(128)<<20);
  const auto& policy=*restored.family->first_row;
  std::ofstream out("actual_bl_storage_accounting.jsonl");REQUIRE(bool(out));out<<std::setprecision(17)<<std::boolalpha;
  auto measure=[&](const char* label,const CBoundaryLayerMetric& metric) {
    const auto stamp=SU2NativeTargetCheckpoint3D::detail::State(metric);
    const size_t storage=metric.StorageBytes(),workspace=metric.GeometryWorkspaceBytes();
    for(bool geometryOnly:{false,true}) {
      size_t calls=0,minimum=SIZE_MAX,maximum=0;volatile size_t checksum=0;
      const auto started=std::chrono::steady_clock::now();const auto cpu=std::clock();
      for(;calls<2048;++calls) {
        if(calls%64==0&&std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count()>30)break;
        const auto bytes=geometryOnly?metric.GeometryWorkspaceBytes():metric.StorageBytes();
        minimum=std::min(minimum,bytes);maximum=std::max(maximum,bytes);checksum+=bytes;
      }
      const auto seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
      const auto cpuSeconds=double(std::clock()-cpu)/CLOCKS_PER_SEC;
      CHECK(calls>0);CHECK(minimum==(geometryOnly?workspace:storage));CHECK(maximum==minimum);
      CHECK(SU2NativeTargetCheckpoint3D::detail::State(metric)==stamp);
      out<<"{\"policy\":"<<std::quoted(label)<<",\"operation\":"<<std::quoted(geometryOnly?"geometry_workspace":"storage")
         <<",\"calls\":"<<calls<<",\"bytes\":"<<minimum<<",\"seconds\":"<<seconds<<",\"CPU_seconds\":"<<cpuSeconds
         <<",\"checksum\":"<<checksum<<",\"target_unchanged\":true,\"mesh_adaptation_performed\":false}"<<std::endl;
      REQUIRE(bool(out));
    }
  };
  measure("full",*policy.full);measure("base",*policy.base);
}

#include "native_boundary_guidance_reference.inc"
#include "native_actual_bl_cavity_planning.inc"

TEST_CASE("Mapped target checkpoints reject the obsolete curvature law", "[NativeTargetCheckpoint]") {
  using namespace native_target_checkpoint_test;
  using namespace SU2NativeTargetCheckpoint3D;
  auto config=Config();auto reference=Reference();
  SU2NativeGeometry::MappedFacetReference::CurvedNets nets;
  const auto& face=reference.facets.front();
  const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
  for(unsigned q=0;q<10;++q) {
    auto& p=nets[face.id][q];
    for(unsigned k=0;k<3;++k){p.x+=powers[q][k]*face.v[k].p.x/3;p.y+=powers[q][k]*face.v[k].p.y/3;p.z+=powers[q][k]*face.v[k].p.z/3;}
  }
  nets[face.id][9].z-=.03;
  auto geometry=std::make_shared<SU2NativeGeometry::MappedFacetReference>(reference.original,
      std::map<Id,double>{{face.id,1e-12}},7,SIZE_MAX,false,nets);
  SU2Native3D::BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:reference.facets) {
    for(auto n:f.v)points.emplace(n.id,SU2Native3D::BoundaryPoint{n,geometry->Vertex(n.id)});
    skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
  }
  auto parameters=std::make_shared<SU2Native3D::BoundaryParameterState>(geometry,points,skin,1);
  reference.original=PlanarReference::WithParameters(reference.original,parameters);
  const auto bytes=Encode(Family(*config,reference),reference,*config,2.,3.,true);
  CHECK(Decode(bytes,reference,*config).family!=nullptr);
  auto obsolete=bytes;detail::Header h;SU2NativeBoundary2D::RecordStream input(static_cast<const std::vector<char>&>(bytes));input(h);
  h.version=2;std::vector<char> header;SU2NativeBoundary2D::RecordStream output(header);output(h);
  REQUIRE(header.size()==input.Size());std::copy(header.begin(),header.end(),obsolete.begin());Rechecksum(obsolete);
  CHECK_THROWS_WITH(Decode(obsolete,reference,*config),Catch::Contains("predates directional curvature"));
}
