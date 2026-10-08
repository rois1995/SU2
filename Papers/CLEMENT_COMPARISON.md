# Clément and boundary-extension comparison

The paper-based comparison does not justify replacing SU2's recovery defaults.
On the tested simplex meshes, Clément and the existing Green–Gauss operator agree
in the central interior to roughly 1e-7 in layer coordinates. Their boundary
behavior differs substantially. A separate Clément backend would duplicate much
of the interior work; the useful next implementation target is the boundary
operator and donor selection.

## What was implemented and checked

`integration_evidence/compare_hessian_recovery.py` implements the two-pass,
volume-weighted P1 recovery described by Alauzet and Frazza (2021), Section 7.3.
It includes two controlled constant boundary extensions: volume-average adjacent
interior donors, or first exclude donors touching physical boundaries. Extension
proceeds in synchronous graph layers, reports missing donors, and replaces only
physical boundary Hessians. This is a benchmark of the operation described by
Galbraith et al. (2020), Section V, not an exact port of refine's extrapolation.
Planar even-scalar symmetry follows the existing SU2 projection rule in M6.

The existing pressure-sensor test exports matched meshes, scalar fields and
native GG/WLS/QR Hessians through `[HessianRecoveryComparison]`. There are 36
manufactured datasets: 2D/3D, straight/curved graded layers, n=8/16/32 and three
native methods. All have aspect ratio 1000 and rotation 0.37 radians; the 3D
meshes also have spanwise skew 2. Errors are measured after mapping the tensor to
local layer coordinates, against analytic derivatives including curvature terms.
The same meshes and sensor values are verified across the three methods.

Checks cover exact affine P1 derivatives, signed simplex orientation, volume
conservation, constant-tensor extension, missing donors, symmetry projection,
flat-interior accuracy and curved-interior convergence. The export test checks
that the native results are finite; finiteness is not an accuracy gate. No
production recovery configuration or BL composition changed in this checkpoint.

## Accuracy measured at identical meshes and fields

The following are maximum component errors in the central interior of the
curved-layer cases. They are derivative errors, not flow-output errors.

| Case | Clément | Native GG | Native QR | Native WLS |
|---|---:|---:|---:|---:|
| 2D, n=8 | 0.07808 | 0.07808 | 0.03305 | 18.81 |
| 2D, n=16 | 0.02458 | 0.02458 | 0.01090 | 1483.53 |
| 2D, n=32 | 0.006505 | 0.006505 | 0.003045 | 26146.56 |
| 3D, n=8 | 0.13272 | 0.13272 | 0.74106 | 10.45 |
| 3D, n=16 | 0.04787 | 0.04787 | 0.21034 | 319.81 |
| 3D, n=32 | 0.01350 | 0.01350 | 0.05482 | 2084.62 |

Clément/GG are about four times more accurate than QR in the central interior of
the finest curved 3D case; QR is about twice as accurate there in 2D. WLS errors
increase on these particular stretched, rotated, graded stencils. This does not
establish a universal ranking or validate a new hybrid policy.

At n=32, wall-interior maximum component errors are:

| Case | Native GG | Raw Clément | Clément with deeper-donor extension | Native QR |
|---|---:|---:|---:|---:|
| Curved 2D | 80.35 | 1.779 | 0.2233 | 0.004561 |
| Curved 3D | 673.29 | 6.400 | 0.3006 | 0.08304 |

The deeper extension is exact to roundoff on the flat wall examples. On the
curved graded wall, its maximum relative normal errors plateau: 2D approximately
5.88%, 5.58%, 5.58%, and 3D approximately 7.35%, 7.15%, 7.51% at n=8/16/32.
Interior status alone therefore does not establish donor quality. Boundary
extension also leaves adjacent interior Hessians unchanged; repairing the
boundary Hessian alone does not repair every derivative influenced by the first
recovery pass.

An independent 1D calculation explains one mechanism. For a quadratic u=y²/2
sampled at y_i=(i/n)^a, volume-weighted P1 recovery gives interior gradients
G_i=(y_{i-1}+y_{i+1})/2. Applying the operator again gives, at ring i=2,
H_2=0.5(4^a)/(3^a-1). With a=1.3 this is 0.9559359834 rather than 1, independent
of n. The runnable check verifies this 4.4% bias at n=8/16/32. This deduction
follows from the recovery formula; it is not attributed to a paper or claimed to
explain every 2D/3D error above. The example is noise-free, so residual filtering
or stronger metric gradation cannot certify recovery accuracy.

## Frozen cases and performance limits

The same operator was evaluated on the frozen RAE2822 RANS and ONERA M6 RANS/SA
fields. All produced tensors are finite; both donor policies cover all 919 RAE
and 10837 M6 physical boundary points within three graph layers. Differences
from WLS/QR remain substantial. Neither frozen field supplies exact Hessians,
so the differences and eigenvalue extremes do not establish which method is
physically correct. There were no remeshing or flow iterations.

The serial NumPy prototype uses approximately 44.4 MB of cached simplex geometry
for M6 and takes approximately 4.5 s for both recovery passes including geometry
preparation. RAE uses about 1.6 MB and 0.13 s. These are prototype measurements;
they are not comparable implementation timings to native C++ WLS/QR and are not
MPI scaling results. Existing native timing evidence remains in the previous
QR-symmetry checkpoint. Production implementations should share per-call
geometry and existing gradient halo exchanges, rather than keep a permanent
full-mesh inverse cache.

## Next implementation choices

1. Correct and verify the adaptation GG boundary gradient operator, including
   propagation into the second pass. Use affine/quadratic consistency tests and
   the graded-layer maximum-error gates before selecting a default.
2. Test higher-order or distance-aware boundary reconstruction against the
   current QR boundary fit. Donor quality needs more than graph distance; a
   physical Hessian also needs the mapping-curvature chain-rule term.
3. Profile QR fallback preparation separately. Its existing WLS prepass costs
   only a fraction of total QR recovery, so quantify the savings before adding
   buffers or more communication.
4. Validate accepted choices on native MPI and adapted flow cycles at fixed
   complexity. This serial, fixed-mesh comparison certifies neither distributed
   recovery nor aerodynamic improvements.

The source, input hashes, exports and measured results are preserved under
`integration_evidence/metric_robustness/clement_comparison_v1` in SU2_AdapNoExt.
The original paper PDFs remain in the user's Papers folder and are not uploaded.

## Replay

Use an existing directory and a serial MPI run for the export; keep thread
counts at one. This benchmark does not require another CFD build.

```sh
mkdir -p /tmp/recovery-comparison
OMP_NUM_THREADS=1 SU2_HESSIAN_BENCHMARK_DIR=/tmp/recovery-comparison \
  mpirun -np 1 BUILD/UnitTests/test_driver '[HessianRecoveryComparison]'
OPENBLAS_NUM_THREADS=1 MKL_NUM_THREADS=1 \
  python3 integration_evidence/compare_hessian_recovery.py \
  --manufactured /tmp/recovery-comparison --output /tmp/recovery-comparison/report.json
```

Optional frozen comparisons use `--frozen LABEL run.cfg fields.csv`; the frozen
sensor export and original mesh must match exactly. The validation receipt keeps
the actual commands, native source/binary fingerprints and full comparison
report. The durable archive retains the manufactured meshes and fields.
