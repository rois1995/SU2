# Native metric gradation work reduction

Built from the validated steady metric-reuse checkpoint f4017c9d705709248e642b312a97ba30abc8d861. The optimization reduces repeated native graded-complexity work. It preserves geometric BL evaluation at actual candidate positions, sensor-only donor storage and intersection with finer sensor demands. No native remeshing, BL geometry, Hessian, flow-gradient or unsteady lifecycle code is changed in this pass.

## Implementation

Skip eigendecomposition/recomposition and the relative-change calculation when a vertex satisfies every transported neighbor demand and its tensor is unchanged. The input SPD validation, neighbor tests, size/aspect bounds for changed tensors, synchronous halo exchange, global-ID order and convergence thresholds remain. A temporary bitset marks changed owned points; no persistent WLS/geometry cache is added. A diagnostic counts actual tensor updates alongside all owned-point visits.

After corner constraints change, start the native complexity solve from its previous scale rather than the unbounded estimate. Keep the same safeguarded bracket, hard endpoints, Illinois solver and 1e-6 complexity tolerance. Reuse the last evaluated field when its scale is the accepted scale; retain a refresh guard for a selected endpoint that was not last evaluated. MMG retains its previous starting guess and finalization path. This warm start changes the numerical stopping point, so the final metric is not bitwise identical to the control.

The unchanged transport/intersection is supported by [Alauzet (2010), Size gradation control of anisotropic meshes](https://doi.org/10.1016/j.finel.2009.06.028), Section 4.1, equation (9), and Section 5.1, equation (11). Section 5.1 also describes a dynamic edge list to reduce repeated visits. The current changes are implementation work reuse; they do not implement that dynamic list or claim the paper certifies the synchronous MPI solver.

## Matched cost measurement

Three alternating four-rank frozen RAE2822 RANS/SA timing pairs use identical inputs and configuration, GG Hessians, noise off and complexity 12000. All runs are sequential at nice 10 with one library thread; controllers wait for shared SU2/build processes.

| Metric-build quantity | Steady-reuse control | Optimized |
|---|---:|---:|
| Seconds in three runs | 18.2606, 17.2081, 16.4553 | 6.25507, 7.18003, 5.92117 |
| Median seconds | 17.2081 | 6.25507 |
| Complexity trials | 78 | 29 |
| Total gradation sweeps | 6015 | 2281 |
| Owned-point visits | 111824865 | 42406071 |

Median metric time is 63.65% lower (2.75 times faster), with 62.08% fewer sweeps/visits. The optimized path performs 15273681 tensor updates in those 42406071 visits. All repetitions of each executable produce identical saved fields. Coordinates, solution, sensors and Hessians match the control bitwise; metric relative differences are at most 6.445e-6. The scalar complexity solve still uses 1e-6 tolerance; that is not a 1e-6 bound on every tensor component.

## Verification and real cycle

The existing serial selection passes 400081 assertions in 152 cases before expanding the native tests; its MPI selections pass on two/four ranks. Expanded native tests cover a coupled anisotropic bump and bounded infeasible complexity targets in both 2D and 3D, including halo tensors, at one/two/four ranks. Those final selections pass 22 serial and 19 cases per MPI rank. Full CSolver.cpp syntax checks pass with forward and reverse AD, MPI and OpenMP using the Meson AD definitions. This is not an AD derivative/runtime certificate.

Frozen GG, WLS and QR RANS cases at complexity 60000 pass the existing SPD, size/aspect, state preservation, reported complexity, finer-sensor/BL intersection and sensor-transport gates at one/two/four ranks. Largest cross-rank metric difference is 3.18e-12. The capped 12000 case also agrees across ranks within 2.97e-12, with bitwise Hessians and preserved state; its remaining transport violations are not accepted as a gradation certificate.

One matched four-rank native adaptation cycle completes 2000 resumed RANS iterations. The pre-remeshing flow state, Hessians and diagnostics match the steady-reuse control bitwise. It produces 15927 points / 31281 triangles versus 15928 / 31283 previously. Independent original-P1 sensor plus original-geometric-BL audits give qmin=0.361313586 and Lmax=1.799947141 with zero failures. Mesh validity, restart/VTU pairing, positive density/pressure/internal energy, nonnegative SA, original wall features and sidecar geometry pass. Maximum relative first-height error is 2.91e-12.

The cycle's two metric builds take 5.79504 and 5.56556 seconds, versus 19.6733 and 16.7390 in the saved control. Whole-cycle timing is excluded from performance evidence because a redundant validation probe overlapped part of the solve after interruption. The frozen matched timings above predate that overlap. CFD residual convergence belongs to the other session and is not evaluated here; this cycle validates the adapted mesh, metric constraints, pairing and admissibility.

## Remaining work and scope

The 80-sweep cap is retained. At complexity 12000 both frozen control and optimized metrics have 104 violating directed sensor edges; maximum transport ratios are 1.000157761 and 1.000157759. Their cost falls, not their residual. On the different adapted meshes, final-output sensor violations are 291 in the control and 299 in the new cycle. Geometric BL composed-field transport remains uncertified. Successful remeshed q/L/height checks are separate from continuous or composed-field gradation.

Next, localize the remaining sensor violations and distinguish slow propagation from active hard bounds. Assess an active vertex/edge frontier or another convergent propagation method, requiring deterministic MPI ownership/order, residual-based acceptance and complexity consistency. The dynamic edge list in Alauzet Section 5.1 is a concrete literature starting point. Avoid lowering the cap or propagating fine wall tensors through donor vertices to hide residuals. Any BL fade/transport policy change belongs with the agent maintaining geometric candidate-point BL evaluation and requires one shared query/integration policy.

The production cycle is 2D triangular native RANS. New 3D tests exercise sensor-only metric computation on tetrahedra with no geometric BL; they do not certify native 3D remeshing, 3D geometric BL complexity or general mixed meshes. Future convective/viscous-gradient transfer remains a separate task as recorded in GRADIENT_TRANSFER_FOLLOWUP.md. Reconcile these shared metric/driver improvements with the newer native-unsteady-performance branch without overwriting its restart/output/worker changes.

Raw configurations, input pins, executables, source patch/fingerprints, MPI results, failures, cycle outputs, plots and independent audits are preserved under /media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/native_gradation_work_v1. The compact tracked receipt is integration_evidence/native_gradation_work_v1_validation.json. Invalid initial test targets and missing AD include/definition probes are preserved and classified; no production acceptance gate was weakened.

A read-only comparison at newer native-unsteady-performance head e49c545d700632bd9c68fc52535ef63d6ca8632d confirms that its gradation/complexity/corner/final tensor regions still match this pass's starting point. The earlier compensated sensor normalization remains specific to this branch and needs separate reconciliation.
