# Native 2D and 3D remeshing: static re-assessment

- Code assessed: commit `b449b5b800` on `codex/native-3d-bl` (2026-10-14 14:29). This branch continues `codex/native-3d-transport-accounting`: the tip of that branch (`23197eba17`) is an ancestor.
- Previous assessment: `NATIVE_REMESHING_STATIC_ASSESSMENT.md`, at `7746fd53fa`. Since then: 51 commits, about 7700 lines added in `Common`, `SU2_CFD` and `UnitTests`. Almost all of them are 3D. 2D changed by 2 lines.
- Method: reading only, from a frozen `git archive` of `b449b5b800`. Nothing was built or run. The live worktree had uncommitted edits in `CNativeBoundary3D.*` (96 lines); they are not assessed.
- Timing, memory and frequency figures are counts or estimates from the code. None is measured.
- Not read: `CReaderSlices`, the `CSolutionTransfer` family, `integration_evidence/`. Conservation of the solution transfer is not assessed.

Structure:
- Part I: merged verdict, written for this file. Duplicate findings of the two reviewers are merged here.
- Part II: full 3D core report (engine, distributed layer, cavity, predicates, field, metric, import, driver).
- Part III: full 3D boundary / boundary-layer report, plus the 2D re-check.
- Appendix A and B: the previous full 3D and 2D reports, for the text of every old finding that is still open.

Finding labels: old findings keep their old IDs (`3D-C2`, `2D-C1`). New ones are `N3-*` (core), `N3B-*` (boundary and BL) and `N2-*` (2D).

---

# Part I — Verdict

## I.1 Summary

| Axis | 2D | 3D |
| --- | --- | --- |
| Correctness of accepted meshes | Unchanged. Good. | Still good. New boundary operators are exception-safe; exact predicates extended (`ConformingDisjoint`, layered trace). No overflow, UB or deadlock found. |
| Capability | Unchanged. Fixed work budget (2D-C1). | Much better. The 512-triangle, one-sphere skin cap is gone. The 512-round cap is gone. Curved charts, a BL policy and first-height walls exist. |
| Robustness as a remesher | Any incomplete result still ends the run. | Worse in practice: every new stop (work budget, time budget, stagnation, deadline veto) also ends the run. |
| Performance | Unchanged. Does not scale to 1e6 cells. | Curved and BL modes add O(N) work for each accepted edit and serialize all ranks. Worse than the planar path. |
| MPI readiness | Unchanged. | New time and work decisions are collective (no divergence, no deadlock). But curved, BL and first-row work run one proposal at a time for the whole job, so more ranks do not help. Several new memory admissions shrink with the rank count. |

Counts after merging duplicates:

| | Critical | High | Medium | Low |
| --- | --- | --- | --- | --- |
| 3D old, still open or partly fixed | 1 | 10 | 9 | 10 |
| 3D new (core + boundary/BL, merged) | 3 | 10 | 13 | 22 |
| 2D old, still open (all) | 1 | 5 | 13 | 12 |
| 2D new | 0 | 0 | 0 | 1 |

Fixed since last time: 3D-C1 (skin cap and closed-sphere demand), 3D-C3 (512-round cap), 3D-M1 (per-rank publish budget). The two reviewers disagree on 3D-C1: the core reviewer says Fixed, the boundary reviewer says Partly fixed. Both are right: the cap is gone, but the 1 MiB reference sidecar (3D-M6, now High) blocks output above about 4000 to 5000 facets. In this file, C1 is Fixed and its residual is counted under M6.

## I.2 Blocking findings

Each item below was re-read in the frozen code for this merged report.

1. **Every stop still ends the CFD run (3D-C2, still open, Critical).** `ReplaceMesh` turns `INCOMPLETE_*` into `CollectiveFailure`, which calls `SU2_MPI::Error`. The new `beforeAcceptance` hook does the same (`CSinglezoneDriver.cpp:1703-1713, 1824-1828`). So the new work budget, time budget and stagnation stops are all fatal. The messages say "accepted mesh and solution retained", but the job exits. The second deadline check runs after the full geometry rebuild and the transfer, so that work is lost too.
2. **A curved edit costs O(N) (N3-P1 = N3B-P1, Critical, Confirmed).** For each round with an approved curved proposal, `Clearance` (`CNativeEngine3D.cpp:71-118`, called at 1801-1806) copies every working cell into a vector and builds a distributed `DonorImport` index over the whole mesh. One event is therefore O(rounds × N).
3. **First-row BL initialization is serial over the whole job (N3-M2 part = N3B-M1, Critical, Confirmed).** `CNativeEngine3D.cpp:648-660`: each global round elects the lowest rank with pending work and handles one wall cell. With 1e5 to 1e6 wall faces this needs that many collective rounds, whatever the rank count.
4. **Curved and BL edits are one proposal per round for all ranks (N3-M2, High, Confirmed).** `CNativeEngine3D.cpp:985-999` keeps only one owner per round in curved or composed runs. The code comment calls it a "shortcut". Throughput does not grow with P.
5. **The boundary cannot be large.** The reference admission (`CNativeImport3D.cpp:528-531`) adds P copies of the facet list, so the boundary is capped at about 2^31 / (4096 + 336·P) triangles: about 520k at P=1, 84k at P=64, 24k at P=256 (N3-M1, formula confirmed). The sidecar written at every mesh output is limited to 1 MiB (`CNativeReferenceIO3D.hpp:11`), about 4000 to 5000 facets (3D-M6, raised to High). Merging and analytic chart completeness are still capped at 512 facets (N3-C4 = N3B-C3).
6. **Complexity quadrature aborts near ridges (N3B-C1, High, Confirmed).** The 3D quadrature stops at depth 32 or 16384 visits and the stop is fatal. Cells at convex ridges or corners need about 3·log2(L/h0) levels, so `ComputeMetric` aborts for L/h0 above about 2e3.
7. **Coarsening can still reject a valid mesh (3D-C4, still open, High).** If coarsening runs out of work or time, the refined mesh is rejected too. Coarsening gets only the budget left by refinement (`CNativeRemesher3D.cpp:340`).
8. **2D is unchanged.** All Critical and High 2D findings are still open: 2D-C1, C2, P1, P2, P3, M1.

## I.3 Performance (new)

- **Donor index admission (N3-P2, High).** `DonorImport::Build` admits 4096 bytes × cells × OpenMP threads against the fixed 2 GiB (`CNativeDonor3D.cpp:138-141`). About 2e5 to 5e5 cells per rank on one thread, divided by the thread count.
- **BL edge integration (N3-P3, N3-P4, High, Plausible).** No edge length is reused in BL mode. One edge can need up to 65536 breakpoints and 65536 BL-metric evaluations. One rejected curved or BL proposal can cost 1e3 to 1e4 collectives.
- **Quadrature (N3B-P2, N3B-P3, High).** Every cell's quadrature is rebuilt in every complexity trial (`CSolver.cpp:3100-3106`), although most rules do not use the sensor. Quadrature storage has a fatal 2 GiB cap, about 1e4 BL cells per rank.
- **Wall search (N3B-P4, High).** `EdgeBreakpoints` cost grows with the wall faces near the chord times about 45 levels, plus a full `Evaluate` per candidate root.
- **Replicated data (N3B-M2, High).** Reference and BL data are admitted at 4096 + 8192 bytes per facet on every rank, against the 2 GiB default ceiling.
- Medium items: `Evaluate` builds full tensors before the weight test (N3B-P5); composed edge integration costs hundreds of compositions per edge (N3B-P6); reference construction has O(V·T) and O(B·V) loops on every rank (N3B-P7); joint placement search has a large constant (N3B-P8); plus N3-P5 to N3-P7.

## I.4 MPI readiness (new)

Good:
- The new time and work decisions use MAX/MIN reductions or an election. No path lets ranks diverge on the wall clock. No deadlock was found on any new path.
- New ID strides, overflow guards and collective ordering in the new trial loops are sound.
- The `CPoint` fix for empty ranks is correct.

Not ready:
- Curved, BL and first-row work are serial across ranks (I.2.3, I.2.4).
- Memory admissions that grow with P (I.2.5, N3B-M2).
- The time limit is checked only at round and loop boundaries. The seed, `Clearance`, the builds, the reader return and the CFD rebuild can overrun it without bound (N3-M3, Medium).
- Old 3D-M2 (dense all-to-all), M3 (lowest rank wins in planar runs), M4 (no load balancing) and M5 (hash directory) are still open.
- Acceptance now depends on timing, so two runs of the same case can give different results (Low).

## I.5 Correctness (new, Medium)

- One proposal's numerical failure (split point or BL integral not converging) rejects the whole event (N3-C2).
- The `Clearance` 256-cell import cap also rejects the whole event (N3-C3).
- 3D-C5 (64-cell star) is fixed only in the curved repair, joint and strip paths. The batched planar path still pushes an oversized star and rejects the event (`CNativeEngine3D.cpp:197, 970-972`).
- First-height check uses a fixed 1e-8 relative tolerance, which fails when h < about 4e-8·|x_wall| (N3B-C2).
- With BL active, boundary splits use metric-balanced chord points that the exact on-edge checks reject (N3B-C4).
- The 65536-cut cap in `EdgeBreakpoints` counts invalid and duplicate roots before filtering (N3B-C5).
- Analytic charts (sphere, bump, cylinder, disk) can come only from a hand-written `.native_ref`. Faceted oblique or curved walls still need exact on-edge points (3D-C6, partly fixed).
- The project documents say BL and curved support are "pending", but the code enables both (3D-C14, still open).
- 2D: the 2D quadrature calls `SU2_MPI::Error` at depth 16, outside the collective failure wrapper (N2-1, Low).

## I.6 Recommended order of work

1. Make every remesher stop non-fatal: keep the previous mesh and solution and continue, or accept the best valid mesh with a warning. Do the deadline check before the rebuild, not after. This is still the first fix, and it is now more urgent.
2. Remove the O(N) `Clearance` per edit: check collisions with a local index that is updated incrementally, not rebuilt.
3. Make first-row initialization and curved/BL edits concurrent: let each rank handle its own wall cells and independent proposals in the same round.
4. Remove the size caps that are fatal or grow with P: sidecar 1 MiB, reference admission × P, 512-facet chart merge, quadrature depth and storage caps, donor index × threads.
5. Make the quadrature incremental across complexity trials.
6. Then the old planar items: many cavities per rank per round, neighbour exchange instead of all-to-all, flat storage instead of `std::map`.
7. Test at 16, 64 and 256 ranks with at least 1e5 wall faces before more qualification at 1, 2 and 4 ranks.

## I.7 What only a run can settle

- Real cost of `Clearance`, of BL edge integration and of the first-row phase.
- How often N3-C2, N3-C3, 3D-C5, 3D-C8 and N3B-C4 trigger.
- `omp_get_max_threads()` in production builds (it multiplies the donor admission).
- Whether transition cells pass the engine's metric gates.
- How an invalid analytic chart is handled downstream.

---

# Part II — 3D core re-assessment (full report)


Code read: the frozen copy `snap_b449` of commit b449b5b800 (branch codex/native-3d-bl). The live worktree was used only for `git log/diff/show` between the two commits.
Nothing was built or run, and no repository file was changed. Every statement below comes from reading the code.

**Read in full:** CNativeEngine3D (hpp, cpp), CNativeDistributed3D (hpp, cpp), CNativeRemesher3D (hpp, cpp), CNativeCavity3D, CNativeTopology3D, CNativeMetric3D, CNativeField3D, CNativeImport3D, CNativeDonor3D.cpp (Build/Import), CNativeMesh3D.hpp, CNativeRuntimeProfile3D.hpp, CRemesher.hpp, CNativeReferenceIO3D.cpp, the predicate diff (ConformingDisjoint, layered TraceSegment) and the integer kernel of CNativePredicates3D.cpp, the ADAP_NATIVE_* options in CConfig, and the native 3D parts of CSinglezoneDriver.cpp (ComputeMetric, MakeRemesher, ReplaceMesh, the call sites).
**Read only where a call path needs it** (second reviewer's scope): CNativeBoundary3D.cpp (CheckFirstHeight, ConstructFirstLayer, ReconstructSplitPatch, the PlanarReference coplanar/analytic chart builder, ConstructionBytes), CBoundaryLayerMetric.cpp (EdgeBreakpoints, ApplyPoint), CDistributedSearch.cpp (CollectiveFailure).
**Unchanged since the baseline** (checked with `git diff --stat`): CNativeDonor3D, CNativePartition3D, CNativeTopology3D, CNativeRuntimeProfile3D.hpp, CNativeDistributed2D.hpp (the `World` transport), CReaderSlices.

Severity and confidence follow the baseline: Critical = blocks use beyond small meshes or aborts the run; High = wrong outcome or loss of the event in realistic cases, or a cost that prevents scaling; Medium = a significant limit or waste; Low = minor. "Confirmed" means I followed the code path through its callers. "Plausible" means the mechanism is in the code but its frequency or cost is my estimate.

---

### 1. What changed (short)

- **Round cap replaced by a global work budget.** `EngineOptions::maximum_rounds` now defaults to 0, which means no round limit (CNativeEngine3D.hpp:14-17, CNativeEngine3D.cpp:815). Termination comes from `work_limit` = ADAP_NATIVE_MAX_WORK, or by default max(4096, 64 x global input cells) (cpp:569; CNativeRemesher3D.cpp:114, 236-237). There is also a stagnation window of 64 rounds (cpp:875-899) and an optional elapsed-time limit, ADAP_NATIVE_MAX_TIME, checked collectively at round and loop boundaries (cpp:650, 867, 1524, 1618).
- **Deadline hook.** `CRemeshResult::beforeAcceptance` (CRemesher.hpp:102) is set when a time limit exists (CNativeRemesher3D.cpp:408-413). ReplaceMesh evaluates it before the rebuild and again after the rebuild/transfer, each time followed by `CollectiveFailure` (CSinglezoneDriver.cpp:1709-1713, 1824-1828).
- **Batched planar transactions.** Up to 8 locally disjoint candidates per rank per round, through LookupBatch/FetchBatch/ReserveBatch (cpp:925-1019; CNativeDistributed3D.cpp:256-627). Only for planar runs without composition and without repair.
- **Curved surfaces.** Analytic charts (sphere, bump, cylinder, disk) come from the sidecar (versions 2 and 3). Synthetic exterior "extension" donors cover the region outside curved walls (CNativeImport3D.cpp:126-303), and a layered segment trace gives the original donors priority over the extension donors (CNativePredicates3D.cpp:428-466). Projected splits, a geometric-error queue (cpp:129, 334-373), a geometric-progress gate (cpp:1065-1078), and a nonlocal collision check `Clearance` with exact `ConformingDisjoint` (cpp:71-118, 1801-1806; Predicates:318-381).
- **Boundary-layer policy.** ADAP_BL is no longer rejected (CNativeRemesher3D.cpp:93-96). A composition factory from the driver provides the BL metric and the first heights (CSinglezoneDriver.cpp:300-325). First-row initialization (cpp:630-750), the first-height gate (cpp:474-480, 497-503), and the joint strip and joint wall reconstruction (cpp:1339-1698) were added.
- **Domain topology.** The global skin check is now `ValidateDomainSurfaceTopology`, without a size cap and allowing any number of components and any genus (CNativeCavity3D.cpp:135-157; CNativeImport3D.cpp:552; CNativeReferenceIO3D.cpp:122).
- **Publication.** `Publish` is bounded by bytes, not by the size of one cavity (CNativeDistributed3D.cpp:739-741).

---

### 2. Status of the old findings

| ID | Old sev. | Status | Evidence in b449 |
|---|---|---|---|
| C1 512-triangle / single-sphere skin | Critical | **Fixed** (superseded by N3-M1) | Import3D.cpp:550-553 and ReferenceIO3D.cpp:122 call `ValidateDomainSurfaceTopology` (Cavity3D.cpp:135-157). It checks manifoldness and orientation only; `Surface(...,cavity=false)` skips the Euler and connectivity test (Cavity3D.cpp:78). The 512 cap survives only in local chart validation (see N3-C4). The binding limit is now the byte model of N3-M1. |
| C2 rejected event aborts the run | Critical | **Still open** | ReplaceMesh: status != COMPLETE -> `CollectiveFailure` -> `SU2_MPI::Error` (CSinglezoneDriver.cpp:1703-1707; CDistributedSearch.cpp:460-463). The new budget and deadline exits feed the same path (N3-C1). The remesher still prints "accepted CFD mesh/solution retained" (CNativeRemesher3D.cpp:469). |
| C3 512 rounds, 1 attempt/rank/round | Critical | **Fixed** (as a round cap) | `maximum_rounds=0` by default (Engine.hpp:14); the loop runs `for (round=0; !options.maximum_rounds || ...)` (cpp:815); work-bounded (cpp:569, 952, 1002). When the budget runs out, the event is still rejected (C2). |
| C4 valid mesh rejected when coarsening is unfinished | High | **Still open** | Coarsening is complete only with an empty queue (cpp:831-832, 842, 851). WorkBudget and TimeBudget exits return `valid=false` (cpp:952-955, 1002-1005, 867-870). The final check is unchanged (cpp:1916-1921). The remesher takes `coarse.valid` as the event result (CNativeRemesher3D.cpp:350-353, 392). The coarsening budget is only what refinement left (N3-C6). |
| C5 star >64 cells rejects the event | High | **Partly fixed** | Oversized stars no longer reject the event in the curved repair path (cpp:1025-1041), the strip/wall lookup (cpp:1383-1385) and joint growth (cpp:1659-1660). They still reject it in the main planar paths: `Lookup` throws on status 3 (Distributed3D.cpp:302, 312, 345-346) and then cpp:1046-1047 returns. `Selection::Batch` deliberately pushes the oversized candidate (cpp:197), and an invalid batch candidate fails the vote (cpp:970-972). `FilterCollapse` still does not test the star size (cpp:243-273). The BL first-row cones raise wall-vertex valence, so this case becomes more likely. |
| C6 boundary points exactly on edge/plane; no curved surfaces | High | **Partly fixed** | Analytic sphere/bump/cylinder/disk charts with projection exist, but only when the sidecar declares them (ReferenceIO3D.cpp:31-33, 113-121). Planar splits remain exact (rounded midpoint, cpp:1118-1121; "planar points never snap", Boundary3D.hpp SplitEdge doc). General faceted curved surfaces and oblique planes with non-dyadic coordinates keep the old limits. No up-front detection. |
| C7 strict shape gate on every intermediate edit | High | **Still open** | `require_shape=true` for fresh cells (Metric3D.cpp:97-100, 248-249; cpp:1276, 1317). The thresholds are unchanged at 0.20 and 0.05 (Mesh3D.hpp:87-88). The joint strip/wall operators build anisotropic children directly, which only reduces the effect for the BL case. |
| C8 256-donor cap per cell | High | **Still open** | MaximumDonors=256 (Field3D.hpp:18; Field3D.cpp:36; Donor3D.cpp:344, 365). The one-cell fallback is unchanged and an event rejection remains possible (cpp:769-776, 1265). Extension donors (3 per analytic boundary face) add to the per-cell union near curved walls (Plausible aggravation). |
| C9 boundary coarsening only on convex coplanar patches | Medium | **Still open** (wider) | The convex/disk rule is unchanged. A coplanar component above 512 triangles now also stays raw (N3-C4). |
| C10 narrow operator set | Medium | **Partly fixed** | New: metric-mean moves (Metric3D.cpp:15-41; cpp:1184-1192, 1711-1753), a collapse inside repair (cpp:1787-1788), SplitSurfaceStrip, ReconstructSplitPatch, ConstructFirstLayer. ADAP_BL is accepted. Still missing: Delaunay/in-sphere, 2-3/3-2 swaps, smoothing as an optimisation pass, tensor repair (a non-SPD tensor is still fatal at import, Import3D.cpp:399-404). ADAP_SURFACE=NO is still rejected (Remesher3D.cpp:97-98). |
| C11 rounded centroid outside the source | Low | **Still open** | `Query` throws "outside" (Field3D.cpp:320). The seed turns this into a rejection (cpp:798-800 -> `Selection::Add` throws, cpp:330-331). Extension donors mitigate it only near analytic walls. |
| C12 UndoEdgeSplit dead | Low | **Still open** | Boundary3D.cpp:1327; there is no production caller. |
| C13 -ffast-math on the filters | Low | **Still open** | meson.build:101. The new `ConformingDisjoint` is integer-only, so it is not affected. |
| C14 stale documentation | Low | **Still open** (new staleness) | NATIVE_ADAPTATION.md:34 still says "no ADAP_BL"; lines 212-213 say BL construction and curved projection are pending. ADAP_NATIVE_MAX_WORK/TIME are not documented there. |
| P1 one cavity/rank/round, ~170 collectives | High | **Partly fixed** (planar), **worse** (curved/BL) | Planar runs batch up to 8 candidates per rank (cpp:925-1019), but each slot still runs its own full transaction (reconstruction vote, import, 3 routing exchanges, Publish, re-measure). Curved and BL runs are globally serial (N3-M2). |
| P2 repeated full-mesh passes | High | **Partly fixed** | `WorkingWeights` and the parent `DonorImport::Build` now run only when repartitioning (Remesher3D.cpp:248-256; the baseline ran them unconditionally). Still present: a seed in every pass (cpp:812), `working=accepted` copy (cpp:604), source and directory builds per pass (cpp:623, 626), the ReaderSlices source build (Import3D.cpp:724). New: a full-mesh rebuild per curved edit (N3-P1). |
| P3 exact rational tracing per edge | High | **Still open** | TraceSegment unchanged in principle (Field3D.cpp:199-202). The layered variant adds work (N3-P8). |
| P4 edges integrated 2-3 times per edit | Medium | **Still open** (worse with BL) | Old cells are measured with `&measured.edges` reuse only (Metric3D.cpp:251). Composed fields disable reuse completely (Metric3D.cpp:111, `target.SensorOnly()`), see N3-P3. |
| P5 node containers, local rebuilds | Medium | **Still open** | Selection maps and sets (cpp:129, 147-154); full `Star` copies in Prepare (Distributed3D.cpp:168-171); new linear geometry-queue scan (N3-P7). |
| P6 memory footprint, fixed 2 GiB ceiling | Medium | **Still open** | `memory_ceiling = 2 GiB` (Engine.hpp:14); the driver passes `EngineOptions{}` (CSinglezoneDriver.cpp:322). There is no config option. See also N3-P2 and N3-M1. |
| P7 metric queries in batches of 16 cells | Medium | **Still open** | cpp:755, 1250, 1723; Import3D.cpp:685, 773. Streamed curved/BL proposals now go through these batches for every trial (N3-P4). |
| P8 Repartition builds a directory for validation only | Low | **Still open** | Partition3D.cpp:147 (file unchanged). |
| P9 replicated map-based planar reference | Low | **Still open** (now relevant) | Lifting C1 makes it bind (N3-M1). Bump charts add per-group `PlanarReference` copies (Boundary3D.cpp:589-607). |
| M1 per-rank sum against single-cavity budget | High | **Fixed** | `Publish` is admitted by bytes (Distributed3D.cpp:734-741). The `retired_edges > 6*64` check is gone (cpp:1883-1885). |
| M2 dense-in-P exchanges | High | **Still open** | `World::exchange` is unchanged (CNativeDistributed2D.hpp). New code adds more P-sized bucket arrays per call (cpp:586, 717-718, 1817-1819). |
| M3 lowest-rank priority | Medium | **Still open** (partly improved) | `Reserve` uses min(caller) (Distributed3D.cpp:663-669); `ReserveBatch` uses min(caller,ticket) (604-612). Curved serialization chooses the lowest active rank (cpp:997-998). Only planar BL chooses by global metric priority (cpp:987-996). |
| M4 no load balancing during a pass | Medium | **Still open** | New cells go to the owner of the parent (cpp:1849, 725). |
| M5 hash directory makes every operation remote | Medium | **Still open** | Distributed3D.cpp:89-97. |
| M6 replicated reference, 1 MiB sidecar | Medium | **Still open, now binding (High)** | `ReferenceFileLimit3D = 1 MiB` (ReferenceIO3D.hpp:11; .cpp:72, 79, 145) is about 4000 facets at ~260 B per text line. Without C1 this cap now bites: restart requires the sidecar (Remesher3D.cpp:190-191) and mesh output writes it (CSinglezoneDriver.cpp:1664), so both are fatal above about 4000 boundary triangles. The replication cost is N3-M1. |
| M7 rank-count dependence | Low | **Still open** | Tokens contain rank and round (cpp:1080-1084). Curved serialization processes in rank order. |
| M8 partial transport accounting | Low | **Still open** | Many new direct `CPassiveComm::Allreduce` calls are not counted (cpp:543-577: 22 at initialization; 946, 951, 990-1001 per round; 1246, 1721). |
| M9 unguarded exceptions abort | Low | **Still open** | Remesher3D.cpp:363-364 (`SU2_MPI::Error`). New code outside `try`, for example `geometry.Get(f.facet)` in the strip setup (cpp:1345-1348, 1642-1643). It aborts; I found no hang. |
| M10 master gathers boundary faces | Low | **Still open** | Import3D.cpp:781 -> `CReaderSlices::FromDistributed` (file unchanged; not re-read). |

Totals over the 33 old findings: **Fixed 3** (C1, C3, M1), **Partly fixed 5** (C5, C6, C10, P1, P2), **Still open 25**, **No longer applies 0**.

---

### 3. New correctness findings

#### N3-C1 — High, Confirmed: budget, time and deadline exits all end the CFD run
Every new stop condition rejects the event: WorkBudget (cpp:649, 952-955, 1002-1005, 1522, 1614-1616), TimeBudget (cpp:650-652, 867-870, 1524-1526, 1618-1620), OperatorStagnation (cpp:894-896, 902-905), and the remesher deadlines (Remesher3D.cpp:316, 343-345, 405). The rejection reaches ReplaceMesh as INCOMPLETE_QUALITY, and `CollectiveFailure` calls `SU2_MPI::Error` there (CSinglezoneDriver.cpp:1703-1707).
The `beforeAcceptance` veto ends in the same `CollectiveFailure`, both before the rebuild and after the full rebuild, solver construction and solution transfer (1709-1713, 1824-1828). The second check therefore throws away the whole rebuild and transfer and then aborts.
Every message says "accepted mesh and solution retained", but the process exits.
Scenario: ADAP_NATIVE_MAX_TIME=600 and a 610 s event. The job dies instead of keeping the current mesh and continuing. The same happens when the default work budget runs out before optional coarsening finishes (C4).
The deadline does bound the remesher's wall time, but as a kill switch, not as a graceful skip.

#### N3-C2 — Medium, mechanism Confirmed, frequency Plausible: a metric-evaluation failure inside one composed proposal rejects the whole event
In the BL (composition) refinement path, the balanced split point is computed inside a `try` whose `catch` sets the elected failure (cpp:1095-1102). Then `vote` fails and `finish()` rejects the event.
`BalancedSplitPoint` throws when it does not converge in 48 steps or when the point is not representable (Field3D.cpp:151-162). It also throws when the composed quadrature does not converge at depth 20 or exceeds 65536 evaluations (Field3D.cpp:223, 246), or when EdgeBreakpoints exceeds its 65536-cut or 4096-band budget (CBoundaryLayerMetric.cpp:1617-1619, 1641).
The same pattern applies to the strip seed computation (cpp:1424-1486) and the joint proxy query (cpp:1605-1611).
By contrast, `measure_private` treats a fresh-measurement exception as an ordinary candidate rejection (cpp:1290, 1295). So the policy is inconsistent: a local, proposal-specific numerical failure should block the candidate, not reject the event.
Scenario: a steep BL band crosses one long edge, the quadrature recursion reaches depth 20, and the event is rejected (then N3-C1 aborts the run).

#### N3-C3 — Medium, Plausible: the clearance import cap rejects the event
`Clearance` imports the working-mesh neighbours of each fresh cell in batches. When a single-cell import exceeds 256 cells it returns `false` (cpp:100-103), and the caller ends the event (cpp:1803-1804).
In the exact-filter mode (`filter_cells` = source > 256 cells, Donor3D.cpp:222), every touching cell is kept, which includes all cells around the 4 vertices of the fresh cell. Near a refined curved wall, or after first-row construction, a vertex star of 60-100 cells times 4 vertices can exceed 256.
This should block the candidate, not reject the event.

#### N3-C4 — Medium, Confirmed (planar) / Plausible (analytic): the 512-triangle cap survives in chart construction
`ValidateSurfaceTopology` still throws above 4 x 128 = 512 triangles (Cavity3D.cpp:130-133). It is still used for:
- **Coplanar merging** (Boundary3D.cpp:561-564, `valid=false`). A planar same-marker component with more than 512 triangles (symmetry plane, far-field face) keeps one raw facet per original triangle. The consequence is C9: its interior boundary vertices can never be collapsed (`CollapseVertex` needs the retained vertex inside every incident facet), and `InsertEdgeCavity` refuses edges between raw facets. Coarsening of large planar boundaries is blocked.
- **Analytic sphere and disk charts** (Boundary3D.cpp:634-635). A sphere or disk chart with more than 512 triangles is marked invalid. What happens to an invalid chart further on is in the second reviewer's scope, so I report it as Plausible.

#### N3-C5 — Low, Plausible: blocked geometric candidates are not re-activated
`Selection::Block` erases every geometry-queue entry of the edge (cpp:213-215). `Touch` only re-inserts into the metric `queue` (cpp:228-242), and only when blocked links are tracked (`coarsening || track_splits`, cpp:219-220, 1903; `track_splits` is true only with composition).
A curved split that is rejected once comes back only when a cell containing that boundary face is replaced and re-added (`Add`, cpp:372-373). If the obstruction was elsewhere in the star, the geometric deficit stays and keeps its cells invalid. Repair moves may still fix it; if they do not, the event ends in OperatorStagnation.

#### N3-C6 — Low, Confirmed: coarsening gets only what refinement left of the work budget
`kernel.maximum_work -= refined.stats.global_work` (Remesher3D.cpp:340). If refinement used the budget exactly, `work_limit=0` (cpp:569, an explicit value, not SIZE_MAX), and the first coarsening proposal triggers WorkBudget, which rejects a valid refined mesh (C4, N3-C1). No underflow is possible (global_work <= work_limit on every path I checked).

#### N3-C7 — Low, Confirmed (latent): the first-row identity stride relies on an unchecked one-cell star
The first-row loop reserves 5 node identities per epoch (cpp:676-681). `ConstructFirstLayer` uses the background id plus one id per wall face in `old` (Boundary3D.cpp:327-330, 345). This is safe today only because a face feature resolves to the single boundary cell, which has at most 3 wall faces. Neither side checks this. The new-id check in `ConstructFirstLayer` covers only the local cavity (Boundary3D.cpp:329-330), not the global mesh.
The main-loop strides (1/2/4/131, cpp:1083) cover every id use I traced: strip movers +2..+4, the extra split shares the id of a mover that is removed in the frames where it is used (cpp:1459-1469, 1538-1540), joint interior +2, background +3, wall apices +4..+131.

#### N3-C8 — Low, Plausible (cross-scope): the first-height tolerance of 1e-8 is below the rounding of apex coordinates for small h/|x|
The final gate checks every working cell (cpp:497-503) with `CheckFirstHeight`, which requires |altitude/h - 1| <= 1e-8 (Boundary3D.cpp:288, 307). The apex `center - h*n` is rounded to binary64 (Boundary3D.cpp:345-347), so the altitude error is about 1e-16·|x|.
The gate fails whenever h < about 1e-8·|x|. Example: coordinates of order 100 with h = 1e-7. This belongs to the second reviewer's file, but it decides the engine's final acceptance.

#### N3-C9 — Low, Confirmed: the deadline makes acceptance timing-dependent
With ADAP_NATIVE_MAX_TIME set, whether an event is accepted depends on wall-clock time. Each rank's `deadline_start` is its own entry time into `Remesh` (Remesher3D.cpp:120). The decision is collective (see Strengths), so ranks do not diverge, but repeated runs can differ between "accepted" and "aborted".

---

### 4. New performance findings

#### N3-P1 — Critical, Confirmed: each accepted curved edit rebuilds a distributed collision index of the whole working mesh
In curved mode, every round in which some rank has an approved candidate calls `Clearance` (cpp:1801-1806). `Clearance` copies all local working cells (cpp:82) and runs a fresh `DonorImport::Build` on them (cpp:90-91). That build sorts the cells, sends 6 authority records per cell through a hash exchange into a `std::map` check, builds an ADT and BisectionBoxes, and runs a collective `CRankBoxTree::Build` (Donor3D.cpp:129-225).
Cost per edit: O((N/P) log N) local work plus O(N) total traffic. Per event this is O(N x edits), that is O(N^2) for an adaptation that changes a fixed fraction of the mesh.
Only a few dozen cells near the fresh patch are actually needed; a persistent index or the existing directory stars would do.
Estimate: 1e5 cells with about 1e5 curved edits, at 0.1-0.5 s per rebuild, gives 3-14 hours spent in clearance alone.

#### N3-P2 — High, Confirmed mechanism: the DonorImport::Build envelope caps cells per rank, and divides the cap by the OpenMP thread count
`envelope = 4096 x (n+1) x omp_get_max_threads()` is admitted against the ceiling (Donor3D.cpp:138-141; pre-existing, not in the baseline report). With the fixed 2 GiB ceiling this allows about 5.2e5/T original cells per rank for the pass-level source builds (cpp:626; Import3D.cpp:724).
In `Clearance`, n is the working mesh and the caller bytes already include the engine's resident storage (cpp:1803-1804). That leaves about 2-2.5e5 cells per rank with one thread.
An OpenMP build with T threads divides these numbers by T (64 threads gives about 8e3 cells per rank). Exceeding the limit rejects the event (then N3-C1 aborts).

#### N3-P3 — High, Plausible: composed (BL) edge integration has no reuse and an unbounded-in-practice cost per edge
- `MeasureMetricCells` reuses cached edge lengths only for sensor-only fields (Metric3D.cpp:111). With BL every seed edge, every old and fresh edge of every trial, and every edge already in the selection cache is integrated again.
- Each composed edge calls `EdgeBreakpoints`, which can return up to 65536 cuts. Each cut is checked by a wall-distance `Evaluate`, over every wall face in a box inflated by the BL thickness, times every band level (CBoundaryLayerMetric.cpp:1600-1690).
- Each sub-interval then runs adaptive 3/5-point Gauss to depth 20, with up to 65536 composition calls per edge (Field3D.cpp:222-256). Each call is an `ApplyPoint` with an ADT distance query per wall.
- `BalancedSplitPoint` adds up to 48 full-edge integrations per split (Field3D.cpp:147-162), and the strip calls it for 3 fractions (cpp:1430-1431).
The seed runs twice per event (Refine and Coarsen). For a near-wall edge I estimate 1e3-1e5 composition calls; not measured.

#### N3-P4 — High, Plausible: one curved/BL proposal can cost 1e3-1e4 collectives
The streamed path measures old and fresh cells in collective batches of 16 (`measure_private`, cpp:1238-1297). Each batch needs at least one `ImportMetricCells` (elections plus 2 exchanges) and votes.
A rejected composed boundary proposal can run:
- up to 36 x frames (frames <= 4) strip trials (cpp:1521),
- 12 growth steps x 2 frames of joint reconstruction (cpp:1613-1698),
- 19 repair trials (cpp:1755-1798),
- each `validate_private` on up to 128 fresh cells (8 batches).
That is about 1e3-1e4 collective operations for one proposal. All other workers take part while only one rank does work (N3-M2).

#### N3-P5 — High, Confirmed structure: first-row initialization is globally serial
The loop processes one wall face per epoch on the lowest rank that has pending cells (cpp:648-740). Each epoch runs about 30 collectives (Lookup, Fetch, Reserve, ImportCells, 2 routing exchanges, Publish, 2-3 votes), plus up to 4 `ConstructFirstLayer` attempts with a composed `MeasureMetricCells` each (cpp:698-713).
The number of epochs is at least the number of wall faces that need a first row, and re-added cells re-enter the queue (cpp:735). 1e5 wall faces mean 1e5 serial epochs, independent of P.

#### N3-P6 — Medium, Plausible: ConformingDisjoint has no floating-point filter
Each pair test evaluates up to 8 face axes and 36 edge-pair axes with Integer<66>/<132>/<198> products (about 30 products per axis), plus up to 12 exact `ClipInterval` calls for tangent pairs (Predicates3D.cpp:318-381). Face-adjacent pairs are always tangent, so they pay the full cost.
`Clearance` tests all fresh-fresh pairs (O(|fresh|^2), up to 8128) and every fresh-neighbour pair (cpp:84-87, 105-110). Estimate 0.1-1 ms per adjacent pair, so 0.1-a few s per curved edit, on top of N3-P1.

#### N3-P7 — Medium, Confirmed mechanism: Block scans the whole geometry queue
`Selection::Block` walks `geometry_queue` linearly to erase entries of one edge (cpp:213-215). The queue holds up to 3 entries per boundary face with geometric excess, so the cost is O(boundary) per rejected proposal. A set keyed by edge, or a reverse index, would make it logarithmic.

#### N3-P8 — Low, Plausible: the layered trace is quadratic in work pieces
When extension donors are present, every sub-interval scans all work pieces with exact `CompareRoot` and runs one `ClipInterval` (Predicates3D.cpp:428-466). This is O(W^2) root comparisons, with W <= 256.

#### N3-P9 — Low, Confirmed: the first-height checks run on every cell
`finish()` calls `CheckFirstHeight` on every working cell after a valid pass (cpp:497-503), and the first-row setup scans every cell (cpp:644). Each call builds an Incidence, a SkinMap and a CheckSurface even for interior cells. This is O(N) with a large constant per pass; a wall-face filter would avoid most of it.

#### N3-P10 — Low, Confirmed: the first BL metric triggers an extra full import
`ComputeMetric` calls `MakeRemesher()` while the reference is uninitialized (CSinglezoneDriver.cpp:251-255). `MakeRemesher` calls `PrepareReference`, which runs a complete `ImportGeometry` with identity sensors only to seed the reference (Remesher3D.cpp:30-47). The first `Remesh` then imports again. Failures there are fatal through `CollectiveFailure`.

---

### 5. New MPI findings

**What is distributed vs gathered (changes only).** The engine, the directory, the donor source and the collision index are distributed. The boundary reference, including the analytic charts and bump sub-references, is replicated on every rank. The sidecar is rank-0 I/O. Per-event report rows are gathered on rank 0.

#### N3-M1 — High, formula Confirmed, numbers estimated: building the boundary reference caps B x P
With C1 lifted, the first import admits `ConstructionBytes(B)` (at least 4096 B per facet, Boundary3D.cpp:741) plus `P x B x 3 x sizeof(Facet)` (Import3D.cpp:528-531). `sizeof(Facet)` = 112 B, so the replication term is 336·P·B. `World::metadata` really sends P copies of each local list (CNativeDistributed2D.hpp:183-186).
Against the fixed 2 GiB ceiling the boundary limit is about B <= 2^31/(4096 + 336·P):
- 5.2e5 triangles at P=1,
- 2.3e5 at P=16,
- 8.4e4 at P=64,
- 2.4e4 at P=256.
Exceeding it rejects the event (`ImportGeometry` returns false, then Remesher3D.cpp:205 breaks) or aborts in `PrepareReference` (BL). The 1 MiB sidecar (M6) is a tighter limit for restart and mesh output. Together they replace the old 512-triangle cap with a cap that shrinks as ranks are added.

#### N3-M2 — High, Confirmed: curved and BL modes serialize proposals globally
With `curved || composition`, only one rank is active per round (cpp:985-999; the code comment calls it a shortcut). Batching is disabled for these modes (cpp:925).
Planar BL picks the globally shortest/longest edge. Curved runs pick the lowest rank that has a candidate, so rank 0 drains first, and the edit order and final mesh depend on the partition (M3, M7).
All other workers join every collective of the round, and of the trial loops of N3-P4, while idle. Throughput is one edit per round regardless of P, and first-row initialization is serial too (N3-P5). This is the dominant MPI scaling limit of the new features.

#### N3-M3 — Medium, Confirmed: the time limit is checked only at coarse safe points
Every check is collective and consistent (see Strengths). There is no check inside:
- the seed `measure` (cpp:752-810; an O(N/P) exact or composed integration of every edge),
- `Clearance` (N3-P1) and the source/directory builds,
- the 8 slots of a batched round (one check per round, cpp:867),
- the 19 repair trials,
- `ImportGeometry`, `Repartition`, `ReaderSlices`,
- the CFD rebuild in ReplaceMesh.
The overshoot is unbounded, and when the limit is detected the run aborts (N3-C1).

#### N3-M4 — Low, Confirmed: an always-on per-event report gather on the master
Every event `Allgatherv`s one row of about 1.5 kB per rank to rank 0, which prints them all (Remesher3D.cpp:446-470), even without WRT_PERFORMANCE. That is O(P) text and memory on the master per event. Pre-existing; not in the baseline report.

---

### 6. Strengths

- **The new controls are consistent across ranks.**
  - The work budget uses `global_work`, which advances identically on every rank: from Allreduce sums (cpp:951-956, 1001-1006) or from increments inside collectively driven loops (cpp:738, 1523, 1617). `global_work <= work_limit` on every path.
  - The time limit is a collective `world.maximum(elapsed >= limit)` (cpp:650, 867, 1524, 1618), with a common remaining time from MIN reductions (Remesher3D.cpp:121-124, 342).
  - `beforeAcceptance` is created only when `ready` (a collective state) and the limit is positive (checked equal on all ranks, Remesher3D.cpp:163-169), so its presence matches on all ranks. The local clock result goes through `CollectiveFailure` (an election), so ranks cannot diverge or deadlock on it.
- **Option consistency is checked up front.** Rounds, work, window, time, batch size, curvature, surface tolerance, composition and edge bands, intent and wall-height policies are all compared across workers (cpp:543-599).
- **Termination is guaranteed.** Every round with `any` true makes at least one proposal, so the round loop is bounded by the work budget. Every inner trial loop is bounded and also charged or checked.
- **Byte-bounded publication** (M1 fixed) and per-candidate refusal in LookupBatch/FetchBatch, with shared payloads fetched once.
- **Exact predicates extended carefully.** `ConformingDisjoint` is integer-exact, including tangencies. The layered trace selects the original source at exact cuts. Extension donors migrate with their CFD parent cell (Remesher3D.cpp:276-285). A static_assert ties the layered scratch to the byte model (Predicates3D.cpp:204-205).
- **Collective consistency in the new trial loops.** Every strip, joint, growth and fallback loop is driven by `world.maximum`. Every lookup, fetch and import is called by all ranks with empty inputs on inactive ranks. Every early return follows an elected result. I found no divergent collective sequence.
- **Identity reservation.** All main-loop id uses fit their strides (N3-C7), and overflow is guarded by saturating `transfer_memory` arithmetic plus explicit range checks (cpp:1109-1111).
- **The repartition-free path is cheaper** than at baseline (no weights, no parent source build).

---

### 7. Not verifiable statically

- Real costs of N3-P1, N3-P3, N3-P4 and N3-P6 (clearance rebuild, composed integration, collective counts per proposal, exact SAT). My numbers are estimates.
- How often N3-C2 (quadrature or split non-convergence), N3-C3 (clearance cap), C5 (oversized stars after first-row cones) and C8 (with extension donors) occur on real meshes.
- `omp_get_max_threads()` in the production builds (N3-P2), and how SU2's OpenMP stub reports it.
- What happens downstream to an invalid analytic chart (N3-C4, second part), and the correctness of SplitSurfaceStrip, ReconstructSplitPatch and ConstructFirstLayer (second reviewer).
- Whether the 1e-8 first-height tolerance (N3-C8) is reached in practice for the intended h/|x| range.
- `CReaderSlices::FromDistributed` and `CSolutionTransfer` (unchanged, not re-read).
- `sizeof(Facet)` = 112 B and the 260 B per sidecar line assume a 64-bit ABI and 17-digit output.

---

# Part III — 3D boundary / boundary layer and 2D re-assessment (full report)


Code read: the frozen copy `$S` = `scratchpad/snap_b449` (commit b449b5b800, branch codex/native-3d-bl). Diffs: `git -C SU2_NativeIntegrated diff 7746fd53fa..b449b5b800` (51 commits).
Nothing was built or run. No repository file was changed. All file:line references point into `$S`.

Shorthand: `B3` = Common/src/adaptation/CNativeBoundary3D.cpp, `B3h` = its header, `RIO` = Common/src/adaptation/CNativeReferenceIO3D.cpp, `BL` = SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp, `E3` = Common/src/adaptation/CNativeEngine3D.cpp, `F3` = Common/src/adaptation/CNativeField3D.cpp, `I3` = Common/src/adaptation/CNativeImport3D.cpp, `R3` = Common/src/adaptation/CNativeRemesher3D.cpp, `CS` = SU2_CFD/src/solvers/CSolver.cpp, `DRV` = SU2_CFD/src/drivers/CSinglezoneDriver.cpp.

Read in full: B3h, B3, CNativeReferenceIO3D (hpp and cpp), CBoundaryLayerMetric (hpp and cpp), the native parts of CS ComputeMetric (2640-3120 and 3340-3370), the CPoint.cpp change.
Read because a call path needed it: E3 (Clearance 71-118, the first-row initialization 630-735, the split point choice 1086-1130, the final height check 475-503), F3 (composed edge length 142-253), I3 (Coverage 119-300, reference construction 505-590), R3 (diff), DRV (composition 296-327, ComputeMetric 227-266, ReplaceMesh 1700-1713), CNativeCavity3D.cpp (Surface 55-133, ValidateDomainSurfaceTopology 135-157), CNativeDistributed3D.cpp (face-feature Lookup 282-320).

Severity scale (as in the baselines): Critical = blocks use beyond toy meshes or aborts the run; High = wrong outcome or event loss in realistic cases, or a cost that prevents scaling; Medium = significant limit or waste; Low = minor. Confidence: Confirmed = the code path was followed through its callers; Plausible = the mechanism is in the code, frequency or cost is my estimate.

---

### 1. What changed (short)

- **Domain skin validation**: import and sidecar decoding now call `ValidateDomainSurfaceTopology` (CNativeCavity3D.cpp:135-157). It requires a manifold, consistently oriented, closed skin, with no size cap and no single-component or genus-0 requirement. Commit 679fe68ef4 is in the range.
- **Analytic charts** (B3h:35-46, B3:29-92, 424-448, 467-675, 794-1008): declared Sphere, Bump, Cylinder and Disk per original facet. They provide projection (`Project`), tolerance containment, chart joining into complete components (`complete_analytic`), bump charts reusing a flattened `PlanarReference`, and a conservative surface error bound (`SurfaceError`). They are declared only through a hand-written `.native_ref` version 2 or 3 (RIO:111-119). A first import builds a planar-only reference (I3:553). Exterior "coverage" donors are added outside curved charts (I3:119-300).
- **Curved operators**: `ProjectSplitPoint`/`ProjectMovePoint` (B3:176-212), the curved paths of `SplitEdge`, `InsertEdgeCavity`, `CollapseVertex` and `MoveVertex`, a new `SplitSurfaceStrip` (B3:1118-1175), and an engine-side `Clearance` (E3:71-118) for every curved round.
- **First-height wall constraints**: `CheckFirstHeight` (B3:253-315) and `ConstructFirstLayer` (B3:317-376), `ConstrainMovePoint` (B3:378-423), the joint wall reconstruction `ReconstructSplitPatch` (B3:1176-1326), a global first-row initialization in the engine (E3:630-735), and a final height and footprint gate on every cell (E3:497-503).
- **BL policy for native 3D**: `CheckSupport` no longer rejects `ADAP_BL` (R3 diff). The driver composes the frozen P1 sensor with `CBoundaryLayerMetric::ApplyPoint` at every query (DRV:300-322), adds geometric edge breakpoints (`EdgeBreakpoints`, BL:1600-1703) and passes the first height per marker. CS integrates the complexity with a new geometric quadrature (`IntegrationRule3D`, BL:1111-1598; CS:2860-2908, 3089-3118). `WallMetric3D` (BL:76-115) replaces the 3D spectral inversion for all non-AD callers, including MMG.
- **Sidecar** (RIO): format versions 1-3. Version 2 adds analytic records, version 3 adds axes. Text, 17 significant digits, classic locale.
- **Work and time controls**: `ADAP_NATIVE_MAX_WORK` and `ADAP_NATIVE_MAX_TIME` (CConfig), with a `beforeAcceptance` deadline veto (DRV:1709-1713, 1824-1828).
- **CPoint.cpp:136-141**: `SetPoints` builds `Edge` from the list of lists when the point pattern has no inner entries (empty rank). `CCompressedSparsePattern::empty()` is `outer empty || inner empty` (graph_toolbox.hpp:184), and the list-of-lists constructor (138-147) gives correct outer pointers with zero values. **Sound; no finding.**
- **2D**: `git diff --stat 7746fd53fa..b449b5b800` over all 2D headers and sources shows only CNativeRemesher.cpp, +2 lines (36-37: reject `ADAP_NATIVE_MAX_WORK`/`MAX_TIME`). The shared CS 2D BL quadrature block is now wrapped in try/`CollectiveFailure` (CS:2871-2908); its behaviour is otherwise unchanged.

---

### 2. Status of old findings

#### 3D (baseline native3d_assessment.md, items that touch boundary, BL or curvature)

| ID | Old severity | Status | Evidence at b449 |
|---|---|---|---|
| C1 (512-triangle, single-sphere boundary) | Critical | **Partly fixed** | Import and decode use `ValidateDomainSurfaceTopology` (CNativeCavity3D.cpp:135-157; I3:552; RIO:122), with no cap and multiple components allowed. Still limiting: (a) the sidecar is capped at 1 MiB (CNativeReferenceIO3D.hpp:11, RIO:72-73, 79, 145), and writing it is mandatory at every mesh output (DRV:1663-1664). That fails above roughly 4-5k facets (about 230 bytes per facet line at 17 digits), on rank 0, fatally. (b) Planar merging and analytic completeness still go through `ValidateSurfaceTopology` (cap of 512, CNativeCavity3D.cpp:130-133; B3:563, 634-635), and a Disk with more than 512 facets is fatal (N3B-C3). (c) Replicated admission is 4096 B per facet (B3:741). Residual severity: Critical for 1e5-facet surfaces, because of the sidecar. |
| C2 (rejected event aborts run) | Critical | **Still open** | DRV:1703-1707: `CollectiveFailure` on a status other than COMPLETE. The new deadline veto (DRV:1709-1713, 1824-1828) is also fatal, although its message says "accepted mesh and solution retained". Every new BL/curved failure listed below ends here. |
| C6 (exact on-edge/coplanar, no curved surfaces) | High | **Partly fixed** | Analytic sphere, bump, cylinder and disk with projection (B3:972-993, 176-198) and off-edge curved splits. Not fixed: (a) analytic charts are declared only in a hand-written sidecar; no config or CAD path exists, and the first import is planar-only (I3:553). (b) Faceted planar walls still need exact `OnEdge` (B3:108-113, 186-189) or exact coplanarity in a merged canonical patch (B3:856, 1072-1073, 1083-1084), so the edges of oblique or faceted-curved walls generally cannot be split. (c) With BL composition the split point is a metric-balanced fraction (E3:1100, 1116-1117), which removes the curved exact-midpoint exemption (N3B-C4). (d) BL on analytic walls throws (BL:997-998). |
| C9 (boundary coarsening only on convex coplanar patches) | Medium | **Still open** | B3:561-590. The "retain raw constraints on nonconvex patches" shortcut is still there (B3:586). Merging is now also limited to 512 facets per component (B3:563). Bump charts reuse the same convex rule (B3:601). |
| C10 (narrow operator/feature set: no BL, no gradation, ...) | Medium | **Partly fixed** (boundary/BL part) | The BL rejection was removed (R3 diff). First-height construction exists (B3:317-376; E3:630-735). Native sensor gradation is in CS (2944-3046). `ADAP_SURFACE= NO` is still rejected (R3:98-99). The composed BL target is neither graded nor bounded (N3B-C14). The operator-set items belong to the engine reviewer. |
| C11 (rounded sliver centroid outside the source) | Low | **Still open** (planar) | F3:320 still throws "Query outside native 3D immutable original donors". For curved charts the exterior coverage donors (I3:119-300) reduce the risk; the planar case is unchanged. |
| C14 (stale documentation) | Low | **Still open, reversed** | NATIVE_ADAPTATION.md:34 says "no `ADAP_BL`", and :212-214 says "3D BL construction, curved projection ... remain pending". The code now enables both (R3 CheckSupport; DRV:296-327). |
| M6 (replicated reference, 1 MiB sidecar) | Medium | **Still open, larger** | `parent.metadata(local)` (I3:545) is admitted as P × count × 3 × sizeof(Facet) (I3:528-531). The 1 MiB cap is unchanged. New: every rank also builds the full BL wall data and its ADT from the replicated reference (BL:988-1012), with 8 KiB per facet admitted on top (R3:211). See N3B-M2. |
| M10 (master gathers returned boundary faces) | Low | **Still open** | CReaderSlices.cpp is unchanged in the diff; it still gathers on the master (CReaderSlices.cpp:632). |
| P9 (map-based, replicated reference) | Low | **Still open, larger** | Seven-plus `std::map` members (B3h:80-89), 4096 B per facet admitted (B3:741). New O(V·T) and O(B·V) construction loops (N3B-P7). |

The other old 3D findings (C3-C5, C7, C8, C12, C13, P1-P8, M1-M5, M7-M9) belong to the engine/core reviewer and were not re-assessed. Two engine items on the BL/curved call path are reported as new findings below (N3B-P1, N3B-M1).

#### 2D (baseline native2d_assessment.md, High and Critical items)

All 2D kernel sources are unchanged apart from CNativeRemesher.cpp:36-37 (two lines that reject the 3D-only controls). Lines after it shift by +2.

| ID | Old severity | Status | Evidence at b449 |
|---|---|---|---|
| C1 (work allowance ≈120k bulk insertions per rank) | Critical | **Still open** | CNativeRemesher.cpp:423-424 (`phase_rounds = min(20000, max(300, ceil(.../(4*ranks))))`), CNativeEngine2D.hpp:60 (`sweeps = 6`). |
| C2 (incomplete result terminates the run) | High | **Still open** | DRV:1703-1707. |
| P1 (one cavity per rank per collective round) | High | **Still open** | CNativeEngine2D.hpp unchanged (round structure 307-616). |
| P2 (donor ADT off above ≈29k cells per rank) | High | **Still open** | CNativeField2D.hpp:341-343; `dependency_bytes = 2 MiB` at CNativeEngine2D.hpp:58. |
| P3 (surface selection rescans and O(K) deviations) | High | **Still open** | CNativeEngine2D.hpp:625-626 (uncached `choose` for surface actions), :203. |
| M1 (certificates replicated to P ranks under 2 MiB) | High | **Still open** | CNativeEngine2D.hpp:492-499. |

---

### 3. New correctness findings

**N3B-C1. The 3D geometric complexity quadrature has fixed depth and work limits; exceeding them aborts ComputeMetric.** High. Plausible (mechanism confirmed).
`IntegrationRule3DImpl` (BL:1478-1536) bisects the longest edge until the wall distance at edge, face and cell midpoints is within `0.1*max(h0, min vertex distance)` of linear interpolation (BL:1505-1516). It throws at depth 32 (BL:1521) or after 16384 visits (BL:1481). In the hidden-support path, the radial and angular refinement throws at depth 32, at 16384 visits, or at 65536 points (BL:1379-1394, 1418-1419, 1447). CS catches the exception and calls `CollectiveFailure` (CS:3114-3117), which is fatal.
Scenario: a wall-adjacent cell with a vertex on a convex ridge or corner of a BL wall (trailing edge, wing tip, box edge). In the wedge where the nearest point is the edge or vertex, the distance is a cylinder or cone, and its midpoint defect is about r·φ²/8 at every scale. With min distance 0 the tolerance is 0.1·h0, so refinement must continue down to r ≈ h0. One longest-edge bisection halves the cell size only about every three levels in 3D, so the depth needed is about 3·log2(L/h0). That exceeds 32 for L/h0 above about 2e3, which is common for original RANS meshes (L ≈ 1e-3, h0 ≈ 1e-6). The first ComputeMetric then aborts the run. Not measured.

**N3B-C2. The first-height certificate uses a fixed relative tolerance of 1e-8, while the apex is a rounded binary64 point.** Medium. Plausible.
`ConstructFirstLayer` places the apex at `round(center - h·n)` (B3:344-349), and `ReconstructSplitPatch` does the same (B3:1195-1197). `CheckFirstHeight` requires `|altitude/h - 1| <= 1e-8` (B3:284, 303). The rounding of the center and of the apex moves the apex normal to the wall by up to about 4e-16·|x|, so the check fails when h < ≈4e-8·|x_wall|. That happens with walls far from the origin or at very high Reynolds numbers, for example a wing at x ≈ 30 m with h0 = 1e-6 m. `ConstrainMovePoint` (B3:419-421) re-rounds moved apices in the same way. The final gate checks every wall cell (E3:497-503), so one failure invalidates the event, which is fatal by C2.

**N3B-C3. Chart joining is still capped at 512 facets per component; a Disk chart above that is fatal.** Medium. Confirmed.
Planar merging (B3:562-566), complete spheres (B3:634) and disks (B3:635) go through `ValidateSurfaceTopology`, which throws above 4·128 facets (CNativeCavity3D.cpp:131). For planes, bumps (via the recursive chart at B3:601) and spheres the component silently stays per-facet, so old C9 applies to every large symmetry or flat wall. For a Disk the constructor throws "Disk authority requires its complete connected circular cap" (B3:674), so a valid sidecar with a finely meshed end cap cannot be loaded. The Cylinder branch has no such cap (B3:636), so the limits are inconsistent.

**N3B-C4. With BL composition, every boundary split uses a metric-balanced chord point, which the boundary operators accept only in special cases.** Medium. Confirmed mechanism.
E3:1086-1100 computes `BalancedSplitPoint`, and E3:1116-1117 replaces the midpoint by it for all edges, boundary edges included; E3:1431 does the same for strips. `ProjectSplitPoint` accepts an off-chord point only if it is exactly the rounded midpoint of a curved edge (B3:184-189). Otherwise it needs exact `OnEdge`, which a rounded fraction meets only on axis-aligned edges. The fallback `InsertEdgeCavity` needs both faces on one canonical chart and, for planar charts, exact coplanarity (B3:1072-1073, 1083-1084).
Consequence: with any `ADAP_BL` active, splits of curved edges on non-complete analytic charts (for example a sphere with more than 512 facets, N3B-C3) lose the path they had, and wall edges between different raw facets (every edge of a faceted curved wall) cannot be split. Long wall edges stay, the event stagnates, and C2 applies.

**N3B-C5. `EdgeBreakpoints` enforces its 65536 cap before discarding invalid and duplicate roots.** Medium. Plausible.
Roots are generated per face for 3 vertex spheres, 3 edge cylinders and 2 face planes at every level (BL:1657-1687). A vertex shared by about 6 faces, or an edge shared by 2, produces the same roots again. `retain` throws at 65536 (BL:1616-1619) before `validRoot` filters (BL:1698) and before `unique` (BL:1701). The levels are about log(full/h0)/log(1.5) + 2 + fadeBands, about 45 for h0 = 1e-6 and T = 0.05.
Example: an edge at distance about 0.4T from a wall whose original faces are 50 times smaller than T yields 1e4-1e5 candidate roots. The composed edge length (F3:209) then throws, and the event is rejected (fatal by C2).

**N3B-C6. `ReconstructSplitPatch` admits skins of up to 128 faces, but its wall path needs at most 64.** Low. Confirmed.
B3:1207 allows `skin.size() <= 128`. The cone `proposal` has one cell per skin face and is passed as `old` to `ConstructFirstLayer` (B3:1283), which starts with `Bounds(old)`, limited to `MaximumCavityCells = 64` (B3:228-231, 322). Joint wall reconstructions with 65-128 skin faces are therefore always rejected, with the misleading reason "edge patch ... exceeds cell budget".

**N3B-C7. BL on an analytic wall is rejected only at run time.** Low. Confirmed.
`FromNativeReference` throws "analytic BL distance/normal evaluation is not implemented" (BL:997-998). It is reached first inside ComputeMetric (CS:2880, caught, then `CollectiveFailure` at CS:2908) or in the composition factory (R3 diff, fatal). `CheckSupport` cannot see it, because analytic charts come only from the sidecar, so the user learns this after the first CFD solve.

**N3B-C8. Analytic tolerances are not checked against binary64 resolution.** Low. Plausible.
The constructor accepts any `tolerance > 0` (B3:472). The default is 1e-12 (B3h:44). `Project` rounds the projected point and then requires `Contains` within the tolerance (B3:981-991). Where ulp(|x|) is above the tolerance (|x| ≳ 4e3 with the default), every projection fails ("projection leaves its finite original chart"), so every curved split and move is rejected without a diagnostic that names the cause.

**N3B-C9. Projections are sequential, so a point cannot satisfy two charts at a junction.** Low. Confirmed (the header documents it: B3h:58).
`ProjectSplitPoint`/`ProjectMovePoint` project onto each incident chart in turn (disks first, B3:190-193, 201-204). Only the disk-cylinder rim works, by construction. At a junction between two non-disk charts, or between an analytic chart and a planar facet (which needs exact coplanarity, B3:856), the point leaves one of the two, and `Contains` rejects (B3:194-196, 205-210). Such feature edges can never be split or moved.

**N3B-C10. `WallMetric3D` adds exception paths to the shared MMG 3D BL metric.** Low. Plausible.
In non-AD builds, every 3D `Evaluate` now goes through `WallMetric3D` (BL:932), including the MMG path `layers.Apply` (CS:3531-3532). That call is not in a try block, so "Invalid/Singular native 3D wall tangent size" (BL:99-104) terminates that rank instead of raising a collective error. The trigger is extreme face anisotropy, so this is rare. The MMG 3D BL metric is also recomputed by a different formula (mathematically equal), so MMG regression references can move in the last bits.

**N3B-C11. Footprint resolvability depends on the width of `long double`.** Low. Plausible.
B3:294-296 throws when `256·eps_ld·max(1,|u||v|/area) > 1e-8`. With 80-bit x87 this triggers only for |u||v|/area > 3.6e8. Where `long double` is double (MSVC), the limit is 1.75e5, so needle wall faces abort the certificate on some platforms only.

**N3B-C12. The sidecar is adopted without an identity check and published without fsync.** Low. Plausible.
`PrepareReference` and `Remesh` load `<MESH_FILENAME>.native_ref` whenever it exists, also on a cold start (R3:30-48, 185). The file carries no mesh hash or provenance. A stale sidecar either fails binding later (fatal) or silently imposes old analytic declarations. `WriteReference` uses temp plus `rename` without fsync (RIO:203-208). The text format itself is sound: 17 digits round-trip exactly, the locale is classic, there is no endianness issue, versions 1-3 are checked, counts are bounded by the file length, kinds 1-4, axes in v3, topology and duplicates are validated (RIO:23-35, 78-125).

**N3B-C13. The BL policy prevents coarsening along walls below the original wall faces.** Low. Confirmed (design).
Tangential sizes come from the immutable original facets (BL:612-709, 896-918). `ApplySample` intersects them with the sensor (BL:1882), so tangential size ≤ original face size within the full band for every later cycle. A first mesh that is over-resolved along a wall stays over-resolved.

**N3B-C14. The composed target is neither graded nor bounded.** Low. Confirmed.
Gradation and `ADAP_HMIN/HMAX/AR` apply to the sensor only (CS:2947-2950 comment, 3027). The composed field is only measured for diagnostics (CS:3351-3370, `composedViolation`). The remesher target therefore can exceed the bounds and the gradation next to the BL edge and the fade.

**N3B-C15. The composed edge length has hard evaluation and convergence limits.** Low. Plausible.
F3:220 allows at most 65536 compositions per edge; F3:236-247 requires a 1e-9 relative 3/5-point agreement at depth ≤ 20. Kinks of the sensor ∩ BL intersection are not among the breakpoints, so convergence is slow there. A throw rejects the event (fatal).

Checked and found sound: exception safety of all B3 operators (built in local `proposal`, swapped or moved into `fresh` only on success); identifier overflow guards (B3:233, 330, 369, 1280, 1317); `CheckFirstHeight` altitude formula (`Orientation` returns the determinant, so det/|cross| is the height; B3:283 with CNativePredicates3D.cpp:286-288); barycentric footprint of the normal foot (B3:289-298); bump interpolation error bound, including the Hessian row-sum bound (B3:944-965); first-row id reservation (one-cell face star, CNativeDistributed3D.cpp:300-310, so at most 3 wall faces and 4 ids within the 5 reserved, E3:676-685); WallMetric3D SPD by construction (BL:110-113); log-Euclidean fade (BL:1873-1875); `EdgeBreakpoints` completeness (vertex, edge and face offset surfaces cover all level-set pieces of the faceted distance); deterministic ordering everywhere (sorted ADT candidates BL:1134, 1656; ties broken by index in the priority queue BL:1415).

---

### 4. New performance findings

**N3B-P1. Every curved round rebuilds a collision index over the whole mesh.** Critical. Confirmed mechanism, cost by reading.
`curved = !geometry.Analytic().empty()` is a global flag (E3:434). In every round where any rank approved a proposal, `Clearance` runs (E3:1801-1803). It copies the whole `working` map into `current` (E3:82-83), builds a new `DonorImport` over it (E3:91-92: copy, sort, ADT, six authority records per cell through a hash exchange, rank boxes; CNativeDonor3D.cpp:129-160), and runs batched `ImportCells` collectives per 16 fresh cells (E3:94-114). The cost is O(N/P·log) per rank and O(N) traffic per round, so a curved event with R rounds is O(R·N), quadratic in mesh size. It applies even when the proposal is far from any curved chart.

**N3B-P2. The complexity quadrature of every cell is rebuilt in every complexity trial, although most rules do not depend on the sensor.** High. Confirmed.
CS:3100-3106 rebuilds `IntegrationRule3D` whenever the four nodal tensors or `frozenCore` differ. Each trial rescales and regrades the sensor (CS:3045-3061), so in practice every cell is rebuilt every trial. For cells without hidden support, the rule is `IntegrationRule3DImpl` (BL:1164-1166), which never calls `density`. Cost per trial: all owned cells × (4 `Evaluate` per wall + ADT box queries + `MayIntersectWallBand` + up to 15 subdivision probes, and for BL cells the clipping plus one `SamplePoints` per point). The number of trials is the bracket expansion plus up to 100 Illinois iterations (CS:3172-3226), typically 10-30.

**N3B-P3. The 3D quadrature storage is capped at 2 GiB per rank, and the failure is fatal.** High. Plausible (point counts estimated).
CS:3110-3111. An `IntegrationPoint3D` is 64 B plus one 280 B `PointSample` per active wall. A coarse original near-wall cell spanning about 18 distance bands produces several hundred points, about 0.1-0.3 MB. That allows only about 1e4 BL-band cells per rank before "Native3D complexity quadrature exceeds storage budget". The failure is fatal (CS:3117).

**N3B-P4. `EdgeBreakpoints` cost scales with the number of wall faces near the chord times the number of levels, plus one `Evaluate` per candidate root.** High. Plausible.
The cost is F_box × (≈7 × levels) quadratic solves, where F_box ≈ (L+2T)²/s_wall² for a planar wall, plus one full `Evaluate` (ADT nearest, rotations, eigensolve) per root in (0,1). Roots are duplicated per incident face (BL:1657-1698). It is called for every composed edge length (F3:209), up to 48 times per `BalancedSplitPoint` (F3:147-163), once per angular sample chord in hidden-support quadrature (BL:1355), and for every edge of the seed. With T/s_wall ≈ 50 this is milliseconds to tens of milliseconds per edge, times 1e6 edges.

**N3B-P5. A 3D `Evaluate` builds the full wall tensor before testing the weight.** Medium. Confirmed.
BL:868-918 (three `RotateNormal`, `Spectral` exp, projections) runs before `FadeWeight` (BL:920). Distance-only callers pay this too: `IntegrationRule3D` hidden test (BL:1127), `IntegrationRule3DImpl` distances (BL:1486), `validRoot` (BL:1690). So do points beyond the thickness that are inside the padded bounding box (BL:1908-1911). A distance-only query would avoid an eigensolve and three 3x3 rotations per call.

**N3B-P6. The composed edge integration costs hundreds to thousands of BL compositions per edge.** Medium. Plausible.
F3:213-253: per cut interval, an adaptive 3/5-point Gauss rule at 1e-9 relative tolerance. Each value is `composition` = `ApplyPoint` (ADT nearest per wall plus `IntersectMetrics`, BL:1904-1913) plus `ValidateTensor`. With about 45 cuts that is at least 360 compositions per BL edge before refinement. The baseline's P2/P4 multipliers (seed passes, repeated integration per edit) apply on top.

**N3B-P7. Reference construction has super-linear loops that run on every rank.** Medium. Confirmed.
- The cylinder link check is O(V·T) per component, with no size cap: for each vertex it scans all triangles (B3:640-647). That is 5e9 steps for a 1e5-facet cylinder.
- The planar convexity test is border edges × vertices (B3:581-585). It also runs for components already marked invalid, for example a 1e5-facet symmetry plane above the 512 cap.
- Both run on every rank at every construction, and `EncodeReference` re-decodes, and so reconstructs, on rank 0 at every write (RIO:75).

**N3B-P8. The placement search in `ReconstructSplitPatch` has a large constant.** Medium. Confirmed.
B3:1256-1267: up to 9 levels × 3 sweeps × 26 directions = 702 `cost` evaluations. Each one runs over up to 128 skin faces with up to 4 wall-child candidates, each with `Orientation` and `Measure`, giving up to about 3.6e5 measures per joint proposal before the engine's metric certification.

**N3B-P9. Linear passes and repeated builds.** Low. Confirmed.
- The final gate runs `CheckFirstHeight` (Incidence, SkinMap, CheckSurface) on every working cell (E3:497-503), and the initial `needs_layer` scan is similar (E3:636-645).
- `FromNativeReference` rebuilds the BL wall data and its ADT in every ComputeMetric (CS:2876-2880) and every Remesh (DRV:302) on every rank.

---

### 5. New MPI findings

What is replicated now: the full `PlanarReference` (facets, nodes, canonical maps, bump sub-charts, ADT of chart boxes) and, new, the full BL wall data of every `ADAP_BL` marker (coordinates, connectivity, normals, vertex log tensors, ADT; BL:988-1012, 511-770) on every rank. The sidecar is read and written on rank 0 only; every rank decodes it. BL evaluation, `CheckFirstHeight`, the quadrature and all B3 operators are rank-local, with no collectives inside.

**N3B-M1. First-row initialization is serialized over the whole job: one rank and one wall cell per global round.** Critical. Confirmed mechanism, cost Plausible.
E3:653 elects `caller = min rank with a non-empty pending set`. Only that rank proposes, and only its first pending cell, with only the first wall face as the feature (E3:656-661). Every rank still takes part in the full protocol for that single cell: Lookup, Fetch, Reserve, ImportCells, two routing exchanges, Publish and several votes (E3:665-726), plus a new `FrozenField` and up to 1+3·walls `ConstructFirstLayer` trials, each measured with the composed metric and therefore `EdgeBreakpoints` (E3:686-704). The number of rounds equals the global number of original wall cells that need a layer. With 1e5-1e6 wall faces and tens of milliseconds per round, this is 1-10+ hours, independent of P (worse with P because the round cost grows), before refinement starts. Lower ranks finish first.

**N3B-M2. Replicated reference and BL data meet the fixed 2 GiB ceiling.** High. Formulas confirmed, threshold estimated.
Per-rank admission:
- 4096 B per facet for the reference (B3:741);
- 8192 B per facet for the BL policy, counted over all facets, not only BL markers (R3:211);
- P × count × 3 × sizeof(Facet) (112 B) for the replicated wire copy at first import (I3:528-531).

Under the default `memory_ceiling = 2 GiB` (CNativeEngine3D.hpp:14, the driver passes `EngineOptions{}` at DRV:323), BL runs are refused somewhere near 1.5e5 facets per rank (fewer with large N/P), and the first import near P·count ≈ 6e6 (for example 1e5 facets at P = 64). Refusal is a collective failure (fatal). The 1 MiB sidecar (C1/M6) comes first in practice.

**N3B-M3. The first unsteady window can use a rank-local BL wall subset.** Low. Plausible.
`SampleTimeWindowMetric` and `PredictWindowMetric` call `ComputeMetric` with `nativeReference3D.get()` without first creating the reference (DRV:573-575, 783-784). `CSinglezoneDriver::ComputeMetric` does create it (DRV:251-255). When the reference is still empty, CS falls back to the restricted constructor (CS:2878-2880, BL:263-448), which keeps per rank only the faces near the owned-point boxes, padded by T + 2·max face diameter (BL:404). Quadrature points in owned cells with halo vertices can lie up to one cell size outside those boxes, so the nearest face can be missing and the complexity can depend on the partition. On the first cycle the walls are the original ones, so the effect is limited.

**N3B-M4. Sidecar decoding and validation are replicated or serial.** Low. Confirmed.
Rank 0 reads, `BcastRounds` (RIO:165), then every rank runs `DecodeReference`, which includes `ValidateDomainSurfaceTopology` and the full `PlanarReference` construction (and N3B-P7). `EncodeReference` re-decodes on rank 0 (RIO:75) while the other ranks wait in `CollectiveFailure` (RIO:213).

**N3B-M5. The complexity result depends on the rank count through reduction order.** Low. Confirmed (by design).
Per-cell rules are partition-independent: node order is sorted by global index (CS:2888-2890). The local sums (CS:3075-3085) and `Allreduce` change with the partition in the last bits, which moves the global factor and so the metric slightly.

Checked, no finding: collective consistency of the new BL and curved paths. The quadrature failure `break`s only the local cell loop, and every rank then enters `CollectiveFailure` (CS:3114-3117). `remaining_time` is an `Allreduce` MIN (R3 diff). The `beforeAcceptance` result goes through `CollectiveFailure` (DRV:1709-1713). The BL count and work/time controls are voted (R3:160-172). Analytic primitive parameters, counts and orientations in `Coverage` are reduced (I3:205-226, 217). The Clearance batch count is an `Allreduce` MAX (E3:93). I found no early return that skips a collective on some ranks.

---

### 6. Strengths

- The B3 operators are all-or-nothing private proposals: every exception becomes a `false` with a reason, and the output is touched only on success.
- The analytic authority is validated up front: finiteness, kinds, axes, vertex-on-surface tolerance, chart horizon (sphere radial orientation, cylinder half-turn), fold checks (`CylinderFacing`), annulus Euler and link manifoldness for cylinders, border winding for disks and cylinder ends.
- `SurfaceError` is a conservative, analytically derived chord-to-surface bound for sphere, cylinder and bump, including the arithmetic envelope.
- The first-height certificate is independent of the constructor: altitude from the exact orientation, normal-foot barycentrics without rounding a projected point, and a final gate over every wall cell.
- The BL composition is evaluated at the actual query point from immutable original geometry, never interpolated from nodes, and the same `ApplySample` policy is used in the complexity integral and in the remesher.
- `WallMetric3D` inverts the tangent block in long double and keeps the exact logarithm for the fade, which avoids losing h0² in a 3D eigensolve.
- The sidecar format is exact and strictly validated.
- Collective failure handling stays uniform across all new paths.
- The CPoint empty-rank fix is correct.

---

### 7. Not verifiable statically

- Real costs of N3B-P1 to P8 and N3B-M1, and how often N3B-C1, C2, C4 and C5 occur on industrial meshes. These need runs, for example a 1e5-wall-face RANS case.
- Whether the engine's metric gates accept the first-row transition cells created by `ConstructFirstLayer`, and whether the joint reconstruction actually completes wall refinement within the work limit.
- Accuracy of the `IntegrationRule3D` angular and radial error estimator relative to the true complexity, and its behaviour at multi-wall corners (the chord direction comes from the first support face only, BL:1175-1192).
- `CADTElemClass` exactness for nearest-element queries on very anisotropic wall faces, and `convex_clip` capacity limits.
- Whether `ConformingDisjoint` in `Clearance` alone is enough to exclude folds of projected surfaces against unedited boundary parts, with no check of overlapping physical faces.
- Behaviour on platforms with a 64-bit or 128-bit `long double` (N3B-C11, the predicate filter).
- Unit tests were not run. The added fixtures (native_bl_cold, native_bl_corner, joint and strip obstruction) are small; none exercises more than 512 facets, more than 4 ranks, depth or point budgets of the quadrature, or the sidecar limit.

---

### 8. Counts

Old findings re-checked (my scope): 3D items C1, C2, C6, C9, C10, C11, C14, M6, M10, P9 → **0 fixed, 3 partly fixed (C1, C6, C10), 7 still open (C2 Critical; C9, M6 Medium; C11, C14, M10, P9 Low)**. Residual severity of the partly fixed ones: C1 Critical (sidecar), C6 High, C10 Medium. 2D C1, C2, P1, P2, P3, M1 → **all 6 still open** (1 Critical, 5 High).

New findings:

| Severity | Count | IDs |
|---|---|---|
| Critical | 2 | N3B-P1, N3B-M1 |
| High | 5 | N3B-C1, N3B-P2, N3B-P3, N3B-P4, N3B-M2 |
| Medium | 8 | N3B-C2, N3B-C3, N3B-C4, N3B-C5, N3B-P5, N3B-P6, N3B-P7, N3B-P8 |
| Low | 15 | N3B-C6 to N3B-C15, N3B-P9, N3B-M3, N3B-M4, N3B-M5, N2-1 |

**N2-1 (new 2D item). The 2D geometric BL quadrature aborts non-collectively.** Low. Plausible.
`IntegrationRule2DImpl` calls `SU2_MPI::Error` at depth 16 (BL:1791), inside the per-cell loop that CS now wraps in try/`CollectiveFailure` (CS:2871-2908). The abort bypasses that wrapper, so one rank calls MPI_Abort for the whole job. That code was unchanged since the baseline, but the baseline did not cover it. It is the same ridge or corner mechanism as N3B-C1; 2D halves the size per level, so the limit is reached only for L/h0 above about 5e4.

---

# Appendix A — Previous 3D report at 7746fd53fa (text of old findings)


Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch `codex/native-3d-transport-accounting`, HEAD `7746fd53fa`.
`git diff HEAD --stat --ignore-submodules -- Common SU2_CFD` is empty at the start and at the end of the assessment: tracked sources are clean.
Nothing was built, run or modified. All statements come from reading the code.

**Read in full** (`Common/include/adaptation`, `Common/src/adaptation`): CNativeMesh3D.hpp, CNativePredicates3D.cpp, CNativeTopology3D, CNativeCavity3D, CNativeBoundary3D, CNativeMetric3D, CNativeField3D, CNativeDonor3D, CNativeDistributed3D, CNativePartition3D, CNativeImport3D, CNativeReferenceIO3D, CNativeEngine3D, CNativeRemesher3D, CNativeRuntimeProfile3D.hpp, CRemesher.hpp, CSimplexMesh.hpp, and CNativeDistributed2D.hpp (it defines the `World` transport that 3D reuses).
There is no CNativeMesh3D.cpp and no CNativePredicates3D.hpp: the predicates are declared in CNativeMesh3D.hpp and implemented in CNativePredicates3D.cpp.
**Read as excerpts only**: CPassiveComm (hpp, AlltoallvRounds), CDistributedSearch (ElectFailure, OwnedSimplices), CADTElemClass::DetermineIntersectingElements, mpi_structure.cpp (Error), CSinglezoneDriver.cpp (MakeRemesher, ReplaceMesh, call sites), meson.build flags, the three design documents, test case titles.
**Not read**: CReaderSlices, CSolutionTransfer, CMeshGather, CTransferMemory.cpp, CRankBoxTree/CPointDirectory internals.

Severity scale: Critical = blocks use beyond toy meshes or aborts the run; High = wrong outcome or loss of the event in realistic cases, or a cost that prevents scaling; Medium = significant limit or waste; Low = minor.
Confidence: "Confirmed" = the code path was followed through its callers; "Plausible" = the mechanism is in the code but its frequency or cost is my estimate.

Findings per severity: **Critical 3, High 10, Medium 10, Low 10** (33 in total).

---

### 1. Architecture summary

**Data structure.** There is no classical tetrahedral mesh structure. The working mesh is `Owned = std::map<Id, OwnedCell>` (CNativeDistributed3D.hpp:26). Every cell stores its four vertices by value (id and coordinates, CNativeMesh3D.hpp:29-44), a version and four optional boundary bindings. There is no stored adjacency: local adjacency is rebuilt for each cavity by sorting (`Incidence`, CNativeTopology3D.cpp:44-116), and global vertex stars live in a hash-distributed `Directory` (`std::map<Key, std::map<Id, Entry>>`, placed on rank `hash(vertex) % P`, CNativeDistributed3D.cpp:73-81). Identifiers are 64-bit, never reused, and grow sparsely (`maximum + 1 + round*P + rank`, CNativeEngine3D.cpp:542-543, 558).

**Data flow of one adaptation event** (`CNativeRemesher3D::Remesh`, CNativeRemesher3D.cpp:86-442):

1. `CheckSupport`: tetrahedra only, triangular boundary faces, no periodic/multizone/moving mesh/adjoint, `ADAP_SURFACE= YES` required, any `ADAP_BL` rejected, `ADAP_HESSIAN_NOISE= 0` (lines 29-84).
2. Reference: load `<mesh>.native_ref` or build the immutable `PlanarReference` from the original boundary triangles (replicated on every rank).
3. `ImportGeometry`: owned tetrahedra into `Owned`, a frozen copy `original` (`DonorCell` = cell plus four nodal metric tensors), face rendezvous by hash to bind boundary faces to reference facets.
4. `DonorImport::Build` (ADT and rank boxes over the original cells), `WorkingWeights`, optional ParMETIS `Repartition` to M <= N worker ranks, donor migration, `MPI_Comm_split`.
5. `Refine`, then `Coarsen` (same function `Adapt`, CNativeEngine3D.cpp:289-911) on the worker communicator. Each builds its own `Directory` and `DonorImport`, measures every cell ("seed"), then runs globally synchronous rounds. In one round each rank proposes at most one cavity: Lookup (complete edge/vertex star) -> Fetch (remote cells) -> Reserve (vertex claims, lowest rank wins) -> private operator (SplitEdge / InsertEdgeCavity / CollapseVertex / MoveVertex / Reconnect, CNativeBoundary3D.cpp) -> donor import -> metric gate (CNativeMetric3D.cpp) -> routing -> atomic `Publish` -> re-measure the new cells.
6. `ReaderSlices`: nodal metric by P1 query of the frozen source, renumbering through `CReaderSlices::FromDistributed`. The driver then rebuilds the geometry and transfers the solution with the shared `CSolutionTransfer`.

**Metric.** The target is the *original* nodal tensor field, linearly interpolated inside original tetrahedra (`FrozenField`). Edge length is the exact piecewise integral of sqrt(e^T M e) along the edge through the original cells (CNativeField3D.cpp:127-182). Gates: edge <= 1.8, collapse candidates < 0.9, mean ratio >= 0.20, scaled Jacobian >= 0.05 (CNativeMetric3D.hpp:12, CNativeEngine3D.hpp:11, CNativeMesh3D.hpp:82-83).

**What "donor" and "transport accounting" mean here.** "Donor" is the frozen metric source, not the CFD solution. No solution transfer or conservation logic exists in the 3D native files; that is in `CSolutionTransfer` (not assessed). "Transport accounting" is MPI byte/call/time bookkeeping (`World::bytes_sent`, `collective_calls`, `collective_seconds`).

**Acceptance model.** All-or-nothing: an event is accepted only if every cell passes every gate and, for coarsening, the candidate queue is empty. Any budget refusal rejects the whole event.

---

### 2. Correctness findings

#### C1 — Critical, Confirmed: the original boundary is limited to 512 triangles and to one closed sphere
`CNativeImport3D.cpp:361-367` calls `ValidateSurfaceTopology(skin, true)` on *all* boundary facets of the first mesh. `CNativeCavity3D.cpp:130-133` throws when `triangles.size() > 4 * MaximumReplacementCells` (4 x 128 = 512, CNativeCavity3D.hpp:13). The same check runs when the sidecar is decoded (`CNativeReferenceIO3D.cpp:88`). With `closed = true`, `Surface()` (CNativeCavity3D.cpp:55-87) also requires one connected component with Euler characteristic 2.
Failure scenario: any mesh with more than 512 boundary triangles, or with two boundary components (body inside a far-field box), or with a handle, ends in `CollectiveFailure` (fatal) at the first import (CNativeImport3D.cpp:371).
The largest case in the project notes has exactly 512 boundary triangles (MESH_ADAPTATION_FINDINGS.md:1305), which matches this cap. This alone rules out 1e6-1e7 tetrahedra and external aerodynamics.

#### C2 — Critical, Confirmed: a rejected event aborts the CFD run
`Remesh` returns `INCOMPLETE_QUALITY` with empty slices on any refusal (CNativeRemesher3D.cpp:90, 291-313) and prints "accepted CFD mesh/solution retained". The driver passes the result straight to `ReplaceMesh`, which calls `CollectiveFailure` -> `SU2_MPI::Error` when the status is not `COMPLETE` (CSinglezoneDriver.cpp:1676-1680; call sites 432/444, 997/1005, 1504 with 1435-1442).
So the "atomic rejection" protects in-memory state only for unit tests. In a run, each of the following stops the job: C3, C4, C5, C7, C8, M1, a memory-ceiling refusal (P6), or a metric evaluation failure in a single cell (`Selection::Add` throws on an incomplete measurement, CNativeEngine3D.cpp:226-227, 442-449).

#### C3 — Critical, Confirmed: hard limit of 512 rounds per pass, one attempt per rank per round
`EngineOptions::maximum_rounds = 512` (CNativeEngine3D.hpp:13). The driver constructs `CNativeRemesher3D(nativeReference3D)` with default options (CSinglezoneDriver.cpp:299); there is no config option (only `ADAP_NATIVE_RANKS` and `ADAP_NATIVE_REPARTITION`, CConfig.cpp:3307-3308). Each round holds one `Choice` per rank (CNativeEngine3D.cpp:458-470), and rejected proposals also consume the round. At the end of the loop the result is invalid unless everything is complete (lines 897-909).
Consequence: at most 512 x P attempted edits in `Refine` and again in `Coarsen`; in serial at most 512. A mesh that needs more is rejected, and by C2 the run aborts.
Corroboration from the project notes: the 1536-tetrahedron wedge fails at the default budget (MESH_ADAPTATION_FINDINGS.md:868-873).

#### C4 — High, Confirmed: a valid mesh is rejected when coarsening does not finish
`incomplete == 1` ("valid cached metric cells and pending collapse candidates") still sets `result.valid = false` (CNativeEngine3D.cpp:899-903). Commit c407d8b9a4 changed only the message. In `Remesh` the coarsening result replaces `valid` (CNativeRemesher3D.cpp:247-252), so a completed refinement is discarded as well.
Scenario: refinement succeeds, all cells pass all gates, 91 optional collapse directions remain at round 512 -> event rejected -> abort. The notes describe exactly this case.
Coarsening is optional by the engine's own contract ("need not reach a minimal mesh", CNativeEngine3D.hpp:73-78), so this refusal is not needed for mesh validity.

#### C5 — High, Confirmed mechanism (frequency Plausible): a star with more than 64 cells rejects the whole event
`Directory::Lookup` returns status 3 when the matches exceed `MaximumCavityCells = 64` (CNativeDistributed3D.cpp:274-289); the caller throws (lines 305-310) and the vote fails on all ranks. The engine then returns instead of blocking that candidate (`CNativeEngine3D.cpp:525-527`).
Scenario: the shortest collapse candidate removes a vertex with 65 incident tetrahedra, or the repair queue contains such a vertex. Transition vertices next to bisected regions gain two cells per split of an opposite edge, so valence above 64 is realistic. `FilterCollapse` (lines 140-170) scans the local star but does not test its size.

#### C6 — High, Confirmed mechanism: new boundary points must be exactly on the edge or exactly coplanar; no reprojection, no curved surfaces
The split point is the rounded midpoint (CNativeEngine3D.cpp:558-559). `SplitEdge` needs exact collinearity (`OnEdge`, CNativeBoundary3D.cpp:34-39, 364-367). The fallback `InsertEdgeCavity` needs exact coplanarity with the reference facet (`PlanarReference::Contains`, lines 319-322, used at 449) and refuses an edge between two different facets (lines 439-440).
Consequences:
- Axis-aligned planes and axis-aligned ridges work with any coordinates.
- On an oblique plane a rounded midpoint is not exactly coplanar unless the coordinates are dyadic. The wedge cases in the notes use dyadic coordinates (MESH_ADAPTATION_FINDINGS.md:1305).
- On a faceted curved surface every original boundary edge is a ridge between two facets, so only the exact `SplitEdge` path remains.
A boundary edge that is too long and cannot be split leaves an invalid cell; the event stagnates (lines 509-513) and is rejected. The documents place curved geometry in "Goal 2, not started", so this is a declared scope limit, but the code does not detect it up front.

#### C7 — High, gate Confirmed, thresholds Plausible: the strict shape gate on every intermediate edit blocks multi-level anisotropic refinement
`MeasureMetricCells(fresh, ..., require_shape = true)` throws when any new cell has mean ratio < 0.20 or scaled Jacobian < 0.05 in the target metric (CNativeMetric3D.cpp:64, 101-109, 189-190). Old cells may be worse; new ones may not.
With the formula in `Measure` (CNativePredicates3D.cpp:482-486) a regular tetrahedron under a metric stretched by a factor s has q = 3 s^(2/3) / (2 + s^2): 0.42 at s = 4, 0.21 at s = 7, 0.18 at s = 8, 0.07 at s = 16. One bisection cannot lift q from 0.07 to 0.20.
Scenario: an isotropic mesh with a target anisotropy mismatch above roughly 7-8 (three or more bisection levels in one direction). The first splits are rejected, the repair fallback passes through the same `Shape` check (CNativeMetric3D.cpp:154, 189-190), the edge is blocked, and the event stagnates (CNativeEngine3D.cpp:509-513). The reported tests use aspect ratios 1.5 to 2.

#### C8 — High, Plausible: the 256-donor cap per cell limits the coarsening ratio per event
One cell's metric queries (centroid and six closed edges) must be covered by at most `MaximumDonors = 256` original tetrahedra (CNativeDonor3D.cpp:344, 365; CNativeField3D.cpp:35; CNativePredicates3D.cpp:334). When the one-cell import still exceeds the cap, the event is rejected (CNativeEngine3D.cpp:414-421, 687).
The exact filter keeps every donor that *touches* a segment, including all tetrahedra around its end vertices and around every original vertex it passes through. Estimate for a Kuhn-type source (24 cells per vertex): an edge spanning k original edges touches about 24 + 16k cells; six edges of a cell with k = 2 give about 250, with k = 4 about 350. So coarsening by more than about a factor 2 relative to the source mesh in one event is expected to hit the cap. Not measured.

#### C9 — Medium, Confirmed: boundary coarsening is limited to convex coplanar same-marker patches
Coplanar facets are merged only when the component is a disk with a convex border (CNativeBoundary3D.cpp:153-228). A non-convex planar region, or one with a hole (symmetry plane cut by a body), keeps one facet per original triangle ("retain raw constraints", line 224). The flood fill marks all its facets as seen, so no convex sub-patches are tried.
Then `CollapseVertex` needs the retained vertex inside every facet incident to the removed one (lines 585-594), which fails for original boundary vertices, and `InsertEdgeCavity` refuses edges between two raw facets (439-440). Corners and marker junctions are preserved correctly by the same rule.

#### C10 — Medium, Confirmed: operator set and features are narrower than the task list assumes
- No in-sphere predicate, no Delaunay kernel, no cavity enlargement. Insertion is a cone over the fixed edge star (`Cone`, CNativeCavity3D.cpp:163-190).
- Swaps exist only as "cone to an existing vertex" (`Reconnect`, CNativeBoundary3D.cpp:726-744), and smoothing only as a move toward the neighbour centroid with steps 1, 0.5, 0.25 (CNativeEngine3D.cpp:606-632). Both run only as repair after a rejection, not as an optimisation pass.
- No gradation in these files; no metric repair (a non-SPD nodal tensor is fatal at import, CNativeImport3D.cpp:207-217).
- No boundary-layer or frozen regions: `ADAP_BL` and `ADAP_SURFACE= NO` are rejected (CNativeRemesher3D.cpp:77-80).
- Tensor interpolation is component-wise linear, not log-Euclidean. It is self-consistent with the exact edge integral.

#### C11 — Low, Plausible: the rounded centroid of a boundary sliver can fall outside the source
`Centroid` rounds to binary64 (CNativePredicates3D.cpp:287-295); `FrozenField::Query` uses exact closed containment and throws "Query outside" (CNativeField3D.cpp:230). In the seed or update this becomes an event rejection (CNativeEngine3D.cpp:448).

#### C12 — Low, Confirmed: `UndoEdgeSplit` is dead in production
CNativeBoundary3D.cpp:484-573 has no caller outside the unit tests.

#### C13 — Low, Plausible: the floating-point filter is compiled with `-ffast-math`
meson.build:101 adds `-ffast-math -fno-finite-math-only`. `SegmentMayIntersectBox` protects its rounding points with `volatile` (CNativeDonor3D.cpp:72-76); the main filter in `Determinant` (CNativePredicates3D.cpp:136-156) does not. Any regrouping of six triple products stays far inside the 64-epsilon bound by my analysis, and the exact path is integer-only, so I expect no wrong sign. It is not proven for every compiler transformation.

#### C14 — Low, Confirmed: documentation is partly stale
NATIVE_ADAPTATION.md:34 and :211 say that 3D is rejected / unsupported, while the driver routes 3D to `CNativeRemesher3D` (CSinglezoneDriver.cpp:289, 298-299). HANDOFF_Codex.md is consistent with the code on the points I checked (rotating policy "not promoted", default-512 failure, Goal 2 not started).

#### Checked and found correct (no finding)
- Exact arithmetic: limb budgets (66/132/198/396 limbs) cover the worst case; `Product` carry placement, `Difference` sign handling and the magnitude-only `CompareRoot` (all roots are non-negative) are right.
- Cone, collapse and move: strict positive orientation of every new cell against an unchanged embedded skin implies that the apex is in the kernel, so the new cells tile the cavity. External edge/face collisions are excluded geometrically.
- `SegmentMayIntersectBox` and `Outside` are conservative (they only discard certain misses).
- The edge integral formula (2/3) L (1 + r + r^2)/(1 + r) is the correct integral of the square root of an affine function.
- Query bit masks between sender and owner match (CNativeDonor3D.cpp:277-287, 347-352).
- Direction indices of the collapse filter are consistent between `Queue`, `Forbidden` and `Touch`.
- Loops are bounded everywhere; no index reuse; 64-bit id overflow is guarded (CNativeEngine3D.cpp:549-551).

---

### 3. Performance findings

#### P1 — High, Confirmed structure: one cavity per rank per global round, about 165-180 MPI collectives per round
Counting the code path of one refinement round with one measure batch: 47 votes/reductions plus 14 `World::exchange` calls. Each exchange is four failure elections, one `MPI_Alltoall` of counts, one reduction, and `AlltoallvRounds` with a second `MPI_Alltoall`, one reduction and at most one `MPI_Alltoallv` (CNativeDistributed2D.hpp:108-181, CPassiveComm.cpp:97-165). Total about 131 `MPI_Allreduce`, 28 `MPI_Alltoall`, up to 14 `MPI_Alltoallv`. Coarsening adds the change notification.
The yield is at most one accepted cavity per rank. Idle ranks take part in every collective. The rounds needed equal the edit count of the busiest rank, not total/P.
Project-reported rates for small cases are 26 to 183 edits/s (HANDOFF_Codex.md:1557, 2630). At 75 edits/s, 1e6 edits take about 3.7 hours before any scaling loss.

#### P2 — High, Confirmed: full-mesh passes are repeated several times per event
- Two complete seeds: `Refine` and `Coarsen` each build a new `Selection` and re-measure every cell, including exact integration of every edge (CNativeEngine3D.cpp:301, 454-456; called at CNativeRemesher3D.cpp:238, 249).
- `DonorImport::Build` runs four times (CNativeRemesher3D.cpp:176-177; CNativeEngine3D.cpp:392 twice; CNativeImport3D.cpp:529-531). Each sends six authority records per original cell through a hash exchange and checks them in a `std::map` (CNativeDonor3D.cpp:163-211).
- `Directory::Build` runs two or three times (CNativeEngine3D.cpp:389 twice; CNativePartition3D.cpp:147-149).
- `WorkingWeights` always runs, also when no repartition is requested and the weights are then unused (CNativeRemesher3D.cpp:181-190): five point queries per cell and N/8 collective import batches.
- The mesh map is deep-copied at the start of each pass (`working = accepted`, CNativeEngine3D.cpp:374).

#### P3 — High, mechanism Confirmed, cost Plausible: every metric edge is traced with exact rational arithmetic
`SensorEdgeLength` -> `TraceSegment` (CNativePredicates3D.cpp:333-387). The only floating-point shortcut is the strict separation test (lines 345-353). Every donor that touches the segment goes to `ClipInterval` (lines 228-260), which computes the exact volume and up to eight more exact 3x3 determinants on up to 6300-bit integers, then root comparisons with 396-limb products.
For an edge of the unchanged mesh about 40-50 source cells touch the segment (all cells around both end vertices), most of them only to be discarded as zero-length. My estimate is 100-400 exact determinants and 0.1-1 ms per edge; not measured. A mesh has about 1.2 edges per tetrahedron and they are all traced in each seed.
`ValidateTensor` (two filtered determinants) also runs inside every `MetricNorm` call (line 399), twice per piece.

#### P4 — Medium, Confirmed: the same edges are integrated two to three times per accepted edit
- The measurement from validation is discarded (`PatchMetric metric` local at CNativeEngine3D.cpp:598). The update re-measures the new cells with reuse limited to cached and retired edges (lines 435-442), so every new edge is traced again.
- `MeasureMetricCells(old, target, stats, &measured.edges)` (CNativeMetric3D.cpp:191) reuses only edges of the fresh patch. Edges that exist only in the old cavity (the split edge, all spokes of a collapsed vertex) are traced again although the caller holds them in `frozen_reuse`.

#### P5 — Medium, Confirmed: node-based containers and repeated local rebuilds in the hot path
- `std::map` for the mesh, the cell cache, the edge cache and vertex stars; `std::set` for the queue (CNativeEngine3D.cpp:83-90). Every access is O(log N) with poor locality.
- One proposal builds `Incidence` three to six times (operator, `ValidatePrescribedInterface`, `Validate`), each with an orientation test of all cells, and `Ball` builds a map of graphs per vertex twice (CNativeCavity3D.cpp:88-103, 108-127).
- `Directory::Prepare` copies the whole star map of every changed vertex before each publication (CNativeDistributed3D.cpp:152-155).
- `GeometryPatch`, `Nodes` and `SkinMap` are rebuilt for every repair trial (up to 19 per cavity).
No O(N^2) stage and no per-operation whole-mesh validation was found. The cost is in constants and in the round structure.

#### P6 — Medium, formulas Confirmed, numbers estimated: memory footprint and the fixed 2 GiB ceiling
Resident storage is about 2.5-3 kB per tetrahedron: two copies of the 248-byte map node, selection caches of about 0.5-0.9 kB, the directory share of about 0.7 kB (five map entries per cell), the donor copy of 328 bytes plus boxes and ADT, and a second donor copy after repartition.
Admission charges 2048 to 6144 bytes per cell for the selection alone (`selection_bound`, CNativeEngine3D.cpp:306, 371-372, 425). With `memory_ceiling = 2 GiB` (CNativeEngine3D.hpp:13, not configurable from the driver) I estimate a limit of 0.3-0.5 million tetrahedra per rank. Beyond it the event is refused collectively, and by C2 the run aborts.

#### P7 — Medium, Confirmed: metric queries are served in collective batches of at most 16 cells
The seed loop (CNativeEngine3D.cpp:397-453) and `Samples` (CNativeImport3D.cpp:414-465) perform, per batch, a collective donor import (seven votes, two exchanges) and build a new `FrozenField` with a new `CADTElemClass` (CNativeField3D.cpp:61-111). For N/P = 1e5 cells this is about 6000 batches per seed, twice per event, plus the weight and return passes.

#### P8 — Low, Confirmed: `Repartition` builds a full `Directory` only as a validation
CNativePartition3D.cpp:147-155; the dual graph comes from a separate face rendezvous.

#### P9 — Low, Confirmed: the planar reference is map-based and replicated
Five `std::map` members (CNativeBoundary3D.hpp:55-59), a convexity loop over border edges times vertices per component (CNativeBoundary3D.cpp:219-223), and an admission model of 4096 bytes per facet (line 269). Not relevant under the 512-triangle cap of C1, but it becomes relevant when that cap is lifted.

---

### 4. MPI readiness findings

**What is distributed.** Import, donor search, directory, engine, partition and return all run distributed; no rank gathers the volume mesh. Not distributed: the boundary reference (replicated on every rank, M6), the sidecar I/O (rank 0), and the boundary faces of the returned mesh (gathered on the master, M10).
**Interfaces.** Partition interfaces are not frozen. A cavity may span ranks; its cells are fetched from their owners and replaced atomically. There are no halo cells. Ownership does not migrate during a pass.

#### M1 — High, Confirmed by reading, not reproduced: single-cavity budgets are applied to the per-rank sum over all concurrent cavities
`Directory::Publish` refuses when `removed.size() > 64` or `added.size() > 128` (CNativeDistributed3D.cpp:504-506). In the engine these vectors are everything routed to this rank from *all* proposers of the round (`world.exchange(removals)`, `world.exchange(additions)`, CNativeEngine3D.cpp:849-851). The same holds for `retired_edges.size() > 6 * 64` (lines 870-874), which is checked after the publication has been committed.
Scenario: in a coarsening round rank X collapses a vertex with 24 cells, and four neighbour ranks each collapse an interface vertex whose star holds 12 cells of X: 72 > 64 -> publication refused -> event rejected (C2).
The probability grows with the number of neighbour ranks. The tests stop at four ranks.

#### M2 — High, Confirmed: every exchange is dense in P
`World::exchange` takes `std::vector<std::vector<T>>(P)` buckets and performs two `MPI_Alltoall` calls of P counts and five to six reductions even when nothing is sent (CNativeDistributed2D.hpp:108-181; CPassiveComm.cpp:97-125). Callers also allocate P-sized arrays per call (`selected`, `payload`, `to`, `reply`; e.g. CNativeDonor3D.cpp:250-255, 309, 354).
With about 14 exchanges per round the work is O(P) per rank and O(P^2) in total per round, independent of activity. There is no neighbour-only or sparse (NBX-type) exchange. Acceptable at 4-64 ranks, a hard wall at hundreds to thousands.

#### M3 — Medium, Confirmed: the reservation gives fixed priority to the lowest rank; "rotating reservations" are not in the production code
`Directory::Reserve` takes `std::min(old->second, row.caller)` (CNativeDistributed3D.cpp:428-434). The rotating policy of commits 2c20503dce to 1bfb136dbd exists only as `integration_evidence/native_3d_core_v1/rotating_reservation_*/directory_rotating.cpp`; its difference to production is a `first_rank = (stats.reservations - 1) % size` rotation carried in the claim. No file under `Common/` changed in those commits.
Effects in production:
- A higher rank loses every interface conflict. It keeps its candidate and retries next round, so there is no livelock, but each lost round is taken from the shared 512 budget (C3).
- Vetoes are transitive: a caller that loses a vertex to another caller is refused even if that other caller is itself vetoed (line 434).
- The lowest active rank can never be vetoed, so at least one proposal proceeds per round.
Note on the probe: the rotation is keyed on a statistics counter. It works because every rank calls `Reserve` the same number of times, but it couples the protocol to a diagnostic field.

#### M4 — Medium, Confirmed: no load balancing during a pass
New cells go to the owner of their parent (CNativeEngine3D.cpp:815-836). Repartition is optional, runs once before the passes, uses static predicted weights, and is off by default (CNativeRemesher3D.cpp:181). Local refinement (a shock inside one partition) therefore runs at serial speed with parallel overhead, and that rank can reach the memory ceiling (P6).

#### M5 — Medium, Confirmed: the hash directory makes every operation remote
The star of a vertex lives on `hash(v) % P`, unrelated to ownership (CNativeDistributed3D.cpp:73-81). A cavity that lies fully inside one rank still needs remote lookup, reservation and publication. There is no local fast path, and the directory memory of a rank does not follow its mesh share.

#### M6 — Medium, Confirmed: the boundary reference is replicated with P copies on the sender, and the sidecar is capped at 1 MiB
`World::metadata` builds `std::vector<std::vector<T>>(size, local)` (CNativeDistributed2D.hpp:183-186), used for the facets at CNativeImport3D.cpp:358 and admitted as `P * count * 3 * sizeof(Facet)` (lines 339-343). `ReferenceFileLimit3D = 1 MiB` (CNativeReferenceIO3D.hpp:11, .cpp:48-49) holds about 4000 text facets; writing beyond it throws on rank 0 and is fatal. Hidden today by C1.

#### M7 — Low, Confirmed: results depend on the rank count
Identifiers contain `round * P + rank` (CNativeEngine3D.cpp:542-543), the reservation depends on the rank, and the edit order depends on the partition. For a fixed rank count and binary I found no source of nondeterminism: ordered containers throughout, the `unordered_map` is a pure lookup cache with FIFO eviction, donor ties are broken by canonical keys, ParMETIS has a fixed seed. The documents state that identity across partitions is not required.

#### M8 — Low, Confirmed: the transport accounting counts only part of the MPI traffic
`collective_calls` and `collective_seconds` are incremented for the count `Alltoall`, for `AlltoallvRounds` (as one call) and for `sum`/`maximum` (CNativeDistributed2D.hpp:133-135, 165-167, 189-195). Not counted: every `ElectFailure` (about 100 of the roughly 170 collectives per round), direct `CPassiveComm::Allreduce` calls in the engine, `LoadReference`, `CReaderSlices::FromDistributed`, ParMETIS. `bytes_sent` includes self-traffic. The code comments say so (CNativeEngine3D.hpp:30, CNativeImport3D.hpp:26); the numbers must not be read as total MPI cost. I found no double counting between the parent, import and worker scopes.

#### M9 — Low, Confirmed: unguarded exceptions between collectives abort the job; no deadlock path found
Examples outside the try blocks: `selection.Best` -> `edges.at` (CNativeEngine3D.cpp:462), bucket allocations (804-806), `reserve` and decoding inside `exchange` (CNativeDistributed2D.hpp:159-179), `.at()` in the donor migration (CNativeRemesher3D.cpp:203-204). They reach `SU2_MPI::Error` (CNativeRemesher3D.cpp:262-263) or `std::terminate`, so the job dies instead of hanging.
I followed every early return in Lookup, Fetch, Reserve, Publish, ImportRegions, Samples, Repartition, LoadReference and the engine loop: each depends on an elected (collective) result or is followed by no further collective. Batch sizes are derived from reduced values. The collective refusal is consistent.

#### M10 — Low, Confirmed (outside the listed files): the master rank gathers all boundary faces of the returned mesh
`CReaderSlices::FromDistributed`, called from `ReaderSlices` (CNativeImport3D.cpp:588), collects the boundary connectivity on `MASTER_NODE` with `GathervRounds` (CReaderSlices.cpp:608-634). This follows the SU2 reader convention. It is a serial memory and time term proportional to the global boundary size, small next to the volume mesh.

#### Checked, no finding: integer ranges
MPI counts go through `CheckedInt` and 1 GiB rounds (CPassiveComm.cpp:86-165); graph sizes are checked against `idx_t` (CNativePartition3D.cpp:143-146); identifiers are 64-bit. No `int` overflow was found in the 3D files.

---

### 5. Mechanisms of the last 20 commits

| Mechanism | Where | Result |
|---|---|---|
| Transport accounting | CNativeDistributed2D.hpp:99-102; CNativeRemesher3D.cpp:316-322 | Consistent but partial (M8). It is not solution-transfer accounting. |
| Donor prefilters: packed per-owner queries, exact cell and segment filters, segment/box separation (7b4e0d767f, 7f76fd9a27, 8c3b40c9be) | CNativeDonor3D.cpp:64-108, 266-289, 317-352 | Conservative and correct by reading; bit masks consistent. They do not remove the exact clipping cost (P3) or the 256 cap (C8). |
| Rotating reservations | only under `integration_evidence/` | Not in production (M3). |
| Coarsening work limits (c407d8b9a4) | CNativeEngine3D.cpp:897-909 | Diagnostic text only; the event is still rejected (C4). |
| Collapse prefilter and re-activation | CNativeEngine3D.cpp:113-170, 200-223 | Correct as a prefilter: a local counterexample is sufficient and the operator remains authoritative. |

---

### 6. Strengths

- Predicates: exact signs through bounded integer arithmetic that compiler floating-point flags cannot change, with a static filter in front; closed containment without tolerances; exact SPD test by Sylvester signs.
- Every edit is built privately and validated before commit: ball topology with manifold vertex links, identical oriented skin, strict positive volumes, exact facet containment for boundary faces.
- Publication is staged: directory changes and mesh changes are prepared, voted, then committed without allocation (CNativeDistributed3D.cpp:492-536). The accepted input is `const` and untouched on failure.
- Failure handling is collective and uniform (`ElectFailure`), with memory admission before allocation.
- Markers, corners and ridges cannot drift: boundary motion is admitted only inside every incident reference facet; marker names are compared at each import.
- The metric source is frozen on the original mesh, so repeated edits do not diffuse it; the edge length is integrated exactly for the P1 field.
- Deterministic containers and canonical tie-breaks; bounded loops; explicit id-range overflow guards.
- No master gather in the engine; worker subset M <= N; idle ranks wait with a sleeping `MPI_Test` loop instead of spinning.
- Wide unit-test coverage at component level (about 100 test cases), and project notes that report failures and limits openly.

**Coverage gaps seen from the test titles:** more than 4 ranks; stars above 64 cells inside an engine run; several cavities converging on one owner (M1); boundaries above 512 triangles or with two components (C1); oblique boundaries with non-dyadic coordinates (C6); anisotropy above about 2 (C7); events that need more than 512 rounds as an accepted outcome.

---

### 7. Not verifiable statically

- How often C5, C7, C8 and M1 occur on real meshes, and the true cost per edge of P3. These need runs.
- All timings and the memory per cell including allocator overhead; my figures come from `sizeof` and the admission formulas.
- Whether `-ffast-math` changes any filter decision with the compilers in use (C13).
- Behaviour of `CADTElemClass` box inflation, `CRankBoxTree`, `CPointDirectory` and `BisectionBoxes`; I assumed they are conservative, as their comments say.
- `CReaderSlices::FromDistributed` (consistency of nodal metric on nodes shared between ranks, renumbering) and `CSolutionTransfer` (conservation). Neither was read.
- ParMETIS determinism and its behaviour with very uneven vertex counts.
- The interplay of the non-collective `SU2_MPI::Error` (one-second `MPI_Ibarrier` probe on the same communicator on which idle ranks wait in their own `MPI_Ibarrier`, CNativeRemesher3D.cpp:273-279) across MPI implementations. By reading it ends in an abort, not a hang.
- Whether any production path passes non-default `EngineOptions`. I found only CSinglezoneDriver.cpp:299.

---

# Appendix B — Previous 2D report at 7746fd53fa (still valid: 2D unchanged)


Worktree: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch `codex/native-3d-transport-accounting`, HEAD `7746fd53fa`.
`git diff HEAD --stat --ignore-submodules -- Common SU2_CFD` is empty: tracked sources are clean.
Method: static reading only. Nothing was built or run. No repository file was modified.

Files read in full: `Common/include/adaptation/` CNativeMesh2D, CNativeEngine2D, CNativeBoundary2D, CNativeField2D,
CNativeDistributed2D, CNativeImport2D, CNativeReference2D, CNativeGeometryValidation2D, CNativeBalanceProfile2D,
CNativePartition2D, CNativeReferenceIO, CNativeRemesher, CRemesher, CSimplexMesh (.hpp); `Common/src/adaptation/`
CNativeRemesher.cpp, CNativePartition2D.cpp, CNativePredicates2D.cpp (no header; declared in CNativeMesh2D.hpp:59-61).
There is no other `CNative*2D` source (no separate 2D scaling file; only the hidden unit test `CNativeScaling2D_tests.cpp`).
Read in part, to check callers: CTransferMemory.{hpp,cpp}, CPassiveComm.cpp (AlltoallvRounds), CDistributedSearch.cpp
(ElectFailure/CollectiveFailure), CReaderSlices.cpp (BuildDistributedSlices), CSinglezoneDriver.cpp (MakeRemesher,
ReplaceMesh, AdaptMesh), CBoundaryLayerMetric.cpp (ApplyPoint/Evaluate), meson files, CConfig option defaults.

Path shorthand below: `Mesh2D` = Common/include/adaptation/CNativeMesh2D.hpp, `Engine2D`, `Boundary2D`, `Field2D`,
`Distributed2D`, `Import2D`, `Reference2D`, `GeomVal2D`, `Partition2D.cpp`, `Remesher.cpp`, `Predicates2D.cpp` likewise.

---

### 1. Architecture summary (data flow)

1. **Import** (`Remesher.cpp:85-153`, `Import2D:59-167`). Each rank turns its owned CFD triangles into value-type
   `Cell` records (`Boundary2D:36-47`): three `Node`s stored by value (global id, coordinates, fixed flag), three edge
   markers, a version, cached quality and edge lengths, three nodal target tensors. Node ids are CFD global indices,
   cell ids come from an exclusive prefix sum. Halo coordinates and metrics are fetched through a `CPointDirectory`.
   Volume edges and boundary labels are hashed to an owner rank to find physical faces and check manifoldness.
2. **Reference** (`Reference2D`, `GeomVal2D`, `CNativeReferenceIO.hpp`). All physical faces are replicated on every
   rank and chained into directed polyline components (`PolylineReference`). This original piecewise-linear geometry
   is the only boundary geometry; it is persisted in a `.native_ref` sidecar. Boundary points are placed on it by
   arclength; deviation is measured against it.
3. **Optional working partition** (`Partition2D.cpp`). Off by default (`ADAP_NATIVE_REPARTITION= NO`). A dual graph
   is built through hash owners and partitioned with ParMETIS on predicted work weights; whole cells migrate.
   `ADAP_NATIVE_RANKS < size` moves all cells to the first ranks, which then work on a split communicator.
4. **Engine** (`Engine2D`). There is no mesh data structure with adjacency. State per rank: `std::map<Id,Cell> owned`,
   an immutable copy of the input cells as metric donor (`DonorField`, with a local ADT), and a distributed hash table
   node id -> incident (cell, version, owner) (`Directory`, `Distributed2D:250-486`, rank = hash(id) mod P).
   Work is a sequence of collective **rounds**. In one round every rank proposes at most one cavity operation
   (`Engine2D:84-86`). The round: look up the stars of the seed nodes in the directory, fetch the cells from their
   owners, reserve all cavity nodes (lowest rank wins a conflict), import the donor cells that cover the cavity,
   allocate fresh ids, rebuild the cavity privately (`Boundary2D::reconstruct` -> `Mesh2D`), exchange perimeter
   certificates, let every participant validate, vote, then publish erase/add on all owners and update the directory.
   Operators: surface HEIGHT / SPLIT / REMOVE / REDISTRIBUTE (re-triangulate the cavity polygon, carve first-layer
   wall triangles at the prescribed height) and bulk REMOVE / SPLIT / FLIP / MOVE, plus a "joint" two-point repair.
   `adapt()` runs 6 sweeps of the 8 phases; a phase is at most `phase_rounds` rounds (`Engine2D:618-681`).
5. **Metric**. The nodal sensor tensor is frozen on the input mesh. Every query interpolates it P1 (linear in M) on
   the imported donor cells, then applies an optional point-wise composition (BL wall constraints,
   `CSinglezoneDriver.cpp:304-330`). Cell quality uses one centroid sample, edge length a 3-point Simpson rule.
6. **Acceptance** (`Remesher.cpp:499-526`). Fixed contract: quality >= 0.18, metric edge length <= 1.8, reference
   deviation <= ADAP_HAUSD, wall height within 1e-8. Otherwise the status is INCOMPLETE.
7. **Output** (`Import2D:185-239`, `CReaderSlices.cpp:303-647`). Each rank builds reader slices from its cells with a
   distributed dense renumbering. The volume mesh is never gathered. Solution transfer is the existing driver code
   (`CSolutionTransfer`), not part of the native kernel.

---

### 2. Correctness findings

Each entry: location, scenario, severity, confidence.

**C1. The work allowance caps one remesh call at about 120 000 bulk insertions per rank.** Critical. Confirmed.
`Remesher.cpp:421-422`: `phase_rounds = min(20000, max(300, ceil(max(globalCells, estimatedCells)/(4*ranks))))`.
`Engine2D:60`: `sweeps = 6` (never set by the remesher). `Engine2D:618-635`: a phase runs at most `phase_rounds`
rounds, and a rank commits at most one cavity per round, successful or not. So a rank can do at most 6 x 20000 bulk
splits per call (+2 cells each). Example: 50k cells to a 1e6-cell target on 4 ranks needs about 475k splits; the
ceiling is 480k only if every round commits on every rank with no conflict, rejection or imbalance. A 1e7-cell target
needs more than 40 perfectly balanced ranks. Below the 20000 cap the formula still gives only a factor 3 margin and
assumes operations are spread evenly over ranks: a rank that needs more than about 3/P of all insertions (a shock in
one partition, repartition off by default) runs out of rounds. The result is INCOMPLETE_QUALITY, which is fatal (C2).

**C2. An incomplete result terminates the CFD run.** High. Confirmed.
`SU2_CFD/src/drivers/CSinglezoneDriver.cpp:1676-1680`: any status other than COMPLETE goes to `CollectiveFailure`,
which calls `SU2_MPI::Error` (`CDistributedSearch.cpp:460-463`). All callers (`:444`, `:1005`, `:1435-1442`, `:1861`)
go through this. One residual cell with q < 0.18 or one edge with L > 1.8 after the bounded work ends the simulation.
The thresholds are hard-coded (`Engine2D:646-647`, `Remesher.cpp:506`) and there is no relaxation, retry with more
rounds, or "keep the old mesh and continue". NATIVE_ADAPTATION.md says the candidate is "rejected before replacing
the accepted CFD mesh"; the code then stops the run.

**C3. With ADAP_SURFACE= NO a metric finer than the boundary can never be satisfied.** Medium. Confirmed.
`Engine2D:193`: `fixed_boundary` skips all surface SPLIT/REMOVE/REDISTRIBUTE candidates. `Engine2D:177` still
proposes every boundary edge longer than 1.6 as BULK_SPLIT; `Mesh2D:482-485` rejects it ("not a complete interior
edge star") because a boundary edge has one cell. Each such edge costs one full collective round per sweep, and
because candidates are ordered by score the longest boundary edges are tried first. `Engine2D:646-647` and
`Remesher.cpp:504-508` count the edge as a residual, so the call ends INCOMPLETE and the run aborts (C2).

**C4. A cavity may overlap at most 256 input cells.** Medium. Mechanism confirmed, impact plausible.
`Field2D:21` (`FIELD_LIMIT = 256`), `Field2D:375-379` and `:389-392`: if the cavity overlaps more donors the
transaction is refused and counted under "memory rejects". Once current cells cover more than roughly 25 input cells
each (about 5x coarser in h) every operation that touches them is refused, including flips, moves and splits needed
to repair shape there. Coarsening therefore saturates per call without a message that says so, and a poor cell in such
a region cannot be repaired, which leads to C2.

**C5. Surface operations next to a first-height wall have no shape floor and delete interior nodes.** Medium.
Mechanism confirmed, consequence plausible. `Boundary2D:284-288` takes only the cavity polygon; `:414-473` carves the
wall triangles and ear-clips the rest, so every interior node of the enlarged cavity (`Engine2D:701-724`) disappears.
`Boundary2D:507-513`: `shapeFloor = 0` when a layer is built and the action is not a coarsening, and the length check
(`:516`) applies to coarsening only. The published cells only need positive area. Repair is left to the bulk phases,
but bulk REMOVE/FLIP refuse any cavity that contains a protected wall cell (`Mesh2D:666-670`), so slivers attached to
the first layer can stay and trigger C2.

**C6. The boundary geometry is the original polyline; refinement cannot recover curvature.** Medium. Confirmed.
`Reference2D:124-137` interpolates linearly between the original boundary nodes; new boundary points
(`Boundary2D:311`, `:356-361`) lie on the original chords. Refining a curved wall adds collinear points. ADAP_HAUSD
only limits how far coarsening may leave the polyline. The sidecar keeps this polyline for all later cycles and
restarts, so the discretisation error of the first mesh is permanent.

**C7. One refused transaction can discard all transactions of the round and blacklist them.** Medium. Confirmed.
The admission flags come from collective maxima (`Distributed2D:152`), so `Engine2D:526-545` sets `approved = false`
on every rank when any rank exceeds its byte budget. The same holds for `failed.any` (`Engine2D:572-576`) and for a
refused directory staging (`Engine2D:581-588`; star cap of 128 at `Distributed2D:295-297`). `Engine2D:633-634` then
inserts each rank's own, valid candidate into `attempted`, so it is not retried until the next sweep. No mesh
corruption, but lost work that counts against the round budget of C1.

**C8. Metric interpolation and quadrature are coarse relative to the composed field.** Low. Plausible.
`Field2D:111-122` blends tensors linearly in M (arithmetic mean). This stays SPD but is biased toward the smaller size
in graded regions; log-Euclidean interpolation is the usual choice. `Mesh2D:115-116` evaluates quality with a single
centroid sample and `Mesh2D:133-145` evaluates length with three samples, while the BL composition is not P1 and can
change sharply inside one edge. A thin band between samples is not seen. The acceptance gates and the final contract
use the same samples, so COMPLETE does not certify conformity to the continuous composed metric.

**C9. Only the predicate unit is compiled with strict arithmetic; the whole 2D kernel is fast-math.** Low. Confirmed.
`Common/src/meson.build:46-56` puts `CNativePredicates2D.cpp` and all 3D kernel sources in the strict library.
`Common/src/adaptation/meson.build` puts `CNativeRemesher.cpp` and `CNativePartition2D.cpp` (which instantiate every
header-inline 2D routine) in the normal library, built with `-ffast-math -fno-finite-math-only` (`meson.build:101`).
I found no bitwise comparison that depends on a recomputed value (shared nodal targets are copied, not recomputed), so
the effect is limited to results that can change with compiler, flags or inlining, and to different code for the same
inline function in the two translation units (`WorkWeights` versus the engine).

**C10. The kernel depends on `long double`.** Low. Confirmed.
41 uses in the 2D headers; areas, qualities, lengths and the orientation filter (`Predicates2D.cpp:124-140`) run in
long double. The filter is written generically and stays valid, but results differ between x87 80-bit, binary128 and
64-bit long double, and on AArch64 Linux long double is software binary128, which makes every predicate and quality
evaluation slow.

**C11. Fault-injection hooks are part of the production kernel and wire format.** Low. Confirmed.
`Boundary2D:57` (`Operation::fault`, serialized at `:61`), `:312`, `:520`, `:572`; `Engine2D:483-489`. No production
caller sets the field.

**C12. `ValidateStaging` calls `area()` before the finiteness test.** Low. Confirmed (not reachable today).
`Engine2D:809-810`: `area()` throws `std::invalid_argument` on non-finite coordinates (`Predicates2D.cpp:125-127`)
and this line is outside the `try` at `:823`. A non-finite remote cell would end in `SU2_MPI::Error`
(`Remesher.cpp:690-693`) instead of a veto. `reconstruct` rejects non-finite cells earlier, so this is a
defence-in-depth ordering error.

**C13. `remeshAttempt` restarts at 1 when the remesher is recreated.** Low. Confirmed for the code path.
`CSinglezoneDriver.cpp:340-343` creates a new `CNativeRemesher` for every `RemeshFromMetric()` (used by `AdaptMesh`,
`:1859`). `Remesher.cpp:649-651` then finds the balance-profile files of the previous call and fails
(`workers.Fail`), and `Remesher.cpp:184-185` refuses to overwrite the rejected-candidate files. Diagnostic modes only.

**C14. `Repartition` takes a `World&` but uses the default communicator.** Low. Confirmed (latent).
`Partition2D.cpp:173` (`MPI_Comm_split(SU2_MPI::GetComm(), ...)`), `:206`, `:255` and every `CollectiveFailure`. The
only production caller passes the default world (`Remesher.cpp:452`), so this is wrong only for a sub-communicator.

Checked and found sound (no finding): orientation and cavity invariants are enforced before every publication
(`Mesh2D:185-235`, `:787-833`); ids are never reused (`Engine2D:689-700`), stale cells are detected by version
(`Distributed2D:440-443`, `Engine2D:805-807`); `ear_clip` cannot index a polygon with fewer than 3 nodes through any
caller; `relax_wall_apices` apex ordering holds because the carved apex is always left of the base; all loops are
bounded; integer widths are checked where they narrow (`Distributed2D:67-76`, `Partition2D.cpp:114-117`);
no uninitialized read found (`Tensor` has no default initializers but every use is value-initialized).

---

### 3. Performance findings

**P1. Every cavity operation costs a full collective round.** High. Confirmed (call count); timings not measured.
Counting calls in `Engine2D:307-616`, `Distributed2D:107-181` and `CPassiveComm.cpp:97-167`: one `exchange` is 4
failure elections (Allreduce), 2 Alltoall, 2 Allreduce and up to one Alltoallv. A bulk round has 16 exchanges plus
about 16 further collectives: about 150 blocking collective calls. A surface round (three dependency-growth passes,
certificate broadcast, collision veto) has 26 exchanges: about 250. Each round commits at most one cavity per rank,
and each exchange allocates P buckets. Throughput is therefore bounded by rounds per second, not by cores; adding
ranks adds commits per round but makes every collective slower, and the same protocol also runs in serial.

**P2. The donor index is switched off above about 29 000 input cells per rank.** High. Confirmed.
`Field2D:341-343` uses the ADT only if `IntersectionQueryBound(owned.size())` fits the dependency budget.
`CTransferMemory.cpp:44-47` with `CTransferMemory.hpp:80` gives 72 bytes per donor cell; the budget is hard-wired to
2 MiB (`Engine2D:58`, not set in `Remesher.cpp`). Above 29 127 cells the code falls back to a linear scan of the
whole donor `std::map` for every active caller (`Field2D:351-354`), in every round. Cost per rank and round:
(active callers) x (owned donor cells), which is the global cell count when all ranks are active. With 1e6 cells on 8
ranks that is about 1e6 map-node visits per rank per round, for up to 20000 rounds per phase. The log line "donor
search candidates/full-scan equivalent" (`Remesher.cpp:615`) shows it at run time.

**P3. Surface selection rescans all owned cells every round and evaluates O(K) reference deviations.** High. Confirmed.
`Engine2D:625-626` uses the uncached `choose` (`:226-235`) for the four surface actions. For SPLIT, `Engine2D:203`
calls `reference.deviation` for every boundary edge; `Reference2D:139-184` makes that two nearest-point searches over
all K knots of the component plus a loop over all knots. Cost per SPLIT round: O(N_owned) + O(N_boundary x K).
`satisfied()` (`Engine2D:640-661`) and the final check (`Remesher.cpp:513`) pay the same once per sweep.

**P4. `CollisionVeto` scans all owned cells once per proposal.** Medium. Confirmed.
`Engine2D:759-788`: for every proposal that edits a physical face, each rank loops over its whole `owned` map
(`:779`). Per surface round and rank this is (editing proposals) x N_owned.

**P5. Bulk selection is a 64-entry cache refilled by a full scan.** Medium. Confirmed.
`Engine2D:241-305`: a refill (`:274-281`) visits every owned cell and is needed whenever the cached top falls below
the best discarded candidate, about once per 64 consumed candidates. Total selection work is O(ops x N_owned / 64):
quadratic for a fixed rank count, and dominant in serial at 1e6 cells. A priority queue keyed by edge would remove it.

**P6. REDISTRIBUTE visits every non-feature boundary node in every sweep.** Medium. Confirmed.
`Engine2D:218-221` gives every such node the constant score 0.1, and `:631-634` marks it attempted only after its
round. One surface round (P1, P3) per boundary node per sweep, needed or not, up to `phase_rounds`.

**P7. The private kernel rebuilds ordered containers and copies cavities for every trial.** Medium. Confirmed.
`nodes()` and `boundary()` build `std::map`s on each call (`Mesh2D:157-184`); `validate_replacement` calls them three
times (`:210-211`). `MoveStar` copies the star and validates for each of up to about 150 trial positions
(`Mesh2D:347-401`). `strict_cells` is O(E^2) with map lookups in the inner loop (`Mesh2D:817-831`).
`relax_wall_apices` copies the whole cavity and runs `strict_cells` per trial (`Boundary2D:183-198`).
`SplitSeed` evaluates about 1480 sample points (`Mesh2D:950-954`) and `JointPatch` nests 3 x 4 x 2 starts, 13 step
sizes, 8 sweeps, all movable nodes and 9 directions, rebuilding `nodes(state)` per node (`Mesh2D:988-1107`). Every
metric evaluation goes through `std::function` and a hash lookup. This is a large constant per operation, not a
complexity problem, except for the joint repair.

**P8. Memory is about 0.9 kB per triangle in steady state, more during setup.** Medium. Confirmed by struct sizes.
`Cell` is 248 bytes because node coordinates are stored by value in every incident cell (`Boundary2D:36-47`); in a
`std::map` about 300 bytes. The donor snapshot adds about 250 bytes (`Field2D:467-472`) and the directory about 320
bytes per cell (`Distributed2D:252`, one map per node, one node per incidence). Setup holds extra full copies
(`Engine2D:113-129`), and migration holds the input, the send buckets, the received records and the replacement map
at once (`Partition2D.cpp:213-254`). 1e7 cells need about 9 GB plus transients, roughly ten times a compact array mesh.

**P9. Physical-graph validation is O(B^2) in the worst case.** Low. Confirmed.
`GeomVal2D:46-66`: the x-sorted sweep degenerates when many faces share an x-range (a long vertical wall). It runs on
every rank at first import and on sidecar load. The source comment states the bound.

---

### 4. MPI readiness findings

What is distributed: the volume mesh, the node-to-cell directory, the donor field, cavity import across ranks,
ownership of new cells, the output slices. Partition interfaces are not frozen: a cavity may span owners and is
committed atomically on all of them. There are no halos; dependencies are fetched on demand each round.
What is replicated or serial: all physical faces and the reference on every rank (O(B)); output boundary rows and the
sidecar on the master (O(B)); serial METIS only when one rank holds all cells (`Partition2D.cpp:184-190`).

**M1. Per-rank replication of certificates meets a fixed 2 MiB budget.** High. Mechanism confirmed, threshold estimated.
`Engine2D:492-499` sends each surface certificate to all P ranks under the dependency ceiling. The admitted peak
(`Distributed2D:144-150`) holds P copies of the certificate in memory, P packed copies, and everything received from
all active callers: about 232 bytes x rows x (P + active callers). With 40 perimeter rows the 2 MiB budget
(`Engine2D:58`) is reached near P + active = 226, with 100 rows near 90. Because admission is a collective maximum,
all surface transactions are then refused on all ranks (C7), boundary adaptation stops, and the call ends INCOMPLETE.

**M2. Several exchanges are all-to-all by construction.** Medium. Confirmed.
Cavity regions go to every rank (`Field2D:321-323`, source comment: "rank routing/ADT remains deferred"), decisions and
proposals use `metadata` (`Distributed2D:182-186`), physical faces are replicated in `Import2D:121` and `:175`. Traffic
per round grows as P^2 x cavity size, and every rank tests every caller's regions against its donors.

**M3. Reservation conflicts always go to the lowest rank.** Medium. Confirmed.
`Distributed2D:467-479`. A higher rank whose best candidate touches nodes claimed by a lower rank is deferred
(`Engine2D:388-390`), retries the same candidate next round (`:633`), and each deferred round counts against
`phase_rounds`. Ranks with higher numbers make systematically less progress along interfaces (HANDOFF_Codex.md notes
the same bias).

**M4. No load balancing during adaptation; the static one is off by default.** Medium. Confirmed.
`CConfig.cpp:3307` (`ADAP_NATIVE_REPARTITION` default NO). New cells inherit the owner of the parent cell
(`Engine2D:725-742`); nothing migrates while refining. A phase lasts until the busiest rank is done
(`Engine2D:628`), with all other ranks taking part in every collective. The predicted weights are capped at 1024 and
are not fitted to measured cost (`Partition2D.cpp:55-57`).

**M5. Results depend on the rank count and the partition.** Low. Confirmed (by design).
Each rank selects its local best (`Engine2D:226-305`), conflicts are decided by rank number, ids are allocated in
rank order (`Engine2D:689-700`), owners follow the partition. For a fixed rank count and partition I found no source
of run-to-run variation: ordered containers, sorted replies (`Distributed2D:282`, `:456`; `Field2D:327`, `:347`),
unordered maps used for lookup only, no time-dependent decisions. ParMETIS runs with seed 0.

**M6. Rank 0 does a pairwise test of all edited physical faces.** Low. Confirmed.
`Engine2D:792-798`: O((P x edited faces)^2) exact segment tests on rank 0 per surface round. Negligible below about
100 ranks.

**M7. A rank-local exception aborts the job; it does not hang it.** Low. Confirmed.
Collective errors are elected and unwound together (`Distributed2D:91-98`, `Remesher.cpp:686-689`). Any other
`std::exception` on one rank (for example from `choose`, `Owner`, or a reference callback) reaches
`Remesher.cpp:690-693` and calls `SU2_MPI::Error`. I found no path where one rank leaves a collective sequence while
the others continue, so no deadlock, but also no recovery.

**M8. Message sizes and counts are 64-bit safe.** Low (information). Confirmed.
`Distributed2D:117-143` checks byte counts in `size_t`/`uint64_t`; `CPassiveComm.cpp:121-164` splits into rounds with
checked `int` counts. Idle CFD ranks poll an `MPI_Ibarrier` every 1 ms (`Remesher.cpp:699-714`).

Overall: the protocol is functionally distributed and careful about consistency, and the repository evidence is at
1, 2 and 4 ranks on small meshes. For 1e6 to 1e7 elements the limits are C1 (work cap), P1 (one cavity per rank per
round), P2/P3/P5 (linear scans per round) and M1/M2 (per-rank replication), in that order.

---

### 5. Strengths

- Orientation is exact: a long-double filter with a correct error bound and an exact fixed-size integer fallback
  without heap allocation (`Predicates2D.cpp:26-140`), compiled strictly, tested at denormal and maximum magnitudes.
  Segment, containment and simplicity tests are built on it without tolerances.
- Transactional cavities: private reconstruction on copies, then area, perimeter-identity, orientation, manifoldness,
  duplicate, crossing and coincident-node checks before anything is published; participants re-validate certificates.
- Versions and monotone 64-bit ids: no index reuse, no dead-entity states, stale data is detected.
- Publication after the final vote uses node transfers and swaps that cannot allocate (`Engine2D:589-598`,
  `Distributed2D:340-355`).
- Collective failure election on every exchange; a working sub-communicator failure returns to the CFD communicator
  before the fatal protocol.
- Wire format is scalar by scalar, without struct padding, with range checks on decode.
- The metric is frozen for the call; shared nodal targets are kept bitwise identical by copying authoritative samples
  (`Engine2D:407-416`), and the output checks it (`Import2D:196-214`).
- Every loop has an explicit bound; byte budgets are checked before allocation.
- No complete volume mesh is gathered at any stage, including output.
- Extensive counters and timers are reported per phase and per rank.

---

### 6. Not verifiable statically

- Wall-clock cost of a round, and therefore the real throughput and the rank count at which it saturates (P1, M1).
- Whether q >= 0.18 and L <= 1.8 are reachable on realistic cases within the round budget (C1 to C5); the source only
  shows that failure is fatal and that the work is bounded.
- How often the donor cap (C4) and the collective refusals (C7) occur in practice.
- Behaviour of `-ffast-math` code generation on the inline kernel (C9), and of the filter on non-x87 long double (C10).
- Determinism of ParMETIS for a fixed seed and rank count; correctness of `CADTElemClass` for boxes encoded as LINE
  elements (`Field2D:437-466`); `SendRounds`/`RecvRounds` used by the diagnostic writer (`Remesher.cpp:193-202`).
- SPD-ness and continuity of the BL composition (`CBoundaryLayerMetric::ApplyPoint`); only its ADT lookup was checked.
- The CFD solution transfer and geometry rebuild after the remesh (driver code, outside this scope).
- Unit tests were not run. Coverage gaps seen by skimming: no test above toy size (the scaling test is hidden,
  `[.]`), none for the 29k donor-index threshold, the work cap, the INCOMPLETE path in the driver, rank counts above
  4, or reproducibility across rank counts.

---

### 7. Finding count

| Severity | Count | Findings |
| --- | --- | --- |
| Critical | 1 | C1 |
| High | 5 | C2, P1, P2, P3, M1 |
| Medium | 13 | C3, C4, C5, C6, C7, P4, P5, P6, P7, P8, M2, M3, M4 |
| Low | 12 | C8, C9, C10, C11, C12, C13, C14, P9, M5, M6, M7, M8 |
