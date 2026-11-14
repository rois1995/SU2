# B2/B3 guidance memo admission follow-up

Read-only source review, 2026-11-14. No runtime files changed.

## Existing accounting snapshots

- The main driver calls `policy->Update()` and then snapshots `policy->StorageBytes()` into `control.caller_bytes` at `CSinglezoneDriver.cpp:394-409`; that value is then passed unchanged through `kernel.caller_bytes` into `EventControls` at `:415-423`.
- `CNativeBLPipeline3D::RunEvent` reserves geometry scratch by subtracting `queryScratch` from the ceiling (`:341-359`), then builds `callerBytes` from the supplied control, cells/donors/metadata, and only adds target/reference storage itself when the incoming caller count is zero (`:365-369`). Later `StateBytes` checks use this count (`:404-428`), and the nested Engine snapshot is computed once at `:441` before `PrepareSurface` invokes `BoundaryGuidance` (`:448-449`). A memo created after `:441` without adding its reserved bytes to `callerBytes` makes the engine snapshot stale.
- Target family accounting is partly snapshotted: `CNativeMetricFamily3D::ResidentBytes()` uses `target_extra_bytes`, initialized from `TargetPolicy::StorageBytes()` at `CSolver.cpp:4070`. The direct saved-stage fixture accounts for `family->ResidentBytes()` and captured mesh/parameters before building `options.caller_bytes` (`native_actual_bl_cavity_planning_body.inc:8-21`, `native_actual_saved_surface_stage.inc:24-45`). Mutating persistent `TargetPolicy` storage after these snapshots can undercount it; recalculating `family->target_extra_bytes` at each caller is fragile and risks double counting.

## One bounded lifecycle

Keep both guidance caches in an event-scoped `BoundaryGuidanceMemo`, outside the persistent `TargetPolicy` and metric family. Give it one fixed reserved-byte limit covering strip-rule and full-guidance entries, with `ResidentBytes()` reporting the whole reservation. Cache insertions are optional and bounded by that reservation; a missing entry or full cache runs the existing uncached path. This makes tight-memory behavior a performance fallback, not a new refusal.

1. In `RunEvent`, after the base state is populated and its baseline `StateBytes` is known (`CNativeBLPipeline3D.hpp:401-405`), choose `cap=min(fixed_cache_cap, stateCeiling-baselineBytes)`; if below the minimum useful entry, use zero. Construct/reserve the memo, add its cap once to local `callerBytes`, and repeat the `StateBytes<=stateCeiling` check. The existing `memoryOk` lambda and nested `preparation.caller_bytes` at `:421-444` then account for it. Pass the event memo to the `BoundaryGuidance` callback at `:448-449`. Its lifetime ends with `RunEvent`, including early returns, so `TargetPolicy::StorageBytes()` and `family->target_extra_bytes` remain unchanged.
2. In the direct saved-stage fixture, make the same optional reservation after its complete base `options.caller_bytes` snapshot at `native_actual_saved_surface_stage.inc:24-45`; add the cap once, recheck against `ceiling`, and pass the memo to the guidance callback at `:52-54`. When no headroom remains, run the existing exact guidance path without a memo.
3. Keep the main driver’s `policy->StorageBytes()` addition unchanged. The event memo is separately included only in the pipeline’s local caller count; never also add it to `policy.StorageBytes()`, `target_extra_bytes`, or `family->ResidentBytes()`.

The event memo has two subcaches under that single cap. Strip membership is keyed by exact point plus Option-A strip generation; its value is the matched per-strip rule list. Full guidance additionally keys the validated node ID/coordinates and current binding, exact six sensor components, marker names/roles, authority identity/version, and the full/base live profile stamp (`GetSampleVersion`, `GetGeometrySampleVersion`, `GetSupportVersion`, tangent/feature and outer-transition state, plus core). The full cache stores the complete ordered guidance sample vector. Validate context authority and current node association before a full-cache lookup; omit hits for contexts without a stable parameter association.

## Invalidation without scanning strips on hits

`TargetPolicy::Update` changes base tangential scale, may rebuild Option A strips/grid via `ConfigureRadius` (`CNativeBLTarget3D.hpp:324-341`, `OptionA::ConfigureRadius:98-112`), changes both metric outer profiles/core, and writes `A->core`. Those production mutation paths should advance an Option-A strip generation when strips rebuild and a target-guidance generation on every policy update; the cache also compares existing full/base version getters, so direct profile mutations miss safely. `A->core` is part of the target generation.

The current production code does not edit `A->strips` or `A->creases` except through `ConfigureRadius`. The test fixture `native_boundary_guidance_reference.inc:85-90` directly replaces `A->creases` and assigns `A->strips`; update that fixture to invalidate/recreate its explicitly supplied memo after each direct mutation. Do not checksum/compare all strips or creases on every hit; that would replace `InStrip` work with another O(number of strips) scan. A cache-free default preserves existing callers and mutable fixtures.

## B6 persistence correction

The prior B6 note should require an exact saved-working-mesh and parameter-payload hash before restoring rows: `Selection::Restore` creates edge endpoints from the *new* `OwnedCell`, then `Reusable` only checks those restored endpoints against other restored rows (`CNativeEngine3D.cpp:975-994`). `CachedRow` stores old lengths and topology/version but not the old endpoint coordinates, so this check alone cannot detect a coordinate change that retained cell IDs/versions. Hash sorted cell IDs/versions, vertex IDs and ordered coordinates, physical facet support, and the exact parameter payload before `Restore`.

Also retain valid partial censuses. A stalled stage may have known measured rows and unknown/unmeasured rows; serialize those row-presence/measurement flags with `census_started` and `progress_consistent`, restore only rows present, and require the resulting public census to remain incomplete when demanded rows are absent. Do not make complete census a prerequisite for saving a resumable partial result. The stable checkpoint boundary still must exclude an in-flight transaction.

## Minimal checks

- With zero and near-zero headroom, compare memo-disabled guidance byte-for-byte to the existing path and verify the stage either proceeds under its old budget or reports the existing memory gate; no uncounted cache allocation.
- Under admission, repeated calls hit strip/full entries and match uncached ordered samples. Change point, binding, sensor, full/base profile, core, and radius; each relevant stamp invalidates. Rebuild strips and directly mutate them in the fixture with explicit invalidation.
- Verify the saved-stage fixture’s `StateBytes` and family `ResidentBytes` do not change when the event memo is active; the memo reservation appears exactly once in the pipeline/Engine caller-byte snapshots and is released at event exit.
- For B6 sidecars, reject a changed coordinate with unchanged cell ID/version, and round-trip a partial census while preserving unknown rows and `complete=false`.
