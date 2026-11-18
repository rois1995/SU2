# Adapt cut phase C — 2026-11-18

The C1–C11 cuts passed their scoped gates and were pushed to `codex/native-3d-bl`. The integrated cut at `167e353ca7` passed G3, G4 and GQ on build569. The same phase C qualification included the M27 checkpoint and one full no-BL workflow.

## Scoped identity checks

| Cut | Change | Checkpoint | G1 serial / 2 / 4 ranks (s) | G2 stop / sealed (s) | GQ stop / sealed (s) |
|---|---|---|---:|---:|---:|
| [C1](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C1/validation/acceptance_summary.json) | Round-head decisions | `f7017771e8` | 771.316 / 12.622 / 15.802 | 312.308 / 320.173 | — |
| [C2](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C2/validation/acceptance_summary.json) | Round acquisition | `c7b7664853` | 771.242 / 13.551 / 14.796 | 317.360 / 325.127 | — |
| [C3](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C3/validation/acceptance_summary.json) | Repair batch commit | `1a2fab4b91` | 772.228 / 13.590 / 15.802 | 312.365 / 320.190 | 1020.283 / 1028.095 |
| [C4](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C4/validation/acceptance_summary.json) | Per-slot commit | `7e101fcb43` | 768.203 / 14.613 / 13.683 | 312.200 / 319.916 | 1029.354 / 1037.076 |
| [C5](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C5/validation/acceptance_summary.json) | Slot acquisition | `227187f952` | 759.244 / 13.561 / 14.735 | 307.181 / 314.872 | 1019.021 / 1026.807 |
| [C6](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C6/validation/acceptance_summary.json) | Trial dispatch | `4a8ad57dbd` | 769.299 / 15.103 / 14.752 | 307.209 / 314.901 | 1018.963 / 1026.750 |
| [C7](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C7/validation/acceptance_summary.json) | Candidate checks | `bb5cac5265` | 759.413 / 12.695 / 16.815 | 312.248 / 319.971 | — |
| [C8](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C8/validation/acceptance_summary.json) | Private measurement and validation | `f3beeb21d8` | 773.184 / 14.549 / 15.787 | 332.443 / 340.210 | 1039.661 / 1047.366 |
| [C9](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C9/validation/acceptance_summary.json) | Legacy wall construction | `c0171bf783` | 767.213 / 13.574 / 14.427 | 312.211 / 319.888 | — |
| [C10](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C10/validation/acceptance_summary.json) | Repair ladders | `e3444aa51d` | 759.426 / 12.548 / 14.772 | 312.160 / 319.851 | 1023.988 / 1031.644 |
| [C11](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C11/validation/acceptance_summary.json) | Candidate evaluation | `167e353ca7` | 757.684 / 12.666 / 13.707 | 312.179 / 319.913 | 1044.215 / 1051.918 |

Each G1 comparison covered 375 serial, 114 two-rank and 228 four-rank records. G2 matched 90,809,869 work units and collapse counts 480 / 473 / 346 / 263 / 469 / 464. G2 and GQ matched every required integer, modeled-storage and timer-call field. Census call adjustments were zero and the timer summaries recorded 4,120 fixed bytes. GQ stopped after the third quality window on all six ranks.

The C11 sampled sums of rank high-water marks were 6,060,253,184 B for G2 and 14,033,412,096 B for GQ. The prefix samples cover the executed prefixes.

## Serial BL fixture and native 2D tests

| Cut | Accepted splits | Common edits exact | Entry work | Fixture (s) | Sampled job RSS (bytes) |
|---|---:|---:|---:|---:|---:|
| [C8](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/C8_G3_integer_rows.json) | 1,475 | 1,113 | 5,113 | 613.301 | 3,408,879,616 |
| [C9](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/C9_G3_integer_rows.json) | 1,442 | 1,113 | 5,113 | 613.450 | 3,408,781,312 |
| [C10](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/C10_G3_integer_rows.json) | 1,542 | 1,113 | 5,113 | 608.455 | 3,408,957,440 |
| [C11](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/C11_G3_integer_rows.json) | 1,562 | 1,113 | 5,113 | 608.232 | 3,409,420,288 |

Each G3 run passed the 336-split minimum, the unchanged frozen input and target checks, positive private cells with sensors, zero upper refinements, and the modeled and job memory limits. Each comparison used the first N14 run with 1,113 edits and observer-adjusted entry work of 5,113. The linked reports show the other integer rows beside the reference. The fixture exercises a saved surface stage.

G4 passed native 2D engine, mesh and BL tests at 1, 2 and 4 ranks in 6.467 / 6.459 / 6.499 s. [G4 receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/C11/validation/g4_case_paths.json)

## Readability

The formatter passed on 12 changed files. The engine implementation was checked on the changed ranges. The formatted inventory found 314 private definitions and 64 private lambdas. `EngineLegacyWall::Run` (1,251 lines) and its retained 182-line lambda are the written move-as-is exception. The 80-line deadline for `Adapt` is phase D. The inventory is lexical. [Source and format receipt](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/readability_build569.json)

### Ten longest functions in the changed files

| Function | Formatted lines |
|---|---:|
| `EngineLegacyWall::Run` | 1,251 |
| `Adapt` | 845 |
| `MeshOptimizer::SmoothBoundary` | 419 |
| `MeshOptimizer::Smooth` | 320 |
| `MeshOptimizer::CoupledBoundary` | 232 |
| `Selection::Add` | 169 |
| `MeshOptimizer::BoundaryFlip` | 156 |
| `MeshOptimizer::EdgeRemoval` | 137 |
| `OptimizedRepairPatch` | 108 |
| `MeshOptimizer::ConeReplace` | 108 |

## Found during the cut

- The donor-source import still ignores `requests`. Construction deferral still has its original uncaught throw. The receipt sort depends on input order for ties.
- The public observer API does not require rank-wide callback presence. Ownership uses measured cost and retained geometry. The World counters omit direct all-reduces.
- The donor import and frozen-field variants preserve distinct retry, call, ownership and timing scopes. The slot and batch commits keep their different conflict rules. The legacy wall keeps all three expand-cavity paths. [Whole-cut source check](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/source_check.md)

## Readings used and helper corrections

- Used the work order §2 references over historical drafts. The G3 prefix is against the first N14 run. G5 uses event-one P6c and event-two P7 file hashes. The complete integer and timer comparison uses the sealed N14 log, which carries the adopted wall-law accounting.
- C1 kept B02 before the lane timer. C2 kept B35e–g under one Topology timer. C3 kept the work observation before the first post-slot collective. C6 borrowed the live per-slot imported cavity. C9 moved E18–E24 together with the three original reservations. [Append-only reviewer questions](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/QUESTIONS_FOR_REVIEWER.md)

- C1 corrected the draft token comparator to strip every generated alias. C2 corrected a progress marker in its acceptance reporter and reused both sealed numerical results. The C1/C2 alias paths were corrected for later short gates.
- The C8 draft compile revealed that the original reuse map must remain mutable. The draft preserved its mutable reference and passed the rerun. A later C8 G3 prelaunch wrapper found three aliases already published by G1. The replacement uses them only when they match the sealed build exactly. The first wrapper never launched a fixture. G3 resumed on build566 and the passed G1 and G2 evidence was retained.
- C11 retained pre-cut formatting in three rewritten call sites. The whitespace-only draft correction passed the changed-range formatter. The first draft, updated pins and correction record are preserved.

## Unit tests that fail on the accepted base

G1 matched the accepted-base failures, including their printed values. [Case and assertion table](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/accepted_base_g1_failures.md)

## M27 and G5

M27 was implemented in `4787358e7a` on build570. It passed G1, G2, GQ, G4 and the physical and identity parts of G3. G2 and GQ stopped at 312.184 and 1039.089 s. Its G3 fixture accepted 1,523 splits, matched all 1,113 common edits and matched adjusted entry work of 5,113. The build produced no warnings in the touched files. [Source check and readings used](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N17/source_check.md) · [Scoped receipts](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/M27/validation/acceptance_summary.json)

The wall-query and smoothing observations came from the same G5 run. All twelve event and rank table rows recorded zero observer failures. The report retains the prescribed counts, work, durations, rank ranges, D9 differences, final memo values and original lines. [M27 report](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N17/reports/M27_build570.md) · [CSV](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N17/reports/M27_build570.csv) · [Raw lines](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N17/reports/M27_build570.log)

G5 passed in 4058.303 s. All six payloads matched their SHA-256 references. The complete log matched the old integer rows and 96 timer snapshots. Both events passed the physical, quality, geometry, size, transfer and continued CFD checks. The certified cell counts matched the output, and both coordinate checks had zero error. [G5 acceptance and hashes](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/G5/acceptance_summary.json)

| Event | Native max (s) | Points / cells | Job peak and process-peak sum (B) | Min. timer coverage | Max. join difference | Owned proof faces / surface faces | Max. transfer residual |
|---|---:|---:|---:|---:|---:|---:|---:|
| 1 | 2181.430 | 146,676 / 726,442 | 14,004,133,888 | 96.5141% | 0.00009571% | 67,170 / 67,170 | 7.713e-14 |
| 2 | 1448.062 | 168,797 / 855,828 | 17,446,068,224 | 96.7599% | 0.00014840% | 62,912 / 62,912 | 9.052e-14 |

The isolated event-two restart reopened 168,797 points and 855,828 cells in 225.848 s and continued CFD. The reopened coordinates had zero error. [Restart receipt](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/G5/disk_restart_full570_event2/disk_restart_validation.json)

G5 preparation corrected two copied event helpers that resolved pinned dependencies relative to their old directory. The restart solver completed, and the post-only validator produced the missing receipt after the launcher’s ownership records were read using their nested identity fields. The M27 report reader now strips physical MPI prefixes for parsing and retains the original raw lines. The summary self-check uses the sealed event-two proof count of 62,912 after its first test expected the wrong count. The same solver and restart outputs served all of these checks.

## Shared-code safeguards

The build570 geometry-merit, saved-481 and current saved-target safeguards passed in 121.145 / 146.766 / 143.969 s. These use the qualified 492/v3 input and the accepted N14 runners. The executed first-collapse, saved surface and 2D checks are recorded above.

| Safeguard | Runtime (s) | Verified pins | Sampled process-tree RSS peak (B) |
|---|---:|---:|---:|
| `merit481` | 121.145 | 1,990 | 82,784,256 |
| `saved481` | 146.766 | 2,051 | 1,216,626,688 |
| `saved_current` | 143.969 | 2,051 | 1,216,499,712 |

The final audit verified all 41 recorded host process identities terminal and all 1,783 source pins. The host preflight found zero owned heavy processes. [Safeguards](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/safeguards/acceptance_summary.json) · [Terminal audit](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/phase_C_terminal_audit.json) · [Final acceptance](../integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11/phase_C/acceptance_summary.json)
