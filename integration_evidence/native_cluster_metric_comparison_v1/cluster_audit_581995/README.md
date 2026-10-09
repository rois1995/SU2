# Independent cluster audit 581995 — PASS

The user-run one-worker saved-output SGE job completes all 32 baseline/candidate case audits successfully (exit0, empty launcher stderr). All four execution modes are covered: N4/M4 without working repartition, weighted N4/M4, N4/M3 and N4/M2. The audit needs only ClusterResults; deleted ClusterRaw directories are not dependencies. No CFD solver, MPI solver run or rebuild is performed by this job.

The aggregate contains 16 frozen-grid audits and 16 actual two-remesh CFD-output audits: 48 replacement meshes and 64 transported BDF histories. Every individual report exactly matches its aggregate record. The reviewed assessment and captured driver/wrapper hashes match publication ecd2b0626f438a673cc15776c0a9b469caed28a7. A fresh lightweight review also verifies all560 original exported data hashes, all32 collection manifests, and320 per-case audit-tool hash checks. All recorded numeric results are finite; checked original input/tool hashes agree with the assessment.

| Version / workload | Meshes | Minimum metric quality | Maximum metric length | Maximum relative first-height error | Maximum transported tensor defect | Maximum CLOSED-policy residual |
|---|---:|---:|---:|---:|---:|---:|
| baseline / frozen_euler_to_bl | 4 | 0.18456 | 1.8 | 5.0396e-12 | 1.0042e-10 | — |
| baseline / frozen_bl_to_euler | 4 | 0.18418 | 1.7983 | — | 6.2752e-15 | — |
| baseline / actual_euler_to_bl | 8 | 0.18386 | 1.8 | 5.6763e-12 | — | 7.1593e-13 |
| baseline / actual_bl_to_euler | 8 | 0.18418 | 1.7992 | — | — | 1.2157e-12 |
| candidate / frozen_euler_to_bl | 4 | 0.18667 | 1.8 | 4.6297e-12 | 7.0458e-11 | — |
| candidate / frozen_bl_to_euler | 4 | 0.18418 | 1.7983 | — | 6.2752e-15 | — |
| candidate / actual_euler_to_bl | 8 | 0.18212 | 1.8 | 4.7083e-12 | — | 7.0292e-13 |
| candidate / actual_bl_to_euler | 8 | 0.18418 | 1.7998 | — | — | 1.2144e-12 |

All meshes pass independent original-connectivity P1/frozen sensor plus actual-query geometric BL evaluation: metric quality>=0.18 and Simpson edge length<=1.8 (numerical tolerance1e-8), positive triangles, conforming topology/markers, no unused/duplicate points or duplicate elements, retained original reference features, complete reference-component coverage and geometric deviation within1e-6. RANS wall first-height relative error stays below1e-8. All16 frozen transported-tensor comparisons pass relative directional tolerance1e-7; raw frozen sensor identities match.

All64 available transferred histories pass density/pressure positivity and independently checked CLOSED-policy mass/momentum/energy residual<1e-10; transported SA values where present are finite and nonnegative. Final flow states are admissible in all16 actual cases. The CLOSED check accounts for changed open-farfield polygon area using the near-constant donor farfield state. It does not claim whole-domain integrals stay exactly unchanged when open geometry is resampled. Actual final transported metric tensors are not exported for this check; their mesh contracts use independently rebuilt original-P1/geometric targets. Frozen tensor transport is checked directly.

This completes the 24 previously pending weighted-mode independent numerical checks and repeats the eight previously certified no-repartition cases. The earlier performance assessment's pending statements remain a historical record and are superseded by this audit. The observed timing values and imbalance findings are unchanged: this job reads old solver logs/outputs and is not a new performance measurement. Quality/transport success does not certify composed sensor-plus-BL gradation, converged aerodynamics, larger-rank scaling, multi-snapshot PREDICT cost or native3D support.

Original downloads remain under `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/ClusterResults/saved_metric_audit_581995/`, including all32 case reports and stdout/stderr. Current RANS simulation grids remain in `ClusterResults/cases/581948_actual_euler_to_bl_n4_m4_pNO_r1/mesh_00200.su2`, `mesh_00400.su2` and its weighted-mode siblings; Euler grids are under job581950 at steps100/200. The actual CFD grids were produced by the earlier runs, not by this audit.

Evidence preserved here: full aggregate validation.json, review.json with input/tool/launcher/report hashes and per-mesh extrema, original launcher output/error/exit. The review was saved-data/JSON/hash checking only; no local CFD/MPI or numerical mesh-audit rerun occurred. The paused goal tracker remains unchanged.

Next engineering priority: inexpensive local reconstruction cost measurements by rank, operation and region, including rejected proposals; correlate them with unresolved work and communication, then improve the initial partition weights. Prior adaptation-event costs may seed subsequent partitions. Keep the original sensor donor field, geometric BL queries and acceptance gates fixed. Mid-remesh migration remains a conditional later experiment if persistent movable hotspots remain after better initial weighting. Repeat cluster timing controls before claiming performance gains.
