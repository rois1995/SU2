// Owns canonical control slots; never advances their active lifetimes or adds policy.
#pragma once

struct EngineContext;

struct EngineControlPrefix {
  std::optional<EnginePhase> phase;
  std::optional<bool> region_rebuilt;
  std::optional<bool> improvement_changed;
  std::optional<bool> construction_changed;
  std::optional<bool> construction_handoff_progress;
  std::optional<bool> construction_generation_changed;
  std::optional<bool> metric_cleanup;
  std::optional<bool> metric_cleanup_tried;
  std::optional<uint64_t> phase_start_round;
  std::optional<Clock::time_point> started;
  std::optional<std::clock_t> started_cpu;
  std::optional<size_t> atlas_work_initial;
  std::optional<size_t> atlas_scratch;
  std::optional<CLocalFailure> parameter_failure;
  std::optional<size_t> parameter_bound;
  EngineControlPrefix() = default;
  EngineControlPrefix(const EngineControlPrefix&) = delete;
  EngineControlPrefix& operator=(const EngineControlPrefix&) = delete;
  EngineControlPrefix(EngineControlPrefix&&) = delete;
  EngineControlPrefix& operator=(EngineControlPrefix&&) = delete;
};

struct EngineControl {
  std::optional<double> parameter_setup;
  std::optional<bool> standalone_coarsening;
  std::optional<bool> coarsening;
  std::optional<bool> repairing;
  std::optional<MetricChange> construction_kind;
  std::optional<size_t> tracked_bytes_begin;
  std::optional<uint64_t> tracked_calls_begin;
  std::optional<double> tracked_seconds_begin;
  std::optional<uint64_t> election_calls_begin;
  std::optional<double> election_seconds_begin;
  std::optional<const SU2NativeGeometry::MappedFacetReference*> geometry_authority;
  std::optional<SU2NativeGeometry::MappedFacetReference::GeometryStats> geometry_begin;
  std::optional<EngineResult> result;
  std::optional<std::array<GenerationDebt, 4>> generation_window;
  std::optional<size_t> generation_window_count;
  std::optional<CoarseningCheckpoint> checkpoint;
  std::optional<size_t> accepted_bytes;
  std::optional<size_t> private_optimizer_allowance;
  std::optional<size_t> batch_bytes;
  std::optional<size_t> diagnostic_bytes;
  std::optional<std::vector<FixedMetricPoint>> fixed_metric_points;
  std::optional<std::array<Id, 16>> fixed_metric_cells;
  std::optional<bool> fixed_metric_initialized;
  std::optional<bool> fixed_metric_available;
  std::optional<std::string> last_rejection;
  std::optional<std::vector<RejectionDiagnostic>> diagnostic_rejections;
  std::optional<uint64_t> diagnostic_sequence;
  std::optional<uint64_t> progress_at_window;
  std::optional<uint64_t> attempted_at_window;
  std::optional<uint64_t> deferred_at_window;
  std::optional<bool> observe_rejections;
  std::optional<bool> observe_state;
  std::optional<bool> observe_stage;
  std::optional<bool> stage_storage_admitted;
  std::optional<bool> stage_observer_failed;
  std::optional<bool> construction_entry_reported;
  std::optional<QualityRepairStats> previous_quality;
  std::optional<StageProgress> previous_quality_census;
  std::optional<double> previous_quality_elapsed;
  std::optional<double> previous_quality_collective;
  std::optional<uint64_t> quality_census_serial;
  std::optional<uint64_t> previous_quality_construction;
  std::optional<uint64_t> previous_quality_ownership;
  std::optional<bool> quality_census_started;
  std::optional<bool> transaction_captured;
  std::optional<bool> layer_active;
  std::optional<bool> region_active;
  std::optional<Clock::time_point> layer_started;
  std::optional<Clock::time_point> region_started;
  std::optional<bool> ready;
  size_t SelectionBound(EngineContext& context, bool repairs);
  EngineControl() = default;
  EngineControl(const EngineControl&) = delete;
  EngineControl& operator=(const EngineControl&) = delete;
  EngineControl(EngineControl&&) = delete;
  EngineControl& operator=(EngineControl&&) = delete;
  static bool ObserveProgress(EngineContext&, Owned&, SU2NativeGeometry::AtlasBudget&, const char* stage);
  static EngineResult Finish(EngineContext&, bool curved, Owned&, SU2NativeGeometry::AtlasBudget&,
                             const Clock::time_point&, const std::clock_t&);
  static void ProveEntitySurface(EngineContext&, Owned&, SU2NativeGeometry::AtlasBudget&);
  static void ProvePreparedSurface(EngineContext&, Owned&);
  static void ProveFinalHeights(EngineContext&, Owned&);
  static void ObserveRejectedState(EngineContext&, Owned&);
  static void CaptureFinalCounts(EngineContext&, Owned&);
  static void CaptureDirectoryDonorStatistics(EngineContext&, DirectoryStats&, SU2NativeGeometry::AtlasBudget&);
  static void EnrichFinalReasonAndTiming(EngineContext&, bool curved, Owned&, const Clock::time_point&,
                                         const std::clock_t&);
  static EngineResult EmitFinalRowsAndMove(EngineContext&, Owned&, SU2NativeGeometry::AtlasBudget&,
                                           const Clock::time_point&, const std::clock_t&);
  static bool NotifyStageObserver(EngineContext&, const char*, Owned&);
};
