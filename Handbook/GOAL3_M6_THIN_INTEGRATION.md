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

Whole saved-state event session51673 runs in oneram6_frozen_v1,4 ranks/nice19/
threads1/1200s outer bound, native300s unchanged. Initial planned ITER0 encountered
three configuration-only errors: follow-up flowIter must be positive, then unused
adjIter defaults0 yet explicit ADAP_ADJ_ITER is forbidden for non-GOAL. All exact
inputs/logs/provenance remain under invalid_* prefixes. No production config change:
current input uses ONE initial CFD step from saved120-step conservative state and
one follow-up only if accepted. No bit-identical target/timing comparison claim.
Native authority decode/import, full-mesh integration and remeshing remain pending.

User authorized consulting local Claude Code with Opus5.5 medium/high. Read-only
review session83342 requested explicit claude-opus-5-5/high, safe-mode and only
Read/Grep/Glob, no commands/edits/jobs or nested delegation,900s bound. Prompt/output
are stored here; result/model identity must be inspected before claiming a review.
All prior build/test/preparation jobs terminal. Full Goal3 remains open, including
exterior-wing construction/transfer/accuracy/performance. No accepted M6 mesh yet.
