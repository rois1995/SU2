# Native3D work-limit failure classification — 2026-10-12

Repository: /media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated.
Branch: codex/native-3d-transport-accounting. Starting checkpoint:
85658a22f884941fd381ce1c72b0e5f49f9b40d6. Goal 1 active; Goal 2 not started.

## Change and invariant

The independently audited production 512 private mesh is valid under the declared
geometry/q/J/L criteria, despite its rejected coarsening completion status. The
old end-of-budget message described every failure as unfinished strict metric
completion, conflating that case with genuinely invalid metric cells.

CNativeEngine3D now uses its existing final maximum reduction to classify:

- 0: all existing completion conditions satisfied; success as before.
- 1: all cached metric cells measured and valid, but coarsening candidates remain.
- 2: cached metric size/shape gates remain unsatisfied on at least one worker.
- 3: at least one worker has incomplete metric-cell measurement coverage.

The reduction remains one integer MAX with the same payload size and engine
collective count. Its zero/nonzero result exactly matches the former Boolean
MAX. Any incomplete measurements take priority over invalid cells, which take
priority over valid-but-unfinished coarsening. All workers share that base cause;
existing last-local-rejection detail still follows it when available. No attempt
is skipped, budget increased, physical gate relaxed or partial mesh accepted.
No ABI/config/operator/sensor/BL/fade/Hessian-noise/2D/MMG change.

The existing one-round rejected refinement fixture now requires the invalid-cell
message and confirms globally nonzero invalid cells. The existing one-round
coarsening control requires the valid-but-pending message, globally zero invalid
cells and globally nonempty candidate work. Both still reject and preserve the
accepted input. Default 512 and mismatched-mode/storage/identity/atomic veto tests
remain in the unchanged 97-case selection.

## Actual qualification

completion_diagnostic_build_v1 passes the complete configured 68-object driver
and 774-member archive qualification after the incremental engine/test rebuild.
Every compiled source/object/archive and binary is pinned, with no drift.
completion_diagnostic_mpi1_v1, mpi2_v1 and mpi4_v1 all pass 97 cases per rank,
seven existing exact distributed-record audits, actual geometry/solver/current/
BDF/SU2CGNS bridge audit and serial MMG 48 assertions/2 cases. Worker-subset/empty-owner
controls remain covered by the existing suites. Detailed phase/transport checks
remain in their receipts.

check_completion_diagnostics.py verifies the complete final evidence, current
compiled-source pins and exact numerical records against the preceding qualified
box_filter_mpi1/2/4_v1 binary. It finds 169/271/391 byte-identical raw geometry,
donor and state JSON records, respectively. Rejected private records are included
and remain distinct from accepted driver results. Timers/counters/certificates
are not incorrectly treated as mesh data. Across all 1/2/4-worker Boolean state
combinations and refinement/coarsening modes, 8336 checks prove the old/new global
completion truth value agrees. This is logical equivalence evidence, separate
from actual compiled fixtures; the missing-measurement reason is not fault-injected
through a private engine cache.

Recheck:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_3d_core_v1/check_completion_diagnostics.py
```

COMPLETION_DIAGNOSTIC_VALIDATION.json contains exact case/build/checker pins and
all matched numerical hashes. Each original case validation keeps full raw logs,
source/object/archive identity, independent audits, current/BDF states, actual
SU2/CGNS readback and rejected/control data. All owned jobs are terminal, executed
sequentially on one CPU/nice19/thread1 (MPI<=4) after real host/memory checks.
Four unrelated FSI ranks were active. These are correctness runs, not profiling.

## Frozen cluster comparison compatibility

The existing four source variants remain unchanged. A fifth fully pinned
box_diagnostics variant differs from box only in the engine completion messages
and regression assertions. Matching now checks all six relevant source hashes
rather than selecting a version solely from donor code, since these two versions
share that donor hash. Mixed old/new engine/test combinations reject.

The ten supported forward pairs include the previous six and
original/routing/packed/box->box_diagnostics. Existing frozen packages are retained
and supported; fresh current candidates must be prepared after pulling/rebuilding.
The same baseline 7f76fd9a279cf384a37f8ae137dc4dc26212ce82 isolates the prefilter
with the diagnostic update. No timed accepted workload or budget changed.
completion_diagnostic_package_checks_v1 passes 32 core+14 comparison checks, including
full shared-donor version identification, mixed-source rejection and reversed-pair
rejection. Full actual compiled source sets also match box->box_diagnostics; this
local check does not claim two freshly frozen cluster bundles or compiled-option
calibration. No comparison bundle or cluster job is launched here.

## Remaining gates

The valid-but-pending default 512 coarsening still rejects. Diagnostics are not a
repair of that completion limit. Strong/thin/n8/persistent-restart whole campaigns
retain their earlier source/binary pins; this change does not silently claim new
runs of them. Repeated cluster total-cost/scaling/detail-profiler overhead,
rank/weight/wait/traffic and memory/practical-limit calibration remain required
before full Goal 1 completion. Curved geometry, production 3D BL, actual full-unsteady
CFD and affordability remain later goals and are not certified here.
