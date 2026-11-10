# Historical handoff snapshot —319 launch

Not live state. Read [current handoff](../../HANDOFF_Codex.md) first.

# Native adaptation — current handoff

Updated 2026-11-10 Europe/Rome. Goal ACTIVE. Euler50k two-event adaptation,
conservative admissible transfer, finite adapted CFD and isolated adapted disk
restart are verified in build293. BL generation/constraint preservation under MPI
and the complete RANS500k workflow remain unproven. Keep this as ONE connected
curved-BL implementation/validation cycle.

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## Frozen owned workload — authoritative current state

Full SAMEseed RANS/BL319 ACTIVE(session70391), pinned C++/helpers/inputs FROZEN.
R/m6_runs/rans_curved319_v1. Wrapper1427833/start33248148;
time1428575/timeout1428576/mpirun1428577(start33250039), ranks1428580..1428585
(start33250042). Current owned_handles.json is authoritative; verify actual
terminal identities before edits/restart/another heavy workload. Initial CFD finite
CL.155731/CD.050268; Hessian/D1 prefix matches314. Target construction active;
no normalized target or BL mesh/event accepted yet. CFDMPI6/FIRST_ROWnative1,
Mach-onlyL1/500k, h02e-6/g1.2/T.01, geometry1e-4, native5400/outer14400total,
12GiB/rank unchanged. Host query-cache quota4GiB/rankquota715827882B; initial
sampled process tree~3.2GB with ample headroom. launch_observation.json verifies
live source/input/runtime pins and all10 exact owned identities. Keep source frozen.

Build319 TERMINAL0/368.360s, all152 sampled actual build handles absent; pins verified.
Focused319 PASSED231assertions/6.347s, all5 actual handles absent, pins verified.
Actual captured chord314 replay319 PASS60assertions/32cuts: certification26.624s
versus31637.560s (-29.1%); work432259 versus409726 (+22533 cache insertions).
384255immutable-region hits/22533misses, no retention refusal,36593592cache bytes
within existing3204054800allowance. All5 actual handles absent/pins/runtime verified.
This is measured query benefit, not full target completion. Earlier chord311
replay319 PASS37assertions/9cuts: certification7.449s vs31610.443s (-28.7%);
work124069 vs113482,102209region hits/10587misses,17193288cache bytes/no refusal.
All5 actual handles absent; pins/runtime verified. Complete first-cell319 replay
TERMINAL124/1803.413s at unchanged1800s diagnostic cap, all5 actual identities absent;
pins/runtime verified. No completed integral or intrinsic guard failure. Last1740s
window73completedchords/13734density evaluations/29.300Mchargedwork. CPU1798.88+.77s;
maximum sampled process-tree RSS1681326080B, plateaued near1.68GB.
Affected serial319 PASSED52cases/286104assertions/326.610s, all5 actual handles
absent/pins verified. ONE MPI6 milestone319 PASSED55cases on EVERYrank/164.704s, all10 actual
identities absent/pins verified. qualification319.json seals BEHAVIOR qualification;
actual_cell remains passed:false/diagnostic124. Next full SAMEseed RANS/BL319
using already authorized native5400/total14400, all constraints unchanged.
Full319 is now active/frozen as recorded above. CFD SHA
`c40a2b865d1be8ede594a52e8935d96047324325fdd49a9d165746bcf81550b6`;
test SHA `77d14ce513410ad6cbf43ed33b943b6ab93369cb460ced53b3bf36fd05ff9642`. Cell318 phase observation TERMINAL124/300s as planned,
all5 actual identities absent; source/input/runtime pins verified. Build318
TERMINAL0/246.227s, pins verified/all52 sampled actual build handles absent.
CFD SHA `b0bc8926898f4ac50c66bf6ecaeeb2e77b5b433905830f08f69b88811cca08c0`;
test SHA `3803931ee74f1319d8feec98de9a326520000b2cf5a42ef8f7627d1bacba63b8`.
Focused318 PASS396assertions/4.353s: exact complete-rule equality with/without
optional diagnostics. Both sampled helper/child identities absent (short case;
sampling does not enumerate every fleeting descendant). Counters do NOT select
scalar estimation, cache data or change query/cut/gate/config/budgets.

Actual318 windows identify expensive strict chord preparation: at120s FOUR
completed chords cost57.996s and another is active; at180s FIVE cost138.749s,
active=false; at240s SEVEN cost138.871s and another is active. Geometry work2.604M,
density arithmetic.003898s. This does not establish identical-query frequency.
`cases/cubic_cell_replay318_v1/assessment.json` preserves measured scope.

Local319 is built but UNVALIDATED, inside this SAME BL cycle: reuse immutable
cubic descendant control boxes/hulls rebuilt in ClosestCurved::prepare for every
nearby query. Existing closest path has no result memo or prepared descendant nets;
surface PreparedFacet/Pool reuse currently serves approximate triangle unions,
which cannot replace exact cubic data. Any correction must reuse bounded pool
ownership/admission, preserve complete current support and fresh query-dependent
bounds/candidates, and prove cached/recomputed agreement and actual M6 progress.
No speedup or exact point-query repetition established yet. Four compiled files changed:
Common CNativeAffineReference.cpp/.hpp and GeometryBackend/BLMetric tests. Bounded
geometry-only data shares the scope half quota with tagged keys; all query bounds
remain fresh. Next build319, focused exact-cached/recomputed/resource checks, actual
captured M6 chords and whole first cell. Details in the linked bound design. No competing Euler
optimization, new configuration/scheduler/geometry authority or budget increase.

One connected BL cycle. Production316 local profile/feature bounds passed both
actual captured chords; full strict-cell qualification is still INCOMPLETE:
- Chord314 PASS60assertions/47.845s; certification37.560s/409726work/32cuts.
- Chord311 PASS37assertions; certification10.443s/113482work/9cuts vs314181652work.
- Faithful actual cell0 replay316 TERMINAL124/1802.249s (diagnostic1800), all5 actual
  identities absent and source/input/runtime pins verified. CPU1797.84+.60s;
  maximum sampled treeRSS324579328B, later stable224550912B. No integral/inner guard.
  This does NOT prove a full native5400 failure or which inner work dominates.
- Focused316 PASS277assertions/51.421s, all5 actual handles absent: cached/fresh,
  transported anchors, invalid padding and uncertain/global fallback.
- Test-only317 observation TERMINAL124/300s as planned, all5 identities absent,
  pins/runtime verified. At240s:2537406geometry work/1407density evaluations/.003751s
  density arithmetic.120→180s adds649969work but only5density evaluations. Thus
  geometry-heavy intervals are confirmed; chord-vs-point attribution is not.

Build316/317 status/source/runtime/binary hashes remain in their pinned receipts;
317 production CFD is byte-identical316. No new performance correction after316.
Affected319 serial and ONE MPI milestone passed as scoped above. After318 completes and
actual handles are terminal, choose the next connected correction from measured
phase cost. Do not launch full316/318 from chord passes alone, raise limits or add
another cycle. Review [bound design/evidence](Handbook/ADAPTATION_CURVED_BL_LEAF_BOUNDS_20261110.md).
Next meaningful result remains actual normalized target/BL mesh, followed by its
current-mesh suitability census, MPI constraints, TWO RANS events/transfer/CFD/restart.

Full SAMEseed RANS/BL314 is TERMINAL1/5462.393s. All10 exact actual owned
PID/start identities absent; all source/input/runtime pins verified. Original
61330-point/303359-tet inputs and saved SA seed remain intact. Initial CFD finite
CL.155731/CD.050268; same303 Hessian/D1 statistics. No normalized target or BL mesh,
transfer/continued adapted CFD/second event/restart. Selected failure is first
complexity trial cell0, a LATER chord work guard16385,7246 splittable leaves,
fresh uncertainty3.13542258e-8 > unchanged1e-8; adjacent0. Local eigenvalue7.00000248e12
vs global1.215e14. This is NOT an outer-timeout result. Full wall duration includes
all ranks and final failure collective; it is NOT measured selected-chord/cell time.
Other-rank local failures/progress are not exposed by the selected global error.
Maximum sampled tree RSS2,543,411,200B; sampled CPU31,422.76s includes MPI polling.

`R/m6_bl_inputs/crossing314_chord_v1` preserves exact chord/cell/provenance and
terminal assessment; cell coordinates/sensors/core/trial match303 EXACTLY.
Selected ideal leaf span~1.04e-18 vs coordinate ULP~2.22e-16 supports, but does not
prove, a coordinate-enclosure/Lipschitz-floor limitation amplified by a coarse
whole-chord feature bound. This motivates the connected316 local-bound correction, not a relaxed certificate.

Point315 passed164assertions on EIGHT deduplicated leaf/neighbor queries using
existing ClosestCurved and separate Evaluate,8600geometry work (6680+1920).
Certified interval width~2.26e-17 vs classifier radius2.48e-16. Measured tensor max
~8.54e6 vs7e12 CHORD bound; sampled normalized distance slope.3779 is not tangent.
This supports interval-local profile/feature envelopes, not a sample certificate
or a proven replacement bound. `point_probe_v1/analysis315.json` records scope.
[Working connected bound design](Handbook/ADAPTATION_CURVED_BL_LEAF_BOUNDS_20261110.md)
contains the numerical contract, actual source-query caller padding and remaining
proof decisions. Review those before qualification; no new config/framework,
CAD dependence, geometry representation, relaxed gate or increased budget.

Affected serial314 PASS283883assertions/22cases/291.681s; ONE MPI6 PASS27cases
EVERYrank/66.660s, all10 sampled MPI handles absent. Earlier7 baseline303 failures
outside this changed selection remain failures, NOT an all-suite pass.
`R/qualification314.json` seals current affected primitive/serial/MPI/actual-chord
PASS, with verified pins and actual terminal handles; no BL mesh/workflow claim.

Actual captured M6 chord314 PASSED45assertions/27.681s helper, all5 exact actual
PID/start identities absent, source/runtime/input pins verified. Chord certification
17.014s/181652geometry work/16cuts. This clears the captured refusal at unchanged
allowance/work caps, NOT whole-cell/target/generation or a controlled whole-cell
speedup (311 totalwork includes prior work before the terminal chord).
`R/cases/cubic_chord_replay314_v1/assessment.json` records scope.

Build314 TERMINAL0/64.130s, pins unchanged/all10 sampled actual handles absent.
Production CFD SHA `2db03acf36789faa1cd06335a6c8818306267d2f7d76f5e803f91868e66cc448`
(byte-identical312/313); test SHA
`c63f3578d9bc8d63ec78dc408bb9ca0063d8feaebf8c2e23cc1b1d7834aafd00`.
Focused314 PASS120assertions/1case/57.444s, all five actual test PIDs absent:
`R/cases/geometry_backend_20261110T002325_655729`.

Replay311 is TERMINAL1/1255.902s; all5 actual owned PID/start identities absent,
source/input/runtime pins verified. Exact13,328,602 geometry work matches310.
Captured chord and Decimal comparison are in `R/m6_bl_inputs/crossing311_chord_v1`;
terminal assessment in `R/cases/cubic_cell_replay311_v1/assessment.json`.
Fresh unresolved2.98786685e-8 (adjacent1.13571506e-9, splittable2.87429535e-8):
this is not mainly accumulator residue and does not prove irreducible uncertainty.

Production312 (retained in314) implements ONE connected correction: root-query reuse to bound all feature
contributions on a conservative enclosure of transported chord anchors. Original
wall bound and every feature remain; finite enclosure/distance arithmetic weakens
distant feature maxima, otherwise global fallback. Candidate query precision and
all original-facet realization contracts are included. Geometry exclusion and1e-8,
work/storage/time/cut gates unchanged. Shared public-edge and volume-column callers
use the same implementation. No construction or metric-target policy change.
New translated regression sections cover genuine features and transported anchors.
Current affected qualification314 PASSED as scoped above; complete BL generation
and full-target acceptance remain unproven.

Test-only313 corrected the new fixture orientation (solid exterior fluid) and314
replaced an invalid equal-frontier-count assertion with physical crossing accuracy.
Their preserved setup/assertion failures and all terminal receipts are linked in
the detailed BL handbook; no production/gate change accompanied either correction.

Use bounded point measurements to separate whole-chord feature overbounds,
coordinate enclosures, query precision and near-tangent distance variation.
Choose ONE demonstrated connected correction; reuse existing facilities and
qualify affected behavior at cadence, then promptly actual SAMEseed M6 RANS/BL.
Preserve all prior qualification/checkpoints and every final BL suitability,
transfer/CFD/TWOevents/actual adapted restart obligation. No budget increases.
Preserve unrelated dirty files; whitelist staging; fork pushes authorized.
Latest pushed checkpoint `eb01836389` preserves319 implementation/build, focused
exactness/resource checks and BOTH actual captured-chord passes. Whole cell319 is
terminal/incomplete; behavior qualification is sealed; full319 workflow is now active, not accepted.
Prepared certification319_affected.py / certification319_mpi6.py / run_rans_curved319.py
serial/MPI passed; full319 is now launched with terminal prerequisite receipts.
The actual cell stays passed:false if diagnostic124; its1800s observation does not
establish native5400 failure. After affected serial and ONE MPI pass, return to
actual SAMEseed M6 at UNCHANGED5400native/14400total budgets rather than extending
the diagnostic or choosing another optimization. Intrinsic guard failures require
a connected fix; a shorter observation timeout is recorded honestly, never a pass.
Complete the current strict-cell diagnosis/correction, affected checks and ONE MPI
milestone, then promptly full SAMEseed M6. Keep pinned sources frozen during jobs.

## Demonstrated BL blocker and next action

[Connected design, complete evidence and acceptance table](Handbook/ADAPTATION_CURVED_BL_INTEGRATION_20261109.md).
Original preflight296 refused M6 CurvedNets. Connected302/303 supports true cubic
wall distance/normals, flow/BL composition, quadrature/chords and shared bounded
resource scopes. This is NOT a complete BL generation claim.

Actual full two-event attempt `R/m6_runs/rans_curved303_v1` terminated1/1332.237s,
all pins/runtime verified and all10 owned identities absent. Initial CFD finite
CL.155731/CD.050268. First complexity trial cell0 refused an unresolved cubic BL
distance crossing before any normalized target or BL mesh. Exact cell/sensor/core
is preserved in `failed_complexity_cell0.json`; inputs/provenance also under
`R/m6_bl_inputs/crossing303_cell0_v1`.

Production304 separates geometric enclosure radius (still used for certified
Lipschitz exclusion) from outward ideal chord span (unresolved integral measure).
The total1e-8 bound, work/cuts/memory/time gates remain unchanged. Existing22 affected
cases/283883 assertions passed304. Translated genuinely cubic regression passed307
(11 assertions,12.997s);314 expands its scope as recorded above. Fixture setup
failures and the replay volume-copy precheck failure remain recorded, not passes.

**Replay fidelity correction:**308/309 had exact cell data but omitted production
outer-transition setup. They cannot establish the configured target's behavior.
`replay309_policy_fidelity_audit.json` supersedes earlier exact-replay claims.
309 was stopped for that demonstrated validation error; GNUtimeout's separate group
required reverified descendant cleanup to complete the SAME authorized stop.
All5 identities are absent; never repeat its stop helper or rerun the old replay.

310 correctly reproduces full-layer first-trial setup: FIRST_ROW initializes at
1/Hmax²=.0001; trial configuration at .007604989 retains the smaller core under
monotone support. Raw captured sensors feed the SAME barycentric/ApplySample density
callback. Independent caller review confirms base/A are not consumed by this strict
cell callback. Resource allowance differs from the full solver; this is not MPI
sensor recovery, full normalization, final A composition or remeshing qualification.

Faithful310 terminated1/1264.192s, all pins/runtime verified/all5 identities absent:
13,328,602 geometry-work units; chord16384-step work guard,7965 unresolved leaves.
The old message rounded its remaining bound to0.000000. No target/mesh accepted.
See `R/cases/cubic_cell_replay310_v1/assessment.json`.

311 captured both work/adjacent guard failures without changing decisions. Its
terminal outcome and precise extracted chord are summarized above. Root failure
metadata exposes global metric bound1.215e14, metric-length factor1.10227e7 for a
.00166317 chord. Actual ApplyFeatures weakens terms by feature distance at transported
anchors, whereas the old crossing bound sums every maximum. Local-bound correction
312 is a hypothesis under qualification; no speedup or full-target pass yet.
Prepared old `run_rans_curved308.py` MUST NOT launch: incomplete308 policy and stale
prerequisites. See linked design/history for fidelity and pending-cover audits.

## Saved RANS seed and unchanged full-case inputs

`R/m6_runs/rans_seed_continuation303_v1` completed80 further actual RANS/SA iterations
on the saved original61330-point/303359-tet grid: terminal0/23.568s, all pins/mapped
ELF verified/all10 identities absent. Independent fields/mesh/positivity checks pass.

- Continued restart SHA `c90000ea804c7e08a54dbec93f218b5bd3ae3dce3db8c287db6783fd138f1c78`
- Original mesh SHA `8c839170eb5e359f7de6a41434aa6a9c2d620053f37de6d9360bd04d2a63088b`
- Original no-CAD reference SHA `2e99fb7870e2c718f110da8509e4c0f7e746ce138e7f27ab57aa0dedbb6c42de`
- CL.155729588/CD.05027280454; last10 relative ranges.085%/.783%; rho residual-3.00758,
  SA-7.39381. All9 fields finite, exact coordinates, positive rho/internalE/pressure.
  SA1e-16 occurs ONLY at all19506 wall vertices; no historical clipping-ledger claim.
- `spatial303/` uniquely matches all wall nodes using Float32 representations and
  stores five finite Cp cuts. WallY+ median32.22/p90193.74/max447.81 warns coarse
  near-wall resolution. No-slip wall Mach0 is not an outer-flow shock diagnostic.
- `rans_seed303_use_decision.json` qualifies ONLY a coarse-grid initial adaptation
  seed, not converged/wall-resolved/experimental physics. Preserve it and originals;
  do not recompute from freestream or substitute Euler fields.

`R/m6_bl_inputs/rans_curved303_v1` contains exact copies and full-case config:
RANS/SA, Mach-only L1/500k, h0=2e-6,growth1.2,thickness.01, native5400/event,
outer14400 TOTAL,12GiB/rank, MPI6 CFD, FIRST_ROW1 native worker. Inputs/config/seed
must stay identical in the next comparison. Native FIRST_ROW currently requires1
worker; MPI6 parents do NOT establish distributed BL generation.

## Qualification scope and remaining BL/RANS requirements

[Current BL suitability evidence route](Handbook/ADAPTATION_BL_SUITABILITY_EVIDENCE_ROUTE_20261110.md)
records existing outputs and unproven curved-normal/progression/coverage properties.
No actual mesh failure is inferred from that read-only audit.

`curved_bl303_qualification.json`:107 serial cases/544736 assertions passed; shared
ledger smoke34061 assertions passed. Actual fluid-side M6 policy probe passed88
assertions on a tiny complete cell, NOT full target/generation. Resumable MPI303
selected348 cases;7 failing cases reproduce exactly on archived296, with both
baseline-control receipts. No new MPI failure identified; NOT an all-suite pass.
Do not weaken those expectations or repeat broad passing work during this cycle.

One event AtlasBudget covers preparation/engine/FIRST_ROW/reassessment/normalization;
borrowed scopes restore windows/lifetimes and retain cumulative work/failure/peaks.
Initial target has its existing config-bounded scope. Query scratch stays inside
existing ceilings. Preserve qualified construction, geometry certificates, bounded
IO/cache and ownership/dependency improvements.

BL suitability is distinct from metric q/J. Verify actual requested height and
alignment, spacing progression, coverage/outer transition, tangential flow conformity,
LE/TE/root/competing walls and justified feature transitions; recensus after split,
collapse, relocation, reconnection and ownership migration. Existing first-row/joint-
wall helpers do not prove full multilayer coverage. Temporary layer coordination is
allowed; no permanent frozen columns/coordinates, mandatory prisms, physics-label
reconstruction dispatch, larger cavities/new scheduler/config or mandatory CAD.
Then complete RANS500k: TWO strictly accepted meshes, conservative admissible flow
AND turbulence transfer, finite adapted CFD and actual isolated adapted disk restart.
Never weaken requested heights to fit complexity. All measurements must be known.

## Preserved Euler regression baseline

A293=`R/m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k`.
D293=`R/m6_runs/euler_build293_adapted_event1_disk_restart_v1`.
Both mandatory whole-surface two-sided certificates passed; all q/J/G/size failures
knownZERO. Zero inadmissible projected states; source enforces scaled target integral
defect<=1e-12 before publication (final numerical defects not printed).

| Event | Points | Tets | Native seconds | Continued CL/CD |
|---|---:|---:|---:|---|
|1|149051|736323|3640.719|.269781/.014756|
|2|172001|870422|2314.855|.269952/.015095|

True CV mass residual maxima7.722e-14/8.970e-14. Matching mesh/reference/continued-CFD
restarts independently copied under `R/m6_inputs/euler_adapted293_event{1,2}_v1`.
Coordinates exact/all22 fields finite, positive rho/internalE. Actual standalone
MPI6 adapted disk restart in D293 passed181.356s: CL.269980/CD.015165, finite/admissible
changed state, unchanged inputs. `workflow293_independent_validation.json` and
`disk_restart_validation.json` seal evidence. CFD was one iteration/event, not a
convergence/experimental shock-resolution claim. Counts differ from complexity50k.

Accepted first replay296 is byte-identical to293event1 (mesh/reference/restart).
-3.4% wall difference had IDENTICAL expensive geometry work/164 matching stages;
causal material performance gain NOT established. User directs BL, no further Euler
optimization cycle for small timing/reuse changes. Both binaries and1303 sources
are archived under `R/baselines/qualified_euler296_pre_bl_v1`; manifest verified.
`R/euler293_preservation_during_bl319.json` freshly verifies both saved checkpoints and
isolated restart artifacts still match hashes. Never overwrite them.

## Rules and records

Strict geometry1e-4,q>=.2,J>=.05,maxedge1.8 and requested heights; validity-first
construction/current nodal metric transport/CAD optional unchanged. One active cycle,
one owned heavy job; actual host load/RAM checks; nice19/libs1/build2–4 workers when
resources permit,4TB compile scratch. Keep overlapping timers separate; join-call
time is not total MPI wait. Judge quality by net whole-field q/J removal/s and
geometry-bad counts, not commits/debt percentages.

[BL design/history](Handbook/ADAPTATION_CURVED_BL_INTEGRATION_20261109.md),
[backlog](Handbook/ADAPTATION_BACKLOG.md), [next work](HowToProceed.md),
[bounded persistence](Handbook/ADAPTATION_REFERENCE_PERSISTENCE_20261109.md),
[boundary performance](Handbook/ADAPTATION_BOUNDARY_QUERY_PERFORMANCE_20261109.md),
[certificate evidence](Handbook/ADAPTATION_PREPARED_SOURCE_CERTIFICATES_20261108.md).
Resume-loop cycle renumber/input-overwrite and rejected-event consumption remain
OPEN; the verified isolated steady CFD restart does not establish loop resumption.
Earlier detailed handoff retained in
[pre-consolidation history](Handbook/History/HANDOFF_before311_consolidation_20261110.md).
