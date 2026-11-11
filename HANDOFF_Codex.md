# Native adaptation — current handoff

Updated2026-11-11 Europe/Rome. Goal ACTIVE. ONE connected curved-BL cycle and
one owned heavy workload. R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md, [HowToProceed.md](HowToProceed.md), the current
[stage cadence/sequence](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[normalization evidence](Handbook/ADAPTATION_CURVED_BL_NORMALIZATION_20261111.md)
and maintained [timing/work ledger](Handbook/ADAPTATION_TIMINGS.md).
Latest user cadence supersedes immediate-full-run retries: finish credible ordered
stages with reconstructible checkpoints, then full integration milestones. Keep
construction validity-first; intermediate quality debt is provisional, not acceptance.

## Current state — authoritative

Current owned workload:371actual saved-CFD METRIC-ONLY stage RUNNING/FROZEN,
session15803; R/cases/curved371_saved_cfd_metric_stage_20261111T111921_1888531.
Wrapper1888531; time1888611. Exact AF7seed/MPI6/Mach-only L1/nominal500k,
unchanged normal/geometry/budgets; no CFD advance or construction. Do not edit
pinned sources until actual-host terminal. R/joint371_qualification.json.
Build371 TERMINAL0/58.132s,14owned identities absent, pins intact;
CFD SHA7cf80707d006eb2c5d80afa1143b730e8502e1458e1f9fe0bfd6fbc68ef24cd8;
test SHAf13261de51726d5fb72bc606b920a45899395994ad491e7756bb24cb0580b92e.
Serial371terminal0/68.606s,1096assertions/16cases, actual-host handles terminal.
371changes ONLY CSolver: remove the obsolete unconditional coarse endpoint
preflight for fixed-normal joint normalization; existing local bracket expansion/
24assessments/budgets retained. Optional ADAP_BL_BUDGET outer-profile mode keeps its
original tangent policy. No numerical/geometry floor added. A needed extreme may
still fail representation. Actual target assessment/checkpoint still outstanding.

370actual saved-CFD metric-only TERMINAL1/65.933s, pins intact; all10owned actual-host
identities terminal. R/cases/curved370_saved_cfd_metric_stage_20261111T111101_1866897.
Error: D1 intersection sum not positive definite in first coarse trial. This is
NOT complexity overshoot rejection; no CFD advance or construction occurred.
Serial370terminal0/75.753s,1096assertions/16cases; MPI6terminal0/5.425s,
841assertions/16cases on EACH rank. All owned identities actual-host terminal;
pins intact. R/joint370_qualification.json owns limited qualification scope.
Build370 TERMINAL0/352.424s,58owned identities absent, all pins intact;
CFD SHAd0872a42bd3354bf3f49515db26dc097005fef921cf0ebecbee28b0d1d141a6f;
test SHAcb71dd694496b148b1fd1552463ee1a521cdde440b48183adc2a67f1b0c40d08.
Joint normalization passed focused serial and connected MPI checks; actual saved-CFD
metric stage failed at an unnecessary extreme endpoint. Not a selected reasonable
target or speedup.371passed serial; actual metric-only MPI6 is running. Reuse370MPI
primitive/publication evidence; no second fixture campaign for CSolver-only change.
369target-only actual diagnostic TERMINAL0/78.391s,
303453assertions/1case; all owned actual-host identities verified terminal and pins
intact in R/geometry369_assessment.json.42samples across5actual M6 contexts, no CFD
advance or construction. Local context costs/work are recorded in the timing ledger;
these exclude shared setup and do not establish a production speedup.
Build369 TERMINAL0/244.469s, all10observed build identities absent; CFD byte-identical368.
Test SHA95b87517866c53256deac8e41ad8e0d035c1723c1f737f98ba0bcdd2f9f6e088.

368focused/actual checks TERMINAL; all5actual
identities absent in EACH case and pins intact. Build368 TERMINAL0/68.130s;
all10observed build identities absent. CFD SHA6297c7f82411e15cd95ae36c6ad5f0529d7dac5c1bdbd9e94c91e5a5ed67283f;
test SHA57978caec389405d8966fc3e56179b50d968511b4e8a41e5d08b5ac6638e965f.
367 failed ONLY a new test's unsupported long-double CPassiveComm reduction;
exact failing source preserved.368 uses existing explicit MPI_LONG_DOUBLE call.
Production byte-identical367. No full M6 retry has been launched.

368serial PASSED420assertions/9cases/5.838s. Actual PASSED306789assertions/1case/
138.187s:5original contexts + saved358generated row and84-cell neighbor union.
R/tangential368_assessment.json seals LIMITED target diagnosis, not accepted BL.
25-cell row keeps strict geometry/height/positivity at factors1,.75,.5, but wall
q-bad4→5/J-bad5→5;20outer cells remain q/J-bad.84-cell union geometry/positivity
passes;34neighbor-only wall cells retain height debt (previously unlayered).
Coarsening alone is NOT selected. Analytic NextLayer control separately shows
metric-unit advancement does not prove physical progression with oblique/fine-
tangent demand. Actual progression/coverage/transition remain unqualified.

CURRENT CADENCE: ordered stage integration (latest user instruction), NOT immediate
full-workflow retry after each fix. Stage1 verified/reusable input is
R/m6_bl_inputs/rans_metric_entry364_v1 (provenance+qualification): exact first
metric-entry restart from364, byte-identical362, mesh/ref unchanged303. All9fields
finite, positive rho/internal energy, SA>=1e-16 with exact wall floor. SHA
 af7ade468dc40cd39dbe9238248e50057cdd151a64fef92fda957677aa3f6cae.
This includes the old first pre-remesh CFD step; stage2 comparisons must load it
WITHOUT another CFD advance. Original303 seed remains intact for full integration.
Stage2 target/normalization is ACTIVE: finish physical geometry/directional/height-
feasibility attribution, select a justified connected correction, then qualify a
complete composed-target disk checkpoint. Sensor-only files are insufficient.
Production CSolver selector now has the371joint flow/inherited-tangent candidate
for fixed-normal policies; focused serial is qualified, actual M6 target remains
unqualified. Frozen replay retains its selected scale. Crease geometry
is reused during radius refresh; feature/index transient storage is pre-admitted.
Nominal complexity-only refusals are diagnostic; physical/resource gates remain.
This is not an established reasonable target or speedup. No new operators/scheduler.

## Production outcome and provenance

Full364 TERMINAL1/1972.886s. Preserved first surface failure before verified owned
stop:391.619s,18commits/122work,51891cached of303310tets, no first row. All10actual
identities absent;1639source/build/input pins and mapped runtime unchanged. Stop
was for construction deadline, NOT complexity overshoot. Lossless rejected capture
in R/m6_runs/rans_curved364_v1/capture_archive; exact partial mesh61312used nodes/
303310tets. Shape/geometry/size census incomplete. No usable BL/infeasibility claim.

Metric1199.666s, same geometry work and provisional948069.856estimate as362;
empirical indicator77214.161, no proven bound; flow-only2.08846.500k is NOMINAL,
not a node count or strict gate. Work/timing attribution and confirmed efficiencies
are in the ledger; no metric work speedup is established. Raw CPU includes MPI polling.

Original303 inputs remain R/m6_bl_inputs/rans_curved_surface_ready_v1:
mesh SHA8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
reference SHA2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
original seed SHAc90000ea804c7e08a54dbec93f218b5bd3ae3dce3db8c287db6783fd138f1c78;
config SHA0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
The separate metric-entry checkpoint above uses AF7...field without repeating CFD.
None is a complete composed-target disk checkpoint. Same Mach-only L1 constraints.
Native5400s/outer14400s,12GiB per rank; nice19,j3,OMP/BLAS/MKL1,4TB compile-tmp.
Check actual host load/RAM and owned PID/start identities before heavy jobs/edits.

## Active stage2 and next bounded work

Full composed3D complexity ALREADY counts BL volume/outer continuation. Existing
normalization adjusts only flow while wall covariance preserves inherited triangle
spacing. The experimental shared API can vary inherited tangents before protected
normal/geometry/D1 composition; exact owner/wall/support/profile dependencies govern
same-point recomposition. Changed feature radius requires index/images/Option-A
refresh and memory admission. Scoped source and limited checks are preserved; this
is not yet a justified production selection or efficiency gain.

Read-only saved_row369_sensitivity.json independently matches saved q/J within
1.67e-16. Finer tangent-only CONTROL factor8 closes5wall q/J cells;20outer cells
remain bad. Aligning metric normal to flat face alone does not close wall quality.
This is not a justified target policy or a mesh change. No finer floor selected.

Next distinguish physical curvature/tolerance and first-layer-feasibility demands
from inherited tangent sizing and point-placement/connectivity obstruction, using
existing physical surface derivatives and saved358/368whole-patch evidence. Uniform
coarsening worsens these fixed generated cells; do not enable it solely for lower
complexity. Preserve constrained/blended directions. Continuous hn versus actual
NextLayer progression must be assessed separately, including oblique/outer behavior.
No new operator, larger cavity, scheduler, geometry representation or depth law is
selected. Queued Coulaud/MAdLib hypotheses remain inside this investigation.

After target reasonableness, establish fresh-process reconstruction of the SAME
composed target and geometry, reusing existing .native_ref, RecordStream/donor and
stage-capture formats first. Include selected core/original family as needed,
normal/tangent/feature/Option-A/outer policy and complete provenance; sensor-only
exports are insufficient. Stage3surface work waits for credible stage2output.
Run focused checks and meaningful MPI ownership/publication/integration milestones;
new profile MPI qualification is still pending. Native FIRST_ROW1 guard remains.
No full M6 baseline/perf/GDB campaign or full retry is currently authorized merely
because a local check passes; latest stage cadence controls.

## Preserved completion and strict remaining objective

Euler293 COMPLETE: TWO strict Euler50k meshes149051points/736323tets and172001/
870422, conservative admissible transfer, finite continued CFD and actual isolated
adapted disk restart. Continued CFD was one iteration, not convergence. Preserve
R/m6_inputs/euler_adapted293_event{1,2}_v1 and
R/m6_runs/euler_build293_adapted_event1_disk_restart_v1. All22payloads384685699B
rehash receipt euler293_preservation_during_bl358.json remains valid; no payload edits.

Remaining: usable strictly accepted curved BL, dedicated native-MPI constraint
qualification, then TWO accepted RANS adaptations, conservative admissible flow+SA
transfer, finite continued adapted CFD and actual isolated adapted disk restart.
Geometry1e-4,exact positivity/body separation,q>=.2,J>=.05,maxedge1.8;
h0=2e-6,g1.2,T=.01,height tolerance.1. CAD optional; original no-CAD authority
sufficient. Never weaken heights for complexity, raise budgets or freeze columns.
Maintain source freezing, resumable evidence, pushed checkpoints and the timing
ledger. Unrelated alternatives go in [backlog](Handbook/ADAPTATION_BACKLOG.md).

Detailed prior provenance/qualification: [pre369snapshot](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_369_20261111.md),
[physical-height evidence](Handbook/ADAPTATION_CURVED_BL_PHYSICAL_HEIGHT_20261111.md),
[construction strategy](Handbook/ADAPTATION_CURVED_BL_CONSTRUCTION_STRATEGY_20261110.md).
Historical RUNNING/next-action notes there are not live instructions.
