# Native adaptation — current handoff

Updated2026-11-10 Europe/Rome. Goal ACTIVE. Euler50k TWO accepted events,
conservative admissible transfer, finite adapted CFD and isolated adapted disk
restart are verified293. BL generation/constraint preservation under MPI and full
RANS500k TWO events/flow+SA transfer/CFD/actual adapted restart remain unproven.
ONE connected curved-BL cycle. R=`integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## Current owned workload — authoritative

Build331 LIVE (tool session5435, wrapper3437236/start37782066,
compiler driver3437269/start37782116). FIVE changed compiled files pinned; sources
FROZEN until every actual build handle is terminal. Low priority j3,4TB scratch,
47.4GB available at launch. This is a prerequisite-only focused build, NOT a BL
capability milestone. Prepared helpers: `R/check_bounds331.py`,
`R/replay_chord331_after_bounds.py`; run only after successful terminal build and
actual-handle checks. No MPI/full attempt until connected surface/front integration.

## Active connected curved-BL strategy

User now requests integrated surface preparation, constrained/blended directions,
coordinated first layer/front growth and outer transition BEFORE another full run.
[Current strategy, audit and acceptance conditions](Handbook/ADAPTATION_CURVED_BL_CONSTRUCTION_STRATEGY_20261110.md).
Audit-only bounded agents completed without edits/jobs. Prerequisite diagnosis uses
ONE preserved actual330 chord with existing build330 diagnostic, helper
`R/replay_chord331.py`, case `curved331_actual_chord_20261110T173020_3365437`.
Chord replay TERMINAL1/401.019s; all5 actual identities absent, pins unchanged.
It reproduced EXACT16385work/406leaves/.00100516uncertainty refusal;5.957792M
geometry work/396.318s primitive. Adjacent-point probe TERMINAL0/12.684s,172assert,
7770geometrywork; all5 actual handles absent/pins unchanged. Both helpers retained.
Uncommitted prerequisite prototype under frozen331 build: certified candidate-box
plus endpoint squared-distance concavity exclusion, existing Lip fallback/limits.
Read-only proof review found no remaining blocker after optional endpoint fallback
fix; build/focused qualification pending. Surface/front integration remains to
implement; do NOT start full workflow after a kernel-only pass. Current strategy
records missing facilities, proof and actual diagnosis. ONE connected milestone.

## Frozen owned workload — authoritative current state

Full SAME303seed BL/RANS330 TERMINAL: outer14400 timeout124, wrapper14403.143s.
ALL10 actual host owned identities ABSENT in330/terminal_handles_actual.json;
source/build/input pins and mapped runtime verified unchanged. Current331 freeze above.
Two remesh attempts both FAILED in W wall-edge measurement before any split/first
row: W5503.34/5503.79s,58305 unmeasurable edges,0 splits. Most edge failures were
reported after time expiry; do not call them independent geometric obstructions.
Rank0 native5508.26/5508.14s; first-event weights only.00433s versus329's5398.49s:
the selected sole-destination correction removed that blocker.0 accepted meshes,
0 adapted transfer/checkpoint/restart. Later full attempt overwrote same event0 BL
report filename; raw log preserves both.330 assessment/terminal manifest preserve
outcome. No representative generated BL patch exists yet.

Initial metric1226.497s had EXACT same target/support/geometry work53473769 as329;
wall−14.02% is not attributable to routing change, which executes afterward.
Target still unqualified1.175M vs500k. Preserved initial prefix and assessment.
NEXT: inspect earliest actual composed-edge/chord refusals and full W counters;
select ONE connected correction from demonstrated failure. Do not substitute an
unproven surface-remeshing policy or increase budgets. Existing W is only a split
prepass from incoming surface, not complete surface coarsen/relocate/reconnect/
quality closure. Preserve requested Coulaud/MAdLib actual-patch investigation until
an actual generated result. Same saved303seed and every gate remain mandatory.

Full SAMEseed RANS/BL329 TERMINAL after evidence-backed authorized stop. Event0
rejected at native5400s: weights5398.49s, import<=.781s, donor build<=.690s;
no first-row construction or accepted operation. Driver consumed event and entered
cycle1 on the original61330-point grid; TWO accepted events were impossible.
`R/m6_runs/rans_curved329_v1/assessment.json`, raw prefix, stop request and manifest
preserve evidence. All10 exact actual host handles ABSENT in
`terminal_handles_actual.json`; rechecked before330 source edits. Current331 freeze is authoritative.

Selected connected correction: skip unused expensive partition work prediction
when there is only ONE native destination. Existing Repartition initializes every
color to0 and calls graph partitioners only for parts>1; uniform admitted weights
cannot change sole-worker routing. Preserve graph/directory validation, donor/cell
migration, multiworker prediction, geometry/height/shape certification and budgets.
Focused serial + ONE MPI milestone then immediate SAME303seed full BL/RANS.
330 minimal remesher branch and sole-destination migration check implemented.
Build330 PASSED22.192s,17 actual sampled handles absent, pins unchanged.
Qualification330 SEALED: serial25.411s10/11cases, MPI6 51.481s21/22cases EACHrank.
Only known baseline diagnostic assertions2780/2781 fail (serial4, MPI24 total);
no new failure. New sole-worker routing and all other affected checks pass.
All build/serial/MPI/full330 actual handles absent; full330 outcome preserved.
`R/qualification330.json` explicitly is NOT an all-suite or BL workflow pass.
Helpers `R/check_weights330.py`, `R/run_rans_curved330.py`; full replay terminal; current failure above.


Original303 savedRANS seed, Mach-onlyL1/500k,h02e-6/g1.2/T.01,geometry1e-4,
native5400/outer14400TOTAL,12GiB/rank unchanged. CFDMPI6/FIRST_ROWnative1 is NOT
distributed BL generation qualification. Target UNQUALIFIED:
outside_estimated_bounds/provisional1, estimate1174528.386 with empirical
indicator108761.874 versus500k; sensor-only complexity2.08846 at density floor.
Metric-build1426.532s,8192samples/6516inside,6516preparation+13032full/base geometry
queries, ONE trial. No actual BL mesh yet. Current POLICY floor is not a proven
physical500k impossibility; inherited wall tangent contribution remains unmeasured.
Preserve first representative actual-patch Coulaud/MAdLib analysis.

329 query-wide optional cache allowance qualification remains valid: checkpoint
be73153380 PUSHED. CFD SHA69d635b238df15292c22f28e7aebee7083b8adabd73210dc320918cd53e36e72;
test SHAaebaf1f8a84b6bfae5a6a134230803f496067d2f2f75d235c26a27e1ed3f1057.
Read-only setup deadline gaps did NOT cause this outcome: measured import fast;
weight-stage event deadline fired at5400s. Do not implement an unrelated setup fix.

## Read-only329 metric-stage diagnosis

[Concrete attribution and gaps](Handbook/ADAPTATION_CURVED_BL_METRIC_STAGE_DIAGNOSIS_20261110.md)
is preserved with `R/m6_runs/rans_curved329_v1/metric_stage_attribution_bounds.json`.
At least899.206s of the broad1424.828s bucket lies outside its constructor/assessment
calls (loose overlapping-timer upper525.622s, not an additive breakdown). Final
nodal audit makes66760 target evaluations incl5430 halo instances; support covers
the domain, so base closest is evaluated at each.6516 full SamplePoints requests
repeat at initial target attachment with unchanged full model/version/coordinates.
A.Weight adds conditional base evaluations outside printed profile counters.
Current charged cache maintenance <=27.09% by source call upper bound; this is NOT
a timing split. Root/refinement, exact/nearby identity and total collective-wait
seconds remain attribution gaps. No new job/profiler or production change.
Actual global inherited-tangent contribution is unmeasured; a clearly labeled
planar face-covariance/normal-law scale proxy is638k, not the production1.175m
integral, physical lower bound or accepted target. Preserve the queued actual
patch analysis. Terminal329 selected the330 routing correction; preserve330 outcome
before selecting further implementation.

## Current connected capability and evidence

296 rejected curved wall authority.302/303 added genuine no-CAD cubic wall support,
distance/normals, composition and chord integration.316/319 improved bounds and
immutable region reuse. Full319 still failed normalization's first cell at5400s.
322 uses existing bounded wall-support correction proposals, actual authority
sampling and shared FIRST_ROW normalization; empirical uncertainty is not a proof.
324/325 correct cube-overinclusive closest-root discovery using existing ADT distance
screening with outward arithmetic padding; fallback stays conservative. Strict-FP
caller/fast-math ADT proof covers current binary64/rounding/DAZ/FTZ envelope; other
scalar modes/exact range endpoints remain unqualified. No new index/representation.

326 full evidence isolated optional cache maintenance:16384work=2seed+17root+
538refinement+15827cache.327/328 one clock scan per cubic miss retained generic
leaf/union64-scan behavior, pinning, byte admission and charges. Full328 reached a
DIFFERENT point but still failed16384=2seed+46root+5604refinement+10732cache.
Both failed before a normalized target/BL mesh; all actual handles terminal and
pins/runtime verified. Detailed receipts: `R/m6_runs/rans_curved{326,328}_v1/assessment.json`.

329 completes the SAME optional-maintenance correction: at most64 charged cache
units per entire cubic query; skip retention if fewer than2 remain for scan+insert.
Fresh immutable geometry still drives full certification; no certificate/query
order/tolerance/work-cap/target/repair change. Generic leaf/union default64 per
admission unchanged. At identical traversal, totalwork<=uncached+64 and<=3xuncached.
Original query cap16384 can still refuse genuinely expensive geometry.

qualification329.json seals: focused681assertions/2cases/8.797s; actual two captured
points58assertions/13.964s; focused MPI6 14cases EACHrank/42.690s. All actual handles
absent/pins verified. Actual325control1314work;328failure5791work at SAME2e-12error/
16384cap; interval widths3.44961e-13/1.78769e-12. Cold serial history differs from full
warmed MPI, so not full target qualification. Reused325 unaffected64case serial and
328 broaderMPI62cases EACHrank/408.243s remain explicit behavior evidence. Prior303
seven baseline-controlled failures remain failures, not an all-suite pass.

[Connected design, proof and full evidence](Handbook/ADAPTATION_CURVED_BL_COMPLEXITY_20261110.md),
[bound/cache history](Handbook/ADAPTATION_CURVED_BL_LEAF_BOUNDS_20261110.md),
[pre329 handoff history](Handbook/History/HANDOFF_before329_cache_consolidation_20261110.md).

## User-queued investigation — first representative saved result

At FIRST representative saved actual BL mesh/patch, run the bounded
[Coulaud2025 leading-edge audit](Handbook/ADAPTATION_LEADING_EDGE_TANGENT_METRIC_QUEUE_20261110.md).
Paper read in focused primary-PDF extracts; hypotheses only. Separate physical
curvature/tolerance demand, flow/geometry/height target plus gradation, and actual
directional conformity/q/J on complete surface/neighboring volume patches. Smooth
patches and genuine features separately; a patch seam is not a physical feature.
Keep CAD optional, all gates and current L1/Hessian policy. No high-order elements,
new geometry/scheduler or paper-normal extension as a BL height law. See backlog.

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
`R/euler293_preservation_during_bl329.json` freshly verifies all22 saved checkpoint/
isolated-restart artifacts (384685699bytes), plus all1305 supplementary qualified296
source/binary archive files, unchanged against existing manifests. No CFD rerun or
new current-runtime numerical qualification. Never overwrite preserved artifacts.

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
