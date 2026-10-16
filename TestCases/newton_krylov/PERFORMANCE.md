# Newton–Krylov performance assessment, 16 October 2026

The subsequent [final qualification](../../Docs/NewtonKrylovQualification.md)
adds family/layout and native-baseline checks plus an empty-rank MPI output fix.
All numerical solver and geometry objects remain identical to this report's
`a3fcc53d2e6` endpoint; the later binary differs only in the shared output helper.
No writer timing improvement is claimed.

The implemented changes reduce segregated Newton compute-loop time by 29–34% on the serial SA/SST/active-LM fixtures and 12–17% on their 2x2 layouts. Outer and Krylov iteration counts are unchanged. Native remains about 7–11 times faster on these fixed-CFL cases. The periodic-axis fixture retains its existing Newton advantage. These are small-mesh measurements, not a production-size or scaling claim.

Three source commits were pushed to `rois1995/SU2:feature_LineSearch`, immediately after each commit:

- `75541a9f1029956d7873e1ef4cdf8de989340375`: cached least-squares geometry, including periodic closure, and local reuse of unchanged Newton bases.
- `66fbffea057ef078b932d0bb1127b7b78e46bb2f`: allocate FGMRES basis payloads only when used; handle shared FGCRODR storage and layout changes safely.
- `a3fcc53d2e6fd0831c1a332f90e02a9b1db0a6a5`: fuse cached nodal gradient zeroing, accumulation and multiplication, preserving periodic communication order.

The nonperiodic cache core and coloring fixes were adapted from upstream `feature_cachedLeastSquares` at `e50d32088516db4d06413d40a3fa95620f4040ee` and retain attribution. The periodic extension uses this checkout's native completed periodic stencils, axis copies and composed corrections. It does not transplant an incompatible periodic implementation.

## Measurement boundaries and controls

Baseline numerical source is clean `fce4c547f60983305d440405217fe0d1c05b3c08`. The timed baseline adds only the same pre-finalization loop observer used by the final code; its patch and binary are preserved. Existing physical-property evidence was retained. Its SA work counts are reproduced exactly; historical wall times are not mixed with the new measurements.

The final cohort contains 144 CFD runs: four cases, two layouts, two versions, three repeats and three modes. Every run also has an independent physical/state reporter invocation, excluded from CFD timing. Original/final order alternates. Each median uses three accepted samples; JSON retains values, ranges, commands, configurations and hashes. All 48 final triplets passed continuous contention monitoring. No build or other owned numerical job ran concurrently.

The guard requires three quiet one-second preflight observations, samples continuously, and rejects an entire triplet if another solver/test/build is active or external CPU consumption exceeds two cores on the eight-core i7-9700 host. It subtracts the complete owned process tree, including exited-child CPU, from host activity. Earlier unmonitored/contended trials and intermediate source versions are excluded and preserved. The final cohort policy was declared before these measurements (`final_cohort_policy.json`, repeat indices 400–402).

Build: GCC 9.4, release `-O3 -march=skylake -ffast-math -fno-finite-math-only`, MPI/OpenMP enabled, same build options for original/final. AD, GPU, MLP, CGNS and TecIO are disabled. Runtime: ROMIO321, `mpirun --bind-to none`, `OMP_WAIT_POLICY=PASSIVE`; 1x1 means one MPI rank/one OpenMP thread, 2x2 means two ranks/two threads each. Frequency scaling and ordinary host noise remain; there is no affinity/scaling qualification.

End-to-end time starts before the MPI launch and ends when CFD exits. Compute-loop time is rank-zero solver computation before finalization, excluding result-file output. It includes convergence history/Newton diagnostics. Phase observers instead time the whole `StartSolver` method, including result output; those observer times are not release medians.

SA/SST/LM use the unchanged first-order Roe fixtures with 81 points: all history fields must reach the existing stopping target, independent raw RMS ≤1e-10, normalized RMS ≤1e-9, and state/quantity errors ≤1e-6. SST/LM histories additionally reach their original -11 threshold. Active LM must retain partially and highly intermittent free nodes. Axis uses 187 points, raw RMS ≤1e-12, normalized RMS ≤1e-11, state/quantity errors ≤1e-6, and an absolute 1e-12 tolerance for the zero axial moment. No stopping, derivative, hard true-defect, Armijo, state or quantity criterion was relaxed.

SA/SST/LM retain fixed Newton CFL and fixed forcing. Axis already requests adaptive Newton CFL while native retains its original fixed CFL; the axis advantage predates these optimizations. Multisystem coupled fixtures explicitly retain block-lower preconditioning (production default is NO), including its extra matrix-free cross products. These fixture choices are identical between source versions.

## Original-controls before/after results

F/T/Tr denotes flow/turbulence/transition equation assemblies. Outer and Krylov columns apply to both versions: all their individual samples are unchanged. Native assembly counts in release runs are inferred from outer updates; separate native profiles confirm the same `Space_Integration` counting boundary. Newton counters are measured equation telemetry plus the known final all-active reporting refresh, independently matched exactly by scoped assembly counts. Diagnostic BASE rows can reference reused evaluations and must not be counted as residual calls. Krylov steps and native ILU applications are different quantities.

| Case/layout | Mode | Outer, both versions | Krylov, both versions | R F/T/Tr before → final | End-to-end s before → final | Compute loop s before → final |
|---|---|---:|---:|---|---:|---:|
| sa/1x1 | native | 3327 | 36381 | 3327/3327 → 3327/3327 | 1.370 → 1.320 | 1.005 → 0.955 |
| sa/1x1 | segregated | 2927 | 19749 | 68520/30709 → 56977/25020 | 9.784 → 7.079 | 9.391 → 6.701 |
| sa/1x1 | coupled | 2927 | 14360 | 55064/55064 → 49210/49210 | 8.983 → 6.830 | 8.569 → 6.446 |
| sa/2x2 | native | 3327 | 63031 | 3327/3327 → 3327/3327 | 4.625 → 4.674 | 4.245 → 4.286 |
| sa/2x2 | segregated | 2927 | 29036 | 77807/30953 → 66264/25264 | 40.037 → 34.877 | 39.645 → 34.521 |
| sa/2x2 | coupled | 2927 | 23344 | 73032/73032 → 67178/67178 | 40.689 → 37.833 | 40.334 → 37.453 |
| sst/1x1 | native | 3488 | 38141 | 3488/3488 → 3488/3488 | 1.520 → 1.520 | 1.148 → 1.106 |
| sst/1x1 | segregated | 3058 | 21184 | 73160/33640 → 60930/27524 | 13.392 → 9.433 | 13.020 → 9.059 |
| sst/1x1 | coupled | 3119 | 15367 | 58806/58806 → 52568/52568 | 12.790 → 9.585 | 12.371 → 9.210 |
| sst/2x2 | native | 3488 | 66054 | 3488/3488 → 3488/3488 | 4.875 → 4.925 | 4.519 → 4.538 |
| sst/2x2 | segregated | 3058 | 33771 | 85747/36694 → 73517/30578 | 52.808 → 46.648 | 52.468 → 46.287 |
| sst/2x2 | coupled | 3119 | 25072 | 78216/78216 → 71978/71978 | 56.064 → 52.559 | 55.725 → 52.236 |
| lm/1x1 | native | 1416 | 17127 | 1416/1416/1416 → 1416/1416/1416 | 0.969 → 0.919 | 0.587 → 0.544 |
| lm/1x1 | segregated | 1414 | 6434 | 34251/11949/14192 → 27023/9573/11364 | 7.781 → 5.276 | 7.393 → 4.857 |
| lm/1x1 | coupled | 5615 | 6205 | 69158/69158/69158 → 57927/57927/57927 | 20.208 → 14.393 | 19.851 → 14.044 |
| lm/2x2 | native | 1416 | 32414 | 1416/1416/1416 → 1416/1416/1416 | 2.621 → 2.621 | 2.239 → 2.228 |
| lm/2x2 | segregated | 1414 | 10985 | 38802/12846/15584 → 31574/10470/12756 | 29.768 → 24.860 | 29.393 → 24.473 |
| lm/2x2 | coupled | 5615 | 11496 | 85031/85031/85031 → 73800/73800/73800 | 81.158 → 72.494 | 80.809 → 72.113 |
| axis/1x1 | native | 1319 | 100353 | 1319 → 1319 | 12.239 → 12.339 | 11.837 → 11.933 |
| axis/1x1 | segregated | 14 | 375 | 502 → 474 | 0.718 → 0.668 | 0.296 → 0.287 |
| axis/1x1 | coupled | 14 | 375 | 502 → 474 | 0.668 → 0.668 | 0.298 → 0.293 |
| axis/2x2 | native | 1319 | 105519 | 1319 → 1319 | 15.694 → 15.645 | 15.304 → 15.277 |
| axis/2x2 | segregated | 14 | 574 | 701 → 673 | 0.968 → 0.918 | 0.580 → 0.552 |
| axis/2x2 | coupled | 14 | 574 | 701 → 673 | 0.968 → 0.969 | 0.571 → 0.548 |

The following endpoint maxima cover all three modes and repeats, each compared against the preserved original-native endpoint using the original uncached physical reporter. Per-mode/component results remain in `benchmark_final_summary.json`. Quantity errors use the fixture's existing relative/absolute comparison convention; the axis zero moment uses its absolute tolerance.

| Case/layout | Max raw RMS before → final | Max normalized RMS before → final | Max relative state error before → final | Max quantity error before → final |
|---|---:|---:|---:|---:|
| sa/1x1 | 9.992e-11 → 9.992e-11 | 1.117e-10 → 1.117e-10 | 8.463e-07 → 8.463e-07 | 3.335e-07 → 3.335e-07 |
| sa/2x2 | 9.977e-11 → 9.977e-11 | 1.117e-10 → 1.117e-10 | 8.463e-07 → 8.463e-07 | 3.335e-07 → 3.335e-07 |
| sst/1x1 | 9.999e-12 → 9.999e-12 | 9.858e-12 → 9.858e-12 | 7.100e-08 → 7.100e-08 | 2.708e-08 → 2.708e-08 |
| sst/2x2 | 9.999e-12 → 9.999e-12 | 9.859e-12 → 9.859e-12 | 7.100e-08 → 7.100e-08 | 2.709e-08 → 2.709e-08 |
| lm/1x1 | 9.997e-12 → 9.997e-12 | 1.036e-11 → 1.036e-11 | 4.574e-10 → 4.574e-10 | 4.833e-11 → 4.833e-11 |
| lm/2x2 | 9.995e-12 → 9.995e-12 | 1.036e-11 → 1.036e-11 | 4.570e-10 → 4.570e-10 | 4.832e-11 → 4.832e-11 |
| axis/1x1 | 9.739e-13 → 9.739e-13 | 3.081e-13 → 3.081e-13 | 1.439e-09 → 1.439e-09 | 2.695e-08 → 2.695e-08 |
| axis/2x2 | 9.739e-13 → 9.739e-13 | 3.081e-13 → 3.081e-13 | 1.679e-09 → 1.679e-09 | 3.740e-08 → 3.740e-08 |

The native changes are small (roughly -7% to +1% in these medians); axis Newton differences are only a few milliseconds with overlapping ranges. They do not establish a general speedup. The larger segregated SA/SST/LM reductions are consistent across these repeats. Parallel tiny partitions regress strongly against serial for both implementations and are unsuitable for extrapolating production scaling.

## Measured explanation

The original SA case has 5854 accepted block attempts, no CFL retry and alpha=1 on every nonzero update. Repeated backtracking does not explain its slowdown. The ordinary active block path originally used approximately k+8 wrapper evaluations. Local base reuse removes two unchanged evaluations, giving approximately k+6, while preserving the independent true defect and physical slope/trial checks.

Flow assemblies fall 68520→56977 (16.8%) and turbulence assemblies 30709→25020 (18.5%), with 2927 outer updates and 19749 Krylov steps unchanged. The separate native solve performs only 3327 assemblies per system. Native's additional assembled-matrix products are much cheaper than fully refreshed finite-difference residual products. A smaller linear iteration count alone does not predict wall time.

The original serial CPU sample profile attributes 26.17% of samples to least-squares gradients (6115 samples, none lost). The geometry cache avoids rebuilding/factoring unchanged normal matrices, rather than freezing state gradients or physical properties. Final SA still performs 284887 flow preprocessings/gradient evaluations: about five per flow residual wrapper, from live dependency refresh, boundary closure and residual preparation.

| SA segregated observer phase | Calls original → final | Inclusive seconds original → final |
|---|---:|---:|
| residual_wrapper | 56810 → 51121 | 7.4624 → 4.9085 |
| implicit_residual_wrapper | 11710 → 5856 | 1.7419 → 0.6327 |
| flow_preprocessing | 342602 → 284887 | 4.5171 → 2.2628 |
| flow_gradient | 342602 → 284887 | 2.5597 → 0.8016 |
| dependency_refresh | 137041 → 113955 | 3.9147 → 2.0423 |
| boundary_closure | 68521 → 56978 | 2.6066 → 1.5103 |
| strong_boundary | 304791 → 252930 | 0.1567 → 0.1007 |
| far_boundary | 205560 → 170931 | 0.8976 → 0.6508 |
| flow_residual_only | 62664 → 54048 | 1.6687 → 1.2806 |
| flow_residual_and_jacobian | 5856 → 2929 | 0.2440 → 0.1002 |
| turb_residual_only | 24853 → 22091 | 0.3769 → 0.2986 |
| turb_residual_and_jacobian | 5856 → 2929 | 0.1274 → 0.0522 |
| preconditioner_build | 5689 → 5689 | 0.1090 → 0.0905 |
| preconditioner_apply | 19749 → 19749 | 0.1198 → 0.0823 |
| finite_difference_probe | 31127 → 31127 | 4.2512 → 3.1217 |
| capture | 5854 → 5854 | 0.0191 → 0.0126 |
| restore_values | 59737 → 48194 | 0.1158 → 0.0776 |
| save_history | 68520 → 56977 | 0.0703 → 0.0495 |
| restore_history | 68520 → 56977 | 0.0818 → 0.0523 |
| newton_diagnostics | 17232 → 17232 | 0.3526 → 0.1943 |

These are single attribution runs, not medians. Original serial observer contention was not continuously monitored; final observer runs were guarded. All rows are inclusive and overlap. Flow/turbulence residual-and-Jacobian scopes contain interleaved residual and matrix kernels and are explicitly joint scopes, not pure Jacobian timings.

The final SA serial observer also measures 1,736,589 MPI Allreduces (0.0738 s), versus 1,967,284 originally. Native has 342,072 in the comparable final profile. Serial PMPI time is small. Final main-thread allocation/free API time totals about 0.111 s; allocation counts/times exclude worker-thread allocation. Capture/restore/history copies and diagnostics are measurable but do not dominate this fixture. Diagnostic time is 0.194 s; result-write checks total under 0.001 s. Output buffering and snapshot pooling were therefore not used as speculative runtime fixes.

| Observer version / SA mode, 2x2 rank 0 | StartSolver s | Barrier/loop-end calls | Time inside barriers/loop ends s | PMPI s |
|---|---:|---:|---:|---:|
| profile_before / native | 4.440 | 2137860 | 2.973 | 0.568 |
| profile_before / segregated | 41.240 | 22211389 | 32.031 | 3.532 |
| profile_before / coupled | 41.741 | 22470352 | 32.308 | 3.487 |
| profile_final / native | 4.466 | 2137898 | 3.015 | 0.573 |
| profile_final / segregated | 35.528 | 19247502 | 27.567 | 3.073 |
| profile_final / coupled | 39.037 | 20878082 | 30.701 | 3.085 |

These exact method-boundary PMPI/GOMP profiles use the same counting boundary across modes. Time inside OpenMP barrier/loop-end calls is about 77% of the segregated method on these tiny partitions, including waits for other threads; it is not a pure hardware latency measurement. The nodal pass fusion removes two barriers per eligible ordinary gradient. An additional 66fb→a3 paired SA2x2 check removes 713174 barrier/loop-end calls and preserves all three modes' restart bytes exactly (`nodal_fusion_comparison.json` and the `program_*` evidence). Periodic gradients complete native RHS exchange before applying the cached inverse. No automatic thread reduction was introduced.

Jacobian assembly and preparation were examined separately using a fixed-state paired harness. Residual-only and R/J wrappers alternate for three batches; the difference estimates the marginal matrix/clearing cost in the same code path. It cannot uniquely separate arithmetic shared by interleaved kernels. The 37249-point checks are fixed-state profiling, not converged CFD performance. Explicit/implicit residual differences are at most 1.6e-17 relative to the largest component; the small fixtures are bitwise equal. The final harness uses an epsilon-scaled check tighter than the existing assembly-equivalence regression. This external profiling assertion does not change any solver criterion.

| Physics / points | R-only ms/sweep | Marginal R/J − R ms/sweep | Native preparation ms/sweep | Max R difference / max R |
|---|---:|---:|---:|---:|
| SA / 81 | 0.1027 | 0.0126 | 0.0015 | 0.000e+00 |
| SST / 81 | 0.1205 | 0.0136 | 0.0015 | 0.000e+00 |
| LM / 81 | 0.1693 | 0.0168 | 0.0021 | 0.000e+00 |
| SA / 37249 | 30.8861 | 5.3285 | 0.9954 | 1.572e-17 |
| SST / 37249 | 39.3121 | 6.1438 | 1.0805 | 1.573e-17 |
| LM / 37249 | 54.5722 | 7.7403 | 1.3287 | 7.617e-19 |

Paired fixed-state R-only versus R/J wrappers estimate marginal assembly/clearing cost, not isolated kernel time. Three alternating paired batches in one guarded process, 1000 sweeps/batch on 81 points and 10 on 37249. Implicit preparation is a sweep over all selected native blocks, excluding its untimed matrix rebuild. Whole block application includes optional lower-block matrix-free products. Method scopes are inclusive master-thread wall time.

| Case / mode | Flow preparation calls / s | Scalar preparation calls / s | Full block build calls / s | Full block apply calls / s |
|---|---:|---:|---:|---:|
| sa / native | 3327 / 0.009747 | 3327 / 0.007181 | 0 / 0.000000 | 0 / 0.000000 |
| sa / segregated | 2927 / 0.003225 | 2762 / 0.001576 | 5689 / 0.092567 | 19749 / 0.112634 |
| sa / coupled | 2927 / 0.003245 | 2927 / 0.001757 | 2927 / 0.091175 | 14360 / 1.813382 |
| lm / native | 1416 / 0.004289 | 2832 / 0.006637 | 0 / 0.000000 | 0 / 0.000000 |
| lm / segregated | 610 / 0.000701 | 2376 / 0.001765 | 2986 / 0.041196 | 6434 / 0.036302 |
| lm / coupled | 5616 / 0.006627 | 11232 / 0.007231 | 5616 / 0.252633 | 6205 / 3.113633 |

The full block-preconditioner timer corrects an important interpretation: native ILU build/application time excludes the optional lower-block matrix-free products. In the final SA coupled observer, full application takes 1.813 s of 6.449 s `StartSolver` time (~28%), while native ILU application alone was ~0.100 s in the phase profile. LM coupled full application takes 3.114 s of 14.363 s (~22%). Construction is relatively cheap; the cross-block application is not. Method timers include normal native RMS work when native requests it; Newton intentionally invokes preparation with RMS reporting suppressed. Do not add these overlapping observer timers.

## Memory and safe reuse

With one million local unknowns, maximum FGMRES dimension 80 and convergence in three iterations, three alternating measurements linked against the same final release supporting archives give median whole-process peak RSS **1305564→94160 KiB** (~1.245 GiB→91.95 MiB). Allocated basis payloads fall from 162 vectors (1296000000 bytes) to seven (56000000 bytes). Both implementations use three iterations and independently checked true residual 2.36051e-12. This is an isolated linear benchmark, not a full CFD memory measurement. Payload storage remains at its high-water mark and can grow when all iterations are actually needed; FGCRODR retains the entries it requires. No restart length changed.

The snapshot audit confirms remaining duplication across segregated controllers. Each group allocates five double arrays plus a row-mask byte per active-system state entry, one relaxation double per active system/point, density and trial-flow workspaces, and reconstruction histories. For primal 2D SA/SST/LM, excluding reconstruction edge entries, matrices, Krylov and other controller vectors:

| Physics | Groups | Primary snapshot/dependency payload, bytes/local mesh point across groups | Payload / million local mesh points |
|---|---:|---:|---:|
| SA | 2 | 522 | 522 MB |
| SST | 2 | 604 | 604 MB |
| LM | 3 | 1176 | 1176 MB |

These are allocation-size estimates from the current constructors, not measured peak RSS. They use mesh points, whereas the Krylov benchmark uses unknowns. Sequential scratch sharing remains a useful larger-mesh memory task, but requires explicit lifetimes around captured accepted states, evaluation history, boundary closure and exact rollback. It was not mixed into this runtime fix after capture itself measured only 0.0126 s in final SA.

Local base reuse is confined to an unchanged attempt: CFL is installed before preparation, retries rebuild after restored state/CFL changes, and no cache survives a new outer state. Explicit `SetBase`, independent defect evaluation, post-defect base restoration, slope and trial remain. Cached LS data contains only geometric inverse normal matrices, with separate weighted/unweighted slots, mesh/stencil invalidation and no discrete-adjoint reuse. Every periodic RHS remains live through native rotations/intersections/axis handling and halo exchange. Configuration-dependent derivatives, prescribed-row masks, numerical history and pseudo-time coordinates are preserved.

Unconditional flow assembly during an auxiliary probe remains. Native characteristic boundary states consumed by scalar equations are currently refreshed through flow integration, and live turbulence/transition properties affect flow closure. Deleting that call or reusing gradients/properties solely because the selected group is TURB would be unsafe. A future boundary-state-only API should first prove which fields are unchanged across every boundary/dependency path. The present work does not claim that separation is implemented.

## Existing control experiments, separate from implementation changes

The following use final source with two explicit option changes: `NEWTON_ADAPT_CFL=YES` and `NEWTON_ADAPTIVE_FORCING=YES`. `CFL_ADAPT=NO` remains for native. All original endpoint/derivative/Armijo requirements and the hard maximum true defect **0.1** remain unchanged; adaptive forcing may vary its requested linear accuracy within that existing cap. Each median has three guarded samples and is excluded from the source-only matrix. The accurate-SA variant instead changes only `USE_ACCURATE_TURB_JACOBIANS=YES`.

| Variant / case (1x1) | Mode | Outer | Krylov | R F/T/Tr | End-to-end s | Loop s | Max state error |
|---|---|---:|---:|---|---:|---:|---:|
| adaptive / sa | native | 3327 | 36381 | 3327/3327 | 1.369 | 0.976 | 0.000e+00 |
| adaptive / sa | segregated | 21 | 257 | 533/213 | 0.468 | 0.062 | 8.551e-08 |
| adaptive / sa | coupled | 23 | 216 | 601/601 | 0.517 | 0.095 | 5.329e-09 |
| adaptive / lm | native | 1416 | 17127 | 1416/1416/1416 | 0.919 | 0.531 | 0.000e+00 |
| adaptive / lm | segregated | 14 | 211 | 470/139/135 | 0.517 | 0.101 | 1.914e-08 |
| adaptive / lm | coupled | 16 | 91 | 392/392/392 | 0.468 | 0.117 | 1.960e-08 |
| accurate_sa / sa | native | 3327 | 36381 | 3327/3327 | 1.319 | 0.958 | 1.990e-13 |
| accurate_sa / sa | segregated | 2927 | 19753 | 56981/25024 | 7.179 | 6.722 | 8.463e-07 |
| accurate_sa / sa | coupled | 2927 | 14360 | 49210/49210 | 6.880 | 6.483 | 8.463e-07 |

Adaptive SA segregated reaches CFL 1e6 and uses a limited nonunit accepted step; coupled SA/LM exercise one rejected attempt per repeat. All exact rollback checks pass. Fixed-control coupled LM has one CFL retry from 20 to 5; with adaptation disabled it never regrows that accepted CFL and takes 5615 updates. This is a concrete pseudo-time/convergence limitation, not repeated line-search overhead. The controls experiment demonstrates why convergence tuning can outweigh another small constant-cost optimization. It does not establish universal defaults: native CFL was not retuned, these are tiny fixtures, and larger/difficult transition cases remain necessary.

The accurate-SA option adds four total segregated Krylov steps here and gives no clear runtime improvement. No default or Jacobian formula was changed. The local paper review (`papers/jacobian_review.md`, PDF paths/hashes and page passages in its manifest) distinguishes the full residual derivative from a stabilized approximate preconditioner. Wang et al. (2021) motivates flux-consistent approximate matrices; Moryossef/Levy (2006) and Park/Kwon (2004) emphasize stabilizing turbulence source treatment; Mosahebi/Laurendeau (2015) shows that stronger transition coupling need not win. Current SST retains sink derivatives including a k–omega off-diagonal; LM uses mostly diagonal source terms. A future comparison of consistent full local source blocks against stabilized sink-dominant blocks is justified, but indiscriminately adding production/blending derivatives is not. The current accurate-flux flag has no Roe consumer, so enabling it would not strengthen these fixtures' Roe preconditioner.

## Correctness and preserved failures

- Final code: all 56 native Newton cases pass on 1x1, 1x2, 2x2 and 2x4; 89 supported ordinary cases pass. These exercise derivative/BC closure, accepted/full and backtracked steps, failed linear solves, CFL retries, resolved-group resume, rollback/state/history/row masks, restart/failure reporting, species and flamelet behavior.
- FGMRES/FGCRODR storage checks: all 13 linear-algebra cases pass on 1x1, 1x4 and 2x2, including changing vector/block layouts, solver switching, zero-iteration reuse, collective execution and an empty MPI rank. This source is unchanged by the final gradient-only commit.
- Cached/natural/colored/weighted/unweighted and periodic gradient tests pass on 1x4 and 2x2, including rotations, intersections, axis points, stale halo poisoning and invalidation. Valgrind gradient and selected Newton/storage checks report zero errors; leak checking was disabled.
- Reporter, species-boundary and residual-failure CLI checks pass. Eight affected production translation units compile with forward AD syntax. This is not AD runtime qualification.
- The existing 37249-point nonuniform SA fixture passes three repetitions on each of 1x1/2x2, with genuine work on each configured thread, one accepted update, and forced linear/search failures restoring exactly. All fixed-layout binary/CSV/config artifacts are bitwise repeatable and bitwise identical before/after the nodal fusion. Cross-layout accepted-state relative L2 difference is 1.74e-15. This checks a larger existing case; it is not a fully converged larger-mesh benchmark or race-detector run.
- Every final release CFD run passes its strict original physical/state/quantity checks. There are no timing assertions in unit tests.

Evidence is preserved under `newton_krylov_refactor_20261015/performance_20261016` relative to the workspace root, with older checkpoints retained outside it as originally found. Failed/contended trials are not deleted or rewritten. Initial lazy-storage test-fixture failures, CLI/profile setup failures, OMPIO failures, intermediate source measurements and final scheduler self-check failure remain. The fixed-state timing harness initially required bitwise R/RJ equality and failed only at a 1.1368683772161603e-13 absolute difference on a 7231.4 maximum residual; initial source/config/logs and the diagnostic binary are in `jacobian_cost/initial_failure.json` and its referenced files. No physical target was changed to resolve this harness issue. Solver-level rejected attempts remain in their original Newton CSV files, including the fixed-LM retry and adaptive experiment failures.

## Hashes and reproduction

| Artifact | SHA256 / source revision |
|---|---|
| Baseline numerical source | `fce4c547f60983305d440405217fe0d1c05b3c08` |
| Baseline original CFD | `1e0f119b20ed30d3ee276722f378a130fcb54c31dae7901da33f39f36de24575` |
| Baseline CFD with loop observer only | `8484d52deee3891dcc5486f6142e39a04310b54ba27a9875064afa9f356b3110` |
| Final numerical source | `a3fcc53d2e6fd0831c1a332f90e02a9b1db0a6a5` |
| Final release CFD | `c4be2a80dd56baddd86e6f96629538181d793cd16426a8eb5a9b736294f59a38` |
| Original independent reporter | `0b48736ffa7c1477f0360c607d1509bcb4e7f39c74275e1df89b14a5df23440e` |
| Final native tests | `19117c3af3cfc498bece09e0100f91cc5489b951cf765d2196be78cc8c63181f` |
| Final ordinary units | `a8aa2633e05eee8c7d526d0a52c2ab2c62f718f320dabc12db7cdb216a8b1474` |
| Common build options JSON | `e06b7645a873c0ba548b05ce2db596603b3a07ee300969da70612fa521b07340` |

Source/build/compiler manifests are in `original/`, `before/`, `final/`, with full source archive manifest at `../periodic_axis_88df7866aff5/source`. Every `results.json` records all mode configuration and mesh/input hashes; every endpoint reporter has its own manifest. The original/final configurations are asserted identical mode-by-mode. `reproducibility_manifest.json` collects the selected evidence/config/runner hashes. Pinned binaries are never overwritten.

Run from the workspace root, using the existing configured build:

```sh
export TMPDIR="$PWD/newton_krylov_refactor_20261015/mpi_tmp"
export OMPI_MCA_orte_tmpdir_base="$TMPDIR"
export OMPI_MCA_io=romio321
export OMP_WAIT_POLICY=PASSIVE
PYTHONPATH="$PWD/externals/meson" NINJA="$PWD/ninja" ./ninja \
  -C newton_krylov_refactor_20261015/build -j8 \
  SU2_CFD/src/SU2_CFD UnitTests/test_driver UnitTests/newton_equation_tests
```

Use an unused repeat index; the runner refuses existing output. Each invocation runs native/segregated/coupled sequentially with independent endpoint checks. Repeat for `sa sst lm axis`, `1x1 2x2`, original/final and three alternating rounds, with no other heavy workload:

```sh
python3 newton_krylov_refactor_20261015/performance_20261016/run_benchmark.py \
  --version before --case sa --layout 1x1 --repeat 403
python3 newton_krylov_refactor_20261015/performance_20261016/run_benchmark.py \
  --version final --case sa --layout 1x1 --repeat 403
python3 newton_krylov_refactor_20261015/performance_20261016/summarize_benchmarks_final.py
```

The summarizer reproduces the declared original three-sample cohort, not newly appended samples. A new timing campaign should declare a fresh cohort before collection rather than pool host-load windows. Separate controls use `run_tuning.py --version adaptive --case sa --layout 1x1 --repeat 3` or `--version accurate_sa`. Exact profiling compiler/link commands and source patches are in `jacobian_cost/build_commands.json`, `profile_final/`, `profile_jacobian/`, and `krylov_memory/release_results.json`. The instrumented source was restored and rebuilt; `profile_jacobian/release_restore.json` verifies that the executable again matches the pinned final hash. All raw profiles and command manifests remain available.

Remaining qualification: fully converged larger-mesh performance, native CFL retuning, AD/GPU runtime and race-detector coverage were not performed. Remaining implementation costs include repeated live boundary/dependency/gradient work, lower-block matrix-free preconditioning and duplicated snapshots. Stronger preconditioning or multilevel work needs larger-mesh evidence; the present small cases do not establish mesh-independent convergence.
