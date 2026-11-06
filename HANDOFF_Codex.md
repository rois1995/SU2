# Native adaptation — current handoff

Updated2026-11-06(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current — full M6 build210 frozen and live

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
LIVE=`R/m6_runs/euler_build210_real_cfd50k_bound_refinement`.
Full two-event M6 exact tool session54752 CONFIRMED LIVE; helper195103/start479820,
time195409/timeout195410/launcher195411allstart480761; ranks195414–18and195420
start480765. LIVE/owned_startup.json records exact argv/cwd/start/nice/parents;
LIVE/loaded_runtime_startup.json covers all6CFDranks and mapped ELFs with no
uncovered library.1517pins, fresh current-host GCC13.3 build210/runtime-v2.
Source checkpoint6744638324 PUSHED. Same saved real CFD seed,50k metric complexity,
2events,5400native budget; strict gates unchanged. No selected failure dump.
Keep all executing/pinned sources, helpers, binary/config/inputs unchanged. Do not
signal or restart from stale logs/time alone; verify this exact owned session.
Only this heavy workload is live. R/euler_build210_launch.json records TMPDIR,
pt2pt backend/isolated4TB MPI scratch, argv and CFD SHA. Current resource/handles
are in LIVE receipts; historical206terminal values below are comparison evidence.
At complete matching stages compare whole-field q/J/geometry counts, net removal,
query traversal/work and CPU/elapsed counters; changed toolchain/runtime prevents
isolated attribution of wall-time differences. No accepted mesh claimed yet.
LIVE/initial_construction_comparison.json now preserves first complete6-rank
construction exits vs206. Coarsening:292680cells/2970geometry-bad/55181size-bad,
2632commits vs292709/3512/55180,2627commits. Surface exit294000/3171/55678,
3226cumulative commits vs294186/3861/55674,3287commits. Upper-work ratios1.0506/
1.1202, max engine elapsed428.451/462.763s vs239.095/266.788s. All6census/cache/
geometry flags complete, shape_complete0 and q/Jmeasured0 as required. Geometry
counts include conservative refusals, so lower counts do not prove proportional
physical improvement. Fresh compiler/runtime/contention prevent isolated elapsed
attribution. First complete quality ENTRY now721643cells/49380q/48218J/2713geometry/0size,
all6census/cache/shape/geometry flags1 and allcells q/Jmeasured. Compare206entry
727818/54081q/53392J/3770geometry/0size. New entry has4701fewer q-bad,5174fewer
J-bad and1057fewer geometry-admission failures; this is a smaller starting burden,
not measured repair yield/acceptance. LIVE/quality_entry_comparison.json preserves
all6rows, counters and source prefixes. Keep run frozen; assess net q/J removal
per second with geometry counts at complete quality windows. No stop justified.
First complete quality window census3/round20928 now723343cells/48006q/46652J/
2700geometry/0size. Net1374q/1566J/13geometry removed in81.966318quality seconds,
725commits. qretired18987/introduced17613, J17788/16222 reconcile to whole-field
net; do not judge by commits or debt alone. LIVE/quality_window1_assessment.json
preserves all6stage/service rows, timing limits and source prefixes. Baseline206
first complete window is158.290quality seconds, not matched to this early window.
Latest preserved trend round21440: 757962cells/24671q/21743J/
2624geometry/0size, 2200.192428quality seconds after entry.
Net since entry24709q/26475J removed through16424quality commits.
All6complete flags1. LIVE/quality_trend_assessment.json preserves9measured windows.
Last interval removed2026q/2097J in154.791793s. Rates vary
with workload/contention under frozen code; no isolated attribution or forecast.
The preserved EARLY nearby-time comparison (967.730s vs206912.143s,55.588s gap)
remains in LIVE/quality_comparison_early.json with immutable source-prefix hashes;
it is not the latest window or an exactly matched-time benchmark. No stop justified.
Earlier staged→committed ownership migration at20992 preserved728526cells/
44355q/42667J/2675geometry/0size, all measurement counts and q/Jdebt sum.
LIVE/quality_ownership20992_assessment.json records per-rank rows/source prefixes;
count preservation alone does not prove every attribute/history. Run remains live.
Latest preserved profile census11/round21184 has743174cells/34688q/32455J/
2643geometry/0size, all6complete flags1, maximum engine2246.689082s. Source/handles
stay frozen/live; no acceptance or terminal forecast. Existing-counter profile is in
LIVE/quality_profile_census11.json. Published interval net2789q/2920J reconciles:
coordinate471q/478J, optimizer2184q/2312J, collapse/reconnect134q/130J. qbad7369surface/
27319interior; J10734surface/21721interior. Interior can be constrained. Smoothing
merit/moves are PRIVATE cumulative trial diagnostics merged before publication
(source3776), not published gains. Rejection bucket0 combines geometry/topology;
it does not prove a distance violation. Published boundary/interior repair yield
cannot be fully separated with existing counters; do not infer it from location.
Checkpoint293b8361bf verified on fork2026-11-06; no owned heavy handles live before edits.
Current unvalidated change: budgeted entity direct-bound failures try the existing
singleton-support physical union query, retain the tighter upper and independent
planar bound; cheap successful direct paths unchanged. Admitted scratch before
allocation; work/storage exhaustion propagates. Actual saved-face fixture adds
production-scope agreement, cheap-path and resource-refusal checks. Runtime-v2
193files includes observed hwloc modules/dependencies; original175pins preserved.
Build208 TERMINAL1 before compilation (3.132s), frozen pins unchanged: old
generated link dependencies require absent glibc2.31. Separate tool /proc scan empty (namespace-limited, not a host-wide proof);
exact build208 tool session83130 TERMINAL1.
Old generated configuration identifies GCC9.4; current compiler GCC13.3. A fresh
current-host Meson configuration preserves all options at
`integration_evidence/build-integrated-v3-currenthost`; setup TERMINAL0. Active
helpers now use that build and pin its generated configuration. Build210 exact tool session34871 TERMINAL0; helper PID4 is execution-
namespace local, never treat it as a host PID. Fresh current-host objects from209are retained, nice19/3workers/single-threaded libraries.
Freeze production/test sources and helpers through build/validation.
Build209 exact session16890 TERMINAL1 after1507.450405s, all1301pins unchanged.
Only compile failure: CNativeAffineReference.cpp used std::optional without its
direct<optional> include. Independent compilation finished (873/876steps).
R/build209_terminal_session.json and discovered_failure.json preserve evidence.
After terminal verification added the one direct include. Build210 reuses the
successful current-host objects and rebuilds the failed object/dependent targets.
No algorithm or acceptance changes accompany that include. Freeze current sources
and helpers through build210/qualification. Earlier tail-only updates missed the
209compile error; no successful build was claimed.
Build210 TERMINAL0 in45.113142s,1301pins unchanged, both targets up to date.
Geometry210qualification COMPLETE: actual captured_cavity38938assertions/1case
PASS89.592s; projection_paths2666assertions/34cases PASS53.350s; MPI6all6ranks
PASS23cases each9.532s. Runtime-v2covers all observed ELF mappings and all6workers,
post-exit hashes stable; pinned sources/helpers/binary/config unchanged.
R/geometry210_qualification.json, validation_progress.json and case receipts.
Initial sessions63687/31432 failed before tests (missing witness config/sandbox
sockets), preserved. Host session85808 passed both serial cases, then MPI fixture
failed because launcher omitted TMPDIR; only verified mpirun181079/start444653
received SIGTERM after pins/error/handles preserved; all10handles now absent.
NEVERrepeat that signal. Corrected MPI-only session87663 TERMINAL0 used pt2pt and
4TB TMPDIR, no source change; serial evidence reused. Tests do not prove full M6.
Full attempt euler_build210_real_cfd50k_bound_refinement is now live as above, same
saved realCFD seed,2events,50k,5400native budget. No
routine rejected-mesh dump; future selected export includes requested metric.
No speedup or full-mesh acceptance established. Full M6 follows affected checks.
User observes poor leading-edge mesh. Read-only selected202localization receipt
`INSPECT202/leading_edge_observation_assessment.json`: forward coordinate strip
xi=x-y/sqrt(3) in[0,.02), |z|<.08, y[-.01,1.23] contains281/450shape-union,
1371/2110geometry-bad. Region is a proxy, not association/feature classification;
no clean-population denominator or construction causation claimed. Two worst
q witnesses151610623/152331796 are existing saved-face probes whose alternative
bounds remain above tolerance. Current correction has not demonstrated shape repair or closure
of that leading-edge tail. Keep this localization within the active cycle;
no competing construction/operator implementation while build is frozen.
User requests metric field in NEXT selected rejected-mesh VTU. Offline exporter
(not in build209pins and not used by live build) now writes symmetric9-component
PointData `captured_nodal_metric` plus UInt8 availability to whole/bad/cavity VTUs.
Shared known values must agree exactly; absent values are NaN. All12offline tests
PASS, frozen1301build pins unchanged; R/metric_vtu209_qualification.json.
Current M6 logs full_target=1, so this is its current nodal target. In composed
modes it is only the captured sensor, not reconstructed query-time composition
or original background. Surface extraction inherits same3Dfield; permitted
surface motion uses its tangent restriction. No separate surface metric invented.
Existing selected202mesh stays unchanged; no per-attempt dump is requested.
LAST=`R/m6_runs/euler_build206_real_cfd50k_feature_companion`.
INSPECT202=`R/m6_runs/euler_build202_real_cfd50k_inspection`.
Build207 exec83367 TERMINAL0,331.648681s,1298source pins unchanged and targets up
to date. Exact HOST helper2320271/start97086031 and Ninja2320273/start97086042
ABSENT, verified2026-11-06 in R/build207_terminal_handles.json.
R/build207_owned_startup.json preserves argv/cwd/children.
Helper status uses namespace PID10 for Ninja (HOST2320273), NOT hostPID10. Initial
invalid namespace lookup is preserved/corrected in that receipt; no signals sent.
Focused helper exec61871 TERMINAL1; case captured_cavity_20261106T090838_20867
stopped BEFORE worker launch, status exit2/pins_verified=false: historical loader
/usr/lib/x86_64-linux-gnu/ld-2.31.so is absent. No test_driver/command was launched.
Historical libc/libstdc++/MPI/PMIx paths changed on this host. Old runtime receipts
stay preserved; qualification under that runtime does not establish current-host
behavior. R/runtime_20261106 is being captured from both binaries' ldd dependencies
and current MPI/PMIx modules/dependencies. Active validation helpers will verify
that versioned snapshot and loaded-library coverage before the focused retry.
Subsequent current-runtime attempt captured_cavity_20261106T091823_38316 failed
MPI_Win_create before any geometry query (exit53,pins unchanged). The pinned
OMPI_MCA_osc=pt2pt backend initialized successfully. Attempt
captured_cavity_20261106T092108_43129 exec76931 TERMINAL0,100.622s,
38924assertions/1case PASS; all5exact HOST handles ABSENT, pins unchanged.
Current175-file runtime snapshot covered ldd/MPI modules, but actual maps found
additional hwloc modules/dependencies. Their observed hashes stayed unchanged
through exit; NEW runtime-v2 now includes them before production qualification.
The historical175-file manifest remains preserved. Build210 is terminal; qualification is terminal; full M6 session54752 is the sole live workload.
Historical build207 changed only the diagnostic fixture/helper input pin:4saved202bad/clean faces,
recorded direct bounds, existing union query, exact cached/fresh comparisons and
1,048,576work/query. R/geometry_probe207_assessment.json: one saved bad face
(cell152256924/facet10699) has direct1.2346577241708644e-4 but existing union
9.4657910071089152e-5, rigorous directed within1e-4. Four cached/fresh results
match exactly; all52oldq/J bit-exact. Two other bad faces remain uncertified;
their union bounds are worse, so retain the better bound. Nearby clean face
direct7.4427575719935332e-5 should keep its cheap path. No mesh acceptance/speedup.
Implemented, not yet qualified in build209: shared ParameterError refines failed direct bounds
through EXISTING physical union query when budgeted, preserving min of valid
upper bounds and independent affine maximum. No new cache/config. Runtime-v2
closure and focused geometry/constraint/resource/MPI validation precede full M6.

Most recent full M6 outcome:
Normal full M6 exec39395 is TERMINAL1. Event1 naturally failed the unchanged
native time budget on all6ranks BEFORE the authorized owned-launcher stop.
COMPLETE actual first EXIT round24710: **809684cells/281q-bad/341J-bad/
2129geometry-bad/0size-bad**, max engine5344.974750123s. All6 EXIT census/cache/
shape/geometry flags complete; DETAIL complete=0 means the capability failed,
not that the census is partial. Three quality stages and two construction returns.
ZERO accepted adapted meshes; no adapted transfer/continued CFD/restart proof.

Natural failure authority LAST/event1_natural_failure.json,
SHA8714a8ebd138f54a660fbc6a4ec303bd83fe3e14f1888a17d8a8a676c4a5674b.
It preserves all6 full EXIT/DETAIL rows, exact source prefixes and pre-stop pins/
owned handles. LAST/event1_failure_owned_stop.json records the single SIGTERM
already sent to launcher2159607/start96386787 AFTER preservation. NEVER repeat it
or signal historical2159xxxPIDs. Driver had begun the next original-grid event;
that output is not an adapted continuation or restart proof. Two accepted events
could no longer be obtained in this configured two-event attempt.

LAST/terminal_handles.json proves all10exact owned handles ABSENT at
1793554482.9539447 and1403pins unchanged. Sources were then safe to edit and were
frozen through successful207build. No owned M6 job remains. Host then MemAvailable40,200,848kB/load5.55; rerun resource and
handle checks before sustained work. Overall helper5619.598s includes startup,
post-rejection original-grid work and owned shutdown, not just first native work.
LAST/terminal_resource_assessment.json:2731existing tree samples peak15,283,798,016B
summed RSS (not physical/job peak); largest individual2,860,605,440B. Sparse live
host snapshots had at least30,466,872kB available; no pressure-driven stop occurred.

Source checkpoint12a14dc955 PUSHED, unchanged throughout build/run.
CFD SHAbb4a5432351a170404332fb89edc7a3f4d6e1a4036adb155838bc1c090a85214.
Config SHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d
and real CFD seedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880
exactly match202. Initial restarted original-grid CFD finite, CL.269535/CD.013536.
No selected206private mesh dump:202actual SU2/VTU remains the saved inspection
mesh; do not create per-trial or routine-attempt mesh files.

First COMPLETE quality entry727818cells/54081q/53392J/3770geometry/0size at
649.508977468engine seconds. Counts and selected cumulative work match202 entry;
seven earlier complete construction stages also match selected state/work exactly.
LAST/quality_entry_comparison.json and construction_matching_stage_assessment.json.
This preserves selected construction behavior, not whole-mesh bit identity.

Complete phases: first quality_exit804645cells/46q/28J/3045geometry; first rebuilt
entry806031/550q/645J/2758geometry. Second quality_exit807610/39q/22J/2592geometry;
second rebuilt entry808914/499q/632J/2229geometry. Actual EXIT above supersedes
last window809464/360q/442J/2167geometry. Neither small shape tail was closure.
Geometry reduction reopens substantial shape counts; this is not proof that the
same cavities cycle. LAST/phase_progress_assessment.json preserves all6 transition
rows/flags, times and prefixes. quality_trend_assessment.json retains45complete
windows from FIRST native event only. Do not mix the interrupted second event.

Compared with202 EXIT810163/295q/294J/2110geometry,206has14fewer q-bad but47more
J-bad and19more geometry-bad. The connected feature proof fix has not established
whole-field closure. Host/query/ownership trajectories differ; no isolated timing
claim. Earlier matching-time and work receipts remain linked in LAST. Executed
round21696migration preserved counts/flags/debt sum and reduced debt max/mean
1.30966→1.14391; balance improvement alone was not acceptance.

Active cycle now: ANALYSIS FIRST of geometry admission and shape/geometry reclosure.
LAST/terminal_counter_assessment.json reconciles all7diagnostic groups through EXIT:
53290published ladder commits,54764q/54278J removed minus construction reopenings
964q/1227J = net53800q/53051J. First two shape tails ALL blocked (46/46and39/39,
empty queues); final18of466shape-bad blocked,queue4920. Final q139surface/142interior,
J220surface/121interior; all466short<.5,not construction causation. Drift16/16supported,
max4.04324%; immutable continuous target fidelity remains unproved.
Source review confirms existing reuse/screens; direct single-patch sufficient
geometry bounds may conservatively refuse. The terminal207diagnostic reused selected202actual bad/clean faces and
demonstrated one conservative refusal using EXISTING rigorous physical union
queries. The selected correction is now in frozen current-host build209. See
Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md. Production correction unvalidated.
No new operator,
cache, scheduler, config option, construction q/J policy or gate relaxation is
authorized by an unproven idea. Reuse selected202actual cavities/reference before
another capture/baseline campaign. Record unrelated ideas in the linked backlog.
Then focused affected validation and one MPI milestone as needed, immediately full
M6 with same real CFD seed/50k/unchanged budgets. Goal remains two strict accepted
meshes, conservative admissible transfer, finite continued CFD and isolated actual
adapted disk restart, then dedicated BL/MPI qualification and RANS500k.

## Connected206 qualification — scoped PASS, not workflow completion

Build206 exec38075 TERMINAL0,261.348s,1298pins unchanged; exact ninja2136532 absent.
Connected strict retained-feature companion proof and incremental RebuildFeatureTraces
byte accounting QUALIFIED locally: feature_publication153assertions/4cases PASS;
actual captured_cavity38878assertions/1case PASS; ONE MPI6 milestone23cases EACH
rank PASS (six success totals3058,2478,2165,2313,2309,2342in interleaved stdout order,
not rank-mapped). All owned handles absent. R/feature_proof206_qualification.json.

Actual52cell witness reproduces ALL oldq/J bit-exactly, edge error<=2.4042e-17.
4502→181199364now passes compatibility,12→9construction, parameter replacement,
metric merit and geometry guidance0→0. It STILL has1q/1J bad (unchanged counts),
debt1.99999995486→.15423814413; not shape closure or accepted repair. Global contact/
publication not checked on this actual witness. Self-contained bent-feature test
checks complete publication/clearance with unchanged companion on remote rank,
emptyMPIrank5, lost-arc refusal, revision+1/stale refusal. Existing MPI suite checks
conflicts/curved associations/heights/migration/census/cache/resource/capture cases;
not general BL qualification. Current boundary reconstruction has760validated
traces but historical trace identity/peer schedule is not claimed.

Change scope: ONLY a newly genuine trace on an already accepted physical edge gets
its unchanged accepted companion face's directed contribution. No companion volume/
face edits, degree changes, gate waivers or scheduling changes. Extra scratch
admitted; exact unsigned coverage and established-route coordination remain strict.
205accounting removes repeated whole-map byte scans, with four full-scan invariants;
allocation overlap/work/routes unchanged. Its34case/2666assertion serial checks PASS.
Global reconstruction now completes, but no ordinary-M6 speedup established.

The qualified implementation is now frozen in the normal full M6 above, same real
CFD seed/50k/5400/gates. No further operator or diagnostic cycle while healthy.

Diagnostic history:203arbitrary100m allowance exhausted before candidate;204whole-
boundary resource callbacks timed out600s before candidate. All handles absent/
pins unchanged; evidence preserved.205relevant source/helper snapshots (7files)
recovered and verified against original pins under R/witness_sources/build205/.
Boundary preparation810163paired rows/38752points/77500faces/52metrics passed;
initial prep interruption withoutcapturedhostPID preserved in LAST receipt. Never
resignal any historical handles. The qualification jobs are terminal; the normal M6 above is now the only owned heavy job.

## Previous selected M6 — build202 terminal inspection

Selected full-driver attempt exec88642 TERMINAL1. Event1 naturally rejected on all6
ranks at native5405.620649636s, stop=time_budget. COMPLETE actual EXIT:
**810163cells/295q-bad/294J-bad/2110geometry-bad/0size-bad**.
All6 full shape/geometry/census/cache flags complete; raw inspection counts match.
ZERO accepted adapted meshes; no adapted transfer/continued CFD/restart proof.

Natural failure was preserved BEFORE the authorized stop of the exact verified
owned launcher. INSPECT202/event1_natural_failure.json and event1_failure_owned_stop.json
are authority. Stop already sent; NEVER resignal historical1886xxxPIDs.
INSPECT202/terminal_handles.json proves all ten exact owned handles ABSENT and1403pins
unchanged. These M6 handles are historical. Sources were unchanged throughout the run.
Source checkpoint18738aeb32 and progress942c154d17 are pushed.
CFD SHA7de0cc448004521becb5609d83a0841836d7744187700244dd36d8538b8ef666.
Test-driver SHAd630234ec127b793237fb7c1dc609cd9db216b96463fc5330560a1fa01acaf04.

Quality entry COMPLETE727818cells/54081q/53392J/3770geometry/0size at
636.681267335engine seconds. Construction matches199on17selected counters across
all18generations, not whole-mesh bit identity; host and later ownership differ.
Bulk shape repair reached49q/26J/2988geometry at quality4362.75369s, with46of49
q-or-J-bad cells fully blocked, q46surface/3interior and J26surface/0interior.
Later complete window45:38q/22J/2554geometry at quality4597.57455s.
Actual EXIT295q/294J/2110geometry supersedes those windows. Detailed counters show
quality_stages=3 and construction_returns=2: the existing continuation WAS exercised
in202, unlike199. Aggregate reopening is not proof of repeated local cycling.
This motivated the strict feature-proof correction qualified above; the current
full206attempt must now establish its effect on whole-field closure.

All6 explicitly selected first-failure raw shards and measurements are complete.
Offline extractor exec17281 TERMINAL0,31input hashes unchanged, all three owned
extraction handles ABSENT. Actual complete files now exist at
INSPECT202/inspection/rejected_mesh.su2 and rejected_mesh.vtu: **164526points/810163tets**.
2476bad cells/35426neighboring vertex-star cells, zero omissions/unknown metrics.
Independent standalone SU2 parse and count checks passed; raw flags match EXIT.
Generic SU2 markers0/1/2 correspond to FARFIELD/SYMMETRY/WING3D, verified from
original sidecar. Actual files stay local, not committed. All10output hashes in
INSPECT202/inspection_artifacts.json. Extraction sampled summedRSS720207872bytes,
wall437.704s; this offline cost is separate from native runtime. Extraction is terminal.

Actual final shape-bad union450:243surface/187constrained-interior/20with no
captured binding. All2110geometry-bad cells have physical faces. Short-edge controls:
32948of32950nearby all-bad-flags-clear cells also have an edge<.5; no construction
cause established. Complete phase totals show48q/26J→571q/666J after first rebuild,
then38q/22J→521q/642J after second; final295/294. Partial construction shape counts
are not whole-field truth. INSPECT202/terminal_phase_assessment.json,
inspection_analysis.json and inspection_clean_controls.json retain evidence.

Selected witness for the206correction: cell143424775 q4.5129e-9/J2.6424e-13, geometry excess0,
physical face4502/174173910/181199364, all genuine-curve mobility1, all14repair
demands blocked. Complete52cell union of its four vertex stars saved in
INSPECT202/witness_143424775_owned.bin and witness_143424775_state.json. Exact integer
orientation test found4502→181199364 keeps all surviving cells positive; this is
NOT topology/geometry/metric/height/contact/dependency admission or a historical
trial replay. Existing CollapseVertex/current-cavity metric/feature-chain facilities reproduced
the old measures and demonstrated the refusal corrected above. Do not treat missing
explicit traces as empty accepted traces or add another operator.
The current target remains repair power and geometry/shape reclosure. The206
change is frozen in its full M6; no full workflow success is established.
Details and actual mesh links:
[inspection](Handbook/ADAPTATION_M6_INSPECTION_20261101.md),
[bounded export](Handbook/ADAPTATION_BAD_CAVITY_EXPORT_20261101.md).

## Connected diagnostic qualification — PASS, not workflow completion

Build202 PASS; serial6845assertions/14cases; one MPI6 milestone22cases EACH rank.
All24actual C++ producer/reader combinations PASS (serial/MPI6, planar/genuinely
curved, quality/construction, full/unknown/capped);636artifact hashes retained.
Four full MPI cases verified complete cross-rank vertex stars and empty rank5.
Cached q/J/edge values match direct known-field measurement; construction q/J stay
unmeasured; migration preserves cached values/census flags; captured and uncaptured
repair cells/work counters match. Curved controls/body/features/bindings/requested
heights, overwrite refusal and scratch/disk cap refusal exercised.11offline Python
synthetic tests PASS. R/failure_export202_qualification.json and
failure_export202_actual_interop.json preserve exact scope/provenance.

Build201 serial found4stale transport test assertions: current-metric snapshot
performs one workers.maximum before entering the engine, so worker total includes
that call. Whole transport aggregate was already correct. Corrected nesting test
explicitly requires the extra call for a single ordinary stage; mesh gates were
unchanged. All build200/201/202 and serial/MPI handles are terminal/absent; no old
handle may be signaled. Source freezing for current M6 began after qualification.
Earlier implementation/test details and terminal199 analysis are preserved in the
[export-validation archive](Handbook/History/HANDOFF_Codex_export_validation_20261101.md).

## Historical comparison — build199, no accepted mesh

LAST199=`R/m6_runs/euler_build199_real_cfd50k_triangle_coverage` (WINDING199).
All6 naturally rejected at native5403.368014s; COMPLETE EXIT802117cells,
4368q-bad/2537J-bad/3098geometry-bad/0size-bad, q+J debt36.326780.
Quality4131.482941s;43866publications; net49713q/50855J/672geometry removed:
12.032725netq/s and12.309140netJ/s. No conservative transfer/adapted CFD/restart.
Original-grid restart00001 and continued history after rejection are not adapted
proof. No private working mesh was saved in199 and it cannot be recovered from
counters or original-grid restart. Natural failure preceded the verified owned
stop; all old1607xxx identities absent/pins unchanged before subsequent edits.
NEVER resignal old handles. LAST199/event1_natural_failure.json and
terminal_handles.json preserve exact proof.

Traversal87.89%of upper-query work; boundary private gain906.097/3367.753ranksec,
interior4623.378/203.816 with0interior upper queries. Private merit is not published
bad-cell gain. Finalq745surface/3623interior;J656/1881. Only33fullyblocked cells;
all6132q-or-J-bad cells have edge<.9,6045edge<.5. Correlation is not construction
cause. Construction returns0:197geometry continuation was unexercised in199.
Shape closure then residual size/geometry closure and shape reclosure remain needed.
Selected199states matched198retry1 throughwindow20; later ownership/trajectories
differ. Baseline198retry1 actual EXIT counts UNKNOWN. Work/timers overlap and host
pressure varied; no isolated runtime speedup claim. Full terminal counters and
matching comparisons remain in LAST199/terminal_assessment.json,
terminal_counter_review.json and geometry_certification_analysis.json.

**Read [HowToProceed.md](HowToProceed.md).** Construction-quality preferences,
geometry motion/envelope bounds and other suggestions are hypotheses, not approved
production policies. Diagnose actual remaining cavities, test a selected connected
hypothesis thoroughly, then assess net whole-field q/J-bad removal per second with
geometry-bad counts. Do not choose from commits/debt percent alone, add competing
implementations, relax gates or raise budgets. Unrelated/unproven ideas belong in
[backlog](Handbook/ADAPTATION_BACKLOG.md).

New attachment reviewed read-only while202was frozen:
[technical assessment review](Handbook/ADAPTATION_TECHNICAL_ASSESSMENT_REVIEW_20261101.md).
Confirmed: geometry-bad can include unresolved conservative upper bounds, not
proved violations; final two-sided certification remains strict. Interior labels
mean no physical face, not necessarily free vertices. Current export is inspection,
not faithful trial replay. Use actual captured witnesses/clean controls before
selecting a change; do not interrupt202or implement attachment proposals now.

Historical191–197 and198 records:
[191–197 archive](Handbook/History/HANDOFF_Codex_build191_to197_20261101.md),
[198 terminal archive](Handbook/History/HANDOFF_Codex_build198_terminal_20261101.md),
[199 archive](Handbook/History/HANDOFF_Codex_build199_20261101.md).
All their handles are historical. Incomplete EXIT flags mean whole-field counts
are unknown, never substitute an earlier subset/window as terminal truth.

## Next diagnostic and complete acceptance obligations

Build202 terminal evidence, completed extraction and exact handle closures are
preserved. Use its actual saved witnesses before selecting a repair change. Reuse
existing reconstruction/metric facilities and require evaluator agreement and
complete constraints before interpreting a local feasibility test. Do not launch another baseline
campaign. Passing diagnostics or a saved rejected mesh do not complete the goal.

Analysis caution: historical matching-window summaries assume the first quality
pass. The retry1 stored command now distinguishes published repair churn from
whole-field construction changes if the197return crosses construction. Adapt analysis assertions to
actual phase/census coverage; an analysis assertion alone is not a solver failure.

Read-only downstreamreadinessreview:
R/m6_runs/euler_build196_real_cfd50k_witness_first/workflow_acceptance_readiness.json.
Existing175transfer_restart_acceptance_source_review.md and audit_native_unsteady.py
were located/reused; P1integralaudit needs IMMEDIATEposttransfer states,notlaterCFD.
Existing workflowreceipts alone are insufficient transfer/state/restartproof.
Actual pinned distributedtransfer unconditionally gates finite trueresidual<=1e-10,
fulladmissibility and1e-12scaledtargetintegrals before publication; inspect ACTUAL
per-eventprojectiontable/source provenance. Donor-vs-newintegralchange is a different
quantity (CLOSEDsliverrule/open-boundary domaincorrection). Existing validate_state
can be reused on an isolatedcopy of ADAPTEDmesh namedm6.su2 and actualmatchingflow;
functionuses parsedpointcount,not61330hardcode. Thenactual RESTART_SOLYES/ADAP_LOOPNO
probe with distinctinput/outputprefixes onlyafterownedrunterminal. No newhelper/
source/configchange now; no adaptedstate/restartclaim.

Existing closure plan: shape/height → residual size → geometry-only repair via
EnableGeometryRepairs/strict complete-patch guidance,or valid construction return,
then shape remeasurement/reclosure.Neither191nor193reached geometry-only entry. Same
cumulative5400s/work/memory limits. Require TWOstrict accepted mesh/native_ref pairs,
actual conservative admissible transfer(double residual1e-10,targetintegral1e-12),
finite continued CFD,rho/internalenergy>0. Then isolated COPYfinal adapted mesh/
native_ref+matchingflow,actual RESTART_SOLYES/ADAP_LOOPNO/correct SOLUTION_FILENAME/
distinct outputs,coordinate/order/state audit. Original seed00000 is NOT adapted
restart evidence. Then dedicated BL curved/junction/interface/subsequent-operation/
redistribution/requested-height MPI qualification and RANS500k. Goal stays ACTIVE.

Readonly seed R/m6_inputs/euler_original_force_seed_v1:61330points/303359cells,
Mach.8395,AOA3.06,CLmean.2695353537415/CDmean.0135358565364,last200ranges1.114e-7/
3.121e-8,finite/rho/IEpositive/exact coordinate/order. Userforce-policy qualified,
not a residual-converged aero reference. SeedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880.
Reuse per first remesh; distinct SOLUTION_FILENAMEbaseline_euler_seed/RESTART_FILENAMErestart_flow.

Open limits: historical191transported nodal tensors drifted~8.3%at16fixed samples;
193/196/197samples are recorded with their actual windows in the archived evidence.
Neither runtime tags nor these samples prove immutable original continuous-CFD-target
fidelity. General
adaptation resume cycle0/path collision and rejected-cycle consumption remain open;
isolated solver restart does not prove general adaptation resume fixed. Broad182MPI
timed out; serial_full170 NOT passing. BL/RANS qualification not started.

## Operating links and discipline

[AGENTS](AGENTS.md) · [complete task](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md) ·
[winding screening](Handbook/ADAPTATION_GEOMETRY_WINDING_20261101.md) ·
[certification analysis](Handbook/ADAPTATION_GEOMETRY_CERTIFICATION_20261101.md) ·
[envelope](Handbook/ADAPTATION_GEOMETRY_ENVELOPE_20261031.md) ·
[memo](Handbook/ADAPTATION_NUMERIC_MEMO_20261031.md) ·
[ownership](Handbook/ADAPTATION_CENSUS_OWNERSHIP_20261031.md) ·
[geometry/smoothing](Handbook/ADAPTATION_GEOMETRY_QUERY_REUSE_20261031.md) ·
[feature](Handbook/ADAPTATION_FEATURE_PUBLICATION_20261031.md) ·
[saved CFD](Handbook/ADAPTATION_M6_CFD_SEED_20261031.md) ·
[cache/ownership](Handbook/ADAPTATION_CACHE_PHASE_20261030.md) ·
[backlog](Handbook/ADAPTATION_BACKLOG.md).

One owned heavy job,nice19,libs1,build2–4workers(helper3),MPI6bounded; TMPDIR4TB
compile-tmp,not nearly full root/tmp. Check host RAM/load/disk; sampled summedRSS
is not physical memory/job peak. Preserve resource/pin/handle receipts,leave other
jobs alone. Fork pushes authorized:originhttps://github.com/rois1995/SU2.git branch
codex/native-3d-bl. Git metadata outside writable root:escalated git -c gc.auto=0
-c submodule.recurse=false; diff/status --ignore-submodules=all. Whitelist adds; no
binaries/meshes/restarts/rawperf. Leave unrelated155logs/.orig/submodules/untracked
steering/history files untouched. Primary owns integrated design/source/acceptance;
bounded read-only routine agents done, no independent jobs.
