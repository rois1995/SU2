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
| Integrate BL correction separately | Not attempted | Deferred alternative; overlapping support and corners still need coverage. |
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
