/*!
 * \file CNativeBLPipeline3D.hpp
 * \brief Native 3D boundary-layer construction pipeline (ADAP_BL_METHOD= FIRST_ROW): surface-first wall resolve W,
 *        first row R1/R2, growth layers G, volume V13 with the metric optimizer, the halving step and the column
 *        redistribution of the first-height ramp. Serial; one pipeline run at a time (PipelineScope).
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <sys/resource.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <deque>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CNativeRemesher3D.hpp"
#include "../../../Common/include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../../Common/include/adaptation/CNativeSupport3D.hpp"
#include "../../../Common/include/adaptation/CNativeImport3D.hpp"
#include "../../../Common/include/adaptation/CNativeMetric3D.hpp"
#include "../../../Common/include/adaptation/CNativeCurvedAtlas3D.hpp"
#include "../../../Common/include/adaptation/CNativeChordEnvelope.hpp"
#include "../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "CBoundaryLayerMetric.hpp"
#include "CNativeBLTarget3D.hpp"
#include "CNativeBLComposition3D.hpp"
#include "CNativeMetricFamily3D.hpp"
#include "../solvers/CSolver.hpp"

#include "native_bl3d/BLState.inl"
#include "native_bl3d/BLLayer.inl"
#include "native_bl3d/BLFirstRow7.inl"
#include "native_bl3d/BLFirstRow9.inl"
#include "native_bl3d/BLOptionA10.inl"
#include "native_bl3d/BLSplit11.inl"
#include "native_bl3d/BLGrowth12.inl"
#include "native_bl3d/BLVolume13.inl"
#include "native_bl3d/BLEvent15.inl"
#include "native_bl3d/BLRebuild17.inl"

/*--- Production entry (DC-14): one remesh event of ADAP_BL_METHOD= FIRST_ROW. ---*/
namespace SU2NativeBLPipeline3D {
using namespace SU2Native3D;

/*! First-row registry of one accepted event, keyed by exact coordinates (continuation of the next event). */
struct RowRegistry {
  using P = std::array<double, 3>;
  std::vector<std::pair<P, P>> lift;                                  // wall vertex -> lid
  std::vector<std::pair<std::array<P, 3>, P>> apex;                   // wall face -> apex
  std::vector<std::pair<std::array<P, 3>, std::array<P, 3>>> overrides;
  std::vector<std::pair<std::array<P, 3>, std::vector<std::pair<std::array<P, 3>, bool>>>> extras;
  uint64_t digest = 0;   // FNV-1a of the sorted vertex coordinates of the mesh it was written for
  size_t unsupported = 0;  // crease-fan copies/fan cells (not produced by the single lift) are not continued
  size_t ResidentBytes() const {
    using transfer_memory::Add;using transfer_memory::Bytes;
    size_t bytes=Add(sizeof(*this),Bytes(lift),Bytes(apex),Bytes(overrides),Bytes(extras));
    for(const auto& row:extras)bytes=Add(bytes,Bytes(row.second));
    return bytes;
  }
  std::string Encode() const {
    std::ostringstream o;
    o << std::setprecision(17) << "SU2_NATIVE_ROWS 1\ndigest " << digest << "\nunsupported " << unsupported << "\n";
    auto put = [&](const P& x) { o << ' ' << x[0] << ' ' << x[1] << ' ' << x[2]; };
    o << "lift " << lift.size() << "\n";
    for (const auto& r : lift) { put(r.first); put(r.second); o << "\n"; }
    o << "apex " << apex.size() << "\n";
    for (const auto& r : apex) { for (const auto& x : r.first) put(x); put(r.second); o << "\n"; }
    o << "overrides " << overrides.size() << "\n";
    for (const auto& r : overrides) { for (const auto& x : r.first) put(x); for (const auto& x : r.second) put(x); o << "\n"; }
    o << "extras " << extras.size() << "\n";
    for (const auto& r : extras) {
      for (const auto& x : r.first) put(x);
      o << ' ' << r.second.size();
      for (const auto& e : r.second) { for (const auto& x : e.first) put(x); o << ' ' << int(e.second); }
      o << "\n";
    }
    o << "end\n";
    return o.str();
  }
  static bool Decode(const std::string& text, RowRegistry& r, std::string& why) {
    std::istringstream in(text);
    std::string key;
    int version = 0;
    in >> key >> version;
    if (key != "SU2_NATIVE_ROWS" || version != 1) { why = "not a SU2_NATIVE_ROWS 1 file"; return false; }
    auto get = [&](P& x) { in >> x[0] >> x[1] >> x[2]; };
    size_t n = 0;
    in >> key >> r.digest >> key >> r.unsupported;
    in >> key >> n; if (key != "lift") { why = "lift block"; return false; }
    r.lift.resize(n); for (auto& e : r.lift) { get(e.first); get(e.second); }
    in >> key >> n; if (key != "apex") { why = "apex block"; return false; }
    r.apex.resize(n); for (auto& e : r.apex) { for (auto& x : e.first) get(x); get(e.second); }
    in >> key >> n; if (key != "overrides") { why = "overrides block"; return false; }
    r.overrides.resize(n); for (auto& e : r.overrides) { for (auto& x : e.first) get(x); for (auto& x : e.second) get(x); }
    in >> key >> n; if (key != "extras") { why = "extras block"; return false; }
    r.extras.resize(n);
    for (auto& e : r.extras) {
      for (auto& x : e.first) get(x);
      size_t m = 0; in >> m; e.second.resize(m);
      for (auto& f : e.second) { for (auto& x : f.first) get(x); int b = 0; in >> b; f.second = b != 0; }
    }
    in >> key;
    if (!in || key != "end") { why = "truncated rows file"; return false; }
    return true;
  }
};
/*! Committed registry (after an accepted replacement) and the pending one of the running event. */
struct Continuation {
  std::shared_ptr<RowRegistry> committed, pending;
  std::string source;  // where `committed` came from (sidecar path or "memory")
  size_t ResidentBytes() const {
    using transfer_memory::Add;
    return Add(sizeof(*this),transfer_memory::Bytes(source),committed?Add(committed->ResidentBytes(),4*sizeof(void*)):0,
        pending&&pending!=committed?Add(pending->ResidentBytes(),4*sizeof(void*)):0);
  }
};
/*! Numbering-independent digest of a mesh: every cell as its sorted quadruple of exact vertex coordinates plus the
 *  physical flags of its faces, sorted (binds coordinates and connectivity, not identities). */
inline uint64_t CoordinateDigest(const Owned& cells) {
  using P = std::array<double, 3>;
  std::vector<std::pair<std::array<P, 4>, unsigned>> keys;
  keys.reserve(cells.size());
  for (const auto& row : cells) {
    std::array<P, 4> c; unsigned flags = 0;
    for (unsigned k = 0; k < 4; ++k) c[k] = {row.second.cell.v[k].p.x, row.second.cell.v[k].p.y, row.second.cell.v[k].p.z};
    std::array<unsigned, 4> order{0, 1, 2, 3};
    std::sort(order.begin(), order.end(), [&](unsigned a, unsigned b) { return c[a] < c[b]; });
    std::array<P, 4> sorted; for (unsigned k = 0; k < 4; ++k) sorted[k] = c[order[k]];
    // face k is opposite vertex k: record which sorted vertex each physical face is opposite to
    for (unsigned k = 0; k < 4; ++k) if (row.second.physical[order[k]].present) flags |= 1u << k;
    keys.push_back({sorted, flags});
  }
  std::sort(keys.begin(), keys.end());
  uint64_t h = 1469598103934665603ULL;
  auto mix = [&](const void* data, size_t n) { const auto* b = static_cast<const unsigned char*>(data); for (size_t k = 0; k < n; ++k) { h ^= b[k]; h *= 1099511628211ULL; } };
  for (const auto& k : keys) { mix(k.first.data(), sizeof(k.first)); mix(&k.second, sizeof(k.second)); }
  return h;
}
/*! Sidecar next to an adapted mesh (<mesh>.native_rows); a missing file is no continuation. */
inline bool LoadContinuation(const std::string& path, Continuation& c, std::string& why) {
  std::ifstream in(path);
  if (!in) { why = "missing"; return false; }
  std::stringstream text; text << in.rdbuf();
  auto r = std::make_shared<RowRegistry>();
  if (!RowRegistry::Decode(text.str(), *r, why)) return false;
  c.committed = r; c.source = path;
  return true;
}
inline void WriteContinuation(const std::string& path, const Continuation& c) {
  if (!c.committed) return;
  std::ofstream out(path); out << c.committed->Encode();
  if (!out) throw std::runtime_error("Cannot write the FIRST_ROW continuation " + path);
}

struct EventControls {
  double seconds = 0;          // whole construction budget (0: 7200 s)
  bool final_cycle = true;     // true first heights: the final gates decide acceptance
  unsigned long event = 0;
  double ramp_factor = 1;
  size_t memory_ceiling = size_t(2) << 30, caller_bytes=0;
  SU2NativeGeometry::AtlasBudget* geometry_budget=nullptr;
  size_t maximum_work = SIZE_MAX;  // Shared event allowance; construction is charged before distributed improvement.
  const EngineOptions* engine_controls=nullptr;  // Borrowed synchronously from the remesher's frozen event.
  std::string report_prefix;   // <prefix>_bl_report.json (and <prefix>_final.mesh when export_mesh)
  bool export_mesh = false;
  double intermediate_v_seconds = 1800;  // DC-15: V cap of an intermediate (ramped-h0) event (DC-16: 1800, the full-M6 x32 event 0 takes 34 of its 60 min with 900); the final cycle uses the budget
  bool full_audit = false;              // DC-15: run the uncached audit also on intermediate events (diagnostic)
};

namespace detail {
// Requested-storage estimate of the construction: its state (cells, stars, wall index, box grid), caches, the frozen
// field's donor copy and index, plus the caller's retained input and source. Not RSS.
inline size_t StateBytes(const junction_wp0::State& s, size_t caller = 0, size_t donors = 0) {
  return s.mesh.size() * 640 + s.wall_index.size() * 80 + s.box_grid.size() * 96 + s.lengths.size() * 112 +
         dc9::shapeCache.size() * 48 + dc9::heightCache.size() * 48 + dc9::geometryLengths.size() * 104 +
         dc9::balancedCache.size() * 120 + s.overlay.size() * 160 + donors * (sizeof(DonorCell) + 64) + caller +
         (s.live_parameters?s.live_parameters->ResidentBytes():0)+(s.reference?s.reference->ParameterOverlayBytes():0);
}
/*! Restores every pipeline-global switch and cache on exit (also on exceptions). */
struct PipelineScope {
  dc12::Policies policies;
  PipelineScope() {
    dc9::ensureDirectionField={};
    dc9::perf = dc9::Perf{}; dc9::ResetCaches(); dc9::cachesOn = true; dc9::balancedCache.clear();
    dc9::firstChildDiagnostics = 0; dc9::geometryHits = 0;
    dc10::reflectiveByRole = false; dc10::reflectiveMarkers.clear(); dc9::artificialMarkers.clear();
    dc13::v13R2Retry = true; dc13::unliftedColumns = false; dc13::diagnosticCutSlide = false; dc13::v13AttemptsPerSweep = 1024;
    dc13::v13NearWallFirst = false; dc12::growthClearing = true; dc13::v13EstimateScan = false; dc13::v13SkipFinalOnLimit = false;
    dc9::rimByRole = true;  // DC-15: artificial boundaries only by role (production has none), never by a marker name
    dc9::validityOnlyConstruction = false; dc13::v13DemandQueue = false; dc16::roomStep = true; dc9::markerRoles.clear(); dc13::eventTensors = dc13::VertexTensors{};
  }
  ~PipelineScope() {
    dc9::ensureDirectionField={};
    dc9::ResetCaches(); dc9::balancedCache.clear(); dc9::cachesOn = true; dc12::layerOfMarker.clear();
    dc10::optionA.reset(); dc10::probeBase.reset(); dc10::probeFull.reset();
    dc13::v13R2Retry = true; dc13::v13AttemptsPerSweep = 1024; dc9::directionField = nullptr; dc9::directionFieldUses = 0;
    dc13::v13NearWallFirst = false; dc12::growthClearing = true; dc13::v13EstimateScan = false; dc13::v13SkipFinalOnLimit = false;
    dc9::rimByRole = false;
    dc9::validityOnlyConstruction = false; dc13::v13DemandQueue = false; dc16::roomStep = true; dc9::markerRoles.clear(); dc13::eventTensors = dc13::VertexTensors{};
  }
};
}  // namespace detail

/*! Geometry/height-guided first-row construction on one worker. The private result always continues
 * through the general distributed construction/improvement lifecycle; this function never certifies final q/J.
 * A digest-matched continuation supplies retained row topology only. */
inline EngineResult RunEvent(World& world, const Owned& cells, const std::vector<DonorCell>& donors,
                             const ReferenceState& state, const TargetPolicy& policy, const CConfig& config,
                             Continuation& cont, const EventControls& ctl, std::string& report) {
  using namespace dc13;
  using Clock = std::chrono::steady_clock;
  EngineResult out;
  out.valid = false;out.stats.initial_cells=cells.size();
  struct RestoreShape {ShapeAdmission previous=GetShapeAdmission();~RestoreShape(){SetShapeAdmission(previous);}} restore_shape;
  SetShapeAdmission(ShapeAdmission::ValidityOnly);
  SU2NativeGeometry::AtlasBudget local_geometry_budget;
  auto& geometry_budget=ctl.geometry_budget?*ctl.geometry_budget:local_geometry_budget;
  struct RestoreGeometryWindow {
    SU2NativeGeometry::AtlasBudget& budget;size_t caller,bits;
    ~RestoreGeometryWindow(){budget.caller_bytes=caller;budget.maximum_integer_bits=bits;}
  } restore_geometry_window{geometry_budget,geometry_budget.caller_bytes,geometry_budget.maximum_integer_bits};
  const auto geometry_work_initial=geometry_budget.work;
  std::shared_ptr<void> geometry_lease;
  std::ostringstream json;
  json << std::setprecision(12) << "{\"event\":" << ctl.event << ",\"ramp_factor\":" << ctl.ramp_factor
       << ",\"final_cycle\":" << ctl.final_cycle << ",\"input_cells\":" << cells.size();
  auto finish = [&](const std::string& reason) {
    out.stats.atlas_work=geometry_budget.work-geometry_work_initial;out.stats.atlas_peak_bytes=geometry_budget.peak_bytes;
    out.stats.maximum_work_bytes=std::max(out.stats.maximum_work_bytes,geometry_budget.peak_bytes);
    out.reason = reason;
    json << ",\"construction_valid\":" << out.valid << ",\"final_acceptance\":\"deferred to distributed stages\",\"reason\":" << junction_wp0::Json(reason) << "}\n";
    report = json.str();
    if (!ctl.report_prefix.empty()) { std::ofstream(ctl.report_prefix + "_bl_report.json") << report; }
    return out;
  };
  if (world.size != 1) return finish("ADAP_BL_METHOD= FIRST_ROW runs on one native worker (set ADAP_NATIVE_RANKS= 1).");
  try {
    const auto t0 = Clock::now();
    const double budget = ctl.seconds > 0 ? ctl.seconds : 7200.0;
    const double reserve = std::min(.5*budget,std::max(120.0,.1*budget)), usable = budget-reserve;
    auto elapsed = [&] { return std::chrono::duration<double>(Clock::now() - t0).count(); };
    constexpr size_t atlasScratch=16u<<20;
    const auto queryScratch=transfer_memory::Add(std::max(policy.full->GeometryWorkspaceBytes(),policy.base->GeometryWorkspaceBytes()),atlasScratch);
    size_t stateCeiling=ctl.memory_ceiling;
    if(queryScratch) {
      if(queryScratch>=stateCeiling)return finish("FIRST_ROW geometry scratch exceeds shared memory ceiling.");
      stateCeiling-=queryScratch;
      geometry_budget.caller_bytes=stateCeiling;
      if(!ctl.geometry_budget){geometry_budget.maximum_bytes=ctl.memory_ceiling;geometry_budget.caller_bytes=stateCeiling;}
      const auto work=ctl.maximum_work==SIZE_MAX?std::max(size_t(4096),transfer_memory::Mul(cells.size(),64)):ctl.maximum_work;
      const auto allowance=transfer_memory::Mul(transfer_memory::Add(cells.size(),work),SU2NativeGeometry::MappedFacetReference::ClearanceWork);
      if(allowance==SIZE_MAX)return finish("FIRST_ROW geometry work reservation overflows.");
      if(!ctl.geometry_budget){geometry_budget.maximum_work=allowance;
        geometry_budget.within_time=[t0,budget] {return std::chrono::duration<double>(Clock::now()-t0).count()<budget;};}
      geometry_lease=policy.ReserveGeometry(geometry_budget);
      size_t low=0,high=atlasScratch*8;
      while(low<high){const auto bits=low+(high-low+1)/2;
        if(transfer_memory::Mul(2,SU2NativeGeometry::CurvedAtlasWorkspaceBytes(bits))<=atlasScratch/2)low=bits;else high=bits-1;}
      if(!low)return finish("FIRST_ROW has no admitted atlas integer workspace.");
      geometry_budget.maximum_integer_bits=geometry_budget.maximum_integer_bits?std::min(geometry_budget.maximum_integer_bits,low):low;
    }
    detail::PipelineScope scope;
    dc10::optionA = policy.A; dc10::probeBase = policy.base; dc10::probeFull = policy.full;
    size_t metadataBytes=transfer_memory::Add(sizeof(State),transfer_memory::Bytes(state.facets),transfer_memory::Bytes(state.marker_names));
    for(const auto& marker:state.marker_names)metadataBytes=transfer_memory::Add(metadataBytes,marker.capacity()+1);
    const size_t callerBytes=transfer_memory::Add(ctl.caller_bytes,transfer_memory::Mul(cells.size(),640),
        transfer_memory::Mul(donors.size(),sizeof(DonorCell)+64),metadataBytes,
        ctl.caller_bytes?0:transfer_memory::Add(policy.StorageBytes(),ReferenceStorageBytes(state)));
    if(transfer_memory::Add(callerBytes,transfer_memory::Mul(cells.size(),640),
        transfer_memory::Mul(donors.size(),sizeof(DonorCell)+64))>stateCeiling)
      return finish("FIRST_ROW input copies leave no admitted geometry workspace.");
    auto s = std::make_unique<State>();
    s->name = "native_first_row";
    s->markers = state.marker_names;
    s->facets = state.facets;
    double declared = 0;
    if (!state.original) return finish("FIRST_ROW needs the original geometry authority.");
    const bool plainFaceted = !state.original->Mapped() && state.original->Analytic().empty() && state.original->Affine().empty();
    size_t fittedPlanes = 0;
    if (plainFaceted) {
      // DC-17: symmetry-plane markers (configuration kinds) get fitted affine plane authorities (FitPlanarMarkers)
      std::set<int> planar;
      for (const auto& r : policy.roles) if (r.second == 0) planar.insert(r.first);
      s->reference = PromoteFacetedReference(FitPlanarMarkers(state.original, state.facets, planar, 1e-13, fittedPlanes), state.facets, 1e-13, declared);
    } else s->reference = state.original;
    s->mapped = s->reference->Mapped();
    for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) {
      const auto m = std::find(state.marker_names.begin(), state.marker_names.end(), config.GetAdap_BL(b).marker);
      if (m == state.marker_names.end()) return finish("FIRST_ROW: BL marker " + config.GetAdap_BL(b).marker + " has no geometry.");
      const int id = int(m - state.marker_names.begin());
      s->walls.insert(id);
      s->heights[id] = SU2_TYPE::GetValue(config.GetAdap_BL(b).firstHeight);
    }
    s->heights.tolerance = layer2d::HeightTolerance;
    s->h0 = policy.h0;
    // DC-15: growth and thickness are per marker (every consumer reads the wall's own production layer); the scalar
    // fields only report the BL entry with the smallest first height
    {unsigned short low = 0;
     for (unsigned short b = 1; b < config.GetnAdap_BL(); ++b) if (config.GetAdap_BL(b).firstHeight < config.GetAdap_BL(low).firstHeight) low = b;
     s->growth = SU2_TYPE::GetValue(config.GetAdap_BL(low).growth);
     s->thickness = SU2_TYPE::GetValue(config.GetAdap_BL(low).thickness);}
    s->core = policy.core;
    s->layers = policy.full;
    s->mesh = cells;
    if(detail::StateBytes(*s,callerBytes,donors.size())>stateCeiling)
      return finish("FIRST_ROW input/state leaves no admitted geometry workspace.");
    std::vector<DonorCell> surface_source;
    size_t directionBytes=0;
    const std::vector<DonorCell>* fieldDonors=&donors;
    auto makeField = [&](const junction_wp0::State& x) {
      return std::make_unique<FrozenField>(*fieldDonors, policy.Composition(), true, AssociatedExtensions(*fieldDonors, *x.reference),
                                           policy.Edges(), std::make_shared<const FrozenMetricIdentity>(), true,
                                           std::max(MaximumDonors, fieldDonors->size()));  // whole stage domain (admitted by StateBytes)
    };
    s->field = makeField(*s);
    Prepare(*s);
    ApplyWallPolicies(*s);
    s->heights.tolerance=SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol());
    DetectCreases(*s, policy.roles);  // DC-15: crease roles from the configuration marker kinds
    json << ",\"authority\":{\"mapped_input\":" << state.original->Mapped() << ",\"declared_error\":" << declared << ",\"fitted_planes\":" << fittedPlanes << ",\"affine\":" << s->reference->Affine().size()
         << "},\"wall_faces\":" << WallFaces(*s).size() << ",\"h0_min\":" << s->h0;
    auto memoryOk = [&](const char* where, std::string& why, size_t extra = 0) {
      const auto bytes = detail::StateBytes(*s, transfer_memory::Add(callerBytes,extra,transfer_memory::Bytes(surface_source),directionBytes), fieldDonors->size());
      out.stats.maximum_work_bytes=std::max(out.stats.maximum_work_bytes,bytes);
      out.stats.maximum_owned=std::max(out.stats.maximum_owned,s->mesh.size());
      if (bytes <= stateCeiling) return true;
      why = std::string("FIRST_ROW state estimate exceeds the native memory ceiling at ") + where + " (" +
            std::to_string(bytes) + " > " + std::to_string(stateCeiling) + " bytes after geometry reservation)";
      return false;
    };
    size_t wallEdges=0,wallVertices=0;
    {std::set<Edge> edges;std::set<Id> vertices;
      for(const auto& wall:WallFaces(*s))for(unsigned k=0;k<3;++k) {
        Edge edge{wall.second[k],wall.second[(k+1)%3]};std::sort(edge.begin(),edge.end());edges.insert(edge);vertices.insert(wall.second[k]);
      }
      wallEdges=edges.size();wallVertices=vertices.size();}
    const size_t wWork=std::min(ctl.maximum_work,std::max<size_t>(8192,4*wallEdges));
    const size_t r1Work=std::min(ctl.maximum_work,std::max<size_t>(4096,2*wallVertices));
    EngineOptions preparation=ctl.engine_controls?*ctl.engine_controls:EngineOptions{};
    preparation.maximum_seconds=std::min(.08*usable,budget-elapsed());preparation.maximum_work=wWork;
    preparation.memory_ceiling=ctl.memory_ceiling;
    preparation.caller_bytes=detail::StateBytes(*s,callerBytes,donors.size());
    preparation.surface_tolerance=SU2_TYPE::GetValue(config.GetAdap_Hausd());preparation.wall_heights=s->heights;
    preparation.shared_geometry_budget=&geometry_budget;preparation.geometry_workspace_bytes=queryScratch;
    if(queryScratch)preparation.geometry_reservation=[&policy](SU2NativeGeometry::AtlasBudget& budget){return policy.ReserveGeometry(budget);};
    preparation.timings=true;
    if(!(preparation.maximum_seconds>0))return finish("FIRST_ROW deadline exhausted before surface preparation.");
    const auto preparation_started=elapsed();
    auto prepared=PrepareSurface(world,s->mesh,donors,*s->reference,
        [&](Node node,const PlanarReference& geometry,Tensor core){return policy.BoundaryGuidance(node,geometry,core,state.marker_names);},preparation);
    const auto preparation_work=prepared.stats.global_work;
    out.stats=prepared.stats;
    json << ",\"surface_preparation\":{\"prepared\":" << prepared.surface_prepared << ",\"final_acceptance\":false"
         << ",\"seconds\":" << elapsed()-preparation_started << ",\"work\":" << preparation_work
         << ",\"commits\":" << prepared.stats.accepted << ",\"geometry_work\":" << prepared.stats.atlas_work
         << ",\"field_compositions\":" << prepared.stats.field_compositions << ",\"reason\":" << junction_wp0::Json(prepared.reason) << '}';
    if(!prepared.surface_prepared) {
      out.cells=std::move(prepared.cells);out.reference=std::move(prepared.reference);
      return finish("FIRST_ROW surface preparation failed: "+prepared.reason);
    }
    s->field.reset();s->mesh=std::move(prepared.cells);if(prepared.reference)s->reference=std::move(prepared.reference);
    s->live_parameters=std::move(prepared.parameters);s->memory_ceiling=ctl.memory_ceiling;
    if(s->live_parameters)s->reference=std::shared_ptr<PlanarReference>(s->reference->WithAtlasReservation(geometry_budget));
    s->retained_bytes=[&](){return detail::StateBytes(*s,transfer_memory::Add(callerBytes,transfer_memory::Bytes(surface_source),directionBytes),fieldDonors->size());};
    s->admit_scratch=[&](size_t extra){std::string why;if(!memoryOk("private transaction",why,extra))throw std::length_error(why);};
    if(detail::StateBytes(*s,callerBytes,s->mesh.size())+transfer_memory::Mul(s->mesh.size(),sizeof(DonorCell))>stateCeiling)
      return finish("FIRST_ROW prepared core snapshot exceeds native storage admission.");
    surface_source.reserve(s->mesh.size());
    for(const auto& row:s->mesh) {
      if(!row.second.sensor_present)throw std::runtime_error("Surface preparation omitted transported core values.");
      DonorCell donor;donor.cell=row.second.cell;donor.sensor=row.second.sensor;surface_source.push_back(donor);
    }
    fieldDonors=&surface_source;s->field=makeField(*s);Prepare(*s);
    // The registry must match the input checkpoint; surviving entries are remapped after owned preparation edits.
    Rows10 rows;
    size_t mapped = 0, dropped = 0;
    const auto digest = CoordinateDigest(s->mesh);
    std::string continuation = "none";
    if (cont.committed) {
      if (cont.committed->digest != CoordinateDigest(cells)) continuation = "digest mismatch (" + cont.source + ")";
      else {
        std::map<std::array<double, 3>, Id> byPoint;
        for (const auto& row : s->mesh) for (const auto& n : row.second.cell.v) byPoint.emplace(std::array<double, 3>{n.p.x, n.p.y, n.p.z}, n.id);
        auto id = [&](const RowRegistry::P& x, Id& v) { const auto f = byPoint.find(x); if (f == byPoint.end()) return false; v = f->second; return true; };
        auto face = [&](const std::array<RowRegistry::P, 3>& f, Face& k) { for (int i = 0; i < 3; ++i) if (!id(f[i], k[i])) return false; k = Sorted(k); return true; };
        for (const auto& e : cont.committed->lift) { Id a, b; if (id(e.first, a) && id(e.second, b)) { rows.lift[a] = b; ++mapped; } else ++dropped; }
        for (const auto& e : cont.committed->apex) { Face k; Id a; if (face(e.first, k) && id(e.second, a)) { rows.apex[k] = a; ++mapped; } else ++dropped; }
        for (const auto& e : cont.committed->overrides) {
          Face k; std::array<Id, 3> v; bool ok = face(e.first, k);
          for (int i = 0; i < 3 && ok; ++i) ok = id(e.second[i], v[i]);
          if (ok) { rows.overrides[k] = v; ++mapped; } else { ++dropped; rows.apex.erase(k); }
        }
        for (const auto& e : cont.committed->extras) {
          Face k; bool ok = face(e.first, k); std::vector<std::pair<Face, bool>> list;
          for (const auto& f : e.second) { Face x; for (int i = 0; i < 3 && ok; ++i) ok = id(f.first[i], x[i]); list.push_back({x, f.second}); }
          if (ok) { rows.extras[k] = list; ++mapped; } else { ++dropped; rows.apex.erase(k); rows.overrides.erase(k); }
        }
        for (auto it = rows.overrides.begin(); it != rows.overrides.end();)
          if (!rows.extras.count(it->first)) { rows.apex.erase(it->first); it = rows.overrides.erase(it); ++dropped; } else ++it;
        continuation = cont.source;
      }
    }
    const bool chain = !rows.apex.empty();
    json << ",\"continuation\":{\"source\":" << junction_wp0::Json(continuation) << ",\"mapped\":" << mapped << ",\"dropped\":" << dropped
         << ",\"chain_step\":" << chain << "}";
    // budgets relative to the problem (the DC-13 cut limits are the lower bounds)
    // Every ramp event returns provisional geometry; the distributed engine owns all shape improvement.
    dc9::validityOnlyConstruction = true; v13DemandQueue = false;
    json << ",\"dc16\":{\"validity_only_construction\":" << dc9::validityOnlyConstruction << ",\"v_demand_queue\":0,\"room_step\":" << dc16::roomStep << "}";
    json << ",\"limits\":{\"budget_seconds\":" << budget << ",\"reserve_seconds\":" << reserve << ",\"W\":[" << wWork << "," << .08 * usable
         << "],\"R1\":[" << r1Work << "," << .05 * usable << "],\"R2_seconds\":" << .25 * usable << ",\"G_seconds\":" << .15 * usable
         << ",\"maximum_work\":" << ctl.maximum_work
         << ",\"memory_ceiling\":" << ctl.memory_ceiling << "}";
    std::ostringstream log;
    std::string why;
    // DC-15: the construction log is streamed to <prefix>_bl.log and every phase prints one stdout line and appends its
    // JSON to <prefix>_bl_report.partial.json (a killed run keeps its phase receipts)
    std::ofstream logFile;
    if (!ctl.report_prefix.empty()) logFile.open(ctl.report_prefix + "_bl.log");
    uint64_t workDone = preparation_work;
    double phaseStart = elapsed();
    auto phase = [&](const char* name, const std::string& extra = "") {
      if(s->publication_failed)throw std::runtime_error("FIRST_ROW private publication failed during this phase.");
      const double now = elapsed();
      std::cout << "DC15 phase " << name << " seconds=" << std::setprecision(6) << now - phaseStart << " elapsed=" << now << " cells=" << s->mesh.size()
                << " rss_kB=" << dc9::PeakKB() << (extra.empty() ? "" : " ") << extra << std::endl;
      json << ",\"phase_" << name << "\":{\"seconds\":" << now - phaseStart << ",\"elapsed\":" << now << ",\"cells\":" << s->mesh.size() << "}";
      if (logFile) { logFile << log.str(); logFile.flush(); }
      log.str(""); log.clear();
      if (!ctl.report_prefix.empty()) std::ofstream(ctl.report_prefix + "_bl_report.partial.json") << json.str() << "}\n";
      phaseStart = now;
      out.stats.global_work=workDone;
      if(now>=budget)throw std::runtime_error("FIRST_ROW construction exhausted the shared elapsed deadline.");
      if(workDone>ctl.maximum_work)throw std::runtime_error("FIRST_ROW construction exhausted shared native work.");
    };
    phase("prepare", "wall_faces=" + std::to_string(WallFaces(*s).size()));
    Growth G;
    std::set<Face> certified;
    std::map<Face, std::array<Id, 4>> snapshot;  // A3: first children after R2 + completion
    dc12::DirField directions;
    struct ReleaseDirections {~ReleaseDirections(){dc9::ensureDirectionField={};dc9::directionField=nullptr;}} release_directions;
    dc9::ensureDirectionField=[&](State& current) {
      if(&current!=s.get())throw std::logic_error("BL direction refresh changed its private state.");
      if(current.publication_failed)throw std::runtime_error("BL direction refresh follows a failed private publication.");
      if(current.direction_revision==current.boundary_revision)return;
      if(elapsed()>=budget)throw std::length_error("BL direction refresh exhausted the shared deadline.");
      size_t stars=0;
      for(const auto& face:current.wall_index)for(Id vertex:face.first) {
        const auto found=current.vertex_cells.find(vertex);
        if(found!=current.vertex_cells.end())stars=transfer_memory::Add(stars,found->second.size());
      }
      const auto scratch=transfer_memory::Add(transfer_memory::Mul(current.wall_index.size(),3*1024),transfer_memory::Mul(stars,1024));
      std::string why;if(!memoryOk("direction field",why,scratch))throw std::length_error(why);
      directions=ComputeDirectionField(current);current.direction_revision=current.boundary_revision;
      directionBytes=transfer_memory::Add(sizeof(directions),transfer_memory::Mul(directions.d.size(),sizeof(decltype(directions.d)::value_type)+4*sizeof(void*)),
          transfer_memory::Bytes(directions.witnesses));
      for(const auto& witness:directions.witnesses)directionBytes=transfer_memory::Add(directionBytes,witness.capacity()+1);
      if(!memoryOk("retained direction field",why))throw std::length_error(why);
      dc9::directionField=&directions.d;
    };
    dc9::ensureDirectionField(*s);dc9::directionFieldUses=0;
    if (!chain) {
      phase("W", "surface_prepared=1");
      if (!memoryOk("W", why)) return finish(why);
      json << ",\"directions\":" << JsonDirections(directions);
      const auto r1 = dc9::PhaseR1(*s, rows, std::min(r1Work,ctl.maximum_work-workDone), .05 * usable, log);workDone += r1.work;
      json << ",\"R1\":" << JsonStage(r1);
      phase("R1", "accepted=" + std::to_string(r1.accepted) + " outcome=" + r1.outcome);
      const auto r2 = dc9::PhaseR2(*s, static_cast<Rows&>(rows), std::min(.25*usable,budget-elapsed()), log,ctl.maximum_work-workDone);workDone += r2.work;
      json << ",\"R2\":" << JsonStage(r2) << ",\"R2_counts\":{\"lifted\":" << r2.lifted << ",\"unlifted\":" << r2.unlifted << ",\"untried\":" << r2.untried
           << ",\"rowed\":" << r2.rowed << ",\"unrowed\":" << r2.unrowed << "}";
      phase("R2", "rowed=" + std::to_string(r2.rowed) + " unrowed=" + std::to_string(r2.unrowed) + " unlifted=" + std::to_string(r2.unlifted) +
                  " untried=" + std::to_string(r2.untried) + " outcome=" + r2.outcome);
      if (!memoryOk("R2", why)) return finish(why);
      MapLayers(*s);
      const auto cp = dc15::CompleteFirstRow(*s, rows, std::min(std::max(60., .1 * usable),budget-elapsed()), certified, log,ctl.maximum_work-workDone);
      workDone += cp.work;
      json << ",\"completion\":" << dc15::JsonCompletion(cp);
      phase("completion", "failing=" + std::to_string(cp.failing_before) + "->" + std::to_string(cp.failing_after) + " inserted=" +
                          std::to_string(cp.inserted) + " cavity=" + std::to_string(cp.cavity) + " separated=" + std::to_string(cp.separated) + " outcome=" + cp.outcome);
      snapshot = FirstChildren(*s);
      certified.clear();  // code review 3: the A3 audit covers only the certified operations after the snapshot
      MapLayers(*s);
      G = PhaseG(*s, rows, std::min(.15*usable,budget-elapsed()), log,{},ctl.maximum_work-workDone);workDone += G.work;
      json << ",\"G\":" << JsonGrowth(*s, G);
      phase("G", "lifted=" + std::to_string(G.lifted) + " refused=" + std::to_string(G.refused) + " clear_moves=" + std::to_string(G.clear_moves) +
                 " clear_collapses=" + std::to_string(G.clear_collapses) + " retries_accepted=" + std::to_string(G.clear_retry_accepted));
      if (!memoryOk("G", why)) return finish(why);
    } else {
      MapLayers(*s);
      snapshot = FirstChildren(*s);
      // DC-17: shared height-constrained lids, lateral flips and prism rebuild; DC-16 cone split as fallback.
      HalveStats hv;
      ColumnStats cs;
      if (dc17::rebuildFirstRow) {
        const auto rb = dc17::RebuildFirstRow(*s, rows, certified,ctl.maximum_work-workDone,budget-elapsed());
        workDone += rb.work;
        hv = rb.halve; hv.faces = rb.faces; hv.split += rb.rebuilt; cs = rb.column;
        json << ",\"rebuild\":" << dc17::JsonRebuild(rb);
      } else {
        hv = HalveFirstRow(*s, rows, certified,ctl.maximum_work-workDone,budget-elapsed());workDone += hv.faces;
        cs = ColumnInsert(*s, rows,ctl.maximum_work-workDone,budget-elapsed());workDone += cs.work;
      }
      json << ",\"halve\":{\"faces\":" << hv.faces << ",\"split\":" << hv.split << ",\"placed\":" << hv.placed << ",\"already_within\":"
           << hv.skipped_low << ",\"refused\":" << hv.refused << "},\"column_insert\":{\"columns\":" << cs.columns << ",\"inserted\":"
           << cs.inserted << ",\"refused\":" << cs.refused << "}";
      phase("halving", "split=" + std::to_string(hv.split) + " refused=" + std::to_string(hv.refused) + " inserted=" + std::to_string(cs.inserted));
      if (!memoryOk("halving", why)) return finish(why);
      // Current height requests must be completed independently of the optional volume-optimization budget.
      const auto cp = dc15::CompleteFirstRow(*s, rows, std::min(std::max(60., .1 * usable),budget-elapsed()), certified, log,ctl.maximum_work-workDone);
      workDone += cp.work;
      json << ",\"completion\":" << dc15::JsonCompletion(cp);
      phase("completion", "failing=" + std::to_string(cp.failing_before) + "->" + std::to_string(cp.failing_after));
      if (!memoryOk("completion", why)) return finish(why);
    }
    json << ",\"work\":" << workDone << ",\"construction_seconds\":" << elapsed();
    if(workDone>ctl.maximum_work)return finish("FIRST_ROW construction exhausted shared native work.");
    if(elapsed()>=budget)return finish("FIRST_ROW construction exhausted the shared elapsed deadline.");
    size_t a3Violations = 0;
    {const auto a3 = FirstRowAudit(*s, snapshot, certified);
     a3Violations = a3.violations;
     json << ",\"first_row_audit\":" << JsonAudit(a3);}
    const auto validity = dc15::IndependentValidity(*s);
    json << ",\"validity\":" << dc15::JsonValidity(validity)
         << ",\"audit\":{\"deferred\":\"distributed quality stage\"}";
    phase("validity", "valid=" + std::to_string(validity.Valid()) + " nonpositive=" + std::to_string(validity.nonpositive));
    auto geometric_validity=validity;geometric_validity.height_failures=0;
    const bool accepted = geometric_validity.Valid() && a3Violations == 0;
    if(ctl.export_mesh && !ctl.report_prefix.empty()) {
      layer2d::Export(*s,ctl.report_prefix+"_final.mesh");
      json << ",\"export_acceptance\":\"provisional construction\"";
      phase("export");
    }
    // Live private publication retained the same geometry, mobility, coverage and active references.
    if(s->live_parameters) {
      out.parameters=s->live_parameters;
      out.reference=s->reference->WithParameterState(s->live_parameters); // Unleased; the event budget cannot escape.
    } else if (s->mapped) {
      BoundaryPoints points;
      std::vector<SurfaceFace> surface;
      const auto* params = s->reference->Parameters();
      for (const auto& row : s->mesh) {
        const auto faces = CellFaces(row.second.cell);
        for (unsigned k = 0; k < 4; ++k) {
          if (!row.second.physical[k].present) continue;
          surface.push_back({faces[k], row.second.physical[k].facet});
          for (Id v : faces[k]) {
            if (points.count(v)) continue;
            const auto o = s->overlay.find(v);
            if (o != s->overlay.end()) { points.emplace(v, o->second); continue; }
            const auto* b = params ? params->Find(v) : nullptr;
            if (!b) return finish("FIRST_ROW: skin vertex " + std::to_string(v) + " has no boundary binding.");
            points.emplace(v, *b);
          }
        }
      }
      auto parameters = std::make_shared<BoundaryParameterState>(params->Authority(), points, surface);
      out.reference = s->reference->WithParameterState(parameters);
      out.parameters = parameters;
    } else out.reference = s->reference;
    // pending continuation (committed only after the replacement succeeds)
    auto reg = std::make_shared<RowRegistry>();
    auto P = [&](Id v) { const auto p = NodePoint(*s, v); return RowRegistry::P{p.x, p.y, p.z}; };
    auto live = [&](Id v) { return s->vertex_cells.count(v) > 0; };
    auto F = [&](const Face& k) { return std::array<RowRegistry::P, 3>{P(k[0]), P(k[1]), P(k[2])}; };
    auto liveFace = [&](const Face& k) { return live(k[0]) && live(k[1]) && live(k[2]); };
    for (const auto& r : rows.lift) if (live(r.first) && live(r.second)) reg->lift.push_back({P(r.first), P(r.second)});
    for (const auto& r : rows.apex) if (liveFace(r.first) && live(r.second)) reg->apex.push_back({F(r.first), P(r.second)});
    for (const auto& r : rows.overrides) {
      const auto e = rows.extras.find(r.first);
      bool ok = liveFace(r.first) && e != rows.extras.end() && live(r.second[0]) && live(r.second[1]) && live(r.second[2]);
      if (ok) for (const auto& x : e->second) ok = ok && liveFace(x.first);
      if (!ok) continue;
      reg->overrides.push_back({F(r.first), {P(r.second[0]), P(r.second[1]), P(r.second[2])}});
      std::vector<std::pair<std::array<RowRegistry::P, 3>, bool>> list;
      for (const auto& x : e->second) list.push_back({F(x.first), x.second});
      reg->extras.push_back({F(r.first), list});
    }
    reg->unsupported = rows.copies.size() + rows.fan.size();
    reg->digest = CoordinateDigest(s->mesh);
    cont.pending = reg;
    json << ",\"registry\":{\"lift\":" << reg->lift.size() << ",\"apex\":" << reg->apex.size() << ",\"overrides\":" << reg->overrides.size()
         << ",\"unsupported\":" << reg->unsupported << "},\"perf\":" << JsonPerf() << ",\"peak_rss_kB\":" << dc9::PeakKB()
         << ",\"total_seconds\":" << elapsed();
    out.cells.swap(s->mesh);
    out.valid = accepted;
    out.stats.final_owned_cells = out.cells.size();
    out.stats.global_work = workDone;
    std::ostringstream reason;
    if (!accepted) {
      reason << "FIRST_ROW provisional geometry/height construction rejected: nonpositive " << validity.nonpositive
             << ", height " << validity.height_failures << ", topology "
             << validity.overshared + validity.same_orientation + validity.interior_physical + validity.open_unbound + validity.nonmanifold_skin_edges + validity.inconsistent_points
             << ", first-row violations " << a3Violations;
      if (!ctl.report_prefix.empty()) reason << "; report " << ctl.report_prefix << "_bl_report.json";
    }
    phase("pack", std::string("construction_valid=") + (accepted ? "1" : "0"));
    return finish(reason.str());
  } catch (const std::exception& e) {
    out.valid = false;
    out.cells.clear();
    return finish(std::string("FIRST_ROW construction failed: ") + e.what());
  }
}
}  // namespace SU2NativeBLPipeline3D
