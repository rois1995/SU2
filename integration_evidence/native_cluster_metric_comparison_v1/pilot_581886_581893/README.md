# Paired cluster pilot 581886–581893 — partial campaign

The four baseline jobs completed all 16 cases. Each candidate job completed its first N4/M4/no-working-repartition case, copied complete selected solver evidence, then exited with `Do not mix audit versions in one export`: ClusterResults/tools still contained a different campaign's audit sources. No weighted M4/M3/M2 candidate case ran. This is an exporter failure, not a solver, native remeshing or MPI failure. The original candidate manifests describe selected data only and were written before tool copying failed; their COMPLETE label is not a complete campaign/audit-tools certificate.

All 350 selected-data hashes across the 16 baseline and four candidate exports match their manifests. Source-pin identities, input/config hashes and four-rank settings match the prepared baseline/candidate comparison. Recorded optimization/architecture flags, GCC10/MPI4.1.6 and stable AMD EPYC 9654 node fields match; instantaneous CPU frequency differs. Both actual cases have bitwise identical initial geometry and conservative flow before their first adaptation (steps 199/99). Build-host native instruction targets are not independently reconstructed from binary hashes.

The eight matched baseline/candidate no-repartition cases independently pass quality/metric-length, topology/original-reference, prescribed-height where present, transported-metric where exported, positivity and available both-history transfer checks. Each version's own auditors were used, in temporary copies of saved data; original downloads stayed unchanged. The remaining 12 baseline weighted cases have verified execution/collection and profile accounting, but were not independently re-audited here because their new-version counterparts never ran. No local solver, MPI run or scheduler submission occurred.

| Actual N4/M4, no working repartition | Metric old → new | Remesh old → new | Whole adaptation old → new | CFD old → new |
|---|---:|---:|---:|---:|
| Euler grid → RANS/BL | 36.67 → 13.57 s | 40.16 → 43.87 s | 77.33 → 57.94 s | 206.82 → 213.77 s |
| BL grid → Euler | 4.17 → 1.51 s | 13.15 → 13.29 s | 17.79 → 15.18 s | 56.81 → 55.92 s |

Whole adaptation includes metric recovery/construction (including the terminal metric window), remeshing/extraction/validation, replacement with CFD geometry repartition/migration and solution transfer, and adapted output. Transfer is nested in replacement and is not added twice. Working repartition is disabled in these matched rows. CFD includes its configured per-step output. Native setup, execution/return-to-CFD, rank stage and CFD geometry details are embedded in assessment.json; initial CFD geometry rows are distinguished from replacement. Do not sum independently reduced rank maxima into a wall-time total.

In this one paired observation, total adaptation decreases 25.08% for RANS and 14.67% for Euler. Metric time falls about 63%, while actual remesh totals increase about 9.2% and 1.0%. Candidate adaptation/CFD ratios are about 27.1% in both cases, compared with baseline 37.4%/31.3%. The complete subprocess wall times change 285.21→272.67s and 75.98→72.46s. These are observed workload results, not established speedups: adapted grids, second-window work and CFD trajectories differ, and sampled external process counts drop during later jobs. There is only one repetition on shared nodes.

Frozen Euler-to-BL remesh changes 34.674→28.504s, but output and commits differ (44812→44679), so it is not a fixed-work speedup. Frozen BL-to-Euler output is byte-identical with 27814 commits in both versions: 9.493→9.485s, a difference below 0.1% and too small for an improvement claim.

Candidate RANS produces 12168/12259 points, qmin 0.200090/0.266030, metric lengths <=1.8 and relative first-height error <=3.884e-12. Euler produces 7964/7240 points, qmin 0.232530/0.581465 and maximum metric length 1.786968. Frozen transported-tensor relative directional defects are <=7.05e-11. Both histories pass the configured CLOSED transfer policy, independently accounting for the changed open farfield polygon using its near-constant state; these are not claims that whole-domain integrals are unchanged under resampled farfield geometry. Final states are positive; converged aerodynamics are not assessed.

The first candidate RANS remesh retains substantial imbalance: private reconstruction min/mean/max 0.470/7.804/17.338s, max/mean=2.222. Exclusive rank-mean engine scopes include reconstruction 7.807s, validation 18.747s, donor import/IDs 2.892s, dependency import 2.576s, selection 1.129s, commit 0.750s, protocol 0.102s and unclassified 2.052s, closing the 36.055s engine total within printed rounding. Validation includes certificate exchange and MPI waits after uneven reconstruction; its large time does not identify pure validation CPU work. The candidate worker-count/repartition matrix is needed before choosing M or claiming the imbalance is solved. Nested private/collective timers must not be added to exclusive round scopes.

Sensor nodal gradation residual counts are zero. The separate candidate RANS composed sensor-plus-BL nodal audit reaches maximum ratio 318.696 and up to 15344 above-tolerance directed edges across its three metric windows. Its passing mesh contracts do not certify combined-field gradation. Euler composed diagnostics have no residual edges. No hard-normal reset, target relaxation or coarse-cell spreading of fine wall tensors was introduced by this collection fix.

The exporter now retains `ClusterResults/cases/CASE/tools/`, records tool hashes, and writes the collection manifest only after all tools are copied. Existing exports and their older shared bundles are untouched. Fake-file checks cover two audit versions coexisting, retained older data, missing outputs, missing tools and duplicate export rejection. The baseline at e6995fbff5 keeps its original layout/code; production source pins are unchanged.

Next, pull the candidate branch and rerun the eight-job serialized pilot (same source/binaries, one repeat, two actual remesh events). No rebuild or CFD correctness rerun is needed for this Python-only export change:

```bash
git pull --ff-only origin codex/native-unsteady-performance
ADAP_WORKERS=4:3:2 bash integration_evidence/native_cluster_metric_comparison_v1/SubmitMetricComparison.sh /global-scratch/bulk_pool/arausa/SU2_Native_Previous 4 1 2
```

Preserve these jobs and receipts. Download only each checkout's ClusterResults, keeping baseline and candidate separate, including case-local candidate tools and baseline's existing root-level bundle. Review all modes before starting repeated/longer scaling controls. Native 3D, converged flow accuracy and a verified universal performance gain remain outside this pilot. The paused goal tracker is unchanged.
