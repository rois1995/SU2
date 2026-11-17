# B1 future binding inventory (read-only)

State: phase-A GQ is live (session 20498). This inventory maps the current build545 source and the frozen PART1 inventory for later B1 planning. It is not a new whole-item source-check, implementation proposal, or authorization to edit. B1 implementation is deferred until phase A passes in full. For the future member-function design, functions borrow `EngineContext&` and explicit parameters; aliases refer to the same active optional slots, with no copies. This note records dependency/wiring constraints only and does not change lambda bodies, decisions, callbacks, allocations, or collective order.

Source hashes:

- `Common/src/adaptation/CNativeEngine3D.cpp`: `dd97c5e2cb75789b1c35934a6fc3851ba297303a25510df5deceaba65efaf66f`
- `N4/section6_3_after_N11/inputs/CUT_INVENTORY_PART1.md`: `034275df32e2f4b7f4ef9bb2aef0bc16453f6d0249971b90fc42ba6a33b663cd`
- `Handbook/ADAPTATION_ADAPT_CUT_20261115.md`: `6fb9244409b2ac25566577dd40977c005244309a479047b6236aaedafd80b50a`

Line numbers below refer to the current hashed `CNativeEngine3D.cpp`, not the earlier PART1 line offsets.

## Leaf and diagnostic bindings

“Calls” lists source call-site lines; repeated calls on one line are noted. Return types for `auto` lambdas are the current deduced type. The capture entries name the exact current local aliases/state read or written. `context` in the future means the existing canonical objects and active slots, not copied payloads.

| Current definition, signature and inclusive bounds | C unit | Call sites | Current captures / local dependencies |
|---|---|---|---|
| `auto retire_directory = [&](const DirectoryStats& old)` → `void`; 1619–1636 (18) | Exchange | 2444, 2445, 3449, 3552 | Mutates canonical `retired_directory` (`DirectoryStats&`); no local callback. Input is `const DirectoryStats&`. |
| `auto selection_bound = [&](bool repairs)` → `size_t`; 1639–1641 (3) | Control (budget) | 2611, 3170, 3426, 3471, 3525, 3757, 3766, 3892, 6322, 6598 | Reads canonical `coarsening` (`bool&`) and `bidirectional` (`bool` Adapt argument); no local callback. Input is `bool` by value. |
| `auto geometry_trial = [&](bool valid)` → `bool`; 1881–1886 (6) | Trial | 4606, 6067, 6082, 6121, 6173, 6466 | Mutates `stats.geometry_trials` / `geometry_rejected`; calls `wall_counters.ObserveGeometry(valid)` on the canonical setup counter. Input is `bool` by value. |
| `auto surface_merit = [&](PatchMetric& metric, const Patch& patch)` → `void`; 1922–1932 (11) | Trial | 4682 (twice), 5035 | Reads `surface_target`, `options.wall_heights`, and canonical `geometry`; mutates the passed `PatchMetric&`; no local callback. `Patch` is const-reference. |
| `auto height_check = [&](const Patch& patch, std::string& reason, bool provisional=false, FirstHeightReport* measured=nullptr)` → `bool`; 1870–1880 (11) | Trial | 2419, 2878, 3208, 4604, 5099 | Reads `options.wall_heights`, `geometry`; updates `stats.height_checks`, `height_rejected`, `footprint_rejected`; calls `CheckFirstHeight` and `GetShapeAdmission`. The final two parameters have the shown defaults; patch and reason are references, report is optional pointer. |
| `auto metric_trial = [&](bool valid, MetricDiagnosis& diagnosis, MetricChange kind, bool surface_geometry_progress=false)` → `bool`; 1887–1921 (35) | Trial | 4677, 4684, 5095 | Reads `surface_target`, `bidirectional`, `repairing`, `selection.geometry_repairs`; may write `result.reason`, mutate `diagnosis` and metric rejection counts in `stats`; calls `wall_counters.MetricTrial`. It has no local lambda dependency. Default is `false`; `diagnosis` is mutable reference. |
| `auto record_rejection = [&](Feature feature, const Patch* old, const MetricDiagnosis* metric, const std::string& reason)` → `void`; 1851–1863 (13) | Control (report), called from Trial | 4043, 6222, 6554 | Mutates `diagnostic_rejections` and `diagnostic_sequence`; reads `stats.rounds`; no local callback. Feature by value; old/metric are nullable const pointers; reason is const-reference. |
| `auto composed_sample = [&](Point point, Tensor sensor)` → `Tensor`; 2852–2859 (8) | Operators (target query) | 5643, 5644, 5915 | Reads callable `composition`, `options.timings`; updates `stats.field_query_seconds`, `field_composition_seconds`, `field_requests`, `field_misses`, `field_compositions`, `field_failures`. Calls the user-supplied `composition(point,sensor)` when present and `ValidateTensor`; catch-all increments failure count and rethrows. Inputs are values. |
| `auto length_direction = [&](bool collapse)` → `void`; 3251–3260 (10) | Queues (writes Control direction fields) | 3478, 3494, 3512, 3879, 3922 | Mutates `selection.blocked_edges/blocked_links/coarsening`, canonical `coarsening` and `construction_kind`, `inactive_blocked_edges/links`, and `length_blocks_dirty`; no local callback. Input is `bool` by value. |
| `auto consider_fixed_metric_row = [&](const OwnedCell& row)` → `void`; 1979–1994 (16) | Control (diagnostic) | 1998, 2002 | Reads `fixed_metric_available`, `fixed_metric_points`; updates `fixed_metric_cells`; calls `source.SampleContainedCurrentMetric`. It catches and swallows `std::exception` as the existing diagnostic behavior. `row` is const-reference. |
| `auto update_fixed_metric_cells = [&](const std::vector<Dependency>& removed, const std::vector<OwnedCell>& added)` → `void`; 1995–1999 (5) | Control (diagnostic), called by Commit | 6301, 6593 | Mutates canonical `fixed_metric_cells`; reads `fixed_metric_available/points`; invokes `consider_fixed_metric_row` for added cells. Both vector inputs are const-references. |
| `auto reset_fixed_metric_cells = [&]()` → `void`; 2000–2003 (4) | Control (diagnostic), called by Ownership | 3450 | Resets canonical `fixed_metric_cells`; reads `working`, `fixed_metric_available/points`; invokes `consider_fixed_metric_row` for each working cell. No explicit arguments. |
| `auto emit_fixed_metric_drift = [&](const char* stage)` → `void`; 2004–2078 (75) | Control (report) | 2214 | Reads/writes all fixed-metric state, `working`, `selection.cells`, `source`, `composition`, `stats`, `options`, `world`, and timing state; prints only when timing output is enabled. It calls the existing `bytes()` lambda and `source.SampleContainedCurrentMetric`, plus `world.elect` / `world.exchange` directly. Preserve its collectives, exception handling, and output sequence exactly. `stage` is a C-string pointer. |

## Captured alias types visible from current source

These are types/qualifiers of current captured aliases where the source spells them directly. They are not proposed API signatures; explicit arguments should preserve the same object identity and constness required by the current body.

| Alias | Current type / qualification |
|---|---|
| `retired_directory` | `DirectoryStats&` in `context.exchange.retired_directory` |
| `coarsening`, `construction_kind` | `bool&`, `MetricChange&` in `context.control`; `selection` is `Selection&` in `context.census` |
| `bidirectional` | Adapt input is `bool` by value; `EngineContext` stores a reference to that active local |
| `stats`, `result` | `EngineStats&` (`result.stats`) and `EngineResult&` |
| `wall_counters` | `wall_diagnostics::Counters&` from the live `EngineSetup` optional |
| `geometry` | `const PlanarReference&` selected from the active atlas/private/accepted geometry |
| `surface_target` | `BoundaryMetric` Adapt input by value; context holds a reference to the active local |
| `options`, `composition` | `EngineOptions` and `MetricComposition` Adapt inputs by value; composition is `std::function<Tensor(Point, Tensor)>` |
| `diagnostic_rejections`, `diagnostic_sequence` | `std::vector<RejectionDiagnostic>&`, `uint64_t&` |
| `working` | `Owned&` in `context.ownership`; `Owned` is `std::map<Id, OwnedCell>` |
| `source` | `CavityMetricSource&` in `context.exchange`; its construction binds the same `World&` and `Owned& working` |
| `fixed_metric_points`, `fixed_metric_cells` | `std::vector<FixedMetricPoint>&`, `std::array<Id,16>&` |
| `fixed_metric_initialized`, `fixed_metric_available` | `bool&` |

## Current call-site inventory

The table above enumerates all matching current call sites in the active `Adapt` body. The frozen PART1 inventory records downstream cross-unit roles: `retire_directory` is used by Control/Ownership; `selection_bound` by Census, Ownership, Control, and the loop; `length_direction` by Control and the loop; `height_check` by Census, Control, setup, and loop; `record_rejection` is called by Trial; fixed-cell update is called by Commit and reset by Ownership. `composed_sample` is the Operators target-query helper. This is retained as caller-routing context only; later callers are to be changed directly, with no forwarding lambda retained.

`emit_fixed_metric_drift` is not a pure leaf: it directly performs rank elections/exchanges and calls `bytes()`. `consider_fixed_metric_row` has an exception-swallowing boundary; `composed_sample` counts and rethrows failures. The diagnostic split must preserve those existing boundaries and the original caller location/order. The inventory does not prescribe a new API for them.

## Invariants to keep visible during later wiring

- The proposed functions borrow the same active `EngineContext` and explicit references/values; aliases must point into the existing context slots. No copied `Selection`, `Owned`, `DirectoryStats`, diagnostics vector, or metric-source state.
- Preserve the current default arguments on `height_check` and `metric_trial`, and all call arguments/order.
- Preserve `geometry_trial` / `metric_trial` wall-counter calls and report mutations exactly.
- Fixed-metric diagnostics are observational, but their current internal `world.elect` / `world.exchange`, `bytes()` callback, and exception paths remain part of the existing implementation and must retain sequence and scope.
- No extra collective, allocation, retained callback, `std::function`, or decision is introduced by this inventory.
