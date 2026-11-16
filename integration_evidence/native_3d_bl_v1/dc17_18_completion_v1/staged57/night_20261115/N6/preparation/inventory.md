# N6 preparation inventory (read-only)

**Scope:** bounded symbol/state/caller/side-effect inventory for the future N6 cuts only. This is not N6's whole-item source check, not an acceptance, and not permission to start before N4 has passed G5. No production or existing helper files were changed; no tests, build, solver, or job handles were touched.

**Instruction pins**

- `AGENTS.md` SHA256: `1eb12bf43c9eb2803f516de582e6e721853f8a57e24807ffc73907047259c046`
- `NIGHT_WORK_ORDER.md` SHA256: `836173151939978d53310f24dd90b0ce78fc11de67014b72d551dc55eeb389a0`
- N6 order: work-order lines 85 and 108-109: after N4 complete; same method/gates as C sections 1, 2, 5; identity of integer counters; one commit per sub-step. N7 follows. 

## Source pins and current locations

- `Common/src/adaptation/CNativeRemesher3D.cpp`, SHA256 `3a9c8cf43d9c4a25ed9e925df5912fca7c6c2c6824c96beb5d23856c6ca74b3d`; `CNativeRemesher3D::Remesh` begins line 201 and runs through line 1126.
- `Common/src/adaptation/CNativeEngine3D.cpp`, SHA256 `c953482705fc1686b14db19ee027f2c795687522e614275e608444a7ebc8566a`; `Selection` lines 415-1294; `Adapt` begins at line 1382 (its body and wrappers must be re-located from the live source before an edit); `MeshOptimizer` lines 6515-6775 with method bodies through 7617; `OptimizeMetricMesh` line 7620; `RefineOptimize` begins 7660.
- Hashes pin this inventory snapshot; re-hash before actual N6 because the active work may move the source.

## 1. `CNativeRemesher3D::Remesh`

**Entry/callers.** Public implementation at `CNativeRemesher3D.cpp:201`, declared in `CNativeRemesher3D.hpp`. Production construction/invocation is through `CSinglezoneDriver.cpp:426,458,522`; BL pipeline declarations/wiring are in `SU2_CFD/include/adaptation/CNativeBLPipeline3D.hpp`. The driver invokes one remesh operation for adaptation.

**State/context.** Function owns `World parent`, result and resettable `stats`; copies `options` to mutable `control`; opens the `remesh` `Session` and adds its reserved storage to `caller_bytes`; carries a single deadline, phase clock, memory ceiling and shared work budget through a `do` workflow. Persistent-in-function data include pending publication/reference state, normalization/composition, imported and working cells, donor partitions/migration, worker context, target geometry reservation, outputs/return slices, and final reporting state.

**Observable effects.** MPI collectives/elections, repartition/exchange, calls to native adaptation operations, retained `reference` publication, mutation of `stats` and returned mesh/state, process/job RSS sampling, timer-tree rows, and rank-serialized `NATIVE3D` output. Failures are elected and may return incomplete status; exceptions in local setup are converted/elected at specific boundaries. `control.caller_bytes` and `memory_ceiling` feed admission decisions.

**Possible identical-result cut seam.** The routine is an orchestration boundary, so only a narrow extraction of a contiguous phase block with the same live state/captured references is plausible. Do not change call order, collective count/order, per-rank branch agreement, `phase()` boundaries, the deadline origin, resource admission points, `Session` layout/reserved bytes, or the single publication/return path. A seam that crosses a conditional collective or moves allocation across `admit()` changes deadlock or memory behavior even if mesh decisions appear unchanged. Its many long-lived captures and `break` exits make helper extraction higher risk than a leaf operation.

## 2. `Selection` in `CNativeEngine3D.cpp`

**Role/callers.** One class combines cached census state and work-queue state. Constructed in `Adapt` at lines 1469 and 3207 (the second is pending selection for repair/batch staging). `Adapt` calls its census/update, candidate selection, cache, queue, phase, and commit-update methods across the round loop. No external production caller directly constructs it.

**State.** Holds authoritative per-cell cached state/debt/versions; edge incidence, metric edge measures, lengths and forbidden directions; geometry/surface/repair queues; vertex-to-cell and blocked-edge indexes; repair-feature membership, severity, age/epoch; counters/debt references; flags for quality-only, surface planning, coarsening/repair, split lanes, timings, and geometry authority. `Bytes()` accounts for these maps/sets/links, so changing member ownership or duplicating state affects admission estimates.

**Existing natural method seams.** Census/update side: `Diagnose` (639), `Account` (585), `RefreshQuality` (945), `Add` (994), `Drop` (851), `ChangePhase` (898), and repair-state methods. Queue side: `Best` (657), `Batch` (717), `RepairBatch` (673), `Queue` (486), `Block` (754), `Touch` (785), `ServiceEdge` (1161), `SeedFrontier` (1154), and collapse filtering (812). These are candidates for locating a cut, not isolated components: methods mutate state used by both sides.

**Boundary hazards.** `Add` (994) / `Drop` (851) update cell debt, edge-user counts, incident indexes, repair-demand/severity structures, queue eligibility, and statistics together. `Touch` (785) / `Block` (754) can invalidate/reactivate choices and change edge history. `ChangePhase` (898) reuses cached edge measures only after identity/version checks and also reseeds queues. Separating queue ownership from census ownership requires preserving single-source consistency and exactly the same invalidation, tie ordering, and iteration order. No additional cache/map/member is free: `Bytes()` and caller admission must stay equal.

## 3. `MeshOptimizer`

**Role/callers.** Internal class at `CNativeEngine3D.cpp:6515`; `Run()` at 7568. Main entry `OptimizeMetricMesh` at 7620; called by `OptimizedRepairPatch` at 1347 (used by `Adapt`'s `optimize_patch` lambda at 4527) and the `RefineOptimize` path at 7695. Recursive use occurs in `CoupledBoundary` (nested optimizer around 7524).

**State.** References the mutable `Owned` mesh, immutable target, next node/cell IDs, options, and report. Owns bounded caches/indexes for vertex stars, points, physical faces, quality/tensor/exact-edge/height evaluations, fixed/kept/sliding vertices, rejected smooths, moved geometry and time/work limits. The report is both output and live decision input (trial/work counters and quality data); optimizer operations mutate mesh and ID counters.

**Observable effects.** Mesh topology/coordinates/physical facet bindings, allocated identities, `OptimizeReport` counts/stop reason/timing/atlas reservation deltas, target and geometry queries, bounded cache clearing, geometry parameter/coverage changes, and nested optimizer work. Candidate operations have rejection counters and validations; `Commit` mutates mesh plus all indexes and enforces physical binding invariants.

**Possible identical-result cut seam.** `Run()` stage loop is a possible orchestration seam only if all current caches and report state remain in the same optimizer instance and stage order/work reservation remains exact. Method extraction within a single instance is lower risk than splitting ownership/state. Keep `Index()` initialization, `Limited()` checkpoints, `Bound()` cache clears, `Commit()` (6697) ordering, nested recursion/coupling, and final census/`report.seconds` unchanged. Moving a cache across optimizer instances, changing cache eviction timing, or changing the time/work clock changes outcomes.

## Cross-cut and execution hazards

- The adaptation engine has private/communication-separated paths, but branch predicates and `World` collectives are rank-synchronous. Preserve collective call sequence and sequence within each selected path; do not put a collective behind rank-local conditions.
- Timers are behavior-adjacent: `Session` construction can change reserved storage; `PhaseScope` / `Scope` affect timer rows; wall-clock checks in `Limited`, `Remesh` deadlines, and geometry reservation callbacks can change accepted counts. Preserve clock start and check sites exactly for an identity cut.
- Resource model hazards: `Selection::Bytes`, `EngineOptions::caller_bytes`, `transfer_memory` bounds, optimizer `cache_entries`, stage work reservation and atlas reservation all affect admission/stop decisions. Member moves/copies can change modeled bytes or allocation peak.
- Iteration/tie hazards: ordered maps/sets, queue priorities and equality/tie order feed candidate order. Keep container types, comparator, insertion/erase order, and loop structure as they stand unless C explicitly directs otherwise.
- User's running six-rank solver remains untouched. Repo-wide `git status` could not fully complete because its configured Eigen submodule worktree is missing; this does not affect these file reads or hashes.

## Deferred until N6 is authorized

Before any N6 edit: wait for N4 G5; re-read current N6/C sections and current `AGENTS.md`/work-order state; re-hash the exact active source; perform the complete N6 source check for the connected sub-step(s), including callers, gates, timers, collectives and storage; record all contradictions together. This preparation inventory does not satisfy those steps.
