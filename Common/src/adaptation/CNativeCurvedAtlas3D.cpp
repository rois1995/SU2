/*!
 * \file CNativeCurvedAtlas3D.cpp
 * \brief Bounded convex PL charts and exact common-refinement remapping.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../include/adaptation/CNativeReferenceTriangulation3D.hpp"
#include "../../include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../include/linear_algebra/CMatrixInverse.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <set>
#include <stdexcept>

namespace SU2NativeGeometry {
namespace {
using namespace SU2Native3D;
using Key=std::vector<uint64_t>;
size_t Plus(size_t a,size_t b){if(b>SIZE_MAX-a)throw std::length_error("Curved atlas size overflow.");return a+b;}
size_t Times(size_t a,size_t b){if(a && b>SIZE_MAX/a)throw std::length_error("Curved atlas size overflow.");return a*b;}
size_t RatioBytes(const ExactRatio& a){return Times(Plus(a.numerator.capacity(),a.denominator.capacity()),sizeof(uint32_t));}
size_t BaryBytes(const ExactBary& a){return Plus(RatioBytes(a.u),RatioBytes(a.v));}
size_t FaceBytes(const FaceCoverage& cover){size_t bytes=Times(cover.pieces.capacity(),sizeof(CoveragePiece));
  for(const auto& p:cover.pieces)for(unsigned k=0;k<3;++k)bytes=Plus(bytes,Plus(BaryBytes(p.reference[k]),BaryBytes(p.active[k])));return bytes;}
size_t TraceBytes(const EdgeTrace& trace){size_t bytes=Times(trace.pieces.capacity(),sizeof(TracePiece));
  for(const auto& p:trace.pieces)bytes=Plus(bytes,Plus(Plus(BaryBytes(p.first),BaryBytes(p.last)),RatioBytes(p.end)));return bytes;}
size_t TraceBytes(const std::array<EdgeTrace,3>& traces){size_t bytes=0;for(const auto& t:traces)bytes=Plus(bytes,TraceBytes(t));return bytes;}
Face Ordered(Face f){std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());return f;}
Edge Sorted(Id a,Id b){return a<b?Edge{a,b}:Edge{b,a};}
struct Vertex {Key key;ExactBary domain;};
struct Triangle {Id facet=0;std::array<size_t,3> vertex{};std::array<ExactBary,3> reference;std::array<Point,3> flat;};
struct Perimeter {Id first=0,last=0;EdgeTrace trace;};
struct Sheet {
  Id identity=0,body=0;int marker=0;
  std::vector<Vertex> vertices;
  std::vector<Triangle> triangles;
  std::map<Id,size_t> current;
  std::vector<Perimeter> perimeter;
  std::vector<SurfaceFace> faces;
  std::optional<std::array<Id,3>> moving_run;
  std::optional<ExactRatio> moving_fraction;  // Canonical original-edge coordinate on A,p,B.
  Point normal;
};
}
struct CurvedAtlas::Data {
  const SU2Native3D::PlanarReference* reference=nullptr;
  std::shared_ptr<const GeometryBackend3D> backend;
  uint64_t version=0,revision=0,policy=0;
  std::optional<SU2Native3D::Id> moving;
  std::vector<Sheet> sheets;
  std::vector<FaceCoverage> old;
  SU2Native3D::BoundaryPoints parameters;
};
namespace {
size_t Bytes(const CurvedAtlas::Data& data) {
  size_t n=Plus(sizeof(data),Times(data.sheets.capacity(),sizeof(Sheet)));
  if(data.backend)n=Plus(n,sizeof(NativeMappedGeometryBackend3D)+4*sizeof(void*));
  n=Plus(n,Times(data.old.capacity(),sizeof(FaceCoverage)));
  n=Plus(n,Times(data.parameters.size(),sizeof(BoundaryPoints::value_type)+4*sizeof(void*)));
  for(const auto& cover:data.old){n=Plus(n,Times(cover.pieces.capacity(),sizeof(CoveragePiece)));
    for(const auto& p:cover.pieces)for(unsigned k=0;k<3;++k)n=Plus(n,Plus(BaryBytes(p.reference[k]),BaryBytes(p.active[k])));}
  for(const auto& s:data.sheets){if(s.moving_fraction)n=Plus(n,RatioBytes(*s.moving_fraction));n=Plus(n,Times(s.vertices.capacity(),sizeof(Vertex)));
    n=Plus(n,Times(s.triangles.capacity(),sizeof(Triangle)));n=Plus(n,Times(s.current.size(),sizeof(decltype(s.current)::value_type)+4*sizeof(void*)));
    n=Plus(n,Times(s.perimeter.capacity(),sizeof(Perimeter)));
    n=Plus(n,Times(s.faces.capacity(),sizeof(SurfaceFace)));
    for(const auto& v:s.vertices)n=Plus(n,Plus(Times(v.key.capacity(),sizeof(uint64_t)),BaryBytes(v.domain)));
    for(const auto& t:s.triangles)for(const auto& p:t.reference)n=Plus(n,BaryBytes(p));
    for(const auto& e:s.perimeter){n=Plus(n,Times(e.trace.pieces.capacity(),sizeof(TracePiece)));
      for(const auto& p:e.trace.pieces)n=Plus(n,Plus(Plus(BaryBytes(p.first),BaryBytes(p.last)),RatioBytes(p.end)));}}
  return n;
}
struct Ops {
  AtlasBudget& budget;size_t saved,extra=0;std::function<size_t()> resident;std::string reason;
  Ops(AtlasBudget& b,std::function<size_t()> r):budget(b),saved(b.caller_bytes),resident(std::move(r)) {
    // Fixed simultaneous arithmetic locals and convex triangle clipping (at most six vertices).
    extra=CurvedAtlasWorkspaceBytes(b.maximum_integer_bits);Sync();
  }
  ~Ops(){budget.caller_bytes=saved;}
  void Work(size_t n=1){if(budget.work>budget.maximum_work || n>budget.maximum_work-budget.work)throw std::length_error("Curved atlas work exhausted.");budget.work+=n;}
  void Sync(size_t additional=0){const auto bytes=Plus(saved,Plus(resident(),Plus(extra,additional)));
    if(bytes>budget.maximum_bytes)throw std::length_error("Curved atlas storage exhausted.");
    budget.peak_bytes=std::max(budget.peak_bytes,bytes);budget.caller_bytes=bytes;}
  void Check(bool ok){if(!ok)throw std::runtime_error(reason);}
  ExactRatio Integer(int64_t v){Sync();ExactRatio r;Check(ExactInteger(v,r,budget,reason));return r;}
  ExactRatio Binary(double v){Sync();ExactRatio r;Check(ExactBinary64(v,r,budget,reason));return r;}
  ExactRatio Add(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactAdd(a,b,r,budget,reason));return r;}
  ExactRatio Sub(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactSubtract(a,b,r,budget,reason));return r;}
  ExactRatio Mul(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactMultiply(a,b,r,budget,reason));return r;}
  ExactRatio Div(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactDivide(a,b,r,budget,reason));return r;}
  int Compare(const ExactRatio& a,const ExactRatio& b){Sync();int order;Check(ExactCompare(a,b,order,budget,reason));return order;}
  ExactRatio Weight(const ExactBary& p,unsigned k){return k==1?p.u:k==2?p.v:Sub(Sub(Integer(1),p.u),p.v);}
  bool Equal(const ExactBary& a,const ExactBary& b){return !Compare(a.u,b.u)&&!Compare(a.v,b.v);}
  ExactBary Lerp(const ExactBary& a,const ExactBary& b,const ExactRatio& t){return {Add(a.u,Mul(t,Sub(b.u,a.u))),Add(a.v,Mul(t,Sub(b.v,a.v)))};}
  ExactRatio Orient(const ExactBary& a,const ExactBary& b,const ExactBary& c){return Sub(Mul(Sub(b.u,a.u),Sub(c.v,a.v)),Mul(Sub(b.v,a.v),Sub(c.u,a.u)));}
  std::array<ExactRatio,3> Bary(const ExactBary& p,const std::array<ExactBary,3>& t){const auto area=Orient(t[0],t[1],t[2]);
    if(!Compare(area,Integer(0)))throw std::invalid_argument("Degenerate curved atlas triangle.");
    return {Div(Orient(p,t[1],t[2]),area),Div(Orient(t[0],p,t[2]),area),Div(Orient(t[0],t[1],p),area)};}
  ExactBary Interpolate(const std::array<ExactBary,3>& t,const std::array<ExactRatio,3>& w){ExactBary out{Integer(0),Integer(0)};
    for(unsigned k=0;k<3;++k){out.u=Add(out.u,Mul(t[k].u,w[k]));out.v=Add(out.v,Mul(t[k].v,w[k]));}return out;}
  double Round(const ExactRatio& x){Sync();std::array<double,2> b;Check(ExactBounds(x,b,budget,reason));
    if(!std::isfinite(b[0]) || !std::isfinite(b[1]))throw std::invalid_argument("Unrepresentable curved atlas proposal.");return b[0];}
  Key ReferenceKey(const MappedFacetReference& geometry,Id facet,const ExactBary& p){Sync();Key key;Check(geometry.ReferencePointKey(facet,p,key,budget,reason));return key;}
  Key BindingKey(const MappedFacetReference& geometry,const SurfaceBinding& binding){Key key;
    geometry.ForFacets(binding,[&](Id facet){if(!key.empty())return;ExactBary p;Sync();Check(geometry.BindingParameters(facet,binding,p,budget,reason));key=ReferenceKey(geometry,facet,p);});
    if(key.empty())throw std::invalid_argument("Atlas binding has no immutable support.");return key;}
};
struct Scratch {
  Ops& op;size_t bytes;
  Scratch(Ops& o,size_t n):op(o),bytes(n){op.Sync(n);op.extra=Plus(op.extra,n);}
  ~Scratch(){op.extra-=bytes;}
};
size_t PieceBound(const Ops& op){const auto limbs=Plus(op.budget.maximum_integer_bits,31)/32;
  return Plus(sizeof(CoveragePiece),Times(24*sizeof(uint32_t),limbs));}
template<class T>void Reserve(std::vector<T>& v,size_t count,Ops& op) {
  if(count<=v.capacity())return;
  op.Sync(Times(count,sizeof(T)));v.reserve(count);
}
size_t AddVertex(Sheet& sheet,Key key,Ops& op) {
  for(size_t i=0;i<sheet.vertices.size();++i){op.Work(Plus(key.size(),sheet.vertices[i].key.size()));if(sheet.vertices[i].key==key)return i;}
  Reserve(sheet.vertices,Plus(sheet.vertices.size(),1),op);op.Sync(Times(key.capacity(),sizeof(uint64_t)));
  sheet.vertices.push_back({std::move(key),{}});return sheet.vertices.size()-1;
}
bool Parameters(const Key& key,const Facet& facet,ExactBary& p,Ops& op) {
  std::array<ExactRatio,3> w{op.Integer(0),op.Integer(0),op.Integer(0)};
  if(key.size()<2)throw std::invalid_argument("Malformed atlas reference key.");
  if(key[0]==0){for(unsigned k=0;k<3;++k)if(facet.v[k].id==key[1]){w[k]=op.Integer(1);p={w[1],w[2]};return true;}return false;}
  if(key[0]==1){if(key.size()<7)throw std::invalid_argument("Malformed atlas edge key.");int first=-1,last=-1;
    for(unsigned k=0;k<3;++k){if(facet.v[k].id==key[1])first=int(k);if(facet.v[k].id==key[2])last=int(k);}if(first<0||last<0)return false;
    size_t at=3;ExactRatio t;op.Sync();op.Check(ExactDecode(key,at,t,op.budget,op.reason));if(at!=key.size())throw std::invalid_argument("Trailing atlas edge key words.");
    w[unsigned(last)]=t;w[unsigned(first)]=op.Sub(op.Integer(1),t);p={w[1],w[2]};return true;}
  if(key[0]!=2)throw std::invalid_argument("Unknown atlas reference key.");
  if(key[1]!=facet.id)return false;
  size_t at=2;op.Sync();op.Check(ExactDecode(key,at,p.u,op.budget,op.reason));op.Check(ExactDecode(key,at,p.v,op.budget,op.reason));
  if(at!=key.size())throw std::invalid_argument("Trailing atlas facet key words.");return true;
}
void Conform(Sheet& s,const PlanarReference& reference,Ops& op) {
  size_t index=0;
  while(index<s.triangles.size()){bool split=false;const auto old=s.triangles[index];
    for(size_t v=0;v<s.vertices.size()&&!split;++v){op.Work();if(std::find(old.vertex.begin(),old.vertex.end(),v)!=old.vertex.end())continue;
      ExactBary p;if(!Parameters(s.vertices[v].key,reference.Get(old.facet),p,op))continue;
      for(unsigned k=0;k<3;++k){const unsigned next=(k+1)%3,other=(k+2)%3;
        if(op.Compare(op.Orient(old.reference[k],old.reference[next],p),op.Integer(0)))continue;
        const auto& a=old.reference[k];const auto& b=old.reference[next];
        const auto t=op.Compare(a.u,b.u)?op.Div(op.Sub(p.u,a.u),op.Sub(b.u,a.u)):op.Div(op.Sub(p.v,a.v),op.Sub(b.v,a.v));
        if(op.Compare(t,op.Integer(0))<=0||op.Compare(t,op.Integer(1))>=0)continue;
        const double fraction=op.Round(t);const auto a3=old.flat[k],b3=old.flat[next];
        const Point flat{a3.x+fraction*(b3.x-a3.x),a3.y+fraction*(b3.y-a3.y),a3.z+fraction*(b3.z-a3.z)};
        Triangle first{old.facet,{old.vertex[k],v,old.vertex[other]},{old.reference[k],p,old.reference[other]},{old.flat[k],flat,old.flat[other]}};
        Triangle second{old.facet,{v,old.vertex[next],old.vertex[other]},{p,old.reference[next],old.reference[other]},{flat,old.flat[next],old.flat[other]}};
        Reserve(s.triangles,Plus(s.triangles.size(),1),op);op.Sync(Plus(BaryBytes(p),BaryBytes(old.reference[other])));
        s.triangles[index]=std::move(first);s.triangles.push_back(std::move(second));split=true;break;}
    }
    if(!split)++index;
  }
}
std::vector<Id> BoundaryCycle(const Sheet& s,Ops& op) {
  Scratch memory(op,Times(Plus(Times(s.faces.size(),3),1),sizeof(std::pair<Edge,std::pair<unsigned,Edge>>)+8*sizeof(void*)));
  std::map<Edge,std::pair<unsigned,Edge>> edges;
  for(const auto& f:s.faces)for(unsigned k=0;k<3;++k){op.Work();const Edge oriented{f.v[k],f.v[(k+1)%3]};auto& edge=edges[Sorted(oriented[0],oriented[1])];
    if(edge.first && edge.second==oriented)throw std::invalid_argument("Atlas source faces repeat an oriented edge.");
    if(++edge.first>2)throw std::invalid_argument("Atlas source edge has excessive incidence.");edge.second=oriented;}
  std::map<Id,Id> next;std::set<Id> incoming;
  for(const auto& row:edges)if(row.second.first==1){const auto edge=row.second.second;
    if(!next.emplace(edge[0],edge[1]).second || !incoming.insert(edge[1]).second)throw std::invalid_argument("Atlas perimeter branches.");}
  if(next.empty())throw std::invalid_argument("Atlas region has no disk perimeter.");
  std::vector<Id> cycle;cycle.reserve(next.size());const auto first=next.begin()->first;auto at=first;
  do{op.Work();if(cycle.size()>=next.size() || !next.count(at))throw std::invalid_argument("Atlas perimeter is not one cycle.");cycle.push_back(at);at=next.at(at);}while(at!=first);
  if(cycle.size()!=next.size())throw std::invalid_argument("Atlas region has holes or disconnected perimeter.");return cycle;
}
void SetBoundary(Sheet& s,size_t vertex,const ExactBary& p,std::set<size_t>& fixed,Ops& op) {
  if(fixed.count(vertex)){if(!op.Equal(s.vertices.at(vertex).domain,p))throw std::invalid_argument("Inherited atlas trace has inconsistent active-edge parameterization.");return;}
  op.Sync(Plus(sizeof(size_t)+4*sizeof(void*),BaryBytes(p)));s.vertices.at(vertex).domain=p;fixed.insert(vertex);
}
std::optional<Id> TraceChain(const EdgeTrace& trace,const MappedFacetReference& geometry,const MobilityPolicy& policy,Ops& op) {
  std::optional<Id> chain;
  for(const auto& p:trace.pieces){op.Work();const auto midpoint=op.Lerp(p.first,p.last,op.Div(op.Integer(1),op.Integer(2)));
    const auto key=op.ReferenceKey(geometry,p.facet,midpoint);
    if(key.size()<3 || key[0]!=1)return {};
    const auto edge=policy.features.find({key[1],key[2]});
    if(edge==policy.features.end() || (chain&&*chain!=edge->second))return {};chain=edge->second;}
  return chain;
}
Id FeatureChain(const Sheet& sheet,Id a,Id b,const MappedFacetReference& geometry,const MobilityPolicy& policy,Ops& op) {
  const auto found=std::find_if(sheet.perimeter.begin(),sheet.perimeter.end(),[&](const Perimeter& edge){return Sorted(edge.first,edge.last)==Sorted(a,b);});
  if(found==sheet.perimeter.end())throw std::invalid_argument("Missing atlas feature perimeter edge.");
  const auto chain=TraceChain(found->trace,geometry,policy,op);
  if(!chain)throw std::invalid_argument("Movable perimeter sample leaves its genuine curve chain.");return *chain;
}
// Genuine feature arcs can be nonseparating globally; cut the admitted physical star locally.
void SplitLocalSheets(CurvedAtlas::Data& data,const PlanarReference& reference,EdgeTrace& trace,Ops& op) {
  size_t planned=0,total_faces=0;
  for(const auto& sheet:data.sheets){total_faces=Plus(total_faces,sheet.faces.size());
    planned=Plus(planned,Times(sheet.faces.size(),sizeof(Sheet)+2*sizeof(SurfaceFace)+12*(sizeof(size_t)+4*sizeof(void*))));
    for(const auto& face:sheet.faces)for(Id id:face.v){const auto& key=sheet.vertices.at(sheet.current.at(id)).key;
      planned=Plus(planned,Plus(2*sizeof(Vertex)+sizeof(std::pair<const Id,size_t>)+4*sizeof(void*)+16,Times(key.capacity(),sizeof(uint64_t))));}}
  Scratch storage(op,Times(2,planned));std::vector<Sheet> split;split.reserve(total_faces);
  const auto& geometry=reference.Parameters()->Geometry();const auto& policy=reference.Parameters()->Mobility();
  for(const auto& sheet:data.sheets){std::vector<size_t> parent(sheet.faces.size());for(size_t i=0;i<parent.size();++i)parent[i]=i;
    auto root=[&](size_t i){while(parent[i]!=i){parent[i]=parent[parent[i]];i=parent[i];}return i;};
    std::map<Edge,std::pair<size_t,unsigned>> edges;
    for(size_t i=0;i<sheet.faces.size();++i)for(unsigned k=0;k<3;++k){op.Work();const auto& face=sheet.faces[i];const Edge edge=Sorted(face.v[k],face.v[(k+1)%3]);
      const auto inserted=edges.emplace(edge,std::make_pair(i,1u));if(inserted.second)continue;
      if(++inserted.first->second.second>2)throw std::invalid_argument("Local working sheet has nonmanifold edge incidence.");
      op.Sync();op.Check(reference.ResolveTrace(edge,face,trace,op.budget,op.reason));
      if(!TraceChain(trace,geometry,policy,op)){const auto a=root(i),b=root(inserted.first->second.first);parent[std::max(a,b)]=std::min(a,b);}}
    std::map<size_t,size_t> components;
    for(size_t i=0;i<sheet.faces.size();++i){const auto component=root(i);auto found=components.find(component);
      if(found==components.end()){components.emplace(component,split.size());split.push_back({});found=components.find(component);
        auto& fresh=split.back();fresh.identity=sheet.identity;fresh.body=sheet.body;fresh.marker=sheet.marker;}
      auto& fresh=split[found->second];const auto& face=sheet.faces[i];fresh.faces.push_back(face);
      for(unsigned k=0;k<3;++k){const Id id=face.v[k];const auto vertex=AddVertex(fresh,sheet.vertices.at(sheet.current.at(id)).key,op);fresh.current.emplace(id,vertex);
        const auto a=data.parameters.at(id).node.p,b=data.parameters.at(face.v[(k+1)%3]).node.p;
        fresh.normal.x+=a.y*b.z-a.z*b.y;fresh.normal.y+=a.z*b.x-a.x*b.z;fresh.normal.z+=a.x*b.y-a.y*b.x;}}
  }
  data.sheets=std::move(split);
}
// A feature path can be reparameterized only when every incident face participates.
ExactRatio CleanFeatureTrace(EdgeTrace& trace,const MappedFacetReference& geometry,const MobilityPolicy& policy,Ops& op) {
  struct Part {Id facet;Edge edge;ExactRatio first,last;};
  Scratch scratch(op,Plus(Times(trace.pieces.size(),PieceBound(op)),Times(3,TraceBytes(trace))));
  std::vector<Part> parts;parts.reserve(trace.pieces.size());std::optional<Id> chain;
  for(const auto& piece:trace.pieces) {
    op.Work();const auto& facet=geometry.Reference().Get(piece.facet);std::optional<Part> part;
    for(unsigned k=0;k<3;++k)if(!op.Compare(op.Weight(piece.first,k),op.Integer(0))&&!op.Compare(op.Weight(piece.last,k),op.Integer(0))) {
      const Edge edge=Sorted(facet.v[(k+1)%3].id,facet.v[(k+2)%3].id);
      const auto feature=policy.features.find(edge);
      if(feature==policy.features.end()||(chain&&*chain!=feature->second))throw std::invalid_argument("Retired curve trace leaves its genuine feature chain.");
      chain=feature->second;unsigned coordinate=0;while(facet.v[coordinate].id!=edge[1])++coordinate;
      part=Part{piece.facet,edge,op.Weight(piece.first,coordinate),op.Weight(piece.last,coordinate)};break;
    }
    if(!part||!op.Compare(part->first,part->last))throw std::invalid_argument("Retired curve trace has no nonzero original-edge segment.");
    if(!parts.empty()&&parts.back().edge==part->edge) {
      if(op.Compare(parts.back().last,part->first)||op.Compare(parts.back().first,parts.back().last)!=op.Compare(part->first,part->last))
        throw std::invalid_argument("Retired curve trace folds or is disconnected.");
      parts.back().last=part->last;
    } else parts.push_back(std::move(*part));
  }
  auto total=op.Integer(0);
  for(const auto& part:parts){auto length=op.Sub(part.last,part.first);length.negative=false;total=op.Add(total,length);}
  if(!op.Compare(total,op.Integer(0)))throw std::invalid_argument("Empty retired curve trace.");
  EdgeTrace fresh;fresh.pieces.reserve(parts.size());auto accumulated=op.Integer(0);
  for(const auto& part:parts) {
    const auto& facet=geometry.Reference().Get(part.facet);
    auto parameter=[&](const ExactRatio& t){std::array<ExactRatio,3> weights{op.Integer(0),op.Integer(0),op.Integer(0)};
      for(unsigned k=0;k<3;++k){if(facet.v[k].id==part.edge[0])weights[k]=op.Sub(op.Integer(1),t);if(facet.v[k].id==part.edge[1])weights[k]=t;}return ExactBary{weights[1],weights[2]};};
    auto length=op.Sub(part.last,part.first);length.negative=false;accumulated=op.Add(accumulated,length);
    fresh.pieces.push_back({part.facet,parameter(part.first),parameter(part.last),op.Div(accumulated,total)});
  }
  trace=std::move(fresh);return total;
}
void Flatten(Sheet& s,const CurvedAtlas::Data& data,Ops& op) {
  const auto& geometry=data.reference->Parameters()->Geometry();const auto& policy=data.reference->Parameters()->Mobility();
  auto cycle=BoundaryCycle(s,op);std::vector<Id> macro=cycle;
  if(data.moving){const auto found=std::find(cycle.begin(),cycle.end(),*data.moving);
    if(found!=cycle.end()){
      const auto& point=data.parameters.at(*data.moving);if(geometry.MobilityDimension(point.binding,policy)!=1)
        throw std::invalid_argument("A movable smooth point requires its complete physical sheet star.");
      const size_t index=size_t(found-cycle.begin());Id a=cycle[(index+cycle.size()-1)%cycle.size()],b=cycle[(index+1)%cycle.size()];
      if(FeatureChain(s,a,*data.moving,geometry,policy,op)!=FeatureChain(s,*data.moving,b,geometry,policy,op))
        throw std::invalid_argument("Atlas perimeter sample crosses distinct curve chains.");
      if(a>b)std::swap(a,b);s.moving_run=std::array<Id,3>{a,*data.moving,b};macro.erase(std::find(macro.begin(),macro.end(),*data.moving));}}
  if(macro.size()<3)throw std::invalid_argument("Atlas disk needs at least three retained macrovertices.");
  Scratch scratch(op,Times(Plus(Times(s.vertices.size(),12),Times(s.triangles.size(),12)),sizeof(size_t)+4*sizeof(void*)));
  std::set<size_t> fixed;
  for(size_t i=0;i<macro.size();++i){op.Work();if(i>size_t(INT64_MAX) || (i && i>size_t(INT64_MAX)/i))throw std::length_error("Atlas polygon integer overflow.");
    SetBoundary(s,s.current.at(macro[i]),{op.Integer(int64_t(i)),op.Integer(int64_t(i*i))},fixed,op);}
  if(s.moving_run){const auto r=*s.moving_run;SetBoundary(s,s.current.at(r[1]),op.Lerp(s.vertices.at(s.current.at(r[0])).domain,
      s.vertices.at(s.current.at(r[2])).domain,s.moving_fraction.value()),fixed,op);}
  for(const auto& edge:s.perimeter){const auto a=s.vertices.at(s.current.at(edge.first)).domain,b=s.vertices.at(s.current.at(edge.last)).domain;
    auto start=op.Integer(0);
    for(const auto& p:edge.trace.pieces){const auto first=op.ReferenceKey(geometry,p.facet,p.first),last=op.ReferenceKey(geometry,p.facet,p.last);
      auto locate=[&](const Key& key){for(size_t i=0;i<s.vertices.size();++i){op.Work();if(s.vertices[i].key==key)return i;}throw std::invalid_argument("Atlas trace knot is absent from common refinement.");};
      SetBoundary(s,locate(first),op.Lerp(a,b,start),fixed,op);SetBoundary(s,locate(last),op.Lerp(a,b,p.end),fixed,op);start=p.end;}
    if(op.Compare(start,op.Integer(1)))throw std::invalid_argument("Atlas inherited trace does not reach its endpoint.");}
  std::vector<Face> topology;topology.reserve(s.triangles.size());std::vector<std::set<size_t>> adjacency(s.vertices.size());
  for(const auto& t:s.triangles){op.Work();topology.push_back({Id(t.vertex[0]),Id(t.vertex[1]),Id(t.vertex[2])});
    for(unsigned k=0;k<3;++k){adjacency[t.vertex[k]].insert(t.vertex[(k+1)%3]);adjacency[t.vertex[(k+1)%3]].insert(t.vertex[k]);}}
  ValidateSurfaceTopology(topology,false,topology.size());
  std::vector<size_t> interior;std::map<size_t,size_t> row;
  for(size_t i=0;i<s.vertices.size();++i)if(!fixed.count(i)){row.emplace(i,interior.size());interior.push_back(i);}
  const auto n=interior.size();
  if(n){if(n>std::numeric_limits<unsigned long>::max())throw std::length_error("Atlas solver dimension overflow.");
    const size_t square=Times(n,n);Scratch dense(op,Times(Plus(Times(2,square),Times(2,n)),sizeof(double)));
    op.Work(Plus(Times(8,Times(square,n)),Times(8,square)));
    std::vector<double> matrix(square),inverse(square),rhs(2*n);
    for(size_t i=0;i<n;++i){const auto v=interior[i];matrix[i*n+i]=double(adjacency[v].size());
      for(size_t neighbor:adjacency[v]){if(row.count(neighbor))matrix[i*n+row.at(neighbor)]=-1;
        else{rhs[2*i]+=op.Round(s.vertices[neighbor].domain.u);rhs[2*i+1]+=op.Round(s.vertices[neighbor].domain.v);}}}
    SU2_LinAlg::MatrixInverse(static_cast<unsigned long>(n),matrix.data(),inverse.data());
    for(size_t i=0;i<n;++i){double x=0,y=0;for(size_t j=0;j<n;++j){x+=inverse[i*n+j]*rhs[2*j];y+=inverse[i*n+j]*rhs[2*j+1];}
      s.vertices[interior[i]].domain={op.Binary(x),op.Binary(y)};}}
  for(const auto& t:s.triangles)if(op.Compare(op.Orient(s.vertices[t.vertex[0]].domain,s.vertices[t.vertex[1]].domain,s.vertices[t.vertex[2]].domain),op.Integer(0))<=0)
    throw std::invalid_argument("Atlas convex PL flattening has a nonpositive exact triangle.");
  Scratch embedding(op,Times(s.triangles.size(),PieceBound(op)));std::vector<CoveragePiece> region;region.reserve(s.triangles.size());
  for(const auto& t:s.triangles)region.push_back({t.facet,t.reference,{}});
  op.Sync();op.Check(geometry.CertifyEmbedding(region,s.normal,op.budget,op.reason));
}
}
size_t CurvedAtlas::ResidentBytes()const{return data?Bytes(*data):0;}
size_t CurvedAtlas::ReferenceTriangleCount()const {
  size_t count=0;if(data)for(const auto& sheet:data->sheets)count=Plus(count,sheet.triangles.size());return count;
}
size_t CurvedAtlasWorkspaceBytes(size_t bits) {
  try{const auto limbs=Plus(Plus(bits,31)/32,1);
    // 64 rational locals use128 arrays;32 arrays bound integer normalization and arithmetic temporaries.
    return Plus(64*sizeof(ExactRatio),Times(160*sizeof(uint32_t),limbs));
  }catch(const std::length_error&){return SIZE_MAX;}
}

namespace { SurfaceBinding Quantize(const MappedFacetReference&,Id,const ExactBary&,Ops&); }

bool PrepareCurvedAtlas(const SU2Native3D::Patch& accepted,const SU2Native3D::PlanarReference& reference,
    std::optional<SU2Native3D::Id> moving,CurvedAtlas& result,AtlasBudget& budget,std::string& reason) {
  std::shared_ptr<CurvedAtlas::Data> data;FaceCoverage working,fresh_reference;std::array<EdgeTrace,3> working_traces;
  try {Ops op(budget,[&]{return Plus(Plus(data?Bytes(*data):0,result.ResidentBytes()),Plus(Plus(FaceBytes(working),FaceBytes(fresh_reference)),TraceBytes(working_traces)));});
    if(!reference.Parameters() || !reference.Mobility() || !reference.Mobility()->identity || accepted.surface.empty())
      throw std::invalid_argument("Curved atlas requires accepted mapped geometry and its configured mobility policy.");
    op.Sync(sizeof(CurvedAtlas::Data)+4*sizeof(void*));data=std::make_shared<CurvedAtlas::Data>();data->reference=&reference;
    const auto& geometry=reference.Parameters()->Geometry();const auto& policy=reference.Parameters()->Mobility();
    op.Sync(sizeof(NativeMappedGeometryBackend3D)+4*sizeof(void*));
    data->backend=std::make_shared<NativeMappedGeometryBackend3D>(reference.Parameters()->Authority(),policy);
    data->version=geometry.Version();data->revision=reference.Parameters()->Revision();data->policy=policy.identity;data->moving=moving;
    for(const auto& face:accepted.surface)for(Id id:face.v)if(!data->parameters.count(id)){
      const auto found=accepted.parameters.find(id);const auto* point=found==accepted.parameters.end()?reference.Parameters()->Find(id):&found->second;
      if(!point || point->node.id!=id)throw std::invalid_argument("Atlas source lacks an actual forward endpoint.");
      op.Check(data->backend->AssociationValid(point->binding,point->node.p,op.reason));
      op.Sync(sizeof(BoundaryPoints::value_type)+4*sizeof(void*));data->parameters.emplace(id,*point);}
    if(moving && (!data->parameters.count(*moving)||!geometry.MobilityDimension(data->parameters.at(*moving).binding,policy)))
      throw std::invalid_argument("Atlas cannot move an absent point or genuine corner/branch.");
    for(const auto& face:accepted.surface){op.Work();const auto sheet_id=policy.sheets.find(face.facet),body=policy.bodies.find(face.facet);
      if(sheet_id==policy.sheets.end()||body==policy.bodies.end())throw std::invalid_argument("Atlas source lacks a sheet/body policy.");
      auto at=std::find_if(data->sheets.begin(),data->sheets.end(),[&](const Sheet& s){return s.identity==sheet_id->second;});
      if(at==data->sheets.end()){Reserve(data->sheets,Plus(data->sheets.size(),1),op);data->sheets.push_back({});at=std::prev(data->sheets.end());
        at->identity=sheet_id->second;at->body=body->second;at->marker=reference.Get(face.facet).marker;}
      auto& sheet=*at;if(sheet.body!=body->second||sheet.marker!=reference.Get(face.facet).marker)throw std::invalid_argument("Atlas sheet merges distinct bodies/markers.");
      auto& cover=working;cover={};op.Sync();op.Check(reference.ResolveCoverage(face,cover,budget,op.reason));
      std::array<SurfaceBinding,3> bindings;std::array<Point,3> realized;auto& traces=working_traces;
      for(unsigned k=0;k<3;++k){bindings[k]=data->parameters.at(face.v[k]).binding;realized[k]=data->parameters.at(face.v[k]).node.p;}
      op.Check(geometry.CoverageTraces(cover,traces,budget,op.reason));op.Check(geometry.ValidateCoverage(face.facet,bindings,realized,cover,traces,budget,op.reason));
      for(unsigned k=0;k<3;++k){const unsigned j=(k+1)%3;const auto a=realized[k],b=realized[j];
        sheet.normal.x+=a.y*b.z-a.z*b.y;sheet.normal.y+=a.z*b.x-a.x*b.z;sheet.normal.z+=a.x*b.y-a.y*b.x;}
      Reserve(sheet.faces,Plus(sheet.faces.size(),1),op);sheet.faces.push_back(face);
      for(const auto& p:cover.pieces)if(policy.sheets.at(p.facet)!=sheet.identity||policy.bodies.at(p.facet)!=sheet.body||reference.Get(p.facet).marker!=sheet.marker)
        throw std::invalid_argument("Atlas face crosses a protected sheet/body boundary.");
      for(unsigned k=0;k<3;++k){const auto vertex=AddVertex(sheet,op.BindingKey(geometry,bindings[k]),op);op.Sync(sizeof(decltype(sheet.current)::value_type)+4*sizeof(void*));
        const auto inserted=sheet.current.emplace(face.v[k],vertex);if(!inserted.second && inserted.first->second!=vertex)throw std::invalid_argument("Atlas endpoint has inconsistent chart locations.");}
      Reserve(data->old,Plus(data->old.size(),1),op);data->old.push_back(std::move(cover));}
    SplitLocalSheets(*data,reference,working_traces[0],op);
    Scratch incidence_storage(op,Times(accepted.surface.size(),Plus(Times(3,sizeof(std::pair<const Edge,size_t>)+4*sizeof(void*)),sizeof(std::pair<const Face,size_t>)+4*sizeof(void*))));
    std::map<Edge,size_t> incidence;std::map<Face,size_t> source_regions;
    for(size_t i=0;i<accepted.surface.size();++i)if(!source_regions.emplace(Ordered(accepted.surface[i].v),i).second)
      throw std::invalid_argument("Atlas repeats an accepted physical face.");
    for(const auto& face:accepted.surface)for(unsigned k=0;k<3;++k){op.Work();++incidence[Sorted(face.v[k],face.v[(k+1)%3])];}
    for(auto& sheet:data->sheets){const auto cycle=BoundaryCycle(sheet,op);
      for(size_t i=0;i<cycle.size();++i){const auto edge=Sorted(cycle[i],cycle[(i+1)%cycle.size()]);
        const auto face=std::find_if(sheet.faces.begin(),sheet.faces.end(),[&](const SurfaceFace& f){return std::find(f.v.begin(),f.v.end(),edge[0])!=f.v.end()&&std::find(f.v.begin(),f.v.end(),edge[1])!=f.v.end();});
        EdgeTrace trace;op.Check(reference.ResolveTrace(edge,*face,trace,budget,op.reason));
        Reserve(sheet.perimeter,Plus(sheet.perimeter.size(),1),op);sheet.perimeter.push_back({edge[0],edge[1],std::move(trace)});}
      std::optional<Id> first_neighbor,last_neighbor;
      if(moving&&sheet.current.count(*moving)&&geometry.MobilityDimension(data->parameters.at(*moving).binding,policy)==1) {
        const auto where=std::find(cycle.begin(),cycle.end(),*moving);
        if(where==cycle.end())throw std::invalid_argument("Curve sampling point is absent from its sheet perimeter.");
        const size_t at=size_t(where-cycle.begin());Id a=cycle[(at+cycle.size()-1)%cycle.size()],b=cycle[(at+1)%cycle.size()];if(a>b)std::swap(a,b);
        first_neighbor=a;last_neighbor=b;
      }
      std::optional<ExactRatio> first_length,last_length;
      for(auto& edge:sheet.perimeter)if(incidence.at(Sorted(edge.first,edge.last))==2&&TraceChain(edge.trace,geometry,policy,op)) {
        const auto length=CleanFeatureTrace(edge.trace,geometry,policy,op);
        if(first_neighbor&&Sorted(edge.first,edge.last)==Sorted(*first_neighbor,*moving))first_length=length;
        if(last_neighbor&&Sorted(edge.first,edge.last)==Sorted(*moving,*last_neighbor))last_length=length;
      }
      if(first_neighbor) {
        if(!first_length||!last_length)throw std::invalid_argument("Curve reconstruction lacks all incident faces.");
        sheet.moving_fraction=op.Div(*first_length,op.Add(*first_length,*last_length));
      }
      // Accepted pieces define the region only; fresh geometry never inherits their interior cuts.
      working.pieces.clear();fresh_reference.pieces.clear();
      for(const auto& face:sheet.faces)for(const auto& piece:data->old.at(source_regions.at(Ordered(face.v))).pieces) {
        Reserve(working.pieces,Plus(working.pieces.size(),1),op);op.Sync(PieceBound(op));working.pieces.push_back(piece);
      }
      Scratch constraints(op,Plus(Times(sheet.current.size(),sizeof(std::pair<const Id,SurfaceBinding>)+4*sizeof(void*)),Bytes(*data)));
      std::map<Id,SurfaceBinding> bindings;std::vector<EdgeTrace> retained;retained.reserve(sheet.perimeter.size());
      for(const auto& point:sheet.current)bindings.emplace(point.first,data->parameters.at(point.first).binding);
      for(const auto& edge:sheet.perimeter)retained.push_back(edge.trace);
      op.Sync();op.Check(RebuildReferenceTriangulation(geometry,working.pieces,bindings,retained,fresh_reference.pieces,budget,op.reason));
      for(const auto& piece:fresh_reference.pieces) {
        Triangle triangle;triangle.facet=piece.facet;triangle.reference=piece.reference;
        for(unsigned k=0;k<3;++k) {
          triangle.vertex[k]=AddVertex(sheet,op.ReferenceKey(geometry,piece.facet,piece.reference[k]),op);
          // Canonical support is required at original vertices/edges as well as chart interiors.
          GeometryEvaluation evaluation;op.Sync();
          const auto binding=Quantize(geometry,piece.facet,piece.reference[k],op);
          op.Check(data->backend->Evaluate(binding,std::nullopt,evaluation,budget,op.reason));triangle.flat[k]=evaluation.point;
        }
        Reserve(sheet.triangles,Plus(sheet.triangles.size(),1),op);op.Sync(PieceBound(op));sheet.triangles.push_back(std::move(triangle));
      }
      Conform(sheet,reference,op);Flatten(sheet,*data,op);}
    result.data=std::move(data);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}

namespace {
const CurvedAtlas::Data& Current(const CurvedAtlas& atlas) {
  if(!atlas.data || !atlas.data->reference || !atlas.data->reference->Parameters())throw std::invalid_argument("Unprepared curved atlas.");
  const auto& d=*atlas.data;const auto* state=d.reference->Parameters();
  if(!d.backend || d.backend->Identity().declaration!=state->Authority().get() ||
     state->Revision()!=d.revision || state->Geometry().Version()!=d.version || state->Mobility().identity!=d.policy)
    throw std::invalid_argument("Curved atlas authority/policy/revision changed.");return d;
}
bool MapPoint(const Sheet& s,const Key& key,const PlanarReference& reference,ExactBary& result,Ops& op) {
  std::optional<ExactBary> mapped;
  for(const auto& t:s.triangles){op.Work();ExactBary point;if(!Parameters(key,reference.Get(t.facet),point,op))continue;
    const auto bary=op.Bary(point,t.reference);bool inside=true;for(const auto& w:bary)inside=inside && op.Compare(w,op.Integer(0))>=0;
    if(!inside)continue;
    const std::array<ExactBary,3> domain{s.vertices[t.vertex[0]].domain,s.vertices[t.vertex[1]].domain,s.vertices[t.vertex[2]].domain};
    auto candidate=op.Interpolate(domain,bary);
    if(mapped && !op.Equal(*mapped,candidate))throw std::invalid_argument("Reference point has inconsistent adjacent atlas maps.");mapped=std::move(candidate);}
  if(!mapped)return false;result=std::move(*mapped);return true;
}
SurfaceBinding Quantize(const MappedFacetReference& geometry,Id facet,const ExactBary& point,Ops& op) {
  std::array<ExactRatio,3> w{op.Weight(point,0),point.u,point.v};std::vector<unsigned> nonzero;
  for(unsigned k=0;k<3;++k){if(op.Compare(w[k],op.Integer(0))<0)throw std::invalid_argument("Atlas proposal leaves an original chart.");if(op.Compare(w[k],op.Integer(0)))nonzero.push_back(k);}
  const auto& f=geometry.Reference().Get(facet);
  if(nonzero.size()==1)return geometry.Vertex(f.v[nonzero[0]].id);
  if(nonzero.size()==2){auto a=nonzero[0],b=nonzero[1];if(f.v[a].id>f.v[b].id)std::swap(a,b);
    return geometry.EdgePoint({f.v[a].id,f.v[b].id},op.Round(w[b]));}
  if(nonzero.size()!=3)throw std::invalid_argument("Empty atlas binding proposal.");
  return geometry.FacetPoint(facet,op.Round(point.u),op.Round(point.v));
}
void Movement(const CurvedAtlas::Data& d,Id old,const SurfaceBinding& binding,Ops& op) {
  const auto& geometry=d.reference->Parameters()->Geometry();const auto& policy=d.reference->Parameters()->Mobility();
  const auto original=d.parameters.find(old);if(original==d.parameters.end())throw std::invalid_argument("Atlas move lacks its accepted endpoint.");
  const auto dimension=geometry.MobilityDimension(original->second.binding,policy);
  if(!dimension || geometry.MobilityDimension(binding,policy)!=dimension)throw std::invalid_argument("Atlas move changes physical feature dimension.");
  const auto key=op.BindingKey(geometry,binding);std::optional<ExactRatio> curve_parameter;size_t incident=0;
  for(const auto& s:d.sheets)if(s.current.count(old)) {++incident;ExactBary p;if(!MapPoint(s,key,*d.reference,p,op))throw std::invalid_argument("Atlas move leaves an incident sheet.");
    if(dimension==1){if(!s.moving_run || (*s.moving_run)[1]!=old)throw std::invalid_argument("Atlas curve move lacks its complete macroperimeter.");
      const auto run=*s.moving_run;const auto& a=s.vertices.at(s.current.at(run[0])).domain;const auto& b=s.vertices.at(s.current.at(run[2])).domain;
      if(op.Compare(op.Orient(a,b,p),op.Integer(0)))throw std::invalid_argument("Atlas curve move leaves the common feature path.");
      const auto t=op.Compare(a.u,b.u)?op.Div(op.Sub(p.u,a.u),op.Sub(b.u,a.u)):op.Div(op.Sub(p.v,a.v),op.Sub(b.v,a.v));
      if(op.Compare(t,op.Integer(0))<=0||op.Compare(t,op.Integer(1))>=0)throw std::invalid_argument("Atlas curve sample reaches a retained endpoint.");
      if(curve_parameter && op.Compare(*curve_parameter,t))throw std::invalid_argument("Incident atlas sheets disagree on the exact curve parameter.");curve_parameter=t;}}
  if(!incident)throw std::invalid_argument("Atlas endpoint has no admitted sheet.");
}
EdgeTrace Reverse(const EdgeTrace& trace,Ops& op) {
  Scratch scratch(op,Times(trace.pieces.size(),PieceBound(op)));EdgeTrace out;out.pieces.reserve(trace.pieces.size());
  for(size_t i=trace.pieces.size();i;--i){op.Work();auto p=trace.pieces[i-1];std::swap(p.first,p.last);
    p.end=op.Sub(op.Integer(1),i>1?trace.pieces[i-2].end:op.Integer(0));out.pieces.push_back(std::move(p));}return out;
}
struct ClipVertex {ExactBary domain,reference;};
std::vector<ClipVertex> Clip(const Triangle& old,const Sheet& sheet,const std::array<ExactBary,3>& face,Ops& op) {
  std::vector<ClipVertex> polygon;polygon.reserve(6);
  for(unsigned k=0;k<3;++k)polygon.push_back({sheet.vertices[old.vertex[k]].domain,old.reference[k]});
  for(unsigned side=0;side<3 && !polygon.empty();++side){std::vector<ClipVertex> next;next.reserve(6);
    const auto& a=face[side];const auto& b=face[(side+1)%3];
    for(size_t i=0;i<polygon.size();++i){op.Work();const auto& p=polygon[i];const auto& q=polygon[(i+1)%polygon.size()];
      const auto dp=op.Orient(a,b,p.domain),dq=op.Orient(a,b,q.domain);const bool pin=op.Compare(dp,op.Integer(0))>=0,qin=op.Compare(dq,op.Integer(0))>=0;
      auto append=[&](ClipVertex v){if(!next.empty() && op.Equal(next.back().domain,v.domain))return;
        if(next.size()>1 && op.Equal(next.front().domain,v.domain))return;
        if(next.size()>=6)throw std::logic_error("Convex triangle intersection exceeds six vertices.");next.push_back(std::move(v));};
      if(pin)append(p);
      if(pin!=qin){const auto t=op.Div(dp,op.Sub(dp,dq));append({op.Lerp(p.domain,q.domain,t),op.Lerp(p.reference,q.reference,t)});}}
    if(next.size()>1 && op.Equal(next.front().domain,next.back().domain))next.pop_back();polygon=std::move(next);}
  return polygon;
}
size_t CoverageBytes(const BoundaryCoverage& out){return SU2Native3D::BoundaryCoverageBytes(out);}
}

bool ProposeAtlasBinding(const CurvedAtlas& atlas,SU2Native3D::Id old,SU2Native3D::Node node,
    SU2Native3D::BoundaryPoint& result,AtlasBudget& budget,std::string& reason) {
  try {const auto& d=Current(atlas);Ops op(budget,[&]{return Bytes(d);});const auto& geometry=d.reference->Parameters()->Geometry();
    if(!d.moving || *d.moving!=old || node.id==old || !std::isfinite(node.p.x)||!std::isfinite(node.p.y)||!std::isfinite(node.p.z))
      throw std::invalid_argument("Invalid atlas movement identity/proposal.");
    std::optional<BoundaryPoint> best;long double distance=std::numeric_limits<long double>::infinity();
    auto consider=[&](Id facet,const ExactBary& reference){const auto binding=Quantize(geometry,facet,reference,op);
      GeometryEvaluation evaluation;GeometrySupport support;op.Sync();
      op.Check(d.backend->Evaluate(binding,std::nullopt,evaluation,budget,op.reason));const auto point=evaluation.point;
      op.Check(d.backend->AssociationValid(binding,point,op.reason));op.Check(d.backend->Support(binding,facet,support,budget,op.reason));
      if(support.body!=d.reference->Parameters()->Mobility().bodies.at(facet))throw std::invalid_argument("Atlas projection changed its physical body.");
      Movement(d,old,binding,op);const long double x=point.x-(long double)node.p.x,y=point.y-(long double)node.p.y,z=point.z-(long double)node.p.z;
      const auto squared=x*x+y*y+z*z;if(squared<distance){distance=squared;best=BoundaryPoint{{node.id,point},binding};}};
    const auto& policy=d.reference->Parameters()->Mobility();const bool curve=geometry.MobilityDimension(d.parameters.at(old).binding,policy)==1;
    if(curve){const auto found=std::find_if(d.sheets.begin(),d.sheets.end(),[&](const Sheet& s){return s.current.count(old);});
      if(found==d.sheets.end()||!found->moving_run)throw std::invalid_argument("Atlas curve proposal lacks a macroedge.");
      const auto run=*found->moving_run;const auto a=d.parameters.at(run[0]).node.p,b=d.parameters.at(run[2]).node.p;
      const long double dx=b.x-(long double)a.x,dy=b.y-(long double)a.y,dz=b.z-(long double)a.z,den=dx*dx+dy*dy+dz*dz;
      if(!(den>0))throw std::invalid_argument("Collapsed physical curve proposal chord.");
      const double parameter=double(((node.p.x-(long double)a.x)*dx+(node.p.y-(long double)a.y)*dy+(node.p.z-(long double)a.z)*dz)/den);
      if(!(parameter>0&&parameter<1))throw std::invalid_argument("Curve proposal leaves retained macroedge endpoints.");
      const auto target=op.Lerp(found->vertices[found->current.at(run[0])].domain,found->vertices[found->current.at(run[2])].domain,op.Binary(parameter));
      bool located=false;
      for(const auto& triangle:found->triangles){const std::array<ExactBary,3> domain{found->vertices[triangle.vertex[0]].domain,found->vertices[triangle.vertex[1]].domain,found->vertices[triangle.vertex[2]].domain};
        const auto w=op.Bary(target,domain);bool inside=true;for(const auto& r:w)inside=inside&&op.Compare(r,op.Integer(0))>=0;
        if(inside){consider(triangle.facet,op.Interpolate(triangle.reference,w));located=true;break;}}
      if(!located)throw std::invalid_argument("Curve proposal is outside the inherited exact path.");
    } else {
      for(const auto& sheet:d.sheets)if(sheet.current.count(old))for(const auto& triangle:sheet.triangles){op.Work();const auto a=triangle.flat[0],b=triangle.flat[1],c=triangle.flat[2];
        const long double u[3]{b.x-(long double)a.x,b.y-(long double)a.y,b.z-(long double)a.z},v[3]{c.x-(long double)a.x,c.y-(long double)a.y,c.z-(long double)a.z},p[3]{node.p.x-(long double)a.x,node.p.y-(long double)a.y,node.p.z-(long double)a.z};
        long double uu=0,uv=0,vv=0,up=0,vp=0;for(unsigned k=0;k<3;++k){uu+=u[k]*u[k];uv+=u[k]*v[k];vv+=v[k]*v[k];up+=u[k]*p[k];vp+=v[k]*p[k];}
        const auto determinant=uu*vv-uv*uv;if(!(determinant>0))continue;
        long double w[3];w[1]=(up*vv-vp*uv)/determinant;w[2]=(vp*uu-up*uv)/determinant;w[0]=1-w[1]-w[2];
        for(auto& r:w)r=std::max(0.L,r);const auto sum=w[0]+w[1]+w[2];if(!(sum>0))continue;
        const auto first=op.Binary(double(w[1]/sum)),second=op.Binary(double(w[2]/sum));const auto remaining=op.Sub(op.Sub(op.Integer(1),first),second);
        if(op.Compare(remaining,op.Integer(0))<0)continue;
        const auto reference=op.Interpolate(triangle.reference,{remaining,first,second});
        // A projection onto an artificial chart boundary is only a proposal; exact remapping decides membership.
        const auto binding=Quantize(geometry,triangle.facet,reference,op);
        if(geometry.MobilityDimension(binding,policy)!=2)continue;
        consider(triangle.facet,reference);}}
    if(!best)throw std::invalid_argument("No representable proposal remains in the admitted atlas sheets.");
    result=*best;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}

bool RemapAtlasSurface(const CurvedAtlas& atlas,const std::vector<SU2Native3D::SurfaceFace>& faces,
    const SU2Native3D::BoundaryPoints& points,BoundaryCoverage& result,AtlasBudget& budget,std::string& reason) {
  BoundaryCoverage output;size_t fresh_bytes=0;FaceCoverage working;std::array<EdgeTrace,3> working_traces;EdgeTrace working_trace;
  try {const auto& d=Current(atlas);Ops op(budget,[&]{return Plus(Plus(Plus(Bytes(d),Plus(CoverageBytes(output),CoverageBytes(result))),fresh_bytes),
      Plus(FaceBytes(working),Plus(TraceBytes(working_traces),TraceBytes(working_trace))));});
    const auto& geometry=d.reference->Parameters()->Geometry();const auto& policy=d.reference->Parameters()->Mobility();
    if(faces.empty())throw std::invalid_argument("Empty candidate atlas surface.");
    Scratch lists(op,Times(faces.size(),sizeof(FaceCoverage)));
    std::vector<FaceCoverage> fresh;fresh.reserve(faces.size());
    for(const auto& face:faces){op.Work();const auto found=policy.sheets.find(face.facet);
      if(found==policy.sheets.end())throw std::invalid_argument("Candidate atlas anchor lacks a physical sheet.");
      std::array<SurfaceBinding,3> bindings;std::array<Point,3> realized;std::array<Key,3> locations;
      for(unsigned k=0;k<3;++k){const auto point=points.find(face.v[k]);const auto old=d.parameters.find(face.v[k]);
        const BoundaryPoint* endpoint=point==points.end()?(old==d.parameters.end()?nullptr:&old->second):&point->second;
        if(!endpoint||endpoint->node.id!=face.v[k])throw std::invalid_argument("Candidate atlas endpoint lacks its actual forward binding.");
        op.Check(d.backend->AssociationValid(endpoint->binding,endpoint->node.p,op.reason));
        bindings[k]=endpoint->binding;realized[k]=endpoint->node.p;locations[k]=op.BindingKey(geometry,bindings[k]);
        if(old!=d.parameters.end() && op.BindingKey(geometry,old->second.binding)!=locations[k])
          throw std::invalid_argument("Retained atlas node changed its binding without a new identity.");}
      const Sheet* sheet=nullptr;std::array<ExactBary,3> domain;
      for(const auto& candidate:d.sheets)if(candidate.identity==found->second&&candidate.body==policy.bodies.at(face.facet)&&candidate.marker==d.reference->Get(face.facet).marker) {
        std::array<ExactBary,3> mapped;bool inside=true;
        for(unsigned k=0;k<3&&inside;++k)inside=MapPoint(candidate,locations[k],*d.reference,mapped[k],op);
        if(!inside||op.Compare(op.Orient(mapped[0],mapped[1],mapped[2]),op.Integer(0))<=0)continue;
        if(sheet)throw std::invalid_argument("Candidate face has ambiguous local physical-sheet support.");sheet=&candidate;domain=std::move(mapped);
      }
      if(!sheet)throw std::invalid_argument("Candidate face leaves or reverses its admitted physical sheet.");
      auto& cover=working;cover={};
      for(const auto& triangle:sheet->triangles){op.Work();auto polygon=Clip(triangle,*sheet,domain,op);if(polygon.size()<3)continue;
        for(size_t i=1;i+1<polygon.size();++i){const std::array<ClipVertex,3> t{polygon[0],polygon[i],polygon[i+1]};
          const int sign=op.Compare(op.Orient(t[0].domain,t[1].domain,t[2].domain),op.Integer(0));if(!sign)continue;if(sign<0)throw std::logic_error("Atlas clip reversed a positive intersection.");
          CoveragePiece piece;piece.facet=triangle.facet;
          for(unsigned k=0;k<3;++k){piece.reference[k]=t[k].reference;const auto w=op.Bary(t[k].domain,domain);piece.active[k]={w[1],w[2]};}
          Reserve(cover.pieces,Plus(cover.pieces.size(),1),op);op.Sync(PieceBound(op));cover.pieces.push_back(std::move(piece));}}
      auto& traces=working_traces;op.Sync();op.Check(geometry.CoverageTraces(cover,traces,budget,op.reason));
      op.Check(geometry.ValidateCoverage(face.facet,bindings,realized,cover,traces,budget,op.reason));
      for(unsigned k=0;k<3;++k){const Edge oriented{face.v[k],face.v[(k+1)%3]},edge=Sorted(oriented[0],oriented[1]);
        auto& trace=working_trace;op.Sync(TraceBytes(traces[k]));trace=oriented[0]<oriented[1]?traces[k]:Reverse(traces[k],op);const auto existing=output.traces.find(edge);
        if(existing!=output.traces.end())op.Check(geometry.EqualTraces(existing->second,trace,budget,op.reason));
        else{for(const auto& old_sheet:d.sheets){
            const auto boundary=std::find_if(old_sheet.perimeter.begin(),old_sheet.perimeter.end(),[&](const Perimeter& p){return Sorted(p.first,p.last)==edge;});
            if(boundary!=old_sheet.perimeter.end())op.Check(geometry.EqualTraces(boundary->trace,trace,budget,op.reason));
          }
          op.Sync(Plus(sizeof(decltype(output.traces)::value_type)+4*sizeof(void*),Times(trace.pieces.size(),PieceBound(op))));output.traces.emplace(edge,std::move(trace));}}
      size_t copied=Times(cover.pieces.size(),sizeof(CoveragePiece));
      for(const auto& piece:cover.pieces)for(unsigned k=0;k<3;++k)copied=Plus(copied,Plus(BaryBytes(piece.reference[k]),BaryBytes(piece.active[k])));
      op.Sync(copied);fresh.push_back(cover);fresh_bytes=Plus(fresh_bytes,copied);
      const auto key=Ordered(face.v);const unsigned rotation=unsigned(std::min_element(face.v.begin(),face.v.end())-face.v.begin());
      if(rotation)for(auto& p:cover.pieces)for(auto& b:p.active){const std::array<ExactRatio,3> weights{op.Weight(b,0),b.u,b.v};b={weights[(rotation+1)%3],weights[(rotation+2)%3]};}
      op.Sync(sizeof(decltype(output.faces)::value_type)+4*sizeof(void*));if(!output.faces.emplace(key,std::move(cover)).second)throw std::invalid_argument("Candidate atlas repeats an oriented face.");}
    op.Check(geometry.ValidateCoverageReplacement(d.old,fresh,budget,op.reason));result=std::move(output);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
}  // namespace SU2NativeGeometry
