# Assessment of supplied M6 findings (frozen build166)

Scope: source inspection and preserved164/166receipts. No source edits/tests for
this assessment. Full accepted Euler50k→BL/MPI→RANS500k goal persists. The active
next milestone is shared geometry-query reuse plus boundary smoothing; unrelated
algorithm/policy changes stay backlog. Confirmed correctness risks remain acceptance
obligations, not performance claims. This note does not claim current M6 acceptance.

## Directly useful to the active geometry/smoothing milestone

- One repair candidate per rank per round is confirmed: Engine3D:2630–2645 uses
  slots=1 for concurrent repairs; independent ranks can commit concurrently. This
  explains exposure to a slow geometry query, not absence of parallel execution.
  164had164quality rounds/901local quality commits (~5.5/round);166has158/928.
  A2s sample with75.5%inclusive Allreduce establishes sampled waiting only, not a
  phase-wide75%overhead figure. Candidate cost/debt per rank is not equalized by
  the one-slot rule. Reuse/query-efficiency work addresses the measured straggler
  first; do not launch an unrelated scheduling framework.
- Oldest(epoch,feature) queue ordering is confirmed (Engine3D:302–308,650–692).
  The optimizer debt priority acts inside its imported cavity, not global queue
  ordering. Useful coverage concern, but a debt-only queue could starve difficult
  geometry/heights; retain fair invalidation/service until measured reprioritization.
- Scope rebuilding is directly confirmed in166rank0atqualityentry:4859misses,
  840hits,907.787scope-build seconds inside1299.877upper-query seconds. Quality
  rank0adds1427.877upper seconds,493.183scope-build seconds. Timers overlap; do not
  sum them. Per-facet triangles are reused, then scopes concatenate them and rebuild
  TriangleUnion (AffineReference.cpp:1694–1720). Investigate reusable prepared
  subtrees and exact support-scoped queries, plus unchanged-face/star reuse.
- Boundary smoothing candidate repetition remains part of the same geometry path.
  Preserve position-dependent metric/geometry reevaluation; measure trials,
  rejections, certificate calls, accepted moves and whole-patch debt/time. No blind
  trial-count cut or boundary immobilization. Interior smoothing measured separately.

## Confirmed with narrower scope / hypotheses

- The observed bidirectional path performs one collapse pass before splitting
  (Engine3D:2487–2540). Rawshort edges<.9are not necessarily legally collapsible:
  blocked directions, forbidden flags, topology/geometry/metric/height admission
  apply. Final metric size gate is upper edge<=1.8; there is no symmetric lower
  .9gate. Extra points causing the bad cells is unproved. A larger coarsen campaign
  is not justified from short-edge counts alone; record as backlog hypothesis.
- Current M6 uses a startup CFD field: ITER1/ADAP_FLOW_ITER1, CL~.059304.
  This is the endorsed functional-loop configuration, not a converged aerodynamic
  qualification. A later accepted loop cannot be reported as flow-converged M6
  accuracy. Do not change the frozen comparison config mid-attempt.
- Target drift concern is substantiated at mechanism level: PublishMetrics and
  TransportNodalMetrics interpolate new node tensors from current cavity values;
  CurrentMetricSource::ImportCells constructs current-nodal donor cells (MetricTransport
  header:131–152,267–300). Engine field construction passes current_nodal=true;
  FrozenField::Query atField3D:536evaluates that cavity. FrozenMetricIdentity alone
  does NOT preserve the immutable Eulerian piecewise field after topology changes.
  Constant/global-affine examples can hide this. Measure nonconstant-field drift
  and resolve the required target semantics before any strict frozen-field claim;
  this is an acceptance risk, not yet a quantified cause of the current geometry
  cost or bad-cell population. Original conservative CFD donor state is distinct.
- Non-composed splitting uses a plain midpoint (Engine3D:2990–3052). BalancedSplitPoint
  is inside the composition condition; physical midpoint can be its fallback. No
  Euler/RANS label dispatch occurs. Effect on M6 is not measured; keep policy change
  out of the active query-reuse milestone unless a demonstrated blocker requires it.
- Tensor interpolation is component-wise arithmetic, not log-Euclidean
  (Field3D:53–60). This is a real numerical-policy choice, not automatically wrong.
  Compared with log-Euclidean interpolation, crossed anisotropy can yield stronger
  metric norms/smaller physical target sizes; do not loosely call that larger
  physical cells. No quantified M6 impact has been established here.
- Generation1has mandatory geometry validity/authority/topology/height checks.
  The bypass atEngine3D:2918–2921skips only fitting-excess descent during entity
  construction, intentionally allowing valid intermediate geometry debt. Final
  full two-sided GeometryCertificates remain at1280–1288. The debt rise is evidence
  to inspect, not proof that geometry has no guard or that final gates were removed.

## Restart / failure semantics and fail-closed behavior

- Adaptation loop cycle numbering restarts at0 (SinglezoneDriver:497+), and
  SetAdaptationCycle resets output basenames with_adap_cycle(COutput:385–395).
  Reusing output basename/directory can overwrite earlier checkpoints, potentially
  the input when its numbered name is reached. Collision is conditional on paths;
  the isolated steady restart must use ADAP_LOOP=NOand distinct output names.
  Record adaptation-loop resume collision as a separate correctness risk; do not
  dismiss it because an isolated restart avoids it.
- Rejected remesh consumes its requested cycle: ReplaceMesh failure retains the
  CFD mesh but the for-loop advances regardless (SinglezoneDriver:548–570).
  Two requested events therefore do not imply two accepted events. Current helper
  demands actual complete rank records and published artifacts; authorized stop
  prevents an unchanged retry after first-event failure. Full fresh retry required
  by the present driver if event1failed; no adaptive rollback/resume capability claim.
- Debt OldMerit returns-INFon invalid/unmeasurable old q/J, and Progress refuses it
  (Engine3D:4699–4728). This is deliberate fail-closed behavior, not refusal to fix
  merely poor finite quality. Do not bypass unresolved measurements. Legacy absolute
  MinScore lacks the same local Valid guard; whether reachable depends on old-patch
  invariants and remains a deferred targeted check.

## Timing interpretation

164update~2512.8seconds is real enclosing update time, including16-cell measurement
batches, cache/queue work, admissions and votes. Donor timing is subtracted via the
Timer enclosing argument; it is not all measurement-exchange cost. Other is a
residual of named phase timers (Engine3D:1354–1365), not a measured pure MPI category.
Election timers include waiting behind uneven geometry as well as protocol work;
1873seconds cannot be labelled pure barrier or protocol CPU from the name alone.
Surface/interior timings and lane-retirement counters identify different work and
constraints. Subtracting107from2024does NOT establish1900seconds wasted. Host CPU
service also varies and is preserved. Use representative query phase counters and
matching actual milestones; no additive accounting of overlapping timers and no
new baseline campaign.
