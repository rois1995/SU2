# M6 no-BL cause confirmation — 2026-11-14

**1 — Confirmed for the first-collapse upper-work criterion.** One diagnostic build and one short six-rank run; no production fix.

## T1 — first collapse pass

| Quantity | Build 293 | Build 494 | Diagnostic test |
|---|---:|---:|---:|
| Upper-bound work, six-rank sum | 54,229,713 | 1,034,769,513 | 78,172,576 |
| Upper-bound calls, six-rank sum | 19,905 | 26,642 | 26,051 |
| Accepted collapses, six-rank sum | 2,495 | 2,496 | 2,495 |
| Collapse exit, rank 0 (s) | 194.723054 | 1410.686670 | 247.375978 |

## T2 — rank 3

| Quantity | Build 293 | Build 494 | Diagnostic test |
|---|---:|---:|---:|
| Proof seconds | 28.654482 | 934.259092 | 46.316131 |
| Proof work | 4,555,668 | 830,252,222 | 8,592,293 |
| Accepted collapses | 263 | 263 | 263 |

## T3 — verdict

**1 — Confirmed.** The diagnostic upper-bound work is at most twice build 293, meeting the brief’s criterion. Work is 1.4415× build 293 (limit 2×), and 7.5546% of build 494. The first-pass wall time remains 1.2704× build 293; this does not establish identical performance or a full adaptation.

## T4 — accepted edits

Counts match build 293 globally and on every rank: 480/473/346/263/469/464. The logs do not emit an accepted collapse-ID list; same-ID equality cannot be established.

Only the isolated BP `tighten` lambda was restored to the parent of 76f10c8310: classification at 2× tolerance instead of ErrorMagnitude above 1×. All other build 494 objects were reused unchanged; compiler/flags match GCC 13.3.0 and recorded build 494 commands (path substitutions only).
Test binary SHA256: `a6f93c42479b6f8c6a3eb6692c8f6dfb786adc7ccdde6647351c7bb9a279eb81`.
The watcher captured all six records by 351.35s from launch, then intentionally sent SIGTERM (outer exit −15). All owned handles were verified terminal; the 1500s fallback was not reached.
Load receipts every ~10s: launch 2.59; sampled min/median/max=2.70/8.55/9.82 (N=36). No profiler; configuration, final acceptance gates and resource limits unchanged.
**Not a fix:** this diagnostic undoes the build 481 BL surface-stage correction. The test source and binary remain isolated/untracked and must not enter production; the branch BP, pinned 494 binary/build inputs (803), and source pins (1539) stayed unchanged. BL and final mesh/transfer acceptance were not tested.
Evidence: `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/nobl_cause_confirm_20261114`: `tighten_diagnostic.patch`, `build_plan.json`, `baseline_build_inputs.json`, `build_status.json`, `run/provenance.json`, `run/collapse_records.json`, `assessment.json`, host/resource receipts.
