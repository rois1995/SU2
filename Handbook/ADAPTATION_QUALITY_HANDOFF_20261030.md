# Construction / quality coordination after build160 M6

Active connected continuation of [cache/ownership integration](ADAPTATION_CACHE_PHASE_20261030.md).
Status: connected regression PASSED; full M6 workflow unvalidated. User freed the
machine and authorized computation. Build161 passed315.521s and serial checks;
MPI6 exposed an old fixture's local-ownership assumption. Corrected that assertion
to gather unchanged cells and compare global identity/versions/bytes, keeping
migration enabled. All nine observed failed-campaign handles are terminal.
Build162 passed108.240s,1296pins unchanged. Only the fixture differs from161.
Connected serial PASS53cases/12462assertions35.085s; MPI6 PASS59cases/rank41.049s,
all pins unchanged and observed handles terminal. Summary:
`staged57/quality_handoff162_integration_summary.json`. CFD SHA256:
`6bb6568831e8900e448f48da98e3c9bb714284a94cb21e3507936c61f3d0ad44`.
Qualification checkpoint `d0c1dbc3f7` is pushed. Full two-event attempt
`euler_build162_quality_handoff50k` is ACTIVE: exec57326, driver1172403,
mpirun1172494, ranks1172498–1172503 (time1172492/timeout1172493). Actual
commands/cwds/start identities and unchanged config verified in `launch_receipt.json`.
Initial CFD completed and wrote input-mesh restart00000. Coarsening completed
1217.769native seconds:292744private cells,2582collapses, q/J unmeasured,
3541geometry-bad/68372size-bad. Split ownership moved115883cells in5.4s.
Splitting is active; generation1 completed1380.654s,516198private cells,
106930size-bad/3878geometry-bad, max edge4.540239, q/J unmeasured. Its counts/debts
match160 generation1 at1714.272s; no accepted event. Completed coarsen counts/debts/cache
accounting match160, which took1462.894s; host availability differs. Receipts:
`coarsen_assessment.json`, `coarsen_comparison_to160.json`. Sources/binary/config/helpers are frozen. Inspect actual handles before
edits/restarts. All strict
Euler/transfer/restart requirements remain, then BL/MPI and RANS500k.

Latest live state: size closed atgeneration21/3378.186native seconds,720963private
cells, maximum edge1.7999928685, zero size-bad and3126geometry-bad. Quality entered
3415.096s, after21generations versus160's44. Migration moved368217cells in15.09s;
all6ranks report complete/consistent q/J/geometry censuses after migration.
Measured initial quality debt:52476q-bad/53533J-bad, qDebt8115.4185/JDebt14256.1421,
3126geometry-bad. About1985native seconds remained on entry, versus~386before;
wall-time differences also reflect changed host availability. No accepted event
or adapted artifact. `first_quality_entry_assessment.json` preserves all6rank rows
and transition details. Sources remain frozen during the full attempt.

The5s early-quality profile (`profile_quality_early_context.json`, no-inline
reports; raw data stays local) has2563samples, lost0. Inclusive71.19%Allreduce,
19.73%QuerySurfaceUnionAtResolution,16.89%GeometryGuidanceExcess (0.00%self).
These overlap and are not phase wall timings or a quantified speedup. Exact
command/start/cwd/executable-inode checks selected the six owned ranks. An initial
wrapper substring precheck failed before any profiler launch; exact executable
matching corrected it. No solver stop occurred. Record avoided root build-id-cache
writes; both bounded no-inline reports completed normally. The read-only review's
transaction-local old-guidance idea remains in the linked backlog pending the
actual target result; existing exact subquery memoization may already save work.

The compact `stage_assessment.json` records completed six-rank stage aggregates;
cache-byte categories are sums, not per-rank maxima. Individual rank stdout avoids
merged `run.log` interleaving. Complete diagnostic channels mean measured, not
strictly accepted; q/J/geometry debts still must close before transfer/publication.

## Demonstrated limitation

Authoritative failure evidence is under
`staged57/m6_runs/euler_build160_cache_phase50k/` in the established evidence root.
`terminal_assessment.json`, `run.log`, rank stdout, resources, pins and
`event1_time_budget_stop_20261030/` preserve the complete first-event failure.
All ten owned handles are terminal. Native first event failed its5400s ceiling;
only afterwards was the owned launcher stopped to prevent an unchanged retry.

Size closed at generation21/4158.21s,720963private cells,3126geometry-bad.
Construction continued23more split generations; quality started at5014.14s with
731135cells,61754q-bad/63459J-bad and278geometry-bad. Only~16operator rounds fit
before5408.5s. Exit732439cells still has273geometry-bad and substantial partial
q/J debt;161cells were not yet shape-measured. Nothing was published or accepted.
The CFD restart on disk belongs only to the61330point/303359cell input mesh.

Across the six terminal engine detail rows, bounded optimizer trials37 and
validated35. They occurred only in quality. These are accepted candidate counts,
not evidence of net whole-mesh quality closure or isolated timing improvements.
The host was occupied; compare observed timings provisionally. Summed sampled
process-tree peak11.884GiB is distinct from cache accounting and physical memory.

## Connected source changes

1. At the existing quiescent, complete-census split-generation exit, continue
   descendant construction when measured size debt remains. If size closes,
   enter the permitted quality census instead of spending further generations
   exclusively on geometry. No timer threshold, case identity, solver label,
   generation cap or q/J construction screening is added.
2. Preserve the existing quality-to-construction return for residual
   `construction_invalid`, which includes geometry debt. Geometry remains a
   construction obligation and a strict final gate. A quality pass can improve
   shape/height before the next geometry generation. `Repair` and
   `GeometryRepair` admission rules are unchanged; geometry-only progress is not
   silently reclassified as sufficient bidirectional quality progress.
3. In streamed quality, try the existing bounded coupled optimizer after the
   three coordinate trials and before up to16directed collapse/reconnect trials.
   All workers cross the same collective boundary. Successful peers retain their
   certificates while other peers continue. Size-only repairs still reach the
   optimizer outside the move/topology loop. The same160work reservation,
   unused-work refund, donor/field identity, geometry/height checks, private
   metric certification, complete dependency reservation and publication apply.
4. Earlier checkpoint `0e576ea198` preserves the migrated census flags and skips
   a vertex's self-collapse destination. The no-edit forced-migration regression
   witnesses immediate complete flags instead of relying on a later publication.

No metric thresholds, target normalization, budgets, ID epochs, cache keys/quotas,
CFD donors or transfer/restart semantics change. Construction keeps exact positive
cells, topology and mandatory geometry/body/features. q/J remain unmeasured there;
existing valid certificates may survive a subsequent construction return.

## Disk-restart preparation (read-only during the live M6 attempt)

Do not start another CFD workload while the owned M6 run is live. The initial
`restart_flow_adap_00000.dat` is only an input-mesh artifact. Its binary/state
inspection (`initial_restart_state_inspection.json`) verifies61330points,23fields,
8byte scalars, exact payload size, all finite fields, positive density/pressure/
internal-energy density. This is NOT execution of a disk restart or transfer proof.

Reuse the publish→destroy driver→fresh reload pattern in
`UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp` (`envelope_restart`,~688),
plus `SaveStates`/`CheckPositiveFlow`/`SaveProjection` in that file. Existing disk
reload fixtures are unsteady Euler or RANS/BDF, not an actual accepted steady M6.
`integration_evidence/audit_native_unsteady.py::audit_3d_transfer` provides existing
independent tetrahedral P1 integral/positivity logic; do not claim its unsteady or
same-mesh restart evidence as coverage of this steady adapted case.

After accepted artifacts exist and the owned workflow is terminal, isolate a new
restart directory with the actual published adapted mesh, its `.native_ref` and
matching saved solution. Pin all three, the actual binary/libraries and config.
Use `RESTART_SOL=YES`, `ADAP_LOOP=NO`, actual `MESH_FILENAME`/format,
`SOLUTION_FILENAME` for the copied input base, and a separate `RESTART_FILENAME`
for continuation output. Steady restart uses `<base>.dat`; unsteady `_NNNNN`
suffix/`RESTART_ITER` semantics must not be copied blindly. Inspect actual final
artifact names rather than infer which state matches which mesh. Require an
actual successful finite/admissible CFD continuation from that isolated directory.
Compare reload states by coordinates when available, and inspect actual transfer
conservation diagnostics against backend tolerances. A post-CFD-step restart is
not the raw just-transferred state; do not compare their integrals as if no solve
intervened. Preserve the difference between transfer proof and restart usability.

## Required validation when computation resumes

- Build the coherent candidate with the established build helper and2–4workers,
  source/binary/helper/config pins,4TB compile-tmp, nice19 and single-threaded libs.
  Verify host resources and actual owned handles first; use the actual next identifier after checking existing receipts.
- Connected serial/MPI6 checks include the existing cache/reference/partition/
  phase-ownership/finite-split/first-height/joint-wall coverage plus
  `NativeBidirectionalLength3D`, `NativeConstructionHandoff3D`,
  `NativeConcurrentRepair3D`, `NativeInteriorPlacement3D`, `NativeQualityPass3D`,
  `NativeHeightOptimization3D`, and `NativeCoarsenQualityReturn3D`.
- The curved handoff regression uses actual cubic nets, not mapped planar error
  metadata. At the first complete split exit it requires zero size debt,
  measured residual geometry debt and no construction q/J measurement, then
  a complete quality census after forced ownership migration under MPI. Its intentional observer stop proves handoff and
  fail-closed private-state handling only; it is not curved mesh acceptance.
- The concurrent repair fixture captures the first bad vertex, requires the
  optimizer before topology candidates, still requires final strict admissibility
  and concurrent complete-star commits. Existing size-only and height fixtures
  retain their independent final-gate assertions.
- Return promptly to the complete two-event M6 at unchanged50k/gates. Compare
  actual phase entry, useful repair work, geometry/size returns, transition/cache
  costs and aggregate memory, not just the optimizer's trial success fraction.
  Both mesh events, normal conservative admissible transfer, continued CFD,
  published mesh/reference and an isolated disk restart are mandatory.

Risks to assess: earlier quality work may be undone by subsequent geometry edits;
optimizer reordering changes the accepted private mesh and may consume its budget
without finding a candidate; residual geometry may still require a different
coordinated construction strategy. An unproductive geometry-only generation can
still stop unresolved rather than manufacture another retry or claim feasibility.
Actual target failure, not passing fixtures, determines any next fix.
