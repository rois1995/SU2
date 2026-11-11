# Native adaptation — current handoff

Updated2026-11-11 Europe/Rome. Goal ACTIVE. ONE connected curved-BL cycle and
one owned heavy workload. R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Read AGENTS.md and [ordered stage cadence](Handbook/ADAPTATION_CURVED_BL_STAGE_INTEGRATION_20261111.md),
[current accuracy/exclusion design](Handbook/ADAPTATION_BL_EDGE_EXCLUSION_20261111.md),
[normalization evidence](Handbook/ADAPTATION_CURVED_BL_NORMALIZATION_20261111.md),
[timing/work ledger](Handbook/ADAPTATION_TIMINGS.md) and [HowToProceed](HowToProceed.md).
Detailed prior state, source/build/input receipts and failures are preserved in
[pre384history](Handbook/ADAPTATION_HANDOFF_HISTORY_PRE_384_20261111.md); historical
RUNNING/next-action notes there are not live instructions.

## Current source and owned workload — authoritative

Original saved-target chord385 RUNNING/FROZEN, session15094;
R/cases/curved385_target_geometry_20261111T155709_2553462, child2553546.
Do NOT edit pinned C++until actual-host terminal verification.
Serial385passed4977assertions/50cases/63.635s; MPI6passed29cases EVERYrank
(3925–4128assertions/rank)/12.602s; owned identities terminal/pins intact.
Same12points385passed12.894s/3180work (383:2870work). Matched coarse/tight normals
now differ5.55e-17–1.67e-16 (383:up to4.62e-6), feet≤1.99e-18;10successful proposal
improvements/310extra work. This is accuracy progress, NOT fresh-query speedup.
R/edge_accuracy385_point_assessment.json. Initial roots are vertices and skip polish
for these points; only final polish runs. Assess full-chord result before any change.
Build385passed28.110s, pins intact/owned handles terminal. CFD
SHA3a65b4b43d0f844365a4ffe669a6da7cd790b2223e572c6216346e0b53394f9b.
384failed466.488s(private test API), terminal/all155identities absent; exact failure
preserved3d1866448b.385only fixes test with independent Bernstein evaluation.

Implementation384hypothesis: reuse existing normalized MinimizeTriangle to improve
upper witnesses before and after the complete distance traversal. At most64charged
callbacks per proposal inside the original total budget; nonconvergence/invalid
proposal falls back. Canonical binding +outward ParameterBox/Blossom certifies every
new witness. Full allowed-component lower search and final clearance remain; cache
only complete final results.4096Badditional scratch admitted, no configured budget
increase. New attempt/improvement/work counters use existing geometry stats.
Optional pre-search work CAN reduce remaining traversal budget; actual benefit and
availability must be measured, not assumed. No numerical tolerance, gate, target
policy, normal law, new config/scheduler or CAD dependency changed.

NEXT: verify build385terminal actual-host identities/pins; focused
R/check_witness_serial385.py → same12points R/replay_accuracy_points385.py → ONE
appropriate connected MPI R/check_witness_mpi385.py → original saved full chord
R/check_witness_actual385.py. Preserve every terminal/failure receipt. A stable normal
or lower query count alone is insufficient: require useful full-chord progress under
unchanged bounds. Production384source checkpoint3d1866448bis unqualified; test fix local. Latest
pushed6b372129f1owns pre-change point evidence/design. Production383checkpointb597d15fd8 remains preserved.

## Demonstrated blocker and prior qualification

Actual383point replay TERMINAL0/12.861swrapper (7.615sprobe),248assertions/12points,
2870work; source/input/binary pins intact, all actual-host handles terminal.
R/edge_accuracy383_point_assessment.json. At identical q,2e-12→2e-14distance request
changes foot3.8065e-8, normal2.7683e-6 and normal-only chord speed4.8824e-5relative.
Nearby coarse queries share EXACTLY one dyadic candidate, then jump. Exact repeats
cost1work. This demonstrates candidate-normal discretization; a distance-width
certificate alone does not certify normal accuracy. The original tangent-policy
probe is geometry attribution, NOT full selected371target acceptance. Tight answers
are not exact normal certificates. Source proposes only vertices/dyadic points;
existing bounded minimizer is now selected from this evidence, not merely literature.

382diagnostic TERMINAL1/359.211s, original300s geometry deadline. Preparation
186.895s/296cuts; GL10whole estimate39.19917in.298s/3634work. Widest retained interval's
numerical reference refuses after7384evaluations/106.016s. Actual GL10errors UNKNOWN.
Scalar diagnostic reparameterization adds rounding and does not reproduce the exact
production trajectory. Analytic normal-profile controls have up to1.7%error and
near-threshold decision differences. No fixed-rule replacement/tolerance relaxation
or conservative action enclosure was established. R/edge_accuracy382_assessment.json.

383complete outside-edge capability QUALIFIED: build434.447s; serial2664assertions/
23cases/4.648s; MPI6passed25cases EVERYrank (2826–3029assertions/rank)/5.523s;
actual5contexts78.990s. All owned actual-host identities terminal/pins intact.
R/edge_exclusion383_qualification.json. All5translated outside controls have exact
target/background analytic lengths and0composed edge samples. All5ORIGINAL M6edges
remain ambiguous/full path; no hard-edge/workflow speedup. Exact per-edge sensor
identity has dependency checks; stale contexts refuse. Existing analytic P1integral
and inverse reused. CFD SHAa0c14c47d7a5f65578da0781748ca1b2beab2aaad09f7e38a45f046603bbf654.

380shared complete closest certificates/unchanged prepared regions are preserved.
Preparation191.679s,4534integrand evaluations,100.308scomposition before the unchanged
300s atlas deadline; no complete chord. Timers overlap; see ledger. The earlier broad
MPI380campaign is NOT wholly green:84of85cases pass on every rank; unchanged planar
automatic-repair fixed-count assertion expects3cells but gets6. Modified geometry/
cache/constraint checks pass. Preserve/backlog before final broad integration; do
not weaken it or start a competing repair cycle. Native MPI BLgeneration remains
unqualified: FIRST_ROW1production guard still exists.

## Stage inputs and credible-output requirements

Stage1reusable exact metric-entry seed: R/m6_bl_inputs/rans_metric_entry364_v1.
Load WITHOUT another CFD advance. All9flow+SA fields finite/admissible, positive rho/
internal energy, SA>=1e-16exact wall floor. Coarse CL.15573/CD.05027reasonable,
not convergence/shock/wall-resolution proof. Original303seed/checkpoints untouched.
Hashes: mesh8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b;
reference2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de;
entry fieldaf7ade468dc40cd39dbe9238248e50057cdd151a64fef92fda957677aa3f6cae;
config0a7de022729f69b170deaee2eaabbd5347550da8134cbdaa39587f7173e78742.
Original303seedc90000ea804c7e08a54dbec93f218b5bd3ae3dce3db8c287db6783fd138f1c78.

Stage2ACTIVE:371saved-CFD MPI6metric-only passed1739.083s/429364assertions, no CFD
advance or construction; exact source61330nodes/303359tets unchanged. Same Mach-only
L1; nominal500k, provisional composed504040.450±48545.704empirical indicator, NOT a
bound. Tangent scale.28717458874925855/core.0047263575870155755; h0/g/T unchanged.
Core-only complexity140142.106;50.42%ofCORE demand beyondr20 is not a composed
additive decomposition or proof of recovery error. Core directed gradation0bad at
tolerance after256cap; composed nodal audit133745bad/worst2.71976e6. Continuous
combined-field gradation and physical target reasonableness remain unproven.

Exact371core export at R/cases/curved371_saved_cfd_metric_stage_20261111T111921_1888531/
selected_core_native.bin, SHA25f03abdbe5e3406af6725e093bedd94c8b1d05cd32586d35d74829e1f1b9cae.
This is CORE-only, NOT a complete composed-target checkpoint. Fresh diagnostic recipe
SelectTangentialScale→Update(final core) omits original family and monotone support/
bounds history. Need credible physical/directional/height/gradation assessment, then
complete disk reconstruction and fresh-process agreement before stage3surface work.
Reuse native_ref/record/donor persistence first; no complete codec implemented yet.

Full364ended before first row: surface391.619s/18commits/122work and shared deadline;
partial capture61312used nodes/303310tets, incomplete strict census. No usable curved
M6 BL exists. Splitting/coarsening/relocation/reconnection/growth, constrained/blended
directions, coupled cavities and heights must still be qualified on actual output.
Surface polish or a target solve alone is not BLsuitability. Coulaud/MAdLib hypotheses
remain queued inside this cycle; unrelated ideas stay in backlog.

## Resources, preserved completion and remaining objective

One owned heavy job; nice19,j3(2–4build workers), OMP/BLAS/MKL1.4TB compile-tmp
integration_evidence/build-integrated-v2/compile-tmp. Native5400s/outer14400s,
12GiB/rank; actual chord diagnostics300s atlas/600souter, existing point probe600s
outer. Verify PID+start identities on actual host (namespace may hide processes)
before edits/restarts; preserve source/binary/input/helper/runtime pins and failures.
No raised limits/relaxed gates or full retry merely because a focused stage passes.

Euler293 COMPLETE: TWO strict Euler50k meshes149051points/736323tets and172001/
870422; conservative admissible transfer, finite continued CFD and isolated adapted
disk restart. Continued CFD was one iteration, not convergence. Preserve
R/m6_inputs/euler_adapted293_event{1,2}_v1 and
R/m6_runs/euler_build293_adapted_event1_disk_restart_v1. All22payloads/384685699B
preservation receipt euler293_preservation_during_bl358.json; no payload edits.

Remaining: strictly accepted usable curved BL, dedicated native-MPI constraint
qualification, TWO accepted RANSadaptations, conservative admissible flow+SA transfer,
finite continued adapted CFD and actual isolated adapted disk restart.500k is NOMINAL
complexity, not exact nodes or rejection gate. Geometry1e-4, exact positivity/body
separation,q>=.2,J>=.05,maxedge1.8,h0=2e-6,g1.2,T=.01,height tolerance.1. CADoptional,
original no-CAD authority sufficient. Validity-first provisional stages may have
diagnosed quality debt; strict closure is mandatory for final acceptance.
