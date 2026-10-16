# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`, Goal2 base `c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
**Full Goal3 ACTIVE/incomplete.** Preserve its full
[scope and gates](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).

## Current milestone and live work

The high-ratio curved target now has a complete native accepted mesh under the
original constraints: 2877 tetrahedra, eight transition fronts, work3771<=4096,
qmin.223148627/Jmin.060674227/full Lmax1.783148856<=1.8. Height.01, growth1.2,
thickness.2, length.2, Hausdorff.04/wall ceiling.005, original sensor/fade and300s
remain unchanged. Independent exact geometry/embedding/links/height/original-P1
and all2877 actual tensors/all4727 full metric edges pass. The reference is
independent numerical quadrature, not a rigorous enclosure.
[Accepted grid](integration_evidence/native_3d_bl_v1/region_construction_v1/native_front_constant_v2/core_accepted.su2).

This follows a construction change, not another recovery fallback: preserve the
first band, build coherently staggered metric-spaced free fronts, and coarsen the
terminal front together with every incident transition/core cell. The earlier
fixed840-cells-per-band strategy was proved unable to fit4096. Its failure and
rejected alternatives remain preserved.
[Design, evidence and limitations](Handbook/GOAL3_COARSENED_METRIC_FRONTS.md).
The new rotated and spatially varying serial cases also independently accept
2448/2284 cells, work3259/3120. Native metrics and geometry/source are checked;
full independent varying-BL quadrature remains open. Regional validation takes
44–66s locally; the attempted passive sampler arrived after completion and was
explicitly skipped. Zero region-service counters in these small direct-source
fixtures do not mean zero metric evaluations.

Changed-source high-ratio MPI2/4 native and independent geometry/source audits
now pass2877 cells/work3771 with the same gate extrema. Floating coordinates/IDs
differ; do not reuse the serial full numerical metric certificate for MPI.
Actual `native_curved_rans_n4_m2_v12` now passes24->621->48 with independent
geometry/current+BDF/open-ledger/SU2/CGNS/reference/restart audits. Separate
`sa_projection_replay_v26` verifies raw projection and every solver-floor
correction; post-bounds SA is not exactly conservative. New-source actual RANS
qualification is N4/M2 only; the earlier six-way matrix retains pre-front scope.
[Complete current milestone receipt](integration_evidence/native_3d_bl_v1/region_construction_v1/coarsened_fronts_v1_summary.json).
The next connected capability now accepts the overlapping opposing-wall target:
`interacting_walls_v1/native_opposing_v3` passes6->1920 BL->250 sensor-only,
construction/removal work2309/1269. Independent exact geometry/height/all2674
full composed edges and all20 complete front-slab coverage pass. BL q.467316/
J.193160/L1.668063; all46 wall-node normal edges exist, but centroid tilt13.1762
is separately recorded. The baseline local path exhausted4096 with30 private
cells and no accepted metric repair. The primary geometry-certified prismatic
constructor joins two prescribed rows without requiring a BL-free core.
Source/build/binary, failed admission/audit attempts and accepted grids are saved.
[Current interacting-wall capability](Handbook/GOAL3_INTERACTING_WALLS.md).
Next qualify actual backend refinement/coarsening/transfer/restart and a single-
wall/fade field using region_walls metadata (the older direct single-wall control
leaves that metadata empty and therefore does not exercise the new primary path).
Also test rotated/localized tangential demands. Direct engine Coarsen is not the
full backend sequence. Do not assume one overlapping scalar case proves all admitted
prisms; maximum-prefix spacing and inherited cap triangulation need these stresses.
The earlier c032 opposing construction passes MPI1/2/4. Its MPI4 removal refusal
(4084 proposals/496 cells), followed by the967 duplicate-only refusal (4091/319),
remain preserved. Complete-star selection now closes that removal gate; see below.
Current-source curved RANS N4/M2 v13 also passes24->621->48 with independent
current+BDF/open-ledger, explicit SA-floor replayv27, SU2/CGNS/reference/restart.
Actual prismatic backend/CFD workflow is still unqualified. A passive rank0
profile (1398 samples, compressed raw data) identifies metric-stage MPI waits and
exact contact arithmetic as significant costs; no extra CFD run or speedup claim.
The connected complete-star selection now closes strict retained-BL removal:
MPI4 1920->250 cells/work1619, zero final reservation conflicts; independent exact
geometry/links/no overlap/full original field pass q.306655/J.102558/L1.783027.
Metadata5390 considered/3771 deferred after2704 duplicate suppressions; planning
.145s nested in9.07s local removal. These are diagnostic timings, not a speedup.
MPI2 completes1920->230/work1344 with zero final conflicts, independently all gates.
Its suite FAIL from an incorrect duplicate-only counter expectation is preserved;
separate corrected controls pass on identical production sources/libraries/solver,
with exact sole-test-change proof. No accepted engine result was rerun for this.
Focused planning/legacy batch/refusal controls pass serial and MPI4; the first
fixed-record transport error is preserved and corrected with separate reply types.
The actual initialized planar RANS/SA N4/M2 backend now passes6->24->6, independently
all full composed edges/geometry/height/complete normal slabs/current+BDF flow,
transported sensor0, SU2/CGNS/reference and real restart. Native checks bounded SA;
post-bounds exact SA conservation is not claimed. No planar CFD evolution/viscous
accuracy follows. Independent finite sensor gradation passes; composed ratio24.5184
still violates HGRAD1.3 and remains uncorrected before freezing.
[Current design/results](Handbook/GOAL3_SELECTION_WORK.md),
[complete scoped receipt](integration_evidence/native_3d_bl_v1/interacting_walls_v1/selection_work_v2_summary.json).
Next focus rotating/localized tangential targets and nonprismatic feature/junction
construction, then evolved viscous accuracy and complete lifecycle affordability.
No owned native/build/audit process is live. Full Goal3 remains active/incomplete.



## Qualified prior foundations

The pre-front campaign is complete15/15: original/rotated/varying frozen fields
native+independent geometry/source MPI1/2/4; thin controls3/877746. Actual RANS/SA
N/M1/1,2/1,2/2,4/1,4/2,4/4 passes24->836->48 with independent physical current+BDF,
explicit bounded-SA correction accounting, SU2/CGNS/reference and real restart.
Post-bounds SA is not exactly conservative; pressure metrics clip coarse, so no
viscous accuracy or affordability claim follows.
[Qualified campaign](integration_evidence/native_3d_bl_v1/region_construction_v1/current_integration_v11.json),
[actual MPI lifecycle](integration_evidence/native_3d_bl_v1/region_construction_v1/rans_lifecycle_v11.json).

Shared geometry AxisFrame and globally controlled edge quadrature are retained.
Original stronger rotated3108-cell mesh has an independent complete numerical
metric reference. Failed/interrupted references and the fixed-band obstruction
are historical evidence, not current blockers for the accepted high-ratio case.
[Earlier construction investigation](Handbook/GOAL3_METRIC_FRONTS.md).

## Full remaining scope and working rules

Certified constructive classes are one inward cylinder with planar caps and
finite right prisms with requested walls on opposite caps, each on one worker.
The cylinder retains one terminal coarsening pass and a heuristic BL-free core.
This does not establish general/exterior/interacting geometry, arbitrary aspect
ratios, structured layers or scalable parallel generation. Wider geometry/features,
independent nonconstant BL integrals, separate composed-gradation feasibility,
repeated retained-BL updates, viscous/M6 accuracy and complete lifecycle cost remain.
Goal4 starts only after full Goal3 completion.

[Requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json),
[current qualification/history](Handbook/GOAL3_CURRENT.md),
[distributed source service](Handbook/GOAL3_DISTRIBUTED_REGION_FIELD.md),
[deferred SST/transition/nonideal questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
One owned heavy task, nice19/threads1/build-j1/MPI<=4; announce full case folders,
inspect contention and live handles, preserve resumable evidence and failures.
Group connected work, avoid repeated full suites and fallback/budget ladders,
maintain handbooks and push reviewable checkpoints. See [permanent rules](AGENTS.md).
Foreign `ClusterResults/`, AdapNoExt and dependencies remain untouched.
