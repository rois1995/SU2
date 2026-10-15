# Codex handoff — start here

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`, based on published Goal2
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`. Goal3 is ACTIVE and incomplete.

The current difficult target is complete curved BL generation with unchanged
original-P1 sensors, actual-query BL/fade, first height, geometry and metric gates.
Planar construction/removal passes on the recorded MPI1/2/4 checkpoint; curved
local repairs and bounded protocol checks are progress, not accepted generation.

Current connected work adds finite curved-feature projection, joint wall-height/
normal-foot fitting, directed visibility closure and complete topological stars.
The strict same1402-cell original-field target makes96 edits to1832 cells, still
rejecting38 oversized edges at4096 work (16 cylinder/20 rim/2 interior).

Current focused geometry and MPI2 protocol/donor-reference checks pass. The entire
1832-cell result independently passes positive/no-overlap geometry,84 exact feet,
first height and exported-tensor shape arithmetic. Its38 lengths still reject.
The isolated rim geometry succeeds but actual shape rejects; its exact-foot audit
also flags a7.5e-18 weight within native numerical allowance. Read
[the connected scope and remaining reconstruction limitation](Handbook/GOAL3_CURVED_FEATURE_REPAIR.md).
No accepted curved generation, CFD/restart/M<N/currentMPI4 or performance claim.
All owned jobs terminal; do not restart from stale receipts.
Read [the previous capability and boundary restriction](Handbook/GOAL3_JOINT_CONNECTIVITY.md).
Earlier [shared-apex](Handbook/GOAL3_SHARED_APEX_REFIT.md) and
[support-selection](Handbook/GOAL3_SUPPORT_SELECTION.md) evidence remains preserved.

Inspectable current cases are below
`integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/`:
`curved_feature_complete`, `curved_feature_residual`, `curved_rim_refit_probe`,
`curved_feature_controls`, `topology_hint_controls`, `geometry`, and
`curved_joint_protocol_feature_mpi2`. Earlier rejected grids remain preserved.
A folder or RUNNING receipt alone is not evidence of a live or completed job.

A wall-front/scheduling study was rejected and preserved separately. The active
production path is restored to the published baseline. Read [the study disposition
and next coupled patch-optimization design](Handbook/GOAL3_PATCH_OPTIMIZATION.md).
The current local binary is from the study; rebuild before executing active source.
All owned jobs terminal. No rejected study is a current production improvement.

Read only the relevant handbook:

- [Current implementation and full remaining gates](Handbook/GOAL3_CURRENT.md).
- [Coupled curved-feature and wall-foot repair](Handbook/GOAL3_CURVED_FEATURE_REPAIR.md).
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
