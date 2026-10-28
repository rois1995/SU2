/*!
 * \file CNativeEngine3D.cpp
 * \brief Incremental frozen-target selection and distributed private planar refinement/coarsening.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeEngine3D.hpp"
#include "../../include/adaptation/CNativeOptimize3D.hpp"
#include "../../include/adaptation/CNativeRegionField3D.hpp"
#include "../../include/adaptation/CNativePointConstruction3D.hpp"
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../include/adaptation/CNativeGeometryBackend3D.hpp"
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>
#include <ctime>
#include <set>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <stdexcept>
#include <type_traits>
namespace SU2Native3D {
namespace {
// A successful separation is only a necessary geometry-error rejection, never admission.
bool CollapseOutsideTolerance(const Patch& old,Id removed,Id retained,const PlanarReference& geometry,double tolerance) {
  const auto* state=geometry.Parameters();
  if(!state || !state->Mobility().identity || !(std::isfinite(tolerance)&&tolerance>0))return false;
  const auto local=old.parameters.find(removed);
  const auto* point=local==old.parameters.end()?state->Find(removed):&local->second;
  if(!point)return false;
  auto* budget=geometry.AtlasReservation();
  if(!budget)throw std::runtime_error("Native collapse geometry screen lacks atlas reservation.");
  SU2NativeGeometry::NativeMappedGeometryBackend3D backend(state->Authority(),state->Mobility());
  SU2NativeGeometry::GeometryEvaluation evaluation;std::string reason;
  if(!backend.Evaluate(point->binding,std::nullopt,evaluation,*budget,reason))throw std::runtime_error(reason);
  const double threshold=std::nextafter(tolerance+evaluation.realization_error,INFINITY);
  if(!std::isfinite(threshold))return false;
  auto position=[&](Id id,Point& value) {
    const auto p=old.parameters.find(id);if(p!=old.parameters.end()){value=p->second.node.p;return true;}
    for(const auto& cell:old.cells)for(const auto& node:cell.v)if(node.id==id){value=node.p;return true;}
    return false;
  };
  bool any=false;const std::array<Point,3> sample{{evaluation.point,evaluation.point,evaluation.point}};
  for(const auto& face:old.surface) {
    const bool has_removed=std::find(face.v.begin(),face.v.end(),removed)!=face.v.end();
    const bool has_retained=std::find(face.v.begin(),face.v.end(),retained)!=face.v.end();
    if(has_removed && has_retained)continue;
    std::array<Point,3> triangle;
    for(unsigned i=0;i<3;++i)if(!position(face.v[i]==removed?retained:face.v[i],triangle[i]))return false;
    any=true;if(!SU2NativeGeometry::SourceFacesSeparated(sample,triangle,threshold))return false;
  }
  return any;
}
using Clock = std::chrono::steady_clock;
class Timer {
 public:
  Timer(double& value, bool enabled, double* parent=nullptr) : value(value), enabled(enabled), parent(parent) { if (enabled) start = Clock::now(); }
  ~Timer() { if (enabled) {const auto elapsed=std::chrono::duration<double>(Clock::now()-start).count();value+=elapsed;if(parent)*parent-=elapsed;} }
 private:
  double& value;
  bool enabled;
  double* parent;  // Keep a nested scope exclusive when its parent is also reported as a phase.
  Clock::time_point start;
};
class CpuTimer {
 public:
  CpuTimer(double& value, bool enabled) : value(value), enabled(enabled) { if (enabled) start=std::clock(); }
  ~CpuTimer() { if (enabled) value += double(std::clock()-start)/CLOCKS_PER_SEC; }
 private:
  double& value;
  bool enabled;
  std::clock_t start;
};
template <class M> size_t MapBytes(const M& m) {
  return transfer_memory::Mul(m.size(), sizeof(typename M::value_type) + 4 * sizeof(void*));
}
Face Key(Face f) { std::sort(f.begin(), f.end()); return f; }
std::array<Face, 4> Faces(const Cell& c) {
  const auto& v = c.v;
  return {{{v[1].id,v[2].id,v[3].id}, {v[0].id,v[3].id,v[2].id},
           {v[0].id,v[1].id,v[3].id}, {v[0].id,v[2].id,v[1].id}}};
}
constexpr size_t AtlasScratchBytes=16u<<20;
void RetainPhysicalCoverage(Patch& patch) {
  std::set<Face> faces;std::set<Edge> edges;
  for(auto face:patch.surface){std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());faces.insert(face.v);
    for(unsigned k=0;k<3;++k){Edge edge{face.v[k],face.v[(k+1)%3]};std::sort(edge.begin(),edge.end());edges.insert(edge);}}
  for(auto it=patch.coverage.faces.begin();it!=patch.coverage.faces.end();)if(!faces.count(it->first))it=patch.coverage.faces.erase(it);else ++it;
  for(auto it=patch.coverage.traces.begin();it!=patch.coverage.traces.end();)if(!edges.count(it->first))it=patch.coverage.traces.erase(it);else ++it;
}
Patch GeometryPatch(const Imported& imported,const PlanarReference* geometry=nullptr) {
  Patch patch;
  for (const auto& row : imported.cells) {
    patch.cells.push_back(row.cell);
    const auto faces = Faces(row.cell);
    for (size_t i = 0; i < 4; ++i) if (row.physical[i].present) patch.surface.push_back({faces[i],row.physical[i].facet});
  }
  if(geometry)geometry->ExportParameters(patch);
  return patch;
}
bool Same(Point a, Point b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
bool DonorCap(const std::string& reason) {
  return reason=="Original donor import exceeds patch budget." || reason=="Original donor union exceeds patch budget.";
}
// Requested altitude and normal footprint are optimization deficits; unresolved geometry is not.
double HeightScore(const Patch& patch,const WallHeights& heights,const PlanarReference& geometry) {
  if(heights.empty())return INFINITY;
  FirstHeightReport report;std::string reason;
  if(CheckFirstHeight(patch,heights,geometry,report,reason))return INFINITY;
  if(!report.measured || report.reference_clearance_failures || report.missing_cells)return -INFINITY;
  const double deficit=std::max(report.maximum_relative_error/heights.tolerance-1.,-3*report.minimum_barycentric);
  return 1/(1+std::max(deficit,2e-6));
}
using RepairFeature = std::pair<unsigned,std::array<Id,3>>;
// Call only for accepted surfaces or successful operators that have completed surface validation.
long double CertifiedGeometryExcess(const Patch& patch,const PlanarReference& base_geometry,double tolerance) {
  auto parameters=base_geometry.PrivateParameters(patch);const auto& geometry=parameters?*parameters:base_geometry;
  std::map<Id,Point> nodes;
  for(const auto& c:patch.cells)for(auto n:c.v)nodes.emplace(n.id,n.p);
  long double excess=0;
  for(const auto& f:patch.surface) {
    const std::array<Point,3> p{nodes.at(f.v[0]),nodes.at(f.v[1]),nodes.at(f.v[2])};
    const double bound=geometry.SurfaceErrorForValidatedFace(f.facet,p);
    const long double error=std::max(0.,bound-tolerance);
    const long double ax=static_cast<long double>(p[1].x)-p[0].x,ay=static_cast<long double>(p[1].y)-p[0].y,az=static_cast<long double>(p[1].z)-p[0].z;
    const long double bx=static_cast<long double>(p[2].x)-p[0].x,by=static_cast<long double>(p[2].y)-p[0].y,bz=static_cast<long double>(p[2].z)-p[0].z;
    excess+=error*error*std::hypot(ay*bz-az*by,az*bx-ax*bz,ax*by-ay*bx);
  }
  return excess;
}
bool SameSkin(const Patch& old,const Patch& fresh) {
  const auto before=Incidence(old.cells).Boundary();
  if(before!=Incidence(fresh.cells).Boundary())return false;
  std::map<Id,Point> a,b;
  for(const auto& cell:old.cells)for(auto n:cell.v)a.emplace(n.id,n.p);
  for(const auto& cell:fresh.cells)for(auto n:cell.v)b.emplace(n.id,n.p);
  for(const auto& face:before)for(Id id:face)if(!Same(a.at(id),b.at(id)))return false;
  return true;
}
struct Choice {
  Edge edge{};EdgeMeasure measure;bool active=false;Feature feature;bool geometric=false;
  long double priority_score=0;Id priority_cell=0;unsigned reservation_priority=0;
  uint64_t repair_epoch=0;
};
struct ProposalKey {
  Feature feature;int rank=0;size_t slot=0;bool geometric=false;double score=0;Id cell=0;uint64_t epoch=0;
  template<class S> void Fields(S& s){s(feature,rank,slot,geometric,score,cell,epoch);}
};
struct RepairReceipt {
  Feature feature;bool accepted=false;
  template<class S> void Fields(S& s){s(feature,accepted);}
};
class Selection {
 public:
  Selection(bool coarsening, bool repairing, const PlanarReference& geometry, EngineStats& stats, bool timings, double tolerance,const WallHeights& heights)
      : coarsening(coarsening), track_repairs(repairing), repair_event(repairing), geometry(geometry), stats(stats), timings(timings), tolerance(tolerance), heights(heights) {}
  bool coarsening, track_repairs, repair_event, prune_collapses = false, track_splits = false;
  const PlanarReference& geometry;
  EngineStats& stats;
  bool timings;
  double tolerance;
  const WallHeights& heights;
  std::set<std::tuple<long double,Id,Edge>> geometry_queue;
  // One construction generation contains the edges present at its entry, including edges of
  // cells whose metric has not been progressively seeded yet. Publications cannot extend it.
  std::set<Edge> construction_frontier;
  long double Priority(long double length) const { return coarsening ? length : -length; }
  bool Candidate(long double length) const {
    return coarsening ? length < CoarseningMetricEdge : length > MaximumMetricEdge;
  }
  void Queue(Edge edge, long double length, bool insert) {
    if (repair_event || !Candidate(length)) return;
    for (unsigned i=0; i<(coarsening ? 2u : 1u); ++i) {
      Edge key=edge; std::sort(key.begin(),key.end());
      const auto blocked=blocked_edges.find(edge[1]);
      if(insert && blocked!=blocked_edges.end() && blocked->second.count(edge[0])) {
        // Measuring another unchanged cell cannot reopen a rejected direction; Touch owns reactivation.
        queue.erase({Priority(length),edge});++stats.blocked_queue_skips;
      } else if(insert && !construction_frontier.count(edge))queue.erase({Priority(length),edge});
      else if (insert && prune_collapses && edges.at(key).forbidden[i])
        stats.collapse_filtered += queue.erase({Priority(length),edge});
      else if (insert) queue.insert({Priority(length),edge}); else queue.erase({Priority(length),edge});
      std::swap(edge[0],edge[1]);
    }
  }
  size_t invalid = 0, construction_invalid = 0, quality_invalid = 0, unmeasured_shape = 0, repair_bytes = 0;
  struct EdgeState { EdgeMeasure measure; size_t users = 0; std::array<size_t,2> forbidden{}; };
  struct CellDebt {
    long double q=0,j=0,size=0,height=0,geometry=0;
    bool q_measured=false,j_measured=false,height_measured=false,geometry_measured=true,height_unresolved=false;
    bool q_bad=false,j_bad=false,size_bad=false,height_bad=false;
  };
  StageProgress debt;
  bool progress_consistent=true,census_started=false;
  struct CellState { uint64_t version=0; CellDebt debt;std::array<Edge, 6> edges{}; std::array<unsigned char,6> forbidden{}; std::array<Id,4> vertices{}; std::array<long double,6> geometry_scores{}; bool valid = false,target_valid=false,geometry_invalid=false,construction_valid=false,quality_valid=false,shape_measured=false; };
  std::map<Id,CellState> cells;
  std::map<Edge,EdgeState> edges;
  std::set<std::pair<long double,Edge>> queue;
  std::map<Id,std::set<Id>> vertex_cells, blocked_edges;
  size_t vertex_links=0, blocked_links=0;
  std::map<Id,std::vector<RepairFeature>> cell_repairs;
  struct RepairDemand { size_t users=0;uint64_t epoch=0;bool blocked=false; };
  uint64_t repair_epoch=0;
  std::map<RepairFeature,RepairDemand> repair_users;
  std::set<std::pair<uint64_t,RepairFeature>> repair_queue;
  std::set<RepairFeature> repair_zero;
  size_t Bytes() const {
    return transfer_memory::Add(MapBytes(cells),MapBytes(edges),MapBytes(queue),MapBytes(construction_frontier),MapBytes(cell_repairs),MapBytes(repair_users),MapBytes(repair_queue),MapBytes(repair_zero),repair_bytes,
        MapBytes(geometry_queue),MapBytes(vertex_cells),MapBytes(blocked_edges),transfer_memory::Mul(transfer_memory::Add(vertex_links,blocked_links),sizeof(Id)+4*sizeof(void*)));
  }
  void Account(const CellState& row,bool add) {
    const auto count=[&](size_t& total,size_t value){if(add)total+=value;else total-=value;};
    const auto& d=row.debt;const long double sign=add?1.L:-1.L;
    count(debt.q_measured_cells,d.q_measured);count(debt.j_measured_cells,d.j_measured);
    count(debt.shape_measured_cells,d.q_measured && d.j_measured);
    count(debt.height_measured_cells,d.height_measured);count(debt.geometry_measured_cells,d.geometry_measured);
    count(debt.height_unresolved_cells,d.height_unresolved);
    count(debt.q_bad_cells,d.q_bad);count(debt.j_bad_cells,d.j_bad);count(debt.size_bad_cells,d.size_bad);
    count(debt.height_bad_cells,d.height_bad);count(debt.geometry_bad_cells,row.geometry_invalid);
    debt.q_hinge_squared+=sign*d.q;debt.j_hinge_squared+=sign*d.j;
    debt.size_hinge_squared+=sign*d.size;debt.height_hinge_squared+=sign*d.height;
    debt.geometry_excess_squared+=sign*d.geometry;
  }
  StageProgress Progress(size_t owned)const {
    StageProgress result=progress_consistent?debt:StageProgress{};
    result.cache_consistent=progress_consistent;result.owned_cells=owned;
    result.measured_cells=progress_consistent?cells.size():0;
    result.invalid_cells=progress_consistent?invalid:0;
    result.construction_invalid_cells=progress_consistent?construction_invalid:0;
    result.quality_invalid_cells=progress_consistent?quality_invalid:0;
    result.remaining_edges=queue.size()+geometry_queue.size();result.remaining_repairs=repair_queue.size();
    result.unmeasured_cells=owned-result.measured_cells;
    result.shape_unmeasured_cells=owned-result.shape_measured_cells;
    result.height_requested=!heights.empty();result.surface_error_requested=!geometry.ExtensionComponents().empty();
    result.height_unmeasured_cells=result.height_requested?owned-result.height_measured_cells:0;
    result.geometry_unmeasured_cells=owned-result.geometry_measured_cells;
    result.census_complete=result.measured_cells==owned && progress_consistent && census_started;
    result.shape_complete=result.shape_measured_cells==owned && progress_consistent && census_started;
    result.height_complete=(!result.height_requested || !result.height_unmeasured_cells) && progress_consistent;
    result.geometry_complete=!result.geometry_unmeasured_cells && progress_consistent;
    result.complete=result.census_complete && result.shape_complete && result.height_complete && result.geometry_complete;
    // Incremental subtraction can leave round-off below zero after a large number of updates.
    result.q_hinge_squared=std::max(0.L,result.q_hinge_squared);result.j_hinge_squared=std::max(0.L,result.j_hinge_squared);
    result.size_hinge_squared=std::max(0.L,result.size_hinge_squared);result.height_hinge_squared=std::max(0.L,result.height_hinge_squared);
    result.geometry_excess_squared=std::max(0.L,result.geometry_excess_squared);
    return result;
  }
  Choice Best(bool repairing) const {
    if (repairing) {
      if (repair_queue.empty()) return {};
      const auto& oldest=*repair_queue.begin();const auto& key=oldest.second;
      Choice choice; choice.active=true; choice.feature={key.second,key.first};choice.repair_epoch=oldest.first;return choice;
    }
    if (!geometry_queue.empty()) {
      const auto key=std::get<2>(*geometry_queue.begin());
      return {key,edges.at(key).measure,true,{},true,std::get<0>(*geometry_queue.begin()),std::get<1>(*geometry_queue.begin())};
    }
    if (queue.empty()) return {};
    const auto key = queue.begin()->second;
    Edge canonical=key; std::sort(canonical.begin(),canonical.end());
    return {key,edges.at(canonical).measure,true,{},false,Priority(edges.at(canonical).measure.length)};
  }
  std::vector<Choice> Batch(const Owned& working, size_t limit) {
    std::vector<Choice> result;
    std::set<Id> used;
    size_t scanned=0;
    for (auto it=queue.begin();it!=queue.end() && result.size()<limit && scanned++<MaximumDirectoryScan;) {
      const auto edge=(it++)->second;
      Edge canonical=edge; std::sort(canonical.begin(),canonical.end());
      const Choice choice{edge,edges.at(canonical).measure,true,{}};
      if (FilterCollapse(choice,working)) continue;
      const auto& star=vertex_cells.at(coarsening ? edge[1] : edge[0]);
      std::set<Id> vertices;
      bool disjoint=true, oversized=false;
      size_t cells=0, scanned_star=0;
      for (Id id:star) {
        if (++scanned_star>MaximumDirectoryScan) { oversized=true; break; }
        const auto& cell=working.at(id).cell;
        if (!coarsening && std::none_of(cell.v.begin(),cell.v.end(),[&](Node node){return node.id==edge[1];})) continue;
        if (++cells>MaximumCavityCells) { oversized=true; break; }
        for (auto node:cell.v) {
          vertices.insert(node.id);
          disjoint=disjoint && !used.count(node.id);
        }
      }
      if (oversized) { result.push_back(choice); break; }  // Complete-star admission reports the cap.
      // Local disjointness only reduces redundant proposals; reservation admits the complete remote stars.
      if (disjoint) { result.push_back(choice); used.insert(vertices.begin(),vertices.end()); }
    }
    return result;
  }
  void Cached(const Cell& cell, std::map<Edge,EdgeMeasure>& values) const {
    for (size_t i=0; i<4; ++i) for (size_t j=i+1; j<4; ++j) {
      const Edge key{std::min(cell.v[i].id,cell.v[j].id),std::max(cell.v[i].id,cell.v[j].id)};
      const auto found=edges.find(key);
      if (found!=edges.end()) values.emplace(key,found->second.measure);
    }
  }
  void Block(Edge edge, Feature feature, bool repairing) {
    if (repairing) { Service(feature,false);return; }
    ServiceEdge(edge);
    Edge canonical=edge;std::sort(canonical.begin(),canonical.end());
    const auto it = edges.find(canonical);
    if(it!=edges.end())queue.erase({Priority(it->second.measure.length),edge});
    // Rejection can reach an owner before progressive measurement discovers its edge.
    if (coarsening || track_splits) {
      const bool inserted=blocked_edges[edge[1]].insert(edge[0]).second;
      blocked_links+=inserted;stats.blocked_unmeasured+=inserted && it==edges.end();
    }
    if (track_splits && !coarsening) blocked_links+=blocked_edges[edge[0]].insert(edge[1]).second;
  }
  void Forget(Edge edge) {
    const auto found=blocked_edges.find(edge[1]);
    if (found==blocked_edges.end()) return;
    blocked_links-=found->second.erase(edge[0]);
    if (found->second.empty()) blocked_edges.erase(found);
  }
  void Touch(Id vertex) {
    // A healthy member of a rejected cavity may change without removing any bad-cell user.
    const auto star=vertex_cells.find(vertex);
    if(star!=vertex_cells.end())for(Id id:star->second) {
      const auto repairs=cell_repairs.find(id);if(repairs==cell_repairs.end())continue;
      for(const auto& key:repairs->second) {
        if(std::find(key.second.begin(),key.second.begin()+key.first,vertex)==key.second.begin()+key.first)continue;
        auto& demand=repair_users.at(key);
        if(!demand.blocked)continue;
        demand.blocked=false;demand.epoch=repair_epoch;
        repair_queue.insert({demand.epoch,key});
      }
    }
    const auto found=blocked_edges.find(vertex);
    if (found==blocked_edges.end()) return;
    for (Id retained:found->second) {
      Edge edge{retained,vertex}, key=edge; std::sort(key.begin(),key.end());
      const auto measured=edges.find(key);
      if (measured==edges.end())continue;
      if(!coarsening && !geometry.ExtensionComponents().empty()) {
        const auto star=vertex_cells.find(vertex);
        if(star!=vertex_cells.end())for(Id id:star->second) {
          const auto& row=cells.at(id);
          for(size_t k=0;k<row.edges.size();++k)if(row.edges[k]==key && row.geometry_scores[k]>0)
            if(construction_frontier.count(key))stats.geometry_reactivated+=geometry_queue.insert({-row.geometry_scores[k],id,key}).second;
        }
      }
      if(!Candidate(measured->second.measure.length))continue;
      const auto direction=retained==key[0] ? 0u : 1u;
      if (prune_collapses && measured->second.forbidden[direction]) continue;
      if (!coarsening) edge=key;
      if(!construction_frontier.count(edge))continue;
      const bool inserted=queue.insert({Priority(measured->second.measure.length),edge}).second;
      if (coarsening) stats.collapse_reactivated+=inserted;
    }
    blocked_links-=found->second.size(); blocked_edges.erase(found);
  }
  bool FilterCollapse(const Choice& choice, const Owned& working) {
    if (!coarsening || !prune_collapses || !choice.active) return false;
    if(!geometry.ContractionAllowed(choice.edge[1],choice.edge[0])) {
      Block(choice.edge,{},false);++stats.collapse_filtered;++stats.collapse_constraint_filtered;return true;
    }
    const auto star=vertex_cells.find(choice.edge[1]);
    if (star==vertex_cells.end()) throw std::logic_error("Missing native 3D local collapse star.");
    const Point apex=choice.edge[0]<choice.edge[1] ? choice.measure.a : choice.measure.b;
    {
      Timer timer(stats.collapse_geometry_seconds,timings);
      size_t scanned=0;
      for (Id id:star->second) {
        if (++scanned>MaximumDirectoryScan) break;
        const auto& row=working.at(id);
        if (std::any_of(row.cell.v.begin(),row.cell.v.end(),[&](Node n){return n.id==choice.edge[0];})) continue;
        if (LeavesFacet(row,choice.edge[1],apex)) {
          Block(choice.edge,{},false); ++stats.collapse_filtered; return true;
        }
        if(CollapseConstraintViolation(row,choice.edge[1],{choice.edge[0],apex},geometry,tolerance,heights)) {
          Block(choice.edge,{},false);++stats.collapse_filtered;++stats.collapse_constraint_filtered;return true;
        }
      }
    }
    Timer timer(stats.collapse_orientation_seconds,timings);
    size_t scanned=0;
    for (Id id:star->second) {
      if (++scanned>MaximumDirectoryScan) break;
      auto v=working.at(id).cell.v;
      if (std::any_of(v.begin(),v.end(),[&](Node n){return n.id==choice.edge[0];})) continue;
      for (auto& node:v) if (node.id==choice.edge[1]) node={choice.edge[0],apex};
      ++stats.collapse_orientation_checks;
      if (!(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0)) {
        Block(choice.edge,{},false); ++stats.collapse_orientation_filtered; return true;
      }
    }
    return false;
  }
  void Drop(Id id) {
    const auto found = cells.find(id);
    if (found==cells.end()) throw std::logic_error("Missing native 3D cached cell removal.");
    Account(found->second,false);
    invalid -= !found->second.valid;
    construction_invalid -= !found->second.construction_valid;
    quality_invalid -= !found->second.quality_valid;
    unmeasured_shape -= !found->second.shape_measured;
    for(size_t i=0;i<6;++i)geometry_queue.erase({-found->second.geometry_scores[i],id,found->second.edges[i]});
    for (Id vertex:found->second.vertices) {
      auto& star=vertex_cells.at(vertex);
      vertex_links-=star.erase(id);
      if (star.empty()) vertex_cells.erase(vertex);
    }
    RemoveRepairs(id);
    for (size_t slot=0;slot<found->second.edges.size();++slot) {
      const auto key=found->second.edges[slot];
      auto& state = edges.at(key);
      Queue(key,state.measure.length,false);
      for (unsigned direction=0;direction<2;++direction)
        state.forbidden[direction]-=bool(found->second.forbidden[slot] & (1u<<direction));
      if (--state.users==0) { Forget(key); Forget({key[1],key[0]}); edges.erase(key); }
      else Queue(key,state.measure.length,true);
    }
    cells.erase(found);
  }
  void RemoveRepairs(Id id) {
    const auto it=cell_repairs.find(id);if(it==cell_repairs.end())return;
    repair_bytes-=transfer_memory::Bytes(it->second);
    // Preserve waiting age and rejection state through transient zero-user counts until the publication completes.
    for(const auto& key:it->second)if(--repair_users.at(key).users==0)repair_zero.insert(key);
    cell_repairs.erase(it);
  }
  void ChangePhase(bool improvement,const Owned& working,const std::shared_ptr<const FrozenMetricIdentity>& identity) {
    track_repairs=repair_event=improvement;prune_collapses=false;queue.clear();geometry_queue.clear();construction_frontier.clear();
    for(auto& item:cells) {
      auto& row=item.second;const auto found=working.find(item.first);
      if(found==working.end()||found->second.version!=row.version)throw std::logic_error("Phase census lost a versioned owned row.");
      const auto& cell=found->second.cell;
      for(unsigned i=0;i<4;++i)if(cell.v[i].id!=row.vertices[i])throw std::logic_error("Phase census changed owned vertices without a version.");
      for(const auto& edge:row.edges) {const auto& cached=edges.at(edge).measure;const Node *a=nullptr,*b=nullptr;
        for(const auto& node:cell.v){if(node.id==edge[0])a=&node;if(node.id==edge[1])b=&node;}
        if(!a||!b||!cached.Reusable(a->p,b->p,identity))throw std::logic_error("Phase census changed edge coordinates or field without a version.");}
      const bool before=row.valid;invalid-=!before;
      row.target_valid=improvement ? (!row.debt.size_bad&&row.quality_valid) : !row.debt.size_bad;
      row.valid=row.target_valid&&!row.geometry_invalid;invalid+=!row.valid;
      if(improvement && !row.valid && !cell_repairs.count(item.first))AddRepairs(found->second.cell);
      else if(improvement && row.valid && cell_repairs.count(item.first))RemoveRepairs(item.first);
    }
    FinishRepairs();
    if(!improvement) {
      SeedFrontier(working);
      for(const auto& row:edges)Queue(row.first,row.second.measure.length,true);
      if(!coarsening)for(const auto& item:cells)for(size_t i=0;i<6;++i){const auto edge=item.second.edges[i];const auto blocked=blocked_edges.find(edge[1]);
        if(item.second.geometry_scores[i]>0 && construction_frontier.count(edge) && (blocked==blocked_edges.end()||!blocked->second.count(edge[0])))
          geometry_queue.insert({-item.second.geometry_scores[i],item.first,edge});}
    }
  }
  void RefreshQuality(const OwnedCell& owned,const PatchMetric& metric,bool valid,bool quality_valid,
                      const FirstHeightReport* height,bool height_valid,const std::shared_ptr<const FrozenMetricIdentity>& identity) {
    auto& row=cells.at(owned.cell.id);if(row.version!=owned.version)throw std::logic_error("Quality refresh found stale owned version.");
    if(metric.edges.size()!=6||!std::isfinite(metric.maximum_length))throw std::invalid_argument("Incomplete quality refresh edge measurements.");
    for(const auto& key:row.edges){const auto measured=metric.edges.find(key);if(measured==metric.edges.end())throw std::logic_error("Quality refresh omitted cached edge.");
      const auto& prior=edges.at(key).measure;const auto& fresh=measured->second;
      if(!prior.Reusable(fresh.a,fresh.b,identity)||!fresh.Reusable(prior.a,prior.b,identity)||prior.length!=fresh.length)
        throw std::logic_error("Quality refresh changed cached edge certificate.");}
    const bool previous_valid=row.valid;Account(row,false);invalid-=!row.valid;quality_invalid-=!row.quality_valid;unmeasured_shape-=!row.shape_measured;
    auto& d=row.debt;auto hinge=[](long double x){return std::max(0.L,x);};
    row.shape_measured=metric.shape_measured;row.quality_valid=quality_valid;
    d.q_measured=metric.shape_measured&&std::isfinite(metric.minimum_mean_ratio);d.j_measured=metric.shape_measured&&std::isfinite(metric.minimum_scaled_jacobian);
    d.q=d.j=0;d.q_bad=d.j_bad=false;
    if(d.q_measured){const auto r=hinge(1.L-static_cast<long double>(metric.minimum_mean_ratio)/MinimumMeanRatio);d.q=r*r;d.q_bad=metric.minimum_mean_ratio<MinimumMeanRatio;}
    if(d.j_measured){const auto r=hinge(1.L-static_cast<long double>(metric.minimum_scaled_jacobian)/MinimumScaledJacobian);d.j=r*r;d.j_bad=metric.minimum_scaled_jacobian<MinimumScaledJacobian;}
    d.height=0;d.height_measured=d.height_bad=d.height_unresolved=false;
    if(height&&!heights.empty()){d.height_measured=height->measured&&std::isfinite(height->maximum_relative_error)&&std::isfinite(height->minimum_barycentric);
      d.height_bad=!height_valid;d.height_unresolved=!d.height_measured||height->missing_cells||height->reference_clearance_failures;
      if(d.height_measured){const auto r=hinge(static_cast<long double>(height->maximum_relative_error)/heights.tolerance-1.L),f=hinge(-3.L*height->minimum_barycentric);d.height=r*r+f*f;}}
    row.target_valid=valid;row.valid=valid&&!row.geometry_invalid;Account(row,true);invalid+=!row.valid;quality_invalid+=!row.quality_valid;unmeasured_shape+=!row.shape_measured;
    if(previous_valid!=row.valid){if(row.valid)RemoveRepairs(owned.cell.id);else AddRepairs(owned.cell);FinishRepairs();}
  }
  void EnableCollapseFilter() {
    prune_collapses=true;
    for (const auto& row:edges) Queue(row.first,row.second.measure.length,true);
  }
  bool LeavesFacet(const OwnedCell& owned, Id removed, Point retained) {
    if(geometry.Mobility())return false; // Original chart seams are not physical constraints; full-star remapping proves replacement.
    for (size_t face=0;face<4;++face) {
      if (!owned.physical[face].present || owned.cell.v[face].id==removed) continue;
      ++stats.collapse_geometry_checks;
      if (!geometry.Contains(owned.physical[face].facet,retained)) return true;
    }
    return false;
  }
  unsigned char Forbidden(const OwnedCell& owned, Edge edge) {
    if (!coarsening) return 0;
    Timer timer(stats.collapse_geometry_seconds,timings);
    unsigned char blocked=0;
    for (unsigned direction=0;direction<2;++direction) {
      const Id retained=edge[direction], removed=edge[1-direction];
      Point point{};
      for (auto node:owned.cell.v) if (node.id==retained) point=node.p;
      if (LeavesFacet(owned,removed,point)) blocked|=1u<<direction;
    }
    return blocked;
  }
  void Add(const OwnedCell& owned, const PatchMetric& metric, bool valid, bool quality_valid, const std::string& reason,
           const FirstHeightReport* height=nullptr,bool height_valid=true) {
    const auto& cell=owned.cell;
    if (metric.edges.size()!=6 || !std::isfinite(metric.maximum_length))
      throw std::runtime_error(reason.empty()?"Incomplete native 3D initial/updated metric measurement.":reason);
    CellState row; row.version=owned.version;row.valid=row.target_valid=valid;row.shape_measured=metric.shape_measured;row.quality_valid=quality_valid;
    row.construction_valid=metric.maximum_length<=MaximumMetricEdge;
    auto hinge=[](long double value){return std::max(0.L,value);};
    auto& d=row.debt;
    d.q_measured=metric.shape_measured && std::isfinite(metric.minimum_mean_ratio);
    d.j_measured=metric.shape_measured && std::isfinite(metric.minimum_scaled_jacobian);
    if(d.q_measured) {
      const auto r=hinge(1.L-static_cast<long double>(metric.minimum_mean_ratio)/MinimumMeanRatio);d.q=r*r;
      d.q_bad=metric.minimum_mean_ratio<MinimumMeanRatio;
    }
    if(d.j_measured) {
      const auto r=hinge(1.L-static_cast<long double>(metric.minimum_scaled_jacobian)/MinimumScaledJacobian);d.j=r*r;
      d.j_bad=metric.minimum_scaled_jacobian<MinimumScaledJacobian;
    }
    d.size_bad=metric.maximum_length>MaximumMetricEdge;
    for(const auto& edge:metric.edges) {
      const auto r=hinge(edge.second.length/MaximumMetricEdge-1.L);d.size+=r*r;
    }
    if(height && !heights.empty()) {
      d.height_measured=height->measured && std::isfinite(height->maximum_relative_error) && std::isfinite(height->minimum_barycentric);
      d.height_bad=!height_valid;
      d.height_unresolved=!d.height_measured || height->missing_cells || height->reference_clearance_failures;
      if(d.height_measured) {
        const auto r=hinge(static_cast<long double>(height->maximum_relative_error)/heights.tolerance-1.L);
        const auto f=hinge(-3.L*height->minimum_barycentric);d.height=r*r+f*f;
      }
    }
    std::map<Edge,long double> geometry_scores;
    if (!geometry.ExtensionComponents().empty()) {
      const auto faces=Faces(cell);
      for(size_t slot=0;slot<4;++slot)if(owned.physical[slot].present) {
        std::array<Point,3> triangle{};
        for(size_t i=0;i<3;++i)for(auto n:cell.v)if(n.id==faces[slot][i])triangle[i]=n.p;
        const double error=geometry.SurfaceError(owned.physical[slot].facet,triangle);
        const auto allowed=tolerance;
        if(std::isfinite(error)) {
          const auto excess=hinge(static_cast<long double>(error)-allowed);d.geometry+=excess*excess;
        } else d.geometry_measured=false;
        if(error<=allowed)continue;
        row.valid=false;row.geometry_invalid=true;
        for(size_t i=0;i<3;++i)for(size_t j=i+1;j<3;++j) {
          const auto a=triangle[i],b=triangle[j];
          const Edge key{std::min(faces[slot][i],faces[slot][j]),std::max(faces[slot][i],faces[slot][j])};
          if(!std::isfinite(error))continue;
          const long double excess=static_cast<long double>(error)-allowed;
          const long double x=static_cast<long double>(b.x)-a.x,y=static_cast<long double>(b.y)-a.y,z=static_cast<long double>(b.z)-a.z;
          // Queue current geometry demand; the selected complete star owns projection and progress validation.
          geometry_scores[key]+=excess*excess*(x*x+y*y+z*z);
        }
      }
    }
    if(!d.geometry_measured)d.geometry=0;
    for (size_t i=0;i<4;++i) {
      row.vertices[i]=cell.v[i].id;
      vertex_links+=vertex_cells[row.vertices[i]].insert(cell.id).second;
    }
    size_t slot=0;
    for (const auto& measured : metric.edges) {
      const auto key=measured.first;
      row.geometry_scores[slot]=geometry_scores[key];
      const auto blocked=blocked_edges.find(key[1]);
      if(!coarsening && construction_frontier.count(key) && row.geometry_scores[slot]>0 &&
          (blocked==blocked_edges.end() || !blocked->second.count(key[0])))
        geometry_queue.insert({-row.geometry_scores[slot],cell.id,key});
      auto inserted=edges.emplace(key,EdgeState{measured.second,0});
      auto& state=inserted.first->second;
      if (!Same(state.measure.a,measured.second.a) || !Same(state.measure.b,measured.second.b) ||
          state.measure.length!=measured.second.length)
        throw std::logic_error("Inconsistent native 3D frozen edge measurement.");
      row.forbidden[slot]=Candidate(measured.second.length) ? Forbidden(owned,key) : 0;
      for (unsigned direction=0;direction<2;++direction)
        state.forbidden[direction]+=bool(row.forbidden[slot] & (1u<<direction));
      ++state.users; row.edges[slot++]=key;
      Queue(key,state.measure.length,true);
    }
    row.construction_valid=row.construction_valid && !row.geometry_invalid;
    const auto inserted=cells.emplace(cell.id,std::move(row));
    if (!inserted.second) throw std::logic_error("Duplicate native 3D cached cell.");
    Account(inserted.first->second,true);
    invalid += !inserted.first->second.valid;
    construction_invalid += !inserted.first->second.construction_valid;
    quality_invalid += !inserted.first->second.quality_valid;
    unmeasured_shape += !inserted.first->second.shape_measured;
    if (track_repairs && !inserted.first->second.valid) AddRepairs(cell);
  }
  void AddRepairs(const Cell& cell) {
    auto& repairs=cell_repairs[cell.id];
    if (!repairs.empty()) throw std::logic_error("Duplicate native 3D repair queue seed.");
    for (auto node:cell.v) repairs.push_back({1,{node.id,0,0}});
    for (auto edge:cells.at(cell.id).edges) repairs.push_back({2,{edge[0],edge[1],0}});
    for (auto face:Faces(cell)) repairs.push_back({3,Key(face)});
    for (auto key:repairs) {
      const auto inserted=repair_users.emplace(key,RepairDemand{0,repair_epoch,false});
      auto& demand=inserted.first->second;
      if(demand.users++==0 && !demand.blocked)repair_queue.insert({demand.epoch,key});
    }
    repair_bytes=transfer_memory::Add(repair_bytes,transfer_memory::Bytes(repairs));
  }
  void Service(Feature feature,bool accepted) {
    const RepairFeature key{feature.count,feature.vertices};
    const auto found=repair_users.find(key);if(found==repair_users.end())return;
    auto& demand=found->second;repair_queue.erase({demand.epoch,key});
    demand.epoch=repair_epoch;demand.blocked=!accepted;
    if(accepted && demand.users)repair_queue.insert({demand.epoch,key});
  }
  void FinishRepairs() {
    for(const auto& key:repair_zero) {
      const auto found=repair_users.find(key);
      if(found!=repair_users.end() && !found->second.users) {
        repair_queue.erase({found->second.epoch,key});repair_users.erase(found);
      }
    }
    repair_zero.clear();
  }
  bool HasCoarseningDemand()const {
    for(const auto& edge:edges)if(Candidate(edge.second.measure.length))for(unsigned direction=0;direction<2;++direction) {
      const auto retained=edge.first[direction],removed=edge.first[1-direction];
      const auto blocked=blocked_edges.find(removed);
      if(!edge.second.forbidden[direction] && (blocked==blocked_edges.end() || !blocked->second.count(retained)))return true;
    }
    return false;
  }
  void SeedFrontier(const Owned& working) {
    for(const auto& row:working)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
      Edge edge{row.second.cell.v[i].id,row.second.cell.v[j].id};std::sort(edge.begin(),edge.end());
      construction_frontier.insert(edge);
      if(coarsening){std::swap(edge[0],edge[1]);construction_frontier.insert(edge);}
    }
  }
  bool ServiceEdge(Edge edge) {
    const bool serviced=construction_frontier.erase(edge);
    const auto direction=edge;std::sort(edge.begin(),edge.end());
    const auto measured=edges.find(edge);
    if(measured!=edges.end())queue.erase({Priority(measured->second.measure.length),direction});
    const auto star=vertex_cells.find(edge[0]);
    if(star!=vertex_cells.end())for(Id id:star->second) {
      const auto& row=cells.at(id);
      for(size_t slot=0;slot<row.edges.size();++slot)if(row.edges[slot]==edge)
        geometry_queue.erase({-row.geometry_scores[slot],id,edge});
    }
    return serviced;
  }
  size_t ServiceClaimedVertex(Id vertex) {
    const auto star=vertex_cells.find(vertex);if(star==vertex_cells.end())return 0;
    size_t serviced=0;
    for(Id id:star->second) {
      const auto& row=cells.at(id);
      if(!coarsening) {
        for(const auto& edge:row.edges)if(construction_frontier.count(edge))serviced+=ServiceEdge(edge);
      } else {
        // Every collapse of this neighboring point imports a star containing the committed vertex.
        for(Id removed:row.vertices) {
          const auto& neighbors=vertex_cells.at(removed);
          for(Id neighbor:neighbors)for(Id retained:cells.at(neighbor).vertices)
            if(retained!=removed && construction_frontier.count({retained,removed}))serviced+=ServiceEdge({retained,removed});
        }
      }
    }
    return serviced;
  }
  void Reset(bool improvement) {
    cells.clear();edges.clear();queue.clear();geometry_queue.clear();construction_frontier.clear();vertex_cells.clear();
    cell_repairs.clear();repair_users.clear();repair_queue.clear();repair_zero.clear();
    invalid=construction_invalid=quality_invalid=unmeasured_shape=repair_bytes=vertex_links=0;
    debt={};progress_consistent=true;census_started=false;
    track_repairs=repair_event=improvement;prune_collapses=false;
  }
};
struct BlockedEdge {
  Edge edge{}; Feature feature;
  template <class S> void Fields(S& s) { s(edge,feature); }
};
void FieldCounts(EngineStats& stats, const FrozenField& field) {
  const auto& f=field.Statistics();
  stats.field_requests+=f.requests; stats.field_hits+=f.hits; stats.field_misses+=f.misses;
  // Edge-only construction can fail before any point query.
  stats.field_invalid_queries+=f.invalid_queries; stats.field_failures+=f.failures+f.edge_failures; stats.field_evictions+=f.evictions;
  stats.field_compositions+=f.compositions;
  stats.donor_hint_tests+=f.donor_hint_tests;stats.donor_hint_hits+=f.donor_hint_hits;
  stats.field_edges+=f.edge_requests;
  stats.edge_separated+=f.kernel.segment_separated;
  stats.field_build_seconds+=f.build_seconds; stats.field_query_seconds+=f.query_seconds;
  stats.field_cache_seconds+=f.cache_seconds; stats.field_search_seconds+=f.search_seconds;
  stats.field_interpolation_seconds+=f.interpolation_seconds; stats.field_composition_seconds+=f.composition_seconds;
  stats.edge_seconds+=f.edge_seconds; stats.edge_search_seconds+=f.edge_search_seconds;
  stats.edge_trace_seconds+=f.edge_trace_seconds; stats.edge_integral_seconds+=f.edge_integral_seconds;
  stats.edge_geometry_seconds+=f.edge_geometry_seconds; stats.edge_composition_seconds+=f.edge_composition_seconds;
}
struct FieldAccounting {
  EngineStats& stats;
  const FrozenField& field;
  // Keep attempted queries when surrounding metric/topology checks throw.
  ~FieldAccounting() { FieldCounts(stats,field); }
};
struct HeightPolicyRow {
  int marker=0;double height=0,growth=0,thickness=0;
  template<class S>void Fields(S& s){s(marker,height,growth,thickness);}
};
struct OptimizationOperations {uint64_t moves=0,insertions=0,collapses=0,swaps=0;};
bool OptimizedRepairPatch(const Patch& old,Id first,Id first_node,const RegionMetric& target,
                          Patch& fresh,std::string& reason,size_t& work,bool preserve_vertices,
                          const PlanarReference& geometry,const WallHeights& heights,double surface_tolerance,
                          OptimizationOperations& operations) {
  operations={};
  Owned local;std::map<Face,Id> physical;for(const auto& f:old.surface)physical.emplace(Key(f.v),f.facet);
  for(auto c:old.cells){c.id=local.size();OwnedCell row;row.cell=c;const auto faces=Faces(c);
    for(unsigned k=0;k<4;++k){const auto found=physical.find(Key(faces[k]));if(found!=physical.end())row.physical[k]={true,found->second};}
    local.emplace(c.id,row);}
  auto parameters=geometry.PrivateParameters(old);
  OptimizeOptions options;options.maximum_work=work;options.cache_entries=8*MaximumReplacementCells;
  options.boundary=false;options.smoothing=!preserve_vertices;options.collapses=!preserve_vertices;
  options.geometry=parameters?parameters.get():&geometry;options.wall_heights=heights;options.surface_tolerance=surface_tolerance;
  OptimizeReport report;Id node=first_node,cell=local.size();
  const bool changed=OptimizeMetricMesh(local,target,node,cell,options,report);
  if(report.trials>work)throw std::logic_error("Native repair optimizer exceeded its work reservation.");
  if(node-first_node>report.trials)throw std::logic_error("Native repair optimizer exceeded its node reservation.");
  work=report.trials;
  if(!changed || local.size()>MaximumReplacementCells)return false;
  Patch proposal;proposal.parameters=old.parameters;proposal.coverage=old.coverage;
  for(const auto& p:report.parameters)proposal.parameters.insert_or_assign(p.first,p.second);
  for(const auto& row:local){auto c=row.second.cell;c.id=first+proposal.cells.size();proposal.cells.push_back(c);
    const auto faces=Faces(c);for(unsigned k=0;k<4;++k)if(row.second.physical[k].present)proposal.surface.push_back({faces[k],row.second.physical[k].facet});}
  for(const auto& row:report.coverage.faces)proposal.coverage.faces.insert_or_assign(row.first,row.second);
  for(const auto& row:report.coverage.traces)proposal.coverage.traces.insert_or_assign(row.first,row.second);
  RetainPhysicalCoverage(proposal);
  auto final_parameters=options.geometry->PrivateParameters(proposal);
  if(final_parameters)final_parameters->ExportParameters(proposal);
  std::map<Face,Face> expected;
  for(auto face:Incidence(old.cells).Boundary())if(!physical.count(Key(face)))expected.emplace(Key(face),face);
  for(auto face:proposal.surface){std::rotate(face.v.begin(),std::min_element(face.v.begin(),face.v.end()),face.v.end());expected.emplace(Key(face.v),face.v);}
  std::vector<Face> skin;for(const auto& f:expected)skin.push_back(f.second);
  if(!ValidatePrescribedInterface(old.cells,proposal.cells,skin,reason))return false;
  fresh=std::move(proposal);
  operations={report.moves,report.shell_insertions+report.neighbourhood_insertions+report.length_insertions,
              report.collapses+report.boundary_collapses,report.edge_removals+report.length_removals+report.face_swaps+report.boundary_flips};
  return true;
}
EngineResult Adapt(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                   const PlanarReference& accepted_geometry, MetricComposition composition, EngineOptions options, MetricChange intent, bool bidirectional=false) {
  struct RestoreAdmission {ShapeAdmission previous=GetShapeAdmission();~RestoreAdmission(){SetShapeAdmission(previous);}} admission;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  using Phase=EnginePhase;
  Phase phase=Phase::Construction;
  bool region_rebuilt=false,improvement_changed=false,construction_changed=false,construction_handoff_progress=false;
  bool construction_generation_changed=false;
  uint64_t phase_start_round=0;
  if(options.stage_observer)options.caller_bytes=transfer_memory::Add(options.caller_bytes,options.stage_observer_bytes);
  const auto started=Clock::now();
  const auto started_cpu=options.timings ? std::clock() : std::clock_t{};
  std::shared_ptr<BoundaryParameterState> private_parameters;std::shared_ptr<const PlanarReference> private_geometry;
  SU2NativeGeometry::AtlasBudget atlas_budget;std::unique_ptr<PlanarReference> atlas_geometry;
  const SU2NativeGeometry::BoundaryCoverage no_coverage;
  CLocalFailure parameter_failure;size_t parameter_bound=0;
  const unsigned mapped=accepted_geometry.Mapped();
  const auto mapped_low=CPassiveComm::Allreduce(mapped,CPassiveComm::Op::MIN,world.comm),mapped_high=CPassiveComm::Allreduce(mapped,CPassiveComm::Op::MAX,world.comm);
  if(mapped_low!=mapped_high)parameter_failure.Set(1,0,"Native3D parameter geometry mode differs across workers.");
  if(mapped)try {
    const auto* parameters=accepted_geometry.Parameters();
    // Stage clones are boundary-only and admitted once, never per transaction. Private overlays are bounded.
    parameter_bound=transfer_memory::Add(options.caller_bytes,accepted_geometry.ResidentBytes(),
        transfer_memory::Mul(parameters->ResidentBytes(),2),AtlasScratchBytes,MapBytes(accepted),transfer_memory::Bytes(original));
    if(parameter_bound==SIZE_MAX || parameter_bound>options.memory_ceiling)throw std::length_error("Native boundary parameter stage exceeds storage admission.");
    private_parameters=std::make_shared<BoundaryParameterState>(*parameters);
    private_geometry=accepted_geometry.WithParameterState(private_parameters);
    options.caller_bytes=transfer_memory::Add(options.caller_bytes,AtlasScratchBytes);
    if(private_parameters->Mobility().identity||!private_parameters->Coverage().faces.empty()||!private_parameters->Coverage().traces.empty()) {
      atlas_budget.caller_bytes=parameter_bound-AtlasScratchBytes;atlas_budget.maximum_bytes=parameter_bound;
      // Half holds the nested planner/reconstruction arithmetic arrays; half holds correspondence records.
      size_t low=0,high=AtlasScratchBytes*8;
      while(low<high){const size_t bits=low+(high-low+1)/2;
        if(transfer_memory::Mul(2,SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits))<=AtlasScratchBytes/2)low=bits;else high=bits-1;}
      if(!low)throw std::length_error("Native3D atlas integer workspace has no admitted capacity.");atlas_budget.maximum_integer_bits=low;
      atlas_geometry=private_geometry->WithAtlasReservation(atlas_budget);
    }
  }catch(const std::exception& e){parameter_failure.Set(3,0,e.what());}
  if(mapped_high) {
    const auto failure=world.elect(parameter_failure);
    if(failure.any){EngineResult rejected;rejected.reason=failure.message;rejected.stats.stop=EngineStop::Failure;rejected.stats.maximum_work_bytes=parameter_bound;return rejected;}
  }
  const auto& geometry=atlas_geometry?*atlas_geometry:private_geometry?*private_geometry:accepted_geometry;
  const double parameter_setup=options.timings?std::chrono::duration<double>(Clock::now()-started).count():0;
  const bool curved=!geometry.ExtensionComponents().empty();
  const bool standalone_coarsening=intent==MetricChange::Coarsen;
  bool coarsening=standalone_coarsening;
  bool repairing=false;
  MetricChange construction_kind=coarsening ? MetricChange::Coarsen : MetricChange::Refine;
  const auto tracked_bytes_begin=world.bytes_sent;
  const auto tracked_calls_begin=world.collective_calls;
  const double tracked_seconds_begin=world.collective_seconds;
  EngineResult result;
  auto& stats=result.stats;stats.bidirectional=bidirectional;stats.parameter_setup_seconds=parameter_setup;stats.initialization_seconds=parameter_setup;stats.maximum_work_bytes=parameter_bound;
  Owned working;
  Selection selection(coarsening,repairing,geometry,stats,options.timings,options.surface_tolerance,options.wall_heights);
  std::map<Edge,EdgeMeasure> retired_edges,campaign_edges;
  std::map<Id,std::set<Id>> inactive_blocked_edges;size_t inactive_blocked_links=0;
  std::array<bool,2> length_blocks_dirty{{false,false}};bool length_pair_changed=false;
  std::shared_ptr<const FrozenMetricIdentity> field_identity;
  // Joint split feasibility depends on neighboring vertex stars, even when the edge itself is unchanged.
  selection.track_splits=bool(composition) || curved;
  // Every adaptation can enter quality improvement; rejected repair support must be invalidated
  // even for flat, uncomposed refinement/repair events.
  auto directory=std::make_unique<Directory>(world,true,geometry.Parameters());
  DonorImport source(world,options.timings);
  struct CoarseningCheckpoint {
    Owned cells;
    std::unique_ptr<BoundaryParameterState> parameters;
    std::unique_ptr<Directory> directory;
    std::map<Id,std::set<Id>> blocked;
    std::set<Edge> edges,attempted;
    std::set<Id> changed;
    size_t blocked_links=0,global_cells=0,collapses=0,splits=0;
    bool valid=false,trial=false;
    size_t Bytes()const {
      return transfer_memory::Add(MapBytes(cells),MapBytes(blocked),MapBytes(edges),MapBytes(attempted),MapBytes(changed),
          transfer_memory::Mul(blocked_links,sizeof(Id)+4*sizeof(void*)),
          parameters ? parameters->ResidentBytes() : 0,directory ? transfer_memory::Add(sizeof(Directory),directory->ResidentBytes()) : 0);
    }
  } checkpoint;
  DirectoryStats retired_directory;
  auto retire_directory=[&](const DirectoryStats& old) {
    retired_directory.collision_seconds+=old.collision_seconds;
    retired_directory.collision_builds+=old.collision_builds;
    retired_directory.collision_candidates+=old.collision_candidates;
    retired_directory.collision_checks+=old.collision_checks;
    retired_directory.elections+=old.elections;
    retired_directory.election_seconds+=old.election_seconds;
    retired_directory.parameter_agreement_seconds+=old.parameter_agreement_seconds;
    retired_directory.parameter_publication_seconds+=old.parameter_publication_seconds;
    retired_directory.parameter_edits+=old.parameter_edits;
    retired_directory.parameter_faces+=old.parameter_faces;
    retired_directory.parameter_payload_bytes+=old.parameter_payload_bytes;
    retired_directory.change_scanned+=old.change_scanned;
    retired_directory.change_sent+=old.change_sent;
    retired_directory.change_received+=old.change_received;
    retired_directory.change_seconds+=old.change_seconds;
    retired_directory.maximum_work_bytes=std::max(retired_directory.maximum_work_bytes,old.maximum_work_bytes);
  };
  size_t accepted_bytes=MapBytes(accepted);
  auto selection_bound=[&](bool repairs) { return transfer_memory::Add(sizeof(Selection::CellDebt),
      (coarsening || bidirectional) ? (repairs ? 8192u : 5120u) : (repairs ? 6144u : 3072u)); };
  std::vector<Id> initial,eligible_extensions;
  auto vote=[&](const CLocalFailure& local) {
    stats.atlas_work=atlas_budget.work;stats.atlas_peak_bytes=atlas_budget.peak_bytes;
    // Reserve this engine-local severity to transport typed work refusal without another collective.
    constexpr unsigned short atlas_work_refusal=4;
    auto pending=local;
    if(atlas_budget.work_exhausted)pending.Set(atlas_work_refusal,0,
        "Native3D shared atlas work budget exhausted; feasibility is undetermined.");
    const auto elected=world.elect(pending);
    if (elected.any) result.reason=elected.message;
    if(elected.any && elected.severity==atlas_work_refusal){
      // A quota refusal need not be the only failure; retain the ordinary elected diagnostic too.
      const auto original=world.elect(local);
      if(original.any)result.reason+=" Original failure: "+original.message;
      result.valid=false;stats.stop=EngineStop::WorkBudget;
    }
    return !elected.any;
  };
  size_t batch_bytes=0;
  auto bytes=[&]() {
    return transfer_memory::Add(options.caller_bytes,MapBytes(options.wall_heights),MapBytes(options.region_walls),accepted_bytes,batch_bytes,MapBytes(working),checkpoint.Bytes(),
        transfer_memory::Bytes(original),transfer_memory::Bytes(initial),transfer_memory::Bytes(eligible_extensions),transfer_memory::Bytes(result.splits),transfer_memory::Bytes(result.collapses),
        selection.Bytes(),MapBytes(retired_edges),MapBytes(campaign_edges),MapBytes(inactive_blocked_edges),
        transfer_memory::Mul(inactive_blocked_links,sizeof(Id)+4*sizeof(void*)),field_identity ? 64 : 0);
  };
  auto real_bytes=[&]() {const auto live=bytes();return atlas_geometry&&live!=SIZE_MAX?live-AtlasScratchBytes:live;};
  auto admit=[&](size_t extra) {
    const auto peak=transfer_memory::Add(bytes(),source.ResidentBytes(),directory->ResidentBytes(),extra);
    stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
    CLocalFailure failure;
    if (peak==SIZE_MAX || peak>options.memory_ceiling) failure.Set(3,0,"Native 3D adaptation working storage exceeds budget.");
    const bool admitted=vote(failure);
    if(admitted&&atlas_geometry){atlas_budget.caller_bytes=peak-AtlasScratchBytes;atlas_budget.maximum_bytes=peak;}
    return admitted;
  };
  auto coalesce=[&](std::vector<Choice>& choices) {
    const bool collapse=coarsening && !curved && !composition;
    const bool concurrent_refine=(repairing || (!coarsening && (curved || composition)) || (bidirectional && coarsening)) && options.candidate_batch>1;
    if(world.size==1 || (!repairing && !collapse && !concurrent_refine))return true;
    const auto feature=[&](const Choice& c){return repairing?c.feature:Feature{{c.edge[0],c.edge[1],0},2};};
    // Replicated vertex/edge/face demands are one proposal; directed collapses retain distinct endpoints.
    const auto scratch=transfer_memory::Mul(transfer_memory::Mul(world.size,MaximumCandidateBatch),
        8*sizeof(ProposalKey)+8*sizeof(void*));
    if(!admit(scratch))return false;
    CLocalFailure failure;std::vector<std::vector<ProposalKey>> to;
    try {
      if(choices.size()>MaximumCandidateBatch)throw std::logic_error("Native edge proposal table exceeds its bound.");
      std::vector<ProposalKey> local;
      for(size_t i=0;i<choices.size();++i)local.push_back({feature(choices[i]),world.rank,i,choices[i].geometric,
          double(choices[i].priority_score),choices[i].priority_cell,choices[i].repair_epoch});
      to.assign(world.size,local);
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return false;bool admitted=false;
    auto all=world.exchange(to,&admitted,options.memory_ceiling,
        transfer_memory::Add(bytes(),source.ResidentBytes(),directory->ResidentBytes(),scratch));
    if(!admitted){result.reason="Native edge proposal exchange exceeds storage.";return false;}
    try {
      if(all.size()>std::numeric_limits<unsigned>::max())throw std::logic_error("Native edge proposal ordering exceeds its bound.");
      for(const auto& key:all)
        if(key.rank<0 || key.rank>=world.size || key.slot>=MaximumCandidateBatch || key.feature.count<1 || key.feature.count>3 || !std::isfinite(key.score))
          throw std::logic_error("Invalid native edge proposal identity/priority.");
      for(const auto& key:all)for(unsigned i=0;i<key.feature.count;++i)for(unsigned j=i+1;j<key.feature.count;++j)
        if(key.feature.vertices[i]==key.feature.vertices[j])throw std::logic_error("Repeated native proposal feature vertex.");
      // Demand priority precedes ownership; this score orders search, never certifies a mesh.
      std::sort(all.begin(),all.end(),[&](const auto& a,const auto& b) {
        if(repairing && a.epoch!=b.epoch)return a.epoch<b.epoch;
        return std::make_tuple(!a.geometric,a.score,a.cell,a.feature.count,a.feature.vertices,a.rank,a.slot)<
               std::make_tuple(!b.geometric,b.score,b.cell,b.feature.count,b.feature.vertices,b.rank,b.slot);
      });
      std::vector<Choice> unique;
      for(size_t i=0;i<choices.size();++i) {
        const auto owner=std::find_if(all.begin(),all.end(),[&](const auto& key){const auto f=feature(choices[i]);return key.feature.count==f.count && key.feature.vertices==f.vertices;});
        if(owner==all.end())throw std::logic_error("Missing native edge proposal.");
        if(owner->rank!=world.rank || owner->slot!=i)++stats.proposal_duplicates;
        else {
          if(repairing && !concurrent_refine && owner!=all.begin())continue;
          if(concurrent_refine || repairing)choices[i].reservation_priority=unsigned(owner-all.begin());
          if(repairing)choices[i].repair_epoch=owner->epoch;
          unique.push_back(choices[i]);
        }
      }
      choices.swap(unique);
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    return vote(failure);
  };
  auto service_frontier=[&](Feature feature,bool active,bool accepted,
                            const std::array<RepairReceipt,MaximumCandidateBatch>* extra=nullptr,size_t count=0) {
    // Only the bounded elected frontier is replicated, never the resident demand queues.
    const bool certify_support=coarsening && !repairing && checkpoint.valid;
    const auto scratch=transfer_memory::Add(transfer_memory::Mul(world.size,sizeof(std::vector<RepairReceipt>)),
        transfer_memory::Mul(transfer_memory::Mul(world.size,MaximumCandidateBatch+1),4*sizeof(RepairReceipt)));
    if(!admit(transfer_memory::Add(scratch,
        certify_support ? transfer_memory::Mul(transfer_memory::Mul(world.size,MaximumCandidateBatch+1),sizeof(Edge)+4*sizeof(void*)) : 0)))return false;
    CLocalFailure failure;std::vector<std::vector<RepairReceipt>> to;
    try {
      if(count>MaximumCandidateBatch || (count && !extra))throw std::logic_error("Native frontier receipt list exceeds its bound.");
      to.resize(world.size);
      for(auto& bucket:to) {
        if(active)bucket.push_back({feature,accepted});
        for(size_t i=0;i<count;++i)bucket.push_back(extra->at(i));
      }
    }
    catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return false;
    bool admitted=false;
    auto receipts=world.exchange(to,&admitted,options.memory_ceiling,
        transfer_memory::Add(bytes(),source.ResidentBytes(),directory->ResidentBytes(),scratch));
    if(!admitted){result.reason="Native repair service receipts exceed storage budget.";return false;}
    try {
      if(receipts.size()>transfer_memory::Mul(world.size,MaximumCandidateBatch+1))throw std::logic_error("Native repair service receipts exceed frontier bound.");
      if(certify_support)std::sort(receipts.begin(),receipts.end(),[](const auto& a,const auto& b) {
        return std::tie(a.feature.count,a.feature.vertices,a.accepted)<std::tie(b.feature.count,b.feature.vertices,b.accepted);
      });
      for(const auto& receipt:receipts)
        if(receipt.feature.count<1 || receipt.feature.count>3 || (!repairing && receipt.feature.count!=2))
          throw std::logic_error("Invalid native repair service receipt.");
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(certify_support && !vote(failure))return false;
    for(const auto& receipt:receipts) {
      if(!certify_support && failure.Failed())break;
      const bool checkpoint_support=certify_support &&
          !world.maximum(checkpoint.changed.count(receipt.feature.vertices[1])!=0);
      try {
        if(checkpoint_support) {
          const Edge edge{receipt.feature.vertices[0],receipt.feature.vertices[1]};auto key=edge;std::sort(key.begin(),key.end());
          if(checkpoint.edges.count(key))checkpoint.attempted.insert(edge);
        }
        if(repairing)selection.Service(receipt.feature,receipt.accepted);
        else {
          if(receipt.feature.count!=2)throw std::logic_error("Invalid native construction service receipt.");
          selection.ServiceEdge({receipt.feature.vertices[0],receipt.feature.vertices[1]});
        }
      }catch(const std::exception& e){failure.Set(2,0,e.what());}
      // Every rank reaches the next receipt support vote even after a local allocation failure.
      if(certify_support && !vote(failure))return false;
    }
    return certify_support || vote(failure);
  };
  std::string last_rejection;
  uint64_t progress_at_window=0, attempted_at_window=0, deferred_at_window=0;
  auto height_check=[&](const Patch& patch,std::string& reason,bool provisional=false,FirstHeightReport* measured=nullptr) {
    if(options.wall_heights.empty())return true;
    ++stats.height_checks;FirstHeightReport report;
    const bool valid=CheckFirstHeight(patch,options.wall_heights,geometry,report,reason);
    if(measured)*measured=report;
    stats.height_rejected+=report.height_failures || report.missing_cells;
    stats.footprint_rejected+=report.footprint_failures!=0;
    if(provisional && GetShapeAdmission()==ShapeAdmission::ValidityOnly && report.measured &&
        !report.reference_clearance_failures && !report.missing_cells){reason.clear();return true;}
    return valid;
  };
  auto geometry_trial=[&](bool valid) {
    ++stats.geometry_trials; stats.geometry_rejected+=!valid; return valid;
  };
  auto metric_trial=[&](bool valid,const MetricDiagnosis& diagnosis) {
    ++stats.metric_trials;
    if (!valid) {
      stats.metric_shape_rejected+=diagnosis.failure==MetricFailure::Shape;
      stats.metric_progress_rejected+=diagnosis.failure==MetricFailure::Progress;
      stats.metric_measurement_rejected+=diagnosis.failure==MetricFailure::Measurement;
    }
    return valid;
  };
  const bool observe_rejections=world.maximum(bool(options.rejected_trial) || bool(options.transaction_observer));
  const bool observe_state=world.maximum(bool(options.rejected_state));
  const bool observe_stage=world.maximum(bool(options.stage_observer));
  bool stage_storage_admitted=false,stage_observer_failed=false,construction_entry_reported=false;
  auto observe_progress=[&](const char* stage) {
    if(!observe_stage || !stage_storage_admitted || stage_observer_failed)return !stage_observer_failed;
    CLocalFailure failure;
    if(options.stage_observer)try {
      const auto progress=selection.Progress(working.size());
      options.stage_observer({stage,phase,stats.rounds,result.valid,progress.complete,progress,stats,result.reason,
          working,original,geometry,composition,options.edge_breakpoints,options.wall_heights,options.region_walls,
          options.surface_tolerance,options.maximum_seconds,std::chrono::duration<double>(Clock::now()-started).count(),
          options.maximum_rounds,options.maximum_work,options.memory_ceiling,options.caller_bytes,options.stage_observer_bytes});
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    const auto elected=world.elect(failure);
    if(elected.any) {
      stage_observer_failed=true;result.valid=false;stats.stop=EngineStop::Failure;
      result.reason+=" Stage progress observer failed: "+elected.message;
      return false;
    }
    return true;
  };
  bool transaction_captured=false;
  bool layer_active=false,region_active=false;Clock::time_point layer_started,region_started;
  auto finish=[&]() {
    // Directory stages can refuse shared work and return directly here without an engine vote.
    const auto prior_reason=result.reason;const auto prior_stop=stats.stop;
    if(!vote(CLocalFailure{})) {
      result.valid=false;
      if(prior_stop==EngineStop::WorkBudget && !prior_reason.empty())result.reason=prior_reason;
      else if(!prior_reason.empty() && prior_reason!=result.reason)result.reason+=" Prior failure: "+prior_reason;
    }
    if(region_active && options.timings)stats.reconstruction_seconds+=std::chrono::duration<double>(Clock::now()-region_started).count();
    region_active=false;
    if(layer_active && options.timings)stats.layer_seconds+=std::chrono::duration<double>(Clock::now()-layer_started).count();
    layer_active=false;
    if(result.valid && geometry.EntityGeometry()) {
      CLocalFailure geometryFailure;
      try {for(const auto& row:geometry.Parameters()->GeometryCertificates(atlas_budget,true))if(!row.second.within_tolerance)
        geometryFailure.Set(1,std::get<0>(row.first),"Complete working surface does not meet the prescribed two-sided geometry tolerance.");}
      catch(const std::exception& e){geometryFailure.Set(2,0,e.what());}
      if(!vote(geometryFailure)){result.valid=false;if(stats.stop==EngineStop::None)stats.stop=EngineStop::Failure;}
    }
    if(result.valid && !options.wall_heights.empty()) {
      CLocalFailure heightFailure;
      for(const auto& row:working) {
        std::string reason;
        if(!height_check(GeometryPatch(Imported{{row.second},true,{}}, &geometry),reason))heightFailure.Set(1,row.first,reason);
      }
      if(!vote(heightFailure)){result.valid=false;if(stats.stop==EngineStop::None)stats.stop=EngineStop::Failure;}
    }
    result.rebuilt_domain=result.valid && region_rebuilt;
    if (stats.stop==EngineStop::None) stats.stop=result.valid ? EngineStop::Complete : EngineStop::Failure;
    if(observe_state) {
      CLocalFailure observed;
      if(!result.valid && options.rejected_state)try {options.rejected_state(working,geometry,result.reason);}
      catch(const std::exception& e){observed.Set(2,0,e.what());}
      const auto elected=world.elect(observed);
      if(elected.any) {
        result.valid=false;stats.stop=EngineStop::Failure;
        result.reason+=" Rejected-state diagnostic failed: "+elected.message;
      }
    }
    stats.final_owned_cells=working.size();
    stats.cached_measured_cells=selection.cells.size();
    stats.cached_invalid_cells=selection.invalid;
    stats.remaining_edges=selection.queue.size()+selection.geometry_queue.size();
    stats.remaining_repairs=selection.repair_queue.size();
    stats.tracked_bytes=world.bytes_sent-tracked_bytes_begin;
    stats.tracked_calls=world.collective_calls-tracked_calls_begin;
    stats.tracked_collective_seconds=world.collective_seconds-tracked_seconds_begin;
    const auto saved_directory=retired_directory;
    retire_directory(directory->Statistics());
    if(checkpoint.directory)retire_directory(checkpoint.directory->Statistics());
    const auto all_directory=retired_directory;retired_directory=saved_directory;
    stats.collision_seconds=all_directory.collision_seconds;
    stats.collision_builds=all_directory.collision_builds;
    stats.collision_candidates=all_directory.collision_candidates;
    stats.collision_checks=all_directory.collision_checks;
    stats.directory_elections=all_directory.elections;
    stats.directory_election_seconds=all_directory.election_seconds;
    stats.parameter_agreement_seconds=all_directory.parameter_agreement_seconds;
    stats.parameter_publication_seconds=all_directory.parameter_publication_seconds;
    stats.parameter_edits=all_directory.parameter_edits;stats.parameter_faces=all_directory.parameter_faces;
    stats.parameter_payload_bytes=all_directory.parameter_payload_bytes;
    stats.changed_star_scans=all_directory.change_scanned;
    stats.changed_vertices_sent=all_directory.change_sent;
    stats.changed_vertices_received=all_directory.change_received;
    stats.change_notification_seconds=all_directory.change_seconds;
    stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,all_directory.maximum_work_bytes);
    const auto& donor = source.Statistics();
    stats.donor_candidates = donor.index_candidates; stats.donor_filtered = donor.filtered; stats.donor_box_filtered = donor.box_filtered;
    stats.donor_payloads = donor.payloads; stats.donor_reused = donor.reused_candidates; stats.donor_maximum_patch = donor.maximum_patch;
    stats.donor_search_seconds = donor.search_seconds; stats.donor_filter_seconds = donor.filter_seconds;
    stats.metric_support_cells=donor.metric_sample_cells;stats.metric_support_edges=donor.metric_edge_queries;stats.metric_support_skipped=donor.metric_edge_skipped;stats.metric_support_duplicates=donor.metric_edge_duplicates;
    stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, donor.maximum_work_bytes);
    stats.atlas_work=atlas_budget.work;stats.atlas_peak_bytes=atlas_budget.peak_bytes;
    result.parameters=private_parameters;result.reference=private_geometry;
    if(curved && !result.valid && !selection.cells.empty()) {
      size_t metric_bad=0,geometry_bad=0;
      for(const auto& row:selection.cells){metric_bad+=!row.second.target_valid;geometry_bad+=row.second.geometry_invalid;}
      result.reason+=" Local cached demands: geometry_cells="+std::to_string(geometry_bad)+
          ", metric_or_height_cells="+std::to_string(metric_bad)+", geometry_edges="+std::to_string(selection.geometry_queue.size())+
          ", metric_edges="+std::to_string(selection.queue.size())+".";
    }
    if (!result.valid && !last_rejection.empty()) result.reason += " Last local rejection: " + last_rejection;
    if (options.timings) {
      stats.process_cpu_seconds=double(std::clock()-started_cpu)/CLOCKS_PER_SEC;
      stats.total_seconds=std::chrono::duration<double>(Clock::now()-started).count();
      const double phases=stats.initialization_seconds+stats.seed_seconds+stats.selection_seconds+
          stats.dependency_seconds+stats.reservation_seconds+stats.donor_seconds+stats.reconstruction_seconds+
          stats.metric_seconds+stats.routing_seconds+stats.publication_seconds+stats.update_seconds+stats.completion_seconds+stats.collision_seconds;
      stats.other_seconds=stats.total_seconds-phases-stats.layer_seconds;
    }
    observe_progress("exit");
    stats.tracked_bytes=world.bytes_sent-tracked_bytes_begin;
    stats.tracked_calls=world.collective_calls-tracked_calls_begin;
    stats.tracked_collective_seconds=world.collective_seconds-tracked_seconds_begin;
    if(options.timings) {
      const auto total=std::chrono::duration<double>(Clock::now()-started).count();
      stats.other_seconds+=total-stats.total_seconds;stats.total_seconds=total;
      stats.process_cpu_seconds=double(std::clock()-started_cpu)/CLOCKS_PER_SEC;
    }
    result.rebuilt_domain=result.valid && region_rebuilt;
    result.cells.swap(working);
    return std::move(result);
  };
  if(observe_stage) {
    stage_storage_admitted=admit(0);
    if(!stage_storage_admitted)return finish();
  }
  bool ready=false;
  Id maximum_node=0,maximum_cell=0;
  {
    Timer timer(stats.initialization_seconds,options.timings);
    CLocalFailure failure;
    const auto curved_lo=CPassiveComm::Allreduce(int(curved),CPassiveComm::Op::MIN,world.comm);
    const auto curved_hi=CPassiveComm::Allreduce(int(curved),CPassiveComm::Op::MAX,world.comm);
    const double surface_lo=CPassiveComm::Allreduce(options.surface_tolerance,CPassiveComm::Op::MIN,world.comm);
    const double surface_hi=CPassiveComm::Allreduce(options.surface_tolerance,CPassiveComm::Op::MAX,world.comm);
    if(curved_lo!=curved_hi || surface_lo!=surface_hi || !std::isfinite(surface_lo) ||
       (curved && !(surface_lo>0)))
      failure.Set(1,0,"Native3D analytic runtime requires a consistent positive frozen surface tolerance.");
    const auto composition_lo=CPassiveComm::Allreduce(int(bool(composition)),CPassiveComm::Op::MIN,world.comm);
    const auto composition_hi=CPassiveComm::Allreduce(int(bool(composition)),CPassiveComm::Op::MAX,world.comm);
    const auto bands_lo=CPassiveComm::Allreduce(int(bool(options.edge_breakpoints)),CPassiveComm::Op::MIN,world.comm);
    const auto bands_hi=CPassiveComm::Allreduce(int(bool(options.edge_breakpoints)),CPassiveComm::Op::MAX,world.comm);
    if(composition_lo!=composition_hi || bands_lo!=bands_hi || (composition_hi && !bands_lo))
      failure.Set(1,0,"Native3D frozen composition/band policies differ or composed integration is missing.");
    const auto lo=CPassiveComm::Allreduce(options.maximum_rounds,CPassiveComm::Op::MIN,world.comm);
    const auto hi=CPassiveComm::Allreduce(options.maximum_rounds,CPassiveComm::Op::MAX,world.comm);
    if (lo!=hi) failure.Set(1,0,"Native 3D round limit must be equal across workers.");
    const auto work_lo=CPassiveComm::Allreduce(options.maximum_work,CPassiveComm::Op::MIN,world.comm);
    const auto work_hi=CPassiveComm::Allreduce(options.maximum_work,CPassiveComm::Op::MAX,world.comm);
    const auto window_lo=CPassiveComm::Allreduce(options.stagnation_window,CPassiveComm::Op::MIN,world.comm);
    const auto window_hi=CPassiveComm::Allreduce(options.stagnation_window,CPassiveComm::Op::MAX,world.comm);
    const double time_lo=CPassiveComm::Allreduce(options.maximum_seconds,CPassiveComm::Op::MIN,world.comm);
    const double time_hi=CPassiveComm::Allreduce(options.maximum_seconds,CPassiveComm::Op::MAX,world.comm);
    if (work_lo!=work_hi || window_lo!=window_hi || !window_hi || !std::isfinite(time_lo) ||
        !std::isfinite(time_hi) || time_lo!=time_hi || time_lo<0)
      failure.Set(1,0,"Native 3D work/time/progress controls differ or are invalid.");
    const auto global_cells=CPassiveComm::Allreduce(uint64_t(accepted.size()),CPassiveComm::Op::SUM,world.comm);
    stats.work_limit=options.maximum_work==SIZE_MAX ? std::max<size_t>(4096,transfer_memory::Mul(global_cells,64)) : options.maximum_work;
    if (stats.work_limit==SIZE_MAX) failure.Set(1,0,"Native 3D automatic work ceiling overflows.");
    if(atlas_geometry){atlas_budget.maximum_work=transfer_memory::Mul(transfer_memory::Add(global_cells,stats.work_limit),SU2NativeGeometry::MappedFacetReference::ClearanceWork);
      if(atlas_budget.maximum_work==SIZE_MAX)failure.Set(1,0,"Native3D exact atlas arithmetic work reservation overflows.");}
    const auto batch_lo=CPassiveComm::Allreduce(options.candidate_batch,CPassiveComm::Op::MIN,world.comm);
    const auto batch_hi=CPassiveComm::Allreduce(options.candidate_batch,CPassiveComm::Op::MAX,world.comm);
    if (batch_lo!=batch_hi || !batch_hi || batch_hi>MaximumCandidateBatch)
      failure.Set(1,0,"Native 3D candidate batch must be bounded and equal across workers.");
    const auto mode_lo=CPassiveComm::Allreduce(2*int(intent)+int(bidirectional),CPassiveComm::Op::MIN,world.comm);
    const auto mode_hi=CPassiveComm::Allreduce(2*int(intent)+int(bidirectional),CPassiveComm::Op::MAX,world.comm);
    if (mode_lo!=mode_hi) failure.Set(1,0,"Native 3D adaptation intent differs across workers.");
    if(options.wall_heights.size()>MaximumDirectoryFeatures)
      failure.Set(1,0,"Native3D wall-height policy exceeds bounded marker count.");
    for(const auto& row:options.region_walls) {
      const auto h=options.wall_heights.find(row.first);const auto p=row.second;
      if(h==options.wall_heights.end() || h->second!=p.height || !(p.growth>=1 && p.thickness>=p.height) ||
          !std::isfinite(p.height+p.growth+p.thickness))
        failure.Set(1,0,"Native3D region/height policies are inconsistent or invalid.");
    }
    for(const auto& h:options.wall_heights)if(!(h.second>0) || !std::isfinite(h.second))
      failure.Set(1,0,"Native3D wall-height policy is nonfinite/nonpositive.");
    // One first-height tolerance T (ADAP_BL_FIRST_HEIGHT_TOL) on every worker; no rank-local gate.
    const auto height_tolerance_lo=CPassiveComm::Allreduce(options.wall_heights.tolerance,CPassiveComm::Op::MIN,world.comm);
    const auto height_tolerance_hi=CPassiveComm::Allreduce(options.wall_heights.tolerance,CPassiveComm::Op::MAX,world.comm);
    if(!(height_tolerance_lo>0 && height_tolerance_hi<1) || height_tolerance_lo!=height_tolerance_hi)
      failure.Set(1,0,"Native3D first-height tolerance must lie in (0,1) and agree across workers.");
    ready=vote(failure);
    if(ready)ready=admit(transfer_memory::Add(transfer_memory::Mul(world.size,sizeof(std::vector<HeightPolicyRow>)),
        transfer_memory::Mul(transfer_memory::Mul(world.size,options.wall_heights.size()),2*sizeof(HeightPolicyRow))));
    if(ready) {
      std::vector<std::vector<HeightPolicyRow>> to(world.size);
      for(auto& peer:to)for(const auto& h:options.wall_heights) {
        const auto p=options.region_walls.find(h.first);
        peer.push_back({h.first,h.second,p==options.region_walls.end()?0:p->second.growth,
                        p==options.region_walls.end()?0:p->second.thickness});
      }
      bool admitted=false;
      const auto rows=world.exchange(to,&admitted,options.memory_ceiling,bytes());
      if(!admitted){result.reason="Native3D wall-height policy exchange exceeds budget.";ready=false;}
      else {
        if(rows.size()!=size_t(world.size)*options.wall_heights.size())failure.Set(1,0,"Native3D wall-height policies differ across workers.");
        else for(size_t i=0;i<rows.size();++i) {
          auto h=options.wall_heights.begin();std::advance(h,i%options.wall_heights.size());
          const auto p=options.region_walls.find(h->first);
          if(rows[i].marker!=h->first || rows[i].height!=h->second ||
             rows[i].growth!=(p==options.region_walls.end()?0:p->second.growth) ||
             rows[i].thickness!=(p==options.region_walls.end()?0:p->second.thickness))
            failure.Set(1,0,"Native3D wall-height/region policies differ across workers.");
        }
        ready=vote(failure);
      }
    }
    if (ready) ready=admit(transfer_memory::Add(accepted_bytes,64,transfer_memory::Mul(geometry.ExtensionComponents().size(),sizeof(Id)),
        transfer_memory::Mul(std::min<uint64_t>(stats.work_limit,4096),
            bidirectional ? sizeof(Edge)+sizeof(SplitRecord) : coarsening ? sizeof(Edge) : sizeof(SplitRecord)),transfer_memory::Mul(accepted.size(),selection_bound(repairing))));
    if (ready) try {
      field_identity=std::make_shared<const FrozenMetricIdentity>();
      working=accepted;
      stats.maximum_owned=working.size();
      if (coarsening || bidirectional) result.collapses.reserve(std::min<uint64_t>(stats.work_limit,4096));
      if (!coarsening && !repairing) result.splits.reserve(std::min<uint64_t>(stats.work_limit,4096));
      initial.reserve(working.size());
      eligible_extensions=geometry.ExtensionComponents();
      if(curved)for(const auto& donor:original) {
        maximum_cell=std::max(maximum_cell,donor.cell.id);
        for(auto node:donor.cell.v)maximum_node=std::max(maximum_node,node.id);
      }
      for (const auto& row : working) {
        initial.push_back(row.first); maximum_cell=std::max(maximum_cell,row.first);
        for (auto node : row.second.cell.v) maximum_node=std::max(maximum_node,node.id);
      }
    } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
    if (ready) ready=vote(failure);
    if (ready && !world.maximum(!working.empty())) { failure.Set(1,0,"Native 3D adaptation input is empty."); ready=vote(failure); }
    if (ready) {
      maximum_node=CPassiveComm::Allreduce(maximum_node,CPassiveComm::Op::MAX,world.comm);
      maximum_cell=CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm);
      ready=directory->Build(working,result.reason,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes()));
    }
    if (ready) ready=source.Build(original,result.reason,options.memory_ceiling,
        transfer_memory::Add(bytes(),directory->ResidentBytes()));
  }
  if (!ready) return finish();
  // Freeze donors first; normalization changes working associations, not the source interpretation.
  if(atlas_geometry && !geometry.EntityGeometry()) {
    if(!admit(0))return finish();
    SU2NativeGeometry::BoundaryCoverage refreshed;CLocalFailure failure;
    const auto before_work=atlas_budget.work;
    try {
      if(!SU2NativeGeometry::RebuildWorkingSurface(*geometry.Parameters(),refreshed,atlas_budget,result.reason))
        failure.Set(2,0,result.reason);
    }catch(const std::exception& error){failure.Set(2,0,error.what());}
    if(!vote(failure))return finish();
    if(world.maximum(!refreshed.faces.empty())) {
      std::set<Face> affected;std::vector<Dependency> removed;std::vector<OwnedCell> added;
      const auto face_storage=transfer_memory::Add(SU2Native3D::BoundaryCoverageBytes(refreshed),
          transfer_memory::Mul(refreshed.faces.size(),sizeof(Face)+4*sizeof(void*)));
      if(!admit(face_storage))return finish();
      size_t count=0;
      auto owns=[&](const OwnedCell& row){const auto faces=Faces(row.cell);
        for(unsigned k=0;k<4;++k)if(row.physical[k].present&&affected.count(Key(faces[k])))return true;return false;};
      try {
        for(const auto& row:refreshed.faces)affected.insert(Key(row.first));
        for(const auto& row:working)count+=owns(row.second);
      }catch(const std::exception& error){failure.Set(2,0,error.what());}
      if(!vote(failure))return finish();
      const auto local_storage=transfer_memory::Add(face_storage,transfer_memory::Mul(count,sizeof(Dependency)+sizeof(OwnedCell)));
      if(!admit(local_storage))return finish();
      try {
        removed.reserve(count);added.reserve(count);
        for(const auto& row:working)if(owns(row.second)) {
          if(row.second.version==UINT64_MAX)throw std::overflow_error("Working-surface owner version overflows.");
          removed.push_back({row.first,row.second.version,world.rank});added.push_back(row.second);++added.back().version;
        }
      }catch(const std::exception& error){failure.Set(2,0,error.what());}
      if(!vote(failure))return finish();
      const SU2NativeGeometry::BoundaryCoverage empty;
      if(!directory->Publish(working,removed,added,true,result.reason,options.memory_ceiling,
          transfer_memory::Add(real_bytes(),source.ResidentBytes(),local_storage),{},
          world.rank?empty:refreshed,&atlas_budget))return finish();
      if(options.timings)std::cout<<"NATIVE3D working surface refresh rank="<<world.rank
          <<" faces="<<refreshed.faces.size()<<" owners="<<added.size()<<" arithmetic_work="<<atlas_budget.work-before_work<<'\n';
    }
  }
  // A complete visible radial region is constructed as one transaction, never as a larger local cavity.
  const auto global_original=CPassiveComm::Allreduce(uint64_t(original.size()),CPassiveComm::Op::SUM,world.comm);
  const bool prismatic=!repairing && options.region_walls.size()==options.wall_heights.size() &&
      (!coarsening || options.wall_heights.empty()) && PrismaticWallRegion(geometry,options.region_walls);
  bool region_applicable=curved && !repairing && options.region_walls.size()<=1 &&
      options.region_walls.size()==options.wall_heights.size() &&
      (!coarsening || options.wall_heights.empty());
  if(region_applicable) {
    std::set<Id> walls;
    if(!options.region_walls.empty()) {
      const auto policy=options.region_walls.begin()->second;
      const double g=policy.growth,requested=.9*MaximumMetricEdge;
      const double depth=g==1?policy.height*requested:policy.height*(.5*(g-1)+.5*(g+1)*std::expm1((requested-.5)*2*(g-1)/(g+1)))/(g-1);
      region_applicable=std::isfinite(depth) && depth>policy.height && depth<policy.thickness;
    }
    for(const auto& row:geometry.Facets()) {
      const Id id=geometry.Canonical(row.first);const auto a=geometry.Analytic().find(id);
      if(!geometry.CompleteAnalytic(id) || a==geometry.Analytic().end()) {region_applicable=false;break;}
      const bool radial=a->second.kind==AnalyticSurface::Kind::Cylinder || a->second.kind==AnalyticSurface::Kind::Sphere;
      if(radial) {
        walls.insert(id);
        region_applicable=region_applicable && (options.region_walls.empty() || options.region_walls.count(row.second.marker));
      } else region_applicable=region_applicable && a->second.kind==AnalyticSurface::Kind::Disk && !options.region_walls.count(row.second.marker);
    }
    region_applicable=region_applicable && walls.size()==1;
  }
  region_applicable=region_applicable || prismatic;
  const int region_lo=CPassiveComm::Allreduce(int(region_applicable),CPassiveComm::Op::MIN,world.comm);
  const int region_hi=CPassiveComm::Allreduce(int(region_applicable),CPassiveComm::Op::MAX,world.comm);
  if(region_lo!=region_hi){result.reason="Native3D region applicability differs across workers.";return finish();}
  if(region_applicable) {
    region_active=true;region_started=Clock::now();CLocalFailure failure;
    // Fresh regional identities must also exceed frozen donor identities after earlier coarsening.
    for(const auto& donor:original) {
      maximum_cell=std::max(maximum_cell,donor.cell.id);
      for(auto node:donor.cell.v)maximum_node=std::max(maximum_node,node.id);
    }
    maximum_node=CPassiveComm::Allreduce(maximum_node,CPassiveComm::Op::MAX,world.comm);
    maximum_cell=CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm);
    const size_t global_input=CPassiveComm::Allreduce(working.size(),CPassiveComm::Op::SUM,world.comm);
    // shortcut: a bounded single-worker visible region; partition into connected regions for large sources.
    const size_t region_cells=stats.work_limit;
    const auto scratch=transfer_memory::Add(transfer_memory::Mul(global_input,8192),
        transfer_memory::Mul(region_cells,16384),RegionFieldWorkspaceBytes);
    if(!admit(scratch))return finish();
    std::vector<std::vector<OwnedCell>> buckets(world.size);
    try {buckets[0].reserve(working.size());for(const auto& row:working)buckets[0].push_back(row.second);}
    catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return finish();bool admitted=false;
    auto rows=world.exchange(buckets,&admitted,options.memory_ceiling,transfer_memory::Add(bytes(),scratch));
    if(!admitted){result.reason="Native3D region input exchange exceeds storage.";return finish();}
    std::vector<DonorRegion> regions;Patch old,fresh;std::vector<OwnedCell> replacement;std::vector<BoundaryParameterEdit> region_parameter_edits;bool core_declined=false;
    try {if(!world.rank){
      std::sort(rows.begin(),rows.end(),[](const OwnedCell& a,const OwnedCell& b){return a.cell.id<b.cell.id;});
      old=GeometryPatch(Imported{rows,true,{}}, &geometry);Point lo=rows.front().cell.v[0].p,hi=lo;
      for(const auto& c:old.cells)for(auto n:c.v){lo={std::min(lo.x,n.p.x),std::min(lo.y,n.p.y),std::min(lo.z,n.p.z)};hi={std::max(hi.x,n.p.x),std::max(hi.y,n.p.y),std::max(hi.z,n.p.z)};}
      regions.push_back({lo.x,lo.y,lo.z,hi.x,hi.y,hi.z});
    }}catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return finish();
    DonorPatch imported;imported.valid=true;
    if(global_original<=MaximumDonors)imported=source.Import(regions,options.memory_ceiling,transfer_memory::Add(bytes(),scratch));
    if(!imported.valid){result.reason=imported.reason;return finish();}
    RegionReport report;
    auto tick=[&](){if(options.maximum_seconds && std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)
      throw std::runtime_error("Native3D region construction exhausts elapsed-time budget.");};
    RegionFieldService service(world,source,geometry,composition,options.edge_breakpoints,options.timings,
        options.memory_ceiling,transfer_memory::Add(bytes(),scratch,directory->ResidentBytes()),
        [&](const FrozenField& field){FieldCounts(stats,field);},tick);
    try {service.Run([&](const RegionMetric& target){
      bool already_valid=CertifiedGeometryExcess(old,geometry,options.surface_tolerance)==0;
      bool short_edges=false;
      if(already_valid)for(const auto& c:old.cells) {
        for(unsigned a=0;a<4 && already_valid;++a)for(unsigned b=a+1;b<4;++b) {
          const auto length=target.edge(c.v[a].p,c.v[b].p);
          if(length>MaximumMetricEdge){already_valid=false;break;}
          short_edges=short_edges || (coarsening && length<CoarseningMetricEdge);
        }
        if(!already_valid)break;
      }
      if(!already_valid || short_edges) {
        ++stats.region_attempts;std::map<Id,Point> points;for(const auto& c:old.cells)for(auto n:c.v)points.emplace(n.id,n.p);
        std::vector<Facet> skin;for(auto f:old.surface){Facet face;face.id=f.facet;face.marker=geometry.Get(f.facet).marker;for(unsigned k=0;k<3;++k)face.v[k]={f.v[k],points.at(f.v[k])};skin.push_back(face);}
        std::sort(skin.begin(),skin.end(),[](const Facet& a,const Facet& b){return Key({a.v[0].id,a.v[1].id,a.v[2].id})<Key({b.v[0].id,b.v[1].id,b.v[2].id});});
        // Surface integrals give one partition-independent region kernel.
        const Point origin=old.cells.front().v[0].p;long double volume=0,x=0,y=0,z=0;
        for(const auto& f:skin){const auto a=f.v[0].p,b=f.v[1].p,c=f.v[2].p;const long double v=Orientation(origin,a,b,c);
          volume+=v;x+=v*((long double)origin.x+a.x+b.x+c.x)/4;y+=v*((long double)origin.y+a.y+b.y+c.y)/4;z+=v*((long double)origin.z+a.z+b.z+c.z)/4;}
        if(!(volume>0))throw std::runtime_error("Native3D region has nonpositive closed volume.");const Point kernel{double(x/volume),double(y/volume),double(z/volume)};
        if(maximum_node==UINT64_MAX||maximum_cell==UINT64_MAX)throw std::overflow_error("Native3D region identity reservation overflows.");
        RegionLimits limits;limits.maximum_cells=region_cells;limits.maximum_work=stats.work_limit;limits.timings=options.timings;
        if(options.rejected_trial)limits.rejected=[&](const Patch& candidate,const std::string& why) {
          options.rejected_trial(old,candidate,{},MetricDiagnosis{},false,why);
        };
        limits.checkpoint=[&](){if(options.maximum_seconds && std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)
            throw std::runtime_error("Native3D region construction exhausts elapsed-time budget.");};
        const bool constructed=ConstructWallRegion(skin,kernel,geometry,options.region_walls,options.surface_tolerance,
            options.wall_heights.tolerance,maximum_node+1,maximum_cell+1,target,limits,fresh,report,result.reason);
        if(!constructed) {
          // A height-free cone cannot resolve every interior field; retain the existing volume operators there.
          core_declined=report.unsupported_topology || (options.region_walls.empty() && !report.exhausted_work &&
              (report.unresolved_core || report.maximum_edge>MaximumMetricEdge));
          if(!core_declined)throw std::runtime_error(result.reason);
          last_rejection=result.reason;
        } else if(coarsening && already_valid && fresh.cells.size()>=old.cells.size()) {
          core_declined=true;
          last_rejection="Native3D regional coarsening did not reduce the cell count.";
        } else {
          stats.global_work=fresh.cells.size()+report.surface_trials+report.junction_trials+report.core_trials+report.front_trials;
          if(stats.global_work>stats.work_limit)throw std::runtime_error("Native3D region construction exceeds global event work.");
          if(geometry.Mapped()){geometry.ExportParameters(old);geometry.ExportParameters(fresh);region_parameter_edits=BoundaryParameterState::Changes(old,fresh,geometry.Parameters()->Revision());}
          std::map<Face,Id> physical;for(auto f:fresh.surface)physical.emplace(Key(f.v),f.facet);
          for(const auto& c:fresh.cells){OwnedCell row;row.cell=c;const auto faces=Faces(c);for(unsigned f=0;f<4;++f){auto found=physical.find(Key(faces[f]));if(found!=physical.end())row.physical[f]={true,found->second};}replacement.push_back(row);}
        }
      }
    },imported.cells);}catch(const std::exception& e){failure.Set(2,0,e.what());}
    const auto& field_stats=service.Statistics();
    stats.region_field_points=field_stats.point_calls;stats.region_field_sensor_hits=field_stats.sensor_hits;
    stats.region_field_interior_hits=field_stats.interior_hits;stats.region_field_edges=field_stats.edge_calls;
    stats.region_field_edge_hits=field_stats.edge_hits;stats.region_field_imports=field_stats.imports;
    stats.region_field_maximum_resident=field_stats.maximum_resident;
    stats.field_requests+=field_stats.sensor_hits+field_stats.sampled_points;
    stats.field_hits+=field_stats.sensor_hits;stats.field_misses+=field_stats.sampled_points;
    if(composition)stats.field_compositions+=field_stats.sensor_hits+field_stats.sampled_points;

    stats.region_surface_trials=report.surface_trials;stats.region_junction_trials=report.junction_trials;
    stats.region_core_trials=report.core_trials;stats.region_core_regions=report.core_regions;
    stats.region_transition_fronts=report.transition_fronts;stats.region_front_trials=report.front_trials;
    stats.region_cap_seconds=report.cap_seconds;
    stats.region_sampling_seconds=report.sampling_seconds;stats.region_first_band_seconds=report.first_band_seconds;
    stats.region_transition_seconds=report.transition_seconds;stats.region_junction_seconds=report.junction_seconds;
    stats.region_core_seconds=report.core_seconds;stats.region_validation_seconds=report.validation_seconds;
    stats.global_work=CPassiveComm::Allreduce(uint64_t(report.generated_cells+report.surface_trials+report.junction_trials+report.core_trials+report.front_trials),CPassiveComm::Op::MAX,world.comm);
    if(!vote(failure)) {
      if(world.maximum(report.exhausted_work))stats.stop=EngineStop::WorkBudget;
      else if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds))stats.stop=EngineStop::TimeBudget;
      return finish();
    }
    const bool changed=world.maximum(!replacement.empty());
    if(changed) {
      std::vector<std::vector<OwnedCell>> to(world.size);
      try {for(auto row:replacement)to[row.cell.id%world.size].push_back(row);}catch(const std::exception& e){failure.Set(2,0,e.what());}
      if(!vote(failure))return finish();
      const auto added=world.exchange(to,&admitted,options.memory_ceiling,transfer_memory::Add(bytes(),scratch));
      if(!admitted){result.reason="Native3D region result exchange exceeds storage.";return finish();}
      std::vector<Dependency> removed;try{for(const auto& row:working)removed.push_back({row.first,row.second.version,world.rank});}catch(const std::exception& e){failure.Set(2,0,e.what());}
      if(!vote(failure))return finish();
      if(!directory->Publish(working,removed,added,true,result.reason,options.memory_ceiling,transfer_memory::Add(real_bytes(),scratch),region_parameter_edits,fresh.coverage,geometry.AtlasReservation()))return finish();
      if(!world.rank)++stats.accepted;stats.maximum_owned=std::max(stats.maximum_owned,working.size());stats.region_cells=CPassiveComm::Allreduce(working.size(),CPassiveComm::Op::SUM,world.comm);
    }
    region_rebuilt=changed;stats.global_work=CPassiveComm::Allreduce(stats.global_work,CPassiveComm::Op::MAX,world.comm);
    if(changed)try {
      initial.clear();initial.reserve(working.size());
      for(const auto& row:working){initial.push_back(row.first);maximum_cell=std::max(maximum_cell,row.first);
        for(auto node:row.second.cell.v)maximum_node=std::max(maximum_node,node.id);}
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return finish();
    maximum_node=CPassiveComm::Allreduce(maximum_node,CPassiveComm::Op::MAX,world.comm);
    maximum_cell=CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm);
    // Valid coarsening inputs and finer interior fields retain the existing volume search.
    if(options.timings)stats.reconstruction_seconds+=std::chrono::duration<double>(Clock::now()-region_started).count();
    region_active=false;result.valid=false;result.reason.clear();
  }
  auto composed_sample=[&](Point point,Tensor sensor) {
    Timer query(stats.field_query_seconds,options.timings);++stats.field_requests;++stats.field_misses;
    try {
      if(composition){Timer compose(stats.field_composition_seconds,options.timings);++stats.field_compositions;sensor=composition(point,sensor);}
      ValidateTensor(sensor);return sensor;
    }catch(...){++stats.field_failures;throw;}
  };
  auto import_source=[&](const std::vector<Cell>& cells,size_t extra,bool layer,
                         const std::vector<unsigned char>* requests=nullptr) {
    auto failed=[&](){DonorPatch patch;patch.reason=result.reason;return patch;};
    if(!admit(transfer_memory::Add(extra,transfer_memory::Bytes(cells),transfer_memory::Mul(cells.size(),sizeof(Point)))))return failed();
    if(requests && phase==Phase::Construction) {
      if(layer)++stats.layer_donor_batches;else ++stats.donor_batches;
      return source.ImportMetricSamples(cells,eligible_extensions,options.memory_ceiling,
          transfer_memory::Add(bytes(),directory->ResidentBytes(),extra),*requests,false);
    }
    CLocalFailure failure;std::vector<Point> points;
    try {points.reserve(cells.size());for(const auto& c:cells)points.push_back(Centroid(c));}
    catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return failed();
    auto count_import=[&](){if(layer)++stats.layer_donor_batches;else ++stats.donor_batches;};
    count_import();
    auto patch=source.ProbePoints(points,options.memory_ceiling,
        transfer_memory::Add(bytes(),directory->ResidentBytes(),extra,transfer_memory::Bytes(cells)));
    if(!patch.valid)return patch;
    bool supported=true;
    try {for(size_t i=0;i<cells.size();++i) {
      bool found=false;
      for(const auto& d:patch.cells) {
        if(d.extension_component!=UINT64_MAX)continue;
        const auto& v=d.cell.v;const Tetrahedron parent{v[0].p,v[1].p,v[2].p,v[3].p};
        auto interior=[&](Point p){std::array<long double,4> w;return Barycentric(parent,p,w) &&
            std::all_of(w.begin(),w.end(),[](long double x){return x>0;});};
        if(!interior(points[i]))continue;
        bool contains=true;
        for(auto n:cells[i].v) {
          const bool vertex=std::any_of(v.begin(),v.end(),[&](Node old){return n.id==old.id &&
              n.p.x==old.p.x && n.p.y==old.p.y && n.p.z==old.p.z;});
          if(!vertex && !interior(n.p)){contains=false;break;}
        }
        if(contains){found=true;break;}
      }
      if(!found){supported=false;break;}
    }}catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return failed();
    // Original edges use their exact nodal P1; all other edges and centroids stay strictly inside a parent.
    // First-row cones and original-cell measurement preserve this property.
    if(!world.maximum(!supported))return patch;
    // shortcut: shared-face inserted points retain full imports; extend only with canonical-source equivalence proof.
    patch={};std::vector<Point>().swap(points);count_import();
    if(requests)return source.ImportMetricSamples(cells,eligible_extensions,options.memory_ceiling,
        transfer_memory::Add(bytes(),directory->ResidentBytes(),extra),*requests,true);
    return source.ImportCells(cells,options.memory_ceiling,
        transfer_memory::Add(bytes(),directory->ResidentBytes(),extra));
  };
  if(!options.wall_heights.empty()) {
    layer_active=true;layer_started=Clock::now();
    CLocalFailure failure;
    std::set<Id> pending;
    const auto layerScratch=transfer_memory::Add(transfer_memory::Mul(working.size(),sizeof(Id)+4*sizeof(void*)),
        transfer_memory::Mul(transfer_memory::Mul(options.candidate_batch,MaximumReplacementCells),8192),MaximumFrozenFieldBytes);
    if(!admit(layerScratch))return finish();
    auto needs_layer=[&](const OwnedCell& row) {
      bool wall=false;
      for(const auto& f:row.physical)wall=wall || (f.present && options.wall_heights.count(geometry.Get(f.facet).marker));
      if(!wall)return false;
      std::string reason;
      return !height_check(GeometryPatch(Imported{{row},true,{}}, &geometry),reason);
    };
    try {for(const auto& row:working)if(needs_layer(row.second))pending.insert(row.first);}
    catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return finish();
    uint64_t epoch=0;
    while(world.maximum(!pending.empty())) {
      if(stats.global_work>=stats.work_limit) {stats.stop=EngineStop::WorkBudget;result.reason="Native3D first-row initialization exhausts global work budget.";return finish();}
      auto timed_out=[&]() {
        if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
          stats.stop=EngineStop::TimeBudget;result.reason="Native3D first-row initialization exhausts elapsed-time budget.";return true;
        }
        return false;
      };
      if(timed_out())return finish();
      std::vector<Feature> features;std::vector<Id> queued;
      DependencyBatch dependencies;ImportedBatch imported;ReservationBatch reserved;
      auto plan=[&]() {
        Timer timer(stats.layer_metadata_seconds,options.timings);
        const uint64_t available=std::min(pending.size(),options.candidate_batch);
        const auto position=CPassiveComm::ExscanSum(available,world.comm);
        const auto remaining=stats.work_limit-stats.global_work;
        const size_t count=position<remaining ? std::min(available,remaining-position) : 0;
        stats.global_work+=CPassiveComm::Allreduce(uint64_t(count),CPassiveComm::Op::SUM,world.comm);
        try {
          auto it=pending.begin();
          for(size_t i=0;i<count;++i,++it) {
            const auto& row=working.at(*it);const auto faces=Faces(row.cell);bool found=false;
            for(unsigned f=0;f<4;++f)if(row.physical[f].present && options.wall_heights.count(geometry.Get(row.physical[f].facet).marker)) {
              features.push_back({faces[f],3});queued.push_back(row.cell.id);found=true;break;
            }
            if(!found)throw std::runtime_error("Native3D first-row queue lost its wall face.");
          }
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!vote(failure))return false;
        ++stats.layer_batches;stats.layer_maximum_batch=std::max(stats.layer_maximum_batch,features.size());
        const auto held=transfer_memory::Add(bytes(),source.ResidentBytes(),layerScratch);
        dependencies=directory->LookupBatch(features,options.memory_ceiling,held);
        if(!dependencies.valid){result.reason=dependencies.reason;return false;}
        imported=directory->FetchBatch(dependencies,working,options.memory_ceiling,held);
        if(!imported.valid){result.reason=imported.reason;return false;}
        try {for(size_t i=0;i<imported.candidates.size();++i) {
          const auto& item=imported.candidates[i];
          if(!item.valid || item.cells.size()!=1 || item.cells.front().cell.id!=queued.at(i))
            throw std::runtime_error("Native3D first-row batch lacks its complete queued cell.");
        }}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!vote(failure))return false;
        // These operators preserve every old face/vertex; only the cell interiors are written.
        reserved=directory->ReserveInteriors(imported,options.memory_ceiling,held);
        if(!reserved.valid){result.reason=reserved.reason;return false;}
        if(!world.maximum(std::any_of(reserved.chosen.begin(),reserved.chosen.end(),[](bool x){return x;}))) {
          result.reason="Native3D first-row batch has no reserved cell.";return false;
        }
        return true;
      };
      if(!plan())return finish();
      std::vector<Patch> proposals;
      try {proposals.resize(imported.candidates.size());}catch(const std::exception& e){failure.Set(2,0,e.what());}
      if(!vote(failure))return finish();
      const size_t slots=CPassiveComm::Allreduce(imported.candidates.size(),CPassiveComm::Op::MAX,world.comm);
      auto joint_candidate=[&](const auto& item) {
        unsigned walls=0;
        for(const auto& face:item.cells.front().physical)
          walls+=face.present && options.wall_heights.count(geometry.Get(face.facet).marker);
        return walls>1;
      };
      bool local_joint=false;
      for(size_t k=0;k<imported.candidates.size();++k)if(reserved.chosen[k])local_joint=local_joint || joint_candidate(imported.candidates[k]);
      const bool ranked_background=world.maximum(local_joint);
      size_t span=options.candidate_batch;
      auto construct=[&]() {
        for(size_t at=0;at<slots;) {
          if(timed_out())return false;
          size_t group=std::min(span,slots-at);DonorPatch donors;
          if(ranked_background) {
            Timer timer(stats.layer_donor_seconds,options.timings);
            for(;;) {
              std::vector<Cell> query;
              try {for(size_t k=at;k<std::min(at+group,imported.candidates.size());++k)
                if(reserved.chosen[k] && joint_candidate(imported.candidates[k]))query.push_back(imported.candidates[k].cells.front().cell);
              }catch(const std::exception& e){failure.Set(2,0,e.what());}
              if(!vote(failure))return false;
              donors=import_source(query,layerScratch,true);
              if(donors.valid)break;
              if(!DonorCap(donors.reason) || group==1){result.reason=donors.reason;return false;}
              span=(group+1)/2;group=std::min(span,slots-at);++stats.layer_donor_retries;
            }
          }
          std::unique_ptr<FrozenField> target;std::unique_ptr<FieldAccounting> accounting;
          try {
            if(ranked_background) {
              Timer timer(stats.layer_metric_seconds,options.timings);
              const auto components=AssociatedExtensions(donors.cells,geometry);
              target=std::make_unique<FrozenField>(std::move(donors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
              accounting.reset(new FieldAccounting{stats,*target});
            }
          }catch(const std::exception& e){failure.Set(2,0,e.what());}
          if(!vote(failure))return false;
          bool rejected=false,expired=false;
          auto local_deadline=[&]() {return options.maximum_seconds && std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds;};
          for(size_t k=at;k<at+group;++k) {
            if(local_deadline()){expired=true;break;}
            if(k<imported.candidates.size() && reserved.chosen[k])try {
              const auto old=GeometryPatch(imported.candidates[k], &geometry);auto& fresh=proposals[k];bool approved=false;
              const auto ticket=transfer_memory::Add(transfer_memory::Mul(
                  transfer_memory::Add(transfer_memory::Mul(epoch,world.size),world.rank),options.candidate_batch),k);
              const auto nodeOffset=transfer_memory::Mul(ticket,5),cellOffset=transfer_memory::Mul(ticket,MaximumReplacementCells);
              if(maximum_node>UINT64_MAX-5 || nodeOffset>UINT64_MAX-maximum_node-5 ||
                 maximum_cell>UINT64_MAX-MaximumReplacementCells || cellOffset>UINT64_MAX-maximum_cell-MaximumReplacementCells)
                throw std::invalid_argument("Native3D first-row reserved range overflows.");
              if(old.cells.size()!=1 || old.surface.size()>4)
                throw std::invalid_argument("Native3D first-row identity reservation requires one complete wall cell.");
              const Id node=maximum_node+1+nodeOffset,cell=maximum_cell+1+cellOffset;
              if(!joint_candidate(imported.candidates[k])) {
                FirstHeightReport report;
                {Timer timer(stats.layer_geometry_seconds,options.timings);
                  approved=InsertFirstWallPoint(old,node+1,cell,options.wall_heights,geometry,fresh,report,result.reason);
                  if(approved && !SameSkin(old,fresh))throw std::runtime_error("Native3D interior first-row replacement changed its interface.");
                }
              } else {
                std::vector<Point> backgrounds{Centroid(old.cells.front())};
                std::map<Id,Point> coordinates;for(const auto& c:old.cells)for(auto n:c.v)coordinates.emplace(n.id,n.p);
                for(const auto& face:old.surface) {
                  const auto h=options.wall_heights.find(geometry.Get(face.facet).marker);if(h==options.wall_heights.end())continue;
                  const auto a=coordinates.at(face.v[0]),b=coordinates.at(face.v[1]),c=coordinates.at(face.v[2]);
                  const std::array<long double,3> u{(long double)b.x-a.x,(long double)b.y-a.y,(long double)b.z-a.z},
                      v{(long double)c.x-a.x,(long double)c.y-a.y,(long double)c.z-a.z};
                  auto normal=std::array<long double,3>{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]};
                  const auto length=std::hypot(normal[0],normal[1],normal[2]);for(auto& x:normal)x/=length;
                  for(double factor:{1.25,1.5,2.})backgrounds.push_back({double(((long double)a.x+b.x+c.x)/3-factor*h->second*normal[0]),
                      double(((long double)a.y+b.y+c.y)/3-factor*h->second*normal[1]),double(((long double)a.z+b.z+c.z)/3-factor*h->second*normal[2])});
                }
                long double bestLength=std::numeric_limits<long double>::infinity();std::map<Edge,EdgeMeasure> reuse;
                for(const auto& background:backgrounds) {
                  Patch candidate;FirstHeightReport report;std::string why;bool built=false;
                  {Timer timer(stats.layer_geometry_seconds,options.timings);
                    built=ConstructFirstLayer(old,{node,background},node+1,cell,options.wall_heights,geometry,candidate,report,why);
                    if(built && !SameSkin(old,candidate))throw std::runtime_error("Native3D interior first-row replacement changed its interface.");
                  }
                  if(!built) {
                    if(!approved)result.reason=why;
                    if(why.find("geometry-height incompatibility")!=std::string::npos)break;
                    continue;
                  }
                  MetricStats counted;PatchMetric measured;
                  {Timer timer(stats.layer_metric_seconds,options.timings);
                    measured=MeasureMetricEdges(candidate.cells,*target,&counted,&reuse);
                  }
                  reuse=measured.edges;
                  stats.layer_edge_evaluations+=counted.edge_evaluations;stats.layer_edge_reused+=counted.edge_reused;
                  if(measured.maximum_length<bestLength) {
                    bestLength=measured.maximum_length;fresh=std::move(candidate);approved=true;result.reason.clear();
                  }
                }
              }
              if(!approved){failure.Set(1,old.cells.front().id,"Native3D initial first-row construction rejected: "+result.reason);rejected=true;}
            }catch(const std::exception& e){failure.Set(2,k<queued.size()?queued[k]:0,e.what());rejected=true;}
            if(rejected)break;
            if(local_deadline()){expired=true;break;}
          }
          // Private operators/query fields are local. Vote once per admitted group, while each worker checks its deadline per candidate.
          if(!vote(failure))return false;
          if(world.maximum(expired)) {
            stats.stop=EngineStop::TimeBudget;result.reason="Native3D first-row initialization exhausts elapsed-time budget.";return false;
          }
          at+=group;
        }
        return true;
      };
      if(!construct())return finish();
      auto publish=[&]() {
        Timer timer(stats.layer_publication_seconds,options.timings);
        std::vector<std::vector<Dependency>> toRemove(world.size);
        std::vector<std::vector<OwnedCell>> toAdd(world.size);size_t published=0;
        std::vector<BoundaryParameterEdit> layer_parameter_edits;SU2NativeGeometry::BoundaryCoverage layer_coverage;
        try {for(size_t k=0;k<proposals.size();++k)if(reserved.chosen[k]) {
          const auto& old=imported.candidates[k].cells.front().cell;const auto& fresh=proposals[k];
          const auto& dep=dependencies.candidates[k].cells.front();
          if(dep.cell!=old.id || dependencies.candidates[k].cells.size()!=1 || fresh.cells.empty())
            throw std::runtime_error("Native3D first-row publication lacks its complete replacement.");
          if(geometry.Mapped()) {
            auto before=GeometryPatch(imported.candidates[k],&geometry);auto after=fresh;geometry.ExportParameters(after);
            const auto changes=BoundaryParameterState::Changes(before,after,geometry.Parameters()->Revision());layer_parameter_edits.insert(layer_parameter_edits.end(),changes.begin(),changes.end());
            for(const auto& cover:after.coverage.faces)if(!layer_coverage.faces.emplace(cover).second)throw std::invalid_argument("Repeated physical face in layer atlas publication.");
            for(const auto& trace:after.coverage.traces){const auto previous=layer_coverage.traces.find(trace.first);std::string why;
              if(previous==layer_coverage.traces.end())layer_coverage.traces.emplace(trace);
              else if(!geometry.AtlasReservation()||!geometry.Parameters()->Geometry().EqualTraces(previous->second,trace.second,*geometry.AtlasReservation(),why))throw std::invalid_argument("Conflicting inherited layer atlas edge: "+why);}
          }
          toRemove.at(dep.owner).push_back(dep);
          std::map<Face,Id> physical;for(const auto& f:fresh.surface)physical.emplace(Key(f.v),f.facet);
          for(const auto& c:fresh.cells) {
            OwnedCell row;row.cell=c;const auto faces=Faces(row.cell);
            for(unsigned f=0;f<4;++f){const auto found=physical.find(Key(faces[f]));if(found!=physical.end())row.physical[f]={true,found->second};}
            toAdd.at(dep.owner).push_back(row);
          }
          ++published;
        }}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!vote(failure))return false;
        bool admitted=false;const auto baseline=transfer_memory::Add(bytes(),source.ResidentBytes(),directory->ResidentBytes(),layerScratch);
        const auto removed=world.exchange(toRemove,&admitted,options.memory_ceiling,baseline);
        if(!admitted){result.reason="Native3D first-row removal routing exceeds budget.";return false;}
        const auto added=world.exchange(toAdd,&admitted,options.memory_ceiling,baseline);
        if(!admitted){result.reason="Native3D first-row addition routing exceeds budget.";return false;}
        if(!directory->Publish(working,removed,added,true,result.reason,options.memory_ceiling,transfer_memory::Add(real_bytes(),source.ResidentBytes(),layerScratch),layer_parameter_edits,layer_coverage,geometry.AtlasReservation()))return false;
        try {
          for(const auto& row:removed)pending.erase(row.cell);
          for(const auto& row:added)if(needs_layer(row))pending.insert(row.cell.id);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!vote(failure))return false;
        stats.layer_published+=published;++epoch;stats.maximum_owned=std::max(stats.maximum_owned,working.size());
        return true;
      };
      if(!publish())return finish();
    }
    try {
      initial.clear();initial.reserve(working.size());
      for(const auto& row:working){initial.push_back(row.first);maximum_cell=std::max(maximum_cell,row.first);for(auto n:row.second.cell.v)maximum_node=std::max(maximum_node,n.id);}
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return finish();
    maximum_node=CPassiveComm::Allreduce(maximum_node,CPassiveComm::Op::MAX,world.comm);
    maximum_cell=CPassiveComm::Allreduce(maximum_cell,CPassiveComm::Op::MAX,world.comm);
    if(options.timings)stats.layer_seconds+=std::chrono::duration<double>(Clock::now()-layer_started).count();
    layer_active=false;
  }
  constexpr size_t MeasurementBatch=16;
  // Construction requests missing edges only; improvement also imports centroid support.
  auto measure=[&](const std::vector<Id>& ids,bool seed,double* enclosing_seconds) {
    const auto plan_scratch=transfer_memory::Mul(3*MaximumCampaignEdges,sizeof(std::pair<const Edge,EdgeMeasure>)+6*sizeof(void*));
    if(!admit(transfer_memory::Add(plan_scratch,transfer_memory::Mul(MeasurementBatch,sizeof(Cell)+sizeof(unsigned char)))))return false;
    selection.census_started=true;
    const size_t count=CPassiveComm::Allreduce(ids.size(),CPassiveComm::Op::MAX,world.comm);
    for (size_t slot=0; slot<count;) {
      if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
        stats.stop=EngineStop::TimeBudget;result.reason=seed ? "Native3D metric seeding exhausts elapsed-time budget." :
            "Native3D metric update exhausts elapsed-time budget.";return false;
      }
      size_t batch=std::min({MeasurementBatch,MaximumCavityCells/6,count-slot});
      DonorPatch imported;std::map<Edge,EdgeMeasure> reuse;
      CLocalFailure failure;
      for (;;) {
        std::vector<Cell> regions;
        try {
          reuse.clear();
          for (size_t i=slot;i<std::min(slot+batch,ids.size());++i) {
            const auto& cell=working.at(ids[i]).cell;regions.push_back(cell);selection.Cached(cell,reuse);
            for(auto a:cell.v)for(auto b:cell.v)if(a.id<b.id) {
              const auto prior=retired_edges.find({a.id,b.id});if(prior!=retired_edges.end())reuse.emplace(prior->first,prior->second);
            }
          }
        } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        if (!vote(failure)) return false;
        std::vector<unsigned char> requests;
        try {requests=MetricEdgeRequests(regions,reuse,field_identity,&campaign_edges);}
        catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!vote(failure))return false;
        {Timer timer(stats.donor_seconds,options.timings,enclosing_seconds);
          imported=import_source(regions,transfer_memory::Add(MaximumFrozenFieldBytes,plan_scratch),false,&requests);}

        if (imported.valid) break;
        if (batch==1) { result.reason=imported.reason; return false; }
        batch=(batch+1)/2; ++stats.batch_retries;
      }
      if (!admit(transfer_memory::Add(transfer_memory::Bytes(imported.cells),
          MaximumFrozenFieldBytes,plan_scratch,transfer_memory::Mul(working.size(),selection_bound(selection.track_repairs))))) return false;
      if (slot<ids.size()) try {
        CpuTimer private_cpu(stats.measurement_cpu_seconds,options.timings);
        const auto components=AssociatedExtensions(imported.cells,geometry);
        FrozenField field(std::move(imported.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
        FieldAccounting accounting{stats,field};
        for (size_t i=slot;i<std::min(slot+batch,ids.size());++i) {
          PatchMetric metric;
          std::string reason;
          MetricStats counted;
          const auto& cell=working.at(ids[i]).cell;
          bool strict=false,quality_valid=false,height_valid=true;FirstHeightReport height;
          if(phase==Phase::Construction) {
            metric=MeasureMetricEdges({cell},field,&counted,&reuse,&campaign_edges);
            strict=metric.maximum_length<=MaximumMetricEdge;
          } else {
            const bool metric_valid=ValidateMetricPatch({cell},field,metric,reason,&counted,&reuse);
            std::string height_reason;
            height_valid=height_check(GeometryPatch(Imported{{working.at(ids[i])},true,{}}, &geometry),height_reason,false,&height);
            if(!height_valid)reason=height_reason;
            quality_valid=metric.shape_measured && std::isfinite(metric.minimum_mean_ratio) && std::isfinite(metric.minimum_scaled_jacobian) &&
                metric.minimum_mean_ratio>=MinimumMeanRatio &&
                metric.minimum_scaled_jacobian>=MinimumScaledJacobian && height_valid;
            strict=metric_valid && height_valid;
          }
          if(phase==Phase::Improvement && selection.cells.count(ids[i]))
            selection.RefreshQuality(working.at(ids[i]),metric,strict,quality_valid,!options.wall_heights.empty()?&height:nullptr,height_valid,field_identity);
          else selection.Add(working.at(ids[i]),metric,strict,quality_valid,reason,
              phase==Phase::Improvement && !options.wall_heights.empty()?&height:nullptr,height_valid);
          for(const auto& edge:metric.edges)reuse.insert_or_assign(edge.first,edge.second);
          stats.edge_evaluations+=counted.edge_evaluations;
          stats.edge_reused+=counted.edge_reused;
          if (seed) ++stats.initial_cells; else ++stats.updated_cells;
        }
      } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      if (!vote(failure)) return false;
      slot+=batch;if(seed)++stats.seed_batches;
    }
    return true;
  };
  size_t seed_slot=0;
  auto seed_next=[&]() {
    Timer timer(stats.seed_seconds,options.timings);
    // Composed edges are expensive; never measure pending topology merely to let a complete cavity remove it.
    if(!composition || phase==Phase::Improvement) {const bool measured=measure(initial,true,&stats.seed_seconds);if(measured)seed_slot=initial.size();return measured;}
    if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
      stats.stop=EngineStop::TimeBudget;result.reason="Native3D metric seeding exhausts elapsed-time budget.";return false;
    }
    if(!admit(transfer_memory::Mul(MeasurementBatch,sizeof(Id))))return false;
    CLocalFailure failure;std::vector<Id> next;
    try {
      size_t scanned=0;next.reserve(MeasurementBatch);
      while(seed_slot<initial.size() && next.size()<MeasurementBatch && scanned++<MaximumDirectoryScan) {
        const auto id=initial[seed_slot++];
        if(working.count(id) && !selection.cells.count(id))next.push_back(id);
      }
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    return vote(failure) && measure(next,true,&stats.seed_seconds);
  };
  auto length_direction=[&](bool collapse) {
    if(coarsening!=collapse) {
      selection.blocked_edges.swap(inactive_blocked_edges);std::swap(selection.blocked_links,inactive_blocked_links);
      coarsening=selection.coarsening=collapse;
      construction_kind=collapse?MetricChange::Coarsen:MetricChange::Refine;
    }
    if(length_blocks_dirty[collapse]) {
      selection.blocked_edges.clear();selection.blocked_links=0;length_blocks_dirty[collapse]=false;
    }
  };
  auto enter_phase=[&](Phase next) {
    if(bidirectional) {
      if(next==Phase::Construction) {
        if(world.maximum(improvement_changed))length_blocks_dirty={{true,true}};
        length_pair_changed=false;directory->BeginConstructionPass();
      } else directory->EndConstructionPass();
      length_direction(false);
    }
    if(phase==Phase::Construction && !construction_entry_reported) {
      if(!observe_progress("construction_entry"))return false;
      construction_entry_reported=true;
    }
    construction_handoff_progress=next==Phase::Improvement && world.maximum(construction_changed);
    if(next==Phase::Construction){construction_changed=false;construction_generation_changed=false;}
    phase=next;repairing=next==Phase::Improvement;improvement_changed=false;phase_start_round=stats.rounds;
    progress_at_window=stats.size_progress+stats.quality_progress+stats.cell_removals+stats.geometry_progress;
    attempted_at_window=stats.attempted;deferred_at_window=stats.length_pass_deferred;
    if(repairing)++stats.quality_stages;else ++stats.construction_returns;
    if(!admit(transfer_memory::Add(transfer_memory::Mul(working.size(),sizeof(Id)),
        transfer_memory::Mul(working.size(),selection_bound(repairing)))))return false;
    CLocalFailure failure;
    try {
      selection.ChangePhase(repairing,working,field_identity);
      initial.clear();initial.reserve(working.size());
      for(const auto& row:working) {const auto found=selection.cells.find(row.first);
        if(found==selection.cells.end() || (repairing && (!found->second.shape_measured ||
            (!options.wall_heights.empty() && !found->second.debt.height_measured))))initial.push_back(row.first);}
      seed_slot=0;
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return false;
    // A phase census has no enclosing update timer to debit for donor imports.
    const bool measured=measure(initial,false,nullptr);retired_edges.clear();
    if(!measured)return false;seed_slot=initial.size();
    if(!repairing)construction_entry_reported=true;
    return observe_progress(repairing?"quality_entry":"construction_entry");
  };
  auto discard_checkpoint=[&]() {
    if(checkpoint.directory)retire_directory(checkpoint.directory->Statistics());
    checkpoint.directory.reset();checkpoint.parameters.reset();
    Owned().swap(checkpoint.cells);decltype(checkpoint.blocked)().swap(checkpoint.blocked);
    std::set<Edge>().swap(checkpoint.edges);std::set<Edge>().swap(checkpoint.attempted);
    std::set<Id>().swap(checkpoint.changed);checkpoint.blocked_links=0;
    checkpoint.valid=checkpoint.trial=false;
  };
  auto capture_checkpoint=[&]() {
    // This call follows the complete fresh quality census, including measured strict height.
    // No mutation intervenes; no initial/construction/partial state is presumed certified.
    CLocalFailure failure;
    const auto extra=transfer_memory::Add(MapBytes(working),directory->ResidentBytes(),
        private_parameters ? private_parameters->ResidentBytes() : 0,MapBytes(selection.blocked_edges),
        transfer_memory::Mul(selection.blocked_links,sizeof(Id)+4*sizeof(void*)),
        transfer_memory::Mul(transfer_memory::Mul(6,working.size()),sizeof(Edge)+4*sizeof(void*)));
    if(!admit(extra))return false;
    try {
      checkpoint.cells=working;checkpoint.blocked=selection.blocked_edges;checkpoint.blocked_links=selection.blocked_links;
      for(const auto& row:working)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j) {
        Edge edge{row.second.cell.v[i].id,row.second.cell.v[j].id};std::sort(edge.begin(),edge.end());checkpoint.edges.insert(edge);
      }
      if(private_parameters)checkpoint.parameters=std::make_unique<BoundaryParameterState>(*private_parameters);
      checkpoint.directory=std::make_unique<Directory>(world,true,geometry.Parameters());
    }catch(const std::exception& e){failure.Set(3,0,e.what());}
    if(!vote(failure))return false;
    // Prebuild while both directories see the checkpoint parameter contents. This dormant
    // directory is never queried during the speculative trial; rollback needs no allocation.
    if(!checkpoint.directory->Build(checkpoint.cells,result.reason,options.memory_ceiling,
        transfer_memory::Add(bytes(),source.ResidentBytes(),directory->ResidentBytes())))return false;
    checkpoint.global_cells=CPassiveComm::Allreduce(working.size(),CPassiveComm::Op::SUM,world.comm);
    checkpoint.collapses=result.collapses.size();checkpoint.splits=result.splits.size();
    checkpoint.valid=true;checkpoint.trial=false;
    ++stats.coarsen_checkpoints;return true;
  };
  auto rollback_checkpoint=[&]() {
    static_assert(std::is_nothrow_swappable<BoundaryParameterState>::value,
        "Private checkpoint parameter contents must restore without allocation.");
    CLocalFailure failure;size_t retired=0;
    if(!checkpoint.valid || !checkpoint.directory || checkpoint.collapses>result.collapses.size() || checkpoint.splits>result.splits.size())
      failure.Set(1,0,"Native coarsening rollback lacks its complete checkpoint.");
    if(bool(private_parameters)!=bool(checkpoint.parameters) || (private_parameters &&
        private_parameters->Authority().get()!=checkpoint.parameters->Authority().get()))
      failure.Set(1,0,"Native coarsening rollback changes immutable geometry authority.");
    if(!vote(failure))return false;
    if(!admit(transfer_memory::Mul(checkpoint.attempted.size(),2*(sizeof(Edge)+8*sizeof(void*)))))return false;
    try {
      for(const auto& edge:checkpoint.attempted) {
        auto key=edge;std::sort(key.begin(),key.end());
        if(!checkpoint.edges.count(key))continue; // Descendants are absent after rollback.
        const bool inserted=checkpoint.blocked[edge[1]].insert(edge[0]).second;
        checkpoint.blocked_links+=inserted;retired+=inserted;
      }
    }catch(const std::exception& e){failure.Set(3,0,e.what());}
    if(!vote(failure))return false;
    if(!world.maximum(retired!=0)) {
      result.reason="Native coarsening trial made no net reduction and retired no new checkpoint obligation.";
      return false;
    }
    // All proposal/import/private-parameter scopes have ended at this phase boundary. Restore
    // local ownership and exact identities together, leaving IDs/work/deadline/service epochs monotone.
    if(private_parameters)std::swap(*private_parameters,*checkpoint.parameters);
    working.swap(checkpoint.cells);directory.swap(checkpoint.directory);
    selection.blocked_edges.swap(checkpoint.blocked);std::swap(selection.blocked_links,checkpoint.blocked_links);
    stats.coarsen_rolled_back_collapses+=result.collapses.size()-checkpoint.collapses;
    result.collapses.resize(checkpoint.collapses);result.splits.resize(checkpoint.splits);
    ++stats.coarsen_rolled_back_passes;stats.coarsen_retired_directions+=retired;
    stats.coarsen_restored_cells+=working.size();
    discard_checkpoint();
    selection.Reset(true);retired_edges.clear();campaign_edges.clear();
    construction_changed=construction_handoff_progress=improvement_changed=false;
    // A strict checkpoint is not permission to return a stale census or to forgive a budget exit.
    return enter_phase(Phase::Improvement);
  };
  if(bidirectional)directory->BeginConstructionPass();
  if(!admit(transfer_memory::Mul(transfer_memory::Mul(coarsening ? 12 : 6,working.size()),sizeof(Edge)+4*sizeof(void*))))return finish();
  {
    CLocalFailure failure;
    try{selection.SeedFrontier(working);}catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return finish();
  }
  ready=seed_next();
  if (!ready) return finish();
  for (size_t round=0; !options.maximum_rounds || round<options.maximum_rounds;) {
    Choice choice;
    bool complete=false,any=false,bad_field=false,refill=false;
    auto pick=[&]() {
      if (!coarsening) { choice=selection.Best(repairing); any=world.maximum(choice.active); return true; }
      CLocalFailure failure;
      try {
        choice=selection.Best(repairing);
        while (!repairing && (!bad_field||bidirectional) && selection.FilterCollapse(choice,working)) choice=selection.Best(false);
      } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      if (!(ready=vote(failure))) return false;
      any=world.maximum(choice.active); return true;
    };
    {
      Timer timer(stats.selection_seconds,options.timings);
      ++stats.selections;
      int dirty=world.maximum(selection.invalid ? 2 :
          (selection.cells.size()!=working.size() || (coarsening && !selection.queue.empty()) ? 1 : 0));
      bad_field=dirty==2;
      if (coarsening && !repairing && (!bad_field||bidirectional) && !selection.prune_collapses) {
        // Joint construction filters necessary validity constraints even while target debt remains.
        CLocalFailure failure;
        try { selection.EnableCollapseFilter(); } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        ready=vote(failure);
        if (ready) dirty=world.maximum(selection.cells.size()!=working.size() || !selection.queue.empty());
      }
      if (ready) {
        complete=dirty==0;
        pick();
        if (ready && coarsening && !repairing && !bad_field && !any)
          complete=!world.maximum(selection.cells.size()!=working.size() || !selection.queue.empty());
        if(ready && !complete && !any)refill=world.maximum(seed_slot<initial.size());
      }
    }
    if (!ready) return finish();
    if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
      stats.stop=EngineStop::TimeBudget;result.reason="Native 3D kernel elapsed-time budget exhausted; feasibility is undetermined.";
      return finish();
    }
    // Before its first strict checkpoint, coarsening must finish the edge-only entry census.
    // A size/geometry-valid input can then establish its checkpoint in the quality phase before
    // destructive removals; invalid inputs still use construction until a strict closure exists.
    if(standalone_coarsening && !checkpoint.valid && phase==Phase::Construction &&
        world.maximum(selection.cells.size()!=working.size()) && world.maximum(seed_slot<initial.size()))refill=true;
    if(refill) {ready=seed_next();if(!ready)return finish();continue;}
    if(standalone_coarsening && !checkpoint.valid && phase==Phase::Construction &&
        !world.maximum(selection.cells.size()!=working.size() || selection.construction_invalid!=0)) {
      if(!enter_phase(Phase::Improvement))return finish();
      continue;
    }
    // Amortize the strict census over a useful repair pass before retrying outstanding construction.
    const bool clean_quality=phase==Phase::Improvement &&
        !world.maximum(selection.quality_invalid!=0 || selection.unmeasured_shape!=0 || selection.cells.size()!=working.size());
    if(standalone_coarsening && phase==Phase::Improvement && checkpoint.valid && !any && !clean_quality &&
        !world.maximum(selection.cells.size()!=working.size()) && world.maximum(checkpoint.trial)) {
      if(stats.global_work>=stats.work_limit) {
        stats.stop=EngineStop::WorkBudget;result.reason="Native coarsening quality closure exhausted its work budget.";return finish();
      }
      if(!rollback_checkpoint())return finish();
      continue;
    }
    if(standalone_coarsening && clean_quality && !world.maximum(selection.construction_invalid!=0)) {
      if(checkpoint.valid && world.maximum(checkpoint.trial)) {
        const size_t cells=CPassiveComm::Allreduce(working.size(),CPassiveComm::Op::SUM,world.comm);
        if(cells>=checkpoint.global_cells) {
          if(!rollback_checkpoint())return finish();
          continue;
        }
        ++stats.coarsen_committed_passes;
        discard_checkpoint();
      }
      if(!world.maximum(selection.HasCoarseningDemand())) {
        result.valid=true;result.reason.clear();return finish();
      }
      if(!checkpoint.valid) {
        if(!capture_checkpoint() || !enter_phase(Phase::Construction))return finish();
        continue;
      }
    }
    if(phase==Phase::Improvement && (world.maximum(improvement_changed) || (construction_handoff_progress && clean_quality)) &&
        (standalone_coarsening || world.maximum(selection.construction_invalid!=0)) &&
        (!world.maximum(selection.quality_invalid!=0) || !any)) {
      if(!enter_phase(Phase::Construction))return finish();
      continue;
    }
    if(bidirectional && phase==Phase::Construction && !any &&
        !world.maximum(!selection.Progress(working.size()).census_complete)) {
      const bool productive=world.maximum(construction_generation_changed);
      length_pair_changed=length_pair_changed||productive;
      if(productive)length_blocks_dirty={{true,true}};
      if(coarsening)++stats.length_collapse_passes;else ++stats.length_split_passes;
      if(options.timings) {
        const auto progress=selection.Progress(working.size());size_t short_edges=0,long_edges=0;
        long double minimum=INFINITY,maximum=0;
        for(const auto& edge:selection.edges){const auto length=edge.second.measure.length;
          short_edges+=length<CoarseningMetricEdge;long_edges+=length>MaximumMetricEdge;
          minimum=std::min(minimum,length);maximum=std::max(maximum,length);}
        std::cout<<std::setprecision(17)<<"NATIVE3D_LENGTH rank="<<world.rank<<" pass="<<(coarsening?"collapse":"split")
          <<" cells="<<working.size()<<" local_edges="<<selection.edges.size()<<" short_local_edges="<<short_edges
          <<" long_local_edges="<<long_edges<<" minimum_length="<<(selection.edges.empty()?0:minimum)
          <<" maximum_length="<<maximum<<" size_hinge_incidence="<<progress.size_hinge_squared
          <<" geometry_bad_cells="<<progress.geometry_bad_cells<<" size_bad_cells="<<progress.size_bad_cells
          <<" q_measured_cells="<<progress.q_measured_cells<<" j_measured_cells="<<progress.j_measured_cells
          <<" split_accepted="<<stats.length_splits<<" collapse_accepted="<<stats.length_collapses
          <<" atlas_work="<<atlas_budget.work<<" admitted_peak_bytes="<<stats.maximum_work_bytes
          <<" elapsed="<<std::chrono::duration<double>(Clock::now()-started).count()<<'\n';
      }
      if(!observe_progress(coarsening?"length_collapse_exit":"length_split_exit"))return finish();
      if(coarsening && !length_pair_changed) {
        if(!enter_phase(Phase::Improvement))return finish();
      } else {
        // Exhaust productive generations before switching operators; shared apexes are deferred within each generation.
        const bool next_collapse=productive?coarsening:!coarsening;
        if(!next_collapse)length_pair_changed=false;
        if(!admit(transfer_memory::Add(transfer_memory::Mul(working.size(),selection_bound(false)),
            transfer_memory::Mul(transfer_memory::Mul(next_collapse?12:6,working.size()),sizeof(Edge)+4*sizeof(void*)))))return finish();
        CLocalFailure failure;
        try {
          directory->BeginConstructionPass();length_direction(next_collapse);selection.ChangePhase(false,working,field_identity);
        }catch(const std::exception& error){failure.Set(2,0,error.what());}
        if(!vote(failure))return finish();
        construction_generation_changed=false;phase_start_round=stats.rounds;
        progress_at_window=stats.size_progress+stats.quality_progress+stats.cell_removals+stats.geometry_progress;
        attempted_at_window=stats.attempted;deferred_at_window=stats.length_pass_deferred;
      }
      continue;
    }
    if(!bidirectional && !coarsening && phase==Phase::Construction && !any) {
      // New edges belong to the next finite generation; productive construction need not remeasure quality first.
      const bool census=world.maximum(!selection.Progress(working.size()).census_complete)==0;
      const bool demanded=world.maximum(selection.construction_invalid!=0);
      const bool productive=world.maximum(construction_generation_changed);
      if(census && demanded && productive) {
        if(stats.global_work>=stats.work_limit){stats.stop=EngineStop::WorkBudget;
          result.reason="Native3D construction generation exhausted global work; feasibility is undetermined.";return finish();}
        if(!admit(transfer_memory::Mul(working.size(),selection_bound(false))))return finish();
        CLocalFailure failure;
        try{selection.ChangePhase(false,working,field_identity);}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!vote(failure))return finish();
        construction_generation_changed=false;
        continue;
      }
    }
    if(!bidirectional && phase==Phase::Construction && (complete || (!any && !world.maximum(selection.cells.size()!=working.size())))) {
      if(!enter_phase(Phase::Improvement))return finish();
      continue;
    }
    {
      Timer timer(stats.selection_seconds,options.timings);
      if (options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
        stats.stop=EngineStop::TimeBudget; result.reason="Native 3D kernel elapsed-time budget exhausted; feasibility is undetermined.";
        return finish();
      }
    }
    if (complete && phase==Phase::Improvement && !world.maximum(selection.unmeasured_shape!=0 || selection.quality_invalid!=0 || selection.construction_invalid!=0)) {
      result.valid=true;result.reason.clear();return finish();
    }
    {
      Timer timer(stats.selection_seconds,options.timings);
      if (stats.rounds>phase_start_round && round && round%options.stagnation_window==0) {
        ++stats.progress_windows;
        const uint64_t progress=stats.size_progress+stats.quality_progress+stats.cell_removals+stats.geometry_progress;
        const bool mesh_progress=world.maximum(progress!=progress_at_window);
        const bool search_progress=world.maximum(stats.attempted!=attempted_at_window ||
            (bidirectional && stats.length_pass_deferred!=deferred_at_window));
        progress_at_window=progress; attempted_at_window=stats.attempted;deferred_at_window=stats.length_pass_deferred;
        // Valid coarsening may exhaust rejected directions without changing an already admissible mesh.
        if (!mesh_progress) ++stats.stagnant_windows;
        if (!mesh_progress && search_progress) ++stats.search_windows;
        if(!observe_progress("window"))return finish();
        // Fresh rejected directions are finite search work even on invalid meshes; do not declare infeasibility early.
        if (!mesh_progress && !search_progress) {
          if(!bidirectional && phase==Phase::Construction && !world.maximum(selection.cells.size()!=working.size())) {
            if(!enter_phase(Phase::Improvement))return finish();
            continue;
          }
          stats.stop=EngineStop::OperatorStagnation;
          result.reason="Native 3D admitted operators made no certified progress over the observation window; feasibility is undetermined.";
          return finish();
        }
      }
    }
    if (!any) {
      stats.stop=EngineStop::OperatorStagnation;
      CLocalFailure failure;
      if (selection.invalid || selection.cells.size()!=working.size()) failure.Set(1,0,"Native 3D adaptation stagnates: admitted operators cannot satisfy the frozen size/shape gates.");
      vote(failure); return finish();
    }
    {
      Timer timer(stats.selection_seconds,options.timings);
      const size_t used=coarsening ? result.collapses.size() : result.splits.size();
      const size_t capacity=coarsening ? result.collapses.capacity() : result.splits.capacity();
      const size_t needed=transfer_memory::Add(used,options.candidate_batch);
      const size_t next=std::min<uint64_t>(stats.work_limit,std::max(needed,transfer_memory::Mul(capacity,2)));
      CLocalFailure failure;
      ready=admit(!repairing && next>capacity ? transfer_memory::Mul(next,coarsening ? sizeof(Edge) : sizeof(SplitRecord)) : 0);
      if (ready && !repairing && next>capacity) {
        if (ready) try {
          if (coarsening) result.collapses.reserve(next); else result.splits.reserve(next);
        } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      }
      // Every worker votes even if its history already has room.
      if (ready) ready=vote(failure);
      if (!ready) return finish();
    }
    {
      CLocalFailure failure;
      if(selection.repair_epoch==UINT64_MAX)failure.Set(1,0,"Native repair service epoch overflows.");
      if(!vote(failure))return finish();
      ++selection.repair_epoch;
    }
    ++stats.rounds;
    const bool concurrent=(repairing || ((composition || curved) && !coarsening) || (bidirectional && coarsening)) && options.candidate_batch>1 && world.size>1;
    stats.concurrent_rounds+=concurrent;
    const bool batched=!curved && !composition && !repairing && (!coarsening || !bad_field) && options.candidate_batch>1;
    std::vector<Choice> choices;
    DependencyBatch dependency_batch;
    ImportedBatch imported_batch;
    ReservationBatch reserved;
    size_t slots=1;
    const bool planned_coarsening=coarsening && !repairing && !curved && !composition && world.size>1;
    // A complete star above the cavity bound blocks only its own proposal, as on every lookup path; changed
    // stars reactivate it. It is never published and never fails the event.
    size_t refused_candidates=0,refused_slots=0,pass_deferred_candidates=0;
    std::array<RepairReceipt,MaximumCandidateBatch> refused_receipts{};size_t refused_receipt_count=0;
    auto refuse=[&](const Choice& candidate,const Feature& feature,const std::string& why) {
      selection.Block(candidate.edge,candidate.feature,repairing);++stats.attempted;++stats.rejected;++stats.oversized_stars;
      if(!repairing && (batched || planned_coarsening)) {
        if(refused_receipt_count==MaximumCandidateBatch)throw std::logic_error("Native refused frontier exceeds batch bound.");
        refused_receipts[refused_receipt_count++]={Feature{{candidate.edge[0],candidate.edge[1],0},2},false};
      }
      stats.last_rejected_feature=feature;++refused_candidates;
      last_rejection="feature "+std::to_string(feature.count)+":"+std::to_string(feature.vertices[0])+": "+why;
    };
    // Every nonempty selected round admits work; at most batch*workers metadata frontiers per admitted round.
    auto plan_choices=[&]() {
      CLocalFailure failure;
      stats.batch_considered+=choices.size();
      if(stats.global_work==stats.work_limit) {
        stats.stop=EngineStop::WorkBudget;result.reason="Native 3D global proposal budget exhausted before admission; feasibility is undetermined.";
        return false;
      }
      if(planned_coarsening) {
        Timer timer(stats.planning_seconds,options.timings);
        std::vector<Feature> features;std::vector<size_t> selected;
        try {for(const auto& candidate:choices)features.push_back({{candidate.edge[1],0,0},1});}
        catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return false;
        dependency_batch=directory->PlanBatch(features,selected,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
        if(!dependency_batch.valid){result.reason=dependency_batch.reason;return false;}
        try {
          for(size_t i:dependency_batch.refused)refuse(choices.at(i),features.at(i),"Native 3D dependency star exceeds budget.");
          for(size_t i:dependency_batch.pass_deferred) {
            if(refused_receipt_count==MaximumCandidateBatch)throw std::logic_error("Native construction deferrals exceed frontier bound.");
            const auto edge=choices.at(i).edge;
            refused_receipts[refused_receipt_count++]={Feature{{edge[0],edge[1],0},2},false};
            ++pass_deferred_candidates;++stats.length_pass_deferred;
          }
          std::vector<Choice> planned;planned.reserve(selected.size());
          for(size_t i:selected)planned.push_back(choices.at(i));
          stats.batch_deferred+=choices.size()-planned.size()-dependency_batch.refused.size();choices.swap(planned);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return false;
        if(world.maximum(refused_receipt_count!=0)) {
          if(!service_frontier({},false,false,&refused_receipts,refused_receipt_count))return false;
          refused_receipt_count=0;
        }
        if(!world.maximum(!choices.empty() || refused_candidates!=0 || pass_deferred_candidates!=0)){result.reason="Native 3D complete-star selection made no progress.";return false;}
      }
      return true;
    };
    if (batched) {
      // All retained metadata/payloads remain byte-admitted while each disjoint candidate uses the existing transaction.
      batch_bytes=transfer_memory::Add(transfer_memory::Mul(options.candidate_batch,
          sizeof(Choice)+sizeof(Feature)+sizeof(Dependencies)+sizeof(Imported)+2*sizeof(bool)+2*sizeof(size_t)),
          transfer_memory::Mul(transfer_memory::Mul(options.candidate_batch,MaximumCavityCells),
              sizeof(Dependency)+sizeof(OwnedCell)),
          transfer_memory::Mul(transfer_memory::Mul(8,options.candidate_batch*MaximumCavityCells),
              sizeof(Id)+4*sizeof(void*)));
      if (!admit(0)) return finish();
      CLocalFailure failure;
      {
        Timer timer(stats.selection_seconds,options.timings);
        try { choices=selection.Batch(working,options.candidate_batch); }
        catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        if (!(ready=vote(failure))) return finish();
        if(!coalesce(choices))return finish();
        if(!plan_choices())return finish();
        slots=CPassiveComm::Allreduce(choices.size(),CPassiveComm::Op::MAX,world.comm);
        ++stats.batch_rounds; stats.batch_candidates+=choices.size(); stats.proposed+=choices.size()+refused_candidates;
      }
      {
        Timer timer(stats.selection_seconds,options.timings);
        // Planned refusals are proposals too: they consume the same global work as serial oversized stars.
        const uint64_t proposed=CPassiveComm::Allreduce(uint64_t(choices.size()+refused_candidates),CPassiveComm::Op::SUM,world.comm);
        if (proposed>stats.work_limit-stats.global_work) {
          stats.stop=EngineStop::WorkBudget; result.reason="Native 3D global proposal budget exhausted before admission; feasibility is undetermined.";
          return finish();
        }
        stats.global_work+=proposed;
      }
      {
        Timer timer(stats.dependency_seconds,options.timings);
        std::vector<Feature> features;
        try { for (const auto& candidate:choices) features.push_back(coarsening ? Feature{{candidate.edge[1],0,0},1} :
            Feature{{candidate.edge[0],candidate.edge[1],0},2}); }
        catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        if (!(ready=vote(failure))) return finish();
        if(!planned_coarsening)
          dependency_batch=directory->LookupBatch(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
        if (!dependency_batch.valid) { result.reason=dependency_batch.reason; return finish(); }
        imported_batch=directory->FetchBatch(dependency_batch,working,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes()));
        if (!imported_batch.valid) { result.reason=imported_batch.reason; return finish(); }
        try {
          for (size_t i=0;i<imported_batch.candidates.size();++i) {
            const auto& candidate=imported_batch.candidates[i];
            if (candidate.valid) continue;
            const auto& star=dependency_batch.candidates.at(i);
            if (!star.valid && star.oversized) { refuse(choices.at(i),features.at(i),star.reason); ++refused_slots; }
            else failure.Set(1,0,candidate.reason);
          }
        } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        if (!(ready=vote(failure))) return finish();
        if(world.maximum(refused_receipt_count!=0)) {
          if(!service_frontier({},false,false,&refused_receipts,refused_receipt_count))return finish();
          refused_receipt_count=0;
        }
      }
      {
        Timer timer(stats.reservation_seconds,options.timings);
        reserved=directory->ReserveBatch(imported_batch,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
        if (!reserved.valid) { result.reason=reserved.reason; return finish(); }
        size_t winners=0;
        for (bool chosen:reserved.chosen) winners+=chosen;
        for(size_t i=0;i<reserved.pass_deferred.size();++i)if(reserved.pass_deferred[i]) {
          if(refused_receipt_count==MaximumCandidateBatch)throw std::logic_error("Native construction deferrals exceed frontier bound.");
          const auto edge=choices.at(i).edge;
          refused_receipts[refused_receipt_count++]={Feature{{edge[0],edge[1],0},2},false};
          ++pass_deferred_candidates;++stats.length_pass_deferred;
        }
        if(world.maximum(refused_receipt_count!=0)) {
          if(!service_frontier({},false,false,&refused_receipts,refused_receipt_count))return finish();
          refused_receipt_count=0;
        }
        stats.batch_reserved+=winners; stats.conflicts+=choices.size()-winners-refused_slots;
        if (!world.maximum(winners!=0 || refused_candidates!=0 || pass_deferred_candidates!=0)) { result.reason="Native 3D batch reservation made no progress."; return finish(); }
      }
    } else {
      Timer timer(stats.selection_seconds,options.timings);
      if((curved || composition) && !repairing && !concurrent) {
        // Single-proposal requests and coarsening retain their established serial ordering.
        if(composition && !curved && !repairing) {
          // Choose global metric priority before ownership; rank numbering must not decide the next edit.
          const double priority=choice.active ? double(selection.Priority(choice.measure.length)) : INFINITY;
          const double best=CPassiveComm::Allreduce(priority,CPassiveComm::Op::MIN,world.comm);
          choice.active=choice.active && priority==best;
          const Id first=CPassiveComm::Allreduce(choice.active ? choice.edge[0] : UINT64_MAX,CPassiveComm::Op::MIN,world.comm);
          choice.active=choice.active && choice.edge[0]==first;
          const Id second=CPassiveComm::Allreduce(choice.active ? choice.edge[1] : UINT64_MAX,CPassiveComm::Op::MIN,world.comm);
          choice.active=choice.active && choice.edge[1]==second;
        }
        const int owner=CPassiveComm::Allreduce(choice.active ? world.rank : world.size,CPassiveComm::Op::MIN,world.comm);
        choice.active=choice.active && world.rank==owner;
      }
      if(repairing || concurrent || (coarsening && !repairing && !curved && !composition && world.size>1)) {
        CLocalFailure failure;
        try {if(choice.active)choices.push_back(choice);}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!vote(failure) || !coalesce(choices) || !plan_choices())return finish();
        choice=choices.empty()?Choice{}:choices.front();
      }
      stats.proposed+=choice.active;
      const uint64_t proposed=CPassiveComm::Allreduce(uint64_t(choice.active),CPassiveComm::Op::SUM,world.comm);
      if (proposed>stats.work_limit-stats.global_work) {
        stats.stop=EngineStop::WorkBudget; result.reason="Native 3D global proposal budget exhausted before admission; feasibility is undetermined.";
        return finish();
      }
      stats.global_work+=proposed;
    }
    for (size_t slot=0;slot<slots;++slot) {
    Dependencies dependencies;
    Imported imported;
    bool chosen=false,pass_deferred=false;
    auto reserve_dependencies=[&](const Imported& rows,bool active,std::string& why,size_t ceiling,size_t caller,unsigned priority) {
      const bool selected=directory->Reserve(rows,active,why,ceiling,caller,priority);
      if(chosen && directory->PassDeferred())++stats.length_expansion_deferrals;
      pass_deferred=pass_deferred||directory->PassDeferred();return selected;
    };
    if (batched) {
      choice=slot<choices.size() ? choices[slot] : Choice{};
      if (choice.active) {
        dependencies=std::move(dependency_batch.candidates[slot]);
        imported=std::move(imported_batch.candidates[slot]);
        chosen=reserved.chosen[slot];
      }
      if (!world.maximum(chosen)) continue;
    } else {
      {
        Timer timer(stats.dependency_seconds,options.timings);
        const Feature feature=repairing ? choice.feature : (coarsening ? Feature{{choice.edge[1],0,0},1} : Feature{{choice.edge[0],choice.edge[1],0},2});
        const auto features=choice.active ? std::vector<Feature>{feature} : std::vector<Feature>{};
        if(planned_coarsening) {
          // Every proposal of this round may have been refused for an oversized star during planning.
          if(!world.maximum(choice.active))continue;
          if(choice.active)dependencies=std::move(dependency_batch.candidates.at(0));else dependencies.valid=true;
        } else {
          auto batch=directory->LookupBatch(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()),repairing);
          if(!batch.valid) {result.reason=batch.reason;return finish();}
          if(choice.active)dependencies=std::move(batch.candidates.at(0));else dependencies.valid=true;
          const bool oversized=choice.active && !dependencies.valid && dependencies.oversized;
          CLocalFailure lookup_failure;
          if(choice.active && !dependencies.valid && !oversized)lookup_failure.Set(1,0,dependencies.reason);
          if(!vote(lookup_failure))return finish();
          if(oversized) {
            // Refuse this proposal; publication still requires its complete star. Changed stars reactivate it.
            try {refuse(choice,feature,dependencies.reason);}catch(const std::exception& e){lookup_failure.Set(2,0,e.what());}
            choice.active=false;dependencies=Dependencies{};dependencies.valid=true;
          }
          if(!vote(lookup_failure))return finish();
          if(world.maximum(oversized) && !service_frontier(repairing ? feature : Feature{{choice.edge[0],choice.edge[1],0},2},oversized,false))return finish();
          // Concurrent peers keep their own proposals; a slot whose every proposal was refused is skipped.
          if(!world.maximum(choice.active))continue;
        }
        if (dependencies.valid) imported=directory->Fetch(dependencies,working,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes()));
      }
      if (!dependencies.valid || !imported.valid) {
        result.reason=dependencies.valid ? imported.reason : dependencies.reason; return finish();
      }
      {
        Timer timer(stats.reservation_seconds,options.timings);
        chosen=reserve_dependencies(imported,choice.active,result.reason,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes()),choice.reservation_priority);
        stats.conflicts+=choice.active && !chosen;
        const int admitted=world.maximum(chosen?2:pass_deferred?1:0);
        if(!admitted)ready=false;
        else if(admitted==1) {
          if(!service_frontier(Feature{{choice.edge[0],choice.edge[1],0},2},choice.active&&pass_deferred,false))return finish();
          stats.length_pass_deferred+=pass_deferred;continue;
        }
      }
      if (!ready) return finish();
    }
    CLocalFailure failure;
    const bool coordinate_only=repairing && imported.coordinate_star.count==1 && imported.cells.size()>MaximumCavityCells;
    const bool coordinate_stream=world.maximum(coordinate_only);
    const size_t private_scratch=transfer_memory::Add(transfer_memory::Bytes(imported.cells),
        transfer_memory::Mul(std::max(MaximumReplacementCells,imported.cells.size()),8192),MaximumFrozenFieldBytes,MetricPatchSolveScratchBytes,
        transfer_memory::Mul(3*MaximumCampaignEdges,sizeof(std::pair<const Edge,EdgeMeasure>)+6*sizeof(void*)));
    // Admit large coordinate-star copies, surfaces, query regions and measurements before GeometryPatch
    // or source-region packing allocates them; the ordinary topology path keeps its existing bound.
    if(coordinate_stream && !admit(private_scratch))return finish();
    Patch old,fresh;
    Node point;
    bool approved=false, moved=false, repair_approved=false, cavity_inserted=false;
    bool optimization_approved=false;OptimizationOperations repair_optimization;
    MetricDiagnosis certified;
    bool geometric_progress=false;
    std::unique_ptr<FrozenField> field;
    const auto epoch=transfer_memory::Add(transfer_memory::Mul(round,world.size),world.rank);
    const auto token=transfer_memory::Add(transfer_memory::Mul(epoch,options.candidate_batch),slot);
    const auto offset=transfer_memory::Mul(token,MaximumReplacementCells);
    const size_t construction_nodes=!options.wall_heights.empty() ? MaximumReplacementCells+3 : composition ? 9 : curved ? 2 : 1;
    const size_t node_stride=construction_nodes+160;
    const auto node_offset=transfer_memory::Mul(token,node_stride);
    bool capture=false, capture_selected=false;
    const Feature transaction_feature=repairing ? choice.feature : Feature{{choice.edge[0],choice.edge[1],0},2};
    if(options.transaction_observer && options.capture_feature.count && !transaction_captured) {
      capture_selected=chosen && transaction_feature.count==options.capture_feature.count &&
          transaction_feature.vertices==options.capture_feature.vertices;
      capture=world.maximum(capture_selected);
      transaction_captured=capture;
    }
    auto observe_transaction=[&](const char* stage,bool passed,const DonorPatch* donors=nullptr) {
      if(capture)options.transaction_observer({stage,transaction_feature,uint64_t(round),capture_selected,passed,
          working,geometry,dependencies,imported,old,fresh,point,maximum_cell+1+offset,maximum_node+1+node_offset,
          certified,result.reason,donors,composition,options.edge_breakpoints});
    };
    auto guarded_geometry=[&](bool valid,const char* operation="geometry") {
      const bool passed=geometry_trial(valid && height_check(fresh,result.reason,true));
      observe_transaction(operation,passed);return passed;
    };
    auto repair_kind=[&]() {
      if(GetShapeAdmission()!=ShapeAdmission::ValidityOnly || options.wall_heights.empty())return MetricChange::Repair;
      const auto before=HeightScore(old,options.wall_heights,geometry),after=HeightScore(fresh,options.wall_heights,geometry);
      return std::isfinite(before) && after>before+1e-6 ? MetricChange::GeometryRepair : MetricChange::Repair;
    };
    auto geometric_change=[&](bool candidate,MetricChange kind) {
      if(!candidate || !curved)return candidate;
      // Entity refinement constructs valid cells; local surrogate descent is not a geometry permission.
      if(geometry.EntityGeometry()&&phase==Phase::Construction&&kind!=MetricChange::Coarsen)return true;
      try {
        const auto before=CertifiedGeometryExcess(old,geometry,options.surface_tolerance);
        const auto after=CertifiedGeometryExcess(fresh,geometry,options.surface_tolerance);
        geometric_progress=before>0 && after<before*(1-1e-12L);
        if((kind==MetricChange::GeometryRefine && !geometric_progress) ||
           (kind==MetricChange::Coarsen && after>0) ||
           (kind!=MetricChange::GeometryRefine && after>before &&
            !((kind==MetricChange::Repair || kind==MetricChange::GeometryRepair) && phase==Phase::Improvement))) {
          result.reason="Native3D candidate does not preserve/improve its frozen geometry error.";return guarded_geometry(false);
        }
        return true;
      }catch(const std::exception& e){result.reason=e.what();return guarded_geometry(false);}
    };
    try {observe_transaction("begin",chosen);}catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(options.transaction_observer && !vote(failure))return finish();
    Point splitPosition{},centralPosition{};bool split_failed=false,central_available=false;
    const bool physical_edge=chosen && std::any_of(imported.cells.begin(),imported.cells.end(),[&](const OwnedCell& c) {
      const auto faces=Faces(c.cell);
      for(unsigned i=0;i<4;++i)if(c.physical[i].present &&
          std::find(faces[i].begin(),faces[i].end(),choice.edge[0])!=faces[i].end() &&
          std::find(faces[i].begin(),faces[i].end(),choice.edge[1])!=faces[i].end())return true;
      return false;
    });
    SegmentFreedom crease_freedom=SegmentFreedom::Unresolved;Point crease_midpoint{};
    const long double diagnostic_limit=MaximumMetricEdge*(1+1e-8L); // Conservative diagnostic margin, never an acceptance tolerance.
    if(!repairing && !coarsening && composition && physical_edge && choice.measure.length>diagnostic_limit)try {
      std::set<Id> facets;
      for(const auto& row:imported.cells) {
        const auto faces=Faces(row.cell);
        for(unsigned i=0;i<4;++i)if(row.physical[i].present &&
            std::find(faces[i].begin(),faces[i].end(),choice.edge[0])!=faces[i].end() &&
            std::find(faces[i].begin(),faces[i].end(),choice.edge[1])!=faces[i].end())
          facets.insert(geometry.Canonical(row.physical[i].facet));
      }
      if(facets.size()==2) {
        const auto first=*facets.begin(),second=*facets.rbegin();
        if(!geometry.Declared(first) && !geometry.Declared(second)) {
          const auto& f=geometry.Get(first);const auto& g=geometry.Get(second);
          const bool folded=std::any_of(g.v.begin(),g.v.end(),[&](Node n){return Orientation(f.v[0].p,f.v[1].p,f.v[2].p,n.p)!=0;});
          if(folded)crease_freedom=Binary64SegmentFreedom(choice.measure.a,choice.measure.b,crease_midpoint);
        }
      }
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    auto incompatible_crease=[&](long double length) {
      std::ostringstream message;message<<std::setprecision(17)<<"Native3D exact-facet resolution conflict on crease edge "
          <<choice.edge[0]<<","<<choice.edge[1]<<": an interval without an interior binary64 point has frozen metric length "
          <<length<<" above "<<MaximumMetricEdge<<"; no admissible subdivision meets the current geometry/metric contract.";
      failure.Set(1,choice.edge[0],message.str());stats.last_rejected_feature={{choice.edge[0],choice.edge[1],0},2};
      if(!imported.cells.empty())stats.last_rejected_cavity_centroid=Centroid(imported.cells.front().cell);
    };
    if(crease_freedom==SegmentFreedom::None)incompatible_crease(choice.measure.length);
    if(!repairing && !coarsening && composition) {
      std::vector<Cell> query;
      try {if(chosen)query.push_back(imported.cells.front().cell);}catch(const std::exception& e){failure.Set(2,0,e.what());}
      if(!vote(failure))return finish();
      if(!admit(transfer_memory::Add(MaximumFrozenFieldBytes,transfer_memory::Bytes(imported.cells))))return finish();
      DonorPatch seed;
      {Timer timer(stats.donor_seconds,options.timings);
        seed=source.ImportMetricCells(query,options.memory_ceiling,transfer_memory::Add(bytes(),directory->ResidentBytes(),MaximumFrozenFieldBytes));}
      if(!seed.valid){result.reason=seed.reason;return finish();}
      if(chosen)try {
        Timer timer(stats.metric_seconds,options.timings);CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
        const auto components=AssociatedExtensions(seed.cells,geometry);
        observe_transaction("split_donors",seed.valid,&seed);
        FrozenField target(std::move(seed.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
        FieldAccounting accounting{stats,target};
        bool conflict=false;
        if(crease_freedom==SegmentFreedom::MidpointOnly) {
          const auto length=std::max(target.TargetEdgeLength(choice.measure.a,crease_midpoint),target.TargetEdgeLength(crease_midpoint,choice.measure.b));
          conflict=length>diagnostic_limit;if(conflict)incompatible_crease(length);
        }
        if(!conflict) {
          if(physical_edge){centralPosition=target.SegmentPoint(choice.measure.a,choice.measure.b,.5);central_available=true;}
          splitPosition=target.BalancedSplitPoint(choice.measure.a,choice.measure.b);
        }
      }catch(const std::exception& e){
        result.reason=e.what();split_failed=!(physical_edge && central_available);
        if(!split_failed)splitPosition=centralPosition;
      }
      if(!vote(failure))return finish();
    }
    {
      Timer timer(stats.reconstruction_seconds,options.timings);
      if (chosen && !split_failed) {
        CpuTimer private_cpu(stats.reconstruction_cpu_seconds,options.timings);
        ++stats.attempted;
        if ((maximum_node>UINT64_MAX-node_stride || node_offset>UINT64_MAX-maximum_node-node_stride) ||
            maximum_cell>UINT64_MAX-MaximumReplacementCells || offset>UINT64_MAX-maximum_cell-MaximumReplacementCells) {
          failure.Set(1,0,"Native 3D reserved identity range overflows.");
        } else try {
          old=GeometryPatch(imported, &geometry);
          const auto a=choice.measure.a,b=choice.measure.b;
          if (!repairing && coarsening) {
            if(!geometry.EntityGeometry()&&CollapseOutsideTolerance(old,choice.edge[1],choice.edge[0],geometry,options.surface_tolerance)) {
              result.reason="Native3D collapsed boundary leaves an original geometry sample outside tolerance.";
              approved=guarded_geometry(false,"CollapseVertex");
            } else approved=guarded_geometry(CollapseVertex(old,choice.edge[1],choice.edge[0],maximum_cell+1+offset,
              geometry,fresh,result.reason),"CollapseVertex");
          }
          else if (!repairing) {
            point={maximum_node+1+node_offset,{double((static_cast<long double>(a.x)+b.x)/2),
                double((static_cast<long double>(a.y)+b.y)/2),double((static_cast<long double>(a.z)+b.z)/2)}};
            if(composition)point.p=splitPosition;
            approved=guarded_geometry(SplitEdge(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason),"SplitEdge");
            if (!approved) {
              approved=guarded_geometry(InsertEdgeCavity(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason),"InsertEdgeCavity");
              cavity_inserted=approved;
            }
            if(!approved && physical_edge && composition && central_available &&
                (point.p.x!=centralPosition.x || point.p.y!=centralPosition.y || point.p.z!=centralPosition.z)) {
              point.p=centralPosition;
              approved=guarded_geometry(SplitEdge(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason),"SplitEdge");
              if(!approved) {
                approved=guarded_geometry(InsertEdgeCavity(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason),"InsertEdgeCavity");
                cavity_inserted=approved;
              }
            }
          }
        } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      }
      ready=vote(failure);
    }
    if (!ready) return finish();
    const bool metric_needed=chosen && (approved || repairing || !coarsening || bad_field);
    DonorPatch sensors;
    std::vector<Cell> regions;
    {
      Timer timer(stats.donor_seconds,options.timings);
      if (metric_needed) try { for (const auto& row : imported.cells) regions.push_back(row.cell); }
      catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      ready=vote(failure);
      if (ready && !curved && !coordinate_stream && world.maximum(metric_needed)) sensors=source.ImportCells(regions,options.memory_ceiling,
          transfer_memory::Add(bytes(),directory->ResidentBytes(),transfer_memory::Bytes(imported.cells),
              transfer_memory::Bytes(old.cells),transfer_memory::Bytes(old.surface),
              transfer_memory::Bytes(fresh.cells),transfer_memory::Bytes(fresh.surface)));
      else if (ready) sensors.valid=true;
    }
    bool streamed=ready && (curved || coordinate_stream || (!sensors.valid && DonorCap(sensors.reason)));
    if (!ready || (!sensors.valid && !streamed)) { if (ready) result.reason=sensors.reason; return finish(); }
    if (streamed && !admit(private_scratch)) return finish();
    if (streamed) ++stats.private_query_retries;
    size_t repair_budget=0,repair_work=0;
    auto reserve_optimization=[&](bool eligible) {
      const auto count=CPassiveComm::Allreduce(uint64_t(eligible),CPassiveComm::Op::SUM,world.comm);
      const auto budget=count?std::min<uint64_t>(160,(stats.work_limit-stats.global_work)/count):0;
      const auto extra=eligible?std::min<uint64_t>(budget,160-repair_work):0;
      stats.global_work+=CPassiveComm::Allreduce(uint64_t(extra),CPassiveComm::Op::SUM,world.comm);
      repair_budget=repair_work+extra;
    };
    auto release_optimization=[&]() {
      stats.global_work-=CPassiveComm::Allreduce(uint64_t(repair_budget-repair_work),CPassiveComm::Op::SUM,world.comm);
      repair_budget=repair_work;
    };
    reserve_optimization(!streamed && chosen && repairing);
    if(world.maximum(repair_budget>repair_work) &&
        !admit(transfer_memory::Add(private_scratch,transfer_memory::Bytes(sensors.cells))))return finish();
    auto optimize_patch=[&](const Patch& candidate,const RegionMetric& target,bool preserve_vertices) {
      optimization_approved=false;repair_optimization={};
      const auto reserved=repair_budget-repair_work;
      if(!reserved)return false;
      const Id first_node=maximum_node+1+node_offset+construction_nodes+repair_work;
      size_t used=reserved;repair_work+=reserved;
      ++stats.repair_proposals;++stats.optimization_trials;
      const bool changed=OptimizedRepairPatch(candidate,maximum_cell+1+offset,first_node,target,
                                              fresh,result.reason,used,preserve_vertices,geometry,options.wall_heights,options.surface_tolerance,repair_optimization);
      repair_work-=reserved-used;
      return changed;
    };
    {
      Timer timer(stats.metric_seconds,options.timings);
      const double reconstruction_before=stats.reconstruction_seconds;
      const double reconstruction_cpu_before=stats.reconstruction_cpu_seconds;
      if (!streamed && metric_needed) try {
        CpuTimer private_cpu(stats.metric_cpu_seconds,options.timings);
        const auto components=AssociatedExtensions(sensors.cells,geometry);
        observe_transaction("support_donors",sensors.valid,&sensors);
        field=std::make_unique<FrozenField>(std::move(sensors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
        PatchMetric metric;
        std::map<Edge,EdgeMeasure> reuse;
        for (const auto& cell : old.cells) selection.Cached(cell,reuse);
        if (approved) {
          approved=metric_trial(ValidateMetricChange(old.cells,fresh.cells,construction_kind,*field,metric,result.reason,nullptr,&reuse,&certified),certified);
        }
        if (!approved) cavity_inserted=false;
        if (repairing) {
          auto try_repair=[&](bool move,Id vertex) {
            std::map<Id,Node> nodes;
            for (const auto& cell:old.cells) for (auto node:cell.v) nodes.emplace(node.id,node);
            const Id first=maximum_cell+1+offset;
            if (move) {
              if (maximum_node>UINT64_MAX-node_stride || node_offset>UINT64_MAX-maximum_node-node_stride) return false;
              std::set<Id> neighbors;
              for (auto face:old.surface) if (std::find(face.v.begin(),face.v.end(),vertex)!=face.v.end())
                neighbors.insert(face.v.begin(),face.v.end());
              if (neighbors.empty()) for (const auto& row:nodes) neighbors.insert(row.first);
              neighbors.erase(vertex);
              long double x=0,y=0,z=0;
              for (Id id:neighbors) { auto p=nodes.at(id).p; x+=p.x; y+=p.y; z+=p.z; }
              Point center{};
              if(!neighbors.empty()) {
                const auto origin=nodes.at(vertex).p;center={double(x/neighbors.size()),double(y/neighbors.size()),double(z/neighbors.size())};
                if(composition) {
                  std::vector<std::pair<Point,Tensor>> samples;
                  for(auto id:neighbors) {
                    const auto q=nodes.at(id).p;
                    samples.emplace_back(q,field->QueryMidpoint(origin,q).target);
                  }
                  center=MetricMeanPoint(origin,samples);
                }
              }
              if (!neighbors.empty()) for (long double step:{1.L,.5L,.25L}) {
                const auto p=nodes.at(vertex).p;
                point={maximum_node+1+node_offset,{double(p.x+step*(center.x-p.x)),
                    double(p.y+step*(center.y-p.y)),double(p.z+step*(center.z-p.z))}};
                if(composition)point=ConstrainMovePoint(old,vertex,point,options.wall_heights,geometry);
                ++stats.repair_proposals;
                { Timer geometry_timer(stats.reconstruction_seconds,options.timings); CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds,options.timings);
                approved=guarded_geometry(MoveVertex(old,vertex,point,first,geometry,fresh,result.reason),"MoveVertex"); }
                if (approved) approved=metric_trial(ValidateMetricChange(old.cells,fresh.cells,repair_kind(),*field,metric,result.reason,nullptr,&reuse,&certified),certified);
                if (approved) { moved=true; break; }
              }
            } else {
              unsigned proposals=0;
              for (const auto& row:nodes) {
                if (++proposals>16) break;
                ++stats.repair_proposals;
                { Timer geometry_timer(stats.reconstruction_seconds,options.timings); CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds,options.timings);
                  approved=guarded_geometry(Reconnect(old,row.first,first,geometry,fresh,result.reason),"Reconnect"); }
                if (approved) approved=metric_trial(ValidateMetricChange(old.cells,fresh.cells,repair_kind(),*field,metric,result.reason,nullptr,&reuse,&certified),certified);
                if (approved) break;
              }
            }
            if(!approved && repair_work<repair_budget) {
              {Timer geometry_timer(stats.reconstruction_seconds,options.timings);CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds,options.timings);
                approved=guarded_geometry(optimize_patch(old,RegionMetric(*field),false));}
              if(approved)approved=metric_trial(ValidateMetricChange(old.cells,fresh.cells,repair_kind(),*field,metric,result.reason,nullptr,&reuse,&certified),certified);
              if(approved){moved=false;optimization_approved=true;++stats.optimization_validated;}
            }
            return approved;
          };
          approved=try_repair(choice.feature.count==1,choice.feature.vertices[0]);
          repair_approved=approved;
        }
      } catch (const std::exception& e) { approved=false; result.reason=e.what(); }
      if (field) FieldCounts(stats,*field);
      stats.metric_seconds-=stats.reconstruction_seconds-reconstruction_before;
      stats.metric_cpu_seconds-=stats.reconstruction_cpu_seconds-reconstruction_cpu_before;
    }
    release_optimization();
    if(!streamed && composition && !repairing && !coarsening) {
      if(world.maximum(chosen && !approved)) {
        field.reset();if(!admit(private_scratch))return finish();
        streamed=true;++stats.private_query_retries;
      }
    }
    if (streamed) {
      // One bounded stage cache; only exact endpoint/field matches authorize reuse across proposals.
      auto measure_private=[&](const std::vector<Cell>& cells, PatchMetric& measured, bool shape,
                               const std::map<Edge,EdgeMeasure>& frozen_reuse) {
        CLocalFailure local;
        bool usable=true;
        std::string rejected;
        std::map<Edge,EdgeMeasure> reuse;
        try { reuse=frozen_reuse; } catch (const std::exception& e) { local.Set(2,0,e.what()); }
        if (!(ready=vote(local))) return false;
        size_t count=CPassiveComm::Allreduce(cells.size(),CPassiveComm::Op::MAX,world.comm);
        measured={};
        const bool measure_shape=phase==Phase::Improvement;
        if(measure_shape) {
          measured.minimum_mean_ratio=measured.minimum_scaled_jacobian=INFINITY;measured.shape_measured=true;
          if(!cells.empty())measured.weakest_cell=measured.sliver_cell=cells.front().id;
        }
        for (size_t slot=0;slot<count;) {
          size_t batch=std::min({MeasurementBatch,MaximumCavityCells/6,count-slot});
          DonorPatch donors;
          for (;;) {
            std::vector<Cell> query;
            try {
              if (usable) for (size_t i=slot;i<std::min(slot+batch,cells.size());++i) query.push_back(cells[i]);
            } catch (const std::exception& e) { local.Set(2,0,e.what()); }
            std::vector<unsigned char> requests;
            try {requests=MetricEdgeRequests(query,reuse,field_identity,&campaign_edges);}
            catch(const std::exception& e){local.Set(2,0,e.what());}
            if (!(ready=vote(local))) return false;
            {
              Timer timer(stats.donor_seconds,options.timings);
              ++stats.donor_batches;
              donors=source.ImportMetricSamples(query,eligible_extensions,options.memory_ceiling,
                  transfer_memory::Add(bytes(),directory->ResidentBytes(),private_scratch),requests,measure_shape);
            }
            if (donors.valid) break;
            if (!DonorCap(donors.reason) || batch==1) { ready=false; result.reason=donors.reason; return false; }
            batch=(batch+1)/2; ++stats.batch_retries;
          }
          if (usable && slot<cells.size()) {
            Timer timer(stats.metric_seconds,options.timings);
            CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
            MetricStats counted;
            std::unique_ptr<FrozenField> target;
            try {
              observe_transaction("metric_donors",donors.valid,&donors);
              const auto components=AssociatedExtensions(donors.cells,geometry);
              observe_transaction("support_donors",donors.valid,&donors);
              target=std::make_unique<FrozenField>(std::move(donors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
              for (size_t i=slot;i<std::min(slot+batch,cells.size());++i) {
                const auto part=measure_shape ? MeasureMetricCells({cells[i]},*target,&counted,&reuse,shape,shape ? &certified : nullptr,&campaign_edges) :
                    MeasureMetricEdges({cells[i]},*target,&counted,&reuse,&campaign_edges);
                if (measure_shape && part.minimum_mean_ratio<measured.minimum_mean_ratio) {
                  measured.minimum_mean_ratio=part.minimum_mean_ratio; measured.weakest_cell=part.weakest_cell;
                }
                if (measure_shape && part.minimum_scaled_jacobian<measured.minimum_scaled_jacobian) {
                  measured.minimum_scaled_jacobian=part.minimum_scaled_jacobian; measured.sliver_cell=part.sliver_cell;
                }
                for (const auto& edge:part.edges) {
                  measured.edges.emplace(edge); reuse[edge.first]=edge.second;
                  if (edge.second.length>measured.maximum_length) {
                    measured.maximum_length=edge.second.length; measured.longest_edge=edge.first;
                  }
                }
              }
            } catch (const std::exception& e) { usable=false; rejected=e.what(); }
            if (target) FieldCounts(stats,*target);
            stats.private_edge_evaluations+=counted.edge_evaluations;
            stats.private_edge_reused+=counted.edge_reused;stats.private_edge_campaign_hits+=counted.edge_campaign_reused;
          }
          slot+=batch;
        }
        if (!usable) result.reason=rejected;
        return usable;
      };
      std::map<Edge,EdgeMeasure> reuse;
      { Timer timer(stats.metric_seconds,options.timings);
        try { for (const auto& cell:old.cells) selection.Cached(cell,reuse); }
        catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        ready=vote(failure);
      }
      if (!ready) return finish();
      PatchMetric before;
      const std::vector<Cell> empty;
      const bool old_measured=measure_private(metric_needed ? old.cells : empty,before,false,reuse);
      // A failed measurement of unchanged accepted geometry invalidates the event on every worker.
      if (!ready) return finish();
      CLocalFailure old_failure;
      if (metric_needed && !old_measured) old_failure.Set(1,0,result.reason);
      if (!(ready=vote(old_failure))) return finish();
      // Refits and joint placement give each moved apex a new identity from maximum_node+2+node_offset on (one for one);
      // the split point is maximum_node+1+node_offset. Only those callers certify relocations (RefineAcceptance).
      auto reserved_relocations=[&]() {
        std::set<Id> previous,ids;for(const auto& c:old.cells)for(auto n:c.v)previous.insert(n.id);
        for(const auto& c:fresh.cells)for(auto n:c.v)
          if(n.id>=maximum_node+2+node_offset && n.id<maximum_node+1+node_offset+construction_nodes && !previous.count(n.id))ids.insert(n.id);
        return ids.size();
      };
      auto validate_private=[&](bool candidate,MetricChange kind,bool no_new_oversized=false,bool strict_size=false,size_t relocated=0) {
        const bool completed=concurrent && approved && !candidate;
        const auto retained=certified;
        certified={};
        candidate=geometric_change(candidate,kind);
        PatchMetric metric;
        const std::vector<Cell> empty;
        bool measured=measure_private(candidate ? fresh.cells : empty,metric,true,before.edges);
        CLocalFailure observed;
        bool valid=false;
        { Timer timer(stats.metric_seconds,options.timings); CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
          if (candidate) {
            const auto effective=kind==MetricChange::Repair ? (geometric_progress?MetricChange::GeometryRepair:repair_kind()) : kind;
            if (measured) valid=ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),metric,effective,result.reason,&certified,relocated,coordinate_only);
            if(valid && no_new_oversized)for(const auto& edge:metric.edges)if(edge.second.length>MaximumMetricEdge) {
              const auto previous=before.edges.find(edge.first);
              auto same=[](Point a,Point b){return a.x==b.x && a.y==b.y && a.z==b.z;};
              if(previous==before.edges.end() || !same(previous->second.a,edge.second.a) || !same(previous->second.b,edge.second.b)) {
                valid=false;certified.failure=MetricFailure::Progress;
                result.reason="Native3D joint strip introduces a new oversized edge.";break;
              }
            }
            if(valid && strict_size && !certified.size_progress) {
              valid=false;certified.failure=MetricFailure::Progress;result.reason="Native3D compound wall repair does not reduce frozen changed-edge excess.";
            }
            metric_trial(valid,certified);
          }
        }
        if(chosen && !valid && !completed) {
          stats.last_metric_rejection=certified;
          stats.last_rejected_feature=repairing ? choice.feature : Feature{{choice.edge[0],choice.edge[1],0},2};
          if(!old.cells.empty())stats.last_rejected_cavity_centroid=Centroid(old.cells.front());
          last_rejection="feature "+std::to_string(stats.last_rejected_feature.count)+":"+
              std::to_string(stats.last_rejected_feature.vertices[0])+": "+result.reason;
          if(options.transaction_observer && !options.capture_feature.count && !transaction_captured)try {
            transaction_captured=true;
            options.transaction_observer({repairing?"first_repair_failure":coarsening?"first_collapse_failure":"first_split_failure",
                transaction_feature,uint64_t(round),true,false,working,geometry,dependencies,imported,old,fresh,point,
                maximum_cell+1+offset,maximum_node+1+node_offset,certified,result.reason,nullptr,composition,options.edge_breakpoints});
          }catch(const std::exception& e){observed.Set(2,0,e.what());}
          if(options.rejected_trial)try {
            options.rejected_trial(old,fresh,stats.last_rejected_feature,certified,candidate,result.reason);
          }catch(const std::exception& e){observed.Set(2,0,e.what());}
        }
        try {observe_transaction("metric",valid);}catch(const std::exception& e){observed.Set(2,0,e.what());}
        if(observe_rejections || (!repairing && !coarsening))ready=vote(observed);
        if(completed)certified=retained;
        return valid;
      };
      approved=validate_private(metric_needed && old_measured && approved,choice.geometric ? MetricChange::GeometryRefine : construction_kind);
      if (!approved) cavity_inserted=false;
      if (!ready) return finish();
      const bool boundary_edge=std::any_of(old.surface.begin(),old.surface.end(),[&](const auto& f) {
        return std::find(f.v.begin(),f.v.end(),choice.edge[0])!=f.v.end() && std::find(f.v.begin(),f.v.end(),choice.edge[1])!=f.v.end();
      });
      bool joint=chosen && !split_failed && (choice.geometric || composition) && !repairing && !coarsening && !approved;
      bool flat_edge=true;
      for(const auto& f:old.surface)if(std::find(f.v.begin(),f.v.end(),choice.edge[0])!=f.v.end() &&
          std::find(f.v.begin(),f.v.end(),choice.edge[1])!=f.v.end()) {
        const auto authority=geometry.Analytic().find(f.facet);
        if(authority!=geometry.Analytic().end() && authority->second.kind!=AnalyticSurface::Kind::Disk)flat_edge=false;
      }
      // Flat caps/interior regions retain coordinated-strip freedom even in a domain with curved walls.
      bool strip=joint && composition && flat_edge && !choice.geometric;
      std::vector<Id> wallApices;
      if(joint && boundary_edge)for(const auto& f:old.surface) {
        if(!options.wall_heights.count(geometry.Get(f.facet).marker) ||
            std::find(f.v.begin(),f.v.end(),choice.edge[0])==f.v.end() ||
            std::find(f.v.begin(),f.v.end(),choice.edge[1])==f.v.end())continue;
        for(const auto& c:old.cells) {
          unsigned count=0;Node apex;
          for(auto n:c.v)if(std::find(f.v.begin(),f.v.end(),n.id)!=f.v.end())++count;else apex=n;
          if(count==3)wallApices.push_back(apex.id);
        }
      }
      std::sort(wallApices.begin(),wallApices.end());wallApices.erase(std::unique(wallApices.begin(),wallApices.end()),wallApices.end());
      bool wallSplit=joint && !wallApices.empty();
      Id placement_obstruction=UINT64_MAX;
      auto place_patch=[&](bool search,const std::vector<Id>& apices,Id height_apex=UINT64_MAX) {
        if(!world.maximum(search))return true;
        if(stats.global_work>=stats.work_limit){stats.stop=EngineStop::WorkBudget;result.reason="Native3D joint placement exhausted the global work budget.";return false;}
        if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
          stats.stop=EngineStop::TimeBudget;result.reason="Native3D joint placement exhausted the elapsed budget.";return false;
        }
        std::vector<Feature> features;
        if(search) {
          features.push_back({{choice.edge[0],choice.edge[1],0},2});
          for(Id id:apices)features.push_back({{id,0,0},1});
        }
        Dependencies joined;
        {Timer timer(stats.dependency_seconds,options.timings);
          joined=directory->Lookup(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!joined.valid) {
          if(joined.reason=="Native 3D dependency star exceeds budget." || joined.reason=="Native 3D dependency union exceeds budget.")search=false;
          else failure.Set(1,0,joined.reason);
        }
        if(!(ready=vote(failure)))return false;
        if(!search){joined.valid=true;joined.cells.clear();}
        Imported more;
        {Timer timer(stats.dependency_seconds,options.timings);
          more=directory->Fetch(joined,working,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!more.valid){result.reason=more.reason;return false;}
        if(search)try {
          std::set<Id> required;
          for(const auto& c:old.cells)if(std::count_if(c.v.begin(),c.v.end(),[&](Node n){return n.id==choice.edge[0] || n.id==choice.edge[1];})==2)required.insert(c.id);
          // Discard unused support from abandoned proposals; the actual edge and moved stars remain complete.
          for(const auto& dep:dependencies.cells)if(required.count(dep.cell)) {
            const auto found=std::find_if(joined.cells.begin(),joined.cells.end(),[&](const auto& d){return d.cell==dep.cell;});
            if(found==joined.cells.end() || found->version!=dep.version || found->owner!=dep.owner)
              throw std::runtime_error("Native3D joint placement lost a complete/versioned dependency.");
          }
          dependencies=std::move(joined);imported=std::move(more);old=GeometryPatch(imported, &geometry);reuse.clear();
          for(const auto& c:old.cells)selection.Cached(c,reuse);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return false;
        bool reserved=false;
        {Timer timer(stats.reservation_seconds,options.timings);
          reserved=reserve_dependencies(imported,search || approved,result.reason,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch),choice.reservation_priority);}
        if((search || approved) && !reserved) {
          if(directory->PassDeferred() || result.reason=="Native 3D conflicting dependency reservation.") {
            ++stats.conflicts;
            if(directory->PassDeferred() || !concurrent){search=false;approved=false;if(directory->PassDeferred())joint=false;}
          } else failure.Set(1,0,"Native3D joint placement lost its complete-star reservation: "+result.reason);
        }
        if(!(ready=vote(failure)))return false;
        PatchMetric expanded_before;
        const bool measured=measure_private(search?old.cells:empty,expanded_before,false,reuse);
        if(search && measured)before=std::move(expanded_before);
        if(!ready)return false;
        if(search && !measured)failure.Set(1,0,result.reason);
        if(!(ready=vote(failure)))return false;
        DonorPatch support;
        {Timer timer(stats.donor_seconds,options.timings);++stats.donor_batches;
          support=source.ImportCells(search?old.cells:empty,options.memory_ceiling,transfer_memory::Add(bytes(),directory->ResidentBytes(),private_scratch));}
        if(!support.valid) {
          if(DonorCap(support.reason))search=false;
          else {result.reason=support.reason;return false;}
        }
        const auto participants=concurrent ? CPassiveComm::Allreduce(uint64_t(search),CPassiveComm::Op::SUM,world.comm) : uint64_t(1);
        const auto allowance=std::min<uint64_t>(256,(stats.work_limit-stats.global_work)/std::max<uint64_t>(1,participants));
        if(!allowance){stats.stop=EngineStop::WorkBudget;result.reason="Native3D joint placement cannot admit simultaneous work within the global budget.";return false;}
        bool candidate=false;JointSplitReport report;
        if(search)try {
          Timer timer(stats.reconstruction_seconds,options.timings);CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
          const auto components=AssociatedExtensions(support.cells,geometry);
          observe_transaction("support_donors",support.valid,&support);
          FrozenField target(std::move(support.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
          FieldAccounting accounting{stats,target};std::vector<std::pair<Id,Id>> reserved_apices;
          for(size_t i=0;i<apices.size();++i)reserved_apices.emplace_back(apices[i],maximum_node+2+i+node_offset);
          if(height_apex!=UINT64_MAX) {
            candidate=ReconstructHeightStar(old,height_apex,maximum_node+1+node_offset,maximum_cell+1+offset,target,
                geometry,options.wall_heights,fresh,report,result.reason,allowance);
            stats.wall_reconstruction_trials+=report.trials;stats.wall_topology_trials+=report.trials;
          } else candidate=PlaceSplitPatch(old,choice.edge,point,reserved_apices,maximum_cell+1+offset,target,geometry,options.wall_heights,
              fresh,report,result.reason,allowance);
          stats.repair_proposals+=report.trials;stats.placement_trials+=report.trials;
          stats.placement_connectivity_trials+=report.connectivity_trials;stats.placement_flips+=report.flips;stats.placement_moves+=report.moves;
          stats.metric_linearizations+=report.linearizations;stats.metric_probes+=report.metric_probes;
          stats.metric_linear_iterations+=report.linear_iterations;stats.metric_solve_trials+=report.solve_trials;
          placement_obstruction=report.blocked_interior_vertex;
          if(!candidate && !report.best.cells.empty()){fresh=std::move(report.best);candidate=true;}
          candidate=guarded_geometry(candidate);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return false;
        stats.global_work+=concurrent ? CPassiveComm::Allreduce(uint64_t(report.trials),CPassiveComm::Op::SUM,world.comm) : world.maximum(report.trials);
        const auto retained_metric=certified;
        const bool passed=validate_private(candidate,MetricChange::Refine,false,false,
            height_apex==UINT64_MAX && candidate ? reserved_relocations() : 0);
        if(search && !approved){approved=passed;stats.placement_validated+=passed;}
        else if(!search)certified=retained_metric;
        if(!ready)return false;
        if(search && approved){cavity_inserted=true;joint=false;}
        return true;
      };
      Id height_apex=UINT64_MAX;
      if(joint && composition && !boundary_edge && !choice.geometric && !options.wall_heights.empty())
        for(Id vertex:choice.edge) {
          size_t walls=0;
          for(const auto& f:old.surface)if(options.wall_heights.count(geometry.Get(f.facet).marker) &&
              std::find(f.v.begin(),f.v.end(),vertex)==f.v.end())
            for(const auto& c:old.cells) {
              unsigned matches=0;bool apex=false;
              for(auto n:c.v){matches+=std::find(f.v.begin(),f.v.end(),n.id)!=f.v.end();apex=apex || n.id==vertex;}
              walls+=matches==3 && apex;
            }
          if(walls>=2){height_apex=vertex;break;}
        }
      if(!place_patch(height_apex!=UINT64_MAX,{height_apex},height_apex))return finish();
      if(!place_patch(joint && composition && (!boundary_edge || (flat_edge && wallApices.empty())) && !choice.geometric,{}))return finish();
      const Id initial_placement_obstruction=placement_obstruction;
      strip=strip && !wallSplit && !approved;  // Admit the required wall region before optional unrelated movers.
      std::vector<Id> movers;
      if(strip)for(const auto& f:old.surface) {
        if(std::find(f.v.begin(),f.v.end(),choice.edge[0])==f.v.end() || std::find(f.v.begin(),f.v.end(),choice.edge[1])==f.v.end())continue;
        for(auto id:f.v)if(id!=choice.edge[0] && id!=choice.edge[1])movers.push_back(id);
      }
      if(strip && !boundary_edge)movers={choice.edge[0],choice.edge[1]};
      if(strip && boundary_edge) {
        std::map<Id,unsigned> interior;
        for(const auto& c:old.cells)for(auto n:c.v)if(n.id!=choice.edge[0] && n.id!=choice.edge[1] &&
            std::none_of(old.surface.begin(),old.surface.end(),[&](const auto& f){return std::find(f.v.begin(),f.v.end(),n.id)!=f.v.end();}))++interior[n.id];
        const auto best=std::max_element(interior.begin(),interior.end(),[](const auto& a,const auto& b){return a.second<b.second;});
        if(best!=interior.end() && best->second>=2)movers.push_back(best->first);
      }
      std::sort(movers.begin(),movers.end());movers.erase(std::unique(movers.begin(),movers.end()),movers.end());
      strip=strip && movers.size()>=2 && movers.size()<=3;
      if(world.maximum(strip || wallSplit)) {
        std::vector<Feature> features;
        if(strip || wallSplit) {
          features.push_back({{choice.edge[0],choice.edge[1],0},2});
          if(strip)for(auto id:movers)features.push_back({{id,0,0},1});
          for(auto id:wallApices)features.push_back({{id,0,0},1});
        }
        Dependencies joined;
        {Timer timer(stats.dependency_seconds,options.timings);
          joined=directory->Lookup(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!joined.valid) {
          if(joined.reason=="Native 3D dependency star exceeds budget." || joined.reason=="Native 3D dependency union exceeds budget.")strip=wallSplit=false;
          else failure.Set(1,0,joined.reason);
        }
        if(!(ready=vote(failure)))return finish();
        if(!strip && !wallSplit){joined.valid=true;joined.cells.clear();}
        Imported more;
        {Timer timer(stats.dependency_seconds,options.timings);
          more=directory->Fetch(joined,working,options.memory_ceiling,
              transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!more.valid){result.reason=more.reason;return finish();}
        if(strip || wallSplit)try {
          for(const auto& olddep:dependencies.cells) {
            const auto found=std::find_if(joined.cells.begin(),joined.cells.end(),[&](const auto& dep){return dep.cell==olddep.cell;});
            if(found==joined.cells.end() || found->version!=olddep.version || found->owner!=olddep.owner)
              throw std::runtime_error("Native3D strip lost an original complete/versioned edge dependency.");
          }
          dependencies=std::move(joined);imported=std::move(more);old=GeometryPatch(imported, &geometry);reuse.clear();
          for(const auto& c:old.cells)selection.Cached(c,reuse);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        bool reserved=false;
        {Timer timer(stats.reservation_seconds,options.timings);
          reserved=reserve_dependencies(imported,strip || wallSplit,result.reason,options.memory_ceiling,
              transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch),choice.reservation_priority);}
        if((strip || wallSplit) && !reserved) {
          if(directory->PassDeferred()) {++stats.conflicts;strip=false;wallSplit=false;joint=false;approved=false;}
          else if(concurrent && result.reason=="Native 3D conflicting dependency reservation.")++stats.conflicts;
          else failure.Set(1,0,"Native3D strip lost its complete-star reservation.");
        }
        if(!(ready=vote(failure)))return finish();
        const bool measured=measure_private(joint?old.cells:empty,before,false,reuse);
        if(!ready)return finish();
        if(joint && !measured)failure.Set(1,0,result.reason);
        if(!(ready=vote(failure)))return finish();
        DonorPatch donors;
        {Timer timer(stats.donor_seconds,options.timings);++stats.donor_batches;
          donors=source.ImportCells(strip?old.cells:empty,options.memory_ceiling,
              transfer_memory::Add(bytes(),directory->ResidentBytes(),private_scratch));}
        if(!donors.valid) {
          if(DonorCap(donors.reason))strip=false;
          else {result.reason=donors.reason;return finish();}
        }
        std::array<Point,3> splits{};Edge extraEdge{};Node extraPoint,offEdgePoint;bool extraSplit=false;
        std::array<std::vector<std::pair<Id,Node>>,3> moveSeeds;std::vector<std::pair<Id,Node>> normalMoves;
        if(strip)try {
          Timer timer(stats.metric_seconds,options.timings);CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
          const auto components=AssociatedExtensions(donors.cells,geometry);
          observe_transaction("support_donors",donors.valid,&donors);
          FrozenField target(std::move(donors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
          FieldAccounting accounting{stats,target};std::map<Id,Node> nodes;
          for(const auto& c:old.cells)for(auto n:c.v)nodes.emplace(n.id,n);
          const std::array<double,3> fractions{{.5,.55,.565}};
          if(boundary_edge)for(size_t i=0;i<fractions.size();++i)splits[i]=target.BalancedSplitPoint(nodes.at(choice.edge[0]).p,nodes.at(choice.edge[1]).p,fractions[i]);
          for(size_t i=0;i<movers.size();++i) {
            const auto id=movers[i];const auto origin=nodes.at(id).p;std::set<Id> neighbors;
            for(const auto& f:old.surface)if(std::find(f.v.begin(),f.v.end(),id)!=f.v.end())neighbors.insert(f.v.begin(),f.v.end());
            const bool physicalMover=!neighbors.empty();
            if(neighbors.empty())for(const auto& c:old.cells) {
              if(std::any_of(c.v.begin(),c.v.end(),[&](Node n){return n.id==id;}))for(auto n:c.v)neighbors.insert(n.id);
            }
            neighbors.erase(id);std::vector<std::pair<Point,Tensor>> values;
            for(auto neighbor:neighbors) {
              const auto q=nodes.at(neighbor).p;
              values.emplace_back(q,target.QueryMidpoint(origin,q).target);
            }
            const Node mean=ConstrainMovePoint(old,id,{maximum_node+2+i+node_offset,MetricMeanPoint(origin,values)},options.wall_heights,geometry);
            for(size_t damping=0;damping<3;++damping) {
              auto proposal=mean;
              const double stiffness=physicalMover ? (damping==1?1.:damping==2?3.:0.) : (damping==2?1.:0.);
              if(stiffness) {
                auto t=target.Query(origin).target;
                const auto scale=std::max({std::abs(t.xx),std::abs(t.xy),std::abs(t.xz),std::abs(t.yy),std::abs(t.yz),std::abs(t.zz)});
                const auto factor=stiffness/scale;t={t.xx*factor,t.xy*factor,t.xz*factor,t.yy*factor,t.yz*factor,t.zz*factor};
                proposal.p=MetricMeanPoint(origin,{{mean.p,Tensor{}},{origin,t}});
                proposal=ConstrainMovePoint(old,id,proposal,options.wall_heights,geometry);
              }
              moveSeeds[damping].emplace_back(id,proposal);
            }
          }
          if(boundary_edge) {
            std::vector<SurfaceFace> incident;
            for(const auto& f:old.surface)if(std::find(f.v.begin(),f.v.end(),choice.edge[0])!=f.v.end() &&
                std::find(f.v.begin(),f.v.end(),choice.edge[1])!=f.v.end())incident.push_back(f);
            if(incident.size()==2 && geometry.Canonical(incident[0].facet)==geometry.Canonical(incident[1].facet)) {
              const auto a=nodes.at(incident[0].v[0]).p,b=nodes.at(incident[0].v[1]).p,c=nodes.at(incident[0].v[2]).p;
              const long double u[3]={(long double)b.x-a.x,(long double)b.y-a.y,(long double)b.z-a.z};
              const long double v[3]={(long double)c.x-a.x,(long double)c.y-a.y,(long double)c.z-a.z};
              std::array<long double,3> normal{{u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]}};
              const auto norm=std::hypot(normal[0],normal[1],normal[2]);
              if(!(norm>0))throw std::runtime_error("Native3D strip has a degenerate physical face.");
              for(auto& x:normal)x/=norm;
              std::vector<std::pair<Id,Node>> inward;
              for(size_t i=0;i<movers.size();++i) {
                const Id id=movers[i];
                if(std::any_of(old.surface.begin(),old.surface.end(),[&](const auto& f){return std::find(f.v.begin(),f.v.end(),id)!=f.v.end();}))continue;
                const auto origin=nodes.at(id).p;const auto m=target.Query(origin).target;
                const auto side=normal[0]*((long double)origin.x-a.x)+normal[1]*((long double)origin.y-a.y)+normal[2]*((long double)origin.z-a.z);
                const auto quadratic=m.xx*normal[0]*normal[0]+m.yy*normal[1]*normal[1]+m.zz*normal[2]*normal[2]+
                    2*(m.xy*normal[0]*normal[1]+m.xz*normal[0]*normal[2]+m.yz*normal[1]*normal[2]);
                if(!(quadratic>0) || !std::isfinite(quadratic))throw std::runtime_error("Native3D strip normal metric is unrepresentable.");
                if(side==0)continue;
                const auto step=std::copysign(.25L/std::sqrt(quadratic),side);
                Node proposal{maximum_node+2+i+node_offset,{double(origin.x+step*normal[0]),double(origin.y+step*normal[1]),double(origin.z+step*normal[2])}};
                inward.emplace_back(id,ConstrainMovePoint(old,id,proposal,options.wall_heights,geometry));
              }
              // A fixed cap's free apex can need depth freedom without moving its neighboring surface points.
              normalMoves=std::move(inward);
            }
          }
          if(boundary_edge)for(size_t i=0;i<movers.size();++i) {
            const Id id=movers[i];
            if(std::any_of(old.surface.begin(),old.surface.end(),[&](const auto& f){return std::find(f.v.begin(),f.v.end(),id)!=f.v.end();}))continue;
            long double longest=MaximumMetricEdge;
            for(Id endpoint:choice.edge) {
              const Edge edge{std::min(id,endpoint),std::max(id,endpoint)};
              const auto found=before.edges.find(edge);
              if(found!=before.edges.end() && found->second.length>longest){extraEdge=edge;longest=found->second.length;extraSplit=true;}
            }
            if(extraSplit) {
              extraPoint={maximum_node+2+i+node_offset,target.BalancedSplitPoint(nodes.at(extraEdge[0]).p,nodes.at(extraEdge[1]).p)};
              std::set<Id> neighbors;
              for(const auto& c:old.cells) {
                unsigned count=0;for(auto n:c.v)count+=n.id==extraEdge[0] || n.id==extraEdge[1];
                if(count==2)for(auto n:c.v)neighbors.insert(n.id);
              }
              std::vector<std::pair<Point,Tensor>> values;
              for(auto neighbor:neighbors) {
                const auto q=nodes.at(neighbor).p;const auto a=extraPoint.p;
                values.emplace_back(q,target.QueryMidpoint(a,q).target);
              }
              const auto mean=MetricMeanPoint(extraPoint.p,values);const auto a=extraPoint.p;
              offEdgePoint={extraPoint.id,{double(((long double)a.x+mean.x)/2),double(((long double)a.y+mean.y)/2),double(((long double)a.z+mean.z)/2)}};
            }
          }
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        std::vector<Patch> frames;std::vector<unsigned> extraFrames;
        if(strip)try {
          Timer timer(stats.reconstruction_seconds,options.timings);CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
          frames.push_back(old);extraFrames.push_back(0);
          if(extraSplit){frames.push_back(old);extraFrames.push_back(2);}
          if(boundary_edge) {
            Patch seed;
            for(const auto& c:old.cells) {
              unsigned count=0;for(auto n:c.v)count+=n.id==choice.edge[0] || n.id==choice.edge[1];
              if(count==2)seed.cells.push_back(c);
            }
            const auto skin=Incidence(seed.cells).Boundary();
            for(const auto& f:old.surface)if(std::any_of(skin.begin(),skin.end(),[&](Face face){return Key(face)==Key(f.v);}))seed.surface.push_back(f);
            const auto apex=std::find_if(movers.begin(),movers.end(),[&](Id id){
              return std::any_of(seed.surface.begin(),seed.surface.end(),[&](const auto& f){return std::find(f.v.begin(),f.v.end(),id)!=f.v.end();});
            });
            Patch fan;
            if(apex!=movers.end() && Reconnect(seed,*apex,maximum_cell+1+offset,geometry,fan,result.reason)) {
              auto frame=old;
              frame.cells.erase(std::remove_if(frame.cells.begin(),frame.cells.end(),[&](const auto& c){
                return std::any_of(seed.cells.begin(),seed.cells.end(),[&](const auto& d){return c.id==d.id;});
              }),frame.cells.end());
              frame.cells.insert(frame.cells.end(),fan.cells.begin(),fan.cells.end());
              // Only the complete replacement is metric-certified; an intermediate reconnection may worsen shape.
              if(frame.cells.size()<=MaximumCavityCells) {
                frames.push_back(frame);extraFrames.push_back(0);
                if(extraSplit){frames.push_back(std::move(frame));extraFrames.push_back(1);}
              }
            }
          }
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        const unsigned frameCount=world.maximum(strip?unsigned(frames.size()):0);
        const unsigned originalTrials=36*frameCount;
        const unsigned normalTrials=world.maximum(strip && !normalMoves.empty() ? 12u : 0u);
        // Preserve the established search before a bounded cap-depth escape using the shared refinement progress gate.
        for(unsigned trial=0;trial<originalTrials+normalTrials && world.maximum(strip && !approved);++trial) {
          const bool normalTrial=trial>=originalTrials;
          const unsigned frame=normalTrial?0:trial/36;
          const bool participating=strip && !approved && frame<frames.size() && (!normalTrial || !normalMoves.empty());
          const auto work=concurrent ? CPassiveComm::Allreduce(uint64_t(participating),CPassiveComm::Op::SUM,world.comm) : uint64_t(1);
          if(work>stats.work_limit-stats.global_work){stats.stop=EngineStop::WorkBudget;result.reason="Native3D joint strip exhausted the global work budget.";return finish();}
          stats.global_work+=work;
          if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
            stats.stop=EngineStop::TimeBudget;result.reason="Native3D joint strip exhausted the elapsed budget.";return finish();
          }
          bool candidate=false;
          if(participating)try {
            const unsigned slotTrial=normalTrial?12*((trial-originalTrials)/4)+(trial-originalTrials)%4:trial%36;
            Timer timer(stats.reconstruction_seconds,options.timings);CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
            ++stats.repair_proposals;auto moves=normalTrial?normalMoves:moveSeeds[(slotTrial%12)/4];const double alpha=std::array<double,4>{{1.,.5,.25,.125}}[slotTrial%4];
            for(auto& move:moves) {
              Point origin{};bool found=false;for(const auto& c:old.cells)for(auto n:c.v)if(n.id==move.first){origin=n.p;found=true;}
              if(!found)throw std::runtime_error("Native3D joint strip misses a moved identity.");
              const auto q=move.second.p;
              move.second.p={double(origin.x+alpha*((long double)q.x-origin.x)),double(origin.y+alpha*((long double)q.y-origin.y)),double(origin.z+alpha*((long double)q.z-origin.z))};
            }
            if(!normalTrial && trial>=36)moves.erase(std::remove_if(moves.begin(),moves.end(),[&](const auto& move){
              return std::none_of(old.surface.begin(),old.surface.end(),[&](const auto& f){return std::find(f.v.begin(),f.v.end(),move.first)!=f.v.end();});
            }),moves.end());
            point.p=splits[slotTrial/12];
            if(!boundary_edge) {
              Point a{},b{};for(const auto& c:old.cells)for(auto n:c.v) {
                if(n.id==choice.edge[0])a=n.p;if(n.id==choice.edge[1])b=n.p;
              }
              for(const auto& move:moves){if(move.first==choice.edge[0])a=move.second.p;if(move.first==choice.edge[1])b=move.second.p;}
              const long double t=std::array<long double,3>{{.5L,.25L,.75L}}[slotTrial/12];
              point.p={double((1-t)*a.x+t*b.x),double((1-t)*a.y+t*b.y),double((1-t)*a.z+t*b.z)};
            }
            // Preserve fixed identities: relabelling a stationary corner would manufacture changed-edge debt.
            moves.erase(std::remove_if(moves.begin(),moves.end(),[&](const auto& move){
              for(const auto& c:old.cells)for(auto n:c.v)if(n.id==move.first)
                return n.p.x==move.second.p.x && n.p.y==move.second.p.y && n.p.z==move.second.p.z;
              return false;
            }),moves.end());
            if(!moves.empty()) {
              candidate=SplitSurfaceStrip(frames[frame],choice.edge,point,moves,maximum_cell+1+offset,geometry,fresh,result.reason);
              if(candidate && extraFrames[frame]) {
                Patch star;
                for(const auto& c:fresh.cells) {
                  unsigned count=0;for(auto n:c.v)count+=n.id==extraEdge[0] || n.id==extraEdge[1];
                  if(count==2)star.cells.push_back(c);
                }
                const auto skin=Incidence(star.cells).Boundary();
                for(const auto& f:fresh.surface)if(std::any_of(skin.begin(),skin.end(),[&](Face face){return Key(face)==Key(f.v);}))star.surface.push_back(f);
                // Mapped mode: the strip's private bindings (including its new split point) must reach the sub-patch.
                star.parameters=fresh.parameters;
                Patch children;
                candidate=extraFrames[frame]==2 ?
                    InsertEdgeCavity(star,extraEdge,offEdgePoint,maximum_cell+1+offset,geometry,children,result.reason) :
                    SplitEdge(star,extraEdge,extraPoint,maximum_cell+1+offset,geometry,children,result.reason);
                if(candidate) {
                  fresh.cells.erase(std::remove_if(fresh.cells.begin(),fresh.cells.end(),[&](const auto& c){
                    return std::any_of(star.cells.begin(),star.cells.end(),[&](const auto& d){return c.id==d.id;});
                  }),fresh.cells.end());
                  fresh.cells.insert(fresh.cells.end(),children.cells.begin(),children.cells.end());
                  for(const auto& row:children.parameters)fresh.parameters.insert_or_assign(row.first,row.second);
                  if(fresh.cells.size()>MaximumReplacementCells)candidate=false;
                  else for(size_t i=0;i<fresh.cells.size();++i)fresh.cells[i].id=maximum_cell+1+offset+i;
                }
              }
              candidate=guarded_geometry(candidate);
            }
          }catch(const std::exception& e){failure.Set(2,0,e.what());}
          if(!(ready=vote(failure)))return finish();
          // A split may retain long changed edges only when their sorted length frontier strictly
          // descends; the metric certificate also handles every removed vertex.
          const bool passed=validate_private(candidate,MetricChange::Refine,
              !normalTrial && GetRefineAcceptance()==RefineAcceptance::StrictExcess);
          if(strip && !approved)approved=passed;
          if(!ready)return finish();
        }
        if(strip && approved){cavity_inserted=true;joint=false;}
      }
      auto movable_star=[&](const Imported& star,Id vertex) {
        std::set<Id> components;
        for(const auto& row:star.cells)for(unsigned f=0;f<4;++f)
          if(row.physical[f].present && row.cell.v[f].id!=vertex)components.insert(geometry.Canonical(row.physical[f].facet));
        return components.size()<=1;  // Smooth physical vertices share the same complete-star admission as interior points.
      };
      bool search=joint && composition && flat_edge && !choice.geometric && wallApices.empty() && !approved;
      std::vector<Id> apices;
      if(world.maximum(search)) {
        std::vector<Id> candidates;std::vector<Feature> features;
        if(search)try {
          std::set<Id> vertices;
          for(const auto& c:old.cells)if(std::count_if(c.v.begin(),c.v.end(),[&](Node n){return n.id==choice.edge[0] || n.id==choice.edge[1];})==2)
            for(auto n:c.v)if(n.id!=choice.edge[0] && n.id!=choice.edge[1])vertices.insert(n.id);
          candidates.assign(vertices.begin(),vertices.end());for(Id id:candidates)features.push_back({{{id,0,0}},1});
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        {Timer timer(stats.dependency_seconds,options.timings);
          auto metadata=directory->LookupBatch(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
          if(!metadata.valid){result.reason=metadata.reason;return finish();}
          auto payload=directory->FetchBatch(metadata,working,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
          if(!payload.valid){result.reason=payload.reason;return finish();}
          if(search)try {
            std::map<Id,size_t> costs;
            for(size_t i=0;i<candidates.size();++i) {
              const auto& star=payload.candidates.at(i);
              if(!star.valid) {
                if(star.reason=="Native 3D dependency star exceeds budget.")continue;
                throw std::runtime_error(star.reason);
              }
              ++stats.placement_classified_vertices;
              if(movable_star(star,candidates[i]))costs.emplace(candidates[i],star.cells.size());
            }
            apices=SplitPatchApices(old,choice.edge,&costs);
            if(costs.count(initial_placement_obstruction)) {
              auto admitted_cells=[&](const std::vector<Id>& movers) {
                std::set<Id> cells;
                for(const auto& c:old.cells)if(std::count_if(c.v.begin(),c.v.end(),[&](Node n){
                    return n.id==choice.edge[0] || n.id==choice.edge[1];})==2)cells.insert(c.id);
                for(size_t i=0;i<candidates.size();++i)if(std::find(movers.begin(),movers.end(),candidates[i])!=movers.end())
                  for(const auto& row:payload.candidates.at(i).cells)cells.insert(row.cell.id);
                return cells.size();
              };
              // Admit an actual blocking star when the all-cell cover cannot fit, without raising the cavity bound.
              const std::vector<Id> targeted{initial_placement_obstruction};
              if((apices.empty() || admitted_cells(apices)>MaximumCavityCells) && admitted_cells(targeted)<=MaximumCavityCells) {
                apices=targeted;++stats.placement_dependency_expansions;
              }
            }
            search=!apices.empty();
          }catch(const std::exception& e){failure.Set(2,0,e.what());}
        }
      }
      if(!(ready=vote(failure)))return finish();
      for(unsigned attempt=0;attempt<8 && world.maximum(search && !approved);++attempt) {
        placement_obstruction=UINT64_MAX;
        if(!place_patch(search,apices))return finish();
        bool expand=search && !approved && stats.global_work<stats.work_limit && apices.size()<8 &&
            placement_obstruction!=UINT64_MAX && placement_obstruction!=choice.edge[0] && placement_obstruction!=choice.edge[1] &&
            std::find(apices.begin(),apices.end(),placement_obstruction)==apices.end();
        if(!world.maximum(expand))break;
        {Timer timer(stats.dependency_seconds,options.timings);
          const std::vector<Feature> hints=expand?std::vector<Feature>{{{{placement_obstruction,0,0}},1}}:std::vector<Feature>{};
          auto metadata=directory->LookupBatch(hints,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
          if(!metadata.valid){result.reason=metadata.reason;return finish();}
          auto payload=directory->FetchBatch(metadata,working,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
          if(!payload.valid){result.reason=payload.reason;return finish();}
          if(expand)try {
            const auto& star=payload.candidates.at(0);
            if(!star.valid) {
              if(star.reason=="Native 3D dependency star exceeds budget.")expand=false;
              else throw std::runtime_error(star.reason);
            } else {
              ++stats.placement_classified_vertices;
              expand=movable_star(star,placement_obstruction);
            }
            if(expand){apices.push_back(placement_obstruction);++stats.placement_dependency_expansions;}
          }catch(const std::exception& e){failure.Set(2,0,e.what());}
        }
        if(!(ready=vote(failure)))return finish();
        search=expand;
      }
      joint=joint && boundary_edge;
      Tensor placement,nearPlacement;
      if(world.maximum(joint)) {
        std::vector<Point> query;
        if(joint)try {
          if(std::none_of(old.cells.begin(),old.cells.end(),[&](const Cell& cell){unsigned matches=0;for(auto n:cell.v)matches+=n.id==choice.edge[0] || n.id==choice.edge[1];return matches==2;}))
            throw std::runtime_error("Native3D joint patch lacks its complete split edge.");
          query.push_back(Centroid(old.cells.front()));if(!choice.geometric)query.push_back(point.p);
        }
        catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        DonorSamples proxy;
        {Timer timer(stats.donor_seconds,options.timings);++stats.donor_batches;
          proxy=source.SamplePoints(query,eligible_extensions,options.memory_ceiling,
              transfer_memory::Add(bytes(),directory->ResidentBytes(),private_scratch));}
        if(!proxy.valid){result.reason=proxy.reason;return finish();}
        if(joint)try {
          placement=composed_sample(query[0],proxy.samples.at(0).sensor);
          nearPlacement=choice.geometric ? placement : composed_sample(query[1],proxy.samples.at(1).sensor);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
      }
      for(unsigned growth=0;growth<12 && world.maximum(joint && !approved);++growth) {
        const auto work=concurrent ? CPassiveComm::Allreduce(uint64_t(joint && !approved),CPassiveComm::Op::SUM,world.comm) : uint64_t(1);
        if(work>stats.work_limit-stats.global_work) {
          stats.stop=EngineStop::WorkBudget;result.reason="Native3D joint repair exhausted the global work budget.";return finish();
        }
        stats.global_work+=work;
        if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
          stats.stop=EngineStop::TimeBudget;result.reason="Native3D joint repair exhausted the elapsed budget.";return finish();
        }
        bool candidate=false;Face obstruction{};Feature refit_obstruction,preferred_obstruction;
        const bool retained_frame=world.maximum(joint && wallSplit);
        const unsigned retained_frames=retained_frame?2:0;
        const unsigned frames=(world.maximum(joint && !choice.geometric)?2:1)+retained_frames;
        for(unsigned frame=0;frame<frames;++frame) {
          const bool participating=joint && !approved;candidate=false;
          {
            Timer timer(stats.reconstruction_seconds,options.timings);CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
            if(participating)try {
              ++stats.repair_proposals;
              const bool wall=std::any_of(old.surface.begin(),old.surface.end(),[&](const auto& f) {
                return options.wall_heights.count(geometry.Get(f.facet).marker);
              });
              stats.wall_reconstruction_trials+=wall;
              if(retained_frame && frame<retained_frames && wallSplit) {
                ++stats.wall_topology_trials;
                if(frame==0)candidate=guarded_geometry(RefitWallSplit(old,choice.edge,point,maximum_node+2+node_offset,
                    maximum_cell+1+offset,options.wall_heights,geometry,fresh,refit_obstruction,result.reason),"RefitWallSplit");
                else candidate=guarded_geometry(ReconstructWallSplit(old,choice.edge,point,maximum_node+2+node_offset,
                    maximum_cell+1+offset,options.wall_heights,geometry,fresh,obstruction,result.reason,nullptr,options.surface_tolerance),"ReconstructWallSplit");
              } else if(!retained_frame || frame>=retained_frames) {
                candidate=guarded_geometry(ReconstructSplitPatch(old,choice.edge,point,{maximum_node+2+node_offset,{}},
                    frame>retained_frames?nearPlacement:placement,maximum_cell+1+offset,geometry,
                    fresh,obstruction,result.reason,nullptr,options.wall_heights),"ReconstructSplitPatch");
              }
            }catch(const std::exception& e){failure.Set(2,0,e.what());}
            ready=vote(failure);
          }
          if(!ready)return finish();
          const bool compound_refit=retained_frame && frame==0;
          // SplitInsertion uses strict lexicographic descent of the changed-edge length frontier,
          // including refitted wall splits; it is not an unconditional size-progress exemption.
          const bool split_refit=compound_refit && !choice.geometric &&
              GetRefineAcceptance()==RefineAcceptance::SplitInsertion;
          auto validate_joint=[&](bool available){return split_refit ? validate_private(available,MetricChange::Refine,false,false,
              available ? reserved_relocations() : 0) :
              validate_private(available,compound_refit?MetricChange::Refine:
              choice.geometric?MetricChange::GeometryRefine:MetricChange::Refine,false,compound_refit && !choice.geometric);};
          bool passed=validate_joint(candidate);
          if(!ready)return finish();
          const bool missing_closure=participating && retained_frame && frame<retained_frames && wallSplit && !candidate &&
              (compound_refit?refit_obstruction.count>0:obstruction!=Face{});
          bool close_first=world.maximum(missing_closure);
          if(missing_closure)preferred_obstruction=compound_refit?refit_obstruction:Feature{obstruction,3};
          const bool repair=phase==Phase::Improvement && !close_first && participating && candidate && !passed && certified.failure==MetricFailure::Shape &&
              fresh.cells.size()<=MaximumReplacementCells && !options.wall_heights.empty() && stats.global_work<stats.work_limit;
          if(world.maximum(repair)) {
            const auto repair_limit=std::min<uint64_t>(256,(stats.work_limit-stats.global_work)/
                CPassiveComm::Allreduce(uint64_t(repair),CPassiveComm::Op::SUM,world.comm));
            Patch repair_seed;if(repair)repair_seed=fresh;
            uint64_t repair_used=0;bool retry=repair;
            for(unsigned strategy=0;strategy<2 && world.maximum(retry);++strategy) {
              const bool active=retry;if(strategy && active)fresh=repair_seed;
              // Movement needs the whole admitted domain, not donors for the previous proposal's queries.
              DonorPatch donors;
              { Timer timer(stats.donor_seconds,options.timings);
                ++stats.donor_batches;
                donors=source.ImportCells(active ? old.cells : std::vector<Cell>{},options.memory_ceiling,
                    transfer_memory::Add(bytes(),directory->ResidentBytes(),private_scratch));
              }
              CLocalFailure local;ConnectivityRepairReport report;bool repaired=false,missing_metric_star=false;
              if(!donors.valid && !DonorCap(donors.reason))local.Set(1,0,donors.reason);
              if(!(ready=vote(local)))return finish();
              if(active && donors.valid)try {
                const auto components=AssociatedExtensions(donors.cells,geometry);
                observe_transaction("support_donors",donors.valid,&donors);
                FrozenField target(std::move(donors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
                FieldAccounting accounting{stats,target};
                Patch proposal;std::string rejected;
                { Timer timer(stats.reconstruction_seconds,options.timings);CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
                  // Probe the whole actual-field interface before exhausting repair on a fixed support point.
                  RepairMetricConnectivity(fresh,target,geometry,options.wall_heights,
                      maximum_cell+1+offset,proposal,report,rejected,0);
                  missing_metric_star=report.blocked_interior_vertex!=UINT64_MAX;
                  if(missing_metric_star && !preferred_obstruction.count)
                    preferred_obstruction={{{report.blocked_interior_vertex,0,0}},1};
                  if(!missing_metric_star)repaired=RepairMetricConnectivity(fresh,target,geometry,options.wall_heights,
                      maximum_cell+1+offset,proposal,report,rejected,repair_limit-repair_used,strategy==0);
                }
                stats.wall_joint_trials+=report.trials;stats.wall_joint_moves+=report.moves;stats.wall_joint_removals+=report.removals;
                stats.wall_joint_flips+=report.flips;stats.wall_joint_vertex_moves+=report.vertex_moves;
                stats.wall_joint_coordinated+=report.coordinated;stats.wall_joint_solve_rejected+=report.solve_rejected;
                stats.wall_joint_move_geometry_rejected+=report.move_geometry_rejected;
                stats.wall_joint_move_height_rejected+=report.move_height_rejected;
                stats.wall_joint_move_loss_rejected+=report.move_loss_rejected;
                stats.metric_linearizations+=report.linearizations;stats.metric_probes+=report.metric_probes;
                stats.metric_linear_iterations+=report.linear_iterations;stats.metric_solve_trials+=report.solve_trials;
                if(repaired) {
                  std::map<Id,Point> previous;std::set<Id> used;
                  for(const auto& c:old.cells)for(auto n:c.v)previous.emplace(n.id,n.p);
                  for(const auto& c:proposal.cells)for(auto n:c.v)used.insert(n.id);
                  std::map<Id,Id> renamed;size_t cursor=0;const Id first_node=maximum_node+1+node_offset;
                  for(const auto& c:proposal.cells)for(auto n:c.v) {
                    const auto found=previous.find(n.id);
                    if(found==previous.end() || renamed.count(n.id))continue;
                    const auto p=found->second;
                    if(p.x==n.p.x && p.y==n.p.y && p.z==n.p.z)continue;
                    while(cursor<construction_nodes && used.count(first_node+cursor))++cursor;
                    if(cursor==construction_nodes)throw std::invalid_argument("Native3D joint movement exceeds reserved node identities.");
                    renamed.emplace(n.id,first_node+cursor);used.insert(first_node+cursor++);
                  }
                  // Fresh identities invalidate selection reuse and preserve transfer's unchanged-node contract.
                  for(auto& c:proposal.cells)for(auto& n:c.v)if(renamed.count(n.id))n.id=renamed.at(n.id);
                  for(auto& f:proposal.surface)for(auto& id:f.v)if(renamed.count(id))id=renamed.at(id);
                  fresh=std::move(proposal);repaired=guarded_geometry(true);
                }
              }catch(const std::exception& e){local.Set(2,0,e.what());}
              repair_used+=report.trials;
              stats.global_work+=CPassiveComm::Allreduce(uint64_t(report.trials),CPassiveComm::Op::SUM,world.comm);
              if(!(ready=vote(local)))return finish();
              close_first=close_first || world.maximum(missing_metric_star);
              if(world.maximum(repaired)) {
                const bool accepted=validate_joint(repaired);
                if(active)passed=accepted;
                stats.wall_joint_accepted+=repaired && accepted;
                if(!ready)return finish();
              }
              if(active && !passed && report.blocked_interior_vertex!=UINT64_MAX && !preferred_obstruction.count)
                preferred_obstruction={{{report.blocked_interior_vertex,0,0}},1};
              // Shape-only retirement may lose required size resolution; backtrack within the same trial allocation.
              retry=active && strategy==0 && repaired && !passed && report.complete_shape && report.removals &&
                  certified.failure==MetricFailure::Progress && repair_used<repair_limit;
            }
          }
          if(participating && !passed)try {
            if(candidate && certified.failure==MetricFailure::Shape) {
              const auto boundary=Incidence(old.cells).Boundary();std::set<Face> artificial;std::set<Id> fixed,physical;
              for(const auto& f:old.surface)physical.insert(f.v.begin(),f.v.end());
              for(auto f:boundary)if(std::none_of(old.surface.begin(),old.surface.end(),[&](const auto& p){return Key(p.v)==Key(f);})) {
                artificial.insert(Key(f));fixed.insert(f.begin(),f.end());
              }
              for(const auto& c:fresh.cells)if(c.id==certified.weakest_cell || c.id==certified.sliver_cell) {
                const auto faces=Faces(c);
                const auto found=std::find_if(faces.begin(),faces.end(),[&](Face f){return artificial.count(Key(f));});
                if(found!=faces.end()){obstruction=Key(*found);if(!preferred_obstruction.count)preferred_obstruction={obstruction,3};break;}
                for(auto n:c.v)if(fixed.count(n.id) && !physical.count(n.id) && !preferred_obstruction.count)
                  preferred_obstruction={{{n.id,0,0}},1};
              }
            }
            // Preserve the first mechanism's actual blockage; a later fallback must not overwrite its dependency.
            if(!preferred_obstruction.count && obstruction!=Face{})preferred_obstruction={obstruction,3};
          }catch(const std::exception& e){failure.Set(2,0,e.what());}
          if(!(ready=vote(failure)))return finish();
          if(participating) {
            stats.wall_topology_accepted+=passed && retained_frame && frame<retained_frames && wallSplit;
            approved=passed;
            if(passed && std::any_of(old.surface.begin(),old.surface.end(),[&](const auto& f) {
              return options.wall_heights.count(geometry.Get(f.facet).marker);
            }))++stats.wall_reconstruction_accepted;
          }
          if(!ready)return finish();
          // Resolve a required retained-row interface before spending trials on incomplete fallback fans.
          if(close_first || !world.maximum(joint && !approved))break;
        }
        if(joint && approved){cavity_inserted=true;joint=false;}
        else if(joint && candidate && certified.failure==MetricFailure::Measurement)joint=false;
        if(!world.maximum(joint && !approved))break;
        if(!preferred_obstruction.count && std::adjacent_find(obstruction.begin(),obstruction.end())==obstruction.end())preferred_obstruction={obstruction,3};
        const bool grow=joint && preferred_obstruction.count;
        const auto added_dependencies=directory->LookupBatch(grow ? std::vector<Feature>{preferred_obstruction} : std::vector<Feature>{},
            options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
        if(!added_dependencies.valid){result.reason=added_dependencies.reason;return finish();}
        Dependencies neighbor;neighbor.valid=true;
        if(grow) {
          neighbor=added_dependencies.candidates.at(0);
          if(!neighbor.valid) {
            if(neighbor.reason=="Native 3D dependency star exceeds budget.")joint=false;
            else failure.Set(1,0,neighbor.reason);
          }
        } else joint=false;
        if(!(ready=vote(failure)))return finish();
        if(!world.maximum(joint))break;
        if(!joint)neighbor.cells.clear();
        neighbor.valid=true;
        const auto more=directory->Fetch(neighbor,working,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
        if(!more.valid){result.reason=more.reason;return finish();}
        if(joint)try {
          auto joined=dependencies.cells;auto payload=imported.cells;
          for(const auto& dep:neighbor.cells) {
            const auto found=std::find_if(joined.begin(),joined.end(),[&](const auto& d){return d.cell==dep.cell;});
            if(found!=joined.end()) {
              if(found->version!=dep.version || found->owner!=dep.owner)throw std::runtime_error("Native3D joint dependency changed ownership/version.");
            } else joined.push_back(dep);
          }
          if(joined.size()==dependencies.cells.size() || joined.size()>MaximumCavityCells)joint=false;
          if(joint) {
            for(const auto& row:more.cells)if(std::none_of(payload.begin(),payload.end(),[&](const auto& c){return c.cell.id==row.cell.id;}))payload.push_back(row);
            dependencies.cells.swap(joined);imported.cells.swap(payload);old=GeometryPatch(imported, &geometry);reuse.clear();
            for(const auto& c:old.cells)selection.Cached(c,reuse);
          }
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        if(!world.maximum(joint))break;
        const bool reserved=reserve_dependencies(imported,joint,result.reason,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch),choice.reservation_priority);
        CLocalFailure reservation_failure;
        if(joint && !reserved) {
          if(directory->PassDeferred()) {++stats.conflicts;joint=false;approved=false;}
          else if(concurrent && result.reason=="Native 3D conflicting dependency reservation.")++stats.conflicts;
          else reservation_failure.Set(1,0,"Native3D joint patch lost complete dependency reservation.");
        }
        if(!(ready=vote(reservation_failure)))return finish();
        if(!world.maximum(joint))break;
        try {observe_transaction("growth_reserved",reserved);}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(capture && !vote(failure))return finish();
        const bool measured=measure_private(joint ? old.cells : empty,before,false,reuse);
        if(!ready)return finish();
        CLocalFailure old_invalid;if(joint && !measured)old_invalid.Set(1,0,result.reason);
        if(!(ready=vote(old_invalid)))return finish();
      }
      const bool fallback=metric_needed && old_measured && repairing;
      if (fallback && !repairing) ++stats.repair_fallbacks;
      const bool move=repairing ? choice.feature.count==1 : coarsening;
      const Id vertex=repairing ? choice.feature.vertices[0] : choice.edge[1];
      std::map<Id,Node> nodes;
      { Timer timer(stats.reconstruction_seconds,options.timings);
        try { if (fallback) for (const auto& cell:old.cells) for (auto node:cell.v) nodes.emplace(node.id,node); }
        catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        ready=vote(failure);
      }
      if (!ready) return finish();
      Point moveCenter{};
      if(composition && world.maximum(fallback && move && !approved)) {
        const bool participating=fallback && move && !approved;
        std::set<Id> neighbors;std::map<Id,Tensor> samples;Point origin{};
        try {if(participating) {
          origin=nodes.at(vertex).p;
          for(const auto& f:old.surface)if(std::find(f.v.begin(),f.v.end(),vertex)!=f.v.end())neighbors.insert(f.v.begin(),f.v.end());
          if(neighbors.empty())for(const auto& n:nodes)neighbors.insert(n.first);
          neighbors.erase(vertex);
        }}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        std::vector<Id> query_ids;
        try {query_ids.assign(neighbors.begin(),neighbors.end());}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        const auto count=CPassiveComm::Allreduce(query_ids.size(),CPassiveComm::Op::MAX,world.comm);
        for(size_t at=0;at<count;at+=MaximumCavityCells) {
          std::vector<std::array<Point,2>> queries;
          try {for(size_t i=at;i<std::min(at+MaximumCavityCells,query_ids.size());++i)queries.push_back({origin,nodes.at(query_ids[i]).p});}
          catch(const std::exception& e){failure.Set(2,0,e.what());}
          if(!(ready=vote(failure)))return finish();
          DonorSamples answer;
          {Timer timer(stats.donor_seconds,options.timings);++stats.donor_batches;
            answer=source.SampleMidpoints(queries,eligible_extensions,options.memory_ceiling,
                transfer_memory::Add(bytes(),directory->ResidentBytes(),private_scratch));}
          if(!answer.valid){result.reason=answer.reason;return finish();}
          try {for(size_t i=0;i<queries.size();++i) {
            const auto a=queries[i][0],b=queries[i][1];
            const Point p{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
            samples.emplace(query_ids[at+i],composed_sample(p,answer.samples.at(i).sensor));
          }}catch(const std::exception& e){failure.Set(2,0,e.what());}
          if(!(ready=vote(failure)))return finish();
        }
        try {if(participating && !neighbors.empty()) {
          std::vector<std::pair<Point,Tensor>> values;for(auto id:neighbors)values.emplace_back(nodes.at(id).p,samples.at(id));
          moveCenter=MetricMeanPoint(origin,values,coordinate_only ? 4*old.cells.size() : 4*MaximumCavityCells);
        }}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
      }
      for (unsigned trial=0;trial<19;++trial) {
        if (!world.maximum(fallback && !approved)) break;
        const bool move_trial=trial<3;
        const bool participating=fallback && !approved && (!coordinate_only || move_trial) &&
            (move_trial ? move : (!move || (repairing && choice.feature.count==1) || (!repairing && coarsening))) &&
            (move_trial || trial-3<nodes.size());
        if (!world.maximum(participating)) continue;
        bool candidate=false;
        { Timer timer(stats.reconstruction_seconds,options.timings);
          if (participating) try {
            CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
            ++stats.repair_proposals;
            const Id first=maximum_cell+1+offset;
            if (move_trial && maximum_node<=UINT64_MAX-node_stride && node_offset<=UINT64_MAX-maximum_node-node_stride) {
              std::set<Id> neighbors;
              for (auto face:old.surface) if (std::find(face.v.begin(),face.v.end(),vertex)!=face.v.end())
                neighbors.insert(face.v.begin(),face.v.end());
              if (neighbors.empty()) for (const auto& row:nodes) neighbors.insert(row.first);
              neighbors.erase(vertex);
              long double x=0,y=0,z=0;
              for (Id id:neighbors) { auto p=nodes.at(id).p; x+=p.x; y+=p.y; z+=p.z; }
              if (!neighbors.empty()) {
                const auto p=nodes.at(vertex).p; const long double step=trial==0 ? 1.L : (trial==1 ? .5L : .25L);
                point={maximum_node+1+node_offset,{double(p.x+step*(x/neighbors.size()-p.x)),
                    double(p.y+step*(y/neighbors.size()-p.y)),double(p.z+step*(z/neighbors.size()-p.z))}};
                if(composition) {
                  point.p={double(p.x+step*(moveCenter.x-p.x)),double(p.y+step*(moveCenter.y-p.y)),double(p.z+step*(moveCenter.z-p.z))};
                  point=ConstrainMovePoint(old,vertex,point,options.wall_heights,geometry);
                }
                candidate=guarded_geometry(MoveVertex(old,vertex,point,first,geometry,fresh,result.reason,nullptr,
                    coordinate_only ? old.cells.size() : MaximumCavityCells),"MoveVertex");
              }
            } else if (!move_trial) {
              auto apex=nodes.begin(); std::advance(apex,trial-3);
              if(move && repairing && choice.feature.count==1)
                candidate=guarded_geometry(CollapseVertex(old,vertex,apex->first,first,geometry,fresh,result.reason),"CollapseVertex");
              else candidate=guarded_geometry(Reconnect(old,apex->first,first,geometry,fresh,result.reason),"Reconnect");
            }
          } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
          ready=vote(failure);
        }
        if (!ready) return finish();
        const bool valid=validate_private(candidate,MetricChange::Repair);
        if (!ready) return finish();
        if (valid) { approved=true; repair_approved=true; moved=move_trial; }
      }
      if(repairing && world.maximum(!coordinate_only && repair_work<160 && chosen && !approved)) {
        reserve_optimization(!coordinate_only && repair_work<160 && chosen && !approved);
        DonorPatch donors;
        {Timer timer(stats.donor_seconds,options.timings);++stats.donor_batches;
          donors=source.ImportCells(!coordinate_only && repair_work<repair_budget && !approved?old.cells:empty,options.memory_ceiling,
              transfer_memory::Add(bytes(),directory->ResidentBytes(),private_scratch));}
        bool candidate=false;
        if(!donors.valid && !DonorCap(donors.reason)){result.reason=donors.reason;return finish();}
        if(!coordinate_only && repair_work<repair_budget && !approved && donors.valid)try {
          const auto components=AssociatedExtensions(donors.cells,geometry);
          observe_transaction("support_donors",donors.valid,&donors);
          FrozenField proposal_field(std::move(donors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
          FieldAccounting accounting{stats,proposal_field};
          Timer timer(stats.reconstruction_seconds,options.timings);CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
          candidate=guarded_geometry(optimize_patch(old,RegionMetric(proposal_field),false));
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        release_optimization();
        if(world.maximum(candidate)) {
          const bool placed=validate_private(candidate,MetricChange::Repair);
          if(!ready)return finish();
          if(placed){approved=repair_approved=true;moved=false;optimization_approved=true;++stats.optimization_validated;}
        }
      }
      if (repair_approved && !repairing) ++stats.fallback_accepted;
    }
    try {observe_transaction("construction_end",approved);}catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(options.transaction_observer && !vote(failure))return finish();
    // Successful peer contact/reservation votes must not erase this worker's construction refusal.
    const std::string pending_rejection=chosen && !approved ? result.reason : std::string{};
    const bool surface_edit=approved && !SameSkin(old,fresh);
    if((curved && world.maximum(surface_edit)) || world.maximum(coordinate_only && approved)) {
      const bool prior=approved;bool deferred=false;
      if(!directory->Clearance(working,old,fresh,approved,result.reason,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch),&deferred,choice.reservation_priority,
          coordinate_only ? imported.coordinate_star : Feature{}))return finish();
      if(deferred){chosen=false;++stats.conflicts;++stats.rejected;++stats.proposal_deferred;}
      else if(prior && !approved)geometry_trial(false);
    }
    if(concurrent && world.maximum(approved)) {
      Timer timer(stats.reservation_seconds,options.timings);CLocalFailure final_failure;
      // Every approved proposal claims its final old footprint together, including completed peers.
      if(approved)try {
        if(old.cells.size()!=imported.cells.size() || dependencies.cells.size()!=imported.cells.size())
          throw std::runtime_error("Native3D final proposal lacks complete imported dependencies.");
        std::set<Id> cells;for(const auto& c:old.cells)if(!cells.insert(c.id).second)
          throw std::runtime_error("Native3D final proposal repeats an old cell.");
        for(const auto& d:dependencies.cells) {
          const auto row=std::find_if(imported.cells.begin(),imported.cells.end(),[&](const auto& c){return c.cell.id==d.cell;});
          if(!cells.erase(d.cell) || row==imported.cells.end() || row->version!=d.version)
            throw std::runtime_error("Native3D final proposal lost a versioned old cell.");
        }
      }catch(const std::exception& e){final_failure.Set(2,0,e.what());}
      if(!vote(final_failure))return finish();
      ++stats.final_reservations;
      const bool admitted=reserve_dependencies(imported,approved,result.reason,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch),choice.reservation_priority);
      if(approved && !admitted) {
        if(directory->PassDeferred() || result.reason=="Native 3D conflicting dependency reservation.") {
          approved=false;chosen=false;++stats.conflicts;++stats.rejected;++stats.proposal_deferred;
        }else final_failure.Set(1,0,"Native3D final proposal reservation failed: "+result.reason);
      }
      if(!vote(final_failure))return finish();
      const auto commits=CPassiveComm::Allreduce(uint64_t(approved),CPassiveComm::Op::SUM,world.comm);
      stats.maximum_parallel_commits=std::max(stats.maximum_parallel_commits,commits);
    }
    if(chosen && !approved && !pending_rejection.empty())result.reason=pending_rejection;
    std::vector<BoundaryParameterEdit> parameter_edits;
    if(approved && geometry.Mapped())try {
      geometry.ExportParameters(old);geometry.ExportParameters(fresh);
      parameter_edits=BoundaryParameterState::Changes(old,fresh,geometry.Parameters()->Revision());
    }catch(const std::exception& e){approved=false;result.reason=e.what();}
    if(approved && !coarsening && !repairing && !repair_approved)
      for(const auto& cell:fresh.cells)for(auto node:cell.v)if(node.id==point.id)point=node;
    try {observe_transaction("transaction_end",approved);}catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(options.transaction_observer && !vote(failure))return finish();
    if (chosen && !approved) {
      // Keep the completed rejection consistent with the earlier private/budget-exit sample.
      stats.last_metric_rejection=certified;
      stats.last_rejected_feature=repairing ? choice.feature : Feature{{choice.edge[0],choice.edge[1],0},2};
      if (!old.cells.empty()) stats.last_rejected_cavity_centroid=Centroid(old.cells.front());
    }
    if (chosen && !approved) last_rejection = (repairing ? "feature " + std::to_string(choice.feature.count) + ":" +
        std::to_string(choice.feature.vertices[0]) : "edge " + std::to_string(choice.edge[0]) + "," + std::to_string(choice.edge[1])) + ": " + result.reason;
    std::vector<std::vector<Dependency>> removals(world.size);
    std::vector<std::vector<OwnedCell>> additions(world.size);
    std::vector<std::vector<BlockedEdge>> blocked(world.size);
    std::vector<Dependency> removed;
    std::vector<OwnedCell> added;
    std::vector<BlockedEdge> failures;
    {
      Timer timer(stats.routing_seconds,options.timings);
      if (chosen && !approved) ++stats.rejected;
      try {
        if (approved) {
          std::vector<Id> parents;
          if(coordinate_only) {
            if(fresh.cells.size()!=old.cells.size())throw std::logic_error("Native coordinate-star movement changed cell count.");
            for(const auto& cell:fresh.cells)parents.push_back(cell.id);
          } else if (repair_approved || cavity_inserted) {
            for (size_t i=0;i<fresh.cells.size();++i) parents.push_back(old.cells[i%old.cells.size()].id);
          } else for (const auto& cell:old.cells) {
            bool keep=false;
            for (auto node:cell.v) keep=keep || node.id==choice.edge[0];
            if (!coarsening || !keep) parents.push_back(cell.id);
            if (!coarsening) parents.push_back(cell.id);
          }
          if (fresh.cells.size()!=parents.size()) throw std::logic_error("Unexpected native tetrahedral edit layout.");
          std::map<Id,int> owners;std::map<Id,uint64_t> versions;
          for (const auto& row : dependencies.cells) {
            owners.emplace(row.cell,row.owner);versions.emplace(row.cell,row.version);removals[row.owner].push_back(row);
          }
          std::map<Face,Id> physical;
          for (auto face : fresh.surface) physical.emplace(Key(face.v),face.facet);
          for (size_t i=0; i<fresh.cells.size(); ++i) {
            OwnedCell row; row.cell=fresh.cells[i];
            if(coordinate_only) {
              const auto version=versions.at(row.cell.id);
              if(version==UINT64_MAX)throw std::overflow_error("Native coordinate-star cell version overflows.");
              row.version=version+1;
            }
            const auto faces=Faces(row.cell);
            for (size_t f=0; f<4; ++f) {
              const auto found=physical.find(Key(faces[f]));
              if (found!=physical.end()) row.physical[f]={true,found->second};
            }
            additions[owners.at(parents[i])].push_back(row);
          }
        } else if (chosen && !pass_deferred) {
          std::set<int> owners;
          for (const auto& row : dependencies.cells) owners.insert(row.owner);
          for (int owner : owners) blocked[owner].push_back({choice.edge,choice.feature});
        }
      } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      ready=vote(failure);
      if (ready) {
        bool admitted=false;
        const auto baseline=transfer_memory::Add(bytes(),source.ResidentBytes(),directory->ResidentBytes(),
            transfer_memory::Bytes(imported.cells),MaximumFrozenFieldBytes);
        removed=world.exchange(removals,&admitted,options.memory_ceiling,baseline);
        ready=admitted;
        if (ready) { added=world.exchange(additions,&admitted,options.memory_ceiling,baseline); ready=admitted; }
        if (ready) { failures=world.exchange(blocked,&admitted,options.memory_ceiling,baseline); ready=admitted; }
        if (!ready) result.reason="Native 3D publication routing exceeds storage budget.";
      }
    }
    if (!ready) return finish();
    {
      Timer timer(stats.publication_seconds,options.timings);
      ready=directory->Publish(working,removed,added,true,result.reason,options.memory_ceiling,
          transfer_memory::Add(real_bytes(),source.ResidentBytes(),MaximumFrozenFieldBytes),parameter_edits,approved?fresh.coverage:no_coverage,geometry.AtlasReservation());
    }
    if (!ready) return finish();
    if(!removed.empty() || !added.empty())selection.progress_consistent=false;
    // A completed attempt is served once on every replica. Conflict/deferred candidates never
    // enter this receipt stream and retain their waiting age.
    stats.length_pass_deferred+=pass_deferred&&!approved;
    if(!service_frontier(repairing ? choice.feature : Feature{{choice.edge[0],choice.edge[1],0},2},chosen||pass_deferred,approved))return finish();
    std::vector<Id> changed;
    {
      Timer timer(stats.update_seconds,options.timings);
      ready=admit(transfer_memory::Add(
          checkpoint.valid ? transfer_memory::Mul(transfer_memory::Mul(4,transfer_memory::Add(removed.size(),added.size())),sizeof(Id)+4*sizeof(void*)) : 0,
          transfer_memory::Mul(working.size(),selection_bound(selection.track_repairs)),
          transfer_memory::Mul(transfer_memory::Mul(6,removed.size()),sizeof(std::pair<const Edge,EdgeMeasure>)+4*sizeof(void*)),
          selection.track_repairs ? transfer_memory::Mul(transfer_memory::Mul(14,removed.size()),sizeof(RepairFeature)+4*sizeof(void*)) : 0,
          transfer_memory::Mul(failures.size(),2*(sizeof(std::pair<const Id,std::set<Id>>)+sizeof(Id)+8*sizeof(void*)))));
      if (ready) try {
        if(checkpoint.valid) {
          for(const auto& row:removed) {
            const auto found=selection.cells.find(row.cell);
            if(found==selection.cells.end())throw std::logic_error("Checkpoint trial lost a changed owned support row.");
            checkpoint.changed.insert(found->second.vertices.begin(),found->second.vertices.end());
          }
          for(const auto& row:added)for(auto node:row.cell.v)checkpoint.changed.insert(node.id);
        }
        // Retain retired scalars until all children are measured against this event's immutable source.
        for (const auto& row : removed) {
          const auto cached=selection.cells.find(row.cell);
          if(cached==selection.cells.end()){++stats.unmeasured_retired;continue;}
          for (auto key : cached->second.edges)retired_edges.emplace(key,selection.edges.at(key).measure);
          selection.Drop(row.cell);
        }
        selection.progress_consistent=true;
        for (const auto& row : added) changed.push_back(row.cell.id);
        for (auto row : failures) selection.Block(row.edge,row.feature,repairing);
        if (approved) {
          if(standalone_coarsening && checkpoint.valid)checkpoint.trial=true;
          improvement_changed=improvement_changed || repairing;
          construction_changed=construction_changed || !repairing;
          construction_generation_changed=construction_generation_changed || !repairing;
          ++stats.accepted;
          if(!repairing){if(coarsening)++stats.length_collapses;else ++stats.length_splits;}
          if(optimization_approved) {
            stats.optimization_moves+=repair_optimization.moves;
            stats.optimization_insertions+=repair_optimization.insertions;
            stats.optimization_collapses+=repair_optimization.collapses;
            stats.optimization_swaps+=repair_optimization.swaps;
          }
          stats.geometry_progress+=geometric_progress;
          stats.size_progress+=certified.size_progress;
          stats.quality_progress+=certified.quality_progress;
          stats.cell_removals+=coarsening && !repair_approved && certified.cell_removal;
          if (cavity_inserted) ++stats.inserted;
          else if (repair_approved) { if (moved) ++stats.moved; else ++stats.reconnected; }
          else if (coarsening) result.collapses.push_back(choice.edge); else result.splits.push_back({choice.edge,point});
        }
        stats.maximum_owned=std::max(stats.maximum_owned,working.size());
      } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      if (ready) ready=vote(failure);
      if (ready) ready=measure(changed,false,&stats.update_seconds);
      if (ready) {
        try {
          selection.FinishRepairs();
          for (Id vertex:directory->ChangedVertices()) selection.Touch(vertex);
          // Retire known conflicts in this pass without another global reservation per edge.
          // Unmeasured/remote support and expanded cavities remain guarded by Directory claims.
          if(bidirectional && phase==Phase::Construction)
            for(Id vertex:directory->ChangedVertices())stats.length_pass_deferred+=selection.ServiceClaimedVertex(vertex);
        }
        catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        ready=vote(failure);
      }
      retired_edges.clear();
    }
    if (!ready) return finish();
    }
    batch_bytes=0;++round;
  }
  {
    Timer timer(stats.completion_seconds,options.timings);
    const int incomplete=world.maximum(selection.cells.size()!=working.size() ? 3 :
        (selection.invalid ? 2 : (coarsening && !selection.queue.empty() ? 1 : 0)));
    result.valid=phase==Phase::Improvement && incomplete==0 && !world.maximum(selection.unmeasured_shape!=0 || selection.quality_invalid!=0 ||
        selection.construction_invalid!=0 || (standalone_coarsening && improvement_changed));
    stats.stop=result.valid ? EngineStop::Complete : EngineStop::RoundBudget;
    if (incomplete==1)
      result.reason="Native 3D coarsening round limit reached with valid cached metric cells and pending collapse candidates.";
    else if (incomplete==2)
      result.reason="Native 3D adaptation round limit reached before cached metric size/shape gates were satisfied.";
    else if (incomplete==3)
      result.reason="Native 3D adaptation round limit reached with incomplete metric-cell measurements.";
    else if(!result.valid)result.reason="Native 3D adaptation round limit reached before strict quality/coarsening completion.";
    else result.reason.clear();
  }
  return finish();
}
}  // namespace
bool CollapseConstraintViolation(const OwnedCell& cell,Id removed,Node retained,const PlanarReference& geometry,
                                double tolerance,const WallHeights& heights) {
  if(heights.empty() && geometry.ExtensionComponents().empty())return false;
  if(std::none_of(cell.physical.begin(),cell.physical.end(),[](FacetBinding f){return f.present;}))return false;
  if(std::any_of(cell.cell.v.begin(),cell.cell.v.end(),[&](Node n){return n.id==retained.id;}))return false;
  auto proposed=cell;bool changes=false;
  for(auto& n:proposed.cell.v)if(n.id==removed){n=retained;changes=true;}
  if(!changes)return false;
  if(geometry.Mapped()&&(geometry.Mobility()||!geometry.Parameters()->Coverage().faces.empty()||!geometry.Parameters()->Coverage().traces.empty())) {
    bool changed_surface=false;
    for(unsigned k=0;k<4;++k)changed_surface=changed_surface||(cell.physical[k].present&&cell.cell.v[k].id!=removed);
    // A single-cell filter cannot remap a complete physical star; the full operator must certify its atlas.
    if(changed_surface||!geometry.AtlasReservation())return false;
  }
  const auto patch=GeometryPatch(Imported{{proposed},true,{}}, &geometry);
  for(const auto& face:patch.surface) {
    std::array<Point,3> points;
    for(unsigned i=0;i<3;++i)for(auto n:proposed.cell.v)if(n.id==face.v[i])points[i]=n.p;
    if(!geometry.ContainsFace(face.facet,points))return true;
    if(geometry.Declared(geometry.Canonical(face.facet)) &&
        geometry.SurfaceError(face.facet,points)>tolerance)return true;
  }
  FirstHeightReport report;std::string reason;
  if(heights.empty() || CheckFirstHeight(patch,heights,geometry,report,reason))return false;
  return GetShapeAdmission()!=ShapeAdmission::ValidityOnly || !report.measured || report.missing_cells ||
         report.reference_clearance_failures;
}
EngineResult AdaptMetric(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition, EngineOptions options) {
  return Adapt(world,accepted,original,geometry,std::move(composition),options,MetricChange::Refine,true);
}
EngineResult Refine(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition, EngineOptions options) {
  return Adapt(world,accepted,original,geometry,std::move(composition),options,MetricChange::Refine);
}
EngineResult Coarsen(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition, EngineOptions options) {
  return Adapt(world,accepted,original,geometry,std::move(composition),options,MetricChange::Coarsen);
}
EngineResult Repair(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition, EngineOptions options) {
  return Adapt(world,accepted,original,geometry,std::move(composition),options,MetricChange::Repair);
}
// ---- DC-12 (OPTIMIZE_PLAN sect. 4/6): mesh-wide metric optimization and the serial build-then-optimize driver ----------
namespace {
struct CoordinateHash {
  size_t operator()(const std::array<double,3>& k) const {
    uint64_t h=1469598103934665603ULL;
    for(const double v:k){uint64_t b=0;std::memcpy(&b,&v,sizeof b);h=(h^b)*1099511628211ULL;}
    return size_t(h);
  }
};
std::array<Face,4> OppositeFaces(const Cell& c) {
  const auto& v=c.v;return {{{v[1].id,v[2].id,v[3].id},{v[0].id,v[3].id,v[2].id},{v[0].id,v[1].id,v[3].id},{v[0].id,v[2].id,v[1].id}}};
}
bool CellHas(const Cell& c,Id v){for(const auto& n:c.v)if(n.id==v)return true;return false;}
struct CellQuality { double q=-1,J=-1;double Score()const{return std::min(q/MinimumMeanRatio,J/MinimumScaledJacobian);}
  bool Valid()const{return std::isfinite(q)&&std::isfinite(J)&&q>=0&&J>=0;} };
// ---- DC-13 placement engine (DC13_PLAN sect. 1.1): proposals only, every acceptance re-measures exactly ----------
// Upper Cholesky factor R of an SPD tensor (M = R^T R); ok=false if not SPD in double.
struct Factor3 { double r00=0,r01=0,r02=0,r11=0,r12=0,r22=0;bool ok=false; };
Factor3 FactorOf(const Tensor& m) {
  Factor3 c;if(!(m.xx>0))return c;c.r00=std::sqrt(m.xx);c.r01=m.xy/c.r00;c.r02=m.xz/c.r00;
  const double p11=m.yy-c.r01*c.r01;if(!(p11>0))return c;c.r11=std::sqrt(p11);c.r12=(m.yz-c.r01*c.r02)/c.r11;
  const double p22=m.zz-c.r02*c.r02-c.r12*c.r12;if(!(p22>0))return c;c.r22=std::sqrt(p22);c.ok=true;return c;
}
// Approximate score min(q/.20, J/.05) of (p0..p3) in a constant tensor (same definitions as Measure, double precision);
// -INFINITY for a non-positive (determinant <= 0) or degenerate cell.
double FastScore(const std::array<Point,4>& p,const Factor3& f) {
  double y[4][3];
  for(unsigned k=0;k<4;++k){const double d0=p[k].x-p[0].x,d1=p[k].y-p[0].y,d2=p[k].z-p[0].z;
    y[k][0]=f.r00*d0+f.r01*d1+f.r02*d2;y[k][1]=f.r11*d1+f.r12*d2;y[k][2]=f.r22*d2;}
  auto e=[&](unsigned i,unsigned j,unsigned c){return y[j][c]-y[i][c];};
  const double u[3]={e(0,1,0),e(0,1,1),e(0,1,2)},v[3]={e(0,2,0),e(0,2,1),e(0,2,2)},w[3]={e(0,3,0),e(0,3,1),e(0,3,2)};
  const double det=u[0]*(v[1]*w[2]-v[2]*w[1])-u[1]*(v[0]*w[2]-v[2]*w[0])+u[2]*(v[0]*w[1]-v[1]*w[0]);
  if(!(det>0)||!std::isfinite(det))return -INFINITY;
  double l2[4][4]={{0}},sum=0;
  for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){double s=0;for(unsigned c=0;c<3;++c){const double d=e(i,j,c);s+=d*d;}l2[i][j]=l2[j][i]=s;sum+=s;}
  const double root=std::cbrt(det/2),q=12*root*root/sum;double J=1;
  for(unsigned k=0;k<4;++k){double den=1;for(unsigned j=0;j<4;++j)if(j!=k)den*=std::sqrt(l2[k][j]);J=std::min(J,den>0?std::sqrt(2.)*det/den:0.);}
  return std::min(q/MinimumMeanRatio,J/MinimumScaledJacobian);
}
// Boundary faces of a cell set (faces used once inside the set), oriented as in their cell (opposite vertex inside).
std::vector<Face> CavityBoundary(const std::vector<Cell>& cells) {
  std::map<Face,std::pair<int,Face>> uses;
  for(const auto& c:cells)for(auto f:OppositeFaces(c)){auto k=f;std::sort(k.begin(),k.end());auto& u=uses[k];++u.first;u.second=f;}
  std::vector<Face> out;for(const auto& u:uses)if(u.second.first==1)out.push_back(u.second.second);
  return out;
}
// Min-max placement of one new point x in a star-shaped cavity bounded by `faces` (cone cells {x, f}). The tensor of a
// cone cell is the arithmetic mean of the point tensors at its three face vertices and at the start point (an SPD
// proposal metric); deterministic best-improvement compass search in the metric frame of the start point (6 axes and
// 8 diagonals, step .25 halving to 1e-3). `admissible` restricts the search region (e.g. the size-split region).
struct Placement { bool ok=false;Point x;double score=-INFINITY;size_t evaluations=0; };
Placement PlaceCone(const std::vector<Face>& faces,const std::function<Point(Id)>& point,const std::function<Tensor(Point)>& at,
                    Point start,const std::function<bool(Point)>& admissible,size_t budget=160) {
  Placement out;if(faces.empty())return out;
  // a start outside the target's coverage (e.g. a smoothing proposal beyond the domain) is no proposal, not an error
  Tensor t0;std::map<Id,Tensor> vt;
  try{t0=at(start);for(const auto& f:faces)for(Id v:f)if(!vt.count(v))vt.emplace(v,at(point(v)));}catch(const std::exception&){return out;}
  std::vector<std::array<Point,3>> base;std::vector<Factor3> factor;
  for(const auto& f:faces){Tensor m=t0;for(Id v:f){const auto& t=vt.at(v);m.xx+=t.xx;m.xy+=t.xy;m.xz+=t.xz;m.yy+=t.yy;m.yz+=t.yz;m.zz+=t.zz;}
    m.xx/=4;m.xy/=4;m.xz/=4;m.yy/=4;m.yz/=4;m.zz/=4;const auto c=FactorOf(m);if(!c.ok)return out;factor.push_back(c);
    base.push_back({point(f[0]),point(f[1]),point(f[2])});}
  auto F=[&](Point x)->double{++out.evaluations;if(admissible&&!admissible(x))return -INFINITY;double s=INFINITY;
    for(size_t k=0;k<base.size()&&s>-INFINITY;++k)s=std::min(s,FastScore({x,base[k][0],base[k][1],base[k][2]},factor[k]));return s;};
  // metric frame of the start: d = R0^{-1} e (unit metric length)
  const auto R=FactorOf(t0);if(!R.ok)return out;
  auto frame=[&](double e0,double e1,double e2)->Point{const double z=e2/R.r22,y=(e1-R.r12*z)/R.r11,x=(e0-R.r01*y-R.r02*z)/R.r00;return {x,y,z};};
  std::vector<Point> dirs;const double r3=1/std::sqrt(3.);
  for(int a=0;a<3;++a)for(int s:{-1,1}){double e[3]={0,0,0};e[a]=s;dirs.push_back(frame(e[0],e[1],e[2]));}
  for(int sx:{-1,1})for(int sy:{-1,1})for(int sz:{-1,1})dirs.push_back(frame(sx*r3,sy*r3,sz*r3));
  Point x=start;double best=F(x),h=.25;
  while(h>=1e-3&&out.evaluations<budget){Point bx=x;double bs=best;
    for(const auto& d:dirs){const Point y{x.x+h*d.x,x.y+h*d.y,x.z+h*d.z};const double s=F(y);if(s>bs){bs=s;bx=y;}if(out.evaluations>=budget)break;}
    if(bs>best){best=bs;x=bx;}else h/=2;}
  out.ok=std::isfinite(best);out.x=x;out.score=best;return out;
}
class MeshOptimizer {
 public:
  MeshOptimizer(Owned& mesh,const RegionMetric& target,Id& next_node,Id& next_cell,const OptimizeOptions& options,OptimizeReport& report)
      : mesh(mesh),target(target),next_node(next_node),next_cell(next_cell),options(options),report(report),started(Clock::now()) {}
  bool Run();

 private:
  static constexpr double Gain=1e-6;
  static constexpr long double EstimatePass=1.4L;  // new edges estimated at most this long are not integrated
  Owned& mesh;
  const RegionMetric& target;
  Id& next_node;
  Id& next_cell;
  const OptimizeOptions& options;
  OptimizeReport& report;
  Clock::time_point started;
  std::unique_ptr<PlanarReference> moved_geometry;
  bool changed=false,limited=false;
  size_t stage_work_end=SIZE_MAX;
  std::unordered_map<Id,std::vector<Id>> star;   // vertex -> incident cells
  std::unordered_map<Id,Point> points;
  std::map<Face,Id> physical;                    // Current physical faces for first-height deficits.
  std::set<Id> fixed;                            // skin vertices: only geometry-certified movement may change them
  std::map<Id,std::array<double,3>> sliding;     // TEST-ONLY diagnostic sliding planes (options.diagnostic_sliding, follows moves)
  std::set<Id> keep;                             // options.keep_vertices, following moves (never collapsed)
  std::unordered_map<Id,double> height_scores;  // Current cells are immutable by identity.
  std::unordered_map<std::array<double,3>,Tensor,CoordinateHash> tensors;
  std::map<std::array<Id,4>,CellQuality> qualities;  // ordered cell vertex ids (an identity keeps its coordinates)
  std::map<std::array<double,6>,long double> exact;
  bool Limited() {
    if(limited)return true;
    if(report.trials>=options.maximum_work)limited=true;
    if(options.maximum_seconds>0 && std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)limited=true;
    return limited || report.trials>=stage_work_end;
  }
  void Bound() {
    if(height_scores.size()>=options.cache_entries)height_scores.clear();
    if(tensors.size()>=options.cache_entries)tensors.clear();
    if(qualities.size()>=options.cache_entries)qualities.clear();
    if(exact.size()>=options.cache_entries)exact.clear();
  }
  void Index() {
    star.clear();points.clear();fixed.clear();physical.clear();std::map<Face,int> uses;
    for(const auto& row:mesh) {
      for(const auto& n:row.second.cell.v){star[n.id].push_back(row.first);points[n.id]=n.p;}
      const auto faces=OppositeFaces(row.second.cell);
      for(unsigned k=0;k<4;++k){const auto f=Key(faces[k]);++uses[f];if(row.second.physical[k].present)physical.emplace(f,row.second.physical[k].facet);}
    }
    for(const auto& u:uses)if(u.second==1)fixed.insert(u.first.begin(),u.first.end());
    sliding=options.diagnostic_sliding;keep=options.keep_vertices;
  }
  Tensor At(Point p) {
    const std::array<double,3> key{p.x,p.y,p.z};const auto found=tensors.find(key);
    if(found!=tensors.end())return found->second;
    ++report.point_queries;const auto m=target.point(p);if(tensors.size()>=options.cache_entries)tensors.clear();tensors.emplace(key,m);return m;
  }
  // q, J in the metric at the actual centroid; q = J = -1 for a non-positive or unmeasurable cell
  CellQuality Quality(const Cell& c,bool memo=true) {
    if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0))return {};
    const std::array<Id,4> key{c.v[0].id,c.v[1].id,c.v[2].id,c.v[3].id};
    if(memo){const auto found=qualities.find(key);if(found!=qualities.end())return found->second;}
    CellQuality r;
    try{const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},At(Centroid(c)));r.q=m.mean_ratio;r.J=m.minimum_scaled_jacobian;}catch(const std::exception&){r=CellQuality{};}
    if(memo){if(qualities.size()>=options.cache_entries)qualities.clear();qualities.emplace(key,r);}
    return r;
  }
  double Score(const Cell& c){return Quality(c).Score();}
  // DC-17: score of a current mesh cell, from the caller's identity memo when given (same exact measure)
  Patch GeometryPatch(const std::vector<Cell>& cells) const {
    Patch patch;patch.cells=cells;
    for(const auto& c:cells)for(auto f:OppositeFaces(c)){const auto b=physical.find(Key(f));if(b!=physical.end())patch.surface.push_back({f,b->second});}
    if(options.geometry){patch.parameters=report.parameters;patch.coverage=report.coverage;RetainPhysicalCoverage(patch);
      const auto& geometry=moved_geometry?*moved_geometry:*options.geometry;geometry.ExportParameters(patch);}
    return patch;
  }
  double Height(const Patch& patch) const {
    if(!options.geometry || options.wall_heights.empty())return INFINITY;
    const auto& base=moved_geometry?*moved_geometry:*options.geometry;auto parameters=base.PrivateParameters(patch);
    return HeightScore(patch,options.wall_heights,parameters?*parameters:base);
  }
  double RowScore(Id id,const Cell& c){
    CellQuality cached;
    const double score=options.mesh_quality && options.mesh_quality(id,cached.q,cached.J)?cached.Score():Score(c);
    if(options.wall_heights.empty())return score;
    auto found=height_scores.find(id);
    if(found==height_scores.end()) {
      if(height_scores.size()>=options.cache_entries)height_scores.clear();
      found=height_scores.emplace(id,Height(GeometryPatch({c}))).first;
    }
    return std::min(score,found->second);}
  double MinScore(const std::vector<Id>& ids){double m=INFINITY;for(Id id:ids)m=std::min(m,RowScore(id,mesh.at(id).cell));return m;}
  // Minimum score of finite, exact-positive fresh cells.
  double FreshScore(const std::vector<Cell>& cells,bool memo=true,const Patch* patch=nullptr) {
    double m=INFINITY;for(const auto& c:cells){const auto r=Quality(c,memo);if(!r.Valid())return -INFINITY;m=std::min(m,r.Score());}
    return options.wall_heights.empty()?m:std::min(m,Height(patch?*patch:GeometryPatch(cells)));}
  long double Estimate(Point a,Point b) {
    const Point mid{double(((long double)a.x+b.x)/2),double(((long double)a.y+b.y)/2),double(((long double)a.z+b.z)/2)};
    ++report.edge_surrogates;
    return (MetricNorm(a,b,At(a))+4*MetricNorm(a,b,At(mid))+MetricNorm(a,b,At(b)))/6;
  }
  long double Exact(Point a,Point b) {
    std::array<double,6> key{a.x,a.y,a.z,b.x,b.y,b.z};
    if(std::make_tuple(b.x,b.y,b.z)<std::make_tuple(a.x,a.y,a.z))key={b.x,b.y,b.z,a.x,a.y,a.z};
    const auto found=exact.find(key);if(found!=exact.end())return found->second;
    ++report.edge_integrations;const auto l=target.edge(a,b);if(exact.size()>=options.cache_entries)exact.clear();exact.emplace(key,l);return l;
  }
  // Edge guard of an accepted candidate: the exact composed length when the estimate exceeds EstimatePass.
  bool Certified(Point a,Point b,long double limit) {
    try{const auto e=Estimate(a,b);if(e<=EstimatePass)return true;if(e>1.45L*limit)return false;return Exact(a,b)<=limit;}
    catch(const std::exception&){return false;}
  }
  bool Frozen(Id cell) const {  // no topology change (DC-17: also cells holding a structure vertex)
    if(options.frozen_cells.count(cell))return true;
    if(options.structure_vertices.empty())return false;
    const auto f=mesh.find(cell);if(f==mesh.end())return false;
    for(const auto& n:f->second.cell.v)if(options.structure_vertices.count(n.id))return true;
    return false;}
  bool Movable(Id v) const {
    if((fixed.count(v)&&!sliding.count(v)) || options.frozen_vertices.count(v))return false;
    const auto s=star.find(v);if(s==star.end() || s->second.empty() || s->second.size()>MaximumCavityCells)return false;
    for(Id id:s->second)if(options.frozen_cells.count(id))return false;  // moves keep the topology of structure cells
    return true;
  }
  static std::vector<Id> Common(std::vector<Id> x,std::vector<Id> y) {
    std::vector<Id> out;std::sort(x.begin(),x.end());std::sort(y.begin(),y.end());
    std::set_intersection(x.begin(),x.end(),y.begin(),y.end(),std::back_inserter(out));return out;
  }
  bool Adjacent(Id a,Id b) const {const auto s=star.find(a);if(s==star.end())return false;for(Id id:s->second)if(CellHas(mesh.at(id).cell,b))return true;return false;}
  // Exact interface/ball-topology certificate of a replacement (temporary identities from next_cell).
  bool Interface(const std::vector<Id>& old,std::vector<Cell>& fresh) {
    std::vector<Cell> before;before.reserve(old.size());for(Id id:old)before.push_back(mesh.at(id).cell);
    for(size_t i=0;i<fresh.size();++i)fresh[i].id=next_cell+i;
    std::string reason;bool ok=false;
    try{ok=ValidateFixedInterface(before,fresh,reason,nullptr,std::max<size_t>(MaximumCavityCells,before.size()));}catch(const std::exception&){ok=false;}
    if(!ok)++report.rejected_interface;
    return ok;
  }
  // Replace old cells by fresh ones: every physical binding of an old face that survives is carried by its face key;
  // `added` binds the boundary faces a boundary operation creates (the faces it retires disappear with their cells).
  void Commit(const std::vector<Id>& old,std::vector<Cell> fresh,Id removed=UINT64_MAX,const std::map<Face,FacetBinding>& added={}) {
    std::map<Face,FacetBinding> bindings=added;std::set<Face> oldPhysical;
    for(Id id:old) {
      const auto& row=mesh.at(id);const auto faces=OppositeFaces(row.cell);
      for(unsigned k=0;k<4;++k)if(row.physical[k].present){auto f=faces[k];std::sort(f.begin(),f.end());bindings.emplace(f,row.physical[k]);oldPhysical.insert(f);physical.erase(f);}
      for(const auto& n:row.cell.v){auto& s=star[n.id];s.erase(std::remove(s.begin(),s.end(),id),s.end());}
      mesh.erase(id);
    }
    std::set<Face> used;
    for(auto& c:fresh) {
      c.id=next_cell++;OwnedCell row;row.cell=c;row.version=1;const auto faces=OppositeFaces(c);
      for(unsigned k=0;k<4;++k){auto f=faces[k];std::sort(f.begin(),f.end());const auto b=bindings.find(f);if(b!=bindings.end()){row.physical[k]=b->second;used.insert(f);physical.insert_or_assign(f,b->second.facet);}}
      if(!mesh.emplace(c.id,row).second)throw std::logic_error("Native3D optimizer repeated a cell identity.");
      for(const auto& n:c.v){star[n.id].push_back(c.id);points[n.id]=n.p;}
    }
    for(const auto& a:added)if(!used.count(a.first))throw std::logic_error("Native3D optimizer lost a created boundary face.");
    if(added.empty())for(const auto& f:oldPhysical)if(!used.count(f))throw std::logic_error("Native3D optimizer lost a physical face binding.");
    if(removed!=UINT64_MAX){const auto s=star.find(removed);if(s!=star.end()&&!s->second.empty())throw std::logic_error("Native3D optimizer removed a used vertex.");star.erase(removed);points.erase(removed);}
    changed=true;Bound();
  }
  bool EdgeRemoval(Id a,Id b,bool lengthDriven=false);
  bool FaceSwap(const Face& f);
  bool Collapse(Id p,Id q);
  bool Smooth(Id p);
  bool SmoothBoundary(Id p);
  bool TryCell(Id id,bool enrichment);
  // DC-13 (DC13_PLAN sect. 1): cone re-meshing of a star-shaped cavity from one new placed vertex
  enum class Insert { Shell, Neighbourhood, Length };
  bool ConeReplace(const std::vector<Id>& old,Point start,Insert kind,const std::pair<Id,Id>* split=nullptr,long double removed=0);
  bool ShellInsert(Id a,Id b,bool lengthDriven=false);
  bool NeighbourhoodInsert(Id cell);
  // boundary operations (no new point; every face stays inside its facet): O5 flip of a boundary edge between two faces
  // bound to the same facet, O6 contraction of a boundary vertex interior to one facet along a boundary edge
  struct BoundaryFace { Face oriented{};Id cell=0;FacetBinding binding; };
  std::vector<BoundaryFace> BoundaryFacesOf(const std::vector<Id>& cells,Id v) const {
    std::vector<BoundaryFace> out;for(Id id:cells){const auto& row=mesh.at(id);const auto faces=OppositeFaces(row.cell);
      for(unsigned k=0;k<4;++k)if(row.physical[k].present&&(v==UINT64_MAX||faces[k][0]==v||faces[k][1]==v||faces[k][2]==v))out.push_back({faces[k],id,row.physical[k]});}
    return out;
  }
  static long double Norm3(Point a,Point b){const long double x=(long double)a.x-b.x,y=(long double)a.y-b.y,z=(long double)a.z-b.z;return std::sqrt(x*x+y*y+z*z);}
  static long double Area2(Point a,Point b,Point c,long double n[3]) {
    const long double u[3]={(long double)b.x-a.x,(long double)b.y-a.y,(long double)b.z-a.z},w[3]={(long double)c.x-a.x,(long double)c.y-a.y,(long double)c.z-a.z};
    n[0]=u[1]*w[2]-u[2]*w[1];n[1]=u[2]*w[0]-u[0]*w[2];n[2]=u[0]*w[1]-u[1]*w[0];return std::sqrt(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
  }
  // fresh boundary = old boundary - retired + created, with the created faces inside the retired faces' plane: same
  // facet, unfolded (normals along the retired ones) and the same total area (no gap or overlap)
  bool BoundaryReplacement(const std::vector<Id>& old,std::vector<Cell>& fresh,const std::vector<BoundaryFace>& retired,FacetBinding binding,std::map<Face,FacetBinding>& added) {
    std::vector<Cell> before;for(Id id:old)before.push_back(mesh.at(id).cell);
    for(size_t i=0;i<fresh.size();++i)fresh[i].id=next_cell+i;
    std::vector<Face> oldSkin,newSkin;
    try{oldSkin=Incidence(before).Boundary();newSkin=Incidence(fresh).Boundary();}catch(const std::exception&){++report.rejected_interface;return false;}
    auto canon=[](Face f){std::rotate(f.begin(),std::min_element(f.begin(),f.end()),f.end());return f;};
    std::set<Face> retiredSet,oldSet,newSet;for(const auto& r:retired)retiredSet.insert(canon(r.oriented));
    for(const auto& f:oldSkin)oldSet.insert(canon(f));for(const auto& f:newSkin)newSet.insert(canon(f));
    for(const auto& f:oldSet)if(!retiredSet.count(f)&&!newSet.count(f)){++report.rejected_interface;return false;}
    long double n0[3]={0,0,0},area0=0,area1=0;
    for(const auto& r:retired){long double n[3];area0+=Area2(points.at(r.oriented[0]),points.at(r.oriented[1]),points.at(r.oriented[2]),n);for(int i=0;i<3;++i)n0[i]+=n[i];}
    added.clear();
    for(const auto& f:newSet){if(oldSet.count(f))continue;
      Point q[3];for(int i=0;i<3;++i){bool found=false;for(const auto& c:fresh)for(const auto& v:c.v)if(v.id==f[i]){q[i]=v.p;found=true;}if(!found)return false;}
      long double n[3];const auto a=Area2(q[0],q[1],q[2],n);area1+=a;if(!(n[0]*n0[0]+n[1]*n0[1]+n[2]*n0[2]>0)){++report.rejected_interface;return false;}
      auto key=f;std::sort(key.begin(),key.end());added.emplace(key,binding);}
    if(!(std::abs(area1-area0)<=1e-10L*area0)){++report.rejected_interface;return false;}
    std::string reason;bool ok=false;
    try{ok=ValidatePrescribedInterface(before,fresh,newSkin,reason,nullptr,std::max<size_t>(MaximumCavityCells,before.size()));}catch(const std::exception&){ok=false;}
    if(!ok)++report.rejected_interface;
    return ok;
  }
  bool BoundaryFlip(Id a,Id b);
  bool BoundaryCollapse(Id p,Id q);
};
// O1: interior edge with a closed shell of n cells (3 <= n <= maximum_shell) replaced by the 2(n-2) cells of the best
// triangulation of its ring polygon (dynamic programming maximizing the minimum score; ring order with (a,b,r_i,r_i+1)
// positive, so (a,r_i,r_k,r_j) and (b,r_i,r_j,r_k) for i < k < j are the positive orientations).
// lengthDriven (O1L): the edge is longer than 1.8; accepted when no new edge exceeds 1.8 and the replaced cells' minimum
// score does not fall below min(old minimum, 1) (the long edge leaves the mesh without creating a final-gate failure).
bool MeshOptimizer::EdgeRemoval(Id a,Id b,bool lengthDriven) {
  if(Limited())return false;
  const auto sa=star.find(a),sb=star.find(b);if(sa==star.end()||sb==star.end())return false;
  const auto shell=Common(sa->second,sb->second);const size_t n=shell.size();
  if(n<3 || n>options.maximum_shell)return false;
  for(Id id:shell)if(Frozen(id)){++report.rejected_frozen;return false;}
  std::map<Id,std::vector<Id>> ring;
  for(Id id:shell){std::vector<Id> o;for(const auto& v:mesh.at(id).cell.v)if(v.id!=a&&v.id!=b)o.push_back(v.id);if(o.size()!=2)return false;ring[o[0]].push_back(o[1]);ring[o[1]].push_back(o[0]);}
  if(ring.size()!=n)return false;
  for(const auto& r:ring)if(r.second.size()!=2)return false;  // open (boundary) shell
  ++report.trials;
  const Point pa=points.at(a),pb=points.at(b);
  std::vector<Id> order{ring.begin()->first};
  {const Id r0=order[0];Id next=UINT64_MAX;for(Id y:ring.at(r0))if(Orientation(pa,pb,points.at(r0),points.at(y))>0)next=y;if(next==UINT64_MAX)return false;
   Id prev=r0;Id cur=next;while(cur!=r0){if(order.size()>n)return false;order.push_back(cur);const auto& nb=ring.at(cur);const Id nx=nb[0]==prev?nb[1]:nb[0];prev=cur;cur=nx;}}
  if(order.size()!=n)return false;
  for(size_t i=0;i<n;++i)if(!(Orientation(pa,pb,points.at(order[i]),points.at(order[(i+1)%n]))>0))return false;
  const double oldMin=MinScore(shell);
  long double limit=MaximumMetricEdge;
  if(!lengthDriven){try{const auto e=Estimate(pa,pb);limit=std::max<long double>(limit,e>EstimatePass?Exact(pa,pb):e);}catch(const std::exception&){}}
  const double floor=lengthDriven?std::min(oldMin,1.)-1e-12:oldMin+Gain;
  const Node A{a,pa},B{b,pb};std::vector<Node> r(n);for(size_t i=0;i<n;++i)r[i]={order[i],points.at(order[i])};
  std::map<std::array<size_t,3>,double> tri;
  auto triangle=[&](size_t i,size_t k,size_t j){const std::array<size_t,3> key{i,k,j};const auto f=tri.find(key);if(f!=tri.end())return f->second;
    const double s=FreshScore({Cell{0,{A,r[i],r[k],r[j]}},Cell{0,{B,r[i],r[j],r[k]}}});tri.emplace(key,s);return s;};
  std::set<std::pair<size_t,size_t>> forbidden;
  auto ringEdge=[&](size_t i,size_t j){return j==i+1 || (i==0&&j==n-1);};
  for(unsigned attempt=0;attempt<4;++attempt) {
    std::vector<std::vector<double>> best(n,std::vector<double>(n,-INFINITY));std::vector<std::vector<size_t>> arg(n,std::vector<size_t>(n,SIZE_MAX));
    for(size_t i=0;i+1<n;++i)best[i][i+1]=INFINITY;
    for(size_t len=2;len<n;++len)for(size_t i=0;i+len<n;++i) {
      const size_t j=i+len;
      for(size_t k=i+1;k<j;++k) {
        if((!ringEdge(i,k)&&forbidden.count({i,k})) || (!ringEdge(k,j)&&forbidden.count({k,j})))continue;
        const double v=std::min({best[i][k],best[k][j],triangle(i,k,j)});
        if(v>best[i][j]){best[i][j]=v;arg[i][j]=k;}
      }
    }
    if(!(best[0][n-1]>floor)){if(best[0][n-1]==-INFINITY)++report.rejected_validity;else ++report.rejected_quality;return false;}
    std::vector<std::array<size_t,3>> used;std::vector<std::pair<size_t,size_t>> stack{{0,n-1}};
    while(!stack.empty()){const auto e=stack.back();stack.pop_back();if(e.second<=e.first+1)continue;const size_t k=arg[e.first][e.second];
      used.push_back({e.first,k,e.second});stack.push_back({e.first,k});stack.push_back({k,e.second});}
    bool ok=true;
    for(const auto& t:used)for(const auto& d:{std::make_pair(t[0],t[1]),std::make_pair(t[1],t[2]),std::make_pair(t[0],t[2])}) {
      if(ringEdge(d.first,d.second)||forbidden.count(d))continue;
      if(!Certified(r[d.first].p,r[d.second].p,limit)){forbidden.insert(d);ok=false;break;}
    }
    if(!ok){++report.rejected_edge;if(Limited())return false;continue;}
    std::vector<Cell> fresh;for(const auto& t:used){fresh.push_back(Cell{0,{A,r[t[0]],r[t[1]],r[t[2]]}});fresh.push_back(Cell{0,{B,r[t[0]],r[t[2]],r[t[1]]}});}
    if(!Interface(shell,fresh))return false;
    Commit(shell,fresh);++(lengthDriven?report.length_removals:report.edge_removals);return true;
  }
  return false;
}
// O2: interior face (x,y,z) of two unfrozen cells with apices d, e (not yet connected) -> three cells around (d,e).
bool MeshOptimizer::FaceSwap(const Face& f) {
  if(Limited())return false;
  const auto both=Common(Common(star.at(f[0]),star.at(f[1])),star.at(f[2]));
  if(both.size()!=2)return false;
  for(Id id:both)if(Frozen(id)){++report.rejected_frozen;return false;}
  Id d=UINT64_MAX,e=UINT64_MAX;for(const auto& v:mesh.at(both[0]).cell.v)if(v.id!=f[0]&&v.id!=f[1]&&v.id!=f[2])d=v.id;
  for(const auto& v:mesh.at(both[1]).cell.v)if(v.id!=f[0]&&v.id!=f[1]&&v.id!=f[2])e=v.id;
  if(d==UINT64_MAX||e==UINT64_MAX||d==e||Adjacent(d,e))return false;
  ++report.trials;
  const Node D{d,points.at(d)},E{e,points.at(e)};std::array<Node,3> r{Node{f[0],points.at(f[0])},Node{f[1],points.at(f[1])},Node{f[2],points.at(f[2])}};
  if(!(Orientation(D.p,E.p,r[0].p,r[1].p)>0))std::swap(r[1],r[2]);
  std::vector<Cell> fresh{Cell{0,{D,E,r[0],r[1]}},Cell{0,{D,E,r[1],r[2]}},Cell{0,{D,E,r[2],r[0]}}};
  const double oldMin=MinScore(both),newMin=FreshScore(fresh);
  if(newMin==-INFINITY){++report.rejected_validity;return false;}
  if(!(newMin>oldMin+Gain)){++report.rejected_quality;return false;}
  if(!Certified(D.p,E.p,MaximumMetricEdge)){++report.rejected_edge;return false;}
  if(!Interface(both,fresh))return false;
  Commit(both,fresh);++report.face_swaps;return true;
}
// O3: interior vertex p contracted to its neighbour q (cells of the edge disappear, the rest take q).
bool MeshOptimizer::Collapse(Id p,Id q) {
  if(Limited()||keep.count(p)||!Movable(p)||!Adjacent(p,q))return false;
  const auto old=star.at(p);
  ++report.trials;
  const Node Q{q,points.at(q)};std::vector<Cell> fresh;std::set<Id> link;
  for(Id id:old){const auto& c=mesh.at(id).cell;for(const auto& v:c.v)if(v.id!=p)link.insert(v.id);if(CellHas(c,q))continue;Cell x=c;for(auto& v:x.v)if(v.id==p)v=Q;fresh.push_back(x);}
  if(fresh.empty())return false;
  const double oldMin=MinScore(old),newMin=FreshScore(fresh);
  if(newMin==-INFINITY){++report.rejected_validity;return false;}
  if(!(newMin>oldMin+Gain)){++report.rejected_quality;return false;}
  for(Id x:link){if(x==q||Adjacent(q,x))continue;if(!Certified(Q.p,points.at(x),MaximumMetricEdge)){++report.rejected_edge;return false;}}
  if(!Interface(old,fresh))return false;
  Commit(old,fresh,p);++report.collapses;return true;
}
// Physical stars move through their forward geometry bindings; artificial interfaces remain fixed.
bool MeshOptimizer::SmoothBoundary(Id p) {
  if(Limited() || !options.geometry || options.frozen_vertices.count(p))return false;
  const auto found=star.find(p);
  if(found==star.end() || found->second.empty() || found->second.size()>MaximumCavityCells)return false;
  const auto old=found->second;Patch patch;
  for(Id id:old) {
    if(options.frozen_cells.count(id))return false;
    const auto& row=mesh.at(id);patch.cells.push_back(row.cell);const auto faces=OppositeFaces(row.cell);
    for(unsigned k=0;k<4;++k)if(row.physical[k].present)patch.surface.push_back({faces[k],row.physical[k].facet});
  }
  if(patch.surface.empty())return false;
  std::set<Face> old_physical;for(const auto& f:patch.surface)old_physical.insert(Key(f.v));
  for(auto f:Incidence(patch.cells).Boundary())
    if(std::find(f.begin(),f.end(),p)!=f.end() && !old_physical.count(Key(f)))return false;
  const auto& geometry=moved_geometry?*moved_geometry:*options.geometry;
  try{geometry.ExportParameters(patch);}catch(const std::exception&){return false;}
  const double before=MinScore(old);const auto origin=points.at(p);
  const long double geometric_before=GetShapeAdmission()==ShapeAdmission::Absolute?CertifiedGeometryExcess(patch,geometry,options.surface_tolerance):0;
  std::vector<Point> targets;std::set<Id> neighbors;
  for(const auto& c:patch.cells)for(auto n:c.v)if(n.id!=p)neighbors.insert(n.id);
  if(geometry.Mapped()) {
    const auto binding=patch.parameters.at(p).binding;const auto& authority=geometry.Parameters()->Geometry();
    const auto* mobility=geometry.Mobility();
    if(mobility && authority.MobilityDimension(binding,*mobility)==0)return false;
    if(!mobility && binding.dimension==0 && authority.PhysicalSupport(binding))return false;
    if(!mobility && binding.dimension==1 && (!authority.ReferenceComponents() || authority.PhysicalSupport(binding))) {
      Edge support{};bool located=false;
      authority.ForFacets(binding,[&](Id facet) {
        if(located)return;const auto& f=geometry.Get(facet);
        for(unsigned k=0;k<3;++k){Edge e{f.v[k].id,f.v[(k+1)%3].id};std::sort(e.begin(),e.end());
          if(authority.EdgePoint(e,.5).feature==binding.feature){support=e;located=true;break;}}
      });
      if(!located)return false;
      Point a{},b{};
      authority.ForFacets(binding,[&](Id facet){for(auto n:geometry.Get(facet).v){if(n.id==support[0])a=n.p;if(n.id==support[1])b=n.p;}});
      // Search the forward parameter, not a chord-plane derivative of the already curved point.
      for(unsigned level=1;level<=4;++level) {
        const unsigned denominator=1u<<level;
        for(unsigned k=1;k<denominator;k+=2){const long double t=static_cast<long double>(k)/denominator;
          targets.push_back({double(a.x+t*(b.x-(long double)a.x)),double(a.y+t*(b.y-(long double)a.y)),double(a.z+t*(b.z-(long double)a.z))});}
      }
    } else if(!mobility && binding.dimension==2 && !authority.ReferenceComponents()) {
      const auto& f=geometry.Get(binding.feature);
      for(double h:{.25,.125,.0625,.03125})for(const auto d:std::array<std::array<int,2>,6>{{{{1,0}},{{-1,0}},{{0,1}},{{0,-1}},{{1,-1}},{{-1,1}}}}) {
        const long double u=binding.u+h*d[0],v=binding.v+h*d[1];if(u<=0 || v<=0 || u+v>=1)continue;
        const auto a=f.v[0].p,b=f.v[1].p,c=f.v[2].p;
        targets.push_back({double(a.x+u*(b.x-(long double)a.x)+v*(c.x-(long double)a.x)),
            double(a.y+u*(b.y-(long double)a.y)+v*(c.y-(long double)a.y)),double(a.z+u*(b.z-(long double)a.z)+v*(c.z-(long double)a.z))});
      }
    }
  }
  if(targets.empty())try {
    std::vector<std::pair<Point,Tensor>> samples;double radius=INFINITY;
    for(Id n:neighbors){samples.emplace_back(points.at(n),At(points.at(n)));radius=std::min(radius,double(Norm3(origin,points.at(n))));}
    targets.push_back(MetricMeanPoint(origin,samples));
    for(double step:{.25,.125,.0625})for(unsigned axis=0;axis<3;++axis)for(double sign:{-1.,1.}) {
      Point q=origin;if(axis==0)q.x+=sign*step*radius;else if(axis==1)q.y+=sign*step*radius;else q.z+=sign*step*radius;targets.push_back(q);
    }
  }catch(const std::exception&){return false;}
  Patch best;double best_score=before;
  for(Point target_point:targets) {
    if(Limited())break;++report.trials;Patch candidate;std::string reason;
    if(geometry.Mobility()) {
      // Projection and volume quality precede expensive surface reconstruction; height can only lower this score.
      double upper=INFINITY;
      {
        auto context=geometry.PrivateParameters(patch);const auto& projected_geometry=context?*context:geometry;
        auto* budget=projected_geometry.AtlasReservation();SU2NativeGeometry::CurvedAtlas atlas;BoundaryPoint projected;
        if(projected_geometry.EntityGeometry()) {
          try {std::set<Id> incident;for(const auto& face:patch.surface)if(std::find(face.v.begin(),face.v.end(),p)!=face.v.end())incident.insert(face.facet);
            projected.node=projected_geometry.BindMovedPoint(p,{next_node,target_point},incident);
          }catch(const std::exception&){++report.rejected_interface;continue;}
        } else if(!budget || !SU2NativeGeometry::PrepareCurvedAtlas(patch,projected_geometry,p,atlas,*budget,reason) ||
            !SU2NativeGeometry::ProposeAtlasBinding(atlas,p,{next_node,target_point},projected,*budget,reason)) {
          ++report.rejected_interface;continue;
        }
        for(auto cell:patch.cells) {
          for(auto& node:cell.v)if(node.id==p)node=projected.node;
          const auto measured=Quality(cell,false);
          if(!measured.Valid()){upper=-INFINITY;break;}
          upper=std::min(upper,measured.Score());
        }
      }
      if(!std::isfinite(upper)){++report.rejected_validity;continue;}
      if(!(upper>best_score+Gain)){++report.rejected_quality;continue;}
    }
    // Use the raw proposal again: projecting the realized surface point need not be idempotent.
    if(!MoveVertex(patch,p,{next_node,target_point},next_cell,geometry,candidate,reason)){++report.rejected_interface;continue;}
    const double score=FreshScore(candidate.cells,false,&candidate);
    if(!std::isfinite(score)){++report.rejected_validity;continue;}
    if(!(score>best_score+Gain)){++report.rejected_quality;continue;}
    auto candidate_parameters=geometry.PrivateParameters(candidate);const auto& candidate_geometry=candidate_parameters?*candidate_parameters:geometry;
    FirstHeightReport height;
    if(GetShapeAdmission()==ShapeAdmission::Absolute && !CheckFirstHeight(candidate,options.wall_heights,candidate_geometry,height,reason)){++report.rejected_frozen;continue;}
    if(GetShapeAdmission()==ShapeAdmission::Absolute && CertifiedGeometryExcess(candidate,candidate_geometry,options.surface_tolerance)>geometric_before){++report.rejected_interface;continue;}
    bool sized=true;
    for(const auto& c:candidate.cells)for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j)
      if((c.v[i].id==next_node || c.v[j].id==next_node) && !Certified(c.v[i].p,c.v[j].p,MaximumMetricEdge))sized=false;
    if(!sized){++report.rejected_edge;continue;}
    best_score=score;best=std::move(candidate);
    if(best_score>=1)break;
  }
  if(best.cells.empty())return false;
  std::map<Face,FacetBinding> added;for(const auto& f:best.surface)added.emplace(Key(f.v),FacetBinding{true,f.facet});
  const Id moved=next_node++;Commit(old,best.cells,p,added);
  report.parameters.erase(p);for(const auto& row:best.parameters)report.parameters.insert_or_assign(row.first,row.second);
  for(const auto& row:best.coverage.faces)report.coverage.faces.insert_or_assign(row.first,row.second);
  for(const auto& row:best.coverage.traces)report.coverage.traces.insert_or_assign(row.first,row.second);
  for(auto it=report.coverage.faces.begin();it!=report.coverage.faces.end();)if(!physical.count(Key(it->first)))it=report.coverage.faces.erase(it);else ++it;
  for(auto it=report.coverage.traces.begin();it!=report.coverage.traces.end();)if(!Adjacent(it->first[0],it->first[1]))it=report.coverage.traces.erase(it);else ++it;
  Patch overlay;overlay.parameters=report.parameters;overlay.coverage=report.coverage;
  for(const auto& row:overlay.coverage.faces)overlay.surface.push_back({row.first,physical.at(Key(row.first))});
  auto parameters=options.geometry->PrivateParameters(overlay);if(parameters)moved_geometry=std::move(parameters);
  fixed.erase(p);fixed.insert(moved);if(keep.erase(p))keep.insert(moved);
  Id first=p;for(const auto& row:report.relabel)if(row.second==p){first=row.first;break;}
  report.relabel[first]=moved;++report.moves;return true;
}
bool MeshOptimizer::Smooth(Id p) {
  if(fixed.count(p) && !sliding.count(p) && options.geometry)return SmoothBoundary(p);
  if(Limited()||!Movable(p))return false;
  const auto old=star.at(p);const Point x0=points.at(p);
  std::set<Id> nbr;for(Id id:old)for(const auto& v:mesh.at(id).cell.v)if(v.id!=p)nbr.insert(v.id);
  ++report.trials;
  const double oldMin=MinScore(old);
  long double oldMax=0,shortest=INFINITY;
  for(Id x:nbr){const Point px=points.at(x);try{oldMax=std::max(oldMax,Estimate(x0,px));}catch(const std::exception&){oldMax=INFINITY;}
    const long double dx=(long double)px.x-x0.x,dy=(long double)px.y-x0.y,dz=(long double)px.z-x0.z;shortest=std::min(shortest,std::sqrt(dx*dx+dy*dy+dz*dz));}
  const long double limit=std::max<long double>(MaximumMetricEdge,oldMax);
  std::vector<Point> targets;
  if(options.geometry && !options.wall_heights.empty()) {
    std::vector<Cell> cells;for(Id id:old)cells.push_back(mesh.at(id).cell);
    const auto patch=GeometryPatch(cells);
    for(const auto& f:patch.surface) {
      const auto h=options.wall_heights.find(options.geometry->Get(f.facet).marker);
      if(h==options.wall_heights.end() || std::find(f.v.begin(),f.v.end(),p)!=f.v.end())continue;
      bool apex=false;for(const auto& c:patch.cells){unsigned hits=0;for(auto n:c.v)hits+=std::find(f.v.begin(),f.v.end(),n.id)!=f.v.end();apex=apex || hits==3;}
      if(!apex)continue;
      const auto a=points.at(f.v[0]),b=points.at(f.v[1]),c=points.at(f.v[2]);long double normal[3];const auto area=Area2(a,b,c,normal);
      if(!(area>0))continue;
      const long double correction=(((long double)x0.x-a.x)*normal[0]+((long double)x0.y-a.y)*normal[1]+
          ((long double)x0.z-a.z)*normal[2])/area+h->second;
      targets.push_back({double(x0.x-correction*normal[0]/area),double(x0.y-correction*normal[1]/area),double(x0.z-correction*normal[2]/area)});
      targets.push_back({double(((long double)a.x+b.x+c.x)/3-h->second*normal[0]/area),
          double(((long double)a.y+b.y+c.y)/3-h->second*normal[1]/area),double(((long double)a.z+b.z+c.z)/3-h->second*normal[2]/area)});
    }
  }
  try{std::vector<std::pair<Point,Tensor>> samples;for(Id x:nbr)samples.push_back({points.at(x),At(points.at(x))});targets.push_back(MetricMeanPoint(x0,samples));}catch(const std::exception&){}
  try{  // damped hinge Gauss-Newton on the failing cells of the star (LinearizeMetricShape residuals, 1% interior aim)
    long double g[3]={0,0,0},H[3][3]={{0,0,0},{0,0,0},{0,0,0}};bool any=false;
    for(Id id:old){const auto& c=mesh.at(id).cell;if(Score(c)>=1.01)continue;unsigned slot=0;while(c.v[slot].id!=p)++slot;
      const auto L=LinearizeMetricShape({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},At(Centroid(c)));
      for(unsigned r=0;r<2;++r)if(L.residual[r]>0){any=true;const auto& d=L.derivative[r][slot];
        for(unsigned i=0;i<3;++i){g[i]+=L.residual[r]*d[i];for(unsigned j=0;j<3;++j)H[i][j]+=(long double)d[i]*d[j];}}}
    if(any) {
      const long double trace=H[0][0]+H[1][1]+H[2][2];if(!(trace>0))throw std::runtime_error("flat hinge");
      for(unsigned i=0;i<3;++i)H[i][i]+=1e-6L*trace;
      long double M[3][4];for(unsigned i=0;i<3;++i){for(unsigned j=0;j<3;++j)M[i][j]=H[i][j];M[i][3]=-g[i];}
      for(unsigned c=0;c<3;++c){unsigned r=c;for(unsigned t=c+1;t<3;++t)if(std::abs(M[t][c])>std::abs(M[r][c]))r=t;if(!(std::abs(M[r][c])>0))throw std::runtime_error("singular hinge");
        for(unsigned q=0;q<4;++q)std::swap(M[c][q],M[r][q]);for(unsigned t=0;t<3;++t)if(t!=c){const auto f=M[t][c]/M[c][c];for(unsigned q=c;q<4;++q)M[t][q]-=f*M[c][q];}}
      long double d[3];for(unsigned i=0;i<3;++i)d[i]=M[i][3]/M[i][i];
      const long double norm=std::sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]),cap=.5L*shortest;
      if(norm>cap&&norm>0)for(auto& v:d)v*=cap/norm;
      if(std::isfinite(norm)&&norm>0)targets.push_back({double(x0.x+d[0]),double(x0.y+d[1]),double(x0.z+d[2])});
    }
  }catch(const std::exception&){}
  // DC-13 O4m: min-max placement over the star (approximate proposal, exact trial below), from p and each proposal
  bool placed=false;
  const auto slideIt=sliding.find(p);const bool slides=slideIt!=sliding.end();const std::array<double,3> plane=slides?slideIt->second:std::array<double,3>{0,0,0};
  auto inPlane=[&](Point t)->Point{if(!slides)return t;const auto& n=plane;const double d=(t.x-x0.x)*n[0]+(t.y-x0.y)*n[1]+(t.z-x0.z)*n[2];return {t.x-d*n[0],t.y-d*n[1],t.z-d*n[2]};};
  for(auto& t:targets)t=inPlane(t);
  {std::vector<Cell> cells;for(Id id:old)cells.push_back(mesh.at(id).cell);auto faces=CavityBoundary(cells);
   faces.erase(std::remove_if(faces.begin(),faces.end(),[&](const Face& f){return f[0]==p||f[1]==p||f[2]==p;}),faces.end());  // the link of p
   std::vector<Point> starts{x0};for(const auto& t:targets)starts.push_back(t);Placement best;
   for(const auto& st:starts){const auto pl=PlaceCone(faces,[&](Id v){return points.at(v);},[&](Point q){return At(q);},st,nullptr,std::max<size_t>(16,options.placement_budget/starts.size()));
     report.placement_evaluations+=pl.evaluations;if(pl.ok&&pl.score>best.score)best=pl;}
   if(best.ok)best.x=inPlane(best.x);
   ++report.placements;if(best.ok&&!(best.x.x==x0.x&&best.x.y==x0.y&&best.x.z==x0.z)){targets.insert(targets.begin(),best.x);placed=true;}}
  struct Trial{double score;Point x;};std::vector<Trial> trials;
  for(size_t ti=0;ti<targets.size();++ti)for(long double w:{1.L,.5L,.25L}) {const Point t=targets[ti];if(ti==0&&placed&&w!=1.L)continue;  // the placed point is tried as is
    const Point x{double(x0.x+w*((long double)t.x-x0.x)),double(x0.y+w*((long double)t.y-x0.y)),double(x0.z+w*((long double)t.z-x0.z))};
    if(x.x==x0.x&&x.y==x0.y&&x.z==x0.z)continue;
    const Node P{next_node,x};std::vector<Cell> fresh;for(Id id:old){Cell c=mesh.at(id).cell;for(auto& v:c.v)if(v.id==p)v=P;fresh.push_back(c);}
    const double s=FreshScore(fresh,false);  // the trial identity next_node is reused: never memoized
    if(s==-INFINITY)++report.rejected_validity;else if(s>oldMin+Gain)trials.push_back({s,x});
  }
  if(trials.empty()){++report.rejected_quality;return false;}
  std::stable_sort(trials.begin(),trials.end(),[](const Trial& u,const Trial& v){return u.score>v.score;});
  for(const auto& t:trials) {
    bool ok=true;for(Id x:nbr)if(!Certified(t.x,points.at(x),limit)){ok=false;break;}
    if(!ok){++report.rejected_edge;continue;}
    const Node P{next_node,t.x};std::vector<Cell> fresh;for(Id id:old){Cell c=mesh.at(id).cell;for(auto& v:c.v)if(v.id==p)v=P;fresh.push_back(c);}
    std::map<Face,FacetBinding> added;
    if(slides){for(Id id:old){const auto& row=mesh.at(id);const auto faces=OppositeFaces(row.cell);  // diagnostic: bindings follow the moved skin vertex
        for(unsigned k=0;k<4;++k)if(row.physical[k].present){auto f=faces[k];if(f[0]!=p&&f[1]!=p&&f[2]!=p)continue;for(auto& x:f)if(x==p)x=P.id;std::sort(f.begin(),f.end());added.emplace(f,row.physical[k]);}}}
    else if(!Interface(old,fresh))continue;
    const Id moved=next_node++;Commit(old,fresh,p,added);
    if(slides){sliding.erase(p);sliding[moved]=plane;fixed.insert(moved);}
    if(keep.erase(p))keep.insert(moved);
    Id origin=p;for(const auto& r:report.relabel)if(r.second==p){origin=r.first;break;}
    report.relabel[origin]=moved;++report.moves;return true;
  }
  return false;
}
// O5: boundary edge (a,b) between two boundary faces bound to the same facet (coplanar within 1e-12 relative); its open
// shell (ring path r_0..r_m from one face to the other) is replaced by the best triangulation of the polygon r_0..r_m
// coned to a and b, the boundary pair (a,b,r_0),(a,b,r_m) becoming (a,r_0,r_m),(b,r_0,r_m).
bool MeshOptimizer::BoundaryFlip(Id a,Id b) {
  if(Limited()||options.frozen_vertices.count(a)||options.frozen_vertices.count(b))return false;
  const auto sa=star.find(a),sb=star.find(b);if(sa==star.end()||sb==star.end())return false;
  const auto shell=Common(sa->second,sb->second);const size_t n=shell.size();
  if(n<2||n+1>options.maximum_shell)return false;
  for(Id id:shell)if(Frozen(id)){++report.rejected_frozen;return false;}
  std::vector<BoundaryFace> faces;for(const auto& f:BoundaryFacesOf(shell,UINT64_MAX)){int k=0;for(Id x:f.oriented)k+=x==a||x==b;if(k==2)faces.push_back(f);}
  if(faces.size()!=2||faces[0].binding.facet!=faces[1].binding.facet)return false;
  std::map<Id,std::vector<Id>> ring;
  for(Id id:shell){std::vector<Id> o;for(const auto& v:mesh.at(id).cell.v)if(v.id!=a&&v.id!=b)o.push_back(v.id);if(o.size()!=2)return false;ring[o[0]].push_back(o[1]);ring[o[1]].push_back(o[0]);}
  Id r0=UINT64_MAX,rm=UINT64_MAX;for(Id x:faces[0].oriented)if(x!=a&&x!=b)r0=x;for(Id x:faces[1].oriented)if(x!=a&&x!=b)rm=x;
  if(r0==UINT64_MAX||rm==UINT64_MAX||r0==rm||Adjacent(r0,rm)||ring.size()!=n+1||ring.at(r0).size()!=1||ring.at(rm).size()!=1)return false;
  std::vector<Id> order{r0};{Id prev=UINT64_MAX,cur=r0;while(cur!=rm){const auto& nb=ring.at(cur);Id nx=UINT64_MAX;for(Id y:nb)if(y!=prev)nx=y;if(nx==UINT64_MAX||order.size()>n)return false;order.push_back(nx);prev=cur;cur=nx;}}
  if(order.size()!=n+1)return false;
  Point pa=points.at(a),pb=points.at(b);
  if(!(Orientation(pa,pb,points.at(order[0]),points.at(order[1]))>0)){std::swap(a,b);std::swap(pa,pb);}
  for(size_t i=0;i+1<order.size();++i)if(!(Orientation(pa,pb,points.at(order[i]),points.at(order[i+1]))>0))return false;
  {long double scale=0;for(Point p:{pa,pb,points.at(r0),points.at(rm)})for(Point q:{pa,pb,points.at(r0),points.at(rm)})
     scale=std::max(scale,std::sqrt(((long double)p.x-q.x)*((long double)p.x-q.x)+((long double)p.y-q.y)*((long double)p.y-q.y)+((long double)p.z-q.z)*((long double)p.z-q.z)));
   if(!(std::abs(Orientation(pa,pb,points.at(r0),points.at(rm)))<=1e-12L*scale*scale*scale))return false;}
  ++report.trials;
  const double oldMin=MinScore(shell);const size_t m=order.size();
  const Node A{a,pa},B{b,pb};std::vector<Node> r(m);for(size_t i=0;i<m;++i)r[i]={order[i],points.at(order[i])};
  std::map<std::array<size_t,3>,double> tri;
  auto triangle=[&](size_t i,size_t k,size_t j){const std::array<size_t,3> key{i,k,j};const auto f=tri.find(key);if(f!=tri.end())return f->second;
    const double s=FreshScore({Cell{0,{A,r[i],r[k],r[j]}},Cell{0,{B,r[i],r[j],r[k]}}});tri.emplace(key,s);return s;};
  std::set<std::pair<size_t,size_t>> forbidden;auto ringEdge=[&](size_t i,size_t j){return j==i+1||(i==0&&j==m-1);};
  if(!Certified(r[0].p,r[m-1].p,MaximumMetricEdge)){++report.rejected_edge;return false;}
  for(unsigned attempt=0;attempt<4;++attempt) {
    std::vector<std::vector<double>> best(m,std::vector<double>(m,-INFINITY));std::vector<std::vector<size_t>> arg(m,std::vector<size_t>(m,SIZE_MAX));
    for(size_t i=0;i+1<m;++i)best[i][i+1]=INFINITY;
    for(size_t len=2;len<m;++len)for(size_t i=0;i+len<m;++i){const size_t j=i+len;
      for(size_t k=i+1;k<j;++k){if((!ringEdge(i,k)&&forbidden.count({i,k}))||(!ringEdge(k,j)&&forbidden.count({k,j})))continue;
        const double v=std::min({best[i][k],best[k][j],triangle(i,k,j)});if(v>best[i][j]){best[i][j]=v;arg[i][j]=k;}}}
    if(!(best[0][m-1]>oldMin+Gain)){if(best[0][m-1]==-INFINITY)++report.rejected_validity;else ++report.rejected_quality;return false;}
    std::vector<std::array<size_t,3>> used;std::vector<std::pair<size_t,size_t>> stack{{0,m-1}};
    while(!stack.empty()){const auto e=stack.back();stack.pop_back();if(e.second<=e.first+1)continue;const size_t k=arg[e.first][e.second];
      used.push_back({e.first,k,e.second});stack.push_back({e.first,k});stack.push_back({k,e.second});}
    bool ok=true;
    for(const auto& t:used)for(const auto& d:{std::make_pair(t[0],t[1]),std::make_pair(t[1],t[2]),std::make_pair(t[0],t[2])}){
      if(ringEdge(d.first,d.second)||forbidden.count(d))continue;if(!Certified(r[d.first].p,r[d.second].p,MaximumMetricEdge)){forbidden.insert(d);ok=false;break;}}
    if(!ok){++report.rejected_edge;if(Limited())return false;continue;}
    std::vector<Cell> fresh;for(const auto& t:used){fresh.push_back(Cell{0,{A,r[t[0]],r[t[1]],r[t[2]]}});fresh.push_back(Cell{0,{B,r[t[0]],r[t[2]],r[t[1]]}});}
    std::map<Face,FacetBinding> added;if(!BoundaryReplacement(shell,fresh,faces,faces[0].binding,added))return false;
    Commit(shell,fresh,UINT64_MAX,added);++report.boundary_flips;return true;
  }
  return false;
}
// O6: boundary vertex p, every incident boundary face bound to one facet (p interior to it), contracted to a neighbour q
// along a boundary edge of that facet; area and orientation of the facet patch are kept.
bool MeshOptimizer::BoundaryCollapse(Id p,Id q) {
  if(Limited()||!fixed.count(p)||options.frozen_vertices.count(p)||keep.count(p)||!Adjacent(p,q))return false;
  const auto sp=star.find(p);if(sp==star.end()||sp->second.size()>MaximumCavityCells)return false;
  const auto old=sp->second;for(Id id:old)if(Frozen(id)){++report.rejected_frozen;return false;}
  const auto faces=BoundaryFacesOf(old,p);if(faces.empty())return false;
  for(const auto& f:faces)if(f.binding.facet!=faces.front().binding.facet)return false;
  bool edge=false;for(const auto& f:faces)edge=edge||f.oriented[0]==q||f.oriented[1]==q||f.oriented[2]==q;if(!edge)return false;
  // p is interior to the facet patch: its boundary faces close a fan around it (every boundary edge at p used twice)
  {std::map<Id,int> uses;for(const auto& f:faces)for(Id x:f.oriented)if(x!=p)++uses[x];for(const auto& u:uses)if(u.second!=2)return false;
   // the fan (p and its boundary ring) is planar within 1e-12 relative: the contracted faces stay in the facet's plane
   const auto& f0=faces.front().oriented;long double scale=0;for(const auto& u:uses)scale=std::max(scale,(long double)Norm3(points.at(u.first),points.at(p)));
   for(const auto& u:uses)if(!(std::abs(Orientation(points.at(f0[0]),points.at(f0[1]),points.at(f0[2]),points.at(u.first)))<=1e-12L*scale*scale*scale))return false;
   if(!(std::abs(Orientation(points.at(f0[0]),points.at(f0[1]),points.at(f0[2]),points.at(p)))<=1e-12L*scale*scale*scale))return false;}
  ++report.trials;
  const Node Q{q,points.at(q)};std::vector<Cell> fresh;std::set<Id> link;
  for(Id id:old){const auto& c=mesh.at(id).cell;for(const auto& v:c.v)if(v.id!=p)link.insert(v.id);if(CellHas(c,q))continue;Cell x=c;for(auto& v:x.v)if(v.id==p)v=Q;fresh.push_back(x);}
  if(fresh.empty())return false;
  const double oldMin=MinScore(old),newMin=FreshScore(fresh);
  if(newMin==-INFINITY){++report.rejected_validity;return false;}
  if(!(newMin>oldMin+Gain)){++report.rejected_quality;return false;}
  for(Id x:link){if(x==q||Adjacent(q,x))continue;if(!Certified(Q.p,points.at(x),MaximumMetricEdge)){++report.rejected_edge;return false;}}
  std::map<Face,FacetBinding> added;if(!BoundaryReplacement(old,fresh,faces,faces.front().binding,added))return false;
  Commit(old,fresh,p,added);++report.boundary_collapses;return true;
}
// DC-13: replace the unfrozen cavity `old` by the cone from one new vertex placed by the min-max search (proposal with
// vertex-mean tensors), accepted on the exact checks: every cavity vertex kept, exact positivity and validity of every
// fresh cell, strict max-min progress (Shell/Neighbourhood) or min >= min(old, 1) (Length), ValidateFixedInterface and
// the edge guard on every new edge (limit max(1.8, removed) or, for Length, max(1.8, .75 removed)).
bool MeshOptimizer::ConeReplace(const std::vector<Id>& old,Point start,Insert kind,const std::pair<Id,Id>* split,long double removed) {
  if(Limited()||old.empty()||old.size()>MaximumCavityCells)return false;
  for(Id id:old)if(Frozen(id)){++report.rejected_frozen;return false;}
  ++report.trials;
  std::vector<Cell> cells;std::set<Id> vertices;for(Id id:old){cells.push_back(mesh.at(id).cell);for(const auto& v:cells.back().v)vertices.insert(v.id);}
  const auto faces=CavityBoundary(cells);
  std::function<bool(Point)> admissible;
  if(split){const Point pa=points.at(split->first),pb=points.at(split->second);Tensor m=At(pa);const Tensor mb=At(pb);
    m.xx=(m.xx+mb.xx)/2;m.xy=(m.xy+mb.xy)/2;m.xz=(m.xz+mb.xz)/2;m.yy=(m.yy+mb.yy)/2;m.yz=(m.yz+mb.yz)/2;m.zz=(m.zz+mb.zz)/2;
    long double whole=0;try{whole=MetricNorm(pa,pb,m);}catch(const std::exception&){return false;}
    admissible=[pa,pb,m,whole](Point x){try{return MetricNorm(x,pa,m)<=.6L*whole&&MetricNorm(x,pb,m)<=.6L*whole;}catch(const std::exception&){return false;}};}
  const auto pl=PlaceCone(faces,[&](Id v){return points.at(v);},[&](Point q){return At(q);},start,admissible,options.placement_budget);
  ++report.placements;report.placement_evaluations+=pl.evaluations;
  if(!pl.ok){++report.rejected_validity;return false;}
  const Node X{next_node,pl.x};std::vector<Cell> fresh;std::set<Id> kept;
  for(const auto& f:faces){fresh.push_back(Cell{0,{X,Node{f[0],points.at(f[0])},Node{f[1],points.at(f[1])},Node{f[2],points.at(f[2])}}});for(Id v:f)kept.insert(v);}
  if(kept.size()!=vertices.size()){++report.placement_refusals;return false;}  // a cavity vertex would vanish
  const double oldMin=MinScore(old),newMin=FreshScore(fresh,false);  // trial identity next_node: never memoized
  if(newMin==-INFINITY){++report.rejected_validity;++report.placement_refusals;return false;}
  const double floor=kind==Insert::Length?std::min(oldMin,1.)-1e-12:oldMin+Gain;
  if(!(newMin>floor)){++report.rejected_quality;++report.placement_refusals;return false;}
  const long double limit=kind==Insert::Length?std::max<long double>(MaximumMetricEdge,.75L*removed):std::max<long double>(MaximumMetricEdge,removed);
  for(Id v:vertices)if(!Certified(X.p,points.at(v),limit)){++report.rejected_edge;++report.placement_refusals;return false;}
  if(split) {  // O7L: both halves of the removed edge are integrated exactly and strictly shorter than it
    try{for(Id e:{split->first,split->second})if(!(Exact(X.p,points.at(e))<removed)){++report.rejected_edge;++report.placement_refusals;return false;}}
    catch(const std::exception&){++report.rejected_edge;++report.placement_refusals;return false;}}
  if(!Interface(old,fresh)){++report.placement_refusals;return false;}
  ++next_node;Commit(old,fresh);
  ++(kind==Insert::Shell?report.shell_insertions:kind==Insert::Neighbourhood?report.neighbourhood_insertions:report.length_insertions);
  return true;
}
// O7 / O7L: the shell of edge (a,b) (closed ring for O7L) re-coned from a vertex placed near the edge midpoint.
bool MeshOptimizer::ShellInsert(Id a,Id b,bool lengthDriven) {
  if(Limited()||!options.insertions)return false;
  const auto sa=star.find(a),sb=star.find(b);if(sa==star.end()||sb==star.end())return false;
  auto shell=Common(sa->second,sb->second);if(shell.size()<2||shell.size()>2*options.maximum_shell)return false;
  if(!lengthDriven){shell.erase(std::remove_if(shell.begin(),shell.end(),[&](Id id){return Frozen(id);}),shell.end());if(shell.empty())return false;}  // partial shell: the edge stays
  if(lengthDriven){std::map<Id,int> ring;for(Id id:shell)for(const auto& v:mesh.at(id).cell.v)if(v.id!=a&&v.id!=b)++ring[v.id];
    if(ring.size()!=shell.size())return false;for(const auto& r:ring)if(r.second!=2)return false;}  // closed shell only
  const Point pa=points.at(a),pb=points.at(b);
  const Point mid{double(((long double)pa.x+pb.x)/2),double(((long double)pa.y+pb.y)/2),double(((long double)pa.z+pb.z)/2)};
  long double removed=MaximumMetricEdge;try{const auto e=Estimate(pa,pb);removed=e>EstimatePass?Exact(pa,pb):e;}catch(const std::exception&){return false;}
  const std::pair<Id,Id> edge{a,b};
  return ConeReplace(shell,mid,lengthDriven?Insert::Length:Insert::Shell,lengthDriven?&edge:nullptr,removed);
}
// O8: the bad cell and its face neighbours across interior faces re-coned from a vertex placed near its centroid.
bool MeshOptimizer::NeighbourhoodInsert(Id cell) {
  if(Limited()||!options.insertions||!mesh.count(cell))return false;
  const auto& c=mesh.at(cell).cell;std::vector<Id> old{cell};
  for(auto f:OppositeFaces(c)){const auto both=Common(Common(star.at(f[0]),star.at(f[1])),star.at(f[2]));for(Id id:both)if(id!=cell&&!Frozen(id))old.push_back(id);}  // a frozen neighbour stays: its face bounds the cavity
  if(Frozen(cell))return false;
  return ConeReplace(old,Centroid(c),Insert::Neighbourhood);
}
bool MeshOptimizer::TryCell(Id id,bool enrichment) {
  const Cell c=mesh.at(id).cell;
  if(!enrichment && options.swaps) {
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){if(!mesh.count(id))return true;if(EdgeRemoval(std::min(c.v[i].id,c.v[j].id),std::max(c.v[i].id,c.v[j].id)))return true;}
    for(auto f:OppositeFaces(c)){if(!mesh.count(id))return true;std::sort(f.begin(),f.end());if(FaceSwap(f))return true;}
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){if(!mesh.count(id))return true;
      if(options.boundary&&!options.geometry&&fixed.count(c.v[i].id)&&fixed.count(c.v[j].id)&&BoundaryFlip(c.v[i].id,c.v[j].id))return true;}
  }
  if(enrichment && options.insertions) {  // DC-13 O7 (longest edges first) and O8
    std::vector<std::pair<long double,std::pair<Id,Id>>> edges;
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){long double l=0;try{l=Estimate(c.v[i].p,c.v[j].p);}catch(const std::exception&){}edges.push_back({-l,{c.v[i].id,c.v[j].id}});}
    std::sort(edges.begin(),edges.end());
    for(const auto& e:edges){if(!mesh.count(id))return true;if(ShellInsert(e.second.first,e.second.second))return true;}
    if(!mesh.count(id))return true;if(NeighbourhoodInsert(id))return true;
  }
  if(!enrichment && options.collapses) {
    std::vector<std::pair<long double,std::pair<Id,Id>>> edges;
    for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){long double l=INFINITY;try{l=Estimate(c.v[i].p,c.v[j].p);}catch(const std::exception&){}edges.push_back({l,{c.v[i].id,c.v[j].id}});}
    std::sort(edges.begin(),edges.end());
    for(const auto& e:edges){if(!mesh.count(id))return true;if(Collapse(e.second.first,e.second.second)||Collapse(e.second.second,e.second.first))return true;}
    if(options.boundary&&!options.geometry)for(const auto& e:edges){if(!mesh.count(id))return true;if(BoundaryCollapse(e.second.first,e.second.second)||BoundaryCollapse(e.second.second,e.second.first))return true;}
  }
  return false;
}
bool MeshOptimizer::Run() {
  Index();
  auto census=[&](size_t& bad,double& minimum){bad=0;minimum=INFINITY;for(const auto& row:mesh){if(options.frozen_cells.count(row.first))continue;const double s=RowScore(row.first,row.second.cell);minimum=std::min(minimum,s);bad+=s<1;}if(!std::isfinite(minimum))minimum=1;};
  census(report.bad_before,report.minimum_before);
  while(report.sweeps<options.maximum_sweeps) {
    ++report.sweeps;
    size_t accepted=0;
    const std::array<bool,3> enabled{options.smoothing,options.swaps||options.collapses,options.insertions};
    for(unsigned stage=0;stage<enabled.size();++stage) {
      stage_work_end=options.maximum_work;
      if(!enabled[stage] || Limited())continue;
      const size_t remaining=std::count(enabled.begin()+stage,enabled.end(),true);
      // A phase refusal is temporary: reserve work for every later enabled phase, including enrichment.
      stage_work_end=report.trials+(options.maximum_work-report.trials)/remaining;
      std::vector<std::pair<double,Id>> bad;
      for(const auto& row:mesh){if(Limited())break;if(options.frozen_cells.count(row.first))continue;
        const double s=RowScore(row.first,row.second.cell);if(s<1)bad.emplace_back(s,row.first);}
      std::sort(bad.begin(),bad.end());
      if(stage==0) {
        std::set<Id> seen;std::vector<Id> vertices;
        for(const auto& row:bad)for(const auto& v:mesh.at(row.second).cell.v)if(seen.insert(v.id).second)vertices.push_back(v.id);
        for(Id v:vertices){if(Limited())break;const auto s=star.find(v);if(s==star.end() || s->second.empty())continue;
          if(MinScore(s->second)<1 && Smooth(v))++accepted;}
      } else {
        for(const auto& row:bad){if(Limited())break;if(!mesh.count(row.second))continue;
          if(RowScore(row.second,mesh.at(row.second).cell)<1 && TryCell(row.second,stage==2))++accepted;}
        // Length repair follows the same topology/enrichment ordering as quality repair.
        if((stage==1?options.swaps:options.insertions)&&!Limited()) {
          std::vector<std::pair<long double,std::pair<Id,Id>>> longs;std::set<std::pair<Id,Id>> seen;
          for(const auto& row:mesh){if(Limited())break;if(Frozen(row.first))continue;const auto& c=row.second.cell;
            for(unsigned i=0;i<4;++i)for(unsigned j=i+1;j<4;++j){const auto e=std::minmax(c.v[i].id,c.v[j].id);if(!seen.insert(e).second)continue;
              try{if(Estimate(c.v[i].p,c.v[j].p)<=1.6L)continue;const auto l=Exact(c.v[i].p,c.v[j].p);
                if(l>MaximumMetricEdge)longs.push_back({-l,e});}catch(const std::exception&){}}}
          std::sort(longs.begin(),longs.end());report.long_edges_seen+=longs.size();
          for(const auto& e:longs){if(Limited())break;if(!Adjacent(e.second.first,e.second.second))continue;
            if(stage==1?EdgeRemoval(e.second.first,e.second.second,true):ShellInsert(e.second.first,e.second.second,true))++accepted;}
        }
      }
    }
    stage_work_end=options.maximum_work;
    if(Limited()){report.stop="limit";break;}
    if(!accepted){census(report.bad_after,report.minimum_after);report.stop=report.bad_after?"no progress":"complete";break;}
  }
  if(report.stop.empty())report.stop="sweep cap";
  census(report.bad_after,report.minimum_after);
  report.seconds=std::chrono::duration<double>(Clock::now()-started).count();
  return changed;
}
}  // namespace
bool OptimizeMetricMesh(Owned& mesh,const RegionMetric& target,Id& next_node,Id& next_cell,const OptimizeOptions& options,OptimizeReport& report) {
  auto* reservation=options.geometry?options.geometry->AtlasReservation():nullptr;
  if(options.geometry && options.geometry->Mapped() && (options.geometry->Mobility()||!options.geometry->Parameters()->Coverage().faces.empty()||!options.geometry->Parameters()->Coverage().traces.empty()) && !reservation) {
    report.stop="atlas reservation missing";return false;
  }
  const auto before=reservation?reservation->work:0;MeshOptimizer optimizer(mesh,target,next_node,next_cell,options,report);
  const bool changed=optimizer.Run();if(reservation){report.atlas_work=reservation->work-before;report.atlas_peak_bytes=reservation->peak_bytes;}return changed;
}
bool PlaceConeInsertion(const std::vector<Cell>& cavity,Point start,Id node,Id first_cell,const RegionMetric& target,
                        std::vector<Cell>& fresh,double& score,const Edge* split_edge,double split_ratio,size_t budget,
                        Id replaced,const std::function<bool(Point)>& region) {
  fresh.clear();score=-INFINITY;if(cavity.empty())return false;
  std::map<Id,Point> points;for(const auto& c:cavity)for(const auto& v:c.v)points[v.id]=v.p;
  std::map<std::array<double,3>,Tensor> memo;
  auto at=[&](Point p){const std::array<double,3> k{p.x,p.y,p.z};const auto f=memo.find(k);if(f!=memo.end())return f->second;const auto m=target.point(p);memo.emplace(k,m);return m;};
  const auto faces=CavityBoundary(cavity);
  std::function<bool(Point)> admissible;
  if(split_edge) {
    const auto fa=points.find((*split_edge)[0]),fb=points.find((*split_edge)[1]);if(fa==points.end()||fb==points.end())return false;
    const Point pa=fa->second,pb=fb->second;Tensor m=at(pa);const Tensor mb=at(pb);
    m.xx=(m.xx+mb.xx)/2;m.xy=(m.xy+mb.xy)/2;m.xz=(m.xz+mb.xz)/2;m.yy=(m.yy+mb.yy)/2;m.yz=(m.yz+mb.yz)/2;m.zz=(m.zz+mb.zz)/2;
    long double whole=0;try{whole=MetricNorm(pa,pb,m);}catch(const std::exception&){return false;}
    const long double limit=split_ratio*whole;
    admissible=[pa,pb,m,limit](Point x){try{return MetricNorm(x,pa,m)<=limit&&MetricNorm(x,pb,m)<=limit;}catch(const std::exception&){return false;}};
  }
  if(region){auto split=admissible;admissible=[split,&region](Point x){return (!split||split(x))&&region(x);};}
  const auto pl=PlaceCone(faces,[&](Id v){return points.at(v);},at,start,admissible,budget);
  if(!pl.ok)return false;
  const Node X{node,pl.x};std::set<Id> kept;
  for(size_t i=0;i<faces.size();++i){const auto& f=faces[i];fresh.push_back(Cell{first_cell+i,{X,Node{f[0],points.at(f[0])},Node{f[1],points.at(f[1])},Node{f[2],points.at(f[2])}}});for(Id v:f)kept.insert(v);}
  if(kept.size()+(points.count(replaced)&&!kept.count(replaced)?1:0)!=points.size()){fresh.clear();return false;}  // a cavity vertex would vanish
  double low=INFINITY;
  for(const auto& c:fresh) {
    if(!(Orientation(c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p)>0)){fresh.clear();return false;}
    MetricMeasures m;try{m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},at(Centroid(c)));}catch(const std::exception&){fresh.clear();return false;}
    if(!std::isfinite(m.mean_ratio) || !std::isfinite(m.minimum_scaled_jacobian)){fresh.clear();return false;}
    low=std::min(low,std::min(m.mean_ratio/MinimumMeanRatio,m.minimum_scaled_jacobian/MinimumScaledJacobian));
  }
  score=low;return true;
}
EngineResult RefineOptimize(World& world,const Owned& accepted,const std::vector<DonorCell>& original,const PlanarReference& geometry,
                            MetricComposition composition,EngineOptions options,OptimizeOptions optimize,RefineOptimizeReport* report,size_t maximum_rounds) {
  struct Restore{ShapeAdmission admission=GetShapeAdmission();~Restore(){SetShapeAdmission(admission);}} restore;
  RefineOptimizeReport local;auto& rep=report?*report:local;rep=RefineOptimizeReport{};
  if(world.size!=1) {
    auto r=Refine(world,accepted,original,geometry,std::move(composition),options);
    r.reason="Native3D RefineOptimize is serial only; plain Refine under the caller's shape admission. "+r.reason;return r;
  }
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  const auto started=Clock::now();auto elapsed=[&]{return std::chrono::duration<double>(Clock::now()-started).count();};
  Owned current=accepted;std::shared_ptr<const PlanarReference> held;const PlanarReference* geo=&geometry;
  EngineResult last;bool optimizedLast=false;
  for(size_t round=0;round<maximum_rounds;++round) {
    EngineOptions o=options;
    if(options.maximum_seconds>0){const double left=options.maximum_seconds-elapsed();if(left<=0)break;o.maximum_seconds=std::max(1.,.5*left);}
    const auto r0=Clock::now();
    last=Refine(world,current,original,*geo,composition,o);optimizedLast=false;
    rep.refine_seconds+=std::chrono::duration<double>(Clock::now()-r0).count();++rep.rounds;
    rep.stops.push_back(EngineStopName(last.stats.stop));rep.cells.push_back(last.cells.size());
    if(last.valid || last.cells.empty())return last;
    if(last.reference){held=last.reference;geo=held.get();}
    Owned cells=last.cells;Id nextNode=1,nextCell=1;
    OptimizeOptions opt=optimize;
    if(geo->Mapped())opt.boundary=false;  // DC-13: boundary faces change only through the parameter publication of Refine
    opt.diagnostic_sliding.clear();  // the TEST-ONLY sliding diagnostic is never applied in production
    for(const auto& row:cells) {
      nextCell=std::max(nextCell,row.first+1);for(const auto& n:row.second.cell.v)nextNode=std::max(nextNode,n.id+1);
      for(unsigned k=0;k<4;++k)if(row.second.physical[k].present && options.wall_heights.count(geo->Get(row.second.physical[k].facet).marker)) {
        opt.frozen_cells.insert(row.first);for(const auto& n:row.second.cell.v)opt.frozen_vertices.insert(n.id);}
    }
    if(options.maximum_seconds>0){const double left=options.maximum_seconds-elapsed();if(left<=0)break;opt.maximum_seconds=opt.maximum_seconds>0?std::min(opt.maximum_seconds,left):left;}
    const auto o0=Clock::now();OptimizeReport orep;bool moved=false;
    DonorImport source(world,false);std::string why;
    if(!source.Build(original,why)){last.reason="Native3D RefineOptimize original source: "+why+"; "+last.reason;return last;}
    {RegionFieldService service(world,source,*geo,composition,options.edge_breakpoints,false,options.memory_ceiling,0);
     service.Run([&](const RegionMetric& target){moved=OptimizeMetricMesh(cells,target,nextNode,nextCell,opt,orep);});}
    rep.optimize_seconds+=std::chrono::duration<double>(Clock::now()-o0).count();rep.optimize.push_back(orep);
    // a budget-limited Refine round that still changed the mesh continues even when the optimizer finds nothing to do
    const bool refining=(last.stats.stop==EngineStop::TimeBudget||last.stats.stop==EngineStop::WorkBudget||last.stats.stop==EngineStop::RoundBudget)&&
                        (last.cells.size()!=current.size()||last.stats.accepted>0);
    if(!moved&&!refining){last.reason="Native3D RefineOptimize: the optimizer changed nothing after Refine round "+std::to_string(round)+"; "+last.reason;return last;}
    current=std::move(cells);optimizedLast=true;
  }
  if(optimizedLast){last.cells=current;last.valid=false;}
  last.reason="Native3D RefineOptimize stopped at its round/time budget; "+last.reason;return last;
}
}  // namespace SU2Native3D
