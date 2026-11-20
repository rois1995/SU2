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

## Ordered lab results

All four pairs passed self-check and exact orientation, and completed the strict metric audit. Both V2 clocks are below the 600 s stop threshold. Pass wall covers array construction through write-back. Each linked receipt also has the input and audit clocks, raw sweep table, and resource observations. [Lab summary](../resume4/lab/lab_summary.json)

| Pair | Pass wall (s) | Process wall (s) | Peak live cells in sweep rows | Nonpositive cells | Strict q-bad / J-bad / long cells |
|---|---:|---:|---:|---:|---|
| [LAB1 V2](../resume4/lab/LAB1_V2/lab_receipt.json) | 43.571 | 80.612 | 564,602 | 0 | 2,196 / 1,826 / 16,336 |
| [LAB2 V2](../resume4/lab/LAB2_V2/lab_receipt.json) | 51.102 | 123.360 | 659,461 | 0 | 843 / 801 / 7,168 |
| [LAB1 V5](../resume4/lab/LAB1_V5/lab_receipt.json) | 47.670 | 88.802 | 641,413 | 0 | 2,213 / 1,851 / 7,563 |
| [LAB2 V3](../resume4/lab/LAB2_V3/lab_receipt.json) | 26.658 | 104.936 | 701,347 | 0 | 863 / 828 / 7,235 |

| Pair | Cells | Points | Margin-bad cells | Short edges | Long edges |
|---|---:|---:|---:|---:|---:|
| LAB1 V2 | 303,359 → 422,857 | 61,330 → 82,344 | 15,008 → 2,836 | 209,553 → 213,999 | 43,113 → 9,417 |
| LAB2 V2 | 726,442 → 558,310 | 146,676 → 110,597 | 22,541 → 1,210 | 502,869 → 336,681 | 25,063 → 2,594 |
| LAB1 V5 | 303,359 → 479,791 | 61,330 → 92,289 | 15,008 → 2,859 | 209,553 → 229,440 | 43,113 → 3,129 |
| LAB2 V3 | 726,442 → 644,771 | 146,676 → 126,993 | 22,541 → 1,232 | 296,546 → 231,510 | 25,063 → 2,596 |

The V3 short-edge counts use the requested threshold of 0.9 / √2. The receipt also gives the 0.9 counts.

| Pair | Edits in collapse / split / removal / swap / move order | Blocked | Valid / exact / size / topology / memory / numeric / shape refusals |
|---|---|---:|---|
| LAB1 V2 | 36,842 / 57,856 / 11,841 / 517 / 5,146 | 51,912 | 371,039 / 0 / 194,851 / 0 / 0 / 0 / 15,234 |
| LAB2 V2 | 57,766 / 21,687 / 11,215 / 552 / 4,326 | 13,467 | 320,684 / 0 / 179,145 / 0 / 0 / 0 / 28,619 |
| LAB1 V5 | 41,094 / 72,053 / 11,993 / 539 / 5,354 | 0 | 383,088 / 0 / 204,552 / 0 / 0 / 0 / 16,408 |
| LAB2 V3 | 42,214 / 22,531 / 11,184 / 554 / 4,424 | 13,478 | 193,077 / 0 / 39,087 / 0 / 0 / 0 / 23,567 |

On LAB1 V2, pass wall dropped from 1,072.121 s on H to 43.571 s on H6. The split phase fell from 515.057 s to 7.826 s and the sweep peak fell from 1,127,484 to 564,602 live cells. On LAB2 V2 the pass dropped from 88.835 s to 51.102 s. [Part H baseline](FP1_613_resume3.md)

LAB1 V5 disabled surface blocking while retaining the progress rule. It finished in 47.670 s, peaked at 641,413 live cells and wrote 479,791 cells. The excessive growth observed on the original LAB1 pass did not recur on this input.

### LAB1 V2 by sweep kind and round

| Kind | Round | Sweeps | Seconds | Accepted collapse / split / removal / swap / move | Live cells at end |
|---|---:|---:|---:|---|---:|
| collapse | 1 | 21 | 9.874 | 6,361 / 0 / 0 / 0 / 0 | 264,173 |
| split | 1 | 15 | 7.030 | 0 / 57,856 / 0 / 0 / 0 | 564,602 |
| collapse | 2 | 12 | 18.511 | 30,481 / 0 / 0 / 0 / 0 | 422,161 |
| split | 2 | 1 | 0.397 | 0 / 0 / 0 / 0 / 0 | 422,161 |
| collapse | 3 | 1 | 1.438 | 0 / 0 / 0 / 0 / 0 | 422,161 |
| split | 3 | 1 | 0.398 | 0 / 0 / 0 / 0 / 0 | 422,161 |
| quality | 0 | 20 | 4.585 | 0 / 0 / 11,841 / 517 / 5,146 | 422,857 |

### LAB2 V2 by sweep kind and round

| Kind | Round | Sweeps | Seconds | Accepted collapse / split / removal / swap / move | Live cells at end |
|---|---:|---:|---:|---|---:|
| collapse | 1 | 23 | 31.138 | 40,938 / 0 / 0 / 0 / 0 | 523,256 |
| split | 1 | 11 | 2.392 | 0 / 21,687 / 0 / 0 / 0 | 631,789 |
| collapse | 2 | 11 | 11.521 | 16,828 / 0 / 0 / 0 / 0 | 555,698 |
| split | 2 | 1 | 0.324 | 0 / 0 / 0 / 0 / 0 | 555,698 |
| collapse | 3 | 1 | 1.622 | 0 / 0 / 0 / 0 / 0 | 555,698 |
| split | 3 | 1 | 0.336 | 0 / 0 / 0 / 0 / 0 | 555,698 |
| quality | 0 | 20 | 2.162 | 0 / 0 / 11,215 / 552 / 4,326 | 558,310 |

### LAB1 V5 by sweep kind and round

| Kind | Round | Sweeps | Seconds | Accepted collapse / split / removal / swap / move | Live cells at end |
|---|---:|---:|---:|---|---:|
| collapse | 1 | 21 | 10.394 | 6,361 / 0 / 0 / 0 / 0 | 264,173 |
| split | 1 | 16 | 9.140 | 0 / 72,053 / 0 / 0 / 0 | 641,413 |
| collapse | 2 | 12 | 20.030 | 34,733 / 0 / 0 / 0 / 0 | 478,940 |
| split | 2 | 1 | 0.328 | 0 / 0 / 0 / 0 / 0 | 478,940 |
| collapse | 3 | 1 | 1.324 | 0 / 0 / 0 / 0 / 0 | 478,940 |
| split | 3 | 1 | 0.370 | 0 / 0 / 0 / 0 / 0 | 478,940 |
| quality | 0 | 20 | 4.738 | 0 / 0 / 11,993 / 539 / 5,354 | 479,791 |

### LAB2 V3 by sweep kind and round

| Kind | Round | Sweeps | Seconds | Accepted collapse / split / removal / swap / move | Live cells at end |
|---|---:|---:|---:|---|---:|
| collapse | 1 | 26 | 13.786 | 28,879 / 0 / 0 / 0 / 0 | 585,150 |
| split | 1 | 11 | 2.733 | 0 / 22,531 / 0 / 0 / 0 | 701,347 |
| collapse | 2 | 11 | 4.271 | 13,335 / 0 / 0 / 0 / 0 | 642,093 |
| split | 2 | 1 | 0.416 | 0 / 0 / 0 / 0 / 0 | 642,093 |
| collapse | 3 | 1 | 0.760 | 0 / 0 / 0 / 0 / 0 | 642,093 |
| split | 3 | 1 | 0.534 | 0 / 0 / 0 / 0 / 0 | 642,093 |
| quality | 0 | 20 | 2.445 | 0 / 0 / 11,184 / 554 / 4,424 | 644,771 |


## Six-rank event 1 replay

The frozen-input replay accepted event 1 in 1,568.109 s and produced 554,738 cells from 303,359. The job peak was 11,236.723 MiB and matched the sum of rank peaks. Minimum timer coverage was 95.055%. The replay uses frozen sensors and zero CFD steps. [Run receipt](../resume4/event1/replay_receipt.json)

| Rank | Stage-0 inclusive core and shifts (s) | Engine-entry cells |
|---:|---:|---:|
| 0 | 16.798 | 50,044 |
| 1 | 16.798 | 53,134 |
| 2 | 16.804 | 89,880 |
| 3 | 16.806 | 131,765 |
| 4 | 16.794 | 51,829 |
| 5 | 16.793 | 43,597 |

All stage-0 higher shifts were observed within 17.610 s of the six initial partition rows. The watchdog uses a one-second polling interval and records first-sweep onset estimates. The comparison preserves all sweep rows and the Resume 2 comparison against the build608 and FP0 OFF baselines. [Watch receipt](../resume4/event1/core_watch.json) · [Replay comparison](../resume4/event1/replay_comparison.json)

## Post-replay OFF proof

Build614 passed G3 with 1,148 accepted splits against a minimum of 336. All 1,113 shared edits matched the N14 reference, and the physical and observer-adjusted work gates passed. G4 passed all three rank cohorts. The saved-state safeguard receipts passed merit481 in 126.450 s, saved481 in 153.365 s, and the current target in 144.596 s. G1, G2 and GQ are the already-run build614 receipts above. [Scoped comparison](../resume4/scoped/scoped.json) · [Safeguards](../resume4/scoped/safeguards.json)

The build608 OFF proof provides the six accepted payloads, isolated restart, standing gates and the 1,818.319 / 1,039.725 s reference times. CConfig and the OFF-reachable NRR source match the Part C option checkpoint outside the isolated local-core and print functions. [OFF workflow receipt](../resume/G5/OFF/run_receipt.json) · [Source scope](../resume4/scope.json)

## G5 verdict

**FP1 ON FAILED under FAST_PATH_STAGE1 §12.5.** ON1 event 2 reached `complete=1` on all six native ranks. The standing timer gate then stopped the workflow. The first failed value was rank 0 named coverage of 93.688330% against 95.000000%. All six ranks fell below 95%. Join agreement passed on all six, the ledgers reconciled and timer fixed storage was 4,120 bytes. [Event 1 timer gate](../resume4/G5/ON1/full614_run1/f3b_event1_timer_gate.json) · [Event 2 timer gate](../resume4/G5/ON1/full614_run1/f3b_event2_timer_gate.json)

| ON1 event 2 rank | Named coverage | Named seconds short of 95% |
|---:|---:|---:|
| 0 | 93.688330% | 5.027 |
| 1 | 94.378722% | 2.381 |
| 2 | 94.515189% | 1.858 |
| 3 | 93.996078% | 3.848 |
| 4 | 94.133442% | 3.321 |
| 5 | 94.069620% | 3.566 |

The single read-only diagnosis reproduced the result from the raw completed ledgers. On rank 0 of event 2, root time fell from 1039.770 s OFF to 383.289 s ON, while unattributed time fell from 44.773 to 24.192 s. The shorter ON event still needed 5.027 more named seconds to clear the gate. Construction was the largest unattributed phase component at 19.011 s, with 4.072 s outside phase rows. The local core and shifts run under the existing named construction topology scope. The evidence identifies an attribution shortfall and supplies neither a wiring correction nor a border-shift cause for a §12.5 retry. [Read-only diagnosis](../resume4/G5/on1_diagnosis.json)

| Measurement | OFF event 1 | ON1 event 1 | OFF event 2 | ON1 event 2 |
|---|---:|---:|---:|---:|
| Native maximum seconds | 1,818.319 | 1,572.119 | 1,039.725 | 383.244 |
| Preparation phase seconds | 64.113 | 84.805 | 80.215 | 52.769 |
| Construction phase seconds | 460.024 | 513.126 | 209.275 | 171.965 |
| Quality phase seconds | 1,183.856 | 872.460 | 607.086 | 90.696 |
| Final proof phase seconds | 65.801 | 56.867 | 103.523 | 28.141 |
| Local core seconds | 0.000 | 18.270 | 0.000 | 10.261 |
| Shift seconds | 0.000 | 0.781 | 0.000 | 0.746 |
| Engine construction seconds | 460.024 | 494.099 | 209.275 | 160.964 |
| Minimum named coverage | 95.257% | 95.467% | 95.333% | 93.688% |
| Job peak RSS MiB | 13,381.543 | 11,029.734 | 15,980.539 | 11,097.828 |
| Final native cells / published points | 694,248 / 138,661 | 554,969 / 111,697 | 822,882 / 161,189 | 553,652 / Unevaluated |
| Last local edges / short local edges | 940,285 / 472,252 | 736,095 / 336,474 | 1,029,309 / 538,452 | 716,154 / 324,865 |
| Engine collapses / splits | 9,709 / 82,036 | 2,821 / 28,728 | 6,660 / 26,108 | 4,282 / 5,844 |
| Normalization stages | 1 | 1 | 1 | 1 |
| Nominal candidate complexity | 51,328.597 | 51,484.195 | 50,588.745 | 50,627.363 |
| Engine entry cells by rank | 48,316 / 51,155 / 56,223 / 51,202 / 50,531 / 45,932 | 50,044 / 53,134 / 89,880 / 131,765 / 51,829 / 43,597 | 115,746 / 102,554 / 127,730 / 119,151 / 121,207 / 107,860 | 104,696 / 80,115 / 88,893 / 95,522 / 81,558 / 86,613 |

Phase seconds are maximum rank-local exclusive totals, with unlabeled phase time still identified in the ledger. The inclusive core and shift clocks are inside construction. Rank maxima and nested clocks cannot be added. The run showed native maxima below the comparison bounds, but its standing coverage gate failed.

Event 1 passed the connected proof gate, published a mesh and authority sidecar, and continued CFD into event 2. The failed workflow retains their hashes and the event 1 restart. Native event 2 reported a candidate of 553,652 cells and an owned-proof face sum of 64,758, equal to the surface total. The controller stopped before event 2 mesh publication, full workflow validation and the isolated restart. ON2 and the six-payload repeat comparison are unevaluated under §12.5. The H6 checkpoint and build608 OFF proof are retained. The default is NO. [Connected event 1](../resume4/G5/ON1/full614_run1/p7p8_event1_gate.json) · [Terminal workflow receipt](../resume4/G5/ON1/full614_run1/full_workflow_receipt.json)
