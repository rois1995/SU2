# Suggestion outcomes — working ledger, final report pending

Requested2026-10-16: final report must map EVERY suggestion to worked/failed/
inconclusive/not attempted, quantify gains and costs, and explain negative results.
Do not count an unattempted item as a failure or a local replay gain as a complete
adaptation improvement. Include saved/rejected evidence, source/commit provenance,
accuracy, work, budget discrepancy, memory/communication and remaining scope.

| Suggestion | Current status | Evidence and limits |
|---|---|---|
| Match normalization to integration accuracy | Qualified replay; production pending | Shared controller passes25 assertions. Three original M6 cells agree exactly on1/4 ranks: target10, estimate10.10364 ±.22168,8 trials,56680 queries; correctly integration-limited at1%. No exact root or complete-field accuracy claim. Production normalization unchanged. |
| Allocate absolute error globally without gathering cells | Qualified on3-cell replay only | Absolute-error priority and collective failure handling agree on1/4 ranks;37 reductions/rank,23 cell estimates,133480 accounted cache bytes. A bad cell owned by one rank is rejected collectively. No all-cell gather; full frozen-field coverage remains incomplete. |
| Reuse integration geometry across scale trials | Worked in local replays | Cell88 2.2006s→.11325s at58112 accounted bytes;157 .12095→.02311s;329 .17431→.04663s. Original/changed-sensor-and-core warm/cold estimates identical. No complete normalization/adaptation speedup established. |
| Integrate BL correction separately | Partial local benefit; support-restricted geometry not implemented | Exact algebraic decomposition on the existing geometry kernel:329 queries6092→5644 (7.35% fewer), reference error.14414%→.01362% (10.6x lower).88/157 essentially unchanged; cold chord counts unchanged on all3. Does not resolve cold preparation or qualify full-field overlap/corner coverage. |
| More integration subcells | Negative evidence |329 error is nonmonotone with subdivision;88 work/time rises substantially. Raw indicator can underestimate157 by1.91x. Not a demonstrated cure. |
| Cheap scalar geometric-root screening | Withdrawn: no useful measured gain | Seed estimates unchanged on88/157/329;88 .2003s→.2090s. Extra code removed; seed_screen_v1 retains source/results/decision. |
| Shared policy in2D | Partially qualified foundations |8 existing composition controls pass95 assertions;2D production normalization unchanged.30P30N planar input prepared/validated, not adapted. |
| Actual refinement should ease integration | Inconclusive; construction blocked | Actual frozen M6 backend starts but rejects first-layer metric-edge integration. No accepted mesh exists to test reassessment after actual adaptation. |
| Reuse donor support/indexes across repair campaigns | Not attempted in this follow-up | Existing reuse must be inventoried; integration-geometry cache above is a different scope. |
| Extend shared-edge integration reuse | Not attempted | Deferred until complexity initialization is resolved. |
| Batch independent mesh transactions | Not attempted | Deferred; compare full lifecycle and accepted/rejected edits, not only kernel time. |
| Avoid unchanged failed searches | Not attempted | Must distinguish stable geometric rejection from incomplete support or temporary reservation conflicts. |
| Reuse transfer geometry across fields/histories | Not attempted | Inventory existing sharing; geometry/time-history identity and physical checks remain required. |
| Measured-cost partitioning/mid-remeshing migration | Not attempted | Include migration/index rebuild/return/solver rebuilding. Use useful work costs; MPI waiting is not itself useful work. |

Detailed receipts and current limitations: GOAL3_UNCERTAINTY_COMPLEXITY.md.
Deferred performance scope: POST_COMPLEXITY_QUERY_TRANSACTIONS.md.


## Additional performance accounting — 2026-10-17

Requested in the performance discussion; these are pending investigations, not
validated improvements. Preserve the distinction between cold preparation,
repeated queries, initial startup and recurring adaptation costs.

| Suggestion | Current status | Evidence and required assessment |
|---|---|---|
| Account for work outside the density callback | Gap identified; finer breakdown pending | Saved cell88 warm replay: total .113250s, density callback .007946s; about .105304s (93%) lies outside that callback. Wall SamplePoints is constructed before it. Separate wall search/sample construction, coordinate placement/exact containment, cache operations and integration bookkeeping; retain an unattributed residual instead of assigning it all to geometry. |
| Bound cold geometric preparation independently of density queries | Pending | A density-query ceiling does not sufficiently bound candidate-facet processing, distance-band root generation or nearest-wall searches. Measure preparation work and enforce explicit bounds with retained approximate contributions/unresolved uncertainty; do not silently omit BL mass or weaken mesh acceptance. |
| Qualify reuse under whole-mesh memory limits | Pending; three-cell cache gains only | Measure peak RSS and transient/allocator memory in addition to retained-byte accounting. Assess useful hits, evictions, rebuild work and geometry/field invalidation under the actual cache budget across cells, scale trials and adaptation events. Do not extrapolate three-cell reuse to the complete mesh. |
| Reconcile complete elapsed time with stage accounting | Gap identified; lifecycle reconciliation pending | Saved M6 construction-only replay: 348.216s overall versus 104.587s backend, with about 117.532s in initial CFD geometry. Attribute the remainder; separate one-time initialization from recurring adaptation/rebuild and launcher/shutdown costs. Reconcile nested stages without double counting; include repartition, migration, transfer, rebuild and output in the complete lifecycle. |
| Separate useful work, MPI waiting and measurement overhead | Pending qualification | Report elapsed critical-rank stages and CPU time separately from accumulated rank-seconds; do not subtract rank sums from wall time or sum unrelated per-stage maxima as an exact critical path. Check instrumentation overhead with bounded coarse/sampled measurements and distinguish host contention from algorithmic imbalance. |

Evidence: [cell88 reuse receipt](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/geometry_reuse_v1/cell88/geometry_reuse.json),
[M6 construction receipt](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/construction_first_trial_v1/result.json)
and [its stage log](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/construction_first_trial_v1/run.log).
These contended local observations identify investigation priorities; they do not
establish scaling or complete adaptation savings.


Additional structural experiment: global finite-feature chord distance envelope
passes20 strict/analytic controls but remains MIXED:88 cold1.997s->.5055s, while
fixed-budget reference error.7682%->4.9486%;157 similar accuracy/faster,329 faster
with worse accuracy. Extra nearest-feature cuts consume query budget. Whole-field
global error must decide usefulness; no lifecycle benefit claimed. Details and
remaining work: GOAL3_CHORD_ENVELOPE.md.
