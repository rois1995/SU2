# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`, Goal2 base `c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
**Full Goal3 ACTIVE/incomplete.** Preserve its full
[scope and gates](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
The pre-intermediate-front campaign is complete15/15: original/rotated/varying
fields pass native+independent geometry/source MPI1/2/4; thin controls pass3/877746.
Actual RANS/SA N/M1/1,2/1,2/2,4/1,4/2,4/4 passes24->836->48, current+BDF/SA accounting,
SU2/CGNS/reference/restart and independent audits/replays. All prior native/audit
jobs are terminal; phase clocks are saved diagnostics, not a CFD-only ratio.
[Qualified campaign](integration_evidence/native_3d_bl_v1/region_construction_v1/current_integration_v11.json),
[actual MPI lifecycle](integration_evidence/native_3d_bl_v1/region_construction_v1/rans_lifecycle_v11.json).
Current connected front extension compiled successfully; all owned jobs are terminal.
Original stronger rotatedv12 accepts3108 cells/work4057, independently passes geometry/
height/P1, and exactly matches every geometric cell/target sample/full edge of the
fully auditedv10 numerical reference. High-ratio native_front_constant_v1 rejects
at work4097>4096 after3 private fronts/3367 cells; input is independently identical.
The fixed Band needs840 cells per front. Even full1.8 radial steps need>=4 more
bands: >=4200 cells before core/planning, so this strategy cannot fit4096.
[Current result and next action](integration_evidence/native_3d_bl_v1/region_construction_v1/metric_fronts_v1_summary.json).
Next reconsider coordinated transition connectivity/front distribution/core
interface; do not add local fallbacks, tweak step size or increase the budget.
Prisms remain rejected (direct L2.026>1.8; two-step q.185/J.0308). The metric-apex
hypothesis is ruled out: old apex shapes pass at36 intervals, but full curved
diagonals reach2.042;42 intervals give1.783. No apex change enters production.
Six regional phase clocks are instrumented; no updated MPI/actual RANS/profile
qualification of the new source yet. No build/native/audit/cluster job is live.
Foreign `ClusterResults/`, AdapNoExt and dependencies remain untouched.

## Current complete curved-mesh milestone

The same stronger frozen rotated target now passes native and independent audits:
3108 cells/2 core regions/4056 work, qmin.412100104/Jmin.156402454/Lmax1.645505430.
Exact whole embedding/no overlap/all740 vertex links/168 wall feet-height,
analytic geometry and every exported original-P1 sensor pass.252 pure-core cells/
466 analytic full edges have residual<=1.95e-16. A subsequent independent NumPy wall-covariance/log interpolation/growth/fade/
Cholesky-intersection and adaptive Gauss16/32 audit now verifies all3108 tensors
and4120 complete edges: tensor residual8.90e-14, length residual5.43e-10. This is
a numerical reference, not a rigorous integration enclosure. No CFD in this fixture.
Original mesh/donors are byte-identical across rejected rotated v4/v5/v9 and
accepted v10. h.05/growth1.2/T.2/fade/Hausdorff.04/wall ceiling.025/q.20/J.05/
L1.8/work4096/300s remain unchanged. Source-collar generation is untouched.
[Accepted grid](integration_evidence/native_3d_bl_v1/region_construction_v1/native_core_rotated_v10/core_accepted.su2),
[complete provenance](integration_evidence/native_3d_bl_v1/region_construction_v1/rotated_geometry_summary.json).

Shared quadrature checkpoint4179246ad2 is pushed: rounded-root brackets and global
nested endpoint Lobatto3/5->5/7. Same error/evaluation/depth/cost limits; microscopic
control6688 vs14133 fixed-order calls, below unchanged10000 gate. No wall-time claim.
The subsequent AxisFrame closest-cylinder/disk and cylinder projection fix passes
12 focused cases/1014 assertions plus the complete rotated mesh. It prevents near-axis
normalization from leaving the analytic authority; no metric-normal reset/fallback.
[Design, failures and limits](Handbook/GOAL3_METRIC_REGION_CORE.md).

## Recorded scope and next substantial work

The prior21d9cbcb0c front/core checkpoint independently accepts constant and varying
stronger fields (3108/2894 cells), preserving identical targets across rejections.
The prior907646e00e actual short RANS/SA matrix passes N/M1/1,2/1,2/2,4/1,4/2,4/4,
including actual CFD/current+BDF transfer/SU2/CGNS/reference/restart. These are exact
old-source records, not MPI/CFD qualification of the changed constructor/field/geometry.
Pressure sensors clip to the coarse bound; post-bounds SA has explicitly accounted
solver-floor corrections and is not claimed exactly conserved.
[Actual RANS evidence/limits](Handbook/GOAL3_CURVED_RANS_RUNTIME.md).

Next qualify the remaining varying-field MPI cases and updated actual RANS
N=M/M<N transfer/output/restart workflow. Original-target and stronger-rotated
MPI1/2/4 qualification is complete in its recorded scope. Keep the full
Goal3 gates: high-ratio metric-driven multiple fronts, general/exterior/interacting
regions, independent remaining BL integrals, composed-gradation limits, viscous/M6
accuracy where practical, and complete adaptation/CFD lifecycle affordability.
Current radial construction remains one generation worker; do not claim general
geometry, structured layers, scaling or performance from these frozen pilots.
Goal4 starts only after full Goal3 completion.
The connected front design and proved span/cell-cost obstructions are in
[metric-driven fronts](Handbook/GOAL3_METRIC_FRONTS.md). No complete accepted
high-ratio multi-front mesh has yet been demonstrated.

[Full current qualification](Handbook/GOAL3_CURRENT.md),
[distributed immutable source service](Handbook/GOAL3_DISTRIBUTED_REGION_FIELD.md),
[permanent rules](AGENTS.md),
[deferred SST/transition/nonideal questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
One owned heavy task, nice19/threads1/build-j1/MPI<=4; inspect contention, announce
full case folders, verify live handles before restart, preserve failures and
resumable provenance. Group substantial work; no broad suite after tiny edits.
Push reviewable checkpoints regularly. Never narrow the goal to passing pilots.
