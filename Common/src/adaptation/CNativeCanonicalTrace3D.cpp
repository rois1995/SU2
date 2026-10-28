/*!
 * \file CNativeCanonicalTrace3D.cpp
 * \brief Fixed original-star/strip charts and exact canonical surface traces.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeCanonicalTrace3D.hpp"
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include <algorithm>
#include <climits>
#include <functional>
#include <numeric>
#include <stdexcept>

namespace SU2NativeGeometry {
namespace {
using Edge=SU2Native3D::Edge;
using Key=std::vector<uint64_t>;
using Triangle=std::array<size_t,3>;
size_t Add(size_t a,size_t b){if(b>SIZE_MAX-a)throw std::length_error("Canonical trace storage overflow.");return a+b;}
size_t Mul(size_t a,size_t b){if(a&&b>SIZE_MAX/a)throw std::length_error("Canonical trace storage overflow.");return a*b;}
size_t Bytes(const ExactRatio& x){return Mul(Add(x.numerator.capacity(),x.denominator.capacity()),sizeof(uint32_t));}
size_t Bytes(const ExactBary& p){return Add(Bytes(p.u),Bytes(p.v));}
size_t Bytes(const EdgeTrace& t){size_t n=Mul(t.pieces.capacity(),sizeof(TracePiece));for(const auto& p:t.pieces)n=Add(n,Add(Add(Bytes(p.first),Bytes(p.last)),Bytes(p.end)));return n;}
template<class M>size_t MapBytes(const M& m){return Mul(m.size(),sizeof(typename M::value_type)+4*sizeof(void*));}
Edge Sorted(Id a,Id b){return a<b?Edge{a,b}:Edge{b,a};}
struct Portal {size_t first,last;Edge edge;};
struct Interval {size_t triangle;ExactRatio first,last;};
struct HalfEdge {unsigned count=0;size_t first=0,last=0;};
struct Work {
  std::vector<Id> first_support,last_support,first_centers,last_centers,centers,candidate,faces,queue,original;
  std::vector<size_t> parent,index,next,previous,cycle,neighbors;
  std::vector<unsigned> flags;
  std::vector<Portal> portals;
  std::vector<Triangle> triangles;
  std::vector<ExactBary> domain;
  std::map<Id,Id> predecessor;
  std::map<std::array<size_t,2>,HalfEdge> edges;
  std::vector<Interval> intervals;
  std::vector<ExactRatio> cuts;
  Key first_key,last_key;
  EdgeTrace trace,reversed;
  Id body=0,center=0;
  bool star=false;
};
size_t Bytes(const Work& w) {
  size_t n=sizeof(w);
  for(const auto* v:{&w.first_support,&w.last_support,&w.first_centers,&w.last_centers,&w.centers,&w.candidate,&w.faces,&w.queue,&w.original})n=Add(n,Mul(v->capacity(),sizeof(Id)));
  for(const auto* v:{&w.parent,&w.index,&w.next,&w.previous,&w.cycle,&w.neighbors})n=Add(n,Mul(v->capacity(),sizeof(size_t)));
  n=Add(n,Mul(w.flags.capacity(),sizeof(unsigned)));n=Add(n,Mul(w.portals.capacity(),sizeof(Portal)));n=Add(n,Mul(w.triangles.capacity(),sizeof(Triangle)));
  n=Add(n,Mul(w.domain.capacity(),sizeof(ExactBary)));for(const auto& p:w.domain)n=Add(n,Bytes(p));
  n=Add(n,Add(MapBytes(w.predecessor),MapBytes(w.edges)));n=Add(n,Mul(w.intervals.capacity(),sizeof(Interval)));
  for(const auto& p:w.intervals)n=Add(n,Add(Bytes(p.first),Bytes(p.last)));
  n=Add(n,Mul(w.cuts.capacity(),sizeof(ExactRatio)));for(const auto& p:w.cuts)n=Add(n,Bytes(p));
  n=Add(n,Mul(Add(w.first_key.capacity(),w.last_key.capacity()),sizeof(uint64_t)));
  return Add(n,Add(Bytes(w.trace),Bytes(w.reversed)));
}
struct Ops {
  AtlasBudget& budget;size_t caller,scratch;std::function<size_t()> resident;std::string reason;
  Ops(AtlasBudget& b,std::function<size_t()> r):budget(b),caller(b.caller_bytes),scratch(CurvedAtlasWorkspaceBytes(b.maximum_integer_bits)),resident(std::move(r)){Sync();}
  ~Ops(){budget.caller_bytes=caller;}
  void Sync(size_t more=0){const auto n=Add(caller,Add(resident(),Add(scratch,more)));budget.peak_bytes=std::max(budget.peak_bytes,n);if(n>budget.maximum_bytes)throw std::length_error("Canonical trace storage exhausted.");budget.caller_bytes=n;}
  void Work(){if(!budget.ChargeWork())throw std::length_error("Canonical trace work exhausted.");}
  void Check(bool ok){if(!ok)throw std::invalid_argument(reason);}
  ExactRatio Integer(int64_t x){Sync();ExactRatio r;Check(ExactInteger(x,r,budget,reason));return r;}
  ExactRatio Sum(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactAdd(a,b,r,budget,reason));return r;}
  ExactRatio Sub(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactSubtract(a,b,r,budget,reason));return r;}
  ExactRatio Product(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactMultiply(a,b,r,budget,reason));return r;}
  ExactRatio Divide(const ExactRatio& a,const ExactRatio& b){Sync();ExactRatio r;Check(ExactDivide(a,b,r,budget,reason));return r;}
  int Compare(const ExactRatio& a,const ExactRatio& b){Sync();int result;Check(ExactCompare(a,b,result,budget,reason));return result;}
  ExactRatio Weight(const ExactBary& p,unsigned k){return k==1?p.u:k==2?p.v:Sub(Sub(Integer(1),p.u),p.v);}
  ExactRatio Orient(const ExactBary& a,const ExactBary& b,const ExactBary& c){return Sub(Product(Sub(b.u,a.u),Sub(c.v,a.v)),Product(Sub(b.v,a.v),Sub(c.u,a.u)));}
  ExactBary Lerp(const ExactBary& a,const ExactBary& b,const ExactRatio& t){return {Sum(a.u,Product(t,Sub(b.u,a.u))),Sum(a.v,Product(t,Sub(b.v,a.v)))};}
  Key KeyAt(const MappedFacetReference& g,Id facet,const ExactBary& point){Sync();Key key;Check(g.ReferencePointKey(facet,point,key,budget,reason));return key;}
};
template<class T>void Reserve(std::vector<T>& v,size_t n,Ops& op){if(n<=v.capacity())return;const auto capacity=std::max(n,Add(v.capacity(),std::max(size_t(1),v.capacity())));op.Sync(Mul(capacity,sizeof(T)));v.reserve(capacity);}
template<class T>void Push(std::vector<T>& v,const T& x,Ops& op){Reserve(v,Add(v.size(),1),op);v.push_back(x);}
void Push(std::vector<ExactRatio>& v,const ExactRatio& x,Ops& op){Reserve(v,Add(v.size(),1),op);op.Sync(Bytes(x));v.push_back(x);}
size_t Root(std::vector<size_t>& parent,size_t i){while(parent[i]!=i){parent[i]=parent[parent[i]];i=parent[i];}return i;}
void Unite(std::vector<size_t>& parent,size_t a,size_t b){a=Root(parent,a);b=Root(parent,b);parent[std::max(a,b)]=std::min(a,b);}
bool Allowed(const MappedFacetReference& g,const MobilityPolicy& policy,Id sheet,Id body,Id facet) {
  const auto s=policy.sheets.find(facet),b=policy.bodies.find(facet);
  if(s==policy.sheets.end()||b==policy.bodies.end()||b->second!=g.Body(facet))throw std::invalid_argument("Incomplete canonical trace body/sheet policy.");
  return s->second==sheet&&b->second==body;
}
void Support(const MappedFacetReference& g,const MobilityPolicy& policy,Id sheet,const SurfaceBinding& binding,
    std::vector<Id>& facets,Ops& op) {
  g.ForFacets(binding,[&](Id facet){op.Work();const auto s=policy.sheets.find(facet),b=policy.bodies.find(facet);
    if(s==policy.sheets.end()||b==policy.bodies.end()||b->second!=g.Body(facet))throw std::invalid_argument("Incomplete canonical trace association policy.");
    if(s->second==sheet)Push(facets,facet,op);});
  std::sort(facets.begin(),facets.end());facets.erase(std::unique(facets.begin(),facets.end()),facets.end());
  if(facets.empty())throw std::invalid_argument("Surface association has no support on the requested sheet.");
}
void NeighborFacets(const MappedFacetReference& g,const MobilityPolicy& policy,Id sheet,Id body,Id facet,
    std::array<Id,6>& neighbors,size_t& count,Ops& op) {
  count=0;const auto& f=g.Reference().Get(facet);
  for(unsigned k=0;k<3;++k){op.Work();const auto e=Sorted(f.v[k].id,f.v[(k+1)%3].id);if(policy.features.count(e))continue;
    g.ForFacets(g.EdgePoint(e,.5),[&](Id neighbor){if(neighbor==facet||!Allowed(g,policy,sheet,body,neighbor))return;
      if(count==neighbors.size())throw std::invalid_argument("Nonmanifold canonical trace adjacency.");neighbors[count++]=neighbor;});}
  std::sort(neighbors.begin(),neighbors.begin()+count);count=size_t(std::unique(neighbors.begin(),neighbors.begin()+count)-neighbors.begin());
}
void SelectChart(const MappedFacetReference& g,const MobilityPolicy& policy,Id sheet,Work& w,Ops& op) {
  for(Id facet:w.first_support)for(auto n:g.Reference().Get(facet).v)Push(w.first_centers,n.id,op);
  for(Id facet:w.last_support)for(auto n:g.Reference().Get(facet).v)Push(w.last_centers,n.id,op);
  for(auto* centers:{&w.first_centers,&w.last_centers}){std::sort(centers->begin(),centers->end());centers->erase(std::unique(centers->begin(),centers->end()),centers->end());}
  for(Id center:w.first_centers)if(std::binary_search(w.last_centers.begin(),w.last_centers.end(),center))Push(w.centers,center,op);
  for(Id center:w.centers) {
    w.candidate.clear();g.ForFacets(g.Vertex(center),[&](Id facet){op.Work();if(Allowed(g,policy,sheet,w.body,facet))Push(w.candidate,facet,op);});
    std::sort(w.candidate.begin(),w.candidate.end());const size_t n=w.candidate.size();Reserve(w.parent,n,op);w.parent.resize(n);std::iota(w.parent.begin(),w.parent.end(),0);
    Reserve(w.flags,n,op);w.flags.assign(n,0);
    for(size_t i=0;i<n;++i){std::array<Id,6> neighbors;size_t count;NeighborFacets(g,policy,sheet,w.body,w.candidate[i],neighbors,count,op);
      for(size_t j=0;j<count;++j){const auto at=std::lower_bound(w.candidate.begin(),w.candidate.end(),neighbors[j]);if(at!=w.candidate.end()&&*at==neighbors[j])Unite(w.parent,i,size_t(at-w.candidate.begin()));}}
    for(size_t i=0;i<n;++i){unsigned flag=std::binary_search(w.first_support.begin(),w.first_support.end(),w.candidate[i])?1u:0u;
      if(std::binary_search(w.last_support.begin(),w.last_support.end(),w.candidate[i]))flag|=2;w.flags[Root(w.parent,i)]|=flag;}
    for(size_t i=0;i<n;++i)if(Root(w.parent,i)==i&&w.flags[i]==3) {
      w.center=center;w.star=true;for(size_t j=0;j<n;++j)if(Root(w.parent,j)==i)Push(w.faces,w.candidate[j],op);return;
    }
  }
  // One multi-source BFS determines the shortest strip and all ties, before any chart is constructed.
  for(Id facet:w.first_support){op.Sync(sizeof(decltype(w.predecessor)::value_type)+4*sizeof(void*));w.predecessor.emplace(facet,facet);Push(w.queue,facet,op);}
  Id target=UINT64_MAX;
  for(size_t head=0;head<w.queue.size();++head) {
    op.Work();const Id facet=w.queue[head];if(std::binary_search(w.last_support.begin(),w.last_support.end(),facet)){target=facet;break;}
    std::array<Id,6> neighbors;size_t count;NeighborFacets(g,policy,sheet,w.body,facet,neighbors,count,op);
    for(size_t i=0;i<count;++i)if(!w.predecessor.count(neighbors[i])){op.Sync(sizeof(decltype(w.predecessor)::value_type)+4*sizeof(void*));w.predecessor.emplace(neighbors[i],facet);Push(w.queue,neighbors[i],op);}
  }
  if(target==UINT64_MAX)throw std::invalid_argument("No unprotected original-facet strip joins the surface associations.");
  for(Id at=target;;at=w.predecessor.at(at)){Push(w.faces,at,op);if(w.predecessor.at(at)==at)break;}
  std::reverse(w.faces.begin(),w.faces.end());
}
Edge SharedEdge(const MappedFacetReference& g,Id first,Id last) {
  Edge edge{};size_t count=0;for(auto a:g.Reference().Get(first).v)for(auto b:g.Reference().Get(last).v)if(a.id==b.id){if(count==2)throw std::invalid_argument("Repeated original facet in canonical strip.");edge[count++]=a.id;}
  if(count!=2)throw std::invalid_argument("Canonical strip has no shared original portal.");return Sorted(edge[0],edge[1]);
}
void BuildChart(const MappedFacetReference& g,const MobilityPolicy& policy,Work& w,Ops& op) {
  if(w.star) {
    for(size_t i=0;i<w.faces.size();++i){const auto& f=g.Reference().Get(w.faces[i]);for(unsigned k=0;k<3;++k){const auto edge=Sorted(f.v[k].id,f.v[(k+1)%3].id);if(policy.features.count(edge))continue;
      g.ForFacets(g.EdgePoint(edge,.5),[&](Id other){const auto at=std::lower_bound(w.faces.begin(),w.faces.end(),other);
        if(at!=w.faces.end()&&*at==other){const size_t j=size_t(at-w.faces.begin());if(i<j)Push(w.portals,Portal{i,j,edge},op);}});}}
  } else for(size_t i=1;i<w.faces.size();++i){const auto edge=SharedEdge(g,w.faces[i-1],w.faces[i]);if(policy.features.count(edge))throw std::invalid_argument("Canonical strip crosses a protected edge.");Push(w.portals,Portal{i-1,i,edge},op);}
  const size_t corners=Mul(w.faces.size(),3);Reserve(w.parent,corners,op);w.parent.resize(corners);std::iota(w.parent.begin(),w.parent.end(),0);
  for(const auto& portal:w.portals)for(Id endpoint:portal.edge) {
    size_t a=SIZE_MAX,b=SIZE_MAX;for(unsigned k=0;k<3;++k){if(g.Reference().Get(w.faces[portal.first]).v[k].id==endpoint)a=3*portal.first+k;if(g.Reference().Get(w.faces[portal.last]).v[k].id==endpoint)b=3*portal.last+k;}
    if(a==SIZE_MAX||b==SIZE_MAX)throw std::logic_error("Canonical portal lost an original corner.");Unite(w.parent,a,b);
  }
  Reserve(w.index,corners,op);w.index.assign(corners,SIZE_MAX);
  for(size_t i=0;i<corners;++i){op.Work();const auto root=Root(w.parent,i);if(w.index[root]!=SIZE_MAX)continue;
    w.index[root]=w.domain.size();Reserve(w.domain,Add(w.domain.size(),1),op);op.Sync(2*sizeof(uint32_t));w.domain.push_back({});
    Push(w.original,g.Reference().Get(w.faces[i/3]).v[i%3].id,op);}
  for(size_t i=0;i<w.faces.size();++i){Triangle triangle;for(unsigned k=0;k<3;++k)triangle[k]=w.index[Root(w.parent,3*i+k)];Push(w.triangles,triangle,op);}
  for(const auto& triangle:w.triangles)for(unsigned k=0;k<3;++k) {
    const size_t a=triangle[k],b=triangle[(k+1)%3];if(a==b)throw std::invalid_argument("Canonical chart has a collapsed original edge.");
    const std::array<size_t,2> key{std::min(a,b),std::max(a,b)};op.Sync(sizeof(decltype(w.edges)::value_type)+4*sizeof(void*));auto& edge=w.edges[key];
    if(!edge.count){edge.first=a;edge.last=b;}else if(edge.first!=b||edge.last!=a)throw std::invalid_argument("Canonical chart has inconsistent original orientation.");
    if(++edge.count>2)throw std::invalid_argument("Canonical chart has nonmanifold original incidence.");
  }
  const size_t vertices=w.domain.size();Reserve(w.next,vertices,op);Reserve(w.previous,vertices,op);w.next.assign(vertices,SIZE_MAX);w.previous.assign(vertices,SIZE_MAX);
  size_t boundary=0;
  for(const auto& row:w.edges)if(row.second.count==1){const auto& e=row.second;
    if(w.next[e.first]!=SIZE_MAX||w.previous[e.last]!=SIZE_MAX)throw std::invalid_argument("Canonical chart boundary branches.");w.next[e.first]=e.last;w.previous[e.last]=e.first;++boundary;}
  if(boundary<3||vertices+w.faces.size()!=w.edges.size()+1)throw std::invalid_argument("Canonical chart is not an original reference disk.");
  size_t first=SIZE_MAX,interior=SIZE_MAX;
  for(size_t i=0;i<vertices;++i){if((w.next[i]==SIZE_MAX)!=(w.previous[i]==SIZE_MAX))throw std::invalid_argument("Canonical chart boundary is open.");
    if(w.next[i]!=SIZE_MAX){if(first==SIZE_MAX||std::make_pair(w.original[i],i)<std::make_pair(w.original[first],first))first=i;}
    else{if(interior!=SIZE_MAX||!w.star||w.original[i]!=w.center)throw std::invalid_argument("Canonical chart has an unsupported original interior vertex.");interior=i;}}
  for(size_t at=first;;at=w.next[at]){op.Work();if(at==SIZE_MAX||w.cycle.size()>=boundary)throw std::invalid_argument("Canonical chart boundary is not one simple cycle.");Push(w.cycle,at,op);if(w.next[at]==first)break;}
  if(w.cycle.size()!=boundary)throw std::invalid_argument("Canonical chart contains multiple boundary loops.");
  for(size_t i=0;i<w.cycle.size();++i) {
    if(i>size_t(INT64_MAX)||(i&&i>size_t(INT64_MAX)/i))throw std::length_error("Canonical chart integer boundary coordinates overflow.");
    w.domain[w.cycle[i]]={op.Integer(int64_t(i)),op.Integer(int64_t(i*i))};
  }
  if(interior!=SIZE_MAX) {
    for(const auto& t:w.triangles)if(std::find(t.begin(),t.end(),interior)!=t.end())for(size_t v:t)if(v!=interior)Push(w.neighbors,v,op);
    std::sort(w.neighbors.begin(),w.neighbors.end());w.neighbors.erase(std::unique(w.neighbors.begin(),w.neighbors.end()),w.neighbors.end());
    if(w.neighbors.empty()||w.neighbors.size()>size_t(INT64_MAX))throw std::invalid_argument("Canonical center has no bounded original neighbor set.");
    ExactBary center{op.Integer(0),op.Integer(0)};for(size_t v:w.neighbors){center.u=op.Sum(center.u,w.domain[v].u);center.v=op.Sum(center.v,w.domain[v].v);}
    const auto n=op.Integer(int64_t(w.neighbors.size()));w.domain[interior]={op.Divide(center.u,n),op.Divide(center.v,n)};
  }
  for(const auto& t:w.triangles)if(op.Compare(op.Orient(w.domain[t[0]],w.domain[t[1]],w.domain[t[2]]),op.Integer(0))<=0)
    throw std::invalid_argument("Canonical original chart has a nonpositive exact triangle.");
}
ExactBary Interpolate(const std::array<ExactBary,3>& vertices,const ExactBary& bary,Ops& op) {
  ExactBary out{op.Integer(0),op.Integer(0)};for(unsigned k=0;k<3;++k){const auto weight=op.Weight(bary,k);out.u=op.Sum(out.u,op.Product(weight,vertices[k].u));out.v=op.Sum(out.v,op.Product(weight,vertices[k].v));}return out;
}
ExactBary BindingDomain(const MappedFacetReference& g,const SurfaceBinding& binding,const std::vector<Id>& support,const Work& w,Ops& op) {
  size_t chosen=SIZE_MAX;for(size_t i=0;i<w.faces.size();++i)if(std::binary_search(support.begin(),support.end(),w.faces[i])&&(chosen==SIZE_MAX||w.faces[i]<w.faces[chosen]))chosen=i;
  if(chosen==SIZE_MAX)throw std::invalid_argument("Canonical chart lost an endpoint support.");ExactBary parameter;op.Sync();op.Check(g.BindingParameters(w.faces[chosen],binding,parameter,op.budget,op.reason));
  const auto& t=w.triangles[chosen];return Interpolate({w.domain[t[0]],w.domain[t[1]],w.domain[t[2]]},parameter,op);
}
ExactBary Parameters(const Work& w,size_t i,const ExactBary& point,Ops& op) {
  const auto& t=w.triangles[i];const auto& a=w.domain[t[0]];const auto& b=w.domain[t[1]];const auto& c=w.domain[t[2]];
  const auto determinant=op.Orient(a,b,c);return {op.Divide(op.Orient(c,a,point),determinant),op.Divide(op.Orient(a,b,point),determinant)};
}
void TraceSegment(const MappedFacetReference& g,const ExactBary& first,const ExactBary& last,Work& w,Ops& op) {
  for(size_t i=0;i<w.triangles.size();++i) {
    op.Work();auto low=op.Integer(0),high=op.Integer(1);const auto& t=w.triangles[i];bool empty=false;
    for(unsigned k=0;k<3;++k) {
      const auto a=op.Orient(w.domain[t[k]],w.domain[t[(k+1)%3]],first),b=op.Orient(w.domain[t[k]],w.domain[t[(k+1)%3]],last);
      const int sa=op.Compare(a,op.Integer(0)),sb=op.Compare(b,op.Integer(0));
      if(sa<0&&sb<0){empty=true;break;}if(sa>=0&&sb>=0)continue;
      const auto crossing=op.Divide(a,op.Sub(a,b));
      if(sa<0){if(op.Compare(crossing,low)>0)low=crossing;}else if(op.Compare(crossing,high)<0)high=crossing;
    }
    if(empty||op.Compare(low,high)>=0)continue;
    Reserve(w.intervals,Add(w.intervals.size(),1),op);op.Sync(Add(Bytes(low),Bytes(high)));w.intervals.push_back({i,low,high});
  }
  Push(w.cuts,op.Integer(0),op);Push(w.cuts,op.Integer(1),op);
  for(const auto& interval:w.intervals){Reserve(w.cuts,Add(w.cuts.size(),2),op);op.Sync(Add(Bytes(interval.first),Bytes(interval.last)));w.cuts.push_back(interval.first);w.cuts.push_back(interval.last);}
  std::sort(w.cuts.begin(),w.cuts.end(),[&](const auto& a,const auto& b){return op.Compare(a,b)<0;});
  w.cuts.erase(std::unique(w.cuts.begin(),w.cuts.end(),[&](const auto& a,const auto& b){return !op.Compare(a,b);}),w.cuts.end());
  Key previous;
  for(size_t cut=1;cut<w.cuts.size();++cut) {
    op.Work();size_t chosen=SIZE_MAX;ExactBary start,finish;Key start_key,last_key;
    const auto a=op.Lerp(first,last,w.cuts[cut-1]),b=op.Lerp(first,last,w.cuts[cut]);
    for(const auto& interval:w.intervals)if(op.Compare(interval.first,w.cuts[cut-1])<=0&&op.Compare(interval.last,w.cuts[cut])>=0) {
      const auto r0=Parameters(w,interval.triangle,a,op),r1=Parameters(w,interval.triangle,b,op);const Id facet=w.faces[interval.triangle];
      const auto key0=op.KeyAt(g,facet,r0),key1=op.KeyAt(g,facet,r1);
      if(chosen!=SIZE_MAX&&(key0!=start_key||key1!=last_key))throw std::invalid_argument("Canonical segment has inconsistent shared original-chart crossings.");
      if(chosen==SIZE_MAX||facet<w.faces[chosen]){chosen=interval.triangle;start=r0;finish=r1;start_key=key0;last_key=key1;}
    }
    if(chosen==SIZE_MAX)throw std::invalid_argument("Canonical original-chart segment has a coverage gap.");
    if(cut==1&&start_key!=w.first_key)throw std::invalid_argument("Canonical trace changed its first association.");
    if(!previous.empty()&&previous!=start_key)throw std::invalid_argument("Canonical trace is discontinuous across original charts.");
    previous=last_key;const Id facet=w.faces[chosen];
    if(!w.trace.pieces.empty()&&w.trace.pieces.back().facet==facet) {
      auto& piece=w.trace.pieces.back();op.Sync(Add(Bytes(finish),Bytes(w.cuts[cut])));piece.last=finish;piece.end=w.cuts[cut];
    } else {
      Reserve(w.trace.pieces,Add(w.trace.pieces.size(),1),op);op.Sync(Add(Add(Bytes(start),Bytes(finish)),Bytes(w.cuts[cut])));
      w.trace.pieces.push_back({facet,start,finish,w.cuts[cut]});
    }
  }
  if(w.trace.pieces.empty()||previous!=w.last_key||op.Compare(w.trace.pieces.back().end,op.Integer(1)))throw std::invalid_argument("Canonical trace has incomplete endpoint coverage.");
  if(w.trace.pieces.size()>w.faces.size())throw std::invalid_argument("Canonical trace exceeds its original-chart piece bound.");
}
}
bool CanonicalSurfaceTrace(const MappedFacetReference& geometry,const MobilityPolicy& policy,Id sheet,
    const SurfaceBinding& first,const SurfaceBinding& last,EdgeTrace& result,AtlasBudget& budget,std::string& reason) {
  Work work;
  try {
    Ops op(budget,[&]{return Add(Bytes(work),Bytes(result));});
    if(!policy.identity||policy.version!=1||sheet==UINT64_MAX)throw std::invalid_argument("Canonical surface trace requires an immutable body/sheet policy.");
    SurfaceBinding a=first,b=last;Support(geometry,policy,sheet,a,work.first_support,op);Support(geometry,policy,sheet,b,work.last_support,op);
    work.body=geometry.Body(work.first_support.front());
    for(const auto* support:{&work.first_support,&work.last_support})for(Id facet:*support)if(!Allowed(geometry,policy,sheet,work.body,facet))throw std::invalid_argument("Canonical trace associations belong to different bodies/sheets.");
    ExactBary parameter;op.Sync();op.Check(geometry.BindingParameters(work.first_support.front(),a,parameter,budget,op.reason));work.first_key=op.KeyAt(geometry,work.first_support.front(),parameter);
    op.Sync();op.Check(geometry.BindingParameters(work.last_support.front(),b,parameter,budget,op.reason));work.last_key=op.KeyAt(geometry,work.last_support.front(),parameter);
    if(work.first_key==work.last_key)throw std::invalid_argument("Canonical surface trace needs distinct original associations.");
    const bool reverse=work.last_key<work.first_key;
    if(reverse){std::swap(a,b);std::swap(work.first_support,work.last_support);std::swap(work.first_key,work.last_key);}
    SelectChart(geometry,policy,sheet,work,op);BuildChart(geometry,policy,work,op);
    const auto start=BindingDomain(geometry,a,work.first_support,work,op),finish=BindingDomain(geometry,b,work.last_support,work,op);
    if(!op.Compare(start.u,finish.u)&&!op.Compare(start.v,finish.v))throw std::invalid_argument("Canonical chart identifies distinct endpoint associations.");
    TraceSegment(geometry,start,finish,work,op);
    if(reverse) {
      for(size_t i=work.trace.pieces.size();i;--i){const auto& p=work.trace.pieces[i-1];const auto end=op.Sub(op.Integer(1),i>1?work.trace.pieces[i-2].end:op.Integer(0));
        Reserve(work.reversed.pieces,Add(work.reversed.pieces.size(),1),op);op.Sync(Add(Add(Bytes(p.first),Bytes(p.last)),Bytes(end)));
        work.reversed.pieces.push_back({p.facet,p.last,p.first,end});}
      result=std::move(work.reversed);
    } else result=std::move(work.trace);
    reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
} // namespace SU2NativeGeometry
