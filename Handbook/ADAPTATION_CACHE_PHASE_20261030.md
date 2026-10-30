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

## Source review while the host is occupied

Current override: build162 full M6 is terminal on its first event time budget.
All owned handles are absent; next connected work is
[whole-field quality closure](ADAPTATION_QUALITY_CLOSURE_20261030.md). Older live
observations below are historical. No accepted workflow exists.

The connected implementation now continues in
[the quality handoff record](ADAPTATION_QUALITY_HANDOFF_20261030.md). Build162 and connected serial/MPI6 checks now pass; the user has freed the host
and the next complete M6 attempt is pending launch.
The discussion below records the review that led to that candidate.

The user attributes timing uncertainty to other local jobs and asks to use this
window for code improvements. No heavy build, MPI campaign or M6 relaunch is
started during this window. Light read-only review and source edits are allowed;
all prior owned handles are independently rechecked absent in
`code_review_host_receipt.json` (host load10.92/12.45/13.48,~33.2GiB available).

Unvalidated source corrections now prepared:
- Preserve `census_started` and `progress_consistent` when staging migrated
  Selection state. Restored exact rows alone do not set those flags, so an
  otherwise complete migrated census can report incomplete until another edit.
  Existing forced-migration fixture now checks the immediate quality-entry census;
  a new already-admissible no-edit case requires complete flags after both forced
  transitions and zero attempted edits. This covers the masked no-work case too.
- Skip the selected vertex as its own collapse destination in the existing repair
  fallback. This removes a known invalid direction and misleading last rejection;
  the endpoint check was intentional, not the cause of M6 failure or a major
  performance gain. All geometry/metric admission remains in place.

These edits are NOT compiled or validated yet. Next identifier is build161 unless
another terminal receipt reserves it. Do not treat build160 checks as covering
these changed sources or run a stale binary as their validation.

The remaining connected design question is construction/quality coordination.
Evidence: size closes at generation21 yet geometry-only construction continues23
more generations; after the quality-entry census/migration, only~16operator rounds
fit before timeout. Quality tries three vertex moves and up to16collapse/reconnect
directions before the bounded optimizer, with repeated private geometry and metric
certification and collective synchronization. On some ranks most optimizer trials
validate, but cumulative counters are insufficient to claim changing order will
reduce wall time. Review this expensive ordering together with phase ownership of
residual geometry, rather than only moving a timer threshold.

A naive handoff-at-size-closure patch is incomplete: streamed metric validation
currently promotes certified geometry progress to `GeometryRepair` only outside
bidirectional mode (or for height-progress repairs). Thus putting geometry-only
debt in a quality queue does not guarantee an admissible geometry-only direction;
normal `Repair` still requires size or q/J progress. Any earlier handoff needs a
coherent residual-geometry policy and strict tests: no q/J construction screening,
mandatory exact validity/body/features throughout, no falsely accepted census,
strict final geometry/q/J/size/height, monotone budgets/IDs, meaningful repair work
before returning to construction, and both actual M6 events plus transfer/restart.
This review does not justify a new fallback ladder or merely increasing limits.

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
Qualification checkpoint `e5240df1e0` and live launch checkpoint `3928fbebe8`
were pushed. The full attempt `euler_build160_cache_phase50k` is now TERMINAL.
First event failed its native time budget at5408.35–5408.54engine seconds.
After preserving that failure and rehashing all1396run pins unchanged, stopped only
verified mpirun859394 to prevent the unchanged second event. All ten owned handles
are absent; driver terminal exit1 follows that authorized stop. The primary
failure was native time exhaustion, not the subsequent SIGTERM. Receipts and
pre-stop diagnostics: `event1_time_budget_stop_20261030/`. Full sampled tree peak
11.883728GiB (not the largest individual-process RSS); `terminal_assessment.json`
records terminal status, peak sample and stage rows. No accepted adaptation or
adapted artifact exists. Source edits are now safe; no owned heavy job is live.
User asks for code improvements while other local jobs finish; hold the next heavy
build/M6 run during that window. Host contention makes timing comparisons provisional.

Initial CFD solved one iteration and wrote input-mesh restart00000. Config is
byte-identical to build155's endorsed50k config. Input61330points/303359cells;
50k remains metric complexity, not a point/cell count. Private point counts are
not independently exported and must not be inferred from that target.

Size closed at generation21,4158.2138engine seconds,720963private cells,
maximum edge1.7999928685,3126geometry-bad. Construction continued through44split
generations. Quality began only at5014.1426s with731135cells,61754q-bad,
63459J-bad and278geometry-bad. Quality-entry migration actually moved349270cells
in13.24s, including4.06s partitioning and1.82s directory rebuild. Its complete
q/J census incorrectly printed incomplete flags after migration, confirming the
source-reviewed `census_started` preservation defect; the next measurement restores
it. This must be fixed without resetting budgets, identity or cached certificates.

Final private exit:732439cells,96245splits,2582collapses,62235q-bad,63848J-bad,
273geometry-bad, zero oversized edges;161cells had not yet received their shape
census. Bad counts are partial, not an accepted assessment. No private mesh was
published or paired with the input CFD restart. Geometry debt2.13374646e-6 remains.
The endpoint-collapse diagnostic is a rejected fallback direction, not the stop
cause. Read-only source review found that vertex fallback fails to skip its own ID.
The demonstrated coordination limitation is spending~93%of the budget before the
first quality opportunity, including~856s after metric size already closed.
Review construction→quality ownership of residual geometry and repair scheduling
as one connected fix; do not raise limits or add a local fallback ladder.

Coarsening completed1462.894engine seconds:292744cells,2582collapses,
68372size-bad and3541geometry-bad cells. Cache-accounted bytes646684568,
including114505520facet bytes and532179048scope bytes;9204evictions.
First in-event migration then actually moved115883cells in10.2s, including
~2.8s partitioning and~2.05s new-directory build. Predicted split-work max/mean
improved3.03->1.13. Existing CFD donors/frozen field were preserved; there was
no ownership-only CFD projection. Both major-phase migration paths executed in M6; neither establishes acceptance.

Matched comparison to157: coarsening1726.614->1462.894s; generation3
2310.332->2073.359s,681829->681500cells, max edge3.26786->2.89690.
Geometry-invalid4238->4241, geometry debt.00752017->.00808828 (~7.6%higher).
Prepared cache17.9GiB->.66GiB at generation3. Previous sampled tree peak21.664GiB
came from a longer run; full current sampled tree peak11.884GiB is not whole-job
physical memory. Matched generation21 was4158.21s versus4079.17s before (~2%later);
the early advantage did not persist. See `comparison_to157.json`, `stage_assessment.json`, initial
partition assessment and resource receipts under the attempt.

The user correctly emphasized host contention: treat wall-time differences as
provisional observed comparisons, not isolated implementation speedups. Scope
hits/misses at generation3 are2390/12851->2432/12899; scope preparation
2993.14->2839.28rank-seconds. The first interior sweep was~62s versus50s before.
The main RAM improvement is the aggregate cache cap plus eviction of cold scopes;
reusable facet data fit comfortably. Fundamental certification work remains.

A bounded5s late-coarsening profile has2388samples and zero lost samples,
~78.5%inclusive sampled cycles in Allreduce,~13.2%surface union and~12.4%
TriangleUnion construction. Inclusive shares overlap. Raw20MBprofile stays local
with SHA/context. Original inline-symbol rendering hit addr2line DWARF errors;
only verified profiler report910783 and its symbolizer were stopped, never M6.
Bounded no-inline reports completed normally. See `profile_1450s_context.json`,
`profile_1450s_noinline{,_flat}.txt` and report-stop receipt.

The initial per-host allowance is2335452160bytes, divided across6ranks before
per-facet preparation rounding and answer-memo reservation. Current retention
is bounded below that allowance; it is not a process-memory limit. Forecast
calibration and a source-reviewed census diagnostic edge case are recorded in
[the backlog](ADAPTATION_BACKLOG.md); the census flag defect was observed at actual M6 quality entry. The terminal
time-budget failure now drives the next connected fix.

After the connected fix and validation, return to the actual target. Require both accepted events, conservative
admissible transfer, published mesh/reference artifacts and isolated disk restart,
then dedicated BL/MPI qualification and RANS500k. No lower-memory/fixture result
establishes that capability.
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
