# Native adaptation — current handoff

Updated2026-11-09 Europe/Rome. Goal ACTIVE. TWO Euler50k meshes are strictly
accepted, conservatively transferred, persisted and followed by finite adapted
CFD in293. Isolated actual adapted disk restart PASSED. Euler functional milestone is verified.
Next prioritize the requested connected boundary-query performance cycle; dedicated
BL/constraints/MPI qualification and corresponding RANS500k remain outstanding.
No convergence/experimental shock-validation claim: continued CFD was one iteration
per adapted cycle. Actual mesh point/cell counts differ from complexity50k.

Geometry1e-4,q>=.2,J>=.05,maxedge1.8,requested heights strict. Validity-first
construction/current nodal metric transport/CAD optional unchanged. One active
cycle/one ownedheavyjob, nice19/libs1/buildj3/4TBscratch. Freeze compiled/executing/
pinned sources, helpers, binaries, runtime and inputs during validation. Actual
/proc/ps MUST require_escalated; sandbox absence is not authoritative. Verify exact
PID/start identities before restart or edits. Fork pushes authorized, exact-whitelist
staging; preserve unrelated dirty logs/.orig/untracked work. No new scheduler/config
options/larger cavities, quality-aware construction, raised budgets or relaxed gates.

R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
A293=`R/m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k`.
D293=`R/m6_runs/euler_build293_adapted_event1_disk_restart_v1`.

## CURRENT — qualified boundary reuse; FIRST-adaptation296 frozen/live

ONLY ownedheavyjob: tool57374, wrapper3191866/start27444699, runner3192139,
process3192385. A296=`R/m6_runs/euler_build296_boundary_reuse_mach_only_real_cfd50k`.
A296/owned_startup.json seals ALL11actualHOST PID/start identities, six executing
inodes and84mappedELF/rank. Same selected-failure wrapper as293 invokes production
CSinglezoneDriver on actual M6; not a toy fixture. Executing test_driver SHA
6f8184dc7359a62ce6d9d82a2f96055ac6caebf2b5e7354c5f9800b4cedd403c;
standalone production294 CFD SHA
b316e4f9fd3c76b43c10c8c4ffb85f7040da0ab93cb2850182bec1c31dbab256.
Build295/296 changed tests only, preserving CFD294 bytes.

Source/build/runtime/inputs/helpers FROZEN. No CPP/helper edits or competing job.
Actual296cfg differs from actual293 ONLY ADAP_SUBITER2→1; native5400/outer14400,
Mach-only L1/50k, original saved seed and all gates/constraints unchanged. Initial
61330coordinates/all22CFD+metric fields BIT-IDENTICAL qualified254/256, independently
verified by verify_m6296_initial.py. Startup in native reference/import; no complete
six-rank stage census yet. Current first-stage evidence: analyze_m6296_stages.py;
quality comparison only after initial quality_entry/window via analyze_m6296_quality.py.
Do NOTuse historical293/291monitor/stop helpers or infer terminal from stale logs.

boundary296_qualification.json PASSED: focused27cases/5877assertions, connected
serial99cases/68536assertions and ONE MPI6 milestone132cases EVERYrank. All stage
pins unchanged and actual handles terminal. Production294 built successfully;
295 exposed a shape-clean test fixture after early-return correction;296 fixes
only that fixture. All insufficient/failed295 evidence and exactsource preserved.
No performance benefit or new accepted mesh established yet. Connected candidate:
immutable old-star validation/maps reused lazily, fresh raw projection/parameters
per trial, exact unchanged entity-face numeric guidance only where such faces
exist, and dependency-specific pure-shape rejected-star invalidation. Retained
storage admitted; bulk replacement still clears history. Prepared-leaf direct
execution remains deferred pending exact-oracle/admission proof, not a new cycle.
[Current design/evidence](Handbook/ADAPTATION_BOUNDARY_QUERY_PERFORMANCE_20261109.md).

Completed293 frozen fullworkflow tool90456 TERMINAL0/6615.087s, all1520pins unchanged,
all11actualHOST identities ABSENT; independent two-event validation and both matching
checkpoints preserved. Actual isolated adapted disk restart tool65802 TERMINAL0/
181.356s, all1527pins unchanged/all10handles ABSENT; standalone MPI6 CFD, bit-exact
mesh coordinates, finite/admissible changed state, input bytes preserved. Independent
A293/euler293_completion.json and D293/disk_restart_validation.json PASS. Euler
milestone verified; full goal still ACTIVE. This steady CFD disk restart does NOT
qualify resumed adaptation-loop numbering/overwrite behavior.

After complete first296 replay, compare net whole-field q/J removal, geometry,
work, CPU/busy/join and total accepted firstAdapt versus2933819.85s. Keep overlapping
timers separate; join-call time is NOTtotalMPIwait. Return to preserved293event1
adapted CFD checkpoint for logical event2 qualification as needed, then dedicated
BL/constraints/MPI and RANS500k. Never overwrite either preserved293checkpoint.

## Completed Euler two-event evidence

Both mandatory final whole-surface two-sided geometry certifications PASSED; all
q/J/geometry/size measurements known and ZERO at each nativeexit. Allsix ranks
complete/stop=complete each event. Native budgets5400/event, outer14400, native
12GiB/rank and hostcache4GiB unchanged.

| Event | Points | Tets | Native seconds | Continued CL/CD |
|---|---:|---:|---:|---|
|1|149051|736323|3640.719193877|.269781/.014756|
|2|172001|870422|2314.855203748|.269952/.015095|

Conservative P1 transfer completed each event, zero inadmissible projected states.
TrueCVmass residuals event1 rho/Mx/My/Mz/E:
7.722e-14/2.649e-14/5.244e-14/3.140e-14/7.627e-14;
event2:6.464e-14/7.261e-14/4.007e-14/8.970e-14/6.340e-14.
Successful pinned production path enforces scaledTARGETintegral defect<=1e-12 before
publication. Numerical final target defects are NOTprinted; old/new domain changes
are NOTthat measurement. Each matching restart has bit-identical mesh coordinates,
all22fields finite, positive density/internalenergy. Both independently COPIED to
R/m6_inputs/euler_adapted293_event{1,2}_v1; checkpoint.json manifests and hashes retained.
Large actual mesh/reference/restart files are local; metadata is pushed. Never replace
these with original-grid states or overwrite them during performance development.
Restart metrics are following-CFD NEXT targets, not construction-transport exports.

A293/workflow293_independent_validation.json seals both actual saved-data checks.
Actual candidate-complexity estimates:51431.85308138/event1,50610.29410286/event2;
uncertainty indicators138.766/140.210,NOTprovenbounds, current nodal transport and
normalization correction0. Requested target50k unchanged; no target-drift correction.
Seed R/m6_inputs/euler_original_force_seed_v1 is original61330points/303359tets,
Mach.8395/AOA3.06,CL.269535/CD.013536. Initial293positions/all22fields bit-identical
qualified254/256, sensorMACH only,L1/50k.

## Connected performance cycle — implementation frozen, qualification pending

[Design/evidence](Handbook/ADAPTATION_BOUNDARY_QUERY_PERFORMANCE_20261109.md).
Frozen workflow had already entered event2 when first checkpoint was verified;
its outcome and actual isolated restart are now preserved. Next ONE connected
boundary-query optimization, appropriate
focused checks +ONE MPI milestone, immediately complete FIRST-adaptation replay
from SAME saved original seed/Mach-onlyL1/50k/constraints/budgets. Do not combine
with sensor/norm/construction-policy changes. Return to preserved adaptedCFD for
logical event2 if current implementation needs renewed qualification; do not discard
its progress. Full goal remains Euler plus dedicated BL/MPI and RANS500k.

First293vs291 all164matching counts/flags/comparedwork EXACT, including acceptedexit.
Native3640.719 versus3535.671s; host contention observed, no causal speedup claim.
A293/first_event_performance_counters.json seals actual first-event prefixes:
boundary292056starsearches/8reusehits/395093candidates/17177moves,
2174.545aggregate rank-seconds versus interior65.269. Scope364250misses/builds,
1080.444rank-seconds; traversal4896.194rank-seconds. Timers overlap, NOTadditive
or critical-path shares. RankCPU snapshots and existing busy/join/churn counters
are available; join-call time is NOTtotalMPIwait. Judge net whole-field q/J removal/s,
geometry-bad and accepted first-event time, not commits/debt alone.

Earlier static opportunities reviewed: old-star topology/parameter reconstruction in
MoveVertex/SmoothBoundary; copied scoped unions despite prepared leaves; Commit
clears rejected_smoothing. Existing rejection cache excludes non-shape/resource
failures;8hits alone does NOTmeasure eligible insertions or prove invalidation is
its cause. HasAcceptedFaceCertificate verifies association/coverage, NOTnumeric
distance. Preserve exact dependencies, projection semantics (not idempotence),
coordinates/associations/support/tolerance/authority, fresh changed-position metrics,
active refs/transient admission, seams/features/bodies/heights and strictfinalproof.
Boundary reuse candidate is qualified as above; secondary measurement/clearance work only
if counters demonstrate it. Unrelated ideas stay in backlog.

## Qualified source and historical records to retain

Production292/test-only293 CFD SHA
89aa555d0a6a13679cc62f989fc30c5b4acec0c773bc3d972fe4cadf5629e75c.
R/build293_source_pins.json +reference293_qualification.json seal source/build/tests.
Bounded IO propagated EXISTING native budget through all3outputpaths, includes
retained references/readers/policy/continuation and serialization/self-decode;
temporary decoder querycache0. Public2GiBdefault/native12GiB unchanged. Atomic
sidecar publication/canonical validation intact. Actual49.68/49.44MBsidecars now saved.
[Persistence design](Handbook/ADAPTATION_REFERENCE_PERSISTENCE_20261109.md).
Failed292expected-exception CHECK harness artifact remains preserved; corrected293
passed29focused/97serial/130cases EVERY MPI6rank plus large-M6authority and expected
negative no-partial-write control. Do not promote failed292 ordinarycase into pass.

Retain qualified287exact finite-geometry proof,289prepared borders,290/291fresh
per-current-face reverse bound plus mandatory fullglobal forward proof/Bothfallback.
Never substitute acceptedassociation/localG0 for distanceproof.
[Certificate records](Handbook/ADAPTATION_PREPARED_SOURCE_CERTIFICATES_20261108.md).
291native accepted/transferred then failed default2GiBreference self-decode; actual
291SU2 preserved but no matching adapted restart. It is not a complete event.

[Cleanup](Handbook/ADAPTATION_POST_REFINEMENT_CLEANUP_20261108.md): originalclean-star
selection/zero-debt gap and four local replacement candidates established, NOTcomplete
cause/global leading-edge benefit. Conditional273cleanup never ran in first293;
no widening while current cycle active. Missing shock-target allocation remains
SEPARATE (sampled requested4–6%chord vs3–4%existing). No blanket minimum-edge gate.
[Backlog](Handbook/ADAPTATION_BACKLOG.md), [next work](HowToProceed.md),
[pre-terminal handoff](Handbook/History/HANDOFF_before293_workflow_terminal_20261109.md).
Resume-loop cycle renumber/input-overwrite and rejected-event consumption remainOPEN;
this isolated steady disk restart is separate. No full-goal completion claim.
