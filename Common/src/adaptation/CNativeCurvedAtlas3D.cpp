/*!
 * \file CNativeCurvedAtlas3D.cpp
 * \brief Fixed-reference edge contracts and fresh prescribed-trace face correspondence.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../include/adaptation/CNativeReferenceTriangulation3D.hpp"
#include "../../include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../include/adaptation/CNativeCanonicalTrace3D.hpp"
#include "../../include/adaptation/CNativeTraceCoverage3D.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <set>
#include <stdexcept>
#include <type_traits>

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
struct Vertex {Key key;};
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
  std::vector<SurfaceFace> faces;
  SU2Native3D::BoundaryPoints parameters;
};
namespace {
size_t Bytes(const CurvedAtlas::Data& data) {
  size_t n=Plus(sizeof(data),Times(data.sheets.capacity(),sizeof(Sheet)));
  if(data.backend)n=Plus(n,sizeof(NativeMappedGeometryBackend3D)+4*sizeof(void*));
  n=Plus(n,Times(data.old.capacity(),sizeof(FaceCoverage)));
  n=Plus(n,Times(data.faces.capacity(),sizeof(SurfaceFace)));
  n=Plus(n,Times(data.parameters.size(),sizeof(BoundaryPoints::value_type)+4*sizeof(void*)));
  for(const auto& cover:data.old){n=Plus(n,Times(cover.pieces.capacity(),sizeof(CoveragePiece)));
    for(const auto& p:cover.pieces)for(unsigned k=0;k<3;++k)n=Plus(n,Plus(BaryBytes(p.reference[k]),BaryBytes(p.active[k])));}
  for(const auto& s:data.sheets){n=Plus(n,Times(s.vertices.capacity(),sizeof(Vertex)));
    n=Plus(n,Times(s.triangles.capacity(),sizeof(Triangle)));n=Plus(n,Times(s.current.size(),sizeof(decltype(s.current)::value_type)+4*sizeof(void*)));
    n=Plus(n,Times(s.perimeter.capacity(),sizeof(Perimeter)));
    n=Plus(n,Times(s.faces.capacity(),sizeof(SurfaceFace)));
    for(const auto& v:s.vertices)n=Plus(n,Times(v.key.capacity(),sizeof(uint64_t)));
    for(const auto& t:s.triangles)for(const auto& p:t.reference)n=Plus(n,BaryBytes(p));
    for(const auto& e:s.perimeter){n=Plus(n,Times(e.trace.pieces.capacity(),sizeof(TracePiece)));
      for(const auto& p:e.trace.pieces)n=Plus(n,Plus(Plus(BaryBytes(p.first),BaryBytes(p.last)),RatioBytes(p.end)));}}
  return n;
}
struct Ops {
  AtlasBudget& budget;size_t saved,extra=0;std::function<size_t()> resident;std::string reason;
  Ops(AtlasBudget& b,std::function<size_t()> r):budget(b),saved(b.caller_bytes),resident(std::move(r)) {
    // Fixed arithmetic locals; delegated reconstruction admits its own scratch window.
    extra=CurvedAtlasWorkspaceBytes(b.maximum_integer_bits);Sync();
  }
  ~Ops(){budget.caller_bytes=saved;}
  void Work(size_t n=1){if(budget.work>budget.maximum_work || n>budget.maximum_work-budget.work)throw std::length_error("Curved atlas work exhausted.");budget.work+=n;}
  void Sync(size_t additional=0){const auto bytes=Plus(saved,Plus(resident(),Plus(extra,additional)));
    if(bytes>budget.maximum_bytes)throw std::length_error("Curved atlas storage exhausted.");
    budget.peak_bytes=std::max(budget.peak_bytes,bytes);budget.caller_bytes=bytes;}
  void Check(bool ok){if(!ok)throw std::runtime_error(reason);}
  template<class F> bool Delegate(F&& call) {
    const auto saved_extra=extra,saved_caller=budget.caller_bytes;
    extra-=CurvedAtlasWorkspaceBytes(budget.maximum_integer_bits);
    try{Sync();const bool ok=call();extra=saved_extra;budget.caller_bytes=saved_caller;return ok;}
    catch(...){extra=saved_extra;budget.caller_bytes=saved_caller;throw;}
  }
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
  sheet.vertices.push_back({std::move(key)});return sheet.vertices.size()-1;
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
std::optional<Id> TraceChain(const EdgeTrace& trace,const MappedFacetReference& geometry,const MobilityPolicy& policy,Ops& op) {
  std::optional<Id> chain;
  for(const auto& p:trace.pieces){op.Work();const auto midpoint=op.Lerp(p.first,p.last,op.Div(op.Integer(1),op.Integer(2)));
    const auto key=op.ReferenceKey(geometry,p.facet,midpoint);
    if(key.size()<3 || key[0]!=1)return {};
    const auto edge=policy.features.find({key[1],key[2]});
    if(edge==policy.features.end() || (chain&&*chain!=edge->second))return {};chain=edge->second;}
  return chain;
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

namespace {
SurfaceBinding Quantize(const MappedFacetReference&,Id,const ExactBary&,Ops&);
EdgeTrace Reverse(const EdgeTrace&,Ops&);
}

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
      auto& cover=working;cover={};op.Sync();op.Check(reference.ResolveCoverage(face,cover,budget,op.reason));
      std::array<SurfaceBinding,3> bindings;std::array<Point,3> realized;auto& traces=working_traces;
      for(unsigned k=0;k<3;++k){bindings[k]=data->parameters.at(face.v[k]).binding;realized[k]=data->parameters.at(face.v[k]).node.p;}
      std::array<BoundaryPoint,3> points;for(unsigned k=0;k<3;++k)points[k]=data->parameters.at(face.v[k]);
      if(!reference.HasAcceptedFaceCertificate(face,points,accepted.coverage)) {
        for(unsigned k=0;k<3;++k){const Edge edge{face.v[k],face.v[(k+1)%3]};
          op.Check(reference.ResolveTrace(edge,face,traces[k],budget,op.reason));
          if(edge[0]>edge[1])traces[k]=Reverse(traces[k],op);}
        op.Check(geometry.ValidateCoverage(face.facet,bindings,realized,cover,traces,budget,op.reason));
      }
      for(const auto& p:cover.pieces)if(policy.sheets.at(p.facet)!=sheet_id->second||policy.bodies.at(p.facet)!=body->second||reference.Get(p.facet).marker!=reference.Get(face.facet).marker)
        throw std::invalid_argument("Atlas face crosses a protected sheet/body boundary.");
      Reserve(data->old,Plus(data->old.size(),1),op);data->old.push_back(std::move(cover));
      Reserve(data->faces,Plus(data->faces.size(),1),op);data->faces.push_back(face);
      if(!moving||std::find(face.v.begin(),face.v.end(),*moving)==face.v.end())continue;
      auto at=std::find_if(data->sheets.begin(),data->sheets.end(),[&](const Sheet& s){return s.identity==sheet_id->second;});
      if(at==data->sheets.end()){Reserve(data->sheets,Plus(data->sheets.size(),1),op);data->sheets.push_back({});at=std::prev(data->sheets.end());
        at->identity=sheet_id->second;at->body=body->second;at->marker=reference.Get(face.facet).marker;}
      auto& sheet=*at;if(sheet.body!=body->second||sheet.marker!=reference.Get(face.facet).marker)throw std::invalid_argument("Atlas sheet merges distinct bodies/markers.");
      for(unsigned k=0;k<3;++k){const unsigned j=(k+1)%3;const auto a=realized[k],b=realized[j];
        sheet.normal.x+=a.y*b.z-a.z*b.y;sheet.normal.y+=a.z*b.x-a.x*b.z;sheet.normal.z+=a.x*b.y-a.y*b.x;}
      Reserve(sheet.faces,Plus(sheet.faces.size(),1),op);sheet.faces.push_back(face);
      for(unsigned k=0;k<3;++k){const auto vertex=AddVertex(sheet,op.BindingKey(geometry,bindings[k]),op);op.Sync(sizeof(decltype(sheet.current)::value_type)+4*sizeof(void*));
        const auto inserted=sheet.current.emplace(face.v[k],vertex);if(!inserted.second && inserted.first->second!=vertex)throw std::invalid_argument("Atlas endpoint has inconsistent chart locations.");}
      }
    SplitLocalSheets(*data,reference,working_traces[0],op);
    Scratch incidence_storage(op,Times(accepted.surface.size(),Plus(Times(3,sizeof(std::pair<const Edge,size_t>)+4*sizeof(void*)),sizeof(std::pair<const Face,size_t>)+4*sizeof(void*))));
    std::map<Edge,size_t> incidence;std::map<Face,size_t> source_regions;
    for(size_t i=0;i<accepted.surface.size();++i)if(!source_regions.emplace(Ordered(accepted.surface[i].v),i).second)
      throw std::invalid_argument("Atlas repeats an accepted physical face.");
    for(const auto& face:accepted.surface)for(unsigned k=0;k<3;++k){op.Work();++incidence[Sorted(face.v[k],face.v[(k+1)%3])];}
    for(auto& sheet:data->sheets){const auto cycle=BoundaryCycle(sheet,op);
      if(moving&&geometry.MobilityDimension(data->parameters.at(*moving).binding,policy)==2&&
          std::find(cycle.begin(),cycle.end(),*moving)!=cycle.end())
        throw std::invalid_argument("A movable smooth point requires its complete physical sheet star.");
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
      std::optional<ExactRatio> first_length,last_length;std::optional<Id> first_chain,last_chain;
      for(auto& edge:sheet.perimeter)if(incidence.at(Sorted(edge.first,edge.last))==2&&TraceChain(edge.trace,geometry,policy,op)) {
        const auto chain=TraceChain(edge.trace,geometry,policy,op);
        const auto length=CleanFeatureTrace(edge.trace,geometry,policy,op);
        if(first_neighbor&&Sorted(edge.first,edge.last)==Sorted(*first_neighbor,*moving)){first_length=length;first_chain=chain;}
        if(last_neighbor&&Sorted(edge.first,edge.last)==Sorted(*moving,*last_neighbor)){last_length=length;last_chain=chain;}
      }
      if(first_neighbor) {
        if(!first_length||!last_length)throw std::invalid_argument("Curve reconstruction lacks all incident faces.");
        if(first_chain!=last_chain)throw std::invalid_argument("Curve sample crosses distinct genuine feature chains.");
        sheet.moving_run=std::array<Id,3>{*first_neighbor,*moving,*last_neighbor};
      }
      if(!moving)continue;
      // Proposal geometry uses a fresh reference mesh; it never defines published edge paths.
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
      Conform(sheet,reference,op);
      Scratch embedding(op,Times(sheet.triangles.size(),PieceBound(op)));std::vector<CoveragePiece> region;region.reserve(sheet.triangles.size());
      for(const auto& t:sheet.triangles)region.push_back({t.facet,t.reference,{}});
      op.Check(geometry.CertifyEmbedding(region,sheet.normal,budget,op.reason));}
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
bool InSheet(const Sheet& s,const Key& key,const PlanarReference& reference,Ops& op) {
  for(const auto& t:s.triangles){op.Work();ExactBary point;if(!Parameters(key,reference.Get(t.facet),point,op))continue;
    const auto bary=op.Bary(point,t.reference);bool inside=true;for(const auto& w:bary)inside=inside&&op.Compare(w,op.Integer(0))>=0;
    if(inside)return true;}
  return false;
}
bool TracePosition(const EdgeTrace& trace,const Key& key,const PlanarReference& reference,ExactRatio& result,Ops& op) {
  auto begin=op.Integer(0);std::optional<ExactRatio> found;
  for(const auto& piece:trace.pieces){op.Work();ExactBary point;
    if(Parameters(key,reference.Get(piece.facet),point,op)&&!op.Compare(op.Orient(piece.first,piece.last,point),op.Integer(0))) {
      const bool u=op.Compare(piece.first.u,piece.last.u)!=0;
      const auto fraction=op.Div(op.Sub(u?point.u:point.v,u?piece.first.u:piece.first.v),
                                 op.Sub(u?piece.last.u:piece.last.v,u?piece.first.u:piece.first.v));
      if(op.Compare(fraction,op.Integer(0))>=0&&op.Compare(fraction,op.Integer(1))<=0){
        auto at=op.Add(begin,op.Mul(fraction,op.Sub(piece.end,begin)));
        if(found&&op.Compare(*found,at))throw std::invalid_argument("Reference trace revisits an association.");found=std::move(at);}
    }
    begin=piece.end;
  }
  if(!found)return false;result=std::move(*found);return true;
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
  if(!dimension||geometry.MobilityDimension(binding,policy)!=dimension)throw std::invalid_argument("Atlas move changes physical feature dimension.");
  const auto key=op.BindingKey(geometry,binding);size_t incident=0;
  for(const auto& sheet:d.sheets)if(sheet.current.count(old)) {
    ++incident;if(!InSheet(sheet,key,*d.reference,op))throw std::invalid_argument("Atlas move leaves an incident reference region.");
    if(dimension==1){if(!sheet.moving_run)throw std::invalid_argument("Curve move lacks its complete incident feature run.");
      const auto run=*sheet.moving_run;
      if(key==op.BindingKey(geometry,d.parameters.at(run[0]).binding)||key==op.BindingKey(geometry,d.parameters.at(run[2]).binding))
        throw std::invalid_argument("Curve move reaches a retained endpoint.");
      bool contained=false;
      for(const auto& edge:sheet.perimeter)if(Sorted(edge.first,edge.last)==Sorted(run[0],old)||Sorted(edge.first,edge.last)==Sorted(old,run[2])) {
        ExactRatio at;if(TracePosition(edge.trace,key,*d.reference,at,op))contained=true;}
      if(!contained)throw std::invalid_argument("Curve move leaves its prescribed feature interval.");
    }
  }
  if(!incident)throw std::invalid_argument("Atlas endpoint has no admitted sheet.");
}
EdgeTrace Reverse(const EdgeTrace& trace,Ops& op) {
  auto previous=op.Integer(0);const auto one=op.Integer(1);
  if(trace.pieces.empty())throw std::invalid_argument("Empty curved trace before reversal.");
  for(const auto& p:trace.pieces){if(op.Compare(previous,p.end)>=0||op.Compare(p.end,one)>0)
      throw std::invalid_argument("Invalid curved trace interval before reversal.");previous=p.end;}
  if(op.Compare(previous,one))throw std::invalid_argument("Incomplete curved trace before reversal.");
  Scratch scratch(op,Times(trace.pieces.size(),PieceBound(op)));EdgeTrace out;out.pieces.reserve(trace.pieces.size());
  for(size_t i=trace.pieces.size();i;--i){op.Work();auto p=trace.pieces[i-1];std::swap(p.first,p.last);
    p.end=op.Sub(op.Integer(1),i>1?trace.pieces[i-2].end:op.Integer(0));out.pieces.push_back(std::move(p));}return out;
}
EdgeTrace FeatureRun(const Sheet& sheet,const CurvedAtlas::Data& d,Ops& op) {
  if(!sheet.moving_run)throw std::invalid_argument("Missing incident feature run.");
  EdgeTrace result;const auto run=*sheet.moving_run;
  for(unsigned i=0;i<2;++i){const auto edge=Sorted(run[i],run[i+1]);
    const auto found=std::find_if(sheet.perimeter.begin(),sheet.perimeter.end(),[&](const Perimeter& p){return Sorted(p.first,p.last)==edge;});
    if(found==sheet.perimeter.end())throw std::invalid_argument("Feature run lacks an inherited edge.");
    auto trace=run[i]<run[i+1]?found->trace:Reverse(found->trace,op);
    Reserve(result.pieces,Plus(result.pieces.size(),trace.pieces.size()),op);
    op.Sync(Plus(TraceBytes(result),TraceBytes(trace)));
    result.pieces.insert(result.pieces.end(),trace.pieces.begin(),trace.pieces.end());
  }
  CleanFeatureTrace(result,d.reference->Parameters()->Geometry(),d.reference->Parameters()->Mobility(),op);return result;
}
bool SliceFeature(const EdgeTrace& input,const Key& first,const Key& last,const CurvedAtlas::Data& d,EdgeTrace& result,Ops& op) {
  ExactRatio a,b;if(!TracePosition(input,first,*d.reference,a,op)||!TracePosition(input,last,*d.reference,b,op))return false;
  if(!op.Compare(a,b))throw std::invalid_argument("Canonical feature edge has coincident endpoints.");
  const bool reverse=op.Compare(a,b)>0;auto trace=reverse?Reverse(input,op):input;
  if(reverse){a=op.Sub(op.Integer(1),a);b=op.Sub(op.Integer(1),b);}
  EdgeTrace output;auto begin=op.Integer(0);
  for(const auto& piece:trace.pieces){const auto left=op.Compare(a,begin)>0?a:begin,right=op.Compare(b,piece.end)<0?b:piece.end;
    if(op.Compare(left,right)<0){const auto width=op.Sub(piece.end,begin);
      auto u=op.Div(op.Sub(left,begin),width),v=op.Div(op.Sub(right,begin),width);
      Reserve(output.pieces,Plus(output.pieces.size(),1),op);op.Sync(TraceBytes(output));
      output.pieces.push_back({piece.facet,op.Lerp(piece.first,piece.last,u),op.Lerp(piece.first,piece.last,v),op.Div(op.Sub(right,a),op.Sub(b,a))});}
    begin=piece.end;
  }
  CleanFeatureTrace(output,d.reference->Parameters()->Geometry(),d.reference->Parameters()->Mobility(),op);result=std::move(output);return true;
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
      auto path=FeatureRun(*found,d,op);const auto target=op.Binary(parameter);auto begin=op.Integer(0);bool located=false;
      for(const auto& piece:path.pieces){if(op.Compare(target,piece.end)<=0){
          const auto t=op.Div(op.Sub(target,begin),op.Sub(piece.end,begin));consider(piece.facet,op.Lerp(piece.first,piece.last,t));located=true;break;}begin=piece.end;}
      if(!located)throw std::invalid_argument("Curve proposal leaves its immutable feature interval.");
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
  BoundaryCoverage output;FaceCoverage region;EdgeTrace working;
  std::map<Edge,std::pair<size_t,EdgeTrace>> inherited;
  std::map<Edge,std::vector<size_t>> incidence;
  std::map<Id,SurfaceBinding> bindings;
  Key first,last;
  try {const auto& d=Current(atlas);
    Ops op(budget,[&]{size_t bytes=Plus(Bytes(d),Plus(CoverageBytes(output),CoverageBytes(result)));
      bytes=Plus(bytes,Plus(FaceBytes(region),TraceBytes(working)));
      bytes=Plus(bytes,Times(Plus(first.capacity(),last.capacity()),sizeof(uint64_t)));
      bytes=Plus(bytes,Times(bindings.size(),sizeof(decltype(bindings)::value_type)+4*sizeof(void*)));
      bytes=Plus(bytes,Times(inherited.size(),sizeof(decltype(inherited)::value_type)+4*sizeof(void*)));
      bytes=Plus(bytes,Times(incidence.size(),sizeof(decltype(incidence)::value_type)+4*sizeof(void*)));
      for(const auto& row:inherited)bytes=Plus(bytes,TraceBytes(row.second.second));
      for(const auto& row:incidence)bytes=Plus(bytes,Times(row.second.capacity(),sizeof(size_t)));return bytes;});
    const auto& geometry=d.reference->Parameters()->Geometry();const auto& policy=d.reference->Parameters()->Mobility();
    if(faces.empty())throw std::invalid_argument("Empty candidate atlas surface.");
    for(size_t i=0;i<faces.size();++i){const auto& face=faces[i];
      if(!policy.sheets.count(face.facet))throw std::invalid_argument("Candidate face has no immutable physical sheet.");
      for(unsigned k=0;k<3;++k){const Id id=face.v[k];const auto supplied=points.find(id),old=d.parameters.find(id);
        const BoundaryPoint* point=supplied==points.end()?(old==d.parameters.end()?nullptr:&old->second):&supplied->second;
        if(!point||point->node.id!=id)throw std::invalid_argument("Candidate face lacks its actual forward binding.");
        op.Check(d.backend->AssociationValid(point->binding,point->node.p,op.reason));
        if(old!=d.parameters.end()&&op.BindingKey(geometry,old->second.binding)!=op.BindingKey(geometry,point->binding))
          throw std::invalid_argument("Retained atlas point changed its binding without a new identity.");
        op.Sync(sizeof(decltype(bindings)::value_type)+4*sizeof(void*));bindings.emplace(id,point->binding);
        const auto edge=Sorted(id,face.v[(k+1)%3]);op.Sync(sizeof(decltype(incidence)::value_type)+4*sizeof(void*)+2*sizeof(size_t));
        auto& sides=incidence[edge];if(sides.size()==2)throw std::invalid_argument("Candidate edge has nonmanifold incidence.");sides.push_back(i);
      }
    }
    if(d.moving){size_t fresh=0;for(const auto& row:bindings)if(!d.parameters.count(row.first)){Movement(d,*d.moving,row.second,op);++fresh;}
      if(bindings.count(*d.moving)||fresh!=1)throw std::invalid_argument("Boundary movement lacks one new complete point identity.");}
    for(const auto& face:d.faces)for(unsigned k=0;k<3;++k){const auto edge=Sorted(face.v[k],face.v[(k+1)%3]);
      auto found=inherited.find(edge);
      if(found==inherited.end()){working={};op.Check(d.reference->ResolveTrace(edge,face,working,budget,op.reason));
        op.Sync(Plus(sizeof(decltype(inherited)::value_type)+4*sizeof(void*),TraceBytes(working)));
        inherited.emplace(edge,std::make_pair(size_t(1),std::move(working)));}
      else if(++found->second.first>2)throw std::invalid_argument("Source edge has nonmanifold incidence.");
    }
    std::vector<Edge> feature_edges;
    Scratch feature_storage(op,Times(inherited.size(),sizeof(Edge)));feature_edges.reserve(inherited.size());
    for(auto& row:inherited)if(TraceChain(row.second.second,geometry,policy,op)){
      CleanFeatureTrace(row.second.second,geometry,policy,op);feature_edges.push_back(row.first);}
    auto feature_path=[&](const Key& a,const Key& b,EdgeTrace& path)->bool {
      std::optional<EdgeTrace> chosen;
      auto accept=[&](EdgeTrace candidate){CleanFeatureTrace(candidate,geometry,policy,op);
        if(chosen)op.Check(geometry.EqualTraces(*chosen,candidate,budget,op.reason));else chosen=std::move(candidate);};
      size_t route_bytes=0;for(const auto& edge:feature_edges)route_bytes=Plus(route_bytes,TraceBytes(inherited.at(edge).second));
      Scratch route_storage(op,Plus(Times(8,route_bytes),Times(Plus(feature_edges.size(),1),16*sizeof(Id))));
      for(const auto& seed:feature_edges){const auto& source=inherited.at(seed).second;ExactRatio at;
        if(!TracePosition(source,a,*d.reference,at,op))continue;
        EdgeTrace direct;if(SliceFeature(source,a,b,d,direct,op)){accept(std::move(direct));continue;}
        for(Id endpoint:seed){const auto end_key=op.BindingKey(geometry,d.parameters.at(endpoint).binding);if(end_key==a)continue;
          EdgeTrace route;if(!SliceFeature(source,a,end_key,d,route,op))continue;
          Edge previous=seed;Id current=endpoint;std::set<Id> visited;
          while(!bindings.count(current)&&visited.insert(current).second){
            std::optional<Edge> next;
            for(const auto& edge:feature_edges)if(edge!=previous&&(edge[0]==current||edge[1]==current)){
              if(next)throw std::invalid_argument("Feature reconstruction reaches a retired branch.");next=edge;}
            if(!next)break;
            const auto& tail=inherited.at(*next).second;const auto current_key=op.BindingKey(geometry,d.parameters.at(current).binding);
            EdgeTrace part;const bool done=SliceFeature(tail,current_key,b,d,part,op);
            const Id other=(*next)[0]==current?(*next)[1]:(*next)[0];
            if(!done){const auto other_key=op.BindingKey(geometry,d.parameters.at(other).binding);
              if(!SliceFeature(tail,current_key,other_key,d,part,op))throw std::invalid_argument("Feature route lacks its endpoint.");}
            Reserve(route.pieces,Plus(route.pieces.size(),part.pieces.size()),op);op.Sync(Plus(TraceBytes(route),TraceBytes(part)));
            route.pieces.insert(route.pieces.end(),part.pieces.begin(),part.pieces.end());
            if(done){accept(std::move(route));break;}previous=*next;current=other;
          }
        }
      }
      if(!chosen)return false;path=std::move(*chosen);return true;
    };
    for(const auto& row:incidence){const auto& edge=row.first;working={};const auto old=inherited.find(edge);
      if(old!=inherited.end()&&old->second.first==1)working=old->second.second;
      else {
        if(row.second.size()!=2)throw std::invalid_argument("Replacement changes an uncoordinated physical interface edge.");
        first=op.BindingKey(geometry,bindings.at(edge[0]));last=op.BindingKey(geometry,bindings.at(edge[1]));
        if(!feature_path(first,last,working)){
          const auto& face=faces.at(row.second.front());const auto sheet=policy.sheets.at(face.facet);
          for(size_t side:row.second)if(policy.sheets.at(faces.at(side).facet)!=sheet)
            throw std::invalid_argument("A shared edge crosses physical sheets without a genuine feature path.");
          op.Check(op.Delegate([&]{return CanonicalSurfaceTrace(geometry,policy,sheet,bindings.at(edge[0]),bindings.at(edge[1]),working,budget,op.reason);}));
        }
      }
      op.Sync(Plus(sizeof(decltype(output.traces)::value_type)+4*sizeof(void*),TraceBytes(working)));
      output.traces.emplace(edge,std::move(working));
    }
    for(const auto& cover:d.old)for(const auto& piece:cover.pieces){Reserve(region.pieces,Plus(region.pieces.size(),1),op);op.Sync(PieceBound(op));
      region.pieces.push_back({piece.facet,piece.reference,{}});}
    op.Check(op.Delegate([&]{return RebuildTraceCoverage(geometry,region.pieces,faces,bindings,output.traces,output.faces,budget,op.reason);}));
    result=std::move(output);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool RebuildWorkingSurface(const SU2Native3D::BoundaryParameterState& state,BoundaryCoverage& result,
                           AtlasBudget& budget,std::string& reason) {
  using namespace SU2Native3D;
  if(!state.Mobility().identity||(state.Coverage().faces.empty()&&state.Coverage().traces.empty())) {
    result={};reason.clear();return true;
  }
  BoundaryCoverage output;EdgeTrace inherited,canonical;FaceCoverage cover;
  std::vector<CoveragePiece> region;std::vector<SurfaceFace> faces;std::map<Id,SurfaceBinding> bindings;
  std::map<Edge,EdgeTrace> replacements;std::set<Face> selected;std::set<Id> visited;
  size_t nodes=0,region_limbs=0;
  try {
    Ops op(budget,[&]{return Plus(nodes,Plus(region_limbs,Plus(Times(region.capacity(),sizeof(CoveragePiece)),
        Plus(Times(faces.capacity(),sizeof(SurfaceFace)),Plus(FaceBytes(cover),Plus(TraceBytes(inherited),
          Plus(TraceBytes(canonical),Plus(BoundaryCoverageBytes(output),BoundaryCoverageBytes(result)))))))));});
    const auto& geometry=state.Geometry();const auto& policy=state.Mobility();
    auto insert=[&](auto& set,const auto& value){if(set.count(value))return false;
      const auto bytes=sizeof(typename std::decay_t<decltype(set)>::value_type)+4*sizeof(void*);
      op.Sync(bytes);set.insert(value);nodes=Plus(nodes,bytes);return true;};
    auto key=[](Face face){std::sort(face.begin(),face.end());return face;};
    auto ratio_equal=[](const ExactRatio& a,const ExactRatio& b){return a.negative==b.negative&&a.numerator==b.numerator&&a.denominator==b.denominator;};
    auto bary_equal=[&](const ExactBary& a,const ExactBary& b){return ratio_equal(a.u,b.u)&&ratio_equal(a.v,b.v);};
    auto identical=[&](const EdgeTrace& a,const EdgeTrace& b){if(a.pieces.size()!=b.pieces.size())return false;
      for(size_t k=0;k<a.pieces.size();++k){const auto& x=a.pieces[k];const auto& y=b.pieces[k];
        if(x.facet!=y.facet||!bary_equal(x.first,y.first)||!bary_equal(x.last,y.last)||!ratio_equal(x.end,y.end))return false;}return true;};
    auto complexity=[](const EdgeTrace& trace){size_t limbs=0,width=0;
      auto ratio=[&](const ExactRatio& r){limbs=Plus(limbs,Plus(r.numerator.size(),r.denominator.size()));width=std::max({width,r.numerator.size(),r.denominator.size()});};
      for(const auto& piece:trace.pieces){for(const auto* b:{&piece.first,&piece.last}){ratio(b->u);ratio(b->v);}ratio(piece.end);}
      return std::array<size_t,3>{{trace.pieces.size(),limbs,width}};};
    auto footprint=[&](Id facet,Id body){
      for(const auto& node:geometry.Reference().Get(facet).v)geometry.ForFacets(geometry.Vertex(node.id),[&](Id support){
        if(policy.bodies.at(support)!=body||!insert(visited,support))return;
        const auto range=state.FacesOnOriginalFacet(support);
        for(auto it=range.first;it!=range.second;++it){op.Work();insert(selected,it->second);}
      });
    };
    auto plan=[&](Edge edge){
      op.Work();const auto incident=state.IncidentFaces(edge);
      if(!incident[0]||!incident[1])throw std::invalid_argument("Working surface refresh lacks both incident physical faces.");
      inherited={};op.Sync();op.Check(state.ResolveTrace(edge,*incident[0],inherited,budget,op.reason));
      canonical={};
      if(TraceChain(inherited,geometry,policy,op)) {
        // Keep the oriented model arc while discarding obsolete subdivision and active parameters.
        op.Sync(TraceBytes(inherited));canonical=inherited;op.Sync();CleanFeatureTrace(canonical,geometry,policy,op);
      } else {
        const auto sheet=policy.sheets.at(incident[0]->facet);
        if(policy.sheets.at(incident[1]->facet)!=sheet)throw std::invalid_argument("Working surface refresh crosses protected sheets.");
        const auto* a=state.Find(edge[0]);const auto* b=state.Find(edge[1]);
        if(!a||!b)throw std::invalid_argument("Working surface refresh lacks current endpoint associations.");
        op.Check(op.Delegate([&]{return CanonicalSurfaceTrace(geometry,policy,sheet,a->binding,b->binding,canonical,budget,op.reason);}));
      }
      if(identical(inherited,canonical))return;
      bool equal=false;op.Sync();op.Check(geometry.CompareTraces(inherited,canonical,equal,budget,op.reason));
      const auto old_cost=complexity(inherited),fresh_cost=complexity(canonical);
      if(equal&&old_cost[0]<=fresh_cost[0]&&old_cost[1]<=fresh_cost[1]&&old_cost[2]<=fresh_cost[2])return;
      for(const auto* face:incident)insert(selected,key(face->v));
      const Id body=policy.bodies.at(incident[0]->facet);for(const auto& piece:canonical.pieces)footprint(piece.facet,body);
      const auto bytes=Plus(sizeof(decltype(replacements)::value_type)+4*sizeof(void*),TraceBytes(canonical));
      op.Sync(bytes);nodes=Plus(nodes,bytes);replacements.emplace(edge,std::move(canonical));
    };
    // Stream current explicit contracts. The incidence index avoids a global temporary edge table.
    for(const auto& row:state.Coverage().traces)plan(row.first);
    for(const auto& row:state.Coverage().faces)for(unsigned k=0;k<3;++k){const auto edge=Sorted(row.first[k],row.first[(k+1)%3]);
      if(state.Coverage().traces.count(edge))continue;
      const auto adjacent=state.IncidentFaces(edge);bool first=true;
      for(const auto* other:adjacent)if(other&&key(other->v)<key(row.first)&&state.Coverage().faces.count(Ordered(other->v)))first=false;
      if(first)plan(edge);
    }
    if(replacements.empty()){result={};reason.clear();return true;}
    op.Sync(Times(selected.size(),sizeof(SurfaceFace)));faces.reserve(selected.size());
    for(const auto& id:selected){const auto& face=state.Surface().at(id);faces.push_back(face);
      cover={};op.Sync();op.Check(state.ResolveCoverage(face,cover,budget,op.reason));
      for(const auto& piece:cover.pieces){size_t limbs=0;for(const auto* triangle:{&piece.reference,&piece.active})for(const auto& b:*triangle)limbs=Plus(limbs,BaryBytes(b));
        Reserve(region,Plus(region.size(),1),op);op.Sync(limbs);region.push_back(piece);region_limbs=Plus(region_limbs,limbs);}
      for(unsigned k=0;k<3;++k){const auto node=face.v[k];if(!bindings.count(node)){
          const auto* point=state.Find(node);if(!point)throw std::invalid_argument("Working surface refresh lost an association.");
          const auto bytes=sizeof(decltype(bindings)::value_type)+4*sizeof(void*);op.Sync(bytes);bindings.emplace(node,point->binding);nodes=Plus(nodes,bytes);}
        const auto edge=Sorted(node,face.v[(k+1)%3]);if(output.traces.count(edge))continue;
        const auto changed=replacements.find(edge);inherited={};
        if(changed!=replacements.end()){op.Sync(TraceBytes(changed->second));inherited=changed->second;}
        else {op.Sync();op.Check(state.ResolveTrace(edge,face,inherited,budget,op.reason));}
        op.Sync(Plus(TraceBytes(inherited),sizeof(decltype(output.traces)::value_type)+4*sizeof(void*)));output.traces.emplace(edge,inherited);
      }
    }
    op.Sync();op.Check(op.Delegate([&]{return RebuildTraceCoverage(geometry,region,faces,bindings,output.traces,output.faces,budget,op.reason);}));
    result=std::move(output);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
}  // namespace SU2NativeGeometry
