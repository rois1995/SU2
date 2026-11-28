/*!
 * \file CNativeRemesher3D.cpp
 * \brief Native tetrahedral remesh, elected failure return and per-rank phase reporting.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/adaptation/CNativeTimerTree3D.hpp"
#include "../../include/adaptation/CNativeRemesher3D.hpp"
#include "../../include/adaptation/CNativeGeometryCertificates3D.hpp"
#include "../../include/adaptation/CNativeMarkerTolerance3D.hpp"
#include "../../include/adaptation/CNativeLocalCore3D.hpp"
#include "../../include/adaptation/CNativeMetricTransport3D.hpp"
#include "../../include/adaptation/CNativeAffineReference.hpp"
#include "../../include/adaptation/CNativeRuntimeProfile3D.hpp"
#include "../../include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../include/CConfig.hpp"
#include "../../include/linear_algebra/blas_structure.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include <chrono>
#include <unistd.h>
#include <sys/resource.h>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>
#include <type_traits>
#if defined(__unix__)
#include <sys/resource.h>
#endif
using namespace SU2Native3D;
CNativeRemesher3D::CNativeRemesher3D(std::shared_ptr<ReferenceState> retained, EngineOptions control, CompositionFactory constraints,NormalizationFactory normalizer)
    : reference(std::move(retained)), options(control), compositionFactory(std::move(constraints)), normalizationFactory(std::move(normalizer)) {
  if (!reference) throw std::invalid_argument("Native3D needs driver-owned reference storage.");
}
namespace SU2Native3D {
// DC-16: FIRST_ROW gathers and multiplies the whole mesh on one worker; its ceiling (reference setup, construction,
// publication) is max(default, 1.5 x the event's need), capped at 25 % of the node memory (shared with other jobs).
// Need before the construction: max(measured peak RSS of this process, 4 KiB per input cell; measured: the import of
// the published 1.25M-cell ONERA M6 x32 mesh requests 3.9 GB, 3.1 KiB/cell); after it, the publication rule in
// Remesh. Other methods: the default.
// ADAP_NATIVE_MEMORY_LIMIT overrides this legacy automatic policy with one fixed per-rank limit.
// Review 3/9: one value on every rank (MAX of the need, MIN of the cap over the communicator), saturating arithmetic;
// the cap never lowers the default (a node with less than 4 x 2 GiB keeps the default).
size_t NativeMemoryCeiling(const CConfig& config,const CGeometry& geometry,size_t base,bool print,SU2_Comm comm) {
  const auto requested=uint64_t(config.GetAdap_Native_MemoryLimit());
  const auto low=CPassiveComm::Allreduce(requested,CPassiveComm::Op::MIN,comm),
             high=CPassiveComm::Allreduce(requested,CPassiveComm::Op::MAX,comm);
  if(low!=high)throw std::invalid_argument("Native3D configured memory limits differ across ranks.");
  if(const auto explicitLimit=config.GetAdap_Native_MemoryLimit()) {
    if(print)std::cout<<"Native3D configured working-storage ceiling: "<<(explicitLimit>>20)<<" MiB per rank"<<std::endl;
    return explicitLimit;
  }
  if (config.GetKind_Adap_BL_Method()!=ADAP_BL_METHOD::FIRST_ROW) return base;
  const long pages=sysconf(_SC_PHYS_PAGES),page=sysconf(_SC_PAGESIZE);
  const uint64_t local_cap=pages>0&&page>0?uint64_t(pages)*uint64_t(page)/4:uint64_t(base);
  struct rusage usage{};getrusage(RUSAGE_SELF,&usage);
  const uint64_t local_need=std::max<uint64_t>(uint64_t(usage.ru_maxrss)*1024,transfer_memory::Mul(4096,size_t(geometry.GetGlobal_nElem())));
  const size_t cap=size_t(CPassiveComm::Allreduce(local_cap,CPassiveComm::Op::MIN,comm));
  const size_t need=size_t(CPassiveComm::Allreduce(local_need,CPassiveComm::Op::MAX,comm));
  const size_t scaled=need>=SIZE_MAX/2?SIZE_MAX:need+need/2;
  const size_t ceiling=std::min(std::max(base,scaled),std::max(base,cap));
  if (print) std::cout<<"DC16 FIRST_ROW memory: need "<<(need>>20)<<" MiB, ceiling "<<(ceiling>>20)<<" MiB (default "<<(base>>20)<<" MiB, cap 25 % of node memory "<<(cap>>20)<<" MiB)"<<std::endl;
  return ceiling;
}
}  // namespace SU2Native3D
namespace {
void PrepareMarkerTolerance(const CConfig& config, const ReferenceState& state, size_t ceiling, size_t caller,
                            std::optional<MarkerTolerance3D>& table) {
  const auto& configured = config.GetAdap_MarkerHausd();
  if (configured.empty() || table) return;
  if (!state.original || !state.original->Mapped() || state.original->Parameters()->Geometry().Curves().empty())
    throw std::invalid_argument("ADAP_HAUSD per marker needs a mapped curved native reference.");
  const auto bytes = MarkerTolerance3D::ConstructionBytes(state.marker_names, configured.size());
  if (transfer_memory::Add(caller, state.original->ResidentBytes(), bytes) > ceiling)
    throw std::length_error("Native marker tolerance table exceeds working-storage admission.");
  double maximum = 0;
  const auto include = [&](Point p) { maximum = std::max({maximum, std::abs(p.x), std::abs(p.y), std::abs(p.z)}); };
  for (const auto& item : state.original->Facets())
    for (const auto& node : item.second.v) include(node.p);
  for (const auto& item : state.original->Parameters()->Geometry().Curves())
    for (const auto& point : item.second) include(point);
  table.emplace(configured, state.marker_names, maximum);
}

// Configuration prepares mobility explicitly; loading an old sidecar never changes its semantics.
void PrepareMobility(const CConfig& config,ReferenceState& state,size_t ceiling,size_t caller,
                     const MarkerTolerance3D* tolerances = nullptr) {
  SU2NativeTiming3D::Scope mobility_timer(SU2NativeTiming3D::Operation::MobilityPolicy);
  if(!state.original || !state.original->Mapped())return;
  const auto* accepted=state.original->Parameters();const auto& authority=accepted->Geometry();
  if(authority.Curves().empty())return;
  std::map<int,int> roles;
  for (size_t m = 0; m < state.marker_names.size(); ++m) {
    const int kind = config.GetMarker_CfgFile_KindBC(state.marker_names[m]) == SYMMETRY_PLANE ? 0 : 2;
    // Preserve the role kind in the low two bits for scalar and equal-list policies.
    roles.emplace(int(m), kind + (tolerances ? 4 * tolerances->Class(int(m)) : 0));
  }
  const double angle=SU2_TYPE::GetValue(config.GetAdap_Angle());
  if(accepted->Mobility().identity) {
    if(accepted->Mobility().angle!=angle || accepted->Mobility().roles!=roles)
      throw std::invalid_argument("Persisted curved mobility policy differs from ADAP_ANGLE/marker roles.");
    if(accepted->EntityGeometry()&&accepted->GeometryTolerance()==SU2_TYPE::GetValue(config.GetAdap_Hausd()))return;
  }
  const auto baseline=transfer_memory::Add(caller,state.original->ResidentBytes());
  if(baseline==SIZE_MAX || baseline>=ceiling)throw std::length_error("Curved mobility setup exceeds storage admission.");
  SU2NativeGeometry::AtlasBudget budget;
  budget.maximum_bytes=ceiling;budget.caller_bytes=baseline;
  budget.maximum_integer_bits=transfer_memory::Mul(ceiling-baseline,8);
  budget.maximum_work=transfer_memory::Mul(transfer_memory::Add(authority.Reference().Facets().size(),accepted->Surface().size()),
      SU2NativeGeometry::MappedFacetReference::ClearanceWork);
  if(budget.maximum_work==SIZE_MAX || budget.maximum_integer_bits==SIZE_MAX)
    throw std::length_error("Curved mobility setup reservation overflows.");
  auto mobility=accepted->Mobility().identity?std::shared_ptr<BoundaryParameterState>{}:
      accepted->WithMobilityPolicy(angle,roles,budget,ceiling,baseline);
  const auto* active=mobility?mobility.get():accepted;
  auto prepared=active->WithEntityGeometry(SU2_TYPE::GetValue(config.GetAdap_Hausd()),budget,ceiling,
      transfer_memory::Add(baseline,mobility?mobility->ResidentBytes():0));
  if(!SU2_MPI::GetRank()) {
    const auto& policy=prepared->Mobility();std::array<size_t,3> dimensions{};
    for(const auto& row:accepted->Records())++dimensions.at(authority.MobilityDimension(row.second.point.binding,policy));
    std::cout<<"Native3D curved mobility: protected_edges="<<policy.features.size()<<" corners="<<policy.corners.size()
        <<" fixed_points="<<dimensions[0]<<" curve_points="<<dimensions[1]<<" surface_points="<<dimensions[2]
        <<" setup_work="<<budget.work<<std::endl;
  }
  state.original=state.original->WithParameterState(std::move(prepared));
}
}  // namespace
void CNativeRemesher3D::PrepareReference(const CConfig& config,const CGeometry& geometry) {
  SU2NativeTiming3D::Session timer_tree(config.GetWrt_Performance() || options.timings, "reference", SU2_MPI::GetRank(),
                                        SU2_MPI::GetSize(), SU2NativeTiming3D::Phase::Preparation);
  SU2NativeTiming3D::Scope reference_timer(SU2NativeTiming3D::Operation::Import);
  if(reference->original) {
    CLocalFailure failure;auto pending=*reference;
    const auto ceiling=NativeMemoryCeiling(config,geometry,options.memory_ceiling,false,SU2_MPI::GetComm());
    try {
      pending.first_height_tol=SU2NativeFirstHeight::AdoptRecorded(pending.first_height_tol,
          SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol()));
      std::optional<MarkerTolerance3D> table;
      PrepareMarkerTolerance(config, pending, ceiling, options.caller_bytes, table);
      PrepareMobility(config, pending, ceiling, transfer_memory::Add(options.caller_bytes, table ? table->Bytes() : 0),
                      table ? &*table : nullptr);
    }
    catch(const std::exception& e){failure.Set(2,0,e.what());}
    CollectiveFailure(failure,CURRENT_FUNCTION);*reference=std::move(pending);return;
  }
  auto pending=*reference;
  bool loaded=false;std::string reason;size_t maximum=0;
  const auto held=transfer_memory::Add(options.caller_bytes,ReferenceStorageBytes(*reference));
  const auto ceiling=NativeMemoryCeiling(config,geometry,options.memory_ceiling,SU2_MPI::GetRank()==0,SU2_MPI::GetComm());
  CLocalFailure failure;
  if(!SU2Native3D::LoadReference(config.GetMesh_FileName()+".native_ref",pending,loaded,reason,
                               ceiling,held,maximum)) failure.Set(2,0,reason);
  CollectiveFailure(failure,CURRENT_FUNCTION);
  if(config.GetRestart() && !loaded)
    failure.Set(1,0,"Native3D restart requires its original .native_ref sidecar; geometry cannot be rebased.");
  // The persisted first-height policy must match ADAP_BL_FIRST_HEIGHT_TOL; legacy sidecars get it recorded now.
  try{pending.first_height_tol=SU2NativeFirstHeight::AdoptRecorded(pending.first_height_tol,
      SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol()));}
  catch(const std::exception& e){failure.Set(1,0,e.what());}
  CollectiveFailure(failure,CURRENT_FUNCTION);
  SU2Native3D::ImportedGeometry imported;SU2Native3D::BridgeStats importedStats;
  if(!SU2Native3D::ImportGeometry(config,geometry,nullptr,pending,imported,reason,importedStats,
                                ceiling,held)) failure.Set(2,0,reason);
  CollectiveFailure(failure,CURRENT_FUNCTION);
  try {
    std::optional<MarkerTolerance3D> table;
    PrepareMarkerTolerance(config, pending, ceiling, held, table);
    PrepareMobility(config, pending, ceiling, transfer_memory::Add(held, table ? table->Bytes() : 0),
                    table ? &*table : nullptr);
  }
  catch(const std::exception& e){failure.Set(2,0,e.what());}
  CollectiveFailure(failure,CURRENT_FUNCTION);
  *reference=std::move(pending);
}

void CNativeRemesher3D::CheckSupport(const CConfig& config, const CGeometry& geometry) {
  CLocalFailure failure;
  if (!std::is_same<su2double, double>::value || !std::is_same<passivedouble, double>::value)
    failure.Set(1, 0, "Native adaptation currently requires a primal double-precision build.");
  if (config.GetContinuous_Adjoint() || config.GetDiscrete_Adjoint())
    failure.Set(1, 0, "Native adaptation does not support continuous or discrete adjoint configurations.");
  if (geometry.GetnDim() != 3) failure.Set(1, 0, "Native tetrahedral adaptation requires 3D geometry.");
  if (config.GetMultizone_Problem() || config.GetnZone() > 1 || config.GetnMarker_ZoneInterface() > 0)
    failure.Set(1, 0, "Native adaptation does not support multizone or sliding meshes.");
  if (config.GetnMarker_Periodic() > 0) failure.Set(1, 0, "Native adaptation does not support periodic boundaries.");
  if (config.GetGrid_Movement() || config.GetDynamic_Grid() || config.GetDeform_Mesh())
    failure.Set(1, 0, "Native adaptation does not support moving/deforming meshes.");
  if (config.GetFEMSolver() || !config.GetMarkerCreateCopy().empty())
    failure.Set(1, 0, "Native adaptation does not support FEM or MARKER_CREATE_COPY.");
  if (config.GetTime_Domain()) {
    const auto marching = config.GetTime_Marching();
    if (marching != TIME_MARCHING::DT_STEPPING_1ST && marching != TIME_MARCHING::DT_STEPPING_2ND)
      failure.Set(1, 0, "Native time-domain adaptation requires first- or second-order dual time stepping.");
    const auto method = config.GetKind_Adap_Unsteady_Metric();
    if (method != ADAP_UNSTEADY_METRIC::WINDOW_AVERAGE && method != ADAP_UNSTEADY_METRIC::PREDICT)
      failure.Set(
          1, 0, "Native time-domain adaptation supports WINDOW_AVERAGE and PREDICT; FIXED_POINT remains unsupported.");
  }
  for (unsigned long e = 0; e < geometry.GetnElem(); ++e)
    if (geometry.elem[e]->GetVTK_Type() != TETRAHEDRON)
      failure.Set(1, e, "Native adaptation currently rejects mixed or non-tetrahedral volume cells.");
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
      if (type != TRIANGLE && type != VERTEX) failure.Set(1, m, "Native adaptation needs triangular physical faces.");
    }
  }
  for (unsigned short b = 0; b < config.GetnAdap_BL(); ++b)
    for (unsigned short m = 0; m < geometry.GetnMarker(); ++m)
      if (config.GetMarker_All_TagBound(m) == config.GetAdap_BL(b).marker && !config.GetSolid_Wall(m))
        failure.Set(1, b, "ADAP_BL_MARKER: " + config.GetAdap_BL(b).marker + " is not a wall marker.");
  if (!config.GetAdap_Surface())
    failure.Set(1, 0, "Native3D currently requires ADAP_SURFACE= YES; fixed surfaces are not validated.");
  if (SU2_TYPE::GetValue(config.GetAdap_Hessian_Noise()) != 0)
    failure.Set(1, 0, "Native3D validation requires ADAP_HESSIAN_NOISE= 0.");
  CollectiveFailure(failure, CURRENT_FUNCTION);
}

#include "native_engine3d/NativeRemeshReporting.inc"
#include "native_engine3d/NativeRemeshRun.inc"

CRemeshResult CNativeRemesher3D::Remesh(const CConfig& config, const CGeometry& geometry,
                                        const su2activematrix& sensor) {
  World parent;
  CRemeshResult result;
  result.status = CRemeshResult::Status::INCOMPLETE_QUALITY;
  stats = {};
  stats.workers = config.GetAdap_Native_Ranks() ? int(config.GetAdap_Native_Ranks()) : parent.size;
  auto control = options;
  control.timings = options.timings || config.GetWrt_Performance();
  using Probe = SU2NativeLocal3D::CollapseProbe;
  namespace cert = geometry_certificates;
  cert::ScopedValue<const MarkerTolerance3D*> tolerance_scope(active_marker_tolerance, nullptr);
  const bool surface_mode = config.GetAdap_Native_Local_Core() && config.GetAdap_Native_Local_Surface() &&
                            !config.GetnAdap_BL() && !construction;
  const auto probe = surface_mode ? SU2NativeLocal3D::FP3Probe() : Probe::None;
  std::optional<cert::Event> certificates;
  std::optional<SU2NativeTiming3D::UnattributedSpans> spans;
  if (surface_mode && (probe == Probe::None || probe == Probe::SC1Count)) {
    certificates.emplace(probe == Probe::SC1Count ? 6 : 0, parent.rank, control.timings);
    if (geometryCertificates && geometryCertificates->accepted && reference->original && reference->original->Parameters() &&
        geometryCertificates->accepted->authority.get() == &reference->original->Parameters()->Geometry())
      certificates->old = std::move(geometryCertificates->accepted);
    if (probe == Probe::SC1Count && control.timings) {
      spans.emplace();
      certificates->observer_bytes = sizeof(*spans);
    }
  }
  if (geometryCertificates) geometryCertificates->accepted.reset();
  SU2NativeTiming3D::Session timer_tree(control.timings, "remesh", parent.rank, parent.size,
                                        SU2NativeTiming3D::Phase::Preparation);
  if (timer_tree.Owns()) control.caller_bytes = transfer_memory::Add(control.caller_bytes, timer_tree.ReservedBytes());
  control.surface_tolerance = SU2_TYPE::GetValue(config.GetAdap_Hausd());
  control.job_memory_limit = config.GetAdap_Native_Job_MemoryLimit();
  if (control.timings) {
    control.requirement_size = {SU2_TYPE::GetValue(config.GetAdap_Hmin()), SU2_TYPE::GetValue(config.GetAdap_Hmax()),
                                SU2_TYPE::GetValue(config.GetAdap_ARmax()), SU2_TYPE::GetValue(config.GetAdap_Hgrad()),
                                SU2_TYPE::GetValue(config.GetAdap_Complexity())};
  }
  control.wall_heights.tolerance = SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol());  // Agreed by the engine.
  if (config.GetAdap_Native_Max_Work()) control.maximum_work = config.GetAdap_Native_Max_Work();
  if (config.GetAdap_Native_Max_Time() > 0)
    control.maximum_seconds = SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time());
  control.memory_ceiling = NativeMemoryCeiling(config, geometry, control.memory_ceiling, parent.rank == 0, parent.comm);
  stats.detailed_timings = control.timings;
  const auto start = parent.seconds();
  const auto cpu = std::clock();
  auto now = start;
  const auto deadline_start = std::chrono::steady_clock::now();
  RemeshRun run{config,
                geometry,
                sensor,
                reference,
                options,
                compositionFactory,
                normalizationFactory,
                construction,
                constructionAccepted,
                stats,
                parent,
                result,
                control,
                start,
                cpu,
                now,
                deadline_start,
                CURRENT_FUNCTION,
                geometryCertificates,
                certificates ? &*certificates : nullptr,
                surface_mode,
                int(probe)};
  run.Run();
  if (certificates) {
    certificates->Report(stats.workers, stats.normalization_stages ? stats.normalization_stages - 1 : 0,
                         certificates->eligible_stages);
    if (spans) {
      timer_tree.Finish();
      spans->Report(timer_tree.Snapshot(), parent.rank, certificates->counts.skip_seconds);
    }
  }
  return result;
}
