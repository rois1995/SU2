# Native sensor gradation: convergence and active stencils

This pass starts from 2f532a2a2e65954dca68cfe769be53919ad1e7a9 on codex/metric-robustness. It follows NATIVE_GRADATION_WORK.md and fixes its remaining nodal sensor violations. Only CSolver::ComputeMetric production code changes. Geometric BL evaluation, Hessians, flow gradients, remeshing and steady/unsteady driver behavior retain their preceding implementations. CFD residual convergence belongs to the other session and is outside this work.

## Diagnosis and implementation

The saved four-rank GG field at complexity 12000 had 104 violating directed sensor edges, affecting 162 vertices, with maximum transported-metric ratio 1.000157758799. The affected eigenvalues were strictly within the configured size bounds, their largest aspect ratio was 29.88 against a bound of 10000, and none of the affected vertices was a wall/farfield vertex. Independent NumPy continuation of the saved field removed the violations above 1+1e-5 after 20 additional rounds and stopped attempting corrections after 47. This diagnostic performs no complexity refit, MPI exchange or BL composition; it identifies slow propagation rather than proving production equivalence.

The production solver now revisits a vertex's expensive stencil only when its own tensor or a neighbor's tensor changes. The first round visits every vertex. Changes are compared after halo exchange; previous-round tensors and global-ID neighbor order remain synchronous and deterministic across ownership. A vertex with an attempted correction is retried even if hard bounds leave its tensor unchanged, so clipping cannot create a false convergence certificate.

The existing transported-demand check and intersection remain. Stop when no rank attempts a correction at the existing 1e-7 demand threshold, replacing the relative-change eigensolve as the stopping proxy. The safety cap increases from 80 to 256; this is a maximum, not a prescribed round count. Pathological propagation or incompatible bounds can still reach the cap and must not be called converged. Eligibility scans and full halo exchanges remain each round; this is an active-stencil implementation, not sparse communication or a distributed work queue. It adds one temporary bitset and no permanent cache, option or MPI collective per round.

The transport formula comes from [Alauzet (2010), Size gradation control of anisotropic meshes](https://doi.org/10.1016/j.finel.2009.06.028), Section 4.1, equation (9), and Section 5.1. Section 5.1's dynamic edge list motivates avoiding unchanged work. This synchronous vertex frontier is an implementation choice, not a reproduction of that list or a claim that the publication proves MPI convergence.

Only sensor tensors enter propagation. Original-wall BL constraints are still evaluated at each actual query position and intersected with the interpolated sensor, retaining finer sensor demands and their coupling. The new frontier does not interpolate a vertex BL composite or impose a prescribed normal size over the sensor.

## Measured improvement and checks

Three alternating, sequential, four-rank GG timing pairs used identical frozen RAE2822 RANS/SA inputs, noise off, complexity 12000 and a quiet machine. Both versions use 29 complexity trials. The control is the already optimized preceding implementation, not the original legacy solver.

| Quantity | Previous implementation | This pass |
|---|---:|---:|
| Metric seconds, three runs | 5.89361, 5.82788, 5.75141 | 5.06883, 4.99775, 5.09225 |
| Median seconds | 5.82788 | 5.06883 |
| Expensive owned-point stencil visits | 42406071 | 20301421 |
| Total synchronous rounds across trials | 2281 | 3615 |
| Accepted field rounds | 80, cap reached | 127, fixed point |
| Directed nodal violations above 1+1e-5 | 104 | 0 |
| Maximum nodal transported-metric ratio | 1.000157758799 | 1.000000099997 |

Median metric time falls 13.02%, with 52.13% fewer expensive stencil visits despite more synchronization rounds. This is a case-specific measurement; MPI reduction/halo costs can rise. Saved fields repeat bitwise within each version. Coordinates, flow state, sensors and Hessians match between versions bitwise. The maximum relative tensor difference is 0.000153053, consistent with correcting previously unsatisfied demands.

All 18 frozen combinations pass the unchanged independent acceptance checks: GG, WLS and QR; complexity 12000 and 60000; one, two and four ranks. They have zero nodal sensor violations above 1+1e-5, SPD tensors, configured size/aspect bounds, preserved restart state, reported complexity within its existing tolerance, and finer-sensor/BL domination. The largest cross-rank metric difference is 3.18e-12. Accepted 12000 fields require 127 rounds for GG, 119 for WLS and 107 for QR. Production complexity tolerance remains 1e-6; the printed-output audit uses 1e-5.

The serial adaptation/gradient/Hessian/restart selection passes 407540 assertions in 154 cases. MPI selections pass 38 cases per rank on two and four ranks. Existing 2D/3D native sensor tests cover coupled anisotropy and bounded infeasible targets. A new distributed 2D regression requires propagation beyond 80 rounds and compares the final horizontal-row sizes with the analytical shortest-path size envelope. It checks a prescribed complexity target and halo tensors. Full CSolver.cpp forward/reverse AD syntax checks with MPI/OpenMP pass; AD runtime/derivatives are not certified.

One four-rank remesh/transfer smoke check passes mesh validity, restart/VTU pairing, state admissibility, original wall features/sidecar geometry and the independent original-P1-sensor plus geometric-BL quality/length audit. It produces 15929 points and 31285 triangles, minimum metric quality 0.361313512 and maximum Simpson edge length 1.799947133, with zero failing quality cells or length edges. Maximum relative first-height error is 2.35e-12. Initial and adapted nodal sensor fields converge in 127 and 126 rounds. One flow step per mesh is used solely to exercise integration/admissibility; it is not a convergence experiment. Smoke-check wall time is not part of the matched performance evidence.

## Remaining gap and next steps

Nodal sensor convergence does not certify gradation inside donor cells. An independent P1 quarter-edge diagnostic still finds 51269 violating directed samples, with maximum ratio 1.919295524; the control has the same count and maximum 1.919296096. This pre-existing gap is essentially unchanged. Linear interpolation of tensor components does not preserve a Lipschitz size bound because directional size depends nonlinearly on the tensor. The diagnostic samples edges; it is not an exhaustive continuous certificate.

Next, characterize actual-position sensor interpolation and assess a consistent interpolation/constraint policy for complexity integration and remesher queries. A policy must preserve SPD, finer sensor demands, MPI consistency and acceptable cost. Coordinate geometric BL transition/fade handling with the agent maintaining that implementation. Existing composed sensor-plus-BL transport diagnostics also report violations; successful domination, mesh quality and first-height checks do not certify composed-field gradation. Propagating fine BL tensors through donor vertices is not an acceptable workaround.

The real remesh check is 2D triangular RANS. The 3D tests validate sensor metric computation on tetrahedra, not native 3D remeshing or 3D geometric BL behavior. General mixed meshes and transfer to convective/viscous gradients remain separate work; see GRADIENT_TRANSFER_FOLLOWUP.md. Large-scale MPI communication optimization remains unimplemented.

The tracked receipt is integration_evidence/native_gradation_residual_v1_validation.json. Full inputs, binaries, source fingerprints, scripts, logs, diagnostics, outputs and audit dependencies are preserved physically under /media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/native_gradation_residual_v1/validated. The earlier preflight directory remains as an explicitly unvalidated historical snapshot. Idle waiting controllers were deliberately stopped/resumed during shared-machine contention; their logs remain, and those terminations are not production gate failures.
