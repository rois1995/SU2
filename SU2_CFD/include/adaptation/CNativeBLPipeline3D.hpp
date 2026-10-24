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
#include "../../../Common/include/adaptation/CNativeChordEnvelope.hpp"
#include "../../../Common/include/adaptation/CNativeOptimize3D.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "CBoundaryLayerMetric.hpp"
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

/*--- Production entry (DC-14): one remesh event of ADAP_BL_METHOD= FIRST_ROW. ---*/
namespace SU2NativeBLPipeline3D {
using namespace SU2Native3D;

/*! One immutable per-event target: full layers (production D1), base layers (no D1 feature), option A blending them
 *  (D1 kept off the wall), the frozen sensor core. The same object gives the composition of the remesher's
 *  reassessment and of the construction's frozen field. */
struct TargetPolicy {
  std::shared_ptr<CBoundaryLayerMetric> full, base;
  std::shared_ptr<dc10::OptionA> A;
  double core = 0, h0 = 0, outer_parameter = -1;
  std::map<int, int> roles;  // marker -> 1 BL wall, 0 reflective continuation (MARKER_SYM), 2 other physical partner
  MetricComposition Composition() const { return dc10::CompositionA(base, full, core, A); }
  MetricEdgeBreakpoints Edges() const { return dc10::EdgeGeometryA(base, full, core, A); }
  size_t StorageBytes() const { return full->StorageBytes() + base->StorageBytes(); }
};

/*! Marker roles from the configuration kinds only (no marker names): BL markers 1, symmetry planes 0, others 2. */
inline std::map<int, int> MarkerRoles(const CConfig& config, const ReferenceState& state) {
  std::map<int, int> roles;
  for (size_t m = 0; m < state.marker_names.size(); ++m) {
    const auto& name = state.marker_names[m];
    bool bl = false;
    for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) bl = bl || config.GetAdap_BL(b).marker == name;
    roles[int(m)] = bl ? 1 : config.GetMarker_CfgFile_KindBC(name) == SYMMETRY_PLANE ? 0 : 2;
  }
  return roles;
}

/*! outerParameter < 0: no budget outer-normal profile (ADAP_BL_BUDGET= NO); otherwise the selected frozen parameter. */
inline std::shared_ptr<TargetPolicy> MakeTargetPolicy(const CConfig& config, const ReferenceState& state, double core,
                                                      double outerParameter) {
  auto p = std::make_shared<TargetPolicy>();
  p->core = core;
  p->outer_parameter = outerParameter;
  p->full = CBoundaryLayerMetric::FromNativeReference(state, config);
  p->base = CBoundaryLayerMetric::FromNativeReference(state, config, 179.999);
  for (auto* layers : {p->full.get(), p->base.get()}) {
    if (config.GetAdap_BL_Budget()) {
      if (!(outerParameter >= 0)) throw std::invalid_argument("FIRST_ROW with ADAP_BL_BUDGET needs the selected outer-normal parameter.");
      layers->ConfigureOuterNormal(SU2_TYPE::GetValue(config.GetAdap_BL_ProtectDistance()),
                                   SU2_TYPE::GetValue(config.GetAdap_BL_OuterGrowthMax()));
      layers->SelectOuterNormal(outerParameter);
      layers->FreezeOuterNormal();
    }
  }
  if (p->base->GetFeatures().segments != 0) throw std::runtime_error("FIRST_ROW base layers still carry a D1 feature.");
  p->roles = MarkerRoles(config, state);
  p->h0 = std::numeric_limits<double>::infinity();
  for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) p->h0 = std::min(p->h0, SU2_TYPE::GetValue(config.GetAdap_BL(b).firstHeight));
  p->A = dc10::BuildOptionA(state.facets, p->roles, p->full->GetFeatures().angle, p->full->GetFeatures().maximumRadius, p->h0);
  p->A->distanceLayers = p->base;
  p->A->core = core;
  return p;
}

/*! First-row registry of one accepted event, keyed by exact coordinates (continuation of the next event). */
struct RowRegistry {
  using P = std::array<double, 3>;
  std::vector<std::pair<P, P>> lift;                                  // wall vertex -> lid
  std::vector<std::pair<std::array<P, 3>, P>> apex;                   // wall face -> apex
  std::vector<std::pair<std::array<P, 3>, std::array<P, 3>>> overrides;
  std::vector<std::pair<std::array<P, 3>, std::vector<std::pair<std::array<P, 3>, bool>>>> extras;
  uint64_t digest = 0;   // FNV-1a of the sorted vertex coordinates of the mesh it was written for
  size_t unsupported = 0;  // crease-fan copies/fan cells (not produced by the single lift) are not continued
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
  size_t memory_ceiling = size_t(2) << 30;
  std::string report_prefix;   // <prefix>_bl_report.json (and <prefix>_final.mesh when export_mesh)
  bool export_mesh = false;
  double intermediate_v_seconds = 900;  // DC-15: V cap of an intermediate (ramped-h0) event; the final cycle uses the budget
  bool full_audit = false;              // DC-15: run the uncached audit also on intermediate events (diagnostic)
};

namespace detail {
// Requested-storage estimate of the construction: its state (cells, stars, wall index, box grid), caches, the frozen
// field's donor copy and index, plus the caller's retained input and source. Not RSS.
inline size_t StateBytes(const junction_wp0::State& s, size_t caller = 0, size_t donors = 0) {
  return s.mesh.size() * 640 + s.wall_index.size() * 80 + s.box_grid.size() * 96 + s.lengths.size() * 112 +
         dc9::shapeCache.size() * 48 + dc9::heightCache.size() * 48 + dc9::geometryLengths.size() * 104 +
         dc9::balancedCache.size() * 120 + s.overlay.size() * 160 + donors * (sizeof(DonorCell) + 64) + caller;
}
/*! Restores every pipeline-global switch and cache on exit (also on exceptions). */
struct PipelineScope {
  dc12::Policies policies;
  PipelineScope() {
    dc9::perf = dc9::Perf{}; dc9::ResetCaches(); dc9::cachesOn = true; dc9::balancedCache.clear();
    dc9::firstChildDiagnostics = 0; dc9::geometryHits = 0;
    dc10::reflectiveByRole = false; dc10::reflectiveMarkers.clear(); dc9::artificialMarkers.clear();
    dc13::v13R2Retry = true; dc13::unliftedColumns = false; dc13::diagnosticCutSlide = false; dc13::v13AttemptsPerSweep = 1024;
    dc13::v13NearWallFirst = false; dc12::growthClearing = true; dc13::v13EstimateScan = false; dc13::v13SkipFinalOnLimit = false;
    dc9::rimByRole = true;  // DC-15: artificial boundaries only by role (production has none), never by a marker name
  }
  ~PipelineScope() {
    dc9::ResetCaches(); dc9::balancedCache.clear(); dc9::cachesOn = true; dc12::layerOfMarker.clear();
    dc10::optionA.reset(); dc10::probeBase.reset(); dc10::probeFull.reset();
    dc13::v13R2Retry = true; dc13::v13AttemptsPerSweep = 1024; dc9::directionField = nullptr; dc9::directionFieldUses = 0;
    dc13::v13NearWallFirst = false; dc12::growthClearing = true; dc13::v13EstimateScan = false; dc13::v13SkipFinalOnLimit = false;
    dc9::rimByRole = false;
  }
};
}  // namespace detail

/*! One FIRST_ROW event on one worker. Fresh mesh (no committed registry matching it): W -> R1 -> R2 -> G -> V13.
 *  Continuation (registry of the previous accepted event, digest-matched): halving of the first children, column
 *  redistribution, V13 (DC-13 chain step). Acceptance by an uncached audit with a fresh field: intermediate events
 *  (ramped h0) need every cell valid (positive, q > 0, J >= 1e-3) and every first height within T of h0_k; the final
 *  event needs the final gates. The report carries every deficit. */
inline EngineResult RunEvent(World& world, const Owned& cells, const std::vector<DonorCell>& donors,
                             const ReferenceState& state, const TargetPolicy& policy, const CConfig& config,
                             Continuation& cont, const EventControls& ctl, std::string& report) {
  using namespace dc13;
  using Clock = std::chrono::steady_clock;
  EngineResult out;
  out.valid = false;
  std::ostringstream json;
  json << std::setprecision(12) << "{\"event\":" << ctl.event << ",\"ramp_factor\":" << ctl.ramp_factor
       << ",\"final_cycle\":" << ctl.final_cycle << ",\"input_cells\":" << cells.size();
  auto finish = [&](const std::string& reason) {
    out.reason = reason;
    json << ",\"accepted\":" << out.valid << ",\"reason\":" << junction_wp0::Json(reason) << "}\n";
    report = json.str();
    if (!ctl.report_prefix.empty()) { std::ofstream(ctl.report_prefix + "_bl_report.json") << report; }
    return out;
  };
  if (world.size != 1) return finish("ADAP_BL_METHOD= FIRST_ROW runs on one native worker (set ADAP_NATIVE_RANKS= 1).");
  try {
    const auto t0 = Clock::now();
    const double budget = ctl.seconds > 0 ? ctl.seconds : 7200.0;
    const double reserve = std::max(120.0, .1 * budget), usable = std::max(1.0, budget - reserve);
    auto elapsed = [&] { return std::chrono::duration<double>(Clock::now() - t0).count(); };
    detail::PipelineScope scope;
    dc10::optionA = policy.A; dc10::probeBase = policy.base; dc10::probeFull = policy.full;
    auto s = std::make_unique<State>();
    s->name = "native_first_row";
    s->markers = state.marker_names;
    s->facets = state.facets;
    double declared = 0;
    if (!state.original) return finish("FIRST_ROW needs the original geometry authority.");
    const bool plainFaceted = !state.original->Mapped() && state.original->Analytic().empty() && state.original->Affine().empty();
    s->reference = plainFaceted ? PromoteFacetedReference(state.original, state.facets, 1e-13, declared) : state.original;
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
    auto makeField = [&](const junction_wp0::State& x) {
      return std::make_unique<FrozenField>(donors, policy.Composition(), true, AssociatedExtensions(donors, *x.reference),
                                           policy.Edges(), std::make_shared<const FrozenMetricIdentity>(), true,
                                           std::max(MaximumDonors, donors.size()));  // whole original domain (admitted by StateBytes)
    };
    s->field = makeField(*s);
    Prepare(*s);
    ApplyWallPolicies(*s);
    DetectCreases(*s, policy.roles);  // DC-15: crease roles from the configuration marker kinds
    json << ",\"authority\":{\"mapped_input\":" << state.original->Mapped() << ",\"declared_error\":" << declared
         << "},\"wall_faces\":" << WallFaces(*s).size() << ",\"h0_min\":" << s->h0;
    const size_t callerBytes = cells.size() * 640 + donors.size() * (sizeof(DonorCell) + 64);
    auto memoryOk = [&](const char* where, std::string& why, size_t extra = 0) {
      const auto bytes = detail::StateBytes(*s, callerBytes + extra, donors.size());
      if (bytes <= ctl.memory_ceiling) return true;
      why = std::string("FIRST_ROW state estimate exceeds the native memory ceiling at ") + where + " (" +
            std::to_string(bytes) + " > " + std::to_string(ctl.memory_ceiling) + " bytes)";
      return false;
    };
    // continuation: the committed registry of the previous accepted event, digest-matched to this mesh
    Rows10 rows;
    size_t mapped = 0, dropped = 0;
    const auto digest = CoordinateDigest(s->mesh);
    std::string continuation = "none";
    if (cont.committed) {
      if (cont.committed->digest != digest) continuation = "digest mismatch (" + cont.source + ")";
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
    size_t wallEdges = 0, wallVertices = 0;
    { std::set<Edge> e; std::set<Id> v;
      for (const auto& wf : WallFaces(*s)) for (int i = 0; i < 3; ++i) { e.insert({std::min(wf.second[i], wf.second[(i + 1) % 3]), std::max(wf.second[i], wf.second[(i + 1) % 3])}); v.insert(wf.second[i]); }
      wallEdges = e.size(); wallVertices = v.size(); }
    const size_t wWork = std::max<size_t>(8192, 4 * wallEdges), r1Work = std::max<size_t>(4096, 2 * wallVertices);
    v13AttemptsPerSweep = 0;  // DC-15: per sweep from the current long-edge demand
    v13NearWallFirst = true;
    v13EstimateScan = v13SkipFinalOnLimit = !ctl.final_cycle;  // intermediate events: deficits reported once, estimated
    json << ",\"limits\":{\"budget_seconds\":" << budget << ",\"reserve_seconds\":" << reserve << ",\"W\":[" << wWork << "," << .08 * usable
         << "],\"R1\":[" << r1Work << "," << .05 * usable << "],\"R2_seconds\":" << .25 * usable << ",\"G_seconds\":" << .12 * usable
         << ",\"V_attempts_per_sweep\":\"max(1024, long/4)\",\"V_intermediate_seconds\":" << ctl.intermediate_v_seconds
         << ",\"memory_ceiling\":" << ctl.memory_ceiling << "}";
    std::ostringstream log;
    std::string why;
    // DC-15: the construction log is streamed to <prefix>_bl.log and every phase prints one stdout line and appends its
    // JSON to <prefix>_bl_report.partial.json (a killed run keeps its phase receipts)
    std::ofstream logFile;
    if (!ctl.report_prefix.empty()) logFile.open(ctl.report_prefix + "_bl.log");
    double phaseStart = elapsed();
    auto phase = [&](const char* name, const std::string& extra = "") {
      const double now = elapsed();
      std::cout << "DC15 phase " << name << " seconds=" << std::setprecision(6) << now - phaseStart << " elapsed=" << now << " cells=" << s->mesh.size()
                << " rss_kB=" << dc9::PeakKB() << (extra.empty() ? "" : " ") << extra << std::endl;
      json << ",\"phase_" << name << "\":{\"seconds\":" << now - phaseStart << ",\"elapsed\":" << now << ",\"cells\":" << s->mesh.size() << "}";
      if (logFile) { logFile << log.str(); logFile.flush(); }
      log.str(""); log.clear();
      if (!ctl.report_prefix.empty()) std::ofstream(ctl.report_prefix + "_bl_report.partial.json") << json.str() << "}\n";
      phaseStart = now;
    };
    phase("prepare", "wall_faces=" + std::to_string(WallFaces(*s).size()));
    Growth G;
    std::set<Face> certified;
    std::map<Face, std::array<Id, 4>> snapshot;  // A3: first children after R2 + completion
    uint64_t workDone = 0;
    auto cappedV = [&] {
      // the final cycle keeps the remaining budget minus the uncached audit estimate; intermediate events are capped
      const double auditEstimate = (ctl.final_cycle || ctl.full_audit) ? .005 * double(s->mesh.size()) : 0.;
      double v = usable - elapsed() - auditEstimate;
      if (!ctl.final_cycle) v = std::min(v, ctl.intermediate_v_seconds);
      return std::max(1.0, v);
    };
    if (!chain) {
      const auto w = dc10::PhaseW10(*s, wWork, .08 * usable, log);workDone += w.work;
      json << ",\"W\":" << JsonStage(w);
      phase("W", "splits=" + std::to_string(w.splits) + " outcome=" + w.outcome);
      if (!memoryOk("W", why)) return finish(why);
      auto field = ComputeDirectionField(*s);
      dc9::directionField = &field.d; dc9::directionFieldUses = 0;
      json << ",\"directions\":" << JsonDirections(field);
      const auto r1 = dc9::PhaseR1(*s, rows, r1Work, .05 * usable, log);workDone += r1.work;
      json << ",\"R1\":" << JsonStage(r1);
      phase("R1", "accepted=" + std::to_string(r1.accepted) + " outcome=" + r1.outcome);
      const auto r2 = dc9::PhaseR2(*s, static_cast<Rows&>(rows), .25 * usable, log);workDone += r2.work;
      json << ",\"R2\":" << JsonStage(r2) << ",\"R2_counts\":{\"lifted\":" << r2.lifted << ",\"unlifted\":" << r2.unlifted << ",\"untried\":" << r2.untried
           << ",\"rowed\":" << r2.rowed << ",\"unrowed\":" << r2.unrowed << "}";
      phase("R2", "rowed=" + std::to_string(r2.rowed) + " unrowed=" + std::to_string(r2.unrowed) + " unlifted=" + std::to_string(r2.unlifted) +
                  " untried=" + std::to_string(r2.untried) + " outcome=" + r2.outcome);
      if (!memoryOk("R2", why)) return finish(why);
      MapLayers(*s);
      const auto cp = dc15::CompleteFirstRow(*s, rows, std::max(60., .1 * usable), certified, log);
      json << ",\"completion\":" << dc15::JsonCompletion(cp);
      phase("completion", "failing=" + std::to_string(cp.failing_before) + "->" + std::to_string(cp.failing_after) + " inserted=" +
                          std::to_string(cp.inserted) + " cavity=" + std::to_string(cp.cavity) + " separated=" + std::to_string(cp.separated) + " outcome=" + cp.outcome);
      snapshot = FirstChildren(*s);
      certified.clear();  // code review 3: the A3 audit covers only the certified operations after the snapshot
      MapLayers(*s);
      G = PhaseG(*s, rows, .12 * usable, log);workDone += G.transactions + G.refused;
      json << ",\"G\":" << JsonGrowth(*s, G);
      phase("G", "lifted=" + std::to_string(G.lifted) + " refused=" + std::to_string(G.refused) + " clear_moves=" + std::to_string(G.clear_moves) +
                 " clear_collapses=" + std::to_string(G.clear_collapses) + " retries_accepted=" + std::to_string(G.clear_retry_accepted));
      if (!memoryOk("G", why)) return finish(why);
      v13R2Retry = true;
      const double vSeconds = cappedV();
      const auto v = PhaseV13(*s, rows, G, std::max<size_t>(size_t(1) << 22, 64 * s->mesh.size()), vSeconds, 64, certified, log);
      json << ",\"V\":{\"stage\":" << JsonStage(v) << ",\"seconds_cap\":" << vSeconds << ",\"sweeps\":" << v.sweeps << ",\"refine_attempts\":" << v.refine_attempts
           << ",\"seconds\":{\"r2\":" << v.r2_seconds << ",\"first\":" << v.first_seconds << ",\"refine\":" << v.refine_seconds << ",\"optimize\":" << v.optimize_seconds << "}}";
      workDone += v.work;
      phase("V", "sweeps=" + std::to_string(v.sweeps) + " attempts=" + std::to_string(v.refine_attempts) + " accepted=" + std::to_string(v.accepted) + " outcome=" + v.outcome);
      dc9::directionField = nullptr;
    } else {
      MapLayers(*s);
      auto field = ComputeDirectionField(*s);
      dc9::directionField = &field.d; dc9::directionFieldUses = 0;
      snapshot = FirstChildren(*s);
      const auto hv = HalveFirstRow(*s, rows, certified);
      const auto cs = ColumnInsert(*s, rows);workDone += hv.faces + cs.inserted + cs.refused;
      json << ",\"halve\":{\"faces\":" << hv.faces << ",\"split\":" << hv.split << ",\"placed\":" << hv.placed << ",\"already_within\":"
           << hv.skipped_low << ",\"refused\":" << hv.refused << "},\"column_insert\":{\"columns\":" << cs.columns << ",\"inserted\":"
           << cs.inserted << ",\"refused\":" << cs.refused << "}";
      phase("halving", "split=" + std::to_string(hv.split) + " refused=" + std::to_string(hv.refused) + " inserted=" + std::to_string(cs.inserted));
      if (!memoryOk("halving", why)) return finish(why);
      v13R2Retry = false;
      const double vSeconds = cappedV();
      const auto v = PhaseV13(*s, rows, G, std::max<size_t>(size_t(1) << 22, 64 * s->mesh.size()), vSeconds, 64, certified, log);
      json << ",\"V\":{\"stage\":" << JsonStage(v) << ",\"seconds_cap\":" << vSeconds << ",\"sweeps\":" << v.sweeps << ",\"refine_attempts\":" << v.refine_attempts << "}";
      workDone += v.work;
      phase("V", "sweeps=" + std::to_string(v.sweeps) + " attempts=" + std::to_string(v.refine_attempts) + " outcome=" + v.outcome);
      dc9::directionField = nullptr;
    }
    json << ",\"work\":" << workDone << ",\"construction_seconds\":" << elapsed();
    // deficits against the final gates from ONE cached scan of the construction state (labelled cached)
    // intermediate events: cached lengths plus midpoint estimates for unmeasured edges (no integration of every edge);
    // final cycle: the cached Final (every edge measured) before the uncached audit
    dc9::Gates8 cached;
    size_t cachedMetricInvalid = 0;
    if (ctl.final_cycle || ctl.full_audit) {
      cached = dc9::Final(*s);
      for (const auto& row : s->mesh) { const auto sh = dc9::MeshShape(*s, row.first); if (!(sh.q > 0 && sh.J >= ValiditySliverJacobian)) ++cachedMetricInvalid; }
      json << ",\"final_cached\":" << dc9::JsonFinal(*s, rows);
    } else {
      const auto est = dc15::FinalEstimated(*s);
      cached = est.gates; cachedMetricInvalid = est.metric_invalid;
      json << ",\"final_estimated\":" << dc15::JsonEstimated(est);
    }
    json << ",\"metric_invalid_cached\":" << cachedMetricInvalid << ",\"bad_witness\":" << BadWitness(*s, rows);
    {const auto a3 = FirstRowAudit(*s, snapshot, certified);
     json << ",\"first_row_audit\":" << JsonAudit(a3);}
    phase("deficits", "bad=" + std::to_string(cached.bad_shape) + " long=" + std::to_string(cached.long_edges) + " height=" + std::to_string(cached.height_failures));
    if (ctl.export_mesh && !ctl.report_prefix.empty()) {
      std::function<long double(Id, Id)> length;
      if (!(ctl.final_cycle || ctl.full_audit)) length = [&](Id a, Id b) { bool est = false; return dc13::LenOrEstimate(*s, a, b, est); };
      dc10::ExportGates10(*s, rows, ctl.report_prefix + "_final.mesh", length);
      phase("export");
    }
    // independent validity of the returned cells (exact orientation, conformity, skin, first heights; no construction cache)
    const auto validity = dc15::IndependentValidity(*s);
    json << ",\"validity\":" << dc15::JsonValidity(validity);
    phase("validity", "valid=" + std::to_string(validity.Valid()) + " nonpositive=" + std::to_string(validity.nonpositive) + " height=" +
                      std::to_string(validity.height_failures));
    dc9::Gates8 audit = cached;
    size_t invalid = validity.nonpositive, metricInvalid = cachedMetricInvalid;
    bool audited = false;
    if (ctl.final_cycle || ctl.full_audit) {
      // the audit allocates a second complete state and field
      if (!memoryOk("audit", why, s->mesh.size() * 640 + donors.size() * (sizeof(DonorCell) + 64))) return finish(why);
      // independent audit: fresh state and field from the same policy, caches off
      dc9::ResetCaches(); dc9::cachesOn = false;
      auto f = std::make_unique<State>();
      f->name = s->name; f->reference = s->reference; f->facets = s->facets; f->markers = s->markers; f->walls = s->walls;
      f->heights = s->heights; f->h0 = s->h0; f->growth = s->growth; f->thickness = s->thickness; f->core = s->core;
      f->layers = s->layers; f->mesh = s->mesh; f->overlay = s->overlay; f->mapped = s->mapped;
      f->field = makeField(*f);
      Prepare(*f);
      audit = dc9::Final(*f);
      invalid = 0; metricInvalid = 0;
      for (const auto& row : f->mesh) {
        const auto& c = row.second.cell;
        const auto sh = dc9::MeshShape(*f, row.first);
        if (!(Orientation(c.v[0].p, c.v[1].p, c.v[2].p, c.v[3].p) > 0)) ++invalid;
        if (!(sh.q > 0 && sh.J >= ValiditySliverJacobian)) ++metricInvalid;
      }
      dc9::cachesOn = true; audited = true;
      json << ",\"audit\":{\"cells\":" << audit.cells << ",\"q_min\":" << audit.q << ",\"J_min\":" << audit.J << ",\"edge_max\":" << double(audit.lmax)
           << ",\"bad_shape\":" << audit.bad_shape << ",\"long_edges\":" << audit.long_edges << ",\"height_failures\":" << audit.height_failures
           << ",\"unmeasurable\":" << audit.unmeasurable << ",\"invalid_cells\":" << invalid << ",\"metric_invalid_cells\":" << metricInvalid
           << ",\"final_gates\":" << audit.complete
           << ",\"agrees_with_cached\":" << (cached.bad_shape == audit.bad_shape && cached.long_edges == audit.long_edges &&
                                             cached.height_failures == audit.height_failures && cached.unmeasurable == audit.unmeasurable) << "}";
      phase("audit", "complete=" + std::to_string(audit.complete));
    } else json << ",\"audit\":{\"skipped\":\"intermediate event: decision by the independent validity check, deficits cached\"}";
    const bool accepted = validity.Valid() && (!ctl.final_cycle || audit.complete);
    // boundary parameters of the final skin: accepted private bindings, else the authority's own binding
    if (s->mapped) {
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
      reason << (ctl.final_cycle ? "FIRST_ROW final gates not met" : "FIRST_ROW intermediate mesh not valid") << ": bad " << audit.bad_shape
             << ", long " << audit.long_edges << ", height " << audit.height_failures << ", unmeasurable " << audit.unmeasurable
             << ", invalid " << invalid << " (q_min " << audit.q << ", J_min " << audit.J << (audited ? ", uncached audit" : ", cached deficits")
             << "); validity: nonpositive " << validity.nonpositive << ", height " << validity.height_failures << ", topology "
             << validity.overshared + validity.same_orientation + validity.interior_physical + validity.open_unbound + validity.nonmanifold_skin_edges + validity.inconsistent_points;
      if (!ctl.report_prefix.empty()) reason << "; report " << ctl.report_prefix << "_bl_report.json";
    }
    phase("pack", std::string("accepted=") + (accepted ? "1" : "0"));
    return finish(reason.str());
  } catch (const std::exception& e) {
    out.valid = false;
    out.cells.clear();
    return finish(std::string("FIRST_ROW construction failed: ") + e.what());
  }
}
}  // namespace SU2NativeBLPipeline3D
