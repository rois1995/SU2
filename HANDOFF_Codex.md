# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-bl`, based on Goal2
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`. Full Goal3 ACTIVE/incomplete.
Production remeshing source remains exactly the published00e3ee4 checkpoint.

**Required next milestone:** a complete accepted curved mesh under unchanged
constraints. User2026-10-15: if current recovery cannot deliver it, reconsider
construction strategy before adding another local fallback. Local accepted patches
and residual reductions are diagnosis, not capability milestones.

The latest recovery fails that requirement. Independent branches/private carry
accept the actual blocked boundary edit, but complete1402-cell runs reject:
eager79 and staged30 long edges, versus published25. The staged1918-cell result
passes independent exact geometry/no overlap/91 normal feet/height/exported-tensor
q/J arithmetic;30 native full composed lengths still reject (15 cylinder/15 rim).
Original sensor donors/reference stay unchanged. No infeasibility or speedup claim.

Rejected source and complete evidence are pushed on
`codex/native-3d-boundary-recovery-study`, commit
`39a1f487a628b72b896a0df1bd4069e7b7a610fa`.
This branch preserves the decision, receipts, source snapshots and inspectable grids
without promoting that recovery implementation. Read
[construction reassessment](Handbook/GOAL3_CONSTRUCTION_REASSESSMENT.md),
[recovery evidence](Handbook/GOAL3_BOUNDARY_RECOVERY.md), and
[verified summary](integration_evidence/native_3d_bl_v1/wall_reconstruction_v1/boundary_recovery_study_summary.json).

All owned jobs are terminal. The build-integrated-v2 binaries/objects currently
belong to the study, not restored production sources. Rebuild only when a connected
implementation needs validation; do not reuse study results as current-source
qualification. To reproduce new study recipes, check out its pinned commit first.
No new MPI/CFD run was performed for the rejected policy; the study MPI scaffold is
compiled but unqualified. Installed production binaries were not changed.

[Full remaining Goal3 gates](Handbook/GOAL3_CURRENT.md),
[requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json),
[permanent rules](AGENTS.md), and
[deferred SST/transition/non-ideal-gas questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).

One owned heavy job, nice19/threads1/build-j1/MPI≤4, bounded sampled affinity. No
cluster submission or messaging. Leave AdapNoExt, foreign ClusterResults and
dependencies untouched. No production Euler/BL modes, weaker gates, field/target
resets, budget ladders or local-fallback chains in place of complete construction.
