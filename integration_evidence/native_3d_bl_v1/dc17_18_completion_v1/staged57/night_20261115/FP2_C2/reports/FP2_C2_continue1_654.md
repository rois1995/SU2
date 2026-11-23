# FP2-C2 continuation 1, build654

**FP2-C2 continuation 1: fixtures FAILED; default path not yet proven**

Step 1 reproduced the build652 N1 result with unchanged production pins and solver. Build653 reported 5,937 collapses, four above-tolerance classifications and excess bins `0,2,1,0,1`. It named all four step-C faces on body 0, sheet 10154, marker 2, and localized the model-side witness to reference facet 11045. Two of the flagged faces were among the nearby boxes. The test-only diagnosis took 567.114 s at a starting one-minute load of 7.00 and was pushed as `2df5aeb78c`. [Build652 failure](FP2_C2_652.md) · [Step 1 report and rows](FP2_C2_continue1_diag_653.md)

## Bound and fixture results

Build654 compiled both targets in 116.149 s with no new warnings. Test 11 queries `EntityFaceError(Classify)` with cyclic smallest-ID-first corner ordering and the state-build canonical facet. Distance and entity binding remain the earlier filters. The candidate includes the post-writeback strict answer comparison, separate model-side lab gate, and query storage observations. [Signed order](../inputs/FP2_C2_CONTINUE_1.md) · [R4](../continuation1_source_check.md) · [Build654 receipt](../build654/build_status.json)

| Fixture group | Result |
| --- | --- |
| Serial, including (m) and (n) | 3,991 assertions in 8 cases passed |
| Rank border and keeper | 271 / 255 assertions passed |
| MPI exchange | Removals-only section failed on rank 0 |

The first differing result was `SurfaceCollapseRefusal=16 (Bound)` for the pinned S33 removal `(1,1,0) → (2,1,0)`. The fixture requires `0 (None)`. The exchange process exited 2 in 2.776 s. The helper watchdog did not fire. [MPI gate receipt](../fixtures/build654/exchange/attempt1/result.json)

## Read-only diagnosis

The S33 fixture builds an uncurved mapped reference with component mode disabled. In this configuration `MappedFacetReference` has no curved search index. `NearbySurfaceFacets` supplies only the hinted inherited triangle to the non-direct whole-face classifier. Substituting a keeper outside that triangle can leave the child exactly on the planar sheet while placing it beyond the queried triangle.

| Direction, vertex IDs | Inherited facet | Parent key | Child key | Keeper x | Inherited triangle x range |
| --- | ---: | --- | --- | ---: | --- |
| 5 → 6 | 1 | 0,1,5 | 0,1,6 | 2 | 0–1 |
| 6 → 5 | 13 | 2,6,7 | 2,5,7 | 1 | 2–3 |

For both examples the new vertex alone is at least one grid unit from the support supplied to the query. The tolerance is `1e-4`. The logs identify the failing section and returned enum. The examples were reconstructed from the pinned fixture's face enumeration. Test 11 passes the ordered inputs through the unchanged classifier, so the diagnosis found no wiring error. [Diagnosis receipt](../continuation1_fixture_diagnosis_654.json)

The candidate code and its failing fixture evidence are retained. The option-off cohort, FP1 cohorts, G1, six lab trials, and S on build654 remain unevaluated. The FP2-D prerequisite is unmet. [Terminal audit](../continuation1_final_terminal_audit.json)
