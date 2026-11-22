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
