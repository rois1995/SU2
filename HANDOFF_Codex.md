# Native adaptation — handoff entry point

Updated2026-10-31. User has freed the machine and authorized continuing computation.
The full Euler50k→BL/MPI→RANS500k goal remains active; zero accepted M6 events.

**TERMINAL, unsuccessful:** `euler_build164_quality_debt50k`, full two-event Euler50k attempt.
All10owned handles independently absent; main exec23733 terminalexit1 after the
verified launcher was stopped only after six-rank native time-budget failure.
All1396run pins unchanged. See `event1_time_budget_stop_20261030/stop_receipt.json`
and its `terminal_handles.json`. No owned heavy job is live; sources safe to edit.
First stop guard selected nonexistent section=refine instead of actual stage=refine;
it aborted before any signal. Corrected guard, exact handles reverified, then only
mpirun1457249 received SIGTERM. Original failure snapshots preserved.

Qualified source `d1ad372091`, fixtures/evidence `475e5c4570`; latest pushed live
quality-entry checkpoint `c1a62b260a`. Build164 CFD SHA256
22c6d45a2f2f95205737745d0fbc8628e0f7199ee87fc0e8243dd8190ab8f9e8.
Config SHA2565b6be56224fa8964532f92085659d63e2586ae1936d585fc30939c1ed69225a1,
byte-identical162. Initial CFD restart also byte-identical162 (initial state only).

Size closed atgeneration21/3715.153native seconds,720963private cells; quality
entered3751.378s with52476q-bad/53533J-bad,3126geometry-bad,size0; all censuses
complete/consistent. Both migrations execute; quality368217cells/13.992s.
Exit5339.170engine seconds:725159private cells,725056measured (103unmeasured),
52359q-bad/53072J-bad,2989geometry-bad,size0; qDebt7968.1229/JDebt13929.2221
versus8115.4185/14256.1421atentry. Modest debt reduction, no accepted mesh,
transfer or adapted disk restart. Counts/debts at exit are partial, not acceptance.
Sampled summed process-tree peak12.606GiB; steady quality~8.21GiB. Startup cache
allowance exceeded162 and host CPU service fell to~.77during splitting; no isolated
timing/memory comparison. Bounded2s perf sample preserves active fitting + waiting,
not an all-rank stall; raw profile stays local. No debugger in164.

Active connected implementation: consistent whole-patch debt search across optimizer
operators/triangulation/placement, near-threshold closure, detailed boundary-support
cost at quality ownership, and existing quality-window telemetry. Source review
complete; bounded geometry-estimator API/tests delegated. Primary retains design/
acceptance. Optional Opus5.5 high read-only review timed out240s without report.
Candidate is implemented and about to be frozen for build165; no heavy job live.
Source includes all-operator exact debt merit, additive DP/placement, height-constrained
shape proposals, cached-length support-cost estimate and quality-window/last-debt
telemetry. Focused tests added; all changed behavior remains UNVALIDATED until build
and campaign complete. Preserve the source freeze once build165 starts. Preserve construction/cache/ownership improvements,
strict gates and full Euler→BL/MPI→RANS500k goal. Check next build ID, not assumed.

Read [active quality-closure diagnosis](Handbook/ADAPTATION_QUALITY_CLOSURE_20261030.md),
[current quality coordination](Handbook/ADAPTATION_QUALITY_HANDOFF_20261030.md)
and [cache/ownership integration](Handbook/ADAPTATION_CACHE_PHASE_20261030.md).

Historical build162 (`9e9ad2ee41` plus fixture correction) qualified the earlier
cache/ownership/construction handoff. Serial53cases and MPI6 59cases/rank passed;
full source/build/evidence history is in the linked quality-coordination record.
Those checks did not establish M6 acceptance.

The full `euler_build162_quality_handoff50k` attempt is TERMINAL, unsuccessful.
All10owned handles are verified absent and all1396run pins unchanged. Event-level
native max5407.566s exhausts configured5400s; refine detail max5345.260s. Only
then was verified mpirun1172494 stopped to prevent another unsuccessful workflow
event. No accepted mesh, transfer or adapted disk restart exists.

Size closed atgeneration21/3378.186s with720963cells; quality entered3415.096s,
after21instead of44generations. Migration moved368217cells in15.09s and all6
censuses stayed complete/consistent. But107quality rounds/607accepted local
quality-progress operations left726732private cells,53443q-bad/54009J-bad,
2969geometry-bad, zero size-bad;114cells are unmeasured. q/J debt is nearly flat.
Sampled summed process-tree peak10.998GiB. This is not workflow completion.
A failed read-only GDB symbol-load snapshot adds unknown timing overhead (upper
bound61.65s); detachment/rank resumption verified. Both perf samples and context
are preserved, raw data stays local. No further GDB attempt.

Next active connected diagnosis/fix: [whole-field quality closure](Handbook/ADAPTATION_QUALITY_CLOSURE_20261030.md).
The connected quality-debt correction is implemented and serial/MPI-qualified;
actual M6 acceptance failed; all owned handles are terminal, and sources are safe to edit: whole-patch measured q/J debt, shape/size/geometry demand
ownership, cheap merit/edge rejection before fitting guidance, and reuse of the
optimizer time bound. Build163 passed510.785s;164test-only correction passed39.168s.
All1296pins unchanged
within each build; all observed build/test handles terminal. Focused163found an
old synthetic metric fixture missing new debt totals; both one-cell and large-star
fixtures now supply complete totals. Connected164serial140cases/20260assertions
36.002s and MPI6 146cases/rank41.082s PASS. Production CFD SHA256:
`22c6d45a2f2f95205737745d0fbc8628e0f7199ee87fc0e8243dd8190ab8f9e8`.
Evidence: `staged57/quality_debt164_integration_summary.json`.
Actual full attempt `euler_build164_quality_debt50k` is terminal, unsuccessful; see top.
Preserve cache/ownership/construction improvements and strict gates; see the active closure record.
Use existing observers for bounded failure diagnosis; no private replay exists.
Historical build162 terminal evidence: that run's `terminal_assessment.json` and
`event1_time_budget_stop_20261030/`. Next unused build must be checked, not assumed.

Build160 M6 is TERMINAL: first native event exhausted5400s at5408.5s, then only its
verified launcher was stopped to prevent an unchanged retry. All10handles absent,
1396pins unchanged. Size closed atgeneration21/4158s; quality began5014s after44
split generations. Exit732439private cells still failed geometry/q/J; no adapted
artifact/transfer/restart. Peak sampled tree11.884GiB. Full terminal evidence lives
in its run directory, including `event1_time_budget_stop_20261030/`.

User explicitly authorizes pushes to their fork `https://github.com/rois1995/SU2.git`,
branch `codex/native-3d-bl`. The previous automatic-review rejection was resolved
by that explicit authorization; no permission is pending. Pushed failure/review
checkpoint `0e576ea198`, handoff `036661baa2`, candidate `9e9ad2ee41`.

Branch: `codex/native-3d-bl`. Integration implementation: `8cf566e229`; qualified checkpoint: `e5240df1e0`, pushed.
Connected cache/phase checks pass; full M6/transfer/restart remains unvalidated. The
construction base is `f104557fa6`/build157; the paused handoff was `adc31b971b`.
Build158 failed only to compile a new Catch2 assertion; its terminal pins and
failure receipts are preserved. No accepted M6 result exists.

## Read next

1. [AGENTS.md](AGENTS.md).
2. [Current cache/phase integration](Handbook/ADAPTATION_CACHE_PHASE_20261030.md) and
   [active construction/quality candidate](Handbook/ADAPTATION_QUALITY_HANDOFF_20261030.md).
3. [User's complete integrated next-task specification](Handbook/ADAPTATION_INTEGRATED_NEXT_TASK.md), saved verbatim between the supplied markers.
4. [Stopped build157 state, evidence and exact reproduction](Handbook/ADAPTATION_CHECKPOINT_20261030_STOP.md).
5. [Split coordination](Handbook/ADAPTATION_SPLIT_LANES_20261030.md) and
   [geometry-aware initial balancing](Handbook/ADAPTATION_GEOMETRY_BALANCING_20261030.md).

## Terminal build157 baseline and remaining acceptance

The owned `euler_build157_split_lanes50k` attempt is **terminal**, intentionally
stopped at4248.933wall seconds. All10owned handles, including the driver, were
verified and are gone. Final independent hashing verified all1396run pins,
1296build pins and both focused-test manifests unchanged (1397distinct files).
Stop/process/pin receipts and full terminal diagnostics are preserved.

There are **zero accepted adaptation events** and no published adapted mesh.
The last completed private generation21 has721311cells,91811accepted private
splits,2602collapses, no oversized edges, maximum metric edge1.7999916803,
3131geometry-invalid cells and geometry debt0.0004182131. Quality never started;
construction q/J were deliberately unmeasured. These private meshes are not CFD
restart states and were not exported as resumable private engine state. The input
has61330points/303359cells. Euler50k is a metric-complexity choice, not a point or
cell limit; RANS500k remains reserved for the later workflow.

The first interior sweep made40452private splits in50.08s; split coordination
removed a real coverage bottleneck. Geometry-only closure and quality handoff
remain unresolved. Prepared scope construction and collective waiting dominate
surface work; cache pools retain cold entries when full. Sampled summed tree RSS
peaked at21.664GiB; this differs from cache accounting and physical memory use.
Initial balancing exists; phase-aware repartitioning does not.

Build157 passes the focused serial11cases/5306assertions and MPI6 18cases/rank.
These do not prove complete curved mesh generation, BL capability or restart.
The dedicated lane test still uses candidate_batch=1; cumulative parallel-commit
assertions can be satisfied during collapse. Add split-specific concurrent
coverage. Stall handoff is unprinted/untested and admits slow positive progress;
quality-only scheduling omits geometry-only debt. More gaps and provenance are in
the stop handbook. No new test campaign was run for this documentation handoff.

## Active integrated task

Implement **bounded geometry caching and phase-aware adaptation ownership** as
one connected change under the linked user specification. Preserve finite
surface/interior split sweeps, immutable generation frontiers, claim release,
fresh full-cavity reservation/versioned publication, frozen-field identity,
cumulative budgets/epochs and strict final gates. Evaluate major-phase ownership
changes only at coordinated quiescent boundaries; account for full migration,
rebuild, cache-warming and transient-memory costs. Keep CFD ownership and donor
solution distinct. Do not restart the engine or project CFD merely to migrate.

BL compatibility is required during these shared changes: preserve wall policies,
geometry/feature associations, complete wall cavities, requested first heights and
clearance checks. No fixed-wall, permanent-column or Euler/RANS-label dispatch.
Use focused existing first-height/joint-wall checks; full curved/junction/MPI/
redistribution BL qualification follows accepted Euler, before full RANS.

## Completion and working constraints

Immediate objective: strictly accepted M6 Euler **CFD → mesh → CFD → mesh**,
conservative admissible transfer, usable published mesh/reference artifacts and
actual disk restart. Then dedicated BL verification and the RANS workflow. Wider
2D/3D steady/unsteady/history qualification remains part of the full project;
see [established design](Handbook/ADAPTATION_ESTABLISHED_DESIGN.md).

Keep geometry tolerance1e-4, q>=.2, J>=.05, maximum metric edge1.8 and requested
height gates strict. Validity-only construction omits q/J; all intermediate cells
retain exact positive/nondegenerate topology and mandatory geometry/body/feature
constraints. Actual geometry, metric and wall requests govern construction.

One owned heavy workload; low priority;2–4build workers (helper uses3), six bounded
local MPI ranks, single-threaded libraries, compile-tmp on4TB. Verify handles and
pins before edits/reuse; preserve resumable evidence; target runs drive fixes.
Push reviewable checkpoints to `https://github.com/rois1995/SU2.git`, branch
`codex/native-3d-bl`. The user explicitly reconfirmed authorization for source and
M6 configuration/hash/process/debugger/profile checkpoints on2026-10-30.
