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
#include "../../include/adaptation/CNativeLocalCore3D.hpp"
#include "../../include/adaptation/CNativeGeometryCertificates3D.hpp"
#include "native_engine3d/EngineQualityStall.hpp"
#include <cassert>
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
#include <new>
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

struct FinalProofReport {
  bool shared = false;
  size_t reference_facets = 0, own_roots = 0, share_work = 0;
  double reverse_seconds = 0, share_seconds = 0;
  const BoundaryParameterState::Certificates* certificates = nullptr;
};

const BoundaryParameterState::Certificates& ProveSharedModel(World& world, const BoundaryParameterState& state,
    SU2NativeGeometry::AtlasBudget& budget, const std::vector<double>& bounds, FinalProofReport* report) {
  std::vector<ModelShareBound> share;
  CLocalFailure failure;
  const auto start = report ? std::chrono::steady_clock::now() : std::chrono::steady_clock::time_point{};
  const size_t work = report ? budget.work : 0;
  try {
    share = state.ModelProofShare(budget, bounds, {static_cast<unsigned>(world.rank), static_cast<unsigned>(world.size)});
  } catch (const std::exception& error) {
    failure.Set(2, 0, error.what());
  }
  if (report) {
    report->share_seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    report->share_work = budget.work - work;
    for (const auto& entity : share) report->own_roots += entity.roots;
  }
  const auto elected = world.elect(failure);
  if (elected.any) throw std::runtime_error(elected.message);
  for (auto& entity : share) {
    entity.model_to_mesh = world.reduce(entity.model_to_mesh, CPassiveComm::Op::MAX);
    entity.outside = world.reduce(int(entity.outside), CPassiveComm::Op::MAX) != 0;
  }
  return state.AdoptModelProof(bounds, share);
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
        SU2NativeTiming3D::Operation operation = SU2NativeTiming3D::Operation::Unattributed,
        SU2NativeTiming3D::SourceSite site = {__builtin_FILE(), __builtin_LINE()})
      : value(value), enabled(enabled), parent(parent) {
    if (enabled) {
      start = Clock::now();
      scope.emplace(operation, site);
    }
  }
  /*! Label an existing legacy service that has no parent-subtraction counter. */
  Timer(double& value, bool enabled, SU2NativeTiming3D::Operation operation,
        SU2NativeTiming3D::SourceSite site = {__builtin_FILE(), __builtin_LINE()})
      : Timer(value, enabled, nullptr, operation, site) {}
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
std::vector<double> ProveOwnedFaceReverse(World& world, const Owned& working, const BoundaryParameterState& state,
    SU2NativeGeometry::AtlasBudget& budget, EngineStats& stats, std::vector<SurfaceFace>& faces,
    std::optional<ProofStorage>& storage) {
  std::vector<double> bounds;
  CLocalFailure failure;
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
  return bounds;
}

FinalProofReport ProveOwnedSurface(World& world, const Owned& working, const PlanarReference& geometry,
    SU2NativeGeometry::AtlasBudget& budget, EngineStats& stats, bool timings) {
  using Clock = std::chrono::steady_clock;
  const auto start = timings ? Clock::now() : Clock::time_point{};
  const auto& state = *geometry.Parameters();
  std::vector<SurfaceFace> faces;
  std::optional<ProofStorage> storage;
  const auto bounds = ProveOwnedFaceReverse(world, working, state, budget, stats, faces, storage);
  const bool shared = world.size > 1 && state.ReverseProofComplete(bounds);
  FinalProofReport report;
  if (timings) {
    report.shared = shared;
    report.reference_facets = state.Geometry().Reference().Facets().size();
    report.reverse_seconds = std::chrono::duration<double>(Clock::now() - start).count();
  }
  const auto forward = timings && !shared ? Clock::now() : Clock::time_point{};
  const size_t work = timings && !shared ? budget.work : 0;
  const auto& certificates = shared ? ProveSharedModel(world, state, budget, bounds, timings ? &report : nullptr)
                                    : state.GeometryCertificates(budget, true, &bounds);
  for (const auto& row : certificates)
    if (!row.second.within_tolerance)
      throw std::runtime_error("Complete working surface does not meet the prescribed two-sided geometry tolerance.");
  if (timings) {
    report.certificates = &certificates;
    if (!shared) {
      report.own_roots = report.reference_facets;
      report.share_seconds = std::chrono::duration<double>(Clock::now() - forward).count();
      report.share_work = budget.work - work;
    }
  }
  return report;
}

void PrintFinalProof(const FinalProofReport& report, int rank, double tolerance) {
  const auto& certificates = *report.certificates;
  std::fprintf(stderr, "NATIVE3D_FINAL_PROOF rank=%d shared=%d entities=%zu reference_facets=%zu own_roots=%zu"
                       " reverse_seconds=%.17g share_seconds=%.17g share_work=%zu\n",
               rank, int(report.shared), certificates.size(), report.reference_facets, report.own_roots,
               report.reverse_seconds, report.share_seconds, report.share_work);
  if (rank != 0) return;
  for (const auto& row : certificates) {
    const auto& bound = row.second;
    std::fprintf(stderr, "NATIVE3D_FINAL_CERT entity=%llu/%llu/%d faces=%zu mesh_to_model=%.17g model_to_mesh=%.17g"
                         " tolerance=%.17g within=%d\n",
                 static_cast<unsigned long long>(std::get<0>(row.first)),
                 static_cast<unsigned long long>(std::get<1>(row.first)), std::get<2>(row.first), bound.physical_pieces,
                 bound.mesh_to_model, bound.model_to_mesh, tolerance, int(bound.within_tolerance));
  }
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
#include "native_engine3d/EngineSelection.hpp"
#include "native_engine3d/EngineSelectionCensus.cpp"
#include "native_engine3d/EngineSelectionQueues.cpp"
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
bool OptimizedRepairPatch(const Patch& old, Id first, Id first_node, const RegionMetric& target, Patch& fresh,
                          std::string& reason, size_t& work, bool preserve_vertices, bool fine_placement,
                          const PlanarReference& geometry, const WallHeights& heights, double surface_tolerance,
                          OptimizationOperations& operations, double maximum_seconds) {
  operations={};
  Owned local;std::map<Face,Id> physical;for(const auto& f:old.surface)physical.emplace(Key(f.v),f.facet);
  for(auto c:old.cells){c.id=local.size();OwnedCell row;row.cell=c;const auto faces=Faces(c);
    for(unsigned k=0;k<4;++k){const auto found=physical.find(Key(faces[k]));if(found!=physical.end())row.physical[k]={true,found->second};}
    local.emplace(c.id,row);}
  auto parameters=geometry.PrivateParameters(old);
  OptimizeOptions options;options.maximum_work=work;options.maximum_seconds=maximum_seconds;options.cache_entries=8*MaximumReplacementCells;
  options.boundary=false;options.smoothing=!preserve_vertices;options.collapses=!preserve_vertices;
  options.fine_placement = fine_placement;
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
      const auto star = selection.census.vertex_cells.find(vertex);
      if (star != selection.census.vertex_cells.end())
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
  if (options.close_construction_collapse &&
      quality_stall::Enabled(options.close_construction_collapse, SU2NativeLocal3D::FP3Probe())) {
    setup.control_prefix.quality_stall.emplace();
    options.caller_bytes = transfer_memory::Add(options.caller_bytes, sizeof(quality_stall::State));
  }
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
