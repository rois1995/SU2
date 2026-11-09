# Boundary query performance — next connected cycle

User steering2026-11-09; performance analysis only, no production performance edits.
Full293 now completed TWO accepted/transferred/persisted meshes with finite continued
CFD; actual isolated first-adapted disk restart passed MPI6. All11fullworkflow and
10restart actualHOST handles terminal; no ownedheavyjob active. Both independent
matched checkpoints are retained in staged57/m6_inputs/euler_adapted293_event{1,2}_v1.
Frozen driver had already entered event2 when checkpoint1 was verified; its outcome
was preserved before any source edits. Euler milestone is now verified; full goal
still includes this requested performance cycle, dedicated BL/MPI and RANS500k.

After that boundary, prioritize ONE connected performance cycle using build291 and
subsequent first-event evidence. Reuse SAME original saved CFD seed, Mach-onlyL1,
50k target, constraints,5400native/14400outer budgets and current validity-first
construction. Compare complete first-adaptation replay; no simultaneous sensor,
norm, target or construction-policy experiment. Then return to the preserved
adapted CFD checkpoint for adaptation2 if still required. Separate actual isolated
adapted disk restart remains mandatory. Never overwrite the preserved checkpoint.

## Confirmed static observations, not demonstrated speedups

- MeshOptimizer::Commit clears all rejected_smoothing after every commit.
  RememberRejectedSmooth currently stores vertex IDs, so simply retaining it would
  be unsafe. Prove complete relevant cavity dependencies and invalidate on changes
  to neighbors, coordinates, bindings, support, metric state and phase/options.
- SmoothBoundary already builds its patch, exports old parameters, caches old merit,
  prepares a projected context/atlas, screens shape/edges and lazily measures old
  geometry. Retain these facilities and whole-patch merit. Inspect remaining repeated
  work across MoveVertex/PrivateParameters/Finish/GeometryGuidanceExcess; do not
  assume a second projection reproduces the first coordinate or association.
- MoveVertex currently reconstructs PrivateParameters(old), Incidence, SkinMap,
  CheckSurface and Nodes on each trial. Reusable old-side context must preserve
  complete interfaces, exact positivity/topology, associations, features, bodies,
  requested heights, storage/work/time admission and current transport authority.
  Fresh trial positions and their metrics/geometry still require evaluation.
- GeometryGuidanceExcess creates a private parameter context and visits complete
  patch.surface. Existing exact-coordinate/support certificate facilities are the
  starting point. Reuse unchanged contributions only with valid coordinates,
  association/support/tolerance/authority identity. Accepted association is not a
  distance proof. Uncertain conservative bounds fall back to full certification.
- CNativeAffineReference scope misses copy selected prepared triangles, then build
  a TriangleUnion. Existing prepared leaf forest is already available. Investigate
  direct reusable leaf/subtree query execution while preserving exact witness
  support identity, conservative finite-domain bounds, active-reference accounting,
  transient admission and ordering/fallback correctness. Separate identical-query
  hits from nearby positions changing selected-piece keys; no benefit assumed.

Existing counters decide the connected scope: geometry traversal/preparation,
trial/move yield, interior versus boundary work, q/J retirement and introduction,
net whole-field removal/s, geometry-bad counts and total accepted first-event time.
Rank CPU and busy/join counters account for contention; join-call time is not total
MPI wait. Overlapping smooth/guidance/certificate clocks must not be added.
Changed-cell measurement/private-to-public reuse and clearance-index rebuilds are
secondary unless their counters demonstrate the active bottleneck. Unrelated
hypotheses belong in ADAPTATION_BACKLOG.md.

## Qualification and return

State the demonstrated limitation and exact dependency design before implementation.
Use existing ownership/reservation/publication, metric/certificate/cache machinery;
no new scheduler/configuration/CAD requirement. Focused checks plus ONE appropriate
MPI milestone: seams, genuine features, separate bodies, requested heights, stale
neighbors/dependencies, cached/recomputed equivalence, empty ranks and resource
exhaustion. Freeze source/build/runtime/helpers/inputs during each campaign.
Then immediately replay complete first adaptation under unchanged gates/budgets.
Compare work/progress at matching stages and explain trajectory changes. Commit
counts/debt alone are insufficient (291 window:8246 q-bad retired,6859 introduced).
Return to preserved adapted-CFD state after qualification; keep logical event
identity and matching restart provenance explicit across isolated directories.

## Actual completed first-event293 counters

FIRST293 is accepted/transferred/persisted with finite adaptedCFD. Its independently
verified matching mesh/reference/restart copy is staged57/m6_inputs/
euler_adapted293_event1_v1. Actual isolated disk restart now PASSED; event2 also
completed and is independently preserved before any production performance edits.

first_event_performance_counters.json in293attempt seals exact first-event prefixes
and comparisons to291. Native3640.719s versus2913535.671s; all164matching counts/flags/
comparedwork exact. Boundary292056starsearches,8reusehits,395093candidates,17177moves;
2174.545aggregate rank-seconds versus interior65.269. Scope364250misses/builds,
1080.444aggregate rank-seconds; traversal4896.194rank-seconds. These overlapping
clocks are NOTadditive and are not critical-path wall shares. They establish real
boundary/scope work worth targeting, not a proven optimization or safe new cache.

Further source review: RememberRejectedSmooth already limits retention to finite
pure-shape rejection with no validity/edge/geometry failures and no exhausted
atlas work/time or geometry_failure. Preserve that exclusion. Dependency-specific
invalidation may reuse this existing set without a new scheduler, but complete
old/new cavity dependencies, projected-association semantics, metric state and
transient limits must be demonstrated first. No production change yet.

Existing-facility review: PlanarReference::HasAcceptedFaceCertificate verifies
retained face identity, BoundaryPoints, association and face/trace coverage. It
is used to avoid repeating support/coverage validation; it contains no numeric
distance bound and must not substitute for SurfaceDeviation.
GeometryGuidanceExcess still builds PrivateParameters and obtains SurfaceDeviation
for complete patch.surface. Reuse must retain an actually evaluated numeric bound
under exact coordinate/association/coverage/support/tolerance/authority validity,
preserve complete-patch accumulation order and fall back on uncertainty. Existing
point-keyed frozen tensor reuse in boundary linearizations must remain intact.
No new certificate or production reuse mechanism has been implemented.

The eight boundary reuse hits do not count eligible pure-shape insertions or
distinguish commit invalidations from capacity resets. Do not attribute low reuse
solely to Commit or assume dependency-specific retention has a material gain.
Test unchanged versus changed complete dependencies and assess actual replay yield.

Completed driver summary293: FIRST Adapt3819.85s, initial Solve40.6008s and
following adapted Solve152.95s; second Adapt2440.84s/final Solve146.688s.
workflow293_driver_timing_summary.json seals actual rounded rows and full log hash.
In this steady path ComputeMetric is in RunTimeLoop postprocessing (Solve timer);
Adapt covers subsequent remesh/replacement/transfer/mesh+reference output. Use
3819.85s as total accepted first-adaptation comparison alongside native3640.719s,
matching-stage work/progress and CPU/busy/join. Native2913535.671s is a native-only
comparison;291 failed IO and has no completed whole-adaptation timing baseline.

## Active implementation start — UNBUILT / UNVALIDATED

First connected component edits MeshOptimizer::Commit: invalidate remembered pure-
shape rejections for every vertex in BOTH retired and introduced cells, rather than
clearing unrelated entries. The complete affected star changes under any shared
coordinate/connectivity/physical-binding edit; target/options stay frozen during
the private optimizer. Removed/moved identities occur in the retired/introduced
sets. Existing pure-shape-only/resource exclusions and capacity bound stay intact;
bulk optimizer replacement still clears all history. This is NOTqualified yet.
Required regressions: unrelated commit retains eligible rejection; shared neighbor/
association/topology changes reopen it; cached/cold optimizer results and heights
agree under complete-search budgets; resource exhaustion is never remembered.

Old-side movement topology and numeric guidance reuse are still under design; no
prepared-context API or new distance certificate implemented yet. Existing
SmoothBoundary already prepares projection screening and passes the RAW proposal
again into MoveVertex, correctly avoiding an idempotent-projection assumption.
Prepare expensive old context lazily after existing cheap screening; eager setup
on every searched star risks regression. Fresh position metrics and trial parameter
views remain mandatory. Finish the connected implementation before broad regression
or another M6 replay; no component-only capability claim.

Read-only prepared-leaf review: target prepare calls Upper BEFORE Distance, and
Distance runs only when upper>tolerance. Existing prepared leaves have footprints=false:
do NOTcall TriangleUnion::Upper on them; its non-passing fallback indexes plane data
that those leaves do not own. A separate convex-only witness can use exact selected
pieces, with original joined-union fallback when no witness passes. Complete-support
lower distance must still visit all pinned support leaves. Hit/miss ordering and
uncached oracle must preserve numeric equivalence, not merely gate classification.
Deferred construction must preserve caller-byte restoration, admitted overlap and
shared-cache lifetime; no half-built target publication. Benefit remains unproven.

Existing ignored corpus is available: build235/geometry_trial_queries_v1/
current_geometry_queries.bin,5744B,SHA
c176553d16ea9a1846807d5377070618c917b74f831d88266d307abd1e6f30e0.
Version18,tol1e-4,12actual trial queries, immutable input/preparation receipts.
Latest preserved replay is build237,cases/geometry_current_queries_20261107T054026_2837742:
559126coldwork,1094scopebuildwork,540961traversalwork,163assertions. HISTORICAL,
NOTcurrent293timing. Reuse tag NativeM6GeometryQueries3D with candidate build and
SU2_NATIVE_TRANSACTION_REFERENCE/HAUSD/GEOMETRY_QUERY_INPUT in a NEWcase directory.
No current293 corpus replay exists; no new old-baseline campaign was started.

C++worktree now differs from qualified293; existing293binaries remain baseline only.
Do not launch old build293 wrappers against modified source pins. No build/test/job
is active. Next: complete/review old-star/guidance reuse and focused cache invalidation
checks, then ONE connected build/serial/MPI milestone and complete first-adaptation
replay at unchanged5400budget using original saved CFDseed.

## Connected candidate294 — source frozen, not qualified

PreparedVertexMove owns a copied bounded old star, private old parameters and
validated skin/physical/node maps. SmoothBoundary prepares it lazily after the
existing projected-shape screen. Every surviving raw proposal still receives a
fresh private overlay, projection, moved-cell positivity, full output topology,
association/feature/geometry and height/metric checks. The prepared context cannot
accept a different old patch, and stale accepted epochs fail through the existing
private-parameter revision check. Optional storage refusal falls back to ordinary
MoveVertex. Factory scratch and retained context are charged to the existing budget.

Scoped numeric entity-face guidance records actually evaluated bounds; reuse needs
exact epoch+revision+definition, tolerance, ordered face IDs/facet, coordinates and
all binding fields. Fresh trial views still validate associations. No publication
certificate is inferred from association. Typical boundary stars have only faces
incident on the moved vertex: memo preparation is skipped there, since all faces
change. Complete-patch summation/early-rejection order is preserved. Counters expose
prepared contexts/trials and exact reused-face counts; benefit is not established.

Rejected-star invalidation remains the connected earlier WIP component. Added cold
optimizer comparison (absolute/validity-only, disjoint and shared commits), numeric
memo equivalence/invalidation/admission cases, prepared-versus-fresh raw move trials
and a mapped boundary caller/accounting check. Tests are not run yet. Read-only
second opinion found no concrete wrong-result defect; it does not qualify the code.
Prepared context lifetime must remain inside its AtlasBudget reservation scope.

Build294 uses pinned eight changed C++ files, nice19/j3/libs1/4TB scratch. Tool20693,
wrapper3125260/start27276529, child3125341. Source/helpers MUST stay unchanged until
terminal identities are verified. Next focused serial, connected serial and ONE
MPI6 milestone, then immediate accepted FIRST-adaptation replay with original seed,
Mach-only L1/50k and unchanged native5400/outer14400 constraints/budgets.

run_m6.py now permits the existing test-event count to be1 or2, preserves native5400
and outer14400 for the requested first-event replay, and applies transfer/output/
continued-CFD acceptance receipts to both. Its config/acceptance self-check passed;
this helper was not a pinned build input. No new production configuration option.
The original293 matching checkpoints and actual two-event/disk-restart evidence
remain preserved. BL/MPI/RANS qualification still outstanding.

### Build294 and corrected test295

294 passed369.256s, allpins unchanged; all11actualHOST handles terminal. Production
CFD SHA b316e4f9fd3c76b43c10c8c4ffb85f7040da0ab93cb2850182bec1c31dbab256.
Focused294 current_production_20261109T125153_3140872 passed27cases/5804assertions
in59.196s with unchanged pins, all5actualHOST handles absent. INSufficient evidence:
an early return in the new Catch section prevented discovery of later memo/caller
sections. Preserve it; do not use it to claim those checks ran.295 removes early
returns and encloses memo checks in their own section; mapped caller fixture has an
offset interior apex to give the boundary point useful movement demand. Test-only
build295 tool58528/wrapper3145883/start27327767/child3145946 must preserve CFD294 bytes.
Source/helper freeze active. No serial/MPI milestone or M6 replay yet.

### Corrected focused296 passed; connected milestone active

295 test-only build passed with unchanged294CFD; focused295 correctly failed two
caller-counter assertions because the original tensor left node8's star without
repair demand. No production change. Exact failed295 source is reconstructed and
SHA-verified in source295_CNativeAffineReference_tests.cpp.snapshot; its build,raw
failure and handles remain preserved. One success-output diagnostic initially
failed MPI_Win_create because osc=pt2pt was missing; corrected established MPI
startup passed and confirmed accepted prepared/cold raw moves. Neither is a perf
campaign or qualification substitute.

296 changes only the fixture: removes early returns, checks at least one admitted
raw move, explicitly uses production validity-only shape admission and an SPD
anisotropy with measured initial q failures and all old edges below1.8. Production
CFD remains BIT-IDENTICAL294. Build296 passed49.091s; focused296 passed27cases/
5877assertions in61.077s, with all three new sections observed, unchangedpins and
allactualHOST handles terminal. Connected serial now tool86407/current_production_
20261109T130501_3172831; sources/helpers frozen. ONE MPI6 follows if successful.
No performance or whole-M6 improvement established yet.

Planned first296 config has EXACTLY one difference from actual293: ADAP_SUBITER2→1.
Same Mach-only L1/50k, original saved seed, bounds, gradation, geometry/heights,
validity-first construction and native5400/outer14400. run_m6 retains transfer,
output and following CFD receipts for the one-event replay and samples per-process
CPU/start ticks alongside existing2s RSS observations; no extra perf/GDB campaign.
Existing first-event stage helper is reused by analyze_m6296_stages.py against the
sealed293 first-event prefixes; quality comparisons must stop at first shape exit
before geometry/size reconstruction. Net whole-field q/J counts and geometry,
CPU/busy/join plus work and total accepted first Adapt3819.85s remain criteria.

### Qualified296; complete first-adaptation replay LIVE

boundary296_qualification.json seals focused27/5877assertions, serial99/68536 and
ONE MPI6 milestone132cases EVERYrank (71963/72862/72138/71755/71935/72016assertions
as printed, rank order not implied). Allpins unchanged/allstagehandles terminal.
MPI223.239s,serial204.053s. No performance benefit claimed.

A296=m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k, tool57374,
wrapper3191866/start27444699, runner3192139, process3192385; sole heavyjob. All11
actualHOST identities and six mapped runtimes sealed in owned_startup.json.
Same selected-failure test_driver wrapper as293 executes actual production driver;
executing SHA6f8184dc7359a62ce6d9d82a2f96055ac6caebf2b5e7354c5f9800b4cedd403c,
84ELF files/rank match pins. Initial61330coordinates/all22CFD+metric fields exactly
match qualified254/256; sensorMACH only. StandaloneCDF294 SHA remains b316e4f9...
Actualcfg differs293 ONLY SUBITER2→1, proven atlaunch; allbudgets/constraints identical.
Current replay is frozen; outcome and work/net progress decide benefit. Inputs and
checkpoints293 remain preserved. Starting ownership check initially expected a
standaloneCFD inode and correctly foundnone; corrected verifier uses the actual
pinned command's test_driver inode, not a fabricated process claim.

### First complete quality296 window — partial, no established gain

Construction through all83complete groups/quality entry matches293 counts,flags
and comparedwork exactly. Entry round23306/native572.310757s:690804tets,27052q-bad,
27499J-bad,1204geometry-bad,0size. Firstwindow23360/native731.824856s:22725q/22530J/
1183geometry-bad, matching293 wholefield counts. Net q removal27.126/s versus
26.906/s (~0.8% timing difference), NOTan established speedup. Current quality159.514s
versus293160.822s; boundary aggregate seconds144.291versus144.426 overlapgeometry.

New296 counters:1295prepared old-star contexts/3007prepared trials/104exactreused
faces. Boundarystarsearches16738/rejected-historyhits0 match293. upper_calls65fewer,
but upper_work349842858,traversal_work327436073,scope_build_work7809526 andscope_misses
52092 are EXACT293 atthis stage. Reuse has not reduced counted expensive querywork
inthis window. Read-only review of existing scope distance/upper preparation is
still within this activecycle; no source change or newcampaign while frozen.
Let complete firstreplay outcome decide benefit and subsequent work. Current host
identity checks all11same; last MemAvailable about39GiB, no demonstrated pressure.

Three matching complete qualitywindows now show no material timing gain: latest
23488 q17888/J16900/G1144, netq17.689/s versus29317.639/s (~0.3%); traversalwork1.0.
first_boundary_reuse_observation.json seals these PARTIAL counters and prefixes.
Frozen full first296 continues, latestownership_committed23488/native1100.527s.
Read-only index audit: scope TriangleUnion index also selects exact Upper seed and
candidate order, not merely lower-distance queries. Joint finite-plane footprints
are absent from prepared per-facet leaves. Omitting index/copy can change numeric
bounds and early-return order; no safe smaller removal identified. No code/change/
newcampaign is authorized by this hypothesis while the run remains frozen.

## Terminal296 outcome and explicit switch to BL

Frozen296 is TERMINAL0/3902.168s; all1520pins unchanged/all11actualHOST identities
ABSENT. Native3518.586s accepted149051points/736323tets, allknown0q/J/G/size.
Independent saved-data validator passed; trueCVresiduals unchanged293(max7.722e-14),
0inadmissible projections, actual continuedCL.269781/CD.014756. Matching threefiles
(mesh/reference/restart) are BYTE-IDENTICAL293event1, independently copied to
m6_inputs/euler_adapted296_event1_v1. Coordinates exact/all22fields finite/positive
rho+internalenergy. No new standalone296diskrestart claimed; completed293 two-event
and actual isolated restart remain primary regression baseline.

accepted_first_performance_comparison293.json preserves actual driverAdapt3689.76
versus3819.85s (-3.4%), native3518.586versus3640.719, perrankCPU and first-quality
busy/join/churn. All164matching counts/flags identical; expensive upper/traversal/
scopebuilding work and scope misses EXACT. Outputs identical. Small wall differences
with unchanged expensive work are not a proven material kernel gain. User explicitly
directs connected BL next; no further Euler cycle for timings/reuse/speculation.

Before BL edits, archive_euler296_pre_bl.py hash-verified1303sources/BOTH qualified
binaries in baselines/qualified_euler296_pre_bl_v1 (localpayloads, metadata retained).
Current CPP unchanged; actual source freeze safely released only after terminal
proof and independent validation. See ADAPTATION_CURVED_BL_INTEGRATION_20261109.md
for the next connected capability, suitability census and strict acceptance.
