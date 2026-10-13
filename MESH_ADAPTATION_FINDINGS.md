# Goal3 integration reaches bounded cost, but fixed angular accuracy fails — 2026-10-13

Feature partitions and conical distance-band rays reduce the hidden-wall case to
65397points within the original65536cap, preserving volume/moments/source queries.
A stronger independent parallel-body density oracle exposes0.47% error; the5e-4
gate fails. Fixed degree/order variants change error sign and do not establish
convergence. Use error-controlled angular integration next; no target/cap/gate
relaxation and no efficiency or valid-adapted-mesh claim. Five other metric/kernel
stages pass. Detailed historical/current evidence is native_3d_bl_v1/feature_partition_v1.
Whole Goal3 remains active with full construction/MPI/RANS/performance scope.

Earlier findings below are historical.

# Goal3 thin-volume check finds an algorithmic cost limit — 2026-10-13

Five latest serial stages pass, including actual native3D complexity on a coarse
cube and original-P1/BL quadrature accuracy/moments. The full new aggregate FAILS:
a finite wall missed by all coarse-cell vertices still reaches65536quadrature
points. Closest-feature separating directions resolved its earlier depth-limit
failure; direct tetrahedral rules avoid redundant coning but do not remove the
point-budget failure. This is a geometric integration/construction cost issue,
not a CFD roundoff excuse or evidence of a valid adapted BL grid. Use feature-aware
support partition/integration next; preserve targets, caps and the failing case.
Two invalid HMIN>h0 fixtures were corrected without changing configuration checks.
Actual failure sources/receipts are retained in native_3d_bl_v1/volume_integration_v1.
Production BL/analytic-distance guards stay intact; whole Goal3 remains active.

Earlier findings below are historical.

# Goal3 thin-volume/complexity implementation under verification — 2026-10-13

NativeIntegrated/codex/native-3d-bl now has3D distance-band volume quadrature using
the existing conservative-transfer convex clipper, with bounded hidden-support/
nearest-face subdivision and original-P1 samples at actual rounded coordinates.
Native complexity uses the same ApplySample/ApplyPoint BL policy; sensor nodal
transport remains separate. Initial/retained3D faceted references are threaded
through steady/window/predicted metric construction. Analytic BL authorities and
production first-height construction remain guarded. A single low-priority
127-object grouped build (77854) is live; the new five-case resumed-per-case checks
have not run. This is implementation progress, not numerical/MPI/BL/performance
qualification. Full Goal3 scope remains pending beyond the earlier kernel proof.

Earlier findings below are historical.

# Goal3 geometric BL integration in progress — 2026-10-13

Working branch codex/native-3d-bl in SU2_NativeIntegrated, based on published
Goal2 final checkpoint c43a8a3907ea2655e4ebbb6286ab16c3fa625d8b.
The3D FrozenField already caches sensors only and composes at every actual query;
its missing composed edge path deliberately rejected productionBL. New faceted
geometry distance-band breakpoints and exact-source composed quadrature are being
implemented and checked. Endpoint/midpoint-only sampling cannot resolve arbitrary
thin finite-wall bands. A focused flat-wall test also exposed existing3D spectral
cancellation: h0=1e-6 produced normal eigenvalue1.0016903085788469e12,
0.169% above the requested1e12. Independent normal/tangent inversion and a
pre-mixing logarithm fix the primal wall metric/fade; finer sensor requests are
still intersected. Final pinned checks pass19 BL cases/3486assertions
and25 field/metric-acceptance cases. No actual3DBL remesh/MPI qualification. Historical failures are retained in native_3d_bl_v1. These changes do not yet qualify native3DBL: coarse-tet
complexity, analytic wall authority, first-height/transition construction, composed
gradation and actual RANS/MPI/lifecycle/performance remain mandatory work. Keep
production rejection until the complete construction is supported. No CFD run or
speedup claim from this arithmetic checkpoint.

Earlier findings below are historical.

# Goal 2 COMPLETE locally and published — 2026-10-13

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-curved-local. Published qualification commit:
85617b0f0e0b14024bc8841bf676aa2386c2f2f5 (remote HEAD verified).
Goal2 full declared local implementation/validation is COMPLETE. No required
implementation, validation or publication work remains within this goal.
GOAL2_PUBLICATION.json records the exact qualified checkpoint.

resolved_vortex_audit_v2 is terminal PASS: six N/M variants, twelve full event
certificates. All exact geometry/conformity/no-overlap, original-P1-or-declared
source/shape/length/transport, conservation/current+BDF/positivity/new-lost-domain,
reference/output and real restart checks pass. q_min.2377832642, J_min.0774960868,
L_max1.7999493230, transported-metric residual2.1146e-16. Physics audit verifies
all six actual evolved Euler states; pressure-derived metrics refresh each event.

Current107-case/rank MPI1/2/4 regression, independent sphere/cylinder/manufactured/
solved-bump audits and direct curved MPI atomic-veto proof remain valid. Final
source/build/input verification PASS:847source files,763unchanged production,
68actual test objects,10libraries,774members and the actual binary. Primitive
source differs only in two reviewed diagnostic expressions, tested by the current
regression. Initial final-verifier path-base error is preserved and corrected.

GOAL2_FINAL_QUALIFICATION.md summarizes the capabilities, concrete grids/states,
contracts and limits. GOAL2_REQUIREMENT_REVIEW.md and the requirement/completion
JSONs cover every original requirement; no required implementation/test work
remains. No owned solver/build/audit job remains live. Both original whole-batch
timeouts/rejected experiments and actual inspectable outputs remain preserved.
Serial meshes/states: integration_evidence/native_3d_curved_v1/resolved_vortex_v3/mpi1.
Matching flow VTUs: integration_evidence/native_3d_curved_v1/resolved_vortex_audit_v1/mpi1.
Final exact certificates: integration_evidence/native_3d_curved_v1/resolved_vortex_audit_v2.

Automatic joint reconstruction is integrated; optional strip placement remains
kernel-only. No production3DBL/generalCAD/fullphysical-unsteady/force-convergence,
performance/scaling/cluster or arbitrary-mesh feasibility claim. AdapNoExt,
foreignClusterResults and dependencies untouched. Permanent AGENTS batching
cadence applies. Next goal requires its own explicit scope/activation.

Earlier notices below are historical.

# Goal 2 independent audit timeout preserved; per-event audit active — 2026-10-13

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-curved-local; previous published a9d4c0e7c70e.
Goal ACTIVE; full completion remains INCOMPLETE.

resolved_vortex_audit_v1 is terminal FAIL: source-index selfcheck, six-variant
physics and flow exports passed; whole-lifecycle stage exited124 at its1800s
wrapper deadline. No assertion error or partial lifecycle certificate was emitted.
Controller38893 and PIDs683492/683637/683638 are terminal. Original scripts,
receipt, stdout/stderr and all actual native grids/states remain preserved.

The native matrix remains terminal PASS, all six N/M variants/two events;
no native CFD/adaptation run or build is repeated. New sole owned job2009 runs
resolved_vortex_audit_v2/run.py sequentially on one sampled CPU/nice19/threads1.
It partitions the same exact audit into twelve N/M/event requests, each with the
same1800s bound, and persists complete event certificates. Reuse requires every
script/helper/input hash to match. Full aggregate PASS requires all twelve.

preparation_validation.json proves function AST identity except progress print
calls against the original auditor;81source-box equivalence tests PASS. Additional
progress messages identify completed geometry and source/length stages. No sensor,
geometry, conservation, quality, sliver, length or work contract changed. No broad
suite is needed for this audit-only scheduling change; current production remains
identical to the qualifying107-case/rank regression.

Next: poll job2009, inspect each terminal certificate and any actual rejection,
complete the whole original requirement review, update final handoff/findings/
audits and push before declaring Goal2complete. No cluster/performance/generalCAD/
production3DBL/fullphysical-unsteady/forces or arbitrary-mesh feasibility claim.
AdapNoExt, foreign ClusterResults and dependencies untouched.

Earlier notices below are historical.

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

## Goal2 local curved-boundary work activated — first coverage study

Branch codex/native-3d-curved-local, base23197eba1715f44d61c9ad3a070f61441dfb0560.
Goal2ACTIVE; no cluster/performance/large-wing qualification. Coverage_study_v1
passes MPI1/2/4 with the actual68-object driver and unchanged production archives/
other67test objects. Unit-sphere octant projection leaves original tetrahedral
coverage; a manually prescribed three-tet exterior source admits it before freezing,
keeps original interiorP1 unchanged, integrates the sensor edge consistently and
still rejects outside its envelope. This is a coverage control, not a production
padding scheme, curved geometry/MPIintegration or actualEuler capability.

User correctly pointed to2D: existing FieldPatch already implements originalP1,
separate roundoff admission, associated-boundary-edge trace withinADAP_HAUSD and
actual-queryBLcomposition, with MPI donor padding and wrong-component/distance
controls. These existing2Dtests pass in the new selection. Prefer reusing this
bounded associated-trace principle for triangular faces, with consistent exterior
edge integration and geometric provenance; Queryfallback alone is insufficient.
Existing barycentric/conservative transfer handles nearest boundary stencils and
newly covered/lost domains. Polyline2Dauthority does not recover unknown curvature.
All4new integrated completion gates remain pending; see native_3d_curved_v1 maps/
audit. Goal1 remains complete; original closedplan preserved. No tolerance weakening,
new dependencies or production behavior change in this first study. No job remains.

## Goal1 passes revised local-only qualification — 2026-10-12

All7local requirements pass LOCAL_GOAL1_COMPLETION_AUDIT.md/json; qualified source/evidence is pushed at
f782ddc4b20703461b3c3b06192126879639d55a. Goal1 is complete under the revised scope. Performance and ALL cluster work were explicitly
excluded by the user; historical wider-resource gates below are superseded for
this closure, not demonstrated. Actual106core/39native2D cases MPI1/2/4, bridge/eight
distributed audits per size, MMG2D/3D,6actual metric/Hessian lifecycle frames and whole
three-event driver/output/state/restart cases pass. Productive serialbatch1 phases
615/790/1002rounds complete under the shared finite work policy, with no cached
metric violations/pending edges. Defaultbatch8numeric mesh/state artifacts exactly
match the qualified baseline. No quality/metric/geometry relaxation. Actual native
clock/work refusal and one-rank acceptance veto retain current/BDF state; failure
is collective fatal, without graceful retry or general configuration rollback.

Native now distinguishes certified mesh progress from finite rejected-direction
exploration, has typed geometry/metric/shape/progress/resource/work/time diagnosis,
bounded representative cavity samples and optional cfg shared work/deadline caps.
Source-only P1, finer requests,2Dactual-queryBL/fade andNOISE0 remain intact;
3DBLexplicitly rejected. Local limits: manufactured faceted boxes/wedges, bounded
operators/donors; no general-feasibility, curved/CAD, production3DBL, physicalCFD/
unsteady, performance/scalability/affordability claim. History beyond4096growth
was reviewed but not experimentally exercised; current789accepted splits fit4096.
All original fixture/parser/import failed receipts preserved; reuse of complete
serial certificates is by exact artifacts plus fresh current logs/output checks,
with a fresh MPI4/M2thinCGNS/two-fileBDFrestart certificate. See final audit/handoff
for exact source/build identities and inspectable grid folders. Goal2 not started.

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

## Cluster compilation storage failure localized — 2026-10-11

Branch codex/native-3d-transport-accounting, predecessor
d509e41870c0f5c67ed7747fac68d738403652e8. Production/test/build/tool code unchanged.
Cluster Ninja reached step672/926 before EDQUOT writing output_CAdjHeatOutput.cpp.o.d
and EDQUOT/ENOSPC writing that object and output_CBaselineOutput.cpp.o. Timestamp
21:47:56CEST. User Default quota252.88/300GiB, aero Default5.15/10TiB, chunk-file
limits unlimited. Both checkout and build directory patterns use Default(pool1),
so the aero flash799.87/800GiB observation is not a proven cause. No full cluster
build/link/numerical run is qualified. Preserve partial build/logs; resume the
same Ninja build after storage recovery. README includes target-level read-only
checks and an administrator diagnosis request, without deletion/migration or
new builds. Server enforcement lists, target capacity and installed BeeGFS
version are unverified; no cluster command/job was launched by this session.

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

Inspectable native result folder:
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

## Native 3D reader return memory admission — 2026-10-11

NativeIntegrated / codex/native-3d-core; predecessor4c9f56b8c861433158e58459ee0d5c157742bb9b.
READER_REPORT.md / READER_CHECKPOINT_VALIDATION.json qualify recoverable native3D
reader admission for point rendezvous, ownership signatures, linear points,
replicated volume rows and complete root boundary gather. Native assembly maps/
vectors now charged; old output and input capacities retained. Budget failure
is collective and does not publish. Legacy2D/MMG reader calls remain unbudgeted
with no added budget collectives; targets/interpolation/actual-query composition
and frozen criteria unchanged. Bounds use current STL tree/growth assumptions,
not allocator/MPI/ParMETIS/CFD internal physical memory.

Actual42-stage full runtime PASS MPI1/2/4,84 main+36 computed-sensor accepted
cycles,42 OFFON controls/42 SU2CGNS readbacks,357 closed/nested lifecycle rows.
126 mesh frames/84 transported current-BDF-metric frames/36 computed-metric frames
identical to predecessor. Native2D/MMG/default/refusal regressions pass. Initial
interrupted build retained. Reader-only11-stage follow-up adds all-key-to-root
routing and late linear-point refusal/retry; production archives identical, only
test source changed. N4 root model18,258B vs10,419B other ranks; not a peak-RSS
certificate. Main max model8MiB/lifetimeRSS37,904,384B. Reader admission adds
three count Alltoalls/nine scalar Allreduces, timed within reader/return stages;
full traffic counters incomplete. Matched three-repeat phase samples retained,
but foreign FSI variability prevents isolated overhead/speedup/scaling claims.

Goal1 ACTIVE: internal-memory/cache calibration, MPI wait/house traffic, practical
larger/rotated/thin/restart/rank-size limits and closed SGE campaigns remain.
Inspectable new cases reader_runtime_v2/lifecycle_mpi4 and mpi4; handoff/evidence
kept. No overnight cluster-launch requests, foreign work untouched.

## Native 3D per-rank lifecycle profiling — 2026-10-11

NativeIntegrated / codex/native-3d-core, predecessord5df2785676e610b2dcaec2c5aee7ea93ceb9670.
WRT_PERFORMANCE native3D records actual metric/Hessian construction, CFD return
ParMETIS/migration/geometry, solver/current-BDF replacement and mesh/reference
output. Per-rank wall/processCPU closure and nested source/partition/query/engine
counters; private CPU labelled separately from processCPU, lifetimeRSS separate
from native buffer model. Stage own emission excluded, parent includes nested
emission; no maxima summation or elapsed-minus-CPU MPI wait inference.
PROFILE_REPORT.md / PROFILE_CHECKPOINT_VALIDATION.json: actual42-stage MPI1/2/4
PASS,84 main+36 fresh computed-sensor lifecycle accepted events.126 predecessor-
identical main frames/states;36 strict new frames,357 closed rank-stage rows and
357 nested rows. Three OFFON repeats per M1/2/4<=N; native2D/MMG/default/budget/
private-progress rollback pass. Compile static-dimension and invalid equal-size
fixture failures retained and fixed, no production numerical contract relaxed.

Serial ON sample refinement45.48ms/coarsen14.83ms, metric17.49/reconstruction6.80
inside refinement. N4/M4 OFF0.175–0.466s, ON0.178–0.431s on contended host; no
scaling/speedup/affordability or reliable distributed instrumentation-cost claim.
Goal1 ACTIVE: MPI wait/house traffic/internal memory, practical larger/rotated/
thin/restart/rank-size envelope and closed SGE work remain. Scheduled unsteady
metric sampling detail incomplete. Inspect profile_runtime_v2/lifecycle_mpi4
actual computed sensor/mesh JSON+SU2 sidecar, and mpi4 donor/accepted BDF/metric
JSON+CGNS. No overnight cluster-launch requests or foreign work touched.

## Native 3D runtime memory admission — 2026-10-11

NativeIntegrated / codex/native-3d-core; predecessor0864f56e60f0f7d66e5db6cf8779cb8aaa1703c7.
Recoverable initial point-directory/face/reference gates and caller-held native
runtime storage now compose through existing requested-buffer helpers. Import
refusal leaves output/reference unchanged. Move imported cells, borrow original
source without repartition and release superseded maps/operation records before
coarsening. NATIVE3D rows distinguish modeled requested bytes from lifetime RSS.
MEMORY_REPORT.md / MEMORY_CHECKPOINT_VALIDATION.json: actual archive build and
33-stage runtime PASS MPI1/2/4, allM1..N; budget elected refusal/retry;84 accepted
events/42 OFFON pairs/42 output roundtrips,126 predecessor-identical mesh frames,
transported tensor error0/max affine current-BDF2 error1.119683e-12. Native2D/MMG
and default ownership/private-progress rollback also PASS. No target relaxed.

Largest model8MiB (reference setup allowance), matrixRSS38,666,240bytes; different
quantities. Reader/partitioner workspace, allocator/MPI/CFD storage and remaining
reader/cache calibration are not fully covered. Full lifecycle profiling and
size/rank/restart qualification remain open. Goal1 ACTIVE, no scaling/speedup or
BL/unsteady affordability claim. Inspect memory_runtime_v1/mpi4 final SU2/CGNS,
original reference and donor/accepted metric/current/BDF histories. All owned
checks sequential nice19/thread1; foreign FSI untouched. Push qualified checkpoint.

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

Cap-only known-query batching now completes source750 refinement/coarsening/repair;
geometry-first removes unnecessary donor work for impossible admissible collapses.
PRIVATE_QUERY_CHECKPOINT.md and receipts retain the63-frame MPI1/2/4/M<N matrix,
1350 prior mesh identities,126 optimization identities, actual non-MPI controls,
three alternating timing pairs and executable-layout diagnostics. Default coarse
removal median .443577->.136109s; other MPI1 binaries6-9% slower, attributed in
part to layout by controlled identical-object relinking. No production linker
flags; full performance/CFD/runtime/ParMETIS/CGNS qualification remains pending.
Goal1 ACTIVE/full scope; source2058 cap, finite facet seams, production3D guard
and composed edge-integrator refusal remain. Frozen targets and 2D/MMG untouched.

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

## Native 3D original sensor/query foundation — 2026-10-10

FrozenField now evaluates immutable original tetrahedral SENSOR-only P1 tensors,
with magnitude-controlled barycentric fallback, canonical donor keys, actual-query
composition on cache hits, bounded FIFO and existing ADT pruning. No composed-wall
interpolation, tensor reset or target clipping. PASS22cases/2294assertions+290 signs
in native_3d_core_v1/field_controls_v4.210 independent actual-binary64 Fraction
queries across rotation/aspect/scales/translations PASS in field_oracle_v3:
weighterror8.1262e-17, directional tensordefect1.5378e-16. Oracle versions byte-identical.
Profile counters include rejected composition work; query child scopes exclusive.
No tree for a singleton donor, one candidate among256 separated donors, no speedup
claim. Exact-only containment and local patches: physical extension, integration
breakpoints, real3D BL provider, metric-gated remeshing, MPI and full-event cost
remain pending. FIELD_CHECKPOINT.md explains source/failure retention and scopes.
Goal1 active; native support still rejects3D.

## Native 3D bounded cavity closure — 2026-10-10

Private coning now handles insertion, interior movement/removal and some
reconnections with fixed oriented interfaces and manifold sphere/disk links.
A stronger rotated-thin test reproduced false rounded-volume closure rejection
at1e6/1e8/1e10. Exact oriented-face/coordinate matching closes signed volume
algebraically; replacing the rounded sum preserves strict positivity/topology.
Failed v2 source/logs retained. PASS16cases/163assertions+290 exact Fraction signs
in native_3d_core_v1/cavity_controls_v3. Independent8probe Fraction geometry/SAT
checks and inspectable SU2 grids in cavity_meshes_v2. Five constant-metric controls
pass .20/.05/1.8 gates; three extreme geometry-only cases deliberately fail
isotropic shape gates. Same10 earlier SU2 files after correction. No full runtime,
physical surface, spatial metric, MPI or performance envelope claim. Full details
and legacy replay references in CAVITY_CHECKPOINT.md. Goal1 remains active.

## Native 3D incidence and local paper design — 2026-10-10

On codex/native-3d-core, canonical indexed supplied-cell face/edge/vertex incidence
now rejects malformed identity/coordinates, nonpositive cells and incompatible
shared faces. Six-cell cube star tests demonstrate why face adjacency alone is
insufficient for edge/vertex dependencies. PASS10 cases/93 assertions and290 exact
orientation signs in native_3d_core_v1/topology_controls_v1. These are isolated
controls; MPI completeness, vertex links, embedding, replacements and adaptable
physical surfaces remain pending. Exact provenance and local Loseille2017,
Tsolakis2021 and Galbraith2020 reading scopes retained with design references.
Build O(k log k), storage O(k), lookups O(log k + star); do not rebuild global
incidence per proposal. No full driver/MPI test or performance claim. Goal1 active.

# Mesh adaptation findings and follow-up work

## 2026-10-10: native3D geometric foundation starts Goal1

User activated first3D goal after the published2D checkpoint. Separate branch
codex/native-3d-core. Isolated strict kernel adds exact tetrahedral sign, stable
SPD admission/metric measures and initial mean-ratio/sliver gates .20/.05.
SixCatch cases40assertions and290 independent Fraction signs PASS locally;
172exact fallback/118filtered cases. No fullSU2build/CFD/MPI/3D remeshing;
no affordable mesh-generation or boundary/BL capability claim.2D sources and
backend support guards unchanged. Initial Python3.8 runner failure retained.

User-provided MeshAdaptation/Scripts contains useful coupled metric-coordinate
movement,2->3/edge-ring reconnection, edge-star insertion and embedding/closure
checks. Full3D repair controller requires fixed boundaries/MMG3D; its transported
log-interpolated donors and source quality exceptions are not native contracts.
Source review/pins: integration_evidence/native_3d_core_v1/SCRIPTS_REFERENCE.md.
Actual geometry test folder: integration_evidence/native_3d_core_v1/geometry_controls_v2.
Goal1 remains ACTIVE; complete MPI/remeshing/performance gates remain ahead.

## 2026-10-10: strict frozen matrix582358 closes the 2D reuse checkpoint

All16 remeshes/eight same-binary OFF/BOTH pairs PASS across both workload
directions and N4/M4NO/M4YES/M3YES/M2YES. Meshes/all4 transported tensor files
and operation/selection counts match byte-for-byte within each pair. Independent
audits PASS; focused native MPI1/2/4 suites63 tests/rank PASS. Saved-data reviewer
checks1259 unique identities and profile/count/timing closure without CFD/MPI
or heavy mesh-audit reruns. Report:
integration_evidence/native_cluster_reuse_diagnostic_v1/cluster_review_582358/REVIEW.md.

Euler-to-BL reuse improves remesh4.45%/4.26% atM4,0.54%/1.71% atM3/M2.
BL-to-Euler evaluation savings0.32-0.37%, no convincing time benefit.
BOTH Euler-to-BL M4NO29.575s is fastest; BL-to-Euler weightedM4=10.292s.
Reduced M improves imbalance but increases total time on these workloads;
partition trajectories/mesh sizes differ, so this is not fixed-work scaling.
48 other compute processes in samples and one ordered repetition limit timing
claims. Private imbalance and expensive actual queries remain; validation
includes MPI wait and query subcosts are not isolated.
Qualitymin.1841786634, lengthmax1.7999995295, BLheightmax4.62974e-12,
transported directional defectmax7.04577e-11. The frozen2D pre-3D checkpoint
is complete; actual unsteady/source-change lifecycle and composed-field
gradation remain separate. Four prepared 3D goals and paused tracker unchanged.

## 2026-10-10: four native 3D development goals prepared

Planning only: NATIVE_3D_DEVELOPMENT_GOALS.md defines tetrahedral MPI/planar
surface adaptation, curved-surface/Euler lifecycle, coarse Euler-to-BL and
BL-to-Euler, then repeated unsteady 3D adaptation. Complete adaptation costs,
query subcosts, partition/migration/return/transfer, memory, imbalance and measured
practical limits are required at each stage. Proposed CFD overhead targets are
R<=0.20 and minimum affordable R<1; no achievement is claimed.
User now prefers small sequential local cases before cluster campaigns.
Contention checks and resource limits remain; no execution or goal activation.
Geometry remains adaptable from the first milestone. Existing 3D metric,
transfer and output support does not establish native tetrahedral reconstruction.
The pending strict 2D matrix remains the regression checkpoint.

## 2026-10-10: runtime reuse preserves outputs in diagnostic582344

All7 frozen Euler-to-BL N4/M4 NO numerical audits PASS; focused native MPI1/2/4
63 tests/rank PASS. Same executable OFF, SCORES, METRIC, BOTH and AUDIT yields
byte-identical mesh and four transported-tensor files. Fresh sampled metric
checks540894 and scalar/star checks424 find zero bit mismatches. Operation/
selection counts equal across all7 roles. Historical control still differs from
new OFF, while archived reuse candidate equals new BOTH. Runtime caching does
not explain the historical discrepancy on this case. Arithmetic/build/source
context strongly implicated; a specific compiler transformation is unproved.

Existing41 low-bit coordinate/43 tensor changes have no demonstrated practical
consequence: maxpoint displacement3.145e-15, all shape/height/reference/transport
contracts pass. Do not spend another campaign pursuing historical byte identity
as accuracy. Keep that failed comparison in evidence and use same-executable
OFF/BOTH for strict operator-invariance validation; numerical tolerances remain
unchanged. No production correction is justified by this diagnostic alone.

OFF->BOTH remesh28.9314->27.5512s(-4.77%), privateCPU24.0507->22.7048s(-5.60%).
Score-only27.5769s, metric-only28.8401s. Most reduction is avoided cache hits;
new-query work and max/mean private imbalance~2.23 remain. Zero other visible
compute processes in samples and verified serial UTC intervals do not establish
exclusive hardware; one frozen workload is not a speedup/scaling certificate.

Review verifies1045 unique saved source/tool/data/log hashes and records scopes,
counts, audits and historical differences in
integration_evidence/native_cluster_reuse_diagnostic_v1/cluster_review_582344.
Original data unchanged; no local solver/build/MPI/heavy audit. Next strict
16-case same-binary matrix covers both coarse Euler-to-BL and BL-to-Euler with
M4NO/M4YES/M3YES/M2YES; package prepared and fake-file tested, not yet executed.
No rebuild needed: orchestration/docs only; prepare guards the tested binary,
unchanged diagnostic C++/build pins/options, and uses a separate exclusive
checkpoint. Full unsteady lifecycle at this source, larger-scale efficiency,
converged viscous quantities and composed-field gradation remain separate work.

## 2026-10-10: bounded same-executable reuse diagnostic prepared

Existing reviewed meshes pass the numerical audits; roundoff/compiler causality
remains unproved. New package `integration_evidence/native_cluster_reuse_diagnostic_v1`
preserves both tested executables and compares archived control/candidate with
same-binary OFF, SCORES, METRIC, BOTH and sampled fresh-hit AUDIT on the first
failing frozen Euler-to-BL N4/M4 NO workload. MPI1/2/4 units precede seven sequential
cluster remeshes. Numerical audits remain mandatory; identity/count differences
are diagnostic observations. DIAGNOSTIC_COMPLETE is not byte identity, a mesh
correctness certificate beyond the recorded checks, or a speedup claim. Ordinary
comparison gate and all numerical/BL policies unchanged.

Audit excludes authoritative seed values, compares identical ordered-coordinate
metric/scalar/star samples with fresh evaluations, reports bounded first mismatch
locations and collective counts, and returns cached values unchanged. Eight hits
per private cache/configuration are sampled, so zero mismatches is not exhaustive
proof. Extra recomputations can change residency/roundoff and add admitted scratch;
audited timings are not used for performance. Historical control vs rebuilt OFF,
and previous candidate vs rebuilt BOTH, expose code/build-context differences.
Source changes still require cluster build/tests; fake-file package4, prior reuse10
and balance8 checks pass locally without solver/MPI/build runs. Failed evidence
and previous reports preserved. Resume the original strict full matrix only after
this discrepancy is explained or fixed; BL-to-Euler/weighted cases remain untested.

## 2026-10-10: reconstruction reuse passes numerics, fails frozen byte identity

Cluster582331 passes the20-stage MPI1/2/4/unsteady Euler/RANS/CGNS/restart gate.
Frozen paired campaigns582333/582334 stop after the first Euler-to-BL pair:
all numerical audits and operation counts pass, connectivity/markers identical,
but41 coordinate rows and43 tensors differ by reproducible low bits(maximum
coordinate displacement3.15e-15; maximum tensor-entry difference over tensor
scale1.04e-13). Four tensors differ even at unchanged coordinates. The byte
comparison remains FAIL; later14 cases per matrix were not reached.

Requests decrease33.2%, actual evaluations only2.8%, private CPU3.1-4.8%; most
removed calls were hits and the expensive new-query work remains. Shared-node
timings and incomplete matrices do not establish a speedup/scaling result.
Global fast-math/inlining is a plausible arithmetic explanation, not a proved
root cause. Separate runtime score/eviction reuse and cached/fresh evaluation
with the same executable before further scaling or production fixes. Original
reports, meshes, failures and strict gate are retained; no guard relaxed.

Review/evidence: `integration_evidence/native_cluster_reconstruction_reuse_v1/cluster_review_582331_582334/REVIEW.md`.


## 2026-10-08: matched RAE2822 recovery adaptation cycle

One matched native RANS cycle passed mesh, geometric BL/frozen-metric, transfer
admissibility and saved-field audits for both GG operators. With ~15900 points
and 2000 resumed iterations, new recovery gives a 4.72x smaller SA residual;
both density criteria pass, neither SA criterion passes. Quality is unchanged
and force accuracy is not established. A single timing pair is 139.8 -> 130.4 s.

The larger performance target is graded complexity work (~6000 sweeps per
pre-remesh metric) and duplicate steady metric evaluation (~18-19 s each).
The 80-sweep sensor limit and composed transport residuals remain explicit.
The same shared remesher was pinned; newer main remesher changes were excluded.
See `Papers/ADAPTATION_CYCLE_RECOVERY.md` and the cycle validation receipt for
measurements, limits and next actions. The original-wall sidecar is now reused
by the continuation auditor; production policies are unchanged in this step.

## 2026-10-08: recovery geometry reuse and supported QR wall bias reduction

GG P1 recovery evaluates geometry/contributions once per cell/pass and scatters
in global-ID order, retaining whole-star fallback on mixed/invalid cells.
Six paired M6 measurements give median 0.570 -> 0.394 s (31% less recovery time),
with 11.52 MiB temporary payload at the maximum tested pass; no persistent
geometry cache or extra exchange. Whole-process timing varies under contention.
The method and simplex-only scope are unchanged; floating reassociation can
change low bits and amplify ill-conditioned metric differences versus the old
implementation, but original derivative/MPI/SPD/bounds/complexity gates pass.

QR's grown thin wall stencils can add selected tangent/mixed quartics only when
sample count, rank and condition gates support them. Curved 3D wall-normal error
at n=32 falls from 2.08% to 1.00%; central interiors and all tested 2D QR fields
are unchanged. Two-ring 2D stencils cannot support the required coupled model.
A smaller pure-tangential model passed an exact polynomial check but failed
existing curved-wall convergence; that variant is preserved and rejected.
No existing convergence or reliability gate was weakened.

The accepted QR extension costs about 1.75% more recovery time in the paired M6
campaign. M6 frozen fields are unchanged, so this is a manufactured derivative
improvement, not demonstrated M6 flow accuracy. Noise off/on RAE/M6 partition
checks pass; neighborhood traffic is unchanged. Final units: 151 serial cases,
400078 assertions; 35 MPI-safe cases per rank; AD syntax and 36 matched cases pass.

Details, publication basis, limitations and deferred work are in
`Papers/RECOVERY_GEOMETRY_WALL_BIAS.md`. Element support and the later convective/
viscous transfer request remain in `GRADIENT_TRANSFER_FOLLOWUP.md`. Cycle tests
should use an integration candidate and baseline on the same accepted remesher
revision, then be repeated after merge on main; the other agent's latest branch
is ahead of our common base and is not an isolated metric control.

## 2026-10-08: deterministic simplex recovery for adaptation GG

Primal adaptation GG now uses a volume-weighted P1 simplex projection through
both recovery passes, reusing the existing GG fallback, symmetry rules and MPI
exchange. Physical-boundary affine gradient errors on stretched/curved 2D/3D
meshes fall from 18.18/121.04 to below 3.4e-12. At n=32, curved wall-interior
Hessian errors fall from 80.35/673.29 to 1.779/6.400 (45/105 times smaller).
Central-interior accuracy is preserved; quadratic/graded-wall bias remains.
QR remains more accurate at these walls. This is not an aerodynamic certificate.

M6 Mach/pressure Hessians match bit-for-bit at 1/2/4 ranks and metric differences
are below 6e-15. The frozen RAE audit passes using the original geometric BL
policy. WLS/QR outputs are byte-identical. The 150 serial cases (400061
assertions), 34 MPI-safe cases per rank, 36 manufactured comparisons and
forward/reverse AD syntax probes validate this checkpoint; AD runtime and
large-scale hybrid/MPI scaling are still unverified.

The first boundary-only fix failed M6 MPI checks. Interior difference sums
improved the pressure-Hessian discrepancy from 1.4e-7 to 1.1e-10, but its
metrics still differed by 7.1e-6. Deterministic original-cell stars eliminate
that geometry/accumulation dependency. Both rejected patches and receipts are
preserved; gates were not relaxed. This evidence justified extending the
projection into the interior instead of relying on dual-face geometry.

Performance cost is explicit: M6 serial Hessian median 0.0501 -> 0.5488 s
(about 11x; +0.499 s per metric computation); four ranks 0.282 s. RAE serial
0.00583 -> 0.01936 s. Whole-process M6 timing does not distinguish an overhead
under contention. There is no persistent geometry cache or additional exchange.
Profile repeated cell geometry/serial accumulation before scaling or flow transfer.

Detailed method, publication, evidence and limitations:
`Papers/GG_BOUNDARY_RECOVERY.md`. Source/evidence are pushed on
`codex/metric-robustness` and durable artifacts saved under
`integration_evidence/metric_robustness/gg_simplex_recovery_v1`.
Next: higher-order/distance-aware boundary recovery on multiple topologies,
performance profiling, fixed-complexity adaptation/flow cycles. The user's
future convective/viscous low-quality-grid investigation is explicitly retained
in `GRADIENT_TRANSFER_FOLLOWUP.md`; current recovery opt-ins do not modify those
flow operators.

## 2026-10-08: Clément comparison and boundary-donor limitation

Completed the paper-based benchmark with 36 matched manufactured datasets
(2D/3D; straight/curved layers; n=8/16/32; GG/WLS/QR), plus independent Clément
recovery and two controlled constant boundary extensions. The replayable source
is `integration_evidence/compare_hessian_recovery.py`; the native pressure-sensor
export case is `[HessianRecoveryComparison]`. Production code/defaults and the
main branch's geometric BL policy are unchanged.

On the tested simplex meshes, native GG and Clément agree in the central
interior within 1.2e-7 in layer coordinates. At n=32 on the curved 3D case,
Clément/GG interior maximum error is 0.01350 versus QR 0.05482 (about four times
smaller); in curved 2D QR remains better (0.003045 versus 0.006505). There is no
universal winner. WLS errors increase with refinement on these stretched,
rotated, graded examples; this remains a recovery limitation.

Raw GG wall-interior maximum errors are 80.35 (2D) and 673.29 (3D); raw Clément
reduces these to 1.779 and 6.400, and deeper-donor constant extension to 0.2233
and 0.3006. These are benchmark derivative comparisons, not implemented
production gains. QR gives 0.004561 and 0.08304 there. Constant extension is
boundary-exact on the flat examples, but its curved near-wall normal errors
plateau at roughly 5.6%/7.5%. It leaves adjacent interior Hessians unchanged.
A runnable 1D noise-free quadratic calculation on y_i=(i/n)^1.3 retains 4.4%
curvature bias at fixed donor ring 2 for n=8/16/32. Interior status alone is an
insufficient donor-quality test; noise filtering and metric gradation cannot
certify that this bias has disappeared.

Frozen RAE/M6 comparisons cover all 919/10837 physical boundary vertices and
produce finite tensors, with substantial disagreements from the original WLS/QR
fields. There is no exact CFD Hessian/output reference, native Clément MPI
validation or adaptation-cycle result. The NumPy prototype takes approximately
4.5 s on M6 and caches 44.4 MB of geometry; these are not native C++ method-cost
comparisons. Detailed source sections, accuracy tables, replay instructions and
limits are in `Papers/CLEMENT_COMPARISON.md`.

Evidence is saved physically in SU2_AdapNoExt under
`integration_evidence/metric_robustness/clement_comparison_v1`. Next: correct
and verify the GG adaptation boundary gradient operator through both recovery
passes, compare higher-order/distance-aware boundary reconstruction, profile QR
fallback preparation, then validate accepted choices with native MPI and fixed-
complexity adaptation/flow cycles. Reuse the existing GG interior operator
instead of introducing a redundant full-mesh Clément backend.


Working branch: `codex/metric-robustness`, fork https://github.com/rois1995/SU2.
Rebased onto `codex/native-metric-integration` at `2abbd11769` on 2026-10-07.
The other agent owns BL composition, geometric complexity and gradation; this
branch continues Hessian/sensor-metric work and validates the shared BL policy.
See `METRIC_ROBUSTNESS_INTEGRATION.md` for the current implementation contract.
Older BL experiments and their receipts below are historical evidence.
This file retains the investigation, implementation status, and deferred work.
Support statements in older entries describe their dated checkpoints.
Historical baseline implementation: `e17a96d301`; detailed measurements and replay
script are in `integration_evidence/metric_robustness_v2_validation.json` and
`integration_evidence/check_rae2822_metric_constraints.py`.

## Literature assessment and QR symmetry support (2026-10-08)

Source commit `747f0ef6ea` allows direct QR sensor recovery with symmetry markers.
After fitting, the existing scalar-gradient and vector-gradient symmetry rules
remove the normal gradient and mixed normal-tangential Hessian components while
retaining normal-normal curvature. The implementation reuses boundary normals and
the existing Hessian-gradient workspace; no new stencil ring, MPI exchange or
full-mesh Hessian buffer is added. Euler and viscous wall behavior and the shared
geometric BL composition remain unchanged. Periodic, goal and differentiated QR
configurations remain rejected; custom sensors must obey the existing mirror-even
contract. General curved/nonorthogonal symmetry cases are not independently
certified by this campaign.

The supplied PDFs identify an important alternative: Alauzet and Frazza 2021,
Section 7.3, use a two-pass volume-weighted Clément recovery, not our QR wall
correction. Vallet et al. 2007 discuss direct quadratic fitting and expanded
boundary patches. Diskin and Thomas 2008 investigate directional enrichment and
distance-based mapping for interior gradients. Galbraith et al. 2020 expose the
cost of weak boundary recovery and compare boundary extrapolation. These are
concrete benchmark candidates; the papers do not establish a universal winner or
validate our exact partial cubic basis and thresholds. Details and section
pointers: [Papers/IMPLEMENTATION_ASSESSMENT.md](Papers/IMPLEMENTATION_ASSESSMENT.md).

New tests cover oblique 2D/3D planes, two orthogonal planes, Euler-wall junctions,
signed normal curvature and a nonquadratic mirror-even sensor, with noise zero
and one. All 148 selected serial cases passed (394550 assertions); all 32 selected
MPI-safe cases per rank passed at two/four ranks. Maximum transformed Hessian
errors for the manufactured quadratic at symmetry points, noise zero:

| Manufactured geometry | WLS | QR |
|---|---:|---:|
| Rotated stretched 2D, one symmetry | 1.5 | 1.31e-10 |
| Rotated stretched 3D, one symmetry | 1.65667 | 8.93e-10 |
| Rotated stretched 3D, two symmetries | 4.36364 | 8.93e-10 |

These are absolute errors in local layer coordinates with known quadratic
derivatives, not aerodynamic error reductions. The new support makes the real
ONERA M6 RANS/SA fixture usable by QR. Six frozen wing runs (noise zero/one at
1/2/4 ranks, 96252 points and 545438 tetrahedra) passed unchanged-state, SPD,
size/aspect, independent complexity, symmetry-Hessian and strict MPI checks.
All 192504 point-sensor fits succeeded without WLS fallback; 169 used cubic terms.
Hessians are identical across partitions; maximum metric relative MPI differences
are 8.47e-15 (noise zero) and 2.02e-15 (noise one). At 8112 symmetry vertices,
relative mixed-curvature residuals are below 7.2e-16 while nonzero normal curvature
is retained. A serial RAE QR/noise-zero regression produces a byte-identical
`fields.csv` to the preceding wall-recovery implementation, including sensor,
Hessian, metric, solution and coordinates. Both AD header syntax probes pass;
this is not full differentiated solver validation or AD QR support.

Performance has a real tradeoff. Three alternating paired serial frozen M6 runs
give median Hessian time 0.176376s for WLS and 2.50669s for QR/noise zero (about
14.2 times); median whole-process times are 9.37608s and 11.66839s (about +24.4%).
This compares the complete optional recovery methods, including QR's existing
WLS fallback preparation, not the symmetry projection alone. Noise-one Hessian
times in the six-run campaign are 3.56326/2.11961/1.21448s at 1/2/4 ranks, versus
2.50511/1.52962/0.854762s without noise. One observation per rank under contention
does not establish general scaling. Repeated paired fields are byte-identical
within each method. All heavy jobs were sequential, nice 10, build jobs one,
maximum MPI ranks four and one library thread per rank.

Reliability limits remain: 105281 final wing fits have residual above 0.05 despite
full rank; the mean is 0.092339, maximum 0.998883 and minimum QR pivot ratio
4.15464e-7. Noise strength one filters every final wing fit. Neither result
establishes better physical Hessians or force predictions, so noise stays zero
by default. Accepted adaptation/flow validation remains open.

Receipt: `integration_evidence/qr_symmetry_v1_validation.json`. Raw fields,
commands, scripts, input/source/binary pins and paper inventory are saved under
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/qr_symmetry_v1`.
Next: benchmark Clément recovery and controlled boundary extrapolation at fixed
complexity, then assess a distance-based Hessian with the full mapping-curvature
chain rule. Profile avoidable WLS fallback preparation and validate accepted
adaptation/flow cycles before adding further recovery heuristics.

## Publication provenance and limits (checked 2026-10-08)

The prioritized PDF checklist is in [Papers/READING_LIST.md](Papers/READING_LIST.md),
with the four papers already requested, five immediate additions, and later
metric/gradation, validation and parallel adaptation references.

The implementation combines published method families, standard numerical
linear algebra and experimental engineering choices. The references checked
here explain the foundations and known failure modes; this audit does not mean
every recent change was derived from these papers before implementation.
Manufactured tests and frozen solution checks are empirical evidence, not a
published convergence proof or evidence of improved aerodynamic predictions.

| Topic | Relevant primary source | Relationship to this branch |
|---|---|---|
| Curved, highly stretched CFD meshes | Mavriplis, *Revisiting the Least-squares Procedure for Gradient Reconstruction on Unstructured Meshes*, NASA/CR-2003-212683 (2003), [public report](https://ntrs.nasa.gov/citations/20040070704) | Documents serious gradient errors from stretching combined with curvature and dependence on weighting/discretization. It motivates examining this failure mode; it does not establish our Hessian algorithm. |
| Quadratic recovery in node-centered finite volumes | Diskin and Thomas, *Effects of Mesh Irregularities on Accuracy of Finite-Volume Discretization Schemes*, AIAA 2012-0609, [public full paper](https://ntrs.nasa.gov/api/citations/20120001451/downloads/20120001451.pdf), particularly Sections II and VII | Studies quadratic least-squares fits, growing insufficient stencils to neighbors of neighbors, and curved/high-aspect meshes. These are directly relevant method choices. Their fits, flux discretizations and accuracy results differ from our weighted sensor Hessian recovery. |
| Polynomial recovery and Hessian theory | Zhang and Naga (2005), [gradient recovery](https://epubs.siam.org/doi/10.1137/S1064827503402837); Guo, Zhang and Zhao (2014 preprint), [Hessian Recovery for Finite Element Methods](https://arxiv.org/html/1406.3108v2), Sections 2.2 and 3 | Polynomial patch fitting is established. The Hessian paper applies gradient recovery twice; our QR path differentiates one scalar polynomial fit directly. Its finite-element assumptions and superconvergence results cannot be claimed for our finite-volume RANS implementation or partial cubic basis. |
| Metric intersection | Alauzet, Frey, George and Mohammadi (2007), [3D transient fixed point mesh adaptation](https://doi.org/10.1016/j.jcp.2006.08.012) | Supports intersection through simultaneous reduction of quadratic forms. The scaled A+B whitening frame in this branch is our numerically motivated algebraic evaluation of that operation; no source identified here specifically prescribes this implementation. |
| Anisotropic gradation and RANS adaptation | Alauzet, [Size gradation control of anisotropic meshes](https://doi.org/10.1016/j.finel.2009.06.028); Alauzet and Frazza (2021), [Feature-based and goal-oriented anisotropic mesh adaptation for RANS applications in aeronautics and aerospace](https://doi.org/10.1016/j.jcp.2021.110340) | Establish the wider metric/gradation and RANS adaptation setting. They do not validate this branch's exact geometric BL composition or new Hessian correction. Geometric BL/gradation implementation remains owned by the integration branch. |

Choices that must remain explicitly identified as implementation heuristics:

- Wall-only partial cubic enrichment, omission of the pure thin-direction cubic,
  the SVD separation threshold 10, condition-bound threshold 1000, and requirement
  of two residual degrees of freedom. These were selected through stencil
  support analysis and manufactured comparisons, not copied as a complete
  published algorithm. The coarse-grid regression and unsupported edges below
  remain material limits.
- Residual-based Hessian uncertainty and the exact spectral noise shrinkage
  policy. Noise remains optional and defaults to zero; preservation of physical
  shocks and wall features has not been established by the frozen checks.
- Global-ID accumulation order for adaptation WLS and the choice to reuse the
  existing compensated summation helper. These address measured floating-point
  partition dependence, not a new mathematical reconstruction method.
- QR/SVD conditioning and rank checks are standard numerical tools. The exact
  basis, scaling, fallback thresholds and MPI stencil packaging are engineering
  decisions that need their own tests.

Next literature assessment: compare direct and twice-recovered Hessians on
one-sided, curved, stretched node-centered finite-volume stencils; assess
boundary/symmetry extensions and anisotropy-uniform error bounds before choosing
further recovery changes. Public sources above are accessible. Full papers on
these specific boundary-layer recovery questions, especially any used for the
user's pyAMG workflow, would be useful additional evidence.

## Curved-wall QR recovery improvement (2026-10-08)

Commit `6312fee370` adds a supported cubic correction to the existing opt-in
quadratic sensor recovery. It applies at solid-wall points on grown stencils,
using the same complete two-ring samples and MPI exchange. Interior recovery
and the integrated geometric BL composition policy retain their existing paths.
Receipt: `integration_evidence/curved_wall_recovery_v1_validation.json`.

The smallest SVD direction must be separated from the other directions by a
factor greater than 10. The fit includes tangential and mixed cubic terms but
omits the pure thin-direction cubic term, which can be inseparable from lower
orders on three sampled layer levels. It requires two remaining residual degrees
of freedom, full rank, and a Frobenius bound on the 2-norm condition at most 1000.
Unsupported extensions use the existing quadratic fit; failed quadratic fits
still retain WLS. Noise covariance, roundoff suppression and fit diagnostics now
use the actual number of fit coefficients. The cubic-fit count uses the existing
statistics reduction, so no extra collective or stencil ring is introduced.

Tests now measure wall-interior and wall-edge transverse, tangential and mixed
derivatives separately. New physical accuracy/convergence assertions would fail
on the old measurements; finite tensors alone are insufficient. All 147 selected
serial cases passed (379112 assertions), and 31 MPI-safe cases per rank passed on
two/four ranks. The differentiated header syntax probes pass; QR is still rejected
in differentiated, symmetry and periodic configurations.

| Refined manufactured case | Before | After | Improvement |
|---|---:|---:|---:|
| Doubly curved/skewed 3D: worst wall transverse relative error | 12.8851% | 6.48616% | 49.7% lower |
| Same 3D case: wall-interior transverse relative error | 12.8851% | 5.67254% | 56.0% lower |
| Same 3D case: wall-interior tangential absolute error | 0.566651 | 0.225506 | 60.2% lower |
| Same 3D case: wall-interior mixed absolute error | 0.194001 | 0.0833072 | 57.1% lower |
| Curved 2D AR1000: wall transverse relative error | 0.34688% | 0.110208% | 68.2% lower |
| Curved 2D AR1000: wall-interior mixed absolute error | 0.0514686 | 0.00247931 | 95.2% lower |
| Curved 3D AR1000 without span curvature/skew: wall-interior transverse relative error | 0.551333% | 0.107329% | 80.5% lower |

These errors use analytic derivatives in local layer coordinates. The new method
does not improve every mesh resolution: on the coarse doubly curved/skewed 3D
case the worst transverse wall error rises from 12.1190% to 14.0426%, then falls
to 6.48616% on refinement. Unsupported edge/corner fits retain the earlier errors.
Interior errors retain the baseline values. WLS's curved/skewed accuracy weakness
is unchanged; this improvement applies when `NUM_METHOD_HESS=QUADRATIC_LEAST_SQUARES`.

Twelve final frozen RANS/SA checks passed independent unchanged-state, SPD,
size/aspect, sensor transport and MPI gates, plus reported complexity checks
(M6 complexity is also independently integrated). The RAE fixture uses
1584 corrected point/sensor fits on each partition. Maximum metric MPI differences
are 6.54e-12 (QR/noise off), 9.44e-11 (QR/noise on), and 1.09e-12 (WLS).
M6 retains its true symmetry and WLS path, with the numerical MPI gate passing.
The noise default stays zero. Geometric composed-field transport residuals remain
diagnostics owned by the BL integration work; this is not a certificate of their
gradation or of improved forces, flow convergence, wall y+ or native 3D remeshing.

Rejected candidates are preserved: stronger inverse-distance weighting improved
the difficult case but more than doubled a simpler case's wall-normal error;
unrestricted enrichment worsened 2D interior eigenvalue errors; full cubic fits
produced unreliable thin-direction/edge derivatives. Final enrichment is limited
to wall points and supported tangential/mixed terms. Independent Python screens
are exploratory; the final acceptance measurements come from the C++ solver chain.

Raw fields, tests, rejected candidates, scripts, paired performance measurements
and input/binary/source pins are saved outside `/tmp` under
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/curved_wall_recovery_v1`.
All heavy work runs sequentially with nice 10, build jobs 1, MPI ranks at most 4
and one library thread per rank. Three alternating paired serial RAE QR/noise-off
runs give median Hessian time 0.193526s before and 0.193830s after (about +0.16%).
Median metric/process times are 9.34549/10.3061s before and 9.18881/10.2028s after.
This small sample under contention establishes no measurable slowdown on this
fixture; it is not a general speedup claim. Different sensor fields also change
complexity/gradation work. Raw observations are in the validation receipt.

Next: add QR symmetry support so the real ONERA wing can exercise direct recovery;
then periodic support. Better WLS/edge reconstruction and accepted adaptation/flow
checks remain open. A complete cubic fit would require more independent normal
layer samples; assess its communication and noise cost before extending rings.

## Numerical stability implementation (2026-10-08)

Source commits `f27bae8dcf` and `3d57a82801` fix the previously failing strict
metric MPI gates without changing the geometric BL policy. Receipt:
`integration_evidence/metric_numerical_stability_v1_validation.json`.

- The shared tensor intersection now uses the scaled sum of both inputs as its
  whitening frame. It evaluates the same generalized spectral maximum while
  avoiding cancellation from whitening by a nearly rank-one rotated sensor.
  Independent 90-digit references cover 2D/3D, condition 1e14, input swapping,
  perturbations and scales 1e-150 to 1e150. Original double evaluation had relative
  errors 4.61e-4 / 8.80e-4 on the rotated cases; the balanced evaluation is near
  machine precision. This shared helper also serves BL composition and gradation;
  their mathematical policy, interpolation and geometry remain unchanged.
- Adaptation WLS gradient/Hessian accumulation visits neighbors by global ID,
  including the reused-geometry Hessian path. A temporary vector is reused per
  calling thread; no persistent weight/stencil cache is added. Ordinary CFD
  gradient calls retain their existing ordering. Reversing fixture adjacency
  preserves adaptation derivatives exactly.
- Primal sensor normalization reuses the existing compensated sum library,
  whose translation unit protects compensation from fast-math reassociation.
  Temporary storage is one passive scalar per owned point/sensor (about 1.47MiB
  on this serial M6 fixture and 0.28MiB on RAE). Differentiated builds retain
  their active MPI normalization sums; these improvements are not a full AD
  reproducibility or accuracy certificate.

All 147 selected serial cases passed (378972 assertions); 31 MPI-safe cases per
rank passed on two/four ranks. The five legacy `[Gradients]` MPI failures also
occur in the unchanged integration executable: those fixtures use null solver
communication and inspect incomplete halo data. A broader MPI in-memory mesh
reader fixture also fails in that executable. Failed logs and baseline runs are
retained; no production guard or assertion was weakened to obtain these passes.
Forward/reverse kernel/header syntax probes pass with a valid reverse tape;
full differentiated solver behavior remains unvalidated.

All twelve frozen RANS/SA runs completed, with zero flow/remeshing iterations.
The original solution values and mesh coordinates pass the independent checks:

| Fixture / recovery | Maximum metric difference, MPI 1 vs 2/4 | Complexity | Independent sensor transport |
|---|---:|---:|---|
| ONERA M6 WLS with true symmetry | 6.63e-15 (previously 7.37e-7) | 300000, independently integrated | Not enabled in this 3D case |
| RAE WLS, noise 0 | 1.09e-12 | Reported geometric composed integral 60000 | Maximum ratio 1.000000094 |
| RAE QR, noise 0 | 3.85e-12 | Reported geometric composed integral 60000 | Maximum ratio 1.000000098 |
| RAE QR, noise 1 | 1.34e-10 (intersection-only: 2.00e-4) | Reported geometric composed integral 60000 | Maximum ratio 1.000000099 |

Recovered Hessian CSV components are identical across these partitions. M6's
former worst weak eigenvalue amplified order-dependent WLS roundoff; changing
intersection alone did not fix it. On RAE with noise enabled, filtered Hessians
were already identical; compensated normalization was additionally necessary.
The strict MPI limit stays 1e-9. Noise strength remains zero by default because
numerical reproducibility does not establish physical feature preservation.

The independent RAE reference composes BL at the actual point and retains both
sensor and BL demands (minimum generalized domination about 1 minus 4.2e-12).
It still measures **composed** nodal transport maxima 6.40012 / 12.39031 / 4.63954
for WLS / QR / QR-noise. Converged sensor gradation is not a composed-field
certificate. No composed-field correction or continuous quadrature accuracy
claim is introduced here; these remain with the BL integration owner.

Performance was checked with one heavy job at a time, nice 10, build jobs 1,
MPI ranks at most 4 and library threads 1. A paired kernel benchmark measures
about 7% (2D) / 8% (3D) additional intersection cost; it still uses two eigensolves
and four matrix products. The combined WLS reuse fixture retains about a 2.37x
advantage over repeated geometry recovery. Observed serial RAE metric times are
10.91s / 9.81s / 11.69s (WLS / QR / QR-noise), M6 0.435s including 0.187s Hessian
recovery. These single observations under machine contention are not controlled
whole-solver performance comparisons. Final sensor gradation needs 32 / 29 / 25
sweeps; cumulative complexity-solve work is recorded separately in the receipts.

Durable raw fields, logs, configs, input copies, build/probe failures, executables,
benchmark and a SHA-256 manifest are saved outside `/tmp` under
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/metric_numerical_stability_v1`.
The numerical blockers below are historical. Next work is curved/skewed wall
recovery with separate wall-interior/edge accuracy gates, followed by QR symmetry
and periodic support. Full adaptation/flow validation and differentiated solver
validation remain necessary; native 3D remeshing is separate work.

## Authorized work and retained history (2026-10-07)

| ID | Work | Status |
|---|---|---|
| 1 | Constrained tensor gradation: classify blocked directions, converge useful updates, reduce repeated work | Direct updater rejected by actual-case checks. Integrated base retains sensor-only gradation and geometric query composition; full composed-field guarantees remain the BL owner's follow-up |
| 2 | Noise-aware Hessian treatment, with controls that retain resolved curvature | Implemented opt-in residual shrinkage; numerical MPI blocker fixed on 2026-10-08. Physical feature preservation remains unvalidated; retain default zero |
| 4 | More selective MPI geometry exchange and storage | Implemented conservative occupied query boxes; unit/MPI tests passed; private candidate reference transport remains separate |
| 5 | Reuse WLS geometry across sensors and derivative passes | Implemented within-call normal-matrix/weight reuse; stretched multisensor equivalence and MPI tests passed |

Performance is an acceptance condition. Check finite SPD tensors, complexity,
wall-normal spacing, MPI agreement, residual gradation and elapsed time together.
Do not obtain a speedup by silently dropping required constraints or declaring a
sweep limit to be convergence. Keep material limitations visible.

## Implemented and validated before this work

This section describes the historical pre-integration implementation. Its hard
normal BL reset was superseded by the geometric intersection contract above.

- Coordinate-equilibrated WLS solves and stencil conditioning diagnostics.
- Opt-in primal direct quadratic recovery using coordinate whitening and pivoted
  QR; up to two rings, complete donor neighborhoods, WLS fallback for failed fits.
- Residual/pivot diagnostics and stencil comparison. These are confidence
  diagnostics, not an established noise filter.
- Sensor-unit normalization, affine roundoff suppression, and protection against
  flat sensors imposing a competing metric.
- Prescribed full-band BL normal sizes and zero normal-tangent coupling, nearest
  active-wall overlap handling, and a single outer fade per trial.
- Tangential chord sizes from the immutable native reference, using the native
  deviation check and feature limits, permitting refined-wall coarsening.
- Complexity solve including BL, size/aspect bounds and tensor gradation;
  infeasible budgets are reported while retaining configured wall heights.
- On-demand MPI second-ring owner neighborhoods and padded-rank-box BL geometry
  exchange with incident-face support for vertex normals.
- Shared eigensolver primal normalization fix for subnormal Householder scales
  under fast-math. The extreme AD derivative issue below remains open.

Validation: 94 selected serial cases; 21 selected cases per rank on two/four
ranks; six frozen RAE2822 WLS/QR runs. Both methods reached complexity 60000,
retained full-band normal sizes to about 7e-12 relative error, and agreed across
MPI within 3.4e-10 relative metric difference. QR Hessians were identical across
those partitions. This establishes metric properties, not aerodynamic accuracy.

## Performance baseline and actual limits

RAE2822 fixture: 18591 points, 36263 triangles, 852 AIRFOIL edges; frozen RANS/SA
restart, zero flow iterations, one OpenMP thread per MPI rank.

| Ranks | WLS metric computation (s) | WLS entire process (s) | QR entire process (s) |
|---|---:|---:|---:|
| 1 | 97.1332 | 98.2302 | 101.9654 |
| 2 | 56.9936 | 57.9460 | 58.0307 |
| 4 | 31.9571 | 33.0874 | 34.2505 |

These are observed wall times, not timings of gradation alone. The previous
report printed 80 sweeps for the last trial; gradation is also repeated within
the complexity solve and corner-size iterations. Instrumentation measured **61 complexity trials, 4576 cumulative sweeps**,
128.644 s in gradation, 2.1212 s in halo staging/exchange and 0.019443 s in
sweep reductions on one rank (fresh instrumented run; total elapsed times vary
with system load). The cost is predominantly local tensor work, not MPI.

The case reaches the 80-sweep limit. Final transported-tensor violations are
about 1.8--1.9% in the prescribed full BL band; outer-band ratios are below
1+1e-7. This is not a proof of geometric infeasibility. Two serial reference
chord/minimum-size conflicts remain (MPI reports can repeat shared faces).

QR reported 33886 of 37182 point-sensor fits above residual 0.05, with no WLS
fallbacks. Large residuals can represent input noise, nonsmooth flow, truncation
or unresolved variation; they do not justify erasing all curvature.

All 852 wall faces were still retained by rank-box exchange on this fixture.
A separated-wall MPI fixture retained at most 9 of 18. The original native
reference remains replicated. WLS weights and geometry matrices are rebuilt
per kernel call; `Rmatrix` is workspace, not a persistent cache.

## Retained deferred work

| Work | What is still needed |
|---|---|
| Full adaptation/flow validation (previous item 3) | New accepted adaptation cycles, subsequent flow convergence, mesh quality, conservation, forces and wall-resolution checks |
| Broader recovery/physics support (previous item 6) | QR periodic/symmetry support and differentiated executables; more than two rings only if failed-fit data warrants it |
| Euler metrics | Assess shock/contact-aware sensor combinations, normalization/weights and directional protection; existing generic/custom/goal machinery is the starting point |
| RANS metrics | Assess turbulence-model sensors, near-wall sensor reliability, shock/BL interaction and geometry-aware normal/tangential treatment |
| Automatic BL sizing | Friction-velocity/y+ based first-height selection with verified physical units; current prescribed apex height does not guarantee y+, cell-centre spacing or multilayer growth |
| AD numerical limits | Full differentiated solver validation. A forward scalar eigensolver derivative at off-diagonal 1e-310 is NaN both before and after the primal fix |
| Reference geometry scaling | Distributed/indexed immutable-reference access; keep restart associations and original-geometry provenance intact |
| Remesher robustness | Broader cavity repair, coordinated topology/geometry changes, geometry-constrained relocation and rollback-safe publication |
| Remesher performance | Avoid repeated rejected proposals, update local work queues, profile cavity size/rejection causes and MPI dependency traffic |
| Mesh topology | Native capability remains 2D simplices; structured multilayer/prism/quad guarantees and 3D native adaptation are separate work |

## Prior research and transferable ideas

### AMG/pyAMG-related publications

- Loseille and colleagues, [unique cavity operator and hierarchical partitioning](https://doi.org/10.1016/j.cad.2016.09.008), 2017:
  simultaneous boundary/volume changes and automatic cavity enlargement can
  combine otherwise rejected insertion/collapse/swap/relocation sequences.
  Metric-distance insertion filtering reduces unnecessary subsequent collapses.
  Investigate bounded cavity correction with retained orientation, reference
  geometry and transactional acceptance checks; replacing isolated moves with
  a coupled local proposal is a candidate, not an implemented guarantee.
- Alauzet and Frazza, [feature-based and goal-oriented RANS adaptation](https://doi.org/10.1016/j.jcp.2021.110340), 2021:
  unstructured anisotropic simplices can represent BL physics; accurate sensor
  recovery, feature/goal error control and primal/adjoint convergence all matter.
  A visually ordered BL grid alone does not establish flow accuracy. Examine
  sensor normalization, boundary recovery and output-functional verification.
- Bellosta, Abergo and Nishikawa, [accuracy on mixed/skewed grids](https://doi.org/10.2514/6.2025-0072), 2025:
  flow-discretization accuracy must be assessed separately from mesh adaptation;
  a better geometric mesh does not itself establish the solver's convergence order.

Evidence is from the accessible publications, not a source-code audit of a
proprietary AMG/pyAMG implementation. Earlier claims about internal operations
must be read with that distinction.

### Inspected open-source alternatives to MMG and Gmsh

Local study copies were in `/tmp/remesher-operator-study-20261007`; pinned upstream
links below preserve provenance if the temporary checkouts disappear.

| Code / inspected commit | Relevant operations to assess | SU2 application |
|---|---|---|
| [NASA refine](https://github.com/nasa/refine/tree/1563f71f224d59c8712e2181369ff93440957b0f) | Metric-space gradation (`src/ref_metric.c`), reconstruction (`src/ref_recon.c`), geometry/quality-controlled local operations | Gradation transport is already independently implemented; further reconstruction and operator policies need assessment |
| [Omega_h](https://github.com/sandialabs/omega_h/tree/dc235450cf15b2b9549fabe8472269e7f8618cae) | Metric gradation and smoothing (`src/Omega_h_metric.cpp`), synchronized owner/ghost fields and adaptation scheduling | Preserve SPD tensors and distributed consistency; measure iteration/work costs; ordinary unconstrained smoothing does not enforce SU2's fixed BL normals |
| [SCOREC Omega_h](https://github.com/SCOREC/omega_h/tree/8fc0a0bb3140e601cea86ebcbd2faecd30b410bf) | Parallel mesh/metric adaptation implementation | Cross-check ownership/communication patterns and current differences from the inspected Sandia version |
| [PUMI/MA](https://github.com/SCOREC/core/tree/7166244ff601fb3b3847543d725762a2e330feef) | Layer refine/coarsen/snap (`ma/maLayer*.cc`), shape handling and balancing | Assess layer/column consistency, geometry projection and coordinated distributed operations; native 2D triangles have a different representation |
| [AVRO](https://gitlab.com/philipclaude/avro/-/tree/2e63b6ef0340dca761e0923d7a4ba21747be7580) | Cavity, swaps, collapses, smoothing and geometry checks (`src/lib/adaptation`) | Evaluate metric-space relocation and coupled repair against native acceptance contracts |
| [Tucanos](https://github.com/tucanos/tucanos/tree/62cad873bce7707a922628398f5729750a53726d) | Anisotropic metric/remeshing and topology implementation | Assess small metric/operator kernels and reproducible quality checks before attempting integration |

The useful borrowing unit is an algorithm or acceptance policy with a measured
SU2 regression, rather than adopting a complete external backend. Keep metric
quality, reference fidelity, prescribed wall resolution, conservation and MPI
publication checks when assessing any candidate.

## Performance-oriented implementation decisions

- Reuse existing math, mesh ownership and communication infrastructure.
- Share WLS geometry within a call; avoid long-lived caches until invalidation
  for motion/adaptation, periodicity and AD is explicit and measured.
- Profile total complexity trials and cumulative sweeps, not just the last trial.
- Prefer direct constrained updates and selective MPI requests over increasing
  an iteration ceiling or repeatedly exchanging the entire boundary.
- Noise controls must be explicit, tested on exact/noisy/sharp fields and timed;
  distinguish coefficient uncertainty from actual resolved flow features.

## Superseded prototype contracts (00e9097db5; see rejection below)

- Direct full-band 2D gradation uses the Schur complement in the wall frame,
  retains the normal size and zero coupling, caps tangential updates, and counts
  currently blocked transports. Counts are not a proof of infeasibility.
- The 80-sweep safety ceiling is retained pending convergence/performance data.
  Final transported-tensor ratios are audited using the actual final halo field,
  rather than the inputs to the last sweep. Convergence of updates and remaining
  constraint violations are reported separately.
- `ADAP_HESSIAN_NOISE=0` retains the previous QR output. Positive strengths
  subtract a residual-derived Frobenius uncertainty from curvature eigenvalues
  in the whitened coordinate frame, retaining gradients. This is regularization,
  not a calibrated noise probability; fit residuals also contain truncation and
  shocks. Exact, noisy and resolved-field tests are required before use.
- WLS reuse is explicit and limited to matching geometry/weights in the same
  call, with no persistent mesh cache. Periodic, symmetry and AD use the prior
  kernels. Neighbour geometry is shared across sensors without storing weights.
- Spatial boxes cover all owned points and retain conservative incident-face
  padding. Metadata is bounded per rank; the immutable original reference remains
  replicated and very large MPI process counts still need distributed indexing.

## BL policy divergence raised by the user (2026-10-07)

This branch's native BL policy is a deliberate change from the original/main
policy, not simply a numerical fix. Original/main intersects each active wall
with the sensor metric, retains the resulting normal entry (so sensors can
refine below the BL normal size), and applies a tangential size floor in a small
near-wall band. It applies BL after the sensor complexity solve; total complexity
can exceed the sensor target.

The branch instead prescribes the closest wall's normal entry throughout the full
BL band, removes coupling there, allows tangential refinement for adapted surfaces,
and includes BL/gradation in the complexity solve. Original-reference chord sizing
also changes the dependence on current wall edge lengths. These choices address
spacing drift and refinement feedback, but hard normal spacing can suppress
legitimate sensor-driven normal refinement. A normal-size upper bound is not the
same as an equality. Do not describe this as a compatibility-preserving fix or
claim that spacing/complexity checks establish superior CFD accuracy.

The 128.644 s / 4576-sweep performance baseline is the earlier implementation of
this branch's constrained policy, **not the original/main implementation**. A
faster version of that policy is not yet a timing comparison against main.

The inspected NativeIntegrated checkout is `codex/native-unsteady-2d` at
`b0cbfaf33409f8dac455d19ed85a74b9c1fa9f43`; it also adds `ApplyPoint` for
pointwise/window use. The metric branch started from `73708f6722` and predates
that API work. Text/API integration and the BL sizing policy are distinct issues.
The user's main-agent notes confirm both semantic conflicts; no main merge is authorized.

### Confirmed current-main representation contract

User supplied the main agent's reconciliation notes. Inspection confirms commit
`73b4bf9434266bbc42c62d6239dec4fe127007a0` freezes only the CFD sensor field.
`CSinglezoneDriver::MakeRemesher` constructs BL constraints from immutable original
wall components and passes a `MetricComposition` callback to `CNativeRemesher`.
Every candidate/remote-cavity query interpolates the donor sensor and invokes
`CBoundaryLayerMetric::ApplyPoint` at that actual position. Native nodal BL
composition is excluded in the solver. The callback uses intersection, retains
finer sensor demands and disables the legacy abrupt tangential floor.

Our composite nodal output must **not** feed that callback. In general
`interpolate(B(x_i,S_i)) != B(x,interpolate(S_i))`; interpolating very large normal
eigenvalues from a coarse Euler wall spreads tiny sizes into the interior.
Prescribed normals do not fix that representation error. Frozen adapted-grid
SPD/height/complexity/MPI checks also do not validate this interpolation contract.

Main's detailed preparation is physically saved at
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/METRIC_ROBUSTNESS_MERGE_PREPARATION.md`
and committed in `b0cbfaf334`. Keep it as the integration reference. Main has
validated cross-grid adaptation work newer than this branch's base; do not replace
that behavior merely to make this branch's nodal checks pass.

Reconciliation requirements retained for follow-up:
1. Freeze/interpolate sensor-only tensors; apply BL once at every actual query
   against original wall geometry, including remote/private candidate points.
2. Use one composition function for complexity estimation and remeshing, including
   normal/tangential/fade policy. Retain finer sensor demands by default; a hard
   normal prescription is a separate explicitly selected policy.
3. Integrate geometric BL density with adequate boundary-aware quadrature on
   coarse seeds. Nodal volume weights alone can misestimate thin-layer complexity.
4. Define how gradation propagates sensor and geometric constraints without
   baking the wall tensors back into the donor interpolation field.
5. Require coarse Euler-to-BL and BL-to-Euler regressions, sensor-finer-normal
   coverage and performance comparison with current main, as well as MPI checks.

Hessian recovery, conditioning, QR noise treatment and scoped MPI optimizations
can be integrated separately. The current BL/complexity/gradation experiments
remain on the research branch and are **not ready for a blind production merge**.

## Unit validation checkpoint before the actual-case rejection

The refinement after `00e9097db5` passed 97 selected serial tests (429643
assertions), and 24 selected cases per rank on MPI 2/4. This includes within-call
WLS reuse, exact/invalid/affine/conditioned/periodic/symmetry/Euler-wall Hessians,
QR noise regularization, native tensor transport, geometry transport and existing
native/reference tests. Forward/reverse AD syntax probes also instantiate the
shared inverse and legacy recovery path; these are not full AD runtime validation.

The initial no-pruning RAE experiment reached nodal complexity 60000, a final
transported ratio printed as 1, 52 last-trial sweeps, 88 complexity trials and
4569 cumulative sweeps, with 30.745 s of gradation on one rank. This is an
experimental-policy comparison with the prior branch, not current main. Exact
unchanged-stencil pruning was included at that checkpoint and subsequently removed;
final timings and independent audits are recorded below. WLS's three-sensor stretched-grid microbenchmark
measured 0.0144882 s for 20 legacy Hessian passes versus 0.00656786 s with reuse.
Noise-energy reduction on the manufactured checkerboard is about 44%; exact
quadratic and resolved steep-profile checks remain intact. No fixed fraction of
noise removal is a universal promise of a one-uncertainty threshold.

## Rejected direct Schur updater: actual-case gate

The `70bfe3144e` direct constrained updater was rejected after all nine frozen
runs. Serial/unit and manufactured MPI passes were insufficient. Unfiltered QR
reported complexity 60617.5 on MPI 1/2 and 59804.9 on MPI 4 for target 60000;
the maximum relative metric disagreement on four ranks was about 0.99975.
QR Hessians remained exactly identical across partitions. Near a zero
fixed-normal Schur gap, tangent forcing switches from a very large capped update
to a blocked-normal fallback. That creates discontinuous trial complexity and
can make different root paths select different tensors.

The updater is removed in the subsequent source. The earlier relaxation policy
is retained with independent performance optimizations while the shared geometric
composition architecture is reconciled. This rollback does not make hard-normal
nodal composition compatible with current main; that policy remains experimental.
The failed runs and their source/hash/configuration receipt are preserved in
`integration_evidence/metric_robustness_rejected_schur_v3.json` and locally under
`/tmp/su2-metric-build/rae-native-metric-v3-rejected-schur`.

Additional measured limits: bounded occupied boxes still retain all 852 wall
faces on this partitioned RAE fixture. They do not solve replicated-original
reference storage or arbitrary remote/private query coverage. Exact pruning cut
WLS owned-point visits from a possible 84.9 million to 29.9 million and preserved
the preliminary serial field bit-for-bit, but the two serial timings (30.745 vs
31.5502 s) do not establish a runtime win from pruning alone. Four-rank scaling
was limited by synchronization/load imbalance. Keep these as findings, not as
claims that every optimization improved elapsed time.

Unchanged-stencil pruning is also removed from the final refinement: the isolated
serial comparison established fewer visits and identical fields, but no runtime
benefit. Keep the receipt and source history for future workload-specific
assessment. The retained performance changes reuse buffers and geometric work,
screen satisfied 2D transports without eigen decomposition, and measure cumulative
work. Do not trade additional control/memory overhead for an unmeasured speedup.

## Final retained-source validation and unresolved limits

Source `887ca28f939302845e731051c410fbda22c2dc0a`, binary SHA-256
`f95474b5338a43ddfa1c5fb4b9d0dd0dc8b40f1307a00d4b1a097fc09e63f456`.
The complete receipt is `integration_evidence/metric_robustness_v3_validation.json`.
Selected serial regressions passed 97 cases / 429643 assertions; MPI two/four
ranks passed 24 cases per rank. Forward/reverse AD syntax probes passed.

The strict independent frozen-restart audit passed for the six default-noise
WLS/QR cases. Complexity independently integrated to 59999.9983 / 59999.9899,
with finite SPD tensors, unchanged five restart solution fields, full-band normal
size error below 6.7e-12 and coupling below 5.2e-10. Maximum relative metric
differences over MPI were below 3.4e-10; QR Hessians were identical.

| Method | Ranks | Process (s) | Gradation (s) | Hessian recovery (s) |
|---|---:|---:|---:|---:|
| WLS | 1 | 34.008 | 32.478 | 0.00620 |
| WLS | 2 | 19.093 | 18.041 | 0.00306 |
| WLS | 4 | 13.125 | 11.996 | 0.00343 |
| QR, noise disabled | 1 | 33.508 | 31.636 | 0.20290 |
| QR, noise disabled | 2 | 20.260 | 18.869 | 0.11119 |
| QR, noise disabled | 4 | 13.231 | 12.193 | 0.06274 |
| QR, noise strength 1 | 1 | 48.850 | 46.798 | 0.28388 |
| QR, noise strength 1 | 2 | 30.976 | 29.461 | 0.16208 |
| QR, noise strength 1 | 4 | 20.613 | 19.505 | 0.07893 |

Both default methods used 37 complexity trials / 2960 cumulative sweeps.
The WLS serial gradation phase fell from the instrumented earlier-branch
128.644 s to 32.478 s; fewer trials and cheaper tensor work both contributed.
This is a single-run comparison against our earlier experimental policy, **not
current main**. Mean WLS sweep time was about 11 ms, or 0.88 s per 80-sweep
trial, repeated throughout the complexity solve. MPI four-rank cumulative sweep
reductions took 4.83 s within 12.00 s of gradation, so synchronization/load balance
remains relevant after reducing local work.

The 80-sweep limit remains active. Independent directed-edge audits measured
maximum full-band ratios 1.01835 / 1.01891 and 1900 / 3236 edges above 1+1e-5;
outer ratios were below 1+1e-7. Faster relaxation is **not** a convergence fix.
Original reference chord conflicts and composite-field representation limits
remain. No adaptation cycle or aerodynamic improvement is demonstrated.

### Enabled-noise integration check failed

The nine-case strict audit fails when it reaches the enabled-noise MPI metric.
QR Hessians are identical across all three partitions, but relative final tensor
differences reach 7.44e-6 (two ranks) and 1.93e-5 (four ranks), above the required
1e-9. All three logs report complexity 60000, but the final transported-metric
ratio worsens to 2.28272, with 17 normal-limited directed edges and 80 sweeps.
The six-case default-only audit passed unchanged; no assertion was loosened.

Retain `ADAP_HESSIAN_NOISE=0` for integration. The opt-in filter's manufactured
curvature tests validate its implementation, not its physical suitability or
interaction with the experimental hard-normal relaxation. Shock/truncation
residuals can be removed as if they were noise. Reassess the filter with main's
geometric composition and converged flow before recommending a nonzero setting.

The rejected Schur updater and enabled-noise failure are separate results;
removing the former did not resolve the latter. Integration should preserve
main's sensor-only donor field, geometric BL evaluation and finer sensor demands.
Independent Hessian/WLS changes remain assessable separately; the BL, constrained
complexity and gradation architecture needs the reconciliation described above.

## Noise configuration and main-compatible gradation proposal

The noise filter is already controlled by one config option; no second boolean
is needed. `ADAP_HESSIAN_NOISE=0.0` is OFF and remains the default. A positive
value is ON and sets strength; `1.0` uses one estimated uncertainty.
Enabled filtering requires `NUM_METHOD_HESS=QUADRATIC_LEAST_SQUARES`.
The config template now gives explicit OFF/ON examples. Three existing checks
for default unfiltered noisy curvature, opt-in shrinkage and a resolved steep
profile passed again; this does not supersede the failed enabled-noise RAE gate.

The following records the earlier proposal. The integration base now implements
sensor-only gradation, geometric query composition and BL-resolving complexity
quadrature. Its combined-field audits and remaining limits are documented in
`METRIC_ROBUSTNESS_INTEGRATION.md`; full composed-field correction remains open.

1. For each complexity scale trial, bound and grade the sensor tensors using
   transported-neighbor intersection. Keep hard-normal BL projection out of
   that sweep. Require the actual directed-edge residual for acceptance;
   small changes alone do not establish that constraints are satisfied.
2. Freeze the resulting sensor field on the donor. At each actual query,
   interpolate that field and invoke main's geometric BL composition against
   the original wall. Preserve finer sensor demands and the existing fade;
   apply the BL constraint once. Do not interpolate composed nodal wall tensors.
3. Estimate complexity from that same query composition, using quadrature that
   resolves the thin geometric band on coarse cells. The fade's core parameter
   must be consistent per trial and frozen for final queries; no collectives
   belong inside candidate-point callbacks.
4. Audit the composed field separately. Sensor-field gradation does **not**
   automatically establish gradation after geometric BL intersection, including
   between sampled donor nodes and new candidate points. Main's lower-bound
   intersection removes the hard-normal reset conflict, but size/aspect limits,
   geometry variation and finite propagation still require checks.

If full composed-field gradation is required, a separate geometric-aware
correction representation is needed. Evaluate BL at the transport/query
locations and retain only the additional gradation demand, with support adequate
to resolve its spatial variation. Merely interpolating a nodal correction that
contains the fine wall tensor can reproduce the coarse-seed spreading defect.
This needs design and validation before code integration, especially for private
and remote queries. Main's original-geometry callback remains authoritative.


## 3D stress checks and noise isolation (2026-10-07)

Source `3b320bfd6b`; the complete receipt, including failures, is
`integration_evidence/metric_robustness_3d_noise_v4.json`. The production binary
contains the restart-reader fix and no metric-probe instrumentation. Ten selected
Hessian/restart cases passed on one, two and four MPI ranks (91482 serial
assertions). These checks establish the stated regression gates; **WLS accuracy
on the severe curved case is not a passing gate**.

### Severe 3D recovery limitations

The existing manufactured pressure helper now also exercises rotated, skewed
and doubly curved tetrahedra: flat aspect 10000/skew 4, curved aspect 1000/skew 2,
with refinements 8 to 16. Errors are measured in layer coordinates, preventing
large physical transverse curvature from hiding tangential error.

| Case/method | Coarse interior Hessian max error | Fine interior Hessian max error | Fine wall transverse relative error | Fine dominant-direction sine |
|---|---:|---:|---:|---:|
| Flat WLS | 8.91e-8 | 5.88e-7 | 0.5 | 6.14e-8 |
| Curved WLS | 10.45 | 319.81 | 0.384 | 0.963 |
| Flat QR | 5.51e-9 | 2.89e-8 | 4.22e-10 | 2.98e-8 |
| Curved QR | 0.741 | 0.210 | 0.129 | 0.0108 |

Wall errors are maxima over the marked wall, including its edges/corners; the
transverse direction is the layer-coordinate direction. This does not isolate
an interior wall patch or prove that the entire wall has those errors. The
initial accuracy-gated WLS run failed, and its log is retained. Comparing reused
WLS geometry with the repeated legacy solve gives differences below 1.2e-13
of the largest Hessian entry: the accuracy weakness predates the reuse change.
QR improves in the curved interior, but the worst wall error does not decrease
on these refinements. One-sided curved-wall reconstruction needs further work;
finite tensors and a good geometry condition diagnostic alone are insufficient.

### A matching ONERA M6 RANS fixture exists

`/media/rausa/4TB/SU2_Versions/TestCasesForSSTIDDES/optimization_rans/steady_oneram6`
contains a RANS/SA mesh and frozen restart: 96252 points, 545438 tetrahedra,
exactly matching coordinates and unchanged six solution fields. The snapshot in
`cont_adj_rans/oneram6` differs from its mesh by up to 0.0284 in coordinates and
was rejected as a matching fixture. No adapted RANS version was established.

Keep the true symmetry boundary. QR currently rejects it, so these runs exercise
WLS's existing symmetry path, with within-call geometry reuse disabled. No flow
or remesher iteration is performed; selecting MMG allows 3D metric construction
but does not invoke MMG. There is no prescribed BL metric in this check and no
exact Hessian or aerodynamic accuracy reference.

| Ranks | Process (s) | Metric computation (s) | Hessian recovery (s) | Relative metric MPI difference |
|---|---:|---:|---:|---:|
| 1 | 9.876 | 0.373 | 0.135 | 0 |
| 2 | 6.129 | 0.203 | 0.0744 | 2.46e-9 |
| 4 | 4.220 | 0.108 | 0.0419 | 7.37e-7 |

The precise metric timings in the JSON receipt take precedence over rounded
values here. Metrics are finite SPD and independently integrate to complexity
300000 within 4.2e-12 relative error. Hessian MPI differences are below 2.71e-11
with the documented global floor. **The strict 1e-9 metric MPI gate fails** on
two/four ranks; do not relax it to claim readiness. Independent eigenvalue
analysis sees aspect 10000.00023 for configured 10000, reflecting visible
roundoff in these condition-number 1e8 tensors. The 3D discrepancy needs separate
isolation; it is not proof of the same cause as the 2D noise failure.

The real run exposed a filename buffer overflow in the shared ASCII restart
reader. Both shared ASCII/binary readers and both output-header readers now use
the existing string directly instead of copying it into `char fname[100]`.
A >100-character filename regression covers all four paths on 1/2/4 ranks.

### Noise failure occurs before gradation

The investigation-only `integration_evidence/frozen_metric_probe.patch` captures
pre-BL tensors/scale and permits fixed-input replay. It is **historical instrumentation, not production
configuration or a merge candidate**. It targets pre-rebase source `3b320bfd6b`,
not the current integrated BL implementation. Check out that historical source
in a disposable worktree/build before applying it;
run `replay_frozen_metric.py`, restore the source and rebuild. The final patch
also captures the initial sensor intersection before corner/BL/gradation work;
use `--capture-all` for that campaign. The replay still performs preceding root
trials, so its elapsed time is not a single-pass gradation benchmark.

With identical serial tensors and final scale, MPI differences stay below
3.4e-14 at 0/1/10/80 sweep caps. Removing experimental hard-normal BL projection
from the same frozen input converges in 32 sweeps to ratio 1.00000009965; MPI
agreement remains below 6e-13. Keeping that projection stalls at ratio 2.28272,
17 normal-limited edges and 80 sweeps. Thus the constraint-policy failure and
the original enabled-noise MPI discrepancy are separate issues.

The initial sensor tensor already differs by 5.47e-4 / 8.36e-4 on two/four
ranks, before any corner adjustment, BL composition or gradation. Filtered QR
Hessians are identical, and sensor normalization integrals differ by less than
8.1e-15 relative. At a worst point the filtered sensors are nearly rank one.
**Inference:** the highly conditioned multi-sensor intersection amplifies the
normalization roundoff (its intermediate eigenvalue ratio cap is 1e14). This
narrows the fault to upstream construction; isolating and stabilizing the
individual operators remains necessary. No numerical intersection/reduction fix
was added, and default noise strength remains zero.

### Next improvements and durable evidence

1. Stabilize multi-sensor combination for nearly rank-one tensors in 2D/3D;
   include tiny normalization perturbations and MPI agreement without discarding
   finer demands. Accurate sums can reduce input error but cannot by themselves
   certify a stable intersection.
2. Improve one-sided curved-wall and skew-sensitive recovery, with separate
   tangential/transverse and wall-interior/edge accuracy checks.
3. Support symmetry/periodicity in QR before using this real wing for QR/noise.
4. Validate against the integrated geometric BL policy and coordinate any failures
   with its owning agent. Rebase onto the published integration is complete;
   superseded nodal hard-normal experiments have not been replayed.
5. Validate actual adaptation/flow cycles, forces, conservation and wall
   resolution. Native 3D remeshing, AD and structured BL topology remain open.

The report, scripts, probe patch, logs, configs, frozen probe tensors and output
fields are also preserved outside `/tmp` in
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/metric_robustness_3d_noise_v4`.
Its manifest records file sizes and SHA-256 hashes. Input RAE files are copied
there; original ONERA inputs remain in their existing test directory. Previous
reports and the full deferred roadmap above remain retained.


## Rebase onto the geometric BL integration (2026-10-07)

The branch now descends from published integration `2abbd11769`. Only the two
post-integration commits were replayed (`3b320bfd6b` -> `1eddab22a6`,
`ce28cd2f99` -> `0164ee16dc`). The duplicate shared restart-reader fix was
resolved in favor of the integration implementation. Relative production changes
are limited to the remaining `CBaselineSolver` filename fixes; the integrated
BL, complexity, solver metric and query code are unchanged.

Fresh build and selected tests passed 26 cases on each of 1/2/4 MPI ranks,
including Hessian, long restart filename, geometric query/quadrature and sensor
gradation checks. Fresh frozen ONERA outputs are byte-for-byte identical to the
pre-rebase runs. Its strict MPI metric gate still fails at the same values; no
accuracy limitation was fixed by rebasing. Historical probe instrumentation
requires its historical source and must not be applied to the integrated policy.

Receipt: `integration_evidence/metric_rebase_v1_validation.json`. The old branch
tip is retained as `codex/metric-robustness-pre-integration-rebase-20261007` and in
a verified complete-history bundle under the durable evidence directory. New
receipts/logs are saved there in `metric_rebase_v1`; unchanged ONERA fields remain
in the earlier v4 archive, with hashes and paths recorded by the new receipt.

Next implementation work belongs to Hessian/sensor metrics: stable multi-sensor
combination, skewed/curved one-sided recovery and QR boundary support. The other
agent continues to own BL composition, complexity and gradation; our branch
validates against that shared contract.


## 2026-10-11 — actual native3D bridge preparation and independent auditors

Native runtime3D remains guarded. Pending CNativeImport3D/test-only bridge code now
imports actual CFD-owner sensors/halos/tetrahedra/marker incidence and prepares
M<=N worker migration, distributed P1 reader return, real geometry/solver rebuild,
barycentric/conservative current/BDF2 transfer and actual SU2/CGNS round trips.
Standalone bridge v1..v5 stopped during compilation/link setup; no actual test or
adaptation run has passed yet. V5 demonstrates stale pre-communicator ABI in the
existing full SU2 archives, requiring a real Common/strict/CFD rebuild rather than
compatibility aliases or relaxed linking. geometry_build_v1 records that one-job
nice19/j1/thread1 build and all source/command/host evidence.

Independent audit tool and geometry_audit_v3 self-check PASS cover original-P1
transported sensors, current/two histories, exact SU2 ASCII/HDF5 CGNS coordinates/
connectivity/physical sections/BC ranges/closed links, with five corruptions vetoed.
Serialized checker fixtures are not actual native-output or CFD proof. V1 synthetic
label-type failure and V2 receipt-variable failure retained. Production driver
wiring, actual bridge matrix, shared 2D regression, full phases/CPU/wait/RSS and
scaling/affordability remain open; full Goal1 stays active.


## 2026-10-11 — bridge library rebuilt; invalid test configuration diagnosed

geometry_build_v1 is PASS (268 actual incremental Common/strict/CFD-core steps,
exit0, no source drift). No compatibility alias or numerical relaxation.
geometry_followup_v1/v6 failed at test compilation because its serializer helper
encoded a vector rather than its records; corrected. V7 built/linked and aborted
before adaptation. One-rank debugger evidence geometry_bridge_abort_v1 identifies
CConfig's existing "COMPUTE_METRIC = YES requires ADAP_SENSOR" check. Test config
now includes PRESSURE. V8 is running sequential MPI1/2/4 with source/library/member
pins; no actual bridge mesh/state/output validation is claimed before its terminal
receipt and independent audits. Production native3D guard remains unchanged.


## 2026-10-11 — first actual bridge results and timestep fixture correction

V8 reached mesh adaptation and reader reconstruction before transfer rejected
incompatible end-step data: SU2 requires current U^n == time_n after its dual-time
update. Diagnostic abort_v2 proves this guard; test input/checker corrected, no
production transfer changes. New auditor self-check v4 adds a time_n corruption
and passes six adversaries. Historical v3's synthetic time_n scaling does not
represent an admissible actual end-step transfer input.

V9 MPI1/2 tests plus independent mesh/metric/current/BDF2/raw SU2/HDF5 CGNS audits
PASS; MPI4 finishes all 3D M1..4/both-method cycles but fails the extra six-point
2D geometry fixture with an empty sparse graph on two ranks. Its whole receipt
remains FAIL. This identifies an existing underfilled-CFD-partition limitation,
not a native3D metric failure or proof that arbitrary empty CFD ranks are supported.
The shared 2D regression now uses the existing n4->n6 transfer fixture sizes.
Four-rank 3D outputs are being independently audited separately. Native runtime3D
still guarded, and full Goal1 correctness/performance/lifecycle scope remains open.


## 2026-10-11 — actual native3D bridge and production-archive validation PASS

geometry_bridge_v10 and geometry_meson_v1 PASS on MPI1/2/4/M1..N, both transfer
methods, three changed-target events. Each independent audit checks84 mesh frames,
42 current/BDF2 state pairs and42 real SU2/CGNS output/readback roundtrips; volume1,
minq.367927/minJ.146382/maxL1.793742,original-P1 transported residual0 and maximum
state error8.471971e-13. Original donor sensors/current/history remain unchanged;
atomic reader budget vetoes and actual cached/remote thin composition hooks pass.
Shared 2D transfer history regression passes after using established n4/n6 sizes.
The actual Meson test objects link complete current SU2 archives without standalone
native/house overrides; source/library/thin-member pins unchanged. V9 supplemental
3D-only audit passes while its original whole receipt remains FAIL.

GEOMETRY_BRIDGE_REPORT.md lists inspectable actual grids/state files, all retained
failure causes, input authority/dependency/donor/storage limits and pending work.
Production3D driver guard stays. Full lifecycle accounting, runtime/ref restart,
bounded allocator/RSS, instrumentation overhead and repeat scaling remain open.
Single contended matrix durations include test/output and distinct M trajectories;
no speedup/affordability claim. Full Goal1 stays active.


## 2026-10-11 — Actual native tetrahedral production runtime checkpoint

Native factory/ReplaceMesh now supports the opt-in simple faceted 3D path; retained
original geometry is published only after accepted replacement and persisted in
bounded, validated `.native_ref` sidecars. Reference marker ordinals follow config
order, not alphabetical order. A common repartition decision prevents empty-owner
ranks skipping donor migration. Original sensor-only P1 interpolation remains at
private/remote/returned queries; no BL tensor spreading or sensor coarsening added.

Runtime strict evidence: MPI1/2/4,all M1..N,84 accepted events,42 exact detail OFF/ON
pairs,42 actual SU2/CGNS output/readback checks,current/BDF2 error<=1.119683e-12 and
transported sensor error0. Default CFD ownership without native repartition and
rejection after private accepted edits also pass with independent output audits.
Actual native2D driver and MPI-aware 2D/3D MMG regressions PASS at MPI1/2/4 using
the same production archives. The original combined runtime_v3 receipt remains
FAIL because a serial-only MMG loader test was incorrectly selected on MPI2;
follow-up regression proves the correct existing MPI path, not a fix to MMG.
See `integration_evidence/native_3d_core_v1/RUNTIME_REPORT.md` and the composite
`RUNTIME_CHECKPOINT_VALIDATION.json`; preserve all failed receipts.

Full Goal1 remains active. Remesher phase rows close per rank, but full metric/
rebuild/transfer/output accounting, aggregate initial-import/caller-copy memory
admission, RSS, restart, broader actual anisotropy/size tests and repeated scaling
are pending. Original authority256-triangle limit; no 3D BL/curved/full-unsteady/
affordability claim. Current different N/M produce different valid workloads;
fixed-work OFF/ON controls retain identical meshes and transported states.


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
