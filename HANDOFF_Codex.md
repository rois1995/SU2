# Goal 2 resolved vortex native matrix complete; independent audit active — 2026-10-13

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-curved-local; previous pushed commit 37a8aa6c6a5f.
Goal 2 remains ACTIVE; completion is unproven pending the full independent audit.

resolved_vortex_v3/validation.json is terminal PASS. All six N/M variants
(1/1, 2/1, 2/2, 4/1, 4/2, 4/4) completed two actual Euler-pressure-derived
metric/remesh/current+BDF-transfer events, real restart and solver continuation.
Earlier successful variants are hash-verified and reused through repository-contained
links. Original v1 parser failure and v2 whole-batch timeout remain preserved.
No frozen physical, geometry, sensor, quality, sliver, length or work limit changed.

resolved_vortex_audit_v1 physics and matching flow exports now cover all six
variants. The exact whole-lifecycle auditor is the sole owned heavy job:
controller session 38893, PID 683492; child PID 683638 as last checked.
Revalidate the handle before acting; never restart solely from elapsed time.
One CPU (4), nice19, threads1. No additional CFD/MPI/build job is running.

current_source_validation.json verifies all 763 current production sources against
the passing 107-case-per-rank MPI1/2/4 regression and both new native receipts;
no source drift. It also verifies prior cylinder/lifecycle audit input hashes and
closed reuse links. Existing curved atomic-veto test is PASS MPI1/2/4 including
worker subsets and a two-owner edge star. No redundant broad suite is required.

Next: inspect the independent resolved geometry/P1-or-extension/quality/length/
transport/current+BDF/conservation/restart certificate, reconcile every original
requirement, and push final evidence before claiming completion. No performance,
cluster, production3DBL, general CAD, shock/force-convergence or arbitrary-mesh
feasibility claim. AdapNoExt, foreign ClusterResults and dependencies untouched.

Earlier notices below are historical.

# Goal 2 curved MPI transaction qualified; final vortex variant active — 2026-10-13

Working repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated,
branch codex/native-3d-curved-local, previouspublishede5b23cc19b. Goal ACTIVE,
full objective retained and final completion INCOMPLETE.

resolved_vortex_v2 controller60343 TERMINAL FAIL_WRAPPER_TIMEOUT: MPI2 passed
M1/M2; MPI4 M1/M2 completed both events/restart/continue, M4 did not complete before
1800s observation ceiling. No native assertion error reported; source_driftNONE.
Verified no surviving owned vortex ranks before changing CPP. All completed and
partial files/receipt are retained; none discarded or replaced with relaxed target.

curved_transaction_v1 PASS MPI1/2/4, M1/M2/M4 where available. New actual curved
edge-star across2owners: predeclared finite source, projected positive/actual-metric/
geometry-progress split, stale version rejection, rank veto, storage veto,
byte-identical owned mesh/directory/frozen sensor data on rejection, successful
publication and complete new vertex-star retrieval. Production code unchanged.
The new test is installed in CNativeDistributed3D_tests.cpp. Test-only vortex
worker selector avoids rerunning already successful variants; complete state
gather remains bounded4096points. Build includes all68actual objects/provenance.

LIVE sole heavy job82953: resolved_vortex_v3/run.py, only N4/M4 outstanding variant,
unchanged frozen CASE_PLAN physics/geometry/metric/quality/sliver/length/work.
Verified reused earlier serial/N2 and N4 M1/M2 files through repository-contained
links, hashes checked; new source changes are tests/selection only. Revalidate this
handle; do not restart from logs/elapsed time or launch another heavy job while
active. One CPU/nice19/threads1/build-j1/MPI<=4, no unrelated-process termination.

Independent physics audit now covers all available evolved donor states; actual
pressure/positivity/seed sampling agree acrossMPI within small numerical differences.
Full resolved exact lifecycle audit sequence is prepared to require terminalPASS
onv3, with source hierarchy plus sweep-box admission; NOTRUN while job active.
GOAL2_REQUIREMENT_MAP.json tracks all original requirements; curved atomic proof
now resolves the previously identified direct transaction gap, pending final
full source/evidence audit and resolved independent certificates.

NEXT: finish final verified variant; preserve terminal matrix/partial timeout;
run one independent resolved physics/flow/exact geometry/P1-or-declared source/
shape/length/transport/conservation/BDF/restart audit; inspect every requirement;
update/push final evidence before declaring Goal2 complete. No cluster/performance/
shock/convergence/CAD/production3DBL/fullphysical-unsteady claims. AdapNoExt/foreign
ClusterResults/dependencies untouched. WIP progress source/evidence is pushed;
unfinished final MPI output waits for terminal receipt before staging.

# Goal 2 final proof preparation; resolved MPI4 active — 2026-10-13

Repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated,
branch codex/native-3d-curved-local, previouspublished27b91dfa238e. Goal ACTIVE;
full original objective retained. No production/test-suite CPP changes this turn.

Resolved vortex MPI2 PASS both M1/M2 two-event cases. Original serial result is
verified/reused; MPI4/M1 both events complete, M2/M4 still pending. LIVE owned
controller60343 is verified; poll same handle before any new heavy work. Prepared
independent audit now uses source-box hierarchy and sweep-box mesh admission,
followed by unchanged exact donor/intersection/edge checks. Stored81-case closed
box equivalence selfcheck PASS. Full resolved lifecycle audit is NOTRUN while
MPI4 active. Same one CPU/nice19/threads1, no source changes while pinned.

Created GOAL2_REQUIREMENT_MAP.json covering the actual full scope with specific
source/tests/certificates and remaining proof. It identifies a direct current
curved MPI partition-interface/atomic-veto transaction proof to establish before
closing; successful mesh outputs alone do not prove rejection atomicity.
Prepared (NOTCOMPILED/NOTRUN) curved_transaction_v1/prepared_test.inc: two-cell
curved graph edge star across ranks, finite source declared before freeze,
actual metric/geometry progress, stale dependencies, rank veto, storage veto,
unchanged mesh/directory/frozen source and successful versioned publication.
Insert into CNativeDistributed3D_tests.cpp only AFTER60343 becomes terminal, then
one focused MPI1/2/4 check. No new production feature or acceptance change needed.

NEXT: complete/preserve existing resolved run; run prepared independent lifecycle,
physics/resolution/flow export audits; qualify curved transaction; inspect every
original requirement and push terminal evidence. Do not close from prepared
scripts or earlier small prototypes. Goal audit remains INCOMPLETE, no cluster/
performance/CAD/production3DBL/fullphysical-unsteady/shock/convergence claims.
AdapNoExt/dependencies/foreignClusterResults untouched. All links must remain
inside this repository; current serial reuse link points to published v1 data.

# Goal 2 resolved Euler vortex: serial qualified, MPI active — 2026-10-13

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-curved-local, previouspublished240a27bd2576. Goal ACTIVE,
full objective retained and completion audit INCOMPLETE/all four gates PENDING.

New testcase [NativeResolvedEuler3D] uses actual Euler/Roe evolution of a resolved
isentropic-vortex initial condition on12x12x3/2592tets/676points, analytic lower
bump-.001, radius.7/beta3, measured8central profile samples. Frozen CASE_PLAN:
complexity400/hmin.2/hmax.9/AR2/noise0, q.20/J.05/L1.8/Hausd.003/work16384.
Two CFD-pressure-Hessian/native remesh/conservative current+BDF/restart/continue
events; no prescribed sensor or post-failure limits change. Production code is
unchanged from240a; existing test-only complete state gather bound is4096points.

Serial native testcase PASS31034assertions/1case:2592/676initial ->2767/655 ->
2763/656. resolved_vortex_v1 receipt FAIL is only a plural-only wrapper parser
('1 test case'); native exit0. Original script/receipt/logs retained. v2 corrects
and self-checks singular/plural, verifies original source/binary/artifact pins,
reuses serial via repository-contained mpi1 link; no serial solver rerun.
Independent seed/evolution physics PASS: initial analytic error3.57e-16, pressure
minimum ratio.74344, relative evolved change.06171, positive current/BDF states.
Measured serial mean core volume ratio.58294, core246->422cells. Actual pressure/
density/Mach VTU exports in resolved_vortex_audit_v1/mpi1; not convergence proof.

LIVE owned controller60343, resolved_vortex_v2/run.py. MPI2/M1 both events complete;
MPI2/M2 active coarsening, then MPI4/M1/M2/M4 outstanding. Brief owned stack samples
in resolved_vortex_progress_v1: rank1 actively ConformingDisjoint exact clearance,
rank0 waits at matching clearance vote. No deadlock established; both detached.
Source pinned while live. One heavy job, one CPU/nice19/threads1/build-j1/MPI<=4.
Revalidate60343; do not restart from elapsed time, stale logs or this handoff alone.

Whole independent resolved lifecycle audit is prepared in resolved_vortex_audit_v1/
audit_lifecycle.py (sweep-box broadphase then exact geometry/contact/field/state/
restart checks), NOTRUN while MPI active. Report RESOLVED_VORTEX_CONTROL.md.
NEXT: finish verified MPI run; independently audit all completed states and actual
metric refresh/resolution; explicit final requirements/features/components/
partition-interface/atomicity audit. Preserve full Goal2; no cluster/performance/
shock/convergence/production3DBL/generalCAD/fullphysical-unsteady claim. No changes
toAdapNoExt/dependencies/foreignClusterResults. WIP source/serial checkpoint will
be pushed; live MPI folders must wait until their terminal receipt before staging.

# Goal 2 capped-cylinder source integration qualified — 2026-10-13

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-curved-local, previouspublished5b3abbd436. Goal ACTIVE;
full completion audit INCOMPLETE/all four full requirement gates remain PENDING.

Fixed before-freeze cylinder/cap sensor coverage through one conforming radial/
axial collar for matched declared cylinder+end disks. Shared rim identities and
coordinates, copied boundary sensors, original-P1 priority, finite immutable
projection envelope, parent routing/component tags/storage/ID guards preserved.
No post-failure extension, source containment epsilon, metric reset or tolerance
relaxation. Old72-cell frozen fixture stays a negative coverage regression; new
actual CFD import generates24original+96extension cells, shared36nodes.

cylinder_cap_coverage_v2 PASS MPI1/2/4:5cases/rank, all primitive workersM=1..N.
Previously stalled tilted-cylinder now completes two actual native events and
CFD conservative/BDF replacement+restart+SU2/CGNS readback:42cells/32points,
matching axis0 control; sphere26/18. Existing runtime operators complete this
control after the source fix. Native coordinated strip is geometrically and
actual-field validated against fresh generation; automatic strip placement/
publication is still absent and can be revisited when an actual target needs it.

cylinder_cap_audit_v1 PASS: exact collar/original no-overlap and shared tensors,
copied varying demand, original-priority samples/exact full edges and physical
source consistency acrossMPI1/2/4. All21primitive variants/two events each are
independently audited, missingNONE: exact embedded geometry/source lengths/shape,
transported metrics, actual current+BDF integrals/positivity/new-lost accounting,
reference/restart/SU2-CGNS signatures. qmin0.20185139; Lmax1.58113883.

cylinder_cap_regression_v2 PASS:107cases/rank MPI1/2/4, whole68-object/current
source/archive/member/binary pins; source_driftNONE. Current manufactured curved
refine/coarsen/grow/shrink/CGNS restart and actual solved-Euler pressure-derived
metric/remesh/SU2restart/continued solve refresh PASS. Independent fresh lifecycle
validation PASS. Initial test build/private-helper error and regression_v1
105/107diagnostic-string failures preserved; precise node/facet diagnostics
restored. No owned job remains live; no broad retest needed before new changes.

Report integration_evidence/native_3d_curved_v1/CYLINDER_CAP_COVERAGE_INTEGRATION.md.
Actual accepted tilted grid:
integration_evidence/native_3d_curved_v1/cylinder_cap_coverage_v2/mpi1/cylinder_axis1_m1_mesh_00001.su2
Refreshed solved Euler grid:
integration_evidence/native_3d_curved_v1/cylinder_cap_regression_v2/mpi1/solved_bump_m1_mesh_00001.su2

NEXT substantial work: resolved nonuniform/shock Euler control where locally
feasible, derived from actual solved states with repeated metric/remesh/continue/
restart, then explicit final proof of every declared geometry/features/components,
partition-interface/atomicity and transfer/output requirement. Existing27-point
subsonic bump demonstrates actual metric cycles but does not resolve a shock.
Prefer a locally bounded resolved isentropic-vortex or normal-shock control; freeze
physics/mesh/metric/geometry/contracts before tests. Do not resume larger-cavity/
placement tuning just to repeat the now-fixed primitive stall. Retain full goal
scope, permanent AGENTS batching cadence, no cluster/performance/generalCAD/
production3DBL/fullphysicalunsteady/force-convergence or arbitrary feasibility
claim. AdapNoExt/dependencies/foreignClusterResults untouched. Push this qualified
integration checkpoint; Goal remains ACTIVE, no full completion justified.

# Goal 2 coordinated strip and frozen cap coverage — 2026-10-13

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated,
branch codex/native-3d-curved-local. Goal ACTIVE/all four full gates PENDING.
Added atomic SplitSurfaceStrip: bounded simultaneous complete-star moves plus
same-chart curved split, fresh reserved identities, incident feature projection,
positive inherited volume connectivity, fixed artificial skin and existing
full manifold/reference validation. No production engine placement/publication
integration yet, no source or acceptance limit changed.

Scientific four-wall-vertex proposal reduces local geometry deficit about6.96%,
q0.204/J0.055. Native first check exposes nonmanifold union; bounded edge-link
closure adds exactly one unchanged cell, 28old/30new. Native v2 geometry succeeds,
then actual frozen-field edge0->200000 rejects uncovered cap-rim interval.
Six of seven focused serial cases pass; MPI2/4 NOTRUN after failure. Independent
exact closure/no-overlap/interface audit succeeds but source query audit FAILS.
Exact scientific-coordinate gap on edge0->12 is~1.05e-18 physical, not a native
reprojection gap measurement. Coverage remains strict and target untouched.

Evidence/report: integration_evidence/native_3d_curved_v1/
SURFACE_STRIP_RECONSTRUCTION_WIP.md; surface_strip_replay_v1/v2 and
surface_strip_native_v1/v2. Inspectable whole working proposal (not accepted):
surface_strip_replay_v2/cylinder_axis1_strip_proposal.su2. Native assertion remains
failing until the before-freeze coverage capability is corrected; do not call
this checkpoint validated. Failed original fixture/diff/auditor versions retained.
No owned job live. Existing bump/sphere/axis0 CFD evidence predates this source.

NEXT major connected work: conforming cylinder/cap source collar declared BEFORE
freeze, consistent shared identities (avoid overlapping same-priority sources),
finite representability envelope and unchanged original-P1 priority/tensors.
Then native bounded placement/full-star union+link closure/reservation/publication,
full tilted lifecycle, independent accepted-state audits, MPI and actual Euler/
resolved nonuniform controls. All full goal requirements retained. Follow permanent
AGENTS.md batching cadence; no micro-optimization/cluster/performance/generalCAD/
production3DBL/fullphysicalunsteady claims. AdapNoExt/dependencies/foreign results
untouched. Source checkpoint will be pushed WIP, not integrated qualification.

# Goal 2 surface/recovery investigation — 2026-10-13

Repo SU2_NativeIntegrated, branch codex/native-3d-curved-local; previouspublished
f6708bce5b2f7ce4d737af27cf7a05cb996449c5. Goal ACTIVE/all four full gates PENDING.
New source changes only CNativeEngine3D.cpp: existing streamed complete-vertex-star
recovery now tries existing CollapseVertex after movement, retaining all geometry/
feature/actual-field/clearance/progress/atomic guards. Planar direct-field path
unchanged. recovery_collapse_v1 handle95657 TERMINAL FAIL: build PASS,4/5serial
cases PASS; tilted-cylinder still operator-stagnates, accepted state retained.
Source/archive/member/binary pins and rejected working evidence retained. MPI and
broader current-source regression for this new extension are NOTRUN; previous
MPI2/4 observation was on f6708/b9e54 source and cannot qualify new code.

Sequential surface probes: surface_swap_replay_v1 handle11235 terminal no immediate
geometry-progress swaps/zero trials; surface_fan_replay_v1 handle30133 no proposal
in10growing patches. frontal_fan_replay_v1 handle22300 local floating candidate
q0.212375/J0.053244, but geometry reduction only~1e-12relative; do NOT qualify as
robust/accepted/exact mesh. frontal_fan_replay_v2 handle66412 no proposal in560
trials with stricter >=1% experimental geometry reduction, otherwise same frozen
target/gates. Source/results/logs preserved; no production relaxation/metric change.
All owned jobs terminal, no heavyjob live. Report SURFACE_RECOVERY_FINDINGS.md.

NEXT substantial step: short coordinated physical surface strip and volume
transition reconstruction, supporting several wall triangles together instead of
more isolated-fan placement/search tweaks. Bound dependencies/work/memory, preserve
features/marker/components and actual frozen originalP1/extension composition.
Qualify accepted-state audits/MPI/broad integration milestone and actual solved
Euler refresh/resolved nonuniform/shock control before full completion. No arbitrary
feasibility claim from failed limited probes. Scope unchanged, AGENTS batching
cadence and no cluster/performance/CAD/3DBL/fullphysicalunsteady/forces claims.
ForeignClusterResults/dependencies/AdapNoExt untouched.

# Goal 2 collected MPI and multi-apex experiment — 2026-10-13

Repository SU2_NativeIntegrated; branch codex/native-3d-curved-local, previous
published b9e54cda6d40c089167e390b8779b8e91c965e5e. Goal ACTIVE/all four full gates
PENDING. Production source unchanged in this continuation.

MPI observer75064 TERMINAL FAIL_MATRIX (MPI2 exit2/MPI4 exit4), no timeout/source
drift. Every rank passes four of five selected cases, failing all tilted-cylinder
worker-count variants. Other primitive lifecycles complete for four N2/eight N4
variants including M=N/M<N. Full independent current-source accepted mesh/source/
state/restart audits remain outstanding; a failing matrix is not qualification.
Owned-rank stack samples in joint_mpi_stall_v1 showed different in-flight phases;
rank2 actively reconstructed and the job finished normally. No deadlock proved.
No restart, source edits during pinned MPI, or unrelated-process interruption.

Fixed physical-face diagnosis on v4 rejected surface: all160 triangles individually
have possible-quality upper bounds >=0.209609 and J upper bounds >=0.079040;
q0.20/J0.05 alone do not individually rule them out. Joint/global feasibility is
not thereby established. Derivation in two_apex_replay_v1/FACE_FEASIBILITY.md.

Two-apex prototype partitions a prescribed cavity skin into two face groups,
cones each to its interior point and bridges the interface-ring edges. It keeps
all outer coordinates/faces except the declared projected split and frozen0.5I
q0.20/J0.05/length1.8/error0.04 contracts. Limited group family: low-quality single
faces and adjacent face pairs, at most8growth/64old/128new/70SLSQP iterations per
proposal, wall ceiling480s between proposals plus540s hard process limit.

Initial two_apex_replay_v1 session95088 TERMINAL FAIL on a nonmanifold intermediate
patch; original source/FAILURE.md retained. v2 skips such topology before proposal,
rejects nonfinite/degenerate trials and keeps separate base growth measures.
Session43705 TERMINAL exit0: NO_PROPOSAL_IN_BOUNDED_SEARCH,3074candidates, bestNULL.
No accepted grid, production operator or complete feasibility result arose.
No owned heavy job remains live. Experiments/source hashes/results/logs retained.

NEXT substantive step: coordinated physical surface diagonal/fan reconnection
plus volume reconstruction on the saved tilted-cylinder blockage. The one-/two-
apex bounded families have not fixed the path; stop tweaking their growth/optimizer
settings or raising caps. Preserve feature/chart/marker junctions, actual frozen
P1/extension target and exact topology/clearance/atomic failure safeguards. Then
qualify current-source MPI/accepted-state audits and the broad connected milestone;
genuine solved Euler refresh and locally resolved nonuniform/shock still required.
Follow AGENTS.md batching cadence; no cluster/performance/3DBL/generalCAD claim.
ForeignClusterResults/dependencies/AdapNoExt untouched.

# Goal 2 MPI observation collected; two-apex probe active — 2026-10-13

Repository SU2_NativeIntegrated, branch codex/native-3d-curved-local; published
b9e54cda6d40c089167e390b8779b8e91c965e5e. Goal ACTIVE/all four full gates PENDING.
Controller75064 TERMINAL FAIL_MATRIX: MPI2 exit2, MPI4 exit4, no timeouts/source
drift. Four of five selected test cases pass on every rank; tilted-cylinder
variants fail for every remeshing-worker count. Completed axis-aligned-cylinder/
sphere lifecycles: 4 on N2, 8 on N4, including M=N/M<N, actual CFD replacement,
BDF restart and SU2/CGNS readback. Independent current-source accepted-grid/state
certificates remain required; a failing matrix is not integration qualification.

MPI4 looked quiet during its final variant. Owned-rank stacks were captured in
joint_mpi_stall_v1: rank0 clearance collective, rank1 metric collective, rank2
actively reconstructing, rank3 dependency lookup. The run subsequently ended
normally, so this observation did NOT establish deadlock. No run was restarted or
unrelated process interrupted. Tiny diagnostics retained; rank2_second records
process disappearance after completion.

Fixed-face diagnosis on joint_runtime_v4 rejected surface:160 physical triangles;
minimum possible-quality upper bound0.209609 and J upper bound0.079040. Neither
q>=0.20 nor J>=0.05 individually rules out those faces, but joint feasibility is
unproven. Derivation in two_apex_replay_v1/FACE_FEASIBILITY.md. Next larger proposal
uses two interior apices plus bridge tetrahedra, preserving the prescribed outer
skin except its projected physical split. Constant0.5I diagnostic target only.

Initial two_apex_replay_v1 session95088 TERMINAL FAIL at nonmanifold intermediate
skin assertion; source and FAILURE.md retained. No candidate certificate or
production code resulted. v2 gates topology before reconstruction, regularizes
only degenerate objective evaluations and rejects nonfinite candidates, preserving
actual final positive/q/J/length/geometry gates. Growth uses separate base measures.

LIVE owned job43705: timeout540s, nice19, one sampled idleCPU, threads1;
`integration_evidence/native_3d_curved_v1/two_apex_replay_v2/run.py`, logs
probe.stdout/probe.stderr. Its internal480s ceiling is checked between proposals.
Revalidate same handle/status; do not launch another heavy job until terminal.
No production source changed in this continuation. After completion, inspect
positive geometry/topology/exact no-overlap/frozen-field coverage before importing
an experimental operator into native; update the full audit and push evidence.
Current-source broad regression, stronger features/components, solved Euler refresh
and resolved nonuniform/shock control remain outstanding. Original Goal2scope and
AGENTS batching cadence retained; no cluster/performance/generalCAD/3DBL claim.
ForeignClusterResults/dependencies/AdapNoExt untouched.

# Goal 2 joint reconstruction WIP — 2026-10-13

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`; branch
`codex/native-3d-curved-local`, previous published `68b24b82eccd9f420f4990d814bbedb530c2a10d`.
Goal ACTIVE; all four full requirements PENDING. Connected implementation adds
projected physical-edge split plus one interior point and complete skin-constrained
reconstruction, using shared projection/surface-child validation. A frozen actual
old-centroid tensor guides bounded metric-frame placement only; all final actual
centroids/edges are checked against the unchanged original/declared source.
Complete neighbouring-face dependencies are imported, version/owner checked and
re-reserved; 12 trials/64 old/128 new, global work/memory/time ceilings remain.
Every extra joint trial counts as work. Curved node IDs reserve two slots; planar
IDs unchanged. Missing/protocol/storage failures remain collective vetoes. Recovery
retries reopened geometry queues as well as oversized metric edges. Existing global
curved serialization, clearance, atomic CFD rejection and conservative rebuild stay.

joint_replay_v1: saved v7 obstruction admits six-old/twelve-new coupled proposal,
48% local geometric deficit reduction, bounded pattern q0.224286/J0.065229.
Native saved-geometry regression passes frozen0.5I metric, geometry progress,
exact pairwise contact/no-overlap and rejection rollback. The small generated
fixture includes its original saved-source hash. This is a local operator proof,
not full arbitrary P1/CFD/MPI/goal qualification.

joint_runtime_v1 terminal FAIL test missing include (production compiled), retained.
joint_runtime_v2/v3 terminal FAIL4/5 serial selected cases PASS; tilted cylinder
still stagnates. joint_runtime_v4 terminal FAIL4/5 PASS; limiting quality/J-ratio
face growth increases private edits to86 but does not complete the case. Last
rejection feature3:1733 geometric error non-improvement; accepted CFD state retained.
No acceptance/source tolerance was relaxed. Full production/member/binary pins and
saved grids/states/failures retained. No broad current-source regression yet.

joint_replay_v2 session29240 TERMINAL: no admissible one-interior-point proposal
in792 bounded trials on the latest v3 rejected geometry. Owned nice19 process moved
fromCPU0 toCPU2 due contention; adjustment record retained, no timing claim. Exact
papers folder found `/media/rausa/4TB/SU2_Versions/Prove/Papers`; read Loseille/Alauzet/
Menier2017 CAD paper sections2.3–2.5 and local repair/2D movement code. An overly broad
owned file search was identified and stopped; no unrelated process touched.

LIVE controller75064: `integration_evidence/native_3d_curved_v1/joint_mpi_audit_v1/run.py`.
Current v4 source/binary pinned, sequential MPI2/4 on oneCPU/nice19/threads1, no new
build beyond no-op integrity check. It continues observations after matrix failures;
status FAIL_MATRIX must not be presented as green. Revalidate its handle/receipt;
do not launch another heavy job or change pinned source while live. Case folder
currently `joint_mpi_audit_v1/mpi2`; MPI4 follows. New test folders already announced.

Next substantial work: coordinated surface retriangulation and/or multiple interior
points to escape the recorded full-path blockage; stop tuning single-point growth
order or raising caps. Preserve failed matrix. Qualify current-source MPI/rejection/
accepted-state audits, then the meaningful broad integration milestone and genuine
solved Euler/resolved nonuniform control. Original full Goal2scope intact; no cluster/
performance/CAD/production3DBL/full physical unsteady/force-convergence claim.
Report JOINT_RECONSTRUCTION_WIP.md. AGENTS.md permanent batching cadence applies.
ForeignClusterResults/dependencies/AdapNoExt untouched.

# Goal 2 primitive runtime WIP — 2026-10-12

Working repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch
`codex/native-3d-curved-local`; previous published commit `2d00b57ffabd78ce05f4350a71fb6b210d93c9a0`.
Goal ACTIVE, all four full requirements PENDING. This connected batch adds finite
arbitrary-axis cylinders, disk cap/rim authority, certified complete primitive
components, shared cylindrical source/projection geometry and axis-aware v3
reference persistence (v1/v2 retained). Geometry selection retains alternatives;
finite-triangle sphere error is tighter where the plane foot is outside. Geometry-only
repairs require actual geometry progress plus strict shape/sliver/nonincreasing
metric excess. Oversized curved repair candidates are skipped using the existing
batch dependency protocol; storage/protocol/missing-feature failures still abort.
Cavity 64/replacement 128/work 4096 limits and accepted-state safeguards unchanged.

Latest focused controller `primitive_runtime_v7/run.py`, session 52797, TERMINAL
FAIL: build PASS, serial 3/4 cases PASS; primitive matrix fails on tilted cylinder.
Sphere and axis-aligned cylinder complete both actual CFD replacement cycles,
BDF/SU2 restart and SU2/CGNS readback. Tilted cylinder accepts 77 private edits,
then operator-stagnates; rejected replay has 121 cells/62 nodes. MPI2/4 were NOTRUN
because serial failed. No broader regression for this changed production batch.
All v1-v7 failures/source/build/member/binary pins and relevant grids/states remain.
The v5 estimated-gain rounding experiment did not fix rotation and was removed.

Independent corrected auditor `primitive_audit_v1/audit_primitives_final.py`,
session 80275 TERMINAL PASS, produces `primitive_runtime_v7_audit.json` with status
PASS_COMPLETED_LIFECYCLES_ONLY, explicitly listing failed/unrun variants. Exact
volume/closure/no interior overlap, geometry/source/metric, conservative actual
current/BDF integrals, positivity, transported metric residual 0 and real restart/
writer signatures pass for the two completed serial variants. Initial auditor
used box marker defaults incorrectly; retained source/AUDIT_CORRECTION explain the
existing marker-parameter fix. No runtime source or gate changed for that audit.
No owned heavy job remains live. Foreign ClusterResults/dependencies/AdapNoExt untouched.

Inspect cases in
`integration_evidence/native_3d_curved_v1/primitive_runtime_v7/mpi1/`:
`sphere_axis0_m1_mesh_00001.su2` (26 cells/18 nodes),
`cylinder_axis0_m1_mesh_00001.su2` (42 cells/32 nodes), and both second-event files.
Rejected tilted grid exported separately at
`primitive_audit_v1/primitive_runtime_v7/mpi1/cylinder_axis1_m1_s0_rejected.su2`.
Frozen geometry targets remain 0.22R sphere/0.04R cylinder, sensor0.5I, q>=0.20,
J>=0.05, metric edge<=1.8. These are coarse prescribed-metric controls, not solved
primitive CFD or tight-geometry proof. Current-source MPI/M<N, component/feature
negative controls, broad regression and resolved nonuniform/shock remain pending.

Next substantial step: exact replay of the saved obstruction and coordinated
boundary/transition cavity reconstruction with off-edge/interior placement under
unchanged dependency/replacement bounds. Do not continue rounding priorities or
blindly increase caps. Then qualify the connected result, MPI and independent
accepted-grid/state audits; retain full Goal 2 scope. Permanent cadence AGENTS.md:
focused checks while developing, broad suite at meaningful integration milestones.
Report PRIMITIVE_RUNTIME_INTEGRATION.md and full requirement audit remain incomplete.

# Goal2 curved conservative and solvedEuler lifecycle — 2026-10-12

Working repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local, previouspublished dbd44516cc. GoalACTIVE/all4fullgates
pending. New single-file lifecycle controls pass actual curved growth/shrink,
threeconservative refine/coarsen events, bothBDF histories through CGNS restart,
actual SU2/CGNS readback, and twoactual solved3DEuler pressure-Hessian metric events
with SU2/BDF restart and solver continuation. No production policy/gate changed.
Source test changes oneobject; libraries/archive members unchanged from previous
100case/rank MPI1/2/4 regression. Permanent cadence in AGENTS.md: no redundant broad
rerun for unchanged production. Focused curved_lifecycle_v1 PASS2cases/rank
MPI1/2/4; independent_lifecycle_validation.json PASS, source/build/binary/67other
object/production member pins verified. Controller73168/auditor63150 terminalPASS;
no owned heavyjob live. ForeignClusterResults/dependencies/AdapNoExt untouched.

Inspect curved_lifecycle_v1/mpi1/solved_bump_m1_mesh_00001.su2 and_00002.su2,
and curved_lifecycle_grow_m1_mesh_00001.cgns, plusMPI2/4/M<N. Serialgrowth8->48->8,
shrink8->48->10; new/lostvolume~0.00020833. Actual accepted-state integrals match
conservative targets, allthree stored states positive, all15restart values match.
Solved subsonicMach0.5/slip-bump pressure yields nonuniform anisotropic changing
tensors, actual event meshes37cells/21then22points. Minimum savedserialq~0.438688,
maxedge~1.792384, metrictransport residual0. This is a small solved lifecycle, not
resolvedshock/convergedphysicalunsteady/forces proof. Curvedcommits stillserialized.

Report CURVED_EULER_LIFECYCLE.md; full completion auditINCOMPLETE. Initially prepared
independent auditor hadoneunmatchedparenthesis; immutable original retained,
audit_lifecycle_final.py corrected, noCFD/source/binary/gate changes. Actual cfgs,
grids/refs/source/state/projection-ledger/restarts/logs retained. Next substantial
work: sphere/cylinder runtime and fullmixed/feature geometry, then suitably resolved
nonuniform/shock controls. Keep original fullGoal2scope; no cluster/performance/CAD/
production3DBL/affordability claim. Earlier notices are historical.

# Goal2 connected curved runtime integration — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE; all4full integrated gates pending.
Blanket analytic refusal replaced by MPI-consistent positive frozen surface
ceiling; geometry drives selection/progress while strict q/J/length remain.
Projected actual-query source imports, projection-owned movement and exact indexed
current-mesh overlap/conforming-contact checks precede atomic publication. Curved
commits serialized until concurrent-displacement collision reservations exist;
planar batching retained. No performance/scaling claim or cluster work.

curved_runtime_regression_v2 PASS:100selected cases/rank MPI1/2/4, full68-object
source/archive/member/binary pins, independent accepted-grid/state/source/volume/
sidecar auditPASS. Actual initial analytic bump sidecar loads;6->8CFD driver mesh
replacement and constant current/BDF transfer pass allM=1..N. Inspect
integration_evidence/native_3d_curved_v1/curved_runtime_regression_v2/mpi1/curved_runtime_m1_mesh_00001.su2
plusMPI2/4/M<N files. Volume1.0002083333333334 approaches analytic1.0002777777777778;
sampled wall error0.00015625<0.003 complements conservative whole-face bounds.
Sensor prescribed0.5I; not solvedCFD-derived metric or physical unsteady proof.
No q/J/length/geometry/BL/fade/NOISE0/transfer/MMG gate weakened.

Connected twelve-file implementation and controls, source snapshots and earlier
owned-build interruption/compile failure retained. First broad99/100 failure was
an obsolete diagnostic assertion, now corrected; failed original receipt remains.
All original artifacts hash-verified directly or in compact planar payload archives;
actual curved grids/sources/states/sidecars stay inspectable. Report
CURVED_RUNTIME_INTEGRATION.md; full audit remainsINCOMPLETE. Controllers94180/78676
terminalPASS, no owned heavy job live. ForeignClusterResults/dependencies/AdapNoExt
untouched. Permanent cadence in AGENTS.md: connected code, focused checks, broad
regression at meaningful integration milestones, regularly push reviewable work.

Next substantial work: repeated curved conservative/current+BDF/restart/CGNS and
real solvedEuler-derived metric/remesh/continue cycles. Sphere runtime/cylinder and
full geometry/feature coverage remain pending. Preserve full objective; do not close
Goal2 from this first accepted prescribed-sensor bump event. Older notices below
are historical.

# Goal2 connected curved runtime — implementation in progress, 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated, branch
codex/native-3d-curved-local, previous published9e7bcac255. Full goalACTIVE/all4gates
pending. Connected production changes now replace blanket analytic refusal with
positive frozen surface tolerance, geometry-driven selection/progress, projected
actual centroid/edge imports, movement-owned projection and exact indexed current-
mesh contact/overlap checks before publication. Curved commits are serialized;
planar batching retained. No performance qualification. Twelve production/test
files changed together; see CURVED_RUNTIME_INTEGRATION.md.

Focused curved_runtime_v3 PASS on MPI1/2/4: actual initial sidecar loads,6->8CFD
replacement, constant current/BDF transfer, actual SU2 writer/reference reload.
Independent runtime auditPASS, volume moves toward analytic bump authority.
Prescribed0.5I sensor, not solvedCFD-derived metric. Inspect
integration_evidence/native_3d_curved_v1/curved_runtime_v3/mpi1/curved_runtime_m1_mesh_00001.su2
andMPI2/4/M<N equivalents. First broaderMPI1 selection99/100 passed; only failure
is obsolete blanket-refusal text assertion, now corrected to frozen-tolerance
refusal. Production code unchanged since focused qualification. Prior controlled
build stop/compile failure and broad failed receipt/log/source patches retained.

Owned controller94180 is the sole live heavy job at this writing, in
curved_runtime_regression_v2; poll its handle/terminal receipt before modifying
pinned source or restarting. It incrementally rebuilds the changed test object,
then runs100selected cases on localMPI1/2/4, nice19/oneCPU/threads1. Next: independent
accepted-grid audit for broad receipt, final source/artifact proof, update report/
findings/completion audit and push this substantial connected implementation.
Then prioritize actual solvedEuler metric cycles, repeated curved conservative/
restart/CGNS and remaining geometry (including cylinder), rather than another
sequence of isolated micro-checkpoints. ForeignClusterResults/dependencies/
AdapNoExt untouched. Permanent implementation cadence in AGENTS.md.
Earlier notices below are historical.

# Goal2 automatic curved sensor-source integration — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE; all4integrated gates pending. Actual CFD
boundary import now produces bounded sensor-only bump columns/sphere frusta before
freeze. Original physical P1 retains priority; original boundary tensors are copied
exactly and new records migrate with their CFD parent for M=N/M<N. Engine/reader
association is wired; private projected-query footprint/component handling and
geometric completion/clearance still block curved runtime publication. Source
fixtures use prescribed arrays on actual CFD geometry, not solved CFD-derived
metrics. No 2DBL/fade/NOISE0/MMG/solution-transfer/acceptance policy changed.

generated_sensor_v2 PASS:97cases/rank MPI1/2/4/full68-object driver. Independent
source auditPASS: exact positive volumes/no-interior-overlap, shared coordinates/
tensors, parent boundary-face provenance and physical MPI consistency. Private
curved operator shape/closure auditsPASS. Source SU2 envelopes/JSON/native references
are directly inspectable in generated_sensor_v2/mpi1 (andmpi2/4); these are not
adapted CFD grids. V1 mistaken numeric facet-ID invariance assumption is preserved
with source/log/receipt and corrected physical-provenance audit. V2 also corrects
parent-routing map storage admission after moving the original cell container.
Every original controller artifact is hash-verified directly or in a compact
planar payload archive. Report GENERATED_SENSOR_CHECKPOINT.md; source/member/binary
pins and publication_integrity.json retained. Controllers2538/91704 terminalPASS;
no owned heavy job remains live. ForeignClusterResults/dependencies/AdapNoExt
untouched. No cluster/performance qualification.

Next substantial step: geometry-driven refinement/completion, actual projected-
candidate source routing and nonlocal clearance together on one end-to-end curved
runtime case, then accepted curvedMPI/solution/history/output/restart and actual
Euler metric cycles. Cylinder/mixed geometry remain gaps. Do not close Goal2 from
these import/operator controls. Permanent user cadence in AGENTS.md: group
connected implementation work, focused development checks, broad regressions at
meaningful integration milestones rather than every small edit/checkpoint.
Earlier notices below are historical.

# Permanent implementation cadence — user instruction, 2026-10-12

See AGENTS.md. Group connected changes into substantial end-to-end implementation
steps. Use focused checks during development; run broad regressions at meaningful
integration milestones, not after every small edit/checkpoint. Keep necessary
correctness gates and regularly push clearly scoped, reviewable work. This rule
persists across context compaction and future sessions.

# Goal2 declared exterior sensor field checkpoint — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE; all4full integrated gates pending.
Original physical P1 now has priority over explicitly associated frozen exterior
cells at queries and exact rational edge intervals. Tags travel through MPI source
imports; wrong components/gaps/ambiguous same-priority intervals reject. Original
thin demands and coincident-cut nonzero widths are preserved. Composition still
uses actual queries/cache hits; composed-target edge audits still reject without
geometric integration. All records remain validated; native field scratch models
include bounded layered trace/cache/index storage. No 2DBL/fade/noise/MMG/transfer
policy change. Default callers exclude tagged exterior coverage.

layered_sensor_v2/validation.json PASS:95cases/rank MPI1/2/4 with full68-object
driver; independent shape/closure/no-overlap audit alsoPASS. New source-tag test
is actual MPI import; curved split/field witness uses a manually prescribed finite
shell. Inspect layered_sensor_v2/mpi1/layered_sphere_split.su2 and matchingJSON,
plusMPI2/4. Not solvedCFD/curvedMPIpublication/history transport or production
coverage-generation proof. Firstv1 failed on a real fixture segment gap; exact
source/logs retained and gapped geometry remains a passing negative control.
V2controller73422 terminalPASS; no owned heavy job remains live. Incidental planar
payloads are hash-verified in archives. Source/proof pins and full limitations in
LAYERED_SENSOR_CHECKPOINT.md. ForeignClusterResults/dependencies/AdapNoExt untouched.

Next big integration: actual CFD-derived exterior coverage generation/admission
before freeze and actual private/remote query footprint/component routing, then
geometric error/clearance completion and actual curvedMPI/CFD/restart. Source layer
is implemented, producer/integration remains missing. Cylinder/mixed geometry and
nonconvex chart unions remain gaps. No cluster/performance gate or claim.
Earlier notices below are historical; do not close Goal2 from these prototypes.

# Goal2 joint curved geometry checkpoint — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE, all4integrated gates still pending.
Compatible supplied bump facets now share certified connected-convex XY charts;
markers, primitive definitions and disconnected components remain distinct.
Off-edge surface-cavity insertion, projected movement across original seams and
changed-diagonal collapse are implemented with positive inherited cell orientation,
whole-face chart checks and atomic rollback. Geometry metadata/v2 persistence
reconstructs the canonical union. Original raw authority/default planar behavior
and sensor/BL/fade/noise/MMG/transfer policies are preserved. Runtime curved
adaptation still refuses pending coverage/final geometry checks.

joint_bump_geometry_v1/validation.json PASS:90cases/rank MPI1/2/4 with full68-object
driver; independent private_shape_validation.json also PASS. Actual saved inserted,
moved and coarsened grids in joint_bump_geometry_v1/mpi1, mpi2 andmpi4. This is
private operator and collective reference proof, not curved distributed adaptation,
solution transport or actual solved3DEuler proof. Incidental planar regression
payloads are hash-verified in an archive; current private grids remain inspectable.
Controller31846 terminalPASS; no owned heavy job live. ForeignClusterResults,
AdapNoExt and dependencies untouched. See JOINT_BUMP_GEOMETRY_CHECKPOINT.md.

Next: integrate geometric completion/clearance and bounded frozen source coverage
with consistent exterior edge integrals; then actual curved MPI/state/output/restart
and small realEuler cycles. Cylinder, mixed curved/planar expansion and nonconvex
chart unions remain gaps. Do not close Goal2 from these operator prototypes.
Earlier notices below are historical; no cluster/performance qualification.

# Goal2 analytic geometry checkpoint — 2026-10-12

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-curved-local. GoalACTIVE, all4integrated completion gates pending.
Analytic sphere/bump finite authority, coupled projected complete-edge-star split
and inverse, inherited-connectivity inversion rejection and version2 persistence
are implemented. Default planar rules/v1 references retained. Final local receipt
native_3d_curved_v1/analytic_geometry_v3/validation.json PASS: full68-object driver,
86cases/rank MPI1/2/4, connected planar bridge/runtime/default and2Dfield controls.
Independent private_shape_validation.json PASS: identity shape/sliver gates,
exact rational no-interior-overlap and signed-skin/cell-volume closure on actual
saved original/split/coarsened sphere/bump private proposals. Inspectable SU2/JSON
in analytic_geometry_v3/mpi1 (andmpi2/mpi4). These are not solved CFD grids.

Mixed curved/planar finite-patch expansion remains unsupported and rejects;
initial failed experiment/source retained in analytic_geometry_v1. V2 preliminary
source/evidence also retained; V3 fixes acute-corner tolerance amplification by
using finite arc/edge distances. No loosening of geometry/quality/field gates.
Runtime explicitly refuses analytic authority pending integrated donor extension
and geometry completion. Existing2D/3D sensors/BL/fade/NOISE0/MMG/transfer unchanged.
No owned build/test controller remains live: v3session33629 and shapeaudit42293
terminalPASS. AdapNoExt, foreignClusterResults and dependencies untouched.

Full stage scope, evidence and remaining steps:
integration_evidence/native_3d_curved_v1/ANALYTIC_GEOMETRY_CHECKPOINT.md.
Next main step remains complete coupled geometry: chart unions/feature constraints,
curved operations and geometry-driven completion/clearance, supported by explicit
bounded triangular sensor extension and exterior edge integration. Point-query
fallback alone is insufficient. Cylinder and actual curved MPI/CFD/restart gates
remain pending. Keep one owned heavy job, announce folders, localMPI<=4, push
checkpoints; no cluster/performance qualification. Earlier notices are historical.

# Goal2 activated: local curved-boundary/Euler work

User cleared Goal1 and explicitly activated the revised second goal. Active branch
codex/native-3d-curved-local from23197eba1715f44d61c9ad3a070f61441dfb0560; working
repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Goal1 complete on its previous
branch. Goal2 excludes ALL cluster/performance qualification/large-wing campaigns.
Full current objective/gates in NATIVE_3D_DEVELOPMENT_GOALS.md. ClosedGoal1 plan
archived in native_3d_curved_v1/GOAL1_CLOSED_PLAN_23197eba17.md; its oldplan hash
refers to that completion snapshot, not subsequentGoal2 revisions.

First coverage study session86307 terminal PASS:
integration_evidence/native_3d_curved_v1/coverage_study_v1/validation.json.
OneCPU/nice19, threads1, full configured68-object driver; MPI1/2/4 selected
NativeCurvedCoverage3D+NativeMesh3D+NativeField2D. Production archives/members and
other67test objects unchanged. Manually prescribed three-tet exterior coverage
supports sphere-octant projection and sensor edge integration while preserving
original interior P1 exactly and rejecting uncovered queries. Existing2D bounded
associated-edge extension and wrong-component/distance controls also pass.
This is NOT production padding/curved remeshing/distributedgeometry/CFD or Goal2
completion. No owned controller remains live; new unit regression is qualified.

After user question traced existing2D: FieldPatch uses exact P1 first, separate
roundoff envelope, then nearest associated original boundary-edge trace within
ADAP_HAUSD; MPI import padding2*limit, query limit unchanged. Sensor interpolated
on old edge, BL composed at actual queryp. Native2D retains original polyline,
not unknown smooth curve.3Ddirection should reuse bounded associated-face trace
semantics with consistent exterior edge lengths; Queryfallback alone is incomplete.
Manual shell is a coverage control, not a commitment to productionghost tets.

Connected map: native_3d_curved_v1/IMPLEMENTATION_MAP.md. Current PlanarReference
and changed-skin checks protect exact faceted domains. Need explicit curved
geometry/features/component/provenance, coupled volume edit/clearance proof,
metric extension before freezing, MPI admission/persistence, existing3Dtransfer
new/lost-domain handling, and real smallEuler metric/remesh/continue/restart proof.
No delegation or extra heavy job; no source/quality/geometry relaxation, unsupported
3DBL still rejects. ForeignClusterResults/dependencies/AdapNoExt untouched.
Next: explicit bounded associated-face sensor trace and consistent exterior edge
integration, analytic geometry/component authority and first coupled curved edit.
Do not commit to ghost tets merely because the manually prescribed study passed.
Push this qualified study plus active plan; all4Goal2 integrated gates remain pending.
Previous Goal1 notices below are historical.

# Goal 1 local qualification complete — 2026-10-12

User scope: local implementation/correctness only; performance qualification and
ALL cluster work excluded. Working repository
/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated, branch
codex/native-3d-transport-accounting. AdapNoExt and foreignClusterResults/dependencies
are untouched. All seven LOCAL_GOAL1_COMPLETION_AUDIT.md/json deliverables PASS_LOCAL;
Qualified source/evidence pushedf782ddc4b20703461b3c3b06192126879639d55a.
All required local work is finished; goal-system completion follows this publication
stamp. Goal2 remains prepared, not activated. No cluster runs or performance
qualification are required to close Goal1.

No owned controller remains live. execution_policy_whole_v3 session77385 PASS and
local_goal1_regression_v1 session75299 PASS. Whole3cases: size8serialbatch1 (productive
615/790/1002round phases), size8defaultbatch8, aspect2/rotated/varying/H20thinN4M2CGNS
with actual persisted two-file BDF restart. Exact original-P1 metric/positivevolume/
conformity/no-overlap/no-gap/finitefacet/marker/current-BDF/SU2CGNS readback contracts
pass. Worst state residual1.13e-14; no target/shape/geometry tolerance weakening.
Serial certificates reused only by exact artifacts/unchanged geometric-metric
primitives plus fresh logs/output checks; MPI4/M2certificate fresh. Inspectable
SU2/CGNS/VTU meshes and state JSON live under execution_policy_whole_v3/.

Final configured68-object driver:106core and39native2D cases each MPI1/2/4, bridge
and8distributed auditors per size, actualMMG2D/3D48assertions and6computed-metric/
Hessian lifecycle frames (one per M, no timing repeats/instrumentation comparisons).
Only the isolated lifecycle fixture changed after wholequalification; production
archives and other67test objects remain byte-identical. Exact current source,
objects,archives/members,binary and artifact hashes in final regression receipt.
Actual cfg shared work/tiny/generous clock controls pass;6actual-driver one-rank
vetoes before/after rebuild preserve original/current/BDF state exactly. Parser
understands typedstop and parentevent versus inactive worker;126actualrows plus
8corrupt controls pass. Historical fixture/parser/import failures remain FAIL.

Native production no longer has an implicit512-round cutoff: default rounds0,
finite global primary-proposal allowance max(4096,64*inputcells), shared across
Refine/Coarsen, optional ADAP_NATIVE_MAX_WORK and ADAP_NATIVE_MAX_TIME. Exploration
of new rejected directions is distinguished from certified size/quality/removal
progress; bounded repair and unchanged frozenq/J/edge criteria remain. History4096
covers observed789accepted splits; beyond4096growth code reviewed but unexercised
experimentally. Deadline covers native entry through prepublication checks; earlier
metric prep and postpublication finalization/output excluded, safe-point overrun
possible. Refusal remains collective fatal, not automatic continuation/retry or
configuration rollback/checkpoint. Simple embedded/conforming faceted tetrahedra;
no CAD/curved, production3DBL, physical PDE/convergence/full unsteady3D or efficiency/
scaling/affordability claim. See final audit for exact envelope and capability limits.
Goals2–4 are prepared and not activated by this closure. No more Goal1 local work
is required after successful publication; the next substantive scope is curved
geometry and actual3DEuler lifecycle. Old notices below are historical/superseded.

# Local Goal1 audit recovery — 2026-10-12

Latest user instruction excludes performance and ALL cluster work from this goal.
Live sequential local correctness controller: session42728, execution_policy_whole_v2/run.py.
Old session90843 is terminal FAIL solely from audit_lifecycle.parse converting the
new string stop=complete to float, after its numerical driver passed and the audit
reached log parsing. No old aggregate PASS or complete independent certificate is
claimed. The shared parser now explicitly validates typed stop, finite numeric
values, global work allowance and complete/stop agreement;21actual rows and6corrupt
controls pass. Original failure preserved. V2 copies successful source/binary/object/
archive-pinned B1driver artifacts by exact hashes and performs a fresh full audit;
remaining batch8 and MPI4/M2CGNS cases are fresh. No CPP edits while v2 is live.

Current source backup and completed policy/runtime/veto proofs pushed03e713341777.
Prepared (NOTRUN) local_goal1_regression_v1/run.py checks actual106registered core
cases on MPI1/2/4 plus bridge/eight distributed auditors each, relevant native2D
operators/engine/eight driver replacements/rejected-output, serialMMG2D/3D; no
scaling/performance tags selected. Actual computed metric/Hessian lifecycle still
needs a correctness-only control, avoiding old repeated OFF/ON profiling campaign.
Goal1 remains ACTIVE until LOCAL_GOAL1_COMPLETION_AUDIT all7deliverables are proven
and the qualified production change/evidence is pushed. No Goal2 work yet.
Previous notices below are historical; use this live handle and current local plan.

# Local execution-policy verification: whole audits live — 2026-10-12

User removed performance and ALL cluster work from current Goal1. Follow current
NATIVE_3D_DEVELOPMENT_GOALS.md and LOCAL_GOAL1_COMPLETION_AUDIT; older goal-system
objective text/archived performance gates are superseded by this explicit revision.
Working repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated, branch
codex/native-3d-transport-accounting. Latest pushed scope revision5ffc6ffa2303;
qualified batching baseline3345e2bc9d. Goal1 ACTIVE, Goal2 not started.

Terminal actual controllers:33248 execution_policy_focus_v1 PASS (full310step
configured68-object driver rebuild +2focused policy cases MPI1/2/4);
26548 execution_policy_runtime_v1 FAIL solely because its first separate veto
fixture omitted CONV_NUM_METHOD_FLOW. Its3selected policy/runtime cases on MPI1/2/4
passed and are preserved. 99225 execution_policy_veto_v2 PASS: only unselected
veto-fixture numerics setup and truth-reporting changed; selected3objects and all
production archives/members stayed byte-identical and were verified before reusing
successful tests. Six fresh actual-driver veto cases on MPI1/2/4 validate one-rank
refusal before replacement and after rebuild/transfer. Observed all-rank state/
reference/deadline predicates plus exact original/current+BDF JSON equality are
audited, not merely expected constants. Expected collective fatal exit is required;
wrong initialization error, Catch failure or accidental success would fail the
collector. Original failed runtime receipt remains FAIL. No numerical rerun of
passed budget controls for this fixture-only correction.

Source fixes already made: successful MeasureMetricCells optional diagnosis sets
None; original-source remeasurement failures classify Measurement. Metric rejection
sample is stored only for the finally rejected cavity, avoiding stale samples from
rescued unrelated trials. Search windows and sample_metric_present are logged.
Added actual cfg shared Refine/Coarsen limits, tiny-clock refusal, successful deadline
callback and CFD/current/BDF donor preservation with M<N workers. Each configuration
uses its own MeshSolution/configuration pair so MPI marker bookkeeping is valid.
None of the old focus TODOs about these fixes remains unimplemented.

Authoritative LIVE whole-case controller session90843:
integration_evidence/native_3d_core_v1/execution_policy_whole_v1/run.py.
OneCPU/nice19/threads1/MPI<=4 correctness only. Poll this handle; do not restart a
quiet process or edit its pinned CPP/config_template while live. Actual serial n8
wedge batch1 production rounds0 completed all3driver events:615coarsening,
790refinement and1002final coarsening rounds, each complete/zero cached-invalid/
zero pending edges. This proves productive operation beyond512 under the finite
shared global work policy at the CPP/driver level. Fresh full exact independent
audit is RUNNING, not yet PASS. Next controller cases: default batch8n8N1M1 and
turning/varying aspect2H20N4M2CGNS restart, all rounds0 and fresh full mesh/original-P1/
current-BDF/SU2CGNS/readback audit plus VTU export. No timing matrix, performance
analysis, cluster package preparation/submission/analysis or cost calibration.

Current scope remaining: finish these independent whole cases; then one broad local
MPI1/2/4 core/default/budget/reference/bridge/operator regression, relevant2D/MMG,
actual metric/Hessian lifecycle correctness, and requirement-by-requirement local
completion audit/push. Verify actual registration counts: base103+3new core cases
=>106core; Runtime profiling-only case is outside local correctness campaign;
separate NativePolicyVeto3D tag is excluded from ordinary regression since it must
abort. Never assume counts without the actual listing. Existing legacy
run_envelope.py relinks a6-object partial driver; DO NOT use it for this closure.
All new controllers use actual configured68-object binary/archives. Optional old
instrumentation remains; no performance claim or performance gating.

Keep declared limitations: simple faceted tetrahedral domains, immutable finite
planar facet authority, bounded stars/donors/operators; no CAD/curved/production3DBL/
physical unsteady3D or efficiency/scalability claim. Deadline covers native entry
through prepublication checks, with safe-point overrun; earlier metric prep and
postpublication finalization/output are outside its hard scope. Production refuses
collectively before replacement, not automatic CFD continuation/retry/checkpoint.
History grows under collective byte admission; current >512case fits initial4096
history capacity. Actual capacity-growth branch still needs a meaningful local
control if claimed in final audit. Do not claim calibrated automatic64-input-cell
work factor; performance was deferred, functional bounds are still required.

working_source.patch/WORK_IN_PROGRESS.json preserve current unqualified full source
while whole audits run. Commit/push only these labelled backups/docs and compact
completed receipts while source is pinned; production CPP qualification/publication
follows the broad local audit. Foreign ClusterResults/dirtydependencies untouched,
AdapNoExt unchanged. Old live-session notices below are historical and superseded.

## User scope revision: local-only Goal1 — 2026-10-12

The user explicitly removed performance qualification and all cluster-related work
from Goal1 and requested closure using only available local resources. The current
NATIVE_3D_DEVELOPMENT_GOALS.md supersedes older performance completion gates. Original
plan is archived unchanged in GOAL1_PRE_LOCAL_SCOPE_PLAN_20261012.md (SHA256 71a4207c973cb9cb4dcfd63a20d60a116126c9424127ef191de0dc41d6dc98db).
No new cluster preparation/submission/analysis, timing/scaling/overhead/throughput/
weight-cost calibration campaigns are authorized for this goal. Historical evidence
and instrumentation stay intact. Functional work/time budgets, bounded memory,
MPI1/2/4 and M<N consistency, metric/geometry/markers, source authority, current/BDF
transfer/restart, rejection preservation, independent local audits and relevant2D/MMG
regression remain mandatory. Current full build is still live under controller33248;
do not edit pinned CPP while it runs. Goal1 ACTIVE until all seven revised local
requirements are proven; this scope revision is not completion. No efficiency or
scalability claim follows from local closure. Goal2 not started.

# Execution-policy implementation build live — 2026-10-12

Working repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-transport-accounting, qualified batching baseline pushed
3345e2bc9d9561045e459f89ea046a3c7583fd5d. Goal1 ACTIVE. User authorized combined
blockage diagnosis and progress-aware bounded execution; actual working source
is now modified, not merely a plan. New source is UNQUALIFIED until checks finish.

Authoritative live controller session33248:
integration_evidence/native_3d_core_v1/execution_policy_focus_v1/run.py.
Changed CNativeMetric3D/CNativeEngine3D objects compiled successfully; complete
configured68-object driver rebuild is running with310steps because CConfig and
CRemesher headers changed. OneCPU affinity5/nice19/j1/threads1; real host memory/
load/idle gate retained. Then focused2case NativePolicy3D MPI1/2/4 sequential.
Do not edit pinned CPP/config_template until this controller becomes terminal;
poll the actual handle and never restart just because output is quiet. Logs live
at repository-root complete_driver_build.stdout/stderr until controller moves them
into evidence. No other owned heavy job, cluster submission or timing campaign.

Implemented working source: EngineOptions maximum_rounds0 defaults to a finite
size-scaled global proposal budget; positive rounds are explicit diagnostic caps.
Automatic initial allowance max(4096,64*global input cells), provisional and not a
feasibility/calibration bound. Runtime shares one allowance across Refine/Coarsen.
CFG ADAP_NATIVE_MAX_WORK0 selects automatic; explicit values bound globally admitted
primary proposals including later conflict/rejection, with separately bounded repair
trials. ADAP_NATIVE_MAX_TIME0 disables the clock; enabled deadline starts at native
remesh entry, checks workers/return and optional beforeAcceptance at actual driver
entry and just before publishing rebuilt/transferred state. Earlier metric prep,
postpublication finalization/output and safe-point overrun remain separate: not a
complete hard CFD-ready deadline. Default2D/MMG do not set callbacks/add their votes;
explicit new controls on2D/MMG are rejected rather than silently ignored.

MetricDiagnosis separates measurement/shape/progress failures and records exact
removed/added frozen-edge excess plus certified size/quality/removal progress.
Private geometry and metric trial/refusal counters and one representative cavity
feature/centroid plus weakest/sliver IDs are emitted. EngineStop distinguishes
complete/forced-round/work/time/operator-stagnation/other failures. No candidate
failure is called proof of infeasibility. Progress-window diagnostics distinguish
certified mesh progress from finite fresh rejected-proposal search; such search
continues under the work budget instead of being prematurely aborted. Existing
bounded repair escalation and no-admitted-candidate rejection remain. History
capacity grows only under collective byte admission before publication; ID bounds
remain checked. Strict q/J/original-P1/full-final-geometry gates unchanged.

Added focused real sliver/nonprogress/bad-measurement and work/time/mismatch/input-
preservation cases. Current selection count will increase105core/106mixed (verify
actual listing after build). Positive512/1024driver fixtures now force their stated
round control; actual envelope supports rounds0 for the production work policy.
Full default-policy >512 completion, runtime cfg/shared-phase caps/deadline callbacks,
independent output/state audit and one broad regression still need qualification.
Cluster comparison preparer intentionally remains source-guarded to the published
batching baseline; do not claim it can freeze the new unqualified dirty source.

Known review follow-ups before broad qualification: successful standalone
MeasureMetricCells diagnosis should set failureNone (current engine validations
reset verdict correctly, but optional standalone diagnostic success still shows
Measurement); include search_windows in reports; independently exercise actual
shared runtime budget and prepublication callback, and audit deadline/control
matching. Do not patch these while the pinned build/controller is live. Preserve
any failed focused receipts; only rebuild changed units after substantive fixes.
Working source snapshot working_source.patch/WORK_IN_PROGRESS.json is a backup,
not a qualified implementation checkpoint. No need to request cluster work while
user unavailable. Keep original fullGoal1performance/scaling/overhead/weights/wait/
memory requirements open; Goal2 not started. Foreign ClusterResults/dependencies
untouched; leave AdapNoExt unchanged.

# Current checkpoint: batching qualified, diagnosis next — 2026-10-12

## Integrated batched refinement/coarsening qualification — 2026-10-12

ENGINE_BATCH_VALIDATION.md/json records the complete current manufactured-domain
milestone: size8default512three-event driver2415→467→2542tets; strong/rotated/varying
H20N4M2CGNS-restart417→64→413tets with fresh complete independent audit; full104case
MPI1/2/4, bridge/eight distributed audits, six actual lifecycle frames, serialMMG and
51tool checks plus actual103core registration and closed same-binary package smoke.
Complete-star lookup/fetch/reservation share exchanges for up to8independent proposals;
private geometry/original-P1 validation and atomic per-candidate publication remain.
Sensor-only original-P1 authority and q>=.20/J>=.05/L<=1.8 criteria are unchanged.
Shared CPoint empty/isolated CSR root fix resolves the actual MPI4emptyCFDrank rebuild
assertion, with focused and full bridge regression. Failed historical receipts/source/
raw outputs remain intact; collector basename/count fixes reuse only source/build/
artifact-verified successful cases. See report for exact recovery/certificate scope.

This supersedes the historical size8production512failure for the declared current
batched driver trajectory, not for all mesh sizes or partitions. No net speedup or
fullGoal1 completion follows. Same-binary batch1/8 SGE comparison is prepared; copied
smoke binaries were removed after hashes to avoid storage duplication, and no cluster
job was submitted. Diagnosis and progress-aware bounded execution are the next
integrated engine step. Current512is a synchronized-round guard, not a justified
mesh-size bound; incomplete coarsening can be metric-valid but remains rejected under
the existing policy. Production driver stops before replacement on incomplete results,
so retention is not automatic continuation. Repeated scaling/cost/overhead/weights/
true-wait/memory envelope requirements remain open; Goal1 ACTIVE, Goal2 not started.

Working repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-transport-accounting; pushed basefa17b3ecee48 at this edit.
All owned numerical/audit/tooling controllers terminal. No live owned heavy job.
Current code matches qualified full68-object binary; no CPP change since the shared
empty-connectivity fix. Relevant sources/docs/tools and compact owned evidence are
ready for explicit staging/review/commit/push. Never stage foreign ClusterResults or
baseline dirty externals/submodules. Preserve AdapNoExt. Candidate comparison now
requires engine_batch_qualification_v1/validation.json PASS/source pins. Current
README uses --comparison-kind candidate_batch and --size4 is corrected to --size 4;
its shell blocks pass bash-n after this documentation-only edit. The prior preparation
smoke retains its original recipe pin; copied large binaries are removed and it is
not a runnable deployment bundle. Recreate from the documented preparer on the
cluster login host before submission; no need to ask/submission while unavailable.
Next actual code milestone: user-authorized combined blockage diagnosis and
progress-aware bounded execution, as recorded in NATIVE_3D_DEVELOPMENT_GOALS.md.
Do not treat a candidate rejection as mathematical infeasibility or declare the
fullGoal1complete. Report post-push SHA separately; commit cannot include its ownSHA.

# Batched engine status and remaining execution-policy gap — 2026-10-12

Working repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-transport-accounting, pushed base fa17b3ecee48; Goal1 ACTIVE.
Integrated refinement/coarsening batches complete the actual size-8 three-event
sequence within the unchanged production 512-round allowance. Current v4 serial
outputs passed exact certificate equivalence plus fresh output/runtime checks.
The v4 MPI4/M2 strong/thin CGNS restart numerical test also passed; its comparison
collector failed with KeyError envelope_mesh_00001.su2.native_ref because that
collector assumes SU2 restart filenames. This was not evidence of numerical drift.
Fresh full independent audit of the saved current MPI4/M2 CGNS outputs now PASS
in engine_batch_cgns_audit_recovery_v1: eight mesh/readback frames, three current/
BDF state frames, 60 closed rank-stage rows. Current production source, binary,
68 actual test objects, libraries/thin members and saved case hashes were verified
before copying/auditing. Original failed v4 receipt remains untouched. No CFD
rerun, local timing campaign, criteria relaxation or speedup claim.

The shared CPoint empty/isolated-row root fix has passed focused bridge MPI1/2/4.
Final full current 104-case mixed MPI1/2/4 regression, lifecycle/MMG checks and
qualified milestone commit/push are still outstanding. Do not present v4 as an
aggregate PASS or Goal1 as complete. Last audit controller 54754 terminal PASS;
no owned heavy job remains live at this update.

User asks why 512 and agrees to progress-aware stopping with explicit work/time
ceilings. EngineOptions::maximum_rounds=512 is an internal safety allowance for
synchronized epochs, independently applied to Refine and Coarsen; not a mesh-size
limit or justified universal scaling bound. A round admits up to eight independent
candidates per worker; conflicts/rejections reduce successful edits. Existing
no-candidate stagnation handling is narrower than sustained residual progress.
After the final round, complete targets may pass; incomplete measurements, frozen
metric violations or pending coarsening candidates reject the entire event.
CSinglezoneDriver::ReplaceMesh calls CollectiveFailure on incomplete results:
accepted mesh/solution stay unmodified, but the production run stops collectively;
there is no implemented retry/skip-and-continue policy. Saved files are not an
automatically updated failure checkpoint.

Next substantive execution-policy work should distinguish useful residual progress
from persistent blockage and impose explicit whole-event work/time bounds, including
partition/import/return/rebuild costs. Preserve frozen target, strict final gates,
MPI collective agreement, bounded storage and atomic state replacement. Useful edit
counts alone do not prove residual progress. Do not merely raise 512, claim current
batching solves general scalability, or soften incomplete coarsening acceptance
without a separately reviewed policy. Repeated cluster cost/scaling/overhead,
weights/wait/memory qualification remains open. Goal2 has not started.

# MPI4 return/rebuild issue exposed and under repair —2026-10-12

Goal1 ACTIVE; integrated batching source under qualification on branch
codex/native-3d-transport-accounting, pushed basefa17b3ecee48. Actual size8default512
three-event driver and strong/thinN4M2CGNS-restart independent audits PASS in v2.
MPI1/2full103-case CPP plus bridge/eight independent record audits PASS in v3;
exactv2CPP/build reuse is pinned. v3MPI4 aborts in CFD bridge cycle1M2 when returned
coarsening leaves an empty CFD rank: CPoint::SetPoints calls Point.outerPtr()
although its CSR has no inner entries. This is a real rebuild issue, not metric
roundoff. Solver matrix initialization already accepts nPoint0. Current small
root fix constructs an empty Edge CSR from the same rows without asserting raw
entry pointers, preserving all nonempty behavior; added2D/3Dempty/isolated/reset
connectivity test. Focused bridge MPI1/2/4 next, then complete current regression.
No lower mesh criteria or artificial partition-size guard. Failed v1/v2/v3
receipts/raw states/snapshots preserved. No owned heavy job live at this edit;
v3controller37175terminalFAIL. Working repo SU2_NativeIntegrated, oneCPU/nice19/
j1/threads1/MPI<=4 correctness only. Core manifests103cases (104with runtime).
Cluster manifest candidate_batch controls now survive ENV cleanup;34core and
17comparison mocked checks PASS. Complete milestone not yet pushed/qualified;
repeated cost/scaling/overhead/weights/wait/memory gates remain. Goal2 not started.

# Integrated engine milestone v2 — actual size8 CPP sequence passes, audit live

Working repo /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-transport-accounting, pushed basefa17b3ecee48; Goal1 ACTIVE.
User focus is complete big steps, not further micro-optimization checkpoints.
New batch lookup + deduplicated payload fetch + per-candidate full-star reservation
is integrated into refinement and admissible coarsening, preserving existing
per-cavity frozen metric/geometry validation and atomic publication. Recovery
repair remains its existing single-candidate path. Internal candidate_batch1/8
controls both operators, same compiled binary, no new public CFD option.

Owned current controller session12646, inspect handle before treating as stopped:
engine_batch_milestone_v2/run.py. Actual size8N1M1three-event driver CPP PASS5078
assertions within production512epochs. Mesh2415->467->2542tets;132->590points at
last two events; solution transfer error<=3.94e-16. Refine/coarsen epochs58/60,
0/116,99/144; every successful phase zero cached-invalid and pending-edge counts.
Frozen targets/q>=.20/J>=.05/L<=1.8 unchanged; trajectories and later resampled P1
sources differ from the earlier single-candidate checkpoint. Exact independent
audit of new size8geometry/metric/marker/state/SU2CGNS records currently RUNNING,
not yet PASS; historical full audit takes~900seconds on1CPU. Do not claim whole
milestone qualification until current receipt passes. Next sequential stages:
N4M2strong/thinCGNS restart;103mixed-case MPI1/2/4 with bridge/eight independent
distributed audits; actual Hessian/CFD/BDF lifecycle and serialMMG.

Focused v1coarsening/metadata MPI1/2/4 PASS retained. Coarsening-only actual
milestone v1FAIL retained: second coarsen127epochs completed, third refine512
one-candidate rounds failed508accepted/990invalid/276pending; accepted state
retained. New sources/snapshots and failures kept; no duplicate build/binary.
SGE harness reused for same-binary candidate_batch controls;17mockedtests PASS.
Need honor manifest controls after core runner's ENV cleanup before publishing
the cluster harness (currently comparison correctness ENV would be cleared).
Do this after live v2controller terminates because core/run.py is pinned; validate
that runner-only change separately, keep numerical receipt exact. No cluster
submission, local performance campaign or speedup claim. OneCPU/nice19/j1/
threads1/MPI<=4 correctness only; foreign work untouched. Goal2 not started;
repeated cost/scaling/instrumentation/weight/wait/memory gates still open.

# Integrated batching: decisive driver evidence, extension to refinement —2026-10-12

Goal1 ACTIVE; full scope preserved. Focused lookup/fetch/reservation/coarsening
MPI1/2/4 PASS, real eight-cavity transaction96->48cells/8accepted in1epoch.
The actual default512size8v1 driver completed the formerly stalled second
coarsening in127epochs/714attempts/443accepted/271rejected, all475cached cells
valid/no pending directions. Third event then exposed the same one-candidate
budget in refinement:512attempts/508accepted/990invalid cached cells/276pending
edges; accepted CFD state retained. Whole v1 FAIL retained with exact source/
binary/object/archive pins, accepted meshes/current+BDF states and raw failure.
No speed claim: new accepted meshes/P1 later-event sources differ, and timings
are contended correctness diagnostics. Do not label v1 full milestone PASS.

Current code extends the same batch admission to refinement edge stars, retains
the existing private fallback/metric checks and individual atomic publication,
and keeps standalone/recovery repairs single-candidate. Bounded local vertex
index, complete remote reservation, unique per-slot IDs, byte budgets and
original sensor-only actual-query metric policy remain. Internal candidate_batch
1/8 compares both operators using the same compiled binary; no new public cfg.
Next actual complete milestone controller: engine_batch_milestone_v2/run.py;
qualify default512three-event driver, strong/thin N4M2CGNS restart and one broad
MPI1/2/4 regression with fresh independent audits, then push the integrated
result. Cluster SGE harness is prepared/17mockedchecks PASS; no submission.
OneCPU/nice19/j1/threads1/MPI<=4 correctness only. No owned numerical job live
at this edit; v1controller terminal FAIL. Working repo SU2_NativeIntegrated,
branch codex/native-3d-transport-accounting, basefa17b3ecee48. AdapNoExt and foreign
work untouched. Practical repeated performance/fullGoal1 remain open; no Goal2.

# Active milestone — complete batched refinement/coarsening engine (2026-10-12)

User asked to focus on complete large steps and avoid further micro-optimization
campaigns. Goal1 remains ACTIVE with its full original correctness/performance
scope. Current milestone integrates lookup + deduplicated fetch + per-candidate
full-star reservations into actual coarsening. Several disjoint cavities share
admission, then each retains existing strict frozen-target validation and atomic
publication. Refinement/invalid-field repair remain the existing path. Batch
rounds and actual edit work are counted separately;512epochs alone is no speed
certificate. Internal batch1 control supports paired work/correctness comparisons.
No new public config. Next: focused MPI1/2/4, the known default512size8 driver
campaign with independent geometry/metric/state/output audit, then one broad
regression and refreshed cluster comparison. No more separate API-only milestones
or further scalar query/cache tuning before assessing this integrated result.

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-transport-accounting; pushed base fa17b3ecee48.
Focused case folder: integration_evidence/native_3d_core_v1/engine_batch_focus_v1.
Source is currently under development, unqualified. OneCPU/nice19/j1/threads1,
MPI<=4 correctness only after actual host check; no high-rank or performance run.
Foreign ClusterResults/AdapNoExt/submodules untouched. Goal2 not started.

# Current handoff — 2026-10-12

## Batched dependency lookup qualified — 2026-10-12

Repository SU2_NativeIntegrated; branch codex/native-3d-transport-accounting;
base679fe68ef414. Goal1 ACTIVE after the user resumed it. LookupBatch now returns
separate bounded complete vertex/edge/face stars through the existing ticketed
exchanges; missing/oversized candidates refuse individually, common protocol/
memory failures refuse the batch. Shared identities checked; legacy union/wire/
collective semantics retained. Targeted MPI1/2/4 PASS; current102-case mixed
regression plus fresh bridge/seven distributed audits and serialMMG PASS.
199/331/511 prior numerical JSON records match exactly. Five equivalent lookups
use30vs6World wrapper calls, same640byte serialized total, not MPI/network/wait
or elapsed remeshing speed. New core manifest101cases (legacy97/frozen100 retained),
34package checks and actual registration PASS. BATCH_LOOKUP_VALIDATION.md/json
pin scope/source/68objects/binary/archives/tools and all evidence. Current full
case folders batch_lookup_regression_v1/mpi1,mpi2,mpi4; initial targeted v1/source
snapshots retained. All owned jobs terminal; oneCPU/nice19/j1/thread1/MPI<=4,
correctness only. About52.8MB, no retained build copies. Runtime C++ checks pass,
no independent runtime certificate claimed on mixed logs. Engine still one
proposal/worker/round: next batched payload/reservation then private multi-edit
engine integration. Default512 and repeated practical performance/fullGoal1
remain open. No Goal2/curved/3DBL/physical-unsteady claim. Foreign work untouched.

## Goal 1 resumed — batched dependency admission in progress

User reactivated Goal1 after pushed679fe68ef414551e754eb8bf71581b4bea4e0a74.
Working repository SU2_NativeIntegrated; branch codex/native-3d-transport-accounting.
Current code factors the existing ticketed lookup into common legacy/batch
validation. Batch candidates retain separate complete vertex/edge/face stars and
64-cell bounds; missing/oversized candidates refuse individually, malformed or
memory failures refuse the common batch. Legacy union semantics/wire/collective
sequence remain. Shared identities are checked across candidates. New targeted
MPI1/2/4 controls cover65independent features, legacy byte equivalence, mixed
logical refusals, geometry feature dimensions, empty peers and store preservation.
Current actual correctness controller: batch_lookup_v1/run.py, owned tool session8529;
inspect its live handle before treating it as stopped. Build/run not yet qualified.
OneCPU/nice19/j1/thread1/MPI<=4 after actual resource checks, no local performance
campaign. This is dependency foundation only: engine still proposes one edit per
worker/round, default512not fixed. Next: qualify broad regressions, then batched
payload/reservation and private engine proposals with bounded memory. Goal1 ACTIVE,
full gates unchanged; Goal2 not started. AdapNoExt/foreign ClusterResults untouched.

## Selected static findings fixed while Goal 1 is paused — 2026-10-12

Working repository SU2_NativeIntegrated; branch codex/native-3d-transport-accounting;
base 7746fd53fa2f. The user separately authorized fixes during the pause.
Global boundary validation no longer inherits cavity sphere/512 restrictions;
closed oriented manifold checks remain. Aggregate owner publication/retired-edge
scratch use byte bounds. Unused parent source/weights are skipped when no
repartition; collective controls are checked before conditional source calls.
Native3D docs and new100/legacy97 cluster guards are corrected.
101 mixed C++ cases per rank MPI1/2/4, fresh bridge and seven distributed audits,
serial MMG and 34 package checks pass. Actual core registration is100.
See integration_evidence/native_3d_core_v1/STATIC_ASSESSMENT_FIXES.md and
STATIC_ASSESSMENT_FIXES_VALIDATION.json for pins, exact scope and retained failures.
MPI1 v2 is reused; MPI2/4 v5. About53.5MB retained, no copied builds/binaries.
Runtime-only audit did not certify mixed logs; driver C++ checks pass and no
independent runtime certificate is claimed. All jobs terminal; oneCPU/nice19/j1/
thread1/MPI<=4 correctness only. Goal1 remains PAUSED, Goal2 not started.
No default512/high-rank/performance/curved/3DBL/physical-unsteady claim. Earlier
comparison packages are frozen oldsource and need refresh. AdapNoExt and foreign
ClusterResults untouched.

## Bounded persistent coarsening-witness replay — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting; starting tip1bfb136dbd. Production unchanged.
collapse_witness_replay_v1/v2 replay940source-matched historical full-star proposals
using prior exact original-P1 directional certificates. Full-star changes can
leave a specific bad chord obstruction intact. Bounded16/128entry,1/4witness
models preserve all709accepted proposals and test support/coordinate invalidation.
V1oracle learns from all rejected categories and identifies7/23rejects. V2learns
only recorded length failures because production rejects shape before integrating
fresh edges: directed adjacency7/19; shared endpoint-chord memo7/20, with2second-
event matches across different collapse directions. Cap16is sufficient for this
observed effect; more entries/witnesses do not improve it. No predicted492round
execution or512completion: historical geometry still266pending directions and
full615/1002tails absent. Sparse partial local-star counterexamples could support
future worker-local sensor-only memo, but conservative floating proof, bounded
learning, MPI ownership/lifetime, actual new trajectory, memory/lookups and cost
qualification remain. No CPP/cache/target/geometry/quality/budget/state/cluster
package change, new SU2/MPI, local performance campaign or submission. Original
trace and both oracle/stricter models/logs/source receipts retained in v2/REVIEW.md.
All owned work TERMINAL, oneCPU/nice19/thread1 exact offline correctness only.
Goal1 ACTIVE; repeated cluster/default512/performance/weight/wait/memory proof
still open; Goal2 not started, no3DBL/physicalunsteady/affordability certificate.

## Reservation-scheduler cluster comparison prepared — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Starting tip6b319ea519. New separate toolkit
integration_evidence/native_cluster_3d_reservation_comparison_v1 compares standard
and isolated rotating priority with fresh exact references per scheduler/N/M,
exact repeatability within each group and exact initial geometry/frozen target
across schedulers. Later P1 triangulations/accepted work may differ; operational
costs, not fixed-work speed. Existing donor comparison/production CPP unchanged.
SGE correctness then serializedN1/N2/N4 full/half workers, warm-up plus3paired
OFFON repeats, actual Hessian/CFD/BDF controls, complete parent/phase/rank/work
analysis and compact verified-repeat deduplication; optional mpiP stays separate.
Actual current68-object preparer smoke PASS: one private directory compile/link,
normal directory member excluded, unchanged source/options/objects/archives/bin,
closed regular-file bundles verified. Include-only change from qualified prototype.
No MPI/adaptation/performance/qsub run. First missing-relative-Ninja failure
preserved; wrapper now validates/resolves caller-relative tool before child cwd
changes/bundle creation.16synthetic orchestration checks and SGE syntax PASS.
PREPARATION_REPORT.md,PREPARATION_SMOKE_VALIDATION.json,selfcheck_v1 pin evidence.
Two terminal local binary copies removed after verification:1595519616bytes
freed; original normal binary and compact link/source/log/manifest/failure data
remain. Retained preparation_check_v2 is historical smoke evidence, NOT a
runnable cluster bundle; prepare fresh prepared_v1 on cluster per README.
All owned work TERMINAL, oneCPU/nice19/thread1 for compile/link after host checks.
No cluster request or submission overnight. Goal1 ACTIVE; actual cluster cost/
scaling/overhead/weight/wait/memory/default512proof still open. Prototype not
promoted; Goal2 not started; no3DBL/physicalunsteady/affordability certificate.

## Isolated rotating-policy stronger whole cases and lifecycle PASS — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Starting tip64fa4386409; standard production
source/binary/objects/archives unchanged. One shared isolated rotating-directory
build serves sequential aspect2 turning size4 N4M2, covariant H20 size4 N4M2,
actual CGNS persisted restart size2 N4M2, and six existing MPI1 metric/Hessian/
CFD/BDF OFFON lifecycle controls. All numerical and fresh exact independent
audits PASS:24whole geometry/metric frames,9current+BDF transfers,actual SU2CGNS
output/readback and CGNS driver/history restart. Sensor residuals0; maximum
whole-state error1.1284049824188437e-14,restart3.844148161277339e-16.
All accepted q>=.24666457/J>=.08654324/L<=1.799025804; strict L<=1.8 verified.
Eight H20 frames normalize exactly to the same saved unscaled isolated variant;
six serial lifecycle geometry/metric records equal standard controls exactly.
Strong aspect2 second-coarse attempts82/80vs130/64 and vetoes37/38vs0/92; rounds
119vs156. Changed accepted meshes are freshly audited, not fixed-work timing.
Small CGNS second-coarse20vs16rounds shows no uniform operation-count benefit.
check_rotating_whole.py and ROTATING_RESERVATION_WHOLE_MATRIX/VALIDATION records
retain complete source/tool/build/case pins and all work observations. Collector-
only legacy missing donor_box_filtered error retained; missing fields explicit,
no zero fabrication, numerical rerun or relaxed guard. All owned jobs TERMINAL;
oneCPU/nice19/thread1/MPI<=4 under actual host contention/resource checks. Shared
terminal executable/object removed after full PASS:806641624bytes freed; hashes,
recipes/linkmap/logs/meshes/states/VTUs/outputs/failure retained, receipts immutable.
No local performance campaign or cluster submission. Not promoted to production.
Still needed: default512large-case completion and repeated matched-target cost/
scaling/traffic/overhead/practical-memory/weight/wait qualification. Existing
fixed-work donor cluster campaign unchanged. Goal1 ACTIVE,Goal2 not started;
no curved/3DBL/physicalunsteady/affordability certificate.

## Isolated rotating-policy full97 regression qualified — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Starting tip8a2c7cf0e4; production remains
86bd7f7824 unchanged. rotating_reservation_full_build_v1 compiles only the exact
previously audited private directory variant and links all actual68test objects/
774members, with old directory member excluded. Shared isolated binary reused
for sequential rotating_reservation_full_mpi1/2/4_v1:97cases per rank PASS, all
seven distributed exact audits plus driver/current+BDF/SU2CGNS audit. Actual
bridge coverage12/24/48frames,6/12/24state pairs and6/12/24round trips. Existing
stale/version/veto/memory/identity/rollback/default, repeated cycles, movement/
reconnection, coplanar/automatic repairs, donor/query composition and subsets
pass. Native2D/MMG controls retained. Changed parallel trajectories freshly
independently audited; no byte-identity or timing comparison claimed.
check_rotating_full.py and ROTATING_RESERVATION_FULL_VALIDATION.md/json pin all
source/build/case/tool/coverage records and verify untouched production driver.
First collector-only len(integer round-trip count) error/source note retained;
correct count handling passed, numerical runs/audits never failed. All owned
work TERMINAL, oneCPU/nice19/j1/thread1/MPI<=4 after actual host/resource checks.
No local profiling or cluster submission. Shared terminal exe/object removed
only after full matrix:806641616bytes freed, immutable receipts/recipes/linkmap/
accepted/rejected/control geometry/state/readback data retained. Not promoted;
stronger aspect/thin/whole restart/lifecycle/default512 envelope and repeated
matched-target cost/scaling/traffic/overhead needed before rotating policy moves
into production. Prepared fixed-work donor campaign stillstandard unchanged.
FullGoal1 performance/weight/wait/memory/practical limits remain open; Goal1 ACTIVE,
Goal2 not started; no general/curved/3DBL/physicalunsteady/affordability certificate.

## Isolated rotating reservation protocol verified on MPI1/2/4 — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Starting evidence tip2c20503dce; production
still86bd7f7824 unchanged. rotating_reservation_protocol_v2 uses the exact prior
whole-mesh audited directory variant and supplementary actual Catch/MPI tests.
Three cases PASS per rank at MPI1/2/4:repeated shared complete stars rotate unique
winner over2Mcalls; disjoint rank-local complete stars all remain selectable;
priority-phase corruption on rank0 rejects all ranks and preserves both stores.
One-rank memory veto also rejects with byte-identical mesh/directory. Assertion
counts24(serial),45perrank(MPI2),77perrank(MPI4). Phase word checks modulo-M
priority, not arbitrary full counter equality. MPI1 mismatch case explicit NA.
Prototype test object replaces only original distributed-test object; original
Dir archive member not extracted.67other test objects/774members unchanged.
Production binary/config/source/header/objects untouched. v1 compiler error in
fixture vector row access happened before MPI; exact failed source/log/receipt
preserved. v2 fixesonly that access, protocol code same as v1 mesh probe.
All owned work TERMINAL, oneCPU/nice19/j1/thread1/MPI<=4 correctness under actual
host gates/FSI contention, no local profiling or cluster submission. Terminal
isolated objects/exe removed with full hashes/recipes/linkmaps retained:840748520
bytes freed. REVIEW.md and validation.json in v2 record scope/limitations.
This strengthens shared/disjoint and collective-refusal proof, but not full97case
rotating-policy, stale-publication/admission/worker-subset/strong/thin/restart/
production-limit or repeated cost/scaling qualification. Not promoted; prepared
fixed-work donor campaign stillisolated. Goal1 ACTIVE; Goal2 not started; full
performance/default512/practical-envelope gates remain open.

## Isolated rotating-reservation prototype passes one complete case — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Production remains86bd7f7824 unchanged.
rotating_reservation_probe_v1 compiles only a private directory variant and links
it ahead of the actual configured driver archives; link map proves original
Dir member not extracted. All production sources/68objects/774members unchanged.
Priority rotates with matching reservation-call epoch; claims carry/check one
extra epoch word through existing common vote, no new collective. Traffic grows
by that encoded word per vertex and must be accounted. This is not promoted.
Actual N4M2 size4 standard wedge, default512, unchanged initial config/mesh/P1target
and q/J/L/noise/facet/cap constraints:three replacements and fresh full exact
geometry/metric/current+BDF/SU2CGNS audits PASS, accepted tets289/51/287. Minq
.24666457185,J=.08654324081,Lmax=1.79577811683,state<=3.5936476e-16,tensor0.
Second coarsening attempts78/78 versus standard106/54; vetoes51/50 versus0/87;
rounds129versus141. Final51tets rather than48; later287rather than314. These
changed trajectories are independently certified, not fixed-work timing evidence.
comparison.json retains all work/communication counts and exact initial hashes.
No runtime epoch-mismatch injection, full MPI1/2/4/admission/stale/general safety
matrix or repeated cost/scaling qualification yet. Match lifecycle/common order
must remain explicit before production promotion; fairer counts need not mean
fewer seconds. CPPguard/weight/default/productionpriority unchanged. No cluster
submission or local profiling campaign; oneCPU/nice19/j1/MPI4/thread1 correctness
under actual host checks, all owned jobs TERMINAL. Reproducible isolated exe/object
removed after hashes/recipes/audits retained:806641560bytes freed. Sources/linkmap/
logs/initial+accepted grids/three VTUs/current+BDF states retained. Review and
complete receipts in rotating_reservation_probe_v1/REVIEW.md and validation.json.
Prepared fixed-work donor campaign remains isolated; prototype not substituted.
Goal1 ACTIVE; repeated performance/default512/practical-limit qualification open,
Goal2 not started. No general/curved/3DBL/fullunsteady/affordability certificate.

## Active partition balance and reservation bias — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Production unchanged from c407d8b9a4.
reservation_balance_review_v1 reparses three pinned box-prefilter N4M2 cases,
18operation events, with full original artifact/audit hashes and current unchanged
partition/directory/donor/remesher source hashes. Aspect2 cycle1 coarsening starts
with proxy weights959/979 and owner ADT candidates649111/644747, but attempts
130/64 and reservation vetoes0/92. Worker1 loses92of156rounds; source inspection
confirms shared vertices always elect the lowest caller rank. Requester donor
payloads39323/15163 differ from owner candidate scans. Counts do not establish
CPU cost, pure wait or scaling; initial weight balance does not remove this bias.
Profiler adds partition_active_load over M workers and proposal_outcomes with
veto/acceptance/unfinished-attempt fractions. Original N-rank measures retained;
zero inactive ranks cannot inflate apparent partition imbalance. No new C++ timer,
collective, config, weight or reservation-policy change. Legacy scope retained;
comparison validates raw logs and every old field, allowing only missing new
derived keys and emitting recomputed fields. Tampered counts/ratios still reject.
34core+15comparison checks PASS; actual saved logs pass full profile closure.
Two simplified mock-schema failures/sources retained, then corrected; no numerical
failure or new simulation. REVIEW.md,analysis.json,validation.json pin all evidence.
All owned work TERMINAL, lightweight oneCPU/nice19; no local performance or cluster
run. Fresh future bundles include this analysis; frozen older ones remain unchanged.
Next benchmark should distinguish rank-priority losses, owner predicate cost and
working-mesh imbalance before weight tuning or mid-remesh repartitioning. Rotating
consistent reservation priority is a possible separate changed-trajectory experiment,
not implemented here. Default512 completion and repeated cluster cost/scaling/
overhead/weight/wait/memory qualification remain open. Goal1 ACTIVE; Goal2 not started.

## Native3D completion diagnostics qualified — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Starting checkpoint85658a22f8. Goal1 ACTIVE;
Goal2 not started. Engine final reduction now distinguishes complete, valid cached
cells with pending coarsening, unsatisfied cached metric gates and missing cache
coverage. Same single integer MAX and exact old zero/nonzero validity result;
no budget/criteria/operator/source/BL/fade/noise/2D/MMG change or partial acceptance.
Existing rejected-refine and rejected-coarsen fixtures assert distinct global
causes and unchanged accepted inputs. completion_diagnostic_build_v1 PASS full
68-object/774-member driver, binary9811747b59cbb64dc91a95a53d8f4bb06d281249d95e2f608705a5681b8076ec.
completion_diagnostic_mpi1/2/4_v1 PASS97cases per rank, seven exact distributed
audits, actual driver/current+BDF/SU2CGNS bridge and serialMMG. Raw geometry/donor/
state JSON169/271/391files match prior box-filter controls exactly;8336Boolean
rank/mode combinations preserve completion truth. check_completion_diagnostics.py
and COMPLETION_DIAGNOSTIC_VALIDATION.md/json pin scope and all evidence.
Frozen comparison retains old four source variants and adds box_diagnostics;
full six-pin matching handles its shared donor hash and rejects mixed engine/test
versions. Ten supported forward pairs include original/routing/packed/box->new.
completion_diagnostic_package_checks_v1 PASS32core+14comparison checks. New current
candidate needs fresh rebuild/freeze after pull; older bundles untouched/supported.
All owned jobs TERMINAL, sequential oneCPU/nice19/j1/MPI<=4/thread1 under actual
host checks/FSI contention. No cluster submission or local performance campaign.
Default512 valid-but-pending coarsening still rejects; this explains that limit,
not a fix. Whole stronger/thin/n8/restart/lifecycle campaigns retain their earlier
source pins. Full repeated cluster cost/scaling/overhead/rank/weight/wait/memory
qualification and practical completion limits remain open before Goal1 completion.

## Production512 private mesh independently passes physical gates — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Starting checkpoint b34877d709; production
unchanged. coarsen512_private_audit_v1 independently audits the retained bb87
second-event private rejection:517tetrahedra/153points/222physical faces, exact
volume1/2, qmin=.20433538142016777,Jmin=.08825346092225352,Lmax=1.7946315664648058.
All exact embedding/incidence/skin/vertex-link/domain/facet/marker and original-P1
quality/full-edge gates pass. Actual input SU2 signature and frozen sensor/reference
are preserved; negative marker-assignment self-check passes. The original result
still FAILS queue completion:512attempts,437accepted/75rejected,0cached invalid
cells,266pending coarsening directions. It was never installed in CFD and has no
new solution/BDF/CGNS acceptance certificate. Do not relabel it as a completed run.
REVIEW.md,independent_audit.json,validation.json pin untouched original trace,
converter,existing auditors,source and inspectable *_rejected.su2/JSON files.
This is fresh independent proof for historical private data, not new current-source
512execution. Scope distinguishes physical validity from coarsening queue exhaustion.
No completion-policy/default/guard change; explicit partial-progress policy would
need separate specification and validation. Existing one-proposal/worker/round
budget has a mesh-size limit; improve useful proposals or ordering within its
contract rather than infer a failed geometry from this cap. OneCPU/nice19/thread1
under actual host checks/FSI contention; all owned jobs TERMINAL. Goal1 ACTIVE,
Goal2 not started; repeated cluster performance/weight/wait/memory/overhead and
production completion qualification remain open. No SU2/MPI/cluster launch.

## Current donor-prefilter stronger envelope qualified — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Production8c3b40c9be unchanged; starting
branch tip47480bbd415b240ee16c9b71bea138ff6332e2a0. Goal1 ACTIVE; Goal2 not started.
Predeclared box_filter_a2_direction_n4_n4_m2_v1 and serial n4_n1_m1_v1 both PASS
three actual driver adaptations with fresh full exact audits, accepted tets
402/71/435 and402/54/424. MPI minimum q=.2039974058/J=.05895219395 remain close
to frozen .20/.05 guards. box_filter_thin_h20_n4_n4_m2_v1 PASS289/48/314tets,
exact volume1/2097152, state residual<=4.6066651e-14; no actual BL claim.
All three use qualified box_filter_build_v2 binary c1bdef2a09d1e13bd1d137910db5713de32514f7a421369d6b1a177b48de54d6,
production512, original-P1 sensor-only targets, adaptable facets, noise0, zero
transported tensor residuals and actual SU2/CGNS/current+BDF replacement.
24geometry/donor frames and9state files exactly match older routing controls;
8H20 normalized frames additionally match current unscaled N4M2. Fresh audits
are not replaced by old certificate reuse. Actual box-filter rank-work counts
165180/22482/62070; no local repeated performance campaign or speedup claim.
BOX_FILTER_ENVELOPE_VALIDATION.md/json and check_box_filter_envelope.py pin all
recipes/source/receipts/audits and comparisons. Three inspectable VTU grids per
case retained; total data about7.1MB, no new binary or CFD series. All owned jobs
TERMINAL, oneCPU/nice19/MPI<=4/thread1 under actual observed FSI contention.
Size8 production512 completion and repeated cluster performance/rank/weight/
wait/traffic/memory/overhead qualification remain open. No cluster submission.

## Exact coarsening rejection classification — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Production checkpoint8c3b40c9be unchanged.
collapse_bound_replay_v1 verifies the retained bb87 trace and actual historical
CollapseVertex/ValidateMetricChange compiled-source pins. All940 proposed stars
remove cells;709accepted reconstructed topologies/coordinates match saved fresh
patches. All216combined no-removal/oversized messages therefore mean oversized
changed edges. Exact original-P1 directional tensor extrema certify76of156 and
47of75doomed proposals, with0accepted false rejections. A bounded12scalar tensor
component envelope certifies only35and24; negative/SPD-correlation adversary passes.
Seven second-epoch shape failures also have oversized edges;five have provably
length-safe changed edges and remain independently shape-blocked. Full exact
witnesses and compact provenance retained in collapse_bound_replay_v1/REVIEW.md,
analysis.json,component_analysis.json,validation.json.
Do not implement the weaker envelope from this replay: it does not demonstrate
512completion, MPI cost payback, safe rounded predicates or arbitrary composition.
No new SU2/MPI execution, production/default/target/guard change, local timing
campaign or cluster submission. Goal1 ACTIVE; repeated cluster performance/rank/
weight/wait/memory/overhead calibration remains required; Goal2 not started.

## Conservative donor-box prefilter qualified — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Goal1 ACTIVE; Goal2 not started.
Production known-query imports use strict projected-box separation before exact
segment/tet tests. Volatile subtraction/product/direction rounding and64eps sign
bound; wide long-double precision/exponent guard, otherwise retain exact path.
Touching/unresolved/point/full unknown queries preserve original behavior. No
metric/q/J/L/geometry/operator/cap/512default/index/collective change; stack-only.
SEGMENT_BOX_SIGN_REVIEW.md documents bound/preconditions; safeguardedv4 standalone
PASS7258rational oracle/reversal/coarse-grid/compiler-mode checks,0false rejections.
Intermediate buildv1 preserved; final box_filter_build_v2 PASS68objects/774members,
binarySHA256 c1bdef2a09d1e13bd1d137910db5713de32514f7a421369d6b1a177b48de54d6.
box_filter_mpi1/2/4_v1 PASS97cases per rank with all exact distributed and actual
bridge/state/SU2CGNS audits and serialMMG. Fresh box_filter_n4_n4_m2_v1 driver
audit PASS; box_filter_lifecycle_mpi1_v1 six actual Hessian/CFD/BDF OFF/ON controls
PASS. box_filter_n8_n1_m1_rounds1024_v1 PASS exact18artifact/22work-field equivalence
to packed control, fresh outputs/accounting; no rerun of identical full geometry
certificate.2344837exact donor tests avoided in second coarsening; totalcandidate/
filtered/payload/batch/retry/call/byte/work counts unchanged. Single nested filter
69.991->28.158s, engine89.308->44.339s; wholefixture126.659->78.545s diagnostic only,
no repeated speedup/scaling/overhead/allocator/wait qualification. No local timing campaign or cluster submission.

New donor_box_filtered counter flows source->engine->runtime/ENGINE logs; strict
integral subset aggregation checked on actual rows. box_filter_package_checks_v1
PASS32core+13comparison checks. SGE source pins now cover six files/four versions;
original/routing/packed->box pairs supported and earlier pairs retained. Baseline
7f76fd9a279cf384a37f8ae137dc4dc26212ce82 isolates the prefilter; fresh current
candidate bundle needed after pulling. Existing frozen bundles remain immutable.
BOX_FILTER_VALIDATION.md/json pins full evidence. All owned jobs TERMINAL.
Source2D/MMG preserved; production3DBL/composed-edge integration still explicitly
unsupported. Size8 production512limit and full repeated performance/rank-cost/
wait/traffic/allocator/RSS/overhead calibration remain open; no Goal2 activation.

## Donor broad-phase experiment and requirement audit — 2026-10-12

Production checkpoint7f76fd9a279cf384a37f8ae137dc4dc26212ce82 unchanged;
repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated, branch
codex/native-3d-transport-accounting. Goal1 ACTIVE; Goal2 not started.
Offline saved-phase review shows expensive donor predicates still dominate;
byte-unchanged stages/edge tracing slowed too, so no isolated packing regression
or speedup from two contended samples. PACKING_SAVED_PHASE_REVIEW.json pins logs.
Standalone segment_box_filter_experiment_v3 PASS3256 exact-rational closed-box
oracle cases in strict/productionfastmath/binary64-long-double modes,0false
rejections.28/158sampled actual n8 candidate boxes can be excluded; finite-corpus
and small sample, not full failed-cavity replay/production/MPI/speed proof.
Unresolved signs and touching remain exact-predicate candidates. v1 Python3.8
nextafter and v2 generated-newline compiler failures preserved. No production
code/config/target/guard change, numericalSU2/MPI run or timing campaign here.
SEGMENT_BOX_FILTER_EXPERIMENT.md has assumptions/next integration checks;
GOAL1_REQUIREMENT_AUDIT.md maps complete original deliverables to current proof
and explicit missing performance/weight/wait/memory calibration. BL composition
hook is tested but production3DBL/composed-edge integration explicitly unsupported.
All owned experiment jobs TERMINAL. Next meaningful local work is conservative
prefilter sign review/coarse-cavity observations and actual production qualification;
full repeated scaling/overhead/rank-cost/allocator calibration still needs cluster
measurements. Do not advance Goal2 or lower original acceptance/cap contracts.

## Grouped donor query requests qualified — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Goal1 ACTIVE; Goal2 not started.
New packing changes only CNativeDonor3D.cpp/hpp and its existing distributed
regression. One fixed-size record per cell/owner carries7query bits and actual
sender centroid in existing point-box fields; owner searches individual closed
boxes with the same predicates/order/union. Full unknown-private imports and
small-source path unchanged. No index/collective/metric/q/J/L/512 change; saturated
request admission models real grouped buffers, caps intact. CPP regression adds
non-dyadic endpoints and exact <=one owner record/root-only=one checks.
Build PASS68objects/774members; binarySHA256
c9cb94011938f532edccfbc683b92dd99d0e9f8a4276138227adb2056de4bdeb.
query_packing_mpi1/2/4_v1 PASS97cases per rank plus seven exact distributed suites,
actual driver/current+BDF/SU2CGNS and serialMMG controls. query_packing_n4_n4_m2_v1
fresh three-event audit PASS; query_packing_lifecycle_mpi1_v1 six actual Hessian/
CFD/BDF OFF/ON controls PASS. query_packing_n8_n1_m1_rounds1024_v1 PASS three events,
18exact geometry/state/reference/input artifacts match certified routing control;
fresh actual outputs/current accounting and22physical/query work fields checked.
Second coarsening tracked bytes180480816->162969840 (17510976saved,9.70%); donor
candidates/filter/payload/batches/retries/calls/work unchanged. Includes self encoded
traffic, not wire. Native admitted peak bound unchanged. Single contended fixture
clock113.328->126.659s is slower here; no net speed benefit/regression/scaling claim.
Cluster repeats must measure CPU/communication tradeoff. No local timing campaign.

QUERY_PACKING_VALIDATION.md/json pins scope/receipts/counters; original case logs,
meshes/states/VTUs retained. All owned numerical jobs TERMINAL. SGE prepare now
pins four source files (including guards/header/tests), permits only original->
routing, original->packed and routing->packed known pairs; latter isolates packing.
Existing older routing bundles stay supported/immutable. Source variants stored
in comparison manifest. query_packing_package_checks_v1 PASS31core+13comparison
checks, including header mutation/reversed-pair refusal. No actual comparison
bundle prepared or cluster submission. Fresh current bundle needed after pulling.
Size8 production512limit and full repeated performance/wait/traffic/allocator/RSS/
overhead/rank-limit calibration stay open. No3DBL/curved/fullunsteady/affordability
certificate and no Goal2 activation.

## Expanded distributed correctness and stronger metric matrix — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Goal1 ACTIVE; Goal2 not started.
Current qualified query-routing binary unchanged. The reusable73-case selection
omitted24existing NativeDistributed3D cases:21directory/donor/engine controls
and3partition controls. New97-case selection PASS per rank on MPI1/2/4, with all
seven reused exact distributed-record audits plus driver geometry/current+BDF/
SU2CGNS bridge audits and serialMMG controls. Successful receipts:
query_routing_complete_mpi1_v2, query_routing_complete_mpi2_v1,
query_routing_complete_mpi4_v1. COMPLETE_SELECTION_VALIDATION.md/json document
scope/pins; ~29.8MB successful case records retained, no copied binary/objects.
Frame counts include initial/private rejected progress, explicitly not all strict
accepted meshes. prepare.py closes the added existing auditors; run.py gates
count97 and all seven audit PASS statuses.31core/12comparison checks PASS in
complete_selection_package_checks_v3; original mock-label and94-vs97count
failures/sources retained. All owned numerical jobs TERMINAL. No C++/binary/target/
production512 changes, local timing campaign or cluster submission.

Stronger predeclared size4 aspect2 spatial-strength/turning-direction wedge
matrix also PASS: query_routing_a2_direction_n4_n1_m1_v1 and
query_routing_a2_direction_n4_n4_m2_v1; three events plus eight geometry/metric/
actual readback frames and three current/BDF pairs each. Serial402/54/424tets,
MPI402/71/435tets; independent certificates, no cross-partition byte claim.
MPI minimum q=.20399740583061948/J=.0589521939546055 are near unchanged .20/.05
bounds; Lmax<1.8, tensor transport0, state residual<3.78e-16. Full report/pins and
inspectable VTUs in QUERY_ROUTING_DIRECTION_A2_VALIDATION.md/json and named cases.

Fresh candidate cluster bundles now use97cases; old frozen baseline72/73recipe
counts remain their historical scope. Two-source compiled routing comparison
remains the same four-file change; new shared tools/README preserve each side
own frozen correctness runner. Size8 production512completion and repeated full
performance/overhead/wait/traffic/allocator/RSS/rank-limit calibration stay open.
Do not advance Goal2 from expanded correctness alone; no3DBL/curved/fullunsteady/
affordability certificate.

## Current-source thin-domain matrix completed — 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Goal1 ACTIVE, Goal2 not started.
query_routing_thin_h10_n4_n4_m2_v1, query_routing_thin_h20_n4_n1_m1_v1
and query_routing_thin_h20_n4_n4_m2_v1 all PASS three-event production512
correctness with qualified query-routing binary bb5a7664dda805daac6bbcc4b16e3da3ee3759f33475937beedd6a177c35955e.
Fresh full independent audits: eight frames and three current/BDF pairs per case,
actual SU2/CGNS readbacks, exact domain/markers/no-overlap/conformity and unchanged
q>=.20/J>=.05/L<=1.8. Tensor transport error0; maximum state relative error
4.606665037477689e-14 in H20 N4M2, within the unchanged audit bound.
Sixteen exact normalized geometry/donor comparisons match unscaled N4M2 control;
physical affine states audited separately. Three inspectable VTUs per case.
Full report/receipts/checker in native_3d_core_v1/QUERY_ROUTING_THIN_VALIDATION.md,
QUERY_ROUTING_THIN_VALIDATION.json and check_query_routing_thin.py.
New physical thin inputs are covariantly scaled, not unscaled byte-identical
controls; no C++/target/round changes. All three owned jobs TERMINAL; about6.5MB
case artifacts retained. No local timing/scaling campaign or cluster submission.
Size8 production512 completion and full repeated performance/overhead/traffic/
wait/allocator/rank-limit calibration remain open. Thin-domain covariance is not
3DBL construction. Do not advance to Goal2 from this bounded correctness result.

## SGE query-routing comparison prepared — 2026-10-12

Working repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-transport-accounting. Production routing checkpoint7b4e0d767fbda42ff17064ed93960316d794d5ab
is pushed. Goal1 ACTIVE, Goal2 not started. New reusable tools in
integration_evidence/native_cluster_3d_query_comparison_v1 prepare regular closed
copies of existing baseline/candidate core bundles, then serialize both-source
correctness and N1/2/4 paired timing jobs with M=N/M=max(1,N/2). Baseline is
2d3ac1e3a552a1b90a87dfa3595fc1b372783ceb, not old2D e6995fb. Preparation enforces
expected donor-source hashes, exact four-file compiled source difference and
matching build options except prefix. Do not replace baseline/candidate binaries
or edit frozen tools. Existing core bundles keep their own72/73-case recipes.

Default hotspot: size8/diagnostic1024, production512 unchanged; optional size4/
512control. One warm-up per side and three paired detailOFF/ON repetitions,
alternating order. Actual metric/Hessian/CFD/BDF lifecycle controls precede each
rank campaign. First N/M reference gets a full independent audit; later exact
frame/state equality plus fresh output/accounting checks permits reuse. A changed
trajectory stops fixed-work claims and retains evidence. One compressed full
reference per N/M; matching repetitions keep logs/configs/environment/rank profiles/
checksums/provenance and remove duplicate meshes/states only after validation.
Failure outputs remain. No ClusterRaw/binaries needed for downloaded analysis.

Optional --mpi-profile appends separate three-rank-choice profilerOFF/ON jobs,
detail alwaysON; both bundles need the same MPI-compatible mpiP library/license.
Existing MPI API/report parser is reused, raw reports retained and verified.
API elapsed and wrapper bytes are not pure wait/wire traffic. Normal detail
and profiler overhead are separate. Analyst re-parses actual current logs and
requires3paired samples, constant accepted work, artifact identities and spread.
Different N/M trajectories remain separately certified, not fixed-work scaling.

selfcheck_v1 PASS12mocked/lightweight checks, real saved n8N1M1/n4N4M2 runtime
log parsing and actual pre/post source-pair receipts. Original harness import/path,
profile-mock and standalone stage-initialization failures retained with source/logs;
fixed before publication. Shell syntax PASS. These are TOOL checks, not new solver
or performance evidence. No actual baseline/candidate comparison bundle prepared,
cluster job submitted or local timing campaign launched. All owned numerical
jobs remain terminal. FullGoal1 performance/traffic/wait/allocator/rank-limit
calibration and the production512larger-case limit remain open. Use the new
README's explicit baseline/candidate preparation and launch commands when the
user can run on cluster. Do not ask for submissions while user is away.

## Narrow actual-query donor routing — qualified checkpoint, 2026-10-12

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated, branch
codex/native-3d-transport-accounting, source changes based on2d3ac1e3a552a1b90a87dfa3595fc1b372783ceb.
Goal1 ACTIVE; Goal2 not started. Known-query imports on sources above256donors
now route actual centroid/closed-edge boxes separately and exact-test only the
specified query. Sender's rounded centroid is transmitted in the point box;
full-cell/unknown-query imports and all original-P1/BL/target/q/J/geometry/512
contracts unchanged. Existing fixed-size Request uses a checked one-byte kind;
no extra source index/collectives. Saturated admission covers7requests per cell.
No2D/MMG/config change. Geometry-timer comment covers selection/seed/update.

Actual full68-object/774-member build PASS; binarySHA256
bb5a7664dda805daac6bbcc4b16e3da3ee3759f33475937beedd6a177c35955e.
query_routing_mpi1_v1/mpi2_v1/mpi4_v1 PASS73cases per rank, including exhaustive
384tet conforming closed-footprint reference, cube/rotatedthin, distributed/empty
owners, remote query, P1/full-edge and atomic invalid/memory rejection. Independent
12/24/48frames,6/12/24state pairs and6/12/24actualSU2CGNS round trips; serialMMG
PASS48assertions/two cases. Fresh lifecycle six actual OFF/ON controls, n4N4M2
three-event adaptation and persisted SU2N1M1/CGNSN4M2 restart checks PASS with
fresh independent audits.31lightweight package checks and all actual counter/
notification/nested phase audits PASS. All owned runners are TERMINAL.

query_routing_n8_n1_m1_v1 stays FAIL at512, identical437accepted/75rejected,
517valid cached cells/266pending directions. Its first accepted mesh/state/
next-input matches baseline bytes and has a scoped cycle0 certificate/VTU.
query_routing_n8_n1_m1_rounds1024_v1 PASS all three events,2489/457/2245tets,
615second-event coarsening attempts and1002third-event attempts. Exactly18current
frames/states/reference/input artifacts match the fully independently certified
prior case; five actualSU2CGNS files and current phase/rank/work counters checked
fresh.22physical/query-work fields match. This reuses the existing full geometry/
metric/state certificate, not a new899s geometry run. New equivalence auditor's
negative controls reject altered BDF data and a reversed actual SU2 tetrahedron.
No relaxed threshold or target. Reference certificate's adversarial/math evidence
is retained; current fresh corruption controls are separately recorded.

One-CPU contended diagnostic: second failed coarsening donor phase67.845->37.963s;
ADT candidates4545418->8472788, filtered1698110->2862395, same347287payloads,
2572batches/121fallbacks/34040tracked calls. Tracked worker bytes130179440->141355136.
One query per candidate replaces testing up to seven; request/candidate work and
traffic grow. These are pilot diagnostics, not repeated speedup/scaling or MPI
wire-byte claims. Current1024fixture113.328s vs previous165.837s also remains a
single contended comparison. No local profiling campaign was launched.

NEXT: publish checkpoint, prepare a fresh closed73-case SGE bundle and matched
baseline/candidate repeated controls for donor CPU versus extra traffic, total
remesh/return/rebuild/transfer, detailOFF/ON, M=N/M<N, memory/wait/API calibration
and practical size/rank limits. Do not advance Goal2 before fullGoal1 performance
and completion audit. Production512default remains the larger-case limit. No
cluster job requested/launched tonight. See QUERY_ROUTING_VALIDATION.md.

## Larger1024-round diagnostic independently PASS — 2026-10-12

Working repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated, branch
codex/native-3d-transport-accounting. Production filter source is pushed at
bb87e97fe1d89ee69ae8deac8fb447d0996f2d5e; its rejection diagnosis atff9f47ae41.
Goal1 ACTIVE; Goal2 not started. full_star_facet_n8_n1_m1_rounds1024_v1 is now
TERMINAL PASS: all three numerical events plus8strict/input/readback frames,
3current+BDF state frames,13closed rank-stage rows and actualSU2/CGNS checks.
Meshes2489/457/2245tets,590/133/542points; second coarsening615attempts,
457accepted/158rejected, third refinement790attempts and coarsening1002attempts
(379accepted/623rejected). No invalid cached cells/pending queues/repairs.
Independent q/J minima .511339/.288666, .204335/.082768, .239746/.090104;
maximum lengths1.799855/1.794632/1.799088, all exact domain volumes1/2.
Current/BDF residual<=3.899327321487061e-16, transported tensor error0 including
SU2/CGNS readbacks. Actual meshes and envelope_s0/s1/s2.vtu retained.

ONLY the separate fixture budget changed; production512default remains and its
matched n8 event1 FAIL is retained. Config/input/first sensor bytes match. This
proves feasibility of the frozen case with more work, not a production budget
fix, speedup, scaling or fullGoal1. Large third-event rejection cost remains.
Numerical fixture165.837s, independent external audit898.664s, VTU export.220s;
no timeout. Audit is external verification, not adaptation cost. All owned
numerical/audit/trace sessions are terminal. Next: implement/equivalence-check
narrower actual-query donor routing after reviewing the documented memory/MPI/
closed-contact constraints; keep full-cell imports for unknown future queries.
Preserve matched512failure and1024success as both correctness/control fixtures.
Repeated cluster performance, overhead and memory/wait/traffic calibration remain
open. No cluster job requested/launched tonight. See longer-budget REVIEW.md and
DONOR_QUERY_ROUTING_INVESTIGATION.md. Rebuild a fresh72-case bundle after updating.

## Published filter and source-matched remaining rejection diagnosis — 2026-10-12

Pushed production checkpoint bb87e97fe1d89ee69ae8deac8fb447d0996f2d5e on
codex/native-3d-transport-accounting. full_star_facet_trace_n8_v1 is terminal:
reproduction/comparison PASS, actual adaptation FAIL. Eight identical input/
accepted/state snapshots and364equal work/communication/cache/queue integers.
Second event has60size/length and15shape/sliver rejections, zero facet failures;
no identical complete-star repeat rejections,38after changed supports. Current
private517tet grid coarsen_rejected_s1.su2 retained, without a second-event
independent certificate. Only terminal owned diagnostic object/executable removed
(808946688bytes); full pins, recipe, trace and grids remain. See TRACE_REVIEW.md.

Live next case is full_star_facet_n8_n1_m1_rounds1024_v1: separate longer-budget
diagnostic, production512default and matched512FAIL unchanged. Initial cfg/mesh/
sensor and all numerical target/acceptance rules frozen; only the documented
test budget changes. Poll actual session42396/runner/terminal receipt before
restarting or making source changes. One owned nice19 one-CPU numerical job.

The separate1024-round diagnostic has now completed all three numerical events:
2489 ->457 ->2245accepted tetrahedra. Second-event coarsening took615attempts
(457accepted/158rejected); third-event refinement790attempts and coarsening1002.
No cached invalid cells/pending directions at these exits. Its strict independent
whole-case audit is LIVE, so this is not yet a whole-case PASS. Production512
remains unchanged and its failure retained. Keep source pins stable until the
actual runner/audit is terminal; session42396 is still the authoritative handle.

Existing failed-case counters locate donor import as67.84of85.10coarsening
seconds, with121private-query fallbacks. Contended diagnostics, not performance
comparison. DONOR_QUERY_ROUTING_INVESTIGATION.md describes narrower actual-query
routing and required equivalence/admission tests; no optimization implemented yet.
Source pins must stay stable during the live diagnostic. Goal1 ACTIVE, Goal2
not started, no cluster launch/request tonight.

## Full local-star facet filtering — qualified checkpoint, 2026-10-12

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; changes based on pushed8807b27d8d.
Goal1 remains ACTIVE; Goal2 has not started. Production/source/test changes are
qualified for publication. The shared LeavesFacet predicate now checks cached
edge cells and surviving indexed local removed-star cells before exact orientation
filtering. Invalid fields retain private repair; missing local witnesses retain
complete distributed import/private validation. No extra index/collective,
metric/geometry/q/J/length change or512-round budget change. No2D/MMG change.

full_star_facet_build_v1 PASS actual68-object/774-member MPI/MMG/CGNS driver;
binary SHA256 d99ccd8e4e4d0477b8badc1e3453a6ac063f579b0d65f1a92950420820dde0c5.
full_star_facet_mpi1_v1/mpi2_v1/mpi4_v1 PASS72cases per rank and independent
12/24/48mesh frames,6/12/24current+BDF pairs and6/12/24SU2/CGNS round trips.
SerialMMG PASS48assertions/two cases. Fresh actual lifecycle six OFF/ON controls,
n4N4M2 three-event adaptation and persisted SU2N1M1/CGNSN4M2 restart campaigns
PASS with independent audits. All owned numerical/audit runners are TERMINAL.
Only one low-priority numerical job at a time, MPI<=4 on one CPU; these are
contended correctness checks, not performance/scaling evidence.

Unchanged frozen full_star_facet_n8_n1_m1_v1 remains FAIL at cycle1. All five
baseline files match. First adaptation:428collapse attempts/272accepted/
156rejected,2489tets/590points. Six actual first-mesh/state/reference/next-input
artifacts are byte-identical to the prior independently certified first event;
first_cycle_equivalence_audit.json verifies that certificate and reruns correct
sensor/current+BDF/actualSU2/accounting checks, without repeating full geometry.
Transported tensor error0,state relative residual3.899327321487061e-16. Actual
SU2 and envelope_s0.vtu retained. This is a cycle0-only certificate.

Second coarsening:512attempts,437accepted/75rejected,517valid cached private
cells,266pending directions,0repairs,1890orientation exclusions/1007reactivations.
Previously429accepted/83rejected/545cells/323pending. The last failure is edge
180,181: no cell removal or oversized edge. It does not classify all75rejections.
First accepted CFD mesh/current+BDF/reference protected; new private cycle1
mesh not yet exported or independently certified. No full-campaign success.

full_star_facet_package_checks_v1 PASS31lightweight checks plus actual42/123/363
unit ENGINE rows and six lifecycle/24n4/sixSU2restart/24CGNSrestart/fourfailed-n8
runtime rows. Finite integer geometry/orientation/change counters, parent-bounded
nested costs and globally closed change records pass. Original collector FAIL
is retained: nonroot Catch summaries interrupted two final MPI4 ENGINE rows.
Explicit exact summary-text removal restores every numeric byte; raw/recovered
logs, removed spans, failed recipe/execution and a recovery self-check retained.
Numerical PASS/audits were unaffected. Do not hide or skip malformed rows.

NEXT: publish this checkpoint, then source-matched trace of the new75rejections/
266pending directions and export its private rejected grid. Keep original target,
quality/geometry/rejection guarantees and512rounds for matched diagnosis. Repeated
cluster timings/scaling, instrumentation overhead, allocator/wait/traffic
calibration and practical limits remain Goal1 gates. No cluster job requested or
launched tonight. Rebuild/prepare a fresh72-case bundle from the pushed branch.
See integration_evidence/native_3d_core_v1/FULL_STAR_FACET_VALIDATION.md.

## Current-source second-coarsening trace and independent first-event audit — 2026-10-11

Code checkpoint c743c12784e6cd09452874887e8b7b3d10c4e837 is pushed on
codex/native-3d-transport-accounting. Working repository:
/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated. Goal1 remains ACTIVE; Goal2
has not started. The first n8 event is now accepted and independently audited
(first_cycle_audit_v2.json, including recovered pinned completed geometry/field
stages from v1 before its wrong-sensor metadata assertion). Actual transported
original-P1 tensor error0,current/BDF residual3.899327321487061e-16. Its actual
SU2 and envelope_s0.vtu mesh/state export are retained. The v1 wrapper FAIL and
exporter missing-argument failure are preserved; no tolerance or CFD rerun.
The full campaign still FAILS at second coarsening; no successful-case audit is
fabricated from this partial result.

coarsen_changes_trace_n8_v1 is TERMINAL: isolated engine with exactly three
hooks, all68actual configured test objects and unchanged production code/build.
Its expected adaptation FAIL byte-matches eight input/accepted/state snapshots
and364integer work/communication/queue counters. Second event:512attempts,
429accepted/83rejected;49size/length,23finite reference-facet,11shape/sliver
rejections. No identical complete-star repeat rejections;42after changed
complete dependencies. Saved545-cell private grid coarsen_rejected_s1.su2 is
inspectable diagnostic output, not accepted CFD or independently certified field.

Exact rational authoritative planar-group containment confirms all23facet
rejections have witnesses in the complete removed-vertex star but NONE in the
candidate edge cells. Selection's cached Forbidden check only reaches edge cells;
private CollapseVertex correctly checks the full star and rejects. This is a
specific incomplete EARLY FILTER, not a bad committed mesh. NEXT: reuse the same
exact facet predicate across the indexed local removed-star cells, retaining
complete dependencies, remote-owner fallback, atomic rejection and invalid-field
repair. Add an adversarial MPI check before rebuilding/retesting; do not infer
that23saved attempts alone complete the remaining323directions. No new production
change for this second finding yet. Classify length/quality obstructions separately.

Own isolated trace object/executable removed after terminal successful replay/
comparison/witness audits; disposable_cleanup.json records hashes and actual
bytes. Production build retained. All owned numerical/audit/trace sessions are
terminal, no owned heavy process remains. Cluster jobs were not requested or
launched tonight. Rebuild/prepare a fresh71-case bundle on the cluster after
updating the branch; repeated performance/calibration remain Goal1 gates.

## Complete-star retry notifications and exact collapse filtering — 2026-10-11

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; source changes based on b49f089b43.
Goal1 remains ACTIVE. Goal2 is not authorized to start before all Goal1 gates pass.

Implemented bounded authoritative changed-vertex notifications to old/new star
owners, a local owned-cell vertex index and directed blocked-collapse index, and
exact zero/negative surviving-cell orientation filtering. After successful atomic
publication, affected blocked directions are reactivated even when their edge
cells did not change. Rejection preserves owned/directory state and notifications.
The filter is disabled for invalid fields so private repair still runs. Missing
local witnesses still require full distributed dependencies/private validation.
No acceptance/metric/geometry/round-budget changes; no 2D/MMG/config changes.

Actual complete68-object/774-member MPI/MMG/CGNS build PASS in
coarsen_changes_build_v1. Binary SHA256:
ed708d6b70e89e36e6403974009c36b0d9b00ec5456db0a52a314cb5cbc74907.
coarsen_changes_mpi1_v1/mpi2_v1/mpi4_v1 PASS71cases per rank, with independent
12/24/48 mesh frames,6/12/24 current+BDF pairs and actual SU2/CGNS round trips.
Serial MMG2D/3D PASS48assertions/two cases. Complete remote-star, unchanged-edge,
rank-veto/budget rejection/mode-mismatch and exact-zero-volume tests are included.
coarsen_changes_lifecycle_mpi1_v1 PASS six actual OFF/ON metric/Hessian/CFD
replacement controls, with original-P1/geometry/BDF/output/exclusive accounting.
coarsen_changes_n4_n4_m2_v1 and both coarsen_changes_restart_* folders PASS three
adaptation events with independent exact geometry, metric, state/output audits.
Restarts are SU2N1M1 and CGNSN4M2. Contended one-CPU correctness, no speedup claim.

Frozen n8 coarsen_changes_n8_n1_m1_v1 remains FAIL, now at cycle1. All five original
parameter/environment/config/mesh/sensor snapshots match the previous rejection.
Cycle0 now completes: refine457accepted; coarsen428attempts/272accepted/156rejected,
295exact orientation exclusions,120reactivations;2489cells/590points, no pending
edges or cached invalid cells. Coordinate connectivity and marker comparison
confirms its accepted actual SU2 grid equals the old rejected private grid,
ignoring numbering. Independent cycle0 geometry/original-P1 q/J/integrated-edge
and actual SU2 checks completed; the v1 wrapper failed at the wrong default
sensor metadata comparison. first_cycle_audit_v2.json recovers those pinned
completed stages and passes the correct rotating-sensor/transported metric/BDF
checks: tensor error0,maximum state relative residual3.899327321487061e-16.
The v1 failure and recipe remain intact; no tolerance changed or CFD rerun.
The full campaign remains FAIL. Subsequent coarsen_changes_trace_n8_v1 was an isolated current-source trace with
three engine hooks, separate epochs/private exports and no production mutation;
see the terminal result above before launching further work. Published code checkpoint c743c12784e6cd09452874887e8b7b3d10c4e837.

Cycle1 coarsening exhausts512rounds:429accepted/83rejected,545private cells, all
545cached valid,323pending directions;1883orientation exclusions/988reactivations.
The accepted cycle0 CFD mesh/solution is retained. Last proposal failed frozen
q/J (.16126001018/.04295752091); this does not classify all83rejections. No new
private cycle1 mesh export or whole-mesh certification yet. Goal1 still needs
second-event larger completion/diagnosis, broader limits and repeated cluster
performance, instrumentation overhead and allocator/wait/traffic calibration.

Reusable SGE selection updated from69to71cases;31lightweight package/parser tests
PASS. coarsen_changes_package_checks_v1 audits actual41/121/359 unit ENGINE rows,
six lifecycle rows and four failed n8 runtime rows: finite integral new work
counters, nested timers inside selection/publication, and globally closed sent/
received notification records. Two collector-only failures (row-name assumption,
relative-path serialization) are retained separately; C++/MPI tests did not fail.
Old terminal receipts keep their original auditor pins; use this checkpoint's
accounting collector for the added fields. Rebuild and prepare a NEW closed SGE
bundle after updating; no cluster job launched and no request tonight.

See integration_evidence/native_3d_core_v1/COARSENING_CHANGES_VALIDATION.md.
The historical sections below describe their own earlier source checkpoints.

## Coarsening dependency gap and actual n8 rejection trace — 2026-10-11

Working repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-transport-accounting, predecessor06d0a859d9.
Production/test/build/tool code and qualified driver remain unchanged from d509.
Goal1 is ACTIVE. The user authorized starting Goal2 only after all Goal1 gates
pass; cluster runs are deferred until tomorrow. Do not call Goal1 complete from
these local experiments or proceed to Goal2 with missing performance evidence.

A source-matched selector/kernel reproduction now confirms a real full-star
retry gap. In coarsen_dependency_replay_v4, the original spatial P1 sensor has
xx=7 at interior vertex4 and0.25 diagonal tensors at all other original nodes.
Collapse4->0 fails the frozen J>=.05 gate withJ=.048765984909417068. A valid
neighboring face2->3 reconnection changes the removed vertex's complete star
while leaving the candidate edge star/coordinates/measurement unchanged. The
same collapse then passes (global3-cell result q=.68723,J=.40825), but the current
selector leaves it blocked. Independent exact geometry/overlap/skin/volume,
centroid metric and analytic original-P1 edge checks PASS; the target and gates
are unchanged. This is a local selection/kernel defect, not a distributed fix
or an accepted whole-engine campaign. Small inspectable SU2 grids are retained.

coarsen_trace_n8_v1 replays the actual frozen n8 failure using all68 configured
test objects and exactly three diagnostic-only engine insertions in an isolated
object. Production build stays untouched. All170 compared integer workload
counters and five baseline input/config/state files match. Adaptation remains
FAIL:512 attempts/272 accepted/240 rejected,2489 private cells/91 pending edges.
There are231 unique rejected directions, zero identical full-star repeated
rejections and nine repeats after changed dependencies. The trace shows120 missed
reactivation events affecting53 blocked directions when their removed-vertex
stars change without touching candidate edges; it does not prove any of those
n8 directions became admissible or that fixing invalidation completes n8.

Independent exact rational witnesses confirm183 geometric rejections:176 zero
and seven negative replacement determinants; all183 interior-point positive
controls pass.56 other rejections introduce oversized edges; one leaves an
incident finite reference facet. This points to a safe early orientation filter
in addition to complete dependency invalidation, not an unchanged-retry cache.
Any filter must retain the invalid-field repair path, use current exact local
counterexamples, and invalidate across all changed removed-vertex dependencies,
including remote edits. Charge index/notification work and memory explicitly.
A local absence of a counterexample cannot certify a complete distributed star.

refine_private.su2 and coarsen_rejected.su2 are retained in coarsen_trace_n8_v1
for inspection (3552/2489 tetrahedra). They are private diagnostic exports, not
accepted CFD/CGNS outputs or independently certified full mesh/field results.
The original failed campaign and accepted CFD/BDF rejection checks remain.
Full study: integration_evidence/native_3d_core_v1/COARSENING_DEPENDENCY_REVIEW.md.
Next: bounded full-star invalidation and conservative early geometry filtering,
MPI1/2/4 atomic/stale/cross-rank tests, then unchanged-target larger campaigns
and measured complete cost. No local performance/scaling claim.

## Disk-quota troubleshooting moved to a separate handoff — 2026-10-11

See [BEEGFS_QUOTA_HANDBOOK.md](BEEGFS_QUOTA_HANDBOOK.md) for the full evidence,
confirmed actual build path, primary references, unresolved diagnosis and a
new-session prompt. The user requested this topic be handled separately from
adaptation. Current cluster build completion remains unverified. Do not resume
quota troubleshooting as part of the adaptation goal unless the user asks.

Coarsening source inspection: rejected directions are erased from the queue;
cell Drop/Add can requeue incident edges after edits. There is no demonstrated
unchanged-proposal retry defect. A directed collapse admits the complete removed
vertex star, while retry invalidation currently follows incident edge changes;
changes elsewhere in that star need a specific adversarial MPI test before a
cache/queue fix. Any new early orientation filter must use exact counterexamples,
track complete dependencies and charge index/communication memory and cost.
Do not replace full-star checks with edge-only admission or weaken frozen gates.
Goal1 remains active; n8 bounded completion and measured performance remain open.

## Termination diagnostics and first n8 size limit — 2026-10-11

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; predecessor
5b9e453c28d42806e012aae54beda324b67c3513. Core remains
6b3fefd039bfc3d1937b8add7d21d0ea41344bc9; AdapNoExt is untouched.

Five constant-cost local counters now distinguish cached invalid cells from
unfinished candidate work on every engine exit. They are emitted in existing
NATIVE3D_DETAIL rows. No operator, guard, target, budget or queue policy changed;
no production loop/collective was added. C++ tests and the independent lifecycle
parser cover successful/rejected exits and corrupted counters. Historical rows
without these fields remain readable but provide no termination-state evidence.

Actual full68-object/774-member configured MPI/MMG/CGNS build PASS:
termination_build_v1, binary SHA256
29b583f1bdd17a54051c0c66f997683b7f20f3c410042dfb5402b640bfe34a76.
termination_mpi1_v1, termination_mpi2_v1 and termination_mpi4_v1 each PASS69 cases
per rank. Independent bridge audits PASS12/24/48 frames,6/12/24 current+BDF state
pairs and6/12/24 actual SU2/CGNS output/readback checks. Serial MMG2D/3D PASS48
assertions/two cases. package_checks_v19 PASS30 lightweight parser/mocked tests,
Python3.8 AST and Bash syntax. All source/object/archive/tool pins remain stable.
termination_lifecycle_mpi1_v1 also PASS: six actual metric/Hessian OFF/ON
controls remesh/replace geometry/solver/BDF and write meshes. Its independent
audit checks six successful engine rows with all new counters. Larger accepted
restart and MPI2/4 lifecycle evidence remain from the preceding checkpoint.

First n8 wedge N1M1 practical-limit pilot and diagnostic rerun both FAIL in
cycle0. Input405 vertices/1536 tetrahedra/512 boundary triangles; frozen targets
64/16/64, aspect1.5/spatial variation0.125, q>=0.20/J>=0.05/L<=1.8,512 rounds and
noise0 unchanged. Refinement457 accepted edits ->3552 private tetrahedra.
Coarsening512 attempts/272 accepted/240 rejected ->2489 private tetrahedra,
2489 cached measurements, zero cached invalid cells,91 queued candidates and
zero pending repairs. Last obstruction: edge642,297, cell74251. The queue is
required to be empty for completion, so the whole event rejects at the budget.
C++ current/BDF/reference/no-callback preservation checks pass before the single
failing COMPLETE assertion. No independent accepted-mesh audit/later cycle ran,
and no rejected private mesh was exported. Cached validity is not certification.

Preserved original FAIL folders transport_limit_n8_n1_m1_v1/v2 and PASS diagnostic
review termination_limit_review_v1 under integration_evidence/native_3d_core_v1.
The review checks receipts/hashes, identical input/config/source fields and 160
equal integer work/communication counters. Input envelope_input.su2 and
envelope_s0_initial.json are inspectable. Do not soften guards or increase rounds
to relabel these campaigns. Next: bounded/scalable coarsening completion and
blocked-proposal handling, followed by independent workload validation.

Cluster MMG lib64 discovery now works in the supplied log. Its static archive
needs SCOTCH from /global-scratch/bulk_pool/aero/utils_gcc10/SCOTCH_INSTALL;
README now includes mmg_scotch_root and runtime lib64. Cluster SU2 compile/link
and campaigns remain unverified; no qsub was submitted. User quota output shows
47.94GiB free, while shared aero flash-pool headroom is only about133MiB;
a transient group exceedance plus enforcement refresh is a plausible explanation,
not a confirmed Meson-caused quota incident.

Goal1 remains ACTIVE. Repeated cluster performance/instrumentation overhead,
larger accepted envelope, full MPI wait/traffic/vendor/allocator calibration
remain open. Sequential one-CPU nice19/thread1 checks are correctness evidence,
not sustained speedup/scaling/affordability. No owned numerical/build job is live.

## Larger frozen-work/restart qualification and MMG lib64 fix — 2026-10-11

Working repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting; predecessor
5a258c8325383eda1fd7e96d2235dba365c42277. AdapNoExt is untouched. Production
C++/test fixtures remain identical to475820f366366f471e0cdb50e362d7c5e572c328;
core branch remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.

Two actual n4 frozen-work cases (N1M1 and N4M2) and persisted SU2 N1M1/CGNS N4M2
restart cases now PASS. Strict unchanged analyze_reference_admission.py verifies
26 byte-identical mesh/source/current+BDF/readback JSON files against the
same-rank baselines and245 closed phase rows. Independent geometry, original-P1,
current/BDF, output/reload and corruption controls pass. Targets and q/J/L guards
are unchanged. These sequential one-CPU contended correctness runs qualify the
larger/restart gates; they do not establish scaling or sustained speedup.

Evidence in integration_evidence/native_3d_core_v1:
- reference_admission_n4_n1_m1_v1 and reference_admission_n4_n4_m2_v1
- reference_admission_restart_su2_n2_n1_m1_v1
- reference_admission_restart_cgns_n2_n4_m2_v1
- reference_admission_regression_v1 (aggregates actual configured MPI1/2/4 and
  serialMMG logs/pins, without rerunning; retains original serial collector FAIL
  and the separately verified successful postprocessing receipt)
- REFERENCE_ADMISSION_VALIDATION.json and reference_admission_rank_phases.csv

Those numerical checks completed before the following build-discovery-only
change. The cluster MMG prefix contains include/mmg/libmmg.h and lib64/libmmg.a;
SU2 previously searched only lib. meson.build now searches lib and lib64 for
combined MMG and explicit SCOTCH prefixes. Required-library checks and the
existing MMG2D/3D compile/link guard are retained. No numerical code/option change.
mmg_lib64_discovery_v2 PASS: exact production dependency block, real static MMG
and SCOTCH copies under lib and lib64, plus empty-MMG-archive rejection. v1 logs
retain the missing-Ninja/collector-path failure; v2 uses the existing actual
local Ninja and resolves the collector path. No full SU2 rebuild or cluster
execution is claimed for this discovery patch.

Cluster: pull this checkpoint on codex/native-3d-transport-accounting. Keep
mmg_root=/home/aero/arausa/global/Libs/mmg-5.8.0/install (do not append lib64).
The old Meson KeyError for options comes from a saved cmd_line.txt missing its
options section: preserve that directory and setup a fresh timestamped build
with existing compiler/MPI/additional flags and --prefix="$(pwd)" if desired.
Build UnitTests/test_driver, then freeze a fresh prepared bundle using that build
directory. README contains preparation/submission commands. Actual cluster
compile/link and campaigns remain unverified; no qsub was submitted here.

Goal1 remains ACTIVE. Current MPI1/2/4 and larger frozen-work/persisted-restart
numerical gates are qualified. Remaining: repeated cluster performance and
instrumentation overhead, n8 practical limits, full MPI wait/traffic/vendor and
allocator calibration. No curved geometry, robust3D BL, full unsteady CFD or
CFD affordability claim. No owned numerical/probe process remains live.

## Contention-authorized MPI correctness and cluster audit gate — 2026-10-11

Working branch: codex/native-3d-transport-accounting, predecessor
ca700f7d686804cf7df7d8eaade73bfa65562e95. Production C++/test fixtures/build
options remain identical to475820f366366f471e0cdb50e362d7c5e572c328; core branch
remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. No numerical guard/target change.

The user explicitly authorizes correctness-only MPI runs despite CPU contention.
Retain actual host observations, available-memory admission, one owned sequential
job, bounded CPU affinity, nice19/yield/thread1; defer local performance claims.
This instruction overrides the previous CPU-idle gate for correctness checks.
Original resource-refusal evidence stays intact. The shared execution contract
now records this distinction. No foreign process was stopped or modified.

Full configured68-object native/reference/2D+3Dbridge selection PASS at MPI2 and
MPI4,69 cases per rank, using the same pinned serial475820 production binary and
774 archive members. Independent bridge audits PASS: MPI2,24frames/12statepairs/
12actual SU2CGNS round trips; MPI4,48frames/24statepairs/24round trips. Both cover
all M=1..N, two solution-transfer policies and three refine/coarsen/refine events,
exact embedding/volume/markers, original-P1 metric gates and current/BDF states;
each independent audit rejects its five corrupted controls. All source/object/
archive/tool pins unchanged; rejected/accepted transport totals close on each
rank. These are one-CPU contended correctness runs, not timing/scaling evidence.

Retained case folders:
integration_evidence/native_3d_core_v1/transport_mpi2_correctness_v2
integration_evidence/native_3d_core_v1/transport_mpi4_correctness_v2
Inspect bridge_barycentric_m*_s*_written_SU2.su2 and matching CGNS, source/state
JSON, independent_audit.json, transport_rows.json and validation.json.

The SGE correctness runner previously executed the bridge assertions but omitted
the existing independent audit. It now requires audit_geometry_bridge.py to
PASS after each MPI1/2/4 selection before progressing. Failed audits preserve
logs and stop later cases/campaigns. Package checks_v18 PASS29 lightweight
mocked/parser controls including the injected MPI2 audit failure. Python3.8 AST,
Bash syntax, scoped whitespace and unchanged production/fixture/build options
PASS. This script-only update needs no solver rebuild; prepare a fresh bundle
after pulling. Never modify an already frozen/submitted bundle. Existing jobs
remain useful, but their bridge outputs need separate independent review.
The README also explains that --prefix controls installation; the bundle copies
the build-directory test driver and requires no local /tmp files.

Goal1 remains ACTIVE. Current small MPI1/2/4 regression is now qualified; larger
n4 fixed-work/persisted-restart qualification, n8 practical limits, repeated
cluster performance/instrumentation overhead and full wait/traffic/vendor/allocator
calibration remain open. No curved geometry,3DBL/full unsteady or CFD affordability
claim. No owned process is live after both terminal PASS results; no qsub was
submitted by this session and no downloaded native3D campaign is present.
Do not merge the isolated transport branch into core before the larger gates.

MPI2 maximum current/BDF relative error: 6.0360605402784081e-13; MPI4: 8.471971137321857e-13. Transported-tensor maximum error:0 in both.

Overnight operating instruction (2026-10-10): the user is unavailable to launch
cluster jobs. Do not ask for launches tonight. Prepare closed scripts/evidence
collection if needed and report pending commands when asked tomorrow. Continue
small sequential local checks with real host-contention checks; push each
validated checkpoint and verify the remote SHA. A terminal capacity failure
cannot be guaranteed to auto-resume; preserve the handoff for recovery.

## Updated cluster batch authorized — 2026-10-11

User can now launch cluster runs and asks for exact commands. Use updated branch
codex/native-3d-transport-accounting, current code/tool checkpointdbd93a2dc26f56a233ebefa8ab494991ef52a14a;
core remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. Build the complete68-object
normal driver with MPI/MMG/CGNS/tests, MMG prefix
/home/aero/arausa/global/Libs/mmg-5.8.0/install and preserved library paths.
Freeze a fresh timestamp-named bundle, then submit the seven serialized default
SGE campaigns plus --limits (eighth n8 practical-limit job). No mpiP dependency
is needed for this batch. Correctness/lifecycle PASS gates precede timing/limits;
submission does not establish success. Do not alter old/running bundles.
The optional deep-MPI campaigns remain separate until an MPI-compatible mpiP
library plus license and real loading checks are available.

transport_mpi2_onecpu_v1 is TERMINAL: fresh one-CPU gate refused BEFORE launch
(load1=8.20 >=7 and selectedCPU3 minimum idle0.395 <0.50). Both MPI ranks would
have shared one actual CPU at nice19/thread1/yield; total CPU allocation was
reduced rather than claiming two idle CPUs. Original two-CPU refusal is retained.
All build/source/binary/68object/774member/tool pins remain unchanged. Only three
host-observation subprocesses ran; no solver/MPI/audit launched. No local timing
or new numerical result. This folder contains runner/gate/provenance/refusal,
not a mesh generated by a successful run.

No owned process is live; no qsub submitted by this session. Until actual cluster
job state/results are available, user launch intent is not a verified wait.
Goal1 ACTIVE with the original scope. Current MPI2/4, larger frozen and persisted
restart qualification, repeated performance/size limits, full wait/traffic/vendor/
allocator calibration remain open. Handoff/findings/evidence preserved; AdapNoExt
and foreign work untouched. Case outputs will be announced by the existing runner
under ClusterResults/native3d_<job>_<mode>_n<ranks>, with launcher logs and compact
mesh/source/current+BDF/audit artifacts only.

## Checked verbose MPI report analysis — 2026-10-11

NativeIntegrated / codex/native-3d-transport-accounting; predecessor
7b3c607b111ad1472acfe08137277c5af5457e79. No production C++/fixture/target/guard
change. Core remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.

Fresh actual-host idle samples0.1607031/0.1132666/0.0343965 and load1=7.33
refused any numerical MPI launch this turn. Four foreign Python ranks each
used approximately99% CPU. No owned heavy process or qsub was started; do not
interpret intent/state files as a live cluster wait.

The optional profiler runner now requires a complete verbose report. Shared
mpi_profile.py checks all rank assignments and times, declared stack/rank-record
counts, exact callsite API-count closure and reported time closure with explicit
display-precision bounds. The top-twenty table is never used as a total.
Legitimate M<N callsites on only the worker ranks are accepted. Source stacks
stay attached; no inferred phase/window/communicator identity. Analysis exports
mpi_api_profiles, whole-process per-rank totals, per-function/callsite rank work,
observed wrapped-API counts and MPI-time max/mean. These are not pure wait,
wire traffic, MPI work inside a particular remesh window or complete API coverage.
The print threshold is explicitly0; report generation and parent overhead remain
separate paired measurements. Normal campaigns remain unchanged.

package_checks_v16 passes26 preliminary lightweight checks. Review caught a
generator-consumption issue in accumulated display bounds; corrected before
publication with a dedicated control. Final package_checks_v17 passes28
mocked/parser controls (1.499s unittest,2.074s wrapper), AST/Bash/diff and
production/fixture/build-option equality to475820. It retains a labeled synthetic
four-rank report/summary and pinned upstream format source identity8a2ae9e9458.
No actual mpiP sample is available in that upstream tree. These checks do not
validate actual loading, MPI2/4 numerical behavior or profiling cost.

Goal1 remains ACTIVE: current MPI2/4, n4/persisted-restart qualification, repeated
cluster overhead/accepted-work/scaling, n8 practical limits and full wait/vendor/
allocator calibration remain open. Retain original resource/collector failures.
Do not merge the isolated transport branch into core before MPI qualification.
User core checkout, AdapNoExt, foreign submodules/results and old frozen bundles
are untouched.

## Optional calibrated MPI campaign preparation — 2026-10-11

NativeIntegrated / codex/native-3d-transport-accounting; predecessor
78f6800a153812c05fbf239cdac5a1d4271faebd. No production C++, numerical fixture,
target or guard change. Core remains the user's6b3fefd039bfc3d1937b8add7d21d0ea41344bc9
cluster checkpoint. Do not merge the transport branch before MPI qualification.

transport_mpi2_v1 was refused BEFORE MPI launch: three idle samples included
0.2993068683 below the frozen0.30 gate, load1=6.41. No rank started. This is
resource-admission evidence, not a numerical failure or a live wait. A later
host sample remained busy; no repeat/local timing/cluster job was launched.
Original refusal validation and host evidence are retained.

Optional mpiP support freezes a shared library plus matching complete license
inside a new closed bundle. Three appended SGE campaigns are serialized and
gated by same-package correctness and N1/2/4 lifecycle PASS. Application-only
preload leaves the launcher/auditors untouched. Native detail remains ON; each
M=N/M<N choice has a warm-up and three alternating profiler OFF/ON pairs and
unchanged exact mesh/source/current+BDF/readback hashes. Missing/incomplete
all-rank reports fail and preserve uncompressed evidence. Offline analysis
distinguishes parent overhead from process overhead including report finalization.

package_checks_v13 preserves the initial mocked collector failure (empty recipe
directory); v14 passed22 checks after correction. Final-source package_checks_v15
passes23 checks (0.962s unittest,1.290s wrapper), AST/Bash/whitespace and
production/fixture/build-option equality to475820. Actual mpiP library
loading, dependency compatibility, callsite attribution and overhead remain
unverified; this host lacks libunwind/BFD development files. No dependency was
installed, no binary rebuilt, no profiler/MPI/CFD/qsub launched.
mpiP API statistics do not measure pure waiting, wire traffic or chronological
phase windows; full traffic/wait/vendor/allocator calibration remains open.

Goal1 ACTIVE. Pending current MPI2/4, frozen n4 and persisted-restart qualification,
n8 practical limits and repeated cluster evidence. No curved geometry/3DBL/full
unsteady/CFD affordability claim. New package README has deferred commands; keep
the user's existing core checkout and frozen/submitted bundles unchanged.

## MPI coverage lower bound and actual serial lifecycle — 2026-10-11; checked

Evidence/tools checkpoint a2f8b2f1c7c13b9b45b3221d3ee6bb87b364b6ea pushed to origin/codex/native-3d-transport-accounting; remote SHA verified. Production remains475820f366. Core remote remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9.

NativeIntegrated / codex/native-3d-transport-accounting; predecessor7398f47f076e19c7ff80f57f4044a3ea4a14bb06.
No production/test-fixture/target/run-package change. Core remains6b3fefd039b.
Source-pinned coverage audit of40 logged engine invocations proves at least
25984 collective API calls:16573 wrapper calls plus9411 directory elections.
At least36.22% of known collective-call count is absent from the wrapper counter;
this is not elapsed-time attribution, network traffic or a scaling result.
Successful failure elections already have a one-reduction fast return.
Three lightweight counter/schema/corruption controls pass. Ledger and limits:
integration_evidence/native_3d_core_v1/MPI_COVERAGE_REPORT.md.

transport_lifecycle_mpi1_v1 PASS:462 assertions in existing case; six actual
metric/Hessian/remesh/repartition/rebuild/current+BDF/output controls, independently
audited6frames/21closed wallCPU stage records/21nested records and complete
current transport scopes. Same built binary/source as475820f366, all pins
reverified. One nice19/thread1 MPI rank after fresh idle/load gate; no concurrent
owned job. Existing fixture repetitions are correctness controls on this busy
host, not a local performance certificate. No CFD time integration or MPI2/4.

Actual case/result folder:
integration_evidence/native_3d_core_v1/transport_lifecycle_mpi1_v1.
Inspect lifecycle_m1_d{0,1}_r{0,1,2}_mesh_00001.su2 and matching .json/.cfg/
.native_ref plus lifecycle_audit.json and rank_profiles.json.

FullGoal1 ACTIVE. Still need MPI2/4/M<N/frozen n4/persisted restart, repeated
cluster and n8 practical limits, plus calibrated full MPI/vendor/wait/memory
coverage. No owned job live or qsub submitted. Never change a running core bundle
or merge this separate feature into core before broader qualification.
AdapNoExt, foreign submodules/ClusterResults and original failure evidence untouched.

## Isolated native3D transport accounting — 2026-10-11; built and serial checked

Implementation/evidence checkpoint475820f366366f471e0cdb50e362d7c5e572c328 pushed to origin/codex/native-3d-transport-accounting; full remote SHA verified. origin/codex/native-3d-core remains6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. Subsequent publication-note commit changes documentation only.

Working branch codex/native-3d-transport-accounting starts at pushed core
6b3fefd039bfc3d1937b8add7d21d0ea41344bc9. codex/native-3d-core remains the user's
cluster checkpoint. Three disjoint import/parent/worker World-wrapper totals
capture attempted work on ordinary rejection, including previously omitted
separate-import traffic. Bridge/engine detail reports per-call deltas; directory-
election API counters are separate nested data. No new runtime clock, MPI call,
query or numerical operator change. Direct/vendor MPI and actual waiting still
need calibration. Shared audits check closure and label legacy coverage.

transport_build_v1 PASS:14 actual nice19/j1 configured build steps, full68 objects/
774 thin members. transport_serial_v1 native10189assertions/69cases and serial
MMG48/2cases PASS; independent bridge12frames/6states/6SU2CGNS round trips PASS.
The collector then failed on a missing helper sys.path: preserve that FAIL receipt.
transport_serial_audit_v2 recovers by checking the same retained results/pins
without numerical rerun, SERIAL_AUDIT_PASS.26 non-CGNS fixtures byte-identical to
prior serial evidence; CGNS binary containers differ but exact signatures/readers
pass. Max state error2.7000623958883732e-15. package_checks_v12 is the final-source
19-check lightweight repetition PASS (0.480s unittest /0.778s wrapper).
Report: integration_evidence/native_3d_core_v1/TRANSPORT_ACCOUNTING_REPORT.md.

Current native result folder:
integration_evidence/native_3d_core_v1/transport_serial_v1/native_serial.
Includes default_native3d.su2 and bridge_barycentric_m1_s*_written_SU2.su2 /
_written_CGNS.cgns with corresponding original-P1/current+BDF audit data.
These are small manufactured correctness cases, not practical CFD grids.

The feature matrix preserves all original numerical guards/gates and requires
current transport records. MPI2/4/n4/persisted-restart qualification and repeated
cluster/n8/memory calibration remain pending. FullGoal1 ACTIVE. No owned job is
live; no local timing campaign or cluster job was launched. AdapNoExt, foreign
submodule/results and original failure evidence remain untouched.
Do not merge this feature into core before broader MPI qualification.

## Cluster MMG prefix and SGE runtime search paths — 2026-10-11; package checked

NativeIntegrated / codex/native-3d-core, predecessor1ba501f9dfc97b17688186cf56cca0c49f152818.
User build-native failed locating libmmg with default mmg_root=/usr/local. Older
downloaded582358/581950 metadata has enable-mmg=false. User supplied cluster
prefix /home/aero/arausa/global/Libs/mmg-5.8.0/install; README now configures it
explicitly and exports its lib search path. submit.py now sends PATH and defined
LD_LIBRARY_PATH via qsub -v names, preserving compiler/MPI/Python/shared-library
paths into the existing GCC-prepending SGE script without full environment export.
Freeze a new bundle after updating if the previous one contains old submission code.

package_checks_v10 passes18 lightweight checks, including actual Bash launcher
with a fake Python probe, mocked scheduler env/serialization/gates/partial receipt,
plus syntax/diff/source/artifact checks. No build/MPI/CFD/qsub launched; remote
MMG install/header/library/dependencies/link/load unverified. Production C++,
configured driver, targets/guards and auditors unchanged. Report:
integration_evidence/native_cluster_3d_core_v1/CLUSTER_MMG_SETUP_REPORT.md.

Goal1 ACTIVE; actual MPI/reference-admission qualification, repeated cluster
performance/n8 limits and full traffic/wait/memory calibration remain. Communication
coverage tracing is incomplete: World statistics omit failure elections/direct
passive/ADT/communicator/vendor calls. Do not infer MPI waiting from wall-minus-CPU.
No owned jobs live; AdapNoExt, foreign submodule work/results and prior evidence
untouched. No MMG-disable workaround or numerical PASS claimed.

## Native3D offline phase/rank analysis — 2026-10-11; checked, analysis only

Published analysis/evidence checkpoint 3e55caee966eef795f60b317a24ba7d948a999a9 on origin/codex/native-3d-core; full remote SHA verified. No executable or run-recipe change.

NativeIntegrated / codex/native-3d-core; predecessor ffeeac545762fb95b116fe47c797f3747e8c0d6f.
The analyzer now retains lifecycle OFF/ON timings and wall/CPU stage rows, plus
closed critical-rank phase breakdowns, active-worker CPU/work/query imbalance
and separately labelled nested detail for timing/limits/lifecycle. M<N reports a
separate closed active-worker row so inactive subset waiting is not mistaken for
the expensive worker phase. Unknown worker count stays null; no CPU-derived MPI
waiting estimate. Preparation includes the helper; C++/driver/runner/auditors and
targets unchanged, no rebuild/new cluster submission needed for analysis alone.

package_checks_v9 passes16 lightweight checks plus syntax/diff checks;
profile_analysis_replay_v2 verifies two pinned historical binaries/logs and
summarizes21 native,76 exclusive stage and84 nested events, preserving old audits.
Historical N4M2 coarsening:8.5271s native versus7.3171s active-worker coarsening;
private metric CPU max/mean1.2571. Contended pilot, no current speedup or wait-cause
claim. Report: integration_evidence/native_cluster_3d_core_v1/PROFILE_ANALYSIS_REPORT.md.

Goal1 ACTIVE. MPI2/4/reference-admission fixed-work/restart qualification, repeated
SGE timings/n8 limits and full privateCPU/wait/traffic/allocator/vendor calibration
remain. User received the seven-job plus optional eighth --limits launch commands.
No owned build/MPI/CFD/qsub jobs live; foreign ClusterResults/submodule work and
AdapNoExt untouched. Preserve already frozen packages and use current checkout
for offline analysis rather than changing a running package.

## Native3D query accounting — 2026-10-11; built and serial checked

Implementation/evidence checkpoint 542ba6375219cb84ddc4c7b8ab1bc02e42fe4a3b pushed to origin/codex/native-3d-core; full remote SHA verified. Serial checks pass; MPI and broader cluster qualification remain pending.

NativeIntegrated / codex/native-3d-core, predecessor9fddd081a8435e83a7f069043608d2b1f68ac437.
FIELD_ACCOUNTING_DRAFT_REPORT.md supersedes the initial uncompiled status below.
reference_admission_build_v2 passed its measured host gate (idle27.33%,load1=6.76)
and completed19 nice19/j1 build steps, complete68-object configured driver with
source/binary/object pins. reference_admission_serial_v1 passes serialMMG48 plus
all69 selected native/reference/2D+3Dbridge cases and9996 assertions, including
512-facet reference/300-facet CFD import/reload/budget refusal and retained failed
engine/weight/reader queries with timingOFFON and accepted/output preservation.
One owned job/rank at a time, threads1, scratch on4TB; both jobs terminal.

Native engine/bridge detail now exports actual cache misses, invalid queries,
failures and evictions. Local guards/after-catch aggregation preserve rejected
query work, without changing original-P1 interpolation, geometric composition,
mesh operators/targets/guards or2D/MMG implementations. Shared current auditors
check complete request closure and bounds; historical incomplete logs use their
saved original tools. package_checks_v6 passes12 lightweight checks and syntax;
source/artifact pins verified. initial resource refusal preserved in build_v1.

MPI2/4 and independent fixed-work n4/persisted-restart qualification remain
pending; last FULL MPI-qualified production is1fafe9e83c. REFERENCE_ADMISSION_MATRIX
freezes15 exact source changes and the original69/full68/26-byte-identical gates.
FullGoal1 ACTIVE, no larger/3DBL/curved/fullunsteady/PDE/affordability claim.

The user asked about cluster work: package is ready for their existing on-cluster
normal MPI/MMG/CGNS build of current branch, then prepare.py and submit.py
--limits. Default7 serialized correctness/lifecycle/timing jobs; optional eighth
n8 (405points/1536tets/512facets) first-limit job covers N1M1,N2M2,N4M2,N4M4 and
stops on first failure. Nodes need no Git; retain/download native3d_* and launcher
only. No actual bundle or qsub was launched here. See package README for commands.

## Native3D field accounting draft — 2026-10-11; C++ NOT VALIDATED

NativeIntegrated / codex/native-3d-core, predecessor9fddd081a8435e83a7f069043608d2b1f68ac437.
FIELD_ACCOUNTING_DRAFT_REPORT.md records missing engine misses/invalid/failure/
eviction exports and lost query work on thrown selection/bridge/private checks.
The draft aggregates actual FrozenField counters on failure via local stack
guards/after-catch collection, preserving the existing interpolation/composition,
mesh operators, targets/guards and2D/MMG sources. No new queries/cache allocations/
collectives. Existing selected tests now inject engine/weight/reader composition
failures with detailOFFON and verify counters plus accepted/output preservation;
these C++ assertions remain uncompiled and NOT RUN.

Shared current lifecycle/envelope/cluster audits now require complete counter
closure/bounds; old logs use their saved original auditors. package_checks_v6
passes12 lightweight fake/mock/synthetic-counter checks plus Python/shell syntax
in0.303s. Source/artifact hashes verified; no native numerical PASS claim.

REFERENCE_ADMISSION_MATRIX now freezes15 exact intended source differences against
both retained reference-lookup baselines, including accounting/auditors and prior
n8 fixture/runner. Its n4 fixed-work/n2 actual persisted-restart targets,69 selected
cases/rank,full68 objects and26 byte-identical output gates remain unchanged.

reference_admission_build_v1 retains source snapshots and a terminal preflight
FAIL due resource refusal, not a C++ compile error: idle27.33% but one-minute load
above8; no compiler PID or compile stage, sources/artifacts verified unchanged.
No owned numerical/build job remains. Scratch stayed on4TB. NEXT new contention-
checked build folder, then configured reference_admission_regression_v1 and
frozen fixed-work/restart qualification. Last validated production1fafe9e83c;
fullGoal1 ACTIVE, actual overhead/traffic/privateCPU/wait/memory/larger-size and
cluster limits remain. Foreign workers/results,AdapNoExt and earlier evidence
untouched; do not treat draft publication as numerical validation.

## Larger native3D practical-limit preparation — 2026-10-11; not a numerical result

Implementation/evidence checkpoint 6565e5575e5b4e00f4f938434d05040227a1d4a7 pushed to origin/codex/native-3d-core; full remote SHA verified. C++/MPI validation remains pending.

NativeIntegrated / codex/native-3d-core, predecessor6db495a38bc. Prepared optional
n8 wedge first-limit campaign:405 points/1536 tets/512 surface triangles, dyadic
coordinates, unchanged rotated/spatial original-P1 target64/16/64 and guards.
Only existing test fixture/runner/package/docs change; no production source change.
submit.py --limits appends one gated four-slot job after the default seven jobs,
with sequential N1M1,N2M2,N4M2,N4M4 and stop on first failure. Case parameters and
relevant environment are now retained before launch, including rejected runs.
Single-pilot summaries retain elapsed/accepted work/imbalance/bounds/RSS, without
fixed-work cross-partition or repeated-timing claims.

PRACTICAL_LIMIT_PREPARATION_REPORT.md and package_checks_v5 retain11 passing
lightweight orchestration checks plus Python/shell syntax; initial mock-recipe
failure is preserved in package_checks_v4. No C++ build, MPI/CFD, freeze or qsub.
Real host sampled0-1% idle, foreign FSI/conservation workers active; no owned heavy
job started. Scratch stayed on4TB. Measured /tmp/native*9MB;9.3GiB metric build is
the other agent's worktree,2.2GiB wall build another session; no foreign cleanup.

NEXT: when host permits, run reference_admission_regression_v1 and the unchanged
REFERENCE_ADMISSION_MATRIX fixed-work/restart cases, qualify and publish actual
numerical evidence; then current bundle freeze and user-authorized cluster launch.
C++ reference-admission/n8 fixture remain uncompiled. Last validated production
1fafe9e83c; fullGoal1 ACTIVE. Larger adaptation, detailed traffic/cache/privateCPU/
wait/memory calibration and disabled-detail overhead remain unproven.

## Initial native3D SGE package — 2026-10-11; orchestration checked only

Package implementation/evidence checkpoint b8add069d4009706fa297a48125bcb52b65522d8 pushed to origin/codex/native-3d-core; full remote SHA verified. Numerical validation still pending.

NativeIntegrated / codex/native-3d-core, predecessorb441f55b412ff30dbd997890746c6aff62480bdf.
integration_evidence/native_cluster_3d_core_v1/README.md and PACKAGE_PREPARATION_REPORT.md describe a closed initial MPI1/2/4 correctness/lifecycle/repeated OFFON timing package. No new C++ build/mesh run/real qsub was performed; reference-admission draft still pending validation, last validated production1fafe9e83c. Nine lightweight fake-file/synthetic-timing/process/mocked-scheduler checks pass in package_checks_v3, with source/artifact pins; priorv1/v2 preserved. Actual configured Ninja dry-run finds19 pending steps and preparation_refusal_v1 proves the stale executable is refused without binary mutation/output bundle.

Preparation requires the current complete68-object normal MPI/MMG/CGNS/test driver and records source/object/archive/thin-member/build-option pins with drift checks. Bundles are closed regular files; no Git/compiler on nodes. Seven serialized SGE jobs: correctness4slots (serialMMG/native/ref/2D3Dbridge MPI1/2/4 and persisted SU2N1M1/CGNSN4M2), actual metric/Hessian lifecycle N1/2/4, then manufactured n4 timing N1/2/4. Jobs gate same-package correctness; timings also gate same-rank lifecycle. Partial submission receipts preserve IDs/pending acknowledgements; no automatic retry or real submission here.

Timing uses N=M/M=max(1,N/2), warm-up plus3 paired repetitions with alternating detailOFFON;13 raw data files must match within each fixed N/M. Raw logs/SU2CGNS/sidecars/BDF remain inspectable; audited large JSON gzip is verified reversible, with no-overwrite restore. Results retain code/pins/audits and exclude ELF/object/build trees. Download native3d_* result and launcher folders only. Reviewer separates exclusive/nested phases, reports median/spread/useful edit rates/RSS/bounds/imbalance and refuses changed artifacts/work.

CPU remained saturated by foreign FSI after the foreign build ended (load~19 ->~10, sampled idle4–7%); no owned heavy job started. Root filesystem transient ENOSPC prevented one heredoc before execution; retry using python-c succeeded. Workspace~77GiB free, root~1GiB. Only small owned /tmp scratch; no foreign cleanup. NEXT remains contention-gated reference_admission_regression_v1 and frozen fixed-work/restart matrix, then actual package freeze/cluster launch only when requested. Detailed cache-miss/traffic/privateCPU/wait/memory calibration, larger sizes/ranks/practical limits remain. FullGoal1 ACTIVE; orchestration PASS is not numerical/3DBL/curved/fullunsteady/PDE/affordability completion.

## Reference admission draft — 2026-10-11; NOT VALIDATED

Draft checkpoint `d0d3fdaa1ecccd88c981fc770a69729449804444` pushed to
origin/codex/native-3d-core; full remote SHA verified. C++/MPI/runtime
validation remains pending; the last validated production checkpoint is1fafe9e83c.

NativeIntegrated / codex/native-3d-core, based on pushed validated checkpoint
`1fafe9e83c9c8d3f9518740c04dba25c15d8e8aa`. The draft replaces the global256-facet
count guard with requested-memory admission in PlanarReference, ImportGeometry,
DecodeReference and collective LoadReference/factory. It bounds declared sidecar
counts by actual file bytes before facet allocation, keeps the independent1MiB
file limit and private64-cell/256-donor bounds, and distinguishes missing sidecar
from recoverable elected memory refusal without changing published authority.
Reference I/O now admits actual byte/parse/construction demand rather than the
old fixed8MiB placeholder. Construction scratch remains a conservative proxy,
not calibrated allocator/RSS accounting.

Exact forward-collinear border subdivisions are reduced to identical oriented
half-planes before the existing all-vertex convex certificate. Separate hole
loops and nonconvex fallback remain; no hull filling, tolerance or geometry
projection is introduced. No sensor/BL/fade/noise/operator/quality defaults or
2D/MMG sources change. New targeted tests cover512 tiled planar facets, constant
rectangular border query work, oblique/nextafter/orientation controls, actual300
facet CFD import/encode/reload, malformed oversized declared counts, and root-only
early/late memory refusal preserving state. They are written but NOT RUN.

`integration_evidence/native_3d_core_v1/REFERENCE_ADMISSION_MATRIX.json` freezes
configured regression, two n4 fixed-work N1M1/N4M2 cases and actual persisted
restart SU2N1M1/CGNSN4M2 cases. `analyze_reference_admission.py` is prepared,
syntax checked, not executed against new results. `reference_admission_preflight_v1`
retains source pins, clean diff check and real host logs: eight CPUs, load~17,
zero sampled CPU idle, four foreign FSI ranks plus foreign nice19/j2 build
(`/tmp/su2-wall-fixes-build`, PID3859854 at inspection). NO owned build/run/audit
was launched. Do not treat draft publication as any C++/MPI/numerical PASS.

Next: check actual host contention; once available run one nice19/j1 configured
regression in `reference_admission_regression_v1`, preserve failures, then
sequential actual fixed-work/restart cases from the frozen matrix and run the
qualifier. Announce each full folder first; no source/pinned-helper changes while
a campaign is live. Compare26 raw mesh/state/readback files and rank-local edits,
verify production/member pins and independent exact audits, then publish a
validated checkpoint. Global-cap removal does NOT yet certify larger adaptation;
follow with closed larger-domain first-limit cases/SGE package. Complete detailed
traffic/cache/privateCPU/wait/memory calibration and disabled-detail overhead
remain open. FullGoal1 ACTIVE, no full3DBL/curved/unsteady/PDE/affordability claim.

## Canonical planar reference lookup checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessore99dee967611ec960a4e9368bcd7df8b7aba1b3d.
Implementation/evidence checkpoint `31df202a90d549fc834fa8d9b16a38edef4c7f36` pushed to origin/codex/native-3d-core; full remote SHA verified.
REFERENCE_LOOKUP_REPORT.md/REFERENCE_LOOKUP_VALIDATION.json qualify canonical same-marker physical binding: direct singleton lookup, house local CADT box broad phase for multiple patches and unchanged exact finite containment/orientation/lowest-ID checks. Resident/construction/query-workspace admission updated; conservative component scratch still needs allocation calibration. No geometry certification, sensor,BL,operator,guard or cap changes. Global256-facet/1MiB-sidecar limits remain; next work reduces collinear certification cost and replaces the global count cap with memory admission across import/decode/restart, then larger first-limit cases including closed same-marker multiple patches.

Actual complete68-object configured regression passes67 native/reference/2D+3Dbridge cases per MPI1/2/4 rank plus serialMMG48 assertions. New lookup test compares against exhaustive containment for tiled/disconnected/marker/plane/gap/reversed/nextafter/nonfinite and output-preservation cases. Two actual n4 wedge three-event N1M1/N4M2 cases pass independent exact geometry/q/J/L/P1/currentBDF/SU2CGNS and6corruption controls;26 raw mesh/source/state/readback files and rank-local accepted edits match vertex_clip baselines byte-for-byte. Only boundary header/implementation, import caller and targeted test sources change;9 native3D/factory production members rebuild, external/MMG/2D members pinned.

Single contended serial import-face samples135/128/55ms ->9/6/2ms; no sustained speedup/scaling claim. Closed phase CSV/useful edit rates retained, nested costs separate; packed bytes include self traffic and collective elapsed is not purewait. Complete communication/cache/private-wait/memory calibration,detail overhead and closedSGE campaigns remain. No fullAD/PDE/curved/robust3DBL/fullunsteady/affordability claim. FullGoal1 ACTIVE. All owned campaigns terminal; foreign FSI,AdapNoExt,ClusterResults and prior evidence untouched.

## Larger wedge and exact-vertex clipping checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, baseline7bc3869babb19171fe402fdfb0e96f745b769703.
Implementation/evidence checkpoint `9dc21db012be53a1e723b3f01e7649c7cad2b5ab` pushed to origin/codex/native-3d-core; full remote SHA verified.
LARGER_WEDGE_REPORT.md and VERTEX_CLIP_REPORT.md under integration_evidence/native_3d_core_v1 qualify four n4 wedge cases each:N1M1,N2M2,N4M2,N4M4. Input75points/192tets/128triangles; oblique and marker-junction geometry; three rotated/spatial originalP1 events16/4/16; guards.20/.05/1.8/default512/noise0 unchanged. All exact geometry/q/J/L/P1/currentBDF/SU2CGNS audits pass:minq.2474003062/minJ.07957138227/maxL1.7987750917934346; states<=3.865e-16,tensors0. Larger baseline uses unchanged previously qualified production.

The measured local hotspot is exact edge trace (~99% of metric-edge work). Production now reuses validated exact-volume/zero barycentric numerators at exactly matching donor vertices, retaining all other exact arithmetic and rejection checks. Only CNativePredicates3D.cpp.o changes in production archives; one targeted NativeMesh3D test added. All52 fixed-work geometry/source/state/readback files and accepted rank-local edits match baselines byte-for-byte. Full configured68-object regression passes66 native/reference/2D+3Dbridge test cases per MPI1/2/4 rank, plus serial MMG2D/3D48 assertions; passive predicate scalar/DD/AD macro syntax passes, no active AD run claim.

Isolated1622-chord probe compares exact returned data,3 alternating O2 samples/event:kernel median lower31.24%,38.20%,20.72%. Actual full-cycle pilots are single contended samples; MPI4/M4 is slower in two events. No end-to-end speedup claim. Both matrices retain374 closed phase rows each, elapsed event maxima and useful edit rates; nested work is labelled separately. Packed bytes include self traffic, wrapper collective time is not purewait. Native requested bound8MiB vs observed lifetimeRSS baseline31.06MiB/candidate30.74MiB distinct/uncalibrated.

Hard global reference limit256 original triangles remains (separate from64-cell cavity/256-cell donor bounds and1MiB sidecar). Next implement scalable authority storage/search/admission, complete traffic/cache/private-wait/memory calibration and detail-OFF overhead, then prepare closed SGE correctness/repetition/scaling package. User permits studying/reusing existing MMG/refine/Omega_h/ParMmg/Tucanos operators where helpful, with file-specific license and matched-contract tests. No dependency/operator imported here. No owned job remains after terminal regressions/syntax; foreign FSI,AdapNoExt,ClusterResults and historic failures untouched. FullGoal1 ACTIVE, no curved/robust3DBL/physicalunsteady/affordability claim.

## Persistent manufactured restart checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessorb936b29e160905e070cab5ca86ff7618bea135a4.
RESTART_REPORT.md / RESTART_CHECKPOINT_VALIDATION.json qualify eight real SU2/CGNS
mesh/reference/two-file BDF2 restart cases N1M1,N2M2,N4M2,N4M4. Refine, write
currentstep2/olderstep1, destroy driver, actual RESTART_ITER3 constructor and
Preprocess(3), then coarsen/refine; no manual restarted-history initialization.
Original sidecars stay byte-identical, pending authority publishes only on accepted
replacement. Fresh sensor-only originalP1 source per next event; complete frozen
sensor-source persistence is still unproven. Wedge target/guards/default512 unchanged.
Binary checkpoint state residual0, current/BDF manufactured residual<=3.845e-16,
transported/reloadedtensor0; exact geometry/marker/incidence/q/J/L passes all events.
Each restart checker rejects4 state/file corruptions, plus existing6 wedge controls.
No new runtime missing-sidecar fault injection claimed. Disabled control13rawfiles
byte-identical to wedge control. Complete68-object configured regression passes
65native/reference/2D+3Dbridge cases per MPI1/2/4 rank plus serialMMG48assertions.
Production archives/thin members unchanged, all pins stable; no numerical failures.
826closed phase rows; restart write/teardown/reload separate from event parent.
Fixture write includes bounded state checks, teardown includes config serialization.
Single contended pilots/different resumed trajectories, no speedup/scaling/private
balance claim. Requested native bound8MiB, lifetimeRSS34.77MiB distinct/uncalibrated.
Inspectable VTU, adapted SU2/CGNS/sidecars, actual binary histories and states in
integration_evidence/native_3d_core_v1/restart_{su2,cgns}_n2_n{1,2,4}_m{1,2,4}_v1.

FullGoal1 ACTIVE: larger first-failure/size-rank limits, complete traffic/wait/private
CPU/memory calibration, overhead and closedSGE repeated/scaling remain. No PDE
iteration/curved/3DBL/fullunsteady/affordability claim. No owned job remains after
regression. Foreign FSI, AdapNoExt, ClusterResults and historical failures untouched.
Implementation/evidence checkpoint `425c32f7480f989e4967c4dd79c1ee7e6e1f6c70` pushed to
origin/codex/native-3d-core; full remote SHA verified.

## Oblique wedge checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor8acadb1bf67bba237cf245acc4f2d7774cf3c65f.
Implementation/evidence checkpoint `413f02c899031a216fb3ca64eaa9983938950704` pushed to
origin/codex/native-3d-core; full remote SHA verified.
WEDGE_REPORT.md/WEDGE_CHECKPOINT_VALIDATION.json qualify four actual default512
n2 triangular-prism three-event cases N1M1,N2M2,N4M2,N4M4, plus13 byte-identical
box-control files. Input18points/24tets/32triangles, exact volume1/2 and oblique
x+y=1; six finite marker patches, bottom junction x=.5. OriginalP1 rotated/
spatial-direction sensor unchanged; physicalq/J/L guards.20/.05/1.8 unchanged.
Wedge qmin.321433/Jmin.152513/Lmax1.793063, sensor/loadedtensor0,
current/BDF2<=3.845e-16. Each audit rejects6 corruptions incl oblique vertex/name.
Accepted tets N1 46/9/38,N2M2 46/9/36,N4M2 46/14/53,N4M4 46/14/46.
Oblique triangles input8 ->16 ->4 ->10 (N1/N2), ->16 ->6 ->14 (N4);
adapted boundary connectivity preserved exact facets and marker junctions.
Actual SU2CGNS output/readback independently passes; fullsensor persistence open.

wedge_regression_v1 complete68-object actual driver passes64native/2D+3Dbridge
cases MPI1/2/4 and serialMMG2D/3D48assertions. All pins stable, production archives
and thin members unchanged from thin checkpoint. Default seven-marker helpers
also pass six retained thin/box SU2CGNS pairs and reject mismatched wedge markers.
No production/2D/MMG edits. Wedge helper n4 supported but not qualified here.
408closed phase rows/useful edit rates retained. One contended sample per changed
trajectory; no speedup/scaling/private balance claim. Native requested bound8MiB,
lifetimeRSS28.96MiB distinct, calibration incomplete. No ownedjob after regression.

FullGoal1 ACTIVE: persistent manufactured restart/larger first-failure limits,
complete traffic/wait/private CPU/memory calibration, overhead and closedSGE
repeated/scaling packages remain. No 3DBL/curved/fullunsteady/CFD affordability
claim or overnight clusterlaunch request. Foreignwork/AdapNoExt untouched.

## Affine thin-domain checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor58b6b40fb046f130be4eef1fa03c4b9104763f47.
Implementation/evidence checkpoint `df5bcf48e758c9845ffea6bcf0dc7e17ab2dc355` pushed to
origin/codex/native-3d-core; full remote SHA verified.
THIN_REPORT.md and THIN_CHECKPOINT_VALIDATION.json qualify six actual default512
three-event cases: zero-height-change control13files byte-identical to direction
baseline; h=2^-10 on N1M1,N2M2,N4M2,N4M4 and h=2^-20 serial. Production archives/
thin members unchanged. Test-only affine tensor pullback preserves physical q/J/L
.20/.05/1.8 checks; tensor residuals in physical and reference units both0.
Current/BDF2 residual<=3.861e-16, qmin.345975/Jmin.150717/Lmax1.799380104.
Thin cases reject7 corruptions each, including1% normal tensor/schema controls.
Accepted cells86/14/85 serial; N2M2 86/13/91; N4M2 86/12/79; N4M4 86/8/73.
Actual SU2CGNS output/readback plus exact geometry/marker/embedding pass.

thin_regression_v1 actual68-object driver PASS64native/2D+3Dbridge tests at
MPI1/2/4 and serialMMG2D/3D48assertions. All pins stable; no unmatchedselectors.
Retain controlv1 independentPASS/exactcomparisonFAIL (multiply-by-one fast-math
association), controlv2 compileFAIL (Tensor named members), finalcontrolv3PASS;
original/revised matrices and diagnoses preserved. No gate or production change.
442closed rows and per-event useful rates in thin_rank_phases.csv/validation;
one contended pilot each, different N/M trajectories, no speedup/scaling claim.
Requested native bound8MiB/lifetime RSS29.74MiB distinct; calibration still open.
Grids/states in thin_h10_* and thin_h20_n1_v1; no ownedjob after regression.

FullGoal1 ACTIVE: wedge/persistent manufactured restart/larger limits,
communication/wait/private CPU/memory calibration, overhead and closedSGE
repeated/scaling packages remain. No BL-generation/curved/unsteady/affordability
claim or overnight clusterlaunch request. Foreignwork/AdapNoExt untouched.

## Spatial metric-direction envelope — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor8b64fe7dba33b62612065009981f1cae660952e1.
Implementation/evidence checkpoint `ed79f20caf9708809722db5d0b871c71a495000f` pushed to origin/codex/native-3d-core; full remote SHA verified. Test-only --direction1 prescribed
spatially rotating SPD tensor with azimuth45–90deg/elevation0–30deg, strength1+.125x,
aspect1.5, scales4/1/4; freeze original sensor P1 per event. Production unchanged.
DIRECTION_REPORT.md/VALIDATION.json qualify five actual default512 driver cases,
408closed accounting rows, current/BDF2/SU2CGNS and exact geometry/metric audits.
DirectionN1/M1 86/14/85tets, N2/M2 86/13/91, N4/M2 86/12/79, N4/M4 86/8/73 PASS.
qmin.345975/Jmin.150717/Lmax1.7993801; tensors/loadedtensors0, state<=3.84415e-16.
Legacy control direction_control_a2_n2_n1_v2 retains11 byte-identical datafiles.
Final direction_regression_v1 complete68objects passes64native/2D+3Dbridge tests
at MPI1/2/4; MMG2D/3D round trips serially (48assertions), no unmatchedselectors.
Existing324 independent automatic frames use unchanged production identities;
not rerun for fixture-only edits. Source/object/archive/binary pins stable.

Retained direction_a15_n2_n2_m2_v1 FAIL independentreload: final frozenP1
Lmax1.7993801, analytic-resampled reloadtarget1.8146257. Rootcause oldhelper
changed nonlinear target during reload; no productionmesh or guard error.
Helper now fetches transported nodes from acceptedSlices and saves separate
reload_nodal/reload_target records; independent nodal topology/SPD/P1+frozen-target
geometry checks pass SU2CGNS, including missing/corrupt-source negatives.
Actual filenames *_reload.json (nodal), *_reload_target.json (originalfield).
This retains oldfield in memory, does not prove relinearized sensor interior
admissibility or full frozenfield persistence in mesh files; actualrestart open.
Initial serialv1 and both default controlv1/v2/failure kept; new serial/MPI2v2 pass.
Allnine adaptation/source/state datafiles from failedMPI2v1 and passedv2 identical.

Parent pilotsN1 .510/.549/.448s, N2 .535/.621/.399, N4M2 .542/.617/.331,
N4M4 1.298/1.149/.699. Different trajectories/singlecontendedruns do not prove
scaling. Parentmax/mean near1 does not prove private CPU balance or no MPI waits.
Requestedbound8MiB/lifetimeRSS31.3MB distinct, full calibration incomplete.
AcceptedVTU/SU2/CGNS/states in direction_a15_* folders, no ownedjob after audits.

FullGoal1 ACTIVE: thin/wedge/restart/larger limits, complete wait/traffic/private
CPU/memory calibration, overhead and closedSGE repetitions/scaling remain.
No actualCFD/fullBL3D/unsteady/affordability claim or overnight cluster request.
Foreignwork/AdapNoExt/submodules/ClusterResults unchanged.

## Stronger anisotropy envelope — 2026-10-11

NativeIntegrated / codex/native-3d-core; tested production unchanged from donor
implementation a406a1281a7d7bd16d45c88070737c82b1347897, prior remote tip verified
157750a75bece663540c36e1ee6d68fbed07ce06. ROBUSTNESS_EXPANSION_MATRIX.json was frozen
before runs. ROBUSTNESS_STRENGTH_REPORT.md/VALIDATION.json qualify actual default512
aspect2, n2 box, fixed45-degree eigenframe and1+0.125x strength, three events:
robustness_a2_n2_n1_v1 PASS128/17/79tets;
robustness_a2_n2_n4_m2_v1 PASS128/16/109tets including return to four CFD ranks.
Independent geometry/marker/embedding/P1/current+BDF2/SU2CGNS audits PASS;
qmin.246842/Jmin.0870495/Lmax1.798437, tensorerror0/state<=3.81218e-16.
Source/object/archive identities stable, exclusive phases close. VTU/SU2/CGNS
and states in both named folders. No owned heavy job live after final audits.

Parent pilot secondsN1 .815206/.970013/.328394, N4M2 .978312/1.442031/.376798;
useful rates77.28/40.21/182.71 and64.40/26.35/138.00edits/s. Different trajectories,
one nice19/thread1 pilot each with foreignFSI; no scaling/speedup/affordability
claim. Maximum bound8MiB/lifetimeRSS31.1MB distinct, not fullmemory calibration.
Initial analyzer dictionary equality failed because full driver lists three
additional system archive aliases; runtime production archives/thin members match.
Corrected using existing donor verifier rule, retained originalscript/failure.
No run/receipt/guard/production edit.

Next: add frozen genuinely spatial direction-change fixture (currentvarying means
strength only), thin/wedge/restart and size limits, then complete wait/traffic/
memory calibration and closedSGE overhead/repeated/scaling packages. FullGoal1
ACTIVE; no physicalCFD/fullBL3D qualification or overnight clusterlaunch request.
Foreignwork/AdapNoExt/submodules/ClusterResults unchanged.

## Native3D donor reuse and certified segment filter checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor89225cf0cbc9215fd92080638274c602c5bd9005.
Implementation/evidence checkpoint `a406a1281a7d7bd16d45c88070737c82b1347897` pushed to origin/codex/native-3d-core; full remote SHA verified. DONOR_REUSE_REPORT.md and
DONOR_REUSE_CHECKPOINT_VALIDATION.json qualify four actual default512 driver
cases (12 events),44 byte-identical geometry/source/state/readback files,
four unchanged rejected-replay files,340 closed phase rows,324 independently
audited automatic frames and nine negative controls. All final receipts PASS;
source/object/archive identities stable. No owned process live after final audit.

DonorImport reuses an immutable donor already selected in this caller union,
using the existing bounded map; no persistent cache/new heap or payload change.
TraceSegment validates finite positive donors then rejects only certified strict
same-face endpoint negatives. Unresolved/touching/crossing intervals retain exact
rational roots/P1/coverage/overlap checks. New donor_reused/edge_separated counts.
No target/geometry/operator/BL/sensor/fade/noise/2D/MMG production changes.

Serial alignedn3 default288/50/312tets: secondcoarsen482/66/416,2717requests/10hits
unchanged;125165 donor tests and21750 clips skipped. Singlecontended engine
4.10262->2.61079s, donorfilter1.41236->0.31636, edgeTrace2.14006->1.76457.
Isolated1547chord comparison preserves all exact interval/weight values;
three alternating samples per3sets show lower medians29.5/21.9/36.4%, but exclude
import/ADT/MPI/CFD. No repeat-qualified total-speedup/scaling/affordability claim.
N4M2/N4M4 default288/47/277 and288/51/264; originaln2N1 88/36/94.
qmin.202935/Jmin.062176/Lmax1.799641; tensorerror0/state<=5.80681e-16.
Originaln3 replay175rounds57privateaccepted/42insertions,420tets remainsrejected;
acceptedCFDstate and allfour original/rejected datafiles preserved.

Verification correction: legacy [MMGRoundTrip] selector did not match registered
[MMG] tests, so historical runner PASS does not prove MMG execution. Correctedv3
passes serial but fails MPI2 because direct MMG fixtures call master-only SetMesh
on empty nonmaster extraction. Failure retained, no production/fixture edit.
Final regression_v4 actual68objects passes62native/2Dbridge tests at MPI1/2/4;
actual2D/3D MMG round trips runserially (48assertions). Unmatched selectors fail.
Probe preflightv1FAIL and superseded exact-signv2 mixed timings retained;
latest production/qualified runtime matches probev3 and actualN1_v2.

FullGoal1 ACTIVE: stronger/direction-changing/thin/wedge/restart/practicallimits,
complete wait/traffic/memory calibration, overhead and closedSGE repetitions/
scaling remain. AcceptedVTU/SU2/CGNS and states in donor_reuse_* casefolders;
rejectedSU2 in donor_reuse_replay_original_n3_v1. No CFDiterations/fullBL3D claim.
No overnight clusterlaunch request; foreignwork/AdapNoExt/submodules unchanged.

## Native3D exact coarsening filter checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessorf05ca3a8828d56619bdcafc763cbe4c8facae1d0.
Implementation/evidence checkpoint `80a4a153da2a3945666661c15c1f6fdb3381da8f` pushed to origin/codex/native-3d-core; full remote SHA verified. COARSEN_FILTER_REPORT.md and
COARSEN_FILTER_CHECKPOINT_VALIDATION.json qualify four actual default512 driver
cases (12 events),340 closed phase rows, complete68-object MPI1/2/4 filter/
coplanar/insertion/automatic repair/budget/default/native2D/MMG regression,
324 independent automatic frames and9 negative controls. Source/library/object/
artifact pins agree. No owned process live after the final audit.

Directed boundary collapse counterexamples use exact immutable finite-facet
Contains, cached in per-cell bits/per-edge counts, updated with mesh edits.
Filter enables only once globallyadmissible; invalid-field collapse repair is
preserved. Counters report facet checks and queued directions actually removed,
not all suppressed future enqueues. Extra storage charged by mapmodels; nested
geometry timer belongs to seed/update. Local witnesses can miss remote-only
obstructions (safe underfilter), full imported validation remains authoritative.
Successful vote preserves local rejection reason; v2 tests invalid-field no-prune
and nonblank exact geometry reason, failure timing return moved outside scope.

Serial alignedn3 full default512 nowpasses288/50/312tets. Second coarsen482
rounds66accepted416rejected versus old diagnostic619/66/553, seven initial/first
accepted/state/sidecar/secondinput files byte-identical to faileddefaultbaseline.
Exactpredicate2933, queuedpruned443, nestedgeometry0.03144s. Fieldrequests2717/
hits10 unchanged. Singlecontended enginewalls4.18517old/4.10262new do NOT prove
speedup. Nestededgetrace2.14006 and donorfilter1.41236 remain dominantquestions.
N4M2 defaultpasses288/47/277; N4M4 passes288/51/264. Originaln2 N1 passes88/36/94.
Qualifiedqmin0.202935/Jmin0.062176/Lmax1.799641, tensorerror0/state<=5.80681e-16.

Inspectable grids: integration_evidence/native_3d_core_v1/
coarsen_filter_aligned_n3_n1_v1/envelope_s0.vtu through envelope_s2.vtu,
plus N4M2/M4 folders, SU2/eventsidecars/finalCGNS. All historic failures retained.
No CFDiterations run. Current reusable runtimebinary lives in this newN1folder.

Next: address measured exact donor clipping/edge tracing without compromising
source coverage/P1/metric/geometry guards. Continue stronger/direction-varying/
thin/wedge/restart/larger practical limits, full wait/traffic/memory/calibration,
instrumentationOFFON/repeated scaling and closed SGE packages. FullGoal1 ACTIVE;
no overnight clusterlaunch question, no speedup/CFDaffordability/full3D claim.
Foreignwork/AdapNoExt/submodules/ClusterResults remain untouched.

## Native3D cavity insertion checkpoint — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor16029e00cba59c54a05c71eb320c4833845762a2.
Implementation/evidence checkpoint `128c9e58e267462b2761804df44708048f359216` pushed to origin/codex/native-3d-core; full remote SHA verified. INSERTION_REPORT.md and
INSERTION_CHECKPOINT_VALIDATION.json qualify nine actual manufactured driver
cases (27 events), 884 accounting rows, complete configured68-object MPI1/2/4
regression and324 independent automatic repair frames plus9 negative controls.
Six originaln2 N/M controls preserve66 predecessor geometry/source/state files
byte-for-byte. Source/object/archive/artifact identities verified.

Bounded InsertEdgeCavity removes exact-placement obstacles by positive coning
with unchanged interior skin or a coupled same-canonical-planar-facet surface
fan. Marker junctions/incomplete stars reject. MPI ownership routing and inserted
statistics reconcile arbitrary fans; existing metric/quality/geometry/atomic
checks, default512 and all2D/MMG/BL/sensor/fade/noise policies stay unchanged.
Direct unit/thin1e-8 insertion/junction/rollback tests pass. Automatic324-frame
audit covers existing repair paths, not insertion itself.

Larger alignedn3 defaultN4M4 passes288/61/308tets; defaultN4M2 passes288/47/281.
Serialdefault accepts first288tets then rejects second coarsening at512;
diagnostic explicit1024adapter passes288/50/312 after619 coarsening rounds
(66accepted/553rejected). Seven common input/first accepted mesh/state/sidecar/
secondinput files identical; full default serial3events still fail.
Originaln3 replay remains rejected:175rounds,57privateaccepted/42insertions,
118rejected,420tets121points,qmin0.554689/Jmin0.334497,6longedges12cells,
Lmax2.229036; geometryvolume1 and accepted CFD state preservation pass.
Historical infeasible original marker junction remains intact.

Inspectable accepted grids: integration_evidence/native_3d_core_v1/
insertion_aligned_n3_n4_m2_v1/envelope_s0.vtu through envelope_s2.vtu,
SU2 meshes/reference sidecars and finalCGNS alongside. Rejected original grid:
insertion_replay_original_n3_v1/replay_rejected.su2. No CFD iterations run.
Single contended pilots/different trajectories establish no speedup/scaling;
serial coarsening candidate failures dominate the next optimization question.

Next: geometry counterexamples can prune impossible directed boundary collapses
only when the current frozen field is globally admissible, preserving useful
invalid-field repair fallback and strict/default completion. Fix successful
vote clearing local rejection reason separately. Both are unimplemented designs.
FullGoal1 ACTIVE: complete wait/traffic/memory/overhead/repeated scaling,
stronger/direction-varying/thin/wedge/restart/practical limits and closedSGE remain.
No owned case live; no overnight clusterlaunch request. Foreignwork unchanged.

## Native3D recovery coordination and placement diagnosis — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessor a410dad7a47bee500d9ccc2d32d8d08250937470.
Implementation/evidence checkpoint `3b84e6a3b9ba9ae1467da4f556dacab84e3ef073` pushed to origin/codex/native-3d-core; full remote SHA verified. COORDINATION_REPORT.md,
COORDINATION_CHECKPOINT_VALIDATION.json and coordination_rank_phases.csv bind
six N/M actual driver controls (18 refine/coarsen/refine events, original-P1
current/BDF2 transfer and SU2/CGNS output/readback) plus configured68-object
MPI1/2/4 automatic repair/coarsening/single-recovery/budget/default/2D/MMG checks.
All PASS; source/object/archive identities stable. 578 exclusive/nested rows.
qmin0.258663/Jmin0.062176/Lmax1.785100; tensor error0/state3.84415e-16.

Production Refine now collectively retries unblocked splits while retaining its
single dependency-tracked recovery queue; retained repair storage is charged
in split rounds. One extra scalar collective per incomplete recovered round.
Default512, source/geometry/quality/metric and 2D/MMG policies unchanged.

Original n3 still rejects:512 rounds64 privateaccepted/448 rejected;1024 allowance
stagnates at631 with64 accepted/567 rejected. Same private240tets/81points,
qmin0.201655/Jmin0.091521,22longedges/53cells/Lmax2.464292. Geometry/volume1
independent checks and accepted CFD state preservation PASS. Inspect rejected
meshes in coordination_replay_r512_v1 and coordination_replay_r1024_v1.

placement_certificate_v2 proves14 remaining edges have no binary64 interior
point; all22 rounded midpoints are off-edge. Original marker junction9–14
(facets30/31, z_minus_b/z_minus_a) is one certified edge, L2.18657>1.8.
Strict exact junction preservation and length completion are incompatible for
that segment. Interior certificates only establish an operator-placement limit;
retriangulation can remove interior edges. No silent geometry tolerance change.
Nonconvex same-marker raw seams are a separate geometry limitation.

Next: preserve originaln3 as feasibility/rejection evidence, add an aligned
physical marker fixture and bounded representable cavity insertion/reconnection
using existing Cone/fixed-skin certification, including MPI routing and coupled
surface checks. Full Goal1 ACTIVE: larger/direction-changing/thin/wedge/restart,
full wait/traffic/memory calibration, overhead/repeated timings and closed SGE
remain. Single contended pilots/different trajectories prove no speedup or CFD
affordability. No owned case live after finalN4M4 terminalPASS; no overnight
cluster launch questions. Foreign FSI/AdapNoExt/submodules/ClusterResults untouched.

## Native3D exact rejected-private replay — 2026-10-11

NativeIntegrated / codex/native-3d-core; predecessorce942b5d6fba97985577b2416e1d4256018b6e7c.
REPLAY_REPORT.md / REPLAY_CHECKPOINT_VALIDATION.json and frozenREPLAY_MATRIX.json
retain exact original n3 input/import/sensor match and bounded512/1024 direct
Refine probes. No production change.512 replay exactlymatches original backend:
59 privateaccepted/453 rejected,18moves/26reconnections,229tets,79points,
qmin0.267933/Jmin0.146700,25longedges/65cells, Lmax2.464292.1024 stillrejects:
79accepted/945rejected,231tets,21longedges/51cells, qmin0.211317/Jmin0.098595,
sameLmax. Both independentpositivevolume/nooverlap/incidence/finitefacet/marker/
volume1 checks pass; accepted CFD mesh/solution/reference/source unchanged.

Inspectable private diagnostics (never accepted CFD grids):
 integration_evidence/native_3d_core_v1/replay_size3_r512_v2/replay_rejected.su2
 and replay_size3_r1024_v1/replay_rejected.su2. Audits list everylongedge with
coordinates and incidentcells.512 enginewall8.046s/exclusivemetric6.425s;
1024wall12.943s/metric10.308s. Nestededge tracing overlapsmultiplephases;
singlecontendedruns, no scaling claim/usefulaccepted CFD edits for rejectedevent.

Initialrunner syntaxfailure retainedbeforebuild. Firstreplayfixturev1 crashed
querying uninitialized driver native reference afterRefine: fixcreatesBackend
before queryingReference, onlytestcode. v2 all17Cppassertionspass; initialPython
raw/derivedfacetcomparisonfailure retainedwithhash-bound correctedwhole-raw-input
comparison recovery, no remeshingrerun.1024 terminalPASS and ordinary runner
controlPASS:11 mesh/source/state files identicalto published N1varyingcase.
Allproduction archives/members equalreader checkpoint; compactlogs/grids/pins.

Next: production Refine currentlylocksinto repairmode untilwholefieldpasses,
suppressing newlyunblocked splits after acceptedrepairs. Test alternating available
splits while retaining a single seeded/tracked repairqueue, dependency blocking,
512/defaultstoragebounds and alltarget/geometry/atomicguards. Separate operation
selection from repairtracking, keep globalMPIoperationchoice consistent. Coarsen/
pureRepair/current<=1recovery tests must remain valid. This is a control-flow
hypothesis, not yet an implemented/provedfix. Exact segmentclipping hotspot remains
separate; preserve tiny/tied/overlap/invalidsource checks if filtering is improved.

FullGoal1 ACTIVE; noownedcase live after ordinarycontrol terminalPASS. Full MPI
wait/traffic, memory calibration, stronger/directionchanging/thin/wedge/restart
and practicalrank/size limits, repeatedOFFON and closedSGE remain. Noovernight
clusterlaunchquestions; foreignwork untouched. Implementation/evidence checkpoint 2463e523c2827a5182139fcdd4a71ff7083053fe pushed
to origin/codex/native-3d-core; full remote SHA verified. This confirmation
follows that publication.

## Native 3D anisotropic driver envelope and first size limit — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessord41c49de169444c6053674860da0285b6347fe18.
ENVELOPE_REPORT.md / ENVELOPE_CHECKPOINT_VALIDATION.json retain frozen initial
matrix, actual complete-archive fixtures/pins, seven qualified cases, and first
162-tetrahedron rejection. No production code changes; archives/members equal
pushed reader checkpoint. N1M1/N2M1/M2/N4M1/M2/M4 varying-strength45deg metric
passes three actual refine/coarsen/refine and ReplaceMesh/BDF events each; also
constant metric control.56 geometry/readback and21 transported-state frames,
234 exclusive+378 nested rows pass. Tensorerror0/stateerror3.84415e-16. Different
partition trajectories independently qualified. Original N2M1 timing-only FAIL
(-4.3368e-19 CPU remainder) kept with hash-bound existing-tolerance recovery.
Legacy helper/default sensor/native2D/MMG MPI1 control passes;84 old stateframes
re-audited. Actual SU2/CGNS/readback and inspectable VTUs with synthetic histories.

Size3 N1M1 first event rejects after512 rounds/59 privateaccepted/453 rejected.
Accepted CFD mesh/solution/reference preservation checks pass; source/input/logs
and FAIL receipt kept. Remesh7.917s/refine7.823s, exclusivemetric6.234s and
reconstruction0.806s; nestededge trace6.496s overlaps phases. No proved cycling
or larger-budget fix. Stop size4/a2 expansion until exact rejected-private replay
and feature/residual diagnostics explain budget versus operator blockage.
Private rejected progress not exported yet. Passing grids:
 integration_evidence/native_3d_core_v1/envelope_mpi4_m4_v1/envelope_s0.vtu
 throughs2.vtu, actualdriverSU2/eventsidecars and finalCGNS nearby.

Full Goal1 ACTIVE. No owned case live after legacy control terminal PASS.
Next exact failure replay, direction-changing/thin/wedge/restart/practical limits,
full wait/traffic and memory calibration; repeated OFFON and closed SGE package
still required. One contended pilot is not scaling/overhead/affordability proof.
Do not ask overnight cluster launches. ForeignFSI/submodules/AdapNoExt/
ClusterResults untouched. Implementation/evidence checkpoint dd673805245e4946bc995ba193a986f418299155
pushed to origin/codex/native-3d-core; full remote SHA verified. Two redundant
terminal-PASS test binaries removed after identical-hash verification, reclaiming
1,217,722,368bytes; qualified shared binary remains in envelope_mpi2_m2_v1.
ENVELOPE_DISK_COMPACTION.json preserves receipts; no numerical evidence removed.

## Native 3D reader admission validated — 2026-10-11

NativeIntegrated / codex/native-3d-core; predecessor
4c9f56b8c861433158e58459ee0d5c157742bb9b. READER_REPORT.md /
READER_CHECKPOINT_VALIDATION.json: new recoverable requested-storage reader gates
cover validation/point/signature/linear/volume/root-boundary buffers; native
assembly maps/vectors and old output included. Targets, original sensor-only P1,
actual-query composition and legacy2D/MMG unbudgeted calls preserved. No new CFG.
reader_runtime_v1 interrupted initial duplicate-block build retained; corrected
reader_runtime_v2 terminal PASS42 stages/session79225exit0. All production/test
pins unchanged during full check.84 main+36 computed-metric accepted events,
42 OFFON/42 SU2CGNS;126 mesh frames/84 transported-state frames/36 computed-metric
frames identical to predecessor.357 closed rank-stage and357 nested records.
Native2D/MMG/default/rollback/refusal regressions pass. reader_skew_v1 terminal
PASS11 stages/session1666exit0; only test source changed, all production archives/
members equal full-run pins. All-key-to-root skew, late linear-point refusal/
retry and seven rank bounds pass; N4 root18,258B vs others10,419B. No owned job live.

Largest main model8MiB, main lifetimeRSS37,904,384B, separate quantities. Budget
success adds three count Alltoalls/nine scalar Allreduces within existing reader
stage; full house/vendor traffic/wait still open. Matched3-repeat phase timings
saved but foreign FSI variability prevents isolated overhead/speedup claims.
Native tree/vector bounds exclude allocator/MPI/ParMETIS/CFD internals; cache
scratch calibration and practical envelope remain. Goal1 ACTIVE: next complete
wait/communication accounting, larger/rotated/thin/restart and rank/size pilots,
then closed SGE campaigns without overnight launch requests. Unsteady sampling
detail also open. Inspect reader_runtime_v2/lifecycle_mpi4 and mpi4 SU2/CGNS plus
metric/current/BDF/source JSON. Implementation/evidence checkpoint
c8c6ffaf9121b19463cd8ca8e6f528e3f269d9bf pushed to origin; full remote SHA
verified with git ls-remote. This handoff confirmation follows that publication. Foreign submodules,
FSI/AdapNoExt/ClusterResults untouched.

## Native 3D lifecycle profiling validated — 2026-10-11

Working NativeIntegrated / codex/native-3d-core; predecessor
 d5df2785676e610b2dcaec2c5aee7ea93ceb9670. PROFILE_REPORT.md and
PROFILE_CHECKPOINT_VALIDATION.json retain exact scope/provenance. Opt-in3D
per-rank exclusive wall/process CPU/lifetimeRSS records cover actual metric/
Hessian build, CFD ParMETIS/return migration/geometry preprocessing, solver/BDF
replacement and requested mesh/reference output. Nested source/partition/engine/
query counters exported. No targets or operators changed; 2D/MMG gate disabled.
profile_build_v1 compile failure and profile_runtime_v1 invalid equal-size CFG
failure preserved; corrected profile_build_v2 and final42-stage profile_runtime_v2
PASS. Final actual test objects recompiled for outer timing/config correction.
Session25990 terminal0, no owned job live. Source/archive/object pins unchanged.
Main84 accepted events/42 OFFON controls/42 SU2CGNS roundtrips;126 predecessor-
identical mesh/source/transported-state frames. New actual-computed-sensor fixture
adds36 accepted events, three fresh repeats per M1/2/4<=N/OFFON;36 strict frames,
357 closed rank-stage rows and357 nested rows, parent containment/adversary checks.
Native2D/MMG/default ownership/private-progress rollback/budget regressions PASS.
Phase CSV and lifecycle_phase_summary.json retained; summarize_lifecycle.py
regenerates them. New fixture C++ affine current/BDF checks; no CFD iteration.

N1/M1 ON sample remesher62.54ms: refine45.48/coarsen14.83; within refinement
metric17.49/reconstruction6.80ms. N4M4 high contention/spread; ON/OFF ratio1.177
cannot establish instrumentation penalty or scaling. Stage own emission excluded,
parent includes nested emission; outer testcase includes test metric observation,
output separately timed. Do not sum stage maxima or infer MPI wait from wall-CPU.

Full Goal1 ACTIVE. Next: complete MPI wait/house traffic and reader/ParMETIS
memory calibration; practical larger/rotated/thin lifecycle/restart and rank/size
limits; then closed SGE campaigns. WINDOW_AVERAGE/PREDICT sampling detail also
not fully covered. No full-memory/scaling/BL/unsteady affordability claim. New
cases profile_runtime_v2/lifecycle_mpi4; transported metrics/states and CGNS in
profile_runtime_v2/mpi4. Implementation checkpoint
 de21d8a462de57ac4f2ba5b96b9418922dbb7e79 pushed to origin and full remote SHA
verified; this handoff update follows that publication. No cluster-launch
questions tonight; foreign FSI/AdapNoExt/
submodules/ClusterResults untouched.

## Native 3D memory checkpoint validated — 2026-10-11

Repository NativeIntegrated; branch codex/native-3d-core; remote predecessor
0864f56e60f0f7d66e5db6cf8779cb8aaa1703c7. MEMORY_REPORT.md and
MEMORY_CHECKPOINT_VALIDATION.json retain scope/pins. memory_build_v1 and
memory_runtime_v1 terminal PASS; session95453 finished, no owned job live.
New budget refusal/retry tests PASS MPI1/2/4. Actual driver matrix:84 accepted
events,42 OFF/ON pairs,42 SU2/CGNS roundtrips;126 mesh/readback signatures and
transported states identical to predecessor. Sensor error0; max affine BDF2
error1.119683e-12; qmin.628068/Jmin.397369/Lmax1.793742. Native2D/MMG/default
ownership and refusal-after-private-edits regressions PASS MPI1/2/4; raw default
mesh/reference audits PASS.336 runtime rank rows include modeled bytes and
process-lifetime peak RSS; largest model8MiB vs matrixRSS38,666,240bytes,
separate quantities, not an allocation certificate. No source/archive/object
changes during checks. Implementation checkpoint
7f004951f080013d82c9fda5d8cd8bc4043f58d4 pushed to origin. Publication tip
64b13aa2f0ff0006e9b134cf29b55b2abfa16f4b verified with git ls-remote; this
handoff confirmation follows that published implementation/evidence.

Full Goal1 remains ACTIVE. Initial import/caller-held storage gates now cover
point directory, face exchange, original authority and retained runtime buffers;
redundant working maps/source/operation records released. Reader/partitioner
internal memory and existing reader/cache model calibration remain open. Next:
close metric/import/query/partition/return/rebuild/transfer per-rank lifecycle
profiling, then restart/larger/rotated cases and repeated practical envelope.
No speedup, full memory or affordability claim; no cluster-launch questions tonight.
Inspectable memory_runtime_v1/mpi4/runtime_barycentric_m4_d0_mesh_00003.su2,
adjacent sidecar, s2_written_CGNS.cgns and s2_states.json. Foreign FSI,
AdapNoExt, submodules and ClusterResults untouched.

## Native 3D production runtime checkpoint — 2026-10-11

Repository `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`; branch
`codex/native-3d-core`; predecessor `5d83dcda7958f5af81ddeb3235b1a0a56f686a01`.
Published runtime implementation commit `28bb42edc7e191b07699ea6eac9090968aa93eb5`,
pushed to origin and full remote SHA verified. Checkpoint report:
`native_3d_core_v1/RUNTIME_REPORT.md` and
`RUNTIME_CHECKPOINT_VALIDATION.json`. Actual factory/ReplaceMesh/accepted-reference
notification, durable original faceted sidecars, M1..N on MPI1/2/4: 84 accepted
driver events,42 exact OFF/ON controls,42 SU2/CGNS output/readbacks, sensor error0,
maximum affine current/BDF2 error1.119683e-12; qmin.628068,Jmin.397369,Lmax1.793742.
Separate actual native2D driver and MPI-aware 2D/3D MMG regression PASS MPI1/2/4.
Default no-native-repartition path and rejection after private accepted edits,
retry/state preservation/output audits also PASS MPI1/2/4. No owned build/test
remains live: runtime regression session68072 and defaults24623 returned PASS.

Historical `runtime_v3/validation.json` remains FAIL due to wrong serial-only MMG
unit selection on MPI2 AFTER all 3D stages/audits passed. Follow-up correct
MPI-aware regressions PASS with the same production sources/archives. Manifest
links both; no failure rewritten. Earlier wrong build target/runner setup and
lexical-marker-order encoder failures retained. No MMG implementation changed.

Inspectable final meshes/states (manufactured cube, not real CFD):
`integration_evidence/native_3d_core_v1/runtime_v3/mpi4/`
`runtime_barycentric_m4_d0_mesh_00003.su2` and adjacent original `.native_ref`,
`runtime_barycentric_m4_d0_s2_written_CGNS.cgns`,
`runtime_barycentric_m4_d0_s2_states.json` and mesh `.json`.

Full Goal1 ACTIVE. Next priorities: complete metric/index/query/repartition/
return/rebuild/transfer per-rank exclusive lifecycle reporting and CPU/wait/RSS,
aggregate live-buffer admission (initial import and caller-held copies currently
not fully covered), actual driver reload/restart, larger/thinner/rotated lifecycle
cases, repeated overhead and practical size/rank qualification, then closed SGE
scripts. Current phase closure is remesher-only; no scaling/affordability claim.
3D requires planar adapted boundaries/no BL/noise0; original reference limit256
triangles remains. Future combined reusable runner assembled from qualified
components has syntax checks but was not redundantly relaunched. No cluster-launch
requests tonight; foreign FSI, AdapNoExt and ClusterResults left untouched.

## Native 3D actual geometry/solution bridge validated — 2026-10-11

Latest checkpoint preparation: geometry_bridge_v10 and geometry_meson_v1 both
terminal PASS, all source/archive/thin-member pins unchanged. No owned build/test
is running after session68185 returned terminal PASS. New CNativeImport3D and
finite oriented ContainsFace are validated against actual CFD geometry, weighted
M<=N core, returned readers, geometry/solver reconstruction, current/BDF2 transfer,
SU2/CGNS writers/readers and independent audits on MPI1/2/4, M1..N. Each matrix:
42 events,84 audited frames,42 state pairs,42 raw output roundtrips; minq.367927,
minJ.146382,maxL1.793742,transported sensor residual0,maxstate8.471971e-13.
Shared 2D fixture regression passes. See native_3d_core_v1/GEOMETRY_BRIDGE_REPORT.md.

At published 5d83dcda, production native3D factory remained guarded. Full Goal1 ACTIVE: wire the opt-in
runtime and durable planar authority, then close complete lifecycle phases/CPU/
wait/RSS/memory and OFF/ON/repeated practical envelope before claiming completion.
Current one-shot timings include tests/output and changed trajectories; no scaling
or affordability claim. Existing underfilled CFD ranks remain a documented limit.
No cluster requests tonight. Leave AdapNoExt/foreign submodules/ClusterResults alone.

### Earlier bridge setup and fixture diagnosis (retained)


Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core; predecessor e19af2b2f22654d6ba95862761663a385d4b15ca.
Goal1 ACTIVE/full scope. CNativeImport3D imports actual CFD-owned
sensor-only tetrahedra, halo coordinates and oriented physical incidence. It
retains bounded immutable faceted authority, rebinds finite triangles after reader
renumbering, queries original P1 in bounded batches for weights and returned reader
metrics, and preserves output on elected failures. No production driver factory
has been wired; native runtime3D guard remains.

Geometry bridge v1..v5 failed during standalone build setup/linking, BEFORE TESTS:
v1 METIS include; v2 public matrix include; v3 MEL include; v4 reduced standalone
house-helper duplicates; v5 actual SU2 archives retain pre-communicator ABI. Do not
claim adaptation/state/output validation from these receipts. All failures remain.

Actual 268-step current-ABI Common/strict/CFD-core rebuild geometry_build_v1
is terminal PASS, exit0, no production source changes during the build. The
follow-up geometry_followup_v1 is terminal FAIL: it launched v6, whose test helper
incorrectly encoded a vector as one record. Fixed by encoding individual records.
V7 compiled/linked but aborted in CConfig before adaptation: COMPUTE_METRIC=YES
requires ADAP_SENSOR. geometry_bridge_abort_v1 captures the existing v7 binary's
one-rank debugger stack proving this cause. The fixture now supplies PRESSURE.
No production config guard or numerical contract changed.

Validated auditor checkpoint pushed/remote-verified:
a4806f8e796d564d6afae0e36b593d8aae7c23b8. Actual bridge is now validated for checkpoint publication.
V8 reached adaptation/reader return but stopped at the existing end-step history
contract: current solution must equal time_n after the dual-time update.
geometry_bridge_abort_v2 proves that cause. Fixtures/checker now use identical
current/time_n and distinct time_n1; no transfer contract changed. New lightweight
geometry_audit_v4 PASS includes six adversaries, adding mismatched time_n.
Historical v3 fixture used a synthetic independently scaled time_n; its old
checker source pins remain, but it is not an admissible production transfer input.

V9 terminal FAIL: MPI1/2 tests and independent audits PASS (12/24 frames,6/12
state pairs and6/12 real SU2/CGNS roundtrips). MPI4 completed all 3D worker choices
M1..4 and both methods, then the added six-point 2D MG fixture crashed in
CPoint::SetPoints/BuildGeometryFVM with an empty sparse graph on two ranks.
No 3D adaptation/transfer numerical failure was observed. The supplemental
four-rank independent audit is terminal PASS (48 frames,24
state pairs,24 actual roundtrips); the whole v9 receipt remains FAIL.
The shared 2D fixture now uses the existing transfer tests' n4->n6 meshes, avoiding
underfilled CFD partitions without changing production geometry handling.
The corrected geometry_bridge_v10 matrix subsequently passed in full.
Original complete v9 receipt stays FAIL, including the 2D crash.

Test-only bridge includes bounded actual owner/halo geometry and sensor import,
retained planar marker authority, weighted explicit M<=N worker communicators,
returned distributed original-P1 metric readers, actual geometry/solver rebuild,
barycentric/conservative current/BDF2 transfer, SU2/CGNS write/readback, atomic
reader-budget vetoes, and a manufactured cached/remote thin query-composition
control. That control is not composed3D edge integration or BL generation.
No configured driver/CFD iteration, full phase/CPU/wait/RSS closure or performance
qualification yet. Actual Meson bridge test compilation and archive-linked checks now PASS.

geometry_audit_v3 PASS: lightweight independent-auditor self-check, valid serialized
box/state/SU2/HDF5 fixtures and five rejected corruptions. It is not actual native
output or CFD integration proof. v1/v2 self-check failures retained. Report
geometry_audit_v3/AUDIT_SELF_CHECK.md explains exact scope and pending work.
Auditor checkpoint is already published; both actual bridge campaigns now pass. No cluster questions tonight. Full configured
native3D, lifecycle/CGNS/rejection, performance/RSS/scaling goal is not complete.

## Native 3D weighted partition checkpoint — 2026-10-11

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core; predecessor32dfde2bd01f81a9c90bbd1452bbdfbd307a7d02.
Goal1 ACTIVE/full scope. New tetrahedral triangular-face dual graph uses sparse
64-bit IDs with separate contiguous graph ordinals, two work/storage constraints,
empty-rank graph exclusion, explicit parent/subset communicators and staged atomic
migration. Tetrahedral work uses sensor-only centroid volume plus actual composed
centroid/vertex operation demands; no wall tensor is spread over coarse volumes.
Weights1..1024/batches<=128 are an uncalibrated static proxy. Original CFD/sensor
stores are separate; accepted CFD solution/history transfer is not established.

partition_controls_v2 PASS: three sequential MPI1/2/4 repeats,63 partition cases,
21 weighted M=N/M<N adaptation-and-return events/42 independent frames; actual
nonMPI3 cases/1236 assertions. Exact cube tiling/graph/wire/cut/load/strictP1/
geometry/embedding/markers pass, four audit adversaries plus atomic bad-input/
storage vetoes. Within each rank choice ownership/mesh/source hashes are identical
across repeats. 6->48 cells,27 points,q.755953/J.577350/L1.732051/volume1.
Named grids and original frozen sensors:
integration_evidence/native_3d_core_v1/partition_controls_v2/mpi{1,2,4}_repeat1_meshes/.
Initial v1 PASS preserved; V2 fixes memory-control array and helper-source pins.

partition_meson_v1 PASS actual SU2 Meson strict library and actual partition/2D
geometry test objects, MPI1/2/4 independent audits; MPI1:25 cases9820 assertions.
Strict arithmetic appended after fastmath; MPI/ParMETIS dependency wiring verified.
PARTITION_CHECKPOINT.md and repeat_summary.json retain scope/phase samples.
N1/M1 one-owner partition parent median .005042s; N4/M4 .034161s with large spread;
foreign4-rank CFD was active. This is contended diagnostic evidence, no speedup/
scaling/CPU calibration claim. Requested bound~1.09MB excludes allocator/RSS and
internal(Par)METIS workspace. Private CPU/wait/full lifecycle costs unfinished.

Next: actual opt-in native3D geometry import/reader-slice return/CFD state/history
rebuild and SU2/CGNS output qualification, using existing driver transfer/lifecycle.
Production3D guard remains until connected and validated; same immutable reference
on workers,256facet/donor caps/composed3D integration/nonconvex reference limits
and preceding unisolated6-7% regression remain. No numerical 2D/MMG algorithm or
target changed; only relevant 2D geometry build checks repeated here, not full CFD.
No owned process remains live after checkpoint. Continue nice19/thread1/singlejob,
real host checks and announced full case folders. No cluster launch questions.
Publish validated checkpoints, verify full remoteSHA, preserve historical failures,
leave AdapNoExt/foreign jobs/submodules/ClusterResults alone. Goal1 is incomplete.

## Native 3D finite coplanar wall checkpoint — 2026-10-11

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core; predecessor9557213b3bbc8b1036464bc98a5030a84d449261.
Goal1 ACTIVE/full scope. Exact certified connected convex coplanar same-marker
finite disks remove artificial original facet seams. Canonical minimum facet IDs
allow wall movement/general collapse/inverse sibling pairing across those seams.
Concave/holed/uncertified components retain raw triangles; creases/markers stay.
Core source/target/BL/fade/noise/2D/MMG policies and strict q/J/L gates unchanged.

coplanar_regression_v2 PASS MPI1/2/4 (MPI1:73 cases4652 assertions;23 cases/rank
MPI2/4),1350 prior mesh/source artifacts identical. New21 independent frames/
18 events with N=M/M<N: pyramid4->2->28/30->2; finite area/marker/embedding/
original-P1 audits and4 adversaries. Grids and donor states:
integration_evidence/native_3d_core_v1/coplanar_regression_v2/mpi{1,2,4}_coplanar/.
actual_nonmpi PASS9 passive cases115 assertions and repeated cycles/repair/
rejection/composed-edge refusal; RSS5288KiB only for this small control.
coplanar_sources_v1 PASS63 frames/36 successful events/six exhausted rejections;
126 prior source750 mesh/state artifacts identical. Failed v1 adversary retained;
it accidentally reused its existing facet binding; actual meshes had passed.

Three sequential MPI1/2/4 pilots PASS with fixed mesh/edit/query work within each
rank choice and closed migration/worker/return phases. MPI1 parent medians:
coarsen4->2 .007931s; refine2->28 .023182s; coarsen28->2 .036094s. Geometry build
8facets .000136s; requested engine work~2.23MB is not RSS. More workers worsen
this tiny case and may change mesh trajectory. Three alternating actual nonMPI
checkpoint pairs match work but candidate is6-7% slower. Shared tracing/field/
metric objects identical, final addresses shifted; cause not isolated, no layout
flags. COPLANAR_GEOMETRY_CHECKPOINT.md retains all receipts and limits.
Streamed reconstruction CPU scope now excludes failure election; full private
CPU/wait qualification remains pending.

Next: tetrahedral cost-weighted partition/actual runtime/CFD/CGNS integration;
general finite-union authority and measured regression remain gaps. Production3D
guard,256-facet/256-donor budgets and composed3D edge-integrator refusal persist.
Same complete immutable geometry authority required on each worker. Do not
complete Goal1 from frozen pilots or claim BL/unsteady3D/CFD affordability.
Continue one owned job,nice19/thread1/MPI<=4; real host checks, announce full case
folders, leave foreign processes/submodules/ClusterResults and AdapNoExt alone.
No cluster launches requested tonight. Push every validated checkpoint and
verify remote full SHA; keep all failures and compact verified evidence aliases.

## Native 3D private-query checkpoint — 2026-10-11

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core. Predecessor dfe841a4ceaa0e7d453960e9afa386e28d7271f4.
Goal1 ACTIVE/full scope. Cap-only known-query private batches now complete the
previously rejected source750 wedge, general wall coarsening and interior repair.
Each proposal gets its own centroid/edge support. Geometry-first skips imports
for impossible already-admissible coarsening; all frozen gates/source/policies
remain. Full source/admission failures reject atomically; bounded query cap stays.

private_query_regression_v4 PASS MPI1/2/4,70 cases/4565 assertions MPI1,22
cases/rank MPI2/4;1350 prior mesh/source artifacts identical. actual_nonmpi
PASS9 cases/115 assertions and repeated cycles/repair/rejection. sources_v3 PASS
MPI1/2/4/M<N:63 frames/36 successful events/six exhausted private rejections;
126 before/after optimization artifacts identical; exact independent audits
and seven adversaries pass. Inspectable grids/frozen metric states in
integration_evidence/native_3d_core_v1/private_query_sources_v3/mpi{1,2,4}_meshes/.
Initial/accepted/rejected SU2 and .json included; no CFD solutions in these pilots.

Three alternating pinned source pairs preserve target/mesh/edit work. Coarsening
parent median .443577->.136109s (~3.26x) with13 rounds/one accepted/12 rejected;
batches19->7, private retries13->1. Other default MPI1 cases6-9% slower; CPU6
pairs reproduce this. A diagnostic-only relink restoring unchanged kernel
addresses restores overlapping unchanged-case timings. All unbound/pinned/
layout receipts retained, no production link flags added. Non-MPI OFF/ON pairs
against prior checkpoint overlap. PRIVATE_QUERY_CHECKPOINT.md gives full figures.

v1 unit fixture omitted facets; v2 wrongly used fixed-skin coning at a new
boundary point; v1 source audit applied the
accepted shape gate to deliberately distorted initial input. Failures retained;
accepted gates remain strict. No owned process remains live after publication.
Next: finite coplanar geometry authority, then actual runtime/weighted partition/
CFD/CGNS and full performance qualification. Source2058 needs294 donors and
still rejects; source1296 measurement is not an adaptation result. Streamed
proposal CPU scopes include election CPU; narrow before private CPU/wait claims.
Production3D guard, original seams and composed edge-integration refusal remain.
Do not complete from frozen fixtures or claim BL/unsteady3D/CFD affordability.
No cluster launches requested; user unavailable overnight. Keep one owned job,
nice19/thread1/MPI<=4, real host checks, announced full folders, and push every
validated checkpoint with a verified remote SHA.

## Native 3D closed-query filter checkpoint — 2026-10-11

Branch codex/native-3d-core; predecessor fcda83cb2b3378c55124c474e25da3aeb33f3d20.
Goal1 ACTIVE/full scope. Exact-sign Orientation settles plane separation and
endpoint containment before rational clipping; unresolved outside-endpoint
crossings retain exact roots. No target/cap/trace/BL/fade/noise/2D/MMG changes.
query_filter_regression_v1 PASS MPI1/2/4 and all independent audits (MPI1:69
cases/4542 assertions;22 cases/rank MPI2/4). All1350 preceding mesh/source files
match. query_filter_sources_v1 PASS MPI1/2/4/M<N: all72 larger-source grid/source
artifacts match prior;36 frames/18 successful events/six private cap failures;
box/thin/shear P1/closed coverage and six adversaries pass. Non-MPI build not
repeated for this MPI-independent predicate; preceding evidence stays retained.

Three alternating pinned MPI1 source binary pairs PASS with identical query,
donor/edit and mesh work. Source750 valid measurement median .617780 -> .210759s
(ranges .617252-.620717/.208175-.212063), about2.93x in this no-edit fixture.
Six-split wedge median .121119 -> .113765s; rejection .110705 -> .042506s is not
successful adaptation. QUERY_FILTER_CHECKPOINT.md and receipts/manifest give
full scope and retained hashes. No owned campaign remains live. No cluster
launch request; nice19/thread1/single job policy and real host logs retained.

Next: staged known private queries/bounded streaming to remove the whole-cavity
cap, finite coplanar geometry patches, then weighted partition/runtime/CFD/CGNS/
full lifecycle/cluster qualification. Original seams and production3D guard
persist. Do not complete Goal1 from frozen manufactured cases or claim general
scaling/BL/unsteady3D/CFD affordability from this measured local improvement.

## Native 3D query-support checkpoint — 2026-10-11

Repository /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated; branch
codex/native-3d-core, predecessor a1a23b12c4c9ccd174c8147a0d5620a9c9ade32f.
Goal1 ACTIVE/full scope. Exact closed centroid/six-edge donor imports admit
coarse measurements that exceeded whole-cell volume imports. Shared Centroid,
canonical original sensor-only P1 and actual composition/cache-hit policy are
preserved. Donor cap256/q>=.20/J>=.05/L<=1.8 stay fixed. Whole-cell imports remain
the fast path; only a one-cell cap failure gets one query-support retry. Full
private reconstruction still uses whole-cell coverage; fixed wire records remain.

query_support_regression_v3 PASS MPI1/2/4 and independent audits;68 cases/4535
assertions on MPI1 and22 cases/rank on MPI2/4. All1350 previous mesh/source artifacts
are byte-identical. Actual non-MPI9 cases/115 assertions plus cycles/repair/
recovery/exhaustion/rejection/composed-edge-refusal PASS. Final three paired
baseline/new process runs have matching work and coarsen ON medians .194757913/
.194757800s with overlapping ranges; no general speedup/scaling guarantee.

query_support_sources_v3 PASS MPI1/2/4/M<N. Sources162/384/750/1296 use162/150/
188/242 donors per coarse tetrahedron;384-source thin8/shear4 also pass. Source2058
needs294 and rejects at256. 18 successful events include12 valid coarse no-ops
and six wedge1->8/six-split refinements; six source750 private-cavity cap failures
preserve input. All36 initial/accepted/rejected frames pass independent audits;
9828 closed SAT pairs,60480 exact query-support comparisons,252 canonical/P1/
composition observations and648 analytic edge observations; six oracle adversaries.
All72 larger-source mesh/source artifacts match earlier policies. Inspect
query_support_sources_v3/mpi{1,2,4}_meshes/*initial.su2, wedge_m*.su2, dense_m*.su2,
dense_wedge_m*_rejected.su2 and matching original/source JSON.

Two rejected performance policies are retained: always-query imports (~.42s wedge
vs~.13s prior) and delayed zero-width trace rejection (.2371s coarsen vs.1951s).
Final positive-width tracing restores original early rejection while donor import
keeps closed contact. QUERY_SUPPORT_CHECKPOINT.md, receipts, identities, summaries
and query_support_checkpoint_artifacts.json give replay/provenance/deduplication.
No binaries/objects/cache or duplicate historical grids are published. One owned
heavy job, nice19/thread1/MPI<=4; actual host contention recorded. No jobs remain
from this campaign. No cluster launch requests overnight.

Next: bound and stage actual private queries where full-cavity imports exceed256,
then finite coplanar geometry-patch authority and weighted partition/runtime/CFD/
CGNS/full lifecycle and cluster qualification. Original planar seams and production
3D guard persist; reported work bounds are not RSS. Source2058 query-cap failure
also needs smaller query batching/streaming. Goal1 is not complete; do not treat
manufactured frozen probes as production/BL/unsteady3D/affordability validation.

## Native 3D larger-source checkpoint — 2026-10-11

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor7bf0eaa1c2b49edeeb3ea2d17daec1c513f743d4.
Goal1 ACTIVE/full scope. Strict cell-shaped imports exclude exactly separated
face planes only when the global original source can exceed256 donors. Smaller
sources retain cheaper boxes. Original sensor-only P1/canonical source coverage,
actual-point composition, strict q/J/L, atomic caller-input preservation and all
2D/MMG/BL/fade/noise paths remain unchanged. Fixed-size wire records remain mandatory.

large_sources_v4 PASS MPI1/2/4 and M<N: source384 imports170 per coarse tetrahedron;
12 successful events include six wedge1->8 refinements/six splits each. Source750
fails six events at the unchanged donor cap, despite admissible input meshes.
27 initial/accepted/rejected grids pass exact independent geometry/marker/P1
metrics;9828 closed SAT pairs/144 P1-composition observations/three oracle
adversaries pass. Inspect large_sources_v4/mpi{1,2,4}_meshes/*initial.su2,
wedge_m*.su2 and oversized_m*_rejected.su2 plus corresponding original/source JSON.

Full regression_v3 MPI1:66 cases4508 assertions; MPI2/4:21 cases/rank. Actual
non-MPI cycles/repair/recovery/rejection and matched timer controls PASS. All1350
prior mesh/source JSON/SU2 artifacts are byte-identical; mesh_identity.json proves it.
Unconditional filtering was correct but cost~.36s small coarsening; v1/v2 evidence
preserved, policy rejected. v3 fixed-record assumption failure preserved. Final
three alternating old/new binary pairs have matched work and coarsen ON medians
.1941285/.1951316s with overlapping ranges; no general performance claim.

Sources384 wedge event maxima .10-.13s include migration/worker comm/core/return,
with measured tracing/filtering costs. Frozen source preparation and diagnostic
export/audits are separate; actual partition/CFD/rebuild/history/CGNS absent.
Reported modeled wedge work bound<=3,149,784bytes is not RSS. Host contention
recorded; one owned heavy job at a time, nice19/MPI<=4/library threads1.
LARGE_SOURCE_CHECKPOINT.md and final receipts/summary give exact scope/replay.

Next: transformed larger-source/multiple/partial cavities, measured query-footprint
imports to remove the coarse/source750 cap limit, and coplanar finite geometry
patch association; then weighted partition/runtime/CFD/CGNS/full lifecycle and
cluster qualification. Original coplanar seams and production3D guard persist.
Do not complete Goal1 from these frozen manufactured probes.

## In-event native 3D repair recovery checkpoint — 2026-10-11

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor2bd703c513577fb0f9a7ea6d8e8c8b73c6d61303.
Goal1 ACTIVE/full scope. Refine rejects can try reconnection; invalid-field
Coarsen rejects can try movement/reconnection. One shared-index/frozen-source
repair queue handles invalid-field stagnation. Normal edge queues stay current;
coarsening resumes afterward. Primary/recovery edits share the original round
budget and atomic path. Target/q>=.20/J>=.05/finalL<=1.8 remain strict.

Exact facet preflight avoids futile movement reconstruction; private bad shape
rejects before edge tracing while actual centroid composition remains mandatory.
Scalar dirty-state reduction reuses one collective. Valid-field coarsening skips
quality-only fallbacks; V1 correctness-passing broad policy rejected for cost.
V1 pilot/source snapshots/logs/audits and V2 work comparison retained.

recovery_integration_v2 PASS MPI1 (65 cases/4493 assertions), MPI2/4 (20 cases/rank)
and actual non-MPI recovery/exhaustion plus existing operator/cycle controls.
216 successful events/324 independent meshes/612 closed parent profiles;
18 needed private fallback acceptances. All918 earlier JSON/SU2 artifacts exact.
Nine adversaries reject. Worst finalq.372093/J.095473/L1.483240. At most35 rounds,
2,166,904 modeled bytes for these fixtures. Regular serial coarsening again has
zero fallbacks and original accepted counts. No general scaling/RSS claim.

Inspect integration_evidence/native_3d_core_v1/recovery_integration_v2/
mpi{1,2,4}_automatic_repairs/restore{R,C}_*_initial.su2 and *_m*.su2/.json.
RECOVERY_INTEGRATION_CHECKPOINT.md/recovery_summary.json describe evidence/limits.
Matched non-MPI instrumentation triples preserve grid/work; source pins stable.
MPI70490/non-MPI10749 terminalPASS; no owned job live. Foreign FSI/interpolation/
mesh-motion jobs were left untouched; local timings are contended.

Next: multiple/partial cavities/larger sources, measured costs, coplanar geometry
patches; then weighted partition/runtime/CFD/CGNS/full lifecycle and cluster
qualification. Finite original coplanar seams remain protected. No new per-private
candidate donor observer; shared field transport/query controls rerun. Manufactured
source preparation is outside subset timing. Actual ParMETIS, CFD rebuild/transfer/
history, full wait/RSS remain unfinished. Production3D guard/2D/MMG/BL/fade/noise
remain unchanged. Do not call Goal1 complete from these manufactured tests.

## Automatic native 3D repair checkpoint — 2026-10-11

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor47071504dd6212828176a8e3f6627aa1d5e87a06.
Goal1 remains ACTIVE/full scope. Repair automatically selects complete failing
vertex/edge/face dependencies and bounded movement/reconnection proposals through
the shared frozen-target MPI transaction engine. Accepted input is immutable;
q>=.20/J>=.05/private progress/finalL<=1.8 remain strict. Repair-only storage is
separate and incremental; ordinary cell state avoids an added vector/whole scan.

automatic_repairs_v2 PASS MPI1 (64 cases/3969 assertions), MPI2/4 (20 cases/rank)
and actual non-MPI automatic reconnection/private movement plus existing
6->48->6->48 controls.72 automatic events (36 moves/36 reconnections),108 meshes
including36 initial,204 closed parent profiles. Independent exact embedding/
volume/facet/marker/original-P1 final audits pass; nine tampering controls reject.
All702 prior JSON/SU2 artifacts unchanged. Source pins stable. V1 native MPI1
passed; failed audit runner/parser evidence and successful re-audits retained.

Inspect integration_evidence/native_3d_core_v1/automatic_repairs_v2/
mpi{1,2,4}_automatic_repairs/auto_*_initial.su2 and auto_*_m*.su2/.json.
AUTOMATIC_REPAIR_CHECKPOINT.md/automatic_repair_summary.json document evidence,
performance and replay. Tiny events used at most19 rounds/2,134,136 modeled bytes;
rejected geometry proposals dominate several cases. Three matched non-MPI
instrumentation pairs per event pass (Repair OFF.014612s/ON.014918s medians),
with identical grids/accepted/query work. No scaling/CFD affordability conclusion.

MPI handle13448 and non-MPI49567 terminalPASS; no owned job live.
Next: repair fallback in blocked refine/coarsen, multiple/partial cavities/larger
sources, measured proposal/tracing cost, coplanar geometry authority; then
weighted partition/runtime/CFD/CGNS/full lifecycle and cluster qualification.
Finite coplanar seams remain protected. Automatic mode is separate from
Refine/Coarsen fallback. Private transport/query controls rerun in the shared core,
but this is not a new per-candidate donor-observation certificate. Manufactured
metric preparation lies outside repeated subset-event profiles; actual ParMETIS,
CFD rebuild/transfer/history, RSS and full wait qualification remain. Production
3D guard/2D/MMG/finer sensor/geometric BL/fade/noise remain unchanged.

## Native 3D movement/reconnection checkpoint — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor92bff69fc1442f0e84eddc81edee85432e5bfccf.
Goal1 remains ACTIVE/full scope. MoveVertex replaces a point's complete star at an
exact facet-contained new position with a reserved replacement ID; positive
cell/face signs preserve the domain. Reconnect reuses fixed-skin Cone for2->3,
3->2 and4->4. Generic coordinate/metric guards remain unchanged. Original finite
triangle seams, including coplanar ones, remain protected; coplanar geometry-patch
association is still required for full original planar-surface freedom.

Final repair_operators_v2 PASS MPI1 (63 cases/3773 assertions), MPI2/4 (19 cases/rank),
and actual non-MPI private metric-validated movement/reconnection plus existing
refine/coarsen/rerefine controls. Ordinary/thin/sheared domains, M=N/M<N, actual
complete face/edge/vertex imports, reservation conflicts, original-P1 sensors,
private metric repair, rank-veto/storage rollback, publication/return all pass.
108 prescribed proposals+36 initial meshes independently pass exact global
embedding/volume/facet/marker/metric audits.702 imported donor records exact;
936 canonical vertex/interior queries with max normalized residual1.10e-16 and
byte-identical worker observations.216 intentional publication rejections preserve
both mesh and directory bytes. Earlier414 JSON/SU2 artifacts unchanged.

Inspect integration_evidence/native_3d_core_v1/repair_operators_v2/mpi{1,2,4}_repairs/
repair_{face,edge4,wall,interior}_{unit,thin8,shear4}_m*_s*.su2 and donor/mesh JSON.
All306 parent profiles close, including preparation/migration/communicator/worker/
return/test audit/export. Private costs, query subcosts and communication counters
are retained. Separate directory/donor byte models are not complete caller/RSS
qualification; overlapping single-patch proposals are not a scaling/balance test.

REPAIR_OPERATORS_CHECKPOINT.md and repair_summary.json give evidence/replay/limits.
V1 native MPI1 passed; its independent parser variable-shadowing failure and
30-frame saved-log re-audit are retained. V2 strengthens nodal sensors/interior
queries. MPI handle30089/non-MPI80660 terminalPASS; no owned job live.

Next: automatic bounded repair selection/fallback, actual shape/sliver stalls,
partial cavities/larger sources and exact tracing cost; coplanar geometry patches;
then weighted partition/runtime/CFD/CGNS and complete performance qualification.
The existing Refine/Coarsen selectors do not yet invoke these new repair operators.
Production3D remains rejected;2D/MMG/finer sensor/geometric BL/fade/noise intact.

## Native 3D general coarsening and repeated cycles — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor badd73ba3a9261c4e9aa69d152a6f7ee3d6282c0.
Goal 1 remains ACTIVE/full scope. General vertex-star collapse now shares the
incremental MPI engine; no split-history requirement. Both short-edge directions
(<0.9), complete vertex dependencies, original-P1 sensors, unchanged q/J/L gates,
immutable incident finite facets, parent-owner routing and atomic rejection remain.
Movement/reconnection, larger sources, partition/runtime/CFD/CGNS remain unfinished.

Final general_coarsening_v2 PASS MPI1 (59 cases/3319 assertions), MPI2/4 (18 cases/rank),
and actual non-MPI 6->48->6->48 controls. Five ordinary/thin/sheared/spatial/turning
cases complete two refine/coarsen cycles per M=N/M<N subset. All120 event grids
plus15 initial grids pass independent global embedding/volume/facet/marker/metric
audits. 6746 imported donor records exact;1040 canonical P1 query samples with
max normalized residual9.02e-17. Every coarsening event returns6 cells; refinements
have40-48 cells. Prior54 box+90 anisotropic artifacts remain byte-identical.

Inspect integration_evidence/native_3d_core_v1/general_coarsening_v2/mpi{1,2,4}_cycles/
cycle_{box,thin8,shear4,spatial,turning}_m*_s*.su2 and original-sensor JSON.
s0/s2 refine, s1/s3 coarsen. Coarsening starts from the actual accepted fine mesh's
frozen original-P1 source, not the earlier six-cell source. Target changes only
between manufactured events (coarse sensors are one-quarter of fine tensors).

All340 parent profiles close including preparation, migration/worker communicator,
worker execution, return and test audit/export. Nested source/rejection controls
are labelled. Local private metric cost remains dominated by exact source-edge
tracing, not interpolation: box coarsening .1751/.2181 s nested trace;19 accepted,
9 rejected edits. Three OFF/ON non-MPI controls preserve geometry/work, with
coarsening medians .2149/.2096 s and unresolved timing overhead within variability.
Maximum observed rounds79/modeled bytes2374600, not size/RSS/scaling certificates.

GENERAL_COARSENING_CHECKPOINT.md and cycle_summary.json give evidence/replay/limits.
Earlier general_collapse_v1 wrong test expectation and general_coarsening_v1
wrapper-argument failure are retained with source pins/logs; v1 independent
constant-target re-audit passes15 frames. No owned jobs remain live after final
validation (MPI handle90541/non-MPI42734 terminalPASS).
Next: bounded movement/reconnection and exact source-trace cost, then larger source
controls/partition/runtime/output and full performance qualification. Production3D
remains rejected; 2D/MMG and geometric BL/finer-demand/fade/noise policies intact.

## Native 3D full anisotropic MPI refinement matrix — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor e920aedd2780c7daa8c47ce1efb3bc311ada9267.
Goal 1 remains ACTIVE/full scope unchanged. Native production code is unchanged.

Final anisotropic_engine_v3 PASS MPI1 (55 cases/2936 assertions), MPI2/4 (17 cases/rank).
Five frozen targets: heights1e-4/1e-8, sheared thin domain, fixed-rotation spatial
sizing and changing-direction nodal metrics. N=M and M<N migrate original sensors
and mesh, refine completely, and return mesh to N. Thirty completed grids have
40-48 tetrahedra; spatial/turning M4 yield42/46 vs40/44 with fewer workers.
Each independently passes exact embedding/volume/facet/marker/metric audits.
260 canonical P1 samples have max normalized residual9.02e-17; transported nodal
sensors/coordinates are exact. Gates q>=.20/J>=.05/L<=1.8 remain unchanged.

View integration_evidence/native_3d_core_v1/anisotropic_engine_v3/mpi{1,2,4}_anisotropic/
aniso_{thin4,thin8,shear4,spatial,turning}_m*.su2 and *_initial.su2. All45 frames
have retained original sensor JSON. Earlier one-round rejected grids remain in
mpi{1,2,4}_meshes/engine_rejected.su2. Prior box grids are byte-unchanged.

New parent fixture profiles include source+mesh migration, worker communicator,
worker execution/wait, return, test audit/export and other. All85 cycle profiles
close; engine/source observations nested. Synthetic metric preparation recorded
once per case. No ParMETIS/CFD/rebuild/transfer costs or full active-rank MPI wait
certificate. Single pilots not scaling evidence; do not compare changed workloads
as fixed work. Bounded-memory models are not RSS/OOM certificates.

Diagnostic fixes: canonical audit donor serialization; explicit-communicator,
single-writer fixture logs after MPI4 interleaved two long v2 ENGINE lines. Final
v3 profiles complete; raw v1/v2 and canonical re-audit provenance preserved.
All90 canonical anisotropic artifacts match, with all54 earlier box artifacts
unchanged. Runner rejects malformed profiles and retains/audits nonzero-command
records; forced-exit7 helper control exports actual saved rejected SU2 mesh.

ANISOTROPIC_MATRIX_PLAN.md and ANISOTROPIC_ENGINE_CHECKPOINT.md give scope/replay.
Next: coarsening/general repair and larger-source controls, then partition/runtime/
CGNS/CFD and closed performance/cluster envelope. These are already thin domains,
not Euler-to-BL construction; production3D remains rejected. 2D/MMG and original
sensor/finer demand/geometric BL/fade/noise policies remain intact.

## Native 3D incremental refinement and frozen scalar reuse — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch codex/native-3d-core; predecessor 0d09c431ea5b07099cf7d671c5c5f9169e4a2433.
Goal 1 remains ACTIVE with its entire original scope.

CNativeEngine3D now performs complete incremental frozen-target refinement on
explicit MPI workers, with complete dependency imports, immutable original sensor
queries, planar surface subdivision, private metric progress and atomic directory
publication. Accepted input is untouched; failure discards the private event.
Selection measures initial cells once and only published children afterward.
Owner-preserving routing is not yet weighted ParMETIS; general coarsening/repair,
anisotropic engine campaigns and production runtime integration remain unfinished.

Final engine_controls_v3 PASS MPI1 (54 cases/2891 assertions), MPI2/4 (16 cases/rank),
N=M and M<N plus actual non-MPI rebuild. Nine independently audited completed grids
have 48 tetrahedra/27 nodes/48 physical triangles from the six-tet box; q_min=.756,
J_min=.577, max metric length=1.732, all twelve original markers retained. View
integration_evidence/native_3d_core_v1/engine_controls_v3/mpi{1,2,4}_meshes/engine_full.su2
and engine_m*.su2; one-round rejected grids are retained as engine_rejected.su2.
Accepted input is byte-preserved on round/storage/options/ID-range rejections.
Donor transport/P1/composition residuals remain zero. No CFD state transfer claim.

Measured dominant cost was exact original-source edge tracing, not interpolation.
Same-target, unchanged-endpoint scalar reuse reduces MPI1 field edge integrations
819->234, MPI2 819->306, MPI4 861->358. All 54 JSON/SU2 artifacts and accepted work
match pre-reuse v2 byte-for-byte within each partition. No predicate/target relaxation.
Composed targets cannot use this sensor-only shortcut; unsupported edge integration
still rejects. Three paired MPI1 observations give engine medians .2080->.0763 s
(~2.73 ratio) on identical accepted work; small contended box, not a CFD/scaling claim.
Detail OFF/ON triples have identical work and unresolved overhead within variability.

ENGINE_REFINEMENT_CHECKPOINT.md documents code, evidence, timing scope and limits.
Engine timing excludes final object destruction; external controls include it.
Private CPU and nested query subcosts are separated; total MPI wait/lifecycle,
CFD rebuild/transfer, ParMETIS and CGNS remain unqualified. Conservative modeled
storage is not allocator/RSS certification. V1/v2 logs/pins/snapshots are retained.
Next: anisotropic/spatial engine qualification, repeated coarsening/repair, then
partition/runtime/CGNS and closed performance/cluster envelope. Production 3D
remains rejected; 2D/MMG and finer sensor/geometric BL/fade/noise policies preserved.

## Native 3D distributed immutable donor checkpoint — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor b18b5d8ecd91e5975b7a8973679ac80807cad749.
Goal 1 remains ACTIVE with its full scope unchanged.

CNativeDonor3D freezes original tetrahedra and sensor-only nodal tensors per
worker communicator. Hash-routed authority checks reject inconsistent shared
coordinates/sensors, duplicate cell IDs/canonical simplices, invalid SPD and
nonpositive cells before freezing. The house local ADT and at most eight owner
boxes per rank route bounded imports without replicating the original mesh or
scanning every original cell per candidate. Complete conservative AABB unions
above 256 donors reject; exact FrozenField containment/edge coverage remains
mandatory. Empty owners and inactive requesters participate collectively.

Final donor_import_controls_v2 PASS on MPI1/2/4: MPI1 50 cases/2841 assertions;
MPI2/4 13 cases per rank. The distributed split/inverse fixture now validates
with routed immutable donors. Independent Fraction audits verify all 42 actual
transported donor records, 28 canonical P1 samples and manufactured thin-region
composition samples: maximum residual zero. Source/cache-hit controls apply
composition at every actual query. M=N and M<N, genuine empty owners, source
retry/rejection, 257-cell owners and oversized-union rejection pass. All 30
retained grid JSON/SU2 files match the previous checkpoint. Final non-MPI
serial_donors passes; its tiny RSS is 3580 KiB, not a service memory certificate.
Seven focused auditor controls detect tampered/missing/duplicate observations.

DONOR_IMPORT_CHECKPOINT.md records scope, limits and replay. Parent Build/Import
timers are exclusive, routing/search/exchange detail nested; house transport
stats remain partial. No speedup, instrumentation overhead or scaling claim.
The source must already be embedded/conforming. AABB import does not authorize
outside-domain queries or certify candidate coverage. Modeled requested storage
is conservative, not allocator/RSS certification; hard-OOM recovery untested.

Next: incremental distributed candidate engine and repeated full metric
completion, general planar operators, tetrahedral partition weights,
runtime/CFD/history/CGNS integration and performance qualification.
Production 3D remains rejected. 2D/MMG, finer sensor, BL/fade and noise policies
are unchanged. No new papers are required for this checkpoint.

## Native 3D worker-communicator search checkpoint — 2026-10-10

Working folder: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-core; predecessor f13a08be3eefa2db8e547c10c52569d552f92ad2.
Goal 1 remains ACTIVE with its full scope unchanged.

House CPassiveComm Allgatherv and CRankBoxTree now accept an explicit communicator.
Existing callers retain the default SU2 communicator. This removes the global
collective from rank-box construction when only M of N CFD ranks are workers.
Empty send buffers avoid null-pointer arithmetic; populated buffers keep a
single range copy, without an extra zero-fill. No measured speedup is claimed.

Final worker_field_controls_v2 passes actual MPI1/2/4: MPI1 has 46 cases and
2752 assertions, including all 37 existing 3D kernel cases; MPI2/4 have nine
cases per rank. Subset controls cover M=N, M=N-1 and M=N/2, empty partitions,
127-byte rounds, and both 2D and 3D boxes while inactive ranks wait on the parent.
The final non-MPI serial_transport control also passes. Source and artifact
hashes were verified; all 15 mesh JSON and 15 SU2 files match the published
checkpoint. Independent audit results match too; their source-log hashes
necessarily differ. Earlier worker_field_controls_v1 evidence is retained.

WORKER_COMMUNICATOR_CHECKPOINT.md records scope and replay. This is a shared
routing prerequisite, not a distributed original-sensor service or an enabled
3D remesher. No full SU2/AD build, CFD regression or scaling campaign was run.
Next: immutable distributed sensor donor import/query, incremental MPI engine,
full metric completion, planar operators, tetrahedral partition weights,
runtime/CFD/history/CGNS integration and performance qualification.

## Native 3D distributed dependency/publication checkpoint — 2026-10-10

Working folder /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated;
branch codex/native-3d-core, predecessor32512ee03632c07c009b543d18f61feabf8b6dbe.
Goal1 ACTIVE/full scope unchanged. New CNativeDistributed3D reuses house passive
transport/election/wire fields on explicit worker communicators. Routed complete
vertex/edge/face stars, authoritative versioned payloads, conflict claims, staged
mesh+directory publication and common rejection. Global cell identity directory
also rejects duplicate IDs with disjoint nodes. Cached resident accounting and
caller/scratch admission avoid per-candidate global scans. Metadata protocol only:
caller must approve complete geometry/embedding/markers/metric and reserve IDs.

Final distributed_controls_v3 PASSMPI1/2/4 (8distributed cases/rank; MPI1 also37
existing3D cases,45total/2749assertions). Actual house MPI/ADT objects rebuilt.
Split+inverse publish across cyclic ownership, rejection preserves bothstores.
N=M1/2/4,2->1->2,4->2/3->4 mesh/version/facet-record migration/return PASS.
15actualMPIframes independently exact embedding/facet/marker/P1 audited, SU2
files in distributed_controls_v3/mpi{1,2,4}_meshes. Private refined/returned maxL
2.828 remainsoversized; coarsened maxL1.732 strictPASS. No complete sweep claim.
v1preserves const-map-key test helper compilationfailure; no MPI run in v1.
Firstpassingv2retained; finalv3adds repeated-build rejection, all30grid/JSON
artifactsunchanged. Rebuild after migration requires a new directory.

DISTRIBUTED_CHECKPOINT.md documents exact scope, limits, pins/replay/phasecaveats.
Directory publicphase totals exclusive; electionnested. World transportpartial
includes fixturecalls and omits internalfail-elections; not totalMPIwait/closure.
Modeled requestedstorage notallocator/RSS certification; overflowprobe SIZE_MAX
is attempted rejectedbound. Single tinycontendedruns notscaling/speedupevidence.

Next distributedoriginalsensorqueries, incrementalMPIengine/repeatedcompletion,
general planaroperators, tetrahedralParMETISweights, runtime/CGNS/CFDhistoryreturn
and full performance/memory/imbalancequalification. Runtime3Dstillrejected;
2D/MMG/finer-sensor/geometriccomposition/fade/noise unchanged. No newpaperneeded.

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
