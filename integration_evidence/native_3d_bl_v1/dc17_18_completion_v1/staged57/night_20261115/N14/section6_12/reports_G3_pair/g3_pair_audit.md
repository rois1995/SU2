# N14 G3 paired report

Post-only report of N9/N11 baselines and both N14 G3 cases. No numerical job launched; no acceptance beyond the paired G3 physical/purpose result is claimed.

## Purpose and paired identity

| Run | Accepted | Purpose threshold | Purpose passed | Geometry rejects / trials | Ratio |
|---|---:|---:|---|---:|---:|
| N9 baseline | 457 | — | Historical | 1170 / 1425 | 0.821053 |
| N11 baseline | 459 | — | Historical | 1046 / 1301 | 0.803997 |
| N14 first | 1113 | 689 | True | — | — |
| N14 second | 1099 | 689 | True | — | — |

Pair identity: passed=True; 1099 common accepted edits compared; first mismatch=None. The comparison is through the smaller accepted count.

## N14 source and trial accounting

| Run | Source validated | Guidance rejected | Source approved | Metric trials | Metric rejected | Required trials / accepted | Checked trials (rejected / accepted) | Geometry accepted | Size accepted |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| First | 1237 | 118 | 1113 | 1119 | 6 | 731 / 731 | 132 (99 / 33) | 764 | 349 |
| Second | 1223 | 118 | 1099 | 1105 | 6 | 717 / 717 | 132 (99 / 33) | 750 | 349 |

Source-approved totals match accepted counts. Required plus checked accepted reconciles with geometry accepted. Detailed per-source rows and all wall-preparation, frame, law-selection, wall-split, and lane-census rows are in `wall_table.md` (322 marker rows across the four cases). These are diagnostic accounting checks.

## Child-parent observations and physical limits

| Run | Output cells; positive / sensors | Upper refinements | Modeled peak / limit (bytes) | Sampled job RSS / limit (bytes) | Registered faces | Parent-known derived | Parent-unknown | Unmeasured | At/below tolerance | Above parent+tolerance |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| First | 306462; 306462 / 306462 | 0 | 11215009617 / 12884901888 | 3390308352 / 19999490048 | 2916 | 2388 | 528 | 8 | 1319 | 0 |
| Second | 306427; 306427 / 306427 | 0 | 11215009617 / 12884901888 | 3390263296 / 19999490048 | 2864 | 2336 | 528 | 4 | 1296 | 0 |

Both N14 stage fixtures passed their stated physical and purpose checks. Child-parent counters are scoped to locally registered primary faces; parent-known is registered minus unknown. Parent-unknown, unmeasured and tolerance buckets may overlap, so these data do not establish serial-complete parent coverage. Neither stage fixture reports final mesh acceptance (`engine_valid=false`, `final_mesh_accepted=false`).

## Timer interpretation

| Run | Configured phase budget (s) | Stage row elapsed (s) | Exit wall-law census (s) | Stage minus census (s) | Timer root (s) | Fixed storage (bytes) | Census inclusive row (s) |
|---|---:|---:|---:|---:|---:|---:|---:|
| First | 388.8 | 464.816136 | 75.947918 | 388.868218 | 465.703599 | 4120 | 529.600381 |
| Second | 388.8 | 464.825662 | 75.908280 | 388.917382 | 465.708467 | 4120 | 530.879643 |

The source calls PrintWallLawCensus from finish after work has stopped, and before observe_progress("exit") prints NATIVE3D_STAGE. PrintWallLawCensus opens the pre-existing Operation::Census scope and has a local clock around its wall-law sweep. Thus the exit diagnostic is included in the emitted stage/Adapt elapsed clocks after the 388.8 s decision budget has stopped trial work; subtraction leaves about 388.9 s in both N14 stage rows. The output is an observation-scope difference, not a new timing gate or a changed configured limit.

Pinned source SHA256 `7545edb4cea518fffc7e0e5cd4f763891fbe2a002b2d26b177b04b76e0d3b848` matches both run pins=True. Source locations: CNativeEngine3D.cpp:100-132, CNativeEngine3D.cpp:2296-2303, CNativeEngine3D.cpp:2313, CNativeEngine3D.cpp:2052, CNativeEngine3D.cpp:2155. Fixed storage remains 4120; no timing gate was added.

## Artifacts

- `g3_pair_audit.json`: machine-readable audit and case receipts.
- `wall_rows.json` and `wall_rows.csv`: extracted wall marker rows.
- `wall_table.md`: detailed marker tables.
