/*!
 * \file CNativeBoundaryParameters3D.cpp
 * \brief Boundary-only parameter deltas with exact finite-facet boundary conservation.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CNativeDistributed2D.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
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
void WritePayload(SU2NativeBoundary2D::RecordStream& out,const State& state) {
  auto version=state.Geometry().Version(),revision=state.Revision();
  uint64_t np=state.Records().size(),nf=state.Surface().size();out(version,revision,np,nf);
  for(auto row:state.Records())out(row.second.point);
  for(auto row:state.Surface())out(row.second);
}
void WriteSnapshot(SU2NativeBoundary2D::RecordStream& out,const State& state) {
  auto version=state.Geometry().Version(),revision=state.Revision();const auto& g=state.Geometry();
  uint64_t nf=g.Reference().Facets().size(),ne=g.Errors().size(),na=g.Reference().Affine().size(),np=state.Records().size(),ns=state.Surface().size();
  out(version,revision,nf,ne,na,np,ns);
  for(auto row:g.Reference().Facets())out(row.second);
  for(auto row:g.Errors()){auto id=row.first;out(id,row.second);}
  for(auto row:g.Reference().Affine()){auto id=row.first;out(id,row.second);}
  for(auto row:state.Records()){auto id=row.first;out(id,row.second);}
  for(auto row:state.Surface())out(row.second);
}
}
BoundaryParameterState::BoundaryParameterState(std::shared_ptr<const SU2NativeGeometry::MappedFacetReference> authority,
    const BoundaryPoints& points,const std::vector<SurfaceFace>& surface,uint64_t epoch,size_t ceiling,size_t caller)
    :geometry(std::move(authority)),revision(epoch) {
  if(!geometry || !revision)throw std::invalid_argument("Invalid boundary parameter authority/revision.");
  const size_t bound=transfer_memory::Add(caller,geometry->ResidentBytes(),transfer_memory::Mul(points.size(),2048),transfer_memory::Mul(surface.size(),8192));
  if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Boundary parameter initialization exceeds storage admission.");
  std::map<Id,SU2NativeGeometry::SurfaceBinding> bindings;std::map<Id,Point> actual;
  for(const auto& row:points) {
    const auto& p=row.second;
    if(row.first!=p.node.id || !geometry->Admissible(p.binding,p.node.p))throw std::invalid_argument("Invalid boundary parameter point/geometry contract.");
    if(!coordinates.emplace(PointKey(p.node.p),p.node.id).second || !supports.emplace(BindingKey(p),p.node.id).second)
      throw std::invalid_argument("Boundary parameter coordinates/support are not unique.");
    records.emplace(row.first,Entry{p,0});bindings.emplace(row.first,p.binding);actual.emplace(row.first,p.node.p);
  }
  SU2NativeGeometry::MappedSurfaceAudit audit;std::string reason;
  if(!geometry->ValidateSurface(surface,bindings,actual,audit,reason,surface.size(),ceiling))throw std::invalid_argument(reason);
  for(auto face:surface){face.v=FaceOrder(face.v);if(!this->surface.emplace(FaceKey(face.v),face).second)throw std::invalid_argument("Duplicate accepted parameter face.");
    for(Id id:face.v)++records.at(id).faces;}
}
const BoundaryPoint* BoundaryParameterState::Find(Id id)const {const auto p=records.find(id);return p==records.end()?nullptr:&p->second.point;}
const BoundaryPoint* BoundaryParameterState::Find(Point point)const {
  for(auto x:PointKey(point))if(!std::isfinite(x))throw std::invalid_argument("Nonfinite boundary parameter lookup.");
  const auto p=coordinates.find(PointKey(point));return p==coordinates.end()?nullptr:Find(p->second);
}
size_t BoundaryParameterState::ResidentBytes()const {return transfer_memory::Add(sizeof(*this),MapBytes(records),MapBytes(coordinates),MapBytes(supports),MapBytes(surface));}
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
  SU2NativeBoundary2D::RecordStream input(bytes);uint64_t version=0,epoch=0,np=0,nf=0;input(version,epoch,np,nf);
  if(!authority || version!=authority->Version() || !epoch || !np || !nf ||
      Add(32,Mul(np,72),Mul(nf,32))!=bytes.size())throw std::invalid_argument("Invalid boundary parameter payload/version/counts.");
  const size_t live=Add(caller,transfer_memory::Bytes(bytes),Mul(np,256),Mul(nf,sizeof(SurfaceFace)));
  const size_t bound=Add(live,authority->ResidentBytes(),Mul(np,2048),Mul(nf,8192));
  if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Boundary parameter payload exceeds storage admission.");
  BoundaryPoints points;std::vector<SurfaceFace> faces;faces.reserve(nf);
  for(uint64_t i=0;i<np;++i){BoundaryPoint point;input(point);
    if(!points.emplace(point.node.id,point).second)throw std::invalid_argument("Duplicate boundary parameter payload identity.");}
  for(uint64_t i=0;i<nf;++i){SurfaceFace face;input(face);faces.push_back(face);}
  if(!input.End())throw std::invalid_argument("Trailing boundary parameter payload bytes.");
  return std::make_shared<BoundaryParameterState>(std::move(authority),points,faces,epoch,ceiling,live);
}
std::shared_ptr<BoundaryParameterState> BoundaryParameterState::Rebind(const std::map<Id,Id>& numbering,
    size_t ceiling,size_t caller)const {
  using transfer_memory::Add;using transfer_memory::Mul;
  if(numbering.size()!=records.size() || revision==UINT64_MAX)throw std::invalid_argument("Incomplete boundary numbering or exhausted revision.");
  const size_t live=Add(caller,Mul(numbering.size(),256),Mul(surface.size(),sizeof(SurfaceFace)));
  const size_t bound=Add(live,geometry->ResidentBytes(),Mul(records.size(),2048),Mul(surface.size(),8192));
  if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Boundary parameter renumbering exceeds storage admission.");
  BoundaryPoints points;std::vector<SurfaceFace> faces;faces.reserve(surface.size());
  for(const auto& row:records){auto p=row.second.point;p.node.id=numbering.at(row.first);
    if(!points.emplace(p.node.id,p).second)throw std::invalid_argument("Boundary reader numbering is not one-to-one.");}
  for(const auto& row:surface){auto face=row.second;for(auto& id:face.v)id=numbering.at(id);faces.push_back(face);}
  return std::make_shared<BoundaryParameterState>(geometry,points,faces,revision+1,ceiling,live);
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
    const std::vector<BoundaryFaceChange>& faces,std::string& reason,size_t ceiling,size_t caller)const {
  Publication staged;staged.before=staged.after=revision;
  try {
    const size_t bound=transfer_memory::Add(caller,ResidentBytes(),transfer_memory::Mul(edits.size(),2048),transfer_memory::Mul(faces.size(),8192));
    if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Boundary parameter publication exceeds storage admission.");
    if(edits.empty() && faces.empty()){staged.valid=true;reason.clear();return staged;}
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
    for(const auto& change:faces)if(!change.insert) {
      const auto key=FaceKey(change.nodes);const auto old=surface.find(key);
      if(old==surface.end() || old->second.facet!=change.facet || old->second.v!=FaceOrder(change.nodes) || !staged.surface_removed.insert(key).second)
        throw std::invalid_argument("Retired physical face differs from accepted parameter topology.");
    }
    for(const auto& change:faces)if(change.insert) {
      const auto key=FaceKey(change.nodes);
      if((surface.count(key)&&!staged.surface_removed.count(key)) || !staged.surface_added.emplace(key,SurfaceFace{FaceOrder(change.nodes),change.facet}).second)
        throw std::invalid_argument("New parameter topology repeats an accepted face.");
    }
    auto point=[&](Id id,bool added)->const BoundaryPoint& {
      if(added){const auto p=staged.added.find(id);if(p!=staged.added.end())return p->second.point;
        if(staged.removed.count(id))throw std::invalid_argument("New physical face retains an erased boundary point.");}
      const auto p=Find(id);if(!p)throw std::invalid_argument("Physical face has no boundary parameter binding.");return *p;
    };
    std::map<Id,int64_t> degree;
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
      if(!geometry->FeatureSegment(any(a).binding,any(b).binding,feature,ta,tb))
        throw std::invalid_argument("Parameter replacement changes its retained facet interface.");
      intervals[{facet,feature,ta}]+=row.second;intervals[{facet,feature,tb}]-=row.second;
    }
    for(const auto& row:intervals)if(row.second)throw std::invalid_argument("Parameter replacement changes finite feature coverage.");
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
  revision=staged.after;
}
} // namespace SU2Native3D

namespace SU2Native3D {
struct PlanarReference::ParameterView {
  std::shared_ptr<BoundaryParameterState> accepted;
  BoundaryPoints local;
  std::map<BoundaryParameterState::Coordinate,Id> coordinates;
  std::map<BoundaryParameterState::Coordinate,BoundaryPoint> pending;
  std::vector<Id> components;
  uint64_t revision=0;
  bool private_view=false;
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
std::unique_ptr<PlanarReference> PlanarReference::PrivateParameters(const Patch& patch)const {
  if(!parameter_view)return {};parameter_view->Check();
  auto view=std::make_shared<ParameterView>(*parameter_view);view->private_view=true;view->revision=view->accepted->Revision();
  for(const auto& row:patch.parameters) {
    if(row.first!=row.second.node.id)throw std::invalid_argument("Private boundary parameter identity mismatch.");
    const auto accepted=view->accepted->Find(row.first);if(!accepted||!Equal(*accepted,row.second))view->Insert(row.second);
  }
  return std::unique_ptr<PlanarReference>(new PlanarReference(definition,std::move(view)));
}
void PlanarReference::ExportParameters(Patch& patch)const {
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
SU2NativeGeometry::SurfaceBinding PlanarReference::ProposedBinding(Point p,const std::set<Id>& incident,
    const SU2NativeGeometry::SurfaceBinding* previous)const {
  if(!parameter_view || !parameter_view->private_view)throw std::invalid_argument("Mapped proposals require a private geometry context.");
  parameter_view->Check();for(double x:PointKey(p))if(!std::isfinite(x))throw std::invalid_argument("Nonfinite mapped point proposal.");
  const auto& geometry=parameter_view->accepted->Geometry();std::set<Id> common;bool first=true;
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
  std::array<SU2NativeGeometry::SurfaceBinding,3> b;
  for(unsigned k=0;k<3;++k){const auto q=Parameter(p[k]);if(!q)return false;b[k]=q->binding;}
  return parameter_view->accepted->Geometry().ContainsFace(facet,b,p,stats);
}
bool PlanarReference::ParameterFindFace(int marker,const std::array<Point,3>& p,Id& facet,KernelStats* stats)const {
  std::array<const BoundaryPoint*,3> b{};for(unsigned k=0;k<3;++k){b[k]=Parameter(p[k]);if(!b[k])return false;}
  const auto chosen=*std::max_element(b.begin(),b.end(),[](auto a,auto b){return a->binding.dimension<b->binding.dimension;});
  std::vector<Id> candidates;parameter_view->accepted->Geometry().ForFacets(chosen->binding,[&](Id id){if(Get(id).marker==marker)candidates.push_back(id);});
  std::sort(candidates.begin(),candidates.end());
  for(Id id:candidates)if(ParameterContainsFace(id,p,stats)){facet=id;return true;}return false;
}
Point PlanarReference::ParameterProject(Id facet,Point p)const {
  if(ParameterContains(facet,p))return p;
  const auto binding=ProposedBinding(p,{facet});const auto q=parameter_view->accepted->Geometry().Realize(binding);
  parameter_view->pending[PointKey(q)]={{UINT64_MAX,q},binding};parameter_view->Bound();return q;
}
bool PlanarReference::ParameterOnEdge(Id facet,Point a,Point b,Point p,KernelStats* stats)const {
  if(!(ParameterContains(facet,a)&&ParameterContains(facet,b)&&ParameterContains(facet,p)))return false;
  const auto& g=parameter_view->accepted->Geometry();return ParametricOnSegment(g.Ideal(Parameter(a)->binding),g.Ideal(Parameter(b)->binding),g.Ideal(Parameter(p)->binding),stats);
}
double PlanarReference::ParameterError(Id facet,const std::array<Point,3>& p)const {
  double maximum=0;const auto& g=parameter_view->accepted->Geometry();
  for(Point q:p){const auto b=Parameter(q);if(!b)throw std::invalid_argument("Unbound mapped surface error query.");maximum=std::max(maximum,g.ErrorBound(b->binding,q));
    const auto plane=Affine().find(facet);if(plane!=Affine().end())maximum=std::max(maximum,SU2NativeGeometry::AffineChart(plane->second).ErrorBound(q));}
  return maximum;
}
size_t PlanarReference::ParameterWorkspaceBytes()const {
  if(!parameter_view)return 0;
  return transfer_memory::Add(sizeof(ParameterView),MapBytes(parameter_view->local),MapBytes(parameter_view->coordinates),MapBytes(parameter_view->pending),
      transfer_memory::Bytes(parameter_view->components),parameter_view->private_view?0:parameter_view->accepted->ResidentBytes(),
      parameter_view->private_view?0:parameter_view->accepted->Geometry().ResidentBytes());
}
} // namespace SU2Native3D
