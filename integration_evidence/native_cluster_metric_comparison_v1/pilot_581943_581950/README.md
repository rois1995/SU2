# Paired cluster pilot 581943–581950

All eight jobs and 32 cases completed execution and collection. The case-local exporter fix succeeded in all 16 candidate cases. All 560 selected-file hashes, 160 candidate case-local tool hashes and 32 profile-accounting records verify. The ten audit tools for each version match their published baseline e6995fbff5 / candidate 93e7403366 versions; the later exporter fix changes no production source or binary. Source-pin identities (762 files), binary identities, selected build flags, GCC10.2/OpenMPI4.1.6 and stable EPYC9654 node fields match the prepared comparison. All 16 matched pairs have identical input/config hashes. Conservative flow before the first adaptation is bitwise identical for all eight actual old/new pairs.

Eight no-working-repartition cases have exactly the same relevant numerical inputs as the earlier independent PASS audits. Their quality/length, topology/reference, BL height, frozen transported tensors, positivity and available both-history CLOSED transfer proofs are reused after hash checks, including matching audit sources. Their previous audit timing fields describe the previous pilot only. The other 24 cases have passing execution and logged final contracts but await independent numerical postprocessing. Do not promote execution PASS to a complete numerical certificate. Raw downloads remain unchanged; no local CFD/MPI, build or heavy mesh-audit run occurred.

## Actual lifecycle timings

Each case has two remeshes and three metric windows, including the terminal window. Candidate seconds below. Adaptation = metric + remesh + replacement + adapted output. Solution transfer is nested inside replacement. Working repartition is inside remeshing; subsequent CFD repartition/migration is inside replacement. Initial CFD geometry is separate. Independently reduced rank maxima cannot be summed into a wall-time total.

| Direction | Workers / initial repartition | Metric | Remesh | Replacement | Nested transfer | Adapted output | All adaptation | CFD |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| Euler → RANS/BL | M4 / NO | 13.580 | 43.902 | 0.346 | 0.147 | 0.220 | 58.048 | 238.664 |
| Euler → RANS/BL | M4 / YES | 14.731 | 35.125 | 0.329 | 0.139 | 7.152 | 57.337 | 225.158 |
| Euler → RANS/BL | M3 / YES | 13.560 | 42.698 | 0.333 | 0.140 | 0.195 | 56.786 | 213.825 |
| Euler → RANS/BL | M2 / YES | 13.397 | 46.788 | 0.319 | 0.144 | 0.229 | 60.734 | 357.929 |
| BL → Euler | M4 / NO | 1.513 | 13.084 | 0.213 | 0.132 | 0.155 | 14.965 | 95.843 |
| BL → Euler | M4 / YES | 1.387 | 12.627 | 0.181 | 0.110 | 0.137 | 14.331 | 55.636 |
| BL → Euler | M3 / YES | 1.358 | 13.861 | 0.186 | 0.109 | 0.140 | 15.545 | 61.471 |
| BL → Euler | M2 / YES | 1.391 | 16.968 | 0.257 | 0.180 | 0.714 | 19.330 | 59.500 |

Observed metric time decreases 62–72% across matched modes. Adaptation lifecycle totals decrease 25–37% for RANS and 10–16% for Euler. These are paired observations, not established speedups: shared-node loads differ, grids/CFD trajectories differ between versions, and baseline RANS stops sensor gradation at 80 sweeps with residual edges while candidate reaches its fixed point. The comparison does not isolate identical metric/reconstruction work.

Candidate RANS M4/YES remesh is 35.125s versus M4/NO 43.902s (20.0% lower), but its adapted-output time rises to 7.152s from 0.220s. Complete adaptation becomes 57.337s versus 58.048s, only 1.2% lower. Filesystem/output variability must be retained in the unsteady affordability assessment. M3/YES gives 56.786s all adaptation; M2/YES gives 60.734s and substantially longer CFD time. No best rank policy is established by this single noisy matrix.

## Frozen remeshing controls

Baseline → candidate remesh seconds, including initial working partition and return to CFD ownership:

| Workers / initial repartition | Euler → BL | BL → Euler |
|---|---:|---:|
| M4 / NO | 42.426 → 33.986 | 12.519 → 12.013 |
| M4 / YES | 36.427 → 38.571 | 12.546 → 12.081 |
| M3 / YES | 36.207 → 33.389 | 14.452 → 13.924 |
| M2 / YES | 42.524 → 45.762 | 16.107 → 17.188 |

All four BL-to-Euler matched pairs produce byte-identical meshes with matching commits: the small differences from -4.1% to +6.7% show no consistent kernel gain. Frozen Euler-to-BL outputs differ across versions. Fewer workers improve some imbalance ratios but M2 is slower in both frozen directions. The candidate M3 frozen Euler-to-BL result is close to M4/NO; that does not establish a scaling optimum.

## Cost and imbalance

First candidate RANS remesh, exclusive rank-mean engine stages in seconds. Private reconstruction is nested in round reconstruction. Validation includes exchange/waiting after uneven reconstruction; it is not pure checking CPU time.

| Workers / partition | Selection | Dependencies | Donor import / IDs | Reconstruction | Validation | Commit | Private max / mean |
|---|---:|---:|---:|---:|---:|---:|---:|
| M4 / NO | 1.120 | 2.602 | 2.915 | 7.809 | 18.745 | 0.762 | 2.208 |
| M4 / YES | 0.912 | 2.532 | 2.668 | 7.434 | 14.897 | 0.789 | 2.440 |
| M3 / YES | 1.736 | 3.017 | 3.088 | 8.229 | 12.410 | 0.943 | 1.843 |
| M2 / YES | 1.996 | 2.576 | 2.758 | 16.696 | 12.798 | 0.990 | 1.530 |

Full exclusive scopes, protocol/unclassified closures, native setup/execution, each working-partition estimate/graph/ParMETIS/migration stage and each replacement geometry/solver/transfer stage are preserved in assessment.json. First-event working-partition stage maxima sum to roughly 0.046–0.056s for RANS and 0.098–0.104s for Euler (an upper-bound summary, not an exact wall timer). They are small relative to the engine, but live directory/telemetry rebuilding would add work during a mid-remesh migration. RANS replacement including transfer is only about 0.3s over both events; donor import inside remeshing is a different, much larger scope. Current counters do not separate pure donor evaluation CPU from communication/waiting.

Candidate RANS records 600 Hessian recoveries, with summed printed rank maxima about 0.53–0.55s; Euler has 300 and about 0.28s. WINDOW_AVERAGE requires those per-step samples. Their sums are diagnostics, not additive total metric wall scopes. Metric construction/gradation and remeshing are larger targets than removing scheduled Hessian samples here. This performance campaign does not measure multi-snapshot PREDICT overhead.

## Contention and numerical limits

Frozen jobs record about 144 other compute processes and load averages around 147–150. Baseline RANS runs on node-a-ag1 with 144 others, while candidate RANS runs on node-a-ag2 with 48 others. Identical CPU model/flags do not make those concurrent loads equivalent. Candidate M4/NO Euler CFD rises 55.924→95.843s from the earlier pilot despite identical numerical audit inputs, all saved numerical outputs and binary. Timing variability is material even when output/work is unchanged; exported histories do not independently prove identical inner-iteration counts. No exclusive allocation is requested.

All 32 logs report q>=0.18, L<=1.8, zero residual cells, height faces and reference faces. Independent numerical proof currently covers only the eight no-repartition cases. Candidate sensor gradation has zero residual edges in all actual modes, while baseline RANS retains terminal residuals. The separate composed sensor-plus-BL nodal diagnostic still reaches ratio 324.523 and 15950 residual edges across candidate modes/windows. Passing sensor gradation and mesh contracts do not certify composed-field gradation. No hard-normal reset or coarse-cell spreading of fine wall tensors was introduced. Converged aerodynamics and native 3D remain outside this pilot.

## Balance first; dynamic repartition as a conditional experiment

Improve measured balance before adding a live-migration mechanism. The initial weighted partition helps actual RANS remesh time but leaves private max/mean 2.440, versus 2.208 without it. Ratio alone is not an objective: lower max/mean with M2 still yields more total remeshing time. Measure local reconstruction cost by rank, operation and spatial region, including failed proposals, and relate it to current unresolved demand. Existing aggregate timers establish imbalance but do not identify a per-cell cost model. Costs learned from a previous adaptation event could improve the next initial partition without an extra migration; moving flow features and changing operator phases limit prediction accuracy.

Mid-remesh repartition is feasible in principle at collective checkpoints after complete commit/rollback, preferably an operation-phase boundary in the first implementation. Actual elapsed work becomes known; remaining cost is only estimated. A busy rank may have many movable cavities or one expensive indivisible cavity. Ownership changes cannot parallelize a single private reconstruction. Repartition only for persistent movable hotspots with enough work remaining that predicted savings exceed graph/migration/directory rebuild cost plus a margin; avoid triggering from MPI validation wait alone.

Start with a fixed active rank count. Keep immutable original sensor DonorField and geometric BL composition fixed; migrate current owned cells, retain identities/versions/allocation counters, rebuild incidence ownership and invalidate selection caches. Preserve phase progress or use phase boundaries where attempted sets/cache lifetimes already end. Admit migration and directory staging together before publishing so failure retains the old state. CFD solution and both histories stay on N until the usual final direct transfer.

Two specific reuse hazards in this checkout: CNativePartition2D.cpp uses CollectiveFailure/default reductions and MPI_Comm_split(SU2_MPI::GetComm()), appropriate for its current initial N-rank invocation but requiring communicator refactoring for a call on M<N workers. Also Engine snapshots its donor from constructor input, while CacheTarget overwrites current Cell.nodal_target with composed query tensors. Reconstructing Engine from current adapted cells would change the frozen target and risk wall-tensor spreading. WorkWeights currently expects original raw sensor-bearing cells and likewise cannot be reused unchanged on live composed caches. No dynamic-repartition production implementation was made in this review.

## Independent saved-output cluster audit

A reusable one-worker SGE job audits all 32 saved cases sequentially using each case/version's own pinned tools, temporary copies, and the already reviewed data hashes. It never launches CFD/MPI or changes original exports. Fake-file regression checks dispatch, version isolation, changed-data rejection, duplicate-output protection and retained nonzero audit failure. The actual cluster audit remains unrun.

From the candidate checkout after pulling this branch, submit:

```bash
qsub integration_evidence/native_cluster_metric_comparison_v1/RunSavedMetricAuditSGE.sh \
 /global-scratch/bulk_pool/arausa/SU2_Native_Previous/ClusterResults \
 integration_evidence/native_cluster_metric_comparison_v1/pilot_581943_581950/assessment.json
```

Download `ClusterResults/saved_metric_audit_JOB_ID/` and `ClusterResults/jobs/JOB_ID_saved_audit_launcher/`. Require all 32 numerical reports and validation.json PASS before claiming weighted-mode robustness. No rebuild is needed for this saved-data audit helper. Future comparisons can use the same helper with a newly reviewed assessment/hash manifest.

Actual candidate RANS grids are in `ClusterResults/cases/581948_actual_euler_to_bl_n4_m4_pNO_r1/mesh_00200.su2` and `mesh_00400.su2`; the weighted M4, M3 and M2 sibling folders contain their grids and histories. Corresponding Euler output is under job 581950, mesh steps 100/200. Their full local roots and all hashes are in assessment.json. Preserve both pilots. After numerical review, use repeated serialized shared-node controls before broader scaling or choosing M; the long-running goal tracker remains paused.
