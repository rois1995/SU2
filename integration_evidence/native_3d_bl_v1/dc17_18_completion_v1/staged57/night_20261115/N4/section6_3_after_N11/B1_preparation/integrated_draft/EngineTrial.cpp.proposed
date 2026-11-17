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
