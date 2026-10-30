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
Initial CFD completed and wrote input-mesh restart00000; metric construction is
underway. Sources/binary/config/helpers are frozen. Inspect actual handles before
edits/restarts. All strict
Euler/transfer/restart requirements remain, then BL/MPI and RANS500k.

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
