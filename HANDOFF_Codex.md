# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`, Goal2 base `c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
**Full Goal3 ACTIVE/incomplete.** Preserve its full
[scope and gates](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
Published6c344dcf7b contains whole-cap construction and the shared closest-facet
correction. Prior c103aa9cf3 preserves the withdrawn cap strategy. Current working
follow-up retains an already admissible complete front plan before resampling.

## Current connected work

Whole-cap point distribution replaces inherited midpoint cap refinement in the
certified right-prism constructor. Boundary full-metric equidistribution, one
proposal chart, native polygon triangulation/containing-cavity insertion/local
reconnection and shared3D front closure produce the complete region. No fallback
or increased work/time/gate. Original-P1 sensor-only donors and actual-query
geometric BL/finer/fade remain. Offline SciPy is not a production dependency.

The first complete1248-cell candidate failed native full-edge evaluation; independent
exact geometry/height/all1967 full numerical edges pass q.357911/J.153523/L1.699959.
Focused replay found the ADT's tolerant facet choice can produce a false in-plane
normal near a coplanar wall diagonal: tensor error96/Mzz100->4. Shared Evaluate now
rechecks actual closest points in the conservative nearest-distance envelope.
The same frozen edge passes with tensor error2.56e-13 and length error6.6e-16,
without changing sensors, normal/fade formulas, quadrature tolerance or65536 bound.
This corrects geometric facet selection; it is not a hard-normal reset.

Current native and independent full audits pass constant/rotating planar demands:
6->1248->447 and6->2112->722, work1646/628 and2728/877. MPI4 rotating passes
6->2112->700, independently; constructed geometry exactly equals serial. Local
constant cap4.63s nested in33.26s construction; no speedup/scaling claim.
Unchanged curved high-ratio v3 accepts2877/eight fronts/work3771. Bit-identical
model/mesh lets the complete independent reference transfer after checking all2877
new tensors/shapes and4727 edges, bound1.064e-14/edge residual4.986e-9. Existing
12 edge/geometry controls and the new edge regression pass. All current source/
binary/object/artifact pins checked in tangential_caps_v2/validation.json.
The batch above is scoped to6c344dcf7b. Current follow-up checks complete actual3D
cap/front admissibility before point generation, using the same prism diagonals
and final whole-region gates. Focused coarse retention and strong v5 workflow pass;
strong outputs exactly reuse v4 independent inputs/proof. Actual initialized planar
N4/M2 v3 now passes6->24->6 (v2 oversampled96), independently full geometry/metric/
current+BDF flow/both output formats/reference/restart, integral defect<=2.75e-16.
This avoids unnecessary cell growth without changing the target or adding recovery.
Builds terminal PASS. Actual evolved curved N4/M2 v14 passes native, independent
whole-mesh/current+BDF/restart checks and SA saved-state replayv28. No job remains
from that batch. User-requested ONERA M6 test now takes priority over the queued
opposing-wall regression. Its folder is
`integration_evidence/native_3d_bl_v1/oneram6_coarse_v1`: supplied Coarse.su2,
303359 tetrahedra/61330 points, real WING3D no-slip/SYMMETRY/FARFIELD, SA at
Mach.8395/AOA3.06/Re11.72e6. Bounded120 initial iterations, one native adaptation,
first height2e-6/g1.2/T.01, original-P1 sensors and actual-query BL. Four ranks,
300s native deadline/1200s process ceiling. Preparation pins current dirty source
and fresh solver. All M6 jobs are terminal. First attempt exposed missing QLS
WLS-fallback workspace with primal GG; shared allocation fixed, analytic check
passes1 case/32260 assertions. Identical-config retry passes all120 flow iterations
and122660 MPI QLS fits, then exhausts65536 thin-BL complexity points on global
cell13 (upper outer wing near tip), BEFORE remeshing. No candidate/adapted grid.
Saved matching `flow_checkpoint/flow.vtu`, surface_flow.vtu and Float64 restart
with exact printed-history/coordinate identity; `failure_cell_13.vtu` locates the
46-cell input neighborhood. Next M6 blocker is thin-volume integration, not yet
a measured construction rejection. No M6 task is queued.
[M6 evidence/diagnosis](Handbook/GOAL3_ONERA_M6.md).
No M6 acceptance or converged aerodynamic claim yet. No wider curved
capability or full Goal3 completion; no local fallback or increased limits.
[Current source/design/evidence](Handbook/GOAL3_WHOLE_CAP_CONSTRUCTION.md).

The earlier cap-operator attempt failed3731 trials/30 cap faces; it was withdrawn
at c103. All50 artifact hashes/exact source snapshots are verified and preserved.
The six-cell returned SU2 file is the unchanged input, not a private-cap snapshot.
[Failure and strategy decision](Handbook/GOAL3_TANGENTIAL_CAPS.md).

## Qualified foundations and scope

High-ratio curved cylinder:2877 cells/eight fronts/work3771, q.223149/J.060674/
full L1.783149, height.01/g1.2/T.2/length.2 and4096/300s unchanged. Independent
exact geometry/height/original-P1 and all2877 tensors/all4727 numerical edges pass.
Rotated/varying serial2448/2284 also pass native/exact geometry/source audits;
full independent nonconstant BL integrals remain open. MPI2/4 high-ratio native/
geometry pass with different coordinates/IDs; no serial full-metric proof reuse.
[Accepted curved grid](integration_evidence/native_3d_bl_v1/region_construction_v1/native_front_constant_v2/core_accepted.su2),
[construction and limits](Handbook/GOAL3_COARSENED_METRIC_FRONTS.md).
These are prior-source receipts, not automatic proof for the current metric change.

Published complete-star coordination closes retained opposing-wall removal:
MPI4 1920->250/work1619, MPI2->230/work1344, zero final conflicts, independent
full fields/exact geometry. Earlier work-budget and transport failures stay saved.
Actual initialized planar RANS/SA N4/M2 backend6->24->6 passes independent geometry/
height/full edges/current+BDF flow, transported sensor0, SU2/CGNS/reference/restart.
It does not evolve CFD or establish post-floor exact SA conservation. Sensor
gradation passes;44 composed probes violate HGRAD1.3, max24.5184, uncorrected.
[Qualified selection/backend evidence](Handbook/GOAL3_SELECTION_WORK.md).

Earlier evolved curved RANS N4/M2 v13 passes24->621->48 with current+BDF/open ledger,
SA-floor replayv27, both outputs/reference and restart. Post-bounds SA is not
exactly conservative; pressure demand clips coarse. No viscous accuracy follows.
The full15-case/six N/M actual campaign is pre-front source only.
[Curved RANS history](Handbook/GOAL3_CURVED_RANS_RUNTIME.md),
[full earlier campaign](integration_evidence/native_3d_bl_v1/region_construction_v1/current_integration_v11.json).
Rank0 passive1398 samples identify metric-stage MPI waits and exact contacts;
no critical-rank/scaling/affordability claim. No extra profiling CFD run.

## Full remaining scope and rules

Constructive classes remain one inward cylinder with planar caps and finite
right prisms, on one generation worker. One terminal cylinder coarsening and a
heuristic positive BL-free core remain limits, not general mesh-generation proof.
General features/junctions/nonprismatic/exterior geometry, interactions, combined
gradation, independent varying BL integrals, repeated retained-BL updates,
viscous/M6 accuracy and complete lifecycle affordability remain. Goal4 is inactive.

[Requirement audit](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json),
[current history](Handbook/GOAL3_CURRENT.md),
[distributed source service](Handbook/GOAL3_DISTRIBUTED_REGION_FIELD.md),
[deferred SST/transition/nonideal questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
One owned heavy task, nice19/threads1/build-j1/MPI<=4; announce full case folders,
check host contention and live handles, preserve resumable evidence/failures.
Group connected capability changes, avoid fallback/budget ladders and unnecessary
regressions; maintain handbooks and push reviewable checkpoints. See [rules](AGENTS.md).
Foreign `ClusterResults/`, AdapNoExt and dependencies remain untouched.
