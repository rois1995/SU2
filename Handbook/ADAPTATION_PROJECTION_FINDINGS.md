# Adaptation projection findings

Last updated: 2026-10-29.

Current build114 passes compilation; full M6 with connected staging is running
(session84714, euler_build114_classified_stages). Sources/tests/helpers pinned.
The smaller workflow rejects by operator stagnation, not exhaustion. Its within-
quality non-regression assertions pass;139 private cells retain q/J1/1, size16 and
geometry32 failures. Curved transfer/restart remain unqualified.

Fresh diagnostic115 is terminal with verified pins: collapse134792→1 obstructs
cell135178,134792→5 cell116744,114056→1 cell174089. Exact validity rejects these
proposals with30/42 work and no exhaustion. Captured worst cell122900 has four
vertices on the same wall facet and tiny positive determinant. Existing interior
edge-removal on edge1–134792 has a complete6-cell ring; investigate it before
claiming unavailable geometry or adding an operator. Raw logs and provenance:
[serial replay115](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/serial_replay115/).
The current MPI focused run has9/10 passing cases per rank; its only failure is a
new scalar fixture expecting Progress where Absolute correctly rejects Shape.
Correct that assertion after pinned M6; production gates remain unchanged.

Build106 PASS114.227s,1295 pins unchanged, compiles the connected failure-path changes; no complete
workflow is accepted. Build103 capture of the actual M6 fatal query records the
current binding, body/sheet/feature scope,9 candidates, raw nets, seed/final
parameters and dispositions. The fatal file is
[m6 replay](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build103_exact_projection_capture/projection_failure.2.0.txt).
All7 permitted wing candidates failed convergence;2 candidates on another sheet
were excluded correctly. On identical fixed nets, normalized local coordinates
make6 converge with the existing minimizer; the hint foot is4.78e-11 from the query.
[Replay source/results](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_projection_replay103/)
retain all candidate results. No fitted geometry, solver fallback or limit increase.

Build104 uses that local frame in the shared cubic projector, canonical bindings
for retained ordinary identity, and physical feature classification before costly
feature-route lookup. Actual route continuity and publication/decode/restart checks
remain. Typed unavailable projection is a conservative candidate refusal; invalid
backend/state/resource errors remain explicit through the existing distributed vote.
An unavailable sample remains unknown/infinite geometry debt and cannot establish
final acceptance. Source review confirms vote precedes publication; runtime tests
remain pending. Build105 compiled successfully in180.272s with1295 pins unchanged;
build104 focused run exposed three incorrect new fixtures (open surface, feature
segment identity, initialized sensor attributes), corrected without changing
production acceptance. Review also rejects nonfinite candidate geometry debt and
keeps exceptions classified as backend failure. Earlier build102 evidence below is historical, not current validation.

Build106 focused projection paths pass16 cases/547 assertions64.187s. All28
compiled-filter selections include the required cases, including16 serial projection
paths and2 distributed paths (selection audit is not execution). Actual smaller
workflow FAIL89.521s: both curved variants reach331 private edits,83 moves,
39 reconnections,41 insertions and117 private cells, then exhaust unchanged
67,207,168 work before transfer; geometry debt19 and metric/height debt25 remain.
Construction samples still report no q/J measurement. Log-wide exact-key work
(including both curved variants and fresh replays) is BindingParameters15.24m and
ReferencePointKey13.77m, versus31.5m/37.5m at build102; trajectories differ, so
this is not a controlled per-event speedup claim. First collapse replay round90
builds, needs zero exact atlas work for unchanged surface, and error.00062553<.001;
recorded rejection is oversized edge/removal policy, not unavailable geometry.
M6 build106 is running with exact query capture. No complete curved transfer or
restart accepted. [Mapped summary](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build106_mapped_summary.json).

M6 build106 terminates1870.389s, solver0/nativefalse, all pins unchanged and ranks
terminal. Native1802.97s/import171.32s/construction1617.64s:11567 private edits,
307011 private cells, two full collapse/split pairs, no improvement stage/transfer.
The second collapse pass costs~716s versus602s first.154 refusal records contain
46 distinct query/scopes, maximum12 repetitions. Every record has an evaluated
original-model foot within1.25e-5 (largest best distance1.80e-8), but stationarity
fails. This is a captured overly strict numerical success criterion. Sampled tree
RSSmax3.43GiB; admitted bytes are not RSS. [M6 cost/capture summary](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build106_m6_summary.json).

Build107 is compiling the connected correction: use the existing conservative
bound to recognize requested spatial accuracy after body/feature validation;
otherwise still require convergence. No second solver or fallback is added.
Captured curved collapse removes4 cells and lowers changed-edge debt .189605 to
.029831, although it adds edge1.973>1.8. Validity-only coarsening may now take that
strict debt decrease; absolute admission and final size gates stay unchanged.
Focused regression and actual workflows are pending; no complete mesh accepted.

Build107 projection paths pass20 cases/597 assertions72.763s; MPI6 passes5
cases/rank6.916s, all pins unchanged.28 compiled selections include all required
cases. The complete smaller workflow still FAIL101.957s: both curved cases make
323 edits/87 moves/23 reconnects/44 insertions and109 private cells, then exhaust
same67,207,168 work before transfer.17 geometry and19 metric/height debts remain.
Its first failure has moved to round109: fresh collapse still builds, error.0003124
<.001, work6159/no exhaustion; no conclusion of geometric impossibility follows.
[Mapped107 evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build107_mapped_summary.json).

The staged driver makes106 collapse/split pairs before its only quality stage;
M6 reaches none. Its unproductive-pair handoff is a custom rule. Build108 removes
that requirement and uses the existing improvement stage after each complete pair,
consistent with SCOREC ma.cc adaptVerbose. No local fallback or cap increase.
The opposing-length distributed workflow now checks improvement after the first
pair while retaining complete final acceptance. Build108 validation is pending.

Build108 PASS790.893s/1295 pins unchanged; host I/O contention dominates linking.
Opposing-length serial workflow passes8.364s and now reaches improvement after
one pair. MPI6 focused8 cases/rank pass11.451s; current-field122 cases pass10.413s;
28 filter selections pass; all10 DC18 geometry groups pass with current header.
These supporting checks do not establish robust curved construction.

Actual mapped108 FAIL43.323s (pins unchanged), versus101.957s build107. Both curved
variants enter8 quality stages/7 construction returns, make75 edits/44 moves/19
reconnections, leave41 private cells and exhaust unchanged67,207,168 work;8 geometry
and25 metric/height debts remain. Last refusal is a cone/interface obstruction,
which does not prove no admissible operation exists. The first stored repair
failure has empty reason and no replay because it is actually an initial streaming
probe before any repair was proposed. Build109 excludes that probe from failure
capture and adds fresh MoveVertex replay using the existing captured arguments.
No new operator, representation or acceptance rule for this diagnostic correction.
[Mapped108 summary](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build108_mapped_summary.json).

Build109 passes373.183s/1295 pins unchanged. Actual mapped workflow still fails
34.902s at unchanged work; this diagnostic change does not claim mesh progress.
First real repair is MoveVertex at genuine corner0, round3. Independent fresh
replay refuses the same physical corner, with zero exact-atlas work and no
exhaustion. Thus this is an unavailable move, not invalid backend state, proof of
an impossible mesh, or the explanation for arithmetic exhaustion. Other proposals
continue. Full final quality/geometry closure remains absent.

Build110 adds rank-local phase receipts to existing timing output: cached q/J/
size/geometry debt, association/working-face/piece counts, maximum ratio storage
limbs and cumulative atlas work. No geometry/quality measurements are added, and
window callbacks are excluded from this scan. The counts are rank-local state
views, not unique global counts. They will distinguish retained representation
growth from repeated verification and actual progress before any cap revision.
No algorithm or limit changes in this diagnostic step.

Build112 receipts validate descriptor bounds in the smaller event: at most19
associations,34 working faces,21 trace pieces; maximum ratio storage2 limbs.
Last improvement starts at atlas work9,834,895 and exits67,207,168 while q/J bad
cells grow2/2→6/5, q debt .07617→1.24217, J debt .32706→1.51453; size debt falls
15.5194→9.7759, geometry debt .0004811→.0004023. About85% of work is spent there.
This identifies mixed improvement goals/repeated verification, not history growth
in this event; it does not qualify repeated accepted adaptations.

Build113 reuses quality_valid for staged improvement demand and synchronizes
membership on phase entry, publication and quality refresh. Combined validity
still controls final acceptance. Public standalone Repair remains broader. Staged
shape repair no longer gets geometry-only progress promotion, and absolute Repair
cannot exchange unacceptable shape for size progress. The direct regression keeps
validity-only construction permissive and verifies shape-gate closure. Mixed-body
serial/MPI regression checks a shape-good/size-bad rank is not selected for shape
repair while another has actual bad cells. Validation pending, no limit change.

Build113 supporting opposing-length/mixed-debt4 cases pass7.290s;28 compiled
selections include all intended cases. Actual curved workflow FAIL16.663s at4096
proposals, atlas work27,386,472 (below cap). It reaches7 quality phases; last
quality q/J bad cells rise1/2→7/5,47→142 cells. This demonstrates the remaining
admission loophole, not arithmetic history growth. Adapt retains ValidityOnly for
geometric builders throughout; its helper's Absolute rule is not native admission.
Build114 uses existing metric_trial for both local and streamed stage acceptance:
ordinary staged repair must improve measured worst shape or pass final shape,
without increasing metric excess. Requested-height repair route is unchanged.
No new construction measurements or gate/cap changes. Captured serial no-height
quality phases assert their actual clipped worst shape score does not regress.
[112 phase evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build112_mapped_summary.json),
[113 phase evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build113_mapped_summary.json).

Build114 passes261.364s with1295 pins unchanged. Actual mapped workflow still
FAIL30.048s: operator stagnation, not exhausted resources (35,630,824 atlas work,
2062 proposals; limits67,207,168/4096). Within-stage clipped shape-score assertions
pass; only two final COMPLETE requirements fail. Final139 cells include one q/J
sliver and32 geometry/16 size debts. Independent exact-rational determinant decode
finds cell122900, q4.995e-12/J6.844e-18; all4 vertices have boundary associations
on facet1, with2 physical faces. Opposite interior edge1–134792 has complete ring6
(within optimizer max8). This identifies a precise boundary-sliver repair target,
not proof of no admissible operation. [114 workflow](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build114_mapped_summary.json),
[exact saved-cell diagnosis](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/build114_worst_cell.json).

MPI114 focused has9/10 cases per rank passing; the new Absolute repair test has
incorrect expectations that progress is measured before its existing shape gate.
It correctly rejects the poor cell as Shape, not Progress. Correct those assertions
without changing acceptance. Diagnostic115 uses existing CollapseVertex on saved
final state, fresh work per proposal and unchanged original geometry, no pre-op
surface-error certificate. It is not an exact earlier transaction replay or accepted
mesh; data inputs are pinned at diagnostic launch. No new operator/cap change.

The connected projection implementation shares the point, patch and derivative
arithmetic used by `PNTriangleSurface::Minimize` through detail helpers. The
backend numerically projects onto fixed surface nets and uses the existing
`NearbySurfaceFacets` ADT to select candidates. Body and sheet constraints and
current association hints remain in force. Its conservative distance interval is
`[0, outward distance + realization error]`; it does not claim a global minimum.
Certified `ClosestCurved` and the final two-sided geometry gates are unchanged.
Build101 passed in 159.273 seconds with 1,295 pins unchanged. The geometry backend
run passed 12 cases in 61.126 seconds. Build102 passed in 72.187 seconds with
1,295 pins unchanged; its geometry backend passed 13 cases in 62.154 seconds. All ten
existing DC18 groups pass with the revised projector. These checks do not validate
the full workflow.

Build101's small mapped workflow failed in 27.016 seconds: both curved Euler and
RANS stopped before transfer with `Numerical cubic projection did not converge on
facet 5`; the flat counterparts passed. Each curved run reached 156 of 4,096 work
units, with 96 splits, 49 collapses, six geometry progress and 77 size progress;
its cached demands were zero geometry cells and edges, 67 metric-or-height cells
and nine metric edges. M6 Euler terminated in 268.155 seconds with solver exit 0
but native completion false, reporting the same failure kind on facet 16607;
all ranks terminated and pins remained unchanged. See the
[mapped run](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/cases/mapped_work_trace_20261028T183331_2833660/run.log)
and [M6 evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build101_numerical_projection/).

The captured facet 5 query was `(0, 0.80499999999999994, 0.5)`: hint facet 3
had already converged at distance 1.24e-16, but a failed facet 5 candidate aborted
the search. The shared minimizer applied a stricter inward-noise threshold before
its existing KKT stationary tolerance. An adjacent linear-face reproducer changed
from convergence 0/1 to 1/1 after applying the stationary test before the inward
push, with distance 1.11e-16. The revised projector selects converged eligible
feet, skips an individual failed candidate, stops when its conservative upper
bound meets the requested accuracy, and fails if none converge. It supplies no
flat or unconverged fallback. The reproducer, captured query, before/after results
and hash-verified build101 header snapshot are archived under `debug_projection101`.
Certified `ClosestCurved`, the two-sided geometry gates and quality gates remain
unchanged. No complete workflow is yet accepted.

The subsequent build102 small mapped run passes that projection failure and
enters improvement, but both curved contexts exhaust the shared 67,207,168 atlas
work limit before transfer. Each reaches 245 private edits, including 26 moves,
and reports 11 geometry cells, 21 metric-or-height cells and 254 repairs still
outstanding. Log-wide exclusive work, including both contexts and diagnostic
replay, is dominated by `ReferencePointKey` (37.5 million) and
`BindingParameters` (31.5 million); it is not a per-event budget. These calls
serve association and feature-trace checks. The classified-cavity design retains
actual features, body separation and final geometry gates, but does not require
persistent smooth rational paths. The aggregate does not prove an individual
check removable. The [run](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/cases/mapped_work_trace_20261028T185636_2889074/run.log)
and [fresh failure replay](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/cases/mapped_work_trace_20261028T185636_2889074/cubic_first_failure_0_rank0.json)
are retained; no final mesh is accepted.

Build102 M6 Euler terminated after 292.221 seconds with solver exit 0, native
completion false, all ranks terminal and pins verified. Its blocker was `No
converged numerical projection on prescribed body/sheet/feature`, first failed
facet 11053; the last prior local rejection was the size/progress veto on edge
13236,19954. Native time was 227.7 seconds (import 172.78, refine 40.3); private
edits per rank were 13, 13, 8, 10, 12 and 14, with no complete length pass. The
query has not been captured, so neither feature inadmissibility nor Newton
failure is established. See the [M6 evidence](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/m6_runs/euler_build102_stationary_projection/).
The fixed stationary test and converged-foot selection remain current; measured
reference-key and feature validation costs remain. Final gates are unchanged and
no work caps were raised.

The build100 small mapped workflow exposed construction cost and scheduling debt.
Both curved Euler and RANS stopped at their time budgets before transfer, after
168 work units, 151 accepted private edits, 98 splits and 53 collapses. Selection
spent about 52–54 seconds in collapse geometry prechecks and filtered 6,113
collapse constraints; 23,432 length-pass candidates were deferred. The final
cached census had zero geometry cells or edges, 63 metric-or-height cells and
five metric edges. Shape improvement had not started. A diagnostic on the 141
private cells measured minimum q 0.001435, minimum J 0.0000340, maximum metric
edge 3.422 and maximum surface error 0.000946 against tolerance 0.001. These
measurements are not an accepted mesh or final quality certification.

The fresh replay of the first failed collapse built the proposal using 12,298
work units and certified surface error 0.000629 below 0.001. Its recorded refusal
was that coarsening did not remove cells or introduced an oversized edge. This
shows a size/progress veto for that proposal, not an unavailable operator or a
failed geometry certificate. The time-budget exit leaves feasibility unresolved.
See the [run log](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/cases/mapped_work_trace_20261028T180234_2751451/run.log),
[first failure](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/cases/mapped_work_trace_20261028T180234_2751451/cubic_first_failure_0_rank0.json)
and [terminal receipt](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/cases/mapped_work_trace_20261028T180234_2751451/status.json).

The build100 M6 run was deliberately stopped at 661.041 seconds with pins
unchanged. It does not establish a completed length pass or scheduling acceptance.
The complete native workflow, including final mesh gates, conservative transfer
and restart, remains unaccepted.
