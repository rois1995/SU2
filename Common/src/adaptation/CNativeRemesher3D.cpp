/*!
 * \file CNativeRemesher3D.cpp
 * \brief Native tetrahedral remesh, elected failure return and per-rank phase reporting.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/adaptation/CNativeRemesher3D.hpp"
#include "../../include/adaptation/CNativeRuntimeProfile3D.hpp"
#include "../../include/adaptation/CNativeReferenceIO3D.hpp"
#include "../../include/CConfig.hpp"
#include "../../include/linear_algebra/blas_structure.hpp"
#include "../../include/geometry/CGeometry.hpp"
#include <chrono>
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
CNativeRemesher3D::CNativeRemesher3D(std::shared_ptr<ReferenceState> retained, EngineOptions control, CompositionFactory constraints)
    : reference(std::move(retained)), options(control), compositionFactory(std::move(constraints)) {
  if (!reference) throw std::invalid_argument("Native3D needs driver-owned reference storage.");
}
void CNativeRemesher3D::PrepareReference(const CConfig& config,const CGeometry& geometry) {
  if(reference->original) return;
  auto pending=*reference;
  bool loaded=false;std::string reason;size_t maximum=0;
  const auto held=transfer_memory::Add(options.caller_bytes,ReferenceStorageBytes(*reference));
  CLocalFailure failure;
  if(!SU2Native3D::LoadReference(config.GetMesh_FileName()+".native_ref",pending,loaded,reason,
                               options.memory_ceiling,held,maximum)) failure.Set(2,0,reason);
  CollectiveFailure(failure,CURRENT_FUNCTION);
  if(config.GetRestart() && !loaded)
    failure.Set(1,0,"Native3D restart requires its original .native_ref sidecar; geometry cannot be rebased.");
  CollectiveFailure(failure,CURRENT_FUNCTION);
  SU2Native3D::ImportedGeometry imported;SU2Native3D::BridgeStats importedStats;
  if(!SU2Native3D::ImportGeometry(config,geometry,nullptr,pending,imported,reason,importedStats,
                                options.memory_ceiling,held)) failure.Set(2,0,reason);
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

CRemeshResult CNativeRemesher3D::Remesh(const CConfig& config, const CGeometry& geometry,
                                        const su2activematrix& sensor) {
  World parent;
  CRemeshResult result;
  result.status = CRemeshResult::Status::INCOMPLETE_QUALITY;
  stats = {};
  stats.workers = config.GetAdap_Native_Ranks() ? int(config.GetAdap_Native_Ranks()) : parent.size;
  auto control = options;
  control.timings = options.timings || config.GetWrt_Performance();
  control.surface_tolerance = SU2_TYPE::GetValue(config.GetAdap_Hausd());
  if (config.GetAdap_Native_Max_Work()) control.maximum_work=config.GetAdap_Native_Max_Work();
  if (config.GetAdap_Native_Max_Time()>0) control.maximum_seconds=SU2_TYPE::GetValue(config.GetAdap_Native_Max_Time());
  stats.detailed_timings = control.timings;
  const auto start = parent.seconds();
  const auto cpu = std::clock();
  auto now = start;
  const auto deadline_start=std::chrono::steady_clock::now();
  auto remaining_time=[&]() {
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-deadline_start).count();
    return CPassiveComm::Allreduce(control.maximum_seconds-elapsed,CPassiveComm::Op::MIN,parent.comm);
  };
  std::string reason;
  auto phase = [&](size_t i) {
    const auto end = parent.seconds();
    stats.phases[i] += end - now;
    now = end;
  };
  auto vote = [&](bool valid, const std::string& message) {
    CLocalFailure local;
    if (!valid) local.Set(2, 0, message);
    const auto elected = parent.elect(local);
    reason = elected.message;
    return !elected.any;
  };
  auto admit = [&](size_t bytes) {
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, bytes);
    return vote(bytes != SIZE_MAX && bytes <= control.memory_ceiling,
                "Native3D runtime retained/working storage exceeds budget.");
  };
  auto partition_bytes = [&]() {
    return transfer_memory::Add(
        transfer_memory::Bytes(stats.partition.destinations), transfer_memory::Bytes(stats.partition.cells_before),
        transfer_memory::Bytes(stats.partition.cells_after), transfer_memory::Bytes(stats.partition.work_before),
        transfer_memory::Bytes(stats.partition.work_after));
  };
  struct Publication {
    ReferenceState state;
    bool published = false;
  };
  do {
    CheckSupport(config, geometry);
    phase(0);
    const int worker_low = CPassiveComm::Allreduce(stats.workers, CPassiveComm::Op::MIN, parent.comm),
              worker_high = CPassiveComm::Allreduce(stats.workers, CPassiveComm::Op::MAX, parent.comm);
    const int low = CPassiveComm::Allreduce(int(control.timings), CPassiveComm::Op::MIN, parent.comm),
              high = CPassiveComm::Allreduce(int(control.timings), CPassiveComm::Op::MAX, parent.comm);
    CLocalFailure failure;
    if (low != high || worker_low != worker_high || stats.workers < 1 || stats.workers > parent.size)
      failure.Set(1, 0, "Native3D profile/worker configuration differs or is invalid.");
    const auto work_lo=CPassiveComm::Allreduce(control.maximum_work,CPassiveComm::Op::MIN,parent.comm);
    const auto work_hi=CPassiveComm::Allreduce(control.maximum_work,CPassiveComm::Op::MAX,parent.comm);
    const auto time_lo=CPassiveComm::Allreduce(control.maximum_seconds,CPassiveComm::Op::MIN,parent.comm);
    const auto time_hi=CPassiveComm::Allreduce(control.maximum_seconds,CPassiveComm::Op::MAX,parent.comm);
    if (work_lo!=work_hi || time_lo!=time_hi || !std::isfinite(time_lo) || !std::isfinite(time_hi) || time_lo<0)
      failure.Set(1,0,"Native3D work/time controls differ or are invalid.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    const int bl_low=CPassiveComm::Allreduce(int(config.GetnAdap_BL()),CPassiveComm::Op::MIN,parent.comm),
              bl_high=CPassiveComm::Allreduce(int(config.GetnAdap_BL()),CPassiveComm::Op::MAX,parent.comm);
    if(!vote(bl_low==bl_high,"Native3D geometric policy differs across CFD ranks."))break;
    // Admit retained/publication state first; sidecar loading admits its actual byte and construction demand.
    const size_t reference_setup =
        transfer_memory::Add(options.caller_bytes, ReferenceStorageBytes(*reference),
                             ReferenceStorageBytes(*reference, false));
    if (!admit(reference_setup)) {
      phase(1);
      break;
    }
    auto pending = std::make_shared<Publication>();
    pending->state = *reference;
    if (!pending->state.original) {
      bool loaded = false;
      if (!LoadReference(config.GetMesh_FileName() + ".native_ref", pending->state, loaded, reason,
                         control.memory_ceiling, reference_setup, stats.maximum_native_bytes)) {
        phase(1);
        break;
      }
      if (config.GetRestart() && !loaded)
        failure.Set(1, 0, "Native3D restart requires its original .native_ref sidecar; geometry cannot be rebased.");
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    phase(1);
    // Retained and pending state share the immutable authority: count its allocation once.
    size_t retained = transfer_memory::Add(options.caller_bytes, ReferenceStorageBytes(*reference, false));
    auto held = [&]() {
      return transfer_memory::Add(retained, ReferenceStorageBytes(pending->state), partition_bytes());
    };
    ImportedGeometry imported;
    bool ready = ImportGeometry(config, geometry, &sensor, pending->state, imported, reason, stats.imported,
                                control.memory_ceiling, retained);
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, stats.imported.maximum_native_bytes);
    phase(2);
    if (!ready) break;
    MetricComposition composition;
    if (config.GetnAdap_BL()) {
      if (!vote(bool(compositionFactory),"Native3D configured BL requires an actual-query geometric policy.")) break;
      // shortcut: conservative wall-policy construction scratch; replace with measured peak admission for large surfaces.
      if (!admit(transfer_memory::Add(held(),OwnedStorageBytes(imported.cells),transfer_memory::Bytes(imported.original),
                                     transfer_memory::Mul(pending->state.facets.size(),8192)))) break;
      double localCore=std::numeric_limits<double>::max();
      try {
        for(unsigned long p=0;p<geometry.GetnPointDomain();++p) {
          su2double m[3][3]={{sensor(p,0),sensor(p,1),sensor(p,2)},
                            {sensor(p,1),sensor(p,3),sensor(p,4)},
                            {sensor(p,2),sensor(p,4),sensor(p,5)}},vec[3][3],val[3],work[3];
          CBlasStructure::EigenDecomposition(m,vec,val,3,work);
          const double smallest=SU2_TYPE::GetValue(*std::min_element(val,val+3));
          if (!(std::isfinite(smallest) && smallest>0)) throw std::runtime_error("Invalid Native3D frozen sensor core.");
          localCore=std::min(localCore,smallest);
        }
      }catch(const std::exception& e){failure.Set(2,0,e.what());}
      CollectiveFailure(failure,CURRENT_FUNCTION);
      const double core=CPassiveComm::Allreduce(localCore,CPassiveComm::Op::MIN,parent.comm);
      try {composition=compositionFactory(config,pending->state,core,control);}
      catch(const std::exception& e){failure.Set(2,0,e.what());}
      CollectiveFailure(failure,CURRENT_FUNCTION);
      if (!vote(bool(composition) && bool(control.edge_breakpoints),"Native3D BL policy omitted composition or geometric edge bands.")) break;
      if(!vote(control.caller_bytes>=options.caller_bytes,"Native3D geometric policy decreased retained storage."))break;
      retained=transfer_memory::Add(retained,control.caller_bytes-options.caller_bytes);
      if (!admit(transfer_memory::Add(held(),OwnedStorageBytes(imported.cells),transfer_memory::Bytes(imported.original)))) break;
    }
    phase(12);
    stats.initial_cells = CPassiveComm::Allreduce(uint64_t(imported.cells.size()), CPassiveComm::Op::SUM, parent.comm);
    if (control.maximum_work==SIZE_MAX)
      control.maximum_work=std::max<size_t>(4096,transfer_memory::Mul(stats.initial_cells,64));
    if (!vote(control.maximum_work!=SIZE_MAX,"Native3D automatic global work ceiling overflows.")) break;
    Owned working = std::move(imported.cells);  // Private import already owns this map; no duplicate is needed.
    std::vector<DonorCell> migrated;
    const std::vector<DonorCell>* worker_source = &imported.original;
    {
      DonorImport source(parent, control.timings);
      const bool repartition = config.GetAdap_Native_Repartition() || stats.workers < parent.size;
      const int partition_low = CPassiveComm::Allreduce(int(repartition), CPassiveComm::Op::MIN, parent.comm),
                partition_high = CPassiveComm::Allreduce(int(repartition), CPassiveComm::Op::MAX, parent.comm);
      if (partition_low != partition_high) ready = vote(false, "Inconsistent native3D repartition controls.");
      if (ready && repartition)
        ready = source.Build(imported.original, reason, control.memory_ceiling,
                             transfer_memory::Add(held(), OwnedStorageBytes(working)));
      phase(3);
      std::vector<uint32_t> weights;
      if (ready && repartition)
        ready =
            WorkingWeights(parent, working, source, weights, reason, stats.weights, composition, control.memory_ceiling,
                           transfer_memory::Add(held(), transfer_memory::Bytes(imported.original)), control.timings);
      phase(4);
      if (ready && repartition)
        ready = Repartition(
            parent, working, weights, stats.workers, stats.partition, reason, control.memory_ceiling,
            transfer_memory::Add(held(), transfer_memory::Bytes(imported.original), source.ResidentBytes()));
      phase(5);
      if (ready && repartition) {
        const size_t baseline =
            transfer_memory::Add(held(), transfer_memory::Bytes(imported.original), OwnedStorageBytes(working),
                                 source.ResidentBytes(), transfer_memory::Bytes(weights));
        if (admit(transfer_memory::Add(baseline,
                                       transfer_memory::GrowthBound(imported.original.size(), sizeof(DonorCell)),
                                       transfer_memory::Mul(stats.partition.destinations.size(),sizeof(std::pair<const Id,int>)+4*sizeof(void*)),
                                       transfer_memory::Mul(parent.size, sizeof(std::vector<DonorCell>))))) {
          std::vector<std::vector<DonorCell>> send(parent.size);
          try {
            const bool coverage=std::any_of(imported.original.begin(),imported.original.end(),[](const DonorCell& d) {
              return d.extension_component!=UINT64_MAX;
            });
            if(!coverage) {
              for(size_t j=0;j<imported.original.size();++j)
                send.at(stats.partition.destinations.at(j)).push_back(imported.original[j]);
            } else {
              std::map<Id,int> destinations;size_t j=0;
              for(const auto& d:imported.original)if(d.extension_component==UINT64_MAX)
                destinations.emplace(d.cell.id,stats.partition.destinations.at(j++));
              for(const auto& d:imported.original)
                send.at(destinations.at(d.extension_component==UINT64_MAX ? d.cell.id : d.parent_cell)).push_back(d);
            }
          }catch(const std::exception& e){ready=false;reason=e.what();}
          ready=vote(ready,reason);
          // Empty original owners still enter this same migration collective after common preparation approval.
          if(ready) {
            bool admitted = false;
            migrated = parent.exchange(send, &admitted, control.memory_ceiling, baseline);
            ready = admitted;
            if (!ready) reason = "Native3D original donor migration exceeds budget.";
            worker_source = &migrated;
          }
        } else
          ready = false;
      }
      phase(6);
      stats.maximum_native_bytes = std::max({stats.maximum_native_bytes, source.Statistics().maximum_work_bytes,
                                             stats.weights.maximum_native_bytes, stats.partition.maximum_native_bytes,
                                             parent.max_exchange_work_bytes});
      stats.source = source.Statistics();
    }
    // Index/weights teardown belongs to source preparation, not communicator construction.
    phase(3);
    if (ready) {
      auto comm = parent.comm;
#ifdef HAVE_MPI
      MPI_Comm_split(parent.comm, parent.rank < stats.workers ? 0 : MPI_UNDEFINED, parent.rank, &comm);
#endif
      phase(7);
      double worker_seconds=0;
      if (control.maximum_seconds) {
        worker_seconds=remaining_time();
        ready=vote(worker_seconds>0,"Native3D elapsed deadline exhausted before worker adaptation.");
      }
      bool valid = ready;
      std::string detail=ready ? std::string{} : reason;
      if (parent.rank < stats.workers && ready) {
        World workers(comm);
        auto kernel = control;
        kernel.maximum_seconds=worker_seconds;
        const auto worker_start=workers.seconds();
        // The engine charges its input map/source; caller_bytes covers only other live runtime buffers.
        kernel.caller_bytes = transfer_memory::Add(
            held(), worker_source == &imported.original ? 0 : transfer_memory::Bytes(imported.original));
        try {
          const auto begin = workers.seconds();
          auto refined = Refine(workers, working, *worker_source, *pending->state.original, composition, kernel);
          stats.refine = refined.stats;
          const bool rebuilt_domain=refined.rebuilt_domain;
          valid = refined.valid;
          detail = refined.reason;
          working.swap(refined.cells);
          Owned().swap(refined.cells);
          std::vector<SplitRecord>().swap(refined.splits);
          std::vector<Edge>().swap(refined.collapses);
          stats.phases[8] = workers.seconds() - begin;
          if (valid) {
            kernel.maximum_work-=refined.stats.global_work;
            if (control.maximum_seconds) {
              kernel.maximum_seconds=CPassiveComm::Allreduce(worker_seconds-(workers.seconds()-worker_start),CPassiveComm::Op::MIN,workers.comm);
              if (!(kernel.maximum_seconds>0)) {
                valid=false; detail="Native3D elapsed deadline exhausted between refinement and coarsening.";
              }
            }
          }
          // A complete domain reconstruction already resampled surface and volume in both metric directions.
          if (valid && !rebuilt_domain && kernel.maximum_work) {
            const auto begin_coarse = workers.seconds();
            auto coarse = Coarsen(workers, working, *worker_source, *pending->state.original, composition, kernel);
            stats.coarsen = coarse.stats;
            valid = coarse.valid;
            detail = coarse.reason;
            working.swap(coarse.cells);
            Owned().swap(coarse.cells);
            std::vector<SplitRecord>().swap(coarse.splits);
            std::vector<Edge>().swap(coarse.collapses);
            stats.phases[9] = workers.seconds() - begin_coarse;
          }
        } catch (const CElectedFailure& e) {
          valid = false;
          detail = e.message;
        } catch (const std::exception& e) {
          SU2_MPI::Error(e.what(), CURRENT_FUNCTION);
        }
        stats.worker_transport_bytes = workers.bytes_sent;
        stats.worker_transport_calls = workers.collective_calls;
        stats.worker_transport_seconds = workers.collective_seconds;
      }
      now = parent.seconds();
      const auto idle_cpu = std::clock();
#ifdef HAVE_MPI
#if MPI_VERSION >= 3
      MPI_Request request;
      MPI_Ibarrier(parent.comm, &request);
      int complete = 0;
      while (!complete) {
        MPI_Test(&request, &complete, MPI_STATUS_IGNORE);
        if (!complete) std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
#else
      MPI_Barrier(parent.comm);
#endif
#endif
      phase(10);
      if (parent.rank >= stats.workers) stats.idle_cpu_seconds = double(std::clock() - idle_cpu) / CLOCKS_PER_SEC;
      now = parent.seconds();
#ifdef HAVE_MPI
      if (comm != MPI_COMM_NULL) MPI_Comm_free(&comm);
#endif
      phase(7);
      ready = vote(valid, detail);
    }
    now = parent.seconds();
    // Transport only the original P1 sensor; geometric BL is recomposed at each subsequent query.
    if (ready)
      ready = ReaderSlices(parent, working, imported.original, pending->state, result.slices, reason, stats.returned,
                           {}, control.memory_ceiling, transfer_memory::Add(held(), transfer_memory::Bytes(migrated)),
                           control.timings);
    phase(11);
    stats.maximum_native_bytes =
        std::max({stats.maximum_native_bytes, stats.refine.maximum_work_bytes, stats.coarsen.maximum_work_bytes,
                  stats.returned.maximum_native_bytes, parent.max_exchange_work_bytes});
    if (ready && control.maximum_seconds)
      ready=vote(remaining_time()>0,"Native3D elapsed deadline exhausted after return-to-CFD packing.");
    if (ready) {
      result.markers = result.slices.markersWithElements;
      if (control.maximum_seconds) {
        const auto duration=std::chrono::duration<double>(control.maximum_seconds);
        result.beforeAcceptance=[deadline_start,duration]() {
          return std::chrono::steady_clock::now()-deadline_start<duration;
        };
      }
      result.status = CRemeshResult::Status::COMPLETE;
      result.onAccepted = [state = reference, pending]() {
        if (!pending->published) {
          std::swap(*state, pending->state);
          pending->published = true;
        }
      };
      stats.final_cells = result.slices.nElemGlobal;
      stats.complete = true;
    }
  } while (false);
  // Capture attempted import/parent/worker work even when an earlier phase rejects the event.
  stats.parent_transport_bytes = parent.bytes_sent;
  stats.parent_transport_calls = parent.collective_calls;
  stats.parent_transport_seconds = parent.collective_seconds;
  stats.communication_bytes = stats.imported.tracked_bytes + stats.parent_transport_bytes + stats.worker_transport_bytes;
  stats.communication_calls = stats.imported.tracked_calls + stats.parent_transport_calls + stats.worker_transport_calls;
  stats.communication_seconds = stats.imported.tracked_collective_seconds + stats.parent_transport_seconds +
                                stats.worker_transport_seconds;
  stats.process_cpu_seconds = double(std::clock() - cpu) / CLOCKS_PER_SEC;
  stats.total_seconds = parent.seconds() - start;
  stats.other_seconds = stats.total_seconds - std::accumulate(stats.phases.begin(), stats.phases.end(), 0.);
#if defined(__unix__)
  struct rusage usage {};
  if (!getrusage(RUSAGE_SELF, &usage)) {
#if defined(__APPLE__)
    stats.process_peak_rss_bytes = usage.ru_maxrss;
#else
    stats.process_peak_rss_bytes = transfer_memory::Mul(usage.ru_maxrss, 1024);
#endif
  }
#endif
  const auto report = parent.seconds();
  std::ostringstream row;
  row << std::setprecision(17) << "NATIVE3D rank=" << parent.rank << " ranks=" << parent.size
      << " workers=" << stats.workers << " complete=" << stats.complete << " detailed=" << stats.detailed_timings
      << " total=" << stats.total_seconds << " other=" << stats.other_seconds << " cpu=" << stats.process_cpu_seconds
      << " idle_cpu=" << stats.idle_cpu_seconds;
  for (size_t j = 0; j < stats.names.size(); ++j) row << ' ' << stats.names[j] << '=' << stats.phases[j];
  row << " initial=" << stats.initial_cells << " final=" << stats.final_cells
      << " refine_accepted=" << stats.refine.accepted << " coarsen_accepted=" << stats.coarsen.accepted
      << " tracked_bytes=" << stats.communication_bytes << " tracked_calls=" << stats.communication_calls
      << " nested_tracked_collectives=" << stats.communication_seconds
      << " tracked_import_bytes=" << stats.imported.tracked_bytes << " tracked_import_calls=" << stats.imported.tracked_calls
      << " nested_tracked_import=" << stats.imported.tracked_collective_seconds
      << " tracked_parent_bytes=" << stats.parent_transport_bytes << " tracked_parent_calls=" << stats.parent_transport_calls
      << " nested_tracked_parent=" << stats.parent_transport_seconds
      << " tracked_worker_bytes=" << stats.worker_transport_bytes << " tracked_worker_calls=" << stats.worker_transport_calls
      << " nested_tracked_worker=" << stats.worker_transport_seconds << " native_bound=" << stats.maximum_native_bytes
      << " process_peak_rss=" << stats.process_peak_rss_bytes << '\n';
  const auto text = row.str();
  const auto all = CPassiveComm::Allgatherv(std::vector<char>(text.begin(), text.end()), nullptr, parent.comm);
  if (!parent.rank) {
    std::cout.write(all.data(), all.size());
    if (!stats.complete)
      std::cout << "Native3D rejected event: " << reason << "; accepted CFD mesh/solution retained.\n";
  }
  if (control.timings) {
    std::ostringstream detail;
    detail << std::setprecision(17);
    auto prefix = [&](const char* stage) {
      detail << "NATIVE3D_DETAIL stage=" << stage << " rank=" << parent.rank << " ranks=" << parent.size
             << " workers=" << stats.workers << " complete=" << stats.complete;
    };
    auto bridge = [&](const char* stage, const BridgeStats& b) {
      prefix(stage);
      detail << " total=" << b.total_seconds << " points=" << b.points_seconds << " cells=" << b.cells_seconds
             << " faces=" << b.faces_seconds << " source=" << b.source_seconds << " query=" << b.query_seconds
             << " reader=" << b.reader_seconds << " other=" << b.other_seconds
             << " requests=" << b.requests << " hits=" << b.hits << " misses=" << b.misses
             << " invalid_queries=" << b.invalid_queries << " failures=" << b.failures << " evictions=" << b.evictions
             << " compositions=" << b.compositions << " batches=" << b.batches << " retries=" << b.batch_retries
             << " field_builds=" << b.field_builds << " nested_field_build=" << b.field_build_seconds << " nested_search=" << b.field_search_seconds
             << " nested_interpolation=" << b.interpolation_seconds << " nested_composition=" << b.composition_seconds
             << " tracked_bytes=" << b.tracked_bytes << " tracked_calls=" << b.tracked_calls
             << " nested_tracked_collectives=" << b.tracked_collective_seconds
             << " native_bound=" << b.maximum_native_bytes << '\n';
    };
    bridge("import", stats.imported); bridge("weights", stats.weights); bridge("return", stats.returned);
    prefix("source");
    const auto& d = stats.source;
    detail << " build=" << d.build_seconds << " imports=" << d.import_seconds << " requests=" << d.owner_requests
           << " candidates=" << d.index_candidates << " payloads=" << d.payloads << " maximum_patch=" << d.maximum_patch
           << " nested_routing=" << d.routing_seconds << " nested_search=" << d.search_seconds
           << " nested_filter=" << d.filter_seconds << " nested_exchange=" << d.exchange_seconds << '\n';
    prefix("partition"); const auto& p = stats.partition;
    auto at_rank = [&](const std::vector<uint64_t>& v) { return v.empty() ? uint64_t(0) : v.at(parent.rank); };
    detail << " total=" << p.total_seconds << " graph=" << p.graph_seconds << " parmetis=" << p.partition_seconds
           << " migration=" << p.migration_seconds << " other=" << p.other_seconds
           << " nested_directory=" << p.directory_seconds << " moved=" << p.moved_cells << " arcs=" << p.graph_arcs
           << " cells_before=" << at_rank(p.cells_before) << " cells_after=" << at_rank(p.cells_after)
           << " work_before=" << at_rank(p.work_before) << " work_after=" << at_rank(p.work_after) << '\n';
    auto engine = [&](const char* stage, const EngineStats& e) {
      prefix(stage);
      detail << " total=" << e.total_seconds << " initialization=" << e.initialization_seconds << " seed=" << e.seed_seconds << " first_layer=" << e.layer_seconds
             << " selection=" << e.selection_seconds << " dependency=" << e.dependency_seconds
             << " reservation=" << e.reservation_seconds << " donor=" << e.donor_seconds
             << " reconstruction=" << e.reconstruction_seconds << " metric=" << e.metric_seconds
             << " routing=" << e.routing_seconds << " publication=" << e.publication_seconds
             << " update=" << e.update_seconds << " completion=" << e.completion_seconds << " other=" << e.other_seconds
             << " tracked_bytes=" << e.tracked_bytes << " tracked_calls=" << e.tracked_calls
             << " nested_tracked_collectives=" << e.tracked_collective_seconds
             << " directory_elections=" << e.directory_elections << " nested_directory_election=" << e.directory_election_seconds
             << " process_cpu=" << e.process_cpu_seconds << " private_reconstruction_cpu=" << e.reconstruction_cpu_seconds
             << " private_metric_cpu=" << e.metric_cpu_seconds << " private_measurement_cpu=" << e.measurement_cpu_seconds
       << " planning_nested_s=" << e.planning_seconds
       << " batch_considered=" << e.batch_considered << " batch_deferred=" << e.batch_deferred
             << " proposal_duplicates=" << e.proposal_duplicates
             << " batch_rounds=" << e.batch_rounds << " batch_candidates=" << e.batch_candidates
             << " stop=" << EngineStopName(e.stop) << " global_work=" << e.global_work << " work_limit=" << e.work_limit
             << " progress_windows=" << e.progress_windows << " stagnant_windows=" << e.stagnant_windows << " search_windows=" << e.search_windows
             << " size_progress=" << e.size_progress << " quality_progress=" << e.quality_progress << " cell_removals=" << e.cell_removals
             << " layer_published=" << e.layer_published << " height_checks=" << e.height_checks << " height_rejected=" << e.height_rejected << " footprint_rejected=" << e.footprint_rejected
             << " wall_reconstruction_trials=" << e.wall_reconstruction_trials << " wall_reconstruction_accepted=" << e.wall_reconstruction_accepted
             << " wall_joint_trials=" << e.wall_joint_trials << " wall_joint_moves=" << e.wall_joint_moves << " wall_joint_removals=" << e.wall_joint_removals << " wall_joint_accepted=" << e.wall_joint_accepted
             << " placement_connectivity_trials=" << e.placement_connectivity_trials << " placement_flips=" << e.placement_flips << " placement_moves=" << e.placement_moves
             << " placement_dependency_expansions=" << e.placement_dependency_expansions
             << " placement_classified_vertices=" << e.placement_classified_vertices
             << " placement_trials=" << e.placement_trials << " placement_validated=" << e.placement_validated
             << " wall_topology_trials=" << e.wall_topology_trials << " wall_topology_accepted=" << e.wall_topology_accepted
             << " geometry_trials=" << e.geometry_trials << " geometry_rejected=" << e.geometry_rejected << " metric_trials=" << e.metric_trials
             << " metric_shape_rejected=" << e.metric_shape_rejected << " metric_progress_rejected=" << e.metric_progress_rejected
             << " metric_measurement_rejected=" << e.metric_measurement_rejected
             << " sample_metric_present=" << (e.last_metric_rejection.failure!=MetricFailure::None)
             << " sample_q=" << e.last_metric_rejection.minimum_mean_ratio << " sample_j=" << e.last_metric_rejection.minimum_scaled_jacobian
             << " sample_cell=" << e.last_metric_rejection.weakest_cell << " sample_sliver=" << e.last_metric_rejection.sliver_cell
             << " sample_feature=" << e.last_rejected_feature.count << " sample_v0=" << e.last_rejected_feature.vertices[0]
             << " sample_v1=" << e.last_rejected_feature.vertices[1] << " sample_v2=" << e.last_rejected_feature.vertices[2]
             << " sample_x=" << e.last_rejected_cavity_centroid.x << " sample_y=" << e.last_rejected_cavity_centroid.y << " sample_z=" << e.last_rejected_cavity_centroid.z
             << " batch_reserved=" << e.batch_reserved << " rounds=" << e.rounds << " attempted=" << e.attempted << " accepted=" << e.accepted
             << " region_core_trials=" << e.region_core_trials << " region_core_regions=" << e.region_core_regions
             << " region_transition_fronts=" << e.region_transition_fronts << " region_front_trials=" << e.region_front_trials
             << " region_cap_nested_s=" << e.region_cap_seconds
             << " region_sampling_s=" << e.region_sampling_seconds << " region_first_band_s=" << e.region_first_band_seconds
             << " region_transition_s=" << e.region_transition_seconds << " region_junction_s=" << e.region_junction_seconds
             << " region_core_s=" << e.region_core_seconds << " region_validation_s=" << e.region_validation_seconds
             << " region_field_points=" << e.region_field_points << " region_sensor_hits=" << e.region_field_sensor_hits
             << " region_interior_hits=" << e.region_field_interior_hits << " region_field_edges=" << e.region_field_edges
             << " region_edge_hits=" << e.region_field_edge_hits << " region_field_imports=" << e.region_field_imports
             << " region_maximum_resident=" << e.region_field_maximum_resident
             << " rejected=" << e.rejected << " conflicts=" << e.conflicts << " requests=" << e.field_requests
             << " hits=" << e.field_hits << " misses=" << e.field_misses << " invalid_queries=" << e.field_invalid_queries
             << " failures=" << e.field_failures << " evictions=" << e.field_evictions
             << " compositions=" << e.field_compositions << " edge_evaluations=" << e.edge_evaluations
             << " collapse_geometry_checks=" << e.collapse_geometry_checks << " collapse_filtered=" << e.collapse_filtered
             << " collapse_orientation_checks=" << e.collapse_orientation_checks << " collapse_orientation_filtered=" << e.collapse_orientation_filtered
             << " collapse_reactivated=" << e.collapse_reactivated << " changed_star_scans=" << e.changed_star_scans
             << " changed_vertices_sent=" << e.changed_vertices_sent << " changed_vertices_received=" << e.changed_vertices_received
             << " nested_change_notifications=" << e.change_notification_seconds << " nested_collapse_orientation=" << e.collapse_orientation_seconds
             << " nested_collapse_geometry=" << e.collapse_geometry_seconds
             << " edge_reused=" << e.edge_reused << " moved=" << e.moved << " reconnected=" << e.reconnected << " inserted=" << e.inserted
             << " maximum_donor_patch=" << e.donor_maximum_patch << " maximum_owned=" << e.maximum_owned
             << " final_owned_cells=" << e.final_owned_cells << " cached_measured_cells=" << e.cached_measured_cells
             << " cached_invalid_cells=" << e.cached_invalid_cells << " remaining_edges=" << e.remaining_edges
             << " remaining_repairs=" << e.remaining_repairs
             << " nested_field_build=" << e.field_build_seconds << " nested_field_query=" << e.field_query_seconds
             << " nested_cache=" << e.field_cache_seconds << " nested_search=" << e.field_search_seconds
             << " nested_interpolation=" << e.field_interpolation_seconds << " nested_composition=" << e.field_composition_seconds
             << " nested_edge_geometry=" << e.edge_geometry_seconds << " nested_edge_composition=" << e.edge_composition_seconds
             << " nested_edge=" << e.edge_seconds << " nested_edge_search=" << e.edge_search_seconds
             << " nested_edge_trace=" << e.edge_trace_seconds << " nested_edge_integral=" << e.edge_integral_seconds
             << " donor_reused=" << e.donor_reused << " edge_separated=" << e.edge_separated
             << " donor_batches=" << e.donor_batches << " donor_candidates=" << e.donor_candidates
             << " donor_filtered=" << e.donor_filtered << " donor_box_filtered=" << e.donor_box_filtered << " donor_payloads=" << e.donor_payloads
             << " nested_donor_search=" << e.donor_search_seconds << " nested_donor_filter=" << e.donor_filter_seconds
             << " batch_retries=" << e.batch_retries << " query_retries=" << e.query_import_retries
             << " private_query_retries=" << e.private_query_retries << " native_bound=" << e.maximum_work_bytes << '\n';
    };
    engine("refine",stats.refine); engine("coarsen",stats.coarsen);
    SU2Native3D::ProfileRow3D(detail.str(), parent.comm);
  }
  stats.report_seconds = parent.seconds() - report;
  return result;
}
