# Native adaptation — handoff entry point

Updated 2026-10-30. **Resumed by the user's explicit instruction.**
One active integration cycle: bounded geometry caching plus phase-aware ownership.
Read [current integration state](Handbook/ADAPTATION_CACHE_PHASE_20261030.md).
Build160 passed; connected serial36cases and MPI6 43cases/rank pass, pins unchanged.
The full two-event `euler_build160_cache_phase50k` attempt is ACTIVE: exec22082,
driver859288, mpirun859394 (time859392/timeout859393); verified rank handles and
commands are in its launch receipt. Initial CFD completed; no adapted event is
accepted. Source/binary/config/helpers stay frozen. Inspect actual handles and
pins before edits/restarts; keep the authorized stop/fix/retry workflow.
Build159's MPI failure was an ownership-specific fixture assumption, now corrected
and preserved. Build158's assertion-compilation failure is also preserved.
The terminal build157 baseline below remains historical evidence, not a live job.

Branch: `codex/native-3d-bl`. Integration implementation: `8cf566e229`; qualified checkpoint: `e5240df1e0`, pushed.
Connected cache/phase checks pass; full M6/transfer/restart remains unvalidated. The
construction base is `f104557fa6`/build157; the paused handoff was `adc31b971b`.
Build158 failed only to compile a new Catch2 assertion; its terminal pins and
failure receipts are preserved. No accepted M6 result exists.

## Read next

1. [AGENTS.md](AGENTS.md).
2. [Current cache/phase integration](Handbook/ADAPTATION_CACHE_PHASE_20261030.md).
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
