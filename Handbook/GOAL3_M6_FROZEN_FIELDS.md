# Goal3 — actual frozen M6 fields and the primary integration strategy

Full Goal3 ACTIVE/incomplete. Published producer/header restored exactly to
977691bc4e after the failed primary-column experiment. All owned jobs terminal.
Current executable still contains the withdrawn experiment: rebuild with the
next connected change; never treat it as restored-source proof. The complete M6 mesh has never entered native construction.

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

NEXT: replace nested per-ray stopping with a coupled cell-wide angular/radial
error ledger. Retain the geometric hidden-support guard and existing tolerances,
point/work/depth bounds and physical metric. Test the3 exact fields before CFD.
A future release of an unresolved support guard requires a conservative bound,
not another chart fallback or a looser tolerance. No such bound is claimed now.

## Independent assistance and unresolved mechanisms

Both read-only Claude reviews returned confirmed claude-opus-5-5, high then medium,
with no nested agents/permission denials. The second is CLAUDE_CAPTURED_REVIEW.md.
It confirms the parallel chart geometry is not mathematically hidden-only, but
predicts reuse may not suffice. Suggested nearest-feature cuts are an experiment,
not established proof. Per-triangle face/edge/vertex Voronoi planes alone do not
partition global competition between nonadjacent triangles; quadratic bisectors
may matter. Rejecting valid concave fields is not a way to complete the goal.
RotateNormal already handles unoriented normals; no transport-sign bug was found.

Nested radial/angle error allocation, nearest-feature crossings, excessive base
cuts and sensor-intersection kinks remain distinct possible causes. The withdrawn experiment and exact error-budget states motivate a
coupled global error ledger; no successful implementation is claimed yet.

The old frozen_sensor.csv checkpoint is a2D unit fixture. A generic whole-trial3D
export was not found. Current cell replay avoids further CFD for these targets;
a future compact full-trial checkpoint could enable a whole-mesh quadrature scan
without repeated flow setup. It is NOT implemented or requested from the cluster.

Full geometry/features/interactions/gradation/transfer/accuracy/lifecycle cost and
all original Goal3 acceptance gates stay open. No accepted M6 grid or speedup claim.
