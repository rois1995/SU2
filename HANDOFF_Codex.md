# Codex handoff — start here

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`. MPI-qualified production base:
`531025fceaa8c45b6655a56281ddf6887ce9b55e`.

Goal 3 is ACTIVE and incomplete. Production planar BL policy and initialized
RANS/current+BDF/restart workflow were qualified on the preceding MPI1/2/4 base.
The recorded pre-curved runtime passes MPI1/2 including M<N; MPI4 remains required. The current connected work improves first-row alignment and coordinated
wall/transition edits. The new normal-foot gate passes focused serial geometry
and one independently audited production/restart case at published508e52cf7b.
Current connected work removes the common-background requirement for wall splits
and grows dependencies from actual metric blockage: first the artificial face,
then a complete constrained interior-vertex star. It retains volume topology,
reconstructs normal wall children privately, preserves partial repair for diagnosis,
and validates complete replacements against the original field. Imported cavities
remain64 cells; constructed private inputs may use the existing128-cell bound.
The recorded stronger original-P1 field independently passed construction and
removal:649 BL cells on MPI1/2/4→265/268/260 sensor-only cells. All ranks give
identical BL coordinates/connectivity/markers and quality/length extrema. Current
initialized RANS/SA transfer/restart audits pass MPI1/2; MPI4 was interrupted before completion and remains required. Nine focused serial
integration controls PASS (1596 assertions). No native Euler/RANS transition modes,
metric reset, target relaxation or new production tuning option was added.
The centroid-insertion experiment did not help and was removed; its evidence is
preserved. Full physical/curved/interacting-wall qualification remains open.
Full Goal3 remains incomplete.

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

Read [resolution-preserving reconstruction and the current gates](Handbook/GOAL3_RESOLUTION_BACKTRACKING.md).
No broad regression, CFD/transfer/restart campaign or performance assessment was
run for this milestone. Native engine N=M1/2/4 is qualified for this declared planar
field; M<N/current+BDF/SA/restart and wider fields/geometries still need integration
qualification. Interacting thin-volume/composed gradation/physical RANS/M6/accuracy/
performance gates remain open. All owned jobs terminal. Goal3 ACTIVE and incomplete.

Current inspectable case folders:
`integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/curved_cold`,
`curved_cold_complete_rejected_v1`, `curved_edge_replay`, `curved_grid_refine`,
`curved_cold_residual`,
`curved_field`, `curved_volume`, `interacting_volume`.
Earlier diagnostic: `integration_evidence/native_3d_bl_v1/curved_wall_feasibility_v1`.
Latest qualified focused controls: `integration_evidence/native_3d_bl_v1/connectivity_replay_v1/integration`.
Historical accepted stronger-field meshes are in `integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/sensor_historical_pass_v1`,
`sensor_mpi2`, `sensor_mpi4`; rejected predecessors remain in `sensor_previous_*`.
Private replay remains in `integration_evidence/native_3d_bl_v1/connectivity_replay_v1`.
The production checkpoint's inspectable grids and states remain in
`integration_evidence/native_3d_bl_v1/production_policy_v1/mpi1`, `mpi2`, `mpi4`.

Read only the relevant handbook:

- [Current Goal 3 implementation, evidence and remaining gates](Handbook/GOAL3_CURRENT.md).
- [Wall reconstruction failures, feasibility control and next replay step](Handbook/GOAL3_WALL_RECONSTRUCTION.md).
- [Resolution-preserving repair and restored planar MPI capability](Handbook/GOAL3_RESOLUTION_BACKTRACKING.md).
- [Curved BL geometry/target/construction coupling](Handbook/GOAL3_CURVED_BL.md).
- [Static assessment 2: fixes, qualification and open architectural limits](Handbook/STATIC_ASSESSMENT_2.md).
- [Completed Goals 1 and 2 and their scope](Handbook/COMPLETED_GOALS.md).
- [Permanent implementation and validation rules](AGENTS.md).
- [Deferred SST, transition-model and non-ideal-gas transfer questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
- [Full requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
- [Historical handoffs, split into smaller files](Handbook/History/INDEX.md).

Permanent working rule: pursue complete capabilities against known difficult
failures, with a small set of distinct fields/geometries. Simple passing tests are
primitive checks, not capability milestones. Reconsider stalled construction
strategies rather than accumulating local variants. See AGENTS.md.

Update current state in place; archive superseded evidence without repeatedly
prepending old updates. Reuse valid checks with source/build/binary/config/input/
helper provenance. Focused serial checks during development; broader regressions
and MPI campaigns at substantial connected milestones. Push reviewable checkpoints.
One owned heavy job, nice19/threads1, MPI<=4/build-j1; verify live handles before
restarting. AdapNoExt, foreign ClusterResults, dependencies and other workers stay
untouched. Euler-to-BL and BL-to-Euler are validation scenarios, never modes.

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
