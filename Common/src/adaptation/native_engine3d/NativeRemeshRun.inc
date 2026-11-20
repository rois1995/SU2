#include "../../../include/adaptation/CNativeLocalCore3D.hpp"


// Remesh owns the timer and public transaction. These stages borrow its controls and live inputs.
namespace {
struct Publication {
  ReferenceState state;
  bool published = false;
};

// Shared geometry ownership is counted once even while pending and live state both refer to it.
struct ReferencePreparation {
  CLocalFailure& failure;
  std::shared_ptr<Publication>& pending;
  size_t& retained;
};

// Import and target lifetimes span the worker run and return packing.
struct RemeshInput {
  NormalizationStage3D& normalizer;
  ImportedGeometry& imported;
  MetricComposition& composition;
  double& frozen_core;
  size_t base_caller;
};

// All workers operate on the original working map and selected original donor source.
struct WorkerMesh {
  Owned& working;
  std::vector<DonorCell>& migrated;
  const std::vector<DonorCell>*& worker_source;
  bool repartition;
  bool& ready;
};

// Borrows a single worker trial, including its private policy and deadline accounting.
struct WorkerTrial {
  World& workers;
  EngineOptions& kernel;
  double worker_seconds;
  double worker_start;
  bool& valid;
  std::string& detail;
  std::vector<DonorCell>& stage_source;
  double begin;
};

// Executes one remesh while the caller's parent communicator and remesh timer stay alive.
struct RemeshRun {
  const CConfig& config;
  const CGeometry& geometry;
  const su2activematrix& sensor;
  const std::shared_ptr<ReferenceState>& reference;
  const EngineOptions& options;
  const CNativeRemesher3D::CompositionFactory& compositionFactory;
  const CNativeRemesher3D::NormalizationFactory& normalizationFactory;
  const CNativeRemesher3D::ConstructionFactory& construction;
  const std::function<void()>& constructionAccepted;
  RemeshStats& stats;
  World& parent;
  CRemeshResult& result;
  EngineOptions& control;
  double start;
  std::clock_t cpu;
  double& now;
  std::chrono::steady_clock::time_point deadline_start;
  const char* caller;
  std::string reason{};
  SU2NativeGeometry::AtlasBudget target_event_budget{};
  bool reset_need_at_start = false;

  // Agrees the remaining parent-event time from the original deadline origin.
  double RemainingTime() {
    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - deadline_start).count();
    return CPassiveComm::Allreduce(control.maximum_seconds - elapsed, CPassiveComm::Op::MIN, parent.comm);
  }

  // Charges the preceding interval to the selected remesh phase.
  void Phase(size_t i) {
    const auto end = parent.seconds();
    stats.phases[i] += end - now;
    now = end;
  }

  // Elects the synchronous failure and stores its reason for the event.
  bool Vote(bool valid, const std::string& message) {
    CLocalFailure local;
    if (!valid) local.Set(2, 0, message);
    const auto elected = parent.elect(local);
    reason = elected.message;
    return !elected.any;
  }

  // Elects the whole-job memory refusal at the existing safe point.
  bool CheckJobMemory(size_t peak, const char* message) {
    CLocalFailure local;
    if (control.job_memory_limit && peak > control.job_memory_limit) local.Set(3, 0, message);
    const auto elected = parent.elect(local);
    if (!elected.any) return true;
    stats.refine.stop = EngineStop::MemoryBudget;
    reason = elected.message;
    return false;
  }

  // Records the peak modeled bytes before voting on the current allowance.
  bool Admit(size_t bytes) {
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, bytes);
    return Vote(bytes != SIZE_MAX && bytes <= control.memory_ceiling,
                "Native3D runtime retained/working storage exceeds budget.");
  }

  // Accounts for the retained partition vectors used throughout the event.
  size_t PartitionBytes() {
    return transfer_memory::Add(
        transfer_memory::Bytes(stats.partition.destinations), transfer_memory::Bytes(stats.partition.cells_before),
        transfer_memory::Bytes(stats.partition.cells_after), transfer_memory::Bytes(stats.partition.work_before),
        transfer_memory::Bytes(stats.partition.work_after), transfer_memory::Bytes(stats.partition.geometry_before),
        transfer_memory::Bytes(stats.partition.geometry_after));
  }

  // Accounts for the shared authority once while both reference states remain live.
  size_t Held(const ReferencePreparation& preparation) {
    return transfer_memory::Add(preparation.retained, ReferenceStorageBytes(preparation.pending->state),
                                PartitionBytes());
  }

  // Reads the same frozen deadline for the local geometry and metric queries.
  bool WithinTime() const {
    return !control.maximum_seconds ||
           std::chrono::duration<double>(std::chrono::steady_clock::now() - deadline_start).count() <
               control.maximum_seconds;
  }

  // Owns the event geometry ledger through final reporting, including failed transactions.
  void Run() {
    if (reference->original && reference->original->Parameters()) {
      reference->original->Parameters()->Geometry().ResetGeometryNeedStats();
      reset_need_at_start = true;
    }

    TryRemesh();
    Finalize();
    ReportRemesh(config, control, parent, stats, reason, target_event_budget);
  }

  // Agrees event controls before making any private reference changes.
  void TryRemesh() {
    CNativeRemesher3D::CheckSupport(config, geometry);
    Phase(0);
    stats.job_peak_rss_bytes = JobPeakRSS3D(parent.comm);
    if (!CheckJobMemory(stats.job_peak_rss_bytes, "Native3D whole-job peak RSS exceeds ADAP_NATIVE_JOB_MEMORY_LIMIT."))
      return;
    const int worker_low = CPassiveComm::Allreduce(stats.workers, CPassiveComm::Op::MIN, parent.comm),
              worker_high = CPassiveComm::Allreduce(stats.workers, CPassiveComm::Op::MAX, parent.comm);
    const int low = CPassiveComm::Allreduce(int(control.timings), CPassiveComm::Op::MIN, parent.comm),
              high = CPassiveComm::Allreduce(int(control.timings), CPassiveComm::Op::MAX, parent.comm);
    CLocalFailure failure;
    if (low != high || worker_low != worker_high || stats.workers < 1 || stats.workers > parent.size)
      failure.Set(1, 0, "Native3D profile/worker configuration differs or is invalid.");
    const auto work_lo = CPassiveComm::Allreduce(control.maximum_work, CPassiveComm::Op::MIN, parent.comm);
    const auto work_hi = CPassiveComm::Allreduce(control.maximum_work, CPassiveComm::Op::MAX, parent.comm);
    const auto time_lo = CPassiveComm::Allreduce(control.maximum_seconds, CPassiveComm::Op::MIN, parent.comm);
    const auto time_hi = CPassiveComm::Allreduce(control.maximum_seconds, CPassiveComm::Op::MAX, parent.comm);
    const auto memory_lo =
        CPassiveComm::Allreduce(uint64_t(control.job_memory_limit), CPassiveComm::Op::MIN, parent.comm);
    const auto memory_hi =
        CPassiveComm::Allreduce(uint64_t(control.job_memory_limit), CPassiveComm::Op::MAX, parent.comm);
    if (work_lo != work_hi || time_lo != time_hi || memory_lo != memory_hi || !std::isfinite(time_lo) ||
        !std::isfinite(time_hi) || time_lo < 0)
      failure.Set(1, 0, "Native3D work/time controls differ or are invalid.");
    const int normalizer_low =
                  CPassiveComm::Allreduce(int(bool(normalizationFactory)), CPassiveComm::Op::MIN, parent.comm),
              normalizer_high =
                  CPassiveComm::Allreduce(int(bool(normalizationFactory)), CPassiveComm::Op::MAX, parent.comm);
    if (normalizer_low != normalizer_high) failure.Set(1, 0, "Native3D normalization factory differs across ranks.");
    CollectiveFailure(failure, caller);
    const int bl_low = CPassiveComm::Allreduce(int(config.GetnAdap_BL()), CPassiveComm::Op::MIN, parent.comm),
              bl_high = CPassiveComm::Allreduce(int(config.GetnAdap_BL()), CPassiveComm::Op::MAX, parent.comm);
    if (!Vote(bl_low == bl_high, "Native3D geometric policy differs across CFD ranks.")) return;
    PrepareReference(failure);
  }

  // Loads and validates the pending reference before geometry import.
  void PrepareReference(CLocalFailure& failure) {
    // Admit retained/publication state first; sidecar loading admits its actual byte and construction demand.
    const size_t reference_setup = transfer_memory::Add(options.caller_bytes, ReferenceStorageBytes(*reference),
                                                        ReferenceStorageBytes(*reference, false));
    if (!Admit(reference_setup)) {
      Phase(1);
      return;
    }
    auto pending = std::make_shared<Publication>();
    pending->state = *reference;
    if (!pending->state.original) {
      bool loaded = false;
      if (!LoadReference(config.GetMesh_FileName() + ".native_ref", pending->state, loaded, reason,
                         control.memory_ceiling, reference_setup, stats.maximum_native_bytes)) {
        Phase(1);
        return;
      }
      if (config.GetRestart() && !loaded)
        failure.Set(1, 0, "Native3D restart requires its original .native_ref sidecar; geometry cannot be rebased.");
    }
    try {
      pending->state.first_height_tol = SU2NativeFirstHeight::AdoptRecorded(
          pending->state.first_height_tol, SU2_TYPE::GetValue(config.GetAdap_BL_FirstHeightTol()));
    } catch (const std::exception& e) {
      failure.Set(1, 0, e.what());
    }
    CollectiveFailure(failure, caller);
    Phase(1);
    Import(failure, pending);
  }

  // Retains the pending mobility and normalization state through the private mesh stages.
  void Import(CLocalFailure& failure, std::shared_ptr<Publication>& pending) {
    // Retained and pending state share the immutable authority: count its allocation once.
    size_t retained = transfer_memory::Add(options.caller_bytes, ReferenceStorageBytes(*reference, false));
    ReferencePreparation preparation{failure, pending, retained};
    try {
      PrepareMobility(config, pending->state, control.memory_ceiling, retained);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    {
      const auto setup_failure = parent.elect(failure);
      if (setup_failure.any) {
        reason = setup_failure.message;
        return;
      }
    }
    NormalizationStage3D normalizer;
    try {
      if (normalizationFactory) normalizer = normalizationFactory(config, geometry, sensor);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    {
      const auto setup_failure = parent.elect(failure);
      if (setup_failure.any) {
        reason = setup_failure.message;
        return;
      }
    }
    const int job_low = CPassiveComm::Allreduce(int(bool(normalizer.propose)), CPassiveComm::Op::MIN, parent.comm),
              job_high = CPassiveComm::Allreduce(int(bool(normalizer.propose)), CPassiveComm::Op::MAX, parent.comm);
    if (!Vote(job_low == job_high, "Native3D normalization job differs across ranks.")) return;
    retained = transfer_memory::Add(retained, normalizer.retained_bytes);
    control.caller_bytes = transfer_memory::Add(control.caller_bytes, normalizer.retained_bytes);
    const auto base_caller = control.caller_bytes;
    ImportedGeometry imported;
    bool ready = ImportGeometry(config, geometry, &sensor, pending->state, imported, reason, stats.imported,
                                control.memory_ceiling, retained);
    if (!reset_need_at_start && pending->state.original && pending->state.original->Parameters())
      pending->state.original->Parameters()->Geometry().ResetGeometryNeedStats();
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, stats.imported.maximum_native_bytes);
    Phase(2);
    if (!ready) return;
    try {
      PrepareMobility(
          config, pending->state, control.memory_ceiling,
          transfer_memory::Add(retained, OwnedStorageBytes(imported.cells), transfer_memory::Bytes(imported.original)));
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    {
      const auto setup_failure = parent.elect(failure);
      if (setup_failure.any) {
        reason = setup_failure.message;
        return;
      }
    }
    if (!PromoteReference(pending)) return;
    CreateTarget(preparation, normalizer, base_caller, imported, ready);
  }

  // Uses one authority kind on all parent ranks before staging first-row work.
  bool PromoteReference(const std::shared_ptr<Publication>& pending) {
    // A construction works on the mapped authority: a plain faceted original is promoted on EVERY rank (deterministic,
    // identical inputs), so later parameter synchronization sees one authority kind on all ranks.
    if (construction && !pending->state.original->Mapped() && pending->state.original->Analytic().empty() &&
        pending->state.original->Affine().empty()) {
      CLocalFailure promotion;
      try {
        double declared = 0;
        // DC-17: symmetry-plane markers (configuration kinds) get fitted affine plane authorities first, so their
        // interior original vertices can slide on the promoted reference (any plane orientation); the fit contract is
        // the promotion's own declared mapping error (code review 1: a looser fit would fail the promotion)
        std::set<int> planar;
        for (size_t m = 0; m < pending->state.marker_names.size(); ++m)
          if (config.GetMarker_CfgFile_KindBC(pending->state.marker_names[m]) == SYMMETRY_PLANE) planar.insert(int(m));
        size_t fitted = 0;
        const auto fittedReference =
            SU2Native3D::FitPlanarMarkers(pending->state.original, pending->state.facets, planar, 1e-13, fitted);
        pending->state.original =
            SU2Native3D::PromoteFacetedReference(fittedReference, pending->state.facets, 1e-13, declared);
        if (parent.rank == 0)
          std::cout << "DC17 symmetry planes fitted: " << fitted << " of the components of " << planar.size()
                    << " symmetry marker(s)" << std::endl;
        control.surface_tolerance = std::max(control.surface_tolerance, declared);
      } catch (const std::exception& e) {
        promotion.Set(2, 0, e.what());
      }
      if (!Vote(!promotion.Failed(), promotion.message)) return false;
    }
    return true;
  }

  // Composes the initial target from the imported frozen sensor and wall policy.
  void CreateTarget(ReferencePreparation& preparation, NormalizationStage3D& normalizer, size_t base_caller,
                    ImportedGeometry& imported, bool& ready) {
    auto& failure = preparation.failure;
    auto& pending = preparation.pending;
    auto& retained = preparation.retained;
    MetricComposition composition;
    double frozen_core = 0;
    if (config.GetnAdap_BL()) {
      if (!Vote(bool(compositionFactory), "Native3D configured BL requires an actual-query geometric policy.")) return;
      // shortcut: conservative wall-policy construction scratch; replace with measured peak admission for large
      // surfaces.
      if (!Admit(transfer_memory::Add(Held(preparation), OwnedStorageBytes(imported.cells),
                                      transfer_memory::Bytes(imported.original),
                                      transfer_memory::Mul(pending->state.facets.size(), 8192))))
        return;
      double localCore = std::numeric_limits<double>::max();
      try {
        for (unsigned long p = 0; p < geometry.GetnPointDomain(); ++p) {
          su2double m[3][3] = {{sensor(p, 0), sensor(p, 1), sensor(p, 2)},
                               {sensor(p, 1), sensor(p, 3), sensor(p, 4)},
                               {sensor(p, 2), sensor(p, 4), sensor(p, 5)}},
                    vec[3][3], val[3], work[3];
          CBlasStructure::EigenDecomposition(m, vec, val, 3, work);
          const double smallest = SU2_TYPE::GetValue(*std::min_element(val, val + 3));
          if (!(std::isfinite(smallest) && smallest > 0))
            throw std::runtime_error("Invalid Native3D frozen sensor core.");
          localCore = std::min(localCore, smallest);
        }
      } catch (const std::exception& e) {
        failure.Set(2, 0, e.what());
      }
      CollectiveFailure(failure, caller);
      const double core = CPassiveComm::Allreduce(localCore, CPassiveComm::Op::MIN, parent.comm);
      frozen_core = core;
      try {
        SU2NativeTiming3D::Scope target_timer(SU2NativeTiming3D::Operation::TargetComposition);
        composition = compositionFactory(config, pending->state, core, control);
      } catch (const std::exception& e) {
        failure.Set(2, 0, e.what());
      }
      CollectiveFailure(failure, caller);
      if (!Vote(bool(composition) && bool(control.edge_breakpoints),
                "Native3D BL policy omitted composition or geometric edge bands."))
        return;
      if (!Vote(control.caller_bytes >= options.caller_bytes, "Native3D geometric policy decreased retained storage."))
        return;
      retained = transfer_memory::Add(retained, control.caller_bytes - base_caller);
      if (!Admit(transfer_memory::Add(Held(preparation), OwnedStorageBytes(imported.cells),
                                      transfer_memory::Bytes(imported.original))))
        return;
    }
    RemeshInput input{normalizer, imported, composition, frozen_core, base_caller};
    LeaseTarget(preparation, input, ready);
  }

  // Keeps the geometry workspace reserved through packing and private teardown.
  void LeaseTarget(ReferencePreparation& preparation, RemeshInput& input, bool& ready) {
    auto& failure = preparation.failure;
    auto& imported = input.imported;
    Phase(12);
    stats.initial_cells = CPassiveComm::Allreduce(uint64_t(imported.cells.size()), CPassiveComm::Op::SUM, parent.comm);
    if (control.maximum_work == SIZE_MAX)
      control.maximum_work = std::max<size_t>(4096, transfer_memory::Mul(stats.initial_cells, 64));
    if (!Vote(control.maximum_work != SIZE_MAX, "Native3D automatic global work ceiling overflows.")) return;
    std::shared_ptr<void> target_event_lease;
    if (control.geometry_workspace_bytes) {
      try {
        if (control.geometry_workspace_bytes >= control.memory_ceiling)
          throw std::length_error("Native event geometry scratch exceeds shared memory ceiling.");
        target_event_budget.maximum_bytes = control.memory_ceiling;
        control.memory_ceiling -= control.geometry_workspace_bytes;
        target_event_budget.caller_bytes = control.memory_ceiling;
        target_event_budget.maximum_work =
            transfer_memory::Mul(transfer_memory::Add(stats.initial_cells, control.maximum_work),
                                 SU2NativeGeometry::MappedFacetReference::ClearanceWork);
        if (target_event_budget.maximum_work == SIZE_MAX)
          throw std::length_error("Native event geometry work overflows.");
        target_event_budget.within_time = [this] { return WithinTime(); };
        if (!control.geometry_reservation)
          throw std::invalid_argument("Native event omitted its retained geometry reservation binding.");
        target_event_lease = control.geometry_reservation(target_event_budget);
      } catch (const std::exception& e) {
        failure.Set(2, 0, e.what());
      }
      if (!Vote(!failure.Failed(), failure.message)) return;
      if (!Admit(transfer_memory::Add(Held(preparation), OwnedStorageBytes(imported.cells),
                                      transfer_memory::Bytes(imported.original))))
        return;
    }
    AdaptImported(preparation, input, ready);
  }

  // Owns the working mesh and donors until the remesh result is packed.
  void AdaptImported(ReferencePreparation& preparation, RemeshInput& input, bool& ready) {
    auto& imported = input.imported;
    Owned working = std::move(imported.cells);  // Private import already owns this map; no duplicate is needed.
    std::vector<DonorCell> migrated;
    const std::vector<DonorCell>* worker_source = &imported.original;
    const bool repartition = config.GetAdap_Native_Repartition() || stats.workers < parent.size;
    WorkerMesh mesh{working, migrated, worker_source, repartition, ready};
    Partition(preparation, input, mesh);
    // Index and weight teardown is part of source preparation.
    Phase(3);
    std::unique_ptr<DonorImport> return_source;
    RunStages(preparation, input, mesh, return_source);
    FinalizeMesh(preparation, input, mesh, return_source);
  }

  // Moves immutable donors under the existing migration storage vote.
  void MigrateSource(RemeshInput& input, WorkerMesh& mesh, size_t baseline) {
    auto& imported = input.imported;
    auto& migrated = mesh.migrated;
    auto& worker_source = mesh.worker_source;
    auto& ready = mesh.ready;
    SU2NativeTiming3D::Scope migration_timer(SU2NativeTiming3D::Operation::Communication);
    if (Admit(transfer_memory::Add(baseline, transfer_memory::GrowthBound(imported.original.size(), sizeof(DonorCell)),
                                   transfer_memory::Mul(stats.partition.destinations.size(),
                                                        sizeof(std::pair<const Id, int>) + 4 * sizeof(void*)),
                                   transfer_memory::Mul(parent.size, sizeof(std::vector<DonorCell>))))) {
      std::vector<std::vector<DonorCell>> send(parent.size);
      try {
        const bool coverage = std::any_of(imported.original.begin(), imported.original.end(),
                                          [](const DonorCell& d) { return d.extension_component != UINT64_MAX; });
        if (!coverage) {
          for (size_t j = 0; j < imported.original.size(); ++j)
            send.at(stats.partition.destinations.at(j)).push_back(imported.original[j]);
        } else {
          std::map<Id, int> destinations;
          size_t j = 0;
          for (const auto& d : imported.original)
            if (d.extension_component == UINT64_MAX)
              destinations.emplace(d.cell.id, stats.partition.destinations.at(j++));
          for (const auto& d : imported.original)
            send.at(destinations.at(d.extension_component == UINT64_MAX ? d.cell.id : d.parent_cell)).push_back(d);
        }
      } catch (const std::exception& e) {
        ready = false;
        reason = e.what();
      }
      ready = Vote(ready, reason);
      // Empty original owners still enter this same migration collective after common preparation approval.
      if (ready) {
        bool admitted = false;
        migrated = parent.exchange(send, &admitted, control.memory_ceiling, baseline);
        ready = admitted;
        if (!ready) reason = "Native3D original donor migration exceeds budget.";
        worker_source = &migrated;
      }
    } else
      ready = false;
  }

  // Keeps the donor index and weights alive through partition and migration.
  void Partition(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh) {
    auto& imported = input.imported;
    auto& working = mesh.working;
    auto& repartition = mesh.repartition;
    auto& ready = mesh.ready;
    DonorImport source(parent, control.timings);

    const int partition_low = CPassiveComm::Allreduce(int(repartition), CPassiveComm::Op::MIN, parent.comm),
              partition_high = CPassiveComm::Allreduce(int(repartition), CPassiveComm::Op::MAX, parent.comm);
    if (partition_low != partition_high) ready = Vote(false, "Inconsistent native3D repartition controls.");
    if (ready && repartition)
      ready = source.Build(imported.original, reason, control.memory_ceiling,
                           transfer_memory::Add(Held(preparation), OwnedStorageBytes(working)));
    Phase(3);
    std::vector<uint32_t> weights, geometry_weights;
    {
      SU2NativeTiming3D::Scope partition_timer(SU2NativeTiming3D::Operation::Communication);
      PrepareWeights(preparation, input, mesh, source, weights, geometry_weights);
      Phase(4);
      RepartitionMesh(preparation, input, mesh, source, weights, geometry_weights);
    }
    Phase(5);
    if (ready && repartition) {
      const size_t baseline = transfer_memory::Add(
          Held(preparation), transfer_memory::Bytes(imported.original), OwnedStorageBytes(working),
          source.ResidentBytes(), transfer_memory::Bytes(weights), transfer_memory::Bytes(geometry_weights));
      MigrateSource(input, mesh, baseline);
    }
    Phase(6);
    stats.maximum_native_bytes = std::max({stats.maximum_native_bytes, source.Statistics().maximum_work_bytes,
                                           stats.weights.maximum_native_bytes, stats.partition.maximum_native_bytes,
                                           parent.max_exchange_work_bytes});
    stats.source = source.Statistics();
  }

  // Measures routing work only when more than one destination can use it.
  void PrepareWeights(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                      DonorImport& source, std::vector<uint32_t>& weights, std::vector<uint32_t>& geometry_weights) {
    auto& pending = preparation.pending;
    auto& imported = input.imported;
    auto& composition = input.composition;
    auto& working = mesh.working;
    auto& repartition = mesh.repartition;
    auto& ready = mesh.ready;
    if (ready && repartition && stats.workers == 1) {
      // A sole destination cannot use work estimates; keep graph validation and complete migration.
      ready = Admit(transfer_memory::Add(Held(preparation), transfer_memory::Bytes(imported.original),
                                         OwnedStorageBytes(working), source.ResidentBytes(),
                                         transfer_memory::Mul(working.size(), sizeof(uint32_t))));
      if (ready) {
        try {
          weights.assign(working.size(), 1);
        } catch (const std::exception& e) {
          ready = false;
          reason = e.what();
        }
        ready = Vote(ready, reason);
      }
    } else if (ready && repartition)
      ready = WorkingWeights(
          parent, working, source, weights, reason, stats.weights, composition, control.memory_ceiling,
          transfer_memory::Add(Held(preparation), transfer_memory::Bytes(imported.original)), control.timings,
          pending->state.original.get(), control.surface_tolerance, &geometry_weights, PartitionWorkPhase::Coarsening);
  }

  // Changes working-cell owners and emits the corresponding partition row.
  void RepartitionMesh(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                       DonorImport& source, const std::vector<uint32_t>& weights,
                       const std::vector<uint32_t>& geometry_weights) {
    auto& imported = input.imported;
    auto& working = mesh.working;
    auto& repartition = mesh.repartition;
    auto& ready = mesh.ready;
    if (ready && repartition)
      ready = Repartition(
          parent, working, weights, stats.workers, stats.partition, reason, control.memory_ceiling,
          transfer_memory::Add(Held(preparation), transfer_memory::Bytes(imported.original), source.ResidentBytes()),
          geometry_weights);
    if (ready && repartition) {
      const auto& p = stats.partition;
      const auto r = parent.rank;
      const auto& w = stats.weights;
      std::cout << std::setprecision(17) << "NATIVE3D_PARTITION rank=" << r << " parts=" << stats.workers
                << " uniform_routing_weights=" << (stats.workers == 1) << " cells_before=" << p.cells_before.at(r)
                << " cells_after=" << p.cells_after.at(r) << " metric_before=" << p.work_before.at(r)
                << " metric_after=" << p.work_after.at(r) << " geometry_before=" << p.geometry_before.at(r)
                << " geometry_after=" << p.geometry_after.at(r) << " geometry_positive_cells="
                << std::count_if(geometry_weights.begin(), geometry_weights.end(), [](uint32_t x) { return x != 0; })
                << " geometry_unit_cells=" << std::count(geometry_weights.begin(), geometry_weights.end(), uint32_t(1))
                << " geometry_maximum_cells="
                << std::count(geometry_weights.begin(), geometry_weights.end(), MaximumPartitionWeight)
                << " geometry_weight_scale=" << w.geometry_weight_scale
                << " estimate_seconds=" << w.geometry_estimate_seconds
                << " original_operations=" << w.geometry_operations << " original_geometry_work=" << w.geometry_work
                << " original_boundary_faces=" << w.geometry_faces
                << " original_cross_chart_faces=" << w.geometry_cross_chart_faces
                << " migration_seconds=" << p.migration_seconds << " partition_seconds=" << p.partition_seconds
                << " moved_cells=" << p.moved_cells << " edgecut=" << p.edgecut << " arcs=" << p.graph_arcs
                << " native_bytes=" << p.maximum_native_bytes << std::endl;
    }
  }

  // Runs worker adaptation, then at most one agreed normalization correction.
  void RunStages(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                 std::unique_ptr<DonorImport>& return_source) {
    auto& normalizer = input.normalizer;
    auto& ready = mesh.ready;
    for (unsigned stage = 0; ready && stage < 2; ++stage) {
      ++stats.normalization_stages;
      stats.construction_stats = {};
      stats.refine = {};
      stats.coarsen = {};
      if (ready) RunWorkers(preparation, input, mesh);
      const auto used = CPassiveComm::Allreduce(
          stats.construction_stats.global_work + stats.refine.global_work + stats.coarsen.global_work,
          CPassiveComm::Op::MAX, parent.comm);
      stats.normalization_stage_work[stage] = used;
      stats.cumulative_work += used;
      stats.maximum_native_bytes = std::max({stats.maximum_native_bytes, stats.construction_stats.maximum_work_bytes,
                                             stats.refine.maximum_work_bytes, stats.coarsen.maximum_work_bytes});
      now = parent.seconds();
      std::shared_ptr<const PreparedComplexity3D> prepared;
      Reassess(preparation, input, mesh, return_source, prepared);
      Phase(13);
      if (ready && stage == 1 && stats.complexity.estimated) {
        const double target = SU2_TYPE::GetValue(config.GetAdap_Complexity());
        if (std::abs(stats.complexity.value - target) >
            std::max(target * 1e-6, stats.complexity.uncertainty + stats.normalization.estimate.uncertainty))
          stats.normalization.reason = "correction_stage_limit_budget_residual";
      }
      if (!ready || stage == 1 || !normalizer.propose || !prepared) break;
      if (!Normalize(preparation, input, mesh, return_source, prepared)) break;
    }
  }

  // Keeps the communicator alive through worker activity and the parent join.
  void RunWorkers(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh) {
    auto& pending = preparation.pending;
    auto& imported = input.imported;
    auto& working = mesh.working;
    auto& migrated = mesh.migrated;
    auto& ready = mesh.ready;
    const auto idle_peak = CPassiveComm::Allreduce(uint64_t(parent.rank >= stats.workers ? ProcessPeakRSS3D() : 0),
                                                   CPassiveComm::Op::SUM, parent.comm);
    auto comm = parent.comm;
#ifdef HAVE_MPI
    MPI_Comm_split(parent.comm, parent.rank < stats.workers ? 0 : MPI_UNDEFINED, parent.rank, &comm);
#endif
    Phase(7);
    double worker_seconds = 0;
    if (control.maximum_seconds) {
      worker_seconds = RemainingTime();
      ready = Vote(worker_seconds > 0, "Native3D elapsed deadline exhausted before worker adaptation.");
    }
    bool valid = ready;
    std::string detail = ready ? std::string{} : reason;
    if (parent.rank < stats.workers && ready)
      AdaptWorker(preparation, input, mesh, comm, idle_peak, worker_seconds, valid, detail);
    JoinWorkers(comm, valid, detail);
    Phase(7);
    ready = Vote(valid, detail);
    // review 3: the construction worker may have raised the publication ceiling; every parent rank uses that value
    control.memory_ceiling =
        size_t(CPassiveComm::Allreduce(uint64_t(control.memory_ceiling), CPassiveComm::Op::MAX, parent.comm));
    if (control.geometry_workspace_bytes) {
      target_event_budget.maximum_bytes =
          transfer_memory::Add(control.memory_ceiling, control.geometry_workspace_bytes);
      target_event_budget.caller_bytes = control.memory_ceiling;
    }
    if (ready && stats.workers < parent.size && pending->state.original->Mapped()) {
      ready = SynchronizeBoundaryParameters(
          parent, stats.workers, pending->state, reason, stats.parameter_return, control.memory_ceiling,
          transfer_memory::Add(Held(preparation), OwnedStorageBytes(working), transfer_memory::Bytes(imported.original),
                               transfer_memory::Bytes(migrated)));
      Phase(11);
    }
  }

  // Waits for all ranks before measuring the peak and freeing the worker communicator.
  void JoinWorkers([[maybe_unused]] SU2_Comm& comm, bool& valid, std::string& detail) {
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
    Phase(10);
    if (parent.rank >= stats.workers) stats.idle_cpu_seconds += double(std::clock() - idle_cpu) / CLOCKS_PER_SEC;
    stats.job_peak_rss_bytes = JobPeakRSS3D(parent.comm);
    if (!CheckJobMemory(stats.job_peak_rss_bytes,
                        "Native3D whole-job peak RSS exceeds ADAP_NATIVE_JOB_MEMORY_LIMIT.")) {
      valid = false;
      detail = reason;
    }
    now = parent.seconds();
#ifdef HAVE_MPI
    if (comm != MPI_COMM_NULL) MPI_Comm_free(&comm);
#endif
  }

  // Keeps private shape admission and donor state through both worker stages.
  void AdaptWorker(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, SU2_Comm comm,
                   uint64_t idle_peak, double worker_seconds, bool& valid, std::string& detail) {
    auto& imported = input.imported;
    auto& worker_source = mesh.worker_source;
    World workers(comm);
    auto kernel = control;
    kernel.idle_peak_rss_bytes = stats.workers < parent.size ? idle_peak : 0;
    if (control.geometry_workspace_bytes) kernel.shared_geometry_budget = &target_event_budget;
    kernel.maximum_seconds = worker_seconds;
    kernel.maximum_work = control.maximum_work - stats.cumulative_work;
    const auto worker_start = workers.seconds();
    // The engine charges its input map/source; caller_bytes covers only other live runtime buffers.
    kernel.caller_bytes =
        transfer_memory::Add(Held(preparation), transfer_memory::Bytes(imported.original),
                             worker_source == &imported.original ? 0 : transfer_memory::Bytes(*worker_source));
    try {
      struct RestoreShape {
        ShapeAdmission previous = GetShapeAdmission();
        ~RestoreShape() { SetShapeAdmission(previous); }
      } restore_shape;
      SetShapeAdmission(ShapeAdmission::ValidityOnly);
      const auto begin = workers.seconds();
      std::vector<DonorCell> stage_source;
      WorkerTrial trial{workers, kernel, worker_seconds, worker_start, valid, detail, stage_source, begin};
      if (config.GetAdap_Native_Local_Core()) {
        const bool applicable = !input.composition && !construction && kernel.wall_heights.empty() &&
                                kernel.region_walls.empty() &&
                                !workers.maximum(std::any_of(mesh.working.begin(), mesh.working.end(),
                                                             [](const auto& row) { return !row.second.sensor_present; }));
        if (applicable)
          RunLocalCore(mesh, trial);
        else if (control.timings)
          std::cout << "NATIVE3D_LOCAL_CORE rank=" << workers.rank << " applicable=0" << std::endl;
      }
      if (valid) valid = SnapshotSource(mesh, trial, *worker_source);
      if (valid && construction) ConstructRows(preparation, input, mesh, trial);
      if (valid && construction) valid = SnapshotSource(mesh, trial, stage_source);
      if (valid && control.maximum_seconds) {
        kernel.maximum_seconds = CPassiveComm::Allreduce(worker_seconds - (workers.seconds() - worker_start),
                                                         CPassiveComm::Op::MIN, workers.comm);
        if (!(kernel.maximum_seconds > 0)) {
          valid = false;
          detail = "Native3D deadline exhausted preparing current stage metrics.";
        }
      }
      if (valid) ApplyMetric(preparation, input, mesh, trial);
    } catch (const CElectedFailure& e) {
      valid = false;
      detail = e.message;
    } catch (const std::exception& e) {
      SU2_MPI::Error(e.what(), caller);
    }
    stats.worker_transport_bytes += workers.bytes_sent;
    stats.worker_transport_calls += workers.collective_calls;
    stats.worker_transport_seconds += workers.collective_seconds;
  }

  void PrintLocalPass(int rank, unsigned pass, const SU2NativeLocal3D::PassStats& p, double seconds, size_t bytes) {
    std::cout << "NATIVE3D_LOCAL_CORE rank=" << rank << " stage=" << stats.normalization_stages - 1
              << " pass=" << pass << " applicable=1 cells_in=" << p.cells_in << " cells_out=" << p.cells_out
              << " points_in=" << p.points_in << " points_out=" << p.points_out
              << " surface_points=" << p.surface_points << " border_points=" << p.border_points
              << " wall_near_points=" << p.wall_near_points << " collapses=" << p.collapses << " splits=" << p.splits
              << " edge_removals=" << p.edge_removals << " face_swaps=" << p.face_swaps << " moves=" << p.moves
              << " deferred=" << p.deferred << " skipped_memory=" << p.skipped_memory
              << " refused_numeric=" << p.refused_numeric << " seconds_build=" << p.seconds_build
              << " seconds_collapse=" << p.seconds_collapse << " seconds_split=" << p.seconds_split
              << " seconds_quality=" << p.seconds_quality << " seconds_check=" << p.seconds_check
              << " seconds_write=" << p.seconds_write << " evaluated_collapse=" << p.evaluated_collapse
              << " evaluated_split=" << p.evaluated_split << " evaluated_quality=" << p.evaluated_quality
              << " round_changes=" << p.round_changes << " quality_changes=" << p.quality_changes
              << " refused_valid=" << p.refused_valid << " refused_size=" << p.refused_size
              << " refused_topology=" << p.refused_topology << " refused_memory=" << p.refused_memory
              << " refused_shape=" << p.refused_shape << " refused_exact=" << p.refused_exact
              << " blocked=" << p.blocked
              << " long_left_surface=" << p.long_left_surface << " long_left_border=" << p.long_left_border
              << " short_out=" << p.short_out << " edges_out=" << p.edges_out << " bad_in=" << p.bad_in
              << " bad_out=" << p.bad_out << " bad_out_free=" << p.bad_out_free
              << " sweeps_collapse=" << p.sweeps_collapse << " sweeps_split=" << p.sweeps_split
              << " sweeps_quality=" << p.sweeps_quality << " rounds=" << p.rounds << " limit=" << p.limit
              << " seconds=" << seconds << " bytes=" << bytes << std::endl;
  }

  void PrintLocalSweep(int rank, unsigned pass, const SU2NativeLocal3D::SweepRow& row) {
    std::cout << "NATIVE3D_LOCAL_SWEEP rank=" << rank << " stage=" << stats.normalization_stages - 1
              << " pass=" << pass;
    SU2NativeLocal3D::PrintSweepFields(std::cout, row);
    std::cout << std::endl;
  }

  void PrintLocalShift(int rank, bool lower, const SU2NativeLocal3D::ShiftStats& shift, double seconds) {
    std::cout << "NATIVE3D_LOCAL_SHIFT rank=" << rank << " stage=" << stats.normalization_stages - 1
              << " direction=" << (lower ? "lower" : "higher")
              << " cells_before=" << shift.cells_before << " cells_after=" << shift.cells_after << " sent=" << shift.sent
              << " received=" << shift.received << " border_points=" << shift.border_points;
    if (!lower) std::cout << " frozen_in_both=" << shift.frozen_in_both;
    std::cout << " seconds=" << seconds << std::endl;
  }

  // Replace the working volume before the existing current-nodal snapshot.
  void RunLocalCore(WorkerMesh& mesh, WorkerTrial& trial) {
    namespace core = SU2NativeLocal3D;
    SU2NativeTiming3D::PhaseScope phase(SU2NativeTiming3D::Phase::Construction);
    SU2NativeTiming3D::Scope topology(SU2NativeTiming3D::Operation::Topology);
    auto& workers = trial.workers;
    auto& working = mesh.working;
    const auto& kernel = trial.kernel;
    Id point_base = 0, cell_base = 0;
    const auto visit = [&](const Cell& cell) {
      cell_base = std::max(cell_base, cell.id);
      for (const auto& point : cell.v) point_base = std::max(point_base, point.id);
    };
    for (const auto& entry : working) visit(entry.second.cell);
    for (const auto& donor : *mesh.worker_source) visit(donor.cell);
    core::IdSequence point_ids(workers.reduce<uint64_t>(point_base, CPassiveComm::Op::MAX), workers.rank, workers.size);
    core::IdSequence cell_ids(workers.reduce<uint64_t>(cell_base, CPassiveComm::Op::MAX), workers.rank, workers.size);

    constexpr size_t owned_row_bytes = sizeof(Owned::value_type) + 4 * sizeof(void*);
    core::MemoryBudget budget{kernel.memory_ceiling, kernel.caller_bytes, owned_row_bytes};
    const unsigned passes = workers.size == 1 ? 1 : core::PassCount;
    const bool report = control.timings;
    core::BorderMeeting previous;
    for (unsigned pass = 1; pass <= passes; ++pass) {
      const double begin = report ? workers.seconds() : 0;
      auto meeting = core::MeetBorderPoints(workers, working, budget, previous.Bytes(), report);
      const auto bytes = transfer_memory::Add(kernel.caller_bytes, OwnedStorageBytes(working), previous.Bytes(),
                                              meeting.Bytes(), core::Mesh::ArrayBytes(working.size()));
      budget.Require(workers, bytes, "Native local core arrays exceed working-storage admission.");
      stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, budget.maximum_bytes);
      const auto cell_limit = (kernel.memory_ceiling - kernel.caller_bytes) / (owned_row_bytes + core::Mesh::BytesPerCell);
      core::PassStats counts;
      counts.report = report;
      if (report) counts.sink = [this, rank = workers.rank, pass](const auto& row) { PrintLocalSweep(rank, pass, row); };
      CLocalFailure local;
      try {
        const auto build_start = report ? workers.seconds() : 0.0;
        core::Mesh pass_mesh(working, workers.size > 1);
        const auto build_seconds = report ? workers.seconds() - build_start : 0.0;
        pass_mesh.MarkBorderSurface(meeting);
        counts = pass_mesh.RunPass(point_ids, cell_limit, std::move(counts));
        const auto write_start = report ? workers.seconds() : 0.0;
        pass_mesh.WriteBack(working, cell_ids);
        if (report) {
          counts.seconds_build = build_seconds;
          counts.seconds_write = workers.seconds() - write_start;
        }
      } catch (const std::exception& e) {
        local.Set(2, 0, e.what());
      }
      const auto elected = workers.elect(local);
      if (report && !local.Failed()) PrintLocalPass(workers.rank, pass, counts, workers.seconds() - begin, bytes);
      if (elected.any) throw elected;

      if (passes > 1) {
        const bool lower = pass == 1;
        const double shift_begin = report ? workers.seconds() : 0;
        const auto direction = lower ? core::Direction::Lower : core::Direction::Higher;
        const auto shift =
            core::ShiftBorders(workers, working, meeting, direction, budget, lower ? nullptr : &previous, report);
        stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, budget.maximum_bytes);
        if (report) PrintLocalShift(workers.rank, lower, shift, workers.seconds() - shift_begin);
        if (lower) previous.rendezvous = std::move(meeting.rendezvous);
      }
    }
  }

  // Replaces the private stage donor snapshot only after the existing vote.
  bool SnapshotSource(WorkerMesh& mesh, WorkerTrial& trial, const std::vector<DonorCell>& supplied) {
    auto& working = mesh.working;
    auto& workers = trial.workers;
    auto& kernel = trial.kernel;
    auto& detail = trial.detail;
    auto& stage_source = trial.stage_source;
    CLocalFailure local;
    const auto live =
        transfer_memory::Add(kernel.caller_bytes, OwnedStorageBytes(working), transfer_memory::Bytes(stage_source));
    if (workers.maximum(std::any_of(working.begin(), working.end(),
                                    [](const Owned::value_type& row) { return !row.second.sensor_present; }))) {
      CavityMetricSource initialize(workers, working, control.timings);
      std::string why;
      if (!initialize.Build(supplied, why, kernel.memory_ceiling, live)) {
        detail = why;
        return false;
      }
      stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, initialize.Statistics().maximum_work_bytes);
    }
    std::vector<DonorCell> next;
    try {
      if (transfer_memory::Add(live, transfer_memory::Mul(working.size(), sizeof(DonorCell))) > kernel.memory_ceiling)
        throw std::length_error("Current stage metric snapshot exceeds working-storage admission.");
      next.reserve(working.size());
      for (const auto& row : working) {
        DonorCell d;
        d.cell = row.second.cell;
        d.sensor = row.second.sensor;
        for (const auto& value : d.sensor) ValidateTensor(value);
        next.push_back(d);
      }
    } catch (const std::exception& e) {
      local.Set(2, 0, e.what());
    }
    const auto elected = workers.elect(local);
    if (elected.any) {
      detail = elected.message;
      return false;
    }
    stage_source.swap(next);
    return true;
  }

  // Accepts first-row cells provisionally and charges their shared work.
  void ConstructRows(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, WorkerTrial& trial) {
    auto& pending = preparation.pending;
    auto& composition = input.composition;
    auto& working = mesh.working;
    auto& workers = trial.workers;
    auto& kernel = trial.kernel;
    auto& worker_seconds = trial.worker_seconds;
    auto& worker_start = trial.worker_start;
    auto& valid = trial.valid;
    auto& detail = trial.detail;
    auto& stage_source = trial.stage_source;
    auto& begin = trial.begin;
    // FIRST_ROW publishes only a provisional private mesh into the shared distributed stages.
    stats.construction = true;
    if (kernel.shared_geometry_budget) kernel.memory_ceiling = target_event_budget.maximum_bytes;
    auto built =
        construction(workers, working, stage_source, pending->state, composition, kernel, stats.construction_report);
    stats.construction_stats = built.stats;
    valid = built.valid;
    if (valid && built.reference) pending->state.original = std::move(built.reference);
    detail = built.reason;
    working.swap(built.cells);
    Owned().swap(built.cells);
    AdjustPublicationCeiling(preparation, input, mesh, trial);
    if (built.stats.global_work > kernel.maximum_work) {
      valid = false;
      detail = "FIRST_ROW construction exhausted shared native work.";
    } else
      kernel.maximum_work -= built.stats.global_work;
    kernel.memory_ceiling = control.memory_ceiling;
    if (valid && control.maximum_seconds) {
      kernel.maximum_seconds = CPassiveComm::Allreduce(worker_seconds - (workers.seconds() - worker_start),
                                                       CPassiveComm::Op::MIN, workers.comm);
      if (!(kernel.maximum_seconds > 0)) {
        valid = false;
        detail = "Native3D elapsed deadline exhausted after first-row construction.";
      }
    }
    stats.phases[8] += workers.seconds() - begin;
  }

  // Accounts for first-row multiplication under the existing automatic cap.
  void AdjustPublicationCeiling(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                                WorkerTrial& trial) {
    auto& imported = input.imported;
    auto& working = mesh.working;
    auto& migrated = mesh.migrated;
    auto& valid = trial.valid;
    auto& detail = trial.detail;
    // DC-16: the construction multiplies the cell count (0.3M -> 1.2M on the ONERA M6 at h0 x32), so the fixed
    // 2 GiB ceiling rejected the publication (original donor transport). The publication ceiling becomes
    // max(2 GiB, 1.5 x the estimated need of this event), capped at 25 % of the node memory; above the cap the
    // event fails with that reason. Need = max(measured peak RSS of the event, live candidate + staged import
    // estimate). An explicit configured ceiling stays fixed and all subsequent stages admit against it.
    if (valid && !config.GetAdap_Native_MemoryLimit()) {
      const long pages = sysconf(_SC_PHYS_PAGES), page = sysconf(_SC_PAGESIZE);
      const size_t cap = pages > 0 && page > 0 ? size_t(uint64_t(pages) * uint64_t(page) / 4) : control.memory_ceiling;
      // measured: the event's own peak resident set (the construction and its inputs); estimated: the live
      // candidate plus the staged import (sorted donor copy, boxes, index, authority records sent and received)
      struct rusage usage {};
      getrusage(RUSAGE_SELF, &usage);
      const size_t measured = size_t(usage.ru_maxrss) * 1024;
      // staged import per original donor (DonorImport::Build): sorted copy, 6 box doubles, 256 B scratch, box
      // tree, 6 authority records with growth, their send/receive copies and the authority map: ~6 KiB
      const size_t estimated = transfer_memory::Add(
          Held(preparation), OwnedStorageBytes(working), transfer_memory::Bytes(migrated),
          transfer_memory::Bytes(imported.original), transfer_memory::Mul(6144, imported.original.size()));
      const size_t need = std::max(measured, estimated);
      const size_t wanted =
          std::max(control.geometry_workspace_bytes ? target_event_budget.maximum_bytes : control.memory_ceiling,
                   need >= SIZE_MAX / 2 ? SIZE_MAX : need + need / 2);  // never below the pre-construction ceiling
      std::cout << "DC16 publication memory: need " << need / (size_t(1) << 20) << " MiB (measured peak "
                << measured / (size_t(1) << 20) << ", estimated " << estimated / (size_t(1) << 20) << "), ceiling "
                << std::min(wanted, cap) / (size_t(1) << 20) << " MiB (default "
                << control.memory_ceiling / (size_t(1) << 20) << " MiB, cap 25 % of node memory "
                << cap / (size_t(1) << 20) << " MiB)" << std::endl;
      if (wanted > cap) {
        valid = false;
        detail = "FIRST_ROW publication needs an estimated " + std::to_string(need >> 20) +
                 " MiB x 1.5, above the 25 % node-memory cap (" + std::to_string(cap >> 20) + " MiB).";
      } else {
        control.memory_ceiling = wanted;
        if (control.geometry_workspace_bytes) {
          target_event_budget.maximum_bytes = wanted;
          control.memory_ceiling -= control.geometry_workspace_bytes;
          target_event_budget.caller_bytes = control.memory_ceiling;
        }
      }
    }
  }

  // Uses the remaining worker work and time budgets for strict metric adaptation.
  void ApplyMetric(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh, WorkerTrial& trial) {
    auto& pending = preparation.pending;
    auto& composition = input.composition;
    auto& working = mesh.working;
    auto& workers = trial.workers;
    auto& kernel = trial.kernel;
    auto& worker_seconds = trial.worker_seconds;
    auto& worker_start = trial.worker_start;
    auto& valid = trial.valid;
    auto& detail = trial.detail;
    auto& stage_source = trial.stage_source;
    const auto refine_begin = workers.seconds();
    if (kernel.shared_geometry_budget) kernel.memory_ceiling = target_event_budget.maximum_bytes;
    kernel.report_rejected_neighborhood = true;
    auto refined = AdaptMetric(workers, working, stage_source, *pending->state.original, composition, kernel);
    stats.refine = refined.stats;
    valid = refined.valid;
    if (valid && refined.reference) pending->state.original = std::move(refined.reference);
    detail = refined.reason;
    working.swap(refined.cells);
    Owned().swap(refined.cells);
    std::vector<SplitRecord>().swap(refined.splits);
    std::vector<Edge>().swap(refined.collapses);
    stats.phases[8] += workers.seconds() - refine_begin;
    if (valid) {
      kernel.maximum_work -= refined.stats.global_work;
      if (control.maximum_seconds) {
        kernel.maximum_seconds = CPassiveComm::Allreduce(worker_seconds - (workers.seconds() - worker_start),
                                                         CPassiveComm::Op::MIN, workers.comm);
        if (!(kernel.maximum_seconds > 0)) {
          valid = false;
          detail = "Native3D elapsed deadline exhausted after metric adaptation.";
        }
      }
    }
  }

  // Reuses the parent donor index for the correction proposal and later return packing.
  void Reassess(const ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                std::unique_ptr<DonorImport>& return_source, std::shared_ptr<const PreparedComplexity3D>& prepared) {
    auto& pending = preparation.pending;
    auto& imported = input.imported;
    auto& composition = input.composition;
    auto& normalizer = input.normalizer;
    auto& working = mesh.working;
    auto& migrated = mesh.migrated;
    auto& ready = mesh.ready;
    if (ready) {
      CLocalFailure failure;
      try {
        return_source = std::make_unique<DonorImport>(parent, control.timings);
      } catch (const std::exception& e) {
        failure.Set(3, 0, e.what());
      }
      ready = Vote(!failure.Failed(), failure.message);
      const auto live =
          transfer_memory::Add(Held(preparation), OwnedStorageBytes(working), transfer_memory::Bytes(migrated));
      const auto buildStart = parent.seconds();
      if (ready) ready = return_source->Build(imported.original, reason, control.memory_ceiling, live);
      const auto sourceSeconds = parent.seconds() - buildStart;
      if (ready) {
        auto within = [this] { return WithinTime(); };
        ready = ReassessComplexity(parent, working, *return_source, *pending->state.original, composition,
                                   stats.complexity, reason, stats.reassessment, control.memory_ceiling,
                                   transfer_memory::Add(Held(preparation), transfer_memory::Bytes(migrated)), 8192,
                                   20261017, within, normalizer.propose ? &prepared : nullptr);
        stats.reassessment.source_seconds += sourceSeconds;
        stats.reassessment.total_seconds += sourceSeconds;
        result.complexity = {stats.complexity.value,          stats.complexity.uncertainty,
                             stats.complexity.standard_error, stats.complexity.block_standard_error,
                             stats.complexity.samples,        stats.complexity.estimated,
                             stats.complexity.reason};
      }
    }
  }

  // Keeps the correction source alive through both the proposal and application.
  bool Normalize(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                 std::unique_ptr<DonorImport>& return_source, std::shared_ptr<const PreparedComplexity3D>& prepared) {
    auto& imported = input.imported;
    auto& working = mesh.working;
    auto& migrated = mesh.migrated;
    now = parent.seconds();
    NormalizationProposal3D proposal;
    const bool nodal_correction = prepared->current_nodal;
    std::vector<DonorCell> correction_source;
    const auto normalization_live = transfer_memory::Add(
        Held(preparation), OwnedStorageBytes(working), transfer_memory::Bytes(imported.original),
        transfer_memory::Bytes(migrated), return_source->ResidentBytes(), prepared->ResidentBytes());
    if (!ProposeCorrection(preparation, input, mesh, prepared, proposal, nodal_correction, correction_source,
                           normalization_live))
      return false;
    return ApplyCorrection(preparation, input, mesh, return_source, prepared, proposal, nodal_correction);
  }

  // Agrees the proposal while the original metric timers and source are live.
  bool ProposeCorrection(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                         const std::shared_ptr<const PreparedComplexity3D>& prepared, NormalizationProposal3D& proposal,
                         bool nodal_correction, std::vector<DonorCell>& correction_source, size_t normalization_live) {
    auto& failure = preparation.failure;
    auto& normalizer = input.normalizer;
    auto& frozen_core = input.frozen_core;
    auto& imported = input.imported;
    auto& ready = mesh.ready;
    auto& working = mesh.working;
    if (nodal_correction &&
        !Admit(transfer_memory::Add(normalization_live, transfer_memory::Mul(working.size(), sizeof(DonorCell))))) {
      ready = false;
      return false;
    }
    try {
      if (nodal_correction) {
        correction_source.reserve(working.size());
        for (const auto& row : working) {
          if (!row.second.sensor_present) throw std::runtime_error("Normalization lost current nodal sensors.");
          DonorCell d;
          d.cell = row.second.cell;
          d.sensor = row.second.sensor;
          correction_source.push_back(d);
        }
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!Vote(!failure.Failed(), failure.message)) {
      ready = false;
      return false;
    }
    try {
      auto within = [this] { return WithinTime(); };
      SU2NativeTiming3D::PhaseScope normalization_phase(SU2NativeTiming3D::Phase::Metric);
      SU2NativeTiming3D::Scope normalization_timer(SU2NativeTiming3D::Operation::MetricNormalization);
      proposal = normalizer.propose(
          stats.complexity, *prepared, nodal_correction ? correction_source : imported.original, frozen_core, within,
          control.memory_ceiling, transfer_memory::Add(normalization_live, transfer_memory::Bytes(correction_source)));
    } catch (const std::exception& e) {
      proposal.valid = false;
      proposal.reason = e.what();
    }
    try {
      stats.normalization = proposal;
      stats.normalization.source_value = {};
    } catch (const std::exception& e) {
      failure.Set(3, 0, e.what());
    }
    if (!Vote(!failure.Failed(), failure.message)) {
      ready = false;
      return false;
    }
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, proposal.maximum_bytes);
    const int changed_low = CPassiveComm::Allreduce(int(proposal.changed), CPassiveComm::Op::MIN, parent.comm),
              changed_high = CPassiveComm::Allreduce(int(proposal.changed), CPassiveComm::Op::MAX, parent.comm);
    ready = Vote(proposal.valid && changed_low == changed_high,
                 proposal.reason.empty() ? "Native3D normalization stage differs across ranks." : proposal.reason);
    Phase(14);
    if (!ready || !proposal.changed) return false;
    return true;
  }

  // Releases assessment indexes before changing the current nodal field.
  bool ApplyCorrection(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                       std::unique_ptr<DonorImport>& return_source,
                       std::shared_ptr<const PreparedComplexity3D>& prepared, NormalizationProposal3D& proposal,
                       bool nodal_correction) {
    auto& failure = preparation.failure;
    auto& pending = preparation.pending;
    auto& retained = preparation.retained;
    auto& imported = input.imported;
    auto& base_caller = input.base_caller;
    auto& composition = input.composition;
    auto& frozen_core = input.frozen_core;
    auto& ready = mesh.ready;
    auto& working = mesh.working;
    auto& migrated = mesh.migrated;
    auto& repartition = mesh.repartition;
    ready = Vote(stats.cumulative_work < control.maximum_work,
                 "Native3D normalization correction exhausted shared mesh work.");
    if (!ready) return false;
    // Retire assessment indexes before updating the current nodal field.
    return_source.reset();
    prepared.reset();
    if (!Admit(transfer_memory::Add(Held(preparation), OwnedStorageBytes(working),
                                    transfer_memory::Bytes(imported.original), transfer_memory::Bytes(migrated),
                                    proposal.retained_bytes))) {
      ready = false;
      return false;
    }
    try {
      if (!proposal.source_value || !(proposal.core > 0) || !std::isfinite(proposal.core))
        throw std::runtime_error("Native3D normalization omitted its original source values/core.");
      if (nodal_correction) {
        for (auto& row : working)
          for (unsigned k = 0; k < 4; ++k) {
            row.second.sensor[k] = proposal.source_value(row.second.cell.v[k].id);
            ValidateTensor(row.second.sensor[k]);
          }
      } else
        for (auto& d : imported.original)
          for (unsigned k = 0; k < 4; ++k) {
            d.sensor[k] = proposal.source_value(d.cell.v[k].id);
            ValidateTensor(d.sensor[k]);
          }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    ready = Vote(!failure.Failed(), failure.message);
    proposal.source_value = {};
    if (!ready) return false;
    const auto previous_policy = control.caller_bytes - base_caller;
    composition = {};
    control.edge_breakpoints = {};
    control.caller_bytes = base_caller;
    frozen_core = proposal.core;
    retained -= previous_policy;
    try {
      SU2NativeTiming3D::Scope target_timer(SU2NativeTiming3D::Operation::TargetComposition);
      if (config.GetnAdap_BL()) composition = compositionFactory(config, pending->state, proposal.core, control);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    ready = Vote(!failure.Failed() && control.caller_bytes >= base_caller &&
                     (!config.GetnAdap_BL() || (bool(composition) && bool(control.edge_breakpoints))),
                 "Native3D normalized geometric policy is invalid.");
    retained = transfer_memory::Add(retained, control.caller_bytes - base_caller);
    Phase(12);
    if (!ready) return false;
    std::vector<DonorCell>().swap(migrated);
    if (repartition)
      MigrateSource(input, mesh,
                    transfer_memory::Add(Held(preparation), OwnedStorageBytes(working),
                                         transfer_memory::Bytes(imported.original)));
    Phase(6);
    return true;
  }

  // Packs the private result and binds publication to CFD acceptance.
  void FinalizeMesh(ReferencePreparation& preparation, RemeshInput& input, WorkerMesh& mesh,
                    std::unique_ptr<DonorImport>& return_source) {
    auto& pending = preparation.pending;
    auto& imported = input.imported;
    auto& ready = mesh.ready;
    auto& working = mesh.working;
    auto& migrated = mesh.migrated;
    // Nominal complexity overshoot is diagnostic; physical and resource gates still govern publication.
    if (ready && stats.complexity.estimated &&
        stats.complexity.value - stats.complexity.uncertainty > SU2_TYPE::GetValue(config.GetAdap_Complexity())) {
      stats.normalization.reason = "nominal_complexity_overshoot: " + stats.normalization.reason;
    }
    // Transport only the original P1 sensor; geometric BL is recomposed at each subsequent query.
    if (ready) {
      std::shared_ptr<const PlanarReference> rebound;
      ready = ReaderSlices(parent, working, imported.original, pending->state, result.slices, reason, stats.returned,
                           {}, control.memory_ceiling,
                           transfer_memory::Add(Held(preparation), transfer_memory::Bytes(migrated)), control.timings,
                           return_source.get(), &rebound);
      if (ready && rebound) pending->state.original = std::move(rebound);
    }
    Phase(11);
    if (pending->state.original && pending->state.original->Parameters()) {
      const auto m18 = pending->state.original->Parameters()->Geometry().GeometryStatsSnapshot();
      stats.geometry_distinct_facets = m18.distinct_facets;
      stats.geometry_distinct_scope_keys = m18.distinct_scope_keys;
      stats.geometry_prepared_facet_mean_bytes = m18.prepared_facet_mean_bytes;
    }
    stats.maximum_native_bytes = std::max(
        {stats.maximum_native_bytes, stats.construction_stats.maximum_work_bytes, stats.refine.maximum_work_bytes,
         stats.coarsen.maximum_work_bytes, stats.returned.maximum_native_bytes, stats.reassessment.maximum_native_bytes,
         stats.parameter_return.maximum_native_bytes, parent.max_exchange_work_bytes});
    if (ready && control.maximum_seconds)
      ready = Vote(RemainingTime() > 0, "Native3D elapsed deadline exhausted after return-to-CFD packing.");
    if (ready) {
      result.markers = result.slices.markersWithElements;
      if (control.maximum_seconds) {
        const auto duration = std::chrono::duration<double>(control.maximum_seconds);
        result.beforeAcceptance = [deadline_start = deadline_start, duration]() {
          return std::chrono::steady_clock::now() - deadline_start < duration;
        };
      }
      result.status = CRemeshResult::Status::COMPLETE;
      result.reference_io_caller_bytes =
          transfer_memory::Add(control.caller_bytes, ReferenceStorageBytes(*reference), result.slices.ResidentBytes(),
                               transfer_memory::Bytes(result.markers));
      result.onAccepted = [state = reference, pending, accepted = constructionAccepted]() {
        if (!pending->published) {
          std::swap(*state, pending->state);
          pending->published = true;
          if (accepted) accepted();
        }
      };
      stats.final_cells = result.slices.nElemGlobal;
      stats.complete = true;
    }
  }

  // Accounts for attempted work and applies the last whole-job memory check.
  void Finalize() {
    stats.maximum_native_bytes = std::max(stats.maximum_native_bytes, target_event_budget.peak_bytes);
    // Capture attempted import/parent/worker work even when an earlier phase rejects the event.
    stats.parent_transport_bytes = parent.bytes_sent;
    stats.parent_transport_calls = parent.collective_calls;
    stats.parent_transport_seconds = parent.collective_seconds;
    stats.communication_bytes =
        stats.imported.tracked_bytes + stats.parent_transport_bytes + stats.worker_transport_bytes;
    stats.communication_calls =
        stats.imported.tracked_calls + stats.parent_transport_calls + stats.worker_transport_calls;
    stats.communication_seconds =
        stats.imported.tracked_collective_seconds + stats.parent_transport_seconds + stats.worker_transport_seconds;
    stats.process_cpu_seconds = double(std::clock() - cpu) / CLOCKS_PER_SEC;
    stats.total_seconds = parent.seconds() - start;
    stats.other_seconds = stats.total_seconds - std::accumulate(stats.phases.begin(), stats.phases.end(), 0.);
    stats.process_peak_rss_bytes = ProcessPeakRSS3D();
    stats.job_peak_rss_bytes = JobPeakRSS3D(parent.comm, stats.process_peak_rss_bytes);
    if (!CheckJobMemory(stats.job_peak_rss_bytes,
                        "Native3D whole-job peak RSS exceeds ADAP_NATIVE_JOB_MEMORY_LIMIT.")) {
      result.status = CRemeshResult::Status::INCOMPLETE_QUALITY;
      result.slices = {};
      result.markers.clear();
      result.beforeAcceptance = {};
      result.onAccepted = {};
      stats.complete = false;
      stats.final_cells = 0;
    }
    const auto& proof = stats.refine.final_proof_surface_faces ? stats.refine : stats.construction_stats;
    stats.final_proof_faces = proof.final_proof_faces;
    stats.final_proof_surface_faces = proof.final_proof_surface_faces;
    stats.job_peak_rss_lower_bytes = std::max({stats.construction_stats.job_peak_rss_lower,
                                               stats.refine.job_peak_rss_lower, stats.coarsen.job_peak_rss_lower});
  }
};
}  // namespace
