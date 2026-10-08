# Metric robustness integration

Integration branch: `codex/native-metric-integration`, based on completed native
main `9450c0880e2b2c1c39dfc98bc2c9655844ab8731`. Incoming merge parent:
`b14a14ea1dec771834d7d691fca27614605df39d` (`rois1995/SU2:codex/metric-robustness`).
Integration validation is complete; final proof is
`integration_evidence/native_metric_integration_v4/completion_audit.json`.
`AdapNoExt` and `codex/native-integrated` remain untouched. Push target:
`rois1995/SU2:codex/native-metric-integration`.

Robustness branch follow-up (2026-10-08): direct QR recovery now supports
mirror-even primal scalar sensors on symmetry markers. It reuses the existing
normal policy and Hessian workspace, retaining normal-normal curvature. This
extends the published integration baseline described below; periodic, goal and
differentiated QR configurations remain rejected. See
`Papers/IMPLEMENTATION_ASSESSMENT.md` and
`integration_evidence/qr_symmetry_v1_validation.json` for the literature assessment,
2D/3D manufactured checks, frozen M6 partition checks and measured method cost.

Robustness follow-up: primal adaptation GG now uses deterministic volume-weighted
P1 recovery on complete owned simplex stars through both passes. Ordinary flow,
periodic/goal and unsupported-cell operators retain their prior behavior.
Geometric BL composition, gradation and complexity remain the shared integration
policy below. See `Papers/GG_BOUNDARY_RECOVERY.md` and
`integration_evidence/gg_simplex_recovery_v1_validation.json` for improvements,
rejected boundary-only trials, MPI gates, native cost and limits. The future
convective/viscous transfer request is retained in `GRADIENT_TRANSFER_FOLLOWUP.md`.

Latest recovery follow-up: single-cell-pass GG evaluation reduces repeated
geometry work using temporary owner sums; supported thin-wall QR fits can add
selected coupled quartics. The shared geometric BL/gradation/complexity policy
remains the contract below. See `Papers/RECOVERY_GEOMETRY_WALL_BIAS.md` for scope,
performance, rejected variants and the cycle-test integration plan. Mixed-cell
and future convective/viscous distinctions are retained in
`GRADIENT_TRANSFER_FOLLOWUP.md`.

## Reconciled metric contract

`Metric_*` stores bounded, graded **sensor tensors only**. Native donor queries
continue interpolating these tensors on original donor-cell connectivity. The
wall metric is evaluated from retained original geometry at the actual query,
including candidates reconstructed privately or imported across ranks. Complete
original wall geometry remains available to those queries; occupied nodal boxes
are not used as a certificate for arbitrary remesher points.

Both complexity quadrature and the remesher use
`CBoundaryLayerMetric::FromNativeReference` and `ApplySample`/`ApplyPoint`.
Composition remains an intersection: finer normal/tangential sensor requests
and their coupling are retained. The existing outer smoothstep weight and
logarithmic eigenvalue fade toward the frozen sensor core remain. Native does
not apply the legacy tangential coarsening floor. The incoming hard-normal reset,
Schur projection and propagation of composed wall tensors are excluded.

Complexity now integrates the **same geometric composition** inside owned
original triangles, with P1 sensor interpolation there. Positive geometric
distance bands resolve the prescribed first height even when interior donor
vertices miss the thin layer. Curved geometry is subdivided when its distance
cannot be represented by the parent vertices; a segment/triangle proximity
check detects bands missed by all three vertices. Exhausting the geometric
subdivision limit raises an error rather than silently omitting the layer.
Weights and geometry samples are cached once and reused during the root solve.
Cells have deterministic global-ID ordering and one MPI owner.

This changes the meaning of the native BL budget from the old sensor-only
budget to the composed integral. A request below the attainable wall complexity
is reported explicitly; wall demands are retained. Historical 4000/6000 controls
are not silently interpreted as higher budgets.

Sensor gradation uses synchronous sweeps and deterministic neighbor order.
Final halos are refreshed after the last sweep. Its transported-metric residual
is measured separately from the composed nodal residual. The independent frozen
audit also samples quarter-edge P1 queries. A converged sensor field alone does
not establish combined-field gradation. No additional composed-field correction
is introduced by this integration, and no thin wall tensor is copied into donor
nodes to repair an audit residual.

## Independent Hessian/WLS review

The shared WLS inverse equilibrates coordinate directions before its singularity
check. This addresses dependence on coordinate units and axis-aligned stretching;
it does not cure a missing direction or a genuinely deficient stencil. The
somewhat unusual 3D `R(2,1)` entry duplicates the x-z normal-matrix contribution;
its incoming scaling by x/z factors matches its assembly. Recovered stencil
condition is a passive diagnostic of the normalized normal matrix.

Within-call WLS Hessian reuse shares geometry/weights across sensors. It requires
the preceding WLS gradient on the same mesh and weighting; periodic, symmetry
and AD recovery retain their prior path. The serial tests compare it directly
against repeated recovery and include anisotropic 2D/3D, periodic, symmetry and
wall cases. Shared changes affect ordinary CFD gradients, so actual solved RAE
cross-grid cases are required in addition to manufactured recovery tests.

The direct quadratic method remains opt-in. Coordinate whitening and pivoted QR
avoid differentiating noisy recovered gradients twice; two-ring neighborhoods
are requested from owners because halo connectivity is incomplete. Deficient
fits retain the existing WLS fallback and expose residual/conditioning counts.
Periodic/symmetry/AD configurations outside its implemented scope are rejected.
A passing manufactured polynomial does not establish improved shock or viscous
force accuracy.

`ADAP_HESSIAN_NOISE=0` is explicit in every integration case. Enabled noise
filtering remains experimental. Incoming v3 enabled-filter RAE MPI and transported
metric checks failed; the independent reports are retained without changing
those gates. The primal eigensolver subnormal normalization fix is retained;
known extreme-scale AD derivative limits are not called fixed by this merge.

The broader assertion-enabled serial campaign uncovered an existing predictor
one-past-end iterator formed through `vector::operator[]`. The shared stiffness
assembly now uses `data()+offset`; its existing prediction test is the regression
check. Original failed evidence remains `native_metric_hessian_v1`.

## Evidence and validation

Working/case root:
`/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence`.
One heavy job is admitted at a time, build jobs <=2, MPI ranks <=4, computational
library threads =1. Controllers wait for other sessions' builds/solver jobs.

Final production archive: `native_metric_integration_build_v4`, 760 production
source hashes, MPI/MMG/CGNS primal application SHA256
`498bb9fa3916635ee9bb9d05a54270ceff2525b30deb887cbc65dc98f7e9f69c`.

| Validation | MPI ranks | Result |
|---|---|---|
| `native_metric_hessian_v3` | 1 | 105 recovery/numerics/predictor cases passed |
| `native_metric_hessian_mpi_v2` | 2, 4 | 23 Hessian cases per rank passed |
| `native_metric_core_v3` | 1, 2, 4 | 47 geometry/metric/cavity cases per rank passed |
| `native_metric_output_mpi_v3` | 1, 2, 4 | 44 CGNS/MMG/output/transfer cases per rank passed |
| `native_metric_adapter_v4` | 1, 2, 4 | 3 adapter cases per rank, including eight replacements and resumed CFD, passed |
| Frozen WLS and quadratic recovery on coarse/fine RAE grids | 1, 2, 4 | All 12 runs and independent MPI/unchanged-solution checks passed |

Hessian runs precede the final BL distance-guard fix; their Hessian/WLS/solver
sources are unchanged. The final core/output/adapter runs cover the final BL
implementation. Full binary and source pins distinguish these scopes.

Frozen evidence and independent auditors: `native_metric_integration_v4`.
Fine cases reuse completed v3 outputs from the identical final application;
coarse cases are fresh. Each case has zero CFD iterations and noise zero.
The coarse restart fixture retains original geometry verbatim; only accepted
bindings are reconstructed for the known original mesh after exact coordinate,
marker and component checks (`coarse_reference_preparation.json`). The initial
missing-sidecar restart rejection is preserved; its guard remains enforced.

The produced-BL continuation fixture was changed to configured freestream scales
while retaining its moving feature and every mesh/conservation/nonzero-update
assertion; v2 passed all3cases at1/2/4ranks. Its old energy3 state was about0.003K
against a288Kfreestream. Failed v1 is retained.

Long-path frozen restart reading exposed existing100-byte filename buffers;
both ASCII and binary readers now use their owning std::string data. Failed
frozen v1 before metric computation is retained.

Independent Gaussian cubature exposed a coarse-cell nearest-face switch outside
the active BL band. The shared distance-resolution guard now checks outside
midpoint discrepancies too. The rejected initial self-check and corrected
20.1281224004/20.1281226287tight-band result are retained in campaign v2. A C++
regression compares the same geometry against an independent1Dintegral.
Fresh final validation root is `native_metric_integration_v4`; its build archive
is `native_metric_integration_build_v4`.

## Frozen metric results and limits

All original restart flow/turbulence values are retained to relative tolerance
1e-14. Sensor tensors remain SPD, and geometric composition dominates their
finer demands. All twelve reported composed complexities are 60000. Independent
4x4 Gaussian integration with tighter distance bands differs by at most 0.0194%
(fine) and 0.0030% (coarse); it resolves the same original-P1 sensor target.
The nodal composed quadrature is recorded only as a counterexample, since it
misses thin-layer geometry inside coarse cells.

All four sensor nodal audits have zero directed edges exceeding 1+1e-5. The
transport test uses source-metric distance and the configured log(1.3) growth
bound. Its failures for composed and interior P1 queries remain explicit:

| Frozen field | Max MPI sensor relative difference | Composed nodal ratio / failures | Quarter-edge sensor ratio | Quarter-edge composed ratio |
|---|---|---|---|---|
| fine/weighted_least_squares | 1.42e-10 | 6.3996 / 3026 | 4.9112 | 7.7341 |
| fine/quadratic_least_squares | 8.4e-12 | 12.3892 / 4112 | 3.2936 | 15.8807 |
| coarse/weighted_least_squares | 1.77e-13 | 14.9075 / 396 | 2.5088 | 43.7069 |
| coarse/quadratic_least_squares | 1.77e-14 | 15.0024 / 412 | 2.7266 | 50.2865 |

Nodal gradation does **not** certify continuous P1 sensor gradation either.
Quarter-edge probes compare adjacent locations among five samples on each
original donor edge, composing geometry freshly at every location. This is a
measured limitation, not an operator acceptance waiver. No hard-normal reset,
finer-sensor relaxation or coarse-cell wall-tensor spreading is introduced.
`frozen_field_audit.json` includes failure counts, wall/full-band/fade/outer
breakdowns and the twenty largest residuals with coordinates and global IDs.
For example, coarse WLS has its largest nodal composed residual on original
leading-edge nodes 40/41 at (0,0)/(0.0001051,0.0011657); fine WLS has its largest
at (0.00298986,-0.00726831), in the full band. Trailing-edge residuals also occur.
These are metric transport residuals, not proof of rejected mesh cells.

Measured metric-only elapsed seconds on this workstation (one observation):

| Field | MPI 1 | MPI 2 | MPI 4 |
|---|---|---|---|
| fine/weighted_least_squares | 16.111 | 8.781 | 6.104 |
| fine/quadratic_least_squares | 15.325 | 6.712 | 4.800 |
| coarse/weighted_least_squares | 2.277 | 1.325 | 0.867 |
| coarse/quadratic_least_squares | 1.680 | 1.507 | 0.966 |

Fine WLS uses 303738 geometry samples, 29 root trials and 948 total synchronous
gradation sweeps; fine quadratic uses 27 trials/837 sweeps. Both final sensor
fields reach a nodal fixed point. At four ranks the fine WLS accumulated
gradation time is 3.228s, including halo 0.302s and sweep reductions 0.476s;
geometry preparation is 0.223s. Samples are cached between root trials.
These are metric-generation costs, not complete remeshing costs. Incoming v3
numbers used a different BL policy, so they are not an equivalent speedup
comparison. General optimization of repeated gradation/root trials remains
an opportunity, rather than an unmeasured scalability claim.

The coarse coarsest-sensor endpoint has composed complexity **6564.71** under
these configured bounds and retained geometry. Historical requests 4000/6000
are both below it. The actual RANS integration control therefore explicitly
requests composed budgets **10000/12000**, with its change receipt retained.
This is a feasible-budget choice, not a relaxed geometric/metric acceptance gate.
The remesher's raw-sensor centroid plus geometric-row work allowance remains an
approximate work estimate; it is not substituted for the composed budget integral.

## Actual cross-grid adaptation

The actual Euler control `rae2822_transonic_v1/metricmerge_euler_cross_seed_v1`
completed on four ranks in148.756s. Both adapted grids passed saved-flow,
reference and independent original-P1 metric audits: qmin0.250666/Lmax1.708282
then qmin0.342287/Lmax1.799994. Both conservative transfers had zero inadmissible
states and zero recovery patches; CFD resumed after each. Mesh points5722/7649.
Solve time87.703s, adaptation42.675s. The initial fine RANS seed reached the
80-sweep sensor-gradation cap with ratio1.02059 and563 directed residuals;
94 complexity trials occurred. Later fields reached nodal fixed points. This
is retained as a performance/gradation limit rather than hidden by the frozen
controls. The original-P1 final mesh gates still pass.

The actual RANS control `rae2822_transonic_v1/metricmerge_rans_cross_seed_v2`
completed on four ranks in339.040s. Both adapted grids passed independent
original-P1 sensor plus actual geometric-BL query audits:

| Cycle | Points / triangles | Minimum q | Maximum metric length | Relative first-height error |
|---|---|---|---|---|
| 1 | 12399 / 24400 | 0.180096002 | 1.799996891 | 4.92e-12 |
| 2 | 14234 / 27865 | 0.265992367 | 1.789548222 | 2.1e-09 |

Both conservative transfers had zero inadmissible states and zero recovery
patches, original boundary features/arc coverage were retained, surface edges
adapted, and CFD resumed. The second target reaches the80-sweep cap with
sensor ratio1.00062 and195 nodal residuals; the composed ratio is91.5395 with
13591 residuals. These are recorded metric-field limitations while the accepted
mesh still satisfies its actual q/length/height/reference contracts.

RANS solve time125.068s and adaptation196.112s **do not meet** the desired cost
objective of adaptation taking less time than CFD. First BL construction costs
99.271s against5.615s of initial coarse-grid CFD. Native candidate transactions,
selection and MPI rounds remain a measured optimization target; repeated sensor
gradation during root trials also adds cost. Different budgets, grids and solver
iterations prevent an equivalent-performance comparison with historical main.
The RANS final density residual is-7.911398 and SA residual-7.336547 atITER2000;
it has not met configured-8 convergence. No grid-converged force claim is made.

The final completion checker rechecks760production files, current binary and
unit archives, all final MPI groups, unchanged Hessian sources, all12frozen
field/input/auditor hashes, both actual cases/meshes/solutions/plots, and57raw
incoming artifacts. `post_frozen_v1.json` has nine successful sequential stages.
`preserved_integration_attempts.json` pins compact failed/cancelled attempt
receipts; none is called a validation pass. Build freshness reports no work.

Prepared RANS v1 is not called a run. Full 3D, native unsteady adaptation, CAD,
large-rank scaling and grid-converged viscous quantities remain outside this
merge's validated scope. No further experimental correction was introduced to
make the composed-gradation audit pass.

## Historical findings preserved

`MESH_ADAPTATION_FINDINGS.md`, `metric_robustness_v3_validation.json`,
`metric_robustness_rejected_schur_v3.json`, and v2 evidence remain verbatim from
the incoming branch. Its v3 numerical-source commit is
`887ca28f939302845e731051c410fbda22c2dc0a`; b14 adds findings/evidence/config text.
The historical checker `check_rae2822_metric_constraints.py` describes the
incoming **composite nodal/hard-normal** policy and is not rewritten to validate
the integrated sensor-only representation.

`metric_branch_v3_reference/inventory.json` pins 57 preserved raw fixture/log/
field files (224.02MiB) from the incoming default/enabled-filter/rejected-Schur
campaigns. Large fields and executable archives stay local, with compact hashes,
source snapshots and receipts committed. Incoming source and rejected experiment
history remain reachable through the merge parent. None of the failed policies
is relabeled a pass by this report.
