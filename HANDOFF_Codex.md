# Codex handoff — start here

Worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-bl`; Goal2 base `c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b`.
**Full Goal3 ACTIVE/incomplete.** [Full scope/gates](integration_evidence/native_3d_bl_v1/GOAL3_REQUIREMENTS.json).
Leave AdapNoExt, foreign ClusterResults and dependencies untouched.

**Current work:** qualify original-source parent support for first-row ranking and
metric measurement, preserving the existing constructor and full-query target.
[Current proof, canonical interpolation issue, checks and remaining gates](Handbook/GOAL3_ORIGINAL_SUPPORT.md).
Direct-cone initialization was fast but failed a complete ordinary cube and is
WITHDRAWN, with all evidence retained. M6 seeding remains a measured bottleneck;
no accepted M6 mesh. [Rejected strategy and complete run](Handbook/GOAL3_M6_DIRECT_INITIALIZATION.md).
Canonical serial/MPI/frozen-query/RANS checks pass; full M6 still rejects during initialization. Baseline attribution confirms the same preexisting generic finer-sensor failure and byte-identical rejected grid. Candidate Engine3D/Field3D and both binaries are restored exactly to their qualified hashes; dependency check clean. All owned jobs are terminal.
Published c70bf64e2b correction stages retain their scoped evidence in
[the normalization handbook](Handbook/GOAL3_FROZEN_NORMALIZATION_STAGES.md).

## Current production and evidence

Bounded complexity initialization uses original sensor-only P1 and actual-query
geometric BL/finer/fade/noise0. Nonplanar faceted walls choose retained sensor+
BL-correction integration; strict planar/analytic/2D behavior remains. Same-config
M6 metric415.97->16.47s, identical coarsest estimate790200.50 +/-27958.45 HEURISTIC
against100000, outside_estimated_bounds/provisional, not normalized/accepted.
[Runtime source, limits and receipts](Handbook/GOAL3_RUNTIME_COMPLEXITY.md).

Canonical owner point lookup, frozen-identity composed-edge reuse, fixed-skin
first-row interior batches and exact shape-dominance screening are qualified in
their saved scope. Last bounded first-row M6 batch preserves exact geometry/skin/
markers/positive volume: metric work~42% lower, elapsed6.05->3.43s /3.58->2.08s
at2/4 ranks. This is64 PRIVATE initializations, not accepted M6/full-event scaling.
[Query/transaction package and all sources/results](Handbook/POST_COMPLEXITY_QUERY_TRANSACTIONS.md).

Full original fields are distinct: hosting-rank RAW9 normalization archive versus
owner-UPPER6 remesher archive. Preserve them and their exact interpolation/core;
never refit from adapted nodes or mix field-specific mass comparisons.
[Exact fields](Handbook/GOAL3_M6_FROZEN_FIELDS.md),
[inspectable initial RANS/SA flow](Handbook/GOAL3_ONERA_M6.md).

## Latest experiments and resulting decision

Whole-field holdouts: two32,768-point RAW9 streams, exact MPI2/4 rows. Pooled total
948103.94 has heuristic indicator15887.91 (1.68%), not a continuum reference.
Eight-block SEM1.54x pointwise; background interval halfwidth5925.71 remains even
with no correction sampling noise. No calibrated default or1e-6 root claim.
[Held-out calibration/work/limits](integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/global_uncertainty_calibration_v1/README.md).

**Reflected face pairs WITHDRAWN:** at8192 points/4096 independent pairs, M6 SEM
improves15–16% on two seeds, planar SEM worsens45%; planar error0.304% misses the
0.05% reference criterion. Native IID SEM is invalid for these dependent samples;
o normalization used it. Actual pair/global assessment is exact MPI2/4 offline.
Executed C++ patch/capsule, outputs, full PDF/source audits and failed-policy
rationale are preserved. All three experimental C++ files are restored to0ad73b;
rebuilt test binary and EVERY prior source/build/binary pin match exactly. Reuse
baseline evidence; no new production API/default/tolerance/sampler/correction.
[Experiment, withdrawal and restoration proof](integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/face_pair_variance_v1/README.md).

**Latest connected implementation:** actual candidate-partition reassessment now
runs before native3D publication, querying the SAME original P1/actual BL; its
parent source index is reused for reader return. MPI1/2/4 controls and curved/RANS
N4M2 lifecycle pass. On an actual accepted planar6->24-cell RANS mesh,8192 samples:
indicator10.63x lower, observed error14.08x lower (.7910%->.05619%), phase.02094s.
Budget1 remains below reference mass3.69416; no normalization correction is claimed.
M6 coarse centroid596350 versus sampled979024+/-101701 HEURISTIC shows substantial
missed BL mass and poor coarse-proposal variance. It does not replace initialization.
[Implementation, exact evidence, work and limits](Handbook/GOAL3_CANDIDATE_REASSESSMENT.md).

**Current correction result:** 8 controlled planar RANS workflows accept151--240
cells under unchanged constraints. Independent final mass19.93549--19.96127 versus
20 (0.194--0.323% discrepancy), with heuristic uncertainty and integration_limited
status. Same-field refinement improves indicator3.07--4.07x; coarsening may worsen
it. No general curved/M6 or calibrated-default claim. Strict2D path retained.

**Next:** difficult complete construction and calibrated global error/allocation,
not another sampler/count/subdivision ladder or local fallback. Preserve the full
scope, conditional M6 budget diagnosis and previous rejected construction evidence.

## Full-goal blockers and qualified scope

No accepted complete M6 mesh, calibrated global error/allocation, general thin/curved
correction beyond controlled planar fields, converged viscous accuracy or complete
lifecycle affordability. Last full M6 construction rejects at unchanged300s,
447 internal publications/1788 work, rank2 geometry147.86s. Keep that failure;
reconsider missing construction capability before another local fallback.
[Constraints, conditional budget bounds and rejected experiments](Handbook/GOAL3_M6_CONSTRAINTS_AND_EXPERIMENTS.md).

Earlier qualified foundations: curved cylinder2877/eight fronts, right-prism whole
caps, opposing-wall removal, original P1 transport and small initialized RANS/SA
N4M2 current+BDF/SA/restart/SU2+CGNS. Preserve each source/binary-qualified scope;
none proves general exterior/nonprismatic/interacting wing construction. Composed
gradation violations44/max24.5184 remain reported, not corrected/certified.
[Construction/caps](Handbook/GOAL3_WHOLE_CAP_CONSTRUCTION.md),
[fronts](Handbook/GOAL3_COARSENED_METRIC_FRONTS.md),
[curved RANS](Handbook/GOAL3_CURVED_RANS_RUNTIME.md).

30P30N input prepared at integration_evidence/native_3d_bl_v1/three_element_input_v1:
64540 points/127914 positive triangles; no CFD config/sensor/adaptation run. Supplied
3D has8.63M hexes, outside native tetrahedral volume support. SST/transition/nonideal
questions remain [saved future work](Handbook/FUTURE_PHYSICS_TRANSFER_QUESTIONS.md).
Final suggestion-by-suggestion report remains outstanding: preserve the other
agent's entries in the [outcome ledger](Handbook/SUGGESTION_OUTCOME_LEDGER.md).
[Earlier full history](Handbook/HANDOFF_ARCHIVE_2026_10_17.md).

## Execution and working rules

All owned jobs terminal. Current source-support package:70 serial cases/14,417
assertions, affected MPI/subset/curved/reader and independent RANS/SA+BDF/CGNS restart
PASS.219,532 unique M6 actual query rows and legacy geometry are bit-identical
between qualified support and full-import paths. Exact source/build/binary restoration
proof allows reuse after the baseline attribution run. Whole M6 still rejects at300s
during initialization after10,080 private parents; generic finer4096-work limitation
is byte-identical on baseline/candidate. No full Goal3/accepted-M6/speedup claim.
One heavy job/nice19/build-j1/thread1/MPI<=4; RAM~38--41GiB, swap unchanged. No cluster
or CFD advancement. All rejected experiments/source/provenance remain.

Follow [AGENTS.md](AGENTS.md): substantial capability-first changes, difficult
case early, source/build/binary/input/helper-aware resumable evidence, no full
regressions per small edit, preserve rejected work and push reviewable checkpoints.
One owned heavy job/nice19/thread1/build-j1/MPI<=4; check actual load/RAM and live
handles before starting another. Announce complete case folders. Use4TB
build-integrated-v2/compile-tmp because root/tmp is nearly full. Leave foreign jobs
untouched. Goal4/full physical unsteady3D/general CAD/mixed volumes remain separate.
