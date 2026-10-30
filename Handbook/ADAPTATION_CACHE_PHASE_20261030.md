# Bounded geometry residency and phase ownership — active integration

Resumed from pushed `adc31b971b`, implementation `f104557fa6`/build157.
The explicit user resumption supersedes the one-time handoff pause. There is one
active implementation/validation cycle. No accepted M6 adaptation exists yet.
Euler target remains 50k metric complexity, RANS 500k. All geometry, q/J, size,
height, transfer and disk-restart gates remain strict. Construction split lanes,
finite frontiers, full reservations and versioned publication are preserved.

## Provenance and observed limitation

`staged57/cache_phase_resumption_provenance.json` independently rehashed all
1397 distinct handoff pins (run/build/serial/MPI manifests), with no changes;
all ten recorded owned host handles were absent. HEAD equals the local tracking
ref for the pushed branch. Existing unrelated logs and untracked files remain.
Host before build158: 34 GiB available, load 5.32/5.72/7.81, nearly full swap;
root 3 GiB and repository disk 64 GiB available. Use 4TB compile-tmp.

Build157's private geometry closure has no remaining oversized edges but 3131
geometry-invalid cells. Scope preparation consumed 5584/7327 nested upper-query
rank-seconds; cold scopes occupied full pools while later scopes were refused.
Recorded summed tree RSS 21.664 GiB is not cache bytes or physical memory.
Initial geometry balancing was useful; phase migration did not exist.

## Connected candidate (not yet validated)

Collective reference loading gives each MPI shared-memory domain at most 4 GiB
of geometry query cache, additionally capped at a quarter of observed free
physical pages, then divides the domain allowance equally among its ranks.
This is a new explicit cache-only policy, not a reinterpretation of native working
storage. Noncollective decode retains its previous local allowance semantics.
No query-path collectives or sidecar-format changes. The answer memo is charged
against the quota; tight allowances omit that optional memo. Native working
storage admission remains independently mandatory.

Prepared data has separate facet and scope pools, each half the remaining
allowance. Bounded clock replacement protects observed reuse with capped credit
based on reconstruction work per byte. One-use scopes start with no credit.
Up to 64 entries are examined per admission; refusal remains possible rather
than unbounded scanning. Active shared references pin both data and accounting.
Map/list/key metadata is included conservatively. Scratch and old/new overlap
remain query/working-storage obligations, outside retained-cache bytes. Exact
authority, support, tolerance and preparation-resolution keys are preserved.
Compact stage rows add live facet/scope bytes, entries and eviction accounting.

Initial metric and geometry workload prediction selects coarsening demand.
The engine evaluates ownership once at coarsening-to-splitting and once after
the permitted full quality-entry census. No generation or lane migration.
Weights use measured edge demand, complete distributed vertex-star counts and geometry preparation;
quality weights additionally use measured q/J/height and geometry debt.
The automatic decision uses predicted imbalance, observed attempted-edit cost,
initialization/seeding, estimation, prior transitions and scope-warming allowance,
with a twofold uncertainty hurdle. This is a heuristic, not calibrated CPU time
or proof of improved balance. Actual transition costs are recorded separately.

At a quiescent transition, stage a new Owned map, repartition it with the existing
atomic migration facility, route exact cached census/edge certificates with the
same destinations, build a new directory, and stage new selection queues/frontier.
Publish these together after collective agreement. Full old/incoming ownership,
records, old/new directory and selection storage overlap is admitted. An active
rollback checkpoint retains its ownership rather than being partly migrated.
Old phase pass claims and rejection queues retire together at the boundary.

CavityMetricSource retains the original bootstrap, references the stable working
map object and reads transported nodal sensors. CFD ownership/donors stay intact;
no projection, engine restart, target renormalization, field identity change or
ID/work/time reset occurs. The worker communicator membership stays fixed.
Geometry caches remain local and bounded, without phase/rank query keys.

Geometry-only invalid rows now join quality repair queues, closing the handoff's
known omission. A split-specific concurrent-commit counter qualifies split lane
coverage independently of collapse commits.

## Validation and next acceptance

Build158 is terminal FAILED561.841s on a Catch2 assertion-expression syntax error;
production targets compiled, all1296pins unchanged and owned PID765540 absent.
The assertion is corrected. Review also identified phase deadline checks, now
added at safe staging/publication points, and cache replacement now charges its
bounded maintenance against cumulative query work. No target result exists yet.
Build159 passed522.746s, all1296pins unchanged. Serial connected regression
PASS36cases/10215assertions,32.077s, pins unchanged:
`cases/cache_phase159_serial_20261030T160844_827121`.
Six-rank connected regression FAILED35.079s, pins unchanged:
`cases/cache_phase159_mpi6_20261030T160940_829443`.
The old coordinate-only repair fixture required unchanged per-rank ownership but
left the new automatic ownership option enabled. Its local REQUIRE then caused
rank test-flow divergence and MPI_ERR_TRUNCATE in the next test. This fixture now
explicitly disables ownership migration; the dedicated phase fixture still
forces and checks both transitions. All nine observed owned handles are absent
(terminal_handles.json). Production source/binary is unchanged by this correction.
Build160 PASSED108.234s, all1296pins unchanged. Connected serial PASS36cases/
10215assertions32.002s; MPI6 PASS43cases/rank34.983s, all pins unchanged.
Cases: `cache_phase160_serial_20261030T161443_842216` and
`cache_phase160_mpi6_20261030T161737_849261` under staged57/cases.
The positive MPI fixture requires both ownership changes and actual moved cells;
its negative variant rejects staged ownership before publication. Both pass.
All observed test handles are terminal. Production binary SHA256:
`67249dbe3f2d331cd67bafe768dd605feb86d7280edfc26a0679d55e591eb03b`.
Compact receipts: `staged57/cache_phase160_integration_summary.json`.
No M6 adaptation, transfer or disk restart is accepted yet. The next action is
fresh full two-event `euler_build160_cache_phase50k`, after checkpointing this
qualification; source/binary/config/helpers stay frozen throughout that attempt.
Focused checks cover cached/recomputed geometry and eviction, host share math,
phase migration through opposing metric demands, field/donor identity, finite
split scheduling/concurrency, existing first-height and joint-wall behavior.
A failure injected through the existing stage observer checks that staged migration
can be rejected without publishing new ownership. Complete-star census checks
cover different rank layouts and tight-memory atomic refusal.
DiagnosticAlways is an internal test control to exercise both migration paths;
production defaults to the profitability decision.

After affected checks and one connected regression, run the complete two-event
M6 Euler attempt with the existing harness and gates. Compare equivalent phase
milestones, cache rebuilding, transition cost, useful work and sampled aggregate
memory. Fixtures and memory reduction are not completion. Accepted Euler must
include conservative admissible transfer, mesh/reference publication, continued
CFD and independent usable disk restart. Then qualify curved/junction/MPI BL and
redistribution before the corresponding RANS500k workflow.

Any additional ideas belong in the [backlog](ADAPTATION_BACKLOG.md) unless a demonstrated blocker
requires them. Preserve stop/fix/retry evidence and push reviewable checkpoints.

Reproduction of the connected focused checks after a successful terminal build
(use its actual identifier; never run against a failed/stale binary):

```bash
for mode in serial mpi6; do
  python3 - build160 "$mode" <<'CHECKS'
import sys
from pathlib import Path
build, mode = sys.argv[1:]
root = Path('integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57').resolve()
sys.path.insert(0, str(root))
import run_cases
filters = ('[NativeSurfaceCache3D],[NativeSurfaceMemo3D],[NativeGeometryStats3D],'
           '[NativeReferenceIO3D],[NativeReference3D],[NativeAtlasReferenceIO3D],'
           '[NativePartition3D],[NativePhaseOwnership3D],[NativeSplitLanes3D],'
           '[NativeGenerationProgress3D],[NativeConstructionPassClaims3D],'
           '[NativeStageProgress3D],[NativeFirstLayer3D],[NativeJointGeometry3D]')
if mode == 'mpi6':
    filters += ',[NativeConcurrentComposed3D],[NativeConstructionHandoff3D],[NativeMappedPublication3D]'
name = 'cache_phase160_' + mode
run_cases.CASES[name] = (1 if mode == 'serial' else 6, filters)
sys.argv = [str(root / 'run_cases.py'), build, name]
run_cases.main()
CHECKS
done
```

After passing checks, full target command (a fresh attempt directory is mandatory):
`python3 integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/run_m6.py build160 euler_build160_cache_phase50k 2`.
Do not launch until build/test handles are terminal and pins/resource headroom are
verified. The helper's workflow result does not itself validate isolated restart.
