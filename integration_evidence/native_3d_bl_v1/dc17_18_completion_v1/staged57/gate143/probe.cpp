#include <array>
#include <cassert>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
using Id = int;
struct Point { double x,y,z; };
struct Node { Id id; Point p; };
struct Cell { std::array<Node,4> v; };
struct Face { Id facet; std::array<Id,3> v; };
struct Patch { std::vector<Cell> cells; std::vector<Face> surface; };
struct PlanarReference {
  std::array<double,8> errors{}; mutable std::vector<int> trace; int throws=-1; bool entity=false;
  std::unique_ptr<PlanarReference> PrivateParameters(const Patch&) const { return {}; }
  double SurfaceDeviation(Id id, const std::array<Point,3>&, bool) const {
    trace.push_back(id); if(id==throws) throw std::runtime_error("mock exception"); return errors.at(id);
  }
  bool EntityGeometry() const { return entity; }
};
enum class MetricChange { Coarsen, GeometryRefine, GeometryRepair, Repair };
enum class Phase { Construction, Improvement };
struct Options { double surface_tolerance=1.; };
struct Result { std::string reason; };
struct Outcome { bool accepted, progress; std::string reason; int guarded; };
namespace baseline {
long double GeometryGuidanceExcess(const Patch& patch,const PlanarReference& base_geometry,double tolerance) {
  auto parameters=base_geometry.PrivateParameters(patch);const auto& geometry=parameters?*parameters:base_geometry;
  std::map<Id,Point> nodes;
  for(const auto& c:patch.cells)for(auto n:c.v)nodes.emplace(n.id,n.p);
  long double excess=0;
  for(const auto& f:patch.surface) {
    const std::array<Point,3> p{nodes.at(f.v[0]),nodes.at(f.v[1]),nodes.at(f.v[2])};
    const double bound=geometry.SurfaceDeviation(f.facet,p,true);
    const long double error=std::max(0.,bound-tolerance);
    const long double ax=static_cast<long double>(p[1].x)-p[0].x,ay=static_cast<long double>(p[1].y)-p[0].y,az=static_cast<long double>(p[1].z)-p[0].z;
    const long double bx=static_cast<long double>(p[2].x)-p[0].x,by=static_cast<long double>(p[2].y)-p[0].y,bz=static_cast<long double>(p[2].z)-p[0].z;
    excess+=error*error*std::hypot(ay*bz-az*by,az*bx-ax*bz,ax*by-ay*bx);
  }
  return excess;
}
Outcome run(bool candidate, MetricChange kind, bool curved, Phase phase, const PlanarReference& geometry, const Patch& old, const Patch& fresh) {
  Options options; Result result; long double geometric_progress = false; int guarded_calls = 0;
  auto guarded_geometry=[&](bool v) { ++guarded_calls; return v; };
  auto geometric_change=[&](bool candidate,MetricChange kind) {
      if(!candidate || !curved)return candidate;
      // Entity refinement constructs valid cells; local surrogate descent is not a geometry permission.
      if(geometry.EntityGeometry()&&phase==Phase::Construction&&kind!=MetricChange::Coarsen)return true;
      try {
        const auto before=GeometryGuidanceExcess(old,geometry,options.surface_tolerance);
        const auto after=GeometryGuidanceExcess(fresh,geometry,options.surface_tolerance);
        geometric_progress=before>0 && after<before*(1-1e-12L);
        if(!std::isfinite(after) || (kind==MetricChange::GeometryRefine && !geometric_progress) ||
           (kind==MetricChange::Coarsen && after>0) ||
           (kind!=MetricChange::GeometryRefine && after>before &&
            !(kind==MetricChange::GeometryRepair && phase==Phase::Improvement))) {
          result.reason="Native3D candidate does not preserve/improve its frozen geometry error.";return guarded_geometry(false);
        }
        return true;
      }catch(const std::exception& e){result.reason=e.what();return guarded_geometry(false);}
    };
  bool accepted=geometric_change(candidate,kind);
  return {accepted, bool(geometric_progress), result.reason, guarded_calls};
}
}
namespace current {
long double GeometryGuidanceExcess(const Patch& patch,const PlanarReference& base_geometry,double tolerance,
                                  bool stop_on_violation=false) {
  auto parameters=base_geometry.PrivateParameters(patch);const auto& geometry=parameters?*parameters:base_geometry;
  std::map<Id,Point> nodes;
  for(const auto& c:patch.cells)for(auto n:c.v)nodes.emplace(n.id,n.p);
  long double excess=0;
  for(const auto& f:patch.surface) {
    const std::array<Point,3> p{nodes.at(f.v[0]),nodes.at(f.v[1]),nodes.at(f.v[2])};
    const double bound=geometry.SurfaceDeviation(f.facet,p,true);
    const long double error=std::max(0.,bound-tolerance);
    const long double ax=static_cast<long double>(p[1].x)-p[0].x,ay=static_cast<long double>(p[1].y)-p[0].y,az=static_cast<long double>(p[1].z)-p[0].z;
    const long double bx=static_cast<long double>(p[2].x)-p[0].x,by=static_cast<long double>(p[2].y)-p[0].y,bz=static_cast<long double>(p[2].z)-p[0].z;
    excess+=error*error*std::hypot(ay*bz-az*by,az*bx-ax*bz,ax*by-ay*bx);
    if(stop_on_violation && (excess>0 || !std::isfinite(excess)))return excess;
  }
  return excess;
}
Outcome run(bool candidate, MetricChange kind, bool curved, Phase phase, const PlanarReference& geometry, const Patch& old, const Patch& fresh) {
  Options options; Result result; long double geometric_progress = false; int guarded_calls = 0;
  auto guarded_geometry=[&](bool v) { ++guarded_calls; return v; };
  auto geometric_change=[&](bool candidate,MetricChange kind) {
      if(!candidate || !curved)return candidate;
      // Entity refinement constructs valid cells; local surrogate descent is not a geometry permission.
      if(geometry.EntityGeometry()&&phase==Phase::Construction&&kind!=MetricChange::Coarsen)return true;
      try {
        long double before,after;
        if(kind==MetricChange::Coarsen) {
          // A coarsening trial needs zero new excess; rejected trials need no old-surface score.
          after=GeometryGuidanceExcess(fresh,geometry,options.surface_tolerance,true);
          if(!std::isfinite(after) || after>0) {
            geometric_progress=false;
            result.reason="Native3D candidate does not preserve/improve its frozen geometry error.";return guarded_geometry(false);
          }
          before=GeometryGuidanceExcess(old,geometry,options.surface_tolerance);
        } else {
          before=GeometryGuidanceExcess(old,geometry,options.surface_tolerance);
          after=GeometryGuidanceExcess(fresh,geometry,options.surface_tolerance);
        }
        geometric_progress=before>0 && after<before*(1-1e-12L);
        if(!std::isfinite(after) || (kind==MetricChange::GeometryRefine && !geometric_progress) ||
           (kind==MetricChange::Coarsen && after>0) ||
           (kind!=MetricChange::GeometryRefine && after>before &&
            !(kind==MetricChange::GeometryRepair && phase==Phase::Improvement))) {
          result.reason="Native3D candidate does not preserve/improve its frozen geometry error.";return guarded_geometry(false);
        }
        return true;
      }catch(const std::exception& e){result.reason=e.what();return guarded_geometry(false);}
    };
  bool accepted=geometric_change(candidate,kind);
  return {accepted, bool(geometric_progress), result.reason, guarded_calls};
}
}

Patch patch(int offset) {
  Patch p; p.cells.push_back({{{{0,{0,0,0}},{1,{1,0,0}},{2,{0,1,0}},{3,{0,0,1}}}}});
  p.surface={ {offset,{0,1,2}}, {offset+1,{0,1,3}}, {offset+2,{0,2,3}}, {offset+3,{1,2,3}} }; return p;
}
int main() {
  auto old=patch(0), fresh=patch(4); constexpr double tol=1.;
  for(int code=0;code<6561;++code) {
    PlanarReference a; int n=code;
    for(auto& e:a.errors) {e=n%3; n/=3;}
    for(auto kind:{MetricChange::Coarsen,MetricChange::GeometryRefine,MetricChange::GeometryRepair,MetricChange::Repair})
    for(auto phase:{Phase::Construction,Phase::Improvement}) for(bool entity:{false,true}) for(bool candidate:{false,true}) for(bool curved:{false,true}) {
      a.entity=entity; a.trace.clear(); auto before=baseline::run(candidate,kind,curved,phase,a,old,fresh);
      PlanarReference b=a; b.trace.clear(); auto after=current::run(candidate,kind,curved,phase,b,old,fresh);
      assert(before.accepted==after.accepted); if(after.accepted) assert(before.progress==after.progress);
      if(kind!=MetricChange::Coarsen) { assert(a.trace==b.trace); assert(before.reason==after.reason && before.guarded==after.guarded); }
      if(candidate && curved && kind==MetricChange::Coarsen) {
        bool valid=true; for(int i=4;i<8;++i) valid &= a.errors[i]<=tol;
        assert(after.accepted==valid);
        if(!valid) for(int id:b.trace) assert(id>=4);
      }
    }
    a.trace.clear(); auto full=current::GeometryGuidanceExcess(fresh,a,tol);
    a.trace.clear(); auto fast=current::GeometryGuidanceExcess(fresh,a,tol,true);
    assert((full==0)==(fast==0));
    int expected=4; for(int i=4;i<8;++i) if(a.errors[i]>tol) {expected=i-4+1; break;}
    assert(int(a.trace.size())==expected);
  }
  // A nonfinite accumulated score must stop and reject immediately.
  PlanarReference inf; inf.errors[5]=std::numeric_limits<double>::infinity();
  auto nonfinite=current::run(true,MetricChange::Coarsen,true,Phase::Construction,inf,old,fresh);
  assert(!nonfinite.accepted && (inf.trace==std::vector<int>{4,5}));
  // A bad early fresh facet makes old and later fresh lookups unnecessary.
  PlanarReference x; x.errors[4]=2; x.throws=0;
  auto rejected=current::run(true,MetricChange::Coarsen,true,Phase::Construction,x,old,fresh);
  assert(!rejected.accepted && x.trace==std::vector<int>{4});
  x.trace.clear(); x.throws=7; rejected=current::run(true,MetricChange::Coarsen,true,Phase::Construction,x,old,fresh);
  assert(!rejected.accepted && x.trace==std::vector<int>{4});
  // A valid fresh patch still evaluates the old patch to establish progress.
  x.errors[4]=0; x.trace.clear(); x.throws=0;
  rejected=current::run(true,MetricChange::Coarsen,true,Phase::Construction,x,old,fresh);
  assert(!rejected.accepted && rejected.reason=="mock exception" && x.trace.back()==0);
  std::cout << "exhaustive geometry admission and trace assertions passed\n";
}
