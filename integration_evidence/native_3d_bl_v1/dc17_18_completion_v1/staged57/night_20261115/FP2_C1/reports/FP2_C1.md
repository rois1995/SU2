# FP2-C part 1

S0 is `7e6953e721` (FX2 accepted, build645). The S0 lab driver was rebuilt as build646 with production unchanged and the build645 solver SHA256 preserved. [Order](../inputs/FAST_PATH_STAGE2_C1.md) · [Whole-item source check](../source_check.md) · [Build receipt](../build646/build_status.json)

## S0 lab reference

| Case | Controller s | Final owned cells | Final surface faces | Bad cells after V |
|---|---:|---:|---:|---:|
| N1 | 298.522 | 565,931 | 87,814 | 3,846 |
| N6 | 319.515 | 560,663 | 87,802 | 3,693 |
| N6P | 276.280 | 556,553 | 86,252 | 3,604 |

All three S0 hand-over measurements passed. The new core rows and the ordered owned-cell FNV-1a digests are sealed with the existing per-rank rows. [S0 lab receipts](../cases/build646/)

## Candidate fixtures

Build647 compiled both targets in 46.662 s with no new warnings. All five wall fixtures passed (1,021 assertions). The two accepted fixtures preserved the oriented wall faces and facet bindings. The flat, size and shape cases gave the specified refusals, with two directional size refusals in the size case. The narrow first-layer x pair was accepted.

The FX2 F1–F5 comparison passed and B1 kept its original timing gate, saving 16.013 s against F0. The inherited build640 cohort and all 18 FP1 cohorts passed. G1 matched 375 / 114 / 228 case records in 744.323 / 9.914 / 11.621 s. [Build receipt](../build647/build_status.json) · [Fixture cohort](../scoped/build647/fixtures.json) · [FX2 comparison](../fixtures/build647/comparison.json)

## Candidate hand-over lab

All six trials passed. Each pair matched all ordered integer fields and the final owned-cell digests, and all required pass-1 surface fields and sweep rows matched S0. N6 and N6P passed the cross-rank snapshot check. N6P used the pinned production partition. The lab found no final-pass edge below 0.01, no donor refusal and no unexplained surface classification.

| Case | Build647 controller s, trials 1 / 2 | Owned cells, S0 → build647 | Final surface faces, S0 → build647 | Wall-near collapses per trial | Bad cells after V, S0 → build647 |
|---|---:|---:|---:|---:|---:|
| N1 | 332.144 / 331.178 | 565,931 → 475,815 | 87,814 → 87,814 | 16,846 | 3,846 → 4,481 |
| N6 | 326.657 / 326.576 | 560,663 → 470,940 | 87,802 → 87,796 | 17,413 | 3,693 → 4,155 |
| N6P | 286.470 / 285.423 | 556,553 → 468,571 | 86,252 → 86,210 | 16,974 | 3,604 → 4,072 |

The per-rank, per-pass dataset contains the core counters, before/after V bad-cell classes, V counters, edge bins and exact shortest-edge records, and surface and modeled-storage rows. The lab prints a case-wide modeled peak for each rank, carried beside that rank's pass rows. [Lab comparisons](../cases/build647/lab_acceptance.json) · [Per-rank and per-pass data](FP2_C1_lab_647.json)

## Single full S run

Build647 passed both adaptation events, continued CFD, and the isolated restart. The full controller took 1,302.161 s and the restart controller took 152.951 s. All six candidate output hashes were independently verified on disk. The candidate points, cells and proof counts are those of the changed mesh.

| Event | FX2 → C1 native s | FX2 → C1 final cells | FX2 → C1 final points | FX2 → C1 owned proof faces | C1 minimum timer coverage |
|---|---:|---:|---:|---:|---:|
| 1 | 614.463 → 606.213 | 550,027 → 467,139 | 113,007 → 98,606 | 80,650 → 79,996 | 98.092% |
| 2 | 410.522 → 387.779 | 575,733 → 464,565 | 116,342 → 96,767 | 79,274 → 77,370 | 98.785% |

The core committed 16,970 and 10,221 wall-near length collapses. The engine accepted 3,936 and 3,884 collapses, versus 4,833 and 5,088 on FX2 S. Its maximum elapsed clocks at the collapse boundary were 156.611 and 170.863 s, versus 161.125 and 191.803 s, including engine setup. Rounds there rose from 8,882 to 12,593 and from 5,319 to 6,503. The full report contains per-rank construction and quality clocks, each geometry-repair span, the quality-entry q/J rows and the six output hashes. [S measurements](../G5/build647/S/S_report.md) · [S receipt](../G5/build647/S/run_receipt.json)

**FP2-C1 S passed.** The default proof and same-binary full repeat remain pending. The next queued item is the separate C2 step 0 measurement.
