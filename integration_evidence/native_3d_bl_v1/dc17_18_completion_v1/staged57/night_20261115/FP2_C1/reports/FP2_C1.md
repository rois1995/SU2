# FP2-C part 1

S0 is `7e6953e721` (FX2 accepted, build645). The S0 lab driver was rebuilt as build646 with production unchanged and the build645 solver SHA256 preserved. [Order](../inputs/FAST_PATH_STAGE2_C1.md) · [Whole-item source check](../source_check.md) · [Build receipt](../build646/build_status.json)

## S0 lab reference

| Case | Controller s | Final owned cells | Final surface faces | Bad cells after V |
|---|---:|---:|---:|---:|
| N1 | 298.522 | 565,931 | 87,814 | 3,846 |
| N6 | 319.515 | 560,663 | 87,802 | 3,693 |
| N6P | 276.280 | 556,553 | 86,252 | 3,604 |

All three S0 hand-over measurements passed. The new core rows and the ordered owned-cell FNV-1a digests are sealed with the existing per-rank rows. [S0 lab receipts](../cases/build646/)
