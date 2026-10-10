# Current handoff — 2026-10-10

## Native 3D exact-arithmetic cost checkpoint — 2026-10-10

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-core, predecessor f5e94f46624c100520dcf0da59561059d1f7457a.
Goal1 ACTIVE/full scope unchanged. Product/difference skip only redundant zero
limb ranges; exact bit representation/sign/overflow/guards remain unchanged.
No public API/metric target/2D changes. Product-only performance inconclusive(.3%);
combined arithmetic retained after repeated fixed-work evidence.

arithmetic_controls_v2 PASS37cases/2651assertions+290Fraction signs+6272bitwise
product/difference/overflow controls. arithmetic_edge_oracle_v2 PASS104edges,
71covered33rejected147intervals; numerical error bounds unchanged.
arithmetic_optimized_v2 PASS5complete independently audited mesh cycles;
all15SU2+15JSON byte-identical to prior accepted coupled_meshes_v2 grids.

arithmetic_comparison_v2 PASS12sequential runs,3per binary/detail mode;
all meshes/work/residuals identical,182edits/8390evaluations/1556reuses.
OFF median2.577178->2.427019s (5.83%less,70.62->74.99edits/s), CPU2.57->2.42s.
All3candidateOFF times below3baselineOFF; one-core tiny envelope only.
TimerON wider ranges: overhead explicitly measured, primary claims useOFF.
Phaseclosure/CPU/RSS/loads/binary/source pins retained. ARITHMETIC_CHECKPOINT.md
explains strict scope/replay, inconclusive v1 and compact repeat-frame publication.

Selection/source queries still dominant. Next distributed complete-star dependency
and version/atomic admission using existing passive exchange/failure election;
incremental selection is required in that engine, not toy full rescans. General
planar association/coarsening and real composed integration remain pending.
Runtime/MPI/CGNS/CFD/history transfer/scaling unqualified;2D/MMG/fade/noise unchanged.
No newpaperneeded; AdapNoExt/external scripts/foreign processes untouched.

## Native 3D coupled planar boundary and metric checkpoint — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core, predecessor5f46583e5b81b95aa4dba9f538f2ae0a9573bdca.
Goal1 ACTIVE with full original scope. Coupled complete edge-star splits and paired
inverse coarsening now adapt physical triangles with immutable finite facet/marker
authority, exact orientation/skin and manifold links, atomic private output rollback.
Actual-centroid target shape and source-resolved unique-edge gates separate private
progress from final .20/.05/1.8 completion. Repair controls support quality/size
progress for movement/reconnection. Combined-target edge integration rejects until
geometrically resolved, never substitutes sensor length; point composition intact.

Final strict-linked unit validation boundary_metric_controls_v3 PASS37cases /
2651assertions plus290 Fraction signs, with Git absent and listed-source snapshots.
Earlier runner metric recompilation corrected; all earlier evidence retained.
Five full manufactured serial cycles in coupled_meshes_v2 independently PASS:
182 edits, geometry/marker/embedding/P1/metric audit; box/thin1e4/thin1e8/sheared
6->48->6 tets and12->48->12 wall triangles, rotated_spatial6->40->6 and12->40->12.
Accepted grids at integration_evidence/native_3d_core_v1/coupled_meshes_v2/*_adapted.su2.
These are adapted manufactured meshes with no CFD state, not source-only grids.
Coarse event snapshots accepted adapted donors. All15SU2+15JSON unchanged fromv1.

Single contended phase observation totals2.723354s: selection1.247643s/private
metric1.337250s/geometry.064687s/other.073774s.8390edge evaluations/1556reuses.
Source tracing and repeated full toy-mesh scans dominate; prioritize those before
larger runs. Nested query costs are not added to parents. No scaling/speedup claim.
BOUNDARY_METRIC_CHECKPOINT.md gives limits, receipts and sequential/no-Git replay.

Exact on-edge planar proposals only; general facet coarsening/rounded-point
association pending. Thin mapped cases are not Euler-to-BL construction. Local
star authority/IDs/embedding remain preconditions, no distributed completeness.
Runtime still rejects3D; no fullMeson/AD/CFD/MPI/CGNS/transfer/performance envelope.
Next query/selection cost and general planar admissibility, then actual distributed
admission/weighted M<=N/return/transfer and fullGoal1matrix.2D/MMG unchanged.
Local papers remain sufficient; AdapNoExt/external repair project untouched.

## Native 3D original-sensor edge checkpoint — 2026-10-10

On codex/native-3d-core, new bounded TraceSegment uses exact rational original-tet
crossings, ordering/coverage, per-donor endpoint weights and cancellation-free
interval widths. FrozenField::SensorEdgeLength analytically integrates original
P1 SENSOR only. One-donor contained edges bypass rational tracing; multi-donor
edges keep the exact path. Thin source strips between3point samples and intervals
whose crossing parameters both round to0.5 are retained without coarse spreading.
Chord norms have magnitude/exact cancellation checks and reject underflow sentinel
substitution. Existing point composition still runs at every actual query.
Sensor audit is explicitly insufficient for composed BL/target acceptance.

Final PASS28cases/2373assertions+290 Fraction signs in native_3d_core_v1/edge_controls_v4.
Independent edge_oracle_v2 PASS104edges/37groups,71covered33rejected147intervals;
lengtherror4.1162e-17, weighterror4.8867e-20, widtherror7.1193e-20.60direct intervals.
Three source slab SU2 grids+sensor JSON/edge defects independently audited there.
They are manufactured SOURCE grids, not adapted CFD outputs. Point field_oracle_v5
PASS210queries and output byte-identical to v3. Preserved v1 fixture failure;
v2 has explicit source-provenance caveat, final runners reject in-flight edits.

Separate optional exclusive edge search/trace/integral timers, rejection/work counts
and bounds added. Single contended control observes trace/containment dominating;
no speedup/scaling/affordability claim. One low-priority job at a time under foreign
Python/4rank FSI. No full SU2/MPI/CGNS/AD run. EDGE_CHECKPOINT.md gives full scope,
receipts/replay, performance observations and unchanged original completion contract.
Next actual geometric composition integration and metric-gated private proposals,
then adaptable planar surface and full MPI admission/partition/transfer. Runtime
still rejects3D;2D/MMG/fade/noise unchanged. Goal1 ACTIVE; no additional paper needed.

## Native 3D immutable sensor/query checkpoint — 2026-10-10

Prior goal turn progressed with cavitye7f87ecf4a published. New strict Barycentric
uses exact signs and1e-13 magnitude admission, falling back to bounded exact
integer arithmetic for thin rotated ratio accuracy. Exact tensor admission shared
with Measure. New FrozenField snapshots sensor-only original tetrahedra, sorts
canonical node keys, validates shared values, reuses SU2 ADT for multiple donors
and bypasses an unnecessary singleton tree. Bound256donors/2048FIFO sensor samples.
Every query recomposes at the actual point including cache hits; composed BL
nodal tensors are never interpolated/cached. Actual-query semantics and finer
sensor demand tested with a manufactured thin floor, not a real3D BL provider.
Exact containment only: associated physical roundoff/extension remains pending.

PASS22cases/2294assertions+290 exact signs in announced
SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/field_controls_v4.
Actual house ADT/base and serial MPI-wrapper compiled/linked, no stubs/full solver.
PASS210 independent Fraction indexed queries in field_oracle_v3,150inside60outside,
aspect1..1e10/scales1e-50..1e50/translations0/1000. Maxweight8.1262e-17,
directional tensor defect1.5378e-16 tolerance1e-12; fine nodalzz1e10..4e10.
Oracle v1/v2/v3 result lines byte-identical. Initial Werror unused-header-parameter
failure retained; runner matches project Wno-unused-parameter. Earlier field
source/test snapshots and failed runner match receipt hashes for replay.

FieldStats records queries/hits/misses/failures, actual interpolation/composition
attempts including rejection, candidates/containment/cache bounds/index bytes and
optional exclusive query child timers under query parent.256 separated donors
produce1candidate/1test. Build/caller-copy scopes documented; memory ceiling and
full MPI/event closure/instrumentation overhead/scaling still pending. One owned
low-priority process at a time under foreign Python/MPI; no full Meson/CFD/MPI/AD
run or speedup claim. FIELD_CHECKPOINT.md is the current detailed scope/replay.

Next actual metric/shape/edge acceptance with source/geometric integration
breakpoints, physical-boundary roundoff/extension and coupled planar surface
operators, then MPI admission/weighted partitions/return/transfer/full validation.
Native runtime support still rejects3D.2D/MMG source/fade/noise untouched.
Goal1 ACTIVE and full original scope unchanged.


## Native 3D private cavity reconstruction — 2026-10-10

Previous goal turn made progress: incidence4be53587cc published. New bounded
private Cone/ValidateFixedInterface support insertion/movement/removal and some
reconnections with unchanged oriented interfaces, exact positive cells, full
sphere/disk links and explicit64/128 source/replacement budgets. Rejection leaves
output unchanged and reports obstructing face/apex IDs. This is geometric private
closure on an embedded accepted source, not metric/embedding/MPI acceptance.
Runtime support still rejects3D; physical planar surfaces remain adaptable in Goal1.

Strong regression reproduced false rounded-volume closure rejection at rotated
1e6/1e8/1e10 aspect (cavity_controls_v2). Preserve failed logs and old source/tests.
Replaced redundant rounded determinant sum with exact oriented-face/coordinate
closure, retaining positivity/topology guards. PASS16cases/163assertions plus290
Fraction signs in cavity_controls_v3. Initial15cases155assertions also retained.

Inspectable initial/private-candidate SU2 meshes+JSON in
SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/cavity_meshes_v2.
Independent Fraction geometry/volume/skin and pairwise tetrahedron SAT PASS8probes.
Five actual constant-tensor controls enforce q>=.20, J>=.05, L<=1.8; qmin.348566,
Jmin.156174, Lmax1.414214. Three very thin geometry-only controls explicitly fail
isotropic shape gates; not accepted metric meshes. All10 earlier v1 SU2 files
byte-identical. CAVITY_CHECKPOINT.md explains scopes/replay/legacy source pins.
One owned small process at a time, nice19/single library thread, under foreign
Python/MPI work; no full build/CFD/MPI. Timings observations only, no scaling claim.

Next immutable sensor-only tetrahedral donor indexing/query composition and metric
acceptance; then richer operators/targeted growth/physical surfaces and distributed
admission/weighted M<=N partitions/return. All original correctness/performance
requirements remain. Goal1 ACTIVE; AdapNoExt/external repair repo untouched.


## Native 3D local incidence and reference check — 2026-10-10

Geometry checkpoint191f35fd58c0cd816b1e1f786cdbbf5441a3099b published and remote
verified on codex/native-3d-core. User asks independent robust/efficient design,
using repair scripts and existing papers as evidence rather than prescriptions.
Local Loseille2017 cavity, Tsolakis2021 parallel and Galbraith2020 verification
sections reviewed; reading scope/paths/hashes in native_3d_core_v1/papers_reference.json,
design decisions/limits in DESIGN_REFERENCES.md. No new paper requested yet.

New Node/Cell records and immutable sorted face/edge/vertex index reject invalid
identity/coordinates/volume/face cancellation and provide oriented boundary and
face-component diagnostics. Queries cover supplied cells only: no MPI-complete
star, vertex-link or geometric embedding certificate. Build O(k log k), memory
O(k), query O(log k + star size); private bounded-patch use intended. No cavity
replacement or full driver support yet; native2D implementation untouched.

PASS10Catch cases/93assertions plus290 exact Fraction signs in announced folder
SU2_NativeIntegrated/integration_evidence/native_3d_core_v1/topology_controls_v1.
One sequential nice19 single-core check under host load~3.8 and one foreign
Python job; no fullSU2build/CFD/MPI. Sources/tests wired into Meson; full link
pending. No adapted grid produced. Goal1 ACTIVE, remaining full scope unchanged.


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


## Earlier validated checkpoints and evidence

Working repository: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`; AdapNoExt remains untouched. Active branch: `codex/native-unsteady-performance`, fast-forwarded from `codex/native-post-rebase-metric-candidate`. Implementation checkpoint: `e8f0a237362f7fb060eb1272f2d174c5081b944c`; a subsequent handoff-only commit does not change the built source or suite pins. The final publication SHA is reported after remote verification.

**User preference: no further local MPI/CFD correctness or performance runs. Run correctness on the cluster.** The local correctness controller was stopped explicitly at the user request, during its first serial metric suite. No new prediction CFD case ran locally; the subsequent user-run cluster gate 581856 now passes completely. Preserve the stop receipt/logs. Only lightweight source, syntax, manifest and Git checks remain local.

The metric merge checkpoint `5cb87667c20f7c590fab00aa373c7c056a2daca1` has its completed 41-stage validation. The new configurable PREDICT history/filter, acceleration transport and native PREDICT support build successfully (302-step ABI rebuild), and the complete 20-stage native 2D PREDICT correctness gate passes on the cluster in job 581856, independently re-audited from saved output. Native FIXED_POINT remains unsupported. Noise remains zero, and sensor-only donor interpolation plus geometric BL queries, finer demands and fade are preserved. No communicator fix was needed for the earlier MPI timeout; four ranks had been confined to two CPUs.

Reusable user workflow after rebuilding both binaries: run `python3 integration_evidence/native_cluster_metric_comparison_v1/prepare_correctness.py`, then submit `qsub -pe mpi 4 integration_evidence/native_cluster_metric_comparison_v1/RunPredictCorrectnessSGE.sh`. The preparation captures the current source/test/input and binary hashes independently of frozen performance pins; intentional code changes are admitted only through explicit preparation after rebuilding. Each job saves its checkpoint with its results. It runs MPI1/2/4 and native Euler/RANS/CGNS/partial-restart checks sequentially with closed source-pinned inputs. Compute nodes need no Git. Download `ClusterResults/predict_correctness_JOB_ID/` and `ClusterResults/jobs/JOB_ID_predict_launcher/`. Require the validation JSON PASS, then review results before launching the separate serialized old/new performance comparison. See `integration_evidence/native_cluster_metric_comparison_v1/CORRECTNESS_README.md` and its README. Timings are still approximate on shared nodes; no exclusive allocation is requested.

Cluster job 581847 reached the first serial metric suite: 160/161 tests and 420254/420255 assertions passed. The sole failure was the new reference-connectivity assertion: MakeSimplexMesh deliberately supplies clockwise triangles, SU2 reverses them during geometry construction, and the independent reference was not reversed. The test fixture now applies the same geometric orientation correction before cell ordering; exact connectivity and predicted-field checks remain. A lightweight replay of the saved vectors proved all 576 triangles match after correcting 288 clockwise reference triangles. No local solver was run. The subsequent job 581852 passes the MPI1/2/4 rerun; see the newer result below. Evidence is preserved in `integration_evidence/native_post_rebase_metric_v1/cluster_correctness_581847_*`.

Cluster job 581852: all six metric/native MPI1/2/4 suites and all five negative guards PASS. Default-two Euler N4/M3 and four-frame fixture Euler N1/M1 pass independent mesh/original-P1/reference, positivity and both-history conservation audits. Four-frame Euler N1/M1 performs both remeshes successfully, then terminates at step 11 because MAX_TIME was omitted (default 1s) despite TIME_ITER=15 and dt=0.1s. The snapshot assertion fails correctly; original aggregate FAIL is preserved. A separately scoped offline audit of the two completed remeshes passes with qmin~0.583, Lmax<1.8 and history defects<2.77e-15. All logged Euler sensor/composed nodal transport residual counts are zero; this is not a BL gradation certificate. Evidence and report: `integration_evidence/native_post_rebase_metric_v1/cluster_correctness_581852/`; raw grids remain in `ClusterResults/predict_correctness_581852/`.

Correction: all seven reusable configs now set MAX_TIME=100.0 so TIME_ITER governs completion, and cadence assertion errors include expected/actual steps and stop-limit hints. No production code or frozen performance pins changed; source/suite checkpoint must be prepared again after pulling. Remaining full four-frame N1/M1, N2/M1, CGNS N4/M3, straight-wall SA RANS and partial-restart cases need the user-run SGE rerun. No local solver or benchmark was run; saved-output audits only. The matched old/new performance comparison is still pending aggregate correctness PASS.

Cluster job 581856 completes all 20 sequential correctness stages: six MPI1/2/4 metric/native suites, five negative guards, six native default-two/four-frame Euler/CGNS/straight-wall SA RANS cases and three actual adapted-mesh partial restarts. All source/suite identities and stage/checkpoint hashes match. Saved-data audits reproduced all six full cases and all three partial states/fallbacks, plus topology/marker/free-boundary geometry checks on 11 replacements. Across replacements qmin=0.5143406, Lmax=1.7950971, maximum both-history conservation defect=3.9791e-15; RANS relative first-height error<=4.4409e-16. No local MPI/CFD run occurred. Current correctness gate is complete; reusable infrastructure remains available for future changes.

Important remaining limit: all sensor nodal gradation residuals are zero, but the RANS composed sensor-plus-BL diagnostic after prediction reaches max ratio 2.73259 and 76 above-tolerance directed edges (terminal window); first two predictions report 1.28988/2 and 1.94235/54. Combined-field gradation is not certified despite passing mesh quality/length/height/transfer. Do not hide this in PASS or reintroduce coarse-cell wall-tensor spreading/hard-normal reset. Euler composed diagnostics have no residual edges. Native 3D, FIXED_POINT and converged aerodynamic robustness remain outside this gate.

Evidence/report: `integration_evidence/native_post_rebase_metric_v1/cluster_correctness_581856/`. Original inspectable grids/flow/state: `ClusterResults/predict_correctness_581856/`; CGNS grids under `four_cgns_n4m3/`, RANS grids under `plate_sa_four_n4m3/`, both at mesh steps 5 and 10. The performance candidate's 762 source pins and input hashes still match. Next: user-run serialized baseline/candidate N4 pilot, two adaptation events, modes pNO/M4 and pYES/M4/M3/M2, using the existing SubmitMetricComparison.sh; then repeats/longer runs after review. No speedup claim or local performance launch. See comparison README. Chronological pending/failure notes above are superseded by this completed gate.

Cluster submission compatibility fix: the first SubmitMetricComparison.sh call failed with `hold[@]: unbound variable`, because older Bash treats an empty optional array as unset under `set -u`. Required qsub flags now initialize a nonempty qsub_args array; optional dependencies append to it. Strict error handling and all source/input preflight checks remain. Extended fake-qsub regression PASS: 24 submissions without initial HOLD_JID and eight with an explicit initial hold, dependency chaining/order/receipts and invalid-source/path guards. bash -n PASS. No actual scheduler/CFD jobs were launched; local Bash is 5.0.17 and the legacy cluster interpreter was not available. Receipt: `integration_evidence/native_cluster_metric_comparison_v1/submission_bash_compatibility_validation.json`; previous receipt retained. Pull and repeat the same performance pilot command; no binary rebuild or correctness rerun is needed. The specific first-job array expansion error submitted no jobs.

Paired performance pilot 581886–581893 reviewed: baseline download `/media/rausa/4TB/SU2_Versions/SU2_Native_Previous/ClusterResults` has all 16 execution/collection cases PASS; candidate has only four N4/M4/pNO cases, each solver/test PASS then exporter abort on mismatched older ClusterResults/tools. Remaining candidate weighted M4/M3/M2 modes never ran. All 350 selected-data hashes verify; all 20 profile accounting records close. Independent saved-data audits PASS for all eight matched no-repartition cases (version-matching tools, temporary copies; no local CFD/MPI). Inputs and selected build/compiler/MPI/node fields match; conservative flow before first adaptation is bitwise identical in actual RANS/Euler. Candidate actual adaptation lifecycle 57.9356 vs baseline77.3303s RANS (-25.08%), 15.1835 vs17.7928s Euler (-14.67%). Metric ~63% lower; actual remesh +9.2%/+1.0%; outputs/trajectories and external contention differ, single repetition, no verified speedup. Frozen BL-to-Euler grid/commits identical, timing within0.1%; frozen Euler-to-BL grid/commits differ.

Remaining MPI imbalance: first candidate RANS private reconstruction max/mean=2.222 (17.338/7.804s). Validation rank-mean18.747s includes MPI waiting; do not interpret it as pure checks or sum rank maxima. Candidate RANS sensor residuals zero but composed nodal diagnostic reaches ratio318.696/up to15344 edges; combined-field gradation remains uncertified even though mesh/height/transport contracts pass. RANS grids: `ClusterResults/cases/581891_actual_euler_to_bl_n4_m4_pNO_r1/mesh_00200.su2` and `mesh_00400.su2`. Report/evidence: `integration_evidence/native_cluster_metric_comparison_v1/pilot_581886_581893/`.

Collection fix: export matching audit sources into CASE/tools, record their hashes, and write collection manifest after successful tool copy. Older shared bundles and exports remain unchanged. New fake-file regression PASS for version isolation, duplicate/missing data, and no premature COMPLETE manifest when an audit tool is missing. Baseline CPP/collector stays at e6995fbff5; current 762 production source pins unchanged. User should pull candidate and rerun the same serialized eight-job N4/M4/M3/M2 pilot; no rebuild or new CFD correctness run needed. Actual baseline cluster root is `/global-scratch/bulk_pool/arausa/SU2_Native_Previous`. Download ClusterResults only from each checkout and retain its tool layout. No local solver or cluster jobs were launched by Codex.

Completed paired pilot 581943–581950: all eight jobs / 32 execution-and-collection cases PASS. Verified 560 selected files, 160 candidate case-local tools, all 32 profile closures, and all 20 audit tools against baseline e6995fbff5 / previous candidate publication 93e7403366. Inputs/configs match across all 16 pairs; initial conservative flow matches bitwise across all eight actual pairs. Eight no-repartition numerical proofs reuse earlier independent PASS after exact relevant-input/source checks; remaining 24 modes await independent cluster postprocessing. No local CFD/MPI/build/heavy audit ran. Report and complete phase/hash evidence: `integration_evidence/native_cluster_metric_comparison_v1/pilot_581943_581950/`.

Candidate actual RANS remesh seconds M4/NO43.902, M4/YES35.125, M3/YES42.698, M2/YES46.788; all-adaptation58.048/57.337/56.786/60.734s. M4/YES adapted output7.152s offsets its remesh improvement; keep output and CFD repartition/transfer in lifecycle accounting. Euler remesh13.084/12.627/13.861/16.968s. Matched metric time decreases62–72%, but shared-node loads differ greatly (baseline RANS144 other processes on ag1 versus candidate48 on ag2), output/work differs across versions, and only one full matrix is available. Candidate Euler M4/NO CFD95.843s versus prior55.924s despite identical saved numerical audit inputs and binary. Frozen BL-to-Euler outputs are byte-identical for all four matched modes; no consistent kernel gain. Do not claim verified scaling/speedup or an optimum M.

MPI imbalance persists: first actual RANS private max/mean2.208 at M4/NO and2.440 at M4/YES, despite faster weighted remeshing. Sensor gradation residuals are zero in all candidate actual modes; composed RANS diagnostic still reaches ratio324.523 and15950 directed residual edges. Remaining combined-field gradation, weighted-mode numerical review, converged aerodynamics and native3D are not certified. Grids now under `ClusterResults/cases/581948_actual_euler_to_bl_n4_m4_pNO_r1/mesh_00200.su2` / `mesh_00400.su2` and weighted siblings; Euler581950 mesh steps100/200.

Reusable saved-result audit helper: `audit_metric_comparison.py` with `RunSavedMetricAuditSGE.sh` (one worker, sequential Python, version-matching tools, temporary copies, no solver/Git on compute nodes). Submit from candidate checkout: `qsub integration_evidence/native_cluster_metric_comparison_v1/RunSavedMetricAuditSGE.sh /global-scratch/bulk_pool/arausa/SU2_Native_Previous/ClusterResults integration_evidence/native_cluster_metric_comparison_v1/pilot_581943_581950/assessment.json`. Download `ClusterResults/saved_metric_audit_JOB_ID/` and matching `_saved_audit_launcher`; require32 numerical PASS. Fake-file dispatch/isolation/hash/rejection/failure-retention checks and shell syntax pass; actual audit job remains user-owned and unrun. No rebuild needed.

Discussion recommendation: improve measured initial balance before implementing live repartition. Instrument local reconstruction by rank/operation/spatial region and distinguish many movable expensive cavities from a single indivisible one; previous adaptation costs may inform the next initial partition. Mid-remesh repartition is feasible at a completed collective round/phase checkpoint, but elapsed work is known and remaining work only predicted. Prototype fixed active rank count, immutable original donor/geometry target, staged current-cell/directory ownership migration, retained identities/allocation counters and invalidated selection caches. Existing Repartition uses the CFD communicator/default failure/reductions and needs refactoring on M<N; Engine constructor snapshots donors from input, while live Cell.nodal_target already contains composed tensors. Neither recreating Engine from live cells nor calling original WorkWeights on them preserves the intended raw sensor field. No production dynamic-repartition changes or new goal activation were made.

Independent saved-output cluster audit581995 now reviewed PASS for all32 baseline/candidate cases (16frozen +16actual), all48 replacement meshes and64 BDF histories. Launcherexit0/emptyerror, driver/wrapper and assessment SHA match ecd2b0626f publication. Every separate audit report equals its aggregate entry; fresh lightweight review verifies560 data hashes,32 manifests and320 per-case audit-tool hash checks. All numerical results finite. No local solver/MPI/rebuild or numerical mesh-audit rerun occurred. ClusterRaw is not needed; original exports remain unchanged.

Candidate extrema across allmodes: qmin0.1821233856, Lmax1.7999999618, relative first-height error<=4.7083448e-12, frozen transported directional tensor defect<=7.0457733e-11 (tolerance1e-7), CLOSED-policy history conservation residual<=1.2143580e-12 (tolerance1e-10), positive flow states/nonnegative transferred SA. Geometry/reference/topology checks pass. CLOSED conservation includes independent changed-farfield-domain correction; actual tensor transport is not directly exported, while frozen tensors are checked. All24 previously pending weighted-mode numerical audits are now complete; earlier pending notes are historical.

Preserved full aggregate, review hashes/extrema and launcher evidence: `integration_evidence/native_cluster_metric_comparison_v1/cluster_audit_581995/`. Original reports: `ClusterResults/saved_metric_audit_581995/`. Existing simulation grids remain under jobs581948(RANS) /581950(Euler), with allfour partition modes. This audit reads saved output and establishes no additional timing/scaling gain. Composed BL gradation, converged aerodynamics/native3D remain unresolved. Next recommended engineering step is measured local reconstruction cost by rank/operation/spatial region and improved initial weights, potentially learned from preceding adaptation events; defer mid-remesh migration unless persistent movable hotspots justify it. No production code change or new goal activation was made.

Compact reconstruction-balance pilot prepared (2026-10-09): user authorized the
next cluster package and requests minimal saved data. Production now has opt-in
`SU2_NATIVE_BALANCE_PROFILE=YES`, with eight operation totals and at most32
expensive private attempts per worker (including failed reconstruction), wall/
process-CPU cost, cavity size/location and target query counters. It never changes
selection, target or publication decisions. NO/unset does not allocate profile
storage; candidate flag-consistency elections remain and count toward overhead.
Profile output is collectively checked, refuses existing readable filenames, and
is separately timed outside engine-adapt but inside full remesh cost. New unit
checks cover bounded rejected-attempt records and unchanged coupled MPI HEIGHT
reconstruction. C++ compilation/runtime remain pending the user-run cluster job.

Package: `integration_evidence/native_cluster_balance_profile_v1/README.md`.
Use existing candidate cluster checkout on `codex/native-unsteady-performance`:
pull, run `prepare.py --preserve-control` BEFORE rebuilding the test driver,
`./ninja -C build-native -j2 UnitTests/test_driver`, run `prepare.py`, then submit
`qsub integration_evidence/native_cluster_balance_profile_v1/RunBalanceProfileSGE.sh`.
Preserved control hash must be validated b8bb7bc0bf3ba701232513d0c4d0e812e3483cac605da6ecd13e1a4845b7971e;
copy stays inside build-native/balance-control. Do not replace old binary first.
Source pins for the reviewed control remain in this package. No CFD rebuild or
local solver run was performed; source/binary preparation is not a build proof.

One shared-node SGE four-slot allocation runs focused MPI1/2/4 units then16
sequential frozen Euler-to-BL/BL-to-Euler control/profile cases at N4/M4 NO and
N4/M4/M3/M2 YES. Mesh/geometry/height/tensor and timing-accounting audits must PASS;
paired adapted mesh +four transported tensor files must be byte-identical or the
campaign stops. Profiling min/mean/max private costs and committed operation
counts must close against existing logs. This measures private operation imbalance
and instrumentation overhead; it is not live repartition, general scaling, or a
composed-BL-gradation certificate. No per-query clocks are added. Existing actual
campaign still supplies CFD repartition/transfer/output lifecycle evidence.

Download `ClusterResults/balance_profile_JOB_ID/` plus
`ClusterResults/jobs/JOB_ID_balance_profile_launcher/`. Cases under cases/ hold
inspectable grids/tensors and complete frozen audit inputs. Tools stored once per
job; repeated verified content hardlinked within the job (use rsync -aH to retain
savings). No CFD timestep histories or ClusterRaw produced. Task-owned temporary
outputs removed only after verified compact export; failed copying/hash checks
retain the actual working directory. Successful unit-stage incidental geometry
is pruned after receipt/log verification; failures preserved. Previous evidence
untouched. Every working/retained testcase folder is printed by the controller.

Local fake-file package checks PASS: control preservation/checkpoint/path guards,
CSV/pair checks, deduplication/verified cleanup, all16-case orchestration, failed
unit gate and changed-pair early stop. Python AST/bash syntax and diff checks PASS.
No SU2/MPI/qsub/build was executed locally. New C++ tests and real cluster audits
are deliberately pending. Historical performance source pins remain unchanged;
use the new package rather than the old frozen source-pinned submission helper.

Cluster job582197 compatibility failure: scratch refused os.link with EPERM
while staging first frozen input (run.cfg), BEFORE all MPI/SU2 gates. This is an
export/filesystem issue, not a remeshing failure. Both staging and compact export
now use one link-or-exclusive-copy helper; known unsupported/refused/cross-device
link errors fall back to copying, while EEXIST/I/O/disk-full failures propagate.
SHA checks and verification-before-cleanup are preserved. Manifests record storage
method; validation records copy/hardlink counts. Fallback saves only selected files
but cannot provide inode deduplication savings on that filesystem. No C++ changes,
rebuild or original-control replacement needed. Pull, move old checkpoint with
mv -n to balance_profile_checkpoint_582197.json, rerun prepare.py and qsub the same
RunBalanceProfileSGE.sh. Preserve previous failed folders/launcher. README contains
the exact recovery commands. Original preparation receipt remains historical;
new link_fallback_checks.json records this fix's lightweight checks. Tests include
complete16-case simulated pilots with/without hardlinks, guarded existing output
and copy-failure retention; no local solver/MPI/build/qsub run.

Cluster balance pilot582199 reviewed (2026-10-10): allfocused MPI1/2/4 gates
(39 cases per rank, including both new profile tests), all16 frozen remesh/audit
cases and all8 byte-identical grid/tensor pairs PASS. Fresh review verifies324
selected case files, eight fixture files, ten matching audit tools, four archived
package scripts and810 local published compiled-source/Meson pins; logs/CSV
accounting, committed counters, saved geometry/height/tensor audit reports close.
Control stays validated b8bb7bc0...; candidate binary identity recorded. Runtime
profiling gate is now complete for these supported frozen2D cases. This does not
establish independent build provenance, unsteady CFD/transfer or3D/scaling proof.

Main hotspot is costly successful coordinated BULK_SPLIT near trailing edge,
not initial ParMETIS cost. Euler-to-BL M4NO private rank wall13.749/0.490/4.419/
5.464s (max/mean2.280), largest46-cell proposal2.253s wall/2.244sCPU and1,329,104
metric evaluations at x0.98330..1.00273,y-0.001678..0.003848. Retained11 joint
attempts allcommitted, total7.214s (bounded top32, not complete joint census).
WeightedM4 worsens private max/mean2.546 with19.605s busiest rank and8.626s
retained joint cost. JointPatch/SplitSeed repeated score/metric work and bounded
query cache deserve focused aggregate profiling/reuse; counts are not exclusive
interpolation CPU timings. ProcessCPU/private-wall~0.995. Round validation mean
13.675s includes MPI waiting, not pure validation computation. Working partition
only0.048..0.058s Euler-to-BL /0.101..0.107s BL-to-Euler. Preserve successful
reconstruction freedom and allraw-sensor/geometric-BL/height/geometry guards;
blindly limiting expensive proposals risks renewed TE stalls.

Control remesh times M4NO/M4YES/M3YES/M2YES: Euler-to-BL30.159/33.881/32.889/
41.914s; BL-to-Euler10.426/10.560/12.747/15.659s. BL-to-Euler weightedM4 improves
private imbalance1.510->1.225, showing workload-specific weights; its retained
hotspots are ordinary splits <=~7ms. One shared192-core node, fourrank pilot,
48 visible other compute processes. Changes in output/connectivity across modes
exclude fixed-work strong-scaling claims. Paired profile/control change-3.08%
to+1.28%, file output2.0..2.7ms; no isolated small instrumentation-overhead bound.
Optimize coordinated objective/query reuse before live migration, then remeasure
remaining movable imbalance. No further run needed solely to identify this hot
path; repeats required for quantitative performance claims after code changes.

Copy fallback succeeds332files; downloaded entirejob102,152,559bytes (~97.42MiB),
no staging objects/tempcase dirs, noClusterRaw/timestep solution history. Original
exports/failed582197 untouched. Evidence and reproducible lightweight review:
integration_evidence/native_cluster_balance_profile_v1/cluster_review_582199/.
Full logged coordinated phases: M4NO7.326s/45commits, M4YES8.684s/25,
M3YES2.108s/22, M2YES5.410s/23; those wall phases include allround scopes and
must not be equated with retained top32 private samples. BL-to-Euler logs zero
coordinated phase/commits.
Original grids under ClusterResults/balance_profile_582199/cases/.../
native_frozen_adapted.su2 (Euler-to-BL and BL-to-Euler, allworker modes). No local
solver/MPI/build/qsub or numerical mesh-audit recomputation, no production change
and no tracker resume. Previous pending profile-runtime notes are now historical.

The long-running goal tracker remains paused. Cluster runs are user-owned. Do not resume local solver checks or claim prediction validation/performance success before reviewing the cluster results. Earlier notes below are chronological history and may describe superseded pending work.

---

# Codex handoff — native integration

Updated 2026-10-08. Previous native integration and RAE repair goals COMPLETE.
Current branch `codex/native-unsteady-performance` (2D unsteady/cost checkpoint validated);
worktree `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`.


## Current goal: adaptation partition and execution

User activated and explicitly superseded the previous goal on 2026-10-08.
Authoritative objective: NATIVE_ADAPTATION_PARTITION_GOAL.md.
Start with contention-aware old/current fixed-mesh timing controls; then local donor
indexing, adaptation-cost weighted M=N partitioning and explicit M<=N execution.
Keep accepted CFD state/history on N ranks and transfer directly from that donor.
Tracker PAUSED at the user's explicit request while waiting for cluster runs.
Newly authorized cluster analysis and metric integration proceed as concrete tasks;
the tracker has not been recreated or marked complete. No budget was requested.
Fixed-mesh CFD controls started sequentially under native_unsteady_performance_v2:
cfd_control_euler_from_bl_{old_bare,current_timed,current_bare,old_timed}_v1.
60 BDF2 steps, 30 inner iterations, same 13386-point BL mesh, adaptation and metric
OFF. Source-pinned old/current binaries; 15-second quiet launch gate requires no
CFD/compiler job, >=5 available CPU equivalents on the 8-core host, CPU PSIavg10<=10.
Record /proc CPU busy fractions, PSI/load and owned rank affinity while running.
All four fixed-mesh controls terminalPASS: old bare54.237, current timed48.144,
current bare55.525, old timed49.690 seconds. Final density/momenta/energy values
are EXACTLY equal across all four, matching input/config hashes. CPU busy fractions
rise~0.83-0.98; CPU PSIavg10 reaches31.94 during controls despite launch gating.
No large old/current regression or GNUtime penalty appears; shared-host variation
prevents attributing historical/full-run phase changes or small timing differences.
Evidence: native_unsteady_performance_v2/fixed_mesh_cost_controls_v1.json.

Local donor index implemented using existing CADTElemClass(globalTree=false),
bounding-box LINE records only; exact uninflated box/padded triangle checks and
original cell-ID discovery/canonical patch ordering stay authoritative. Other
numeric builds retain original scan (production Native already requires primal64).
Pre-query IntersectionQueryBound is admitted against the dependency ceiling;
otherwise original allocation-free scan. Candidate arrays enter payload admission.
End-of-remesh candidate/full-scan counters and retained index bytes are reported.
Differential distributed donor-cover check includes translated geometry/reversed IDs.
Build v1 terminalFAIL: missing direct include for existing LINE enum; log/source
preserved. Added option_structure.hpp. Build v2 terminalPASS, archive/hash controller
session8569 terminal0. Appf3a03ef23612ac336c98386cf250626ea6343a82640f677774e714c5ee69811f,
test72116a8b7f5307afd9719f72091a9224557430cff7e4f617ab4e7617b45f6923.
Native donor index core MPI1/2/4 PASSES64cases/rank (session57936 terminal0).
Euler-to-BL index frozen MPI4 pilot terminalPASS53.292s whole test,52.759290s
remeshing;985k candidates vs719m full-scan equivalent,405272 index bytes maxrank.
Independent original-P1/geometric-BL/reference/height/topology audit PASS;
output byte-identical to pre-index pruning-cost-v2 mesh (44811commits unchanged).
Fresh pre-index same-work control terminalPASS58.186s whole test,57.673294kernel;
one pair gives8.52% kernel reduction, provisional pending repeats/host variation.
Private reconstruction max26.733index vs26.695baseline: imbalance persists.
BL-to-Euler index MPI4 pilot terminalPASS16.763s whole test;3.215m candidates
vs1.217b full-scan equivalent,1486072 index bytes maxrank.27814commits unchanged;
mesh byte-identical to pre-index. Independent original-P1/reference/topology audit PASS.
Two fixed-work repeats per binary/workload now COMPLETE (8 cases, allterminal0).
Euler-to-BL baseline kernels57.673294/57.619868, index52.759290/54.940094s:
mean6.5865% reduction. BL-to-Euler baseline40.695734/41.106110, index16.104937/
16.455077s: mean60.1965% reduction. All grids byte-identical per workload; operations,
private target-query counters and input/config/CSV hashes identical. CPU/PSI recorded;
shared-host results, not universal speedups. Summary and runnable checker:
native_adaptation_partition_v1/donor_index_fixed_work_comparison_v1.json and
summarize_donor_index_fixed_work_v1.py. Case folders include input.su2, frozen
sensor CSV, adapted SU2 grid and a labeled copy of the original source flow VTU.
Case-selectable independent auditor: integration_evidence/audit_native_frozen_case.py.
Frozen immutable contention-aware runner: run_native_frozen_contention_control.py.
All campaign sessions terminal0:34554,52092,63085,10923,80880,38500,68887,75915.
Weighted M=N helper and production wiring validated on two frozen workloads; M<N is pending. Implementation:
weighted cell dual graph through existing ParMETIS (N communicator), migrate raw
sensor-bearing cells unchanged BEFORE Engine construction. Work estimates use
separate temporary samples of actual geometric composition: CacheTarget overwrites
nodal_target with composed values, so never snapshot those as original sensor donors.
For M<N add explicit communicator arguments to native passive transport, World and
failure election; keep SU2's CFD communicator N unchanged. Subset failure handling
must return/elect on N safely (current SU2_MPI::Error barrier uses N, so calling it
inside only M participants can deadlock). Existing ReaderSlices::FromDistributed on
all N can redistribute output from first-M nonempty records; keep donor geometry,
solution and all histories on N until direct transfer to the new N-rank geometry.
2026-10-08 working-partition helper checkpoint: CNativePartition2D.hpp/.cpp,
registered in Common/src/adaptation/meson.build. Distributed cell-dual graph built
by edge rendezvous, sparse cell IDs mapped to independent graph ordinals; existing
ParMETIS balances two constraints (bounded static work predictor and cell storage).
Graph-only communicator excludes empty original owners. Single original owner
uses existing serial METIS, no replicated volume mesh in production. Raw full Cell
records migrate before Engine snapshot/cache composition; counts, weights and
unique recipient identities checked before publishing by map swap. Collective
migration admission rejection leaves the entire input unchanged. Graph storage
is O(local cells), outside the migration-specific ceiling; idx_t range checked.
Work predictor uses raw-sensor centroid complexity plus bounded actual-query
quality/length demand and wall-row allowance, never coarse-area integration of
wall tensors. Static predictor is provisional: measured private costs decide
whether it is useful. No reduction in actual RAE imbalance established yet.
Build v1 failed only Meson regeneration (Ninja not on PATH), retained log/source.
Build v2 PASSES after correcting controller environment, -j2, archive762sources;
app hash unchanged f3a03ef... because helper is not yet called by production.
Core MPI1/2/4 PASSES66cases/rank, including sole-owner, empty-rank and interleaved
ownership, exact complete wire-record preservation, balanced predicted work/storage
and one-rank budget rejection. Hidden malformed/nonmanifold graph check PASSES
expected collective diagnostic atMPI2/4, no deadlock. Evidence folders:
native_working_partition_build_v{1,2}, native_working_partition_core_mpi_v1,
native_working_partition_invalid_graph_v1. All owned job sessions terminal.
Next: opt-in ADAP_NATIVE_REPARTITION production config at M=N; export final reader
point tensors outside frozen remeshing timer and independently audit their
original-sensor-plus-actual-query-BL residuals. Frozen RAE comparisons include full
weight estimation/graph/partition/migration overhead; ownership changes work,
so these are whole-workload comparisons, not fixed-work speedups.
Weighted M=N production config draft is now ADAP_NATIVE_REPARTITION= NO by
default (native backend only). YES uses the helper before Engine construction;
weight/dual-graph/ParMETIS/migration timers and rank load tables are printed.
Protected first-row cells omit the full wall-row construction allowance.
Build v3 terminalPASS after full config rebuild (~305 targets, -j2, CPU PSI
recorded); appde0d98e736bebd4e66f1a0d5340e90f64f25878dbfc5ad15e42ad2d8a374a5fc.
Final-source core v2 terminalPASS66cases/rank atMPI1/2/4. New frozen helper exports
native_frozen_target_rank_*.csv after kernel timer/HWM; independent auditor now
supports exact frozen sensor CSV plus every final reader tensor. Relative
DIRECTIONAL tensor error is checked by whitening with the expected metric, so a
large BL eigenvalue cannot conceal a defect in the weak direction. Tolerance1e-7;
small runnable anisotropic weak-direction math check PASS. Actual transported-tensor audit now PASS on both weighted/default frozen workloads;
this does not establish final-source unsteady/restart residuals.
First weighted frozen Euler-to-BL attempt v1 TERMINALFAIL before partition/remesh,
0.3665s: work sampler omitted physical-component associations, so an ulp-rounded
wall-edge midpoint could not use the existing numerical containment envelope.
This is reproduced by the existing FieldPatch roundoff regression. Added the
same association loop as Engine initialization, with a focused WorkWeights
regression; NO containment limits, geometry or metric semantics relaxed.
Build v4 terminalPASS: app71f81287fefdc7dbb56980a5a3ad69f86400d21d4e347ea2f6481985f8b6ebba;
core v3 PASS67cases/rank MPI1/2/4, including rounded-edge work sampler.
Weighted Euler-to-BL v2 and same-source default control PASS remeshing and
independent grid/reference/first-height/final-tensor audits. Whole-process times
39.772/39.869s: no meaningful speedup in this matched pair, despite predicted
work balanced from15270/5303/24620/20617 to16906/15684/16937/16283.
Weighted/private max21.166s vs default22.278s; substantial actual imbalance remains.
Weighted commits49616 vs44811, so not fixed-work acceleration.
Weighted BL-to-Euler v1 and default control PASS; whole-process10.624/11.084s,
a small difference that needs repeats. Same-source summary:
native_adaptation_partition_v1/weighted_same_source_comparison_v1.json.
Transported metric audit found and fixed an independent Python projection bug:
length**2 reconstructed a rounded squared norm and selected the wrong adjacent
edge at an EXACT original wall vertex. Native uses direct dot(delta,delta).
Auditor now matches that projection; unchanged 1e-7 directional tensor tolerance.
Failed checker sources/report and diagnosis retained under Euler-to-BL weighted v2
rejected_tensor_auditor_sources_v1. Corrected audit_v2 PASS, maximum directional
defect7.091e-11. A regression retains the unequal-edge exact-endpoint tie case.
Independent runner supports --config/--ranks and copies labeled original flow VTU.
Subset wrapper/config originated in /tmp/native_subset_wrapper_draft_v1;
the applied/live-build state below supersedes this initial draft checkpoint.
Weighted production checkpoint committed locally:24558223c6. No push performed.
Reduced-rank implementation applied from /tmp/native_subset_transport_draft_v2
and /tmp/native_subset_wrapper_draft_v1, original/draft SHA verified first.
ADAP_NATIVE_RANKS=0 keeps all CFD ranks; M<N automatically weights/migrates raw
working cells to first M ranks. Explicit native passive communicator parameters,
World-local rank/size/election, M engine/diagnostics, sleeping MPI-3 N Ibarrier wait,
N ReaderSlices/Bindings/rejected writer return. Global SU2 CFD communicator remains
N; direct CFD/current/history solution transfer follows the existing driver.
Only collectively elected M failures unwind to a common N election; unexpected
rank-local failures retain fail-fast SU2 termination. MPI-2 fallback can spin.
Added scoped transaction/budget/failure regression and full backend affine
barycentric/conservative replacement tests for default, weighted N, M1/M2.
Rejected output/state-preservation regression now exercises M1/M2; config rejects
workers>N. Build native_subset_execution_build_v1 terminalPASS, app
dfe3b1f81368406829964ae5549a596062191964716e0e62fd8f05921176b529.
Core v1 stopped MPI1: test generator selected M1 twice while SAVE_AUDIT retained
the first rejected candidate; writer correctly refused overwrite. Failed case
retained, no native operator failure or CFD state loss established. Corrected
unique rank modes; build v2 PASS, same CFD app. Core v2 PASS83cases/rank at
MPI1/2/4 (test7e8372eaade722143259aa463e920d6cdc5a1ea856bcb8db36d61048f993001d).
Measured inactive rank maxCPU .02656s during5.18291s wall (tiny-core cases only).
Invalid-worker-count v1 exited .317s without hanging, but MakeConfig muted its
diagnostic; expected-error checker correctly marked FAIL. Replaced that hidden
fixture with unmuted CConfig constructor; build v3 PASS same application hash.
Invalid-worker-count v2 PASS expected M>N diagnostic atMPI4, no deadlock.
Frozen v3 test executable06c8a3ed6bc33cef9f4d08ae76bb86331ce7b077684b9aa3115f056579aa59be
(full762source pins), applicationdfe3b1f81368406829964ae5549a596062191964716e0e62fd8f05921176b529.
First frozen RAE Euler-to-BL N4/M1 launch in
native_frozen_rae_euler_to_bl_workers1_v1/runtime_np4; inspect
run_evidence.json for owned process. TERMINALPASS65.529s whole,63.907541s kernel,
56011commits/11285points; all independent geometry/first-height/P1+BL/final-reader
tensor checks PASS (max directional defect7.091e-11). Idle ranks maxCPU .334714s
for63.832s wall; low CPU use demonstrated, no cost gain versus earlier M4 controls.
N4/M2 Euler-to-BL frozen workers2_v1 terminalPASS45.866s whole,45.372772s kernel,
56392commits, independent full metric/height/reference audit PASS. Same-source
M4 workers4_v1 terminalPASS42.567s whole;49616commits, independent audit PASS.
M4 grid is BYTE-IDENTICAL to pre-subgroup weighted v2 grid. First same-source
Euler-to-BL trio favors M4 (M1/M2/M4 whole65.529/45.866/42.567s), no fixed-work
speedup claim. Comparison: native_adaptation_partition_v1/subset_euler_to_bl_comparison_v1.json.
M3 first frozen trial completed/audited PASS37.816s whole, followed by current-source
exclusive timing and actual N4/M3 vortex/SA lifecycle checks (see latest checkpoint).
Reverse workload, repeats and real unsteady RAE/restart remain pending.
No native/CFD source changes for either fixture correction. Frozen and small-case actual unsteady reduced-rank operation are demonstrated;
real RAE/restart validation and recommended worker-count total-cost improvement
remain pending. Authoritative native
sources stay pinned during build. Original drafts retained. Actual CFD trial
runner now includes CPU busy/PSI/owned-rank affinity before/during runs.
Next: finish frozen same-input M1/M2/N comparisons and independent full-target/height/reference/tensor audits,
actual unsteady/restart/current+history transfer controls and measured complete
costs. Final-source actual unsteady/restart and practical scaling limits remain
pending. No native3D/general affordability claim; goal ACTIVE.
No unrelated heavy job was running at activation; editor activity/CPU pressure
exists and must be measured during controls. One heavy job, MPI<=4, threads1.

## Superseded goal evidence: frozen-target MPI cost and optimization

Superseded as the working objective; its evidence remains available.
Committed checkpoint: aa73b62a773af472815bbc7c8b77187f8e763aee, branch
codex/native-unsteady-performance. Completion index: integration_evidence/
native_unsteady_performance_v1/lifecycle_cost_checkpoint_v3.json (32 actual
completed cases,748 pinned artifacts). Checkpoint document snapshots are beside it.

Next goal is same-frozen-sensor/geometry MPI1/2/4 remeshing scaling, profiling
private interpolation/selection/communication imbalance, then improving the
measured bottleneck with all main metric/geometry/height/rejection gates intact.
Reuse the production backend via opt-in NativeFrozenAirfoil2D in the existing
CNativeAirfoil2D test file; no separate mesher implementation. Fixture CSVs copy
original double donor sensor samples exactly by coordinates. Geometry/BL is
composed by the actual backend. Test-only replicated CSV lookup and artifact
mesh gathering are outside measured remeshing. Different rank counts may still
produce different accepted meshes; do not call these fixed-work efficiencies.

Fixtures: integration_evidence/native_frozen_scaling_v1/{plate_smoke,
rae_euler_to_bl,rae_bl_to_euler}. Small frozen BL smoke passes MPI1/2/4.
Both real frozen campaigns are terminalPASS atMPI4/2/1, timeout300s per job.
Euler-to-BL: integration_evidence/native_frozen_rae_euler_to_bl_v1/runtime_np{1,2,4},
remesh88.957/79.947/51.568s, points11285/11757/12252, commits56011/56028/44811.
BL-to-Euler: native_frozen_rae_bl_to_euler_v1/runtime_np{1,2,4},
remesh62.066/36.112/23.800s, points7795/7778/7964, commits28623/28596/27814.
Independent original-P1/geometric-BL q/length and topology audits PASS all six.
Both MPI4 grids exactly reproduce actual unsteady first meshes (coordinates,
connectivity). Different rank counts perform different work; no fixed-work speedup
claim. Summary: native_frozen_scaling_v1/frozen_scaling_summary_v1.json.

Saved-profile details authoritative v2:8526 native samples with MPI API frames,
8035 PMPI_Allreduce; includes waiting/utilities and truncated stacks. v1 falsely
counted native ReferenceState in ComputeMetric arguments and is retained. New
profile_native_stack_details.py selftest guards path-name/argument-type mistakes.
Preflight consolidation was IMPLEMENTED, passed48corecases/rank atMPI1/2/4
and single-malformed-rank negative controls atMPI1/2/4. Six frozen real trials
produce byte-identical meshes and identical commits/operations/queries/transport
counters to baseline, but show no speed benefit (mostly0.1-2.8%slower). REJECTED
and original World::exchange restored. Evidence and complete trial source retained
in native_frozen_scaling_v1/preflight_rejected_experiment_v1.json; archived app
native_preflight_build_v1, SHA2560ef465018efac316041a7317dc47da05de7d68421fea8f312a219c8f74cbec67.
Linux per-rank peakRSS through remeshing (driver/CSV setup included, artifact
gather excluded) is now recorded by the hidden frozen benchmark. Trial peaks:
RANS~44/53/66MiB at4/2/1, Euler~57/83/139MiB at4/2/1. These are process resident
highwater measurements, not requested-byte admission limits or remesher-only memory.

Standard unordered_map trial passed48corecases/rank atMPI1/2/4 and malformed-peer
control. All six frozen real meshes/counters are byte-identical to baseline.
RANS1/2/4 costs88.003/76.064/48.994s; Euler66.338/36.070/23.626s.
Mixed result preserved in native_frozen_scaling_v1/std_hash_trial_comparison_v1.json.
A repeated immutable serial Euler tree baseline costs65.810s versus62.066s
original, exposing~6% timing variation; no established serial hash regression.
Control: serial_timing_control_v1.json. Standalone real-coordinate warm lookup
probe favors exact-bit hashing but is not a full mesher result.

Exact-bit hash trial passed48corecases/rank atMPI1/2/4 and native gates atall
six frozen workloads, but FAILED strict byte reproduction: RANS276/531/304 lines
and Euler1line per rank differ at roundoff scale. Cause is not established; no
independent-audit reuse or fixed-work speedup claim. REJECTED in favor of the
simpler validated standard hash. Complete source/binaries/grids/logs retained in
native_query_bits_hash_build_v1 and bits_hash_rejected_experiment_v1.json.

Containment pilot passed48corecases/rank atMPI1/2/4. MPI4 frozen pilots
pruning_np4_v1 cost47.946s Euler→BL and21.851s BL→Euler; native gates/operation
counts pass. Coordinates differ at rounding scale, identical to the rejected
exact-bit trial meshes; independent audits and timing repeats are required.
Pilot archive native_query_pruning_build_v1 and pruning_pilot_v1.json retained.

Final candidate BUILT: standard hash plus exact-only donor containment pruning
and whole-private-operation timing. Rank min/mean/max selection, reconstruction,
tracked collectives and largest transaction are reported once per remesh with
three packed end reductions; no per-query clocks. Tracked collective timings
include waiting and exclude validation elections, so are not additive wall cost.
Archive native_query_pruning_cost_build_v2 has app/test and760production pins.
CoreMPI1/2/4 passes63cases per rank including metric composition controls.
Six frozenMPI4/2/1 workloads complete, with independent original-P1/geometric-BL,
height/reference and topology audits PASS. Immutable paired MPI4 controls give
Euler→BL52.134→48.287s (-7.38%), BL→Euler23.882→21.793s (-8.75%).
Serial Euler64.496s sample excluded due possible housekeeping overlap; clean
immutable repeat54.083s. Do not use the excluded sample for timing comparisons.
MPI4 RANS private reconstruction ranges0.618–23.477s per rank, largest private
transaction2.823s; substantial imbalance remains. Costs are not additive phases.
Follow-up controls/audits session99294 terminalPASS; log/tmp/native_final_measurement_followup_v2.log.
Actual CFD session50942 terminalPASS (log/tmp/native_final_cfd_v4.log).
Fresh Euler,SA,SST,CGNS BDF2 cases each finish two adaptations and independent
solution/history/mesh audits PASS. RAE RANS600 and Euler300 also complete two
adaptations and independent frozen-field/geometry/height/state-history audits PASS.
Actual RANS elapsed800.925s: CFD683.475, metric43.7732, remesh72.6062,
replacement0.503051 (transfer0.237451 included), adapted output0.0743186.
Actual Euler elapsed226.030s: CFD160.708, metric5.27348, remesh59.0476,
replacement0.309975 (transfer0.162664 included), adapted output0.0394626.
Both substantially slower than historic checkpoints, including CFD BEFORE adaptation;
cause remains unresolved. RANS adaptation/CFD17.11% is not evidence of software
improvement because CFD became slower; Euler remains40.24%. Controlled kernel
improvements7.38/8.75% remain separately supported. Fixed-mesh controls pending.
Actual cases: integration_evidence/native_unsteady_performance_v2. Fresh SU2 4→2
and CGNS-input restarts are prepared but NOT RUN YET. No code relinks planned.
User resumed on2026-10-08; tracker returned blocked despite current progress.
Work is authorized/resumed and there is no present technical blocker; only user/
system can reset tracker status. Finish required validation before marking complete.
Trial runner now wraps MPI in GNUtime for local-process-tree highwater memory,
including launcher; not aggregate rank memory or remesher admission limits.
One heavy job, ranks<=4, threads1, builds-j2; no relinks while campaigning.
Previous goal superseded by NATIVE_ADAPTATION_PARTITION_GOAL.md; no general affordability or native3D claim.

## Resumed unsteady and performance work — 2026-10-07

The user explicitly resumed the unsteady goal after the metric merge and made
performance essential, including profiling interpolation paths. New separate
branch codex/native-unsteady-performance is based on pushed2abbd11769; completed
branches and AdapNoExt are untouched. Worktree: SU2_NativeIntegrated.
Current evidence/case root: integration_evidence/native_unsteady_performance_v1.
Goal scope: static physical geometry, adaptive surface meshes, primal2Dtriangles,
WINDOW_AVERAGE Euler/RANS, dual-time orders1/2, conservative solution and history
transfer, rejection/restart/SU2/CGNS, MPI1/2/4. Cost must be measured at practical
cadence. Profile native transactions, metric work and interpolation/history;
use existing phase timing and Linuxperf rather than introducing a profiler.
The old paused goal's work is now authorized again; tool status may still show
paused because only the user/system can resume its control state.

Native now admits static 2D WINDOW_AVERAGE with BDF1/2.
The shared driver and existing conservative/barycentric history transfer are reused.
Immutable scope app: native_unsteady_build_v1/SU2_CFD, SHA2565973119ef9aa4120c40af25c2b09e1c4ebf0a696f0d558ed2dbc7214b69a1207.
Six actual vortex_v2 cases (orders1/2, MPI1/2/4) completed two adaptations each.
All independent original-connectivity P1 metric, q/length, current/history
integral and final positivity audits PASS; see independent_unsteady_audit.json
inside each case. Double output is the existing VOLUME_OUTPUT_PRECISION=DOUBLE.
The capcheck reader now reports that precision correctly. Original VTUs are donor
snapshots; rewritten restarts n/n-1 are paired with the next adapted mesh.
NativeSupport2D + DistributedTransfer tests pass at MPI1/2/4 in
native_unsteady_history_v2; initial v1 fixture omitted required TIME_STEP and
failed before native support checks. Failure evidence is preserved.

Performance is NOT met at freq3. MPI4 vortex totals: CFD.093s, metric.052s,
remesh.925s, replace.0198s (transfer.0101s). Native remesh does not scale on this
small case; CFD/metric do. Baseline real RAE MPI4 profile is terminalPASS,
176.610s elapsed, one accepted BL adaptation then CFD. Corrected CPU attribution
in profile_rans_bl_startup_np4/sampling_summary.json:34421samples, native21009,
metric3875, CFD9494, transfer9, setup34. Native13986samples have MPI frames
(66.6% of native), suggesting waiting/imbalance/collectives. Initial root-only
classifier was incomplete and is retained as sampling_summary_root_only_initial.
Do not confuse these CPU samples with wall time. Native transaction/metric work
is the priority; existing wall transfer timers resolve short projections.

RAE MPI1 comparison completed:247.781s versus176.610s MPI4 total; native
phase totals83.717s versus107.281s, with different distributed reconstruction
trajectories (serialjoint46/6.91s, MPI4joint97/64.12s). Native scaling is weak;
CFD benefits from MPI. This supports eventual separate remeshing communicator
work, not a current M-rank capability claim.

RANS time-domain coarse-to-BL SA/SST MPI4 cases plate_{sa,sst}_bdf2_np4_v1
completed two accepted adaptations. Independent geometric BL composition,
frozen sensor, current/history conservation and altitude audits PASS; maximum
height relative error4.45e-16. These are lifecycle controls, not wall-shear/drag
accuracy validation. Six SU2 window-boundary restarts reproduce uninterrupted
results exactly at MPI1/2/4 for both orders. CGNS output plus actual four-rank
CGNS BDF2 restart matches within8.89e-16. Mid-window average persistence and
pre-first-adaptation native-reference checkpoints are not validated.

Resolved-vortex physical-time.9 control: cadence100,300steps,dt.003,1225input
points ->752finalpoints, elapsed3.128s; fixedgrid2.111s. CFD1.388s, metric.103s,
remesh1.162s, replace.0223s (transfer.0123s), output.00476s. Cost narrowly below
CFD with little margin; densityP1-lumpedL2 3.840e-4 adapted vs3.364e-4 fixed,
Linf4.239e-3 vs4.477e-3. No accuracy/convergence certificate. Sparse output
omitted pre-adaptation donor VTUs; new build forces ordinary requested output
at native window ends under WRT_ADAP_MESH, reusing existing writer.

Bounded FIFO query-cache trial preserves original P1/actual geometric BL math
and authoritative unchanged-vertex samples; at most2048entries with conservative
per-entry admission estimate128bytes. Dynamic computed samples may be evicted;
seeded vertex samples never enter the FIFO. MPI field/engine/support/history/
CGNS/rejected-output suite passes42cases per rank at1/2/4 in
native_unsteady_cache_mpi_v2. Immutable trial app native_unsteady_build_v2
SHA2563888c9067df010618b4f045ecb03a0c2d8c3805a925ad3dcf442913e7ea9e587.
Completed case profile_rans_cache_np4_v2: nativecommits/operations/transport match
baseline; adaptedmesh and donor restart are byte-identical. Coordinated repair
64.118s ->26.759s; bulk split87.752s ->50.497s. Privatequeries132585546,
evaluations27775647, dynamicevictions12045836. Final CFD continued and exited0:138.878s versus176.610s baseline. Nativephase
sum107.281s ->69.547s (35.2% reduction); full adaptation108.716s ->70.9245s.
Final solution is byte-identical too; one matched end-to-end reduction21.4%. Validated implementation ready for the separate-branch checkpoint.
Post-cache cadence, all five SA order/MPI variants and BL-to-Euler cases
completed; independent target/history/height audits pass. Barycentric unsteady
case completes;4->2rank window restart differs1.35e-12. SA/SST saved current,
history and continued turbulence scalars are finite and nonnegative. Native
unsupported-window MPI2 exits1 with the intended diagnostic; three native/BL
adapter cases pass at MPI1/2/4. See NATIVE_UNSTEADY_PERFORMANCE.md.
Both actual RAE unsteady cases are terminalPASS with independent original-P1,
reference/geometry/height and positive current/history checks:
rae_rans_window200_np4_v1 (Euler mesh -> BL,12252/13386points) and
rae_euler_from_bl_window100_np4_v1 (accepted RANS BL -> Euler,7964/7233points).
Saved mesh_mach_windows.png previews, actual meshes, VTUs/restarts are in each
folder. Total adaptation-related cost / CFD26.28%RANS,41.09%Euler. RANS first
BL construction alone remains more expensive than its first CFD window. These
are developing-flow controls, not certified temporal/aerodynamic accuracy.
Initial strict whole-domain conservation audit failed because open curved
farfield resampling changes domain area. Default CLOSED sliver-policy correction
using independently measured near-constant farfield state passes unchanged1e-10
gate (RANS policyresidual<5.3e-13). Initial failure/diagnosis preserved; sparse
previous-history donor integral conservation is not independently established.

Shared full BDF2 restart writer now refreshes primitive diagnostics around its
history swap; compact restarts bypass it. Primary transfer/continuation were
already correct, but old previous-history pressure diagnostics disagreed by up to .56%. New Euler/SA/SST MPI4 V3 cases have consistentP/T/Mach/velocity below
4.5e-16, identical meshes and continuedprimary/turbulence fields toV2. The
baseline diagnostic regression failure is retained. V3 4->2rank full checkpoint
restart passes; combinedfinal MPI1/2/4 suite passes45cases/rank. Immutablefinal
app native_unsteady_build_v3, SHA256ac36d778c1286abe67eb2dc775ba0f513479ee3da3e4a112feeeded86460833a.

One heavy job, MPI<=4, threads1, builds-j2. Bounded 2D lifecycle/cost checkpoint
is complete; the larger native unsteady goal continues. Next push
is same-target MPI scaling and communication/imbalance cost, then converged
physical-time/cadence accuracy. Mid-window metric-average persistence and
pre-first-adaptation native-reference checkpoints remain gaps. Do not call
production affordability or 3D support complete. Immutable test-driver archive
deduplication reclaimed 2.035 GiB without removing case/evidence content; see
archive_deduplication_v1.json. See NATIVE_UNSTEADY_PERFORMANCE.md.

## Validated metric integration — 2026-10-07

Branch: `codex/native-metric-integration`, based on native main
`9450c0880e2b2c1c39dfc98bc2c9655844ab8731`, with incoming merge parent
`b14a14ea1dec771834d7d691fca27614605df39d`. The user authorized merging and
pushing to rois1995/SU2. AdapNoExt and codex/native-integrated are untouched.
At merge completion the unsteady goal was paused; it is now explicitly resumed.
The completed merge had no owned jobs; see the current section for live work.

Read METRIC_ROBUSTNESS_INTEGRATION.md for the reconciliation, independent
Hessian/WLS review, residual locations and performance limits. Original P1
donors store sensors only; geometric BL is evaluated at every actual, private
and remote query. Finer sensor requests and main's fade are retained. Noise is
zero; the hard-normal reset is excluded. Shared composition feeds thin-BL
integration and remesher queries; wall tensors are not spread into coarse donors.
The measured coarse BL floor is 6564.71, above the historical 4000/6000 sensor
budgets. Actual RANS explicitly requests composed budgets 10000/12000.

Final app: integration_evidence/native_metric_integration_build_v4/SU2_CFD.
SHA256: 498bb9fa3916635ee9bb9d05a54270ceff2525b30deb887cbc65dc98f7e9f69c.
There are 760 production source pins. Core_v3 has 47 cases per rank,
output_mpi_v3 has 44 and adapter_v4 has 3; all pass at MPI 1/2/4.
Serial Hessian_v3 passes 105 cases, and Hessian_mpi_v2 passes 23 per rank
at MPI 2/4; their Hessian production sources are unchanged. Reuse
build-integrated-v2 with -j2, one heavy job, at most four ranks and library
threads set to one. Never relink immutable archives. Final dry build has no work.

Final evidence root: integration_evidence/native_metric_integration_v4.
Run integration_evidence/check_native_metric_integration.py to recheck current
sources, binaries, MPI groups, actual artifacts and preserved incoming data.
completion_audit.json passes. post_frozen_v1.json is TERMINAL_PASS, nine stages;
exec sessions 55058 and 60405 are terminal with exit zero.
All twelve frozen coarse/fine WLS/QR runs pass with noise zero and unchanged
flow. Maximum MPI sensor relative difference is 1.42e-10, below 1e-9.
Independent Gaussian integration agrees within 0.0194% on the fine grid and
0.0030% on the coarse grid. Frozen nodal sensor fixed points pass, but composed
and quarter-edge P1 sensor transport residuals remain. Actual Euler cycle 0
and RANS cycle 1 hit the 80-sweep sensor cap. Continuous or composed gradation
is not certified. Mesh acceptance gates and sensor/geometry policy are intact.

Actual four-rank folders under integration_evidence/rae2822_transonic_v1:

- metricmerge_euler_cross_seed_v1: two accepted meshes, 5722/7649 points;
  minimum q 0.250666/0.342287, maximum length 1.708282/1.799994.
  Total 148.756 s; CFD 87.703 s, adaptation 42.675 s. Adapted density
  residuals are below -8.
- metricmerge_rans_cross_seed_v2: two accepted meshes, 12399/14234 points;
  minimum q 0.180096/0.265992, maximum length 1.799997/1.789548,
  relative height error at most 2.10e-9. Total 339.040 s; CFD 125.068 s,
  adaptation 196.112 s. The cost objective is not met. RANS has not reached
  configured convergence: density -7.911398/SA -7.336547 at iteration 2000.

Both cases pass independent mesh, flow, reference and height checks. Surfaces
adapt; both transfers have zero inadmissible states and recovery patches, and
CFD resumes after each. GRID_GUIDE.md links the actual meshes, solutions and
previews. RANS v1 was prepared only and was never run.

Incoming findings and v3 pass/rejected reports are unchanged. The 57 raw files
(224.02 MiB) in metric_branch_v3_reference have a SHA/size inventory. Failed
predictor-iterator, restart-filename, synthetic-CFD-fixture, missing-reference,
initial-quadrature and cancelled-campaign evidence remains local; compact
receipts are pinned in preserved_integration_attempts.json. Coarse sidecar
preparation retains original geometry/features verbatim and verifies bindings
for the known original mesh; the missing-reference guard remains enforced.

No 3D, unsteady, CAD, large-rank or converged-force claim is made. Future cost
work should target measured candidate/collective-round costs and repeated
gradation/root trials. Geometry-safe continuous field gradation is a separate
investigation. The later explicit user request above authorizes resuming this goal.

## Artifact cleanup — 2026-10-07

User requested removal of unnecessary/superseded test disk usage.
Completed in integration_evidence:864 generated executable/object files removed,
39.22GiB unique content reclaimed;12035 retained files verified unchanged by
size/mtime/inode. All meshes, solutions/restarts, failure evidence, plots, logs,
source snapshots and runners retained. Current build-integrated-v2 and appv28,
Euler baseline appv20, latest AD archivev14 and recent actual/MPI replay binaries
remain. Old AD build retains setup/compile commands/logs but compiler outputs
are removed, so it requires rebuilding. Older archived executables (including
superseded gzip versions) are removed; their historical test results and hashes
remain valid evidence, but those exact deleted executables are not directly
replayable. Affected manifests explicitly record retention status; original
manifests copied into cleanup_20261007/original_manifests.
Exact paths/counts:integration_evidence/cleanup_20261007/inventory.json;
readable report:integration_evidence/cleanup_20261007/README.md.
AdapNoExt and other agents' work/builds untouched. Keep only current/selected
baseline and required replay executables at future cleanup checkpoints.

### Follow-up cleanup — 2026-10-07

Removed7further superseded executable archives (appv28/v29 and older test drivers),
shared2byte-identical current test-driver copies by hard link, reclaiming4.17GiB.
Total across both cleanups:43.39GiB. Verified14521retained files unchanged.
Current appv30, current build-integrated-v2, comparison appv20 and latest AD
archivev14 remain; every mesh, solution/restart, rejected-grid diagnostic, plot,
log, source snapshot and runner is retained. Current test archive manifest paths
remain replayable and SHA-verified; never build/relink inside immutable archives.
Original manifests for deleted binaries are backed up. Exact paths/bytes:
integration_evidence/cleanup_20261007_followup/inventory.json; readable README.md
in the same folder. Keep only current/selected baseline executables going forward,
and share immutable identical archives instead of repeatedly copying them.

## Final common-source validation — 2026-10-07

Appv31 final campaign TERMINAL_PASS; exec session80005 closed exit0.
ParentPID680570/validationPID689650 are terminal; no owned CFD/test controller.
759production sources and all case/MPI artifacts rechecked by
integration_evidence/check_native_cross_grid_completion_v31.py; proof integration_evidence/
native_cross_grid_v31_case_audit.json. SolverSHA
449bca7c124990a12c0f95f3c4d87ce94e2d100e9148b0b4e5a7e6276a5dbe85,
testSHAeadbcd87fb51c059858153c295af9151e6c66b9bdbdcc3fe5a574ade85868e20.
Build evidence/source snapshots native_cross_grid_build_v31; -j1 build PASS.
Current source removes temporary phase scans/collectives/full replay capture and
unused corner/probe experiments. Normal rejected meshes/location reports remain;
appv30 archives retain removed experimental source. No target or gate relaxed.

Actual final cases under integration_evidence/rae2822_transonic_v1:
- nativefix_euler_rans_seed_v14:MPI4 exit0,187.383s, two accepted meshes;
  final6529points/12728triangles,qmin.182441861462,Lmax1.798842097984.
  One joint surface transaction,.0177985s. Solver106.515s/adapt80.251s.
- nativefix_rans_euler_seed_v23:MPI4 exit0,421.243s, two accepted meshes;
  final18591points/36263triangles,qmin.221503940968,Lmax1.799994857893,
  h0relativeerror3.77165e-12. Solver220.04566s/adapt200.7638s.
  First BL construction107.776s;29joint transactions51.7703s.
All four adapted meshes byte-identical to appv30/v13/v22 accepted controls.
Every saved-flow/geometry/reference/height inspection and independent P1/composite
metric audit PASS. Conservative transfer twice/case,0inadmissible states/0recovery
patches. CFD resumed; Euler adapted density residuals below-8, RANS final
density-7.956698575/SA-7.186480 atITER2000 (not configured convergence).

Final MPI1/2/4 groups, same compiled test binary:
- native_release_core_v1:42cases/rank, full shape/size/boundary rollback gates.
- native_release_output_mpi_v1:44cases/rank, configured CGNS/rejected output/
  default MMG/general MPI interpolation and conservative history transfer.
- native_release_adapter_captured_v1:3cases/rank, eight-cycle actual adapter/
  produced BL, captured two-point cross-owner publication and rejection.
Immutable identical current test archives share one inode; no build/relink there.
Main final runner run_native_release_validation_v1.py, state
native_release_validation_v1.json,9stages exit0. Original incorrectly broad
supplemental filter's serial-projection MPI abort and the cancelled-before-launch
corrected-v2 attempt remain recorded; neither is called a pass.

Current reports:RAE_CROSS_GRID_VALIDATION.md, RAE_CROSS_GRID_COMPLETION_AUDIT.md,
ROBUSTNESS_SCALING_RESULTS.md and GRID_GUIDE.md. RAE repair goal COMPLETE.
Source implementation committed as 73b4bf9434; final runnable evidence recheck
PASS after normalizing tuple/list JSON representation in the checker.
No owned CFD, build or test controller remains. Next authorized milestone:
static 2D native unsteady adaptation, reusing existing MPI history transfer and
time-window driver; implementation and lifecycle validation are still pending.
Static primal2D triangles/original polyline only, up to4ranks/36kaccepted cells;
no3D/unsteady/CAD/large-rank or grid-converged force claim. Cost remains high
(75%/91%of CFD time), and coarse-seed BL startup exceeds its initial CFD cost.

## Historical continuation: compound Euler wall repair (2026-10-07)

Production now has a bounded atomic unconstrained-height surface fallback:
up to8private boundary splits, complete free-apex movement and private flips,
q>=.18, maximum length nonworsening and size deficit nonincreasing (strictly
reduced for size-only repair). Positivity, original physical geometry, artificial
perimeter and participant certificates remain enforced; protected/prescribed
wall layers are excluded. Complete dependency rings are imported/reserved only
for the coordinated fallback. Ordinary Euler imports restored to appv28.

Appv29 SHA82df169f892480f0de334f71fd0072033795f801607b173494a6c8cd1eb1ba9a
was a control with broader ordinary Euler imports:43core cases PASS MPI1/2/4
(native_joint_surface_core_v1), but actual Euler v12 rejected adaptation2,
218.709s:2length cells,0shape,height,geometry failures. qmin.1880064082,
Lmax2.310971419, upper wall near(.6148,.0575). Accepted mesh/solution1 and
rejected2/VTU/failure CSV/location plots retained. Saved-flow inspection PASS0/1.
Do not call this complete. Broad ordinary imports have been reverted.

Reproducible production-kernel archive native_euler_joint_surface_probe_v3:
actual v11 shape patches (48/56cells) finish with4boundary splits,
qmin.182441861462,Lmax1.798842097984; actual v12 length patch (58cells)
finishes with3splits,qmin.249177269209,Lmax1.799775694864. Original frozen
sensor target retained. Standalone diagnostic original-P1 locator; no full
solver/MPI claim from these local probes.

Refined rebuild COMPLETE exit0, log/tmp/native_joint_surface_build_v2.log,-j1.
Appv30 archived, SHA3722ee354b386842b79c0bb409a7275ec413de221dbe70eed928fb55a3a0c723,
759production source hashes rechecked against/tmp/native_joint_surface_source_pin_v2.json.
Sequential validation campaign TERMINAL_PASS, exec session43537 closed exit0;
state integration_evidence/native_joint_surface_validation_v1.json,
runner run_joint_surface_validation_v1.py. Core MPI v2 PASS43cases on1/2/4ranks.
Actual Euler v13 COMPLETE exit0,315.106s,MPI4; independent saved-flow, geometry,
and original-P1 sensor metric audits PASS all cycles. Mesh1 byte-identical to
rejected-control v11:SHAe9d9a90a40e68d4be00113e6e6dd811e5536ba2758390edfb385a357052c0218.
One compound surface transaction adds4boundary points and repairs both old
shape failures in.0194721s; final6529points/12728triangles,qmin.182441861462,
Lmax1.798842097984,all native residuals0. Original AIRFOIL192edges->318->254,
FAR40->60->76; reference deviations<=9.38524e-7, feature retained.
Transfer.112503/.054428s,no inadmissible projected states. CFD resumed and
reached density-8.009458/-8.003233 on meshes1/2; no aerodynamic convergence claim.
Euler solve total142.0534s,adapt172.3432s (contended during run by a foreign
CPU0-pinned solver and compiler; not a clean scaling benchmark).
RANS v22 COMPLETE exit0,490.145s,MPI4. Both meshes and conservative transfers
accepted; saved-flow/geometry/first-height and complete composite target audits
PASS all cycles. Final18591points/36263triangles,qmin.221503940968,
Lmax1.799994857893,height error3.77165e-12; density-7.956698575/SA-7.186480
atITER2000, not configured convergence. Solve278.10677s/adapt211.5452s.
First adaptation accepted,29jointcommits53.1357s,
qmin.200002,Lmax1.79999; mesh1 byte-identical to previous accepted v21:
SHA93d79ecdfce03ce63347e154a8bbe1f563cc008afb3d0b9728b44caebe493213.
Do not restart the campaign or terminate foreign work. Main sequential campaign TERMINAL_PASS, exec session43537 closed exit0.
Adapter/manual CGNS/produced BL/captured two-point MPI group PASS np1/2/4,
native_joint_surface_adapter_captured_v1. Note its filter omitted the distinct
NativeCGNS2D production-loop tag; manual writer coverage is present.
Supplemental configured CGNS/default MMG/rejected-output/AdaptationMPI/
conservative projection+transfer group TERMINAL_FAIL, controller635079 exited,
label native_joint_surface_output_transfer_v1. np1PASS58cases/7218assertions;
np2aborted in a serial geometry projection helper with halo points, np4not run.
The broad filter included serial-only Conservative projection/transfer tests;
split these from the actual MPI tags before retrying. Do not mark the whole
configured CGNS/MMG/MPI group passed. Failure log and exact binary retained.
No owned CFD or validation controller is running.
Case proof:RAE_CROSS_GRID_VALIDATION.md and
integration_evidence/native_cross_grid_v30_case_audit.json.
Remaining: supplemental configured-output/conservative-transfer MPI controls,
review/selective cleanup and commit; no accepted-grid numerical convergence claim.
The main campaign completed core MPI1/2/4, Euler v13 lifecycle/inspection/
independent original-P1 audit, RANS v22 lifecycle/inspection/independent complete
composite audit, then adapter/manual-CGNS/captured two-point MPI1/2/4 checks.
CFD MPI4, timeout700s per case, all thread env1. Completed cases retain unchanged
prior input/config. Goal ACTIVE: supplemental MPI selection must be corrected,
then review/selective source cleanup and commit. Do not relabel the failed
supplemental attempt as passing or rebuild archived binaries in place.

## Active goal: repair both cross-grid RAE2822 adaptation paths

Activated at the user's request on 2026-10-06; goal ACTIVE, unbudgeted.
Work only on codex/native-integrated in SU2_NativeIntegrated; AdapNoExt untouched.
Current changes remain uncommitted. One heavy job at a time, MPI<=4, build-j2;
foreign test_driver PID918696 is no longer live (checked Oct7).

Requested contracts: RANS starts from user Euler seed3592points/6952triangles,
h0=1e-5,growth1.2,thickness.02; Euler starts from triangulated original RANS
seed13937points/27642triangles and has NO BL metric. Adaptive boundaries,
q>=.18,Simpson L<=1.8,original-reference hausd<=1e-6,wall-height relative
error<=1e-8 and admissible conservative solution transfer remain required.

Euler completed both adaptations on appv6,v10 and now COMMON appv20.
Latest casev10 exit0,129.241s,MPI4;10162/12288triangles,
qmin.269974/.206052,Lmax1.79918/1.79959,all residuals0.
Case v3 has independent saved-flow/topology/reference and original-connectivity
P1 metric audits PASS; qmin.244718610215/.223654157706,
Lmax1.799974214812/1.797694270119. Only final CFD density criterion met;
no aerodynamic grid-convergence claim. Later common-source replay still required.

RANS v21/appv28 COMPLETE exit0,415.109s,MPI4, two accepted adaptations
and conservative transfer/resumed CFD. First12615points/24833triangles,
qmin.20000165,Lmax1.79998593,29jointcommits52.2479s,transfer.0715395s.
Second18591points/36263triangles,qmin.221504,Lmax1.79999,transfer.170946s,
all four mesh residuals0; completion predicate stops after2complete sweeps.
Independent saved-flow/topology/reference/first-height inspection PASS0/1/2;
inspection.json,mesh_and_mach.png,surface_cp.png in casev21. Maximum relative
wall-height error3.77e-12. Independent complete sensor+geometric BL audits
PASS both adapted meshes: qmin.200001654519/.221503940968,
Lmax1.799985927001/1.799994857893, no shape/length failures at tolerance1e-8.
Reports independent_composite_metric_00001/00002.json; standalone independent
checker integration_evidence/audit_native_composite_rae.py.
CFD hit ITER2000: intermediate density-6.050995/SA-6.840865,
final density-7.956699/SA-7.186480; no configured-8 convergence claim.
Current binaryv28 SHA653f6697a38b743577a8d2bb291df71f760f997dfee7cf68eda8d9f77772fcf5.
Core42cases PASS np1/2/4 in native_joint_completion_core_v1 (gzip binary archive).
Actual captured two-point joint repair publication and participant rejection
PASS MPI1/2/4 in native_joint_captured_mpi_v1:42->46cells,
qmin.200002,Lmax5.4354 (local progress, not complete adaptation).
Broad NativeRemesher/NativeProducedBL2D adapter and CGNS checks PASS MPI1/2/4
in native_joint_adapter_cgns_v2; controller terminal exit0.
The v1 broad controller failed ENOSPC before its first MPI launch; no test result.

Common-source Euler v11/appv28 is terminal exit1,193.881s,MPI4.
First adaptation accepted5411points/10444triangles, saved-flow/reference
inspection PASS0/1. Second rejected2lower-wall shape cells,0length residuals:
qmin.0891642077,Lmax1.79993084. Centroids(.4918005,-.0498070) and
(.4754246,-.0514559). Adaptive surface, no BL metric. Preserved accepted
mesh/solution1 and mesh_adap_00002_rejected.su2/.vtu, failure CSV and
location plot/JSON. Early complete-sweep stop changes mesh1 versus appv20;
keep the early stop and repair the exposed shape-only wall operator gap.
Independent original-P1 metric audit for Euler v11 remains pending.

User reports insufficient shock adaptation2. Saved-field diagnosis completed:
casev21/SHOCK_ADAPTATION_ASSESSMENT.md and shock_detail.png, JSON measurements,
reproducible analyze_shock.py/provenance. Modest actual shock refinement:
median x span .021405c->.016764c in fixed x.50-.68,y.07-.18; upper region
.052028c->.021795c. Solution1 sensor asks h_x~.019379c at shock on y=.12;
solution2 asks .008138c, but no mesh3 was built. Local independent sensor-only
length audit474mesh2edges,max1.640028<1.8. Sensor allocation reproduces6000:
3144.041(52.4%) beyond1c from(.5,0),1532.116(25.54%) beyond10c. HMAX20 floor
only78.217. Investigate metric demand/recovery and outer allocation; do not
blame cavity reconstruction or BL alone, or claim noise without evidence.
Appv28 source provenance remains pinned for completed Euler/RANS evidence.
Next controlled target experiment: same donor flow/budget/BL contracts,
inspect outer Hessians, compare pressure-only/higher Lp; extra adaptation
would sample the sharper final target but isn't proof of robustness.

v20/appv27 first adaptation accepted, second terminaltimeout124 at302.712s
because unnecessary remaining sweeps continued after q/L completion.
No deadline extension happened. v28 all-contract early stop fixes that cost.
Independent complete composite RANS metric audit, broad adapter/CGNS checks,
and actual multi-point cross-owner regression are now complete.
Production Euler shape-only compound wall repair, common-app Euler/RANS
validation, source cleanup and commit remain.

Private Euler repair evidence: native_euler_joint_surface_probe_v1 contains
self-contained C++ probe, exact saved-flow original-P1 target inputs and
48/56cell rejected patches, source/binary hashes, logs and reproduction commands.
Four coordinated boundary splits with free-apex movement finish both patches
qmin.182441861462,Lmax1.798842097984. A private intermediate decreases shape
quality (.12060->.11270), so ordinary published single-operation guards block
it. This is NOT integrated production code or MPI/full-grid validation.
Next: bounded atomic compound surface proposal, complete apex-star dependency
closure for Euler, private reconnection/movement and final unchanged acceptance
contracts. Do not relax published shape/geometry/length gates.
Goal ACTIVE, not complete; no scaling claim beyond4ranks/36k triangles.
Prior actual RANS v19/appv26 rejected44 length cells before joint integration.
Earlier completed case v7(appv11),exit1,41.017s:
all shape,height,reference checks PASS;139length residuals,qmin.185078,
Lmax15.0981. Its _rejected.su2 and failure CSV are saved. Earlier v6(case)
had2shape+171length failures,12172points/23951triangles.

Root fixes implemented:
- Freeze sensor P1 separately; compose existing geometric BL metric at query
  points using retained original geometry, including remote imported cavities.
  Native CFD Metric_* output now describes SENSOR only. Direct manual-kernel
  target APIs and MMG nodal-BL behavior are retained.
- Guard unrepaired midpoint slivers, allow shape-driven insertion and progressive
  deletion of poor seed rows; preserve Euler apices in boundary edge splitting.
- Retain unchanged vertices' authoritative target samples to avoid shared-point
  ulp differences from rounded donor-edge projections. Exact output consistency
  check remains; MPI regression reproduces the one-ulp cache discrepancy.
- Allow flips which strictly shorten oversized diagonals while preserving the
  shape floor; prioritize length deficits. Reverse long-edge restoration fails.
- v12's strict old-vertex retention experiment worsened RANS (casev8:40shape,
  240length,qmin.0365067,Lmax204.609,34.441s); reverted to tested v11 insertion.
  v13 guards surface REMOVE/REDISTRIBUTE bulk shape and maximum edge length;
  the previous prescribed-layer exception let them regenerate coarse transitions
  during every sweep, even from resolved BL stars. Initial HEIGHT/SPLIT layer
  construction remains allowed. New multi-row BL coarsening regression added.
- BL work allowance uses geometric rows*original wall edges, not coarse nodal
  density weighting; first RANS estimate22312.5triangles versus historical2.8M.

Core v13 all31 cases PASS MPI1/2/4. Diagnostic appv14 RANS casev10
reproduces v9,exit1,47.025s:5shape+134length (union134),qmin.13583,
Lmax92.0367,height/reference PASS. Trace: bulk phase reaches165 length residuals
at sweep2,then stalls near134; largest edge unchanged from sweep1 onward.
Late surface phases create no new deficits. Further sweeps alone are not a fix.
v15 changes bulk size-insertion location to balance Simpson metric lengths
(same principle as existing surface split), instead of Euclidean midpoint.
New graded isotropic fixture tests the actual inserted point and both half-lengths.
Build v15 COMPLETE/pinned; core v15 all32 cases PASS MPI1/2/4.
App SHA00855d9706fdcae82866b7499044e7a8ddb824361f291880d44650595876b68b.
RANS casev11 COMPLETE,rejected,45.430s,MPI4:24176triangles,3shape+109length
(union109),qmin.121522,Lmax19.9974,height/reference PASS. Metric-balanced
insertion reduces the plateau but does not finish the mesh.
Next diagnosis: shape-only movement rejects legal shape/size tradeoffs.
Draft size-aware movement reduces unique oversized-spoke deficits, preserving
q>=.18 and existing per-edge caps; graded fixture reproduces the old blockage.
Folder integration_evidence/rae2822_transonic_v1/nativefix_rans_euler_seed_v11.
Core v16 all33 cases PASS MPI1/2/4. App SHA4b632941df10dbfa1733e06f7f185bcde1f91b9282637ad16794088e538ef940.
RANS casev12 COMPLETE,rejected,52.07s:24478triangles,4shape+66length
(union66),qmin.145823,Lmax10.1556,height/reference PASS. Movement helps,
but 12 residual cells touch first-layer apices; ordinary bulk movement rejects
those entire stars. Draft v17 permits only tangential apex motion, preserving
all incident wall altitudes and old per-edge caps; new coupled regression.
Other 54 residual cells still require bulk repair; no success claim.
Appv17 was NOT used for CFD: core first-rank run failed4cases/8assertions
because the new bulk height check called the reference policy with marker0.
Corrected both zero-marker lookups and per-protected-cell missing-wall guard.
Standalone existing NativeMesh2D tests now PASS,305assertions/18cases.
Build v18 COMPLETE. Core v18 all35 cases PASS MPI1/2/4, including direct
cross-owner tangential apex publication; app SHA096a2d8299fd3eb5567613d19406bf8384a47e9501733b603599dfede7770ea9.
Post-test archive copy hit ENOSPC on /media/rausa/4TB (not a test failure).
Partial corev18 test_driver removed; intact build binary still matches recorded
hash; corev18 archive restored/verified, with explicit archive_recovery record.
Recovered10.44GiB before restoring650MiB test binary. Losslessly gzip-compressed
THIS TASK's old executable
archives only, verifying decompressed SHA256 against existing manifests before
removing original copies. Each archive_compression.json explains restoration.
All grids/solutions/source snapshots/logs preserved; latest appv18 retained.
Compression COMPLETE. RANS casev13 COMPLETE/rejected on appv18,MPI4,43.127s.
Draft v19 adds size-deficit priority to movement selection (matching flip size
repair). Previously healthy oversized cells waited behind ordinary shape
smoothing in the bounded phase. Added functional MPI test: one ordered round
must repair a graded oversized star before smoothing a fitting star.
Core v19 all36 cases PASS MPI1/2/4. App SHAaa5347a97b38b39f758d99082b248089fa61a56121a44283a98c09ba23cdc96b.
Casev14 rejected41.335s,23982triangles,2shape+61length,qmin.154751,Lmax19.8959.
v20 EXPERIMENT changes native pointwise BL target to SENSOR/BL intersection
without the legacy tangential coarsening floor. This STRENGTHENS sensor constraints,
keeps h0/growth/thickness and all acceptance thresholds; it is a target-definition
change, not yet validated. The old floor has a hard band switch hidden by its old
nodal interpolation; native point queries expose it. New flat-wall test exhibits
10,000x jump in default/legacy mode and verifies native-mode intersection has no
such switch, stays SPD, and keeps analytic normal size. MMG/default batch policy
unchanged. Buildv20 COMPLETE,pinned app SHA16dbfa9fc35d3fc3f7300414c01b554bc1f0ff9a4f03dfc49cd2aedf0f38e168.
All44 wider core/legacy cases PASS serial (6811assertions); one legacy-only
fixture fails MPI2 because it assumes33 LOCAL floor points and master stdout
on every rank. Reproduced on prior appv19 test_driver in
native_legacy_floor_np2_baseline; baseline failure is not a new regression.
The MPI-compatible native suite (37cases) PASS MPI1/2/4, with np2/np4 saved
in native_cross_grid_core_v20_mpi. Identical immutable test archives hardlinked
between v20 scopes to recover650MiB; original paths/hashes remain valid.
RANS casev15 COMPLETE/rejected42.896s:24482triangles,shape/height/reference PASS,
42length residuals,qmin.180033,Lmax8.12272. Native pure intersection removes the
remaining shape failure without coarse-seed complexity growth, but remesher
still does not finish. Preserve this target-definition change as EXPERIMENT
until common-source validations finish; no accepted RANS mesh/transfer yet.
Euler casev10 launched on appv20,MPI4,300s timeout,unchanged NO-BL config.
Next investigation: private insertion fans need local reconnection when one
new point cannot satisfy both shape floor and graded size target.
Synthetic probe native_private_flip_probe_v1 found oldq.200149,private fanq.140478,
existing flips repair q.181905 and remove requested oversized diagonal,
L288.913 -> max183.236 (perimeter remains oversized; not a complete mesh).
Archived BEFORE header/predicate/search source and hashes reproduce old rejection.
v21 draft reuses existing flips to reduce private shape deficit before insertion
publication; never restore size-requested edge. Shared RepairShape also replaces
surface's separate private flip loop, allowing tied deficits to improve locally.
New captured-fixture regression added. Standalone all19 NativeMesh2D cases
PASS324assertions. Added direct MPI/imported-composition version of the fixture:
private reconnection must publish across owners, preserve perimeter and retain
new point without restoring requested oversized diagonal. Buildv21 COMPLETE;
common39case suite PASS MPI1/2/4; RANSv16 rejected44length failures.
Next: bounded actual-cavity snapshot/replay, rather than another aggregate-only trial.
Temporary per-phase trace still present; consolidate/remove after diagnosis.
Goal ACTIVE; Euler latest completed casev10 passes on appv20.
Final density residual -7.991470261 reached ITER2000 cap, short of -8;
accepted adaptation/transfer does not establish CFD or aerodynamic grid convergence.

After both paths pass on common sources: inspect actual flows/grids, independent
metric/reference audits, broader NativeRemesher/BL/rejected/CGNS MPI checks,
update reports and commit selected code/evidence. Do not claim goal complete yet.


## Authorized next goal and long-term direction

User instruction on 2026-10-06: after completing the active cross-grid repair,
autonomously select and pursue the next feasible goal. Final target: full native
2D/3D anisotropic UNSTEADY adaptation, MPI-aware and optimized so adaptation does
not cost more than CFD work. Do not replace or prematurely complete the active
repair goal. No new goal object yet (one active goal already exists).

Next feasible goal selected: make validated repeated native 2D adaptation
cost-effective. Profile accepted RAE Euler/RANS workloads, measure adaptation vs
CFD time between triggers with explicit counts/ranks/hardware, fix dominant
remesher/MPI costs and retain quality/geometry/conservative-transfer gates.
Set measurable workload-specific acceptance criteria; do not claim a universal
cost ratio from one run. Use these results to support native 2D unsteady lifecycle
and conservative state/history transfer, followed by genuine 3D cavity/geometry/BL
operators and unsteady integration. 3D and unsteady native support are still absent;
current code supports primal static 2D triangles. Preserve evidence and handoff.

## Current follow-up: RAE2822 transonic Euler/RANS and rejected diagnostics

User mesh integration_evidence/rae2822_transonic_v1/mesh_RAE2822_euler.su2,
SHA940d8aed5d9ee0d6dc0a9f5b43a3973e7c76ed5d9123188c6e81d048cf6b6d26.
Exact no-BL seed3592points/6952triangles shared by both fresh cases.
euler_no_bl_seed_v1 completed on4ranks,exit0,28.372s,two accepted adaptations;
6952 ->8667 ->12788triangles. All3density criteria(log10RMS<=-8), topology,
mesh/restart/VTU scalar pairing, admissible flow, original-polyline checks PASS.
See its inspection.json,RESULTS.md,mesh_and_mach.png,surface_cp.png.
Force changes are substantial; NOT grid-converged aerodynamic validation.

rans_no_bl_seed_v1 explicitly stopped by user during native remeshing: terminal
exit1,851.186s,user_stop.json distinguishes cancellation from native rejection.
Initial coarse RANS solve met density+SA residual criteria atiteration847.
BL h0=1e-5,growth1.2,T=.02 made nodal complexity824836.8507; sensor target4000.
ONE TE node148 at(1,.00017),isotropic M=1e10I,dualarea7.365066e-5,contributes
736506.639 (89.29%). Frozen centroid complexity1220091.959; about2.82million
unit triangles estimated. Native work performed hundreds of thousands of
transaction rounds, not an idle solve. Root investigation: coarse-mesh nodal/P1
representation spreads extreme sharp-corner and wall metrics over donor areas.
Do NOT restart this same thin-BL campaign or silently weaken target/quality.

Rejected diagnostics IMPLEMENTED and VALIDATED. CNativeRemesher exports incomplete
native candidates as _rejected.su2 and _rejected_failures.csv when WRT_ADAP_MESH
isYES; accepted-state publication stays behind the existing COMPLETE guard.
Diagnostic SU2 output is independent of accepted mesh format (CGNS remains supported).
NativeRejectedOutput1/2/4 PASS, independently checked topology and exact failure
coordinate pairing; accepted geometry/flow/reference unchanged. Existing eight-
cycle NativeRemesher accepted-state control passes4ranks,both transfer methods.
Archived diagnostic app and759source byte hashes: native_rejected_build_v1/evidence.json.
Only two production source files changed: CNativeRemesher.cpp/.hpp.

Actual old Euler failure reproduced at euler_rejected_replay_v1,exit1,96.454s;
input/config/history/initial restart/VTU all BYTE-IDENTICAL to euler_native_v2.
Candidate5318points/10256triangles,36bad cells independently confirmed against
frozen donor P1 metric.34upperTE nearx=.99623-.99664,y=.000894-.000982;
2lower surface near(.66673,-.02656),(.69068,-.02313).qmin1.88613895e-10,Lmax1.79947.
Exactpositive/manifold/reference checks pass; separate near-duplicatepoint gate
FAILS(two pairs at~1e-11chord),minimum area1.471e-27. Do not hide this limitation.
SU2 and ParaView rejected files plus CSV/JSON/PNG locations are in that case.
See its RESULTS.md. Specific transaction provenance was not recorded.

BL_COMPLEXITY_DIAGNOSIS.md and bl_complexity_diagnosis_v1.json localize excessive
RANS demand. Reproducible inspector and flat-wall self-check demonstrate P1
representation inflation22.88x on a coarse normal interval. No BL metric remedy
has been implemented. Next implementation choice: composite geometric BL oracle
with frozen CFD sensor,or explicit staged BL construction; preserve finalh0.
Do not rerun stopped RANS unchanged,lower its target silently,or accept bad grids.
No own CFD/build/test jobs remain. Root defect and next work are documented;
this follow-up is NOT a new activated goal.

Old euler_native parser failure and rans_sa_native NOTRUN remain preserved.
Work only on codex/native-integrated,AdapNoExt untouched. One own heavy job,
MPI<=4,OMP/OPENBLAS1; check host handles before any launch. Prior goal complete.

## Completed campaign and next actions

All requested integration/robustness/scaling validation is complete within the
explicit native2D scope. Its runs are terminal; follow-up status is above. Completion proof is in
INTEGRATION_COMPLETION_AUDIT.md and integration_evidence/integration_completion_v1.json.
Do not restart any old supervisor or repeat passed campaigns without a new
change, failure or unresolved question.

Test-only BL-step/NACA-phase changes and the closure helper return fix are
committed as ffb3fb4c46. Current test_driver SHA256:
839340d6f5f05c89072b1efa35831ebbf0e716f74b9a16e0313fbb701304832d.
Normal rebuild native_phase_step_build_v1: two objects+link,exit0,109.385s.
All759 production source files remain unchanged since executed ADv14; final
coverage is executed_source_coverage_v2.json.

The final closure runner PID621669 is gone. Its state is terminal1 because a
Python helper omitted return row after the actual one-rank TWO_PASS rejection
passed. native_closure_failure_classification_v1.json records the bookkeeping
error; remaining2/4 negatives passed independently in native_twopass_rejection_v2.
The original failure and all completed steps remain preserved. All9NACA targets,
phase/field/ownership checks and both partial accepted-mesh contracts PASS in
native_phase_partial_complete_v1.json. General [AdaptationMPI]40cases/rank also
PASS1/2/4 in integrated_general_transfer_v1. No required runtime gates remain.

Future work should start from the measured limits, not from a broad scalability
claim: profile native reference/collision/transaction costs on representative
meshes; improve proven bottlenecks while rerunning the same frozen targets and
accepted-state controls. Native3D/CAD and high-demand thin-BL MPI capacity remain
unproved and require separately scoped work. Do not infer support from this goal.

Every progress update must name the actual **testcase directory**, not just the
repository. For audits name saved-mesh input and report destination. Current
JSON labels and historical PIDs are insufficient without host process checks.

## Checkout and resource invariants

AdapNoExt remains `feat_adap_noExt` at
88828474db587652ee1b0c09010de838ddbcfd06. Its only pre-existing tracked change
is `.gitignore`; FILE SHA256 (not git-diff SHA):
7ccfe4565155e5b9b4c2241148628c35317e27c452345dde976da2ef816c564c.
Initial pin: integration_evidence/main_initial.json. Other workers' worktrees
must remain untouched. Never stage `T externals/{codi,eigen,medi,mel,meson}`;
these are dependency symlinks, not integration edits.

One own heavy job at a time; MPI<=4, build-j2, OMP_NUM_THREADS=1,
OPENBLAS_NUM_THREADS=1. Defer to foreign solver/build jobs. Never kill foreign
PID918696. Host process inspection requires escalated execution; print only
PID/PPID/comm/time/CPU, not full arguments or environment. Do not restart a live
handle, extend an existing timeout, or discard a failed evidence label.

## Reconciliation and supported scope

Native57f0550db4 + Stage G1e39b683ee and its pinned draft + cached CGNS output
e41e7eabc7 are combined. Draft SHA:
ff0a4c9588447bd89964ecae859fe44c465a102423b080619e4cc8018aaa2346.
B0Spike remains separate research. BRANCH_RECONCILIATION.md records dispositions.
Later cached origin/fix_periodic_rotation c39428c1da and origin/pr-images
29c71a64b3 were assessed, not imported: both remeshers and Stage G residual
capture reject periodic boundaries. No network refresh by this agent is claimed.
The latest ref/Stage G/main checks match these assessed pins.

One driver factory selects native, MMG TWO_PASS, or ordinary MMG; MMG remains
default. Native is experimental static single-zone primal-double 2D triangles.
Boundary sampling adapts on the immutable original marker polyline. Native3D,
CAD, mixed volumes, moving/time-domain and native adjoint paths are unsupported.
Complete-status gates protect accepted mesh/flow/reference. Preserve in-memory
boundaries, mixed output order, SU2 17-digit coordinates and CGNS marker descriptors.

## Executed proof and observed limits

- Fresh assertions-enabled primal MPI/MMG/CGNS build; output7 cases/rank,
  native/primal49 cases/rank, native-memory/default-MMG controls and auxiliary58
  cases/rank pass1/2/4. Independent mesh/CGNS output audits84PASS.
- AD halo-seed correction79ce4bc81d: clear only nonowned seeds before recording;
  taped primal halo exchange already carries owned seeds. Primal transfer halo
  semantics stay intact. v14 controls pass3 cases/26 assertions per rank1/2/4,
  ten lifecycles, counters past LIMITER_ITER, exact continuation differences0.
  This unchanged-ownership fixture does not establish sensitivity accuracy.
- Actual main SU2_CFD native production1/2/4 and all9 actual-metric/reference
  audits PASS. All12 binary restart/grid pairs are exact with admissible flow;
  every plotted scalar and coordinate matches restart roundedFloat32. Vector
  fields are outside the scalar-reader check. native_production_complete_v1.json
  supersedes the earlier runtime-only pending flag.
- Fresh smooth and abrupt moving nodal-step BL1/2/4, single/opposing walls and
  barycentric/conservative transfer: all192 independent P1/height/reference
  snapshots PASS. Exact continuous-adjoint and missing-reference negatives
  pass1/2/4, MPIexit1 with exact diagnostics. native_step_support_complete_v1.json.
- Engine pilot12, low-cut larger9, repeated8192-cell27, matchedAR10/100/1000
  controls9 all independently PASS. Matched tests scale geometry/h/tensor;
  they are not curved high-demand AR1000 airfoil proof. Conflicting constant
  metric h0_metric2.5>lengthcap1.8 gives valid incomplete output on1/2/4.
- Repeated8192 medians1/2/4(s): cyclic99.516/95.533/52.513;
  horizontal106.234/52.416/53.982; vertical97.598/51.900/40.365.
  Vertical4 range28.060–50.656 despite identical work/mesh. Shared workstation;
  rank-dependent work prevents identical-work speedup claims.
- Strip capacity: serial16384 timeout241.410s, two-rank16384 timeout241.910s,
  four-rank16384 PASS (11718output,adapt98.625s,whole176.847s), four-rank32768
  timeout241.040s. Larger sizes NOTRUN under predeclared stops. Timeouts cover
  entire job; no timed-out phase, infeasibility or invalid topology established.
- NACA h0=.0002,4k/6k/3k passes1/2/4. Serial4k/12k/3k accepted23368 then40968
  triangles, timed out during next coarsening902.433s. Serialh0=.0001 accepted
  38852 triangles, next remesh timed out902.055s. Both partial target/reference audits now independently PASS.
  h0=5e-5 and higher-rank failed-demand controls NOTRUN under stopping policy.
- Actual AD warm1/2,cold1 and warm/cold4 short lifecycles pass. cold2 diverged;
  static replay on exact saved grid/flow reproduced131 printed trajectories
  within one residual print unit. Classified CFD limit, not successful lifecycle.
  Warm4 final adjoint grows(log10RMS1.54155); cold4 -2.68074, neither converged.
  All four own-descendant interruption checks(primal/recording,1/2) pass with
  collective cleanstop/no-remesh/checkpoints; exact native rejection2 passes.

Memory caps2MiB transactionwork/256donors/128discovery/64cache are not whole-
process bounds. Reference/master-reader boundary isO(B). Encoded traffic includes
self buckets; World counters omit direct CPassiveComm/failure votes. Engine
strip has fixedny4 andB grows withN; no general2D/3D scaling claim follows.

## Actual case files

Start with GRID_GUIDE.md. Actual native CFD input+flow/restart files:
`integration_evidence/integrated_native_production_v2/np{1,2,4}`.
input.su2 pairs with flow_adap_00000.vtu and solution_adap_00000.dat BEFORE first
adaptation. mesh_adap_0000N.su2 pairs with flow/restart of the same index.
Initial flow is computed from freestream in memory, not loaded from restart.
These short solves are not converged aerodynamic validation.

Immediate donor/transferred conserved fields from the new timing fixture:
`integrated_airfoil_phase_timing_v1/audit_np{1,2,4}/native_airfoil_cycle_N_{donor,adapted}_solution.csv`.
These use exact global IDs/coordinates; they are CSV fields, not SU2 restarts.
The older baseline_v8 fixture saved grids/metrics/transfer integrals but no
nodal solution. Actual production outputs address that earlier artifact gap.

Terminal history (preserved, not queued): robustness_chain_v8(0), native_production_chain_v3(0),
integration_capacity_v2(0), goal_remaining_v1(0). goal_runtime_v7(1) preserves
cold2 divergence; its original follow-through/production followers stopped
without jobs. native_production_chain_v2(1) preserves config-parser rejection of
PARAVIEW_BINARY; correct repository token is PARAVIEW. Do not restart old chains.

## Actual cavity replay / corner experiment (active, not validated)

Appv22/RANSv17 reproduces v16 rejection (44length,0shape/height/reference).
Saved 21640-byte bounded witness: native edge941452--1519722,12cells,6donors.
Exact metric reconstruction matches golden samples (44assertions). Existing
single-edge search199fractions plus move/flip reaches only q.140076. Internal
long-edge simultaneous subdivision and64private insertions also FAIL; do not
promote these algorithms. Probe source/logs/hashes retained in
integration_evidence/native_actual_cavity_probe_v1. Production test trimmed to
small hidden exact replay [NativeRAECavityReplay], environment path
NATIVE_RAE_CAVITY_REPLAY, snapshot version1(oldtarget) or2(cornerexperiment).

Next appv23 experiment: strengthen native 2D tangential corner resolution by
sagitta-inspired ht<=sqrt(2*r*hn)/sin(turn/2), smoothly fade extra coefficient
at existing corner support radius. h0/growth/T and final q/L/height/reference
gates unchanged; target definition explicitly STRONGER, not same target.
Legacy batch/MMG defaults unchanged. Build/core/RANS validation pending.
Temporary cavity capture and per-phase trace still need cleanup before commit.

Appv23 core40 PASS MPI1/2/4; legacy7cases3331assertions PASS serial.
RANSv18 FAIL47.781s,25090tri(beforefinalmove),52length,shape/height/reference0,
Lmax247.44. Corner experiment worsens protected surface stalls; NOT ADOPTED.
Native driver reverted to no-floor target without corner regularization.
Optional experimental metric code/test remains temporarily for replay ofv18;
remove before final production commit. Next: dense off-edge point-placement
search in the original actual quadrilateral (v17target), no further CFD yet.

2026-10-07 continuation: revalidated integration HEAD/branch; no live own
solver/build at entry. Previous turn PROGRESS: exact cavity capture/replay,
failed corner target reverted, dense all one-point quad search archived as
native_actual_cavity_probe_v2 (gzip executable restored+SHA verified).
Temporary build Ninja was lost from /tmp; recovered from existing cached wheel
into /tmp/native-build-tools/ninja, no network/install or AdapNoExt edits.
Optimizing adjacent growth proposals still FAILS. Exhaustive subset probe of
12cell cavity (five seed positions, shape optimizer/private flips) also FAILS;
best sampled q.134159. Logs/source preserved in
native_actual_cavity_growth_probe_v1. Next: rank-local complete rejected-state
snapshot, then exact larger-cavity/reconnection replays without repeated CFD.
All diagnostic drafts still require cleanup; no current RANS acceptance.

2026-10-07 exact full-state replay completed: current24757 cells/original6952
donors from four rank-local snapshots in nativefix_rans_euler_seed_v19.
All44 failing cell CENTROIDS are above TE ordinate .00017; some edges straddle
the wake. Lower side has no length-failure centroid. User visual observation
therefore matches residual localization; no claim that whole cells lie above.
Ring0/1/2 imports12/42/94 cells. Larger patches alone still FAIL; bypassing
only kernel32cell cap also fails. Optimizing adjacent growth and retaining
moved insertion-point position (four combinations) all FAIL. Copied probe
and logs/hashes archived in native_actual_state_growth_probe_v1. Passing
31785 assertions establish snapshot parsing/geometry only, not acceptance.
Next diagnostic tests bounded intermediate bulk size-refinement shape loss
with strict size-deficit progress. Final q>=.18 remains unchanged; experiment
is test-only, not adopted. No own solver or build live at this checkpoint.

Guarded intermediate size-refinement probe completed: FAIL in all12/42/94cell
patches. It required q>=half previous capped floor, strict squared oversized
edge-deficit progress and nonincreasing maximum length. Not adopted; production
kernel/target unchanged. Source/build/replay logs archived as
native_guarded_state_probe_v1. One serial test, no CFD, no own jobs left.
Assessment priorities: coordinated multi-point reconnection first; joint
first-row/transition rebuild is plausible but not proven necessary here.
Measure actual direction/size variation in parallel; P1 componentwise blend
preserves SPD and is not intrinsically defective. Establish feasibility versus
operator stagnation; target regularization must precede freezing and disclose
any strengthened constraints/complexity change. Smooth geometry is a separate
capability, not an established cause. Keep MPI bounded dependencies and original
accepted CFD state throughout; do not raise cavity caps blindly.

Coordinated reconstruction breakthrough (test-only): MultiPatchProbe with42
imported cells jointly moves existing interior vertices and two new points,
with private flips and intermediate soft q/length demand. Locked unchanged
perimeter and protected-wall vertices. Final46cell patch qmin .20001553815,
Lmax5.43540090835; squared unique-edge length excess147.0193->86.1521(-41.4%).
Full mesh is NOT accepted: Lmax still exceeds1.8. Exact source/logs/hashes in
native_multi_patch_probe_v1. Fixed original quad two-point search had failed;
this larger joint degree-of-freedom test makes progress without changing target
or final q floor. Next: repeat over saved full state and export geometry, then
MPI integration only if strict residual convergence is demonstrated.

Source-linked external operator assessment read from AdapNoExt (READ ONLY);
that report remains untouched. Web fetches failed cache misses; checked cached
primary repositories in /tmp/remesher-operator-study-20261007 and matched cited
HEADs for avro/SCOREC Omega_h/MeshAdapt, relevant working source diffs empty.
Verified optional collapse swap fallback, staged metric step requiring initial
admissibility, and MeshAdapt component noncommercial terms. Visibility-oriented
growth is useful but current positive-geometry/poor-quality failure requires
quality/length obstruction handling too. Sensor-only continuation may leave
dominant geometric BL direction changes unchanged; test contribution first.
No external code copied into production. Serial129.6s loop now converged and
independent geometry check passed; current production still appv26 equivalent.
No own live build/solver at this checkpoint. Goal remains ACTIVE.

Local-cost optimizer tested (native_serial_repair_probe_v2): total replay52.109s,
51commits,24903triangles,all q/L pass. Initial/first/candidate grids AND final
metric CSV byte-identical to independently audited v1. Geometry certificate
reused by exact hash equality. Added shared probe guard against splitting a
protected edge star (one protected incident cell suffices to refuse).
Early-admission experiment v3: native q/L PASS,66commits,24981triangles,64.624s;
slower, NOT ADOPTED, reverted. Next v4 mirrors intended stagnation MPI route:
import second dependency ring, try ordinary SplitPatch on larger admitted
patch first, then joint optimization. No target/final acceptance changes.
Unrelated foreign build in /tmp/codex-periodic-followup-20261007/build_omp was
observed; never interrupted. Own builds limited-j1 while it ran. No source
changes in AdapNoExt: HEAD8882847 and .gitignore SHA7ccfe456... reverified.

2026-10-07 v4 second-ring/ordinary-split-first replay completed: native q/L
PASS,29commits,24833triangles,qmin .200001654520,Lmax1.79998592700,
0shape/length. Total subprocess65.034s, slower than v2 total52.109s despite
fewer commits; do not claim speed improvement. Independent topology, physical
markers, original reference, and h0 checks PASS on this different candidate.
Artifacts/source/hashes: integration_evidence/native_serial_repair_probe_v4.
Candidate mesh SHA71fe8cc5daf894b50807e8add130b780bd09a7934eb58c73cc0c18f0de9e6f29.
No production MPI repair, conservative transfer or resumed CFD demonstrated.
Both v2 and v4 establish serial reachability for the captured RANS state under
unchanged final target; not general robustness or 3D evidence. Current test
source contains v4 hierarchy; retain v2 archive as faster measured baseline.
Assessment: bounded joint repair remains first priority; distinguish visibility
closure from valid-geometry/poor-quality blockage. Private exploration may
have poorer positive-volume shapes but published replacement must retain all
contracts. Full affected stars and donor dependencies belong in MPI transaction
reservations. Trigger expensive repair on stagnation. Sensor-only metric staging
may not help where unchanged geometric BL dominates; full original target must
be attained. Current serial repair locks already-adapted physical boundary;
this is not a proposal to fix boundaries throughout adaptation. MeshAdapt ma/
license checked in pinned local repository; no third-party code copied.
Next: production bounded joint operator and existing MPI transaction integration,
regressions, actual RANS transfer/resumed solves, then common-source Euler replay.
No own build/solver/replay remains live at this checkpoint. Goal ACTIVE.

2026-10-07 production integration draft now present: JointPatch/JointSplitPatch
in shared CNativeMesh2D; strict_cells moved unchanged from boundary layer to
shared mesh layer. Ordinary32cell admission unchanged; joint128cell cap reused
as boundary PATCH_LIMIT. Existing split tried first, then two-point insertion,
metric-direction movement and private flips. Complete perimeter/protected/fixed
vertices locked; full replacement validated. Native Engine coordinated mode
imports two dependency rings, reserves them before search, uses existing ID
allocation, owner assignment, participant certificates, common publication vote.
Repair after ordinary sweeps: at most128rounds/pass and existing sweep count;
stop on no commits or no length residuals. New joint time/commit counters.
Focused joint tests cover private two-point search, second-ring MPI publication,
protected wall cells, memory rejection and certificate-fault state preservation.
First build failed because shared strict_cells remained in boundary namespace;
relocated existing implementation, no duplicate validator. Rebuild live in
/tmp/native_joint_mpi_build_v2.log (one compiler job), not yet test-validated.
Other agent's orthogonal /tmp/su2-metric-build build acknowledged by user;
leave it untouched. Fresh unchanged-input/config cases prepared (NOT run):
rae2822_transonic_v1/nativefix_rans_euler_seed_v20 and nativefix_euler_rans_seed_v11.
Diagnostic captured-state helpers now call the shared joint kernel. Production
CFD acceptance and flow transfer remain unproven. Goal ACTIVE.

2026-10-07 appv27 / RANSv20 FIRST MPI ADAPTATION ACCEPTED:29 coordinated
commits,24833triangles/12615points,qmin .20000165,Lmax1.79998593,all
shape/length/height/reference residuals0. Conservative transfer completed
0.06547s; CFD resumed and saved solution_adap_00001.dat/flow_adap_00001.vtu.
Repair52.235s, total ordinary+repair phase cost exceeds CFD; optimization still
needed. MPI core41cases PASS1/2/4 (native_joint_mpi_core_v2). Earlier test v1
fault initialized parameter by mistake; corrected explicit choice.op.fault=4.
Failed v1 sealed with compressed pinned test binary; runner now tolerates raw
Catch char-vector diagnostics while preserving failure status. Protocol mode
agreement uses existing two action reductions, no extra per-round collectives.
RANSv20 controller TERMINAL timeout124 at302.712s. Second target all q/L
residuals already0 after sweep1, but unnecessary sweeps continued. Attempted
runtime extension found controller already terminal; NO extension performed,
NO solver/controller signals sent. First mesh and resumed solution preserved.
Root cost correction now draft: shared Engine::satisfied() checks q/L AND all
physical height/reference contracts collectively; stop only at complete sweep.
Both production adapter and direct Engine adapt use predicate. New regression
checks valid q/L with unmet altitude does NOT count complete. Validation pending.
Next fresh common appv28 RANS/Euler replays, ten-minute bound set at launch.

## Latest subset and profiling update — 2026-10-08

Source checkpoint bdb59520885ff0686e2cc40d46a661f37074a7c5; goal ACTIVE.
N4/M3 frozen Euler-to-BL workers3_v1 terminal PASS: whole37.816206s,
complete Remesh37.349963s,12246points,50739commits. Independent audit PASS;
qmin.1845586768,Lmax1.7999860512,hrel4.433e-12, transported directional
defect1.004e-10. Audit session43333 terminal0; no owned heavy job remains.
M3 is promising against one-trial M4 42.567s; repeats remain required.
Detailed current timings/scopes/gaps:
`integration_evidence/native_adaptation_partition_v1/remeshing_profile_assessment_v1.md`.
Repartitioning is included in Remesh, but return ReaderSlices is not the subsequent
CFD geometry partition/setup or direct-original solution/history transfer.
Bulk split21.9394s dominates elapsed phases. Private reconstruction max15.0236s
versus min4.03715s; active-worker imbalance persists. Tracked MPI totals include
waits and do not cover every validation election; diagnostics are nonadditive.
Next profiling needs exclusive per-rank dependency/donor import, reconstruction,
validation/commit scopes and current-source sampled stacks. Keep lightweight
operation scopes, no per-query clocks/profiling collectives. Reverse workload,
repeat trials and actual unsteady/restart on reduced-rank source still pending.

## Exclusive timing and actual subset lifecycle checkpoint — 2026-10-08

Native timing build_v1 PASS, app77fbefa77179acd6d8e5c66da3eed61974f8dede4f4d2ce5937544cf9185c6f6,
762 source pins. Added six adjacent round scopes: protocol, dependencies/reservation,
original-sensor donors/IDs, reconstruction, certificate validation/decisions,
staging/publication. Timers are outside point queries; existing three statistics
reductions pack the extra values. Explicit engine-adapt unclassified residual
accounts for inter-round checks, temporary destruction and loop work. Added
engine initialization/final gates, native setup/import/working partition and
CFD return geometry partition/migration, wall distance/solver initialization/
transfer/finalization timers. Maxima are nonadditive; exclusive worker means
close against engine-adapt means. Numerical/transaction decisions unchanged.
Core current-source MPI1/2/4 PASS83cases/rank; all observed timing profiles close
(93/109/126 profiles respectively). Profile audit has positive and broken-closure
selfchecks. Existing stack classifier supports both SU2_CFD and test_driver,
selfcheck PASS.
Actual N4/M3 BDF2 vortex and coarse straight-wall SA RANS each completed two
adaptations: independent quality/length/original-P1 and BL/current+previous
history conservation/positivity audits PASS. Vortex max conservation3.269e-15;
SA max2.267e-15 and hrel4.441e-16. This is the first actual unsteady lifecycle
validation on reduced-rank source, not a real-airfoil cost/accuracy certificate.
Cases: native_adaptation_partition_v1/{vortex_bdf2_n4_m3_profile_v1,
plate_sa_bdf2_n4_m3_profile_v1}. Inspect independent_unsteady_audit.json and
independent_profile_accounting.json.
Current owned perf+frozen run is session70510 at
native_adaptation_partition_v1/frozen_rae_euler_to_bl_n4_m3_profile_v1/runtime_np4.
Profile CPU-clock:u99Hz/dwarf16384; classify stacks after terminal, then audit
frozen metrics and timing closure. Its elapsed time is instrumented evidence,
not a performance comparison. No other owned heavy job. Need SST/CGNS/restarts,
actual unsteady RAE comparisons, reverse frozen workload and repeat/overhead
controls; goal ACTIVE. Four-mode pre-timing-source frozen comparison preserved
in subset_euler_to_bl_comparison_v2.json (M3 fastest single trial).

Current profile+audit sessions70510/4089 terminalPASS; no owned heavy job now.
Real frozen RAE exclusive timing/metric audit PASS; original M3 candidate remains
BYTE-IDENTICAL. Authoritative current CPU classification sampling_summary_v3.json:
9745native,5339MPI frames(54.79%),4771Allreduce;World::Fail3533nearest,
FieldPatch::evaluate1185(12.16%). Earlier template-type labeling and failed
call-only classifier preserved explicitly in classifier_reconciliation.json.
Detailed scopes/costs/limitations: remeshing_profile_assessment_v2.md.
Mean reconstruction8.285s,validation/waits11.572s,unclassified2.912s,adapt32.891s.
This is sampled evidence, not speedup; timings include sampling overhead.
Next: current-source restart/CGNS/SST controls, unprofiled repeats and actual RAE
both directions; do not optimize synchronization blindly or weaken failure gates.

Additional current-source controls all PASS: SST BDF2 N4/M2 and CGNS-output
BDF2 N4/M3 each two adaptations, independent metric/BL/history/positivity and
timing audits. N4/M3 accepted SU2 checkpoint restarted on N2; all conserved
resumed states match parent within3.463e-13. CGNS input N4 restart matches
within8.840e-16. These resume on fixed accepted meshes; post-restart remeshing
and mid-window restart are not established by these comparisons. CGNS runner
correctly deferred while an unrelated solver was active, then launched after
it ended and the quiet gate passed. Owned sessions99399,90737,2032,67723,81073,
67835 all terminalPASS. Summary subset_unsteady_lifecycle_checkpoint_v1.json.
Next real RANS case prepared at native_adaptation_partition_v1/
rae_rans_window200_n4_m3_v1. Same v4 physical/metric setup except explicit M3
weighted partition and output every step, preserving independent donor snapshots
for both BDF2 histories; retain same output policy in future M4 control.
No affordability or recommended-worker-count claim; goal ACTIVE.

Current local checkpoint:99aa0d7f725b0c872f969d6bd64c6ee9f8414165
on codex/native-unsteady-performance. No push performed.
Real RAE RANS owned runner session19476 VERIFIED LIVE; state running;
child 2728578, elapsed 111.16298110299977s at this observation.
Case folder:/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_adaptation_partition_v1/rae_rans_window200_n4_m3_v1
Poll session19476 before any new heavy job. Runner timeout1800s, MPI4,threads1;
quiet gate passed and machine CPU/PSI/affinity is recorded during execution.
This is actual unsteady N4/M3 from coarse Euler geometry,600steps/window200,
50inner cap,original sensor/BL settings/noise0. Output every step retains
independent donor histories; future M4 comparison must match that policy.
No frozen-speedup attribution from profiled elapsed times. Goal ACTIVE.

RAE RANS N4/M3 session19476 terminalPASS482.458493s. Independent audit25984
terminalPASS including all four donor histories (freq1 output):
meshes12246/13091,qmin.1845587/.2258252,Lmax1.799986/1.799954,
hrel4.433e-12. Raw domain-integral defects1.489e-10/5.995e-10; independently
corrected CLOSED/open-farfield policy residuals<=6.943e-13. AllSA histories
nonnegative, finaldensity.2977074/pressure26709.6357 positive. Exact coverage
receipt history_coverage_receipt.json distinguishes generic sparse-snapshot
audit prose from this fully saved case. Timing accounting PASS.
Costs CFD398.089,metric40.6189,remesh42.9013,replace.343071 (transfer.189376
nested),adaptedoutput.0598474; total adaptation83.9231184s,21.0815% of CFD.
Not an affordability or speedup claim. Host editor/browser contention recorded;
no unrelated solver present at the during-run process audit.
Current owned M4 control runner session1344 is VERIFIED LIVE, lastobserved
running,child2765998. Case rae_rans_window200_n4_m4_v1; identical inputs/
physics/output/executable except M4. Poll this handle before anotherheavyjob.
Prepared (NOT RUN) Euler-on-BL cases rae_euler_from_bl_window100_n4_m3_v1
and rae_euler_from_bl_window100_n4_m4_v1, copied the existing RANSgrid,
no BL metric request. Same per-step output policy for both. GoalACTIVE.

RAE RANS M4 session1344 and independent audit78064 both terminalPASS.
Whole478.790746s, CFD397.273,metric33.0217,remesh47.5823,
replace.349569 (transfer.19525 nested),output.0583533;
fulladaptation81.0119223s,20.3920% of CFD. Meshes12216/13005;
qmin.2035572/.2219526,Lmax1.7999718/1.7999813,hrel<=6.043e-12.
M3 reduced native remesh9.84% but fulladaptation was3.59% larger relative toM4.
One pair/different meshes/recorded host activity: NO speedup recommendation.
FirstRANS private reconstruction M4 min3.430/mean8.829/max21.485s;
M3 min3.961/mean8.579/max14.791. Mid-remesh repartition discussed only,
NOT implemented or added to goal. Would require safe transaction checkpoint,
immutable original donor preservation and subgroup-safe partition collectives.
Current owned Euler-from-BL runner session68601 VERIFIED LIVE, phase running.
Case:/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_adaptation_partition_v1/rae_euler_from_bl_window100_n4_m3_v1
Poll session68601 before anotherheavyjob. CFD4,workers3,threads1,
quietmachinegate passed. New pair audit helper prepared, run after solver
terminal to avoid analysis interference. Goal ACTIVE; no push.

RANS matched-pair audit PASS: identical first donor conserved arrays and metric
arrays; complete both-history gates and fullcost comparison saved in
native_adaptation_partition_v1/rae_rans_subset_pair_v1.json.
EulerfromBL M3 runner68601 terminalPASS117.812s; independentaudit35016
terminalPASS both events:7721/7569points,qmin.1841787/.3282109,
Lmax1.7982748/1.7991837; no BL metric requested. Timingclosure PASS;
CFD95.2124,metric4.08796,remesh17.6747,replace.247041 (transfer.148726nested),
output.03458; fulladaptation22.044281s,23.153% of CFD.
Current owned matchingEuler M4 runner47275 started; pollbeforeanotherheavyjob.
Prepared (NOT RUN) vortex_restart_remesh_n4_m3_v1 from mesh00003 +histories1/2,
restartstep3 through8, additional remesh at6. Existing smallcase auditor now
supports complete window-boundary restart windows and retained original
reference; validate on this fresh case and a previous nonrestartcase.
No production source changes; goalACTIVE; no push.

Actual RAE reverse-direction M4 and restart-remesh controls terminal PASS.
Both actual RAE first donor states/metrics match M3/M4 exactly; both histories
pass independent gates. RANS applied-window costs favor M3 by5.71%, while
terminal metric accumulation reverses total cost ordering; do not recommend M.
Euler M4 CPU pressure peak36.33 invalidates a clean timing comparison. A compiler
observed afterward started after the control ended: rejected attribution receipt
and corrected qualification preserved. Pair v2 reports separate applied-window
and terminal metric costs. No production C++ changes since99aa0d7f72.
Vortex N4/M3 window-boundary restart with another remesh at6 PASS; original donor
histories, next metric, next mesh and resumed states bit-exact to parent.
Nonrestart auditor regression PASS; mid-window restart not established.

SGE/qsub campaign prepared in integration_evidence/native_cluster_campaign_v1.
Four frozen/actual directions, original N partition control and weighted N/N2/N4
workers, repetitions and longer actual windows; ranks<=192 only as cluster
experiments, no scaling claim. Twelve regular inputs9.36MB and762 source pins
are contained in repository. RunNativeSGE.sh uses supplied queue/PE/GCC paths;
site machinefile/exclusive resources remain platform responsibilities.
Raw outputs stay in ClusterRaw; selected evidence and audit tools in
ClusterResults for download only. Fake-launcher success/missing-data checks
PASS; actual Euler portable export23files22.58MB and frozen19files4.25MB
independently audit PASS from /tmp using only copied tools/data.
No cluster jobs submitted. New shared host sampler records rank RSS/process
CPU affinity, retaining inaccessible heavy processes in the local launch gate.
No owned heavy job now; no push. Goal ACTIVE: repeated fullcost/scaling/memory
limits and reproducible total improvements remain outstanding. A Git bundle
will provide branch checkout without pushing. Production sources unchanged.

2026-10-08: User clarified that the performance branch should be pushed for
cluster checkout. Non-force push to origin(rois1995/SU2) succeeded at
0af90ff5e09a858b15ea2198341208d53812ee7b. Campaign README now directs remote
checkout; bundle is optional. Goal still ACTIVE; cluster execution/scaling
and repeated local cost measurements remain pending. No heavy job active.

Local frozen repetition campaign started at native_adaptation_partition_v1/
frozen_repeats_v1. First weighted Euler-to-BL M3/M4 controls terminalPASS:
37.763226/41.732095 seconds, same archived12c72828 test executable and762
production source pins. Shared sampler observed all4 ranks, no competing
compute samples; independent mesh/target audits and repeats still pending.
Updated frozen runner uses shared sampler/root-relative paths and checks
immutable small inputs at exit. No owned heavy job now. User cluster Git1.8.3.1
requires checkout instead of switch; README corrected. GitLab Eigen clone
server-load failure: retry pinned submodule, no dependency version change.
Cluster pilots now one repetition, M4original plus weighted M4/M3/M2, actual
twoevents; schedule sequentially before 3repeat/long-window scaling campaign.

USER CONSTRAINT 2026-10-08: STOP local performance assessments. Future timing/
scaling campaigns belong on the cluster. Do not launch more local performance
controls or restart this campaign without explicit user authorization.
Frozen original-M4 r1 solver terminalPASS47.609295s. M3/M4 independent mesh,
transported target, first-height/geometry and timing-accounting audits PASS.
Original-M4 independent audit(pid3011304) terminated with SIGTERM at user request;
its orchestration handle1254 terminalexit1. No PASS for interrupted audit.
No owned heavy job remains. All grids/sensors/logs/source pins preserved in
native_adaptation_partition_v1/frozen_repeats_v1; stopped receipts distinguish
completed controls from cancelled audit. No timing recommendation from single
controls. Goal ACTIVE/incomplete: cluster results, repeated totalcost evidence
and practical scaling/memory limits remain required.

Cluster workflow follow-up only; local performance stop remains in effect.
Found early SGE failures (e.g. missing machinefile) occurred before Python
created ClusterResults and therefore escaped the download-only recipe.
RunNativeSGE.sh now archives wrapper stdout/stderr, revision, exact wrapper
and exit status under ClusterResults/jobs/${JOB_ID}_launcher before resolving
compiler/MPI/input prerequisites. bash-n and fake-launcher selfcheck PASS,
including missing machinefile exit2 with downloadable diagnostics. No real
CFD/MPI/metric/performance assessment launched locally. Solver C++ unchanged;
existing compiled native executables need no rebuild for this script update.
Full goal remains ACTIVE/incomplete pending cluster data and verified totalcost
advantages/scaling limits; no worker recommendation or affordability claim.

User-reported cluster job581658 failed at mandatory `git rev-parse HEAD`: Git
is absent on compute nodes. Bugfix only; performance goal remains PAUSED.
Both shell and Python runner revision capture are now optional (unavailable/
null), retaining mandatory input/source/binary hashes. Source-pin manifest hash
is recorded in job/case provenance. Existing fake-script selfcheck now runs
with Git absent from PATH, checks archived early-failure diagnostics and full
case collection, rejects a changed pinned source, and also handles Git present
but an unavailable repository revision. Checks PASS; no real CFD/MPI/metric/
performance test launched. User must pull scripts on login node and resubmit;
no C++ changes or solver rebuild. Actual cluster execution remains unverified.

## Downloaded cluster results and post-rebase integration (2026-10-08)

All 16 downloaded N=4 cases independently PASS: eight frozen and eight actual
unsteady runs, two adaptations each. All 280 collected original files (276242614
bytes) retain their manifest hashes. Both histories per event have donor snapshots
and pass the independent CLOSED-policy check with open-farfield area correction.
First donor conservative states and sensor metrics are exact across the four
partition modes within each actual workload. Raw evidence: ClusterResults/cases/.
Assessment and compact numerical/phase receipts:
integration_evidence/native_cluster_analysis_v1/ASSESSMENT.md and assessment.json.
All campaigns overlap on node-a-ag2.local and have overlapping allowed CPU sets;
this does not prove oversubscription but prevents an isolated speedup claim. One
repeat per mode, N=4 only. Observed weighted M4 full adaptation totals: RANS65.223,
Euler16.700 s; M3/M2 are larger. RANS metric36.882 s vs remesh27.738 s motivates
the authorized metric optimizations. MPI reconstruction imbalance remains.

User now requests integrating the post-rebase metric handoff and preparing a
matched old/new cluster campaign after correctness validation. Source handoff:
SU2_AdapNoExt/Papers/POST_REBASE_METRIC_HANDOFF.md, validated metric head6285044614;
current performance baselinee6995fbff5. Read/reviewed functional range including
carry-over long-path restart fix, stable intersections, deterministic WLS,
compensated normalization, supported QR wall/symmetry corrections, adaptation-only
GG simplex recovery, steady metric reuse and active nodal sensor gradation.
Preserve sensor-only donor P1 and actual-position geometric BL, finer demands,
fade, M<=N execution, failure/output/restart lifecycle and noise default0.
Uncertified P1/composed-field gradation stays explicit; no hard-normal reset.
No new local performance campaign. Build/test jobs wait for machine capacity.
Unsteady WINDOW_AVERAGE averages |H| every step by definition; PREDICT already
samples only its scheduled snapshot steps. Steady duplicate metric is removable
without changing those temporal targets or public refresh semantics.


## Pending integration and new PREDICT request (2026-10-08)

Working branch codex/native-post-rebase-metric-candidate has an uncommitted merge
of metric head6285044614 onto d8fc473a00. Production auto-merge is clean; the RAE
CSV/Euler/transport audit conflict is resolved using the immutable original wall.
Combined executable archive native_post_rebase_metric_v1/build_v2 is pinned.
MPI1 serial metric163/native87 cases and MPI2 metric38/native87 per rank PASS.
MPI4 metric37 and steady metric reuse separately PASS, but GoalMPI followed by
SteadyMetricReuse reproducibly times out in native BULK_FLIP (round45, 14 commits).
CustomSensorsMPI->steady and recovery->steady PASS. Do not publish as fully
validated or discard these failures. One accidentally broadened Catch negated-OR
selection exited139 in an unrelated serial geometry case; invalid selection and
raw receipt are preserved explicitly. Further frozen/unsteady/AD checks pending.
Local work is correctness only, low priority, one owned compute job at a time.
Two-core affinity plus OpenMPI yield is used; no local timing conclusions.

Temporary passive-communication tracing has been removed. Original/debug versions
and the trace binary are preserved in native_post_rebase_metric_v1/debug_passive_trace.
An M=N private-worker communicator trial also timed out and has been reverted;
its failed receipt is in private_worker_trial/run. Both production sources are
restored exactly. Original production archive build_v2 remains unchanged.
The MPI4 test-order interaction is unresolved; no sensor/BL guard is relaxed.

User explicitly requests selectable PREDICT snapshot count and fixed cadence
backward from the last adaptation step, plus temporal noise filtering. User chose
feature velocity/acceleration tracking rather than tensor-entry extrapolation.
Draft changes are isolated in /tmp/native-predict-multisnapshot; NOT applied to
repository sources yet. Planned options ADAP_PREDICT_SNAPSHOTS (default2), existing
ADAP_PREDICT_SEPARATION cadence, and ADAP_PREDICT_TEMPORAL_FILTER (default1). Extra
history fits centered/ridge-damped feature acceleration with an original-history
mismatch fallback to constant velocity; time-dependent transport traces each
future endpoint independently, preserving convex SPD interpolation and exponential
congruence. Additional optical flows and trajectories cost more; no performance
claim. Config/cadence and manufactured motion/SPD tests drafted, not compiled.
ADAP_HESSIAN_NOISE stays0. The latest user declines exclusive cluster nodes; the
old/new comparison uses usual shared nodes and chained jobs, timings approximate.
Comparison helper/README/fake-qsub checks are drafted in /tmp, not yet finalized.
Goal tracker remains PAUSED; these tasks are separately authorized.

## MPI4 timeout diagnosis corrected (2026-10-08)

Per-rank LD_PRELOAD traces on the unchanged archived build show matching native
collective sequences and continued progress. The supposed four-CPU mask 6,7,8,9
was also effectively two CPUs because this host has only CPUs0-7. Moving owned
ranks to CPUs4-7 completes GoalMPI->SteadyMetricReuse successfully, without any
production fix. Earlier timeouts do NOT establish a protocol deadlock and remain
preserved as failures. mpi_timeout_resolution.json records the diagnosis and
trace hashes. Communicator hypothesis and CPP tracing both reverted exactly.
Remaining baseline-merge correctness campaign now runs sequentially at nice10
on four valid CPUs; still no local performance assessment. New PREDICT draft
has boundary input validation and a manufactured jitter filtering test added.

## Baseline metric integration validated before prediction extension

validation.json aggregates the successful checks: metric/native MPI1/2/4,
18 frozen GG/WLS/QR RAE fields (targets12000/60000), independent sensor residuals
and composed diagnostics, forward/reverse non-OpenMP syntax, steady RAE N4M3,
frozen Euler-to-BL and BL-to-Euler N4M3, native WINDOW_AVERAGE CGNS vortex and
straight-wall SA with both BDF histories, MMG PREDICT/FIXED_POINT compatibility,
immutable RAE reference and accepted fixed-point output checks. Native PREDICT
and FIXED_POINT were explicitly unsupported; prepared native PREDICT smoke
stopped at that guard. The authorized prediction extension will admit PREDICT
only, retain native FIXED_POINT rejection, and add support tests. Invalid setup
attempts (duplicate config options, contradictory AD flags, applying native
sidecar/event checks to MMG) remain preserved with their corrected controls.
No mesh/transfer/metric numerical gate relaxed. Native audit defaults are intact;
only an explicitly selected MMG Euler control can omit native-sidecar/event checks.
Prediction extension is still unapplied and uncompiled.

## Prediction extension applied; full rebuild pending

Validated metric merge checkpoint5cb87667c20f7c590fab00aa373c7c056a2daca1
is committed locally, not pushed. Eleven reviewed files now extend PREDICT with
count/cadence, temporally filtered feature acceleration and time-dependent
transport. Native CheckSupport admits PREDICT; native FIXED_POINT still rejected
and its expected-failure test retained. Manufactured acceleration/SPD/jitter,
legacy two-frame equality, config schedules and MPI gather/scatter tests added.
Full j1 ABI rebuild (CConfig layout changed) is running in build_predict_v1.
Correctness cases prepared in predict_history_v1, including legacy MMG control,
native default-two, four-frame N1/M1 N2/M1 N4/M3 (CGNS) and a no-remesh donor
fixture for partial-window restarts. No new tests executed against the old ABI.
Cluster comparison drafts still pending final-source pins and final validation.

Prediction correctness preparation: `predict_history_v1/four_fixture_n1m1` now has ten steps (one actual native replacement at step 5), providing its real immutable reference sidecar and matching BDF restart states. Partial-window restarts will start at steps 7, 8, and 9 to exercise three-/two-/one-snapshot fallbacks without fabricating reference bindings. The sequential controller is saved as `integration_evidence/native_post_rebase_metric_v1/complete_predict_history_validation.py`. Invalid count/cadence/filter configurations and the retained native fixed-point guard are negative checks. Full ABI build remains in progress; no prediction smoke has run yet.
