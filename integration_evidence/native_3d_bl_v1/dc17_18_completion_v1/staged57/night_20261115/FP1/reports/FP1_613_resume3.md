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
