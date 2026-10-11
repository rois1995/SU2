/*!
 * \file CNativeImport3D_tests.cpp
 * \brief Actual CFD-owner import, worker adaptation, reader return and solution/history transfer.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/output/CMeshOutput.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "../../../Common/include/geometry/meshreader/CSU2ASCIIMeshReaderFVM.hpp"
#include "../../../Common/include/geometry/meshreader/CCGNSMeshReaderFVM.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include <iomanip>
#include <numeric>
using namespace SU2Native3D;
using namespace transfer_test;
namespace {
Tensor Sensor(const su2double* p,double scale) {
  return {scale+.125*SU2_TYPE::GetValue(p[0]),.03125*SU2_TYPE::GetValue(p[0]),0,
          scale+.125*SU2_TYPE::GetValue(p[1]),0,scale+.125*SU2_TYPE::GetValue(p[2])};
}
std::vector<char> State(const CGeometry& geometry,const CSolver& solver) {
  std::vector<char> bytes;RecordStream encode(bytes);const auto* n=solver.GetNodes();
  for(unsigned long p=0;p<geometry.GetnPoint();++p)for(unsigned short k=0;k<solver.GetnVar();++k) {
    std::array<double,3> v{SU2_TYPE::GetValue(n->GetSolution(p,k)),SU2_TYPE::GetValue(n->GetSolution_time_n(p,k)),SU2_TYPE::GetValue(n->GetSolution_time_n1(p,k))};encode(v);
  }
  return bytes;
}
std::vector<char> State(const MeshSolution& mesh) {return State(mesh.Fine(),*mesh.solver[MESH_0][FLOW_SOL]);}
template<class Records> std::vector<char> Bytes(Records records) {
  std::vector<char> result;RecordStream encode(result);for(auto& record:records)encode(record);return result;
}
struct FlowRecord {
  Id id=0;Point point;std::array<double,15> values{};std::array<double,6> metric{};
  template<class S> void Fields(S& s){s(id,point.x,point.y,point.z,values,metric);}
};
std::vector<FlowRecord> Flow(World& world,const CGeometry& geometry,const CSolver& solver,const std::vector<passivedouble>& metric={}) {
  std::vector<FlowRecord> records;const auto* nodes=solver.GetNodes();
  REQUIRE(geometry.GetGlobal_nPointDomain()<=1024);  // Test-only bounded complete state gather.
  for(unsigned long p=0;p<geometry.GetnPointDomain();++p) {
    FlowRecord r;r.id=geometry.nodes->GetGlobalIndex(p);r.point={SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,0)),SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,1)),SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,2))};
    for(size_t k=0;k<5;++k){r.values[k]=SU2_TYPE::GetValue(nodes->GetSolution(p,k));r.values[5+k]=SU2_TYPE::GetValue(nodes->GetSolution_time_n(p,k));r.values[10+k]=SU2_TYPE::GetValue(nodes->GetSolution_time_n1(p,k));}
    if(!metric.empty())std::copy_n(metric.begin()+6*p,6,r.metric.begin());records.push_back(r);
  }
  return world.metadata(records);
}
void SaveStates(World& world,const std::string& name,const std::vector<FlowRecord>& old,const std::vector<FlowRecord>& fresh,double scale) {
  if(world.rank)return;
  std::ofstream out(name+"_states.json");out<<std::setprecision(17)<<"{\"sensor_scale\":"<<scale;
  auto rows=[&](const char* label,const std::vector<FlowRecord>& records,bool tensors){out<<",\""<<label<<"\":[";bool comma=false;
    for(auto r:records){if(comma)out<<',';comma=true;out<<"{\"id\":"<<r.id<<",\"point\":["<<r.point.x<<','<<r.point.y<<','<<r.point.z<<"],\"states\":[";
      for(size_t k=0;k<r.values.size();++k){if(k)out<<',';out<<r.values[k];}out<<']';
      if(tensors){out<<",\"metric\":[";for(size_t k=0;k<6;++k){if(k)out<<',';out<<r.metric[k];}out<<']';}out<<'}';}out<<']';};
  rows("donor",old,false);rows("accepted",fresh,true);out<<"}\n";CHECK(out.good());
}
void SaveStates(World& world,const std::string& name,const MeshSolution& donor,const MeshSolution& accepted,
                const std::vector<passivedouble>& metric,double scale) {
  SaveStates(world,name,Flow(world,donor.Fine(),*donor.solver[MESH_0][FLOW_SOL]),
             Flow(world,accepted.Fine(),*accepted.solver[MESH_0][FLOW_SOL],metric),scale);
}
void Save(World& world,const std::string& name,const Owned& local,const std::vector<DonorCell>& original,const ReferenceState& reference) {
  const auto cells=world.metadata([&]{std::vector<OwnedCell> v;for(auto r:local)v.push_back(r.second);return v;}());
  const auto source=world.metadata(original);if(world.rank)return;
  std::map<Id,Node> nodes;for(auto c:cells)for(auto n:c.cell.v)nodes.emplace(n.id,n);
  std::ofstream out(name+".json");out<<std::setprecision(17)<<"{\"nodes\":[";bool comma=false;
  auto node=[&](Node n){out<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<']';};
  for(auto r:nodes){if(comma)out<<',';comma=true;node(r.second);}out<<"],\"cells\":[";comma=false;
  auto faces=[](Cell c){auto v=c.v;return std::array<Face,4>{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};};
  for(auto c:cells){if(comma)out<<',';comma=true;out<<"{\"id\":"<<c.cell.id<<",\"nodes\":[";for(size_t k=0;k<4;++k){if(k)out<<',';out<<c.cell.v[k].id;}out<<"]}";}
  out<<"],\"surface\":[";comma=false;
  for(auto c:cells)for(size_t k=0;k<4;++k)if(c.physical[k].present){if(comma)out<<',';comma=true;auto f=faces(c.cell)[k];out<<"{\"facet\":"<<c.physical[k].facet<<",\"nodes\":["<<f[0]<<','<<f[1]<<','<<f[2]<<"]}";}
  out<<"],\"facets\":[";comma=false;
  for(auto f:reference.facets){if(comma)out<<',';comma=true;out<<"{\"id\":"<<f.id<<",\"marker\":"<<f.marker<<",\"vertices\":[";for(size_t k=0;k<3;++k){if(k)out<<',';node(f.v[k]);}out<<"]}";}
  out<<"],\"facet_groups\":[";std::map<Id,std::vector<Id>> groups;for(auto f:reference.facets)groups[reference.original->Canonical(f.id)].push_back(f.id);comma=false;
  for(auto g:groups){if(comma)out<<',';comma=true;out<<'[';for(size_t k=0;k<g.second.size();++k){if(k)out<<',';out<<g.second[k];}out<<']';}
  out<<"],\"marker_names\":[";for(size_t k=0;k<reference.marker_names.size();++k){if(k)out<<',';out<<'"'<<reference.marker_names[k]<<'"';}
  out<<"],\"original\":[";comma=false;
  for(auto d:source){if(comma)out<<',';comma=true;out<<"{\"vertices\":[";for(size_t k=0;k<4;++k){if(k)out<<',';auto n=d.cell.v[k];auto m=d.sensor[k];out<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<",["<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<"]]";}out<<"]}";}
  out<<"]}\n";CHECK(out.good());
}

auto Signature(World& world,const Owned& owned,const ReferenceState& reference) {
  std::vector<OwnedCell> local;for(auto r:owned)local.push_back(r.second);const auto all=world.metadata(local);
  using XYZ=std::array<double,3>;std::vector<std::array<XYZ,4>> volume;
  std::vector<std::pair<int,std::array<XYZ,3>>> boundary;
  for(auto r:all) {
    std::array<XYZ,4> corners;for(size_t i=0;i<4;++i){auto p=r.cell.v[i].p;corners[i]={p.x,p.y,p.z};}
    auto sorted=corners;std::sort(sorted.begin(),sorted.end());volume.push_back(sorted);
    for(size_t i=0;i<4;++i)if(r.physical[i].present){std::array<XYZ,3> face;size_t j=0;for(size_t k=0;k<4;++k)if(k!=i)face[j++]=corners[k];std::sort(face.begin(),face.end());boundary.emplace_back(reference.original->Get(r.physical[i].facet).marker,face);}
  }
  std::sort(volume.begin(),volume.end());std::sort(boundary.begin(),boundary.end());return std::make_pair(volume,boundary);
}
void OutputRoundTrip(World& world,const std::string& name,const CReaderSlices& slices,const Owned& adapted,
                     const ReferenceState& reference,double scale) {
  const auto expected=Signature(world,adapted,reference);
  std::vector<std::string> formats{"SU2"};
#ifdef HAVE_CGNS
  formats.push_back("CGNS");
#endif
  for(const auto& format:formats) {
    const auto stem=name+"_written_"+format,file=stem+(format=="CGNS"?".cgns":".su2");
    std::string tags;for(auto tag:reference.marker_names){if(!tags.empty())tags+=", ";tags+=tag;}
    std::stringstream options("SOLVER= EULER\nMGLEVEL= 0\nMESH_FORMAT= "+format+"\nMESH_OUT_FORMAT= "+format+"\nMESH_FILENAME= "+file+"\nMARKER_FAR= ("+tags+")\n");
    std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(options,SU2_COMPONENT::SU2_CFD,false);}
    MeshSolution output(config.get(),slices,0);{Mute mute;CMeshOutput::WriteMesh(config.get(),&output.Fine(),stem);}
    std::unique_ptr<CMeshReaderBase> reader;
    {Mute mute;if(format=="SU2")reader=std::make_unique<CSU2ASCIIMeshReaderFVM>(config.get(),0,1);
#ifdef HAVE_CGNS
    else reader=std::make_unique<CCGNSMeshReaderFVM>(config.get(),0,1);
#endif
    }
    MeshSolution loaded(config.get(),*reader,0);su2activematrix metric(loaded.Fine().GetnPointDomain(),6);
    for(unsigned long p=0;p<metric.rows();++p){auto m=Sensor(loaded.Fine().nodes->GetCoord(p),scale);std::array<double,6> values{m.xx,m.xy,m.xz,m.yy,m.yz,m.zz};for(size_t k=0;k<6;++k)metric(p,k)=values[k];}
    auto authority=reference;auto returned=ImportGeometry(*config,loaded.Fine(),&metric,authority);CHECK(authority.original==reference.original);
    CHECK(Signature(world,returned.cells,authority)==expected);Save(world,stem+"_reload",returned.cells,returned.original,authority);
  }
}
}
TEST_CASE("Native3D CFD bridge returns adapted readers and preserves affine BDF2 states", "[NativeBridge3D]") {
  World parent;const int m=GENERATE(1,2,3,4);if(m>parent.size)return;const bool conservative=GENERATE(false,true);
  auto config=MakeConfig(3,"SOLVER= EULER\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\nTIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n");
  auto current=std::make_unique<MeshSolution>(config.get(),BoxMesh(3,1,false),0);ReferenceState reference;
  // Remeshing follows the dual-time update: current and time_n both hold U^n.
  auto field=AffineFlow(3);auto* initial=current->solver[MESH_0][FLOW_SOL]->GetNodes();
  for(unsigned long p=0;p<current->Fine().GetnPoint();++p){su2double values[MAXVAR]{};field(current->Fine().nodes->GetCoord(p),values);for(unsigned short k=0;k<5;++k){initial->SetSolution(p,k,values[k]);initial->GetSolution_time_n()(p,k)=values[k];initial->GetSolution_time_n1()(p,k)=values[k]*.96875;}}
  std::shared_ptr<const PlanarReference> authority;std::vector<unsigned long> counts;
  for(int cycle=0;cycle<3;++cycle) {
    const double scale=cycle==1?1.:4.;const auto& geometry=current->Fine();const auto saved=State(*current);
    su2activematrix metric(geometry.GetnPointDomain(),6);
    for(unsigned long p=0;p<metric.rows();++p){auto m=Sensor(geometry.nodes->GetCoord(p),scale);std::array<double,6> values{m.xx,m.xy,m.xz,m.yy,m.yz,m.zz};for(size_t k=0;k<6;++k)metric(p,k)=values[k];}
    auto imported=ImportGeometry(*config,geometry,&metric,reference);const auto original_bytes=Bytes(imported.original);
    REQUIRE(imported.stats.total_seconds>0);REQUIRE(imported.original.size()==imported.cells.size());
    if(!authority)authority=reference.original;CHECK(reference.original==authority);
    DonorImport source(parent,true);std::string reason;REQUIRE(source.Build(imported.original,reason));
    std::vector<uint32_t> weights;BridgeStats weight_stats;REQUIRE(WorkingWeights(parent,imported.cells,source,weights,reason,weight_stats));
    CHECK(weight_stats.requests==5*imported.cells.size());
    auto working=imported.cells;PartitionStats partition;REQUIRE(Repartition(parent,working,weights,m,partition,reason));
    std::vector<std::vector<DonorCell>> send(parent.size);
    for(size_t i=0;i<imported.original.size();++i)send[partition.destinations[i]].push_back(imported.original[i]);
    const auto migrated=parent.exchange(send);Owned adapted;bool valid=true;std::string detail;
#ifdef HAVE_MPI
    MPI_Comm comm=MPI_COMM_NULL;MPI_Comm_split(parent.comm,parent.rank<m?0:MPI_UNDEFINED,parent.rank,&comm);
    if(comm!=MPI_COMM_NULL){World workers(comm);auto result=cycle==1?Coarsen(workers,working,migrated,*authority):Refine(workers,working,migrated,*authority);valid=result.valid;detail=result.reason;adapted.swap(result.cells);MPI_Comm_free(&comm);}
#else
    auto result=cycle==1?Coarsen(parent,working,migrated,*authority):Refine(parent,working,migrated,*authority);valid=result.valid;detail=result.reason;adapted.swap(result.cells);
#endif
    INFO("cycle="<<cycle<<", M="<<m<<", reason="<<detail);REQUIRE_FALSE(parent.maximum(!valid));
    CReaderSlices slices;BridgeStats reader_stats;REQUIRE(ReaderSlices(parent,adapted,imported.original,reference,slices,reason,reader_stats));
    CHECK(slices.nDim==3);CHECK(slices.nMetric==6);CHECK(reader_stats.compositions==0);CHECK(State(*current)==saved);
    CReaderSlices sentinel;sentinel.nDim=99;BridgeStats refused;
    CHECK_FALSE(ReaderSlices(parent,adapted,imported.original,reference,sentinel,reason,refused,{},parent.rank==0?1:SIZE_MAX));CHECK(sentinel.nDim==99);CHECK(State(*current)==saved);
    const auto name="bridge_"+std::string(conservative?"conservative":"barycentric")+"_m"+std::to_string(m)+"_s"+std::to_string(cycle);Save(parent,name,adapted,imported.original,reference);
    auto fresh=std::make_unique<MeshSolution>(config.get(),slices,0);const auto transported=slices.FetchPointMetric(fresh->Fine());
    REQUIRE(transported.size()==6*fresh->Fine().GetnPoint());
    for(unsigned long p=0;p<fresh->Fine().GetnPoint();++p){auto m=Sensor(fresh->Fine().nodes->GetCoord(p),scale);std::array<double,6> expected{m.xx,m.xy,m.xz,m.yy,m.yz,m.zz};for(size_t k=0;k<6;++k)CHECK(transported[6*p+k]==Approx(expected[k]).margin(1e-13));}
    if(cycle==0 && m==1 && !conservative) {
      // Query-hook control only: composed 3D edge integration/BL remeshing remains unsupported.
      uint64_t calls=0;MetricComposition composition=[&](Point p,Tensor sensor){++calls;sensor.zz=std::max(sensor.zz,4.+60.*std::max(0.,1.-8.*p.z));return sensor;};
      std::vector<uint32_t> composed_weights;BridgeStats sampled;
      REQUIRE(WorkingWeights(parent,adapted,source,composed_weights,reason,sampled,composition));
      CHECK(sampled.requests==5*adapted.size());CHECK(sampled.compositions==sampled.requests);CHECK(calls==sampled.requests);
      CHECK(CPassiveComm::Allreduce(sampled.hits,CPassiveComm::Op::SUM,parent.comm)>0);
      CReaderSlices composed;BridgeStats returned;REQUIRE(ReaderSlices(parent,adapted,imported.original,reference,composed,reason,returned,composition));
      CHECK(composed.coord==slices.coord);CHECK(composed.elemRows==slices.elemRows);CHECK(composed.firstPoint==slices.firstPoint);
      CHECK(returned.compositions==returned.requests);CHECK(calls==sampled.requests+returned.requests);
      const auto tensors=composed.FetchPointMetric(fresh->Fine());REQUIRE(tensors.size()==transported.size());
      for(unsigned long p=0;p<fresh->Fine().GetnPoint();++p){const auto* x=fresh->Fine().nodes->GetCoord(p);auto tensor=Sensor(x,scale);tensor.zz=std::max(tensor.zz,4.+60.*std::max(0.,1.-8.*SU2_TYPE::GetValue(x[2])));std::array<double,6> expected{tensor.xx,tensor.xy,tensor.xz,tensor.yy,tensor.yz,tensor.zz};for(size_t k=0;k<6;++k)CHECK(tensors[6*p+k]==Approx(expected[k]).margin(1e-13));}
      CHECK(Bytes(imported.original)==original_bytes);CHECK(State(*current)==saved);
    }
    std::unique_ptr<CSolutionTransfer> transfer;if(conservative)transfer=std::make_unique<CConservativeTransfer>();else transfer=std::make_unique<CBarycentricTransfer>();
    {Mute mute;transfer->Transfer(config.get(),current->Donor(),fresh->geometry,fresh->solver);}
    const auto* nodes=fresh->solver[MESH_0][FLOW_SOL]->GetNodes();double error=0;
    for(unsigned long p=0;p<fresh->Fine().GetnPoint();++p){su2double values[MAXVAR]{};field(fresh->Fine().nodes->GetCoord(p),values);for(unsigned short k=0;k<5;++k){error=std::max(error,RelDiff(nodes->GetSolution(p,k),values[k]));error=std::max(error,RelDiff(nodes->GetSolution_time_n(p,k),values[k]));error=std::max(error,RelDiff(nodes->GetSolution_time_n1(p,k),values[k]*.96875));}}
    CHECK(CPassiveComm::Allreduce(error,CPassiveComm::Op::MAX,parent.comm)<2e-10);CHECK(State(*current)==saved);
    CHECK(Bytes(imported.original)==original_bytes);SaveStates(parent,name,*current,*fresh,transported,scale);
    if(!conservative)OutputRoundTrip(parent,name,slices,adapted,reference,scale);
    counts.push_back(slices.nElemGlobal);
    std::ostringstream log;log<<std::setprecision(17)<<"BRIDGE rank="<<parent.rank<<" ranks="<<parent.size<<" workers="<<m<<" conservative="<<conservative<<" cycle="<<cycle<<" cells="<<slices.nElemGlobal<<" points="<<slices.nPointGlobal<<" state_error="<<error<<" import_s="<<imported.stats.total_seconds<<" weights_s="<<weight_stats.total_seconds<<" reader_s="<<reader_stats.total_seconds<<'\n';const auto text=log.str();const auto b=CPassiveComm::Allgatherv(std::vector<char>(text.begin(),text.end()),nullptr,parent.comm);if(!parent.rank)std::cout.write(b.data(),b.size());
    current.swap(fresh);
  }
  REQUIRE(counts.size()==3);CHECK(counts[1]<counts[0]);CHECK(counts[2]>counts[1]);
}
TEST_CASE("Native3D import binding checks finite oriented triangles", "[NativeBridge3D]") {
  PlanarReference reference({{0,0,{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}}}}}});
  std::array<Point,3> f{{{0,0,0},{.5,0,0},{0,.5,0}}};CHECK(reference.ContainsFace(0,f));
  std::swap(f[0],f[1]);CHECK_FALSE(reference.ContainsFace(0,f));std::swap(f[0],f[1]);
  f[1]={2,0,0};CHECK_FALSE(reference.ContainsFace(0,f));f[1]={.5,0,std::ldexp(1.,-100)};CHECK_FALSE(reference.ContainsFace(0,f));
}

TEST_CASE("Shared adaptation geometry fixture retains 2D transfer histories", "[NativeBridge2D]") {
  const bool conservative=GENERATE(false,true);
  auto config=MakeConfig(2,"SOLVER= EULER\nTIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n");
  MeshSolution donor(config.get(),BoxMesh(2,4,false),1);const auto field=AffineFlow(2);auto* old=donor.solver[MESH_0][FLOW_SOL]->GetNodes();
  for(unsigned long p=0;p<donor.Fine().GetnPoint();++p){su2double values[MAXVAR]{};field(donor.Fine().nodes->GetCoord(p),values);for(unsigned short k=0;k<4;++k){old->SetSolution(p,k,values[k]);old->GetSolution_time_n()(p,k)=values[k];old->GetSolution_time_n1()(p,k)=values[k]*.96875;}}
  // Match existing transfer fixtures; six-point donors can leave CFD ranks empty.
  const auto saved=State(donor);MeshSolution target(config.get(),BoxMesh(2,6,false),1);
  std::unique_ptr<CSolutionTransfer> transfer;if(conservative)transfer=std::make_unique<CConservativeTransfer>();else transfer=std::make_unique<CBarycentricTransfer>();
  {Mute mute;transfer->Transfer(config.get(),donor.Donor(),target.geometry,target.solver);}
  const auto* nodes=target.solver[MESH_0][FLOW_SOL]->GetNodes();double error=0;
  for(unsigned long p=0;p<target.Fine().GetnPoint();++p){su2double values[MAXVAR]{};field(target.Fine().nodes->GetCoord(p),values);for(unsigned short k=0;k<4;++k){error=std::max(error,RelDiff(nodes->GetSolution(p,k),values[k]));error=std::max(error,RelDiff(nodes->GetSolution_time_n(p,k),values[k]));error=std::max(error,RelDiff(nodes->GetSolution_time_n1(p,k),values[k]*.96875));}}
  CHECK(CPassiveComm::Allreduce(error,CPassiveComm::Op::MAX)<2e-10);CHECK(State(donor)==saved);
}

namespace {
class TetraDriver : public CSinglezoneDriver {
 public:
  using CSinglezoneDriver::CSinglezoneDriver;
  using CSinglezoneDriver::WriteAdaptedMesh;
  using CSinglezoneDriver::AdaptedMeshName;
  CConfig& Config(){return *config_container[ZONE_0];}
  CGeometry& Geometry(){return *geometry_container[ZONE_0][INST_0][MESH_0];}
  CSolver& Solver(){return *solver_container[ZONE_0][INST_0][MESH_0][FLOW_SOL];}
  std::unique_ptr<CRemesher> Backend(){return MakeRemesher();}
  std::shared_ptr<ReferenceState> Reference(){return nativeReference3D;}
};
}
TEST_CASE("Native3D immutable reference encoding rejects malformed authority", "[NativeReference3D]") {
  World world;auto config=MakeConfig(3,"SOLVER= EULER\n");MeshSolution cube(config.get(),BoxMesh(3,1,false),0);
  ReferenceState original;ImportGeometry(*config,cube.Fine(),nullptr,original);
  const auto text=EncodeReference(original);auto copy=DecodeReference(text);CHECK(EncodeReference(copy)==text);
  CHECK(copy.original!=original.original);CHECK(copy.facets.size()==original.facets.size());
  CHECK_THROWS(EncodeReference(ReferenceState{}));CHECK_THROWS(DecodeReference(text+"junk"));
  CHECK_THROWS(DecodeReference(std::string(ReferenceFileLimit3D+1,'x')));
  auto wrong=text;wrong.replace(wrong.find("3D 1"),4,"3D 2");CHECK_THROWS(DecodeReference(wrong));
  wrong=text;wrong.replace(wrong.find('\n')+1,wrong.find('\n',wrong.find('\n')+1)-wrong.find('\n')-1,"0 0");CHECK_THROWS(DecodeReference(wrong));
  auto broken=original;broken.facets.pop_back();CHECK_THROWS(EncodeReference(broken));
  broken=original;broken.marker_names[1]=broken.marker_names[0];CHECK_THROWS(EncodeReference(broken));
  broken=original;broken.facets[0].v[0].p.x=std::numeric_limits<double>::infinity();CHECK_THROWS(EncodeReference(broken));
  WriteReference("original.native_ref",original);ReferenceState loaded;CHECK(LoadReference("original.native_ref",loaded));CHECK(EncodeReference(loaded)==text);
  CHECK_FALSE(LoadReference("nonexistent.native_ref",loaded));CHECK(EncodeReference(loaded)==text);
}
TEST_CASE("Native3D driver publishes only accepted mesh and transfers affine BDF2 states", "[NativeRuntime3D]") {
  World world;const int m=GENERATE(1,2,3,4);if(m>world.size)return;
  const bool conservative=GENERATE(false,true),detail=GENERATE(false,true);
  const auto name="runtime_"+std::string(conservative?"conservative":"barycentric")+"_m"+std::to_string(m)+"_d"+std::to_string(detail);
  if(!world.rank){simplex_test::WriteSU2Mesh(BoxMesh(3,1,false),name+".su2");std::ofstream cfg(name+".cfg");
    cfg<<"SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\nMESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\n"
       <<"MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n"
       <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<m<<"\nWRT_PERFORMANCE= "<<(detail?"YES":"NO")<<"\n"
       <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
       <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= SU2\nMESH_OUT_FILENAME= "<<name<<"_mesh\nCONV_FILENAME= "<<name<<"_history\n";}
  SU2_MPI::Barrier(SU2_MPI::GetComm());const auto cfg=name+".cfg";
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,SU2_MPI::GetComm());
  const auto field=AffineFlow(3);auto* initial=driver->Solver().GetNodes();
  for(unsigned long p=0;p<driver->Geometry().GetnPoint();++p){su2double values[MAXVAR]{};field(driver->Geometry().nodes->GetCoord(p),values);
    for(unsigned short k=0;k<5;++k){initial->SetSolution(p,k,values[k]);initial->GetSolution_time_n()(p,k)=values[k];initial->GetSolution_time_n1()(p,k)=values[k]*.96875;}}
  auto backend=driver->Backend();auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);CHECK_FALSE(driver->Reference()->original);
  std::vector<uint64_t> counts;std::string authority;
  std::unique_ptr<CSolutionTransfer> transfer;if(conservative)transfer=std::make_unique<CConservativeTransfer>();else transfer=std::make_unique<CBarycentricTransfer>();
  for(int cycle=0;cycle<3;++cycle){const double scale=cycle==1?1.:4.;auto& geometry=driver->Geometry();
    const auto before=State(geometry,driver->Solver());const auto old=Flow(world,geometry,driver->Solver());
    su2activematrix metric(geometry.GetnPointDomain(),6);
    for(unsigned long p=0;p<metric.rows();++p){auto t=Sensor(geometry.nodes->GetCoord(p),scale);std::array<double,6> a{t.xx,t.xy,t.xz,t.yy,t.yz,t.zz};for(size_t k=0;k<6;++k)metric(p,k)=a[k];}
    if(cycle==0){EngineOptions denied;denied.memory_ceiling=world.rank==0?1:SIZE_MAX;CNativeRemesher3D rejected(driver->Reference(),denied);
      const auto bad=rejected.Remesh(driver->Config(),geometry,metric);CHECK(bad.status!=CRemeshResult::Status::COMPLETE);CHECK_FALSE(bad.onAccepted);
      CHECK_FALSE(driver->Reference()->original);CHECK(State(geometry,driver->Solver())==before);}
    ReferenceState donor_reference;if(driver->Reference()->original)donor_reference=*driver->Reference();
    auto donor=ImportGeometry(driver->Config(),geometry,&metric,donor_reference);
    const auto result=backend->Remesh(driver->Config(),geometry,metric);INFO("cycle="<<cycle<<", M="<<m<<", detailed="<<detail);
    REQUIRE(result.status==CRemeshResult::Status::COMPLETE);CHECK(State(geometry,driver->Solver())==before);
    if(cycle==0)CHECK_FALSE(driver->Reference()->original);else CHECK(EncodeReference(*driver->Reference())==authority);
    const auto event=name+"_s"+std::to_string(cycle);const auto summary=native->Statistics();
    CHECK(summary.complete);CHECK(summary.detailed_timings==detail);CHECK(summary.total_seconds>0);CHECK(summary.other_seconds>=-1e-12);
    if(!detail){CHECK(summary.weights.field_build_seconds==0);CHECK(summary.returned.field_build_seconds==0);CHECK(summary.refine.field_query_seconds==0);CHECK(summary.coarsen.field_query_seconds==0);}
    const auto sum=std::accumulate(summary.phases.begin(),summary.phases.end(),summary.other_seconds);CHECK(sum==Approx(summary.total_seconds).margin(1e-12));
    driver->ReplaceMesh(result,*transfer);REQUIRE(driver->Reference()->original);if(cycle==0)authority=EncodeReference(*driver->Reference());
    CHECK(EncodeReference(*driver->Reference())==authority);result.onAccepted();result.onAccepted();CHECK(EncodeReference(*driver->Reference())==authority);
    const auto transported=result.slices.FetchPointMetric(driver->Geometry());const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);
    SaveStates(world,event,old,fresh,scale);double error=0;
    for(auto row:fresh){su2double x[3]{row.point.x,row.point.y,row.point.z},expected[MAXVAR]{};field(x,expected);
      for(size_t k=0;k<5;++k){error=std::max(error,RelDiff(row.values[k],expected[k]));error=std::max(error,RelDiff(row.values[5+k],expected[k]));error=std::max(error,RelDiff(row.values[10+k],expected[k]*.96875));}}
    CHECK(error<2e-10);CHECK(transported.size()==6*driver->Geometry().GetnPoint());
    su2activematrix accepted_metric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<accepted_metric.rows();++p)for(size_t k=0;k<6;++k)accepted_metric(p,k)=transported[6*p+k];
    auto retained=*driver->Reference();auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&accepted_metric,retained);
    Save(world,event,accepted.cells,donor.original,retained);counts.push_back(result.slices.nElemGlobal);
    if(!detail && !conservative){driver->WriteAdaptedMesh(cycle+1,cycle+1);ReferenceState loaded;
      CHECK(LoadReference(driver->AdaptedMeshName(cycle+1,cycle+1)+".su2.native_ref",loaded));CHECK(EncodeReference(loaded)==authority);
      OutputRoundTrip(world,event,result.slices,accepted.cells,retained,scale);}
  }
  REQUIRE(counts.size()==3);CHECK(counts[1]<counts[0]);CHECK(counts[2]>counts[1]);
  {Mute mute;driver->Finalize();driver.reset();}
}

TEST_CASE("Native3D actual metric-to-replacement lifecycle profiling preserves fixed work", "[NativeLifecycle3D]") {
  World world;
  const int workers = GENERATE(1, 2, 4);
  if (workers > world.size) return;
  const bool detail = GENERATE(false, true);
  // Identical fresh initial mesh/field per repetition; later events of a changing mesh are a different workload.
  for (int repeat = 0; repeat < 3; ++repeat) {
    const auto name = "lifecycle_m" + std::to_string(workers) + "_d" + std::to_string(detail) + "_r" + std::to_string(repeat);
    if (!world.rank) {
      simplex_test::WriteSU2Mesh(BoxMesh(3, 1, false), name + ".su2");
      std::ofstream cfg(name + ".cfg");
      cfg << "SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\nMESH_FORMAT= SU2\nMESH_FILENAME= " << name << ".su2\n"
          << "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n"
          << "COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
          << "ADAP_HMIN= 0.5\nADAP_HMAX= 0.5\nADAP_COMPLEXITY= 8\nADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= " << workers << "\n"
          << "WRT_PERFORMANCE= " << (detail ? "YES" : "NO") << "\nMGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
          << "TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
          << "OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= SU2\nMESH_OUT_FILENAME= " << name << "_mesh\nCONV_FILENAME= " << name << "_history\n";
    }
    SU2_MPI::Barrier(SU2_MPI::GetComm());
    const auto cfg = name + ".cfg";
    class ProfileDriver : public TetraDriver {
     public:
      using TetraDriver::TetraDriver;
      std::vector<DonorCell>* observed_source = nullptr;
      void ComputeMetric() override {
        CSinglezoneDriver::ComputeMetric();
        const auto& metric = Solver().GetNodes()->GetMetric();
        for (unsigned long p = 0; p < Geometry().GetnPointDomain(); ++p)
          for (size_t k = 0; k < 6; ++k) CHECK(metric(p,k) == Approx((k==0 || k==3 || k==5) ? 4. : 0.).margin(1e-12));
        REQUIRE(observed_source);
        // Small-fixture observation only: record actual computed owner/halo tensors for the independent audit.
        for (auto& donor : *observed_source)
          for (size_t k=0;k<4;++k) {
            unsigned long p=0;
            while (p<Geometry().GetnPoint() && Geometry().nodes->GetGlobalIndex(p)!=donor.cell.v[k].id) ++p;
            REQUIRE(p<Geometry().GetnPoint());
            donor.sensor[k]={metric(p,0),metric(p,1),metric(p,2),metric(p,3),metric(p,4),metric(p,5)};
          }
      }
    };
    auto driver = std::make_unique<ProfileDriver>(const_cast<char*>(cfg.c_str()), 1, SU2_MPI::GetComm());
    const auto field = AffineFlow(3);
    auto* initial = driver->Solver().GetNodes();
    for (unsigned long p = 0; p < driver->Geometry().GetnPoint(); ++p) {
      su2double values[MAXVAR]{};
      field(driver->Geometry().nodes->GetCoord(p), values);
      for (size_t k = 0; k < 5; ++k) {
        initial->SetSolution(p,k,values[k]); initial->GetSolution_time_n()(p,k)=values[k];
        initial->GetSolution_time_n1()(p,k)=values[k]*.96875;
      }
    }
    su2activematrix prescribed(driver->Geometry().GetnPointDomain(), 6);
    for (unsigned long p=0;p<prescribed.rows();++p)
      for (size_t k=0;k<6;++k) prescribed(p,k)=(k==0 || k==3 || k==5) ? 4. : 0.;
    ReferenceState original;
    auto donor=ImportGeometry(driver->Config(),driver->Geometry(),&prescribed,original);
    driver->observed_source=&donor.original;
    CBarycentricTransfer transfer;
    driver->AdaptMesh(transfer);
    REQUIRE(driver->Reference()->original);
    double error=0;
    for (const auto& row : Flow(world,driver->Geometry(),driver->Solver())) {
      su2double x[3]{row.point.x,row.point.y,row.point.z},expected[MAXVAR]{};field(x,expected);
      for(size_t k=0;k<5;++k) {
        error=std::max(error,RelDiff(row.values[k],expected[k]));
        error=std::max(error,RelDiff(row.values[5+k],expected[k]));
        error=std::max(error,RelDiff(row.values[10+k],expected[k]*.96875));
      }
    }
    CHECK(error<2e-10);
    su2activematrix accepted_metric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<accepted_metric.rows();++p)
      for(size_t k=0;k<6;++k) accepted_metric(p,k)=(k==0 || k==3 || k==5) ? 4. : 0.;
    auto retained=*driver->Reference();
    auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&accepted_metric,retained);
    Save(world,name,accepted.cells,donor.original,retained);
    driver->WriteAdaptedMesh(1,1);
    {Mute mute;driver->Finalize();driver.reset();}
  }
}
