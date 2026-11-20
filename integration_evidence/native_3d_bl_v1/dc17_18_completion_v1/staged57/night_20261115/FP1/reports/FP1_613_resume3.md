# FP1 Resume 3, build613

Resume 3 was copied with its SHA256 and verified. The whole-order source check lists each reading used. [Order](../inputs/FP1_RESUME_3.md) · [SHA256](../inputs/FP1_RESUME_3.sha256) · [Source check](../resume3/source_check.md)

## Part G checkpoint

Part G was retained and pushed at `9940a85314`. The authorized stop ended the owned six-rank replay after 5,303.001 s. It preserved six first-pass and six lower-shift rows, with no completed second pass or higher shift. The build, fixtures, G1 and both OFF prefixes had passed. [Part G report](FP1_609_resume2.md)

## Part H fixtures and scope

Build613 linked in 13.148 s with zero new warnings. The source-scope receipt confirms the authorized H code paths. Outside the local-core call implementation and its print functions, the bytes of NRR and CConfig match the Part C option checkpoint. [Build receipt](../resume3/build613/build_status.json) · [Code scope](../resume3/scope.json)

- The shape fixture refused a valid short collapse that would reduce the worst margin from 2.51984 to 0.533931. It accepted a collapse that raised the worst margin from 0.00273911 to 0.533931.
- The split fixture found the expected sign disagreement after two ULPs. The float determinant was 3.46945e-18 and the exact determinant was -1.2326e-32. The exact refusal left the cell and point arrays and ID sequence unchanged. The float box passed self-check and strict metric counting. Its 384 live cells agreed with exact orientation for all 9,216 permutations.
- The donor fixture compared 24 samples, with the same winning donor in all 18 separated cases and a maximum relative component error of 2.17876e-16.
- The surface fixture kept the protected cells byte-identical and counted two blocked entries. Both MPI ranks saw and honored the remote-only unresolved mark.
- Both work-list cases matched the full-scan Owned bytes and all five edit counts. F4.1 reached 10 collapse and 4 split sweeps in one call. The second box reached 38 and 23. The sink's per-sweep counters reconcile with the pass totals. The work list stays enabled.
- The four-switch G control matched all 1,102,400 serialized Owned bytes. Five edit counts were 74 collapses, 169 splits, 380 removals, 12 swaps and 317 moves. There were 2,104 deferrals and 1,134 memory skips. [G byte capture](../resume3/g_baseline/baseline.json)

### Fixture corrections

1. Build611 emitted a GCC 13 warning in an inlined range insertion in a new hand fixture. Three ordered appends preserved the test's data and assertions. Builds612 and 613 emitted zero new warnings.
2. The build612 second box matched scan bytes and edit counts but reached only one split sweep. Collapse could remove its only hot point along a short transverse edge. The new box's existing interior hot-node tensor now has transverse components of 144, with its longitudinal component, geometry, walls and assertions kept. The box then reached 38 collapse and 23 split sweeps.
3. The inherited A fixture found 72 residual closed long edges and could account only for validity or the sweep limit. Build613 classifies the H4 split refusal explicitly and checks its counter in the individual probe and the pass. All 72 were blocked. The original fixture inputs, metric thresholds and short-edge collapse probe are retained. Its frozen-surface, quality, ID, and strict-measure checks passed.

### Changed counts in inherited fixtures

| Test | Build609 G | Build613 H |
|---|---:|---:|
| A output cells | 1,757 | 1,415 |
| A edits in collapse, split, removal, swap, move order | 30 / 121 / 25 / 0 / 34 | 23 / 49 / 7 / 0 / 10 |
| A strict q-bad, J-bad, long cells | 56 / 20 / 88 | 128 / 80 / 176 |
| A residual closed long, validity, blocked | 0 / 0 / 0 | 72 / 0 / 72 |
| F4.1 output cells | 2,600 | 2,228 |
| F4.1 edits in the same order | 74 / 169 / 380 / 12 / 317 | 51 / 81 / 182 / 5 / 286 |
| F4.1 deferrals, refusal memory skips | 2,104 / 1,134 | 1,691 / 1,258 |
| Modeled local-core bytes per cell | 10,620 | 10,980 |

[Final fixture receipts](../resume3/build613_checks/checkpoint_paths.json) · [Build612 fixture evidence](../resume3/build612_checks/checkpoint_paths.json)

[All inherited counts beside build609](../resume3/inherited_fixture_counts.json)

## Part H identity gates

G1 matched the adopted records and failed-assertion identities for serial, two-rank and four-rank cohorts (375 / 114 / 228 records). The cohort times were 766.897 / 9.922 / 9.928 s. [G1 receipts](../resume3/G1_613/checkpoint_paths.json)

G2 passed at the first-collapse prefix in 246.756 s with 90,809,869 work units. All six exact decision and modeled-storage rows matched the N14 reference under the corrected comparator. The timer fixed storage was 4,120 bytes. Communication and collective report rows are recorded alongside the reference. [G2 comparison](../resume3/prefix/G2_comparison.json)

GQ passed at the quality prefix in 685.827 s. All exact decisions and modeled storage matched the adopted N22 capture. Timer fixed storage remained 4,120 bytes. The two OFF prefixes preserve runtime evidence with matching binary, source and frozen input hashes. [GQ comparison](../resume3/prefix/GQ_comparison.json) · [Prefix receipts](../resume3/prefix/checkpoint_paths.json)

## Ordered lab results

Resume 3 §9.1 stops the item at the lab. Production-form LAB1 V2 took 1,072.121 s, exceeding the 600 s threshold by 472.121 s. Both V2 passes passed self-check and exact orientation. All six prescribed pair attempts reached terminal receipts. [Lab summary](../resume3/lab/lab_summary.json)

| Order | Pair | Pass wall (s) | Process wall (s) | Result |
|---:|---|---:|---:|---|
| 1 | [LAB2 V2](../resume3/lab/LAB2_V2/lab_receipt.json) | 88.835 | 161.828 | Completed |
| 2 | [LAB1 V2](../resume3/lab/LAB1_V2/lab_receipt.json) | 1,072.121 | 1,117.901 | Completed |
| 3 | [LAB2 V3](../resume3/lab/LAB2_V3/lab_receipt.json) | 60.969 | 138.263 | Completed |
| 4 | [LAB1 V1](../resume3/lab/LAB1_V1/lab_receipt.json) | — | 1,292.142 | Wall cutoff during quality |
| 5 | [LAB2 V1](../resume3/lab/LAB2_V1/lab_receipt.json) | 66.450 | 119.856 | Exact-positive output, strict audit stopped |
| 6 | [LAB2 V0](../resume3/lab/LAB2_V0/lab_receipt.json) | 162.988 | 215.797 | Exact-positive output, strict audit stopped |

Pass time covers array construction through write-back. Input loading and post-pass exact and strict audits are measured separately.

| Pair | Cells | Points | Margin-bad cells | Short edges | Long edges |
|---|---:|---:|---:|---:|---:|
| LAB2 V2 | 726,442 → 577,406 | 146,676 → 114,227 | 22,541 → 17,246 | 502,869 → 356,016 | 25,063 → 2,846 |
| LAB1 V2 | 303,359 → 593,731 | 61,330 → 114,317 | 15,008 → 122,791 | 209,553 → 353,380 | 43,113 → 15,743 |
| LAB2 V3 | 726,442 → 664,394 | 146,676 → 130,688 | 22,541 → 17,674 | 296,546 → 249,939 | 25,063 → 2,849 |
| LAB1 V1 | 303,359 → — | 61,330 → — | 15,008 → — | 209,553 → — | 43,113 → — |
| LAB2 V1 | 726,442 → 580,374 | 146,676 → 114,717 | 22,541 → 10,345 | 502,869 → 354,495 | 25,063 → 587 |
| LAB2 V0 | 726,442 → 566,975 | 146,676 → 112,471 | 22,541 → 11,121 | 502,869 → 344,577 | 25,063 → 610 |

LAB2 V3 counts short edges at 0.6363961031. Its pair receipt also records the count at 0.9.

| Pair | Nonpositive output cells | Strict q-bad / J-bad / long cells |
|---|---:|---:|
| LAB2 V2 | 0 | 16,748 / 16,560 / 8,230 |
| LAB1 V2 | 0 | 121,267 / 119,593 / 35,040 |
| LAB2 V3 | 0 | 17,181 / 17,032 / 8,331 |
| LAB1 V1 | — | Unavailable |
| LAB2 V1 | 0 | Unavailable |
| LAB2 V0 | 0 | Unavailable |

| Pair | Edits in collapse / split / removal / swap / move order | Blocked | Valid / exact / size / topology / memory / numeric / shape refusals |
|---|---|---:|---|
| LAB2 V2 | 63,641 / 31,192 / 37,536 / 3,677 / 16,001 | 67,478 | 3,377,262 / 402 / 297,594 / 0 / 0 / 0 / 32,096 |
| LAB1 V2 | 412,810 / 465,797 / 190,920 / 18,747 / 89,940 | 1,222,901 | 25,789,445 / 32,658 / 2,868,759 / 0 / 45,429 / 0 / 178,756 |
| LAB2 V3 | 48,110 / 32,122 / 38,521 / 3,801 / 16,475 | 69,279 | 3,353,898 / 414 / 161,170 / 0 / 0 / 0 / 27,079 |
| LAB1 V1 through cutoff | 77,786 / 249,273 / 414,488 / 21,408 / 356,866 | 0 | 24,366,587 / 557 / 3,222,042 / 0 / 12,033,017 / 0 / 35,289 |
| LAB2 V1 | 58,573 / 26,614 / 25,625 / 2,074 / 10,976 | 0 | 2,078,391 / 202 / 254,806 / 0 / 0 / 0 / 29,280 |
| LAB2 V0 | 60,678 / 26,473 / 32,286 / 2,072 / 10,996 | 0 | 2,193,804 / 0 / 239,425 / 0 / 0 / 0 / 0 |

The sweep counters reconcile with the pass rows in all five cases that reached write-back. The pair receipts contain every raw sweep row. The assessment records the cutoff-aware totals, per-kind sweep counts, live cell peaks and test-process RSS where available. [Sweep assessment](../resume3/lab/sweep_assessment.json)

On LAB1 V2, the measured 1,072.121 s included 265.073 s for collapse, 515.057 s for split and 290.467 s for quality. The strict audit found 121,267 q-bad cells, 119,593 J-bad cells and 35,040 long cells on its exact-positive mesh.

LAB1 V1 reached the 1,200 s wall limit during quality. GNU timeout sent SIGTERM, which the SU2 handler acknowledged while core work continued. The verified owned process group was hard-stopped at 1,292.141 s. The cutoff assessment uses the 188 complete rows preceding the first SIGTERM marker. The raw logs retain two later rows. The killed wrapper did not report the test process peak. A live sample observed its RSS high-water mark at 1,161,600 KiB. A separate nice-19 guard monitored the same deadline for the remaining pairs, which both ended before it. [Stop receipt](../resume3/lab/LAB1_V1/hard_wall_stop.json) · [Guard](../resume3/lab_wall_guard.py)

LAB2 V1 and V0 passed the core self-check and exact orientation audit. Production MeasureMetricCells then raised Degenerate metric donor at cells 732833 and 729907, respectively. Their complete strict totals are unavailable. The two V2 runs and LAB2 V3 completed the strict audit.

The six-rank replay, G3, shared-code safeguards and G5 ON remain unevaluated under this order. The build608 G5 OFF proof remains the accepted workflow evidence. The default is NO. [Build608 report](FP1_608.md)
