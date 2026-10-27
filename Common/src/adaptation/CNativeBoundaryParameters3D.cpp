/*!
 * \file CNativeBoundaryParameters3D.cpp
 * \brief Boundary-only parameter deltas with exact finite-facet boundary conservation.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CNativeDistributed2D.hpp"
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
#include "CNativePredicates3D_internal.hpp"
#include <algorithm>
#include <functional>
#include <cmath>
#include <cstring>
#include <limits>
namespace SU2Native3D {
namespace {
using State=BoundaryParameterState;
State::Coordinate PointKey(Point p){return {p.x,p.y,p.z};}
Face FaceKey(Face f){std::sort(f.begin(),f.end());return f;}
Face FaceOrder(Face f){const auto first=std::min_element(f.begin(),f.end());std::rotate(f.begin(),first,f.end());return f;}
State::Support BindingKey(const BoundaryPoint& p){const auto& b=p.binding;return {b.dimension,b.feature,b.u,b.v};}
bool EqualDouble(double a,double b){uint64_t x,y;std::memcpy(&x,&a,8);std::memcpy(&y,&b,8);return x==y;}
bool Equal(const BoundaryPoint& a,const BoundaryPoint& b) {
  return a.node.id==b.node.id && EqualDouble(a.node.p.x,b.node.p.x)&&EqualDouble(a.node.p.y,b.node.p.y)&&EqualDouble(a.node.p.z,b.node.p.z)&&
    a.binding.dimension==b.binding.dimension&&a.binding.feature==b.binding.feature&&a.binding.version==b.binding.version&&
    EqualDouble(a.binding.u,b.binding.u)&&EqualDouble(a.binding.v,b.binding.v);
}
template<class M>size_t MapBytes(const M& m){return transfer_memory::Mul(m.size(),sizeof(typename M::value_type)+4*sizeof(void*));}
void BoundaryIds(const Patch& patch,std::set<Id>& used){for(const auto& f:patch.surface)used.insert(f.v.begin(),f.v.end());}
using namespace SU2NativeGeometry;
constexpr uint64_t AtlasSchema=0x4e41544956454138ULL;
size_t RatioBytes(const ExactRatio& r){return transfer_memory::Add(transfer_memory::Bytes(r.numerator),transfer_memory::Bytes(r.denominator));}
size_t BaryBytes(const ExactBary& b){return transfer_memory::Add(RatioBytes(b.u),RatioBytes(b.v));}
bool AtlasActive(const State& s){return s.Mobility().identity || !s.Coverage().faces.empty() || !s.Coverage().traces.empty();}
void WriteRatio(SU2NativeBoundary2D::RecordStream& out,const ExactRatio& r) {
  uint64_t sign=r.negative,n=r.numerator.size(),d=r.denominator.size();out(sign,n,d);
  for(auto limb:r.numerator)out(limb);for(auto limb:r.denominator)out(limb);
}
void WriteBary(SU2NativeBoundary2D::RecordStream& out,const ExactBary& b){WriteRatio(out,b.u);WriteRatio(out,b.v);}
void WriteAtlas(SU2NativeBoundary2D::RecordStream& out,const BoundaryCoverage& c,const MobilityPolicy& p) {
  uint64_t schema=1,version=p.version,id=p.identity,nr=p.roles.size(),nf=p.features.size(),nc=p.corners.size(),nb=p.bodies.size(),ns=p.sheets.size(),nt=c.traces.size(),np=c.faces.size();
  double angle=p.angle;out(schema,version,id,angle,nr,nf,nc,nb,ns,nt,np);
  for(const auto& row:p.roles){auto key=row.first,value=row.second;out(key,value);}
  for(const auto& row:p.features){auto key=row.first;auto value=row.second;out(key,value);}
  for(auto value:p.corners)out(value);
  for(const auto& row:p.bodies){auto key=row.first,value=row.second;out(key,value);}
  for(const auto& row:p.sheets){auto key=row.first,value=row.second;out(key,value);}
  for(const auto& row:c.traces){auto key=row.first;uint64_t n=row.second.pieces.size();out(key,n);
    for(const auto& piece:row.second.pieces){auto facet=piece.facet;out(facet);WriteBary(out,piece.first);WriteBary(out,piece.last);WriteRatio(out,piece.end);}}
  for(const auto& row:c.faces){auto key=row.first;uint64_t n=row.second.pieces.size();out(key,n);
    for(const auto& piece:row.second.pieces){auto facet=piece.facet;out(facet);for(const auto& b:piece.reference)WriteBary(out,b);for(const auto& b:piece.active)WriteBary(out,b);}}
}
void CheckRatio(const ExactRatio& r,AtlasBudget& b){std::string why;if(!ExactValidate(r,b,why))throw std::invalid_argument(why);}
void CheckBary(const ExactBary& r,AtlasBudget& b){CheckRatio(r.u,b);CheckRatio(r.v,b);}
void ReadRatio(SU2NativeBoundary2D::RecordStream& in,size_t bytes,ExactRatio& r,AtlasBudget& b) {
  uint64_t sign,n,d;in(sign,n,d);
  const auto count=transfer_memory::Add(n,d);
  if(sign>1 || !d || count==SIZE_MAX || count>(bytes-in.Size())/8 ||
      n>(b.maximum_integer_bits+31)/32 || d>(b.maximum_integer_bits+31)/32)
    throw std::invalid_argument("Invalid bounded atlas rational lengths/sign.");
  const auto needed=transfer_memory::Add(b.caller_bytes,transfer_memory::Mul(count,sizeof(uint32_t)));
  if(needed==SIZE_MAX || needed>b.maximum_bytes)throw std::length_error("Atlas rational storage exceeds admission.");
  r.negative=sign;r.numerator.resize(n);r.denominator.resize(d);
  for(auto& limb:r.numerator)in(limb);for(auto& limb:r.denominator)in(limb);CheckRatio(r,b);
}
void ReadBary(SU2NativeBoundary2D::RecordStream& in,size_t bytes,ExactBary& r,AtlasBudget& b){ReadRatio(in,bytes,r.u,b);ReadRatio(in,bytes,r.v,b);}
EdgeTrace ReverseTrace(const EdgeTrace& t,AtlasBudget& budget) {
  EdgeTrace out;out.pieces.reserve(t.pieces.size());ExactRatio one;std::string why;
  if(!ExactInteger(1,one,budget,why))throw std::invalid_argument(why);
  for(size_t i=t.pieces.size();i;--i){auto p=t.pieces[i-1];std::swap(p.first,p.last);
    const ExactRatio begin=i==1?ExactRatio{}:t.pieces[i-2].end;
    if(!ExactSubtract(one,begin,p.end,budget,why))throw std::invalid_argument(why);out.pieces.push_back(std::move(p));}
  return out;
}
FaceCoverage RotateCoverage(FaceCoverage cover,unsigned rotation,AtlasBudget& budget) {
  if(!rotation)return cover;ExactRatio one;std::string why;if(!ExactInteger(1,one,budget,why))throw std::invalid_argument(why);
  for(auto& piece:cover.pieces)for(auto& b:piece.active){ExactRatio third,temp;
    if(!ExactSubtract(one,b.u,temp,budget,why)||!ExactSubtract(temp,b.v,third,budget,why))throw std::invalid_argument(why);
    const std::array<ExactRatio,3> weights{third,b.u,b.v};b.u=weights[(rotation+1)%3];b.v=weights[(rotation+2)%3];}
  return cover;
}
size_t FaceBytes(const FaceCoverage& cover){size_t n=transfer_memory::Bytes(cover.pieces);for(const auto& piece:cover.pieces)for(const auto* triangle:{&piece.reference,&piece.active})for(const auto& b:*triangle)n=transfer_memory::Add(n,BaryBytes(b));return n;}
size_t TraceBytes(const EdgeTrace& trace){size_t n=transfer_memory::Bytes(trace.pieces);for(const auto& p:trace.pieces)n=transfer_memory::Add(n,BaryBytes(p.first),BaryBytes(p.last),RatioBytes(p.end));return n;}
void CheckPolicyCoverage(Id anchor,const FaceCoverage& cover,const MobilityPolicy& policy,
    const MappedFacetReference& geometry,AtlasBudget& budget,std::string& reason) {
  auto zero=[](const ExactRatio& value){return !value.negative&&value.numerator.empty()&&value.denominator.size()==1&&value.denominator.front()==1;};
  auto zero_weight=[&](const ExactBary& point,unsigned weight){
    if(weight==1)return zero(point.u);if(weight==2)return zero(point.v);
    ExactRatio sum;if(!ExactAdd(point.u,point.v,sum,budget,reason))throw std::invalid_argument(reason);
    return !sum.negative&&sum.numerator.size()==1&&sum.numerator.front()==1&&sum.denominator.size()==1&&sum.denominator.front()==1;
  };
  for(const auto& piece:cover.pieces) {
    if(!policy.identity) {if(piece.facet!=anchor)throw std::invalid_argument("Inactive mobility policy forbids cross-chart coverage.");continue;}
    if(policy.bodies.at(piece.facet)!=policy.bodies.at(anchor)||policy.sheets.at(piece.facet)!=policy.sheets.at(anchor))
      throw std::invalid_argument("Atlas coverage crosses a physical feature, marker interface, or body.");
    const auto& facet=geometry.Reference().Get(piece.facet);
    for(unsigned opposite=0;opposite<3;++opposite) {
      Edge edge{facet.v[(opposite+1)%3].id,facet.v[(opposite+2)%3].id};std::sort(edge.begin(),edge.end());
      if(!policy.features.count(edge))continue;
      for(unsigned k=0;k<3;++k){const unsigned next=(k+1)%3;
        if(!zero_weight(piece.reference[k],opposite)||!zero_weight(piece.reference[next],opposite))continue;
        bool boundary=false;for(unsigned weight=0;weight<3&&!boundary;++weight)
          boundary=zero_weight(piece.active[k],weight)&&zero_weight(piece.active[next],weight);
        if(!boundary)throw std::invalid_argument("Protected original feature lies inside an active physical face.");
      }
    }
  }
}
size_t PolicyBytes(const MobilityPolicy& policy){return transfer_memory::Add(MapBytes(policy.roles),MapBytes(policy.features),MapBytes(policy.corners),MapBytes(policy.bodies),MapBytes(policy.sheets));}
struct AtlasStorage {
  AtlasBudget* budget;size_t caller=0,ceiling=0;
  AtlasStorage(AtlasBudget* value,size_t live,size_t limit):budget(value) {
    if(!budget)return;caller=budget->caller_bytes;ceiling=budget->maximum_bytes;
    if(caller>ceiling)throw std::length_error("Invalid atlas scratch reservation.");
    const auto next=std::max(caller,live),cap=transfer_memory::Add(next,ceiling-caller);
    if(next>limit||cap==SIZE_MAX)throw std::length_error("Atlas reserved live/scratch storage exceeds admission.");
    budget->caller_bytes=next;budget->maximum_bytes=std::min(cap,limit);
  }
  ~AtlasStorage(){if(budget){budget->caller_bytes=caller;budget->maximum_bytes=ceiling;}}
};
void WritePayload(SU2NativeBoundary2D::RecordStream& out,const State& state) {
  auto version=state.Geometry().Version(),revision=state.Revision();
  uint64_t np=state.Records().size(),nf=state.Surface().size(),policy=state.Geometry().ReferenceComponents();
  if(AtlasActive(state)){uint64_t magic=AtlasSchema,schema=8,atlas=BoundaryAtlasBytes(state.Coverage(),state.Mobility());out(magic,schema,version,revision,np,nf,policy,atlas);}
  else out(version,revision,np,nf,policy);
  for(auto row:state.Records())out(row.second.point);
  for(auto row:state.Surface())out(row.second);
  if(AtlasActive(state))WriteAtlas(out,state.Coverage(),state.Mobility());
}
void WriteSnapshot(SU2NativeBoundary2D::RecordStream& out,const State& state) {
  auto version=state.Geometry().Version(),revision=state.Revision();const auto& g=state.Geometry();
  uint64_t nf=g.Reference().Facets().size(),ne=g.Errors().size(),na=g.Reference().Affine().size(),np=state.Records().size(),ns=state.Surface().size();
  uint64_t policy=g.ReferenceComponents(),nc=g.Curves().size();out(version,revision,nf,ne,na,np,ns,policy,nc);
  for(auto row:g.Reference().Facets())out(row.second);
  for(auto row:g.Errors()){auto id=row.first;out(id,row.second);}
  for(auto row:g.Reference().Affine()){auto id=row.first;out(id,row.second);}
  for(auto row:g.Curves()){auto id=row.first;out(id,row.second);}
  for(auto row:state.Records()){auto id=row.first;out(id,row.second);}
  for(auto row:state.Surface())out(row.second);
  if(AtlasActive(state))WriteAtlas(out,state.Coverage(),state.Mobility());
}
}
AtlasBudget BoundaryAtlasBudget(size_t encoded,size_t ceiling,size_t caller) {
  using transfer_memory::Mul;using transfer_memory::Add;
  AtlasBudget budget;const auto words=Add(encoded/8,1);
  // Cubic limb-work envelope for canonical GCD/division; all storage/bit bounds derive from admitted wire words.
  budget.maximum_work=Mul(Mul(words,words),Mul(words,64));
  budget.maximum_integer_bits=Mul(encoded,16);budget.caller_bytes=caller;
  const auto archive_peak=Add(caller,Mul(encoded,32),SU2NativeGeometry::CurvedAtlasWorkspaceBytes(budget.maximum_integer_bits));
  budget.maximum_bytes=std::min(ceiling,archive_peak);
  if(budget.maximum_work==SIZE_MAX||budget.maximum_integer_bits==SIZE_MAX||archive_peak==SIZE_MAX||caller>budget.maximum_bytes)
    throw std::length_error("Atlas archive certificate resources overflow admission.");
  return budget;
}
size_t BoundaryCoverageBytes(const BoundaryCoverage& c) {
  size_t n=transfer_memory::Add(MapBytes(c.faces),MapBytes(c.traces));
  for(const auto& row:c.faces){n=transfer_memory::Add(n,transfer_memory::Bytes(row.second.pieces));for(const auto& p:row.second.pieces)
    for(const auto* triangle:{&p.reference,&p.active})for(const auto& b:*triangle)n=transfer_memory::Add(n,BaryBytes(b));}
  for(const auto& row:c.traces){n=transfer_memory::Add(n,transfer_memory::Bytes(row.second.pieces));for(const auto& p:row.second.pieces)
    n=transfer_memory::Add(n,BaryBytes(p.first),BaryBytes(p.last),RatioBytes(p.end));}
  return n;
}
size_t BoundaryAtlasBytes(const BoundaryCoverage& c,const MobilityPolicy& p) {SU2NativeBoundary2D::RecordStream out;WriteAtlas(out,c,p);return out.Size();}
std::vector<char> EncodeBoundaryAtlas(const BoundaryCoverage& c,const MobilityPolicy& p,size_t ceiling,size_t caller) {
  const auto size=BoundaryAtlasBytes(c,p),need=transfer_memory::Add(caller,BoundaryCoverageBytes(c),size);
  if(need==SIZE_MAX||need>ceiling)throw std::length_error("Atlas encoding exceeds storage admission.");
  auto budget=BoundaryAtlasBudget(size,ceiling,need);for(const auto& row:c.faces)for(const auto& piece:row.second.pieces)
    for(const auto* triangle:{&piece.reference,&piece.active})for(const auto& b:*triangle)CheckBary(b,budget);
  for(const auto& row:c.traces)for(const auto& piece:row.second.pieces){CheckBary(piece.first,budget);CheckBary(piece.last,budget);CheckRatio(piece.end,budget);}
  std::vector<char> bytes;bytes.reserve(size);SU2NativeBoundary2D::RecordStream out(bytes);WriteAtlas(out,c,p);return bytes;
}
void DecodeBoundaryAtlas(const std::vector<char>& bytes,BoundaryCoverage& coverage,MobilityPolicy& policy,size_t ceiling,size_t caller) {
  const auto need=transfer_memory::Add(caller,transfer_memory::Mul(bytes.size(),16));
  if(need==SIZE_MAX||need>ceiling)throw std::length_error("Atlas decoding exceeds storage admission.");
  SU2NativeBoundary2D::RecordStream in(bytes);uint64_t schema,nr,nf,nc,nb,ns,nt,np;MobilityPolicy p;BoundaryCoverage c;
  in(schema,p.version,p.identity,p.angle,nr,nf,nc,nb,ns,nt,np);
  if(schema!=1||p.version!=1||!std::isfinite(p.angle)||p.angle<0||p.angle>180||
      transfer_memory::Add(nr,nf,nc,nb,ns,nt,np)>bytes.size()/8)
    throw std::invalid_argument("Invalid atlas schema/policy/counts.");
  auto budget=BoundaryAtlasBudget(bytes.size(),ceiling,need);
  for(uint64_t i=0;i<nr;++i){int marker,role;in(marker,role);if(marker<0||(role!=0&&role!=2)||!p.roles.emplace(marker,role).second)throw std::invalid_argument("Invalid atlas marker role.");}
  for(uint64_t i=0;i<nf;++i){Edge edge;Id chain;in(edge,chain);if(edge[0]>=edge[1]||!p.features.emplace(edge,chain).second)throw std::invalid_argument("Invalid atlas feature edge.");}
  for(uint64_t i=0;i<nc;++i){Id id;in(id);if(!p.corners.insert(id).second)throw std::invalid_argument("Duplicate atlas corner.");}
  for(auto* map:{&p.bodies,&p.sheets}){const auto n=map==&p.bodies?nb:ns;for(uint64_t i=0;i<n;++i){Id id,root;in(id,root);if(!map->emplace(id,root).second)throw std::invalid_argument("Duplicate atlas policy facet.");}}
  for(uint64_t i=0;i<nt;++i){Edge edge;uint64_t n;in(edge,n);
    if(edge[0]>=edge[1]||!n||n>(bytes.size()-in.Size())/128)throw std::invalid_argument("Invalid atlas trace key/count.");
    EdgeTrace trace;trace.pieces.reserve(n);for(uint64_t j=0;j<n;++j){TracePiece piece;in(piece.facet);ReadBary(in,bytes.size(),piece.first,budget);ReadBary(in,bytes.size(),piece.last,budget);ReadRatio(in,bytes.size(),piece.end,budget);trace.pieces.push_back(std::move(piece));}
    if(!c.traces.emplace(edge,std::move(trace)).second)throw std::invalid_argument("Duplicate atlas trace.");}
  for(uint64_t i=0;i<np;++i){Face face;uint64_t n;in(face,n);
    if(face!=FaceOrder(face)||face[0]==face[1]||face[1]==face[2]||face[0]==face[2]||!n||n>(bytes.size()-in.Size())/296)throw std::invalid_argument("Invalid atlas coverage key/count.");
    FaceCoverage cover;cover.pieces.reserve(n);for(uint64_t j=0;j<n;++j){CoveragePiece piece;in(piece.facet);for(auto& b:piece.reference)ReadBary(in,bytes.size(),b,budget);for(auto& b:piece.active)ReadBary(in,bytes.size(),b,budget);cover.pieces.push_back(std::move(piece));}
    if(!c.faces.emplace(face,std::move(cover)).second)throw std::invalid_argument("Duplicate atlas coverage.");}
  if(!in.End())throw std::invalid_argument("Trailing atlas descriptor bytes.");coverage=std::move(c);policy=std::move(p);
}
BoundaryParameterState::BoundaryParameterState(std::shared_ptr<const SU2NativeGeometry::MappedFacetReference> authority,
    const BoundaryPoints& points,const std::vector<SurfaceFace>& surface,uint64_t epoch,size_t ceiling,size_t caller,
    const BoundaryCoverage& descriptors,const MobilityPolicy& policy,AtlasBudget* reservation)
    :geometry(std::move(authority)),revision(epoch) {
  if(!geometry || !revision)throw std::invalid_argument("Invalid boundary parameter authority/revision.");
  if((policy.identity||!descriptors.faces.empty()||!descriptors.traces.empty())&&!reservation)
    throw std::invalid_argument("Active atlas initialization requires an admitted geometry reservation.");
  const size_t bound=transfer_memory::Add(caller,geometry->ResidentBytes(),transfer_memory::Mul(points.size(),2048),transfer_memory::Mul(surface.size(),8192),transfer_memory::Mul(BoundaryCoverageBytes(descriptors),4),
      transfer_memory::Mul(BoundaryAtlasBytes(descriptors,policy),4));
  if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Boundary parameter initialization exceeds storage admission.");
  AtlasStorage reservation_storage(reservation,bound,ceiling);
  std::map<Id,SU2NativeGeometry::SurfaceBinding> bindings;std::map<Id,Point> actual;
  for(const auto& row:points) {
    const auto& p=row.second;
    if(row.first!=p.node.id || !geometry->Admissible(p.binding,p.node.p))throw std::invalid_argument("Invalid boundary parameter point/geometry contract.");
    if(!coordinates.emplace(PointKey(p.node.p),p.node.id).second || !supports.emplace(BindingKey(p),p.node.id).second)
      throw std::invalid_argument("Boundary parameter coordinates/support are not unique.");
    records.emplace(row.first,Entry{p,0});bindings.emplace(row.first,p.binding);actual.emplace(row.first,p.node.p);
  }
  coverage=descriptors;mobility=policy;
  if(mobility.identity)for(Id corner:mobility.corners)if(!supports.count(Support{0,corner,0,0}))
    throw std::invalid_argument("Atlas boundary omits a genuine physical corner/branch.");
  SU2NativeGeometry::MappedSurfaceAudit audit;std::string reason;
  if(mobility.identity) {
    auto& budget=*reservation;MobilityPolicy expected;
    if(!geometry->BuildMobilityPolicy(mobility.angle,mobility.roles,expected,budget,reason) ||
       EncodeBoundaryAtlas({},expected)!=EncodeBoundaryAtlas({},mobility))throw std::invalid_argument("Boundary mobility policy differs from immutable geometry/configuration.");
  } else if(!mobility.roles.empty()||!mobility.features.empty()||!mobility.corners.empty()||!mobility.bodies.empty()||!mobility.sheets.empty())
    throw std::invalid_argument("Inactive mobility policy retains nonlegacy declarations.");
  if(!AtlasActive(*this)) {
    if(!geometry->ValidateSurface(surface,bindings,actual,audit,reason,surface.size(),ceiling))throw std::invalid_argument(reason);
  } else {
    if(geometry->ReferenceComponents())throw std::invalid_argument("Curved atlas records cannot reinterpret planar component policy.");
    auto& budget=*reservation;std::vector<Face> skin;std::set<Id> used;std::vector<FaceCoverage> covers,original;
    for(const auto& face:surface) {
      FaceCoverage cover;if(!ResolveCoverage(face,cover,budget,reason))throw std::invalid_argument(reason);
      std::array<EdgeTrace,3> traces;std::array<SurfaceBinding,3> binding;std::array<Point,3> realized;
      for(unsigned k=0;k<3;++k){binding[k]=bindings.at(face.v[k]);realized[k]=actual.at(face.v[k]);used.insert(face.v[k]);
        Edge edge{face.v[k],face.v[(k+1)%3]};if(!ResolveTrace(edge,face,traces[k],budget,reason))throw std::invalid_argument(reason);
        if(edge[0]>edge[1])traces[k]=ReverseTrace(traces[k],budget);}
      CheckPolicyCoverage(face.facet,cover,mobility,*geometry,budget,reason);
      if(!geometry->ValidateCoverage(face.facet,binding,realized,cover,traces,budget,reason))throw std::invalid_argument(reason);
      skin.push_back(face.v);covers.push_back(std::move(cover));
    }
    if(used.size()!=records.size())throw std::invalid_argument("Atlas parameter table contains unused boundary points.");
    ValidateDomainSurfaceTopology(skin);
    for(const auto& row:geometry->Reference().Facets()) {
      std::array<SurfaceBinding,3> binding;for(unsigned k=0;k<3;++k)binding[k]=geometry->Vertex(row.second.v[k].id);
      FaceCoverage cover;if(!geometry->ResolveSingleChart(row.first,binding,cover,budget,reason))throw std::invalid_argument(reason);original.push_back(std::move(cover));
    }
    if(!geometry->ValidateCoverageReplacement(original,covers,budget,reason))throw std::invalid_argument(reason);
  }
  for(auto face:surface){face.facet=geometry->Canonical(face.facet);face.v=FaceOrder(face.v);if(!this->surface.emplace(FaceKey(face.v),face).second)throw std::invalid_argument("Duplicate accepted parameter face.");
    for(Id id:face.v)++records.at(id).faces;
    for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());++edge_counts[edge];edge_incident.emplace(edge,face);}}
  for(const auto& row:coverage.faces)if(!this->surface.count(FaceKey(row.first)) || this->surface.at(FaceKey(row.first)).v!=row.first)
    throw std::invalid_argument("Atlas cover has no accepted oriented physical face.");
  for(const auto& row:coverage.traces)if(!edge_counts.count(row.first))throw std::invalid_argument("Atlas trace has no accepted physical edge.");
}
std::shared_ptr<BoundaryParameterState> BoundaryParameterState::WithMobilityPolicy(double angle,const std::map<int,int>& roles,
    AtlasBudget& reservation,size_t ceiling,size_t caller)const {
  if(geometry->ReferenceComponents())throw std::invalid_argument("Curved mobility policy cannot reinterpret planar components.");
  if(mobility.identity&&(angle!=mobility.angle||roles!=mobility.roles))
    throw std::invalid_argument("Accepted boundary mobility policy differs from requested configuration.");
  const auto live=transfer_memory::Add(caller,geometry->ResidentBytes(),ResidentBytes());
  if(live==SIZE_MAX||live>ceiling)throw std::length_error("Boundary mobility policy setup exceeds storage admission.");
  AtlasStorage scope(&reservation,live,ceiling);MobilityPolicy policy;std::string reason;
  if(!geometry->BuildMobilityPolicy(angle,roles,policy,reservation,reason))throw std::invalid_argument(reason);
  if(mobility.identity&&(mobility.version!=policy.version||mobility.identity!=policy.identity||
      !EqualDouble(mobility.angle,policy.angle)||mobility.roles!=policy.roles||mobility.features!=policy.features||
      mobility.corners!=policy.corners||mobility.bodies!=policy.bodies||mobility.sheets!=policy.sheets))
    throw std::invalid_argument("Accepted boundary mobility policy differs from immutable geometry/configuration.");
  for(Id corner:policy.corners)if(!supports.count(Support{0,corner,0,0}))
    throw std::invalid_argument("Boundary mobility policy requires a missing genuine physical corner/branch.");
  if(!mobility.identity)for(const auto& row:surface) {
    const auto& face=row.second;const auto& original=geometry->Reference().Get(face.facet);
    std::array<SurfaceBinding,3> binding;std::array<Point,3> actual;Face features,original_ids;
    bool original_face=!coverage.faces.count(FaceOrder(face.v));
    for(unsigned k=0;k<3;++k) {
      const auto& point=records.at(face.v[k]).point;binding[k]=point.binding;actual[k]=point.node.p;
      features[k]=binding[k].feature;original_ids[k]=original.v[k].id;
      original_face=original_face&&binding[k].dimension==0&&binding[k].version==geometry->Version();
      bool exact=false;for(const auto& n:original.v)if(n.id==binding[k].feature)
        exact=EqualDouble(n.p.x,actual[k].x)&&EqualDouble(n.p.y,actual[k].y)&&EqualDouble(n.p.z,actual[k].z);
      original_face=original_face&&exact;
      Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());
      original_face=original_face&&!coverage.traces.count(edge);
    }
    // Original corners inherit the authority's positive cubic Jacobian against this exact chord normal.
    if(original_face&&FaceOrder(features)==FaceOrder(original_ids))continue;
    FaceCoverage cover;std::array<EdgeTrace,3> traces;
    if(!ResolveCoverage(face,cover,reservation,reason))throw std::invalid_argument(reason);
    for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};
      if(!ResolveTrace(edge,face,traces[k],reservation,reason))throw std::invalid_argument(reason);
      if(edge[0]>edge[1])traces[k]=ReverseTrace(traces[k],reservation);}
    CheckPolicyCoverage(face.facet,cover,policy,*geometry,reservation,reason);
    if(!geometry->ValidateCoverage(face.facet,binding,actual,cover,traces,reservation,reason))throw std::invalid_argument(reason);
  }
  const auto peak=transfer_memory::Add(live,PolicyBytes(policy),ResidentBytes());
  if(peak==SIZE_MAX||peak>ceiling)throw std::length_error("Boundary mobility policy copy exceeds storage admission.");
  reservation.peak_bytes=std::max(reservation.peak_bytes,peak);
  auto migrated=std::make_shared<BoundaryParameterState>(*this);migrated->mobility=std::move(policy);return migrated;
}
bool BoundaryParameterState::ResolveCoverage(const SurfaceFace& face,FaceCoverage& out,AtlasBudget& budget,std::string& reason)const {
  try {
    const auto found=coverage.faces.find(FaceOrder(face.v));
    if(found!=coverage.faces.end()) {
      const auto key=found->first;unsigned rotation=0;while(key[rotation]!=face.v[0])++rotation;
      out=RotateCoverage(found->second,rotation,budget);return true;
    }
    std::array<SurfaceBinding,3> binding;for(unsigned k=0;k<3;++k){const auto p=Find(face.v[k]);if(!p)throw std::invalid_argument("Coverage face lacks a forward binding.");binding[k]=p->binding;}
    return geometry->ResolveSingleChart(face.facet,binding,out,budget,reason);
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool BoundaryParameterState::ResolveTrace(Edge edge,const SurfaceFace& face,EdgeTrace& out,AtlasBudget& budget,std::string& reason)const {
  try {
    std::sort(edge.begin(),edge.end());const auto found=coverage.traces.find(edge);
    if(found!=coverage.traces.end()){out=found->second;return true;}
    std::array<SurfaceBinding,3> binding;for(unsigned k=0;k<3;++k){const auto p=Find(face.v[k]);if(!p)throw std::invalid_argument("Trace face lacks a forward binding.");binding[k]=p->binding;}
    FaceCoverage single;std::array<EdgeTrace,3> traces;
    if(!geometry->ResolveSingleChart(face.facet,binding,single,budget,reason)||!geometry->CoverageTraces(single,traces,budget,reason))return false;
    for(unsigned k=0;k<3;++k){Edge candidate{face.v[k],face.v[(k+1)%3]},key=candidate;std::sort(key.begin(),key.end());
      if(key==edge){out=candidate[0]<candidate[1]?traces[k]:ReverseTrace(traces[k],budget);return true;}}
    throw std::invalid_argument("Trace is not incident to its physical face.");
  }catch(const std::exception& e){reason=e.what();return false;}
}
const SurfaceFace* BoundaryParameterState::IncidentFace(Edge edge)const {std::sort(edge.begin(),edge.end());const auto face=edge_incident.find(edge);return face==edge_incident.end()?nullptr:&face->second;}
const BoundaryPoint* BoundaryParameterState::Find(Id id)const {const auto p=records.find(id);return p==records.end()?nullptr:&p->second.point;}
const BoundaryPoint* BoundaryParameterState::Find(Point point)const {
  for(auto x:PointKey(point))if(!std::isfinite(x))throw std::invalid_argument("Nonfinite boundary parameter lookup.");
  const auto p=coordinates.find(PointKey(point));return p==coordinates.end()?nullptr:Find(p->second);
}
size_t BoundaryParameterState::ResidentBytes()const {return transfer_memory::Add(sizeof(*this),MapBytes(records),MapBytes(coordinates),MapBytes(supports),MapBytes(surface),MapBytes(edge_counts),MapBytes(edge_incident),BoundaryCoverageBytes(coverage),PolicyBytes(mobility));}
size_t BoundaryParameterState::SnapshotBytes()const {SU2NativeBoundary2D::RecordStream measure;WriteSnapshot(measure,*this);return measure.Size();}
std::vector<char> BoundaryParameterState::Snapshot()const {
  std::vector<char> bytes;bytes.reserve(SnapshotBytes());SU2NativeBoundary2D::RecordStream out(bytes);WriteSnapshot(out,*this);return bytes;
}
size_t BoundaryParameterState::PayloadBytes()const {
  SU2NativeBoundary2D::RecordStream measure;WritePayload(measure,*this);return measure.Size();
}
std::vector<char> BoundaryParameterState::Payload()const {
  std::vector<char> bytes;bytes.reserve(PayloadBytes());SU2NativeBoundary2D::RecordStream out(bytes);WritePayload(out,*this);return bytes;
}
std::shared_ptr<BoundaryParameterState> BoundaryParameterState::FromPayload(
    std::shared_ptr<const SU2NativeGeometry::MappedFacetReference> authority,const std::vector<char>& bytes,
    size_t ceiling,size_t caller) {
  using transfer_memory::Add;using transfer_memory::Mul;
  SU2NativeBoundary2D::RecordStream input(bytes);uint64_t version=0,epoch=0,np=0,nf=0,policy=0,atlas_bytes=0,schema=0;input(version);
  const bool atlas=version==AtlasSchema;
  if(atlas){input(schema,version);if(schema!=8)throw std::invalid_argument("Unsupported boundary parameter payload schema.");}
  input(epoch,np,nf,policy);if(atlas)input(atlas_bytes);
  if(!authority || version!=authority->Version() || policy!=authority->ReferenceComponents() || !epoch || !np || !nf ||
      Add(atlas?64:40,Mul(np,72),Mul(nf,32),atlas_bytes)!=bytes.size())throw std::invalid_argument("Invalid boundary parameter payload/version/counts.");
  const size_t live=Add(caller,transfer_memory::Bytes(bytes),Mul(np,256),Mul(nf,sizeof(SurfaceFace)));
  const size_t bound=Add(live,authority->ResidentBytes(),Mul(np,2048),Mul(nf,8192),Mul(atlas_bytes,32));
  if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Boundary parameter payload exceeds storage admission.");
  BoundaryPoints points;std::vector<SurfaceFace> faces;faces.reserve(nf);
  for(uint64_t i=0;i<np;++i){BoundaryPoint point;input(point);
    if(!points.emplace(point.node.id,point).second)throw std::invalid_argument("Duplicate boundary parameter payload identity.");}
  for(uint64_t i=0;i<nf;++i){SurfaceFace face;input(face);faces.push_back(face);}
  BoundaryCoverage descriptors;MobilityPolicy mobility;
  if(atlas){std::vector<char> encoded;encoded.reserve(atlas_bytes);SU2NativeBoundary2D::RecordStream write(encoded);
    for(uint64_t i=0;i<atlas_bytes/8;++i){uint64_t word;input(word);write(word);}DecodeBoundaryAtlas(encoded,descriptors,mobility,ceiling,live);}
  if(!input.End())throw std::invalid_argument("Trailing boundary parameter payload bytes.");
  auto reservation=BoundaryAtlasBudget(atlas?atlas_bytes:0,ceiling,bound);
  return std::make_shared<BoundaryParameterState>(std::move(authority),points,faces,epoch,ceiling,live,descriptors,mobility,&reservation);
}
std::shared_ptr<BoundaryParameterState> BoundaryParameterState::Rebind(const std::map<Id,Id>& numbering,
    size_t ceiling,size_t caller)const {
  using transfer_memory::Add;using transfer_memory::Mul;
  if(numbering.size()!=records.size() || revision==UINT64_MAX)throw std::invalid_argument("Incomplete boundary numbering or exhausted revision.");
  const size_t live=Add(caller,Mul(numbering.size(),256),Mul(surface.size(),sizeof(SurfaceFace)),Mul(BoundaryCoverageBytes(coverage),4));
  const size_t bound=Add(live,geometry->ResidentBytes(),Mul(records.size(),2048),Mul(surface.size(),8192),Mul(BoundaryAtlasBytes({},mobility),4));
  if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Boundary parameter renumbering exceeds storage admission.");
  BoundaryPoints points;std::vector<SurfaceFace> faces;faces.reserve(surface.size());
  for(const auto& row:records){auto p=row.second.point;p.node.id=numbering.at(row.first);
    if(!points.emplace(p.node.id,p).second)throw std::invalid_argument("Boundary reader numbering is not one-to-one.");}
  for(const auto& row:surface){auto face=row.second;for(auto& id:face.v)id=numbering.at(id);faces.push_back(face);}
  BoundaryCoverage rebound;auto budget=BoundaryAtlasBudget(AtlasActive(*this)?BoundaryAtlasBytes(coverage,mobility):0,ceiling,bound);
  for(const auto& row:coverage.traces){Edge key{numbering.at(row.first[0]),numbering.at(row.first[1])};auto trace=row.second;
    if(key[0]>key[1]){std::swap(key[0],key[1]);trace=ReverseTrace(trace,budget);}rebound.traces.emplace(key,std::move(trace));}
  for(const auto& row:coverage.faces){Face key=row.first;for(auto& id:key)id=numbering.at(id);
    const auto rotation=std::min_element(key.begin(),key.end())-key.begin();auto cover=RotateCoverage(row.second,unsigned(rotation),budget);
    rebound.faces.emplace(FaceOrder(key),std::move(cover));}
  return std::make_shared<BoundaryParameterState>(geometry,points,faces,revision+1,ceiling,live,rebound,mobility,&budget);
}
std::vector<BoundaryParameterEdit> BoundaryParameterState::Changes(const Patch& old,const Patch& fresh,uint64_t epoch) {
  std::set<Id> before,after;BoundaryIds(old,before);BoundaryIds(fresh,after);std::vector<BoundaryParameterEdit> changes;
  auto check=[&](const Patch& patch,const std::set<Id>& ids) {
    if(patch.parameters.size()!=ids.size())throw std::invalid_argument("Private patch has incomplete/extra boundary parameters.");
    std::map<Id,Point> coordinates;for(const auto& cell:patch.cells)for(auto n:cell.v)coordinates.emplace(n.id,n.p);
    for(Id id:ids) {
      const auto& p=patch.parameters.at(id);if(p.node.id!=id || PointKey(p.node.p)!=PointKey(coordinates.at(id)))
        throw std::invalid_argument("Private boundary parameters differ from actual mesh coordinates.");
    }
  };
  check(old,before);check(fresh,after);
  for(Id id:before)if(!after.count(id))changes.push_back({old.parameters.at(id),epoch,false});
  for(Id id:after) {
    if(before.count(id)) {if(!Equal(old.parameters.at(id),fresh.parameters.at(id)))
      throw std::invalid_argument("Boundary coordinate/parameter movement requires a new point identity.");}
    else changes.push_back({fresh.parameters.at(id),epoch,true});
  }
  return changes;
}
BoundaryParameterState::Publication BoundaryParameterState::Prepare(const std::vector<BoundaryParameterEdit>& edits,
    const std::vector<BoundaryFaceChange>& faces,std::string& reason,size_t ceiling,size_t caller,const BoundaryCoverage& delta,AtlasBudget* reservation)const {
  Publication staged;staged.before=staged.after=revision;
  try {
    size_t inherited_bytes=0;
    for(const auto& face:faces)if(!face.insert){const auto cover=coverage.faces.find(FaceOrder(face.nodes));if(cover!=coverage.faces.end())inherited_bytes=transfer_memory::Add(inherited_bytes,FaceBytes(cover->second));
      for(unsigned k=0;k<3;++k){Edge edge{face.nodes[k],face.nodes[(k+1)%3]};std::sort(edge.begin(),edge.end());const auto trace=coverage.traces.find(edge);if(trace!=coverage.traces.end())inherited_bytes=transfer_memory::Add(inherited_bytes,TraceBytes(trace->second));}}
    for(const auto& row:delta.faces){const auto cover=coverage.faces.find(row.first);if(cover!=coverage.faces.end())inherited_bytes=transfer_memory::Add(inherited_bytes,FaceBytes(cover->second));
      for(unsigned k=0;k<3;++k){Edge edge{row.first[k],row.first[(k+1)%3]};std::sort(edge.begin(),edge.end());const auto trace=coverage.traces.find(edge);if(trace!=coverage.traces.end())inherited_bytes=transfer_memory::Add(inherited_bytes,TraceBytes(trace->second));}}
    const size_t bound=transfer_memory::Add(caller,ResidentBytes(),transfer_memory::Mul(inherited_bytes,4),transfer_memory::Mul(edits.size(),2048),transfer_memory::Mul(faces.size(),8192),transfer_memory::Mul(BoundaryCoverageBytes(delta),8));
    if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Boundary parameter publication exceeds storage admission.");
    AtlasStorage reservation_storage(reservation,bound,ceiling);
    if(edits.empty() && faces.empty() && delta.faces.empty() && delta.traces.empty()){staged.valid=true;reason.clear();return staged;}
    if(revision==UINT64_MAX)throw std::length_error("Boundary parameter revision range exhausted.");staged.after=revision+1;
    std::set<Id> changed;
    for(const auto& edit:edits) {
      const auto& p=edit.point;const auto old=records.find(p.node.id);
      if(edit.revision!=revision || !changed.insert(p.node.id).second)throw std::invalid_argument("Stale/duplicate boundary parameter edit.");
      if(edit.insert) {
        if(old!=records.end() || !geometry->Admissible(p.binding,p.node.p))throw std::invalid_argument("Boundary parameter insertion conflicts with its identity/geometry.");
        if(!staged.added.emplace(p.node.id,Entry{p,0}).second)throw std::invalid_argument("Duplicate boundary parameter insertion.");
      }else {
        if(old==records.end() || !Equal(p,old->second.point))throw std::invalid_argument("Boundary parameter erasure differs from accepted state.");
        staged.removed.insert(p.node.id);
      }
    }
    for(const auto& row:staged.added) {
      const auto& p=row.second.point;const auto coord=PointKey(p.node.p);const auto support=BindingKey(p);
      const auto c=coordinates.find(coord);const auto s=supports.find(support);
      if((c!=coordinates.end()&&!staged.removed.count(c->second)) || (s!=supports.end()&&!staged.removed.count(s->second)) ||
         !staged.coordinates.emplace(coord,row.first).second || !staged.supports.emplace(support,row.first).second)
        throw std::invalid_argument("New boundary coordinates/support collide with accepted geometry.");
    }
    if(mobility.identity)for(Id id:staged.removed){const auto& old=records.at(id).point;
      if(old.binding.dimension==0&&mobility.corners.count(old.binding.feature)&&!staged.supports.count(BindingKey(old)))
        throw std::invalid_argument("Atlas publication removes a genuine physical corner/branch.");}
    for(const auto& change:faces)if(!change.insert) {
      const auto key=FaceKey(change.nodes);const auto old=surface.find(key);
      if(old==surface.end() || old->second.facet!=change.facet || old->second.v!=FaceOrder(change.nodes) || !staged.surface_removed.insert(key).second)
        throw std::invalid_argument("Retired physical face differs from accepted parameter topology.");
    }
    for(const auto& change:faces)if(change.insert) {
      const auto key=FaceKey(change.nodes);const auto retained=surface.find(key);
      if(retained!=surface.end()&&retained->second.v==FaceOrder(change.nodes)&&retained->second.facet!=change.facet)throw std::invalid_argument("Retained physical face changes its immutable anchor.");
      if((surface.count(key)&&!staged.surface_removed.count(key)) || !staged.surface_added.emplace(key,SurfaceFace{FaceOrder(change.nodes),change.facet}).second)
        throw std::invalid_argument("New parameter topology repeats an accepted face.");
    }
    auto point=[&](Id id,bool added)->const BoundaryPoint& {
      if(added){const auto p=staged.added.find(id);if(p!=staged.added.end())return p->second.point;
        if(staged.removed.count(id))throw std::invalid_argument("New physical face retains an erased boundary point.");}
      const auto p=Find(id);if(!p)throw std::invalid_argument("Physical face has no boundary parameter binding.");return *p;
    };
    std::map<Id,int64_t> degree;
    const bool atlas=AtlasActive(*this)||!delta.faces.empty()||!delta.traces.empty();
    if(!atlas) {
    std::map<std::tuple<Id,Id,Id>,int64_t> edges;
    std::set<std::tuple<bool,Id,Face>> unique;
    for(const auto& face:faces) {
      auto key=face.nodes;std::sort(key.begin(),key.end());
      if(!unique.emplace(face.insert,face.facet,key).second)throw std::invalid_argument("Duplicate physical face in parameter publication.");
      std::array<SU2NativeGeometry::SurfaceBinding,3> b;std::array<Point,3> p;
      for(unsigned k=0;k<3;++k){const auto& q=point(face.nodes[k],face.insert);b[k]=q.binding;p[k]=q.node.p;degree[face.nodes[k]]+=face.insert?1:-1;}
      if(!geometry->ContainsFace(face.facet,b,p))throw std::invalid_argument("Published face leaves/reverses its finite parameter support.");
      for(unsigned k=0;k<3;++k){Id a=face.nodes[k],b=face.nodes[(k+1)%3];const int orientation=a<b?1:-1;if(a>b)std::swap(a,b);
        edges[{face.facet,a,b}]+=(face.insert?1:-1)*orientation;}
    }
    std::map<std::tuple<Id,Id,double>,int64_t> intervals;
    for(const auto& row:edges)if(row.second) {
      const auto facet=std::get<0>(row.first),a=std::get<1>(row.first),b=std::get<2>(row.first);
      auto any=[&](Id id)->const BoundaryPoint& {const auto n=staged.added.find(id);return n==staged.added.end()?*Find(id):n->second.point;};
      Id feature;double ta,tb;
      if(!geometry->BoundarySegment(facet,any(a).binding,any(b).binding,feature,ta,tb))
        throw std::invalid_argument("Parameter replacement changes its retained facet interface.");
      intervals[{facet,feature,ta}]+=row.second;intervals[{facet,feature,tb}]-=row.second;
    }
    for(const auto& row:intervals)if(row.second)throw std::invalid_argument("Parameter replacement changes finite feature coverage.");
    } else {
      if(geometry->ReferenceComponents())throw std::invalid_argument("Atlas publication cannot reinterpret planar components.");
      if(!reservation)throw std::invalid_argument("Atlas publication lacks an admitted geometry reservation.");
      auto& budget=*reservation;std::map<Edge,int64_t> edge_degree;
      std::vector<FaceCoverage> before,after;
      std::vector<BoundaryFaceChange> coverage_faces=faces;
      std::map<Edge,std::set<Face>> coordinated;
      auto include_incident=[&](const SurfaceFace& face){const auto key=FaceKey(face.v);
        for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());coordinated[edge].insert(key);}};
      for(const auto& key:staged.surface_removed)include_incident(surface.at(key));
      for(const auto& row:delta.faces) {
        if(row.first!=FaceOrder(row.first))throw std::invalid_argument("Atlas cover key is not cyclic canonical.");
        const auto key=FaceKey(row.first);const auto added=staged.surface_added.find(key);const auto old=surface.find(key);
        if(added!=staged.surface_added.end()) {
          if(added->second.v!=row.first)throw std::invalid_argument("Atlas cover orientation differs from added face.");
        } else {
          if(old==surface.end()||old->second.v!=row.first||staged.surface_removed.count(key))
            throw std::invalid_argument("Atlas cover delta has no retained oriented physical face.");
          // Include metadata edits in the same old/new reference-chain proof without changing physical incidence.
          coverage_faces.push_back({row.first,old->second.facet,false});coverage_faces.push_back({row.first,old->second.facet,true});
        }
        if(old!=surface.end())include_incident(old->second);
        staged.coverage_added.faces.emplace(row);
      }
      for(const auto& face:faces)for(unsigned k=0;k<3;++k){Edge edge{face.nodes[k],face.nodes[(k+1)%3]};std::sort(edge.begin(),edge.end());edge_degree[edge]+=face.insert?1:-1;}
      for(const auto& row:edge_degree) {
        const auto old=edge_counts.find(row.first);const uint64_t count=old==edge_counts.end()?0:old->second;const auto change=row.second;
        if(change<0&&uint64_t(-change)>count)throw std::invalid_argument("Atlas edge incidence underflows.");
        const uint64_t next=change<0?count-uint64_t(-change):transfer_memory::Add(count,uint64_t(change));
        if(next==SIZE_MAX||(next!=0&&next!=2))throw std::invalid_argument("Atlas edge has nonmanifold physical incidence.");
        staged.edge_counts.emplace(row.first,next);if(!next)staged.traces_removed.insert(row.first);
      }
      for(const auto& key:staged.surface_removed){const auto& face=surface.at(key);staged.coverage_removed.insert(face.v);}
      auto accepted_trace=[&](Edge edge,EdgeTrace& trace)->bool {
        const auto explicit_trace=coverage.traces.find(edge);if(explicit_trace!=coverage.traces.end()){trace=explicit_trace->second;return true;}
        const auto incident=IncidentFace(edge);if(incident)return ResolveTrace(edge,*incident,trace,budget,reason);
        return false;
      };
      for(const auto& row:delta.traces) {
        const auto next=staged.edge_counts.find(row.first);const auto old=edge_counts.find(row.first);
        if(row.first[0]>=row.first[1] || (next==staged.edge_counts.end()?old==edge_counts.end():!next->second))
          throw std::invalid_argument("Atlas trace delta has no surviving physical edge.");
        if(old!=edge_counts.end()) {EdgeTrace inherited;
          if(!accepted_trace(row.first,inherited))throw std::invalid_argument("Retained physical edge lacks its inherited atlas trace.");
          if(!geometry->EqualTraces(inherited,row.second,budget,reason)) {
            const auto incident=coordinated.find(row.first);
            if(incident==coordinated.end()||incident->second.size()!=old->second)
              throw std::invalid_argument("Changed retained atlas trace requires every accepted incident face in the coordinated update.");
          }}
        staged.coverage_added.traces.emplace(row);
      }
      for(const auto& face:coverage_faces) {
        const SurfaceFace f{face.nodes,face.facet};FaceCoverage cover;
        std::array<SurfaceBinding,3> binding;std::array<Point,3> realized;
        for(unsigned k=0;k<3;++k){const auto& p=point(face.nodes[k],face.insert);binding[k]=p.binding;realized[k]=p.node.p;degree[face.nodes[k]]+=face.insert?1:-1;}
        const auto supplied=delta.faces.find(FaceOrder(face.nodes));
        if(!face.insert) {
          if(!ResolveCoverage(f,cover,budget,reason))throw std::invalid_argument(reason);
          // Retired faces belong to this validated state; retain their chain, not another proof.
          before.push_back(std::move(cover));continue;
        }
        if(supplied!=delta.faces.end()) {
          const auto key=supplied->first;unsigned rotation=0;while(key[rotation]!=face.nodes[0])++rotation;cover=RotateCoverage(supplied->second,rotation,budget);
        } else if(!geometry->ResolveSingleChart(face.facet,binding,cover,budget,reason))throw std::invalid_argument("New cross-chart physical face has no explicit coverage: "+reason);
        std::array<EdgeTrace,3> traces;
        for(unsigned k=0;k<3;++k) {
          Edge edge{face.nodes[k],face.nodes[(k+1)%3]},key=edge;std::sort(key.begin(),key.end());
          const auto supplied_trace=delta.traces.find(key);
          if(supplied_trace!=delta.traces.end())traces[k]=supplied_trace->second;
          else if(edge_counts.count(key)) {if(!accepted_trace(key,traces[k]))throw std::invalid_argument("Retained edge lacks its accepted atlas trace.");}
          else {FaceCoverage single;std::array<EdgeTrace,3> direct;
            if(!geometry->ResolveSingleChart(face.facet,binding,single,budget,reason)||!geometry->CoverageTraces(single,direct,budget,reason))
              throw std::invalid_argument("New cross-chart edge lacks its explicit atlas trace.");
            traces[k]=edge[0]<edge[1]?direct[k]:ReverseTrace(direct[k],budget);
          }
          if(supplied!=delta.faces.end() && !coverage.traces.count(key) && !delta.traces.count(key))
            staged.coverage_added.traces.emplace(key,traces[k]);
          if(edge[0]>edge[1])traces[k]=ReverseTrace(traces[k],budget);
        }
        CheckPolicyCoverage(face.facet,cover,mobility,*geometry,budget,reason);
        if(!geometry->ValidateCoverage(face.facet,binding,realized,cover,traces,budget,reason))throw std::invalid_argument(reason);
        after.push_back(std::move(cover));
      }
      if((!before.empty()||!after.empty())&&!geometry->ValidateCoverageReplacement(before,after,budget,reason))throw std::invalid_argument(reason);
    }
    for(const auto& row:degree) {
      const auto old=records.find(row.first);const uint64_t count=old==records.end()?0:old->second.faces;const int64_t delta=row.second;
      if((delta<0 && uint64_t(-delta)>count) || (delta>0 && uint64_t(delta)>UINT64_MAX-count))
        throw std::invalid_argument("Boundary face-incidence count underflows/overflows.");
      const uint64_t next=delta<0?count-uint64_t(-delta):count+uint64_t(delta);
      if(staged.removed.count(row.first)){if(next)throw std::invalid_argument("Erased boundary point is still used by physical faces.");}
      else if(!next)throw std::invalid_argument("Boundary parameter erasure is missing for a retired point.");
      else if(staged.added.count(row.first))staged.added.at(row.first).faces=next;
      else staged.final_counts.emplace(row.first,next);
    }
    for(const auto& row:staged.added)if(!row.second.faces)throw std::invalid_argument("Unused boundary parameter insertion.");
    for(Id id:staged.removed)if(!degree.count(id))throw std::invalid_argument("Boundary parameter erasure has no mesh change.");
    if(!atlas) {
      std::map<Edge,int64_t> counts;for(const auto& face:faces)for(unsigned k=0;k<3;++k){Edge e{face.nodes[k],face.nodes[(k+1)%3]};std::sort(e.begin(),e.end());counts[e]+=face.insert?1:-1;}
      for(const auto& row:counts){const auto old=edge_counts.find(row.first);const auto count=old==edge_counts.end()?0:old->second;
        if(row.second<0 && uint64_t(-row.second)>count)throw std::invalid_argument("Boundary edge incidence underflows.");
        const auto next=row.second<0?count-uint64_t(-row.second):count+uint64_t(row.second);if(next!=0&&next!=2)throw std::invalid_argument("Nonmanifold boundary edge incidence.");staged.edge_counts.emplace(row.first,next);}
    }
    for(const auto& face:faces)if(face.insert)for(unsigned k=0;k<3;++k){Edge edge{face.nodes[k],face.nodes[(k+1)%3]};std::sort(edge.begin(),edge.end());staged.edge_incident.emplace(edge,SurfaceFace{FaceOrder(face.nodes),face.facet});}
    staged.valid=true;reason.clear();return staged;
  }catch(const std::exception& e){reason=e.what();return {};}
}
void BoundaryParameterState::Commit(Publication&& staged)noexcept {
  if(!staged.valid || staged.before!=revision)std::terminate();
  for(Id id:staged.removed){const auto p=records.find(id);coordinates.erase(PointKey(p->second.point.node.p));supports.erase(BindingKey(p->second.point));records.erase(p);}
  for(const auto& row:staged.final_counts)records.find(row.first)->second.faces=row.second;
  while(!staged.added.empty())records.insert(staged.added.extract(staged.added.begin()));
  while(!staged.coordinates.empty())coordinates.insert(staged.coordinates.extract(staged.coordinates.begin()));
  while(!staged.supports.empty())supports.insert(staged.supports.extract(staged.supports.begin()));
  for(const auto& key:staged.surface_removed)surface.erase(key);
  while(!staged.surface_added.empty())surface.insert(staged.surface_added.extract(staged.surface_added.begin()));
  for(const auto& face:staged.coverage_removed)coverage.faces.erase(face);
  for(const auto& edge:staged.traces_removed)coverage.traces.erase(edge);
  while(!staged.coverage_added.faces.empty()){auto node=staged.coverage_added.faces.extract(staged.coverage_added.faces.begin());coverage.faces.erase(node.key());coverage.faces.insert(std::move(node));}
  while(!staged.coverage_added.traces.empty()){auto node=staged.coverage_added.traces.extract(staged.coverage_added.traces.begin());coverage.traces.erase(node.key());coverage.traces.insert(std::move(node));}
  while(!staged.edge_counts.empty()){auto node=staged.edge_counts.extract(staged.edge_counts.begin());edge_counts.erase(node.key());if(node.mapped())edge_counts.insert(std::move(node));else edge_incident.erase(node.key());}
  while(!staged.edge_incident.empty()){auto node=staged.edge_incident.extract(staged.edge_incident.begin());edge_incident.erase(node.key());edge_incident.insert(std::move(node));}
  revision=staged.after;
}
} // namespace SU2Native3D

namespace SU2Native3D {
struct PlanarReference::ParameterView {
  std::shared_ptr<BoundaryParameterState> accepted;
  BoundaryPoints local;
  BoundaryCoverage coverage;
  std::map<Face,SurfaceFace> surface;
  AtlasBudget* reservation=nullptr;
  std::map<BoundaryParameterState::Coordinate,Id> coordinates;
  std::map<BoundaryParameterState::Coordinate,BoundaryPoint> pending;
  std::vector<Id> components;
  uint64_t revision=0;
  bool private_view=false;
  size_t OverlayBytes()const{return transfer_memory::Add(sizeof(*this),MapBytes(local),MapBytes(coordinates),MapBytes(pending),MapBytes(surface),BoundaryCoverageBytes(coverage),transfer_memory::Bytes(components));}
  void Check()const{if(private_view&&revision!=accepted->Revision())throw std::invalid_argument("Stale private boundary parameter epoch.");}
  void Bound()const{if(local.size()+pending.size()>8*MaximumReplacementCells)throw std::length_error("Private boundary parameter workspace exhausted.");}
  void Insert(BoundaryPoint p) {
    Check();
    if(!accepted->Geometry().Admissible(p.binding,p.node.p))throw std::invalid_argument("Private boundary point violates its geometry binding.");
    const auto key=PointKey(p.node.p);const auto old=coordinates.find(key);
    if(old!=coordinates.end()&&old->second!=p.node.id)throw std::invalid_argument("Private boundary coordinates have conflicting identities.");
    const auto before=local.find(p.node.id);if(before!=local.end())coordinates.erase(PointKey(before->second.node.p));
    local[p.node.id]=p;coordinates[key]=p.node.id;Bound();
  }
};
PlanarReference::PlanarReference(std::shared_ptr<const PlanarReference> base,std::shared_ptr<ParameterView> view)
    :analytic{},affine{},definition(std::move(base)),parameter_view(std::move(view)) {}
std::shared_ptr<const PlanarReference> PlanarReference::WithParameters(std::shared_ptr<const PlanarReference> base,
    std::shared_ptr<BoundaryParameterState> parameters) {
  if(!base||!parameters)throw std::invalid_argument("Missing mapped boundary reference/parameters.");
  if(base->definition)base=base->definition;
  if(&parameters->Geometry().Reference()!=base.get())throw std::invalid_argument("Parameter authority differs from its shared reference.");
  auto view=std::make_shared<ParameterView>();view->accepted=std::move(parameters);view->revision=view->accepted->Revision();
  const Id tag=base->Facets().begin()->first;if(tag==UINT64_MAX)throw std::invalid_argument("Mapped source identity uses the reserved physical-source tag.");
  view->components.push_back(tag);
  return std::shared_ptr<const PlanarReference>(new PlanarReference(std::move(base),std::move(view)));
}
const std::vector<Id>& PlanarReference::ExtensionComponents()const{return parameter_view?parameter_view->components:declared_components;}
BoundaryParameterState* PlanarReference::Parameters()const{return parameter_view?parameter_view->accepted.get():nullptr;}
std::shared_ptr<const PlanarReference> PlanarReference::WithParameterState(std::shared_ptr<BoundaryParameterState> parameters)const {
  if(!definition)throw std::invalid_argument("Unmapped reference cannot replace boundary parameters.");
  return WithParameters(definition,std::move(parameters));
}
const BoundaryPoint* PlanarReference::Parameter(Point p)const {
  if(!parameter_view)return nullptr;parameter_view->Check();const auto key=PointKey(p);
  for(double x:key)if(!std::isfinite(x))throw std::invalid_argument("Nonfinite mapped geometry lookup.");
  const auto local=parameter_view->coordinates.find(key);
  if(local!=parameter_view->coordinates.end())return &parameter_view->local.at(local->second);
  const auto pending=parameter_view->pending.find(key);if(pending!=parameter_view->pending.end())return &pending->second;
  return parameter_view->accepted->Find(p);
}
std::unique_ptr<PlanarReference> PlanarReference::PrivateParameters(const Patch& patch,AtlasBudget* reservation)const {
  if(!parameter_view)return {};parameter_view->Check();
  auto* admitted=reservation?reservation:parameter_view->reservation;
  const bool atlas=AtlasActive(*parameter_view->accepted)||!patch.coverage.faces.empty()||!patch.coverage.traces.empty();
  if(atlas) {
    if(!admitted)throw std::invalid_argument("Private atlas requires an admitted geometry reservation.");
    const auto bytes=transfer_memory::Add(admitted->caller_bytes,parameter_view->OverlayBytes(),transfer_memory::Mul(BoundaryCoverageBytes(patch.coverage),2),transfer_memory::Mul(patch.parameters.size(),512),transfer_memory::Mul(patch.surface.size(),128));
    admitted->peak_bytes=std::max(admitted->peak_bytes,bytes);if(bytes==SIZE_MAX||bytes>admitted->maximum_bytes)throw std::length_error("Private atlas overlay exceeds storage admission.");
  }
  auto view=std::make_shared<ParameterView>(*parameter_view);view->private_view=true;view->revision=view->accepted->Revision();view->reservation=admitted;
  for(auto face:patch.surface){face.v=FaceOrder(face.v);view->surface.insert_or_assign(FaceKey(face.v),face);}
  for(const auto& row:patch.coverage.faces)view->coverage.faces.insert_or_assign(row.first,row.second);
  for(const auto& row:patch.coverage.traces)view->coverage.traces.insert_or_assign(row.first,row.second);
  for(const auto& row:patch.parameters) {
    if(row.first!=row.second.node.id)throw std::invalid_argument("Private boundary parameter identity mismatch.");
    const auto accepted=view->accepted->Find(row.first);if(!accepted||!Equal(*accepted,row.second))view->Insert(row.second);
  }
  return std::unique_ptr<PlanarReference>(new PlanarReference(definition,std::move(view)));
}
std::unique_ptr<PlanarReference> PlanarReference::WithAtlasReservation(AtlasBudget& reservation)const {
  auto scoped=PrivateParameters(Patch{},&reservation);
  // A work scope over accepted state follows publications; real proposal overlays remain revision-pinned.
  if(scoped)scoped->parameter_view->private_view=parameter_view->private_view;
  return scoped;
}
AtlasBudget* PlanarReference::AtlasReservation()const{return parameter_view?parameter_view->reservation:nullptr;}
void PlanarReference::ExportPointParameters(Patch& patch)const {
  if(!parameter_view)return;parameter_view->Check();
  std::set<Id> used;BoundaryIds(patch,used);BoundaryPoints prepared;std::map<Id,Point> actual;
  for(const auto& c:patch.cells)for(auto n:c.v)if(used.count(n.id))actual.emplace(n.id,n.p);
  for(Id id:used) {
    const auto explicit_point=patch.parameters.find(id);
    const auto p=explicit_point==patch.parameters.end()?Parameter(actual.at(id)):&explicit_point->second;
    if(!p)throw std::invalid_argument("New physical point has no forward parameter binding.");
    if(PointKey(p->node.p)!=PointKey(actual.at(id)) || !parameter_view->accepted->Geometry().Admissible(p->binding,actual.at(id)))
      throw std::invalid_argument("Private forward binding differs from realized geometry.");
    if(p->node.id!=id && p->node.id!=UINT64_MAX)throw std::invalid_argument("Mapped physical coordinate has another current identity.");
    BoundaryPoint record=*p;record.node={id,actual.at(id)};prepared.emplace(id,record);
  }
  patch.parameters.swap(prepared);
}
bool PlanarReference::HasExplicitCoverage(const Patch& patch)const {
  if(!parameter_view)return false;parameter_view->Check();
  if(!patch.coverage.faces.empty()||!patch.coverage.traces.empty())return true;
  const auto& accepted=parameter_view->accepted->Coverage();
  for(const auto& face:patch.surface) {
    const auto key=FaceOrder(face.v);
    if(parameter_view->coverage.faces.count(key)||accepted.faces.count(key))return true;
    for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());
      if(parameter_view->coverage.traces.count(edge)||accepted.traces.count(edge))return true;}
  }
  return false;
}
bool PlanarReference::HasAcceptedFaceCertificate(const SurfaceFace& face,const std::array<BoundaryPoint,3>& points,
    const BoundaryCoverage& supplied)const {
  if(!parameter_view)return false;parameter_view->Check();
  const auto& state=*parameter_view->accepted;if(!AtlasActive(state))return false;const auto key=FaceOrder(face.v);
  const auto found=state.Surface().find(FaceKey(face.v));
  if(found==state.Surface().end()||found->second.v!=key||found->second.facet!=face.facet)return false;
  const auto local_face=parameter_view->surface.find(FaceKey(face.v));
  if(local_face!=parameter_view->surface.end()&&(local_face->second.v!=key||local_face->second.facet!=face.facet))return false;
  if(parameter_view->coverage.faces.count(key)||supplied.faces.count(key))return false;
  for(unsigned k=0;k<3;++k) {
    const auto* accepted=state.Find(face.v[k]);
    if(!accepted||!Equal(points[k],*accepted))return false;
    Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());
    if(parameter_view->coverage.traces.count(edge)||supplied.traces.count(edge))return false;
    const auto local=parameter_view->local.find(face.v[k]);
    if(local!=parameter_view->local.end()&&!Equal(local->second,*accepted))return false;
  }
  return true;
}
void PlanarReference::ExportParameters(Patch& patch,AtlasBudget* reservation)const {
  if(!parameter_view)return;parameter_view->Check();ExportPointParameters(patch);
  std::set<Face> faces;std::set<Edge> edges;
  for(const auto& face:patch.surface){faces.insert(FaceOrder(face.v));for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());edges.insert(edge);}}
  for(const auto& row:patch.coverage.faces)if(!faces.count(row.first))throw std::invalid_argument("Private atlas has an extraneous face descriptor.");
  for(const auto& row:patch.coverage.traces)if(!edges.count(row.first))throw std::invalid_argument("Private atlas has an extraneous edge descriptor.");
  auto copy=[&](const BoundaryCoverage& source){
    for(const auto& face:faces){const auto record=source.faces.find(face);if(record!=source.faces.end())patch.coverage.faces.emplace(*record);}
    for(const auto& edge:edges){const auto record=source.traces.find(edge);if(record!=source.traces.end())patch.coverage.traces.emplace(*record);}
  };
  copy(parameter_view->coverage);copy(parameter_view->accepted->Coverage());
  if(AtlasActive(*parameter_view->accepted)||!patch.coverage.faces.empty()||!patch.coverage.traces.empty()) {
    auto overlay=PrivateParameters(patch,reservation);auto& budget=*(reservation?reservation:parameter_view->reservation);std::string reason;
    for(const auto& face:patch.surface){FaceCoverage cover;std::array<EdgeTrace,3> traces;std::array<SurfaceBinding,3> binding;std::array<Point,3> actual;
      std::array<BoundaryPoint,3> points;for(unsigned k=0;k<3;++k)points[k]=patch.parameters.at(face.v[k]);
      if(overlay->HasAcceptedFaceCertificate(face,points,patch.coverage))continue;
      if(!overlay->ResolveCoverage(face,cover,budget,reason))throw std::invalid_argument(reason);
      for(unsigned k=0;k<3;++k){const auto& point=patch.parameters.at(face.v[k]);binding[k]=point.binding;actual[k]=point.node.p;}
      FaceCoverage single;std::string strict_reason;
      const bool cross=!parameter_view->accepted->Geometry().ResolveSingleChart(face.facet,binding,single,budget,strict_reason);
      for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};if(!overlay->ResolveTrace(edge,face,traces[k],budget,reason))throw std::invalid_argument(reason);
        if(cross){auto key=edge;std::sort(key.begin(),key.end());patch.coverage.traces.emplace(key,traces[k]);}
        if(edge[0]>edge[1])traces[k]=ReverseTrace(traces[k],budget);}
      CheckPolicyCoverage(face.facet,cover,parameter_view->accepted->Mobility(),parameter_view->accepted->Geometry(),budget,reason);
      if(!parameter_view->accepted->Geometry().ValidateCoverage(face.facet,binding,actual,cover,traces,budget,reason))throw std::invalid_argument(reason);
    }
  }
}
const MobilityPolicy* PlanarReference::Mobility()const{return parameter_view&&parameter_view->accepted->Mobility().identity?&parameter_view->accepted->Mobility():nullptr;}
bool PlanarReference::ResolveCoverage(const SurfaceFace& face,FaceCoverage& out,AtlasBudget& budget,std::string& reason)const {
  try {
    if(!parameter_view)throw std::invalid_argument("Unmapped reference has no parameter coverage.");parameter_view->Check();
    const auto key=FaceOrder(face.v);const auto local=parameter_view->coverage.faces.find(key);
    if(local!=parameter_view->coverage.faces.end()){unsigned rotation=0;while(key[rotation]!=face.v[0])++rotation;out=RotateCoverage(local->second,rotation,budget);return true;}
    const auto accepted=parameter_view->accepted->Surface().find(FaceKey(face.v));
    if(accepted!=parameter_view->accepted->Surface().end()&&accepted->second.v==key&&accepted->second.facet==face.facet)
      return parameter_view->accepted->ResolveCoverage(face,out,budget,reason);
    std::array<SurfaceBinding,3> binding;
    for(unsigned k=0;k<3;++k){const auto local=parameter_view->local.find(face.v[k]);const auto p=local==parameter_view->local.end()?parameter_view->accepted->Find(face.v[k]):&local->second;
      if(!p)throw std::invalid_argument("Candidate physical face has no forward binding.");binding[k]=p->binding;}
    return parameter_view->accepted->Geometry().ResolveSingleChart(face.facet,binding,out,budget,reason);
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool PlanarReference::ResolveTrace(Edge edge,const SurfaceFace& face,EdgeTrace& out,AtlasBudget& budget,std::string& reason)const {
  try {
    if(!parameter_view)throw std::invalid_argument("Unmapped reference has no parameter traces.");parameter_view->Check();std::sort(edge.begin(),edge.end());
    const auto local=parameter_view->coverage.traces.find(edge);if(local!=parameter_view->coverage.traces.end()){out=local->second;return true;}
    const auto accepted=parameter_view->accepted->Coverage().traces.find(edge);if(accepted!=parameter_view->accepted->Coverage().traces.end()){out=accepted->second;return true;}
    const auto incident=parameter_view->accepted->IncidentFace(edge);
    if(incident)return parameter_view->accepted->ResolveTrace(edge,*incident,out,budget,reason);
    std::array<SurfaceBinding,3> binding;
    for(unsigned k=0;k<3;++k){const auto local=parameter_view->local.find(face.v[k]);const auto p=local==parameter_view->local.end()?parameter_view->accepted->Find(face.v[k]):&local->second;
      if(!p)throw std::invalid_argument("Candidate physical trace has no forward binding.");binding[k]=p->binding;}
    FaceCoverage single;std::array<EdgeTrace,3> direct;
    if(!parameter_view->accepted->Geometry().ResolveSingleChart(face.facet,binding,single,budget,reason)||!parameter_view->accepted->Geometry().CoverageTraces(single,direct,budget,reason))return false;
    for(unsigned k=0;k<3;++k){Edge candidate{face.v[k],face.v[(k+1)%3]},key=candidate;std::sort(key.begin(),key.end());if(key==edge){out=candidate[0]<candidate[1]?direct[k]:ReverseTrace(direct[k],budget);return true;}}
    throw std::invalid_argument("Candidate trace is not an edge of its physical face.");
  }catch(const std::exception& e){reason=e.what();return false;}
}
SU2NativeGeometry::SurfaceBinding PlanarReference::ProposedBinding(Point p,const std::set<Id>& incident,
    const SU2NativeGeometry::SurfaceBinding* previous)const {
  if(!parameter_view || !parameter_view->private_view)throw std::invalid_argument("Mapped proposals require a private geometry context.");
  parameter_view->Check();for(double x:PointKey(p))if(!std::isfinite(x))throw std::invalid_argument("Nonfinite mapped point proposal.");
  const auto& geometry=parameter_view->accepted->Geometry();
  if(geometry.ReferenceComponents()) {
    std::set<Id> roots;for(Id f:incident)roots.insert(geometry.Canonical(f));
    if(roots.empty())throw std::invalid_argument("Mapped proposal has no incident component.");
    auto supported=[&](const SU2NativeGeometry::SurfaceBinding& b){for(Id f:roots)if(!geometry.Supports(f,b))return false;return true;};
    auto edge_proposal=[&](Edge edge) {
      std::sort(edge.begin(),edge.end());const auto a=geometry.Ideal(geometry.Vertex(edge[0])).origin,b=geometry.Ideal(geometry.Vertex(edge[1])).origin;
      const double x[3]={a.x,a.y,a.z},y[3]={b.x,b.y,b.z},q[3]={p.x,p.y,p.z};long double numerator=0,square=0;
      for(unsigned k=0;k<3;++k){const auto d=static_cast<long double>(y[k])-x[k];numerator+=(static_cast<long double>(q[k])-x[k])*d;square+=d*d;}
      if(!(square>0&&std::isfinite(square)))throw std::invalid_argument("Unresolved component feature parameter.");
      return geometry.EdgePoint(edge,double(std::clamp(numerator/square,0.L,1.L)));
    };
    if(previous&&geometry.PhysicalSupport(*previous)) {
      if(previous->dimension==0){if(!supported(*previous))throw std::invalid_argument("Physical corner conflicts with incident components.");return *previous;}
      bool found=false;Edge edge{};
      geometry.ForFacets(*previous,[&](Id f){for(unsigned k=0;k<3&&!found;++k){Edge e{Get(f).v[k].id,Get(f).v[(k+1)%3].id};std::sort(e.begin(),e.end());
        if(geometry.EdgePoint(e,.5).feature==previous->feature){edge=e;found=true;}}});
      if(!found)throw std::invalid_argument("Physical component feature is missing.");const auto b=edge_proposal(edge);
      if(!supported(b))throw std::invalid_argument("Moved feature conflicts with incident components.");return b;
    }
    if(roots.size()>1) {
      bool found=false;SU2NativeGeometry::SurfaceBinding best;long double distance=INFINITY;
      for(auto edge:geometry.ComponentBorder(*roots.begin())) {
        auto ordered=edge;std::sort(ordered.begin(),ordered.end());if(!supported(geometry.EdgePoint(ordered,.5)))continue;
        const auto b=edge_proposal(edge);const auto q=geometry.Realize(b);
        const long double d=std::abs(static_cast<long double>(p.x)-q.x)+std::abs(static_cast<long double>(p.y)-q.y)+std::abs(static_cast<long double>(p.z)-q.z);
        if(!found||d<distance){best=b;distance=d;found=true;}
      }
      if(!found)throw std::invalid_argument("Declared components have no shared physical feature.");return best;
    }
    const Id root=*roots.begin();const unsigned axis=geometry.ComponentAxis(root),x=axis==0?1:0,y=axis==2?1:2;
    const double q[3]={p.x,p.y,p.z};
    for(Id id:geometry.CandidateFacets(root,p)) {
      const auto& f=Get(id);const double a[3]={f.v[0].p.x,f.v[0].p.y,f.v[0].p.z},b[3]={f.v[1].p.x,f.v[1].p.y,f.v[1].p.z},c[3]={f.v[2].p.x,f.v[2].p.y,f.v[2].p.z};
      const auto bx=static_cast<long double>(b[x])-a[x],by=static_cast<long double>(b[y])-a[y],cx=static_cast<long double>(c[x])-a[x],cy=static_cast<long double>(c[y])-a[y];
      const auto left=bx*cy,right=by*cx,det=left-right;
      if(!std::isfinite(det)||std::abs(det)<=64*std::numeric_limits<long double>::epsilon()*(std::abs(left)+std::abs(right)))continue;
      const auto px=static_cast<long double>(q[x])-a[x],py=static_cast<long double>(q[y])-a[y];
      const double u=double((px*cy-py*cx)/det),v=double((bx*py-by*px)/det);
      if(!(std::isfinite(u)&&std::isfinite(v)&&u>=0&&v>=0))continue;
      const auto remainder=Orientation({1,0,0},{0,1,0},{u,v,0},{0,0,1});if(remainder<0)continue;
      if(u>0&&v>0&&remainder>0)return geometry.FacetPoint(id,u,v);
      Edge edge=u==0?Edge{f.v[0].id,f.v[2].id}:v==0?Edge{f.v[0].id,f.v[1].id}:Edge{f.v[1].id,f.v[2].id};
      return edge_proposal(edge);
    }
    throw std::invalid_argument("Mapped proposal leaves its finite component or has unresolved support.");
  }
  std::set<Id> common;bool first=true;
  for(Id id:incident) {
    std::set<Id> vertices;for(auto n:Get(id).v)vertices.insert(n.id);
    if(first){common=std::move(vertices);first=false;}else {std::set<Id> next;std::set_intersection(common.begin(),common.end(),vertices.begin(),vertices.end(),std::inserter(next,next.end()));common.swap(next);}
  }
  if(first||common.empty())throw std::invalid_argument("Mapped point has conflicting/disjoint physical supports.");
  unsigned dimension=common.size()==1?0:common.size()==2?1:2;
  if(previous)dimension=previous->dimension;
  if(dimension==0) {
    const Id id=previous?previous->feature:*common.begin();const auto b=geometry.Vertex(id);
    for(Id f:incident)if(!geometry.Supports(f,b))throw std::invalid_argument("Moved corner conflicts with incident geometry.");return b;
  }
  auto wide=[](Point q){return std::array<long double,3>{q.x,q.y,q.z};};
  if(dimension==1) {
    Edge edge{};
    if(previous) {
      bool found=false;
      geometry.ForFacets(*previous,[&](Id f){if(found)return;const auto& face=Get(f);for(unsigned k=0;k<3;++k){Edge e{face.v[k].id,face.v[(k+1)%3].id};std::sort(e.begin(),e.end());
        const auto probe=geometry.EdgePoint(e,.5);if(probe.dimension==1&&probe.feature==previous->feature){edge=e;found=true;break;}}});
      if(!found)throw std::invalid_argument("Missing mapped original edge support.");
    } else {auto i=common.begin();edge={*i,*std::next(i)};}
    Point a{},b{};bool has_a=false,has_b=false;
    geometry.ForFacets(geometry.Vertex(edge[0]),[&](Id f){for(auto n:Get(f).v){if(n.id==edge[0]){a=n.p;has_a=true;}if(n.id==edge[1]){b=n.p;has_b=true;}}});
    if(!(has_a&&has_b))throw std::invalid_argument("Missing mapped feature endpoints.");
    const auto x=wide(a),y=wide(b),q=wide(p);long double numerator=0,square=0;
    for(unsigned k=0;k<3;++k){const auto d=y[k]-x[k];numerator+=(q[k]-x[k])*d;square+=d*d;}
    if(!(square>0&&std::isfinite(square)))throw std::invalid_argument("Unresolved mapped feature parameter proposal.");
    return geometry.EdgePoint(edge,double(std::clamp(numerator/square,0.L,1.L)));
  }
  const Id id=previous?previous->feature:*incident.begin();const auto& f=Get(id);
  const auto a=wide(f.v[0].p),b=wide(f.v[1].p),c=wide(f.v[2].p),q=wide(p);
  long double largest=0,determinant=0;unsigned x=0,y=1;
  for(unsigned k=0;k<3;++k) {
    const unsigned i=k==0?1:0,j=k==2?1:2;const auto left=(b[i]-a[i])*(c[j]-a[j]),right=(b[j]-a[j])*(c[i]-a[i]),d=left-right;
    if(std::abs(d)>largest && std::abs(d)>64*std::numeric_limits<long double>::epsilon()*(std::abs(left)+std::abs(right))){largest=std::abs(d);determinant=d;x=i;y=j;}
  }
  if(!(largest>0&&std::isfinite(determinant)))throw std::invalid_argument("Unresolved mapped facet parameter proposal.");
  const double u=double(((q[x]-a[x])*(c[y]-a[y])-(q[y]-a[y])*(c[x]-a[x]))/determinant);
  const double v=double(((b[x]-a[x])*(q[y]-a[y])-(b[y]-a[y])*(q[x]-a[x]))/determinant);
  const auto remainder=Orientation({1,0,0},{0,1,0},{u,v,0},{0,0,1});
  if(std::isfinite(u)&&std::isfinite(v)&&u>=0&&v>=0&&remainder>=0&&(u==0||v==0||remainder==0)) {
    // FacetPoint represents only the open facet; preserve canonical boundary strata.
    Edge edge=u==0?Edge{f.v[0].id,f.v[2].id}:v==0?Edge{f.v[0].id,f.v[1].id}:Edge{f.v[1].id,f.v[2].id};
    double t=u==0?v:v==0?u:v;
    if(edge[0]>edge[1]){std::swap(edge[0],edge[1]);t=1-t;}
    return geometry.EdgePoint(edge,t);
  }
  return geometry.FacetPoint(id,u,v);
}
Node PlanarReference::BindNewPoint(Node point,const std::set<Id>& incident)const {
  const auto binding=ProposedBinding(point.p,incident);point.p=parameter_view->accepted->Geometry().Realize(binding);
  parameter_view->Insert({point,binding});return point;
}
Node PlanarReference::BindMovedPoint(Id old,Node point,const std::set<Id>& incident)const {
  const auto local=parameter_view->local.find(old);const auto previous=local==parameter_view->local.end()?parameter_view->accepted->Find(old):&local->second;
  if(!previous)throw std::invalid_argument("Moved point has no accepted forward binding.");
  const auto binding=ProposedBinding(point.p,incident,&previous->binding);point.p=parameter_view->accepted->Geometry().Realize(binding);
  parameter_view->Insert({point,binding});return point;
}
bool PlanarReference::ParameterContains(Id facet,Point p)const {
  const auto q=Parameter(p);const auto& g=parameter_view->accepted->Geometry();return q&&g.Supports(facet,q->binding)&&g.Admissible(q->binding,p);
}
bool PlanarReference::ParameterContainsFace(Id facet,const std::array<Point,3>& p,KernelStats* stats)const {
  std::array<SU2NativeGeometry::SurfaceBinding,3> b;std::array<bool,3> pending{};SurfaceFace face;face.facet=facet;
  for(unsigned k=0;k<3;++k){const auto q=Parameter(p[k]);if(!q)return false;b[k]=q->binding;face.v[k]=q->node.id;
    const auto found=parameter_view->pending.find(PointKey(p[k]));pending[k]=found!=parameter_view->pending.end()&&q==&found->second;}
  const auto& g=parameter_view->accepted->Geometry();
  std::array<BoundaryPoint,3> points;for(unsigned k=0;k<3;++k)points[k]=*Parameter(p[k]);
  if(HasAcceptedFaceCertificate(face,points,BoundaryCoverage{}))return true;
  if(!AtlasActive(*parameter_view->accepted)&&!parameter_view->private_view&&parameter_view->local.empty()&&parameter_view->pending.empty()&&
      parameter_view->surface.empty()&&parameter_view->coverage.faces.empty()&&parameter_view->coverage.traces.empty()) {
    const auto found=parameter_view->accepted->Surface().find(FaceKey(face.v));
    if(found!=parameter_view->accepted->Surface().end()&&found->second.v==FaceOrder(face.v)&&found->second.facet==facet)return true;
  }
  if(!AtlasActive(*parameter_view->accepted)&&parameter_view->coverage.faces.empty()&&parameter_view->coverage.traces.empty())return g.ContainsFace(facet,b,p,stats);
  if(!parameter_view->reservation)return false;auto& budget=*parameter_view->reservation;std::string reason;FaceCoverage cover;std::array<EdgeTrace,3> traces;
  const bool transient=std::any_of(pending.begin(),pending.end(),[](bool p){return p;});
  if(transient) {
    // A projection probe has associations but no publishable mesh face identity.
    if(!g.ResolveSingleChart(facet,b,cover,budget,reason)||!g.CoverageTraces(cover,traces,budget,reason))return false;
  } else if(!ResolveCoverage(face,cover,budget,reason))return false;
  for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};
    if(pending[k]||pending[(k+1)%3])continue;
    if(!ResolveTrace(edge,face,traces[k],budget,reason))return false;if(edge[0]>edge[1])traces[k]=ReverseTrace(traces[k],budget);}
  try{CheckPolicyCoverage(facet,cover,parameter_view->accepted->Mobility(),g,budget,reason);}catch(const std::exception&){return false;}
  return g.ValidateCoverage(facet,b,p,cover,traces,budget,reason);
}
bool PlanarReference::ParameterFindFace(int marker,const std::array<Point,3>& p,Id& facet,KernelStats* stats)const {
  std::array<const BoundaryPoint*,3> b{};for(unsigned k=0;k<3;++k){b[k]=Parameter(p[k]);if(!b[k])return false;}
  Face face{b[0]->node.id,b[1]->node.id,b[2]->node.id};const auto key=FaceKey(face);
  const auto local=parameter_view->surface.find(key);const auto accepted=parameter_view->accepted->Surface().find(key);
  const auto known=local!=parameter_view->surface.end()?&local->second:accepted!=parameter_view->accepted->Surface().end()?&accepted->second:nullptr;
  if(known&&known->v==FaceOrder(face)&&Get(known->facet).marker==marker&&ParameterContainsFace(known->facet,p,stats)){facet=known->facet;return true;}
  const auto chosen=*std::max_element(b.begin(),b.end(),[](auto a,auto b){return a->binding.dimension<b->binding.dimension;});
  std::vector<Id> candidates;parameter_view->accepted->Geometry().ForFacets(chosen->binding,[&](Id id){if(Get(id).marker==marker)candidates.push_back(parameter_view->accepted->Geometry().Canonical(id));});
  std::sort(candidates.begin(),candidates.end());candidates.erase(std::unique(candidates.begin(),candidates.end()),candidates.end());
  for(Id id:candidates)if(ParameterContainsFace(id,p,stats)){facet=id;return true;}return false;
}
Point PlanarReference::ParameterProject(Id facet,Point p)const {
  if(ParameterContains(facet,p))return p;
  const auto binding=ProposedBinding(p,{facet});const auto q=parameter_view->accepted->Geometry().Realize(binding);
  parameter_view->pending[PointKey(q)]={{UINT64_MAX,q},binding};parameter_view->Bound();return q;
}
bool PlanarReference::ParameterOnEdge(Id facet,Point a,Point b,Point p,KernelStats* stats)const {
  if(!(ParameterContains(facet,a)&&ParameterContains(facet,b)&&ParameterContains(facet,p)))return false;
  const auto& g=parameter_view->accepted->Geometry();
  if(g.ReferenceComponents())return ContainsSegment(facet,a,b,stats)&&ParametricOnProjectedSegment(g.Ideal(Parameter(a)->binding),g.Ideal(Parameter(b)->binding),g.Ideal(Parameter(p)->binding),g.ComponentAxis(facet),stats);
  return ParametricOnSegment(g.Ideal(Parameter(a)->binding),g.Ideal(Parameter(b)->binding),g.Ideal(Parameter(p)->binding),stats);
}
double PlanarReference::ParameterError(Id facet,const std::array<Point,3>& p)const {
  double maximum=0;const auto& g=parameter_view->accepted->Geometry();std::array<SU2NativeGeometry::SurfaceBinding,3> binding;
  for(unsigned k=0;k<3;++k){const auto b=Parameter(p[k]);if(!b)throw std::invalid_argument("Unbound mapped surface error query.");binding[k]=b->binding;
    const auto plane=Affine().find(facet);if(plane!=Affine().end())maximum=std::max(maximum,SU2NativeGeometry::AffineChart(plane->second).ErrorBound(p[k]));}
  if(!AtlasActive(*parameter_view->accepted)&&parameter_view->coverage.faces.empty()&&parameter_view->coverage.traces.empty())return std::max(maximum,g.SurfaceError(facet,binding,p));
  SurfaceFace face;face.facet=facet;bool transient=false;
  for(unsigned k=0;k<3;++k){const auto* point=Parameter(p[k]);face.v[k]=point->node.id;
    const auto found=parameter_view->pending.find(PointKey(p[k]));transient=transient||(found!=parameter_view->pending.end()&&point==&found->second);}
  if(!parameter_view->reservation)throw std::invalid_argument("Mapped atlas error query lacks admitted geometry work.");
  auto& budget=*parameter_view->reservation;std::string reason;FaceCoverage cover;
  // Projection probes have forward bindings, not published mesh identities or face records.
  const bool resolved=transient?g.ResolveSingleChart(facet,binding,cover,budget,reason):ResolveCoverage(face,cover,budget,reason);
  if(!resolved)throw std::invalid_argument(reason);
  CheckPolicyCoverage(facet,cover,parameter_view->accepted->Mobility(),g,budget,reason);
  return std::max(maximum,g.SurfaceError(cover,p,budget));
}
size_t PlanarReference::ParameterWorkspaceBytes()const {
  if(!parameter_view)return 0;
  return transfer_memory::Add(parameter_view->OverlayBytes(),parameter_view->private_view?0:parameter_view->accepted->ResidentBytes(),
      parameter_view->private_view?0:parameter_view->accepted->Geometry().ResidentBytes(),
      parameter_view->accepted->Geometry().CurvedQueryWorkspaceBytes(SU2NativeGeometry::MappedFacetReference::ClearanceWork));
}
std::shared_ptr<const PlanarReference> PromoteFacetedReference(std::shared_ptr<const PlanarReference> reference,
    const std::vector<Facet>& facets,double relative_error,double& declared_error,size_t ceiling) {
  if(!reference||facets.empty())throw std::invalid_argument("Native3D faceted promotion needs a reference and its facets.");
  if(!(relative_error>0)||!std::isfinite(relative_error))throw std::invalid_argument("Native3D faceted promotion error must be finite and positive.");
  long double scale=0;
  for(const auto& f:facets)for(const auto& x:f.v)scale=std::max({scale,std::abs((long double)x.p.x),std::abs((long double)x.p.y),std::abs((long double)x.p.z)});
  declared_error=double(relative_error*std::max(1.L,scale));
  std::map<Id,double> errors;for(const auto& f:facets)errors.emplace(f.id,declared_error);
  auto authority=std::make_shared<SU2NativeGeometry::MappedFacetReference>(reference,errors,7,ceiling,true);
  BoundaryPoints points;std::vector<SurfaceFace> skin;
  for(const auto& f:facets){skin.push_back({{f.v[0].id,f.v[1].id,f.v[2].id},f.id});for(const auto& x:f.v)points.emplace(x.id,BoundaryPoint{x,authority->Vertex(x.id)});}
  return PlanarReference::WithParameters(reference,std::make_shared<BoundaryParameterState>(authority,points,skin,1,ceiling));
}
std::shared_ptr<const PlanarReference> FitPlanarMarkers(std::shared_ptr<const PlanarReference> reference,
    const std::vector<Facet>& facets,const std::set<int>& planar_markers,double relative_tolerance,size_t& fitted,size_t ceiling) {
  fitted=0;
  if(!reference||facets.empty()||planar_markers.empty())return reference;
  if(!(relative_tolerance>0)||!std::isfinite(relative_tolerance))throw std::invalid_argument("Native3D planar marker fit tolerance must be finite and positive.");
  if(reference->Mapped()||!reference->Analytic().empty()||!reference->Affine().empty())return reference;  // plain faceted only
  long double scale=0;
  for(const auto& f:facets)for(const auto& x:f.v)scale=std::max({scale,std::abs((long double)x.p.x),std::abs((long double)x.p.y),std::abs((long double)x.p.z)});
  const double tolerance=double(relative_tolerance*std::max(1.L,scale));
  // connected components (shared vertices) of the facets of each planar marker; disconnected bodies stay separate
  std::map<Id,size_t> parent;std::function<size_t(size_t)> root=[&](size_t a){while(parent.at(a)!=a)a=parent[a]=parent.at(parent.at(a));return a;};
  std::map<std::pair<int,Id>,std::vector<size_t>> byVertex;
  for(size_t i=0;i<facets.size();++i)if(planar_markers.count(facets[i].marker)){parent[i]=i;for(const auto& x:facets[i].v)byVertex[{facets[i].marker,x.id}].push_back(i);}
  for(const auto& row:byVertex)for(size_t k=1;k<row.second.size();++k){const size_t a=root(row.second[0]),b=root(row.second[k]);if(a!=b)parent[b]=a;}
  std::map<size_t,std::vector<size_t>> components;for(const auto& p:parent)components[root(p.first)].push_back(p.first);
  std::map<Id,SU2NativeGeometry::AffineAuthority> planes;Id identity=0x5D17000000000000ULL;
  for(const auto& c:components) {
    // area-weighted normal (consistently oriented skin facets) and vertex centroid, long double
    long double nx=0,ny=0,nz=0,ox=0,oy=0,oz=0;std::map<Id,Point> vertices;
    for(size_t i:c.second){const auto& f=facets[i];
      const long double ux=(long double)f.v[1].p.x-f.v[0].p.x,uy=(long double)f.v[1].p.y-f.v[0].p.y,uz=(long double)f.v[1].p.z-f.v[0].p.z;
      const long double vx=(long double)f.v[2].p.x-f.v[0].p.x,vy=(long double)f.v[2].p.y-f.v[0].p.y,vz=(long double)f.v[2].p.z-f.v[0].p.z;
      nx+=uy*vz-uz*vy;ny+=uz*vx-ux*vz;nz+=ux*vy-uy*vx;for(const auto& x:f.v)vertices.emplace(x.id,x.p);}
    const long double length=std::sqrt(nx*nx+ny*ny+nz*nz);if(!(length>0)||vertices.empty())continue;
    for(const auto& v:vertices){ox+=v.second.x;oy+=v.second.y;oz+=v.second.z;}
    const long double n=vertices.size();
    SU2NativeGeometry::AffineAuthority a;a.identity=identity++;a.version=1;a.dimension=3;
    a.origin={double(ox/n),double(oy/n),double(oz/n)};a.normal={double(nx/length),double(ny/length),double(nz/length)};a.maximum_error=tolerance;
    bool within=true;
    try{const SU2NativeGeometry::AffineChart chart(a);for(const auto& v:vertices)within=within&&chart.ErrorBound(v.second)<=tolerance;}
    catch(const std::exception&){within=false;}
    if(!within)continue;  // a curved or kinked marker patch keeps its exact faceted authority
    for(size_t i:c.second)planes.emplace(facets[i].id,a);
    ++fitted;
  }
  if(planes.empty())return reference;
  return std::make_shared<PlanarReference>(facets,ceiling,std::map<Id,AnalyticSurface>{},planes);
}
} // namespace SU2Native3D
