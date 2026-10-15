# Codex handoff — start here

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`, based on published Goal2
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`. Goal3 is ACTIVE and incomplete.

The current difficult target is complete curved BL generation with unchanged
original-P1 sensors, actual-query BL/fade, first height, geometry and metric gates.
Planar construction/removal passes on the recorded MPI1/2/4 checkpoint; curved
local repairs and bounded protocol checks are progress, not accepted generation.

Current connected work combines shared-apex wall refitting, private insertion/
reconnection and actual-field-directed complete-star admission. The strict same
1402-cell original-field target now makes94 insertions to1842 cells, but still
rejects40 oversized edges at4096 work. Remaining groups:16 cylinder,22 cylinder/
cap junction,2 interior. Field, height, geometry and quality gates are unchanged.

Current focused/native/MPI2 protocol and independent entire geometry/height/shape
arithmetic checks pass within their scope. No accepted curved mesh or CFD/restart/
M<N/currentMPI4/performance qualification follows. All owned jobs are terminal.
Read [the current capability and next boundary restriction](Handbook/GOAL3_JOINT_CONNECTIVITY.md).
Earlier [shared-apex](Handbook/GOAL3_SHARED_APEX_REFIT.md) and
[support-selection](Handbook/GOAL3_SUPPORT_SELECTION.md) evidence remains preserved.

Inspectable current cases are below
`integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/`:
`curved_joint_connectivity_complete`, `curved_joint_connectivity_residual`,
`curved_refit_interior_dependencies`, `curved_cover_probe`, `curved_repair_controls`,
and `curved_joint_protocol_connectivity_mpi2`.
A folder or RUNNING receipt alone is not evidence of a live or completed job.

Read only the relevant handbook:

- [Current implementation and full remaining gates](Handbook/GOAL3_CURRENT.md).
- [Joint insertion and private connectivity repair](Handbook/GOAL3_JOINT_CONNECTIVITY.md).
- [Coordinated shared-apex refit, qualified scope](Handbook/GOAL3_SHARED_APEX_REFIT.md).
- [Support selection and latest curved progress](Handbook/GOAL3_SUPPORT_SELECTION.md).
- [Interior placement](Handbook/GOAL3_INTERIOR_PLACEMENT.md).
- [Curved BL policy and earlier failures](Handbook/GOAL3_CURVED_BL.md).
- [Resolution-preserving planar reconstruction](Handbook/GOAL3_RESOLUTION_BACKTRACKING.md).
- [Static assessment reconciliation](Handbook/STATIC_ASSESSMENT_2.md).
- [Full requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
- [Deferred SST/transition/non-ideal-gas questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
- [Completed goals](Handbook/COMPLETED_GOALS.md) and [preserved history](Handbook/History/INDEX.md).
- [Permanent implementation and validation rules](AGENTS.md).

Pursue complete capabilities against actual difficult failures. Group connected
changes; use focused checks to resolve risks, broader/MPI checks at substantial
milestones, and reuse evidence only within verified provenance and scope. Preserve
failures, update these linked handbooks, and push reviewable checkpoints regularly.

One owned heavy job; build-j1/MPI<=4, nice19/threads1, sampled bounded affinity.
Use task-local build temporary storage. Inspect processes outside the restricted
sandbox before restarting: a sandbox-local process list missed a live build once.
Never terminate unrelated work. AdapNoExt, foreign ClusterResults and dependencies
stay untouched. No production Euler/BL mode dispatch, relaxed gates or target resets.
