/*!
 * \file CNativeReferenceTriangulation3D.cpp
 * \brief Fresh exact planar constraints, corridor recovery and reference-region winding.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeReferenceTriangulation3D.hpp"
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include <algorithm>
#include <climits>
#include <functional>
#include <memory>
#include <set>
#include <stdexcept>

namespace SU2NativeGeometry {
namespace {
using Id=SU2Native3D::Id;
using Key=std::vector<uint64_t>;
using Triangle=std::array<size_t,3>;
using Edge=std::array<size_t,2>;
constexpr size_t NoTrace=SIZE_MAX;
size_t Add(size_t a,size_t b){if(b>SIZE_MAX-a)throw std::length_error("Reference triangulation size overflow.");return a+b;}
size_t Mul(size_t a,size_t b){if(a&&b>SIZE_MAX/a)throw std::length_error("Reference triangulation size overflow.");return a*b;}
size_t Bytes(const ExactRatio& r){return Mul(Add(r.numerator.capacity(),r.denominator.capacity()),sizeof(uint32_t));}
size_t Bytes(const ExactBary& p){return Add(Bytes(p.u),Bytes(p.v));}
size_t Bytes(const std::vector<CoveragePiece>& pieces){size_t bytes=Mul(pieces.capacity(),sizeof(CoveragePiece));
  for(const auto& p:pieces)for(unsigned k=0;k<3;++k)bytes=Add(bytes,Add(Bytes(p.reference[k]),Bytes(p.active[k])));return bytes;}
size_t Bytes(const std::vector<RegionSegment>& segments){size_t bytes=Mul(segments.capacity(),sizeof(RegionSegment));
  for(const auto& s:segments)bytes=Add(bytes,Add(Bytes(s.first),Bytes(s.last)));return bytes;}
Edge Sorted(size_t a,size_t b){return a<b?Edge{a,b}:Edge{b,a};}
struct Segment {ExactBary first,last;size_t trace=NoTrace;};
struct Mesh {
  std::vector<ExactBary> seeds,points;
  std::vector<Triangle> triangles;
  std::vector<Segment> segments;
  std::vector<Edge> constraints;
};
size_t Bytes(const Mesh& m){size_t bytes=Mul(Add(m.seeds.capacity(),m.points.capacity()),sizeof(ExactBary));
  bytes=Add(bytes,Mul(m.triangles.capacity(),sizeof(Triangle)));bytes=Add(bytes,Mul(m.segments.capacity(),sizeof(Segment)));
  bytes=Add(bytes,Mul(m.constraints.capacity(),sizeof(Edge)));
  for(const auto& p:m.seeds)bytes=Add(bytes,Bytes(p));for(const auto& p:m.points)bytes=Add(bytes,Bytes(p));
  for(const auto& s:m.segments)bytes=Add(bytes,Add(Bytes(s.first),Bytes(s.last)));return bytes;}
struct Ops {
  AtlasBudget& budget;size_t saved,extra;std::function<size_t()> resident;std::string reason;
  Ops(AtlasBudget& b,std::function<size_t()> r):budget(b),saved(b.caller_bytes),extra(CurvedAtlasWorkspaceBytes(b.maximum_integer_bits)),resident(std::move(r)){Sync();}
  ~Ops(){budget.caller_bytes=saved;}
  void Work(size_t count=1){if(!budget.ChargeWork(count))throw std::length_error("Reference triangulation work exhausted.");}
  void Sync(size_t additional=0){const auto bytes=Add(saved,Add(resident(),Add(extra,additional)));
    if(bytes>budget.maximum_bytes)throw std::length_error("Reference triangulation storage exhausted.");budget.peak_bytes=std::max(budget.peak_bytes,bytes);budget.caller_bytes=bytes;}
  void Check(bool ok){if(!ok)throw std::invalid_argument(reason);}
  ExactRatio Integer(int64_t x){Sync();ExactRatio r;Check(ExactInteger(x,r,budget,reason));return r;}
  ExactRatio AddR(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactAdd(a,b,r,budget,reason));return r;}
  ExactRatio Sub(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactSubtract(a,b,r,budget,reason));return r;}
  ExactRatio Product(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactMultiply(a,b,r,budget,reason));return r;}
  ExactRatio Divide(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactDivide(a,b,r,budget,reason));return r;}
  int Compare(const ExactRatio& a,const ExactRatio& b){Sync();int order;Check(ExactCompare(a,b,order,budget,reason));return order;}
  bool Same(const ExactRatio& a,const ExactRatio& b){Work(Add(Add(a.numerator.size(),b.numerator.size()),Add(a.denominator.size(),b.denominator.size())));
    return a.negative==b.negative&&a.numerator==b.numerator&&a.denominator==b.denominator;}
  bool Same(const ExactBary& a,const ExactBary& b){return Same(a.u,b.u)&&Same(a.v,b.v);}
  static int Sign(const ExactRatio& x){return x.numerator.empty()?0:x.negative?-1:1;}
  ExactRatio Orient(const ExactBary& a,const ExactBary& b,const ExactBary& c){return Sub(Product(Sub(b.u,a.u),Sub(c.v,a.v)),Product(Sub(b.v,a.v),Sub(c.u,a.u)));}
  int Side(const ExactBary& a,const ExactBary& b,const ExactBary& c){return Sign(Orient(a,b,c));}
  bool On(const ExactBary& a,const ExactBary& b,const ExactBary& p){if(Side(a,b,p))return false;
    const auto& first=Same(a.u,b.u)?a.v:a.u;const auto& last=Same(a.u,b.u)?b.v:b.u;const auto& x=Same(a.u,b.u)?p.v:p.u;
    return Compare(x,first)*Compare(x,last)<=0;}
  void Bary(const ExactBary& p){Sync();Check(ExactValidate(p.u,budget,reason));Check(ExactValidate(p.v,budget,reason));
    if(Sign(p.u)<0||Sign(p.v)<0||Compare(AddR(p.u,p.v),Integer(1))>0)throw std::invalid_argument("Reference constraint leaves its original simplex.");}
  Key ReferenceKey(const MappedFacetReference& g,Id facet,const ExactBary& p){Sync();Key key;Check(g.ReferencePointKey(facet,p,key,budget,reason));return key;}
  Key BindingKey(const MappedFacetReference& g,const SurfaceBinding& b){Key key;g.ForFacets(b,[&](Id facet){if(!key.empty())return;ExactBary p;Sync();Check(g.BindingParameters(facet,b,p,budget,reason));key=ReferenceKey(g,facet,p);});
    if(key.empty())throw std::invalid_argument("Current binding has no immutable support.");return key;}
  bool Parameters(const MappedFacetReference& g,Id facet,const Key& key,ExactBary& p){Sync();const bool supported=g.ReferenceParameters(facet,key,p,budget,reason);
    if(!supported&&!reason.empty())throw std::invalid_argument(reason);return supported;}
};
struct Scratch {Ops& op;size_t bytes;Scratch(Ops& o,size_t n):op(o),bytes(n){op.Sync(n);op.extra=Add(op.extra,n);}~Scratch(){op.extra-=bytes;}};
template<class T>void Reserve(std::vector<T>& v,size_t count,Ops& op){if(count<=v.capacity())return;const auto next=std::max(count,Add(v.capacity(),std::max(size_t(1),v.capacity())));
  op.Sync(Mul(next,sizeof(T)));v.reserve(next);}
size_t Find(const Mesh& m,const ExactBary& p,Ops& op){for(size_t i=0;i<m.points.size();++i)if(op.Same(m.points[i],p))return i;throw std::logic_error("Reference constraint endpoint was not inserted.");}
bool HasEdge(const Mesh& m,Edge edge){for(const auto& t:m.triangles)for(unsigned k=0;k<3;++k)if(Sorted(t[k],t[(k+1)%3])==edge)return true;return false;}
bool Contains(const Mesh& m,const Triangle& t,const ExactBary& p,Ops& op){for(unsigned k=0;k<3;++k)if(op.Side(m.points[t[k]],m.points[t[(k+1)%3]],p)<0)return false;return true;}
void Insert(Mesh& m,const ExactBary& p,Ops& op){for(const auto& old:m.points)if(op.Same(old,p))return;
  op.Bary(p);size_t containing=m.triangles.size();std::optional<Edge> edge;
  for(size_t i=0;i<m.triangles.size();++i){op.Work();const auto& t=m.triangles[i];if(!Contains(m,t,p,op))continue;containing=i;
    for(unsigned k=0;k<3;++k)if(!op.Side(m.points[t[k]],m.points[t[(k+1)%3]],p))edge=Sorted(t[k],t[(k+1)%3]);break;}
  if(containing==m.triangles.size())throw std::invalid_argument("Reference point has no containing original-chart triangle.");
  Reserve(m.points,Add(m.points.size(),1),op);op.Sync(Bytes(p));const auto point=m.points.size();m.points.push_back(p);
  Reserve(m.triangles,Add(m.triangles.size(),2),op);
  if(!edge){const auto t=m.triangles[containing];m.triangles[containing]={t[0],t[1],point};m.triangles.push_back({t[1],t[2],point});m.triangles.push_back({t[2],t[0],point});}
  else{const size_t old_size=m.triangles.size();size_t users=0;
    for(size_t i=0;i<old_size;++i){const auto t=m.triangles[i];for(unsigned k=0;k<3;++k)if(Sorted(t[k],t[(k+1)%3])==*edge){++users;
        m.triangles[i]={t[k],point,t[(k+2)%3]};m.triangles.push_back({point,t[(k+1)%3],t[(k+2)%3]});break;}}
    if(!users||users>2)throw std::invalid_argument("Reference point split found a nonmanifold edge.");}
}
bool Crosses(const Mesh& m,Edge a,Edge b,Ops& op){if(a==b)return false;for(auto x:a)for(auto y:b)if(x==y)return false;
  return op.Side(m.points[a[0]],m.points[a[1]],m.points[b[0]])*op.Side(m.points[a[0]],m.points[a[1]],m.points[b[1]])<0&&
         op.Side(m.points[b[0]],m.points[b[1]],m.points[a[0]])*op.Side(m.points[b[0]],m.points[b[1]],m.points[a[1]])<0;}
bool InteriorIntersection(const Mesh& m,const Triangle& t,Edge edge,Ops& op){auto first=op.Integer(0),last=op.Integer(1);
  for(unsigned k=0;k<3;++k){const auto& a=m.points[t[k]];const auto& b=m.points[t[(k+1)%3]];
    const auto u=op.Orient(a,b,m.points[edge[0]]),v=op.Orient(a,b,m.points[edge[1]]);const int su=op.Sign(u),sv=op.Sign(v);
    if((!su&&!sv)||(su<0&&sv<0))return false;
    if(su<0){auto crossing=op.Divide(u,op.Sub(u,v));if(op.Compare(crossing,first)>0)first=std::move(crossing);}
    if(sv<0){auto crossing=op.Divide(u,op.Sub(u,v));if(op.Compare(crossing,last)<0)last=std::move(crossing);}}
  return op.Compare(first,last)<0;
}
void Ears(const Mesh& m,std::vector<size_t> polygon,std::vector<Triangle>& out,Ops& op){
  while(polygon.size()>3){bool found=false;
    for(size_t i=0;i<polygon.size();++i){op.Work();const Triangle t{polygon[(i+polygon.size()-1)%polygon.size()],polygon[i],polygon[(i+1)%polygon.size()]};
      if(op.Side(m.points[t[0]],m.points[t[1]],m.points[t[2]])<=0)continue;bool blocked=false;
      for(size_t v:polygon)if(v!=t[0]&&v!=t[1]&&v!=t[2]&&Contains(m,t,m.points[v],op)){blocked=true;break;}
      if(blocked)continue;
      for(size_t k=0;k<polygon.size();++k)if(Crosses(m,Sorted(t[0],t[2]),Sorted(polygon[k],polygon[(k+1)%polygon.size()]),op)){blocked=true;break;}
      if(blocked)continue;out.push_back(t);polygon.erase(polygon.begin()+i);found=true;break;}
    if(!found)throw std::invalid_argument("Reference constraint corridor has no exact positive ear.");}
  if(polygon.size()!=3||op.Side(m.points[polygon[0]],m.points[polygon[1]],m.points[polygon[2]])<=0)throw std::invalid_argument("Reference corridor ends in a degenerate polygon.");
  out.push_back({polygon[0],polygon[1],polygon[2]});
}
void Recover(Mesh& m,Edge edge,Ops& op){op.Work(m.triangles.size());if(HasEdge(m,edge))return;
  using BoundaryMap=std::map<Edge,std::pair<unsigned,Edge>>;using NextMap=std::map<size_t,size_t>;
  const auto sides=Mul(3,m.triangles.size());
  const auto per_side=sizeof(BoundaryMap::value_type)+sizeof(NextMap::value_type)+sizeof(size_t)+12*sizeof(void*)+2*sizeof(size_t);
  Scratch scratch(op,Add(Add(m.triangles.size(),Mul(sides,per_side)),Mul(Add(Mul(2,m.triangles.size()),4),sizeof(Triangle))));
  std::vector<unsigned char> corridor(m.triangles.size());std::map<Edge,std::pair<unsigned,Edge>> boundary;size_t removed=0;
  for(size_t i=0;i<m.triangles.size();++i)if(InteriorIntersection(m,m.triangles[i],edge,op)){corridor[i]=1;++removed;const auto t=m.triangles[i];
    for(unsigned k=0;k<3;++k){auto& entry=boundary[Sorted(t[k],t[(k+1)%3])];if(++entry.first>2)throw std::invalid_argument("Nonmanifold reference corridor.");entry.second={t[k],t[(k+1)%3]};}}
  if(!removed)throw std::invalid_argument("Reference constraint has no recoverable corridor.");
  std::map<size_t,size_t> next;std::set<size_t> incoming;
  for(const auto& item:boundary)if(item.second.first==1){const auto e=item.second.second;
    if(!next.emplace(e[0],e[1]).second||!incoming.insert(e[1]).second)throw std::invalid_argument("Reference corridor boundary branches.");}
  if(!next.count(edge[0])||!next.count(edge[1]))throw std::invalid_argument("Reference corridor misses an endpoint.");
  std::vector<size_t> first{edge[0]},second{edge[1]};auto walk=[&](std::vector<size_t>& polygon,size_t end){
    while(polygon.back()!=end){op.Work();if(polygon.size()>next.size()||!next.count(polygon.back()))throw std::invalid_argument("Reference corridor boundary does not close.");polygon.push_back(next.at(polygon.back()));}};
  walk(first,edge[1]);walk(second,edge[0]);if(first.size()+second.size()!=next.size()+2)throw std::invalid_argument("Reference corridor has holes/disconnected components.");
  for(size_t i=0;i<m.triangles.size();++i)if(corridor[i])for(auto vertex:m.triangles[i]){
    op.Work();if(!next.count(vertex))throw std::invalid_argument("Reference corridor encloses a point constraint.");}
  if(next.size()<4||next.size()-2>removed)throw std::invalid_argument("Reference corridor is not a triangulated disk strip.");
  std::vector<Triangle> replacement;replacement.reserve(Add(removed,2));Ears(m,std::move(first),replacement,op);Ears(m,std::move(second),replacement,op);
  std::vector<Triangle> fresh;fresh.reserve(Add(m.triangles.size()-removed,replacement.size()));
  for(size_t i=0;i<m.triangles.size();++i)if(!corridor[i])fresh.push_back(m.triangles[i]);fresh.insert(fresh.end(),replacement.begin(),replacement.end());m.triangles=std::move(fresh);
  if(!HasEdge(m,edge))throw std::logic_error("Reference corridor did not recover its constraint.");
}
int Winding(const ExactBary& p,const std::vector<RegionSegment>& boundary,Id facet,Ops& op){int winding=0;
  for(const auto& edge:boundary)if(edge.facet==facet){op.Work();if(op.On(edge.first,edge.last,p))return -1;
    const int a=op.Compare(edge.first.v,p.v),b=op.Compare(edge.last.v,p.v),side=op.Side(edge.first,edge.last,p);
    if(a<=0&&b>0&&side>0){if(winding==INT_MAX)throw std::length_error("Reference winding overflow.");++winding;}
    if(a>0&&b<=0&&side<0){if(winding==INT_MIN)throw std::length_error("Reference winding overflow.");--winding;}}
  if(winding<0||winding>1)throw std::invalid_argument("Reference region has nonunit winding.");return winding;
}
}

bool RebuildReferenceTriangulation(const MappedFacetReference& geometry,const std::vector<CoveragePiece>& accepted,
    const std::map<Id,SurfaceBinding>& bindings,const std::vector<EdgeTrace>& traces,std::vector<CoveragePiece>& fresh,
    AtlasBudget& budget,std::string& reason) {
  std::vector<CoveragePiece> output;std::vector<RegionSegment> boundary,rebuilt;std::set<Key> mandatory;std::map<Id,Key> binding_keys;
  std::unique_ptr<Mesh> mesh;size_t trace_bytes=0;
  try {trace_bytes=Mul(traces.capacity(),sizeof(EdgeTrace));
    for(const auto& trace:traces){trace_bytes=Add(trace_bytes,Mul(trace.pieces.capacity(),sizeof(TracePiece)));for(const auto& p:trace.pieces)trace_bytes=Add(trace_bytes,Add(Add(Bytes(p.first),Bytes(p.last)),Bytes(p.end)));}
    Ops op(budget,[&]{size_t bytes=Add(Add(Bytes(accepted),Bytes(fresh)),Add(Bytes(output),Add(Bytes(boundary),Bytes(rebuilt))));bytes=Add(bytes,trace_bytes);
      bytes=Add(bytes,Mul(bindings.size(),sizeof(std::map<Id,SurfaceBinding>::value_type)+4*sizeof(void*)));
      for(const auto& key:mandatory)bytes=Add(bytes,Add(sizeof(Key)+4*sizeof(void*),Mul(key.capacity(),sizeof(uint64_t))));
      for(const auto& key:binding_keys)bytes=Add(bytes,Add(sizeof(decltype(binding_keys)::value_type)+4*sizeof(void*),Mul(key.second.capacity(),sizeof(uint64_t))));return Add(bytes,mesh?Bytes(*mesh):0);});
    if(accepted.empty())throw std::invalid_argument("Empty accepted reference region.");
    auto retain=[&](Key key){op.Sync(Add(sizeof(Key)+4*sizeof(void*),Mul(key.capacity(),sizeof(uint64_t))));mandatory.insert(std::move(key));};
    for(const auto& binding:bindings){auto key=op.BindingKey(geometry,binding.second);if(mandatory.count(key))throw std::invalid_argument("Current CFD points repeat a reference location.");
      op.Sync(Add(sizeof(Key)+sizeof(decltype(binding_keys)::value_type)+8*sizeof(void*),Mul(Mul(2,key.capacity()),sizeof(uint64_t))));
      mandatory.insert(key);binding_keys.emplace(binding.first,std::move(key));}
    size_t trace_count=0;
    for(const auto& trace:traces){if(trace.pieces.empty())throw std::invalid_argument("Empty retained reference trace.");auto begin=op.Integer(0);std::optional<Key> previous;
      for(const auto& p:trace.pieces){op.Bary(p.first);op.Bary(p.last);op.Sync();op.Check(ExactValidate(p.end,budget,op.reason));
        if(op.Compare(p.end,begin)<=0||op.Compare(p.end,op.Integer(1))>0)throw std::invalid_argument("Retained trace does not tile its active parameter.");
        auto a=op.ReferenceKey(geometry,p.facet,p.first),b=op.ReferenceKey(geometry,p.facet,p.last);
        if(a==b||(previous&&*previous!=a))throw std::invalid_argument("Retained reference trace is collapsed/discontinuous.");retain(std::move(a));retain(b);previous=std::move(b);begin=p.end;trace_count=Add(trace_count,1);}
      if(op.Compare(begin,op.Integer(1)))throw std::invalid_argument("Retained trace does not reach active parameter one.");}
    op.Sync();op.Check(geometry.RegionBoundary(accepted,mandatory,boundary,budget,op.reason));
    Scratch flags(op,Add(Mul(trace_count,sizeof(unsigned char)),Mul(bindings.size(),sizeof(Id)+4*sizeof(void*))));std::vector<unsigned char> covered_trace(trace_count);std::set<Id> covered_binding;
    Scratch facet_memory(op,Mul(accepted.size(),sizeof(Id)+4*sizeof(void*)));std::set<Id> facets;for(const auto& p:accepted)facets.insert(p.facet);
    for(Id facet:facets){op.Sync(sizeof(Mesh));mesh=std::make_unique<Mesh>();
      mesh->points={{op.Integer(0),op.Integer(0)},{op.Integer(1),op.Integer(0)},{op.Integer(0),op.Integer(1)}};mesh->triangles.push_back({0,1,2});
      auto seed=[&](const ExactBary& p){Reserve(mesh->seeds,Add(mesh->seeds.size(),1),op);op.Sync(Bytes(p));mesh->seeds.push_back(p);};
      auto segment=[&](ExactBary a,ExactBary b,size_t trace){if(op.Same(a,b))throw std::invalid_argument("Collapsed reference PSLG segment.");seed(a);seed(b);Reserve(mesh->segments,Add(mesh->segments.size(),1),op);
        op.Sync(Add(Bytes(a),Bytes(b)));mesh->segments.push_back({std::move(a),std::move(b),trace});};
      for(const auto& edge:boundary)if(edge.facet==facet)segment(edge.first,edge.last,NoTrace);
      for(const auto& binding:binding_keys){ExactBary point;if(op.Parameters(geometry,facet,binding.second,point))seed(point);}
      size_t trace=0;
      for(const auto& path:traces)for(const auto& p:path.pieces){auto first=op.ReferenceKey(geometry,p.facet,p.first),last=op.ReferenceKey(geometry,p.facet,p.last);ExactBary a,b;
        if(op.Parameters(geometry,facet,first,a)&&op.Parameters(geometry,facet,last,b))segment(std::move(a),std::move(b),trace);++trace;}
      std::sort(mesh->seeds.begin(),mesh->seeds.end(),[&](const ExactBary& a,const ExactBary& b){const int order=op.Compare(a.u,b.u);return order?order<0:op.Compare(a.v,b.v)<0;});
      mesh->seeds.erase(std::unique(mesh->seeds.begin(),mesh->seeds.end(),[&](const ExactBary& a,const ExactBary& b){return op.Same(a,b);}),mesh->seeds.end());
      for(const auto& p:mesh->seeds)Insert(*mesh,p,op);
      for(const auto& s:mesh->segments){Scratch cuts(op,Mul(mesh->points.size(),sizeof(size_t)));std::vector<size_t> points;points.reserve(mesh->points.size());
        for(size_t i=0;i<mesh->points.size();++i)if(op.On(s.first,s.last,mesh->points[i]))points.push_back(i);
        const bool use_u=!op.Same(s.first.u,s.last.u);const bool forward=op.Compare(use_u?s.first.u:s.first.v,use_u?s.last.u:s.last.v)<0;
        std::sort(points.begin(),points.end(),[&](size_t a,size_t b){const auto& x=use_u?mesh->points[a].u:mesh->points[a].v;const auto& y=use_u?mesh->points[b].u:mesh->points[b].v;return forward?op.Compare(x,y)<0:op.Compare(x,y)>0;});
        if(points.size()<2)throw std::logic_error("Reference PSLG segment has no endpoints.");
        for(size_t i=1;i<points.size();++i){const auto edge=Sorted(points[i-1],points[i]);if(std::find(mesh->constraints.begin(),mesh->constraints.end(),edge)!=mesh->constraints.end())continue;
          for(const auto& previous:mesh->constraints){op.Work();if(Crosses(*mesh,edge,previous,op))throw std::invalid_argument("Reference PSLG constraints cross.");}
          Reserve(mesh->constraints,Add(mesh->constraints.size(),1),op);mesh->constraints.push_back(edge);}}
      for(const auto& edge:mesh->constraints)Recover(*mesh,edge,op);
      Scratch selected_memory(op,mesh->triangles.size());std::vector<unsigned char> selected(mesh->triangles.size());
      for(size_t i=0;i<mesh->triangles.size();++i){const auto& t=mesh->triangles[i];if(op.Side(mesh->points[t[0]],mesh->points[t[1]],mesh->points[t[2]])<=0)throw std::logic_error("Reference reconstruction produced an invalid triangle.");
        const auto& a=mesh->points[t[0]];const auto& b=mesh->points[t[1]];const auto& c=mesh->points[t[2]];
        ExactBary center{op.Divide(op.AddR(op.AddR(a.u,b.u),c.u),op.Integer(3)),op.Divide(op.AddR(op.AddR(a.v,b.v),c.v),op.Integer(3))};
        const int winding=Winding(center,boundary,facet,op);if(winding<0)throw std::logic_error("Recovered reference triangle straddles its boundary.");if(!winding)continue;selected[i]=1;
        CoveragePiece piece;piece.facet=facet;piece.reference={a,b,c};piece.active={ExactBary{op.Integer(0),op.Integer(0)},ExactBary{op.Integer(1),op.Integer(0)},ExactBary{op.Integer(0),op.Integer(1)}};
        Reserve(output,Add(output.size(),1),op);size_t bytes=0;for(unsigned k=0;k<3;++k)bytes=Add(bytes,Add(Bytes(piece.reference[k]),Bytes(piece.active[k])));op.Sync(bytes);output.push_back(std::move(piece));}
      auto selected_edge=[&](size_t a,size_t b){for(size_t i=0;i<mesh->triangles.size();++i)if(selected[i]){const auto& t=mesh->triangles[i];for(unsigned k=0;k<3;++k)if(Sorted(t[k],t[(k+1)%3])==Sorted(a,b))return true;}return false;};
      for(const auto& s:mesh->segments)if(s.trace!=NoTrace){bool used=true;size_t count=0;
        for(const auto& edge:mesh->constraints)if(op.On(s.first,s.last,mesh->points[edge[0]])&&op.On(s.first,s.last,mesh->points[edge[1]])){++count;used=used&&selected_edge(edge[0],edge[1]);}
        if(used&&count)covered_trace.at(s.trace)=1;}
      for(const auto& binding:binding_keys){ExactBary p;if(!op.Parameters(geometry,facet,binding.second,p))continue;const auto vertex=Find(*mesh,p,op);
        for(size_t i=0;i<mesh->triangles.size();++i)if(selected[i]&&std::find(mesh->triangles[i].begin(),mesh->triangles[i].end(),vertex)!=mesh->triangles[i].end()){
          covered_binding.insert(binding.first);break;}}
      for(const auto& edge:mesh->constraints)if(!HasEdge(*mesh,edge))throw std::logic_error("Reference recovery retired an earlier constraint.");
      mesh.reset();}
    if(output.empty()||covered_binding.size()!=bindings.size()||std::find(covered_trace.begin(),covered_trace.end(),0)!=covered_trace.end())throw std::invalid_argument("Reference reconstruction omits a current point or retained trace.");
    // New mandatory knots may split an unchanged region boundary; compare its geometric chains.
    op.Sync();op.Check(geometry.RegionBoundary(accepted,{},boundary,budget,op.reason));
    op.Sync();op.Check(geometry.RegionBoundary(output,{},rebuilt,budget,op.reason));
    auto less=[&](const RegionSegment& a,const RegionSegment& b){if(a.facet!=b.facet)return a.facet<b.facet;
      for(const auto pair:{std::make_pair(&a.first.u,&b.first.u),std::make_pair(&a.first.v,&b.first.v),std::make_pair(&a.last.u,&b.last.u),std::make_pair(&a.last.v,&b.last.v)}){
        const int order=op.Compare(*pair.first,*pair.second);if(order)return order<0;}return false;};
    std::sort(boundary.begin(),boundary.end(),less);std::sort(rebuilt.begin(),rebuilt.end(),less);
    if(boundary.size()!=rebuilt.size())throw std::logic_error("Fresh reference triangulation changed its region boundary.");
    for(size_t i=0;i<boundary.size();++i)if(boundary[i].facet!=rebuilt[i].facet||!op.Same(boundary[i].first,rebuilt[i].first)||!op.Same(boundary[i].last,rebuilt[i].last))
      throw std::logic_error("Fresh reference triangulation changed its oriented reference 2-chain.");
    fresh=std::move(output);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
}  // namespace SU2NativeGeometry
