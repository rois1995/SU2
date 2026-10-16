# Goal3 — actual frozen M6 fields and the primary integration strategy

Full Goal3 ACTIVE/incomplete. Current work has moved to the
[complete first-trial checkpoint and integration reassessment](GOAL3_M6_COMPLEXITY_REASSESSMENT.md).
Production producer/header restored and binaries freshly rebuilt; a test-only
point-query extension supports independent references. Historical failed/stale
binary notes below refer to412230, superseded by that rebuild. The complete M6
mesh has never entered native construction.

## Exact actual-field counterexamples

`integration_evidence/native_3d_bl_v1/oneram6_frozen_v2` reuses byte-identical
configuration, original reference and saved120-step conservative flow from v1,
with ONE initial CFD iteration. Printed flow/Hessian counts match v1. Normal
reference gate passed before launch: all4 apexes agree4.432e-5; radial16/24 on
apex3 agree1.223e-7. Native1335-point normal error worst1.994e-4 (<5e-4 pilot).
The nominal1e-4 error indicator is not a rigorous bound. No independent full
sensor-plus-BL density proof follows from this normal-only audit.

The event was deliberately stopped after capturing actual local failures, rather
than waiting for a completed global failure election. Exact runtime record in
CLocalFailure was obtained passively, with no inferior function calls:

- rank0/cell329: radial work limit,22193 retained/188 panels/ray5;
- rank1/cell157: point limit,65411 retained/652 panels/ray4;
- rank3/cell88: point limit,64957 retained/202 panels/ray6;
- rank2 was still computing (11 completed rules/49524 retained points).

All are first complexity trial, log scale-.47810758149935007/core.056676963474793819.
The bounded new runtime failure record is now exercised and parsed successfully.
No final complexity normalization, remesher/candidate/accepted mesh/transfer.
SIGTERM targeted only the verified owned MPI launcher; exit1 is deliberate after
confirmed local failure, not successful natural completion. Logs, four passive
snapshots, exact3 trial fields and termination reason remain in v2.

## No more CFD needed for these three targets

`oneram6_quadrature_v1/captured_cells_v1/cell88|cell157|cell329` contain complete
native-oriented original wing plus recorded4 source coordinates/nodal tensors/core
in replay.dat. Input coordinates and node sets are independently matched against
Coarse.su2/global element ordinal. All raw binary64 coefficients are retained.
The reader permits64eps*matrix-norm asymmetry (observed2.27e-13/4.55e-13), checks
finite/SPD input and never symmetrizes the recorded tensor. Version2 replays now
use one effective band setting, eliminating duplicated work. Version1 historical
normal protocol is unchanged. No production case/flow-class dispatch was added.

The exact cell329 replay reproduces the runtime failure verbatim:22193/188/ray5,
575930 callbacks,6.23s on one core. Baseline source/header/test snapshots, executable
hash and input identities were saved before the new build. Current-root replay
outputs are not automatically accepted results. Preserve baseline_* evidence.

Independent exact-triangle queries using the whole wing give vertex distances:

-157:0,.004065,.012218,.005705 (one wall vertex);
-329:.009784,.032306,.011020,.007169 (no wall contact, crosses fade);
-88:.009904,.010476,.005417,.014677 (no wall contact, crosses fade).

Point/edge-midpoint/face-centroid distances are diagnostics, not a proof of full
entity contact. They rule out explaining all failures with a wall-vertex chart.

## Withdrawn primary-column experiment

The geometry-based primary routed every nonempty active wall support through the
existing wall-normal entry/exit columns, using the closest support facet for the
visible seed. It changed no physical target, fade, first height or limits. Build
passed, but the3 actual fields fail:329 work26191 points/288 panels/8.02s;
157 timeout90s;88 angular depth32/12.36s. Sources, source/build/input pins and
all results are preserved under captured_cells_v1/parallel_primary_rejected and
each cell's parallel_* files. The producer/header were restored byte-for-byte
from published977691bc4e. No broad regression or repeated CFD for this prototype.

Read-only debugger receipts expose two mechanisms:

-329 spends its work on a ray with estimated remaining error1.41e-8 while the
 current partial cell integral is.252795. The aggregate cell estimate still
 exceeds tolerance: this does not establish that releasing that ray alone fixes it.
-88 reaches depth32 because its geometric support guard remains unresolved.
 The sampled panel value1.57e-12/error5.99e-23 cannot bound an unsampled fine
 wall contribution. Never dismiss hidden support from tiny sampled values alone.

## Cell-wide ledger also rejected

The subsequent connected controller retained the original chart routing/support
guard and physical field. All angular/radial contributions shared one cell error
ledger, with aggregate radial1e-5*(fine+comparison) and total1e-4*fine estimates.
Global/work/depth/point bounds remained unchanged. Comparison rays retained only
scalar estimates and wall-hit lists; fine rays retained positive3/5-point sets.
Finite weights/radial values/errors were checked before heap insertion and cached
priority entries carried revisions. This is numerical estimation, not enclosure.

The3 actual fields fail, preserved under captured_cells_v1/cell_ledger_v1:
329 point cap65424/panel1074,947699 callbacks/22.53s;157 point cap65430/panel776,
949117 callbacks/89.45s;88 timeout90s. These contended low-priority diagnostic
wall times do not establish performance. No accepted rule/mesh/transfer follows.

One read-only GDB failure state shows current partial mass.260515, angular
estimate1.75884e-5 and radial2.61199e-4, no unresolved support,65424 points.
Angular selection continued even after its reserved allocation was satisfied.
The scheduling eligibility fix is captured and asserted against those exact
values; nevertheless v2 still fails329 (65497/panel1078/948270 callbacks/21.69s),
157 (65430/panel776/949117 callbacks/84.81s),88 timeout90s. Both sources/results
are preserved. No more queue weight/chart/local fallback variants now.

The GDB run shared a testcase cwd and left two CSV streams unflushed. Original
saved hashes exposed this; both exact CSVs were recovered from previously completed
v2 files matching those hashes. The original receipts remain unchanged, and the
recovery record preserves the empty debugger outputs. Future debugger cwd must
be separate. Debugger maximum RSS includes symbols/GDB; it is not native RSS.

Producer/header restored exactly977691/11a539. Test executable contains ledgerv2;
CFD executable contains the earlier rejected parallel route. Neither is proof of
restored-source runtime behavior. Rebuild with the next connected implementation.
All owned jobs terminal. No broad regression or MPI/CFD campaign for these failures.

NEXT: reassess representation and whole-complexity error allocation before another
implementation. A complete original-P1 first-trial field checkpoint would allow
whole-mesh offline integration without repeating CFD. Its passive extraction now passes in oneram6_frozen_field_v1; see the current reassessment handbook. Current cell fixtures remain exact useful counterexamples.
Uniform per-cell relative accuracy may be inappropriate for a global scalar sum,
but changing allocation is not permission to lose thin-band mass or weaken mesh
acceptance. The existing scale solver stops at1e-6 while quadrature uses a1e-4
numerical indicator; any new design must account for both and independently check
accuracy. Unobserved BL support needs conservative mass handling. No such mass
certificate or global accuracy guarantee is established by these experiments.

## Independent assistance and unresolved mechanisms

Three completed read-only Claude reviews confirm claude-opus-5-5: high, medium,
then medium (CLAUDE_LEDGER_REVIEW.md). No nested agents/permission denials; the
ledger review had all tools disabled. Its material findings were addressed before
actual-field checks, without treating review as validation. A fourth structural
high-effort request timed out360s with no returned review; saved attempt records
its provisional157-timeout and omitted1e-6-scale-tolerance prompt errata. Do not
attribute any global-budget conclusion to that incomplete attempt. The second
completed review is CLAUDE_CAPTURED_REVIEW.md.
It confirms the parallel chart geometry is not mathematically hidden-only, but
predicts reuse may not suffice. Suggested nearest-feature cuts are an experiment,
not established proof. Per-triangle face/edge/vertex Voronoi planes alone do not
partition global competition between nonadjacent triangles; quadratic bisectors
may matter. Rejecting valid concave fields is not a way to complete the goal.
RotateNormal already handles unoriented normals; no transport-sign bug was found.

Nested radial/angle error allocation, nearest-feature crossings, excessive base
cuts and sensor-intersection kinks remain distinct possible causes. Both withdrawn strategies and exact budget states motivate structural
reassessment; no successful whole-M6 integration is claimed.

The old frozen_sensor.csv checkpoint is a2D unit fixture. A generic whole-trial3D
export was not found. Current cell replay avoids further CFD for these targets;
a complete compact first-trial checkpoint now enables whole-mesh offline work
without repeated flow setup; see the current reassessment handbook. No cluster request.

Full geometry/features/interactions/gradation/transfer/accuracy/lifecycle cost and
all original Goal3 acceptance gates stay open. No accepted M6 grid or speedup claim.
