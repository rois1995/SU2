/*!
 * \file CNativeImport3D.cpp
 * \brief CFD-owned tetrahedral import and original-P1 distributed reader return.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeImport3D.hpp"
#include "../../include/adaptation/CDistributedSearch.hpp"
#include "../../include/adaptation/CMeshGather.hpp"
#include "../../include/CConfig.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include <cmath>
#include <type_traits>
namespace SU2Native3D {
namespace {
std::array<Face, 4> Faces(const Cell& c) {
  const auto& v = c.v;
  return {{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},
           {v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
}
Face Key(Face f) { std::sort(f.begin(),f.end()); return f; }
int Bucket(Face f, int ranks) {
  uint64_t hash = 14695981039346656037ULL;
  for (auto id : Key(f)) { hash ^= id; hash *= 1099511628211ULL; }
  return hash % ranks;
}
int Parity(Face f) {
  unsigned count=0;for(size_t i=0;i<3;++i)for(size_t j=i+1;j<3;++j)count+=f[i]>f[j];return count%2?-1:1;
}
size_t OwnedBytes(const Owned& cells) {
  return transfer_memory::Mul(cells.size(),sizeof(Owned::value_type)+4*sizeof(void*));
}
struct Envelope {
  World& world; BridgeStats& stats; double start;
  Envelope(World& world, BridgeStats& stats):world(world),stats(stats),start(world.seconds()) { stats={}; }
  ~Envelope() {
    stats.total_seconds=world.seconds()-start;
    stats.other_seconds=std::max(0.,stats.total_seconds-stats.points_seconds-stats.cells_seconds-stats.faces_seconds-
        stats.source_seconds-stats.query_seconds-stats.reader_seconds);
  }
};
bool Vote(World& world, const CLocalFailure& failure, std::string& reason) {
  const auto elected=world.elect(failure);reason=elected.message;return !elected.any;
}
bool Admit(World& world,size_t bytes,size_t ceiling,BridgeStats& stats,std::string& reason) {
  stats.maximum_native_bytes=std::max(stats.maximum_native_bytes,bytes);
  CLocalFailure failure;if(bytes==SIZE_MAX || bytes>ceiling)failure.Set(3,0,"Native 3D bridge storage budget exceeded.");
  return Vote(world,failure,reason);
}
void Add(BridgeStats& stats,const FieldStats& field) {
  stats.requests+=field.requests;stats.hits+=field.hits;stats.misses+=field.misses;stats.compositions+=field.compositions;
  stats.field_build_seconds+=field.build_seconds;stats.field_search_seconds+=field.search_seconds;
  stats.interpolation_seconds+=field.interpolation_seconds;stats.composition_seconds+=field.composition_seconds;
}
struct FaceRecord {
  Face face{}; Id cell=0; int owner=0,slot=0,marker=-1,parity=0; bool label=false;
  template<class S> void Fields(S& s) { s(face,cell,owner,slot,marker,parity,label); }
};
struct Binding {
  Id cell=0;int slot=0,marker=0;
  template<class S> void Fields(S& s) { s(cell,slot,marker); }
};
}
ImportedGeometry ImportGeometry(const CConfig& config,const CGeometry& geometry,const su2activematrix* metric,
                                ReferenceState& state) {
  World parent;ImportedGeometry result;
  {
  Envelope envelope(parent,result.stats);CLocalFailure failure;
  if(geometry.GetnDim()!=3 || !std::is_same<su2double,double>::value || !std::is_same<passivedouble,double>::value)
    failure.Set(1,0,"Native tetrahedral import requires a primal double-precision 3D geometry.");
  if(metric && (metric->cols()!=6 || metric->rows()<geometry.GetnPointDomain()))
    failure.Set(1,0,"Native tetrahedral import needs six sensor components at every CFD-owned point.");
  CollectiveFailure(failure,CURRENT_FUNCTION);
  const auto simplices=OwnedSimplices(geometry);
  std::vector<std::string> tags;
  for(unsigned short m=0;m<geometry.GetnMarker();++m)tags.push_back(config.GetMarker_All_TagBound(m));
  const auto names=CMeshGather::GatherMarkerNames(config,tags,geometry);
  const auto physical=OwnedBoundaryFaces(geometry,tags,config);
  const double point_start=parent.seconds();
  CPointDirectory points;std::vector<uint64_t> gids;std::vector<char> bytes;
  for(unsigned long p=0;p<geometry.GetnPointDomain();++p) {
    gids.push_back(geometry.nodes->GetGlobalIndex(p));
    std::array<double,9> values{};
    for(size_t d=0;d<3;++d)values[d]=SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,d));
    for(size_t k=0;k<6;++k)values[k+3]=metric?SU2_TYPE::GetValue((*metric)(p,k)):(k==0 || k==3 || k==5?1.:0.);
    try {
      for(size_t d=0;d<3;++d)if(!std::isfinite(values[d]))throw std::invalid_argument("Nonfinite CFD-owned native3D coordinate.");
      ValidateTensor({values[3],values[4],values[5],values[6],values[7],values[8]});
      RecordStream encode(bytes);encode(values);
    } catch(const std::exception& e) { failure.Set(1,gids.back(),e.what()); }
  }
  CollectiveFailure(failure,CURRENT_FUNCTION);
  points.Build(gids,bytes,72,"native tetrahedral original sensor points");gids.clear();
  for(unsigned long p=0;p<geometry.GetnPoint();++p)gids.push_back(geometry.nodes->GetGlobalIndex(p));
  const auto fetched=points.Fetch(gids);result.stats.points_seconds=parent.seconds()-point_start;
  const double cell_start=parent.seconds();const auto prefix=CPassiveComm::ExscanSum(simplices.size());
  try {
    result.original.reserve(simplices.size());
    for(unsigned long e=0;e<simplices.size();++e) {
      DonorCell donor;donor.cell.id=prefix+e;
      for(size_t k=0;k<4;++k) {
        const auto p=simplices.nodes[4*e+k];const char* data=fetched.data()+72*p;
        const auto x=GetBytes<double>(data),y=GetBytes<double>(data),z=GetBytes<double>(data);
        donor.cell.v[k]={gids[p],{x,y,z}};
        donor.sensor[k]={GetBytes<double>(data),GetBytes<double>(data),GetBytes<double>(data),
                         GetBytes<double>(data),GetBytes<double>(data),GetBytes<double>(data)};
        if(x!=SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,0)) || y!=SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,1)) ||
           z!=SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,2)))throw std::invalid_argument("Inconsistent native3D CFD halo coordinates.");
      }
      const auto& v=donor.cell.v;const auto sign=Orientation(v[0].p,v[1].p,v[2].p,v[3].p);
      if(!sign)throw std::invalid_argument("Degenerate native3D CFD tetrahedron.");
      if(sign<0) { std::swap(donor.cell.v[0],donor.cell.v[1]);std::swap(donor.sensor[0],donor.sensor[1]); }
      result.cells.emplace(donor.cell.id,OwnedCell{donor.cell});result.original.push_back(donor);
    }
  } catch(const std::exception& e) { failure.Set(2,0,e.what()); }
  CollectiveFailure(failure,CURRENT_FUNCTION);result.stats.cells_seconds=parent.seconds()-cell_start;
  const double face_start=parent.seconds();std::vector<std::vector<FaceRecord>> to(parent.size);
  for(unsigned long i=0;i<physical.size();++i) {
    Face f{};for(size_t k=0;k<3;++k)f[k]=gids[physical.nodes[3*i+k]];
    const auto name=config.GetMarker_CfgFile_TagBound(physical.markerId[i]);
    const auto found=std::find(names.begin(),names.end(),name);
    if(found==names.end())failure.Set(1,f[0],"Native3D physical marker is missing from the global name list.");
    to[Bucket(f,parent.size)].push_back({Key(f),0,parent.rank,0,int(found-names.begin()),0,true});
  }
  for(const auto& r:result.cells) {
    const auto faces=Faces(r.second.cell);
    for(size_t k=0;k<4;++k)to[Bucket(faces[k],parent.size)].push_back({Key(faces[k]),r.first,parent.rank,int(k),-1,Parity(faces[k]),false});
  }
  CollectiveFailure(failure,CURRENT_FUNCTION);auto records=parent.exchange(to);
  std::sort(records.begin(),records.end(),[](const auto& a,const auto& b){return std::tie(a.face,a.label,a.owner,a.cell)<std::tie(b.face,b.label,b.owner,b.cell);});
  std::vector<std::vector<Binding>> reply(parent.size);
  for(size_t first=0;first<records.size();) {
    size_t last=first+1;while(last<records.size() && records[last].face==records[first].face)++last;
    size_t volumes=0,labels=0;int marker=-1,orientation=0;
    for(size_t i=first;i<last;++i)if(records[i].label) { ++labels;marker=records[i].marker; } else { ++volumes;orientation+=records[i].parity; }
    if((volumes==1 && labels==1) || (volumes==2 && labels==0 && orientation==0)) {
      if(labels)for(size_t i=first;i<last;++i)if(!records[i].label)
        reply[records[i].owner].push_back({records[i].cell,records[i].slot,marker});
    } else failure.Set(1,records[first].face[0],"Native3D face/physical-label incidence is not an oriented conforming manifold.");
    first=last;
  }
  CollectiveFailure(failure,CURRENT_FUNCTION);const auto bindings=parent.exchange(reply);
  ReferenceState pending;
  if(!state.original) {
    const auto count=CPassiveComm::Allreduce(uint64_t(bindings.size()),CPassiveComm::Op::SUM,parent.comm);
    if(!count || count>4*MaximumCavityCells)failure.Set(1,0,"Native3D immutable facet authority exceeds the 256-face limit.");
    CollectiveFailure(failure,CURRENT_FUNCTION);std::vector<Facet> local;
    const auto offset=CPassiveComm::ExscanSum(bindings.size());
    for(size_t i=0;i<bindings.size();++i) {
      const auto b=bindings[i];const auto& cell=result.cells.at(b.cell).cell;const auto face=Faces(cell)[b.slot];
      Facet f;f.id=offset+i;f.marker=b.marker;
      for(size_t k=0;k<3;++k)for(auto n:cell.v)if(n.id==face[k])f.v[k]=n;
      local.push_back(f);
    }
    pending.facets=parent.metadata(local);pending.marker_names=names;
    try {
      std::vector<Face> skin;for(auto f:pending.facets)skin.push_back({f.v[0].id,f.v[1].id,f.v[2].id});
      ValidateSurfaceTopology(skin,true);pending.original=std::make_shared<PlanarReference>(pending.facets);
    } catch(const std::exception& e) { failure.Set(1,0,e.what()); }
  }
  const auto& authority=state.original?state:pending;
  if(authority.marker_names!=names)failure.Set(1,0,"Native3D immutable physical marker names changed.");
  CollectiveFailure(failure,CURRENT_FUNCTION);
  try {
    for(auto b:bindings) {
      auto& cell=result.cells.at(b.cell);const auto face=Faces(cell.cell)[b.slot];std::array<Point,3> p{};
      for(size_t k=0;k<3;++k)for(auto n:cell.cell.v)if(n.id==face[k])p[k]=n.p;
      bool found=false;Id chosen=0;
      for(auto f:authority.facets)if(f.marker==b.marker && authority.original->ContainsFace(f.id,p)) {
        const auto id=authority.original->Canonical(f.id);if(!found || id<chosen)chosen=id;found=true;
      }
      if(!found)throw std::invalid_argument("Native3D physical triangle leaves its immutable oriented marker facets.");
      cell.physical[b.slot]={true,chosen};
    }
  } catch(const std::exception& e) { failure.Set(1,0,e.what()); }
  CollectiveFailure(failure,CURRENT_FUNCTION);
  if(!state.original)std::swap(state,pending);
  result.stats.faces_seconds=parent.seconds()-face_start;
  result.stats.maximum_native_bytes=transfer_memory::Add(OwnedBytes(result.cells),transfer_memory::Bytes(result.original),
      transfer_memory::Bytes(bytes),transfer_memory::Bytes(fetched),points.GetMemory(),parent.max_exchange_work_bytes);
  }
  return result;
}
namespace {
/*! Shared collective batch schedule. Only the original sensor is imported; composition remains query-local. */
template<class Make,class Use>
bool Samples(World& parent,size_t count,DonorImport& source,MetricComposition composition,Make make,Use use,
             BridgeStats& stats,std::string& reason,size_t ceiling,size_t live,size_t initial,bool query_timings) {
  // shortcut: conservative bounded field/cache scratch; calibrate against the actual index/cache allocations.
  const size_t scratch=transfer_memory::Add(transfer_memory::Mul(MaximumDonors,2*sizeof(DonorCell)+64*sizeof(double)),
      transfer_memory::Mul(MaximumSensorSamples,32*sizeof(double)),transfer_memory::Mul(MaximumCavityCells,sizeof(DonorRegion)));
  live=transfer_memory::Add(live,scratch);
  if(!Admit(parent,live,ceiling,stats,reason))return false;
  size_t next=0,batch=initial;
  while(parent.maximum(next<count)) {
    const double start=parent.seconds();CLocalFailure failure;std::vector<DonorRegion> regions;
    try { for(size_t i=next;i<std::min(count,next+batch);++i)make(i,regions); }
    catch(const std::exception& e) { failure.Set(2,0,e.what()); }
    if(!Vote(parent,failure,reason)) { stats.query_seconds+=parent.seconds()-start;return false; }
    auto imported=source.Import(regions,ceiling,live);++stats.batches;
    if(!imported.valid) {
      reason=imported.reason;stats.query_seconds+=parent.seconds()-start;
      if(batch==1)return false;
      batch=std::max(size_t(1),batch/2);++stats.batch_retries;continue;
    }
    try {
      if(!regions.empty()) {
        FrozenField field(std::move(imported.cells),composition,query_timings);
        for(size_t i=next;i<std::min(count,next+batch);++i)use(i,field);
        Add(stats,field.Statistics());
      }
    } catch(const std::exception& e) { failure.Set(2,0,e.what()); }
    if(!Vote(parent,failure,reason)) { stats.query_seconds+=parent.seconds()-start;return false; }
    next+=std::min(batch,count-next);stats.query_seconds+=parent.seconds()-start;
  }
  stats.maximum_native_bytes=std::max(stats.maximum_native_bytes,source.Statistics().maximum_work_bytes);
  return true;
}
void PointRegion(Point p,std::vector<DonorRegion>& regions) { regions.push_back({p.x,p.y,p.z,p.x,p.y,p.z}); }
}
bool WorkingWeights(World& parent,const Owned& cells,DonorImport& source,std::vector<uint32_t>& output,
                    std::string& reason,BridgeStats& stats,MetricComposition composition,size_t ceiling,size_t caller_bytes,bool query_timings) {
  Envelope envelope(parent,stats);const size_t live=transfer_memory::Add(caller_bytes,OwnedBytes(cells),source.ResidentBytes(),
      transfer_memory::Mul(cells.size(),sizeof(const Cell*)+sizeof(uint32_t)));
  if(!Admit(parent,live,ceiling,stats,reason))return false;
  std::vector<const Cell*> ordered;std::vector<uint32_t> weights;CLocalFailure failure;
  try { weights.resize(cells.size());ordered.reserve(cells.size());for(auto& r:cells)ordered.push_back(&r.second.cell); }
  catch(const std::exception& e) { failure.Set(2,0,e.what()); }
  if(!Vote(parent,failure,reason))return false;
  auto make=[&](size_t i,std::vector<DonorRegion>& regions) { const auto& cell=*ordered[i];PointRegion(Centroid(cell),regions);for(auto n:cell.v)PointRegion(n.p,regions); };
  auto use=[&](size_t i,FrozenField& field) { weights[i]=WorkWeights({*ordered[i]},field).front(); };
  if(!Samples(parent,cells.size(),source,std::move(composition),make,use,stats,reason,ceiling,live,8,query_timings))return false;
  output.swap(weights);reason.clear();return true;
}
bool ReaderSlices(World& parent,const Owned& cells,const std::vector<DonorCell>& original,const ReferenceState& state,
                  CReaderSlices& output,std::string& reason,BridgeStats& stats,MetricComposition composition,
                  size_t ceiling,size_t caller_bytes,bool query_timings) {
  Envelope envelope(parent,stats);CLocalFailure failure;
#ifdef HAVE_MPI
  int relation=MPI_UNEQUAL;MPI_Comm_compare(parent.comm,SU2_MPI::GetComm(),&relation);
  if(relation!=MPI_IDENT)failure.Set(1,0,"Native3D reader return requires the actual CFD communicator.");
#endif
  if(!state.original || state.marker_names.empty())failure.Set(1,0,"Uninitialized native3D reader reference.");
  if(!Vote(parent,failure,reason))return false;
  const size_t live=transfer_memory::Add(caller_bytes,OwnedBytes(cells),transfer_memory::Bytes(original),
      transfer_memory::Mul(cells.size(),4*(sizeof(Node)+sizeof(void*)*5+9*sizeof(double)+sizeof(unsigned long))),
      transfer_memory::Mul(state.marker_names.size(),sizeof(CSimplexMesh::Marker)));
  if(!Admit(parent,live,ceiling,stats,reason))return false;
  DonorImport source(parent,query_timings);const double build=parent.seconds();
  if(!source.Build(original,reason,ceiling,live)) { stats.source_seconds=parent.seconds()-build;return false; }
  stats.source_seconds=parent.seconds()-build;std::map<Id,Node> nodes;std::vector<Node> ordered;
  CSimplexMesh mesh;mesh.nDim=3;std::vector<uint64_t> keys;std::map<Id,unsigned long> index;
  try {
    for(const auto& r:cells)for(auto n:r.second.cell.v) {
      const auto old=nodes.emplace(n.id,n);
      if(!old.second && (old.first->second.p.x!=n.p.x || old.first->second.p.y!=n.p.y || old.first->second.p.z!=n.p.z))
        throw std::invalid_argument("Inconsistent native3D output coordinates.");
    }
    for(auto r:nodes) { index.emplace(r.first,ordered.size());ordered.push_back(r.second);keys.push_back(r.first);
      mesh.coord.insert(mesh.coord.end(),{r.second.p.x,r.second.p.y,r.second.p.z}); }
    mesh.metric.resize(6*ordered.size());
    for(size_t m=0;m<state.marker_names.size();++m)mesh.markers.push_back({state.marker_names[m],int(m+1),{}});
    for(const auto& r:cells) {
      const auto& c=r.second.cell;for(auto n:c.v)mesh.elem.push_back(index.at(n.id));mesh.elemRef.push_back(1);
      const auto faces=Faces(c);
      for(size_t k=0;k<4;++k)if(r.second.physical[k].present) {
        const auto marker=state.original->Get(r.second.physical[k].facet).marker;
        for(auto id:faces[k])mesh.markers.at(marker).elem.push_back(index.at(id));
      }
    }
  } catch(const std::exception& e) { failure.Set(2,0,e.what()); }
  if(!Vote(parent,failure,reason))return false;
  auto make=[&](size_t i,std::vector<DonorRegion>& regions){PointRegion(ordered[i].p,regions);};
  auto use=[&](size_t i,FrozenField& field){const auto m=field.Query(ordered[i].p).target;const std::array<double,6> values{m.xx,m.xy,m.xz,m.yy,m.yz,m.zz};std::copy(values.begin(),values.end(),mesh.metric.begin()+6*i);};
  if(!Samples(parent,ordered.size(),source,std::move(composition),make,use,stats,reason,ceiling,
      transfer_memory::Add(live,source.ResidentBytes()),16,query_timings))return false;
  const double reader_start=parent.seconds();auto pending=CReaderSlices::FromDistributed(mesh,keys);
  static_assert(std::is_nothrow_move_assignable<CReaderSlices>::value,"Reader publication must not allocate.");
  output=std::move(pending);stats.reader_seconds=parent.seconds()-reader_start;reason.clear();return true;
}
}  // namespace SU2Native3D
