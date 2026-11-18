# Adaptation cut phase D

D1 and D2 passed and were pushed as `829e8db24b` and `4c6cbddbdb`. Build572 passed the two-event no-BL workflow and isolated restart. The standing safeguards passed.

## D1

Removed the unreachable non-streamed metric block and the unreachable repairing branches of the per-slot commit. The optimizer reservation, work vote and release still run in order. The live metric timer and field accounting remain in place. [Source readings](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/source_readings.md)

| Gate | Result on build571 |
|---|---|
| Build | 72.732 s controller wall, 1,783 verified source pins |
| G1 | 375 / 114 / 228 case records with zero changes in 769.252 / 14.583 / 14.702 s |
| G2 | Work 90,809,869 and counts 480 / 473 / 346 / 263 / 469 / 464 exact. Stopped at 312.230 s, sealed at 319.951 s. |
| G3 | 1,458 splits, all 1,113 common edits exact, adjusted entry work 5,113. Positive private cells with sensors, zero upper refinements and both memory gates passed. |
| GQ | All third quality windows, old integer rows, modeled bytes and timer calls exact. Stopped at 1034.570 s, sealed at 1042.311 s. |

G2 and GQ both preserved `fixedstorage=4120` and census delta zero. The six-rank sums of observed prefix process high-water marks were 6,100,688,896 B and 14,032,584,704 B. [Gate receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/D1/validation/acceptance_summary.json)

## D2

Moved the setup, round and slot orchestration into named helpers inside the original translation unit. The helpers retain the original timer, cavity and value lifetimes through the corresponding continuation. Each early completion still evaluates `Finish` inside its original scopes. The slot's cavity is cleared before the private batch joins.

The formatter passed on the new file and the changed range of the original file. `Adapt` occupies 58 formatted lines. The new file has 28 detected definitions and seven detected lambdas. Its longest definition is 58 lines and its longest lambda is nine lines. [Readability receipt](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/D2_draft/readability.json)

| Gate | Result on build572 |
|---|---|
| Build | 70.135 s, 1,784 verified source pins |
| G1 | 375 / 114 / 228 case records with zero changes in 768.300 / 13.611 / 15.759 s |
| G2 | Work 90,809,869 and counts 480 / 473 / 346 / 263 / 469 / 464 exact. Stopped at 312.308 s, sealed at 320.065 s. |
| G3 | 1,514 splits, all 1,113 common edits exact, adjusted entry work 5,113 and all physical gates passed. |
| G4 | Native 2D regression passed at 1 / 2 / 4 ranks in 6.472 / 6.461 / 6.474 s. |
| GQ | All third quality windows, old integer rows, modeled bytes and timer calls exact. Stopped at 1034.154 s, sealed at 1041.807 s. |

G2 and GQ preserved `fixedstorage=4120` and census delta zero. The six-rank sums of observed prefix process high-water marks were 6,236,643,328 B and 14,031,937,536 B. [Gate receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/D2/validation/acceptance_summary.json)


### Ten longest functions in the touched files

| Function | Formatted lines |
|---|---:|
| `MeshOptimizer::SmoothBoundary` | 423 |
| `MeshOptimizer::Smooth` | 320 |
| `MeshOptimizer::CoupledBoundary` | 232 |
| `Selection::Add` | 169 |
| `MeshOptimizer::BoundaryFlip` | 156 |
| `MeshOptimizer::EdgeRemoval` | 137 |
| `OptimizedRepairPatch` | 108 |
| `MeshOptimizer::ConeReplace` | 108 |
| `MeshOptimizer::Run` | 102 |
| `RefineOptimize` | 100 |

The inventory uses formatted source and lexical function boundaries. The long definitions in the table predate phase D. C §3 names the moved `EngineLegacyWall::Run` and its retained 182-line lambda as the written exception. [Inventory](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/D2_draft/phase_D_touched_function_inventory.json)

## Found during the cut

- The outer metric-normalization timer also covers field accounting on the live streamed route. Moving it with the deleted block would alter timer calls.
- The per-slot commit is entered only after the private repair path exits the slot body. Repair proposals still use the batch commit.
- The early-finish paths depend on several original lifetime boundaries. In particular, the scheduling reservation can finish while its timer is active, and a private slot can exit while leaving a batch to join.
- Prior findings about donor import, the acquisition throw and receipt ordering are retained in the whole-cut record. [Whole-cut source check](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/source_check.md)

## Readings used and helper corrections

- D1 retained the live metric timer and collective sequence. D2 used named scope-owning helpers with the original construction and reverse destruction order. [Reviewer ledger](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/QUESTIONS_FOR_REVIEWER.md)
- D1's first adapter stopped before building because the accepted manifest pinned the pre-amendment `AGENTS.md`. The corrected adapter verified the committed amendment and every other original pin. [Setup correction](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/D1_R3_amendment_pin/correction.json)
- The D2 draft generator initially aliased a function-local failure through slot state and retained the opening of a separately moved branch. The generator corrected both extraction errors. The syntax-only check then passed with the accepted compile flags. [Syntax receipt](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/D2_draft/syntax_final.exit)

## Unit tests that fail on the accepted base

G1 compares the case and failed-assertion tables with the adopted N14 reference. [Accepted-base failures](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/accepted_base_g1_failures.md)

## D3

The full workflow completed in 4140.805 s. All three event-one P6c files and all three event-two P7 files matched byte for byte. The full logs matched the old integer rows and 96 timer snapshots. Both events passed physical certification, transfer and continued CFD with zero coordinate error. [Full gate receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/G5/acceptance_summary.json)

| Event | Native max (s) | Points / cells | Job peak and sum of process peaks (B) | Min. timer coverage | Max. join difference | Owned proof faces / surface faces | Max. transfer residual |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 2188.708 | 146,676 / 726,442 | 14,017,724,416 | 96.4580% | 0.00006936% | 67,170 / 67,170 | 7.713e-14 |
| 2 | 1505.656 | 168,797 / 855,828 | 17,328,295,936 | 96.3409% | 0.00023658% | 62,912 / 62,912 | 9.052e-14 |

The isolated event-two restart reopened 168,797 points and 855,828 cells, then continued CFD. It took 233.893 s, with 0 coordinate error. [Restart receipt](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/G5/disk_restart_full572_event2/disk_restart_validation.json)

## Standing safeguards

| Safeguard | Elapsed (s) | Verified pins | Sampled process-tree RSS peak (B) |
|---|---:|---:|---:|
| `merit481` | 125.718 | 1,991 | 82,096,128 |
| `saved481` | 148.529 | 2,052 | 1,220,542,464 |
| `saved_current` | 145.178 | 2,052 | 1,216,245,760 |

The final audit verified 41 process identities terminal and all 1,784 source pins. The actual-host preflight found zero owned heavy processes. [Safeguard receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/safeguards/acceptance_summary.json) · [Terminal audit](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/phase_D_terminal_audit.json) · [Binary manifest](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_D/qualified_build572/manifest.json)

The first staging check found the solver’s trailing spaces in the two history CSV headers. The checkpoint includes lossless archives of both files and keeps the raw logs in their case directories.
