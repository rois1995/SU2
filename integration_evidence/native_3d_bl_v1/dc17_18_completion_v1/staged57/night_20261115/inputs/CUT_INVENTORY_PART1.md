# CUT INVENTORY, PART 1: `Adapt` setup (start of function up to the round loop)

Source: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/Common/src/adaptation/CNativeEngine3D.cpp`.
All line numbers refer to a frozen snapshot taken at the start of this inventory
(`/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/cut/snap.cpp`,
7444 lines, md5 `27237345887b00e3f5db0eba6e8795fb`). The live file can drift by a few lines; names are given
with each line so that every item can be found again.

* `Adapt` starts at **line 1237** (signature 1237-1239) and ends at **line 6133** (`}` before `}  // namespace` at 6134).
* The round loop **`for (size_t round=0;;) {` starts at line 3285.**
* This part covers lines **1237-3284** (2048 lines, about 151 kB of text).

Unit names used below: **Census**, **Queues**, **Operators**, **Trial**, **Commit**, **Exchange**, **Control**, **Ownership**.

---------------------------------------------------------------------------------------------------

## 1. Signature, parameters, callers

```cpp
EngineResult Adapt(World& world, const Owned& accepted, const std::vector<DonorCell>& original,
                   const PlanarReference& accepted_geometry, MetricComposition composition, EngineOptions options,
                   MetricChange intent, bool bidirectional=false, BoundaryMetric surface_target={})   // 1237-1239
```

It is in `namespace SU2Native3D { namespace { ... } }` (anonymous namespace 34-6134).

| Parameter | Type | Passing | Meaning |
|---|---|---|---|
| `world` | `World` = `SU2NativeBoundary2D::World` | non-const ref | MPI context: `rank`, `size`, `comm`, `elect`, `maximum`, `exchange`, `seconds`, collective counters (`collective_calls`, `collective_seconds`, `election_collective_*`, `bytes_sent`, `max_exchange_work_bytes`). Bound by reference inside `Directory`, `CavityMetricSource`, `RegionFieldService`. |
| `accepted` | `Owned` = `std::map<Id,OwnedCell>` | const ref | Locally owned input cells (this rank's partition). Copied into `working` at 2270. |
| `original` | `std::vector<DonorCell>` | const ref | Frozen donor cells holding the source sensor metric; used by `source.Build` (2294), id maxima (2276, 2372), stage observer, `bytes()`. |
| `accepted_geometry` | `PlanarReference` | const ref | Input geometry authority; replaced by a private/atlas view (`geometry`, 1301). |
| `composition` | `MetricComposition` = `std::function<Tensor(Point,Tensor)>` | **by value** | Optional composed target (sensor -> target tensor). `bool(composition)` switches many paths. |
| `options` | `EngineOptions` | **by value, mutated** | All controls. `options.caller_bytes` is increased at 1248, 1253 and 1286; afterwards `options` is read-only. |
| `intent` | `MetricChange` | value | Refine / Coarsen / Repair. Only read in setup (1304, 2220). |
| `bidirectional` | `bool` | value | Refine+coarsen in one run (AdaptMetric and PrepareSurface pass `true`). |
| `surface_target` | `BoundaryMetric` = `std::function<...>` | **by value** | Non-empty only for surface preparation (`PrepareSurface`). `bool(surface_target)` switches many paths; passed to `MeasureSurfacePlanning` (2832). |

Callers of `Adapt` (all in the same file, public `SU2Native3D` functions after the anonymous namespace):

| Caller | Definition line | Call line | Arguments |
|---|---|---|---|
| `PrepareSurface` | 6169 | 6177 | `composition={}`, `Refine`, `bidirectional=true`, `surface_target=target` (clears `options.edge_breakpoints` first, 6176) |
| `AdaptMetric` | 6179 | 6181 | `Refine`, `bidirectional=true` |
| `Refine` | 6183 | 6185 | `Refine` |
| `Coarsen` | 6187 | 6189 | `Coarsen` |
| `Repair` | 6191 | 6193 | `Repair` |

Production callers of those wrappers: `Common/src/adaptation/CNativeRemesher3D.cpp:656` (`AdaptMetric`),
`SU2_CFD/include/adaptation/CNativeBLPipeline3D.hpp:453` (`PrepareSurface`); many unit tests in
`UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp` (e.g. 865, 1272, 4908, 5256).

---------------------------------------------------------------------------------------------------

## 2. STATE (function-scope locals declared in 1237-3284 that live across the function)

Legend: **C** = const (or never written) after setup; `RAII` = object whose destructor has an effect.
"Written by" lists the writers found by text search (setup straight-line code = S, lambdas by name,
loop = L with the first loop write lines). Loop counts are for lines 3285-6133.

Total: **112 named local variables** (including 3 `constexpr` and 2 RAII guards), plus the alias `Phase`
and 4 local *types* (2.9). The 9 parameters are listed in section 1 and not counted here.
The live file was re-checked after writing: same md5, loop still at 3285.

### 2.1 Control (phases, budgets, deadlines, checkpoints, stage reports)

| Name | Type | Decl | C | Purpose | Written by |
|---|---|---|---|---|---|
| `admission` | local struct `RestoreAdmission` (RAII) | 1240 | RAII | Saves global `GetShapeAdmission()` and restores it on exit; 1241 sets `ValidityOnly`. | ctor/dtor only |
| `Phase` | alias `EnginePhase` | 1242 | - | Type alias. | - |
| `phase` | `EnginePhase` | 1243 | | Construction / Improvement. | `enter_phase` only (L reads 27x) |
| `timer_session` | `SU2NativeTiming3D::Session` (RAII) | 1244 | RAII | Timing ledger session (Surface or Construction phase). `Owns()` adds reserved bytes to `options.caller_bytes` (1248). | ctor/dtor |
| `region_rebuilt` | bool | 1249 | | Region construction replaced the domain; read by `finish`. | S 2489 |
| `improvement_changed` | bool | 1249 | | Improvement phase changed the mesh. | `enter_phase`, `rollback_checkpoint`, L 5819, 6084 |
| `construction_changed` | bool | 1249 | | Construction phase changed the mesh. | `enter_phase`, `rollback_checkpoint`, L 5820 |
| `construction_handoff_progress` | bool | 1249 | | Construction->Improvement hand-off made progress. | `enter_phase`, `rollback_checkpoint` |
| `construction_generation_changed` | bool | 1250 | | A length generation changed something. | `begin_length_generation`, `enter_phase`, L 3527, 5821 |
| `metric_cleanup`, `metric_cleanup_tried` | bool | 1251 | | Metric clean-up sub-phase flags. | `enter_phase` (cleanup only), L 3557 |
| `phase_start_round` | uint64_t | 1252 | | Round at phase start (stagnation/round windows). | `begin_length_generation`, `enter_phase`, L 3557 |
| `started` | `Clock::time_point` | 1254 | C | Wall-clock origin for every deadline/elapsed value. | - |
| `started_cpu` | `std::clock_t` | 1255 | C | CPU origin (timings only). | - |
| `local_atlas_budget` | `SU2NativeGeometry::AtlasBudget` | 1257 | | Own budget when no shared one is supplied. | via `atlas_budget` |
| `atlas_budget` | `AtlasBudget&` | 1258 | | Shared or local geometry work/byte/time budget. Fields written: `within_time` (1270), `caller_bytes`, `maximum_bytes`, `maximum_integer_bits` (1288-1293), `maximum_work` (2208), `time_exhausted` (`vote`, L 5633), `caller_bytes`/`maximum_bytes` (`admit`). | S, `vote`, `admit`, L |
| `atlas_work_initial` | size_t | 1259 | C | Budget work at entry (for `stats.atlas_work`). | - |
| `restore_budget` | local struct `RestoreBudgetWindow` (RAII) | 1260-1265 | RAII | Restores the shared budget's caller/max bytes, integer bits and max work on exit. | ctor/dtor |
| `target_geometry_lease` | `std::shared_ptr<void>` | 1267 | C after 2214 | Lease returned by `options.geometry_reservation(atlas_budget)`; released at exit. | S 2214 |
| `atlas_scratch` | size_t | 1268 | C | Fixed atlas integer workspace bytes; subtracted in `real_bytes`/`admit`. | - |
| `parameter_failure` | `CLocalFailure` | 1274 | setup-only | Failure of parameter staging (elected at 1298). | S |
| `parameter_bound` | size_t | 1274 | C after 1281 | Storage bound of parameter stage; seeds `stats.maximum_work_bytes`. | S |
| `parameter_setup` | double | 1302 | C | Seconds of parameter setup. | - |
| `standalone_coarsening` | bool | 1304 | C | `intent==Coarsen`. Read 8x in loop. | - |
| `coarsening` | bool | 1305 | | Current length direction (collapse vs split). Also copied into `Selection`. | `length_direction` |
| `repairing` | bool | 1306 | | Improvement (repair) mode. | `enter_phase`, L 3557 |
| `construction_kind` | `MetricChange` | 1307 | | Refine or Coarsen for construction trials. | `length_direction` |
| `tracked_bytes_begin`, `tracked_calls_begin`, `tracked_seconds_begin` | counters | 1308-1310 | C | Baselines of `world` traffic counters for `stats.tracked_*`. | - |
| `election_calls_begin`, `election_seconds_begin` | counters | 1311-1312 | C | Baselines of election counters (stage report). | - |
| `geometry_begin` | `GeometryStats` | 1314 | C | Geometry cache stats at entry (stage report deltas). | - |
| `result` | `EngineResult` | 1315 | | The output. `reason`, `valid`, `surface_prepared`, `splits`, `collapses`, `cells`, `parameters`, `reference`, `rebuilt_domain`. | almost everyone (S 21 writes, L 85 writes) |
| `stats` | `EngineStats&` (= `result.stats`) | 1316 | | All counters/timers. Also held by reference inside `Selection`. | everyone (S 55, L 253 writes) |
| `generation_window`, `generation_window_count` | `std::array<GenerationDebt,4>`, size_t | 1324 | | Sliding window of generation debts for stagnation decisions. | `enter_phase` (count), L 3463-3468 |
| `checkpoint` | local struct `CoarseningCheckpoint` | 1335-1349 | | Rollback snapshot (cells, parameters, directory, blocked edges, edge sets). Its `Bytes()` is part of `bytes()`. | `capture_checkpoint`, `discard_checkpoint`, `rollback_checkpoint`, `service_frontier` (`attempted`), L 5803-6083 |
| `accepted_bytes` | size_t | 1369 | C | `MapBytes(accepted)`; part of `bytes()`. | - |
| `private_optimizer_allowance` | size_t | 1389 | | Per-trial private optimizer work allowance. | L 3919 |
| `batch_bytes` | size_t | 1420 | | Bytes of the current batch; part of `bytes()`. | L 3691, 3752, 3772, 3914, 5597, 6049 |
| `diagnostic_bytes` | size_t | 1422 | C | Fixed diagnostic reserve in `bytes()`. | - |
| `FixedMetricSamples` | constexpr 16 | 1421 | C | Sample count of drift diagnostic. | - |
| `fixed_metric_points` | `std::vector<FixedMetricPoint>` | 1423 | | Drift diagnostic sample points. | `emit_fixed_metric_drift` |
| `fixed_metric_cells` | `std::array<Id,16>` | 1424 | | Cell containing each sample. | `emit_fixed_metric_drift`, `consider_fixed_metric_row` (via `update_`/`reset_fixed_metric_cells`) |
| `fixed_metric_initialized`, `fixed_metric_available` | bool | 1425 | | Drift diagnostic state. | `emit_fixed_metric_drift` |
| `last_rejection` | std::string | 1556 | | Last local rejection text (appended to failure reason; exposed to observer). | S 2441, 2444, L 3649, 4598, 5699, 6018 |
| `diagnostic_rejections` | `std::vector<RejectionDiagnostic>` | 1557 | | Ring buffer (64) of rejections; reserved only if the stage observer has room (1559-1560). | `record_rejection` |
| `diagnostic_sequence` | uint64_t | 1557 | | Ring buffer sequence. | `record_rejection` |
| `diagnostic_rejection_limit` | constexpr 64 | 1558 | C | Ring size. | - |
| `progress_at_window`, `attempted_at_window`, `deferred_at_window` | uint64_t | 1574 | | Progress counters at window start (stagnation). | `begin_length_generation`, `enter_phase`, L 3558-3576 |
| `observe_rejections` | bool | 1629 | C | Any rank has `rejected_trial`/`transaction_observer` (collective `world.maximum`). | - |
| `observe_state` | bool | 1630 | C | Any rank has `rejected_state` (collective). | - |
| `observe_stage` | bool | 1631 | C | Any rank has `stage_observer` (collective). | - |
| `stage_storage_admitted` | bool | 1632 | C after 2165 | Storage for stage observer admitted. | S 2165 |
| `stage_observer_failed` | bool | 1632 | | Sticky observer failure. | `observe_progress` |
| `construction_entry_reported` | bool | 1632 | | "construction_entry" stage reported once. | `enter_phase` |
| `previous_quality`, `previous_quality_census` | `QualityRepairStats`, `StageProgress` | 1766 | | Previous quality census (window deltas in report). | `emit_quality_census` |
| `previous_quality_elapsed`, `previous_quality_collective` | double | 1767 | | idem. | `emit_quality_census` |
| `quality_census_serial`, `previous_quality_construction`, `previous_quality_ownership` | uint64_t | 1768 | | idem. | `emit_quality_census` |
| `quality_census_started` | bool | 1769 | | idem. | `emit_quality_census` |
| `transaction_captured` | bool | 2028 | | Transaction observer captured once. | L 4117, 4601 |
| `layer_active`, `region_active` | bool | 2029 | | A layer/region timing window is open (closed by `finish`). | S, `finish` |
| `layer_started`, `region_started` | `Clock::time_point` | 2029 | | Start of these windows. | S |
| `ready` | bool | 2168 | | Generic "still OK" flag reused by setup and the loop. | S 11x, L 62x |

### 2.2 Census (rows, debts, measure)

| Name | Type | Decl | C | Purpose | Written by |
|---|---|---|---|---|---|
| `selection` | `Selection` (class at 271) | 1318 | | **Shared object**: census rows (`cells`, `edges`, `invalid`, `construction_invalid`, progress) *and* queues (`queue`, `surface_queue`, `geometry_queue`, `repair_queue`, `blocked_edges`, `edge_history`, lanes). Holds references to `geometry`, `stats`, `options.wall_heights`. Flags set at 1319, 1330. | `measure`, `service_frontier`, `length_direction`, `begin_length_generation`, `enter_phase`, `phase_ownership` (`SwapState`), `rollback_checkpoint` (`Reset`), S 3277 (`SeedFrontier`), L (many) |
| `edge_progress` | `GenerationProgress3D` | 1321 | | Global edge census result (long edges, debt, histogram, top). | `generation_census` (via `MeasureGenerationProgress`) |
| `edge_census_round` | uint64_t | 1322 | | Round of last edge census (report). | `generation_census` |
| `retired_edges`, `campaign_edges` | `std::map<Edge,EdgeMeasure>` | 1325 | | Edge measurement reuse caches (retired edges of removed cells; campaign cache). Part of `bytes()`. | `phase_ownership`, `enter_phase` (`retired_edges.clear`), `rollback_checkpoint`, `measure` (passes `&campaign_edges` to `MetricEdgeRequests`/`MeasureMetricEdges`), L 5811, 5871, 6073, 6111 |
| `field_identity` | `std::shared_ptr<const FrozenMetricIdentity>` | 1328 | C after 2268 | Identity of the frozen target (SurfacePlanning or FullTarget). | S 2268 |
| `initial` | `std::vector<Id>` | 1373 | | Cell ids to seed/measure. Part of `bytes()` **by capacity**. | S 2274, 2281, 2491-2492, 2754-2755, `enter_phase`, `phase_ownership` (`swap`) |
| `seed_slot` | size_t | 1374 | | Seeding cursor into `initial`. | `seed_next`, `enter_phase`, `phase_ownership` |
| `MeasurementBatch` | constexpr 16 | 2763 | C | Measurement batch size. | - |

### 2.3 Queues (selection, priority)

| Name | Type | Decl | C | Purpose | Written by |
|---|---|---|---|---|---|
| `inactive_blocked_edges` | `std::map<Id,std::set<Id>>` | 1326 | | Blocked-edge set of the inactive length direction (swapped with `selection.blocked_edges`). | `length_direction`, `phase_ownership` |
| `inactive_blocked_links` | size_t | 1326 | | Its link count. | `length_direction`, `phase_ownership` |
| `length_blocks_dirty` | `std::array<bool,2>` | 1327 | | Blocked sets must be cleared on next direction entry. | `length_direction`, `enter_phase`, `phase_ownership`, L 3476 |
| (`selection`) | | | | queue half of the shared object, see 2.2 | |

### 2.4 Operators (calls into boundary/cavity code)

| Name | Type | Decl | C | Purpose | Written by |
|---|---|---|---|---|---|
| `private_geometry` | `std::shared_ptr<const PlanarReference>` | 1256 | C after 1285 | Geometry view over the private parameter state. | S 1285 |
| `atlas_geometry` | `std::unique_ptr<PlanarReference>` | 1266 | C after 1294 | Geometry view with atlas reservation. | S 1294 |
| `no_coverage` | `const BoundaryCoverage` | 1273 | C | Empty coverage passed to Publish on rejection (L 5771). | - |
| `mapped`, `mapped_low`, `mapped_high` | unsigned | 1275-1276 | C | Parameter-geometry mode and its global min/max. | - |
| `geometry` | `const PlanarReference&` | 1301 | C | **The** geometry authority used everywhere (atlas, else private, else accepted). Bound by reference into `Selection`. | - |
| `curved` | bool | 1303 | C | Geometry has extension components. | - |
| `geometry_authority` | `const MappedFacetReference*` | 1313 | C | Exact facet authority (or null). | - |
| `eligible_extensions` | `std::vector<Id>` | 1373 | C after 2275 | Extension components for trial imports (L 4470, 5132, 5394). Part of `bytes()` by capacity. | S 2275 |
| `global_original` | uint64_t | 2340 | C | Global donor count (region import gate). | - |
| `prismatic` | bool | 2341 | C | Prismatic wall-region applicability. | - |
| `region_applicable` | bool | 2343 | setup-only | Region construction applicable. | S 2343-2365 |

### 2.5 Trial (validity and requirement checks)

| Name | Type | Decl | C | Purpose | Written by |
|---|---|---|---|---|---|
| `private_repair` | bool | 1388 | | A private (rank-local) repair trial is running. **Changes `vote` from collective to local** (1391). | L 5588, 5612, 5631, 5634 |

### 2.6 Commit (apply, ids, metric transport, parameter changes)

| Name | Type | Decl | C | Purpose | Written by |
|---|---|---|---|---|---|
| `private_parameters` | `std::shared_ptr<BoundaryParameterState>` | 1256 | contents mutable | Private boundary parameter state (all parameter edits land here through `Directory::Publish`). | S 1284; contents swapped by `rollback_checkpoint` |
| `batch_changed` | `std::vector<Id>` | 1426 | | Cells changed by the current batch. Part of `bytes()` by capacity. | L 5852, 6076, 6078, 6111 |
| `batch_touched` | `std::set<Id>` | 1427 | | Touched cells. | L 5853 |
| `batch_supports` | `std::set<std::array<Id,4>>` | 1428 | | Supports of the batch. | L 5854 |
| `maximum_node`, `maximum_cell` | Id | 2169 | **C after 2759** (never written in the loop) | Base for fresh node/cell id reservations (loop uses `maximum_node+1+offset`). | S (2276-2290, 2373-2377, 2492-2497, 2755-2759) |

### 2.7 Exchange (directory, reserve, clearance, publication, MPI)

| Name | Type | Decl | C | Purpose | Written by |
|---|---|---|---|---|---|
| `construction_claims` | bool | 1320 | | Directory construction pass (claims) active. | `begin_length_generation`, `enter_phase`, `phase_ownership`, L 3445, 3454, 3553 |
| `directory` | `std::unique_ptr<Directory>` | 1333 | | Distributed cell/feature directory (lookup, fetch, reserve, publish, construction passes). Holds `world&` and `geometry.Parameters()` pointer. | `phase_ownership` (`swap`), `rollback_checkpoint` (`swap`) |
| `source` | `CavityMetricSource` | 1334 | | Donor/metric import service. **Holds `Owned& working` and `World&`**. Collective imports. (Metric transport is Commit by purpose, but every call is collective.) | `Build`, `SetCavity`, imports |
| `retired_directory` | `DirectoryStats` | 1350 | | Accumulated stats of retired directories. | `retire_directory`, `finish` (save/restore) |

### 2.8 Ownership (partition, migration)

| Name | Type | Decl | C | Purpose | Written by |
|---|---|---|---|---|---|
| `working` | `Owned` | 1317 | | The local partition of the mesh being edited. **Referenced by `source`**. Mutated in place by `directory->Publish(working,...)` (S 2332, 2486, 2742; L many). | S 2270, `phase_ownership` (`swap`), `rollback_checkpoint` (`swap`), `finish` (`swap` into `result.cells`) |
| `ownership_evaluated` | `std::array<bool,2>` | 2911 | | Ownership evaluated at construction/quality boundary. | `phase_ownership` |
| `quality_ownership_round` | uint64_t | 2912 | | Round of last quality ownership evaluation. | `phase_ownership` |

### 2.9 Local types declared inside `Adapt` (must be hoisted)

| Type | Lines | Use |
|---|---|---|
| `RestoreAdmission` | 1240 | RAII restoring global shape admission |
| `RestoreBudgetWindow` | 1260-1265 | RAII restoring shared atlas budget window |
| `GenerationDebt` | 1323 | Element of `generation_window`; returned by `generation_debt` |
| `CoarseningCheckpoint` | 1335-1349 | Type of `checkpoint` (has `Bytes()`) |

---------------------------------------------------------------------------------------------------

## 3. LAMBDAS (function level, lines 1237-3284)

37 function-level lambdas (`auto name=[&](...)` at indentation 2), plus one stored lambda
(`atlas_budget.within_time`, 1270-1272, **captures by value**) and the block-level lambdas inside the
setup blocks (section 4). Lengths are in source lines (many statements per line).
"MPI" = makes collectives directly or through `vote`/`admit`/`world.*`/`TimedAllreduce`/directory/source calls.
`vote` is collective **except when `private_repair` is true** (then `elect_trial_failure` is local);
therefore `admit` and every caller of `vote` are conditionally collective.

| # | Name | Lines | Len | Purpose | Writes | Reads (short) | Calls | MPI | Unit |
|---|---|---|---|---|---|---|---|---|---|
| 1 | `retire_directory` | 1351-1368 | 18 | Add a directory's stats into `retired_directory`. | `retired_directory` | arg | - | no | Exchange |
| 2 | `selection_bound` | 1370-1372 | 3 | Per-cell selection memory bound. | - | `coarsening`, `bidirectional` | - | no | Control (budget) |
| 3 | `scope_census_complete` | 1375-1379 | 5 | Local census complete (surface mode: requested-wall census). | - | `surface_target`, `selection`, `working` | - | no | Census |
| 4 | `refresh_surface_census` | 1380-1387 | 8 | Copy surface census counts into stats. | `stats.surface_demand_*` | `selection`, `working` | - | no | Census |
| 5 | `elect_trial_failure` | 1390-1393 | 4 | Elect a failure; local when `private_repair`. | - | `world`, `private_repair` | - | yes (`world.elect`) unless private | Exchange |
| 6 | `vote` | 1394-1419 | 26 | Elect failure + sticky atlas refusals (work/backend/time, severities 4/5/6); on quota refusal elect the original failure **a second time**; set `result.reason/valid/surface_prepared`, `stats.stop`. | `result`, `stats.atlas_*`, `stats.maximum_work_bytes`, `stats.stop`, `atlas_budget.time_exhausted` | `atlas_work_initial`, `private_repair` | `elect_trial_failure` (1 or 2x) | yes (conditional) | Exchange (writes Control state) |
| 7 | `bytes` | 1429-1435 | 7 | Live resident bytes of all engine state (capacity-based for vectors). | - | `options.caller_bytes`, `accepted_bytes`, `batch_*`, `working`, `checkpoint`, `initial`, `eligible_extensions`, `result.splits/collapses`, `selection`, `retired_edges`, `campaign_edges`, `inactive_blocked_*`, `field_identity`, `diagnostic_bytes`, `original` | - | no | Control |
| 8 | `real_bytes` | 1436 | 1 | `bytes()` minus atlas scratch. | - | `atlas_geometry`, `atlas_scratch` | `bytes` | no | Control |
| 9 | `admit` | 1437-1446 | 10 | Memory admission vote of `bytes()+source+directory+extra`; update peak and atlas window. | `stats.maximum_work_bytes`, `atlas_budget.caller_bytes/maximum_bytes` | `options`, `source`, `directory`, `atlas_*` | `bytes`, `vote` | yes (via `vote`) | Control (budget) + Exchange (vote) |
| 10 | `coalesce` | 1447-1501 | 55 | Replicate local proposals, globally sort by priority, drop duplicates, assign `reservation_priority`/`repair_epoch`. | `choices` (arg), `stats.proposal_duplicates`, `result.reason` | `world`, `repairing`, `coarsening`, `curved`, `composition`, `bidirectional`, `selection.split_lanes`, `options.candidate_batch` | `admit`, `vote` x2, `bytes` | yes (`world.exchange`) | Queues (priority) with Exchange part |
| 11 | `service_frontier` | 1502-1555 | 54 | Exchange service receipts of the elected frontier; apply `selection.Service`/`Note`/`ServiceEdge`; note checkpoint attempts. | `selection`, `checkpoint.attempted`, `result.reason` | `coarsening`, `repairing`, `checkpoint.valid/changed/edges` | `admit`, `vote` (per receipt when `certify_support`), `bytes` | yes (`world.exchange`, `world.maximum` **per receipt**) | Queues with Exchange part |
| 12 | `record_rejection` | 1561-1573 | 13 | Append to rejection ring buffer. | `diagnostic_rejections`, `diagnostic_sequence` | `stats.rounds` | - | no | Control (report), called by Trial |
| 13 | `height_check` | 1575-1585 | 11 | First-height check of a patch (+provisional ValidityOnly acceptance). | `stats.height_checks/height_rejected/footprint_rejected` | `options.wall_heights`, `geometry`, global `GetShapeAdmission()` | - | no | Trial |
| 14 | `geometry_trial` | 1586-1588 | 3 | Count a geometry trial. | `stats.geometry_trials/rejected` | - | - | no | Trial |
| 15 | `metric_trial` | 1589-1617 | 29 | Metric acceptance with surface/improvement/geometry-repair progress rules. | `diagnosis` (arg), `result.reason`, `stats.metric_*` | `surface_target`, `bidirectional`, `repairing`, `selection.geometry_repairs` | - | no | Trial |
| 16 | `surface_merit` | 1618-1628 | 11 | Neutralize non-requested edges in a surface patch metric. | `metric` (arg) | `surface_target`, `options.wall_heights`, `geometry` | - | no | Trial |
| 17 | `generation_census` | 1633-1654 | 22 | Global edge-length census of the current generation. | `edge_progress`, `stats.unique_*`, `stats.maximum_edge`, `stats.edge_histogram`, `edge_census_round`, `selection.edge_history` (via `RefreshHistory(false)`), `result.reason` | `selection`, `surface_target` | `admit`, `vote`, `bytes` | yes (`MeasureGenerationProgress`) | Census |
| 18 | `generation_debt` | 1655-1665 | 11 | Global generation debt (edge + geometry). | - (returns `GenerationDebt`) | `edge_progress`, `selection`, `working` | - | yes (`TimedAllreduce`, `world.maximum`) | Census |
| 19 | `consider_fixed_metric_row` | 1666-1681 | 16 | Locate drift samples in a cell (swallows exceptions). | `fixed_metric_cells` | `fixed_metric_*`, `source` | - | no | Control (diagnostic) |
| 20 | `update_fixed_metric_cells` | 1682-1686 | 5 | Update drift sample owners after an edit. | `fixed_metric_cells` | - | `consider_fixed_metric_row` | no | Control (diagnostic), called by Commit (L 5774, 6059) |
| 21 | `reset_fixed_metric_cells` | 1687-1690 | 4 | Recompute drift sample owners for all cells. | `fixed_metric_cells` | `working` | `consider_fixed_metric_row` | no | Control (diagnostic), called by Ownership |
| 22 | `emit_fixed_metric_drift` | 1691-1765 | 75 | Initialize (once at "quality_entry") and report metric drift at fixed samples. | `fixed_metric_*`, `stats.quality.diagnostic_seconds` | `working`, `selection.cells`, `source`, `composition` | `bytes` | yes (`world.elect` x1-2, `world.exchange` x1-2) | Control (report) |
| 23 | `emit_quality_census` | 1770-1863 | **94** | Quality census report (histograms, ladders) and window update. | `previous_quality*`, `quality_census_*`, `stats.quality.diagnostic_seconds` | `selection`, `stats`, `world` counters, `started` | - | no | Control (report) |
| 24 | `observe_progress` | 1864-2027 | **164** | Stage hook: RSS memory gate, timing checkpoint, surface census, quality census/drift, STORAGE/STAGE/EDGE prints, stage observer callback. | `result.valid/surface_prepared/reason`, `stats.stop`, `stats.job_peak_rss_lower`, `stats.surface_demand_*`, `stage_observer_failed` | nearly all report state, `phase`, `geometry*`, `edge_progress`, `edge_census_round`, `atlas_budget`, `last_rejection`, `diagnostic_rejections`, `observe_stage`, `stage_storage_admitted` | `refresh_surface_census`, `emit_fixed_metric_drift`, `emit_quality_census` | yes (`JobPeakRSS3D`, `world.elect` 1-3x, `world.maximum`) | Control |
| 25 | `finish` | 2030-2163 | **134** | Final votes, final proofs (surface/geometry/height), final stats, reasons, `observe_progress("exit")`, hand `working` to `result.cells`, `return std::move(result)`. | `result`, `stats`, `working` (swapped out), `retired_directory` (restored), `layer_active`, `region_active` | everything | `vote` (1-4x), `height_check`, `retire_directory`, `observe_progress` | yes (`vote`, `ProveOwnedSurface`, `world.elect`) | Control |
| 26 | `composed_sample` | 2502-2509 | 8 | Compose and validate a target tensor at a point. | `stats.field_*` | `composition` | - | no | Operators (target query; used by trial sampling L 5136, 5137, 5400) |
| 27 | `import_source` | 2510-2515 | 6 | Count and run `source.ImportCells`. **Parameter `requests` is unused.** | `stats.donor_batches` / `layer_donor_batches` | `source`, `directory` | `bytes` | yes (`source.ImportCells`) | Exchange |
| 28 | `measure` | 2765-2882 | **118** | Batched census of cell metrics (construction: edge lengths / surface planning; improvement: metric + shape + height), filling `selection`. | `selection` (`census_started`, `Add`, `RefreshQuality`, `Cached`), `stats` (many), `result.reason`, `stats.stop` | `phase`, `surface_target`, `composition`, `working`, `retired_edges`, `campaign_edges`, `field_identity`, `geometry` | `admit` x2-3, `vote` x3-4 per batch, `height_check`, `import_source`, `selection_bound` | yes (`TimedAllreduce`, `world.maximum` per batch, imports, votes) | Census |
| 29 | `seed_next` | 2883-2900 | 18 | Seed the next measurement batch from `initial` (full seeding in simple modes). | `seed_slot`, `stats.stop`, `result.reason` | `initial`, `working`, `selection.cells`, `phase` | `measure`, `admit`, `vote` | yes | Census |
| 30 | `length_direction` | 2901-2910 | 10 | Switch split/collapse direction (swap blocked sets, set `coarsening`/`construction_kind`). | `coarsening`, `selection.coarsening`, `construction_kind`, `selection.blocked_edges/links`, `inactive_blocked_*`, `length_blocks_dirty` | - | - | no | Queues (writes Control's `coarsening`/`construction_kind`) |
| 31 | `phase_ownership` | 2913-3117 | **205** | Decide and execute a repartition at a phase boundary (cost model, `Repartition`, certificate migration, directory rebuild, selection restore, swap). | `working`, `directory`, `selection` (`SwapState`), `initial`, `seed_slot`, `construction_claims`, `retired_edges`, `campaign_edges`, `inactive_blocked_*`, `length_blocks_dirty`, `ownership_evaluated`, `quality_ownership_round`, `fixed_metric_cells`, `retired_directory`, `stats`, `result.reason` | `checkpoint.valid`, `selection`, `geometry_authority`, `field_identity`, `coarsening`, `repairing` | `scope_census_complete`, `admit` x2, `vote` x3, `bytes`, `selection_bound`, `observe_progress` x1-2, `retire_directory`, `reset_fixed_metric_cells` | yes (many: `world.maximum`, `TimedAllreduce` x>=12, `VertexStarSizes`, `Repartition`, `world.exchange`, `Directory::Build`, `world.seconds`) | Ownership |
| 32 | `begin_length_generation` | 3118-3133 | 16 | Start a split or collapse generation. | `selection.lane/RefreshHistory/ChangePhase`, `construction_claims`, `stats.split_lane/split_generation`, `construction_generation_changed`, `phase_start_round`, `*_at_window` | `working`, `field_identity`, `directory` | `admit`, `vote`, `length_direction`, `selection_bound`; `directory->BeginConstructionPass` | yes | Control |
| 33 | `surface_ready` | 3134-3137 | 4 | Global surface readiness. | - | `selection` | `scope_census_complete` | yes (`world.maximum`) | Control |
| 34 | `enter_phase` | 3138-3198 | 61 | Phase transition: surface-repair entry or Construction/Improvement switch, fresh census, geometry priorities, quality ownership, stage report. | `phase`, `repairing`, `metric_cleanup`, `construction_*` flags, `improvement_changed`, `phase_start_round`, `*_at_window`, `generation_window_count`, `length_blocks_dirty`, `construction_claims`, `construction_entry_reported`, `initial`, `seed_slot`, `retired_edges`, `selection`, `stats`, `result.surface_prepared/reason` | `bidirectional`, `surface_target`, `composition`, `geometry`, `started` | `surface_ready`, `length_direction`, `observe_progress` x1-2, `admit` x1-2, `begin_length_generation`, `vote` x1-2, `measure`, `phase_ownership`, `selection_bound`; `directory->Begin/EndConstructionPass` | yes | Control |
| 35 | `discard_checkpoint` | 3199-3206 | 8 | Free the checkpoint (retire its directory stats). | `checkpoint`, `retired_directory` | - | `retire_directory` | no | Control |
| 36 | `capture_checkpoint` | 3207-3233 | 27 | Snapshot cells/blocked/edges/parameters and prebuild a dormant directory. | `checkpoint`, `stats.coarsen_checkpoints` | `working`, `selection`, `private_parameters`, `result.splits/collapses` | `admit`, `vote`; `checkpoint.directory->Build` | yes (`Directory::Build`, `TimedAllreduce`) | Control |
| 37 | `rollback_checkpoint` | 3234-3272 | 39 | Restore checkpoint (swap parameters, cells, directory, blocked), retire attempted directions, re-enter Improvement. | `*private_parameters`, `working`, `directory`, `selection` (`blocked_*`, `Reset`), `result.collapses/splits` (resize), `retired_edges`, `campaign_edges`, change flags, `stats.coarsen_*`, `checkpoint` | `checkpoint` | `vote` x2, `admit`, `discard_checkpoint`, `enter_phase` | yes (`world.maximum`, votes, `enter_phase`) | Control |
| - | `atlas_budget.within_time` | 1270-1272 | 3 | Elapsed-time predicate stored in the local atlas budget. **Captures `started` and `maximum` by value.** | - | - | - | no | Control |

Summary counts: 37 function-level lambdas; **18 above 15 lines**; **5 above 80 lines**
(`phase_ownership` 205, `observe_progress` 164, `finish` 134, `measure` 118, `emit_quality_census` 94);
`emit_fixed_metric_drift` (75) is just under 80. A sixth lambda above 80 lines is block-level:
`construct` (2598-2705, 108) inside the first-row layer block.

Usage of each lambda inside the loop (3285-6133), for the second helper:
`vote` 80, `finish` 108 (all `return finish()`), `bytes` 39, `admit` 18, `measure` 2 (5857, 6103),
`observe_progress` 12 (3361-3580), `enter_phase` 7 (3372-3585), `service_frontier` 7, `geometry_trial` 6,
`selection_bound` 5, `begin_length_generation` 3, `generation_census` 3, `metric_trial` 3,
`record_rejection` 3, `composed_sample` 3, `surface_merit` 2, `coalesce` 2 (3721, 3881),
`height_check` 2, `update_fixed_metric_cells` 2, `seed_next` 2, `length_direction` 2, `phase_ownership` 2,
`rollback_checkpoint` 2, `scope_census_complete` 2, `real_bytes` 2, `generation_debt` 1, `surface_ready` 1,
`capture_checkpoint` 1, `discard_checkpoint` 1. Not used by the loop: `retire_directory`,
`refresh_surface_census`, `elect_trial_failure`, `consider_fixed_metric_row`, `reset_fixed_metric_cells`,
`emit_fixed_metric_drift`, `emit_quality_census`, `import_source` (only via `measure` and the layer block).

### 3.1 Split points of the lambdas above 80 lines

**`phase_ownership` 2913-3117 (205)**
- 2914-2918 nested `within_time` (collective deadline; sets `stats.stop`, `result.reason`).
- 2919-2932 gating (deadline, single rank/disabled, evaluated-once rules, checkpoint/census/repair-zero/progress collectives, global cell count). Note `ownership_evaluated[boundary]` is set at 2924 (construction) **before** the collective gates but at 2932 (quality) **after** them.
- 2933-2943 scratch admission and `VertexStarSizes` (collective).
- 2944-2993 per-cell cost model (demand, geometry cost, quality debt).
- 2995-3002 normalize to partition weights (2-3 `TimedAllreduce`).
- 3003-3031 imbalance/benefit model, decision vote (`world.maximum`), timing print; return when not migrating.
- 3032-3043 stage pending `Owned`, `Directory`, `Selection` (copies 11 selection flags).
- 3044-3049 `Repartition`.
- 3050-3072 certificate migration (`world.exchange`).
- 3073-3079 pending directory build.
- 3080-3092 restore pending selection (Restore, census check, RestoreRepairState / ChangePhase).
- 3093-3095 staged report and `observe_progress("ownership_staged")` (reports the **old** state).
- 3096-3105 commit swap: `retire_directory`, swap `working`/`directory`/`selection`, `reset_fixed_metric_cells`, clear queue/edge caches, swap `initial`, `construction_claims`, `BeginConstructionPass`, stats.
- 3106-3116 report and `observe_progress("ownership_committed")` (quality only).

**`observe_progress` 1864-2027 (164)**
- 1865-1875 whole-job peak RSS gate (`JobPeakRSS3D`, `world.elect`), sets `stats.stop=MemoryBudget`.
- 1876-1878 timing checkpoint for 3 stage names.
- 1879-1884 surface census refresh + `world.elect`.
- 1885-1890 Improvement: progress census, `world.maximum`, then `emit_fixed_metric_drift` and `emit_quality_census`.
- 1891-1901 NATIVE3D_STORAGE print.
- 1902-1910 coverage piece/limb count (nested `ratio`, `bary`).
- 1911-2003 NATIVE3D_STAGE print (about 90 lines of one statement).
- 2004-2007 NATIVE3D_EDGE print at "length_split_exit".
- 2009-2026 stage observer callback (`options.stage_observer`, nested diagnose lambda 2017) + `world.elect`.

**`finish` 2030-2163 (134)**
- 2032-2037 final vote and merge of the prior reason/stop.
- 2038-2041 close region/layer timing windows.
- 2042-2053 entity-geometry owned-surface proof (`ProveOwnedSurface`, `vote`).
- 2054-2072 non-entity prepared-surface tolerance check (`vote`).
- 2073-2086 final first-height check of every cell (`height_check`, `vote`).
- 2087-2088 `rebuilt_domain`, stop code.
- 2089-2098 rejected-state observer (`world.elect`).
- 2099-2106 final counters.
- 2107-2125 directory stats aggregation (save, retire current+checkpoint, copy, restore).
- 2126-2131 donor stats.
- 2132-2134 atlas stats, `result.parameters/reference`.
- 2135-2142 reason enrichment (local cached demands, last rejection).
- 2143-2150 timing totals.
- 2151 `observe_progress("exit")` (collective).
- 2152-2159 tracked counters and timing re-read after the exit report.
- 2160-2162 `rebuilt_domain`, `result.cells.swap(working)`, `return std::move(result)`.

**`measure` 2765-2882 (118)**
- 2766-2774 census timer scope, scratch admission.
- 2775-2783 surface demand filter (requested walls) + `vote`.
- 2785-2787 id list, `census_started`, global batch count (`TimedAllreduce`).
- 2788-2880 batch loop: 2789-2792 collective deadline; 2793-2818 donor import with halving retry (2 votes per try, `import_source`); 2819-2820 admit; 2822-2825 frozen field; 2827-2834 surface planning; 2835-2876 per-cell measurement (2841-2854 construction, 2855-2864 improvement, 2865-2868 selection update, 2869-2875 counters); 2878 vote; 2879 advance.

**`emit_quality_census` 1770-1863 (94)**
- 1771-1779 elapsed values and first-window init.
- 1780-1814 scan of `selection.cells`/`cell_repairs`/`repair_users` into histograms (nested `miss_bin`, captures nothing).
- 1815-1817 timing.
- 1818-1859 three report prints (QUALITY, LOCATION, LADDER) with nested `histogram`.
- 1860-1862 window update.

---------------------------------------------------------------------------------------------------

## 4. STRAIGHT-LINE SETUP CODE (in order)

| # | Lines | Purpose | Collectives | Early exit | Unit |
|---|---|---|---|---|---|
| S1 | 1240-1241 | Save/restore global shape admission; set `ValidityOnly`. | - | - | Control |
| S2 | 1242-1253 | Phase, timing session (+reserved bytes into `options.caller_bytes`), phase flags, stage-observer bytes into `options.caller_bytes`. | (Session ctor: unknown) | - | Control |
| S3 | 1254-1272 | Clocks, atlas budget selection, `RestoreBudgetWindow`, atlas scratch, local time predicate. | - | - | Control |
| S4 | 1273-1300 | Parameter geometry staging: global mapped-mode agreement, storage bound, private parameter clone, private geometry, atlas integer-bit search (1290-1293), atlas reservation; elect. | 2 `TimedAllreduce`, `world.elect` (only if `mapped_high`) | **1299 `return rejected;` without `finish()`** | Commit (parameters) / Operators (geometry) / Control (budget) |
| S5 | 1301-1316 | Resolve `geometry`, mode flags, traffic baselines, `result`/`stats` init. | - | - | Control / Operators |
| S6 | 1317-1334 | `working`, `selection` (+flags 1319, 1330), census/queue containers, `field_identity` slot, `directory`, `source`. | (Directory ctor: unknown) | - | Census / Queues / Exchange / Ownership |
| S7 | 1335-1350 | `checkpoint` type and object, `retired_directory`. | - | - | Control |
| S8 | 1369, 1373-1374, 1388-1389, 1420-1428, 1556-1560, 1574, 1629-1632, 1766-1769, 2028-2029 | Declarations between lambdas (see section 2). 1559-1560: `diagnostic_rejections.reserve(64)` only if observer bytes suffice. 1629-1631: three `world.maximum` (observer flags). | 3 `world.maximum` | - | Control |
| S9 | 2164-2167 | Stage-observer storage admission. | `admit` | `finish` | Control |
| S10 | 2168-2296 | **Initialization block** under `Timer(stats.initialization_seconds)`: (a) 2173-2237 option agreement checks (curved, tolerance, ownership, composition/bands, rounds, work, window, time, geometry reservation, batch, mode, wall heights, region walls, height tolerance) with ~26 `TimedAllreduce`, work limit and atlas work allowance (2203-2210), geometry lease (2214); (b) 2238 vote; (c) 2239-2263 wall-height policy exchange and comparison (`world.exchange`, vote); (d) 2264-2266 initial admission; (e) 2267-2285 `field_identity`, `working=accepted`, reserves (`result.collapses/splits`, `initial`), `eligible_extensions`, id maxima, `initial` list; (f) 2286-2287 vote + empty-input check (`world.maximum`); (g) 2288-2295 global id maxima, `directory->Build`, `source.Build`. | many | 2297 `finish` | Control (a,b,d) / Exchange (c,g) / Census (e initial) / Commit (e ids) |
| S11 | 2298-2338 | Working-surface refresh for atlas, non-entity geometry: `RebuildWorkingSurface`, affected owners (nested `owns` 2314-2315), version bump, `directory->Publish` with coverage on rank 0 only. | `admit` x3, `vote` x4, `world.maximum`, `Publish` | 2300-2334 (7x `finish`) | Commit (+Exchange publish) |
| S12 | 2339-2368 | Region applicability (`global_original`, `prismatic`, analytic radial wall test, depth formula 2350-2352) + agreement. | 3 `TimedAllreduce` | 2368 `finish` | Operators / Control |
| S13 | 2369-2501 | **Region construction**: id maxima incl. donors, gather all cells to rank 0 (`world.exchange`), bounding box, donor import (`source.Import` if `global_original<=MaximumDonors`), `RegionFieldService::Run` callback (2407-2454: already-valid test, skin, kernel by surface integrals, `ConstructWallRegion`, decline rules, parameter edits, `PublishMetrics`), field/region stats, global work, vote, scatter replacement by `cell.id % world.size`, `directory->Publish`, rebuild `initial` and id maxima; resets `result.valid=false`, `result.reason.clear()` (2500). Nested lambdas: `tick` 2402-2403, FieldCounts callback 2406, `limits.rejected` 2429-2431, `limits.checkpoint` 2432-2433. | many | 2383-2495 (10x `finish`) | Operators (construction) + Exchange (gather/scatter/publish) + Commit (ids, parameter edits) |
| S14 | 2502-2515 | Lambdas `composed_sample`, `import_source` (between blocks). | - | - | - |
| S15 | 2516-2762 | **First-row layer initialization** (`!surface_target && wall_heights`): pending wall cells failing height (`needs_layer` 2523-2529), epoch loop while any pending: work and time checks (`timed_out` 2536-2541), `plan` 2545-2582 (batch by `ExscanSum`, global work, LookupBatch, FetchBatch, ReserveInteriors), proposal slots, `joint_candidate` 2588-2593, `construct` 2598-2705 (ranked donor import with halving, frozen field, per-candidate id reservation by ticket, `InsertFirstWallPoint` or `ConstructFirstLayer` over background points, best max length), `publish` 2707-2750 (parameter edits, coverage merge, route removals/additions by owner, `directory->Publish`, update pending), then rebuild `initial` and id maxima. | many | 2522-2757 (9x `finish`) | Operators + Trial + Commit + Exchange (one block containing all four) |
| S16 | 2763 | `MeasurementBatch`. | - | - | Census |
| S17 | 3273 | `directory->BeginConstructionPass()` if bidirectional. | (directory) | - | Exchange |
| S18 | 3274-3279 | Admission and `selection.SeedFrontier(working)`. | `admit`, `vote` | 3274, 3278 | Queues |
| S19 | 3280-3284 | Seeding `do { seed_next(); } while(surface_target && !world.maximum(construction_invalid) && world.maximum(seed_slot<initial.size()))`. | `seed_next`, 2 `world.maximum` (short-circuit) | 3283 | Census |

Totals in 1237-3284: 33 `return finish()` exits plus one `return rejected` (1299); 53 `catch` clauses;
in 2164-3284 alone: 60 `TimedAllreduce`, 22 `world.maximum`, 6 `world.exchange`, 40 `vote(`, 19 `admit(`,
3 `directory->Build`, 3 `directory->Publish`, 1 `ExscanSum`.

---------------------------------------------------------------------------------------------------

## 5. COUPLING

### 5.1 Lambdas called from more than one unit

| Lambda (home unit) | Called from |
|---|---|
| `vote` (Exchange) | every unit: Control (`finish`, `enter_phase`, `begin_length_generation`, checkpoints), Census (`measure`, `seed_next`, `generation_census`), Queues (`coalesce`, `service_frontier`), Ownership (`phase_ownership`), Control via `admit`, setup blocks, loop 80x. |
| `admit` (Control/Exchange) | Census, Queues, Ownership, Control, setup, loop 18x. |
| `bytes` / `real_bytes` (Control) | Exchange calls (`world.exchange` baselines, `Publish`), Census, Queues, Ownership, Control, loop 39x/2x. |
| `finish` (Control) | setup straight-line 33x, loop 108x (never from a lambda). |
| `observe_progress` (Control) | Control (`enter_phase`, `finish`), Ownership (`phase_ownership` x2), loop 12x. |
| `measure` (Census) | Census (`seed_next`), Control (`enter_phase`), loop 2x (Commit side, 5857, 6103). |
| `height_check` (Trial) | Census (`measure`), Control (`finish`), setup layer block (`needs_layer`), loop 2x. |
| `selection_bound` (Control) | Census (`measure`), Ownership, Control, loop 5x. |
| `scope_census_complete` (Census) | Control (`surface_ready`), Ownership, loop 2x. |
| `retire_directory` (Exchange) | Control (`finish`, `discard_checkpoint`), Ownership. |
| `length_direction` (Queues) | Control (`begin_length_generation`, `enter_phase`), loop 2x. |
| `update_fixed_metric_cells` / `reset_fixed_metric_cells` (Control diagnostic) | Commit (loop 5774, 6059) / Ownership. |
| `record_rejection` (Control report) | Trial (loop 3650, 5701, 6020). |
| `import_source` (Exchange) | Census (`measure`), Operators (layer block `construct`). |
| `phase_ownership` (Ownership) | Control (`enter_phase`), loop 2x. |
| `enter_phase` (Control) | Control (`rollback_checkpoint`), loop 7x. |

### 5.2 Cycles between units

1. **Control <-> Exchange**: `admit` (Control budget) calls `vote` (Exchange), and `vote` writes Control state
   (`result.reason`, `result.valid`, `result.surface_prepared`, `stats.stop`, `stats.atlas_*`,
   `stats.maximum_work_bytes`, `atlas_budget.time_exhausted`). `finish` (Control) calls `vote`.
2. **Exchange <- Trial mode**: `vote`/`elect_trial_failure` read `private_repair` (Trial, set in the loop at
   5588/5631); during private trials every `vote`/`admit` silently becomes rank-local. Exchange behaviour
   depends on Trial state.
3. **Control <-> Ownership**: `enter_phase` -> `phase_ownership` -> `observe_progress` (Control),
   `reset_fixed_metric_cells` (Control diagnostic), `admit`/`vote`.
4. **Control <-> Census**: `enter_phase`/`seed_next` -> `measure` -> `admit`/`vote` (Control/Exchange);
   `measure` writes `stats.stop` and `result.reason` (Control); `observe_progress` -> `refresh_surface_census`
   (Census) and reads `selection.Progress`.
5. **Control <-> Queues**: `begin_length_generation`/`enter_phase` (Control) -> `length_direction` (Queues),
   and `length_direction` writes `coarsening` and `construction_kind` (Control). `service_frontier` (Queues)
   writes `checkpoint.attempted` (Control) and reads `checkpoint.valid/changed/edges`.
6. **Census -> Trial -> Census**: `measure` (Census) calls `height_check` (Trial); Trial results feed back
   into `selection` (Census) through `selection.Add/RefreshQuality` in `measure`.
7. **Control (checkpoint) -> Control (phase) -> Ownership**: `rollback_checkpoint` -> `enter_phase` ->
   `phase_ownership` -> `observe_progress`.

### 5.3 State written by more than one unit

| State | Writers (unit: who) |
|---|---|
| `result` | Exchange: `vote`; Control: `finish`, `observe_progress`, `enter_phase`, checkpoints (`collapses/splits` resize); Census: `measure`, `seed_next`, `generation_census`; Queues: `coalesce`, `service_frontier`; Trial: `metric_trial` (`reason`); Ownership: `phase_ownership` (`reason`, through `VertexStarSizes`/`Repartition`/`Build`); Exchange calls (`directory->Build/Publish`, `MeasureGenerationProgress`) write `result.reason` by reference; setup and loop (85 writes). |
| `stats` | every unit (also held by reference in `Selection` and `pending_selection`, and `FieldAccounting`). |
| `selection` | Census (`measure`, `seed_next` via measure), Queues (`service_frontier`, `length_direction`, S18 `SeedFrontier`), Control (`begin_length_generation`, `enter_phase`, `rollback_checkpoint`), Ownership (`SwapState`), loop. |
| `working` | Exchange (`directory->Publish(working,...)` in S11, S13, S15 and the loop), Ownership (`phase_ownership` swap), Control (`rollback_checkpoint` swap, `finish` swap out), setup S10 (copy). |
| `directory` (the pointer) | Ownership (swap), Control (`rollback_checkpoint` swap). |
| `checkpoint` | Control (capture/discard/rollback), Queues (`service_frontier`: `attempted`), loop (Commit side 5803-6083). |
| `coarsening`, `construction_kind` | Queues (`length_direction`); `coarsening` is read by Census (`selection_bound`), Queues (`coalesce`), Ownership, Control. |
| `repairing` | Control (`enter_phase`), loop 3557. |
| `construction_claims` | Control (`enter_phase`, `begin_length_generation`), Ownership, loop (3445, 3454, 3553). |
| `initial`, `seed_slot` | Census (`seed_next`), Control (`enter_phase`), Ownership (swap), setup S10/S13/S15. |
| `retired_edges`, `campaign_edges` | Census (`measure` passes `&campaign_edges` into measurement), Control (`enter_phase`, `rollback_checkpoint`), Ownership (clear), loop Commit side (5811, 5871, 6073, 6111). |
| `inactive_blocked_*`, `length_blocks_dirty` | Queues (`length_direction`), Control (`enter_phase`), Ownership (`phase_ownership`), loop 3476. |
| `progress_at_window`, `attempted_at_window`, `deferred_at_window`, `phase_start_round` | Control (`enter_phase`, `begin_length_generation`), loop. |
| `atlas_budget` | Control (setup, `admit`), Exchange (`vote`: `time_exhausted`), loop 5633; and all geometry operators charge its work. |
| `fixed_metric_cells` | Control diagnostic, Commit (loop via `update_fixed_metric_cells`), Ownership (`reset_fixed_metric_cells`). |
| `*private_parameters` (contents) | Exchange/Commit (`Directory::Publish` edits through `geometry.Parameters()`), Control (`rollback_checkpoint` swap). |
| `retired_directory` | Exchange (`retire_directory`), Control (`finish` save/restore, `discard_checkpoint`), Ownership. |
| `last_rejection` | Operators (setup S13 2441/2444), Trial (loop 3649, 4598, 5699, 6018); read by Control. |
| `ready` | setup and loop (generic flag shared across units). |

The single largest coupling point is **`Selection`**: one object carries Census rows, all Queues,
lane/phase flags and repair epochs, and holds references to `stats`, `geometry` and `options.wall_heights`.
The second is **`vote`/`admit`**, which every unit calls and which writes Control's result/stop state.

---------------------------------------------------------------------------------------------------

## 6. IDENTITY RISKS for a mechanical extraction

1. **Reference members bound to locals.** `CavityMetricSource source(world,working,...)` (1334) stores
   `Owned& working`; `Selection` (1318, ctor at 276) stores `const PlanarReference& geometry`, `EngineStats& stats`
   and `const WallHeights& heights` (= `options.wall_heights`); `Directory` stores `World&` and the
   `BoundaryParameterState*` from `geometry.Parameters()`. `working` is only ever swapped (3097, 3261, 2161),
   never reassigned as an object. Moving `working`, `options`, `stats`/`result` or `geometry` into a struct
   requires the referencing objects to be built from the struct members, and the struct must never be copied
   or moved afterwards.
2. **`options` is a mutated copy.** `options.caller_bytes` grows at 1248, 1253 and 1286 before any `bytes()`
   call. Replacing the copy by a reference to the caller's options changes every admission. `composition`
   and `surface_target` are also by-value `std::function`s.
3. **Construction and destruction order of RAII locals.** Declaration order: `admission` (1240) ->
   `timer_session` (1244) -> `restore_budget` (1260) -> `atlas_geometry` (1266) -> `target_geometry_lease`
   (1267) -> ... -> `directory`, `source`, `checkpoint`. Destruction is reversed: the lease and atlas geometry
   are released **before** `RestoreBudgetWindow` restores the shared budget, and `RestoreAdmission` restores
   the global shape admission last, after `finish()` has built the return value. `timer_session` closes after
   `observe_progress("exit")`. Member order in any new struct must reproduce this.
   `Timer`/`CpuTimer`/`SU2NativeTiming3D::Scope` (2171, 2546, 2603, 2624, 2649, 2670, 2681, 2708, 2812, 2822,
   2884, 2503, 2505) cover exact regions; nested `Timer(..., enclosing_seconds, ...)` at 2812 subtracts donor
   time from `stats.seed_seconds` (passed by `seed_next` 2886/2899). Only timing values, but they are reported
   counters.
4. **Two-stage construction.** `geometry` (1301) is a reference that can be bound only after the parameter
   election (1298) and its early return (1299). Everything that references `geometry` (`selection`,
   `directory`, `source` via world only) is created after it. The early `return rejected;` at 1299 does **not**
   call `finish()` (no final vote, no stats, no `observe_progress("exit")`).
5. **Collective sequence and counters.** `stats.tracked_calls/tracked_bytes/tracked_collective_seconds`,
   the STAGE `collective_calls`/`election_calls` and `world.max_exchange_work_bytes` count every World
   collective. `TimedAllreduce` bypasses the World counters, `world.maximum/elect/exchange` do not. Adding,
   removing, merging or reordering a collective (or swapping `world.maximum` for `TimedAllreduce`) changes
   counters and can deadlock. Examples of conditional/short-circuit collectives: 1537
   (`certify_support && !world.maximum(...)`, one vote per receipt at 1552), 3284 (two `world.maximum` in
   a short-circuit `while` condition), 2927-2928, 1298 (only if `mapped_high`), 1888, 2009-2020,
   `vote` electing twice on quota refusal (1409, 1413), `finish` 1-4 votes depending on `result.valid`.
6. **`vote` is not always collective.** With `private_repair` true it is local (1391); `admit` inherits this.
   A unit split must keep `vote` reading the live `private_repair` flag.
7. **Memory accounting is capacity based.** `transfer_memory::Bytes(vector)` = `capacity()*sizeof(T)`
   (`CTransferMemory.hpp:55`), `MapBytes` = `size()*(sizeof(value_type)+4*sizeof(void*))` (103). `bytes()`
   includes `initial`, `eligible_extensions`, `batch_changed`, `result.splits/collapses`,
   `checkpoint.Bytes()`, `selection.Bytes()`, and others. Any change to `reserve`/`clear`/`swap` patterns
   (e.g. 2272-2275, 2491, 2754, 3101 `initial.swap(pending_initial)`), to element types, or to the argument
   list of `bytes()` changes `stats.maximum_work_bytes` and can flip memory admissions (results).
8. **Sorting with ties.** `service_frontier` sorts receipts by `(count, vertices, accepted)` only (1527-1529),
   not including `disposition`; with `std::sort` the order of equal keys depends on the input order. Keep the
   same container and input order. `coalesce` uses a total tuple order (1476-1485) and `std::find_if` takes
   the first owner (1488). Other sorts (1715, 2392, 2421) are on unique keys.
9. **Exceptions caught and voted.** 53 `catch` clauses map exceptions to `failure.Set(severity, gid, what())`;
   the elected message (severity/gid/rank) becomes `result.reason`. Moving a throwing call across a
   `try` boundary, or into a function that catches earlier, changes the severity (1, 2 or 3) or message.
   `consider_fixed_metric_row` swallows all `std::exception` (1679); `composed_sample` counts and rethrows
   with `catch(...)` (2508). The layer block uses `result.reason` as an out-parameter of
   `InsertFirstWallPoint`/`ConstructFirstLayer` (2651, 2676) and clears it on success (2688).
10. **Early `return finish()` paths.** 33 in this range; `finish` is never called from a lambda, only from
    straight-line code. Extracted setup functions must return a status and let the caller `return finish()`
    at the same points; `finish` moves `result` out (`return std::move(result)`) and swaps `working` into
    `result.cells`, so it may run only once.
11. **Shadowing.** `vote` declares `const auto original` (1413) shadowing parameter `original`;
    `service_frontier` parameter `accepted` (1502) shadows the input `accepted`; the loop shadows `original`
    (3804) and `accepted` (5262). Region/layer blocks reuse names `old`, `fresh`, `imported`, `pending`,
    `admitted` (2387 reused at 2482). Renaming to members must not capture the wrong object.
12. **Capture by value.** `atlas_budget.within_time` (1270) copies `started` and `options.maximum_seconds`
    at setup; it outlives nothing because `local_atlas_budget` is a local. All other function-level lambdas
    capture `[&]`; `miss_bin` captures nothing. No `mutable` lambdas in `Adapt`. No `static` or
    `thread_local` in `Adapt` (only a `static_assert` at 3235).
13. **Process-global state.** `SetShapeAdmission`/`GetShapeAdmission` (atomic in
    `Common/src/adaptation/CNativeMetric3D.cpp:183-184`) is set at 1241 and read by `height_check` (1582) and
    the loop (4129); `SU2NativeTiming3D::Session::Checkpoint` (1878) and the timing scope ledger; `std::cout`
    output order (timing reports only).
14. **Unordered containers.** None used in this range; `Owned` is `std::map`, `Selection` members are
    `std::map`/`std::set`. Iteration orders are deterministic and must stay on the same containers.
15. **Statement order inside one line.** Many lines hold several dependent statements, e.g. 3097
    (`retire_directory(directory->Statistics());working.swap(pending);directory.swap(pending_directory);selection.SwapState(pending_selection);`),
    3260-3262 (parameter swap before cell/directory swap), 2107-2110 (save, retire, copy, restore),
    3186-3187 (`measure` then `retired_edges.clear()` then `seed_slot`). Function arguments such as
    `transfer_memory::Add(bytes(),source.ResidentBytes(),directory->ResidentBytes(),...)` are unsequenced but
    side-effect free.
16. **Order-dependent state reads.** `enter_phase` reads `phase` (3162) before assigning it (3168) and
    `observe_progress`/`measure` read the new `phase`; `phase_ownership` reports "ownership_staged" before
    the swap; `finish` calls `observe_progress("exit")` and then re-reads tracked counters (2152-2154).
17. **Unused parameter.** `import_source`'s `requests` argument (2511) is not forwarded to
    `source.ImportCells`; `measure` computes `requests` (2809) only to pass it there. Forwarding it during the
    cut would change behaviour.
18. **ID reservation arithmetic.** Layer tickets `((epoch*world.size+rank)*candidate_batch+k)` with node stride
    5 and cell stride `MaximumReplacementCells` (2638-2646), and region ids from `maximum_node+1`
    (2435). `maximum_node/maximum_cell` are not written in the loop; changing when they are recomputed
    changes every fresh id.

---------------------------------------------------------------------------------------------------

## 7. PROPOSED EXTRACTION ORDER (this range only)

Each step compiles and must reproduce the same mesh and counters on the existing fixtures (including
`tracked_calls`, `collective_calls`, `election_calls`, `maximum_work_bytes`) at 1, 2 and 4 ranks.

1. **Hoist local types** (`RestoreAdmission`, `RestoreBudgetWindow`, `GenerationDebt`,
   `CoarseningCheckpoint`) to namespace scope. No behaviour change.
2. **Pure leaf lambdas to free functions with explicit arguments** (no state move yet): `selection_bound`,
   `geometry_trial`, `surface_merit`, `height_check`, `metric_trial`, `record_rejection`, `composed_sample`,
   `retire_directory`, `consider/update/reset_fixed_metric_cells`, `length_direction`. The lambdas stay as
   one-line forwarders so that call sites do not change.
3. **Group setup-only state into an `EngineSetup` object** for 1240-1300 (admission, timing session,
   budget window, atlas budget, private parameters/geometry, lease), preserving member order = current
   declaration order; keep the early `return rejected` path.
4. **Move long-lived state into per-unit structs** (Control, Census, Queues, Exchange, Ownership,
   Commit) owned by one `EngineContext` built after `geometry` is resolved. The lambdas keep `[&]` and
   refer to `context.unit.member`. Order of members must give the same construction/destruction order,
   and `selection`, `source`, `directory` must bind to members (risk 1). `bytes()` must list exactly the same
   containers (risk 7).
5. **Exchange core**: `elect_trial_failure`, `vote`, `admit`, `bytes`, `real_bytes`, `import_source` to
   member functions of one object that reads `private_repair` and writes Control's `result`/`stats.stop`
   (cycle 5.2-1/2 made explicit).
6. **Census**: `scope_census_complete`, `refresh_surface_census`, `generation_census`, `generation_debt`,
   `seed_next`, then `measure` split along 3.1 (admission, demand filter, import with retry, field build,
   per-cell measurement).
7. **Queues**: `coalesce` (proposal table / exchange / ordering / deduplication) and `service_frontier`
   (receipts / exchange / per-receipt service) keeping the per-receipt collective sequence.
8. **Control reports**: `emit_fixed_metric_drift`, `emit_quality_census`, `observe_progress` split along 3.1.
9. **`finish`** split along 3.1, keeping the vote order and the `observe_progress("exit")` position.
10. **Ownership**: `phase_ownership` split along 3.1 (gate, cost model, decision, staging, migration,
    commit swap, report).
11. **Control phases and checkpoints**: `begin_length_generation`, `surface_ready`, `enter_phase`,
    `discard/capture/rollback_checkpoint`.
12. **Setup blocks to functions returning a status** (caller does `return finish()`): S10 option agreement
    and initialization, S11 working-surface refresh, S12-S13 region construction, S15 first-row layer
    (`plan`, `construct` split by candidate kind, `publish`), S18-S19 seeding.

**Riskiest step: step 4** (moving state into structs): it touches every lambda at once and is where
reference binding (`source`->`working`, `Selection`->`stats`/`geometry`/`options.wall_heights`), RAII
destruction order and capacity-based memory accounting can silently change results. Of the function-level
steps, **step 10 (`phase_ownership`)** and the rollback part of step 11 are the riskiest: they swap
`working`, `directory`, `selection` and `initial` while other objects hold references to them, and contain
the longest collective sequence in this range.

---------------------------------------------------------------------------------------------------

## 8. Not determined from this range

* Whether `SU2NativeTiming3D::Session`, `Directory` or `CavityMetricSource` constructors/destructors make
  collectives (not checked; affects whether construction/destruction order is also a collective order).
* Whether `Selection` member functions touch `stats.rounds` beyond `Note` (seen at 327); the loop helper must
  confirm all `Selection` writes.
* Exact collective content of `directory->Begin/EndConstructionPass`, `LookupBatch`, `FetchBatch`,
  `ReserveInteriors`, `source.Import/ImportCells/Build` (assumed collective from their use under votes).
