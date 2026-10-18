/*!
 * \file CNativeRegion3D.cpp
 * \brief Whole-region surface sampling and coupled wall/transition tetrahedral construction.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeRegion3D.hpp"
#include "../../include/adaptation/CNativeMetric3D.hpp"
#include "../../include/adaptation/CNativeMesh2D.hpp"
#include "Eigen/Dense"
#include <algorithm>
#include <deque>
#include <chrono>
#include <cmath>
#include <set>
#include <tuple>
#include <sstream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace SU2Native3D {
namespace {
Point Add(Point a, Point b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Point Sub(Point a, Point b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Point Mul(Point a, double b) { return {a.x * b, a.y * b, a.z * b}; }
double Dot(Point a, Point b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Point Cross(Point a, Point b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
double Norm(Point a) { return std::hypot(a.x, a.y, a.z); }
Point Unit(Point a) {
  const double n = Norm(a);
  if (!(n > 0)) throw std::runtime_error("Region has a degenerate normal.");
  return Mul(a, 1 / n);
}
Edge Key(Id a, Id b) { return a < b ? Edge{a, b} : Edge{b, a}; }
Face Key(Face f) {
  std::sort(f.begin(), f.end());
  return f;
}
constexpr std::array<std::array<unsigned,4>,3> PrismTets{{{0,1,2,5},{0,1,5,4},{0,4,5,3}}};
// Proposal-only tangential envelope uses the solver's stable sum-whitening construction; the 3D field stays frozen.
Eigen::Matrix2d CapIntersection(Eigen::Matrix2d a,const Eigen::Matrix2d& b) {
  if(a==b)return a;
  const double scale=std::max(a.cwiseAbs().maxCoeff(),b.cwiseAbs().maxCoeff());
  if(!(scale>0) || !a.allFinite() || !b.allFinite())throw std::runtime_error("Prismatic cap metric is not finite.");
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> sum(a/scale+b/scale);
  if(sum.info()!=Eigen::Success || !(sum.eigenvalues().minCoeff()>0))throw std::runtime_error("Prismatic cap metric sum is not SPD.");
  const Eigen::Matrix2d root=sum.eigenvectors()*sum.eigenvalues().cwiseSqrt().asDiagonal()*sum.eigenvectors().transpose(),
      inverse=sum.eigenvectors()*sum.eigenvalues().cwiseSqrt().cwiseInverse().asDiagonal()*sum.eigenvectors().transpose();
  Eigen::Matrix2d relative=inverse*(a/scale)*inverse;relative=.5*(relative+relative.transpose()).eval();
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> eig(relative);
  if(eig.info()!=Eigen::Success)throw std::runtime_error("Prismatic cap metric eigensolve failed.");
  const Eigen::Vector2d values=eig.eigenvalues().cwiseMax(Eigen::Vector2d::Ones()-eig.eigenvalues());
  Eigen::Matrix2d out=scale*root*eig.eigenvectors()*values.asDiagonal()*eig.eigenvectors().transpose()*root;
  out=.5*(out+out.transpose()).eval();if(!out.allFinite())throw std::runtime_error("Prismatic cap envelope overflows.");
  return out;
}
struct Use {
  size_t face;
  Id a, b;
};
using Edges = std::map<Edge, std::vector<Use>>;
class PhaseClock {
  using Clock = std::chrono::steady_clock;
  bool enabled;
  double* elapsed = nullptr;
  Clock::time_point start{};
 public:
  explicit PhaseClock(bool enabled) : enabled(enabled) {}
  void Set(double& next) {
    if (!enabled) return;
    const auto now = Clock::now();
    if (elapsed) *elapsed += std::chrono::duration<double>(now-start).count();
    elapsed = &next; start = now;
  }
  ~PhaseClock() { if (elapsed) *elapsed += std::chrono::duration<double>(Clock::now()-start).count(); }
};
struct Junction {
  Id node, facet;
  std::array<Id, 4> quad;
  std::vector<size_t> cells;
};
struct PrismaticChart {
  Id base=0,top=0;
  Point origin{},direction{};
  double gap=0;
};
bool PrismChart(const PlanarReference& geometry,const std::map<int,RegionWallPolicy>& policies,PrismaticChart& chart) {
  if(policies.empty() || !geometry.ExtensionComponents().empty() || geometry.Facets().empty())return false;
  const Facet* base=nullptr;
  for(const auto& row:geometry.Facets())if(policies.count(row.second.marker)){base=&row.second;break;}
  if(!base)return false;
  chart.base=geometry.Canonical(base->id);chart.origin=base->v[0].p;
  chart.direction=Mul(Unit(Cross(Sub(base->v[1].p,chart.origin),Sub(base->v[2].p,chart.origin))),-1);
  double scale=0;
  for(const auto& row:geometry.Facets())for(auto n:row.second.v) {
    scale=std::max(scale,Norm(Sub(n.p,chart.origin)));
    chart.gap=std::max(chart.gap,Dot(Sub(n.p,chart.origin),chart.direction));
  }
  const double roundoff=256*std::numeric_limits<double>::epsilon()*scale;
  if(!(chart.gap>roundoff))return false;
  Id top=UINT64_MAX;
  for(const auto& row:geometry.Facets()) {
    const auto& f=row.second;const auto normal=Unit(Cross(Sub(f.v[1].p,f.v[0].p),Sub(f.v[2].p,f.v[0].p)));
    const auto alignment=Dot(normal,chart.direction);
    bool lower=true,upper=true;
    for(auto n:f.v) {
      const auto z=Dot(Sub(n.p,chart.origin),chart.direction);
      lower=lower && std::abs(z)<=roundoff;upper=upper && std::abs(z-chart.gap)<=roundoff;
    }
    if(lower) {if(alignment>-.999999999999 || geometry.Canonical(f.id)!=chart.base)return false;}
    else if(upper) {
      const Id root=geometry.Canonical(f.id);
      if(alignment<.999999999999 || (top!=UINT64_MAX && top!=root))return false;
      top=root;
    }
    else if(std::abs(alignment)>256*std::numeric_limits<double>::epsilon() || policies.count(f.marker))return false;
  }
  if(top==UINT64_MAX)return false;chart.top=top;
  try {
    for(const auto& row:geometry.Facets())for(auto n:row.second.v) {
      const auto z=Dot(Sub(n.p,chart.origin),chart.direction);
      if(z < -roundoff || z>chart.gap+roundoff)return false;
      const auto lower=Sub(n.p,Mul(chart.direction,z)),upper=Add(n.p,Mul(chart.direction,chart.gap-z));
      if(!geometry.Contains(chart.base,lower) || !geometry.Contains(chart.top,upper))return false;
    }
    for(const auto& row:geometry.Facets())if(geometry.Canonical(row.first)==chart.base)
      for(auto n:row.second.v)if(!geometry.Contains(chart.top,Add(n.p,Mul(chart.direction,chart.gap))))return false;
  }catch(const std::exception&){return false;}
  return true;
}
class Construction {
 public:
  const PlanarReference& geometry;
  const std::map<int, RegionWallPolicy>& policy;
  const double tolerance;
  const RegionMetric& field;
  const RegionLimits& limits;
  RegionReport& report;
  std::map<Id, Point> nodes;
  std::vector<SurfaceFace> faces;
  std::map<Id, std::set<int>> tags;
  std::map<int, Id> caps;
  Id next_node, next_cell;
  Patch fresh;
  std::vector<Junction> junctions;
  std::map<Edge, long double> lengths;
  std::vector<Id> cylinder_grid;
  std::vector<double> cylinder_levels;
  size_t cylinder_angular = 0;
  Point cylinder_origin{}, cylinder_axis{}, cylinder_u{}, cylinder_v{};
  std::array<Id,2> cylinder_caps{UINT64_MAX,UINT64_MAX};
  Construction(const PlanarReference& g, const std::map<int, RegionWallPolicy>& p, double t, const RegionMetric& f,
               const RegionLimits& l, RegionReport& r, Id n, Id c)
      : geometry(g), policy(p), tolerance(t), field(f), limits(l), report(r), next_node(n), next_cell(c) {}
  void Check() const {
    if (limits.checkpoint) limits.checkpoint();
    if (fresh.cells.size() > limits.maximum_cells || nodes.size() > 4 * limits.maximum_cells ||
        fresh.cells.size() + report.surface_trials + report.junction_trials + report.core_trials + report.front_trials > limits.maximum_work) {
      report.exhausted_work = true;
      throw std::runtime_error("Region construction exceeds admitted cells/work.");
    }
  }
  Id New(Point p) {
    Check();
    if (next_node == UINT64_MAX) throw std::overflow_error("Region node IDs exhausted.");
    nodes.emplace(next_node, p);
    return next_node++;
  }
  bool Wall(Id f) const {
    if (!policy.empty()) return policy.count(geometry.Get(f).marker);
    const auto kind = geometry.Analytic().at(geometry.Canonical(f)).kind;
    return kind == AnalyticSurface::Kind::Cylinder || kind == AnalyticSurface::Kind::Sphere;
  }
  double Height(Id f) const { return policy.at(geometry.Get(f).marker).height; }
  std::array<Point, 3> Triangle(const SurfaceFace& f) const {
    return {nodes.at(f.v[0]), nodes.at(f.v[1]), nodes.at(f.v[2])};
  }
  Point Normal(const SurfaceFace& f) const {
    const auto p = Triangle(f);
    return Unit(Cross(Sub(p[1], p[0]), Sub(p[2], p[0])));
  }
  Point Center(const SurfaceFace& f) const {
    const auto p = Triangle(f);
    return Mul(Add(Add(p[0], p[1]), p[2]), 1. / 3);
  }
  Edges Links(const std::vector<SurfaceFace>& fs) const {
    Edges edges;
    for (size_t i = 0; i < fs.size(); ++i)
      for (unsigned k = 0; k < 3; ++k) {
        const Id a = fs[i].v[k], b = fs[i].v[(k + 1) % 3];
        edges[Key(a, b)].push_back({i, a, b});
      }
    return edges;
  }
  long double Length(Id a, Id b) {
    const auto k = Key(a, b);
    auto it = lengths.find(k);
    if (it == lengths.end()) it = lengths.emplace(k, field.edge(nodes.at(a), nodes.at(b))).first;
    return it->second;
  }
  Cell Tet(std::array<Id, 4> ids, Id id = 0) const {
    Cell c;
    c.id = id;
    for (unsigned k = 0; k < 4; ++k) c.v[k] = {ids[k], nodes.at(ids[k])};
    return c;
  }
  MetricMeasures Shape(const Cell& c) {
    return Measure({c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p}, field.point(Centroid(c)));
  }
  double Score(const std::vector<std::array<Id, 4>>& cells, bool measure_edges = true) {
    double score = INFINITY;
    for (auto ids : cells) {
      const auto c = Tet(ids);
      if (!(Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) > 0)) return -INFINITY;
      const auto m = Shape(c);
      score = std::min({score, m.mean_ratio / .22, m.minimum_scaled_jacobian / .055});
      if (measure_edges)
        for (unsigned a = 0; a < 4; ++a)
          for (unsigned b = a + 1; b < 4; ++b)
            score = std::min(score, double(.95 * MaximumMetricEdge / Length(ids[a], ids[b])));
    }
    return score;
  }
  Point Balanced(Point a,Point b) {
    const auto total=field.edge(a,b);
    if (!(total>0) || !std::isfinite(total)) throw std::runtime_error("Region transition has no finite metric span.");
    double lo=0,hi=1,t=.5;
    for (unsigned iteration=0;iteration<16;++iteration) {
      Check();const auto p=Add(Mul(a,1-t),Mul(b,t));const auto length=field.edge(a,p);
      if (std::abs(length-.5L*total)<=1e-10L*total) return p;
      if (length<.5L*total) lo=t;else hi=t;t=.5*(lo+hi);
    }
    return Add(Mul(a,1-t),Mul(b,t));
  }
  void Append(const std::array<Id, 4>& ids) {
    Check();
    if (next_cell == UINT64_MAX) throw std::overflow_error("Region cell IDs exhausted.");
    fresh.cells.push_back(Tet(ids, next_cell++));
    report.generated_cells = fresh.cells.size();
  }
  bool SurfaceValid(const SurfaceFace& f) {
    const auto p = Triangle(f);
    if (!geometry.ContainsFace(f.facet, p)) return false;
    const double tol =
        policy.count(geometry.Get(f.facet).marker) ? std::min(tolerance, .5 * Height(f.facet)) : tolerance;
    return geometry.SurfaceError(f.facet, p) <= tol;
  }
  bool SampleCylinder() {
    Id wall=UINT64_MAX; std::map<int,Id> cap;
    for (auto f:faces) {
      if (Wall(f.facet)) wall=geometry.Canonical(f.facet);
      else cap.emplace(geometry.Get(f.facet).marker,f.facet);
    }
    if (wall==UINT64_MAX || geometry.Analytic().at(wall).kind!=AnalyticSurface::Kind::Cylinder) return false;
    const auto g=geometry.Analytic().at(wall);const double radius=g.extent.x,height=g.extent.y;
    if (!(radius>0 && height>0) || cap.size()!=2) throw std::runtime_error("Cylindrical front needs two admitted caps.");
    const Point axis=Unit(g.axis);std::array<Id,2> ends{UINT64_MAX,UINT64_MAX};
    for (auto f:cap) for (unsigned k=0;k<2;++k) if (geometry.Contains(f.second,Add(g.origin,Mul(axis,k*height)))) {
      if (ends[k]!=UINT64_MAX) throw std::runtime_error("Cylindrical front has ambiguous cap authority.");
      ends[k]=f.second;
    }
    if (ends[0]==UINT64_MAX || ends[1]==UINT64_MAX || ends[0]==ends[1])
      throw std::runtime_error("Cylindrical front caps do not close its declared axial interval.");
    Point radial{};
    for (auto f:faces) if (Wall(f.facet)) {radial=Sub(nodes.at(f.v[0]),g.origin);break;}
    const Point u=Unit(Sub(radial,Mul(axis,Dot(radial,axis)))),v=Cross(axis,u);
    const double pi=std::acos(-1.),error=policy.empty()?tolerance:std::min(tolerance,.5*Height(wall));
    const double half=std::acos(std::max(-1.,1-std::min(error/radius,1.)));
    if (!(half>0)) throw std::runtime_error("Cylindrical front curvature is unresolved in binary64.");
    const double curvature=std::ceil(pi/half);
    if (!std::isfinite(curvature) || curvature>static_cast<long double>(limits.maximum_work) || limits.maximum_work<3) {
      report.exhausted_work=true;throw std::runtime_error("Cylindrical curvature sampling exceeds admitted work.");
    }
    size_t angular=std::max(size_t(3),size_t(curvature)),axial=1;
    double around=0,along=0;
    for (size_t i=0;i<angular;++i) for (unsigned k=0;k<3;++k) {
      ++report.surface_trials;Check();
      const double a=2*pi*i/angular;const Point r=Add(Mul(u,std::cos(a)),Mul(v,std::sin(a))),t=Add(Mul(u,-std::sin(a)),Mul(v,std::cos(a)));
      const Point p=Add(Add(g.origin,Mul(axis,.5*k*height)),Mul(r,radius));const auto m=field.point(p);
      around=std::max(around,double(MetricNorm({},Mul(t,2*pi*radius),m)));
      along=std::max(along,double(MetricNorm({},Mul(axis,height),m)));
    }
    const double aim=.9*MaximumMetricEdge;
    if (!std::isfinite(around+along) || std::max(around,along)>aim*limits.maximum_work)
      throw std::runtime_error("Cylindrical front metric demand exceeds admitted sampling work.");
    auto count=[&](double demand) {
      const double n=std::ceil(demand/aim);
      if (!std::isfinite(n) || n>static_cast<long double>(limits.maximum_work)) {
        report.exhausted_work=true;throw std::runtime_error("Cylindrical metric sampling exceeds admitted work.");
      }
      return size_t(n);
    };
    angular=std::max(angular,count(around));axial=std::max(axial,count(along));
    for (;;) {
      Check();
      if (angular>limits.maximum_work/(axial+1)) {report.exhausted_work=true;throw std::runtime_error("Cylindrical front sampling exceeds admitted work.");}
      nodes.clear();faces.clear();tags.clear();lengths.clear();caps=cap;
      auto position=[&](size_t i,double z) {
        const double a=2*pi*i/angular;
        Point p=Add(Add(g.origin,Mul(axis,z)),Mul(Add(Mul(u,std::cos(a)),Mul(v,std::sin(a))),radius));
        if (z==0 || z==height) p=geometry.Project(ends[z==height],p);
        return p;
      };
      auto cumulative=[&](double z) {
        long double length=0;
        for (size_t i=0;i<angular;++i) {Check();length+=field.edge(position(i,0),position(i,z));}
        return length;
      };
      const long double total=cumulative(height);
      if (!(total>0) || !std::isfinite(total)) throw std::runtime_error("Cylindrical front has no finite metric span.");
      std::vector<double> levels(axial+1);levels.back()=height;
      for (size_t j=1;j<axial;++j) {
        ++report.surface_trials;Check();const long double target=total*j/axial;
        double lo=0,hi=height,z=height*j/axial;
        for (unsigned iteration=0;iteration<16;++iteration) {
          const auto value=cumulative(z);
          if (std::abs(value-target)<=1e-10L*total) break;
          if (value<target) lo=z;else hi=z;
          z=.5*(lo+hi);
        }
        if (!(z>levels[j-1] && z<height)) throw std::runtime_error("Cylindrical metric front has unrepresentable axial spacing.");
        levels[j]=z;
      }
      std::vector<Id> ids(angular*(axial+1));
      for (size_t j=0;j<=axial;++j) for (size_t i=0;i<angular;++i) {
        ++report.surface_trials;Check();
        ids[j*angular+i]=New(position(i,levels[j]));
        if (j==0 || j==axial) tags[ids[j*angular+i]].insert(geometry.Get(ends[j==axial]).marker);
      }
      bool valid=true,geometry_failure=false;long double azimuth=0,longitudinal=0;
      for (size_t j=0;j<axial;++j) for (size_t i=0;i<angular;++i) {
        const Id a=ids[j*angular+i],b=ids[j*angular+(i+1)%angular],c=ids[(j+1)*angular+(i+1)%angular],d=ids[(j+1)*angular+i];
        azimuth=std::max({azimuth,Length(a,b),Length(d,c)});longitudinal=std::max({longitudinal,Length(a,d),Length(b,c)});
        const Face choices[2][2]={{Face{a,b,c},Face{a,c,d}},{Face{a,b,d},Face{b,c,d}}};
        double best=-INFINITY;unsigned selected=0;
        for (unsigned choice=0;choice<2;++choice) {
          double score=INFINITY;
          for (auto f:choices[choice]) {
            const SurfaceFace face{f,wall};if (!SurfaceValid(face)) {geometry_failure=true;score=-INFINITY;break;}
            for (unsigned k=0;k<3;++k) score=std::min(score,double(MaximumMetricEdge/Length(f[k],f[(k+1)%3])));
            if (!policy.empty()) {
              const Point p=Sub(Center(face),Mul(Normal(face),Height(wall)));const auto m=Measure({p,nodes.at(f[0]),nodes.at(f[1]),nodes.at(f[2])},field.point(Mul(Add(Add(p,nodes.at(f[0])),Add(nodes.at(f[1]),nodes.at(f[2]))),.25)));
              score=std::min({score,m.mean_ratio/MinimumMeanRatio,m.minimum_scaled_jacobian/MinimumScaledJacobian});
            }
          }
          if (score>best) {best=score;selected=choice;}
        }
        valid=valid && best>=1;
        for (auto f:choices[selected]) faces.push_back({f,wall});
      }
      if (valid) {
        cylinder_grid=ids; cylinder_levels=levels; cylinder_angular=angular;
        cylinder_origin=g.origin; cylinder_axis=axis; cylinder_u=u; cylinder_v=v; cylinder_caps=ends;
        for (unsigned end=0;end<2;++end) {
          const Id center=New(geometry.Project(ends[end],Add(g.origin,Mul(axis,end*height))));
          for (size_t i=0;i<angular;++i) {
            Face f{center,ids[end*axial*angular+i],ids[end*axial*angular+(i+1)%angular]};
            if (!end) std::swap(f[1],f[2]);faces.push_back({f,ends[end]});
          }
        }
        return true;
      }
      // Refine the whole sampling direction, rather than retaining edge-split diagonal points.
      if (geometry_failure || azimuth>longitudinal) ++angular;else ++axial;
    }
  }
  bool SampleSphere() {
    if (faces.empty()) return false;
    const Id wall=geometry.Canonical(faces.front().facet);
    const auto found=geometry.Analytic().find(wall);
    if (found==geometry.Analytic().end() || found->second.kind!=AnalyticSurface::Kind::Sphere) return false;
    for (auto f:faces) if (!Wall(f.facet) || geometry.Canonical(f.facet)!=wall) return false;
    const auto g=found->second;
    if (!(Orientation(g.origin,geometry.Get(wall).v[0].p,geometry.Get(wall).v[1].p,geometry.Get(wall).v[2].p)>0))
      throw std::runtime_error("Spherical region requires an outward complete wall around its kernel.");
    nodes.clear();faces.clear();tags.clear();lengths.clear();caps.clear();
    auto site=[&](Point direction) {
      ++report.surface_trials;Check();
      return New(geometry.Project(wall,Add(g.origin,Mul(Unit(direction),g.extent.x))));
    };
    const double golden=.5*(1+std::sqrt(5.));
    std::vector<Id> vertices;
    for(double a:{-1.,1.}) for(double b:{-golden,golden}) {
      vertices.push_back(site({a,b,0}));vertices.push_back(site({0,a,b}));vertices.push_back(site({b,0,a}));
    }
    for(size_t i=0;i<vertices.size();++i) for(size_t j=i+1;j<vertices.size();++j) for(size_t k=j+1;k<vertices.size();++k) {
      const auto a=nodes.at(vertices[i]),b=nodes.at(vertices[j]),c=nodes.at(vertices[k]);
      bool positive=false,negative=false;
      for(size_t l=0;l<vertices.size();++l) if(l!=i && l!=j && l!=k) {
        const auto side=Orientation(a,b,c,nodes.at(vertices[l]));positive=positive || side>0;negative=negative || side<0;
      }
      if(positive && negative)continue;
      Face face{vertices[i],vertices[j],vertices[k]};
      const auto sign=Orientation(g.origin,a,b,c);
      if(!sign)throw std::runtime_error("Spherical sampling frame is not representable.");
      if(sign<0)std::swap(face[1],face[2]);faces.push_back({face,wall});
    }
    if(faces.size()!=20)throw std::runtime_error("Spherical sampling frame has an unresolved convex hull.");
    for(;;) {
      Check();bool valid=true;
      for(const auto& face:faces) {
        if(!SurfaceValid(face)){valid=false;break;}
        if(!policy.empty()) {
          const Point apex=Sub(Center(face),Mul(Normal(face),Height(wall)));
          const auto p=Triangle(face);const auto center=Mul(Add(Add(apex,p[0]),Add(p[1],p[2])),.25);
          const auto shape=Measure({apex,p[0],p[1],p[2]},field.point(center));
          if(shape.mean_ratio<MinimumMeanRatio || shape.minimum_scaled_jacobian<MinimumScaledJacobian){valid=false;break;}
        }
      }
      if(valid)for(const auto& edge:Links(faces))if(Length(edge.first[0],edge.first[1])>MaximumMetricEdge){valid=false;break;}
      if(valid)return true;
      // shortcut: uniform geodesic proposals; localized/strongly anisotropic surface demands need a surface remesher.
      const size_t factor=policy.empty()?1:8;
      if(faces.size()>limits.maximum_cells/(4*factor) ||
          faces.size()>limits.maximum_work/(4*factor) ||
          4*factor*faces.size()>limits.maximum_work-report.surface_trials) {
        report.exhausted_work=true;throw std::runtime_error("Spherical sampling and its mandatory band exceed admitted work/cells.");
      }
      std::map<Edge,Id> midpoints;std::vector<SurfaceFace> next;next.reserve(4*faces.size());
      auto middle=[&](Id a,Id b) {
        const auto edge=Key(a,b);const auto hit=midpoints.find(edge);if(hit!=midpoints.end())return hit->second;
        const auto direction=Sub(Mul(Add(nodes.at(a),nodes.at(b)),.5),g.origin);
        const Id id=site(direction);midpoints.emplace(edge,id);++report.surface_splits;return id;
      };
      for(auto f:faces) {
        const auto a=f.v[0],b=f.v[1],c=f.v[2],ab=middle(a,b),bc=middle(b,c),ca=middle(c,a);
        for(auto face:std::array<Face,4>{{{a,ab,ca},{ab,b,bc},{ca,bc,c},{ab,bc,ca}}})next.push_back({face,wall});
      }
      faces.swap(next);
    }
  }
  void SplitSurface() {
    // Planar cap interiors are rebuilt; close the radial sampling over all new edges.
    for (;;) {
      Check();
      const auto edges = Links(faces);
      std::set<Edge> marked;
      for (const auto& row : edges) {
        bool radial = false;
        for (auto use : row.second) radial = radial || Wall(faces[use.face].facet);
        if (radial && Length(row.first[0], row.first[1]) > MaximumMetricEdge) marked.insert(row.first);
      }
      for (const auto& face : faces)
        if (Wall(face.facet) && !SurfaceValid(face)) {
          Edge longest = Key(face.v[0], face.v[1]);
          for (unsigned k = 1; k < 3; ++k) {
            const auto edge = Key(face.v[k], face.v[(k + 1) % 3]);
            if (Length(edge[0], edge[1]) > Length(longest[0], longest[1])) longest = edge;
          }
          marked.insert(longest);
        }
      if (marked.empty()) return;
      for (auto e : marked) {
        Check();
        ++report.surface_trials;
        const auto uses = Links(faces).at(e);
        if (uses.size() != 2) throw std::runtime_error("Region surface is not closed.");
        Point p = Mul(Add(nodes.at(e[0]), nodes.at(e[1])), .5);
        std::vector<Id> charts;
        for (const auto& u : uses) charts.push_back(faces[u.face].facet);
        std::stable_sort(charts.begin(), charts.end(), [&](Id a, Id b) {
          return geometry.Analytic().at(geometry.Canonical(a)).kind == AnalyticSurface::Kind::Disk &&
                 geometry.Analytic().at(geometry.Canonical(b)).kind != AnalyticSurface::Kind::Disk;
        });
        for (Id f : charts) p = geometry.Project(f, p);
        for (Id f : charts)
          if (!geometry.Contains(f, p)) throw std::runtime_error("Region split leaves its declared feature.");
        const Id mid = New(p);
        std::vector<SurfaceFace> next;
        for (size_t i = 0; i < faces.size(); ++i) {
          bool hit = false;
          for (auto u : uses)
            if (u.face == i) {
              hit = true;
              Id c = 0;
              for (Id n : faces[i].v)
                if (n != e[0] && n != e[1]) c = n;
              next.push_back({{u.a, mid, c}, faces[i].facet});
              next.push_back({{mid, u.b, c}, faces[i].facet});
            }
          if (!hit) next.push_back(faces[i]);
        }
        faces.swap(next);
        ++report.surface_splits;
      }
    }
  }
  void CoarsenSurface(Point kernel) {
    std::vector<SurfaceFace> wall;
    for (auto f : faces)
      if (Wall(f.facet))
        wall.push_back(f);
      else {
        const int m = geometry.Get(f.facet).marker;
        auto c = caps.emplace(m, f.facet);
        c.first->second = std::min(c.first->second, f.facet);
        for (Id n : f.v) tags[n].insert(m);
      }
    std::map<Id, size_t> versions;
    std::map<std::pair<Id, Id>, std::pair<size_t, size_t>> blocked;
    for (;;) {
      Check();
      const auto edges = Links(wall);
      std::map<Id, std::set<Id>> neighbors;
      std::vector<std::pair<long double, Edge>> order;
      for (const auto& e : edges) {
        neighbors[e.first[0]].insert(e.first[1]);
        neighbors[e.first[1]].insert(e.first[0]);
        const auto l = Length(e.first[0], e.first[1]);
        if (l < .9) order.push_back({l, e.first});
      }
      std::sort(order.begin(), order.end());
      bool changed = false;
      for (const auto& entry : order) {
        for (unsigned reverse = 0; reverse < 2; ++reverse) {
          const Id removed = entry.second[reverse], retained = entry.second[1 - reverse];
          if (!std::includes(tags[retained].begin(), tags[retained].end(), tags[removed].begin(), tags[removed].end()))
            continue;
          if (!tags[removed].empty() && !tags[retained].empty() && edges.at(entry.second).size() != 1) continue;
          std::set<Id> opposite, shared;
          for (auto u : edges.at(entry.second))
            for (Id n : wall[u.face].v)
              if (n != removed && n != retained) opposite.insert(n);
          std::set_intersection(neighbors[removed].begin(), neighbors[removed].end(), neighbors[retained].begin(),
                                neighbors[retained].end(), std::inserter(shared, shared.end()));
          if (opposite != shared) continue;
          const auto directed = std::make_pair(removed, retained);
          const auto stamp = std::make_pair(versions[removed], versions[retained]);
          const auto rejected = blocked.find(directed);
          if (rejected != blocked.end() && rejected->second == stamp) continue;
          blocked[directed] = stamp;
          ++report.surface_trials;
          Check();
          std::vector<SurfaceFace> next;
          std::set<Edge> old_edges;
          for (auto f : wall)
            if (std::find(f.v.begin(), f.v.end(), removed) != f.v.end())
              for (unsigned k = 0; k < 3; ++k) old_edges.insert(Key(f.v[k], f.v[(k + 1) % 3]));
          bool valid = true;
          for (auto f : wall) {
            if (std::find(f.v.begin(), f.v.end(), removed) == f.v.end()) {
              next.push_back(f);
              continue;
            }
            for (auto& n : f.v)
              if (n == removed) n = retained;
            if (Key(f.v)[0] == Key(f.v)[1] || Key(f.v)[1] == Key(f.v)[2]) continue;
            if (!SurfaceValid(f)) {
              valid = false;
              break;
            }
            for (unsigned k = 0; k < 3; ++k)
              if (!old_edges.count(Key(f.v[k], f.v[(k + 1) % 3])) &&
                  Length(f.v[k], f.v[(k + 1) % 3]) > .95 * MaximumMetricEdge)
                valid = false;
            if (!valid) break;
            const Point apex = policy.empty() ? kernel : Sub(Center(f), Mul(Normal(f), Height(f.facet)));
            Cell c;
            c.v[0] = {UINT64_MAX, apex};
            for (unsigned k = 0; k < 3; ++k) c.v[k + 1] = {f.v[k], nodes.at(f.v[k])};
            const auto shape = Shape(c);
            if (shape.mean_ratio < .24 || shape.minimum_scaled_jacobian < .06) {
              valid = false;
              break;
            }
            next.push_back(f);
          }
          if (!valid) continue;
          std::set<Face> keys;
          for (auto f : next)
            if (!keys.insert(Key(f.v)).second) valid = false;
          if (!valid) continue;
          std::set<Id> changed_nodes{retained};
          for (const auto& face : wall)
            if (std::find(face.v.begin(), face.v.end(), removed) != face.v.end())
              changed_nodes.insert(face.v.begin(), face.v.end());
          for (Id n : changed_nodes) ++versions[n];
          wall.swap(next);
          ++report.surface_collapses;
          changed = true;
          break;
        }
        if (changed) break;
      }
      if (!changed) break;
    }
    faces = wall;
    std::map<int, std::vector<std::pair<Id, Id>>> loops;
    for (const auto& row : Links(wall)) {
      if (row.second.size() > 2) throw std::runtime_error("Region wall is not manifold.");
      if (row.second.size() == 1) {
        std::vector<int> shared;
        std::set_intersection(tags[row.first[0]].begin(), tags[row.first[0]].end(), tags[row.first[1]].begin(),
                              tags[row.first[1]].end(), std::back_inserter(shared));
        if (shared.size() != 1) throw std::runtime_error("Region rim lost its unique cap.");
        const auto use = row.second.front();
        loops[shared.front()].push_back({use.b, use.a});
      }
    }
    for (const auto& loop : loops) {
      const Id facet = caps.at(loop.first), center = New(geometry.Project(facet, kernel));
      VerifyLoop(loop.second);
      for (auto e : loop.second) faces.push_back({{center, e.first, e.second}, facet});
    }
  }
  void VerifyLoop(const std::vector<std::pair<Id, Id>>& edges) const {
    std::map<Id, Id> next;
    std::set<Id> incoming;
    for (auto e : edges)
      if (!next.emplace(e.first, e.second).second || !incoming.insert(e.second).second)
        throw std::runtime_error("Region cap has branched rims.");
    if (next.empty()) throw std::runtime_error("Region cap has an empty rim.");
    Id at = next.begin()->first;
    std::set<Id> walked;
    for (size_t k = 0; k < edges.size(); ++k) {
      if (!walked.insert(at).second || !next.count(at)) throw std::runtime_error("Region cap is not one closed loop.");
      at = next.at(at);
    }
    if (at != next.begin()->first) throw std::runtime_error("Region cap is open.");
  }
  struct Flow {
    Eigen::Matrix<double, 3, 4> displacement;
    Point center;
    double scale;
    Point Apply(Point p, double step = 1) const {
      const Point q = Mul(Sub(p, center), 1 / scale);
      const Eigen::Vector3d d = displacement * Eigen::Vector4d(q.x, q.y, q.z, 1);
      return Add(p, {step * d[0], step * d[1], step * d[2]});
    }
  };
  Flow Fit(const std::vector<SurfaceFace>& fs, double depth) {
    std::set<Id> ids;
    for (auto f : fs)
      for (Id n : f.v) ids.insert(n);
    Point center{};
    for (Id n : ids) center = Add(center, nodes.at(n));
    center = Mul(center, 1. / ids.size());
    double scale = 0;
    for (Id n : ids) scale = std::max(scale, Norm(Sub(nodes.at(n), center)));
    if (!(scale > 0)) throw std::runtime_error("Region has no scale.");
    std::vector<Eigen::Matrix<double, 1, 12>> rows, constraints;
    std::vector<double> rhs;
    for (auto f : fs) {
      const auto p = Triangle(f);
      const auto n = Normal(f);
      const double weight = std::sqrt(Norm(Cross(Sub(p[1], p[0]), Sub(p[2], p[0]))));
      for (Point x : p) {
        const Point q = Mul(Sub(x, center), 1 / scale);
        Eigen::Matrix<double, 1, 12> row;
        const double a[3] = {n.x, n.y, n.z}, b[4] = {q.x, q.y, q.z, 1};
        for (unsigned i = 0; i < 3; ++i)
          for (unsigned j = 0; j < 4; ++j) row[4 * i + j] = a[i] * b[j];
        if (Wall(f.facet)) {
          rows.push_back(weight * row);
          rhs.push_back(-weight * depth);
        } else
          constraints.push_back(row);
      }
    }
    Eigen::MatrixXd c(constraints.size(), 12), a(rows.size(), 12);
    Eigen::VectorXd b(rhs.size());
    for (size_t i = 0; i < constraints.size(); ++i) c.row(i) = constraints[i];
    for (size_t i = 0; i < rows.size(); ++i) {
      a.row(i) = rows[i];
      b[i] = rhs[i];
    }
    Eigen::MatrixXd null = Eigen::MatrixXd::Identity(12, 12);
    if (c.rows()) {
      Eigen::JacobiSVD<Eigen::MatrixXd> svd(c, Eigen::ComputeFullV);
      svd.setThreshold(1e-11);
      null = svd.matrixV().rightCols(12 - svd.rank());
    }
    if (!null.cols()) throw std::runtime_error("Region cap constraints block the inward flow.");
    const Eigen::VectorXd coefficients =
        null * (a * null).jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b);
    Flow flow;
    flow.center = center;
    flow.scale = scale;
    for (unsigned i = 0; i < 3; ++i)
      for (unsigned j = 0; j < 4; ++j) flow.displacement(i, j) = coefficients[4 * i + j];
    if (c.rows() && (c * coefficients).lpNorm<Eigen::Infinity>() > 1e-11 * depth)
      throw std::runtime_error("Region flow fails planar constraints.");
    if ((Eigen::Matrix3d::Identity() + flow.displacement.leftCols<3>() / scale).determinant() <= 0)
      throw std::runtime_error("Region inward flow folds.");
    return flow;
  }
  std::map<Id, Id> MapFront(const std::vector<SurfaceFace>& fs, const Flow& flow, double step) {
    std::set<Id> ids;
    std::map<Id, std::set<Id>> charts;
    for (auto f : fs)
      for (Id n : f.v) {
        ids.insert(n);
        if (!Wall(f.facet)) charts[n].insert(f.facet);
      }
    std::map<Id, Id> result;
    for (Id n : ids) {
      Point p = flow.Apply(nodes.at(n), step);
      for (Id f : charts[n]) p = geometry.Project(f, p);
      result[n] = New(p);
    }
    return result;
  }
  void Band(const std::vector<SurfaceFace>& outer, const std::map<Id, Id>& inner, const std::vector<Id>& apices,
            const std::map<Edge, Id>& side, bool physical) {
    for (size_t i = 0; i < outer.size(); ++i) {
      const auto f = outer[i];
      const auto a = f.v;
      Append({apices[i], a[0], a[1], a[2]});
      Append({apices[i], inner.at(a[2]), inner.at(a[1]), inner.at(a[0])});
      if (physical) fresh.surface.push_back(f);
    }
    for (const auto& entry : Links(outer)) {
      Check();
      const auto uses = entry.second;
      const auto u = uses.front();
      const Id p = apices[u.face], a = u.a, b = u.b;
      const std::array<Id, 4> q{a, inner.at(a), inner.at(b), b};
      if (uses.size() == 2) {
        const auto v = uses[1];
        if (v.a != b || v.b != a) throw std::runtime_error("Region wall edge orientations disagree.");
        const Id other = apices[v.face];
        std::vector<std::vector<std::array<Id, 4>>> options(3);
        for (unsigned k = 0; k < 4; ++k) options[0].push_back({p, other, q[k], q[(k + 1) % 4]});
        const unsigned diagonals[2][2][3] = {{{0, 1, 2}, {0, 2, 3}}, {{0, 1, 3}, {1, 2, 3}}};
        for (unsigned d = 0; d < 2; ++d)
          for (const auto& tri : diagonals[d]) {
            options[d + 1].push_back({p, q[tri[0]], q[tri[1]], q[tri[2]]});
          }
        for (unsigned d = 0; d < 2; ++d)
          for (const auto& tri : diagonals[d]) {
            options[d + 1].push_back({other, q[tri[2]], q[tri[1]], q[tri[0]]});
          }
        double best = -INFINITY;
        size_t choice = 0;
        for (size_t i = 0; i < options.size(); ++i) {
          const double s = Score(options[i]);
          if (s > best) {
            best = s;
            choice = i;
          }
        }
        if (!std::isfinite(best)) throw std::runtime_error("Region shared interface has no positive triangulation.");
        for (auto t : options[choice]) Append(t);
      } else {
        if (uses.size() != 1 || !side.count(entry.first)) throw std::runtime_error("Region wall rim is not admitted.");
        const Id facet = side.at(entry.first);
        Point center{};
        for (Id n : q) center = Add(center, nodes.at(n));
        const Id other = New(geometry.Project(facet, Mul(center, .25)));
        Junction junction{other, facet, q, {}};
        for (unsigned k = 0; k < 4; ++k) {
          junction.cells.push_back(fresh.cells.size());
          Append({p, other, q[k], q[(k + 1) % 4]});
          fresh.surface.push_back({{other, q[k], q[(k + 1) % 4]}, facet});
        }
        junctions.push_back(junction);
      }
    }
  }
  bool PrismaticCapFits(const std::vector<SurfaceFace>& base,const PrismaticChart& chart,const std::vector<double>& levels) {
    ++report.surface_trials;Check();
    if(base.size()>limits.maximum_cells/(3*(levels.size()-1)))return false;
    using Vertex=std::pair<Id,size_t>;std::set<std::pair<Vertex,Vertex>> seen;
    for(auto face:base)for(size_t layer=1;layer<levels.size();++layer) {
      auto order=face.v;std::sort(order.begin(),order.end());std::array<Point,6> points;std::array<Vertex,6> keys;
      for(unsigned k=0;k<6;++k) {
        const auto level=layer-(k<3);points[k]=Add(nodes.at(order[k%3]),Mul(chart.direction,levels[level]));keys[k]={order[k%3],level};
      }
      for(auto pattern:PrismTets) {
        Check();Tetrahedron tet;for(unsigned k=0;k<4;++k)tet[k]=points[pattern[k]];
        const auto sign=Orientation(tet[0],tet[1],tet[2],tet[3]);if(!sign)return false;if(sign<0)std::swap(tet[0],tet[1]);
        Point center{};for(auto p:tet)center=Add(center,Mul(p,.25));const auto shape=Measure(tet,field.point(center));
        if(shape.mean_ratio<MinimumMeanRatio || shape.minimum_scaled_jacobian<MinimumScaledJacobian)return false;
        for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
          auto edge=std::minmax(keys[pattern[i]],keys[pattern[j]]);if(!seen.emplace(edge.first,edge.second).second)continue;
          if(field.edge(points[pattern[i]],points[pattern[j]])>MaximumMetricEdge)return false;
        }
      }
    }
    return true;
  }
  void ConstructPrismaticCap(std::vector<SurfaceFace>& base,const PrismaticChart& chart,const std::vector<double>& levels) {
    namespace Cap=SU2Native2D;
    PhaseClock timer(limits.timings);timer.Set(report.cap_seconds);
    // Retain a complete admissible front plan before creating a different point distribution.
    if(PrismaticCapFits(base,chart,levels))return;
    Point axis{1,0,0};if(std::abs(chart.direction.y)<std::abs(chart.direction.x))axis={0,1,0};
    if(std::abs(chart.direction.z)<std::abs(Dot(axis,chart.direction)))axis={0,0,1};
    const Point u=Unit(Sub(axis,Mul(chart.direction,Dot(axis,chart.direction)))),v=Cross(chart.direction,u);
    auto project=[&](Point p){p=Sub(p,chart.origin);return Cap::Point{Dot(p,u),Dot(p,v)};};
    auto lift=[&](Cap::Point p){return geometry.Project(chart.base,Add(chart.origin,Add(Mul(u,p.x),Mul(v,p.y))));};
    auto at=[&](Point p,double z){return Add(p,Mul(chart.direction,z));};
    const Cap::Metric metric=[&](Cap::Point p) {
      Check();Eigen::Matrix2d envelope;bool first=true;
      for(double z:levels) {
        const auto m=field.point(at(lift(p),z));
        auto apply=[&](Point d){return Point{m.xx*d.x+m.xy*d.y+m.xz*d.z,m.xy*d.x+m.yy*d.y+m.yz*d.z,m.xz*d.x+m.yz*d.y+m.zz*d.z};};
        Eigen::Matrix2d tangent;tangent<<Dot(u,apply(u)),Dot(u,apply(v)),Dot(v,apply(u)),Dot(v,apply(v));
        envelope=first?tangent:CapIntersection(envelope,tangent);first=false;
      }
      const Cap::Tensor result{envelope(0,0),envelope(0,1),envelope(1,1)};
      if(!(Cap::NormalizedDeterminant(result.xx,result.xy,result.yy)>0))throw std::runtime_error("Prismatic cap tangent metric is not SPD.");
      return result;
    };
    auto charge=[&](){++report.surface_trials;Check();};
    auto admit=[&](size_t count) {
      const size_t factor=3*(levels.size()-1);
      if(count>limits.maximum_cells/factor || count*factor+report.surface_trials+report.junction_trials+report.core_trials+report.front_trials>limits.maximum_work) {
        report.exhausted_work=true;throw std::runtime_error("Whole prismatic cap/front proposal exceeds admitted work/cells.");
      }
    };
    std::vector<Cap::Triangle> seed;
    Eigen::Matrix2d average=Eigen::Matrix2d::Zero();double area=0;
    for(auto f:base) {
      auto t=Cap::triangle({f.v[0],project(nodes.at(f.v[0])),0},{f.v[1],project(nodes.at(f.v[1])),0},{f.v[2],project(nodes.at(f.v[2])),0});
      const double weight=double(Cap::area(t));if(!(weight>0))throw std::runtime_error("Prismatic cap source is degenerate.");
      const auto m=metric(Cap::centroid(t));Eigen::Matrix2d matrix;matrix<<m.xx,m.xy,m.xy,m.yy;
      average+=weight*matrix;area+=weight;seed.push_back(t);
    }
    // shortcut: one average-metric proposal chart; unresolved localized demands reject at full-region validation.
    average/=area;Eigen::LLT<Eigen::Matrix2d> factor(average);
    if(factor.info()!=Eigen::Success)throw std::runtime_error("Prismatic cap proposal chart is not SPD.");
    const Eigen::Matrix2d transform=factor.matrixU(),inverse=transform.inverse();
    const Cap::Tensor chart_metric{average(0,0),average(0,1),average(1,1)};
    const Cap::Metric uniform=[&](Cap::Point){Check();return chart_metric;};
    std::vector<Cap::Node> polygon;
    if(!Cap::star_polygon(seed,UINT64_MAX,polygon))throw std::runtime_error("Prismatic cap requires one simple boundary loop.");
    constexpr double spacing=.85;
    std::vector<Cap::Node> boundary,sites;
    auto span=[&](Cap::Point a,Cap::Point b) {
      if(a.x==b.x && a.y==b.y)return 0.L;
      long double maximum=0;for(double z:levels){Check();maximum=std::max(maximum,field.edge(at(lift(a),z),at(lift(b),z)));}
      return maximum;
    };
    for(size_t k=0;k<polygon.size();++k) {
      const auto a=polygon[k],b=polygon[(k+1)%polygon.size()];boundary.push_back(a);
      const auto total=span(a.p,b.p),count=std::ceil(total/spacing);
      if(!(count>=1 && count<=limits.maximum_work)){report.exhausted_work=true;throw std::runtime_error("Prismatic cap boundary sampling exceeds admitted work.");}
      for(size_t j=1;j<size_t(count);++j) {
        charge();double lo=0,hi=1;
        for(unsigned iteration=0;iteration<32;++iteration) {
          const double t=.5*(lo+hi);if(span(a.p,a.p+(b.p-a.p)*t)<total*j/count)lo=t;else hi=t;
        }
        const auto p=a.p+(b.p-a.p)*(.5*(lo+hi));const Id id=New(lift(p));boundary.push_back({id,p,0});
      }
      admit(boundary.size()>2?boundary.size()-2:1);
    }
    std::vector<Cap::Triangle> cap;
    if(!Cap::ear_clip(boundary,uniform,cap))throw std::runtime_error("Prismatic cap boundary cannot be triangulated.");
    Eigen::Vector2d low=Eigen::Vector2d::Constant(INFINITY),high=-low;
    for(auto n:polygon){const Eigen::Vector2d p=transform*Eigen::Vector2d(n.p.x,n.p.y);low=low.cwiseMin(p);high=high.cwiseMax(p);}
    auto index=[&](double value) {
      if(!std::isfinite(value) || std::abs(value)>double(limits.maximum_work)) {
        report.exhausted_work=true;throw std::runtime_error("Prismatic cap lattice exceeds admitted work.");
      }
      return int64_t(value);
    };
    const double row_height=spacing*std::sqrt(3.)/2;
    const auto i0=index(std::floor(low[0]/spacing)-1),i1=index(std::ceil(high[0]/spacing)+1),
               j0=index(std::floor(low[1]/row_height)-1),j1=index(std::ceil(high[1]/row_height)+1);
    for(auto j=j0;j<=j1;++j)for(auto i=i0;i<=i1;++i) {
      charge();const Eigen::Vector2d q=inverse*(spacing*Eigen::Vector2d(i+.5*((j%2+2)%2)+.25,j*std::sqrt(3.)/2+.25));
      const Cap::Point p{q[0],q[1]};bool inside=false;
      for(auto t:seed)if(Cap::orient(t.v[0].p,t.v[1].p,p)>=0 && Cap::orient(t.v[1].p,t.v[2].p,p)>=0 && Cap::orient(t.v[2].p,t.v[0].p,p)>=0){inside=true;break;}
      if(!inside)continue;
      const auto m=metric(p);bool near=false;
      for(size_t k=0;k<polygon.size();++k) {
        const auto a=polygon[k].p,d=polygon[(k+1)%polygon.size()].p-a,r=p-a;
        const double t=std::max(0.,std::min(1.,(r.x*(m.xx*d.x+m.xy*d.y)+r.y*(m.xy*d.x+m.yy*d.y))/m.quadratic(d)));
        if(m.quadratic(r-d*t)<std::pow(.45*spacing,2)){near=true;break;}
      }
      if(near)continue;
      admit(cap.size()+2*(sites.size()+1));sites.push_back({New(lift(p)),p,0});
    }
    // Construct connectivity for the complete point set, independent of inherited cap edges.
    for(auto n:sites) {
      charge();std::vector<Cap::Triangle> old,next;
      for(auto t:cap) {
        if(Cap::orient(t.v[0].p,t.v[1].p,n.p)>=0 && Cap::orient(t.v[1].p,t.v[2].p,n.p)>=0 && Cap::orient(t.v[2].p,t.v[0].p,n.p)>=0)old.push_back(t);
        else next.push_back(t);
      }
      if(old.empty())throw std::runtime_error("Prismatic cap site leaves its complete containing cavity.");
      for(const auto& edge:Cap::boundary(old)) {
        const auto a=edge.second.first,b=edge.second.second;
        if(Cap::orient(a.p,b.p,n.p)>0)next.push_back(Cap::triangle(a,b,n));
      }
      cap.swap(next);admit(cap.size());
    }
    for(;;) {
      Check();bool changed=false;std::map<Cap::Edge,std::vector<size_t>> stars;
      for(size_t k=0;k<cap.size();++k)for(unsigned j=0;j<3;++j)stars[Cap::edge(cap[k].v[j].id,cap[k].v[(j+1)%3].id)].push_back(k);
      for(const auto& star:stars) {
        Check();if(star.second.size()!=2)continue;
        const auto i=star.second[0],j=star.second[1];std::vector<Cap::Triangle> old{cap[i],cap[j]};
        const auto vertices=Cap::nodes(old);std::vector<Cap::Node> other;
        for(const auto& n:vertices)if(n.first!=star.first.first && n.first!=star.first.second)other.push_back(n.second);
        if(other.size()!=2)throw std::runtime_error("Prismatic cap edge has an invalid complete star.");
        const auto a=vertices.at(star.first.first),b=vertices.at(star.first.second),c=other[0],d=other[1];
        if(!Cap::Opposite(Cap::orient(c.p,d.p,a.p),Cap::orient(c.p,d.p,b.p)))continue;
        const auto first=Cap::triangle(c,d,a),second=Cap::triangle(d,c,b);
        if(std::min(Cap::quality(first,metric),Cap::quality(second,metric))<=Cap::min_quality(old,metric)+1e-7)continue;
        charge();admit(cap.size());std::string why;
        if(!Cap::validate_replacement(old,{first,second},why))throw std::runtime_error("Prismatic cap reconnection is invalid: "+why);
        cap[i]=first;cap[j]=second;changed=true;break;
      }
      if(!changed)break;
    }
    admit(cap.size());base.clear();for(auto t:cap)base.push_back({{t.v[0].id,t.v[1].id,t.v[2].id},chart.base});
  }
  void PrismaticFronts(const PrismaticChart& chart,PhaseClock& clock) {
    // shortcut: a certified right prism with wall policies only on its opposite caps; nonprismatic junctions need a regional decomposition.
    std::vector<SurfaceFace> base;
    for(const auto& row:geometry.Facets())if(geometry.Canonical(row.first)==chart.base) {
      if(base.size()>=limits.maximum_cells/3){report.exhausted_work=true;throw std::runtime_error("Prismatic source triangulation exceeds admitted transition cells.");}
      SurfaceFace f;f.facet=row.first;
      for(unsigned k=0;k<3;++k){const auto n=row.second.v[k];
        if(n.id>=next_node)throw std::runtime_error("Prismatic new identities overlap the original cap.");
        nodes.emplace(n.id,n.p);f.v[k]=n.id;}
      Check();
      base.push_back(f);
    }
    const auto bottom=policy.find(geometry.Get(chart.base).marker),top=policy.find(geometry.Get(chart.top).marker);
    const double first=bottom==policy.end()?0:bottom->second.height,
                 last=chart.gap-(top==policy.end()?0:top->second.height);
    if(!(first<last))throw std::runtime_error("Prismatic geometry-height incompatibility: opposite prescribed rows leave no transition interval.");
    auto point=[&](Id n,double z){return Add(nodes.at(n),Mul(chart.direction,z));};
    std::vector<double> levels;
    for(;;) {
      Check();std::set<Id> vertices;for(auto f:base)for(Id n:f.v)vertices.insert(n);
      auto prefix=[&](double z) {
        ++report.front_trials;Check();long double maximum=0;
        for(Id n:vertices){Check();const auto length=field.edge(point(n,first),point(n,z));
          if(!(length>0) || !std::isfinite(length))throw std::runtime_error("Prismatic column has no finite metric span.");
          maximum=std::max(maximum,length);}
        return maximum;
      };
      if(levels.empty()) {
      const auto total=prefix(last);const auto count=std::ceil(total/(.55L*MaximumMetricEdge));
      if(!(count>=1) || count>limits.maximum_work){report.exhausted_work=true;throw std::runtime_error("Prismatic front span exceeds admitted work.");}
      const size_t intervals=size_t(count);
      if(intervals>limits.maximum_cells/(3*base.size())) {
        report.exhausted_work=true;throw std::runtime_error("Prismatic normal intervals exceed admitted transition cells.");
      }
      levels.clear();if(first>0)levels.push_back(0);levels.push_back(first);
      for(size_t j=1;j<intervals;++j) {
        const auto wanted=total*j/intervals;double lo=levels.back(),hi=last;
        for(unsigned iteration=0;iteration<24;++iteration) {
          const double z=.5*(lo+hi);const auto value=prefix(z);
          if(value<=wanted)lo=z;else hi=z;
          if(value<=wanted && wanted-value<=1e-7L*total)break;
        }
        if(!(lo>levels.back() && lo<last))throw std::runtime_error("Prismatic front makes no representable progress.");
        levels.push_back(lo);
      }
      levels.push_back(last);if(last<chart.gap)levels.push_back(chart.gap);
      }

      ConstructPrismaticCap(base,chart,levels);
      // New columns can expose a stronger original sensor; close both directions before constructing cells.
      vertices.clear();for(auto f:base)for(Id n:f.v)vertices.insert(n);
      bool divided=false;
      for(size_t k=1;k<levels.size();++k) {
        auto interval=[&](double end) {
          ++report.front_trials;Check();long double maximum=0;
          for(Id n:vertices){Check();const auto value=field.edge(point(n,levels[k-1]),point(n,end));
            if(!(value>0) || !std::isfinite(value))throw std::runtime_error("Prismatic interval has no finite metric span.");
            maximum=std::max(maximum,value);}
          return maximum;
        };
        const auto length=interval(levels[k]);
        const bool wall=(first>0 && k==1) || (last<chart.gap && k+1==levels.size());
        if(wall && length>MaximumMetricEdge)throw std::runtime_error("Prismatic prescribed-height column exceeds the original metric length; this layout cannot satisfy both constraints.");
        if(wall || length<=.55L*MaximumMetricEdge*(1+1e-6L))continue;
        if(levels.size()>limits.maximum_cells/(3*base.size())) {
          report.exhausted_work=true;throw std::runtime_error("Prismatic interval closure exceeds admitted transition cells.");
        }
        // Maxima of cumulative prefixes need not bound their differences when the controlling column changes.
        double lo=levels[k-1],hi=levels[k];const auto wanted=.5L*length;
        for(unsigned iteration=0;iteration<24;++iteration) {
          const double mid=.5*(lo+hi);const auto value=interval(mid);
          if(value<=wanted)lo=mid;else hi=mid;
          if(value<=wanted && wanted-value<=1e-7L*length)break;
        }
        if(!(lo>levels[k-1] && lo<levels[k]))throw std::runtime_error("Prismatic interval closure makes no representable progress.");
        levels.insert(levels.begin()+k,lo);divided=true;--k;
      }
      if(!divided)break;
    }
    if(base.size()>limits.maximum_cells/(3*(levels.size()-1))) {
      report.exhausted_work=true;throw std::runtime_error("Prismatic complete transition exceeds admitted cells.");
    }
    std::map<Id,std::vector<Id>> columns;
    for(auto f:base)for(Id n:f.v)if(!columns.count(n)) {
      auto& column=columns[n];column.push_back(n);
      for(size_t k=1;k<levels.size();++k)column.push_back(New(point(n,levels[k])));
    }
    clock.Set(report.transition_seconds);
    for(size_t layer=1;layer<levels.size();++layer) {
      for(auto f:base) {
        auto order=f.v;std::sort(order.begin(),order.end());
        std::array<Id,6> vertices;for(unsigned k=0;k<6;++k)vertices[k]=columns.at(order[k%3])[layer-(k<3)];
        int orientation=0;
        for(auto pattern:PrismTets) {
          std::array<Id,4> ids;for(unsigned k=0;k<4;++k)ids[k]=vertices[pattern[k]];
          const auto cell=Tet(ids);const auto volume=Orientation(cell.v[0].p,cell.v[1].p,cell.v[2].p,cell.v[3].p);
          if(!volume)throw std::runtime_error("Prismatic transition has an unresolvable tetrahedron.");
          const int sign=volume>0?1:-1;
          if(orientation && sign!=orientation)throw std::runtime_error("Prismatic transition folds within a prism.");
          orientation=sign;if(sign<0)std::swap(ids[0],ids[1]);Append(ids);
        }
      }
      ++report.transition_fronts;
    }
    std::set<int> markers;for(const auto& row:geometry.Facets())markers.insert(row.second.marker);
    for(auto face:Incidence(fresh.cells).Boundary()) {
      std::array<Point,3> triangle{nodes.at(face[0]),nodes.at(face[1]),nodes.at(face[2])};Id facet=0;bool found=false;
      for(int marker:markers)if(geometry.FindFace(marker,triangle,facet)){found=true;break;}
      if(!found)throw std::runtime_error("Prismatic physical face leaves the finite original authority.");
      fresh.surface.push_back({face,facet});
    }
    report.core_regions=1;
  }
  void MetricCylinderFronts(std::vector<SurfaceFace>& wall, const std::map<Id,Id>& first,
                            std::map<int,std::vector<std::array<Id,3>>>& rim,
                            const RegionWallPolicy& policy, Point& kernel) {
    const size_t angular=cylinder_angular, levels=cylinder_levels.size();
    if (angular<3 || levels<2 || cylinder_grid.size()!=angular*levels)
      throw std::runtime_error("Metric cylindrical front has no complete periodic chart.");
    const auto& g=geometry.Analytic().at(geometry.Canonical(wall.front().facet));
    // shortcut: one BL-free radial core interface; interacting/non-radial regions need another decomposition.
    const double core_radius=.75*(g.extent.x-policy.thickness),pi=std::acos(-1.);
    if (!(core_radius>0)) throw std::runtime_error("Metric cylindrical layout requires a positive BL-free core radius.");
    auto position=[&](double radius,double angle,size_t level) {
      auto point=Add(Add(cylinder_origin,Mul(cylinder_axis,cylinder_levels[level])),
          Mul(Add(Mul(cylinder_u,std::cos(angle)),Mul(cylinder_v,std::sin(angle))),radius));
      if (!level || level+1==levels) point=geometry.Project(cylinder_caps[level+1==levels],point);
      return point;
    };
    std::vector<Id> current(cylinder_grid.size());double first_radius=INFINITY;
    for(size_t k=0;k<current.size();++k) {
      current[k]=first.at(cylinder_grid[k]);const auto q=Sub(nodes.at(current[k]),cylinder_origin);
      first_radius=std::min(first_radius,std::hypot(Dot(q,cylinder_u),Dot(q,cylinder_v)));
    }
    if (!(first_radius>core_radius)) throw std::runtime_error("Metric cylindrical core overlaps the prescribed first band.");
    const auto initial=current;
    auto prefix=[&](double radius) {
      ++report.front_trials;Check();long double maximum=0;
      for(size_t j=0;j<levels;++j) for(size_t i=0;i<angular;++i) {
        Check();const auto a=nodes.at(initial[j*angular+i]),b=position(radius,2*pi*i/angular,j);
        if (Norm(Sub(a,b))>0) maximum=std::max(maximum,field.edge(a,b));
      }
      if (!(maximum>0) || !std::isfinite(maximum)) throw std::runtime_error("Metric cylindrical column has no finite span.");
      return maximum;
    };
    const auto total=prefix(core_radius);const auto count=std::ceil(total/(.5L*MaximumMetricEdge));
    if (!(count>=1) || count>limits.maximum_work) {
      report.exhausted_work=true;throw std::runtime_error("Metric cylindrical front count exceeds admitted work.");
    }
    const size_t fronts=size_t(count);
    if(fronts>limits.maximum_work/(3*wall.size())) {
      report.exhausted_work=true;throw std::runtime_error("Metric cylindrical transition cells exceed admitted work.");
    }
    std::vector<double> radii(fronts+1);radii.front()=first_radius;radii.back()=core_radius;
    for(size_t j=1;j<fronts;++j) {
      const auto wanted=total*j/fronts;double lo=core_radius,hi=radii[j-1];
      for(unsigned iteration=0;iteration<24;++iteration) {
        const double r=.5*(lo+hi);const auto value=prefix(r);
        if(value<=wanted) hi=r;else lo=r;
        if(value<=wanted && wanted-value<=1e-7L*total) break;
      }
      radii[j]=hi;
      if(!(radii[j]<radii[j-1] && radii[j]>core_radius))
        throw std::runtime_error("Metric cylindrical front makes no representable inward progress.");
    }
    for(size_t layer=1;layer<=fronts;++layer) {
      Check();const double phase=-double(layer)*pi/angular;
      std::map<Id,std::pair<size_t,size_t>> chart;
      for(size_t j=0;j<levels;++j)for(size_t i=0;i<angular;++i) chart.emplace(current[j*angular+i],std::make_pair(i,j));
      auto image=[&](bool coarsen) {
        const size_t columns=coarsen?(angular+1)/2:angular;
        std::vector<Point> points(columns*levels);
        for(size_t j=0;j<levels;++j)for(size_t i=0;i<columns;++i) {
          const size_t begin=coarsen?2*i:i;
          const double center=coarsen && begin+1<angular ? begin+.5 : double(begin);
          points[j*columns+i]=position(radii[layer],2*pi*center/angular+phase,j);
        }
        return points;
      };
      bool coarsen=layer==fronts && (angular+1)/2>=3;
      auto points=image(coarsen);
      auto logical=[&](Id n) {const auto c=chart.at(n);return c.second*(coarsen?(angular+1)/2:angular)+(coarsen?c.first/2:c.first);};
      if(coarsen) {
        ++report.front_trials;Check();std::set<Edge> edges;
        for(auto f:wall) {
          const Face mapped{Id(logical(f.v[0])),Id(logical(f.v[1])),Id(logical(f.v[2]))};
          if(mapped[0]==mapped[1] || mapped[0]==mapped[2] || mapped[1]==mapped[2])continue;
          for(unsigned i=0;i<3;++i)edges.insert(Key(mapped[i],mapped[(i+1)%3]));
        }
        for(auto edge:edges) {
          Check();if(field.edge(points[edge[0]],points[edge[1]])>MaximumMetricEdge){coarsen=false;break;}
        }
        if(!coarsen)points=image(false);
      }
      std::vector<Id> new_ids;new_ids.reserve(points.size());for(auto point:points)new_ids.push_back(New(point));
      std::map<Id,Id> mapped;
      for(Id n:current)mapped.emplace(n,new_ids[logical(n)]);
      std::map<Edge,std::pair<Id,Id>> arrows;
      for(auto f:wall) {
        Face order=f.v;size_t low=angular,high=0;
        for(Id n:order){low=std::min(low,chart.at(n).first);high=std::max(high,chart.at(n).first);}
        const size_t start=low==0 && high+1==angular ? angular-1 : low;
        auto key=[&](Id n){const auto c=chart.at(n);return std::make_tuple((c.first+angular-start)%angular,c.second,n);};
        std::sort(order.begin(),order.end(),[&](Id a,Id b){return key(a)<key(b);});
        for(unsigned i=0;i<3;++i)for(unsigned j=i+1;j<3;++j) {
          const auto edge=Key(order[i],order[j]);const auto direction=std::make_pair(order[i],order[j]);
          const auto found=arrows.emplace(edge,direction);
          if(!found.second && found.first->second!=direction)
            throw std::runtime_error("Metric cylindrical periodic interface ordering disagrees.");
        }
        const Id a=order[0],b=order[1],c=order[2],aa=mapped.at(a),bb=mapped.at(b),cc=mapped.at(c);
        const std::array<std::array<Id,4>,3> proposed{{{a,b,c,cc},{a,b,cc,bb},{a,bb,cc,aa}}};
        int orientation=0;
        for(auto ids:proposed) {
          auto unique=ids;std::sort(unique.begin(),unique.end());if(std::adjacent_find(unique.begin(),unique.end())!=unique.end())continue;
          const auto tet=Tet(ids);const auto volume=Orientation(tet.v[0].p,tet.v[1].p,tet.v[2].p,tet.v[3].p);
          if(!volume)throw std::runtime_error("Metric cylindrical transition has a degenerate tetrahedron.");
          const int sign=volume>0?1:-1;
          if(orientation && orientation!=sign)throw std::runtime_error("Metric cylindrical transition folds within a prism.");
          orientation=sign;if(sign<0)std::swap(ids[0],ids[1]);Append(ids);
        }
      }
      std::map<Edge,Id> cap_faces;
      for(const auto& loop:rim)for(auto row:loop.second)cap_faces.emplace(Key(row[0],row[1]),row[2]);
      for(const auto& edge:Links(wall))if(edge.second.size()==1) {
        const auto use=edge.second.front();const Id a=use.a,b=use.b,aa=mapped.at(a),bb=mapped.at(b);
        const auto facet=cap_faces.find(edge.first);if(facet==cap_faces.end())throw std::runtime_error("Metric cylindrical rim is not admitted.");
        const std::array<Face,2> triangles=arrows.at(edge.first).first==a ?
            std::array<Face,2>{{{a,aa,bb},{a,bb,b}}}:std::array<Face,2>{{{a,aa,b},{aa,bb,b}}};
        for(auto triangle:triangles)if(triangle[0]!=triangle[1] && triangle[0]!=triangle[2] && triangle[1]!=triangle[2])
          fresh.surface.push_back({triangle,facet->second});
      }
      std::vector<SurfaceFace> next;
      for(auto f:wall) {
        for(auto& n:f.v)n=mapped.at(n);
        if(f.v[0]!=f.v[1] && f.v[0]!=f.v[2] && f.v[1]!=f.v[2])next.push_back(f);
      }
      wall.swap(next);
      for(auto& loop:rim) {
        std::vector<std::array<Id,3>> next;
        for(auto row:loop.second){row[0]=mapped.at(row[0]);row[1]=mapped.at(row[1]);if(row[0]!=row[1])next.push_back(row);}
        loop.second.swap(next);
      }
      current.swap(new_ids);++report.transition_fronts;
    }
    kernel=Add(cylinder_origin,Mul(cylinder_axis,.5*g.extent.y));
  }
  void PlaceJunctions() {
    for (const auto& j : junctions) {
      Check();
      const Point origin = nodes.at(j.node), u = Unit(Sub(nodes.at(j.quad[1]), nodes.at(j.quad[0]))),
                  normal = Unit(Cross(Sub(nodes.at(j.quad[1]), nodes.at(j.quad[0])),
                                      Sub(nodes.at(j.quad[2]), nodes.at(j.quad[0])))),
                  v = Cross(normal, u);
      double diameter = 0;
      for (Id n : j.quad) diameter = std::max(diameter, Norm(Sub(nodes.at(n), origin)));
      auto evaluate = [&](Point point) {
        ++report.junction_trials;
        Check();
        point = geometry.Project(j.facet, point);
        for (unsigned k = 0; k < 4; ++k)
          if (Dot(Cross(Sub(nodes.at(j.quad[(k + 1) % 4]), nodes.at(j.quad[k])), Sub(point, nodes.at(j.quad[k]))),
                  normal) < 0)
            return -std::numeric_limits<double>::infinity();
        double score = INFINITY;
        for (size_t index : j.cells) {
          auto c = fresh.cells[index];
          for (auto& n : c.v)
            if (n.id == j.node) n.p = point;
          if (!(Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) > 0))
            return -std::numeric_limits<double>::infinity();
          const auto m = Shape(c);
          score = std::min({score, m.mean_ratio / .22, m.minimum_scaled_jacobian / .055});
        }
        return score;
      };
      Point best = origin;
      double value = evaluate(best), step = .1 * diameter;
      for (unsigned iteration = 0; iteration < 48 && value < 1.0 && step > 1e-7 * diameter; ++iteration) {
        Point chosen = best;
        double next = value;
        for (int x = -1; x <= 1; ++x)
          for (int y = -1; y <= 1; ++y)
            if (x || y) {
              const Point trial = geometry.Project(j.facet, Add(best, Mul(Add(Mul(u, x), Mul(v, y)), step)));
              const double s = evaluate(trial);
              if (s > next) {
                next = s;
                chosen = trial;
              }
            }
        if (next > value) {
          best = chosen;
          value = next;
        } else
          step *= .5;
      }
      nodes.at(j.node) = best;
      for (size_t index : j.cells)
        for (auto& n : fresh.cells[index].v)
          if (n.id == j.node) n.p = best;
    }
    lengths.clear();
  }
  void Core(const std::vector<SurfaceFace>& skin, Id apex) {
    struct Region { Id apex; std::vector<SurfaceFace> skin; std::vector<Cell> cells; };
    const size_t band = fresh.cells.size();
    Region initial{apex, skin, {}};
    std::map<Id, Cell> cells;
    for (auto f : skin) {
      Append({apex, f.v[0], f.v[1], f.v[2]});
      initial.cells.push_back(fresh.cells.back());
      cells.emplace(fresh.cells.back().id, fresh.cells.back());
    }
    report.core_regions = 1;
    std::deque<Region> pending; pending.push_back(std::move(initial));
    while (!pending.empty()) {
      ++report.core_trials; Check();
      auto region = std::move(pending.front()); pending.pop_front();
      bool resolved = true;
      for (const auto& c : region.cells) {
        if (!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0))
          throw std::runtime_error("Region core has no positive initial fan.");
        const auto m = Shape(c);
        resolved = resolved && m.mean_ratio>=MinimumMeanRatio && m.minimum_scaled_jacobian>=MinimumScaledJacobian;
        for (unsigned a=0;a<4;++a) for (unsigned b=a+1;b<4;++b)
          if (Length(c.v[a].id,c.v[b].id)>MaximumMetricEdge) resolved=false;
      }
      if (resolved) continue;
      report.unresolved_core=true;
      for (const auto& edge:Links(region.skin)) if (Length(edge.first[0],edge.first[1])>MaximumMetricEdge) {
        std::ostringstream why;why<<"Region core needs shared-interface remeshing at edge "<<edge.first[0]<<','<<edge.first[1];
        throw std::runtime_error(why.str());
      }
      // A separator changes the volume complex; it never changes the wall/transition interface.
      const Point origin=nodes.at(region.apex); std::set<Id> vertices;
      for (auto f:region.skin) for (Id n:f.v) vertices.insert(n);
      double scale=0;
      for (Id n:vertices) scale=std::max(scale,Norm(Sub(nodes.at(n),origin)));
      if (!(scale>0)) throw std::runtime_error("Region core has no finite extent.");
      Eigen::Matrix3d covariance=Eigen::Matrix3d::Zero();
      for (Id n:vertices) {
        const auto p=Mul(Sub(nodes.at(n),origin),1/scale); const Eigen::Vector3d x(p.x,p.y,p.z);
        covariance.noalias()+=x*x.transpose()/double(vertices.size());
      }
      const auto m=field.point(origin); ValidateTensor(m);
      Eigen::Matrix3d metric; metric<<m.xx,m.xy,m.xz,m.xy,m.yy,m.yz,m.xz,m.yz,m.zz;
      metric/=metric.cwiseAbs().maxCoeff(); Eigen::LLT<Eigen::Matrix3d> factor(metric);
      if (factor.info()!=Eigen::Success) throw std::runtime_error("Region core metric frame cannot be factored.");
      const Eigen::Matrix3d root=factor.matrixL();
      Eigen::SelfAdjointEigenSolver<Eigen::Matrix3d> principal(root.transpose()*covariance*root);
      if (principal.info()!=Eigen::Success) throw std::runtime_error("Region core metric extent cannot be resolved.");
      const Eigen::Vector3d normal=root*principal.eigenvectors().col(2);
      const Point axis=Unit({normal[0],normal[1],normal[2]});
      std::map<Id,long double> level;
      for (Id n:vertices) {const auto p=Sub(nodes.at(n),origin);const long double z=(long double)axis.x*p.x+(long double)axis.y*p.y+(long double)axis.z*p.z;level[n]=z;}
      std::vector<std::pair<long double,long double>> volume_order;long double volume=0;
      for (auto f:region.skin) {
        const long double z=std::max({level.at(f.v[0]),level.at(f.v[1]),level.at(f.v[2])});
        const long double v=Orientation(origin,nodes.at(f.v[0]),nodes.at(f.v[1]),nodes.at(f.v[2]));
        volume_order.push_back({z,v});volume+=v;
      }
      std::sort(volume_order.begin(),volume_order.end());
      long double cut=0,best=INFINITY,left=0;
      for (size_t i=1;i<volume_order.size();++i) {
        left+=volume_order[i-1].second;
        if (!(volume_order[i-1].first<volume_order[i].first)) continue;
        const long double imbalance=std::abs(left-.5L*volume);
        if (imbalance<best) {best=imbalance;cut=(volume_order[i-1].first+volume_order[i].first)/2;}
      }
      std::array<Region,2> children; std::vector<unsigned> part;
      for (auto f:region.skin) {
        const unsigned group=std::max({level.at(f.v[0]),level.at(f.v[1]),level.at(f.v[2])})>cut;
        part.push_back(group);children[group].skin.push_back(f);
      }
      if (children[0].skin.empty() || children[1].skin.empty())
        throw std::runtime_error("Region core metric separator has an empty side.");
      std::vector<std::pair<Id,Id>> loop;
      for (const auto& row:Links(region.skin)) {
        if (row.second.size()!=2) throw std::runtime_error("Region core boundary is not closed.");
        auto a=row.second[0],b=row.second[1];
        if (a.a!=b.b || a.b!=b.a) throw std::runtime_error("Region core boundary orientations disagree.");
        if (part[a.face]==part[b.face]) continue;
        if (part[a.face]) std::swap(a,b);
        loop.push_back({a.a,a.b});
        children[0].skin.push_back({{region.apex,a.b,a.a},0});
        children[1].skin.push_back({{region.apex,a.a,a.b},0});
      }
      VerifyLoop(loop);
      // The old positive fan supplies volume-weighted interior proposals on each side.
      for (unsigned group=0;group<2;++group) {
        long double volume=0,x=0,y=0,z=0;
        for (size_t i=0;i<region.skin.size();++i) if (part[i]==group) {
          const auto f=region.skin[i];const auto a=nodes.at(f.v[0]),b=nodes.at(f.v[1]),c=nodes.at(f.v[2]);
          const long double v=Orientation(origin,a,b,c); volume+=v;
          x+=v*(((long double)a.x-origin.x)+((long double)b.x-origin.x)+((long double)c.x-origin.x))/4;
          y+=v*(((long double)a.y-origin.y)+((long double)b.y-origin.y)+((long double)c.y-origin.y))/4;
          z+=v*(((long double)a.z-origin.z)+((long double)b.z-origin.z)+((long double)c.z-origin.z))/4;
        }
        if (!(volume>0)) throw std::runtime_error("Region core separator has nonpositive volume.");
        const Point point{double(origin.x+x/volume),double(origin.y+y/volume),double(origin.z+z/volume)};
        std::vector<Face> boundary;
        for (auto f:children[group].skin) {
          boundary.push_back(f.v);
          if (!(Orientation(point,nodes.at(f.v[0]),nodes.at(f.v[1]),nodes.at(f.v[2]))>0)) {
            std::ostringstream why;why<<std::setprecision(17)<<"Region core separator lacks a visible child kernel at ("<<point.x<<','<<point.y<<','<<point.z
              <<") across face "<<f.v[0]<<','<<f.v[1]<<','<<f.v[2]<<" of parent "<<region.apex;
            throw std::runtime_error(why.str());
          }
        }
        ValidateSurfaceTopology(boundary,true,4*limits.maximum_cells);
        children[group].apex=New(point);
        for (auto f:children[group].skin) {
          ++report.core_trials; Check();
          if (next_cell==UINT64_MAX) throw std::overflow_error("Region core cell IDs exhausted.");
          children[group].cells.push_back(Tet({children[group].apex,f.v[0],f.v[1],f.v[2]},next_cell++));
        }
      }
      for (const auto& c:region.cells) cells.erase(c.id);
      for (auto& child:children) for (const auto& c:child.cells) cells.emplace(c.id,c);
      fresh.cells.resize(band);
      for (const auto& row:cells) fresh.cells.push_back(row.second);
      report.generated_cells=fresh.cells.size(); ++report.core_regions; Check();
      for (auto& child:children) pending.push_back(std::move(child));
    }
    report.unresolved_core=false;
  }
  void Validate() {
    Check();
    const Incidence topology(fresh.cells);
    if (topology.FaceComponents() != 1) throw std::runtime_error("Region construction has disconnected volumes.");
    const auto skin = topology.Boundary();
    ValidateDomainSurfaceTopology(skin);
    std::map<Face, Id> physical;
    for (auto f : fresh.surface) {
      if (!SurfaceValid(f)) throw std::runtime_error("Region physical face fails authority/error.");
      if (!physical.emplace(Key(f.v), f.facet).second) throw std::runtime_error("Region repeats a physical face.");
    }
    if (physical.size() != skin.size()) throw std::runtime_error("Region has a missing physical face.");
    for (auto f : skin)
      if (!physical.count(Key(f))) throw std::runtime_error("Region has a spurious interface face.");
    // Complete vertex links exclude pinched interior stars that face incidence alone cannot detect.
    std::map<Id, std::vector<Face>> links;
    std::set<Id> boundary;
    for (auto f : skin)
      for (Id n : f) boundary.insert(n);
    for (const auto& c : fresh.cells)
      for (unsigned k = 0; k < 4; ++k) {
        Face f;
        unsigned at = 0;
        for (unsigned j = 0; j < 4; ++j)
          if (j != k) f[at++] = c.v[j].id;
        if (k % 2) std::swap(f[1], f[2]);
        links[c.v[k].id].push_back(f);
      }
    for (const auto& row : links)
      ValidateSurfaceTopology(row.second, !boundary.count(row.first), 4 * limits.maximum_cells);
    std::map<int, double> heights;
    for (auto p : policy) heights[p.first] = p.second.height;
    // The regional skin is validated above; reuse the bounded height predicate on each incident cell.
    for (const auto& cell : fresh.cells) {
      Patch local;
      local.cells.push_back(cell);
      const Id a = cell.v[0].id, b = cell.v[1].id, c = cell.v[2].id, d = cell.v[3].id;
      const std::array<Face, 4> faces{{{b, c, d}, {a, d, c}, {a, b, d}, {a, c, b}}};
      bool wall = false;
      for (auto f : faces) {
        auto found = physical.find(Key(f));
        if (found != physical.end()) {
          local.surface.push_back({f, found->second});
          wall = wall || Wall(found->second);
        }
      }
      if (wall) {
        FirstHeightReport height;
        std::string why;
        if (!CheckFirstHeight(local, heights, geometry, height, why)) throw std::runtime_error(why);
      }
    }
    struct Box {
      Point lo, hi;
    };
    std::vector<Box> boxes;
    boxes.reserve(fresh.cells.size());
    for (const auto& c : fresh.cells) {
      Check();
      const auto m = Shape(c);
      report.minimum_quality = std::min(report.minimum_quality, m.mean_ratio);
      report.minimum_jacobian = std::min(report.minimum_jacobian, m.minimum_scaled_jacobian);
      if (m.mean_ratio < MinimumMeanRatio || m.minimum_scaled_jacobian < MinimumScaledJacobian) {
        const auto x = Centroid(c);
        std::ostringstream reason;
        reason << std::setprecision(17) << "Region actual-centroid shape failure at cell " << c.id << " centroid ("
               << x.x << ',' << x.y << ',' << x.z << "): q=" << m.mean_ratio << ", J=" << m.minimum_scaled_jacobian;
        throw std::runtime_error(reason.str());
      }
      Box box{c.v[0].p, c.v[0].p};
      for (auto n : c.v) {
        box.lo = {std::min(box.lo.x, n.p.x), std::min(box.lo.y, n.p.y), std::min(box.lo.z, n.p.z)};
        box.hi = {std::max(box.hi.x, n.p.x), std::max(box.hi.y, n.p.y), std::max(box.hi.z, n.p.z)};
      }
      boxes.push_back(box);
      for (unsigned a = 0; a < 4; ++a)
        for (unsigned b = a + 1; b < 4; ++b) {
          const auto l = Length(c.v[a].id, c.v[b].id);
          report.maximum_edge = std::max(report.maximum_edge, l);
          if (l > MaximumMetricEdge) {
            const auto x = c.v[a], y = c.v[b];
            std::ostringstream reason;
            reason << std::setprecision(20) << "Region full metric edge " << x.id << ',' << y.id << " has length " << l
                   << " at (" << x.p.x << ',' << x.p.y << ',' << x.p.z << ") -> (" << y.p.x << ',' << y.p.y << ','
                   << y.p.z << ')';
            throw std::runtime_error(reason.str());
          }
        }
    }
    for (size_t i = 0; i < fresh.cells.size(); ++i) {
      Check();
      for (size_t j = i + 1; j < fresh.cells.size(); ++j) {
        const auto a = boxes[i], b = boxes[j];
        if (a.hi.x < b.lo.x || b.hi.x < a.lo.x || a.hi.y < b.lo.y || b.hi.y < a.lo.y || a.hi.z < b.lo.z ||
            b.hi.z < a.lo.z)
          continue;
        if (!ConformingDisjoint(fresh.cells[i], fresh.cells[j]))
          throw std::runtime_error("Region has overlapping/nonconforming tetrahedra.");
      }
    }
  }
};
}  // namespace
bool PrismaticWallRegion(const PlanarReference& geometry,const std::map<int,RegionWallPolicy>& policies) {
  PrismaticChart chart;return PrismChart(geometry,policies,chart);
}
bool ConstructWallRegion(const std::vector<Facet>& skin, Point kernel, const PlanarReference& geometry,
                         const std::map<int, RegionWallPolicy>& policies, double tolerance, Id first_node,
                         Id first_cell, const RegionMetric& target, const RegionLimits& limits, Patch& output,
                         RegionReport& report, std::string& reason) {
  report = {};
  PhaseClock clock(limits.timings);
  clock.Set(report.sampling_seconds);
  Construction c(geometry, policies, tolerance, target, limits, report, first_node, first_cell);
  try {
    PrismaticChart prism;
    if(PrismChart(geometry,policies,prism)) {
      if(!limits.maximum_cells || !limits.maximum_work)throw std::runtime_error("Prismatic construction requires positive admission limits.");
      for(const auto& p:policies)if(!(p.second.height>0 && p.second.growth>=1 && p.second.thickness>=p.second.height) ||
          !std::isfinite(p.second.height+p.second.growth+p.second.thickness))throw std::runtime_error("Invalid prismatic wall policy.");
      c.PrismaticFronts(prism,clock);clock.Set(report.validation_seconds);c.Validate();
      output=std::move(c.fresh);reason.clear();return true;
    }
    if (policies.size() > 1 || !(tolerance > 0) || !limits.maximum_cells || !limits.maximum_work)
      throw std::runtime_error(
          "Region construction supports at most one wall policy and needs positive admission limits.");
    const auto p = policies.empty() ? RegionWallPolicy{} : policies.begin()->second;
    if (!policies.empty() && (!(p.height > 0 && p.growth >= 1 && p.thickness > p.height) ||
                              !std::isfinite(p.height + p.growth + p.thickness)))
      throw std::runtime_error("Invalid region wall policy.");
    std::set<Id> wall_authorities;
    for (const auto& f : skin) {
      if (!geometry.CompleteAnalytic(f.id))
        throw std::runtime_error("Region requires complete declared geometry authority.");
      const auto chart = geometry.Canonical(f.id);
      const auto& a = geometry.Analytic().at(chart);
      if (c.Wall(f.id)) {
        if (a.kind != AnalyticSurface::Kind::Cylinder && a.kind != AnalyticSurface::Kind::Sphere)
          throw std::runtime_error("Region wall has unsupported inward geometry.");
        wall_authorities.insert(chart);
      } else if (a.kind != AnalyticSurface::Kind::Disk)
        throw std::runtime_error("Region nonwall boundary must be planar.");
      SurfaceFace face;
      face.facet = f.id;
      for (unsigned k = 0; k < 3; ++k) {
        const auto n = f.v[k];
        if (n.id >= first_node) throw std::runtime_error("Region new ID reservation overlaps retained skin.");
        auto inserted = c.nodes.emplace(n.id, n.p);
        if (!inserted.second && Norm(Sub(inserted.first->second, n.p)) != 0)
          throw std::runtime_error("Region skin repeats inconsistent coordinates.");
        face.v[k] = n.id;
      }
      c.faces.push_back(face);
    }
    if (wall_authorities.size() != 1) throw std::runtime_error("Region needs one connected complete wall authority.");
    std::vector<Face> original_skin;
    for (auto f : c.faces) original_skin.push_back(f.v);
    ValidateDomainSurfaceTopology(original_skin);
    if (!c.SampleCylinder() && !c.SampleSphere()) {
      c.SplitSurface();
      c.CoarsenSurface(kernel);
    }
    for (auto f : c.faces)
      if (!(Dot(c.Normal(f), Sub(kernel, c.nodes.at(f.v[0]))) < 0))
        throw std::runtime_error("Region lacks a visible interior kernel.");
    if (policies.empty()) {
      const Id core = c.New(kernel);
      c.fresh.surface = c.faces;
      clock.Set(report.core_seconds);
      c.Core(c.faces,core);
      clock.Set(report.validation_seconds);
      c.Validate();
      output = std::move(c.fresh);
      reason.clear();
      return true;
    }
    const double requested = .9 * MaximumMetricEdge,
                 depth = p.growth == 1 ? p.height * requested
                                       : p.height *
                                             (.5 * (p.growth - 1) +
                                              .5 * (p.growth + 1) *
                                                  std::expm1((requested - .5) * 2 * (p.growth - 1) / (p.growth + 1))) /
                                             (p.growth - 1);
    if (!(depth > p.height && depth < p.thickness))
      throw std::runtime_error("Region first band does not fit the requested thickness.");
    clock.Set(report.first_band_seconds);
    auto flow = c.Fit(c.faces, depth);
    auto front = c.MapFront(c.faces, flow, 1);
    kernel = flow.Apply(kernel);
    std::vector<SurfaceFace> wall, inner;
    std::vector<Id> apices;
    for (auto f : c.faces)
      if (c.Wall(f.facet)) {
        wall.push_back(f);
        apices.push_back(c.New(Sub(c.Center(f), Mul(c.Normal(f), p.height))));
      }
    std::map<Edge, Id> side;
    std::map<int, std::vector<std::array<Id, 3>>> rim;
    for (const auto& edge : c.Links(c.faces)) {
      if (edge.second.size() != 2) throw std::runtime_error("Region skin is not closed.");
      std::vector<Use> walls, other;
      for (auto u : edge.second) (c.Wall(c.faces[u.face].facet) ? walls : other).push_back(u);
      if (walls.size() == 1) {
        if (other.size() != 1) throw std::runtime_error("Region rim lacks a planar cap.");
        const auto u = other.front();
        const Id f = c.faces[u.face].facet;
        side[edge.first] = f;
        rim[geometry.Get(f).marker].push_back({front.at(u.a), front.at(u.b), f});
      }
    }
    c.Band(wall, front, apices, side, true);
    for (auto f : wall) {
      for (auto& n : f.v) n = front.at(n);
      inner.push_back(f);
    }
    auto close = [&](std::vector<SurfaceFace>& fs, Point k, const std::map<Id, Id>* mapped) {
      for (const auto& loop : rim) {
        const Id facet = loop.second.front()[2], center = c.New(geometry.Project(facet, k));
        std::vector<std::pair<Id, Id>> edges;
        for (auto row : loop.second) {
          const Id a = mapped ? mapped->at(row[0]) : row[0], b = mapped ? mapped->at(row[1]) : row[1];
          edges.push_back({a, b});
          fs.push_back({{center, a, b}, row[2]});
        }
        c.VerifyLoop(edges);
      }
    };
    if(c.cylinder_grid.empty())close(inner, kernel, nullptr);
    clock.Set(report.transition_seconds);
    if(!c.cylinder_grid.empty()) {
      c.MetricCylinderFronts(inner,front,rim,p,kernel);
      close(inner,kernel,nullptr);
    } else for (;;) {
      c.Check();
      flow = c.Fit(inner, p.thickness);
      std::set<Id> ids;
      for (auto f : inner) for (Id n : f.v) ids.insert(n);
      auto clearance = [&](double step) {
        double distance = INFINITY;
        for (Id n : ids)
          distance = std::min(distance, geometry.ClosestAnalytic(*wall_authorities.begin(),
              flow.Apply(c.nodes.at(n), step)).distance);
        return distance;
      };
      const double before = clearance(0);
      if (!(clearance(1) > p.thickness * (1 + 1e-10)))
        throw std::runtime_error("Region transition cannot reach outside the original BL fade.");
      double lo = 0, hi = 1;
      for (unsigned k = 0; k < 48; ++k) {
        const double mid = .5 * (lo + hi);
        if (clearance(mid) >= p.thickness * (1 + 1e-10)) hi = mid;
        else lo = mid;
      }
      const double terminal = hi;
      auto span = [&](double step) {
        ++report.front_trials; c.Check();
        long double maximum = 0;
        std::map<Id, std::set<Id>> caps;
        std::set<Id> moving;
        for (auto f : inner) for (Id n : f.v)
          if (c.Wall(f.facet)) moving.insert(n); else caps[n].insert(f.facet);
        for (Id n : moving) {
          c.Check();
          const auto a = c.nodes.at(n); auto b = flow.Apply(a, step);
          for (Id f : caps[n]) b = geometry.Project(f, b);
          if (Norm(Sub(a,b)) > 0) maximum = std::max(maximum, target.edge(a,b));
        }
        if (!std::isfinite(maximum)) throw std::runtime_error("Region front has no finite metric span.");
        return maximum;
      };
      double step = terminal;
      if (span(step) > requested) {
        lo = 0; hi = step;
        for (unsigned iteration = 0; iteration < 24; ++iteration) {
          const double mid = .5 * (lo + hi);
          const auto length = span(mid);
          if (length <= requested) lo = mid; else hi = mid;
          if (length <= requested && length >= requested*(1-1e-6)) break;
        }
        step = lo;
      }
      if (!(step > 0) || !(clearance(step) > before))
        throw std::runtime_error("Region metric-directed front cannot make representable inward progress.");
      const auto last = c.MapFront(inner, flow, step);
      kernel = flow.Apply(kernel, step);
      wall.clear(); apices.clear();
      std::map<Edge, Id> side2;
      for (auto f : inner) if (c.Wall(f.facet)) {
        wall.push_back(f); SurfaceFace mapped = f;
        for (auto& n : mapped.v) n = last.at(n);
        apices.push_back(c.New(c.Balanced(c.Center(f), c.Center(mapped))));
      }
      for (const auto& loop : rim)
        for (auto row : loop.second) side2[Key(row[0], row[1])] = row[2];
      c.Band(wall, last, apices, side2, false);
      ++report.transition_fronts;
      inner.clear();
      for (auto f : wall) {
        for (auto& n : f.v) n = last.at(n);
        inner.push_back(f);
      }
      for (auto& loop : rim) for (auto& row : loop.second) {
        row[0] = last.at(row[0]); row[1] = last.at(row[1]);
      }
      close(inner, kernel, nullptr);
      if (step == terminal) break;
    }
    for (auto f : inner) if (!c.Wall(f.facet)) c.fresh.surface.push_back(f);
    clock.Set(report.junction_seconds);
    c.PlaceJunctions();
    const Id core = c.New(kernel);
    clock.Set(report.core_seconds);
    c.Core(inner,core);
    clock.Set(report.validation_seconds);
    c.Validate();
    output = std::move(c.fresh);
    reason.clear();
    return true;
  } catch (const std::exception& e) {
    if (limits.rejected && !c.fresh.cells.empty()) limits.rejected(c.fresh,e.what());
    reason = e.what();
    return false;
  }
}
}  // namespace SU2Native3D
