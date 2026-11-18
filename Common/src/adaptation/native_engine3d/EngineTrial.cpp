// Trial helpers remain in the original strict-arithmetic translation unit.
#include "EngineTrial.hpp"

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
