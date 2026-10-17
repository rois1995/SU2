# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`; Goal2 base `c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
**Full Goal3 ACTIVE/incomplete.** Preserve the [full scope and gates](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
AdapNoExt, foreign ClusterResults and dependencies remain untouched.

## Current connected work

Primary experiment: uncertainty-controlled complexity initialization on original
coarse M6 sensor-only P1 plus actual-query geometric BL. Bound difficult work,
retain positive unresolved contributions, assess absolute error globally including
MPI, match normalization to integration uncertainty and reuse geometry across
scale trials. No post-freeze changes during local operations, finer-demand loss,
coarse wall-tensor spreading, hard-normal reset, fade/noise0/gate changes.

New numerical feasibility evidence: the strong BL demand in disjoint original
M6 wall-cell subsets alone estimates384179/382098 on two16384-sample seeds.
Conditional99% simultaneous statistical lower estimates364473/362392 exceed the
100000 target by at least262.39%. MPI1/4 match every actual query. Not an outward
machine certificate/full composed integral/accepted mesh. Refinement cannot lower
the continuum demand of the SAME frozen field; it may still ease integration.
Finite coarse support envelopes were too loose; supporting-plane floor inconclusive.
Do not silently change budget/BL constraints or add another fallback. Next: use
this feasibility diagnosis to reconsider bounded global initialization/out-of-range
normalization, while retaining all original gates. No new production policy yet.
[Derivation, negative experiments, work/RAM and receipts](Handbook/GOAL3_COMPLEXITY_SUPPORT_BOUNDS.md).

Current production normalization is UNCHANGED. Shared experimental controller and
3-cell MPI1/4 priority reductions agree but correctly return integration_limited
at1%; not a calibrated complete-field policy. Geometry caching works locally;
BL-correction decomposition helps329 (queries-7.35%/historical reference error10.6x
lower) but not88/157 or cold preparation. More subdivision is neither reliably
cheaper nor monotone. Scalar root screening was withdrawn. Full-field prefixes
are incomplete;14 unknown contributions in the earlier9442-cell prefix are kept,
not silently dropped. Scalar distance-only cuts restore the demonstrated622/2331
seed regressions.622 independent quadrature remains unconverged through96; no
more reference-order ladder. These are partial findings, not normalized M6 meshes.
[Uncertainty evidence](Handbook/GOAL3_UNCERTAINTY_COMPLEXITY.md),
[chord envelope/current global limits](Handbook/GOAL3_CHORD_ENVELOPE.md),
[whole-field/source audit](Handbook/GOAL3_M6_COMPLEXITY_REASSESSMENT.md).

Current follow-up addresses the shared strict edge blocker: local closest-feature
displacement, the existing wall-distance/barycentric normal-classification cuts,
and interior Gauss7/Kronrod15 on exact original source intervals. All error/work/
depth/mesh gates and sensor-only integration unchanged. Geometry cuts alone still
failed the second actual surface edge; endpoint-inclusive quadrature was the
remaining limitation. Obsolete endpoint-bracketing panels initially regressed
work and were removed (failed source/check preserved). Final two exact M6 edges
pass45/75 compositions instead of65536 exhausted, independent reference differences
9.58e-12/2.28e-12.29 connected3D tests/439823 assertions and8 existing2D/95 pass,
including the unchanged query-count ceiling, undeclared-step rejection, microscopic
fields, original coverage and balanced splits. Three scalar fields/cold-warm reuse
remain consistent. Error indicators/references are numerical, not proven bounds.
[Current exact source/strategy/evidence](Handbook/GOAL3_LOCAL_FEATURE_PRECISION.md).

**All owned jobs terminal.** construction_interior_edge_v1 progresses past both
known edge failures and makes447 global INTERNAL first-row publications/1788 work,
then rejects at the unchanged300s budget.313.21s launcher/300.65s backend, no final
adapted mesh/CFD step/transfer or normalized-budget proof. Input CFD state retained.
The hypothesis that an accepted finer mesh eases integration remains untested.
Do not rerun or raise time/work limits merely to finish. Rank2 geometric edge
preparation147.86s versus9.37–11.88s on others is a new measured hotspot; reuse/
shared-edge/transaction package remains unimplemented. These contended rejected
case timings are not accepted-lifecycle or scaling claims. Stage/resource/source
receipts saved. Maximum rankRSS~725MiB, sum rank peaks2.71GiB upper bound/zero
swaps, host~40GiB available afterward. test_driver current; standalone SU2_CFD
executable older and must be rebuilt before CFD validation.

## Remaining evidence and scope

No accepted M6 grid, complete composed mass, calibrated global error/production
normalization, actual finer-mesh reassessment or lifecycle improvement yet. Need
defensible uncertainty/absolute work allocation before further mechanisms.
Reference sources/failed approaches remain preserved. Final suggestion-by-suggestion
report requested: keep [outcome ledger](Handbook/SUGGESTION_OUTCOME_LEDGER.md) current,
including its other-agent performance accounting; do not replace or discard it.
[Broader query/transaction package](Handbook/POST_COMPLEXITY_QUERY_TRANSACTIONS.md)
is recorded and NOT implemented in this follow-up.

M6 initial120-step RANS/SA state, exact original owner/halo tensors and geometry
are saved. The subsequent construction-only attempts are provisional captured
fields, not final100000-budget normalization or flow/transfer validation.
[Inspectable M6 state](Handbook/GOAL3_ONERA_M6.md),
[exact frozen fields](Handbook/GOAL3_M6_FROZEN_FIELDS.md),
[thin-integration failures](Handbook/GOAL3_M6_THIN_INTEGRATION.md).

30P30N input extracted/validated at integration_evidence/native_3d_bl_v1/
three_element_input_v1 (64540 points/127914 positive triangles); supplied2D CGNS
was one hex layer. No airfoil adaptation/CFD run. The8.63M-hex3D input is outside
native tetrahedral volume support.

Prior-source qualified foundations remain: curved cylinder2877/eight fronts,
right-prism coordinated caps, original sensor transport, retained opposing-wall
removal and actual small RANS/SA N4/M2 current+BDF/SU2/CGNS/reference/restart.
They are NOT automatic current-source proofs. General features/junctions, exterior/
nonprismatic/interacting construction, repeated retained-BL updates, independent
varying full fields, viscous convergence and lifecycle affordability remain open.
Sensor gradation does NOT establish composed gradation; saved violations remain.
[Construction](Handbook/GOAL3_COARSENED_METRIC_FRONTS.md),
[whole caps](Handbook/GOAL3_WHOLE_CAP_CONSTRUCTION.md),
[selection/actual planar checks](Handbook/GOAL3_SELECTION_WORK.md),
[curved RANS](Handbook/GOAL3_CURVED_RANS_RUNTIME.md),
[prior full entry/history](Handbook/HANDOFF_ARCHIVE_2026_10_17.md),
[deferred physics questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).

## Working rules and resources

Follow [AGENTS.md](AGENTS.md): substantial capability-first changes, known hard
case early, scoped/provenance-aware resumable checks, no broad suite per small
edit, no repeated unchanged failures/limit ladders, push reviewable work. Keep
handbooks compact with detailed linked topics. Announce full case folder paths.
One owned heavy job, low priority/thread1/build-j1, locally available MPI<=4.
Check host contention/RAM and live handles. Latest host~40GiB available; recheck.
Exact edge reference~49MiB processRSS, controls~68MiB observed; these are not whole
mesh/aggregate peaks. Root/tmp nearly full: use build-integrated-v2/compile-tmp
on4TB. Do not kill unrelated processes or request cluster jobs overnight.
Goal4 inactive; full physical unsteady3D/general CAD/mixed volumes separate scope.
