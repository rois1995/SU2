# B8 setup partition and continuation audit

Read-only preparation for B8. No C++ source, build, test, or numerical workflow was changed or run.

## Snapshot and scope

- Controlling source check: `P/source_check.md`, SHA256 `3fff8559d3f67b42c1314f2388fb8ba9d510fa064e625f53ef99df0d7f17f8a1`.
- Current candidate source: `Common/src/adaptation/CNativeEngine3D.cpp`, SHA256 `806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5`. This exactly matches the source pin recorded in B3/build553. `EngineSetup.hpp` is SHA256 `46bbb95747d1f39dd5dc4faa0553577b3b686fd0cebd198431c6b00b934d2d1b`; `EngineContext.hpp` is `74d6202b1462cd3ce3670bf89009b08d8e55dfd05f46cd4010e2fd5ae9e5ee9d`; `ContextValue.hpp` is `8080f325279156926b04ea05ac907d0e1904bfa92936dbe55bddaaf2fff47b55`.
- The earlier B8 inventory names `CNativeEngine3D.cpp` hash `7545edb4…` and older source offsets. Treat those coordinates and lambda names as historical. The current candidate has already moved `scope_census_complete`, `refresh_surface_census`, `generation_census`, `generation_debt`, `measure`, and `seed_next` into `EngineCensus`; the B8 code calls `context.census.*`. This is an accepted snapshot delta, not a request to recreate or move those methods again.
- B8 remains the S10–S19 setup/control boundary only. Do not pull the round loop or B6/B7 state into this item.

## Proposed private method boundary

These declarations are implementation targets, not applied code. They are static methods on the existing `EngineSetup`, declared privately and defined in its included translation-unit implementation. Every method borrows the existing `EngineContext` and already-active values; none constructs a `ContextValue`, owns a second context, copies `EngineOptions`, or adds a persistent slot.

```cpp
static void CheckInitialNumericPolicies(EngineContext&, SU2NativeGeometry::AtlasBudget&, bool curved, CLocalFailure&);
static void CheckInitialWallPolicies(EngineContext&, CLocalFailure&);
static void ExchangeInitialWallPolicies(EngineContext&, CLocalFailure&, bool& ready);
static void PopulateInitialStateAndBuildIndexes(EngineContext&, bool curved, CLocalFailure&, bool& ready);

template <class Finish>
static std::optional<EngineResult> RefreshWorkingSurface(EngineContext&, SU2NativeGeometry::AtlasBudget&, Finish&&);

template <class Finish>
static std::optional<EngineResult> CheckRegionApplicability(
    EngineContext&, bool curved, uint64_t& global_original, bool& prismatic,
    bool& region_applicable, Finish&&);

template <class Finish>
static std::optional<EngineResult> ConstructRegion(
    EngineContext&, SU2NativeGeometry::AtlasBudget&, uint64_t global_original, bool region_applicable,
    Finish&&);

template <class Finish>
static std::optional<EngineResult> InitializeFirstRows(EngineContext&, Finish&&);

static void BeginConstructionPass(EngineContext&);
static bool SeedFrontier(EngineContext&, CLocalFailure&);
```

The `SU2NativeGeometry::AtlasBudget&` is the existing reference chosen once in `Adapt`; it is passed by reference, never copied. `Finish` is the existing `finish` closure, passed as a template parameter so no `std::function` or heap-backed callback is introduced.

For the S10 group, leave its existing `Timer` and `CLocalFailure` declarations in `Adapt`, with all four methods called in their current order inside that timer. The first two methods only add failures to the one shared failure object; the caller executes the existing vote. The wall exchange method performs the existing guarded exchange and vote. The initialization method performs the existing admission, allocation/copy, vote, empty-input check, ID maxima, and `directory->Build` / `source.Build` in order. The caller retains the final `if (!ready) return finish();` after the timer scope. A normal `ready == false` return from the timer block must not call `finish` early.

For S11, S12, S13, and S15, the optional result is a scoped continuation payload: on each existing direct-finish failure branch, the method evaluates `finish()` and explicitly wraps that returned `EngineResult` in an engaged optional before returning. The caller immediately returns the moved payload (`if (auto done = ... ) return std::move(*done);`). On success the method returns `std::nullopt`. This preserves exactly one finalization while helper-local temporary objects and timer/service owners are still alive. Do not have the caller invoke `finish()` again after an engaged result.

The S13 and S15 orchestrators need smaller private submethods to stay under the 80-line rule. Keep their resource-owning block locals at the original caller sequence points and pass references into the submethods. In particular, retain S13 `buckets`, `rows`, `regions`, `old`, `fresh`, `replacement`, `region_parameter_edits`, `RegionReport`, and `RegionFieldService` through the same end of the regional block. Retain S15 `pending`, `epoch`, per-batch vectors/records, donor ownership, `FrozenField`/`FieldAccounting`, and timer scopes at their original iteration boundaries. Do not put `RegionFieldService` or those batches in a new state aggregate: an earlier destructor or a new object layout changes the storage/lifetime contract.

The nested S15 `plan`, `construct`, and `publish` operations keep their current `bool` result. Their current caller executes `finish()` after the nested operation has returned, which is also where its local `Timer`/per-candidate scopes have already ended. Preserve this status boundary rather than invoking the continuation from inside those nested operations.

## Current live bindings and free-identifier map

Every identifier below is either accessed through the single `EngineContext`/`EngineSetup` or is an existing local explicitly passed by reference. Do not synthesize replacement values or reevaluate setup predicates merely to avoid a parameter.

| Block | Current anchors | Canonical context bindings used | Existing locals/scratch that must remain explicit |
|---|---|---|---|
| S10 initialization | 2169–2302; timer starts 2176 | `world`, `accepted`, `original`, `geometry`, `composition`, `options`, `intent`, `bidirectional`, `surface_target`; `control.ready/result`; `commit.maximum_node/maximum_cell`; `ownership.working`; `census.selection/field_identity/initial/eligible_extensions`; `exchange.directory/source`; `setup.atlas_geometry/target_geometry_lease` | The already-computed `curved`, the chosen `atlas_budget` reference, the active `ready` alias, `maximum_node/cell` aliases, and one `CLocalFailure`. `global_cells`, height rows, and exchanged rows remain local to their original helper sub-blocks. |
| S11 surface refresh | 2303–2343 | `geometry`, `options`, `world`, `exchange` (`Admit`, `Vote`, `directory`, byte accounting), `ownership.working`, `census.selection`, `exchange.source`, `control.result`, `setup.atlas_geometry` | The same atlas budget reference; `refreshed`, `failure`, `before_work`, `affected`, `removed`, `added`, `face_storage`, `local_storage`, `count`, and the `owns` predicate. Keep the rank-zero coverage argument and version updates in place. |
| S12 applicability | 2344–2373 | `world`, `original`, `surface_target`, `options.region_walls/wall_heights`, `geometry`, `control.coarsening/repairing`, `control.result` | `curved` (precomputed once), `global_original`, `prismatic`, `region_applicable`, `walls`, and the shared `finish` continuation. Keep the two `TimedAllreduce` operations and mismatch return in order. |
| S13 region transaction | 2374–2511 | `world`, `original`, `geometry`, `composition`, `options`, `started`, `control.result/stats/region_active/region_started`, `control.maximum work`, `ownership.working`, `census.selection/initial`, `exchange.source/directory`, `control.maximum_node/maximum_cell`, `setup.atlas_geometry`/budget, and `wall_counters` | `global_original`, `scratch`, `buckets`, `admitted`, `rows`, `regions`, `old/fresh`, `replacement`, `region_parameter_edits`, `core_declined`, `imported`, `report`, `service`, `field_stats`, `failure`, `changed`, and callback state. These objects must stay alive to their current branch/block endpoints. |
| S15 first-row initialization | 2512–2758 | `world`, `geometry`, `options`, `composition`, `started`, `control.result/stats/layer_active/layer_started/maximum_node/maximum_cell`, `ownership.working`, `census.selection/initial/field_identity`, `exchange.directory/source`, `exchange`, `control.maximum work`, `surface_target`, `wall_counters` | `failure`, `pending`, `layerScratch`, `epoch`, `needs_layer`, `timed_out`, per-batch `features/queued/dependencies/imported/reserved`, proposal/parameter/coverage records, and current callback objects. Keep the original `Timer`, `CpuTimer`, `FrozenField`, and `FieldAccounting` scopes. |
| S16 measurement batch | 2759 | No new B8 method: `MeasurementBatch` remains the existing local `constexpr size_t{16}` and subsequent calls use `EngineCensus`. | Keep its current declaration point/value. |
| S17 construction-pass start | 3125 | `bidirectional`, `exchange.directory` | One guard only; keep this before seed admission. |
| S18 frontier setup | 3126–3131 | `world`, `control.coarsening`, `ownership.working`, `census.selection`, `exchange` | Keep the `CLocalFailure` object in its caller block; pass it by reference to `SeedFrontier`. Its destructor must occur after the existing `finish()` expression on vote failure. |
| S19 initial seeding | 3132–3136 | `surface_target`, `world`, `census.SeedNext`, `census.selection`, `census.seed_slot`, `census.initial` | No new `EngineSetup` method: this candidate already calls `EngineCensus::SeedNext(context)`. Keep the exact `surface_target && !world.maximum(...) && world.maximum(...)` short-circuit. `SeedNext`'s timer ends before caller `finish()` on false. |

## ContextValue activation and destruction audit

`ContextValue<T>` stores a reference to an `std::optional<T>`. Its constructor emplaces at that exact statement; `Get()` aliases the engaged value; its destructor resets the slot. Copy/move are deleted. All B8 methods must borrow existing objects and must not create or destroy any of these activations.

1. **Pre-context owners, 1512–1597:** `EngineSetup setup`, validity-only shape admission, `phase`, timer session, D9 profile, wall/lane counters, `region_rebuilt`, `improvement_changed`, `construction_changed`, handoff/generation/cleanup flags, `phase_start_round`, `started`, `started_cpu`, `private_parameters`, `private_geometry`, local/shared atlas budget, `atlas_work_initial`, `restore_budget`, `atlas_geometry`, target lease, scratch, parameter failure and parameter bound. Mapped-mode reductions and the `mapped_high` elected rejection at 1594–1597 occur before `EngineContext` and before `finish` exists. Keep the direct rejected return there; it is not a B8 continuation.
2. **Context creation, 1598–1603:** resolve the single `geometry`, construct one non-movable `EngineContext` borrowing the mutated by-value `composition`/`options` and canonical inputs, then optional requirements output. Do not move context construction earlier.
3. **ContextValue aliases, 1604–1753:** parameter setup, coarsening/repair state, tracked counters, geometry authority/snapshot, result/stats, working, selection, construction claims, census/queue slots, directory/source/checkpoint, accepted bytes, initial IDs/extensions/seed slot, fixed-metric slots, transaction/lane/region observation and quality census slots. Keep every declaration and constructor argument at its current position; `directory`, `source`, and `selection` retain references to their canonical objects.
4. **Finish closure and timing activations, 2013–2164:** transaction-captured, `layer_active`, `region_active`, layer/region start times are active before `finish` is defined at 2023. `finish` closes those windows, performs final proof/votes/census/reporting, then swaps result cells. Each B8 callback borrows this exact closure; no second finish closure or duplicate finalizer.
5. **S10 output slots, 2169–2174:** `ready`, `maximum_node`, and `maximum_cell` are activated immediately before the initialization timer. S10 helpers may mutate only those active slots and the canonical data reached through context.
6. **Unwinding order:** all `ContextValue` owners unwind in reverse declaration order after the returned `EngineResult` has been produced. Do not hold new aliases beyond Adapt, clear any slot in a helper, or move a setup failure across the pre-context boundary.

## Exact return/finalization map

The branch line numbers below are for the SHA-pinned current candidate, not the older B8 inventory.

| Block | Current direct finish sites | Required propagation |
|---|---|---|
| S10 | 2302, after the 2176–2301 timer block | Keep as caller `if (!ready) return finish();`; timer and block locals have ended first. Helpers return/mutate `ready`, not an early finished result. |
| S11 | 2305, 2312, 2317, 2325, 2327, 2335, 2339 | The refresh continuation must run `finish()` inside the helper at these failure branches, then return its result payload. |
| S12 | 2373 | Applicability disagreement sets the same reason and executes `finish()` before its helper returns. |
| S13 | 2388, 2392, 2394, 2402, 2405, 2480, 2486, 2488, 2490, 2491, 2505 | Preserve each branch and any intervening failure vote / global reduction. A false status from an inner helper is propagated to the S13 owner; the continuation is evaluated before S13 locals are destroyed. |
| S15 | 2518, 2528, 2531, 2538, 2579, 2582, 2702, 2747, 2753 | Outer-layer failure evaluates `finish()` while the layer block state is live. For `plan`, `construct`, and `publish`, their current internal `Timer` scopes end before the caller tests false and invokes the single finish continuation. |
| S17 | no finish site | Keep the conditional `BeginConstructionPass()` before seed admission. |
| S18 | 3126, 3130 | Keep the admission return. Keep `CLocalFailure` in the caller block so `finish()` executes before that block unwinds; helper returns `bool`. |
| S19 | 3135 | Existing `EngineCensus::SeedNext` returns status; caller performs the same one `return finish()` after its internal timer has ended. Preserve the loop short-circuit order at 3136. |

`finish()` finalization itself remains at 2023 and unchanged. The optional continuation payload is only a way to transport its already-produced result back through the new method boundary; it does not introduce another acceptance decision, result state, collective, timer, or result gate.

## Snapshot corrections and implementation handoff

1. The old B8 inventory's line references and `seed_next` local-lambda description are stale for the candidate; current source uses `EngineCensus::SeedNext` at 3134 and has current block ranges as above. Reuse the candidate and current call sites, not the old line offsets.
2. Existing `EngineSetup` member order is unchanged and must remain so: `RestoreAdmission`, control prefix, session/profile/counters, parameter/geometry staging, budget restore, atlas geometry, and target lease. The earlier setup path at 1512–1597 is not part of the B8 block move.
3. Keep all `return finish()` ordering and collective short-circuits literal. The wrapper statuses describe only whether this setup path continues; they cannot mask failure, convert a pre-context rejection, reorder votes, or advance initialization.
4. Before implementation, preserve this map in the combined B8 review and measure actual clang-formatted function lengths. This document is a source binding proposal only, not a gate or authorization to edit production code.
