## Goal 1 activated: native 3D core — 2026-10-10

User requested final push of 2D work, then activation/pursuit of first3D goal.
2D reuse checkpoint038fdfa5786fb3f6af392d97257bfbb2bb29f036 verified on remote;
all source/docs/review evidence already pushed. Downloaded ClusterResults remain
local user data. Goal tracker now ACTIVE with Goal1+shared contract, no budget.
Implementation branch codex/native-3d-core created from that checkpoint in
SU2_NativeIntegrated; performance branch retained, AdapNoExt untouched.

User suggested /media/rausa/4TB/MeshAdaptation/Scripts. Source review finds useful
3D coupled metric-space coordinate optimization, 2->3 and edge-ring (3..7)
reconnection, interior edge-star insertion, manifold/feasibility closure and
embedding checks. Full controller is fixed-boundary/MMG3D/serial and metric field
uses deformation transport/log interpolation; these do not replace native P1,
actual-query geometric BL, strict guards or adaptable MPI surface transactions.
Reference note and source hashes in integration_evidence/native_3d_core_v1.

First source increment adds isolated native3D predicate/metric-measure kernel
and focused unit tests, leaving native2D source and support checks intact.
Existing strict passive library builds the new unit without fast math/contraction.
Exact binary64 sign uses bounded integer fallback; quality combines regular-tet
mean ratio with normalized minimum vertex Jacobian and separate edge sizes.
Initial 3D shape gates .20/.05 frozen for first campaign; not CFD accuracy claims.
No 3D driver support, cavity edit, surface adaptation or MPI remeshing claimed yet.

Initial host check showed foreign Python/MPI work; heavier builds deferred.
Later MPI run ended, leaving one busy Python process, so a small one-core nice19
kernel check ran in integration_evidence/native_3d_core_v1/geometry_controls_v2.
PASS6Catch cases/40assertions and290 exact Fraction orientation references
(118filtered/172exact). No fullSU2build/CFD/MPI; .65s predicate compile,9.86s
Catch compile, .0045s tests observed, not performance claims. Failed v1 retained:
Python3.8 math.ulp and relative __file__ provenance errors corrected in runner.
Source/artifact pins and commands retained; native3D core README explains limits.
No grids yet: geometry-only tests, not mesh adaptation. Next canonical incidence,
bounded cavities/reconnection, actual-query field/indexing, planar surface
transactions and MPI admission/partition/return. No existing full build folder.
Goal ACTIVE; local-first/resource policy holds. Case folder announced before run.

## Strict frozen matrix582358 reviewed — 2026-10-10

All16 frozen Euler-to-BL/BL-to-Euler cases and8 strict OFF/BOTH pairs PASS,
covering N4/M4 NO and M4/M3/M2 YES. Same tested diagnostic binary5827ffbe...,
MPI1/2/4 focused63 tests/rank PASS. Every accepted mesh/all4 tensor CSVs and
per-rank operation/selection counts are identical within each pair. Reviewer
verifies1259 unique source/tool/data/log hashes, numerical reports, profile/count
closure, matched input/config receipts, modes/binary and serial absolute UTC.
Report/reproducer/JSON:
integration_evidence/native_cluster_reuse_diagnostic_v1/cluster_review_582358/.

Reuse Euler-to-BL total remesh improvement4.45% M4/NO,4.26% M4/YES,0.54% M3,
1.71% M2; privateCPU5.44%/5.10% less at M4. BL-to-Euler request counts unchanged,
evaluations only0.32-0.37% lower; times range2.76% slower to0.58% faster.
One ordered repetition;48 other compute processes recorded onnode-a-ag1:
small timing deltas are not reproducible speedup/regression proof.
BOTH Euler-to-BL M4NO29.575s fastest, weightedM4/M3/M2=33.116/32.410/42.059s;
BL-to-Euler weightedM4=10.292s fastest, M3/M2=12.465/15.317s.
Lower M improves imbalance but not total wall time. Different partition modes
produce different meshes/work; no fixed-work scaling claim. Working partition
setup~0.05-0.11s; private imbalance~2.22 remains. Validation timings include
MPI wait; no exclusive donor-search/interpolation/BL subtiming exists yet.

Global qmin.1841786634, Lmax1.7999995295, BLheightmax4.62974e-12,
transported directional defectmax7.04577e-11. PeakRSS159.84-176.78MiB.
Exports103195291bytes case data,384 verified copies/no hardlinks. Grids in
ClusterResults/reuse_matrix_582358/cases/frozen_*_n4_m*_p*_r1_profile/.
No local solver/build/MPI/heavy geometric audit rerun; source unchanged.
Frozen2D pre-development checkpoint complete; no new unsteady-history,
composed-gradation, converged-CFD or3D certificate. Goal1 may now be activated
by user; no activation this turn, existing tracker remains PAUSED.
Local-first policy from previous turn retained. Planning goals updated to link
completed checkpoint. AdapNoExt/raw downloads/historical failures untouched.

## Native 3D staged goals prepared — 2026-10-10

User requested four staged goals with performance central and then changed the
execution preference to small local cases first, larger campaigns on the cluster.
NATIVE_3D_DEVELOPMENT_GOALS.md contains individually activatable objectives for
tetrahedral MPI/planar surface adaptation, curved geometry/Euler lifecycle,
coarse-grid BL construction/removal, and repeated affordable unsteady 3D.
Each includes correctness, profiling, scaling/memory and evidence gates.
Surface connectivity can adapt from Goal 1; immutable reference geometry does
not mean fixed boundaries. Tetrahedral reconstruction and BL transition repair
are substantial new work; metrics/transfer/output and transaction patterns reuse
existing infrastructure. No universal 3D readiness claim.

Measure complete adaptation cost including metric sampling, partition/migration,
remesh, validation, return/rebuild and solution/history transfer. Bounded query
subtiming separates donor search/interpolation/geometric BL; aggregate counts,
private CPU/wait, instrumentation overhead and accounting closure required.
Proposed actual-CFD target R<=0.20, minimum affordability R<1 on the declared
envelope; these are planning criteria, not observed results. Cadence/accuracy
cannot be weakened to meet them. Local one-heavy-job/initial MPI<=4/-j2/single
library thread and contention checks; cluster repeated/scaling jobs via SGE.
Print full case folders before new cases. No case, solver, build or job started
for this planning request. Pending strict 2D matrix remains the pre-change
regression checkpoint. Goal tracker stays PAUSED; new goals are not activated.
Planning repo/branch SU2_NativeIntegrated / codex/native-unsteady-performance;
use a separate implementation branch when Goal 1 is activated. AdapNoExt,
downloaded results and original evidence remain untouched.

## Diagnostic582344 reviewed; strict same-binary matrix ready — 2026-10-10

User confirms diagnostic downloaded, and asks whether tiny differences matter.
Answer: no demonstrated practical accuracy issue here; stop historical byte-
identity chasing. All7 numerical audits PASS, MPI1/2/4 focused63 tests per rank
PASS. New executable OFF/SCORES/METRIC/BOTH/AUDIT produces byte-identical mesh
and all4 tensor CSVs. Sampled fresh checks540894 metrics/424 scores, zero bit
mismatches. All operation/selection counts identical, including historicalcontrol.
Historical control differs even from new OFF; archived candidate equals new BOTH.
This rules against runtime reuse policies as the explanation on this workload;
arithmetic/source/build context strongly implicated, precise compiler cause not
proved. No production arithmetic fix, tolerance widening or target change.

Lightweight saved-output review verifies1045 unique source/tool/data/log hashes,
receipts/manifests/audit reports and profile/timing/count closure. Report/reproducer
and detailed JSON: integration_evidence/native_cluster_reuse_diagnostic_v1/cluster_review_582344.
Old differences unchanged41points(max3.1447e-15),43tensors(entry-scaled1.0304e-13),
4tensors at unchanged coords. All meshes12235points/24078triangles; qmin.189608758,
Lmax1.799999529, height error3.81295e-12, transported-directional defect7.04577e-11.
Do not treat entry-scaled differences as directional tensor errors. NativeBL
composed-gradation limitation and unsteady/new-source lifecycle checks remain.

Same-binary OFF28.9314s/BOTH27.5512s(-4.77%); privateCPU24.0507/22.7048s(-5.60%).
Score-only27.5769s; metric-only28.8401s. Requests-33.20%, evaluations-2.81%; most
saved requests hits. No other visible compute processes in recorded samples and
UTC verifies serial cases, but no exclusivity/general speedup/scaling claim.
Private imbalance BOTHmax/mean2.2275; expensive new-query work remains.

Next prepared16-case frozen same-binary OFF/BOTH matrix (two directions, N4/M4NO,
M4YES/M3YES/M2YES). Numerical audits, exact accepted output hashes and identical
operation/selection counts mandatory; first failure stops. Original historical
comparison remains strict/unchanged with its failure preserved. Matrix README
and matrix_prepare.py/matrix_run.py/RunReuseMatrixSGE.sh are in same package.
No rebuild needed: known job582344 profile SHA5827ffbe8b7ac9dd0336d000a33a8d649975579f1cd57f623a440a00fb87019e.
Preparation verifies old diagnostic C++/build-source pins, unchanged buildoptions
and that binary; creates build-native/reuse_matrix_checkpoint.json exclusively.
Only orchestration/docs changed this turn. Preserve build-native/reuse_diagnostic_checkpoint.json.
Results: ClusterResults/reuse_matrix_JOB/cases; jobs/JOB_reuse_matrix_launcher.
One ordinary4-slot sequential job, no CFDsteps/ClusterRaw/flowseries; closed compact
exports and copy fallback kept. Package7 fake checks PASS, including16-case flow,
stop-on-difference, work counts and binary/source/build-option drift guards.

No local SU2/C++build/MPI/performance or heavy mesh-audit rerun; no qsub submission.
Goal tracker remains PAUSED. AdapNoExt and original downloads untouched. Pending:
user's new matrix job; broader workload/scaling/interpolation optimization after
its results. Current publication commit will be reported after push verification.

## Frozen reuse diagnostic prepared — 2026-10-10

User asks whether audited meshes are correct and differences are roundoff. Answer:
reviewed meshes pass numerical contracts; root cause remains unproved. Prepared
integration_evidence/native_cluster_reuse_diagnostic_v1 on the existing performance
branch. Seven sequential frozen Euler-to-BL N4/M4 NO runs: validated archived
control, preserved previous candidate, same new executable OFF/SCORES/METRIC/BOTH,
and separate bounded fresh-hit AUDIT. Focused MPI1/2/4 units precede remeshes.
Numerical failures still stop; DIAGNOSTIC_COMPLETE is never byte-identity PASS.
Ordinary comparison identity/operation gate remains strict. Actual-query geometric
BL, sensor-only donors, finer demand/fade, candidates and thresholds remain intact.

Runtime diagnostic environment flags collectively validated; default BOTH/NO.
OFF disables scalar and unchanged-star reuse, metric cache remains original FIFO.
Audit checks eight dynamic metric hits per imported patch (authoritative vertex
seeds excluded), eight scalar hits per score cache and eight complete star checks
per joint configuration. It recomputes with identical ordered coordinates/donors,
reports collective counts/errors and first mismatch values/coordinates per worker,
and returns original cached values. Sampling is bounded, not exhaustive; extra
queries/admission/call context can affect audited outputs and timing. Additional
bounded fresh-patch scratch is admitted. All non-audit modes have the same scratch
reservation. OFF vs historical control and BOTH vs previous candidate expose build/
code-context differences; four flags alone cannot prove compiler causality.

Preserve both actual tested binaries BEFORE rebuilding via new prepare.py
--preserve-previous. It requires control1a092865535502a3cf42a4f4d16c481c3cdc45dc6feff1a9844afa22c181634f
and previous candidate13751201942dfcf9731bc377e45a69b0793ce82f0e2abe0b72edfbcee0902626.
Previous candidate copied exclusively to build-native/reconstruction-previous;
new checkpoint build-native/reuse_diagnostic_checkpoint.json. Build test_driver
on cluster login host, prepare, qsub new RunReuseDiagnosticSGE.sh. No local C++
compilation/CFD/MPI/performance or actual scheduler submission. Four new fake-file
checks pass; original reuse10 and balance8 checks pass. Focused new C++ regressions
await cluster compilation/execution. Existing failed results/review/findings kept.
Absolute UTC case timestamps added to shared compact runner. Download only new
ClusterResults/reuse_diagnostic_JOB and its jobs/JOB_reuse_diagnostic_launcher.
Goal tracker remains PAUSED; separately authorized diagnostic only. AdapNoExt untouched.

## Downloaded reuse results reviewed — 2026-10-10

Implementation d0dc9822fc6c3c1a84fcb1ec0907c869822d3580 is published on
codex/native-unsteady-performance. Correctness582331 PASS(all20 stages); reuse
comparison582333 and582334 FAIL at the first frozen Euler-to-BL N4/M4 NO pair.
Both baseline/candidate remeshes return COMPLETE and all numerical mesh/height/
transported-metric audits pass; selected/attempt/reconstruct/commit/cell/scan
counts and connectivity/markers are identical. Repeatable low-bit differences:
41 point rows(max displacement3.144727846e-15),43 tensors(max normalized entry
change1.030423515e-13),45 total tensor CSV rows. Both jobs reproduce identical
per-role hashes. Four tensors differ at unchanged coordinates, so geometry
motion alone is insufficient. The byte-identity gate remains FAIL/intact;14
later cases in each matrix never ran. This is not an invalid-mesh failure.

Fresh lightweight review verifies1247 unique source/suite/tool/data/log hashes,
job receipts, saved audit reports, timing CSV/log and operation/counter closure.
Evidence/report/reproducer in integration_evidence/native_cluster_reconstruction_reuse_v1/cluster_review_582331_582334.
Original ClusterResults and compiled sources unchanged; no local solver/build/
MPI or heavy numerical mesh-audit rerun. Correctness11 replacements: qmin.51434,
Lmax1.79510, BDF-history defect3.9791e-15, height error4.4409e-16. Native111 tests
per rank, focused61 per rank on MPI1/2/4 pass. Expected guard exit1 is normal. Known composed RANS gradation remains
ratio2.73259/76 residual edges despite sensor-zero residuals; no new certificate.

Requests70.1076M->46.8351M(-33.20%), evaluations15.4894M->15.0544M(-2.81%),
evictions4.4630M->4.0281M(-9.75%). Most removed calls were hits. Private CPU
reduces3.09%/4.78%; remesh30.6468->29.6013s and32.3679->29.3249s. Shared ag1
contention(broad24-core masks, up to64 other visible processes), possible job
overlap not verifiable without absolute timestamps, failed bitwise gate and
missing other modes prevent a general speedup/scaling claim. Private max/mean
still~2.21. Worst46-cell TE attempt evaluations1.3291M->1.2957M, requests
10.5634M->3.3373M, wall2.2618->2.0115s; expensive new-query work remains.

Root cause NOT established. Global fast-math plus changed inline/call/storage/
cache-residency contexts is plausible; an actual cache-hit/recompute discrepancy
must be separated from compiler arithmetic. Next recommended narrow cluster
diagnostic: same binary BOTH/OFF/score-only/eviction-only plus exact cached/fresh
metric and score checks at identical coordinates, and rebuilt OFF vs archived
baseline. Controls are NOT implemented/prepared by this review. Preserve current
candidate13751201942dfcf9731bc377e45a69b0793ce82f0e2abe0b72edfbcee0902626 and
validated control1a092865535502a3cf42a4f4d16c481c3cdc45dc6feff1a9844afa22c181634f
before any rebuild. No speculative production fix or threshold/gate relaxation.
Goal tracker remains PAUSED. All raw failure evidence retained.

## Reconstruction reuse candidate and next cluster checks

Latest user authorized BOTH unchanged-score reuse and improved metric-sample reuse.
Implemented on `codex/native-unsteady-performance` in
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`; AdapNoExt untouched.
Publication SHA will be reported after push/remote verification. Goal tracker
remains PAUSED; this concrete implementation/package work is separately authorized.

SplitSeed/JointPatch now reuse exact ordered-coordinate edge/cell scores through
1024 fixed slots; collisions and moved/reconnected geometry recompute. Joint
search also retains the unchanged current-star score between rejected proposals,
and invalidates it after EVERY accepted edit or different index sequence. Search
order/arithmetic/penalties/thresholds/final geometry checks are unchanged. Added
score scratch is admitted against dependency_bytes (about65KiB); tight budgets
can reject earlier. FieldPatch replaces FIFO-only eviction with a bounded
second-chance hit bit: no allocation/scan per hit, same2048 total entries, seeded
authoritative samples remain pinned. Sensor-only donor/actual-query geometric BL,
finer sensor demands and fade stay intact. No numerical policy or MPI partition
strategy change; no measured speedup yet. Existing per-attempt profiler does not
separate SplitPatch/SplitSeed/JointPatch substage costs or count distinct points.

New meaningful C++ regressions cover exact primitive reuse, changed coordinates,
IDs/order, collisions/target lifetime, hot-point retention, all-hot eviction and
pre-payload score-memory admission. Tests will execute on the cluster. The full
20-stage reusable correctness gate now selects NativeMesh2D/NativeBalanceProfile2D
as well as its previous suites at MPI1/2/4, including actual unsteady Euler/RANS,
CGNS and restart/history checks. No local compilation/SU2/MPI/CFD runs performed.

New package: `integration_evidence/native_cluster_reconstruction_reuse_v1/README.md`.
Preserve job582199's profiled test_driver BEFORE rebuilding BOTH CFD/test binaries;
expected SHA2561a092865535502a3cf42a4f4d16c481c3cdc45dc6feff1a9844afa22c181634f.
New control path build-native/reconstruction-control/test_driver, new checkpoint
build-native/reconstruction_reuse_checkpoint.json. Original b8 control/checkpoints
and historical evidence preserved. Prepare correctness and comparison checkpoints,
run correctness first; only after validation PASS submit new comparison.

Comparison reuses the audited compact runner: focused MPI1/2/4 gate then16
sequential frozen RAE baseline/candidate cases N4/M4 NO and N4/M4/M3/M2 YES on one
shared four-slot SGE allocation. BOTH roles profile. Independent numerical audits,
byte-identical mesh+four tensors AND identical per-rank selected/attempt/reconstruct/
commit/cell/selection-scan counts gate performance. Summaries compare queries,
evaluations, evictions, private CPU/wall/longest and full remesh including working
repartition. Sampling misses are not unique queries or Hessian recalculations.
Baseline/profile labels retain shared runner convention (profile=new candidate).
Download ClusterResults/reconstruction_reuse_JOB_ID and jobs/JOB_ID_reconstruction_reuse_launcher;
actual case folders printed. Minimal verified exports/no new ClusterRaw/timestep
series; hardlink refusal retains safe copy fallback. Original pilot defaults stay
unchanged. Optional REPEATS3 reverses/rotates case order; run no owned jobs concurrently.

Fake-file Python package suites (10 new/8 original checks), reusable checkpoint
self-check, Python AST and shell syntax PASS locally. New C++ compilation, MPI numerical/runtime equivalence and speedup
remain PENDING the user-run cluster jobs. No submission or exclusive allocation
requested. Preserved reviewed job582199 evidence remains the performance baseline.


