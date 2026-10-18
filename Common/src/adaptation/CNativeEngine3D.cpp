/*!
 * \file CNativeEngine3D.cpp
 * \brief Incremental frozen-target selection and distributed private planar refinement/coarsening.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeEngine3D.hpp"
#include "../../include/adaptation/CNativeRegionField3D.hpp"
#include "../../include/adaptation/CNativePointConstruction3D.hpp"
#include <chrono>
#include <cmath>
#include <ctime>
#include <set>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <stdexcept>
namespace SU2Native3D {
namespace {
using Clock = std::chrono::steady_clock;
class Timer {
 public:
  Timer(double& value, bool enabled) : value(value), enabled(enabled) { if (enabled) start = Clock::now(); }
  ~Timer() { if (enabled) value += std::chrono::duration<double>(Clock::now() - start).count(); }
 private:
  double& value;
  bool enabled;
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
Patch GeometryPatch(const Imported& imported) {
  Patch patch;
  for (const auto& row : imported.cells) {
    patch.cells.push_back(row.cell);
    const auto faces = Faces(row.cell);
    for (size_t i = 0; i < 4; ++i) if (row.physical[i].present) patch.surface.push_back({faces[i],row.physical[i].facet});
  }
  return patch;
}
bool Same(Point a, Point b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
bool DonorCap(const std::string& reason) {
  return reason=="Original donor import exceeds patch budget." || reason=="Original donor union exceeds patch budget.";
}
using RepairFeature = std::pair<unsigned,std::array<Id,3>>;
double SurfaceTolerance(const PlanarReference& geometry,Id facet,double configured,const std::map<int,double>& heights) {
  const auto height=heights.find(geometry.Get(facet).marker);
  return height!=heights.end() && geometry.Analytic().count(geometry.Canonical(facet)) ?
      std::min(configured,.5*height->second) : configured;
}
long double GeometryExcess(const Patch& patch,const PlanarReference& geometry,double tolerance,const std::map<int,double>& heights) {
  std::map<Id,Point> nodes;
  for(const auto& c:patch.cells)for(auto n:c.v)nodes.emplace(n.id,n.p);
  long double excess=0;
  for(const auto& f:patch.surface) {
    const std::array<Point,3> p{nodes.at(f.v[0]),nodes.at(f.v[1]),nodes.at(f.v[2])};
    const long double error=std::max(0.,geometry.SurfaceError(f.facet,p)-SurfaceTolerance(geometry,f.facet,tolerance,heights));
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
bool Clearance(World& world,const Owned& working,const Patch& old,const Patch& fresh,bool& candidate,
               std::string& reason,EngineStats& stats,size_t ceiling,size_t caller) {
  const size_t scratch=transfer_memory::Add(caller,65536,transfer_memory::GrowthBound(working.size(),sizeof(DonorCell)));
  CLocalFailure failure;
  auto vote=[&]() {const auto elected=world.elect(failure);if(elected.any)reason=elected.message;return !elected.any;};
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,scratch);
  if(scratch==SIZE_MAX || scratch>ceiling)failure.Set(3,0,"Native3D collision preparation exceeds storage budget.");
  if(!vote())return false;
  std::vector<DonorCell> current;std::set<Id> removed;
  std::string rejected;
  try {
    current.reserve(working.size());for(const auto& row:working)current.push_back(DonorCell{row.second.cell});
    for(const auto& cell:old.cells)removed.insert(cell.id);
    if(candidate)for(size_t i=0;i<fresh.cells.size();++i)for(size_t j=i+1;j<fresh.cells.size();++j)
      if(!ConformingDisjoint(fresh.cells[i],fresh.cells[j])) {
        candidate=false;rejected="Native3D curved replacement overlaps or has nonconforming contact.";break;
      }
  }catch(const std::exception& e){failure.Set(2,0,e.what());}
  if(!vote())return false;
  DonorImport collision(world);
  if(!collision.Build(current,reason,ceiling,scratch))return false;
  const size_t count=CPassiveComm::Allreduce(candidate ? fresh.cells.size() : 0,CPassiveComm::Op::MAX,world.comm);
  for(size_t at=0;at<count;) {
    size_t batch=std::min<size_t>(16,count-at);DonorPatch neighbors;
    for(;;) {
      std::vector<Cell> query;
      try {if(candidate)for(size_t i=at;i<std::min(at+batch,fresh.cells.size());++i)query.push_back(fresh.cells[i]);}
      catch(const std::exception& e){failure.Set(2,0,e.what());}
      if(!vote())return false;
      neighbors=collision.ImportCells(query,ceiling,scratch);
      if(neighbors.valid)break;
      if(!DonorCap(neighbors.reason)){reason=neighbors.reason;return false;}
      if(batch==1){candidate=false;rejected=neighbors.reason;break;}
      batch=(batch+1)/2;
    }
    if(candidate)try {
      for(const auto& neighbor:neighbors.cells)if(!removed.count(neighbor.cell.id))
        for(size_t i=at;i<std::min(at+batch,fresh.cells.size());++i)
          if(!ConformingDisjoint(fresh.cells[i],neighbor.cell)) {
            candidate=false;rejected="Native3D projected candidate collides with unedited cell "+std::to_string(neighbor.cell.id)+".";break;
          }
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote())return false;
    at+=batch;
  }
  stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,collision.Statistics().maximum_work_bytes);
  if(!rejected.empty())reason=rejected;
  return true;
}
struct Choice { Edge edge{}; EdgeMeasure measure; bool active = false; Feature feature; bool geometric=false; };
struct ProposalKey {
  Edge edge{};int rank=0;size_t slot=0;
  template<class S> void Fields(S& s){s(edge,rank,slot);}
};
class Selection {
 public:
  Selection(bool coarsening, bool repairing, const PlanarReference& geometry, EngineStats& stats, bool timings, double tolerance,const std::map<int,double>& heights)
      : coarsening(coarsening), track_repairs(repairing), repair_event(repairing), geometry(geometry), stats(stats), timings(timings), tolerance(tolerance), heights(heights) {}
  bool coarsening, track_repairs, repair_event, prune_collapses = false, track_splits = false;
  const PlanarReference& geometry;
  EngineStats& stats;
  bool timings;
  double tolerance;
  const std::map<int,double>& heights;
  std::set<std::tuple<long double,Id,Edge>> geometry_queue;
  long double Priority(long double length) const { return coarsening ? length : -length; }
  bool Candidate(long double length) const {
    return coarsening ? length < CoarseningMetricEdge : length > MaximumMetricEdge;
  }
  void Queue(Edge edge, long double length, bool insert) {
    if (repair_event || !Candidate(length)) return;
    for (unsigned i=0; i<(coarsening ? 2u : 1u); ++i) {
      Edge key=edge; std::sort(key.begin(),key.end());
      if (insert && prune_collapses && edges.at(key).forbidden[i])
        stats.collapse_filtered += queue.erase({Priority(length),edge});
      else if (insert) queue.insert({Priority(length),edge}); else queue.erase({Priority(length),edge});
      std::swap(edge[0],edge[1]);
    }
  }
  size_t invalid = 0, repair_bytes = 0;
  struct EdgeState { EdgeMeasure measure; size_t users = 0; std::array<size_t,2> forbidden{}; };
  struct CellState { std::array<Edge, 6> edges{}; std::array<unsigned char,6> forbidden{}; std::array<Id,4> vertices{}; std::array<long double,6> geometry_scores{}; bool valid = false; };
  std::map<Id,CellState> cells;
  std::map<Edge,EdgeState> edges;
  std::set<std::pair<long double,Edge>> queue;
  std::map<Id,std::set<Id>> vertex_cells, blocked_edges;
  size_t vertex_links=0, blocked_links=0;
  std::map<Id,std::vector<RepairFeature>> cell_repairs;
  std::map<RepairFeature,size_t> repair_users;
  std::set<RepairFeature> repair_queue;
  size_t Bytes() const {
    return transfer_memory::Add(MapBytes(cells),MapBytes(edges),MapBytes(queue),MapBytes(cell_repairs),MapBytes(repair_users),MapBytes(repair_queue),repair_bytes,
        MapBytes(geometry_queue),MapBytes(vertex_cells),MapBytes(blocked_edges),transfer_memory::Mul(transfer_memory::Add(vertex_links,blocked_links),sizeof(Id)+4*sizeof(void*)));
  }
  Choice Best(bool repairing) const {
    if (repairing) {
      if (repair_queue.empty()) return {};
      const auto& key=*repair_queue.begin();
      Choice choice; choice.active=true; choice.feature={key.second,key.first}; return choice;
    }
    if (!geometry_queue.empty()) {
      const auto key=std::get<2>(*geometry_queue.begin());
      return {key,edges.at(key).measure,true,{},true};
    }
    if (queue.empty()) return {};
    const auto key = queue.begin()->second;
    Edge canonical=key; std::sort(canonical.begin(),canonical.end());
    return {key,edges.at(canonical).measure,true,{}};
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
    if (repairing) { repair_queue.erase({feature.count,feature.vertices}); return; }
    Edge canonical=edge;std::sort(canonical.begin(),canonical.end());
    for(auto it=geometry_queue.begin();it!=geometry_queue.end();) {
      if(std::get<2>(*it)==canonical)it=geometry_queue.erase(it);else ++it;
    }
    const auto it = edges.find(canonical);
    if (it==edges.end()) return;
    queue.erase({Priority(it->second.measure.length),edge});
    if (coarsening || track_splits) blocked_links+=blocked_edges[edge[1]].insert(edge[0]).second;
    if (track_splits && !coarsening) blocked_links+=blocked_edges[edge[0]].insert(edge[1]).second;
  }
  void Forget(Edge edge) {
    const auto found=blocked_edges.find(edge[1]);
    if (found==blocked_edges.end()) return;
    blocked_links-=found->second.erase(edge[0]);
    if (found->second.empty()) blocked_edges.erase(found);
  }
  void Touch(Id vertex) {
    const auto found=blocked_edges.find(vertex);
    if (found==blocked_edges.end()) return;
    for (Id retained:found->second) {
      Edge edge{retained,vertex}, key=edge; std::sort(key.begin(),key.end());
      const auto measured=edges.find(key);
      if (measured==edges.end() || !Candidate(measured->second.measure.length)) continue;
      const auto direction=retained==key[0] ? 0u : 1u;
      if (prune_collapses && measured->second.forbidden[direction]) continue;
      if (!coarsening) edge=key;
      const bool inserted=queue.insert({Priority(measured->second.measure.length),edge}).second;
      if (coarsening) stats.collapse_reactivated+=inserted;
    }
    blocked_links-=found->second.size(); blocked_edges.erase(found);
  }
  bool FilterCollapse(const Choice& choice, const Owned& working) {
    if (!coarsening || !prune_collapses || !choice.active) return false;
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
    invalid -= !found->second.valid;
    for(size_t i=0;i<6;++i)geometry_queue.erase({-found->second.geometry_scores[i],id,found->second.edges[i]});
    for (Id vertex:found->second.vertices) {
      auto& star=vertex_cells.at(vertex);
      vertex_links-=star.erase(id);
      if (star.empty()) vertex_cells.erase(vertex);
    }
    const auto repairs=cell_repairs.find(id);
    if (repairs!=cell_repairs.end()) {
      repair_bytes-=transfer_memory::Bytes(repairs->second);
      for (auto key:repairs->second) {
        repair_queue.erase(key);
        if (--repair_users.at(key)==0) repair_users.erase(key); else repair_queue.insert(key);
      }
      cell_repairs.erase(repairs);
    }
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
  void EnableCollapseFilter() {
    prune_collapses=true;
    for (const auto& row:edges) Queue(row.first,row.second.measure.length,true);
  }
  bool LeavesFacet(const OwnedCell& owned, Id removed, Point retained) {
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
  void Add(const OwnedCell& owned, const PatchMetric& metric, bool valid) {
    const auto& cell=owned.cell;
    if (metric.edges.size()!=6 || !std::isfinite(metric.maximum_length))
      throw std::runtime_error("Incomplete native 3D initial/updated metric measurement.");
    CellState row; row.valid=valid;
    std::map<Edge,long double> geometry_scores;
    if (!geometry.Analytic().empty()) {
      const auto faces=Faces(cell);
      for(size_t slot=0;slot<4;++slot)if(owned.physical[slot].present) {
        std::array<Point,3> triangle{};
        for(size_t i=0;i<3;++i)for(auto n:cell.v)if(n.id==faces[slot][i])triangle[i]=n.p;
        const double error=geometry.SurfaceError(owned.physical[slot].facet,triangle);
        const auto allowed=SurfaceTolerance(geometry,owned.physical[slot].facet,tolerance,heights);
        if(error<=allowed)continue;
        row.valid=false;
        for(size_t i=0;i<3;++i)for(size_t j=i+1;j<3;++j) {
          const auto a=triangle[i],b=triangle[j];
          const Edge key{std::min(faces[slot][i],faces[slot][j]),std::max(faces[slot][i],faces[slot][j])};
          long double progress=-INFINITY;
          try {
            const Point middle{double((static_cast<long double>(a.x)+b.x)/2),double((static_cast<long double>(a.y)+b.y)/2),double((static_cast<long double>(a.z)+b.z)/2)};
            const auto projected=geometry.Project(owned.physical[slot].facet,middle);
            auto left=triangle,right=triangle;left[j]=projected;right[i]=projected;
            const auto before=GeometryExcess(Patch{{cell},{{faces[slot],owned.physical[slot].facet}}},geometry,tolerance,heights);
            const double l=std::max(0.,geometry.SurfaceError(owned.physical[slot].facet,left)-allowed);
            const double r=std::max(0.,geometry.SurfaceError(owned.physical[slot].facet,right)-allowed);
            auto area=[](const std::array<Point,3>& t) {
              const long double ax=static_cast<long double>(t[1].x)-t[0].x,ay=static_cast<long double>(t[1].y)-t[0].y,az=static_cast<long double>(t[1].z)-t[0].z;
              const long double bx=static_cast<long double>(t[2].x)-t[0].x,by=static_cast<long double>(t[2].y)-t[0].y,bz=static_cast<long double>(t[2].z)-t[0].z;
              return std::hypot(ay*bz-az*by,az*bx-ax*bz,ax*by-ay*bx);
            };
            progress=before-l*l*area(left)-r*r*area(right);
          }catch(const std::exception&) {} // Complete-star reconstruction remains the authoritative feasibility check.
          if(progress>0)geometry_scores[key]+=progress;
        }
      }
    }
    for (size_t i=0;i<4;++i) {
      row.vertices[i]=cell.v[i].id;
      vertex_links+=vertex_cells[row.vertices[i]].insert(cell.id).second;
    }
    size_t slot=0;
    for (const auto& measured : metric.edges) {
      const auto key=measured.first;
      row.geometry_scores[slot]=geometry_scores[key];
      if(!coarsening && row.geometry_scores[slot]>0)
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
    const auto inserted=cells.emplace(cell.id,std::move(row));
    if (!inserted.second) throw std::logic_error("Duplicate native 3D cached cell.");
    invalid += !inserted.first->second.valid;
    if (track_repairs && !inserted.first->second.valid) AddRepairs(cell);
  }
  void AddRepairs(const Cell& cell) {
    auto& repairs=cell_repairs[cell.id];
    if (!repairs.empty()) throw std::logic_error("Duplicate native 3D repair queue seed.");
    for (auto node:cell.v) repairs.push_back({1,{node.id,0,0}});
    for (auto edge:cells.at(cell.id).edges) repairs.push_back({2,{edge[0],edge[1],0}});
    for (auto face:Faces(cell)) repairs.push_back({3,Key(face)});
    for (auto key:repairs) { ++repair_users[key]; repair_queue.insert(key); }
    repair_bytes=transfer_memory::Add(repair_bytes,transfer_memory::Bytes(repairs));
  }
  void Recover(const Owned& working) {
    track_repairs=true;
    for (const auto& row:cells) if (!row.second.valid) AddRepairs(working.at(row.first).cell);
  }
};
struct BlockedEdge {
  Edge edge{}; Feature feature;
  template <class S> void Fields(S& s) { s(edge,feature); }
};
void FieldCounts(EngineStats& stats, const FrozenField& field) {
  const auto& f=field.Statistics();
  stats.field_requests+=f.requests; stats.field_hits+=f.hits; stats.field_misses+=f.misses;
  stats.field_invalid_queries+=f.invalid_queries; stats.field_failures+=f.failures; stats.field_evictions+=f.evictions;
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
EngineResult Adapt(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                   const PlanarReference& geometry, MetricComposition composition, EngineOptions options, MetricChange intent) {
  const bool curved=!geometry.Analytic().empty();
  const bool coarsening=intent==MetricChange::Coarsen;
  bool repairing=intent==MetricChange::Repair, recovered=false;
  const auto tracked_bytes_begin=world.bytes_sent;
  const auto tracked_calls_begin=world.collective_calls;
  const double tracked_seconds_begin=world.collective_seconds;
  const auto started=Clock::now();
  const auto started_cpu=options.timings ? std::clock() : std::clock_t{};
  EngineResult result;
  auto& stats=result.stats;
  Owned working;
  Selection selection(coarsening,repairing,geometry,stats,options.timings,options.surface_tolerance,options.wall_heights);
  std::map<Edge,EdgeMeasure> retired_edges;
  std::shared_ptr<const FrozenMetricIdentity> field_identity;
  // Joint split feasibility depends on neighboring vertex stars, even when the edge itself is unchanged.
  selection.track_splits=bool(composition);
  Directory directory(world,coarsening || selection.track_splits);
  DonorImport source(world,options.timings);
  size_t accepted_bytes=MapBytes(accepted);
  auto selection_bound=[&](bool repairs) { return coarsening ? (repairs ? 6144u : 4096u) : (repairs ? 5120u : 3072u); };
  std::vector<Id> initial;
  auto vote=[&](const CLocalFailure& local) {
    const auto elected=world.elect(local);
    if (elected.any) result.reason=elected.message;
    return !elected.any;
  };
  size_t batch_bytes=0;
  auto bytes=[&]() {
    return transfer_memory::Add(options.caller_bytes,MapBytes(options.wall_heights),MapBytes(options.region_walls),accepted_bytes,batch_bytes,MapBytes(working),
        transfer_memory::Bytes(original),transfer_memory::Bytes(initial),transfer_memory::Bytes(result.splits),transfer_memory::Bytes(result.collapses),
        selection.Bytes(),MapBytes(retired_edges),field_identity ? 64 : 0);
  };
  auto admit=[&](size_t extra) {
    const auto peak=transfer_memory::Add(bytes(),source.ResidentBytes(),directory.ResidentBytes(),extra);
    stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
    CLocalFailure failure;
    if (peak==SIZE_MAX || peak>options.memory_ceiling) failure.Set(3,0,"Native 3D adaptation working storage exceeds budget.");
    return vote(failure);
  };
  auto coalesce=[&](std::vector<Choice>& choices) {
    if(!coarsening || repairing || curved || composition || world.size==1)return true;
    // A replicated directed collapse is one proposal; different retained endpoints remain distinct.
    const auto scratch=transfer_memory::Mul(transfer_memory::Mul(world.size,MaximumCandidateBatch),
        8*sizeof(ProposalKey)+8*sizeof(void*));
    if(!admit(scratch))return false;
    CLocalFailure failure;std::vector<std::vector<ProposalKey>> to;
    try {
      if(choices.size()>MaximumCandidateBatch)throw std::logic_error("Native collapse proposal table exceeds its bound.");
      std::vector<ProposalKey> local;
      for(size_t i=0;i<choices.size();++i)local.push_back({choices[i].edge,world.rank,i});
      to.assign(world.size,local);
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return false;bool admitted=false;
    const auto all=world.exchange(to,&admitted,options.memory_ceiling,
        transfer_memory::Add(bytes(),source.ResidentBytes(),directory.ResidentBytes(),scratch));
    if(!admitted){result.reason="Native collapse proposal exchange exceeds storage.";return false;}
    try {
      std::vector<Choice> unique;
      for(size_t i=0;i<choices.size();++i) {
        const auto owner=std::make_pair(world.rank,i);bool duplicate=false;
        for(const auto& key:all) {
          if(key.rank<0 || key.rank>=world.size || key.slot>=MaximumCandidateBatch || key.edge[0]==key.edge[1])
            throw std::logic_error("Invalid native collapse proposal identity.");
          duplicate=duplicate || (key.edge==choices[i].edge && std::make_pair(key.rank,key.slot)<owner);
        }
        if(duplicate)++stats.proposal_duplicates;else unique.push_back(choices[i]);
      }
      choices.swap(unique);
    }catch(const std::exception& e){failure.Set(2,0,e.what());}
    return vote(failure);
  };
  std::string last_rejection;
  uint64_t progress_at_window=0, attempted_at_window=0;
  auto height_check=[&](const Patch& patch,std::string& reason) {
    if(options.wall_heights.empty())return true;
    ++stats.height_checks;FirstHeightReport report;
    const bool valid=CheckFirstHeight(patch,options.wall_heights,geometry,report,reason);
    stats.height_rejected+=report.height_failures || report.missing_cells;
    stats.footprint_rejected+=report.footprint_failures!=0;return valid;
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
  const bool observe_rejections=world.maximum(bool(options.rejected_trial));
  const bool observe_state=world.maximum(bool(options.rejected_state));
  bool layer_active=false,region_active=false;Clock::time_point layer_started,region_started;
  auto finish=[&]() {
    if(region_active && options.timings)stats.reconstruction_seconds+=std::chrono::duration<double>(Clock::now()-region_started).count();
    region_active=false;
    if(layer_active && options.timings)stats.layer_seconds+=std::chrono::duration<double>(Clock::now()-layer_started).count();
    layer_active=false;
    if(result.valid && !options.wall_heights.empty()) {
      CLocalFailure heightFailure;
      for(const auto& row:working) {
        std::string reason;
        if(!height_check(GeometryPatch(Imported{{row.second},true,{}}),reason))heightFailure.Set(1,row.first,reason);
      }
      if(!vote(heightFailure)){result.valid=false;stats.stop=EngineStop::Failure;}
    }
    if(!result.valid)result.rebuilt_domain=false;
    if (stats.stop==EngineStop::None) stats.stop=result.valid ? EngineStop::Complete : EngineStop::Failure;
    if(observe_state) {
      CLocalFailure observed;
      if(!result.valid && options.rejected_state)try {options.rejected_state(working,result.reason);}
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
    stats.directory_elections=directory.Statistics().elections;
    stats.directory_election_seconds=directory.Statistics().election_seconds;
    stats.changed_star_scans=directory.Statistics().change_scanned;
    stats.changed_vertices_sent=directory.Statistics().change_sent;
    stats.changed_vertices_received=directory.Statistics().change_received;
    stats.change_notification_seconds=directory.Statistics().change_seconds;
    stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,directory.Statistics().maximum_work_bytes);
    const auto& donor = source.Statistics();
    stats.donor_candidates = donor.index_candidates; stats.donor_filtered = donor.filtered; stats.donor_box_filtered = donor.box_filtered;
    stats.donor_payloads = donor.payloads; stats.donor_reused = donor.reused_candidates; stats.donor_maximum_patch = donor.maximum_patch;
    stats.donor_search_seconds = donor.search_seconds; stats.donor_filter_seconds = donor.filter_seconds;
    stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, donor.maximum_work_bytes);
    result.cells.swap(working);
    if (!result.valid && !last_rejection.empty()) result.reason += " Last local rejection: " + last_rejection;
    if (options.timings) {
      stats.process_cpu_seconds=double(std::clock()-started_cpu)/CLOCKS_PER_SEC;
      stats.total_seconds=std::chrono::duration<double>(Clock::now()-started).count();
      const double phases=stats.initialization_seconds+stats.seed_seconds+stats.selection_seconds+
          stats.dependency_seconds+stats.reservation_seconds+stats.donor_seconds+stats.reconstruction_seconds+
          stats.metric_seconds+stats.routing_seconds+stats.publication_seconds+stats.update_seconds+stats.completion_seconds;
      stats.other_seconds=stats.total_seconds-phases-stats.layer_seconds;
    }
    return std::move(result);
  };
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
    const auto batch_lo=CPassiveComm::Allreduce(options.candidate_batch,CPassiveComm::Op::MIN,world.comm);
    const auto batch_hi=CPassiveComm::Allreduce(options.candidate_batch,CPassiveComm::Op::MAX,world.comm);
    if (batch_lo!=batch_hi || !batch_hi || batch_hi>MaximumCandidateBatch)
      failure.Set(1,0,"Native 3D candidate batch must be bounded and equal across workers.");
    const auto mode_lo=CPassiveComm::Allreduce(int(intent),CPassiveComm::Op::MIN,world.comm);
    const auto mode_hi=CPassiveComm::Allreduce(int(intent),CPassiveComm::Op::MAX,world.comm);
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
    if (ready) ready=admit(transfer_memory::Add(accepted_bytes,64,
        transfer_memory::Mul(std::min<uint64_t>(stats.work_limit,4096),
            coarsening ? sizeof(Edge) : sizeof(SplitRecord)),transfer_memory::Mul(accepted.size(),selection_bound(repairing))));
    if (ready) try {
      field_identity=std::make_shared<const FrozenMetricIdentity>();
      working=accepted;
      stats.maximum_owned=working.size();
      if (coarsening) result.collapses.reserve(std::min<uint64_t>(stats.work_limit,4096));
      else if (!repairing) result.splits.reserve(std::min<uint64_t>(stats.work_limit,4096));
      initial.reserve(working.size());
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
      ready=directory.Build(working,result.reason,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes()));
    }
    if (ready) ready=source.Build(original,result.reason,options.memory_ceiling,
        transfer_memory::Add(bytes(),directory.ResidentBytes()));
  }
  if (!ready) return finish();
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
    std::vector<DonorRegion> regions;Patch old,fresh;std::vector<OwnedCell> replacement;bool complete=false,core_declined=false;
    try {if(!world.rank){
      std::sort(rows.begin(),rows.end(),[](const OwnedCell& a,const OwnedCell& b){return a.cell.id<b.cell.id;});
      old=GeometryPatch(Imported{rows,true,{}});Point lo=rows.front().cell.v[0].p,hi=lo;
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
        options.memory_ceiling,transfer_memory::Add(bytes(),scratch,directory.ResidentBytes()),
        [&](const FrozenField& field){FieldCounts(stats,field);},tick);
    try {service.Run([&](const RegionMetric& target){
      bool already_valid=GeometryExcess(old,geometry,options.surface_tolerance,options.wall_heights)==0;
      bool short_edges=false;
      std::string why;
      if(already_valid)for(const auto& row:rows)if(!height_check(GeometryPatch(Imported{{row},true,{}}),why)){already_valid=false;break;}
      if(already_valid)for(const auto& c:old.cells){
        const auto m=Measure({c.v[0].p,c.v[1].p,c.v[2].p,c.v[3].p},target.point(Centroid(c)));
        if(m.mean_ratio<MinimumMeanRatio || m.minimum_scaled_jacobian<MinimumScaledJacobian){already_valid=false;break;}
        for(unsigned a=0;a<4 && already_valid;++a)for(unsigned b=a+1;b<4;++b) {
          const auto length=target.edge(c.v[a].p,c.v[b].p);
          if(length>MaximumMetricEdge){already_valid=false;break;}
          short_edges=short_edges || (coarsening && length<CoarseningMetricEdge);
        }
        if(!already_valid)break;
      }
      if(already_valid && !short_edges)complete=true;
      else {
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
            maximum_node+1,maximum_cell+1,target,limits,fresh,report,result.reason);
        if(!constructed) {
          // A height-free cone cannot resolve every interior field; retain the existing volume operators there.
          core_declined=options.region_walls.empty() && !report.exhausted_work &&
              (report.unresolved_core || report.minimum_quality<MinimumMeanRatio || report.minimum_jacobian<MinimumScaledJacobian ||
               report.maximum_edge>MaximumMetricEdge);
          if(!core_declined)throw std::runtime_error(result.reason);
          last_rejection=result.reason;
        } else if(coarsening && already_valid && fresh.cells.size()>=old.cells.size()) {
          core_declined=true;complete=already_valid;
          last_rejection="Native3D regional coarsening did not reduce the cell count.";
        } else {
          stats.global_work=fresh.cells.size()+report.surface_trials+report.junction_trials+report.core_trials+report.front_trials;
          if(stats.global_work>stats.work_limit)throw std::runtime_error("Native3D region construction exceeds global event work.");
          std::map<Face,Id> physical;for(auto f:fresh.surface)physical.emplace(Key(f.v),f.facet);
          for(const auto& c:fresh.cells){OwnedCell row;row.cell=c;const auto faces=Faces(c);for(unsigned f=0;f<4;++f){auto found=physical.find(Key(faces[f]));if(found!=physical.end())row.physical[f]={true,found->second};}replacement.push_back(row);}
          complete=true;
        }
      }
    },imported.cells);}catch(const std::exception& e){failure.Set(2,0,e.what());}
    const auto& field_stats=service.Statistics();
    stats.region_field_points=field_stats.point_calls;stats.region_field_sensor_hits=field_stats.sensor_hits;
    stats.region_field_interior_hits=field_stats.interior_hits;stats.region_field_edges=field_stats.edge_calls;
    stats.region_field_edge_hits=field_stats.edge_hits;stats.region_field_imports=field_stats.imports;
    stats.region_field_maximum_resident=field_stats.maximum_resident;
    stats.field_requests+=field_stats.sensor_hits;stats.field_hits+=field_stats.sensor_hits;
    if(composition)stats.field_compositions+=field_stats.sensor_hits;

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
    const bool declined=world.maximum(core_declined);
    const bool changed=world.maximum(!replacement.empty());
    if(changed) {
      std::vector<std::vector<OwnedCell>> to(world.size);
      try {for(auto row:replacement)to[row.cell.id%world.size].push_back(row);}catch(const std::exception& e){failure.Set(2,0,e.what());}
      if(!vote(failure))return finish();
      const auto added=world.exchange(to,&admitted,options.memory_ceiling,transfer_memory::Add(bytes(),scratch));
      if(!admitted){result.reason="Native3D region result exchange exceeds storage.";return finish();}
      std::vector<Dependency> removed;try{for(const auto& row:working)removed.push_back({row.first,row.second.version,world.rank});}catch(const std::exception& e){failure.Set(2,0,e.what());}
      if(!vote(failure))return finish();
      if(!directory.Publish(working,removed,added,true,result.reason,options.memory_ceiling,transfer_memory::Add(bytes(),scratch)))return finish();
      if(!world.rank)++stats.accepted;stats.maximum_owned=std::max(stats.maximum_owned,working.size());stats.region_cells=CPassiveComm::Allreduce(working.size(),CPassiveComm::Op::SUM,world.comm);
    }
    result.valid=world.maximum(complete);stats.global_work=CPassiveComm::Allreduce(stats.global_work,CPassiveComm::Op::MAX,world.comm);
    if(changed || (!coarsening && !declined)) {
      result.rebuilt_domain=result.valid && changed;
      stats.stop=result.valid?EngineStop::Complete:EngineStop::Failure;result.reason.clear();return finish();
    }
    // Valid coarsening inputs and finer interior fields retain the existing volume search.
    if(options.timings)stats.reconstruction_seconds+=std::chrono::duration<double>(Clock::now()-region_started).count();
    region_active=false;result.valid=false;result.reason.clear();
  }
  auto import_source=[&](const std::vector<Cell>& cells,size_t extra,bool layer) {
    auto failed=[&](){DonorPatch patch;patch.reason=result.reason;return patch;};
    if(!admit(transfer_memory::Add(extra,transfer_memory::Bytes(cells),transfer_memory::Mul(cells.size(),sizeof(Point)))))return failed();
    CLocalFailure failure;std::vector<Point> points;
    try {points.reserve(cells.size());for(const auto& c:cells)points.push_back(Centroid(c));}
    catch(const std::exception& e){failure.Set(2,0,e.what());}
    if(!vote(failure))return failed();
    auto count_import=[&](){if(layer)++stats.layer_donor_batches;else ++stats.donor_batches;};
    count_import();
    auto patch=source.ProbePoints(points,options.memory_ceiling,
        transfer_memory::Add(bytes(),directory.ResidentBytes(),extra,transfer_memory::Bytes(cells)));
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
    // First-row cones preserve this property; arbitrary reconstruction still needs full-cell imports.
    if(!world.maximum(!supported))return patch;
    // shortcut: shared-face inserted points retain full imports; extend only with canonical-source equivalence proof.
    patch={};std::vector<Point>().swap(points);count_import();
    return source.ImportCells(cells,options.memory_ceiling,
        transfer_memory::Add(bytes(),directory.ResidentBytes(),extra));
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
      return !height_check(GeometryPatch(Imported{{row},true,{}}),reason);
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
        dependencies=directory.LookupBatch(features,options.memory_ceiling,held);
        if(!dependencies.valid){result.reason=dependencies.reason;return false;}
        imported=directory.FetchBatch(dependencies,working,options.memory_ceiling,held);
        if(!imported.valid){result.reason=imported.reason;return false;}
        try {for(size_t i=0;i<imported.candidates.size();++i) {
          const auto& item=imported.candidates[i];
          if(!item.valid || item.cells.size()!=1 || item.cells.front().cell.id!=queued.at(i))
            throw std::runtime_error("Native3D first-row batch lacks its complete queued cell.");
        }}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!vote(failure))return false;
        // These operators preserve every old face/vertex; only the cell interiors are written.
        reserved=directory.ReserveInteriors(imported,options.memory_ceiling,held);
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
      size_t span=options.candidate_batch;
      auto construct=[&]() {
        for(size_t at=0;at<slots;) {
          if(timed_out())return false;
          size_t group=std::min(span,slots-at);DonorPatch donors;
          {
            Timer timer(stats.layer_donor_seconds,options.timings);
            for(;;) {
              std::vector<Cell> query;
              try {for(size_t k=at;k<std::min(at+group,imported.candidates.size());++k)
                if(reserved.chosen[k])query.push_back(imported.candidates[k].cells.front().cell);
              }catch(const std::exception& e){failure.Set(2,0,e.what());}
              if(!vote(failure))return false;
              donors=import_source(query,layerScratch,true);
              if(donors.valid)break;
              if(!DonorCap(donors.reason) || group==1){result.reason=donors.reason;return false;}
              span=(group+1)/2;group=std::min(span,slots-at);++stats.layer_donor_retries;
            }
          }
          std::unique_ptr<FrozenField> target;
          try {
            Timer timer(stats.layer_metric_seconds,options.timings);
            const auto components=AssociatedExtensions(donors.cells,geometry);
            target=std::make_unique<FrozenField>(std::move(donors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
          }catch(const std::exception& e){failure.Set(2,0,e.what());}
          if(!vote(failure))return false;
          FieldAccounting accounting{stats,*target};
          bool rejected=false,expired=false;
          auto local_deadline=[&]() {return options.maximum_seconds && std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds;};
          for(size_t k=at;k<at+group;++k) {
            if(local_deadline()){expired=true;break;}
            if(k<imported.candidates.size() && reserved.chosen[k])try {
              const auto old=GeometryPatch(imported.candidates[k]);auto& fresh=proposals[k];bool approved=false;
              const auto ticket=transfer_memory::Add(transfer_memory::Mul(
                  transfer_memory::Add(transfer_memory::Mul(epoch,world.size),world.rank),options.candidate_batch),k);
              const auto nodeOffset=transfer_memory::Mul(ticket,5),cellOffset=transfer_memory::Mul(ticket,MaximumReplacementCells);
              if(maximum_node>UINT64_MAX-5 || nodeOffset>UINT64_MAX-maximum_node-5 ||
                 maximum_cell>UINT64_MAX-MaximumReplacementCells || cellOffset>UINT64_MAX-maximum_cell-MaximumReplacementCells)
                throw std::invalid_argument("Native3D first-row reserved range overflows.");
              if(old.cells.size()!=1 || old.surface.size()>4)
                throw std::invalid_argument("Native3D first-row identity reservation requires one complete wall cell.");
              const Id node=maximum_node+1+nodeOffset,cell=maximum_cell+1+cellOffset;
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
              double bestQuality=-1;std::vector<Patch> strongest;
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
                {Timer timer(stats.layer_metric_seconds,options.timings);
                  ++stats.layer_shape_screens;const auto quality=MetricShapeQuality(candidate.cells,*target);
                  if(quality<bestQuality){++stats.layer_shape_pruned;continue;}
                  if(quality>bestQuality) {
                    stats.layer_shape_pruned+=strongest.size();strongest.clear();bestQuality=quality;
                  }
                  strongest.push_back(std::move(candidate));approved=true;result.reason.clear();
                }
              }
              // Length only breaks exact shape ties; full composed seeding certifies every selected cell later.
              if(strongest.size()==1)fresh=std::move(strongest.front());
              else if(approved) {
                long double bestLength=std::numeric_limits<long double>::infinity();std::map<Edge,EdgeMeasure> reuse;
                for(auto& candidate:strongest) {
                  MetricStats counted;PatchMetric measured;
                  {Timer timer(stats.layer_metric_seconds,options.timings);
                    measured=MeasureMetricCells(candidate.cells,*target,&counted,&reuse);
                  }
                  reuse=measured.edges;
                  stats.layer_edge_evaluations+=counted.edge_evaluations;stats.layer_edge_reused+=counted.edge_reused;
                  const auto quality=std::min(measured.minimum_mean_ratio/MinimumMeanRatio,measured.minimum_scaled_jacobian/MinimumScaledJacobian);
                  if(quality!=bestQuality)throw std::runtime_error("Native3D first-row shape changed under the frozen target.");
                  if(measured.maximum_length<bestLength) {bestLength=measured.maximum_length;fresh=std::move(candidate);}
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
        try {for(size_t k=0;k<proposals.size();++k)if(reserved.chosen[k]) {
          const auto& old=imported.candidates[k].cells.front().cell;const auto& fresh=proposals[k];
          const auto& dep=dependencies.candidates[k].cells.front();
          if(dep.cell!=old.id || dependencies.candidates[k].cells.size()!=1 || fresh.cells.empty())
            throw std::runtime_error("Native3D first-row publication lacks its complete replacement.");
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
        bool admitted=false;const auto baseline=transfer_memory::Add(bytes(),source.ResidentBytes(),directory.ResidentBytes(),layerScratch);
        const auto removed=world.exchange(toRemove,&admitted,options.memory_ceiling,baseline);
        if(!admitted){result.reason="Native3D first-row removal routing exceeds budget.";return false;}
        const auto added=world.exchange(toAdd,&admitted,options.memory_ceiling,baseline);
        if(!admitted){result.reason="Native3D first-row addition routing exceeds budget.";return false;}
        if(!directory.Publish(working,removed,added,true,result.reason,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),layerScratch)))return false;
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
  // Small batches amortize donor exchange/index construction; halve on common import rejection to the one-cell bound.
  auto measure=[&](const std::vector<Id>& ids,bool seed) {
    const size_t count=CPassiveComm::Allreduce(ids.size(),CPassiveComm::Op::MAX,world.comm);
    for (size_t slot=0; slot<count;) {
      if(options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
        stats.stop=EngineStop::TimeBudget;result.reason=seed ? "Native3D metric seeding exhausts elapsed-time budget." :
            "Native3D metric update exhausts elapsed-time budget.";return false;
      }
      size_t batch=std::min(MeasurementBatch,count-slot);
      DonorPatch imported;
      CLocalFailure failure;
      for (;;) {
        std::vector<Cell> regions;
        try {
          for (size_t i=slot;i<std::min(slot+batch,ids.size());++i)
            regions.push_back(working.at(ids[i]).cell);
        } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        if (!vote(failure)) return false;
        imported=import_source(regions,0,false);
        // Narrow only a one-cell cap failure; private reconstruction still needs whole-cell coverage.
        if (!imported.valid && batch==1 &&
            DonorCap(imported.reason)) {
          ++stats.donor_batches; ++stats.query_import_retries;
          imported=source.ImportMetricCells(regions,options.memory_ceiling,
              transfer_memory::Add(bytes(),directory.ResidentBytes()));
        }
        if (imported.valid) break;
        if (batch==1) { result.reason=imported.reason; return false; }
        batch=(batch+1)/2; ++stats.batch_retries;
      }
      if (!admit(transfer_memory::Add(transfer_memory::Bytes(imported.cells),
          MaximumFrozenFieldBytes,transfer_memory::Mul(working.size(),selection_bound(selection.track_repairs))))) return false;
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
          std::map<Edge,EdgeMeasure> reuse;
          selection.Cached(cell,reuse);
          for (size_t a=0; a<4; ++a) for (size_t b=a+1; b<4; ++b) {
            const Edge key{std::min(cell.v[a].id,cell.v[b].id),std::max(cell.v[a].id,cell.v[b].id)};
            const auto prior=retired_edges.find(key);
            if (prior!=retired_edges.end()) reuse.emplace(key,prior->second);
          }
          const bool strict=ValidateMetricPatch({cell},field,metric,reason,&counted,&reuse) &&
              height_check(GeometryPatch(Imported{{working.at(ids[i])},true,{}}),reason);
          selection.Add(working.at(ids[i]),metric,strict);
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
    if(!composition) {seed_slot=initial.size();return measure(initial,true);}
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
    return vote(failure) && measure(next,true);
  };
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
        while (!repairing && !bad_field && selection.FilterCollapse(choice,working)) choice=selection.Best(false);
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
      if (coarsening && !bad_field && !selection.prune_collapses) {
        // Invalid fields still need rejected-collapse repair; a local facet counterexample suffices only afterward.
        CLocalFailure failure;
        try { selection.EnableCollapseFilter(); } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        ready=vote(failure);
        if (ready) dirty=world.maximum(selection.cells.size()!=working.size() || !selection.queue.empty());
      }
      if (ready) {
        complete=dirty==0;
        if (recovered && intent==MetricChange::Refine && !complete) {
          // Recovery edits can unblock splits; retain the seeded repair dependencies while retrying them.
          repairing=!world.maximum(!selection.queue.empty() || !selection.geometry_queue.empty());
        } else if (repairing && intent!=MetricChange::Repair && !bad_field) {
          repairing=false; selection.track_repairs=false;
        }
        pick();
        if (ready && coarsening && !repairing && !bad_field && !any)
          complete=!world.maximum(selection.cells.size()!=working.size() || !selection.queue.empty());
        if(ready && !complete && !any)refill=world.maximum(seed_slot<initial.size());
        if (ready && !complete && !any && !refill && !repairing && !recovered) {
          ready=admit(transfer_memory::Mul(working.size(),selection_bound(true)));
          CLocalFailure failure;
          if (ready) try { selection.Recover(working); } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
          if (ready) ready=vote(failure);
          if (ready) {
            repairing=true; recovered=true; ++stats.recovery_starts;
            pick();
          }
        }
      }
    }
    if (!ready) return finish();
    if(refill) {ready=seed_next();if(!ready)return finish();continue;}
    {
      Timer timer(stats.selection_seconds,options.timings);
      if (options.maximum_seconds && world.maximum(std::chrono::duration<double>(Clock::now()-started).count()>=options.maximum_seconds)) {
        stats.stop=EngineStop::TimeBudget; result.reason="Native 3D kernel elapsed-time budget exhausted; feasibility is undetermined.";
        return finish();
      }
    }
    if (complete) { result.valid=true; result.reason.clear(); return finish(); }
    {
      Timer timer(stats.selection_seconds,options.timings);
      if (round && round%options.stagnation_window==0) {
        ++stats.progress_windows;
        const uint64_t progress=stats.size_progress+stats.quality_progress+stats.cell_removals+stats.geometry_progress;
        const bool mesh_progress=world.maximum(progress!=progress_at_window);
        const bool search_progress=world.maximum(stats.attempted!=attempted_at_window);
        progress_at_window=progress; attempted_at_window=stats.attempted;
        // Valid coarsening may exhaust rejected directions without changing an already admissible mesh.
        if (!mesh_progress) ++stats.stagnant_windows;
        if (!mesh_progress && search_progress) ++stats.search_windows;
        // Fresh rejected directions are finite search work even on invalid meshes; do not declare infeasibility early.
        if (!mesh_progress && !search_progress) {
          if (!recovered && !repairing) {
            ready=admit(transfer_memory::Mul(working.size(),selection_bound(true)));
            CLocalFailure blocked;
            if (ready) try { selection.Recover(working); } catch (const std::exception& e) { blocked.Set(2,0,e.what()); }
            if (ready) ready=vote(blocked);
            if (!ready) return finish();
            repairing=true; recovered=true; ++stats.recovery_starts; pick();
          } else {
            stats.stop=EngineStop::OperatorStagnation;
            result.reason="Native 3D admitted operators made no certified progress over the observation window; feasibility is undetermined.";
            return finish();
          }
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
    ++stats.rounds;
    const bool concurrent=composition && !curved && !repairing && !coarsening && options.candidate_batch>1 && world.size>1;
    stats.concurrent_rounds+=concurrent;
    const bool batched=!curved && !composition && !repairing && (!coarsening || !bad_field) && options.candidate_batch>1;
    std::vector<Choice> choices;
    DependencyBatch dependency_batch;
    ImportedBatch imported_batch;
    ReservationBatch reserved;
    size_t slots=1;
    const bool planned_coarsening=coarsening && !repairing && !curved && !composition && world.size>1;
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
        dependency_batch=directory.PlanBatch(features,selected,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
        if(!dependency_batch.valid){result.reason=dependency_batch.reason;return false;}
        try {
          std::vector<Choice> planned;planned.reserve(selected.size());
          for(size_t i:selected)planned.push_back(choices.at(i));
          stats.batch_deferred+=choices.size()-planned.size();choices.swap(planned);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return false;
        if(!world.maximum(!choices.empty())){result.reason="Native 3D complete-star selection made no progress.";return false;}
      }
      return true;
    };
    if (batched) {
      // All retained metadata/payloads remain byte-admitted while each disjoint candidate uses the existing transaction.
      batch_bytes=transfer_memory::Add(transfer_memory::Mul(options.candidate_batch,
          sizeof(Choice)+sizeof(Feature)+sizeof(Dependencies)+sizeof(Imported)+sizeof(bool)),
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
        ++stats.batch_rounds; stats.batch_candidates+=choices.size(); stats.proposed+=choices.size();
      }
      {
        Timer timer(stats.selection_seconds,options.timings);
        const uint64_t proposed=CPassiveComm::Allreduce(uint64_t(choices.size()),CPassiveComm::Op::SUM,world.comm);
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
          dependency_batch=directory.LookupBatch(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
        if (!dependency_batch.valid) { result.reason=dependency_batch.reason; return finish(); }
        imported_batch=directory.FetchBatch(dependency_batch,working,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes()));
        if (!imported_batch.valid) { result.reason=imported_batch.reason; return finish(); }
        for (const auto& candidate:imported_batch.candidates)
          if (!candidate.valid) failure.Set(1,0,candidate.reason);
        if (!(ready=vote(failure))) return finish();
      }
      {
        Timer timer(stats.reservation_seconds,options.timings);
        reserved=directory.ReserveBatch(imported_batch,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
        if (!reserved.valid) { result.reason=reserved.reason; return finish(); }
        size_t winners=0;
        for (bool chosen:reserved.chosen) winners+=chosen;
        stats.batch_reserved+=winners; stats.conflicts+=choices.size()-winners;
        if (!world.maximum(winners!=0)) { result.reason="Native 3D batch reservation made no progress."; return finish(); }
      }
    } else {
      Timer timer(stats.selection_seconds,options.timings);
      if((curved || composition) && !concurrent) {
        // Projected curvature and repair/coarsening retain their established serial ordering.
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
      if(coarsening && !repairing && !curved && !composition && world.size>1) {
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
    bool chosen=false;
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
          if(choice.active)dependencies=std::move(dependency_batch.candidates.at(0));else dependencies.valid=true;
        } else if(curved && repairing) {
          auto batch=directory.LookupBatch(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
          if(!batch.valid) {result.reason=batch.reason;return finish();}
          if(choice.active)dependencies=std::move(batch.candidates.at(0));else dependencies.valid=true;
          const bool oversized=choice.active && !dependencies.valid && dependencies.reason=="Native 3D dependency star exceeds budget.";
          CLocalFailure lookup_failure;
          if(choice.active && !dependencies.valid && !oversized)lookup_failure.Set(1,0,dependencies.reason);
          if(!vote(lookup_failure))return finish();
          if(world.maximum(oversized)) {
            // Reject this oversized repair star; other bounded repairs may still unlock the curved mesh.
            if(oversized) {
              selection.Block(choice.edge,choice.feature,true);++stats.attempted;++stats.rejected;
              stats.last_rejected_feature=choice.feature;
              last_rejection="feature "+std::to_string(choice.feature.count)+":"+std::to_string(choice.feature.vertices[0])+": "+dependencies.reason;
            }
            continue;
          }
        } else dependencies=directory.Lookup(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
        if (dependencies.valid) imported=directory.Fetch(dependencies,working,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes()));
      }
      if (!dependencies.valid || !imported.valid) {
        result.reason=dependencies.valid ? imported.reason : dependencies.reason; return finish();
      }
      {
        Timer timer(stats.reservation_seconds,options.timings);
        chosen=directory.Reserve(imported,choice.active,result.reason,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes()));
        stats.conflicts+=choice.active && !chosen;
        if (!world.sum(chosen)) ready=false;
      }
      if (!ready) return finish();
    }
    CLocalFailure failure;
    Patch old,fresh;
    auto guarded_geometry=[&](bool valid){return geometry_trial(valid && height_check(fresh,result.reason));};
    Node point;
    bool approved=false, moved=false, repair_approved=false, cavity_inserted=false;
    MetricDiagnosis certified;
    bool geometric_progress=false;
    auto geometric_change=[&](bool candidate,MetricChange kind) {
      if(!candidate || !curved)return candidate;
      try {
        const auto before=GeometryExcess(old,geometry,options.surface_tolerance,options.wall_heights);
        const auto after=GeometryExcess(fresh,geometry,options.surface_tolerance,options.wall_heights);
        geometric_progress=before>0 && after<before*(1-1e-12L);
        if((kind==MetricChange::GeometryRefine && !geometric_progress) ||
           (kind==MetricChange::Coarsen && after>0) ||
           (kind!=MetricChange::GeometryRefine && after>before)) {
          result.reason="Native3D candidate does not preserve/improve its frozen geometry error.";return guarded_geometry(false);
        }
        return true;
      }catch(const std::exception& e){result.reason=e.what();return guarded_geometry(false);}
    };
    std::unique_ptr<FrozenField> field;
    const auto epoch=transfer_memory::Add(transfer_memory::Mul(round,world.size),world.rank);
    const auto token=transfer_memory::Add(transfer_memory::Mul(epoch,options.candidate_batch),slot);
    const auto offset=transfer_memory::Mul(token,MaximumReplacementCells);
    const size_t node_stride=!options.wall_heights.empty() ? MaximumReplacementCells+3 : composition ? 9 : curved ? 2 : 1;
    const auto node_offset=transfer_memory::Mul(token,node_stride);
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
        if(!geometry.Analytic().count(first) && !geometry.Analytic().count(second)) {
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
        seed=source.ImportMetricCells(query,options.memory_ceiling,transfer_memory::Add(bytes(),directory.ResidentBytes(),MaximumFrozenFieldBytes));}
      if(!seed.valid){result.reason=seed.reason;return finish();}
      if(chosen)try {
        Timer timer(stats.metric_seconds,options.timings);CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
        const auto components=AssociatedExtensions(seed.cells,geometry);
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
        if ((((!repairing && !coarsening) || (repairing && choice.feature.count==1)) && (maximum_node>UINT64_MAX-node_stride || node_offset>UINT64_MAX-maximum_node-node_stride)) ||
            maximum_cell>UINT64_MAX-MaximumReplacementCells || offset>UINT64_MAX-maximum_cell-MaximumReplacementCells) {
          failure.Set(1,0,"Native 3D reserved identity range overflows.");
        } else try {
          old=GeometryPatch(imported);
          const auto a=choice.measure.a,b=choice.measure.b;
          if (!repairing && coarsening) approved=guarded_geometry(CollapseVertex(old,choice.edge[1],choice.edge[0],maximum_cell+1+offset,
              geometry,fresh,result.reason));
          else if (!repairing) {
            point={maximum_node+1+node_offset,{double((static_cast<long double>(a.x)+b.x)/2),
                double((static_cast<long double>(a.y)+b.y)/2),double((static_cast<long double>(a.z)+b.z)/2)}};
            if(composition)point.p=splitPosition;
            approved=guarded_geometry(SplitEdge(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason));
            if (!approved) {
              approved=guarded_geometry(InsertEdgeCavity(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason));
              cavity_inserted=approved;
            }
            if(!approved && physical_edge && composition && central_available &&
                (point.p.x!=centralPosition.x || point.p.y!=centralPosition.y || point.p.z!=centralPosition.z)) {
              point.p=centralPosition;
              approved=guarded_geometry(SplitEdge(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason));
              if(!approved) {
                approved=guarded_geometry(InsertEdgeCavity(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason));
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
      if (ready && !curved && world.maximum(metric_needed)) sensors=source.ImportCells(regions,options.memory_ceiling,
          transfer_memory::Add(bytes(),directory.ResidentBytes(),transfer_memory::Bytes(imported.cells),
              transfer_memory::Bytes(old.cells),transfer_memory::Bytes(old.surface),
              transfer_memory::Bytes(fresh.cells),transfer_memory::Bytes(fresh.surface)));
      else if (ready) sensors.valid=true;
    }
    bool streamed=ready && (curved || (!sensors.valid && DonorCap(sensors.reason)));
    if (!ready || (!sensors.valid && !streamed)) { if (ready) result.reason=sensors.reason; return finish(); }
    const size_t private_scratch=transfer_memory::Add(transfer_memory::Bytes(imported.cells),
        transfer_memory::Mul(MaximumReplacementCells,8192),MaximumFrozenFieldBytes,MetricPatchSolveScratchBytes,
        transfer_memory::Mul(MaximumCampaignEdges,sizeof(std::pair<const Edge,EdgeMeasure>)+6*sizeof(void*)));
    if (streamed && !admit(private_scratch)) return finish();
    if (streamed) ++stats.private_query_retries;
    {
      Timer timer(stats.metric_seconds,options.timings);
      const double reconstruction_before=stats.reconstruction_seconds;
      const double reconstruction_cpu_before=stats.reconstruction_cpu_seconds;
      if (!streamed && metric_needed) try {
        CpuTimer private_cpu(stats.metric_cpu_seconds,options.timings);
        const auto components=AssociatedExtensions(sensors.cells,geometry);
        field=std::make_unique<FrozenField>(std::move(sensors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
        PatchMetric metric;
        std::map<Edge,EdgeMeasure> reuse;
        for (const auto& cell : old.cells) selection.Cached(cell,reuse);
        if (approved) approved=metric_trial(ValidateMetricChange(old.cells,fresh.cells,intent,*field,metric,result.reason,nullptr,&reuse,&certified),certified);
        if (!approved) cavity_inserted=false;
        // An already-admissible coarsening mesh needs removal, not repeated quality-only rearrangement.
        if (repairing || (!approved && (!coarsening || bad_field))) {
          if (!repairing) ++stats.repair_fallbacks;
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
                approved=guarded_geometry(MoveVertex(old,vertex,point,first,geometry,fresh,result.reason)); }
                if (approved) approved=metric_trial(ValidateMetricChange(old.cells,fresh.cells,MetricChange::Repair,*field,metric,result.reason,nullptr,&reuse,&certified),certified);
                if (approved) { moved=true; break; }
              }
            } else {
              unsigned proposals=0;
              for (const auto& row:nodes) {
                if (++proposals>16) break;
                ++stats.repair_proposals;
                { Timer geometry_timer(stats.reconstruction_seconds,options.timings); CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds,options.timings);
                  approved=guarded_geometry(Reconnect(old,row.first,first,geometry,fresh,result.reason)); }
                if (approved) approved=metric_trial(ValidateMetricChange(old.cells,fresh.cells,MetricChange::Repair,*field,metric,result.reason,nullptr,&reuse,&certified),certified);
                if (approved) break;
              }
            }
            return approved;
          };
          const bool move=repairing ? choice.feature.count==1 : coarsening;
          const Id vertex=repairing ? choice.feature.vertices[0] : choice.edge[1];
          approved=try_repair(move,vertex);
          if (!approved && !repairing && coarsening) approved=try_repair(false,vertex);
          repair_approved=approved;
          if (approved && !repairing) ++stats.fallback_accepted;
        }
      } catch (const std::exception& e) { approved=false; result.reason=e.what(); }
      if (field) FieldCounts(stats,*field);
      stats.metric_seconds-=stats.reconstruction_seconds-reconstruction_before;
      stats.metric_cpu_seconds-=stats.reconstruction_cpu_seconds-reconstruction_cpu_before;
    }
    if(!streamed && composition && !repairing && !coarsening) {
      if(world.maximum(chosen && !approved)) {
        field.reset();if(!admit(private_scratch))return finish();
        streamed=true;++stats.private_query_retries;
      }
    }
    if (streamed) {
      // Point support stays proposal-specific. Completed full-field edge integrals survive within this admitted campaign.
      std::map<Edge,EdgeMeasure> campaign_edges;
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
        if (!cells.empty()) measured.weakest_cell=measured.sliver_cell=cells.front().id;
        for (size_t slot=0;slot<count;) {
          size_t batch=std::min(MeasurementBatch,count-slot);
          DonorPatch donors;
          for (;;) {
            std::vector<Cell> query;
            try {
              if (usable) for (size_t i=slot;i<std::min(slot+batch,cells.size());++i) query.push_back(cells[i]);
            } catch (const std::exception& e) { local.Set(2,0,e.what()); }
            if (!(ready=vote(local))) return false;
            {
              Timer timer(stats.donor_seconds,options.timings);
              ++stats.donor_batches;
              donors=source.ImportMetricCells(query,options.memory_ceiling,
                  transfer_memory::Add(bytes(),directory.ResidentBytes(),private_scratch));
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
              const auto components=AssociatedExtensions(donors.cells,geometry);
              target=std::make_unique<FrozenField>(std::move(donors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
              for (size_t i=slot;i<std::min(slot+batch,cells.size());++i) {
                const auto part=MeasureMetricCells({cells[i]},*target,&counted,&reuse,shape,shape ? &certified : nullptr,&campaign_edges);
                if (part.minimum_mean_ratio<measured.minimum_mean_ratio) {
                  measured.minimum_mean_ratio=part.minimum_mean_ratio; measured.weakest_cell=part.weakest_cell;
                }
                if (part.minimum_scaled_jacobian<measured.minimum_scaled_jacobian) {
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
      auto validate_private=[&](bool candidate,MetricChange kind,bool no_new_oversized=false,bool strict_size=false) {
        const bool completed=concurrent && approved && !candidate;
        const auto retained=certified;
        certified={};
        candidate=geometric_change(candidate,kind);
        PatchMetric metric;
        const std::vector<Cell> empty;
        const bool measured=measure_private(candidate ? fresh.cells : empty,metric,true,before.edges);
        bool valid=false;
        { Timer timer(stats.metric_seconds,options.timings); CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
          if (candidate) {
            const auto effective=(kind==MetricChange::Repair && geometric_progress) ? MetricChange::GeometryRepair : kind;
            if (measured) valid=ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),metric,effective,result.reason,&certified);
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
        CLocalFailure observed;
        if(chosen && !valid && !completed) {
          stats.last_metric_rejection=certified;
          stats.last_rejected_feature=repairing ? choice.feature : Feature{{choice.edge[0],choice.edge[1],0},2};
          if(!old.cells.empty())stats.last_rejected_cavity_centroid=Centroid(old.cells.front());
          last_rejection="feature "+std::to_string(stats.last_rejected_feature.count)+":"+
              std::to_string(stats.last_rejected_feature.vertices[0])+": "+result.reason;
          if(options.rejected_trial)try {
            options.rejected_trial(old,fresh,stats.last_rejected_feature,certified,candidate,result.reason);
          }catch(const std::exception& e){observed.Set(2,0,e.what());}
        }
        if(observe_rejections)ready=vote(observed);
        if(completed)certified=retained;
        return valid;
      };
      approved=validate_private(metric_needed && old_measured && approved,choice.geometric ? MetricChange::GeometryRefine : intent);
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
      auto place_patch=[&](bool search,const std::vector<Id>& apices) {
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
          joined=directory.Lookup(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!joined.valid) {
          if(joined.reason=="Native 3D dependency star exceeds budget." || joined.reason=="Native 3D dependency union exceeds budget.")search=false;
          else failure.Set(1,0,joined.reason);
        }
        if(!(ready=vote(failure)))return false;
        if(!search){joined.valid=true;joined.cells.clear();}
        Imported more;
        {Timer timer(stats.dependency_seconds,options.timings);
          more=directory.Fetch(joined,working,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
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
          dependencies=std::move(joined);imported=std::move(more);old=GeometryPatch(imported);reuse.clear();
          for(const auto& c:old.cells)selection.Cached(c,reuse);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return false;
        bool reserved=false;
        {Timer timer(stats.reservation_seconds,options.timings);
          reserved=directory.Reserve(imported,search || approved,result.reason,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if((search || approved) && !reserved) {
          if(result.reason=="Native 3D conflicting dependency reservation.") {
            ++stats.conflicts;
            if(!concurrent){search=false;approved=false;}
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
          support=source.ImportCells(search?old.cells:empty,options.memory_ceiling,transfer_memory::Add(bytes(),directory.ResidentBytes(),private_scratch));}
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
          FrozenField target(std::move(support.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
          FieldAccounting accounting{stats,target};std::vector<std::pair<Id,Id>> reserved_apices;
          for(size_t i=0;i<apices.size();++i)reserved_apices.emplace_back(apices[i],maximum_node+2+i+node_offset);
          candidate=PlaceSplitPatch(old,choice.edge,point,reserved_apices,maximum_cell+1+offset,target,geometry,options.wall_heights,
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
        const bool passed=validate_private(candidate,MetricChange::Refine);
        if(search && !approved){approved=passed;stats.placement_validated+=passed;}
        else if(!search)certified=retained_metric;
        if(!ready)return false;
        if(search && approved){cavity_inserted=true;joint=false;}
        return true;
      };
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
          joined=directory.Lookup(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!joined.valid) {
          if(joined.reason=="Native 3D dependency star exceeds budget." || joined.reason=="Native 3D dependency union exceeds budget.")strip=wallSplit=false;
          else failure.Set(1,0,joined.reason);
        }
        if(!(ready=vote(failure)))return finish();
        if(!strip && !wallSplit){joined.valid=true;joined.cells.clear();}
        Imported more;
        {Timer timer(stats.dependency_seconds,options.timings);
          more=directory.Fetch(joined,working,options.memory_ceiling,
              transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if(!more.valid){result.reason=more.reason;return finish();}
        if(strip || wallSplit)try {
          for(const auto& olddep:dependencies.cells) {
            const auto found=std::find_if(joined.cells.begin(),joined.cells.end(),[&](const auto& dep){return dep.cell==olddep.cell;});
            if(found==joined.cells.end() || found->version!=olddep.version || found->owner!=olddep.owner)
              throw std::runtime_error("Native3D strip lost an original complete/versioned edge dependency.");
          }
          dependencies=std::move(joined);imported=std::move(more);old=GeometryPatch(imported);reuse.clear();
          for(const auto& c:old.cells)selection.Cached(c,reuse);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        bool reserved=false;
        {Timer timer(stats.reservation_seconds,options.timings);
          reserved=directory.Reserve(imported,strip || wallSplit,result.reason,options.memory_ceiling,
              transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));}
        if((strip || wallSplit) && !reserved) {
          if(concurrent && result.reason=="Native 3D conflicting dependency reservation.")++stats.conflicts;
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
              transfer_memory::Add(bytes(),directory.ResidentBytes(),private_scratch));}
        if(!donors.valid) {
          if(DonorCap(donors.reason))strip=false;
          else {result.reason=donors.reason;return finish();}
        }
        std::array<Point,3> splits{};Edge extraEdge{};Node extraPoint,offEdgePoint;bool extraSplit=false;
        std::array<std::vector<std::pair<Id,Node>>,3> moveSeeds;std::vector<std::pair<Id,Node>> normalMoves;
        if(strip)try {
          Timer timer(stats.metric_seconds,options.timings);CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
          const auto components=AssociatedExtensions(donors.cells,geometry);
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
                Patch children;
                candidate=extraFrames[frame]==2 ?
                    InsertEdgeCavity(star,extraEdge,offEdgePoint,maximum_cell+1+offset,geometry,children,result.reason) :
                    SplitEdge(star,extraEdge,extraPoint,maximum_cell+1+offset,geometry,children,result.reason);
                if(candidate) {
                  fresh.cells.erase(std::remove_if(fresh.cells.begin(),fresh.cells.end(),[&](const auto& c){
                    return std::any_of(star.cells.begin(),star.cells.end(),[&](const auto& d){return c.id==d.id;});
                  }),fresh.cells.end());
                  fresh.cells.insert(fresh.cells.end(),children.cells.begin(),children.cells.end());
                  if(fresh.cells.size()>MaximumReplacementCells)candidate=false;
                  else for(size_t i=0;i<fresh.cells.size();++i)fresh.cells[i].id=maximum_cell+1+offset+i;
                }
              }
              candidate=guarded_geometry(candidate);
            }
          }catch(const std::exception& e){failure.Set(2,0,e.what());}
          if(!(ready=vote(failure)))return finish();
          const bool passed=validate_private(candidate,MetricChange::Refine,!normalTrial);
          if(strip && !approved)approved=passed;
          if(!ready)return finish();
        }
        if(strip && approved){cavity_inserted=true;joint=false;}
      }
      bool search=joint && composition && flat_edge && !choice.geometric && wallApices.empty() && !approved;
      std::vector<Id> apices;
      if(world.maximum(search)) {
        std::vector<Id> candidates;std::vector<Feature> features;
        if(search)try {
          std::set<Id> physical,vertices;
          for(const auto& f:old.surface)physical.insert(f.v.begin(),f.v.end());
          for(const auto& c:old.cells)if(std::count_if(c.v.begin(),c.v.end(),[&](Node n){return n.id==choice.edge[0] || n.id==choice.edge[1];})==2)
            for(auto n:c.v)if(n.id!=choice.edge[0] && n.id!=choice.edge[1] && !physical.count(n.id))vertices.insert(n.id);
          candidates.assign(vertices.begin(),vertices.end());for(Id id:candidates)features.push_back({{{id,0,0}},1});
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        {Timer timer(stats.dependency_seconds,options.timings);
          auto metadata=directory.LookupBatch(features,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
          if(!metadata.valid){result.reason=metadata.reason;return finish();}
          auto payload=directory.FetchBatch(metadata,working,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
          if(!payload.valid){result.reason=payload.reason;return finish();}
          if(search)try {
            std::map<Id,size_t> costs;
            for(size_t i=0;i<candidates.size();++i) {
              const auto& star=payload.candidates.at(i);
              if(!star.valid) {
                if(star.reason=="Native 3D dependency star exceeds budget.")continue;
                throw std::runtime_error(star.reason);
              }
              ++stats.placement_classified_vertices;bool physical=false;
              for(const auto& row:star.cells)for(unsigned f=0;f<4;++f)
                physical=physical || (row.physical[f].present && row.cell.v[f].id!=candidates[i]);
              if(!physical)costs.emplace(candidates[i],star.cells.size());
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
          auto metadata=directory.LookupBatch(hints,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
          if(!metadata.valid){result.reason=metadata.reason;return finish();}
          auto payload=directory.FetchBatch(metadata,working,options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
          if(!payload.valid){result.reason=payload.reason;return finish();}
          if(expand)try {
            const auto& star=payload.candidates.at(0);
            if(!star.valid) {
              if(star.reason=="Native 3D dependency star exceeds budget.")expand=false;
              else throw std::runtime_error(star.reason);
            } else {
              ++stats.placement_classified_vertices;bool physical=false;
              for(const auto& row:star.cells)for(unsigned f=0;f<4;++f)
                physical=physical || (row.physical[f].present && row.cell.v[f].id!=placement_obstruction);
              expand=!physical;
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
        std::vector<Cell> query;
        if(joint)try {
          query.push_back(old.cells.front());
          const auto edgeCell=std::find_if(old.cells.begin(),old.cells.end(),[&](const auto& c){
            unsigned matches=0;for(auto n:c.v)matches+=n.id==choice.edge[0] || n.id==choice.edge[1];return matches==2;
          });
          if(edgeCell==old.cells.end())throw std::runtime_error("Native3D joint patch lacks its complete split edge.");
          if(edgeCell->id!=query.front().id)query.push_back(*edgeCell);
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        const auto proxy=source.ImportMetricCells(query,
            options.memory_ceiling,transfer_memory::Add(bytes(),directory.ResidentBytes(),private_scratch));
        if(!proxy.valid){result.reason=proxy.reason;return finish();}
        if(joint)try {
          const auto components=AssociatedExtensions(proxy.cells,geometry);
          FrozenField sample(proxy.cells,composition,options.timings,components,options.edge_breakpoints,field_identity);
          placement=sample.Query(Centroid(old.cells.front())).target;
          nearPlacement=choice.geometric ? placement : sample.Query(point.p).target;FieldCounts(stats,sample);
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
                    maximum_cell+1+offset,options.wall_heights,geometry,fresh,refit_obstruction,result.reason));
                else candidate=guarded_geometry(ReconstructWallSplit(old,choice.edge,point,maximum_node+2+node_offset,
                    maximum_cell+1+offset,options.wall_heights,geometry,fresh,obstruction,result.reason,nullptr,options.surface_tolerance));
              } else if(!retained_frame || frame>=retained_frames) {
                candidate=guarded_geometry(ReconstructSplitPatch(old,choice.edge,point,{maximum_node+2+node_offset,{}},
                    frame>retained_frames?nearPlacement:placement,maximum_cell+1+offset,geometry,
                    fresh,obstruction,result.reason,nullptr,options.wall_heights));
              }
            }catch(const std::exception& e){failure.Set(2,0,e.what());}
            ready=vote(failure);
          }
          if(!ready)return finish();
          const bool compound_refit=retained_frame && frame==0;
          auto validate_joint=[&](bool available){return validate_private(available,compound_refit?MetricChange::Repair:
              choice.geometric?MetricChange::GeometryRefine:MetricChange::Refine,false,compound_refit && !choice.geometric);};
          bool passed=validate_joint(candidate);
          if(!ready)return finish();
          const bool missing_closure=participating && retained_frame && frame<retained_frames && wallSplit && !candidate &&
              (compound_refit?refit_obstruction.count>0:obstruction!=Face{});
          bool close_first=world.maximum(missing_closure);
          if(missing_closure)preferred_obstruction=compound_refit?refit_obstruction:Feature{obstruction,3};
          const bool repair=!close_first && participating && candidate && !passed && certified.failure==MetricFailure::Shape &&
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
                    transfer_memory::Add(bytes(),directory.ResidentBytes(),private_scratch));
              }
              CLocalFailure local;ConnectivityRepairReport report;bool repaired=false,missing_metric_star=false;
              if(!donors.valid && !DonorCap(donors.reason))local.Set(1,0,donors.reason);
              if(!(ready=vote(local)))return finish();
              if(active && donors.valid)try {
                const auto components=AssociatedExtensions(donors.cells,geometry);
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
                    while(cursor<node_stride && used.count(first_node+cursor))++cursor;
                    if(cursor==node_stride)throw std::invalid_argument("Native3D joint movement exceeds reserved node identities.");
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
        const auto added_dependencies=directory.LookupBatch(grow ? std::vector<Feature>{preferred_obstruction} : std::vector<Feature>{},
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
        const auto more=directory.Fetch(neighbor,working,options.memory_ceiling,
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
            dependencies.cells.swap(joined);imported.cells.swap(payload);old=GeometryPatch(imported);reuse.clear();
            for(const auto& c:old.cells)selection.Cached(c,reuse);
          }
        }catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        if(!world.maximum(joint))break;
        const bool reserved=directory.Reserve(imported,joint,result.reason,options.memory_ceiling,
            transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
        CLocalFailure reservation_failure;
        if(joint && !reserved) {
          if(concurrent && result.reason=="Native 3D conflicting dependency reservation.")++stats.conflicts;
          else reservation_failure.Set(1,0,"Native3D joint patch lost complete dependency reservation.");
        }
        if(!(ready=vote(reservation_failure)))return finish();
        if(!world.maximum(joint))break;
        const bool measured=measure_private(joint ? old.cells : empty,before,false,reuse);
        if(!ready)return finish();
        CLocalFailure old_invalid;if(joint && !measured)old_invalid.Set(1,0,result.reason);
        if(!(ready=vote(old_invalid)))return finish();
      }
      const bool fallback=metric_needed && old_measured && (repairing || (!approved && (!coarsening || bad_field)));
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
      if(composition && world.maximum(fallback && move)) {
        const bool participating=fallback && move;
        std::set<Id> neighbors;std::map<Id,Tensor> samples;Point origin{};
        try {if(participating) {
          origin=nodes.at(vertex).p;
          for(const auto& f:old.surface)if(std::find(f.v.begin(),f.v.end(),vertex)!=f.v.end())neighbors.insert(f.v.begin(),f.v.end());
          if(neighbors.empty())for(const auto& n:nodes)neighbors.insert(n.first);
          neighbors.erase(vertex);
        }}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
        const size_t count=CPassiveComm::Allreduce(participating?old.cells.size():0,CPassiveComm::Op::MAX,world.comm);
        for(size_t at=0;at<count;) {
          size_t batch=std::min<size_t>(16,count-at);DonorPatch donors;
          for(;;) {
            std::vector<Cell> queries;
            try {if(participating)for(size_t i=at;i<std::min(at+batch,old.cells.size());++i)queries.push_back(old.cells[i]);}
            catch(const std::exception& e){failure.Set(2,0,e.what());}
            if(!(ready=vote(failure)))return finish();
            {Timer timer(stats.donor_seconds,options.timings);++stats.donor_batches;
              donors=source.ImportMetricCells(queries,options.memory_ceiling,transfer_memory::Add(bytes(),directory.ResidentBytes(),private_scratch));}
            if(donors.valid)break;
            if(!DonorCap(donors.reason) || batch==1){result.reason=donors.reason;return finish();}
            batch=(batch+1)/2;++stats.batch_retries;
          }
          if(participating && at<old.cells.size())try {
            Timer timer(stats.metric_seconds,options.timings);CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
            const auto components=AssociatedExtensions(donors.cells,geometry);
            FrozenField target(std::move(donors.cells),composition,options.timings,components,options.edge_breakpoints,field_identity);
            FieldAccounting accounting{stats,target};
            for(size_t i=at;i<std::min(at+batch,old.cells.size());++i)for(const auto& n:old.cells[i].v)
              if(neighbors.count(n.id) && !samples.count(n.id)) {
                const auto q=n.p;
                samples.emplace(n.id,target.QueryMidpoint(origin,q).target);
              }
          }catch(const std::exception& e){failure.Set(2,0,e.what());}
          if(!(ready=vote(failure)))return finish();at+=batch;
        }
        try {if(participating && !neighbors.empty()) {
          std::vector<std::pair<Point,Tensor>> values;for(auto id:neighbors)values.emplace_back(nodes.at(id).p,samples.at(id));
          moveCenter=MetricMeanPoint(origin,values);
        }}catch(const std::exception& e){failure.Set(2,0,e.what());}
        if(!(ready=vote(failure)))return finish();
      }
      for (unsigned trial=0;trial<19;++trial) {
        if (!world.maximum(fallback && !approved)) break;
        const bool move_trial=trial<3;
        const bool participating=fallback && !approved && (move_trial ? move : (!move || (repairing && choice.feature.count==1) || (!repairing && coarsening))) &&
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
                candidate=guarded_geometry(MoveVertex(old,vertex,point,first,geometry,fresh,result.reason));
              }
            } else if (!move_trial) {
              auto apex=nodes.begin(); std::advance(apex,trial-3);
              if(move && repairing && choice.feature.count==1)
                candidate=guarded_geometry(CollapseVertex(old,vertex,apex->first,first,geometry,fresh,result.reason));
              else candidate=guarded_geometry(Reconnect(old,apex->first,first,geometry,fresh,result.reason));
            }
          } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
          ready=vote(failure);
        }
        if (!ready) return finish();
        const bool valid=validate_private(candidate,MetricChange::Repair);
        if (!ready) return finish();
        if (valid) { approved=true; repair_approved=true; moved=move_trial; }
      }
      if (repair_approved && !repairing) ++stats.fallback_accepted;
    }
    const bool surface_edit=approved && !SameSkin(old,fresh);
    if(curved && world.maximum(surface_edit)) {
      const bool prior=approved;
      if(!Clearance(world,working,old,fresh,approved,result.reason,stats,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes(),directory.ResidentBytes(),private_scratch)))return finish();
      if(prior && !approved)geometry_trial(false);
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
      const bool admitted=directory.Reserve(imported,approved,result.reason,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes(),private_scratch));
      if(approved && !admitted) {
        if(result.reason=="Native 3D conflicting dependency reservation.") {
          approved=false;chosen=false;++stats.conflicts;++stats.rejected;++stats.proposal_deferred;
        }else final_failure.Set(1,0,"Native3D final proposal reservation failed: "+result.reason);
      }
      if(!vote(final_failure))return finish();
      const auto commits=CPassiveComm::Allreduce(uint64_t(approved),CPassiveComm::Op::SUM,world.comm);
      stats.maximum_parallel_commits=std::max(stats.maximum_parallel_commits,commits);
    }
    if(approved && !coarsening && !repairing && !repair_approved)
      for(const auto& cell:fresh.cells)for(auto node:cell.v)if(node.id==point.id)point=node;
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
          if (repair_approved || cavity_inserted) {
            for (size_t i=0;i<fresh.cells.size();++i) parents.push_back(old.cells[i%old.cells.size()].id);
          } else for (const auto& cell:old.cells) {
            bool keep=false;
            for (auto node:cell.v) keep=keep || node.id==choice.edge[0];
            if (!coarsening || !keep) parents.push_back(cell.id);
            if (!coarsening) parents.push_back(cell.id);
          }
          if (fresh.cells.size()!=parents.size()) throw std::logic_error("Unexpected native tetrahedral edit layout.");
          std::map<Id,int> owners;
          for (const auto& row : dependencies.cells) { owners.emplace(row.cell,row.owner); removals[row.owner].push_back(row); }
          std::map<Face,Id> physical;
          for (auto face : fresh.surface) physical.emplace(Key(face.v),face.facet);
          for (size_t i=0; i<fresh.cells.size(); ++i) {
            OwnedCell row; row.cell=fresh.cells[i];
            const auto faces=Faces(row.cell);
            for (size_t f=0; f<4; ++f) {
              const auto found=physical.find(Key(faces[f]));
              if (found!=physical.end()) row.physical[f]={true,found->second};
            }
            additions[owners.at(parents[i])].push_back(row);
          }
        } else if (chosen) {
          std::set<int> owners;
          for (const auto& row : dependencies.cells) owners.insert(row.owner);
          for (int owner : owners) blocked[owner].push_back({choice.edge,choice.feature});
        }
      } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      ready=vote(failure);
      if (ready) {
        bool admitted=false;
        const auto baseline=transfer_memory::Add(bytes(),source.ResidentBytes(),directory.ResidentBytes(),
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
      ready=directory.Publish(working,removed,added,true,result.reason,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes(),MaximumFrozenFieldBytes));
    }
    if (!ready) return finish();
    std::vector<Id> changed;
    {
      Timer timer(stats.update_seconds,options.timings);
      ready=admit(transfer_memory::Add(transfer_memory::Mul(working.size(),selection_bound(selection.track_repairs)),
          transfer_memory::Mul(transfer_memory::Mul(6,removed.size()),sizeof(std::pair<const Edge,EdgeMeasure>)+4*sizeof(void*))));
      if (ready) try {
        // Retain retired scalars until all children are measured against this event's immutable source.
        for (const auto& row : removed) {
          const auto cached=selection.cells.find(row.cell);
          if(cached==selection.cells.end()){++stats.unmeasured_retired;continue;}
          for (auto key : cached->second.edges)retired_edges.emplace(key,selection.edges.at(key).measure);
          selection.Drop(row.cell);
        }
        for (const auto& row : added) changed.push_back(row.cell.id);
        for (auto row : failures) selection.Block(row.edge,row.feature,repairing);
        if (approved) {
          ++stats.accepted;
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
      if (ready) ready=measure(changed,false);
      if (ready && (coarsening || selection.track_splits)) {
        try { for (Id vertex:directory.ChangedVertices()) selection.Touch(vertex); }
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
    result.valid=incomplete==0;
    stats.stop=result.valid ? EngineStop::Complete : EngineStop::RoundBudget;
    if (incomplete==1)
      result.reason="Native 3D coarsening round limit reached with valid cached metric cells and pending collapse candidates.";
    else if (incomplete==2)
      result.reason="Native 3D adaptation round limit reached before cached metric size/shape gates were satisfied.";
    else if (incomplete==3)
      result.reason="Native 3D adaptation round limit reached with incomplete metric-cell measurements.";
    else result.reason.clear();
  }
  return finish();
}
}  // namespace
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
}  // namespace SU2Native3D
