# Goal 3 — current implementation and qualification

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`; MPI-qualified production base
`531025fceaa8c45b6655a56281ddf6887ce9b55e`. Goal 3 is ACTIVE and incomplete.

The published production checkpoint connects the actual-query geometric BL policy
to partition weights, refinement and coarsening. It preserves original-P1
sensor-only donor interpolation, finer sensor demands, existing fade and retained
geometry. The returned nodal metric contains only the sensor. Euler/BL transitions
are validation scenarios, never production modes or solver-label dispatch.

Initialized RANS/SA with a computed metric passes N/M=1/1,2/1,2/2,4/1,4/2,4/4.
Independent audits qualify twelve planar cube frames, exact geometry and coverage,
full composed metric lengths/shape/height, transported sensor, current+BDF flow
integrals, SU2/CGNS/reference reload and real restart. Native checks cover bounded
SA histories. No CFD iterations or viscous accuracy/performance qualification.
See [production evidence](../integration_evidence/native_3d_bl_v1/production_policy_v1/summary.json).

## Current connected work: first-row alignment and coordinated wall edits

The published coarsened meshes retain height but merge normal wall apices, giving
one outside normal footprint in each of six worker combinations. Exact-rational
[baseline audit](../integration_evidence/native_3d_bl_v1/alignment_audit_v1/baseline.json)
reproduces this without new SU2 runs. Centroid tilt alone is not the criterion.

Current changes add normal-foot containment in the actual wall triangle to
shared first-row initialization, private candidate and final checks, with separate
footprint rejection counters. Arithmetic-ambiguous footprints reject. Compilation
completed. Six focused serial cases pass (976 assertions); one actual production
case passes (1267 assertions). Its independent audit qualifies two frames: BL58
cells, normal centroid tilt0 and minimum footprint barycentric1/3; unconstrained
coarsening6 cells. Height error and transported sensor error0; current+BDF flow
integral defect below6e-16. Two offending coarsening proposals were rejected.
No full regression or MPI campaign was run for this small change; reserve those
for the connected alignment/reconstruction milestone. All owned jobs terminal.
See [serial summary](../integration_evidence/native_3d_bl_v1/alignment_audit_v1/summary.json).

Cases: `integration_evidence/native_3d_bl_v1/alignment_audit_v1`.
Current frontier: coordinated wall/transition carving, actual-field reconnection
and simultaneous constrained interior movement. The private witness passes shape,
coverage and wall constraints, but not full edge admission. The new connected
engine run accepts3 joint wall reconstructions and still rejects at work4096:
453 cells, qmin .225576,Jmin .054521,Lmax2.54379>1.8, exact first height and valid
normal footprints. Original source and target are unchanged. No removal or CFD
replacement is reached. Seven focused serial integration controls PASS (855 assertions); MPI
qualification of the new connection is pending. The non-native576-cell control
continues to prove declared-target feasibility, not native completion.

Read [the smaller wall-reconstruction handbook](GOAL3_WALL_RECONSTRUCTION.md)
for exact failures, preserved artifacts and the next substantial replay/reconnection
step. Goal3 remains ACTIVE; this is not complete native BL generation.

## Static assessment reconciliation

The supplied b449 static report was checked against current callers and kernels.
A connected patch makes supported native rejection recoverable before replacement,
keeps ordinary replacement output conditional on acceptance, removes the late
post-transfer deadline veto, admits whole geometry components/reference files by
storage, all-gathers initial facets without P sender copies, runs disjoint
first-row owners through complete-vertex reservations, preserves certifiable metric-balanced physical placement with a
midpoint/projection fallback, validates unique physical edge cuts before counting
them and reuses geometry-only quadrature across sensor trials. Hidden support
still invalidates against trial sensor/core. No full-window snapshot was added to
unsupported native fixed-point mode.

Focused serial22-case and MPI2/4 seven-case-per-rank correctness stages PASS.
Serial includes the actual initialized RANS/SA adaptation/transfer/restart control.
All owned jobs are terminal. Saved stages are in
`integration_evidence/native_3d_bl_v1/static_assessment2_v1`.
Read [the reconciliation](STATIC_ASSESSMENT_2.md) for exact fixed/partial/open scope.
This does not establish general curved/BL concurrency or scaling. Whole changed-skin
curved clearance and expanded repair serialization remain open. Goal3 completion
still requires the full gates below, not just this assessment patch.

## Remaining full-goal gates

Composed gradation probes violate the sensor HGRAD transport bound although sensor
gradation passes. The requested narrow layer and unchanged fade make that particular
bound infeasible on the current fixture; reconstruction cannot fix the frozen
field. Preserve the target and diagnose it rather than spreading wall tensors.

General thin/curved/interacting walls and marker junctions, stronger sensor fields,
layer coverage and alignment beyond this necessary footprint condition remain
unqualified. Analytic BL distance/normal evaluation remains explicitly rejected.
Actual small RANS convergence/wall shear/forces, practical M6 comparison, complete
adaptation/CFD cost and affordability remain open. Goal 4 starts only after full
Goal 3 completion. No external cluster submission is authorized by this handoff.

The [requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json)
preserves the full objective; passing this fixture does not complete it.
