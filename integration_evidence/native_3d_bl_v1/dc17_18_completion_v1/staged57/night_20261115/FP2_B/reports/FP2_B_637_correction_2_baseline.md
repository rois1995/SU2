# FP2-B correction 2 · D1 partition baseline

The tracked production tree matched D1 `873eaf5a9e`. Build637 compiled test_driver only in 68.326 s with zero new warnings. The test driver SHA256 is `5f8d98163e2c0ed77b235d1bb7c826749c6aca608a1ed15d01e57feacc71861b`. Both six-rank lab runs completed under the original wrapper. [Build receipt](../build637/build_status.json)

N6P finished in 309.304 s. All six partition and pass-1 face pairs match D1. Rank 2 exactly reproduced the named shifted minimum `3.9070055295542924e-10` on 64572–64650, 362 edges below 0.01 and 138 below 1e-6. Rank 3 had zero shifted edges below 0.01 and reproduced the pass-1 minimum `5.4527443514081957e-10` on 74533–74593. At pass 1 rank 3 had 121 below 0.01, 87 below 1e-4 and 53 below 1e-6 in both transcripts. Its 0.1 bin was 2668 in N6P and 2682 in D1. [N6P receipt](../cases/build637/N6P/measurement.json)

The baseline helper originally required equality across every displayed field and recorded `reproduced=false` for N6P because the wider 0.1 bins and the final above-floor rank-3 minimum differ. The named reproduction values of §5.2 all match. The sealed original receipt preserves the wider comparisons. The corrected gate evaluator applies the named criteria and reports the broader comparison separately.

N7P finished in 456.941 s and is report only. The 552,946 input cells, six before-partition counts and 14,790 CFD partition cuts match D1. The first native partition difference is rank 0, 95,994 against 95,408, followed by 14,216 against 8,490 pass-1 faces. The pinned inputs contain the event-2 restart tensors. [N7P receipt](../cases/build637/N7P/measurement.json) · [Event-2 input pins](../inputs/N7P/pins.json)

A read-only check identified a geometry state difference. D1 event 1 exited with 24,805 prepared facet entries on rank 0. The next event reused the live reference. The v9 sidecar serializes the geometry and its binding state and reconstructs a fresh mapped reference on load. It contains no prepared facet cache. `WorkingWeights` takes a rank maximum of the cached piece counts and uses them in its geometry-work estimate, which feeds the graph partition. The saved inputs therefore provide no exact replay of those weights. The corrected lab will print the two weight components and cache occupancy to test that explanation. No source or input changes were used to force a match. [D1 report](../../FP2_B_D1/reports/FP2_B_D1_635.md)

## Observed and reference rank pairs

### N6P on build637

Wrapper completed gates True. Partition valid True. Full displayed-field reproduction False. Corrected gate applies False.

| Rank | Cells after partition | Pass 1 cells in | D1 cells | Pass 1 faces in | D1 faces |
| --- | --- | --- | --- | --- | --- |
| 0 | 48316 | 48316 | 48316 | 9423 | 9423 |
| 1 | 51155 | 51155 | 51155 | 9031 | 9031 |
| 2 | 56223 | 56223 | 56223 | 6040 | 6040 |
| 3 | 51202 | 51202 | 51202 | 6694 | 6694 |
| 4 | 50531 | 50531 | 50531 | 9634 | 9634 |
| 5 | 45932 | 45932 | 45932 | 8132 | 8132 |

| Rank | Source | Metric min | A | B | <0.01 | <1e-6 | <1e-4 | <0.1 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 2 | observed | 3.9070055295542924e-10 | 64572 | 64650 | 362 | 138 | 221 | 2604 |
| 2 | shifted | 3.9070055295542924e-10 | 64572 | 64650 | 362 | 138 | 221 | 2603 |
| 2 | seed | 3.9070055295542927e-10 | 64572 | 64650 | 362 | 138 | 221 | 2603 |
| 3 | observed | 0.027364967690064603 | 79447 | 366775 | 0 | 0 | 0 | 2690 |
| 3 | shifted | 0.027261040654623699 | 79453 | 366721 | 0 | 0 | 0 | 2719 |
| 3 | seed | 0.027261040654623701 | 79453 | 366721 | 0 | 0 | 0 | 2719 |

### N7P on build637

Wrapper completed gates True. Partition valid False. Full displayed-field reproduction False. Corrected gate applies False.

| Rank | Cells after partition | Pass 1 cells in | D1 cells | Pass 1 faces in | D1 faces |
| --- | --- | --- | --- | --- | --- |
| 0 | 95994 | 95994 | 95408 | 14216 | 8490 |
| 1 | 93730 | 93730 | 93521 | 14249 | 15690 |
| 2 | 98134 | 98134 | 94879 | 7337 | 14277 |
| 3 | 79171 | 79171 | 87449 | 14518 | 12452 |
| 4 | 87124 | 87124 | 86552 | 15915 | 14459 |
| 5 | 98793 | 98793 | 95137 | 9891 | 10758 |

| Rank | Source | Metric min | A | B | <0.01 | <1e-6 | <1e-4 | <0.1 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 | observed | 0.012308859004207421 | 21630 | 61619 | 0 | 0 | 0 | 2026 |
| 0 | shifted | 1.616941326830997e-09 | 115118 | 115130 | 23 | 10 | 16 | 4014 |
| 0 | seed | 1.6169413268309967e-09 | 115118 | 115130 | 23 | 10 | 16 | 4014 |

