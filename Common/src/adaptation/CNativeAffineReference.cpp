/*!
 * \file CNativeAffineReference.cpp
 * \brief Strict arithmetic and finite chart geometry; no solver or target-metric dispatch.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "../../include/adaptation/CNativeAffineReference.hpp"
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CNativeGeometryValidation2D.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
#include <numeric>
namespace SU2NativeGeometry {
namespace {
using Vec=std::array<double,3>;
Vec Values(Point p){return {p.x,p.y,p.z};}
Point ToPoint(Vec p){return {p[0],p[1],p[2]};}
void Finite(Point p){for(auto x:Values(p))if(!std::isfinite(x))throw std::invalid_argument("Nonfinite affine geometry coordinate.");}
// Each operation encloses the exact real result, including underflow. Compile without fast math/contraction.
struct Range {long double lo=0,hi=0;};
Range Add(Range a,Range b){return {std::nextafter(a.lo+b.lo,-INFINITY),std::nextafter(a.hi+b.hi,INFINITY)};}
Range Multiply(Range a,long double b){return b>=0?Range{std::nextafter(a.lo*b,-INFINITY),std::nextafter(a.hi*b,INFINITY)}:
    Range{std::nextafter(a.hi*b,-INFINITY),std::nextafter(a.lo*b,INFINITY)};}
Range Subtract(double a,double b){const auto x=static_cast<long double>(a)-b;return {std::nextafter(x,-INFINITY),std::nextafter(x,INFINITY)};}
SU2Native2D::Point Flat(Point p){return {p.x,p.y};}
}
AffineChart::AffineChart(AffineAuthority value):authority(value) {
  Finite(value.origin);Finite(value.normal);
  if((value.dimension!=2 && value.dimension!=3) || !value.version ||
     !(std::isfinite(value.maximum_error)&&value.maximum_error>=std::numeric_limits<double>::min()))throw std::invalid_argument("Invalid affine authority version/dimension/error contract.");
  const auto n=Values(value.normal);
  for(unsigned k=1;k<value.dimension;++k)if(std::abs(n[k])>std::abs(n[dependent]))dependent=k;
  if(n[dependent]==0 || (value.dimension==2 && (value.normal.z!=0 || value.origin.z!=0)))
    throw std::invalid_argument("Invalid affine normal or 2D chart embedding.");
  if(!std::numeric_limits<long double>::is_iec559 || std::numeric_limits<long double>::round_style!=std::round_to_nearest)
    throw std::invalid_argument("Affine geometry requires IEEE round-to-nearest wide arithmetic.");
}
Point AffineChart::Coordinates(Point p)const {
  Finite(p);if(authority.dimension==2 && p.z!=0)throw std::invalid_argument("2D affine query leaves its embedding.");
  const auto q=Values(p);Vec flat{};unsigned j=0;
  for(unsigned k=0;k<authority.dimension;++k)if(k!=dependent)flat[j++]=q[k];return ToPoint(flat);
}
double AffineChart::ErrorBound(Point p)const {
  Finite(p);if(authority.dimension==2 && p.z!=0)return INFINITY;
  const auto q=Values(p),o=Values(authority.origin),n=Values(authority.normal);Range residual;
  for(unsigned k=0;k<authority.dimension;++k)residual=Add(residual,Multiply(Subtract(q[k],o[k]),n[k]));
  // ||n||_infinity <= ||n||_2, so this is a conservative physical-distance bound.
  const auto bound=std::nextafter(std::max(std::abs(residual.lo),std::abs(residual.hi))/std::abs(n[dependent]),INFINITY);
  if(!std::isfinite(bound))return INFINITY;
  // A normal output floor remains conservative even when the process flushes subnormals.
  const auto result=std::nextafter(double(std::max(bound,static_cast<long double>(std::numeric_limits<double>::min()))),INFINITY);
  return std::isfinite(result)?result:INFINITY;
}
Point AffineChart::Lift(Point flat)const {
  Finite(flat);if(flat.z!=0 || (authority.dimension==2 && flat.y!=0))throw std::invalid_argument("Invalid affine chart coordinate.");
  const auto f=Values(flat),o=Values(authority.origin),n=Values(authority.normal);Vec q=o;unsigned j=0;long double sum=0;
  for(unsigned k=0;k<authority.dimension;++k)if(k!=dependent){q[k]=f[j++];sum+=static_cast<long double>(n[k])*(static_cast<long double>(q[k])-o[k]);}
  q[dependent]=double(static_cast<long double>(o[dependent])-sum/n[dependent]);const auto p=ToPoint(q);
  if(ErrorBound(p)>authority.maximum_error)throw std::invalid_argument("Affine lift cannot satisfy its declared geometry error contract.");
  return p;
}
Point AffineChart::Project(Point p)const {
  Finite(p);const auto q=Values(p),o=Values(authority.origin),n=Values(authority.normal);
  if(authority.dimension==2 && p.z!=0)throw std::invalid_argument("2D affine projection leaves its embedding.");
  if(ErrorBound(p)<=authority.maximum_error)return p;
  long double residual=0,square=0;const long double scale=std::abs(n[dependent]);
  for(unsigned k=0;k<authority.dimension;++k){const long double a=n[k]/scale;residual+=a*(static_cast<long double>(q[k])-o[k]);square+=a*a;}
  Vec foot=q;for(unsigned k=0;k<authority.dimension;++k)foot[k]=double(static_cast<long double>(q[k])-(n[k]/scale)*residual/square);
  return Lift(Coordinates(ToPoint(foot)));
}
FiniteLineReference::FiniteLineReference(AffineAuthority authority,const std::vector<LineEdge>& edges,size_t ceiling):chart(authority) {
  if(authority.dimension!=2 || edges.empty())throw std::invalid_argument("Finite line reference requires 2D intervals.");
  const auto preparation=transfer_memory::Mul(edges.size(),1024);
  if(preparation==SIZE_MAX || preparation>ceiling)throw std::invalid_argument("Finite line preparation exceeds storage budget.");
  std::map<Id,SU2Native3D::Node> points;std::map<Id,LineEdge> next;std::set<Id> previous,identities;
  std::map<double,Id> positions;
  for(auto edge:edges) {
    if(!identities.insert(edge.identity).second || edge.a.id==edge.b.id || !next.emplace(edge.a.id,edge).second || !previous.insert(edge.b.id).second)
      throw std::invalid_argument("Duplicate/branched finite line topology.");
    for(auto node:{edge.a,edge.b}) {
      if(chart.ErrorBound(node.p)>authority.maximum_error)throw std::invalid_argument("Finite line vertex leaves its declared authority.");
      const auto point=points.emplace(node.id,node);
      if(!point.second && Values(point.first->second.p)!=Values(node.p))throw std::invalid_argument("Inconsistent shared finite line vertex.");
      const auto position=positions.emplace(chart.Coordinates(node.p).x,node.id);
      if(!position.second && position.first->second!=node.id)throw std::invalid_argument("Distinct finite line features have coincident chart coordinates.");
    }
  }
  std::vector<Id> starts;for(const auto& row:next)if(!previous.count(row.first))starts.push_back(row.first);
  for(Id start:starts) {
    const double first=chart.Coordinates(points.at(start).p).x;double last=first;Id at=start,root=UINT64_MAX;int direction=0;
    while(next.count(at)) {
      const auto edge=next.at(at);next.erase(at);root=std::min(root,edge.identity);
      const auto value=chart.Coordinates(edge.b.p).x;const int sign=value>last?1:value<last?-1:0;
      if(!sign || (direction && direction!=sign))throw std::invalid_argument("Degenerate/reversed finite line interval.");
      direction=sign;last=value;at=edge.b.id;
    }
    intervals.push_back({std::min(first,last),std::max(first,last),root,first<last?start:at,first<last?at:start});
  }
  if(!next.empty())throw std::invalid_argument("Cyclic finite line topology.");
  std::sort(intervals.begin(),intervals.end(),[](const auto& a,const auto& b){return a.lo<b.lo;});
  for(size_t i=1;i<intervals.size();++i)if(intervals[i].lo<=intervals[i-1].hi)throw std::invalid_argument("Overlapping/touching disconnected finite line intervals.");
}
bool FiniteLineReference::Contains(Point p)const {
  if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;const auto x=chart.Coordinates(p).x;
  const auto it=std::upper_bound(intervals.begin(),intervals.end(),x,[](double x,const auto& interval){return x<interval.lo;});
  return it!=intervals.begin() && x<=std::prev(it)->hi;
}
bool FiniteLineReference::ContainsSegment(Point a,Point b)const {
  if(!Contains(a)||!Contains(b))return false;const auto x=chart.Coordinates(a).x,y=chart.Coordinates(b).x;
  for(const auto& interval:intervals)if(std::min(x,y)>=interval.lo && std::max(x,y)<=interval.hi)return true;return false;
}
Point FiniteLineReference::Project(Point p)const {p=chart.Project(p);if(!Contains(p))throw std::invalid_argument("Projection leaves finite line intervals.");return p;}
std::vector<Id> FiniteLineReference::FeaturesIntersecting(Point a,Point b)const {
  if(chart.ErrorBound(a)>chart.Authority().maximum_error || chart.ErrorBound(b)>chart.Authority().maximum_error)
    throw std::invalid_argument("Finite line feature query leaves its authority.");
  const auto x=chart.Coordinates(a).x,y=chart.Coordinates(b).x;std::vector<Id> found;
  for(const auto& interval:intervals){if(interval.lo>=std::min(x,y)&&interval.lo<=std::max(x,y))found.push_back(interval.first);
    if(interval.hi>=std::min(x,y)&&interval.hi<=std::max(x,y))found.push_back(interval.last);}
  std::sort(found.begin(),found.end());return found;
}
size_t FiniteLineReference::ResidentBytes()const{return transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(intervals));}
FinitePlaneReference::FinitePlaneReference(AffineAuthority authority,const std::vector<SU2Native3D::Facet>& facets,size_t ceiling):chart(authority) {
  using namespace SU2Native3D;
  if(authority.dimension!=3 || facets.empty())throw std::invalid_argument("Finite plane reference requires 3D facets.");
  const auto preparation=transfer_memory::Add(PlanarReference::ConstructionBytes(facets.size()),transfer_memory::Mul(facets.size(),4096));
  if(preparation==SIZE_MAX || preparation>ceiling)throw std::invalid_argument("Finite plane preparation exceeds storage budget.");
  auto flat=facets;std::map<Edge,std::vector<Edge>> uses;std::map<Id,Point> physical;
  for(auto& f:flat) {
    f.marker=0; // Geometry components do not depend on boundary-condition labels.
    for(auto& n:f.v){if(chart.ErrorBound(n.p)>authority.maximum_error)throw std::invalid_argument("Finite plane vertex leaves its declared authority.");
      const auto original=physical.emplace(n.id,n.p);
      if(!original.second && Values(original.first->second)!=Values(n.p))throw std::invalid_argument("Inconsistent shared finite plane vertex coordinates.");
      n.p=chart.Coordinates(n.p);const auto old=nodes.emplace(n.id,n.p);if(!old.second && Values(old.first->second)!=Values(n.p))throw std::invalid_argument("Inconsistent finite plane vertex.");}
    if(Orientation(f.v[0].p,f.v[1].p,f.v[2].p,{0,0,1})==0)throw std::invalid_argument("Degenerate finite plane facet.");
    for(unsigned k=0;k<3;++k){Edge edge{f.v[k].id,f.v[(k+1)%3].id},key=edge;std::sort(key.begin(),key.end());uses[key].push_back(edge);}
  }
  std::vector<SU2NativeBoundary2D::PolylineReference::Face> boundary;
  int orientation=0;
  for(const auto& f:flat){const int sign=Orientation(f.v[0].p,f.v[1].p,f.v[2].p,{0,0,1})>0?1:-1;
    if(orientation && sign!=orientation)throw std::invalid_argument("Inconsistent finite plane facet orientation.");orientation=sign;}
  for(const auto& row:uses) {
    if(row.second.size()==1){auto edge=row.second[0];borders.push_back(edge);if(orientation<0)std::swap(edge[0],edge[1]);
      boundary.push_back({{edge[0],Flat(nodes.at(edge[0]))},{edge[1],Flat(nodes.at(edge[1]))},0});}
    else if(row.second.size()!=2 || row.second[0][0]!=row.second[1][1] || row.second[0][1]!=row.second[1][0])
      throw std::invalid_argument("Nonmanifold/inconsistently oriented finite plane edge.");
  }
  SU2NativeBoundary2D::ValidatePhysicalGraph(boundary);
  reference=std::make_shared<PlanarReference>(std::move(flat),ceiling);
  std::set<Id> roots;for(const auto& f:facets)roots.insert(reference->Canonical(f.id));components.assign(roots.begin(),roots.end());
}
Id FinitePlaneReference::Component(Id facet)const {return reference->Canonical(facet);}
bool FinitePlaneReference::Contains(Id facet,Point p,SU2Native3D::KernelStats* stats)const {
  if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;
  return reference->Contains(facet,chart.Coordinates(p),stats);
}
bool FinitePlaneReference::ContainsSegment(Id facet,Point a,Point b,SU2Native3D::KernelStats* stats)const {
  if(!Contains(facet,a,stats)||!Contains(facet,b,stats))return false;
  return reference->ContainsSegment(facet,chart.Coordinates(a),chart.Coordinates(b),stats);
}
bool FinitePlaneReference::ContainsFace(Id facet,const std::array<Point,3>& face,SU2Native3D::KernelStats* stats)const {
  auto flat=face;for(auto& p:flat){if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;p=chart.Coordinates(p);}
  return reference->ContainsFace(facet,flat,stats);
}
bool FinitePlaneReference::Contains(Point p)const {
  if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;const auto flat=chart.Coordinates(p);
  for(Id root:components)if(reference->Contains(root,flat))return true;return false;
}
bool FinitePlaneReference::ContainsSegment(Point a,Point b)const {
  if(!Contains(a)||!Contains(b))return false;a=chart.Coordinates(a);b=chart.Coordinates(b);
  for(Id root:components)if(reference->ContainsSegment(root,a,b))return true;return false;
}
bool FinitePlaneReference::ContainsFace(const std::array<Point,3>& face)const {
  auto flat=face;for(auto& p:flat){if(chart.ErrorBound(p)>chart.Authority().maximum_error)return false;p=chart.Coordinates(p);}
  for(Id root:components)if(reference->ContainsFace(root,flat))return true;return false;
}
Point FinitePlaneReference::Project(Point p)const {p=chart.Project(p);if(!Contains(p))throw std::invalid_argument("Projection leaves finite plane regions.");return p;}
std::vector<SU2Native3D::Edge> FinitePlaneReference::FeaturesIntersecting(Point a,Point b)const {
  if(chart.ErrorBound(a)>chart.Authority().maximum_error || chart.ErrorBound(b)>chart.Authority().maximum_error)
    throw std::invalid_argument("Finite plane feature query leaves its authority.");
  const auto x=Flat(chart.Coordinates(a)),y=Flat(chart.Coordinates(b));std::vector<SU2Native3D::Edge> found;
  for(auto edge:borders)if(SU2Native2D::SegmentsIntersect(x,y,Flat(nodes.at(edge[0])),Flat(nodes.at(edge[1]))))found.push_back(edge);
  return found;
}
size_t FinitePlaneReference::ResidentBytes()const {
  return transfer_memory::Add(sizeof(*this),reference->ResidentBytes(),transfer_memory::Bytes(components),transfer_memory::Bytes(borders),
      transfer_memory::Mul(nodes.size(),sizeof(decltype(nodes)::value_type)+4*sizeof(void*)));
}
} // namespace SU2NativeGeometry
