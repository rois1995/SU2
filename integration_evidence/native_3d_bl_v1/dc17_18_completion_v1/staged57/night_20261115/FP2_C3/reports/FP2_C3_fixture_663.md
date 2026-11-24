# FP2-C3 outward surface contact

The signed FP2-C3 order started from FP2-D S0 `0c6860fd3a`, build660. [Order](../inputs/FAST_PATH_STAGE2_C3.md) · [Whole-item source check](../source_check.md) · [S0 report](../../FP2_D/reports/FP2_D_660.md)

## Implementation and storage

The opt-in step C records the outward caps of each trial and checks the remote face screen, new/new pairs and new/kept pairs in that order before the existing geometry queries. The local index retains a static ADT and additions from earlier commits. Queries ignore old entries whose slots or vertex tuples changed and sort current candidates by slot. The pass-end check tests every owned cell against every physical-face owner whose closed box meets its box, before the partition shifts or state exchange. The same check runs on one rank.

Admission adds one local contact entry with its closed box per potential live cell. It charges full-tree prediction for the admitted cell limit, caller arrays at construction, serial query fronts and hits, both direction cap lists, current candidates and the fresh list through one commit past its rebuild threshold. Admission takes the greater of the tree construction and query peaks and uses checked arithmetic for the full pass. The retained remote rows, boxes, tree and reserved query growth remain in the caller baseline for the pass. The owner check and lab-only embedding check each have their own measured/modelled storage receipts.

## Build and focused fixtures

Build661 stopped during graph regeneration because Meson could not locate Ninja. Build662 used the existing repository Ninja and regenerated the graph. Its C++ contact file needed the existing line-element declaration. Build663 includes that header and built both targets in 48.121 s with zero warnings and unchanged source pins. [Build663 receipt](../build663/build_status.json)

Serial contact fixtures passed 7 cases and 9,648 assertions. The MPI group passed 3 cases on both ranks with 898 and 727 assertions. Fixture (c) uses the order's direct N1 fallback after the bounded search. [Serial receipt](../fixtures/build663/contact_serial/attempt1/result.json) · [MPI receipt](../fixtures/build663/contact_mpi/attempt1/result.json) · [Search and mixed fan derivation](../fixtures/design/search_contact_fan.md)

## Option-off and regression results

The FP2-D control on build663 reproduced all 18 cases and the exact S0 diagnostic counts. Its cylinder used 27 legacy splits and 15 directional splits, one marked sweep and one residual refusal. Its legacy chain control repeated 227 splits, 22 sweeps and 726 short edges. The corrected FP2-C2 serial, border and exchange fixtures preserved their Catch results, and the FP2-A/B option-off comparisons matched S0. No option-off log in the scoped comparison contained a contact row. These cases emitted no NATIVE3D_LOCAL_SURFACE rows, so the printed-zero check had zero rows to evaluate.

All 18 FP1 cohorts passed. G1 matched 375 / 114 / 228 case records in 748.269 / 10.154 / 9.966 s. [Fixture acceptance receipt](../fixtures/build663/c3_acceptance.json) · [Option-off comparison](../fixtures/build663/off_comparison.json)
