# Adaptation Green–Gauss boundary recovery

This records the original simplex-recovery checkpoint 0eeb5a49ba. The newer
cell-pass optimization and separate QR wall-bias work, including measured cost,
additional temporary memory and rejected variants, are documented in
`RECOVERY_GEOMETRY_WALL_BIAS.md`. The historical measurements below are retained.

## Operator and publication basis

[Alauzet and Frazza (2021)](https://doi.org/10.1016/j.jcp.2021.110340),
Section 7.3 (PDF page 34), describes exact P1 simplex gradients averaged onto
vertices with incident-cell volume weights, repeated for Hessian recovery and
symmetrized. Our matched benchmark found the original dual GG operator agrees
with this projection in central simplex interiors but fails badly at physical
boundaries. This change uses the volume-weighted P1 projection at complete owned
triangle/tetrahedron stars in both adaptation recovery passes, with incident
cells accumulated by global ID. Existing GG provides the fallback on unsupported
stars; symmetry corrections and halo exchange are reused. No method enum,
persistent full-mesh geometry cache or extra communication is introduced.

A first boundary-only trial passed affine tests but failed the unchanged M6
MPI gates. Subtracting the central field value in interior GG accumulation
reduced its worst pressure-Hessian MPI discrepancy from 1.4e-7 to 1.1e-10,
but the resulting metric still differed by 7.1e-6. Very small Hessian eigenvalues
are sensitive to roundoff in dual normals and local accumulation. The complete
simplex projection instead computes both field differences and geometry from
the same original cell stars in stable global order. This deterministic
implementation is our numerical design; the paper does not certify MPI
bitwise consistency. Both rejected source patches and their receipts remain
with the final evidence. Gates were not relaxed.

The implementation sorts incident cells by global ID, equilibrates
coordinate columns, uses signed cofactors and absolute-volume weights, and
rejects an entire mixed/degenerate/nonfinite star before modifying its gradient.
The common determinant-volume factor cancels, avoiding inversion of each thin
simplex. It currently evaluates ordinary GG before replacing supported stars.
This preserves one existing fallback path but adds work, which is included in
the measured native timings. It uses a reusable local star buffer and bounded stack arrays, with no
persistent geometry cache or additional MPI exchange.

## Measured derivative improvements

The pressure-sensor-to-gradient-to-Hessian checks use 2D/3D, 1000:1 stretching,
rotation, grading and curved boundaries. Physical-boundary affine gradient
maximum errors fall from 18.18/121.04 to 1.85e-12/3.33e-12. Scaled affine
Hessian maxima are below 3.3e-13 across all owned points, with identical maxima
at 1/2/4 ranks. Reversed simplex orientation and whole-star degenerate fallback
are also exercised. The default generic GG invocation and the hexahedral opt-in
fallback remain bitwise identical to their old invocations in the tests.

The 36 manufactured datasets reuse exactly the previous meshes and scalar
values. WLS/QR exports remain byte-identical. At n=32, maximum Hessian component
errors in layer coordinates at wall vertices excluding wall edges are:

| Geometry | Original GG | Simplex recovery | Reduction factor |
|---|---:|---:|---:|
| 2D flat | 9.8194 | 2.3889 | 4.1 |
| 2D curved | 80.3480 | 1.7788 | 45.2 |
| 3D flat | 215.1278 | 6.3708 | 33.8 |
| 3D curved | 673.2942 | 6.4002 | 105.2 |

Curved central-interior errors remain 0.0065055/0.0134980, preserving the prior
operator's accuracy there. On flat interiors, roundoff maxima fall from
8.7e-9/1.1e-7 to 3.7e-10/8.5e-10. Boundary errors remain nonzero, including
large corner errors; this is not a convergent high-order wall reconstruction.
The QR curved-wall maxima from the paired dataset are still smaller at
0.004561/0.08304. There is no universal preferred recovery method.

## Frozen validation and cost

Both frozen RANS/SA campaigns pass their existing MPI, SPD, size/aspect,
complexity, retained-state and boundary-policy gates at 1/2/4 ranks. M6 has
96252 vertices/545438 tetrahedra; RAE has 18591 vertices/36263 triangles.
M6 Mach/pressure Hessians are bitwise identical across partitions and metric
relative differences are below 6e-15. The RAE geometric BL composition retains
both finer sensor and wall demands. WLS/QR M6 field dumps remain byte-identical
to their prior checkpoint. These are zero-iteration, zero-remeshing checks.

Three alternating serial old/new M6 pairs give median Hessian recovery times
0.05007 s versus 0.54881 s: about 11 times the old GG kernel, adding 0.499 s per
metric computation on this mesh. Four-rank recovery takes 0.282 s. Whole-process
medians are 10.634/10.626 s with overlapping variability; that does not establish
an end-to-end speed improvement or zero overhead. RAE serial recovery increases
from 0.00583 to 0.01936 s. Each RAE timing is one sample. All heavy work was
sequential, reduced priority, one thread, MPI at most four ranks. Machine
contention limits timing precision.

The extra work restores reproducible, affine-consistent recovery; it is not a
free speed optimization. Repeated per-vertex simplex geometry and the serial
star loop are performance follow-ups. Profile single-cell geometry reuse or
parallel accumulation under the same reproducibility/memory gates before
scaling this recovery or considering flow-gradient transfer. No persistent
per-cell inverse cache is allocated in this implementation.

Evidence is in `integration_evidence/gg_simplex_recovery_v1_validation.json`;
raw manufactured fields, frozen dumps, replay configs, binaries, source, checks
and both rejected trials are preserved physically under
`integration_evidence/metric_robustness/gg_simplex_recovery_v1` in SU2_AdapNoExt.

## Scope and limitations

Adaptation call sites opt in when `NUM_METHOD_HESS= GREEN_GAUSS`,
including GG derivatives used by custom sensor expressions. Ordinary convective
and viscous gradient callers retain the original operator. Periodic and goal
recovery remain unchanged. Non-simplex stars retain GG; this is not a new hybrid-
cell boundary recovery method. P1 star recovery requires complete owned volume
stars and its opt-in call sites run outside OpenMP parallel regions.
The star traversal is currently serial; large hybrid-MPI scaling requires a
separate performance assessment before any shared flow-kernel transfer.

This fixes affine boundary inconsistency, not quadratic boundary exactness.
The topology-dependent and self-similar graded-wall biases found in
`CLEMENT_COMPARISON.md` remain. No manufactured error reduction is a certificate
of aerodynamic accuracy or nonlinear RANS robustness. Forward/reverse AD syntax
checks do not replace differentiated-runtime or adjoint validation. Local
1/2/4-rank tests do not establish large-scale MPI scalability.

The original geometric BL intersection, candidate-point evaluation, gradation
and complexity policy are unchanged. Frozen RAE verification uses sensor-only
metrics and the existing independent geometric BL reference; its nodal checks
are not a continuous geometric-complexity/gradation certificate.

## Next work

Compare higher-order or distance-aware wall recovery using exact manufactured
curved layers and several connectivities before accepting an extrapolation
policy. Then compare fixed-complexity adaptation/flow cycles. Profile the
existing QR fallback preparation separately. The requested future convective
and viscous transfer investigation is recorded in
`../GRADIENT_TRANSFER_FOLLOWUP.md`; each scalar, vector, limiter and wall policy
needs its own flow validation.
