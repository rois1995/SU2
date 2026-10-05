/*!
 * \file CNativeRemesher.cpp
 * \brief Native 2D adaptation of actual distributed SU2 geometry and nodal metrics.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/adaptation/CNativeRemesher.hpp"
#include "../../include/adaptation/CNativeEngine2D.hpp"
#include "../../include/adaptation/CNativeReferenceIO.hpp"
#include "../../include/adaptation/CMeshGather.hpp"
#include "../../include/CConfig.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include <iostream>

using namespace SU2NativeBoundary2D;

CNativeRemesher::CNativeRemesher(std::shared_ptr<ReferenceState> retainedReference)
    : reference(std::move(retainedReference)) {
  if (!reference) throw std::invalid_argument("Native remesher needs driver-owned reference storage.");
}

void CNativeRemesher::CheckSupport(const CConfig& config, const CGeometry& geometry) {
  CLocalFailure failure;
  if (!std::is_same<su2double, double>::value || !std::is_same<passivedouble, double>::value)
    failure.Set(1, 0, "Native adaptation currently requires a primal double-precision build.");
  if (config.GetContinuous_Adjoint() || config.GetDiscrete_Adjoint())
    failure.Set(1, 0, "Native adaptation does not support continuous or discrete adjoint configurations.");
  if (geometry.GetnDim() != 2) failure.Set(1, 0, "Native adaptation currently supports 2D triangles only.");
  if (config.GetMultizone_Problem() || config.GetnZone() > 1 || config.GetnMarker_ZoneInterface() > 0)
    failure.Set(1, 0, "Native adaptation does not support multizone or sliding meshes.");
  if (config.GetnMarker_Periodic() > 0) failure.Set(1, 0, "Native adaptation does not support periodic boundaries.");
  if (config.GetGrid_Movement() || config.GetDynamic_Grid() || config.GetDeform_Mesh())
    failure.Set(1, 0, "Native adaptation does not support moving/deforming meshes.");
  if (config.GetFEMSolver() || !config.GetMarkerCreateCopy().empty())
    failure.Set(1, 0, "Native adaptation does not support FEM or MARKER_CREATE_COPY.");
  if (config.GetTime_Domain()) failure.Set(1, 0, "Native time-domain adaptation is not enabled yet.");
  for (unsigned long e = 0; e < geometry.GetnElem(); ++e)
    if (geometry.elem[e]->GetVTK_Type() != TRIANGLE)
      failure.Set(1, e, "Native adaptation currently rejects mixed or non-triangle volume cells.");
  for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) {
    switch (config.GetMarker_All_KindBC(m)) {
      case PERIODIC_BOUNDARY:
      case ACTDISK_INLET:
      case ACTDISK_OUTLET:
      case NEARFIELD_BOUNDARY:
      case FLUID_INTERFACE:
        failure.Set(1, m, "Native adaptation rejects periodic/paired physical markers.");
        break;
      default:
        break;
    }
    if (config.GetMarker_All_TagBound(m) == "SEND_RECEIVE") continue;
    for (unsigned long e = 0; e < geometry.GetnElem_Bound(m); ++e) {
      const auto type = geometry.bound[m][e]->GetVTK_Type();
      if (type != LINE && type != VERTEX) failure.Set(1, m, "Native adaptation needs line physical faces.");
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
}

namespace {
std::map<Id, Cell> ImportNative(const CConfig& config, const CGeometry& geometry, const su2activematrix* metric,
                                ReferenceState& state) {
  World world;
  CLocalFailure failure;
  const auto simplices = OwnedSimplices(geometry);
  std::vector<std::string> tags;
  for (unsigned short m = 0; m < geometry.GetnMarker(); ++m) tags.push_back(config.GetMarker_All_TagBound(m));
  const auto names = CMeshGather::GatherMarkerNames(config, tags, geometry);
  const auto boundaryFaces = OwnedBoundaryFaces(geometry, tags, config);
  std::map<Id, Cell> input;
  {
    // Freeze owner records, including halo tensor values fetched from their true CFD point owners.
    // This directory is distributed; sparse temporary remesher IDs are introduced only after import.
    CPointDirectory points;
    std::vector<uint64_t> gids;
    std::vector<char> records;
    gids.reserve(geometry.GetnPointDomain());
    records.reserve(geometry.GetnPointDomain() * 40);
    for (unsigned long p = 0; p < geometry.GetnPointDomain(); ++p) {
      const auto gid = geometry.nodes->GetGlobalIndex(p);
      gids.push_back(gid);
      std::array<double, 5> values{
          SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 0)), SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 1)),
          metric ? SU2_TYPE::GetValue((*metric)(p, 0)) : 1., metric ? SU2_TYPE::GetValue((*metric)(p, 1)) : 0.,
          metric ? SU2_TYPE::GetValue((*metric)(p, 2)) : 1.};
      if (!std::isfinite(values[0]) || !std::isfinite(values[1]) ||
          !(NormalizedDeterminant(values[2], values[3], values[4]) > 1e-14L))
        failure.Set(1, gid, "Nonfinite coordinate or target outside the native SPD guard.");
      RecordStream encode(records);
      encode(values);
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    points.Build(gids, records, 40, "native frozen donor points");
    gids.clear();
    gids.reserve(geometry.GetnPoint());
    for (unsigned long p = 0; p < geometry.GetnPoint(); ++p) gids.push_back(geometry.nodes->GetGlobalIndex(p));
    const auto fetched = points.Fetch(gids);
    const auto prefix = CPassiveComm::ExscanSum(simplices.size());
    for (unsigned long e = 0; e < simplices.size(); ++e) {
      Cell cell;
      cell.t.id = prefix + e;
      for (int k = 0; k < 3; ++k) {
        const auto p = simplices.nodes[3 * e + k];
        const char* data = fetched.data() + 40 * p;
        const auto x = GetBytes<double>(data), y = GetBytes<double>(data);
        cell.t.v[k] = {gids[p], {x, y}, 0};
        cell.nodal_target[k] = {GetBytes<double>(data), GetBytes<double>(data), GetBytes<double>(data)};
        if (x != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 0)) ||
            y != SU2_TYPE::GetValue(geometry.nodes->GetCoord(p, 1)))
          failure.Set(1, gids[p], "Native import found inconsistent CFD halo coordinates.");
      }
      if (area(cell.t) < 0) {
        std::swap(cell.t.v[1], cell.t.v[2]);
        std::swap(cell.nodal_target[1], cell.nodal_target[2]);
      }
      input.emplace(cell.t.id, cell);
    }
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  std::vector<BoundaryLabel> labels;
  for (unsigned long f = 0; f < boundaryFaces.size(); ++f) {
    const auto name = config.GetMarker_CfgFile_TagBound(boundaryFaces.markerId[f]);
    const auto marker = std::find(names.begin(), names.end(), name) - names.begin();
    labels.push_back({geometry.nodes->GetGlobalIndex(boundaryFaces.nodes[2 * f]),
                      geometry.nodes->GetGlobalIndex(boundaryFaces.nodes[2 * f + 1]), int(marker)});
  }
  Associate(world, input, labels, names, SU2_TYPE::GetValue(config.GetAdap_Angle()), state);
  return input;
}
struct PendingBindings {
  std::map<FaceCoordinates, int> edges;
  bool published = false;
};
struct RejectionCount {
  std::array<char, 128> reason{};
  uint64_t count = 0;
  template <class S>
  void Fields(S& s) {
    s(reason, count);
  }
};
}  // namespace

void CNativeRemesher::PrepareReference(const CConfig& config, const CGeometry& geometry) {
  CheckSupport(config, geometry);
  if (reference->original) return;
  const auto filename = config.GetMesh_FileName() + ".native_ref";
  const bool loaded = LoadReference(filename, *reference);
  CLocalFailure failure;
  if (config.GetRestart() && !loaded)
    failure.Set(1, 0, "Native solution restart requires the original geometry sidecar: " + filename);
  CollectiveFailure(failure, CURRENT_FUNCTION);
  ImportNative(config, geometry, nullptr, *reference);  // Geometry preflight before solving; no target yet.
}

CRemeshResult CNativeRemesher::Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) {
  CheckSupport(config, geometry);
  World world;
  CLocalFailure failure;
  if (metric.rows() < geometry.GetnPointDomain() || metric.cols() != 3)
    failure.Set(1, 0, "Native adaptation needs three metric values at every owned point.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
  PrepareReference(config, geometry);
  auto input = ImportNative(config, geometry, &metric, *reference);
  const auto& names = reference->marker_names;
  std::map<int, double> heights;
  for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b) {
    const auto& layer = config.GetAdap_BL(b);
    const auto marker = std::find(names.begin(), names.end(), layer.marker);
    if (marker == names.end())
      failure.Set(1, b, "Native BL marker has no physical faces: " + layer.marker);
    else
      heights.emplace(marker - names.begin(), SU2_TYPE::GetValue(layer.firstHeight));
  }
  CollectiveFailure(failure, CURRENT_FUNCTION);
  const auto policy = reference->original->Policy(heights);
  for (auto& entry : input)
    for (int k = 0; k < 3; ++k)
      if (entry.second.marker[k]) {
        auto& cell = entry.second;
        const auto h = policy.height(cell.marker[k]);
        if (h > 0 &&
            std::abs(static_cast<double>(2 * area(cell.t)) / norm(cell.t.v[(k + 1) % 3].p - cell.t.v[k].p) / h - 1) <
                1e-8)
          cell.t.protected_cell = 1;
      }
  EngineOptions control;
  // Account for refinement demand as well as input size. Original frozen P1
  // tensors are averaged at donor centroids; no tensor/target is altered.
  long double localComplexity = 0;
  for (const auto& entry : input) {
    const auto& cell = entry.second;
    Tensor m{0, 0, 0};
    for (const auto& node : cell.nodal_target) {
      m.xx += node.xx / 3;
      m.xy += node.xy / 3;
      m.yy += node.yy / 3;
    }
    const auto scale = std::max({std::abs(m.xx), std::abs(m.xy), std::abs(m.yy)});
    localComplexity += area(cell.t) * scale * std::sqrt(NormalizedDeterminant(m.xx, m.xy, m.yy));
  }
  CLocalFailure workFailure;
  if (!(std::isfinite(localComplexity) && localComplexity >= 0 &&
        localComplexity <= std::numeric_limits<double>::max()))
    workFailure.Set(1, 0, "Unrepresentable native frozen-target work estimate.");
  CollectiveFailure(workFailure, CURRENT_FUNCTION);
  const auto complexity = CPassiveComm::Allreduce(static_cast<double>(localComplexity), CPassiveComm::Op::SUM);
  if (!std::isfinite(complexity)) workFailure.Set(1, 0, "Unrepresentable global native work estimate.");
  CollectiveFailure(workFailure, CURRENT_FUNCTION);
  const auto globalCells = CPassiveComm::Allreduce(uint64_t(input.size()), CPassiveComm::Op::SUM);
  // A unit-edge equilateral metric triangle has area sqrt(3)/4. This is a
  // work estimate, not a requested cell count or a guarantee of completion.
  const long double estimatedCells = 4 * static_cast<long double>(complexity) / std::sqrt(3.L);
  const auto rounds = std::ceil(std::max(static_cast<long double>(globalCells), estimatedCells) / (4 * world.size));
  control.phase_rounds = int(std::min(20000.L, std::max(300.L, rounds)));
  control.geometry_tolerance = SU2_TYPE::GetValue(config.GetAdap_Hausd());
  control.fixed_boundary = !config.GetAdap_Surface();
  if (world.rank == 0)
    std::cout << "Native contract: q>=0.18, frozen-target Simpson edge length<=1.8, reference deviation<="
              << control.geometry_tolerance
              << ", requested wall altitude relative error<=1e-8; dependency budget=" << control.dependency_bytes
              << " bytes per rank, original P1 nodal target, boundary sampling "
              << (control.fixed_boundary ? "fixed" : "adaptive") << ".\n";
  if (world.rank == 0)
    std::cout << "Native work allowance: " << control.phase_rounds << " collective rounds per phase, "
              << control.sweeps << " sweeps, for " << globalCells << " input cells on " << world.size
              << " ranks; frozen P1 centroid complexity=" << complexity << ", estimated unit triangles="
              << estimatedCells << ".\n";
  if (world.rank == 0)
    for (const auto& height : heights)
      std::cout << "Native requested first altitude: " << names[height.first] << " = " << height.second << '\n';
  Engine engine(world, std::move(input), policy, control);
  uint64_t initialShape = 0, initialSize = 0;
  for (const auto& entry : engine.owned) {
    initialShape += entry.second.target_cache[0] < .18;
    initialSize += *std::max_element(entry.second.target_cache.begin() + 1, entry.second.target_cache.end()) > 1.8;
  }
  initialShape = CPassiveComm::Allreduce(initialShape, CPassiveComm::Op::SUM);
  initialSize = CPassiveComm::Allreduce(initialSize, CPassiveComm::Op::SUM);
  if (world.rank == 0)
    std::cout << "Native incoming residuals: shape=" << initialShape << ", edge length=" << initialSize << '\n';
  engine.adapt();
  uint64_t missedQuality = 0, missedHeight = 0, missedGeometry = 0, missedShape = 0, missedSize = 0;
  double minQuality = 1, maxLength = 0;
  for (const auto& entry : engine.owned) {
    const auto& cell = entry.second;
    minQuality = std::min(minQuality, cell.target_cache[0]);
    const auto largest = *std::max_element(cell.target_cache.begin() + 1, cell.target_cache.end());
    maxLength = std::max(maxLength, largest);
    missedQuality += cell.target_cache[0] < .18 || largest > 1.8;
    missedShape += cell.target_cache[0] < .18;
    missedSize += largest > 1.8;
    for (int k = 0; k < 3; ++k)
      if (cell.marker[k]) {
        const auto a = cell.t.v[k], b = cell.t.v[(k + 1) % 3];
        const auto h = policy.height(cell.marker[k]);
        missedGeometry += policy.deviation({a, b, cell.marker[k]}) > control.geometry_tolerance;
        if (h > 0) missedHeight += std::abs(static_cast<double>(2 * area(cell.t)) / norm(b.p - a.p) / h - 1) > 1e-8;
      }
  }
  missedQuality = CPassiveComm::Allreduce(missedQuality, CPassiveComm::Op::SUM);
  missedHeight = CPassiveComm::Allreduce(missedHeight, CPassiveComm::Op::SUM);
  missedGeometry = CPassiveComm::Allreduce(missedGeometry, CPassiveComm::Op::SUM);
  missedShape = CPassiveComm::Allreduce(missedShape, CPassiveComm::Op::SUM);
  missedSize = CPassiveComm::Allreduce(missedSize, CPassiveComm::Op::SUM);
  minQuality = CPassiveComm::Allreduce(minQuality, CPassiveComm::Op::MIN);
  maxLength = CPassiveComm::Allreduce(maxLength, CPassiveComm::Op::MAX);
  CRemeshResult result;
  result.status = missedHeight || missedGeometry
                      ? CRemeshResult::Status::INCOMPLETE_COVERAGE
                      : missedQuality ? CRemeshResult::Status::INCOMPLETE_QUALITY : CRemeshResult::Status::COMPLETE;
  result.slices = ReaderSlices(engine.owned, *reference);
  result.markers = result.slices.markersWithElements;
  auto pending = std::make_shared<PendingBindings>();
  pending->edges = Bindings(world, engine.owned);
  result.onAccepted = [state = reference, pending]() {
    if (!pending->published) {
      state->accepted_edges.swap(pending->edges);
      pending->published = true;
    }
  };
  const auto commits = world.sum(engine.stats.commits), cross = world.sum(engine.stats.cross_rank);
  const auto rejectedSize = world.sum(engine.stats.size_rejected),
             rejectedMemory = world.sum(engine.stats.memory_rejected),
             rejectedStale = world.sum(engine.stats.stale_rejected), conflicts = world.sum(engine.stats.conflicts);
  const auto bytes = CPassiveComm::Allreduce(uint64_t(world.bytes_sent), CPassiveComm::Op::SUM);
  const auto transportEstimate =
      CPassiveComm::Allreduce(uint64_t(world.max_exchange_work_bytes), CPassiveComm::Op::MAX);
  std::array<uint64_t, 8> actions;
  std::array<double, 8> phaseSeconds, choiceSeconds;
  for (size_t a = 0; a < actions.size(); ++a) {
    actions[a] = CPassiveComm::Allreduce(uint64_t(engine.stats.accepted[a]), CPassiveComm::Op::SUM);
    phaseSeconds[a] = CPassiveComm::Allreduce(engine.stats.phase_seconds[a], CPassiveComm::Op::MAX);
    choiceSeconds[a] = CPassiveComm::Allreduce(engine.stats.choice_seconds[a], CPassiveComm::Op::MAX);
  }
  std::vector<RejectionCount> localReasons;
  for (const auto& entry : engine.stats.rejected) {
    RejectionCount record;
    std::copy_n(entry.first.begin(), std::min(entry.first.size(), record.reason.size() - 1), record.reason.begin());
    record.count = entry.second;
    localReasons.push_back(record);
  }
  const auto allReasons = world.metadata(localReasons);
  std::map<std::string, uint64_t> reasons;
  for (const auto& entry : allReasons) reasons[entry.reason.data()] += entry.count;
  if (world.rank == 0)
    std::cout << "Native adaptation: " << commits << " commits (" << cross << " across owners), qmin=" << minQuality
              << ", Lmax=" << maxLength << ", residual cells=" << missedQuality << ", height faces=" << missedHeight
              << ", reference faces=" << missedGeometry << ". No remesher-side target floor.\n";
  if (world.rank == 0)
    std::cout << "Native remaining residuals: shape=" << missedShape << ", edge length=" << missedSize << '\n';
  if (world.rank == 0) {
    std::cout << "Native operations (height/split/remove/redistribute/bulk remove/split/flip/move):";
    for (const auto count : actions) std::cout << ' ' << count;
    std::cout << "; conflicts=" << conflicts << ", dependency-size rejects=" << rejectedSize
              << ", memory rejects=" << rejectedMemory << ", stale rejects=" << rejectedStale
              << ", engine encoded bytes=" << bytes << ", largest transport admission estimate=" << transportEstimate
              << '\n';
    std::cout << "Native phase elapsed seconds (same action order, maximum across ranks):";
    for (const auto seconds : phaseSeconds) std::cout << ' ' << seconds;
    std::cout << '\n';
    std::cout << "Native candidate selection seconds (same action order, maximum across ranks):";
    for (const auto seconds : choiceSeconds) std::cout << ' ' << seconds;
    std::cout << '\n';
    for (const auto& entry : reasons)
      std::cout << "Native deferred/rejected candidate: " << entry.first << " (" << entry.second << ").\n";
  }
  return result;
}
