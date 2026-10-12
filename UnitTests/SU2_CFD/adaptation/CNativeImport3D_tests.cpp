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
#include "../../../Common/include/adaptation/CNativeRuntimeProfile3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include <iomanip>
#include <cstdlib>
#include <numeric>
using namespace SU2Native3D;
using namespace transfer_test;
namespace {
Tensor Sensor(const su2double* p,double scale) {
  return {scale+.125*SU2_TYPE::GetValue(p[0]),.03125*SU2_TYPE::GetValue(p[0]),0,
          scale+.125*SU2_TYPE::GetValue(p[1]),0,scale+.125*SU2_TYPE::GetValue(p[2])};
}
CSimplexMesh WedgeMesh(int n) {
  REQUIRE((n==2 || n==4 || n==8));
  CSimplexMesh mesh;mesh.nDim=3;
  std::map<std::array<int,3>,unsigned long> ids;
  for(int k=0;k<=n;++k)for(int j=0;j<=n;++j)for(int i=0;i+j<=n;++i) {
    ids[{i,j,k}]=mesh.GetnPoint();
    mesh.coord.insert(mesh.coord.end(),{double(i)/n,double(j)/n,double(k)/n});
  }
  const unsigned long stride=(n+1)*(n+2)/2;
  std::vector<std::array<unsigned long,3>> base;
  for(int j=0;j<n;++j)for(int i=0;i+j<n;++i) {
    base.push_back({ids.at({i,j,0}),ids.at({i+1,j,0}),ids.at({i,j+1,0})});
    if(i+j+1<n)base.push_back({ids.at({i+1,j,0}),ids.at({i+1,j+1,0}),ids.at({i,j+1,0})});
  }
  auto point=[&](unsigned long id){return Point{mesh.coord[3*id],mesh.coord[3*id+1],mesh.coord[3*id+2]};};
  std::map<std::array<unsigned long,3>,unsigned> faces;
  for(int k=0;k<n;++k)for(auto triangle:base) {
    // Sorted base IDs choose the same diagonal on both sides of every shared prism face.
    std::sort(triangle.begin(),triangle.end());
    const auto a=triangle[0]+k*stride,b=triangle[1]+k*stride,c=triangle[2]+k*stride;
    const std::array<std::array<unsigned long,4>,3> tets{{{a,b,c,c+stride},{a,b,b+stride,c+stride},{a,b+stride,a+stride,c+stride}}};
    for(auto tet:tets) {
      const auto volume=Orientation(point(tet[0]),point(tet[1]),point(tet[2]),point(tet[3]));
      REQUIRE(volume!=0);if(volume<0)std::swap(tet[0],tet[1]);
      mesh.elem.insert(mesh.elem.end(),tet.begin(),tet.end());
      for(size_t omit=0;omit<4;++omit) {
        std::array<unsigned long,3> face;size_t at=0;
        for(size_t v=0;v<4;++v)if(v!=omit)face[at++]=tet[v];
        std::sort(face.begin(),face.end());++faces[face];
      }
    }
  }
  CHECK(mesh.GetnElem()==3*n*n*n);mesh.elemRef.assign(mesh.GetnElem(),0);
  std::map<std::string,std::vector<unsigned long>> boundary;
  for(const auto& row:faces) {
    REQUIRE((row.second==1 || row.second==2));if(row.second!=1)continue;
    auto on=[&](auto predicate){return std::all_of(row.first.begin(),row.first.end(),[&](unsigned long id){return predicate(point(id));});};
    std::string marker;
    if(on([](Point p){return p.z==0;})) {
      marker=on([](Point p){return p.x<=.5;})?"z_minus_a":"z_minus_b";
      if(marker=="z_minus_b")REQUIRE(on([](Point p){return p.x>=.5;}));
    } else if(on([](Point p){return p.z==1;}))marker="z_plus";
    else if(on([](Point p){return p.x==0;}))marker="x_minus";
    else if(on([](Point p){return p.y==0;}))marker="y_minus";
    else {REQUIRE(on([](Point p){return p.x+p.y==1;}));marker="wedge_oblique";}
    auto& nodes=boundary[marker];nodes.insert(nodes.end(),row.first.begin(),row.first.end());
  }
  REQUIRE(boundary.size()==6);
  for(auto& row:boundary){CSimplexMesh::Marker marker;marker.name=row.first;marker.elem=std::move(row.second);mesh.markers.push_back(std::move(marker));}
  return mesh;
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
                     const ReferenceState& reference,double scale,
                     std::function<Tensor(const su2double*,double)> sensor=Sensor,
                     const std::vector<DonorCell>* target_source=nullptr) {
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
    if(target_source) {
      // A nonlinear sensor must retain its transported nodes and original P1 target across output checks.
      const auto transported=slices.FetchPointMetric(loaded.Fine());
      REQUIRE(transported.size()==6*loaded.Fine().GetnPoint());
      for(unsigned long p=0;p<metric.rows();++p)for(size_t k=0;k<6;++k)metric(p,k)=transported[6*p+k];
    } else {
    for(unsigned long p=0;p<metric.rows();++p){auto m=sensor(loaded.Fine().nodes->GetCoord(p),scale);std::array<double,6> values{m.xx,m.xy,m.xz,m.yy,m.yz,m.zz};for(size_t k=0;k<6;++k)metric(p,k)=values[k];}
    }
    auto authority=reference;auto returned=ImportGeometry(*config,loaded.Fine(),&metric,authority);CHECK(authority.original==reference.original);
    CHECK(Signature(world,returned.cells,authority)==expected);Save(world,stem+"_reload",returned.cells,returned.original,authority);
    if(target_source)Save(world,stem+"_reload_target",returned.cells,*target_source,authority);
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
    CHECK(weight_stats.requests==weight_stats.hits+weight_stats.misses+weight_stats.invalid_queries);
    CHECK(weight_stats.failures==0);CHECK(weight_stats.evictions<=weight_stats.misses);
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
      for(bool detail : {false,true}) {
        uint64_t attempted=0;
        MetricComposition failing=[&](Point,Tensor)->Tensor {
          ++attempted;throw std::runtime_error("Injected bridge query composition failure.");
        };
        std::vector<uint32_t> preserved{123};BridgeStats failed_weights;
        const auto weight_bytes=parent.bytes_sent;const auto weight_calls=parent.collective_calls;
        const double weight_seconds=parent.collective_seconds;
        CHECK_FALSE(WorkingWeights(parent,adapted,source,preserved,reason,failed_weights,failing,SIZE_MAX,0,detail));
        CHECK(failed_weights.tracked_bytes==parent.bytes_sent-weight_bytes);
        CHECK(failed_weights.tracked_calls==parent.collective_calls-weight_calls);
        CHECK(failed_weights.tracked_collective_seconds==Approx(parent.collective_seconds-weight_seconds).margin(1e-12));
        CHECK(preserved==std::vector<uint32_t>{123});CHECK_FALSE(reason.empty());
        CHECK(failed_weights.requests==failed_weights.hits+failed_weights.misses+failed_weights.invalid_queries);
        CHECK(failed_weights.failures==attempted);CHECK(failed_weights.compositions==attempted);
        CHECK(failed_weights.requests==attempted);CHECK(parent.sum(int(attempted))>0);
        CReaderSlices preserved_slices;preserved_slices.nDim=99;BridgeStats failed_return;
        attempted=0;
        const auto return_bytes=parent.bytes_sent;const auto return_calls=parent.collective_calls;
        const double return_seconds=parent.collective_seconds;
        CHECK_FALSE(ReaderSlices(parent,adapted,imported.original,reference,preserved_slices,reason,failed_return,failing,SIZE_MAX,0,detail));
        CHECK(failed_return.tracked_bytes==parent.bytes_sent-return_bytes);
        CHECK(failed_return.tracked_calls==parent.collective_calls-return_calls);
        CHECK(failed_return.tracked_collective_seconds==Approx(parent.collective_seconds-return_seconds).margin(1e-12));
        CHECK(preserved_slices.nDim==99);CHECK_FALSE(reason.empty());
        CHECK(failed_return.requests==failed_return.hits+failed_return.misses+failed_return.invalid_queries);
        CHECK(failed_return.failures==attempted);CHECK(failed_return.compositions==attempted);
        CHECK(failed_return.requests==attempted);CHECK(parent.sum(int(attempted))>0);
        CHECK(Bytes(imported.original)==original_bytes);CHECK(State(*current)==saved);
      }
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
  using CSinglezoneDriver::WriteTimeHistoryRestarts;
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
TEST_CASE("Native3D large reference import and reload elect memory refusal without publication", "[NativeReference3D]") {
  World world;auto config=MakeConfig(3,"SOLVER= EULER\n");const auto mesh=BoxMesh(3,5,false);
  if(!world.rank)simplex_test::WriteSU2Mesh(mesh,"large_reference_input.su2");
  MeshSolution cube(config.get(),mesh,0);
  ReferenceState original;ImportGeometry(*config,cube.Fine(),nullptr,original);
  REQUIRE(original.facets.size()==300);
  const auto text=EncodeReference(original);
  CHECK(EncodeReference(DecodeReference(text))==text);
  CHECK_THROWS_AS(DecodeReference(text,0),std::invalid_argument);
  CHECK_THROWS_AS(DecodeReference(text,8*text.size()),std::invalid_argument);
  CHECK_THROWS_AS(DecodeReference("SU2_NATIVE_REFERENCE_3D 1\n1 18446744073709551615\n"),std::invalid_argument);
  WriteReference("large.native_ref",original);
  ReferenceState loaded=original;const auto saved=loaded.original;
  const size_t caller=ReferenceStorageBytes(loaded),transport=transfer_memory::TransportBytes(world.size);
  const size_t early=transfer_memory::Add(caller,8*text.size(),transport);
  for(size_t limit : {size_t(0),early}) {
    bool found=true;std::string reason;size_t maximum=0;
    CHECK_FALSE(LoadReference("large.native_ref",loaded,found,reason,world.rank?SIZE_MAX:limit,caller,maximum));
    CHECK_FALSE(found);CHECK_FALSE(reason.empty());CHECK(maximum>limit);
    CHECK(loaded.original==saved);CHECK(EncodeReference(loaded)==text);
  }
  bool found=false;std::string reason;size_t maximum=0;
  const size_t limit=transfer_memory::Add(early,PlanarReference::ConstructionBytes(original.facets.size()));
  REQUIRE(LoadReference("large.native_ref",loaded,found,reason,limit,caller,maximum));
  CHECK(found);CHECK(reason.empty());CHECK(maximum==limit);
  CHECK(loaded.original!=saved);CHECK(EncodeReference(loaded)==text);
  CHECK(LoadReference("missing_large.native_ref",loaded,found,reason,limit,ReferenceStorageBytes(loaded),maximum));
  CHECK_FALSE(found);CHECK(reason.empty());CHECK(EncodeReference(loaded)==text);
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

TEST_CASE("Native3D actual metric-to-replacement lifecycle preserves CFD state", "[NativeLifecycle3D][NativeMetricLifecycle3D]") {
  World world;
  const bool correctness = std::getenv("SU2_NATIVE3D_LIFECYCLE_CORRECTNESS") != nullptr;
  const int workers = GENERATE(1, 2, 4);
  if (workers > world.size) return;
  const bool detail = GENERATE(false, true);
  if (correctness && detail) return;
  // Local correctness uses one actual computed-metric lifecycle per worker count.
  for (int repeat = 0; repeat < (correctness ? 1 : 3); ++repeat) {
    const auto name = "lifecycle_m" + std::to_string(workers) + "_d" + std::to_string(detail) + "_r" + std::to_string(repeat);
    if (!world.rank) {
      simplex_test::WriteSU2Mesh(BoxMesh(3, 1, false), name + ".su2");
      std::ofstream cfg(name + ".cfg");
      cfg << "SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\nMESH_FORMAT= SU2\nMESH_FILENAME= " << name << ".su2\n"
          << "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n"
          << "COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
          << "ADAP_HMIN= 0.5\nADAP_HMAX= 0.5000000000000001\nADAP_COMPLEXITY= 8\nADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= " << workers << "\n"
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
    const auto original_states=correctness ? Flow(world,driver->Geometry(),driver->Solver()) : std::vector<FlowRecord>{};
    const auto started=SU2_MPI::Wtime(); const auto started_cpu=std::clock();
    driver->AdaptMesh(transfer);
    const auto elapsed=SU2_MPI::Wtime()-started;
    const auto elapsed_cpu=double(std::clock()-started_cpu)/CLOCKS_PER_SEC;
    std::ostringstream timing;
    timing<<std::setprecision(17)<<"LIFECYCLECASE rank="<<world.rank<<" ranks="<<world.size
          <<" workers="<<workers<<" detailed="<<detail<<" repeat="<<repeat<<" wall="<<elapsed<<" cpu="<<elapsed_cpu<<'\n';
    ProfileRow3D(timing.str(),world.comm);
    REQUIRE(driver->Reference()->original);
    double error=0;
    const auto accepted_states=Flow(world,driver->Geometry(),driver->Solver());
    for (const auto& row : accepted_states) {
      su2double x[3]{row.point.x,row.point.y,row.point.z},expected[MAXVAR]{};field(x,expected);
      for(size_t k=0;k<5;++k) {
        error=std::max(error,RelDiff(row.values[k],expected[k]));
        error=std::max(error,RelDiff(row.values[5+k],expected[k]));
        error=std::max(error,RelDiff(row.values[10+k],expected[k]*.96875));
      }
    }
    CHECK(error<2e-10);
    if (correctness) SaveStates(world,name,original_states,accepted_states,4.);
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

TEST_CASE("Native3D manufactured anisotropic driver envelope", "[NativeEnvelope3D]") {
  World world;
  auto parameter=[](const char* key,int fallback){const auto* value=std::getenv(key);return value?std::stoi(value):fallback;};
  const int n=parameter("SU2_NATIVE3D_ENVELOPE_SIZE",2), mode=parameter("SU2_NATIVE3D_ENVELOPE_ANISOTROPY",1);
  const int varying=parameter("SU2_NATIVE3D_ENVELOPE_VARYING",0), workers=parameter("SU2_NATIVE3D_ENVELOPE_WORKERS",1);
  const int direction=parameter("SU2_NATIVE3D_ENVELOPE_DIRECTION",0);
  const int height_power=parameter("SU2_NATIVE3D_ENVELOPE_HEIGHT_POWER",0);
  const int wedge=parameter("SU2_NATIVE3D_ENVELOPE_WEDGE",0);
  const int restart=parameter("SU2_NATIVE3D_ENVELOPE_RESTART",0);
  REQUIRE(restart>=0);REQUIRE(restart<=2);
  const int repeat=parameter("SU2_NATIVE3D_ENVELOPE_REPEAT",0), detail=parameter("SU2_NATIVE3D_ENVELOPE_DETAIL",1);
  const bool aligned=parameter("SU2_NATIVE3D_ENVELOPE_ALIGNED",0)!=0;
  const int rounds=parameter("SU2_NATIVE3D_ENVELOPE_ROUNDS",512);
  const int batch=parameter("SU2_NATIVE3D_ENVELOPE_BATCH",MaximumCandidateBatch);
  REQUIRE(batch>=1);REQUIRE(batch<=int(MaximumCandidateBatch));
  REQUIRE((rounds==0 || rounds==512 || rounds==1024));
  REQUIRE((!aligned || n>=2));
  REQUIRE(n>=1);REQUIRE((n<=4 || n==8));REQUIRE(mode>=0);REQUIRE(mode<=2);REQUIRE(varying>=0);REQUIRE(varying<=1);
  REQUIRE(direction>=0);REQUIRE(direction<=1);
  REQUIRE((height_power==0 || height_power==10 || height_power==20));
  REQUIRE((wedge==0 || wedge==1));REQUIRE((!wedge || (aligned && (n==2 || n==4 || n==8))));
  REQUIRE(workers>=1);REQUIRE(workers<=world.size);REQUIRE(repeat>=0);REQUIRE(repeat<3);REQUIRE(detail>=0);REQUIRE(detail<=1);
  const double anisotropy=std::array<double,3>{1.,1.5,2.}[mode], variation=varying?.125:0.;
  auto reference_sensor=[=](const su2double* p,double scale) {
    const double q=scale*(1.+variation*SU2_TYPE::GetValue(p[0]));
    if(direction) {
      const double pi=std::acos(-1.),phi=pi*(.25+.25*SU2_TYPE::GetValue(p[0]));
      const double theta=pi*(SU2_TYPE::GetValue(p[1])/6.);
      const double x=std::cos(phi)*std::cos(theta),y=std::sin(phi)*std::cos(theta),z=std::sin(theta);
      const double delta=anisotropy*anisotropy-1.;
      return Tensor{q*(1.+delta*x*x),q*delta*x*y,q*delta*x*z,q*(1.+delta*y*y),q*delta*y*z,q*(1.+delta*z*z)};
    }
    const double diagonal=.5*(anisotropy*anisotropy+1.),off_diagonal=.5*(anisotropy*anisotropy-1.);
    return Tensor{q*diagonal,q*off_diagonal,0.,q*diagonal,0.,q};
  };
  auto sensor=[=](const su2double* p,double scale) {
    auto tensor=reference_sensor(p,scale);
    if(height_power) {
      tensor.xz=std::ldexp(tensor.xz,height_power);tensor.yz=std::ldexp(tensor.yz,height_power);
      tensor.zz=std::ldexp(tensor.zz,2*height_power);
    }
    return tensor;
  };
  if(!world.rank) {
    const double junction=double(n/2)/n;
    auto mesh=wedge ? WedgeMesh(n) : aligned ? simplex_test::MakeSimplexMesh(3,n,[=](const passivedouble* p) {
      if(p[2]<1e-12)return std::string(p[0]<junction ? "z_minus_a" : "z_minus_b");
      return simplex_test::Marker3D(p);
    }) : BoxMesh(3,n,false);
    if(height_power)for(size_t p=2;p<mesh.coord.size();p+=3)mesh.coord[p]=std::ldexp(mesh.coord[p],-height_power);
    simplex_test::WriteSU2Mesh(mesh,"envelope_input.su2");
    std::ofstream cfg("envelope.cfg");
    cfg<<"% Manufactured target: n="<<n<<" anisotropy="<<anisotropy<<" variation="<<variation<<" direction="<<direction<<" height_power="<<height_power<<" wedge="<<wedge<<" repeat="<<repeat<<"\n"
       <<"SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\nMESH_FORMAT= SU2\nMESH_FILENAME= envelope_input.su2\n"
       <<(wedge?"MARKER_FAR= (x_minus, y_minus, wedge_oblique, z_plus, z_minus_a, z_minus_b)\n":"MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n")
       <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nWRT_PERFORMANCE= "<<(detail?"YES":"NO")<<"\n"
       <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
       <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= "<<(restart==2?"CGNS":"SU2")<<"\nMESH_OUT_FILENAME= envelope_mesh\nCONV_FILENAME= envelope_history\n";
    if(restart)cfg<<"RESTART_FILENAME= envelope_restart\nSOLUTION_FILENAME= envelope_restart\nWRT_RESTART_COMPACT= YES\n";
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  char cfg[]="envelope.cfg";
  auto driver=std::make_unique<TetraDriver>(cfg,1,SU2_MPI::GetComm());
  auto field=AffineFlow(3);auto* initial=driver->Solver().GetNodes();
  for(unsigned long p=0;p<driver->Geometry().GetnPoint();++p) {
    su2double values[MAXVAR]{};field(driver->Geometry().nodes->GetCoord(p),values);
    for(unsigned short k=0;k<5;++k){initial->SetSolution(p,k,values[k]);initial->GetSolution_time_n()(p,k)=values[k];initial->GetSolution_time_n1()(p,k)=values[k]*.96875;}
  }
  auto backend=driver->Backend();
  if(rounds || batch!=int(MaximumCandidateBatch)) {
    EngineOptions diagnostic;diagnostic.maximum_rounds=rounds;diagnostic.candidate_batch=batch;
    backend=std::make_unique<CNativeRemesher3D>(driver->Reference(),diagnostic);
  }
  auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  CBarycentricTransfer transfer;std::string authority;std::vector<unsigned long> counts;
  for(int cycle=0;cycle<3;++cycle) {
    const double scale=n*n*(cycle==1?.25:1.);const auto name="envelope_s"+std::to_string(cycle);
    const auto before=State(driver->Geometry(),driver->Solver());const auto old=Flow(world,driver->Geometry(),driver->Solver());
    const auto parent_start=SU2_MPI::Wtime();const auto parent_cpu_start=std::clock();
    RuntimeProfile3D<4> profile(detail,"envelope");
    su2activematrix target(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<target.rows();++p){auto t=sensor(driver->Geometry().nodes->GetCoord(p),scale);std::array<double,6> a{t.xx,t.xy,t.xz,t.yy,t.yz,t.zz};for(size_t k=0;k<6;++k)target(p,k)=a[k];}
    profile.Mark(0);
    ReferenceState observed;if(driver->Reference()->original)observed=*driver->Reference();
    else if(driver->Config().GetRestart())REQUIRE(LoadReference(driver->Config().GetMesh_FileName()+".native_ref",observed));
    auto donor=ImportGeometry(driver->Config(),driver->Geometry(),&target,observed);
    Save(world,name+"_initial",donor.cells,donor.original,observed);
    profile.Mark(1);
    const auto result=backend->Remesh(driver->Config(),driver->Geometry(),target);
    profile.Mark(2);
    CHECK(State(driver->Geometry(),driver->Solver())==before);
    if(cycle==0 || (restart && cycle==1))CHECK_FALSE(driver->Reference()->original);else CHECK(EncodeReference(*driver->Reference())==authority);
    if(result.status!=CRemeshResult::Status::COMPLETE) {
      CHECK_FALSE(result.onAccepted);
      profile.Mark(3);profile.Report(cycle,{{"metric","observation","remesh","replace"}});
      INFO("n="<<n<<" anisotropy="<<anisotropy<<" varying="<<varying<<" N="<<world.size<<" M="<<workers<<" cycle="<<cycle);
      REQUIRE(result.status==CRemeshResult::Status::COMPLETE);
    }
    driver->ReplaceMesh(result,transfer);REQUIRE(driver->Reference()->original);
    if(cycle==0)authority=EncodeReference(*driver->Reference());CHECK(EncodeReference(*driver->Reference())==authority);
    profile.Mark(3);profile.Report(cycle,{{"metric","observation","remesh","replace"}});
    const auto parent_wall=SU2_MPI::Wtime()-parent_start;const auto parent_cpu=double(std::clock()-parent_cpu_start)/CLOCKS_PER_SEC;
    const auto transported=result.slices.FetchPointMetric(driver->Geometry());const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);
    SaveStates(world,name,old,fresh,scale);double error=0;
    for(auto row:fresh){su2double p[3]{row.point.x,row.point.y,row.point.z},expected[MAXVAR]{};field(p,expected);
      for(size_t k=0;k<5;++k){error=std::max(error,RelDiff(row.values[k],expected[k]));error=std::max(error,RelDiff(row.values[5+k],expected[k]));error=std::max(error,RelDiff(row.values[10+k],expected[k]*.96875));}}
    CHECK(error<2e-10);
    su2activematrix accepted_metric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<accepted_metric.rows();++p)for(size_t k=0;k<6;++k)accepted_metric(p,k)=transported[6*p+k];
    auto retained=*driver->Reference();auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&accepted_metric,retained);
    Save(world,name,accepted.cells,donor.original,retained);driver->WriteAdaptedMesh(cycle+1,cycle+1);
    if(cycle==2)OutputRoundTrip(world,name,result.slices,accepted.cells,retained,scale,sensor,(direction || height_power)?&donor.original:nullptr);
    if(restart && cycle==0) {
      RuntimeProfile3D<3> checkpoint(detail,"envelope_restart");
      driver->Config().SetTimeIter(2);
      const auto checkpoint_mesh=driver->AdaptedMeshName(1,1)+driver->Config().GetMesh_Out_FileExtension();
      const auto written_state=State(driver->Geometry(),driver->Solver());
      driver->WriteTimeHistoryRestarts();CHECK(State(driver->Geometry(),driver->Solver())==written_state);
      checkpoint.Mark(0);
      if(!world.rank) {
        std::ifstream input("envelope.cfg");std::string text((std::istreambuf_iterator<char>(input)),{});
        const std::string old_mesh="MESH_FILENAME= envelope_input.su2";
        const auto at=text.find(old_mesh);REQUIRE(at!=std::string::npos);
        text.replace(at,old_mesh.size(),"MESH_FILENAME= "+checkpoint_mesh);
        if(restart==2) {
          const std::string old_format="MESH_FORMAT= SU2";const auto format=text.find(old_format);REQUIRE(format!=std::string::npos);
          text.replace(format,old_format.size(),"MESH_FORMAT= CGNS");
        }
        std::ofstream output("envelope_restart.cfg");output<<text<<"RESTART_SOL= YES\nRESTART_ITER= 3\n";REQUIRE(output.good());
      }
      backend.reset();{Mute mute;driver->Finalize();driver.reset();}
      checkpoint.Mark(1);
      SU2_MPI::Barrier(world.comm);char restart_cfg[]="envelope_restart.cfg";
      driver=std::make_unique<TetraDriver>(restart_cfg,1,world.comm);
      driver->Preprocess(3);backend=driver->Backend();native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
      REQUIRE_FALSE(driver->Reference()->original);
      checkpoint.Mark(2);checkpoint.Report(3,{{"write","teardown","reload"}});
      const auto reloaded=Flow(world,driver->Geometry(),driver->Solver());
      SaveStates(world,"envelope_restart",fresh,reloaded,scale);
      std::map<std::array<double,3>,std::array<double,15>> wanted;
      for(const auto& r:fresh)REQUIRE(wanted.emplace(std::array<double,3>{r.point.x,r.point.y,r.point.z},r.values).second);
      REQUIRE(reloaded.size()==wanted.size());double restart_error=0;
      for(const auto& r:reloaded) {
        const auto& values=wanted.at({r.point.x,r.point.y,r.point.z});
        for(size_t k=0;k<15;++k)restart_error=std::max(restart_error,RelDiff(r.values[k],values[k]));
      }
      CHECK(restart_error<2e-10);
    }
    counts.push_back(result.slices.nElemGlobal);
    std::ostringstream row;row<<std::setprecision(17)<<"ENVELOPE rank="<<world.rank<<" ranks="<<world.size<<" workers="<<workers
      <<" size="<<n<<" anisotropy="<<anisotropy<<" variation="<<variation<<" direction="<<direction<<" height_power="<<height_power<<" wedge="<<wedge<<" repeat="<<repeat<<" detailed="<<detail<<" cycle="<<cycle
      <<" cells="<<counts.back()<<" points="<<result.slices.nPointGlobal<<" state_error="<<error<<" parent_wall="<<parent_wall<<" parent_cpu="<<parent_cpu<<'\n';ProfileRow3D(row.str(),world.comm);
  }
  REQUIRE(counts.size()==3);CHECK(counts[1]<counts[0]);CHECK(counts[2]>counts[1]);
  {Mute mute;driver->Finalize();driver.reset();}
}

TEST_CASE("Native3D size-limit replay retains private progress and accepted CFD state", "[NativeEnvelopeReplay3D]") {
  World world;REQUIRE(world.size==1);
  const auto* supplied=std::getenv("SU2_NATIVE3D_REPLAY_ROUNDS");
  const int rounds=supplied?std::stoi(supplied):512;
  REQUIRE((rounds==512 || rounds==1024 || rounds==2048));
  char cfg[]="envelope.cfg";
  auto driver=std::make_unique<TetraDriver>(cfg,1,SU2_MPI::GetComm());
  REQUIRE(driver->Geometry().GetGlobal_nPointDomain()==64);
  auto backend=driver->Backend();REQUIRE(dynamic_cast<CNativeRemesher3D*>(backend.get()));REQUIRE(driver->Reference());
  const auto before=State(driver->Geometry(),driver->Solver());
  su2activematrix metric(driver->Geometry().GetnPointDomain(),6);
  for(unsigned long p=0;p<metric.rows();++p) {
    const double q=9.*(1.+.125*SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p,0)));
    const std::array<double,6> tensor{q*1.625,q*.625,0.,q*1.625,0.,q};
    for(size_t k=0;k<6;++k)metric(p,k)=tensor[k];
  }
  ReferenceState reference;auto imported=ImportGeometry(driver->Config(),driver->Geometry(),&metric,reference);
  REQUIRE(imported.cells.size()==162);const auto authority=EncodeReference(reference);
  const auto old=Signature(world,imported.cells,reference);const auto sensors=Bytes(imported.original);
  Save(world,"replay_initial",imported.cells,imported.original,reference);
  EngineOptions options;options.maximum_rounds=rounds;options.timings=true;
  const auto replay=Refine(world,imported.cells,imported.original,*reference.original,{},options);
  CHECK(Signature(world,imported.cells,reference)==old);CHECK(Bytes(imported.original)==sensors);
  CHECK(State(driver->Geometry(),driver->Solver())==before);CHECK_FALSE(driver->Reference()->original);
  CHECK(EncodeReference(reference)==authority);REQUIRE_FALSE(replay.cells.empty());
  CHECK(replay.stats.rounds<=static_cast<uint64_t>(rounds));CHECK(replay.stats.accepted+replay.stats.rejected==replay.stats.attempted);
  Save(world,replay.valid?"replay_accepted":"replay_rejected",replay.cells,imported.original,reference);
  const auto& s=replay.stats;
  std::ofstream out("replay_result.json");out<<std::setprecision(17)
    <<"{\"complete\":"<<(replay.valid?"true":"false")<<",\"reason\":"<<std::quoted(replay.reason)
    <<",\"maximum_rounds\":"<<rounds<<",\"rounds\":"<<s.rounds<<",\"attempted\":"<<s.attempted
    <<",\"accepted\":"<<s.accepted<<",\"rejected\":"<<s.rejected<<",\"moved\":"<<s.moved<<",\"reconnected\":"<<s.reconnected
    <<",\"inserted\":"<<s.inserted<<",\"repair_proposals\":"<<s.repair_proposals<<",\"recovery_starts\":"<<s.recovery_starts
    <<",\"private_cells\":"<<replay.cells.size()<<",\"total_seconds\":"<<s.total_seconds
    <<",\"metric_seconds\":"<<s.metric_seconds<<",\"reconstruction_seconds\":"<<s.reconstruction_seconds
    <<",\"edge_trace_seconds\":"<<s.edge_trace_seconds<<",\"native_bound\":"<<s.maximum_work_bytes
    <<",\"process_cpu_seconds\":"<<s.process_cpu_seconds<<",\"accepted_state_preserved\":true}\n";
  CHECK(out.good());out.close();
  {Mute mute;driver->Finalize();driver.reset();}
}

TEST_CASE("Native3D initial boundary above cavity cap imports and reloads", "[NativeReference3D][NativeStaticFix3D]") {
  auto config = MakeConfig(3,"SOLVER= EULER\n");
  MeshSolution cube(config.get(),BoxMesh(3,8,false),0);
  ReferenceState reference;
  ImportGeometry(*config,cube.Fine(),nullptr,reference);
  REQUIRE(reference.original);
  REQUIRE(reference.facets.size()==768);
  const auto text = EncodeReference(reference);
  CHECK(EncodeReference(DecodeReference(text))==text);
  WriteReference("above_cavity_cap.native_ref",reference);
  ReferenceState loaded;
  REQUIRE(LoadReference("above_cavity_cap.native_ref",loaded));
  CHECK(EncodeReference(loaded)==text);
}

TEST_CASE("Native returned geometry accepts empty point connectivity", "[NativeBridge3D][NativeEmptyConnectivity3D]") {
  for (unsigned short dim:{2,3}) {
    CPoint empty(0,dim);
    CHECK_NOTHROW(empty.SetPoints({}));
    CPoint points(3,dim);
    const std::vector<std::vector<unsigned long>> connected{{1,2},{0,2},{0,1}};
    points.SetPoints(connected);
    for (unsigned long i=0;i<3;++i) {
      CHECK(points.GetnPoint(i)==2);
      CHECK(points.GetEdge(i,0)==-1);CHECK(points.GetEdge(i,1)==-1);
    }
    CHECK_NOTHROW(points.SetPoints(std::vector<std::vector<unsigned long>>(3)));
    for (unsigned long i=0;i<3;++i) CHECK(points.GetnPoint(i)==0);
    points.SetPoints(connected);
    for (unsigned long i=0;i<3;++i) CHECK(points.GetEdge(i,1)==-1);
  }
}

// Separate death-test tag: the external local runner expects collective rejection, never a successful Catch exit.
TEST_CASE("Native3D acceptance veto retains current and BDF state before publication", "[NativePolicyVeto3D]") {
  World world;const int veto=std::getenv("SU2_NATIVE3D_VETO_CALL") ? std::stoi(std::getenv("SU2_NATIVE3D_VETO_CALL")) : 2;
  REQUIRE((veto==1 || veto==2));
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(BoxMesh(3,1,false),"veto_input.su2");
    std::ofstream cfg("veto.cfg");cfg<<"SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\nMGLEVEL= 0\nMESH_FILENAME= veto_input.su2\n"
      <<"MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus_a, z_minus_b, z_plus)\n"
      <<"TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 0.01\nMAX_TIME= 0.02\n"
      <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\n"
      <<"ADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\nADAP_UNSTEADY_METRIC= WINDOW_AVERAGE\n"
      <<"ADAP_NATIVE_MAX_WORK= 2048\nADAP_NATIVE_MAX_TIME= 1000\n"
      <<"ADAP_NATIVE_RANKS= "<<(world.size>1 ? world.size/2 : 1)<<"\nWRT_PERFORMANCE= NO\n";
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>("veto.cfg"),1,SU2_MPI::GetComm());
  const auto before=State(driver->Geometry(),driver->Solver());
  const auto original=Flow(world,driver->Geometry(),driver->Solver());
  SaveStates(world,"veto_initial",original,original,1);
  su2activematrix sensor(driver->Geometry().GetnPointDomain(),6);
  for(unsigned long p=0;p<sensor.rows();++p)for(unsigned short k=0;k<6;++k)
    sensor(p,k)=(k==0 || k==3 || k==5) ? 4. : 0.;
  auto backend=driver->Backend();auto remeshed=backend->Remesh(driver->Config(),driver->Geometry(),sensor);
  REQUIRE(remeshed.status==CRemeshResult::Status::COMPLETE);REQUIRE(remeshed.beforeAcceptance);
  const auto deadline=remeshed.beforeAcceptance;int calls=0;
  remeshed.beforeAcceptance=[&]() {
    ++calls;const bool in_time=deadline();const bool unchanged=State(driver->Geometry(),driver->Solver())==before;
    const bool published=bool(driver->Reference()->original);
    CHECK(in_time);CHECK(unchanged);CHECK_FALSE(published);
    if(calls==veto) {
      const auto retained=Flow(world,driver->Geometry(),driver->Solver());
      SaveStates(world,"veto_retained",original,retained,1);
      const int changed_any=world.maximum(!unchanged),published_any=world.maximum(published),late_any=world.maximum(!in_time);
      if(!world.rank) {std::ofstream out("veto_evidence.json");out<<"{\"veto_call\":"<<calls
        <<",\"state_unchanged\":"<<(changed_any ? "false" : "true")
        <<",\"reference_published\":"<<(published_any ? "true" : "false")
        <<",\"deadline_valid\":"<<(late_any ? "false" : "true")<<"}\n";}
      return world.rank!=0;  // One-rank refusal must stop all ranks before the pointer/reference publication.
    }
    return true;
  };
  CBarycentricTransfer transfer;driver->ReplaceMesh(remeshed,transfer);
  FAIL("Expected collective acceptance veto before publication");
}

TEST_CASE("Analytic native3D reference persists explicit geometry and requires a frozen runtime tolerance",
          "[NativeCurvedReference3D]") {
  World world;
  ReferenceState state;
  state.marker_names={"sphere","plane"};
  const std::array<Node,4> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}};
  const Cell cell{0,nodes};
  Id sphere=0;
  for(auto face:Incidence({cell}).Boundary()) {
    const bool curved=std::find(face.begin(),face.end(),0)==face.end();
    const Id id=state.facets.size();if(curved)sphere=id;
    state.facets.push_back({id,curved?0:1,{nodes[face[0]],nodes[face[1]],nodes[face[2]]}});
  }
  state.original=std::make_shared<PlanarReference>(state.facets,SIZE_MAX,std::map<Id,AnalyticSurface>{{sphere,{}}});
  const auto text=EncodeReference(state);
  CHECK(text.find("SU2_NATIVE_REFERENCE_3D 2\n")==0);
  const auto decoded=DecodeReference(text);
  CHECK(EncodeReference(decoded)==text);CHECK(decoded.original->Analytic().size()==1);
  const auto p=decoded.original->Project(sphere,{.5,.5,0});
  CHECK(p.x==Approx(std::sqrt(.5)).epsilon(1e-14));
  CHECK_THROWS(DecodeReference(text,8*text.size()));
  CHECK_THROWS(DecodeReference(text+"junk"));
  auto stale=state;
  for(auto& f:stale.facets)for(auto& n:f.v)if(n.id==1)n.p={0,1,0};
  CHECK_THROWS(EncodeReference(stale));
  auto bad=text;bad.replace(bad.find("3D 2"),4,"3D 3");CHECK_THROWS(DecodeReference(bad));
  WriteReference("analytic.native_ref",state);
  ReferenceState loaded;REQUIRE(LoadReference("analytic.native_ref",loaded));CHECK(EncodeReference(loaded)==text);
  Owned accepted;std::vector<DonorCell> source;
  if(!world.rank){accepted.emplace(0,Owned::mapped_type{});accepted.at(0).cell=cell;source.push_back({cell,{}});}
  const auto result=Refine(world,accepted,source,*state.original,{},{});
  CHECK_FALSE(result.valid);CHECK(result.reason.find("positive frozen surface tolerance")!=std::string::npos);
  CHECK(result.cells.empty());CHECK(result.stats.accepted==0);
}

TEST_CASE("Persisted analytic bump authority reconstructs canonical finite chart unions after reload",
          "[NativeCurvedChart3D]") {
  World world;ReferenceState state;state.marker_names={"bump","other"};
  const std::array<Node,5> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{.5,.5,-1}},{4,{1,1,0}}}};
  std::vector<Cell> cells;
  for(auto v:{std::array<Node,4>{nodes[0],nodes[1],nodes[2],nodes[3]},std::array<Node,4>{nodes[1],nodes[4],nodes[2],nodes[3]}}){
    if(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)<0)std::swap(v[0],v[1]);cells.push_back({Id(cells.size()),v});}
  std::map<Id,AnalyticSurface> curves;AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.extent={1,1,1};
  for(auto face:Incidence(cells).Boundary()) {
    const bool bump=std::find(face.begin(),face.end(),3)==face.end();const Id id=state.facets.size();
    state.facets.push_back({id,bump?0:1,{nodes[face[0]],nodes[face[1]],nodes[face[2]]}});if(bump)curves.emplace(id,g);
  }
  state.original=std::make_shared<PlanarReference>(state.facets,SIZE_MAX,curves);
  const auto text=EncodeReference(state);
  const auto root=state.original->Canonical(curves.begin()->first);
  const auto decoded=DecodeReference(text);CHECK(EncodeReference(decoded)==text);
  REQUIRE(curves.size()==2);
  for(const auto& curve:curves)CHECK(decoded.original->Canonical(curve.first)==root);
  WriteReference("bump_chart.native_ref",state);ReferenceState loaded;REQUIRE(LoadReference("bump_chart.native_ref",loaded));
  CHECK(EncodeReference(loaded)==text);
  const auto p=loaded.original->Project(curves.begin()->first,{.8,.7,0});CHECK(p.z==Approx(.0336).epsilon(1e-14));
  Id rebound=999;
  for(auto face:std::vector<std::array<Id,3>>{{0,1,4},{0,4,2}})
    CHECK(loaded.original->FindFace(0,{nodes[face[0]].p,nodes[face[1]].p,nodes[face[2]].p},rebound));
  CHECK(rebound==root);CHECK_THROWS(DecodeReference(text,8*text.size()));
}

namespace {
void SaveCoverage(World& world,const std::string& name,const std::vector<DonorCell>& local,const ReferenceState& reference) {
  const auto source=world.metadata(local);
  WriteReference(name+".native_ref",reference);
  if(world.rank)return;
  std::ofstream out(name+"_source.json");out<<std::setprecision(17)<<"[";bool comma=false;
  for(auto d:source) {
    if(comma)out<<',';comma=true;
    out<<"{\"id\":"<<d.cell.id<<",\"component\":"<<d.extension_component<<",\"parent\":"<<d.parent_cell<<",\"vertices\":[";
    for(size_t i=0;i<4;++i){if(i)out<<',';const auto n=d.cell.v[i];const auto m=d.sensor[i];
      out<<'['<<n.id<<','<<n.p.x<<','<<n.p.y<<','<<n.p.z<<",["<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<"]]";}
    out<<"]}";
  }
  out<<"]\n";CHECK(out.good());
}
void DeclareImportedCurve(ReferenceState& state,bool sphere,double amplitude=1) {
  AnalyticSurface g;g.kind=sphere?AnalyticSurface::Kind::Sphere:AnalyticSurface::Kind::Bump;
  g.extent=sphere?Point{1,0,0}:Point{1,1,amplitude};
  std::map<Id,AnalyticSurface> curves;
  for(auto f:state.facets) {
    const bool bottom=state.marker_names.at(f.marker).rfind("z_minus",0)==0;
    if(sphere || bottom)curves.emplace(f.id,g);
  }
  state.original=std::make_shared<PlanarReference>(state.facets,SIZE_MAX,curves);
}
}
TEST_CASE("Actual CFD import generates bounded bump sensor columns with collective provenance and refusal",
          "[NativeGeneratedCoverage3D]") {
  World world;const int m=GENERATE(1,2,3,4);if(m>world.size)return;
  auto config=MakeConfig(3,"SOLVER= EULER\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\nADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "+std::to_string(m)+"\nADAP_HAUSD= 1e-3\nADAP_NATIVE_MAX_WORK= 32\n");
  MeshSolution mesh(config.get(),BoxMesh(3,1,false),0);
  su2activematrix metric(mesh.Fine().GetnPointDomain(),6);
  for(unsigned long p=0;p<metric.rows();++p){const auto value=Sensor(mesh.Fine().nodes->GetCoord(p),4);
    const std::array<double,6> row{value.xx,value.xy,value.xz,value.yy,value.yz,value.zz};for(size_t j=0;j<6;++j)metric(p,j)=row[j];}
  ReferenceState state;const auto plain=ImportGeometry(*config,mesh.Fine(),&metric,state);DeclareImportedCurve(state,false);
  const auto source=ImportGeometry(*config,mesh.Fine(),&metric,state);
  const auto total=CPassiveComm::Allreduce(uint64_t(source.original.size()),CPassiveComm::Op::SUM,world.comm);
  CHECK(total==12); // Six CFD tetrahedra and three extension tetrahedra per two physical bottom triangles.
  size_t ext=0;
  for(auto d:source.original)if(d.extension_component!=UINT64_MAX){++ext;CHECK(source.cells.count(d.parent_cell)==1);}
  CHECK(CPassiveComm::Allreduce(uint64_t(ext),CPassiveComm::Op::SUM,world.comm)==6);
  DonorImport service(world);std::string reason;REQUIRE(service.Build(source.original,reason));
  const auto patch=service.Import({DonorRegion{0,0,-.1,1,1,1}});REQUIRE(patch.valid);REQUIRE(patch.cells.size()==12);
  const auto components=AssociatedExtensions(patch.cells,*state.original);
  FrozenField field(patch.cells,{},false,components),original_only(patch.cells);
  const Point outside{.25,.25,-.02},inside{.25,.25,.1};
  const auto exterior=field.Query(outside);CHECK(exterior.extension_component!=UINT64_MAX);
  CHECK(exterior.sensor.xx==Approx(4+.125*.25));CHECK(exterior.sensor.zz==4);
  const auto old=original_only.Query(inside);CHECK(field.Query(inside).sensor.zz==old.sensor.zz);
  CHECK(field.Query(inside).weights==old.weights);CHECK(field.Query(inside).donor==old.donor);
  CHECK_THROWS(original_only.Query(outside));CHECK_THROWS(field.Query({.25,.25,-.1}));
  const double large=std::sqrt(4+.0125),ratio=2/large;
  const double expected=.1*large*(2./3)*(1+ratio+ratio*ratio)/(1+ratio)+.04;
  CHECK(double(field.SensorEdgeLength(inside,outside))==Approx(expected).epsilon(1e-12));
  CHECK(double(field.SensorEdgeLength(outside,inside))==Approx(expected).epsilon(1e-12));
  Save(world,"generated_bump",source.cells,source.original,state);
  SaveCoverage(world,"generated_bump",source.original,state);
  const auto held=state.original;ImportedGeometry unchanged=plain;BridgeStats stats;
  CHECK_FALSE(ImportGeometry(*config,mesh.Fine(),&metric,state,unchanged,reason,stats,world.rank?SIZE_MAX:0));
  CHECK(state.original==held);CHECK(unchanged.original.size()==plain.original.size());
  auto retained=std::make_shared<ReferenceState>(state);CNativeRemesher3D backend(retained);
  const auto result=backend.Remesh(*config,mesh.Fine(),metric);
  CHECK(result.status!=CRemeshResult::Status::COMPLETE);CHECK_FALSE(result.onAccepted);
  CHECK(backend.Statistics().workers==m);
  if(world.rank<m)CHECK(backend.Statistics().refine.stop!=EngineStop::Complete);
  CHECK(retained->original==state.original);
}
TEST_CASE("Actual CFD import generates a finite spherical sensor shell with copied boundary values",
          "[NativeGeneratedCoverage3D]") {
  World world;auto config=MakeConfig(3,"SOLVER= EULER\nADAP_HAUSD= 1e-3\n");
  const double q=1/std::sqrt(3.);CSimplexMesh seed;seed.nDim=3;
  seed.coord={q,q,q,-q,-q,q,-q,q,-q,q,-q,-q};seed.elem={0,1,2,3};
  auto point=[&](unsigned long i){return Point{seed.coord[3*i],seed.coord[3*i+1],seed.coord[3*i+2]};};
  if(Orientation(point(0),point(1),point(2),point(3))<0)std::swap(seed.elem[0],seed.elem[1]);
  seed.elemRef={0};CSimplexMesh::Marker marker;marker.name="x_minus";
  Cell cell;for(size_t i=0;i<4;++i)cell.v[i]={seed.elem[i],point(seed.elem[i])};
  for(auto f:Incidence({cell}).Boundary())marker.elem.insert(marker.elem.end(),f.begin(),f.end());seed.markers.push_back(marker);
  MeshSolution mesh(config.get(),seed,0);su2activematrix metric(mesh.Fine().GetnPointDomain(),6);
  for(unsigned long p=0;p<metric.rows();++p)for(size_t j=0;j<6;++j)metric(p,j)=j==0||j==3||j==5?.5:0;
  ReferenceState state;ImportGeometry(*config,mesh.Fine(),&metric,state);DeclareImportedCurve(state,true);
  const auto source=ImportGeometry(*config,mesh.Fine(),&metric,state);
  CHECK(CPassiveComm::Allreduce(uint64_t(source.original.size()),CPassiveComm::Op::SUM,world.comm)==13);
  DonorImport service(world);std::string reason;REQUIRE(service.Build(source.original,reason));
  const auto patch=service.Import({DonorRegion{-4,-4,-4,4,4,4}});REQUIRE(patch.valid);REQUIRE(patch.cells.size()==13);
  FrozenField field(patch.cells,{},false,AssociatedExtensions(patch.cells,*state.original)),original_only(patch.cells);
  const Point projected{-1,0,0};CHECK_THROWS(original_only.Query(projected));
  CHECK(field.Query(projected).sensor.xx==.5);CHECK(field.Query(projected).extension_component!=UINT64_MAX);
  CHECK(field.Query({0,0,0}).donor==original_only.Query({0,0,0}).donor);
  CHECK(double(field.SensorEdgeLength({-q,0,0},projected))==Approx((1-q)*std::sqrt(.5)).epsilon(1e-12));
  CHECK_THROWS(field.Query({10,0,0}));Save(world,"generated_sphere",source.cells,source.original,state);
  SaveCoverage(world,"generated_sphere",source.original,state);
}

TEST_CASE("Curved native3D runtime refines geometry, publishes and transfers constant BDF states", "[NativeCurvedRuntime3D]") {
  World world;const int m=GENERATE(1,2,3,4);if(m>world.size)return;
  const std::string name="curved_runtime_m"+std::to_string(m),filename=name+".cfg";
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(BoxMesh(3,1,false),name+".su2");std::ofstream cfg(filename);
    cfg<<"SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\nMESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\n"
       <<"MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n"
       <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<m<<"\nADAP_NATIVE_MAX_WORK= 256\nADAP_HAUSD= .003\n"
       <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 4\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
       <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= SU2\nMESH_OUT_FILENAME= "<<name<<"_mesh\nCONV_FILENAME= "<<name<<"_history\n";
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  TetraDriver driver(const_cast<char*>(filename.c_str()),1,SU2_MPI::GetComm());
  ReferenceState declared;ImportGeometry(driver.Config(),driver.Geometry(),nullptr,declared);
  AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.extent={1,1,-.01};
  std::map<Id,AnalyticSurface> curves;
  for(auto f:declared.facets)if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))curves.emplace(f.id,g);
  declared.original=std::make_shared<PlanarReference>(declared.facets,SIZE_MAX,curves);
  WriteReference(name+".su2.native_ref",declared);
  const auto authority=EncodeReference(declared);auto backend=driver.Backend();
  auto reference=driver.Reference();REQUIRE(reference);CHECK_FALSE(reference->original);
  auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  auto* nodes=driver.Solver().GetNodes();
  const std::array<su2double,5> constant{1.,.2,.1,.05,3.};
  for(unsigned long p=0;p<driver.Geometry().GetnPoint();++p)for(size_t k=0;k<5;++k) {
    nodes->SetSolution(p,k,constant[k]);nodes->GetSolution_time_n()(p,k)=constant[k];nodes->GetSolution_time_n1()(p,k)=constant[k]*.96875;
  }
  su2activematrix metric(driver.Geometry().GetnPointDomain(),6);
  for(unsigned long p=0;p<metric.rows();++p)for(size_t k=0;k<6;++k)metric(p,k)=(k==0 || k==3 || k==5)? .5:0.;
  const auto initial=State(driver.Geometry(),driver.Solver());const auto old=Flow(world,driver.Geometry(),driver.Solver());
  const auto donor=ImportGeometry(driver.Config(),driver.Geometry(),&metric,declared);
  Save(world,name+"_initial",donor.cells,donor.original,declared);SaveCoverage(world,name,donor.original,declared);
  const auto result=backend->Remesh(driver.Config(),driver.Geometry(),metric);
  CHECK(State(driver.Geometry(),driver.Solver())==initial);CHECK_FALSE(reference->original);
  INFO("workers="<<m<<" stop="<<EngineStopName(native->Statistics().refine.stop));
  REQUIRE(result.status==CRemeshResult::Status::COMPLETE);
  CHECK(CPassiveComm::Allreduce(native->Statistics().refine.geometry_progress,CPassiveComm::Op::SUM,world.comm)>0);
  CHECK(result.slices.nElemGlobal>6);CBarycentricTransfer transfer;driver.ReplaceMesh(result,transfer);
  CHECK(EncodeReference(*reference)==authority);
  const auto transported=result.slices.FetchPointMetric(driver.Geometry());const auto fresh=Flow(world,driver.Geometry(),driver.Solver(),transported);
  SaveStates(world,name,old,fresh,.5);
  for(auto row:fresh)for(size_t k=0;k<5;++k) {
    CHECK(row.values[k]==Approx(SU2_TYPE::GetValue(constant[k])).epsilon(1e-11));
    CHECK(row.values[5+k]==Approx(SU2_TYPE::GetValue(constant[k])).epsilon(1e-11));
    CHECK(row.values[10+k]==Approx(SU2_TYPE::GetValue(constant[k]*.96875)).epsilon(1e-11));
  }
  su2activematrix accepted_metric(driver.Geometry().GetnPointDomain(),6);
  for(unsigned long p=0;p<accepted_metric.rows();++p)for(size_t k=0;k<6;++k)accepted_metric(p,k)=transported[6*p+k];
  const auto accepted=ImportGeometry(driver.Config(),driver.Geometry(),&accepted_metric,*reference);
  Save(world,name+"_accepted",accepted.cells,donor.original,*reference);driver.WriteAdaptedMesh(1,1);
  WriteReference(name+"_accepted.native_ref",*reference);
  ReferenceState loaded;REQUIRE(LoadReference(name+"_accepted.native_ref",loaded));CHECK(EncodeReference(loaded)==authority);
}

namespace {
void SaveProjection(World& world,const std::string& name,const CConservativeTransfer& transfer) {
  const auto& s=transfer.GetSummary();const auto& p=s.projection;
  REQUIRE(s.nTimeLevels==2);REQUIRE(s.interpolateTimeN1);
  REQUIRE(p.targetTotal.size()==s.newIntegral.size());
  CHECK(std::abs(p.targetVolume-p.donorVolume-p.fillVolume+p.sliverVolume)<1e-10);
  for(size_t f=0;f<p.targetTotal.size();++f) {
    CHECK(std::abs(s.newIntegral[f]-p.targetTotal[f])<=1e-10*std::max(1.,double(p.scale[f])));
    CHECK(std::abs(p.targetTotal[f]-p.donorTotal[f]-p.fillOpen[f]+p.sliverOpen[f])<=1e-10*std::max(1.,double(p.scale[f])));
  }
  if(world.rank)return;
  std::ofstream out(name+"_projection.json");out<<std::setprecision(17)
    <<"{\"donor_volume\":"<<p.donorVolume<<",\"target_volume\":"<<p.targetVolume
    <<",\"fill_volume\":"<<p.fillVolume<<",\"lost_volume\":"<<p.sliverVolume<<",\"time_levels\":"<<s.nTimeLevels;
  auto values=[&](const char* key,const auto& v){out<<",\""<<key<<"\":[";for(size_t i=0;i<v.size();++i){if(i)out<<',';out<<v[i];}out<<']';};
  values("donor_total",p.donorTotal);values("target_total",p.targetTotal);values("new_total",s.newIntegral);
  values("fill",p.fill);values("lost",p.sliver);values("fill_open",p.fillOpen);values("lost_open",p.sliverOpen);
  values("scale",p.scale);values("residual",p.residual);out<<"}\n";CHECK(out.good());
}
void CheckPositiveFlow(const std::vector<FlowRecord>& records) {
  for(const auto& row:records)for(size_t level=0;level<3;++level) {
    const auto* u=row.values.data()+5*level;CHECK(u[0]>0);
    CHECK(u[4]-(u[1]*u[1]+u[2]*u[2]+u[3]*u[3])/(2*u[0])>0);
  }
}
std::unique_ptr<TetraDriver> ReloadCurved(World& world,std::unique_ptr<TetraDriver> driver,
                                        const std::string& name,const std::string& mesh,const std::string& format) {
  driver->Config().SetTimeIter(2);const auto before=State(driver->Geometry(),driver->Solver());
  const auto expected=Flow(world,driver->Geometry(),driver->Solver());driver->WriteTimeHistoryRestarts();
  CHECK(State(driver->Geometry(),driver->Solver())==before);
  if(!world.rank) {
    std::ifstream input(name+".cfg");std::string text((std::istreambuf_iterator<char>(input)),{});
    const std::string original="MESH_FILENAME= "+name+".su2";const auto at=text.find(original);REQUIRE(at!=std::string::npos);
    text.replace(at,original.size(),"MESH_FILENAME= "+mesh);
    const std::string oldformat="MESH_FORMAT= SU2";const auto f=text.find(oldformat);REQUIRE(f!=std::string::npos);
    text.replace(f,oldformat.size(),"MESH_FORMAT= "+format);
    std::ofstream output(name+"_restart.cfg");output<<text<<"RESTART_SOL= YES\nRESTART_ITER= 3\n";CHECK(output.good());
  }
  {Mute mute;driver->Finalize();driver.reset();}
  SU2_MPI::Barrier(world.comm);const auto cfg=name+"_restart.cfg";
  driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);driver->Preprocess(3);
  const auto loaded=Flow(world,driver->Geometry(),driver->Solver());SaveStates(world,name+"_restart",expected,loaded,0);
  std::map<std::array<double,3>,std::array<double,15>> wanted;
  for(const auto& r:expected)REQUIRE(wanted.emplace(std::array<double,3>{r.point.x,r.point.y,r.point.z},r.values).second);
  REQUIRE(loaded.size()==wanted.size());
  for(const auto& r:loaded)for(size_t k=0;k<15;++k)CHECK(RelDiff(r.values[k],wanted.at({r.point.x,r.point.y,r.point.z})[k])<2e-10);
  return driver;
}
}
TEST_CASE("Curved native3D repeated conservative events preserve domain accounting and CGNS restart",
          "[NativeCurvedLifecycle3D]") {
  World world;const int workers=GENERATE(1,2,3,4);if(workers>world.size)return;
  const bool growth=GENERATE(false,true);const double amplitude=growth?-.01:.01;
#ifdef HAVE_CGNS
  const std::string format="CGNS";
#else
  const std::string format="SU2";
#endif
  const auto name="curved_lifecycle_"+std::string(growth?"grow":"shrink")+"_m"+std::to_string(workers);
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(BoxMesh(3,1,false),name+".su2");std::ofstream cfg(name+".cfg");
    cfg<<"SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\nMESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\n"
       <<"MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n"
       <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nADAP_NATIVE_MAX_WORK= 4096\nADAP_HAUSD= .003\n"
       <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
       <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= "<<format<<"\nMESH_OUT_FILENAME= "<<name<<"_mesh\nCONV_FILENAME= "<<name<<"_history\n"
       <<"RESTART_FILENAME= "<<name<<"_flow\nSOLUTION_FILENAME= "<<name<<"_flow\nWRT_RESTART_COMPACT= YES\n";
  }
  SU2_MPI::Barrier(world.comm);const auto cfg=name+".cfg";
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  ReferenceState declared;ImportGeometry(driver->Config(),driver->Geometry(),nullptr,declared);DeclareImportedCurve(declared,false,amplitude);
  WriteReference(name+".su2.native_ref",declared);const auto authority=EncodeReference(declared);
  auto field=AffineFlow(3);auto* nodes=driver->Solver().GetNodes();
  for(unsigned long p=0;p<driver->Geometry().GetnPoint();++p) {
    su2double u[MAXVAR]{};field(driver->Geometry().nodes->GetCoord(p),u);
    for(size_t k=0;k<5;++k){nodes->SetSolution(p,k,u[k]);nodes->GetSolution_time_n()(p,k)=u[k];nodes->GetSolution_time_n1()(p,k)=u[k]*.96875;}
  }
  auto backend=driver->Backend();CConservativeTransfer transfer;std::vector<unsigned long> counts;
  for(int cycle=0;cycle<3;++cycle) {
    const double scale=cycle==1?4.:.5;const auto event=name+"_s"+std::to_string(cycle);
    su2activematrix metric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<metric.rows();++p)for(size_t k=0;k<6;++k)metric(p,k)=(k==0 || k==3 || k==5)?scale:0.;
    auto observed=declared;const auto donor=ImportGeometry(driver->Config(),driver->Geometry(),&metric,observed);
    Save(world,event+"_initial",donor.cells,donor.original,observed);SaveCoverage(world,event,donor.original,observed);
    const auto before=State(driver->Geometry(),driver->Solver());const auto old=Flow(world,driver->Geometry(),driver->Solver());
    const auto result=backend->Remesh(driver->Config(),driver->Geometry(),metric);
    CHECK(State(driver->Geometry(),driver->Solver())==before);REQUIRE(result.status==CRemeshResult::Status::COMPLETE);
    driver->ReplaceMesh(result,transfer);CHECK(EncodeReference(*driver->Reference())==authority);SaveProjection(world,event,transfer);
    const auto transported=result.slices.FetchPointMetric(driver->Geometry());const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);
    SaveStates(world,event,old,fresh,scale);CheckPositiveFlow(fresh);counts.push_back(result.slices.nElemGlobal);
    auto accepted_metric=metric;accepted_metric.resize(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<accepted_metric.rows();++p)for(size_t k=0;k<6;++k)accepted_metric(p,k)=transported[6*p+k];
    auto retained=*driver->Reference();const auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&accepted_metric,retained);
    Save(world,event+"_accepted",accepted.cells,donor.original,retained);driver->WriteAdaptedMesh(cycle+1,cycle+1);
    if(cycle==0) {
      const auto checkpoint=driver->AdaptedMeshName(1,1)+driver->Config().GetMesh_Out_FileExtension();
      backend.reset();driver=ReloadCurved(world,std::move(driver),name,checkpoint,format);backend=driver->Backend();
      REQUIRE_FALSE(driver->Reference()->original);
    }
    if(cycle==2)OutputRoundTrip(world,event,result.slices,accepted.cells,retained,scale,
      [](const su2double*,double s){return Tensor{s,0,0,s,0,s};},&donor.original);
  }
  REQUIRE(counts.size()==3);CHECK(counts[1]>counts[0]);CHECK(counts[2]<counts[1]);
}
TEST_CASE("Solved three-dimensional Euler pressure supplies repeated native curved metric events", "[NativeSolvedEuler3D]") {
  World world;const int workers=GENERATE(1,2,4);if(workers>world.size)return;
  const auto name="solved_bump_m"+std::to_string(workers);const double amplitude=-.01;
  if(!world.rank) {
    auto mesh=BoxMesh(3,2,false);
    for(size_t p=0;p<mesh.coord.size();p+=3){const auto x=mesh.coord[p],y=mesh.coord[p+1],z=mesh.coord[p+2];mesh.coord[p+2]=z+(1-z)*amplitude*x*(1-x)*y*(1-y);}
    simplex_test::WriteSU2Mesh(mesh,name+".su2");std::ofstream cfg(name+".cfg");
    cfg<<"SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\nMESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\n"
       <<"MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\nMARKER_EULER= (z_minus_a, z_minus_b)\n"
       <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
       <<"ADAP_COMPLEXITY= 6\nADAP_HMIN= .3\nADAP_HMAX= 2\nADAP_ARMAX= 2\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nADAP_NATIVE_MAX_WORK= 4096\nADAP_HAUSD= .003\n"
       <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\nCFL_NUMBER= .5\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 8\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 5\n"
       <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= SU2\nMESH_OUT_FILENAME= "<<name<<"_mesh\nCONV_FILENAME= "<<name<<"_history\n"
       <<"RESTART_FILENAME= "<<name<<"_flow\nSOLUTION_FILENAME= "<<name<<"_flow\nWRT_RESTART_COMPACT= YES\n";
  }
  SU2_MPI::Barrier(world.comm);const auto cfg=name+".cfg";
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  ReferenceState declared;ImportGeometry(driver->Config(),driver->Geometry(),nullptr,declared);DeclareImportedCurve(declared,false,amplitude);
  WriteReference(name+".su2.native_ref",declared);const auto authority=EncodeReference(declared);CConservativeTransfer transfer;
  auto backend=driver->Backend();
  for(int cycle=0;cycle<2;++cycle) {
    const auto initial=Flow(world,driver->Geometry(),driver->Solver());
    for(unsigned long step=cycle?3:0;step<(cycle?5:2);++step){driver->Preprocess(step);driver->Run();driver->Postprocess();driver->Update();}
    const auto solved=Flow(world,driver->Geometry(),driver->Solver());CheckPositiveFlow(solved);
    double change=0;for(size_t p=0;p<initial.size();++p)for(size_t k=0;k<5;++k)change=std::max(change,std::abs(initial[p].values[k]-solved[p].values[k]));
    CHECK(change>1e-10);driver->ComputeMetric();const auto& metric=driver->Solver().GetNodes()->GetMetric();
    const auto event=name+"_s"+std::to_string(cycle);auto observed=declared;
    const auto donor=ImportGeometry(driver->Config(),driver->Geometry(),&metric,observed);
    Save(world,event+"_initial",donor.cells,donor.original,observed);SaveCoverage(world,event,donor.original,observed);
    const auto result=backend->Remesh(driver->Config(),driver->Geometry(),metric);
    INFO("actual Euler-derived metric workers="<<workers<<" cycle="<<cycle);
    REQUIRE(result.status==CRemeshResult::Status::COMPLETE);driver->ReplaceMesh(result,transfer);SaveProjection(world,event,transfer);
    CHECK(EncodeReference(*driver->Reference())==authority);const auto transported=result.slices.FetchPointMetric(driver->Geometry());
    const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);SaveStates(world,event,solved,fresh,0);CheckPositiveFlow(fresh);
    su2activematrix accepted_metric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<accepted_metric.rows();++p)for(size_t k=0;k<6;++k)accepted_metric(p,k)=transported[6*p+k];
    auto retained=*driver->Reference();const auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&accepted_metric,retained);
    Save(world,event+"_accepted",accepted.cells,donor.original,retained);driver->WriteAdaptedMesh(cycle+1,cycle+1);
    if(cycle==0){const auto checkpoint=driver->AdaptedMeshName(1,1)+driver->Config().GetMesh_Out_FileExtension();backend.reset();driver=ReloadCurved(world,std::move(driver),name,checkpoint,"SU2");backend=driver->Backend();}
  }
  driver->Preprocess(5);driver->Run();driver->Postprocess();driver->Update();CheckPositiveFlow(Flow(world,driver->Geometry(),driver->Solver()));
}

namespace {
CSimplexMesh PrimitiveMesh(bool sphere,bool tilted) {
  CSimplexMesh mesh;mesh.nDim=3;
  auto add=[&](Point p){const auto id=mesh.GetnPoint();mesh.coord.insert(mesh.coord.end(),{p.x,p.y,p.z});return id;};
  if(sphere) {
    add({0,0,0});for(Point p:std::array<Point,6>{{{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}}})add(p);
    for(int x=0;x<2;++x)for(int y=0;y<2;++y)for(int z=0;z<2;++z)
      mesh.elem.insert(mesh.elem.end(),{0,static_cast<unsigned long>(1+x),static_cast<unsigned long>(3+y),static_cast<unsigned long>(5+z)});
  } else {
    const double q=1/std::sqrt(3.),s=1/std::sqrt(2.);
    const Point axis=tilted?Point{q,q,q}:Point{0,0,1},u=tilted?Point{0,-s,s}:Point{1,0,0},v=tilted?Point{2*s*q,-s*q,-s*q}:Point{0,1,0};
    const Point origin=tilted?Point{2,-1,.5}:Point{0,0,0};
    for(int level=0;level<2;++level) {
      add({origin.x+level*axis.x,origin.y+level*axis.y,origin.z+level*axis.z});
      for(int i=0;i<8;++i){const auto a=i*std::acos(-1.)/4;add({origin.x+std::cos(a)*u.x+std::sin(a)*v.x+level*axis.x,origin.y+std::cos(a)*u.y+std::sin(a)*v.y+level*axis.y,origin.z+std::cos(a)*u.z+std::sin(a)*v.z+level*axis.z});}
    }
    for(unsigned long i=1;i<=8;++i) {
      std::array<unsigned long,3> triangle{0,i,i==8?1:i+1};std::sort(triangle.begin(),triangle.end());
      const auto a=triangle[0],b=triangle[1],c=triangle[2];
      for(auto tet:std::array<std::array<unsigned long,4>,3>{{{a,b,c,c+9},{a,b,b+9,c+9},{a,b+9,a+9,c+9}}})mesh.elem.insert(mesh.elem.end(),tet.begin(),tet.end());
    }
  }
  auto node=[&](unsigned long i){return Node{i,{mesh.coord[3*i],mesh.coord[3*i+1],mesh.coord[3*i+2]}};};
  std::vector<Cell> cells;
  for(size_t e=0;e<mesh.elem.size();e+=4) {
    auto& ids=mesh.elem;Cell cell{e/4,{node(ids[e]),node(ids[e+1]),node(ids[e+2]),node(ids[e+3])}};
    if(Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p)<0){std::swap(ids[e],ids[e+1]);std::swap(cell.v[0],cell.v[1]);}
    cells.push_back(cell);
  }
  mesh.elemRef.assign(cells.size(),0);std::map<std::string,std::vector<unsigned long>> boundary;
  for(auto face:Incidence(cells).Boundary()) {
    const std::string name=sphere?"sphere":std::all_of(face.begin(),face.end(),[](Id id){return id<9;})?"cap0":std::all_of(face.begin(),face.end(),[](Id id){return id>=9;})?"cap1":"cylinder";
    auto& ids=boundary[name];ids.insert(ids.end(),face.begin(),face.end());
  }
  for(auto row:boundary){CSimplexMesh::Marker marker;marker.name=row.first;marker.elem=row.second;mesh.markers.push_back(std::move(marker));}
  return mesh;
}
void DeclarePrimitive(ReferenceState& state,bool sphere,bool tilted) {
  AnalyticSurface g;g.kind=sphere?AnalyticSurface::Kind::Sphere:AnalyticSurface::Kind::Cylinder;g.extent=sphere?Point{1,0,0}:Point{1,1,0};
  if(tilted && !sphere){g.origin={2,-1,.5};g.axis={1,1,1};}
  std::map<Id,AnalyticSurface> curves;
  for(auto f:state.facets) {
    auto local=g;const auto name=state.marker_names.at(f.marker);
    if(name=="cap0" || name=="cap1") {
      local.kind=AnalyticSurface::Kind::Disk;local.extent.y=0;
      if(name=="cap1") {const auto l=std::hypot(g.axis.x,g.axis.y,g.axis.z);local.origin={g.origin.x+g.axis.x/l,g.origin.y+g.axis.y/l,g.origin.z+g.axis.z/l};}
    }
    curves.emplace(f.id,local);
  }
  state.original=std::make_shared<PlanarReference>(state.facets,SIZE_MAX,curves);
}
}
TEST_CASE("Native sphere cylinder and disk rims adapt through full distributed runtime and persistence", "[NativePrimitiveRuntime3D]") {
  World world;const bool sphere=GENERATE(false,true);const bool tilted=GENERATE(false,true);if(sphere && tilted)return;
  const int workers=GENERATE(1,2,3,4);if(workers>world.size)return;
  const auto name=std::string(sphere?"sphere":"cylinder")+"_axis"+std::to_string(tilted)+"_m"+std::to_string(workers);
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(PrimitiveMesh(sphere,tilted),name+".su2");std::ofstream cfg(name+".cfg");
    cfg<<"SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nMESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\n"
       <<(sphere?"MARKER_FAR= (sphere)\n":"MARKER_FAR= (cylinder, cap0, cap1)\n")
       <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nADAP_NATIVE_MAX_WORK= 4096\nADAP_HAUSD= "<<(sphere?.22:.04)<<"\n"
       <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 8\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
       <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= SU2\nMESH_OUT_FILENAME= "<<name<<"_mesh\nCONV_FILENAME= "<<name<<"_history\n"
       <<"RESTART_FILENAME= "<<name<<"_flow\nSOLUTION_FILENAME= "<<name<<"_flow\nWRT_RESTART_COMPACT= YES\n";
  }
  SU2_MPI::Barrier(world.comm);const auto cfg=name+".cfg";
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  ReferenceState declared;ImportGeometry(driver->Config(),driver->Geometry(),nullptr,declared);DeclarePrimitive(declared,sphere,tilted);
  const auto authority=EncodeReference(declared);CHECK(authority.find(sphere?"3D 2\n":"3D 3\n")!=std::string::npos);
  WriteReference(name+".su2.native_ref",declared);auto backend=driver->Backend();CConservativeTransfer transfer;
  auto* nodes=driver->Solver().GetNodes();const std::array<su2double,5> constant{1,.2,.1,.05,3};
  for(unsigned long p=0;p<driver->Geometry().GetnPoint();++p)for(size_t k=0;k<5;++k){nodes->SetSolution(p,k,constant[k]);nodes->GetSolution_time_n()(p,k)=constant[k];nodes->GetSolution_time_n1()(p,k)=constant[k]*.96875;}
  for(int cycle=0;cycle<2;++cycle) {
    const auto event=name+"_s"+std::to_string(cycle);su2activematrix metric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<metric.rows();++p)for(size_t k=0;k<6;++k)metric(p,k)=(k==0 || k==3 || k==5)?.5:0.;
    auto observed=declared;const auto donor=ImportGeometry(driver->Config(),driver->Geometry(),&metric,observed);
    Save(world,event+"_initial",donor.cells,donor.original,observed);SaveCoverage(world,event,donor.original,observed);
    const auto before=State(driver->Geometry(),driver->Solver());const auto old=Flow(world,driver->Geometry(),driver->Solver());
    const auto result=backend->Remesh(driver->Config(),driver->Geometry(),metric);
    CHECK(State(driver->Geometry(),driver->Solver())==before);
    if(result.status!=CRemeshResult::Status::COMPLETE) {
      EngineOptions diagnosis;diagnosis.maximum_work=4096;diagnosis.surface_tolerance=SU2_TYPE::GetValue(driver->Config().GetAdap_Hausd());
      const auto rejected=Refine(world,donor.cells,donor.original,*observed.original,{},diagnosis);
      Save(world,event+"_rejected",rejected.cells,donor.original,observed);
      INFO("replay: "<<rejected.reason);
    }
    INFO(name<<" cycle="<<cycle);REQUIRE(result.status==CRemeshResult::Status::COMPLETE);
    driver->ReplaceMesh(result,transfer);SaveProjection(world,event,transfer);CHECK(EncodeReference(*driver->Reference())==authority);
    const auto transported=result.slices.FetchPointMetric(driver->Geometry());const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);
    SaveStates(world,event,old,fresh,.5);CheckPositiveFlow(fresh);
    su2activematrix accepted_metric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<accepted_metric.rows();++p)for(size_t k=0;k<6;++k)accepted_metric(p,k)=transported[6*p+k];
    auto retained=*driver->Reference();const auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&accepted_metric,retained);
    Save(world,event+"_accepted",accepted.cells,donor.original,retained);driver->WriteAdaptedMesh(cycle+1,cycle+1);
    if(cycle==0){const auto checkpoint=driver->AdaptedMeshName(1,1)+driver->Config().GetMesh_Out_FileExtension();backend.reset();driver=ReloadCurved(world,std::move(driver),name,checkpoint,"SU2");backend=driver->Backend();}
    if(cycle==1)OutputRoundTrip(world,event,result.slices,accepted.cells,retained,.5,[](const su2double*,double){return Tensor{.5,0,0,.5,0,.5};},&donor.original);
  }
}
