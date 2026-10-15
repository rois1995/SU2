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

Current continuation: certified radial queries, frozen tangent curvature demand,
actual edge/thin integration and physical first-row clearance are built. Field
controls and the single-sphere volume oracle pass on their recorded binaries;
interacting thin-volume integration and composed gradation remain unqualified.
Early support-star admission resolves the verified15-cell private wall blockage.
The previous complete cold-cylinder run admitted210 edits/all8 wall reconstructions
but rejected4096 work:1026 cells,150 oversized actual-target edges,max3.860547.
That grid passes scoped shape/geometry/declared height checks and is preserved in
`wall_reconstruction_v1/curved_cold_complete_rejected_v1`.

The cap-edge replay now establishes a valid private retained proposal: simultaneous
point placement/free-apex movement passes original-field refinement, while separate
edits fail. Independent34→36-cell checks preserve artificial interface, volume and
original sensor donors, with positive disjoint tetrahedra. The complete production
mover union is54 cells, within the64-cell import bound. Production strip eligibility
now uses local incident geometry, so flat caps remain eligible in curved domains;
rounded same-chart insertion also supports volume moves. Four focused geometry
controls PASS1716 assertions and their independent geometry/normal-foot audit PASS.
The integrated cold run still times out at the420-second collector limit without
returning/exporting a final mesh. Its logs/private pair and the earlier complete
rejected grid are preserved separately. No release/CFD replacement, MPI or
performance claim. The next missing capability is production coordinated layout
and selection that realizes the private repair and resolves the full residual,
not larger common fans or higher limits. All owned jobs terminal. Goal3 incomplete.

Current inspectable case folders:
`integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/curved_cold`,
`curved_cold_complete_rejected_v1`, `curved_edge_replay`,
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
