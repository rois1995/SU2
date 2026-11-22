# FP2-B correction 1 · build633

The archived order matches SHA256 `b735b39ea86da2b780d8108e79782466f9d8eb5f492b570b87c9b3b160455b45`. It replaces the two Step D rules from FP2-B §4. [Archived correction](../inputs/FP2_B_CORRECTION_1.md) · [SHA256](../inputs/FP2_B_CORRECTION_1.sha256)

## Scope and source check

R4 was sealed at `457822008c` before production edits. The list records the path traversal, direct and terminal marks, the fixture observer, the fixture-before-lab sequence and replay logging scope. The per-sweep set of feature edges whose requests followed a terminal edge is outside `SurfaceArrayBytes`, as ordered. The path-only flag stays in the existing mark and its existing size term. [R4 source check](../source_check_correction_1.md) · [Reviewer questions](../../QUESTIONS_FOR_REVIEWER.md)

The Step D decision and its counters account for the full production diff since `535d311882`. Gate E remains at build630. Changes.cpp and Pass.cpp match the build632 manifest.

| Volume source | SHA256 |
|---|---|
| CNativeLocalCore3DChanges.cpp | `94b0a5acabdc7f3cc142b1267edffa88da713f64c954913451b696881ad5ceed` |
| CNativeLocalCore3DPass.cpp | `752bc3a7d7db3caee98064da8e5396ff83c7b8f0cb4776fcb30a1a68feffc21b` |

[Gate E source audit](../correction1_gate_e_sources.json) · [Build630 Gate E](FP2_B_632.md#a1-volume-rounds-and-gate-e)

## Build633

Both binaries built in 46.863 s with zero new warnings.

| Binary | SHA256 |
|---|---|
| SU2_CFD | `1c65f5cfa834c748c3a68db56a332fbe5ec5bd0aedd474975634e86d6e56da5a` |
| UnitTests/test_driver | `bc183e741f993232b7504dad13e0dc3c787526abec0dc4d086a40f1c0bbd3176` |

[Build633 receipt](../build633/build_status.json)

## Fixtures and G1

- The three corrected skewed-box cases passed with 141 assertions. The unlocked and feature requests accepted 3–6, 2–3 and 0–2 in that order. The border and refusal cases returned `neighbor_locked`. [Corrected box fixtures](../scoped/build633/locked/fixture_status.json)
- The FP2-A box and cylinder cohort passed 797 assertions in four cases. The cylinder observer counted 27 accepted Step D splits and zero violations. Its two work-list variants retained integer parity, including the new report counters. [FP2-A fixture receipt](../scoped/build633/box/fixture_status.json)
- A1 passed 105 assertions. All 18 inherited FP1 cohorts and the remaining requested FP2-A fixtures passed, including the frozen IN1 case and both MPI exchange sizes. [Fixture cohort receipt](../scoped/build633/fixtures.json)
- G1 matched 375, 114 and 228 records in 761.056, 10.138 and 9.797 s for one, two and four ranks. [Serial](../scoped/build633/G1/serial/comparison.json) · [MPI2](../scoped/build633/G1/mpi2/comparison.json) · [MPI4](../scoped/build633/G1/mpi4/comparison.json)

R3. An inherited Python import had already cached a different `run_scoped` module. The first fixture attempt exited before any workload. Loaded the existing FP2-B scoped runner by its exact path, then used the original build633 binaries and pinned the corrected runner in the fixture receipt.

## Corrected hand-over gate

| Run | Seconds | Cells after pass or shift | Bad cells by class (a/b/c/d) | Bad total | Edges below 0.01 | Smallest metric edge | Pass face sums and limits |
|---|---:|---:|---|---:|---:|---:|---|
| N1 | 292.431 | 548,127 | 1,142 / 1,449 / 0 / 114 | 2,705 | 0 | 0.027364967690064603 | 76,736 ≤ 95,413 |
| N6 | 320.498 | 542,867 | 1,168 / 1,324 / 6 / 14 | 2,512 | 0 | 0.027364967690064603 | 76,612 ≤ 95,305 / 76,772 ≤ 95,422 |

All cells were exactly positive and the owned-geometry checks passed. The six N6 snapshots agreed at 26,697,320 bytes. Geometry qualified by reason accounting in both runs. N1 accounted for 1,204 above-tolerance faces (100 refused, 1,104 locked-neighbor). N6 accounted for 1,203 (100 refused, 5 border, 1,098 locked-neighbor). Both had zero unexplained faces. N6's largest post-surface star was 162, the most distance sweeps was 23, and the longest rank pass took 27.068 s. The V-round limit was clear on every rank and pass.

The separate N1 observer passed in 288.250 s with zero longest-edge violations and exactly reproduced the unwrapped N1 integer surface fields and `bad_out`. The corrected six-rank run showed no subthreshold edges, so the conditional diagnostic replay was unnecessary. [Comparison with build632](FP2_B_633_correction_1_lab.md) · [Historical hand-over comparisons](FP2_B_633_lab.md) · [N1 invariant receipt](../cases/build633/N1Invariant/measurement.json)
