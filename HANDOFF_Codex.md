# Codex handoff — start here

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`. MPI-qualified production base:
`531025fceaa8c45b6655a56281ddf6887ce9b55e`.

Goal 3 is ACTIVE and incomplete. Production planar BL policy and initialized
RANS/current+BDF/restart workflow are qualified locally on MPI1/2/4, including
M<N. The current connected work improves first-row alignment and coordinated
wall/transition edits. The new normal-foot gate passes focused serial geometry
and one independently audited production/restart case at published508e52cf7b.
Current frontier: coordinated wall/transition reconstruction with retained
interior points. Its geometric witness passes, but the stronger native field
still fails the work ceiling. A separately generated control proves target
feasibility under the declared contracts. Private actual-field 2/3/4-cell reconnection now passes its saved-proposal replay
and independent audit, but still fails full shape/edge admission and is not wired
into production. The connected static-assessment patch passes focused serial22-case and MPI2/4
7-case-per-rank checks for recoverable refusal, first-row owner concurrency,
reference admission and quadrature reuse. All owned jobs are terminal.
Full Goal3 remains incomplete.

Current testcase folder:
`integration_evidence/native_3d_bl_v1/static_assessment2_v1` (focused serial/MPI stages).
Private replay remains in `integration_evidence/native_3d_bl_v1/connectivity_replay_v1`.
The production checkpoint's inspectable grids and states remain in
`integration_evidence/native_3d_bl_v1/production_policy_v1/mpi1`, `mpi2`, `mpi4`.

Read only the relevant handbook:

- [Current Goal 3 implementation, evidence and remaining gates](Handbook/GOAL3_CURRENT.md).
- [Wall reconstruction failures, feasibility control and next replay step](Handbook/GOAL3_WALL_RECONSTRUCTION.md).
- [Static assessment 2: fixes, qualification and open architectural limits](Handbook/STATIC_ASSESSMENT_2.md).
- [Completed Goals 1 and 2 and their scope](Handbook/COMPLETED_GOALS.md).
- [Permanent implementation and validation rules](AGENTS.md).
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
