# Cavity operators and active curved-BL pipeline: review

What this change does: this is a review of the existing adaptation implementation, using the supplied cavity-operator handoff as comparison material. It traces production callers and distinguishes mesh validity from successful metric adaptation and usable boundary layers. No production code was changed for this review.

Reviewed 2026-11-12. Baseline: `e4d3ca7618`, branch `codex/native-3d-bl`, build431 test binary. Two earlier, unbuilt432 census edits in `CNativeEngine3D.cpp/.hpp` remain held and unchanged; they are reviewed separately below. Working-tree line numbers refer to the reviewed files and can move after later edits. Evidence root R is `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

This report records the review delivered at `10a694e43c`, before fixes. Subsequent goal continuation qualified the scoped-census implementation and addressed finding4's scan ordering; see [432 implementation/outcome](ADAPTATION_SURFACE_DEMAND_CENSUS_20261112.md). Findings1–3 remain unimplemented. The historical source/line references below describe the reviewed state.

## Scope and evidence

The attached `CAVITY_OPERATOR_REVIEW_HANDOFF.md` and its three supporting notes were read in the requested order. Their pyAMG claims are static reverse-engineering findings about one wheel, not independent runtime qualification. Optional raw decompilations are available locally, but this review did not reverify assembly. None of the proposed fixes depends on copying that implementation or its constants.

The expected load is the actual saved M6 target and curved wall authority, hundreds of thousands of cells, bounded work/memory/time, and eventual native-MPI BL generation. Analytic tests are supporting diagnostics. Euler293's completed workflow and checkpoints remain the regression baseline; curved-BL/RANS completion is still unproven.

Source/build provenance was checked against build431 source pins and binary hashes. The only mismatching compiled-source files are the two explicitly held432 files; their build431 versions were recovered from the baseline commit and matched the build pins. The fresh diagnostic executed the verified build431 binary, **not** the unbuilt432 source. Current sources were unchanged throughout. Actual PID/start identities, including the review wrapper, were verified terminal. No heavy workload is now owned.

Receipts: `R/reviews/cavity_operator_20261112/`, especially `checks_20261112T085552_640304/{command.json,status.json,run.log,terminal_handles_actual.json}`. The manifest records attachment/source hashes and the held diff.

## Active execution paths

`CSinglezoneDriver.cpp:300,338,389–400` selects the native remesher and the configured FIRST_ROW construction callback; this is an actual runtime option, not an Euler/RANS-name dispatch. The saved configuration has `ADAP_BL_METHOD=FIRST_ROW` and `ADAP_NATIVE_RANKS=1`.

`CNativeRemesher3D.cpp:512,568` creates the native worker communicator and invokes construction. `CNativeBLPipeline3D.hpp:254` explicitly refuses FIRST_ROW on more than one native worker. An outer MPI solver with one native worker is supported by this path; that does **not** establish distributed BL generation.

The active initial construction sequence is:

1. Saved field/geometry and composed target setup.
2. Common `PrepareSurface` at `CNativeBLPipeline3D.hpp:377–420`.
3. Rebuild constrained directions after surface changes, at `:465–483`.
4. `dc9::PhaseR1`, `dc9::PhaseR2`, then `dc15::CompleteFirstRow`, at `:525–540`.
5. Shared production `PhaseG` successive growth at `:544`.
6. First-row/independent-validity audit and private output at `:580–621`.
7. Common `AdaptMetric` and its complete strict census, through `CNativeRemesher3D.cpp:618–629` and `CNativeEngine3D.cpp:2689–2716,3051–3113`.
8. Later flow transfer and solver replacement, after mesh acceptance. The pending row registry is not itself a solver-mesh commit.

The continuation branch instead uses `dc17::RebuildFirstRow` and completion (`CNativeBLPipeline3D.hpp:549–575`). Historical `PhaseW`, `PhaseW10`, `PhaseR1w`, `PhaseR2_10` and `PhaseV*` variants were not found on this production route. Their fixture presence is not evidence that they execute here.

General insertion/splitting, collapse, relocation and reconnection are reachable from the Common engine. Examples: `CNativeEngine3D.cpp:4071–4081` calls `SplitEdge`/`InsertEdgeCavity`; `:4192,5268` calls `MoveVertex`; `:4203,4735,5277` calls `Reconnect`. Boundary operators use private parameter contexts and bounded stars/shells. `CNativeCavity3D.cpp:351` supplies private coning. No general metric in-sphere/Delaunay cavity-enlargement path was found. That is a design difference, not by itself a defect.

## Must fix before curved-BL qualification

### 1. P1 — Active growth cannot complete the existing box case after a front dependency changes

**Observed bug.** `BLGrowth12.inl:360–364` captures `boundary_revision` once. Guards at `:380,415,426` throw when it changes. Growth can call `MakeRoom`, `ClearAbove` and `Apply` while retaining that front snapshot. `BLState.inl:286–290` invalidates direction data when a transaction changes relevant wall/carrier dependencies.

**Concrete trigger:** the existing `[NativeGrowthLayer12]` serial test, `native_first_row_dc12.inc:206–217`, runs R1/R2 followed by the same production `PhaseG`. On verified build431 it planned 12 active tops and then threw: “BL growth surface dependencies changed; current fronts need reconstruction before advancement.” This is a fresh reproduction of the current failure, not evidence that432 introduced a regression. The exact first mutation causing the revision change was not isolated.

**Consequence:** successive growth aborts rather than advancing a valid refreshed front. The guard is protective; removing it would permit stale directions/support and is not an appropriate fix. Actual M6 stages430/431 did not reach growth, so this is not an observed M6 growth outcome.

**Smallest correction:** preserve the stale-dependency guard. At the transaction boundary that changes its dependencies, rebuild/revalidate the affected current front, directions and column ownership before reusing a plan. Reuse existing direction invalidation and reconstruction facilities; do not simply assign the new revision to the old plan. Distinguish expected local mutations from unrelated changes.

**Meaningful test:** make the existing growth case pass with physical heights, progression, carrier constraints and complete-mesh validity checked. Add a focused case where an owned preparation transaction changes a dependency and safe reconstruction succeeds, plus an external/stale edit that must invalidate the old plan. Cover a competing-wall control. A successful first layer alone is insufficient.

### 2. P1 — Acceptance does not independently establish full BL coverage and progression

**Missing capability/acceptance contract.** `CNativeBLPipeline3D.hpp:544–548` records growth results, then `:580–589` bases provisional construction validity on independent geometry/positivity and the first-row audit. It does not require a successful whole-region coverage/progression assessment. `BLGrowth12.inl:466` can label the growth loop complete after active columns stop. `CountLayers`/`JsonLayers` exist (`:495–517,799–802`) but are not used here; their limited distance band and diagnostic thresholds do not constitute the missing complete BL contract.

**Concrete trigger:** first rows exist, while later columns stall or stop early without exhausting the whole event budget. The callback can still pass its current provisional checks. Subsequent Common validation enforces geometry, q/J, metric size and requested first height; it does not independently measure actual physical layer intervals, coverage, alignment and outer transition throughout the BL after arbitrary remeshing. Metric conformity alone is not that measurement.

**Consequence:** the code has no demonstrated acceptance condition distinguishing an acceptable full BL from an acceptable first row plus incomplete growth. This is a source-proven validation gap, **not** an observed falsely accepted M6 mesh. Provisional construction may legitimately carry diagnosed debt under the stage cadence; that output must stay provisional.

**Smallest correction:** define and evaluate the current-mesh BL suitability contract at the appropriate stage/final boundary. Reuse requested height/growth/thickness data, current geometry and existing layer diagnostics, but assess actual physical intervals, alignment, coverage and transition after general adaptation as well. Record justified junction/feature transitions separately. Do not turn a diagnostic's arbitrary pass fraction into a new physical rule, impose final shape gates prematurely, or freeze columns permanently.

**Meaningful test:** stop a synthetic and an actual M6 patch after only the first row. It may be a saved provisional stage output but must not qualify as a complete BL. Then demonstrate full coverage/progression, with a documented junction exception, and repeat the measurement after split, collapse, relocation, reconnection and ownership migration.

## Should fix

### 3. P2 — Planar proposal constraints can immobilize a semantically smooth curved vertex

**Verified source mismatch, corroborated by actual M6 diagnostic.** `ConstrainMovePoint`, `CNativeBoundary3D.cpp:665–748`, adds original facet-plane normals for nonanalytic/nonaffine incident faces (`:705–720`). Three independent normals return the original position (`:746`). A mapped PN reference is facet-local (`CNativeAffineReference.cpp:1248,1411`), while its mobility policy permits two-dimensional movement at smooth original edges/vertices (`:1495–1507`). Canonical facet identity therefore does not mean physical crease identity in this case.

**Concrete trigger:** a smooth mapped wall vertex with mobility dimension2 and at least three independent incident chord-plane normals. Active composed repair calls the helper before `MoveVertex` (`CNativeEngine3D.cpp:4189,5266`); joint proposals also call it (`:4647,4656,4685`). The actual430 smooth control selected using semantic features/corners had its proposal reduced to zero. `MoveVertex` has entity-aware binding support (`CNativeBoundary3D.cpp:3205`), but it receives the already eliminated proposal.

**Consequence:** some permitted curved-surface repairs are never tried. This is not universal boundary immobility: separate boundary smoothing and composition-empty initial W paths limit the scope. Neither the static analysis nor430 proves that a nonzero replacement would pass every final gate.

**Smallest correction:** route mapped on-surface proposal constraints through existing semantic binding/mobility machinery. Retain true faceted-plane, genuine-feature, affine-boundary and requested wall-apex constraints. Preserve fresh position-dependent certification, exact support identities and private rejection semantics.

**Meaningful test:** use a smooth PN vertex with three independent chord normals and mobility2. A small valid surface proposal must remain nonzero and private, then undergo full `MoveVertex` admission. Add genuine ridge/corner, affine carrier and requested-height apex controls. Test rejection leaves coordinates, connectivity and parameters unchanged.

### 4. P2 — Held432 performs a full ownership scan on every surface repair round

**Unbuilt working-tree finding only.** `scope_census_complete`, `CNativeEngine3D.cpp:1294–1297`, calls `Selection::SurfaceCensus` (`:387`), which scans `Owned`. `surface_ready` (`:2978–2979`) evaluates that helper before the cheap construction-debt test. The main loop calls it while repairing (`:3126`), so many repair rounds can each traverse the entire local mesh even when known remaining debt already proves unreadiness.

**Concrete trigger:** a large M6 ownership set with remaining requested-wall repair debt across multiple rounds. This is avoidable O(local cells × repair rounds) work, not a measured timing regression:432 has never been built or run. The ordinary dirty/cursor checks are already cheap; this finding does not claim every loop scans the mesh.

**Smallest correction:** test known debt/pending work before the complete coverage scan, retaining the required full check at quiescent readiness/phase-transition boundaries. Reuse existing progress accounting. Do not replace unknown census flags with fabricated clean measurements.

**Meaningful test:** several wall repair rounds with many unrelated interior cells. Count complete census visits and show they occur at the necessary boundaries rather than per round. Confirm identical required coverage, cached/recomputed results, migration flags and empty-rank collective alignment. Keep432 held until its connected change is reviewed and qualified.

## Compact comparison with the supplied reference

| Topic | Our active implementation | Recovered reference behavior | Practical difference and confidence |
|---|---|---|---|
| Proposal | Engine4071–4081; Boundary1562,1607,3181,3370; Cavity351 | Driver proposal/reduction, shared correction; BL front-derived cavities | Ours uses bounded stars/shells/cones, not a general in-sphere growth kernel. High confidence in ours; reference is static. No demonstrated need for larger cavities. |
| Correction/stops | Boundary operators and Cavity123,351 | Kernel visibility enlargement; upstream reduction; caller-dependent options | Our exact bounded refusal is intentional. A later geometry refusal does not necessarily retry every insertion alternative. No feasible missed replacement was demonstrated, so this remains a hypothesis. |
| Numerical predicates | Predicates142–159,305,847–855,878–918 | Floating scaled determinant rejection in checked routine, no exact fallback found | Ours has finite/SPD checks and bounded exact orientation/quadratic fallbacks. Fresh primitive tests pass. Tolerance and resource failures are not silently accepted. |
| Topology/interfaces | Cavity123,227–286; private Cone370–371 | Manifold cavity tests with conditional nonmanifold options | Our connected sphere/disk/link and fixed-interface checks intentionally reject unsupported topology. Tests reject duplicate, disconnected and genus-one cavities. Not a general nonmanifold mesher. |
| Publication/failure | Distributed1433 onward; Engine reservation/clearance/publication | Preserve old connectivity before commit; temporary state may change; postcommit STOP possible | Ours stages private proposals, versions dependencies and prepares publication state. No concrete reachable atomicity bug found. Exhaustive allocation-failure rollback was not fault-injected. |
| BL constraints | Pipeline377–548; BLFirstRow9:239–279; BLGrowth12 | Surface/coarsen, BL insertion, ordinary volume processing; mutable fronts, domain filtering later | Similar broad ordering, different implementation. Constrained direction refresh exists, but findings1–3 remain. MPI6 surface preparation does not qualify native-MPI FIRST_ROW. |
| Objectives | Metric436–445; Engine2689–2716; composed target checkpoint396 | Metric complexity is density, not final count; no explicit BL subtraction found | Ours accounts for composed volume metric;500k is nominal. Validity-first construction may carry q/J debt, while final gates remain strict. No justification to copy reference quality constants or subtract overlapping BL costs. |

The published generic-cavity method supports simultaneous surface/volume modification as a design reference, not proof of equivalent certification or runtime capability. Sources: [Loseille, Alauzet and Menier (2017)](https://doi.org/10.1016/j.cad.2016.09.008), [Alauzet and Loseille (2016)](https://doi.org/10.1016/j.cad.2015.09.005). The supplied wheel investigation and these papers are separate evidence.

## Validation actually performed

Fresh serial diagnostics on verified build431: **9 cases, 8 passed and 1 failed; 86 assertions, 85 passed and 1 failed**. Native executable wall0.56s, CPU0.26s, maximum RSS37,540kB; wrapper2.706s. No performance claim follows from this short run.

Passed cases cover exact orientation/fallback, singular/indefinite/nonfinite tensors, private single-tet four-child insertion and vertex removal/movement, two-to-three reconnection, obstruction/interface/budget refusal preserving output, duplicate/disconnected skins, genus-one rejection and rotated thin cavities. The growth case failed as finding1 records; its remaining subcases were **not** completed.

Reused, not rerun: actual430 current-target complete-cavity diagnostics and431 nativeMPI6 surface-stage receipts. Actual430 demonstrates a private smooth collapse and LE split preflight, plus correctly refused geometry regression. It does not establish published admissible BL cells. Actual431 is a passing diagnostic with `surface_prepared=false`:389.056s,784 collapses,0 splits,0 completed passes;15,980 cached and285,203 unknown cell rows. Its134 known geometry-bad/3 known size-bad counts are partial; q/J/heights remain unmeasured. These receipts establish the current stage3 blocker, not whole-mesh quality or BL success.

No new baseline, perf/GDB campaign or full M6 retry was launched. No new MPI campaign was warranted for a read-only review. Proposed tests under the findings have not been run.

## Robustness boundaries and recommendation

The checked cavity boundaries preserve exact positivity, interface/topology restrictions and private rejection; publication uses global reservations, clearance and versioned dependencies. Strict metric/shape/height checks are distinct from construction validity. I found no demonstrated general connectivity corruption or reason to relax these safeguards.

Remaining capability gaps are material. FIRST_ROW explicitly requires one native worker; dedicated distributed BL construction and constraint preservation remain unqualified. The current actual surface stage has not reached readiness, growth has a reproducible diagnostic failure, and complete physical BL suitability lacks an independent acceptance contract. Narrow-gap growth, repeated mutation/publication faults, exhaustive resource-failure rollback and complete two-event RANS transfer/restart were not exercised by this review.

Recommended order: retain the review hold and preserve the existing432 diff; resolve its scan issue within the existing surface-census cycle before qualifying that change. Preserve the ordered stage workflow. Reproduce and correct the growth dependency lifecycle at its stage boundary, then require complete BL suitability before any claim of curved-BL acceptance. Test the mapped proposal correction on actual representative cavities before production adoption. Do not bundle a new scheduler, larger cavities, Delaunay conversion or geometry representation into these fixes.

**Verdict: fix1 and2 before claiming curved-BL qualification; address3 with demonstrated patch admission and4 before merging432.** The reviewed primitive paths provide evidence of validity preservation, but the current implementation does not yet demonstrate the complete curved-BL/RANS adaptation objective. Failure is protective in the reproduced growth case; whole-workflow failure safety is not exhaustively established.

Not checked: assembly equivalence of the recovered wheel, exhaustive allocation-failure injection, native-MPI BL generation or a new end-to-end RANS run. No production changes were made after the review request; pre-existing432 remains unbuilt.
