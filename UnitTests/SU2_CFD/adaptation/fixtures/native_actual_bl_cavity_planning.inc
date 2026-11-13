#include "../../../../Common/include/adaptation/CNativeMetricTransport3D.hpp"
#include <ctime>
// Complete saved actual cavities, transported current nodal core and associations; diagnostic only.
namespace {void RunSavedActualCavities(bool cavityCorrection,bool heightContinuation=false,bool productionCorrection=false,bool guidanceReuse=false,bool planningCost=false,bool clearanceProbe=false,bool geometryProbe=false,bool compoundSplit=false,bool splitContinuation=false,bool residualProbe=false,bool diagonalProbe=false,bool stripProbe=false,bool patchTrajectory=false,bool currentResidual=false,bool productionSurface=false,bool directedCollapse=false) {
  World world;REQUIRE((world.size==1||(productionSurface&&world.size==6)));
  const auto* cfg=std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  const auto* ref=std::getenv("SU2_NATIVE_BL_REFERENCE");
  const auto* target=std::getenv("SU2_NATIVE_TARGET_CHECKPOINT3D");REQUIRE(cfg);REQUIRE(ref);REQUIRE(target);
  std::ifstream configFile(cfg);REQUIRE(bool(configFile));CConfig config(configFile,SU2_COMPONENT::SU2_CFD,false);
  const auto ceiling=config.GetAdap_Native_MemoryLimit();
  const auto* captureRoot=std::getenv("SU2_NATIVE_BL_PLANNING_CAPTURE");REQUIRE(captureRoot);
  const bool coherentCapture=bool(std::ifstream(std::string(captureRoot)+"/trajectory_working_owned.bin",std::ios::binary));
  if(productionSurface)REQUIRE(coherentCapture);
  auto reference=std::make_shared<ReferenceState>();bool loaded=false;std::string reason;size_t peak=0;
  REQUIRE(LoadReference(ref,*reference,loaded,reason,ceiling,0,peak));REQUIRE(loaded);
  struct GeometryOwner {
    CGeometry** levels=nullptr;unsigned count=0;
    ~GeometryOwner(){if(levels){for(unsigned i=0;i<count;++i)delete levels[i];delete[] levels;}}
  } geometryOwner;
  if(!coherentCapture) {
    geometryOwner.count=config.GetnMGLevels()+1;
    CDriver::BuildGeometryFVM(&config,new CPhysicalGeometry(&config,0,1),geometryOwner.levels,true);
    const auto& mesh=*geometryOwner.levels[MESH_0];
    CNativeRemesher3D prepare(reference);prepare.PrepareReference(config,mesh);
  } else {
    REQUIRE(reference->original);REQUIRE(reference->original->Parameters());
    REQUIRE(reference->original->Parameters()->EntityGeometry());
    REQUIRE(reference->original->Mobility());
  }
  SU2NativeBLPipeline3D::detail::PipelineScope pipeline;
  std::string checkpoint=target;
  if(productionSurface&&world.size>1) {
    const auto slash=checkpoint.find_last_of('/');REQUIRE(slash!=std::string::npos);
    checkpoint=checkpoint.substr(0,slash+1)+"target_rank"+std::to_string(world.rank)+".bin";
  }
  const auto restored=SU2NativeTargetCheckpoint3D::Read(checkpoint,*reference,config,ceiling,size_t(128)<<20);
  const auto& policy=*restored.family->first_row;
#include "native_actual_bl_cavity_planning_body.inc"
}
}
TEST_CASE("Saved actual BL cavities compare cellwise and complete planning", "[.][NativeActualBLCavityPlanning3D]") {
  RunSavedActualCavities(false);
}
TEST_CASE("Saved actual M6 cavities test bounded correction candidates", "[.][NativeActualCavityCorrection3D]") {
  RunSavedActualCavities(true);
}
#include "native_actual_cavity_correction_mpi.inc"

TEST_CASE("Saved actual M6 cavities compare explicit first-height stages", "[.][NativeActualCavityHeightContinuation3D]") {
  RunSavedActualCavities(true,true);
}

TEST_CASE("Saved M6 production correction preserves complete patch admission", "[.][NativeActualCavityCorrectionProduction3D]") {
  RunSavedActualCavities(true,false,true);
}

TEST_CASE("Saved M6 cavities quantify existing unchanged-face guidance reuse", "[.][NativeActualCavityGuidanceReuse3D]") {
  RunSavedActualCavities(true,false,true,true);
}

TEST_CASE("Saved M6 boundary planning separates actual guidance components", "[.][NativeActualBoundaryPlanningCost3D]") {
  RunSavedActualCavities(true,false,true,false,true);
}

TEST_CASE("Saved M6 surface splits distinguish physical clearance from unresolved bounds", "[.][NativeActualSurfaceClearance3D]") {
  RunSavedActualCavities(true,false,true,false,false,true);
}

TEST_CASE("Saved M6 parent child geometry bounds preserve exact supports", "[.][NativeActualSurfaceGeometry3D]") {
  RunSavedActualCavities(true,false,true,false,false,false,true);
}

TEST_CASE("Saved M6 bounded adjacent edge splits compare complete replacement", "[.][NativeActualCompoundSurfaceSplit3D]") {
  RunSavedActualCavities(true,false,true,false,false,false,false,true);
}

TEST_CASE("Saved M6 perimeter refinement tests bounded continuation after metric transport", "[.][NativeActualSurfaceSplitContinuation3D]") {
  RunSavedActualCavities(true,false,true,false,false,false,false,false,true);
}

TEST_CASE("Saved M6 residual faces compare existing complete cavity splits", "[.][NativeActualSurfaceResidual3D]") {
  RunSavedActualCavities(true,false,true,false,false,false,false,false,true,true);
}

TEST_CASE("Saved M6 split faces test existing reconnect and boundary shell composition", "[.][NativeActualSurfaceDiagonal3D]") {
  RunSavedActualCavities(true,false,true,false,false,false,false,false,true,true,true);
}

TEST_CASE("Saved M6 directional surface strips retain complete adjacent volume", "[.][NativeActualSurfaceStripCavity3D]") {
  RunSavedActualCavities(true,false,true,false,false,false,false,false,true,true,true,true);
}

TEST_CASE("Saved M6 connected surface volume patch follows geometry to first row", "[.][NativeActualSurfacePatchTrajectory3D]") {
  RunSavedActualCavities(true,false,true,false,false,false,false,false,true,true,false,false,true);
}

TEST_CASE("Saved current M6 residuals classify complete geometry and clearance refusals", "[.][NativeActualCurrentResidual3D]") {
  RunSavedActualCavities(true,false,false,false,false,false,false,false,false,false,false,false,false,true);
}

TEST_CASE("Saved current M6 patches follow the complete bounded surface volume trajectory", "[.][NativeActualCurrentTrajectory3D]") {
  RunSavedActualCavities(true,false,true,false,false,false,false,false,false,false,false,false,true,true);
}

TEST_CASE("Saved transported M6 state resumes the production surface sequence", "[.][NativeActualSavedSurfaceStage3D]") {
  RunSavedActualCavities(true,false,false,false,false,false,false,false,false,false,false,false,false,true,true);
}

TEST_CASE("Saved current M6 nominated directed collapses test the physical surface", "[.][NativeActualDirectedResidual3D]") {
  RunSavedActualCavities(true,false,false,false,false,false,false,false,false,false,false,false,false,true,false,true);
}
