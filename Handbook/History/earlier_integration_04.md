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


