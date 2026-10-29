// Geometry-only proposal on a final private capture, not a replay of an earlier transaction.
#include "../../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../../Common/include/adaptation/CNativeDistributed3D.hpp"
#include "../../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../../Common/include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>
#include <string>
using namespace SU2Native3D;
using namespace SU2NativeGeometry;
static std::vector<char> Read(const char* path) {
  std::ifstream in(path,std::ios::binary|std::ios::ate);if(!in)throw std::runtime_error("Cannot open input");
  const auto size=in.tellg();if(size<0 || size>std::streamoff(size_t(128)<<20))throw std::length_error("Input exceeds capture ceiling");
  in.seekg(0);std::vector<char> out(size_t(size),0);if(!in.read(out.data(),size))throw std::runtime_error("Cannot read input");return out;
}
#include "../../../../Common/include/adaptation/CNativeEngine3D.hpp"
int main(int argc,char** argv) {
  SU2_MPI::Init(&argc,&argv);int exit_code=0;
  try {
    if(argc!=4)throw std::invalid_argument("usage: continuation authority payload owned");
    World world;if(world.size!=1)throw std::invalid_argument("Diagnostic requires one MPI rank");
    const auto refbytes=Read(argv[1]);auto archive=DecodeReference(std::string(refbytes.begin(),refbytes.end()),ReferenceStorageLimit3D);
    if(!archive.original||!archive.original->Parameters())throw std::runtime_error("Authority lacks parameters");
    const auto payload=Read(argv[2]);auto state=BoundaryParameterState::FromPayload(archive.original->Parameters()->Authority(),payload,ReferenceStorageLimit3D,0);
    auto reference=PlanarReference::WithParameters(archive.original,state);const auto bytes=Read(argv[3]);RecordStream input(bytes);uint64_t count=0;input(count);
    if(count>4096)throw std::length_error("Capture exceeds bound");Owned owned;std::vector<DonorCell> source;
    auto uniform=[](const Tensor& t){return t.xx==4&&t.xy==0&&t.xz==0&&t.yy==4&&t.yz==0&&t.zz==4;};
    for(uint64_t i=0;i<count;++i){OwnedCell row;input(row);if(!row.sensor_present)throw std::invalid_argument("Captured cell lacks nodal metric");
      for(const auto& metric:row.sensor)if(!uniform(metric))throw std::invalid_argument("Captured nodal metric differs from diag4");
      if(!owned.emplace(row.cell.id,row).second)throw std::invalid_argument("Duplicate captured cell");DonorCell donor;donor.cell=row.cell;donor.sensor=row.sensor;source.push_back(donor);}
    if(!input.End()||owned.empty())throw std::invalid_argument("Invalid captured cells");
    std::cout<<"scope=diagnostic_continuation_of_unaccepted_final_private_state donor=captured_cells_uniform_diag4 input_cells="<<owned.size()<<" geometry_entity="<<state->EntityGeometry()<<" geometry_version="<<state->Geometry().Version()<<std::endl;
    EngineOptions options;options.maximum_work=4096;options.maximum_seconds=120;options.memory_ceiling=size_t(2)<<30;options.surface_tolerance=.001;options.timings=true;
    size_t rejected=0;options.rejected_trial=[&](const Patch& old,const Patch& fresh,Feature feature,const MetricDiagnosis& diagnosis,bool built,const std::string& reason){
      if(rejected++==0)std::cout<<"first_rejected feature_count="<<feature.count<<" feature="<<feature.vertices[0]<<','<<feature.vertices[1]<<','<<feature.vertices[2]<<" old_cells="<<old.cells.size()<<" fresh_cells="<<fresh.cells.size()<<" built="<<built<<" metric_failure="<<int(diagnosis.failure)<<" reason="<<reason<<std::endl;};
    options.stage_observer=[&](const StageProgressView& view){const auto& p=view.progress;
      std::cout<<"stage="<<view.stage<<" round="<<view.round<<" cells="<<view.working.size()<<" q_bad="<<p.q_bad_cells<<" j_bad="<<p.j_bad_cells<<" size_bad="<<p.size_bad_cells<<" geometry_bad="<<p.geometry_bad_cells<<" measured="<<p.measured_cells<<" shape_measured="<<p.shape_measured_cells<<" q_debt="<<p.q_hinge_squared<<" j_debt="<<p.j_hinge_squared<<" size_debt="<<p.size_hinge_squared<<" repairs="<<p.remaining_repairs<<" edges="<<p.remaining_edges<<" work="<<view.stats.global_work<<" atlas_work="<<view.stats.atlas_work<<std::endl;};
    const auto result=AdaptMetric(world,owned,source,*reference,{},options);
    std::cout<<"result_valid="<<result.valid<<" stop="<<EngineStopName(result.stats.stop)<<" reason="<<result.reason<<" final_cells="<<result.cells.size()<<" work="<<result.stats.global_work<<" atlas_work="<<result.stats.atlas_work<<" atlas_peak="<<result.stats.atlas_peak_bytes<<" admitted_peak="<<result.stats.maximum_work_bytes<<" attempted="<<result.stats.attempted<<" accepted="<<result.stats.accepted<<" rejected="<<result.stats.rejected<<" rejected_callbacks="<<rejected<<std::endl;
    if(result.valid){size_t bad=0;for(const auto& row:result.cells){const auto& v=row.second.cell.v;const auto m=Measure({v[0].p,v[1].p,v[2].p,v[3].p},Tensor{4,0,0,4,0,4});bad+=Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<=0||m.mean_ratio<MinimumMeanRatio||m.minimum_scaled_jacobian<MinimumScaledJacobian||m.maximum_edge>MaximumMetricEdge;}std::cout<<"independent_final_metric_bad_cells="<<bad<<std::endl;if(bad)exit_code=2;}
  }catch(const std::exception& e){std::cerr<<"continuation_error="<<e.what()<<std::endl;exit_code=1;}
  SU2_MPI::Finalize();return exit_code;
}
