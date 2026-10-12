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
CNativeRemesher3D::CNativeRemesher3D(std::shared_ptr<ReferenceState> retained, EngineOptions control)
    : reference(std::move(retained)), options(control) {
  if (!reference) throw std::invalid_argument("Native3D needs driver-owned reference storage.");
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
  if (config.GetnAdap_BL())
    failure.Set(1, 0, "Native3D composed BL edge integration and first-height construction are not implemented.");
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
  stats.detailed_timings = control.timings;
  const auto start = parent.seconds();
  const auto cpu = std::clock();
  auto now = start;
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
    CollectiveFailure(failure, CURRENT_FUNCTION);
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
    const size_t retained = transfer_memory::Add(options.caller_bytes, ReferenceStorageBytes(*reference, false));
    auto held = [&]() {
      return transfer_memory::Add(retained, ReferenceStorageBytes(pending->state), partition_bytes());
    };
    ImportedGeometry imported;
    bool ready = ImportGeometry(config, geometry, &sensor, pending->state, imported, reason, stats.imported,
                                control.memory_ceiling, retained);
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, stats.imported.maximum_native_bytes);
    phase(2);
    if (!ready) break;
    stats.initial_cells = CPassiveComm::Allreduce(uint64_t(imported.cells.size()), CPassiveComm::Op::SUM, parent.comm);
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
            WorkingWeights(parent, working, source, weights, reason, stats.weights, {}, control.memory_ceiling,
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
                                       transfer_memory::Mul(parent.size, sizeof(std::vector<DonorCell>))))) {
          std::vector<std::vector<DonorCell>> send(parent.size);
          for (size_t j = 0; j < imported.original.size(); ++j)
            send.at(stats.partition.destinations.at(j)).push_back(imported.original[j]);
          // Empty original owners still enter this same migration collective.
          bool admitted = false;
          migrated = parent.exchange(send, &admitted, control.memory_ceiling, baseline);
          ready = admitted;
          if (!ready) reason = "Native3D original donor migration exceeds budget.";
          worker_source = &migrated;
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
      bool valid = true;
      std::string detail;
      if (parent.rank < stats.workers) {
        World workers(comm);
        auto kernel = control;
        // The engine charges its input map/source; caller_bytes covers only other live runtime buffers.
        kernel.caller_bytes = transfer_memory::Add(
            held(), worker_source == &imported.original ? 0 : transfer_memory::Bytes(imported.original));
        try {
          const auto begin = workers.seconds();
          auto refined = Refine(workers, working, *worker_source, *pending->state.original, {}, kernel);
          stats.refine = refined.stats;
          valid = refined.valid;
          detail = refined.reason;
          working.swap(refined.cells);
          Owned().swap(refined.cells);
          std::vector<SplitRecord>().swap(refined.splits);
          std::vector<Edge>().swap(refined.collapses);
          stats.phases[8] = workers.seconds() - begin;
          if (valid) {
            const auto begin_coarse = workers.seconds();
            auto coarse = Coarsen(workers, working, *worker_source, *pending->state.original, {}, kernel);
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
    if (ready)
      ready = ReaderSlices(parent, working, imported.original, pending->state, result.slices, reason, stats.returned,
                           {}, control.memory_ceiling, transfer_memory::Add(held(), transfer_memory::Bytes(migrated)),
                           control.timings);
    phase(11);
    stats.maximum_native_bytes =
        std::max({stats.maximum_native_bytes, stats.refine.maximum_work_bytes, stats.coarsen.maximum_work_bytes,
                  stats.returned.maximum_native_bytes, parent.max_exchange_work_bytes});
    if (ready) {
      result.markers = result.slices.markersWithElements;
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
             << " nested_field_build=" << b.field_build_seconds << " nested_search=" << b.field_search_seconds
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
      detail << " total=" << e.total_seconds << " initialization=" << e.initialization_seconds << " seed=" << e.seed_seconds
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
             << " batch_rounds=" << e.batch_rounds << " batch_candidates=" << e.batch_candidates
             << " batch_reserved=" << e.batch_reserved << " rounds=" << e.rounds << " attempted=" << e.attempted << " accepted=" << e.accepted
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
