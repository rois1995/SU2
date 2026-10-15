# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.
Branch `codex/native-3d-bl`, based on Goal2
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`. Full Goal3 ACTIVE/incomplete.
The connected distributed-region-field checkpoint9bd5fb76e4 is qualified/pushed.
The subsequent actual curved RANS/current+BDF/SA check compiles but exposes coarse-cell
BL complexity quadrature exhaustion before remeshing. Extending the spherical radial
fan to a certified cylinder-axis point also fails its volume control. Both failures
and initial invalid-config refusal are preserved. Rejected production change restored
to9bd5fb76e4; local binary still contains the rejected object and MUST be rebuilt.
All owned jobs terminal. No new curved RANS runtime qualification.
Read [failure evidence, next integration strategy and gates](Handbook/GOAL3_CURVED_RANS_RUNTIME.md). No cluster submission, messaging,
foreign `ClusterResults/`, AdapNoExt or dependency modification.

**Complete curved meshes are accepted under unchanged constraints.** The original
1402-cell difficult target completes with1368 cells/423 nodes, zero long edges,
qmin .211386792/Jmin .052846703/Lmax1.795195. Final compatibility SU2/ref bytes and
original donors match prior independent geometry/height/embedding evidence, explicitly
reused. Rejected local recovery stays on `codex/native-3d-boundary-recovery-study`;
do not resume its fallback chain.

**Actual source refresh/removal/rebuild passes MPI1/2/4.** Original24-cell cylinder
->1368 BL, import that actual mesh with affine sensors `diag(1+.02*z)` ->3072 donors
->72 sensor-only cells, import again ->288 donors ->1368 BL. Rebuild qmin>=.33135,
Jmin>=.09572/full Lmax1.795195; analytic sensor/composed target residual<=1.67e-14.
Height.05/growth1.2/thickness.2/fade, geometry, q>=.20/J>=.05/L<=1.8 remain unchanged.
Each refreshed event retains4096-work/300s; no shared4096 claim across this whole test.

Inspect actual `refreshed_released.su2` and `refreshed_BL.su2`, each with `.native_ref`,
in `integration_evidence/native_3d_bl_v1/region_construction_v1/native_refreshed_source_v2/`.
No CFD solution is fabricated. [Design, gates and limits](Handbook/GOAL3_DISTRIBUTED_REGION_FIELD.md),
[complete provenance](integration_evidence/native_3d_bl_v1/region_construction_v1/distributed_field_summary.json).
The750-cell distributed analytic control passes point P1/composition, exact segment
footprint/integral, bounded storage and refusal/recovery checks.

Whole-source256 donor admission is removed. Original donors remain distributed;
each actual point/segment footprint and resident cache still fits256. Point memoization
stores sensors only; BL composition runs at actual queries. All ranks use matching
query/stop collectives on their existing MPI thread, including exceptions. Geometry
remains one visible radial/caps region on one worker. Rebuild records2529..2567 imports;
no low communication cost, lifecycle speedup or scalable generation claim follows,
especially under host contention.

**Next connected work:** geometry-aligned cylindrical annulus integration, then actual
curved RANS evolution and conservative
current+BDF/SA transfer/restart with refreshed sources, reusing existing runtime
fixtures. Current tests import actual CFD geometry but do not advance or transfer
flow histories. Full Goal3 also needs stronger/interacting/exterior fields/geometry,
high-ratio metric-driven multiple fronts, thin/composed-gradation limits, practical
M6/viscous accuracy and complete lifecycle performance. Keep those gates intact.

Previous cold24 ->1368 ->68 shared-work MPI1/2/4 proof and independent BL/released
audits remain in [cold evidence](integration_evidence/native_3d_bl_v1/region_construction_v1/cold_summary.json).
Previous primitive-runtime current+BDF/SU2/CGNS restart evidence retains its exact
source/build scope; it does not qualify the new refreshed curved CFD workflow.

[Full current scope](Handbook/GOAL3_CURRENT.md),
[requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json),
[permanent rules](AGENTS.md), and
[deferred SST/transition/non-ideal-gas questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).

One owned heavy job, nice19/threads1/build-j1/MPI<=4, bounded sampled affinity.
Group substantial capability changes, reuse appropriately pinned evidence, preserve
failures/inspectable meshes, avoid full suites after small edits and push regularly.
No production Euler/BL modes, weaker gates, target resets, budget ladders or another
local-fallback chain in place of complete construction.
