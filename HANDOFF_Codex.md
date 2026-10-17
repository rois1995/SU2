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

New bounded full-support BL-correction experiment is complete as a REPLAY:
two8192-sample seeds estimate total961498/961691, pooled empirical SE6656;
this is a heuristic, not a proven global error bound. Sensor-only mass remains
152994. Independent finite-square face/edge/corner/fade reference passes MPI1/2/4;
every inside M6 original-P1 sensor density independently agrees<=7.22e-15 and all
M6 rows match MPI2/4. Tiny-cell conditioning is sparse/inefficient (11–22 source
hits/16384); reference errors2–38%, broad indicators cover them but do not qualify
SEM tails. Static proposal/PDF preparation dominates23s versus~3s native kernel;
no total/lifecycle speedup. Rejected drafts/producers retained. New exact point
footprints reuse existing donor service; strict imports unchanged, affected controls
pass. No CFD/accepted mesh or production normalization coupling.
[Method, evidence, limitations and resources](Handbook/GOAL3_SUPPORT_CORRECTION.md).
Retained-field normalization now passes8 planned replay checks: M6 target1M gives
992920/993123 at the same selected scale, heuristic uncertainty~35650;100000 lies
outside diagnostic1/64..64 bracket;1e-6 stops integration_limited. No repeated
source/wall searches during trials; every cached/direct preparation-field query tensor bitwise equal.
Old scalar density drift retained, not a field-gate relaxation. MPI2/4 normalizations
agree; independent planar references and35 controller assertions pass. Production
bounds/gradation/stages and I/O fault qualification remain missing. Reproducible
large export restoration passes. [Scope/work/next coupling](Handbook/GOAL3_RETAINED_NORMALIZATION.md).


Separate strong-BL subset lower estimates364473/362392 already exceed100000 under
conditional99% simultaneous statistical assumptions (not machine certificates).
Refinement cannot lower the continuum demand of the same frozen field. Finite
upper envelopes were too loose; supporting-plane floor inconclusive. Complete
Q4/Q18 audit covers303359 cells/6.674M points but misses thin mass (329 error39.35%
vs tiny rule indicator). Shared experimental one-sided controller/35 assertions
and MPI1/2/4 reductions qualify its scope, not production. Next: port/connect the retained provider to actual bounded/graded production
fields and a calibrated global work policy to explicit uncertainty-aware normalization/stages;
handle unattainable budget visibly, without changing BL/target/gates silently.
[Earlier support bounds and feasibility](Handbook/GOAL3_COMPLEXITY_SUPPORT_BOUNDS.md).

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

Shared strict-edge correction remains qualified in its saved scope: actual M6
edges pass45/75 queries instead of65536 exhausted, reference differences9.58e-12/
2.28e-12;29 affected3D/8 existing2D controls and original scalar cold/warm checks.
All gates unchanged. [Exact strategy/source/evidence](Handbook/GOAL3_LOCAL_FEATURE_PRECISION.md).

**Construction replay terminal.** construction_interior_edge_v1 progresses past both
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

No accepted M6 grid, rigorously qualified complete mass, calibrated global error/production
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
Latest retained replay peak667MiB per process/build2.35GiB, zero swaps;
these are not aggregate or whole-lifecycle peaks. All owned jobs terminal. Root/tmp nearly full: use build-integrated-v2/compile-tmp
on4TB. Do not kill unrelated processes or request cluster jobs overnight.
Goal4 inactive; full physical unsteady3D/general CAD/mixed volumes separate scope.
