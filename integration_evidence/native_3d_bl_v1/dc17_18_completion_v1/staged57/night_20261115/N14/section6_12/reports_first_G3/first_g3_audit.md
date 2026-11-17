# First N14 G3 wall-counter audit

Read-only report for the first N14 G3 fixture. Run two and final N14 acceptance are separate.

## Historical purpose comparison

| run | accepted splits | geometry-queue guidance rejects / trials | ratio |
|---|---:|---:|---:|
| N9_baseline | 457 | 1170 / 1425 | 0.821053 |
| N11_baseline | 459 | 1046 / 1301 | 0.803997 |
| N14_first | 1113 | n/a | minimum ≥689 |

## Required, checked, and source sums

- Required: 731 trials / 731 accepted.
- Checked: 132 trials = 99 rejected + 33 accepted.
- Required accepted + checked accepted = 764; law-selection accepted geometry = 764.
- All accepted = 1113 (geometry 764 + size 349).
- N14 source rows (validated / guidance-rejected / approved): primary 1225 / 106 / 1113; RefitWallSplit 0 / 0 / 0; ReconstructWallSplit 0 / 0 / 0; ReconstructSplitPatch 12 / 12 / 0.
- Source totals 1237 / 118 / 1113: 1237−118=1119 metric trials; 1119−6 metric rejects=1113 accepted. All three source-sum checks pass.
- N11 source rows and sums are preserved in `wall_rows.json`; N9 baseline predates `NATIVE3D_WALL_SOURCE` rows.

## Child-parent and measurement accounting

- Registered new faces=2916; derived parent-known=2388; unknown-parent=528.
- At/below tolerance=1319; above parent bound + tolerance=0.
- Unmatched=0; unmeasured=8; observation failures=0.
- Counter definitions do not establish overlap among parent-unknown, unmeasured, and tolerance buckets; no residual or serial-complete-parent-coverage claim is made.

## Physical, resource, and state limits

- First-run physical receipt passed: positive private cells=306462; sensor cells=306462; upper refinements=0.
- Modeled storage max=11215009617 B; 12 GiB limit=12884901888 B; within limit=True.
- Sampled job RSS=3390308352 B; 19073 MiB limit=19999490048 B; within limit=True. Wrapper timeout=900 s; elapsed=624.866412 s.
- Phase max_seconds=388.8 s; observed elapsed=465.703646 s; stop=time_budget; work=6067 / maximum_work=233220. Source/config stage limit is reported unchanged. Observed phase elapsed exceeds configured maximum; recorded without adjudicating the gate.
- Stage result reports engine_valid=False, final_mesh_accepted=False; this fixture is not a final accepted BL mesh claim.

## Wall-lane and prepare context

- First-case `NATIVE3D_WALL_LANE_CENSUS` is all zero (commits=0). This one surface-stage fixture is not the full two-event M26 population.
- `NATIVE3D_WALL_PREPARE` surface row: calls=1113, seconds=1.3670429710000005, edits=1113, faces=6678; other seven phase rows are zero.
- All raw wall rows, frames, source counters, stage exits, and case hashes are in `wall_rows.json`, `wall_rows.csv`, and `wall_table.md`.
