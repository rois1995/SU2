# B4 census reuse: callback review (source only)

## Current contract and source of exact q/J

- `Common/include/adaptation/CNativeOptimize3D.hpp:36-40` documents `OptimizeOptions::mesh_quality` as an exact measure for a *current* cell by identity, unknown returns false, and fresh candidates are always measured.
- `Common/src/adaptation/CNativeEngine3D.cpp:6195-6198` is the sole consumer: `RowQuality(id,c)` calls the callback and falls back to `Quality(c)` when absent/false. The row-quality path feeds census and bad-cell scans (`7129-7144`), candidate ordering (`7038-7042`, `7143-7152`), and current-vs-fresh admission comparisons (`6221-6237`). Fresh cells go through `FreshScore` → `Quality` (`6219-6220`), so they do not use the callback.
- The existing authoritative census state already retains raw values: `Selection::CellState` has `q_value`, `j_value`, `shape_measured`, and `version` (`CNativeEngine3D.cpp:300-307`); `Add` stores metric q/J and measured flags (`776-798`), and `RefreshQuality` replaces them on a version-matched update (`727-749`). `Drop` removes the cell row (`633-647`). The `CellDebt` q/j fields (`289-296`) are normalized penalties, not a substitute for raw q/J.
- In BL, `MeshShape` is the existing state lookup (`SU2_CFD/include/adaptation/native_bl3d/BLVolume13.inl:145`, with repeated use at `455-460`, `485-500`). The currently wired callbacks at `281` and `534` call `MeshShape(s,id)` for any ID currently in `s.mesh`, otherwise return false. The whole-mesh optimizer is invoked at `558`; local optimization passes a copied `local` mesh at `270` and invokes it at `284`.

## Smallest safe wiring boundary

Reuse is results-identical only when the returned q/J belong to the exact old cell geometry being scored. The current `Id` callback signature does not itself establish that: optimizer-local IDs can be reassigned (`CNativeEngine3D.cpp:1120-1122`), local optimization scores a copied region (`BLVolume13.inl:270,284`), and the BL callback looks up the caller's live mesh by ID (`281,534`). Missing IDs correctly fall back; a reused ID that names a different current/private cell could return a valid but wrong old value. Callback lookup also needs freshness across `Apply`/state updates; a stale publication must not survive changed connectivity/coordinates or q/J refresh.

The minimal safe contract is to publish an immutable per-optimizer snapshot for eligible old rows only, keyed by identity **and** a matching cell/version certificate (or exact vertex IDs and coordinates), with a measured/finite-q-and-J check. For a local copied mesh, construct eligible entries by matching each local row to the corresponding original region row before optimization; do not infer correspondence from local ID alone. Return false for private/new cells, changed cells, unmeasured values, and any mismatch so the existing exact `Quality(c)` path remains authoritative. Snapshot lifetime should be one `OptimizeMetricMesh` call, preventing reuse after publication/update. This preserves the current fallback semantics and requires no change to candidate measurement.

Do not use the optimizer's own `qualities` memo (`CNativeEngine3D.cpp:6140,6172-6179`) as census reuse: it is optimizer-local and keyed by ordered vertex IDs, whereas B4's savings target is the already completed external census. Nor can `Selection::q_value/j_value` be read through the BL callback without a deliberate bridge: `Selection` is private to Engine3D and distinct from BL `State::MeshShape` storage.

## Callers and focused test targets

- Callback installations: `BLVolume13.inl:281` (local V13 optimizer) and `:534` (mesh-wide V13 demand-queue optimizer). The V12 whole-mesh optimizer (`:527-558`) does not set it; generic `OptimizeMetricMesh` users such as `CNativeEngine3D.cpp:1128` and `:7252` remain uncached and use exact optimizer measurement.
- Existing optimizer exercise: `UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp:2761-2766` directly calls `OptimizeMetricMesh`; it is the natural location to exercise hit/fallback/results parity if later authorized. `UnitTests/SU2_CFD/adaptation/CNativeRemesher3D_tests.cpp:801-806` covers remesher callback behavior, not the `mesh_quality` callback. Scoped search found no direct `mesh_quality` test.
- Required edge cases for a future focused check: valid old-cell cache hit equals direct `Quality`; absent/unmeasured row falls back; same local ID with changed vertices/coordinates is rejected; fresh candidate cannot hit old cache; post-update/new optimizer invocation cannot observe a stale published snapshot. No tests were run for this source review.

## Limits

This is a source contract review, not runtime proof that the current external `MeshShape` is equivalent to the optimizer's `Measure` at its centroid in every field/configuration. Before wiring, confirm that both calculate the same q and scaled-J definition, centroid metric, and shape-enabled conditions. The safe fallback is already present; the reuse must never broaden its eligibility based on ID membership alone.
