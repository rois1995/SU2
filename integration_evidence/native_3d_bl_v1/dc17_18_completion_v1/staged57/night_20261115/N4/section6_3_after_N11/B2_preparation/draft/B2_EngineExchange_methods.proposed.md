# Unapplied CUT B2 method draft

**Not applied.** This text is a review draft against frozen B1 source `Common/src/adaptation/CNativeEngine3D.cpp` SHA256 `5d95a9c8556084c7faaa10cb04603aea7e10354dc942cd89162d7f27210511a7`. It must not be applied before B1 G1/G2 passes and the required commit/push. No build, test, or solver was run for this draft.

## Method surface

Add these declarations to `EngineExchange.hpp` after forward-declaring `EngineContext`. Keep method bodies in `EngineExchange.cpp` (include `EngineContext.hpp` there after the existing header include). Existing noncopyable slots and construction/destruction stay unchanged.

```cpp
struct EngineContext;

struct EngineExchange {
  // existing slots and RetireDirectory declaration stay in place
  CElectedFailure ElectTrialFailure(EngineContext&, const CLocalFailure&);
  bool Vote(EngineContext&, const CLocalFailure&);
  size_t Bytes(EngineContext&);
  size_t RealBytes(EngineContext&);
  bool Admit(EngineContext&, size_t extra);
  DonorPatch ImportSource(EngineContext&, const std::vector<Cell>& cells, size_t extra, bool layer,
                          const std::vector<unsigned char>* requests = nullptr);
  // existing special members stay in place
};
```

The declarations use existing exact value types and explicit arguments. `requests` remains accepted and deliberately unused. No member, callback, pointer, counter, or storage is added.

## Proposed definitions

```cpp
CElectedFailure EngineExchange::ElectTrialFailure(EngineContext& context, const CLocalFailure& local) {
  const auto& private_repair = *context.exchange.private_repair;
  auto& world = context.world;
  if (private_repair) return CElectedFailure{local.Failed(), world.rank, local.severity, local.gid, local.message};
  return world.elect(local);
}

bool EngineExchange::Vote(EngineContext& context, const CLocalFailure& local) {
  auto& options = context.options;
  auto& world = context.world;
  auto& result = *context.control.result;
  auto& stats = result.stats;
  auto& private_repair = *context.exchange.private_repair;
  auto& atlas_budget = options.shared_geometry_budget ? *options.shared_geometry_budget : *context.setup.local_atlas_budget;
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
    stats.stop = elected.severity == atlas_time_refusal ? EngineStop::TimeBudget :
                 elected.severity == atlas_work_refusal ? EngineStop::WorkBudget : EngineStop::Failure;
  }
  return !elected.any;
}

size_t EngineExchange::Bytes(EngineContext& context) {
  const auto& options = context.options;
  const auto& accepted_bytes = *context.control.accepted_bytes;
  const auto& batch_bytes = *context.control.batch_bytes;
  const auto& diagnostic_bytes = *context.control.diagnostic_bytes;
  const auto& checkpoint = *context.control.checkpoint;
  const auto& result = *context.control.result;
  const auto& working = *context.ownership.working;
  const auto& batch_changed = *context.commit.batch_changed;
  const auto& batch_touched = *context.commit.batch_touched;
  const auto& batch_supports = *context.commit.batch_supports;
  const auto& original = context.original;
  const auto& initial = *context.census.initial;
  const auto& eligible_extensions = *context.census.eligible_extensions;
  const auto& selection = *context.census.selection;
  const auto& retired_edges = *context.census.retired_edges;
  const auto& campaign_edges = *context.census.campaign_edges;
  const auto& field_identity = *context.census.field_identity;
  const auto& inactive_blocked_edges = *context.queues.inactive_blocked_edges;
  const auto& inactive_blocked_links = *context.queues.inactive_blocked_links;
  return transfer_memory::Add(options.caller_bytes, MapBytes(options.wall_heights), MapBytes(options.region_walls),
      accepted_bytes, batch_bytes, diagnostic_bytes, MapBytes(working), checkpoint.Bytes(),
      transfer_memory::Bytes(batch_changed), MapBytes(batch_touched), MapBytes(batch_supports),
      transfer_memory::Bytes(original), transfer_memory::Bytes(initial), transfer_memory::Bytes(eligible_extensions),
      transfer_memory::Bytes(result.splits), transfer_memory::Bytes(result.collapses), selection.Bytes(),
      MapBytes(retired_edges), MapBytes(campaign_edges), MapBytes(inactive_blocked_edges),
      transfer_memory::Mul(inactive_blocked_links, sizeof(Id) + 4 * sizeof(void*)), field_identity ? 64 : 0);
}

size_t EngineExchange::RealBytes(EngineContext& context) {
  const auto live = Bytes(context);
  const bool atlas_geometry = bool(*context.setup.atlas_geometry);
  const auto& atlas_scratch = *context.setup.control_prefix.atlas_scratch;
  return atlas_geometry && live != SIZE_MAX ? live - atlas_scratch : live;
}

bool EngineExchange::Admit(EngineContext& context, size_t extra) {
  auto& options = context.options;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& stats = context.control.result->stats;
  auto& atlas_budget = options.shared_geometry_budget ? *options.shared_geometry_budget : *context.setup.local_atlas_budget;
  const bool atlas_geometry = bool(*context.setup.atlas_geometry);
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

DonorPatch EngineExchange::ImportSource(EngineContext& context, const std::vector<Cell>& cells, size_t extra, bool layer,
                                       const std::vector<unsigned char>* requests) {
  auto& stats = context.control.result->stats;
  auto& source = *context.exchange.source;
  auto& directory = *context.exchange.directory;
  auto& options = context.options;
  if (layer) ++stats.layer_donor_batches;
  else ++stats.donor_batches;
  (void)requests;  // Preserves the existing ignored requests argument.
  return source.ImportCells(cells, options.memory_ceiling,
      transfer_memory::Add(Bytes(context), directory->ResidentBytes(), extra));
}
```

## Adapt and report binding delta

1. In `Adapt` only, remove the six lambda definitions at original source lines 1647-1714 and 2657-2663. Replace their direct calls with `context.exchange.ElectTrialFailure(context, ...)`, `Vote(context, ...)`, `Bytes(context)`, `RealBytes(context)`, `Admit(context, ...)`, and `ImportSource(context, ...)`. The lexical direct-use counts in the frozen file are: elect 2 (only from vote); vote 132; bytes 57; real_bytes 5; admit 40; import_source 2. Retain every existing call site, argument order, conditional, catch, and caller timer. This also removes the current “uncalled `bytes` callback” dependency described below.
2. In `EngineReports.hpp/.cpp`, remove the `Bytes&` template parameter/argument from `InitializeFixedMetricDrift`, `ReadFixedMetricDrift`, and `EmitFixedMetricDrift`; bind `context.exchange.Bytes(context)` at the two current use sites (the two `world.exchange` memory operands in Initialize and Read). Remove the bytes argument from the two nested helper invocations and from `Adapt`’s `EmitFixedMetricDrift(context, stage, bytes)` call. No callback or `std::function` is introduced.

## Binding/reversal manifest and proof obligations

- `vote` reads `*context.exchange.private_repair` anew on every invocation. It resolves `atlas_budget` at the call site with the same selector used in `Adapt`: shared budget if the option pointer exists, otherwise the active `setup.local_atlas_budget`. The current source search has no assignments to `shared_geometry_budget`; do not store this resolved reference or add a pointer slot.
- `Bytes` keeps the original operand order, `MapBytes` / `Bytes` / `Mul` helpers, and the exact original containers. It does not include imports/new member storage. The two report call paths use this method synchronously at their original memory-accounting expressions.
- `ImportSource` keeps the `requests` argument and never forwards it. Batch increments occur before `source.ImportCells`, as before. Layer call 2761 remains within its existing `Import` scope and retains its surrounding `SetCavity` / clear order; measurement call 2960 remains within its existing `Import` and `Census` scopes.
- No collective, timer, catch, reserve/clear/swap, call count, or exception boundary is added or deleted. `Vote` -> `ElectTrialFailure` retains one election normally and the conditional second election for sticky refusals. `Admit` calls `Vote` after peak accounting and updates the atlas byte window only on success.
- Reversal manifest: revert only the six method declarations/definitions, the Adapt-local call substitutions/removal of the six lambdas, and the three `EngineReports` signature/call-site changes. Do not revert unrelated B2-preparation material or any B1 work.
- Before applying, reviewer should check optional-slot liveness at every call site, include dependencies for the method definitions, and verify source-level direct call counts and the uncalled callback elimination. This draft was not compiled, so it makes no compilability claim.
