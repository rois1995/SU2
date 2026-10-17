# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`. **Full Goal3 ACTIVE/incomplete.** Goal2 base
`c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
[Full requirements](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
Leave AdapNoExt, foreign ClusterResults, dependencies and foreign jobs untouched.

## Current work and execution

Opt-in budget-aware native3D BL policy now passes controlled complete RANS/SA
current+BDF workflows at MPI1/2/4 (including N4/M2), atomic rejection, independent
geometry/height/q/J/full-edge/transport/conservation checks. Original sensor-only
P1, actual-query BL, finer demands, thickness/support/fade and strict defaults stay.
Two budgets select lambda .875 / 0 with unchanged physical gates. These initialized
workflows do not establish converged viscous accuracy or general curved construction.
[Policy, qualified scope, failures and limitations](Handbook/GOAL3_BUDGET_AWARE_BL.md).

Current qualified source/evidence: this checkpoint (see git HEAD and
`budget_bl_policy_v1/summary.json`). Previous published WIP was
`ca689ee91b8d1ec5f2d7265d042f5c690d4dc3d2`. All owned builds/tests/report checks are
TERMINAL; no live owned heavy job. Source/build/binary/config/input/helper receipts
verified by report.py; preserved failures are not passing campaigns.

V7: primitives/strict controls, uncertain serial/MPI4, and enabled serial/N4M2 full
workflow including SU2/CGNS and actual restart PASS. V4 retains MPI2 scope.
M6 growth2/protect2e-6:100k sampled-profile conflict (260488 +/-10780 at maximum),
1000 explicit protected-lower conflict. MPI2/4 costs/indicators agree2.04e-13 relative.
At300k,lambda.75/version7 remains integration_limited,308423 +/-12078. New enabled
normalization freezes original coarsest scale in6.552s rather than93.477s unresolved
finer-scale search (changed provisional scale, not same-field/lifecycle speedup).
Independent saved-cloud estimates314146 +/-12153 and312539 +/-11852 suggest300k
may still conflict; do not call it attainable. No CFD/M6 construction rerun.
[Final scoped report, inspectable grids, clocks and limitations](integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/budget_bl_policy_v1/README.md).

Next full-goal work: calibrated global uncertainty/absolute-error allocation and an
accepted difficult complete construction under declared physical limits, preserving
same-original-field reassessment/stage atomicity. Do not assume relaxing a profile
or solving integration establishes construction feasibility. Existing M6 strict
source-segment failure remains open. Broader query/transaction package remains.
Do not rerun already-applied implementation mutators or add local fallback/cap
ladders. Policy auto-selection currently native3D only; strict2D unchanged.

## Existing qualified foundations and open failures

Bounded M6 complexity initialization uses retained sensor-plus-BL-correction
integration:415.97->16.47s in saved same-config scope; coarsest estimate790200.50
+/-27958.45 HEURISTIC against100000. Strict planar/analytic/2D integration retained.
No calibrated uncertainty default or rigorous bound/global error allocation claim.
[Runtime complexity](Handbook/GOAL3_RUNTIME_COMPLEXITY.md),
[calibration](integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/global_uncertainty_calibration_v1/README.md).

Original hosting-rank RAW9 normalization and owner-UPPER6 construction fields are
separate immutable references. Newly recovered saved-CFD original families are
also distinct. Never refit from adapted samples or rescale clipped donors.
[Exact fields](Handbook/GOAL3_M6_FROZEN_FIELDS.md).

Progressive FULL measurement and exact declared-midpoint original-P1 queries pass
focused serial/MPI2/4 and independent N4/M2 RANS/SA/BDF/SU2/CGNS/restart. Ordinary
point/full-edge predicates remain strict. Stronger finer-sensor4096-work control
still fails. Actual M6 rejects at84.355s with `Uncovered native 3D source segment
endpoint.` after30 private rounds. No accepted wing or full-event speedup claim.
[Current construction evidence](Handbook/GOAL3_PROGRESSIVE_MEASUREMENT.md),
[constraints/rejected strategies](Handbook/GOAL3_M6_CONSTRAINTS_AND_EXPERIMENTS.md).

Same-original-field candidate reassessment and one bounded normalization correction
accept8 controlled planar RANS workflows (151--240 cells); independent final mass
19.93549--19.96127 against20. Refinement often improves uncertainty; coarsening need
not. Rejected subdivision, root-screening and dependent face-pair experiments are
preserved; none should silently return as production policy.
[Reassessment](Handbook/GOAL3_CANDIDATE_REASSESSMENT.md),
[outcome ledger](Handbook/SUGGESTION_OUTCOME_LEDGER.md).

Canonical owner lookup, frozen-identity edge reuse and bounded interior first-row
batches are qualified in their recorded scope. Broader repair-campaign index reuse,
extended edge reuse and independent transaction batching remain incomplete.
[Query/transaction work and cost limits](Handbook/POST_COMPLEXITY_QUERY_TRANSACTIONS.md).

Earlier foundations: curved cylinder2877/eight fronts, right-prism whole caps,
opposing-wall removal and small initialized RANS/SA current+BDF/restart workflows.
None proves general exterior/nonprismatic/interacting-wing construction.
[Whole caps](Handbook/GOAL3_WHOLE_CAP_CONSTRUCTION.md),
[fronts](Handbook/GOAL3_COARSENED_METRIC_FRONTS.md),
[curved RANS](Handbook/GOAL3_CURVED_RANS_RUNTIME.md).

Full Goal3 blockers: no accepted M6 mesh, calibrated global uncertainty/allocation,
general curved/interacting construction, converged viscous references or affordable
complete adaptation lifecycle. Composed-field gradation remains audited separately,
not certified by sensor gradation. 30P30N2D input is prepared (64540 points/127914
triangles), no CFD/sensor/adaptation yet; supplied3D8.63M hexes exceed tetra support.
[SST/transition/nonideal questions](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
[Earlier history](Handbook/HANDOFF_ARCHIVE_2026_10_17.md).

## Permanent working rules

Follow [AGENTS.md](AGENTS.md): substantial capability-first batches; difficult case
early; provenance-aware resumable checks; preserve failures and all gates; push
reviewable checkpoints. No full regressions per small edit or local-fallback/cap
ladders. One owned heavy job, nice19, build-j1, libraries/thread1, local MPI<=4;
check host contention/RAM/live handles. Announce full testcase folders. Use4TB
`integration_evidence/build-integrated-v2/compile-tmp` while root/tmp is nearly full.
No cluster request/launch, unnecessary CFD rerun or foreign-process termination.
