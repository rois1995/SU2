# Mesh adaptation findings and follow-up work

Working branch: `codex/metric-robustness`, fork https://github.com/rois1995/SU2.
Rebased onto `codex/native-metric-integration` at `2abbd11769` on 2026-10-07.
The other agent owns BL composition, geometric complexity and gradation; this
branch continues Hessian/sensor-metric work and validates the shared BL policy.
See `METRIC_ROBUSTNESS_INTEGRATION.md` for the current implementation contract.
Older BL experiments and their receipts below are historical evidence.
This file retains the investigation, implementation status, and deferred work.
Support statements in older entries describe their dated checkpoints.
Historical baseline implementation: `e17a96d301`; detailed measurements and replay
script are in `integration_evidence/metric_robustness_v2_validation.json` and
`integration_evidence/check_rae2822_metric_constraints.py`.

## Literature assessment and QR symmetry support (2026-10-08)

Source commit `747f0ef6ea` allows direct QR sensor recovery with symmetry markers.
After fitting, the existing scalar-gradient and vector-gradient symmetry rules
remove the normal gradient and mixed normal-tangential Hessian components while
retaining normal-normal curvature. The implementation reuses boundary normals and
the existing Hessian-gradient workspace; no new stencil ring, MPI exchange or
full-mesh Hessian buffer is added. Euler and viscous wall behavior and the shared
geometric BL composition remain unchanged. Periodic, goal and differentiated QR
configurations remain rejected; custom sensors must obey the existing mirror-even
contract. General curved/nonorthogonal symmetry cases are not independently
certified by this campaign.

The supplied PDFs identify an important alternative: Alauzet and Frazza 2021,
Section 7.3, use a two-pass volume-weighted Clément recovery, not our QR wall
correction. Vallet et al. 2007 discuss direct quadratic fitting and expanded
boundary patches. Diskin and Thomas 2008 investigate directional enrichment and
distance-based mapping for interior gradients. Galbraith et al. 2020 expose the
cost of weak boundary recovery and compare boundary extrapolation. These are
concrete benchmark candidates; the papers do not establish a universal winner or
validate our exact partial cubic basis and thresholds. Details and section
pointers: [Papers/IMPLEMENTATION_ASSESSMENT.md](Papers/IMPLEMENTATION_ASSESSMENT.md).

New tests cover oblique 2D/3D planes, two orthogonal planes, Euler-wall junctions,
signed normal curvature and a nonquadratic mirror-even sensor, with noise zero
and one. All 148 selected serial cases passed (394550 assertions); all 32 selected
MPI-safe cases per rank passed at two/four ranks. Maximum transformed Hessian
errors for the manufactured quadratic at symmetry points, noise zero:

| Manufactured geometry | WLS | QR |
|---|---:|---:|
| Rotated stretched 2D, one symmetry | 1.5 | 1.31e-10 |
| Rotated stretched 3D, one symmetry | 1.65667 | 8.93e-10 |
| Rotated stretched 3D, two symmetries | 4.36364 | 8.93e-10 |

These are absolute errors in local layer coordinates with known quadratic
derivatives, not aerodynamic error reductions. The new support makes the real
ONERA M6 RANS/SA fixture usable by QR. Six frozen wing runs (noise zero/one at
1/2/4 ranks, 96252 points and 545438 tetrahedra) passed unchanged-state, SPD,
size/aspect, independent complexity, symmetry-Hessian and strict MPI checks.
All 192504 point-sensor fits succeeded without WLS fallback; 169 used cubic terms.
Hessians are identical across partitions; maximum metric relative MPI differences
are 8.47e-15 (noise zero) and 2.02e-15 (noise one). At 8112 symmetry vertices,
relative mixed-curvature residuals are below 7.2e-16 while nonzero normal curvature
is retained. A serial RAE QR/noise-zero regression produces a byte-identical
`fields.csv` to the preceding wall-recovery implementation, including sensor,
Hessian, metric, solution and coordinates. Both AD header syntax probes pass;
this is not full differentiated solver validation or AD QR support.

Performance has a real tradeoff. Three alternating paired serial frozen M6 runs
give median Hessian time 0.176376s for WLS and 2.50669s for QR/noise zero (about
14.2 times); median whole-process times are 9.37608s and 11.66839s (about +24.4%).
This compares the complete optional recovery methods, including QR's existing
WLS fallback preparation, not the symmetry projection alone. Noise-one Hessian
times in the six-run campaign are 3.56326/2.11961/1.21448s at 1/2/4 ranks, versus
2.50511/1.52962/0.854762s without noise. One observation per rank under contention
does not establish general scaling. Repeated paired fields are byte-identical
within each method. All heavy jobs were sequential, nice 10, build jobs one,
maximum MPI ranks four and one library thread per rank.

Reliability limits remain: 105281 final wing fits have residual above 0.05 despite
full rank; the mean is 0.092339, maximum 0.998883 and minimum QR pivot ratio
4.15464e-7. Noise strength one filters every final wing fit. Neither result
establishes better physical Hessians or force predictions, so noise stays zero
by default. Accepted adaptation/flow validation remains open.

Receipt: `integration_evidence/qr_symmetry_v1_validation.json`. Raw fields,
commands, scripts, input/source/binary pins and paper inventory are saved under
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/qr_symmetry_v1`.
Next: benchmark Clément recovery and controlled boundary extrapolation at fixed
complexity, then assess a distance-based Hessian with the full mapping-curvature
chain rule. Profile avoidable WLS fallback preparation and validate accepted
adaptation/flow cycles before adding further recovery heuristics.

## Publication provenance and limits (checked 2026-10-08)

The prioritized PDF checklist is in [Papers/READING_LIST.md](Papers/READING_LIST.md),
with the four papers already requested, five immediate additions, and later
metric/gradation, validation and parallel adaptation references.

The implementation combines published method families, standard numerical
linear algebra and experimental engineering choices. The references checked
here explain the foundations and known failure modes; this audit does not mean
every recent change was derived from these papers before implementation.
Manufactured tests and frozen solution checks are empirical evidence, not a
published convergence proof or evidence of improved aerodynamic predictions.

| Topic | Relevant primary source | Relationship to this branch |
|---|---|---|
| Curved, highly stretched CFD meshes | Mavriplis, *Revisiting the Least-squares Procedure for Gradient Reconstruction on Unstructured Meshes*, NASA/CR-2003-212683 (2003), [public report](https://ntrs.nasa.gov/citations/20040070704) | Documents serious gradient errors from stretching combined with curvature and dependence on weighting/discretization. It motivates examining this failure mode; it does not establish our Hessian algorithm. |
| Quadratic recovery in node-centered finite volumes | Diskin and Thomas, *Effects of Mesh Irregularities on Accuracy of Finite-Volume Discretization Schemes*, AIAA 2012-0609, [public full paper](https://ntrs.nasa.gov/api/citations/20120001451/downloads/20120001451.pdf), particularly Sections II and VII | Studies quadratic least-squares fits, growing insufficient stencils to neighbors of neighbors, and curved/high-aspect meshes. These are directly relevant method choices. Their fits, flux discretizations and accuracy results differ from our weighted sensor Hessian recovery. |
| Polynomial recovery and Hessian theory | Zhang and Naga (2005), [gradient recovery](https://epubs.siam.org/doi/10.1137/S1064827503402837); Guo, Zhang and Zhao (2014 preprint), [Hessian Recovery for Finite Element Methods](https://arxiv.org/html/1406.3108v2), Sections 2.2 and 3 | Polynomial patch fitting is established. The Hessian paper applies gradient recovery twice; our QR path differentiates one scalar polynomial fit directly. Its finite-element assumptions and superconvergence results cannot be claimed for our finite-volume RANS implementation or partial cubic basis. |
| Metric intersection | Alauzet, Frey, George and Mohammadi (2007), [3D transient fixed point mesh adaptation](https://doi.org/10.1016/j.jcp.2006.08.012) | Supports intersection through simultaneous reduction of quadratic forms. The scaled A+B whitening frame in this branch is our numerically motivated algebraic evaluation of that operation; no source identified here specifically prescribes this implementation. |
| Anisotropic gradation and RANS adaptation | Alauzet, [Size gradation control of anisotropic meshes](https://doi.org/10.1016/j.finel.2009.06.028); Alauzet and Frazza (2021), [Feature-based and goal-oriented anisotropic mesh adaptation for RANS applications in aeronautics and aerospace](https://doi.org/10.1016/j.jcp.2021.110340) | Establish the wider metric/gradation and RANS adaptation setting. They do not validate this branch's exact geometric BL composition or new Hessian correction. Geometric BL/gradation implementation remains owned by the integration branch. |

Choices that must remain explicitly identified as implementation heuristics:

- Wall-only partial cubic enrichment, omission of the pure thin-direction cubic,
  the SVD separation threshold 10, condition-bound threshold 1000, and requirement
  of two residual degrees of freedom. These were selected through stencil
  support analysis and manufactured comparisons, not copied as a complete
  published algorithm. The coarse-grid regression and unsupported edges below
  remain material limits.
- Residual-based Hessian uncertainty and the exact spectral noise shrinkage
  policy. Noise remains optional and defaults to zero; preservation of physical
  shocks and wall features has not been established by the frozen checks.
- Global-ID accumulation order for adaptation WLS and the choice to reuse the
  existing compensated summation helper. These address measured floating-point
  partition dependence, not a new mathematical reconstruction method.
- QR/SVD conditioning and rank checks are standard numerical tools. The exact
  basis, scaling, fallback thresholds and MPI stencil packaging are engineering
  decisions that need their own tests.

Next literature assessment: compare direct and twice-recovered Hessians on
one-sided, curved, stretched node-centered finite-volume stencils; assess
boundary/symmetry extensions and anisotropy-uniform error bounds before choosing
further recovery changes. Public sources above are accessible. Full papers on
these specific boundary-layer recovery questions, especially any used for the
user's pyAMG workflow, would be useful additional evidence.

## Curved-wall QR recovery improvement (2026-10-08)

Commit `6312fee370` adds a supported cubic correction to the existing opt-in
quadratic sensor recovery. It applies at solid-wall points on grown stencils,
using the same complete two-ring samples and MPI exchange. Interior recovery
and the integrated geometric BL composition policy retain their existing paths.
Receipt: `integration_evidence/curved_wall_recovery_v1_validation.json`.

The smallest SVD direction must be separated from the other directions by a
factor greater than 10. The fit includes tangential and mixed cubic terms but
omits the pure thin-direction cubic term, which can be inseparable from lower
orders on three sampled layer levels. It requires two remaining residual degrees
of freedom, full rank, and a Frobenius bound on the 2-norm condition at most 1000.
Unsupported extensions use the existing quadratic fit; failed quadratic fits
still retain WLS. Noise covariance, roundoff suppression and fit diagnostics now
use the actual number of fit coefficients. The cubic-fit count uses the existing
statistics reduction, so no extra collective or stencil ring is introduced.

Tests now measure wall-interior and wall-edge transverse, tangential and mixed
derivatives separately. New physical accuracy/convergence assertions would fail
on the old measurements; finite tensors alone are insufficient. All 147 selected
serial cases passed (379112 assertions), and 31 MPI-safe cases per rank passed on
two/four ranks. The differentiated header syntax probes pass; QR is still rejected
in differentiated, symmetry and periodic configurations.

| Refined manufactured case | Before | After | Improvement |
|---|---:|---:|---:|
| Doubly curved/skewed 3D: worst wall transverse relative error | 12.8851% | 6.48616% | 49.7% lower |
| Same 3D case: wall-interior transverse relative error | 12.8851% | 5.67254% | 56.0% lower |
| Same 3D case: wall-interior tangential absolute error | 0.566651 | 0.225506 | 60.2% lower |
| Same 3D case: wall-interior mixed absolute error | 0.194001 | 0.0833072 | 57.1% lower |
| Curved 2D AR1000: wall transverse relative error | 0.34688% | 0.110208% | 68.2% lower |
| Curved 2D AR1000: wall-interior mixed absolute error | 0.0514686 | 0.00247931 | 95.2% lower |
| Curved 3D AR1000 without span curvature/skew: wall-interior transverse relative error | 0.551333% | 0.107329% | 80.5% lower |

These errors use analytic derivatives in local layer coordinates. The new method
does not improve every mesh resolution: on the coarse doubly curved/skewed 3D
case the worst transverse wall error rises from 12.1190% to 14.0426%, then falls
to 6.48616% on refinement. Unsupported edge/corner fits retain the earlier errors.
Interior errors retain the baseline values. WLS's curved/skewed accuracy weakness
is unchanged; this improvement applies when `NUM_METHOD_HESS=QUADRATIC_LEAST_SQUARES`.

Twelve final frozen RANS/SA checks passed independent unchanged-state, SPD,
size/aspect, sensor transport and MPI gates, plus reported complexity checks
(M6 complexity is also independently integrated). The RAE fixture uses
1584 corrected point/sensor fits on each partition. Maximum metric MPI differences
are 6.54e-12 (QR/noise off), 9.44e-11 (QR/noise on), and 1.09e-12 (WLS).
M6 retains its true symmetry and WLS path, with the numerical MPI gate passing.
The noise default stays zero. Geometric composed-field transport residuals remain
diagnostics owned by the BL integration work; this is not a certificate of their
gradation or of improved forces, flow convergence, wall y+ or native 3D remeshing.

Rejected candidates are preserved: stronger inverse-distance weighting improved
the difficult case but more than doubled a simpler case's wall-normal error;
unrestricted enrichment worsened 2D interior eigenvalue errors; full cubic fits
produced unreliable thin-direction/edge derivatives. Final enrichment is limited
to wall points and supported tangential/mixed terms. Independent Python screens
are exploratory; the final acceptance measurements come from the C++ solver chain.

Raw fields, tests, rejected candidates, scripts, paired performance measurements
and input/binary/source pins are saved outside `/tmp` under
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/curved_wall_recovery_v1`.
All heavy work runs sequentially with nice 10, build jobs 1, MPI ranks at most 4
and one library thread per rank. Three alternating paired serial RAE QR/noise-off
runs give median Hessian time 0.193526s before and 0.193830s after (about +0.16%).
Median metric/process times are 9.34549/10.3061s before and 9.18881/10.2028s after.
This small sample under contention establishes no measurable slowdown on this
fixture; it is not a general speedup claim. Different sensor fields also change
complexity/gradation work. Raw observations are in the validation receipt.

Next: add QR symmetry support so the real ONERA wing can exercise direct recovery;
then periodic support. Better WLS/edge reconstruction and accepted adaptation/flow
checks remain open. A complete cubic fit would require more independent normal
layer samples; assess its communication and noise cost before extending rings.

## Numerical stability implementation (2026-10-08)

Source commits `f27bae8dcf` and `3d57a82801` fix the previously failing strict
metric MPI gates without changing the geometric BL policy. Receipt:
`integration_evidence/metric_numerical_stability_v1_validation.json`.

- The shared tensor intersection now uses the scaled sum of both inputs as its
  whitening frame. It evaluates the same generalized spectral maximum while
  avoiding cancellation from whitening by a nearly rank-one rotated sensor.
  Independent 90-digit references cover 2D/3D, condition 1e14, input swapping,
  perturbations and scales 1e-150 to 1e150. Original double evaluation had relative
  errors 4.61e-4 / 8.80e-4 on the rotated cases; the balanced evaluation is near
  machine precision. This shared helper also serves BL composition and gradation;
  their mathematical policy, interpolation and geometry remain unchanged.
- Adaptation WLS gradient/Hessian accumulation visits neighbors by global ID,
  including the reused-geometry Hessian path. A temporary vector is reused per
  calling thread; no persistent weight/stencil cache is added. Ordinary CFD
  gradient calls retain their existing ordering. Reversing fixture adjacency
  preserves adaptation derivatives exactly.
- Primal sensor normalization reuses the existing compensated sum library,
  whose translation unit protects compensation from fast-math reassociation.
  Temporary storage is one passive scalar per owned point/sensor (about 1.47MiB
  on this serial M6 fixture and 0.28MiB on RAE). Differentiated builds retain
  their active MPI normalization sums; these improvements are not a full AD
  reproducibility or accuracy certificate.

All 147 selected serial cases passed (378972 assertions); 31 MPI-safe cases per
rank passed on two/four ranks. The five legacy `[Gradients]` MPI failures also
occur in the unchanged integration executable: those fixtures use null solver
communication and inspect incomplete halo data. A broader MPI in-memory mesh
reader fixture also fails in that executable. Failed logs and baseline runs are
retained; no production guard or assertion was weakened to obtain these passes.
Forward/reverse kernel/header syntax probes pass with a valid reverse tape;
full differentiated solver behavior remains unvalidated.

All twelve frozen RANS/SA runs completed, with zero flow/remeshing iterations.
The original solution values and mesh coordinates pass the independent checks:

| Fixture / recovery | Maximum metric difference, MPI 1 vs 2/4 | Complexity | Independent sensor transport |
|---|---:|---:|---|
| ONERA M6 WLS with true symmetry | 6.63e-15 (previously 7.37e-7) | 300000, independently integrated | Not enabled in this 3D case |
| RAE WLS, noise 0 | 1.09e-12 | Reported geometric composed integral 60000 | Maximum ratio 1.000000094 |
| RAE QR, noise 0 | 3.85e-12 | Reported geometric composed integral 60000 | Maximum ratio 1.000000098 |
| RAE QR, noise 1 | 1.34e-10 (intersection-only: 2.00e-4) | Reported geometric composed integral 60000 | Maximum ratio 1.000000099 |

Recovered Hessian CSV components are identical across these partitions. M6's
former worst weak eigenvalue amplified order-dependent WLS roundoff; changing
intersection alone did not fix it. On RAE with noise enabled, filtered Hessians
were already identical; compensated normalization was additionally necessary.
The strict MPI limit stays 1e-9. Noise strength remains zero by default because
numerical reproducibility does not establish physical feature preservation.

The independent RAE reference composes BL at the actual point and retains both
sensor and BL demands (minimum generalized domination about 1 minus 4.2e-12).
It still measures **composed** nodal transport maxima 6.40012 / 12.39031 / 4.63954
for WLS / QR / QR-noise. Converged sensor gradation is not a composed-field
certificate. No composed-field correction or continuous quadrature accuracy
claim is introduced here; these remain with the BL integration owner.

Performance was checked with one heavy job at a time, nice 10, build jobs 1,
MPI ranks at most 4 and library threads 1. A paired kernel benchmark measures
about 7% (2D) / 8% (3D) additional intersection cost; it still uses two eigensolves
and four matrix products. The combined WLS reuse fixture retains about a 2.37x
advantage over repeated geometry recovery. Observed serial RAE metric times are
10.91s / 9.81s / 11.69s (WLS / QR / QR-noise), M6 0.435s including 0.187s Hessian
recovery. These single observations under machine contention are not controlled
whole-solver performance comparisons. Final sensor gradation needs 32 / 29 / 25
sweeps; cumulative complexity-solve work is recorded separately in the receipts.

Durable raw fields, logs, configs, input copies, build/probe failures, executables,
benchmark and a SHA-256 manifest are saved outside `/tmp` under
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/metric_numerical_stability_v1`.
The numerical blockers below are historical. Next work is curved/skewed wall
recovery with separate wall-interior/edge accuracy gates, followed by QR symmetry
and periodic support. Full adaptation/flow validation and differentiated solver
validation remain necessary; native 3D remeshing is separate work.

## Authorized work and retained history (2026-10-07)

| ID | Work | Status |
|---|---|---|
| 1 | Constrained tensor gradation: classify blocked directions, converge useful updates, reduce repeated work | Direct updater rejected by actual-case checks. Integrated base retains sensor-only gradation and geometric query composition; full composed-field guarantees remain the BL owner's follow-up |
| 2 | Noise-aware Hessian treatment, with controls that retain resolved curvature | Implemented opt-in residual shrinkage; numerical MPI blocker fixed on 2026-10-08. Physical feature preservation remains unvalidated; retain default zero |
| 4 | More selective MPI geometry exchange and storage | Implemented conservative occupied query boxes; unit/MPI tests passed; private candidate reference transport remains separate |
| 5 | Reuse WLS geometry across sensors and derivative passes | Implemented within-call normal-matrix/weight reuse; stretched multisensor equivalence and MPI tests passed |

Performance is an acceptance condition. Check finite SPD tensors, complexity,
wall-normal spacing, MPI agreement, residual gradation and elapsed time together.
Do not obtain a speedup by silently dropping required constraints or declaring a
sweep limit to be convergence. Keep material limitations visible.

## Implemented and validated before this work

This section describes the historical pre-integration implementation. Its hard
normal BL reset was superseded by the geometric intersection contract above.

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

## Superseded prototype contracts (00e9097db5; see rejection below)

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
The user's main-agent notes confirm both semantic conflicts; no main merge is authorized.

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

## Unit validation checkpoint before the actual-case rejection

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
unchanged-stencil pruning was included at that checkpoint and subsequently removed;
final timings and independent audits are recorded below. WLS's three-sensor stretched-grid microbenchmark
measured 0.0144882 s for 20 legacy Hessian passes versus 0.00656786 s with reuse.
Noise-energy reduction on the manufactured checkerboard is about 44%; exact
quadratic and resolved steep-profile checks remain intact. No fixed fraction of
noise removal is a universal promise of a one-uncertainty threshold.

## Rejected direct Schur updater: actual-case gate

The `70bfe3144e` direct constrained updater was rejected after all nine frozen
runs. Serial/unit and manufactured MPI passes were insufficient. Unfiltered QR
reported complexity 60617.5 on MPI 1/2 and 59804.9 on MPI 4 for target 60000;
the maximum relative metric disagreement on four ranks was about 0.99975.
QR Hessians remained exactly identical across partitions. Near a zero
fixed-normal Schur gap, tangent forcing switches from a very large capped update
to a blocked-normal fallback. That creates discontinuous trial complexity and
can make different root paths select different tensors.

The updater is removed in the subsequent source. The earlier relaxation policy
is retained with independent performance optimizations while the shared geometric
composition architecture is reconciled. This rollback does not make hard-normal
nodal composition compatible with current main; that policy remains experimental.
The failed runs and their source/hash/configuration receipt are preserved in
`integration_evidence/metric_robustness_rejected_schur_v3.json` and locally under
`/tmp/su2-metric-build/rae-native-metric-v3-rejected-schur`.

Additional measured limits: bounded occupied boxes still retain all 852 wall
faces on this partitioned RAE fixture. They do not solve replicated-original
reference storage or arbitrary remote/private query coverage. Exact pruning cut
WLS owned-point visits from a possible 84.9 million to 29.9 million and preserved
the preliminary serial field bit-for-bit, but the two serial timings (30.745 vs
31.5502 s) do not establish a runtime win from pruning alone. Four-rank scaling
was limited by synchronization/load imbalance. Keep these as findings, not as
claims that every optimization improved elapsed time.

Unchanged-stencil pruning is also removed from the final refinement: the isolated
serial comparison established fewer visits and identical fields, but no runtime
benefit. Keep the receipt and source history for future workload-specific
assessment. The retained performance changes reuse buffers and geometric work,
screen satisfied 2D transports without eigen decomposition, and measure cumulative
work. Do not trade additional control/memory overhead for an unmeasured speedup.

## Final retained-source validation and unresolved limits

Source `887ca28f939302845e731051c410fbda22c2dc0a`, binary SHA-256
`f95474b5338a43ddfa1c5fb4b9d0dd0dc8b40f1307a00d4b1a097fc09e63f456`.
The complete receipt is `integration_evidence/metric_robustness_v3_validation.json`.
Selected serial regressions passed 97 cases / 429643 assertions; MPI two/four
ranks passed 24 cases per rank. Forward/reverse AD syntax probes passed.

The strict independent frozen-restart audit passed for the six default-noise
WLS/QR cases. Complexity independently integrated to 59999.9983 / 59999.9899,
with finite SPD tensors, unchanged five restart solution fields, full-band normal
size error below 6.7e-12 and coupling below 5.2e-10. Maximum relative metric
differences over MPI were below 3.4e-10; QR Hessians were identical.

| Method | Ranks | Process (s) | Gradation (s) | Hessian recovery (s) |
|---|---:|---:|---:|---:|
| WLS | 1 | 34.008 | 32.478 | 0.00620 |
| WLS | 2 | 19.093 | 18.041 | 0.00306 |
| WLS | 4 | 13.125 | 11.996 | 0.00343 |
| QR, noise disabled | 1 | 33.508 | 31.636 | 0.20290 |
| QR, noise disabled | 2 | 20.260 | 18.869 | 0.11119 |
| QR, noise disabled | 4 | 13.231 | 12.193 | 0.06274 |
| QR, noise strength 1 | 1 | 48.850 | 46.798 | 0.28388 |
| QR, noise strength 1 | 2 | 30.976 | 29.461 | 0.16208 |
| QR, noise strength 1 | 4 | 20.613 | 19.505 | 0.07893 |

Both default methods used 37 complexity trials / 2960 cumulative sweeps.
The WLS serial gradation phase fell from the instrumented earlier-branch
128.644 s to 32.478 s; fewer trials and cheaper tensor work both contributed.
This is a single-run comparison against our earlier experimental policy, **not
current main**. Mean WLS sweep time was about 11 ms, or 0.88 s per 80-sweep
trial, repeated throughout the complexity solve. MPI four-rank cumulative sweep
reductions took 4.83 s within 12.00 s of gradation, so synchronization/load balance
remains relevant after reducing local work.

The 80-sweep limit remains active. Independent directed-edge audits measured
maximum full-band ratios 1.01835 / 1.01891 and 1900 / 3236 edges above 1+1e-5;
outer ratios were below 1+1e-7. Faster relaxation is **not** a convergence fix.
Original reference chord conflicts and composite-field representation limits
remain. No adaptation cycle or aerodynamic improvement is demonstrated.

### Enabled-noise integration check failed

The nine-case strict audit fails when it reaches the enabled-noise MPI metric.
QR Hessians are identical across all three partitions, but relative final tensor
differences reach 7.44e-6 (two ranks) and 1.93e-5 (four ranks), above the required
1e-9. All three logs report complexity 60000, but the final transported-metric
ratio worsens to 2.28272, with 17 normal-limited directed edges and 80 sweeps.
The six-case default-only audit passed unchanged; no assertion was loosened.

Retain `ADAP_HESSIAN_NOISE=0` for integration. The opt-in filter's manufactured
curvature tests validate its implementation, not its physical suitability or
interaction with the experimental hard-normal relaxation. Shock/truncation
residuals can be removed as if they were noise. Reassess the filter with main's
geometric composition and converged flow before recommending a nonzero setting.

The rejected Schur updater and enabled-noise failure are separate results;
removing the former did not resolve the latter. Integration should preserve
main's sensor-only donor field, geometric BL evaluation and finer sensor demands.
Independent Hessian/WLS changes remain assessable separately; the BL, constrained
complexity and gradation architecture needs the reconciliation described above.

## Noise configuration and main-compatible gradation proposal

The noise filter is already controlled by one config option; no second boolean
is needed. `ADAP_HESSIAN_NOISE=0.0` is OFF and remains the default. A positive
value is ON and sets strength; `1.0` uses one estimated uncertainty.
Enabled filtering requires `NUM_METHOD_HESS=QUADRATIC_LEAST_SQUARES`.
The config template now gives explicit OFF/ON examples. Three existing checks
for default unfiltered noisy curvature, opt-in shrinkage and a resolved steep
profile passed again; this does not supersede the failed enabled-noise RAE gate.

The following records the earlier proposal. The integration base now implements
sensor-only gradation, geometric query composition and BL-resolving complexity
quadrature. Its combined-field audits and remaining limits are documented in
`METRIC_ROBUSTNESS_INTEGRATION.md`; full composed-field correction remains open.

1. For each complexity scale trial, bound and grade the sensor tensors using
   transported-neighbor intersection. Keep hard-normal BL projection out of
   that sweep. Require the actual directed-edge residual for acceptance;
   small changes alone do not establish that constraints are satisfied.
2. Freeze the resulting sensor field on the donor. At each actual query,
   interpolate that field and invoke main's geometric BL composition against
   the original wall. Preserve finer sensor demands and the existing fade;
   apply the BL constraint once. Do not interpolate composed nodal wall tensors.
3. Estimate complexity from that same query composition, using quadrature that
   resolves the thin geometric band on coarse cells. The fade's core parameter
   must be consistent per trial and frozen for final queries; no collectives
   belong inside candidate-point callbacks.
4. Audit the composed field separately. Sensor-field gradation does **not**
   automatically establish gradation after geometric BL intersection, including
   between sampled donor nodes and new candidate points. Main's lower-bound
   intersection removes the hard-normal reset conflict, but size/aspect limits,
   geometry variation and finite propagation still require checks.

If full composed-field gradation is required, a separate geometric-aware
correction representation is needed. Evaluate BL at the transport/query
locations and retain only the additional gradation demand, with support adequate
to resolve its spatial variation. Merely interpolating a nodal correction that
contains the fine wall tensor can reproduce the coarse-seed spreading defect.
This needs design and validation before code integration, especially for private
and remote queries. Main's original-geometry callback remains authoritative.


## 3D stress checks and noise isolation (2026-10-07)

Source `3b320bfd6b`; the complete receipt, including failures, is
`integration_evidence/metric_robustness_3d_noise_v4.json`. The production binary
contains the restart-reader fix and no metric-probe instrumentation. Ten selected
Hessian/restart cases passed on one, two and four MPI ranks (91482 serial
assertions). These checks establish the stated regression gates; **WLS accuracy
on the severe curved case is not a passing gate**.

### Severe 3D recovery limitations

The existing manufactured pressure helper now also exercises rotated, skewed
and doubly curved tetrahedra: flat aspect 10000/skew 4, curved aspect 1000/skew 2,
with refinements 8 to 16. Errors are measured in layer coordinates, preventing
large physical transverse curvature from hiding tangential error.

| Case/method | Coarse interior Hessian max error | Fine interior Hessian max error | Fine wall transverse relative error | Fine dominant-direction sine |
|---|---:|---:|---:|---:|
| Flat WLS | 8.91e-8 | 5.88e-7 | 0.5 | 6.14e-8 |
| Curved WLS | 10.45 | 319.81 | 0.384 | 0.963 |
| Flat QR | 5.51e-9 | 2.89e-8 | 4.22e-10 | 2.98e-8 |
| Curved QR | 0.741 | 0.210 | 0.129 | 0.0108 |

Wall errors are maxima over the marked wall, including its edges/corners; the
transverse direction is the layer-coordinate direction. This does not isolate
an interior wall patch or prove that the entire wall has those errors. The
initial accuracy-gated WLS run failed, and its log is retained. Comparing reused
WLS geometry with the repeated legacy solve gives differences below 1.2e-13
of the largest Hessian entry: the accuracy weakness predates the reuse change.
QR improves in the curved interior, but the worst wall error does not decrease
on these refinements. One-sided curved-wall reconstruction needs further work;
finite tensors and a good geometry condition diagnostic alone are insufficient.

### A matching ONERA M6 RANS fixture exists

`/media/rausa/4TB/SU2_Versions/TestCasesForSSTIDDES/optimization_rans/steady_oneram6`
contains a RANS/SA mesh and frozen restart: 96252 points, 545438 tetrahedra,
exactly matching coordinates and unchanged six solution fields. The snapshot in
`cont_adj_rans/oneram6` differs from its mesh by up to 0.0284 in coordinates and
was rejected as a matching fixture. No adapted RANS version was established.

Keep the true symmetry boundary. QR currently rejects it, so these runs exercise
WLS's existing symmetry path, with within-call geometry reuse disabled. No flow
or remesher iteration is performed; selecting MMG allows 3D metric construction
but does not invoke MMG. There is no prescribed BL metric in this check and no
exact Hessian or aerodynamic accuracy reference.

| Ranks | Process (s) | Metric computation (s) | Hessian recovery (s) | Relative metric MPI difference |
|---|---:|---:|---:|---:|
| 1 | 9.876 | 0.373 | 0.135 | 0 |
| 2 | 6.129 | 0.203 | 0.0744 | 2.46e-9 |
| 4 | 4.220 | 0.108 | 0.0419 | 7.37e-7 |

The precise metric timings in the JSON receipt take precedence over rounded
values here. Metrics are finite SPD and independently integrate to complexity
300000 within 4.2e-12 relative error. Hessian MPI differences are below 2.71e-11
with the documented global floor. **The strict 1e-9 metric MPI gate fails** on
two/four ranks; do not relax it to claim readiness. Independent eigenvalue
analysis sees aspect 10000.00023 for configured 10000, reflecting visible
roundoff in these condition-number 1e8 tensors. The 3D discrepancy needs separate
isolation; it is not proof of the same cause as the 2D noise failure.

The real run exposed a filename buffer overflow in the shared ASCII restart
reader. Both shared ASCII/binary readers and both output-header readers now use
the existing string directly instead of copying it into `char fname[100]`.
A >100-character filename regression covers all four paths on 1/2/4 ranks.

### Noise failure occurs before gradation

The investigation-only `integration_evidence/frozen_metric_probe.patch` captures
pre-BL tensors/scale and permits fixed-input replay. It is **historical instrumentation, not production
configuration or a merge candidate**. It targets pre-rebase source `3b320bfd6b`,
not the current integrated BL implementation. Check out that historical source
in a disposable worktree/build before applying it;
run `replay_frozen_metric.py`, restore the source and rebuild. The final patch
also captures the initial sensor intersection before corner/BL/gradation work;
use `--capture-all` for that campaign. The replay still performs preceding root
trials, so its elapsed time is not a single-pass gradation benchmark.

With identical serial tensors and final scale, MPI differences stay below
3.4e-14 at 0/1/10/80 sweep caps. Removing experimental hard-normal BL projection
from the same frozen input converges in 32 sweeps to ratio 1.00000009965; MPI
agreement remains below 6e-13. Keeping that projection stalls at ratio 2.28272,
17 normal-limited edges and 80 sweeps. Thus the constraint-policy failure and
the original enabled-noise MPI discrepancy are separate issues.

The initial sensor tensor already differs by 5.47e-4 / 8.36e-4 on two/four
ranks, before any corner adjustment, BL composition or gradation. Filtered QR
Hessians are identical, and sensor normalization integrals differ by less than
8.1e-15 relative. At a worst point the filtered sensors are nearly rank one.
**Inference:** the highly conditioned multi-sensor intersection amplifies the
normalization roundoff (its intermediate eigenvalue ratio cap is 1e14). This
narrows the fault to upstream construction; isolating and stabilizing the
individual operators remains necessary. No numerical intersection/reduction fix
was added, and default noise strength remains zero.

### Next improvements and durable evidence

1. Stabilize multi-sensor combination for nearly rank-one tensors in 2D/3D;
   include tiny normalization perturbations and MPI agreement without discarding
   finer demands. Accurate sums can reduce input error but cannot by themselves
   certify a stable intersection.
2. Improve one-sided curved-wall and skew-sensitive recovery, with separate
   tangential/transverse and wall-interior/edge accuracy checks.
3. Support symmetry/periodicity in QR before using this real wing for QR/noise.
4. Validate against the integrated geometric BL policy and coordinate any failures
   with its owning agent. Rebase onto the published integration is complete;
   superseded nodal hard-normal experiments have not been replayed.
5. Validate actual adaptation/flow cycles, forces, conservation and wall
   resolution. Native 3D remeshing, AD and structured BL topology remain open.

The report, scripts, probe patch, logs, configs, frozen probe tensors and output
fields are also preserved outside `/tmp` in
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/metric_robustness_3d_noise_v4`.
Its manifest records file sizes and SHA-256 hashes. Input RAE files are copied
there; original ONERA inputs remain in their existing test directory. Previous
reports and the full deferred roadmap above remain retained.


## Rebase onto the geometric BL integration (2026-10-07)

The branch now descends from published integration `2abbd11769`. Only the two
post-integration commits were replayed (`3b320bfd6b` -> `1eddab22a6`,
`ce28cd2f99` -> `0164ee16dc`). The duplicate shared restart-reader fix was
resolved in favor of the integration implementation. Relative production changes
are limited to the remaining `CBaselineSolver` filename fixes; the integrated
BL, complexity, solver metric and query code are unchanged.

Fresh build and selected tests passed 26 cases on each of 1/2/4 MPI ranks,
including Hessian, long restart filename, geometric query/quadrature and sensor
gradation checks. Fresh frozen ONERA outputs are byte-for-byte identical to the
pre-rebase runs. Its strict MPI metric gate still fails at the same values; no
accuracy limitation was fixed by rebasing. Historical probe instrumentation
requires its historical source and must not be applied to the integrated policy.

Receipt: `integration_evidence/metric_rebase_v1_validation.json`. The old branch
tip is retained as `codex/metric-robustness-pre-integration-rebase-20261007` and in
a verified complete-history bundle under the durable evidence directory. New
receipts/logs are saved there in `metric_rebase_v1`; unchanged ONERA fields remain
in the earlier v4 archive, with hashes and paths recorded by the new receipt.

Next implementation work belongs to Hessian/sensor metrics: stable multi-sensor
combination, skewed/curved one-sided recovery and QR boundary support. The other
agent continues to own BL composition, complexity and gradation; our branch
validates against that shared contract.
