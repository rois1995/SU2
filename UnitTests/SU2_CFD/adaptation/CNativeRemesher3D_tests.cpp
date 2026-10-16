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
    std::unique_ptr<CRemesher> Backend(){return MakeRemesher();}
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
