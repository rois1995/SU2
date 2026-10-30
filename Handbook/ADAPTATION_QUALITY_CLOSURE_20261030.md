# Active diagnosis: globally useful M6 quality closure

**Current full attempt LIVE:** `euler_build164_quality_debt50k`, exec23733,
driver1457158/time1457247/timeout1457248/mpirun1457249/ranks1457268–1457273.
Run `launch_receipt.json` pins exact identities;1396pins verified, config identical
to162. Source/build/helpers/config remain frozen. Initial CFD0 advanced into metric
recovery; no adaptation accepted. Native5400s/outer14400s. Qualified evidence/source
checkpoint475e5c4570 is pushed. All162 notes below are terminal historical evidence.

Build162 full M6 Euler50k attempt is TERMINAL, unsuccessful. All ten owned handles
were independently verified absent and all1396run pins unchanged before the
connected source correction. Builds163/164 and the serial/MPI integration campaign
are now terminal and qualified. The actual build164 M6 attempt above remains live;
its pinned sources must not be edited. Euler→BL/MPI→RANS500k, strict final gates,
conservative admissible transfer and actual disk restart remain mandatory.

Evidence root: `staged57/m6_runs/euler_build162_quality_handoff50k/`.
Pushed source/qualification history and design:
[quality handoff](ADAPTATION_QUALITY_HANDOFF_20261030.md),
[cache/ownership](ADAPTATION_CACHE_PHASE_20261030.md).

## Actual target outcome

- Size closes at generation21/3378.186engine seconds,720963private cells.
- Quality enters3415.096s, after21 rather than44split generations. Both phase
  migrations execute; quality migration moves368217cells in15.09s. All six
  post-migration censuses are complete/consistent, confirming the flag fix.
- Entry52476q-bad/53533J-bad,3126geometry-bad, zero size-bad; qDebt8115.4185,
  JDebt14256.1421, geometryDebt0.0006976122.
- Event fails the configured5400s budget: native whole-event maximum5407.566s;
  refine-detail maximum5345.260s (different timer scopes). First event complete0
  on all ranks. Only afterwards was verified mpirun1172494 stopped, preventing
  an already unsuccessful two-event workflow from retrying. Driver then exits1.
- Private exit726732cells,53443q-bad/54009J-bad,2969geometry-bad, zero size-bad;
  qDebt8135.0285/JDebt14121.6554, geometryDebt0.0006965354. There are114unmeasured
  cells; counts/debts are partial and cannot establish final admissibility.
- Quality gets107rounds,607accepted local quality-progress operations; optimizer
  trials350/validated321. Yet whole q/J debt barely improves and bad-cell counts
  grow. Merely counting accepted edits is not evidence of closure. Counts are
  cumulative where indicated in `terminal_assessment.json`; do not sum rank-rounds.
- Sampled summed process-tree peak10.998GiB, not individual RSS or physical memory.
  No accepted adaptation, adapted mesh/reference pair, transfer or adapted restart.
  Initial restart00000 belongs only to61330point/303359cell input. Other restart
  outputs after the rejected event still belong to the retained CFD mesh.

Full terminal/first-event pin/process receipts:
`terminal_assessment.json`, `event1_time_budget_stop_20261030/stop_receipt.json`,
`terminal_handles.json`, full logs/rank stdout/resources, first-quality assessment.
A GDB symbol-loading snapshot failed without useful counters; final detachment
was verified. Actual pause duration is unknown, conservative upper bound61.65s.
Keep this timing limitation explicit; it does not establish a counterfactual pass.

## Demonstrated limitation and next design work

The missing capability is strictly accepted whole-field quality/geometry closure,
not phase entry or cheap cache residency. More time in quality alone did not
close the bad-cell set. Before choosing a connected implementation, inspect:

1. Whether feature-age ordering services already shape-valid geometry-only users
   before the actual bad-cell set, and whether new descendants repeatedly consume
   a useful pass. Geometry-only demand and normal bidirectional Repair admission
   need coherent ownership; do not silently drop geometry from final acceptance.
2. How local minimum-score improvement and insertion affect whole changed-patch
   q/J debt and bad-cell coverage. Determine what the actual target lacks before
   replacing the merit rule or adding operators.
3. Whether complete imported stars/cavity interfaces leave enough coordinated
   freedom. Boundary=false is O5/O6 topology control, not physical coordinate
   pinning: SmoothBoundary can redistribute physical vertices when their stars
   have no artificial interface incident. Never move incomplete external stars.
4. Geometry fitting query cost versus mandatory geometry validity. Early5s sample
   has71.19%inclusive Allreduce/19.73%surface union; late2s shows rank0 actively in
   SmoothBoundary→GeometryGuidanceExcess while peers wait. No all-rank stall was
   demonstrated. Saved callchain+disassembly identifies the late sample as the
   *fresh candidate* guidance call (SmoothBoundary+0x133c, call13e1178), not the
   old-side score (call13e055e). Therefore old-score caching alone does not explain
   or solve this sampled hot path. Cheap projected shape tests already precede
   the fresh guide; edge certification follows it. Assess opportunities using
   actual call ordering and retain all mandatory/final checks.

Ideas remain in [the linked backlog](ADAPTATION_BACKLOG.md). Do not accumulate a
fallback ladder, add a forecasting project, raise limits arbitrarily, or treat
more accepted local operations as completion. Any chosen change must connect
operator/selection/phase behavior to the actual whole-mesh failure and preserve
bounded work/memory, exact validity/topology/body/features, geometry authority,
wall constraints, frozen field identity, donor ownership, cumulative epochs and
conservative transfer/restart semantics.

Reuse existing stage/transaction/rejected-state observers if a bounded difficult
case snapshot is needed; inspect their production wiring before inventing tooling.
The failed private engine state was not exported as an exact resumable mesh.

## Acceptance of the next connected change

State its missing capability, direct evidence and complete acceptance conditions
before C++ edits. Qualify risky behavior with focused existing fields/geometries,
then one connected serial/MPI milestone and immediately the complete M6 target.
A complete accepted mesh under unchanged1e-4geometry, q>=.2, J>=.05, max edge1.8
and requested-height gates is required; two accepted events plus conservative
admissible transfer, continued CFD and isolated usable disk restart complete Euler.
Then dedicated curved/junction/interface/subsequent-operation/redistribution BL/MPI
qualification before the RANS500k workflow. Actual mesh counts remain separate
from metric complexity. No fixture, memory, local-edit or partial-debt substitute.

## Connected correction qualified for the actual target

Normal validity-only shape repair now measures the whole changed patch's sum of
normalized squared q/J hinge deficits, rather than accepting a minimum-score gain
that can increase total debt. Streamed per-cell measurement carries both the sum
and measured cell count; incomplete measurements cannot authorize shape progress.
Construction remains edge-only and never queries q/J. Absolute diagnostic admission
retains its existing policy. Size progress remains separately classified.

Bidirectional quality demand services shape/height first, residual size next, then
explicit geometry-only repair. Geometry repair must strictly reduce certified
geometry excess while preserving closed q/J, changed-edge size and requested-height
gates. Geometry remains mandatory in final acceptance and can return to general
construction after a finite useful pass. No scenario switches or relaxed gates.

Boundary smoothing rejects useless whole-star shape debt and oversized moved edges
before expensive candidate fitting guidance. The unchanged old geometry score is
computed lazily within the transaction. Streamed shape proposals receive measured
merit admission before fitting guidance; geometry/height intents retain their
separate checks. The existing optimizer time bound receives the event's remaining
time, without resetting cumulative work or IDs. The connected serial/MPI checks pass; actual M6 acceptance is still unvalidated.

## Actual build164 progress (live, sources frozen)

Initial CFD restart is byte-identical to162, allowing reuse of its initial-state
finite/positive inspection; this is not adapted restart evidence. Startup partition
discrete fields match162. Coarsening completed at1190.751native seconds versus
1217.769at162,292744cells/2582collapses,68372size-bad/3541geometry-bad. Selected
counts and all q/J/size/geometry debt sums match exactly; q/J are correctly
unmeasured in construction. All six censuses are complete/consistent.

Startup cache host quota3266762752bytes (rank544460458before rounding) exceeds162's
quota because more physical RAM was free. Actual prepared live1285311176bytes
versus646684568, facet live unchanged114505520; scope live1170805656versus532179048.
Scope builds1609.780rank-seconds versus1665.658; evictions9003versus9204. Observed
time is not isolated source speedup. Coarsening→splitting migration moved115883cells
in5.597s. All1396run pins reverified after the transition. The actual quality phase
and full mesh acceptance remain ahead; continue one owned workload.

Construction closed size at generation21/3715.153native seconds,720963private
cells,max edge1.7999928685. Quality entered3751.378s with52476q-bad/53533J-bad,
3126geometry-bad,size0, qDebt8115.4185323/JDebt14256.1420969. Complete entry counts
and all measured debt sums match162 exactly. All six censuses are complete and
consistent with allshape measured. Quality migration moved368217cells in13.992s.
Quality repair is LIVE; the changed repair behavior is now under actual validation.

Production observe_progress explicitly suppresses window output. No interim
quality-debt census is available; assess full debt/closure at the next emitted
phase/event census. Do not invent current progress from absent output.

Two-second49Hz perf sample:539samples,zero lost. Inclusive shares77.60%Allreduce,
18.96%GeometryGuidanceExcess/surface-union query,14.38%SmoothBoundary, wrapper self0.
Shares overlap and are neither phase totals nor speedup/debt-closure proof. Active
fitting plus collective waiting was observed, not an all-rank stall. Raw4,998,372byte
profile stays local, SHA256022f1736ccc58e7c6ebdcc5b0b5d0f87bfb833701e3ef9eb8f43877df9ba3cd3.
Reports/context/assessment are saved; no debugger attachment or source change.

Host contention rose during splitting: over446wall seconds every owned rank
received~.77CPU seconds per wall second, compared with nearly full CPU service
earlier. Load11.67, available~26.5GiB, no active paging at that observation.
In quality aggregate RSS~8.21GiB, transient sampled peak12.606GiB; all1396pins and
10owned identities reverified and host headroom remains adequate. This is not
an isolated memory/timing comparison: startup cache quota differs too. Continue
one healthy low-priority workload and leave unrelated jobs untouched.
