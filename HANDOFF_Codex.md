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

No owned heavy workload. Actual375 TERMINAL1/361.336s, pins intact; all4children+
wrapper verified absent on actual host. R/geometry_target375_assessment.json.
Breakpoint preparation now COMPLETES in242.075s;1925composed integrand evaluations
then consume50.101s before the same300s atlas deadline.374could not finish
preparation in293.289s and performed0integrand evaluations. Context work4268956
versus4700941, but new work includes composition: different progress, not identical
fixed work. Final chord/usable target still unavailable. Local memo hit fraction
not exported on this successful preparation path; no full-adaptation speedup claim.
Build375 passed42.162s; focused serial424assertions/4cases/178.614s, pins intact.
Only production CBoundaryLayerMetric.cpp changes: per-wall/chord exact point memo,
sufficient successful accuracy only, bounded scratch from existing envelope,
work/deadline checks and no cross-call lifetime.

Next connected assessment: remaining point certification during composition.
Existing OptionA::Weight→WallDistance and subsequent Compose(base) both evaluate
the exact same point/support; if w>0, Compose(full) evaluates again. Determine
safe use of existing immutable geometry cache/pool for complete closest-distance
certificates across these callers, including exact support/hint, allowed-facet
callbacks, sufficient outward accuracy, active references, admission and exhaustion.
Prefer one shared facility over keeping competing per-chord/global memos. Do not
edit merely to add a cache-hit counter or repeat a baseline. No implementation
of that extension yet. Keep stage2target diagnosis/checkpoint obligation and budgets.

374single saved-target chord diagnostic TERMINAL1/
358.197s, pins intact; all4children+wrapper rechecked absent on actual host.
R/geometry_target374_assessment.json preserves exact replay/counters. Build374
TERMINAL0/202.232s; focused serial374424assertions/4cases/257.590s passed.
Measured10627point projections/292.589s,3687intervals; region preparation13.059s.
Active width7.1e-15, point error3.6e-18, unresolved1.25e-8>1e-8; no edge integrand.
Next connected375hypothesis: local per-wall/chord exact-point projection certificate
reuse only when its successful accuracy meets the request; same authority/support,
bounded/admitted memory, budget checks and recomputation fallback. No cross-call
cache, target change or gate/budget relaxation. Valid hit fraction not measured yet.

373actual saved-target diagnostic TERMINAL1/355.999s, pins intact, all5owned
actual-host identities terminal. R/geometry_target373_assessment.json. FIRST
20-cell context only:7associated wall-point contributions and20centroid queries
complete, then293.942566s in edge_geometry,0edge integrand compositions,1edge failure,
contextgeometrywork4836754. Existing300s atlas deadline exhausted. Normal metric
density≈500000 agrees with h0; physical placement/progression remain unverified.
Other4contexts and the actual chord length remain unmeasured. Source identifies
curved EdgeBreakpoints preparation, not Mach recovery/integrand evaluation. It does
not distinguish repeated preparation from conservative refinement yet.

374closes the timeout replay gap: current interval/point/error, projection counts/
cost and existing geometry counters; journals completed centroids before the chord.
First run focused [NativeCubicChordSpan3D],[NativeBLTangentialNormalization3D] via
R/check_chord_serial374.py, then exact single failed context using
R/check_chord_capture374.py. R/m6_bl_inputs/target_chord373_v1 preserves its20cells,
endpoints65/18865 and exact fresh-policy recipe from371selected core. No new global
metric/CFD/baseline/perf/GDB run. No optimization selected before those counters.

Build373 TERMINAL0/198.222s,10owned handles terminal; CFD byte-identical371,
test SHAedfcb725e02b092de88e98d0e137be297793ca8795e79b76700db85d38a1c591.
372failed only the new test's Tensor/ApplyPoint type mismatch (terminal1/18.146s,
all5owned handles absent; source_snapshots372 preserves it);373uses Composition().


371actual saved-CFD METRIC-ONLY stage TERMINAL0/
1739.083s,429364assertions total across6ranks. All10owned actual-host identities
terminal;1601source/build/input pins intact. R/metric_stage371_assessment.json and
R/cases/curved371_saved_cfd_metric_stage_20261111T111921_1888531 own the evidence.
No CFD advance or construction. Runtime Mach error4.44e-16; independent saved-field
reconstruction6.66e-16. Conservative flow variables unchanged at every owned node.

Provisional composed target504040.450±48545.704empirical indicator (not a bound),
nominal500k; tangent scale.28717458874925855, core.0047263575870155755. Original
h0/g/T unchanged. Core nodal complexity140142.106 versus364's2.08846;50.42%ofthat
CORE-only diagnostic lies beyondradius20. This is not an additive decomposition
of the composed target and does not establish inappropriate allocation by itself.
Core gradation reached256sweep limit but has0directed nodal edges over1+1e-5;
composed audit has133745failures (364:293237), maximumratio2.71976e6 (364:796263).
No continuous-field/geometry/BL-suitability acceptance is claimed.

Source mesh61330nodes/303359tets is unchanged; NO new BL mesh exists.
Metric1732.127s includes57.672authority setup; build1673.930s; geometrywork71299713.
No speedup:3distinct field evaluations versus1in364, target changed. Timings ledger
separates nested/overlapping costs, CPU and unknown wait attribution.

Exact core export (existing NativeFrozenVolume3D layout) and42source-coordinate
flow-direction diagnostics: current case/{selected_core_native.bin,
core_allocation_assessment.json,saved_cfd_metric_rank*.jsonl}. These are CORE-only,
NOT a complete composed-target checkpoint or original-family persistence.

Next target diagnosis is limited by the373chord-preparation failure above. Reuse
the exact saved tensors for all further full/flow/BL/geometry/height checks.
Farfield localization in R/farfield371_allocation_report.json shows distributed
core demand and large outer volume: top32nodes account for12%ofr>20core demand,
none directly boundary-incident; this does not prove a corner/recovery bug or
justify changing sensors/norms/gradation. Target reasonableness remains unselected.
Do not repeat CFD or launch surface generation merely because total≈500k. Complete
composed-target reconstruction/reload remains required before stage3.

Build371 TERMINAL0/58.132s,14owned identities terminal, pins intact;
CFD SHA7cf80707d006eb2c5d80afa1143b730e8502e1458e1f9fe0bfd6fbc68ef24cd8;
test SHAf13261de51726d5fb72bc606b920a45899395994ad491e7756bb24cb0580b92e.
Serial371passed1096assertions/16cases/68.606s.370MPI primitive/publication milestone
passed841assertions/16cases EACH rank;371actual target solve also passedMPI6.
Dedicated native-MPI BL generation remains unqualified (FIRST_ROW1 guard remains).
370failed the unnecessary extreme coarse preflight (SPD);371uses existing local
bracket expansion/24assessment cap. Optional outer-normal ADAP_BL_BUDGET mode keeps
its original tangent policy. No numeric/geometry floor or relaxed gate was added.
An actually needed extreme trial may still expose representation limits.

Prior368–370 patch/API/diagnostic evidence is preserved in the linked normalization
handbook and R/{tangential368_assessment,geometry369_assessment,
joint370_qualification,metric_stage370_failure,joint371_qualification}.json.
Neither uniform coarsening nor finer-tangent controls established admissible actual
BL replacements. Physical discrete progression/coverage/outer transition stay open.

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
for fixed-normal policies; focused serial and actual M6 MPI target evaluation pass,
while target reasonableness remains under assessment. Frozen replay retains its selected scale. Crease geometry
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

Full composed3D complexity ALREADY counts BL volume/outer continuation. Before370,
normalization adjusted only flow while wall covariance preserves inherited triangle
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

371source audit confirms monotone support/broad-phase history is missing
from config+scale+final-core reconstruction; see stage cadence checkpoint audit.
The current JSON remains diagnostic, not a complete checkpoint. No source edits
were made during the run. Source trace confirms SA restart loading refreshes flow
primitives before Mach sampling; terminal runtime comparison is still required.
Timing ledger now identifies the untimed complete nodal composition audit; its
share of metric cost is NOT measured and no optimization is selected from it.

After target reasonableness, establish fresh-process reconstruction of the SAME
composed target and geometry, reusing existing .native_ref, RecordStream/donor and
stage-capture formats first. Include selected core/original family as needed,
normal/tangent/feature/Option-A/outer policy and complete provenance; sensor-only
exports are insufficient. Stage3surface work waits for credible stage2output.
Run focused checks and meaningful MPI ownership/publication/integration milestones;
profile primitives and actual target solve pass MPI; generated BL qualification
is still pending. Native FIRST_ROW1 guard remains.
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
