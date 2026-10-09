# Cluster PREDICT correctness job 581856 — reviewed PASS

Validated checkout: codex/native-unsteady-performance at d4416246224aebee506bb1b5601c553cb8af0dc0. The documentation/evidence publication adds no production changes. The source/suite checkpoint matches all 837 captured files, excluding only cluster-specific Meson metadata from the local identity comparison. All 20 stage-log hashes and the saved checkpoint hash match. The CFD/test binaries retain the hashes from job 581852; the successful duration fix changed fixtures only.

All 20 sequential stages PASS: six metric/native suites at MPI1/2/4, five rejection guards, six native CFD cases and three actual adapted-mesh partial restarts. This completes the reusable native 2D PREDICT correctness gate. The corrected orientation test and explicit MAX_TIME fixtures now pass. All six independent audits were reproduced from the downloaded tools and outputs without running a local solver. Additional existing capcheck validity, marker and free-boundary geometry checks pass on all 11 replacements.

| Case | Replacements | Minimum q | Maximum metric length | Maximum history defect |
|---|---:|---:|---:|---:|
| default_two_n4m3 | 2 | 0.514341 | 1.795097 | 2.274e-15 |
| four_fixture_n1m1 | 1 | 0.582902 | 1.788690 | 2.763e-15 |
| four_n1m1 | 2 | 0.582902 | 1.788690 | 2.763e-15 |
| four_n2m1 | 2 | 0.582902 | 1.788690 | 3.003e-15 |
| four_cgns_n4m3 | 2 | 0.544042 | 1.786396 | 3.979e-15 |
| plate_sa_four_n4m3 | 2 | 0.595501 | 1.718409 | 2.578e-15 |

Both BDF history states pass independent conservation/positivity checks at every replacement. For the straight-wall SA RANS Euler-to-BL case, relative first-height error is at most 4.440892098500626e-16. CGNS grids decode successfully through the independent HDF5 reader. Partial restarts at 7/8/9 preserve the fixed cadence and exercise three-/two-/one-frame fallback, matching geometry/state, finite SPD metric and final positivity. Those short restarts contain no remesh and do not independently certify another transfer.

Four-frame Euler runs exercise filtered acceleration at every prediction window. The RANS fit uses constant-velocity fallback in the first and last windows, and acceleration in the middle window. Manufactured unit tests cover acceleration, temporal jitter filtering and MPI transfer; fitting the recorded history does not prove out-of-sample forecast accuracy. MPI consistency checks pass; whole adapted CFD trajectories are not required to be bitwise identical across decompositions.

All sensor gradation diagnostics report zero directed edges above 1+1e-5. Euler composed nodal diagnostics also report zero. The RANS composed field does **not** satisfy the nodal gradation audit: after prediction, maximum ratios are 1.28988, 1.94235 and 2.73259, with 2, 54 and 76 above-tolerance directed edges respectively. The last observation is the terminal metric window, without another remesh. The quality/length/height/transfer gates pass despite this distinct limitation. No target relaxation, hard-normal reset or spreading of fine BL tensors into coarse sensor donors was introduced.

No matched performance/scaling result is established by these short correctness cases. The candidate's 762 frozen production-source pins and all performance input hashes still match the prepared comparison. The next feasible step is the existing serialized N=4 baseline/candidate pilot, with no-repartition, weighted M=N and M=3/2 modes, then repeated or longer controls after reviewing the pilot. See ../../native_cluster_metric_comparison_v1/README.md. Keep the baseline at e6995fbff565955a5677ffcbcc66af9acf96a8f1 in its separate complete checkout. User-owned cluster runs only; shared-node timings remain approximate.

This gate does not certify native 3D remeshing, native FIXED_POINT, converged wall shear/drag/heat flux, general curved-wall RANS robustness, or MMG-control CFD compatibility. Earlier failure evidence remains preserved. The goal tracker stays paused pending user-owned performance results.

Raw validation/checkpoint, RANS diagnostic log/config and launcher receipts are preserved alongside the assessment. All per-case audits and supplemental geometry/topology results are embedded in assessment.json. Original grids, flow/restart snapshots and source-pinned tools remain unchanged in ClusterResults/predict_correctness_581856/. Inspect the CGNS grids in four_cgns_n4m3/mesh_00005.cgns and mesh_00010.cgns, and the RANS grids in plate_sa_four_n4m3/mesh_00005.su2 and mesh_00010.su2.
