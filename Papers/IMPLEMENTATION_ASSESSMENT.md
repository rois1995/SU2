# Published methods and next SU2 recovery improvements

The supplied papers support several recovery methods with different strengths.
Vallet et al. favor direct quadratic fitting on their test meshes; Alauzet and
Frazza use two passes of a volume-weighted Clément projection for RANS; Picasso
et al. show that recovery can fail to converge on particular mesh topologies.
The next recovery choice should therefore come from a comparison on the same SU2
fields, meshes and computation budget.

The PDF inventory contains 16 files in
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/Papers`. The Kamenski and Huang paper
listed in `READING_LIST.md` is not currently among those files; its
[public preprint](https://arxiv.org/abs/1211.2877) remains available. The assessment
uses the specific sections listed below. PDF hashes and section pointers are
retained in the validation receipt.

## Recovery methods and their implementation implications

| Source and section | Finding | SU2 action |
|---|---|---|
| [Alauzet and Frazza 2021](https://doi.org/10.1016/j.jcp.2021.110340), Section 7.3, PDF page 34 | Compute exact P1 gradients per simplex, volume-average them onto vertices, apply that operator again, and average the two mixed derivatives. The authors report robustness on highly anisotropic meshes. | Benchmark this Clément operator against WLS and QR before adding more QR heuristics. The current dual-control-volume Green–Gauss code must not simply be assumed equivalent, especially at boundaries. |
| [Vallet et al. 2007](https://doi.org/10.1002/nme.2036), Sections 3.5 and 6 | Direct quadratic fitting differentiates a scalar patch polynomial twice. Enlarged patches improve boundary robustness in their comparison; boundary convergence remains weaker than interior convergence. | The existing direct QR path belongs to this method family, but its anchored weighted fit, SVD scaling and partial cubic correction differ. Test the boundary and interior separately; retain rank checks and supported patch growth. |
| [Diskin and Thomas 2008](https://ntrs.nasa.gov/api/citations/20090007493/downloads/20090007493.pdf), Sections 3, 4.2 and 6 | Higher-order terms in the large-spacing direction and approximate mapping with a distance function can address stretched/curved gradient errors. Flat-panel distance errors can themselves damage reconstruction. The analysis excludes boundary effects. | Compare a distance-based reconstruction against the current affine whitening in a manufactured curved layer first. This supports the investigation of directional enrichment, not the exact wall-only cubic basis or its thresholds. |
| [Picasso et al. 2011](https://doi.org/10.1137/100798715), Section 4 | Recovery accuracy depends strongly on topology; general convergence is not assured by their 2D/3D results. | Keep multiple connectivities, distorted stencils and refinement checks. A finite tensor or successful QR rank check is insufficient. |
| [Guo, Zhang and Zhao](https://arxiv.org/abs/1406.3108), Sections 2.2 and 3 | Recover gradients by polynomial fitting and recover Hessians by applying gradient recovery twice, under finite-element assumptions. | Compare the repeated operator with direct fitting; do not transfer its superconvergence guarantee to FV RANS or our incomplete cubic basis. |
| [Galbraith et al. 2020](https://doi.org/10.2514/1.J058783), Sections V and VII.B | The refine L2-projection path extrapolates interior Hessians to boundaries. Their boundary-layer benchmark exposes higher errors for a recovery path without specialized boundary treatment; gradation can spend additional DOFs compensating for weak boundary recovery. | Compare controlled interior extrapolation with direct boundary fitting. Measure interpolation/output errors at fixed complexity; stronger gradation is not evidence that Hessian recovery improved. |
| [Tsolakis et al. 2021](https://doi.org/10.2514/1.J060270), parallel-system descriptions and scaling discussion | Different operation scheduling, partition handling, migration and communication choices produce different scaling limits. | Preserve complete owner neighborhoods for QR and measure communication volumes. Our 1/2/4-rank checks establish local partition consistency, not large-scale MPI scalability. |

## Symmetry support implemented after the assessment

The direct QR path now supports mirror-even primal scalar sensors on symmetry
markers, using the same normals and boundary rule as the existing WLS/GG paths.
One-sided samples are fitted as before, then the scalar gradient is corrected and
normal–tangential Hessian components are removed. Normal-normal curvature remains.
Euler and viscous walls retain their one-sided treatment. The method remains
unavailable for periodic, goal and differentiated configurations.

For a planar symmetry with unit normal n, let R = I - 2nn^T be its reflection.
An even scalar has g = Rg and H = RHR at the plane. The corresponding projections
are g <- (g + Rg)/2 and H <- (H + RHR)/2. This elementary derivation explains why
mixed normal–tangential curvature is removed while normal-normal curvature is
retained; it is not a claim that a cited paper supplies our exact implementation.

The implementation reuses `correctGradientsSymmetry` and the existing Hessian
gradient workspace. It adds work proportional to the number of symmetry vertices
and selected sensors, without adding a reconstruction ring, MPI exchange or
per-mesh Hessian buffer. Tests cover oblique planes, two orthogonal planes,
Euler-wall junctions, signed curvature, a smooth nonquadratic even sensor and both
noise settings in 2D/3D. General curved/nonorthogonal symmetry configurations have
not been independently certified by this campaign. Selected custom sensors must
be mirror-even, as required by the existing sensor contract.

## Prioritized next comparisons

1. Benchmark Clément recovery and a controlled boundary extrapolation on the
   current manufactured curved layers and the frozen RAE/M6 fields. Compare
   directional errors, spectral directions, cost and partition consistency.
2. Prototype a distance-based fit on analytic curved geometry. For q = q(x),
   physical Hessians require Hx = J^T Hq J + sum_a (gq)_a Hessian(q_a).
   Omitting the mapping-curvature term gives an incorrect Hessian even when the
   fitted field is linear in wall distance. This chain-rule requirement is our
   mathematical assessment; the 2008 report studies gradients, not this Hessian
   extension. Validate polygon/reference distance accuracy before integrating it.
3. Use accepted adaptation/flow cycles to measure interpolation error, shock
   location, wall resolution and forces at fixed complexity. Frozen MPI/SPD
   checks provide no exact CFD Hessian or aerodynamic accuracy reference.
4. Profile avoidable fallback preparation: QR currently computes WLS derivatives
   first even when every QR fit succeeds. Any optimization must preserve failed
   fits, corrected neighbor gradients and halo values without adding an expensive
   permanent cache.

On the frozen M6 field, 105281 of 192504 final point-sensor fits retain a relative
weighted residual above 0.05; the mean is 0.092339 and the maximum is 0.998883.
Zero rank failures therefore do not establish recovery accuracy. With noise
strength 1, all 192504 final fits are filtered. Residuals contain truncation and
physical flow features as well as noise, so the default remains zero until
feature/output preservation is validated.
