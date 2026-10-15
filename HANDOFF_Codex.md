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

Current continuation: curved BL generation remains incomplete and a recreated
stronger planar control now fails. Historical MPI1/2/4 success remains valid on
its recorded older source; it is not current-runtime qualification.

The final cap-normal fallback preserves established strip proposals/gates first,
then admits12 bounded complete proposals using actual geometry/metric spacing and
the shared changed-edge progress gate. The unchanged1026-cell saved-grid engine
probe PASS: one edit in76 work,34→36 changed cells,1028 total cells; native excess
39.4413→35.2322 (10.7% lower), qmin .210041/Jmin .058245. Oversized edges150→151;
final acceptance correctly remains false. Independent geometric proof was reused
only after identical old/new geometry/reference/donor hashes and helper checks.
Interface/volume/first-height/feet and absence of positive-volume overlap remain
certified in the stated scope; no independent curved field/edge/CFD qualification.

A preceding first-choice/global-progress policy experiment failed full cold
construction at the300-second kernel ceiling after52 edits/all8 wall reconstructions.
Its438-cell rejected grid has no native shape violations (q .229233/J .056062),
but281 of624 edges exceed1.8,max4.201315. That run is tied to its earlier policy
snapshot, not the final fallback policy. The kernel now returns/saves progress
before the collector deadline. Input/rejected grids/private pairs/receipts preserved.
No complete cold rerun after the final policy reconciliation; no MPI/performance claim.

The stronger planar reproduction fails (559 cells, operator stagnation) even after
restoring existing strip ordering/gates and testing a surface-motion restriction.
Matching the last PASS receipt reveals a much older pre-curved source. Causality is
not isolated; the ineffective restriction was removed. Failed policy/order/current
stages and the earlier successful evidence are preserved. This is an explicit open
regression requiring repair before broad capability qualification. Independent
audit confirms valid geometry, altitude/normal feet and q .213044/J .052552,
but length1.855981 remains on a wall edge. Its two first-row owners and complete
edge/apex-star union are recorded in `wall_reconstruction_v1/planar_wall_residual.json`.
The next diagnosis should replay that complete constrained boundary patch.
Next substantial step: actual-target-aware coordinated transition reconstruction,
including point-retirement/dependency choices and combined support insertion/motion;
do not accumulate cap-placement variants or repeat cold runs before that gap is addressed.

The interacting thin-sphere reference now correctly includes both unsigned-distance
shells (8*pi integral). Its previous work-budget failure remains open; this is a
reference correction, not an integration success. A closed-form angular probe
found projective sampling worse (12396 versus4032 panels), so it was not integrated.
Composed gradation, interacting volume, wider geometry, MPI/transfer/restart,
physical RANS/accuracy and complete performance gates remain open. All owned jobs
terminal. Goal3 ACTIVE and incomplete.

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
