# Strict frozen reuse matrix 582358: review

Reviewed 2026-10-10. **PASS: all 16 remeshes and all eight strict OFF/BOTH
comparisons.** Focused MPI 1/2/4 suites pass (63 tests per rank). This closes
the previously pending two-direction/four-partition frozen 2D reuse checkpoint.

## Evidence and numerical result

Downloaded case folder:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/reuse_matrix_582358/`.
Launcher: `ClusterResults/jobs/582358_reuse_matrix_launcher/`; exit 0.
Planning/publication source checkpoint d44cc986c2880ac3bf7e975342efc257f55cd8a9;
C++ diagnostic implementation remains 0bb829910ad646496e9e87ab9a5983487e69efc7.
Both roles use the tested job582344 binary:
`5827ffbe8b7ac9dd0336d000a33a8d649975579f1cd57f623a440a00fb87019e`.
Control is reuse OFF, profile is BOTH; fresh-hit audit is NO throughout.

The lightweight reviewer verifies **1,259 unique source/tool/data/log files**,
all source pins and collection manifests, matching paired input/config hashes,
binary/mode receipts, numerical reports, rank operation records, commit-count
closure, phase accounting and nonoverlapping UTC execution intervals.
Every pair has byte-identical accepted SU2 mesh and all four transported-tensor
CSVs, with identical per-rank selections/attempts/reconstructions/commits/cell and
selection-scan counts. The strict gate is unchanged. All 16 independent frozen
geometry/topology/reference/quality/length/first-height/metric audits report PASS.

Across cases:
- Minimum metric quality: 0.1841786634467251 (required 0.18).
- Maximum metric edge length: 1.799999529453445 (limit 1.8).
- Maximum Euler-to-BL relative first-height error: 4.629741034989365e-12.
- Maximum transported directional tensor defect: 7.04577330522297e-11
  (audit tolerance 1e-7).
- Peak process RSS from recorded high-water marks: 159.84–176.78 MiB.
- Exclusive engine phase means close within printed rounding; maximum absolute
  printed closure defect is 9.2e-5 s. Rank maxima are not added together.

BL-to-Euler cases have no prescribed first-height constraint. These are actual
frozen-target remeshes, without CFD time steps or new Hessian recovery.

## Measured total remeshing times

N=4 CFD ranks throughout; M is the adaptation worker count. YES enables weighted
working partitioning. Times include native setup, working partition/migration,
worker execution and accepted mesh return to CFD ownership. They do not include
metric construction, subsequent solver geometry rebuild, solution/history
transfer, external audits or output collection. Therefore these times are not
complete CFD adaptation-event costs or the affordability ratio R.

| Workload | M / weighted | OFF (s) | BOTH (s) | Time reduction |
|---|---|---:|---:|---:|
| Euler-to-BL | 4 / NO | 30.952 | 29.575 | 4.45% |
| Euler-to-BL | 4 / YES | 34.589 | 33.116 | 4.26% |
| Euler-to-BL | 3 / YES | 32.585 | 32.410 | 0.54% |
| Euler-to-BL | 2 / YES | 42.789 | 42.059 | 1.71% |
| BL-to-Euler | 4 / NO | 10.613 | 10.906 | -2.76% |
| BL-to-Euler | 4 / YES | 10.302 | 10.292 | 0.10% |
| BL-to-Euler | 3 / YES | 12.465 | 12.465 | 0.001% |
| BL-to-Euler | 2 / YES | 15.406 | 15.317 | 0.58% |

Euler-to-BL M4/NO saves 33.20% of requests but only 2.81% of actual metric
evaluations, with 5.44% less summed private process CPU. M4/YES saves 33.26% of
requests, 3.97% of evaluations and 5.10% private CPU. M3/M2 gains are smaller.
BL-to-Euler request counts are unchanged; evaluation reductions are only
0.32–0.37%, with no convincing total-time benefit from reuse here.

This agrees with diagnostic582344: reuse helps the expensive coordinated
Euler-to-BL repairs modestly, not every operation mix. It is not a general
large speedup or a solution to interpolation/BL query cost.

## Rank choice, partition and bottlenecks

For BOTH, Euler-to-BL M4/NO is fastest (29.575 s); weighted M4/M3/M2 take
33.116/32.410/42.059 s. BL-to-Euler weighted M4 is fastest (10.292 s);
unweighted M4/M3/M2 take 10.906/12.465/15.317 s.

Reducing M improves Euler-to-BL private max/mean imbalance from 2.22
(unweighted M4; weighted M4 is 2.48) to 1.72/1.49 at M3/M2, but does not reduce
total wall time. Partition setup itself is small: recorded working-partition
maximum is about 0.05–0.11 s including estimation, graph, ParMETIS and migration.
Changed ownership also changes reconstruction trajectories and resulting mesh
sizes (Euler-to-BL 22,654–24,259 triangles; BL-to-Euler 14,817–15,305).
Cross-partition comparisons are matched-target workload comparisons, not
fixed-work scaling or proof that one partition strategy always wins.

For BOTH Euler-to-BL M4/NO, the 29.462 s mean engine time contains exclusive
rank-mean scopes: selection 1.240, protocol 0.112, dependency import 2.874,
donor import/IDs 3.184, reconstruction 5.829, validation 13.015, commit 0.864
and unclassified 2.344 s. Private reconstruction is nested in reconstruction;
tracked collectives are also nested detail. Validation includes waiting for
slower private work: its large time is not proof that numerical checks dominate
CPU. Private max/mean 2.22 remains a material imbalance.

BL-to-Euler M4/YES has engine mean 10.021 s, with donor import/IDs 2.761,
reconstruction 1.755, validation 2.816 and other exclusive scopes making up the
remainder. This workload has no large coordinated-repair attempts comparable
to Euler-to-BL. Current profiles do not isolate donor search, interpolation and
BL composition inside private queries. Those measurements belong in the next
development stage, alongside efficient local indexing and bounded MPI edits.

## Timing limits and retained data

All cases are sequential by absolute UTC, 14:44:29–15:01:10 on 2026-10-10.
Recorded node-a-ag1 samples show **48 other compute processes**, load averages
near 48, and broad per-rank CPU masks. Each pair has one ordered repetition.
The roughly 4% Euler-to-BL gains agree with the previous diagnostic, but small
differences and the BL-to-Euler slowdown cannot be classified as reproducible
performance effects without repetitions under recorded contention.
No extra timing campaign is required to accept the numerical checkpoint.

Collection retained 103,195,291 bytes (98.41 MiB) of case data by its own
accounting, using 384 verified copies and no hardlinks. Hardlink fallback worked.
The exports already contain the necessary grids, tensors, configs, logs and
reports; there is no need to recover ClusterRaw or rerun CFD for this review.
Case meshes are under `cases/frozen_*_n4_m*_p*_r1_profile/native_frozen_adapted.su2`.

## Decision and reproduction

The frozen 2D checkpoint is complete and suitable as the pre-development
reference for Goal 1 in NATIVE_3D_DEVELOPMENT_GOALS.md. Keep full 2D lifecycle
regressions when production code changes; this matrix does not validate new CFD
histories, converged viscous quantities, composed-field gradation or native 3D.
No production/config/rank-policy change is warranted solely from these timings.
The paused tracker and four prepared goals remain unactivated.

`review.py` reads saved data and existing independent reports, hashes inputs,
recomputes bounded profile/pair summaries and runs small negative checks for
changed hash/count/corrupted bytes. It runs no solver, build, MPI or geometric
mesh-audit campaign. From the matching checkout:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_reuse_diagnostic_v1/cluster_review_582358/review.py
```

An optional first argument selects the repository; an optional second selects a
new, exclusive JSON output. Saved review.json records checked identities, phases,
costs and limits. Receipt identities do not independently prove compilation
provenance. Original downloads and failed historical comparisons remain intact.
