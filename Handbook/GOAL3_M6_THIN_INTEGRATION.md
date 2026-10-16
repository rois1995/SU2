> Current state and live handles moved to [actual frozen fields](GOAL3_M6_FROZEN_FIELDS.md).
> The work below is historical cell13/column-controller evidence; old live-job notes
> are superseded by that topic. Full Goal3 remains active/incomplete.

# M6 thin-volume integration — current connected work

Goal3 remains ACTIVE/incomplete. This is quadrature work before wing remeshing,
not an accepted mesh or proof of viscous accuracy. Published base4f5c8a9c6a keeps
the actual failed run, matching flow checkpoint and QLS workspace fix.

Current case folder:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/oneram6_quadrature_v1`.

## Isolated cause

The full original38800-face wing and source tetrahedron13 are extracted into
wing_cell13.dat, with original mesh/fixture hashes. A hidden test
`[NativeFacetedVolumeReplay3D]` reads this generic complete-wall/cell format via
SU2_NATIVE_BL_REPLAY. No case dispatch or new production configuration.

Unchanged production geometry-first integration fails in0.457s at65536 points,
with zero density-callback calls. Independent all-nearby-triangle distances agree
with every initial probe to roundoff. Two distinct faces37327/37760 dominate the
positive-distance probes; their normals differ22.773 degrees. Source-cell distance
affinity defect1.02707161556e-4 exceeds the first-height scale2e-6 by51 times.
This is real closest-feature geometry, not a new query defect or Hessian issue.

An independent feature-plane experiment preserves volume with6 convex pieces/24
tetrahedra but leaves8 children above the existing distance criterion and maximum
defect1.0201354e-5. It was not introduced as another production recovery. Its
first NumPy integer serialization failure is preserved separately.

## Rejected integration-tetrahedron strategy

Band-first prepartition accepts1192 geometry-only points, but the independently
converged normal-density reference rejects its0.1529%/0.1595% errors. Source and
results are preserved in band_only_rejected_accuracy. Strict piece-local density
refinement then exhausts16384 work, preserved in local_density_rejected_work.
Global error priority/directional bisection passes7 native cases but independent
normal errors5.40e-4/8.19e-4 still exceed5e-4; retained in
global_low_order_rejected_accuracy. The positive degree-five FEM experiment
exhausts65536 points at default spacing; the tighter rule completes47502 points.
That default-budget failure is preserved in positive_tetra_rejected_budget.
No tolerance/budget/height change or local fallback was accepted.

The independent Gauss/Duffy reference converges at angular orders32/48/72 to
normal integral6.223134359667434e-5, last relative change1.813e-7. Its cached
reference identities are checked before reuse. Per-point native distances and
normal density agree with independent triangles, so these are integration errors.
The FEM positivity/polynomial coefficient checks remain evidence for a withdrawn
strategy, not current production dependencies.

## Current connected replacement: face-to-vertex columns

The unsuccessful integration-tetrahedron code is replaced by Duffy face-to-vertex
columns. The nearest source face sets the angular base; every ray traverses the
whole source tetrahedron and its positive measure is3V(1-t)^2. Exact geometric
EdgeBreakpoints resolve all thin normal bands on each ray. This does not interpolate
wall tensors or change the physical target.

The existing qualified parallel-chord adaptive quadrature is factored into a
shared column integrator: retained positive degree-five triangular7-point rule,
12-point higher-degree angular comparison, positive3/5/7-point radial rules and
the existing global1e-4 angular/1e-5 radial error control. New collapsed columns
include their exact radial volume factor. Hidden-support safeguards and original
point65536/work16384/depth32 bounds stay. Provided callbacks mark visible rules
sensor-dependent so complexity cannot reuse a rule across changed frozen targets.
No Euler/RANS/input-case mode or automatic MMG substitution.

The complete original wing fixture is geometry-only and uses its input triangle
orientation. Its independent reference establishes normal density, not the full
runtime tangent tensor or mesh acceptance. The original volume-oriented native
reference and matching120-step restart for a whole frozen event are prepared in
`integration_evidence/native_3d_bl_v1/oneram6_frozen_v1`; closed/oriented skin and
exact coordinate round-trip are checked, but native decode/import and the event
are NOT run yet. Requested CFD iterations are0.

Current build completed PASS. The connected native batch passes7 cases/601990
assertions, including hidden/finite/rotated controls and actual M6 replay. Cell13
retains1419 points and20121 callback evaluations per requested bandRatio, with
exact positive volume/first+second barycentric moments. Independent normal-density
error1.59271733975e-4 is below unchanged5e-4; reference last change1.813e-7.
Both bandRatio1.5/1.25 use the shared controller's same minimum radial band ratio2;
identical results are not independent native-resolution convergence. The full
unit-tensor integral9.2830 remains diagnostic, not independently established.

## Whole-event counterexample and primary-controller correction

Published14e59d4c68 retains the exact scoped cell13/normal/control proof above.
Whole saved-state event51673 is TERMINAL FAILED (exit1), one initial CFD iteration,
no remesher/candidate/accepted mesh/transfer. Elected original global cell22 exceeds
radial depth32. Rank0 separately records cell32 radial work failure (18453 retained,
98 panels). It reached failure collection after9.47s quadrature; sparse snapshots
show rank0+2 waiting there while rank1+3 still execute EdgeBreakpoints/Evaluate.
Whole elapsed626s is not a remesher timing or scalable cost estimate. Actual target
sensors were not captured before exit: GDB attempt says no process, preserved.
Do not claim a full frozen-sensor replay. Exact failures/startup errors stay saved.

Normal-only source cells22/32 complete (4750/14949 points). The existing generic
replay now accepts version2:4 finite symmetric SPD sensor tensors/core. A unit
sensor with actual original native volume-oriented wall order reproduces cell22's
radial failure in.008s/661 callbacks. It does not require the CFD sensor. Catch-throw
GDB captures actual3/5/7 rejected rules at t[.7700333435664457,.7700333436991762].
Independent NumPy density matches all3 weighted integrals to roundoff, with a
normal/density jump14500389->14616826; this is not just spectral roundoff. The tiny
integral6.6e-12 fails a local relative test despite already accumulated ray mass.554.
This exposes the controller's structural inability to integrate a discontinuity:
volume-proportional local allowance also shrinks with interval width.

The current connected replacement controls TOTAL ray error, using a maximum-error
interval queue over all geometric bands. Existing positive3/5 rules are retained;
selected intervals use the existing7-point comparison and retain5 when upgraded,
then bisect only when needed. Ray tolerance1e-5/global angular1e-4 and original
65536 point/16384 work/depth32 remain. No pilot-floor fallback, target/geometry/
height change or budget increase. Build10003 terminal PASS. Both actual full identity-sensor probes now complete
(cell22:5460 points/integral3.41728875793347; cell32:24890/1.83778862378228).
One affected batch passes7 cases/578415 assertions. Cell13 normal rule now1335
points/integral6.22410248594398e-5. These are scoped controller checks, not actual
CFD-sensor/whole-mesh or rigorous error-bound proof. Ray and angular differences
remain numerical indicators. First independent radial16/double-ended reference is
6.22313814200777e-5, relative change6.1e-7 from the old angular-only reference.
Reference57600 stopped at180s with1664/1704 nodes saved. Resume89213 completed
only the final40 nodes: radial24/band1.25 integral6.223137380817692e-5, relative
change1.223e-7 from radial16. New native same-chart error1.55083e-4 passes5e-4
pilot but exceeds nominal1e-4 indicator; numerical differences are not enclosures.
Other3 apex checks run sequentially in72066,1 core/240s per reference case, with
identity-checked partial/completed checkpoints. No production budget change. Do not confuse
source-cell subdivision/independent tensor math with these normal-only checks.

## Independent Opus5.5 review

User-authorized local Claude Code used explicit claude-opus-5-5/high; returned model
identity confirms it, no nested agents or permission denials. Read-only prompt/raw
result and CLAUDE_REVIEW.md retained. The reviewer inspected the earlier source
while it was being validated, and did not verify hashes; our source/build/input/
helper hashes were separately verified before publishing14e59. It found no chart
Jacobian/volume/orientation defect, but correctly identifies reference radial/
chart convergence and error-estimator honesty gaps, equal bandRatio values and
whole-mesh cost. The claim that keeping higher order automatically bounds error
is NOT accepted as a theorem for nonsmooth metrics. Sparse actual stacks support
wall-query work as a phase, not a percentage/cost extrapolation. Cached geometry
across changed sensor trials may help, but must be bounded and proved identical.

Full Goal3 stays ACTIVE/incomplete, including all exterior-wing construction,
features/interactions, composed gradation, transfer, accuracy and lifecycle cost.
No owned CFD run. Only sequential independent-reference job72066 runs; no owned CFD or build.
CSolver catch now formats a bounded numeric replay record (cell IDs/coords/core/
trial/nodal sensors) on failure. Build89213 terminal PASS; runtime record branch NOT YET EXERCISED. Producer
source/header/test/object are identical to the qualified control batch. Whole
caller source equals14e59 after replacing only the added catch text; no repeated
broad suite for formatting. Fresh binary/object identities are pinned separately. Do not rerun full CFD before accuracy checks. Runtime failure22
and actual32 sensors remain uncaptured; new diagnostics avoid a repeat of the
failed debugger capture race.

Original failure neighborhoods22/32 are inspectable in oneram6_frozen_v1 under
failure_cell_22_input.vtu and failure_cell_32_input.vtu,45/62 cells respectively.
Color FailureCell; OriginalCellID/OriginalPointID retained and coordinates roundtrip
exactly. These are original input neighborhoods, not rejected candidates. No
remesher candidate exists. See locate_failures.py/log and per-cell JSON receipts.
