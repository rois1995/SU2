# Bounded complexity on current normalization trials

Full Goal3 remains ACTIVE/incomplete. Base before this batch:
`cffdfe8ec16a706dfea0ceb6d390e06a19c4b4b7` on `codex/native-3d-bl`.
Evidence root: `integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/production_complexity_v1`.

## Connected production change

`CNativeComplexity3D` retains original source-cell/node indices, exact-containment
barycentrics and actual-point wall geometry. It never retains the trial sensor
answer. Every assessment interpolates the CURRENT bounded/graded node tensors and
applies the existing geometric BL composition/fade at that query and current
frozen core. Finer sensor demands and tangential-floor disabling remain unchanged.
The original source and its ownership must remain fixed for the object's lifetime.

The complete volume sensor contribution uses positive18-point Duffy quadrature.
Minkowski determinant concavity and Jensen give the per-cell lower density
`(mean det(M_i)^(1/3))^(3/2)`; Jensen and the exact cubic determinant average give
upper density `sqrt(mean det(M))`. These are exact-arithmetic inequalities, not
outward floating-point certificates. Scaled long-double LDL pivots avoid direct
cofactor cancellation. Lower/upper sums are reduced globally, rather than requiring
each cell to achieve a common relative error.

The BL correction uses the complete finite-wall face/edge/vertex proposal. Rank0
prepares/draws the deterministic8192-point stream once, then releases the sampler
and routes disjoint point batches. Exact original-source footprints identify actual
publishers; geometry-only stencils live on those publishers. Empty exact footprints
are outside-domain zeros; failed/missing/oversized imports never become zeros.
All samples, including outside zeros, enter global moment normalization. The error
indicator is three empirical standard errors plus half the background interval;
fewer than32 nonzero samples leave it infinite. This is HEURISTIC and can miss rare
heavy contributions; no statistical/machine bound or calibrated default tolerance
is claimed. Source search and wall-distance preparation do not repeat during trials.

`CSolver` retains successful existing strict rules and their existing65536-point,
16384-panel/work and32-depth ceilings. Typed faceted volume work/point/depth exhaustion triggers one collective switch to the decomposed global estimate;
invalid geometry/density and unsupported analytic authorities retain their errors.
The switch is permanent within that metric initialization. Analytic3D and2D retain
their existing geometry integrators; differentiated builds retain their old path.

Eligible faceted3D normalization uses the shared uncertainty-aware search with at
most24 callback trials. Repeated requests for the same scale reuse the SAME assessed
field. Strict angular/radial stopping contributes a nominal1.1e-4 relative heuristic;
that is not proof of an error bound. The requested root tolerance remains1e-6. A
field that cannot support it is reported integration_limited/provisional, rather
than an exact root. Outside-bracket status is explicitly estimated. No budget,
wall height, geometry, positivity, quality, transfer or full-edge mesh gate changes.
The selected sensor and core remain frozen during local remeshing operations.

## Validation and resource state

Rejected first strict callback cutoff8192: the existing planar wall integral was
629.0712 instead of independent636.4348 (1.157% low, within its heuristic indicator
but outside the unchanged0.05% reference gate). Rejected65536-query cutoff also
switches this case. Both executions are retained. No further query-count ladder:
remove the new callback cutoff and use the existing point/panel/depth limits to
trigger collective fallback. Those existing limits are not raised.

The rotated1e10-condition field exposed floating-point inversion of the nominal
background interval. The estimate now separately reports a first-order roundoff
indicator64*binary64_epsilon*estimated_condition*density. The condition estimate
uses the matrix row-sum norm and inverse trace from the same scaled LDL pivots.
This covers interpolation/determinant conditioning heuristically, NOT with a
proven bound. Numerically inverted endpoints remain an explicit uncertainty,
never an exact root or a silent discarded contribution.

The connected focused batch passes:18 existing2D/3D composition/controller cases
(1795 assertions), the actual planar solver/reference (82), changed-field/reversion/
rotated-anisotropy/fault control (20 serial and per MPI4 rank), actual intersecting-
wall solver control (84), complete planar support and whole frozen M6 on2/4 ranks.
All5326 inside-source M6 samples per assessment compare cached geometry against
actual-point composition; baseline -> spatially scaled/clipped field/new core ->
baseline restores the original result exactly. MPI2/4 agree to the2e-12 criterion.

The planar complete-support estimate13.89942324 differs from independent coarea
13.90130035 by0.0135%, inside its heuristic indicator0.09953922. The exact existing
planar solver gate0.05% is preserved by retaining strict integration. M6 original
background lower/upper142421.03244/154272.45539 match the independent audit. Its
Q18 diagnostic152987.09178672 agrees with the independent SAME positively oriented
rule within4e-16 relative. The older sorted-source rule152994.20093906 uses different
points for151756 negatively oriented source records: sqrt(det(P1)) is not a cubic,
so Q18 is not invariant to vertex permutation. The rejected verifier's assumption
is preserved; native outputs were reused after fixing verification, not rerun.

Current frozen-field M6 mean940969.8012 has heuristic uncertainty34056.6968,
including correction792623.0573/empirical SE9376.9951 and background numerical
indicator1.56e-7. The clipped nonhomogeneous trial gives986172.5647 +/-35970.2764.
These are not exact full-mass references or proof of accuracy; the independent
coarea/reference coverage and historical sparse-cell failures remain qualified.
The cheap whole-volume composed Q18~780726 under-resolves this same original field
relative to complete-support estimates; do not confuse it with the coarsest field
in the actual runtime below.

`m6_runtime_metric` completes one saved-restart RANS/SA iteration and the REAL
initializer on4 ranks, exit0. No adapted mesh or transfer is attempted. At the
coarsest permitted donor (min/max density1e-6, hmax100), the estimate is790200.4998,
heuristic indicator27958.4546. Target100000 is outside ESTIMATED attainable bounds,
reported provisional. This supports the earlier independent conditional lower
mass>362k: a finer mesh cannot make the SAME field's continuum demand100000.
The input budget/height/fade/geometry remain unchanged; no silent renormalization
or accepted-mesh claim follows.

Actual metric415.98s, of which metric-field assembly408.38s: strict quadrature230.56s before
collective fallback (178 successful cell-rule builds), gradation168.89s across7
trials/1218 sweeps. These nested maxima are not an additive per-rank timeline;
gradation includes halo/reduction waits. Bounded preparation5.831s; all7 bounded
assessments max1.383s (background1.328, correction0.03165, reduction0.000164).
46,717,286 background determinants/37,282 correction queries; retained helper max
14,207,936bytes. Trial query-zero counters refer to donor/wall spatial searches;
background volume determinants are still recomputed. New total adaptation speedup
is NOT established and this initialization is not affordable for unsteady work.

Whole pilot425.42s and peak process613.5MiB; rank lifetime peaks are in stage rows,
not a simultaneous aggregate peak. Sequential checks/build-j1/nice19/thread1/MPI<=4,
host38–40GiB available. Host pre-existing swap rose747->800MiB during the broader
batch; no attribution is established. The first read-only stack sample only shows
MPI waiting; later samples found already-terminal PIDs and did not restart them.
All owned handles are terminal. Root/tmp pressure avoided with4TB compile-tmp.

Build diagnostics, both rejected cutoffs and invalid runtime fixture are preserved.
Both cutoff executions have gzip source capsules verified against EVERY available
source pin; large historical binaries are not duplicated. Corrected fixture uses
hmin1e-6 for h01e-6; the configuration gate was never relaxed. Execution provenance
is separate from verifier hashes; independently complete groups are resumable.

## Remaining end-to-end work

This batch does not implement post-refinement reassessment/correction stages.
The estimator's local node indices are deliberately valid only while original CFD
geometry/ownership remain alive. The complete strategy must preserve the original
source tensors before replacement, reassess that SAME field using candidate-mesh
integration or admitted temporary subdivisions, and retain explicit frozen epochs.
A material corrected budget requires a bounded new normalization/remeshing stage,
never continuously rescaling a candidate or refitting samples on its mesh.

A possible integration point is BEFORE mesh replacement/reference publication:
old CFD geometry/source are still intact and a rejected candidate preserves the
solution. Any retry must use the original unbounded eigenframes/bounds/gradation,
not scale the already clipped/graded answer. Stage limits, candidate reference
rollback and uncertainty/status propagation need to be connected across solver,
driver and remesher, not another replay-only variant. This is a design direction,
not an implemented/validated API.

Still open: calibration/rare-tail coverage, work allocation by measured global
error, same-original-field refined-mesh reassessment, accepted M6 construction,
transfer/viscous accuracy and complete adaptation affordability. Broader donor-
campaign retention, shared-edge lifetime and independent transaction batching are
separate recorded work, not implemented by this complexity cache.

## Geometry-aware preflight follow-up — current

The connected follow-up is validated: eligible nonplanar faceted walls select the
bounded estimator immediately; planar references retain strict accuracy. The
coarsest bounded/graded original sensor is assessed before expensive trials. Outside
estimated bounds retains that same endpoint; reachable budgets continue the search
with total24-callback ceiling. Cached source volumes add explicit payload accounting.
No field/geometry/fade/height/full-edge/transfer gate changes or higher work limits.

Actual same-config M6 metric415.97->16.47s (summed rank CPU1480.46->48.18s),7->1 trials,
1218->1 sweeps,51.9M->61330 point visits; strict builds178->0. Exact same estimate/
uncertainty and CFD restart bytes. Target100000 remains outside the estimated
790200.50 +/-27958.45 mass; no construction or new accepted mesh. Cold preparation
5.83->7.69s is now dominant, so no preparation/volume-cache speedup is claimed.
This is a contended initialization comparison, not full adaptation/scaling evidence.
Nine connected serial/MPI groups pass including unchanged planar0.05% accuracy,
reachable-budget actual solver and whole original RAW9 M6 MPI2/4. One owned heavy
job/build-j1/nice19/threads1,38–40GiB host available; build peak process2.37GiB,
actual pilot577.72MiB. No aggregate RAM peak claim. All owned handles terminal.

[Implementation, exact comparison, limitations and reproducible evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/production_complexity_geometry_v1/README.md).
Next: calibrate global uncertainty/error-directed work and connect same-original-field
reassessment/frozen correction epochs before claiming a complete strategy. Do not
refit on adapted nodes, rerun unchanged rejected construction or add local fallbacks
without a revised diagnosis. Broader donor/edge/transaction improvements remain open.


Current whole-original-field calibration: two32,768-point streams and exact MPI2/4
rows preserve the original field. The separate background interval floor and
block-vs-pointwise variability prevent a default/root-accuracy claim; the sampled
planar control still misses the strict0.05% gate. No production behavior changed.
[Complete evidence and next decision](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/global_uncertainty_calibration_v1/README.md).


## Next connected implementation after sampling experiments

Reflected face pairs improved M6 empirical SEM15–16% but worsened the embedded
planar control45%, and were withdrawn. Production source/build/test binary is
exactly restored; [executed source, evidence and decision](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/face_pair_variance_v1/README.md).
Do not add another sampler/count/subdivision fallback. The separate background
interval and original-field reassessment/correction remain the structural gaps.

The existing Remesher3D lifecycle retains imported.original while workers operate
and before ReaderSlices/onAccepted publication. That is the ownership/lifetime
window to use. Worker Engine source indexes already serve the immutable original
field; rebuild/reuse decisions must account for parent N versus selected M worlds,
migrated original publishers, peak memory, and all collective participants. Current
point sampling requires explicit extension eligibility: never turn an unassociated
extension or failed query into a zero contribution or a new donor fit.

Reassessment must integrate the SAME original sensor/actual BL on the candidate
partition, distinguishing coverage, geometric-domain change and heuristic numerical
error. Q4/Q18 disagreement alone cannot certify missing thin BL mass; retain the
complete-support/unknown-mass status where necessary. Compare coarse and accepted
finer geometry against independent references before inferring that refinement
helps. M6 itself remains without an accepted candidate; temporary partitions are
not to be reported as delivered meshes.

A material supported correction starts a bounded new epoch with rebuilt target
identity. Reset from the original unbounded eigenframes and current bounds/gradation,
not clipped/graded output; freeze the new field through local edits. Repeated
normalization must not rebuild Hessians, append prediction histories or alter the
original CFD/flow/BDF state. Keep per-stage global work/time/byte ceilings and
provisional/unattainable/integration-limited states explicit. Return/transfer the
final candidate once; only the accepted callback publishes geometry/reference.
All geometry/h0/positive/q/J/full-edge/transfer gates remain unchanged. This coupling
is a design/required next implementation, not a completed API or working milestone.
