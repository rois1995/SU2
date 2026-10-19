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
#include <type_traits>
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
double DisplacementBound(Point a,Point b) {
  Finite(a);Finite(b);const auto x=Values(a),y=Values(b);Range bound;
  if(x==y)return 0;
  for(unsigned k=0;k<3;++k) {
    const auto difference=Subtract(x[k],y[k]);
    bound=Add(bound,{0,std::max(std::abs(difference.lo),std::abs(difference.hi))});
  }
  return std::nextafter(double(bound.hi),INFINITY);
}
Point ProjectIntersection(Point point,const std::vector<AffineAuthority>& constraints) {
  Finite(point);if(constraints.empty())return point;
  std::vector<AffineChart> charts;bool satisfied=true;
  for(const auto& g:constraints) {
    charts.emplace_back(g);
    if(g.dimension!=constraints.front().dimension)throw std::invalid_argument("Conflicting affine constraint dimensions.");
    satisfied=satisfied && charts.back().ErrorBound(point)<=g.maximum_error;
  }
  if(satisfied)return point;
  if(charts.size()==1)return charts.front().Project(point);
  if(charts.size()>constraints.front().dimension)
    throw std::invalid_argument("Unsupported dependent affine junction constraints; no coordinates changed.");
  using Wide=std::array<long double,3>;std::array<Wide,3> basis{};std::array<long double,3> offset{};size_t rank=0;
  auto dot=[](Wide a,Wide b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
  for(const auto& g:constraints) {
    const auto n=Values(g.normal),o=Values(g.origin),p=Values(point);Wide row{};
    const auto scale=std::max({std::abs(n[0]),std::abs(n[1]),std::abs(n[2])});long double value=0;
    for(unsigned j=0;j<3;++j){row[j]=static_cast<long double>(n[j])/scale;value+=row[j]*(static_cast<long double>(o[j])-p[j]);}
    const auto length=std::sqrt(dot(row,row));for(auto& x:row)x/=length;value/=length;
    for(unsigned pass=0;pass<2;++pass)for(size_t k=0;k<rank;++k) {
      const auto factor=dot(row,basis[k]);value-=factor*offset[k];
      for(unsigned j=0;j<3;++j)row[j]-=factor*basis[k][j];
    }
    const auto independent=std::sqrt(dot(row,row));
    // Reject an unresolved intersection; preserving each authority is different from merging nearby planes.
    if(!(independent>1e-12L))throw std::invalid_argument("Conflicting/dependent or ill-conditioned affine junction constraints.");
    for(auto& x:row)x/=independent;basis[rank]=row;offset[rank++]=value/independent;
  }
  auto p=Values(point);Wide shift{};
  for(size_t k=0;k<rank;++k)for(unsigned j=0;j<3;++j)shift[j]+=basis[k][j]*offset[k];
  for(unsigned j=0;j<3;++j)p[j]=double(static_cast<long double>(p[j])+shift[j]);
  const auto candidate=ToPoint(p);
  for(size_t k=0;k<charts.size();++k)if(charts[k].ErrorBound(candidate)>constraints[k].maximum_error)
    throw std::invalid_argument("Simultaneous affine projection cannot meet its declared mapping errors.");
  return candidate;
}
Point SourceColumn(Point origin,const std::vector<std::array<Point,3>>& faces,double radius,
                   uint64_t& work,uint64_t maximum_work) {
  Finite(origin);
  if(!(std::isfinite(radius)&&radius>=std::numeric_limits<double>::min()) || faces.empty())
    throw std::invalid_argument("Invalid original-source column radius/skin.");
  using Wide=std::array<long double,3>;std::vector<Wide> rows;
  auto dot=[](Wide a,Wide b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];};
  for(const auto& f:faces) {
    for(auto p:f)Finite(p);
    const auto a=Values(f[0]),b=Values(f[1]),c=Values(f[2]);Wide u{},v{};
    for(unsigned k=0;k<3;++k){u[k]=static_cast<long double>(b[k])-a[k];v[k]=static_cast<long double>(c[k])-a[k];}
    Wide n{{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]}};
    const auto length=std::sqrt(dot(n,n));if(!(length>0 && std::isfinite(length)))
      throw std::invalid_argument("Degenerate original-source boundary face.");
    for(auto& x:n)x/=length;
    // Identical solver rows are redundant; no facets/authorities are merged.
    if(std::find(rows.begin(),rows.end(),n)==rows.end())rows.push_back(n);
    if(rows.size()>64)throw std::length_error("Original-source normal-cone constraint resource ceiling exceeded.");
  }
  Wide best{};long double square=INFINITY;
  auto trial=[&](std::array<size_t,3> active,size_t count) {
    if(work>=maximum_work)throw std::length_error("Original-source column work ceiling exceeded.");++work;
    std::array<Wide,3> basis{};std::array<long double,3> offset{};
    for(size_t i=0;i<count;++i) {
      Wide row=rows[active[i]];long double value=1;
      for(unsigned pass=0;pass<2;++pass)for(size_t j=0;j<i;++j) {
        const auto factor=dot(row,basis[j]);value-=factor*offset[j];
        for(unsigned k=0;k<3;++k)row[k]-=factor*basis[j][k];
      }
      const auto length=std::sqrt(dot(row,row));if(!(length>1e-12L))return;
      for(auto& x:row)x/=length;basis[i]=row;offset[i]=value/length;
    }
    Wide proposal{};for(size_t i=0;i<count;++i)for(unsigned k=0;k<3;++k)proposal[k]+=basis[i][k]*offset[i];
    for(auto row:rows)if(dot(row,proposal)<1-256*std::numeric_limits<long double>::epsilon())return;
    const auto norm=dot(proposal,proposal);if(std::isfinite(norm)&&norm<square){best=proposal;square=norm;}
  };
  for(size_t i=0;i<rows.size();++i) {
    trial({i,0,0},1);
    for(size_t j=i+1;j<rows.size();++j) {
      trial({i,j,0},2);
      for(size_t k=j+1;k<rows.size();++k)trial({i,j,k},3);
    }
  }
  if(!std::isfinite(square))throw std::invalid_argument("No admitted original-source outward normal cone.");
  auto result=Values(origin);
  for(unsigned k=0;k<3;++k)result[k]=double(static_cast<long double>(result[k])+4*static_cast<long double>(radius)*best[k]);
  const auto p=ToPoint(result);Finite(p);if(Values(p)==Values(origin))
    throw std::invalid_argument("Unrepresentable original-source column.");
  return p;
}
bool SourceFacesSeparated(const std::array<Point,3>& a,const std::array<Point,3>& b,double distance) {
  if(!(std::isfinite(distance)&&distance>0))throw std::invalid_argument("Invalid original-source separation distance.");
  for(auto p:a)Finite(p);for(auto p:b)Finite(p);
  using Box=std::array<Range,3>;
  auto subtract=[](Range a,Range b){return Add(a,{-b.hi,-b.lo});};
  auto multiply=[](Range a,Range b) {
    const std::array<long double,4> p{{a.lo*b.lo,a.lo*b.hi,a.hi*b.lo,a.hi*b.hi}};
    return Range{std::nextafter(*std::min_element(p.begin(),p.end()),-INFINITY),
                 std::nextafter(*std::max_element(p.begin(),p.end()),INFINITY)};
  };
  auto delta=[](Point a,Point b){const auto x=Values(a),y=Values(b);Box v;
    for(unsigned k=0;k<3;++k)v[k]=Subtract(x[k],y[k]);return v;};
  auto cross=[&](Box a,Box b){return Box{{subtract(multiply(a[1],b[2]),multiply(a[2],b[1])),
      subtract(multiply(a[2],b[0]),multiply(a[0],b[2])),subtract(multiply(a[0],b[1]),multiply(a[1],b[0]))}};};
  auto dot=[&](Box a,Box b){Range sum;for(unsigned k=0;k<3;++k)sum=Add(sum,multiply(a[k],b[k]));return sum;};
  auto separated=[&](Box axis) {
    long double norm=0;for(auto x:axis)norm=std::nextafter(norm+std::max(std::abs(x.lo),std::abs(x.hi)),INFINITY);
    if(!(norm>0 && std::isfinite(norm)))return false;
    long double alo=INFINITY,ahi=-INFINITY,blo=INFINITY,bhi=-INFINITY;
    for(auto p:a){const auto q=dot(axis,delta(p,a[0]));alo=std::min(alo,q.lo);ahi=std::max(ahi,q.hi);}
    for(auto p:b){const auto q=dot(axis,delta(p,a[0]));blo=std::min(blo,q.lo);bhi=std::max(bhi,q.hi);}
    const auto gap=std::max(std::nextafter(blo-ahi,-INFINITY),std::nextafter(alo-bhi,-INFINITY));
    // ||axis||_2 <= ||axis||_1; every operation encloses its real value.
    const auto required=std::nextafter(static_cast<long double>(distance)*norm,INFINITY);
    return std::isfinite(gap)&&std::isfinite(required)&&gap>=required;
  };
  for(unsigned k=0;k<3;++k){Box axis{};axis[k]={1,1};if(separated(axis))return true;}
  std::array<Box,3> ea,eb;
  for(unsigned k=0;k<3;++k){ea[k]=delta(a[(k+1)%3],a[k]);eb[k]=delta(b[(k+1)%3],b[k]);}
  if(separated(cross(ea[0],ea[1])) || separated(cross(eb[0],eb[1])))return true;
  for(auto x:ea)for(auto y:eb)if(separated(cross(x,y)))return true;
  return false;
}
namespace {
double PointModelError(const SU2Native3D::ParametricPoint& model,Point p) {
  Finite(p);const auto box=SU2Native3D::ParametricPointBounds(model);
  const auto q=Values(p),lo=Values(box[0]),hi=Values(box[1]);
  if(q==lo && q==hi)return 0;
  Range total;
  for(unsigned k=0;k<3;++k) {
    const auto a=Subtract(q[k],lo[k]),b=Subtract(q[k],hi[k]);
    total=Add(total,{0,std::max({std::abs(a.lo),std::abs(a.hi),std::abs(b.lo),std::abs(b.hi)})});
  }
  return std::nextafter(double(std::max(total.hi,static_cast<long double>(std::numeric_limits<double>::min()))),INFINITY);
}
}
MappedFacetReference::MappedFacetReference(std::shared_ptr<const SU2Native3D::PlanarReference> input,
    const std::map<Id,double>& mapping_errors,uint64_t definition_version,size_t ceiling)
    :reference(std::move(input)),version(definition_version) {
  using namespace SU2Native3D;
  if(!reference || !version || mapping_errors.empty())throw std::invalid_argument("Invalid mapped-facet reference/version/declaration.");
  const auto bound=transfer_memory::Mul(reference->Facets().size(),4096);
  if(bound==SIZE_MAX || bound>ceiling)throw std::length_error("Mapped-facet preparation exceeds storage admission.");
  for(const auto& row:mapping_errors)
    if(!reference->Facets().count(row.first) || !(std::isfinite(row.second)&&row.second>=std::numeric_limits<double>::min()))
      throw std::invalid_argument("Invalid mapped-facet identity/error declaration.");
  if(!reference->Analytic().empty())throw std::invalid_argument("Mixed analytic/mapped-facet authority is not supported.");
  errors=mapping_errors;
  std::map<Edge,std::vector<Id>> incident;std::vector<Face> skin;
  for(const auto& row:reference->Facets()) {
    const auto& f=row.second;skin.push_back({f.v[0].id,f.v[1].id,f.v[2].id});
    for(unsigned k=0;k<3;++k) {
      nodes.emplace(f.v[k].id,f.v[k].p);vertex_facets[f.v[k].id].push_back(f.id);
      Edge edge{f.v[k].id,f.v[(k+1)%3].id};std::sort(edge.begin(),edge.end());incident[edge].push_back(f.id);
    }
  }
  ValidateDomainSurfaceTopology(skin);
  edges.reserve(incident.size());edge_facets.reserve(incident.size());
  for(auto& row:incident){edges.push_back(row.first);edge_facets.push_back(std::move(row.second));}
}
Id MappedFacetReference::EdgeIdentity(SU2Native3D::Edge edge)const {
  const auto found=std::lower_bound(edges.begin(),edges.end(),edge);
  if(found==edges.end() || *found!=edge)throw std::invalid_argument("Missing canonical original mapped edge.");
  return Id(found-edges.begin());
}
void MappedFacetReference::CheckBinding(const SurfaceBinding& b)const {
  if(b.version!=version || !std::isfinite(b.u) || !std::isfinite(b.v))
    throw std::invalid_argument("Mapped binding version/nonfinite parameter conflict.");
  if(b.dimension==0) {
    if(!nodes.count(b.feature) || b.u!=0 || b.v!=0)throw std::invalid_argument("Invalid mapped vertex binding.");
  } else if(b.dimension==1) {
    if(b.feature>=edges.size() || !(b.u>0 && b.u<1) || b.v!=0)throw std::invalid_argument("Invalid mapped edge binding.");
  } else if(b.dimension==2) {
    if(!reference->Facets().count(b.feature) || !(b.u>0 && b.v>0) ||
        SU2Native3D::Orientation({1,0,0},{0,1,0},{b.u,b.v,0},{0,0,1})<=0)
      throw std::invalid_argument("Invalid mapped facet interior binding.");
  } else throw std::invalid_argument("Invalid mapped support dimension.");
}
SurfaceBinding MappedFacetReference::Vertex(Id vertex)const {
  SurfaceBinding out{vertex,version,0,0,0};CheckBinding(out);return out;
}
SurfaceBinding MappedFacetReference::EdgePoint(SU2Native3D::Edge edge,double t)const {
  if(edge[0]>=edge[1])throw std::invalid_argument("Mapped edge parameters require canonical endpoint order.");
  const auto feature=EdgeIdentity(edge);
  if(t==0)return Vertex(edge[0]);if(t==1)return Vertex(edge[1]);
  SurfaceBinding out{feature,version,1,t,0};CheckBinding(out);return out;
}
SurfaceBinding MappedFacetReference::FacetPoint(Id facet,double u,double v)const {
  SurfaceBinding out{facet,version,2,u,v};CheckBinding(out);return out;
}
SU2Native3D::ParametricPoint MappedFacetReference::Ideal(const SurfaceBinding& b)const {
  CheckBinding(b);
  if(b.dimension==0){const auto p=nodes.at(b.feature);return {p,p,p,0,0};}
  if(b.dimension==1){const auto edge=edges.at(b.feature);return {nodes.at(edge[0]),nodes.at(edge[1]),nodes.at(edge[0]),b.u,0};}
  const auto& f=reference->Get(b.feature);return {f.v[0].p,f.v[1].p,f.v[2].p,b.u,b.v};
}
double MappedFacetReference::MaximumError(const SurfaceBinding& b)const {
  CheckBinding(b);auto error=[&](Id id){const auto found=errors.find(id);return found==errors.end()?0.:found->second;};
  if(b.dimension==2)return error(b.feature);
  const auto& incident=b.dimension==0?vertex_facets.at(b.feature):edge_facets.at(b.feature);
  double bound=INFINITY;for(auto id:incident)bound=std::min(bound,error(id));return bound;
}
bool MappedFacetReference::Admissible(const SurfaceBinding& b,Point p)const {
  if(ErrorBound(b,p)>MaximumError(b))return false;
  auto plane=[&](Id id){const auto g=reference->Affine().find(id);return g==reference->Affine().end() || AffineChart(g->second).ErrorBound(p)<=g->second.maximum_error;};
  if(b.dimension==2)return plane(b.feature);
  const auto& rows=b.dimension==0?vertex_facets.at(b.feature):edge_facets.at(b.feature);
  return std::all_of(rows.begin(),rows.end(),plane);
}
bool MappedFacetReference::FeatureSegment(const SurfaceBinding& a,const SurfaceBinding& b,Id& feature,double& ta,double& tb)const {
  CheckBinding(a);CheckBinding(b);if(a.dimension>1 || b.dimension>1)return false;
  if(a.dimension==0 && b.dimension==0) {
    SU2Native3D::Edge edge{a.feature,b.feature};std::sort(edge.begin(),edge.end());
    const auto found=std::lower_bound(edges.begin(),edges.end(),edge);if(found==edges.end() || *found!=edge)return false;
    feature=Id(found-edges.begin());
  }else {feature=a.dimension==1?a.feature:b.feature;if(a.dimension==1&&b.dimension==1&&a.feature!=b.feature)return false;}
  const auto edge=edges.at(feature);
  auto parameter=[&](const SurfaceBinding& p,double& t){if(p.dimension==1){t=p.u;return true;}
    if(p.feature==edge[0] || p.feature==edge[1]){t=p.feature==edge[0]?0.:1.;return true;}return false;};
  return parameter(a,ta)&&parameter(b,tb)&&ta!=tb;
}
double MappedFacetReference::ErrorBound(const SurfaceBinding& b,Point p)const {return PointModelError(Ideal(b),p);}
Point MappedFacetReference::Realize(const SurfaceBinding& b)const {
  const auto model=Ideal(b);auto point=SU2Native3D::ParametricPointBounds(model)[0];
  std::map<Id,AffineAuthority> equations;
  ForFacets(b,[&](Id id){const auto row=reference->Affine().find(id);if(row!=reference->Affine().end())equations.emplace(row->second.identity,row->second);});
  if(!equations.empty()){std::vector<AffineAuthority> constraints;for(const auto& row:equations)constraints.push_back(row.second);point=ProjectIntersection(point,constraints);}
  if(!Admissible(b,point))throw std::invalid_argument("Mapped realization cannot satisfy all incident geometry error contracts.");
  return point;
}
bool MappedFacetReference::Supports(Id facet,const SurfaceBinding& b)const {
  CheckBinding(b);const auto& f=reference->Get(facet);
  auto has=[&](Id id){return std::any_of(f.v.begin(),f.v.end(),[&](const auto& n){return n.id==id;});};
  return b.dimension==0?has(b.feature):b.dimension==1?(has(edges.at(b.feature)[0])&&has(edges.at(b.feature)[1])):b.feature==facet;
}
bool MappedFacetReference::ContainsFace(Id facet,const std::array<SurfaceBinding,3>& binding,
    const std::array<Point,3>& realized,SU2Native3D::KernelStats* stats)const {
  using namespace SU2Native3D;
  std::array<ParametricPoint,3> ideal;
  for(unsigned i=0;i<3;++i) {
    if(!Supports(facet,binding[i]) || !Admissible(binding[i],realized[i]))return false;
    ideal[i]=Ideal(binding[i]);
  }
  const auto& f=reference->Get(facet);
  const ParametricPoint a{f.v[0].p},b{f.v[1].p},c{f.v[2].p};
  for(unsigned axis=0;axis<3;++axis) {
    const int old=ParametricOrientation(a,b,c,axis,stats);
    if(old)return ParametricOrientation(ideal[0],ideal[1],ideal[2],axis,stats)==old;
  }
  throw std::logic_error("Degenerate admitted mapped facet.");
}
size_t MappedFacetReference::ResidentBytes()const {
  auto bytes=[](const auto& map){return transfer_memory::Mul(map.size(),sizeof(typename std::decay_t<decltype(map)>::value_type)+4*sizeof(void*));};
  size_t out=transfer_memory::Add(sizeof(*this),bytes(errors),bytes(nodes),bytes(vertex_facets),transfer_memory::Bytes(edges),transfer_memory::Bytes(edge_facets));
  for(const auto& row:vertex_facets)out=transfer_memory::Add(out,transfer_memory::Bytes(row.second));
  for(const auto& row:edge_facets)out=transfer_memory::Add(out,transfer_memory::Bytes(row));return out;
}
bool MappedFacetReference::ValidateSurface(const std::vector<SU2Native3D::SurfaceFace>& surface,
    const std::map<Id,SurfaceBinding>& bindings,const std::map<Id,Point>& realized,MappedSurfaceAudit& report,
    std::string& reason,size_t maximum_faces,size_t ceiling)const {
  using namespace SU2Native3D;
  try {
    if(surface.empty() || surface.size()>maximum_faces)throw std::length_error("Mapped surface face work admission exhausted.");
    MappedSurfaceAudit audit;
    audit.requested_bytes=transfer_memory::Add(ResidentBytes(),transfer_memory::Mul(surface.size(),4096),transfer_memory::Mul(bindings.size(),256));
    if(audit.requested_bytes==SIZE_MAX || audit.requested_bytes>ceiling)throw std::length_error("Mapped surface audit storage admission exhausted.");
    std::map<Id,std::vector<Face>> groups;std::vector<Face> skin;std::set<Id> used;
    for(const auto& face:surface) {
      std::array<SurfaceBinding,3> b;std::array<Point,3> p;
      for(unsigned k=0;k<3;++k){b[k]=bindings.at(face.v[k]);p[k]=realized.at(face.v[k]);used.insert(face.v[k]);
        audit.maximum_mapping_error=std::max(audit.maximum_mapping_error,ErrorBound(b[k],p[k]));}
      if(!ContainsFace(face.facet,b,p))throw std::invalid_argument("Mapped face leaves its finite support, reverses, or exceeds mapping error.");
      groups[face.facet].push_back(face.v);skin.push_back(face.v);
    }
    if(used.size()!=bindings.size() || used.size()!=realized.size() || groups.size()!=reference->Facets().size())
      throw std::invalid_argument("Mapped boundary bindings or original facet coverage are incomplete.");
    ValidateDomainSurfaceTopology(skin);
    // Canonical original support and scalar parameter identify one current shared node, independent of rank IDs.
    std::map<Id,Id> vertices;std::map<std::pair<Id,double>,Id> edge_nodes;
    for(const auto& row:bindings) {
      const auto& b=row.second;
      if(b.dimension==0 && !vertices.emplace(b.feature,row.first).second)throw std::invalid_argument("Duplicate mapped original corner.");
      if(b.dimension==1 && !edge_nodes.emplace(std::make_pair(b.feature,b.u),row.first).second)throw std::invalid_argument("Duplicate mapped feature parameter.");
    }
    for(const auto& group:groups) {
      ValidateSurfaceTopology(group.second,false,maximum_faces);
      const auto& original=reference->Get(group.first);
      std::map<Edge,std::vector<Edge>> uses;
      for(const auto& face:group.second)for(unsigned k=0;k<3;++k){Edge e{face[k],face[(k+1)%3]},key=e;std::sort(key.begin(),key.end());uses[key].push_back(e);}
      std::map<Id,std::vector<std::pair<double,double>>> chains;
      for(const auto& row:uses)if(row.second.size()==1) {
        const auto directed=row.second.front();bool found=false;
        for(unsigned k=0;k<3;++k) {
          Edge edge{original.v[k].id,original.v[(k+1)%3].id};const bool forward=edge[0]<edge[1];std::sort(edge.begin(),edge.end());const Id feature=EdgeIdentity(edge);
          auto parameter=[&](const SurfaceBinding& b,double& t) {
            if(b.dimension==1 && b.feature==feature){t=b.u;return true;}
            if(b.dimension==0 && (b.feature==edge[0]||b.feature==edge[1])){t=b.feature==edge[0]?0.:1.;return true;}return false;
          };
          double a,b;
          if(!parameter(bindings.at(directed[0]),a)||!parameter(bindings.at(directed[1]),b))continue;
          if(!(forward?a<b:b<a))throw std::invalid_argument("Mapped feature chain reverses or collapses.");
          chains[feature].push_back({std::min(a,b),std::max(a,b)});found=true;++audit.feature_segments;break;
        }
        if(!found)throw std::invalid_argument("Mapped facet border leaves its original feature.");
      }
      if(chains.size()!=3)throw std::invalid_argument("Mapped facet has missing border features.");
      for(auto& row:chains) {
        auto& chain=row.second;std::sort(chain.begin(),chain.end());double end=0;
        for(const auto& interval:chain){if(interval.first!=end)throw std::invalid_argument("Mapped feature chain has a gap or overlap.");end=interval.second;}
        if(end!=1)throw std::invalid_argument("Mapped feature chain does not cover its complete finite interval.");
      }
    }
    audit.nodes=used.size();audit.faces=surface.size();report=audit;reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
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
