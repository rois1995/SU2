/*!
 * \file CNativeRemesher3D_tests.cpp
 * \brief Default CFD ownership and private-progress rejection in the native tetrahedral runtime.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <map>
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../SU2_CFD/include/output/CMeshOutput.hpp"
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
using namespace transfer_test;
using namespace SU2Native3D;

namespace {
std::vector<double> Snapshot(const MeshSolution& mesh) {
  std::vector<double> values;
  const auto& geometry = mesh.Fine();
  const auto* nodes = mesh.solver[MESH_0][FLOW_SOL]->GetNodes();
  for (unsigned long p = 0; p < geometry.GetnPoint(); ++p) {
    values.push_back(geometry.nodes->GetGlobalIndex(p));
    for (size_t d = 0; d < 3; ++d) values.push_back(SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, d)));
    for (size_t k = 0; k < 5; ++k) values.push_back(SU2_TYPE::GetValue(nodes->GetSolution(p, k)));
  }
  for (unsigned long e = 0; e < geometry.GetnElem(); ++e)
    for (size_t k = 0; k < 4; ++k) values.push_back(geometry.elem[e]->GetNode(k));
  return values;
}
void CheckTransport(const RemeshStats& s) {
  CHECK(s.communication_bytes == s.imported.tracked_bytes + s.parent_transport_bytes + s.worker_transport_bytes);
  CHECK(s.communication_calls == s.imported.tracked_calls + s.parent_transport_calls + s.worker_transport_calls);
  CHECK(s.communication_seconds == Approx(s.imported.tracked_collective_seconds + s.parent_transport_seconds +
                                          s.worker_transport_seconds).margin(1e-12));
  CHECK(s.worker_transport_bytes == s.refine.tracked_bytes + s.coarsen.tracked_bytes);
  CHECK(s.worker_transport_calls == s.refine.tracked_calls + s.coarsen.tracked_calls);
  CHECK(s.worker_transport_seconds == Approx(s.refine.tracked_collective_seconds + s.coarsen.tracked_collective_seconds).margin(1e-12));
  CHECK(s.parent_transport_bytes >= s.weights.tracked_bytes + s.returned.tracked_bytes);
  CHECK(s.parent_transport_calls >= s.weights.tracked_calls + s.returned.tracked_calls);
  for (const auto* engine : {&s.refine, &s.coarsen}) {
    CHECK(engine->cached_invalid_cells <= engine->cached_measured_cells);
    if (s.complete) {
      CHECK(engine->cached_invalid_cells == 0);
      CHECK(engine->cached_measured_cells == engine->final_owned_cells);
      CHECK(engine->remaining_edges == 0);
    }
  }
  CHECK(s.imported.tracked_calls > 0);
  CHECK(s.communication_calls > 0);
}
}  // namespace

TEST_CASE("Native3D default ownership survives rejection after private accepted edits", "[NativeDefault3D]") {
  World world;
  auto config = MakeConfig(3,
                           "SOLVER= EULER\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\n"
                           "ADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
                           "ADAP_NATIVE_REPARTITION= NO\nADAP_NATIVE_RANKS= 0\n");
  MeshSolution donor(config.get(), BoxMesh(3, 1, false), 0);
  const auto before = Snapshot(donor);
  su2activematrix sensor(donor.Fine().GetnPointDomain(), 6);
  for (unsigned long p = 0; p < sensor.rows(); ++p)
    for (size_t k = 0; k < 6; ++k) sensor(p, k) = (k == 0 || k == 3 || k == 5) ? 4. : 0.;
  auto reference = std::make_shared<ReferenceState>();
  EngineOptions limited;
  limited.maximum_rounds = 1;
  CNativeRemesher3D rejected(reference, limited);
  const auto bad = rejected.Remesh(*config, donor.Fine(), sensor);
  CHECK(bad.status != CRemeshResult::Status::COMPLETE);
  CHECK_FALSE(bad.onAccepted);
  CHECK_FALSE(reference->original);
  CHECK(Snapshot(donor) == before);
  CHECK(CPassiveComm::Allreduce(rejected.Statistics().refine.accepted, CPassiveComm::Op::SUM) > 0);
  CheckTransport(rejected.Statistics());
  CHECK(rejected.Statistics().workers == world.size);
  CHECK(rejected.Statistics().partition.destinations.empty());
  // Retry the same accepted CFD mesh and unchanged frozen sensor, without partitioning it.
  CNativeRemesher3D backend(reference);
  const auto result = backend.Remesh(*config, donor.Fine(), sensor);
  REQUIRE(result.status == CRemeshResult::Status::COMPLETE);
  CHECK_FALSE(reference->original);
  CHECK(backend.Statistics().partition.destinations.empty());
  CHECK(backend.Statistics().source.builds == 0);
  CHECK(backend.Statistics().weights.requests == 0);
  CHECK(backend.Statistics().weights.tracked_calls == 0);
  CheckTransport(backend.Statistics());
  CHECK(backend.Statistics().workers == world.size);
  CHECK(Snapshot(donor) == before);
  for (unsigned long p = 0; p < sensor.rows(); ++p)
    for (size_t k = 0; k < 6; ++k) CHECK(sensor(p, k) == ((k == 0 || k == 3 || k == 5) ? 4. : 0.));
  MeshSolution accepted(config.get(), result.slices, 0);
  const auto transported = result.slices.FetchPointMetric(accepted.Fine());
  REQUIRE(transported.size() == 6 * accepted.Fine().GetnPoint());
  for (size_t k = 0; k < transported.size(); ++k)
    CHECK(transported[k] == ((k % 6 == 0 || k % 6 == 3 || k % 6 == 5) ? 4. : 0.));
  result.onAccepted();
  REQUIRE(reference->original);
  const auto encoded = EncodeReference(*reference);
  result.onAccepted();
  CHECK(EncodeReference(*reference) == encoded);
  {
    Mute mute;
    CMeshOutput::WriteMesh(config.get(), &accepted.Fine(), "default_native3d");
  }
  WriteReference("default_native3d.su2.native_ref", *reference);
  ReferenceState reloaded;
  CHECK(LoadReference("default_native3d.su2.native_ref", reloaded));
  CHECK(EncodeReference(reloaded) == encoded);
}

TEST_CASE("Native3D budgeted import preserves both outputs and reference on elected refusal", "[NativeBudget3D]") {
  World world;
  auto config = MakeConfig(3, "SOLVER= EULER\n");
  MeshSolution donor(config.get(), BoxMesh(3, 1, false), 0);
  const auto saved = Snapshot(donor);
  ReferenceState reference;
  ImportedGeometry baseline;
  BridgeStats measured;
  std::string reason;
  REQUIRE(ImportGeometry(*config, donor.Fine(), nullptr, reference, baseline, reason, measured, SIZE_MAX));
  REQUIRE(reference.original);
  CHECK(measured.maximum_native_bytes > 0);
  CHECK(measured.tracked_calls > 0);
  CHECK(baseline.stats.total_seconds == measured.total_seconds);
  CHECK(baseline.stats.tracked_bytes == measured.tracked_bytes);
  CHECK(baseline.stats.tracked_calls == measured.tracked_calls);
  CHECK(baseline.stats.tracked_collective_seconds == measured.tracked_collective_seconds);
  CHECK(reference.original->ResidentBytes() > 0);
  CHECK(ReferenceStorageBytes(reference) >= reference.original->ResidentBytes());
  const auto immutable = EncodeReference(reference);
  const auto original = reference.original;
  ImportedGeometry loaded;
  BridgeStats loaded_stats;
  REQUIRE(ImportGeometry(*config, donor.Fine(), nullptr, reference, loaded, reason, loaded_stats, SIZE_MAX));
  CHECK(loaded.stats.tracked_calls == loaded_stats.tracked_calls);
  CHECK(loaded.stats.tracked_collective_seconds == loaded_stats.tracked_collective_seconds);
  for (const size_t ceiling : {size_t(1), loaded_stats.maximum_native_bytes / 2}) {
    ImportedGeometry sentinel;
    sentinel.cells.emplace(999, OwnedCell{});
    sentinel.stats.requests = 123;
    BridgeStats refused;
    CHECK_FALSE(ImportGeometry(*config, donor.Fine(), nullptr, reference, sentinel, reason, refused,
                               world.rank == 0 ? ceiling : SIZE_MAX));
    CHECK_FALSE(reason.empty());
    CHECK(sentinel.cells.size() == 1);
    CHECK(sentinel.cells.count(999) == 1);
    CHECK(sentinel.original.empty());
    CHECK(sentinel.stats.requests == 123);
    CHECK(reference.original == original);
    CHECK(EncodeReference(reference) == immutable);
    CHECK(Snapshot(donor) == saved);
  }
  ReferenceState fresh;
  fresh.marker_names = {"sentinel"};
  ImportedGeometry no_output;
  BridgeStats rejected;
  CHECK_FALSE(ImportGeometry(*config, donor.Fine(), nullptr, fresh, no_output, reason, rejected,
                             world.rank == 0 ? 1 : SIZE_MAX));
  CHECK_FALSE(fresh.original);
  CHECK(fresh.marker_names == std::vector<std::string>{"sentinel"});
  CHECK(no_output.cells.empty());
  BridgeStats late;
  CHECK_FALSE(ImportGeometry(*config, donor.Fine(), nullptr, fresh, no_output, reason, late,
                             world.rank == 0 ? measured.maximum_native_bytes / 2 : SIZE_MAX));
  CHECK(late.points_seconds > 0);
  CHECK(late.cells_seconds > 0);
  CHECK_FALSE(fresh.original);
  CHECK(fresh.marker_names == std::vector<std::string>{"sentinel"});
  CHECK(no_output.cells.empty());
  ImportedGeometry retry;
  BridgeStats retry_stats;
  REQUIRE(ImportGeometry(*config, donor.Fine(), nullptr, reference, retry, reason, retry_stats, SIZE_MAX));
  CHECK(reference.original == original);
  CHECK(retry.original.size() == baseline.original.size());
  CHECK(Snapshot(donor) == saved);
  // Saturated caller storage must refuse before adding source buffers, without overflow or publication.
  CHECK_FALSE(ImportGeometry(*config, donor.Fine(), nullptr, reference, retry, reason, rejected, SIZE_MAX, SIZE_MAX));
  CHECK(retry.original.size() == baseline.original.size());
  CHECK(reference.original == original);
}

TEST_CASE("Native3D configured work and time ceilings preserve CFD and BDF donor state", "[NativeDefault3D][NativePolicyRuntime3D]") {
  World world;const int workers=world.size>1 ? world.size/2 : 1;
  auto cfg=[&](unsigned long budget,double seconds) {
    std::ostringstream options;
    options<<"SOLVER= EULER\nTIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n"
      <<"TIME_STEP= 0.01\nMAX_TIME= 0.02\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\n"
      <<"ADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
      <<"ADAP_UNSTEADY_METRIC= WINDOW_AVERAGE\nADAP_NATIVE_RANKS= "<<workers
      <<"\nADAP_NATIVE_MAX_WORK= "<<budget<<"\nADAP_NATIVE_MAX_TIME= "<<std::setprecision(17)<<seconds<<"\n";
    return MakeConfig(3,options.str());
  };
  auto config=cfg(workers,0);
  MeshSolution donor(config.get(),BoxMesh(3,1,false),0);
  auto snapshot=[&](const MeshSolution& mesh) {
    auto values=Snapshot(mesh);const auto* nodes=mesh.solver[MESH_0][FLOW_SOL]->GetNodes();
    for(unsigned long p=0;p<mesh.Fine().GetnPoint();++p)for(unsigned short k=0;k<5;++k) {
      values.push_back(SU2_TYPE::GetValue(nodes->GetSolution_time_n(p,k)));
      values.push_back(SU2_TYPE::GetValue(nodes->GetSolution_time_n1(p,k)));
    }
    return values;
  };
  auto metric=[](const MeshSolution& mesh) {
    su2activematrix values(mesh.Fine().GetnPointDomain(),6);
    for(unsigned long p=0;p<values.rows();++p)for(unsigned short k=0;k<6;++k)
      values(p,k)=(k==0 || k==3 || k==5) ? 4. : 0.;
    return values;
  };
  const auto saved=snapshot(donor);auto sensor=metric(donor);
  auto reference=std::make_shared<ReferenceState>();CNativeRemesher3D limited(reference);
  const auto refused=limited.Remesh(*config,donor.Fine(),sensor);
  CHECK(refused.status!=CRemeshResult::Status::COMPLETE);CHECK_FALSE(refused.onAccepted);
  CHECK_FALSE(reference->original);CHECK(snapshot(donor)==saved);
  const auto& failure=limited.Statistics();
  CHECK(CPassiveComm::Allreduce(failure.refine.accepted,CPassiveComm::Op::SUM)>0);
  if(world.rank<workers) { CHECK(failure.refine.stop==EngineStop::WorkBudget);CHECK(failure.refine.global_work<=uint64_t(workers)); }
  auto expired=cfg(2048,1e-12);CNativeRemesher3D timed(reference);
  MeshSolution timed_donor(expired.get(),BoxMesh(3,1,false),0);
  const auto timed_saved=snapshot(timed_donor);const auto timed_sensor=metric(timed_donor);
  const auto elapsed=timed.Remesh(*expired,timed_donor.Fine(),timed_sensor);
  CHECK(elapsed.status!=CRemeshResult::Status::COMPLETE);CHECK_FALSE(elapsed.onAccepted);
  CHECK_FALSE(reference->original);CHECK(snapshot(timed_donor)==timed_saved);
  auto generous=cfg(2048,1000);CNativeRemesher3D complete(reference);
  MeshSolution generous_donor(generous.get(),BoxMesh(3,1,false),0);
  const auto generous_saved=snapshot(generous_donor);const auto generous_sensor=metric(generous_donor);
  const auto good=complete.Remesh(*generous,generous_donor.Fine(),generous_sensor);
  REQUIRE(good.status==CRemeshResult::Status::COMPLETE);
  REQUIRE(good.beforeAcceptance);CHECK(good.beforeAcceptance());
  CHECK_FALSE(reference->original);CHECK(snapshot(generous_donor)==generous_saved);
  if(world.rank<workers) {
    const auto& stats=complete.Statistics();
    CHECK(stats.refine.global_work+stats.coarsen.global_work<=2048);
    CHECK(stats.coarsen.work_limit==2048-stats.refine.global_work);
  }
}


TEST_CASE("Native3D remeshes an externally frozen original sensor field without advancing CFD", "[.][NativeFrozenVolume3D]") {
  World world;
  const auto* configFile=std::getenv("SU2_NATIVE_FROZEN_CONFIG");
  const auto* sensorFile=std::getenv("SU2_NATIVE_FROZEN_SENSOR3D");
  REQUIRE(configFile);REQUIRE(sensorFile);
  class FrozenDriver : public CSinglezoneDriver {
   public:
    using CSinglezoneDriver::CSinglezoneDriver;
    CConfig& Config(){return *config_container[ZONE_0];}
    CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
    std::unique_ptr<CRemesher> Backend(){
      auto backend=MakeRemesher();const auto* selected=std::getenv("SU2_NATIVE_FROZEN_BL_PARAMETER");
      if(!selected)return backend;
      const auto* versionText=std::getenv("SU2_NATIVE_FROZEN_BL_VERSION");REQUIRE(versionText);
      const double parameter=std::stod(selected);const auto version=std::stoull(versionText);
      REQUIRE(parameter>=0);REQUIRE(parameter<=1);REQUIRE(version>0);REQUIRE(version<=27);REQUIRE(Config().GetAdap_BL_Budget());
      auto composition=[parameter,version](const CConfig& cfg,const ReferenceState& state,double core,EngineOptions& control)->MetricComposition {
        auto layers=CBoundaryLayerMetric::FromNativeReference(state,cfg);
        layers->ConfigureOuterNormal(SU2_TYPE::GetValue(cfg.GetAdap_BL_ProtectDistance()),SU2_TYPE::GetValue(cfg.GetAdap_BL_OuterGrowthMax()));
        for(uint64_t i=0;i<version;++i)layers->SelectOuterNormal(parameter);layers->FreezeOuterNormal();
        for(unsigned short b=0;b<cfg.GetnAdap_BL();++b) {
          const auto& layer=cfg.GetAdap_BL(b);const auto marker=std::find(state.marker_names.begin(),state.marker_names.end(),layer.marker);
          if(marker==state.marker_names.end())throw std::runtime_error("Frozen replay lacks its original BL marker.");
          const int id=int(marker-state.marker_names.begin());control.wall_heights[id]=SU2_TYPE::GetValue(layer.firstHeight);
          control.region_walls[id]={SU2_TYPE::GetValue(layer.firstHeight),SU2_TYPE::GetValue(layer.growth),SU2_TYPE::GetValue(layer.thickness)};
        }
        control.caller_bytes=transfer_memory::Add(control.caller_bytes,layers->StorageBytes());
        control.edge_breakpoints=[layers](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
        if(!CPassiveComm::Rank())std::cout<<std::setprecision(17)<<"FROZEN_PROFILE parameter="<<parameter<<" version="<<version<<" core="<<core<<'\n';
        return [layers,core](Point p,Tensor sensor){
          CBoundaryLayerMetric::Tensor m;m.m[0][0]=sensor.xx;m.m[1][1]=sensor.yy;m.m[2][2]=sensor.zz;
          m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;m.m[1][2]=m.m[2][1]=sensor.yz;
          const su2double x[]={p.x,p.y,p.z};layers->ApplyPoint(x,m,core,nullptr,false);
          return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
              SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
        };
      };
      // Exact construction replay has no unclipped family: an unmatched budget must reject, never rescale donors.
      return std::make_unique<CNativeRemesher3D>(nativeReference3D,EngineOptions{},std::move(composition));
    }
    ReferenceState& Reference(){return *nativeReference3D;}
  };
  FrozenDriver driver(const_cast<char*>(configFile),1,SU2_MPI::GetComm());
  auto& geometry=driver.Geometry();REQUIRE(geometry.GetnDim()==3);
  std::ifstream input(sensorFile,std::ios::binary);uint64_t count=0;input.read(reinterpret_cast<char*>(&count),8);
  REQUIRE(count>0);REQUIRE(count<=10000000);
  struct Record {std::array<double,3> x;std::array<double,9> m;};std::map<uint64_t,Record> original;
  for(uint64_t k=0;k<count;++k) {
    uint64_t id=0;Record r;input.read(reinterpret_cast<char*>(&id),8);
    input.read(reinterpret_cast<char*>(r.x.data()),3*8);input.read(reinterpret_cast<char*>(r.m.data()),9*8);
    REQUIRE(bool(input));REQUIRE(original.emplace(id,r).second);
  }
  REQUIRE(input.peek()==std::char_traits<char>::eof());
  su2activematrix sensor(geometry.GetnPointDomain(),6);
  for(unsigned long p=0;p<sensor.rows();++p) {
    const auto found=original.find(geometry.nodes->GetGlobalIndex(p));REQUIRE(found!=original.end());
    const auto& r=found->second;
    for(unsigned j=0;j<3;++j)REQUIRE(SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,j))==r.x[j]);
    const unsigned upper[6]={0,1,2,4,5,8};for(unsigned j=0;j<6;++j)sensor(p,j)=r.m[upper[j]];
  }
  REQUIRE(world.sum(uint64_t(sensor.rows()))==count);
  auto backend=driver.Backend();const auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  const auto* edgeLimit=std::getenv("SU2_NATIVE_EDGE_REUSE_LIMIT");
  if(const char* limit=edgeLimit ? edgeLimit : std::getenv("SU2_NATIVE_PREPARATION_LIMIT")) {
    const auto maximum=std::stoul(limit);REQUIRE(maximum>0);REQUIRE(maximum<=4096);
    ReferenceState reference=driver.Reference();ImportedGeometry imported;BridgeStats importStats;std::string reason;
    REQUIRE(ImportGeometry(driver.Config(),geometry,&sensor,reference,imported,reason,importStats,size_t(2)<<30));
    DonorImport source(world,true);REQUIRE(source.Build(imported.original,reason,size_t(2)<<30));
    double core=std::numeric_limits<double>::infinity();
    for(unsigned long p=0;p<sensor.rows();++p) {
      su2double m[3][3]={{sensor(p,0),sensor(p,1),sensor(p,2)},{sensor(p,1),sensor(p,3),sensor(p,4)},
          {sensor(p,2),sensor(p,4),sensor(p,5)}},frame[3][3],value[3],scratch[3];
      CBlasStructure::EigenDecomposition(m,frame,value,3,scratch);
      for(auto x:value)core=std::min(core,SU2_TYPE::GetValue(x));
    }
    core=CPassiveComm::Allreduce(core,CPassiveComm::Op::MIN,world.comm);
    auto layers=CBoundaryLayerMetric::FromNativeReference(reference,driver.Config());
    std::vector<std::array<double,15>> observations;const bool recordQueries=!edgeLimit;
    MetricComposition composition=[&](Point p,Tensor sensor) {
      CBoundaryLayerMetric::Tensor m;m.m[0][0]=sensor.xx;m.m[1][1]=sensor.yy;m.m[2][2]=sensor.zz;
      m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;m.m[1][2]=m.m[2][1]=sensor.yz;
      const su2double x[3]={p.x,p.y,p.z};layers->ApplyPoint(x,m,core,nullptr,false);
      const Tensor target{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
          SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
      if(recordQueries)observations.push_back({p.x,p.y,p.z,sensor.xx,sensor.xy,sensor.xz,sensor.yy,sensor.yz,sensor.zz,
          target.xx,target.xy,target.xz,target.yy,target.yz,target.zz});
      return target;
    };
    auto key=[](const Cell& cell){std::array<Id,4> ids;for(unsigned j=0;j<4;++j)ids[j]=cell.v[j].id;std::sort(ids.begin(),ids.end());return ids;};
    // Gather only a bounded prefix of keys, never all cells, for identical test coverage on1/2/4 ranks.
    std::set<std::array<Id,4>> local;
    std::set<int> wallMarkers;
    if(edgeLimit) {
      REQUIRE(maximum<=64);
      for(size_t m=0;m<reference.marker_names.size();++m)for(unsigned short b=0;b<driver.Config().GetnAdap_BL();++b)
        if(reference.marker_names[m]==driver.Config().GetAdap_BL(b).marker)wallMarkers.insert(m);
      REQUIRE_FALSE(wallMarkers.empty());
    }
    for(const auto& row:imported.cells) {
      if(edgeLimit && !std::any_of(row.second.physical.begin(),row.second.physical.end(),[&](const auto& face) {
        return face.present && wallMarkers.count(reference.original->Get(face.facet).marker);
      }))continue;
      local.insert(key(row.second.cell));if(local.size()>maximum)local.erase(std::prev(local.end()));
    }
    std::vector<std::vector<std::array<Id,4>>> send(world.size);send[0].assign(local.begin(),local.end());
    auto candidates=world.exchange(send);send.assign(world.size,{});
    if(!world.rank) {
      std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
      if(candidates.size()>maximum)candidates.resize(maximum);for(auto& bucket:send)bucket=candidates;
    }
    const auto admitted=world.exchange(send);std::set<std::array<Id,4>> wanted(admitted.begin(),admitted.end());
    Owned selected;for(const auto& row:imported.cells)if(wanted.count(key(row.second.cell)))selected.insert(row);
    REQUIRE(world.sum(uint64_t(selected.size()))==wanted.size());
    if(edgeLimit) {
      REQUIRE(wanted.size()==maximum);
      const MetricEdgeBreakpoints bands=[&](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
      const auto identity=std::make_shared<const FrozenMetricIdentity>();
      std::vector<const Cell*> ordered;for(const auto& row:selected)ordered.push_back(&row.second.cell);
      std::vector<PatchMetric> cold,retained;std::map<Edge,EdgeMeasure> reuse;
      const size_t slots=CPassiveComm::Allreduce(ordered.size(),CPassiveComm::Op::MAX,world.comm);
      std::array<double,3> elapsed{},geometryTime{},composeTime{};std::array<uint64_t,3> evaluations{},hits{},compositions{};
      std::ofstream rows("edges_rank"+std::to_string(world.rank)+".csv");
      rows<<std::setprecision(21)<<"cell,n0,n1,n2,n3,a,b,cold,reused,repeat,q,J,max_length\n";
      for(unsigned pass=0;pass<3;++pass) {
        const auto begin=world.seconds();
        for(size_t i=0;i<slots;++i) {
          const auto query=i<ordered.size()?std::vector<Cell>{*ordered[i]}:std::vector<Cell>{};
          auto donors=source.ImportMetricCells(query,size_t(2)<<30);INFO(donors.reason);REQUIRE(donors.valid);
          CLocalFailure failure;
          try {if(i<ordered.size()) {
            FrozenField target(std::move(donors.cells),composition,true,{},bands,identity);MetricStats stats;
            const auto measured=MeasureMetricCells(query,target,&stats,pass?&reuse:nullptr);
            evaluations[pass]+=stats.edge_evaluations;hits[pass]+=stats.edge_reused;
            const auto& field=target.Statistics();geometryTime[pass]+=field.edge_geometry_seconds;
            composeTime[pass]+=field.edge_composition_seconds;compositions[pass]+=field.compositions;
            if(!pass)cold.push_back(measured);
            else {
              const auto& original=cold.at(i);
              if(measured.minimum_mean_ratio!=original.minimum_mean_ratio ||
                 measured.minimum_scaled_jacobian!=original.minimum_scaled_jacobian || measured.maximum_length!=original.maximum_length)
                throw std::runtime_error("Cached M6 metric bounds differ from cold evaluation.");
              if(pass==1)retained.push_back(measured);
              for(const auto& edge:measured.edges) {
                if(edge.second.length!=original.edges.at(edge.first).length)
                  throw std::runtime_error("Cached M6 full-target edge differs from cold integration.");
                if(pass==2) {
                  rows<<ordered[i]->id;for(auto node:key(*ordered[i]))rows<<','<<node;
                  rows<<','<<edge.first[0]<<','<<edge.first[1]<<','<<original.edges.at(edge.first).length<<','
                      <<retained.at(i).edges.at(edge.first).length<<','<<edge.second.length<<','<<measured.minimum_mean_ratio<<','
                      <<measured.minimum_scaled_jacobian<<','<<measured.maximum_length<<'\n';
                }
              }
              for(const auto& edge:measured.edges)reuse[edge.first]=edge.second;
            }
          }}catch(const std::exception& error){failure.Set(2,0,error.what());}
          const auto rejected=world.elect(failure);INFO(rejected.message);REQUIRE_FALSE(rejected.any);
        }
        elapsed[pass]=world.seconds()-begin;
      }
      REQUIRE(evaluations[0]==6*selected.size());REQUIRE(evaluations[2]==0);REQUIRE(hits[2]==evaluations[0]);
      REQUIRE(compositions[2]==selected.size());
      rows.close();REQUIRE(bool(rows));
      std::ofstream out("edge_reuse_rank"+std::to_string(world.rank)+".json");out<<std::setprecision(17)
        <<"{\"ranks\":"<<world.size<<",\"rank\":"<<world.rank<<",\"cells\":"<<selected.size()<<",\"passes\":[";
      for(unsigned p=0;p<3;++p) {
        if(p)out<<',';out<<"{\"seconds\":"<<elapsed[p]<<",\"evaluations\":"<<evaluations[p]<<",\"reused\":"<<hits[p]
          <<",\"geometry_seconds\":"<<geometryTime[p]<<",\"composition_seconds\":"<<composeTime[p]
          <<",\"compositions\":"<<compositions[p]<<'}';
      }
      out<<"],\"exact_metric_match\":true,\"CFD_steps\":0,\"mesh_constructed\":false}\n";
      out.close();REQUIRE(bool(out));driver.Finalize();return;
    }
    BridgeStats prepared;std::vector<uint32_t> actual;
    const auto begin=world.seconds();REQUIRE(WorkingWeights(world,selected,source,actual,reason,prepared,composition));
    const auto elapsed=world.seconds()-begin;
    const auto retainedQueries=std::move(observations);observations.clear();
    // Historical fresh-import8/halve schedule, without production admission bookkeeping; compare exact weights/work.
    size_t next=0,batch=8;std::vector<uint32_t> cold;std::vector<const Cell*> ordered;
    for(const auto& row:selected)ordered.push_back(&row.second.cell);
    uint64_t coldImports=0,coldBuilds=0;const auto coldStart=world.seconds();
    while(world.maximum(next<ordered.size())) {
      std::vector<DonorRegion> regions;
      for(size_t i=next;i<std::min(ordered.size(),next+batch);++i) {
        const auto c=Centroid(*ordered[i]);regions.push_back({c.x,c.y,c.z,c.x,c.y,c.z});
        for(auto n:ordered[i]->v)regions.push_back({n.p.x,n.p.y,n.p.z,n.p.x,n.p.y,n.p.z});
      }
      auto patch=source.Import(regions,size_t(2)<<30);++coldImports;
      if(!patch.valid){REQUIRE(batch>1);batch=std::max(size_t(1),batch/2);continue;}
      CLocalFailure failure;
      try {if(!regions.empty()) {
        FrozenField field(std::move(patch.cells),composition,true);++coldBuilds;
        for(size_t i=next;i<std::min(ordered.size(),next+batch);++i)cold.push_back(WorkWeights({*ordered[i]},field).front());
      }}catch(const std::exception& error){failure.Set(2,0,error.what());}
      const auto rejected=world.elect(failure);INFO(rejected.message);REQUIRE_FALSE(rejected.any);
      next+=std::min(batch,ordered.size()-next);
    }
    const auto coldSeconds=world.seconds()-coldStart;REQUIRE(actual==cold);
    REQUIRE(observations==retainedQueries);CHECK(observations.size()==5*selected.size());
    CHECK(prepared.requests==5*selected.size());CHECK(prepared.compositions==prepared.requests);
    std::ofstream rows("preparation_weights_rank"+std::to_string(world.rank)+".csv");rows<<"cell,n0,n1,n2,n3,retained,cold\n";
    for(size_t i=0;i<actual.size();++i) {rows<<ordered[i]->id;for(auto id:key(*ordered[i]))rows<<','<<id;rows<<','<<actual[i]<<','<<cold[i]<<'\n';}
    rows.close();REQUIRE(bool(rows));
    std::ofstream out("preparation_rank"+std::to_string(world.rank)+".json");out<<std::setprecision(17)
        <<"{\"ranks\":"<<world.size<<",\"rank\":"<<world.rank<<",\"cells\":"<<selected.size()
        <<",\"seconds\":"<<elapsed<<",\"cold_seconds\":"<<coldSeconds<<",\"cold_imports\":"<<coldImports
        <<",\"cold_field_builds\":"<<coldBuilds<<",\"imports\":"<<prepared.batches<<",\"field_builds\":"<<prepared.field_builds
        <<",\"source_point_queries\":"<<prepared.source_point_queries<<",\"source_point_payloads\":"<<prepared.source_point_payloads
        <<",\"source_point_seconds\":"<<prepared.source_point_seconds<<",\"source_point_search_seconds\":"<<prepared.source_point_search_seconds
        <<",\"source_point_interpolation_seconds\":"<<prepared.source_point_interpolation_seconds
        <<",\"requests\":"<<prepared.requests<<",\"sensor_hits\":"<<prepared.hits<<",\"compositions\":"<<prepared.compositions
        <<",\"field_build_seconds\":"<<prepared.field_build_seconds<<",\"search_seconds\":"<<prepared.field_search_seconds
        <<",\"composition_seconds\":"<<prepared.composition_seconds<<",\"tracked_calls\":"<<prepared.tracked_calls
        <<",\"maximum_native_bytes\":"<<prepared.maximum_native_bytes
        <<",\"exact_weight_match\":true,\"exact_query_tensors_match\":true,\"CFD_steps\":0,\"mesh_constructed\":false}\n";
    out.close();REQUIRE(bool(out));driver.Finalize();return;
  }
  const auto start=SU2_MPI::Wtime();auto result=backend->Remesh(driver.Config(),geometry,sensor);
  const auto elapsed=CPassiveComm::Allreduce(SU2_MPI::Wtime()-start,CPassiveComm::Op::MAX);
  const auto& stats=native->Statistics();CHECK(stats.complete==(result.status==CRemeshResult::Status::COMPLETE));
  if(world.rank==0) {
    std::ofstream out("construction_result.json");out<<std::setprecision(17)
      <<"{\"accepted\":"<<(stats.complete?"true":"false")<<",\"seconds\":"<<elapsed
      <<",\"initial_cells\":"<<stats.initial_cells<<",\"final_cells\":"<<stats.final_cells
      <<",\"workers\":"<<stats.workers<<",\"normalization_accuracy_established\":false,\"CFD_steps\":0}\n";
    REQUIRE(bool(out));
  }
  if(stats.complete) {
    MeshSolution candidate(&driver.Config(),result.slices,0);
    CMeshOutput::WriteMesh(&driver.Config(),&candidate.Fine(),"frozen_geometry_accepted");
  }
  driver.Finalize();
}

TEST_CASE("Native first-row transactions replay original fields without solver initialization", "[.][NativeFirstRowReplay3D]") {
  World world;const auto* folder=std::getenv("SU2_NATIVE_FIRST_ROW_ROOT");REQUIRE(folder);
  const std::string root(folder);std::ifstream configFile(root+"/run.cfg");REQUIRE(configFile.good());
  CConfig config(configFile,SU2_COMPONENT::SU2_CFD,false);ReferenceState reference;
  REQUIRE(LoadReference(root+"/Coarse.su2.native_ref",reference));REQUIRE(reference.original);
  std::ifstream sensorFile(root+"/sensor_original.bin",std::ios::binary);uint64_t count=0;
  sensorFile.read(reinterpret_cast<char*>(&count),8);REQUIRE(count>0);REQUIRE(count<=10000000);
  struct Record {Point point;Tensor sensor;};std::map<Id,Record> nodes;double core=INFINITY;
  for(uint64_t i=0;i<count;++i) {
    Id id;std::array<double,3> x;std::array<double,9> m;
    sensorFile.read(reinterpret_cast<char*>(&id),8);sensorFile.read(reinterpret_cast<char*>(x.data()),24);
    sensorFile.read(reinterpret_cast<char*>(m.data()),72);REQUIRE(bool(sensorFile));
    REQUIRE(nodes.emplace(id,Record{{x[0],x[1],x[2]},{m[0],m[1],m[2],m[4],m[5],m[8]}}).second);
    if(id%world.size==unsigned(world.rank)) {
      su2double tensor[3][3]={{m[0],m[1],m[2]},{m[1],m[4],m[5]},{m[2],m[5],m[8]}},frame[3][3],value[3],scratch[3];
      CBlasStructure::EigenDecomposition(tensor,frame,value,3,scratch);
      for(auto v:value)core=std::min(core,SU2_TYPE::GetValue(v));
    }
  }
  REQUIRE(sensorFile.peek()==std::char_traits<char>::eof());core=CPassiveComm::Allreduce(core,CPassiveComm::Op::MIN,world.comm);
  REQUIRE(std::isfinite(core));REQUIRE(core>0);
  std::ifstream selection(root+"/working_cells.dat");std::string format;size_t nWorking=0;
  selection>>format>>nWorking;REQUIRE(format=="SU2_NATIVE_FIRST_ROW_REPLAY_1");REQUIRE(nWorking>0);REQUIRE(nWorking<=512);
  std::map<Id,size_t> wanted;for(size_t i=0;i<nWorking;++i){Id id;selection>>id;REQUIRE(wanted.emplace(id,i).second);}
  REQUIRE(selection.good());
  std::map<Face,Id> physical;
  for(const auto& face:reference.facets){Face key{face.v[0].id,face.v[1].id,face.v[2].id};std::sort(key.begin(),key.end());REQUIRE(physical.emplace(key,face.id).second);}
  std::ifstream mesh(root+"/Coarse.su2");std::string line;size_t elements=0;
  while(std::getline(mesh,line))if(line.find("NELEM=")==0){elements=std::stoull(line.substr(line.find('=')+1));break;}
  REQUIRE(elements>0);REQUIRE(elements<=10000000);std::vector<DonorCell> original;Owned owned;
  original.reserve(elements/world.size+1);
  for(size_t i=0;i<elements;++i) {
    REQUIRE(bool(std::getline(mesh,line)));std::istringstream row(line);unsigned type;std::array<Id,4> ids;row>>type;
    REQUIRE(type==TETRAHEDRON);for(auto& id:ids)row>>id;REQUIRE(bool(row));
    const auto selected=wanted.find(i);const bool input=selected!=wanted.end() && selected->second%world.size==unsigned(world.rank);
    const bool donor=(i+1)%world.size==unsigned(world.rank);
    if(!input && !donor)continue;
    DonorCell cell;cell.cell.id=i;
    for(unsigned j=0;j<4;++j){const auto& value=nodes.at(ids[j]);cell.cell.v[j]={ids[j],value.point};cell.sensor[j]=value.sensor;}
    const auto& v=cell.cell.v;
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0){std::swap(cell.cell.v[0],cell.cell.v[1]);std::swap(cell.sensor[0],cell.sensor[1]);}
    if(input) {
      OwnedCell current;current.cell=cell.cell;const auto& n=current.cell.v;
      const std::array<Face,4> faces{{{n[1].id,n[2].id,n[3].id},{n[0].id,n[3].id,n[2].id},{n[0].id,n[1].id,n[3].id},{n[0].id,n[2].id,n[1].id}}};
      for(unsigned f=0;f<4;++f){auto key=faces[f];std::sort(key.begin(),key.end());const auto found=physical.find(key);if(found!=physical.end())current.physical[f]={true,found->second};}
      REQUIRE(owned.emplace(i,current).second);
    }
    if(donor)original.push_back(std::move(cell));
  }
  REQUIRE(world.sum(uint64_t(original.size()))==elements);REQUIRE(world.sum(uint64_t(owned.size()))==nWorking);
  Id maximum_node=0,maximum_cell=0;for(const auto& row:owned){maximum_cell=std::max(maximum_cell,row.first);for(auto n:row.second.cell.v)maximum_node=std::max(maximum_node,n.id);}
  REQUIRE(CPassiveComm::Allreduce(maximum_node,CPassiveComm::Op::MAX,world.comm)==nodes.rbegin()->first);
  REQUIRE(CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm)==elements-1);
  const bool traceQueries=std::getenv("SU2_NATIVE_FIRST_ROW_TRACE");
  std::vector<std::array<double,15>> queryTrace;if(traceQueries)queryTrace.reserve(262144);
  if(std::getenv("SU2_NATIVE_FIRST_ROW_SOURCE_ALIAS"))for(auto& d:original)for(auto& n:d.cell.v)n.id+=(Id(1)<<32);
  auto layers=CBoundaryLayerMetric::FromNativeReference(reference,config);
  const MetricComposition composition=[&](Point p,Tensor sensor) {
    CBoundaryLayerMetric::Tensor m;m.m[0][0]=sensor.xx;m.m[0][1]=m.m[1][0]=sensor.xy;m.m[0][2]=m.m[2][0]=sensor.xz;
    m.m[1][1]=sensor.yy;m.m[1][2]=m.m[2][1]=sensor.yz;m.m[2][2]=sensor.zz;
    const su2double x[3]={p.x,p.y,p.z};layers->ApplyPoint(x,m,core,nullptr,false);
    const Tensor target{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
        SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
    if(traceQueries) {
      if(queryTrace.size()==262144)throw std::runtime_error("Frozen first-row query trace exceeds its bounded storage.");
      queryTrace.push_back({p.x,p.y,p.z,sensor.xx,sensor.xy,sensor.xz,sensor.yy,sensor.yz,sensor.zz,
          target.xx,target.xy,target.xz,target.yy,target.yz,target.zz});
    }
    return target;
  };
#include "fixtures/native_bulk_layer_probe.inc"
  EngineOptions options;options.timings=true;options.maximum_work=64;options.maximum_seconds=120;
  if(traceQueries)options.caller_bytes=size_t(32)<<20;
  const auto* batch=std::getenv("SU2_NATIVE_FIRST_ROW_BATCH");REQUIRE(batch);options.candidate_batch=std::stoul(batch);
  REQUIRE((options.candidate_batch==1 || options.candidate_batch==MaximumCandidateBatch));
  options.edge_breakpoints=[&](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
  for(unsigned short b=0;b<config.GetnAdap_BL();++b)for(size_t m=0;m<reference.marker_names.size();++m)
    if(reference.marker_names[m]==config.GetAdap_BL(b).marker)options.wall_heights[m]=SU2_TYPE::GetValue(config.GetAdap_BL(b).firstHeight);
  auto snapshot=[&]() {std::vector<char> bytes;RecordStream stream(bytes);
    for(const auto& row:owned){auto copy=row.second;stream(copy);}return bytes;};
  const auto before=snapshot(); // The original donor vector is borrowed const by Refine.
  const auto result=Refine(world,owned,original,*reference.original,composition,options);
  if(traceQueries) {
    const auto calls=queryTrace.size();std::sort(queryTrace.begin(),queryTrace.end());
    queryTrace.erase(std::unique(queryTrace.begin(),queryTrace.end()),queryTrace.end());
    std::ofstream trace("query_trace_rank"+std::to_string(world.rank)+".csv");trace<<std::setprecision(17);
    for(const auto& row:queryTrace){for(unsigned k=0;k<15;++k)trace<<(k?",":"")<<row[k];trace<<'\n';}
    trace.close();REQUIRE(bool(trace));std::ofstream counts("query_trace_rank"+std::to_string(world.rank)+".json");
    counts<<"{\"calls\":"<<calls<<",\"unique\":"<<queryTrace.size()<<"}\n";REQUIRE(bool(counts));
  }
  INFO(result.reason);CHECK(snapshot()==before);CHECK_FALSE(result.valid);CHECK(result.stats.stop==EngineStop::WorkBudget);CHECK(result.stats.global_work==64);
  CHECK(world.sum(uint64_t(result.stats.layer_published))==64);
  std::ofstream output("first_row_rank"+std::to_string(world.rank)+".json");const auto& s=result.stats;
  output<<std::setprecision(17)<<"{\"rank\":"<<world.rank<<",\"ranks\":"<<world.size<<",\"batch\":"<<options.candidate_batch<<",\"core\":"<<core
    <<",\"total_seconds\":"<<s.total_seconds<<",\"layer_seconds\":"<<s.layer_seconds<<",\"published\":"<<s.layer_published
    <<",\"batches\":"<<s.layer_batches<<",\"maximum_batch\":"<<s.layer_maximum_batch<<",\"donor_batches\":"<<s.layer_donor_batches
    <<",\"donor_retries\":"<<s.layer_donor_retries<<",\"metadata_seconds\":"<<s.layer_metadata_seconds<<",\"donor_seconds\":"<<s.layer_donor_seconds
    <<",\"geometry_seconds\":"<<s.layer_geometry_seconds<<",\"metric_seconds\":"<<s.layer_metric_seconds<<",\"publication_seconds\":"<<s.layer_publication_seconds
    <<",\"shape_screens\":"<<s.layer_shape_screens<<",\"shape_pruned\":"<<s.layer_shape_pruned
    <<",\"edge_evaluations\":"<<s.layer_edge_evaluations<<",\"edge_reused\":"<<s.layer_edge_reused<<",\"tracked_calls\":"<<s.tracked_calls
    <<",\"directory_elections\":"<<s.directory_elections<<",\"maximum_native_bytes\":"<<s.maximum_work_bytes
    <<",\"stop\":\""<<EngineStopName(s.stop)<<"\",\"reason\":"<<std::quoted(result.reason)
    <<",\"accepted_mesh\":false,\"CFD_steps\":0,\"global_work\":"<<s.global_work<<"}\n";output.close();REQUIRE(bool(output));
  // Root-only small inspectable subset, not a full adapted wing or accepted CFD grid.
  std::vector<std::vector<OwnedCell>> send(world.size);for(const auto& row:result.cells)send[0].push_back(row.second);
  const auto rows=world.exchange(send);
  if(!world.rank) {
    std::ofstream data("first_row_rejected.dat");data<<std::setprecision(17)<<rows.size()<<'\n';
    for(const auto& row:rows){data<<row.cell.id;for(auto n:row.cell.v)data<<' '<<n.id<<' '<<n.p.x<<' '<<n.p.y<<' '<<n.p.z;for(auto f:row.physical)data<<' '<<f.present<<' '<<f.facet;data<<'\n';}
    data.close();REQUIRE(bool(data));
  }
}
