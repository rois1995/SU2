# Adaptation projection findings

Last updated: 2026-10-29.

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
