# Native 3D N6 cuts

The three N6 cuts passed and were pushed. Build578 qualified the integrated extraction. The shared no-BL workflow still uses the six P6c/P7 reference files. N6 compared full integer rows and timer calls with the accepted build574 T0 run. [Whole-item source check](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N6/source_check.md)

## Scoped gates

| Sub-step | Checkpoint and build | G1 serial / 2 / 4 ranks (s) | G2 stop (s) | G3 splits / exact common edits | GQ stop (s) |
|---|---|---:|---:|---:|---:|
| [Remesh](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/N6Remesh/validation/acceptance_summary.json) | `4946b2d126` / 576 | 777.415 / 13.842 / 15.956 | 317.525 | 1,434 / 1,113 | 1091.439 |
| [Optimizer](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/N6Optimizer/validation/acceptance_summary.json) | `cd2c46b73d` / 577 | 782.518 / 13.811 / 14.976 | 333.367 | 1,306 / 1,113 | 1061.200 |
| [Selection](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/N6Selection/validation/acceptance_summary.json) | `7ffb54c83e` / 578 | 771.375 / 14.756 / 15.836 | 317.610 | 1,368 / 1,113 | 1122.623 |

G1 matched 375 / 114 / 228 case records and failed assertions in every sub-step. G2 matched work 90,809,869 and counts 480 / 473 / 346 / 263 / 469 / 464. G2 and GQ preserved old integer rows, modeled storage, all timer calls, fixed storage 4,120 B and census delta zero. GQ matched all six third quality windows. G3 met every physical gate with at least 336 splits and observer-adjusted entry work 5,113. G3 is a clock-bounded saved surface fixture.

G4 passed native 2D with and without BL at 1 / 2 / 4 ranks in 6.689 / 6.690 / 6.630 s. [Native 2D receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/N6Selection/validation/g4_case_paths.json)

## Readability

The public Remesh entry is 50 formatted lines. The six source fragments keep their numerical checks and caller contracts. The formatted inventories found maxima of 74 lines for the Remesh helpers, 76 for the optimizer and 60 for Selection. New lambdas fit the 15-line limit. N6 added no exception. The original move-as-is exception for the legacy wall remains part of phase C.

| Longest new or moved definition | Formatted lines |
|---|---:|
| `MeshOptimizer::BoundaryCollapse` | 76 |
| `ApplyCorrection` | 74 |
| `BoundaryReplacement` | 71 |
| `MeshOptimizer::RemoveInteriorRing` | 71 |
| `MeshOptimizer::Smooth` | 70 |
| `PlaceCone` | 70 |
| `MeshOptimizer::FlipBoundaryRing` | 68 |
| `MeshOptimizer::ShellBoundary` | 68 |
| `MeshOptimizer::TryCell` | 66 |
| `MeshOptimizer::BoundaryProjection` | 65 |

## Found during the cut

- The public remesh entry supplies the failure caller name. Its timing summary reads the report clock before releasing the summary buffers.
- The optimizer can fail a late binding check after mutation. The cut kept that order. The interior and boundary ring operators use the same scoring and traversal loops, with independent memoization, retry, certification and commit state.
- Selection reset leaves the blocked directions, blocked links and repair age alone. Migration swaps its explicit original member list. The traveling row types, bytes calculation and repair update order stay in place.

## Readings used and helper corrections

The Remesh helpers retain the public caller name and report clock boundary. The optimizer uses two named helpers for the identical ring loops. Selection keeps a single owner with two value members. The Selection patch generator initially resolved the repository one directory too high, then located the formatting file from its ancestors. Both engine drafts passed syntax-only compilation with the accepted flags in private overlays. The no-launch self-check first resolved the historic safeguard module by name, then loaded the N6 entry directly. The first optimizer GQ wrapper stopped at the existing free-space preflight before numerical launch. We retained the passed build, G1, G2 and G3, verified lossless archived copies of the phase C and D backup binaries and ran only the missing GQ. [R13 choices](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/QUESTIONS_FOR_REVIEWER.md) · [Selection draft review](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N6/drafts/selection/REVIEW.md) · [Adapter self-check](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_N6/adapter_self_check.json) · [Disk recovery](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N6/R3_disk_preflight/space_recovery.json)

## G5 and isolated restart

The two-event run completed in 4041.079 s. The three event-one P6c files, the three event-two P7 files, the old integer rows, and 96 timer snapshots matched. Both events passed certification, transfer and continued CFD. [G5 summary and six SHA-256 hashes](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_N6/G5/acceptance_summary.json)

| Event | Native max (s) | Points / cells | Job peak and process-peak sum (B) | Min. timer coverage | Join difference max | Owned proof faces / surface size | Max. transfer residual |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 2173.311 | 146,676 / 726,442 | 14,014,275,584 | 96.4000% | 0.00007029% | 67,170 / 67,170 | 7.713e-14 |
| 2 | 1434.914 | 168,797 / 855,828 | 17,365,278,720 | 96.7297% | 0.00011054% | 62,912 / 62,912 | 9.052e-14 |

The isolated restart reopened 168,797 points and 855,828 cells with zero coordinate error. It passed continued CFD in 234.819 s. [Disk restart validator](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_N6/G5/disk_restart_full578_event2/disk_restart_validation.json)

## Standing safeguards and terminal state

The geometry merit, saved 481 and current saved-target checks passed in 119.201 / 146.687 / 142.945 s. All required saved payloads and logical rows matched. The audit verified 271 terminal process identities and 1,790 source pins. The actual-host preflight found zero owned heavy jobs. [Safeguard receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_N6/safeguards/acceptance_summary.json) · [Terminal audit](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_N6/terminal_audit.json) · [Binary manifest](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_N6/binary_manifest.json)

## Unit tests that fail on the accepted base

G1 uses the adopted N14 cases and exact failure values. [Case, file, line and printed values](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/accepted_base_g1_failures.md)
