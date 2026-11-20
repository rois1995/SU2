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

All four pairs passed self-check, exact orientation and the strict metric audit. Both V2 clocks are below the 600 s stop threshold. Pass wall covers array construction through write-back. Each linked receipt also has the input and audit clocks, raw sweep table, and resource observations. [Lab summary](../resume4/lab/lab_summary.json)

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

