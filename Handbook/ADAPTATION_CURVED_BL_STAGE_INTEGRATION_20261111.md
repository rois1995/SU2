# Curved BL stage integration — current cadence

2026-11-11 user instruction supersedes immediate full-M6 retries after each fix.
Read HANDOFF_Codex.md for live handles/provenance. One connected cycle and one heavy
job; strict physical/resource gates remain. Euler293 completion/checkpoints preserved.

## Actual execution sequence and observed timing

Current same303 production workflow loads saved RANS+SA CFD and original mapped
no-CAD reference, continues its configured CFD step, constructs Mach-only L1 sensor
and combined target on MPI6, then invokes native FIRST_ROW with its existing serial
guard. Native import/ownership/geometry preparation precede RunEvent. Inside it:
PrepareSurface → transported core snapshot → Prepare/index/direction field → R1 →
R2 → CompleteFirstRow → first-child snapshot → PhaseG → validity/A3 checks. The
remesher then snapshots current transported core and calls AdaptMetric for full
size/geometry/quality closure, reassesses complexity and may use bounded source
normalization. Strict final publication/reader/solver validation precede conservative
transfer and replacement, continued CFD and the next event. Outer-transition
metric transport exists during target evaluation; a usable geometric outer
transition is not an independently demonstrated output yet.

Source: CSinglezoneDriver::MakeRemesher/AdaptMesh/ReplaceMesh;
CSolver::ComputeMetric; CNativeBLPipeline3D::RunEvent;
Common/src/adaptation/CNativeRemesher3D.cpp: worker construction and AdaptMetric.
RunEvent currently allocates .08*usable to surface preparation, .05 to R1, .25 to
R2, at least60s or .1*usable to completion and .15 to growth, all within the shared
native limit. This is the current implementation, not newly endorsed stage budgets.
No allocation or limit has been changed.

Matching full364 evidence (first event, before preserved construction failure):

| Work | Measured wall/cost | Scope |
|---|---:|---|
| Initial CFD geometry | max1.4644s | partition/migration/preprocessing, not CFD solve |
| Mach Hessian | max0.40331s | within metric stage |
| Complete metric | max1199.6663s | includes metric construction |
| Metric construction | max1199.2540s | nested composition/normalization/gradation/audit |
| Geometry proposal preparation | max251.2757s | nested, not additive to above |
| Composed assessment | max160.4093s | nested; includes profile refresh |
| Numerical correction evaluation | max0.006216s | excludes expensive sample preparation |
| Background integration | max0.093245s | nested |
| Sensor gradation | ~0.041425s | nested, not composed-field certification |
| Surface preparation |391.6188s |18commits/122work; stopped before first row |
| Surface seed measurement |311.7963s | nested in preparation |
| Surface measurement CPU |311.9047s | overlaps seed measurement |

Metric work per rank:10136289/12188969/10368433/8959758/7598151/4222169.
8192proposals/6516inside;6516preparation geometry queries and13032trial queries,
one trial, six rank refreshes. Composed estimate948069.856, empirical indicator
77214.161, no proven bound; sensor-only2.08846. Requested500k is NOMINAL, not a
rejection gate or node count. Partial surface output:61312used nodes/303310tets,
51891cached cells; incomplete shape/geometry/size census. Not a usable BL.

Rank metric CPU approximately1174–1175s includes possible MPI polling; CPU alone
cannot assign useful geometry versus wait. The above maxima/nested timers overlap;
do not sum them. No measured first-row/growth/closure/transfer time exists for this
full attempt because those stages were not reached. First-event failure was saved
before verified owned stop during the unwanted next event. Whole wrapper1972.886s
includes setup/output/stop and that later prefix, so is not first-event time.

## Inputs, outputs and acceptance by stage

1. **CFD seed and authority.** Input: immutable303 mesh/reference/RANS+SA restart
and matching config. The exact364/362metric-entry restart now resides in
R/m6_bl_inputs/rans_metric_entry364_v1 with verified finite/admissible fields, exact
coordinates and hashes; load WITHOUT another CFD advance for metric comparisons.
Original303seed remains preserved. Output: verified provenance and reusable finite/admissible
fields on the same node IDs. Preserve existing hashes in HANDOFF; no CFD recompute
without demonstrated reason. Coarse CL.15573/CD.05027 does not prove wall/shock
resolution or convergence. Keep that limitation explicit.

2. **Combined target (ACTIVE).** Input: stage1, Mach-only L1, nominal500k, h0=2e-6,
g1.2,T=.01 and unchanged bounds/geometry/features. Output: reasonable composed
field and a reconstructible target checkpoint. Require directional allocation,
physical curvature/tolerance/feature and first-row-feasibility attribution; normal
law and actual oblique advancement distinguished; correct recomposition/support
invalidation and gradation audit; composed-volume accounting with uncertainty,
not additive flow+BL costs. Current tangent candidate is not yet a selected target.
Use saved actual original contexts AND358 generated row/neighbor capture. No new
surface-generation attempt until target reasonableness and checkpoint fidelity are
established. Geometry or quality debt in these diagnosis inputs is reported honestly.

3. **Coupled surface preparation.** Input: credible stage2 checkpoint. Output:
transported core + current mesh/associations/coverage meeting measured tangential
readiness, geometry/body/feature constraints and positive adjacent cells. Preserve
passing heights; identify missing row obligations. Quality debt remains provisional;
an overlong-edge pass or fewer points does not prove readiness.

4. **Directions/first row.** Input: stage3 plus actual wall controls. Output:
coordinated valid cavities with constrained/blended directions, physical first
height, alignment and measured coverage, including root/symmetry, LE/TE, features
and competing walls. Rebuild dependencies after surface/ownership changes.

5. **Growth.** Input: credible first row, directions and target. Output: measured
physical normal progression and coverage with junction/collision/stall accounting;
preserve earlier passing layers. Metric-unit steps alone are not proof of physical
geometric progression. Temporary coordinated fronts are allowed, permanent column
freezes and compulsory prism connectivity are not required.

6. **Transition/closure.** Input: stage5 and surrounding volume. Output: complete
mesh meeting strict geometry1e-4, positivity, q>=.2,J>=.05,maxedge1.8, all requested
height/body/feature constraints and resource gates. Verify preservation after
splitting/coarsening/relocation/reconnection/migration. Only this is final mesh
acceptance; intermediate stage checkpoints are explicitly provisional.

7. **Integrated workflow.** Input: strict first accepted BL/RANS mesh. Output:
conservative admissible flow+SA transfer, finite continued CFD, strict second
adaptation and actual isolated adapted disk restart. Dedicated native-MPI constraint
qualification remains necessary; current native1 guard does not qualify it.

## Checkpoint fidelity and current gap

Reuse existing WriteReference/LoadReference (.native_ref), RecordStream/OwnedCell
and donor payloads, exact transaction/stage captures and CFD restart formats before
adding a codec. Existing in-memory CNativeMetricFamily3D retains original frames,
eigenvalues, selected nodal core, normalization metadata, full/base layer policies
and geometry. Complete codec395/396 now preserves that family; see the current checkpoint section below.
A sensor-only file or Float64 visualization is NOT a complete target checkpoint.

A credible stage2 checkpoint must bind original CFD node/donor identities and exact
selected core tensors to the original wall geometry/parameter authority, config,
normal/tangent/feature/Option-A/outer-transition policy and normalization state,
including any support/history needed to reconstruct the SAME target. Preserve
original-family data if later source-family reassessment will use it. Record
binary/source/input/helper hashes and resource/acceptance receipts. Reload in a
fresh process and verify actual M6 point/chord/tensor/authority agreement before
claiming upstream reuse; do not substitute sensor-only reconstruction.

Complete target persistence is now implemented/qualified396; stage dispatch remains unchanged.
The current bounded target/row investigation finishes first. Use stage checkpoints
only when credible; a failed or partial diagnostic is evidence, not a resumable
successful stage. Full retries are now integration milestones, not the default
next action after every correction.

## Historical371 checkpoint audit — resolved by395/396

Source audit during371 confirmed a specific reconstruction gap. The native3D
reference sidecar preserves geometry/control curves, marker/facet identity and
parameter/mobility records. RecordStream can encode explicit scalar/array records;
it does not serialize runtime metric pointers or vectors automatically. The
current diagnostic writes exact selected tensors and Mach, but omits original
frames/eigenvalues and full/base transition state. It remains explicitly incomplete.

ConfigureOuterTransition monotonically retains each wall's support and expands
its low/high broad-phase bounds. A later finer core may leave these bounds intact;
feature reindexing and outer-profile changes also affect the transition identity.
Therefore final config + selected tangent scale + final core alone does not
reconstruct the same retained state. This is confirmed source behavior, not an
observed discrepancy in371 (371has since terminated successfully; no checkpoint codec yet).

The smallest candidate persistence path is the existing encoded reference plus
explicit family records keyed by global ID/coordinates, selected and original
frames/eigenvalues, normalization/bounds/normal and tangential policy, and the
ordered policy/transition updates needed to reproduce support history. New caches
must start empty; accepted associations are not reusable distance certificates.
Alternatively, exact effective support/bounds could be persisted only with a safe
restore/revalidation API, which does not currently exist. Do not silently choose
final-core reconstruction or treat the diagnostic export as that API.

Choose and qualify the persistence change only after the target result is credible.
Fresh-process reload must validate identities/config/authority, complete record
counts, storage/time admission, requested controls and actual M6 point/chord/tensor
agreement, including seams, features, competing walls and stale-state refusal.
Stage3must not consume the current sensor-only diagnostic as a complete target.


## Current checkpoint and physical assessment status (399/404)

The historical371gap above is resolved by complete target codec/profile replay395/396.
Same sourceCFD/authority, complete original family and originalselected core, both
full/base profile histories/transition support and OptionAstate are preserved.
FreshMPI6reload reproduces61330owned composed tensors and difficult chord BITEXACT
without CFDadvance or metric recomputation. Payloads/receipts are in currenthandoff;
originalsource61330points/303359tets. Nominal500k, composed estimate461495.991 with
47864.008empirical indicator, not bound. Checkpoint fidelity is qualified; physical
suitability remains bounded to measured controls, not yet a credible usableBL.

398/399measure exact-policy original contexts and saved25row/84neighbor coordinates.
25rowpositive/geometry/height pass, all25centroidq/Jbad. Frozen-metric controls do not
prove fixedface/height target incompatibility.399composed nodal transport localization
reproduces123186bad/core0; notcontinuousgradation/noisecausality.402/404test existing
private fixedskin interior motion/reconnection under fresh changed-position queries.
402exposed passingheightloss;403/404fix qualifies preservation with serial/MPI/actual
evidence, while zeroq/Jbad retirement remains. No newpolicy/operator/globalpublication.
Handoff owns currentterminal/live handles.
Do not rerun metric/checkpoint upstream for this stage test. Do not confuse a private
improvement or incomplete transition with final complete-mesh acceptance.
