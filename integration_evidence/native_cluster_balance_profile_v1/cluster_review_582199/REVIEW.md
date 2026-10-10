# Cluster reconstruction-balance pilot 582199 — reviewed 2026-10-10

**All gates pass. The dominant Euler-to-BL compute hotspot is coordinated bulk-split repair near the trailing edge. Optimize that local work before adding mid-remesh migration.**

Original results: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/balance_profile_582199/`. Cluster working root was `/global-scratch/bulk_pool/arausa/SU2_Native/`. No local solver, MPI, build, scheduler submission or numerical mesh-audit recomputation occurred. The long-running goal tracker remains paused.

## Evidence and correctness

Launcher exit0/empty stderr. MPI1/2/4 gates pass 39 test cases per rank, including the two new balance tests. All16 frozen cases pass the cluster's independent mesh/geometry/first-height/transported-tensor and timing-accounting gates. All8 control/profile pairs have byte-identical adapted meshes and allfour transported tensor files.

Fresh download review verifies 324 selected case files, eight immutable fixture files, 10 archived audit tools, 4 archived package Python files, unit-log identities, input/binary receipts, and 810 compiled-source/Meson pins against the current published source. Recorded audit reports equal the aggregate entries; CSV private costs/commits close against the logs. Source identity and runtime gates do not independently prove build provenance; binary hashes are preserved in review.json.

Across accepted grids: qmin=0.1841786634, maximum Simpson metric edge length=1.799999529, relative prescribed first-height error <= 4.63e-12, maximum directional transported-tensor defect=7.046e-11 against tolerance1e-7. Original reference/marker/topology checks pass. This is a frozen adaptation/transport gate, not a converged-flow or combined-field-gradation certificate.

## Paired remeshing measurements

Total remeshing seconds include working repartition, worker execution/return and profile output. All runs are N4, one repetition on the same shared allocation. Comparison across M/partition modes changes final mesh/connectivity and operation counts; it is not fixed-work strong scaling. Byte identity is established within each control/profile pair only.

| Frozen workload | Workers / initial partition | Control s | Profile s | Observed change | Private max/mean (profile) |
|---|---|---:|---:|---:|---:|
| Euler→BL | M4 / none | 30.159 | 30.539 | +1.26% | 2.280 |
| Euler→BL | M4 / weighted | 33.881 | 34.316 | +1.28% | 2.546 |
| Euler→BL | M3 / weighted | 32.889 | 32.134 | -2.30% | 1.734 |
| Euler→BL | M2 / weighted | 41.914 | 41.955 | +0.10% | 1.494 |
| BL→Euler | M4 / none | 10.426 | 10.527 | +0.97% | 1.510 |
| BL→Euler | M4 / weighted | 10.560 | 10.234 | -3.08% | 1.225 |
| BL→Euler | M3 / weighted | 12.747 | 12.417 | -2.59% | 1.251 |
| BL→Euler | M2 / weighted | 15.659 | 15.751 | +0.58% | 1.256 |

Observed control/profile changes range from -3.08% to +1.28%; they do not show a large timing regression, but cannot isolate small instrumentation overhead from shared-node/order variation. Negative changes are not evidence that instrumentation speeds up remeshing. Explicit profile-file output cost is about2.0–2.7ms per case.

## What is expensive

**Euler→BL M4, no working repartition.** Private wall cost by rank is 13.749s, 0.489s, 4.419s, 5.464s; max/mean=2.280. Bulk splitting accounts for 71.1% of aggregate private wall cost. On the busiest rank it consumes 11.963s. The retained coordinated attempts total 7.214s (11 records, all committed). These are bounded top32 samples, not a complete joint-cost census.

Largest attempt: 46 imported cells, coordinated bulk split, 2.253358s wall / 2.243802s process CPU, 1,329,104 metric evaluations. Cavity box x=[0.98329840,1.00273295], y=[-0.00167778,0.00384827]. It committed successfully and accounts for 7.38% of this case's full remeshing time. Boxes can straddle both sides of y=0; this does not establish an upper-wall-only failure.

**Euler→BL M4, weighted initial repartition.** Private wall cost by rank is 19.605s, 2.773s, 4.264s, 4.157s; max/mean=2.546. Bulk splitting accounts for 79.6% of aggregate private wall cost. On the busiest rank it consumes 17.536s. The retained coordinated attempts total 8.626s (11 records, all committed). These are bounded top32 samples, not a complete joint-cost census.

Largest attempt: 42 imported cells, coordinated bulk split, 2.046232s wall / 2.039333s process CPU, 1,181,434 metric evaluations. Cavity box x=[0.99284938,1.00039144], y=[-0.00005429,0.00174986]. It committed successfully and accounts for 5.96% of this case's full remeshing time. Boxes can straddle both sides of y=0; this does not establish an upper-wall-only failure.

Euler→BL private process-CPU/wall ratios are about0.995 in allfour modes. The expensive private work therefore represents actual process CPU use during reconstruction, rather than mostly scheduling wait in that scope. Metric evaluation counts identify demand, but do not measure how much CPU is inside interpolation/BL evaluation versus search/topology/objective work.

The existing [joint reconstruction](../../../Common/include/adaptation/CNativeMesh2D.hpp) performs a dense SplitSeed search and nested weight/placement/step/sweep/vertex trials, repeatedly evaluating size deficits and quality. [DonorField](../../../Common/include/adaptation/CNativeField2D.hpp) has a bounded 2048-entry dynamic query cache; high private evaluation/eviction counts motivate examining query reuse and objective reuse. These source paths are suspects supported by the operation/counter evidence, not a measured exclusive breakdown within JointPatch.

**BL→Euler is a different cost pattern.** Its longest private attempt is about6.6–7.0ms; none of the retained hotspots is coordinated. Many ordinary bulk splits dominate private cost (77–79%). Donor import/ID allocation and round validation also occupy material wall scopes. This workload benefits from better aggregate balance at weightedM4 (private max/mean1.225 vs1.510), whereas Euler→BL weightedM4 is worse (2.546 vs2.280). One generic weight policy is not established as universally effective.

The logged complete coordinated phases took7.326s/45commits at M4/no partition,
8.684s/25commits at weightedM4, 2.108s/22commits at M3, and5.410s/23commits at M2.
These full phase timers include selection/protocol/import/validation and waiting;
they are different from the partial private sums of retained top32 attempts.
All BL→Euler cases logged zero coordinated commits/phase time. This confirms a
workload-specific joint-repair path, without treating the retained hotspot sample
as a full census.

## Exclusive phase accounting and working repartition

Euler→BL profiled M4/no-partition engine costs below are **rank means**. Their exclusive scopes add to engine-adapt30.423s within printed rounding. Private reconstruction6.030s is nested in round reconstruction6.056s; tracked collectives are overlapping diagnostic scopes and cannot be added again. Validation wall time includes other ranks waiting at elections and does not identify pure validation CPU cost.

| Exclusive engine scope | Rank-mean seconds |
|---|---:|
| selection | 1.260960 |
| round protocol | 0.110859 |
| round dependency import | 2.899240 |
| round donor import and IDs | 3.172080 |
| round reconstruction | 6.056320 |
| round validation | 13.675300 |
| round commit | 0.871597 |
| adapt unclassified | 2.376650 |

Working partition itself is small: Euler→BL about0.048–0.058s and BL→Euler about0.101–0.107s in these profiled cases. The logs separately retain estimate/graph/ParMETIS/migration timings. Weighted Euler→BL M4 stages are0.02118/0.01259/0.01759/0.00593s (rank maxima); its overall working-partition timer is0.05799s. Those maxima must not be added as a general critical-path decomposition.

Reducing active ranks does not improve these observed total costs: unprofiled Euler→BL M4/no partition30.159s, weightedM4 33.881s, M3 32.889s, M2 41.914s; BL→Euler10.426/10.560/12.747/15.659s. Max/mean ratios have a different ceiling at different M; a smaller ratio alone is not a benefit. Frozen runs do not perform CFD geometry replacement/BDF solution transfer. Existing actual-campaign evidence remains authoritative for those lifecycle costs.

## Recommended next step

1. Optimize repeated objective/metric work in successful coordinated repair. Inspect SplitSeed/JointPatch cost with aggregate substage timers and counters, then cache or reuse unchanged cell/edge scores and exact query results within the existing admitted scratch budget. Preserve frozen raw-sensor interpolation and actual-point geometric BL evaluation, finer demands/fade, geometry, altitude and all publication guards. Verify complete resulting grids/tensors and MPI gates on the cluster.
2. Preserve the repair freedom that produced these successful meshes. Simply cutting off the expensive committed proposals can reintroduce the trailing-edge stall. A different bounded search budget/order is a separate robustness experiment, not a free optimization.
3. Reassess balance after reducing pathological per-cavity work. Measured cost can then guide initial weights if many movable hotspots remain. Moving an unchanged indivisible search to another rank does not remove its cost from a synchronized round; repartition can also change reconstruction trajectories, as this matrix demonstrates. Mid-remesh migration is feasible but is not the first response supported by this evidence.

No extra cluster run is needed to identify this hotspot. Repeated/mirrored-order runs are needed before assigning a small overhead or speedup to a code change. Do not call this a192-core scaling result.

## Saved data and limits

The copy fallback worked: 332 verified staged/exported copies, no hardlinks, no leftover objects or temporary case folders. Actual downloaded files occupy 102,152,559 bytes (97.42MiB); the original cluster byte total precedes the final validation receipt update. No CFD timestep histories or ClusterRaw are needed. Original files/failed job582197 remain unchanged.

All sampled cases ran on node-a-ag1 (AMD EPYC9654,192physical cores), with48 visible other compute processes and four distinct24-core NUMA affinity ranges. This is shared-node contention sampling, not an exclusive allocation or a guarantee of equal memory/frequency conditions. Logs contain an ignored OpenMPI openib/libosmcomp warning; allfour ranks are on one node, so this campaign does not validate inter-node transport.

Inspect `ClusterResults/balance_profile_582199/cases/frozen_euler_to_bl_n4_m4_pNO_r1_profile/native_frozen_adapted.su2` for the adapted BL grid and the corresponding `frozen_bl_to_euler_...` folder for the Euler grid. All other worker modes are alongside them. Input SU2/raw tensor/source-flow provenance and final rank tensors are present in every retained case.

`review.json` stores full verified hashes, pair timings, rank/operation totals, largest cavity locations, numerical extrema and timing scopes. `review.py` reproduces these lightweight checks from the closed downloaded job with matching published source:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 integration_evidence/native_cluster_balance_profile_v1/cluster_review_582199/review.py ClusterResults/balance_profile_582199
```

No claim is made about actual unsteady end-to-end cost, converged aerodynamics, combined sensor-plus-BL gradation, native3D, or general scaling. No new production implementation or goal activation is part of this review.
