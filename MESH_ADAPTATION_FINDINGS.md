# Mesh adaptation findings and follow-up work

Working branch: `codex/metric-robustness`, fork https://github.com/rois1995/SU2.
This file retains the investigation, implementation status, and deferred work.
Baseline validated implementation: `e17a96d301`; detailed measurements and replay
script are in `integration_evidence/metric_robustness_v2_validation.json` and
`integration_evidence/check_rae2822_metric_constraints.py`.

## Current authorized work (2026-10-07)

| ID | Work | Status |
|---|---|---|
| 1 | Constrained tensor gradation: classify blocked directions, converge useful updates, reduce repeated work | Direct updater rejected by actual-case checks. Retained relaxation/performance changes; main representation reconciliation required |
| 2 | Noise-aware Hessian treatment, with controls that retain resolved curvature | Implemented opt-in residual shrinkage; enabled RAE gate fails upstream metric construction. Fixed-input gradation is MPI consistent; retain default zero |
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

The following is a proposed integration sequence, **not implemented main support**:

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
pre-BL tensors/scale and permits fixed-input replay. It is **not production
configuration or a merge candidate**. Apply it only to a disposable source/build,
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
4. Reconcile complexity/gradation with main's geometric BL query policy, then
   rebase after the main agent's integrated branch is published. Superseded
   nodal hard-normal experiments must not be replayed onto it.
5. Validate actual adaptation/flow cycles, forces, conservation and wall
   resolution. Native 3D remeshing, AD and structured BL topology remain open.

The report, scripts, probe patch, logs, configs, frozen probe tensors and output
fields are also preserved outside `/tmp` in
`/media/rausa/4TB/SU2_Versions/SU2_AdapNoExt/integration_evidence/metric_robustness/metric_robustness_3d_noise_v4`.
Its manifest records file sizes and SHA-256 hashes. Input RAE files are copied
there; original ONERA inputs remain in their existing test directory. Previous
reports and the full deferred roadmap above remain retained.
