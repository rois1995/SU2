# Sensor interpolation and geometric BL transition assessment

This assessment follows the validated nodal gradation checkpoint d980963c677c3224b3ec49456d6b28f63807c8d4 on codex/metric-robustness. It adds an offline diagnostic and one small test of the actual native donor query. No production interpolation, gradation, BL, flow, Hessian or driver policy changes. All 759 production source fingerprints still match that checkpoint.

## What the production code does

CNativeField2D::FieldPatch::Blend interpolates the frozen sensor tensor components with barycentric weights. Its evaluate method then applies geometric composition at the actual query point. CSolver::ComputeMetric performs the same P1 sensor interpolation before ApplySample at its BL quadrature points. These paths agree on representation and composition order.

The sensor-only nodal transport certificate tests donor-edge endpoints. Its constraint is not a certificate for every subsegment of the P1 interpolant. Running more rounds of the same endpoint test cannot detect this representation gap once those endpoint constraints pass.

[Alauzet (2010)](https://doi.org/10.1016/j.finel.2009.06.028), Section 2.3.2, equation (3), describes log-Euclidean interpolation; Section 4.1, equation (9), gives the frozen-source transport law used here. Section 6 discusses limitations of edge-based gradation approximations. The comparisons and derivative checks below are our measurements and analysis; the paper does not establish that swapping interpolation will certify our query field.

## Interpolation comparisons on saved RANS sensor fields

The offline tool compares arithmetic P1, log-Euclidean, inverse-tensor and inverse-square-root interpolation. It preserves the saved vertices and checks SPD, endpoint reconstruction, orthogonal covariance and an analytical isotropic example. At each edge sample it checks transport and domination of the current P1 field. An additional centroid-density integral uses the same estimator for each candidate; it is not the production geometric-BL complexity integral.

GG at complexity 12000, four equal subsegments per original edge, 438832 directed samples:

| Interpolation | Maximum transport ratio | Samples above 1+1e-5 | Minimum domination of current P1 | Centroid sensor integral / P1 |
|---|---:|---:|---:|---:|
| P1 | 1.919296 | 51269 | 1.000000 | 1.000000 |
| Log | 1.160145 | 29748 | 0.521159 | 0.979904 |
| Inverse | 1.335164 | 45223 | 0.306373 | 0.960558 |
| Inverse square root | 1.014513 | 4391 | 0.394447 | 0.973080 |

These lower residuals are comparison results, not implemented improvements. Every alternative still violates sampled transport and weakens some directions of the declared P1 donor target. The GG inverse-root minimum corresponds to a size increase of about 1.59 in a worst direction relative to P1. The comparator is the currently stored graded sensor field, not separately exported ungraded sensor demands; this does not by itself prove loss of a particular CFD error target. That distinction must be resolved before accepting a new interpolation policy.

The behavior persists across all six GG/WLS/QR fields, at complexities 12000 and 60000:

| Field | P1 maximum ratio | Log maximum ratio | Inverse-root maximum ratio | P1 maximum local slope / allowance |
|---|---:|---:|---:|---:|
| GG, 12000 | 1.919296 | 1.160145 | 1.014513 | 6.637798 |
| WLS, 12000 | 1.742593 | 1.152693 | 1.004623 | 5.584412 |
| QR, 12000 | 1.863453 | 1.150322 | 1.013772 | 6.258809 |
| GG, 60000 | 2.645245 | 1.280418 | 1.044319 | 12.200233 |
| WLS, 60000 | 4.911065 | 1.542776 | 1.028822 | 41.020638 |
| QR, 60000 | 3.497777 | 1.478486 | 1.038432 | 20.656643 |

The independent P1 quarter-edge counts and maximum agree with the preceding audit. GG checks with 16 and 64 subsegments also retain violations; they inspect 1755328 and 7021312 directed samples. At 64 subdivisions P1's maximum ratio is 1.127128 and inverse-root's is 1.000843. Ratios approach one as distances shrink even for an excessively steep field, so raw maxima at different sample spacings are not directly comparable.

## Local slope check and the cost of keeping P1

For an affine edge M(t)=A+tD with physical edge vector e and g=log(ADAP_HGRAD), a necessary infinitesimal transport condition is

rho(M^(-1/2) D M^(-1/2)) <= 2 g sqrt(e^T M e),

where rho is the largest absolute eigenvalue. Both travel directions are checked. The diagnostic evaluates this at both endpoints. A violation proves that sufficiently small adjacent queries violate the exact transport bound; passing these sampled necessary conditions is not a whole-cell certificate. This derived check avoids mistaking small finite-step ratios for a smooth field.

At GG complexity 12000, 36198 endpoint-edge pairs exceed 1+1e-5 in this slope ratio. The maximum is 6.637798 on donor edge 6701--14042 at endpoint 14042. This also localizes the worst quarter-edge sample. The analysis is independent of nodal stopping tolerance and uses an explicitly supplied hgrad rather than a hardcoded benchmark value.

The new NativeField2D regression uses an isotropic unit-edge example: fine size 0.01, coarse size 0.01+log(1.3)=0.272364. All directed nodal transport checks on its donor triangle pass, but the real P1 query between x=0.75 and x=1 fails. Interpolating inverse square roots gives a linear size law and satisfies this isotropic example; rotating anisotropic tensors still fail in the real comparisons.

With the fine endpoint fixed, preserving P1 and satisfying its local size derivative requires m_f-m_c <= 2g L m_c^(3/2). The largest permissible coarse size in the example is 0.038328, about seven times smaller than the endpoint-only size, with a 50.497-fold increase in the coarse tensor eigenvalue. This is an illustrative two-point calculation, not the cost or result of a complete complexity solve. Stronger P1 constraints may be expensive and may consume much more complexity. Simply tightening hgrad or running more existing sweeps is not a demonstrated remedy.

## BL fade is a separate constraint-policy issue

An independent flat-wall counterexample uses the existing actual-position BL formula and a perfectly uniform sensor. First height is 1e-5, BL growth 1.2, thickness 0.02 and hgrad 1.3. The full constraint ends at distance 0.018 with normal size 0.003281818; it fades over the remaining 0.002.

With uniform background size 0.05, global normal-size gradation would require at least (0.05-0.003281818)/log(1.3)=0.178066 of transition distance. The current 0.002 fade gives endpoint transport ratio 172.535188. The sensor is uniform, so this discrepancy cannot be fixed by sensor interpolation or its nodal sweeps. An additional uniform size-20 example has an even larger mismatch; these are synthetic illustrations, not statements about the actual background sizes on every RAE wall-normal ray.

Retaining the first-height constraint, returning exactly to an arbitrary coarse background at the prescribed narrow band boundary, and enforcing global hgrad everywhere can be incompatible requirements. Smoothstep fading makes the metric continuous but does not bound its growth. The local BL growth rule and global composed-field gradation must have separate acceptance criteria unless the geometric policy explicitly supplies a sufficiently wide transition or additional background refinement. Widening the band or propagating vertex BL tensors is not implemented here and could recreate the unwanted interior refinement on coarse Euler donors.

The flat-wall reference uses the existing independent GeometricWall oracle; its endpoints are cross-checked against the analytical normal-size formula. The oracle's smoothstep/log-eigenvalue fade matches the inspected production ApplySample implementation. This is not a runtime certificate of arbitrary wall geometry.

## Validation, branch alignment and next work

The existing NativeField2D selection, including the new actual-query counterexample, passes seven cases per rank at one, two and four MPI ranks. The CLI checks preserve a constant non-diagonal tensor at hgrad 1.17 and reject hgrad <= 1. Eight real-field comparisons pass their diagnostic self-checks. No sampling result is promoted into a continuous certificate. A first MPI attempt was blocked before startup by sandbox loopback restrictions; the authorized local-MPI rerun passes. A reader-attribute probe and ANSI summary-parser issue were corrected without changing acceptance criteria; the events are retained.

A read-only comparison with codex/native-unsteady-performance at e6995fbff565955a5677ffcbcc66af9acf96a8f1 confirms FieldPatch::Blend is identical. Its donor search/cache code has advanced independently. This assessment adds no numerical conflict with those changes. The other branch was not built or modified; its submodule metadata prevented a full default status query, so only status ignoring submodules and the relevant source comparison were used.

Next, prototype interpolation-aware sensor constraints on manufactured cases before extending the production complexity loop. First measure whether keeping P1 can meet subedge and within-cell checks without excessive refinement or runtime. Compare any representation change against separately pinned ungraded sensor demands, rather than silently redefining their contract. Require one shared representation for quadrature, immutable donor queries and independent audits, then verify complexity consistency, MPI, hard bounds and cost. Keep geometric BL composition at actual positions. Resolve the BL transition's distinct acceptance policy with the geometric-BL work; do not claim global composed gradation from local BL growth or nodal sensor convergence.

CFD convergence, native 3D remeshing, mixed meshes and convective/viscous-gradient transfer remain outside this assessment. There is no additional production speedup to report; the preceding validated 13% metric improvement remains unchanged. The new diagnostic is offline and adds no adaptation runtime or persistent memory.

The tracked receipt is integration_evidence/native_interpolation_assessment_v1_validation.json. Full inputs, diagnostics, test binary, source snapshots, literature pin, failures and plots are saved under /media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/native_interpolation_assessment_v1. The standalone figure is interpolation_and_BL_transition.pdf in that archive.
