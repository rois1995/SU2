# Native adaptation — current handoff

Updated 2026-11-10 Europe/Rome. Goal ACTIVE. Euler50k two-event adaptation,
conservative admissible transfer, finite adapted CFD and isolated adapted disk
restart are verified in build293. BL generation/constraint preservation under MPI
and the complete RANS500k workflow remain unproven. Keep this as ONE connected
curved-BL implementation/validation cycle.

R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.

## Frozen owned workload — authoritative current state

ONE heavy job: faithful first-complexity-cell replay311, tool session **65502**,
`R/cases/cubic_cell_replay311_v1`, launcher `R/replay_cubic_cell311.py`.

| Process | PID | Start ticks |
|---|---:|---:|
| Wrapper | 544556 | 31225876 |
| time | 544864 | 31226539 |
| timeout | 544865 | 31226539 |
| test_driver | 544866 | 31226539 |
| orted | 544867 | 31226540 |

Actual host identities are in `owned_handles.json`; resources/CPU are sampled every
2 seconds. Input and original 48,954-net no-CAD authority loaded. Runtime prints
initial/retained transition core .0001, captured trial core .0076049890473404921,
hgrad1.3; production IntegrationRule3D began at10.329s. No outcome yet.
Serial diagnostic limit1800s, native config5400s, nice19, single-threaded libraries,
4TB scratch. Do not edit pinned C++/helpers/binaries/runtime/config/input or launch
another heavy job until actual handles are terminal or preserved evidence justifies
the existing authorized stop workflow. Poll the SAME handle, never restart from an
observation timeout or stale log. Actual /proc/ps requires escalation; sandbox
absence is not terminal evidence. Docs/git snapshots may change while frozen.

Current build311 PASSED72.169s, all pins unchanged, all14 sampled build PIDs absent:

- CFD SHA `27eb8051216de283cb1e2f76b69bbc86019856cf7768039ad0d407a10c6dd3a1`
- test_driver SHA `3e7ceed54467544aaecb6fbcb5ac402316d99fb958a93a090c4e9a4a67aeb6c9`
- `build311_source_pins.json`, `build311_status.json`, `build311_terminal_handles.json`
- Pushed checkpoint `4b2b75b0ce`; source pin/binary receipts take precedence over
  historical build numbers. Preserve unrelated dirty logs/.orig/untracked/submodules.
  Fork pushes authorized; stage exact whitelists only.

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
(11 assertions,12.997s); later test-only changes preserve its code. Fixture setup
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

311 adds failure-only capture at BOTH work and adjacent guards. Pending split region
plus heap covers all remaining unresolved intervals once; fresh outward totals are
valid upper bounds. At a work exit, pending territory is not yet queried: exceeding
tolerance does NOT prove irreducibility. Query order, acceptance and budgets did not
change. Independent pending-cover review passed. Current frozen replay captures
precise endpoints/profile/intervals/fresh and incremental totals/work.

After terminal, verify actual identities, then use the existing generic parser:

```text
python3 R/parse_chord310_terminal.py R/cases/cubic_cell_replay311_v1 NEW_OUTPUT_DIRECTORY
```

Expand R in the actual command. Parser requires terminal/pin/runtime/handle receipts,
uses Decimal against the actual binary64 tolerance, and emits a nine-value chord
input plus provenance. It launches no job. Use that chord to select ONE demonstrated
correction, qualify affected behavior at the established cadence, then promptly
return to complete SAME-seed M6. No new baseline/perf/GDB campaign, raised budgets,
sampling-only exclusion or speculative fallback. Prepared `run_rans_curved308.py`
MUST NOT launch: its required308 replay failed and had incomplete policy.

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
`R/euler293_preservation_during_bl303.json` confirms both saved checkpoints and
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
