# Codex handoff — start here

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`. MPI-qualified production base:
`531025fceaa8c45b6655a56281ddf6887ce9b55e`.

Goal 3 is ACTIVE and incomplete. Production planar BL policy and initialized
RANS/current+BDF/restart workflow were qualified on the preceding MPI1/2/4 base.
The current connected runtime passes MPI1/2 including M<N; MPI4 remains required. The current connected work improves first-row alignment and coordinated
wall/transition edits. The new normal-foot gate passes focused serial geometry
and one independently audited production/restart case at published508e52cf7b.
Current connected work removes the common-background requirement for wall splits
and grows dependencies from actual metric blockage: first the artificial face,
then a complete constrained interior-vertex star. It retains volume topology,
reconstructs normal wall children privately, preserves partial repair for diagnosis,
and validates complete replacements against the original field. Imported cavities
remain64 cells; constructed private inputs may use the existing128-cell bound.
The required stronger original-P1 field now independently passes construction and
removal:649 BL cells on MPI1/2/4→265/268/260 sensor-only cells. All ranks give
identical BL coordinates/connectivity/markers and quality/length extrema. Current
initialized RANS/SA transfer/restart audits pass MPI1/2; MPI4 was interrupted before completion and remains required. Nine focused serial
integration controls PASS (1596 assertions). No native Euler/RANS transition modes,
metric reset, target relaxation or new production tuning option was added.
The centroid-insertion experiment did not help and was removed; its evidence is
preserved. Full physical/curved/interacting-wall qualification remains open.
Full Goal3 remains incomplete.

Current continuation: connected certified radial BL queries, frozen tangent
curvature demand, actual edge/thin integration, per-wall hidden coverage and
physical row-clearance checks are implemented and built. Field4 controls PASS;
single-sphere radial fan/volume oracle PASS. Composed gradation remains uncertified.
Full curved cold construction still rejects at4096 work with120 cells; interacting
thin-wall integration rejects at16384 subdivisions. Off-wall tangent lookup caused
an earlier edge quadrature failure and was fixed by binding to the true foot.
Early interface probing now closes the verified missing vertex71 star before
repair. The complete15-cell replay resolves in14 trials; independent audits retained.
Full cold run still rejects4096 work after210 private edits/all8 wall reconstructions.
Its1026-cell/260-node rejected grid passes all shape, declared height/feet and exact
local geometry checks;150 of1368 actual-target edges remain oversized (max3.860547).
No CFD replacement/release. Reusable saved-grid audit isolates those residuals
without another construction run. Five focused controls PASS56 assertions.
Longest cap-edge replay diagnoses a rounded point rejected by strict/joint split.
Joint same-chart cavities now admit it through full geometry checks; strict split
and junction checks remain. Targeted2→14-cell common-fan growth still fails metric
shape. Four focused geometry controls PASS1622 assertions; local geometry audit
PASS scoped. Next: retained/transition reconstruction for those long edges, rather
than more fan enlargement, without raising limits or retargeting. All failures, input/rejected grids, sources/
binaries/helpers and partial certificates are preserved. All owned jobs terminal.
No new MPI/full regression or physical CFD campaign. Earlier qualification stays
tied to its executed source/binary. Full Goal3 remains active and incomplete.

Current inspectable case folders:
`integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/curved_cold`,
`curved_field`, `curved_volume`, `interacting_volume`.
Earlier diagnostic: `integration_evidence/native_3d_bl_v1/curved_wall_feasibility_v1`.
Latest qualified focused controls: `integration_evidence/native_3d_bl_v1/connectivity_replay_v1/integration`.
Accepted stronger-field meshes are in `integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/sensor`,
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
