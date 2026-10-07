# Mesh adaptation findings and follow-up work

Working branch: `codex/metric-robustness`, fork https://github.com/rois1995/SU2.
This file retains the investigation, implementation status, and deferred work.
Baseline validated implementation: `e17a96d301`; detailed measurements and replay
script are in `integration_evidence/metric_robustness_v2_validation.json` and
`integration_evidence/check_rae2822_metric_constraints.py`.

## Current authorized work (2026-10-07)

| ID | Work | Status |
|---|---|---|
| 1 | Constrained tensor gradation: classify blocked directions, converge useful updates, reduce repeated work | Experimental BL policy: direct updates, sparse stencil work and timings; unit/MPI tests passed, main representation reconciliation required |
| 2 | Noise-aware Hessian treatment, with controls that retain resolved curvature | Implemented opt-in residual shrinkage with correlated-center uncertainty; exact/noisy/steep-profile and MPI tests passed |
| 4 | More selective MPI geometry exchange and storage | Implemented conservative occupied query boxes; unit/MPI tests passed; private candidate reference transport remains separate |
| 5 | Reuse WLS geometry across sensors and derivative passes | Implemented within-call normal-matrix/weight reuse; stretched multisensor equivalence and MPI tests passed |

Performance is an acceptance condition. Check finite SPD tensors, complexity,
wall-normal spacing, MPI agreement, residual gradation and elapsed time together.
Do not obtain a speedup by silently dropping required constraints or declaring a
sweep limit to be convergence. Keep material limitations visible.

## Implemented and validated before this work

- Coordinate-equilibrated WLS solves and stencil conditioning diagnostics.
- Opt-in primal direct quadratic recovery using coordinate whitening and pivoted
  QR; up to two rings, complete donor neighborhoods, WLS fallback for failed fits.
- Residual/pivot diagnostics and stencil comparison. These are confidence
  diagnostics, not an established noise filter.
- Sensor-unit normalization, affine roundoff suppression, and protection against
  flat sensors imposing a competing metric.
- Prescribed full-band BL normal sizes and zero normal-tangent coupling, nearest
  active-wall overlap handling, and a single outer fade per trial.
- Tangential chord sizes from the immutable native reference, using the native
  deviation check and feature limits, permitting refined-wall coarsening.
- Complexity solve including BL, size/aspect bounds and tensor gradation;
  infeasible budgets are reported while retaining configured wall heights.
- On-demand MPI second-ring owner neighborhoods and padded-rank-box BL geometry
  exchange with incident-face support for vertex normals.
- Shared eigensolver primal normalization fix for subnormal Householder scales
  under fast-math. The extreme AD derivative issue below remains open.

Validation: 94 selected serial cases; 21 selected cases per rank on two/four
ranks; six frozen RAE2822 WLS/QR runs. Both methods reached complexity 60000,
retained full-band normal sizes to about 7e-12 relative error, and agreed across
MPI within 3.4e-10 relative metric difference. QR Hessians were identical across
those partitions. This establishes metric properties, not aerodynamic accuracy.

## Performance baseline and actual limits

RAE2822 fixture: 18591 points, 36263 triangles, 852 AIRFOIL edges; frozen RANS/SA
restart, zero flow iterations, one OpenMP thread per MPI rank.

| Ranks | WLS metric computation (s) | WLS entire process (s) | QR entire process (s) |
|---|---:|---:|---:|
| 1 | 97.1332 | 98.2302 | 101.9654 |
| 2 | 56.9936 | 57.9460 | 58.0307 |
| 4 | 31.9571 | 33.0874 | 34.2505 |

These are observed wall times, not timings of gradation alone. The previous
report printed 80 sweeps for the last trial; gradation is also repeated within
the complexity solve and corner-size iterations. Instrumentation measured **61 complexity trials, 4576 cumulative sweeps**,
128.644 s in gradation, 2.1212 s in halo staging/exchange and 0.019443 s in
sweep reductions on one rank (fresh instrumented run; total elapsed times vary
with system load). The cost is predominantly local tensor work, not MPI.

The case reaches the 80-sweep limit. Final transported-tensor violations are
about 1.8--1.9% in the prescribed full BL band; outer-band ratios are below
1+1e-7. This is not a proof of geometric infeasibility. Two serial reference
chord/minimum-size conflicts remain (MPI reports can repeat shared faces).

QR reported 33886 of 37182 point-sensor fits above residual 0.05, with no WLS
fallbacks. Large residuals can represent input noise, nonsmooth flow, truncation
or unresolved variation; they do not justify erasing all curvature.

All 852 wall faces were still retained by rank-box exchange on this fixture.
A separated-wall MPI fixture retained at most 9 of 18. The original native
reference remains replicated. WLS weights and geometry matrices are rebuilt
per kernel call; `Rmatrix` is workspace, not a persistent cache.

## Retained deferred work

| Work | What is still needed |
|---|---|
| Full adaptation/flow validation (previous item 3) | New accepted adaptation cycles, subsequent flow convergence, mesh quality, conservation, forces and wall-resolution checks |
| Broader recovery/physics support (previous item 6) | QR periodic/symmetry support and differentiated executables; more than two rings only if failed-fit data warrants it |
| Euler metrics | Assess shock/contact-aware sensor combinations, normalization/weights and directional protection; existing generic/custom/goal machinery is the starting point |
| RANS metrics | Assess turbulence-model sensors, near-wall sensor reliability, shock/BL interaction and geometry-aware normal/tangential treatment |
| Automatic BL sizing | Friction-velocity/y+ based first-height selection with verified physical units; current prescribed apex height does not guarantee y+, cell-centre spacing or multilayer growth |
| AD numerical limits | Full differentiated solver validation. A forward scalar eigensolver derivative at off-diagonal 1e-310 is NaN both before and after the primal fix |
| Reference geometry scaling | Distributed/indexed immutable-reference access; keep restart associations and original-geometry provenance intact |
| Remesher robustness | Broader cavity repair, coordinated topology/geometry changes, geometry-constrained relocation and rollback-safe publication |
| Remesher performance | Avoid repeated rejected proposals, update local work queues, profile cavity size/rejection causes and MPI dependency traffic |
| Mesh topology | Native capability remains 2D simplices; structured multilayer/prism/quad guarantees and 3D native adaptation are separate work |

## Prior research and transferable ideas

### AMG/pyAMG-related publications

- Loseille and colleagues, [unique cavity operator and hierarchical partitioning](https://doi.org/10.1016/j.cad.2016.09.008), 2017:
  simultaneous boundary/volume changes and automatic cavity enlargement can
  combine otherwise rejected insertion/collapse/swap/relocation sequences.
  Metric-distance insertion filtering reduces unnecessary subsequent collapses.
  Investigate bounded cavity correction with retained orientation, reference
  geometry and transactional acceptance checks; replacing isolated moves with
  a coupled local proposal is a candidate, not an implemented guarantee.
- Alauzet and Frazza, [feature-based and goal-oriented RANS adaptation](https://doi.org/10.1016/j.jcp.2021.110340), 2021:
  unstructured anisotropic simplices can represent BL physics; accurate sensor
  recovery, feature/goal error control and primal/adjoint convergence all matter.
  A visually ordered BL grid alone does not establish flow accuracy. Examine
  sensor normalization, boundary recovery and output-functional verification.
- Bellosta, Abergo and Nishikawa, [accuracy on mixed/skewed grids](https://doi.org/10.2514/6.2025-0072), 2025:
  flow-discretization accuracy must be assessed separately from mesh adaptation;
  a better geometric mesh does not itself establish the solver's convergence order.

Evidence is from the accessible publications, not a source-code audit of a
proprietary AMG/pyAMG implementation. Earlier claims about internal operations
must be read with that distinction.

### Inspected open-source alternatives to MMG and Gmsh

Local study copies were in `/tmp/remesher-operator-study-20261007`; pinned upstream
links below preserve provenance if the temporary checkouts disappear.

| Code / inspected commit | Relevant operations to assess | SU2 application |
|---|---|---|
| [NASA refine](https://github.com/nasa/refine/tree/1563f71f224d59c8712e2181369ff93440957b0f) | Metric-space gradation (`src/ref_metric.c`), reconstruction (`src/ref_recon.c`), geometry/quality-controlled local operations | Gradation transport is already independently implemented; further reconstruction and operator policies need assessment |
| [Omega_h](https://github.com/sandialabs/omega_h/tree/dc235450cf15b2b9549fabe8472269e7f8618cae) | Metric gradation and smoothing (`src/Omega_h_metric.cpp`), synchronized owner/ghost fields and adaptation scheduling | Preserve SPD tensors and distributed consistency; measure iteration/work costs; ordinary unconstrained smoothing does not enforce SU2's fixed BL normals |
| [SCOREC Omega_h](https://github.com/SCOREC/omega_h/tree/8fc0a0bb3140e601cea86ebcbd2faecd30b410bf) | Parallel mesh/metric adaptation implementation | Cross-check ownership/communication patterns and current differences from the inspected Sandia version |
| [PUMI/MA](https://github.com/SCOREC/core/tree/7166244ff601fb3b3847543d725762a2e330feef) | Layer refine/coarsen/snap (`ma/maLayer*.cc`), shape handling and balancing | Assess layer/column consistency, geometry projection and coordinated distributed operations; native 2D triangles have a different representation |
| [AVRO](https://gitlab.com/philipclaude/avro/-/tree/2e63b6ef0340dca761e0923d7a4ba21747be7580) | Cavity, swaps, collapses, smoothing and geometry checks (`src/lib/adaptation`) | Evaluate metric-space relocation and coupled repair against native acceptance contracts |
| [Tucanos](https://github.com/tucanos/tucanos/tree/62cad873bce7707a922628398f5729750a53726d) | Anisotropic metric/remeshing and topology implementation | Assess small metric/operator kernels and reproducible quality checks before attempting integration |

The useful borrowing unit is an algorithm or acceptance policy with a measured
SU2 regression, rather than adopting a complete external backend. Keep metric
quality, reference fidelity, prescribed wall resolution, conservation and MPI
publication checks when assessing any candidate.

## Performance-oriented implementation decisions

- Reuse existing math, mesh ownership and communication infrastructure.
- Share WLS geometry within a call; avoid long-lived caches until invalidation
  for motion/adaptation, periodicity and AD is explicit and measured.
- Profile total complexity trials and cumulative sweeps, not just the last trial.
- Prefer direct constrained updates and selective MPI requests over increasing
  an iteration ceiling or repeatedly exchanging the entire boundary.
- Noise controls must be explicit, tested on exact/noisy/sharp fields and timed;
  distinguish coefficient uncertainty from actual resolved flow features.

## Current prototype contracts

- Direct full-band 2D gradation uses the Schur complement in the wall frame,
  retains the normal size and zero coupling, caps tangential updates, and counts
  currently blocked transports. Counts are not a proof of infeasibility.
- The 80-sweep safety ceiling is retained pending convergence/performance data.
  Final transported-tensor ratios are audited using the actual final halo field,
  rather than the inputs to the last sweep. Convergence of updates and remaining
  constraint violations are reported separately.
- `ADAP_HESSIAN_NOISE=0` retains the previous QR output. Positive strengths
  subtract a residual-derived Frobenius uncertainty from curvature eigenvalues
  in the whitened coordinate frame, retaining gradients. This is regularization,
  not a calibrated noise probability; fit residuals also contain truncation and
  shocks. Exact, noisy and resolved-field tests are required before use.
- WLS reuse is explicit and limited to matching geometry/weights in the same
  call, with no persistent mesh cache. Periodic, symmetry and AD use the prior
  kernels. Neighbour geometry is shared across sensors without storing weights.
- Spatial boxes cover all owned points and retain conservative incident-face
  padding. Metadata is bounded per rank; the immutable original reference remains
  replicated and very large MPI process counts still need distributed indexing.

## BL policy divergence raised by the user (2026-10-07)

This branch's native BL policy is a deliberate change from the original/main
policy, not simply a numerical fix. Original/main intersects each active wall
with the sensor metric, retains the resulting normal entry (so sensors can
refine below the BL normal size), and applies a tangential size floor in a small
near-wall band. It applies BL after the sensor complexity solve; total complexity
can exceed the sensor target.

The branch instead prescribes the closest wall's normal entry throughout the full
BL band, removes coupling there, allows tangential refinement for adapted surfaces,
and includes BL/gradation in the complexity solve. Original-reference chord sizing
also changes the dependence on current wall edge lengths. These choices address
spacing drift and refinement feedback, but hard normal spacing can suppress
legitimate sensor-driven normal refinement. A normal-size upper bound is not the
same as an equality. Do not describe this as a compatibility-preserving fix or
claim that spacing/complexity checks establish superior CFD accuracy.

The 128.644 s / 4576-sweep performance baseline is the earlier implementation of
this branch's constrained policy, **not the original/main implementation**. A
faster version of that policy is not yet a timing comparison against main.

The inspected NativeIntegrated checkout is `codex/native-unsteady-2d` at
`b0cbfaf33409f8dac455d19ed85a74b9c1fa9f43`; it also adds `ApplyPoint` for
pointwise/window use. The metric branch started from `73708f6722` and predates
that API work. Text/API integration and the BL sizing policy are distinct issues.
The user has been asked which conflict they mean; no main merge is authorized.

### Confirmed current-main representation contract

User supplied the main agent's reconciliation notes. Inspection confirms commit
`73b4bf9434266bbc42c62d6239dec4fe127007a0` freezes only the CFD sensor field.
`CSinglezoneDriver::MakeRemesher` constructs BL constraints from immutable original
wall components and passes a `MetricComposition` callback to `CNativeRemesher`.
Every candidate/remote-cavity query interpolates the donor sensor and invokes
`CBoundaryLayerMetric::ApplyPoint` at that actual position. Native nodal BL
composition is excluded in the solver. The callback uses intersection, retains
finer sensor demands and disables the legacy abrupt tangential floor.

Our composite nodal output must **not** feed that callback. In general
`interpolate(B(x_i,S_i)) != B(x,interpolate(S_i))`; interpolating very large normal
eigenvalues from a coarse Euler wall spreads tiny sizes into the interior.
Prescribed normals do not fix that representation error. Frozen adapted-grid
SPD/height/complexity/MPI checks also do not validate this interpolation contract.

Main's detailed preparation is physically saved at
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/METRIC_ROBUSTNESS_MERGE_PREPARATION.md`
and committed in `b0cbfaf334`. Keep it as the integration reference. Main has
validated cross-grid adaptation work newer than this branch's base; do not replace
that behavior merely to make this branch's nodal checks pass.

Reconciliation requirements retained for follow-up:
1. Freeze/interpolate sensor-only tensors; apply BL once at every actual query
   against original wall geometry, including remote/private candidate points.
2. Use one composition function for complexity estimation and remeshing, including
   normal/tangential/fade policy. Retain finer sensor demands by default; a hard
   normal prescription is a separate explicitly selected policy.
3. Integrate geometric BL density with adequate boundary-aware quadrature on
   coarse seeds. Nodal volume weights alone can misestimate thin-layer complexity.
4. Define how gradation propagates sensor and geometric constraints without
   baking the wall tensors back into the donor interpolation field.
5. Require coarse Euler-to-BL and BL-to-Euler regressions, sensor-finer-normal
   coverage and performance comparison with current main, as well as MPI checks.

Hessian recovery, conditioning, QR noise treatment and scoped MPI optimizations
can be integrated separately. The current BL/complexity/gradation experiments
remain on the research branch and are **not ready for a blind production merge**.

## Current validation checkpoint

The refinement after `00e9097db5` passed 97 selected serial tests (429643
assertions), and 24 selected cases per rank on MPI 2/4. This includes within-call
WLS reuse, exact/invalid/affine/conditioned/periodic/symmetry/Euler-wall Hessians,
QR noise regularization, native tensor transport, geometry transport and existing
native/reference tests. Forward/reverse AD syntax probes also instantiate the
shared inverse and legacy recovery path; these are not full AD runtime validation.

The initial no-pruning RAE experiment reached nodal complexity 60000, a final
transported ratio printed as 1, 52 last-trial sweeps, 88 complexity trials and
4569 cumulative sweeps, with 30.745 s of gradation on one rank. This is an
experimental-policy comparison with the prior branch, not current main. Exact
unchanged-stencil pruning is now included; final timings and independent audits
will be recorded separately. WLS's three-sensor stretched-grid microbenchmark
measured 0.0144882 s for 20 legacy Hessian passes versus 0.00656786 s with reuse.
Noise-energy reduction on the manufactured checkerboard is about 44%; exact
quadratic and resolved steep-profile checks remain intact. No fixed fraction of
noise removal is a universal promise of a one-uncertainty threshold.
