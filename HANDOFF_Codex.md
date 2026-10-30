# Native adaptation — handoff entry point

Updated2026-10-30. User has freed the machine and authorized continuing computation.
The full Euler50k→BL/MPI→RANS500k goal remains active; zero accepted M6 events.
Read [current quality coordination](Handbook/ADAPTATION_QUALITY_HANDOFF_20261030.md)
and [cache/ownership integration](Handbook/ADAPTATION_CACHE_PHASE_20261030.md).

Connected candidate `9e9ad2ee41` preserves migrated census flags, skips self-collapse,
hands measured size closure to quality at a complete finite-generation boundary,
and tries the bounded optimizer before topology guesses. Strict final gates,
validity-only construction, geometry returns, caches, frozen donors and budgets stay.
Build161 passed315.521s. Its serial checks passed; MPI found an old local-ownership
assertion in the quality-pass fixture. Corrected that fixture to gather unchanged
rows and compare global count/versions/bytes, retaining automatic migration.
Build162 passed108.240s,1296pins unchanged; only that fixture differs from161.
Connected serial53cases/12462assertions35.085s and MPI6 59cases/rank41.049s PASS,
pins unchanged. All observed build/test handles are terminal. CFD SHA256:
`6bb6568831e8900e448f48da98e3c9bb714284a94cb21e3507936c61f3d0ad44`.
Summary: `staged57/quality_handoff162_integration_summary.json`.

The full two-event `euler_build162_quality_handoff50k` attempt is ACTIVE: exec57326,
driver1172403, time1172492, timeout1172493, mpirun1172494, ranks1172498–1172503.
Actual commands/cwds/start identities and config equality to build160 are verified
in `launch_receipt.json`. Initial CFD solved and wrote input-mesh restart00000;
coarsening completed1217.769native seconds:292744private cells,2582collapses,
3541geometry-bad/68372size-bad, q/J unmeasured. Split ownership moved115883cells
in5.4s; splitting is active. Generation1 completed1380.654native seconds with
516198private cells,106930size-bad/3878geometry-bad, max edge4.540239 and q/J
unmeasured. Coarsen/gen1 counts and debts match160, which took1462.894/1714.272s;
host availability differs. No accepted event. Compact current stage receipts are
in `coarsen_assessment.json`, `coarsen_comparison_to160.json`, `stage_assessment.json`.
Source/binary/config/helpers are frozen. Inspect
actual status/owned handles before edits/restarts; retain authorized stop/fix/retry.
Input61330points/303359cells;50k is metric complexity, not a mesh-count limit.
Prelaunch~33.65GiB available, load4.07/3.75/5.97; no active paging in bounded sample.
Qualification checkpoint `d0c1dbc3f7` is pushed; fixtures are not workflow completion.

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
