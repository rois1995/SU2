// Private reporting definitions, included after EngineContext type completion.
#include "EngineReports.hpp"

void EngineReports::RecordRejection(EngineContext& context, size_t diagnostic_rejection_limit, Feature feature,
                                    const Patch* old, const MetricDiagnosis* metric, const std::string& reason) {
  auto& diagnostic_rejections = *context.control.diagnostic_rejections;
  auto& diagnostic_sequence = *context.control.diagnostic_sequence;
  auto& stats = context.control.result->stats;
  if (!diagnostic_rejections.capacity()) return;
  const size_t index = diagnostic_sequence % diagnostic_rejection_limit;
  if (diagnostic_rejections.size() < diagnostic_rejection_limit) diagnostic_rejections.emplace_back();
  auto& row = diagnostic_rejections[index];
  row = {};
  row.sequence = diagnostic_sequence++;
  row.round = stats.rounds;
  row.feature = feature;
  if (old) {
    row.old_count = std::min(old->cells.size(), row.old_cells.size());
    row.old_cells_truncated = row.old_count < old->cells.size();
    for (size_t i = 0; i < row.old_count; ++i) row.old_cells[i] = old->cells[i].id;
  } else {
    row.old_cells_truncated = true;
  }
  if (metric) {
    row.shape_measured = metric->shape_measured;
    row.q = metric->minimum_mean_ratio;
    row.J = metric->minimum_scaled_jacobian;
    row.failure = metric->failure;
  }
  size_t count = std::min(reason.size(), row.reason.size() - 1);
  if (count < reason.size())
    while (count && (static_cast<unsigned char>(reason[count]) & 0xc0) == 0x80) --count;
  std::copy_n(reason.data(), count, row.reason.data());
  row.reason_truncated = count < reason.size();
}

void EngineReports::ConsiderFixedMetricRow(EngineContext& context, const OwnedCell& row) {
  auto& fixed_metric_available = *context.control.fixed_metric_available;
  auto& fixed_metric_points = *context.control.fixed_metric_points;
  auto& fixed_metric_cells = *context.control.fixed_metric_cells;
  auto& source = *context.exchange.source;
  if (!fixed_metric_available) return;
  for (size_t i = 0; i < fixed_metric_points.size(); ++i) {
    if (row.cell.id >= fixed_metric_cells[i]) continue;
    const auto p = fixed_metric_points[i].point;
    bool inside = true;
    const double xyz[3] = {p.x, p.y, p.z};
    for (unsigned axis = 0; axis < 3 && inside; ++axis) {
      double low = INFINITY, high = -INFINITY;
      for (const auto& node : row.cell.v) {
        const double coordinates[3] = {node.p.x, node.p.y, node.p.z};
        low = std::min(low, coordinates[axis]);
        high = std::max(high, coordinates[axis]);
      }
      inside = xyz[axis] >= low && xyz[axis] <= high;
    }
    if (!inside) continue;
    try {
      Tensor current;
      if (source.SampleContainedCurrentMetric(row, p, current)) fixed_metric_cells[i] = row.cell.id;
    } catch (const std::exception&) {
    }  // Unsupported diagnostics never authorize or veto a mesh edit.
  }
}

void EngineReports::UpdateFixedMetricCells(EngineContext& context, const std::vector<Dependency>& removed,
                                           const std::vector<OwnedCell>& added) {
  auto& fixed_metric_available = *context.control.fixed_metric_available;
  auto& fixed_metric_points = *context.control.fixed_metric_points;
  auto& fixed_metric_cells = *context.control.fixed_metric_cells;
  if (!fixed_metric_available) return;
  for (size_t i = 0; i < fixed_metric_points.size(); ++i)
    for (const auto& row : removed)
      if (row.cell == fixed_metric_cells[i]) fixed_metric_cells[i] = UINT64_MAX;
  for (const auto& row : added) ConsiderFixedMetricRow(context, row);
}

void EngineReports::ResetFixedMetricCells(EngineContext& context) {
  auto& fixed_metric_available = *context.control.fixed_metric_available;
  auto& fixed_metric_cells = *context.control.fixed_metric_cells;
  auto& working = *context.ownership.working;
  if (!fixed_metric_available) return;
  fixed_metric_cells.fill(UINT64_MAX);
  for (const auto& row : working) ConsiderFixedMetricRow(context, row.second);
}

template <class Bytes>
void EngineReports::InitializeFixedMetricDrift(EngineContext& context, bool& available, Bytes& bytes) {
  auto& world = context.world;
  auto& options = context.options;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& fixed_metric_initialized = *context.control.fixed_metric_initialized;
  auto& fixed_metric_available = *context.control.fixed_metric_available;
  auto& fixed_metric_points = *context.control.fixed_metric_points;
  auto& fixed_metric_cells = *context.control.fixed_metric_cells;
  fixed_metric_initialized = true;
  CLocalFailure failure;
  std::vector<FixedMetricPoint> local;
  std::vector<std::vector<FixedMetricPoint>> outgoing;
  try {
    const size_t quota = FixedMetricSamples / world.size + (size_t(world.rank) < FixedMetricSamples % world.size);
    std::array<std::vector<Id>, 3> candidates;
    for (const auto& row : working) {
      const auto& cached = selection.cells.at(row.first);
      const unsigned kind = cached.debt.q_bad || cached.debt.j_bad ? (cached.surface ? 0u : 1u) : 2u;
      if (candidates[kind].size() < quota) candidates[kind].push_back(row.first);
    }
    for (const auto& category : candidates)
      for (Id id : category)
        if (local.size() < quota) {
          const auto& row = working.at(id);
          const auto point = Centroid(row.cell);
          Tensor metric;
          if (source.SampleContainedCurrentMetric(row, point, metric)) local.push_back({id, world.rank, point, metric});
        }
    outgoing.assign(world.size, local);
  } catch (const std::exception& e) {
    failure.Set(1, 0, e.what());
  }
  if (!world.elect(failure).any) {
    bool admitted = false;
    auto points = world.exchange(outgoing, &admitted, options.memory_ceiling,
                                 transfer_memory::Add(bytes(), source.ResidentBytes(), directory->ResidentBytes()));
    if (admitted && points.size() <= FixedMetricSamples) {
      std::sort(points.begin(), points.end(),
                [](const auto& a, const auto& b) { return std::tie(a.owner, a.cell) < std::tie(b.owner, b.cell); });
      fixed_metric_points = std::move(points);
      fixed_metric_available = !fixed_metric_points.empty();
      for (size_t i = 0; i < fixed_metric_points.size(); ++i)
        if (fixed_metric_points[i].owner == world.rank) fixed_metric_cells[i] = fixed_metric_points[i].cell;
    }
  }
  available = fixed_metric_available;
}

template <class Bytes>
void EngineReports::ReadFixedMetricDrift(EngineContext& context,
                                         std::array<FixedMetricReading, FixedMetricSamples>& selected,
                                         std::array<bool, FixedMetricSamples>& have, bool& available, Bytes& bytes) {
  auto& world = context.world;
  auto& options = context.options;
  auto& working = *context.ownership.working;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& fixed_metric_points = *context.control.fixed_metric_points;
  auto& fixed_metric_cells = *context.control.fixed_metric_cells;
  CLocalFailure failure;
  std::vector<FixedMetricReading> local;
  std::vector<std::vector<FixedMetricReading>> outgoing;
  try {
    local.reserve(fixed_metric_points.size());
    for (size_t i = 0; i < fixed_metric_points.size(); ++i) {
      const auto row = working.find(fixed_metric_cells[i]);
      if (row == working.end()) continue;
      Tensor current;
      if (source.SampleContainedCurrentMetric(row->second, fixed_metric_points[i].point, current))
        local.push_back({i, row->first, world.rank, current});
    }
    outgoing.assign(world.size, local);
  } catch (const std::exception& e) {
    failure.Set(1, 0, e.what());
  }
  if (world.elect(failure).any)
    available = false;
  else {
    bool admitted = false;
    const auto readings =
        world.exchange(outgoing, &admitted, options.memory_ceiling,
                       transfer_memory::Add(bytes(), source.ResidentBytes(), directory->ResidentBytes()));
    available = admitted;
    if (admitted)
      for (const auto& reading : readings)
        if (reading.sample < fixed_metric_points.size()) {
          const auto i = reading.sample;
          if (!have[i] || std::tie(reading.cell, reading.owner) < std::tie(selected[i].cell, selected[i].owner)) {
            selected[i] = reading;
            have[i] = true;
          }
        }
  }
}

void EngineReports::CompareFixedMetricDrift(EngineContext& context,
                                            const std::array<FixedMetricReading, FixedMetricSamples>& selected,
                                            const std::array<bool, FixedMetricSamples>& have, size_t& supported,
                                            size_t& owned, long double& maximum, long double& local_maximum) {
  auto& world = context.world;
  auto& fixed_metric_points = *context.control.fixed_metric_points;
  for (size_t i = 0; i < fixed_metric_points.size(); ++i)
    if (have[i]) {
      const auto a = fixed_metric_points[i].baseline, b = selected[i].current;
      const long double before[6] = {a.xx, a.xy, a.xz, a.yy, a.yz, a.zz},
                        after[6] = {b.xx, b.xy, b.xz, b.yy, b.yz, b.zz};
      long double scale = 0, difference = 0;
      for (unsigned k = 0; k < 6; ++k) {
        const auto weight = (k == 1 || k == 2 || k == 4) ? std::sqrt(2.L) : 1.L;
        scale = std::hypot(scale, weight * before[k]);
        difference = std::hypot(difference, weight * (after[k] - before[k]));
      }
      if (!(std::isfinite(scale) && scale > 0 && std::isfinite(difference))) continue;
      const auto relative = difference / scale;
      if (!std::isfinite(relative)) continue;
      ++supported;
      maximum = std::max(maximum, relative);
      if (selected[i].owner == world.rank) {
        ++owned;
        local_maximum = std::max(local_maximum, relative);
      }
    }
}

template <class Bytes>
void EngineReports::EmitFixedMetricDrift(EngineContext& context, const char* stage, Bytes& bytes) {
  auto& world = context.world;
  auto& options = context.options;
  auto& fixed_metric_available = *context.control.fixed_metric_available;
  auto& fixed_metric_initialized = *context.control.fixed_metric_initialized;
  auto& fixed_metric_points = *context.control.fixed_metric_points;
  auto& stats = context.control.result->stats;
  auto& composition = context.composition;
  const auto began = Clock::now();
  bool available = fixed_metric_available;
  if (!fixed_metric_initialized && std::string(stage) == "quality_entry")
    InitializeFixedMetricDrift(context, available, bytes);
  std::array<FixedMetricReading, FixedMetricSamples> selected{};
  std::array<bool, FixedMetricSamples> have{};
  if (fixed_metric_available) ReadFixedMetricDrift(context, selected, have, available, bytes);
  size_t supported = 0, owned = 0;
  long double maximum = 0, local_maximum = 0;
  if (available) CompareFixedMetricDrift(context, selected, have, supported, owned, maximum, local_maximum);
  stats.quality.diagnostic_seconds += std::chrono::duration<double>(Clock::now() - began).count();
  if (options.timings)
    std::cout << std::setprecision(17) << "NATIVE3D_QUALITY_DRIFT rank=" << world.rank << " stage=" << stage
              << " initialized=" << fixed_metric_initialized << " available=" << available
              << " samples=" << fixed_metric_points.size() << " supported=" << supported
              << " lost=" << fixed_metric_points.size() - supported << " owned=" << owned
              << " maximum_relative_frobenius=" << maximum << " owned_maximum_relative_frobenius=" << local_maximum
              << " current_nodal_sensor=1 full_target=" << !bool(composition) << '\n';
}
