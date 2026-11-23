# FP2-C1 continuation 1 on build647

The continuation uses the build647 production sources and existing SU2_CFD binary. RCFD1 had already restored the build647 tests in `f2781622a5` and rebuilt the test-driver target as build649. The continuation verified all three hashes and the live source pins. The original build649 source manifest used a different JSON key order. The 1,829 key/value entries and file hashes were identical, and serialization in build647 order reproduced its exact SHA256. [Order](../inputs/FP2_C1_CONTINUE_1.md) · [Build647 receipt](../build647/build_status.json) · [Build649 receipt](../build649/build_status.json) · [Source manifest receipt](../build649/source_pin_order.json)

## Reused build647 gates

| Gate | Result | Receipt |
| --- | --- | --- |
| C1 wall collapse fixtures | Five cases passed | [Terminal test](../fixtures/build647/wall/test_wait.json) |
| FX2 F1–F5 | Standing rows and digests matched | [Fixture comparison](../fixtures/build647/comparison.json) |
| FP1 and C1 fixtures, G1 | 18 FP1 cohorts and 375 / 114 / 228 G1 records | [Scoped fixture receipt](../scoped/build647/fixtures.json) |
| Labs N1, N6, N6P | Two independent trials per case with required rows and digests matched | [Lab acceptance](../cases/build647/lab_acceptance.json) |

The default proof uses the FX2 scoped driver and marks build640 as the source of the reference paths and scheduling calls. Exact G2 rows refer to the N14 baseline and GQ to N22. G3 compares with N14, and G4 and saved-state safeguards use the build640 filters and keys. The OFF full-run comparator uses build645. CORE uses the FP1 configuration and payload reference. [Scoped driver](../run_c1_default_scoped.py)

## Scoped default proof

The executed G2/GQ prefix gates passed in 275.060/769.670 s with exact decision rows, modeled storage, 4,120 bytes of fixed timer storage and the reference scheduling calls. The G3 fixture passed with 1,148 accepted splits, with 1,113 exact shared edits against N14 and 5,113 observer-adjusted entry work units in both runs. G4 passed the required 1/2/4-rank native 2D cohorts. All three saved-state safeguards passed. The scoped proof was complete when execution switched to RCFD2, and the continuation then resumed at the full OFF check. [G2 and GQ checkpoint](../scoped/build647/prefix/checkpoint_paths.json) · [Scoped receipt](../scoped/build647/scoped.json) · [G3, G4 and safeguard receipt](../scoped/build647/safeguards.json)

## Full default proof

OFF passed both events and isolated restart. The six files match the N21 W reference by SHA256 from disk. The FX2 build645 OFF comparison had zero exact differences. Native maxima were 1,801.488 and 1,070.726 s. [Run receipt](../G5/build647/OFF/run_receipt.json) · [Payload identity](../G5/build647/OFF/payload_identity.json) · [Exact row comparison](../G5/build647/OFF/full_comparison.json)

CORE passed both remesh events and continued CFD with native maxima of 2,012.729 and 661.089 s. All six on-disk payloads match the FP1 CORE reference. The default proof **FAILED** at the isolated restart. [Full workflow](../G5/build647/CORE/full647_run1/full_workflow_receipt.json) · [Observed payload identity](../G5/build647/CORE/observed_payload_identity.json) · [First failure receipt](../G5/build647/CORE/failed_attempt.json)

## Read-only diagnosis

The first failing value was the isolated restart exit code, **124** against the required 0. Its inherited command ran under `timeout 600`, and the outer supervisor finished after 617.692 s. The initial restart state was loaded and iteration 0 was recorded. The last solver phase logged Hessian recovery before the watchdog expired. The continued restart file and completed restart validator receipt were unavailable. The six ranks' sampled peak sum was 1,289,035,776 bytes against a 19,999,490,048-byte limit, and the CPU-process receipt passed. The restart preflight reported a one-minute load average of 23.39 and other busy processes. This contention may have contributed to the elapsed time. The run evidence establishes the watchdog expiry during metric computation. [Restart command](../G5/build647/CORE/disk_restart_full647_run1_event2/command.json) · [Restart wait](../G5/build647/CORE/restart_wait.json) · [Resource receipt](../G5/build647/CORE/disk_restart_full647_run1_event2/resource_peak_receipt.json) · [Host preflight](../G5/build647/CORE/restart_host_preflight.json)

Section 1.3 requires rollback on this failure. S2 and FP2-C2 are unevaluated. The S and FX2 S timing tables remain part of the continuation evidence.

## Mesh and native time

| Event | Run | Native maximum s | Cells | Points | Proof faces |
| --- | --- | --- | --- | --- | --- |
| 1 | FX2 S | 614.463 | 550027 | 113007 | 80650 |
| 1 | S | 606.213 | 467139 | 98606 | 79996 |
| 1 | S2 | — | — | — | — |
| 2 | FX2 S | 410.522 | 575733 | 116342 | 79274 |
| 2 | S | 387.779 | 464565 | 96767 | 77370 |
| 2 | S2 | — | — | — | — |

## Where the time is

Each event uses the rank with the largest native time in the sealed summaries. Maxima are computed independently over all six ranks, and the maximum sum is the maximum of rank sums. Core surface clocks include their nested sampling and placement work. Core quality includes V rounds when enabled. Surface-state build covers gathering, construction and checking of the state. Ordinary pass setup, exchange and write-back remain in Rest. Engine phases use nonoverlapping elapsed stage spans. The remesh ledger’s final-proof exclusive seconds are moved from the terminal engine phase to the separate proof row. Transfer is the remesh ledger’s transfer phase. Replacement and mesh output have distinct timer roots outside the native total. Zero marks a part that did not run, and — marks unavailable evidence.

### Event 1

| Part | FX2 S rank 1 | FX2 S max over ranks | S rank 2 | S max over ranks | S2 rank — | S2 max over ranks |
| --- | --- | --- | --- | --- | --- | --- |
| Core surface length step | 0.028 | 1.299 | 0.286 | 1.299 | — | — |
| Core surface distance step | 2.861 | 3.952 | 2.817 | 3.973 | — | — |
| Core length work | 5.077 | 20.822 | 18.799 | 29.499 | — | — |
| Core quality, including V rounds when enabled | 7.441 | 9.877 | 0.981 | 9.554 | — | — |
| Core surface-state build | 15.961 | 15.961 | 16.191 | 16.272 | — | — |
| Engine construction | 210.259 | 210.259 | 201.091 | 201.092 | — | — |
| Engine quality | 234.303 | 234.306 | 221.728 | 221.731 | — | — |
| Engine geometry repair 1 | 24.343 | 24.343 | 22.730 | 22.730 | — | — |
| Engine geometry repair 2 | 0.000 | 0.000 | 3.514 | 3.515 | — | — |
| Engine geometry repair 3 | 0.000 | 0.000 | 0.610 | 0.611 | — | — |
| Engine final proof | 33.361 | 33.361 | 35.300 | 35.300 | — | — |
| Transfer within the remesh root | 14.773 | 14.774 | 14.611 | 14.616 | — | — |
| Sum | 548.407 | 562.794 | 538.660 | 553.754 | — | — |
| Rest | 66.056 | 74.066 | 67.553 | 77.370 | — | — |
| Native total | 614.463 | 614.463 | 606.213 | 606.213 | — | — |

- S2 timing unavailable. Six-rank native summaries or final remesh ledgers are unavailable.

### Event 2

| Part | FX2 S rank 4 | FX2 S max over ranks | S rank 2 | S max over ranks | S2 rank — | S2 max over ranks |
| --- | --- | --- | --- | --- | --- | --- |
| Core surface length step | 0.069 | 0.288 | 0.240 | 0.240 | — | — |
| Core surface distance step | 2.726 | 2.726 | 1.051 | 2.613 | — | — |
| Core length work | 10.067 | 10.448 | 15.730 | 16.735 | — | — |
| Core quality, including V rounds when enabled | 1.931 | 1.931 | 1.404 | 2.057 | — | — |
| Core surface-state build | 15.595 | 15.675 | 15.648 | 15.693 | — | — |
| Engine construction | 212.803 | 212.803 | 187.658 | 187.658 | — | — |
| Engine quality | 111.899 | 111.900 | 112.393 | 112.393 | — | — |
| Engine geometry repair 1 | 0.000 | 0.000 | 0.000 | 0.000 | — | — |
| Engine final proof | 24.375 | 24.375 | 21.750 | 21.750 | — | — |
| Transfer within the remesh root | 15.033 | 15.042 | 14.645 | 14.645 | — | — |
| Sum | 394.496 | 394.496 | 370.520 | 372.890 | — | — |
| Rest | 16.025 | 21.268 | 17.260 | 20.246 | — | — |
| Native total | 410.522 | 410.522 | 387.779 | 387.779 | — | — |

- S2 timing unavailable. Six-rank native summaries or final remesh ledgers are unavailable.


## Rollback and close

`FP2-C1: default proof FAILED` was pushed as `935fa3ff9a`. The single rollback commit `6825979092` restored Common, SU2_CFD and UnitTests from FX2 commit `7e6953e721`. All 1,829 FX2 source pins checked before and after the two-target rebuild. The build completed in 433.180 s and both active binary hashes match FX2 build645.

| Artifact | FX2 build645 SHA256 | Rebuilt SHA256 |
| --- | --- | --- |
| SU2_CFD | `4f85c34986b98aa1520b29eae956a274a184f52fef0675f13f89c2da17d7d574` | `4f85c34986b98aa1520b29eae956a274a184f52fef0675f13f89c2da17d7d574` |
| test_driver | `abdd26035c5dfb849640527c45ee0649f267520b1f73b58309714f4f495f3322` | `abdd26035c5dfb849640527c45ee0649f267520b1f73b58309714f4f495f3322` |

The C1 test and production commits and the C2 step 0 test commit remain in history. The audit found no owned workloads. The continuation ended with the rollback label. S2 did not run, and the prerequisites for C2 and D remain unmet. [Build receipt](../restored645/build_status.json) · [Source and binary verification](../restored645/restoration_verification.json) · [Terminal audit](../restored645/terminal_audit.json)
