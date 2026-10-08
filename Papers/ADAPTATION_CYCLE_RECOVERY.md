# RAE2822 matched recovery adaptation cycle

One native adaptation cycle from the saved RAE2822 RANS SA restart completed with both old and new Green–Gauss recovery. Both resulting meshes passed independent geometry, first-height, frozen sensor plus geometric BL, solution admissibility and output-pairing checks. The new run finished with a 4.72 times smaller turbulence residual after 2000 iterations on nearly the same number of points. Both met the density criterion; neither met the turbulence criterion. This single pair establishes a successful real adaptation and continuation and a promising residual result, not improved aerodynamic accuracy or grid convergence.

## Matched controls

Baseline executable source: `747f0ef6ead0d1fe361f02cbedd1c86e0e933d10`. Improved executable source: `7cfc984277755a7b1369f9e3d168e9ad4e5f680b`. Both use the accepted integrated remesher base `2abbd11769`; 66 remesher, BL, transfer, adaptation and driver files were checked identical. The recovery sensor file is deliberately excluded from that common-file list. The other agent's newer `99aa0d7f72` performance/worker/partition implementation was not used. Production files were unchanged during this campaign.

Inputs are `nativefix_rans_euler_seed_v21/mesh_adap_00002.su2`, its `.native_ref` sidecar and `solution_adap_00002.dat`. Their copies and configurations are byte-identical between cases. The initial single flow iteration yields bitwise-identical coordinates and density, momentum, energy and SA fields before remeshing. All recovery and metric differences therefore start from the same solution.

Both runs select `NUM_METHOD_HESS=GREEN_GAUSS`, keep flow gradients WLS, disable noise, use conservative transfer and geometric BL with h0=1e-5, growth=1.2 and thickness=0.02. The composed complexity target is 12000 with hmin=1e-6, hmax=20, ARmax=10000 and hgrad=1.3. Each run uses four MPI ranks, one computational-library thread and nice 10; runs and independent audits are sequential. This cycle exercises GG simplex consistency and geometry reuse; it does not exercise the optional QR wall quartics.

## Measured results

| Quantity | Baseline | Improved |
|---|---:|---:|
| Adapted points | 15945 | 15928 |
| Adapted triangles | 31316 | 31283 |
| Independent minimum metric quality | 0.360178299 | 0.361313374 |
| Independent maximum Simpson metric edge length | 1.799846859 | 1.799947117 |
| Quality / length failures | 0 / 0 | 0 / 0 |
| Maximum relative first-height error | 2.9e-12 | 3.83e-12 |
| Final log10 density residual | -8.419764 | -8.332433 |
| Final log10 SA residual | -6.857260 | -7.531627 |
| CL | 0.7237685196 | 0.7241585199 |
| CD | 0.01379211046 | 0.01379978027 |
| Whole process seconds | 139.828 | 130.398 |

The quality gate is q >= 0.18 and the edge gate is L <= 1.8. Exact first height, positive cells, manifold marker topology, original reference features and restart/VTU scalar pairing pass. Density, pressure and internal energy remain positive; SA values remain nonnegative. The reported transfer-integral changes are identical to printed precision, with density/energy about -1.819e-10, dominated by the measured boundary/domain-volume change. Saved plots are `baseline/mesh_and_mach.png`, `improved/mesh_and_mach.png` and the corresponding `surface_cp.png` files.

The improved density residual is 1.223 times the baseline value, although both are below 1e-8. Both SA residuals are above 1e-8. CL changes by 0.00039 and CD by 7.66981e-06; there is no independent force-accuracy reference here. Mesh quality is effectively unchanged. The smaller SA residual is one matched observation, not a guarantee across meshes, methods or MPI partitions.

## Performance and remaining limits

Hessian recovery immediately before remeshing takes 0.001962 versus 0.004866 seconds. The consistent recovery costs a few extra milliseconds relative to the original GG operator. Metric construction takes 19.3717 versus 17.7244 seconds, dominated by repeated full gradation during the complexity solve: both have 78 trials, 6015 sweeps and 111824865 owner-point visits at this stage. Final output metric construction needs 114 trials / 8961 sweeps in the baseline versus 79 / 6222 in the improved case. These workload differences contribute to elapsed-time differences. Single sequential timings under contention do not establish a repeatable speedup.

The steady driver calls `ComputeMetric()` at the end of cycle 0 and again in `RemeshFromMetric()` before consuming the same solution. The two logs show identical fields/work statistics and about 18–19 seconds each before native adaptation. Avoiding the duplicate evaluation is a concrete performance candidate, provided lifecycle invalidation and output/remesher equivalence remain correct. Native geometric integration already caches geometry samples within a metric solve; adding an unrelated geometry cache is not the immediate remedy.

Every final sensor gradation reaches its 80-sweep limit. Immediately before remeshing, the largest sensor transport ratio is 1.00017 with 103 failing directed edges in the baseline and 1.00016 with 104 in the improved field; the composed BL nodal maxima are 96.0982 and 98.9903. Produced meshes still pass their independent q/L/height/reference gates. These are separate checks: the cycle does not certify continuous or composed-field gradation, and no wall tensors were spread into donor vertices to hide the residuals.

The continuation auditor now reuses the existing original-wall sidecar reader through `airfoil_reference_audit.original_wall`; previously it built its geometric BL target from current adapted wall chords. This matters for restarts with retained original geometry. `run_metric_cycle_pair.py --self-check` checks sidecar and no-sidecar paths against the actual original wall plus metric-intersection math. No production metric, BL or remesher policy was changed for the audit.

## Next steps

1. Repeat the comparison with the validated recovery changes on the same accepted newer remesher revision as its control, then on the merged main branch.
2. Remove the duplicate steady pre-remesh metric computation with an explicit lifecycle/equivalence check; measure the actual cycle saving.
3. Reduce repeated graded complexity work while retaining the geometric BL target, finer sensor demands and convergence/transport reporting. Simply lowering the sweep cap would leave the measured residual unresolved.
4. Assess the SA residual plateau and forces across further adaptation cycles or a matched reference solution. Keep any future convective/viscous-gradient transfer a separate validated change.

Raw evidence, copied inputs, configurations, final states, plots, audit reports, executable pins and source checks are preserved under `/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/adaptation_cycle_pair_v1`. The compact receipt is `integration_evidence/adaptation_cycle_pair_v1_validation.json`; `comparison.json` retains recovery-field quantiles and all cycle measurements. This is a 2D triangular native RANS test, not a 3D/mixed-element or aerodynamic accuracy certificate.
