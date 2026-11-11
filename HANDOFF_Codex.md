# Native adaptation — current handoff

Updated2026-11-11 Europe/Rome. Goal ACTIVE. ONE connected curved-BL cycle;
one owned heavy workload. R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md and [HowToProceed.md](HowToProceed.md). Historical states are in
[pre359 history](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_359_RUN_20261111.md) and
[pre352 history](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_352_20261111.md), NOT live state.

## Current owned workload — authoritative

Current owned workload: full same303 workflow362 RUNNING/FROZEN, tool session6695.
Folder `R/m6_runs/rans_curved362_v1`; wrapper1251081/start42532701,
timeout1251565/start42533873, MPI1251566/start42533873; actual10PID/start identities
verified live in start_verified_actual.json,1623pins intact, runtime mapping recorded.
Started2026-11-11T07:08:19UTC. Initial same-seed CFD finite CL.155731/CD.050268;
first composed metric COMPLETE1254.119s, metric_build1253.731s(+2.74% versus359),
IDENTICAL per-rank geometry work and complexity948069.856. Metric-entry CFD restart
byte-identical359. No metric work reduction; this change targets surface planning.
Now in native surface preparation; no remesh outcome yet. See
R/metric362_first_stage_comparison.json and immutable initial_cfd_metric_entry.log.
Actual10identities last reverified live; metric_live_observation.json is historical
within this same still-live attempt, not its latest elapsed stage. Compiled C++/tests
and pinned helpers remain unchanged until verified terminal or preserved failure
justifies authorized stop. Qualified change pushed2e151415c8 before launch. Build362 TERMINAL0/70.163s, pins intact,9observed build
PIDs absent. Serial362 TERMINAL0/4.426s,741assertions/9cases, pins intact/all actual
identities absent. MPI362 TERMINAL0/369.066s,17cases on EVERYrank,4062–4217assertions/rank,
pins intact/all10actual identities absent. `R/qualification362.json` scopes affected
planning/seeding/height/curved/ownership/resource behavior only, NOT usable BL.
Attempts: `R/cases/curved362_height_callers_serial_20261111T065906_1229092` and
`R/cases/curved362_height_callers_mpi6_20261111T065929_1230143`.
362 is a fixture-only successor.360 serial
9/10cases passed incl strict normal bounds; new320-cell fixture hit default donor
cap.361 admitted donor count then exposed existing whole-metric patch128 limit.
Both failures/pins/actual-terminal receipts preserved; neither campaign passed.
362 uses supported80-cell planning star, keeps320 whole-patch refusal, and retains
existing large-star streamed engine tests in MPI. NO production limit raised.
361 production CFD byte-identical360; new build changes only fixture source.
C++/tests frozen during362 build/validation; source CFD360hash
37d822060736776a92d3cd0091b433245a9b1d94c6d8c9bda4a62f40a521eb22.
359 terminal receipts and source snapshots verified before edits.
360 connected change: fixed-patch planning guidance once per unique node via sorted
linear incidence, one sided sample list live, no cross-call cache. Additional scratch
is dynamically admitted for actual imported size, including imported large coordinate stars (streamed measurement where required); no new global cache/cavity bound. Existing incremental construction
seeding now recognizes nonempty surface_target with empty ordinary composition.
Improvement/full readiness/ownership guards remain. BL reports expose existing
seed/measurement/field counters. Normal unit assertions now use explicit absolute
bounds (Approx's default relative epsilon no longer masks their margin).
Affected serial checks and ONE connected MPI milestone passed; next immediately full
same303 workflow. No new diagnostic loader/config, new CFD seed or target change.
Passing checks will not establish usable BL. Read-only surface359_seeding_coverage.json
confirms17540/303359 measured(5.78%),285819 location/measurement UNKNOWN; zero q/J
measurements. Partial exports lack composed tensors/component contributions, so cannot
apportion complexity excess. Full359 failure remains active evidence:
391.593s surface seeding,0work/commits,geometry_work273738. All10actual handles absent,
pins/runtime intact; first-event report/log/core preserved under R/m6_runs/rans_curved359_v1.
Exact entry sensor core1e-4I at all61330points to2.304e-15relative, Float64 association;
composed estimate948069.856 versus500k remains provisional/no proven bound. No
accepted BL/RANS mesh, transfer or disk restart. See current physical-height topic.

Same saved303 reasonable coarse RANS+SA seed, Mach-only L1,500k metric complexity;
not500k points. Native5400s/outer14400s,12GiB per rank, CFD MPI6/native FIRST_ROW1.
The native1 guard remains: this run does NOT qualify native-MPI BL generation.
Geometry1e-4,q>=.2,J>=.05,maxedge1.8; h0=2e-6,g=1.2,T=.01,height tolerance.1.
Keep positivity, body separation and original no-CAD authority strict; CAD optional.

Inputs `R/m6_bl_inputs/rans_curved_surface_ready_v1`: SAME303 mesh/reference/seed,
only existing DOUBLE volume-output setting. Config SHA
0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
Seed SHAc90000ea804c7e08a54dbec93f218b5bd3ae3dce3db8c287db6783fd138f1c78;
mesh SHA8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
reference SHA2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de.
Canonical/prepared payloads rehashed unchanged in rans303_preservation_during_bl358.json.

## Pinned implementation and qualification

Build359 TERMINAL0/62.118s, actual build handles absent, pins intact.
CFD SHAc52027cd43e5c1d75283a76e459d5489253c6bbce08ed448d14d794a2a4b4e9b
(byte-identical358); test SHA53702def233891ee7b8a5c146cdb2dab2f8987010d2bae91a0919fd235bd2e04.
358 fixes shared smooth-curved BL metric frames: differential surface normal,
not query-minus-approximate-foot direction. Distance accuracy does not certify that
angle. Retained MobilityDimension distinguishes smooth seams from genuine features;
absent policy retains conservative ambiguous-support fallback. No per-query policy
build, advancement-direction replacement, sensor/norm/construction-policy/gate change.

Serial358:4387assertions/20cases. MPI359:29cases on EVERYrank,4321–4547assertions/rank,
362.286s, pins intact/all actual identities absent. qualification359.json scopes
LIMITED normal/feature/resource/caller behavior, NOT usable BL or full workflow.
359 changes only a fixture's constant-sensor witness to a local donor; all MPI checks
remain. Failed358 MPI/global-point-through-local-donor exception and exact verified
owned stop are preserved. Serial358 reused with explicit unchanged-production proof
and test-source snapshot; MPI359 supersedes its fixture-only sampling change.

Qualified346/347 physical-height/resource contracts and surface344 operators remain.
347 serial/MPI scope and evidence are linked from the physical-height topic below.

## Actual M6 result and active blocker

[Physical-height, normal and repair evidence](Handbook/ADAPTATION_CURVED_BL_PHYSICAL_HEIGHT_20261111.md).
Same5 saved wall-child points now have metric axes equal to differential normals
(to rounding), unchanged query work. Prior angular error.009–.413degrees was real;
rigid tensor rotation was sensitivity evidence, NOT an accepted target/replacement.

Actual358 patch TERMINAL1/356.018s/pins intact/actual handles absent:
SAME16splits and byte-identical5-column/25-cell candidate as356. Strict initial
geometry/height/contact PASS, q-bad24/J-bad25 (356:25/25), unknown0. Existing repair
improves loss but full strict height check rejects21.2111% error>10%; original
candidate retained. Native optimizer again exhausts the shared300s geometry window.
front358_assessment.json seals captures/counters. No admissible repair, usable BL
or speedup proven. Read-only shape decomposition is in front358_shape_decomposition.json:
4/5wall q/J-bad plus20/20above-row q/J-bad. Above-row cells still await growth/transition;
control-target evidence alone cannot diagnose the production target. Private interface
fixes5of6wallvertices; this is not a permanent production boundary freeze. Remaining construction/repair capability is the next target.
Provisional AdmitFirstHeight under validity-only construction is not final height
admission; later nonprovisional cell/completion gates remain strict. Keep all gates.

Do not infer infeasibility from a timeout, better debt or helper success. Current
quality has no permanent row/column registry freeze. Complete actual neighbor star
at blocked vertex39424 has64cells (59outside candidate), exported within existing bound.
No larger cavity/operator/scheduler/config selected. Keep coupled surface/volume
operators and constrained/blended ComputeDirectionField/DirectionAt directions.

## Preserved completion and remaining obligations

Euler293 COMPLETE: TWO strictly accepted Euler50k meshes149051points/736323tets and
172001points/870422tets; conservative admissible transfers, finite continued CFD,
actual isolated adapted disk restart. Continued CFD was one iteration, not convergence.
All22 payloads/384685699bytes rehashed unchanged358 in euler293_preservation_during_bl358.json.
Preserve checkpoints `R/m6_inputs/euler_adapted293_event{1,2}_v1` and restart baseline
`R/m6_runs/euler_build293_adapted_event1_disk_restart_v1`. No new Euler cycle selected.

[Construction strategy](Handbook/ADAPTATION_CURVED_BL_CONSTRUCTION_STRATEGY_20261110.md):
measured-ready tangential surface, constrained/blended directions, coordinated first
row, successive fronts, outer transition and complete strict closure. Progression,
coverage, transition and true native-MPI BL construction remain UNPROVEN. Preserve
queued Coulaud/MAdLib audit of physical curvature/tolerance demand, separate target
contributions and realized leading-edge conformity. Inherited tangential contribution
to historical1.174528M/current948070 versus500k remains unquantified; do not call it infeasible,
raise complexity or weaken heights. Exact330 core was not saved; no baseline campaign.

Use frozen359 outcome to select the next connected correction. Completion remains
TWO strictly accepted RANS500k adaptations, conservative admissible flow+SA transfer,
finite continued adapted CFD and actual isolated adapted disk restart, plus dedicated
BL/native-MPI constraint qualification. Passing fixtures/private patches are not
completion. Keep resource receipts, source freezing, resumable evidence and pushed
checkpoints. Unrelated ideas stay in [backlog](Handbook/ADAPTATION_BACKLOG.md).
