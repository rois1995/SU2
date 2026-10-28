/*!
 * \file CNativeImport3D_tests.cpp
 * \brief Actual CFD-owner import, worker adaptation, reader return and solution/history transfer.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "TransferTestCase.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../Common/include/adaptation/CNativeEngine3D.hpp"
#include "../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include "../../../Common/include/adaptation/CNativeRegionField3D.hpp"
#include "../../../SU2_CFD/include/adaptation/CBarycentricTransfer.hpp"
#include "../../../SU2_CFD/include/output/CMeshOutput.hpp"
#include "../../../SU2_CFD/include/adaptation/CConservativeTransfer.hpp"
#include "../../../SU2_CFD/include/adaptation/CBoundaryLayerMetric.hpp"
#include "../../../SU2_CFD/include/adaptation/CNativeNormalizationStage3D.hpp"
#include "../../../Common/include/geometry/meshreader/CSU2ASCIIMeshReaderFVM.hpp"
#include "../../../Common/include/geometry/meshreader/CCGNSMeshReaderFVM.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../Common/include/adaptation/CNativeRuntimeProfile3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../SU2_CFD/include/drivers/CSinglezoneDriver.hpp"
#include "../../../SU2_CFD/include/solvers/CTurbSolver.hpp"
#include <iomanip>
#include <chrono>
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
  REQUIRE(solver.GetnVar()<=5);
  REQUIRE(geometry.GetGlobal_nPointDomain()<=4096);  // Test-only bounded complete gather for resolved controls.
  for(unsigned long p=0;p<geometry.GetnPointDomain();++p) {
    FlowRecord r;r.id=geometry.nodes->GetGlobalIndex(p);r.point={SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,0)),SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,1)),SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,2))};
    for(size_t k=0;k<solver.GetnVar();++k){r.values[k]=SU2_TYPE::GetValue(nodes->GetSolution(p,k));r.values[5+k]=SU2_TYPE::GetValue(nodes->GetSolution_time_n(p,k));r.values[10+k]=SU2_TYPE::GetValue(nodes->GetSolution_time_n1(p,k));}
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
std::vector<OwnedCell> PatchRecords(const Patch& patch) {
      std::vector<OwnedCell> rows;
      for(const auto& c:patch.cells) {
        OwnedCell row;row.cell=c;
        for(unsigned omit=0;omit<4;++omit) {
          Face face{};unsigned at=0;for(unsigned k=0;k<4;++k)if(k!=omit)face[at++]=c.v[k].id;
          std::sort(face.begin(),face.end());
          for(const auto& f:patch.surface){auto key=f.v;std::sort(key.begin(),key.end());if(key==face)row.physical[omit]={true,f.facet};}
        }
        rows.push_back(row);
      }
      return rows;
}

void SaveRecords(const std::string& name,const std::vector<OwnedCell>& cells,const std::vector<DonorCell>& source,const ReferenceState& reference) {
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

void Save(World& world,const std::string& name,const Owned& local,const std::vector<DonorCell>& original,const ReferenceState& reference) {
  const auto cells=world.metadata([&]{std::vector<OwnedCell> v;for(auto r:local)v.push_back(r.second);return v;}());
  const auto source=world.metadata(original);if(!world.rank)SaveRecords(name,cells,source,reference);
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
    auto authority=reference;
    if(reference.original->Mapped()) {
      WriteReference(file+".native_ref",reference);authority={};REQUIRE(LoadReference(file+".native_ref",authority));
      CHECK(EncodeReference(authority)==EncodeReference(reference));
    }
    auto returned=ImportGeometry(*config,loaded.Fine(),&metric,authority);
    if(!reference.original->Mapped())CHECK(authority.original==reference.original);
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
      CHECK(sampled.field_builds==0);CHECK(sampled.source_point_queries<=sampled.requests);
      CHECK(CPassiveComm::Allreduce(sampled.source_point_queries,CPassiveComm::Op::SUM,parent.comm)<
            CPassiveComm::Allreduce(sampled.requests,CPassiveComm::Op::SUM,parent.comm));
      if(CPassiveComm::Allreduce(sampled.source_point_queries,CPassiveComm::Op::SUM,parent.comm)>0)
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
  CTurbSolver* Turbulence(){return static_cast<CTurbSolver*>(solver_container[ZONE_0][INST_0][MESH_0][TURB_SOL]);}
  std::unique_ptr<CRemesher> Backend(){return MakeRemesher();}
  std::shared_ptr<ReferenceState> Reference(){return nativeReference3D;}
};
}
TEST_CASE("Affine authority survives actual Euler and RANS original-source import", "[NativeAffineImport3D]") {
  const bool rans=GENERATE(false,true);World world;const std::string name=rans?"affine_import_rans":"affine_import_euler";
  const std::string ransOptions="SOLVER= RANS\nKIND_TURB_MODEL= SA\nREYNOLDS_NUMBER= 1e6\nREYNOLDS_LENGTH= 1\n";
  auto config=MakeConfig(3,(rans?ransOptions:std::string("SOLVER= EULER\n"))+
      "TIME_DOMAIN= YES\nTIME_STEP= .001\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n");
  auto mesh=BoxMesh(3,2,false);
  for(size_t p=0;p<mesh.GetnPoint();++p)mesh.coord[3*p+1]+=.37*mesh.coord[3*p]+.61*mesh.coord[3*p+2];
  MeshSolution donor(config.get(),mesh,0);const auto saved=State(donor);ReferenceState reference;
  ImportGeometry(*config,donor.Fine(),nullptr,reference);
  const SU2NativeGeometry::AffineAuthority definition{17,3,3,{0,0,0},{-.37,1,-.61},1e-13};
  std::map<Id,SU2NativeGeometry::AffineAuthority> planes;
  for(const auto& f:reference.facets)if(reference.marker_names.at(f.marker)=="y_minus")planes.emplace(f.id,definition);
  REQUIRE(planes.size()==8);
  // A disjoint analytic collar makes the largest generated node differ from the original span.
  std::map<Id,AnalyticSurface> curves;Id originalMaximum=0;
  for(const auto& f:reference.facets)for(auto n:f.v)originalMaximum=std::max(originalMaximum,n.id);
  for(const auto& f:reference.facets)if(curves.empty() && std::all_of(f.v.begin(),f.v.end(),[&](Node n){
        return n.p.z==1 && n.p.y-.37*n.p.x-.61*n.p.z>.25 && n.id!=originalMaximum;})) {
    AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.origin={0,0,1};g.extent={1,3,0};curves.emplace(f.id,g);
  }
  REQUIRE(curves.size()==1);
  reference.original=std::make_shared<PlanarReference>(reference.facets,SIZE_MAX,curves,planes);
  if(rans) {
    auto wallConfig=MakeConfig(3,ransOptions+"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_HESSIAN_NOISE= 0\n"
        "ADAP_BL_MARKER= (z_minus_a,z_minus_b)\n"
        "ADAP_BL_FIRST_HEIGHT= (.1,.1)\nADAP_BL_GROWTH= (1.2,1.2)\nADAP_BL_THICKNESS= (.3,.3)\n");
    REQUIRE(wallConfig->GetnAdap_BL()==2);std::map<Id,SU2NativeGeometry::AffineAuthority> wallPlanes;
    auto g=definition;g.identity=19;g.normal={0,0,1};
    for(const auto& f:reference.facets)if(reference.marker_names.at(f.marker).find("z_minus_")==0)wallPlanes.emplace(f.id,g);
    REQUIRE(wallPlanes.size()==8);ReferenceState wallReference=reference;
    wallReference.original=std::make_shared<PlanarReference>(reference.facets,SIZE_MAX,std::map<Id,AnalyticSurface>{},wallPlanes);
    try {CBoundaryLayerMetric::FromNativeReference(wallReference,*wallConfig);FAIL("An affine BL wall must not freeze a faceted substitute.");}
    catch(const std::invalid_argument& e){CHECK(std::string(e.what()).find("BL on declared affine wall")!=std::string::npos);}
  }
  const auto encoded=EncodeReference(reference);CHECK(encoded.find("SU2_NATIVE_REFERENCE_3D 4\n")==0);
  auto reloaded=DecodeReference(encoded);CHECK(EncodeReference(reloaded)==encoded);
  CHECK_THROWS(DecodeReference(encoded+"trailing"));CHECK_THROWS(DecodeReference(encoded,1));
  auto conflict=planes;conflict.begin()->second.version++;
  CHECK_THROWS(PlanarReference(reference.facets,SIZE_MAX,curves,conflict));
  su2activematrix metric(donor.Fine().GetnPointDomain(),6);
  for(size_t p=0;p<metric.rows();++p) {
    const auto* x=donor.Fine().nodes->GetCoord(p);
    for(unsigned k=0;k<6;++k)metric(p,k)=0;
    metric(p,0)=2+.125*x[0];metric(p,3)=3+.125*x[2];metric(p,5)=4;
  }
  const auto imported=ImportGeometry(*config,donor.Fine(),&metric,reference);
  uint64_t extensions=0;for(const auto& row:imported.original)extensions+=row.extension_component!=UINT64_MAX;
  CHECK(CPassiveComm::Allreduce(extensions,CPassiveComm::Op::SUM,world.comm)==3*planes.size()+3);
  const auto span=OriginalSourceNodeSpan(world,imported.original);std::map<Id,Point> physicalNodes;
  for(const auto& f:reference.facets)for(auto n:f.v)physicalNodes.emplace(n.id,n.p);
  for(const auto& d:imported.original)if(planes.count(d.extension_component))for(unsigned i=0;i<4;++i) {
    const auto n=d.cell.v[i];const auto original=n.id%span;REQUIRE(physicalNodes.count(original));
    CHECK(physicalNodes.at(original).x==n.p.x);CHECK(physicalNodes.at(original).z==n.p.z);
    CHECK(d.sensor[i].xx==Approx(2+.125*physicalNodes.at(original).x));
    CHECK(d.sensor[i].yy==Approx(3+.125*physicalNodes.at(original).z));
  }
  DonorImport source(world,true);std::string reason;REQUIRE(source.Build(imported.original,reason));
  const Point outside{.25,.37*.25+.61*.25-5e-14,.25},inside{.25,.37*.25+.61*.25+5e-14,.25};
  REQUIRE(SU2NativeGeometry::AffineChart(definition).ErrorBound(outside)<definition.maximum_error);
  const auto answer=source.SamplePoints({outside,inside},reference.original->ExtensionComponents());
  INFO(answer.reason);REQUIRE(answer.valid);REQUIRE(answer.samples.size()==2);
  CHECK(answer.samples[0].extension_component!=UINT64_MAX);CHECK(answer.samples[1].extension_component==UINT64_MAX);
  for(const auto& row:answer.samples){CHECK(row.sensor.xx==Approx(2+.125*.25));CHECK(row.sensor.yy==Approx(3+.125*.25));CHECK(row.sensor.zz==4);}
  const auto received=source.Import({{0,-1,0,1,3,1}});INFO(received.reason);REQUIRE(received.valid);
  const auto associated=AssociatedExtensions(received.cells,*reference.original);REQUIRE(associated.size()==2);
  unsigned compositions=0;
  FrozenField field(received.cells,[&](Point p,Tensor sensor){++compositions;sensor.zz+=1+.5*p.x;return sensor;},false,associated,
                    [](Point,Point){return std::vector<double>{0,1};});
  for(unsigned i=0;i<2;++i){const auto sample=field.Query(outside);CHECK(sample.sensor.xx==answer.samples[0].sensor.xx);CHECK(sample.target.zz==5.125);}
  CHECK(compositions==2);CHECK(field.Statistics().hits==1);
  CHECK(field.Query(inside).extension_component==UINT64_MAX);
  CHECK(field.TargetEdgeLength(outside,inside)==Approx((inside.y-outside.y)*std::sqrt(3+.125*.25)).epsilon(1e-10));
  FrozenField undeclared(received.cells);CHECK_THROWS(undeclared.Query(outside));
  CHECK(State(donor)==saved);CHECK(EncodeReference(reference)==encoded);
  WriteReference(name+".native_ref",reference);ReferenceState restored;REQUIRE(LoadReference(name+".native_ref",restored));
  CHECK(EncodeReference(restored)==encoded);Save(world,name,imported.cells,imported.original,reference);
  // A second declared plane at this junction requires a simultaneous source collar; failure is collective/atomic.
  auto joined=planes;auto other=definition;other.identity=18;other.normal={1,0,0};
  for(const auto& f:reference.facets)if(reference.marker_names.at(f.marker)=="x_minus")joined.emplace(f.id,other);
  ReferenceState junction=reference;junction.original=std::make_shared<PlanarReference>(junction.facets,SIZE_MAX,curves,joined);
  ImportedGeometry retained=imported;BridgeStats stats;const auto retainedBytes=Bytes(retained.original);const auto beforeJunction=EncodeReference(junction);
  CHECK_FALSE(ImportGeometry(*config,donor.Fine(),&metric,junction,retained,reason,stats,size_t(128)<<20));
  CHECK(reason.find("joint affine/analytic original-source envelope")!=std::string::npos);
  CHECK(Bytes(retained.original)==retainedBytes);CHECK(EncodeReference(junction)==beforeJunction);CHECK(State(donor)==saved);
  if(world.size>1) {
    auto mismatch=planes;if(!world.rank)for(auto& row:mismatch)row.second.version++;
    ReferenceState different=reference;different.original=std::make_shared<PlanarReference>(different.facets,SIZE_MAX,curves,mismatch);
    CHECK_FALSE(ImportGeometry(*config,donor.Fine(),&metric,different,retained,reason,stats,size_t(128)<<20));
    CHECK(reason.find("geometry declarations differ")!=std::string::npos);CHECK(State(donor)==saved);CHECK(Bytes(retained.original)==retainedBytes);
  }
}

TEST_CASE("Native3D immutable reference encoding rejects malformed authority", "[NativeReference3D]") {
  World world;auto config=MakeConfig(3,"SOLVER= EULER\n");MeshSolution cube(config.get(),BoxMesh(3,1,false),0);
  ReferenceState original;ImportGeometry(*config,cube.Fine(),nullptr,original);
  const auto text=EncodeReference(original);auto copy=DecodeReference(text);CHECK(EncodeReference(copy)==text);
  CHECK(copy.original!=original.original);CHECK(copy.facets.size()==original.facets.size());
  CHECK_THROWS(EncodeReference(ReferenceState{}));CHECK_THROWS(DecodeReference(text+"junk"));
  CHECK_THROWS(DecodeReference(text, 8*text.size()-1));
  const auto large = text + std::string(1024*1024, ' ');
  CHECK(EncodeReference(DecodeReference(large)) == text);
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

// Collective refusal is recoverable and must leave current/BDF state unpublished.
TEST_CASE("Native3D acceptance veto retains current and BDF state before publication", "[NativePolicyVeto3D]") {
  World world;const int veto=std::getenv("SU2_NATIVE3D_VETO_CALL") ? std::stoi(std::getenv("SU2_NATIVE3D_VETO_CALL")) : 1;
  REQUIRE(veto==1);
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
  CBarycentricTransfer transfer;
  CHECK_FALSE(driver->ReplaceMesh(remeshed,transfer));
  CHECK(calls==1);CHECK(State(driver->Geometry(),driver->Solver())==before);
  CHECK_FALSE(driver->Reference()->original);
  remeshed.beforeAcceptance={};
  remeshed.status=world.rank==0 ? CRemeshResult::Status::INCOMPLETE_QUALITY : CRemeshResult::Status::COMPLETE;
  CHECK_FALSE(driver->ReplaceMesh(remeshed,transfer));
  CHECK(State(driver->Geometry(),driver->Solver())==before);CHECK_FALSE(driver->Reference()->original);
  remeshed.status=CRemeshResult::Status::COMPLETE;
  REQUIRE(driver->ReplaceMesh(remeshed,transfer));CHECK(driver->Reference()->original);
}

TEST_CASE("Native3D reference records the first-height tolerance and refuses another", "[NativeReference3D]") {
  ReferenceState state;
  state.marker_names={"wall","plane"};
  const std::array<Node,4> nodes{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}};
  for(auto face:Incidence({Cell{0,nodes}}).Boundary()) {
    const bool wall=std::find(face.begin(),face.end(),0)==face.end();
    state.facets.push_back({Id(state.facets.size()),wall?0:1,{nodes[face[0]],nodes[face[1]],nodes[face[2]]}});
  }
  state.original=std::make_shared<PlanarReference>(state.facets);
  const auto legacy=EncodeReference(state);
  CHECK(legacy.find("BL_FIRST_HEIGHT_TOL")==std::string::npos);CHECK(DecodeReference(legacy).first_height_tol==0);
  state.first_height_tol=.1;
  const auto text=EncodeReference(state);const auto decoded=DecodeReference(text);
  CHECK(decoded.first_height_tol==.1);CHECK(EncodeReference(decoded)==text);
  CHECK_THROWS(DecodeReference(text+"BL_FIRST_HEIGHT_TOL 0.1\n"));
  CHECK_THROWS(DecodeReference(legacy+"BL_FIRST_HEIGHT_TOL 0\n"));
  WriteReference("first_height_tol.native_ref",state);
  ReferenceState loaded;REQUIRE(LoadReference("first_height_tol.native_ref",loaded));CHECK(loaded.first_height_tol==.1);
  CHECK_THROWS(SU2NativeFirstHeight::AdoptRecorded(loaded.first_height_tol,.125));
  CHECK(SU2NativeFirstHeight::AdoptRecorded(loaded.first_height_tol,.1)==.1);
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  if(!SU2_MPI::GetRank())std::remove("first_height_tol.native_ref");
  SU2_MPI::Barrier(SU2_MPI::GetComm());
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
namespace {
Point SourceFrame(Point p) {return {2.1+p.x+.2*p.y-.1*p.z,-.7+.37*p.x+p.y+.61*p.z,.8+.15*p.x-.25*p.y+p.z};}
CSimplexMesh JointSourceMesh(bool gap) {
  auto mesh=BoxMesh(3,1,false);
  if(gap) {
    const auto old=mesh;const auto offset=mesh.GetnPoint();
    for(size_t i=0;i<old.GetnPoint();++i){mesh.coord.push_back(old.coord[3*i]+1.01);mesh.coord.push_back(old.coord[3*i+1]);mesh.coord.push_back(old.coord[3*i+2]);}
    for(auto node:old.elem)mesh.elem.push_back(node+offset);
    mesh.elemRef.insert(mesh.elemRef.end(),old.elemRef.begin(),old.elemRef.end());
    for(size_t i=0;i<mesh.markers.size();++i)for(auto node:old.markers[i].elem)mesh.markers[i].elem.push_back(node+offset);
  }
  for(size_t i=0;i<mesh.GetnPoint();++i) {
    const auto p=SourceFrame({mesh.coord[3*i],mesh.coord[3*i+1],mesh.coord[3*i+2]});
    mesh.coord[3*i]=p.x;mesh.coord[3*i+1]=p.y;mesh.coord[3*i+2]=p.z;
  }
  return mesh;
}
std::unique_ptr<CConfig> JointSourceConfig(bool rans) {
  const std::string physics=rans?"SOLVER= RANS\nKIND_TURB_MODEL= SA\nREYNOLDS_NUMBER= 1e6\nREYNOLDS_LENGTH= 1\n":"SOLVER= EULER\n";
  return MakeConfig(3,physics+"TIME_DOMAIN= YES\nTIME_STEP= .001\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nADAP_REMESHER= NATIVE_CAVITY\nADAP_NATIVE_MAX_TIME= 30\n");
}
void JointSourceReference(ReferenceState& state,bool gap,double error=1e-13) {
  // Exact declared carriers come from the affine frame; input rounding is covered explicitly.
  SU2NativeGeometry::AffineAuthority x{17,1,3,SourceFrame({0,0,0}),{1.1525,-.175,.222},error};
  auto y=x;y.identity=18;y.normal={-.2785,1.015,-.647};
  auto second=x;second.identity=19;second.origin=SourceFrame({1.01,0,0});
  std::map<Id,SU2NativeGeometry::AffineAuthority> affine;
  for(const auto& f:state.facets) {
    const auto& marker=state.marker_names.at(f.marker);
    if(marker=="y_minus")affine.emplace(f.id,y);
    if(marker=="x_minus") {
      const bool first=std::all_of(f.v.begin(),f.v.end(),[&](Node n){return SU2NativeGeometry::AffineChart(x).ErrorBound(n.p)<error;});
      REQUIRE((first || (gap && std::all_of(f.v.begin(),f.v.end(),[&](Node n){return SU2NativeGeometry::AffineChart(second).ErrorBound(n.p)<error;}))));
      affine.emplace(f.id,first?x:second);
    }
  }
  REQUIRE(affine.size()>2);state.original=std::make_shared<PlanarReference>(state.facets,SIZE_MAX,std::map<Id,AnalyticSurface>{},affine);
}
}
TEST_CASE("Joint affine import freezes a conforming owner-sourced shell through N/M migration", "[NativeJointAffineSource3D]") {
  World parent;const bool rans=GENERATE(false,true);const int workers=GENERATE(1,2,4);if(workers>parent.size)return;
  auto config=JointSourceConfig(rans);
  MeshSolution mesh(config.get(),JointSourceMesh(false),0);const auto field=AffineFlow(3);
  for(size_t p=0;p<mesh.Fine().GetnPoint();++p) {
    su2double values[MAXVAR]{};field(mesh.Fine().nodes->GetCoord(p),values);
    auto* flow=mesh.solver[MESH_0][FLOW_SOL]->GetNodes();
    for(unsigned k=0;k<5;++k){flow->SetSolution(p,k,values[k]);flow->GetSolution_time_n()(p,k)=values[k]*.984375;flow->GetSolution_time_n1()(p,k)=values[k]*.96875;}
    if(rans){auto* turb=mesh.solver[MESH_0][TURB_SOL]->GetNodes();const double v=.002*(1+.1*std::abs(SU2_TYPE::GetValue(mesh.Fine().nodes->GetCoord(p,0))));
      turb->SetSolution(p,0,v);turb->GetSolution_time_n()(p,0)=v*.875;turb->GetSolution_time_n1()(p,0)=v*.75;}
  }
  const auto saved=State(mesh);const auto saved_turb=rans?State(mesh.Fine(),*mesh.solver[MESH_0][TURB_SOL]):std::vector<char>{};ReferenceState state;
  su2activematrix metric(mesh.Fine().GetnPointDomain(),6);
  for(size_t p=0;p<metric.rows();++p){const auto m=Sensor(mesh.Fine().nodes->GetCoord(p),4);const std::array<double,6> row{m.xx,m.xy,m.xz,m.yy,m.yz,m.zz};for(unsigned k=0;k<6;++k)metric(p,k)=row[k];}
  const auto plain=ImportGeometry(*config,mesh.Fine(),&metric,state);JointSourceReference(state,false);
  const auto encoded=EncodeReference(state);const auto imported=ImportGeometry(*config,mesh.Fine(),&metric,state);
  CHECK(imported.stats.source_envelope_nodes==8);CHECK(imported.stats.source_envelope_cells==3*state.facets.size());
  CHECK(imported.stats.source_envelope_solves>0);CHECK(imported.stats.source_envelope_contacts>0);CHECK(imported.stats.source_envelope_distances>0);
  const auto span=OriginalSourceNodeSpan(parent,imported.original);const auto global=parent.metadata(imported.original);std::map<Id,Node> old,outer;
  std::map<Id,Tensor> sensors;
  for(const auto& d:global)if(d.extension_component==UINT64_MAX)for(unsigned k=0;k<4;++k){old.emplace(d.cell.v[k].id,d.cell.v[k]);sensors.emplace(d.cell.v[k].id,d.sensor[k]);}
  for(const auto& d:global)if(d.extension_component!=UINT64_MAX)for(unsigned k=0;k<4;++k) {
    const auto n=d.cell.v[k];const auto id=n.id%span;REQUIRE(sensors.count(id));
    CHECK(Bytes(std::vector<Tensor>{d.sensor[k]})==Bytes(std::vector<Tensor>{sensors.at(id)}));
    if(n.id>=span)outer.emplace(id,n);
  }
  REQUIRE(old.size()==8);REQUIRE(outer.size()==8);
  std::set<Id> first,second;
  for(const auto& f:state.facets)if(state.original->Affine().count(f.id))for(auto n:f.v)
    (state.original->Affine().at(f.id).identity==17?first:second).insert(n.id);
  std::vector<Id> junction;std::set_intersection(first.begin(),first.end(),second.begin(),second.end(),std::back_inserter(junction));
  REQUIRE_FALSE(junction.empty());const auto corner=old.at(junction.front()).p;const auto out=outer.at(junction.front()).p;
  const Point q{double((long double)corner.x+((long double)out.x-corner.x)/32),
                double((long double)corner.y+((long double)out.y-corner.y)/32),
                double((long double)corner.z+((long double)out.z-corner.z)/32)};
  const auto inside=SourceFrame({.25,.25,.25});DonorImport service(parent,true);std::string reason;REQUIRE(service.Build(imported.original,reason));
  const auto sample=service.SamplePoints({q,inside},state.original->ExtensionComponents());INFO(sample.reason);REQUIRE(sample.valid);
  REQUIRE(sample.samples.size()==2);CHECK(sample.samples[0].extension_component!=UINT64_MAX);CHECK(sample.samples[1].extension_component==UINT64_MAX);
  FrozenField frozen(global,{},false,AssociatedExtensions(global,*state.original)),original(global);
  const auto a=frozen.Query(inside),b=original.Query(inside);CHECK(a.donor==b.donor);CHECK(a.weights==b.weights);
  CHECK(Bytes(std::vector<Tensor>{a.sensor})==Bytes(std::vector<Tensor>{b.sensor}));
  CHECK(double(frozen.SensorEdgeLength(inside,q))>0);
  unsigned calls=0;FrozenField composed(global,[&](Point p,Tensor s){++calls;s.zz+=1+.5*p.x;return s;},false,AssociatedExtensions(global,*state.original),[](Point,Point){return std::vector<double>{0,1};});
  for(unsigned i=0;i<2;++i){const auto answer=composed.Query(q);CHECK(answer.target.zz==Approx(answer.sensor.zz+(1+.5*q.x)).margin(1e-13));}
  CHECK(calls==2);CHECK(composed.Statistics().hits==1);
  auto working=imported.cells;std::map<Id,int> destinations;PartitionStats partition;
  REQUIRE(Repartition(parent,working,std::vector<uint32_t>(working.size(),1),workers,partition,reason));
  size_t position=0;for(const auto& row:imported.cells)destinations.emplace(row.first,partition.destinations.at(position++));
  std::vector<std::vector<DonorCell>> send(parent.size);
  for(auto d:imported.original)send[destinations.at(d.extension_component==UINT64_MAX?d.cell.id:d.parent_cell)].push_back(d);
  const auto migrated=parent.exchange(send);
  auto check=[&](World& worker) {
    DonorImport local(worker);std::string detail;REQUIRE(local.Build(migrated,detail));
    const auto answer=local.SamplePoints({q,inside},state.original->ExtensionComponents());INFO(answer.reason);REQUIRE(answer.valid);
    for(unsigned i=0;i<2;++i)CHECK(Bytes(std::vector<Tensor>{answer.samples[i].sensor})==Bytes(std::vector<Tensor>{sample.samples[i].sensor}));
  };
#ifdef HAVE_MPI
  MPI_Comm comm=MPI_COMM_NULL;MPI_Comm_split(parent.comm,parent.rank<workers?0:MPI_UNDEFINED,parent.rank,&comm);
  if(comm!=MPI_COMM_NULL){World worker(comm);check(worker);MPI_Comm_free(&comm);}
#else
  check(parent);
#endif
  CHECK(State(mesh)==saved);CHECK(EncodeReference(state)==encoded);
  ImportedGeometry retained=plain;BridgeStats refused;const auto before=Bytes(retained.original);
  CHECK_FALSE(ImportGeometry(*config,mesh.Fine(),&metric,state,retained,reason,refused,parent.rank?SIZE_MAX:0));
  CHECK(Bytes(retained.original)==before);CHECK(EncodeReference(state)==encoded);CHECK(State(mesh)==saved);
  if(parent.size>1)for(bool empty:{false,true}) {
    ReferenceState different=state;
    if(!parent.rank) {
      auto profiles=state.original->Affine();if(empty)profiles.clear();else for(auto& row:profiles)++row.second.version;
      different.original=std::make_shared<PlanarReference>(state.facets,SIZE_MAX,std::map<Id,AnalyticSurface>{},profiles);
    }
    const auto held=EncodeReference(different);
    CHECK_FALSE(ImportGeometry(*config,mesh.Fine(),&metric,different,retained,reason,refused,size_t(128)<<20));
    INFO(reason);CHECK(reason.find(empty?"presence differs":"geometry declarations differ")!=std::string::npos);
    CHECK(Bytes(retained.original)==before);CHECK(EncodeReference(different)==held);CHECK(State(mesh)==saved);
  }
  const auto name="joint_affine_source_"+std::string(rans?"rans":"euler")+"_m"+std::to_string(workers);
  Save(parent,name,imported.cells,imported.original,state);SaveCoverage(parent,name,imported.original,state);
  if(rans)CHECK(State(mesh.Fine(),*mesh.solver[MESH_0][TURB_SOL])==saved_turb);
  if(!parent.rank){std::ofstream out(name+"_preparation.json");const auto& b=imported.stats;
    out<<std::setprecision(17)<<"{\"seconds\":"<<b.source_envelope_seconds<<",\"payload_seconds\":"<<b.source_envelope_payload_seconds
       <<",\"replicated_logical_payload_bytes\":"<<b.source_envelope_payload_bytes<<",\"requested_native_bound_bytes\":"<<b.maximum_native_bytes
       <<",\"nodes\":"<<b.source_envelope_nodes<<",\"cells\":"<<b.source_envelope_cells<<",\"column_work\":"<<b.source_envelope_solves
       <<",\"contacts\":"<<b.source_envelope_contacts<<",\"distances\":"<<b.source_envelope_distances<<"}\n";CHECK(out.good());}
}
TEST_CASE("Joint source rejects a narrow disconnected gap without publishing its shell", "[NativeJointAffineSource3D]") {
  World world;auto config=JointSourceConfig(false);
  MeshSolution mesh(config.get(),JointSourceMesh(true),0);const auto saved=State(mesh);ReferenceState state;
  const auto plain=ImportGeometry(*config,mesh.Fine(),nullptr,state);JointSourceReference(state,true,1e-3);
  const auto encoded=EncodeReference(state);auto retained=plain;const auto before=Bytes(retained.original);BridgeStats attempted;std::string reason;
  CHECK_FALSE(ImportGeometry(*config,mesh.Fine(),nullptr,state,retained,reason,attempted,size_t(128)<<20));
  INFO(reason);CHECK(reason.find("joint original-source contact")!=std::string::npos);
  CHECK(Bytes(retained.original)==before);CHECK(EncodeReference(state)==encoded);CHECK(State(mesh)==saved);
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
  declared.first_height_tol=SU2_TYPE::GetValue(driver.Config().GetAdap_BL_FirstHeightTol());
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
  REQUIRE(p.targetTotal.size()%s.nTimeLevels==0);
  const size_t perLevel=p.targetTotal.size()/s.nTimeLevels;
  REQUIRE(perLevel>=s.nVarFlow);const size_t nTurb=perLevel-s.nVarFlow;
  REQUIRE(s.turbulenceChange.size()==s.nTimeLevels*nTurb);
  CHECK(std::abs(p.targetVolume-p.donorVolume-p.fillVolume+p.sliverVolume)<1e-10);
  for(size_t f=0;f<p.targetTotal.size();++f) {
    // Solver turbulence bounds are the documented exception; all changes must be accounted for.
    const size_t variable=f%perLevel;const double scale=std::max(1e-300,double(p.scale[f]));
    const double bounded=variable<s.nVarFlow?0:s.turbulenceChange[(f/perLevel)*nTurb+variable-s.nVarFlow]*scale;
    if(bounded!=0)CHECK(s.nTurbLimited+s.nTurbLimitedStage2>0);
    CHECK(std::abs(s.newIntegral[f]-p.targetTotal[f]-bounded)<=1e-10*scale);
    CHECK(std::abs(p.targetTotal[f]-p.donorTotal[f]-p.fillOpen[f]+p.sliverOpen[f])<=1e-10*scale);
  }
  if(world.rank)return;
  std::ofstream out(name+"_projection.json");out<<std::setprecision(17)
    <<"{\"donor_volume\":"<<p.donorVolume<<",\"target_volume\":"<<p.targetVolume
    <<",\"fill_volume\":"<<p.fillVolume<<",\"lost_volume\":"<<p.sliverVolume<<",\"time_levels\":"<<s.nTimeLevels
    <<",\"flow_fields\":"<<s.nVarFlow<<",\"turbulence_limited\":"<<s.nTurbLimited
    <<",\"turbulence_limited_stage2\":"<<s.nTurbLimitedStage2;
  auto values=[&](const char* key,const auto& v){out<<",\""<<key<<"\":[";for(size_t i=0;i<v.size();++i){if(i)out<<',';out<<v[i];}out<<']';};
  values("donor_total",p.donorTotal);values("target_total",p.targetTotal);values("new_total",s.newIntegral);
  values("fill",p.fill);values("lost",p.sliver);values("fill_open",p.fillOpen);values("lost_open",p.sliverOpen);
  values("scale",p.scale);values("residual",p.residual);values("turbulence_bound_change",s.turbulenceChange);
  out<<"}\n";CHECK(out.good());
}
void CheckPositiveFlow(const std::vector<FlowRecord>& records) {
  for(const auto& row:records)for(size_t level=0;level<3;++level) {
    const auto* u=row.values.data()+5*level;CHECK(u[0]>0);
    CHECK(u[4]-(u[1]*u[1]+u[2]*u[2]+u[3]*u[3])/(2*u[0])>0);
  }
}
std::unique_ptr<TetraDriver> ReloadCurved(World& world,std::unique_ptr<TetraDriver> driver,
                                        const std::string& name,const std::string& mesh,const std::string& format,
                                        const std::string& configuration = {}) {
  driver->Config().SetTimeIter(2);const auto before=State(driver->Geometry(),driver->Solver());
  const auto expected=Flow(world,driver->Geometry(),driver->Solver());driver->WriteTimeHistoryRestarts();
  CHECK(State(driver->Geometry(),driver->Solver())==before);
  if(!world.rank) {
    std::ifstream input(name+".cfg");std::string text=configuration.empty()?std::string((std::istreambuf_iterator<char>(input)),{}):configuration;
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
  ReferenceState declared;ImportGeometry(driver->Config(),driver->Geometry(),nullptr,declared);
  declared.first_height_tol=SU2_TYPE::GetValue(driver->Config().GetAdap_BL_FirstHeightTol());
  DeclareImportedCurve(declared,false,amplitude);
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
  ReferenceState declared;ImportGeometry(driver->Config(),driver->Geometry(),nullptr,declared);
  declared.first_height_tol=SU2_TYPE::GetValue(driver->Config().GetAdap_BL_FirstHeightTol());
  DeclareImportedCurve(declared,false,amplitude);
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
CSimplexMesh PrimitiveMesh(bool sphere,bool tilted,double length=1) {
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
      add({origin.x+level*length*axis.x,origin.y+level*length*axis.y,origin.z+level*length*axis.z});
      for(int i=0;i<8;++i){const auto a=i*std::acos(-1.)/4;add({origin.x+std::cos(a)*u.x+std::sin(a)*v.x+level*length*axis.x,origin.y+std::cos(a)*u.y+std::sin(a)*v.y+level*length*axis.y,origin.z+std::cos(a)*u.z+std::sin(a)*v.z+level*length*axis.z});}
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
void DeclarePrimitive(ReferenceState& state,bool sphere,bool tilted,double length=1) {
  AnalyticSurface g;g.kind=sphere?AnalyticSurface::Kind::Sphere:AnalyticSurface::Kind::Cylinder;g.extent=sphere?Point{1,0,0}:Point{1,length,0};
  if(tilted && !sphere){g.origin={2,-1,.5};g.axis={1,1,1};}
  std::map<Id,AnalyticSurface> curves;
  for(auto f:state.facets) {
    auto local=g;const auto name=state.marker_names.at(f.marker);
    if(name=="cap0" || name=="cap1") {
      local.kind=AnalyticSurface::Kind::Disk;local.extent.y=0;
      if(name=="cap1") {const auto l=std::hypot(g.axis.x,g.axis.y,g.axis.z);local.origin={g.origin.x+length*g.axis.x/l,g.origin.y+length*g.axis.y/l,g.origin.z+length*g.axis.z/l};}
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
  ReferenceState declared;ImportGeometry(driver->Config(),driver->Geometry(),nullptr,declared);
  declared.first_height_tol=SU2_TYPE::GetValue(driver->Config().GetAdap_BL_FirstHeightTol());
  DeclarePrimitive(declared,sphere,tilted);
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

namespace {
#include "../../Common/adaptation/fixtures/native_joint_obstruction.inc"
#include "../../Common/adaptation/fixtures/native_strip_obstruction.inc"
}
TEST_CASE("Actual CFD import freezes a conforming cylinder-cap collar for joint strip edits",
          "[NativeCylinderCoverage3D]") {
  World world;const bool varying=GENERATE(false,true);
  const auto reference=JointReference();const auto saved=StripSource();CSimplexMesh seed;seed.nDim=3;
  std::map<Id,Point> points;
  for(const auto& d:saved)if(d.extension_component==UINT64_MAX) {
    for(auto n:d.cell.v)points.emplace(n.id,n.p);
    for(auto n:d.cell.v)seed.elem.push_back(n.id);
  }
  REQUIRE(points.size()==18);for(Id id=0;id<18;++id){const auto p=points.at(id);seed.coord.insert(seed.coord.end(),{p.x,p.y,p.z});}
  seed.elemRef.assign(seed.GetnElem(),0);
  const std::array<std::string,3> names{{"x_minus","x_plus","y_minus"}};
  for(size_t marker=0;marker<3;++marker) {
    CSimplexMesh::Marker tag;tag.name=names[marker];
    for(Id id=0;id<32;++id){const auto& f=reference.Get(id);if(f.marker==int(marker))for(auto n:f.v)tag.elem.push_back(n.id);}
    seed.markers.push_back(tag);
  }
  auto config=MakeConfig(3,"SOLVER= EULER\nADAP_HAUSD= 0.04\nADAP_HESSIAN_NOISE= 0\n");
  MeshSolution mesh(config.get(),seed,0);su2activematrix sensor(mesh.Fine().GetnPointDomain(),6);
  for(unsigned long n=0;n<sensor.rows();++n) {
    const auto x=SU2_TYPE::GetValue(mesh.Fine().nodes->GetCoord(n)[0]);
    for(size_t j=0;j<6;++j)sensor(n,j)=j==0||j==3||j==5 ? .5+(varying?.01*(x-2):0) : 0;
  }
  ReferenceState state;state.original=std::make_shared<PlanarReference>(reference);
  for(Id id=0;id<32;++id)state.facets.push_back(reference.Get(id));state.marker_names.assign(names.begin(),names.end());
  // This source is generated from the accepted original CFD mesh before any private strip proposal exists.
  const auto imported=ImportGeometry(*config,mesh.Fine(),&sensor,state);
  CHECK(CPassiveComm::Allreduce(uint64_t(imported.original.size()),CPassiveComm::Op::SUM,world.comm)==120);
  DonorImport service(world);std::string reason;REQUIRE(service.Build(imported.original,reason));
  const auto loaded=service.Import({DonorRegion{-4,-4,-4,4,4,4}});INFO(loaded.reason);REQUIRE(loaded.valid);
  REQUIRE(loaded.cells.size()==120);
  FrozenField field(loaded.cells,{},false,AssociatedExtensions(loaded.cells,*state.original)),original_only(loaded.cells);
  std::map<Id,Tensor> original_tensors;
  for(const auto& d:loaded.cells)if(d.extension_component==UINT64_MAX)for(size_t j=0;j<4;++j)original_tensors.emplace(d.cell.v[j].id,d.sensor[j]);
  const auto span=original_tensors.rbegin()->first+1;
  for(const auto& d:loaded.cells)if(d.extension_component!=UINT64_MAX)for(size_t j=0;j<4;++j) {
    const auto id=d.cell.v[j].id;const auto old=original_tensors.at(id<span ? id : id-span);
    CHECK(d.sensor[j].xx==old.xx);CHECK(d.sensor[j].yy==old.yy);CHECK(d.sensor[j].zz==old.zz);
  }
  for(const auto& d:loaded.cells)if(d.extension_component==UINT64_MAX) {
    Point center{};for(auto n:d.cell.v){center.x+=n.p.x/4;center.y+=n.p.y/4;center.z+=n.p.z/4;}
    const auto original=original_only.Query(center),actual=field.Query(center);
    CHECK(actual.donor==original.donor);CHECK(actual.weights==original.weights);CHECK(actual.sensor.xx==original.sensor.xx);
  }
  Patch fresh;const auto old=StripObstruction();
  REQUIRE(SplitSurfaceStrip(old,{12,1604},StripPoint(),StripMoves(),300000,*state.original,fresh,reason));
  PatchMetric measured;const bool valid=ValidateMetricChange(old.cells,fresh.cells,MetricChange::GeometryRefine,field,measured,reason);
  INFO(reason);REQUIRE(valid);CHECK(measured.minimum_mean_ratio>.204);CHECK(measured.minimum_scaled_jacobian>.055);
  CHECK(field.Statistics().extended_edge_pieces>0);
  for(const auto& m:StripMoves())CHECK(field.Query(m.second.p).sensor.xx>0);
  // The collar covers representability near the declared cap, while remaining a bounded extension.
  const Point far_cap{2+2/std::sqrt(3.),-1+2/std::sqrt(3.),.5+2/std::sqrt(3.)};CHECK_THROWS(field.Query(far_cap));
  const auto name=std::string("generated_cylinder_cap_")+(varying?"varying":"constant");
  SaveCoverage(world,name,imported.original,state);
  Owned proposed;
  for(const auto& c:fresh.cells)if(c.id%world.size==unsigned(world.rank)) {
    OwnedCell owned;owned.cell=c;
    for(size_t slot=0;slot<4;++slot)for(const auto& f:fresh.surface){Face a{};size_t at=0;for(size_t k=0;k<4;++k)if(k!=slot)a[at++]=c.v[k].id;auto b=f.v;std::sort(a.begin(),a.end());std::sort(b.begin(),b.end());if(a==b)owned.physical[slot]={true,f.facet};}
    proposed.emplace(c.id,owned);
  }
  Save(world,name+"_strip",proposed,imported.original,state);
}

TEST_CASE("Resolved three-dimensional Euler vortex drives native curved adaptation and restart",
          "[NativeResolvedEuler3D]") {
  World world;const int workers=GENERATE(1,2,4);if(workers>world.size)return;
  const auto* selected=std::getenv("SU2_NATIVE_RESOLVED_WORKERS");const auto only=selected?std::stoi(selected):0;REQUIRE((only==0 || only==1 || only==2 || only==4));if(only && workers!=only)return;
  const auto name="resolved_vortex_m"+std::to_string(workers);constexpr double amplitude=-.001,radius=.7,beta=3.;
  if(!world.rank) {
    auto marker=[](const passivedouble* p) {
      if(p[2]==0)return std::string(p[0]<=.5?"z_minus_a":"z_minus_b");
      if(p[2]==.25)return std::string("z_plus");
      if(p[0]==0)return std::string("x_minus");if(p[0]==1)return std::string("x_plus");
      if(p[1]==0)return std::string("y_minus");return std::string("y_plus");
    };
    auto mesh=simplex_test::MakeSimplexMesh(3,12,marker,[](const passivedouble* p){return p[2]<.25;});
    REQUIRE(mesh.GetnPoint()==676);REQUIRE(mesh.GetnElem()==2592);
    for(size_t p=0;p<mesh.coord.size();p+=3) {
      const auto u=mesh.coord[p],v=mesh.coord[p+1],z=4*mesh.coord[p+2];
      mesh.coord[p]=-2+4*u;mesh.coord[p+1]=-2+4*v;mesh.coord[p+2]=z+(1-z)*amplitude*u*(1-u)*v*(1-v);
    }
    simplex_test::WriteSU2Mesh(mesh,name+".su2");std::ofstream cfg(name+".cfg");
    cfg<<"SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nAOA= 0\nREF_DIMENSIONALIZATION= FREESTREAM_VEL_EQ_ONE\nMESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\n"
       <<"MARKER_FAR= (x_minus, x_plus, y_minus, y_plus)\nMARKER_EULER= (z_plus, z_minus_a, z_minus_b)\n"
       <<"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
       <<"ADAP_COMPLEXITY= 400\nADAP_HMIN= .2\nADAP_HMAX= .9\nADAP_ARMAX= 2\nADAP_HAUSD= .003\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nADAP_NATIVE_MAX_WORK= 16384\n"
       <<"MGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\nCFL_NUMBER= .3\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-4\nTIME_ITER= 8\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 8\n"
       <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= SU2\nMESH_OUT_FILENAME= "<<name<<"_mesh\nCONV_FILENAME= "<<name<<"_history\n"
       <<"RESTART_FILENAME= "<<name<<"_flow\nSOLUTION_FILENAME= "<<name<<"_flow\nWRT_RESTART_COMPACT= YES\n";
  }
  SU2_MPI::Barrier(world.comm);const auto cfg=name+".cfg";
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  ReferenceState declared;ImportGeometry(driver->Config(),driver->Geometry(),nullptr,declared);
  declared.first_height_tol=SU2_TYPE::GetValue(driver->Config().GetAdap_BL_FirstHeightTol());
  std::map<Id,AnalyticSurface> curves;
  for(const auto& f:declared.facets)if(declared.marker_names.at(f.marker).rfind("z_minus",0)==0) {
    AnalyticSurface g;g.kind=AnalyticSurface::Kind::Bump;g.origin={-2,-2,0};g.extent={4,4,amplitude};curves.emplace(f.id,g);
  }
  declared.original=std::make_shared<PlanarReference>(declared.facets,SIZE_MAX,curves);
  WriteReference(name+".su2.native_ref",declared);const auto authority=EncodeReference(declared);
  driver->Preprocess(0);
  const double gamma=SU2_TYPE::GetValue(driver->Config().GetGamma()),rho0=SU2_TYPE::GetValue(driver->Solver().GetDensity_Inf()),p0=SU2_TYPE::GetValue(driver->Solver().GetPressure_Inf());
  const double u0=SU2_TYPE::GetValue(driver->Solver().GetVelocity_Inf(0)),v0=SU2_TYPE::GetValue(driver->Solver().GetVelocity_Inf(1));
  const double pi=std::acos(-1.);auto* nodes=driver->Solver().GetNodes();
  for(unsigned long p=0;p<driver->Geometry().GetnPoint();++p) {
    const auto* x=driver->Geometry().nodes->GetCoord(p);const double a=(SU2_TYPE::GetValue(x[0])+.25)/radius,b=SU2_TYPE::GetValue(x[1])/radius;
    const double shape=std::exp(1-a*a-b*b),theta=1-(gamma-1)*beta*beta*shape/(8*gamma*pi*pi);REQUIRE(theta>0);
    const double rho=rho0*std::pow(theta,1/(gamma-1)),pressure=p0*std::pow(theta,gamma/(gamma-1));
    const double swirl=beta/(2*pi)*std::sqrt(p0/rho0*shape),u=u0-swirl*b,v=v0+swirl*a;
    const std::array<double,5> state{{rho,rho*u,rho*v,0,pressure/(gamma-1)+rho*(u*u+v*v)/2}};
    for(size_t k=0;k<5;++k){nodes->SetSolution(p,k,state[k]);nodes->GetSolution_time_n()(p,k)=state[k];nodes->GetSolution_time_n1()(p,k)=state[k];}
  }
  const auto seeded=Flow(world,driver->Geometry(),driver->Solver());SaveStates(world,name+"_seed",seeded,seeded,0);
  std::set<double> profile;
  for(const auto& r:seeded)if(std::abs(r.point.y)<1e-12 && r.point.z>.2 && r.point.z<.5 && std::abs(r.point.x+.25)<=2*radius)profile.insert(r.point.x);
  REQUIRE(profile.size()>=8);
  if(!world.rank){std::ofstream out(name+"_physics.json");out<<std::setprecision(17)<<"{\"gamma\":"<<gamma<<",\"rho0\":"<<rho0<<",\"p0\":"<<p0<<",\"u0\":"<<u0<<",\"v0\":"<<v0<<",\"radius\":"<<radius<<",\"beta\":"<<beta<<",\"initial_profile_samples\":"<<profile.size()<<"}\n";}
  auto backend=driver->Backend();CConservativeTransfer transfer;
  for(int cycle=0;cycle<2;++cycle) {
    const auto initial=Flow(world,driver->Geometry(),driver->Solver());
    for(unsigned long step=cycle?3:0;step<(cycle?5:2);++step){if(step)driver->Preprocess(step);driver->Run();driver->Postprocess();driver->Update();}
    const auto solved=Flow(world,driver->Geometry(),driver->Solver());CheckPositiveFlow(solved);
    double change=0,minimum_pressure=p0;
    for(size_t p=0;p<solved.size();++p){for(size_t k=0;k<5;++k)change=std::max(change,RelDiff(initial[p].values[k],solved[p].values[k]));const auto& u=solved[p].values;minimum_pressure=std::min(minimum_pressure,(gamma-1)*(u[4]-(u[1]*u[1]+u[2]*u[2]+u[3]*u[3])/(2*u[0])));}
    CHECK(change>1e-8);CHECK(minimum_pressure/p0<.9);CHECK(minimum_pressure/p0>.5);
    driver->ComputeMetric();const auto& metric=driver->Solver().GetNodes()->GetMetric();
    const auto event=name+"_s"+std::to_string(cycle);auto observed=declared;const auto donor=ImportGeometry(driver->Config(),driver->Geometry(),&metric,observed);
    Save(world,event+"_initial",donor.cells,donor.original,observed);SaveCoverage(world,event,donor.original,observed);
    SaveStates(world,event+"_evolved",initial,solved,0);
    const auto result=backend->Remesh(driver->Config(),driver->Geometry(),metric);
    INFO("resolved vortex workers="<<workers<<" cycle="<<cycle);
    REQUIRE(result.status==CRemeshResult::Status::COMPLETE);
    driver->ReplaceMesh(result,transfer);SaveProjection(world,event,transfer);CHECK(EncodeReference(*driver->Reference())==authority);
    const auto transported=result.slices.FetchPointMetric(driver->Geometry());const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);SaveStates(world,event,solved,fresh,0);CheckPositiveFlow(fresh);
    su2activematrix accepted_metric(driver->Geometry().GetnPointDomain(),6);for(unsigned long p=0;p<accepted_metric.rows();++p)for(size_t k=0;k<6;++k)accepted_metric(p,k)=transported[6*p+k];
    auto retained=*driver->Reference();const auto accepted=ImportGeometry(driver->Config(),driver->Geometry(),&accepted_metric,retained);
    Save(world,event+"_accepted",accepted.cells,donor.original,retained);driver->WriteAdaptedMesh(cycle+1,cycle+1);
    if(!cycle){const auto checkpoint=driver->AdaptedMeshName(1,1)+driver->Config().GetMesh_Out_FileExtension();backend.reset();driver=ReloadCurved(world,std::move(driver),name,checkpoint,"SU2");backend=driver->Backend();}
  }
  driver->Preprocess(5);driver->Run();driver->Postprocess();driver->Update();CheckPositiveFlow(Flow(world,driver->Geometry(),driver->Solver()));
}

TEST_CASE("Native3D production wall policy preserves sensor donors and RANS histories across worker counts", "[NativeBLPolicyRuntime3D]") {
  World world;const int workers=GENERATE(1,2,4);if(workers>world.size)return;
  const auto* selected=std::getenv("SU2_NATIVE_POLICY_WORKERS");const auto only=selected?std::stoi(selected):0;
  REQUIRE((only==0 || only==1 || only==2 || only==4));REQUIRE(only<=world.size);
  if(only && workers!=only)return;
  const std::string name="policy_m"+std::to_string(workers);
  auto configText=[&](bool walls) {
    std::ostringstream cfg;
    cfg<<"SOLVER= RANS\nKIND_TURB_MODEL= SA\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.2\nAOA= 0\n"
       <<"REYNOLDS_NUMBER= 1e6\nREYNOLDS_LENGTH= 1\nMESH_FORMAT= SU2\nMESH_FILENAME= "<<name<<".su2\n"
       <<"MARKER_FAR= (far)\nMARKER_HEATFLUX= (wall,0)\nCOMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\n"
       <<"ADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\nADAP_ISO_CORNER= NO\n"
       <<"ADAP_HMIN= .1\nADAP_HMAX= 1.0000000000000002\nADAP_ARMAX= 100\nADAP_COMPLEXITY= 1\nADAP_HGRAD= 1.3\n"
       <<"ADAP_NATIVE_REPARTITION= YES\nADAP_NATIVE_RANKS= "<<workers<<"\nADAP_NATIVE_MAX_WORK= 4096\n"
       <<"WRT_PERFORMANCE= YES\nMGLEVEL= 0\nNUM_METHOD_GRAD= GREEN_GAUSS\nCONV_NUM_METHOD_FLOW= ROE\nMUSCL_FLOW= NO\n"
       <<"TIME_DOMAIN= YES\nTIME_STEP= 1e-3\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nINNER_ITER= 2\n"
       <<"OUTPUT_FILES= (RESTART)\nWRT_ADAP_MESH= NO\nMESH_OUT_FORMAT= SU2\nRESTART_FILENAME= "<<name<<"_restart\nSOLUTION_FILENAME= "<<name<<"_restart\nCONV_FILENAME= "<<name<<"_history\n";
    if(walls)cfg<<"ADAP_BL_MARKER= (wall)\nADAP_BL_FIRST_HEIGHT= (.1)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.3)\n";
    return cfg.str();
  };
  if(!world.rank) {
    simplex_test::WriteSU2Mesh(simplex_test::MakeSimplexMesh(3,1,[](const passivedouble* x){return x[2]==0 ? "wall" : "far";}),name+".su2");
    std::ofstream cfg(name+".cfg");cfg<<configText(true);
    std::ofstream next(name+"_sensor_only.cfg");next<<configText(false);
  }
  SU2_MPI::Barrier(world.comm);const auto cfg=name+".cfg";
  auto driver=std::make_unique<TetraDriver>(const_cast<char*>(cfg.c_str()),1,world.comm);
  auto* flow=driver->Solver().GetNodes();auto* turb=driver->Turbulence()->GetNodes();
  const int root=CPassiveComm::Allreduce(driver->Geometry().GetnPoint()?world.rank:world.size,CPassiveComm::Op::MIN,world.comm);
  REQUIRE(root<world.size);std::vector<double> initial(4);
  if(world.rank==root)initial={SU2_TYPE::GetValue(flow->GetSolution(0,0)),SU2_TYPE::GetValue(flow->GetSolution(0,1)),
                              SU2_TYPE::GetValue(flow->GetSolution(0,4)),SU2_TYPE::GetValue(turb->GetSolution(0,0))};
  CPassiveComm::Bcast(initial,root,world.comm);
  const double rho=initial[0],momentum=initial[1];
  const double kinetic=momentum*momentum/(2*rho),internal=initial[2]-kinetic;
  const double viscosity=initial[3];REQUIRE(viscosity>0);
  auto expectedFlow=[&](double z) {return std::array<double,5>{{rho,momentum*z,0,0,internal+kinetic*z}};};
  for(unsigned long p=0;p<driver->Geometry().GetnPoint();++p) {
    const double z=SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p,2));const auto values=expectedFlow(z);
    for(unsigned short k=0;k<5;++k) {
      flow->SetSolution(p,k,values[k]);flow->GetSolution_time_n()(p,k)=values[k];flow->GetSolution_time_n1()(p,k)=values[k]*.96875;
    }
    turb->SetSolution(p,0,viscosity*z);turb->GetSolution_time_n()(p,0)=viscosity*z;turb->GetSolution_time_n1()(p,0)=viscosity*z*.9375;
  }
  driver->ComputeMetric();auto backend=driver->Backend();auto* native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
  const std::string authority=EncodeReference(*driver->Reference());CConservativeTransfer transfer;
  std::stringstream nextOptions(configText(false));std::unique_ptr<CConfig> sensorConfig;
  {Mute mute;sensorConfig=std::make_unique<CConfig>(nextOptions,SU2_COMPONENT::SU2_CFD,false);}
  std::vector<uint64_t> counts;ReferenceState restartAuthority;
  for(unsigned cycle=0;cycle<2;++cycle) {
    const auto before=State(driver->Geometry(),driver->Solver()),beforeTurb=State(driver->Geometry(),*driver->Turbulence());
    const auto old=Flow(world,driver->Geometry(),driver->Solver());const CConfig& config=driver->Config();
    su2activematrix metric(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<metric.rows();++p)for(unsigned k=0;k<6;++k) {
      metric(p,k)=driver->Solver().GetNodes()->GetMetric()(p,k);
      CHECK(metric(p,k)==Approx(k==0 || k==3 || k==5 ? 1. : 0.).margin(2e-14));
    }
    auto original=cycle ? restartAuthority : *driver->Reference();const auto input=ImportGeometry(config,driver->Geometry(),&metric,original);
    if(!cycle) {
      auto empty=std::make_shared<ReferenceState>();CNativeRemesher3D missing(empty);
      const auto denied=missing.Remesh(config,driver->Geometry(),metric);CHECK(denied.status!=CRemeshResult::Status::COMPLETE);
      CHECK_FALSE(denied.onAccepted);CHECK_FALSE(empty->original);CHECK(State(driver->Geometry(),driver->Solver())==before);
      if(world.size>1) {
        const auto mismatch=backend->Remesh(world.rank ? *sensorConfig : driver->Config(),driver->Geometry(),metric);
        CHECK(mismatch.status!=CRemeshResult::Status::COMPLETE);CHECK_FALSE(mismatch.onAccepted);
        CHECK(EncodeReference(*driver->Reference())==authority);
        CHECK(State(driver->Geometry(),driver->Solver())==before);CHECK(State(driver->Geometry(),*driver->Turbulence())==beforeTurb);
      }
    }
    const auto result=backend->Remesh(config,driver->Geometry(),metric);INFO("workers="<<workers<<" cycle="<<cycle);
    REQUIRE(result.status==CRemeshResult::Status::COMPLETE);CHECK(native->Statistics().complete);CHECK(native->Statistics().workers==workers);
    REQUIRE(result.complexity.estimated);CHECK(result.complexity.samples==8192);CHECK(result.complexity.value>0);CHECK(std::isfinite(result.complexity.uncertainty));
    if(!world.rank) {
      std::ofstream out("reassessment_"+name+"_stage"+std::to_string(cycle)+".json");out<<std::setprecision(17)
        <<"{\"value\":"<<result.complexity.value<<",\"indicator\":"<<result.complexity.uncertainty
        <<",\"point_SE\":"<<result.complexity.standard_error<<",\"block_SE\":"<<result.complexity.block_standard_error
        <<",\"samples\":"<<result.complexity.samples<<",\"phase_seconds\":"<<native->Statistics().phases[13]
        <<",\"return_source_seconds\":"<<native->Statistics().returned.source_seconds
        <<",\"actual_original_field\":true,\"proven_bound\":false,\"normalization_correction\":false}\n";REQUIRE(bool(out));
    }

    CHECK(State(driver->Geometry(),driver->Solver())==before);CHECK(State(driver->Geometry(),*driver->Turbulence())==beforeTurb);
    if(cycle)CHECK_FALSE(driver->Reference()->original);else CHECK(EncodeReference(*driver->Reference())==authority);
    driver->ReplaceMesh(result,transfer);CHECK(EncodeReference(*driver->Reference())==authority);
    const auto transported=result.slices.FetchPointMetric(driver->Geometry());const auto fresh=Flow(world,driver->Geometry(),driver->Solver(),transported);
    const auto event=name+"_s"+std::to_string(cycle);SaveStates(world,event,old,fresh,1.);SaveProjection(world,event,transfer);CheckPositiveFlow(fresh);
    auto acceptedMetric=su2activematrix(driver->Geometry().GetnPointDomain(),6);
    for(unsigned long p=0;p<acceptedMetric.rows();++p)for(unsigned k=0;k<6;++k) {
      acceptedMetric(p,k)=transported[6*p+k];CHECK(acceptedMetric(p,k)==Approx(k==0 || k==3 || k==5 ? 1. : 0.).margin(2e-14));
    }
    auto state=*driver->Reference();const auto accepted=ImportGeometry(config,driver->Geometry(),&acceptedMetric,state);
    Save(world,event,accepted.cells,input.original,state);counts.push_back(result.slices.nElemGlobal);
    for(auto row:fresh) {
      const auto expected=expectedFlow(row.point.z);
      for(unsigned k=0;k<5;++k) {
        CHECK(RelDiff(row.values[k],expected[k])<2e-10);CHECK(RelDiff(row.values[5+k],expected[k])<2e-10);
        CHECK(RelDiff(row.values[10+k],expected[k]*.96875)<2e-10);
      }
    }
    turb=driver->Turbulence()->GetNodes();
    for(unsigned long p=0;p<driver->Geometry().GetnPointDomain();++p) {
      const double expected=viscosity*SU2_TYPE::GetValue(driver->Geometry().nodes->GetCoord(p,2));
      const double lower=SU2_TYPE::GetValue(driver->Turbulence()->GetLowerLimit(0));
      // Normalize affine accuracy by its donor scale; the strict solver bound stays pointwise.
      CHECK(turb->GetSolution(p,0)>=lower);CHECK(RelDiff(turb->GetSolution(p,0),std::max(lower,expected),viscosity)<2e-10);
      CHECK(RelDiff(turb->GetSolution_time_n(p,0),std::max(lower,expected),viscosity)<2e-10);
      CHECK(RelDiff(turb->GetSolution_time_n1(p,0),std::max(lower,expected*.9375),viscosity*.9375)<2e-10);
    }
    OutputRoundTrip(world,event,result.slices,accepted.cells,state,1.,[](const su2double*,double){return Tensor{1,0,0,1,0,1};},&input.original);
    WriteReference(event+".su2.native_ref",state);ReferenceState loaded;CHECK(LoadReference(event+".su2.native_ref",loaded));
    CHECK(EncodeReference(loaded)==authority);
    if(!cycle) {
#ifdef HAVE_CGNS
      const std::string restartMesh=event+"_written_CGNS.cgns",format="CGNS";
#else
      const std::string restartMesh=event+"_written_SU2.su2",format="SU2";
#endif
      WriteReference(restartMesh+".native_ref",state);restartAuthority=state;
      driver->Config().SetTimeIter(2);const auto written=State(driver->Geometry(),driver->Solver());
      const auto writtenTurb=State(driver->Geometry(),*driver->Turbulence());driver->WriteTimeHistoryRestarts();
      CHECK(State(driver->Geometry(),driver->Solver())==written);CHECK(State(driver->Geometry(),*driver->Turbulence())==writtenTurb);
      if(!world.rank) {
        auto text=configText(false);const std::string oldMesh="MESH_FILENAME= "+name+".su2";
        const auto at=text.find(oldMesh);REQUIRE(at!=std::string::npos);text.replace(at,oldMesh.size(),"MESH_FILENAME= "+restartMesh);
        const std::string oldFormat="MESH_FORMAT= SU2";const auto f=text.find(oldFormat);REQUIRE(f!=std::string::npos);
        text.replace(f,oldFormat.size(),"MESH_FORMAT= "+format);
        std::ofstream cfg(name+"_restart.cfg");cfg<<text<<"RESTART_SOL= YES\nRESTART_ITER= 3\n";
      }
      backend.reset();{Mute mute;driver->Finalize();driver.reset();}SU2_MPI::Barrier(world.comm);
      const auto restartCfg=name+"_restart.cfg";driver=std::make_unique<TetraDriver>(const_cast<char*>(restartCfg.c_str()),1,world.comm);
      driver->Preprocess(3);const auto reloaded=Flow(world,driver->Geometry(),driver->Solver());SaveStates(world,name+"_restart",fresh,reloaded,1.);
      driver->ComputeMetric();backend=driver->Backend();native=dynamic_cast<CNativeRemesher3D*>(backend.get());REQUIRE(native);
    }
  }
  REQUIRE(counts.size()==2);CHECK(counts[0]>6);CHECK(counts[1]<counts[0]);
  {Mute mute;driver->Finalize();driver.reset();}
}

namespace {
#include "fixtures/native_curved_wall_blockage.inc"
#include "fixtures/native_curved_remaining.inc"
}

TEST_CASE("Cold native cylinder BL construction and release use the same original sensor and smooth policy", "[.][NativeCurvedBLConstruction3D]") {
  World world;const auto seed=PrimitiveMesh(false,false);
  std::stringstream text("SOLVER= RANS\nKIND_TURB_MODEL= SA\nMACH_NUMBER= .2\nAOA= 90\nREYNOLDS_NUMBER= 1000\nREYNOLDS_LENGTH= 1\n"
      "MESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\nMARKER_FAR= (cap0,cap1)\nMARKER_HEATFLUX= (cylinder,0)\nMGLEVEL= 0\n"
      "COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\nADAP_HESSIAN_NOISE= 0\n"
      "ADAP_BL_MARKER= (cylinder)\nADAP_BL_FIRST_HEIGHT= (.05)\nADAP_BL_GROWTH= (1.2)\nADAP_BL_THICKNESS= (.2)\nADAP_HAUSD= .04\n");
  if(!world.rank) {
    auto cfg=text.str();const std::string old="MESH_FILENAME= unused.su2";const auto at=cfg.find(old);REQUIRE(at!=std::string::npos);
    cfg.replace(at,old.size(),"MESH_FILENAME= curved_cylinder_initial_euler.su2");std::ofstream out("curved_cylinder.cfg");out<<cfg;
  }
  std::unique_ptr<CConfig> config;{Mute mute;config=std::make_unique<CConfig>(text,SU2_COMPONENT::SU2_CFD,false);}
  MeshSolution mesh(config.get(),seed,0);su2activematrix sensor(mesh.Fine().GetnPointDomain(),6);
  for(unsigned long p=0;p<sensor.rows();++p)for(unsigned k=0;k<6;++k)sensor(p,k)=k==0||k==3||k==5?1.:0.;
  ReferenceState reference;ImportGeometry(*config,mesh.Fine(),nullptr,reference);DeclarePrimitive(reference,false,false);
  const auto input=ImportGeometry(*config,mesh.Fine(),&sensor,reference);const auto before=Signature(world,input.cells,reference);
  auto layers=CBoundaryLayerMetric::FromNativeReference(reference,*config);
  MetricComposition compose=[layers](Point p,Tensor s) {
    CBoundaryLayerMetric::Tensor m;const su2double x[3]={p.x,p.y,p.z};
    m.m[0][0]=s.xx;m.m[0][1]=m.m[1][0]=s.xy;m.m[0][2]=m.m[2][0]=s.xz;
    m.m[1][1]=s.yy;m.m[1][2]=m.m[2][1]=s.yz;m.m[2][2]=s.zz;
    layers->ApplyPoint(x,m,1.,nullptr,false);
    return Tensor{SU2_TYPE::GetValue(m.m[0][0]),SU2_TYPE::GetValue(m.m[0][1]),SU2_TYPE::GetValue(m.m[0][2]),
        SU2_TYPE::GetValue(m.m[1][1]),SU2_TYPE::GetValue(m.m[1][2]),SU2_TYPE::GetValue(m.m[2][2])};
  };
  EngineOptions options;options.surface_tolerance=.04;options.maximum_work=4096;options.timings=true;
  for(unsigned marker=0;marker<reference.marker_names.size();++marker)if(reference.marker_names[marker]=="cylinder")options.wall_heights[marker]=.05;
  REQUIRE(options.wall_heights.size()==1);
  for(const auto& wall:options.wall_heights)options.region_walls[wall.first]={wall.second,1.2,.2};
  options.edge_breakpoints=[layers](Point a,Point b){return layers->EdgeBreakpoints({a.x,a.y,a.z},{b.x,b.y,b.z});};
  auto save=[&](const std::string& name,const Owned& owned,const std::vector<DonorCell>* source=nullptr) {
    Save(world,name,owned,source?*source:input.original,reference);
    std::vector<OwnedCell> local;for(const auto& row:owned)local.push_back(row.second);const auto all=world.metadata(local);
    if(world.rank)return;
    CSimplexMesh output;output.nDim=3;std::map<Id,unsigned long> ids;std::map<int,unsigned> markers;
    const std::array<std::array<unsigned,3>,4> faces{{{{1,2,3}},{{0,3,2}},{{0,1,3}},{{0,2,1}}}};
    for(const auto& row:all) {
      for(auto n:row.cell.v){auto inserted=ids.emplace(n.id,ids.size());if(inserted.second)output.coord.insert(output.coord.end(),{n.p.x,n.p.y,n.p.z});output.elem.push_back(inserted.first->second);}
      output.elemRef.push_back(0);
      for(unsigned f=0;f<4;++f)if(row.physical[f].present) {
        const auto marker=reference.original->Get(row.physical[f].facet).marker;auto inserted=markers.emplace(marker,markers.size());
        if(inserted.second)output.markers.push_back({reference.marker_names.at(marker),marker,{}});
        for(auto k:faces[f])output.markers[inserted.first->second].elem.push_back(ids.at(row.cell.v[k].id));
      }
    }
    simplex_test::WriteSU2Mesh(output,name+".su2");std::ofstream sidecar(name+".su2.native_ref");sidecar<<EncodeReference(reference);
  };
  save("curved_cylinder_initial_euler",input.cells);
  const auto captured_source=world.metadata(input.original);
  if(const auto* filename=std::getenv("SU2_NATIVE_CURVED_GRID_AUDIT")) {
    const bool protocol=std::getenv("SU2_NATIVE_CURVED_PROTOCOL");
    if(!protocol)REQUIRE(world.size==1);auto cfg=text.str();const auto at=cfg.find("unused.su2");REQUIRE(at!=std::string::npos);cfg.replace(at,10,filename);
    std::stringstream gridText(cfg);std::unique_ptr<CConfig> gridConfig;
    {Mute mute;gridConfig=std::make_unique<CConfig>(gridText,SU2_COMPONENT::SU2_CFD,false);}
    CSU2ASCIIMeshReaderFVM reader(gridConfig.get(),0,1);MeshSolution loaded(gridConfig.get(),reader,0);
    auto authority=reference;const auto grid=ImportGeometry(*gridConfig,loaded.Fine(),nullptr,authority);
    CHECK(authority.original==reference.original);Save(world,"audited_grid",grid.cells,input.original,reference);
    if(protocol) {
      const auto untouched=Signature(world,grid.cells,reference);options.maximum_work=4096;options.maximum_seconds=300;
      const auto partial=Refine(world,grid.cells,input.original,*reference.original,compose,options);
      save(partial.valid?"protocol_adapted":"protocol_rejected",partial.cells);
      const auto attempts=world.sum(partial.stats.region_attempts),count=world.sum(partial.cells.size());
      if(!world.rank)std::cout<<"REGION_PROTOCOL ranks="<<world.size<<" valid="<<partial.valid<<" cells="<<count
          <<" region_attempts="<<attempts<<" work="<<partial.stats.global_work
          <<" stop="<<EngineStopName(partial.stats.stop)<<" reason="<<partial.reason<<std::endl;
      CHECK(Signature(world,grid.cells,reference)==untouched);CHECK(Signature(world,input.cells,reference)==before);
      CHECK(partial.stats.global_work<=4096);CHECK(attempts==1);INFO(partial.reason);REQUIRE(partial.valid);
      FrozenField target(captured_source,compose,false,AssociatedExtensions(captured_source,*reference.original),options.edge_breakpoints);
      double q=1,j=1;long double maximum=0;
      for(const auto& row:partial.cells) {
        PatchMetric measured;std::string reason;
        REQUIRE(ValidateMetricPatch({row.second.cell},target,measured,reason));
        q=std::min(q,measured.minimum_mean_ratio);j=std::min(j,measured.minimum_scaled_jacobian);maximum=std::max(maximum,measured.maximum_length);
      }
      q=CPassiveComm::Allreduce(q,CPassiveComm::Op::MIN,world.comm);j=CPassiveComm::Allreduce(j,CPassiveComm::Op::MIN,world.comm);
      // Passive wire scalar is double; the strict long-double edge gate has already run above on every cell.
      const auto length=CPassiveComm::Allreduce(double(maximum),CPassiveComm::Op::MAX,world.comm);
      if(!world.rank){std::ofstream out("region_summary.json");out<<std::setprecision(17)<<"{\"valid\":true,\"shape_measured\":true,\"ranks\":"<<world.size
          <<",\"cells\":"<<count<<",\"q\":"<<q<<",\"j\":"<<j<<",\"maximum_full_length\":"<<length
          <<",\"work\":"<<partial.stats.global_work<<"}\n";REQUIRE(out.good());}
      return;  // Complete construction/publication only; CFD transfer and restart remain separate gates.
    }
    FrozenField target(captured_source,compose,false,AssociatedExtensions(captured_source,*reference.original),options.edge_breakpoints);
    std::ofstream cells("grid_samples.csv");cells<<std::setprecision(17)<<"cell,q,j,xx,xy,xz,yy,yz,zz\n";
    std::map<Edge,std::pair<Point,Point>> edges;double q=1,j=1;size_t shapeFailures=0;
    for(const auto& row:grid.cells) {
      const auto& c=row.second.cell;const auto m=target.Query(Centroid(c)).target;
      const auto shape=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},m);q=std::min(q,shape.mean_ratio);j=std::min(j,shape.minimum_scaled_jacobian);
      shapeFailures+=shape.mean_ratio<MinimumMeanRatio || shape.minimum_scaled_jacobian<MinimumScaledJacobian;
      cells<<c.id<<','<<shape.mean_ratio<<','<<shape.minimum_scaled_jacobian<<','<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz<<'\n';
      for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b) {
        const auto na=c.v[a],nb=c.v[b];const auto key=Edge{std::min(na.id,nb.id),std::max(na.id,nb.id)};
        edges.emplace(key,na.id<nb.id?std::make_pair(na.p,nb.p):std::make_pair(nb.p,na.p));
      }
    }
    std::ofstream lengths("grid_edges.csv");lengths<<std::setprecision(20)<<"a,b,length\n";long double maximum=0;size_t longEdges=0;
    for(const auto& e:edges){const auto length=target.TargetEdgeLength(e.second.first,e.second.second);maximum=std::max(maximum,length);longEdges+=length>MaximumMetricEdge;lengths<<e.first[0]<<','<<e.first[1]<<','<<length<<'\n';}
    REQUIRE(cells.good());REQUIRE(lengths.good());std::ofstream summary("grid_metric_summary.json");summary<<std::setprecision(17)
        <<"{\"cells\":"<<grid.cells.size()<<",\"shape_measured\":true,\"shape_failures\":"<<shapeFailures<<",\"q\":"<<q<<",\"j\":"<<j
        <<",\"edges\":"<<edges.size()<<",\"oversized_edges\":"<<longEdges<<",\"maximum_length\":"<<maximum<<"}\n";REQUIRE(summary.good());
    if(std::getenv("SU2_NATIVE_CURVED_GRID_REFINE")) {
      const auto unchanged=Signature(world,grid.cells,reference);
      options.maximum_rounds=std::getenv("SU2_NATIVE_CURVED_GRID_COMPLETE")?0:1;
      options.maximum_seconds=300;
      Patch rejected_old,rejected_proposal;Feature rejected_feature;MetricDiagnosis rejected_metric;
      std::string rejected_reason;bool rejected_geometry=false;size_t rejected_count=0;
      options.rejected_trial=[&](const Patch& old,const Patch& fresh,Feature feature,const MetricDiagnosis& metric,bool geometry,const std::string& reason) {
        rejected_old=old;rejected_proposal=fresh;rejected_feature=feature;rejected_metric=metric;
        rejected_reason=reason;rejected_geometry=geometry;++rejected_count;
      };
      const auto partial=Refine(world,grid.cells,input.original,*reference.original,compose,options);
      if(rejected_count) {
        SaveRecords("saved_grid_last_rejected_old",PatchRecords(rejected_old),captured_source,reference);
        SaveRecords("saved_grid_last_rejected_proposal",PatchRecords(rejected_proposal),captured_source,reference);
        std::ofstream out("saved_grid_last_rejected_diagnosis.json");out<<std::setprecision(17)<<"{\"trials\":"<<rejected_count
            <<",\"feature\":["<<rejected_feature.vertices[0]<<','<<rejected_feature.vertices[1]<<','<<rejected_feature.vertices[2]
            <<"],\"count\":"<<rejected_feature.count<<",\"geometry\":"<<(rejected_geometry?"true":"false")
            <<",\"metric_failure\":"<<unsigned(rejected_metric.failure)<<",\"shape_measured\":"<<(rejected_metric.shape_measured?"true":"false")<<",\"q\":";
        if(rejected_metric.shape_measured)out<<rejected_metric.minimum_mean_ratio;else out<<"null";
        out<<",\"j\":";if(rejected_metric.shape_measured)out<<rejected_metric.minimum_scaled_jacobian;else out<<"null";
        out<<",\"reason\":"<<std::quoted(rejected_reason)<<"}\n";REQUIRE(out.good());
      }
      std::cout<<"CURVED_GRID_REFINE accepted="<<partial.stats.accepted<<" inserted="<<partial.stats.inserted
          <<" classified_vertices="<<partial.stats.placement_classified_vertices<<" placement_trials="<<partial.stats.placement_trials<<" placement_validated="<<partial.stats.placement_validated
          <<" dependency_expansions="<<partial.stats.placement_dependency_expansions<<" connectivity_trials="<<partial.stats.placement_connectivity_trials<<" placement_flips="<<partial.stats.placement_flips<<" placement_moves="<<partial.stats.placement_moves
          <<" metric_linearizations="<<partial.stats.metric_linearizations<<" metric_probes="<<partial.stats.metric_probes
          <<" metric_linear_iterations="<<partial.stats.metric_linear_iterations<<" metric_solve_trials="<<partial.stats.metric_solve_trials
          <<" work="<<partial.stats.global_work<<" stop="<<EngineStopName(partial.stats.stop)<<" reason="<<partial.reason<<std::endl;
      save(partial.valid?"curved_grid_adapted":"curved_grid_partial_rejected",partial.cells);
      CHECK(Signature(world,grid.cells,reference)==unchanged);CHECK(Signature(world,input.cells,reference)==before);
      if(!std::getenv("SU2_NATIVE_CURVED_GRID_COMPLETE")){REQUIRE(partial.stats.accepted==1);CHECK(partial.stats.inserted==1);}
      if(!std::getenv("SU2_NATIVE_CURVED_GRID_COMPLETE")){CHECK_FALSE(partial.valid);CHECK(partial.stats.stop==EngineStop::RoundBudget);}
      PatchMetric measured;size_t oversized=0;double minimum_q=1,minimum_j=1;long double excess=0;
      std::map<Edge,long double> output_edges;
      for(const auto& row:partial.cells) {
        measured=MeasureMetricCells({row.second.cell},target);minimum_q=std::min(minimum_q,measured.minimum_mean_ratio);
        minimum_j=std::min(minimum_j,measured.minimum_scaled_jacobian);
        for(const auto& e:measured.edges)output_edges.emplace(e.first,e.second.length);
      }
      for(const auto& e:output_edges) {oversized+=e.second>MaximumMetricEdge;excess+=std::pow(std::max(0.L,e.second-MaximumMetricEdge),2);}
      long double initial_excess=0;
      for(const auto& e:edges)initial_excess+=std::pow(std::max(0.L,target.TargetEdgeLength(e.second.first,e.second.second)-MaximumMetricEdge),2);
      CHECK(minimum_q>=MinimumMeanRatio);CHECK(minimum_j>=MinimumScaledJacobian);CHECK(excess<initial_excess);
      std::ofstream diagnosis("grid_refine_summary.json");diagnosis<<std::setprecision(17)<<"{\"accepted\":"<<partial.stats.accepted<<",\"shape_measured\":true"
          <<",\"cells\":"<<partial.cells.size()<<",\"q\":"<<minimum_q<<",\"j\":"<<minimum_j<<",\"oversized_edges\":"<<oversized
          <<",\"region_attempts\":"<<partial.stats.region_attempts<<",\"region_cells\":"<<partial.stats.region_cells
          <<",\"region_surface_trials\":"<<partial.stats.region_surface_trials<<",\"region_junction_trials\":"<<partial.stats.region_junction_trials
          <<",\"metric_linearizations\":"<<partial.stats.metric_linearizations<<",\"metric_probes\":"<<partial.stats.metric_probes
          <<",\"metric_linear_iterations\":"<<partial.stats.metric_linear_iterations<<",\"metric_solve_trials\":"<<partial.stats.metric_solve_trials
          <<",\"initial_excess\":"<<initial_excess<<",\"remaining_excess\":"<<excess<<",\"work\":"<<partial.stats.global_work<<"}\n";
      REQUIRE(diagnosis.good());
      if(std::getenv("SU2_NATIVE_CURVED_GRID_COMPLETE")){INFO(partial.reason);REQUIRE(partial.valid);CHECK(oversized==0);}else CHECK(oversized>0);
      return;  // Saved-grid engine check; solver transfer/replacement and cold construction are separate gates.
    }
    if(std::getenv("SU2_NATIVE_CURVED_EDGE_REPLAY")) {
      auto eligible=edges;
      if((std::getenv("SU2_NATIVE_CURVED_JOINT_SEARCH") && !std::getenv("SU2_NATIVE_CURVED_INTERIOR_SEARCH")) || std::getenv("SU2_NATIVE_CURVED_WALL_REFIT")) {
        std::map<Edge,std::vector<Id>> charts;
        for(const auto& row:grid.cells) {
          const auto& c=row.second.cell;
          const std::array<Face,4> faces{{{{c.v[1].id,c.v[2].id,c.v[3].id}},{{c.v[0].id,c.v[3].id,c.v[2].id}},{{c.v[0].id,c.v[1].id,c.v[3].id}},{{c.v[0].id,c.v[2].id,c.v[1].id}}}};
          for(unsigned f=0;f<4;++f)if(row.second.physical[f].present)for(unsigned k=0;k<3;++k) {
            const auto a=faces[f][k],b=faces[f][(k+1)%3];charts[{std::min(a,b),std::max(a,b)}].push_back(row.second.physical[f].facet);
          }
        }
        for(auto it=eligible.begin();it!=eligible.end();) {
          const auto found=charts.find(it->first);bool flat=false;
          if(std::getenv("SU2_NATIVE_CURVED_WALL_REFIT")) {
            if(std::getenv("SU2_NATIVE_CURVED_RIM_REFIT"))flat=found!=charts.end() && found->second.size()==2 &&
                reference.original->Canonical(found->second[0])!=reference.original->Canonical(found->second[1]) &&
                std::any_of(found->second.begin(),found->second.end(),[&](Id facet){return options.wall_heights.count(reference.original->Get(facet).marker);});
            else flat=found!=charts.end() && found->second.size()==2 &&
                reference.original->Canonical(found->second[0])==reference.original->Canonical(found->second[1]) &&
                std::all_of(found->second.begin(),found->second.end(),[&](Id facet){return options.wall_heights.count(reference.original->Get(facet).marker);});
          } else if(found!=charts.end() && found->second.size()==2 && found->second[0]==found->second[1]) {
            const auto g=reference.original->Analytic().find(found->second[0]);
            flat=g==reference.original->Analytic().end() || g->second.kind==AnalyticSurface::Kind::Disk;
          }
          if(!flat)it=eligible.erase(it);else ++it;
        }
      }
      REQUIRE(!eligible.empty());const auto longest=std::max_element(eligible.begin(),eligible.end(),[&](const auto& a,const auto& b) {
        return target.TargetEdgeLength(a.second.first,a.second.second)<target.TargetEdgeLength(b.second.first,b.second.second);
      });
      const auto edge=longest->first;Patch old;
      for(const auto& row:grid.cells) {
        unsigned count=0;for(auto n:row.second.cell.v)count+=n.id==edge[0] || n.id==edge[1];if(count!=2)continue;
        old.cells.push_back(row.second.cell);
        const auto& c=row.second.cell;const std::array<Face,4> faces{{{{c.v[1].id,c.v[2].id,c.v[3].id}},{{c.v[0].id,c.v[3].id,c.v[2].id}},{{c.v[0].id,c.v[1].id,c.v[3].id}},{{c.v[0].id,c.v[2].id,c.v[1].id}}}};
        for(unsigned f=0;f<4;++f)if(row.second.physical[f].present)old.surface.push_back({faces[f],row.second.physical[f].facet});
      }
      REQUIRE(!old.cells.empty());SaveRecords("edge_original",PatchRecords(old),captured_source,reference);
      const Node point{1000000,target.BalancedSplitPoint(longest->second.first,longest->second.second)};
      std::cout<<std::setprecision(17)<<"EDGE_REPLAY edge="<<edge[0]<<','<<edge[1]<<" star="<<old.cells.size()<<" point="<<point.p.x<<','<<point.p.y<<','<<point.p.z<<std::endl;
      if(std::getenv("SU2_NATIVE_CURVED_WALL_REFIT")) {
        std::set<Id> apices;
        for(const auto& f:old.surface)if(options.wall_heights.count(reference.original->Get(f.facet).marker) &&
            std::find(f.v.begin(),f.v.end(),edge[0])!=f.v.end() && std::find(f.v.begin(),f.v.end(),edge[1])!=f.v.end())
          for(const auto& c:old.cells) {
            unsigned count=0;Id apex=0;for(auto n:c.v)if(std::find(f.v.begin(),f.v.end(),n.id)!=f.v.end())++count;else apex=n.id;
            if(count==3)apices.insert(apex);
          }
        REQUIRE(!apices.empty());std::vector<OwnedCell> support;
        for(const auto& row:grid.cells)if(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return apices.count(n.id);}) ||
            std::count_if(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return n.id==edge[0] || n.id==edge[1];})==2)support.push_back(row.second);
        Patch full;
        for(const auto& row:support) {
          full.cells.push_back(row.cell);const auto& c=row.cell;
          const std::array<Face,4> faces{{{{c.v[1].id,c.v[2].id,c.v[3].id}},{{c.v[0].id,c.v[3].id,c.v[2].id}},{{c.v[0].id,c.v[1].id,c.v[3].id}},{{c.v[0].id,c.v[2].id,c.v[1].id}}}};
          for(unsigned f=0;f<4;++f)if(row.physical[f].present)full.surface.push_back({faces[f],row.physical[f].facet});
        }
        REQUIRE(full.cells.size()<=MaximumCavityCells);SaveRecords("wall_refit_original",PatchRecords(full),captured_source,reference);
        if(std::getenv("SU2_NATIVE_CURVED_RIM_REFIT")) {
          REQUIRE(apices.size()==1);Node support{};bool found=false;
          for(const auto& c:full.cells)for(auto n:c.v)if(n.id==*apices.begin()){support=n;found=true;}
          REQUIRE(found);const auto original=support.id;support.id=1000001;std::string reason;Patch controls;
          const auto a=longest->second.first,b=longest->second.second;
          for(long double fraction:{.25L,.5L,.75L}) {
            const Node proposal{point.id,{double((1-fraction)*a.x+fraction*b.x),double((1-fraction)*a.y+fraction*b.y),double((1-fraction)*a.z+fraction*b.z)}};
            const bool valid=SplitSurfaceStrip(full,edge,proposal,{{original,support}},2000000,*reference.original,controls,reason);
            INFO(reason);REQUIRE(valid);
            Point inserted{};bool present=false;for(const auto& c:controls.cells)for(auto n:c.v)if(n.id==point.id){inserted=n.p;present=true;}
            REQUIRE(present);
            for(const auto& f:old.surface)if(std::find(f.v.begin(),f.v.end(),edge[0])!=f.v.end() && std::find(f.v.begin(),f.v.end(),edge[1])!=f.v.end()) {
              CHECK(reference.original->Contains(f.facet,inserted));
              const auto g=reference.original->Analytic().find(f.facet);
              if(g!=reference.original->Analytic().end() && g->second.kind!=AnalyticSurface::Kind::Disk)
                CHECK(reference.original->OnProjectedEdge(f.facet,a,b,inserted));
            }
          }
          const auto untouched=controls;
          const Node beyond{point.id,{double(-.25L*a.x+1.25L*b.x),double(-.25L*a.y+1.25L*b.y),double(-.25L*a.z+1.25L*b.z)}};
          CHECK_FALSE(SplitSurfaceStrip(full,edge,beyond,{{original,support}},2000000,*reference.original,controls,reason));
          REQUIRE(controls.cells.size()==untouched.cells.size());
          for(size_t i=0;i<controls.cells.size();++i) {
            CHECK(controls.cells[i].id==untouched.cells[i].id);
            for(unsigned k=0;k<4;++k) {
              CHECK(controls.cells[i].v[k].id==untouched.cells[i].v[k].id);
              CHECK(controls.cells[i].v[k].p.x==untouched.cells[i].v[k].p.x);
              CHECK(controls.cells[i].v[k].p.y==untouched.cells[i].v[k].p.y);
              CHECK(controls.cells[i].v[k].p.z==untouched.cells[i].v[k].p.z);
            }
          }
        }
        Patch fresh;Feature obstruction;std::string reason;bool built=false;
        for(unsigned growth=0;growth<12;++growth) {
          SaveRecords("wall_refit_original",PatchRecords(full),captured_source,reference);
          if(std::getenv("SU2_NATIVE_CURVED_METRIC_SOLVE")) {
            Face face{};built=ReconstructWallSplit(full,edge,point,1000001,2000000,options.wall_heights,*reference.original,fresh,face,reason,nullptr,options.surface_tolerance);
            obstruction=face==Face{}?Feature{}:Feature{face,3};
          } else built=RefitWallSplit(full,edge,point,1000001,2000000,options.wall_heights,*reference.original,fresh,obstruction,reason);
          if(built || !obstruction.count)break;
          std::set<Id> admitted;for(const auto& c:full.cells)admitted.insert(c.id);
          std::vector<OwnedCell> missing;
          for(const auto& row:grid.cells)if(!admitted.count(row.first) &&
              std::count_if(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return std::find(obstruction.vertices.begin(),obstruction.vertices.begin()+obstruction.count,n.id)!=obstruction.vertices.begin()+obstruction.count;})==obstruction.count)
            missing.push_back(row.second);
          if(missing.empty() || full.cells.size()+missing.size()>MaximumCavityCells)break;
          for(const auto& row:missing) {
            full.cells.push_back(row.cell);const auto& c=row.cell;
            const std::array<Face,4> faces{{{{c.v[1].id,c.v[2].id,c.v[3].id}},{{c.v[0].id,c.v[3].id,c.v[2].id}},{{c.v[0].id,c.v[1].id,c.v[3].id}},{{c.v[0].id,c.v[2].id,c.v[1].id}}}};
            for(unsigned f=0;f<4;++f)if(row.physical[f].present)full.surface.push_back({faces[f],row.physical[f].facet});
          }
          std::cout<<"WALL_REFIT_DEPENDENCY feature="<<obstruction.vertices[0]<<','<<obstruction.vertices[1]<<','<<obstruction.vertices[2]
              <<" count="<<obstruction.count<<" cells="<<full.cells.size()<<std::endl;
        }
        std::cout<<"WALL_REFIT_GEOMETRY old="<<full.cells.size()<<" apices="<<apices.size()<<" built="<<built<<" reason="<<reason<<std::endl;
        INFO(reason);REQUIRE(built);SaveRecords("wall_refit_proposal",PatchRecords(fresh),captured_source,reference);
        FirstHeightReport height;REQUIRE(CheckFirstHeight(fresh,options.wall_heights,*reference.original,height,reason));
        PatchMetric measured;MetricDiagnosis diagnosis;
        const auto intent=std::getenv("SU2_NATIVE_CURVED_METRIC_SOLVE")?MetricChange::Refine:MetricChange::Repair;
        bool valid=ValidateMetricChange(full.cells,fresh.cells,intent,target,measured,reason,nullptr,nullptr,&diagnosis) && diagnosis.size_progress;
        if(std::getenv("SU2_NATIVE_CURVED_METRIC_SOLVE") && !valid && diagnosis.failure==MetricFailure::Shape) {
          Patch repaired;ConnectivityRepairReport report;
          const bool progress=RepairMetricConnectivity(fresh,target,*reference.original,options.wall_heights,3000000,repaired,report,reason,256);
          std::cout<<"METRIC_PATCH_REPAIR progress="<<progress<<" trials="<<report.trials<<" flips="<<report.flips<<" moves="<<report.moves<<" removals="<<report.removals
              <<" linearizations="<<report.linearizations<<" probes="<<report.metric_probes<<" linear_iterations="<<report.linear_iterations<<" solve_trials="<<report.solve_trials
              <<" complete="<<report.complete_shape<<" before="<<report.before_loss<<" after="<<report.after_loss<<" blocked="<<report.blocked_interior_vertex<<" reason="<<reason<<std::endl;
          if(progress) {
            fresh=std::move(repaired);SaveRecords("metric_patch_repaired",PatchRecords(fresh),captured_source,reference);
            valid=ValidateMetricChange(full.cells,fresh.cells,intent,target,measured,reason,nullptr,nullptr,&diagnosis) && diagnosis.size_progress;
          }
        }
        std::cout<<std::setprecision(17)<<"WALL_REFIT old="<<full.cells.size()<<" fresh="<<fresh.cells.size()<<" apices="<<apices.size()
            <<" valid="<<valid<<" q=";
        if(diagnosis.shape_measured)std::cout<<diagnosis.minimum_mean_ratio;else std::cout<<"unavailable";
        std::cout<<" j=";if(diagnosis.shape_measured)std::cout<<diagnosis.minimum_scaled_jacobian;else std::cout<<"unavailable";
        std::cout<<" reason="<<reason<<std::endl;
        std::ofstream out("wall_refit_metric.json");out<<std::setprecision(17)<<"{\"accepted\":"<<(valid?"true":"false")
            <<",\"shape_measured\":"<<(diagnosis.shape_measured?"true":"false")<<",\"q\":";
        if(diagnosis.shape_measured)out<<diagnosis.minimum_mean_ratio;else out<<"null";
        out<<",\"j\":";if(diagnosis.shape_measured)out<<diagnosis.minimum_scaled_jacobian;else out<<"null";
        out<<",\"reason\":"<<std::quoted(reason)<<"}\n";REQUIRE(out.good());
        auto retained=fresh;const auto count=retained.cells.size();
        CHECK_FALSE(RefitWallSplit(full,edge,point,point.id,2000000,options.wall_heights,*reference.original,retained,obstruction,reason));CHECK(retained.cells.size()==count);
        CHECK_FALSE(RefitWallSplit(old,edge,point,1000001,2000000,options.wall_heights,*reference.original,retained,obstruction,reason));CHECK(retained.cells.size()==count);
        CHECK(Signature(world,input.cells,reference)==before);INFO(reason);CHECK(valid);return;
      }
      if(std::getenv("SU2_NATIVE_CURVED_JOINT_SEARCH")) {
        std::map<Id,size_t> costs;
        if(std::getenv("SU2_NATIVE_CURVED_COUPLED_COVER")) {
          std::set<Id> physical;for(const auto& row:grid.cells)for(unsigned f=0;f<4;++f)if(row.second.physical[f].present)
            for(unsigned k=0;k<4;++k)if(k!=f)physical.insert(row.second.cell.v[k].id);
          for(const auto& row:grid.cells)for(auto n:row.second.cell.v)if(!physical.count(n.id))++costs[n.id];
        }
        const auto selected=SplitPatchApices(old,edge,std::getenv("SU2_NATIVE_CURVED_COUPLED_COVER")?&costs:nullptr);
        REQUIRE((!selected.empty() || (std::getenv("SU2_NATIVE_CURVED_INTERIOR_SEARCH") && !std::getenv("SU2_NATIVE_CURVED_COUPLED_COVER")) || std::getenv("SU2_NATIVE_CURVED_POINT_ONLY")));
        std::vector<std::pair<Id,Id>> apices;
        for(Id id:selected)apices.emplace_back(id,1000001+apices.size());
        Patch full;
        for(const auto& row:grid.cells)if(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return std::any_of(apices.begin(),apices.end(),[&](const auto& p){return n.id==p.first;});}) ||
            std::count_if(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return n.id==edge[0] || n.id==edge[1];})==2) {
          full.cells.push_back(row.second.cell);const auto& c=row.second.cell;
          const std::array<Face,4> faces{{{{c.v[1].id,c.v[2].id,c.v[3].id}},{{c.v[0].id,c.v[3].id,c.v[2].id}},{{c.v[0].id,c.v[1].id,c.v[3].id}},{{c.v[0].id,c.v[2].id,c.v[1].id}}}};
          for(unsigned f=0;f<4;++f)if(row.second.physical[f].present)full.surface.push_back({faces[f],row.second.physical[f].facet});
        }
        if((std::getenv("SU2_NATIVE_CURVED_INTERIOR_SEARCH") && !std::getenv("SU2_NATIVE_CURVED_COUPLED_COVER")) || std::getenv("SU2_NATIVE_CURVED_POINT_ONLY")){full=old;apices.clear();}
        SaveRecords("joint_original",PatchRecords(full),captured_source,reference);
        Patch fresh;JointSplitReport report;std::string reason;
        bool accepted=false;size_t cumulative=0;
        for(unsigned attempt=0;attempt<8;++attempt) {
          SaveRecords("joint_original",PatchRecords(full),captured_source,reference);
          accepted=PlaceSplitPatch(full,edge,point,apices,2000000,target,*reference.original,options.wall_heights,fresh,report,reason,std::min<size_t>(256,4096-cumulative));
          cumulative+=report.trials;
          if(accepted || !std::getenv("SU2_NATIVE_CURVED_ADMIT_HINTS") || report.blocked_interior_vertex==UINT64_MAX || apices.size()==8)break;
          const Id hint=report.blocked_interior_vertex;
          if(hint==edge[0] || hint==edge[1] || std::any_of(apices.begin(),apices.end(),[&](const auto& p){return p.first==hint;}))break;
          bool physical=false;for(const auto& row:grid.cells)for(unsigned f=0;f<4;++f)if(row.second.physical[f].present && row.second.cell.v[f].id!=hint)
            physical=physical || std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return n.id==hint;});
          if(physical)break;
          std::set<Id> admitted;for(const auto& c:full.cells)admitted.insert(c.id);
          for(const auto& row:grid.cells)if(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return n.id==hint;}))admitted.insert(row.first);
          if(admitted.size()>MaximumCavityCells)break;
          apices.emplace_back(hint,1000001+apices.size());full={};
          for(Id id:admitted) {
            const auto& row=grid.cells.at(id);full.cells.push_back(row.cell);const auto& c=row.cell;
            const std::array<Face,4> faces{{{{c.v[1].id,c.v[2].id,c.v[3].id}},{{c.v[0].id,c.v[3].id,c.v[2].id}},{{c.v[0].id,c.v[1].id,c.v[3].id}},{{c.v[0].id,c.v[2].id,c.v[1].id}}}};
            for(unsigned f=0;f<4;++f)if(row.physical[f].present)full.surface.push_back({faces[f],row.physical[f].facet});
          }
          std::cout<<"JOINT_DEPENDENCY vertex="<<hint<<" cells="<<full.cells.size()<<" cumulative_work="<<cumulative<<std::endl;
        }
        CHECK(cumulative<=4096);
        std::cout<<"JOINT_SEARCH cells="<<full.cells.size()<<" apices="<<apices.size()<<" accepted="<<accepted<<" trials="<<report.trials
            <<" connectivity_trials="<<report.connectivity_trials<<" flips="<<report.flips<<" moves="<<report.moves<<" blocked_vertex="<<report.blocked_interior_vertex
            <<" geometry_rejected="<<report.geometry_rejected<<" height_rejected="<<report.height_rejected<<" metric_rejected="<<report.metric_rejected
            <<" shape_loss="<<report.best_shape_loss<<" initial="<<report.initial_loss<<" best="<<report.best_loss<<" reason="<<reason<<std::endl;
        if(accepted)SaveRecords("joint_private",PatchRecords(fresh),captured_source,reference);
        else if(!report.best.cells.empty())SaveRecords("joint_best_rejected",PatchRecords(report.best),captured_source,reference);
        CHECK(report.trials<=256);
        const auto search_reason=reason;Patch rejected=full;JointSplitReport invalid;
        CHECK_FALSE(PlaceSplitPatch(full,edge,point,apices,2000000,target,*reference.original,options.wall_heights,rejected,invalid,reason,0));
        REQUIRE(rejected.cells.size()==full.cells.size());CHECK(rejected.surface.size()==full.surface.size());
        for(size_t i=0;i<full.cells.size();++i) {
          CHECK(rejected.cells[i].id==full.cells[i].id);
          for(unsigned k=0;k<4;++k) {
            CHECK(rejected.cells[i].v[k].id==full.cells[i].v[k].id);
            CHECK(rejected.cells[i].v[k].p.x==full.cells[i].v[k].p.x);
            CHECK(rejected.cells[i].v[k].p.y==full.cells[i].v[k].p.y);
            CHECK(rejected.cells[i].v[k].p.z==full.cells[i].v[k].p.z);
          }
        }
        INFO(search_reason);REQUIRE(accepted);
        CHECK(Signature(world,input.cells,reference)==before);return;
      }
      for(unsigned op=0;op<3;++op) {
        Patch fresh;Face obstruction{};std::string reason;
        const bool geometry=op==0?SplitEdge(old,edge,point,2000000,*reference.original,fresh,reason):
            op==1?InsertEdgeCavity(old,edge,point,2000000,*reference.original,fresh,reason):
            ReconstructSplitPatch(old,edge,point,{1000001,{}},target.Query(Centroid(old.cells.front())).target,2000000,*reference.original,fresh,obstruction,reason,nullptr,options.wall_heights);
        FirstHeightReport height;bool valid=false;PatchMetric measured;
        if(geometry){SaveRecords("edge_op"+std::to_string(op),PatchRecords(fresh),captured_source,reference);
          if(CheckFirstHeight(fresh,options.wall_heights,*reference.original,height,reason))
            valid=ValidateMetricChange(old.cells,fresh.cells,MetricChange::Refine,target,measured,reason);}
        std::cout<<"EDGE_REPLAY op="<<op<<" geometry="<<geometry<<" valid="<<valid<<" q=";
        if(measured.shape_measured)std::cout<<measured.minimum_mean_ratio;else std::cout<<"unavailable";
        std::cout<<" j=";if(measured.shape_measured)std::cout<<measured.minimum_scaled_jacobian;else std::cout<<"unavailable";
        std::cout<<" obstruction="<<obstruction[0]<<','<<obstruction[1]<<','<<obstruction[2]<<" reason="<<reason<<std::endl;
        if(op==1 && geometry && !valid) {
          for(double fraction:{.05,.075,.1,.125,.15,.2,.25}) {
            const auto a=longest->second.first,b=longest->second.second;
            const Node trial{1000001,{double((1-(long double)fraction)*a.x+fraction*b.x),double((1-(long double)fraction)*a.y+fraction*b.y),double((1-(long double)fraction)*a.z+fraction*b.z)}};
            Patch moved;bool accepted=false;const bool rebuilt=MoveVertex(fresh,point.id,trial,3000000,*reference.original,moved,reason);
            if(rebuilt && CheckFirstHeight(moved,options.wall_heights,*reference.original,height,reason))
              accepted=ValidateMetricChange(old.cells,moved.cells,MetricChange::Refine,target,measured,reason);
            std::cout<<"EDGE_PLACEMENT fraction="<<fraction<<" geometry="<<rebuilt<<" valid="<<accepted<<" reason="<<reason<<std::endl;
            if(accepted){SaveRecords("edge_placement_private",PatchRecords(moved),captured_source,reference);break;}
          }
        }
        if(op==1 && geometry && !valid) {
          std::map<Id,unsigned> incidence;std::set<Id> boundary;
          for(const auto& f:old.surface)boundary.insert(f.v.begin(),f.v.end());
          for(const auto& c:old.cells)for(auto n:c.v)if(!boundary.count(n.id))++incidence[n.id];
          const auto selected=std::max_element(incidence.begin(),incidence.end(),[](const auto& a,const auto& b){return a.second<b.second;});
          if(selected!=incidence.end()) {
            const Id apex=selected->first;Patch full;
            for(const auto& row:grid.cells)if(std::any_of(row.second.cell.v.begin(),row.second.cell.v.end(),[&](Node n){return n.id==apex;})) {
              full.cells.push_back(row.second.cell);const auto& c=row.second.cell;const std::array<Face,4> faces{{{{c.v[1].id,c.v[2].id,c.v[3].id}},{{c.v[0].id,c.v[3].id,c.v[2].id}},{{c.v[0].id,c.v[1].id,c.v[3].id}},{{c.v[0].id,c.v[2].id,c.v[1].id}}}};
              for(unsigned f=0;f<4;++f)if(row.second.physical[f].present)full.surface.push_back({faces[f],row.second.physical[f].facet});
            }
            SaveRecords("edge_apex_old",PatchRecords(full),captured_source,reference);Patch split;
            if(SplitSurfaceStrip(full,edge,point,{},2000000,*reference.original,split,reason)) {
              Point origin{};for(const auto& c:full.cells)for(auto n:c.v)if(n.id==apex)origin=n.p;
              const auto m=target.Query(origin).target;
              for(unsigned axis=0;axis<3;++axis)for(double sign:{-1.,1.})for(double fraction:{.25,.5,1.}) {
                Point moved=origin;const auto step=sign*fraction/std::sqrt(axis==0?m.xx:axis==1?m.yy:m.zz);
                if(axis==0)moved.x+=step;else if(axis==1)moved.y+=step;else moved.z+=step;
                Patch candidate;bool accepted=false;const bool rebuilt=MoveVertex(split,apex,{1000001,moved},3000000,*reference.original,candidate,reason);
                if(rebuilt && CheckFirstHeight(candidate,options.wall_heights,*reference.original,height,reason))accepted=ValidateMetricChange(full.cells,candidate.cells,MetricChange::Refine,target,measured,reason);
                std::cout<<"EDGE_APEX cells="<<full.cells.size()<<" axis="<<axis<<" step="<<step<<" geometry="<<rebuilt<<" valid="<<accepted<<" reason="<<reason<<std::endl;
                if(accepted)SaveRecords("edge_apex_private",PatchRecords(candidate),captured_source,reference);
              }
              for(double fraction:{.1,.125,.15,.175,.2})for(double dz:{-.015,-.03,-.06}) {
                const auto a=longest->second.first,b=longest->second.second;
                const Node inserted{1000000,{double((1-(long double)fraction)*a.x+fraction*b.x),double((1-(long double)fraction)*a.y+fraction*b.y),double((1-(long double)fraction)*a.z+fraction*b.z)}};
                Patch base,moved;bool accepted=false;std::string rejected;
                const bool split_ok=SplitSurfaceStrip(full,edge,inserted,{},2000000,*reference.original,base,rejected);
                const Node changed{1000001,{origin.x,origin.y,origin.z+dz}};
                const bool rebuilt=split_ok && MoveVertex(base,apex,changed,3000000,*reference.original,moved,rejected);
                if(rebuilt && CheckFirstHeight(moved,options.wall_heights,*reference.original,height,rejected))accepted=ValidateMetricChange(full.cells,moved.cells,MetricChange::Refine,target,measured,rejected);
                std::cout<<"EDGE_JOINT fraction="<<fraction<<" dz="<<dz<<" geometry="<<rebuilt<<" valid="<<accepted<<" reason="<<rejected<<std::endl;
                if(accepted)SaveRecords("edge_joint_private",PatchRecords(moved),captured_source,reference);
              }
            } else std::cout<<"EDGE_APEX retained split rejected: "<<reason<<std::endl;
          }
        }
        if(op==2 && !valid) {
          std::set<Id> admitted;for(const auto& c:old.cells)admitted.insert(c.id);
          for(unsigned growth=0;growth<12 && obstruction!=Face{};++growth) {
            auto key=obstruction;std::sort(key.begin(),key.end());bool added=false;
            for(const auto& row:grid.cells)if(!admitted.count(row.first)) {
              unsigned count=0;for(auto n:row.second.cell.v)count+=std::find(key.begin(),key.end(),n.id)!=key.end();
              if(count==3){admitted.insert(row.first);added=true;}
            }
            if(!added || admitted.size()>MaximumCavityCells)break;
            std::vector<OwnedCell> rows;for(Id id:admitted)rows.push_back(grid.cells.at(id));
            old.cells.clear();old.surface.clear();for(const auto& row:rows) {
              old.cells.push_back(row.cell);const auto& c=row.cell;const std::array<Face,4> faces{{{{c.v[1].id,c.v[2].id,c.v[3].id}},{{c.v[0].id,c.v[3].id,c.v[2].id}},{{c.v[0].id,c.v[1].id,c.v[3].id}},{{c.v[0].id,c.v[2].id,c.v[1].id}}}};
              for(unsigned f=0;f<4;++f)if(row.physical[f].present)old.surface.push_back({faces[f],row.physical[f].facet});
            }
            Patch expanded;const bool rebuilt=ReconstructSplitPatch(old,edge,point,{1000001,{}},target.Query(Centroid(old.cells.front())).target,2000000,*reference.original,expanded,obstruction,reason,nullptr,options.wall_heights);
            bool accepted=false;
            if(rebuilt && CheckFirstHeight(expanded,options.wall_heights,*reference.original,height,reason))accepted=ValidateMetricChange(old.cells,expanded.cells,MetricChange::Refine,target,measured,reason);
            SaveRecords("edge_grown_old",PatchRecords(old),captured_source,reference);
            if(rebuilt)SaveRecords("edge_grown_proposal",PatchRecords(expanded),captured_source,reference);
            std::cout<<"EDGE_GROWTH step="<<growth<<" cells="<<old.cells.size()<<" geometry="<<rebuilt<<" valid="<<accepted
                <<" obstruction="<<obstruction[0]<<','<<obstruction[1]<<','<<obstruction[2]<<" reason="<<reason<<std::endl;
            if(accepted)break;
          }
        }
      }
    }
    CHECK(Signature(world,input.cells,reference)==before);return;  // Read-only diagnostics; rejected mesh is never a CFD replacement.
  }
  if(std::getenv("SU2_NATIVE_CURVED_REPLAY")) {
    REQUIRE(world.size==1);REQUIRE(EncodeReference(reference)==CurvedBlockedReference());
    const std::string selected=std::getenv("SU2_NATIVE_CURVED_REPLAY");
    const bool remaining=selected=="remaining" || selected=="complete";
    const auto old=remaining?CurvedRemainingWallPatch(selected=="complete"):CurvedBlockedWallPatch();
    const Edge edge=remaining?Edge{4,14}:Edge{1,17};std::map<Id,Point> nodes;
    for(const auto& c:old.cells)for(auto n:c.v)nodes.emplace(n.id,n.p);
    const auto a=nodes.at(edge[0]),b=nodes.at(edge[1]);
    const Node point{1000000,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
    FrozenField field(captured_source,compose,false,AssociatedExtensions(captured_source,*reference.original),options.edge_breakpoints);
    SaveRecords("replay_original",PatchRecords(old),captured_source,reference);
    Patch fresh;Face obstruction{};std::string reason;
    REQUIRE(ReconstructWallSplit(old,edge,point,1000001,2000000,options.wall_heights,*reference.original,fresh,obstruction,reason,nullptr,options.surface_tolerance));
    auto samples=[&](const char* name,const Patch& patch) {
      SaveRecords(name,PatchRecords(patch),captured_source,reference);
      std::ofstream out(std::string(name)+"_samples.json");out<<std::setprecision(17)<<"[";bool comma=false;
      double q=1,j=1;
      for(const auto& c:patch.cells) {
        const auto p=Centroid(c);const auto m=field.Query(p).target;
        const auto shape=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},m);q=std::min(q,shape.mean_ratio);j=std::min(j,shape.minimum_scaled_jacobian);
        if(comma)out<<',';comma=true;out<<"{\"cell\":"<<c.id<<",\"tensor\":["<<m.xx<<','<<m.xy<<','<<m.xz<<','<<m.yy<<','<<m.yz<<','<<m.zz
            <<"],\"shape_measured\":true,\"q\":"<<shape.mean_ratio<<",\"j\":"<<shape.minimum_scaled_jacobian<<"}";
      }
      out<<"]\n";REQUIRE(out.good());std::cout<<std::setprecision(17)<<"CURVED_REPLAY "<<name<<" cells="<<patch.cells.size()<<" q="<<q<<" j="<<j<<std::endl;
    };
    samples("replay_coordinated_rejected",fresh);
    Patch repaired;ConnectivityRepairReport report;
    const bool progress=RepairMetricConnectivity(fresh,field,*reference.original,options.wall_heights,3000000,repaired,report,reason,256);
    std::cout<<"CURVED_REPLAY repair progress="<<progress<<" trials="<<report.trials<<" flips="<<report.flips<<" moves="<<report.moves<<" removals="<<report.removals
        <<" before="<<report.before_loss<<" after="<<report.after_loss<<" blocked_vertex="<<report.blocked_interior_vertex<<" reason="<<reason<<std::endl;
    if(progress)samples("replay_repaired_rejected",repaired);
    CHECK(Signature(world,input.cells,reference)==before);return;  // Diagnostic replay does not qualify full construction/release.
  }
  size_t captured=0;
  options.rejected_trial=[&](const Patch& old,const Patch& fresh,Feature feature,const MetricDiagnosis& metric,
                            bool geometry_valid,const std::string& reason) {
    // One latest pair per worker bounds evidence size; the log retains the trial sequence.
    const auto name="curved_rejected_patch_rank"+std::to_string(world.rank);
    SaveRecords(name+"_old",PatchRecords(old),captured_source,reference);
    SaveRecords(name+"_proposal",PatchRecords(fresh),captured_source,reference);
    std::ofstream out(name+"_diagnosis.json");out<<std::setprecision(17)
        <<"{\"trial\":"<<++captured<<",\"feature\":["<<feature.vertices[0]<<','<<feature.vertices[1]<<','<<feature.vertices[2]
        <<"],\"feature_count\":"<<feature.count<<",\"geometry_valid\":"<<(geometry_valid?"true":"false")
        <<",\"metric_failure\":"<<unsigned(metric.failure)<<",\"shape_measured\":"<<(metric.shape_measured?"true":"false")<<",\"q\":";
    if(metric.shape_measured)out<<metric.minimum_mean_ratio;else out<<"null";
    out<<",\"j\":";if(metric.shape_measured)out<<metric.minimum_scaled_jacobian;else out<<"null";
    out<<",\"weakest_cell\":"<<metric.weakest_cell
        <<",\"sliver_cell\":"<<metric.sliver_cell<<",\"reason\":"<<std::quoted(reason)<<"}\n";
    REQUIRE(out.good());
    std::cout<<"CURVED_REJECTED_TRIAL rank="<<world.rank<<" trial="<<captured<<" old="<<old.cells.size()<<" fresh="<<fresh.cells.size()
        <<" geometry="<<geometry_valid<<" failure="<<unsigned(metric.failure)<<" q=";
    if(metric.shape_measured)std::cout<<metric.minimum_mean_ratio;else std::cout<<"unavailable";
    std::cout<<" j=";if(metric.shape_measured)std::cout<<metric.minimum_scaled_jacobian;else std::cout<<"unavailable";
    std::cout<<" reason="<<reason<<std::endl;
  };
  options.maximum_seconds=300;  // Return/save private progress before the collector's hard timeout.
  const auto cold_started=std::chrono::steady_clock::now();
  const auto refined=Refine(world,input.cells,input.original,*reference.original,compose,options);
  if(!world.rank)std::cout<<"CURVED_NATIVE3D refine valid="<<refined.valid<<" work="<<refined.stats.global_work<<" reason="<<refined.reason<<std::endl;
  if(!world.rank) {
    const auto& e=refined.stats;const auto& m=e.last_metric_rejection;
    std::cout<<std::setprecision(17)<<"CURVED_NATIVE3D_DIAG accepted="<<e.accepted<<" rejected="<<e.rejected
        <<" placement_trials="<<e.placement_trials<<" placement_validated="<<e.placement_validated
        <<" topology_trials="<<e.wall_topology_trials<<" topology_accepted="<<e.wall_topology_accepted
        <<" joint_trials="<<e.wall_joint_trials<<" joint_moves="<<e.wall_joint_moves<<" joint_removals="<<e.wall_joint_removals<<" joint_accepted="<<e.wall_joint_accepted
        <<" geometry_rejected="<<e.geometry_rejected<<" shape_rejected="<<e.metric_shape_rejected
        <<" failure="<<unsigned(m.failure)<<" q=";
    if(m.shape_measured)std::cout<<m.minimum_mean_ratio;else std::cout<<"unavailable";
    std::cout<<" j=";if(m.shape_measured)std::cout<<m.minimum_scaled_jacobian;else std::cout<<"unavailable";
    std::cout
        <<" feature_count="<<e.last_rejected_feature.count<<" v0="<<e.last_rejected_feature.vertices[0]
        <<" v1="<<e.last_rejected_feature.vertices[1]<<" v2="<<e.last_rejected_feature.vertices[2]
        <<" x="<<e.last_rejected_cavity_centroid.x<<" y="<<e.last_rejected_cavity_centroid.y<<" z="<<e.last_rejected_cavity_centroid.z<<std::endl;
  }
  save(refined.valid?"curved_cylinder_BL":"curved_cylinder_BL_rejected",refined.cells);
  CHECK(Signature(world,input.cells,reference)==before);INFO(refined.reason);REQUIRE(refined.valid);CHECK(refined.rebuilt_domain);
  if(std::getenv("SU2_NATIVE_CURVED_REFRESH_SOURCE")) {
    auto import_current=[&](const std::string& name) {
      SU2_MPI::Barrier(world.comm);auto cfg=text.str();const auto at=cfg.find("unused.su2");REQUIRE(at!=std::string::npos);cfg.replace(at,10,name+".su2");
      std::stringstream inputText(cfg);std::unique_ptr<CConfig> c;
      {Mute mute;c=std::make_unique<CConfig>(inputText,SU2_COMPONENT::SU2_CFD,false);}
      CSU2ASCIIMeshReaderFVM reader(c.get(),0,1);MeshSolution mesh(c.get(),reader,0);
      su2activematrix sensor(mesh.Fine().GetnPointDomain(),6);
      for(unsigned long p=0;p<sensor.rows();++p){const double scale=1+.02*SU2_TYPE::GetValue(mesh.Fine().nodes->GetCoord(p,2));
        for(unsigned k=0;k<6;++k)sensor(p,k)=k==0||k==3||k==5?scale:0;}
      auto authority=reference;return ImportGeometry(*c,mesh.Fine(),&sensor,authority);
    };
    auto check_current=[&](const std::string& name,const ImportedGeometry& source,const EngineResult& result,bool layers) {
      REQUIRE(result.valid);
      const auto current=world.sum(source.original.size());CHECK(current>MaximumDonors);
      const auto payloads=world.sum(result.stats.region_field_imports);CHECK(payloads>0);
      CHECK(result.stats.region_field_maximum_resident<=MaximumDonors);
      Save(world,name,result.cells,source.original,reference);
      std::vector<OwnedCell> local;for(const auto& row:result.cells)local.push_back(row.second);auto cells=world.metadata(local);
      DonorImport donors(world);std::string reason;REQUIRE(donors.Build(source.original,reason));
      double residual=0,q=1,j=1;long double maximum=0;CLocalFailure failure;
      RegionFieldService verifier(world,donors,*reference.original,layers?compose:MetricComposition{},layers?options.edge_breakpoints:MetricEdgeBreakpoints{},false,SIZE_MAX,0);
      try{verifier.Run([&](const RegionMetric& metric){for(const auto& row:cells){
        const auto& c=row.cell;const auto p=Centroid(c);const double scale=1+.02*p.z;
        const auto actual=metric.point(p),expected=layers?compose(p,{scale,0,0,scale,0,scale}):Tensor{scale,0,0,scale,0,scale};
        const double a[6]={actual.xx,actual.xy,actual.xz,actual.yy,actual.yz,actual.zz},b[6]={expected.xx,expected.xy,expected.xz,expected.yy,expected.yz,expected.zz};
        for(unsigned k=0;k<6;++k)residual=std::max(residual,std::abs(a[k]-b[k])/(1+std::abs(b[k])));
        const auto shape=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},actual);q=std::min(q,shape.mean_ratio);j=std::min(j,shape.minimum_scaled_jacobian);
        for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b)maximum=std::max(maximum,metric.edge(c.v[a].p,c.v[b].p));
      }});}catch(const std::exception& e){failure.Set(1,0,e.what());}
      const auto rejected=world.elect(failure);INFO(rejected.message);REQUIRE_FALSE(rejected.any);
      if(!world.rank){CHECK(residual<2e-12);CHECK(q>=MinimumMeanRatio);CHECK(j>=MinimumScaledJacobian);CHECK(maximum<=MaximumMetricEdge);
        std::ofstream out(name+"_summary.json");out<<std::setprecision(17)<<"{\"original_donors\":"<<current<<",\"cells\":"<<cells.size()
            <<",\"sensor_target_residual\":"<<residual<<",\"shape_measured\":true,\"q\":"<<q<<",\"j\":"<<j<<",\"maximum_full_length\":"<<maximum
            <<",\"work\":"<<result.stats.global_work<<",\"field_imports\":"<<result.stats.region_field_imports
            <<",\"sensor_hits\":"<<result.stats.region_field_sensor_hits<<",\"interior_hits\":"<<result.stats.region_field_interior_hits
            <<",\"edge_hits\":"<<result.stats.region_field_edge_hits<<",\"maximum_resident\":"<<result.stats.region_field_maximum_resident<<"}\n";REQUIRE(out.good());}
    };
    const auto current=import_current("curved_cylinder_BL");const auto signature=Signature(world,current.cells,reference);
    auto release=options;release.wall_heights.clear();release.region_walls.clear();release.edge_breakpoints={};release.rejected_trial={};
    const auto coarse=Coarsen(world,current.cells,current.original,*reference.original,{},release);
    save(coarse.valid?"refreshed_released":"refreshed_release_rejected",coarse.cells,&current.original);
    CHECK(Signature(world,current.cells,reference)==signature);INFO(coarse.reason);check_current("refreshed_release",current,coarse,false);
    const auto updated=import_current("refreshed_released");const auto before=Signature(world,updated.cells,reference);
    const auto rebuilt=Refine(world,updated.cells,updated.original,*reference.original,compose,options);
    save(rebuilt.valid?"refreshed_BL":"refreshed_BL_rejected",rebuilt.cells,&updated.original);
    CHECK(Signature(world,updated.cells,reference)==before);INFO(rebuilt.reason);check_current("refreshed_rebuild",updated,rebuilt,true);
    return;  // Refreshed actual mesh sources with affine sensors, not CFD solution transfer or accuracy qualification.
  }
  const auto allOriginal=world.metadata(input.original);
  FrozenField target(allOriginal,compose,false,AssociatedExtensions(allOriginal,*reference.original),options.edge_breakpoints);
  double layer_q=1,layer_j=1,layer_l=0;
  for(const auto& row:refined.cells) {
    PatchMetric measured;std::string reason;CHECK(ValidateMetricPatch({row.second.cell},target,measured,reason));
    CHECK(target.Query(Centroid(row.second.cell)).sensor.xx==Approx(1.));
    layer_q=std::min(layer_q,measured.minimum_mean_ratio);layer_j=std::min(layer_j,measured.minimum_scaled_jacobian);
    layer_l=std::max(layer_l,double(measured.maximum_length));
  }
  options.wall_heights.clear();options.region_walls.clear();options.edge_breakpoints={};options.rejected_trial={};options.maximum_work-=refined.stats.global_work;
  options.maximum_seconds=CPassiveComm::Allreduce(300-std::chrono::duration<double>(std::chrono::steady_clock::now()-cold_started).count(),CPassiveComm::Op::MIN,world.comm);
  REQUIRE(options.maximum_seconds>0);
  const auto released=Coarsen(world,refined.cells,input.original,*reference.original,{},options);
  if(!world.rank)std::cout<<"CURVED_NATIVE3D release valid="<<released.valid<<" work="<<released.stats.global_work<<" reason="<<released.reason<<std::endl;
  save(released.valid?"curved_cylinder_released":"curved_cylinder_released_rejected",released.cells);
  INFO(released.reason);REQUIRE(released.valid);
  FrozenField sensorOnly(allOriginal,{},false,AssociatedExtensions(allOriginal,*reference.original));
  double release_q=1,release_j=1,release_l=0;
  for(const auto& row:released.cells) {
    PatchMetric measured;std::string reason;REQUIRE(ValidateMetricPatch({row.second.cell},sensorOnly,measured,reason));
    release_q=std::min(release_q,measured.minimum_mean_ratio);release_j=std::min(release_j,measured.minimum_scaled_jacobian);
    release_l=std::max(release_l,double(measured.maximum_length));
  }
  layer_q=CPassiveComm::Allreduce(layer_q,CPassiveComm::Op::MIN,world.comm);layer_j=CPassiveComm::Allreduce(layer_j,CPassiveComm::Op::MIN,world.comm);
  release_q=CPassiveComm::Allreduce(release_q,CPassiveComm::Op::MIN,world.comm);release_j=CPassiveComm::Allreduce(release_j,CPassiveComm::Op::MIN,world.comm);
  layer_l=CPassiveComm::Allreduce(layer_l,CPassiveComm::Op::MAX,world.comm);release_l=CPassiveComm::Allreduce(release_l,CPassiveComm::Op::MAX,world.comm);
  const auto layer_count=world.sum(refined.cells.size()),release_count=world.sum(released.cells.size());
  CHECK(release_count<layer_count);CHECK(refined.stats.global_work+released.stats.global_work<=4096);
  if(!world.rank) {
        std::ofstream out("cold_summary.json");out<<std::setprecision(17)<<"{\"ranks\":"<<world.size<<",\"shape_measured\":true,\"layer_cells\":"<<layer_count
        <<",\"released_cells\":"<<release_count<<",\"layer_q\":"<<layer_q<<",\"layer_j\":"<<layer_j<<",\"layer_maximum_length\":"<<layer_l
        <<",\"release_q\":"<<release_q<<",\"release_j\":"<<release_j<<",\"release_maximum_length\":"<<release_l
        <<",\"construction_work\":"<<refined.stats.global_work<<",\"removal_work\":"<<released.stats.global_work<<"}\n";REQUIRE(out.good());
  }

}

#include "fixtures/native_curved_rans_runtime.inc"

#include "fixtures/native_region_core_fields.inc"


TEST_CASE("Candidate complexity reassesses original P1 and actual composition without donor refitting", "[NativeReassessment3D]") {
  World world;std::string reason;
  auto geometry=[&](int divisions,bool original) {
    const auto mesh=simplex_test::MakeSimplexMesh(3,divisions,[](const passivedouble*){return "far";});
    ImportedGeometry result;
    for(size_t i=0;i<mesh.GetnElem();++i) {
      Cell c;c.id=i;DonorCell d;
      for(unsigned k=0;k<4;++k) {const auto n=mesh.elem[4*i+k];c.v[k]={n,{mesh.coord[3*n],mesh.coord[3*n+1],mesh.coord[3*n+2]}};}
      if(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)<0)std::swap(c.v[0],c.v[1]);
      d.cell=c;for(unsigned k=0;k<4;++k)d.sensor[k]={1+c.v[k].p.x,0,0,1,0,1};
      if(original && (i+1)%world.size==unsigned(world.rank))result.original.push_back(d);
      if(i%world.size==unsigned(world.rank)){OwnedCell row;row.cell=c;result.cells.emplace(i,row);}
    }return result;
  };
  const auto input=geometry(1,true),fine=geometry(4,false);const auto saved=Bytes(input.original);
  DonorImport source(world,true);REQUIRE(source.Build(input.original,reason));CHECK(source.BuiltFrom(input.original));
  const auto copy=input.original;CHECK_FALSE(source.BuiltFrom(copy));
  CReaderSlices preserved;preserved.nDim=99;ReferenceState empty;BridgeStats denied;
  CHECK_FALSE(ReaderSlices(world,input.cells,copy,empty,preserved,reason,denied,{},size_t(64)<<20,0,true,&source));
  CHECK(preserved.nDim==99);CHECK(reason.find("Prepared native3D reader source")!=std::string::npos);
  if(world.size>1) {
    CHECK_FALSE(ReaderSlices(world,input.cells,input.original,empty,preserved,reason,denied,{},size_t(64)<<20,0,true,world.rank==0?&source:nullptr));
    CHECK(preserved.nDim==99);CHECK(reason.find("Prepared native3D reader source")!=std::string::npos);
  }
  PlanarReference reference(std::vector<Facet>{});uint64_t compositions=0;
  const auto compose=[&](Point p,Tensor m){++compositions;m.zz*=1+.25*p.z;return m;};
  ComplexityReassessment3D coarse,refined;BridgeStats a,b;
  REQUIRE(ReassessComplexity(world,input.cells,source,reference,compose,coarse,reason,a,size_t(64)<<20));
  REQUIRE(ReassessComplexity(world,fine.cells,source,reference,compose,refined,reason,b,size_t(64)<<20));
  REQUIRE(coarse.estimated);REQUIRE(refined.estimated);CHECK(refined.samples==8192);
  const double exact=(2./3*(std::pow(2.,1.5)-1))*(2./(.75)*(std::pow(1.25,1.5)-1));
  CHECK(std::abs(refined.value-exact)<=5*std::max(refined.standard_error,refined.block_standard_error));
  CHECK(refined.uncertainty<.7*coarse.uncertainty);CHECK(compositions==a.compositions+b.compositions);
  CHECK(Bytes(input.original)==saved);CHECK(source.Statistics().builds==1);
  ComplexityReassessment3D sentinel;sentinel.value=123;sentinel.samples=77;BridgeStats failed;
  CHECK_FALSE(ReassessComplexity(world,fine.cells,source,reference,[](Point,Tensor){return Tensor{-1,0,0,1,0,1};},sentinel,reason,failed,size_t(64)<<20));
  CHECK(sentinel.value==123);CHECK(sentinel.samples==77);
  REQUIRE(ReassessComplexity(world,fine.cells,source,reference,compose,sentinel,reason,failed,size_t(64)<<20,0,128,1,[]{return false;}));
  CHECK_FALSE(sentinel.estimated);CHECK(std::isinf(sentinel.uncertainty));CHECK_FALSE(sentinel.reason.empty());
  if(!world.rank) {
    std::ofstream out("original_field_reassessment.json");out<<std::setprecision(17)<<"{\"coarse\":"<<coarse.value<<",\"fine\":"<<refined.value
      <<",\"reference\":"<<exact<<",\"coarse_indicator\":"<<coarse.uncertainty<<",\"fine_indicator\":"<<refined.uncertainty
      <<",\"original_source_unchanged\":true,\"source_builds\":"<<source.Statistics().builds<<",\"proven_bound\":false}\n";REQUIRE(bool(out));
  }
}

#include "fixtures/native_normalization_stages.inc"

#include "fixtures/native_saved_metric_probe.inc"

#include "fixtures/native_sphere_bl_runtime.inc"

TEST_CASE("Mapped boundary records follow actual CFD reader numbering output and reload", "[NativeMappedReader3D]") {
  World world;const bool rans=GENERATE(false,true);
  const std::string physics=rans?"SOLVER= RANS\nKIND_TURB_MODEL= SA\nREYNOLDS_NUMBER= 1e6\nREYNOLDS_LENGTH= 1\n":"SOLVER= EULER\n";
  auto config=MakeConfig(3,physics+"TIME_DOMAIN= YES\nTIME_STEP= .001\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n");
  auto mesh=BoxMesh(3,1,false);
  for(size_t p=0;p<mesh.GetnPoint();++p)mesh.coord[3*p+1]+=.37*mesh.coord[3*p]+.61*mesh.coord[3*p+2];
  MeshSolution current(config.get(),mesh,0);ReferenceState reference;
  ImportGeometry(*config,current.Fine(),nullptr,reference);const auto saved=State(current);
  std::map<Id,double> errors;std::map<Id,SU2NativeGeometry::AffineAuthority> planes;
  for(const auto& f:reference.facets) {
    errors.emplace(f.id,1e-13);
    if(reference.marker_names.at(f.marker)=="y_minus")planes.emplace(f.id,SU2NativeGeometry::AffineAuthority{17,3,3,{0,0,0},{-.37,1,-.61},1e-13});
  }
  reference.original=std::make_shared<PlanarReference>(reference.facets,SIZE_MAX,std::map<Id,AnalyticSurface>{},planes);
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(reference.original,errors,7,SIZE_MAX,std::getenv("SU2_NATIVE_MAPPED_COMPONENTS")!=nullptr);
  BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:reference.facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});
    for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});}
  auto parameters=std::make_shared<BoundaryParameterState>(authority,points,skin);
  reference.original=PlanarReference::WithParameters(reference.original,parameters);
  const auto encoded=EncodeReference(reference);auto imported=ImportGeometry(*config,current.Fine(),nullptr,reference);
  CHECK(State(current)==saved);
  // Force a nontrivial old-ID namespace; the house reader must supply the actual map back to dense IDs.
  const Id offset=1000;Owned shifted;
  for(auto row:imported.cells){for(auto& n:row.second.cell.v)n.id+=offset;shifted.emplace(row.first,row.second);}
  std::map<Id,Id> numbering;for(auto row:parameters->Records())numbering.emplace(row.first,row.first+offset);
  reference.original=reference.original->WithParameterState(parameters->Rebind(numbering));
  const auto shifted_bytes=reference.original->Parameters()->Snapshot();
  CReaderSlices output;output.nDim=99;std::shared_ptr<const PlanarReference> returned=reference.original;BridgeStats stats;std::string reason;
  CHECK_FALSE(ReaderSlices(world,shifted,imported.original,reference,output,reason,stats,{},world.rank?SIZE_MAX:1,0,true,nullptr,&returned));
  CHECK(output.nDim==99);CHECK(returned==reference.original);CHECK(reference.original->Parameters()->Snapshot()==shifted_bytes);
  REQUIRE(ReaderSlices(world,shifted,imported.original,reference,output,reason,stats,{},SIZE_MAX,0,true,nullptr,&returned));
  REQUIRE(returned);CHECK(returned->Parameters()->Revision()==3);CHECK(reference.original->Parameters()->Snapshot()==shifted_bytes);
  reference.original=returned;MeshSolution fresh(config.get(),output,0);
  auto rebound=ImportGeometry(*config,fresh.Fine(),nullptr,reference);
  CHECK(CPassiveComm::Allreduce(uint64_t(rebound.cells.size()),CPassiveComm::Op::SUM,world.comm)==output.nElemGlobal);
  CHECK(State(current)==saved);
  const std::string name=rans?"mapped_reader_rans":"mapped_reader_euler";
  OutputRoundTrip(world,name,output,shifted,reference,1.,[](const su2double*,double){return Tensor{1,0,0,1,0,1};});
  if(world.size>1) {
    // Inactive ranks retain the old boundary IDs until the explicit parent stage return.
    auto parent_state=DecodeReference(encoded);const int workers=world.size/2;
    if(world.rank<workers)parent_state.original=parent_state.original->WithParameterState(BoundaryParameterState::FromPayload(
        parent_state.original->Parameters()->Authority(),returned->Parameters()->Payload()));
    BridgeStats synchronized;REQUIRE(SynchronizeBoundaryParameters(world,workers,parent_state,reason,synchronized));
    CHECK(parent_state.original->Parameters()->Snapshot()==returned->Parameters()->Snapshot());CHECK(synchronized.parameter_return_bytes>0);
  }
}

TEST_CASE("Mapped native runtime completes a uniform target before conservative BDF2 transfer", "[NativeMappedRuntime3D]") {
  World world;const bool rans=GENERATE(false,true);
  const bool curved=GENERATE(false,true);
  const auto physics=rans?std::string("SOLVER= RANS\nKIND_TURB_MODEL= SA\nREYNOLDS_NUMBER= 1e6\nREYNOLDS_LENGTH= 1\n"):std::string("SOLVER= EULER\n");
  const auto all=std::getenv("SU2_NATIVE_MAPPED_ALL_WORKERS");
  const int workers=all&&std::string(all)=="1"?world.size:(world.size>1?world.size/2:1);
  auto config=MakeConfig(3,physics+"COMPUTE_METRIC= YES\nADAP_SENSOR= PRESSURE\nADAP_HESSIAN_NOISE= 0\nADAP_REMESHER= NATIVE_CAVITY\nADAP_SURFACE= YES\n"
      "ADAP_NATIVE_REPARTITION= YES\nADAP_HAUSD= .001\nADAP_NATIVE_RANKS= "+std::to_string(workers)+"\nTIME_DOMAIN= YES\nTIME_STEP= .001\nTIME_ITER= 10\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\n");
  auto mesh=BoxMesh(3,1,false);for(size_t p=0;p<mesh.GetnPoint();++p)mesh.coord[3*p+1]+=.37*mesh.coord[3*p]+.61*mesh.coord[3*p+2];
  const auto base=AffineFlow(3);
  const transfer_test::Field flow=[base,rans,curved](const su2double* x,su2double* u){
    if(curved){const std::array<su2double,5> constant{1.,.2,.1,.05,3.};std::copy(constant.begin(),constant.end(),u);}
    else {base(x,u);if(rans){u[1]=20*x[2];u[2]=3*x[2];u[3]=2*x[2];}}};
  MeshSolution current(config.get(),mesh,0);current.SetField(FLOW_SOL,flow);
  if(rans)current.SetField(TURB_SOL,[](const su2double*,su2double* u){u[0]=.01;});
  for(auto slot:{FLOW_SOL,TURB_SOL}) {
    if(slot==TURB_SOL&&!rans)continue;auto* solver=current.solver[MESH_0][slot];auto* nodes=solver->GetNodes();
    for(unsigned long p=0;p<current.Fine().GetnPoint();++p)for(unsigned k=0;k<solver->GetnVar();++k) {
      nodes->GetSolution_time_n()(p,k)=nodes->GetSolution(p,k);nodes->GetSolution_time_n1()(p,k)=.96875*nodes->GetSolution(p,k);
    }
  }
  auto reference=std::make_shared<ReferenceState>();ImportGeometry(*config,current.Fine(),nullptr,*reference);
  if(curved) {
    std::reverse(reference->marker_names.begin(),reference->marker_names.end());
    for(auto& f:reference->facets)f.marker=reference->marker_names.size()-1-f.marker;
    reference->original=std::make_shared<PlanarReference>(reference->facets);
  }
  std::map<Id,double> errors;for(auto f:reference->facets)errors.emplace(f.id,1e-13);
  SU2NativeGeometry::MappedFacetReference::CurvedNets nets;
  if(curved) {
    const unsigned powers[10][3]={{3,0,0},{0,3,0},{0,0,3},{2,1,0},{1,2,0},{0,2,1},{0,1,2},{1,0,2},{2,0,1},{1,1,1}};
    for(const auto& f:reference->facets) {
      for(unsigned q=0;q<10;++q){long double x=0,y=0,z=0;for(unsigned k=0;k<3;++k){
        x+=powers[q][k]*(long double)f.v[k].p.x;y+=powers[q][k]*(long double)f.v[k].p.y;z+=powers[q][k]*(long double)f.v[k].p.z;}
        nets[f.id][q]={double(x/3),double(y/3),double(z/3)};}
      if(std::all_of(f.v.begin(),f.v.end(),[](Node n){return n.p.z==0;}))nets[f.id][9].z-=.02;
    }
  }
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(reference->original,errors,7,SIZE_MAX,!curved && std::getenv("SU2_NATIVE_MAPPED_COMPONENTS")!=nullptr,nets);
  BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(auto f:reference->facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(auto n:f.v)points.emplace(n.id,BoundaryPoint{n,authority->Vertex(n.id)});}
  reference->original=PlanarReference::WithParameters(reference->original,std::make_shared<BoundaryParameterState>(authority,points,skin));
  const auto original_reference=EncodeReference(*reference);const auto saved=State(current);
  su2activematrix sensor(current.Fine().GetnPointDomain(),6);
  for(size_t p=0;p<sensor.rows();++p)for(unsigned k=0;k<6;++k)sensor(p,k)=(k==0||k==3||k==5)?4.:0.;
  EngineOptions limits;limits.maximum_seconds=120;limits.timings=true;
  const char* work_trace=std::getenv("SU2_NATIVE_GEOMETRY_WORK_TRACE");
  if(work_trace && std::string(work_trace)=="1")
    limits.rejected_trial=[&](const Patch& old,const Patch& fresh,Feature feature,const MetricDiagnosis&,bool geometry,const std::string& reason) {
      std::cerr<<"MAPPED_REJECTION rans="<<rans<<" curved="<<curved<<" feature_count="<<unsigned(feature.count)
        <<" feature="<<feature.vertices[0]<<','<<feature.vertices[1]<<','<<feature.vertices[2]
        <<" geometry="<<geometry<<" old_cells="<<old.cells.size()<<" new_cells="<<fresh.cells.size()<<" reason="<<reason<<'\n';
    };
  if(curved && work_trace && std::string(work_trace)=="1")
    limits.transaction_observer=[&](const TransactionView& view) {
      constexpr size_t ceiling=size_t(128)<<20;
      const auto* parameters=view.geometry.Parameters();REQUIRE(parameters);
      const auto retained=transfer_memory::Add(view.geometry.ResidentBytes(),
          transfer_memory::Mul(view.old.cells.size()+view.fresh.cells.size(),sizeof(Cell)),parameters->PayloadBytes());
      if(retained>ceiling/4)throw std::length_error("First operator capture exceeds independent storage admission.");
      const auto name="cubic_first_failure_"+std::to_string(rans)+"_rank"+std::to_string(world.rank);
      auto captured=*reference;captured.original=view.geometry.WithParameterState(std::make_shared<BoundaryParameterState>(*parameters));
      const auto encoded=EncodeReference(captured);
      std::ofstream reference_file(name+".native_ref",std::ios::binary);reference_file.write(encoded.data(),encoded.size());reference_file.close();REQUIRE(bool(reference_file));
      SaveRecords(name+"_old",PatchRecords(view.old),{},captured);
      SaveRecords(name+"_proposal",PatchRecords(view.fresh),{},captured);
      auto budget=BoundaryAtlasBudget(parameters->PayloadBytes(),ceiling,retained);
      if(view.geometry.AtlasReservation())budget.maximum_work=view.geometry.AtlasReservation()->maximum_work;
      bool supported=false,built=false,error_measured=false;double error=0;std::string why;Patch replay;
      try {
        auto scoped=view.geometry.PrivateParameters(view.old,&budget);const Edge edge{view.feature.vertices[0],view.feature.vertices[1]};
        if(std::string(view.stage)=="first_collapse_failure") {
          supported=true;built=CollapseVertex(view.old,edge[1],edge[0],view.first_cell,*scoped,replay,why);
        } else if(std::string(view.stage)=="first_split_failure") {
          supported=true;built=SplitEdge(view.old,edge,view.point,view.first_cell,*scoped,replay,why);
        }
        if(built) {
          auto proof=scoped->PrivateParameters(replay,&budget);std::map<Id,Point> coordinates;
          for(const auto& cell:replay.cells)for(auto n:cell.v)coordinates.emplace(n.id,n.p);
          for(const auto& face:replay.surface){std::array<Point,3> p;for(unsigned k=0;k<3;++k)p[k]=coordinates.at(face.v[k]);
            error=std::max(error,proof->SurfaceError(face.facet,p));}
          error_measured=true;
        }
      }catch(const std::exception& e){why=e.what();}
      std::ofstream receipt(name+".json");receipt<<std::setprecision(17)
          <<"{\"stage\":"<<std::quoted(view.stage)<<",\"feature\":["<<view.feature.vertices[0]<<','<<view.feature.vertices[1]
          <<"],\"round\":"<<view.round<<",\"reason\":"<<std::quoted(view.reason)<<",\"old_physical_faces\":"<<view.old.surface.size()
          <<",\"replay_supported\":"<<(supported?"true":"false")<<",\"replay_built\":"<<(built?"true":"false")
          <<",\"replay_reason\":"<<std::quoted(why)<<",\"maximum_surface_error_bound\":";
      if(error_measured)receipt<<error;else receipt<<"null";
      receipt<<",\"geometry_tolerance\":"<<config->GetAdap_Hausd()<<",\"fresh_work\":"<<budget.work
          <<",\"work_limit\":"<<budget.maximum_work<<",\"work_exhausted\":"<<(budget.work_exhausted?"true":"false")
          <<",\"admitted_peak_bytes\":"<<budget.peak_bytes<<",\"accepted_mesh\":false}\n";
      receipt.close();REQUIRE(bool(receipt));
    };
  limits.rejected_state=[&](const Owned& owned,const PlanarReference& geometry,const std::string& why) {
    const auto name=std::string(curved?"cubic":"affine")+"_runtime_rejected_"+std::to_string(rans);
    const char* trace=std::getenv("SU2_NATIVE_GEOMETRY_WORK_TRACE");
    if(trace&&std::string(trace)=="1") {
      const auto* parameters=geometry.Parameters();if(!parameters)throw std::runtime_error("Rejected capture lacks boundary parameters.");
      constexpr size_t ceiling=size_t(128)<<20,scratch=1024;const auto size=parameters->PayloadBytes();
      if(owned.size()>(ceiling-scratch)/sizeof(OwnedCell))throw std::length_error("Rejected cell capture exceeds admission.");
      const auto retained=scratch+owned.size()*sizeof(OwnedCell);
      if(original_reference.size()>ceiling-retained || size>(ceiling-retained-original_reference.size())/2)throw std::length_error("Rejected capture payload exceeds admission.");
      const auto prefix=name+"_rank"+std::to_string(world.rank),owned_name=prefix+"_owned.bin",parameter_name=prefix+"_parameters.bin",reference_name=prefix+".native_ref";
      std::ofstream cells(owned_name,std::ios::binary|std::ios::trunc);if(!cells)throw std::runtime_error("Cannot open rejected owned capture.");
      std::vector<char> bytes;bytes.reserve(scratch);
      auto write=[&](auto value){bytes.clear();RecordStream stream(bytes);stream(value);cells.write(bytes.data(),bytes.size());if(!cells)throw std::runtime_error("Cannot write rejected owned capture.");};
      write(uint64_t(owned.size()));for(const auto& row:owned)write(row.second);
      cells.close();if(!cells)throw std::runtime_error("Cannot close rejected owned capture.");
      const auto payload=parameters->Payload();if(payload.size()!=size)throw std::runtime_error("Rejected parameter payload size changed.");
      std::ofstream parameter_file(parameter_name,std::ios::binary|std::ios::trunc);parameter_file.write(payload.data(),payload.size());parameter_file.close();
      if(!parameter_file)throw std::runtime_error("Cannot write rejected parameter capture.");
      std::ofstream authority_file(reference_name,std::ios::binary|std::ios::trunc);authority_file.write(original_reference.data(),original_reference.size());authority_file.close();
      if(!authority_file)throw std::runtime_error("Cannot write rejected original authority.");
      std::ofstream manifest(prefix+"_manifest.json");manifest<<"{\"record_format\":1,\"complete_shard\":true,\"scope\":\"private_mapped_runtime_rejection\",\"private_capture\":true,\"final_mesh_accepted\":false,\"rank\":"<<world.rank
        <<",\"generation\":"<<parameters->Revision()<<",\"owned_count\":"<<owned.size()<<",\"parameter_bytes\":"<<size
        <<",\"owned\":\""<<owned_name<<"\",\"parameters\":\""<<parameter_name<<"\",\"authority\":\""<<reference_name<<"\"}\n";
      manifest.close();if(!manifest)throw std::runtime_error("Cannot write rejected capture manifest.");
    }
    auto diagnostic=BoundaryAtlasBudget(geometry.Parameters()->PayloadBytes(),size_t(128)<<20,geometry.ResidentBytes());
    if(geometry.AtlasReservation())diagnostic.maximum_work=geometry.AtlasReservation()->maximum_work;
    auto proof=geometry.WithAtlasReservation(diagnostic);
    std::ofstream out(name+"_"+std::to_string(world.rank)+".txt");
    out<<std::setprecision(17)<<why<<'\n';
    for(const auto& row:owned) {
      const auto& c=row.second.cell;const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},Tensor{4,0,0,4,0,4});
      double error=0;
      for(unsigned k=0;k<4;++k)if(row.second.physical[k].present) {
        std::array<Point,3> p{};unsigned at=0;for(unsigned i=0;i<4;++i)if(i!=k)p[at++]=c.v[i].p;
        if(k%2)std::swap(p[1],p[2]);
        error=std::max(error,proof->SurfaceError(row.second.physical[k].facet,p));
      }
      out<<c.id<<" q="<<m.mean_ratio<<" J="<<m.minimum_scaled_jacobian<<" edge="<<m.maximum_edge<<" surface="<<error<<'\n';
      for(auto n:c.v)out<<n.id<<' '<<n.p.x<<' '<<n.p.y<<' '<<n.p.z<<'\n';
    }
    if(all && std::string(all)=="1" && std::getenv("SU2_NATIVE_MAPPED_REPAIR_PROBE")) {
      if(CPassiveComm::Allreduce(uint64_t(owned.size()),CPassiveComm::Op::SUM,world.comm)>4096)
        throw std::length_error("Runtime diagnostic mesh exceeds its gather bound.");
      std::vector<std::vector<OwnedCell>> send(world.size);
      for(const auto& row:owned)send[0].push_back(row.second);
      const auto gathered=world.exchange(send);
      if(world.rank==0) {
        Owned whole;Id next_node=0,next_cell=0;
        for(const auto& row:gathered){whole.emplace(row.cell.id,row);next_cell=std::max(next_cell,row.cell.id+1);
          for(auto n:row.cell.v)next_node=std::max(next_node,n.id+1);}
        const RegionMetric target([](Point){return Tensor{4,0,0,4,0,4};},
            [](Point a,Point b){return 2*std::hypot((long double)a.x-b.x,(long double)a.y-b.y,(long double)a.z-b.z);});
        std::ofstream probe(name+"_optimizer.txt");probe<<std::setprecision(17);
        for(unsigned mode=0;mode<3;++mode) {
          auto candidate=whole;auto node=next_node,cell=next_cell;OptimizeOptions options;
          options.maximum_work=4096;options.maximum_seconds=5;options.cache_entries=8192;
          options.boundary=mode==2;options.smoothing=options.insertions=mode>0;
          OptimizeReport report;OptimizeMetricMesh(candidate,target,node,cell,options,report);
          probe<<"mode="<<mode<<" before="<<report.bad_before<<" after="<<report.bad_after
               <<" score="<<report.minimum_after<<" work="<<report.trials<<" stop="<<report.stop
               <<" swaps="<<report.edge_removals+report.face_swaps<<" moves="<<report.moves
               <<" insertions="<<report.shell_insertions+report.neighbourhood_insertions+report.length_insertions
               <<" boundary="<<report.boundary_flips+report.boundary_collapses<<'\n';
        }
      }
    }
  };
  CNativeRemesher3D backend(reference,limits);const auto result=backend.Remesh(*config,current.Fine(),sensor);
  INFO("refine="<<EngineStopName(backend.Statistics().refine.stop)<<", coarsen="<<EngineStopName(backend.Statistics().coarsen.stop));
  REQUIRE(result.status==CRemeshResult::Status::COMPLETE);CHECK(EncodeReference(*reference)==original_reference);CHECK(State(current)==saved);
  if(curved) {
    CHECK(CPassiveComm::Allreduce(backend.Statistics().refine.geometry_progress,CPassiveComm::Op::SUM,world.comm)>0);
    CHECK(CPassiveComm::Allreduce(backend.Statistics().refine.geometry_reactivated,CPassiveComm::Op::SUM,world.comm)>0);
    if(workers>1)CHECK(world.maximum(backend.Statistics().refine.maximum_parallel_commits)>1);
  }
  MeshSolution accepted(config.get(),result.slices,0);CConservativeTransfer transfer;
  {Mute mute;transfer.Transfer(config.get(),current.Donor(),accepted.geometry,accepted.solver);}
  const std::string name=std::string(curved?"cubic_":"")+(rans?"mapped_runtime_rans":"mapped_runtime_euler");
  const auto transported=result.slices.FetchPointMetric(accepted.Fine());
  SaveStates(world,name,current,accepted,transported,4.);SaveProjection(world,name,transfer);
  std::vector<bool> wall(accepted.Fine().GetnPoint(),false);double volumeScale=1,momentumScale=1;
  if(curved && rans) {
    // Closed-wall domain growth conserves content; no-slip momentum moves to the free dual volumes.
    for(unsigned short m=0;m<accepted.Fine().GetnMarker();++m)if(config->GetMarker_All_KindBC(m)==HEAT_FLUX)
      for(unsigned long v=0;v<accepted.Fine().GetnVertex(m);++v)wall[accepted.Fine().vertex[m][v]->GetNode()]=true;
    std::vector<Id> localWall;for(size_t p=0;p<wall.size();++p)if(wall[p])localWall.push_back(accepted.Fine().nodes->GetGlobalIndex(p));
    const auto globalWall=world.exchange(std::vector<std::vector<Id>>(world.size,localWall));
    const std::set<Id> wallIds(globalWall.begin(),globalWall.end());
    for(size_t p=0;p<wall.size();++p)wall[p]=wallIds.count(accepted.Fine().nodes->GetGlobalIndex(p));
    double oldVolume=0,newVolume=0,freeVolume=0;
    for(unsigned long p=0;p<current.Fine().GetnPointDomain();++p)oldVolume+=SU2_TYPE::GetValue(current.Fine().nodes->GetVolume(p));
    for(unsigned long p=0;p<accepted.Fine().GetnPointDomain();++p) {
      const double volume=SU2_TYPE::GetValue(accepted.Fine().nodes->GetVolume(p));newVolume+=volume;if(!wall[p])freeVolume+=volume;
    }
    oldVolume=CPassiveComm::Allreduce(oldVolume,CPassiveComm::Op::SUM,world.comm);
    newVolume=CPassiveComm::Allreduce(newVolume,CPassiveComm::Op::SUM,world.comm);
    freeVolume=CPassiveComm::Allreduce(freeVolume,CPassiveComm::Op::SUM,world.comm);
    REQUIRE(newVolume>oldVolume);REQUIRE(freeVolume>0);REQUIRE(freeVolume<newVolume);
    volumeScale=oldVolume/newVolume;momentumScale=oldVolume/freeVolume;
    CHECK(transfer.GetSummary().nWallPoints>0);CHECK(transfer.GetSummary().nTurbLimited==0);
  }
  double error=0;
  for(unsigned long p=0;p<accepted.Fine().GetnPoint();++p) {
    su2double expected[MAXVAR]{};flow(accepted.Fine().nodes->GetCoord(p),expected);const auto* nodes=accepted.solver[MESH_0][FLOW_SOL]->GetNodes();
    if(curved && rans)for(unsigned k=0;k<5;++k)expected[k]*=(k==0||k==4)?volumeScale:(wall[p]?0:momentumScale);
    for(unsigned k=0;k<5;++k){error=std::max(error,RelDiff(nodes->GetSolution(p,k),expected[k]));
      error=std::max(error,RelDiff(nodes->GetSolution_time_n(p,k),expected[k]));error=std::max(error,RelDiff(nodes->GetSolution_time_n1(p,k),.96875*expected[k]));}
    if(rans){const auto* turb=accepted.solver[MESH_0][TURB_SOL]->GetNodes();CHECK(turb->GetSolution(p,0)>=0);CHECK(turb->GetSolution_time_n(p,0)>=0);CHECK(turb->GetSolution_time_n1(p,0)>=0);
      error=std::max(error,RelDiff(turb->GetSolution(p,0),.01*volumeScale));error=std::max(error,RelDiff(turb->GetSolution_time_n(p,0),.01*volumeScale));
      error=std::max(error,RelDiff(turb->GetSolution_time_n1(p,0),.0096875*volumeScale));}
  }
  CHECK(CPassiveComm::Allreduce(error,CPassiveComm::Op::MAX,world.comm)<2e-9);CHECK(State(current)==saved);
  if(result.beforeAcceptance)REQUIRE(CPassiveComm::Allreduce(int(result.beforeAcceptance()),CPassiveComm::Op::MIN,world.comm));
  REQUIRE(result.onAccepted);result.onAccepted();const auto stable=EncodeReference(*reference);result.onAccepted();CHECK(EncodeReference(*reference)==stable);
  OutputRoundTrip(world,name,result.slices,ImportGeometry(*config,accepted.Fine(),nullptr,*reference).cells,*reference,4.,
      [](const su2double*,double scale){return Tensor{scale,0,0,scale,0,scale};});
}

TEST_CASE("Native3D reader returns current nodal sensors outside donor coverage", "[NativeBridge3D]") {
  World world;
  auto config=MakeConfig(3,"SOLVER= EULER\n");
  MeshSolution mesh(config.get(),BoxMesh(3,1,false),0);ReferenceState reference;
  auto imported=ImportGeometry(*config,mesh.Fine(),nullptr,reference);
  for(auto& row:imported.cells) {
    row.second.sensor_present=true;
    row.second.sensor.fill({7,0,0,7,0,7});
  }
  // Keep the valid physical mesh fixed while making its original donor coverage strictly interior.
  for(auto& donor:imported.original)for(auto& node:donor.cell.v) {
    node.p.x=.25+.5*node.p.x;node.p.y=.25+.5*node.p.y;node.p.z=.25+.5*node.p.z;
  }
  CReaderSlices result;BridgeStats stats;std::string reason;
  REQUIRE(ReaderSlices(world,imported.cells,imported.original,reference,result,reason,stats,
      [](Point,Tensor sensor){sensor.zz+=2;return sensor;}));
  CHECK(stats.source_point_queries==0);
  DonorImport original(world,true);REQUIRE(original.Build(imported.original,reason));
  ComplexityReassessment3D assessed;BridgeStats assessment;
  std::shared_ptr<const PreparedComplexity3D> prepared;
  REQUIRE(ReassessComplexity(world,imported.cells,original,*reference.original,{},assessed,reason,assessment,
      size_t(64)<<20,0,128,20261017,{},&prepared));
  REQUIRE(assessed.estimated);REQUIRE(prepared);CHECK(prepared->current_nodal);
  long double local_volume=0,volume=0;
  for(const auto& row:imported.cells) {
    const auto& v=row.second.cell.v;
    local_volume+=TetrahedronVolume({v[0].p,v[1].p,v[2].p,v[3].p});
  }
  SU2_MPI::Allreduce(&local_volume,&volume,1,MPI_LONG_DOUBLE,MPI_SUM,world.comm);
  CHECK(assessed.value==Approx(double(volume)*7*std::sqrt(7.)).margin(1e-10));
  for(size_t i=0;i<result.metric.size();i+=6) {
    CHECK(result.metric[i]==Approx(7));CHECK(result.metric[i+3]==Approx(7));CHECK(result.metric[i+5]==Approx(9));
  }
}
