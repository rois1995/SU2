# Scoped reverse proof — EngineReports proposal (unapplied)

This proof ties each removed Adapt lambda and caller to the proposed private static member. It is source accounting, not a compiled equivalence claim. The complete CNativeEngine3D.cpp source remains byte-identical to the build545 pinned file.

## Baseline identity

- Active source: `Common/src/adaptation/CNativeEngine3D.cpp`
- SHA256: `dd97c5e2cb75789b1c35934a6fc3851ba297303a25510df5deceaba65efaf66f`
- It matches the current binding inventory's source hash.
- Per-body source SHA256 and exact old line bounds are in `binding_manifest.json`.

## Reverse bindings

1. `record_rejection` source lines 1851–1863 -> `EngineReports::RecordRejection`. Direct callers at 4043, 6222, 6554 are replaced in the unapplied CPP patch, each passing the still-local `size_t(diagnostic_rejection_limit)`. This limit remains the reserve and indexing limit; no replacement constant exists. Row sequence, round, feature, optional old/metric copies, UTF-8 boundary handling, and truncation are retained in the same order.
2. `consider_fixed_metric_row` lines 1979–1994 -> `EngineReports::ConsiderFixedMetricRow`, called from the moved Update and Reset bodies only. The identity/index comparison, coordinate bounds, metric sample, and swallowed `std::exception` path remain.
3. `update_fixed_metric_cells` lines 1995–1999 -> `EngineReports::UpdateFixedMetricCells`; direct caller sites 6301 and 6593. The removed-cell pass precedes the added-cell loop, which invokes Consider.
4. `reset_fixed_metric_cells` lines 2000–2003 -> `EngineReports::ResetFixedMetricCells`; direct caller 3450. The availability return, `fill(UINT64_MAX)`, and ordered `working` traversal remain.
5. `emit_fixed_metric_drift` lines 2004–2078 -> `EngineReports::EmitFixedMetricDrift`; caller 2214. Its statement ordering is partitioned into original blocks: started/available/initialization test; initialization body; selected/have declaration; read test/body; scalar initialization; comparison test/body; diagnostic time update; output. No local vector, `CLocalFailure`, exchange result, catch scope, or selected/have array crosses its former block.

## Canonical storage identity

Every state alias dereferences the same active optional in `EngineContext`; the complete alias table is in the manifest. `bytes` remains the existing Adapt closure object and enters only by non-owning reference. The namespace-private header receives the one current `FixedMetricSamples=16` definition after the source-location metadata types exist; the old Adapt declaration is removed so there is only one definition. `diagnostic_rejection_limit=64` stays in Adapt and is passed by value at all three recording calls.

## Collective, timer, and output order

The initialization member retains its local-build `try/catch`, then the original election, admission exchange, sort, and assignments. The read member retains its own local-build `try/catch`, election, exchange, and `(cell,owner)` winner order. The comparison body preserves the six tensor components, Frobenius weights, `hypot` call order, finite filters, and owner count. The top Emit member starts the clock before initialization, updates diagnostic seconds after comparison, and emits the same fields in the same sequence after the update. No election, exchange, bytes callback, timer, output, or exception boundary is added or reordered by this proposal.

## Static source-parser assertions

`source_parser_assertions.json` records checked markers, exact direct call counts, one original sample-count definition, exact array signatures, no `.at()` fixed-cell access in the proposal, and an unchanged production source hash. No compile, test, production edit, or numerical run was performed.
