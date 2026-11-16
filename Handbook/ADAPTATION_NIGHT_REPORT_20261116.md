# Unattended adaptation work, 2026-11-16

DRAFT: N5 still active; this file is not a completion claim. The accepted implementation
remains F3b48f07ad1a8 plus report helpera27f56cd78. Production restorations arebecb4927c9
and8039fc1b59. Latest measurement checkpointeb5e48174a is pushed. N5 observation G2/GQ identity passed;
full two-event qualification is still pending. Current CFD hash861737e1 is unchanged
through test-only build521 (test382dc714).

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
NIGHT/H/D/C and both inventories are SHA-sealed in `R/night_20261115/inputs`.
The latest unattended section2 is in AGENTS.md; no user/reviewer answer was requested.

| Item/sub-step | State | Commit | Numbers and evidence |
|---|---|---|---|
| N1 ordered fixtures/checks | PASSED |6322be34ea/d4e9e81cac|Exact first-collapse90809869work; C492461splits/304986positivecells; prior focused/MPI/safeguards passed.|
| N1 full two-event result identity | FAILED |failed N1; revertedbecb4927c9|Event1 all3hashes identical; event2 firstNELEM855828 vs855760 (+68), points168797 vs168790 (+7), all3hashes differ.|
| N2 B7 | NOT STARTED |—|Accepted N1 dependency false; failed M18 is not qualified condition input.|
| N3 whole design source review | PASSED |6941fcc41c|Read-only, one14-item list; M11/M15/M17/M19 and conditional T1–T3 answered.|
| N4 F3b reference rebuild/G2/G3/GQ | PASSED |4798ea3936|Build870.265s; G2exact90809869work; G3fresh473splits/305015positivecells/first336P6c; GQall6thirdwindows.|
| N4 A1 five-type hoist G1 | FAILED |failedec0e8f3ef5; reverted8039fc1b59|First old metric assertion209.9357526597 vs656.1522579418, epsilon5e-4;13FAILmarkers, serial600s timeout. MPI/G2notstarted.|
| N4 later phases | NOT STARTED |—|A1 and unchanged G1 prerequisite failed.|
| N5 existing F3b timing extraction | PASSED |4f4919ef61|204qualitywindows/180timerrows,2events×6ranks; source/runtime/input/physical receipts retained.|
| N5 observation prefix gates | PASSED |a90d420937/6e0777868b|G2 all integers/exact90809869work; GQ all6thirdwindows and old integers identical. CFD861737e1.|
| N5 bounded measurements/full profile | ACTIVE |eb5e48174a|Build521; five refreshed stack/channel cases, actual operators and all three geometry measurements passed. Corrected locality check active; full two-event profile pending.|
| N6 further cuts | NOT STARTED |—|Requires completeN4+passedG5.|
| N7 final closure | NOT STARTED |—|This draft will be updated afterN5.|

## Failed items and reversions

N1: first difference is event2 mesh line2 (+68 cells). The one bounded read-only
diagnosis found prepared-cache history feeding StructuralEstimate, MAX-reduced geometry
work and partition weights: rank0geometry_before55816→56650, geometry work31467242.537→
32019399.242, cells_after125428→115965. No proposal transcription/wiring error was found.
Reverted16connected production/config/test paths inbecb4927c9; no numerical retry/B7.
[Full failure report](ADAPTATION_P7_P8_R3_20261115.md), `R/p7p8_r3_20261115/first_result_failure.json`
and `readonly_diagnosis_rows.json` preserve numbers and attribution limits.

N4 A1: first failure is BoundaryLayerMetric_tests.cpp1220, complexity209.9357526597 vs
656.1522579418 at existing epsilon5e-4. The bounded read-only diagnosis found the five
moved type bodies and remaining Adapt tokens unchanged. This assertion calls ComputeMetric,
not Adapt; its unscaled tangent expectation differs from the current normalizing path
(recorded scale0.046415888336127774). Both files are unchanged from F3b. This is not proof
of which side is wrong, and no baseline numerical replay was done. No allowed transcription
fix exists. Failed code preservedec0e8f3ef5, reverted8039fc1b59. Evidence:
`R/night_20261115/N4/A1/{first_result_failure,readonly_diagnosis,G1_terminal_handles}.json`.
No old test, production policy, gate or tolerance was changed to pass G1.

## Full runs beside accepted F3b

| Run/event | Native seconds | Remesh root seconds | Six-rank peak RSS sum B | Result |
|---|---:|---:|---:|---|
| Accepted F3b/1 |2683.411540761|2683.459776222|13950185472|All physical gates and3P6c hashes pass|
| Accepted F3b/2 |1977.214813802|1977.279934371|17696780288|All physical gates and3P6c hashes pass|
| Failed N1/1 |2220.339770577|2220.388 (rounded)|14007463936|All physical gates and3P6c hashes pass|
| Failed N1/2 |1450.966461726|see sealed timer receipt|17343528960|Physical gates pass;3P6c hashes fail|

N1 controller4117.532307s. All event times stay below5400s; all exact rank peak sums
stay below19073MiB=19999490048B. Failed-item timings are observations, not accepted
speedups. Complete rank/event timer-tree comparisons are
`R/p7p8_r3_20261115/failed_N1_vs_F3b_timer_rows.csv`; inclusive nested times are nonadditive.
M8 measurements are `R/night_20261115/N5/existing_profile.md` and the two accompanying CSVs.
Busy is wall time, not CPU; join/collective intervals overlap; named exclusive includes join.

## N3: single consolidated source-check list


1. **M15, D4.3: entity mode has no per-face authority partition; use the written fallback.** `CNativeBoundaryParameters3D.cpp:256–257` explicitly rejects nonempty `coverage.faces` in entity mode. Its certificate aggregates every current face by entity at423–445. `CNativeGeometryBackend3D.cpp:249–265` selects the complete body/marker/sheet authority and compares it with the supplied physical union; it does not return a source partition or a combination receipt. `CNativeAffineReference.hpp:189–205` defines complete-union directed Hausdorff bounds. For physical patches F_i, reverse error combines by max, but forward error is `sup_g min_i dist(g,F_i)`, which cannot be recovered as max of independently computed whole-authority forward bounds. **Reading used:** D4.3's explicit fallback: retain the complete entity forward/two-sided statement on one assigned worker per entity, distribute only the separable face part. Do not claim arbitrary owned patches plus scalar reduction are the same entity certificate. This is a result prerequisite for later steps, not a reason to stop N3/N4.

2. **A proof-query/cache change is not necessarily results-neutral in the next event.** Restored `CNativeAffineReference.cpp:2114–2121` reads prepared cache triangle counts in `StructuralEstimate`; `CNativeImport3D.cpp:1760–1788` MAX-reduces those observations and1664–1665/1804–1806 feed them into geometry partition weights. N1's owner proof changed retained cache history. Event1's three payloads match P6c, but before event2 construction rank0 geometry weight55816→56650 and original geometry work31467242.537374206→32019399.241544221, despite matching mesh/metric counts. Event2 ends with+68cells/+7points and three different payload hashes. The exact per-facet cache inventory was not logged; that attribution limit remains explicit. N1 was reverted in `becb4927c9`, without a retry or new production fix. D4.3/5B certificate reuse and the cost model must account for this existing dependency; a stored proof bit is not evidence that downstream work/partition choices stay identical. See [N1 report](ADAPTATION_P7_P8_R3_20261115.md).

3. **D3/4.1/4.3: factor-four decay is an estimate, not a general termination proof for supported cubic geometry.** The source supports arbitrary admissible cubic control nets (`CNativeAffineReference.hpp:112–115`); `EvaluateSurface` at cpp1929–1983 supplies point differential values, not a facet-wide curvature bound. The osculating-paraboloid derivation does not prove that every projected split divides actual certified error E by four. An analytic counterexample is the admissible graph `z=epsilon*(x^3-x)` over the triangle with corners(-1,0),(1,0),(0,1). All three corners and the projected midpoint(0,0) have z=0. Bisecting that long edge leaves the union of the two flat child faces exactly equal to the old flat triangle, so either directed distance between the physical union and the model is unchanged after that level. `SplitEdge` preserves that inherited split connectivity (`CNativeBoundary3D.cpp:1562–1606`). Thus `ceil(log4(E/tau))` can be a planned amount of work, but the claimed per-level decrease/termination is not established. The unchanged exact final gate remains necessary. Likewise, D5.9.5a's claim `depth(Q)<=tau` does not follow merely from a point estimate. T2 can instead be assessed using the actual Q diameter as written; it must not silently substitute tau.

4. **M11, D5A.2.7: unchanged cavity skin is conditional, not universal.** Row `Transaction` preserves wall and artificial faces but permits replacement non-wall carrier faces and checks their marker boundary loops (`BLFirstRow9.inl:774–802`). `LayerTransaction` does the same (`BLGrowth12.inl:230–269`). Positive cells and `ValidatePrescribedInterface` are also required, but equality of carrier boundary loops is not equality of a curved triangulated carrier surface. The asserted containment in the *old* cavity therefore needs an additional argument for carrier-changing cases; the source does not establish blanket removal of body-contact admission for all supported geometries. For a transaction with identical complete oriented skin, the containment argument has the required boundary premise. `ClearAbove` is not a counterexample to this argument: it answers inverted-cell room refusals, not distributed `Directory::Clearance` (`BLGrowth12.inl:209–214,293–326`).

5. **D4.2/5A.2.3/5A.2b, M17: the inverse and layer-structure mapping need more than a simple column chain.** `Transaction` starts with lifted-vertex stars, then recursively closes over old apex stars (`BLFirstRow9.inl:694–700`), creates sector points/gap cells/overrides/extras (`:713–761`), and updates those records in `ApplyTx` (`:821–825`). The exact operation sequence is summarized below. Removing face apices followed by vertical edge collapses is not a source-proven inverse of all those states. `LayerTransaction` creates sequential fill tetrahedra at partial/stopped fronts (`BLGrowth12.inl:215–226`); a general complete-shell split preserves conformity, but neither it nor the first-row ramp rebuild promises to update arbitrary upper-layer chains/fill-cell identities. `BLRebuild17.inl:10–23,189–267` handles its specific first-row lid split/diagonal flip/prism rebuild with refusal paths. **Reading used for the cavity:** D5.1's *complete cavity required by the operator* includes recursive apex closure, not only the initially lifted stars. The new Unlift and per-layer column edits still need their exact state/topology mapping before8.6. Their absence today is expected; the claimed inverse/structure preservation is what remains unproved.

6. **D5.9.4: rank-ordered semantic changes need a revision/merge rule.** `BoundaryParameterState::Prepare` increments a nonempty batch's revision and rejects edits whose revision differs from current state (`CNativeBoundaryParameters3D.cpp:643–649`). Local edits from a common pass start can finish with different private revisions. Applying their already-stamped lists sequentially to a changed revision is not the current API's commuting operation; applying a worker's own edits twice is also not valid. The geometric disjointness rule alone does not resolve these revision semantics. `CommitPrivate` is specifically for owned unpublished single-worker state (`CNativeBoundaryParameters3D.hpp:133–148`); distributed publication currently validates a common union collectively (`CNativeDistributed3D.cpp:1441+`). D must specify the common publication state/revision semantics; no independent merge redesign was implemented here.

7. **D5.9.5a T1 reuse is not a direct triangle/Q predicate.** Full replicated wall coordinates and, under5.9.1, local neighbor owner cells provide the data on one worker. However, the exposed clearance predicate is `ConformingDisjoint(Cell,Cell)` (`CNativeMesh3D.hpp:67–69`), which requires two positive finite tetrahedra (`CNativePredicates3D.cpp:474–479`). D's T1 is a wall-triangle versus tetrahedral-union condition and may involve flat components. The existing predicate's preconditions, shared-node identities and Q representation must be preserved by any proposed reuse. Source availability alone does not establish that the stated test is already implemented. T2/T3 data availability and the conditional T2 argument are recorded below; lack of the new tree or pass snapshot is not itself a contradiction.

8. **D5.10 conflates local kernels with collective call paths.** `SplitSurfaceStrip`, `ReconstructHeightStar`, `PlaceSplitPatch`, `RefitWallSplit`, `ReconstructWallSplit`, `ReconstructSplitPatch` are patch operators in `CNativeBoundary3D.cpp:1677,1896,1991,2188,2274,2398`, without a Directory/world parameter. Their engine repair paths can acquire/grow cavities collectively; the operator implementations are not all collective cavity-growth APIs. `Directory::CorrectCollapse` is collective (`CNativeDistributed3D.hpp:127–130`). **Reading used:** retain D's conservative choice of only SplitEdge/InsertEdgeCavity inside S1 passes; interpret the excluded collective growth as the existing *engine call paths*, not every listed kernel's intrinsic behavior. No broader local-operator substitution was made.

9. **D5.5.4: `global_work` is not only one unit per proposed edit.** Proposal admission adds proposed counts (`CNativeEngine3D.cpp:3810–3817`), but private optimizer work is reserved and charged (`:3846–3847,5807–5813`), collapse correction adds its work (`:3908`), and reconstruction/extra trials add more (`:4230,4660,4891,5073,5185`). The default limit is `max(4096,64*global_cells)` (`:2135`), not simply64*input cells in every path. A deterministic proposed-edit pass share must not be described as direct reuse of this mixed counter without defining which units it uses. No budget or counter semantics were changed.

10. **D5.6 determinism is not guaranteed by the current source solely from a fixed rank count.** Existing phase ownership uses measured elapsed time, census/initialization time and cache-warming seconds in its migration decision (`CNativeEngine3D.cpp:2933–3030`, especially wall/remaining/estimate/rebuild/warming and `locally_eligible`). The source therefore permits placement decisions to vary with timing even before the event deadline binds. D5.5 proposes a count-driven replacement, but8.2 must preserve the existing rule. The strict cut identity gates remain mandatory; this observation does not authorize freezing decisions, changing tolerances, or relaxing a failed identity check.

11. **D5A terminology and D5B query counts need source qualifiers.** A standard face prism has a face apex; growth `Column` objects are per wall vertex, initialized from `rows.lift` (`BLGrowth12.inl:339–346`), while `Rows` separately stores vertex lids, face apices and sector metadata (`BLFirstRow9.inl:131–138`). Regular eight-tet apex stars do not describe every sector/extra-skin case. In5B, `Selection::Add` queries physical faces only when extension components exist, with requested-marker filtering during surface planning (`CNativeEngine3D.cpp:838–845`). SmoothBoundary's fallback has19 targets (`:6620–6627`), its feature-edge search has15 (`:6606–6609`), and the legacy chart-interior branch can generate24 (`:6611–6618`); “up to19” is not universal. These are source-fact/count corrections, not reasons to stop an otherwise passing item. The specific `geometric_change` call has no memo as D says; other memo-capable guidance callers do not contradict that specific statement.

12. **D5A.3: containing the row demand is not, alone, a consecutive-layer progression certificate.** The target does include the wall-normal profile (`CBoundaryLayerMetric.cpp:2377–2383`), and `NextLayer` integrates one unit of that target (`BLGrowth12.inl:154–176`). But the final `size_bad` test is an upper edge-length test (`CNativeEngine3D.cpp:819,827`); it stores no adjacent-column height-ratio comparison. An upper bound permits shorter edges and does not by itself prove a ratio between consecutive heights. Keep the existing row-generation mathematics and requested progression; do not represent the maximum-edge gate alone as a separate progression proof. No new acceptance test or tolerance was invented here.

13. **D5.5 explicit-destination migration is reusable code, not an existing public entry point.** `Repartition` accepts weights/part count, not a destination list (`CNativePartition3D.hpp:36–39`). Its body computes colors then uses them for exchange at cpp341–390, and returns destinations in stats. D's inferred direct-call reuse needs an interface extraction/extension at its future step; the exchange mechanism exists. This does not block the identical cut.

14. **D1 memory wording is superseded by the latest user order.** Source's12288MiB option models one rank, while the night order's physical job gate is19073MiB=19999490048B over rank process peaks. **Reading used:**12GiB remains the modeled per-rank ceiling, not the whole-job cap. N1's P8 implementation is reverted; an external strict job gate is still required for subsequent validation. No cache quota from that cap, no gate/budget change, and no P8 acceptance claim follows from this reading.

## N5: measured numbers

Scope: rows requested by D section 9 for N5. Values below are qualified by each fixture's population and setup. `Measured` does not mean the integrated design is validated. M18/M20/M21 derive from N1 and remain unqualified because N1 failed its second-event result identity gate; emitted values are shown with that qualification and are not accepted evidence.

| Row | Current value / evidence-backed result | Measurement scope and evidence | Status / gap |
|---|---|---|---|
| M1 | Accepted Euler 293: 82,559 selected edges; 7,521 geometry-need edges; 7,127 need vertices; 7,521 first-pass midpoint candidates; complete edge coverage, no unvisited edges. Saved492 provisional-state diagnostic: 59,884 selected edges; 6,759 need edges; 3,164 need vertices; 6,759 candidates; complete coverage. Euler input decode12 (build520): 58,305 selected, 4,368 needs, 2,273 need vertices; this repeats the prior build515 counts under uniform decoder setup. | Euler293: `N5/cases/n5_d9_geometry_euler293_20261116T033059_2608544/d9_geometry_summary.json`; provisional saved492: `N5/cases/n5_d9_geometry_saved492_20261116T033718_2624292/d9_geometry_summary.json`; Euler input decode12 `N5/cases/n5_d9_geometry_euler_input_decode12_20261116T034246_2637556/d9_geometry_summary.json`; marker filter 2, geometry-only endpoint needs at tau 1e-4. | Both geometry-only endpoint-need censuses are complete; midpoint candidates are not completed S1 point counts. Saved492 numbers are diagnostics on a known provisional state, not an accepted mesh/result. |
| M2 | On 32 lexicographic wall-edge candidates, plain=32, cone=0, refused=0; average split API time 1.682 ms (range 1.410–2.088 ms). | `N5/cases/n5_d9_actual_operators1_20261116T032929_2605008/native_d9_actual_operators_m1.json`; actual Euler 293 import; population scope is first 32 of 82,559 WING3D edges. | Partial bounded population; 82,527 wall candidates unvisited. Not a whole-surface split share or full repair-lane cost. |
| M3 | Accepted Euler293: 54,961/54,961 faces classified within tolerance. Direct-path calls 26,067 and union-path calls 30,863 are inclusive; 1,969 faces use both. Exclusive counts: direct-only 24,098, union-only 28,894, mixed 1,969. Per-face timing sums: direct 2.059850 s, union 242.627970 s, classify 248.099255 s; aggregate query work 136,047,061. Complete census within per-face caps. Saved492 provisional-state diagnostic: 39,853/39,853 faces classified; direct calls 36,908 and union calls 6,361 are inclusive with 3,416 overlap (direct-only 33,492, union-only 2,945, mixed 3,416); 36,861 certified, 2,992 not certified within tolerance; aggregate query work 72,805,949. Path timer sums: direct 2.673689 s, union 92.093153 s, classify 95.313785 s. Euler input decode12 lists 38,800 selected faces but Classify unsupported and zero attempts; it has no parameter payload and geometry tolerance does not match M1 tau. | Euler293 `d9_geometry_summary.json` + `d9_geometry_faces.csv`; saved492 provisional `d9_geometry_summary.json` + `d9_geometry_faces.csv`; supported cached-binding path, tau=1e-4. For each input, direct/union path call and time totals include its mixed-overlap faces and must not be treated as exclusive face populations. The saved492 old-demand-face addendum is diagnostic on a provisional mesh: 2,736 pass / 2,992 over tolerance; it is not an M13 planned-face result. | Euler293 census measured on accepted input; saved492 census is a diagnostic only on a provisional state. Euler input decode12 remains unsupported for Classify; no result acceptance inferred from saved492. |
| M4 | One interior split attempt took 0.101836 ms and was refused: split point was not a distinct exact on-edge point. | Actual-operator JSON above; one interior edge only. | No accepted interior-edit cost measured; this is refusal-call timing only. |
| M5 | Euler293: one `BindNewPoint` success, 277.498 microseconds projection time, 7.273 microseconds private-context setup; one of 7,521 need edges attempted. Saved492 provisional-state diagnostic: one success, 113.472 microseconds projection, 963.039 microseconds context setup, one of 6,759 edges. Euler input decode12: one success, 50.026 microseconds projection and 7.102 microseconds context setup (build520); this supersedes the earlier build515 one-sample value. | Geometry summaries; fresh cache contexts. | One sample per input only; full need-edge projection census not measured. Saved492 values are provisional-state diagnostics. |
| M6 | 6/32 wall splits inside the old cavity union (18.75%); all 32 proofs completed. | Actual-operator JSON above; exact containment against each split's old cavity cells. | Partial first-32 candidate population; 82,527 wall candidates remain unvisited. |
| M7 | Complete local edge shell: 2 ranks 917,080 / 923,210 summed rank-local visible edges (99.3360%); 917,080 / 920,145 globally unique edges complete on at least one owner (99.6669%). 4 ranks: 911,273 / 929,077 (98.0837%) local; 911,273 / 920,145 (99.0358%) global. 6 ranks: 908,175 / 932,225 (97.4202%) local; 908,175 / 920,145 (98.6991%) global. | `locality2/4/6` measurement JSONs and owner maps in the attempt directories below. Population is accepted Euler293 imported then partitioned through production `Repartition` API with unit weights. | The local-visible denominator sums duplicated interface entities across ranks. The global denominator deduplicates edge IDs; global complete means the shell is complete on at least one owner, not on every rank. Fixture reports shell availability, not executed edit success. |
| M8 | Reused accepted F3b profile: 204 window rows and 180 timer rows. Event 1 sums: busy-window wall time 2,536.023 s across ranks; join-wait exclusive timer sum 3,347.298 s; named non-join exclusive 5,200.207 s; unattributed 127.678 s. Event 2: busy-window wall 1,327.003 s; join-wait exclusive 2,111.766 s; named non-join exclusive 3,149.188 s; unattributed 52.405 s. | `N5/existing_profile.md`, `existing_profile.json`, `existing_quality_windows.csv`, `existing_quality_timer_rows.csv`; existing accepted F3b evidence, no new run. | Reused profile, not a new handoff-step-2 profile. `busy_seconds` is wall time in the marker interval, not CPU time. Rank-summed busy and timer values overlap in execution intervals; join and tracked collectives are non-additive; timer rows are nested. These figures compare reported activities, not exclusive job compute vs wait wall durations. |
| M9 | Local ownership rule (`item1`) split wall-edge share: 2 ranks 81,858 / 82,641 summed local edges (99.0525%); 81,858 / 82,559 globally unique wall edges eligible on at least one owner (99.1509%). 4 ranks 80,553 / 82,802 (97.2839%) local; 80,553 / 82,559 (97.5702%) global. 6 ranks 80,175 / 82,861 (96.7584%) local; 80,175 / 82,559 (97.1124%) global. Collapse wall-vertex link share: 2 ranks 27,291 / 27,682 local (98.5875%); 27,291 / 27,599 global (98.8840%). 4 ranks 26,719 / 27,843 (95.9631%) local; 26,719 / 27,599 (96.8115%) global. 6 ranks 26,560 / 27,904 (95.1835%) local; 26,560 / 27,599 (96.2354%) global. | Same locality receipts; `item1` requires a complete local shell plus wall-face ownership: split tests every vertex of changed wall faces; collapse tests the removed vertex’s wall-face link. | These are locality/ownership counters, not executed wall edits. The distributed body-contact proof share requested by M9 is not emitted and remains unavailable. Local/global denominators have the same distinction described for M7. |
| M10 | M10 n1 has zero nontrivial growth; n2 logs partial growth layer 0=2 lifts, layer 1=4 cumulative lifts before dependency guard; n4 records 12 R2 row lifts but no completed G growth layer. Face altitudes, wall stacks, MakeRoom counters, quality and topology are detailed in `N5/stack_measurements.md`. | Five successful build520 face-height receipts; `N5/stack_measurements.json` and `.csv`. M10 n2 is the smallest tested nontrivial partial-growth case in this sample, not a mathematical minimum. Guarded G column totals remain unavailable. | Partial fixture measurements; no complete M10 column census for n2/n4 and no accepted BL mesh. Surface-preparation need and requested row-transaction vertex-set growth count are not supplied by these receipts. `R2_max.closure_vertices` is only the accepted outer seed-set count. |
| M12 | Control (2T<G): 3,734 positive cells; meet (2T>G): 3,914 positive cells. Both have 50 columns, first-child wall-face altitudes within the recorded 0.1 h0 tolerance, and detailed per-wall stacks/stops/q/J/size data in `stack_measurements.md`. | Build520 M12 control and meet receipts; flat-channel fixture, one rank. | Fixture result recorded; both have q/J/size debt and `complete=false`, so neither is an accepted mesh. |
| M13 | Not measured. Euler293: 10,801/10,801 old current-demand incident faces pass within tolerance. Saved492 provisional state: 2,736/5,728 old current-demand faces pass and 2,992/5,728 are over tolerance (0 query errors). These are separate old-face diagnostics, not M13’s planned-new-face denominator. | `N5/cases/n5_d9_geometry_euler293_20261116T033059_2608544/d9_geometry_summary.json` and `N5/cases/n5_d9_geometry_saved492_20261116T033718_2624292/d9_geometry_summary.json`. Summary explicitly marks M13 unavailable because current-demand faces do not identify planned new faces. A qualified planned-face set requires the later design steps 8.3–8.7, which are expressly out of scope for this work order. | No qualified S1 plan exists; design steps 8.3–8.7 are outside this work order. Saved492’s 2,992 over-tolerance results concern old current-demand faces only and do not become M13 failures. Do not treat old-face proof as M13 or proceed to design the missing plan here. |
| M14 | Not measurable from current production source: no inverse `Unlift` operator is implemented or numerically qualified. | N3 source check [`Handbook/ADAPTATION_DESIGN_SOURCE_CHECK_20261115.md`](../../../../../../../Handbook/ADAPTATION_DESIGN_SOURCE_CHECK_20261115.md), item 17 and lines 58–59: removal must account for recursive apex closure, sector/gap/junction records, overrides/extras, and certification; no inverse implementation exists. | Source capability gap; do not report a refusal share or imply a current measurement can be run. |
| M16 | Not measured as D requests it. | No complete F3 quality timer-tree receipt split by callsites (a)–(g), nor per-wall candidate export/projection/q/J/move/guidance breakdown. | Pending complete quality profile. |
| M18 | Failed N1 emitted all 12 rank/event stage-exit rows: `distinct_facets` 5,104–22,944; `prepared_facet_mean_bytes` 3,561.89–24,894.67; `distinct_scope_keys` 6,826–25,185; `scope_builds` 7,317–30,713. | `N5/failed_N1_M18_rows.csv`; per-rank/event stdout line and stream SHA are included. | Values exist but are UNQUALIFIED because N1 failed event-2 identity and was reverted. Do not reuse as accepted result/performance evidence. |
| M20 | Failed N1 emitted all 192 window rows. Event 1 (120 rows): private work 0–41,635, sum 1,879,372; busy-window wall 0.000000925–63.7358465 s, sum 2,472.942365 s. Event 2 (72 rows): work 0–36,560, sum 961,920; busy-window wall 0.000019032–44.371448614 s, sum 1,347.674433 s. | `N5/failed_N1_M20_rows.csv`; rank/event/census/round and source line are recorded. | Values exist but are UNQUALIFIED due N1 result failure. Busy is wall time, not CPU; sums do not establish equal work-unit cost across ranks. |
| M21 | Failed N1 stage-exit rows contain run-level `job_peak_rss_lower` (event 1 14,007,463,936 B; event 2 17,343,528,960 B; unqualified). Requested transition-local before/after current RSS, `ProcessPeakRSS3D` pair, and exchange bytes / `max_exchange_work_bytes` are `NOT_EMITTED`. | `N5/failed_N1_M21_rows.csv` and `N5/failed_N1_counter_inventory.md`. | Keep distinct: some cumulative peak fields exist, but required before/after transition pairs and exchange counters are missing; all source-run values unqualified because N1 failed event-2 identity and was reverted. |

## Locality receipt paths

All three locality fixtures used the accepted Euler293 input and production `Repartition` with unit weights. Build521 test binary SHA-256 `382dc7142320d2520dd665858fb47492990261e67bae7890717a67138c5ec48b`; shared source-pins receipt SHA-256 `e5f7ffe147eadaf8d64b821bcb90395fc026107ce7ed57d1f4464e27befc36da`.

| Ranks | Attempt | Summary artifact SHA-256 |
|---:|---|---|
| 2 | `N5/cases/n5_d9_locality2_20261116T034905_2653395/native_d9_locality_m2.json` | `eef55ca851cc416da945d8b7850dae60d2c729e7ec733dcd4a1a39252216da40` |
| 4 | `N5/cases/n5_d9_locality4_20261116T035038_2657289/native_d9_locality_m4.json` | `27fff2303d8cda0b102e88e56208bfc3a2b40b14967b4122c5bc63596583cd68` |
| 6 | `N5/cases/n5_d9_locality6_20261116T035242_2662583/native_d9_locality_m6.json` | `ded94b4f7e2ebe5a578108f61b704c3b95e1bc99df29d4ba82e46d22dcf03877` |

The first locality2 launch failed in fixture setup at `map::at` before producing owner data; a test-only marker-to-facet lookup correction was applied. The recorded successful 2-rank retry and 4/6-rank cases completed with verified pins and resource receipts. No production source changed.

## Shared measurement provenance

Face-height M10/M12 receipts used build520 test binary `8a77a3fb0e294ecbb143410c08f7fc995dffc88818f8c648653dbf9a3d1bfede` and source-pins receipt SHA-256 `29dfaf2c251de2d53ea38c42f9ead09fb5ff825879e81d062d24fec1bd18101d`. The original Euler input geometry sample used the build515 test binary; the repeated decode12 receipt uses build520 binary `8a77a3fb0e294ecbb143410c08f7fc995dffc88818f8c648653dbf9a3d1bfede` and source pins above. Actual operator receipt used build520 test binary above. Exact case paths and artifact hashes are in the per-case receipts and `stack_measurements.json`.

Unmeasured geometry/locality work remains pending: M3 Classify on the Euler input is unavailable because its source receipt has no parameter payload and Classify support is false; full M9 distributed contact-proof share (locality `item1` counts are available, but contact proof is not); complete all-candidate M2/M6 coverage; new handoff-step-2 quality profile M8 and component timing M16 (M8 reuses the existing accepted F3b profile); and M14 remains unavailable because the source has no implemented/qualified inverse Unlift operator. Do not interpret this draft as completing N5.

## Cut findings, exceptions and function sizes

Found during the cut: uncaught acquisition throw; ignored import_source requests;
receipt-sort ties depending on input order; asymmetric callback presence; retained
geometry cache affecting subsequent partition weights. These were preserved, not fixed.
The only written C exception is EngineLegacyWall's80-line limit; three growth copies
must remain separate. No cut is retained after8039fc1b59, so there is no accepted set
of ten longest newly cut functions. N5 diagnostic/helper scan covers158 active functions, all at most80lines; retained proposal drafts are excluded. Current top ten (unrelated to the reverted Adapt cut) follow.

| Function | Lines | File |
|---|---:|---|
| produced_artifacts | 79 | `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/run_d9_units.py` |
| D9MeasureFaces | 77 | `UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp` |
| D9Measure | 72 | `UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc12.inc` |
| D9MeasureWallEdges | 68 | `UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp` |
| D9WriteOperatorReport | 63 | `UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp` |
| D9WriteSummary | 62 | `UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp` |
| D9MeasureProjection | 59 | `UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp` |
| D9FirstHeights | 57 | `UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc12.inc` |
| run | 55 | `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N5/run_d9_units.py` |
| D9Columns | 48 | `UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc12.inc` |

Source hashes and precise bounds are in `N5/reports/function_lengths_current.json`.
[Complete C ledger](ADAPTATION_ADAPT_CUT_20261115.md).

## Readings and fixture repairs

The14N3 readings/qualifications above, the18N4 source-check entries and12N5 entries
are preserved in their original ledgers. The latest NIGHT order overrides H's dependency
on a cut engine for N5; F3b is used after N4failed. No reading waived a result gate.

R3 repair details: `R/p7p8_r3_20261115/fixture_corrections.json`, helper receipts named in
its report; N4 comparator19selfchecks and unit/cut helper repairs inN4 ledgers;
N5 `fixture_repair_index.md` consolidates all counter, stack, geometry, descriptor, operator and launcher repair receipts and build511–521 setup outcomes. The latest locality setup correction resolves vertex-index face records to authority facet IDs before reading markers. No production policy/gate/tolerance repair is included.

## Terminal state and next authorized work

Pending N5completion and host verification. Design8.3–8.7, A5/B5, job-derived cache quota,
BL/RANS acceptance and other production redesign remain outside this work order.
