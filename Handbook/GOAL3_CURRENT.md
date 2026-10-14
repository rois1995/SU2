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
Current frontier: coordinated wall/transition carving, complete apex-star
admission, wall-child placement and retained interior-point reconstruction.
Two focused serial geometric cases and independent geometry audit pass. The
stronger original-P1 field remains rejected at work4096 after117 refinement
edits/418 private cells; longest wall edge4.3997755. A576-cell non-native control
independently proves feasibility under the declared acceptance contracts.
No MPI or full regression campaign was run; all owned jobs are terminal.

Read [the smaller wall-reconstruction handbook](GOAL3_WALL_RECONSTRUCTION.md)
for exact failures, preserved artifacts and the next substantial replay/reconnection
step. Goal3 remains ACTIVE; this is not complete native BL generation.

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
