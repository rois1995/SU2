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

void EngineReports::InitializeFixedMetricDrift(EngineContext& context, bool& available) {
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
      const auto& cached = selection.census.cells.at(row.first);
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
    auto points = world.exchange(
        outgoing, &admitted, options.memory_ceiling,
        transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), directory->ResidentBytes()));
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

void EngineReports::ReadFixedMetricDrift(EngineContext& context,
                                         std::array<FixedMetricReading, FixedMetricSamples>& selected,
                                         std::array<bool, FixedMetricSamples>& have, bool& available) {
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
    const auto readings = world.exchange(
        outgoing, &admitted, options.memory_ceiling,
        transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), directory->ResidentBytes()));
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

void EngineReports::EmitFixedMetricDrift(EngineContext& context, const char* stage) {
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
    InitializeFixedMetricDrift(context, available);
  std::array<FixedMetricReading, FixedMetricSamples> selected{};
  std::array<bool, FixedMetricSamples> have{};
  if (fixed_metric_available) ReadFixedMetricDrift(context, selected, have, available);
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

// Quality reporting helpers retain the original scan and output order.
// Collects the original quality census without reordering its scans.
void EngineReports::CollectQualityCensus(Selection& selection, std::array<uint64_t, 2>& q_location,
                                         std::array<uint64_t, 2>& j_location, std::array<uint64_t, 3>& q_features,
                                         std::array<uint64_t, 3>& j_features, std::array<uint64_t, 6>& q_hist,
                                         std::array<uint64_t, 6>& j_hist, std::array<uint64_t, 6>& geometry_hist,
                                         std::array<uint64_t, 8>& geometry_types, uint64_t& bad_total,
                                         uint64_t& bad_short09, uint64_t& bad_short05, uint64_t& stuck_shape,
                                         uint64_t& stuck_required, uint64_t& missing_features,
                                         uint64_t& blocked_features) {
  auto miss_bin = [](long double value) {
    return !std::isfinite(value) ? 5u
           : value < .01L        ? 0u
           : value < .1L         ? 1u
           : value < .25L        ? 2u
           : value < .5L         ? 3u
                                 : 4u;
  };
  for (const auto& item : selection.census.cells) {
    const auto& row = item.second;
    const auto& d = row.debt;
    const bool bad = d.q_bad || d.j_bad;
    const unsigned feature = row.feature_known ? unsigned(row.near_feature) : 2u;
    if (d.q_bad) {
      ++q_location[row.surface];
      ++q_features[feature];
      ++q_hist[miss_bin(std::sqrt(d.q))];
    }
    if (d.j_bad) {
      ++j_location[row.surface];
      ++j_features[feature];
      ++j_hist[miss_bin(std::sqrt(d.j))];
    }
    if (bad) {
      ++bad_total;
      bool short09 = false, short05 = false;
      for (const auto& edge : row.edges) {
        const auto length = selection.census.edges.at(edge).measure.length;
        short09 = short09 || length < .9L;
        short05 = short05 || length < .5L;
      }
      bad_short09 += short09;
      bad_short05 += short05;
    }
    if (row.geometry_invalid) {
      ++geometry_hist.at(row.geometry_excess_bin);
      if (!row.geometry_types) ++geometry_types[7];
      for (unsigned type = 0; type < 7; ++type)
        if (row.geometry_types & (1u << type)) ++geometry_types[type];
    }
    const auto repairs = selection.queues.cell_repairs.find(item.first);
    if (repairs == selection.queues.cell_repairs.end() || repairs->second.empty()) {
      missing_features += bad;
      continue;
    }
    bool blocked = true;
    for (const auto& key : repairs->second) {
      const auto demand = selection.queues.repair_users.find(key);
      if (demand == selection.queues.repair_users.end() || !demand->second.users) {
        missing_features += bad;
        blocked = false;
        break;
      }
      if (!demand->second.blocked) {
        blocked = false;
        break;
      }
    }
    stuck_required += blocked;
    stuck_shape += blocked && bad;
  }
  for (const auto& feature : selection.queues.repair_users)
    blocked_features += feature.second.users && feature.second.blocked;
}

// Prints the original quality rows when timings are enabled.

// Continues the original quality output on the current stdout stream.
static void PrintQualityDetails(EngineContext& context, uint64_t quality_census_serial, const QualityRepairStats& q,
                                const QualityRepairStats& old, const std::array<uint64_t, 2>& q_location,
                                const std::array<uint64_t, 2>& j_location, const std::array<uint64_t, 3>& q_features,
                                const std::array<uint64_t, 3>& j_features, const std::array<uint64_t, 6>& q_hist,
                                const std::array<uint64_t, 6>& j_hist, const std::array<uint64_t, 6>& geometry_hist,
                                const std::array<uint64_t, 8>& geometry_types, uint64_t bad_total, uint64_t bad_short09,
                                uint64_t bad_short05, uint64_t stuck_shape, uint64_t stuck_required,
                                uint64_t missing_features, uint64_t blocked_features) {
  auto& world = context.world;
  auto histogram = [&](const auto& values) {
    for (size_t i = 0; i < values.size(); ++i) {
      if (i) std::cout << ',';
      std::cout << values[i];
    }
  };
  std::cout << "NATIVE3D_QUALITY_LOCATION rank=" << world.rank << " census=" << quality_census_serial
            << " q_surface=" << q_location[1] << " q_interior=" << q_location[0] << " j_surface=" << j_location[1]
            << " j_interior=" << j_location[0] << " q_feature=" << q_features[1] << " q_no_feature=" << q_features[0]
            << " q_feature_unknown=" << q_features[2] << " j_feature=" << j_features[1]
            << " j_no_feature=" << j_features[0] << " j_feature_unknown=" << j_features[2] << " bad_union=" << bad_total
            << " short09=" << bad_short09 << " short05=" << bad_short05
            << " short09_share=" << (bad_total ? double(bad_short09) / bad_total : 0.)
            << " short05_share=" << (bad_total ? double(bad_short05) / bad_total : 0.) << " q_miss_hist=";
  histogram(q_hist);
  std::cout << " j_miss_hist=";
  histogram(j_hist);
  std::cout << " geometry_excess_hist=";
  histogram(geometry_hist);
  std::cout << " geometry_authority_types=";
  histogram(geometry_types);
  std::cout << '\n';
  for (unsigned kind = 0; kind < 3; ++kind) {
    const auto& now = q.ladder[kind];
    const auto& prior = old.ladder[kind];
    std::cout << "NATIVE3D_QUALITY_LADDER rank=" << world.rank << " census=" << quality_census_serial
              << " kind=" << kind << " trials=" << now.trials - prior.trials
              << " certified=" << now.certified - prior.certified << " commits=" << now.published - prior.published
              << " optimizer_work=" << now.work - prior.work << " seconds=" << now.seconds - prior.seconds
              << " debt_removed=" << double(now.debt_removed - prior.debt_removed)
              << " q_removed=" << now.q_removed - prior.q_removed << " j_removed=" << now.j_removed - prior.j_removed
              << " rejected=";
    for (size_t reason = 0; reason < now.rejected.size(); ++reason) {
      if (reason) std::cout << ',';
      std::cout << now.rejected[reason] - prior.rejected[reason];
    }
    std::cout << '\n';
  }
}
void EngineReports::PrintQualityCensus(
    EngineContext& context, const char* stage, const StageProgress& p, double elapsed, double tracked_collective,
    uint64_t construction_edits, const std::array<uint64_t, 2>& q_location, const std::array<uint64_t, 2>& j_location,
    const std::array<uint64_t, 3>& q_features, const std::array<uint64_t, 3>& j_features,
    const std::array<uint64_t, 6>& q_hist, const std::array<uint64_t, 6>& j_hist,
    const std::array<uint64_t, 6>& geometry_hist, const std::array<uint64_t, 8>& geometry_types, uint64_t bad_total,
    uint64_t bad_short09, uint64_t bad_short05, uint64_t stuck_shape, uint64_t stuck_required,
    uint64_t missing_features, uint64_t blocked_features) {
  auto& world = context.world;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& previous_quality = *context.control.previous_quality;
  auto& previous_quality_census = *context.control.previous_quality_census;
  auto& previous_quality_elapsed = *context.control.previous_quality_elapsed;
  auto& previous_quality_collective = *context.control.previous_quality_collective;
  auto& previous_quality_construction = *context.control.previous_quality_construction;
  auto& previous_quality_ownership = *context.control.previous_quality_ownership;
  auto& quality_census_serial = *context.control.quality_census_serial;
  const auto& q = stats.quality;
  const auto& old = previous_quality;
  const double seconds = elapsed - previous_quality_elapsed;
  if (options.timings) {
    std::cout << std::setprecision(17) << "NATIVE3D_QUALITY rank=" << world.rank << " stage=" << stage
              << " census=" << quality_census_serial << " complete=1 round=" << stats.rounds << " elapsed=" << elapsed
              << " window_seconds=" << seconds << " busy_seconds=" << q.busy_seconds - old.busy_seconds
              << " join_call_seconds=" << q.join_wait_seconds - old.join_wait_seconds
              << " private_work_used=" << q.private_work_used - old.private_work_used
              << " tracked_collective_call_seconds=" << tracked_collective - previous_quality_collective
              << " diagnostic_seconds=" << q.diagnostic_seconds - old.diagnostic_seconds
              << " commits=" << q.commits - old.commits << " commits_total=" << q.commits
              << " private_tracked_collectives=" << q.private_tracked_collectives - old.private_tracked_collectives
              << " owned_q_removed=" << int64_t(previous_quality_census.q_bad_cells) - int64_t(p.q_bad_cells)
              << " owned_j_removed=" << int64_t(previous_quality_census.j_bad_cells) - int64_t(p.j_bad_cells)
              << " owned_debt_removed="
              << double(previous_quality_census.q_hinge_squared + previous_quality_census.j_hinge_squared -
                        p.q_hinge_squared - p.j_hinge_squared)
              << " q_removed=" << q.q_removed - old.q_removed << " q_created=" << q.q_created - old.q_created
              << " j_removed=" << q.j_removed - old.j_removed << " j_created=" << q.j_created - old.j_created
              << " q_retired=" << q.q_retired - old.q_retired << " q_introduced=" << q.q_introduced - old.q_introduced
              << " j_retired=" << q.j_retired - old.j_retired << " j_introduced=" << q.j_introduced - old.j_introduced
              << " unmeasured_churn=" << q.unmeasured_churn - old.unmeasured_churn
              << " construction_edits=" << construction_edits - previous_quality_construction
              << " ownership_changes=" << stats.ownership_changes - previous_quality_ownership
              << " queue=" << selection.queues.repair_queue.size() << " blocked_features=" << blocked_features
              << " stuck_shape=" << stuck_shape << " stuck_required=" << stuck_required
              << " missing_features=" << missing_features << '\n';
    PrintQualityDetails(context, quality_census_serial, q, old, q_location, j_location, q_features, j_features, q_hist,
                        j_hist, geometry_hist, geometry_types, bad_total, bad_short09, bad_short05, stuck_shape,
                        stuck_required, missing_features, blocked_features);
  }
}

// Prints one blocked shape cell from the certified census, keeping the feature order.
static void PrintStuckCell(const Selection& selection, int rank, uint64_t round, const char* stage, Id cell,
                           const Selection::CellState& row, const std::vector<RepairFeature>& features) {
  long double shortest = INFINITY, longest = -INFINITY;
  for (const auto& edge : row.edges) {
    const auto length = selection.census.edges.at(edge).measure.length;
    shortest = std::min(shortest, length);
    longest = std::max(longest, length);
  }
  std::cout << "NATIVE3D_M28_STUCK rank=" << rank << " stage=" << stage << " round=" << round << " cell=" << cell
            << " surface=" << unsigned(row.surface) << " q_bad=" << unsigned(row.debt.q_bad)
            << " j_bad=" << unsigned(row.debt.j_bad) << " q_debt=" << row.debt.q << " j_debt=" << row.debt.j
            << " shortest=" << shortest << " longest=" << longest << " epoch_now=" << selection.queues.repair_epoch
            << " features=" << features.size() << " f=";
  for (auto feature = features.begin(); feature != features.end(); ++feature) {
    if (feature != features.begin()) std::cout << ',';
    const auto& [count, vertices] = *feature;
    const auto& demand = selection.queues.repair_users.at(*feature);
    std::cout << count << ':' << vertices[0] << ':' << vertices[1] << ':' << vertices[2] << ':' << demand.epoch << ':'
              << unsigned(demand.refusals[0]) << ':' << unsigned(demand.refusals[1]) << ':'
              << unsigned(demand.refusals[2]);
  }
  std::cout << '\n';
}

constexpr size_t MaximumPrintedStuckCells = 8;

// Keeps the M28 and M29 seed predicates identical.
static const std::vector<RepairFeature>* BlockedShapeRepairs(const Selection& selection, Id cell,
                                                             const Selection::CellState& row) {
  if (!row.debt.q_bad && !row.debt.j_bad) return nullptr;
  const auto repairs = selection.queues.cell_repairs.find(cell);
  if (repairs == selection.queues.cell_repairs.end() || repairs->second.empty()) return nullptr;
  const bool blocked = std::all_of(repairs->second.begin(), repairs->second.end(), [&](const RepairFeature& feature) {
    const auto demand = selection.queues.repair_users.find(feature);
    return demand != selection.queues.repair_users.end() && demand->second.users && demand->second.blocked;
  });
  return blocked ? &repairs->second : nullptr;
}

// M28, print only: names each bad cell whose repair features are all blocked, with the refusal notes.
void EngineReports::PrintStuckCells(EngineContext& context, const char* stage) {
  const auto& selection = *context.census.selection;
  const auto rank = context.world.rank;
  const auto round = context.control.result->stats.rounds;
  const auto flags = std::cout.flags();
  const auto precision = std::cout.precision();
  std::cout << std::dec << std::defaultfloat << std::setprecision(17);
  size_t printed = 0, not_printed = 0;
  for (const auto& [cell, row] : selection.census.cells) {
    const auto* repairs = BlockedShapeRepairs(selection, cell, row);
    if (!repairs) continue;
    if (printed == MaximumPrintedStuckCells) {
      ++not_printed;
      continue;
    }
    PrintStuckCell(selection, rank, round, stage, cell, row, *repairs);
    ++printed;
  }
  if (not_printed)
    std::cout << "NATIVE3D_M28_STUCK_MORE rank=" << rank << " stage=" << stage << " round=" << round
              << " not_printed=" << not_printed << '\n';
  std::cout.flags(flags);
  std::cout.precision(precision);
}

// Collects the M28 seeds in census order and exchanges their vertex IDs.
static std::set<Id> M29SeedVertices(const EngineContext& context, std::vector<Id>& seeds) {
  const auto& selection = *context.census.selection;
  std::vector<Id> vertices;
  for (const auto& [id, row] : selection.census.cells) {
    if (!BlockedShapeRepairs(selection, id, row)) continue;
    seeds.push_back(id);
    vertices.insert(vertices.end(), row.vertices.begin(), row.vertices.end());
    if (seeds.size() == MaximumPrintedStuckCells) break;
  }
  const auto all = CPassiveComm::Allgatherv(vertices, nullptr, context.world.comm);
  return {all.begin(), all.end()};
}
// Tests vertex incidence without expanding a cell into an auxiliary record.
static bool M29Touches(const Cell& cell, const std::set<Id>& vertices) {
  return std::any_of(cell.v.begin(), cell.v.end(), [&](const Node& node) { return vertices.count(node.id); });
}

// A face on another rank can constrain a vertex at the fringe of the dump.
static void M29CollectConstraints(const EngineContext& context, const OwnedCell& owned, std::set<Id>& constrained) {
  for (unsigned face = 0; face < 4; ++face) {
    const auto& binding = owned.physical[face];
    if (!binding.present) continue;
    for (unsigned vertex = 0; vertex < 4; ++vertex)
      if (vertex != face) constrained.insert(owned.cell.v[vertex].id);
    if (context.composition && !context.options.wall_heights.empty() &&
        context.options.wall_heights.count(context.geometry.Get(binding.facet).marker))
      constrained.insert(owned.cell.v[face].id);
  }
}

// Scans for the first ring and exchanges its vertex IDs together with the mobility constraints.
static std::set<Id> M29FirstRing(const EngineContext& context, const std::set<Id>& roots, std::set<Id>& cells,
                                 std::set<Id>& constrained) {
  std::set<Id> vertices, local_constraints;
  if (!roots.empty()) {
    for (const auto& [id, owned] : *context.ownership.working) {
      M29CollectConstraints(context, owned, local_constraints);
      if (!M29Touches(owned.cell, roots)) continue;
      cells.insert(id);
      for (const auto& node : owned.cell.v) vertices.insert(node.id);
    }
  }
  std::vector<Id> payload{vertices.size()};
  payload.insert(payload.end(), vertices.begin(), vertices.end());
  payload.insert(payload.end(), local_constraints.begin(), local_constraints.end());
  std::vector<size_t> counts;
  const auto all = CPassiveComm::Allgatherv(payload, &counts, context.world.comm);
  std::set<Id> ring;
  auto start = all.begin();
  for (const auto count : counts) {
    const auto end = start + count;
    const auto split = start + 1 + *start;
    ring.insert(start + 1, split);
    constrained.insert(split, end);
    start = end;
  }
  return ring;
}

// Reproduces the current-nodal centroid query for a positive cell and checks its cached shape.
static Tensor M29CellMetric(const EngineContext& context, const OwnedCell& owned, const Selection::CellState& row) {
  try {
    if (owned.sensor_present && row.shape_measured) {
      const auto& v = owned.cell.v;
      const auto center = Centroid(owned.cell);
      const auto sensor = EvaluateCavitySample({DonorCell{owned.cell, owned.sensor}}, center).sensor;
      const auto target = context.composition ? context.composition(center, sensor) : sensor;
      const auto shape = Measure({v[0].p, v[1].p, v[2].p, v[3].p}, target);
      if (shape.mean_ratio == row.q_value && shape.minimum_scaled_jacobian == row.j_value) return target;
    }
  } catch (const std::exception&) {
  }  // An unavailable diagnostic cannot alter the already rejected result.
  const double missing = std::numeric_limits<double>::quiet_NaN();
  return {missing, missing, missing, missing, missing, missing};
}
// Writes tensor components in the six-value order shared by both M29 columns.
static void M29PrintTensor(std::ostream& out, const Tensor& tensor) {
  out << tensor.xx << ',' << tensor.xy << ',' << tensor.xz << ',' << tensor.yy << ',' << tensor.yz << ',' << tensor.zz;
}

// Writes the cached census and the coordinates of a single owned cell.
static void M29PrintCell(const EngineContext& context, const OwnedCell& owned, unsigned level,
                         const std::set<Id>& constrained) {
  const auto& census = context.census.selection->census;
  const auto& cell = owned.cell;
  const auto& row = census.cells.at(cell.id);
  std::ostringstream line;
  line << std::setprecision(17) << "NATIVE3D_M29_CELL rank=" << context.world.rank << " level=" << level
       << " cell=" << cell.id << " surface=" << unsigned(row.surface) << " q=" << row.q_value << " j=" << row.j_value
       << " debt=" << row.debt.q + row.debt.j << " lengths=";
  for (unsigned i = 0; i < 4; ++i)
    for (unsigned j = i + 1; j < 4; ++j) {
      Edge edge{cell.v[i].id, cell.v[j].id};
      std::sort(edge.begin(), edge.end());
      if (i != 0 || j != 1) line << ',';
      line << census.edges.at(edge).measure.length;
    }
  line << " metric=";
  M29PrintTensor(line, M29CellMetric(context, owned, row));
  line << " v=" << cell.v[0].id;
  for (size_t i = 1; i < cell.v.size(); ++i) line << ',' << cell.v[i].id;
  line << " x=";
  for (size_t i = 0; i < cell.v.size(); ++i) {
    if (i) line << ',';
    const auto& point = cell.v[i].p;
    line << point.x << ',' << point.y << ',' << point.z;
  }
  line << " m=";
  for (size_t i = 0; i < owned.sensor.size(); ++i) {
    if (i) line << ',';
    M29PrintTensor(line, owned.sensor[i]);
  }
  line << " free=" << unsigned(!constrained.count(cell.v[0].id));
  for (size_t i = 1; i < cell.v.size(); ++i) line << ',' << unsigned(!constrained.count(cell.v[i].id));
  std::cout << line.str() + '\n';
}

// Scans once more and emits only cells outside the previous levels.
static size_t M29PrintSecondRing(const EngineContext& context, const std::set<Id>& first_ring,
                                 const std::set<Id>& printed, const std::set<Id>& constrained) {
  if (first_ring.empty()) return 0;
  size_t count = 0;
  for (const auto& [id, owned] : *context.ownership.working) {
    if (!M29Touches(owned.cell, first_ring) || printed.count(id)) continue;
    M29PrintCell(context, owned, 2, constrained);
    ++count;
  }
  return count;
}
// Writes the owned, lowest-level cells of the rejected exit census.
void EngineReports::PrintStuckNeighborhood(const EngineContext& context) {
  std::vector<Id> seeds;
  const auto roots = M29SeedVertices(context, seeds);
  const auto rank = context.world.rank;
  const auto round = context.control.result->stats.rounds;
  std::cout << "NATIVE3D_M29_BEGIN rank=" << rank << " round=" << round << " stuck=" << seeds.size() << '\n';
  std::set<Id> cells, constrained;
  const auto first_ring = M29FirstRing(context, roots, cells, constrained);
  const auto& working = *context.ownership.working;
  for (const Id id : seeds) M29PrintCell(context, working.at(id), 0, constrained);
  for (const Id id : cells)
    if (!std::binary_search(seeds.begin(), seeds.end(), id)) M29PrintCell(context, working.at(id), 1, constrained);
  const auto second_count = M29PrintSecondRing(context, first_ring, cells, constrained);
  std::cout << "NATIVE3D_M29_END rank=" << rank << " level0=" << seeds.size()
            << " level1=" << cells.size() - seeds.size() << " level2=" << second_count << '\n';
}

// Times, collects, prints, and advances the live quality window.
void EngineReports::EmitQualityCensus(EngineContext& context, const char* stage, const StageProgress& p) {
  auto& world = context.world;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  const auto& started = *context.setup.control_prefix.started;
  auto& previous_quality = *context.control.previous_quality;
  auto& previous_quality_census = *context.control.previous_quality_census;
  auto& previous_quality_elapsed = *context.control.previous_quality_elapsed;
  auto& previous_quality_collective = *context.control.previous_quality_collective;
  auto& previous_quality_construction = *context.control.previous_quality_construction;
  auto& previous_quality_ownership = *context.control.previous_quality_ownership;
  auto& quality_census_serial = *context.control.quality_census_serial;
  auto& quality_census_started = *context.control.quality_census_started;
  const auto began = Clock::now();
  const double elapsed = std::chrono::duration<double>(began - started).count();
  const double tracked_collective = world.collective_seconds + world.election_collective_seconds;
  const uint64_t construction_edits = stats.length_splits + stats.length_collapses;
  if (!quality_census_started) {
    previous_quality = stats.quality;
    previous_quality_census = p;
    previous_quality_elapsed = elapsed;
    previous_quality_collective = tracked_collective;
    previous_quality_construction = construction_edits;
    previous_quality_ownership = stats.ownership_changes;
    quality_census_started = true;
  }
  std::array<uint64_t, 2> q_location{}, j_location{};
  std::array<uint64_t, 3> q_features{}, j_features{};
  std::array<uint64_t, 6> q_hist{}, j_hist{}, geometry_hist{};
  std::array<uint64_t, 8> geometry_types{};
  uint64_t bad_total = 0, bad_short09 = 0, bad_short05 = 0, stuck_shape = 0, stuck_required = 0, missing_features = 0,
           blocked_features = 0;
  CollectQualityCensus(selection, q_location, j_location, q_features, j_features, q_hist, j_hist, geometry_hist,
                       geometry_types, bad_total, bad_short09, bad_short05, stuck_shape, stuck_required,
                       missing_features, blocked_features);
  stats.quality.diagnostic_seconds += std::chrono::duration<double>(Clock::now() - began).count();
  PrintQualityCensus(context, stage, p, elapsed, tracked_collective, construction_edits, q_location, j_location,
                     q_features, j_features, q_hist, j_hist, geometry_hist, geometry_types, bad_total, bad_short09,
                     bad_short05, stuck_shape, stuck_required, missing_features, blocked_features);
  if (context.options.timings && stuck_shape != 0) PrintStuckCells(context, stage);
  previous_quality = stats.quality;
  previous_quality_census = p;
  previous_quality_elapsed = elapsed;
  previous_quality_collective = tracked_collective;
  previous_quality_construction = construction_edits;
  previous_quality_ownership = stats.ownership_changes;
  ++quality_census_serial;
}

// Stage report helpers retain the original stream and row order.
#include "EngineReports.hpp"

// Emits the first half of the original stage row on the same stream.
static std::ostream& NativeStagePrefix(EngineContext& context, const char* stage, const StageProgress& p,
                                       const BoundaryParameterState* state,
                                       SU2NativeGeometry::AtlasBudget& atlas_budget, size_t face_pieces,
                                       size_t trace_pieces, size_t limbs,
                                       const SU2NativeGeometry::MappedFacetReference::GeometryStats& g) {
  using Phase = EnginePhase;
  auto& world = context.world;
  auto& stats = context.control.result->stats;
  auto& phase = *context.setup.control_prefix.phase;
  std::cout << std::setprecision(17) << "NATIVE3D_STAGE rank=" << world.rank << " stage=" << stage
            << " job_peak_rss_lower=" << stats.job_peak_rss_lower
            << " phase=" << (phase == Phase::Construction ? "construction" : "improvement")
            << " cells=" << p.owned_cells << " measured=" << p.measured_cells
            << " cache_consistent=" << p.cache_consistent << " census_complete=" << p.census_complete
            << " shape_complete=" << p.shape_complete << " geometry_complete=" << p.geometry_complete
            << " q_measured=" << p.q_measured_cells << " j_measured=" << p.j_measured_cells
            << " q_bad=" << p.q_bad_cells << " j_bad=" << p.j_bad_cells << " size_bad=" << p.size_bad_cells
            << " geometry_bad=" << p.geometry_bad_cells << " q_debt=" << p.q_hinge_squared
            << " j_debt=" << p.j_hinge_squared << " size_debt=" << p.size_hinge_squared
            << " geometry_debt=" << p.geometry_excess_squared << " distinct_facets=" << g.distinct_facets
            << " distinct_scope_keys=" << g.distinct_scope_keys << " prepared_facet_builds=" << g.prepared_facet_builds
            << " prepared_facet_mean_bytes=" << g.prepared_facet_mean_bytes
            << " associations=" << (state ? state->Records().size() : 0)
            << " working_faces=" << (state ? state->Surface().size() : 0) << " face_pieces=" << face_pieces
            << " trace_pieces=" << trace_pieces << " maximum_ratio_limbs=" << limbs
            << " atlas_work=" << atlas_budget.work << " atlas_bytes=" << atlas_budget.peak_bytes
            << " global_work=" << stats.global_work << " rounds=" << stats.rounds
            << " batch_rounds=" << stats.batch_rounds << " collapse_corrections=" << stats.collapse_corrections
            << " collapse_correction_cells=" << stats.collapse_correction_cells
            << " collapse_correction_work=" << stats.collapse_correction_work
            << " collapse_correction_seconds=" << stats.collapse_correction_seconds
            << " batch_candidates=" << stats.batch_candidates << " batch_reserved=" << stats.batch_reserved
            << " quality_progress=" << stats.quality_progress << " accepted=" << stats.accepted
            << " updated_cells=" << stats.updated_cells << " update_seconds=" << stats.update_seconds
            << " surface_measurement_skipped=" << stats.surface_measurement_skipped
            << " surface_required_handoffs=" << stats.surface_required_handoffs
            << " surface_demand_required=" << stats.surface_demand_required_cells
            << " surface_demand_measured=" << stats.surface_demand_measured_cells
            << " surface_demand_complete=" << stats.surface_demand_census_complete
            << " surface_geometry_complete=" << stats.surface_demand_geometry_complete
            << " seed_cell_classes=" << stats.seed_cell_classes[0] << ',' << stats.seed_cell_classes[1] << ','
            << stats.seed_cell_classes[2] << " updated_cell_classes=" << stats.updated_cell_classes[0] << ','
            << stats.updated_cell_classes[1] << ',' << stats.updated_cell_classes[2]
            << " geometry_trials=" << stats.geometry_trials << " geometry_rejected=" << stats.geometry_rejected
            << " guidance_trials=" << stats.geometry_guidance_trials
            << " guidance_rejected=" << stats.geometry_guidance_rejected
            << " guidance_faces=" << stats.geometry_guidance_faces
            << " guidance_early_rejected=" << stats.geometry_guidance_early_rejections
            << " guidance_seconds=" << stats.geometry_guidance_seconds;
  return std::cout;
}

// Continues the stage row and terminates it on the same stream.
static void NativeStageSuffixWork(std::ostream& out, EngineContext& context, const Clock::time_point& started,
                                  const uint64_t& edge_census_round,
                                  const SU2NativeGeometry::MappedFacetReference::GeometryStats& g);

static void NativeStageSuffix(std::ostream& out, EngineContext& context,
                              const SU2NativeGeometry::MappedFacetReference::GeometryStats& g) {
  using Phase = EnginePhase;
  auto& world = context.world;
  auto& stats = context.control.result->stats;
  const auto& geometry_begin = *context.control.geometry_begin;
  const auto& started = *context.setup.control_prefix.started;
  auto& edge_census_round = *context.census.edge_census_round;
  const auto& tracked_calls_begin = *context.control.tracked_calls_begin;
  const auto& tracked_seconds_begin = *context.control.tracked_seconds_begin;
  const auto& election_calls_begin = *context.control.election_calls_begin;
  const auto& election_seconds_begin = *context.control.election_seconds_begin;
  out << " collective_calls=" << world.collective_calls - tracked_calls_begin
      << " collective_seconds=" << world.collective_seconds - tracked_seconds_begin
      << " election_calls=" << world.election_collective_calls - election_calls_begin
      << " election_seconds=" << world.election_collective_seconds - election_seconds_begin
      << " direct_calls=" << g.direct_calls - geometry_begin.direct_calls
      << " direct_exceptions=" << g.direct_exceptions - geometry_begin.direct_exceptions
      << " direct_work=" << g.direct_work - geometry_begin.direct_work
      << " direct_seconds=" << g.direct_seconds - geometry_begin.direct_seconds
      << " upper_calls=" << g.upper_calls - geometry_begin.upper_calls
      << " union_exceptions=" << g.union_exceptions - geometry_begin.union_exceptions
      << " upper_refusals=" << g.upper_refusals - geometry_begin.upper_refusals
      << " upper_work=" << g.upper_work - geometry_begin.upper_work
      << " upper_refinements=" << g.upper_refinements - geometry_begin.upper_refinements
      << " upper_deferred_bounds=" << g.upper_deferred_bounds - geometry_begin.upper_deferred_bounds
      << " classification_calls=" << g.classification_calls - geometry_begin.classification_calls
      << " classification_work=" << g.classification_work - geometry_begin.classification_work
      << " magnitude_calls=" << g.magnitude_calls - geometry_begin.magnitude_calls
      << " magnitude_work=" << g.magnitude_work - geometry_begin.magnitude_work
      << " limit_calls=" << g.limit_calls - geometry_begin.limit_calls
      << " limit_work=" << g.limit_work - geometry_begin.limit_work
      << " upper_seconds=" << g.upper_seconds - geometry_begin.upper_seconds
      << " full_calls=" << g.full_calls - geometry_begin.full_calls
      << " full_work=" << g.full_work - geometry_begin.full_work
      << " full_seconds=" << g.full_seconds - geometry_begin.full_seconds
      << " facet_hits=" << g.facet_hits - geometry_begin.facet_hits
      << " facet_misses=" << g.facet_misses - geometry_begin.facet_misses
      << " facet_resolutions=" << g.facet_resolutions << " scope_hits=" << g.scope_hits - geometry_begin.scope_hits
      << " scope_misses=" << g.scope_misses - geometry_begin.scope_misses
      << " scope_attempts=" << g.scope_attempts - geometry_begin.scope_attempts
      << " scope_builds=" << g.scope_builds - geometry_begin.scope_builds
      << " scope_build_work=" << g.scope_build_work - geometry_begin.scope_build_work
      << " scope_build_seconds=" << g.scope_build_seconds - geometry_begin.scope_build_seconds
      << " support_calls=" << g.support_calls - geometry_begin.support_calls
      << " support_facets=" << g.support_facets - geometry_begin.support_facets
      << " support_seconds=" << g.support_seconds - geometry_begin.support_seconds
      << " facet_build_work=" << g.facet_build_work - geometry_begin.facet_build_work
      << " facet_build_seconds=" << g.facet_build_seconds - geometry_begin.facet_build_seconds
      << " piece_selection_work=" << g.piece_selection_work - geometry_begin.piece_selection_work
      << " piece_selection_seconds=" << g.piece_selection_seconds - geometry_begin.piece_selection_seconds
      << " traversal_work=" << g.traversal_work - geometry_begin.traversal_work
      << " traversal_seconds=" << g.traversal_seconds - geometry_begin.traversal_seconds
      << " selected_pieces=" << g.selected_pieces - geometry_begin.selected_pieces
      << " support_pieces=" << g.support_pieces - geometry_begin.support_pieces;
  NativeStageSuffixWork(out, context, started, edge_census_round, g);
}

// Continues the original stage row on the same stream without a flush or new query.
static void NativeStageSuffixWork(std::ostream& out, EngineContext& context, const Clock::time_point& started,
                                  const uint64_t& edge_census_round,
                                  const SU2NativeGeometry::MappedFacetReference::GeometryStats& g) {
  auto& stats = context.control.result->stats;
  const auto& geometry_begin = *context.control.geometry_begin;
  out << " smooth_boundary_stars=" << stats.boundary_smoothing_stars
      << " smooth_boundary_reused=" << stats.boundary_smoothing_reused
      << " smooth_boundary_candidates=" << stats.boundary_smoothing_candidates
      << " smooth_boundary_rejects=" << stats.boundary_smoothing_validity_rejects << ','
      << stats.boundary_smoothing_shape_rejects << ',' << stats.boundary_smoothing_edge_rejects << ','
      << stats.boundary_smoothing_geometry_rejects
      << " smooth_boundary_guidance=" << stats.boundary_smoothing_guidance_calls
      << " smooth_boundary_upper_queries=" << stats.boundary_smoothing_upper_queries
      << " smooth_boundary_move_contexts=" << stats.boundary_smoothing_move_contexts
      << " smooth_boundary_prepared_trials=" << stats.boundary_smoothing_prepared_trials
      << " smooth_boundary_guidance_reused_faces=" << stats.boundary_smoothing_guidance_reused_faces
      << " smooth_boundary_guidance_faces=" << stats.boundary_smoothing_guidance_faces
      << " smooth_boundary_guidance_early_rejected=" << stats.boundary_smoothing_guidance_early_rejections
      << " smooth_boundary_moves=" << stats.boundary_smoothing_moves
      << " smooth_boundary_debt_reduction=" << double(stats.boundary_smoothing_debt_reduction)
      << " smooth_boundary_seconds=" << stats.boundary_smoothing_seconds
      << " smooth_interior_stars=" << stats.interior_smoothing_stars
      << " smooth_interior_reused=" << stats.interior_smoothing_reused
      << " smooth_interior_candidates=" << stats.interior_smoothing_candidates
      << " smooth_interior_rejects=" << stats.interior_smoothing_validity_rejects << ','
      << stats.interior_smoothing_shape_rejects << ',' << stats.interior_smoothing_edge_rejects << ','
      << stats.interior_smoothing_geometry_rejects
      << " smooth_interior_guidance=" << stats.interior_smoothing_guidance_calls
      << " smooth_interior_upper_queries=" << stats.interior_smoothing_upper_queries
      << " smooth_interior_moves=" << stats.interior_smoothing_moves
      << " smooth_interior_debt_reduction=" << double(stats.interior_smoothing_debt_reduction)
      << " smooth_interior_seconds=" << stats.interior_smoothing_seconds
      << " retention_refusals=" << g.retention_refusals - geometry_begin.retention_refusals
      << " prepared_bytes=" << g.prepared_bytes << " prepared_limit=" << g.prepared_limit
      << " facet_bytes=" << g.facet_bytes << " scope_bytes=" << g.scope_bytes << " facet_entries=" << g.facet_entries
      << " scope_entries=" << g.scope_entries << " cache_evictions=" << g.evictions
      << " cache_evicted_bytes=" << g.evicted_bytes
      << " memo_direct_hits=" << g.memo_direct_hits - geometry_begin.memo_direct_hits
      << " memo_direct_misses=" << g.memo_direct_misses - geometry_begin.memo_direct_misses
      << " memo_union_hits=" << g.memo_union_hits - geometry_begin.memo_union_hits
      << " memo_union_misses=" << g.memo_union_misses - geometry_begin.memo_union_misses
      << " memo_stores=" << g.memo_stores - geometry_begin.memo_stores
      << " memo_evictions=" << g.memo_evictions - geometry_begin.memo_evictions
      << " memo_evicted_bytes=" << g.memo_evicted_bytes - geometry_begin.memo_evicted_bytes
      << " memo_retention_refusals=" << g.memo_retention_refusals - geometry_begin.memo_retention_refusals
      << " memo_maintenance_work=" << g.memo_maintenance_work - geometry_begin.memo_maintenance_work
      << " memo_bytes=" << g.memo_bytes << " memo_limit=" << g.memo_limit
      << " memo_direct_entries=" << g.memo_direct_entries << " memo_union_entries=" << g.memo_union_entries
      << " length_splits=" << stats.length_splits << " length_collapses=" << stats.length_collapses
      << " split_passes=" << stats.length_split_passes << " collapse_passes=" << stats.length_collapse_passes
      << " quality_stages=" << stats.quality_stages << " split_generation=" << stats.split_generation
      << " split_lane=" << stats.split_lane << " surface_rounds=" << stats.surface_rounds
      << " interior_rounds=" << stats.interior_rounds << " surface_seconds=" << stats.surface_seconds
      << " interior_seconds=" << stats.interior_seconds << " surface_conflicts=" << stats.surface_conflicts
      << " interior_conflicts=" << stats.interior_conflicts
      << " interior_boundary_discoveries=" << stats.interior_boundary_discoveries
      << " unique_long_surface=" << stats.unique_long_surface << " unique_long_interior=" << stats.unique_long_interior
      << " unique_edge_debt=" << stats.unique_edge_debt << " maximum_edge=" << stats.maximum_edge
      << " edge_census_round=" << edge_census_round << " edge_census_current=" << (edge_census_round == stats.rounds)
      << " edge_histogram=" << stats.edge_histogram[0] << ',' << stats.edge_histogram[1] << ','
      << stats.edge_histogram[2] << " child_parent_max=" << stats.maximum_child_parent_ratio
      << " child_ratio_measured=" << stats.child_ratio_measured
      << " elapsed=" << std::chrono::duration<double>(Clock::now() - started).count() << std::endl;
}

// Counts coverage and emits stage/edge reports in original order.
void EngineReports::EmitStageRows(EngineContext& context, const char* stage, const StageProgress& p,
                                  const BoundaryParameterState* state, SU2NativeGeometry::AtlasBudget& atlas_budget) {
  auto& world = context.world;
  auto& stats = context.control.result->stats;
  auto& geometry_authority = *context.control.geometry_authority;
  auto& edge_progress = *context.census.edge_progress;
  size_t face_pieces = 0, trace_pieces = 0, limbs = 0;
  auto ratio = [&](const SU2NativeGeometry::ExactRatio& r) {
    limbs = std::max({limbs, r.numerator.size(), r.denominator.size()});
  };
  auto bary = [&](const SU2NativeGeometry::ExactBary& b) {
    ratio(b.u);
    ratio(b.v);
  };
  if (state) {
    for (const auto& row : state->Coverage().faces)
      for (const auto& piece : row.second.pieces) {
        ++face_pieces;
        for (const auto& b : piece.reference) bary(b);
        for (const auto& b : piece.active) bary(b);
      }
    for (const auto& row : state->Coverage().traces)
      for (const auto& piece : row.second.pieces) {
        ++trace_pieces;
        bary(piece.first);
        bary(piece.last);
        ratio(piece.end);
      }
  }
  const auto g = geometry_authority ? geometry_authority->GeometryStatsSnapshot()
                                    : SU2NativeGeometry::MappedFacetReference::GeometryStats{};
  auto& out = NativeStagePrefix(context, stage, p, state, atlas_budget, face_pieces, trace_pieces, limbs, g);
  NativeStageSuffix(out, context, g);
  if (std::string(stage) == "length_split_exit")
    for (const auto& edge : edge_progress.top)
      std::cout << "NATIVE3D_EDGE rank=" << world.rank << " generation=" << stats.split_generation
                << " a=" << edge.edge[0] << " b=" << edge.edge[1] << " length=" << edge.length
                << " surface=" << edge.surface << " age=" << edge.age << " disposition=" << edge.disposition
                << " disposition_round=" << edge.disposition_round << '\n';
}

// Prints the existing per-rank length-pass row at the generation boundary.
void EngineReports::EmitLengthPass(EngineContext& context, SU2NativeGeometry::AtlasBudget& atlas_budget) {
  auto& world = context.world;
  auto& options = context.options;
  auto& working = *context.ownership.working;
  auto& started = *context.setup.control_prefix.started;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& coarsening = *context.control.coarsening;
  if (options.timings) {
    const auto progress = selection.Progress(working.size());
    size_t short_edges = 0, long_edges = 0;
    long double minimum = INFINITY, maximum = 0;
    for (const auto& edge : selection.census.edges) {
      const auto length = edge.second.measure.length;
      short_edges += length < CoarseningMetricEdge;
      long_edges += length > MaximumMetricEdge;
      minimum = std::min(minimum, length);
      maximum = std::max(maximum, length);
    }
    std::cout << std::setprecision(17) << "NATIVE3D_LENGTH rank=" << world.rank
              << " pass=" << (coarsening ? "collapse" : "split") << " cells=" << working.size()
              << " local_edges=" << selection.census.edges.size() << " short_local_edges=" << short_edges
              << " long_local_edges=" << long_edges
              << " minimum_length=" << (selection.census.edges.empty() ? 0 : minimum) << " maximum_length=" << maximum
              << " size_hinge_incidence=" << progress.size_hinge_squared
              << " geometry_bad_cells=" << progress.geometry_bad_cells << " size_bad_cells=" << progress.size_bad_cells
              << " q_measured_cells=" << progress.q_measured_cells << " j_measured_cells=" << progress.j_measured_cells
              << " construction_stall_handoffs=" << stats.construction_stall_handoffs
              << " maximum_parallel_split_commits=" << stats.maximum_parallel_split_commits
              << " split_accepted=" << stats.length_splits << " collapse_accepted=" << stats.length_collapses
              << " atlas_work=" << atlas_budget.work << " admitted_peak_bytes=" << stats.maximum_work_bytes
              << " elapsed=" << std::chrono::duration<double>(Clock::now() - started).count() << '\n';
  }
}
