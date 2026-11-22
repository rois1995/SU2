// Measures the canonical Selection; owns no extra state and preserves the original protocol.
#include "EngineCensus.hpp"
#include "EngineContext.hpp"
#include "EngineTrial.hpp"

EngineCensus::EngineCensus() = default;
EngineCensus::~EngineCensus() = default;

// Census workers borrow the live Adapt state; none stores a copied payload.
bool EngineCensus::ScopeCensusComplete(EngineContext& context) const {
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  if (!surface_target) return selection.Progress(working.size()).census_complete;
  if (!selection.census.progress_consistent || !selection.census.census_started) return false;
  const auto count = selection.SurfaceCensus(working);
  return count[0] == count[1];
}

void EngineCensus::RefreshSurfaceCensus(EngineContext& context) const {
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  if (!surface_target) return;
  const auto count = selection.SurfaceCensus(working);
  stats.surface_demand_required_cells = count[0];
  stats.surface_demand_measured_cells = selection.census.progress_consistent ? count[1] : 0;
  stats.surface_demand_census_complete =
      selection.census.progress_consistent && selection.census.census_started && count[0] == count[1];
  stats.surface_demand_geometry_complete = stats.surface_demand_census_complete && count[0] == count[2];
}

bool EngineCensus::GenerationCensus(EngineContext& context) {
  auto& selection = *context.census.selection;
  auto& surface_target = context.surface_target;
  auto& edge_progress = *context.census.edge_progress;
  auto& edge_census_round = *context.census.edge_census_round;
  auto& world = context.world;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& options = context.options;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  SU2NativeTiming3D::Scope scheduling_timer(SU2NativeTiming3D::Operation::Scheduling);
  const auto extra =
      transfer_memory::Mul(selection.census.edges.size(),
                           sizeof(EdgeProgressSample) + sizeof(Selection::History) + sizeof(Edge) + 4 * sizeof(void*));
  if (!context.exchange.Admit(context, extra)) return false;
  CLocalFailure failure;
  std::vector<EdgeProgressSample> samples;
  try {
    selection.RefreshHistory(false);
    samples.reserve(selection.census.edges.size());
    for (const auto& row : selection.census.edges) {
      if (surface_target && !row.second.physical_users) continue;
      const auto h = selection.queues.edge_history.find(row.first);
      const auto length = row.second.measure.length;
      double value = double(length);
      if (value < length) value = std::nextafter(value, INFINITY);
      samples.push_back({row.first, value, row.second.physical_users != 0,
                         h == selection.queues.edge_history.end() ? 0 : h->second.age,
                         h == selection.queues.edge_history.end() ? 0 : h->second.disposition,
                         h == selection.queues.edge_history.end() ? 0 : h->second.round});
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  if (!MeasureGenerationProgress(
          world, samples, edge_progress, result.reason, options.memory_ceiling,
          transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(), directory->ResidentBytes(),
                               transfer_memory::Bytes(edge_progress.top))))
    return false;
  stats.unique_long_surface = edge_progress.long_surface;
  stats.unique_long_interior = edge_progress.long_interior;
  stats.unique_edge_debt = edge_progress.hinge_squared;
  stats.maximum_edge = edge_progress.max_length;
  stats.edge_histogram = edge_progress.histogram;
  edge_census_round = stats.rounds;
  return true;
}

GenerationDebt EngineCensus::CurrentGenerationDebt(EngineContext& context) const {
  auto& edge_progress = *context.census.edge_progress;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& surface_target = context.surface_target;
  auto& world = context.world;
  GenerationDebt d;
  d.edges = edge_progress.hinge_squared;
  d.maximum = edge_progress.max_length;
  const auto p = selection.Progress(working.size());
  d.geometry = TimedAllreduce(double(p.geometry_excess_squared), CPassiveComm::Op::SUM, world.comm);
  bool geometry_complete = p.geometry_complete;
  if (surface_target) {
    const auto count = selection.SurfaceCensus(working);
    geometry_complete = selection.census.progress_consistent && selection.census.census_started &&
                        count[0] == count[1] && count[0] == count[2];
  }
  d.geometry_complete = !world.maximum(!geometry_complete);
  return d;
}

bool EngineCensus::ImportMeasurementBatch(EngineContext& context, const std::vector<Id>& ids, size_t slot,
                                          size_t& batch, size_t plan_scratch, double* enclosing_seconds,
                                          CLocalFailure& failure, std::map<Edge, EdgeMeasure>& reuse,
                                          DonorPatch& imported) {
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  auto& retired_edges = *context.census.retired_edges;
  auto& field_identity = *context.census.field_identity;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& options = context.options;
  for (;;) {
    std::vector<Cell> regions;
    try {
      reuse.clear();
      for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i) {
        const auto& cell = working.at(ids[i]).cell;
        regions.push_back(cell);
        selection.Cached(cell, reuse);
        for (auto a : cell.v)
          for (auto b : cell.v)
            if (a.id < b.id) {
              const auto prior = retired_edges.find({a.id, b.id});
              if (prior != retired_edges.end()) reuse.emplace(prior->first, prior->second);
            }
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return false;
    std::vector<unsigned char> requests;
    try {
      requests = MetricEdgeRequests(regions, reuse, field_identity, &campaign_edges);
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return false;
    {
      Timer timer(stats.donor_seconds, options.timings, enclosing_seconds, SU2NativeTiming3D::Operation::Import);
      imported = context.exchange.ImportSource(
          context, regions, transfer_memory::Add(MaximumFrozenFieldBytes, plan_scratch), false, &requests);
    }
    if (imported.valid) return true;
    if (batch == 1) {
      result.reason = imported.reason;
      if (context.exchange.deferred_votes->active) failure.Set(2, 0, imported.reason);
      return false;
    }
    batch = (batch + 1) / 2;
    ++stats.batch_retries;
  }
}

void EngineCensus::MeasureCell(EngineContext& context, Id id, bool seed, FrozenField& field, PatchMetric& planned,
                               std::map<Edge, EdgeMeasure>& reuse) {
  using Phase = EnginePhase;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& field_identity = *context.census.field_identity;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& phase = *context.setup.control_prefix.phase;
  auto& options = context.options;
  auto& stats = context.control.result->stats;

  PatchMetric metric;
  std::string reason;
  MetricStats counted;
  const auto& cell = working.at(id).cell;
  bool strict = false, quality_valid = false, height_valid = true;
  FirstHeightReport height;
  if (phase == Phase::Construction) {
    if (surface_target) {
      const auto& owned = working.at(id);
      for (unsigned a = 0; a < 4; ++a)
        for (unsigned b = a + 1; b < 4; ++b) {
          Edge key{cell.v[a].id, cell.v[b].id};
          std::sort(key.begin(), key.end());
          metric.edges.emplace(key, planned.edges.at(key));
        }
      for (const auto& edge : metric.edges)
        for (unsigned face = 0; face < 4; ++face)
          if (cell.v[face].id != edge.first[0] && cell.v[face].id != edge.first[1] && owned.physical[face].present &&
              options.wall_heights.count(geometry.Get(owned.physical[face].facet).marker) &&
              edge.second.length > metric.maximum_length) {
            metric.maximum_length = edge.second.length;
            metric.longest_edge = edge.first;
          }
    } else
      metric = MeasureMetricEdges({cell}, field, &counted, &reuse, &campaign_edges);
    strict = metric.maximum_length <= MaximumMetricEdge;
  } else {
    const bool metric_valid = ValidateMetricPatch({cell}, field, metric, reason, &counted, &reuse);
    if (options.timings && !metric_valid &&
        (metric.edges.size() != 6 || !std::isfinite(metric.maximum_length))) {
      try {
        const auto cached_cell = selection.census.cells.find(id);
        if (cached_cell != selection.census.cells.end()) {
          std::ostringstream row;
          row << std::setprecision(17) << "NATIVE3D_METRIC_REJECT rank=" << context.world.rank
              << " phase=" << (phase == Phase::Improvement ? "improvement" : "construction") << " cell=" << id;
          const auto xyz = [&row](Point point) { row << point.x << ',' << point.y << ',' << point.z; };
          for (size_t k = 0; k < cell.v.size(); ++k) {
            row << " v" << k << '=' << cell.v[k].id << " v" << k << "_xyz=";
            xyz(cell.v[k].p);
          }
          row << " centroid=";
          xyz(Centroid(cell));
          row << " measured_edges=" << metric.edges.size() << " maximum_length=" << metric.maximum_length;
          const auto& edges = cached_cell->second.edges;
          const auto node = [&cell](Id vertex) {
            return std::find_if(cell.v.begin(), cell.v.end(), [vertex](const Node& n) { return n.id == vertex; });
          };
          for (size_t k = 0; k < edges.size(); ++k) {
            const auto& key = edges[k];
            const auto a = node(key[0]);
            const auto b = node(key[1]);
            const double space = a != cell.v.end() && b != cell.v.end()
                                     ? std::hypot(a->p.x - b->p.x, a->p.y - b->p.y, a->p.z - b->p.z)
                                     : -1;
            const auto cached = selection.census.edges.find(key);
            row << " e" << k << "_a=" << key[0] << " e" << k << "_b=" << key[1]
                << " e" << k << "_space=" << space << " e" << k << "_metric="
                << (cached == selection.census.edges.end() ? -1 : cached->second.measure.length);
          }
          row << " reason=" << std::quoted(reason) << '\n';
          std::cout << row.str();
        }
      } catch (...) {
        // Keep the original refresh failure even when its diagnostic cannot be printed.
      }
    }
    std::string height_reason;
    height_valid = EngineTrial::HeightCheck(context, GeometryPatch(Imported{{working.at(id)}, true, {}}, &geometry),
                                            height_reason, false, &height);
    if (!height_valid) reason = height_reason;
    quality_valid = metric.shape_measured && std::isfinite(metric.minimum_mean_ratio) &&
                    std::isfinite(metric.minimum_scaled_jacobian) && metric.minimum_mean_ratio >= MinimumMeanRatio &&
                    metric.minimum_scaled_jacobian >= MinimumScaledJacobian && height_valid;
    strict = metric_valid && height_valid;
  }
  if (phase == Phase::Improvement && selection.census.cells.count(id))
    selection.RefreshQuality(working.at(id), metric, strict, quality_valid,
                             !options.wall_heights.empty() ? &height : nullptr, height_valid, field_identity);
  else
    selection.Add(working.at(id), metric, strict, quality_valid, reason,
                  phase == Phase::Improvement && !options.wall_heights.empty() ? &height : nullptr, height_valid);
  for (const auto& edge : metric.edges) reuse.insert_or_assign(edge.first, edge.second);
  stats.edge_evaluations += counted.edge_evaluations;
  stats.edge_reused += counted.edge_reused;
  if (seed)
    ++stats.initial_cells;
  else
    ++stats.updated_cells;
  const auto& owned = working.at(id);
  const unsigned category = selection.RequestedWall(owned) ? 2 : selection.census.cells.at(id).surface ? 1 : 0;
  ++(seed ? stats.seed_cell_classes : stats.updated_cell_classes)[category];
}

bool EngineCensus::MeasureBatchCells(EngineContext& context, const std::vector<Id>& ids, size_t slot, size_t batch,
                                     bool seed, size_t plan_scratch, CLocalFailure& failure, DonorPatch& imported,
                                     std::map<Edge, EdgeMeasure>& reuse) {
  using Phase = EnginePhase;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& geometry = context.geometry;
  auto& surface_target = context.surface_target;
  auto& composition = context.composition;
  auto& field_identity = *context.census.field_identity;
  auto& phase = *context.setup.control_prefix.phase;
  auto& options = context.options;
  auto& stats = context.control.result->stats;
  if (!context.exchange.Admit(
          context, transfer_memory::Add(transfer_memory::Bytes(imported.cells), MaximumFrozenFieldBytes, plan_scratch,
                                        transfer_memory::Mul(working.size(), context.control.SelectionBound(
                                                                                 context, selection.track_repairs)))))
    return false;
  if (slot < ids.size()) try {
      CpuTimer private_cpu(stats.measurement_cpu_seconds, options.timings);
      const auto components = AssociatedExtensions(imported.cells, geometry);
      FrozenField field(std::move(imported.cells), composition, options.timings, components, options.edge_breakpoints,
                        field_identity, true, MaximumDonors, true);
      FieldAccounting accounting{stats, field};
      PatchMetric planned;
      if (surface_target && phase == Phase::Construction) {
        Imported published;
        published.cells.reserve(batch);
        for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i)
          published.cells.push_back(working.at(ids[i]));
        MetricStats counted;
        // Published coordinates/associations and the field are unchanged throughout this measurement batch.
        planned = MeasureSurfacePlanning(GeometryPatch(published, &geometry), field, geometry, options.wall_heights,
                                         surface_target, &counted);
        stats.edge_evaluations += counted.edge_evaluations;
        stats.edge_reused += counted.edge_reused;
      }
      for (size_t i = slot; i < std::min(slot + batch, ids.size()); ++i) {
        MeasureCell(context, ids[i], seed, field, planned, reuse);
      }
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
  return context.exchange.Vote(context, failure);
}

static bool StopMeasurementAtDeadline(EngineContext& context, bool seed) {
  auto& result = *context.control.result;
  result.stats.stop = EngineStop::TimeBudget;
  result.reason = seed ? "Native3D metric seeding exhausts elapsed-time budget."
                       : "Native3D metric update exhausts elapsed-time budget.";
  return false;
}

bool EngineCensus::MeasureChunks(EngineContext& context, const std::vector<Id>& ids, size_t count, bool seed,
                                 size_t plan_scratch, double* enclosing_seconds, bool deferred,
                                 CLocalFailure& first, bool& time_stop) {
  auto& options = context.options;
  auto& world = context.world;
  auto& stats = context.control.result->stats;
  const auto& started = *context.setup.control_prefix.started;
  auto& votes = *context.exchange.deferred_votes;
  std::optional<EngineExchange::DeferredVoteScope> scope;
  if (deferred) scope.emplace(votes);
  bool complete = true;
  for (size_t slot = 0; slot < count;) {
    if (options.maximum_seconds) {
      const bool over = std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds;
      if (deferred ? over : world.maximum(over) != 0) {
        if (!deferred) return StopMeasurementAtDeadline(context, seed);
        time_stop = true;
        complete = false;
        break;
      }
    }
    size_t batch = std::min({MeasurementBatch, MaximumCavityCells / 6, count - slot});
    DonorPatch imported;
    std::map<Edge, EdgeMeasure> reuse;
    CLocalFailure failure;
    if (!ImportMeasurementBatch(context, ids, slot, batch, plan_scratch, enclosing_seconds, failure, reuse, imported) ||
        !MeasureBatchCells(context, ids, slot, batch, seed, plan_scratch, failure, imported, reuse)) {
      if (deferred && failure.Failed() && !votes.first.Failed()) votes.first = failure;
      complete = false;
      break;
    }
    slot += batch;
    if (seed) ++stats.seed_batches;
  }
  if (deferred) first = votes.first;
  return complete;
}

bool EngineCensus::JoinMeasurement(EngineContext& context, bool seed, bool complete, bool time_stop,
                                   CLocalFailure& first) {
  const bool timed_out = context.options.maximum_seconds && context.world.maximum(time_stop);
  if (!complete && !time_stop && !first.Failed())
    first.Set(2, 0, "Native3D census stopped without a recorded failure.");
  if (!context.exchange.Vote(context, first)) return false;
  return !timed_out || StopMeasurementAtDeadline(context, seed);
}

bool EngineCensus::Measure(EngineContext& context, const std::vector<Id>& requested_ids, bool seed,
                           double* enclosing_seconds) {
  auto& options = context.options;
  auto& world = context.world;
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  std::optional<SU2NativeTiming3D::Scope> census_scope;
  if (options.timings) census_scope.emplace(SU2NativeTiming3D::Operation::Census);
  std::vector<Id> demand;
  const auto demand_bytes = surface_target ? transfer_memory::GrowthBound(requested_ids.size(), sizeof(Id)) : 0;
  const auto plan_scratch = transfer_memory::Add(
      demand_bytes,
      transfer_memory::Mul(3 * MaximumCampaignEdges, sizeof(std::pair<const Edge, EdgeMeasure>) + 6 * sizeof(void*)),
      surface_target ? transfer_memory::Add(SurfacePlanningScratchBytes(MeasurementBatch),
                                            transfer_memory::Mul(MeasurementBatch, sizeof(OwnedCell)))
                     : 0);
  if (!context.exchange.Admit(
          context, transfer_memory::Add(plan_scratch,
                                        transfer_memory::Mul(MeasurementBatch, sizeof(Cell) + sizeof(unsigned char)))))
    return false;
  if (surface_target) {
    CLocalFailure failure;
    try {
      demand.reserve(requested_ids.size());
      for (Id id : requested_ids)
        if (selection.RequestedWall(working.at(id))) demand.push_back(id);
      stats.surface_measurement_skipped += requested_ids.size() - demand.size();
    } catch (const std::exception& e) {
      failure.Set(2, 0, e.what());
    }
    if (!context.exchange.Vote(context, failure)) return false;
  }
  const auto& ids = surface_target ? demand : requested_ids;
  selection.census.census_started = true;
  const size_t count = TimedAllreduce(ids.size(), CPassiveComm::Op::MAX, world.comm);
  const bool deferred = !surface_target && !*context.exchange.private_repair;
  CLocalFailure first;
  bool time_stop = false;
  const bool complete = MeasureChunks(context, ids, count, seed, plan_scratch, enclosing_seconds, deferred, first,
                                      time_stop);
  return deferred ? JoinMeasurement(context, seed, complete, time_stop, first) : complete;
}

bool EngineCensus::SeedNext(EngineContext& context, bool local_active) {
  using Phase = EnginePhase;
  auto& options = context.options;
  auto& world = context.world;
  auto& composition = context.composition;
  auto& surface_target = context.surface_target;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& phase = *context.setup.control_prefix.phase;
  auto& initial = *context.census.initial;
  auto& seed_slot = *context.census.seed_slot;
  auto& working = *context.ownership.working;
  auto& selection = *context.census.selection;
  const auto& started = *context.setup.control_prefix.started;
  Timer timer(stats.seed_seconds, options.timings, SU2NativeTiming3D::Operation::Census);
  // Composed or sided planning is expensive; a complete cavity may remove still-unmeasured topology.
  if ((!composition && !surface_target) || phase == Phase::Improvement) {
    const bool measured = Measure(context, initial, true, &stats.seed_seconds);
    if (measured) seed_slot = initial.size();
    return measured;
  }
  if (options.maximum_seconds &&
      world.maximum(std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds)) {
    stats.stop = EngineStop::TimeBudget;
    result.reason = "Native3D metric seeding exhausts elapsed-time budget.";
    return false;
  }
  if (!context.exchange.Admit(context, transfer_memory::Mul(MeasurementBatch, sizeof(Id)))) return false;
  CLocalFailure failure;
  std::vector<Id> next;
  try {
    size_t scanned = 0;
    next.reserve(MeasurementBatch);
    while (local_active && seed_slot < initial.size() && next.size() < MeasurementBatch &&
           scanned++ < MaximumDirectoryScan) {
      const auto id = initial[seed_slot++];
      if (working.count(id) && !selection.census.cells.count(id)) next.push_back(id);
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  return context.exchange.Vote(context, failure) && Measure(context, next, true, &stats.seed_seconds);
}
