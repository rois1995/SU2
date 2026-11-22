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

## Part B · FP2-B ON: FAILED

S1 ran first on the same build633 solver with the authorized config SHA256 `07b91a94996bf3cb48352041d8e1643ffe17a61bed07432085cfcf87b1725382`. Event 1 passed the native, physical, transfer, continued-CFD, timer coverage and join agreement. It published 552,946 cells, 112,400 points and 76,126 owned proof faces, equal to the surface size. Final job peak and sum of rank peaks were both 13,032,607,744 B. The maximum solver-reported native time was 1,667.908 s. The maximum remesh timer root was 1,667.955 s, with minimum named coverage 99.2429%.

| Event 1 output | SHA256 |
|---|---|
| adapted_mesh_adap_00001.su2 | `f6fd75c71a6a42b222aa2d56e9de8a789e357b6ebd5d9b1f370ca4d428313cbd` |
| adapted_mesh_adap_00001.su2.native_ref | `c59dabc30c39e2691bcedcef477fa4b3aaac66f601a175dbfa584d6e97c658a1` |
| restart_flow_adap_00001.dat | `5edce653ec7ac205d7679781c33a40cdd4f49ab5a1e9bc93c50a46f455dc61f9` |

### One read-only diagnosis

The first failed standing value was event 2 `complete=0` on all ranks against the required 1. The engine rejected the event at the improvement refresh with `Incomplete quality refresh edge measurements.` Rank 0's exit had `shape_complete=0` and 107,960 of its 108,211 cells shape-measured. The other ranks' exit shape censuses were complete. The measured subset at the event-2 exit gave 2,242 q failures, 2,150 J failures and zero geometry failures.

`EngineCensus::MeasureCell` asks `ValidateMetricPatch` for the remeasured shape and edges. If measurement fails before returning a complete result, validation catches the underlying exception. `Selection::RefreshQuality` then rejects the incomplete edge certificate. The retained failure message omits the underlying exception and cell ID, so the diagnosis establishes the rejecting stage and incomplete measurement without resolving the offending cell. Rank 0's first event-2 engine length row already reported a minimum metric length of `3.4973302189904171e-09`. The retained counters do not tie that edge to the refresh failure or a particular split.

The outer timer supervisor recorded `f3b_event2_timer_gate_failed` and stopped the rejected run. All six raw remesh ledgers cleared the coverage and join-difference checks, with minimum named coverage 98.3510%. The total timer gate failed because each rank had zero completed quality-window and quality-exit rows. The maximum event-2 remesh timer root was 260.762 s. Event 2 did not publish an adapted mesh, sidecar or restart.

| Event 1 engine census | Max engine clock (s) | q bad | J bad | Geometry bad |
|---|---:|---:|---:|---:|
| Length-collapse exit, shape unmeasured | 178.978 | — | — | 1,344 |
| Quality entry | 311.805 | 13,129 | 13,645 | 271 |
| Improvement window | 1,054.307 | 3,350 | 3,271 | 84 |
| Improvement exit | 1,522.415 | 0 | 0 | 0 |

The full comparison places the event-1 rows beside S1 and CORE of build627. It includes per-rank construction and improvement clocks, quality-entry q/J, geometry at length-collapse exit, the first engine minimum metric edge, length collapses and ownership migrations. The stage history preserves q/J/geometry and measured-complete flags over time. [Workflow comparison](FP2_B_633_workflow.md) · [Event 1 history](FP2_B_633_engine_event1.json) · [S1 failure receipt](../G5/build633/S1/failed_attempt.json)

The S1 standing failure triggers FP2-B §6.2. Isolated restart, post-S1 default proof, S2, repeat payload identity and the same-binary timing comparison remain unevaluated.

## Restoration and closure

The revert restores the paths changed by A1 `51647d8ecf`, A2 `535d311882` and correction 1 `beb1374ff8`. The build633 lab checkpoint was `ccd9d60593`. The production diff against `852e566d17` is empty and all 1,624 non-test source SHA256 pins match build627. The test tree matches the pre-FP2-B commit `bb352600c5`. Both binaries rebuilt without a new warning in build634. The solver SHA256 equals build627 and the test driver SHA256 equals build629. The host audit verified 111 process identities terminal. [Restoration audit](../rollback_build627_audit.json) · [Build634 receipt](../build634/build_status.json) · [Terminal audit](../terminal_audit_correction_1.json)
