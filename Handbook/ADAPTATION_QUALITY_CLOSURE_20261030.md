# Active diagnosis: globally useful M6 quality closure

Build162 full M6 Euler50k attempt is TERMINAL, unsuccessful. All ten owned handles
are independently verified absent; all1396run pins unchanged. Sources are now safe
to edit. The connected quality-debt correction is frozen for build163
(session90746, Ninja1427130,1296pins). Do not edit compiled sources while active. Preserve one connected cycle and
return to the complete target after the next coherent fix. Euler→BL/MPI→RANS500k,
strict final gates, conservative admissible transfer and actual disk restart remain.

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

## Connected correction in progress

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
time, without resetting cumulative work or IDs. These changes are UNVALIDATED.
