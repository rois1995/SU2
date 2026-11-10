# Native adaptation — current handoff

Updated 2026-11-10 Europe/Rome. Goal ACTIVE. Euler50k two-event adaptation,
conservative admissible transfer, finite adapted CFD and isolated adapted disk
restart are verified in build293. BL generation/constraint preservation under MPI
and the complete RANS500k workflow remain unproven. Keep this as ONE connected
curved-BL implementation/validation cycle.

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## Frozen owned workload — authoritative current state

Build321 TERMINAL0/62.122s, all18 sampled actual host build handles absent; source
pins verified. CFD SHA dbbbf8b98e60dc9a24692f0473bb6209b00b262a1cf945025a17ebf62e4b4aa1;
test c0ff0695e915e6e00680c448bea97588697f4d3fdce0130a0b644d7df78d2781.
Focused321 ACTIVE(session67371), all compiled sources/helpers FROZEN:
`R/cases/curved321_focused_20261110T070621_1682998`; wrapper1682998,
time1683100/timeout1683101/test1683102/orted1683103. Actual identities recorded in
owned_handles_actual.json. Verify terminal handles before source edits/next job.
The first focused launch failed before fixture entry with MPI_Win_create (exit53),
all5 actual handles absent; rerun uses established OMPI_MCA_osc=pt2pt. Preserve both.
320 passed246.232s/all11 observed actual host handles absent; no tests ran between
320/321. New mapped cubic proposal/shared-target capability remains UNVALIDATED.
Focused/affected serial and ONE MPI milestone pending, then promptly same-seed M6.

Full SAMEseed RANS/BL319 TERMINAL1/5457.353s (session70391 closed). All10 exact
owned PID/start identities are ABSENT in actual host /proc; terminal_handles_actual.json
seals this. Source/input pins and84mappedELF verified unchanged. No owned heavy job.
Run: `R/m6_runs/rans_curved319_v1`; assessment.json records failure before the first
normalized target/BL mesh: selected global cell0/trial1 exhausted original native5400s.
The full wall includes collective joining; CPU includes possible MPI polling.
Peak sampled summed treeRSS3,235,180,544B. No target or BL mesh accepted.

Build319 terminal0/368.360s, pins verified/all152 sampled actual build handles absent.
CFD SHA `c40a2b865d1be8ede594a52e8935d96047324325fdd49a9d165746bcf81550b6`;
test SHA `77d14ce513410ad6cbf43ed33b943b6ab93369cb460ced53b3bf36fd05ff9642`.
Initial CFD finite CL.155731/CD.050268 and Hessian/D1 prefix match314. Original
CFDMPI6/FIRST_ROWnative1, Mach-onlyL1/500k,h02e-6/g1.2/T.01,geometry1e-4,
native5400/outer14400TOTAL,12GiB/rank unchanged. No more cache microvariants:
reconsider target integration using existing bounded correction facilities.
[Next connected design](Handbook/ADAPTATION_CURVED_BL_COMPLEXITY_20261110.md).

Pushed implementation `eb01836389`; qualification/full-launch `3fdd2cfd31`.
[Connected design/history](Handbook/ADAPTATION_CURVED_BL_INTEGRATION_20261109.md),
[active bound/cache design and evidence](Handbook/ADAPTATION_CURVED_BL_LEAF_BOUNDS_20261110.md).
All source/input/runtime receipts are under staged57; keep unrelated dirty files.

## Active connected BL cycle and qualification scope

Original296 preflight refused curved M6 authority.302/303 added genuine no-CAD cubic
wall distance/normals, flow/BL composition, quadrature/chords and shared budgets.
Full303/314 failed first target-cell chord certification; no BL mesh was produced.
316 introduced interval-local profile/feature bounds, explicit rounded-distance
contracts and source-consistent metric query padding. Both captured chords pass,
but complete cell316 timed out at its1800s diagnostic limit (not native5400).
317/318 counters located long strict chord preparation intervals; they do not
establish identical-query frequency. All these owned handles are terminal.

319 reuses exact IMMUTABLE cubic region control boxes/hulls inside ClosestCurved.
It shares the EXISTING scope-half prepared pool/quota/clock pinning, using disjoint
facet+parameter keys. Query candidates, distance/clearance bounds, normals and
allowed-support traversal stay fresh. No closest-result memo, new configuration,
scheduler, geometry representation, physical-target or construction-policy change.
Cache metadata is charged; work units may rise while CPU falls. Scope-pool
competition can affect performance. Exact-query memo/upper seeding ideas are in
backlog, not another active implementation.

- Focused319 PASS231assertions/6.347s: exact cached/zero/tiny-cache results,
  restricted facets, separate bodies, work/storage/deadline refusals.
- Affected serial PASS52cases/286104assertions/326.610s; all5 actual handles absent.
- ONE MPI6 PASS55cases EVERYrank/164.704s; all10 actual identities absent.
- Actual captured chord314 PASS60assertions/32cut COUNT: certification26.624s vs
 31637.560s(-29.1%); work432259 vs409726 (+22533charged cache insertions).
- Chord311 PASS37assertions/9cut COUNT:7.449s vs10.443s(-28.7%);124069 vs113482work.
  Both chord trees terminal/all5 identities absent/pins/runtime verified. Actual
  cut coordinates were not logged for a bitwise cross-build comparison.
- Complete cell319 TERMINAL124/1803.413s at original1800s observation cap; all5
  identities absent and all pins/runtime verified. Last1740s window73chords/
 13734density evaluations/29.300Mchargedwork. CPU1798.88+.77s, sampled treeRSS
 maximum1681326080B, plateaued~1.68GB. No integral or intrinsic guard refusal.

qualification319.json seals BEHAVIOR scope; actual_cell stays passed:false.
The shorter1800s diagnostic does not establish native5400 failure. Actual full319
used original production budgets after behavior qualification and failed as recorded above. No target/mesh/
workflow acceptance claim. Prior303seven baseline-controlled MPI failures outside
this affected selection remain failures, not an all-suite pass. FIRST_ROWnative1
with MPI6 parents is NOT distributed BL generation qualification.

Use full319 outcome to drive the next connected step; no more speculative cache
variants during this frozen attempt. Preserve every acceptance/BL suitability,
flow+SA transfer, TWOevent/continuedCFD/actual adapted disk-restart obligation.

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
[pre311 history](Handbook/History/HANDOFF_before311_consolidation_20261110.md) and
[pre319 runtime consolidation](Handbook/History/HANDOFF_before319_runtime_consolidation_20261110.md).
