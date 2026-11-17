// Owns only empty optional slots; local Adapt scopes govern payload lifetimes.
#include "EngineExchange.hpp"
#include "EngineContext.hpp"

EngineExchange::EngineExchange() = default;
EngineExchange::~EngineExchange() = default;

void EngineExchange::RetireDirectory(const DirectoryStats& old) {
  auto& retired_directory = *this->retired_directory;

  retired_directory.collision_seconds += old.collision_seconds;
  retired_directory.collision_builds += old.collision_builds;
  retired_directory.collision_candidates += old.collision_candidates;
  retired_directory.collision_checks += old.collision_checks;
  retired_directory.elections += old.elections;
  retired_directory.election_seconds += old.election_seconds;
  retired_directory.parameter_agreement_seconds += old.parameter_agreement_seconds;
  retired_directory.parameter_publication_seconds += old.parameter_publication_seconds;
  retired_directory.parameter_edits += old.parameter_edits;
  retired_directory.parameter_faces += old.parameter_faces;
  retired_directory.parameter_payload_bytes += old.parameter_payload_bytes;
  retired_directory.change_scanned += old.change_scanned;
  retired_directory.change_sent += old.change_sent;
  retired_directory.change_received += old.change_received;
  retired_directory.change_seconds += old.change_seconds;
  retired_directory.maximum_work_bytes = std::max(retired_directory.maximum_work_bytes, old.maximum_work_bytes);
}

CElectedFailure EngineExchange::ElectTrialFailure(EngineContext& context, const CLocalFailure& local) {
  auto& private_repair = *context.exchange.private_repair;
  auto& world = context.world;
  if (private_repair) return CElectedFailure{local.Failed(), world.rank, local.severity, local.gid, local.message};
  return world.elect(local);
}

bool EngineExchange::Vote(EngineContext& context, const CLocalFailure& local) {
  auto& options = context.options;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& private_repair = *context.exchange.private_repair;
  auto& atlas_budget =
      options.shared_geometry_budget ? *options.shared_geometry_budget : *context.setup.local_atlas_budget;
  const auto& atlas_work_initial = *context.setup.control_prefix.atlas_work_initial;
  if (private_repair && atlas_budget.within_time && !atlas_budget.within_time()) atlas_budget.time_exhausted = true;
  stats.atlas_work = atlas_budget.work - atlas_work_initial;
  stats.atlas_peak_bytes = atlas_budget.peak_bytes;
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, atlas_budget.peak_bytes);
  // Elect sticky geometry failures at the existing stage votes, even if a trial caught the exception.
  constexpr unsigned short atlas_work_refusal = 4;
  auto pending = local;
  if (atlas_budget.work_exhausted)
    pending.Set(atlas_work_refusal, 0, "Native3D shared atlas work budget exhausted; feasibility is undetermined.");
  constexpr unsigned short geometry_backend_failure = 5;
  if (!atlas_budget.geometry_failure.empty())
    pending.Set(geometry_backend_failure, 0, "Native3D geometry backend failure: " + atlas_budget.geometry_failure);
  constexpr unsigned short atlas_time_refusal = 6;
  if (atlas_budget.time_exhausted)
    pending.Set(atlas_time_refusal, 0,
                "Native3D geometry/private work exhausts elapsed-time budget; feasibility is undetermined.");
  const auto elected = ElectTrialFailure(context, pending);
  if (elected.any) result.reason = elected.message;
  if (elected.any && (elected.severity == atlas_work_refusal || elected.severity == geometry_backend_failure ||
                      elected.severity == atlas_time_refusal)) {
    // A quota refusal need not be the only failure; retain the ordinary elected diagnostic too.
    const auto original = ElectTrialFailure(context, local);
    if (original.any) result.reason += " Original failure: " + original.message;
    result.valid = result.surface_prepared = false;
    stats.stop = elected.severity == atlas_time_refusal   ? EngineStop::TimeBudget
                 : elected.severity == atlas_work_refusal ? EngineStop::WorkBudget
                                                          : EngineStop::Failure;
  }
  return !elected.any;
}

size_t EngineExchange::Bytes(EngineContext& context) {
  auto& options = context.options;
  auto& accepted_bytes = *context.control.accepted_bytes;
  auto& batch_bytes = *context.control.batch_bytes;
  const auto& diagnostic_bytes = *context.control.diagnostic_bytes;
  auto& checkpoint = *context.control.checkpoint;
  auto& result = *context.control.result;
  auto& working = *context.ownership.working;
  auto& batch_changed = *context.commit.batch_changed;
  auto& batch_touched = *context.commit.batch_touched;
  auto& batch_supports = *context.commit.batch_supports;
  const auto& original = context.original;
  auto& initial = *context.census.initial;
  auto& eligible_extensions = *context.census.eligible_extensions;
  auto& selection = *context.census.selection;
  auto& retired_edges = *context.census.retired_edges;
  auto& campaign_edges = *context.census.campaign_edges;
  auto& field_identity = *context.census.field_identity;
  auto& inactive_blocked_edges = *context.queues.inactive_blocked_edges;
  auto& inactive_blocked_links = *context.queues.inactive_blocked_links;
  return transfer_memory::Add(
      options.caller_bytes, MapBytes(options.wall_heights), MapBytes(options.region_walls), accepted_bytes, batch_bytes,
      diagnostic_bytes, MapBytes(working), checkpoint.Bytes(), transfer_memory::Bytes(batch_changed),
      MapBytes(batch_touched), MapBytes(batch_supports), transfer_memory::Bytes(original),
      transfer_memory::Bytes(initial), transfer_memory::Bytes(eligible_extensions),
      transfer_memory::Bytes(result.splits), transfer_memory::Bytes(result.collapses), selection.Bytes(),
      MapBytes(retired_edges), MapBytes(campaign_edges), MapBytes(inactive_blocked_edges),
      transfer_memory::Mul(inactive_blocked_links, sizeof(Id) + 4 * sizeof(void*)), field_identity ? 64 : 0);
}

size_t EngineExchange::RealBytes(EngineContext& context) {
  const auto live = Bytes(context);
  auto& atlas_geometry = *context.setup.atlas_geometry;
  const auto& atlas_scratch = *context.setup.control_prefix.atlas_scratch;
  return atlas_geometry && live != SIZE_MAX ? live - atlas_scratch : live;
}

bool EngineExchange::Admit(EngineContext& context, size_t extra) {
  auto& options = context.options;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& stats = context.control.result->stats;
  auto& atlas_budget =
      options.shared_geometry_budget ? *options.shared_geometry_budget : *context.setup.local_atlas_budget;
  auto& atlas_geometry = *context.setup.atlas_geometry;
  const auto& atlas_scratch = *context.setup.control_prefix.atlas_scratch;
  const auto peak = transfer_memory::Add(Bytes(context), source.ResidentBytes(), directory->ResidentBytes(), extra);
  stats.maximum_work_bytes = std::max(stats.maximum_work_bytes, peak);
  CLocalFailure failure;
  if (peak == SIZE_MAX || peak > options.memory_ceiling)
    failure.Set(3, 0, "Native 3D adaptation working storage exceeds budget.");
  const bool admitted = Vote(context, failure);
  // Integer workspace stays fixed; geometric query arrays use the remaining configured memory pool.
  if (admitted && atlas_geometry) {
    atlas_budget.caller_bytes = peak - atlas_scratch;
    atlas_budget.maximum_bytes = options.memory_ceiling;
  }
  return admitted;
}

DonorPatch EngineExchange::ImportSource(EngineContext& context, const std::vector<Cell>& cells, size_t extra,
                                        bool layer, const std::vector<unsigned char>* requests) {
  auto& stats = context.control.result->stats;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& options = context.options;
  if (layer)
    ++stats.layer_donor_batches;
  else
    ++stats.donor_batches;
  return source.ImportCells(cells, options.memory_ceiling,
                            transfer_memory::Add(Bytes(context), directory->ResidentBytes(), extra));
}
