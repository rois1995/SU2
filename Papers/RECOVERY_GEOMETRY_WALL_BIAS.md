# Recovery geometry cost and remaining wall bias

## Element support and experimental scope

The deterministic GG P1 projection remains restricted to complete triangle or
tetrahedron owner stars. Vertices touching a quad, prism, pyramid or hex retain
legacy GG. WLS stabilization/reuse, point-based QR/noise/owner-neighborhood
handling and metric tensor algebra are not inherently simplex-only; that does
not certify their accuracy on arbitrary mixed meshes. The requested mixed-grid
and eventual convective/viscous follow-up is in `../GRADIENT_TRANSFER_FOLLOWUP.md`.
The manufactured/frozen gains below apply to the tested simplex grids.

## Geometry work reduced

The GG projection now visits locally available volume cells in global-ID order,
evaluates equilibrated geometry and field contributions once per cell/pass,
and scatters to owned vertices in that same order. It avoids reevaluating a
triangle/tetrahedron for every vertex star. Temporary owner sums, volumes and
validity flags delay writes until complete mixed/degenerate/nonfinite-star
fallback has been checked. Ordinary GG still provides that fallback. There is
no persistent inverse/cofactor cache or additional MPI exchange. The traversal
is serial; deterministic parallel scatter and sharing geometry across recovery
passes are future profiling work.

For native doubles, maximum explicit temporary payload for the two-sensor M6
case is 12075696 bytes (11.52 MiB): owned sums, volumes, validity bits and sorted
local element indices. RAE requires about 0.99 MiB. These are buffer payloads,
not measured process RSS; custom expressions with more input columns need more
scratch memory. The buffers are released after each projection call.

Six alternating serial M6 pairs over two campaigns give median Hessian recovery
0.57025 -> 0.39355 s (31% lower). Individual campaign reductions are 13% and 32%;
contended sample ranges overlap. Combined whole-process medians are
11.620/10.195 s, but individual campaigns disagree about the sign of that
change. This does not establish an end-to-end adaptation speedup. RAE serial
recovery is 0.01829 -> 0.01715 s in a single pair. Geometry evaluation is reduced
by construction; native cost is measured separately.

All owned M6 Mach/pressure Hessians remain bitwise identical across 1/2/4 MPI
ranks, with metric relative differences below 8e-15. RAE sensor metrics agree
within 4e-12. The mathematical projection is unchanged, but compiler reassociation
changes low bits versus the previous implementation: M6 maximum pressure-Hessian
relative difference is 6.8e-13, while ill-conditioned tensor composition amplifies
some metric differences to 4.1e-6. RAE maximum metric change is 1.3e-10. These are
old/new differences, not exact-error estimates. Bounds, SPD, complexity, geometric
BL policy and frozen states pass their existing gates.

## Supported QR wall extension

The existing two-ring thin-solid-wall QR fit can now add selected degree-four
monomials after its partial cubic basis. The added terms contain at most one
thin-coordinate factor; the method is not a full fourth-order recovery and does
not resolve a pure thin-direction cubic from two normal layers. Whitening,
anchored sensor differences, direct physical derivatives and noise covariance
handling remain the existing QR approach. `NUM_METHOD_HESS= QUADRATIC_LEAST_SQUARES`
selects it; default GG is not changed into a boundary QR hybrid.

The fit requires at least two residual degrees of freedom, full numerical rank
at the existing 1e-10 pivot threshold, and the existing Frobenius condition bound
of 1000. Unsupported quartics fall back to the supported cubic model, then
quadratic, with WLS retained on failed fits. Both tangential and mixed selected
quartics are tested together; the rejected smaller tangential-only model is not
used. No third ring or additional owner-neighborhood exchange is introduced.

On the matched curved/skewed 3D layer meshes, maximum wall-interior relative
normal-curvature errors are:

| Resolution | Previous QR | Supported quartic QR | Reduction |
|---|---:|---:|---:|
| n=8 | 14.0426% | 9.1801% | 34.6% |
| n=16 | 5.6725% | 3.2982% | 41.9% |
| n=32 | 2.0761% | 1.0036% | 51.7% |

At n=32, the maximum wall-interior Hessian component error decreases from
0.08304 to 0.05877 (29%); tangent/corner errors still limit the result. Central
interior errors are unchanged. A flat, rotated, 1000:1 stretched 3D pressure field
with prescribed tangential and mixed quartics has central-wall Hessian error
below 2e-10. This regression has an exact derivative reference.

The tested 2D wall stencils do not have enough samples for the coupled quartic
basis. All 12 matched 2D QR exports remain byte-identical. A tangential-only
quartic submodel could reproduce an exact polynomial but caused the curved 2D
normal error to plateau (coarse 0.00105948, fine 0.00106258), failing two existing
convergence checks. That variant is rejected, with patch/log/receipt preserved;
those gates were not relaxed. The initial all-quartic probe also documents why
an exact 2D quartic promise is unsupported by the present sample budget.

M6 paired serial QR medians are 2.65229 -> 2.69866 s (1.75% more recovery time).
One new sample is 3.088 s, illustrating contention. The frozen M6 cases do not
exercise a new accepted quartic correction: their fields match the previous
checkpoint. There is therefore no measured M6 CFD accuracy improvement. RAE/M6
noise off/on at 1/2/4 ranks pass the original SPD, bounds, complexity, retained
state, MPI, symmetry and geometric BL checks. M6 second-ring request/export
counts exactly match the prior implementation.

## Publication basis and limits

[Alauzet and Frazza (2021)](https://doi.org/10.1016/j.jcp.2021.110340), Section 7.3,
provides the two-pass volume-weighted P1 projection; the cell traversal is our
implementation of that operator. [Vallet et al. (2007)](https://doi.org/10.1002/nme.2036)
supports direct polynomial fitting as a recovery family.
[Diskin and Thomas (2008)](https://ntrs.nasa.gov/api/citations/20090007493/downloads/20090007493.pdf)
discusses higher-order directional terms for stretched/curved gradients, while
excluding boundary effects. These papers do not prescribe or certify our chosen
quartic subset, thresholds or boundary convergence. It is an independently tested
extension. The rejected pure-quartic experiment and topology-dependent biases
in `CLEMENT_COMPARISON.md` are why fit residual/rank alone are insufficient.

Final checks: 151 serial cases/400078 assertions, 35 MPI-safe cases per rank,
36 manufactured datasets, frozen campaigns and forward/reverse AD syntax.
Direct QR remains primal-only. AD syntax does not certify differentiated runtime,
and local MPI checks do not certify large-scale hybrid scalability. Zero-iteration
frozen checks do not establish aerodynamic accuracy, nonlinear robustness or
adaptation-cycle quality. GG's remaining graded-wall bias is not removed by
improving the separate QR method.

## Cycle testing and further work

Start cycle tests on an isolated integration candidate containing these metric
changes, based on the same accepted remesher revision as the baseline. Our shared
base is 2abbd11769; the other agent's worktree is ahead on
codex/native-unsteady-performance at 99aa0d7f72 with pending audit/handoff edits.
Do not attribute direct comparisons of those diverged heads solely to recovery.
Pin the baseline and candidate, match initial restart/mesh, complexity, BL,
gradation, solver settings and resources, then compare mesh validity/quality,
wall spacing, interpolation error, residual/force convergence and cost.
After merge, repeat accepted cycles on the final main executable.

Next numerical work: wall reconstruction with enough independent samples or
verified wall coordinates, multiple topologies and prism/hex interfaces; profile
shared geometry and fallback preparation without weakening derivative/MPI gates.
Convective/viscous transfer remains a separately validated follow-up.
