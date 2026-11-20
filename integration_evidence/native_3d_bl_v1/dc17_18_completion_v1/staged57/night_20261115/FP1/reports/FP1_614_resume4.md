# FP1 Resume 4, build614

The archived order matches SHA256 `742c936b94e08cb02badd7f3b77757395f2123075ebcffe442cbd064ab1f363f`. All six Resume 3 lab handles were terminal. The already-pushed lab checkpoint `0b4d3b2baf` fulfills §0 and contains both V2 receipts. [Order](../inputs/FP1_RESUME_4.md) · [Hash](../inputs/FP1_RESUME_4.sha256) · [Terminal receipt](../resume4/resume3_terminal.json)

## Part H6

The split tests the two half-edges and the midpoint-to-ring edges with the midpoint tensor in the existing double length formula. Each must be strictly below the old edge length times `1 - 1e-12`. A failure uses the existing Size count and occurs before any mesh change or ID consumption. The split candidate order, work-list logic and engine code match the previous checkpoint. [Whole-item source check](../resume4/source_check.md) · [Code scope](../resume4/scope.json) · [Reviewer questions](../../QUESTIONS_FOR_REVIEWER.md)

Build614 linked in 35.365 s with zero new warnings. The H6 ring passed 50 assertions using the production defaults. The old metric length was 1.87083. A midpoint spoke of length 2.1 caused one Size refusal, zero edits and byte-identical arrays. Moving its ring point closer produced a longest midpoint edge of 1.05 and one committed split. [Build receipt](../resume4/build614/build_status.json) · [Ring fixture](../resume4/build614_checks/H6/rank1/fixture_status.json)

All inherited fixtures passed. Both H5 boxes matched the full-scan Owned bytes and five edit counts. The second box now reached 7 collapse sweeps and 16 split sweeps in one call, compared with H's 38 and 23. F4.1 retained 10 and 4 sweeps and 944,672 Owned bytes. The archived G control, with H and H6 switches off, still matched 1,102,400 bytes. Part A reported 72 residual closed long edges under H4's block and zero eligible short collapses. No pinned mesh inputs changed. [Fixture receipts](../resume4/build614_checks/checkpoint_paths.json) · [Changed fixture rows](../resume4/inherited_fixture_changes.json)

The lab runner now uses a SIGKILL deadline in the isolated 1,200 s timeout group. This addresses the Resume 3 timeout overrun after SU2 acknowledged SIGTERM and kept computing. The harness checks cutoff and incomplete-audit parsing and saves per-kind and per-round timing and edit totals. [Runner](../resume4/run_lab.py) · [Input verification](../resume4/lab_input_verification.json)

## OFF identity gates

G1 matched the adopted cases and failure identities for 375 serial, 114 two-rank and 228 four-rank case records. Cohort times were 767.842 / 10.282 / 11.300 s. [G1 receipts](../resume4/G1_614/checkpoint_paths.json)

G2 passed at 241.778 s with 90,809,869 work units. Exact decisions, modeled bytes and timer rows matched under the corrected N14 comparator. Timer fixed storage was 4,120 bytes. [G2 comparison](../resume4/prefix/G2_comparison.json)

GQ passed at 716.584 s to cutoff. The corrected N22 comparison matched the exact decisions, counters and modeled bytes. Timer fixed storage was 4,120 bytes. [GQ comparison](../resume4/prefix/GQ_comparison.json) · [Prefix receipts](../resume4/prefix/checkpoint_paths.json)
