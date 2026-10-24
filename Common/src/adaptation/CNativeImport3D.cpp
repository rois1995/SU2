/*!
 * \file CNativeImport3D.cpp
 * \brief CFD-owned tetrahedral import and original-P1 distributed reader return.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeImport3D.hpp"
#include "../../include/adaptation/CNativeCellSampling3D.hpp"
#include "../../include/adaptation/CDistributedSearch.hpp"
#include "../../include/adaptation/CMeshGather.hpp"
#include "../../include/CConfig.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include <cmath>
#include <type_traits>
#include <tuple>
#include <limits>
#include <fstream>
#include <iomanip>
#include <locale>
#include <numeric>
#include <climits>
#include <sstream>
namespace SU2Native3D {
size_t OwnedStorageBytes(const Owned& cells) {
  return transfer_memory::Mul(cells.size(), sizeof(Owned::value_type) + 4 * sizeof(void*));
}
size_t ReferenceStorageBytes(const ReferenceState& reference, bool authority) {
  return transfer_memory::Add(transfer_memory::Bytes(reference.facets), transfer_memory::Bytes(reference.marker_names),
                              authority && reference.original ? reference.original->ResidentBytes() : 0);
}
namespace {
std::array<Face, 4> Faces(const Cell& c) {
  const auto& v = c.v;
  return {{{v[1].id, v[2].id, v[3].id},
           {v[0].id, v[3].id, v[2].id},
           {v[0].id, v[1].id, v[3].id},
           {v[0].id, v[2].id, v[1].id}}};
}
Face Key(Face f) {
  std::sort(f.begin(), f.end());
  return f;
}
int Bucket(Face f, int ranks) {
  uint64_t hash = 14695981039346656037ULL;
  for (auto id : Key(f)) {
    hash ^= id;
    hash *= 1099511628211ULL;
  }
  return hash % ranks;
}
int Parity(Face f) {
  unsigned count = 0;
  for (size_t i = 0; i < 3; ++i)
    for (size_t j = i + 1; j < 3; ++j) count += f[i] > f[j];
  return count % 2 ? -1 : 1;
}
size_t OwnedBytes(const Owned& cells) { return OwnedStorageBytes(cells); }
struct Envelope {
  World& world;
  BridgeStats& stats;
  double start, collective_start;
  uint64_t bytes_start, calls_start;
  Envelope(World& world, BridgeStats& stats)
      : world(world), stats(stats), start(world.seconds()), collective_start(world.collective_seconds),
        bytes_start(world.bytes_sent), calls_start(world.collective_calls) { stats = {}; }
  ~Envelope() {
    stats.tracked_bytes = world.bytes_sent - bytes_start;
    stats.tracked_calls = world.collective_calls - calls_start;
    stats.tracked_collective_seconds = world.collective_seconds - collective_start;
    stats.total_seconds = world.seconds() - start;
    stats.other_seconds =
        std::max(0., stats.total_seconds - stats.points_seconds - stats.cells_seconds - stats.faces_seconds -
                         stats.source_seconds - stats.query_seconds - stats.reader_seconds - stats.reduction_seconds);
  }
};
bool Vote(World& world, const CLocalFailure& failure, std::string& reason) {
  const auto elected = world.elect(failure);
  reason = elected.message;
  return !elected.any;
}
bool Admit(World& world, size_t bytes, size_t ceiling, BridgeStats& stats, std::string& reason) {
  stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, bytes);
  CLocalFailure failure;
  // DC-16: the message names the requested and admitted bytes (diagnosis of fixed ceilings)
  if (bytes == SIZE_MAX || bytes > ceiling) failure.Set(3, 0, "Native 3D bridge storage budget exceeded: requested " +
      (bytes == SIZE_MAX ? std::string("overflow") : std::to_string(bytes >> 20) + " MiB") + " > ceiling " + std::to_string(ceiling >> 20) + " MiB.");
  return Vote(world, failure, reason);
}
constexpr size_t SymmetryFileBytes=65536;
struct SymmetryPlane {
  std::string marker;
  unsigned axis=3; // Versions1 and2 have distinct adoption/rounding contracts.
  double coordinate=0,input_error=0;
  SU2NativeGeometry::AffineAuthority authority;
  bool Legacy()const{return axis<3;}
};
bool HasSymmetryReference(const CConfig& config) {
  const auto& path=config.GetAdap_Native_Symmetry_Reference();return !path.empty() && path!="NONE";
}
auto DefinitionKey(const SU2NativeGeometry::AffineAuthority& g) {
  return std::make_tuple(g.identity,g.version,g.dimension,g.origin.x,g.origin.y,g.origin.z,
      g.normal.x,g.normal.y,g.normal.z,g.maximum_error);
}
// One declaration decoder serves initial adoption, original-source freezing and persisted-reference checks.
bool ReadSymmetryPlanes(World& world,const CConfig& config,std::vector<SymmetryPlane>& output,std::string& reason) {
  if(!HasSymmetryReference(config)){output.clear();return true;}
  CLocalFailure failure;std::vector<char> bytes;
  if(!world.rank)try {
    std::ifstream in(config.GetAdap_Native_Symmetry_Reference(),std::ios::binary|std::ios::ate);const auto size=in.tellg();
    if(!in || size<=0 || size>std::streamoff(SymmetryFileBytes))throw std::invalid_argument("Missing/oversized native symmetry reference file.");
    bytes.resize(size);in.seekg(0);
    if(!in.read(bytes.data(),bytes.size()))throw std::invalid_argument("Cannot read native symmetry reference file.");
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!Vote(world,failure,reason))return false;
  CPassiveComm::BcastRounds(bytes,0,world.comm);std::vector<SymmetryPlane> pending;
  try {
    std::istringstream in(std::string(bytes.begin(),bytes.end()));in.imbue(std::locale::classic());
    std::string magic;unsigned version=0,count=0;std::set<std::string> legacy_markers;
    std::set<std::pair<std::string,Id>> entries;std::map<Id,SU2NativeGeometry::AffineAuthority> identities;
    if(!(in>>magic>>version>>count) || magic!="SU2_NATIVE_SYMMETRY_PLANES" || (version!=1 && version!=2) || !count || count>64)
      throw std::invalid_argument("Invalid native symmetry reference header/count/version.");
    for(unsigned i=0;i<count;++i) {
      SymmetryPlane plane;
      if(!(in>>std::quoted(plane.marker)))throw std::invalid_argument("Missing native plane marker.");
      if(version==1) {
        std::string axis;
        if(!(in>>axis>>plane.coordinate>>plane.input_error) || !std::isfinite(plane.coordinate) ||
            !(std::isfinite(plane.input_error)&&plane.input_error>=0) || !legacy_markers.insert(plane.marker).second ||
            (axis!="X" && axis!="Y" && axis!="Z"))throw std::invalid_argument("Invalid/duplicate native Cartesian symmetry plane.");
        plane.axis=axis=="X"?0:axis=="Y"?1:2;
      } else {
        auto natural=[&]() {
          std::string token;if(!(in>>token) || token.empty() || token.find_first_not_of("0123456789")!=std::string::npos)
            throw std::invalid_argument("Affine identity/version must be unsigned decimal integers.");
          try {size_t used=0;const auto value=std::stoull(token,&used);if(used==token.size())return uint64_t(value);}
          catch(const std::exception&){}
          throw std::invalid_argument("Unrepresentable affine identity/version.");
        };
        auto& g=plane.authority;g.identity=natural();g.version=natural();
        if(!(in>>g.origin.x>>g.origin.y>>g.origin.z>>g.normal.x>>g.normal.y>>g.normal.z
             >>plane.input_error>>g.maximum_error) || g.identity==UINT64_MAX ||
             !(std::isfinite(plane.input_error)&&plane.input_error>=g.maximum_error) ||
             !entries.emplace(plane.marker,g.identity).second)throw std::invalid_argument("Invalid/duplicate native affine symmetry declaration.");
        SU2NativeGeometry::AffineChart checked(g);
        const auto old=identities.emplace(g.identity,g);
        if(!old.second && DefinitionKey(old.first->second)!=DefinitionKey(g))
          throw std::invalid_argument("Conflicting native affine authority definitions.");
      }
      bool declared=false;
      for(unsigned m=0;m<config.GetnMarker_CfgFile();++m)if(config.GetMarker_CfgFile_TagBound(m)==plane.marker)
        declared=config.GetMarker_CfgFile_KindBC(plane.marker)==SYMMETRY_PLANE;
      if(!declared)throw std::invalid_argument("Native plane marker is not a declared MARKER_SYM: "+plane.marker);
      pending.push_back(std::move(plane));
    }
    in>>std::ws;if(!in.eof())throw std::invalid_argument("Trailing native symmetry reference data.");
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!Vote(world,failure,reason))return false;
  output=std::move(pending);return true;
}
std::map<Id,SU2NativeGeometry::AffineAuthority> DeclaredSymmetryFacets(const std::vector<Facet>& facets,
    const std::vector<std::string>& markers,const std::vector<SymmetryPlane>& planes) {
  std::map<Id,SU2NativeGeometry::AffineAuthority> result;std::set<size_t> used;
  if(std::none_of(planes.begin(),planes.end(),[](const auto& p){return !p.Legacy();}))return result;
  for(const auto& f:facets) {
    bool declared=false;const SymmetryPlane* selected=nullptr;
    for(size_t i=0;i<planes.size();++i) {
      const auto& p=planes[i];if(p.Legacy() || p.marker!=markers.at(f.marker))continue;declared=true;
      const SU2NativeGeometry::AffineChart chart(p.authority);
      if(std::all_of(f.v.begin(),f.v.end(),[&](Node n){return chart.ErrorBound(n.p)<=p.authority.maximum_error;})) {
        if(selected)throw std::invalid_argument("Ambiguous finite affine symmetry facet "+std::to_string(f.id));
        selected=&p;used.insert(i);
      }
    }
    if(declared && !selected)throw std::invalid_argument("Finite symmetry facet leaves its declared mapping contract: "+std::to_string(f.id));
    if(selected)result.emplace(f.id,selected->authority);
  }
  for(size_t i=0;i<planes.size();++i)if(!planes[i].Legacy() && !used.count(i))
    throw std::invalid_argument("Declared affine symmetry component has no finite physical facets.");
  return result;
}
void CheckDeclaredSymmetry(const ReferenceState& reference,const std::vector<SymmetryPlane>& planes) {
  if(std::none_of(planes.begin(),planes.end(),[](const auto& p){return !p.Legacy();}))return;
  const auto expected=DeclaredSymmetryFacets(reference.facets,reference.marker_names,planes);
  for(const auto& row:expected) {
    const auto found=reference.original->Affine().find(row.first);
    if(found==reference.original->Affine().end() || DefinitionKey(found->second)!=DefinitionKey(row.second))
      throw std::invalid_argument("Frozen/persisted native affine definition differs from its configuration declaration.");
  }
  for(const auto& row:reference.original->Affine()) {
    const auto& marker=reference.marker_names.at(reference.original->Get(row.first).marker);
    if(std::any_of(planes.begin(),planes.end(),[&](const auto& p){return !p.Legacy() && p.marker==marker;}) && !expected.count(row.first))
      throw std::invalid_argument("Configured affine symmetry omits a frozen finite component.");
  }
}

struct ProjectionWitness {Cell cell;bool changed=false;template<class S>void Fields(S& s){s(cell,changed);}};
// Unchanged connectivity, positive relative cell Jacobians and an embedded boundary certify the initial map.
// Boundary-adjacent tetrahedra are witnesses: their count is at most the number of physical boundary faces.
bool CheckProjectedBoundary(World& world,const CConfig& config,const CGeometry& geometry,
    const std::vector<std::array<double,3>>& candidate,
    SymmetryReferenceAudit3D& report,size_t base,size_t ceiling,std::string& reason) {
  const auto started=world.seconds();CLocalFailure failure;BridgeStats memory;
  auto admit=[&](size_t bytes){const bool ok=Admit(world,bytes,ceiling,memory,reason);
    report.maximum_bytes=std::max(report.maximum_bytes,memory.maximum_native_bytes);return ok;};
  auto vote=[&](){report.boundary_check_seconds=CPassiveComm::Allreduce(world.seconds()-started,CPassiveComm::Op::MAX,world.comm);
    return Vote(world,failure,reason);};
  size_t local_faces=0;
  for(unsigned m=0;m<geometry.GetnMarker();++m)if(config.GetMarker_All_KindBC(m)!=SEND_RECEIVE)
    local_faces=transfer_memory::Add(local_faces,geometry.GetnElem_Bound(m));
  const auto face_count=CPassiveComm::Allreduce(uint64_t(local_faces),CPassiveComm::Op::SUM,world.comm);
  const auto mesh_storage=transfer_memory::GrowthBound(geometry.GetnElem(),5*sizeof(unsigned long)+sizeof(CSimplexKey));
  const size_t preparation=transfer_memory::Add(base,mesh_storage,transfer_memory::Mul(face_count,128));
  if(!admit(preparation))return false;
  std::vector<Face> local_skin;local_skin.reserve(local_faces);
  try {for(unsigned m=0;m<geometry.GetnMarker();++m)if(config.GetMarker_All_KindBC(m)!=SEND_RECEIVE)
    for(size_t f=0;f<geometry.GetnElem_Bound(m);++f) {
      const auto* face=geometry.bound[m][f];if(face->GetnNodes()!=3)continue;Face key;
      for(unsigned k=0;k<3;++k)key[k]=geometry.nodes->GetGlobalIndex(face->GetNode(k));
      local_skin.push_back(Key(key));
    }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!vote())return false;
  auto skin=CPassiveComm::Allgatherv(local_skin,nullptr,world.comm);
  std::sort(skin.begin(),skin.end());skin.erase(std::unique(skin.begin(),skin.end()),skin.end());
  const auto simplices=OwnedSimplices(geometry);
  const auto prefix=CPassiveComm::ExscanSum(simplices.size());
  if(!admit(transfer_memory::Add(preparation,transfer_memory::GrowthBound(std::min(simplices.size(),static_cast<unsigned long>(skin.size())),sizeof(ProjectionWitness)))))return false;
  std::vector<ProjectionWitness> local;local.reserve(std::min(simplices.size(),static_cast<unsigned long>(skin.size())));
  try {for(size_t e=0;e<simplices.size();++e) {
    ProjectionWitness witness;witness.cell.id=prefix+e;
    for(unsigned k=0;k<4;++k) {
      const auto p=simplices.nodes[4*e+k];const auto q=candidate.at(p);
      witness.cell.v[k]={geometry.nodes->GetGlobalIndex(p),{q[0],q[1],q[2]}};
      const auto* old=geometry.nodes->GetCoord(p);
      witness.changed=witness.changed || q[0]!=SU2_TYPE::GetValue(old[0]) || q[1]!=SU2_TYPE::GetValue(old[1]) || q[2]!=SU2_TYPE::GetValue(old[2]);
    }
    bool boundary=false;for(auto face:Faces(witness.cell))boundary=boundary || std::binary_search(skin.begin(),skin.end(),Key(face));
    if(!boundary)continue;
    auto& v=witness.cell.v;const auto sign=Orientation(v[0].p,v[1].p,v[2].p,v[3].p);
    if(!sign)throw std::invalid_argument("Declared geometry adoption has a degenerate boundary-adjacent cell.");
    if(sign<0)std::swap(v[0],v[1]);local.push_back(witness);
  }}catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!vote())return false;
  const auto count=CPassiveComm::Allreduce(uint64_t(local.size()),CPassiveComm::Op::SUM,world.comm);
  if(!count || count>skin.size())failure.Set(1,0,"Invalid physical-boundary witness ownership/count.");
  if(!vote())return false;
  // Raw transport, shared-node audit, replicated boundary boxes/tree and query vectors; no volume payload gather.
  if(!admit(transfer_memory::Add(preparation,transfer_memory::Mul(count,3*sizeof(ProjectionWitness)+1024))))return false;
  auto witnesses=CPassiveComm::Allgatherv(local,nullptr,world.comm);report.boundary_witnesses=count;
  try {
    std::map<Face,Face> oriented;
    for(const auto& w:witnesses)for(auto face:Faces(w.cell))if(std::binary_search(skin.begin(),skin.end(),Key(face)))
      if(!oriented.emplace(Key(face),face).second)throw std::invalid_argument("Physical boundary face has multiple volume witnesses.");
    if(oriented.size()!=skin.size())throw std::invalid_argument("Physical boundary is missing a volume witness.");
    std::vector<Face> closed;closed.reserve(oriented.size());for(const auto& row:oriented)closed.push_back(row.second);
    ValidateDomainSurfaceTopology(closed);
    std::map<Id,Point> shared;
    for(const auto& w:witnesses)for(auto n:w.cell.v) {
      const auto old=shared.emplace(n.id,n.p);
      if(!old.second && (old.first->second.x!=n.p.x || old.first->second.y!=n.p.y || old.first->second.z!=n.p.z))
        throw std::invalid_argument("Projected boundary has inconsistent shared coordinates across ranks.");
    }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!vote())return false;
  size_t tree_resident=0,tree_peak=0;
  if(count>std::numeric_limits<unsigned long>::max()/2)failure.Set(3,0,"Boundary witness index exceeds addressable storage.");
  if(!vote())return false;
  CADTElemClass::PredictBytes(3,2*count,count,2*count,&tree_resident,&tree_peak);
  if(!admit(transfer_memory::Add(preparation,transfer_memory::Mul(count,3*sizeof(ProjectionWitness)+128),tree_peak)))return false;
  std::unique_ptr<CADTElemClass> tree;
  try {
    std::vector<su2double> coordinates;std::vector<unsigned long> connectivity,ids;
    std::vector<unsigned short> types(count,LINE),markers(count,0);
    coordinates.reserve(6*count);connectivity.reserve(2*count);ids.reserve(count);
    for(size_t i=0;i<witnesses.size();++i) {
      for(unsigned corner=0;corner<2;++corner)for(unsigned axis=0;axis<3;++axis) {
        double value=corner?-INFINITY:INFINITY;
        for(auto n:witnesses[i].cell.v){const double q=axis==0?n.p.x:axis==1?n.p.y:n.p.z;value=corner?std::max(value,q):std::min(value,q);}
        coordinates.push_back(value);
      }
      connectivity.push_back(2*i);connectivity.push_back(2*i+1);ids.push_back(i);
    }
    // A local box-surrogate tree: boundary witnesses were already exchanged; no second geometry collective.
    tree=std::make_unique<CADTElemClass>(3,coordinates,connectivity,types,markers,ids,false);
  }catch(const std::exception& e){failure.Set(3,0,e.what());}
  if(!vote())return false;
  if(!admit(transfer_memory::Add(preparation,transfer_memory::Mul(count,3*sizeof(ProjectionWitness)+16),
      tree->GetAllocatedBytes(),tree->RetainedWorkspaceBound(1))))return false;
  constexpr uint64_t maximum_pairs=1000000;
  const auto seconds=SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time());
  auto within=[&](){return !(seconds>0) || world.seconds()-started<seconds;};
  auto candidates=[&](auto use) {
    std::vector<unsigned long> overlap;
    for(size_t i=world.rank;i<witnesses.size();i+=world.size)if(witnesses[i].changed) {
      if(!within()) {failure.Set(3,0,"Native affine boundary audit exhausted its elapsed resource budget.");break;}
      std::array<su2double,3> lo,hi;
      for(unsigned axis=0;axis<3;++axis) {
        double first=INFINITY,last=-INFINITY;
        for(auto n:witnesses[i].cell.v){const double q=axis==0?n.p.x:axis==1?n.p.y:n.p.z;first=std::min(first,q);last=std::max(last,q);}
        lo[axis]=first;hi[axis]=last;
      }
      tree->DetermineIntersectingElements(lo.data(),hi.data(),overlap);
      for(auto j:overlap)if(i!=j && (!witnesses[j].changed || i<j))if(!use(i,j))return;
    }
  };
  uint64_t pairs=0;
  candidates([&](size_t,size_t){if(++pairs>maximum_pairs){failure.Set(3,0,"Native affine boundary audit exceeds its pair-work ceiling.");return false;}return true;});
  const auto global_pairs=CPassiveComm::Allreduce(pairs,CPassiveComm::Op::SUM,world.comm);
  if(global_pairs>maximum_pairs)failure.Set(3,0,"Native affine boundary audit exceeds its global pair-work ceiling.");
  if(!vote())return false;
  uint64_t checked=0;
  try {candidates([&](size_t i,size_t j) {
    if((checked%128)==0 && !within()){failure.Set(3,0,"Native affine boundary audit exhausted its elapsed resource budget.");return false;}
    ++checked;
    if(!ConformingDisjoint(witnesses[i].cell,witnesses[j].cell)) {
      failure.Set(1,witnesses[i].cell.id,"Projected geometry has nonconforming boundary-cell contact: "+
          std::to_string(witnesses[i].cell.id)+","+std::to_string(witnesses[j].cell.id));return false;
    }
    return true;
  });}catch(const std::exception& e){failure.Set(1,0,e.what());}
  report.boundary_pair_checks=CPassiveComm::Allreduce(checked,CPassiveComm::Op::SUM,world.comm);
  return vote();
}

struct FaceRecord {
  Face face{};
  Id cell = 0;
  int owner = 0, slot = 0, marker = -1, parity = 0;
  bool label = false;
  template <class S>
  void Fields(S& s) {
    s(face, cell, owner, slot, marker, parity, label);
  }
};
struct Binding {
  Id cell = 0;
  int slot = 0, marker = 0;
  template <class S>
  void Fields(S& s) {
    s(cell, slot, marker);
  }
};

/*! One closed current-source shell, not independent projections or a new physical authority.
 * Accepted original embedding is a precondition. Positive product cells plus conforming shell/old-boundary
 * contacts certify the augmented embedding; old/outer separation supplies a complete numerical query band.
 * Analytic physical envelopes stay on their qualified paths until joint coverage is independently admitted. */
bool JointAffineCoverage(World& world,const Owned& owned,std::vector<DonorCell>& source,
    const PlanarReference& reference,double maximum_seconds,size_t live,size_t ceiling,
    BridgeStats& stats,std::string& reason) {
  const auto started=world.seconds();
  struct Receipt {World& world;BridgeStats& stats;double start;
    ~Receipt(){stats.source_envelope_seconds+=world.seconds()-start;}} receipt{world,stats,started};
  uint64_t maximum_work=1000000;  // DC-16: scaled below with the skin size (a published FIRST_ROW event has more skin vertices)
  CLocalFailure failure;
  auto vote=[&](){return Vote(world,failure,reason);};
  auto within=[&](){return !(maximum_seconds>0) || world.seconds()-started<maximum_seconds;};
  double radius=0;Id component=UINT64_MAX,maximum_cell=0;
  try {
    if(!reference.Analytic().empty())throw std::invalid_argument("Unsupported joint affine/analytic original-source envelope; accepted state unchanged.");
    for(const auto& row:reference.Affine()) {
      radius=std::max(radius,4*row.second.maximum_error);
      component=std::min(component,reference.Canonical(row.first));
    }
    if(reference.Mapped()) {
      for(const auto& row:reference.Parameters()->Geometry().Errors())radius=std::max(radius,4*row.second);
      component=reference.ExtensionComponents().front();
    }
    if(!(std::isfinite(radius)&&radius>0) || component==UINT64_MAX)
      throw std::invalid_argument("Unrepresentable joint affine source envelope.");
    for(const auto& d:source) {
      if(d.extension_component!=UINT64_MAX)throw std::invalid_argument("Joint affine source already has an exterior envelope.");
      maximum_cell=std::max(maximum_cell,d.cell.id);
    }
  }catch(const std::bad_alloc& e){failure.Set(3,0,e.what());}
   catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!vote())return false;
  uint64_t local_faces=0,local_boundary=0;
  for(const auto& row:owned) {
    bool boundary=false;for(auto f:row.second.physical)if(f.present){++local_faces;boundary=true;}
    local_boundary+=boundary;
  }
  const auto count=CPassiveComm::Allreduce(local_faces,CPassiveComm::Op::SUM,world.comm);
  const auto witnesses=CPassiveComm::Allreduce(local_boundary,CPassiveComm::Op::SUM,world.comm);
  maximum_cell=CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm);
  // DC-16: the column normal-cone solves are bounded per skin face (128 trials on average, a vertex with eight distinct
  // incident normals needs 92), not by a fixed total: the published full-M6 x32 mesh exceeded 10^6 with 49.5k faces
  maximum_work=std::max<uint64_t>(maximum_work,128*count);
  // Maps, full product geometry, replicated boundary witness transport, planned checks and tree buffers.
  const auto base=transfer_memory::Add(live,transfer_memory::Mul(count,8192),
      transfer_memory::Mul(witnesses,3*sizeof(OwnedCell)),transfer_memory::Mul(2*maximum_work,3*sizeof(size_t)),
      transfer_memory::GrowthBound(transfer_memory::Add(source.size(),3*local_faces),sizeof(DonorCell)));
  if(!count || !witnesses || witnesses>count || count>ULONG_MAX/24 ||
      transfer_memory::Add(maximum_cell,1,transfer_memory::Mul(12,transfer_memory::Add(maximum_cell,1)))==SIZE_MAX)
    failure.Set(3,0,"Joint affine source topology/identity resource limit exceeded.");
  if(!vote() || !Admit(world,base,ceiling,stats,reason))return false;
  std::vector<OwnedCell> boundary;
  try {boundary.reserve(local_boundary);for(const auto& row:owned)
    if(std::any_of(row.second.physical.begin(),row.second.physical.end(),[](auto f){return f.present;}))boundary.push_back(row.second);
  }catch(const std::exception& e){failure.Set(3,0,e.what());}
  if(!vote())return false;
  auto payload_started=world.seconds();auto all=CPassiveComm::Allgatherv(boundary,nullptr,world.comm);
  stats.source_envelope_payload_seconds+=world.seconds()-payload_started;
  stats.source_envelope_payload_bytes+=all.size()*sizeof(OwnedCell);
  std::vector<Facet> skin;std::vector<std::pair<Id,unsigned>> parents;
  std::map<std::pair<Id,unsigned>,size_t> slots;std::map<Id,Point> original;
  std::map<Id,std::vector<size_t>> incident;
  try {
    std::sort(all.begin(),all.end(),[](const auto& a,const auto& b){return a.cell.id<b.cell.id;});
    for(size_t i=0;i<all.size();++i) {
      const auto& row=all[i];if(i && all[i-1].cell.id==row.cell.id)throw std::invalid_argument("Duplicate joint-source boundary witness ownership.");
      for(auto node:row.cell.v) {
        const auto old=original.emplace(node.id,node.p);
        if(!old.second && (old.first->second.x!=node.p.x || old.first->second.y!=node.p.y || old.first->second.z!=node.p.z))
          throw std::invalid_argument("Inconsistent shared original-source coordinates.");
      }
      const auto faces=Faces(row.cell);
      for(unsigned slot=0;slot<4;++slot)if(row.physical[slot].present) {
        Facet f;f.id=skin.size();
        for(unsigned k=0;k<3;++k)for(auto node:row.cell.v)if(node.id==faces[slot][k])f.v[k]=node;
        skin.push_back(f);parents.push_back({row.cell.id,slot});
      }
    }
    // Face/node keys, not rank ownership or marker/body assumptions, fix all arithmetic order.
    std::vector<size_t> order(skin.size());std::iota(order.begin(),order.end(),0);
    std::sort(order.begin(),order.end(),[&](size_t a,size_t b){return Key({skin[a].v[0].id,skin[a].v[1].id,skin[a].v[2].id})<
        Key({skin[b].v[0].id,skin[b].v[1].id,skin[b].v[2].id});});
    auto old_skin=skin;auto old_parents=parents;
    for(size_t i=0;i<order.size();++i) {
      skin[i]=old_skin[order[i]];skin[i].id=i;parents[i]=old_parents[order[i]];slots.emplace(parents[i],i);
      const int orientation=Parity({skin[i].v[0].id,skin[i].v[1].id,skin[i].v[2].id});
      std::sort(skin[i].v.begin(),skin[i].v.end(),[](Node a,Node b){return a.id<b.id;});
      if(orientation<0)std::swap(skin[i].v[1],skin[i].v[2]);
    }
    std::vector<Face> oriented;oriented.reserve(count);
    for(size_t i=0;i<skin.size();++i) {
      Face f;for(unsigned k=0;k<3;++k){f[k]=skin[i].v[k].id;incident[f[k]].push_back(i);}oriented.push_back(f);
    }
    if(skin.size()!=count)throw std::invalid_argument("Incomplete joint-source physical skin.");
    ValidateDomainSurfaceTopology(oriented);
  }catch(const std::bad_alloc& e){failure.Set(3,0,e.what());}
   catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!vote())return false;
  const auto span=OriginalSourceNodeSpan(world,source);
  if(transfer_memory::Add(span,span-1)==SIZE_MAX)failure.Set(3,0,"Joint source node identity range exhausted.");
  if(!vote())return false;
  std::vector<Node> columns;uint64_t solves=0;
  if(!world.rank)try {
    columns.reserve(incident.size());
    for(const auto& row:incident) {
      if(!within())throw std::length_error("Joint source column elapsed resource budget exhausted.");
      std::vector<std::array<Point,3>> faces;faces.reserve(row.second.size());
      for(auto i:row.second)faces.push_back({skin[i].v[0].p,skin[i].v[1].p,skin[i].v[2].p});
      columns.push_back({span+row.first,SU2NativeGeometry::SourceColumn(original.at(row.first),faces,radius,solves,maximum_work)});
    }
  }catch(const std::length_error& e){failure.Set(3,0,e.what());}
   catch(const std::bad_alloc& e){failure.Set(3,0,e.what());}
   catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!vote())return false;
  payload_started=world.seconds();CPassiveComm::Bcast(columns,0,world.comm);
  stats.source_envelope_payload_seconds+=world.seconds()-payload_started;
  stats.source_envelope_payload_bytes+=columns.size()*sizeof(Node);
  stats.source_envelope_solves=CPassiveComm::Allreduce(solves,CPassiveComm::Op::SUM,world.comm);
  stats.source_envelope_nodes=columns.size();std::map<Id,Point> outer;
  std::vector<Cell> geometry;std::vector<std::array<Point,3>> outer_faces;
  try {
    for(auto node:columns)if(!outer.emplace(node.id-span,node.p).second)throw std::invalid_argument("Duplicate joint-source column.");
    geometry.reserve(witnesses+3*count);for(const auto& row:all)geometry.push_back(row.cell);
    outer_faces.reserve(count);
    for(const auto& f:skin) {
      std::array<Node,6> v;std::array<Node,3> sorted=f.v;
      std::sort(sorted.begin(),sorted.end(),[](Node a,Node b){return a.id<b.id;});
      for(unsigned k=0;k<3;++k){v[k]=sorted[k];v[k+3]={span+sorted[k].id,outer.at(sorted[k].id)};}
      const std::array<std::array<unsigned,4>,3> tets{{{0,1,2,5},{0,1,4,5},{0,3,4,5}}};
      const auto parent=parents[f.id];
      for(unsigned i=0;i<3;++i) {
        Cell cell;cell.id=maximum_cell+1+12*parent.first+3*parent.second+i;
        for(unsigned j=0;j<4;++j)cell.v[j]=v[tets[i][j]];
        const auto& q=cell.v;const auto sign=Orientation(q[0].p,q[1].p,q[2].p,q[3].p);
        const int expected=Parity({f.v[0].id,f.v[1].id,f.v[2].id})*(i==1?-1:1);
        if(!(sign*expected>0))throw std::invalid_argument("Folded/degenerate joint original-source prism at face "+std::to_string(f.id));
        if(sign<0)std::swap(cell.v[0],cell.v[1]);geometry.push_back(cell);
      }
      outer_faces.push_back({outer.at(f.v[0].id),outer.at(f.v[1].id),outer.at(f.v[2].id)});
    }
  }catch(const std::bad_alloc& e){failure.Set(3,0,e.what());}
   catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!vote())return false;
  stats.source_envelope_cells=3*count;
  size_t resident=0,peak=0;const auto n=geometry.size();
  CADTElemClass::PredictBytes(3,2*n,n,2*n,&resident,&peak);
  if(!Admit(world,transfer_memory::Add(base,peak),ceiling,stats,reason))return false;
  std::unique_ptr<CADTElemClass> tree;
  try {
    std::vector<su2double> coordinates;std::vector<unsigned long> connectivity,ids;
    std::vector<unsigned short> types(n,LINE),markers(n,0);
    coordinates.reserve(6*n);connectivity.reserve(2*n);ids.reserve(n);
    for(size_t i=0;i<n;++i) {
      for(unsigned corner=0;corner<2;++corner)for(unsigned axis=0;axis<3;++axis) {
        double value=corner?-INFINITY:INFINITY;
        for(auto node:geometry[i].v){const double q=axis==0?node.p.x:axis==1?node.p.y:node.p.z;value=corner?std::max(value,q):std::min(value,q);}
        coordinates.push_back(value);
      }
      connectivity.push_back(2*i);connectivity.push_back(2*i+1);ids.push_back(i);
    }
    tree=std::make_unique<CADTElemClass>(3,coordinates,connectivity,types,markers,ids,false);
  }catch(const std::exception& e){failure.Set(3,0,e.what());}
  if(!vote())return false;
  if(!Admit(world,transfer_memory::Add(base,tree->GetAllocatedBytes(),tree->RetainedWorkspaceBound(1)),ceiling,stats,reason))return false;
  struct Check {size_t a,b;bool distance;};std::vector<Check> checks;std::vector<unsigned long> candidates;
  try {
    auto keep=[&](size_t a,size_t b,bool distance) {
      if(checks.size()==maximum_work)throw std::length_error("Joint source pair-work ceiling exceeded.");
      checks.push_back({a,b,distance});
    };
    for(size_t i=world.rank;i<3*count;i+=world.size) {
      if(!within())throw std::length_error("Joint source planning elapsed resource budget exhausted.");
      std::array<su2double,3> lo,hi;
      for(unsigned axis=0;axis<3;++axis) {
        lo[axis]=INFINITY;hi[axis]=-INFINITY;
        for(auto node:geometry[witnesses+i].v){const double q=axis==0?node.p.x:axis==1?node.p.y:node.p.z;lo[axis]=std::min(SU2_TYPE::GetValue(lo[axis]),q);hi[axis]=std::max(SU2_TYPE::GetValue(hi[axis]),q);}
      }
      tree->DetermineIntersectingElements(lo.data(),hi.data(),candidates);
      for(auto j:candidates)if(j<witnesses || witnesses+i<j)keep(witnesses+i,j,false);
    }
    for(size_t i=world.rank;i<count;i+=world.size) {
      if(!within())throw std::length_error("Joint source distance planning elapsed resource budget exhausted.");
      std::array<su2double,3> lo,hi;
      for(unsigned axis=0;axis<3;++axis) {
        double first=INFINITY,last=-INFINITY;
        for(auto p:outer_faces[i]){const double q=axis==0?p.x:axis==1?p.y:p.z;first=std::min(first,q);last=std::max(last,q);}
        lo[axis]=std::nextafter(first-radius,-INFINITY);hi[axis]=std::nextafter(last+radius,INFINITY);
      }
      tree->DetermineIntersectingElements(lo.data(),hi.data(),candidates);
      for(auto j:candidates)if(j<witnesses)for(unsigned slot=0;slot<4;++slot)if(all[j].physical[slot].present)
        keep(i,slots.at({all[j].cell.id,slot}),true);
    }
  }catch(const std::length_error& e){failure.Set(3,0,e.what());}
   catch(const std::bad_alloc& e){failure.Set(3,0,e.what());}
   catch(const std::exception& e){failure.Set(1,0,e.what());}
  const auto work=CPassiveComm::Allreduce(uint64_t(checks.size()),CPassiveComm::Op::SUM,world.comm);
  if(work>maximum_work)failure.Set(3,0,"Joint source global pair-work ceiling exceeded.");
  if(!vote())return false;
  uint64_t contacts=0,distances=0;
  try {for(size_t i=0;i<checks.size();++i) {
    if(i%128==0 && !within())throw std::length_error("Joint source audit elapsed resource budget exhausted.");
    const auto c=checks[i];
    if(c.distance) {
      ++distances;const auto& f=skin[c.b];
      if(!SU2NativeGeometry::SourceFacesSeparated(outer_faces[c.a],{f.v[0].p,f.v[1].p,f.v[2].p},radius))
        throw std::invalid_argument("Uncertified joint original-source envelope coverage at face "+std::to_string(c.a));
    }else {
      ++contacts;if(!ConformingDisjoint(geometry[c.a],geometry[c.b]))
        throw std::invalid_argument("Nonconforming joint original-source contact: "+std::to_string(geometry[c.a].id)+","+std::to_string(geometry[c.b].id));
    }
  }}catch(const std::length_error& e){failure.Set(3,0,e.what());}
    catch(const std::bad_alloc& e){failure.Set(3,0,e.what());}
   catch(const std::exception& e){failure.Set(1,0,e.what());}
  stats.source_envelope_contacts=CPassiveComm::Allreduce(contacts,CPassiveComm::Op::SUM,world.comm);
  stats.source_envelope_distances=CPassiveComm::Allreduce(distances,CPassiveComm::Op::SUM,world.comm);
  if(!vote())return false;
  try {
    const auto old=source.size();source.reserve(transfer_memory::Add(old,3*local_faces));
    for(size_t i=0;i<old;++i) {
      const auto original=source[i];const auto& row=owned.at(original.cell.id);
      for(unsigned slot=0;slot<4;++slot)if(row.physical[slot].present) {
        const auto face=slots.at({original.cell.id,slot});
        for(unsigned j=0;j<3;++j) {
          DonorCell donor;donor.cell=geometry[witnesses+3*face+j];donor.extension_component=component;donor.parent_cell=original.cell.id;
          for(unsigned k=0;k<4;++k) {
            const auto id=donor.cell.v[k].id%span;bool found=false;
            for(unsigned v=0;v<4;++v)if(original.cell.v[v].id==id){donor.sensor[k]=original.sensor[v];found=true;break;}
            if(!found)throw std::invalid_argument("Joint source column lost its original nodal tensor.");
          }
          source.push_back(donor);
        }
      }
    }
  }catch(const std::exception& e){failure.Set(3,0,e.what());}
  if(!vote())return false;
  if(!world.rank)std::cout<<"Native joint affine source: radius="<<radius<<" nodes="<<columns.size()<<" cells="<<3*count
      <<" column_work="<<stats.source_envelope_solves<<" contacts="<<stats.source_envelope_contacts
      <<" distances="<<stats.source_envelope_distances<<" seconds="<<world.seconds()-started<<std::endl;
  return true;
}

/*! Frozen sensor-only prism coverage for one affine authority at a reference vertex.
 * Dependent-coordinate extrusion retains chart coordinates and face P1 traces.
 * Shared distinct/curved authorities need a simultaneous collar, never independent face guesses. */
bool AffineCoverage(World& world,const Owned& cells,std::vector<DonorCell>& source,const PlanarReference& reference,
                    size_t live,size_t ceiling,BridgeStats& stats,std::string& reason,double maximum_seconds) {
  if(reference.Affine().empty())return true;
  CLocalFailure failure;
  size_t active_faces=0;
  for(const auto& row:cells)for(const auto& face:row.second.physical)
    if(face.present && reference.Affine().count(face.facet))active_faces=transfer_memory::Add(active_faces,1);
  const size_t added=transfer_memory::Mul(active_faces,3);
  const size_t capacity=std::max(source.capacity(),transfer_memory::Add(source.size(),added));
  if(!Admit(world,transfer_memory::Add(live,transfer_memory::GrowthBound(capacity,sizeof(DonorCell)),
      transfer_memory::Mul(reference.Affine().size(),2048),transfer_memory::Mul(reference.Analytic().size(),64)),ceiling,stats,reason))return false;
  struct Rule {SU2NativeGeometry::AffineAuthority geometry;unsigned axis=0;int lo=2,hi=-2;};
  std::map<Id,Rule> rules;Id maximum_node=0,maximum_cell=0;bool joint=false;
  try {
    std::map<Id,Id> junctions;
    for(const auto& row:reference.Affine()) {
      const auto& g=row.second;Rule rule{g};const std::array<double,3> n{{g.normal.x,g.normal.y,g.normal.z}};
      for(unsigned k=1;k<3;++k)if(std::abs(n[k])>std::abs(n[rule.axis]))rule.axis=k;
      rules.emplace(reference.Canonical(row.first),rule);
      for(auto node:reference.Get(row.first).v) {
        const auto old=junctions.emplace(node.id,g.identity);
        if(!old.second && old.first->second!=g.identity)
          joint=true;
      }
    }
    for(const auto& row:reference.Analytic())for(auto node:reference.Get(row.first).v)if(junctions.count(node.id) && !joint)
      throw std::invalid_argument("Unsupported simultaneous affine/curved source collar at reference vertex "+std::to_string(node.id)+"; accepted state unchanged.");
    for(const auto& d:source) {
      maximum_cell=std::max(maximum_cell,d.cell.id);for(auto n:d.cell.v)maximum_node=std::max(maximum_node,n.id);
      if(d.extension_component!=UINT64_MAX)continue;
      const auto& owned=cells.at(d.cell.id);
      for(unsigned slot=0;slot<4;++slot)if(owned.physical[slot].present) {
        const auto component=owned.physical[slot].facet;auto rule=rules.find(component);if(rule==rules.end())continue;
        const auto face=Faces(d.cell)[slot];std::array<Point,3> p{};const SU2NativeGeometry::AffineChart chart(rule->second.geometry);
        for(unsigned i=0;i<3;++i)for(auto n:d.cell.v)if(n.id==face[i])p[i]=chart.Coordinates(n.p);
        const auto orientation=Orientation(p[0],p[1],p[2],{0,0,1});int sign=orientation>0?1:orientation<0?-1:0;
        if(rule->second.axis&1)sign=-sign;
        if(!sign)throw std::invalid_argument("Degenerate affine source face.");
        rule->second.lo=std::min(rule->second.lo,sign);rule->second.hi=std::max(rule->second.hi,sign);
      }
    }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!Vote(world,failure,reason))return false;
  const auto joint_low=CPassiveComm::Allreduce(int(joint),CPassiveComm::Op::MIN,world.comm);
  const auto joint_high=CPassiveComm::Allreduce(int(joint),CPassiveComm::Op::MAX,world.comm);
  if(joint_low!=joint_high)failure.Set(1,0,"Joint source authority incidence differs across ranks.");
  if(!Vote(world,failure,reason))return false;
  if(joint)return JointAffineCoverage(world,cells,source,reference,maximum_seconds,live,ceiling,stats,reason);
  const auto count_low=CPassiveComm::Allreduce(uint64_t(rules.size()),CPassiveComm::Op::MIN,world.comm);
  const auto count_high=CPassiveComm::Allreduce(uint64_t(rules.size()),CPassiveComm::Op::MAX,world.comm);
  if(count_low!=count_high)failure.Set(1,0,"Affine source component counts differ across ranks.");
  if(!Vote(world,failure,reason))return false;
  maximum_node=CPassiveComm::Allreduce(maximum_node,CPassiveComm::Op::MAX,world.comm);
  maximum_cell=CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm);
  for(auto& row:rules) {
    auto& rule=row.second;const auto& g=rule.geometry;
    for(uint64_t value:{row.first,g.identity,g.version,uint64_t(g.dimension)})
      if(CPassiveComm::Allreduce(value,CPassiveComm::Op::MIN,world.comm)!=CPassiveComm::Allreduce(value,CPassiveComm::Op::MAX,world.comm))
        failure.Set(1,0,"Affine source identities/versions differ across ranks.");
    for(double value:{g.origin.x,g.origin.y,g.origin.z,g.normal.x,g.normal.y,g.normal.z,g.maximum_error})
      if(CPassiveComm::Allreduce(value,CPassiveComm::Op::MIN,world.comm)!=CPassiveComm::Allreduce(value,CPassiveComm::Op::MAX,world.comm))
        failure.Set(1,0,"Affine source definitions differ across ranks.");
    rule.lo=CPassiveComm::Allreduce(rule.lo,CPassiveComm::Op::MIN,world.comm);
    rule.hi=CPassiveComm::Allreduce(rule.hi,CPassiveComm::Op::MAX,world.comm);
    if(!(rule.lo==2&&rule.hi==-2) && rule.lo!=rule.hi)failure.Set(1,0,"Inconsistent affine source orientation.");
  }
  // Metric-family replay decodes every generated node modulo the ORIGINAL physical-source span.
  const auto span=OriginalSourceNodeSpan(world,source);
  const auto first_offset=transfer_memory::Mul(transfer_memory::Add(maximum_node/span,1),span);
  if(first_offset==SIZE_MAX || transfer_memory::Add(first_offset,transfer_memory::Mul(span,rules.size()))==SIZE_MAX ||
      transfer_memory::Add(maximum_cell,1,transfer_memory::Mul(12,transfer_memory::Add(maximum_cell,1)))==SIZE_MAX)
    failure.Set(1,0,"Affine source identity range exhausted.");
  if(!Vote(world,failure,reason))return false;
  const auto count=source.size();
  try {
    source.reserve(transfer_memory::Add(count,added));
    for(size_t at=0;at<count;++at) {
      const auto original=source[at];if(original.extension_component!=UINT64_MAX)continue;
      const auto& owned=cells.at(original.cell.id);
      for(unsigned slot=0;slot<4;++slot)if(owned.physical[slot].present) {
        const Id component=owned.physical[slot].facet;const auto found=rules.find(component);if(found==rules.end())continue;
        const auto& rule=found->second;const auto offset=first_offset+span*std::distance(rules.begin(),found);
        const auto face=Faces(original.cell)[slot];std::array<std::pair<Node,Tensor>,3> old;
        for(unsigned i=0;i<3;++i)for(unsigned j=0;j<4;++j)if(original.cell.v[j].id==face[i])old[i]={original.cell.v[j],original.sensor[j]};
        std::sort(old.begin(),old.end(),[](const auto& a,const auto& b){return a.first.id<b.first.id;});auto next=old;
        for(auto& vertex:next) {
          auto& n=vertex.first;n.id+=offset;double& coordinate=rule.axis==0?n.p.x:rule.axis==1?n.p.y:n.p.z;
          // The largest absolute normal component is the dropped coordinate: two admitted surfaces differ by at most2*error vertically.
          const auto distance=rule.lo*4*static_cast<long double>(rule.geometry.maximum_error);
          coordinate=std::nextafter(double(static_cast<long double>(coordinate)+distance),rule.lo>0?INFINITY:-INFINITY);
          if(!std::isfinite(coordinate))throw std::invalid_argument("Unrepresentable affine source envelope.");
        }
        const std::array<std::array<std::pair<Node,Tensor>,4>,3> tets{{
            {old[0],old[1],old[2],next[2]},{old[0],old[1],next[1],next[2]},{old[0],next[0],next[1],next[2]}}};
        int orientation=0;
        for(unsigned i=0;i<3;++i) {
          auto v=tets[i];const auto sign=Orientation(v[0].first.p,v[1].first.p,v[2].first.p,v[3].first.p);
          const int direction=(sign>0?1:sign<0?-1:0)*(i==1?-1:1);
          if(!direction || (orientation && orientation!=direction))throw std::invalid_argument("Degenerate/folded affine source envelope.");
          orientation=direction;if(sign<0)std::swap(v[0],v[1]);
          DonorCell donor;donor.cell.id=maximum_cell+1+12*original.cell.id+3*slot+i;
          donor.extension_component=component;donor.parent_cell=original.cell.id;
          for(unsigned j=0;j<4;++j){donor.cell.v[j]=v[j].first;donor.sensor[j]=v[j].second;}
          source.push_back(donor);
        }
      }
    }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  return Vote(world,failure,reason);
}

using PrimitiveKey=std::tuple<unsigned,double,double,double,double,double,double,double,double,double,double>;
PrimitiveKey Primitive(const AnalyticSurface& g) {
  return {unsigned(g.kind),g.origin.x,g.origin.y,g.origin.z,g.extent.x,g.extent.y,g.extent.z,g.tolerance,g.axis.x,g.axis.y,g.axis.z};
}
/*! Freeze a continuous piecewise-affine exterior sensor target from original boundary nodes.
 * Bump columns copy nodal tensors vertically; spherical frusta copy them radially. Original P1 has priority.
 * Capped cylinders use one shared radial/axial collar map, including disk faces and rim identities.
 * Sorted source IDs choose compatible prism diagonals; component tags still control later association. */
bool Coverage(World& world,const Owned& cells,std::vector<DonorCell>& source,const PlanarReference& reference,
              double geometry_tolerance,size_t live,size_t ceiling,BridgeStats& stats,std::string& reason,double maximum_seconds) {
  if(reference.Analytic().empty() && reference.Affine().empty() && !reference.Mapped())return true;
  // Agree on source policy before any analytic/affine branch or geometry collective.
  // Complete bytes, not a hash, cover identities, versions, definitions and finite-facet assignments.
  const auto declaration_started=world.seconds();CLocalFailure declarations_failed;
  const auto declaration_bound=transfer_memory::Add(128,transfer_memory::Mul(128,
      transfer_memory::Add(reference.Analytic().size(),reference.Affine().size())),reference.Mapped()?reference.Parameters()->SnapshotBytes():0);
  if(!Admit(world,transfer_memory::Add(live,transfer_memory::Mul(4,declaration_bound),
      transfer_memory::TransportBytes(world.size)),ceiling,stats,reason))return false;
  std::vector<char> declarations;
  try {
    RecordStream encode(declarations);uint64_t na=reference.Affine().size(),nc=reference.Analytic().size();encode(na,nc);
    for(auto row:reference.Affine()){auto id=row.first;encode(id,row.second);}
    for(auto row:reference.Analytic()) {
      auto& g=row.second;auto kind=unsigned(g.kind);auto id=row.first;
      encode(id,kind,g.origin,g.extent,g.tolerance,g.axis);
    }
    if(reference.Mapped()){const auto parameters=reference.Parameters()->Snapshot();declarations.insert(declarations.end(),parameters.begin(),parameters.end());}
  }catch(const std::exception& e){declarations_failed.Set(3,0,e.what());}
  if(!Vote(world,declarations_failed,reason))return false;
  auto expected=declarations;const auto payload_started=world.seconds();CPassiveComm::BcastRounds(expected,0,world.comm);
  stats.source_envelope_payload_seconds+=world.seconds()-payload_started;
  stats.source_envelope_payload_bytes+=expected.size();
  if(declarations!=expected)declarations_failed.Set(1,0,"Original-source geometry declarations differ across ranks.");
  const bool consistent=Vote(world,declarations_failed,reason);
  stats.source_envelope_seconds+=world.seconds()-declaration_started;
  if(!consistent)return false;
  if(reference.Mapped())return JointAffineCoverage(world,cells,source,reference,maximum_seconds,live,ceiling,stats,reason);
  if(reference.Analytic().empty())return AffineCoverage(world,cells,source,reference,live,ceiling,stats,reason,maximum_seconds);
  CLocalFailure failure;
  const size_t added=transfer_memory::Mul(cells.size(),12);
  const size_t requested=transfer_memory::Add(live,transfer_memory::GrowthBound(transfer_memory::Add(source.size(),added),sizeof(DonorCell)),
      transfer_memory::Mul(reference.Analytic().size(),1024));
  if(!Admit(world,requested,ceiling,stats,reason))return false;
  struct Rule {AnalyticSurface geometry;int lo=2,hi=-2;double height=std::numeric_limits<double>::infinity(),factor=0,pad=0;
    std::array<int,2> cap_lo{{2,2}},cap_hi{{-2,-2}};};
  std::map<PrimitiveKey,Rule> rules;
  std::map<Id,std::pair<PrimitiveKey,unsigned>> caps;
  Id maximum_node=0,maximum_cell=0;
  try {
    if(!(std::isfinite(geometry_tolerance)&&geometry_tolerance>0))throw std::invalid_argument("Invalid exterior source geometry bound.");
    for(const auto& row:reference.Analytic())if(row.second.kind!=AnalyticSurface::Kind::Disk)rules.emplace(Primitive(row.second),Rule{row.second});
    // Match declared end disks to a unique cylinder; their shared nodes use the same affine collar map.
    for(const auto& disk:reference.Analytic())if(disk.second.kind==AnalyticSurface::Kind::Disk) {
      const auto& d=disk.second;unsigned matches=0;
      for(const auto& row:rules)if(row.second.geometry.kind==AnalyticSurface::Kind::Cylinder) {
        const auto& g=row.second.geometry;
        const long double length=std::hypot(g.axis.x,g.axis.y,g.axis.z),dl=std::hypot(d.axis.x,d.axis.y,d.axis.z);
        const std::array<long double,3> a{{g.axis.x/length,g.axis.y/length,g.axis.z/length}},b{{d.axis.x/dl,d.axis.y/dl,d.axis.z/dl}};
        const auto cross=std::hypot(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]);
        if(cross>64*std::numeric_limits<double>::epsilon() || std::abs(g.extent.x-d.extent.x)>g.tolerance+d.tolerance)continue;
        for(unsigned end=0;end<2;++end) {
          const long double t=end ? g.extent.y : 0;
          const auto distance=std::hypot(static_cast<long double>(d.origin.x)-g.origin.x-t*a[0],
                                        static_cast<long double>(d.origin.y)-g.origin.y-t*a[1],
                                        static_cast<long double>(d.origin.z)-g.origin.z-t*a[2]);
          if(distance<=g.tolerance+d.tolerance){caps[disk.first]={row.first,end};++matches;}
        }
      }
      if(matches>1)throw std::invalid_argument("Ambiguous cylinder/end-disk source collar association.");
    }
    for(const auto& d:source) {
      if(d.extension_component!=UINT64_MAX)throw std::invalid_argument("Exterior source already generated.");
      maximum_cell=std::max(maximum_cell,d.cell.id);for(auto n:d.cell.v)maximum_node=std::max(maximum_node,n.id);
      const auto& owned=cells.at(d.cell.id);
      for(size_t slot=0;slot<4;++slot)if(owned.physical[slot].present) {
        const auto id=owned.physical[slot].facet;
        const auto entry=reference.Analytic().find(id);
        if(entry==reference.Analytic().end())continue;
        const auto cap=caps.find(id);
        if(entry->second.kind==AnalyticSurface::Kind::Disk && cap==caps.end())continue;
        auto& rule=rules.at(cap==caps.end() ? Primitive(entry->second) : cap->second.first);const auto& g=rule.geometry;
        const auto ids=Faces(d.cell)[slot];std::array<Point,3> p{};
        for(size_t i=0;i<3;++i)for(auto n:d.cell.v)if(n.id==ids[i])p[i]=n.p;
        if(cap!=caps.end()) {
          const auto length=std::hypot(g.axis.x,g.axis.y,g.axis.z);
          const Point tip{p[0].x+g.axis.x/length,p[0].y+g.axis.y/length,p[0].z+g.axis.z/length};
          const auto area=Orientation(p[0],p[1],p[2],tip);const int sign=area>0?1:area<0?-1:0;
          if(!sign)throw std::invalid_argument("Degenerate cylinder end-disk source face.");
          const auto end=cap->second.second;rule.cap_lo[end]=std::min(rule.cap_lo[end],sign);rule.cap_hi[end]=std::max(rule.cap_hi[end],sign);
          continue;
        }
        int direction=0;
        if(g.kind==AnalyticSurface::Kind::Bump) {
          const auto area=Orientation({p[0].x,p[0].y,0},{p[1].x,p[1].y,0},{p[2].x,p[2].y,0},{0,0,1});
          direction=area>0?1:area<0?-1:0;
        } else if(g.kind==AnalyticSurface::Kind::Cylinder) {
          const auto orientation=CylinderFacing(g,p);direction=orientation>0?1:orientation<0?-1:0;
          rule.height=std::min(rule.height,CylinderRadialFloor(g,p));
        } else {
          const auto volume=Orientation(g.origin,p[0],p[1],p[2]);direction=volume>0?1:volume<0?-1:0;
          std::array<long double,3> a{},b{},normal{};
          for(size_t k=0;k<3;++k){const auto x=[](Point p,size_t k){return k==0?p.x:k==1?p.y:p.z;};
            a[k]=static_cast<long double>(x(p[1],k))-x(p[0],k);b[k]=static_cast<long double>(x(p[2],k))-x(p[0],k);}
          normal={a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
          const auto length=std::hypot(normal[0],normal[1],normal[2]);
          const auto height=std::abs(volume)/length;
          if(!(height>0&&std::isfinite(height)))throw std::invalid_argument("Unusable radial exterior source face.");
          rule.height=std::min(rule.height,static_cast<double>(height));
        }
        if(!direction)throw std::invalid_argument("Degenerate exterior source chart.");
        rule.lo=std::min(rule.lo,direction);rule.hi=std::max(rule.hi,direction);
      }
    }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!Vote(world,failure,reason))return false;
  const auto count_lo=CPassiveComm::Allreduce(uint64_t(rules.size()),CPassiveComm::Op::MIN,world.comm);
  const auto count_hi=CPassiveComm::Allreduce(uint64_t(rules.size()),CPassiveComm::Op::MAX,world.comm);
  if(count_lo!=count_hi)failure.Set(1,0,"Exterior primitive count differs across ranks.");
  if(!Vote(world,failure,reason))return false;
  maximum_node=CPassiveComm::Allreduce(maximum_node,CPassiveComm::Op::MAX,world.comm);
  maximum_cell=CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm);
  for(auto& row:rules) {
    auto& rule=row.second;const auto& g=rule.geometry;
    for(double value:{double(unsigned(g.kind)),g.origin.x,g.origin.y,g.origin.z,g.extent.x,g.extent.y,g.extent.z,g.tolerance,g.axis.x,g.axis.y,g.axis.z}) {
      const auto low=CPassiveComm::Allreduce(value,CPassiveComm::Op::MIN,world.comm);
      const auto high=CPassiveComm::Allreduce(value,CPassiveComm::Op::MAX,world.comm);
      if(low!=high)failure.Set(1,0,"Exterior primitive parameters differ across ranks.");
    }
    rule.lo=CPassiveComm::Allreduce(rule.lo,CPassiveComm::Op::MIN,world.comm);
    rule.hi=CPassiveComm::Allreduce(rule.hi,CPassiveComm::Op::MAX,world.comm);
    rule.height=CPassiveComm::Allreduce(rule.height,CPassiveComm::Op::MIN,world.comm);
    for(unsigned end=0;end<2;++end) {
      rule.cap_lo[end]=CPassiveComm::Allreduce(rule.cap_lo[end],CPassiveComm::Op::MIN,world.comm);
      rule.cap_hi[end]=CPassiveComm::Allreduce(rule.cap_hi[end],CPassiveComm::Op::MAX,world.comm);
    }
    if(rule.lo==2&&rule.hi==-2)continue; // Authority may include a chart absent from this accepted surface.
    if(rule.lo!=rule.hi)failure.Set(1,0,"Inconsistent exterior source orientation for one analytic primitive.");
    if(g.kind==AnalyticSurface::Kind::Cylinder && rule.cap_lo[0]!=2 && rule.cap_lo[1]!=2) {
      for(unsigned end=0;end<2;++end)if(rule.cap_lo[end]!=rule.cap_hi[end] || rule.cap_lo[end]!=(end?rule.lo:-rule.lo))
        failure.Set(1,0,"Cylinder/end-disk source collar orientations disagree.");
      const auto scale=std::max({1.,std::abs(g.origin.x),std::abs(g.origin.y),std::abs(g.origin.z),g.extent.x,g.extent.y});
      const auto pad=std::max(4*g.tolerance,128*std::numeric_limits<double>::epsilon()*scale);
      if(!(pad>0 && pad<g.extent.y/4))failure.Set(1,0,"Cylinder end-disk source envelope is unrepresentable.");
      rule.pad=rule.lo*pad;
    }
    if(g.kind==AnalyticSurface::Kind::Bump)
      rule.factor=rule.lo*std::nextafter(std::abs(g.extent.z)/16+2*g.tolerance,INFINITY);
    else if(rule.lo>0)
      rule.factor=std::nextafter(double((static_cast<long double>(g.extent.x)+geometry_tolerance+2*g.tolerance)/rule.height),INFINITY);
    else {
      const long double radius=static_cast<long double>(g.extent.x)-geometry_tolerance-2*g.tolerance;
      if(!(radius>0))failure.Set(1,0,"Inward source envelope reaches the sphere center.");
      rule.factor=std::nextafter(double(radius/(static_cast<long double>(g.extent.x)+g.tolerance)),0.);
    }
    if(!std::isfinite(rule.factor) || (g.kind!=AnalyticSurface::Kind::Bump && !(rule.factor>0 && rule.factor!=1)))
      failure.Set(1,0,"Unrepresentable exterior source envelope.");
  }
  // Reserve disjoint transient source identities; accepted CFD identities are never changed.
  const auto node_span=transfer_memory::Add(maximum_node,1),node_end=transfer_memory::Mul(node_span,transfer_memory::Add(rules.size(),1));
  const auto cell_end=transfer_memory::Add(maximum_cell,1,transfer_memory::Mul(12,transfer_memory::Add(maximum_cell,1)));
  if(node_end==SIZE_MAX || cell_end==SIZE_MAX)failure.Set(1,0,"Exterior source identity range exhausted.");
  if(!Vote(world,failure,reason))return false;
  const size_t count=source.size();
  try {
    source.reserve(transfer_memory::Add(count,added));
    for(size_t at=0;at<count;++at) {
      const auto original=source[at];const auto& owned=cells.at(original.cell.id);
      for(size_t slot=0;slot<4;++slot)if(owned.physical[slot].present) {
        const Id component=owned.physical[slot].facet;
        const auto entry=reference.Analytic().find(component);if(entry==reference.Analytic().end())continue;
        const auto cap=caps.find(component);
        if(entry->second.kind==AnalyticSurface::Kind::Disk && cap==caps.end())continue;
        const auto key=cap==caps.end() ? Primitive(entry->second) : cap->second.first;
        const auto found=rules.find(key);const auto& rule=found->second;const auto& g=rule.geometry;
        if(cap!=caps.end() && !rule.pad)continue;
        const size_t index=std::distance(rules.begin(),found);
        const auto ids=Faces(original.cell)[slot];std::array<std::pair<Node,Tensor>,3> old;
        for(size_t i=0;i<3;++i)for(size_t j=0;j<4;++j)if(original.cell.v[j].id==ids[i])old[i]={original.cell.v[j],original.sensor[j]};
        std::sort(old.begin(),old.end(),[](const auto& a,const auto& b){return a.first.id<b.first.id;});
        auto next=old;
        for(auto& point:next) {
          auto& n=point.first;n.id=node_span*(index+1)+n.id;
          if(g.kind==AnalyticSurface::Kind::Bump)n.p.z=std::nextafter(n.p.z+rule.factor,rule.factor>0?INFINITY:-INFINITY);
          else if(g.kind==AnalyticSurface::Kind::Cylinder) {
            const auto original_point=n.p;n.p=ScaleCylinder(g,n.p,rule.factor);
            if(rule.pad) {
              const long double length=std::hypot(g.axis.x,g.axis.y,g.axis.z);
              const std::array<long double,3> a{{g.axis.x/length,g.axis.y/length,g.axis.z/length}};
              const long double t=(static_cast<long double>(original_point.x)-g.origin.x)*a[0]+
                  (static_cast<long double>(original_point.y)-g.origin.y)*a[1]+(static_cast<long double>(original_point.z)-g.origin.z)*a[2];
              const auto shift=rule.pad*(2*t/g.extent.y-1);
              n.p={double(n.p.x+shift*a[0]),double(n.p.y+shift*a[1]),double(n.p.z+shift*a[2])};
            }
          }
          else n.p={double(g.origin.x+(static_cast<long double>(n.p.x)-g.origin.x)*rule.factor),
                    double(g.origin.y+(static_cast<long double>(n.p.y)-g.origin.y)*rule.factor),
                    double(g.origin.z+(static_cast<long double>(n.p.z)-g.origin.z)*rule.factor)};
        }
        const std::array<std::array<std::pair<Node,Tensor>,4>,3> tets{{
          {old[0],old[1],old[2],next[2]},{old[0],old[1],next[1],next[2]},{old[0],next[0],next[1],next[2]}}};
        for(size_t i=0;i<3;++i) {
          auto v=tets[i];const auto sign=Orientation(v[0].first.p,v[1].first.p,v[2].first.p,v[3].first.p);
          if(!sign)throw std::invalid_argument("Unrepresentable exterior source tetrahedron.");
          if(sign<0)std::swap(v[0],v[1]);
          DonorCell donor;donor.cell.id=maximum_cell+1+12*original.cell.id+3*slot+i;donor.extension_component=component;donor.parent_cell=original.cell.id;
          for(size_t j=0;j<4;++j){donor.cell.v[j]=v[j].first;donor.sensor[j]=v[j].second;}
          source.push_back(donor);
        }
      }
    }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!Vote(world,failure,reason))return false;
  return AffineCoverage(world,cells,source,reference,live,ceiling,stats,reason,maximum_seconds);
}
}  // namespace
bool ImportGeometry(const CConfig& config, const CGeometry& geometry, const su2activematrix* metric,
                    ReferenceState& state, ImportedGeometry& output, std::string& reason, BridgeStats& stats,
                    size_t ceiling, size_t caller_bytes) {
  World parent;
  ImportedGeometry result;
  ReferenceState pending;
  {
    Envelope envelope(parent, stats);
    CLocalFailure failure;
    if (geometry.GetnDim() != 3 || !std::is_same<su2double, double>::value ||
        !std::is_same<passivedouble, double>::value)
      failure.Set(1, 0, "Native tetrahedral import requires a primal double-precision 3D geometry.");
    if (metric && (metric->cols() != 6 || metric->rows() < geometry.GetnPointDomain()))
      failure.Set(1, 0, "Native tetrahedral import needs six sensor components at every CFD-owned point.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const unsigned presence=(state.original?1u:0u)|(metric?2u:0u)|(HasSymmetryReference(config)?4u:0u)|
        (state.original && !state.original->Affine().empty()?8u:0u)|(state.original && !state.original->Analytic().empty()?16u:0u)|
        (state.original && state.original->Mapped()?32u:0u);
    const auto presence_low=CPassiveComm::Allreduce(presence,CPassiveComm::Op::MIN,parent.comm);
    const auto presence_high=CPassiveComm::Allreduce(presence,CPassiveComm::Op::MAX,parent.comm);
    if(presence_low!=presence_high)
      failure.Set(1,0,"Native3D import reference/sensor/declaration presence differs across ranks.");
    if(!Vote(parent,failure,reason))return false;
    const size_t declaration_bytes=HasSymmetryReference(config)?8*SymmetryFileBytes:0;
    std::vector<SymmetryPlane> declared_planes;
    if(declaration_bytes) {
      if(!Admit(parent,transfer_memory::Add(caller_bytes,ReferenceStorageBytes(state),OwnedStorageBytes(output.cells),
          transfer_memory::Bytes(output.original),declaration_bytes),ceiling,stats,reason))return false;
      if(!ReadSymmetryPlanes(parent,config,declared_planes,reason))return false;
    }
    const size_t np = geometry.GetnPoint(), owned_points = geometry.GetnPointDomain(), ne = geometry.GetnElem();
    size_t nf = 0, tag_chars = 0;
    for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) {
      nf = transfer_memory::Add(nf, geometry.GetnElem_Bound(m));
      tag_chars = transfer_memory::Add(tag_chars, config.GetMarker_All_TagBound(m).size() + 1);
    }
    const size_t global_tag_chars = CPassiveComm::Allreduce(uint64_t(tag_chars), CPassiveComm::Op::SUM, parent.comm);
    // OwnedSimplices/OwnedBoundaryFaces grow from empty; marker gather retains local and replicated text.
    const size_t helper_bound = transfer_memory::Add(
        caller_bytes, declaration_bytes, ReferenceStorageBytes(state), OwnedBytes(output.cells), transfer_memory::Bytes(output.original),
        transfer_memory::GrowthBound(ne, 5 * sizeof(unsigned long) + sizeof(CSimplexKey)),
        transfer_memory::GrowthBound(nf, 3 * sizeof(unsigned long) + sizeof(CSimplexKey) + sizeof(uint32_t)),
        transfer_memory::Mul(8, transfer_memory::Add(tag_chars, global_tag_chars)),
        transfer_memory::GrowthBound(config.GetnMarker_CfgFile() + geometry.GetnMarker(), sizeof(std::string)),
        transfer_memory::TransportBytes(parent.size), transfer_memory::Mul(parent.size, 16 * sizeof(uint64_t)));
    if (!Admit(parent, helper_bound, ceiling, stats, reason)) return false;
    const auto simplices = OwnedSimplices(geometry);
    std::vector<std::string> tags;
    for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) tags.push_back(config.GetMarker_All_TagBound(m));
    const auto names = CMeshGather::GatherMarkerNames(config, tags, geometry);
    const auto physical = OwnedBoundaryFaces(geometry, tags, config);
    const double point_start = parent.seconds();
    const uint64_t global_points = CPassiveComm::Allreduce(uint64_t(owned_points), CPassiveComm::Op::SUM, parent.comm);
    const uint64_t block = std::max<uint64_t>(1, transfer_memory::CeilDiv(global_points, parent.size));
    const uint64_t first = std::min<uint64_t>(global_points, uint64_t(parent.rank) * block),
                   last = std::min<uint64_t>(global_points, uint64_t(parent.rank + 1) * block);
    std::vector<uint64_t> build_to(parent.size), fetch_to(parent.size), build_from(parent.size),
        fetch_from(parent.size);
    for (unsigned long p = 0; p < np; ++p) {
      const auto id = geometry.nodes->GetGlobalIndex(p);
      if (id >= global_points) failure.Set(1, id, "Native3D point indices are outside the original owner numbering.");
      const int owner = std::min<uint64_t>(parent.size - 1, id / block);
      if (p < owned_points) ++build_to[owner];
      ++fetch_to[owner];
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    // Counts alone bound the house directory before it allocates payloads; halo requests may repeat across ranks.
    CPassiveComm::Alltoall(build_to.data(), build_from.data(), sizeof(uint64_t), parent.comm);
    CPassiveComm::Alltoall(fetch_to.data(), fetch_from.data(), sizeof(uint64_t), parent.comm);
    size_t build_in = 0, fetch_in = 0;
    for (int q = 0; q < parent.size; ++q) {
      build_in = transfer_memory::Add(build_in, build_from[q]);
      fetch_in = transfer_memory::Add(fetch_in, fetch_from[q]);
    }
    const size_t point_resident = transfer_memory::Add(helper_bound, transfer_memory::Mul(np, sizeof(uint64_t)),
                                                       transfer_memory::Mul(owned_points, 72));
    const size_t directory_build = transfer_memory::Add(
        point_resident, transfer_memory::Mul(owned_points, sizeof(int) + 80), transfer_memory::Mul(build_in, 80),
        transfer_memory::Mul(last - first, 73),
        transfer_memory::Staging(CPassiveComm::GetRoundBytes(), transfer_memory::Mul(owned_points, 80),
                                 transfer_memory::Mul(build_in, 80)));
    const size_t directory_fetch =
        transfer_memory::Add(point_resident, transfer_memory::Mul(last - first, 72),
                             transfer_memory::Mul(np, 8 + 72 + 72), transfer_memory::Mul(fetch_in, 8 + 72),
                             transfer_memory::Staging(CPassiveComm::GetRoundBytes(), transfer_memory::Mul(np, 8),
                                                      transfer_memory::Mul(fetch_in, 8)),
                             transfer_memory::Staging(CPassiveComm::GetRoundBytes(), transfer_memory::Mul(fetch_in, 72),
                                                      transfer_memory::Mul(np, 72)));
    if (!Admit(parent, std::max(directory_build, directory_fetch), ceiling, stats, reason)) return false;
    const size_t native_cells =
        transfer_memory::Mul(simplices.size(), sizeof(Owned::value_type) + 4 * sizeof(void*) + sizeof(DonorCell));
    const size_t point_and_cells = transfer_memory::Add(point_resident, transfer_memory::Mul(last - first, 72),
                                                        transfer_memory::Mul(np, 72), native_cells);
    if (!Admit(parent, point_and_cells, ceiling, stats, reason)) return false;
    {
      CPointDirectory points;
      std::vector<uint64_t> gids;
      std::vector<char> bytes;
      gids.reserve(np);
      bytes.reserve(transfer_memory::Mul(owned_points, 72));
      for (unsigned long p = 0; p < geometry.GetnPointDomain(); ++p) {
        gids.push_back(geometry.nodes->GetGlobalIndex(p));
        std::array<double, 9> values{};
        for (size_t d = 0; d < 3; ++d) values[d] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, d));
        for (size_t k = 0; k < 6; ++k)
          values[k + 3] = metric ? SU2_TYPE::GetValue((*metric)(p, k)) : (k == 0 || k == 3 || k == 5 ? 1. : 0.);
        try {
          for (size_t d = 0; d < 3; ++d)
            if (!std::isfinite(values[d])) throw std::invalid_argument("Nonfinite CFD-owned native3D coordinate.");
          ValidateTensor({values[3], values[4], values[5], values[6], values[7], values[8]});
          RecordStream encode(bytes);
          encode(values);
        } catch (const std::exception& e) {
          failure.Set(1, gids.back(), e.what());
        }
      }
      CollectiveFailure(failure, CURRENT_FUNCTION);
      points.Build(gids, bytes, 72, "native tetrahedral original sensor points");
      gids.clear();
      for (unsigned long p = 0; p < geometry.GetnPoint(); ++p) gids.push_back(geometry.nodes->GetGlobalIndex(p));
      const auto fetched = points.Fetch(gids);
      stats.points_seconds = parent.seconds() - point_start;
      const double cell_start = parent.seconds();
      const auto prefix = CPassiveComm::ExscanSum(simplices.size());
      try {
        result.original.reserve(simplices.size());
        for (unsigned long e = 0; e < simplices.size(); ++e) {
          DonorCell donor;
          donor.cell.id = prefix + e;
          for (size_t k = 0; k < 4; ++k) {
            const auto p = simplices.nodes[4 * e + k];
            const char* data = fetched.data() + 72 * p;
            const auto x = GetBytes<double>(data), y = GetBytes<double>(data), z = GetBytes<double>(data);
            donor.cell.v[k] = {gids[p], {x, y, z}};
            donor.sensor[k] = {GetBytes<double>(data), GetBytes<double>(data), GetBytes<double>(data),
                               GetBytes<double>(data), GetBytes<double>(data), GetBytes<double>(data)};
            if (x != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 0)) ||
                y != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 1)) ||
                z != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 2)))
              throw std::invalid_argument("Inconsistent native3D CFD halo coordinates.");
          }
          const auto& v = donor.cell.v;
          const auto sign = Orientation(v[0].p, v[1].p, v[2].p, v[3].p);
          if (!sign) throw std::invalid_argument("Degenerate native3D CFD tetrahedron.");
          if (sign < 0) {
            std::swap(donor.cell.v[0], donor.cell.v[1]);
            std::swap(donor.sensor[0], donor.sensor[1]);
          }
          result.cells.emplace(donor.cell.id, OwnedCell{donor.cell});
          result.original.push_back(donor);
        }
      } catch (const std::exception& e) {
        failure.Set(2, 0, e.what());
      }
      CollectiveFailure(failure, CURRENT_FUNCTION);
      stats.cells_seconds = parent.seconds() - cell_start;
    }  // Release point payloads/directory before constructing the face rendezvous.
    const double face_start = parent.seconds();
    const size_t face_count = transfer_memory::Add(physical.size(), transfer_memory::Mul(result.cells.size(), 4));
    const size_t face_base =
        transfer_memory::Add(helper_bound, OwnedBytes(result.cells), transfer_memory::Bytes(result.original));
    if (!Admit(parent,
               transfer_memory::Add(face_base, transfer_memory::GrowthBound(face_count, sizeof(FaceRecord)),
                                    transfer_memory::Mul(parent.size, sizeof(std::vector<FaceRecord>))),
               ceiling, stats, reason))
      return false;
    std::vector<std::vector<FaceRecord>> to(parent.size);
    for (unsigned long i = 0; i < physical.size(); ++i) {
      Face f{};
      for (size_t k = 0; k < 3; ++k) f[k] = geometry.nodes->GetGlobalIndex(physical.nodes[3 * i + k]);
      const auto name = config.GetMarker_CfgFile_TagBound(physical.markerId[i]);
      const auto found = std::find(names.begin(), names.end(), name);
      if (found == names.end()) failure.Set(1, f[0], "Native3D physical marker is missing from the global name list.");
      to[Bucket(f, parent.size)].push_back({Key(f), 0, parent.rank, 0, int(found - names.begin()), 0, true});
    }
    for (const auto& r : result.cells) {
      const auto faces = Faces(r.second.cell);
      for (size_t k = 0; k < 4; ++k)
        to[Bucket(faces[k], parent.size)].push_back(
            {Key(faces[k]), r.first, parent.rank, int(k), -1, Parity(faces[k]), false});
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    bool admitted = false;
    auto records = parent.exchange(to, &admitted, ceiling, face_base);
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, parent.max_exchange_work_bytes);
    if (!admitted) {
      reason = "Native3D face rendezvous exceeds storage budget.";
      return false;
    }
    std::sort(records.begin(), records.end(), [](const auto& a, const auto& b) {
      return std::tie(a.face, a.label, a.owner, a.cell) < std::tie(b.face, b.label, b.owner, b.cell);
    });
    const size_t reply_base =
        transfer_memory::Add(face_base, transfer_memory::Bytes(to), transfer_memory::Bytes(records));
    if (!Admit(parent,
               transfer_memory::Add(reply_base, transfer_memory::GrowthBound(records.size(), sizeof(Binding)),
                                    transfer_memory::Mul(parent.size, sizeof(std::vector<Binding>))),
               ceiling, stats, reason))
      return false;
    std::vector<std::vector<Binding>> reply(parent.size);
    for (size_t first = 0; first < records.size();) {
      size_t last = first + 1;
      while (last < records.size() && records[last].face == records[first].face) ++last;
      size_t volumes = 0, labels = 0;
      int marker = -1, orientation = 0;
      for (size_t i = first; i < last; ++i)
        if (records[i].label) {
          ++labels;
          marker = records[i].marker;
        } else {
          ++volumes;
          orientation += records[i].parity;
        }
      if ((volumes == 1 && labels == 1) || (volumes == 2 && labels == 0 && orientation == 0)) {
        if (labels)
          for (size_t i = first; i < last; ++i)
            if (!records[i].label) reply[records[i].owner].push_back({records[i].cell, records[i].slot, marker});
      } else
        failure.Set(1, records[first].face[0],
                    "Native3D face/physical-label incidence is not an oriented conforming manifold.");
      first = last;
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const auto bindings = parent.exchange(reply, &admitted, ceiling, reply_base);
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, parent.max_exchange_work_bytes);
    if (!admitted) {
      reason = "Native3D physical binding exchange exceeds storage budget.";
      return false;
    }
    const size_t reference_base =
        transfer_memory::Add(reply_base, transfer_memory::Bytes(reply), transfer_memory::Bytes(bindings));
    if (!state.original) {
      const auto count = CPassiveComm::Allreduce(uint64_t(bindings.size()), CPassiveComm::Op::SUM, parent.comm);
      if (!count || transfer_memory::Mul(count, sizeof(Facet)) == SIZE_MAX)
        failure.Set(1, 0, "Native3D immutable facet authority is empty or exceeds addressable storage.");
      CollectiveFailure(failure, CURRENT_FUNCTION);
      // Admit reference maps/index/component scratch and replicated wire copies before construction.
      const bool affine=std::any_of(declared_planes.begin(),declared_planes.end(),[](const auto& p){return !p.Legacy();});
      const size_t reference_scratch = PlanarReference::ConstructionBytes(count,0,affine?count:0);
      if (!Admit(parent,
                 transfer_memory::Add(reference_base, reference_scratch,
                                      transfer_memory::Add(transfer_memory::Mul(count, 3 * sizeof(Facet)),
                                                           transfer_memory::TransportBytes(parent.size))),
                 ceiling, stats, reason))
        return false;
      std::vector<Facet> local;
      const auto offset = CPassiveComm::ExscanSum(bindings.size());
      for (size_t i = 0; i < bindings.size(); ++i) {
        const auto b = bindings[i];
        const auto& cell = result.cells.at(b.cell).cell;
        const auto face = Faces(cell)[b.slot];
        Facet f;
        f.id = offset + i;
        f.marker = b.marker;
        for (size_t k = 0; k < 3; ++k)
          for (auto n : cell.v)
            if (n.id == face[k]) f.v[k] = n;
        local.push_back(f);
      }
      pending.facets = CPassiveComm::Allgatherv(local, nullptr, parent.comm);
      pending.marker_names = names;
      try {
        std::vector<Face> skin;
        for (auto f : pending.facets) skin.push_back({f.v[0].id, f.v[1].id, f.v[2].id});
        ValidateDomainSurfaceTopology(skin);
        pending.original = std::make_shared<PlanarReference>(pending.facets, reference_scratch,
            std::map<Id,AnalyticSurface>{},DeclaredSymmetryFacets(pending.facets,pending.marker_names,declared_planes));
      } catch (const std::exception& e) {
        failure.Set(1, 0, e.what());
      }
    }
    const auto& authority = state.original ? state : pending;
    if (authority.marker_names != names) failure.Set(1, 0, "Native3D immutable physical marker names changed.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    CLocalFailure declaration_failure;
    try {CheckDeclaredSymmetry(authority,declared_planes);}catch(const std::exception& e){declaration_failure.Set(1,0,e.what());}
    if(!Vote(parent,declaration_failure,reason))return false;
    if (!Admit(parent, transfer_memory::Add(reference_base,
                   state.original ? 0 : ReferenceStorageBytes(pending), authority.original->SearchWorkspaceBytes()),
               ceiling, stats, reason)) return false;
    try {
      for (auto b : bindings) {
        auto& cell = result.cells.at(b.cell);
        const auto face = Faces(cell.cell)[b.slot];
        std::array<Point, 3> p{};
        for (size_t k = 0; k < 3; ++k)
          for (auto n : cell.cell.v)
            if (n.id == face[k]) p[k] = n.p;
        Id chosen = 0;
        if (!authority.original->FindFace(b.marker, p, chosen))
          throw std::invalid_argument("Native3D physical triangle leaves its immutable oriented marker facets.");
        if(authority.original->Mapped()) {
          const auto* parameters=authority.original->Parameters();const auto found=parameters->Surface().find(Key(face));
          if(found==parameters->Surface().end() || found->second.facet!=chosen || Parity(found->second.v)!=Parity(face))
            throw std::invalid_argument("Actual boundary differs from persisted mapped topology.");
          for(auto n:cell.cell.v)if(std::find(face.begin(),face.end(),n.id)!=face.end()) {
            const auto* record=parameters->Find(n.id);
            if(!record || record->node.p.x!=n.p.x || record->node.p.y!=n.p.y || record->node.p.z!=n.p.z)
              throw std::invalid_argument("Actual boundary differs from persisted mapped coordinates/identities.");
          }
        }
        cell.physical[b.slot] = {true, chosen};
      }
    } catch (const std::exception& e) {
      failure.Set(1, 0, e.what());
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    if(authority.original->Mapped()) {
      const auto count=CPassiveComm::Allreduce(uint64_t(bindings.size()),CPassiveComm::Op::SUM,parent.comm);
      if(count!=authority.original->Parameters()->Surface().size())
        failure.Set(1,0,"Actual boundary has incomplete persisted mapped topology.");
      CollectiveFailure(failure,CURRENT_FUNCTION);
    }
    if(!Coverage(parent,result.cells,result.original,*authority.original,SU2_TYPE::GetValue(config.GetAdap_Hausd()),
                 transfer_memory::Add(reference_base,state.original ? 0 : ReferenceStorageBytes(pending)),ceiling,stats,reason,
                 SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time())))
      return false;
    stats.faces_seconds = parent.seconds() - face_start;
  }
  static_assert(std::is_nothrow_move_assignable<ImportedGeometry>::value,
                "Native3D import publication must not allocate.");
  result.stats = stats;
  output = std::move(result);
  if (!state.original) {
    pending.first_height_tol = state.first_height_tol;  // the caller's adopted policy survives the new authority
    std::swap(state, pending);
  }
  reason.clear();
  return true;
}
bool PrepareSymmetryReference(const CConfig& config,CGeometry& geometry,bool allow_projection,std::string& reason,
                             SymmetryReferenceAudit3D* audit,size_t ceiling) {
  if(config.GetAdap_Native_Symmetry_Reference().empty() || config.GetAdap_Native_Symmetry_Reference()=="NONE")return true;
  World world;CLocalFailure failure;SymmetryReferenceAudit3D report;
  auto vote=[&]() {const auto chosen=world.elect(failure);reason=chosen.message;if(audit)*audit=report;return !chosen.any;};
  auto admit=[&](size_t bytes) {
    report.maximum_bytes=std::max(report.maximum_bytes,bytes);
    if(bytes==SIZE_MAX || bytes>ceiling)failure.Set(3,0,"Native symmetry reference preparation exceeds storage budget.");
    return vote();
  };
  if(geometry.GetnDim()!=3 || config.GetFEMSolver() || config.GetKind_Adap_Remesher()!=ADAP_REMESHER::NATIVE_CAVITY ||
     !std::is_same<su2double,double>::value || config.GetDynamic_Grid())
    failure.Set(1,0,"Explicit native symmetry references require a static primal tetrahedral FVM geometry and NATIVE_CAVITY.");
  if(!vote())return false;
  constexpr size_t maximum_file=SymmetryFileBytes;
  if(!admit(transfer_memory::Add(transfer_memory::Mul(maximum_file,8),transfer_memory::TransportBytes(world.size))))return false;
  std::vector<SymmetryPlane> planes;
  if(!ReadSymmetryPlanes(world,config,planes,reason))return false;
  const size_t np=geometry.GetnPoint();size_t boundary_faces=0;
  for(unsigned m=0;m<geometry.GetnMarker();++m)if(config.GetMarker_All_KindBC(m)!=SEND_RECEIVE)
    boundary_faces=transfer_memory::Add(boundary_faces,geometry.GetnElem_Bound(m));
  const size_t base=transfer_memory::Add(transfer_memory::Mul(np,sizeof(std::array<double,3>)+sizeof(uint64_t)+sizeof(bool)),
      transfer_memory::Mul(boundary_faces,6*sizeof(Id)),transfer_memory::Mul(planes.size(),sizeof(std::vector<Id>)),
      transfer_memory::Mul(maximum_file,8),transfer_memory::TransportBytes(world.size));
  if(!admit(base))return false;
  std::vector<std::array<double,3>> candidate(np);std::vector<uint64_t> constraints(np,0);
  std::vector<std::vector<Id>> local_nodes(planes.size());
  for(size_t p=0;p<np;++p)for(unsigned k=0;k<3;++k)candidate[p][k]=SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,k));
  try {
    for(unsigned m=0;m<geometry.GetnMarker();++m)if(config.GetMarker_All_KindBC(m)!=SEND_RECEIVE)
      for(size_t f=0;f<geometry.GetnElem_Bound(m);++f) {
        const auto* face=geometry.bound[m][f];size_t chosen=planes.size();bool declared=false;
        for(size_t i=0;i<planes.size();++i) {
          const auto& plane=planes[i];if(plane.marker!=config.GetMarker_All_TagBound(m))continue;declared=true;
          bool match=plane.Legacy();
          if(!match) {
            const SU2NativeGeometry::AffineChart chart(plane.authority);match=true;
            for(unsigned k=0;k<face->GetnNodes();++k) {
              const auto q=candidate.at(face->GetNode(k));match=match && chart.ErrorBound({q[0],q[1],q[2]})<=plane.input_error;
            }
          }
          if(match) {
            if(chosen!=planes.size())throw std::invalid_argument("Ambiguous finite symmetry facet: overlapping input-deviation contracts.");
            chosen=i;
          }
        }
        if(!declared)continue;
        if(chosen==planes.size())throw std::invalid_argument("Original native symmetry facet exceeds its declared plane deviation.");
        for(unsigned k=0;k<face->GetnNodes();++k)local_nodes[chosen].push_back(geometry.nodes->GetGlobalIndex(face->GetNode(k)));
      }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  if(!vote())return false;
  for(size_t i=0;i<planes.size();++i) {
    const auto& plane=planes[i];auto& ids=local_nodes[i];
    std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());
    const auto count=CPassiveComm::Allreduce(uint64_t(ids.size()),CPassiveComm::Op::SUM,world.comm);
    if(!admit(transfer_memory::Add(base,transfer_memory::Mul(count,3*sizeof(Id)))))return false;
    auto global=CPassiveComm::Allgatherv(ids,nullptr,world.comm);
    std::sort(global.begin(),global.end());global.erase(std::unique(global.begin(),global.end()),global.end());
    if(global.empty())failure.Set(1,0,"Native symmetry reference component has no physical faces: "+plane.marker);
    double local_deviation=0;
    try {for(size_t p=0;p<np;++p)if(std::binary_search(global.begin(),global.end(),geometry.nodes->GetGlobalIndex(p))) {
      const auto q=candidate[p];const Point original{SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,0)),
          SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,1)),SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,2))};
      const double value=plane.Legacy()?SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,plane.axis)):0;
      const long double difference=plane.Legacy()?std::abs(static_cast<long double>(value)-plane.coordinate):
          SU2NativeGeometry::AffineChart(plane.authority).ErrorBound(original);
      const double deviation=double(difference);
      if(!std::isfinite(deviation) || difference>plane.input_error) {
        std::ostringstream message;message<<std::setprecision(17)<<"Original native symmetry vertex exceeds its declared plane deviation: marker="
            <<plane.marker<<" vertex="<<geometry.nodes->GetGlobalIndex(p)<<" deviation="<<deviation<<" limit="<<plane.input_error;
        failure.Set(1,geometry.nodes->GetGlobalIndex(p),message.str());
      }
      if(plane.Legacy()) {
        if(!allow_projection && value!=plane.coordinate)
          failure.Set(1,geometry.nodes->GetGlobalIndex(p),"Existing state/returned mesh is not exactly on its declared symmetry plane; initialize the reference before freezing fields.");
        for(size_t j=0;j<i;++j)if((constraints[p]&(uint64_t(1)<<j)) && planes[j].axis==plane.axis && planes[j].coordinate!=plane.coordinate)
          failure.Set(1,geometry.nodes->GetGlobalIndex(p),"Conflicting declared symmetry planes at a shared vertex.");
        candidate[p][plane.axis]=value==plane.coordinate?value:plane.coordinate;
      }
      constraints[p]|=uint64_t(1)<<i;local_deviation=std::max(local_deviation,deviation);
    }}catch(const std::exception& e){failure.Set(1,0,e.what());}
    const auto maximum=CPassiveComm::Allreduce(local_deviation,CPassiveComm::Op::MAX,world.comm);
    report.maximum_deviation=std::max(report.maximum_deviation,maximum);
    if(!world.rank)std::cout<<std::setprecision(17)<<"Native symmetry reference audit: marker="<<plane.marker
        <<" schema="<<(plane.Legacy()?1:2)<<" maximum_original_deviation="<<maximum<<" declared_limit="<<plane.input_error<<"\n";
    if(!vote())return false;
  }
  double mapping_error=0;
  try {for(size_t p=0;p<np;++p)if(constraints[p] && !planes.front().Legacy()) {
    std::map<Id,SU2NativeGeometry::AffineAuthority> active;
    const auto old=candidate[p];const Point original{old[0],old[1],old[2]};bool within=true;
    for(size_t i=0;i<planes.size();++i)if(constraints[p]&(uint64_t(1)<<i)) {
      const auto& g=planes[i].authority;active.emplace(g.identity,g);
      within=within && SU2NativeGeometry::AffineChart(g).ErrorBound(original)<=g.maximum_error;
    }
    if(!allow_projection && !within)throw std::invalid_argument("Existing state/returned mesh leaves its declared affine mapping contract; no frozen coordinates may move.");
    std::vector<SU2NativeGeometry::AffineAuthority> equations;for(const auto& row:active)equations.push_back(row.second);
    const auto projected=allow_projection?SU2NativeGeometry::ProjectIntersection(original,equations):original;
    candidate[p]={projected.x,projected.y,projected.z};
    for(const auto& row:active)mapping_error=std::max(mapping_error,SU2NativeGeometry::AffineChart(row.second).ErrorBound(projected));
  }}catch(const std::exception& e){failure.Set(1,0,e.what());}
  report.maximum_mapping_error=CPassiveComm::Allreduce(mapping_error,CPassiveComm::Op::MAX,world.comm);
  if(!vote())return false;
  uint64_t local_changed=0;double displacement=0;
  for(size_t p=0;p<geometry.GetnPointDomain();++p) {
    bool changed=false;for(unsigned k=0;k<3;++k)changed=changed || candidate[p][k]!=SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,k));
    local_changed+=changed;
    displacement=std::max(displacement,std::hypot(candidate[p][0]-SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,0)),
        candidate[p][1]-SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,1)),candidate[p][2]-SU2_TYPE::GetValue(geometry.nodes->GetCoord(p,2))));
  }
  report.maximum_displacement=CPassiveComm::Allreduce(displacement,CPassiveComm::Op::MAX,world.comm);
  std::vector<bool> junction(np,false);
  for(unsigned m=0;m<geometry.GetnMarker();++m)if(config.GetMarker_All_KindBC(m)!=SEND_RECEIVE)
    for(size_t f=0;f<geometry.GetnElem_Bound(m);++f)for(unsigned k=0;k<geometry.bound[m][f]->GetnNodes();++k) {
      const auto p=geometry.bound[m][f]->GetNode(k);
      if(constraints[p] && config.GetMarker_All_KindBC(m)!=SYMMETRY_PLANE)junction[p]=true;
    }
  double body_displacement=0;
  try {for(size_t p=0;p<np;++p)if(junction[p]) {
    const auto* old=geometry.nodes->GetCoord(p);const auto q=candidate[p];
    body_displacement=std::max(body_displacement,SU2NativeGeometry::DisplacementBound(
        {SU2_TYPE::GetValue(old[0]),SU2_TYPE::GetValue(old[1]),SU2_TYPE::GetValue(old[2])},{q[0],q[1],q[2]}));
  }}catch(const std::exception& e){failure.Set(1,0,e.what());}
  report.maximum_body_displacement=CPassiveComm::Allreduce(body_displacement,CPassiveComm::Op::MAX,world.comm);
  if(!planes.front().Legacy() && report.maximum_body_displacement>SU2_TYPE::GetValue(config.GetAdap_Hausd()))
    failure.Set(1,0,"Declared symmetry adoption exceeds the original faceted body accuracy contract; no coordinates changed.");
  if(!vote())return false;
  const auto junction_count=CPassiveComm::Allreduce(uint64_t(std::count(junction.begin(),junction.end(),true)),CPassiveComm::Op::SUM,world.comm);
  if(!admit(transfer_memory::Add(base,transfer_memory::Mul(junction_count,8*sizeof(Id)))))return false;
  std::vector<Id> junction_ids;
  for(size_t p=0;p<np;++p)if(junction[p])junction_ids.push_back(geometry.nodes->GetGlobalIndex(p));
  auto all_junctions=CPassiveComm::Allgatherv(junction_ids,nullptr,world.comm);
  std::sort(all_junctions.begin(),all_junctions.end());
  report.junction_points=std::unique(all_junctions.begin(),all_junctions.end())-all_junctions.begin();
  std::vector<Id>().swap(junction_ids);std::vector<Id>().swap(all_junctions);
  report.changed_points=CPassiveComm::Allreduce(local_changed,CPassiveComm::Op::SUM,world.comm);
  if(!world.rank && report.changed_points) {
    std::ifstream existing(config.GetMesh_FileName()+".native_ref");
    if(existing)failure.Set(1,0,"Cannot change coordinates beneath a persisted native reference; prepare a fresh initial reference explicitly.");
  }
  double minimum_ratio=1;
  try {for(size_t e=0;e<geometry.GetnElem();++e) {
    const auto* cell=geometry.elem[e];
    if(cell->GetVTK_Type()!=TETRAHEDRON){failure.Set(1,e,"Native symmetry reference needs a tetrahedral mesh.");continue;}
    bool touched=false;for(unsigned k=0;k<4;++k)touched=touched || constraints[cell->GetNode(k)];
    if(!touched || !report.changed_points)continue;
    Tetrahedron old,fresh;
    for(unsigned k=0;k<4;++k) {
      const auto p=cell->GetNode(k);const auto* q=geometry.nodes->GetCoord(p);
      old[k]={SU2_TYPE::GetValue(q[0]),SU2_TYPE::GetValue(q[1]),SU2_TYPE::GetValue(q[2])};
      fresh[k]={candidate[p][0],candidate[p][1],candidate[p][2]};
    }
    const auto before=Orientation(old[0],old[1],old[2],old[3]),after=Orientation(fresh[0],fresh[1],fresh[2],fresh[3]);
    if(!before || !after || (before>0)!=(after>0))failure.Set(1,cell->GetGlobalIndex(),"Declared symmetry projection degenerates/reverses tetrahedron "+std::to_string(cell->GetGlobalIndex())+".");
    else minimum_ratio=std::min(minimum_ratio,double(std::abs(after/before)));
  }
  }catch(const std::exception& e){failure.Set(1,0,e.what());}
  report.minimum_volume_ratio=CPassiveComm::Allreduce(minimum_ratio,CPassiveComm::Op::MIN,world.comm);
  if(!vote())return false;
  if(!planes.front().Legacy() && report.changed_points &&
      !CheckProjectedBoundary(world,config,geometry,candidate,report,base,ceiling,reason)) {
    if(audit)*audit=report;return false;
  }
  // Audit-only paths never write CFD coordinates, including signed-zero representations.
  if(allow_projection && report.changed_points)
    for(size_t p=0;p<np;++p)if(constraints[p])for(unsigned k=0;k<3;++k)geometry.nodes->SetCoord(p,k,candidate[p][k]);
  if(!world.rank)std::cout<<"Native symmetry reference "<<(allow_projection && report.changed_points?"adopted":"verified")
      <<": changed_points="<<report.changed_points
      <<" junction_points="<<report.junction_points<<" maximum_displacement="<<report.maximum_displacement
      <<" maximum_mapping_error="<<report.maximum_mapping_error<<" body_displacement_bound="<<report.maximum_body_displacement
      <<" minimum_volume_ratio="<<report.minimum_volume_ratio<<" boundary_witnesses="<<report.boundary_witnesses
      <<" boundary_pairs="<<report.boundary_pair_checks<<" boundary_check_seconds="<<report.boundary_check_seconds
      <<" maximum_requested_bytes="<<report.maximum_bytes<<" frozen_fields_moved=0\n";
  if(audit)*audit=report;reason.clear();return true;
}

ImportedGeometry ImportGeometry(const CConfig& config, const CGeometry& geometry, const su2activematrix* metric,
                                ReferenceState& state) {
  ImportedGeometry result;
  BridgeStats stats;
  std::string reason;
  if (!ImportGeometry(config, geometry, metric, state, result, reason, stats, SIZE_MAX))
    SU2_MPI::Error(reason, CURRENT_FUNCTION);
  return result;
}
namespace {
/*! Shared collective batch schedule. Only the original sensor is imported; composition remains query-local. */
template <class Make, class Use>
bool Samples(World& parent, size_t count, DonorImport& source, MetricComposition composition, Make make, Use use,
             BridgeStats& stats, std::string& reason, size_t ceiling, size_t live, size_t initial, bool query_timings, const PlanarReference* reference=nullptr,
             const std::function<void(size_t,Point,const DonorSample&)>& observe={}) {
  using Key=std::array<double,3>;
  const auto scratch=transfer_memory::Add(
      transfer_memory::Mul(MaximumSensorSamples,2*(sizeof(DonorSample)+sizeof(Key)+8*sizeof(void*))),
      transfer_memory::Mul(MaximumCavityCells,4*sizeof(DonorSample)+6*sizeof(Key)+256),
      reference?transfer_memory::GrowthBound(reference->ExtensionComponents().size(),sizeof(Id)):0,65536);
  live=transfer_memory::Add(live,scratch);
  if(!Admit(parent,live,ceiling,stats,reason))return false;
  std::map<Key,DonorSample> cache;std::deque<Key> order;std::vector<Id> extensions;
  CLocalFailure setup;
  try {if(reference)extensions=reference->ExtensionComponents();}
  catch(const std::exception& error){setup.Set(2,0,error.what());}
  if(!Vote(parent,setup,reason))return false;
  size_t next=0;
  while(parent.maximum(next<count)) {
    const auto start=parent.seconds();CLocalFailure failure;
    std::vector<DonorRegion> regions;std::vector<Point> missing;std::vector<size_t> slots;
    std::map<Key,size_t> index;std::vector<DonorSample> values;std::vector<unsigned char> seen;
    try {
      for(size_t i=next;i<std::min(count,next+initial);++i)make(i,regions);
      for(const auto& region:regions) {
        const Key key{region[0],region[1],region[2]};
        for(unsigned j=0;j<3;++j)if(!std::isfinite(key[j]) || key[j]!=region[j+3])
          throw std::invalid_argument("Native3D sample preparation requires finite point footprints.");
        const auto inserted=index.emplace(key,values.size());if(!inserted.second)continue;
        const auto hit=cache.find(key);
        values.push_back(hit==cache.end()?DonorSample{}:hit->second);seen.push_back(hit!=cache.end());
        if(hit==cache.end()){slots.push_back(inserted.first->second);missing.push_back({key[0],key[1],key[2]});}
      }
    }catch(const std::exception& error){failure.Set(2,0,error.what());}
    if(!Vote(parent,failure,reason)){stats.query_seconds+=parent.seconds()-start;return false;}
    const auto& before=source.Statistics();
    const double search=before.point_search_seconds,interpolation=before.point_interpolation_seconds;
    const uint64_t payloads=before.point_payloads;
    const auto lookupStart=parent.seconds();auto samples=source.SamplePoints(missing,extensions,ceiling,live);++stats.batches;
    stats.source_point_seconds+=parent.seconds()-lookupStart;stats.source_point_queries+=missing.size();
    stats.source_point_payloads+=source.Statistics().point_payloads-payloads;
    stats.source_point_search_seconds+=source.Statistics().point_search_seconds-search;
    stats.source_point_interpolation_seconds+=source.Statistics().point_interpolation_seconds-interpolation;
    if(!samples.valid){reason=samples.reason;stats.query_seconds+=parent.seconds()-start;return false;}
    try {
      if(samples.samples.size()!=slots.size())throw std::runtime_error("Incomplete original point sample batch.");
      for(size_t j=0;j<slots.size();++j) {
        values.at(slots[j])=samples.samples[j];const auto p=missing[j];const Key key{p.x,p.y,p.z};
        if(cache.size()==MaximumSensorSamples){cache.erase(order.front());order.pop_front();++stats.evictions;}
        cache.emplace(key,samples.samples[j]);order.push_back(key);
      }
      size_t active=SIZE_MAX;
      std::function<FieldSample(Point)> query=[&](Point p) {
        ++stats.requests;
        try {
          if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
            ++stats.invalid_queries;throw std::runtime_error("Nonfinite native3D prepared point query.");
          }
          const auto it=index.find({p.x,p.y,p.z});
          if(it==index.end()){++stats.invalid_queries;throw std::runtime_error("Undeclared native3D prepared point query.");}
          const auto i=it->second;const auto& value=values.at(i);
          if(seen.at(i))++stats.hits;else{++stats.misses;seen[i]=1;}
          if(observe)observe(active,p,value);
          FieldSample answer{value.sensor,value.sensor,value.Key(),value.weights,value.extension_component};
          if(composition) {
            const auto composeStart=query_timings?parent.seconds():0;++stats.compositions;
            try {answer.target=composition(p,answer.sensor);ValidateTensor(answer.target);}
            catch(...){if(query_timings)stats.composition_seconds+=parent.seconds()-composeStart;throw;}
            if(query_timings)stats.composition_seconds+=parent.seconds()-composeStart;
          }
          return answer;
        }catch(...){++stats.failures;throw;}
      };
      for(size_t i=next;i<std::min(count,next+initial);++i){active=i;use(i,query);}
    }catch(const std::exception& error){failure.Set(2,0,error.what());}
    if(!Vote(parent,failure,reason)){stats.query_seconds+=parent.seconds()-start;return false;}
    next+=std::min(initial,count-next);stats.query_seconds+=parent.seconds()-start;
  }
  stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, source.Statistics().maximum_work_bytes);
  return true;
}
void PointRegion(Point p, std::vector<DonorRegion>& regions) { regions.push_back({p.x, p.y, p.z, p.x, p.y, p.z}); }
}  // namespace
bool WorkingWeights(World& parent, const Owned& cells, DonorImport& source, std::vector<uint32_t>& output,
                    std::string& reason, BridgeStats& stats, MetricComposition composition, size_t ceiling,
                    size_t caller_bytes, bool query_timings) {
  Envelope envelope(parent, stats);
  const size_t live = transfer_memory::Add(caller_bytes, OwnedBytes(cells), source.ResidentBytes(),
                                           transfer_memory::Mul(cells.size(), sizeof(const Cell*) + sizeof(uint32_t)));
  if (!Admit(parent, live, ceiling, stats, reason)) return false;
  std::vector<const Cell*> ordered;
  std::vector<uint32_t> weights;
  CLocalFailure failure;
  try {
    weights.resize(cells.size());
    ordered.reserve(cells.size());
    for (auto& r : cells) ordered.push_back(&r.second.cell);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!Vote(parent, failure, reason)) return false;
  auto make = [&](size_t i, std::vector<DonorRegion>& regions) {
    const auto& cell = *ordered[i];
    PointRegion(Centroid(cell), regions);
    for (auto n : cell.v) PointRegion(n.p, regions);
  };
  auto use = [&](size_t i,const std::function<FieldSample(Point)>& query) { weights[i] = WorkWeights({*ordered[i]},query).front(); };
  if (!Samples(parent, cells.size(), source, std::move(composition), make, use, stats, reason, ceiling, live, 8,
               query_timings))
    return false;
  output.swap(weights);
  reason.clear();
  return true;
}
bool ReassessComplexity(World& parent,const Owned& candidate,DonorImport& source,
    const PlanarReference& reference,MetricComposition composition,ComplexityReassessment3D& output,
    std::string& reason,BridgeStats& stats,size_t ceiling,size_t caller_bytes,uint64_t count,uint64_t seed,
    std::function<bool()> withinBudget,std::shared_ptr<const PreparedComplexity3D>* prepared) {
  Envelope envelope(parent,stats);ComplexityReassessment3D result;CLocalFailure failure;
  const uint64_t controls[]={count,seed,uint64_t(bool(withinBudget)),uint64_t(bool(composition)),uint64_t(bool(prepared))};uint64_t lo[5],hi[5];
  SU2_MPI::Allreduce(controls,lo,5,MPI_UNSIGNED_LONG_LONG,MPI_MIN,parent.comm);
  SU2_MPI::Allreduce(controls,hi,5,MPI_UNSIGNED_LONG_LONG,MPI_MAX,parent.comm);
  if(!std::equal(lo,lo+5,hi) || count<32 || count>1048576 || source.Communicator()!=parent.comm)failure.Set(1,0,"Inconsistent/invalid candidate complexity assessment controls/source communicator.");
  if(!Vote(parent,failure,reason))return false;
  const size_t live=transfer_memory::Add(caller_bytes,OwnedBytes(candidate),source.ResidentBytes(),
      transfer_memory::Mul(candidate.size(),sizeof(const Cell*)+sizeof(WeightedCell3D)));
  auto limited=[&](const std::string& why) {result.reason=why;output=std::move(result);if(prepared)prepared->reset();reason.clear();return true;};
  if(!Admit(parent,live,ceiling,stats,reason))return limited(reason);
  std::vector<const Cell*> ordered;std::vector<WeightedCell3D> weighted;bool expired=false;
  auto time=[&]() {if(withinBudget && !withinBudget()){expired=true;throw CellSamplingLimit3D("Candidate complexity time budget exhausted.");}};
  try {ordered.reserve(candidate.size());weighted.resize(candidate.size());
    for(const auto& row:candidate)ordered.push_back(&row.second.cell);
    for(size_t i=0;i<ordered.size();++i) {
      weighted[i].id=ordered[i]->id;
      for(unsigned k=0;k<4;++k)weighted[i].points[k]=ordered[i]->v[k].p;
      auto& p=weighted[i].points;std::sort(p.begin(),p.end(),[](Point a,Point b){return std::tie(a.x,a.y,a.z)<std::tie(b.x,b.y,b.z);});
      if(Orientation(p[0],p[1],p[2],p[3])<0)std::swap(p[0],p[1]);
    }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(parent,failure,reason))return false;
  auto centroid=[&](size_t i){Cell c;for(unsigned k=0;k<4;++k)c.v[k].p=weighted[i].points[k];return Centroid(c);};
  auto prepare=[&](size_t i,std::vector<DonorRegion>& regions){time();PointRegion(centroid(i),regions);};
  auto use=[&](size_t i,const std::function<FieldSample(Point)>& query){weighted[i].importance=MetricDensity(query(centroid(i)).target);};
  if(!Samples(parent,ordered.size(),source,composition,prepare,use,stats,reason,ceiling,live,MaximumCavityCells,true,&reference)) {
    if(parent.maximum(expired))return limited(reason);
    if(parent.maximum(source.Statistics().maximum_work_bytes>ceiling || stats.maximum_native_bytes>ceiling))return limited(reason);
    return false;
  }
  long double local_centroid=0,global_centroid=0;
  for(const auto& row:weighted)local_centroid+=TetrahedronVolume(row.points)*row.importance;
  SU2_MPI::Allreduce(&local_centroid,&global_centroid,1,MPI_LONG_DOUBLE,MPI_SUM,parent.comm);
  result.value=result.centroid_estimate=double(global_centroid);
  try {
    CellSampler3D sampler(parent,std::move(weighted),ceiling,
        transfer_memory::Add(caller_bytes,OwnedBytes(candidate),source.ResidentBytes(),transfer_memory::Bytes(ordered)));
    stats.cells_seconds=sampler.preparation_seconds;
    result.centroid_estimate=double(sampler.CentroidEstimate());result.value=result.centroid_estimate;
    auto points=sampler.Draw(count,seed);stats.points_seconds=sampler.draw_seconds;
    stats.maximum_native_bytes=std::max(stats.maximum_native_bytes,sampler.MaximumBytes());
    std::shared_ptr<PreparedComplexity3D> cloud;
    std::vector<std::vector<OriginalComplexityPoint3D>> outgoing;
    if(prepared) {
      const auto scratch=transfer_memory::Add(caller_bytes,OwnedBytes(candidate),source.ResidentBytes(),
          transfer_memory::Bytes(ordered),sampler.ResidentBytes(),transfer_memory::Bytes(points),
          transfer_memory::GrowthBound(points.size(),sizeof(OriginalComplexityPoint3D)),
          transfer_memory::Mul(parent.size,sizeof(std::vector<OriginalComplexityPoint3D>)));
      if(!Admit(parent,scratch,ceiling,stats,reason))return limited(reason);
      try{cloud=std::make_shared<PreparedComplexity3D>();outgoing.resize(parent.size);}
      catch(const std::exception& e){failure.Set(3,0,e.what());}
      if(!Vote(parent,failure,reason))return limited(reason);
    }
    std::array<long double,11> local{},global{};
    const auto sample_live=transfer_memory::Add(caller_bytes,OwnedBytes(candidate),source.ResidentBytes(),
        transfer_memory::Bytes(ordered),sampler.ResidentBytes(),transfer_memory::Bytes(points),
        cloud?transfer_memory::Add(transfer_memory::GrowthBound(points.size(),sizeof(OriginalComplexityPoint3D)),transfer_memory::Bytes(outgoing)):0);
    auto make=[&](size_t i,std::vector<DonorRegion>& regions){time();PointRegion(points[i].point,regions);};
    auto consume=[&](size_t i,const std::function<FieldSample(Point)>& query) {
      const long double y=(long double)MetricDensity(query(points[i].point).target)*points[i].inverse_pdf;
      const auto residual=y-sampler.CentroidEstimate();
      if(!std::isfinite(residual))throw std::runtime_error("Nonfinite original-field complexity residual.");
      local[0]+=residual;local[1]+=residual*residual;++local[2];local[3+points[i].row%8]+=residual;
    };
    std::function<void(size_t,Point,const DonorSample&)> observe;
    if(cloud)observe=[&](size_t i,Point p,const DonorSample& donor) {
      if(i>=points.size() || donor.source_rank<0 || donor.source_rank>=parent.size)
        throw std::runtime_error("Invalid original complexity stencil publisher.");
      outgoing[donor.source_rank].push_back({points[i].row,p,points[i].inverse_pdf,donor.nodes,donor.weights,
          donor.cell,donor.extension_component,donor.source_rank});
    };
    if(!Samples(parent,points.size(),source,composition,make,consume,stats,reason,ceiling,sample_live,MaximumCavityCells,true,&reference,observe)) {
      if(parent.maximum(expired))return limited(reason);
      if(parent.maximum(source.Statistics().maximum_work_bytes>ceiling || stats.maximum_native_bytes>ceiling))return limited(reason);
      return false;
    }
    if(cloud) {
      bool admitted=false;
      cloud->points=parent.exchange(outgoing,&admitted,ceiling,sample_live);
      if(!admitted)return limited("Prepared original complexity stencils exceed storage ceiling.");
      const auto total=CPassiveComm::Allreduce(uint64_t(cloud->points.size()),CPassiveComm::Op::SUM,parent.comm);
      if(total!=count)failure.Set(2,0,"Incomplete prepared original complexity cloud.");
      for(const auto& p:cloud->points)if(p.row>=count || p.publisher!=parent.rank)
        failure.Set(2,p.row,"Misrouted prepared original complexity point.");
      if(!Vote(parent,failure,reason))return false;
      cloud->count=count;cloud->seed=seed;
    }
    const auto reductionStart=parent.seconds();
    SU2_MPI::Allreduce(local.data(),global.data(),global.size(),MPI_LONG_DOUBLE,MPI_SUM,parent.comm);
    stats.reduction_seconds+=parent.seconds()-reductionStart;
    if(uint64_t(global[2])!=count)throw std::runtime_error("Incomplete global original-field complexity assessment.");
    const long double mean=global[0]/count;
    result.value=double(sampler.CentroidEstimate()+mean);result.samples=count;
    result.standard_error=double(std::sqrt(std::max(0.L,(global[1]-global[0]*mean)/(count*(count-1.L)))));
    if(count%8==0) {
      long double spread=0;for(size_t b=0;b<8;++b)spread+=std::pow(global[3+b]/(count/8)-mean,2);
      result.block_standard_error=double(std::sqrt(spread/(8*7)));
    }
    result.uncertainty=3*std::max(result.standard_error,result.block_standard_error);
    if(!(result.value>0) || !std::isfinite(result.value) || !std::isfinite(result.uncertainty))throw std::runtime_error("Invalid global candidate complexity estimate.");
    result.estimated=true;output=std::move(result);if(prepared)*prepared=std::move(cloud);reason.clear();return true;
  }catch(const CellSamplingLimit3D& e){return limited(e.what());}
  catch(const std::exception& e){reason=e.what();return false;}
}
bool SynchronizeBoundaryParameters(World& parent,int workers,ReferenceState& state,std::string& reason,
    BridgeStats& stats,size_t ceiling,size_t caller_bytes) {
  Envelope envelope(parent,stats);CLocalFailure failure;
  if(workers<1 || workers>parent.size || !state.original || !state.original->Mapped())
    failure.Set(1,0,"Invalid mapped boundary return participants/reference.");
  if(!Vote(parent,failure,reason))return false;
  std::array<uint64_t,3> local{},counts{};
  try {if(!parent.rank)local={state.original->Parameters()->PayloadBytes(),state.original->Parameters()->Records().size(),
      state.original->Parameters()->Surface().size()};}
  catch(const std::exception& e){failure.Set(3,0,e.what());}
  if(!Vote(parent,failure,reason))return false;
  CPassiveComm::Allreduce(local.data(),counts.data(),counts.size(),CPassiveComm::Op::SUM,parent.comm);
  const auto live=transfer_memory::Add(caller_bytes,ReferenceStorageBytes(state),state.original->Parameters()->Geometry().ResidentBytes(),
      transfer_memory::Mul(counts[0],3),transfer_memory::Mul(counts[1],2304),transfer_memory::Mul(counts[2],8256),
      transfer_memory::TransportBytes(parent.size));
  if(!Admit(parent,live,ceiling,stats,reason))return false;
  std::vector<char> bytes;
  try {if(!parent.rank)bytes=state.original->Parameters()->Payload();}
  catch(const std::exception& e){failure.Set(3,0,e.what());}
  if(!Vote(parent,failure,reason))return false;
  const auto begin=parent.seconds();CPassiveComm::BcastRounds(bytes,0,parent.comm);
  stats.parameter_return_seconds=parent.seconds()-begin;stats.parameter_return_bytes=bytes.size();
  // Include this direct broadcast in the same parent transport totals as World exchanges.
  parent.collective_seconds+=stats.parameter_return_seconds;++parent.collective_calls;
  if(!parent.rank)parent.bytes_sent=transfer_memory::Add(parent.bytes_sent,transfer_memory::Mul(bytes.size(),parent.size-1));
  std::shared_ptr<const PlanarReference> replacement;
  try {
    if(bytes.size()!=counts[0])throw std::invalid_argument("Mapped return payload differs from its admitted length.");
    if(parent.rank>=workers)replacement=state.original->WithParameterState(BoundaryParameterState::FromPayload(
        state.original->Parameters()->Authority(),bytes,ceiling,transfer_memory::Add(caller_bytes,ReferenceStorageBytes(state))));
    else if(state.original->Parameters()->Payload()!=bytes)throw std::invalid_argument("Worker boundary parameters differ on parent return.");
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(parent,failure,reason))return false;
  if(replacement)state.original=std::move(replacement);
  stats.parameter_points=state.original->Parameters()->Records().size();
  stats.parameter_faces=state.original->Parameters()->Surface().size();
  reason.clear();return true;
}
namespace {
bool RebindReaderParameters(World& parent,const PlanarReference& reference,
    const std::vector<uint64_t>& keys,const std::vector<unsigned long>& dense,
    std::shared_ptr<const PlanarReference>& output,std::string& reason,BridgeStats& stats,size_t ceiling,size_t caller) {
  const auto& parameters=*reference.Parameters();
  using transfer_memory::Add;using transfer_memory::Mul;using transfer_memory::Bytes;
  const auto live=Add(caller,Mul(parameters.Records().size(),Add(1024,Mul(parent.size,64))));
  if(!Admit(parent,live,ceiling,stats,reason))return false;
  CLocalFailure failure;std::vector<std::vector<std::array<Id,2>>> to(parent.size);
  try {
    if(keys.size()!=dense.size())throw std::invalid_argument("Missing native reader dense identities.");
    for(size_t i=0;i<keys.size();++i)if(parameters.Find(keys[i]))to[keys[i]%parent.size].push_back({keys[i],dense[i]});
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(parent,failure,reason))return false;
  bool admitted=false;const auto rows=parent.exchange(to,&admitted,ceiling,live);
  stats.maximum_native_bytes=std::max(stats.maximum_native_bytes,parent.max_exchange_work_bytes);
  if(!admitted){reason="Native boundary numbering rendezvous exceeds storage admission.";return false;}
  std::map<Id,Id> owned;
  for(auto row:rows){const auto found=owned.emplace(row[0],row[1]);
    if(!found.second&&found.first->second!=row[1])failure.Set(2,row[0],"Inconsistent shared boundary reader identity.");}
  if(!Vote(parent,failure,reason))return false;
  std::vector<std::vector<std::array<Id,2>>> send(parent.size);
  for(auto& bucket:send)for(auto row:owned)bucket.push_back({row.first,row.second});
  const auto all=parent.exchange(send,&admitted,ceiling,Add(live,Bytes(to),Bytes(rows)));
  stats.maximum_native_bytes=std::max(stats.maximum_native_bytes,parent.max_exchange_work_bytes);
  if(!admitted){reason="Native boundary numbering publication exceeds storage admission.";return false;}
  const auto retained=Add(live,Bytes(to),Bytes(rows),Bytes(send),Bytes(all));
  const auto rebuilding=Add(retained,parameters.Geometry().ResidentBytes(),Mul(parameters.Records().size(),2304),
      Mul(parameters.Surface().size(),8192+sizeof(SurfaceFace)));
  if(!Admit(parent,rebuilding,ceiling,stats,reason))return false;
  std::shared_ptr<const PlanarReference> pending;
  try {
    std::map<Id,Id> numbering;
    for(auto row:all)if(!numbering.emplace(row[0],row[1]).second)throw std::invalid_argument("Repeated boundary numbering publisher.");
    pending=reference.WithParameterState(parameters.Rebind(numbering,ceiling,retained));
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!Vote(parent,failure,reason))return false;
  output=std::move(pending);return true;
}
} // namespace
bool ReaderSlices(World& parent, const Owned& cells, const std::vector<DonorCell>& original,
                  const ReferenceState& state, CReaderSlices& output, std::string& reason, BridgeStats& stats,
                  MetricComposition composition, size_t ceiling, size_t caller_bytes, bool query_timings,DonorImport* prepared_source,
                  std::shared_ptr<const PlanarReference>* returned_reference) {
  Envelope envelope(parent, stats);
  CLocalFailure failure;
  const int supplied=bool(prepared_source);
  const auto supplied_low=CPassiveComm::Allreduce(supplied,CPassiveComm::Op::MIN,parent.comm);
  const auto supplied_high=CPassiveComm::Allreduce(supplied,CPassiveComm::Op::MAX,parent.comm);
  if(supplied_low!=supplied_high || (prepared_source && (!prepared_source->BuiltFrom(original) || prepared_source->Communicator()!=parent.comm)))
    failure.Set(1,0,"Prepared native3D reader source differs from its immutable input/communicator.");
#ifdef HAVE_MPI
  int relation = MPI_UNEQUAL;
  MPI_Comm_compare(parent.comm, SU2_MPI::GetComm(), &relation);
  if (relation != MPI_IDENT) failure.Set(1, 0, "Native3D reader return requires the actual CFD communicator.");
#endif
  if (!state.original || state.marker_names.empty()) failure.Set(1, 0, "Uninitialized native3D reader reference.");
  if(state.original && state.original->Mapped() && !returned_reference)
    failure.Set(1,0,"Mapped native3D return requires its boundary-parameter output.");
  if (!Vote(parent, failure, reason)) return false;
  using transfer_memory::Add;
  using transfer_memory::Bytes;
  using transfer_memory::GrowthBound;
  using transfer_memory::Mul;
  const auto vertices = Mul(cells.size(), 4);
  size_t physical_faces = 0, name_bytes = 0;
  for (const auto& row : cells)
    for (const auto& face : row.second.physical) physical_faces += face.present;
  for (const auto& name : state.marker_names) name_bytes = Add(name_bytes, name.size(), 1);
  const size_t live = Add(caller_bytes, OwnedBytes(cells), Bytes(original), output.ResidentBytes(),
                         prepared_source?prepared_source->ResidentBytes():0,
                         Mul(vertices, sizeof(std::pair<const Id, Node>) + sizeof(std::pair<const Id, unsigned long>) +
                                           8 * sizeof(void*)),
                         GrowthBound(vertices, sizeof(Node) + sizeof(uint64_t) + 3 * sizeof(passivedouble)),
                         Mul(vertices, 6 * sizeof(passivedouble)),
                         GrowthBound(cells.size(), 4 * sizeof(unsigned long) + sizeof(int)),
                         GrowthBound(physical_faces, 3 * sizeof(unsigned long)),
                         GrowthBound(state.marker_names.size(), sizeof(CSimplexMesh::Marker)), name_bytes);
  if (!Admit(parent, live, ceiling, stats, reason)) return false;
  DonorImport transient(parent, query_timings);
  DonorImport& source=prepared_source?*prepared_source:transient;
  const double build = parent.seconds();
  if (!prepared_source && !source.Build(original, reason, ceiling, live)) {
    stats.source_seconds = parent.seconds() - build;
    return false;
  }
  stats.source_seconds = parent.seconds() - build;
  std::map<Id, Node> nodes;
  std::vector<Node> ordered;
  CSimplexMesh mesh;
  mesh.nDim = 3;
  std::vector<uint64_t> keys;
  std::map<Id, unsigned long> index;
  try {
    for (const auto& r : cells)
      for (auto n : r.second.cell.v) {
        const auto old = nodes.emplace(n.id, n);
        if (!old.second &&
            (old.first->second.p.x != n.p.x || old.first->second.p.y != n.p.y || old.first->second.p.z != n.p.z))
          throw std::invalid_argument("Inconsistent native3D output coordinates.");
      }
    for (auto r : nodes) {
      index.emplace(r.first, ordered.size());
      ordered.push_back(r.second);
      keys.push_back(r.first);
      mesh.coord.insert(mesh.coord.end(), {r.second.p.x, r.second.p.y, r.second.p.z});
    }
    mesh.metric.resize(6 * ordered.size());
    for (size_t m = 0; m < state.marker_names.size(); ++m)
      mesh.markers.push_back({state.marker_names[m], int(m + 1), {}});
    for (const auto& r : cells) {
      const auto& c = r.second.cell;
      for (auto n : c.v) mesh.elem.push_back(index.at(n.id));
      mesh.elemRef.push_back(1);
      const auto faces = Faces(c);
      for (size_t k = 0; k < 4; ++k)
        if (r.second.physical[k].present) {
          const auto marker = state.original->Get(r.second.physical[k].facet).marker;
          for (auto id : faces[k]) mesh.markers.at(marker).elem.push_back(index.at(id));
        }
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!Vote(parent, failure, reason)) return false;
  auto make = [&](size_t i, std::vector<DonorRegion>& regions) { PointRegion(ordered[i].p, regions); };
  auto use = [&](size_t i,const std::function<FieldSample(Point)>& query) {
    const auto m = query(ordered[i].p).target;
    const std::array<double, 6> values{m.xx, m.xy, m.xz, m.yy, m.yz, m.zz};
    std::copy(values.begin(), values.end(), mesh.metric.begin() + 6 * i);
  };
  if (!Samples(parent, ordered.size(), source, std::move(composition), make, use, stats, reason, ceiling,
               transfer_memory::Add(live,prepared_source?0:source.ResidentBytes()), 16, query_timings,state.original.get()))
    return false;
  const double reader_start = parent.seconds();
  CReaderSliceStats reader_stats;
  const size_t reader_caller = Add(caller_bytes, OwnedBytes(cells), Bytes(original), source.ResidentBytes(),
                                  Mul(nodes.size(), sizeof(decltype(nodes)::value_type) + 4 * sizeof(void*)),
                                  Mul(index.size(), sizeof(decltype(index)::value_type) + 4 * sizeof(void*)), Bytes(ordered),
                                  output.ResidentBytes());
  CReaderSlices pending;std::vector<unsigned long> dense;
  const bool mapped=state.original->Mapped();
  const bool accepted = CReaderSlices::FromDistributed(mesh, keys, pending, reason, reader_stats, ceiling, reader_caller,
      mapped?&dense:nullptr);
  stats.reader_seconds = parent.seconds() - reader_start;
  stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, reader_stats.maximum_bytes);
  if (!accepted) return false;
  std::shared_ptr<const PlanarReference> rebound;
  if(mapped && !RebindReaderParameters(parent,*state.original,keys,dense,rebound,reason,stats,ceiling,
      Add(live,pending.ResidentBytes(),Bytes(dense),source.ResidentBytes())))return false;
  output=std::move(pending);
  if(returned_reference)*returned_reference=std::move(rebound);
  reason.clear();
  return true;
}
}  // namespace SU2Native3D
