# Downloaded cluster campaign assessment

All 16 cases pass independent numerical and profiling-accounting audits: eight frozen remeshes and eight actual unsteady runs (two adaptations each). Original collected evidence remains unchanged: 280 files, 276242614 bytes, hashes checked. All four histories per actual case have original donor snapshots and independently pass the CLOSED transfer check, including the measured open-farfield area correction. The checker’s generic sparse-history limitation does not apply to these complete snapshots.

Both directions preserve the original geometry, features, first-height constraints where requested, positive cells, conformity, metric quality and edge-length limits. Frozen reader tensors pass the independent directional transport check. Actual runs reconstruct the target independently from saved sensor donors and geometry; no final-tensor CSV residual is claimed for those runs.

The first pre-adaptation conservative flow state and Float64 sensor metric are bitwise identical across all four modes within each actual workload; configurations differ only in worker count/repartitioning. After adaptation, meshes and trajectories differ, so comparisons are not fixed-work speedups.

## Full actual lifecycle costs

Seconds. Adaptation includes metric construction, remeshing (including its working repartition/migration), mesh replacement (including CFD repartition and solution transfer), and adapted output. Transfer is nested in replacement and is not added twice. The terminal window builds a metric without remeshing; its metric cost is included.

| Workload | Workers / partition | Whole | CFD | Metric | Remesh | Replace | Output | Full adaptation |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| RANS Euler→BL | 2 / YES | 296.02 | 210.17 | 43.51 | 39.73 | 0.313 | 0.356 | 83.92 |
| RANS Euler→BL | 3 / YES | 326.90 | 255.70 | 33.61 | 32.70 | 0.322 | 3.135 | 69.77 |
| RANS Euler→BL | 4 / NO | 291.43 | 213.12 | 36.62 | 40.03 | 0.338 | 0.154 | 77.14 |
| RANS Euler→BL | 4 / YES | 270.23 | 203.97 | 36.88 | 27.74 | 0.335 | 0.268 | 65.22 |
| Euler BL→Euler | 2 / YES | 74.48 | 51.71 | 4.08 | 17.09 | 0.196 | 0.147 | 21.52 |
| Euler BL→Euler | 3 / YES | 69.73 | 50.40 | 3.85 | 13.91 | 0.194 | 0.130 | 18.09 |
| Euler BL→Euler | 4 / NO | 78.00 | 59.28 | 4.17 | 12.94 | 0.196 | 0.124 | 17.43 |
| Euler BL→Euler | 4 / YES | 68.78 | 50.96 | 3.65 | 12.74 | 0.189 | 0.123 | 16.70 |

## Where work accumulates

For weighted M=N=4 RANS, the first engine adaptation takes 23.77 s. Exclusive rank means are: selection 0.82, protocol 0.09, dependencies 2.42, donor import/IDs 2.60, reconstruction 5.47, validation/decision 10.28, commit 0.75, unclassified 1.33. Private reconstruction ranges 2.62–8.85 s: MPI imbalance remains. Validation wall scopes include collective waiting; their size does not prove equivalent validation CPU work.

Working repartition is small in these N=4 cases: weighted RANS estimates/graph/partition/migration component maxima are approximately 0.021/0.013/0.017/0.006 s before the first remesh and 0.082/0.019/0.011/0.008 s before the second. Component maxima are descriptive, not additive critical-path costs. CFD return partition/migration/preprocessing is inside replacement; its first adapted geometry row is 0.049/0.032/0.010 s. The initial geometry row is outside adaptation.

Weighted M=N=4 Euler takes 9.08 s in its first engine adaptation: donor import/IDs 2.43, private reconstruction 1.68, validation 2.69 s (rank means). Its second takes 3.26 s, with donor import/IDs 1.13 s. Dependency/donor traffic, recovery/metric work, and reconstruction imbalance deserve attention; changing worker count alone has not established a total-cost gain.

## Performance interpretation

Weighted four-worker actual runs have the lowest observed adaptation totals: RANS 65.22 s and Euler 16.70 s. M=3 and M=2 do not improve these observed totals. Frozen Euler→BL kernel/whole times are respectively: original M4 34.62/35.65, weighted M4 30.76/31.81, M3 30.48/32.65, M2 37.41/42.04 s. Frozen BL→Euler: original M4 9.46/10.73, weighted M4 9.19/11.53, M3 11.03/15.57, M2 13.47/15.42 s. The best kernel time need not be the best whole-process time.

These are single observations, not accepted speedup estimates or a default-setting recommendation. All campaigns share node-a-ag2.local (EPYC 9654, 192 physical cores); recorded processes show overlaps and common allowed CPU groups. With 24 permitted cores per rank group, overlap alone does not establish oversubscription. CPU PSI and actual per-thread placement/activity are unavailable. Even identical pre-adaptation fixed-mesh CFD varies: RANS 20.33–28.78 s, Euler 22.17–30.79 s. Shared caches, memory and filesystem activity are uncontrolled. RANS M3 adapted output alone takes 3.14 s versus 0.15–0.36 s in other modes.

The next cluster campaign must pair old/new executables under the same compiler/MPI/build options, rotate order, repeat at least three times, and serialize jobs through SGE dependencies. Repartition remains included. Use matched frozen targets to isolate the remesher, and actual cycles to assess changed metric/recovery trajectories. Keep noise disabled. No new local performance campaign is needed.

Memory samples are partial: sum of rank HWM is not a simultaneous peak; host-local RSS samples miss other nodes and may miss short peaks. These pilots establish N=4 numerical feasibility, not a scaling limit, native 3D support, long-time accuracy, or converged viscous forces.

Machine-readable numerical receipts and phase profiles: `assessment.json`; complete sampled timing/contention evidence: `timing_and_contention_evidence.json`; exact audit commands/results: `audit_progress.json`. Raw cases remain under `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/cases/`.
