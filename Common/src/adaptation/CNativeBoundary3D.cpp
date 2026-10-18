/*!
 * \file CNativeBoundary3D.cpp
 * \brief Exact faceted surface and volume bisection, inverse pairing and private rollback.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeBoundary3D.hpp"
#include "../../include/adaptation/CTransferMemory.hpp"
#include "../../include/adaptation/CNativeMetric3D.hpp"
#include "../../include/adt/CADTElemClass.hpp"
#include "../../include/option_structure.hpp"
#include <cmath>
#include <functional>
#include <iomanip>
#include <sstream>
#include <type_traits>
#include <algorithm>
#include <limits>
#include <set>
#include <tuple>
#include <stdexcept>
namespace SU2Native3D {
namespace {
using Vector = std::array<long double, 3>;
Vector Delta(Point a, Point b) {
  return {static_cast<long double>(a.x)-b.x, static_cast<long double>(a.y)-b.y, static_cast<long double>(a.z)-b.z};
}
Vector Cross(Vector a, Vector b) {
  return {a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]};
}
long double Dot(Vector a, Vector b) { return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]; }
long double Norm(Vector a) { return std::hypot(a[0], a[1], a[2]); }
long double BumpHeight(const AnalyticSurface& g, long double x, long double y) {
  const long double u=(x-g.origin.x)/g.extent.x;
  const long double v=(y-g.origin.y)/g.extent.y;
  return g.origin.z + static_cast<long double>(g.extent.z)*u*(1-u)*v*(1-v);
}
long double BumpHeight(const AnalyticSurface& g, Point p) { return BumpHeight(g,p.x,p.y); }
long double ArcDistance(const AnalyticSurface& g, Point a, Point b, Point p) {
  auto u=Delta(a,g.origin),v=Delta(b,g.origin);
  const auto w=Delta(p,g.origin);
  const auto ul=Norm(u),vl=Norm(v);
  if (!(ul>0 && vl>0)) throw std::invalid_argument("Invalid native3D finite radial edge.");
  for(size_t i=0;i<3;++i){u[i]/=ul;v[i]/=vl;}
  const auto n=Cross(u,v);
  const auto square=Dot(n,n);
  if (!(square>0 && std::isfinite(square))) throw std::invalid_argument("Degenerate native3D finite radial edge.");
  Vector q{},du{},dv{};
  const auto normal=Dot(n,w)/square;
  for(size_t i=0;i<3;++i){q[i]=w[i]-normal*n[i];du[i]=w[i]-g.extent.x*u[i];dv[i]=w[i]-g.extent.x*v[i];}
  auto distance=std::min(Norm(du),Norm(dv));
  const auto length=Norm(q);
  if (length>0 && std::isfinite(length)) {
    for(auto& x:q)x/=length;
    if (Dot(n,Cross(u,q))>=0 && Dot(n,Cross(q,v))>=0) {
      for(size_t i=0;i<3;++i)q[i]=w[i]-g.extent.x*q[i];
      distance=std::min(distance,Norm(q));
    }
  }
  return distance;
}
Vector UnitAxis(const AnalyticSurface& g) {
  Vector a{g.axis.x,g.axis.y,g.axis.z};const auto n=Norm(a);
  if(!(n>0 && std::isfinite(n)))throw std::invalid_argument("Invalid native3D analytic axis.");
  for(auto& v:a)v/=n;return a;
}
struct AxisFrame {
  Vector axis,u,v;
  explicit AxisFrame(const AnalyticSurface& g):axis(UnitAxis(g)) {
    size_t k=0;for(size_t i=1;i<3;++i)if(std::abs(axis[i])<std::abs(axis[k]))k=i;
    Vector seed{};seed[k]=1;u=Cross(seed,axis);const auto n=Norm(u);for(auto& x:u)x/=n;v=Cross(axis,u);
  }
  Point Coordinates(const AnalyticSurface& g,Point p) const {
    const auto d=Delta(p,g.origin);return {double(Dot(d,u)),double(Dot(d,v)),double(Dot(d,axis))};
  }
};
std::array<Point,3> CylinderChart(const AnalyticSurface& g,const std::array<Point,3>& p) {
  const AxisFrame frame(g);std::array<Point,3> chart{};long double first=0;
  for(size_t i=0;i<3;++i) {
    const auto local=frame.Coordinates(g,p[i]);const auto angle=std::atan2(static_cast<long double>(local.y),local.x);
    if(!i)first=angle;
    chart[i]={double(g.extent.x*std::remainder(angle-first,2*std::acos(-1.L))),local.z,0};
  }
  const auto ends=std::minmax_element(chart.begin(),chart.end(),[](Point a,Point b){return a.x<b.x;});
  if(ends.second->x-ends.first->x>=g.extent.x*std::acos(-1.L))
    throw std::invalid_argument("Native3D cylinder face crosses its radial horizon.");
  return chart;
}
std::array<Point,3> DiskChart(const AnalyticSurface& g,const std::array<Point,3>& p) {
  const AxisFrame frame(g);std::array<Point,3> chart{};
  for(size_t i=0;i<3;++i){chart[i]=frame.Coordinates(g,p[i]);chart[i].z=0;}return chart;
}
long double ParameterOrientation(const AnalyticSurface& g,const std::array<Point,3>& p) {
  const auto chart=g.kind==AnalyticSurface::Kind::Cylinder ? CylinderChart(g,p) : DiskChart(g,p);
  return Orientation(chart[0],chart[1],chart[2],{0,0,1});
}
bool Has(Face f, Id id) { return std::find(f.begin(), f.end(), id) != f.end(); }
Face Key(Face f) {
  std::sort(f.begin(), f.end());
  return f;
}
Face Oriented(Face f) {
  std::rotate(f.begin(), std::min_element(f.begin(), f.end()), f.end());
  return f;
}
long double Projected(Point a, Point b, Point c, size_t axis, KernelStats* stats) {
  auto project = [axis](Point p) {
    return axis == 0 ? Point{p.y, p.z, 0} : axis == 1 ? Point{p.x, p.z, 0} : Point{p.x, p.y, 0};
  };
  return Orientation(project(a), project(b), project(c), {0, 0, 1}, stats);
}
bool OnEdge(Point a, Point b, Point p, KernelStats* stats) {
  for (size_t axis = 0; axis < 3; ++axis)
    if (Projected(a, b, p, axis, stats) != 0) return false;
  return p.x >= std::min(a.x, b.x) && p.x <= std::max(a.x, b.x) && p.y >= std::min(a.y, b.y) &&
         p.y <= std::max(a.y, b.y) && p.z >= std::min(a.z, b.z) && p.z <= std::max(a.z, b.z);
}
Point PlaneCoordinates(Point p,size_t axis) {
  return axis==0?Point{p.y,p.z,0}:axis==1?Point{p.x,p.z,0}:Point{p.x,p.y,0};
}
bool BorderCrossesTriangleInterior(Point a,Point b,std::array<Point,3> t,KernelStats* stats) {
  const auto sign=Orientation(t[0],t[1],t[2],{0,0,1},stats);
  for(size_t i=0;i<3;++i) {
    const auto x=Orientation(t[i],t[(i+1)%3],a,{0,0,1},stats),y=Orientation(t[i],t[(i+1)%3],b,{0,0,1},stats);
    if((x==0 && y==0) || (sign>0 ? x<=0 && y<=0:x>=0 && y>=0))return false;
  }
  if(sign<0)std::swap(t[1],t[2]);
  // The base face is the finite triangle. Exact positive-length clipping excludes isolated contacts.
  return IntersectsSegmentPositiveLength({t[0],t[1],t[2],{0,0,1}},a,b,stats);
}
std::vector<Edge> StraightBorders(const std::vector<Edge>& border, const std::map<Id, Node>& nodes) {
  std::map<Id, Id> next, previous;
  for (auto edge : border)
    if (!next.emplace(edge[0], edge[1]).second || !previous.emplace(edge[1], edge[0]).second) return border;
  std::set<Id> corners;
  for (const auto& edge : next) {
    const auto before = previous.find(edge.first);
    if (before == previous.end()) return border;
    if (!OnEdge(nodes.at(before->second).p, nodes.at(edge.second).p, nodes.at(edge.first).p, nullptr))
      corners.insert(edge.first);
  }
  std::vector<Edge> result;
  size_t walked = 0;
  for (Id first : corners) {
    Id last = first;
    do {
      if (++walked > border.size()) return border;
      last = next.at(last);
    } while (!corners.count(last));
    result.push_back({first, last});
  }
  // Remove only forward-collinear subdivisions: identical oriented half-planes, including separate hole loops.
  return walked == border.size() ? result : border;
}
std::map<Id, Node> Nodes(const std::vector<Cell>& cells) {
  std::map<Id, Node> result;
  for (const auto& c : cells)
    for (auto n : c.v) result.emplace(n.id, n);
  return result;
}
using Skin = std::map<Face, Face>;
Skin SkinMap(const std::vector<Face>& faces) {
  Skin skin;
  for (auto f : faces) skin.emplace(Key(f), Oriented(f));
  return skin;
}
std::vector<Face> SkinVector(const Skin& skin) {
  std::vector<Face> result;
  result.reserve(skin.size());
  for (const auto& f : skin) result.push_back(f.second);
  return result;
}
using Physical = std::map<Face, SurfaceFace>;
Physical CheckSurface(const Patch& patch, const Skin& skin, const PlanarReference& reference, KernelStats* stats) {
  if (patch.surface.size() > 4 * MaximumReplacementCells)
    throw std::invalid_argument("Native 3D physical skin exceeds budget.");
  const auto nodes = Nodes(patch.cells);
  Physical faces;
  for (auto f : patch.surface) {
    f.v = Oriented(f.v);
    f.facet = reference.Canonical(f.facet);
    const auto key = Key(f.v);
    const auto found = skin.find(key);
    if (found == skin.end() || found->second != f.v || !faces.emplace(key, f).second)
      throw std::invalid_argument("Native 3D physical face is duplicate or not the oriented patch skin.");
    const std::array<Point,3> triangle{nodes.at(f.v[0]).p, nodes.at(f.v[1]).p, nodes.at(f.v[2]).p};
    if (!reference.ContainsFace(f.facet, triangle, stats))
      throw std::invalid_argument("Native 3D physical face leaves or reverses original facet " +
                                  std::to_string(f.facet) + ".");
  }
  return faces;
}
Node ProjectSplitPoint(Edge edge,Node point,const std::map<Id,Node>& nodes,const Physical& physical,
                       const PlanarReference& reference,KernelStats* stats,bool cavity=false) {
  const auto a=nodes.at(edge[0]),b=nodes.at(edge[1]);
    bool curved_edge=false;
    for(const auto& face:physical)if(Has(face.second.v,edge[0]) && Has(face.second.v,edge[1])) {
      const auto geometry=reference.Analytic().find(face.second.facet);
      curved_edge=curved_edge || reference.Affine().count(face.second.facet) || (geometry!=reference.Analytic().end() && geometry->second.kind!=AnalyticSurface::Kind::Disk);
    }
    const Point midpoint{double((static_cast<long double>(a.p.x)+b.p.x)/2),double((static_cast<long double>(a.p.y)+b.p.y)/2),double((static_cast<long double>(a.p.z)+b.p.z)/2)};
    const bool rounded_midpoint=curved_edge && point.p.x==midpoint.x && point.p.y==midpoint.y && point.p.z==midpoint.z;
    bool same_chart=false,curved_feature=false;
    if(cavity) {
      Id component=UINT64_MAX;unsigned incident=0;same_chart=true;
      for(const auto& f:physical)if(Has(f.second.v,edge[0]) && Has(f.second.v,edge[1])) {
        const auto next=reference.Canonical(f.second.facet);
        if(component==UINT64_MAX)component=next;else same_chart=same_chart && component==next;
        ++incident;
      }
      same_chart=same_chart && incident==2;
      curved_feature=curved_edge && incident==2 && !same_chart;
    }
    // Same-chart fans certify their border; a curved feature instead certifies its finite projected parameter.
    if (nodes.count(point.id) || (!rounded_midpoint && !same_chart && !curved_feature && !OnEdge(a.p, b.p, point.p, stats)) ||
        (point.p.x == a.p.x && point.p.y == a.p.y && point.p.z == a.p.z) ||
        (point.p.x == b.p.x && point.p.y == b.p.y && point.p.z == b.p.z))
      throw std::invalid_argument("Native 3D split point is not a distinct exact on-edge point.");
    for(unsigned pass=0;pass<2;++pass)for(const auto& f:physical)
      if(Has(f.second.v,edge[0]) && Has(f.second.v,edge[1]) && reference.Declared(f.second.facet) &&
         (reference.Affine().count(f.second.facet) || reference.Analytic().at(f.second.facet).kind==AnalyticSurface::Kind::Disk)==(pass==0))
        point.p=reference.Project(f.second.facet,point.p,stats);
    for (const auto& f : physical)if(Has(f.second.v,edge[0]) && Has(f.second.v,edge[1])) {
      if(!reference.Contains(f.second.facet,point.p,stats))
        throw std::invalid_argument("Native3D projected split leaves an incident physical feature.");
      const auto geometry=reference.Analytic().find(f.second.facet);
      if(curved_feature && geometry!=reference.Analytic().end() && geometry->second.kind!=AnalyticSurface::Kind::Disk &&
         !reference.OnProjectedEdge(f.second.facet,a.p,b.p,point.p,stats))
        throw std::invalid_argument("Native3D projected split leaves the finite curved feature parameter.");
    }
  return point;
}
Node ProjectMovePoint(Id vertex,Node point,const Skin& skin,const Physical& physical,
                      const PlanarReference& reference,KernelStats* stats) {
  for(unsigned pass=0;pass<2;++pass)for(const auto& face:physical)
    if(Has(face.second.v,vertex) && reference.Declared(face.second.facet) &&
       (reference.Affine().count(face.second.facet) || reference.Analytic().at(face.second.facet).kind==AnalyticSurface::Kind::Disk)==(pass==0))
      point.p=reference.Project(face.second.facet,point.p,stats);
  for(const auto& face:skin)if(Has(face.second,vertex)) {
    const auto marked=physical.find(face.first);
    if(marked==physical.end())throw std::invalid_argument("Native 3D incomplete movement dependency at interface node "+std::to_string(vertex)+".");
    if(!reference.Contains(marked->second.facet,point.p,stats))
      throw std::invalid_argument("Native 3D movement leaves incident reference facet "+std::to_string(marked->second.facet)+".");
  }
  return point;
}
void SplitSkin(Edge edge,Node point,Skin& skin,Physical& physical) {
  const auto before=skin;
  for(const auto& entry:before)if(Has(entry.second,edge[0]) && Has(entry.second,edge[1])) {
    const auto found=physical.find(entry.first);
    if(found==physical.end())throw std::invalid_argument("Native3D incomplete split dependency at interface face.");
    const auto facet=found->second.facet;physical.erase(found);skin.erase(entry.first);
    const auto f=entry.second;
    for(size_t i=0;i<3;++i)if((f[i]==edge[0] && f[(i+1)%3]==edge[1]) || (f[i]==edge[1] && f[(i+1)%3]==edge[0])) {
      for(auto child:{Face{f[i],point.id,f[(i+2)%3]},Face{point.id,f[(i+1)%3],f[(i+2)%3]}}) {
        child=Oriented(child);skin.emplace(Key(child),child);physical.emplace(Key(child),SurfaceFace{child,facet});
      }
      break;
    }
  }
}
void Bounds(const Patch& patch) {
  if (patch.cells.empty() || patch.cells.size() > MaximumCavityCells)
    throw std::invalid_argument("Native 3D edge patch is empty or exceeds cell budget.");
}
void Append(std::vector<Cell>& cells, std::array<Node, 4> nodes, Id first, KernelStats* stats) {
  if (cells.size() >= MaximumReplacementCells || first > std::numeric_limits<Id>::max() - cells.size())
    throw std::invalid_argument("Native 3D edge proposal exceeds cell/ID budget.");
  const auto sign = Orientation(nodes[0].p, nodes[1].p, nodes[2].p, nodes[3].p, stats);
  if (sign == 0) throw std::invalid_argument("Native 3D edge proposal has a degenerate tetrahedron.");
  if (sign < 0) std::swap(nodes[0], nodes[1]);
  cells.push_back({first + cells.size(), nodes});
}
void Finish(const Patch& old, Patch& proposal, const Skin& expected, const Physical& physical,
            const PlanarReference& reference, KernelStats* stats) {
  for (const auto& f : physical) proposal.surface.push_back(f.second);
  std::string reason;
  if (!ValidatePrescribedInterface(old.cells, proposal.cells, SkinVector(expected), reason, stats))
    throw std::invalid_argument(reason);
  CheckSurface(proposal, expected, reference, stats);
}
void Endpoints(Edge& edge) {
  if (edge[0] == edge[1]) throw std::invalid_argument("Native 3D edge repeats an endpoint.");
  std::sort(edge.begin(), edge.end());
}
}  // namespace
bool CheckFirstHeight(const Patch& patch,const std::map<int,double>& heights,const PlanarReference& reference,
                      FirstHeightReport& report,std::string& reason,KernelStats* stats) {
  report={};
  try {
    for(const auto& height:heights)if(!(height.second>0) || !std::isfinite(height.second))
      throw std::invalid_argument("Native3D wall height must be finite and positive.");
    const auto skin=SkinMap(Incidence(patch.cells,stats).Boundary());
    const auto physical=CheckSurface(patch,skin,reference,stats);
    const auto nodes=Nodes(patch.cells);
    for(const auto& entry:physical) {
      const auto& face=entry.second;
      const auto height=heights.find(reference.Get(face.facet).marker);
      if(height==heights.end())continue;
      ++report.wall_faces;
      const auto a=nodes.at(face.v[0]).p,b=nodes.at(face.v[1]).p,c=nodes.at(face.v[2]).p;
      auto normal=Cross(Delta(b,a),Delta(c,a));const auto area=Norm(normal);
      if(!(area>0))throw std::invalid_argument("Native3D first-height face is degenerate.");
      for(auto& x:normal)x/=area;
      Point center{double(((long double)a.x+b.x+c.x)/3),double(((long double)a.y+b.y+c.y)/3),
                   double(((long double)a.z+b.z+c.z)/3)};
      size_t found=0;Node apex;Id cell=0;
      for(const auto& candidate:patch.cells) {
        unsigned count=0;Node opposite;
        for(auto node:candidate.v) {
          if(std::find(face.v.begin(),face.v.end(),node.id)!=face.v.end())++count;
          else opposite=node;
        }
        if(count==3){++found;apex=opposite;cell=candidate.id;}
      }
      if(found!=1){++report.missing_cells;throw std::invalid_argument("Native3D wall face has no unique first child.");}
      if(reference.CompleteAnalytic(face.facet)) {
        const auto closest=reference.ClosestAnalytic(face.facet,apex.p);
        const auto n=closest.normal,d=closest.direction;
        const long double clearance=-closest.distance*((long double)d.x*n.x+(long double)d.y*n.y+(long double)d.z*n.z);
        if(clearance/height->second<report.minimum_reference_clearance_ratio) {
          report.minimum_reference_clearance_ratio=double(clearance/height->second);
          report.reference_clearance_facet=face.facet;report.reference_clearance_cell=cell;report.reference_clearance_apex=apex.p;
        }
        if(!(clearance>0))++report.reference_clearance_failures;
      }
      const auto altitude=std::abs(Orientation(a,b,c,apex.p,stats))/area;
      const double error=double(std::abs(altitude/height->second-1));
      auto tangent=Delta(apex.p,center);const auto projection=Dot(tangent,normal);
      for(unsigned j=0;j<3;++j)tangent[j]-=projection*normal[j];
      report.maximum_tilt=std::max(report.maximum_tilt,double(Norm(tangent)/height->second));
      // Normal-foot barycentrics use tangential areas, without rounding a projected point.
      const std::array<Vector,3> relative{{Delta(a,apex.p),Delta(b,apex.p),Delta(c,apex.p)}};
      long double minimum=1./3;bool outside=false;
      for(unsigned j=0;j<3;++j) {
        const auto& u=relative[(j+1)%3];const auto& v=relative[(j+2)%3];
        const auto weight=Dot(Cross(u,v),normal)/area;
        const auto tolerance=256*std::numeric_limits<long double>::epsilon()*std::max(1.L,Norm(u)*Norm(v)/area);
        if(!std::isfinite(weight) || tolerance>1e-8L)
          throw std::invalid_argument("Native3D first-row footprint cannot be resolved at cell "+std::to_string(cell));
        minimum=std::min(minimum,weight);outside=outside || weight < -tolerance;
      }
      report.footprint_failures+=outside;
      if(minimum<report.minimum_barycentric) {
        report.minimum_barycentric=double(minimum);report.footprint_facet=face.facet;report.footprint_cell=cell;
      }
      report.height_failures+=error>1e-8;
      if(error>report.maximum_relative_error) {
        report.maximum_relative_error=error;report.worst_facet=face.facet;report.worst_cell=cell;report.worst_apex=apex.p;
      }
    }
    if(report.reference_clearance_failures)throw std::invalid_argument("Native3D geometry-height incompatibility of the normal-row proposal at facet "+
        std::to_string(report.reference_clearance_facet)+" cell "+std::to_string(report.reference_clearance_cell)+
        " clearance/h0="+std::to_string(report.minimum_reference_clearance_ratio)+
        ": apex crosses smooth reference; resolve curvature before constructing the row.");
    if(report.height_failures)throw std::invalid_argument("Native3D prescribed height fails at facet "+std::to_string(report.worst_facet)+
        " cell "+std::to_string(report.worst_cell)+" relative error "+std::to_string(report.maximum_relative_error));
    if(report.footprint_failures)throw std::invalid_argument("Native3D first-row normal foot leaves wall triangle at facet "+
        std::to_string(report.footprint_facet)+" cell "+std::to_string(report.footprint_cell)+
        " minimum barycentric "+std::to_string(report.minimum_barycentric));
    reason.clear();return true;
  }catch(const std::exception& error){reason=error.what();return false;}
}

bool ConstructFirstLayer(const Patch& old,Node background,Id first_wall_node,Id first_cell,
                         const std::map<int,double>& heights,const PlanarReference& reference,
                         Patch& fresh,FirstHeightReport& report,std::string& reason,KernelStats* stats) {
  report={};
  try {
    Bounds(old);
    if(heights.empty())throw std::invalid_argument("Native3D first-layer construction needs wall heights.");
    for(const auto& height:heights)if(!(height.second>0) || !std::isfinite(height.second))
      throw std::invalid_argument("Native3D wall height must be finite and positive.");
    const auto skin=SkinMap(Incidence(old.cells,stats).Boundary());
    const auto physical=CheckSurface(old,skin,reference,stats);const auto nodes=Nodes(old.cells);
    if(nodes.count(background.id))throw std::invalid_argument("Native3D background identity is not newly reserved.");
    size_t count=0;for(const auto& face:physical)count+=heights.count(reference.Get(face.second.facet).marker);
    if(!count || count>MaximumReplacementCells || first_wall_node>UINT64_MAX-(count-1))
      throw std::invalid_argument("Native3D first-layer node range/coverage exceeds budget.");
    for(size_t k=0;k<count;++k)if(nodes.count(first_wall_node+k) || background.id==first_wall_node+k)
      throw std::invalid_argument("Native3D wall apex identity is not newly reserved.");
    Patch proposal;
    if(!Cone(old.cells,background,first_cell,proposal.cells,reason,stats))throw std::invalid_argument(reason);
    Id next=first_wall_node;
    for(const auto& entry:physical) {
      const auto& face=entry.second;const auto height=heights.find(reference.Get(face.facet).marker);
      if(height==heights.end())continue;
      const auto a=nodes.at(face.v[0]).p,b=nodes.at(face.v[1]).p,c=nodes.at(face.v[2]).p;
      auto normal=Cross(Delta(b,a),Delta(c,a));const auto length=Norm(normal);
      if(!(length>0))throw std::invalid_argument("Native3D first-layer face is degenerate.");
      for(auto& value:normal)value/=length;
      Point center{double(((long double)a.x+b.x+c.x)/3),double(((long double)a.y+b.y+c.y)/3),
                   double(((long double)a.z+b.z+c.z)/3)};
      // The oriented physical skin points out of the cavity; use its inward normal.
      Node apex{next++,{double((long double)center.x-height->second*normal[0]),
                       double((long double)center.y-height->second*normal[1]),
                       double((long double)center.z-height->second*normal[2])}};
      auto found=proposal.cells.end();
      for(auto cell=proposal.cells.begin();cell!=proposal.cells.end();++cell) {
        unsigned matches=0;for(auto node:cell->v)matches+=std::find(face.v.begin(),face.v.end(),node.id)!=face.v.end();
        if(matches==3){found=cell;break;}
      }
      if(found==proposal.cells.end())throw std::invalid_argument("Native3D background fan misses wall coverage.");
      const auto opposite=std::find_if(found->v.begin(),found->v.end(),[&](Node node){return std::find(face.v.begin(),face.v.end(),node.id)==face.v.end();});
      long double admittedClearance=0;
      for(const auto& node:nodes)admittedClearance=std::max(admittedClearance,-Dot(Delta(node.second.p,center),normal));
      if(!(admittedClearance>height->second))
        throw std::invalid_argument("Native3D geometry-height incompatibility within admitted volume at facet "+std::to_string(face.facet));
      const auto clearance=-Dot(Delta(opposite->p,center),normal);
      if(!(clearance>height->second))throw std::invalid_argument("Native3D background obstruction at facet "+std::to_string(face.facet));
      std::vector<Cell> children;
      if(!Cone({*found},apex,first_cell,children,reason,stats))
        throw std::invalid_argument("Native3D normal wall child obstructed at facet "+std::to_string(face.facet)+": "+reason);
      const auto index=found-proposal.cells.begin();proposal.cells.erase(proposal.cells.begin()+index);
      proposal.cells.insert(proposal.cells.end(),children.begin(),children.end());
      if(proposal.cells.size()>MaximumReplacementCells)throw std::invalid_argument("Native3D first layer exceeds replacement budget.");
      if(first_cell>UINT64_MAX-(proposal.cells.size()-1))throw std::invalid_argument("Native3D first-layer cell range overflows.");
      for(size_t k=0;k<proposal.cells.size();++k)proposal.cells[k].id=first_cell+k;
    }
    Finish(old,proposal,skin,physical,reference,stats);
    if(!CheckFirstHeight(proposal,heights,reference,report,reason,stats))throw std::invalid_argument(reason);
    fresh=std::move(proposal);reason.clear();return true;
  }catch(const std::exception& error){reason=error.what();return false;}
}

Node ConstrainMovePoint(const Patch& old,Id vertex,Node proposal,const std::map<int,double>& heights,
                        const PlanarReference& reference) {
  // Private reconnection may already have produced the admitted replacement-size patch.
  if(old.cells.empty() || old.cells.size()>MaximumReplacementCells)
    throw std::invalid_argument("Native3D constrained movement exceeds replacement budget.");
  const auto nodes=Nodes(old.cells);const auto origin=nodes.at(vertex).p;
  if(!std::isfinite(proposal.p.x) || !std::isfinite(proposal.p.y) || !std::isfinite(proposal.p.z))
    throw std::invalid_argument("Nonfinite native3D constrained movement.");
  std::array<Vector,3> basis{};size_t rank=0;
  auto constrain=[&](Vector normal) {
    const auto length=Norm(normal);if(!(length>0))throw std::invalid_argument("Degenerate native3D movement constraint.");
    for(auto& x:normal)x/=length;
    for(unsigned pass=0;pass<2;++pass)for(size_t i=0;i<rank;++i) {
      const auto component=Dot(normal,basis[i]);for(unsigned j=0;j<3;++j)normal[j]-=component*basis[i][j];
    }
    const auto independent=Norm(normal);
    if(rank<3 && independent>64*std::numeric_limits<long double>::epsilon()) {
      for(auto& x:normal)x/=independent;basis[rank++]=normal;
    }
  };
  std::set<Id> incident;
  for(const auto& face:old.surface) {
    const bool onFace=Has(face.v,vertex);
    if(onFace && !reference.Analytic().count(face.facet))incident.insert(face.facet);
    bool apex=false;
    if(!onFace && heights.count(reference.Get(face.facet).marker))for(const auto& cell:old.cells) {
      unsigned count=0;bool hasVertex=false;
      for(const auto& n:cell.v){count+=std::find(face.v.begin(),face.v.end(),n.id)!=face.v.end();hasVertex=hasVertex || n.id==vertex;}
      apex=apex || (count==3 && hasVertex);
    }
    if(!apex && (!onFace || reference.Analytic().count(face.facet)))continue;
    const auto plane=reference.Affine().find(face.facet);
    if(onFace && plane!=reference.Affine().end()) {
      const auto n=plane->second.normal;constrain({n.x,n.y,n.z});continue;
    }
    constrain(Cross(Delta(nodes.at(face.v[1]).p,nodes.at(face.v[0]).p),Delta(nodes.at(face.v[2]).p,nodes.at(face.v[0]).p)));
  }
  // Coplanar marker/chart seams restrict motion along their original shared edge too.
  for(auto a=incident.begin();a!=incident.end();++a)for(auto b=std::next(a);b!=incident.end();++b) {
    if(reference.Canonical(*a)==reference.Canonical(*b))continue;
    const auto& left=reference.Get(*a);const auto& right=reference.Get(*b);
    std::vector<Node> shared;
    for(auto n:left.v)for(auto m:right.v)if(n.id==m.id)shared.push_back(n);
    if(shared.size()!=2 || !OnEdge(shared[0].p,shared[1].p,origin,nullptr))continue;
    const auto normal=Cross(Delta(left.v[1].p,left.v[0].p),Delta(left.v[2].p,left.v[0].p));
    constrain(Cross(normal,Delta(shared[1].p,shared[0].p)));
  }
  if(rank==3){proposal.p=origin;return proposal;}
  auto displacement=Delta(proposal.p,origin);
  for(size_t i=0;i<rank;++i){const auto component=Dot(displacement,basis[i]);for(unsigned j=0;j<3;++j)displacement[j]-=component*basis[i][j];}
  proposal.p={double(origin.x+displacement[0]),double(origin.y+displacement[1]),double(origin.z+displacement[2])};
  return proposal;
}
PlanarReference::ClosestSurface PlanarReference::ClosestAnalytic(Id id,Point p) const {
  id=Canonical(id);
  if(!CompleteAnalytic(id))throw std::invalid_argument("Native3D closest smooth wall requires a complete certified component.");
  for(double x:{p.x,p.y,p.z})if(!std::isfinite(x))throw std::invalid_argument("Nonfinite native3D smooth-wall query.");
  const auto& g=analytic.at(id);auto q=Delta(p,g.origin);Vector n{},foot{};
  const auto& face=Get(id);const auto physical=Cross(Delta(face.v[1].p,face.v[0].p),Delta(face.v[2].p,face.v[0].p));
  if(g.kind==AnalyticSurface::Kind::Sphere) {
    const auto length=Norm(q);
    n=length>0?q:Delta(face.v[0].p,g.origin);const auto scale=Norm(n);
    for(size_t j=0;j<3;++j){n[j]/=scale;foot[j]=g.extent.x*n[j];}
    if(Dot(physical,Delta(face.v[0].p,g.origin))<0)for(auto& x:n)x=-x;
  } else if(g.kind==AnalyticSurface::Kind::Cylinder || g.kind==AnalyticSurface::Kind::Disk) {
    const AxisFrame frame(g);const auto& axis=frame.axis;const auto z=Dot(axis,q);
    auto radialProjection=[&](Vector p) {
      const auto u=Dot(p,frame.u),v=Dot(p,frame.v);Vector r{};
      for(size_t j=0;j<3;++j)r[j]=u*frame.u[j]+v*frame.v[j];return r;
    };
    // A radial basis avoids amplifying axial cancellation when a near-axis residual is normalized.
    const auto radial=radialProjection(q);const auto rho=Norm(radial);
    if(g.kind==AnalyticSurface::Kind::Cylinder) {
      if(rho>0)n=radial;
      else n=radialProjection(Delta(face.v[0].p,g.origin));
      const auto scale=Norm(n);
      if(!(scale>0 && std::isfinite(scale)))throw std::runtime_error("Unrepresentable native3D cylinder radial direction.");
      for(auto& x:n)x/=scale;
      const auto level=std::clamp(z,0.L,static_cast<long double>(g.extent.y));
      for(size_t j=0;j<3;++j)foot[j]=g.extent.x*n[j]+level*axis[j];
      const auto original=radialProjection(Delta(face.v[0].p,g.origin));
      if(Dot(physical,original)<0)for(auto& x:n)x=-x;
    } else {
      const auto scale=rho>g.extent.x?g.extent.x/rho:1.L;
      for(size_t j=0;j<3;++j)foot[j]=radial[j]*scale;
      n=axis;if(Dot(physical,axis)<0)for(auto& x:n)x=-x;
    }
  } else throw std::invalid_argument("Native3D closest graph-wall authority is not implemented.");
  ClosestSurface result;
  Vector separation{};for(size_t j=0;j<3;++j)separation[j]=q[j]-foot[j];
  result.distance=double(Norm(separation));
  result.point={double(g.origin.x+foot[0]),double(g.origin.y+foot[1]),double(g.origin.z+foot[2])};
  result.normal={double(n[0]),double(n[1]),double(n[2])};
  result.direction=result.normal;
  if(result.distance>0)result.direction={double(separation[0]/result.distance),double(separation[1]/result.distance),double(separation[2]/result.distance)};
  if(!std::isfinite(result.distance) || !Contains(id,result.point)) {
    std::ostringstream why;why<<std::setprecision(17)<<"Unrepresentable native3D smooth closest point: authority="<<id
        <<" query=("<<p.x<<','<<p.y<<','<<p.z<<") foot=("<<result.point.x<<','<<result.point.y<<','<<result.point.z<<')';
    throw std::runtime_error(why.str());
  }
  return result;
}
Point ScaleCylinder(const AnalyticSurface& g,Point p,double factor) {
  const auto a=UnitAxis(g),d=Delta(p,g.origin);const auto t=Dot(a,d);Vector q{};
  for(size_t i=0;i<3;++i)q[i]=a[i]*t+factor*(d[i]-a[i]*t);
  return {double(g.origin.x+q[0]),double(g.origin.y+q[1]),double(g.origin.z+q[2])};
}
double CylinderRadialFloor(const AnalyticSurface& g,const std::array<Point,3>& p) {
  const auto chart=CylinderChart(g,p);const AxisFrame frame(g);
  long double radius=INFINITY,lo=chart[0].x,hi=lo;
  for(size_t i=0;i<3;++i){const auto q=frame.Coordinates(g,p[i]);radius=std::min(radius,std::hypot(static_cast<long double>(q.x),q.y));lo=std::min(lo,static_cast<long double>(chart[i].x));hi=std::max(hi,static_cast<long double>(chart[i].x));}
  const auto lower=radius*std::cos((hi-lo)/(2*g.extent.x))-256*std::numeric_limits<double>::epsilon()*g.extent.x;
  if(!(lower>0 && std::isfinite(lower)))throw std::invalid_argument("Unusable native3D cylindrical source face.");
  return std::nextafter(double(lower),0.);
}
long double CylinderFacing(const AnalyticSurface& g,const std::array<Point,3>& p) {
  const auto sign=ParameterOrientation(g,p);if(!sign)return 0;
  const auto normal=Cross(Delta(p[1],p[0]),Delta(p[2],p[0]));const auto a=UnitAxis(g);
  for(auto point:p) {
    const auto d=Delta(point,g.origin);Vector radial{};const auto t=Dot(a,d);
    for(size_t i=0;i<3;++i)radial[i]=d[i]-a[i]*t;
    const auto value=Dot(normal,radial);
    const auto error=256*std::numeric_limits<double>::epsilon()*Norm(normal)*Norm(radial);
    if(sign>0 ? value<=error : value>=-error)throw std::invalid_argument("Folded native3D cylindrical face.");
  }
  return sign;
}
PlanarReference::PlanarReference(std::vector<Facet> input, size_t ceiling, std::map<Id, AnalyticSurface> curved,
                                 std::map<Id,SU2NativeGeometry::AffineAuthority> planes)
    : analytic(std::move(curved)),affine(std::move(planes)) {
  const size_t bytes = ConstructionBytes(input.size(), analytic.size(),affine.size());
  if (bytes == SIZE_MAX || bytes > ceiling) throw std::invalid_argument("Native 3D reference construction exceeds storage budget.");
  for (const auto& f : input) {
    std::set<Id> ids;
    for (auto n : f.v) {
      ids.insert(n.id);
      const auto old = nodes.emplace(n.id, n);
      if (!old.second &&
          (old.first->second.p.x != n.p.x || old.first->second.p.y != n.p.y || old.first->second.p.z != n.p.z))
        throw std::invalid_argument("Inconsistent native 3D reference node coordinates.");
    }
    bool area = false;
    for (size_t axis = 0; axis < 3; ++axis) area = Projected(f.v[0].p, f.v[1].p, f.v[2].p, axis, nullptr) != 0 || area;
    if (ids.size() != 3 || !area || !facets.emplace(f.id, f).second)
      throw std::invalid_argument("Native 3D reference has a duplicate or degenerate facet.");
  }
  for (const auto& row : analytic) {
    if(row.first==UINT64_MAX)throw std::invalid_argument("Native3D declared source component uses the reserved physical-source identity.");
    if (!facets.count(row.first)) throw std::invalid_argument("Analytic native3D chart has no original facet.");
    const auto& g=row.second;
    for (double x : {g.origin.x,g.origin.y,g.origin.z,g.extent.x,g.extent.y,g.extent.z,g.tolerance,g.axis.x,g.axis.y,g.axis.z})
      if (!std::isfinite(x)) throw std::invalid_argument("Nonfinite native3D analytic authority.");
    if (!(g.tolerance>0) || !(g.extent.x>0) ||
        (unsigned(g.kind)<1 || unsigned(g.kind)>4) ||
        ((g.kind==AnalyticSurface::Kind::Bump || g.kind==AnalyticSurface::Kind::Cylinder) && !(g.extent.y>0)))
      throw std::invalid_argument("Invalid native3D analytic authority parameters.");
    const auto& f=Get(row.first);
    if (g.kind==AnalyticSurface::Kind::Sphere) {
      if(g.axis.x!=0 || g.axis.y!=0 || g.axis.z!=1)throw std::invalid_argument("Sphere authority has no directional axis.");
      if (Orientation(g.origin,f.v[0].p,f.v[1].p,f.v[2].p)==0)
        throw std::invalid_argument("Native3D spherical chart crosses its radial horizon.");
      for (auto n : f.v)
        if (std::abs(Norm(Delta(n.p,g.origin))-g.extent.x)>g.tolerance)
          throw std::invalid_argument("Original native3D vertex leaves declared sphere.");
    } else if(g.kind==AnalyticSurface::Kind::Cylinder || g.kind==AnalyticSurface::Kind::Disk) {
      const AxisFrame frame(g);const std::array<Point,3> p{f.v[0].p,f.v[1].p,f.v[2].p};
      if(!ParameterOrientation(g,p))throw std::invalid_argument("Degenerate native3D axis chart.");
      if(g.kind==AnalyticSurface::Kind::Cylinder)CylinderFacing(g,p);
      for(auto n:f.v) {
        const auto q=frame.Coordinates(g,n.p);const auto r=std::hypot(static_cast<long double>(q.x),q.y);
        if(g.kind==AnalyticSurface::Kind::Cylinder ?
           (std::abs(r-g.extent.x)>g.tolerance || q.z<-g.tolerance || q.z>g.extent.y+g.tolerance) :
           (std::abs(q.z)>g.tolerance || r>g.extent.x+g.tolerance))
          throw std::invalid_argument("Original native3D vertex leaves its declared cylinder/disk.");
      }
    } else {
      if (g.axis.x!=0 || g.axis.y!=0 || g.axis.z!=1)throw std::invalid_argument("Bump authority uses Cartesian XY parameters.");
      if (Projected(f.v[0].p,f.v[1].p,f.v[2].p,2,nullptr)==0)
        throw std::invalid_argument("Degenerate native3D bump chart.");
      for (auto n : f.v)
        if (n.p.x<g.origin.x || static_cast<long double>(n.p.x)-g.origin.x>g.extent.x ||
            n.p.y<g.origin.y || static_cast<long double>(n.p.y)-g.origin.y>g.extent.y ||
            std::abs(static_cast<long double>(n.p.z)-BumpHeight(g,n.p))>g.tolerance)
          throw std::invalid_argument("Original native3D vertex leaves declared bump.");
    }
  }
  std::map<Edge, std::vector<Id>> owners;
  for (const auto& row : facets) {
    canonical.emplace(row.first, row.first);
    for (size_t i = 0; i < 3; ++i) {
      Edge edge{row.second.v[i].id, row.second.v[(i + 1) % 3].id};
      std::sort(edge.begin(), edge.end());
      owners[edge].push_back(row.first);
    }
  }
  std::set<Id> seen;
  for (const auto& root : facets) {
    if (seen.count(root.first) || analytic.count(root.first) || affine.count(root.first)) continue;
    const auto& reference = root.second;
    size_t axis = 0;
    while (Projected(reference.v[0].p, reference.v[1].p, reference.v[2].p, axis, nullptr) == 0) ++axis;
    const auto sign = Projected(reference.v[0].p, reference.v[1].p, reference.v[2].p, axis, nullptr);
    std::vector<Id> component{root.first};
    seen.insert(root.first);
    for (size_t j = 0; j < component.size(); ++j) {
      const auto& f = Get(component[j]);
      for (size_t i = 0; i < 3; ++i) {
        Edge edge{f.v[i].id, f.v[(i + 1) % 3].id};
        std::sort(edge.begin(), edge.end());
        const auto& adjacent = owners.at(edge);
        if (adjacent.size() != 2) continue;
        for (Id id : adjacent) {
          if (seen.count(id) || analytic.count(id) || affine.count(id)) continue;
          const auto& next = Get(id);
          if (next.marker != reference.marker) continue;
          bool coplanar = true;
          for (auto n : next.v)
            coplanar = coplanar && Orientation(reference.v[0].p, reference.v[1].p, reference.v[2].p, n.p) == 0;
          const auto area = Projected(next.v[0].p, next.v[1].p, next.v[2].p, axis, nullptr);
          if (coplanar && ((sign > 0 && area > 0) || (sign < 0 && area < 0))) {
            component.push_back(id);
            seen.insert(id);
          }
        }
      }
    }
    if (component.size() == 1) continue;
    std::vector<Face> triangles;
    std::map<Edge, std::vector<Edge>> edges;
    std::set<Id> vertices;
    for (Id id : component) {
      const auto& f = Get(id);
      triangles.push_back({f.v[0].id, f.v[1].id, f.v[2].id});
      for (size_t i = 0; i < 3; ++i) {
        vertices.insert(f.v[i].id);
        const Edge oriented{f.v[i].id, f.v[(i + 1) % 3].id};
        auto key = oriented;
        std::sort(key.begin(), key.end());
        edges[key].push_back(oriented);
      }
    }
    bool valid = true;
    try {
      ValidateSurfaceTopology(triangles, false, SIZE_MAX,nullptr,true);
    } catch (const std::invalid_argument&) {
      valid = false;
    }
    std::set<std::array<double, 3>> coordinates;
    for (Id id : vertices) {
      const auto p = nodes.at(id).p;
      valid = coordinates.insert({p.x, p.y, p.z}).second && valid;
    }
    std::vector<Edge> border;
    for (const auto& entry : edges) {
      const auto& uses = entry.second;
      if (uses.size() == 1)
        border.push_back(uses[0]);
      else if (uses.size() != 2 || uses[0][0] != uses[1][1] || uses[0][1] != uses[1][0])
        valid = false;
    }
    if (valid) border = StraightBorders(border, nodes);
    bool convex=true;
    for (auto edge : border)
      for (Id id : vertices) {
        const auto side = Projected(nodes.at(edge[0]).p, nodes.at(edge[1]).p, nodes.at(id).p, axis, nullptr);
        if ((sign > 0 && side < 0) || (sign < 0 && side > 0)) convex=false;
      }
    if(!convex && valid)for(size_t i=0;i<border.size();++i)for(size_t j=0;j<i;++j) {
      const auto a=border[i],b=border[j];
      const auto p=PlaneCoordinates(nodes.at(a[0]).p,axis),q=PlaneCoordinates(nodes.at(a[1]).p,axis);
      const auto r=PlaneCoordinates(nodes.at(b[0]).p,axis),t=PlaneCoordinates(nodes.at(b[1]).p,axis);
      auto opposite=[](long double x,long double y){return (x<0 && y>0)||(x>0 && y<0);};
      if(opposite(Projected(p,q,r,2,nullptr),Projected(p,q,t,2,nullptr)) &&
         opposite(Projected(r,t,p,2,nullptr),Projected(r,t,q,2,nullptr)))valid=false;
      for(unsigned k=0;k<2;++k) {
        if(a[k]!=b[0] && a[k]!=b[1] && OnEdge(r,t,k?q:p,nullptr))valid=false;
        if(b[k]!=a[0] && b[k]!=a[1] && OnEdge(p,q,k?t:r,nullptr))valid=false;
      }
    }
    if (!valid) continue;
    if(!convex)nonconvex.insert(root.first);
    boundaries.emplace(root.first, std::move(border));
    for (Id id : component) canonical.at(id) = root.first;
  }
  // Definitions are geometry-owned; marker cuts retain physical labels without assuming one body/component.
  std::map<Id,SU2NativeGeometry::AffineAuthority> definitions;
  std::map<std::pair<Id,int>,std::vector<Facet>> plane_groups;
  auto key=[](const SU2NativeGeometry::AffineAuthority& g){return std::make_tuple(g.version,g.dimension,g.origin.x,g.origin.y,
      g.origin.z,g.normal.x,g.normal.y,g.normal.z,g.maximum_error);};
  for(const auto& row:affine) {
    if(row.first==UINT64_MAX)throw std::invalid_argument("Native3D declared source component uses the reserved physical-source identity.");
    if(!facets.count(row.first)||analytic.count(row.first))throw std::invalid_argument("Conflicting/missing affine facet authority.");
    if(row.second.dimension!=3)throw std::invalid_argument("A volume reference needs a 3D plane authority.");
    SU2NativeGeometry::AffineChart audit(row.second);
    const auto old=definitions.emplace(row.second.identity,row.second);
    if(!old.second && key(old.first->second)!=key(row.second))throw std::invalid_argument("Conflicting definitions of one affine authority identity.");
    const auto& f=Get(row.first);plane_groups[{row.second.identity,f.marker}].push_back(f);
  }
  for(const auto& group:plane_groups) {
    const auto& definition=definitions.at(group.first.first);
    auto chart=std::make_shared<SU2NativeGeometry::FinitePlaneReference>(definition,group.second,bytes);
    const auto slot=affine_domains.size();affine_domains.push_back(chart);
    for(const auto& f:group.second){const auto id=chart->Component(f.id);canonical.at(f.id)=id;affine_charts.emplace(id,slot);}
  }
  // Reuse the certified connected-convex planar union in graph parameter space; markers and definitions stay distinct.
  using BumpKey=std::tuple<int,double,double,double,double,double,double,double>;
  std::map<BumpKey,std::vector<Facet>> groups;
  for (const auto& row : analytic) if (row.second.kind==AnalyticSurface::Kind::Bump) {
    const auto& g=row.second;
    auto f=Get(row.first);
    for(auto& n:f.v)n.p.z=0;
    groups[{f.marker,g.origin.x,g.origin.y,g.origin.z,g.extent.x,g.extent.y,g.extent.z,g.tolerance}].push_back(f);
  }
  for(const auto& group:groups) {
    auto chart=std::make_shared<PlanarReference>(group.second,ConstructionBytes(group.second.size()));
    for(const auto& f:group.second) {
      const auto id=chart->Canonical(f.id);
      canonical.at(f.id)=id;
      bump_charts.emplace(id,chart);
    }
  }
  using CurveKey=std::tuple<int,unsigned,double,double,double,double,double,double,double,double,double,double>;
  auto definition=[&](Id id) {const auto& g=analytic.at(id);return CurveKey{Get(id).marker,unsigned(g.kind),g.origin.x,g.origin.y,g.origin.z,g.extent.x,g.extent.y,g.extent.z,g.tolerance,g.axis.x,g.axis.y,g.axis.z};};
  std::set<Id> curved_seen;
  for(const auto& row:analytic) {
    const Id root=row.first;const auto& g=row.second;
    if(g.kind==AnalyticSurface::Kind::Bump || curved_seen.count(root))continue;
    std::vector<Id> component{root};curved_seen.insert(root);
    for(size_t j=0;j<component.size();++j) {
      const auto& f=Get(component[j]);
      for(size_t i=0;i<3;++i)for(Id id:owners.at(Edge{std::min(f.v[i].id,f.v[(i+1)%3].id),std::max(f.v[i].id,f.v[(i+1)%3].id)}))
        if(!curved_seen.count(id) && analytic.count(id) && definition(id)==definition(root)) {component.push_back(id);curved_seen.insert(id);}
    }
    std::vector<Face> triangles;std::map<Edge,std::vector<Edge>> uses;std::set<Id> vertices;
    bool valid=true;int direction=0;
    for(Id id:component) {
      const auto& f=Get(id);triangles.push_back({f.v[0].id,f.v[1].id,f.v[2].id});
      const auto oriented=g.kind==AnalyticSurface::Kind::Sphere ? Orientation(g.origin,f.v[0].p,f.v[1].p,f.v[2].p) : ParameterOrientation(g,{f.v[0].p,f.v[1].p,f.v[2].p});
      const int sign=oriented>0?1:-1;if(!direction)direction=sign;valid=valid && sign==direction;
      for(size_t i=0;i<3;++i){vertices.insert(f.v[i].id);Edge edge{f.v[i].id,f.v[(i+1)%3].id};auto key=edge;std::sort(key.begin(),key.end());uses[key].push_back(edge);}
    }
    std::vector<Edge> border;
    for(const auto& edge:uses) {
      if(edge.second.size()==1)border.push_back(edge.second[0]);
      else if(edge.second.size()!=2 || edge.second[0][0]!=edge.second[1][1] || edge.second[0][1]!=edge.second[1][0])valid=false;
    }
    try {
      if(g.kind==AnalyticSurface::Kind::Sphere){if(!border.empty())valid=false;else ValidateSurfaceTopology(triangles,true,SIZE_MAX);}
      else if(g.kind==AnalyticSurface::Kind::Disk)ValidateSurfaceTopology(triangles,false,SIZE_MAX);
      else if(vertices.size()+component.size()!=uses.size())valid=false; // Cylinder annulus Euler characteristic.
    }catch(const std::invalid_argument&){valid=false;}
    if(g.kind==AnalyticSurface::Kind::Cylinder && valid) {
      std::set<Id> border_nodes;for(auto edge:border){border_nodes.insert(edge[0]);border_nodes.insert(edge[1]);}
      for(Id vertex:vertices) {
        std::map<Id,std::set<Id>> link;
        for(auto face:triangles)for(size_t i=0;i<3;++i)if(face[i]==vertex){link[face[(i+1)%3]].insert(face[(i+2)%3]);link[face[(i+2)%3]].insert(face[(i+1)%3]);}
        std::set<Id> visited;std::vector<Id> pending{link.begin()->first};unsigned ends=0;
        for(size_t i=0;i<pending.size();++i)if(visited.insert(pending[i]).second)for(Id next:link.at(pending[i]))pending.push_back(next);
        for(const auto& neighbors:link){if(neighbors.second.size()==1)++ends;else if(neighbors.second.size()!=2)valid=false;}
        valid=valid && visited.size()==link.size() && ends==(border_nodes.count(vertex)?2u:0u);
      }
    }
    if(g.kind!=AnalyticSurface::Kind::Sphere && valid) {
      const AxisFrame frame(g);std::map<Id,Id> next;std::set<Id> visited;unsigned loops=0;
      for(auto edge:border)if(!next.emplace(edge[0],edge[1]).second)valid=false;
      std::array<bool,2> ends{};
      for(auto edge:border)if(!visited.count(edge[0])) {
        ++loops;Id at=edge[0];long double winding=0;int end=-1;
        do {
          if(!visited.insert(at).second || !next.count(at)){valid=false;break;}
          const auto p=frame.Coordinates(g,nodes.at(at).p),q=frame.Coordinates(g,nodes.at(next.at(at)).p);
          const auto radius=std::hypot(static_cast<long double>(p.x),p.y);
          valid=valid && std::abs(radius-g.extent.x)<=g.tolerance;
          if(g.kind==AnalyticSurface::Kind::Cylinder) {
            const int level=std::abs(p.z)<=g.tolerance?0:std::abs(p.z-g.extent.y)<=g.tolerance?1:-1;
            if(level<0 || (end>=0 && end!=level))valid=false;else end=level;
          }
          const auto step=std::atan2(static_cast<long double>(p.x)*q.y-static_cast<long double>(p.y)*q.x,static_cast<long double>(p.x)*q.x+static_cast<long double>(p.y)*q.y);
          if(std::abs(step)>=std::acos(-1.L))valid=false;winding+=step;at=next.at(at);
        }while(at!=edge[0] && valid);
        if(std::abs(std::abs(winding)-2*std::acos(-1.L))>1e-10L)valid=false;
        if(end>=0){if(ends[end])valid=false;ends[end]=true;}
      }
      valid=valid && visited.size()==border.size() && loops==(g.kind==AnalyticSurface::Kind::Disk?1u:2u);
      if(g.kind==AnalyticSurface::Kind::Cylinder)valid=valid && ends[0] && ends[1];
    }
    if(valid){complete_analytic.insert(root);for(Id id:component)canonical.at(id)=root;}
    else if(g.kind==AnalyticSurface::Kind::Disk)throw std::invalid_argument("Disk authority requires its complete connected circular cap.");
  }
  for(const auto& row:boundaries)if(nonconvex.count(row.first)) {
    std::map<Id,Id> previous,next;
    for(auto edge:row.second){previous.emplace(edge[1],edge[0]);next.emplace(edge[0],edge[1]);}
    auto& corners=boundary_corners[row.first];corners.reserve(next.size());
    for(const auto& entry:next)corners.push_back({previous.at(entry.first),entry.first,entry.second});
  }
  for(const auto& row:analytic)declared_components.push_back(Canonical(row.first));
  for(const auto& row:affine)declared_components.push_back(Canonical(row.first));
  std::sort(declared_components.begin(),declared_components.end());
  declared_components.erase(std::unique(declared_components.begin(),declared_components.end()),declared_components.end());
  std::map<Id, std::array<Point, 2>> boxes;
  for (const auto& row : facets) {
    const auto id = Canonical(row.first);
    auto& box = boxes.emplace(id, std::array<Point, 2>{row.second.v[0].p, row.second.v[0].p}).first->second;
    for (auto n : row.second.v) {
      box[0] = {std::min(box[0].x, n.p.x), std::min(box[0].y, n.p.y), std::min(box[0].z, n.p.z)};
      box[1] = {std::max(box[1].x, n.p.x), std::max(box[1].y, n.p.y), std::max(box[1].z, n.p.z)};
    }
  }
  for (const auto& row : analytic) {
    if (Canonical(row.first)!=row.first) continue;
    const auto& g=row.second;
    auto& box=boxes.at(row.first);
    if (g.kind==AnalyticSurface::Kind::Sphere) {
      // Conservative for the entire tolerated radial cone; tighter spherical boxes are a later optimization.
      box[0]={g.origin.x-g.extent.x,g.origin.y-g.extent.x,g.origin.z-g.extent.x};
      box[1]={g.origin.x+g.extent.x,g.origin.y+g.extent.x,g.origin.z+g.extent.x};
    } else if(g.kind==AnalyticSurface::Kind::Cylinder || g.kind==AnalyticSurface::Kind::Disk) {
      const auto a=UnitAxis(g);const long double height=g.kind==AnalyticSurface::Kind::Cylinder?g.extent.y:0;
      Point end{double(g.origin.x+a[0]*height),double(g.origin.y+a[1]*height),double(g.origin.z+a[2]*height)};
      box[0]={std::min(g.origin.x,end.x)-g.extent.x,std::min(g.origin.y,end.y)-g.extent.x,std::min(g.origin.z,end.z)-g.extent.x};
      box[1]={std::max(g.origin.x,end.x)+g.extent.x,std::max(g.origin.y,end.y)+g.extent.x,std::max(g.origin.z,end.z)+g.extent.x};
    } else {
      box[0].z=double(g.origin.z+std::min(0.,g.extent.z)/16);
      box[1].z=double(g.origin.z+std::max(0.,g.extent.z)/16);
    }
    box[0]={std::nextafter(box[0].x-g.tolerance,-INFINITY),std::nextafter(box[0].y-g.tolerance,-INFINITY),
            std::nextafter(box[0].z-g.tolerance,-INFINITY)};
    box[1]={std::nextafter(box[1].x+g.tolerance,INFINITY),std::nextafter(box[1].y+g.tolerance,INFINITY),
            std::nextafter(box[1].z+g.tolerance,INFINITY)};
    for (auto p : box)
      if (!(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)))
        throw std::invalid_argument("Unrepresentable native3D analytic chart bounds.");
  }
  for(const auto& row:affine_charts) {
    auto& box=boxes.at(row.first);const auto error=2*affine_domains.at(row.second)->Chart().Authority().maximum_error;
    box[0]={std::nextafter(box[0].x-error,-INFINITY),std::nextafter(box[0].y-error,-INFINITY),std::nextafter(box[0].z-error,-INFINITY)};
    box[1]={std::nextafter(box[1].x+error,INFINITY),std::nextafter(box[1].y+error,INFINITY),std::nextafter(box[1].z+error,INFINITY)};
    for(auto p:box)if(!(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)))
      throw std::invalid_argument("Unrepresentable affine broad-phase bounds.");
  }
  bool multiple = false;
  for (const auto& row : boxes) {
    auto& ids = marker_facets[Get(row.first).marker];
    ids.push_back(row.first);
    multiple = multiple || ids.size() > 1;
    indexed.push_back(row.first);
  }
  if constexpr (std::is_same<su2double, double>::value) {
    if (multiple) {
      std::vector<su2double> coordinates;
      std::vector<unsigned long> connectivity, ids;
      std::vector<unsigned short> types(indexed.size(), LINE), markers(indexed.size(), 0);
      coordinates.reserve(6 * indexed.size());
      connectivity.reserve(2 * indexed.size());
      ids.reserve(indexed.size());
      for (Id id : indexed) {
        for (auto p : boxes.at(id)) {
          coordinates.push_back(p.x);coordinates.push_back(p.y);coordinates.push_back(p.z);
        }
        connectivity.push_back(connectivity.size());connectivity.push_back(connectivity.size());
        ids.push_back(ids.size());
      }
      search = std::make_shared<CADTElemClass>(3, coordinates, connectivity, types, markers, ids, false);
    }
  }
}
size_t PlanarReference::ConstructionBytes(size_t count, size_t curves,size_t planes) {
  if (count > std::numeric_limits<unsigned long>::max() / 2) return SIZE_MAX;
  size_t retained = 0, peak = 0;
  if (count) CADTElemClass::PredictBytes(3, 2 * count, count, 2 * count, &retained, &peak);
  // shortcut: conservative map/component scratch; calibrate allocator requests before larger reference campaigns.
  return transfer_memory::Add(transfer_memory::Mul(count, 4096), curves ? ConstructionBytes(curves) : 0,
                             planes ? transfer_memory::Add(ConstructionBytes(planes),transfer_memory::Mul(planes,12288)) : 0,
                             transfer_memory::Mul(curves, 2*(sizeof(std::pair<const Id,AnalyticSurface>)+4*sizeof(void*))), retained, peak,
                             transfer_memory::Mul(count, 6 * sizeof(su2double) + 3 * sizeof(unsigned long) + 4 * sizeof(unsigned short)));
}
size_t PlanarReference::ResidentBytes() const {
  auto map = [](const auto& m) {
    return transfer_memory::Mul(m.size(), sizeof(typename std::decay_t<decltype(m)>::value_type) + 4 * sizeof(void*));
  };
  size_t result = transfer_memory::Add(sizeof(*this), map(facets), map(canonical), map(boundaries), map(nodes), map(analytic), map(bump_charts), map(complete_analytic), map(nonconvex), map(boundary_corners),map(affine),map(affine_charts),
      transfer_memory::Bytes(affine_domains),transfer_memory::Bytes(declared_components));
  for(const auto& domain:affine_domains)result=transfer_memory::Add(result,domain->ResidentBytes());
  for(const auto& row:boundary_corners)result=transfer_memory::Add(result,transfer_memory::Bytes(row.second));
  for (const auto& entry : boundaries) result = transfer_memory::Add(result, transfer_memory::Bytes(entry.second));
  for(const auto& entry:bump_charts)
    if(entry.first==entry.second->facets.begin()->first) result=transfer_memory::Add(result,entry.second->ResidentBytes());
  result = transfer_memory::Add(result, map(marker_facets), transfer_memory::Bytes(indexed),
                                search ? search->GetAllocatedBytes() : 0);
  for (const auto& entry : marker_facets) result = transfer_memory::Add(result, transfer_memory::Bytes(entry.second));
  return result;
}
size_t PlanarReference::SearchWorkspaceBytes() const {
  return search ? transfer_memory::Add(transfer_memory::Mul(indexed.size(), 2 * sizeof(unsigned long)),
                                       search->RetainedWorkspaceBound(1)) : 0;
}
bool PlanarReference::FindFace(int marker, const std::array<Point, 3>& triangle, Id& facet, KernelStats* stats) const {
  for (auto p : triangle)
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))
      throw std::invalid_argument("Nonfinite native 3D physical binding query.");
  const auto group = marker_facets.find(marker);
  if (group == marker_facets.end()) return false;
  if (group->second.size() == 1 || !search) {
    for (Id id : group->second)
      if (ContainsFace(id, triangle, stats)) {facet = id;return true;}
    return false;
  }
  Point lo = triangle[0], hi = triangle[0];
  for (auto p : triangle) {
    lo = {std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z)};
    hi = {std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z)};
  }
  const std::array<su2double, 3> lower{lo.x, lo.y, lo.z}, upper{hi.x, hi.y, hi.z};
  std::vector<unsigned long> candidates;
  search->DetermineIntersectingElements(lower.data(), upper.data(), candidates);
  bool found = false;
  Id chosen = 0;
  for (auto index : candidates) {
    const auto id = indexed.at(index);
    if (Get(id).marker == marker && ContainsFace(id, triangle, stats) && (!found || id < chosen)) {
      chosen = id;found = true;
    }
  }
  if (found) facet = chosen;
  return found;
}
Id PlanarReference::Canonical(Id id) const { return canonical.at(id); }
const Facet& PlanarReference::Get(Id id) const { return facets.at(id); }
bool PlanarReference::Contains(Id id, Point p, KernelStats* stats) const {
  id = Canonical(id);
  const auto& v = Get(id).v;
  if(affine.count(id))return affine_domains.at(affine_charts.at(id))->Contains(id,p,stats);
  const auto curved=analytic.find(id);
  if (curved!=analytic.end()) {
    const auto& g=curved->second;
    if (!(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)))
      throw std::invalid_argument("Nonfinite native3D analytic query.");
    if (g.kind==AnalyticSurface::Kind::Sphere) {
      if (std::abs(Norm(Delta(p,g.origin))-g.extent.x)>g.tolerance) return false;
      if(complete_analytic.count(id))return true;
      const auto area=Orientation(g.origin,v[0].p,v[1].p,v[2].p,stats);
      bool inside=true;
      for (size_t i=0;i<3;++i) {
        const auto side=Orientation(g.origin,v[i].p,v[(i+1)%3].p,p,stats);
        inside=inside && (area>0 ? side>=0 : side<=0);
      }
      if (!inside) {
        bool near=false;
        for (size_t i=0;i<3;++i) near=near || ArcDistance(g,v[i].p,v[(i+1)%3].p,p)<=g.tolerance;
        if (!near) return false;
      }
    } else if(g.kind==AnalyticSurface::Kind::Cylinder || g.kind==AnalyticSurface::Kind::Disk) {
      const auto q=AxisFrame(g).Coordinates(g,p);const auto radius=std::hypot(static_cast<long double>(q.x),q.y);
      if(g.kind==AnalyticSurface::Kind::Disk)return std::abs(q.z)<=g.tolerance && radius<=g.extent.x+g.tolerance;
      if(std::abs(radius-g.extent.x)>g.tolerance || q.z<-g.tolerance || q.z>g.extent.y+g.tolerance)return false;
      if(complete_analytic.count(id))return true;
      const auto chart=CylinderChart(g,{v[0].p,v[1].p,v[2].p});
      const auto first=AxisFrame(g).Coordinates(g,v[0].p);
      const Point query{double(g.extent.x*std::remainder(std::atan2(static_cast<long double>(q.y),q.x)-std::atan2(static_cast<long double>(first.y),first.x),2*std::acos(-1.L))),q.z,0};
      const auto area=Orientation(chart[0],chart[1],chart[2],{0,0,1},stats);
      for(size_t i=0;i<3;++i) {
        const auto side=Orientation(chart[i],chart[(i+1)%3],query,{0,0,1},stats);
        if(area>0 ? side<0 : side>0) {
          const auto a=chart[i],b=chart[(i+1)%3];const auto dx=static_cast<long double>(b.x)-a.x,dy=static_cast<long double>(b.y)-a.y;
          const auto t=std::clamp(((query.x-a.x)*dx+(query.y-a.y)*dy)/(dx*dx+dy*dy),0.L,1.L);
          if(std::hypot(static_cast<long double>(query.x)-a.x-t*dx,static_cast<long double>(query.y)-a.y-t*dy)>g.tolerance)return false;
        }
      }
    } else {
      if (std::abs(static_cast<long double>(p.z)-BumpHeight(g,p))>g.tolerance) return false;
      const auto& chart=*bump_charts.at(id);
      if (!chart.Contains(id,{p.x,p.y,0},stats)) {
        bool near=false;
        auto edge_near=[&](Point a,Point b) {
          const long double x=static_cast<long double>(b.x)-a.x,y=static_cast<long double>(b.y)-a.y;
          const auto t=std::clamp(((static_cast<long double>(p.x)-a.x)*x+(static_cast<long double>(p.y)-a.y)*y)/(x*x+y*y),0.L,1.L);
          const long double qx=a.x+t*x,qy=a.y+t*y;
          return Norm({static_cast<long double>(p.x)-qx,static_cast<long double>(p.y)-qy,
                       static_cast<long double>(p.z)-BumpHeight(g,qx,qy)})<=g.tolerance;
        };
        const auto border=chart.boundaries.find(id);
        if(border==chart.boundaries.end()) {
          for(size_t i=0;i<3;++i)near=near || edge_near(v[i].p,v[(i+1)%3].p);
        } else {
          for(auto edge:border->second)near=near || edge_near(chart.nodes.at(edge[0]).p,chart.nodes.at(edge[1]).p);
        }
        if(!near)return false;
      }
    }
    return true;
  }
  if (Orientation(v[0].p, v[1].p, v[2].p, p, stats) != 0) return false;
  for (size_t axis = 0; axis < 3; ++axis) {
    const auto area = Projected(v[0].p, v[1].p, v[2].p, axis, stats);
    if (area == 0) continue;
    const auto group = boundaries.find(id);
    if (group == boundaries.end()) {
      for (size_t i = 0; i < 3; ++i) {
        const auto side = Projected(v[i].p, v[(i + 1) % 3].p, p, axis, stats);
        if ((area > 0 && side < 0) || (area < 0 && side > 0)) return false;
      }
    } else if(nonconvex.count(id)) {
      const auto q=PlaneCoordinates(p,axis);int winding=0;
      for(auto edge:group->second) {
        const auto a=PlaneCoordinates(nodes.at(edge[0]).p,axis),b=PlaneCoordinates(nodes.at(edge[1]).p,axis);
        if(OnEdge(a,b,q,stats))return true;
        const auto side=Projected(a,b,q,2,stats);
        if(a.y<=q.y && b.y>q.y && side>0)++winding;
        if(a.y>q.y && b.y<=q.y && side<0)--winding;
      }
      return winding==(area>0?1:-1);
    } else
      for (auto edge : group->second) {
        const auto side = Projected(nodes.at(edge[0]).p, nodes.at(edge[1]).p, p, axis, stats);
        if ((area > 0 && side < 0) || (area < 0 && side > 0)) return false;
      }
    return true;
  }
  throw std::logic_error("Degenerate admitted native 3D reference facet.");
}
bool PlanarReference::ContainsSegment(Id id,Point a,Point b,KernelStats* stats)const {
  id=Canonical(id);
  if(affine.count(id))return affine_domains.at(affine_charts.at(id))->ContainsSegment(id,a,b,stats);
  if(analytic.count(id))throw std::invalid_argument("Analytic segment containment requires its finite parameter chart.");
  if(!Contains(id,a,stats)||!Contains(id,b,stats))return false;
  if(!nonconvex.count(id))return true;
  const auto& f=Get(id);size_t axis=0;
  while(Projected(f.v[0].p,f.v[1].p,f.v[2].p,axis,stats)==0)++axis;
  const int sign=Projected(f.v[0].p,f.v[1].p,f.v[2].p,axis,stats)>0?1:-1;
  a=PlaneCoordinates(a,axis);b=PlaneCoordinates(b,axis);
  auto side=[&](Point x,Point y,Point p){return Projected(x,y,p,2,stats);};
  auto opposite=[](long double x,long double y){return (x<0&&y>0)||(x>0&&y<0);};
  for(auto edge:boundaries.at(id)) {
    const auto p=PlaneCoordinates(nodes.at(edge[0]).p,axis),q=PlaneCoordinates(nodes.at(edge[1]).p,axis);
    if(opposite(side(a,b,p),side(a,b,q))&&opposite(side(p,q,a),side(p,q,b)))return false;
    auto same=[](Point x,Point y){return x.x==y.x && x.y==y.y;};
    if(!same(a,p)&&!same(a,q)&&OnEdge(p,q,a,stats)&&sign*side(p,q,b)<0)return false;
    if(!same(b,p)&&!same(b,q)&&OnEdge(p,q,b,stats)&&sign*side(p,q,a)<0)return false;
  }
  // At a touched corner, test both chord directions against the complete local fluid wedge.
  // This includes boundary-only endpoints and tangencies, which a proper-crossing test misses.
  for(auto corner:boundary_corners.at(id)) {
    const auto p=PlaneCoordinates(nodes.at(corner[0]).p,axis),v=PlaneCoordinates(nodes.at(corner[1]).p,axis),
        q=PlaneCoordinates(nodes.at(corner[2]).p,axis);
    if(!OnEdge(a,b,v,stats))continue;
    const auto turn=sign*side(p,v,q);
    for(auto direction:{a,b}) {
      if(direction.x==v.x && direction.y==v.y)continue;
      const auto left=sign*side(p,v,direction),right=sign*side(v,q,direction);
      if(turn>=0 ? left<0||right<0 : left<0&&right<0)return false;
    }
  }
  return true;
}
bool PlanarReference::ContainsFace(Id id, const std::array<Point, 3>& triangle, KernelStats* stats) const {
  id=Canonical(id);
  if(affine.count(id))return affine_domains.at(affine_charts.at(id))->ContainsFace(id,triangle,stats);
  for (auto p : triangle)
    if (!Contains(id, p, stats)) return false;
  const auto& f = Get(Canonical(id));
  const auto curved=analytic.find(Canonical(id));
  if (curved!=analytic.end()) {
    const auto& g=curved->second;
    if(g.kind==AnalyticSurface::Kind::Cylinder || g.kind==AnalyticSurface::Kind::Disk) {
      const auto old=ParameterOrientation(g,{f.v[0].p,f.v[1].p,f.v[2].p});
      const auto fresh=g.kind==AnalyticSurface::Kind::Cylinder ? CylinderFacing(g,triangle) : ParameterOrientation(g,triangle);
      return old>0 ? fresh>0 : fresh<0;
    }
    if(g.kind==AnalyticSurface::Kind::Bump && bump_charts.at(Canonical(id))->nonconvex.count(Canonical(id))) {
      std::array<Point,3> flat=triangle;for(auto& p:flat)p.z=0;
      if(!bump_charts.at(Canonical(id))->ContainsFace(Canonical(id),flat,stats))return false;
    }
    const auto old=g.kind==AnalyticSurface::Kind::Sphere ?
        Orientation(g.origin,f.v[0].p,f.v[1].p,f.v[2].p,stats) : Projected(f.v[0].p,f.v[1].p,f.v[2].p,2,stats);
    const auto fresh=g.kind==AnalyticSurface::Kind::Sphere ?
        Orientation(g.origin,triangle[0],triangle[1],triangle[2],stats) : Projected(triangle[0],triangle[1],triangle[2],2,stats);
    // Convex radial/XY footprints plus consistent orientation admit complete faces, not just their vertices.
    return (old>0 && fresh>0) || (old<0 && fresh<0);
  }
  for (size_t axis = 0; axis < 3; ++axis) {
    const auto old = Projected(f.v[0].p, f.v[1].p, f.v[2].p, axis, stats);
    if (old != 0) {
      const auto fresh = Projected(triangle[0], triangle[1], triangle[2], axis, stats);
      if(!((old>0 && fresh>0)||(old<0 && fresh<0)))return false;
      if(nonconvex.count(Canonical(id))) {
        std::array<Point,3> t;for(size_t i=0;i<3;++i)t[i]=PlaneCoordinates(triangle[i],axis);
        int winding=0;
        for(auto edge:boundaries.at(Canonical(id))) {
          const auto a=PlaneCoordinates(nodes.at(edge[0]).p,axis),b=PlaneCoordinates(nodes.at(edge[1]).p,axis);
          const int ay=OrientationCentroidSign({0,a.y,0},{1,a.y,0},t,{0,0,1},stats);
          const int by=OrientationCentroidSign({0,b.y,0},{1,b.y,0},t,{0,0,1},stats);
          const int side=OrientationCentroidSign(a,b,t,{0,0,1},stats);
          if(ay>=0 && by<0 && side>0)++winding;
          if(ay<0 && by>=0 && side<0)--winding;
          if(BorderCrossesTriangleInterior(a,b,t,stats))return false;
        }
        if(winding!=(old>0?1:-1))return false;
      }
      return true;
    }
  }
  return false;
}
double PlanarReference::SurfaceError(Id id,const std::array<Point,3>& triangle,KernelStats* stats) const {
  id=Canonical(id);
  if(!ContainsFace(id,triangle,stats))throw std::invalid_argument("Native3D surface error leaves its authority.");
  if(affine.count(id)) {
    const auto& chart=affine_domains.at(affine_charts.at(id))->Chart();double error=0;
    for(auto p:triangle)error=std::max(error,chart.ErrorBound(p));return error;
  }
  const auto found=analytic.find(id);if(found==analytic.end())return 0;
  const auto& g=found->second;
  long double error=0,scale=std::max({std::abs(g.origin.x),std::abs(g.origin.y),std::abs(g.origin.z),
                                   std::abs(g.extent.x),std::abs(g.extent.y),std::abs(g.extent.z)});
  if(g.kind==AnalyticSurface::Kind::Disk)return g.tolerance;
  if(g.kind==AnalyticSurface::Kind::Cylinder) {
    error=std::max(0.L,static_cast<long double>(g.extent.x)-CylinderRadialFloor(g,triangle));
  } else if(g.kind==AnalyticSurface::Kind::Sphere) {
    const auto a=Delta(triangle[1],triangle[0]),b=Delta(triangle[2],triangle[0]),c=Delta(triangle[0],g.origin);
    const auto normal=Cross(a,b);const auto length=Norm(normal);
    if(!(length>0 && std::isfinite(length)))throw std::invalid_argument("Degenerate native3D geometry triangle.");
    const auto uncertainty=64*std::numeric_limits<long double>::epsilon()*Norm(a)*Norm(b);
    const auto numerator=std::max(0.L,std::abs(Dot(normal,c))-uncertainty*Norm(c));
    const auto height=numerator/(length+uncertainty);
    long double radius=height;
    bool outside=false;
    for(size_t i=0;i<3;++i) {
      const auto edge=Delta(triangle[(i+1)%3],triangle[i]),vertex=Delta(g.origin,triangle[i]);
      const auto side=Dot(Cross(edge,vertex),normal);
      const auto envelope=128*std::numeric_limits<long double>::epsilon()*Norm(edge)*Norm(vertex)*length;
      outside=outside || side < -envelope;
    }
    if(outside) {
      // An exterior plane foot overstates the defect and rewards endless splits along the wrong edge.
      radius=INFINITY;
      for(size_t i=0;i<3;++i) {
        const auto v=Delta(triangle[i],g.origin),e=Delta(triangle[(i+1)%3],triangle[i]);
        const auto square=Dot(e,e);
        if(!(square>0))throw std::invalid_argument("Degenerate native3D geometry edge.");
        const auto t=std::clamp(-Dot(v,e)/square,0.L,1.L);
        Vector closest{};for(size_t k=0;k<3;++k)closest[k]=v[k]+t*e[k];
        const auto envelope=1024*std::numeric_limits<long double>::epsilon()*(Norm(v)+Norm(e));
        radius=std::min(radius,std::max(0.L,Norm(closest)-envelope));
      }
    }
    error=std::max(0.L,static_cast<long double>(g.extent.x)-radius);
    for(auto p:triangle)error=std::max(error,std::abs(Norm(Delta(p,g.origin))-g.extent.x));
  } else {
    long double diameter=0;
    for(size_t i=0;i<3;++i) {
      error=std::max(error,std::abs(static_cast<long double>(triangle[i].z)-BumpHeight(g,triangle[i])));
      for(size_t j=i+1;j<3;++j) {
        const long double dx=static_cast<long double>(triangle[i].x)-triangle[j].x;
        const long double dy=static_cast<long double>(triangle[i].y)-triangle[j].y;
        diameter=std::max(diameter,dx*dx+dy*dy);
      }
    }
    const long double x=g.extent.x,y=g.extent.y,a=std::abs(g.extent.z);
    std::array<long double,3> u{},v{};
    for(size_t i=0;i<3;++i){u[i]=(static_cast<long double>(triangle[i].x)-g.origin.x)/x;v[i]=(static_cast<long double>(triangle[i].y)-g.origin.y)/y;}
    auto quadratic=[](const std::array<long double,3>& q) {
      const auto ends=std::minmax_element(q.begin(),q.end());
      long double maximum=0;for(auto value:q)maximum=std::max(maximum,std::abs(value*(1-value)));
      return *ends.first<=.5L && *ends.second>=.5L ? std::max(maximum,.25L) : maximum;
    };
    long double du=0,dv=0;for(size_t i=0;i<3;++i){du=std::max(du,std::abs(1-2*u[i]));dv=std::max(dv,std::abs(1-2*v[i]));}
    const auto mixed=a*du*dv/(x*y);
    const auto hessian=std::max(2*a*quadratic(v)/(x*x)+mixed,2*a*quadratic(u)/(y*y)+mixed);
    // Linear interpolation remainder <= ||H||/2 * sum_{i<j} lambda_i lambda_j |xi-xj|^2.
    error+=hessian*diameter/6;
  }
  const auto envelope=256*std::numeric_limits<long double>::epsilon()*scale;
  const double bound=std::nextafter(double(error+envelope),INFINITY);
  if(!std::isfinite(bound))throw std::invalid_argument("Unrepresentable native3D surface error bound.");
  return bound;
}
Point PlanarReference::Project(Id id, Point p, KernelStats* stats) const {
  id=Canonical(id);
  if(affine.count(id)) {
    const auto& domain=*affine_domains.at(affine_charts.at(id));p=domain.Chart().Project(p);
    if(!domain.Contains(id,p,stats))throw std::invalid_argument("Affine projection leaves its finite component.");return p;
  }
  const auto curved=analytic.find(id);
  if (curved!=analytic.end()) {
    const auto& g=curved->second;
    if (g.kind==AnalyticSurface::Kind::Sphere) {
      const auto direction=Delta(p,g.origin);
      const auto length=Norm(direction);
      if (!(length>0 && std::isfinite(length))) throw std::invalid_argument("Invalid native3D radial projection.");
      p={double(g.origin.x+g.extent.x*direction[0]/length),double(g.origin.y+g.extent.x*direction[1]/length),
         double(g.origin.z+g.extent.x*direction[2]/length)};
    } else if(g.kind==AnalyticSurface::Kind::Cylinder) {
      const AxisFrame frame(g);const auto d=Delta(p,g.origin);
      const auto x=Dot(d,frame.u),y=Dot(d,frame.v),z=Dot(d,frame.axis),r=std::hypot(x,y);
      if(!(r>0))throw std::invalid_argument("Cylinder projection reaches its axis.");
      Vector q{};for(size_t j=0;j<3;++j)q[j]=g.extent.x*(x*frame.u[j]+y*frame.v[j])/r+z*frame.axis[j];
      p={double(g.origin.x+q[0]),double(g.origin.y+q[1]),double(g.origin.z+q[2])};
    } else if(g.kind==AnalyticSurface::Kind::Disk) {
      const auto a=UnitAxis(g),d=Delta(p,g.origin);const auto t=Dot(a,d);
      p={double(static_cast<long double>(p.x)-a[0]*t),double(static_cast<long double>(p.y)-a[1]*t),double(static_cast<long double>(p.z)-a[2]*t)};
    } else p.z=double(BumpHeight(g,p));
  }
  if (!Contains(id,p,stats)) throw std::invalid_argument("Native3D projection leaves its finite original chart.");
  return p;
}
bool PlanarReference::OnProjectedEdge(Id id, Point a, Point b, Point p, KernelStats* stats) const {
  id=Canonical(id);
  const auto curved=analytic.find(id);
  if(affine.count(id)) {
    if(!Contains(id,a,stats)||!Contains(id,b,stats)||!Contains(id,p,stats))return false;
    const auto& chart=affine_domains.at(affine_charts.at(id))->Chart();
    return OnEdge(chart.Coordinates(a),chart.Coordinates(b),chart.Coordinates(p),stats);
  }
  if (curved==analytic.end()) return OnEdge(a,b,p,stats);
  if (!Contains(id,a,stats) || !Contains(id,b,stats) || !Contains(id,p,stats)) return false;
  const auto& g=curved->second;
  if(g.kind==AnalyticSurface::Kind::Disk)return OnEdge(a,b,p,stats);
  if(g.kind==AnalyticSurface::Kind::Cylinder) {
    const auto chart=CylinderChart(g,{a,b,p});const auto x=static_cast<long double>(chart[1].x)-chart[0].x,y=static_cast<long double>(chart[1].y)-chart[0].y;
    const auto t=std::clamp(((chart[2].x-chart[0].x)*x+(chart[2].y-chart[0].y)*y)/(x*x+y*y),0.L,1.L);
    return std::hypot(static_cast<long double>(chart[2].x)-chart[0].x-t*x,static_cast<long double>(chart[2].y)-chart[0].y-t*y)<=g.tolerance;
  }
  if (g.kind==AnalyticSurface::Kind::Bump) return OnEdge({a.x,a.y,0},{b.x,b.y,0},{p.x,p.y,0},stats);
  return ArcDistance(g,a,b,p)<=g.tolerance;
}
bool SplitEdge(const Patch& old, Edge edge, Node point, Id first, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    Endpoints(edge);
    const Incidence index(old.cells, stats);
    auto skin = SkinMap(index.Boundary());
    auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    point=ProjectSplitPoint(edge,point,nodes,physical,reference,stats);
    Patch proposal;
    for (const auto& c : old.cells) {
      auto left = c.v, right = c.v;
      bool has_a = false, has_b = false;
      for (size_t i = 0; i < 4; ++i) {
        if (c.v[i].id == edge[0]) {
          right[i] = point;
          has_a = true;
        }
        if (c.v[i].id == edge[1]) {
          left[i] = point;
          has_b = true;
        }
      }
      if (!(has_a && has_b)) throw std::invalid_argument("Native 3D split input is not an edge star.");
      if (!(Orientation(left[0].p,left[1].p,left[2].p,left[3].p,stats)>0 &&
            Orientation(right[0].p,right[1].p,right[2].p,right[3].p,stats)>0))
        throw std::invalid_argument("Native3D projected split inverts inherited child connectivity.");
      Append(proposal.cells, left, first, stats);
      Append(proposal.cells, right, first, stats);
    }
    SplitSkin(edge,point,skin,physical);
    Finish(old, proposal, skin, physical, reference, stats);
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool InsertEdgeCavity(const Patch& old, Edge edge, Node point, Id first, const PlanarReference& reference,
                      Patch& fresh, std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    Endpoints(edge);
    const Incidence index(old.cells, stats);
    const auto before = SkinMap(index.Boundary());
    auto skin = before;
    auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    if (nodes.count(point.id)) throw std::invalid_argument("Native 3D cavity insertion reuses a vertex identity.");
    for (const auto& cell : old.cells) {
      unsigned endpoints = 0;
      for (auto node : cell.v) endpoints += node.id == edge[0] || node.id == edge[1];
      if (endpoints != 2) throw std::invalid_argument("Native 3D insertion input is not a complete edge star.");
    }
    std::vector<Face> wall;
    Id facet = 0;
    for (const auto& row : before) if (Has(row.second, edge[0]) && Has(row.second, edge[1])) {
      const auto found = physical.find(row.first);
      if (found == physical.end()) throw std::invalid_argument("Native 3D incomplete insertion dependency at interface face.");
      if (!wall.empty() && facet != found->second.facet)
        throw std::invalid_argument("Native 3D insertion cannot change an immutable facet or marker junction.");
      facet = found->second.facet;
      wall.push_back(row.second);
    }
    Patch proposal;
    if (wall.empty()) {
      if (!Cone(old.cells, point, first, proposal.cells, reason, stats)) throw std::invalid_argument(reason);
      for (const auto& row : physical) proposal.surface.push_back(row.second);
    } else {
      if (reference.Declared(facet)) point.p=reference.Project(facet,point.p,stats);
      if (wall.size() != 2 || !reference.Contains(facet, point.p, stats))
        throw std::invalid_argument("Native 3D surface insertion needs two same-chart triangles.");
      for (auto face : wall) {
        physical.erase(Key(face));
        skin.erase(Key(face));
        for (size_t i = 0; i < 3; ++i) {
          const Id a = face[i], b = face[(i + 1) % 3];
          if ((a == edge[0] && b == edge[1]) || (a == edge[1] && b == edge[0])) continue;
          auto child = Oriented(Face{a, b, point.id});
          if (!reference.ContainsFace(facet, {nodes.at(a).p, nodes.at(b).p, point.p}, stats))
            throw std::invalid_argument("Native 3D surface insertion leaves or reverses its finite chart fan.");
          skin.emplace(Key(child), child);
          physical.emplace(Key(child), SurfaceFace{child, facet});
        }
      }
      // Oriented chart children retain the old parameter-space border; tetrahedra must see the retained interface.
      for (const auto& row : before) {
        if (std::find(wall.begin(), wall.end(), row.second) != wall.end()) continue;
        const auto f = row.second;
        const std::array<Node, 4> v{point, nodes.at(f[0]), nodes.at(f[1]), nodes.at(f[2])};
        if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p, stats) > 0))
          throw std::invalid_argument("Native 3D insertion cannot see its retained cavity skin.");
        Append(proposal.cells, v, first, stats);
      }
      Finish(old, proposal, skin, physical, reference, stats);
    }
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool SplitSurfaceStrip(const Patch& old,Edge edge,Node point,const std::vector<std::pair<Id,Node>>& moves,
                       Id first,const PlanarReference& reference,Patch& fresh,std::string& reason,KernelStats* stats) {
  try {
    Bounds(old);Endpoints(edge);
    if(moves.size()>8)throw std::invalid_argument("Native3D surface strip movement exceeds its bounded proposal.");
    const auto before=SkinMap(Incidence(old.cells,stats).Boundary());
    const auto physical=CheckSurface(old,before,reference,stats);auto nodes=Nodes(old.cells);
    if(nodes.count(point.id))throw std::invalid_argument("Native3D surface strip reuses the split identity.");
    std::map<Id,Node> replacement;std::set<Id> new_ids{point.id};
    for(const auto& move:moves) {
      if(!nodes.count(move.first) || nodes.count(move.second.id) || !new_ids.insert(move.second.id).second ||
         !replacement.emplace(move.first,ProjectMovePoint(move.first,move.second,before,physical,reference,stats)).second)
        throw std::invalid_argument("Native3D surface strip reuses a movement identity.");
    }
    auto mapped=[&](Id id){const auto found=replacement.find(id);return found==replacement.end() ? nodes.at(id) : found->second;};
    Skin skin;Physical marked;
    for(const auto& row:before) {
      auto f=row.second;for(auto& id:f)id=mapped(id).id;f=Oriented(f);skin.emplace(Key(f),f);
      const auto found=physical.find(row.first);
      if(found!=physical.end())marked.emplace(Key(f),SurfaceFace{f,found->second.facet});
    }
    Id facet=UINT64_MAX;unsigned incident=0;bool curved=false;
    for(const auto& row:physical)if(Has(row.second.v,edge[0]) && Has(row.second.v,edge[1])) {
      const auto authority=reference.Analytic().find(row.second.facet);
      curved=curved || (authority!=reference.Analytic().end() && authority->second.kind!=AnalyticSurface::Kind::Disk);
      if(facet==UINT64_MAX)facet=row.second.facet;
      ++incident;
    }
    if(incident!=0 && incident!=2)throw std::invalid_argument("Native3D strip split needs a complete interior or physical boundary edge.");
    const auto a=nodes.at(edge[0]).p,b=nodes.at(edge[1]).p;
    const Point midpoint{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
    bool interior_moves=true,feature=false;
    for(const auto& row:physical) {
      for(Id id:row.second.v)interior_moves=interior_moves && !replacement.count(id);
      if(Has(row.second.v,edge[0]) && Has(row.second.v,edge[1]))feature=feature || row.second.facet!=facet;
    }
    if(curved && interior_moves && (feature || (point.p.x==midpoint.x && point.p.y==midpoint.y && point.p.z==midpoint.z))) {
      // Interior support motion does not change a physical feature or its authoritative projection.
      point=ProjectSplitPoint(edge,point,nodes,physical,reference,stats,feature);
    } else if(curved) {
      for(const auto& row:physical)if(Has(row.second.v,edge[0]) && Has(row.second.v,edge[1]) && row.second.facet!=facet)
        throw std::invalid_argument("Native3D off-edge strip split crosses a chart or marker junction.");
      if(!reference.Analytic().count(facet) || reference.Analytic().at(facet).kind==AnalyticSurface::Kind::Disk)
        throw std::invalid_argument("Native3D off-edge strip split needs two same-chart curved faces.");
      point.p=reference.Project(facet,point.p,stats);
    } else {
      for(const auto& move:replacement)nodes.at(move.first)=move.second;
      if(incident)point=ProjectSplitPoint(edge,point,nodes,physical,reference,stats,true);
      else {
        // A complete interior fan preserves its skin even when the inserted point leaves the old diagonal.
        for(const auto& f:before)if(Has(f.second,edge[0]) && Has(f.second,edge[1]))
          throw std::invalid_argument("Native3D interior insertion lacks a complete edge star.");
        if(!(std::isfinite(point.p.x) && std::isfinite(point.p.y) && std::isfinite(point.p.z)))
          throw std::invalid_argument("Native3D interior insertion has nonfinite coordinates.");
      }
    }
    const Edge split{mapped(edge[0]).id,mapped(edge[1]).id};SplitSkin(split,point,skin,marked);
    Patch proposal;
    for(const auto& cell:old.cells) {
      auto left=cell.v;for(auto& n:left)n=mapped(n.id);auto right=left;
      bool a=false,b=false;
      for(size_t i=0;i<4;++i){if(cell.v[i].id==edge[0]){right[i]=point;a=true;}
                             if(cell.v[i].id==edge[1]){left[i]=point;b=true;}}
      if(!(a && b)){left=cell.v;for(auto& n:left)n=mapped(n.id);}
      auto append=[&](const std::array<Node,4>& tet){
        if(!(Orientation(tet[0].p,tet[1].p,tet[2].p,tet[3].p,stats)>0))
          throw std::invalid_argument("Native3D surface strip inverts inherited volume connectivity.");
        Append(proposal.cells,tet,first,stats);
      };
      append(left);if(a && b)append(right);
    }
    Finish(old,proposal,skin,marked,reference,stats);
    fresh.cells.swap(proposal.cells);fresh.surface.swap(proposal.surface);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
std::vector<Id> SplitPatchApices(const Patch& patch,Edge edge,const std::map<Id,size_t>* costs) {
  Bounds(patch);Endpoints(edge);std::set<Id> fixed{edge[0],edge[1]};
  for(const auto& f:patch.surface)fixed.insert(f.v.begin(),f.v.end());
  std::vector<Edge> pairs;
  for(const auto& c:patch.cells)if(std::count_if(c.v.begin(),c.v.end(),[&](Node n){return n.id==edge[0] || n.id==edge[1];})==2) {
    Edge pair{};unsigned i=0;for(auto n:c.v)if(n.id!=edge[0] && n.id!=edge[1])pair[i++]=n.id;
    pairs.push_back(pair);
  }
  if(pairs.empty())return {};
  std::map<Id,uint64_t> covered;std::map<Id,size_t> weight;
  for(size_t i=0;i<pairs.size();++i)for(Id id:pairs[i]) {
    if(fixed.count(id) || (costs && (!costs->count(id) || !costs->at(id))))continue;
    covered[id]|=uint64_t(1)<<i;weight[id]=costs?costs->at(id):1;
  }
  const uint64_t all=pairs.size()==64?UINT64_MAX:(uint64_t(1)<<pairs.size())-1;
  std::vector<Id> chosen,best;size_t best_cost=SIZE_MAX;
  std::function<void(uint64_t,size_t)> visit=[&](uint64_t mask,size_t cost) {
    if(mask==all) {
      auto sorted=chosen;std::sort(sorted.begin(),sorted.end());
      if(cost<best_cost || (cost==best_cost && (best.empty() || sorted<best))){best_cost=cost;best=std::move(sorted);}
      return;
    }
    if(chosen.size()==8 || cost>=best_cost)return;
    size_t i=0;while(mask&(uint64_t(1)<<i))++i;auto options=pairs[i];
    auto value=[&](Id id){return weight.count(id)?weight.at(id):SIZE_MAX;};
    if(std::make_pair(value(options[1]),options[1])<std::make_pair(value(options[0]),options[0]))std::swap(options[0],options[1]);
    for(Id id:options)if(covered.count(id) && weight.at(id)<=SIZE_MAX-cost) {
      chosen.push_back(id);visit(mask|covered.at(id),cost+weight.at(id));chosen.pop_back();
    }
  };
  visit(0,0);return best;
}
bool PlaceSplitPatch(const Patch& old,Edge edge,Node point,const std::vector<std::pair<Id,Id>>& apices,Id first,
                       FrozenField& target,const PlanarReference& reference,const std::map<int,double>& heights,
                       Patch& fresh,JointSplitReport& report,std::string& reason,size_t maximum_trials) {
  report={};
  try {
    Bounds(old);Endpoints(edge);
    if(apices.size()>8)throw std::invalid_argument("Native3D joint placement exceeds its apex bound.");
    if(!maximum_trials || maximum_trials>256)throw std::invalid_argument("Native3D joint placement exceeds its trial bound.");
    const auto nodes=Nodes(old.cells);const auto skin=SkinMap(Incidence(old.cells).Boundary());
    CheckSurface(old,skin,reference,nullptr);
    std::set<Id> selected,reserved{point.id};
    for(const auto& apex:apices) {
      if(!selected.insert(apex.first).second || !reserved.insert(apex.second).second || nodes.count(apex.second))
        throw std::invalid_argument("Native3D joint placement repeats an apex or reserved identity.");
      nodes.at(apex.first);
      for(const auto& f:skin)if(Has(f.second,apex.first))
        throw std::invalid_argument("Native3D joint placement needs complete free apex stars.");
    }
    const Point a=nodes.at(edge[0]).p,b=nodes.at(edge[1]).p;
    auto tangent=Delta(b,a);const auto square=Dot(tangent,tangent),length=Norm(tangent);
    if(!(length>0))throw std::invalid_argument("Native3D joint placement has a degenerate edge.");
    Vector normal{};bool boundary=false;
    for(const auto& f:old.surface)if(Has(f.v,edge[0]) && Has(f.v,edge[1])) {
      boundary=true;normal=Cross(Delta(nodes.at(f.v[1]).p,nodes.at(f.v[0]).p),Delta(nodes.at(f.v[2]).p,nodes.at(f.v[0]).p));break;
    }
    if(!boundary) {
      unsigned k=0;for(unsigned i=1;i<3;++i)if(std::abs(tangent[i])<std::abs(tangent[k]))k=i;
      Vector seed{};seed[k]=1;normal=Cross(tangent,seed);
    }
    const auto n=Norm(normal);if(!(n>0))throw std::invalid_argument("Native3D joint placement has a degenerate frame.");
    for(unsigned i=0;i<3;++i){tangent[i]/=length;normal[i]/=n;}
    const std::array<Vector,3> basis{normal,tangent,Cross(normal,tangent)};
    std::vector<std::array<long double,3>> steps(apices.size());
    for(size_t k=0;k<apices.size();++k) {
      const auto origin=nodes.at(apices[k].first).p;const auto metric=target.Query(origin).target;
      for(unsigned i=0;i<3;++i) {
        const Point end{double(origin.x+basis[i][0]),double(origin.y+basis[i][1]),double(origin.z+basis[i][2])};
        const auto norm=MetricNorm(origin,end,metric);
        if(!(norm>0))throw std::invalid_argument("Native3D joint placement has an invalid directional metric.");
        steps[k][i]=.25L/norm;
      }
    }
    const size_t point_dofs=boundary?1:3;std::array<long double,2> transverse{};
    if(!boundary)for(unsigned i=0;i<2;++i) {
      const auto d=basis[2*i];const Point end{double(point.p.x+d[0]),double(point.p.y+d[1]),double(point.p.z+d[2])};
      const auto norm=MetricNorm(point.p,end,target.Query(point.p).target);
      if(!(norm>0))throw std::invalid_argument("Native3D joint placement has an invalid insertion metric.");
      transverse[i]=.25L/norm;
    }
    std::vector<long double> best(point_dofs+3*apices.size(),0);best[0]=Dot(Delta(point.p,a),Delta(b,a))/square;
    double best_loss=std::numeric_limits<double>::infinity();
    const size_t placement_limit=(maximum_trials+1)/2;
    auto score=[&](const Patch& proposal) {
      double shape=0,excess=0;std::set<Edge> measured;
      for(const auto& c:proposal.cells) {
        const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},target.Query(Centroid(c)).target);
        shape+=std::pow(std::max(0.,1-m.mean_ratio/MinimumMeanRatio),2)+
               std::pow(std::max(0.,1-m.minimum_scaled_jacobian/MinimumScaledJacobian),2);
        for(unsigned j=0;j<4;++j)for(unsigned k=j+1;k<4;++k) {
          const auto u=c.v[j],v=c.v[k];
          if(!measured.insert({std::min(u.id,v.id),std::max(u.id,v.id)}).second)continue;
          excess+=std::pow(std::max(0.L,MetricNorm(u.p,v.p,target.QueryMidpoint(u.p,v.p).target)-MaximumMetricEdge),2);
        }
      }
      return std::make_pair(shape,100*shape+excess);
    };
    auto trial=[&](const std::vector<long double>& state) {
      if(report.trials>=placement_limit)return false;
      ++report.trials;
      if(!(state[0]>0 && state[0]<1)){++report.geometry_rejected;return false;}
      Node inserted=point;inserted.p={double((1-state[0])*a.x+state[0]*b.x),double((1-state[0])*a.y+state[0]*b.y),double((1-state[0])*a.z+state[0]*b.z)};
      if(!boundary) {
        Vector offset{};for(unsigned i=0;i<2;++i)for(unsigned k=0;k<3;++k)offset[k]+=state[1+i]*transverse[i]*basis[2*i][k];
        inserted.p={double(inserted.p.x+offset[0]),double(inserted.p.y+offset[1]),double(inserted.p.z+offset[2])};
      }
      std::vector<std::pair<Id,Node>> moves;
      for(size_t i=0;i<apices.size();++i) {
        const auto origin=nodes.at(apices[i].first).p;Vector displacement{};
        for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)displacement[k]+=state[point_dofs+3*i+j]*basis[j][k]*steps[i][j];
        Node moved{apices[i].second,{double(origin.x+displacement[0]),double(origin.y+displacement[1]),double(origin.z+displacement[2])}};
        moved=ConstrainMovePoint(old,apices[i].first,moved,heights,reference);
        if(moved.p.x!=origin.x || moved.p.y!=origin.y || moved.p.z!=origin.z)moves.emplace_back(apices[i].first,moved);
      }
      Patch proposal;
      if(!SplitSurfaceStrip(old,edge,inserted,moves,first,reference,proposal,reason)) {
        ++report.geometry_rejected;return false;
      }
      FirstHeightReport height;
      if(!CheckFirstHeight(proposal,heights,reference,height,reason)){++report.height_rejected;return false;}
      const auto measured=score(proposal);const double shape=measured.first,loss=measured.second;
      if(report.trials==1)report.initial_loss=loss;
      if(!(loss<best_loss))return false;
      best_loss=loss;report.best_loss=loss;report.best_shape_loss=shape;report.best=proposal;best=state;
      if(shape==0) {
        PatchMetric metric;
        if(ValidateMetricChange(old.cells,proposal.cells,MetricChange::Refine,target,metric,reason)) {
          fresh=std::move(proposal);return true;
        }
        ++report.metric_rejected;
      }
      return false;
    };
    if(trial(best)){reason.clear();return true;}
    // Paired fraction/normal directions cross a blockage that either independent displacement can worsen.
    std::vector<std::vector<int>> directions;
    for(size_t i=0;i<best.size();++i){std::vector<int> d(best.size(),0);d[i]=1;directions.push_back(std::move(d));}
    for(size_t i=0;i<apices.size();++i)for(int sign:{-1,1}) {
      std::vector<int> d(best.size(),0);d[0]=1;d[point_dofs+3*i]=sign;directions.push_back(std::move(d));
    }
    if(!apices.empty())for(int split:{-1,0,1}) {
      std::vector<int> d(best.size(),0);d[0]=split;for(size_t i=0;i<apices.size();++i)d[point_dofs+3*i]=1;
      directions.push_back(std::move(d));
    }
    long double step=1;
    while(report.trials<placement_limit && step>=1.L/128) {
      const auto start=best_loss;
      for(const auto& direction:directions)for(int sign:{-1,1}) {
        auto state=best;for(size_t i=0;i<state.size();++i)state[i]+=sign*step*direction[i]*(i==0?.25L:1.L);
        if(trial(state)){reason.clear();return true;}
      }
      if(!(best_loss<start))step/=2;
    }
    if(!report.best.cells.empty() && report.best_shape_loss>0 && report.trials<maximum_trials) {
      const auto present=Nodes(report.best.cells);std::set<Id> movable{point.id};
      for(const auto& apex:apices)movable.insert(present.count(apex.second)?apex.second:apex.first);
      Patch repaired;ConnectivityRepairReport connectivity;
      const bool progress=RepairMetricConnectivity(report.best,target,reference,heights,first,repaired,connectivity,reason,
          maximum_trials-report.trials,false,&movable);
      report.trials+=connectivity.trials;report.connectivity_trials=connectivity.trials;
      report.flips=connectivity.flips;report.moves=connectivity.moves;
      report.linearizations=connectivity.linearizations;report.metric_probes=connectivity.metric_probes;
      report.linear_iterations=connectivity.linear_iterations;report.solve_trials=connectivity.solve_trials;
      report.blocked_interior_vertex=connectivity.blocked_interior_vertex;
      if(progress) {
        for(const auto& apex:apices) {
          const auto origin=nodes.at(apex.first).p;
          for(auto& c:repaired.cells)for(auto& n:c.v)if(n.id==apex.first || n.id==apex.second)
            n.id=(n.p.x==origin.x && n.p.y==origin.y && n.p.z==origin.z)?apex.first:apex.second;
        }
        const auto measured=score(repaired);
        if(measured.second<best_loss) {
          best_loss=measured.second;report.best_loss=best_loss;report.best_shape_loss=measured.first;report.best=repaired;
        }
        PatchMetric metric;
        if(ValidateMetricChange(old.cells,repaired.cells,MetricChange::Refine,target,metric,reason)) {
          fresh=std::move(repaired);reason.clear();return true;
        }
        ++report.metric_rejected;
      }
    }
    reason="Native3D joint placement/reconnection exhausted its shared trial budget without frozen metric progress: "+reason;return false;
  }catch(const std::exception& e){reason=e.what();return false;}
}
namespace {
struct FootPlane { std::array<Point,3> p; Vector normal; long double area; };
Point FitNormalFeet(Point origin,const std::array<Vector,3>& constrained,size_t rank,
                    const std::vector<FootPlane>& planes) {
  std::array<Vector,2> free{};size_t dimensions=0;
  for(unsigned axis=0;axis<3 && dimensions<3-rank;++axis) {
    Vector v{};v[axis]=1;
    for(unsigned pass=0;pass<2;++pass) {
      for(size_t k=0;k<rank;++k){const auto c=Dot(v,constrained[k]);for(unsigned j=0;j<3;++j)v[j]-=c*constrained[k][j];}
      for(size_t k=0;k<dimensions;++k){const auto c=Dot(v,free[k]);for(unsigned j=0;j<3;++j)v[j]-=c*free[k][j];}
    }
    const auto norm=Norm(v);if(norm>64*std::numeric_limits<long double>::epsilon()) {
      for(auto& x:v)x/=norm;free[dimensions++]=v;
    }
  }
  struct Bound { std::array<long double,2> gradient{};long double value=0; };
  std::vector<Bound> bounds;bool inside=true;
  for(const auto& plane:planes)for(unsigned j=0;j<3;++j) {
    const auto u=Delta(plane.p[(j+1)%3],origin),v=Delta(plane.p[(j+2)%3],origin);
    const auto tolerance=256*std::numeric_limits<long double>::epsilon()*std::max(1.L,Norm(u)*Norm(v)/plane.area);
    if(!std::isfinite(tolerance) || tolerance>1e-8L)throw std::invalid_argument("Native3D wall-foot fit cannot resolve its footprint.");
    Bound bound;bound.value=Dot(Cross(u,v),plane.normal)/plane.area+tolerance;
    const auto gradient=Cross(Delta(plane.p[(j+1)%3],plane.p[(j+2)%3]),plane.normal);
    for(size_t k=0;k<dimensions;++k)bound.gradient[k]=Dot(gradient,free[k])/plane.area;
    inside=inside && bound.value>=0;bounds.push_back(bound);
  }
  if(inside)return origin;
  if(!dimensions)throw std::invalid_argument("Native3D retained height planes leave no normal-foot freedom.");
  using Q=std::array<long double,2>;std::vector<Q> polygon;
  for(auto p:planes.front().p) {
    Q q{};for(size_t k=0;k<dimensions;++k)q[k]=Dot(Delta(p,origin),free[k]);polygon.push_back(q);
  }
  if(dimensions==1) {
    const auto ends=std::minmax_element(polygon.begin(),polygon.end(),[](Q a,Q b){return a[0]<b[0];});
    polygon={*ends.first,*ends.second};
  }
  for(const auto& bound:bounds) {
    std::vector<Q> clipped;
    for(size_t i=0;i<polygon.size();++i) {
      const auto a=polygon[i],b=polygon[(i+1)%polygon.size()];
      const auto x=bound.value+bound.gradient[0]*a[0]+bound.gradient[1]*a[1];
      const auto y=bound.value+bound.gradient[0]*b[0]+bound.gradient[1]*b[1];
      if(x>=0)clipped.push_back(a);
      if((x>=0)!=(y>=0)) {const auto t=x/(x-y);clipped.push_back({a[0]+t*(b[0]-a[0]),a[1]+t*(b[1]-a[1])});}
    }
    polygon=std::move(clipped);if(polygon.empty())throw std::invalid_argument("Native3D retained height/normal-foot constraints have no common fit.");
  }
  auto square=[](Q q){return q[0]*q[0]+q[1]*q[1];};Q best=polygon.front(),center{};
  for(size_t i=0;i<polygon.size();++i) {
    const auto a=polygon[i],b=polygon[(i+1)%polygon.size()];Q edge{b[0]-a[0],b[1]-a[1]};
    const auto norm=square(edge);const auto t=norm>0?std::clamp(-(a[0]*edge[0]+a[1]*edge[1])/norm,0.L,1.L):0.L;
    const Q candidate{a[0]+t*edge[0],a[1]+t*edge[1]};if(square(candidate)<square(best))best=candidate;
    for(unsigned j=0;j<2;++j)center[j]+=a[j]/polygon.size();
  }
  // A convex inward displacement avoids rounding a closest boundary fit outside the same closed footprint.
  for(unsigned j=0;j<2;++j)best[j]+=64*std::numeric_limits<double>::epsilon()*(center[j]-best[j]);
  Vector displacement{};for(size_t k=0;k<dimensions;++k)for(unsigned j=0;j<3;++j)displacement[j]+=best[k]*free[k][j];
  return {double(origin.x+displacement[0]),double(origin.y+displacement[1]),double(origin.z+displacement[2])};
}
void RestorePatchPoints(Patch& proposal,const std::map<Id,Node>& wanted,Id first,std::string& reason,KernelStats* stats) {
  for(const auto& row:wanted) {
    const auto current=Nodes(proposal.cells);if(current.count(row.first))continue;
    const auto coincident=std::find_if(current.begin(),current.end(),[&](const auto& n) {
      return n.second.p.x==row.second.p.x && n.second.p.y==row.second.p.y && n.second.p.z==row.second.p.z;
    });
    if(coincident!=current.end()) {
      for(auto& c:proposal.cells)for(auto& n:c.v)if(n.id==coincident->first)n=row.second;
      for(auto& f:proposal.surface)for(auto& id:f.v)if(id==coincident->first)id=row.first;
      continue;
    }
    std::vector<Cell> region;
    for(const auto& c:proposal.cells) {
      const auto volume=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p,stats);bool contains=true;
      for(unsigned k=0;k<4;++k) {
        auto v=c.v;v[k]=row.second;
        contains=contains && Orientation(v[0].p,v[1].p,v[2].p,v[3].p,stats)>=-64*std::numeric_limits<double>::epsilon()*volume;
      }
      if(contains)region.push_back(c);
    }
    if(region.empty())throw std::invalid_argument("Native3D wall reconstruction lost interior point "+std::to_string(row.first));
    std::vector<Cell> children;if(!Cone(region,row.second,first,children,reason,stats))throw std::invalid_argument(reason);
    proposal.cells.erase(std::remove_if(proposal.cells.begin(),proposal.cells.end(),[&](const auto& c) {
      return std::any_of(region.begin(),region.end(),[&](const auto& r){return r.id==c.id;});
    }),proposal.cells.end());
    proposal.cells.insert(proposal.cells.end(),children.begin(),children.end());
    if(proposal.cells.size()>MaximumReplacementCells || first>UINT64_MAX-(proposal.cells.size()-1))
      throw std::invalid_argument("Native3D wall point restoration exceeds replacement/identity budget.");
    for(size_t k=0;k<proposal.cells.size();++k)proposal.cells[k].id=first+k;
  }
}
}  // namespace
bool RefitWallSplit(const Patch& old,Edge edge,Node point,Id first_apex,Id first,
                    const std::map<int,double>& heights,const PlanarReference& reference,
                    Patch& fresh,Feature& obstruction,std::string& reason,KernelStats* stats) {
  obstruction={};
  try {
    Bounds(old);Endpoints(edge);FirstHeightReport height;
    const Incidence index(old.cells,stats);const auto old_skin=index.Boundary();
    ValidateSurfaceTopology(old_skin,true,4*MaximumReplacementCells,&obstruction);
    if(heights.empty() || !CheckFirstHeight(old,heights,reference,height,reason,stats))
      throw std::invalid_argument("Native3D wall refit needs an admissible prescribed-height input: "+reason);
    std::set<Id> apices;
    for(const auto& face:old.surface)if(Has(face.v,edge[0]) && Has(face.v,edge[1]) && heights.count(reference.Get(face.facet).marker))
      for(const auto& c:old.cells) {
        unsigned count=0;Id apex=0;for(auto n:c.v)if(Has(face.v,n.id))++count;else apex=n.id;
        if(count==3)apices.insert(apex);
      }
    if(apices.empty())throw std::invalid_argument("Native3D wall refit needs a requested physical wall edge.");
    for(Id id:apices)for(auto face:old_skin)if(Has(face,id))throw std::invalid_argument("Native3D wall refit needs complete free apex stars.");
    Patch split;if(!SplitSurfaceStrip(old,edge,point,{},first,reference,split,reason,stats))throw std::invalid_argument(reason);
    const auto skin=SkinMap(Incidence(split.cells,stats).Boundary());const auto physical=CheckSurface(split,skin,reference,stats);
    const auto nodes=Nodes(split.cells);std::vector<std::pair<Id,Node>> moves;size_t slot=0;
    for(Id id:apices) {
      const auto origin=nodes.at(id).p;std::array<Vector,3> basis{};std::array<long double,3> offset{};size_t rank=0;
      std::vector<FootPlane> planes;
      for(const auto& face:split.surface) {
        const auto h=heights.find(reference.Get(face.facet).marker);if(h==heights.end())continue;bool owns=false;
        for(const auto& c:split.cells) {
          unsigned count=0;bool vertex=false;for(auto n:c.v){count+=Has(face.v,n.id);vertex=vertex || n.id==id;}
          owns=owns || (count==3 && vertex && !Has(face.v,id));
        }
        if(!owns)continue;
        FootPlane plane;for(unsigned k=0;k<3;++k)plane.p[k]=nodes.at(face.v[k]).p;
        plane.normal=Cross(Delta(plane.p[1],plane.p[0]),Delta(plane.p[2],plane.p[0]));plane.area=Norm(plane.normal);
        if(!(plane.area>0))throw std::invalid_argument("Native3D wall refit has a degenerate height plane.");
        for(auto& x:plane.normal)x/=plane.area;planes.push_back(plane);auto normal=plane.normal;
        auto value=Dot(normal,Delta(plane.p[0],origin))-h->second;
        for(unsigned pass=0;pass<2;++pass)for(size_t k=0;k<rank;++k) {
          const auto component=Dot(normal,basis[k]);value-=component*offset[k];for(unsigned j=0;j<3;++j)normal[j]-=component*basis[k][j];
        }
        const auto independent=Norm(normal);
        if(rank<3 && independent>64*std::numeric_limits<long double>::epsilon()) {
          for(auto& x:normal)x/=independent;basis[rank]=normal;offset[rank++]=value/independent;
        } else if(std::abs(value)>1e-12L*h->second)
          throw std::invalid_argument("Native3D retained apex has incompatible height planes; different connectivity is required.");
      }
      if(planes.empty())throw std::invalid_argument("Native3D wall refit loses a height dependency.");
      Vector displacement{};for(size_t k=0;k<rank;++k)for(unsigned j=0;j<3;++j)displacement[j]+=offset[k]*basis[k][j];
      const auto p=FitNormalFeet({double(origin.x+displacement[0]),double(origin.y+displacement[1]),double(origin.z+displacement[2])},basis,rank,planes);
      if(first_apex>UINT64_MAX-slot || nodes.count(first_apex+slot))throw std::invalid_argument("Native3D wall refit exceeds reserved apex identities.");
      if(p.x!=origin.x || p.y!=origin.y || p.z!=origin.z)moves.emplace_back(id,Node{first_apex+slot,p});++slot;
    }
    Patch proposal=split;
    for(const auto& move:moves) {
      auto wanted=Nodes(proposal.cells);wanted.erase(move.first);const auto current=Nodes(proposal.cells);const Incidence global(proposal.cells,stats);
      std::set<Id> region_ids;for(const auto& c:proposal.cells)if(std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==move.first;}))region_ids.insert(c.id);
      if(region_ids.empty())throw std::invalid_argument("Native3D wall refit loses its moving vertex star.");
      std::vector<Cell> region;
      for(;;) {
        region.clear();for(const auto& c:proposal.cells)if(region_ids.count(c.id))region.push_back(c);
        if(region.size()>MaximumCavityCells)throw std::invalid_argument("Native3D wall refit visibility exceeds the cone input budget.");
        Id neighbor=UINT64_MAX;
        for(auto face:Incidence(region,stats).Boundary()) {
          if(Orientation(move.second.p,current.at(face[0]).p,current.at(face[1]).p,current.at(face[2]).p,stats)>0)continue;
          const auto adjacent=global.FaceCells(face);const auto outside=std::find_if(adjacent.begin(),adjacent.end(),[&](Id id){return !region_ids.count(id);});
          if(outside==adjacent.end()) {
            if(!physical.count(Key(face)))obstruction={face,3};
            throw std::invalid_argument("Native3D wall-foot fit needs visibility across an admitted interface.");
          }
          neighbor=*outside;break;
        }
        if(neighbor==UINT64_MAX)break;region_ids.insert(neighbor);
      }
      std::vector<Cell> children;if(!Cone(region,move.second,first,children,reason,stats))throw std::invalid_argument(reason);
      proposal.cells.erase(std::remove_if(proposal.cells.begin(),proposal.cells.end(),[&](const auto& c){return region_ids.count(c.id);}),proposal.cells.end());
      proposal.cells.insert(proposal.cells.end(),children.begin(),children.end());
      if(proposal.cells.size()>MaximumReplacementCells || first>UINT64_MAX-(proposal.cells.size()-1))throw std::invalid_argument("Native3D wall refit exceeds replacement/identity budget.");
      for(size_t k=0;k<proposal.cells.size();++k)proposal.cells[k].id=first+k;
      RestorePatchPoints(proposal,wanted,first,reason,stats);
    }
    proposal.surface.clear();Finish(old,proposal,skin,physical,reference,stats);
    if(!CheckFirstHeight(proposal,heights,reference,height,reason,stats))throw std::invalid_argument(reason);
    fresh=std::move(proposal);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool ReconstructWallSplit(const Patch& old,Edge edge,Node point,Id first_wall_node,Id first,
                          const std::map<int,double>& heights,const PlanarReference& reference,
                          Patch& fresh,Face& obstruction,std::string& reason,KernelStats* stats,double surface_tolerance) {
  obstruction={};
  try {
    Bounds(old);Endpoints(edge);
    if(!std::isfinite(surface_tolerance) || surface_tolerance<0)
      throw std::invalid_argument("Native3D coupled wall split has an invalid surface tolerance.");
    FirstHeightReport height;
    if(heights.empty() || !CheckFirstHeight(old,heights,reference,height,reason,stats))
      throw std::invalid_argument("Native3D retained wall split needs an admissible prescribed-height input: "+reason);
    Patch proposal;
    if(!SplitSurfaceStrip(old,edge,point,{},first,reference,proposal,reason,stats))throw std::invalid_argument(reason);
    size_t perimeter_points=0;
    if(surface_tolerance>0) {
      const auto boundary=SkinMap(Incidence(old.cells,stats).Boundary());
      const auto marked=CheckSurface(old,boundary,reference,stats);const auto original_nodes=Nodes(old.cells);
      std::map<Edge,double> perimeter;
      for(const auto& f:old.surface)if(Has(f.v,edge[0]) && Has(f.v,edge[1]) &&
          heights.count(reference.Get(f.facet).marker) && reference.Analytic().count(f.facet)) {
        const auto tolerance=std::min(surface_tolerance,.5*heights.at(reference.Get(f.facet).marker));
        for(unsigned i=0;i<3;++i) {
          Edge other{f.v[i],f.v[(i+1)%3]};std::sort(other.begin(),other.end());
          auto selected=edge;std::sort(selected.begin(),selected.end());if(other==selected)continue;
          const auto found=perimeter.emplace(other,tolerance);if(!found.second)found.first->second=std::min(found.first->second,tolerance);
        }
      }
      for(const auto& row:perimeter) {
        if(first_wall_node>UINT64_MAX-perimeter_points)throw std::invalid_argument("Native3D perimeter split identity overflows.");
        const auto a=original_nodes.at(row.first[0]).p,b=original_nodes.at(row.first[1]).p;
        Node mid{first_wall_node+perimeter_points,{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)}};
        const auto projected=ProjectSplitPoint(row.first,mid,original_nodes,marked,reference,stats);
        if(Norm(Delta(projected.p,mid.p))<=row.second)continue;
        for(const auto& f:boundary)if(Has(f.second,row.first[0]) && Has(f.second,row.first[1]) && !marked.count(f.first)) {
          obstruction=f.second;throw std::invalid_argument("Native3D coupled perimeter split needs its complete physical edge star.");
        }
        Patch next;
        if(!SplitSurfaceStrip(proposal,row.first,mid,{},first,reference,next,reason,stats))throw std::invalid_argument(reason);
        proposal=std::move(next);++perimeter_points;
      }
    }
    if(first_wall_node>UINT64_MAX-perimeter_points)throw std::invalid_argument("Native3D wall identities overflow after perimeter splits.");
    first_wall_node+=perimeter_points;
    const auto skin=SkinMap(Incidence(proposal.cells,stats).Boundary());
    const auto physical=CheckSurface(proposal,skin,reference,stats);
    if(std::none_of(old.surface.begin(),old.surface.end(),[&](const auto& f) {
      return heights.count(reference.Get(f.facet).marker) && Has(f.v,edge[0]) && Has(f.v,edge[1]);
    }))throw std::invalid_argument("Native3D retained wall split needs a requested physical wall edge.");
    std::vector<SurfaceFace> walls;
    for(const auto& row:physical)if(heights.count(reference.Get(row.second.facet).marker))walls.push_back(row.second);
    if(walls.empty() || walls.size()+perimeter_points>MaximumReplacementCells || first_wall_node>UINT64_MAX-(walls.size()-1))
      throw std::invalid_argument("Native3D retained wall split exceeds reserved wall identities.");
    auto nodes=Nodes(proposal.cells);
    for(size_t i=0;i<walls.size();++i)if(nodes.count(first_wall_node+i))
      throw std::invalid_argument("Native3D retained wall split reuses a node identity.");
    std::set<Face> protected_faces;
    for(size_t at=0;at<walls.size();++at) {
      const auto face=walls[at];nodes=Nodes(proposal.cells);
      const auto a=nodes.at(face.v[0]).p,b=nodes.at(face.v[1]).p,c=nodes.at(face.v[2]).p;
      auto normal=Cross(Delta(b,a),Delta(c,a));const auto area=Norm(normal);
      if(!(area>0))throw std::invalid_argument("Native3D retained wall split has a degenerate face.");
      for(auto& value:normal)value/=area;
      const auto h=heights.at(reference.Get(face.facet).marker);
      const Point center{double(((long double)a.x+b.x+c.x)/3),double(((long double)a.y+b.y+c.y)/3),double(((long double)a.z+b.z+c.z)/3)};
      Node apex{first_wall_node+at,{double((long double)center.x-h*normal[0]),double((long double)center.y-h*normal[1]),double((long double)center.z-h*normal[2])}};
      const auto coincident=std::find_if(nodes.begin(),nodes.end(),[&](const auto& row) {
        const auto p=row.second.p;return p.x==apex.p.x && p.y==apex.p.y && p.z==apex.p.z;
      });
      if(coincident!=nodes.end())apex=coincident->second;
      const Incidence global(proposal.cells,stats);
      const auto owners=global.FaceCells(face.v);
      if(owners.size()!=1)throw std::invalid_argument("Native3D retained wall split loses a physical face owner.");
      auto cell=[&](Id id)->const Cell& {
        const auto found=std::find_if(proposal.cells.begin(),proposal.cells.end(),[&](const auto& c){return c.id==id;});
        if(found==proposal.cells.end())throw std::invalid_argument("Native3D retained wall split loses a cavity dependency.");
        return *found;
      };
      bool already=false;
      for(auto n:cell(owners.front()).v)already=already || n.id==apex.id;
      if(already){protected_faces.insert(Key(face.v));continue;}
      const auto seed_faces=Incidence({cell(owners.front())},stats).Boundary();
      if(std::any_of(seed_faces.begin(),seed_faces.end(),[&](Face f){return protected_faces.count(Key(f));}))
        throw std::invalid_argument("Native3D normal wall seed also owns a completed wall child.");
      std::set<Id> region_ids{owners.front()};std::vector<Cell> region;
      for(;;) {
        region.clear();for(Id id:region_ids)region.push_back(cell(id));
        const auto boundary=Incidence(region,stats).Boundary();Id neighbor=UINT64_MAX;
        for(const auto f:boundary) {
          if(Has(f,apex.id))continue;
          if(Orientation(apex.p,nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p,stats)>0)continue;
          const auto adjacent=global.FaceCells(f);
          const auto outside=std::find_if(adjacent.begin(),adjacent.end(),[&](Id id){return !region_ids.count(id);});
          if(outside==adjacent.end()) {
            if(!physical.count(Key(f)))obstruction=f;
            throw std::invalid_argument("Native3D normal wall point is obstructed by retained patch interface.");
          }
          const auto& candidate=cell(*outside);const auto faces=Incidence({candidate},stats).Boundary();
          if(std::any_of(faces.begin(),faces.end(),[&](Face f){return protected_faces.count(Key(f));}))
            throw std::invalid_argument("Native3D normal wall insertion is blocked by a completed wall child.");
          neighbor=*outside;break;
        }
        if(neighbor==UINT64_MAX)break;
        if(region_ids.size()==MaximumCavityCells)
          throw std::invalid_argument("Native3D normal wall visibility cavity exceeds its admitted cell budget.");
        region_ids.insert(neighbor);
      }
      std::vector<Cell> children;
      if(!Cone(region,apex,first,children,reason,stats))throw std::invalid_argument(reason);
      proposal.cells.erase(std::remove_if(proposal.cells.begin(),proposal.cells.end(),[&](const auto& c) {
        return region_ids.count(c.id);
      }),proposal.cells.end());
      proposal.cells.insert(proposal.cells.end(),children.begin(),children.end());
      if(proposal.cells.size()>MaximumReplacementCells || first>UINT64_MAX-(proposal.cells.size()-1))
        throw std::invalid_argument("Native3D retained wall split exceeds replacement cells/identities.");
      for(size_t i=0;i<proposal.cells.size();++i)proposal.cells[i].id=first+i;
      protected_faces.insert(Key(face.v));
    }
    proposal.surface.clear();Finish(old,proposal,skin,physical,reference,stats);
    if(!CheckFirstHeight(proposal,heights,reference,height,reason,stats))throw std::invalid_argument(reason);
    fresh=std::move(proposal);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool ReconstructSplitPatch(const Patch& old,Edge edge,Node point,Node interior,const Tensor& placement,
                           Id first,const PlanarReference& reference,Patch& fresh,Face& obstruction,
                           std::string& reason,KernelStats* stats,const std::map<int,double>& heights) {
  obstruction={};Face wallObstruction{};
  try {
    Bounds(old);Endpoints(edge);ValidateTensor(placement,stats);
    const auto index=Incidence(old.cells,stats);
    auto skin=SkinMap(index.Boundary());auto physical=CheckSurface(old,skin,reference,stats);
    auto nodes=Nodes(old.cells);
    if(std::none_of(physical.begin(),physical.end(),[&](const auto& f){return Has(f.second.v,edge[0]) && Has(f.second.v,edge[1]);}))
      throw std::invalid_argument("Native3D joint boundary split requires a physical edge.");
    if(nodes.count(interior.id) || interior.id==point.id)throw std::invalid_argument("Native3D joint insertion reuses a node identity.");
    point=ProjectSplitPoint(edge,point,nodes,physical,reference,stats,true);
    SplitSkin(edge,point,skin,physical);nodes.emplace(point.id,point);
    long double wallViolation=-1;std::map<Face,Point> wallPoints;
    for(const auto& entry:physical) {
      const auto height=heights.find(reference.Get(entry.second.facet).marker);if(height==heights.end())continue;
      const auto f=entry.second.v;const auto a=nodes.at(f[0]).p,b=nodes.at(f[1]).p,c=nodes.at(f[2]).p;
      auto normal=Cross(Delta(b,a),Delta(c,a));const auto area=Norm(normal);for(auto& n:normal)n/=area;
      const Point center{double(((long double)a.x+b.x+c.x)/3),double(((long double)a.y+b.y+c.y)/3),double(((long double)a.z+b.z+c.z)/3)};
      const Point apex{double((long double)center.x-height->second*normal[0]),
                       double((long double)center.y-height->second*normal[1]),double((long double)center.z-height->second*normal[2])};
      wallPoints.emplace(entry.first,apex);
      for(const auto& row:skin)if(!physical.count(row.first)) {
        const auto face=row.second;const auto x=nodes.at(face[0]).p,y=nodes.at(face[1]).p,z=nodes.at(face[2]).p;
        const auto orient=Orientation(apex,x,y,z,stats);
        if(orient>0)continue;
        const auto violation=-orient/Norm(Cross(Delta(y,x),Delta(z,x)));
        if(violation>wallViolation){wallViolation=violation;wallObstruction=row.first;}
      }
    }
    if(skin.size()>MaximumReplacementCells)throw std::invalid_argument("Native3D joint skin exceeds replacement budget.");
    const auto origin=old.cells.front().v.front().p;
    Vector moment{};long double volume=0,rms=0;
    for(const auto& c:old.cells) {
      const auto det=Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p,stats);
      if(!(det>0))throw std::invalid_argument("Native3D joint patch contains an inverted donor.");
      for(const auto n:c.v){const auto d=Delta(n.p,origin);for(size_t k=0;k<3;++k)moment[k]+=det*d[k]/4;}
      volume+=det;rms+=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},placement,stats).rms_edge;
    }
    interior.p={double(origin.x+moment[0]/volume),double(origin.y+moment[1]/volume),double(origin.z+moment[2]/volume)};
    rms/=old.cells.size();
    const long double scale=std::max({std::abs(placement.xx),std::abs(placement.xy),std::abs(placement.xz),std::abs(placement.yy),std::abs(placement.yz),std::abs(placement.zz)});
    const long double m[3][3]={{placement.xx/scale,placement.xy/scale,placement.xz/scale},{placement.xy/scale,placement.yy/scale,placement.yz/scale},{placement.xz/scale,placement.yz/scale,placement.zz/scale}};
    long double l[3][3]{};
    for(size_t i=0;i<3;++i)for(size_t j=0;j<=i;++j) {
      auto value=m[i][j];for(size_t k=0;k<j;++k)value-=l[i][k]*l[j][k];
      if(i==j){if(!(value>0))throw std::invalid_argument("Unrepresentable native3D placement frame.");l[i][j]=std::sqrt(value);}
      else l[i][j]=value/l[j][j];
    }
    std::array<Vector,3> basis{};
    for(size_t column=0;column<3;++column)for(int i=2;i>=0;--i) {
      long double value=size_t(i)==column ? 1/std::sqrt(scale) : 0;
      for(size_t j=i+1;j<3;++j)value-=l[j][i]*basis[column][j];
      basis[column][i]=value/l[i][i];
    }
    auto cost=[&](Point apex,Face* blocked) {
      long double score=0,worst=INFINITY;
      for(const auto& row:skin) {
        const auto f=row.second;
        const Tetrahedron tet{apex,nodes.at(f[0]).p,nodes.at(f[1]).p,nodes.at(f[2]).p};
        const auto wall=wallPoints.find(row.first);
        long double ratio=INFINITY;
        // Score the actual four wall children, so placement respects their height and visibility.
        for(unsigned child=0;child<(wall==wallPoints.end()?1u:4u);++child) {
          auto candidate=tet;if(wall!=wallPoints.end())candidate[child]=wall->second;
          long double deficit=10,qualityRatio=-INFINITY;
          if(Orientation(candidate[0],candidate[1],candidate[2],candidate[3],stats)>0) {
            const auto quality=Measure(candidate,placement,stats);
            const auto q=std::max(0.,.22-quality.mean_ratio),j=std::max(0.,.06-quality.minimum_scaled_jacobian);
            deficit=q*q+j*j;
            qualityRatio=std::min(quality.mean_ratio/MinimumMeanRatio,quality.minimum_scaled_jacobian/MinimumScaledJacobian);
          }
          score+=deficit;ratio=std::min(ratio,qualityRatio);
        }
        if(blocked && !physical.count(row.first) && ratio<worst){worst=ratio;*blocked=row.first;}
      }
      return score;
    };
    // Placement uses one frozen proxy; the engine certifies the complete actual-query field afterwards.
    auto best=cost(interior.p,nullptr);
    for(unsigned level=0;level<9 && best>1e-12L;++level)for(unsigned sweep=0;sweep<3 && best>1e-12L;++sweep) {
      const auto center=interior.p;const auto previous=best;
      for(int a=-1;a<=1;++a)for(int b=-1;b<=1;++b)for(int c=-1;c<=1;++c)if(a || b || c) {
        const auto step=std::ldexp(rms,-int(level)-1)/std::sqrt(static_cast<long double>(a*a+b*b+c*c));
        const Point p{double(center.x+step*(a*basis[0][0]+b*basis[1][0]+c*basis[2][0])),
                      double(center.y+step*(a*basis[0][1]+b*basis[1][1]+c*basis[2][1])),
                      double(center.z+step*(a*basis[0][2]+b*basis[1][2]+c*basis[2][2]))};
        const auto score=cost(p,nullptr);if(score<best){best=score;interior.p=p;}
      }
      if(best>=previous)break;
    }
    cost(interior.p,&obstruction);
    Patch proposal;
    for(const auto& row:skin) {
      const auto f=row.second;const std::array<Node,4> v{interior,nodes.at(f[0]),nodes.at(f[1]),nodes.at(f[2])};
      if(!(Orientation(v[0].p,v[1].p,v[2].p,v[3].p,stats)>0))throw std::invalid_argument("Native3D joint point cannot see its retained skin.");
      Append(proposal.cells,v,first,stats);
    }
    Finish(old,proposal,skin,physical,reference,stats);
    const bool wall=std::any_of(physical.begin(),physical.end(),[&](const auto& row) {
      return heights.count(reference.Get(row.second.facet).marker);
    });
    if(wall) {
      if(interior.id>UINT64_MAX-2)throw std::invalid_argument("Native3D joint wall identities overflow.");
      FirstHeightReport report;Patch layer;
      // Reuse the certified constructor; only the complete carved replacement reaches metric validation.
      if(!ConstructFirstLayer(proposal,{interior.id+1,interior.p},interior.id+2,first,heights,reference,layer,report,reason,stats))
        throw std::invalid_argument(reason);
      proposal=std::move(layer);
      RestorePatchPoints(proposal,nodes,first,reason,stats);
      proposal.surface.clear();
      Finish(old,proposal,skin,physical,reference,stats);
      if(!CheckFirstHeight(proposal,heights,reference,report,reason,stats))throw std::invalid_argument(reason);
    }
    fresh.cells.swap(proposal.cells);fresh.surface.swap(proposal.surface);reason.clear();return true;
  }catch(const std::exception& e){if(wallObstruction!=Face{})obstruction=wallObstruction;reason=e.what();return false;}
}

namespace {
std::map<Id,Vector> CoupledMetricDirection(const Patch& patch,FrozenField& target,
    const std::map<int,double>& heights,const PlanarReference& reference,const std::set<Id>& fixed,
    const std::set<Id>& active,ConnectivityRepairReport& report) {
  struct Frame {
    size_t first=0,count=0,rank=0;std::array<Vector,3> basis{},constraint{};std::vector<FootPlane> feet;Tensor metric;
  };
  struct Row { std::array<std::pair<size_t,long double>,12> entry{};size_t count=0;long double residual=0; };
  const auto nodes=Nodes(patch.cells);std::map<Id,Frame> frames;size_t count=0;long double span=0;
  for(const auto& row:nodes)span=std::max(span,Norm(Delta(row.second.p,nodes.begin()->second.p)));
  if(!(span>0))return {};
  std::map<Id,std::vector<FootPlane>> feet;
  for(const auto& face:patch.surface)if(heights.count(reference.Get(face.facet).marker)) {
    FootPlane plane;for(unsigned j=0;j<3;++j)plane.p[j]=nodes.at(face.v[j]).p;
    plane.normal=Cross(Delta(plane.p[1],plane.p[0]),Delta(plane.p[2],plane.p[0]));plane.area=Norm(plane.normal);
    for(auto& x:plane.normal)x/=plane.area;
    for(const auto& c:patch.cells) {
      unsigned matches=0;Id apex=0;for(auto n:c.v){matches+=Has(face.v,n.id);if(!Has(face.v,n.id))apex=n.id;}
      if(matches==3)feet[apex].push_back(plane);
    }
  }
  auto metricDot=[](Tensor m,Vector a,Vector b) {
    return a[0]*(m.xx*b[0]+m.xy*b[1]+m.xz*b[2])+a[1]*(m.xy*b[0]+m.yy*b[1]+m.yz*b[2])+a[2]*(m.xz*b[0]+m.yz*b[1]+m.zz*b[2]);
  };
  for(const auto& row:nodes)if(!fixed.count(row.first)) {
    Frame frame;frame.first=count;std::array<Vector,3> tangent{};const auto origin=row.second.p;
    for(unsigned axis=0;axis<3;++axis) {
      auto p=origin;if(axis==0)p.x=double(p.x+span);else if(axis==1)p.y=double(p.y+span);else p.z=double(p.z+span);
      auto v=Delta(ConstrainMovePoint(patch,row.first,{row.first,p},heights,reference).p,origin);
      for(unsigned pass=0;pass<2;++pass)for(size_t k=0;k<frame.count;++k) {
        const auto q=Dot(v,tangent[k]);for(unsigned j=0;j<3;++j)v[j]-=q*tangent[k][j];
      }
      const auto norm=Norm(v);if(norm<=256*std::numeric_limits<double>::epsilon()*span)continue;
      for(auto& x:v)x/=norm;tangent[frame.count++]=v;
    }
    if(!frame.count)continue;
    for(unsigned axis=0;axis<3 && frame.rank<3-frame.count;++axis) {
      Vector normal{};normal[axis]=1;
      for(unsigned pass=0;pass<2;++pass) {
        for(size_t k=0;k<frame.count;++k){const auto q=Dot(normal,tangent[k]);for(unsigned a=0;a<3;++a)normal[a]-=q*tangent[k][a];}
        for(size_t k=0;k<frame.rank;++k){const auto q=Dot(normal,frame.constraint[k]);for(unsigned a=0;a<3;++a)normal[a]-=q*frame.constraint[k][a];}
      }
      const auto norm=Norm(normal);if(norm>256*std::numeric_limits<double>::epsilon()){for(auto& x:normal)x/=norm;frame.constraint[frame.rank++]=normal;}
    }
    if(frame.rank!=3-frame.count)throw std::invalid_argument("Unresolved wall tangent complement.");
    frame.feet=feet[row.first];const auto metric=target.Query(origin).target;frame.metric=metric;
    for(size_t k=0;k<frame.count;++k) {
      auto v=tangent[k];for(unsigned pass=0;pass<2;++pass)for(size_t j=0;j<k;++j) {
        const auto q=metricDot(metric,v,frame.basis[j]);for(unsigned a=0;a<3;++a)v[a]-=q*frame.basis[j][a];
      }
      const auto norm=std::sqrt(metricDot(metric,v,v));if(!(norm>0) || !std::isfinite(norm))throw std::invalid_argument("Unresolved constrained metric solve frame.");
      for(auto& x:v)x/=norm;frame.basis[k]=v;
    }
    count+=frame.count;frames.emplace(row.first,frame);
  }
  if(!count)return {};
  if(count>12*MaximumReplacementCells)throw std::invalid_argument("Native3D metric solve exceeds bounded variables.");
  ++report.linearizations;std::vector<Row> rows;rows.reserve(2*patch.cells.size());
  for(const auto& cell:patch.cells)if(active.count(cell.id) && std::any_of(cell.v.begin(),cell.v.end(),[&](Node n){return frames.count(n.id);})) {
    const auto center=Centroid(cell);const auto metric=target.Query(center).target;
    const auto volume=Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p);
    long double altitude=INFINITY;
    for(unsigned opposite=0;opposite<4;++opposite) {
      std::array<Point,3> f{};unsigned at=0;for(unsigned k=0;k<4;++k)if(k!=opposite)f[at++]=cell.v[k].p;
      altitude=std::min(altitude,volume/Norm(Cross(Delta(f[1],f[0]),Delta(f[2],f[0]))));
    }
    std::array<Tensor,3> spatial{};
    for(unsigned axis=0;axis<3;++axis) {
      const auto value=axis==0?center.x:axis==1?center.y:center.z;
      const auto diagonal=axis==0?metric.xx:axis==1?metric.yy:metric.zz;
      const auto step=std::min(altitude/16,std::cbrt(std::numeric_limits<double>::epsilon())/std::sqrt(static_cast<long double>(diagonal)));
      if(!(step>8*std::numeric_limits<double>::epsilon()*std::max(span,std::abs(static_cast<long double>(value)))))
        throw std::invalid_argument("Native3D metric sensitivity step is not representable.");
      auto plus=center,minus=center;double high=double(value+step),low=double(value-step);
      if(axis==0){plus.x=high;minus.x=low;}else if(axis==1){plus.y=high;minus.y=low;}else{plus.z=high;minus.z=low;}
      ++report.metric_probes;const auto a=target.Query(plus).target;
      ++report.metric_probes;const auto b=target.Query(minus).target;
      const long double inverse=1/(static_cast<long double>(high)-low);
      spatial[axis]={double((a.xx-b.xx)*inverse),double((a.xy-b.xy)*inverse),double((a.xz-b.xz)*inverse),double((a.yy-b.yy)*inverse),double((a.yz-b.yz)*inverse),double((a.zz-b.zz)*inverse)};
    }
    const auto linear=LinearizeMetricShape({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},metric,&spatial);
    for(unsigned r=0;r<2;++r)if(linear.residual[r]>0) {
      Row row;row.residual=linear.residual[r];
      for(unsigned k=0;k<4;++k) {
        const auto found=frames.find(cell.v[k].id);if(found==frames.end())continue;const auto& frame=found->second;
        const auto& d=linear.derivative[r][k];const Vector gradient{d[0],d[1],d[2]};
        for(size_t j=0;j<frame.count;++j)row.entry[row.count++]={frame.first+j,Dot(gradient,frame.basis[j])};
      }
      rows.push_back(row);
    }
  }
  std::vector<long double> diagonal(count),gradient(count),step(count),residual(count),z(count),direction(count),product(count);
  for(const auto& row:rows)for(size_t k=0;k<row.count;++k) {
    const auto e=row.entry[k];diagonal[e.first]+=e.second*e.second;gradient[e.first]+=e.second*row.residual;
  }
  const auto maximum=*std::max_element(diagonal.begin(),diagonal.end());if(!(maximum>0))return {};
  auto regularizer=[&](size_t i){return 1e-3L*(diagonal[i]+maximum*1e-12L);};
  auto dot=[](const auto& a,const auto& b){long double result=0;for(size_t i=0;i<a.size();++i)result+=a[i]*b[i];return result;};
  for(size_t i=0;i<count;++i){residual[i]=-gradient[i];z[i]=residual[i]/(diagonal[i]+regularizer(i));direction[i]=z[i];}
  auto rz=dot(residual,z);const auto initial=dot(residual,residual);if(!(initial>0))return {};
  for(size_t iteration=0;iteration<count;++iteration) {
    for(size_t i=0;i<count;++i)product[i]=regularizer(i)*direction[i];
    for(const auto& row:rows) {
      long double value=0;for(size_t k=0;k<row.count;++k)value+=row.entry[k].second*direction[row.entry[k].first];
      for(size_t k=0;k<row.count;++k)product[row.entry[k].first]+=row.entry[k].second*value;
    }
    const auto denominator=dot(direction,product);if(!(denominator>0))break;
    const auto alpha=rz/denominator;++report.linear_iterations;
    for(size_t i=0;i<count;++i){step[i]+=alpha*direction[i];residual[i]-=alpha*product[i];}
    if(dot(residual,residual)<=1e-16L*initial)break;
    for(size_t i=0;i<count;++i)z[i]=residual[i]/(diagonal[i]+regularizer(i));
    const auto next=dot(residual,z);if(!(next>0))break;const auto beta=next/rz;rz=next;
    for(size_t i=0;i<count;++i)direction[i]=z[i]+beta*direction[i];
  }
  if(!(dot(step,gradient)<0))return {};
  long double largest=0;
  for(const auto& row:frames){long double norm=0;for(size_t k=0;k<row.second.count;++k)norm+=step[row.second.first+k]*step[row.second.first+k];largest=std::max(largest,std::sqrt(norm));}
  const auto scale=largest>.5L?.5L/largest:1.L;std::map<Id,Vector> movement;
  for(const auto& row:frames) {
    Vector delta{};for(size_t k=0;k<row.second.count;++k)for(unsigned a=0;a<3;++a)delta[a]+=scale*step[row.second.first+k]*row.second.basis[k][a];
    if(!std::isfinite(Norm(delta)))throw std::invalid_argument("Nonfinite coupled metric step.");
    if(!row.second.feet.empty()) {
      const auto origin=nodes.at(row.first).p;
      const auto fitted=FitNormalFeet({double(origin.x+delta[0]),double(origin.y+delta[1]),double(origin.z+delta[2])},row.second.constraint,row.second.rank,row.second.feet);
      delta=Delta(fitted,origin);
    }
    if(Norm(delta)>0)movement.emplace(row.first,delta);
  }
  largest=0;for(const auto& row:movement)largest=std::max(largest,std::sqrt(metricDot(frames.at(row.first).metric,row.second,row.second)));
  if(largest>.5L)for(auto& row:movement)for(auto& x:row.second)x*=.5L/largest;
  return movement;
}
}  // namespace

bool RepairMetricConnectivity(const Patch& old,FrozenField& target,const PlanarReference& reference,
                              const std::map<int,double>& heights,Id first,Patch& fresh,
                              ConnectivityRepairReport& report,std::string& reason,size_t maximum_trials,bool retire_points,const std::set<Id>* movable) {
  report={};
  try {
    if(old.cells.empty() || old.cells.size()>MaximumReplacementCells || maximum_trials>4096)
      throw std::invalid_argument("Native3D private connectivity repair exceeds its cell/trial bound.");
    const auto skin=SkinMap(Incidence(old.cells).Boundary());CheckSurface(old,skin,reference,nullptr);
    FirstHeightReport height;
    if(!CheckFirstHeight(old,heights,reference,height,reason))throw std::invalid_argument(reason);
    Patch working=old;
    std::map<std::array<Id,4>,double> losses;
    std::set<std::vector<Id>> rejected_states;
    auto loss=[&](const Cell& cell) {
      std::array<Id,4> key;for(unsigned i=0;i<4;++i)key[i]=cell.v[i].id;std::sort(key.begin(),key.end());
      const auto found=losses.find(key);if(found!=losses.end())return found->second;
      const auto m=Measure({cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p},target.Query(Centroid(cell)).target);
      const double q=std::max(0.,1-m.mean_ratio/MinimumMeanRatio),j=std::max(0.,1-m.minimum_scaled_jacobian/MinimumScaledJacobian);
      const auto value=q*q+j*j;losses.emplace(key,value);return value;
    };
    auto total=[&](const std::vector<Cell>& cells) {double value=0;for(const auto& c:cells)value+=loss(c);return value;};
    report.before_loss=report.after_loss=total(working.cells);
    while(report.after_loss>0 && report.trials<maximum_trials) {
      bool changed=false;
      const auto nodes=Nodes(working.cells);
      std::set<Id> fixed;for(const auto& f:skin)fixed.insert(f.second.begin(),f.second.end());
      if(movable)for(const auto& row:nodes)if(!movable->count(row.first))fixed.insert(row.first);
      std::map<Id,double> deficits;
      for(const auto& c:working.cells)for(auto n:c.v)if(!fixed.count(n.id))deficits[n.id]+=loss(c);
      std::vector<std::pair<double,Id>> order;
      for(const auto& row:deficits)if(row.second>0)order.emplace_back(-row.second,row.first);
      std::sort(order.begin(),order.end());
      if(!order.empty())try {
        std::set<Id> active;for(const auto& c:working.cells)if(loss(c)>0)active.insert(c.id);
        const auto direction=CoupledMetricDirection(working,target,heights,reference,fixed,active,report);
        if(!direction.empty())for(double fraction=1;fraction>std::numeric_limits<double>::epsilon() && report.trials<maximum_trials;fraction*=.5) {
          ++report.trials;++report.solve_trials;Patch proposal=working;size_t moved=0;std::map<Id,Point> points;
          for(const auto& row:direction) {
            const auto origin=nodes.at(row.first).p;const auto& d=row.second;
            const auto point=ConstrainMovePoint(working,row.first,{row.first,{double(origin.x+fraction*d[0]),double(origin.y+fraction*d[1]),double(origin.z+fraction*d[2])}},heights,reference).p;
            if(point.x!=origin.x || point.y!=origin.y || point.z!=origin.z){points.emplace(row.first,point);++moved;}
          }
          if(!moved)break;
          for(auto& c:proposal.cells)for(auto& n:c.v)if(points.count(n.id))n.p=points.at(n.id);
          std::string rejected;
          if(!ValidateFixedInterface(working.cells,proposal.cells,rejected,nullptr,MaximumReplacementCells)){++report.move_geometry_rejected;continue;}
          if(!CheckFirstHeight(proposal,heights,reference,height,rejected)){++report.move_height_rejected;continue;}
          losses.clear();double after=0;
          try{after=total(proposal.cells);}catch(const std::exception&){losses.clear();++report.move_loss_rejected;continue;}
          if(!(after==0 || after<report.after_loss*(1-1e-8)-1e-14)){losses.clear();++report.move_loss_rejected;continue;}
          working=std::move(proposal);losses.clear();rejected_states.clear();report.moves+=moved;++report.coordinated;
          report.after_loss=after;changed=true;break;
        }
      }catch(const std::exception&){++report.solve_rejected;}
      if(changed)continue;
      // Rebuilt wall rows can leave obsolete interior support points that flips/movement cannot eliminate.
      std::set<Id> height_apices;
      for(const auto& f:working.surface)if(heights.count(reference.Get(f.facet).marker))for(const auto& c:working.cells) {
        unsigned count=0;Id apex=0;for(auto n:c.v){if(Has(f.v,n.id))++count;else apex=n.id;}
        if(count==3)height_apices.insert(apex);
      }
      for(const auto& row:order) {
        if(!retire_points || changed || report.trials>=maximum_trials)break;
        const Id point=row.second;if(height_apices.count(point))continue;
        std::set<Id> neighbors;std::vector<size_t> selected;Patch star;
        for(size_t i=0;i<working.cells.size();++i)if(std::any_of(working.cells[i].v.begin(),working.cells[i].v.end(),[&](Node n){return n.id==point;})) {
          selected.push_back(i);star.cells.push_back(working.cells[i]);for(auto n:working.cells[i].v)if(n.id!=point)neighbors.insert(n.id);
        }
        if(star.cells.size()>MaximumCavityCells)continue;
        const auto boundary=SkinMap(Incidence(star.cells).Boundary());
        for(const auto& f:working.surface)if(boundary.count(Key(f.v)))star.surface.push_back(f);
        const auto before=total(star.cells);std::vector<std::pair<long double,Id>> ranked;
        const auto origin=nodes.at(point).p;
        for(Id neighbor:neighbors) {
          const auto p=nodes.at(neighbor).p;
          ranked.emplace_back(MetricNorm(origin,p,target.QueryMidpoint(origin,p).target),neighbor);
        }
        std::sort(ranked.begin(),ranked.end());
        for(const auto& destination:ranked) {
          if(report.trials>=maximum_trials)break;
          std::vector<Id> key{0,point,destination.second};std::vector<std::array<Id,4>> state;
          for(const auto& c:star.cells){std::array<Id,4> v;for(unsigned k=0;k<4;++k)v[k]=c.v[k].id;std::sort(v.begin(),v.end());state.push_back(v);}
          std::sort(state.begin(),state.end());for(const auto& v:state)key.insert(key.end(),v.begin(),v.end());
          if(!rejected_states.insert(key).second)continue;
          ++report.trials;Patch collapsed;std::string rejected;
          if(!CollapseVertex(star,point,destination.second,first,reference,collapsed,rejected))continue;
          const auto after=total(collapsed.cells);if(!(after<before*(1-1e-8)-1e-14))continue;
          Patch proposal;proposal.surface=working.surface;
          for(size_t i=0;i<working.cells.size();++i)if(std::find(selected.begin(),selected.end(),i)==selected.end())proposal.cells.push_back(working.cells[i]);
          proposal.cells.insert(proposal.cells.end(),collapsed.cells.begin(),collapsed.cells.end());
          if(proposal.cells.empty() || first>UINT64_MAX-(proposal.cells.size()-1))continue;
          for(size_t i=0;i<proposal.cells.size();++i)proposal.cells[i].id=first+i;
          if(!ValidateFixedInterface(working.cells,proposal.cells,rejected,nullptr,MaximumReplacementCells) ||
             !CheckFirstHeight(proposal,heights,reference,height,rejected))continue;
          working=std::move(proposal);rejected_states.clear();++report.removals;
          report.after_loss=total(working.cells);changed=true;break;
        }
      }
      if(changed)continue;
      std::map<Face,std::vector<size_t>> faces;
      std::map<Edge,std::vector<size_t>> edges;
      for(size_t i=0;i<working.cells.size();++i) {
        const auto& v=working.cells[i].v;
        for(auto f:std::array<Face,4>{{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},
                                      {v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}})faces[Key(f)].push_back(i);
        for(unsigned a=0;a<4;++a)for(unsigned b=a+1;b<4;++b) {
          Edge e{v[a].id,v[b].id};std::sort(e.begin(),e.end());edges[e].push_back(i);
        }
      }
      auto trial=[&](const std::vector<size_t>& selected,Id apex) {
        if(report.trials>=maximum_trials)return false;
        std::vector<Cell> region;for(auto i:selected)region.push_back(working.cells[i]);
        const auto before=total(region);if(before==0)return false;
        std::vector<std::array<Id,4>> state;
        for(const auto& c:region){std::array<Id,4> v;for(unsigned i=0;i<4;++i)v[i]=c.v[i].id;std::sort(v.begin(),v.end());state.push_back(v);}
        std::sort(state.begin(),state.end());std::vector<Id> key{Id(region.size()),apex};
        for(const auto& v:state)key.insert(key.end(),v.begin(),v.end());
        if(!rejected_states.insert(key).second)return false;
        ++report.trials;const auto nodes=Nodes(region);std::vector<Cell> replacement;std::string rejected;
        if(!Cone(region,nodes.at(apex),first,replacement,rejected))return false;
        if(!((region.size()+replacement.size()==5 && region.size()!=replacement.size()) ||
             (region.size()==4 && replacement.size()==4)))return false;
        const auto after=total(replacement);
        if(!(after<before*(1-1e-8)-1e-14))return false;
        Patch proposal;proposal.surface=working.surface;
        for(size_t i=0;i<working.cells.size();++i)
          if(std::find(selected.begin(),selected.end(),i)==selected.end())proposal.cells.push_back(working.cells[i]);
        proposal.cells.insert(proposal.cells.end(),replacement.begin(),replacement.end());
        if(proposal.cells.size()>MaximumReplacementCells || first>UINT64_MAX-(proposal.cells.size()-1))return false;
        for(size_t i=0;i<proposal.cells.size();++i)proposal.cells[i].id=first+i;
        if(!CheckFirstHeight(proposal,heights,reference,height,rejected))return false;
        rejected_states.erase(key);
        working=std::move(proposal);++report.flips;report.after_loss=total(working.cells);return true;
      };
      for(const auto& row:faces)if(row.second.size()==2) {
        const auto& c=working.cells[row.second[0]];
        const auto apex=std::find_if(c.v.begin(),c.v.end(),[&](Node n){return !Has(row.first,n.id);});
        if(apex!=c.v.end() && trial(row.second,apex->id)){changed=true;break;}
        if(report.trials>=maximum_trials)break;
      }
      if(!changed)for(const auto& row:edges)if(row.second.size()==3 || row.second.size()==4) {
        bool interface=false;for(const auto& face:skin)interface=interface || (Has(face.second,row.first[0]) && Has(face.second,row.first[1]));
        if(interface)continue;
        std::vector<Cell> region;for(auto i:row.second)region.push_back(working.cells[i]);const auto nodes=Nodes(region);
        for(const auto& apex:nodes)if(apex.first!=row.first[0] && apex.first!=row.first[1]) {
          if(trial(row.second,apex.first)){changed=true;break;}
          if(row.second.size()==3 || report.trials>=maximum_trials)break;
        }
        if(changed || report.trials>=maximum_trials)break;
      }
      if(!changed && report.trials<maximum_trials) {
        std::map<Id,std::vector<Point>> suggestions;
        // Coordinate all constrained interior points before demanding descent from an individual move.
        for(size_t at=0;at<std::min<size_t>(8,order.size());++at) {
          const Id id=order[at].second;const auto origin=nodes.at(id).p;std::set<Id> neighbors;
          for(const auto& c:working.cells)if(std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==id;}))for(auto n:c.v)if(n.id!=id)neighbors.insert(n.id);
          std::vector<std::pair<Point,Tensor>> samples;
          for(Id neighbor:neighbors) {
            const auto p=nodes.at(neighbor).p;
            samples.emplace_back(p,target.QueryMidpoint(origin,p).target);
          }
          auto& poses=suggestions[id];poses.push_back(MetricMeanPoint(origin,samples));
          const auto m=target.Query(origin).target;const std::array<double,3> steps{{.5/std::sqrt(m.xx),.5/std::sqrt(m.yy),.5/std::sqrt(m.zz)}};
          for(unsigned axis=0;axis<3;++axis)for(double sign:{-1.,1.}) {
            Point q=origin;if(axis==0)q.x+=sign*steps[axis];else if(axis==1)q.y+=sign*steps[axis];else q.z+=sign*steps[axis];poses.push_back(q);
          }
        }
        if(suggestions.size()>1)for(unsigned pose=0;pose<7 && !changed && report.trials<maximum_trials;++pose)for(double fraction:{1.,.5,.25}) {
          if(changed || report.trials>=maximum_trials)break;
          std::map<Id,Point> moved;
          for(const auto& row:suggestions) {
            const auto origin=nodes.at(row.first).p;const auto p=row.second[pose];
            Node proposal{row.first,{double(origin.x+fraction*((long double)p.x-origin.x)),double(origin.y+fraction*((long double)p.y-origin.y)),double(origin.z+fraction*((long double)p.z-origin.z))}};
            proposal=ConstrainMovePoint(working,row.first,proposal,heights,reference);
            if(proposal.p.x!=origin.x || proposal.p.y!=origin.y || proposal.p.z!=origin.z)moved.emplace(row.first,proposal.p);
          }
          if(moved.size()<2)continue;
          ++report.trials;Patch proposal=working;
          for(auto& c:proposal.cells)for(auto& n:c.v){const auto found=moved.find(n.id);if(found!=moved.end())n.p=found->second;}
          std::string rejected;
          if(!ValidateFixedInterface(old.cells,proposal.cells,rejected,nullptr,MaximumReplacementCells)){++report.move_geometry_rejected;continue;}
          if(!CheckFirstHeight(proposal,heights,reference,height,rejected)){++report.move_height_rejected;continue;}
          losses.clear();double after=0;
          try {after=total(proposal.cells);}catch(const std::exception&){losses.clear();++report.move_loss_rejected;continue;}
          if(!(after<report.after_loss*(1-1e-8)-1e-14)){losses.clear();++report.move_loss_rejected;continue;}
          working=std::move(proposal);losses.clear();rejected_states.clear();report.moves+=moved.size();++report.coordinated;
          report.after_loss=total(working.cells);changed=true;
        }
        for(const auto& row:order) {
          if(changed)break;
          const auto id=row.second;const auto origin=nodes.at(id).p;
          std::vector<size_t> selected;std::vector<Cell> region;std::set<Id> neighbors;
          for(size_t i=0;i<working.cells.size();++i)if(std::any_of(working.cells[i].v.begin(),working.cells[i].v.end(),[&](Node n){return n.id==id;})) {
            selected.push_back(i);region.push_back(working.cells[i]);for(auto n:working.cells[i].v)if(n.id!=id)neighbors.insert(n.id);
          }
          if(region.size()>MaximumCavityCells)continue;
          const auto before=total(region);std::vector<std::pair<Point,Tensor>> samples;
          for(Id neighbor:neighbors) {
            const auto p=nodes.at(neighbor).p;
            samples.emplace_back(p,target.QueryMidpoint(origin,p).target);
          }
          std::vector<Point> destinations{MetricMeanPoint(origin,samples)};
          const auto m=target.Query(origin).target;
          const std::array<double,3> step{{.5/std::sqrt(m.xx),.5/std::sqrt(m.yy),.5/std::sqrt(m.zz)}};
          for(unsigned axis=0;axis<3;++axis)for(double sign:{-1.,1.}) {
            Point p=origin;if(axis==0)p.x+=sign*step[axis];else if(axis==1)p.y+=sign*step[axis];else p.z+=sign*step[axis];
            destinations.push_back(p);
          }
          for(auto destination:destinations) {
            destination=ConstrainMovePoint(working,id,{id,destination},heights,reference).p;
          for(double fraction:{1.,.5,.25,.125,.0625}) {
            if(report.trials>=maximum_trials)break;
            const Node moved{id,{double(origin.x+fraction*((long double)destination.x-origin.x)),
                double(origin.y+fraction*((long double)destination.y-origin.y)),double(origin.z+fraction*((long double)destination.z-origin.z))}};
            if(moved.p.x==origin.x && moved.p.y==origin.y && moved.p.z==origin.z)continue;
            ++report.trials;std::vector<Cell> replacement;std::string rejected;
            if(!Cone(region,moved,first,replacement,rejected)){++report.move_geometry_rejected;continue;}
            // Shape keys contain identities, not coordinates: movement invalidates all such entries.
            losses.clear();double after=0;
            try {after=total(replacement);}catch(const std::exception&){losses.clear();++report.move_loss_rejected;continue;}
            if(!(after<before*(1-1e-8)-1e-14)){losses.clear();++report.move_loss_rejected;continue;}
            Patch proposal;proposal.surface=working.surface;
            for(size_t i=0;i<working.cells.size();++i)if(std::find(selected.begin(),selected.end(),i)==selected.end())proposal.cells.push_back(working.cells[i]);
            proposal.cells.insert(proposal.cells.end(),replacement.begin(),replacement.end());
            if(proposal.cells.size()>MaximumReplacementCells || first>UINT64_MAX-(proposal.cells.size()-1) ||
                !CheckFirstHeight(proposal,heights,reference,height,rejected)){losses.clear();++report.move_height_rejected;continue;}
            for(size_t i=0;i<proposal.cells.size();++i)proposal.cells[i].id=first+i;
            working=std::move(proposal);losses.clear();rejected_states.clear();++report.moves;
            report.after_loss=total(working.cells);changed=true;break;
          }
            if(changed || report.trials>=maximum_trials)break;
          }
          if(changed || report.trials>=maximum_trials)break;
        }
      }
      if(!changed)break;
    }
    report.complete_shape=report.after_loss==0;
    if(!report.complete_shape) {
      std::set<Id> physical,fixed;for(const auto& f:working.surface)physical.insert(f.v.begin(),f.v.end());
      for(const auto& f:skin)for(Id n:f.second)if(!physical.count(n))fixed.insert(n);
      std::map<Id,double> deficits;
      for(const auto& c:working.cells)for(auto n:c.v)if(fixed.count(n.id))deficits[n.id]+=loss(c);
      double maximum=0;for(const auto& row:deficits)if(row.second>maximum){maximum=row.second;report.blocked_interior_vertex=row.first;}
    }
    if(!report.flips && !report.moves && !report.removals)throw std::invalid_argument("Native3D admitted joint repair made no actual-field shape progress.");
    working.surface.clear();
    const auto physical=CheckSurface(old,skin,reference,nullptr);
    // Flip geometry has an unchanged interface; final reference/topology checks remain independent.
    std::string closure;
    if(!ValidatePrescribedInterface(old.cells,working.cells,SkinVector(skin),closure,nullptr,MaximumReplacementCells))throw std::invalid_argument(closure);
    for(const auto& f:physical)working.surface.push_back(f.second);
    CheckSurface(working,skin,reference,nullptr);
    fresh=std::move(working);reason.clear();return true;
  }catch(const std::exception& e){reason=e.what();return false;}
}
bool UndoEdgeSplit(const Patch& old, Id point, Edge edge, Id first, const PlanarReference& reference, Patch& fresh,
                   std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    Endpoints(edge);
    if (point == edge[0] || point == edge[1])
      throw std::invalid_argument("Native 3D inverse split repeats an endpoint.");
    const Incidence index(old.cells, stats);
    auto skin = SkinMap(index.Boundary());
    auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    const auto a = nodes.at(edge[0]), b = nodes.at(edge[1]), p = nodes.at(point);
    if (!OnEdge(a.p,b.p,p.p,stats)) {
      bool curved=false;
      for (const auto& f : physical) if (Has(f.second.v,point) && reference.Declared(f.second.facet)) {
        curved=true;
        if (!reference.OnProjectedEdge(f.second.facet,a.p,b.p,p.p,stats))
          throw std::invalid_argument("Native3D inverse split leaves its original projected edge.");
      }
      if (!curved) throw std::invalid_argument("Native 3D inverse split point is off its original edge.");
    }
    std::map<Edge, std::array<bool, 2>> pairs;
    for (const auto& c : old.cells) {
      bool has_p = false, has_a = false, has_b = false;
      Edge others{};
      size_t count = 0;
      for (auto n : c.v) {
        if (n.id == point)
          has_p = true;
        else if (n.id == edge[0])
          has_a = true;
        else if (n.id == edge[1])
          has_b = true;
        else {
          if (count >= 2) throw std::invalid_argument("Native 3D inverse split is not a paired vertex star.");
          others[count++] = n.id;
        }
      }
      if (!has_p || has_a == has_b || count != 2)
        throw std::invalid_argument("Native 3D inverse split is not a paired vertex star.");
      std::sort(others.begin(), others.end());
      auto& pair = pairs[others];
      if (pair[has_b]) throw std::invalid_argument("Native 3D inverse split repeats a child.");
      pair[has_b] = true;
    }
    Patch proposal;
    for (const auto& pair : pairs) {
      if (!(pair.second[0] && pair.second[1]))
        throw std::invalid_argument("Native 3D inverse split has a missing volume sibling.");
      Append(proposal.cells, {a, b, nodes.at(pair.first[0]), nodes.at(pair.first[1])}, first, stats);
    }
    struct SurfacePair {
      Id facet;
      Face face;
      bool side;
    };
    std::map<std::pair<Id, Id>, std::vector<SurfacePair>> surface_pairs;
    const auto before = skin;
    for (const auto& entry : before)
      if (Has(entry.second, point)) {
        const auto found = physical.find(entry.first);
        if (found == physical.end())
          throw std::invalid_argument("Native 3D incomplete inverse split dependency at interface node " +
                                      std::to_string(point) + ".");
        const auto f = entry.second;
        const bool has_a = Has(f, edge[0]), has_b = Has(f, edge[1]);
        if (has_a == has_b) throw std::invalid_argument("Native 3D inverse split has an unpaired physical face.");
        Id other = 0;
        for (auto id : f)
          if (id != point && id != edge[has_b]) other = id;
        const auto facet = found->second.facet;
        surface_pairs[{facet, other}].push_back({facet, f, has_b});
        physical.erase(found);
        skin.erase(entry.first);
      }
    for (const auto& entry : surface_pairs) {
      const auto& pair = entry.second;
      if (pair.size() != 2 || pair[0].side == pair[1].side)
        throw std::invalid_argument("Native 3D inverse split has a missing physical sibling.");
      auto parent = pair[0].face;
      for (auto& id : parent)
        if (id == point) id = edge[!pair[0].side];
      parent = Oriented(parent);
      const auto key = Key(parent);
      if (!skin.emplace(key, parent).second || !physical.emplace(key, SurfaceFace{parent, pair[0].facet}).second)
        throw std::invalid_argument("Native 3D inverse split repeats a parent face.");
    }
    Finish(old, proposal, skin, physical, reference, stats);
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool CollapseVertex(const Patch& old, Id point, Id neighbor, Id first, const PlanarReference& reference, Patch& fresh,
                    std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    if (point == neighbor) throw std::invalid_argument("Native 3D collapse repeats its endpoint.");
    const Incidence index(old.cells, stats);
    auto skin = SkinMap(index.Boundary());
    auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    const auto apex = nodes.at(neighbor);
    nodes.at(point);
    for (const auto& f : skin)
      if (Has(f.second, point)) {
        const auto marked = physical.find(f.first);
        if (marked == physical.end())
          throw std::invalid_argument("Native 3D incomplete collapse dependency at interface node " +
                                      std::to_string(point) + ".");
        if (!reference.Contains(marked->second.facet, apex.p, stats))
          throw std::invalid_argument("Native 3D collapse leaves incident reference facet " +
                                      std::to_string(marked->second.facet) + ".");
      }
    Patch proposal;
    size_t removed = 0;
    for (const auto& cell : old.cells) {
      auto v = cell.v;
      bool has_point = false, has_neighbor = false;
      for (auto& n : v) {
        has_neighbor = has_neighbor || n.id == neighbor;
        if (n.id == point) {
          has_point = true;
          n = apex;
        }
      }
      if (!has_point) throw std::invalid_argument("Native 3D collapse input is not a complete vertex star.");
      if (has_neighbor) {
        ++removed;
        continue;
      }
      if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p, stats) > 0))
        throw std::invalid_argument("Native 3D collapse obstructed at cell " + std::to_string(cell.id) + ".");
      Append(proposal.cells, v, first, stats);
    }
    if (!removed || proposal.cells.empty())
      throw std::invalid_argument("Native 3D collapse removes no cells or the entire volume patch.");
    const auto before = skin;
    for (const auto& entry : before)
      if (Has(entry.second, point)) {
        auto found = physical.find(entry.first);
        const auto facet = found->second.facet;
        physical.erase(found);
        skin.erase(entry.first);
        if (Has(entry.second, neighbor)) continue;
        auto face = entry.second;
        for (auto& id : face)
          if (id == point) id = neighbor;
        if (reference.Declared(facet)) {
          if (!reference.ContainsFace(facet,{nodes.at(face[0]).p,nodes.at(face[1]).p,nodes.at(face[2]).p},stats))
            throw std::invalid_argument("Native3D collapse folds or leaves its finite curved chart.");
        } else for (size_t axis = 0; axis < 3; ++axis) {
          const auto old_area = Projected(nodes.at(entry.second[0]).p, nodes.at(entry.second[1]).p,
                                          nodes.at(entry.second[2]).p, axis, stats);
          if (old_area == 0) continue;
          const auto area = Projected(nodes.at(face[0]).p, nodes.at(face[1]).p, nodes.at(face[2]).p, axis, stats);
          if ((old_area > 0 && !(area > 0)) || (old_area < 0 && !(area < 0)))
            throw std::invalid_argument("Native 3D collapse folds physical facet " + std::to_string(facet) + ".");
          break;
        }
        face = Oriented(face);
        if (!skin.emplace(Key(face), face).second || !physical.emplace(Key(face), SurfaceFace{face, facet}).second)
          throw std::invalid_argument("Native 3D collapse repeats a physical face.");
      }
    Finish(old, proposal, skin, physical, reference, stats);
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool MoveVertex(const Patch& old, Id vertex, Node point, Id first, const PlanarReference& reference, Patch& fresh,
                std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    const Incidence index(old.cells, stats);
    const auto before = SkinMap(index.Boundary());
    const auto physical = CheckSurface(old, before, reference, stats);
    const auto nodes = Nodes(old.cells);
    const auto original = nodes.at(vertex);
    if (nodes.count(point.id) || (original.p.x == point.p.x && original.p.y == point.p.y && original.p.z == point.p.z))
      throw std::invalid_argument("Native 3D movement needs a new identity and a distinct position.");
    point=ProjectMovePoint(vertex,point,before,physical,reference,stats);
    if (original.p.x==point.p.x && original.p.y==point.p.y && original.p.z==point.p.z)
      throw std::invalid_argument("Native3D movement projects back to its original position.");
    for (const auto& face : before)
      if (Has(face.second, vertex)) {
        const auto marked = physical.find(face.first);
        if (marked == physical.end())
          throw std::invalid_argument("Native 3D incomplete movement dependency at interface node " +
                                      std::to_string(vertex) + ".");
        if (!reference.Contains(marked->second.facet, point.p, stats))
          throw std::invalid_argument("Native 3D movement leaves incident reference facet " +
                                      std::to_string(marked->second.facet) + ".");
      }
    Patch proposal;
    for (const auto& cell : old.cells) {
      auto v = cell.v;
      bool found = false;
      for (auto& node : v)
        if (node.id == vertex) {
          found = true;
          node = point;
        }
      if (!found) throw std::invalid_argument("Native 3D movement input is not a complete vertex star.");
      if (!(Orientation(v[0].p, v[1].p, v[2].p, v[3].p, stats) > 0))
        throw std::invalid_argument("Native 3D movement obstructed at cell " + std::to_string(cell.id) + ".");
      Append(proposal.cells, v, first, stats);
    }
    auto moved_nodes = nodes;
    moved_nodes.emplace(point.id, point);
    Skin skin;
    Physical surface;
    for (const auto& entry : before) {
      auto face = entry.second;
      const bool moving = Has(face, vertex);
      if (moving)
        for (auto& id : face)
          if (id == vertex) id = point.id;
      const auto marked = physical.find(entry.first);
      if (moving && marked!=physical.end() && reference.Declared(marked->second.facet)) {
        if (!reference.ContainsFace(marked->second.facet,
              {moved_nodes.at(face[0]).p,moved_nodes.at(face[1]).p,moved_nodes.at(face[2]).p},stats))
          throw std::invalid_argument("Native3D movement folds or leaves its finite curved chart.");
      } else if (moving)
        for (size_t axis = 0; axis < 3; ++axis) {
          const auto old_area = Projected(nodes.at(entry.second[0]).p, nodes.at(entry.second[1]).p,
                                          nodes.at(entry.second[2]).p, axis, stats);
          if (old_area == 0) continue;
          const auto area =
              Projected(moved_nodes.at(face[0]).p, moved_nodes.at(face[1]).p, moved_nodes.at(face[2]).p, axis, stats);
          if ((old_area > 0 && !(area > 0)) || (old_area < 0 && !(area < 0)))
            throw std::invalid_argument("Native 3D movement folds physical face at node " + std::to_string(vertex) +
                                        ".");
          break;
        }
      face = Oriented(face);
      if (!skin.emplace(Key(face), face).second) throw std::invalid_argument("Native 3D movement repeats a skin face.");
      if (marked != physical.end()) surface.emplace(Key(face), SurfaceFace{face, marked->second.facet});
    }
    // Check both prescribed interface and complete chart-oriented physical skin before publishing the private proposal.
    Finish(old, proposal, skin, surface, reference, stats);
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
bool Reconnect(const Patch& old, Id vertex, Id first, const PlanarReference& reference, Patch& fresh,
               std::string& reason, KernelStats* stats) {
  try {
    Bounds(old);
    const auto skin = SkinMap(Incidence(old.cells, stats).Boundary());
    const auto physical = CheckSurface(old, skin, reference, stats);
    const auto nodes = Nodes(old.cells);
    Patch proposal;
    if (!Cone(old.cells, nodes.at(vertex), first, proposal.cells, reason, stats)) return false;
    Finish(old, proposal, skin, physical, reference, stats);
    fresh.cells.swap(proposal.cells);
    fresh.surface.swap(proposal.surface);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    reason = e.what();
    return false;
  }
}
}  // namespace SU2Native3D
