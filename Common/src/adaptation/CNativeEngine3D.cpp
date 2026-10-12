/*!
 * \file CNativeEngine3D.cpp
 * \brief Incremental frozen-target selection and distributed private planar refinement/coarsening.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeEngine3D.hpp"
#include <chrono>
#include <cmath>
#include <ctime>
#include <set>
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
struct Choice { Edge edge{}; EdgeMeasure measure; bool active = false; Feature feature; };
class Selection {
 public:
  Selection(bool coarsening, bool repairing, const PlanarReference& geometry, EngineStats& stats, bool timings)
      : coarsening(coarsening), track_repairs(repairing), repair_event(repairing), geometry(geometry), stats(stats), timings(timings) {}
  bool coarsening, track_repairs, repair_event, prune_collapses = false;
  const PlanarReference& geometry;
  EngineStats& stats;
  bool timings;
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
  struct CellState { std::array<Edge, 6> edges{}; std::array<unsigned char,6> forbidden{}; std::array<Id,4> vertices{}; bool valid = false; };
  std::map<Id,CellState> cells;
  std::map<Edge,EdgeState> edges;
  std::set<std::pair<long double,Edge>> queue;
  std::map<Id,std::set<Id>> vertex_cells, blocked_collapses;
  size_t vertex_links=0, blocked_links=0;
  std::map<Id,std::vector<RepairFeature>> cell_repairs;
  std::map<RepairFeature,size_t> repair_users;
  std::set<RepairFeature> repair_queue;
  size_t Bytes() const {
    return transfer_memory::Add(MapBytes(cells),MapBytes(edges),MapBytes(queue),MapBytes(cell_repairs),MapBytes(repair_users),MapBytes(repair_queue),repair_bytes,
        MapBytes(vertex_cells),MapBytes(blocked_collapses),transfer_memory::Mul(transfer_memory::Add(vertex_links,blocked_links),sizeof(Id)+4*sizeof(void*)));
  }
  Choice Best(bool repairing) const {
    if (repairing) {
      if (repair_queue.empty()) return {};
      const auto& key=*repair_queue.begin();
      Choice choice; choice.active=true; choice.feature={key.second,key.first}; return choice;
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
    Edge canonical=edge; std::sort(canonical.begin(),canonical.end());
    const auto it = edges.find(canonical);
    if (it==edges.end()) return;
    queue.erase({Priority(it->second.measure.length),edge});
    if (coarsening) blocked_links+=blocked_collapses[edge[1]].insert(edge[0]).second;
  }
  void Forget(Edge edge) {
    const auto found=blocked_collapses.find(edge[1]);
    if (found==blocked_collapses.end()) return;
    blocked_links-=found->second.erase(edge[0]);
    if (found->second.empty()) blocked_collapses.erase(found);
  }
  void Touch(Id vertex) {
    const auto found=blocked_collapses.find(vertex);
    if (found==blocked_collapses.end()) return;
    for (Id retained:found->second) {
      Edge edge{retained,vertex}, key=edge; std::sort(key.begin(),key.end());
      const auto measured=edges.find(key);
      if (measured==edges.end() || !Candidate(measured->second.measure.length)) continue;
      const auto direction=retained==key[0] ? 0u : 1u;
      if (prune_collapses && measured->second.forbidden[direction]) continue;
      stats.collapse_reactivated+=queue.insert({Priority(measured->second.measure.length),edge}).second;
    }
    blocked_links-=found->second.size(); blocked_collapses.erase(found);
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
    for (size_t i=0;i<4;++i) {
      row.vertices[i]=cell.v[i].id;
      vertex_links+=vertex_cells[row.vertices[i]].insert(cell.id).second;
    }
    size_t slot=0;
    for (const auto& measured : metric.edges) {
      const auto key=measured.first;
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
    invalid += !valid;
    if (track_repairs && !valid) AddRepairs(cell);
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
    for (const auto& row:working) if (!cells.at(row.first).valid) AddRepairs(row.second.cell);
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
  stats.field_edges+=f.edge_requests;
  stats.edge_separated+=f.kernel.segment_separated;
  stats.field_build_seconds+=f.build_seconds; stats.field_query_seconds+=f.query_seconds;
  stats.field_cache_seconds+=f.cache_seconds; stats.field_search_seconds+=f.search_seconds;
  stats.field_interpolation_seconds+=f.interpolation_seconds; stats.field_composition_seconds+=f.composition_seconds;
  stats.edge_seconds+=f.edge_seconds; stats.edge_search_seconds+=f.edge_search_seconds;
  stats.edge_trace_seconds+=f.edge_trace_seconds; stats.edge_integral_seconds+=f.edge_integral_seconds;
}
struct FieldAccounting {
  EngineStats& stats;
  const FrozenField& field;
  // Keep attempted queries when surrounding metric/topology checks throw.
  ~FieldAccounting() { FieldCounts(stats,field); }
};
EngineResult Adapt(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                   const PlanarReference& geometry, MetricComposition composition, EngineOptions options, MetricChange intent) {
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
  Selection selection(coarsening,repairing,geometry,stats,options.timings);
  std::map<Edge,EdgeMeasure> retired_edges;
  Directory directory(world,coarsening);
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
    return transfer_memory::Add(options.caller_bytes,accepted_bytes,batch_bytes,MapBytes(working),
        transfer_memory::Bytes(original),transfer_memory::Bytes(initial),transfer_memory::Bytes(result.splits),transfer_memory::Bytes(result.collapses),
        selection.Bytes(),MapBytes(retired_edges));
  };
  auto admit=[&](size_t extra) {
    const auto peak=transfer_memory::Add(bytes(),source.ResidentBytes(),directory.ResidentBytes(),extra);
    stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
    CLocalFailure failure;
    if (peak==SIZE_MAX || peak>options.memory_ceiling) failure.Set(3,0,"Native 3D adaptation working storage exceeds budget.");
    return vote(failure);
  };
  std::string last_rejection;
  uint64_t progress_at_window=0, attempted_at_window=0;
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
  auto finish=[&]() {
    if (stats.stop==EngineStop::None) stats.stop=result.valid ? EngineStop::Complete : EngineStop::Failure;
    stats.final_owned_cells=working.size();
    stats.cached_measured_cells=selection.cells.size();
    stats.cached_invalid_cells=selection.invalid;
    stats.remaining_edges=selection.queue.size();
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
      stats.other_seconds=stats.total_seconds-phases;
    }
    return std::move(result);
  };
  bool ready=false;
  Id maximum_node=0,maximum_cell=0;
  {
    Timer timer(stats.initialization_seconds,options.timings);
    CLocalFailure failure;
    // shortcut: enable curved runtime only after donor extension and geometry completion gates are integrated.
    if (!geometry.Analytic().empty())
      failure.Set(1,0,"Native3D analytic runtime requires integrated donor coverage and geometry completion gates.");
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
    ready=vote(failure);
    if (ready) ready=admit(transfer_memory::Add(accepted_bytes,
        transfer_memory::Mul(std::min<uint64_t>(stats.work_limit,4096),
            coarsening ? sizeof(Edge) : sizeof(SplitRecord)),transfer_memory::Mul(accepted.size(),selection_bound(repairing))));
    if (ready) try {
      working=accepted;
      stats.maximum_owned=working.size();
      if (coarsening) result.collapses.reserve(std::min<uint64_t>(stats.work_limit,4096));
      else if (!repairing) result.splits.reserve(std::min<uint64_t>(stats.work_limit,4096));
      initial.reserve(working.size());
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
  // Small batches amortize donor exchange/index construction; halve on common import rejection to the one-cell bound.
  auto measure=[&](const std::vector<Id>& ids,bool seed) {
    const size_t count=CPassiveComm::Allreduce(ids.size(),CPassiveComm::Op::MAX,world.comm);
    for (size_t slot=0; slot<count;) {
      size_t batch=std::min<size_t>(16,count-slot);
      DonorPatch imported;
      CLocalFailure failure;
      for (;;) {
        std::vector<Cell> regions;
        try {
          for (size_t i=slot;i<std::min(slot+batch,ids.size());++i)
            regions.push_back(working.at(ids[i]).cell);
        } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        if (!vote(failure)) return false;
        ++stats.donor_batches;
        imported=source.ImportCells(regions,options.memory_ceiling,
            transfer_memory::Add(bytes(),directory.ResidentBytes()));
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
        FrozenField field(std::move(imported.cells),composition,options.timings,components);
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
          const bool strict=ValidateMetricPatch({cell},field,metric,reason,&counted,&reuse);
          selection.Add(working.at(ids[i]),metric,strict);
          stats.edge_evaluations+=counted.edge_evaluations;
          stats.edge_reused+=counted.edge_reused;
          if (seed) ++stats.initial_cells; else ++stats.updated_cells;
        }
      } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      if (!vote(failure)) return false;
      slot+=batch;
    }
    return true;
  };
  {
    Timer timer(stats.seed_seconds,options.timings); ready=measure(initial,true);
  }
  if (!ready) return finish();
  for (size_t round=0; !options.maximum_rounds || round<options.maximum_rounds; ++round) {
    Choice choice;
    bool complete=false,any=false,bad_field=false;
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
          repairing=!world.maximum(!selection.queue.empty());
        } else if (repairing && intent!=MetricChange::Repair && !bad_field) {
          repairing=false; selection.track_repairs=false;
        }
        pick();
        if (ready && coarsening && !repairing && !bad_field && !any)
          complete=!world.maximum(selection.cells.size()!=working.size() || !selection.queue.empty());
        if (ready && !complete && !any && !repairing && !recovered) {
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
        const uint64_t progress=stats.size_progress+stats.quality_progress+stats.cell_removals;
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
    const bool batched=!repairing && (!coarsening || !bad_field) && options.candidate_batch>1;
    std::vector<Choice> choices;
    DependencyBatch dependency_batch;
    ImportedBatch imported_batch;
    ReservationBatch reserved;
    size_t slots=1;
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
        dependencies=directory.Lookup(choice.active ? std::vector<Feature>{feature} : std::vector<Feature>{},
            options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
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
    Node point;
    bool approved=false, moved=false, repair_approved=false, cavity_inserted=false;
    MetricDiagnosis certified;
    std::unique_ptr<FrozenField> field;
    const auto epoch=transfer_memory::Add(transfer_memory::Mul(round,world.size),world.rank);
    const auto token=transfer_memory::Add(transfer_memory::Mul(epoch,options.candidate_batch),slot);
    const auto offset=transfer_memory::Mul(token,MaximumReplacementCells);
    {
      Timer timer(stats.reconstruction_seconds,options.timings);
      if (chosen) {
        CpuTimer private_cpu(stats.reconstruction_cpu_seconds,options.timings);
        ++stats.attempted;
        if ((((!repairing && !coarsening) || (repairing && choice.feature.count==1)) && (maximum_node==UINT64_MAX || token>=UINT64_MAX-maximum_node)) ||
            maximum_cell>UINT64_MAX-MaximumReplacementCells || offset>UINT64_MAX-maximum_cell-MaximumReplacementCells) {
          failure.Set(1,0,"Native 3D reserved identity range overflows.");
        } else try {
          old=GeometryPatch(imported);
          const auto a=choice.measure.a,b=choice.measure.b;
          if (!repairing && coarsening) approved=geometry_trial(CollapseVertex(old,choice.edge[1],choice.edge[0],maximum_cell+1+offset,
              geometry,fresh,result.reason));
          else if (!repairing) {
            point={maximum_node+1+token,{double((static_cast<long double>(a.x)+b.x)/2),
                double((static_cast<long double>(a.y)+b.y)/2),double((static_cast<long double>(a.z)+b.z)/2)}};
            approved=geometry_trial(SplitEdge(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason));
            if (!approved) {
              approved=geometry_trial(InsertEdgeCavity(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason));
              cavity_inserted=approved;
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
      if (ready && world.maximum(metric_needed)) sensors=source.ImportCells(regions,options.memory_ceiling,
          transfer_memory::Add(bytes(),directory.ResidentBytes(),transfer_memory::Bytes(imported.cells),
              transfer_memory::Bytes(old.cells),transfer_memory::Bytes(old.surface),
              transfer_memory::Bytes(fresh.cells),transfer_memory::Bytes(fresh.surface)));
      else if (ready) sensors.valid=true;
    }
    const bool streamed=ready && !sensors.valid && DonorCap(sensors.reason);
    if (!ready || (!sensors.valid && !streamed)) { if (ready) result.reason=sensors.reason; return finish(); }
    const size_t private_scratch=transfer_memory::Add(transfer_memory::Bytes(imported.cells),
        transfer_memory::Mul(MaximumReplacementCells,8192),MaximumFrozenFieldBytes);
    if (streamed && !admit(private_scratch)) return finish();
    if (streamed) ++stats.private_query_retries;
    {
      Timer timer(stats.metric_seconds,options.timings);
      const double reconstruction_before=stats.reconstruction_seconds;
      const double reconstruction_cpu_before=stats.reconstruction_cpu_seconds;
      if (!streamed && metric_needed) try {
        CpuTimer private_cpu(stats.metric_cpu_seconds,options.timings);
        const auto components=AssociatedExtensions(sensors.cells,geometry);
        field=std::make_unique<FrozenField>(std::move(sensors.cells),composition,options.timings,components);
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
              if (maximum_node==UINT64_MAX || token>=UINT64_MAX-maximum_node) return false;
              std::set<Id> neighbors;
              for (auto face:old.surface) if (std::find(face.v.begin(),face.v.end(),vertex)!=face.v.end())
                neighbors.insert(face.v.begin(),face.v.end());
              if (neighbors.empty()) for (const auto& row:nodes) neighbors.insert(row.first);
              neighbors.erase(vertex);
              long double x=0,y=0,z=0;
              for (Id id:neighbors) { auto p=nodes.at(id).p; x+=p.x; y+=p.y; z+=p.z; }
              if (!neighbors.empty()) for (long double step:{1.L,.5L,.25L}) {
                auto p=nodes.at(vertex).p;
                point={maximum_node+1+token,{double(p.x+step*(x/neighbors.size()-p.x)),
                    double(p.y+step*(y/neighbors.size()-p.y)),double(p.z+step*(z/neighbors.size()-p.z))}};
                ++stats.repair_proposals;
                { Timer geometry_timer(stats.reconstruction_seconds,options.timings); CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds,options.timings);
                  bool contained=true;
                  for (auto face:old.surface) if (std::find(face.v.begin(),face.v.end(),vertex)!=face.v.end())
                    contained=contained && geometry.Contains(face.facet,point.p);
                  if (!contained) result.reason="Native 3D movement proposal leaves an incident immutable facet.";
                approved=contained ? geometry_trial(MoveVertex(old,vertex,point,first,geometry,fresh,result.reason)) : geometry_trial(false); }
                if (approved) approved=metric_trial(ValidateMetricChange(old.cells,fresh.cells,MetricChange::Repair,*field,metric,result.reason,nullptr,&reuse,&certified),certified);
                if (approved) { moved=true; break; }
              }
            } else {
              unsigned proposals=0;
              for (const auto& row:nodes) {
                if (++proposals>16) break;
                ++stats.repair_proposals;
                { Timer geometry_timer(stats.reconstruction_seconds,options.timings); CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds,options.timings);
                  approved=geometry_trial(Reconnect(old,row.first,first,geometry,fresh,result.reason)); }
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
    if (streamed) {
      // Geometry is now known. Import only its actual metric queries, never reuse this support for another proposal.
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
          size_t batch=std::min<size_t>(16,count-slot);
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
            std::unique_ptr<FrozenField> target;
            try {
              const auto components=AssociatedExtensions(donors.cells,geometry);
              target=std::make_unique<FrozenField>(std::move(donors.cells),composition,options.timings,components);
              for (size_t i=slot;i<std::min(slot+batch,cells.size());++i) {
                const auto part=MeasureMetricCells({cells[i]},*target,nullptr,&reuse,shape,shape ? &certified : nullptr);
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
      auto validate_private=[&](bool candidate,MetricChange kind) {
        PatchMetric metric;
        const std::vector<Cell> empty;
        const bool measured=measure_private(candidate ? fresh.cells : empty,metric,true,before.edges);
        bool valid=false;
        { Timer timer(stats.metric_seconds,options.timings); CpuTimer cpu(stats.metric_cpu_seconds,options.timings);
          if (candidate) {
            if (measured) valid=ValidateMetricChange(old.cells.size(),before,fresh.cells.size(),metric,kind,result.reason,&certified);
            metric_trial(valid,certified);
          }
        }
        return valid;
      };
      approved=validate_private(metric_needed && old_measured && approved,intent);
      if (!approved) cavity_inserted=false;
      if (!ready) return finish();
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
      for (unsigned trial=0;trial<19;++trial) {
        if (!world.maximum(fallback && !approved)) break;
        const bool move_trial=trial<3;
        const bool participating=fallback && !approved && (move_trial ? move : (!move || (!repairing && coarsening))) &&
            (move_trial || trial-3<nodes.size());
        if (!world.maximum(participating)) continue;
        bool candidate=false;
        { Timer timer(stats.reconstruction_seconds,options.timings);
          if (participating) try {
            CpuTimer cpu(stats.reconstruction_cpu_seconds,options.timings);
            ++stats.repair_proposals;
            const Id first=maximum_cell+1+offset;
            if (move_trial && maximum_node!=UINT64_MAX && token<UINT64_MAX-maximum_node) {
              std::set<Id> neighbors;
              for (auto face:old.surface) if (std::find(face.v.begin(),face.v.end(),vertex)!=face.v.end())
                neighbors.insert(face.v.begin(),face.v.end());
              if (neighbors.empty()) for (const auto& row:nodes) neighbors.insert(row.first);
              neighbors.erase(vertex);
              long double x=0,y=0,z=0;
              for (Id id:neighbors) { auto p=nodes.at(id).p; x+=p.x; y+=p.y; z+=p.z; }
              if (!neighbors.empty()) {
                const auto p=nodes.at(vertex).p; const long double step=trial==0 ? 1.L : (trial==1 ? .5L : .25L);
                point={maximum_node+1+token,{double(p.x+step*(x/neighbors.size()-p.x)),
                    double(p.y+step*(y/neighbors.size()-p.y)),double(p.z+step*(z/neighbors.size()-p.z))}};
                bool contained=true;
                for (auto face:old.surface) if (std::find(face.v.begin(),face.v.end(),vertex)!=face.v.end())
                  contained=contained && geometry.Contains(face.facet,point.p);
                if (!contained) result.reason="Native 3D movement proposal leaves an incident immutable facet.";
                candidate=contained ? geometry_trial(MoveVertex(old,vertex,point,first,geometry,fresh,result.reason)) : geometry_trial(false);
              }
            } else if (!move_trial) {
              auto apex=nodes.begin(); std::advance(apex,trial-3);
              candidate=geometry_trial(Reconnect(old,apex->first,first,geometry,fresh,result.reason));
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
    if (chosen && !approved) {
      // A sample belongs to this finally rejected cavity, not a failed trial later rescued elsewhere.
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
        for (const auto& row : removed)
          for (auto key : selection.cells.at(row.cell).edges)
            retired_edges.emplace(key,selection.edges.at(key).measure);
        for (const auto& row : removed) selection.Drop(row.cell);
        for (const auto& row : added) changed.push_back(row.cell.id);
        for (auto row : failures) selection.Block(row.edge,row.feature,repairing);
        if (approved) {
          ++stats.accepted;
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
      if (ready && coarsening) {
        try { for (Id vertex:directory.ChangedVertices()) selection.Touch(vertex); }
        catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        ready=vote(failure);
      }
      retired_edges.clear();
    }
    if (!ready) return finish();
    }
    batch_bytes=0;
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
