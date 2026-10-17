# Suggestion outcomes — working ledger, final report pending

Requested2026-10-16: final report must map EVERY suggestion to worked/failed/
inconclusive/not attempted, quantify gains and costs, and explain negative results.
Do not count an unattempted item as a failure or a local replay gain as a complete
adaptation improvement. Include saved/rejected evidence, source/commit provenance,
accuracy, work, budget discrepancy, memory/communication and remaining scope.

| Suggestion | Current status | Evidence and limits |
|---|---|---|
| Match normalization to integration accuracy | Qualified replay; production pending | Shared controller passes25 assertions. Three original M6 cells agree exactly on1/4 ranks: target10, estimate10.10364 ±.22168,8 trials,56632 queries (current distance-band kernel); correctly integration-limited at1%. No exact root or complete-field accuracy claim. Production normalization unchanged. |
| Allocate absolute error globally without gathering cells | Qualified on3-cell replay only | Absolute-error priority and collective failure handling agree on1/4 ranks;37 reductions/rank,23 cell estimates,133352 accounted cache bytes (current distance-band kernel). A bad cell owned by one rank is rejected collectively. No all-cell gather; full frozen-field coverage remains incomplete. |
| Reuse integration geometry across scale trials | Worked in local replays | Cell88 2.2006s→.11325s at58112 accounted bytes;157 .12095→.02311s;329 .17431→.04663s. Original/changed-sensor-and-core warm/cold estimates identical. No complete normalization/adaptation speedup established. |
| Integrate BL correction separately | Partial local benefit; support-restricted geometry not implemented | Exact algebraic decomposition on the existing geometry kernel:329 queries6092→5644 (7.35% fewer), reference error.14414%→.01362% (10.6x lower).88/157 essentially unchanged; cold chord counts unchanged on all3. Does not resolve cold preparation or qualify full-field overlap/corner coverage. |
| More integration subcells | Negative evidence |329 error is nonmonotone with subdivision;88 work/time rises substantially. Raw indicator can underestimate157 by1.91x. Not a demonstrated cure. |
| Cheap scalar geometric-root screening | Withdrawn: no useful measured gain | Seed estimates unchanged on88/157/329;88 .2003s→.2090s. Extra code removed; seed_screen_v1 retains source/results/decision. |
| Shared policy in2D | Partially qualified foundations |8 existing composition controls pass95 assertions;2D production normalization unchanged.30P30N planar input prepared/validated, not adapted. |
| Actual refinement should ease integration | Inconclusive; complete construction hits budget | Shared edge correction passes both exact M6 blockers. Same-budget MPI4 reaches447 INTERNAL first-row publications, then stops at300s. No accepted complete mesh exists for finer-mesh reassessment. |
| Reuse donor support/indexes across repair campaigns | Not attempted in this follow-up | Existing reuse must be inventoried; integration-geometry cache above is a different scope. |
| Extend shared-edge integration reuse | Not attempted | Deferred until complexity initialization is resolved. |
| Batch independent mesh transactions | Not attempted | Deferred; compare full lifecycle and accepted/rejected edits, not only kernel time. |
| Avoid unchanged failed searches | Not attempted | Must distinguish stable geometric rejection from incomplete support or temporary reservation conflicts. |
| Reuse transfer geometry across fields/histories | Not attempted | Inventory existing sharing; geometry/time-history identity and physical checks remain required. |
| Measured-cost partitioning/mid-remeshing migration | Not attempted | Include migration/index rebuild/return/solver rebuilding. Use useful work costs; MPI waiting is not itself useful work. |

Detailed receipts and current limitations: GOAL3_UNCERTAINTY_COMPLEXITY.md.
Deferred performance scope: POST_COMPLEXITY_QUERY_TRANSACTIONS.md.


## Additional performance accounting — 2026-10-17

Requested in the performance discussion; these are pending investigations, not
validated improvements. Preserve the distinction between cold preparation,
repeated queries, initial startup and recurring adaptation costs.

| Suggestion | Current status | Evidence and required assessment |
|---|---|---|
| Account for work outside the density callback | Gap identified; finer breakdown pending | Saved cell88 warm replay: total .113250s, density callback .007946s; about .105304s (93%) lies outside that callback. Wall SamplePoints is constructed before it. Separate wall search/sample construction, coordinate placement/exact containment, cache operations and integration bookkeeping; retain an unattributed residual instead of assigning it all to geometry. |
| Bound cold geometric preparation independently of density queries | Partial per-chord bound; whole-cell/global budget pending | A density-query ceiling does not sufficiently bound candidate-facet processing, distance-band root generation or nearest-wall searches. The chord-envelope experiment bounds features/pieces and comparisons per chord; scalar exhaustion retains a positive contribution with unknown uncertainty. Whole-cell/global preparation budgets and finer accounting remain to qualify. Measure preparation work and enforce explicit bounds with retained approximate contributions/unresolved uncertainty; do not silently omit BL mass or weaken mesh acceptance. |
| Qualify reuse under whole-mesh memory limits | Pending; three-cell cache gains only | Measure peak RSS and transient/allocator memory in addition to retained-byte accounting. Assess useful hits, evictions, rebuild work and geometry/field invalidation under the actual cache budget across cells, scale trials and adaptation events. Do not extrapolate three-cell reuse to the complete mesh. |
| Reconcile complete elapsed time with stage accounting | Gap identified; lifecycle reconciliation pending | Saved M6 construction-only replay: 348.216s overall versus 104.587s backend, with about 117.532s in initial CFD geometry. Attribute the remainder; separate one-time initialization from recurring adaptation/rebuild and launcher/shutdown costs. Reconcile nested stages without double counting; include repartition, migration, transfer, rebuild and output in the complete lifecycle. |
| Separate useful work, MPI waiting and measurement overhead | Pending qualification | Report elapsed critical-rank stages and CPU time separately from accumulated rank-seconds; do not subtract rank sums from wall time or sum unrelated per-stage maxima as an exact critical path. Check instrumentation overhead with bounded coarse/sampled measurements and distinguish host contention from algorithmic imbalance. |

Evidence: [cell88 reuse receipt](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/geometry_reuse_v1/cell88/geometry_reuse.json),
[M6 construction receipt](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/construction_first_trial_v1/result.json)
and [its stage log](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/construction_first_trial_v1/run.log).
These contended local observations identify investigation priorities; they do not
establish scaling or complete adaptation savings.


Additional structural experiment: global finite-feature chord distance envelope
passes20 strict/analytic controls but remains MIXED:88 cold1.997s->.5055s, while
fixed-budget reference error.7682%->4.9486%;157 similar accuracy/faster,329 faster
with worse accuracy. Extra nearest-feature cuts consume query budget. Whole-field
global error must decide usefulness; no lifecycle benefit claimed. Details and
remaining work: GOAL3_CHORD_ENVELOPE.md.


Larger-field check of that structural experiment is terminal:9442/303359 cells in
205.88s,14 unresolved contributions with infinite total indicator. Strict feature
cuts exhaust provisional scalar seeds on some previously finite cells. This is a
regression requiring correction, not successful whole-field normalization. Positive
approximations/evidence retained; no BL contribution silently discarded. Current
adjustment separates scalar distance-band partitioning from strict edge transition
cuts using one envelope, with the actual field and gates unchanged; unvalidated.
Resource snapshot39GiB available/one compilerRSS1.7GiB, one owned heavy job only.


Current distance-band scalar adjustment restores the original3 reference errors
and resolves initial seeds622/2331 under unchanged8192 limits; strict edge checks
retain feature transitions. This corrects a regression, not a demonstrated new
whole-field speedup.622 independent references remain unconverged through order96;
its small raw indicator is not certified.2331 references agree. MPI1/4 agree exactly
on the3-cell uncertain normalization, still integration_limited. Serial validation
peak processRSS58.4MiB;622 reference extension210.5MiB; zero swaps, no CFD, no
simultaneous heavy jobs. These are validation-process maxima, not whole-mesh cache
or complete lifecycle memory qualification. All rejected/partial evidence retained.


Connected original-edge follow-up (2026-10-17): local closest-feature displacement
plus the existing wall-classification distance cut changes the saved M6 first-layer
edge from65536 exhausted queries to67 completed. Precision alone failed and remains
saved.22 connected3D/8 existing2D controls pass;3 difficult-cell volume estimates
change<=9e-14, with identical cold/warm reuse. Independent edge discrepancy1.69e-9
absolute/1.48e-9 relative is explicitly retained; this is a numerical indicator, not
certified1e-9 accuracy or complete adaptation throughput. A single unchanged-budget
MPI4 construction-only M6 replay is active in construction_local_geometry_v1; no
CFD step or solution transfer. Complete mass/global uncertainty/production coupling
and the broader performance package remain open. Details: GOAL3_LOCAL_FEATURE_PRECISION.md.


Current shared-edge correction completes the diagnostic above: classification
cuts alone still failed the second actual wall edge. Interior Gauss7/Kronrod15
addresses rounded cut-endpoint contamination while preserving the global error/
work/depth/mesh gates. Keeping old endpoint brackets first caused23787 compositions
versus an existing10000 efficiency ceiling; that regression is preserved and fixed
by deleting obsolete panels, without raising the ceiling. Final actual edges use
45/75 compositions versus65536 exhausted, independent numerical differences
9.58e-12/2.28e-12.29 affected3D/8 existing2D controls pass; original3-cell scalar
results/cold-warm consistency remain. This is a measured local integration remedy,
NOT donor-index reuse/shared-edge caching/transaction batching or a lifecycle
speedup. One same-field/config MPI4 construction-only attempt is active in
construction_interior_edge_v1; normalization, accepted mesh and transfer remain
unestablished. Detailed outcome/negative sources: GOAL3_LOCAL_FEATURE_PRECISION.md.


The final MPI4 construction-only attempt is TERMINAL REJECTED by the unchanged
300s elapsed limit during first-row initialization, after447 INTERNAL publications/
1788 work. It advances past both known edge failures, but no complete accepted
mesh/normalization/transfer follows; the refinement-integration hypothesis remains
inconclusive. Do not confuse private progress with accepted adaptation. No further
run or budget increase. Rejected-run timing313.21s total/300.65s backend; rank2
geometry preparation147.86s versus9.37–11.88s on others, despite fewer composition
queries. Nested rank timings/cost heterogeneity identify investigation targets,
not a lifecycle/scaling gain. Peak rank~725MiB; sum rank peaks2.71GiB upper bound,
zero swaps; host~40GiB available. Stage/source/resource evidence is in
construction_interior_edge_v1. Primary global uncertainty/normalization and all
broader performance proposals remain open.


## Global uncertainty and budget-feasibility follow-up — 2026-10-17

Finite conservative coarse-support envelopes FAIL as useful stopping estimates:
14 unresolved cells have error-envelope sum1.093M versus complete target100000;
an anisotropic/logarithmic refinement improves only0.57%. Preserved17-cell
results, not discarded contributions. Supporting-plane lower cones are INCONCLUSIVE:
348 faces/lower2525 cannot establish infeasibility. Neither is a production policy.

A smaller direct-support experiment WORKS as numerical feasibility diagnosis:
two disjoint-original-wall-cell sampling seeds give384179/382098, conditional
one-sided lower estimates364473/362392 (delta0.005 each, at least99% simultaneous
under ideal IID/PDF/field assumptions). No machine-certified bound. All16384 query
rows agree on1/4 MPI ranks. Native query time.713/.799s serial/.936s MPI4;
preparation3.83/4.72s. This lower subset already exceeds100000 by262.39% or more;
refinement of the same immutable field cannot reduce its continuum demand to that
target. Integration accuracy and construction rejection remain separate issues.
No full composed integral, accepted M6, production root/global reducer or
higher feasible budget established. Existing out-of-range policy is unchanged.
The general sensor-integral plus full-support BL-correction sampler remains
UNIMPLEMENTED; proposal weights alone are not that capability.

Peak preparation process301MiB, query process46–51MiB, incremental build2.35GiB;
one job/thread/build worker at nice19, no swaps. These are maximum processes,
not aggregate MPI/whole lifecycle peaks. No CFD rerun or broad regressions for
this fixture-only extension. Broader donor/edge/transaction/transfer/balance items
remain pending. Details: [support bounds and feasibility](GOAL3_COMPLEXITY_SUPPORT_BOUNDS.md).


One-sided global uncertainty follow-up: WORKED in the shared experimental
controller and actual MPI1/2/4 lower-contribution provider.35 assertions cover
finite/unknown uncertainty, inconsistent lower inputs and ambiguity; real M6
100000 target is outside the diagnostic bracket in4 bound-only checks,0 additional
queries. Remaining positive mass is explicitly unknown, not dropped. One-rank bad
query rejects collectively. Confidence still conditional on the provider; production
normalization/stage integration pending, no accepted mesh or root claim.

Cheap complete-field trial measurement: WORKED as a bounded baseline, FAILED as
an accuracy/coverage criterion. All303359 cells/6.674M points complete in26.24s MPI4;
sensor Q18 matches independent audit2.22e-16. Composed Q4/Q18=681173/780726 and
absolute-cell-difference sum110684; no known global error.329's39.35% reference
error is about46210x its rule indicator. Reusing actual geometric samples for the
separate coarsest-sensor comparison avoids a second geometry pass; not a production
cache or complete root speedup. Peak process audit46.4MiB/preparation279.4MiB/
build2.37GiB, zero swaps;113.4MiB reproducible export removed after validation.
No expensive references or CFD repeated. See GOAL3_COMPLEXITY_SUPPORT_BOUNDS.md.
