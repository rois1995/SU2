# B8 setup extraction draft (unapplied)

Status: proposal only. Source is immutable `git show ce252:Common/src/adaptation/CNativeEngine3D.cpp` (SHA256 `806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5`). No production file, build, test, or numerical run was changed or performed.

## Explicit static setup interface

Add these private static members to `EngineSetup` (types already visible through `EngineContext.hpp`). Definitions belong in the existing private include path, after dependencies are complete. They borrow current context and locals; they do not activate `ContextValue` slots.

```cpp
static void CheckInitialNumericPolicies(
    EngineContext&, SU2NativeGeometry::AtlasBudget&, bool curved,
    CLocalFailure&);
static void CheckInitialWallPolicies(EngineContext&, CLocalFailure&);
static void ExchangeInitialWallPolicies(
    EngineContext&, CLocalFailure&, bool& ready);
static void PopulateInitialStateAndBuildIndexes(
    EngineContext&, bool curved, CLocalFailure&, bool& ready,
    Id& maximum_node, Id& maximum_cell);

template <class Finish>
static std::optional<EngineResult> RefreshWorkingSurface(
    EngineContext&, SU2NativeGeometry::AtlasBudget&, Finish&);
template <class Finish>
static std::optional<EngineResult> CheckRegionApplicability(
    EngineContext&, bool& region_applicable, bool prismatic);
template <class Finish>
static std::optional<EngineResult> InitializeFirstRows(
    EngineContext&, Finish&);
static void BeginConstructionPass(EngineContext&);
static bool SeedFrontier(EngineContext&, CLocalFailure&);
```

The two small methods below are concrete token-preserving candidates. They are
proposals only; no production header or implementation was edited.

```cpp
void EngineSetup::BeginConstructionPass(EngineContext& context) {
  if (context.bidirectional)
    context.exchange.directory->BeginConstructionPass();
}

bool EngineSetup::SeedFrontier(EngineContext& context,
                               CLocalFailure& failure) {
  try {
    context.census.selection->SeedFrontier(
        *context.ownership.working);
  } catch (const std::exception& e) {
    failure.Set(2, 0, e.what());
  }
  return context.exchange.Vote(context, failure);
}
```

The caller keeps the original lifetime shape:

```cpp
EngineSetup::BeginConstructionPass(context);
if (!context.exchange.Admit(context, original_admission_bytes))
  return finish();
{
  CLocalFailure failure;
  if (!EngineSetup::SeedFrontier(context, failure)) return finish();
}
```

`original_admission_bytes` above is a placeholder for the original expression,
not a new variable or a recommendation to cache/recompute it. The source token
expression remains `transfer_memory::Mul(transfer_memory::Mul(coarsening ? 12 :
6,working.size()),sizeof(Edge)+4*sizeof(void*))` at the caller. The helper moves
only the `try`/`catch` and vote body; `failure` remains caller-owned and finish
still runs before that scope exits.

`Finish&` is deliberately a reference to the existing local finalizer. This avoids type erasure, allocation, and another callback object. Once the planned finalizer becomes `EngineControl::Finish(context, curved, working, atlas_budget, started, started_cpu)`, rebind the template call to a non-owning synchronous continuation at the call site; do not change helper result semantics.

## Proposed caller skeleton

This is the exact order and lifetime shape. Existing `ContextValue` declarations above this block stay at their original statements. The S13 region transaction and S15 layer body stay inline in this draft because their resources and nested timers must remain in the caller’s lexical scopes; moving them wholesale would change destruction order. No B6/B7 state or round loop is included.

```cpp
{
  Timer timer(stats.initialization_seconds, options.timings,
              SU2NativeTiming3D::Operation::Communication);
  CLocalFailure failure;
  EngineSetup::CheckInitialNumericPolicies(
      context, atlas_budget, curved, failure);
  EngineSetup::CheckInitialWallPolicies(context, failure);
  ready = context.exchange.Vote(context, failure);
  if (ready) {
    EngineSetup::ExchangeInitialWallPolicies(context, failure, ready);
  }
  if (ready) {
    EngineSetup::PopulateInitialStateAndBuildIndexes(
        context, curved, failure, ready, maximum_node, maximum_cell);
  }
}
if (!ready) return finish();

if (auto done = EngineSetup::RefreshWorkingSurface(
        context, atlas_budget, finish)) {
  return std::move(*done);
}
```

`CheckInitialNumericPolicies` contains the original S10 statements from `curved_lo` through the wall-height tolerance validation, ending before the first `ready=context.exchange.Vote`. It changes no predicates, messages, collectives, or statement order. `CheckInitialWallPolicies` is an exact named subrange for the existing `region_walls` consistency and `wall_heights` positivity loops plus the first-height tolerance check; this draft keeps the shared vote in the caller. `ExchangeInitialWallPolicies` contains the original admission/exchange/equality check and second vote. `PopulateInitialStateAndBuildIndexes` contains the final admission, field identity and working-state initialization, IDs, selection rows, directory build, and source build in source order. Its `ready` reference preserves the current guarded execution.

The proposed 4-way S10 split is a source partition, not a changed phase boundary: keep one `Timer`, one `CLocalFailure`, and the caller’s final `if (!ready) return finish()` after timer destruction. Helpers that set `ready=false` must not call finish.

## Result propagation at extracted branches

S11 uses an engaged optional only to carry the already-finalized result through the helper boundary. Each original failure branch calls the same `finish()` at that exact point and returns its result; success returns `std::nullopt`. The caller returns the moved payload immediately and must not finish a second time. S12 now uses a plain `bool`: keep `global_original`, `prismatic`, and the initial `region_applicable` calculation in `Adapt`; the helper mutates only `region_applicable`, sets the existing mismatch reason, and returns false. The caller retains `if (!CheckRegionApplicability(...)) return finish();`. S18’s `CLocalFailure` remains in the caller block so its destruction remains after the existing finish expression. S19 already calls `EngineCensus::SeedNext(context)` and stays unchanged.

S13 and S15 are intentionally not extracted in this draft. Their callback/timer/resource scopes include `RegionFieldService`, `pending`, `FrozenField`, `FieldAccounting`, per-batch vectors, and nested plan/construct/publish operations. A follow-up partition must leave those owners and each existing finish branch at its current caller lexical scope, using synchronous by-reference continuation only. No helper should return a status and allow those scopes to unwind before finish.

## Reverse-proof obligations

Before this is rebound after B7, a generator must compare the original main source and proposed main source after reversing only explicit call substitutions. Required checks:

1. All S10 body statements map one-for-one to the four helper bodies, in original lexical order; timer and failure declarations remain in `Adapt`.
2. Every S10 branch/message, collective, admission byte expression, and `ready` update hashes to the corresponding original token range.
3. Every S11/S12/S18 `return finish()` is accounted for exactly once and no new finish branch exists.
4. S13, S15, S17, S19, all `ContextValue` declarations, the finish lambda, and all other source bytes remain identical.
5. Reversing helper calls back to source ranges reconstructs the entire `CNativeEngine3D.cpp` byte-for-byte.

This outline is superseded for S10–S12 by the generated unapplied proposals and proof receipts alongside it: `CNativeEngine3D_S10.cpp.proposed`, `CNativeEngine3D_S11.cpp.proposed`, and `CNativeEngine3D_S12.cpp.proposed`. S13/S15 and S18 remain pending; this outline is not an applied or compiled change.

## Unverified return move

The existing finalizer returns `std::move(result)` and `EngineResult` contains container-owning members. Source inspection suggests normal allocator-backed moves, but no `is_nothrow_move_constructible<EngineResult>` proof was established and compilation is prohibited for this prep. Keep the current return-move idiom and confirm the type property read-only before any production binding; do not add a copy or alter result representation.
