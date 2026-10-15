# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-bl`, based on Goal2
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`. Full Goal3 ACTIVE/incomplete.
**Complete cold/removal milestone achieved on MPI1/2/4:** original24-cell/18-node
coarse cylinder ->1368 BL cells ->68 sensor-only cells. Total charged work3132/3172/3252
under unchanged shared4096-work/300s limits. BL qmin>=.21137/Jmin>=.05284/Lmax1.795195;
released qmin .33418/Jmin .19717/Lmax1.119412. Both physical boundaries and volume
are rebuilt according to requested metric/height/geometry; no solver/testcase modes.

Inspect `region_construction_v1/native_cold_v3/curved_cylinder_BL.su2` and
`curved_cylinder_released.su2`, each with `.native_ref` (prefix under
`integration_evidence/native_3d_bl_v1/`). No CFD solution is fabricated.
[Cold/removal proof and provenance](integration_evidence/native_3d_bl_v1/region_construction_v1/cold_summary.json),
[focused handbook](Handbook/GOAL3_COLD_CONSTRUCTION.md).

Radial sampling now resolves newly created edges before volume construction;
rejected surface collapses are reused only with unchanged endpoint-star versions.
No-height policies test a metric-driven core; finer interior fields that need more
than one cone retain the existing volume operators. Rejected generation work is
counted and failures report locations/budgets. The preserved original976-cell cold
proposal has58 long edges/16 bad cap cells; its field-export PASS is diagnosis only.

Independent serial BL exact geometry/height/feet/tensor arithmetic and independent
released exact geometry/constant-P1 metric audits on all3 outcomes pass. Original hard
target still completes (`native_complete_v3`,2940 work); its SU2/ref are byte-identical
to the prior independently audited result, with explicit verified reuse. Existing
primitive-runtime transfer/BDF/SU2/CGNS restart checks pass2557 assertions; production
objects/libraries and that test body are unchanged after later cold-harness logging,
so reuse is recorded rather than claiming execution with the final harness binary.

Full Goal3 remains open. Next major capability: distributed original-field queries
for complete regions, then actual curved RANS/current+BDF/transfer/restart and stronger
fields/geometry. The regional256-original-donor bound is still real: larger sources
retain incremental adaptation, which does not establish repeated whole-region BL
rebuilding from a generated1368-cell CFD source. High thickness/height ratio targets
also need metric-driven multiple fronts, not another local-fallback chain or a forced
jump beyond the fade. Keep all full-goal physics/accuracy/performance gates intact.
All owned jobs are terminal. The connected cold/removal batch is the current publication checkpoint;
verify HEAD/remote for the final commit rather than assuming the earlier checkpoint.

The earlier native curved-target baseline and its inspectable quality VTU remain
[archived with exact evidence](integration_evidence/native_3d_bl_v1/region_construction_v1/native_summary.json).
The failed local-recovery strategy stays on `codex/native-3d-boundary-recovery-study`;
do not resume its fallback chain. Foreign `ClusterResults/`, AdapNoExt and dependencies
remain untouched; no cluster submission or messaging was performed.

[Full remaining Goal3 gates](Handbook/GOAL3_CURRENT.md),
[requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json),
[permanent rules](AGENTS.md), and
[deferred SST/transition/non-ideal-gas questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).

One owned heavy job, nice19/threads1/build-j1/MPI≤4, bounded sampled affinity. No
cluster submission or messaging. Leave AdapNoExt, foreign ClusterResults and
dependencies untouched. No production Euler/BL modes, weaker gates, field/target
resets, budget ladders or local-fallback chains in place of complete construction.
