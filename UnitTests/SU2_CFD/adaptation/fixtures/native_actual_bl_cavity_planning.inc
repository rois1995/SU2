#include "../../../../Common/include/adaptation/CNativeMetricTransport3D.hpp"
// Complete saved actual cavities, transported current nodal core and associations; diagnostic only.
TEST_CASE("Saved actual BL cavities compare cellwise and complete planning", "[.][NativeActualBLCavityPlanning3D]") {
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
#include "native_actual_bl_cavity_planning_body.inc"
}
