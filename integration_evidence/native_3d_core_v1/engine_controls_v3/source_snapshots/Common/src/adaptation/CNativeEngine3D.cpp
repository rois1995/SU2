/*!
 * \file CNativeEngine3D.cpp
 * \brief Incremental frozen-target selection and distributed private planar edge refinement.
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
DonorRegion Region(const Cell& c) {
  const auto p = c.v[0].p;
  DonorRegion b{p.x,p.y,p.z,p.x,p.y,p.z};
  for (auto node : c.v) {
    const double x[3]{node.p.x,node.p.y,node.p.z};
    for (size_t a = 0; a < 3; ++a) { b[a] = std::min(b[a],x[a]); b[a+3] = std::max(b[a+3],x[a]); }
  }
  return b;
}
bool Same(Point a, Point b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
struct Choice { Edge edge{}; EdgeMeasure measure; bool active = false; };
class Selection {
 public:
  size_t invalid = 0;
  struct EdgeState { EdgeMeasure measure; size_t users = 0; };
  struct CellState { std::array<Edge, 6> edges{}; bool valid = false; };
  std::map<Id,CellState> cells;
  std::map<Edge,EdgeState> edges;
  std::set<std::pair<long double,Edge>> queue;
  size_t Bytes() const { return transfer_memory::Add(MapBytes(cells),MapBytes(edges),MapBytes(queue)); }
  Choice Best() const {
    if (queue.empty()) return {};
    const auto key = queue.begin()->second;
    return {key,edges.at(key).measure,true};
  }
  void Cached(const Cell& cell, std::map<Edge,EdgeMeasure>& values) const {
    for (size_t i=0; i<4; ++i) for (size_t j=i+1; j<4; ++j) {
      const Edge key{std::min(cell.v[i].id,cell.v[j].id),std::max(cell.v[i].id,cell.v[j].id)};
      const auto found=edges.find(key);
      if (found!=edges.end()) values.emplace(key,found->second.measure);
    }
  }
  void Block(Edge edge) {
    const auto it = edges.find(edge);
    if (it==edges.end()) return;
    queue.erase({-it->second.measure.length,edge});
  }
  void Drop(Id id) {
    const auto found = cells.find(id);
    if (found==cells.end()) throw std::logic_error("Missing native 3D cached cell removal.");
    invalid -= !found->second.valid;
    for (auto key : found->second.edges) {
      auto& state = edges.at(key);
      queue.erase({-state.measure.length,key});
      if (--state.users==0) edges.erase(key);
      else { if (state.measure.length>MaximumMetricEdge) queue.insert({-state.measure.length,key}); }
    }
    cells.erase(found);
  }
  void Add(const Cell& cell, const PatchMetric& metric, bool valid) {
    if (metric.edges.size()!=6 || !std::isfinite(metric.maximum_length))
      throw std::runtime_error("Incomplete native 3D initial/updated metric measurement.");
    CellState row; row.valid=valid;
    size_t slot=0;
    for (const auto& measured : metric.edges) {
      const auto key=measured.first;
      auto inserted=edges.emplace(key,EdgeState{measured.second,0});
      auto& state=inserted.first->second;
      if (!Same(state.measure.a,measured.second.a) || !Same(state.measure.b,measured.second.b) ||
          state.measure.length!=measured.second.length)
        throw std::logic_error("Inconsistent native 3D frozen edge measurement.");
      ++state.users; row.edges[slot++]=key;
      if (state.measure.length>MaximumMetricEdge) queue.insert({-state.measure.length,key});
    }
    if (!cells.emplace(cell.id,row).second) throw std::logic_error("Duplicate native 3D cached cell.");
    invalid += !valid;
  }
};
struct BlockedEdge {
  Edge edge{};
  template <class S> void Fields(S& s) { s(edge); }
};
void FieldCounts(EngineStats& stats, const FrozenField& field) {
  const auto& f=field.Statistics();
  stats.field_requests+=f.requests; stats.field_hits+=f.hits; stats.field_compositions+=f.compositions;
  stats.field_edges+=f.edge_requests;
  stats.field_build_seconds+=f.build_seconds; stats.field_query_seconds+=f.query_seconds;
  stats.field_cache_seconds+=f.cache_seconds; stats.field_search_seconds+=f.search_seconds;
  stats.field_interpolation_seconds+=f.interpolation_seconds; stats.field_composition_seconds+=f.composition_seconds;
  stats.edge_seconds+=f.edge_seconds; stats.edge_search_seconds+=f.edge_search_seconds;
  stats.edge_trace_seconds+=f.edge_trace_seconds; stats.edge_integral_seconds+=f.edge_integral_seconds;
}
}  // namespace
EngineResult Refine(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                    const PlanarReference& geometry, MetricComposition composition, EngineOptions options) {
  const auto started=Clock::now();
  const auto started_cpu=options.timings ? std::clock() : std::clock_t{};
  EngineResult result;
  auto& stats=result.stats;
  Owned working;
  Selection selection;
  std::map<Edge,EdgeMeasure> retired_edges;
  Directory directory(world);
  DonorImport source(world,options.timings);
  size_t accepted_bytes=MapBytes(accepted);
  std::vector<Id> initial;
  auto vote=[&](const CLocalFailure& local) {
    const auto elected=world.elect(local); result.reason=elected.message; return !elected.any;
  };
  auto bytes=[&]() {
    return transfer_memory::Add(options.caller_bytes,accepted_bytes,MapBytes(working),
        transfer_memory::Bytes(original),transfer_memory::Bytes(initial),transfer_memory::Bytes(result.splits),
        selection.Bytes(),MapBytes(retired_edges));
  };
  auto admit=[&](size_t extra) {
    const auto peak=transfer_memory::Add(bytes(),source.ResidentBytes(),directory.ResidentBytes(),extra);
    stats.maximum_work_bytes=std::max(stats.maximum_work_bytes,peak);
    CLocalFailure failure;
    if (peak==SIZE_MAX || peak>options.memory_ceiling) failure.Set(3,0,"Native 3D refinement working storage exceeds budget.");
    return vote(failure);
  };
  std::string last_rejection;
  auto finish=[&]() {
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
    const auto lo=CPassiveComm::Allreduce(options.maximum_rounds,CPassiveComm::Op::MIN,world.comm);
    const auto hi=CPassiveComm::Allreduce(options.maximum_rounds,CPassiveComm::Op::MAX,world.comm);
    if (lo!=hi || !hi) failure.Set(1,0,"Native 3D round limit must be positive and equal across workers.");
    ready=vote(failure);
    if (ready) ready=admit(transfer_memory::Add(accepted_bytes,
        transfer_memory::Mul(options.maximum_rounds,sizeof(SplitRecord)),transfer_memory::Mul(accepted.size(),2048)));
    if (ready) try {
      working=accepted;
      stats.maximum_owned=working.size();
      result.splits.reserve(options.maximum_rounds);
      initial.reserve(working.size());
      for (const auto& row : working) {
        initial.push_back(row.first); maximum_cell=std::max(maximum_cell,row.first);
        for (auto node : row.second.cell.v) maximum_node=std::max(maximum_node,node.id);
      }
    } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
    if (ready) ready=vote(failure);
    if (ready && !world.maximum(!working.empty())) { failure.Set(1,0,"Native 3D refinement input is empty."); ready=vote(failure); }
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
        std::vector<DonorRegion> regions;
        try {
          for (size_t i=slot;i<std::min(slot+batch,ids.size());++i)
            regions.push_back(Region(working.at(ids[i]).cell));
        } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
        if (!vote(failure)) return false;
        ++stats.donor_batches;
        imported=source.Import(regions,options.memory_ceiling,
            transfer_memory::Add(bytes(),directory.ResidentBytes()));
        if (imported.valid) break;
        if (batch==1) { result.reason=imported.reason; return false; }
        batch=(batch+1)/2; ++stats.batch_retries;
      }
      if (!admit(transfer_memory::Add(transfer_memory::Bytes(imported.cells),
          transfer_memory::Mul(MaximumDonors,8192),transfer_memory::Mul(working.size(),2048)))) return false;
      if (slot<ids.size()) try {
        CpuTimer private_cpu(stats.measurement_cpu_seconds,options.timings);
        FrozenField field(std::move(imported.cells),composition,options.timings);
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
          selection.Add(cell,metric,strict);
          stats.edge_evaluations+=counted.edge_evaluations;
          stats.edge_reused+=counted.edge_reused;
          if (seed) ++stats.initial_cells; else ++stats.updated_cells;
        }
        FieldCounts(stats,field);
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
  for (size_t round=0; round<options.maximum_rounds; ++round) {
    Choice choice;
    bool complete=false,any=false;
    {
      Timer timer(stats.selection_seconds,options.timings);
      ++stats.selections;
      complete=!world.maximum(selection.invalid!=0 || selection.cells.size()!=working.size());
      choice=selection.Best(); any=world.maximum(choice.active);
    }
    if (complete) { result.valid=true; result.reason.clear(); return finish(); }
    if (!any) {
      CLocalFailure failure;
      if (selection.invalid) failure.Set(1,0,"Native 3D refinement stagnates: blocked oversized edges or unsupported shape-only repair.");
      vote(failure); return finish();
    }
    ++stats.rounds; stats.proposed+=choice.active;
    Dependencies dependencies;
    Imported imported;
    {
      Timer timer(stats.dependency_seconds,options.timings);
      dependencies=directory.Lookup(choice.active ? std::vector<Feature>{{{choice.edge[0],choice.edge[1],0},2}} :
          std::vector<Feature>{},options.memory_ceiling,transfer_memory::Add(bytes(),source.ResidentBytes()));
      if (dependencies.valid) imported=directory.Fetch(dependencies,working,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes()));
    }
    if (!dependencies.valid || !imported.valid) {
      result.reason=dependencies.valid ? imported.reason : dependencies.reason; return finish();
    }
    bool chosen=false;
    {
      Timer timer(stats.reservation_seconds,options.timings);
      chosen=directory.Reserve(imported,choice.active,result.reason,options.memory_ceiling,
          transfer_memory::Add(bytes(),source.ResidentBytes()));
      stats.conflicts+=choice.active && !chosen;
      if (!world.sum(chosen)) ready=false;
    }
    if (!ready) return finish();
    DonorPatch sensors;
    std::vector<DonorRegion> regions;
    CLocalFailure failure;
    {
      Timer timer(stats.donor_seconds,options.timings);
      if (chosen) try { for (const auto& row : imported.cells) regions.push_back(Region(row.cell)); }
      catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      ready=vote(failure);
      if (ready) sensors=source.Import(regions,options.memory_ceiling,
          transfer_memory::Add(bytes(),directory.ResidentBytes(),transfer_memory::Bytes(imported.cells)));
    }
    if (!ready || !sensors.valid) { if (ready) result.reason=sensors.reason; return finish(); }
    Patch old,fresh;
    Node point;
    bool approved=false;
    std::unique_ptr<FrozenField> field;
    const auto token=transfer_memory::Add(transfer_memory::Mul(round,world.size),world.rank);
    const auto offset=transfer_memory::Mul(token,MaximumReplacementCells);
    {
      Timer timer(stats.reconstruction_seconds,options.timings);
      if (chosen) {
        CpuTimer private_cpu(stats.reconstruction_cpu_seconds,options.timings);
        ++stats.attempted;
        if (maximum_node==UINT64_MAX || token>=UINT64_MAX-maximum_node ||
            maximum_cell>UINT64_MAX-MaximumReplacementCells || offset>UINT64_MAX-maximum_cell-MaximumReplacementCells) {
          failure.Set(1,0,"Native 3D reserved identity range overflows.");
        } else try {
          old=GeometryPatch(imported);
          const auto a=choice.measure.a,b=choice.measure.b;
          point={maximum_node+1+token,{double((static_cast<long double>(a.x)+b.x)/2),
              double((static_cast<long double>(a.y)+b.y)/2),double((static_cast<long double>(a.z)+b.z)/2)}};
          approved=SplitEdge(old,choice.edge,point,maximum_cell+1+offset,geometry,fresh,result.reason);
        } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      }
      ready=vote(failure);
    }
    if (!ready) return finish();
    {
      Timer timer(stats.metric_seconds,options.timings);
      if (approved) try {
        CpuTimer private_cpu(stats.metric_cpu_seconds,options.timings);
        field=std::make_unique<FrozenField>(std::move(sensors.cells),composition,options.timings);
        PatchMetric metric;
        std::map<Edge,EdgeMeasure> reuse;
        for (const auto& cell : old.cells) selection.Cached(cell,reuse);
        approved=ValidateMetricChange(old.cells,fresh.cells,MetricChange::Refine,*field,metric,result.reason,nullptr,&reuse);
        FieldCounts(stats,*field);
      } catch (const std::exception& e) { approved=false; result.reason=e.what(); }
    }
    if (chosen && !approved) last_rejection = "edge " + std::to_string(choice.edge[0]) + "," +
        std::to_string(choice.edge[1]) + ": " + result.reason;
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
          if (fresh.cells.size()!=2*old.cells.size()) throw std::logic_error("Unexpected native tetrahedral split layout.");
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
            additions[owners.at(old.cells[i/2].id)].push_back(row);
          }
        } else if (chosen) {
          std::set<int> owners;
          for (const auto& row : dependencies.cells) owners.insert(row.owner);
          for (int owner : owners) blocked[owner].push_back({choice.edge});
        }
      } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      ready=vote(failure);
      if (ready) {
        bool admitted=false;
        const auto baseline=transfer_memory::Add(bytes(),source.ResidentBytes(),directory.ResidentBytes(),
            transfer_memory::Bytes(imported.cells),transfer_memory::Mul(MaximumDonors,8192));
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
          transfer_memory::Add(bytes(),source.ResidentBytes(),transfer_memory::Mul(MaximumDonors,8192)));
    }
    if (!ready) return finish();
    std::vector<Id> changed;
    {
      Timer timer(stats.update_seconds,options.timings);
      ready=admit(transfer_memory::Add(transfer_memory::Mul(working.size(),2048),
          transfer_memory::Mul(6*MaximumCavityCells,sizeof(std::pair<const Edge,EdgeMeasure>)+4*sizeof(void*))));
      if (ready) try {
        // Retain retired scalars until all children are measured against this event's immutable source.
        for (const auto& row : removed)
          for (auto key : selection.cells.at(row.cell).edges)
            retired_edges.emplace(key,selection.edges.at(key).measure);
        if (retired_edges.size()>6*MaximumCavityCells)
          throw std::runtime_error("Native 3D retired edge reuse exceeds cavity budget.");
        for (const auto& row : removed) selection.Drop(row.cell);
        for (const auto& row : added) changed.push_back(row.cell.id);
        for (auto row : failures) selection.Block(row.edge);
        if (approved) { ++stats.accepted; result.splits.push_back({choice.edge,point}); }
        stats.maximum_owned=std::max(stats.maximum_owned,working.size());
      } catch (const std::exception& e) { failure.Set(2,0,e.what()); }
      if (ready) ready=vote(failure);
      if (ready) ready=measure(changed,false);
      retired_edges.clear();
    }
    if (!ready) return finish();
  }
  {
    Timer timer(stats.completion_seconds,options.timings);
    result.valid=!world.maximum(selection.invalid!=0 || selection.cells.size()!=working.size());
    if (!result.valid) result.reason="Native 3D refinement round limit reached before strict metric completion.";
    else result.reason.clear();
  }
  return finish();
}
}  // namespace SU2Native3D
