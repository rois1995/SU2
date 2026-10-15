# Goal 3 — current implementation and qualification

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`; MPI-qualified production base
`531025fceaa8c45b6655a56281ddf6887ce9b55e`. Goal 3 is ACTIVE and incomplete.

The published production checkpoint connects the actual-query geometric BL policy
to partition weights, refinement and coarsening. It preserves original-P1
sensor-only donor interpolation, finer sensor demands, existing fade and retained
geometry. The returned nodal metric contains only the sensor. Euler/BL transitions
are validation scenarios, never production modes or solver-label dispatch.

The preceding production checkpoint qualified N/M=1/1,2/1,2/2,4/1,4/2,4/4.
The recorded pre-curved runtime passes N/M=1/1,2/1,2/2 and independent six-frame audits;
its four-rank run was interrupted before completion and must be repeated.
Independent audits qualify exact planar cube geometry and coverage,
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
Current frontier: retained-topology wall reconstruction and metric-directed complete
dependency escalation. The actual constrained vertex star resolves the reproduced
wall-seam blockage in35 cells. The historical stronger original-P1 field passed
complete construction/removal and independent audits: MPI1/2/4 gives649 BL cells
→265/268/260 sensor-only cells, with identical BL coordinates/connectivity/markers. BL qmin .215076,Jmin .050331,Lmax1.798735; exact first height and valid
normal footprints. Nine focused integration controls PASS (1596 assertions).
Current actual-policy transfer/restart audits pass MPI1/2; MPI4 was interrupted before completion and remains required.
The ineffective centroid-insertion proposal was removed and its evidence retained.
No metric reset, acceptance relaxation or production transition mode was added.

Read [the smaller wall-reconstruction handbook](GOAL3_WALL_RECONSTRUCTION.md)
for exact failures, preserved artifacts and the next substantial replay/reconnection
step. Goal3 remains ACTIVE; this is not complete native BL generation.

## Current continuation

Current continuation: the stronger planar regression is repaired and independently
qualified on MPI1/2/4. Exact wall-patch replay showed that shape-only retirement
removed required size resolution: after geometric closure18→22 cells and complete
support-star admission→31, a40-cell shape-valid proposal added .036023 edge excess
while removing only .003134. The frozen gate correctly rejected it.

Native now backtracks from that rejection and retries without point retirement,
using only the unspent part of the original256-trial allocation. No field, height,
quality, length, global work or commit gate changed. The exact replacement passes
in19+70 trials, q .209831/J .051600, with added excess0. Complete construction/removal
and independent original-field/geometry/height audits PASS: MPI1/2/4 gives591 BL
cells→252/250/243 sensor-only cells. BL coordinates/connectivity/markers are exactly
identical across ranks; qmin .213044,Jmin .050377,Lmax1.798735, altitude error0,
contained normal feet. Maximum centroid tilt1.05409 remains a reported alignment
limit; this does not qualify a complete orthogonal layer stack.

The full curved cold run still returns the300-second kernel limit after52 edits/
all8 wall reconstructions, with438 rejected cells. Its SU2 grid is byte-identical
to the preceding recorded residual audit; unchanged relevant query/field sources
were checked before reusing those measurements: no native shape failures,281 of624
edges oversized,max4.201315. Full curved acceptance/removal/CFD replacement remains
unqualified. The earlier cap-depth probe and negative angular experiment remain
scoped historical evidence, not full mesh generation or performance results.

Read [resolution-preserving reconstruction and the current gates](GOAL3_RESOLUTION_BACKTRACKING.md).
No broad regression, CFD/transfer/restart campaign or performance assessment was
run for this milestone. Native engine N=M1/2/4 is qualified for this declared planar
field; M<N/current+BDF/SA/restart and wider fields/geometries still need integration
qualification. Interacting thin-volume/composed gradation/physical RANS/M6/accuracy/
performance gates remain open. All owned jobs terminal. Goal3 ACTIVE and incomplete.

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
unqualified. The new certified radial BL implementation is unqualified; partial/graph/mixed
analytic components still reject.
Actual small RANS convergence/wall shear/forces, practical M6 comparison, complete
adaptation/CFD cost and affordability remain open. Goal 4 starts only after full
Goal 3 completion. No external cluster submission is authorized by this handoff.

The [requirement audit](../integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json)
preserves the full objective; passing this fixture does not complete it.

Saved-grid transition experiment: simultaneous two-point insertion/apex movement
remained rejected at446 cells/279 oversized edges, excess162.661→144.871. Its
benefit was not isolated, so the extra production retries were withdrawn; the
engine is restored exactly to published3f323314fb. The strict resumable
`curved_transition_complete` harness and closed source-pinned experiment remain.
Next: field-residual-directed joint placement, not additional fixed move seeds
or budget increases. See the curved handbook's rejected-experiment section.

Joint-placement continuation: coordinated cap split/apex placement passes its
focused replay (169 trials,32→38 cells,569 assertions) and independent exact local
geometry audit. The single-apex failure is preserved. Engine integration retains
complete versioned-star admission/reservation and shared work/final gates.
The strict saved438-cell run remains rejected:7 edits/5 insertions,473 cells,
287 oversized edges, excess162.660529→142.380265, work1131 before its kernel time
ceiling. No speedup/completion claim. Last reported cone blockage is an interior
diagonal; MPI integration and all full Goal3 gates remain open. See the curved
handbook and joint_placement_summary.json. All owned jobs terminal.
