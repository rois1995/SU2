// Trial helpers remain in the original strict-arithmetic translation unit.
#include "EngineTrial.hpp"
#include "EngineLegacyWall.hpp"

bool EngineTrial::HeightCheck(EngineContext& context, const Patch& patch, std::string& reason, bool provisional,
                              FirstHeightReport* measured) {
  auto& options = context.options;
  const auto& geometry = context.geometry;
  auto& stats = context.control.result->stats;

  if (options.wall_heights.empty()) return true;
  ++stats.height_checks;
  FirstHeightReport report;
  const bool valid = CheckFirstHeight(patch, options.wall_heights, geometry, report, reason);
  if (measured) *measured = report;
  stats.height_rejected += report.height_failures || report.missing_cells;
  stats.footprint_rejected += report.footprint_failures != 0;
  if (provisional && GetShapeAdmission() == ShapeAdmission::ValidityOnly && report.measured &&
      !report.reference_clearance_failures && !report.missing_cells) {
    reason.clear();
    return true;
  }
  return valid;
}

bool EngineTrial::GeometryTrial(EngineContext& context, bool valid) {
  auto& stats = context.control.result->stats;
  auto& wall_counters = *context.setup.wall_counters;

  ++stats.geometry_trials;
  stats.geometry_rejected += !valid;
  wall_counters.ObserveGeometry(valid);
  return valid;
}

bool EngineTrial::MetricTrial(EngineContext& context, bool valid, MetricDiagnosis& diagnosis, MetricChange kind,
                              bool surface_geometry_progress) {
  auto& surface_target = context.surface_target;
  auto& bidirectional = context.bidirectional;
  auto& repairing = *context.control.repairing;
  auto& selection = *context.census.selection;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& wall_counters = *context.setup.wall_counters;

  const bool previously_valid = valid;
  ++stats.metric_trials;
  if (valid && surface_target && kind == MetricChange::GeometryRepair && !surface_geometry_progress &&
      !diagnosis.size_progress) {
    valid = false;
    diagnosis.failure = MetricFailure::Progress;
    result.reason = "Surface preparation repair removes neither tangential nor geometry demand.";
  }
  if (valid && bidirectional && repairing && kind == MetricChange::Repair) {
    const bool final_shape = diagnosis.shape_measured && diagnosis.minimum_mean_ratio >= MinimumMeanRatio &&
                             diagnosis.minimum_scaled_jacobian >= MinimumScaledJacobian;
    if ((!diagnosis.quality_progress && !final_shape) || diagnosis.added_excess > diagnosis.removed_excess) {
      valid = false;
      diagnosis.failure = MetricFailure::Progress;
      result.reason =
          "Native3D improvement must improve measured shape or reach its final gates without increasing metric excess.";
    }
  }
  if (valid && bidirectional && repairing && selection.geometry_repairs) {
    if (!diagnosis.shape_measured || diagnosis.minimum_mean_ratio < MinimumMeanRatio ||
        diagnosis.minimum_scaled_jacobian < MinimumScaledJacobian ||
        diagnosis.added_excess > diagnosis.removed_excess) {
      valid = false;
      diagnosis.failure = MetricFailure::Progress;
      result.reason = "Native3D geometry repair must preserve closed shape and metric-size gates.";
    }
  }
  if (!valid) {
    stats.metric_shape_rejected += diagnosis.failure == MetricFailure::Shape;
    stats.metric_progress_rejected += diagnosis.failure == MetricFailure::Progress;
    stats.metric_measurement_rejected += diagnosis.failure == MetricFailure::Measurement;
  }
  wall_counters.MetricTrial(previously_valid, valid);
  return valid;
}

void EngineTrial::SurfaceMerit(EngineContext& context, PatchMetric& metric, const Patch& patch) {
  auto& surface_target = context.surface_target;
  auto& options = context.options;
  const auto& geometry = context.geometry;

  if (!surface_target) return;
  std::set<Edge> requested;
  for (const auto& face : patch.surface)
    if (options.wall_heights.count(geometry.Get(face.facet).marker))
      for (unsigned k = 0; k < 3; ++k) {
        Edge edge{face.v[k], face.v[(k + 1) % 3]};
        std::sort(edge.begin(), edge.end());
        requested.insert(edge);
      }
  metric.maximum_length = 0;
  for (auto& edge : metric.edges) {
    if (!requested.count(edge.first)) edge.second.length = 1;  // Neutral merit, not a full-target edge certificate.
    if (edge.second.length > metric.maximum_length) {
      metric.maximum_length = edge.second.length;
      metric.longest_edge = edge.first;
    }
  }
}

Tensor EngineTrial::ComposedSample(EngineContext& context, Point point, Tensor sensor) {
  auto& stats = context.control.result->stats;
  auto& options = context.options;
  auto& composition = context.composition;

  Timer query(stats.field_query_seconds, options.timings, SU2NativeTiming3D::Operation::TargetComposition);
  ++stats.field_requests;
  ++stats.field_misses;
  try {
    if (composition) {
      Timer compose(stats.field_composition_seconds, options.timings, SU2NativeTiming3D::Operation::TargetComposition);
      ++stats.field_compositions;
      sensor = composition(point, sensor);
    }
    ValidateTensor(sensor);
    return sensor;
  } catch (...) {
    ++stats.field_failures;
    throw;
  }
}

// Borrows active trial state for validity, accounting and guidance checks.
struct TrialChecks {
  using Phase = EnginePhase;
  EngineContext& context;
  RoundState& round_state;
  SlotState& slot_state;
  SU2NativeGeometry::AtlasBudget& atlas_budget;
  TrialComm& trial_comm;
  bool curved;

  void NoteLadder(unsigned kind, bool valid, Clock::time_point began, uint64_t geometry_before, uint64_t height_before,
                  size_t work = 0);
  template <class BoundaryReady, class Observe>
  bool GuardedGeometry(BoundaryReady& boundary_replacement_ready, Observe& observe_transaction, bool valid,
                       const char* operation = "geometry",
                       wall_diagnostics::GuardOrigin origin = wall_diagnostics::GuardOrigin::Operator);
  MetricChange RepairKind();
  template <class BoundaryReady, class Observe>
  bool GeometricChange(BoundaryReady& boundary_replacement_ready, Observe& observe_transaction, bool candidate,
                       MetricChange kind);
  bool SkipConstructionGuidance(MetricChange kind);
  void IncompatibleCrease(long double length);
  void ReserveOptimization(bool eligible);
  void ReleaseOptimization();
  bool OptimizePatch(const Patch& candidate, const RegionMetric& target, bool preserve_vertices);
  size_t ReservedRelocations();
  bool MovableStar(const Imported& star, Id vertex);
  size_t AdmittedCells(const std::vector<Id>& movers, const std::vector<Id>& candidates, const ImportedBatch& payload);
  std::pair<size_t, size_t> GeometryCounts(const Patch& patch, std::set<Edge>* failed_edges = nullptr);
  template <class Observe>
  bool MeasurePrivate(Observe& observe_transaction, const std::vector<Cell>& cells, PatchMetric& measured, bool shape,
                      const std::map<Edge, EdgeMeasure>& frozen_reuse, const Patch* surface_context = nullptr);
  bool FetchPrivateBatch(const std::vector<Cell>& cells, std::map<Edge, EdgeMeasure>& reuse, size_t slot, size_t& batch,
                         bool usable, bool measure_shape, CLocalFailure& local, DonorPatch& donors);
  template <class Observe>
  void ProcessPrivateBatch(Observe& observe_transaction, const std::vector<Cell>& cells, PatchMetric& measured,
                           std::map<Edge, EdgeMeasure>& reuse, size_t slot, size_t batch, bool measure_shape,
                           bool shape, const Patch* surface_context, DonorPatch& donors, bool& usable,
                           std::string& rejected);
  void MeasureSurfaceBatch(const std::vector<Cell>& cells, size_t slot, size_t batch, const Patch* surface_context,
                           PatchMetric& planned, FrozenField& target, MetricStats& counted);
  void AddCellMeasurement(const Cell& cell, PatchMetric& measured, std::map<Edge, EdgeMeasure>& reuse,
                          bool measure_shape, bool shape, const PatchMetric& planned, FrozenField& target,
                          MetricStats& counted);
  template <class BoundaryReady, class Observe>
  bool ValidatePrivate(BoundaryReady& boundary_replacement_ready, Observe& observe_transaction,
                       const PatchMetric& before, bool candidate, MetricChange kind, bool no_new_oversized = false,
                       bool strict_size = false, size_t relocated = 0,
                       wall_diagnostics::PatchSource source = wall_diagnostics::PatchSource::Unreported,
                       bool required_split = false);
  bool ValidateMeasuredTrial(bool candidate, MetricChange kind, bool measured, bool no_new_oversized, bool strict_size,
                             size_t relocated, const PatchMetric& before, PatchMetric& metric);
  void ReportPrivateTrialRejection(bool candidate, bool valid, bool completed, CLocalFailure& observed);
};

// Records the ladder disposition at the existing trial boundary.
void TrialChecks::NoteLadder(unsigned kind, bool valid, Clock::time_point began, uint64_t geometry_before,
                             uint64_t height_before, size_t work) {
  auto& stats = context.control.result->stats;
  auto& repairing = *context.control.repairing;
  auto& accepted_ladder = *slot_state.accepted_ladder;
  auto& certified = *slot_state.certified;
  if (!repairing) return;
  auto& counter = stats.quality.ladder.at(kind);
  ++counter.trials;
  counter.certified += valid;
  counter.work += work;
  counter.seconds += std::chrono::duration<double>(Clock::now() - began).count();
  if (valid)
    accepted_ladder = kind;
  else {
    counter.rejected[0] += stats.geometry_rejected > geometry_before;
    counter.rejected[1] += stats.height_rejected + stats.footprint_rejected > height_before;
    counter.rejected[2] += certified.failure == MetricFailure::Shape;
    counter.rejected[3] += certified.failure == MetricFailure::Progress;
    counter.rejected[4] += certified.failure == MetricFailure::Measurement;
    counter.rejected[7] += stats.geometry_rejected == geometry_before &&
                           stats.height_rejected + stats.footprint_rejected == height_before &&
                           certified.failure == MetricFailure::None;
  }
}

// Applies the original short-circuit wall guards and reports their disposition.
template <class BoundaryReady, class Observe>
bool TrialChecks::GuardedGeometry(BoundaryReady& boundary_replacement_ready, Observe& observe_transaction, bool valid,
                                  const char* operation, wall_diagnostics::GuardOrigin origin) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& wall_counters = *context.setup.wall_counters;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& last_geometry_operation = *slot_state.last_geometry_operation;
  last_geometry_operation = operation;
  std::array<bool, 4> guards{valid};
  guards[1] = valid && (!surface_target ||
                        PreservePassingFirstHeights(old, fresh, options.wall_heights, geometry, result.reason));
  guards[2] = guards[1] && EngineTrial::HeightCheck(context, fresh, result.reason, !selection.geometry_repairs);
  guards[3] = guards[2] && boundary_replacement_ready();
  const bool passed = EngineTrial::GeometryTrial(context, guards[3]);
  wall_counters.ObserveGuard(operation, origin, guards[0], guards[1], guards[2], guards[3], result.reason);
  observe_transaction(operation, passed);
  return passed;
}

// Chooses the existing repair requirement from surface, census and height state.
MetricChange TrialChecks::RepairKind() {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  if (surface_target) return MetricChange::GeometryRepair;
  if (selection.geometry_repairs) return MetricChange::GeometryRepair;
  if (selection.size_repairs || GetShapeAdmission() != ShapeAdmission::ValidityOnly || options.wall_heights.empty())
    return MetricChange::Repair;
  const auto before = HeightScore(old, options.wall_heights, geometry),
             after = HeightScore(fresh, options.wall_heights, geometry);
  return std::isfinite(before) && after > before + 1e-6 ? MetricChange::GeometryRepair : MetricChange::Repair;
}

// Entity construction uses validity; it does not use surrogate descent as permission.
bool TrialChecks::SkipConstructionGuidance(MetricChange kind) {
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& phase = *context.setup.control_prefix.phase;
  return !surface_target && geometry.EntityGeometry() && phase == Phase::Construction && kind != MetricChange::Coarsen;
}

// Runs the exact guidance comparison under its existing query and timer scopes.
template <class BoundaryReady, class Observe>
bool TrialChecks::GeometricChange(BoundaryReady& boundary_replacement_ready, Observe& observe_transaction,
                                  bool candidate, MetricChange kind) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& phase = *context.setup.control_prefix.phase;
  auto& wall_counters = *context.setup.wall_counters;
  auto& choice = *round_state.choice;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& geometric_progress = *slot_state.geometric_progress;
  if (!candidate || !curved) return candidate;
  if (SkipConstructionGuidance(kind)) return true;
  D9QueryScope d9_query(SU2NativeD9::Site::Guidance, &geometry);
  ++stats.geometry_guidance_trials;
  wall_counters.GuidanceTrial(choice.geometric);
  Timer guidance_timer(stats.geometry_guidance_seconds, options.timings, SU2NativeTiming3D::Operation::GeometryBound);
  try {
    long double before, after;
    bool complete_guidance = true;
    if (kind == MetricChange::Coarsen) {
      // A coarsening trial needs zero new excess; rejected trials need no old-surface score.
      const auto guidance = GeometryGuidanceExcess(fresh, geometry, options.surface_tolerance, 0);
      stats.geometry_guidance_faces += guidance.evaluated_faces;
      stats.geometry_guidance_early_rejections += !guidance.complete;
      after = guidance.excess;
      if (!guidance.complete || !std::isfinite(after) || after > 0) {
        ++stats.geometry_guidance_rejected;
        wall_counters.GuidanceReject(WallGuidanceCause(guidance.complete, std::isfinite(after), false, false),
                                     choice.geometric);
        geometric_progress = false;
        result.reason = "Native3D candidate does not preserve/improve its frozen geometry error.";
        return GuardedGeometry(boundary_replacement_ready, observe_transaction, false, "geometry",
                               wall_diagnostics::GuardOrigin::Guidance);
      }
      const auto old_guidance = GeometryGuidanceExcess(old, geometry, options.surface_tolerance);
      stats.geometry_guidance_faces += old_guidance.evaluated_faces;
      before = old_guidance.excess;
    } else {
      const auto old_guidance = GeometryGuidanceExcess(old, geometry, options.surface_tolerance);
      stats.geometry_guidance_faces += old_guidance.evaluated_faces;
      before = old_guidance.excess;
      const bool allow_increase =
          kind == MetricChange::GeometryRepair && phase == Phase::Improvement && !selection.geometry_repairs;
      const auto guidance =
          GeometryGuidanceExcess(fresh, geometry, options.surface_tolerance,
                                 allow_increase ? std::numeric_limits<long double>::infinity() : before);
      stats.geometry_guidance_faces += guidance.evaluated_faces;
      stats.geometry_guidance_early_rejections += !guidance.complete;
      after = guidance.excess;
      complete_guidance = guidance.complete;
    }
    geometric_progress = complete_guidance && before > 0 && after < before * (1 - 1e-12L);
    if (!complete_guidance || !std::isfinite(after) || (selection.geometry_repairs && !geometric_progress) ||
        (kind == MetricChange::GeometryRefine && !geometric_progress) || (kind == MetricChange::Coarsen && after > 0) ||
        (kind != MetricChange::GeometryRefine && after > before &&
         !(kind == MetricChange::GeometryRepair && phase == Phase::Improvement))) {
      ++stats.geometry_guidance_rejected;
      wall_counters.GuidanceReject(
          WallGuidanceCause(complete_guidance, std::isfinite(after),
                            selection.geometry_repairs || kind == MetricChange::GeometryRefine, geometric_progress),
          choice.geometric);
      result.reason = "Native3D candidate does not preserve/improve its frozen geometry error.";
      return GuardedGeometry(boundary_replacement_ready, observe_transaction, false, "geometry",
                             wall_diagnostics::GuardOrigin::Guidance);
    }
    return true;
  } catch (const std::exception& e) {
    ++stats.geometry_guidance_rejected;
    wall_counters.GuidanceReject(wall_diagnostics::GuidanceCause::Exception, choice.geometric);
    result.reason = e.what();
    return GuardedGeometry(boundary_replacement_ready, observe_transaction, false, "geometry",
                           wall_diagnostics::GuardOrigin::Guidance);
  }
}

// Reports an exact-facet conflict without changing the admitted edge.
void TrialChecks::IncompatibleCrease(long double length) {
  auto& stats = context.control.result->stats;
  auto& choice = *round_state.choice;
  auto& failure = *slot_state.failure;
  auto& imported = *slot_state.imported;
  std::ostringstream message;
  message << std::setprecision(17) << "Native3D exact-facet resolution conflict on crease edge " << choice.edge[0]
          << "," << choice.edge[1] << ": an interval without an interior binary64 point has frozen metric length "
          << length << " above " << MaximumMetricEdge
          << "; no admissible subdivision meets the current geometry/metric contract.";
  failure.Set(1, choice.edge[0], message.str());
  stats.last_rejected_feature = {{choice.edge[0], choice.edge[1], 0}, 2};
  if (!imported.cells.empty()) stats.last_rejected_cavity_centroid = Centroid(imported.cells.front().cell);
}

// Reserves optimizer work using the original private or collective accounting.
void TrialChecks::ReserveOptimization(bool eligible) {
  auto& stats = context.control.result->stats;
  auto& private_repair = *context.exchange.private_repair;
  auto& private_optimizer_allowance = *context.control.private_optimizer_allowance;
  auto& repair_budget = *slot_state.repair_budget;
  auto& repair_work = *slot_state.repair_work;
  if (private_repair) {
    repair_budget = eligible ? private_optimizer_allowance : repair_work;
    return;
  }
  const auto count = trial_comm.Reduce(uint64_t(eligible), CPassiveComm::Op::SUM);
  const auto budget = count ? std::min<uint64_t>(160, (stats.work_limit - stats.global_work) / count) : 0;
  const auto extra = eligible ? std::min<uint64_t>(budget, 160 - repair_work) : 0;
  stats.global_work += trial_comm.Reduce(uint64_t(extra), CPassiveComm::Op::SUM);
  repair_budget = repair_work + extra;
}

// Returns unused optimizer work through the existing trial reduction.
void TrialChecks::ReleaseOptimization() {
  auto& stats = context.control.result->stats;
  auto& private_repair = *context.exchange.private_repair;
  auto& repair_budget = *slot_state.repair_budget;
  auto& repair_work = *slot_state.repair_work;
  if (private_repair) {
    repair_budget = repair_work;
    return;
  }
  stats.global_work -= trial_comm.Reduce(uint64_t(repair_budget - repair_work), CPassiveComm::Op::SUM);
  repair_budget = repair_work;
}

// Records the optimizer counters at the existing successful-return boundary.
static void AccumulateOptimization(EngineStats& stats, const OptimizationOperations& repair_optimization) {
  stats.boundary_smoothing_stars += repair_optimization.boundary_stars;
  stats.boundary_smoothing_reused += repair_optimization.boundary_reused;
  stats.boundary_smoothing_candidates += repair_optimization.boundary_candidates;
  stats.boundary_smoothing_validity_rejects += repair_optimization.boundary_validity;
  stats.boundary_smoothing_shape_rejects += repair_optimization.boundary_shape;
  stats.boundary_smoothing_edge_rejects += repair_optimization.boundary_edge;
  stats.boundary_smoothing_geometry_rejects += repair_optimization.boundary_geometry;
  stats.boundary_smoothing_guidance_calls += repair_optimization.boundary_guidance;
  stats.boundary_smoothing_move_contexts += repair_optimization.boundary_move_contexts;
  stats.boundary_smoothing_prepared_trials += repair_optimization.boundary_prepared_trials;
  stats.boundary_smoothing_guidance_faces += repair_optimization.boundary_guidance_faces;
  stats.boundary_smoothing_guidance_reused_faces += repair_optimization.boundary_guidance_reused_faces;
  stats.boundary_smoothing_guidance_early_rejections += repair_optimization.boundary_guidance_early;
  stats.boundary_smoothing_upper_queries += repair_optimization.boundary_upper;
  stats.boundary_smoothing_moves += repair_optimization.boundary_moves;
  stats.boundary_smoothing_debt_reduction += repair_optimization.boundary_debt;
  stats.boundary_smoothing_seconds += repair_optimization.boundary_seconds;
  stats.interior_smoothing_stars += repair_optimization.interior_stars;
  stats.interior_smoothing_reused += repair_optimization.interior_reused;
  stats.interior_smoothing_candidates += repair_optimization.interior_candidates;
  stats.interior_smoothing_validity_rejects += repair_optimization.interior_validity;
  stats.interior_smoothing_shape_rejects += repair_optimization.interior_shape;
  stats.interior_smoothing_edge_rejects += repair_optimization.interior_edge;
  stats.interior_smoothing_geometry_rejects += repair_optimization.interior_geometry;
  stats.interior_smoothing_guidance_calls += repair_optimization.interior_guidance;
  stats.interior_smoothing_upper_queries += repair_optimization.interior_upper;
  stats.interior_smoothing_moves += repair_optimization.interior_moves;
  stats.interior_smoothing_debt_reduction += repair_optimization.interior_debt;
  stats.interior_smoothing_seconds += repair_optimization.interior_seconds;
}

// Optimizes the patch within the original local work, ID and deadline bounds.
bool TrialChecks::OptimizePatch(const Patch& candidate, const RegionMetric& target, bool preserve_vertices) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& started = *context.setup.control_prefix.started;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& fresh = *slot_state.fresh;
  auto& repair_budget = *slot_state.repair_budget;
  auto& repair_work = *slot_state.repair_work;
  auto& optimization_approved = *slot_state.optimization_approved;
  auto& repair_optimization = *slot_state.repair_optimization;
  auto& offset = *slot_state.offset;
  auto& node_offset = *slot_state.node_offset;
  auto& construction_nodes = *slot_state.construction_nodes;
  optimization_approved = false;
  repair_optimization = {};
  const auto reserved = repair_budget - repair_work;
  if (!reserved) return false;
  const double remaining = options.maximum_seconds
                               ? options.maximum_seconds - std::chrono::duration<double>(Clock::now() - started).count()
                               : 0;
  if (options.maximum_seconds && remaining <= 0) return false;
  const Id first_node = maximum_node + 1 + node_offset + construction_nodes + repair_work;
  size_t used = reserved;
  repair_work += reserved;
  ++stats.repair_proposals;
  ++stats.optimization_trials;
  const bool changed = OptimizedRepairPatch(candidate, maximum_cell + 1 + offset, first_node, target, fresh,
                                            result.reason, used, preserve_vertices, geometry, options.wall_heights,
                                            options.surface_tolerance, repair_optimization, remaining);
  repair_work -= reserved - used;
  AccumulateOptimization(stats, repair_optimization);
  return changed;
}

// Counts new apex identities from the existing reserved range.
size_t TrialChecks::ReservedRelocations() {
  auto& maximum_node = *context.commit.maximum_node;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& node_offset = *slot_state.node_offset;
  auto& construction_nodes = *slot_state.construction_nodes;
  std::set<Id> previous, ids;
  for (const auto& c : old.cells)
    for (auto n : c.v) previous.insert(n.id);
  for (const auto& c : fresh.cells)
    for (auto n : c.v)
      if (n.id >= maximum_node + 2 + node_offset && n.id < maximum_node + 1 + node_offset + construction_nodes &&
          !previous.count(n.id))
        ids.insert(n.id);
  return ids.size();
}

// Tests the original mobility rule for an imported vertex star.
bool TrialChecks::MovableStar(const Imported& star, Id vertex) {
  auto& geometry = context.geometry;
  auto& choice = *round_state.choice;
  std::set<Id> components;
  for (const auto& row : star.cells)
    for (unsigned f = 0; f < 4; ++f)
      if (row.physical[f].present && row.cell.v[f].id != vertex)
        components.insert(geometry.Canonical(row.physical[f].facet));
  if (choice.geometric)
    return components.empty();    // Geometry placement moves free volume support, not its prescribed surface.
  return components.size() <= 1;  // Smooth physical vertices share the same complete-star admission as interior points.
}

// Counts unique support cells for the proposed movers in the original patch.
size_t TrialChecks::AdmittedCells(const std::vector<Id>& movers, const std::vector<Id>& candidates,
                                  const ImportedBatch& payload) {
  auto& choice = *round_state.choice;
  auto& old = *slot_state.old;
  std::set<Id> cells;
  for (const auto& c : old.cells)
    if (std::count_if(c.v.begin(), c.v.end(),
                      [&](Node n) { return n.id == choice.edge[0] || n.id == choice.edge[1]; }) == 2)
      cells.insert(c.id);
  for (size_t i = 0; i < candidates.size(); ++i)
    if (std::find(movers.begin(), movers.end(), candidates[i]) != movers.end())
      for (const auto& row : payload.candidates.at(i).cells) cells.insert(row.cell.id);
  return cells.size();
}

// Counts faces missing an exact certificate using the same local atlas deadline.
std::pair<size_t, size_t> TrialChecks::GeometryCounts(const Patch& patch, std::set<Edge>* failed_edges) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& stats = context.control.result->stats;
  Timer timer(stats.geometry_guidance_seconds, options.timings, SU2NativeTiming3D::Operation::GeometryBound);
  auto parameters = geometry.PrivateParameters(patch);
  const auto& reference = parameters ? *parameters : geometry;
  std::map<Face, Id> facets;
  for (const auto& face : patch.surface) facets.emplace(Key(face.v), face.facet);
  std::pair<size_t, size_t> counts{};
  for (const auto& cell : patch.cells) {
    atlas_budget.CheckTime();
    bool bad = false;
    for (const auto& face : Faces(cell)) {
      const auto found = facets.find(Key(face));
      if (found == facets.end()) continue;
      std::array<Point, 3> triangle;
      for (unsigned k = 0; k < 3; ++k)
        for (auto n : cell.v)
          if (n.id == face[k]) triangle[k] = n.p;
      const auto error =
          reference.SurfaceDeviation(found->second, triangle, SU2NativeGeometry::SurfaceQueryNeed::Classify());
      ++stats.geometry_guidance_faces;
      if (!std::isfinite(error)) throw std::runtime_error("Native3D coupled repair lacks a complete face certificate.");
      if (error <= options.surface_tolerance) continue;
      bad = true;
      ++counts.second;
      if (failed_edges)
        for (unsigned k = 0; k < 3; ++k) {
          Edge edge{face[k], face[(k + 1) % 3]};
          std::sort(edge.begin(), edge.end());
          failed_edges->insert(edge);
        }
    }
    counts.first += bad;
  }
  return counts;
}

// Maximum number of cells per private donor batch, before the other existing bounds.
constexpr size_t MeasurementBatch = 16;

// Measures the exact private patch over the original globally agreed donor batches.
template <class Observe>
bool TrialChecks::MeasurePrivate(Observe& observe_transaction, const std::vector<Cell>& cells, PatchMetric& measured,
                                 bool shape, const std::map<Edge, EdgeMeasure>& frozen_reuse,
                                 const Patch* surface_context) {
  auto& result = *context.control.result;
  auto& phase = *context.setup.control_prefix.phase;
  auto& surface_target = context.surface_target;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& measured_flags = *slot_state.measured_flags;
  if (repairing) measured_flags.clear();
  CLocalFailure local;
  bool usable = true;
  std::string rejected;
  std::map<Edge, EdgeMeasure> reuse;
  try {
    reuse = frozen_reuse;
  } catch (const std::exception& e) {
    local.Set(2, 0, e.what());
  }
  if (!(ready = context.exchange.Vote(context, local))) return false;
  size_t count = trial_comm.Reduce(cells.size(), CPassiveComm::Op::MAX);
  measured = {};
  const bool measure_shape = !surface_target && phase == Phase::Improvement;
  if (measure_shape) {
    measured.minimum_mean_ratio = measured.minimum_scaled_jacobian = INFINITY;
    measured.shape_measured = true;
    if (!cells.empty()) measured.weakest_cell = measured.sliver_cell = cells.front().id;
  }
  for (size_t slot = 0; slot < count;) {
    size_t batch = std::min({MeasurementBatch, MaximumCavityCells / 6, count - slot});
    DonorPatch donors;

    if (!FetchPrivateBatch(cells, reuse, slot, batch, usable, measure_shape, local, donors)) return false;
    if (usable && slot < cells.size())
      ProcessPrivateBatch(observe_transaction, cells, measured, reuse, slot, batch, measure_shape, shape,
                          surface_context, donors, usable, rejected);
    slot += batch;
  }
  if (usable && surface_target && surface_context) EngineTrial::SurfaceMerit(context, measured, *surface_context);
  if (!usable) result.reason = rejected;
  return usable;
}

// Retries the original donor import with the same batch halving and failure votes.
bool TrialChecks::FetchPrivateBatch(const std::vector<Cell>& cells, std::map<Edge, EdgeMeasure>& reuse, size_t slot,
                                    size_t& batch, bool usable, bool measure_shape, CLocalFailure& local,
                                    DonorPatch& donors) {
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& surface_target = context.surface_target;
  auto& stats = context.control.result->stats;
  auto& directory = *context.exchange.directory;
  auto& eligible_extensions = *context.census.eligible_extensions;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& field_identity = *context.census.field_identity;
  auto& ready = *context.control.ready;
  auto& private_scratch = *slot_state.private_scratch;
  for (;;) {
    std::vector<Cell> query;
    try {
      if (usable)
        for (size_t i = slot; i < std::min(slot + batch, cells.size()); ++i) query.push_back(cells[i]);
    } catch (const std::exception& e) {
      local.Set(2, 0, e.what());
    }
    std::vector<unsigned char> requests;
    try {
      requests = MetricEdgeRequests(query, reuse, field_identity, &campaign_edges);
      if (surface_target) std::fill(requests.begin(), requests.end(), static_cast<unsigned char>(63));
    } catch (const std::exception& e) {
      local.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, local))) return false;
    {
      Timer timer(stats.donor_seconds, options.timings, SU2NativeTiming3D::Operation::Import);
      ++stats.donor_batches;
      donors = trial_comm.ImportSamples(
          query, eligible_extensions, options.memory_ceiling,
          transfer_memory::Add(context.exchange.Bytes(context), directory->ResidentBytes(), private_scratch), requests,
          measure_shape);
    }
    if (donors.valid) break;
    if (!DonorCap(donors.reason) || batch == 1) {
      ready = false;
      result.reason = donors.reason;
      return false;
    }
    batch = (batch + 1) / 2;
    ++stats.batch_retries;
  }
  return true;
}

// Measures one subscribed donor batch and preserves its original timers and field accounting.
template <class Observe>
void TrialChecks::ProcessPrivateBatch(Observe& observe_transaction, const std::vector<Cell>& cells,
                                      PatchMetric& measured, std::map<Edge, EdgeMeasure>& reuse, size_t slot,
                                      size_t batch, bool measure_shape, bool shape, const Patch* surface_context,
                                      DonorPatch& donors, bool& usable, std::string& rejected) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& composition = context.composition;
  auto& stats = context.control.result->stats;
  auto& field_identity = *context.census.field_identity;
  Timer timer(stats.metric_seconds, options.timings, SU2NativeTiming3D::Operation::MetricNormalization);
  CpuTimer cpu(stats.metric_cpu_seconds, options.timings);
  MetricStats counted;
  std::unique_ptr<FrozenField> target;
  try {
    observe_transaction("metric_donors", donors.valid, &donors);
    const auto components = AssociatedExtensions(donors.cells, geometry);
    observe_transaction("support_donors", donors.valid, &donors);
    target = std::make_unique<FrozenField>(std::move(donors.cells), composition, options.timings, components,
                                           options.edge_breakpoints, field_identity, true, MaximumDonors, true);
    PatchMetric planned;

    MeasureSurfaceBatch(cells, slot, batch, surface_context, planned, *target, counted);
    for (size_t i = slot; i < std::min(slot + batch, cells.size()); ++i)
      AddCellMeasurement(cells[i], measured, reuse, measure_shape, shape, planned, *target, counted);
  } catch (const std::exception& e) {
    usable = false;
    rejected = e.what();
  }
  if (target) FieldCounts(stats, *target);
  stats.private_edge_evaluations += counted.edge_evaluations;
  stats.private_edge_reused += counted.edge_reused;
  stats.private_edge_campaign_hits += counted.edge_campaign_reused;
}

// Builds only the surface faces and cells of the current donor batch.
void TrialChecks::MeasureSurfaceBatch(const std::vector<Cell>& cells, size_t slot, size_t batch,
                                      const Patch* surface_context, PatchMetric& planned, FrozenField& target,
                                      MetricStats& counted) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  if (surface_target) {
    if (!surface_context) throw std::logic_error("Surface planning lacks its current private cavity.");
    Patch local_surface;
    std::set<Face> faces;
    local_surface.parameters = surface_context->parameters;
    local_surface.coverage = surface_context->coverage;
    for (size_t i = slot; i < std::min(slot + batch, cells.size()); ++i) {
      local_surface.cells.push_back(cells[i]);
      for (auto face : Faces(cells[i])) faces.insert(Key(face));
    }
    for (const auto& face : surface_context->surface)
      if (faces.count(Key(face.v))) local_surface.surface.push_back(face);
    // Guidance is cell-independent; use the same admitted source batch and current private associations once.
    planned = MeasureSurfacePlanning(local_surface, target, geometry, options.wall_heights, surface_target, &counted);
  }
}

// Folds the original per-cell metric into the patch measure and edge reuse map.
void TrialChecks::AddCellMeasurement(const Cell& cell, PatchMetric& measured, std::map<Edge, EdgeMeasure>& reuse,
                                     bool measure_shape, bool shape, const PatchMetric& planned, FrozenField& target,
                                     MetricStats& counted) {
  auto& surface_target = context.surface_target;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& repairing = *context.control.repairing;
  auto& certified = *slot_state.certified;
  auto& measured_flags = *slot_state.measured_flags;
  PatchMetric part;
  if (surface_target) {
    for (unsigned a = 0; a < 4; ++a)
      for (unsigned b = a + 1; b < 4; ++b) {
        Edge edge{cell.v[a].id, cell.v[b].id};
        std::sort(edge.begin(), edge.end());
        part.edges.emplace(edge, planned.edges.at(edge));
      }
  } else
    part = measure_shape ? MeasureMetricCells({cell}, target, &counted, &reuse, shape, shape ? &certified : nullptr,
                                              &campaign_edges)
                         : MeasureMetricEdges({cell}, target, &counted, &reuse, &campaign_edges);
  if (measure_shape && part.minimum_mean_ratio < measured.minimum_mean_ratio) {
    measured.minimum_mean_ratio = part.minimum_mean_ratio;
    measured.weakest_cell = part.weakest_cell;
  }
  if (measure_shape && part.minimum_scaled_jacobian < measured.minimum_scaled_jacobian) {
    measured.minimum_scaled_jacobian = part.minimum_scaled_jacobian;
    measured.sliver_cell = part.sliver_cell;
  }
  measured.shape_cells += part.shape_cells;
  measured.shape_debt += part.shape_debt;
  measured.q_bad_cells += part.q_bad_cells;
  measured.j_bad_cells += part.j_bad_cells;
  if (repairing && part.shape_measured)
    measured_flags.push_back({cell.id, part.q_bad_cells != 0, part.j_bad_cells != 0});
  for (const auto& edge : part.edges) {
    measured.edges.emplace(edge);
    reuse[edge.first] = edge.second;
    if (edge.second.length > measured.maximum_length) {
      measured.maximum_length = edge.second.length;
      measured.longest_edge = edge.first;
    }
  }
}

// Validates a trial while keeping its candidate scope around measurement, diagnostics and the existing observer vote.
template <class BoundaryReady, class Observe>
bool TrialChecks::ValidatePrivate(BoundaryReady& boundary_replacement_ready, Observe& observe_transaction,
                                  const PatchMetric& before, bool candidate, MetricChange kind, bool no_new_oversized,
                                  bool strict_size, size_t relocated, wall_diagnostics::PatchSource source,
                                  bool required_split) {
  auto& result = *context.control.result;
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& bidirectional = context.bidirectional;
  auto& stats = context.control.result->stats;
  auto& wall_counters = *context.setup.wall_counters;
  auto& concurrent = *round_state.concurrent;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& observe_rejections = *context.control.observe_rejections;
  auto& fresh = *slot_state.fresh;
  auto& approved = *slot_state.approved;
  auto& geometric_progress = *slot_state.geometric_progress;
  auto& certified = *slot_state.certified;
  auto& measured_flags = *slot_state.measured_flags;
  auto& fresh_flags = *slot_state.fresh_flags;
  const bool source_available = bool(surface_target) && !repairing && !coarsening && candidate;
  const auto source_rejected_before = stats.geometry_guidance_rejected;
  SU2NativeTiming3D::Scope candidate_timer(SU2NativeTiming3D::Operation::Candidate);
  const bool completed = concurrent && approved && !candidate;
  const auto retained = certified;
  certified = {};
  const bool merit_first =
      !surface_target && bidirectional && repairing && kind == MetricChange::Repair && !selection.geometry_repairs;
  if (required_split) geometric_progress = false;
  if (!merit_first && !required_split)
    candidate = GeometricChange(boundary_replacement_ready, observe_transaction, candidate,
                                selection.geometry_repairs ? MetricChange::GeometryRepair : kind);
  PatchMetric metric;
  const std::vector<Cell> empty;
  bool measured =
      MeasurePrivate(observe_transaction, candidate ? fresh.cells : empty, metric, true, before.edges, &fresh);
  CLocalFailure observed;
  bool valid = false;

  valid = ValidateMeasuredTrial(candidate, kind, measured, no_new_oversized, strict_size, relocated, before, metric);
  if (valid && merit_first && !required_split)
    valid = GeometricChange(boundary_replacement_ready, observe_transaction, true, RepairKind());
  if (valid && selection.geometry_repairs) valid = EngineTrial::HeightCheck(context, fresh, result.reason);
  // The initial streaming probe precedes repair proposals; it is not an operator failure.

  ReportPrivateTrialRejection(candidate, valid, completed, observed);
  try {
    observe_transaction("metric", valid);
  } catch (const std::exception& e) {
    observed.Set(2, 0, e.what());
  }
  if (observe_rejections || (!repairing && !coarsening)) ready = context.exchange.Vote(context, observed);
  if (completed) certified = retained;
  if (repairing && valid) fresh_flags = measured_flags;
  wall_counters.ObserveSource(source, source_available, stats.geometry_guidance_rejected > source_rejected_before,
                              valid);
  return valid;
}

// Runs the original metric gates with the same normalization and CPU scopes.
bool TrialChecks::ValidateMeasuredTrial(bool candidate, MetricChange kind, bool measured, bool no_new_oversized,
                                        bool strict_size, size_t relocated, const PatchMetric& before,
                                        PatchMetric& metric) {
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& bidirectional = context.bidirectional;
  auto& stats = context.control.result->stats;
  auto& wall_counters = *context.setup.wall_counters;
  auto& metric_cleanup = *context.setup.control_prefix.metric_cleanup;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& geometric_progress = *slot_state.geometric_progress;
  auto& certified = *slot_state.certified;
  auto& coordinate_only = *slot_state.coordinate_only;
  bool valid = false;
  {
    Timer timer(stats.metric_seconds, options.timings, SU2NativeTiming3D::Operation::MetricNormalization);
    CpuTimer cpu(stats.metric_cpu_seconds, options.timings);
    if (candidate) {
      const auto effective = kind == MetricChange::Repair
                                 ? (geometric_progress && !bidirectional ? MetricChange::GeometryRepair : RepairKind())
                                 : kind;
      if (measured)
        valid =
            metric_cleanup
                ? ValidateMetricCleanup(old.cells.size(), before, fresh.cells.size(), metric, result.reason, &certified)
                : wall_counters.MetricValidation(ValidateMetricChange(old.cells.size(), before, fresh.cells.size(),
                                                                      metric, effective, result.reason, &certified,
                                                                      relocated, coordinate_only));
      if (valid && no_new_oversized)
        for (const auto& edge : metric.edges)
          if (edge.second.length > MaximumMetricEdge) {
            const auto previous = before.edges.find(edge.first);
            auto same = [](Point a, Point b) { return a.x == b.x && a.y == b.y && a.z == b.z; };
            if (previous == before.edges.end() || !same(previous->second.a, edge.second.a) ||
                !same(previous->second.b, edge.second.b)) {
              valid = false;
              certified.failure = MetricFailure::Progress;
              result.reason = "Native3D joint strip introduces a new oversized edge.";
              break;
            }
          }
      if (valid && strict_size && !certified.size_progress) {
        valid = false;
        certified.failure = MetricFailure::Progress;
        result.reason = "Native3D compound wall repair does not reduce frozen changed-edge excess.";
      }
      valid = EngineTrial::MetricTrial(context, valid, certified, effective, geometric_progress);
    }
  }
  return valid;
}

// Reports a failed private trial with the original first-capture and rejected-trial callbacks.
void TrialChecks::ReportPrivateTrialRejection(bool candidate, bool valid, bool completed, CLocalFailure& observed) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& working = *context.ownership.working;
  auto& composition = context.composition;
  auto& stats = context.control.result->stats;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& round = round_state.round;
  auto& choice = *round_state.choice;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& last_rejection = *context.control.last_rejection;
  auto& transaction_captured = *context.control.transaction_captured;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& chosen = *slot_state.chosen;
  auto& point = *slot_state.point;
  auto& certified = *slot_state.certified;
  auto& imported = *slot_state.imported;
  auto& dependencies = *slot_state.dependencies;
  auto& last_geometry_operation = *slot_state.last_geometry_operation;
  auto& transaction_feature = *slot_state.transaction_feature;
  auto& node_offset = *slot_state.node_offset;
  auto& offset = *slot_state.offset;
  if (chosen && !valid && !completed && (!repairing || last_geometry_operation)) {
    stats.last_metric_rejection = certified;
    stats.last_rejected_feature = repairing ? choice.feature : Feature{{choice.edge[0], choice.edge[1], 0}, 2};
    if (!old.cells.empty()) stats.last_rejected_cavity_centroid = Centroid(old.cells.front());
    last_rejection = "feature " + std::to_string(stats.last_rejected_feature.count) + ":" +
                     std::to_string(stats.last_rejected_feature.vertices[0]) + ": " + result.reason;
    if (options.transaction_observer && !options.capture_feature.count && !transaction_captured) try {
        transaction_captured = true;
        const std::string stage = repairing    ? "first_" + std::string(last_geometry_operation) + "_failure"
                                  : coarsening ? "first_collapse_failure"
                                               : "first_split_failure";
        options.transaction_observer({stage.c_str(), transaction_feature, uint64_t(round), true, false, working,
                                      geometry, dependencies, imported, old, fresh, point, maximum_cell + 1 + offset,
                                      maximum_node + 1 + node_offset, certified, result.reason, nullptr, composition,
                                      options.edge_breakpoints});
      } catch (const std::exception& e) {
        observed.Set(2, 0, e.what());
      }
    if (options.rejected_trial) try {
        options.rejected_trial(old, fresh, stats.last_rejected_feature, certified, candidate, result.reason);
      } catch (const std::exception& e) {
        observed.Set(2, 0, e.what());
      }
  }
}

// Borrows an evaluated patch and its existing callbacks for the bounded repair ladders.
template <class BoundaryReady, class Observe>
struct RepairLadders {
  EngineContext& context;
  RoundState& round_state;
  SlotState& slot_state;
  TrialComm& trial_comm;
  TrialChecks& trial_checks;
  BoundaryReady& boundary_replacement_ready;
  Observe& observe_transaction;
  const PatchMetric& before;
  const std::vector<Cell>& empty;
  bool curved;

  RepairLadders(EngineContext& context, RoundState& round_state, SlotState& slot_state, TrialComm& trial_comm,
                TrialChecks& trial_checks, BoundaryReady& boundary_replacement_ready, Observe& observe_transaction,
                const PatchMetric& before, const std::vector<Cell>& empty, bool curved)
      : context(context),
        round_state(round_state),
        slot_state(slot_state),
        trial_comm(trial_comm),
        trial_checks(trial_checks),
        boundary_replacement_ready(boundary_replacement_ready),
        observe_transaction(observe_transaction),
        before(before),
        empty(empty),
        curved(curved) {}

  bool Run(bool metric_needed, bool old_measured);
  bool MoveCentre(const std::map<Id, Node>& nodes, bool fallback, bool move, Id vertex, Point& moveCenter);
  bool SampleMoveMidpoints(const std::map<Id, Node>& nodes, const std::vector<Id>& query_ids, Point origin,
                           size_t count, std::map<Id, Tensor>& samples);
  bool OptimizeRepair();
  bool FallbackLadder(const std::map<Id, Node>& nodes, bool fallback, bool move, Id vertex, Point moveCenter);
  bool ProposeFallback(const std::map<Id, Node>& nodes, bool move, Id vertex, Point moveCenter, unsigned trial,
                       bool move_trial);
  bool ProposeMove(const std::map<Id, Node>& nodes, Id vertex, Point moveCenter, unsigned trial, Id first);
  bool GeometryRepairSplits();
  bool TryGeometrySplit(const Edge& edge, const std::map<Id, Node>& nodes, const std::pair<size_t, size_t>& old_counts,
                        FrozenField& target);
  bool RecloseGeometrySplit(bool& candidate, bool& valid, const std::pair<size_t, size_t>& old_counts,
                            FrozenField& target);
};

// Prepares fallback nodes before the bounded move, topology and geometry ladders.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::Run(bool metric_needed, bool old_measured) {
  auto& options = context.options;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& choice = *round_state.choice;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& old = *slot_state.old;
  auto& failure = *slot_state.failure;
  auto& repair_approved = *slot_state.repair_approved;
  const bool fallback = metric_needed && old_measured && repairing && !selection.size_repairs;
  if (fallback && !repairing) ++stats.repair_fallbacks;
  const bool move = repairing ? choice.feature.count == 1 : coarsening;
  const Id vertex = repairing ? choice.feature.vertices[0] : choice.edge[1];
  std::map<Id, Node> nodes;
  {
    Timer timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
    try {
      if (fallback)
        for (const auto& cell : old.cells)
          for (auto node : cell.v) nodes.emplace(node.id, node);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    ready = context.exchange.Vote(context, failure);
  }
  if (!ready) return false;
  Point moveCenter{};

  if (!MoveCentre(nodes, fallback, move, vertex, moveCenter) ||
      !FallbackLadder(nodes, fallback, move, vertex, moveCenter) || !GeometryRepairSplits())
    return false;
  if (selection.size_repairs && !OptimizeRepair()) return false;
  if (repair_approved && !repairing) ++stats.fallback_accepted;
  return true;
}

// Computes the same metric mean for the participating repair vertices.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::MoveCentre(const std::map<Id, Node>& nodes, bool fallback, bool move,
                                                       Id vertex, Point& moveCenter) {
  auto& composition = context.composition;
  auto& ready = *context.control.ready;
  auto& old = *slot_state.old;
  auto& approved = *slot_state.approved;
  auto& coordinate_only = *slot_state.coordinate_only;
  auto& failure = *slot_state.failure;
  if (composition && trial_comm.Maximum(fallback && move && !approved)) {
    const bool participating = fallback && move && !approved;
    std::set<Id> neighbors;
    std::map<Id, Tensor> samples;
    Point origin{};
    try {
      if (participating) {
        origin = nodes.at(vertex).p;
        for (const auto& f : old.surface)
          if (std::find(f.v.begin(), f.v.end(), vertex) != f.v.end()) neighbors.insert(f.v.begin(), f.v.end());
        if (neighbors.empty())
          for (const auto& n : nodes) neighbors.insert(n.first);
        neighbors.erase(vertex);
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, failure))) return false;
    std::vector<Id> query_ids;
    try {
      query_ids.assign(neighbors.begin(), neighbors.end());
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, failure))) return false;
    const auto count = trial_comm.Reduce(query_ids.size(), CPassiveComm::Op::MAX);

    if (!SampleMoveMidpoints(nodes, query_ids, origin, count, samples)) return false;
    try {
      if (participating && !neighbors.empty()) {
        std::vector<std::pair<Point, Tensor>> values;
        for (auto id : neighbors) values.emplace_back(nodes.at(id).p, samples.at(id));
        moveCenter = MetricMeanPoint(origin, values, coordinate_only ? 4 * old.cells.size() : 4 * MaximumCavityCells);
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, failure))) return false;
  }
  return true;
}

// Imports midpoint samples in the original batches and retains both votes per batch.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::SampleMoveMidpoints(const std::map<Id, Node>& nodes,
                                                                const std::vector<Id>& query_ids, Point origin,
                                                                size_t count, std::map<Id, Tensor>& samples) {
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& directory = *context.exchange.directory;
  auto& eligible_extensions = *context.census.eligible_extensions;
  auto& ready = *context.control.ready;
  auto& private_scratch = *slot_state.private_scratch;
  auto& failure = *slot_state.failure;
  for (size_t at = 0; at < count; at += MaximumCavityCells) {
    std::vector<std::array<Point, 2>> queries;
    try {
      for (size_t i = at; i < std::min(at + MaximumCavityCells, query_ids.size()); ++i)
        queries.push_back({origin, nodes.at(query_ids[i]).p});
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, failure))) return false;
    DonorSamples answer;
    {
      Timer timer(stats.donor_seconds, options.timings, SU2NativeTiming3D::Operation::Import);
      ++stats.donor_batches;
      answer = trial_comm.SampleMidpoints(
          queries, eligible_extensions, options.memory_ceiling,
          transfer_memory::Add(context.exchange.Bytes(context), directory->ResidentBytes(), private_scratch));
    }
    if (!answer.valid) {
      result.reason = answer.reason;
      return false;
    }
    try {
      for (size_t i = 0; i < queries.size(); ++i) {
        const auto a = queries[i][0], b = queries[i][1];
        const Point p{double(((long double)a.x + b.x) / 2), double(((long double)a.y + b.y) / 2),
                      double(((long double)a.z + b.z) / 2)};
        samples.emplace(query_ids[at + i], EngineTrial::ComposedSample(context, p, answer.samples.at(i).sensor));
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!(ready = context.exchange.Vote(context, failure))) return false;
  }
  return true;
}

// Attempts the bounded coupled optimization with its original work and validation accounting.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::OptimizeRepair() {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& surface_target = context.surface_target;
  auto& composition = context.composition;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& directory = *context.exchange.directory;
  auto& field_identity = *context.census.field_identity;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& old = *slot_state.old;
  auto& approved = *slot_state.approved;
  auto& chosen = *slot_state.chosen;
  auto& coordinate_only = *slot_state.coordinate_only;
  auto& private_scratch = *slot_state.private_scratch;
  auto& failure = *slot_state.failure;
  auto& moved = *slot_state.moved;
  auto& repair_approved = *slot_state.repair_approved;
  auto& optimization_approved = *slot_state.optimization_approved;
  auto& repair_budget = *slot_state.repair_budget;
  auto& repair_work = *slot_state.repair_work;
  if (surface_target || selection.geometry_repairs || !repairing ||
      !trial_comm.Maximum(!coordinate_only && repair_work < 160 && chosen && !approved))
    return true;
  const auto began = Clock::now();
  const auto calls_before = stats.optimization_trials;
  const auto work_before = repair_work;
  const auto geometry_before = stats.geometry_rejected,
             height_before = stats.height_rejected + stats.footprint_rejected;
  bool validated = false;
  trial_checks.ReserveOptimization(!coordinate_only && repair_work < 160 && chosen && !approved);
  DonorPatch donors;
  {
    Timer timer(stats.donor_seconds, options.timings, SU2NativeTiming3D::Operation::Import);
    ++stats.donor_batches;
    donors = trial_comm.ImportCells(
        !coordinate_only && repair_work < repair_budget && !approved ? old.cells : empty, options.memory_ceiling,
        transfer_memory::Add(context.exchange.Bytes(context), directory->ResidentBytes(), private_scratch));
  }
  bool candidate = false;
  if (!donors.valid && !DonorCap(donors.reason)) {
    result.reason = donors.reason;
    return false;
  }
  if (!coordinate_only && repair_work < repair_budget && !approved && donors.valid) try {
      const auto components = AssociatedExtensions(donors.cells, geometry);
      observe_transaction("support_donors", donors.valid, &donors);
      FrozenField proposal_field(std::move(donors.cells), composition, options.timings, components,
                                 options.edge_breakpoints, field_identity, true, MaximumDonors, true);
      FieldAccounting accounting{stats, proposal_field};
      Timer timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
      CpuTimer cpu(stats.reconstruction_cpu_seconds, options.timings);
      candidate = trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction,
                                               trial_checks.OptimizePatch(old, RegionMetric(proposal_field), false),
                                               "OptimizeMetricMesh");
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
  if (!(ready = context.exchange.Vote(context, failure))) return false;
  trial_checks.ReleaseOptimization();
  if (trial_comm.Maximum(candidate || (selection.size_repairs && chosen))) {
    const bool placed = trial_checks.ValidatePrivate(boundary_replacement_ready, observe_transaction, before, candidate,
                                                     MetricChange::Repair);
    if (!ready) return false;
    if (placed) {
      approved = repair_approved = true;
      moved = false;
      optimization_approved = true;
      ++stats.optimization_validated;
      validated = true;
    }
  }
  if (stats.optimization_trials > calls_before)
    trial_checks.NoteLadder(1, validated, began, geometry_before, height_before, repair_work - work_before);
  return true;
}

// Tries move, optimizer and topology rungs in their existing order.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::FallbackLadder(const std::map<Id, Node>& nodes, bool fallback, bool move,
                                                           Id vertex, Point moveCenter) {
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& choice = *round_state.choice;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& approved = *slot_state.approved;
  auto& coordinate_only = *slot_state.coordinate_only;
  auto& failure = *slot_state.failure;
  auto& moved = *slot_state.moved;
  auto& repair_approved = *slot_state.repair_approved;
  for (unsigned trial = 0; trial < 19; ++trial) {
    // Keep cheap coordinate trials first; one bounded coupled solve precedes serial topology guesses.
    if (trial == 3 && !OptimizeRepair()) return false;
    if (!trial_comm.Maximum(fallback && !approved)) break;
    const bool move_trial = trial < 3;
    const bool participating =
        fallback && !approved && (!coordinate_only || move_trial) &&
        (move_trial ? move : (!move || (repairing && choice.feature.count == 1) || (!repairing && coarsening))) &&
        (move_trial || trial - 3 < nodes.size());
    if (!trial_comm.Maximum(participating)) continue;
    const auto trial_started = Clock::now();
    const auto geometry_before = stats.geometry_rejected,
               height_before = stats.height_rejected + stats.footprint_rejected;
    bool candidate = false;
    {
      Timer timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);

      if (participating) try {
          candidate = ProposeFallback(nodes, move, vertex, moveCenter, trial, move_trial);
        } catch (const std::exception& e) {
          failure.Set(2, 0, e.what());
        }
      ready = context.exchange.Vote(context, failure);
    }
    if (!ready) return false;
    const bool valid = trial_checks.ValidatePrivate(boundary_replacement_ready, observe_transaction, before, candidate,
                                                    MetricChange::Repair);
    if (!ready) return false;
    if (valid) {
      approved = true;
      repair_approved = true;
      moved = move_trial;
    }
    if (participating)
      trial_checks.NoteLadder(move_trial ? 0 : 2, valid, trial_started, geometry_before, height_before);
  }
  return true;
}

// Proposes the existing local operation, keeping its CPU timing near reconstruction.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::ProposeFallback(const std::map<Id, Node>& nodes, bool move, Id vertex,
                                                            Point moveCenter, unsigned trial, bool move_trial) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& choice = *round_state.choice;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& repairing = *context.control.repairing;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& node_offset = *slot_state.node_offset;
  auto& node_stride = *slot_state.node_stride;
  auto& offset = *slot_state.offset;
  CpuTimer cpu(stats.reconstruction_cpu_seconds, options.timings);
  ++stats.repair_proposals;
  const Id first = maximum_cell + 1 + offset;

  bool candidate = false;
  if (move_trial && maximum_node <= UINT64_MAX - node_stride &&
      node_offset <= UINT64_MAX - maximum_node - node_stride) {
    candidate = ProposeMove(nodes, vertex, moveCenter, trial, first);
  } else if (!move_trial) {
    auto apex = nodes.begin();
    std::advance(apex, trial - 3);
    if (move && repairing && choice.feature.count == 1) {
      if (apex->first != vertex)
        candidate = trial_checks.GuardedGeometry(
            boundary_replacement_ready, observe_transaction,
            CollapseVertex(old, vertex, apex->first, first, geometry, fresh, result.reason), "CollapseVertex");
    } else
      candidate =
          trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction,
                                       Reconnect(old, apex->first, first, geometry, fresh, result.reason), "Reconnect");
  }
  return candidate;
}

// Builds the original constrained coordinate trial for the given step size.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::ProposeMove(const std::map<Id, Node>& nodes, Id vertex, Point moveCenter,
                                                        unsigned trial, Id first) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& composition = context.composition;
  auto& maximum_node = *context.commit.maximum_node;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& point = *slot_state.point;
  auto& coordinate_only = *slot_state.coordinate_only;
  auto& node_offset = *slot_state.node_offset;
  bool candidate = false;
  std::set<Id> neighbors;
  for (auto face : old.surface)
    if (std::find(face.v.begin(), face.v.end(), vertex) != face.v.end()) neighbors.insert(face.v.begin(), face.v.end());
  if (neighbors.empty())
    for (const auto& row : nodes) neighbors.insert(row.first);
  neighbors.erase(vertex);
  long double x = 0, y = 0, z = 0;
  for (Id id : neighbors) {
    auto p = nodes.at(id).p;
    x += p.x;
    y += p.y;
    z += p.z;
  }
  if (!neighbors.empty()) {
    const auto p = nodes.at(vertex).p;
    const long double step = trial == 0 ? 1.L : (trial == 1 ? .5L : .25L);
    point = {maximum_node + 1 + node_offset,
             {double(p.x + step * (x / neighbors.size() - p.x)), double(p.y + step * (y / neighbors.size() - p.y)),
              double(p.z + step * (z / neighbors.size() - p.z))}};
    if (composition) {
      point.p = {double(p.x + step * (moveCenter.x - p.x)), double(p.y + step * (moveCenter.y - p.y)),
                 double(p.z + step * (moveCenter.z - p.z))};
      point = ConstrainMovePoint(old, vertex, point, options.wall_heights, geometry);
    }
    candidate =
        trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction,
                                     MoveVertex(old, vertex, point, first, geometry, fresh, result.reason, nullptr,
                                                coordinate_only ? old.cells.size() : MaximumCavityCells),
                                     "MoveVertex");
  }
  return candidate;
}

// Tries exact geometry repair splits with the retained old cavity and original work reservation.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::GeometryRepairSplits() {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& composition = context.composition;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& directory = *context.exchange.directory;
  auto& maximum_node = *context.commit.maximum_node;
  auto& started = *context.setup.control_prefix.started;
  auto& field_identity = *context.census.field_identity;
  auto& old = *slot_state.old;
  auto& approved = *slot_state.approved;
  auto& chosen = *slot_state.chosen;
  auto& coordinate_only = *slot_state.coordinate_only;
  auto& node_offset = *slot_state.node_offset;
  auto& node_stride = *slot_state.node_stride;
  auto& private_scratch = *slot_state.private_scratch;
  auto& repair_budget = *slot_state.repair_budget;
  auto& repair_work = *slot_state.repair_work;
  if (selection.geometry_repairs && !coordinate_only && chosen && !approved && curved &&
      maximum_node <= UINT64_MAX - node_stride && node_offset <= UINT64_MAX - maximum_node - node_stride) {
    trial_checks.ReserveOptimization(true);
    std::map<Id, Node> nodes;
    std::set<Edge> edges;
    for (const auto& cell : old.cells)
      for (auto n : cell.v) nodes.emplace(n.id, n);
    const auto old_counts = trial_checks.GeometryCounts(old, &edges);
    std::set<Face> physical;
    for (const auto& face : old.surface) physical.insert(Key(face.v));
    for (const auto& face : Incidence(old.cells).Boundary())
      if (!physical.count(Key(face)))
        for (unsigned k = 0; k < 3; ++k) {
          Edge edge{face[k], face[(k + 1) % 3]};
          std::sort(edge.begin(), edge.end());
          edges.erase(edge);
        }
    DonorPatch donors;
    {
      Timer timer(stats.donor_seconds, options.timings, SU2NativeTiming3D::Operation::Import);
      ++stats.donor_batches;
      donors = trial_comm.ImportCells(
          repair_work < repair_budget ? old.cells : empty, options.memory_ceiling,
          transfer_memory::Add(context.exchange.Bytes(context), directory->ResidentBytes(), private_scratch));
    }
    if (!donors.valid && !DonorCap(donors.reason)) {
      result.reason = donors.reason;
      return false;
    }
    if (donors.valid && repair_work < repair_budget) {
      const auto components = AssociatedExtensions(donors.cells, geometry);
      observe_transaction("support_donors", true, &donors);
      FrozenField target(std::move(donors.cells), composition, options.timings, components, options.edge_breakpoints,
                         field_identity, true, MaximumDonors, true);
      FieldAccounting accounting{stats, target};
      for (const auto& edge : edges) {
        if (approved || repair_work >= repair_budget ||
            (options.maximum_seconds &&
             std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds))
          break;

        if (!TryGeometrySplit(edge, nodes, old_counts, target)) return false;
      }
    }
    trial_checks.ReleaseOptimization();
  }
  return true;
}

// Tests one split, then optionally recloses the new shape under the original private budget.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::TryGeometrySplit(const Edge& edge, const std::map<Id, Node>& nodes,
                                                             const std::pair<size_t, size_t>& old_counts,
                                                             FrozenField& target) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& ready = *context.control.ready;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& approved = *slot_state.approved;
  auto& point = *slot_state.point;
  auto& node_offset = *slot_state.node_offset;
  auto& offset = *slot_state.offset;
  auto& moved = *slot_state.moved;
  auto& repair_approved = *slot_state.repair_approved;
  auto& repair_work = *slot_state.repair_work;
  const auto began = Clock::now();
  const auto geometry_before = stats.geometry_rejected;
  const auto height_before = stats.height_rejected + stats.footprint_rejected;
  const auto work_before = repair_work;
  ++repair_work;
  ++stats.repair_proposals;
  const auto a = nodes.at(edge[0]).p, b = nodes.at(edge[1]).p;
  point = {maximum_node + 1 + node_offset,
           {double(((long double)a.x + b.x) / 2), double(((long double)a.y + b.y) / 2),
            double(((long double)a.z + b.z) / 2)}};
  bool candidate;
  {
    Timer timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
    CpuTimer cpu(stats.reconstruction_cpu_seconds, options.timings);
    candidate = trial_checks.GuardedGeometry(
        boundary_replacement_ready, observe_transaction,
        SplitSurfaceStrip(old, edge, point, {}, maximum_cell + 1 + offset, geometry, fresh, result.reason),
        "SplitSurfaceStrip");
  }
  if (candidate) {
    const auto counts = trial_checks.GeometryCounts(fresh);
    candidate =
        EngineTrial::GeometryTrial(context, counts.first <= old_counts.first && counts.second < old_counts.second);
  }
  bool valid = trial_checks.ValidatePrivate(boundary_replacement_ready, observe_transaction, before, candidate,
                                            MetricChange::GeometryRepair);
  if (!ready) return false;
  // Reclose shape privately; only the complete replacement is eligible for publication.

  if (!RecloseGeometrySplit(candidate, valid, old_counts, target)) return false;
  if (valid) {
    approved = repair_approved = true;
    moved = false;
  }
  trial_checks.NoteLadder(2, valid, began, geometry_before, height_before, repair_work - work_before);
  return true;
}

// Attempts the existing bounded shape reclosure for a geometry-improving split.
template <class BoundaryReady, class Observe>
bool RepairLadders<BoundaryReady, Observe>::RecloseGeometrySplit(bool& candidate, bool& valid,
                                                                 const std::pair<size_t, size_t>& old_counts,
                                                                 FrozenField& target) {
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& ready = *context.control.ready;
  auto& fresh = *slot_state.fresh;
  auto& geometric_progress = *slot_state.geometric_progress;
  auto& certified = *slot_state.certified;
  auto& optimization_approved = *slot_state.optimization_approved;
  auto& repair_budget = *slot_state.repair_budget;
  auto& repair_work = *slot_state.repair_work;
  if (candidate && !valid && geometric_progress && certified.shape_measured &&
      (certified.minimum_mean_ratio < MinimumMeanRatio || certified.minimum_scaled_jacobian < MinimumScaledJacobian) &&
      repair_work < repair_budget) {
    const Patch split = fresh;
    {
      Timer timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
      CpuTimer cpu(stats.reconstruction_cpu_seconds, options.timings);
      candidate = trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction,
                                               trial_checks.OptimizePatch(split, RegionMetric(target), false),
                                               "OptimizeGeometrySplit");
    }
    valid = trial_checks.ValidatePrivate(boundary_replacement_ready, observe_transaction, before, candidate,
                                         MetricChange::GeometryRepair);
    if (!ready) return false;
    if (valid) {
      const auto counts = trial_checks.GeometryCounts(fresh);
      valid =
          EngineTrial::GeometryTrial(context, counts.first <= old_counts.first && counts.second < old_counts.second);
      if (valid) {
        optimization_approved = true;
        ++stats.optimization_validated;
      }
    }
  }
  return true;
}

// Holds the original local outputs of crease and split placement.
struct TrialSplitPositions {
  Point split_position{}, central_position{};
  bool split_failed = false, central_available = false;
};

// Borrows the slot and round payloads during the existing private or collective trial.
template <class Reserve, class BoundaryReady, class Observe>
struct CandidateTrial {
  EngineContext& context;
  RoundState& round_state;
  SlotState& slot_state;
  TrialComm& trial_comm;
  TrialChecks& trial_checks;
  Reserve& reserve_dependencies;
  BoundaryReady& boundary_replacement_ready;
  Observe& observe_transaction;
  wall_diagnostics::Choice& wall_choice;
  bool curved;

  // Binds the live trivia and protocol state without starting candidate work.
  CandidateTrial(EngineContext& context, RoundState& round_state, SlotState& slot_state, TrialComm& trial_comm,
                 TrialChecks& trial_checks, Reserve& reserve_dependencies, BoundaryReady& boundary_replacement_ready,
                 Observe& observe_transaction, wall_diagnostics::Choice& wall_choice, bool curved)
      : context(context),
        round_state(round_state),
        slot_state(slot_state),
        trial_comm(trial_comm),
        trial_checks(trial_checks),
        reserve_dependencies(reserve_dependencies),
        boundary_replacement_ready(boundary_replacement_ready),
        observe_transaction(observe_transaction),
        wall_choice(wall_choice),
        curved(curved) {}

  bool Run();
  bool BeginTrial();
  bool PrepareSplitPositions(TrialSplitPositions& positions);
  void ClassifyCrease(SegmentFreedom& crease_freedom, Point& crease_midpoint, long double diagnostic_limit);
  bool PlaceComposedSplit(TrialSplitPositions& positions, SegmentFreedom crease_freedom, Point crease_midpoint,
                          long double diagnostic_limit);
  bool ApplyPrimaryOperator(const TrialSplitPositions& positions);
  void ApplyPrimaryCollapse();
  void ApplyPrimarySplit(const TrialSplitPositions& positions);
  bool PrepareMeasurement(DonorPatch& sensors, bool& streamed, bool metric_needed);
  void MeasureNonStreamed(DonorPatch& sensors, bool streamed, bool metric_needed);
  bool ValidateStoredMetric(MetricChange kind, FrozenField& target, PatchMetric& metric,
                            const std::map<Edge, EdgeMeasure>& reuse);
  bool TryStoredFieldRepair(bool move, Id vertex, PatchMetric& metric, const std::map<Edge, EdgeMeasure>& reuse);
  void TryStoredFieldMove(const std::map<Id, Node>& nodes, Id vertex, Id first, PatchMetric& metric,
                          const std::map<Edge, EdgeMeasure>& reuse);
  Point StoredMoveCentre(const std::map<Id, Node>& nodes, Id vertex, const std::set<Id>& neighbors);
  void TryStoredFieldReconnect(const std::map<Id, Node>& nodes, Id first, PatchMetric& metric,
                               const std::map<Edge, EdgeMeasure>& reuse);
  bool MeasureStreamedTrial(bool metric_needed, bool split_failed);
  bool ValidatePrimaryPatch(bool metric_needed, bool old_measured, const PatchMetric& before);
};

// Keeps all candidate work under the original lane census and caller-owned cavity scope.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::Run() {
  auto& options = context.options;
  auto& world = context.world;
  auto& surface_target = context.surface_target;
  auto& lane_counters = *context.setup.lane_counters;
  auto& slot = slot_state.slot;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& bad_field = *round_state.bad_field;
  auto& lane_slots = *round_state.lane_slots;
  auto& approved = *slot_state.approved;
  auto& chosen = *slot_state.chosen;
  auto& failure = *slot_state.failure;
  auto& repair_work = *slot_state.repair_work;
  auto& imported = *slot_state.imported;
  auto& dependencies = *slot_state.dependencies;
  lane_census::Trial lane_trial(lane_counters, repairing, lane_slots[slot], dependencies, imported, world.rank,
                                repair_work);

  if (!BeginTrial()) return false;
  TrialSplitPositions positions;
  if (!PrepareSplitPositions(positions) || !ApplyPrimaryOperator(positions)) return false;
  // A rejected surface collapse has no later repair route; unrelated debt needs no old-patch measurement.
  const bool metric_needed = chosen && (approved || repairing || !coarsening || (bad_field && !surface_target));
  DonorPatch sensors;
  sensors.valid = true;
  bool streamed = ready;

  if (!PrepareMeasurement(sensors, streamed, metric_needed)) return false;
  if (streamed && !MeasureStreamedTrial(metric_needed, positions.split_failed)) return false;
  try {
    observe_transaction("construction_end", approved);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (options.transaction_observer && !context.exchange.Vote(context, failure)) return false;
  return true;
}

// Admits the private workspace and performs the original observer selection and vote.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::BeginTrial() {
  auto& options = context.options;
  auto& transaction_captured = *context.control.transaction_captured;
  auto& chosen = *slot_state.chosen;
  auto& coordinate_only = *slot_state.coordinate_only;
  auto& private_scratch = *slot_state.private_scratch;
  auto& failure = *slot_state.failure;
  auto& transaction_feature = *slot_state.transaction_feature;
  auto& capture_selected = *slot_state.capture_selected;
  auto& capture = *slot_state.capture;
  const bool coordinate_stream = trial_comm.Maximum(coordinate_only);
  // Admit large coordinate-star copies, surfaces, query regions and measurements before GeometryPatch
  // or source-region packing allocates them; the ordinary topology path keeps its existing bound.
  if (coordinate_stream && !context.exchange.Admit(context, private_scratch)) return false;
  if (options.transaction_observer && options.capture_feature.count && !transaction_captured) {
    capture_selected = chosen && transaction_feature.count == options.capture_feature.count &&
                       transaction_feature.vertices == options.capture_feature.vertices;
    capture = trial_comm.Maximum(capture_selected);
    transaction_captured = capture;
  }
  try {
    observe_transaction("begin", chosen);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (options.transaction_observer && !context.exchange.Vote(context, failure)) return false;
  return true;
}

// Classifies the physical crease before sampling any composition donors.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::PrepareSplitPositions(TrialSplitPositions& positions) {
  auto& stats = context.control.result->stats;
  auto& choice = *round_state.choice;
  auto& timed_lane = *round_state.timed_lane;
  auto& physical_edge = *slot_state.physical_edge;
  if (timed_lane == 2 && physical_edge) ++stats.interior_boundary_discoveries;
  SegmentFreedom crease_freedom = SegmentFreedom::Unresolved;
  Point crease_midpoint{};
  const long double diagnostic_limit =
      MaximumMetricEdge * (1 + 1e-8L);  // Conservative diagnostic margin, never an acceptance tolerance.

  ClassifyCrease(crease_freedom, crease_midpoint, diagnostic_limit);
  if (crease_freedom == SegmentFreedom::None) trial_checks.IncompatibleCrease(choice.measure.length);

  return PlaceComposedSplit(positions, crease_freedom, crease_midpoint, diagnostic_limit);
}

// Classifies whether the selected binary64 crease permits an interior split point.
template <class Reserve, class BoundaryReady, class Observe>
void CandidateTrial<Reserve, BoundaryReady, Observe>::ClassifyCrease(SegmentFreedom& crease_freedom,
                                                                     Point& crease_midpoint,
                                                                     long double diagnostic_limit) {
  auto& geometry = context.geometry;
  auto& composition = context.composition;
  auto& choice = *round_state.choice;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& failure = *slot_state.failure;
  auto& physical_edge = *slot_state.physical_edge;
  auto& imported = *slot_state.imported;
  if (!repairing && !coarsening && composition && physical_edge && choice.measure.length > diagnostic_limit) try {
      std::set<Id> facets;
      for (const auto& row : imported.cells) {
        const auto faces = Faces(row.cell);
        for (unsigned i = 0; i < 4; ++i)
          if (row.physical[i].present &&
              std::find(faces[i].begin(), faces[i].end(), choice.edge[0]) != faces[i].end() &&
              std::find(faces[i].begin(), faces[i].end(), choice.edge[1]) != faces[i].end())
            facets.insert(geometry.Canonical(row.physical[i].facet));
      }
      if (facets.size() == 2) {
        const auto first = *facets.begin(), second = *facets.rbegin();
        if (!geometry.Declared(first) && !geometry.Declared(second)) {
          const auto& f = geometry.Get(first);
          const auto& g = geometry.Get(second);
          const bool folded = std::any_of(g.v.begin(), g.v.end(),
                                          [&](Node n) { return Orientation(f.v[0].p, f.v[1].p, f.v[2].p, n.p) != 0; });
          if (folded) crease_freedom = Binary64SegmentFreedom(choice.measure.a, choice.measure.b, crease_midpoint);
        }
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
}

// Positions the split under the existing frozen donor field and composition guards.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::PlaceComposedSplit(TrialSplitPositions& positions,
                                                                         SegmentFreedom crease_freedom,
                                                                         Point crease_midpoint,
                                                                         long double diagnostic_limit) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& composition = context.composition;
  auto& stats = context.control.result->stats;
  auto& directory = *context.exchange.directory;
  auto& choice = *round_state.choice;
  auto& field_identity = *context.census.field_identity;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& chosen = *slot_state.chosen;
  auto& failure = *slot_state.failure;
  auto& physical_edge = *slot_state.physical_edge;
  auto& imported = *slot_state.imported;
  auto& splitPosition = positions.split_position;
  auto& centralPosition = positions.central_position;
  auto& split_failed = positions.split_failed;
  auto& central_available = positions.central_available;
  if (!repairing && !coarsening && composition) {
    std::vector<Cell> query;
    try {
      if (chosen) query.push_back(imported.cells.front().cell);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return false;
    if (!context.exchange.Admit(context,
                                transfer_memory::Add(MaximumFrozenFieldBytes, transfer_memory::Bytes(imported.cells))))
      return false;
    DonorPatch seed;
    {
      Timer timer(stats.donor_seconds, options.timings, SU2NativeTiming3D::Operation::Import);
      seed = trial_comm.ImportCells(
          query, options.memory_ceiling,
          transfer_memory::Add(context.exchange.Bytes(context), directory->ResidentBytes(), MaximumFrozenFieldBytes));
    }
    if (!seed.valid) {
      result.reason = seed.reason;
      return false;
    }
    if (chosen) try {
        Timer timer(stats.metric_seconds, options.timings, SU2NativeTiming3D::Operation::MetricNormalization);
        CpuTimer cpu(stats.metric_cpu_seconds, options.timings);
        const auto components = AssociatedExtensions(seed.cells, geometry);
        observe_transaction("split_donors", seed.valid, &seed);
        FrozenField target(std::move(seed.cells), composition, options.timings, components, options.edge_breakpoints,
                           field_identity, true, MaximumDonors, true);
        FieldAccounting accounting{stats, target};
        bool conflict = false;
        if (crease_freedom == SegmentFreedom::MidpointOnly) {
          const auto length = std::max(target.TargetEdgeLength(choice.measure.a, crease_midpoint),
                                       target.TargetEdgeLength(crease_midpoint, choice.measure.b));
          conflict = length > diagnostic_limit;
          if (conflict) trial_checks.IncompatibleCrease(length);
        }
        if (!conflict) {
          if (physical_edge) {
            centralPosition = target.SegmentPoint(choice.measure.a, choice.measure.b, .5);
            central_available = true;
          }
          splitPosition = target.BalancedSplitPoint(choice.measure.a, choice.measure.b);
        }
      } catch (const std::exception& e) {
        result.reason = e.what();
        split_failed = !(physical_edge && central_available);
        if (!split_failed) splitPosition = centralPosition;
      }
    if (!context.exchange.Vote(context, failure)) return false;
  }
  return true;
}

// Runs the primary candidate under the original Candidate and CPU timing scopes.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::ApplyPrimaryOperator(const TrialSplitPositions& positions) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& stats = context.control.result->stats;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& old = *slot_state.old;
  auto& chosen = *slot_state.chosen;
  auto& node_offset = *slot_state.node_offset;
  auto& node_stride = *slot_state.node_stride;
  auto& offset = *slot_state.offset;
  auto& failure = *slot_state.failure;
  auto& imported = *slot_state.imported;
  const bool split_failed = positions.split_failed;
  {
    Timer timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
    if (chosen && split_failed) wall_choice.Disposition(wall_diagnostics::PreTrialDisposition::SplitPosition);
    if (chosen && !split_failed) {
      CpuTimer private_cpu(stats.reconstruction_cpu_seconds, options.timings);
      wall_choice.EnterOperator();
      ++stats.attempted;
      if ((maximum_node > UINT64_MAX - node_stride || node_offset > UINT64_MAX - maximum_node - node_stride) ||
          maximum_cell > UINT64_MAX - MaximumReplacementCells ||
          offset > UINT64_MAX - maximum_cell - MaximumReplacementCells) {
        failure.Set(1, 0, "Native 3D reserved identity range overflows.");
      } else
        try {
          old = GeometryPatch(imported, &geometry);

          if (!repairing && coarsening)
            ApplyPrimaryCollapse();
          else if (!repairing)
            ApplyPrimarySplit(positions);
        } catch (const std::exception& e) {
          failure.Set(2, 0, e.what());
        }
    }
    ready = context.exchange.Vote(context, failure);
  }
  if (!ready) return false;
  return true;
}

// Keeps the original geometry, corrected collapse and baseline collapse branches.
template <class Reserve, class BoundaryReady, class Observe>
void CandidateTrial<Reserve, BoundaryReady, Observe>::ApplyPrimaryCollapse() {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& choice = *round_state.choice;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& approved = *slot_state.approved;
  auto& offset = *slot_state.offset;
  auto& collapse_corrected = *slot_state.collapse_corrected;
  auto& collapse_apex = *slot_state.collapse_apex;
  if (!geometry.EntityGeometry() &&
      CollapseOutsideTolerance(old, choice.edge[1], choice.edge[0], geometry, options.surface_tolerance)) {
    result.reason = "Native3D collapsed boundary leaves an original geometry sample outside tolerance.";
    approved = trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction, false, "CollapseVertex");
  } else if (collapse_corrected)
    approved =
        trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction,
                                     ReconnectCollapse(old, choice.edge[1], choice.edge[0], collapse_apex,
                                                       maximum_cell + 1 + offset, geometry, fresh, result.reason),
                                     "ReconnectCollapse");
  else
    approved = trial_checks.GuardedGeometry(
        boundary_replacement_ready, observe_transaction,
        CollapseVertex(old, choice.edge[1], choice.edge[0], maximum_cell + 1 + offset, geometry, fresh, result.reason),
        "CollapseVertex");
}

// Applies the same primary split and central-point retry in their original order.
template <class Reserve, class BoundaryReady, class Observe>
void CandidateTrial<Reserve, BoundaryReady, Observe>::ApplyPrimarySplit(const TrialSplitPositions& positions) {
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& composition = context.composition;
  auto& choice = *round_state.choice;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& approved = *slot_state.approved;
  auto& point = *slot_state.point;
  auto& node_offset = *slot_state.node_offset;
  auto& offset = *slot_state.offset;
  auto& physical_edge = *slot_state.physical_edge;
  auto& cavity_inserted = *slot_state.cavity_inserted;
  auto& splitPosition = positions.split_position;
  auto& centralPosition = positions.central_position;
  auto& central_available = positions.central_available;
  const auto a = choice.measure.a, b = choice.measure.b;
  point = {maximum_node + 1 + node_offset,
           {double((static_cast<long double>(a.x) + b.x) / 2), double((static_cast<long double>(a.y) + b.y) / 2),
            double((static_cast<long double>(a.z) + b.z) / 2)}};
  if (composition) point.p = splitPosition;
  approved = trial_checks.GuardedGeometry(
      boundary_replacement_ready, observe_transaction,
      SplitEdge(old, choice.edge, point, maximum_cell + 1 + offset, geometry, fresh, result.reason), "SplitEdge");
  if (!approved) {
    approved = trial_checks.GuardedGeometry(
        boundary_replacement_ready, observe_transaction,
        InsertEdgeCavity(old, choice.edge, point, maximum_cell + 1 + offset, geometry, fresh, result.reason),
        "InsertEdgeCavity");
    cavity_inserted = approved;
  }
  if (!approved && physical_edge && composition && central_available &&
      (point.p.x != centralPosition.x || point.p.y != centralPosition.y || point.p.z != centralPosition.z)) {
    point.p = centralPosition;
    approved = trial_checks.GuardedGeometry(
        boundary_replacement_ready, observe_transaction,
        SplitEdge(old, choice.edge, point, maximum_cell + 1 + offset, geometry, fresh, result.reason), "SplitEdge");
    if (!approved) {
      approved = trial_checks.GuardedGeometry(
          boundary_replacement_ready, observe_transaction,
          InsertEdgeCavity(old, choice.edge, point, maximum_cell + 1 + offset, geometry, fresh, result.reason),
          "InsertEdgeCavity");
      cavity_inserted = approved;
    }
  }
}

// Keeps all optimizer reservation and admission calls around the original metric branches.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::PrepareMeasurement(DonorPatch& sensors, bool& streamed,
                                                                         bool metric_needed) {
  auto& result = *context.control.result;
  auto& composition = context.composition;
  auto& stats = context.control.result->stats;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& approved = *slot_state.approved;
  auto& chosen = *slot_state.chosen;
  auto& private_scratch = *slot_state.private_scratch;
  auto& repair_budget = *slot_state.repair_budget;
  auto& repair_work = *slot_state.repair_work;
  auto& field = *slot_state.field;
  if (!ready || (!sensors.valid && !streamed)) {
    if (ready) result.reason = sensors.reason;
    return false;
  }
  if (streamed && !context.exchange.Admit(context, private_scratch)) return false;
  if (streamed) ++stats.private_query_retries;
  trial_checks.ReserveOptimization(!streamed && chosen && repairing);
  if (trial_comm.Maximum(repair_budget > repair_work) &&
      !context.exchange.Admit(context, transfer_memory::Add(private_scratch, transfer_memory::Bytes(sensors.cells))))
    return false;
  MeasureNonStreamed(sensors, streamed, metric_needed);
  trial_checks.ReleaseOptimization();
  if (!streamed && composition && !repairing && !coarsening) {
    if (trial_comm.Maximum(chosen && !approved)) {
      field.reset();
      if (!context.exchange.Admit(context, private_scratch)) return false;
      streamed = true;
      ++stats.private_query_retries;
    }
  }
  return true;
}

// Preserves the non-streamed metric timer, reconstruction subtraction and legacy branch.
template <class Reserve, class BoundaryReady, class Observe>
void CandidateTrial<Reserve, BoundaryReady, Observe>::MeasureNonStreamed(DonorPatch& sensors, bool streamed,
                                                                         bool metric_needed) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& composition = context.composition;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& choice = *round_state.choice;
  auto& field_identity = *context.census.field_identity;
  auto& repairing = *context.control.repairing;
  auto& construction_kind = *context.control.construction_kind;
  auto& old = *slot_state.old;
  auto& approved = *slot_state.approved;
  auto& repair_approved = *slot_state.repair_approved;
  auto& cavity_inserted = *slot_state.cavity_inserted;
  auto& field = *slot_state.field;
  Timer timer(stats.metric_seconds, options.timings, SU2NativeTiming3D::Operation::MetricNormalization);
  const double reconstruction_before = stats.reconstruction_seconds;
  const double reconstruction_cpu_before = stats.reconstruction_cpu_seconds;
  if (!streamed && metric_needed) try {
      CpuTimer private_cpu(stats.metric_cpu_seconds, options.timings);
      const auto components = AssociatedExtensions(sensors.cells, geometry);
      observe_transaction("support_donors", sensors.valid, &sensors);
      field = std::make_unique<FrozenField>(std::move(sensors.cells), composition, options.timings, components,
                                            options.edge_breakpoints, field_identity, true, MaximumDonors, true);
      PatchMetric metric;
      std::map<Edge, EdgeMeasure> reuse;
      for (const auto& cell : old.cells) selection.Cached(cell, reuse);
      if (approved) {
        approved = ValidateStoredMetric(construction_kind, *field, metric, reuse);
      }
      if (!approved) cavity_inserted = false;
      if (repairing) {
        approved = TryStoredFieldRepair(choice.feature.count == 1, choice.feature.vertices[0], metric, reuse);
        repair_approved = approved;
      }
    } catch (const std::exception& e) {
      approved = false;
      result.reason = e.what();
    }
  if (field) FieldCounts(stats, *field);
  stats.metric_seconds -= stats.reconstruction_seconds - reconstruction_before;
  stats.metric_cpu_seconds -= stats.reconstruction_cpu_seconds - reconstruction_cpu_before;
}

// Validates a patch under the old vector metric branch using its original frozen inputs.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::ValidateStoredMetric(MetricChange kind, FrozenField& target,
                                                                           PatchMetric& metric,
                                                                           const std::map<Edge, EdgeMeasure>& reuse) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& surface_target = context.surface_target;
  auto& wall_counters = *context.setup.wall_counters;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& geometric_progress = *slot_state.geometric_progress;
  auto& certified = *slot_state.certified;
  if (!surface_target)
    return EngineTrial::MetricTrial(
        context,
        ValidateMetricChange(old.cells, fresh.cells, kind, target, metric, result.reason, nullptr, &reuse, &certified),
        certified, kind);
  if (!trial_checks.GeometricChange(boundary_replacement_ready, observe_transaction, true, kind)) return false;
  auto before = MeasureSurfacePlanning(old, target, geometry, options.wall_heights, surface_target);
  metric = MeasureSurfacePlanning(fresh, target, geometry, options.wall_heights, surface_target);
  EngineTrial::SurfaceMerit(context, before, old);
  EngineTrial::SurfaceMerit(context, metric, fresh);
  const auto effective = kind == MetricChange::Repair ? MetricChange::GeometryRepair : kind;
  return EngineTrial::MetricTrial(
      context,
      wall_counters.MetricValidation(ValidateMetricChange(old.cells.size(), before, fresh.cells.size(), metric,
                                                          effective, result.reason, &certified)),
      certified, effective, geometric_progress);
}

// Retains the bounded legacy repair branch until its separate deletion gate.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::TryStoredFieldRepair(bool move, Id vertex, PatchMetric& metric,
                                                                           const std::map<Edge, EdgeMeasure>& reuse) {
  auto& options = context.options;
  auto& surface_target = context.surface_target;
  auto& stats = context.control.result->stats;
  auto& maximum_node = *context.commit.maximum_node;
  auto& maximum_cell = *context.commit.maximum_cell;
  auto& old = *slot_state.old;
  auto& approved = *slot_state.approved;
  auto& node_offset = *slot_state.node_offset;
  auto& node_stride = *slot_state.node_stride;
  auto& offset = *slot_state.offset;
  auto& moved = *slot_state.moved;
  auto& optimization_approved = *slot_state.optimization_approved;
  auto& repair_budget = *slot_state.repair_budget;
  auto& repair_work = *slot_state.repair_work;
  auto& field = *slot_state.field;
  std::map<Id, Node> nodes;
  for (const auto& cell : old.cells)
    for (auto node : cell.v) nodes.emplace(node.id, node);
  const Id first = maximum_cell + 1 + offset;
  if (move) {
    if (maximum_node > UINT64_MAX - node_stride || node_offset > UINT64_MAX - maximum_node - node_stride) return false;

    TryStoredFieldMove(nodes, vertex, first, metric, reuse);
  } else {
    TryStoredFieldReconnect(nodes, first, metric, reuse);
  }
  if (!surface_target && !approved && repair_work < repair_budget) {
    {
      Timer geometry_timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
      CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds, options.timings);
      approved = trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction,
                                              trial_checks.OptimizePatch(old, RegionMetric(*field), false));
    }
    if (approved) {
      const auto kind = trial_checks.RepairKind();
      approved = ValidateStoredMetric(kind, *field, metric, reuse);
    }
    if (approved) {
      moved = false;
      optimization_approved = true;
      ++stats.optimization_validated;
    }
  }
  return approved;
}

// Tries all three coordinates in the existing stored-field order.
template <class Reserve, class BoundaryReady, class Observe>
void CandidateTrial<Reserve, BoundaryReady, Observe>::TryStoredFieldMove(const std::map<Id, Node>& nodes, Id vertex,
                                                                         Id first, PatchMetric& metric,
                                                                         const std::map<Edge, EdgeMeasure>& reuse) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& composition = context.composition;
  auto& stats = context.control.result->stats;
  auto& maximum_node = *context.commit.maximum_node;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& approved = *slot_state.approved;
  auto& point = *slot_state.point;
  auto& node_offset = *slot_state.node_offset;
  auto& moved = *slot_state.moved;
  auto& field = *slot_state.field;
  std::set<Id> neighbors;
  for (auto face : old.surface)
    if (std::find(face.v.begin(), face.v.end(), vertex) != face.v.end()) neighbors.insert(face.v.begin(), face.v.end());
  if (neighbors.empty())
    for (const auto& row : nodes) neighbors.insert(row.first);
  neighbors.erase(vertex);

  const Point center = StoredMoveCentre(nodes, vertex, neighbors);
  if (!neighbors.empty())
    for (long double step : {1.L, .5L, .25L}) {
      const auto p = nodes.at(vertex).p;
      point = {maximum_node + 1 + node_offset,
               {double(p.x + step * (center.x - p.x)), double(p.y + step * (center.y - p.y)),
                double(p.z + step * (center.z - p.z))}};
      if (composition) point = ConstrainMovePoint(old, vertex, point, options.wall_heights, geometry);
      ++stats.repair_proposals;
      {
        Timer geometry_timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
        CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds, options.timings);
        approved = trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction,
                                                MoveVertex(old, vertex, point, first, geometry, fresh, result.reason),
                                                "MoveVertex");
      }
      if (approved) {
        const auto kind = trial_checks.RepairKind();
        approved = ValidateStoredMetric(kind, *field, metric, reuse);
      }
      if (approved) {
        moved = true;
        break;
      }
    }
}

// Calculates the existing Cartesian or composed-metric move centre.
template <class Reserve, class BoundaryReady, class Observe>
Point CandidateTrial<Reserve, BoundaryReady, Observe>::StoredMoveCentre(const std::map<Id, Node>& nodes, Id vertex,
                                                                        const std::set<Id>& neighbors) {
  auto& composition = context.composition;
  auto& field = *slot_state.field;
  long double x = 0, y = 0, z = 0;
  for (Id id : neighbors) {
    auto p = nodes.at(id).p;
    x += p.x;
    y += p.y;
    z += p.z;
  }
  Point center{};
  if (!neighbors.empty()) {
    const auto origin = nodes.at(vertex).p;
    center = {double(x / neighbors.size()), double(y / neighbors.size()), double(z / neighbors.size())};
    if (composition) {
      std::vector<std::pair<Point, Tensor>> samples;
      for (auto id : neighbors) {
        const auto q = nodes.at(id).p;
        samples.emplace_back(q, field->QueryMidpoint(origin, q).target);
      }
      center = MetricMeanPoint(origin, samples);
    }
  }
  return center;
}

// Uses the legacy reconnection order and sixteen-proposal bound.
template <class Reserve, class BoundaryReady, class Observe>
void CandidateTrial<Reserve, BoundaryReady, Observe>::TryStoredFieldReconnect(
    const std::map<Id, Node>& nodes, Id first, PatchMetric& metric, const std::map<Edge, EdgeMeasure>& reuse) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& result = *context.control.result;
  auto& stats = context.control.result->stats;
  auto& old = *slot_state.old;
  auto& fresh = *slot_state.fresh;
  auto& approved = *slot_state.approved;
  auto& field = *slot_state.field;
  unsigned proposals = 0;
  for (const auto& row : nodes) {
    if (++proposals > 16) break;
    ++stats.repair_proposals;
    {
      Timer geometry_timer(stats.reconstruction_seconds, options.timings, SU2NativeTiming3D::Operation::Candidate);
      CpuTimer geometry_cpu(stats.reconstruction_cpu_seconds, options.timings);
      approved =
          trial_checks.GuardedGeometry(boundary_replacement_ready, observe_transaction,
                                       Reconnect(old, row.first, first, geometry, fresh, result.reason), "Reconnect");
    }
    if (approved) {
      const auto kind = trial_checks.RepairKind();
      approved = ValidateStoredMetric(kind, *field, metric, reuse);
    }
    if (approved) break;
  }
}

// Measures the old cavity, validates the primary trial and delegates the later repair paths.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::MeasureStreamedTrial(bool metric_needed, bool split_failed) {
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& selection = *context.census.selection;
  auto& stats = context.control.result->stats;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& old = *slot_state.old;
  auto& failure = *slot_state.failure;
  auto& old_flags = *slot_state.old_flags;
  auto& measured_flags = *slot_state.measured_flags;
  std::map<Edge, EdgeMeasure> reuse;
  {
    Timer timer(stats.metric_seconds, options.timings, SU2NativeTiming3D::Operation::MetricNormalization);
    try {
      for (const auto& cell : old.cells) selection.Cached(cell, reuse);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    ready = context.exchange.Vote(context, failure);
  }
  if (!ready) return false;
  PatchMetric before;
  const std::vector<Cell> empty;
  const bool old_measured =
      trial_checks.MeasurePrivate(observe_transaction, metric_needed ? old.cells : empty, before, false, reuse, &old);
  if (repairing && old_measured) old_flags = measured_flags;
  // A failed measurement of unchanged accepted geometry invalidates the event on every worker.
  if (!ready) return false;
  CLocalFailure old_failure;
  if (metric_needed && !old_measured) old_failure.Set(1, 0, result.reason);
  if (!(ready = context.exchange.Vote(context, old_failure))) return false;

  if (!ValidatePrimaryPatch(metric_needed, old_measured, before)) return false;
  if (!EngineLegacyWall::Run(context, round_state, slot_state, trial_comm, trial_checks, reserve_dependencies,
                             boundary_replacement_ready, observe_transaction, split_failed, before, reuse, empty))
    return false;
  RepairLadders repair_ladders(context, round_state, slot_state, trial_comm, trial_checks, boundary_replacement_ready,
                               observe_transaction, before, empty, curved);
  if (!repair_ladders.Run(metric_needed, old_measured)) return false;
  return true;
}

// Validates the primary edit under the existing lift, placement and wall-diagnostic rules.
template <class Reserve, class BoundaryReady, class Observe>
bool CandidateTrial<Reserve, BoundaryReady, Observe>::ValidatePrimaryPatch(bool metric_needed, bool old_measured,
                                                                           const PatchMetric& before) {
  auto& options = context.options;
  auto& surface_target = context.surface_target;
  auto& stats = context.control.result->stats;
  auto& choice = *round_state.choice;
  auto& wall_counters = *context.setup.wall_counters;
  auto& slot = slot_state.slot;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& ready = *context.control.ready;
  auto& construction_kind = *context.control.construction_kind;
  auto& lane_slots = *round_state.lane_slots;
  auto& fresh = *slot_state.fresh;
  auto& approved = *slot_state.approved;
  auto& point = *slot_state.point;
  auto& cavity_inserted = *slot_state.cavity_inserted;
  bool required_split = false, checked_split = false;
  if (surface_target && curved && !coarsening && !repairing && choice.geometric && approved) {
    const Node* bound_point = nullptr;
    for (const auto& cell : fresh.cells)
      for (const auto& node : cell.v)
        if (node.id == point.id) bound_point = &node;
    if (bound_point) {
      required_split = SplitPointLiftRequiresSplit(point.p, bound_point->p, options.surface_tolerance);
      checked_split = !required_split;
      wall_counters.ObservePrimarySplit(required_split);
    }
  }
  const auto primary_guidance_rejected = stats.geometry_guidance_rejected;
  approved = trial_checks.ValidatePrivate(boundary_replacement_ready, observe_transaction, before,
                                          metric_needed && old_measured && approved,
                                          choice.geometric ? MetricChange::GeometryRefine : construction_kind, false,
                                          false, 0, wall_diagnostics::PatchSource::Primary, required_split);
  if (options.timings) {
    lane_slots[slot].required_primary_split = required_split && approved;
    lane_slots[slot].checked_primary_split = checked_split && approved;
    wall_counters.ObserveCheckedRejection(checked_split &&
                                          stats.geometry_guidance_rejected > primary_guidance_rejected);
  }
  if (!approved) cavity_inserted = false;
  if (!ready) return false;
  return true;
}
