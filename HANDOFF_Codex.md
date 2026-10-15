# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-bl`, based on Goal2
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`. Full Goal3 ACTIVE/incomplete.
Production remeshing source remains exactly the published00e3ee4 checkpoint.

**Required next milestone:** a complete accepted curved mesh under unchanged
constraints. User2026-10-15: if current recovery cannot deliver it, reconsider
construction strategy before adding another local fallback. Local accepted patches
and residual reductions are diagnosis, not capability milestones.

The local-recovery strategy remains rejected and archived on
`codex/native-3d-boundary-recovery-study` at39a1f487a628b72b896a0df1bd4069e7b7a610fa.
Production C++ remains00e3. A different connected construction now produces a
complete **prototype** candidate from the actual1402-cell target:1368 cells,
native qmin .215225/Jmin .053451,2074 full edges all≤1.8 (Lmax1.795195).
Independent exact embedding/no overlap/all vertex links/64 normal feet/height/
authority and exported-tensor arithmetic pass. Original sensors/BL/fade/geometry/
constraints remain unchanged. This is not integrated native runtime acceptance.

Read [the connected construction and required native port](Handbook/GOAL3_REGION_CONSTRUCTION.md)
and [its exact evidence](integration_evidence/native_3d_bl_v1/region_construction_v1/summary.json).
Inspect `region_construction_v1/metric_region/candidate_validated.su2` or
`candidate_metrics.vtu`. No CFD solution is fabricated. Earlier failed region
proposals are retained; do not turn their partial checks into milestones.

Next implement this complete surface/band/transition construction in native C++
with actual FrozenField queries, existing configuration and bounded collective-safe
MPI/atomic state handling. No proxies, saved tensors/grid, fixture dispatch or
Python dependency may enter production. Validate the unchanged full target before
calling the required milestone achieved. Full Goal3 remains open.

All owned jobs are terminal. Existing build-integrated-v2 binaries belong to39a1f487,
not restored production sources; the read-only native field oracle verifies that
exact binary and captured source provenance. Rebuild only for the connected native
implementation, then requalify affected checks. No new MPI/CFD run was performed.

[Full remaining Goal3 gates](Handbook/GOAL3_CURRENT.md),
[requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json),
[permanent rules](AGENTS.md), and
[deferred SST/transition/non-ideal-gas questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).

One owned heavy job, nice19/threads1/build-j1/MPI≤4, bounded sampled affinity. No
cluster submission or messaging. Leave AdapNoExt, foreign ClusterResults and
dependencies untouched. No production Euler/BL modes, weaker gates, field/target
resets, budget ladders or local-fallback chains in place of complete construction.
