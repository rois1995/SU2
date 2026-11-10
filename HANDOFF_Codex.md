# Native adaptation — current handoff

Updated2026-11-10 Europe/Rome. Goal ACTIVE. Euler50k TWO accepted events,
conservative admissible transfer, finite adapted CFD and isolated adapted disk
restart are verified293. BL generation/constraint preservation under MPI and full
RANS500k TWO events/flow+SA transfer/CFD/actual adapted restart remain unproven.
ONE connected curved-BL cycle. R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## Frozen owned workload — authoritative current state

Full SAMEseed RANS/BL325 ACTIVE(session20806), all compiled C++/helpers/inputs FROZEN.
`R/m6_runs/rans_curved325_v1`. Wrapper1960877/start34441534;
time1962667/start34444969,timeout1962668/mpirun1962669/start34444970;
ranks1962672,1962673,1962674,1962675,1962678,1962684(starts in owned_handles.json).
Current status/owned_handles/resources are authoritative. Verify actual terminal
PID/start identities before edits/another job. Preserve outcome unless evidence
justifies the existing authorized owned stop. No other owned heavy workload.

SAME303 saved SA seed/mesh/ref/config. CFDMPI6/FIRST_ROWnative1, Mach-onlyL1/500k,
h02e-6/g1.2/T.01, geometry1e-4, native5400/outer14400TOTAL,12GiB/rank unchanged.
Initial CFD finite CL.155731/CD.050268; Hessian/D1 prefix matches323. Now bounded
shared FIRST_ROW target construction; no normalized target, BL mesh or accepted event.
launch_observation.json independently verifies1595source/input pins,84mappedELF and
all10 exact live identities. At216.963s sampled summed treeRSS3,747,180,544B,
MemAvailable25.81GB. CPU includes possible MPI polling, not useful-work proof.
Reference cache host4GiB/rank715827882B; model reservations are not actual RSS.
Monitor actual summed tree memory and host headroom; leave unrelated jobs untouched.

Build325 terminal0/32.122s/all10 sampled actual build handles absent. Production CFD
byte-identical324: afcb2a2e3bc586d5f1838e5e713a4ca5d0c2d73d025397980694a6c594adf3ee;
test e0daef24add891fb4af8daa05555dbb0666a443259653e5cf2edbe277910a042.
Current325 implementation/qualification/full launch checkpoint needs push; Git/doc
snapshots allowed while sources remain frozen. Never edit qualification325.json or
other pinned receipts/helpers during this run.

## Connected correction and scope

296 refused curved M6 wall authority.302/303 implemented genuine no-CAD cubic
wall distance/normals, composition, quadrature/chords and shared resource budgets.
303/314 failed chord work;316 bounds and319 exact immutable region reuse cleared
captured chords but full319 failed first target cell at native5400s. No BL mesh.
322 qualified padded cubic correction proposals, true authority evaluation and
shared FIRST_ROW normalization; its full case then failed a distance query.
323 diagnostics captured/reproduced exact far-field point(-19.516658935823891,
2.0845123426984715,19.090249991586017), component/hint10154,38800facets,
error2e-12,work16384. All these historical handles terminal and pins/runtime verified.

324/325 reuses EXISTING ADT distance discovery with proven outward arithmetic
padding in a safe squared-radius range; otherwise keeps original cube. Full support,
allowed facets, mutex/sorting, fresh normals/clearance/certification and all budgets
remain. No new index/cache/config/scheduler/geometry representation or metric policy.
[Proof, callers and evidence](Handbook/ADAPTATION_CURVED_BL_COMPLEXITY_20261110.md).
Caller is strict FP, ADT fast-math; proof covers DAZ/FTZ and positive accumulation.
Read-only review found no concrete current binary64 completeness defect under stated
conditions. Other scalar build modes and exact range endpoints remain unqualified.

- Focused325 PASS120assertions/5.767s/all5 actual handles absent: indexed48facet/two-
  body tight32work oracle, cold/warm/zero cache, tiny radius, changed rounding,
  explicit FTZ/DAZ+subnormal input, restricted support/work/storage refusal.
- Actual saved point325 PASS39assertions/11.816s/all5 identities absent/pins/runtime
  verified: direct406work at SAME16384cap/error2e-12, interval width1.04805e-12;
  further metric evaluation204work. Inward final-upper cube contains38800roots,
  a subset of old initial cube. Old root preparation necessarily exceeded16384
  before refinement regardless of cache hits. Point323 stays FAILED16384work.
- Affected serial PASS64cases/286722assertions/362.233s/all5 actual handles absent.
- ONE MPI6 PASS62cases EVERYrank/459.039s/all10 exact identities absent/pins verified;
  assertions4701/4664/4703/5297/4752/4712(log order).

qualification325.json is BEHAVIOR scope, not normalized target/BL mesh/workflow
acceptance. No full timing speedup inference from a previously incomplete query.
325 only corrects324 test's unjustified exact-zero upper expectation and adds FP
checks; production identical324. Preserve failed evidence. Prior303seven baseline-
controlled failures outside affected selection remain failures, not an all-suite pass.

Let actual325 outcome drive next work. No more isolated baseline/perf/GDB campaigns
or speculative cache variants while frozen. Preserve every BL suitability, mesh,
transfer, continued-CFD and restart obligation. FIRST_ROWnative1 with MPI6 parents
is NOT distributed BL generation qualification. CAD stays optional.
[Integration history](Handbook/ADAPTATION_CURVED_BL_INTEGRATION_20261109.md),
[bound/cache history](Handbook/ADAPTATION_CURVED_BL_LEAF_BOUNDS_20261110.md).

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
