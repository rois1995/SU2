/*!
 * \file CNativeRemesher3D.cpp
 * \brief Native tetrahedral remesh, elected failure return and per-rank phase reporting.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#include "../../include/adaptation/CNativeRemesher3D.hpp"
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
  const auto start = parent.seconds();
  const auto cpu = std::clock();
  CLocalFailure failure;
  std::string reason;
  auto now = start;
  auto phase = [&](size_t i) {
    const auto end = parent.seconds();
    stats.phases[i] += end - now;
    now = end;
  };
  struct Publication {
    ReferenceState state;
    bool published = false;
  };
  {
    CheckSupport(config, geometry);
    phase(0);
    auto pending = std::make_shared<Publication>();
    pending->state = *reference;
    if (!pending->state.original) {
      const bool loaded = LoadReference(config.GetMesh_FileName() + ".native_ref", pending->state);
      if (config.GetRestart() && !loaded)
        failure.Set(1, 0, "Native3D restart requires its original .native_ref sidecar; geometry cannot be rebased.");
    }
    CollectiveFailure(failure, CURRENT_FUNCTION);
    phase(1);
    auto imported = ImportGeometry(config, geometry, &sensor, pending->state);
    stats.imported = imported.stats;
    phase(2);
    stats.initial_cells = CPassiveComm::Allreduce(uint64_t(imported.cells.size()), CPassiveComm::Op::SUM, parent.comm);
    stats.workers = config.GetAdap_Native_Ranks() ? int(config.GetAdap_Native_Ranks()) : parent.size;
    auto control = options;
    control.timings = options.timings || config.GetWrt_Performance();
    stats.detailed_timings = control.timings;
    const int worker_low = CPassiveComm::Allreduce(stats.workers, CPassiveComm::Op::MIN, parent.comm),
              worker_high = CPassiveComm::Allreduce(stats.workers, CPassiveComm::Op::MAX, parent.comm);
    const int low = CPassiveComm::Allreduce(int(control.timings), CPassiveComm::Op::MIN, parent.comm),
              high = CPassiveComm::Allreduce(int(control.timings), CPassiveComm::Op::MAX, parent.comm);
    if (low != high || worker_low != worker_high || stats.workers < 1 || stats.workers > parent.size)
      failure.Set(1, 0, "Native3D profile/worker configuration differs or is invalid.");
    CollectiveFailure(failure, CURRENT_FUNCTION);
    auto vote = [&](bool valid, const std::string& message) {
      CLocalFailure local;
      if (!valid) local.Set(2, 0, message);
      const auto elected = parent.elect(local);
      reason = elected.message;
      return !elected.any;
    };
    bool ready = true;
    Owned working = imported.cells;
    std::vector<DonorCell> migrated;
    {
      DonorImport source(parent, control.timings);
      ready = source.Build(imported.original, reason, control.memory_ceiling, control.caller_bytes);
      phase(3);
      std::vector<uint32_t> weights;
      const bool repartition = config.GetAdap_Native_Repartition() || stats.workers < parent.size;
      const int partition_low = CPassiveComm::Allreduce(int(repartition), CPassiveComm::Op::MIN, parent.comm),
                partition_high = CPassiveComm::Allreduce(int(repartition), CPassiveComm::Op::MAX, parent.comm);
      if (partition_low != partition_high) ready = vote(false, "Inconsistent native3D repartition controls.");
      if (ready)
        ready = WorkingWeights(parent, working, source, weights, reason, stats.weights, {}, control.memory_ceiling,
                               control.caller_bytes, control.timings);
      phase(4);
      if (ready && repartition)
        ready = Repartition(parent, working, weights, stats.workers, stats.partition, reason, control.memory_ceiling,
                            control.caller_bytes);
      phase(5);
      if (ready) {
        // A rank with no original donors must still enter the migration collective.
        if (!repartition)
          migrated = imported.original;
        else {
          std::vector<std::vector<DonorCell>> send(parent.size);
          for (size_t i = 0; i < imported.original.size(); ++i)
            send.at(stats.partition.destinations.at(i)).push_back(imported.original[i]);
          bool admitted = false;
          migrated = parent.exchange(
              send, &admitted, control.memory_ceiling,
              transfer_memory::Add(control.caller_bytes, transfer_memory::Bytes(imported.original),
                                   transfer_memory::Mul(working.size() + imported.cells.size(),
                                                        sizeof(Owned::value_type) + 4 * sizeof(void*))));
          ready = admitted;
          if (!ready) reason = "Native3D original donor migration exceeds budget.";
        }
      }
      phase(6);
    }
    if (ready) {
      auto comm = parent.comm;
#ifdef HAVE_MPI
      MPI_Comm_split(parent.comm, parent.rank < stats.workers ? 0 : MPI_UNDEFINED, parent.rank, &comm);
#endif
      phase(7);
      bool valid = true;
      std::string detail;
      uint64_t bytes = 0;
      if (parent.rank < stats.workers) {
        World workers(comm);
        try {
          const auto begin = workers.seconds();
          auto refined = Refine(workers, working, migrated, *pending->state.original, {}, control);
          stats.phases[8] = workers.seconds() - begin;
          stats.refine = refined.stats;
          valid = refined.valid;
          detail = refined.reason;
          if (valid) {
            const auto coarsening = workers.seconds();
            auto coarse = Coarsen(workers, refined.cells, migrated, *pending->state.original, {}, control);
            stats.phases[9] = workers.seconds() - coarsening;
            stats.coarsen = coarse.stats;
            valid = coarse.valid;
            detail = coarse.reason;
            working.swap(coarse.cells);
          } else
            working.swap(refined.cells);
        } catch (const CElectedFailure& e) {
          valid = false;
          detail = e.message;
        } catch (const std::exception& e) {
          SU2_MPI::Error(e.what(), CURRENT_FUNCTION);
        }
        bytes = workers.bytes_sent;
      }
      // Inactive CFD ranks park instead of burning a core while M workers adapt.
      now = parent.seconds();
      const auto idle_cpu = std::clock();
#ifdef HAVE_MPI
#if MPI_VERSION >= 3
      MPI_Request ready_request;
      MPI_Ibarrier(parent.comm, &ready_request);
      int complete = 0;
      while (!complete) {
        MPI_Test(&ready_request, &complete, MPI_STATUS_IGNORE);
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
      stats.communication_bytes = parent.bytes_sent + bytes;
      ready = vote(valid, detail);
    }
    now = parent.seconds();
    if (ready)
      ready = ReaderSlices(parent, working, imported.original, pending->state, result.slices, reason, stats.returned,
                           {}, control.memory_ceiling, control.caller_bytes, control.timings);
    phase(11);
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
  }
  stats.process_cpu_seconds = double(std::clock() - cpu) / CLOCKS_PER_SEC;
  stats.total_seconds = parent.seconds() - start;
  stats.other_seconds = stats.total_seconds - std::accumulate(stats.phases.begin(), stats.phases.end(), 0.);
  const auto report = parent.seconds();
  std::ostringstream row;
  row << std::setprecision(17) << "NATIVE3D rank=" << parent.rank << " ranks=" << parent.size
      << " workers=" << stats.workers << " complete=" << stats.complete << " detailed=" << stats.detailed_timings
      << " total=" << stats.total_seconds << " other=" << stats.other_seconds << " cpu=" << stats.process_cpu_seconds
      << " idle_cpu=" << stats.idle_cpu_seconds;
  for (size_t i = 0; i < stats.names.size(); ++i) row << ' ' << stats.names[i] << '=' << stats.phases[i];
  row << " initial=" << stats.initial_cells << " final=" << stats.final_cells
      << " refine_accepted=" << stats.refine.accepted << " coarsen_accepted=" << stats.coarsen.accepted
      << " tracked_bytes=" << stats.communication_bytes << '\n';
  const auto text = row.str();
  const auto all = CPassiveComm::Allgatherv(std::vector<char>(text.begin(), text.end()), nullptr, parent.comm);
  if (!parent.rank) {
    std::cout.write(all.data(), all.size());
    if (!stats.complete)
      std::cout << "Native3D rejected event: " << reason << "; accepted CFD mesh/solution retained.\n";
  }
  stats.report_seconds = parent.seconds() - report;
  return result;
}
