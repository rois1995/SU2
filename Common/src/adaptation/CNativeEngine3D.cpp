/*!
 * \file CNativeEngine3D.cpp
 * \brief Incremental frozen-target selection and distributed private planar refinement/coarsening.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#include "../../include/adaptation/CNativeEngine3D.hpp"
#include "../../include/adaptation/CNativeMetricTransport3D.hpp"
#include "../../include/adaptation/CNativeOptimize3D.hpp"
#include "../../include/adaptation/CNativeRegionField3D.hpp"
#include "../../include/adaptation/CNativePointConstruction3D.hpp"
#include "../../include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../include/adaptation/CNativeGeometryBackend3D.hpp"
#include "../../include/adaptation/CNativeGenerationProgress3D.hpp"
#include "../../include/adaptation/CNativePartition3D.hpp"
#include "../../include/adaptation/CNativeImport3D.hpp"
#include "../../include/adaptation/CNativeTimerTree3D.hpp"
#include "../../include/adaptation/CNativeD9Profile3D.hpp"
#include "../../include/adaptation/CNativeWallDiagnostics3D.hpp"
#include "../../include/adaptation/CNativeWallGeometryLaw3D.hpp"
#include "../../include/adaptation/CNativeLaneCensus3D.hpp"
#include "../../include/adaptation/CNativeRuntimeProfile3D.hpp"
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
#include <optional>
#include <utility>
namespace SU2Native3D {
static_assert(4 * MaximumRepairBatch <= MaximumDirectoryFeatures,
              "Curved repair support must fit in one directory query.");

AdaptationRequirements MakeAdaptationRequirements(const PlanarReference& geometry, bool composition,
                                                  const EngineOptions& options) {
  AdaptationRequirements result;
  result.size.controls = options.requirement_size;
  result.size.composition_present = composition;
  result.geometry.mapped = geometry.Mapped();
  result.geometry.entity_geometry = geometry.EntityGeometry();
  result.geometry.curved = !geometry.ExtensionComponents().empty();
  result.geometry.tolerance = options.surface_tolerance;
  if (const auto* policy = geometry.Mobility()) {
    result.features.policy_present = true;
    result.features.features = policy->features.size();
    result.features.corners = policy->corners.size();
    std::set<Id> roots;
    for (const auto& sheet : policy->sheets) roots.insert(sheet.second);
    result.features.sheets = roots.size();
    result.features.angle = policy->angle;
  }
  result.walls.first_heights = &options.wall_heights;
  result.walls.region_policies = &options.region_walls;
  result.walls.height_tolerance = options.wall_heights.tolerance;
  result.budgets = {options.maximum_work, options.maximum_seconds, options.memory_ceiling, options.job_memory_limit};
  return result;
}

void PrintAdaptationRequirements(const AdaptationRequirements& r, int rank, std::ostream& out) {
  const auto precision = out.precision();
  out << std::setprecision(17) << "NATIVE3D_REQUIREMENTS rank=" << rank << " hmin=" << r.size.controls.hmin
      << " hmax=" << r.size.controls.hmax << " ARmax=" << r.size.controls.armax << " hgrad=" << r.size.controls.hgrad
      << " complexity=" << r.size.controls.complexity << " composition_present=" << r.size.composition_present
      << " mapped=" << r.geometry.mapped << " entity_geometry=" << r.geometry.entity_geometry
      << " curved=" << r.geometry.curved << " tolerance=" << r.geometry.tolerance
      << " policy_present=" << r.features.policy_present << " features=" << r.features.features
      << " corners=" << r.features.corners << " sheets=" << r.features.sheets << " angle=" << r.features.angle
      << " height_tolerance=" << r.walls.height_tolerance << " maximum_work=" << r.budgets.maximum_work
      << " maximum_seconds=" << r.budgets.maximum_seconds << " memory_ceiling=" << r.budgets.memory_ceiling
      << " job_memory_limit=" << r.budgets.job_memory_limit << " wall_rows=";
  std::set<int> markers;
  for (const auto& wall : *r.walls.first_heights) markers.insert(wall.first);
  for (const auto& region : *r.walls.region_policies) markers.insert(region.first);
  bool first = true;
  for (const int marker : markers) {
    if (!first) out << ';';
    first = false;
    const auto height = r.walls.first_heights->find(marker);
    const auto region = r.walls.region_policies->find(marker);
    const double h0 = height != r.walls.first_heights->end() ? height->second : region->second.height;
    const double growth =
        region != r.walls.region_policies->end() ? region->second.growth : std::numeric_limits<double>::quiet_NaN();
    const double thickness =
        region != r.walls.region_policies->end() ? region->second.thickness : std::numeric_limits<double>::quiet_NaN();
    out << marker << ':' << h0 << ',' << growth << ',' << thickness;
  }
  out << '\n';
  out.precision(precision);
}

namespace {
namespace wall_diagnostics = SU2NativeWallDiagnostics3D;
namespace lane_census = SU2NativeLaneCensus3D;

// Face-edge incidences deliberately include an edge once for each owned physical face.
void PrintWallLawCensus(const Owned& cells, const PlanarReference& geometry, double tau, int rank, const char* boundary,
                        bool enabled) {
  const auto* parameters = geometry.Parameters();
  if (!enabled || !parameters || !parameters->EntityGeometry()) return;
  SU2NativeTiming3D::Scope timer(SU2NativeTiming3D::Operation::Census);
  const auto started = std::chrono::steady_clock::now();
  size_t faces = 0, incidences = 0, need = 0, unsupported = 0;
  double max_ratio = 0;
  for (const auto& row : cells) {
    for (unsigned opposite = 0; opposite < 4; ++opposite) {
      const auto& binding = row.second.physical[opposite];
      if (!binding.present) continue;
      ++faces;
      std::array<unsigned, 3> vertices{};
      unsigned count = 0;
      for (unsigned k = 0; k < 4; ++k)
        if (k != opposite) vertices[count++] = k;
      for (unsigned k = 0; k < 3; ++k) {
        ++incidences;
        const auto estimate = EstimateWallEdge(*parameters, row.second.cell.v[vertices[k]],
                                               row.second.cell.v[vertices[(k + 1) % 3]], binding.facet);
        if (!estimate.supported) {
          ++unsupported;
          continue;
        }
        need += EstimateSaysNeed(estimate, tau);
        max_ratio = std::max(max_ratio, estimate.value / (.75 * tau));
      }
    }
  }
  const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
  std::cout << "NATIVE3D_WALL_LAW rank=" << rank << " boundary=" << boundary << " faces=" << faces
            << " edge_incidences=" << incidences << " need_incidences=" << need
            << " unsupported_incidences=" << unsupported << " max_ratio=" << max_ratio << " seconds=" << seconds
            << '\n';
}
// First-failure precedence from I.B0; only labels an already rejected guidance check.
wall_diagnostics::GuidanceCause WallGuidanceCause(bool complete, bool finite, bool needs_progress, bool progress) {
  if (!complete) return wall_diagnostics::GuidanceCause::Incomplete;
  if (!finite) return wall_diagnostics::GuidanceCause::Nonfinite;
  if (needs_progress && !progress) return wall_diagnostics::GuidanceCause::NoProgress;
  return wall_diagnostics::GuidanceCause::AfterAbove;
}

// Attribute existing bound work without issuing or resetting geometry queries.
struct D9QueryScope {
  SU2NativeD9::Data* data = nullptr;
  const SU2NativeGeometry::MappedFacetReference* authority = nullptr;
  SU2NativeGeometry::MappedFacetReference::GeometryStats before{};
  SU2NativeD9::Clock::time_point start{};
  int phase = -1;
  SU2NativeD9::Site site;
  SU2NativeD9::PartScope part;
  D9QueryScope(SU2NativeD9::Site value, const PlanarReference* geometry)
      : site(value), part(SU2NativeD9::SitePart(value)) {
    phase = SU2NativeD9::Phase();
    if (phase < 0 || !geometry || !geometry->Parameters()) return;
    data = SU2NativeD9::active;
    authority = &geometry->Parameters()->Geometry();
    before = authority->GeometryStatsSnapshot();
    start = SU2NativeD9::Clock::now();
  }
  ~D9QueryScope() {
    if (!data) return;
    const auto seconds = SU2NativeD9::Seconds(start);
    const auto after = authority->GeometryStatsSnapshot();
    auto& row = data->queries[size_t(phase)][size_t(site)];
    ++row.scopes;
    row.direct += after.direct_calls - before.direct_calls;
    row.upper += after.upper_calls - before.upper_calls;
    row.classify += after.classification_calls - before.classification_calls;
    row.magnitude += after.magnitude_calls - before.magnitude_calls;
    row.limit += after.limit_calls - before.limit_calls;
    row.memo_direct += after.memo_direct_hits - before.memo_direct_hits;
    row.memo_union += after.memo_union_hits - before.memo_union_hits;
    row.work += after.upper_work - before.upper_work;
    row.direct_seconds += after.direct_seconds - before.direct_seconds;
    row.upper_seconds += after.upper_seconds - before.upper_seconds;
    row.seconds += seconds;
  }
};

// M27 attributes the existing exact checks to the patch being measured.
template <class... Arguments>
GeometryGuidance GuidanceOf(SU2NativeD9::Part part, Arguments&&... arguments) {
  SU2NativeD9::PartScope scope(part);
  return GeometryGuidanceExcess(std::forward<Arguments>(arguments)...);
}

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
/*! Retain legacy counters while adding exclusive service rows to the active F3 ledger. */
class Timer {
 public:
  /*! Start both counters only when the existing timings flag permits clocks. */
  Timer(double& value, bool enabled, double* parent=nullptr,
        SU2NativeTiming3D::Operation operation = SU2NativeTiming3D::Operation::Unattributed) : value(value), enabled(enabled), parent(parent) { if (enabled) {
      start = Clock::now();
      scope.emplace(operation);
    }
  }
  /*! Label an existing legacy service that has no parent-subtraction counter. */
  Timer(double& value, bool enabled, SU2NativeTiming3D::Operation operation)
      : Timer(value, enabled, nullptr, operation) {}
  ~Timer() { if (enabled) {const auto elapsed=std::chrono::duration<double>(Clock::now()-start).count();value+=elapsed;if(parent)*parent-=elapsed;} }
 private:
  double& value;
  bool enabled;
  double* parent;  // Keep a nested scope exclusive when its parent is also reported as a phase.
  Clock::time_point start;
  std::optional<SU2NativeTiming3D::Scope> scope;
};
/*! Direct collectives bypass World: name their wall time without changing its legacy counters. */
template <class T>
T TimedAllreduce(T value, CPassiveComm::Op operation, CPassiveComm::Communicator communicator) {
  SU2NativeTiming3D::Scope communication_timer(SU2NativeTiming3D::Operation::Communication);
  return CPassiveComm::Allreduce(value, operation, communicator);
}

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
/*! Keep the owned-face buffers inside the existing event reservation until certification ends. */
struct ProofStorage {
  SU2NativeGeometry::AtlasBudget& budget;
  size_t caller;
  ProofStorage(SU2NativeGeometry::AtlasBudget& value, size_t extra) : budget(value), caller(value.caller_bytes) {
    const auto live = transfer_memory::Add(caller, extra);
    if (live == SIZE_MAX || live > budget.maximum_bytes)
      throw std::length_error("Owned face proof exceeds storage admission.");
    budget.caller_bytes = live;
    budget.peak_bytes = std::max(budget.peak_bytes, live);
  }
  ~ProofStorage() { budget.caller_bytes = caller; }
};
/*! Combine the disjoint owned-face reverse proofs before the unchanged entity proof. */
void ProveOwnedSurface(World& world, const Owned& working, const PlanarReference& geometry,
                       SU2NativeGeometry::AtlasBudget& budget, EngineStats& stats) {
  const auto& state = *geometry.Parameters();
  std::vector<SurfaceFace> faces;
  std::vector<double> bounds;
  CLocalFailure failure;
  std::optional<ProofStorage> storage;
  try {
    storage.emplace(budget, transfer_memory::Add(transfer_memory::Mul(working.size(), 8 * sizeof(SurfaceFace)),
        transfer_memory::Mul(state.Geometry().Reference().Facets().size(), 2 * sizeof(double))));
    for (const auto& row : working) {
      const auto cell_faces = Faces(row.second.cell);
      for (unsigned i = 0; i < 4; ++i)
        if (row.second.physical[i].present) faces.push_back({cell_faces[i], row.second.physical[i].facet});
    }
    bounds = state.OwnedFaceReverseBounds(faces, budget);
  } catch (const std::exception& error) {
    failure.Set(2, 0, error.what());
  }
  const auto elected = world.elect(failure);
  if (elected.any) throw std::runtime_error(elected.message);
  stats.final_proof_faces = faces.size();
  stats.final_proof_surface_faces = state.Surface().size();
  if (world.reduce(uint64_t(faces.size()), CPassiveComm::Op::SUM) != state.Surface().size())
    throw std::runtime_error("Owned wall-face proof does not cover the complete working surface exactly once.");
  for (auto& bound : bounds) bound = world.reduce(bound, CPassiveComm::Op::MAX);
  for (const auto& row : state.GeometryCertificates(budget, true, &bounds))
    if (!row.second.within_tolerance)
      throw std::runtime_error("Complete working surface does not meet the prescribed two-sided geometry tolerance.");
}
/*! Report the ownership spike with live payload sizes; this is not a memory admission model. */
void ReportOwnershipMemory(const World& world, const char* boundary, size_t pending = 0, size_t outgoing = 0,
                           size_t records = 0) {
  SU2NativeTiming3D::Scope timer(SU2NativeTiming3D::Operation::Output);
  std::cout << "NATIVE3D_OWNERSHIP_MEMORY rank=" << world.rank << " boundary=" << boundary
            << " current_rss=" << CurrentRSS3D() << " process_peak_rss=" << ProcessPeakRSS3D()
            << " max_exchange_work_bytes=" << world.max_exchange_work_bytes << " bytes_sent=" << world.bytes_sent
            << " pending_bytes=" << pending << " outgoing_bytes=" << outgoing << " record_bytes=" << records << '\n';
}
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
  bool fair_repair=false;
};
struct CellQualityFlag {Id cell=0;bool q_bad=false,j_bad=false;};
struct FixedMetricPoint {
  Id cell=0;int owner=0;Point point;Tensor baseline;
  template<class S>void Fields(S& s){s(cell,owner,point,baseline);}
};
struct FixedMetricReading {
  uint64_t sample=0;Id cell=0;int owner=0;Tensor current;
  template<class S>void Fields(S& s){s(sample,cell,owner,current);}
};
void AccountQualityChurn(const Patch& old,const Patch& fresh,const std::vector<CellQualityFlag>& before,
                        const std::vector<CellQualityFlag>& after,QualityRepairStats& stats) {
  if(before.size()!=old.cells.size()||after.size()!=fresh.cells.size()) {
    stats.unmeasured_churn+=old.cells.size()+fresh.cells.size();return;
  }
  using Key=std::array<std::array<double,3>,4>;
  auto key=[](const Cell& cell){Key out;for(unsigned i=0;i<4;++i)out[i]={cell.v[i].p.x,cell.v[i].p.y,cell.v[i].p.z};
    std::sort(out.begin(),out.end());return out;};
  std::map<Key,std::array<bool,2>> prior;
  for(size_t i=0;i<old.cells.size();++i) {
    if(before[i].cell!=old.cells[i].id||!prior.emplace(key(old.cells[i]),std::array<bool,2>{{before[i].q_bad,before[i].j_bad}}).second) {
      stats.unmeasured_churn+=old.cells.size()+fresh.cells.size();return;
    }
  }
  QualityRepairStats change;
  for(size_t i=0;i<fresh.cells.size();++i) {
    if(after[i].cell!=fresh.cells[i].id){stats.unmeasured_churn+=old.cells.size()+fresh.cells.size();return;}
    const auto found=prior.find(key(fresh.cells[i]));
    if(found==prior.end()) {
      change.q_created+=after[i].q_bad;change.q_introduced+=after[i].q_bad;
      change.j_created+=after[i].j_bad;change.j_introduced+=after[i].j_bad;
    } else {
      change.q_removed+=found->second[0]&&!after[i].q_bad;change.q_created+=!found->second[0]&&after[i].q_bad;
      change.j_removed+=found->second[1]&&!after[i].j_bad;change.j_created+=!found->second[1]&&after[i].j_bad;
      prior.erase(found);
    }
  }
  for(const auto& row:prior) {
    change.q_removed+=row.second[0];change.q_retired+=row.second[0];
    change.j_removed+=row.second[1];change.j_retired+=row.second[1];
  }
  stats.q_removed+=change.q_removed;stats.q_created+=change.q_created;stats.j_removed+=change.j_removed;stats.j_created+=change.j_created;
  stats.q_retired+=change.q_retired;stats.q_introduced+=change.q_introduced;
  stats.j_retired+=change.j_retired;stats.j_introduced+=change.j_introduced;
}
struct ProposalKey {
  Feature feature;int rank=0;size_t slot=0;bool geometric=false;double score=0;Id cell=0;uint64_t epoch=0;
  bool fair_repair=false;
  template<class S> void Fields(S& s){s(feature,rank,slot,geometric,score,cell,epoch,fair_repair);}
};
struct RepairReceipt {
  Feature feature;bool accepted=false;unsigned disposition=4;
  template<class S> void Fields(S& s){s(feature,accepted,disposition);}
};
template<class S>void CachedScalar(S& s,long double& value) {
  if(!std::isfinite(value))throw std::logic_error("Nonfinite migrated cache scalar.");
  int exponent=0;bool negative=std::signbit(value);
  std::array<long double,4> words{{std::frexp(std::abs(value),&exponent),0,0,0}};
  s(exponent,negative);OriginalWeights3D(s,words);value=std::ldexp(words[0],exponent);if(negative)value=-value;
}
class Selection {
 public:
  enum class Lane {Surface,Interior};
  bool split_lanes=false;
  Lane lane=Lane::Surface;
  Selection(bool coarsening, bool repairing, const PlanarReference& geometry, EngineStats& stats, bool timings,
            double tolerance, const WallHeights& heights, bool quality_only, bool surface_planning = false,
            wall_diagnostics::Counters* wall_counters = nullptr)
      : coarsening(coarsening),
        track_repairs(repairing),
        repair_event(repairing),
        quality_only(quality_only),
        surface_planning(surface_planning),
        geometry(geometry),
        stats(stats),
        timings(timings),
        tolerance(tolerance),
        heights(heights),
        wall_counters(wall_counters) {}
  bool coarsening, track_repairs, repair_event, prune_collapses = false, track_splits = false;
  const bool quality_only;
  const bool surface_planning;
  bool size_repairs=false,geometry_repairs=false;
  const PlanarReference& geometry;
  EngineStats& stats;
  bool timings;
  double tolerance;
  const WallHeights& heights;
  wall_diagnostics::Counters* wall_counters;
  std::set<std::tuple<long double,Id,Edge>> geometry_queue;
  std::map<Edge,long double> geometry_split_priorities;
  bool geometry_priority_mode=false;
  long double GeometryPriority(long double raw,Edge edge)const {
    if(!geometry_priority_mode)return raw;
    const auto found=geometry_split_priorities.find(edge);
    return found!=geometry_split_priorities.end()&&found->second>1 ? found->second : raw/(1+raw);
  }
  size_t GeometryPriorityBytes()const {
    return transfer_memory::Mul(geometry_queue.size(),sizeof(std::pair<const Edge,long double>)+4*sizeof(void*));
  }
  void PrioritizeGeometrySplits(Clock::time_point started,double maximum_seconds) {
    geometry_split_priorities.clear();geometry_priority_mode=true;
    auto pending=std::move(geometry_queue);geometry_queue.clear();
    while(!pending.empty()) {
      auto entry=pending.extract(pending.begin());const auto edge=std::get<2>(entry.value());
      if(!geometry_split_priorities.count(edge)&&
          (!maximum_seconds||std::chrono::duration<double>(Clock::now()-started).count()<maximum_seconds)) {
        const auto& measured=edges.at(edge).measure;
        Timer timer(stats.geometry_guidance_seconds,timings, SU2NativeTiming3D::Operation::GeometryBound);++stats.geometry_guidance_trials;
        const auto prediction=PredictGeometrySplit(edge,{measured.a,measured.b},geometry,tolerance);
        stats.geometry_guidance_faces+=prediction.evaluated_faces;
        const bool better=prediction.complete&&prediction.before>0&&prediction.after<prediction.before*(1-1e-12L);
        geometry_split_priorities.emplace(edge,better?1+(prediction.before-prediction.after)/prediction.before:-1.L);
      }
      const auto id=std::get<1>(entry.value());const auto& row=cells.at(id);
      const auto found=std::find(row.edges.begin(),row.edges.end(),edge);
      if(found==row.edges.end())throw std::logic_error("Geometry priority lost its versioned edge.");
      std::get<0>(entry.value())=-GeometryPriority(row.geometry_scores[found-row.edges.begin()],edge);
      geometry_queue.insert(std::move(entry));
    }
  }
  std::set<std::pair<long double,Edge>> surface_queue;
  // One construction generation contains the edges present at its entry, including edges of
  // cells whose metric has not been progressively seeded yet. Publications cannot extend it.
  std::set<Edge> construction_frontier;
  struct History {uint64_t age=0,round=0;unsigned disposition=0;};
  std::map<Edge,History> edge_history;
  void Note(Edge edge,unsigned disposition) {
    std::sort(edge.begin(),edge.end());const auto found=edge_history.find(edge);
    if(found!=edge_history.end()){found->second.round=stats.rounds;found->second.disposition=disposition;}
  }
  void RefreshHistory(bool generation) {
    for(auto it=edge_history.begin();it!=edge_history.end();) {
      const auto edge=edges.find(it->first);
      if(edge==edges.end()||edge->second.measure.length<=MaximumMetricEdge)it=edge_history.erase(it);else ++it;
    }
    for(const auto& edge:edges)if(edge.second.measure.length>MaximumMetricEdge) {
      auto& history=edge_history[edge.first];if(generation)++history.age;
    }
  }
  long double Priority(long double length) const { return coarsening ? length : -length; }
  bool Candidate(long double length) const {
    return coarsening ? length < CoarseningMetricEdge : length > MaximumMetricEdge;
  }
  void Queue(Edge edge, long double length, bool insert) {
    if (repair_event || !Candidate(length)) return;
    for (unsigned i=0; i<(coarsening ? 2u : 1u); ++i) {
      Edge key=edge; std::sort(key.begin(),key.end());
      const auto value=std::make_pair(Priority(length),edge);
      auto& destination=split_lanes&&!coarsening&&edges.at(key).physical_users ? surface_queue : queue;
      const auto erased=queue.erase(value)+surface_queue.erase(value);
      if(surface_planning&&insert&&!edges.at(key).physical_users){std::swap(edge[0],edge[1]);continue;}
      const auto blocked=blocked_edges.find(edge[1]);
      if(insert && blocked!=blocked_edges.end() && blocked->second.count(edge[0])) {
        // Measuring another unchanged cell cannot reopen a rejected direction; Touch owns reactivation.
        queue.erase({Priority(length),edge});++stats.blocked_queue_skips;
      } else if(insert && !construction_frontier.count(edge))queue.erase({Priority(length),edge});
      else if (insert && prune_collapses && edges.at(key).forbidden[i])
        stats.collapse_filtered += erased;
      else if (insert) destination.insert(value);
      std::swap(edge[0],edge[1]);
    }
  }
  size_t invalid = 0, construction_invalid = 0, quality_invalid = 0, unmeasured_shape = 0, repair_bytes = 0;
  struct EdgeState { EdgeMeasure measure; size_t users = 0,physical_users=0; std::array<size_t,2> forbidden{}; };
  struct CellDebt {
    long double q=0,j=0,size=0,height=0,geometry=0;
    bool q_measured=false,j_measured=false,height_measured=false,geometry_measured=true,height_unresolved=false;
    bool q_bad=false,j_bad=false,size_bad=false,height_bad=false;
    template<class S>void Fields(S& s) {
      CachedScalar(s,q);CachedScalar(s,j);CachedScalar(s,size);CachedScalar(s,height);CachedScalar(s,geometry);
      s(q_measured,j_measured,height_measured,geometry_measured,height_unresolved,q_bad,j_bad,size_bad,height_bad);
    }
  };
  StageProgress debt;
  bool progress_consistent=true,census_started=false;
  struct CellState { uint64_t version=0; CellDebt debt;double q_value=0,j_value=0;std::array<Edge, 6> edges{}; std::array<unsigned char,6> forbidden{};std::array<bool,6> physical{}; std::array<Id,4> vertices{}; std::array<long double,6> geometry_scores{}; bool valid = false,target_valid=false,geometry_invalid=false,construction_valid=false,quality_valid=false,shape_measured=false;
    bool surface=false,near_feature=false,feature_known=false;
    unsigned char geometry_types=0,geometry_excess_bin=0;
    template<class S>void Fields(S& s) {
      s(version,debt,q_value,j_value,edges,forbidden,physical,vertices);
      for(auto& x:geometry_scores)CachedScalar(s,x);
      s(valid,target_valid,geometry_invalid,construction_valid,quality_valid,shape_measured);
      s(surface,near_feature,feature_known,geometry_types,geometry_excess_bin);
    }
  };
  struct CachedRow {
    Id id=0;CellState state;std::array<long double,6> lengths{};
    std::array<uint64_t,14> repair_epochs{};uint16_t repair_active=0,repair_blocked=0;
    template<class S>void Fields(S& s){s(id,state);for(auto& x:lengths)CachedScalar(s,x);s(repair_epochs,repair_active,repair_blocked);}
  };
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
  std::map<Id,long double> repair_cell_severity;
  std::map<RepairFeature,long double> repair_severity;
  std::set<std::pair<long double,RepairFeature>> repair_severity_queue;
  std::set<RepairFeature> repair_zero;
  size_t Bytes() const {
    return transfer_memory::Add(MapBytes(cells),MapBytes(edges),MapBytes(queue),MapBytes(surface_queue),MapBytes(edge_history),MapBytes(construction_frontier),MapBytes(cell_repairs),MapBytes(repair_users),MapBytes(repair_queue),MapBytes(repair_cell_severity),MapBytes(repair_severity),MapBytes(repair_severity_queue),MapBytes(repair_zero),repair_bytes,
        MapBytes(geometry_queue),MapBytes(geometry_split_priorities),MapBytes(vertex_cells),MapBytes(blocked_edges),transfer_memory::Mul(transfer_memory::Add(vertex_links,blocked_links),sizeof(Id)+4*sizeof(void*)));
  }
  long double RepairWeight(const CellState& row) const {
    // Cached local required debt steers search; only the complete distributed transaction certifies the feature.
    const auto& d=row.debt;long double weight=0;
    if(surface_planning)return d.size+d.geometry+(row.geometry_invalid||d.size_bad?1.L:0.L);
    if(d.q_measured)weight+=d.q;else if(!row.quality_valid)weight+=1;
    if(d.j_measured)weight+=d.j;else if(!row.quality_valid)weight+=1;
    if(!heights.empty())weight+=d.height+(d.height_unresolved||d.height_bad?1.L:0.L);
    if(size_repairs)weight+=d.size+(d.size_bad?1.L:0.L);
    if(geometry_repairs)weight+=d.geometry+(!d.geometry_measured||row.geometry_invalid?1.L:0.L);
    if(!std::isfinite(weight)||weight<0)throw std::logic_error("Nonfinite native repair severity.");
    return weight>0?weight:1.L;
  }
  void AddRepairSeverity(const RepairFeature& key,long double value) {
    if(!std::isfinite(value)||value<0)throw std::logic_error("Invalid native repair severity contribution.");
    auto& total=repair_severity[key];
    if(repair_users.at(key).users)repair_severity_queue.erase({-total,key});
    total+=value;
    if(!std::isfinite(total))throw std::logic_error("Nonfinite native repair severity sum.");
    const auto& demand=repair_users.at(key);
    if(demand.users&&!demand.blocked)repair_severity_queue.insert({-total,key});
  }
  void RemoveRepairSeverity(const RepairFeature& key,long double value) {
    const auto found=repair_severity.find(key);if(found==repair_severity.end())throw std::logic_error("Missing native repair severity.");
    auto& total=found->second;repair_severity_queue.erase({-total,key});
    total=std::max(0.L,total-value);
  }
  void UpdateRepairWeight(Id id,const CellState& row) {
    const auto found=cell_repairs.find(id);if(found==cell_repairs.end())return;
    const long double prior=repair_cell_severity.at(id),fresh=RepairWeight(row);
    if(prior==fresh)return;
    for(const auto& key:found->second)RemoveRepairSeverity(key,prior);
    for(const auto& key:found->second)AddRepairSeverity(key,fresh);
    repair_cell_severity[id]=fresh;
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
  bool RequestedWall(const OwnedCell& owned)const {
    if(heights.empty())return false;
    return std::any_of(owned.physical.begin(),owned.physical.end(),[&](const FacetBinding& f) {
      return f.present && heights.count(geometry.Get(f.facet).marker);
    });
  }
  // Demand coverage only: unmeasured volume rows remain unknown in the public full-mesh census.
  std::array<size_t,3> SurfaceCensus(const Owned& owned)const {
    std::array<size_t,3> count{}; // required, current measured, current geometry measured
    for(const auto& item:owned)if(RequestedWall(item.second)) {
      ++count[0];const auto found=cells.find(item.first);
      if(found!=cells.end()&&found->second.version==item.second.version) {
        ++count[1];count[2]+=found->second.debt.geometry_measured;
      }
    }
    return count;
  }
  StageProgress Progress(size_t owned)const {
    StageProgress result=progress_consistent?debt:StageProgress{};
    result.cache_consistent=progress_consistent;result.owned_cells=owned;
    result.measured_cells=progress_consistent?cells.size():0;
    result.invalid_cells=progress_consistent?invalid:0;
    result.construction_invalid_cells=progress_consistent?construction_invalid:0;
    result.quality_invalid_cells=progress_consistent?quality_invalid:0;
    result.remaining_edges=queue.size()+surface_queue.size()+geometry_queue.size();result.remaining_repairs=repair_queue.size();
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
  bool Diagnose(const OwnedCell& owned,CellDiagnostic& out)const {
    out={};const auto found=cells.find(owned.cell.id);
    if(!progress_consistent||found==cells.end()||found->second.version!=owned.version)return false;
    const auto& row=found->second;const auto& d=row.debt;
    out.q_measured=d.q_measured;out.j_measured=d.j_measured;out.geometry_measured=d.geometry_measured;
    out.q=row.q_value;out.J=row.j_value;out.geometry_excess_l2=double(std::sqrt(std::max(0.L,d.geometry)));
    out.q_bad=d.q_bad;out.j_bad=d.j_bad;out.geometry_bad=row.geometry_invalid;out.size_bad=d.size_bad;
    out.height_bad=d.height_bad;out.height_unresolved=d.height_unresolved;out.surface=row.surface;
    out.near_feature=row.near_feature;out.feature_known=row.feature_known;out.edges=row.edges;
    for(size_t i=0;i<6;++i){const auto edge=edges.find(row.edges[i]);if(edge==edges.end())return false;out.lengths[i]=edge->second.measure.length;}
    const auto repairs=cell_repairs.find(owned.cell.id);
    if(repairs!=cell_repairs.end())for(const auto& key:repairs->second) {
      if(out.repair_count==out.repairs.size())return false;
      const auto demand=repair_users.find(key);if(demand==repair_users.end())return false;
      out.repairs[out.repair_count++]={Feature{key.second,key.first},demand->second.epoch,demand->second.blocked};
    }
    out.cache_valid=true;return true;
  }
  Choice Best(bool repairing) const {
    if (repairing) {
      if (repair_queue.empty()) return {};
      const auto& oldest=*repair_queue.begin();const auto& key=oldest.second;
      Choice choice; choice.active=true; choice.feature={key.second,key.first};choice.repair_epoch=oldest.first;choice.fair_repair=true;return choice;
    }
    if ((!split_lanes||coarsening||lane==Lane::Surface) && !geometry_queue.empty()) {
      const auto key=std::get<2>(*geometry_queue.begin());
      return {key,edges.at(key).measure,true,{},true,std::get<0>(*geometry_queue.begin()),std::get<1>(*geometry_queue.begin())};
    }
    const auto& active=split_lanes&&!coarsening&&lane==Lane::Surface ? surface_queue : queue;
    if (active.empty()) return {};
    const auto key = active.begin()->second;
    Edge canonical=key; std::sort(canonical.begin(),canonical.end());
    return {key,edges.at(canonical).measure,true,{},false,Priority(edges.at(canonical).measure.length)};
  }
  std::vector<Choice> RepairBatch(const Owned& working,size_t limit,bool& oversized) const {
    oversized=false;std::vector<Choice> result;if(!limit||repair_queue.empty())return result;
    auto feature_cells=[&](const RepairFeature& key,std::vector<Id>& ids,std::set<Id>& vertices,bool& too_large) {
      const auto count=key.first;const auto& feature=key.second;
      if(count<1||count>3)throw std::logic_error("Invalid native repair feature.");
      const std::set<Id>* seed=nullptr;
      for(unsigned i=0;i<count;++i) {
        const auto found=vertex_cells.find(feature[i]);if(found==vertex_cells.end())return;
        if(!seed||found->second.size()<seed->size())seed=&found->second;
      }
      size_t scanned=0;
      for(Id id:*seed) {
        if(++scanned>MaximumDirectoryScan){too_large=true;return;}
        const auto found=working.find(id);if(found==working.end())throw std::logic_error("Repair star lost an owned cell.");
        const auto& cell=found->second.cell;bool contains=true;
        for(unsigned i=0;i<count;++i)contains=contains&&std::any_of(cell.v.begin(),cell.v.end(),[&](Node node){return node.id==feature[i];});
        if(!contains)continue;
        ids.push_back(id);for(const auto& node:cell.v)vertices.insert(node.id);
        if(ids.size()>MaximumCavityCells){too_large=true;return;}
      }
    };
    auto make_choice=[&](const RepairFeature& key,uint64_t epoch,bool fair) {
      Choice choice;choice.active=true;choice.feature={key.second,key.first};choice.repair_epoch=epoch;
      choice.fair_repair=fair;if(!fair)choice.priority_score=-repair_severity.at(key);return choice;
    };
    const auto oldest=*repair_queue.begin();const auto first=make_choice(oldest.second,oldest.first,true);
    std::vector<Id> first_cells;std::set<Id> used;bool first_oversized=false;
    feature_cells(oldest.second,first_cells,used,first_oversized);
    if(first_oversized){oversized=true;result.push_back(first);return result;}
    result.push_back(first);std::set<RepairFeature> selected{oldest.second};size_t scanned=0;
    for(const auto& priority:repair_severity_queue) {
      if(result.size()>=limit||scanned++>=MaximumDirectoryScan)break;
      const auto& key=priority.second;if(selected.count(key))continue;
      const auto demand=repair_users.find(key);
      if(demand==repair_users.end()||!demand->second.users||demand->second.blocked)continue;
      Choice choice=make_choice(key,demand->second.epoch,false);
      std::vector<Id> star_cells;std::set<Id> star_vertices;bool star_oversized=false;
      feature_cells(key,star_cells,star_vertices,star_oversized);if(star_oversized)continue;
      bool disjoint=true;for(Id vertex:star_vertices)disjoint=disjoint&&!used.count(vertex);
      if(!disjoint)continue;
      result.push_back(choice);selected.insert(key);used.insert(star_vertices.begin(),star_vertices.end());
    }
    return result;
  }
  std::vector<Choice> Batch(const Owned& working, size_t limit) {
    std::vector<Choice> result;
    std::set<Id> used;
    size_t scanned=0;
    const auto& active=split_lanes&&!coarsening&&lane==Lane::Surface ? surface_queue : queue;
    for (auto it=active.begin();it!=active.end() && result.size()<limit && scanned++<MaximumDirectoryScan;) {
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
    Note(edge,4);
    ServiceEdge(edge);
    Edge canonical=edge;std::sort(canonical.begin(),canonical.end());
    const auto it = edges.find(canonical);
    if(it!=edges.end()){queue.erase({Priority(it->second.measure.length),edge});surface_queue.erase({Priority(it->second.measure.length),edge});}
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
  void TouchRepairSupport(const std::array<Id,4>& vertices) {
    // Only a tetrahedron containing the whole feature changes its complete imported star.
    for(unsigned mask=1;mask<15;++mask) {
      RepairFeature key{};
      for(unsigned i=0;i<4;++i)if(mask&(1u<<i))key.second[key.first++]=vertices[i];
      const auto found=repair_users.find(key);
      if(found==repair_users.end() || !found->second.users || !found->second.blocked)continue;
      auto& demand=found->second;demand.blocked=false;demand.epoch=repair_epoch;
      repair_queue.insert({demand.epoch,key});repair_severity_queue.insert({-repair_severity.at(key),key});
    }
  }
  void Touch(Id vertex) {
    const auto found=blocked_edges.find(vertex);
    if (found==blocked_edges.end()) return;
    auto reactivated=std::move(found->second);
    blocked_links-=reactivated.size();blocked_edges.erase(found);
    for (Id retained:reactivated) {
      Edge edge{retained,vertex}, key=edge; std::sort(key.begin(),key.end());
      const auto measured=edges.find(key);
      if (measured==edges.end())continue;
      if(!coarsening && !geometry.ExtensionComponents().empty()) {
        const auto star=vertex_cells.find(vertex);
        if(star!=vertex_cells.end())for(Id id:star->second) {
          const auto& row=cells.at(id);
          for(size_t k=0;k<row.edges.size();++k)if(row.edges[k]==key && row.geometry_scores[k]>0)
            if(construction_frontier.count(key))stats.geometry_reactivated+=geometry_queue.insert({-GeometryPriority(row.geometry_scores[k],key),id,key}).second;
        }
      }
      if(!Candidate(measured->second.measure.length))continue;
      const auto direction=retained==key[0] ? 0u : 1u;
      if (prune_collapses && measured->second.forbidden[direction]) continue;
      if (!coarsening) edge=key;
      if(!construction_frontier.count(edge))continue;
      const auto count=queue.size()+surface_queue.size();Queue(key,measured->second.measure.length,true);
      const bool inserted=queue.size()+surface_queue.size()>count;
      if (coarsening) stats.collapse_reactivated+=inserted;
    }
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
      Timer timer(stats.collapse_geometry_seconds,timings, SU2NativeTiming3D::Operation::GeometryBound);
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
    Timer timer(stats.collapse_orientation_seconds,timings, SU2NativeTiming3D::Operation::Topology);
    size_t scanned=0;
    for (Id id:star->second) {
      if (++scanned>MaximumDirectoryScan) break;
      auto v=working.at(id).cell.v;
      if (std::any_of(v.begin(),v.end(),[&](Node n){return n.id==choice.edge[0];})) continue;
      for (auto& node:v) if (node.id==choice.edge[1]) node={choice.edge[0],apex};
      ++stats.collapse_orientation_checks;
      if (!(Orientation(v[0].p,v[1].p,v[2].p,v[3].p)>0)) {
        // Surface planning may repair connectivity before contracting; all final validity checks remain.
        if(surface_planning && !geometry.ExtensionComponents().empty())return false;
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
    for(size_t i=0;i<6;++i)geometry_queue.erase({-GeometryPriority(found->second.geometry_scores[i],found->second.edges[i]),id,found->second.edges[i]});
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
      state.physical_users-=found->second.physical[slot];
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
    const long double weight=repair_cell_severity.at(id);
    for(const auto& key:it->second) {
      RemoveRepairSeverity(key,weight);
      auto& demand=repair_users.at(key);
      if(--demand.users==0)repair_zero.insert(key);
      else if(!demand.blocked)repair_severity_queue.insert({-repair_severity.at(key),key});
    }
    repair_cell_severity.erase(id);
    cell_repairs.erase(it);
  }
  void UpdateRepairs(const Cell& cell,const CellState& row) {
    if(!track_repairs)return;
    const bool needed=surface_planning?!row.construction_valid:quality_only?(!row.quality_valid || (geometry_repairs && row.geometry_invalid) || (size_repairs && row.debt.size_bad)):!row.valid;
    if(needed&&!cell_repairs.count(cell.id))AddRepairs(cell,quality_only && size_repairs && row.quality_valid && !geometry_repairs);
    else if(!needed&&cell_repairs.count(cell.id))RemoveRepairs(cell.id);
  }
  void ChangePhase(bool improvement,const Owned& working,const std::shared_ptr<const FrozenMetricIdentity>& identity) {
    geometry_split_priorities.clear();geometry_priority_mode=false;
    size_repairs=geometry_repairs=false;
    track_repairs=repair_event=improvement;prune_collapses=false;queue.clear();surface_queue.clear();geometry_queue.clear();construction_frontier.clear();
    for(auto& item:cells) {
      auto& row=item.second;const auto found=working.find(item.first);
      if(found==working.end()||found->second.version!=row.version)throw std::logic_error("Phase census lost a versioned owned row.");
      const auto& cell=found->second.cell;
      for(unsigned i=0;i<4;++i)if(cell.v[i].id!=row.vertices[i])throw std::logic_error("Phase census changed owned vertices without a version.");
      for(const auto& edge:row.edges) {const auto& cached=edges.at(edge).measure;const Node *a=nullptr,*b=nullptr;
        for(const auto& node:cell.v){if(node.id==edge[0])a=&node;if(node.id==edge[1])b=&node;}
        if(!a||!b||!cached.Reusable(a->p,b->p,identity))throw std::logic_error("Phase census changed edge coordinates or field without a version.");}
      const bool before=row.valid;invalid-=!before;
      row.target_valid=improvement&&!surface_planning ? (!row.debt.size_bad&&row.quality_valid) : !row.debt.size_bad;
      row.valid=row.target_valid&&!row.geometry_invalid;invalid+=!row.valid;
      UpdateRepairs(found->second.cell,row);UpdateRepairWeight(item.first,row);
    }
    FinishRepairs();
    if(!improvement) {
      SeedFrontier(working);
      for(const auto& row:edges)Queue(row.first,row.second.measure.length,true);
      if(!coarsening)for(const auto& item:cells)for(size_t i=0;i<6;++i){const auto edge=item.second.edges[i];const auto blocked=blocked_edges.find(edge[1]);
        if(item.second.geometry_scores[i]>0 && construction_frontier.count(edge) && (blocked==blocked_edges.end()||!blocked->second.count(edge[0])))
          geometry_queue.insert({-GeometryPriority(item.second.geometry_scores[i],edge),item.first,edge});}
    }
  }
  void EnableSizeRepairs(const Owned& working) {
    size_repairs=true;
    for(const auto& row:working){UpdateRepairs(row.second.cell,cells.at(row.first));UpdateRepairWeight(row.first,cells.at(row.first));}
    FinishRepairs();
  }
  void EnableGeometryRepairs(const Owned& working) {
    size_repairs=false;geometry_repairs=true;
    for(const auto& row:working){UpdateRepairs(row.second.cell,cells.at(row.first));UpdateRepairWeight(row.first,cells.at(row.first));}
    FinishRepairs();
  }
  void ClassifyFeatures(const Cell& cell,CellState& row) const {
    const auto* parameters=geometry.Parameters();const auto* mobility=geometry.Mobility();
    row.near_feature=false;
    row.feature_known=parameters&&mobility&&mobility->identity;
    if(row.feature_known)try {
      for(const auto& node:cell.v) {
        const auto* point=parameters->Find(node.id);
        if(point&&parameters->Geometry().MobilityDimension(point->binding,*mobility)<2)row.near_feature=true;
      }
    }catch(const std::exception&){row.feature_known=row.near_feature=false;}
  }
  void RefreshQuality(const OwnedCell& owned,const PatchMetric& metric,bool valid,bool quality_valid,
                      const FirstHeightReport* height,bool height_valid,const std::shared_ptr<const FrozenMetricIdentity>& identity) {
    auto& row=cells.at(owned.cell.id);if(row.version!=owned.version)throw std::logic_error("Quality refresh found stale owned version.");
    ClassifyFeatures(owned.cell,row);
    if(metric.edges.size()!=6||!std::isfinite(metric.maximum_length))throw std::invalid_argument("Incomplete quality refresh edge measurements.");
    for(const auto& key:row.edges){const auto measured=metric.edges.find(key);if(measured==metric.edges.end())throw std::logic_error("Quality refresh omitted cached edge.");
      const auto& prior=edges.at(key).measure;const auto& fresh=measured->second;
      if(!prior.Reusable(fresh.a,fresh.b,identity)||!fresh.Reusable(prior.a,prior.b,identity)||prior.length!=fresh.length)
        throw std::logic_error("Quality refresh changed cached edge certificate.");}
    Account(row,false);invalid-=!row.valid;quality_invalid-=!row.quality_valid;unmeasured_shape-=!row.shape_measured;
    auto& d=row.debt;auto hinge=[](long double x){return std::max(0.L,x);};
    row.shape_measured=metric.shape_measured;row.quality_valid=quality_valid;
    row.q_value=metric.shape_measured?metric.minimum_mean_ratio:0;row.j_value=metric.shape_measured?metric.minimum_scaled_jacobian:0;
    d.q_measured=metric.shape_measured&&std::isfinite(metric.minimum_mean_ratio);d.j_measured=metric.shape_measured&&std::isfinite(metric.minimum_scaled_jacobian);
    d.q=d.j=0;d.q_bad=d.j_bad=false;
    if(d.q_measured){const auto r=hinge(1.L-static_cast<long double>(metric.minimum_mean_ratio)/MinimumMeanRatio);d.q=r*r;d.q_bad=metric.minimum_mean_ratio<MinimumMeanRatio;}
    if(d.j_measured){const auto r=hinge(1.L-static_cast<long double>(metric.minimum_scaled_jacobian)/MinimumScaledJacobian);d.j=r*r;d.j_bad=metric.minimum_scaled_jacobian<MinimumScaledJacobian;}
    d.height=0;d.height_measured=d.height_bad=d.height_unresolved=false;
    if(height&&!heights.empty()){d.height_measured=height->measured&&std::isfinite(height->maximum_relative_error)&&std::isfinite(height->minimum_barycentric);
      d.height_bad=!height_valid;d.height_unresolved=!d.height_measured||height->missing_cells||height->reference_clearance_failures;
      if(d.height_measured){const auto r=hinge(static_cast<long double>(height->maximum_relative_error)/heights.tolerance-1.L),f=hinge(-3.L*height->minimum_barycentric);d.height=r*r+f*f;}}
    row.target_valid=valid;row.valid=valid&&!row.geometry_invalid;Account(row,true);invalid+=!row.valid;quality_invalid+=!row.quality_valid;unmeasured_shape+=!row.shape_measured;
    UpdateRepairs(owned.cell,row);UpdateRepairWeight(owned.cell.id,row);FinishRepairs();
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
    Timer timer(stats.collapse_geometry_seconds,timings, SU2NativeTiming3D::Operation::GeometryBound);
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
    D9QueryScope d9_query(SU2NativeD9::Site::Census, &geometry);
    const auto& cell=owned.cell;
    if (metric.edges.size()!=6 || !std::isfinite(metric.maximum_length))
      throw std::runtime_error(reason.empty()?"Incomplete native 3D initial/updated metric measurement.":reason);
    CellState row; row.version=owned.version;row.valid=row.target_valid=valid;row.shape_measured=metric.shape_measured;row.quality_valid=quality_valid;
    row.surface=std::any_of(owned.physical.begin(),owned.physical.end(),[](const FacetBinding& face){return face.present;});
    const auto* parameters=geometry.Parameters();
    if(metric.shape_measured)ClassifyFeatures(cell,row);
    row.construction_valid=metric.maximum_length<=MaximumMetricEdge;
    auto hinge=[](long double value){return std::max(0.L,value);};
    auto& d=row.debt;
    row.q_value=metric.shape_measured?metric.minimum_mean_ratio:0;row.j_value=metric.shape_measured?metric.minimum_scaled_jacobian:0;
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
      if(surface_planning) {
        bool requested=false;
        for(unsigned f=0;f<4;++f)if(owned.physical[f].present&&heights.count(geometry.Get(owned.physical[f].facet).marker)&&
            cell.v[f].id!=edge.first[0]&&cell.v[f].id!=edge.first[1])requested=true;
        if(!requested)continue;
      }
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
        if(surface_planning&&!heights.count(geometry.Get(owned.physical[slot].facet).marker))continue;
        std::array<Point,3> triangle{};
        for(size_t i=0;i<3;++i)for(auto n:cell.v)if(n.id==faces[slot][i])triangle[i]=n.p;
        const double error=geometry.SurfaceDeviation(owned.physical[slot].facet,triangle,SU2NativeGeometry::SurfaceQueryNeed::Magnitude());
        const auto allowed=tolerance;
        if(surface_planning && wall_counters)
          wall_counters->ObserveMeasuredFace(faces[slot],owned.physical[slot].facet,error,tolerance);
        if(std::isfinite(error)) {
          const auto excess=hinge(static_cast<long double>(error)-allowed);d.geometry+=excess*excess;
        } else d.geometry_measured=false;
        if(error<=allowed)continue;
        unsigned type=0;const Id facet=owned.physical[slot].facet;
        const auto analytic=geometry.Analytic().find(facet);
        if(analytic!=geometry.Analytic().end())type=unsigned(analytic->second.kind);
        else if(geometry.Affine().count(facet))type=5;
        else if(parameters&&parameters->Geometry().Curves().count(facet))type=6;
        row.geometry_types|=static_cast<unsigned char>(1u<<type);
        unsigned bin=5;
        if(std::isfinite(error)&&allowed>0) {
          const auto excess=static_cast<long double>(error)/allowed-1;
          bin=excess<.01L?0:excess<.1L?1:excess<1?2:excess<10?3:4;
        }
        row.geometry_excess_bin=std::max(row.geometry_excess_bin,static_cast<unsigned char>(bin));
        row.valid=false;row.geometry_invalid=true;
        WallFaceWeights law_weights;
        if (surface_planning && std::isfinite(error)) {
          std::array<WallEdgeEstimate, 3> estimates{};
          if (parameters) {
            size_t edge = 0;
            for (size_t i = 0; i < 3; ++i)
              for (size_t j = i + 1; j < 3; ++j)
                estimates[edge++] =
                    EstimateWallEdge(*parameters, {faces[slot][i], triangle[i]}, {faces[slot][j], triangle[j]}, facet);
          }
          law_weights = WallFaceEdgeWeights(estimates);
          if (wall_counters) wall_counters->ObserveLawFace(law_weights.weighted);
        }
        size_t face_edge = 0;
        for (size_t i = 0; i < 3; ++i)
          for (size_t j = i + 1; j < 3; ++j, ++face_edge) {
            const auto a = triangle[i], b = triangle[j];
            const Edge key{std::min(faces[slot][i], faces[slot][j]), std::max(faces[slot][i], faces[slot][j])};
            if (!std::isfinite(error)) continue;
            const long double excess = static_cast<long double>(error) - allowed;
            const long double x = static_cast<long double>(b.x) - a.x, y = static_cast<long double>(b.y) - a.y,
                              z = static_cast<long double>(b.z) - a.z;
            // Queue current geometry demand; the selected complete star owns projection and progress validation.
            if (surface_planning)
              geometry_scores[key] += excess * excess * (x * x + y * y + z * z) * law_weights.factors[face_edge];
            else
              geometry_scores[key] += excess * excess * (x * x + y * y + z * z);
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
        geometry_queue.insert({-GeometryPriority(row.geometry_scores[slot],key),cell.id,key});
      auto inserted=edges.emplace(key,EdgeState{measured.second,0});
      auto& state=inserted.first->second;
      Queue(key,state.measure.length,false);
      if (!Same(state.measure.a,measured.second.a) || !Same(state.measure.b,measured.second.b) ||
          state.measure.length!=measured.second.length)
        throw std::logic_error("Inconsistent native 3D frozen edge measurement.");
      row.forbidden[slot]=Candidate(measured.second.length) ? Forbidden(owned,key) : 0;
      for (unsigned direction=0;direction<2;++direction)
        state.forbidden[direction]+=bool(row.forbidden[slot] & (1u<<direction));
      bool physical=false;for(unsigned face=0;face<4;++face)if(owned.physical[face].present &&
          (!surface_planning||heights.count(geometry.Get(owned.physical[face].facet).marker)) &&
          cell.v[face].id!=key[0] && cell.v[face].id!=key[1])physical=true;
      state.physical_users+=physical;row.physical[slot]=physical;
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
    UpdateRepairs(cell,inserted.first->second);
  }
  void AddRepairs(const Cell& cell,bool length_only=false) {
    auto& repairs=cell_repairs[cell.id];
    if (!repairs.empty()) throw std::logic_error("Duplicate native 3D repair queue seed.");
    if(!length_only)for (auto node:cell.v) repairs.push_back({1,{node.id,0,0}});
    for (auto edge:cells.at(cell.id).edges)
      if(!length_only || edges.at(edge).measure.length>MaximumMetricEdge)repairs.push_back({2,{edge[0],edge[1],0}});
    if(!length_only)for (auto face:Faces(cell)) repairs.push_back({3,Key(face)});
    for (auto key:repairs) {
      const auto inserted=repair_users.emplace(key,RepairDemand{0,repair_epoch,false});
      auto& demand=inserted.first->second;
      if(demand.users++==0 && !demand.blocked)repair_queue.insert({demand.epoch,key});
    }
    const long double weight=RepairWeight(cells.at(cell.id));repair_cell_severity[cell.id]=weight;
    for(const auto& key:repairs)AddRepairSeverity(key,weight);
    repair_bytes=transfer_memory::Add(repair_bytes,transfer_memory::Bytes(repairs));
  }
  void Service(Feature feature,bool accepted) {
    const RepairFeature key{feature.count,feature.vertices};
    const auto found=repair_users.find(key);if(found==repair_users.end())return;
    auto& demand=found->second;repair_queue.erase({demand.epoch,key});repair_severity_queue.erase({-repair_severity.at(key),key});
    demand.epoch=repair_epoch;demand.blocked=!accepted;
    if(accepted && demand.users){repair_queue.insert({demand.epoch,key});repair_severity_queue.insert({-repair_severity.at(key),key});}
  }
  void FinishRepairs() {
    for(const auto& key:repair_zero) {
      const auto found=repair_users.find(key);
      if(found!=repair_users.end() && !found->second.users) {
        repair_queue.erase({found->second.epoch,key});repair_severity_queue.erase({-repair_severity.at(key),key});
        repair_severity.erase(key);repair_users.erase(found);
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
    if(measured!=edges.end()){queue.erase({Priority(measured->second.measure.length),direction});surface_queue.erase({Priority(measured->second.measure.length),direction});}
    const auto star=vertex_cells.find(edge[0]);
    if(star!=vertex_cells.end())for(Id id:star->second) {
      const auto& row=cells.at(id);
      for(size_t slot=0;slot<row.edges.size();++slot)if(row.edges[slot]==edge)
        geometry_queue.erase({-GeometryPriority(row.geometry_scores[slot],edge),id,edge});
    }
    return serviced;
  }
  size_t ServiceClaimedVertex(Id vertex) {
    // Full old/new vertex claims retire every edge whose paired-face prediction could have changed.
    const auto star=vertex_cells.find(vertex);if(star==vertex_cells.end())return 0;
    size_t serviced=0;
    for(Id id:star->second) {
      const auto& row=cells.at(id);
      if(!coarsening) {
        for(const auto& edge:row.edges)if(construction_frontier.count(edge)&&
            (!split_lanes||edges.at(edge).physical_users)){Note(edge,3);serviced+=ServiceEdge(edge);}
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
  void Restore(const OwnedCell& owned,const CachedRow& record,const std::shared_ptr<const FrozenMetricIdentity>& identity) {
    const auto& row=record.state;const auto& cell=owned.cell;
    if(record.id!=cell.id || row.version!=owned.version)throw std::logic_error("Migrated selection version mismatch.");
    for(unsigned i=0;i<4;++i) {
      if(row.vertices[i]!=cell.v[i].id)throw std::logic_error("Migrated selection vertex mismatch.");
      vertex_links+=vertex_cells[row.vertices[i]].insert(cell.id).second;
    }
    if(!cells.emplace(cell.id,row).second)throw std::logic_error("Duplicate migrated selection row.");
    for(unsigned i=0;i<6;++i) {
      const auto edge=row.edges[i];Point a{},b{};bool has_a=false,has_b=false;
      for(auto node:cell.v){if(node.id==edge[0]){a=node.p;has_a=true;}if(node.id==edge[1]){b=node.p;has_b=true;}}
      if(!has_a||!has_b||!(record.lengths[i]>0))throw std::logic_error("Invalid migrated edge certificate.");
      auto inserted=edges.emplace(edge,EdgeState{EdgeMeasure{a,b,record.lengths[i],identity},0});auto& e=inserted.first->second;
      if(!e.measure.Reusable(a,b,identity)||e.measure.length!=record.lengths[i])throw std::logic_error("Migrated edge certificates disagree.");
      e.physical_users+=row.physical[i];++e.users;
      for(unsigned d=0;d<2;++d)e.forbidden[d]+=bool(row.forbidden[i]&(1u<<d));
    }
    Account(row,true);invalid+=!row.valid;construction_invalid+=!row.construction_valid;
    quality_invalid+=!row.quality_valid;unmeasured_shape+=!row.shape_measured;
  }
  static std::array<RepairFeature,14> RepairKeys(const CellState& row) {
    std::array<RepairFeature,14> keys{};unsigned at=0;
    for(Id vertex:row.vertices)keys[at++]={1,{vertex,0,0}};
    for(const auto& edge:row.edges)keys[at++]={2,{edge[0],edge[1],0}};
    for(unsigned opposite=0;opposite<4;++opposite) {
      Face face{};unsigned k=0;for(unsigned i=0;i<4;++i)if(i!=opposite)face[k++]=row.vertices[i];
      keys[at++]={3,Key(face)};
    }
    return keys;
  }
  void SaveRepairState(CachedRow& record)const {
    const auto found=cell_repairs.find(record.id);if(found==cell_repairs.end())return;
    const auto keys=RepairKeys(record.state);size_t saved=0;
    for(unsigned i=0;i<keys.size();++i) {
      if(std::find(found->second.begin(),found->second.end(),keys[i])==found->second.end())continue;
      const auto& demand=repair_users.at(keys[i]);
      if(!demand.users||demand.epoch>repair_epoch)throw std::logic_error("Invalid phase repair history.");
      record.repair_active|=uint16_t(1u<<i);record.repair_epochs[i]=demand.epoch;
      if(demand.blocked)record.repair_blocked|=uint16_t(1u<<i);
      ++saved;
    }
    if(saved!=found->second.size())throw std::logic_error("Incomplete phase repair history.");
  }
  void RestoreRepairState(const std::vector<CachedRow>& records) {
    if(!cell_repairs.empty()||!repair_users.empty()||!repair_zero.empty())
      throw std::logic_error("Phase repair history must restore into an empty queue.");
    for(const auto& record:records) {
      if((record.repair_active&~uint16_t(0x3fff)) || (record.repair_blocked&~record.repair_active))
        throw std::logic_error("Invalid migrated repair masks.");
      const auto& row=cells.at(record.id);
      const bool needed=track_repairs&&(surface_planning?!row.construction_valid:quality_only?(!row.quality_valid || (geometry_repairs&&row.geometry_invalid) ||
          (size_repairs&&row.debt.size_bad)):!row.valid);
      if(bool(record.repair_active)!=needed)throw std::logic_error("Migration changed required repair support.");
      if(!needed)continue;
      const auto keys=RepairKeys(row);auto& repairs=cell_repairs[record.id];repairs.reserve(keys.size());
      const auto weight=RepairWeight(row);repair_cell_severity.emplace(record.id,weight);
      for(unsigned i=0;i<keys.size();++i) {
        if(!(record.repair_active&(1u<<i)))continue;
        if(record.repair_epochs[i]>repair_epoch)throw std::logic_error("Migrated repair epoch is in the future.");
        repairs.push_back(keys[i]);
        const bool blocked=record.repair_blocked&(1u<<i);
        auto& demand=repair_users.emplace(keys[i],RepairDemand{0,record.repair_epochs[i],blocked}).first->second;
        ++demand.users;
        demand.epoch=std::min(demand.epoch,record.repair_epochs[i]);
        demand.blocked=demand.blocked&&blocked;
        auto& severity=repair_severity[keys[i]];severity+=weight;
        if(!std::isfinite(severity))throw std::logic_error("Nonfinite migrated repair severity.");
      }
      repair_bytes=transfer_memory::Add(repair_bytes,transfer_memory::Bytes(repairs));
    }
    // A merged feature stays unavailable only if every prior contributor rejected its unchanged support.
    for(const auto& row:repair_users) {
      const auto& demand=row.second;
      if(!demand.users)throw std::logic_error("Missing migrated repair history.");
      if(!demand.blocked){repair_queue.insert({demand.epoch,row.first});repair_severity_queue.insert({-repair_severity.at(row.first),row.first});}
    }
  }
  void SwapState(Selection& other) noexcept {
    cells.swap(other.cells);edges.swap(other.edges);queue.swap(other.queue);surface_queue.swap(other.surface_queue);
    geometry_queue.swap(other.geometry_queue);geometry_split_priorities.swap(other.geometry_split_priorities);
    std::swap(geometry_priority_mode,other.geometry_priority_mode);
    construction_frontier.swap(other.construction_frontier);edge_history.swap(other.edge_history);
    vertex_cells.swap(other.vertex_cells);blocked_edges.swap(other.blocked_edges);cell_repairs.swap(other.cell_repairs);
    repair_users.swap(other.repair_users);repair_queue.swap(other.repair_queue);
    repair_cell_severity.swap(other.repair_cell_severity);repair_severity.swap(other.repair_severity);
    repair_severity_queue.swap(other.repair_severity_queue);repair_zero.swap(other.repair_zero);
    std::swap(vertex_links,other.vertex_links);std::swap(blocked_links,other.blocked_links);std::swap(repair_bytes,other.repair_bytes);
    std::swap(invalid,other.invalid);std::swap(construction_invalid,other.construction_invalid);
    std::swap(quality_invalid,other.quality_invalid);std::swap(unmeasured_shape,other.unmeasured_shape);
    std::swap(debt,other.debt);std::swap(progress_consistent,other.progress_consistent);std::swap(census_started,other.census_started);
  }
  void Reset(bool improvement) {
    geometry_split_priorities.clear();geometry_priority_mode=false;
    cells.clear();edges.clear();queue.clear();surface_queue.clear();geometry_queue.clear();construction_frontier.clear();edge_history.clear();vertex_cells.clear();
    cell_repairs.clear();repair_users.clear();repair_queue.clear();repair_cell_severity.clear();
    repair_severity.clear();repair_severity_queue.clear();repair_zero.clear();
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
struct OptimizationOperations {
  uint64_t moves=0,insertions=0,collapses=0,swaps=0;
  uint64_t boundary_stars=0,boundary_reused=0,boundary_candidates=0,boundary_validity=0,boundary_shape=0,boundary_edge=0,boundary_geometry=0;
  uint64_t boundary_move_contexts=0,boundary_prepared_trials=0;
  uint64_t boundary_guidance=0,boundary_guidance_faces=0,boundary_guidance_reused_faces=0,boundary_guidance_early=0,boundary_upper=0,boundary_moves=0;long double boundary_debt=0;double boundary_seconds=0;
  uint64_t interior_stars=0,interior_reused=0,interior_candidates=0,interior_validity=0,interior_shape=0,interior_edge=0,interior_geometry=0;
  uint64_t interior_guidance=0,interior_upper=0,interior_moves=0;long double interior_debt=0;double interior_seconds=0;
};
bool OptimizedRepairPatch(const Patch& old,Id first,Id first_node,const RegionMetric& target,
                          Patch& fresh,std::string& reason,size_t& work,bool preserve_vertices,
                          const PlanarReference& geometry,const WallHeights& heights,double surface_tolerance,
                          OptimizationOperations& operations,double maximum_seconds) {
  operations={};
  Owned local;std::map<Face,Id> physical;for(const auto& f:old.surface)physical.emplace(Key(f.v),f.facet);
  for(auto c:old.cells){c.id=local.size();OwnedCell row;row.cell=c;const auto faces=Faces(c);
    for(unsigned k=0;k<4;++k){const auto found=physical.find(Key(faces[k]));if(found!=physical.end())row.physical[k]={true,found->second};}
    local.emplace(c.id,row);}
  auto parameters=geometry.PrivateParameters(old);
  OptimizeOptions options;options.maximum_work=work;options.maximum_seconds=maximum_seconds;options.cache_entries=8*MaximumReplacementCells;
  options.boundary=false;options.smoothing=!preserve_vertices;options.collapses=!preserve_vertices;
  options.geometry=parameters?parameters.get():&geometry;options.wall_heights=heights;options.surface_tolerance=surface_tolerance;
  OptimizeReport report;Id node=first_node,cell=local.size();
  const bool changed=OptimizeMetricMesh(local,target,node,cell,options,report);
  operations.moves=report.moves;operations.insertions=report.shell_insertions+report.neighbourhood_insertions+report.length_insertions;
  operations.collapses=report.collapses+report.boundary_collapses;operations.swaps=report.edge_removals+report.length_removals+report.face_swaps+report.boundary_flips;
  operations.boundary_stars=report.boundary_stars_searched;operations.boundary_reused=report.boundary_stars_reused;operations.boundary_candidates=report.boundary_candidates;
  operations.boundary_validity=report.boundary_rejected_validity;operations.boundary_shape=report.boundary_rejected_shape;operations.boundary_edge=report.boundary_rejected_edge;operations.boundary_geometry=report.boundary_rejected_geometry;
  operations.boundary_guidance=report.boundary_guidance_calls;operations.boundary_upper=report.boundary_upper_queries;operations.boundary_moves=report.boundary_moves;
  operations.boundary_move_contexts=report.boundary_move_contexts;operations.boundary_prepared_trials=report.boundary_prepared_trials;
  operations.boundary_guidance_faces=report.boundary_guidance_faces;operations.boundary_guidance_reused_faces=report.boundary_guidance_reused_faces;operations.boundary_guidance_early=report.boundary_guidance_early_rejections;
  operations.boundary_debt=report.boundary_debt_reduction;operations.boundary_seconds=report.boundary_seconds;
  operations.interior_stars=report.interior_stars_searched;operations.interior_reused=report.interior_stars_reused;operations.interior_candidates=report.interior_candidates;
  operations.interior_validity=report.interior_rejected_validity;operations.interior_shape=report.interior_rejected_shape;operations.interior_edge=report.interior_rejected_edge;operations.interior_geometry=report.interior_rejected_geometry;
  operations.interior_guidance=report.interior_guidance_calls;operations.interior_upper=report.interior_upper_queries;operations.interior_moves=report.interior_moves;
  operations.interior_debt=report.interior_debt_reduction;operations.interior_seconds=report.interior_seconds;
  if(report.trials>work)throw std::logic_error("Native repair optimizer exceeded its work reservation.");
  if(node-first_node>report.trials)throw std::logic_error("Native repair optimizer exceeded its node reservation.");
  work=report.trials;
  if(!changed){reason="Native3D cavity optimizer found no admissible quality/length change ("+report.stop+").";return false;}
  if(local.size()>MaximumReplacementCells){reason="Native3D cavity optimizer exceeds replacement admission.";return false;}
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
  return true;
}
struct RestoreAdmission {
  ShapeAdmission previous = GetShapeAdmission();
  ~RestoreAdmission() { SetShapeAdmission(previous); }
};
struct RestoreBudgetWindow {
  SU2NativeGeometry::AtlasBudget* budget;
  size_t caller, bytes, bits, maximum_work;
  ~RestoreBudgetWindow() {
    if (budget) {
      budget->caller_bytes = caller;
      budget->maximum_bytes = bytes;
      budget->maximum_integer_bits = bits;
      budget->maximum_work = maximum_work;
    }
  }
};
struct GenerationDebt {
  double edges = 0, geometry = 0, maximum = 0;
  bool geometry_complete = false;
};
struct CoarseningCheckpoint {
  Owned cells;
  std::unique_ptr<BoundaryParameterState> parameters;
  std::unique_ptr<Directory> directory;
  std::map<Id, std::set<Id>> blocked;
  std::set<Edge> edges, attempted;
  std::set<Id> changed;
  size_t blocked_links = 0, global_cells = 0, collapses = 0, splits = 0;
  bool valid = false, trial = false;
  size_t Bytes() const {
    return transfer_memory::Add(MapBytes(cells), MapBytes(blocked), MapBytes(edges), MapBytes(attempted),
                                MapBytes(changed), transfer_memory::Mul(blocked_links, sizeof(Id) + 4 * sizeof(void*)),
                                parameters ? parameters->ResidentBytes() : 0,
                                directory ? transfer_memory::Add(sizeof(Directory), directory->ResidentBytes()) : 0);
  }
};
struct RepairProposal {
  Choice choice;
  Dependencies dependencies;
  Imported imported;
  Patch old, fresh;
  Node point;
  MetricDiagnosis certified;
  OptimizationOperations optimization;
  bool chosen = false, approved = false, coordinate_only = false, moved = false, repair_approved = false,
       cavity_inserted = false;
  bool optimization_approved = false, geometric_progress = false, capture = false, capture_selected = false;
  Id first_cell = 0, first_node = 0;
  size_t optimizer_work = 0;
  unsigned ladder = 3;
  std::vector<CellQualityFlag> old_flags, fresh_flags;
  std::string rejection;
};

// Used by print-only batch and per-slot wall commit diagnostics.
bool WallLinkLocal(const Patch&, const BoundaryParameterState*, const Selection&, const Owned&);

// Keep setup ownership private to the original translation unit and namespace.
#include "native_engine3d/EngineSetup.cpp"
#include "native_engine3d/EngineControl.cpp"
#include "native_engine3d/EngineCensus.cpp"
#include "native_engine3d/EngineQueues.cpp"
#include "native_engine3d/EngineExchange.cpp"
#include "native_engine3d/EngineCommit.cpp"
#include "native_engine3d/EngineOwnership.cpp"
#include "native_engine3d/EngineRound.cpp"
#include "native_engine3d/EngineSlot.cpp"
#include "native_engine3d/EngineContext.cpp"
#include "native_engine3d/EngineTrial.cpp"
#include "native_engine3d/EngineLegacyWall.cpp"
#include "native_engine3d/EngineReports.cpp"

// Read-only old-state footprint; never participates in publication or acceptance.
bool WallLinkLocal(const Patch& old, const BoundaryParameterState* parameters, const Selection& selection,
                   const Owned& working) {
  if (!parameters) return old.surface.empty();
  std::set<Id> vertices;
  for (const auto& face : old.surface) vertices.insert(face.v.begin(), face.v.end());
  for (const auto vertex : vertices) {
    const auto incident = parameters->FacesOnVertex(vertex);
    for (auto face = incident.first; face != incident.second; ++face) {
      bool local = false;
      const auto star = selection.vertex_cells.find(vertex);
      if (star != selection.vertex_cells.end())
        for (const auto cell : star->second) {
          const auto row = working.find(cell);
          if (row == working.end()) continue;
          const auto faces = Faces(row->second.cell);
          for (size_t slot = 0; slot < 4; ++slot)
            local = local || (row->second.physical[slot].present && Key(faces[slot]) == face->second);
        }
      if (!local) return false;
    }
  }
  return true;
}

#include "native_engine3d/EngineAdapt.cpp"

EngineResult Adapt(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                   const PlanarReference& accepted_geometry, MetricComposition composition, EngineOptions options,
                   MetricChange intent, bool bidirectional = false, BoundaryMetric surface_target = {}) {
  EngineSetup setup;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  using Phase = EnginePhase;
  ContextValue phase_lifetime(setup.control_prefix.phase, Phase::Construction);
  auto& timer_session = setup.timer_session.emplace(
      options.timings, "native_engine", world.rank, world.size,
      surface_target ? SU2NativeTiming3D::Phase::Surface : SU2NativeTiming3D::Phase::Construction);
  if (timer_session.Owns())
    options.caller_bytes = transfer_memory::Add(options.caller_bytes, SU2NativeTiming3D::Session::ReservedBytes());
  setup.d9_profile.emplace(options.timings, world.rank);
  setup.wall_counters.emplace(options.timings && bool(surface_target));
  setup.lane_counters.emplace(options.timings);
  ContextValue region_rebuilt_lifetime(setup.control_prefix.region_rebuilt, false);
  ContextValue improvement_changed_lifetime(setup.control_prefix.improvement_changed, false);
  ContextValue construction_changed_lifetime(setup.control_prefix.construction_changed, false);
  ContextValue construction_handoff_progress_lifetime(setup.control_prefix.construction_handoff_progress, false);
  ContextValue construction_generation_changed_lifetime(setup.control_prefix.construction_generation_changed, false);
  ContextValue metric_cleanup_lifetime(setup.control_prefix.metric_cleanup, false);
  ContextValue metric_cleanup_tried_lifetime(setup.control_prefix.metric_cleanup_tried, false);
  ContextValue phase_start_round_lifetime(setup.control_prefix.phase_start_round, 0);
  if (options.stage_observer)
    options.caller_bytes = transfer_memory::Add(options.caller_bytes, options.stage_observer_bytes);
  ContextValue started_lifetime(setup.control_prefix.started, Clock::now());
  const auto& started = started_lifetime.Get();
  ContextValue started_cpu_lifetime(setup.control_prefix.started_cpu, options.timings ? std::clock() : std::clock_t{});
  setup.private_parameters.emplace();
  auto& private_geometry = setup.private_geometry.emplace();
  auto& local_atlas_budget = setup.local_atlas_budget.emplace();
  auto& atlas_budget = options.shared_geometry_budget ? *options.shared_geometry_budget : local_atlas_budget;
  ContextValue atlas_work_initial_lifetime(setup.control_prefix.atlas_work_initial, atlas_budget.work);
  auto& restore_budget = setup.restore_budget.emplace();
  restore_budget.budget = options.shared_geometry_budget;
  restore_budget.caller = atlas_budget.caller_bytes;
  restore_budget.bytes = atlas_budget.maximum_bytes;
  restore_budget.bits = atlas_budget.maximum_integer_bits;
  restore_budget.maximum_work = atlas_budget.maximum_work;
  auto& atlas_geometry = setup.atlas_geometry.emplace();
  setup.target_geometry_lease.emplace();
  ContextValue atlas_scratch_lifetime(setup.control_prefix.atlas_scratch,
                                      std::max(AtlasScratchBytes, options.geometry_workspace_bytes));
  if (!options.shared_geometry_budget && std::isfinite(options.maximum_seconds) && options.maximum_seconds > 0)
    atlas_budget.within_time = [started, maximum = options.maximum_seconds] {
      return std::chrono::duration<double>(Clock::now() - started).count() < maximum;
    };
  const SU2NativeGeometry::BoundaryCoverage no_coverage;
  ContextValue parameter_failure_lifetime(setup.control_prefix.parameter_failure);
  ContextValue parameter_bound_lifetime(setup.control_prefix.parameter_bound, 0);
  if (auto rejected =
          SetupBoundaryParameters(setup, world, accepted, original, accepted_geometry, options, atlas_budget))
    return std::move(*rejected);
  const auto& geometry = atlas_geometry ? *atlas_geometry : private_geometry ? *private_geometry : accepted_geometry;
  EngineContext context(setup, world, accepted, original, geometry, composition, options, intent, bidirectional,
                        surface_target);
  return EngineRun{context, atlas_budget, no_coverage}.Run();
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
  if(!geometry.EntityGeometry()&&geometry.Mapped()&&(geometry.Mobility()||!geometry.Parameters()->Coverage().faces.empty()||!geometry.Parameters()->Coverage().traces.empty())) {
    bool changed_surface=false;
    for(unsigned k=0;k<4;++k)changed_surface=changed_surface||(cell.physical[k].present&&cell.cell.v[k].id!=removed);
    // A single-cell filter cannot remap a complete physical star; the full operator must certify its atlas.
    if(changed_surface||!geometry.AtlasReservation())return false;
  }
  // Entity faces use existing point associations; test them before exporting/reconstructing a complete cavity.
  const auto patch=GeometryPatch(Imported{{proposed},true,{}},geometry.EntityGeometry()?nullptr:&geometry);
  for(const auto& face:patch.surface) {
    std::array<Point,3> points;
    for(unsigned i=0;i<3;++i)for(auto n:proposed.cell.v)if(n.id==face.v[i])points[i]=n.p;
    if(!geometry.ContainsFace(face.facet,points))return true;
    bool direct=true;
    if(geometry.EntityGeometry())for(Id id:face.v) {
      const auto* point=geometry.Parameters()->Find(id);
      direct=direct && point && geometry.Parameters()->Geometry().Supports(face.facet,point->binding);
    }
    // Remapped faces need complete-cavity admission, not a global enclosure inside a selection shortcut.
    if(direct && geometry.Declared(geometry.Canonical(face.facet)) &&
        geometry.SurfaceDeviation(face.facet,points,SU2NativeGeometry::SurfaceQueryNeed::Classify())>tolerance)return true;
  }
  FirstHeightReport report;std::string reason;
  if(heights.empty() || CheckFirstHeight(patch,heights,geometry,report,reason))return false;
  return GetShapeAdmission()!=ShapeAdmission::ValidityOnly || !report.measured || report.missing_cells ||
         report.reference_clearance_failures;
}
EngineResult PrepareSurface(World& world,const Owned& accepted,const std::vector<DonorCell>& original,
                            const PlanarReference& geometry,BoundaryMetric target,EngineOptions options) {
  CLocalFailure local;
  if(!target||options.wall_heights.empty())local.Set(1,0,"Surface preparation needs requested walls and a boundary target.");
  const auto failure=world.elect(local);
  if(failure.any){EngineResult result;result.reason=failure.message;result.stats.stop=EngineStop::Failure;return result;}
  // Planning uses the transported core; full composed edge certification belongs to AdaptMetric after construction.
  options.edge_breakpoints={};
  return Adapt(world,accepted,original,geometry,{},options,MetricChange::Refine,true,std::move(target));
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
#include "native_engine3d/MeshOptimizer.cpp"
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
