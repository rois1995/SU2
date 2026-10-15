# Codex handoff — start here

Repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch: `codex/native-3d-bl`, based on Goal2
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`. Full Goal3 ACTIVE and incomplete.

Current connected capability is constrained coupled metric-quality optimization
inside native private repair. Actual-query original-P1+geometric BL/fade, geometry,
first height, quality/full lengths/dependencies/atomic publication remain strict.
Read [the implementation, evidence and remaining blocker](Handbook/GOAL3_PATCH_OPTIMIZATION.md).

The actual hard50-cell cylinder patch now passes frozen metric/height/progress
with85 cells in15 trials. The same1402-cell full target reaches1984 cells/25 long
edges (previous38), still rejecting at300s/3985 of4096 work. All25 remaining edges
are physical:13 cylinder/12 rim; none interior. This is partial generation, not
an accepted curved adapted grid or a speedup.

470 derivative assertions/five repair controls pass. Independent local and whole
geometry/no overlap/98 exact wall feet/height/exported-tensor arithmetic pass;
bounded MPI2 donor/reference protocol passes. Cold curved construction/removal,
currentMPI4/M<N/current+BDF/SA/CFD/restart/accuracy/performance remain unqualified.
The earlier wall-front and eager-repair scheduling failures are preserved; eager
scheduling is withdrawn. Current final engine uses published scheduling.

Cases under `integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/`:
`metric_derivative_controls`, `curved_repair_controls`, `curved_metric_patch_probe`,
`curved_metric_complete`, `curved_metric_residual`,
`curved_joint_protocol_metric_mpi2`; inspect `coupled_metric_summary.json`.
All owned jobs terminal. Final whole/MPI2/residual source/binary pins match;
primitive/direct-patch stages use the documented earlier engine binary with
unchanged exercised repair/metric objects. Verify relevant provenance on reuse.

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
