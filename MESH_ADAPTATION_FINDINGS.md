# Mesh adaptation findings and follow-up work

Working branch: `codex/metric-robustness`, fork https://github.com/rois1995/SU2.
This file retains the investigation, implementation status, and deferred work.
Baseline validated implementation: `e17a96d301`; detailed measurements and replay
script are in `integration_evidence/metric_robustness_v2_validation.json` and
`integration_evidence/check_rae2822_metric_constraints.py`.

## Current authorized work (2026-10-07)

| ID | Work | Status |
|---|---|---|
| 1 | Constrained tensor gradation: classify blocked directions, converge useful updates, reduce repeated work | Prototype: direct constrained 2D updates, PSD transport screening, reusable buffers; validation pending |
| 2 | Noise-aware Hessian treatment, with controls that retain resolved curvature | Prototype: opt-in QR spectral shrinkage using residual sensitivity; validation pending |
| 4 | More selective MPI geometry exchange and storage | Prototype: up to 64 occupied spatial boxes per rank; validation pending |
| 5 | Reuse WLS geometry across sensors and derivative passes | Prototype: reuse WLS normal matrices and share weights across sensors within one adaptation call; validation pending |

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
