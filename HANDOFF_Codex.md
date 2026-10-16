# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`, Goal2 base `c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
**Full Goal3 incomplete; uncertainty experiment authorized 2026-10-16.** Preserve its full
[scope and gates](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
Published4f5c8a9c6a includes whole-cap construction, closest-facet correction,
admissible cap/front retention and the QLS workspace fix. Current work addresses
the practical M6 thin-volume integration failure before wing remeshing.

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
from that batch.

M6 actual RANS/SA completes120 initial iterations; QLS workspace fix and original
flow/restart/input-failure views are published. Subsequent saved-state events fail
BEFORE remeshing. Published977691bc4e fixes a demonstrated interval-relative
integration defect and passes7 controls plus full-unit22/32; it is not a full M6 fix.
All4 normal reference charts now agree4.432e-5; radial16/24 change1.223e-7.
Actual v2 runtime failures88/157/329 have exact first-trial sensor/core/coordinate
records and closed-wall replays. Cell329 reproduces the native failure verbatim
in6.23s without CFD. No candidate/accepted mesh/transfer. The primary wall-normal-column experiment and the subsequent cell-wide error
ledger both fail all3 actual fields and are withdrawn. Cell329/157 exceed65536
points;88 times out90s. Correcting a demonstrated queue eligibility bug does not
fix the decisive failure. Production metric source remains977691/412230 and fresh baseline binaries now
match it. Complete4-rank pre-quadrature checkpoint captures61330 points/303359
cells; all3 failing fields match exactly. Both owner and halo raw tensors retained.
Sensor-only Q18 mass152994 (bounds142421..154272), not full BL/final normalization.
Test-only point-query interface passes saved samples and malformed inputs.
Independent full-density references88/329 at128 and157 at72 now complete; final
chart gaps<1e-6. No reference job remains. Current bounded-complexity experiment
is experimental, not yet production normalization; see
[uncertainty work and remaining gates](Handbook/GOAL3_UNCERTAINTY_COMPLEXITY.md).
Conditioned scalar fan and strict controls pass;159-cell prefix is incomplete and
expensive. Construction-only MPI4 replay is terminal rejected by a first-layer
composed edge integral, separately from volume integration. No owned job remains.
User now prioritizes uncertain normalization/global error/geometry reuse; unbuilt
subdivision extension withdrawn. Three-element30P30N coarse2D added to validation.
Production normalization remains unchanged; no M6 mesh or full-field integral yet.
[Current whole-field checkpoint and reassessment](Handbook/GOAL3_M6_COMPLEXITY_REASSESSMENT.md).
[Current exact-field/replay/design/live state](Handbook/GOAL3_M6_FROZEN_FIELDS.md),
[earlier thin integration and rejected experiments](Handbook/GOAL3_M6_THIN_INTEGRATION.md),
[actual initial M6 flow/inspectable files](Handbook/GOAL3_ONERA_M6.md).
Full Goal3 stays active; full tensor accuracy, actual M6 construction and lifecycle
cost remain unproven. Three completed model-confirmed read-only Opus reviews are preserved; a fourth
high-effort structural request timed out without returning a review. User authorizes
Claude Code assistance with Opus5.5 at medium/high effort; keep reviews read-only.
[Whole-cap construction and scoped earlier proof](Handbook/GOAL3_WHOLE_CAP_CONSTRUCTION.md).

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
