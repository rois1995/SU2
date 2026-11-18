// Executes ownership transitions using canonical borrowed state and original transaction lifetimes.
#include "EngineOwnership.hpp"
#include "EngineReports.hpp"

EngineOwnership::EngineOwnership() = default;
EngineOwnership::~EngineOwnership() = default;

bool EngineOwnership::WithinTime(EngineContext& context) {
  auto& world = context.world;
  auto& options = context.options;
  const auto& started = *context.setup.control_prefix.started;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;

  if (!options.maximum_seconds) return true;
  if (!world.maximum(std::chrono::duration<double>(Clock::now() - started).count() >= options.maximum_seconds))
    return true;
  stats.stop = EngineStop::TimeBudget;
  result.reason = "Native3D phase ownership exceeds the event deadline; prior ownership retained.";
  return false;
}

bool EngineOwnership::PhaseOwnership(EngineContext& context, bool quality,
                                     SU2NativeGeometry::AtlasBudget& atlas_budget) {
  auto& world = context.world;
  auto& options = context.options;
  auto& working = *context.ownership.working;
  auto& ownership_evaluated = *context.ownership.ownership_evaluated;
  auto& quality_ownership_round = *context.ownership.quality_ownership_round;
  auto& stats = context.control.result->stats;
  auto& checkpoint = *context.control.checkpoint;
  auto& selection = *context.census.selection;
  if (!WithinTime(context)) return false;
  const unsigned boundary = quality ? 1 : 0;
  if (world.size == 1 || options.phase_ownership == SU2Native3D::PhaseOwnership::Disabled) return true;
  if (ownership_evaluated[boundary] && (!quality || stats.rounds < quality_ownership_round ||
                                        stats.rounds - quality_ownership_round < WindowRounds(options, quality)))
    return true;
  if (!quality) {
    ownership_evaluated[boundary] = true;
    ++stats.ownership_evaluations;
  }
  // The checkpoint's ownership is part of its rollback contract. Its enclosing speculative
  // coarsening cycle retains that ownership until it is explicitly retired.
  if (world.maximum(checkpoint.valid || !context.census.ScopeCensusComplete(context) ||
                    (quality && !selection.queues.repair_zero.empty())))
    return true;
  if (quality && world.maximum(!selection.Progress(working.size()).complete)) return true;
  const auto global_cells = TimedAllreduce(uint64_t(working.size()), CPassiveComm::Op::SUM, world.comm);
  // Optional migration cannot populate one graph part per rank with fewer cells than ranks.
  if (global_cells < uint64_t(world.size)) return true;
  if (quality) {
    ownership_evaluated[boundary] = true;
    quality_ownership_round = stats.rounds;
    ++stats.ownership_evaluations;
  }
  const double transition_started = world.seconds();
  return EstimateOwnership(context, quality, atlas_budget, global_cells, transition_started);
}

bool EngineOwnership::EstimateOwnership(EngineContext& context, bool quality,
                                        SU2NativeGeometry::AtlasBudget& atlas_budget, uint64_t global_cells,
                                        double transition_started) {
  auto& world = context.world;
  auto& options = context.options;
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& geometry_authority = *context.control.geometry_authority;
  const auto scratch = transfer_memory::Add(
      MapBytes(working),
      transfer_memory::Mul(working.size(), sizeof(uint32_t) * (quality ? 3 : 2) + sizeof(double) * (quality ? 3 : 2) +
                                               sizeof(Selection::CachedRow) * 3 + sizeof(Id)),
      transfer_memory::Mul(working.size(), 4 * (sizeof(std::pair<const Id, uint64_t>) + 4 * sizeof(void*))),
      transfer_memory::Mul(world.size, 256),
      quality && geometry_authority ? transfer_memory::Mul(geometry_authority->Reference().Facets().size(), 256) : 0);
  if (!context.exchange.Admit(context, scratch)) return false;
  std::map<Id, uint64_t> complete_stars;
  if (!VertexStarSizes(world, working, complete_stars, result.reason, options.memory_ceiling,
                       transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(),
                                            directory->ResidentBytes(), scratch),
                       &stats.maximum_work_bytes))
    return false;
  if (!WithinTime(context)) return false;
  CLocalFailure failure;
  std::vector<uint32_t> weights, geometric, quality_weights;
  std::vector<double> costs, geometry_costs, quality_debts;
  double local_demand = 0, local_geometry = 0, local_quality_debt = 0;
  FillOwnershipCosts(context, quality, complete_stars, failure, weights, geometric, quality_weights, costs,
                     geometry_costs, quality_debts, local_demand, local_geometry, local_quality_debt);
  if (!context.exchange.Vote(context, failure) || !WithinTime(context)) return false;
  NormalizeOwnershipWeights(context, quality, weights, geometric, quality_weights, costs, geometry_costs,
                            quality_debts);
  if (!ShouldMigrate(context, quality, global_cells, transition_started, local_demand, local_geometry,
                     local_quality_debt))
    return true;
  return MigrateOwnership(context, quality, atlas_budget, transition_started, scratch, failure, weights, geometric,
                          quality_weights);
}

void EngineOwnership::FillOwnershipCosts(EngineContext& context, bool quality,
                                         const std::map<Id, uint64_t>& complete_stars, CLocalFailure& failure,
                                         std::vector<uint32_t>& weights, std::vector<uint32_t>& geometric,
                                         std::vector<uint32_t>& quality_weights, std::vector<double>& costs,
                                         std::vector<double>& geometry_costs, std::vector<double>& quality_debts,
                                         double& local_demand, double& local_geometry, double& local_quality_debt) {
  auto& working = *context.ownership.working;
  try {
    costs.reserve(working.size());
    geometry_costs.reserve(working.size());
    weights.reserve(working.size());
    geometric.reserve(working.size());
    if (quality) {
      quality_debts.reserve(working.size());
      quality_weights.reserve(working.size());
    }
    for (const auto& owned : working) {
      AccumulateOwnershipCell(context, quality, owned, complete_stars, costs, geometry_costs, quality_debts,
                              local_demand, local_geometry, local_quality_debt);
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
}

void EngineOwnership::AccumulateOwnershipCell(EngineContext& context, bool quality, const Owned::value_type& owned,
                                              const std::map<Id, uint64_t>& complete_stars, std::vector<double>& costs,
                                              std::vector<double>& geometry_costs, std::vector<double>& quality_debts,
                                              double& local_demand, double& local_geometry,
                                              double& local_quality_debt) {
  auto& surface_target = context.surface_target;
  auto& selection = *context.census.selection;

  if (surface_target && !quality && !selection.RequestedWall(owned.second)) {
    // Untouched volume has no independent wall demand; it still migrates with the complete mesh.
    costs.push_back(1);
    geometry_costs.push_back(0);
    local_demand += 1;
    return;
  }
  const auto& row = selection.census.cells.at(owned.first);
  double demand = 1;
  if (quality) {
    if (!row.shape_measured) throw std::logic_error("Quality ownership requires the permitted complete shape census.");
    const double debt = double(row.debt.q + row.debt.j);
    if (!std::isfinite(debt) || debt < 0) throw std::logic_error("Invalid quality ownership debt.");
    quality_debts.push_back(debt);
    local_quality_debt += debt;
    double star = 0;
    for (Id vertex : row.vertices) star += complete_stars.at(vertex) / 4.;
    demand += (double(row.debt.q + row.debt.j + row.debt.height) + double(row.geometry_invalid)) * (1 + star);
  }
  for (unsigned i = 0; i < 6; ++i) {
    const auto edge = row.edges[i];
    const auto length = selection.census.edges.at(edge).measure.length;
    if (surface_target && !selection.census.edges.at(edge).physical_users) continue;
    const double insertions = double(std::max(0.L, length / MaximumMetricEdge - 1));
    const auto star = complete_stars.at(edge[0]) + complete_stars.at(edge[1]);
    demand += insertions * (1 + std::log2(1. + star));
  }
  const double geometry_cost = OwnershipGeometryCost(context, quality, owned, demand);
  if (!(std::isfinite(demand) && std::isfinite(geometry_cost)))
    throw std::logic_error("Nonfinite phase workload estimate.");
  costs.push_back(demand);
  geometry_costs.push_back(geometry_cost);
  local_demand += demand;
  local_geometry += geometry_cost;
}

double EngineOwnership::OwnershipGeometryCost(EngineContext& context, bool quality, const Owned::value_type& owned,
                                              double demand) {
  auto& options = context.options;
  auto& geometry = context.geometry;
  auto& selection = *context.census.selection;
  auto& geometry_authority = *context.control.geometry_authority;
  double geometry_cost = 0;
  if (quality && geometry_authority && options.surface_tolerance > 0) {
    std::array<long double, 6> lengths;
    // Cached rows use global key order; the predictor requires local vertex-pair order.
    unsigned slot = 0;
    for (unsigned i = 0; i < 4; ++i)
      for (unsigned j = i + 1; j < 4; ++j) {
        Edge edge{owned.second.cell.v[i].id, owned.second.cell.v[j].id};
        std::sort(edge.begin(), edge.end());
        lengths[slot++] = selection.census.edges.at(edge).measure.length;
      }
    geometry_cost = EstimateGeometryWork(owned.second, geometry, options.surface_tolerance, lengths).work * demand;
  } else
    for (unsigned face = 0; face < 4; ++face)
      if (owned.second.physical[face].present) {
        double preparation = 1;
        if (geometry_authority) {
          const auto structure = geometry_authority->StructuralEstimate(owned.second.physical[face].facet);
          const double pieces = std::max(
              double(structure.prepared_pieces),
              std::clamp(
                  options.surface_tolerance > 0 ? 16 * structure.control_deviation / options.surface_tolerance : 1., 1.,
                  1048576.));
          preparation += pieces * (1 + std::log2(pieces));
        }
        geometry_cost += preparation * demand;
      }
  return geometry_cost;
}

void EngineOwnership::NormalizeOwnershipWeights(EngineContext& context, bool quality, std::vector<uint32_t>& weights,
                                                std::vector<uint32_t>& geometric,
                                                std::vector<uint32_t>& quality_weights, std::vector<double>& costs,
                                                std::vector<double>& geometry_costs,
                                                std::vector<double>& quality_debts) {
  auto& world = context.world;
  const auto max_cost = TimedAllreduce(costs.empty() ? 0 : *std::max_element(costs.begin(), costs.end()),
                                       CPassiveComm::Op::MAX, world.comm);
  const auto max_geometry =
      TimedAllreduce(geometry_costs.empty() ? 0 : *std::max_element(geometry_costs.begin(), geometry_costs.end()),
                     CPassiveComm::Op::MAX, world.comm);
  const double max_quality_debt =
      quality
          ? TimedAllreduce(quality_debts.empty() ? 0 : *std::max_element(quality_debts.begin(), quality_debts.end()),
                           CPassiveComm::Op::MAX, world.comm)
          : 0;
  for (size_t i = 0; i < costs.size(); ++i) {
    weights.push_back(uint32_t(std::clamp(std::ceil(costs[i] / std::max(1., max_cost) * MaximumPartitionWeight), 1.,
                                          double(MaximumPartitionWeight))));
    geometric.push_back(geometry_costs[i] > 0
                            ? uint32_t(std::clamp(std::ceil(geometry_costs[i] / max_geometry * MaximumPartitionWeight),
                                                  1., double(MaximumPartitionWeight)))
                            : 0);
    if (quality)
      quality_weights.push_back(
          quality_debts[i] > 0
              ? uint32_t(std::clamp(std::ceil(quality_debts[i] / max_quality_debt * MaximumPartitionWeight), 1.,
                                    double(MaximumPartitionWeight)))
              : 0);
  }
}

bool EngineOwnership::ShouldMigrate(EngineContext& context, bool quality, uint64_t global_cells,
                                    double transition_started, double local_demand, double local_geometry,
                                    double local_quality_debt) {
  auto& world = context.world;
  auto& options = context.options;
  const auto& started = *context.setup.control_prefix.started;
  auto& stats = context.control.result->stats;
  auto& geometry_authority = *context.control.geometry_authority;
  const double demand_sum = TimedAllreduce(local_demand, CPassiveComm::Op::SUM, world.comm);
  const double demand_max = TimedAllreduce(local_demand, CPassiveComm::Op::MAX, world.comm);
  const double geometry_sum = TimedAllreduce(local_geometry, CPassiveComm::Op::SUM, world.comm);
  const double geometry_max = TimedAllreduce(local_geometry, CPassiveComm::Op::MAX, world.comm);
  const double quality_debt_sum = quality ? TimedAllreduce(local_quality_debt, CPassiveComm::Op::SUM, world.comm) : 0;
  const double quality_debt_max = quality ? TimedAllreduce(local_quality_debt, CPassiveComm::Op::MAX, world.comm) : 0;
  const double imbalance = std::max({demand_sum ? world.size * demand_max / demand_sum : 1.,
                                     geometry_sum ? world.size * geometry_max / geometry_sum : 1.,
                                     quality_debt_sum ? world.size * quality_debt_max / quality_debt_sum : 1.});
  const double wall = std::chrono::duration<double>(Clock::now() - started).count();
  // Conservative benefit proxy: remaining demand relative to observed attempted edits. The
  // twofold hurdle acknowledges unknown placement, operator mix and destination cache locality.
  const double attempts = TimedAllreduce(double(stats.attempted), CPassiveComm::Op::SUM, world.comm);
  const double remaining = wall * std::max(0., demand_sum - double(global_cells)) / std::max(1., attempts);
  const double estimate_seconds =
      TimedAllreduce(world.seconds() - transition_started, CPassiveComm::Op::MAX, world.comm);
  const double rebuild =
      TimedAllreduce(stats.initialization_seconds + stats.seed_seconds, CPassiveComm::Op::MAX, world.comm);
  double warming = 0;
  if (geometry_authority) {
    const auto cache = geometry_authority->GeometryStatsSnapshot();
    warming = TimedAllreduce(cache.scope_build_seconds / std::max<size_t>(1, cache.scope_builds) * cache.scope_entries,
                             CPassiveComm::Op::MAX, world.comm);
  }
  const double predicted_cost = estimate_seconds + rebuild + warming + stats.ownership_seconds;
  const double predicted_gain = remaining * (1 - 1 / std::max(1., imbalance));
  stats.ownership_predicted_gain += predicted_gain;
  stats.ownership_predicted_cost += predicted_cost;
  const bool locally_eligible = options.phase_ownership == SU2Native3D::PhaseOwnership::DiagnosticAlways ||
                                (imbalance > 1.25 && predicted_gain > 2 * predicted_cost &&
                                 (!options.maximum_seconds || 2 * predicted_cost < options.maximum_seconds - wall));
  // Local elapsed/cost estimates must not split participation in collective repartition.
  const bool proceed = !world.maximum(!locally_eligible);
  if (options.timings)
    std::cout << "NATIVE3D_PHASE_OWNERSHIP rank=" << world.rank << " quality=" << quality
              << " evaluate=1 migrate=" << proceed << " predicted_imbalance=" << imbalance
              << " predicted_gain_seconds=" << predicted_gain << " predicted_cost_seconds=" << predicted_cost
              << " estimate_seconds=" << estimate_seconds << " rebuild_allowance=" << rebuild
              << " warming_allowance=" << warming << " uncertainty_factor=2" << '\n';
  if (!proceed) {
    stats.ownership_seconds += world.seconds() - transition_started;
    return false;
  }
  return true;
}

bool EngineOwnership::MigrateOwnership(EngineContext& context, bool quality,
                                       SU2NativeGeometry::AtlasBudget& atlas_budget, double transition_started,
                                       size_t scratch, CLocalFailure& failure, std::vector<uint32_t>& weights,
                                       std::vector<uint32_t>& geometric, std::vector<uint32_t>& quality_weights) {
  auto& world = context.world;
  auto& options = context.options;
  auto& surface_target = context.surface_target;
  auto& geometry = context.geometry;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& selection = *context.census.selection;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& coarsening = *context.control.coarsening;
  auto& repairing = *context.control.repairing;
  auto& wall_counters = *context.setup.wall_counters;
  if (options.timings) ReportOwnershipMemory(world, "before");
  Owned pending;
  std::vector<Id> pending_initial;
  std::unique_ptr<Directory> pending_directory;
  Selection pending_selection(coarsening, repairing, geometry, stats, options.timings, options.surface_tolerance,
                              options.wall_heights, bidirectional, bool(surface_target), &wall_counters);
  pending_selection.queues.split_lanes = selection.queues.split_lanes;
  pending_selection.queues.lane = selection.queues.lane;
  pending_selection.size_repairs = selection.size_repairs;
  pending_selection.geometry_repairs = selection.geometry_repairs;
  pending_selection.track_repairs = selection.track_repairs;
  pending_selection.repair_event = selection.repair_event;
  pending_selection.track_splits = selection.track_splits;
  pending_selection.queues.repair_epoch = selection.queues.repair_epoch;
  pending_selection.census.census_started = selection.census.census_started;
  pending_selection.census.progress_consistent = selection.census.progress_consistent;
  try {
    pending = working;
    pending_directory = std::make_unique<Directory>(world, true, geometry.Parameters());
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  PartitionStats partition;
  const auto baseline = transfer_memory::Add(context.exchange.Bytes(context), source.ResidentBytes(),
                                             directory->ResidentBytes(), scratch);
  if (!WithinTime(context)) return false;
  const bool partitioned = Repartition(world, pending, weights, world.size, partition, result.reason,
                                       options.memory_ceiling, baseline, geometric, quality_weights);
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, partition.maximum_native_bytes);
  if (!partitioned || !WithinTime(context)) return false;
  return MigrateCertificates(context, quality, atlas_budget, transition_started, scratch, failure, baseline, pending,
                             pending_initial, pending_directory, pending_selection, partition);
}

bool EngineOwnership::MigrateCertificates(EngineContext& context, bool quality,
                                          SU2NativeGeometry::AtlasBudget& atlas_budget, double transition_started,
                                          size_t scratch, CLocalFailure& failure, size_t baseline, Owned& pending,
                                          std::vector<Id>& pending_initial,
                                          std::unique_ptr<Directory>& pending_directory, Selection& pending_selection,
                                          PartitionStats& partition) {
  auto& world = context.world;
  auto& options = context.options;
  auto& surface_target = context.surface_target;
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  auto& result = *context.control.result;
  auto& selection = *context.census.selection;
  // Stage exact per-cell census certificates with their cells. Mathematical cache identity is
  // retained; no geometry query, target recomposition or CFD projection occurs during migration.
  std::vector<std::vector<Selection::CachedRow>> outgoing;
  try {
    outgoing.resize(world.size);
    size_t slot = 0;
    for (const auto& row : working) {
      const auto destination = partition.destinations.at(slot++);
      const auto found = selection.census.cells.find(row.first);
      if (found == selection.census.cells.end()) {
        if (surface_target && !quality && !selection.RequestedWall(row.second)) continue;
        throw std::logic_error("Phase migration lacks a required cached row.");
      }
      Selection::CachedRow record;
      record.id = row.first;
      record.state = found->second;
      for (unsigned i = 0; i < 6; ++i)
        record.lengths[i] = selection.census.edges.at(record.state.edges[i]).measure.length;
      if (quality) selection.SaveRepairState(record);
      outgoing.at(destination).push_back(std::move(record));
    }
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  bool admitted = false;
  const auto records =
      world.exchange(outgoing, &admitted, options.memory_ceiling, transfer_memory::Add(baseline, MapBytes(pending)));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, world.max_exchange_work_bytes);
  if (!admitted) {
    result.reason = "Native3D phase certificate migration exceeds memory admission.";
    return false;
  }
  const auto pending_bytes = transfer_memory::Add(
      MapBytes(pending), transfer_memory::Bytes(records), transfer_memory::Bytes(outgoing),
      transfer_memory::Mul(pending.size(),
                           transfer_memory::Add(context.control.SelectionBound(context, quality), sizeof(Id))));
  if (!context.exchange.Admit(context, transfer_memory::Add(scratch, pending_bytes))) return false;
  if (!WithinTime(context)) return false;
  const bool built = pending_directory->Build(pending, result.reason, options.memory_ceiling,
                                              transfer_memory::Add(baseline, pending_bytes));
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, pending_directory->Statistics().maximum_work_bytes);
  if (!built) return false;
  return RestoreOwnership(context, quality, atlas_budget, transition_started, failure, pending, pending_initial,
                          pending_directory, pending_selection, partition, outgoing, records, pending_bytes);
}

bool EngineOwnership::RestoreOwnership(EngineContext& context, bool quality,
                                       SU2NativeGeometry::AtlasBudget& atlas_budget, double transition_started,
                                       CLocalFailure& failure, Owned& pending, std::vector<Id>& pending_initial,
                                       std::unique_ptr<Directory>& pending_directory, Selection& pending_selection,
                                       PartitionStats& partition,
                                       std::vector<std::vector<Selection::CachedRow>>& outgoing,
                                       const std::vector<Selection::CachedRow>& records, size_t pending_bytes) {
  auto& world = context.world;
  auto& options = context.options;
  auto& surface_target = context.surface_target;
  auto& working = *context.ownership.working;
  auto& field_identity = *context.census.field_identity;
  try {
    pending_initial.reserve(pending.size());
    for (const auto& row : pending)
      if (!surface_target || pending_selection.RequestedWall(row.second)) pending_initial.push_back(row.first);
    for (const auto& record : records) pending_selection.Restore(pending.at(record.id), record, field_identity);
    if (surface_target && !quality) {
      const auto count = pending_selection.SurfaceCensus(pending);
      if (!pending_selection.census.progress_consistent || !pending_selection.census.census_started ||
          count[0] != count[1])
        throw std::logic_error("Incomplete migrated surface demand census.");
    } else if (records.size() != pending.size())
      throw std::logic_error("Incomplete migrated phase census.");
    if (quality)
      pending_selection.RestoreRepairState(records);
    else
      pending_selection.ChangePhase(false, pending, field_identity);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  if (!context.exchange.Vote(context, failure)) return false;
  if (options.timings)
    ReportOwnershipMemory(world, "staged", pending_bytes, transfer_memory::Bytes(outgoing),
                          transfer_memory::Bytes(records));
  if (!WithinTime(context) || !EngineControl::ObserveProgress(context, working, atlas_budget, "ownership_staged"))
    return false;
  return FinalizeOwnership(context, quality, atlas_budget, transition_started, pending, pending_initial,
                           pending_directory, pending_selection, partition, outgoing, records, pending_bytes);
}

bool EngineOwnership::FinalizeOwnership(EngineContext& context, bool quality,
                                        SU2NativeGeometry::AtlasBudget& atlas_budget, double transition_started,
                                        Owned& pending, std::vector<Id>& pending_initial,
                                        std::unique_ptr<Directory>& pending_directory, Selection& pending_selection,
                                        PartitionStats& partition,
                                        std::vector<std::vector<Selection::CachedRow>>& outgoing,
                                        const std::vector<Selection::CachedRow>& records, size_t pending_bytes) {
  auto& world = context.world;
  auto& options = context.options;
  auto& bidirectional = context.bidirectional;
  auto& working = *context.ownership.working;
  auto& stats = context.control.result->stats;
  auto& selection = *context.census.selection;
  auto& directory = *context.exchange.directory;
  auto& initial = *context.census.initial;
  auto& seed_slot = *context.census.seed_slot;
  auto& inactive_blocked_edges = *context.queues.inactive_blocked_edges;
  auto& inactive_blocked_links = *context.queues.inactive_blocked_links;
  auto& length_blocks_dirty = *context.queues.length_blocks_dirty;
  auto& retired_edges = *context.census.retired_edges;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& construction_claims = *context.exchange.construction_claims;
  // No private transaction survives this swap; required features retain their inherited service history.
  context.exchange.RetireDirectory(directory->Statistics());
  working.swap(pending);
  directory.swap(pending_directory);
  selection.SwapState(pending_selection);
  EngineReports::ResetFixedMetricCells(context);
  inactive_blocked_edges.clear();
  inactive_blocked_links = 0;
  length_blocks_dirty = {{false, false}};
  retired_edges.clear();
  campaign_edges.clear();
  initial.swap(pending_initial);
  seed_slot = initial.size();
  construction_claims = bidirectional && !quality;
  if (construction_claims) directory->BeginConstructionPass();
  ++stats.ownership_changes;
  stats.ownership_moved_cells += partition.moved_cells;
  stats.maximum_owned = std::max(stats.maximum_owned, working.size());
  const auto seconds = world.seconds() - transition_started;
  stats.ownership_seconds += seconds;
  if (options.timings)
    std::cout << "NATIVE3D_PHASE_OWNERSHIP rank=" << world.rank << " quality=" << quality
              << " committed=1 moved_cells=" << partition.moved_cells << " total_seconds=" << seconds
              << " partition_seconds=" << partition.total_seconds
              << " directory_build_seconds=" << directory->Statistics().build_seconds
              << " work_before=" << partition.work_before.at(world.rank)
              << " work_after=" << partition.work_after.at(world.rank)
              << " geometry_before=" << partition.geometry_before.at(world.rank)
              << " geometry_after=" << partition.geometry_after.at(world.rank)
              << " quality_debt_weight_before=" << partition.quality_debt_before.at(world.rank)
              << " quality_debt_weight_after=" << partition.quality_debt_after.at(world.rank)
              << " directory_peak_bytes=" << directory->Statistics().maximum_work_bytes << " cells=" << working.size()
              << " transition_peak_bytes=" << stats.maximum_work_bytes << '\n';
  if (options.timings)
    ReportOwnershipMemory(world, "committed", pending_bytes, transfer_memory::Bytes(outgoing),
                          transfer_memory::Bytes(records));
  return !quality || EngineControl::ObserveProgress(context, working, atlas_budget, "ownership_committed");
}
