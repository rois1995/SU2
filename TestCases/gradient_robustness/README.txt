First CFD stencil-robustness experiment (2026-10-08)

Branch: codex/cfd-mesh-robustness
Starting commit: e49c545d700632bd9c68fc52535ef63d6ca8632d on codex/native-unsteady-performance.
Worktree: /tmp/su2-cfd-mesh-robustness. Private build: /tmp/su2-cfd-mesh-build.

The shared LS/WLS gradient kernel retries highly correlated nonperiodic stencils
with streaming Givens QR. It preserves the original linear fit, distance weights,
variable ranges, boundary corrections, normal-matrix storage, AD input/output
registration, and halo communication. It reaches primitive-variable gradients
for viscosity and MUSCL, solution/turbulence gradients, and ordinary adaptation
calls to this kernel. Green-Gauss gradients are unaffected.

The existing coordinate-equilibrated Cholesky factorization selects the retry
when its normalized Gram determinant is at most 1e-6. This internal cutoff is a
conservative roundoff guard, not a general mesh-quality measure. QR uses scaled
coordinate rows, active scalar arithmetic, and an unresolved-pivot guard of
64 machine epsilons. Zero-length WLS rows are skipped. Unresolved stencils return
zero gradients, following the existing singular-stencil convention.

The implementation is independent of feature_2ndOrderMixedGrids. No Bellosta
flux, centroid, viscous-damping, or LS cache patch was copied. In particular,
scalar/SIMD projection guards and correction behavior after a positivity
fallback require independent checks before bringing that work across.

Numerical evidence is saved in results_20261008.json. The 36 affine-field tests
cover perturbed triangles and tetrahedra, rotation, coordinate scales 1e-7 to
1e7, LS and WLS, and aspect ratios 1, 1000, and 1e6. Their affine constants scale
with mesh size; these are solve-precision tests, not tests of cancellation under
arbitrarily large field offsets. The baseline fails 12 cases at the 1e-7 bound.
The candidate recovers gradients to approximately 2.5e-9 in the worst case.

An independent column-pivoted Householder QR comparison checks a nonlinear fit
at aspect ratio 1e4. The tests also check constant fields, variable slices,
collapsed stencils, two OpenMP threads, and two MPI ranks. A preliminary nonlinear
comparison at aspect ratio 1e6 differed by about 9.3e-6 between the two QR solves:
an inconsistent, severely anisotropic fit remains sensitive to perturbations of
the coordinate rows. The affine precision bound does not apply to that problem.

The standalone CoDiPack checks exercise forward and reverse derivatives of the
actual QR helper, including a nonlinear fit with a nonzero geometry derivative.
They do not validate an entire discrete-adjoint CFD solve or preaccumulation
through all solver callers.

The small CFD comparison uses the existing compressible NS MMS source and exact
Dirichlet data on alternating, perturbed triangles. It tests rotated regular
squares, straight thin layers, and annular thin layers at 12 and 24 cells per
direction. Thin-layer aspect parameter is 1e4; angular columns remain aligned
on the annulus to avoid folded cells. Viscous gradients use WLS, MUSCL uses LS,
and the limiter is disabled. Both binaries use the same compiler flags, common
objects, and unchanged flow objects; the six gradient-dependent CFD objects in
the baseline were rebuilt from the starting commit and placed in an independent
archive. This avoids comparing against the moving metric branch or another build.

These are short integration comparisons, capped at 300 iterations, not an
asymptotic-order study. Most cases do not reach the requested density-residual
threshold. Their solution errors are essentially unchanged between baseline and
candidate. The QR change repairs demonstrated roundoff; it does not establish
better nonlinear CFD accuracy, viscous stability, or wall shear/heat accuracy.

Periodic meshes keep their current communicated normal-equation solve. Batched
Hessian recovery that reuses inverse normal matrices also keeps its current
algorithm. Extending QR to periodic interfaces needs a row or factor exchange.
No full RANS, moving-grid, unsteady, remeshing, or end-to-end adjoint campaign was
run. Compilation used one job with nice=10; CFD used one thread and nice=10.
Only the ownership check used two MPI ranks. Timings are local gradient-kernel
measurements under shared-machine conditions, not whole-solver speed claims.
After reusing the existing factorization for the retry decision, measured normal
paths are within about 1.5% of baseline; the QR-triggering one-variable stencil
costs about 2.4 times baseline. The raw samples are retained in the JSON evidence.

Useful literature already checked:
- Diskin and Thomas, NIA 2008-12: high-aspect-ratio gradient and discretization
  behavior (Papers/20090007493.pdf).
- Diskin and Thomas, AIAA 2012-0609: irregular-grid truncation errors and curvature
  (Papers/20120001451.pdf). Stable algebra alone does not remove these errors.
- Syrakos et al., arXiv:2111.02182, section 8.2: QR/SVD avoids forming the squared
  condition number of the normal equations. https://arxiv.org/abs/2111.02182
- Nishikawa, JCP 468 (2022), 111481: flux integration on composite dual faces
  (Papers/1-s2.0-S0021999122005435-main.pdf).
- Nishikawa, AIAA 2020-1786: face-area-weighted element centroids (the added 2020 PDF).

The next accuracy experiment needs converged manufactured solutions on mixed
triangular/quadrilateral grids and separate interior/boundary linear-flux
quadrature checks. A flux correction must be tested together with boundary
integration. Modified centroids need consistent normals, volumes, and moving-grid
geometric conservation. These changes are not validated by the present QR tests.

Reproduce from the configured worktree (use paths for your private build):
  OMP_NUM_THREADS=1 /tmp/su2-cfd-mesh-build/UnitTests/test_driver '[Gradients],[HessianReliability]'
  OMP_NUM_THREADS=2 /tmp/su2-cfd-mesh-build/UnitTests/test_driver '[AnisotropicLS]'
  OMP_NUM_THREADS=1 mpirun -np 2 /tmp/su2-cfd-mesh-build/UnitTests/test_driver '[AnisotropicLS]'
  OMP_NUM_THREADS=1 /tmp/su2-cfd-mesh-build/UnitTests/test_driver '[AnisotropicTiming]'
  python3 TestCases/gradient_robustness/run_mms.py BASELINE_SU2_CFD CANDIDATE_SU2_CFD NEW_OUTPUT_DIRECTORY

Standalone AD check, from the source root:
  c++ -std=c++17 -O2 -ffast-math -fno-finite-math-only -ffunction-sections -fdata-sections -Wl,--gc-sections -Iexternals/codi/include -Iexternals/eigen -Iexternals/CLI11 -DCODI_FORWARD_TYPE UnitTests/SU2_CFD/anisotropic_ls_ad.cpp -o /tmp/anisotropic_ls_forward
  /tmp/anisotropic_ls_forward
For reverse mode, replace CODI_FORWARD_TYPE with CODI_REVERSE_TYPE and add
-DCODI_JACOBIAN_LINEAR_TAPE. The section flags discard unused solver-header
functions, keeping this a standalone check of the actual helper.
