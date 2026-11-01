# Native adaptation — current handoff

Updated2026-11-01(Europe/Rome). Goal ACTIVE: TWO strictly accepted ONERA M6 Euler CFD → mesh →
CFD → mesh events at50k metric complexity; actual conservative admissible transfer,
finite continued CFD and isolated usable adapted disk restart. Then dedicated BL/
constraint/MPI qualification and corresponding RANS500k workflow. Actual point/cell
counts are separate from target complexity. Geometry1e-4,q>=.2,J>=.05,maxedge1.8 and
requested heights stay strict. Construction preserves exact positive/nondegenerate/
topology/body/feature/height validity and deliberately omits q/J. CAD optional;
no-CAD geometry authority sufficient. ZERO accepted adapted M6 meshes so far.

## Current — build206 normal full two-event M6 LIVE; source frozen

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
LAST=`R/m6_runs/euler_build206_real_cfd50k_feature_companion`.
INSPECT202=`R/m6_runs/euler_build202_real_cfd50k_inspection`.
Full M6 exec39395 RUNNING with normal SU2_CFD, not a selected test-driver wrapper.
Source checkpoint12a14dc955 PUSHED.1403runtime/source/input pins. ConfigSHA74b2e9f476cf6ba1fdc5ebcdcb1c34a2b3c45b25e58d651e75417d89402bfe8d
and saved real CFD seedSHA1eaee788bd62bd92a92ec539577dcbcfc0f8042dcbcec23adb04e825bb01a880
exactly match202. CFD SHAbb4a5432351a170404332fb89edc7a3f4d6e1a4036adb155838bc1c090a85214.
First restarted original-grid CFD step finite, screenCL.269535/CD.013536; native
adaptation entered. No accepted adapted mesh/transfer/continued adapted CFD yet.

First COMPLETE quality entry now recorded on all6 ranks:727818cells,54081q-bad,
53392J-bad,3770geometry-bad,0size-bad; maximum engine elapsed649.508977468s.
Counts and selected cumulative geometry/work counters exactly match202 entry
(636.681267335s); J-debt sum differs only1.82e-12 from rank summation. Ownership/
host conditions differ, so no isolated runtime claim. LAST/quality_entry_comparison.json
records all6 raw row locations and observed log-prefix hashes. Seven earlier
complete construction stages also match selected state/work counters exactly;
LAST/construction_matching_stage_assessment.json. These do not establish closure.
Latest COMPLETE window round21824:749172cells/39886q/38086J/3518geometry/0size,
at771.76424484quality seconds (engine1421.273222308). Latest-window net removal
2796q/2973J/45geometry. q9746surface/30140interior; J13684surface/24402interior.
Interior can still contain constrained vertices. LAST/quality_progress_snapshot.json
retains first4complete windows, per-rank counters/location and matching202rounds.
At round21760, previous-window net removal was3494q/3558J/45geometry over
253.492094885s,2164commits. Matching202round interval
removed3171q/3363J/57geometry over178.16399285s,2042commits.206traversal work is
2.84%higher, netq/commit1.615vs1.553, but elapsed42.28%higher. Different queries,
ownership and host scheduling prevent isolated code/contending-job attribution.
LAST/quality_window3_work_comparison.json records selected per-rank work/timers,
whole-field churn reconciliation and exact observed prefixes; both receipts
are included in this evidence checkpoint.
The targeted late-tail effect is not established. No stop justified.

Executed round21696redistribution preserved739233cells/46176q/44617J/3608geometry/
0size, complete shape/geometry/cache flags and aggregate q/J debt (within1e-9);
q/J-debt max/mean fell1.30966→1.14391. This is measured balance/count preservation,
not throughput or every-attribute identity. LAST/quality_census_migration_round21696.json
and quality_first_window_comparison.json pushed in2138e8092b; matching-entry
checkpoint b2af598568. Latest HOST audit1793550495.5545266:all10exact handles LIVE,
1403pins unchanged,load6.81,MemAvailable30,712,388kB,sampled summedRSS9,363,513,344B
(not physical/job peak). No actual resource pressure.

HOST startup at1793548936.893354, all10LIVE:
helper2159446/start96386176;time2159605/timeout2159606/start96386786;
launcher2159607/start96386787;ranks2159611–2159614/start96386794,
2159615–2159616/start96386795. Launcher/ranks nice19, libs1; helper0.
LAST/owned_startup.json/command.json/pins.json/status.json/resources.jsonl are live
authority. StartupMemAvailable38,709,816kB; sampled summedRSS2.202GB at100.108s,
not physical/job peak. Unique OMPI4TBcompile-tmp/native_feature206_m6. No selected
failure dump:202actual mesh remains saved; no routine/per-trial mesh files.

Do not edit C++/headers/tests/helpers/binaries/config/inputs or launch another
heavy job while this healthy run lives. Let it finish or preserve demonstrated
failure/bottleneck/resource evidence before an authorized exact-owned stop. No206
stop sent. Before any retry check SAME exec39395 and exact host startticks; stale
logs/timeouts alone do not establish termination. At natural first-event rejection,
preserve all6complete EXITs/stops/pins/handles BEFORE stopping only verified owned
launcher if two accepted events can no longer be obtained. Never resignal oldPIDs.
Judge net whole-field q/J retirement+introduction, geometry and size counts at
matching complete stages; witness debt reduction is not closure. Shape→residual
size/geometry→shape reclosure→two strict accepted events with conservative admissible
transfer/finite continued CFD→isolated adapted disk restart, then BL/MPI/RANS500k.

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
