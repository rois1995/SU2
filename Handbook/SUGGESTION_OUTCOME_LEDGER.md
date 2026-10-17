# Suggestion outcomes — working ledger, final report pending

Requested2026-10-16: final report must map EVERY suggestion to worked/failed/
inconclusive/not attempted, quantify gains and costs, and explain negative results.
Do not count an unattempted item as a failure or a local replay gain as a complete
adaptation improvement. Include saved/rejected evidence, source/commit provenance,
accuracy, work, budget discrepancy, memory/communication and remaining scope.

| Suggestion | Current status | Evidence and limits |
|---|---|---|
| Match normalization to integration accuracy | Implemented, qualified controlled3D scope | Bounded uncertainty-aware production search; original-family correction uses at most24 trials/2 frozen stages. Eight planar RANS workflows independently finish0.194--0.323% below budget, integration_limited. No exact root, calibrated global tolerance or normalized M6 claim. [Current scope](GOAL3_FROZEN_NORMALIZATION_STAGES.md). |
| Allocate absolute error globally without gathering cells | Partial; general allocation/calibration still open | Global reductions and3-cell absolute-error prioritization qualify their replay scope. Production uses a global sensor interval plus BL-correction indicator; heuristic3max SEM can miss rare heavy mass. Full error-priority allocation is not implemented/calibrated. [Production](GOAL3_RUNTIME_COMPLEXITY.md). |
| Reuse integration geometry across scale trials | Implemented, scoped gains measured | Production retains original publisher/barycentric/wall geometry across current scale trials. Controlled correction retains8192 preparations with zero donor/wall queries DURING root trials (49152--57344 density evaluations). Preparation cost and memory remain; no full-event gain claim. |
| Integrate BL correction separately | Implemented for nonplanar faceted3D | Same-config M6 metric initialization415.97->16.47s, identical coarsest estimate790200.50 +/-27958.45 heuristic against100000. Complete geometric face/edge/vertex proposal; no discarded unresolved BL mass. This is a contended metric-phase result, not whole remeshing speedup. [Evidence](GOAL3_RUNTIME_COMPLEXITY.md). |
| More integration subcells | Negative evidence | 329 accuracy is nonmonotone;88 work/time rises. No subdivision/count ladder accepted. Actual-refinement reassessment below is a different experiment. |
| Cheap scalar geometric-root screening | Withdrawn: no useful gain | Seed estimates unchanged on88/157/329;88 .2003->.2090s. Source/results/decision retained in seed_screen_v1; production restored. |
| Shared policy in2D | Foundations retained; shared correction not integrated | Existing2D composition/geometry rules remain.30P30N input prepared, not adapted.3D sampling/correction does not establish2D robustness or cross-dimensional error calibration. |
| Actual refinement should ease integration | Worked in controlled RANS; M6 remains open | Same corrected ORIGINAL field on actual refined partitions lowers indicator3.07--4.07x and observed error5.48--77.70x. Coarsening ratios.915--1.206 show no monotonic guarantee. No adapted-node refit. [Evidence](GOAL3_FROZEN_NORMALIZATION_STAGES.md). |
| Reuse donor support/indexes across repair campaigns | Related point-query reuse implemented; full campaigns open | Canonical source-owner samples replace repeated point-only whole-star imports/secondary indexes in weights/return; bounded2048 sensor memo, actual BL still queried. New certified original-parent lookup also qualifies first-row/measurement support; it does not retain whole repair-campaign donor support. [Scope](GOAL3_OWNER_POINT_SAMPLES.md). |
| Extend shared-edge integration reuse | Implemented for frozen identities/endpoints | Existing edge maps now reuse full composed integrals across certified imports only under identical field/geometry/accuracy policy and exact endpoints. Saved M6 MPI1/4 values match cold exactly. No general lifetime or lifecycle speedup claim. [Evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/composed_edge_reuse_v1/README.md). |
| Batch independent mesh transactions | Implemented for fixed-skin first-row interiors | Prior64-parent stage drops12.7%/23.2% at2/4 ranks from group-local coordination; later exact shape ranking cuts metric work~42%. These overlap and must not be added. Direct-cone initialization was withdrawn after a complete cube failure; no production geometry change retained. Displaced general transactions remain open. [Scope](POST_COMPLEXITY_QUERY_TRANSACTIONS.md). |
| Avoid unchanged failed searches | No new extension in this follow-up | Existing dependency-triggered behavior retained. Further work must distinguish geometry rejection, insufficient source support, work exhaustion and temporary conflicts. |
| Reuse transfer geometry across fields/histories | Existing facilities retained; additional optimization not attempted | The new correction stages transfer once after final acceptance and share one original source for reassessment/return. No additional overlap-weight sharing campaign or transfer performance result. |
| Measured-cost partitioning/mid-remeshing migration | Not attempted | Migration/index rebuild/return/solver rebuilding and useful work must enter break-even. Historical2D improved imbalance could still increase elapsed time. No new balancing claim. |

Current table reconciled2026-10-17 against the linked published receipts; subsequent
paragraphs are historical checkpoints. The [initial table](SUGGESTION_INITIAL_SNAPSHOT_2026_10_16.md)
is retained. Full Goal3, accepted M6 construction and full lifecycle affordability
remain open; this is not the final completed-goal report.
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


## Sensor integral plus full-support correction — bounded replay outcome

The suggested decomposition WORKS as a global experimental estimator of the
unchanged original field. Two independent8192-sample seeds give961498/961691
including sensor152994; pooled empirical SE6656, not a proven error bound. Saved
whole-volume Q18=780726 is18.81% below their mean. Independent planar face/edge/
corner/fade integral and point density pass MPI1/2/4; M6 original-P1 densities
agree<=7.22e-15 against independent arithmetic and saved rows match MPI2/4.
This is no production tolerance/root/accepted-grid qualification.

It DOES NOT WORK efficiently as a per-tiny-cell estimator:16384 samples per saved
88/157/329 yield only22/16/11 source hits and38.42%/2.03%/24.79% reference errors.
All indicators flag sparsity; broad3SE covers these observed errors, not a tail
certificate. No sample-limit escalation, expensive reference ladder or CFD rerun.

The expected query savings do NOT yet establish elapsed-time gains. First M6
preparation23.17s (PDF16.86s/static5.31s), native2.74s including index2.03s; second
native2.97s. This broadly matches the earlier complete cheap volume audit cost,
while addressing missed-support risk differently. Reusable geometry/query PDFs
exist in immutable replay files; production scale/campaign index retention remains
unimplemented. Point routing reuses the existing donor service; new complete
outside probes preserve strict existing-import behavior. This is not delivery of
the broader1–3 repair-campaign/cache/transaction performance package.

Rejected inverse-endpoint and coplanar-sector drafts are archived. Stable common
edge-plane/half-angle frames pass the independent square tube check. Legacy
`correction.py` remains unchanged. One heavy job at a time, nice19/thread1/MPI<=4;
replay peak process383–667MiB/build2.35GiB, zero swaps, host~40GiB available.
No aggregate-memory/scaling or lifecycle claim. Detailed method and all receipts:
[full-support correction](GOAL3_SUPPORT_CORRECTION.md).


## Retention across normalization trials — follow-up result

WORKED in the complete frozen-family replay: original donor locations and actual
wall samples are retained, then the same original field is recomposed for trial
scales. Three M6 trials take0.0073–0.0156s AFTER cold preparation, with zero donor/
wall searches during trials. Both independent8192-sample seeds support target1M
at the same factor1.148698355 under explicit experimental5%; MPI2/4 agree. Asking
1e-6 returns integration_limited, with the repeated same-scale result reused and
no extra compositions.100000 is outside the diagnostic1/64..64 family bracket.
No production hard bounds/gradation or stage coupling is implied.

Exact direct/cached tensor checks pass every planar/M6 retained query at the
unscaled preparation field. Scalar
old/new density differences reach7.736e-10 relative under strong anisotropy, but
mean absolute integral drift1.412e-7 versus correction808697 and SE9414. Failed
unit-scale/64eps verifier assumptions and all partial receipts remain; field gates
unchanged. Independent planar integration covers every trial, selected true target
error0.577%.8 planned checks plus35 existing controller assertions pass.

Still NOT a complete-lifecycle gain: cold routing/index/identity audit and the
separately saved~23s proposal/PDF cost remain. No donor-campaign retention/shared-
edge reuse/transaction batching in remeshing itself. Native test provider only;
actual production clipping/regradation, global work policy, collective control/I/O
fault paths and normalization/adaptation/reassessment stages remain to be connected.
No CFD or accepted M6 grid. Final wrapper49.41s/peak process667MiB, zero swaps.
Large input restoration is now reproducible without modifying pinned manifests;
7.22s round-trip check, changed originals rejected, temporary export removed.
Details: [retained normalization](GOAL3_RETAINED_NORMALIZATION.md).


## C++ support preparation and actual owner routing — connected follow-up

WORKED as production-library integration infrastructure: complete faceted support
proposal/PDF now runs in C++ over the existing ADT, with strict resource refusal,
overlap sums and stable coplanar frames. At all8192 M6 seed2641 points its PDF agrees
with the independent Python/KD-tree result to3.636e-12. Native preparation/PDF
0.489/4.678s versus23.70s independent Python preparation/PDF on the SAME points.
This improves a measured kernel; wall model build/output/cold donor preparation,
actual normalization callbacks and all remeshing/transfer costs remain outside
that comparison. No full-event speedup or default uncertainty tolerance claim.

Actual source owners now accompany existing admitted donor responses. The final
vote is reused, not another collective per import. A shifted(id+1)%ranks storage
layout passes exact query rows/normalization on2/4 ranks, with original RAW9 values
and bitwise direct/cached baseline tensors preserved. Extra wire integer/result
copy is an unmeasured whole-remeshing overhead, not free work.4 affected donor/
regional cases pass1/4 ranks;21 serial provider/composition/controller cases pass.

Two native RNG seeds give correction801696/816508 (empirical SE~9420); mean total
using backgroundQ18 is962097. Normalization uses the wider analytic background
interval explicitly; at the same selected scale target1M gives985772/1000965,
heuristic uncertainty~35660, meeting the experimental5% test. No rigorous bound.
Root output-open failure and inconsistent MPI controls reject collectively; no
fault injection for close-time disk-full errors. Geometry/metric/mesh/transfer
gates are unchanged. All evidence and resource receipts retained; no CFD rerun.

STILL OPEN: wire this provider to actual CSolver bound/gradation callbacks and
bounded frozen adaptation/reassessment stages. Broader remeshing campaign caches,
shared-edge integrals and transaction batches remain unimplemented. Build peak
2.38GiB process/replay667MiB, recorded swaps0, one owned heavy job throughout.
[Implementation and exact scope](GOAL3_NATIVE_SUPPORT_PROVIDER.md).

## 2026-10-17: actual trial-field complexity and M6 initialization

Runtime CSolver is now connected to bounded global sensor+BL-correction estimation
with retained original node/weight/wall-geometry stencils. Changed/clipped fields,
changed cores, reversion and MPI2/4 whole M6 checks pass. The real RANS/SA initializer
now completes; at the coarsest permitted sensor it estimates790200 +/-27958 against
100000, explicitly provisional/outside estimated bounds. Same-field refinement
cannot remove that continuum demand. No accepted new mesh/transfer is demonstrated.

What worked: sensor+BL correction, geometry reuse across actual trial fields and
uncertainty-aware stopping (no fictitious1e-6 root). Planar support error0.0135%
against independent coarea; actual planar solver0.05% gate retained. What failed:
8192/65536 extra query cutoffs replaced accurate planar integration with1.16% noisy
sampling; both rejected, existing point/panel/depth limits retained. Rotated sensor
conditioning needs an explicit heuristic numerical indicator, not rejection from
an inverted floating interval. Q18 vertex-order invariance was a verifier mistake;
an independent matching rule agrees within4e-16 and native results were reused.

Performance remains poor:415.98s metric initialization,230.56s strict work before
fallback and168.89s grading7 trials. Bounded prep5.83s/all assessments1.38s. Next
large step is early coarsest-field feasibility plus geometry-based selection;
post-refinement same-original-field correction epochs remain unimplemented.
Repair-campaign donor retention/shared-edge lifetime/batched transactions are not
implemented by this normalization cache. Detailed evidence/resources/rejections:
[Runtime complexity](GOAL3_RUNTIME_COMPLEXITY.md).


## 2026-10-17: eliminate repeated work before M6 remeshing

WORKED for the actual outside-budget M6 initializer: test the coarsest permitted
sensor BEFORE graded scale trials, and use nonplanar geometry to select bounded
sensor+BL-correction integration before the known-expensive strict search. Metric
415.97->16.47s (25.25x elapsed), summed rank CPU1480.46->48.18s (30.73x),7->1 trials,
1218->1 sweeps/51.9M->61330 visits; strict builds178->0. Same estimate790200.4998,
heuristic uncertainty27958.4546 and byte-identical CFD restart; budget100000 remains
outside estimated bounds. Preserves same original sensor/clipping/gradation/core,
actual-query BL/finer demands/fade and every mesh/transfer gate. No silent mass loss.

Accuracy DID NOT improve: same heuristic3.54% indicator, not a proven bound or
calibrated tolerance. Initialization is no longer blocked, but no accepted mesh or
post-refinement correction stage is demonstrated. Exact planar0.05% gate, reachable
budget, original field/clipped trial/reversion and MPI2/4 checks pass. Coarsest
preflight adds work for reachable targets; no general speedup claim for those.
Cold preparation5.83->7.69s DID NOT improve in this contended comparison and is now
the dominant assembly cost. Volume caching removes repeated geometry determinants
but adds1.23MB max-rank payload; no isolated timing benefit is established.

Resources: one owned heavy job/build-j1/nice19/thread1, host38–40GiB available;
build peak process2.37GiB/pilot577.72MiB, not aggregate RAM. Foreign CFD untouched.
Broader repair-campaign donor retention/shared-edge lifetime/batched transactions
remain NOT implemented. [Exact outcomes and receipts](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/production_complexity_geometry_v1/README.md).


## 2026-10-17: whole-donor preparation retention did NOT work

The bounded support/index retention experiment was correct on the SAME512 original
M6 cells1/2/4 ranks: all2560 per-event point coordinates/sensor/composed tensors and
weights exactly match fresh imports; weights match across ranks. Curved runtime
and initialized RANS/SA N4/M2 BL construction/removal/transfer/restart pass independent
audits. No physical constraint or frozen target changed.

Performance FAILED: AABB retention0.156/0.189/0.183s versus fresh reference0.085/
0.112/0.080s; max imports189/189/146 versus125/125/95, builds122/122/123 versus124/
124/123 and ZERO useful index reuses. Exact closed-point filtering still needs189
imports/122 builds and takes0.557s versus0.154s in its serial pilot.51–65 support
resets overwhelm the tiny reuse. AABB false positives alone are not the explanation;
whole incident donor stars are a poor reuse unit for these point batches.

Both production cache variants WITHDRAWN; no repeated broad exact-variant campaign
and no bigger cap/reset ladder. Retain the test-only per-query M6 audit and index-
build counter. Times are contended selected-cell kernel observations; cold reference
omits production admission bookkeeping. No full-event performance claim. Source
capsules, failure/config diagnostics, raw receipts and accepted scoped test grids
preserved. Restored source/current binaries PASS serial/MPI4 bridge, full query/serial-reference
comparison and independent N4/M2 RANS/BDF/SU2+CGNS/restart audit. At least1594/2560
queries repeat, with<=966 distinct positions fitting2048 samples: a measured
opportunity for sample/stencil lookup, not an implemented optimization.
[Detailed outcome and next design implication](GOAL3_POINT_PREPARATION_REUSE.md).


## 2026-10-17: canonical owner point samples WORKED in the measured scope

Replace point-only whole-star imports/second indexes with direct lookup in the
existing original-owner ADTs. Return canonical sensor-only samples and use one
bounded2048-entry memo across weights/mesh-return batches. This replaces that
path's former per-batch cache/index. Actual geometric BL composition still runs
on every query, including hits; source keys/publishers/extensions/finer/fade and
mesh/transfer gates unchanged. Integer mantissa/exponent metadata preserves native
barycentric precision through the existing fixed integer/double codec.

Same512 M6 cells on1/2/4 ranks: exact query coordinates/sensor/composed tensors and
weights match fresh imports and saved serial weights.966/966/969 source lookups
instead of2560, no second indexes instead of124/124/123; measured preparation
.03337/.03086/.02960s versus.12975/.11124/.08883s (3.0–3.9x). Larger4096-cell MPI4
replay: ALL20480 exact queries,8098 lookups/12382 hits,0 versus1852 builds,
.26134 versus2.28299s (8.74x); at least4002 FIFO evictions exercised. Cold reference
omits production admission bookkeeping and these are contended selected-cell kernel
observations, NOT full M6 lifecycle/scaling/accepted-wing speedups.

18 serial cases/2941 assertions and MPI4 controls pass, including genuine288-star
sampling without changing the256 full-import cap, extension/physical priority,
shifted source publishers, failure/storage and precision. Curved runtime return
and independent initialized RANS/SA N4/M2 BL construction/removal/BDF/SA/geometry/
full field/restart/SU2+CGNS pass24->6 cells. No new CFD solve or accepted M6 mesh.
Whole-donor caches remain rejected; shared-edge lifetime/independent publications
and reconstruction-campaign support remain unimplemented. One owned heavy job,
38–39GiB host available; resources/provenance/compile rejection preserved.
[Exact comparison and remaining work](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/owner_point_samples_v1/README.md).


## 2026-10-17: extend shared composed-edge reuse WORKED in the measured scope

The existing scalar maps skipped reuse for every geometric BL field. They now
require one explicit immutable full-field identity, exact edge IDs/endpoints and
finite positive successful measurements. Fresh default identities prevent reuse
across changed targets; native imports share one identity per engine invocation.
The identity owns its lifetime, is allocated under elected/admitted initialization,
and is not migrated/persisted. First-row alternatives retain the last bounded
candidate map. No overlapping edge cache or integration/mesh tolerance change.

Original M6 wall32, MPI1/4: all cold/shared/repeated q/J/max/full edge lengths match
exactly. First shared pass192->137 integrations and62897->48182 compositions;
serial elapsed.61096->.44427s (27.3%), MPI max.87581->.55143s (37.0%). Repeated pass
reuses all192, retaining32 actual centroid compositions. All32 requesters are on
rank0; other ranks serve the matching protocol. These are contended selected-cell
observations including imports, not multi-requester scaling or full-event speedups.

73 serial cases/6234 assertions and selected MPI first-row/disjoint/coupled controls
pass; affected curved return and independent initialized RANS/SA N4M2 construction/
removal/BDF/SA/SU2+CGNS/restart remain accepted24->6 cells under unchanged gates.
No accepted M6 mesh or new production first-row speedup certificate. Initial build
plus one safety refresh are saved; no repeated native test campaign per small edit.
One heavy job, peak build process2.38GiB/serial pilot1.63GiB, host38–40GiB available.

Campaign donor support, scoped reconstruction edge lifetime and independent
publications are still open, as are original-field uncertainty/reassessment and
complete lifecycle affordability. Existing2D reuse must be inspected before sharing
this policy; no2D or MMG path was replaced.
[Exact evidence and limitations](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/composed_edge_reuse_v1/README.md).


## 2026-10-17: independent first-row transactions WORKED after removing inner synchronization

Existing complete-star/fetch/publication infrastructure now batches up to8 one-cell
interior replacements. ReserveInteriors arbitrates duplicate cells while allowing
shared read-only vertices; exact SameSkin/height/prescribed-interface checks and
one atomic publication protect topology and accepted state. No general movement/
boundary reservation was weakened. Bounded shared donor imports split only at the
existing cap, and per-candidate local deadlines precede the common group vote.

The first version cut calls but still synchronized after every private candidate:
MPI2 first-row6.8685->6.8809s, no elapsed benefit. Executed source/outputs preserved.
Group-local scheduling then preserved exact M6 batch1/8 geometry/markers and rational
positivity/volume/outer skin, with the complete original303359-donor field/wall.
64 private initializations, MPI2/4: first-row6.9331->6.0494s /4.6660->3.5846s;
wrapper calls1326->234 /670->124, directory elections935->123 /471->65. Metric
rank-work remains~9.6–10.0 rank-seconds; local coordination improves elapsed time,
not tensor arithmetic. One-shot contended stages, NOT complete adaptation/scaling.

Donor packing has real cost: MPI2 attempts32->12 with4 retries; MPI4 16->6 with2.
No larger caps or geometric retry ladder. The old full-driver64-work baseline
hit180s and provides no completed-stage timing. The separate direct MPI2 rank-config
failure was corrected without changing the field; exact failed setup remains.

9 current serial controls/723 assertions, MPI4 reservation/first-row/general batch
checks, affected curved return and independent initialized RANS/SA N4M2 current+BDF/
SA/restart/SU2+CGNS workflow pass. No accepted M6 mesh; partial VTUs are explicitly
_rejected. No final full-target quality/length or transfer claim for those subsets.
One heavy job/build-j1/thread1/MPI<=4, max build process2.40GiB, host39–40GiB available.
Wider campaign support, general displaced-transaction batching, original-field
uncertainty/reassessment/correction, viscous accuracy and lifecycle affordability
remain open. See [full evidence, failures and limits](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/first_row_transactions_v1/README.md).


## 2026-10-17: avoid unselectable first-row edge work WORKED in the measured scope

The existing initializer maximizes normalized minimum q/J, then minimizes full
maximum edge length on exact shape ties. One shared scalar shape-ranking path now
screens strictly worse candidates before expensive edges. Equal/better candidates
retain full measurement and comparison. No approximate screen, new candidate,
target adjustment, mesh tolerance or partial acceptance certificate is introduced.
Actual centroid composition still occurs, original donors/finer/fade remain frozen,
and final geometry/height/positivity/q/J/full-edge/transfer gates stay unchanged.

Same full original M6 field, 64 private initializations, MPI2/4 batch1/8: exact
selected geometry/physical bindings/rational positive volume/outer skin equal the
previous constructor.181 of256 candidates prune, fresh integrations1,856->951.
At batch8, metric rank-work9.555->5.455 /9.983->5.778s; first-row6.049->3.433 /
3.585->2.077s. Single contended observations, not scaling or whole-event gains.
Cache hits fall because whole measurements disappear, not because reuse broke.

67 serial controls/5,208 assertions plus affected MPI4 controls, curved return and
independent RANS/SA N4M2 current+BDF/SA/restart/SU2+CGNS checks pass. The first
serial pilot rejected an obsolete expectation that this fixture must hit its edge
cache; the screen now avoids those later measurements. Failed executed source/logs
are retained. Only that test expectation changed; separate edge-reuse tests remain.
One owned heavy job/build-j1/thread1; max build process2.40GiB, host39–40GiB available,
zero recorded GNU-time swaps. Process-tree memory snapshot and receipts preserved.

This completes the measured first-row query/publication follow-up, not general
campaign support or displaced batching. No accepted M6 mesh, original-field global
uncertainty calibration/reassessment/correction, viscous accuracy or lifecycle
qualification. Resume those capability gaps before another cache/local fallback.
No2D/MMG path changed. [Exact results, failure and limits](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/first_row_shape_ranking_v1/README.md).


## 2026-10-17: independent global holdouts INFORMED error control; no policy enabled

The existing complete-support replay was sufficient: no C++ changes/build, CFD or
remesher runs. Two32,768-point streams of the same complete original RAW9 M6 field
retain outside zeros and unchanged geometric BL. The old8192-point prefix is
reproduced; disjoint holdout differences0.186/1.364 combined empirical SE. Every
first-stream query/donor/sensor/composed/correction/weight row agrees exactly on
MPI2/4; independent source-density/exact containment and declared PDF-subset audits
pass. These checks establish arithmetic/protocol scope, not whole-domain coverage.

Total estimates942477.38/953730.50 differ by1.694 combined SE. Pooling65,536 gives
948103.94 with empirical indicator15887.91 (1.676%), but eight-block SEM5128.60 is
1.54x the pointwise3320.73. Few blocks cannot calibrate a safety factor or detect
unseen tails. Larger streams cut pointwise SE as expected; they do NOT solve root
accuracy or justify a default tolerance. Background halfwidth5925.71 remains even
with zero correction sampling error. This original field is distinct from the
production coarsest trial790200.5; do not mix their budgets or sensor demands.

Planar sampled error0.132686% lies within3SE but exceeds the retained strict0.05%
gate. Thus the sampled estimator has NOT earned replacement of strict planar/2D
behavior. Earlier difficult-cell underestimation/failures remain preserved. Global
error-directed allocation, refined-original-field reassessment and frozen correction
epochs still need implementation/qualification; this calibration does not supply them.

Per M6 stream~59.3–59.5M indexed PDF candidates stay below80M work. Preparation
~0.3s/PDF~7.1s, MPI2 native original-source/density work~2.2s. One owned heavy job,
nice19/thread1/MPI<=4; pilot21.76s plus resumed campaign44.40s, peak process667MiB,
host40–41GiB available/no observed swap growth.21MiB evidence; original export shared.
No sample-count/subdivision ladder follows. About71% observed variance is within
face proposals, motivating a fixed-work reflected-pair experiment with correct MPI
pair covariance and a planar control where pairing could be worse. Not implemented
or adopted. [Exact observations and limitations](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/global_uncertainty_calibration_v1/README.md).


## 2026-10-17: fixed-budget reflected face pairs MIXED; production experiment WITHDRAWN

Existing importance sampling can pair a face point with its reflected normal
coordinate while retaining both complete marginal PDFs. Edge/vertex duplicates
preserve their distribution. At8,192 actual points/4,096 independent pairs, M6 SEM
9377->7901 /9417->8017 (15–16% lower) on two streams; holdout discrepancies2.330/
0.835 combined SE. This is empirical variance, not proven improved accuracy. The
embedded planar control has positive covariance: SEM.03260->.04711 (45% worse),
actual relative error0.304% against coarea, above0.05% reference criterion. Pairing
cannot be a general default, and it leaves the separate background interval floor.

Pair means, not points, define independent units. The native scalar replay's IID
SEM was explicitly invalid and unused for normalization/acceptance. Offline global
pair/source/density/weight results match exactly MPI2/4. All paired PDFs pass the
independent oracle; every anchor matches its original draw.25 focused serial cases/
13,570 assertions plus affected MPI4 dynamic controls pass. Default32,768-point
M6 streams remain bit-identical on both seeds. No CFD or remesher run.

Do not add a covariance-selection pilot/API merely to rescue modest/conditional
sampling gains. All three experimental C++ changes are WITHDRAWN. Executed source
patch/capsule and complete provenance/results remain; restored production/test
binary and every pre-experiment source/build/binary pin match exactly. Baseline
checks reused without another regression. Independent sampler/tolerances/mesh
gates unchanged. One owned job/build-j1/thread1/MPI<=4; max build process2.40GiB,
replay667MiB, host40–41GiB available, no observed swap growth. Experimental build
1m39s/restoration1m38s; pilot39.35s/resumed campaign64.33s.

Stop the sampler follow-up. The next substantial capability is actual candidate-
partition reassessment of the immutable original field and bounded frozen correction
stages before publication. The actual-refinement hypothesis remains untested;
negative pairing is not evidence against it. Full Goal3/M6 acceptance/global error/
viscous/full-event gates remain. [Source, work, decision and restoration proof](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/face_pair_variance_v1/README.md).

## 2026-10-17: actual candidate/original-field reassessment IMPLEMENTED; correction still open

Before native3D publication, a bounded candidate-cell proposal queries the immutable
original sensor-only P1 and actual BL at every centroid/sample. It never refits the
field from candidate tensors. One original-source index serves reassessment and
reader return. A separate phase/result reports global mass, point/eight-block SEM,
heuristic3*max indicator, work/storage/time limits and unresolved states. Sampling
probability/resource limits do not discard cells; invalid fields still reject.
No target, geometry/h0/positive/q/J/full-edge/transfer gate changes or scale update.

Actual accepted planar RANS SA N4/M2,6->24 tetrahedra at8192 samples: indicator
.165326->.0155528 (10.63x lower), error .7910%->.05619% (14.08x lower); independent
reference3.6941577302, estimate3.6962333832. This supports actual-refinement
reassessment in that scope. Phase.02094s; return does not rebuild its original
index. Fixture budget1 remains outside the constrained mass; not normalized success.
Smooth integration-only6->384-cell control improves indicator4.30x, separately.

Frozen complete M6 RAW9 coarse centroid596350.33 vs sampled979024.48 +/-101701.02
HEURISTIC shows missed BL mass and poor coarse weighting. The existing independent
support-stream pooled estimate948103.94 is consistent but is not an exact reference;
equal-budget geometric-support sampling has much lower variance. Keep existing
initialization; no accepted/finer M6 field or general thin/curved/interacting BL
qualification. MPI1/2/4 clouds/global controls, exact M6 MPI2/4 rows, original P1/
PDF/exact-containment audits, affected curved/reader and independent RANS full-field/
BDF/SA/restart/SU2+CGNS pass. No full regression or CFD rerun.

Initial compile typos, a rejected128-point call above the existing64-point source
cap, and report-only NumPy Boolean serialization failure are retained; caller now
uses MaximumCavityCells. Runtime checks were reused with explicit executed-helper
archive/report transition after the report failure. One heavy job/nice19/thread1/
build-j1/MPI<=4, peak process2.41GiB build/667MiB replay; host~39–41GiB available,
no observed swap growth. Not aggregate job peaks or scaling claims.

This is actual-field reassessment, not the whole uncertainty/correction strategy.
Global error calibration/allocation, frozen correction epochs from original
unclipped frames/bounds/gradation, difficult accepted M6 construction, viscous
accuracy and whole-lifecycle affordability remain open. Do not rescue the coarse
proposal with another sampler/count/subdivision ladder. [Evidence and limitations](GOAL3_CANDIDATE_REASSESSMENT.md).

## 2026-10-17: frozen original-family correction IMPLEMENTED in controlled3D workflows

The actual candidate reassessment is now coupled to at most2 private mesh stages
and1 final transfer/publication. CSolver retains ORIGINAL UNCLIPPED eigenframes,
original CFD geometry/nodes and the bounds/gradation family; replay skips Hessian/
sensor recovery and restores CFD metric/diagnostics/flow/SA/BDF. Trials interpolate
current original-node P1 and reuse geometry/publisher stencils. No clipped-donor
rescaling, adapted-node refit or target change during local mesh operations.
Optional eligibility is global, including prediction changed on only some ranks.

What worked: original nonhomogeneous mass21.76446 ->20.00000 +/-0.04471; independent
19.99506 (.0247% error). MPI1/2/4, source reversion, aliases/empty publishers and
collective control/storage/deadline faults pass.8 actual initialized RANS/SA planar
workflows (N1M1/N2M2/N4M2/N4M4, controlled initial scales1/10) accept151--240 cells,
work433--1374/root6--7 calls. Independent geometry/q/J/full composed edges/height,
exact sensor transport, BDF/SA, SU2/CGNS and real restart pass unchanged gates.
Independent final mass19.93549--19.96127 against20:0.194--0.323% discrepancy; empirical
indicators retained, every root integration_limited, no exact normalization claim.

The SAME corrected original field on actual pre/post partitions gives refinement
indicator3.07--4.07x lower and observed error5.48--77.70x lower. Coarsening indicator
ratios.915--1.206 show no monotonic benefit; error ratios.824--.997. This is evidence
for reassessment, not for blindly increasing subdivisions or sampling complexity.
Serial root phase.035--.040s/49152--57344 density evaluations,8192 geometric
preparations; donor/wall queries DURING trials0. Preparation and both mesh stages
remain costs. No full-event speedup/scaling claim on these small contended controls.

What failed: a work-cap fixture used a newly parsed config without mesh-bound
marker order, after a native-complete240-cell candidate. Correct tag/kind/send-receive
bindings fixed the fixture; the77-work cap then rejects without changing accepted
CFD/reference. A reporting wrapper Path-type error occurred before any MPI run.
Both failures/source capsules remain. Exact sealed serial results/audit are reused,
not rerun for MPI reporting. Build-j1/nice19/thread1/MPI<=4, one heavy job. Build
peak process2.40GiB, pilot~50MiB, host~38--41GiB available/swap unchanged.

Not established: natural M6 normalization/construction success, general curved/
thin/interacting correction, calibrated global error/default tolerance/allocation,
2D/predicted-family coupling, viscous accuracy or unsteady affordability. Earlier
M6 coarsest demand~790k exceeds100k; refining integration cannot remove that demand.
No unchanged300s retry, CFD advancement or cluster run. Broader repair-campaign
retention/edge lifetime/transaction/balance package remains separate; this is
normalization-stencil reuse, not completion of every performance suggestion.
[Complete result and evidence](GOAL3_FROZEN_NORMALIZATION_STAGES.md).

## 2026-10-17: direct initialization WITHDRAWN; canonical parent support under final attribution

A direct four-child cone initialized all38,800 M6 parents in3.74s, but all116,400
transition children still fail final shape criteria and the ordinary complete cube
stalled after20 edits/1176 work. Constructor/API/default changes are WITHDRAWN;
source/patch/probes/failed control remain. No attempt-then-fallback ladder installed.
M6 seeding then exhausted~272s after27,044 cells; no volume edits or accepted mesh.
This is a rejected strategy, not a completed construction improvement.
[Negative result](GOAL3_M6_DIRECT_INITIALIZATION.md).

Retained source-index centroid probes now certify a complete physical original
parent for old vertices or strict interior vertices; unsupported/alias/shared-face
points keep full imports. First-row cones/ranking stay unchanged. Initial exact
M6 comparison found an existing singleton/general endpoint-weight mismatch: sensor
relative2.16e-16, amplified target component relative4.27e-13. Existing exact segment
kernel now supplies both; no target, fade, finer-demand or tolerance changes.
Canonical219,532 actual sensor+composed-query rows and legacy geometry match exactly.
70 serial cases/14,417 assertions, MPI/subset/curved/reader and independent RANS/SA/
BDF/CGNS/restart pass. Generic finer-sensor4096-work failure still needs unchanged
baseline attribution; do not label the combined campaign passing.

Changed complete M6 run makes10,080 PRIVATE parent replacements at300s, still before
seeding/volume edits. Source import~.09s/rank; geometric edge preparation reaches192s
on rank2 and is the next hotspot. No accepted/full-event speedup or M6 hypothesis
success claimed. Original donor P1 and geometric BL remain frozen, not refit from
adapted samples. This is bounded support-query reuse, not all repair-campaign
retention or transaction/balance ideas. Deadline checks now bound seeding/update at
common batch boundaries. One owned job, nice19/build-j1/thread1/MPI<=4; RAM~38--41GiB,
swap unchanged. [Exact proof/evidence/status](GOAL3_ORIGINAL_SUPPORT.md).

Follow-up attribution: the unchanged c70 source paths (plus only the independent
seeding deadline check) fail the stronger generic finer case at the same4096 work,
162 rounds and67 rank0 accepted edits. Initial/rejected finer SU2 files are
byte-identical. This is an existing limitation; neither failed folder is promoted
to passing. Candidate sources were restored for final build verification.

Final source/build/binary restoration is EXACT: both executables, build manifests and
candidate Engine3D/Field3D hashes match the qualified canonical producer; dependency
check has no work. Reuse the70-case serial, MPI,219,532-row exact frozen comparison
and independent RANS/BDF/restart evidence. Current package is qualified only within
that scope; full M6 and the baseline generic-finer limitation remain rejected.
[Complete summary/proof](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/original_parent_support_v1/summary.json).


## 2026-10-17: defer irrelevant first-row ranking lengths — scoped success, M6 still rejected

The strongest-first existing ranking needs full edge length only for exact shape
ties. New selection retains original backgrounds/geometry/height/q/J scoring and
tie order, measuring full lengths only among tied maxima. Selected cells still
undergo mandatory full composed seeding and unchanged final acceptance. No new
point-placement strategy, target/cache/configuration option or scenario mode.

Worked:71 serial cases/14,440 assertions, affected MPI/subset/curved/reader and
actual N4/M2 initialized RANS/SA/current+BDF/conservation/SU2+CGNS/restart pass.
Frozen64-parent M6 replay retains byte-identical legacy geometry;1600 actual query
rows versus219,532 before, all exact old-trace sensor+composed subsets. This removes
unnecessary work; it does not complete every edge-reuse/retention/batching suggestion.

Whole M6: all38,800 first-row parents now initialize in26.793s. Mandatory seed uses
249.789s and measures32,832 of536,159 PRIVATE cells before rejection at301.004s under
the unchanged300s control. Nested seed geometry reaches172.949s on rank2. No volume
edits, accepted mesh or transfer; no lifecycle speedup/feasibility conclusion. The
combined construction campaign FAILS its known stronger finer-sensor control at
4096 work/162 rounds; initial/rejected grids exactly match the unchanged baseline.
No cap ladder or additional constructor fallback. Next work targets mandatory seed
query geometry/repeated edge work while retaining frozen-field/final mesh contracts.

Current source/build/binary/helper pins, source capsules, logs, frozen queries and
baseline grids are verified by a lightweight postprocessor, without restarting MPI.
A repeated buffer-copy reporting draft was stopped and a Python3.8 Path-API reporting
failure corrected; neither required new native runs. One heavy job/nice19/build-j1/
thread1/MPI<=4; host~39--41GiB available, unchanged816MiB swap. No CFD advancement,
cluster submission or full regression per small edit. Full Goal3 remains ACTIVE.
[Evidence, hashes and limits](GOAL3_DEFERRED_FIRST_ROW_LENGTHS.md).


## 2026-10-17: geometric repeated work — rejected copy optimization, scoped support gain

Suggested reducing repeated geometric preparation/query work. Measured two distinct
strategies using the SAME finite-feature legacy kernel and69 actual original M6
edges, including the32 largest candidate boxes in the declared sample.

- Local interval updates/copy avoidance: exact distances/cuts/work counts, but
  legacy/candidate ratios.994/.995 on alternating-order passes. WITHDRAWN; all source,
  patch, replay inputs and resources retained. No native suite/CFD run for this.
- Active-support restriction: existing fade thickness/on-wall/roundoff radius,
  padded residual slabs and a constant inactive background. Actual original-P1+
  geometric-BL queries/finer demands/fade and final contracts are unchanged. About
  11.3% lower sampled geometry time (.640->.568s and.639->.567s), exact sampled active
  distances/crossings. RETAINED within qualified scope, not a full-seed/lifecycle
  speedup. No new cache/configuration or tensor-arithmetic shortcut.

23 serial cases/502,489 assertions, MPI2 envelope/thin-volume, five actual original-
field edge replays and independent N4/M2 RANS/SA/current+BDF/conservation/SU2/CGNS/
restart PASS. Three difficult-cell reference errors:.7682/.09178/.10737%. Cell157
indicator misses error by~1.91x, reconfirming existing global-calibration weakness;
no bound/default tolerance/exact root claim. Controlled final budget discrepancies
~.0960%/.3226%; one accepted mesh changes with the integration/normalization path,
while every remesh-stage field remains frozen. These are two planar workflows,
not new general curved/M6 or all eight prior workflows qualification.

No extra300s M6/CFD/cluster run spent on the11% component result. The next substantial
capability is progressively full-measured remeshing with complete remote cavities
and whole-mesh final certification (DESIGN ONLY). A saved64-parent diagnostic finds
375/448 PRIVATE children below final shape limits; do not extrapolate feasibility.
The broader repair-campaign retention, transaction batching and transfer/balance
package remains incomplete. One heavy job/nice19/build-j1/thread1/MPI<=4, host~39--40GiB,
swap unchanged. [Sources, evidence, limits and next design](GOAL3_ACTIVE_CHORD_SUPPORT.md).


## 2026-10-18: progressive full measurement reaches the M6 volume path; midpoint representation blocks it

Implemented interleaving of existing full measurement batches and complete local/
remote cavity transactions, with whole-mesh final certification unchanged. No
approximate metrics, relaxed gates, new cap/configuration or scenario dispatch.
Sensor-only P1 keeps cheap bulk measurement. Serial/MPI2/MPI4 and independent N4/M2
RANS/SA/BDF/SU2/CGNS/restart pass on the precursor producer; the stronger finer-sensor
case remains rejected at4096 work. Controlled54->432 fixture retires6 unmeasured
cells, avoiding separate seed work; private old/new cavity integration still costs.

Actual M6: all38,800 parents initialize in26.98s; first64 cells seed in.12825s and
one volume proposal starts. It rejects at51.53s on an original-donor query, no volume
edit/accepted wing/transfer. Previous run spent249.79s seeding32,832 cells and never
entered volume operations. Different rejected work is not a full-event speedup.
One diagnostic repeat adds only error coordinates/admitted identities: exact rational
audit proves the guide is a rounded midpoint outside the original volume, while
its ideal midpoint belongs to3 ALREADY IMPORTED cells. Not missing MPI support.

Shared exact midpoint sensor queries/callers are implemented and scoped-qualified:
original P1 at the ideal declared midpoint, geometric BL at represented coordinate;
ordinary point queries/final gates stay strict and cache separation is explicit.
No barycentric epsilon, new source extrapolation or endpoint-tensor averaging.
The existing backend did not dump the full rejected private M6 grid; witnesses,
all source/input/provenance/failures and controlled grids remain. Broader retained
campaign indexes, independent transaction batching, transfer sharing and measured
balance/repartitioning remain incomplete. No CFD advancement or cluster request.
[Current scope and exact evidence](GOAL3_PROGRESSIVE_MEASUREMENT.md).

Current midpoint producer:16 focused serial cases/8980 assertions, MPI2/4 and independent
N4/M2 initialized RANS/SA/current+BDF/conservation/SU2/CGNS/restart PASS. Stronger
finer-sensor4096-work campaign remains FAIL. Changed M6 passes the guide, then rejects
at84.355s on strict source-segment endpoint coverage after30 private rounds. No
accepted wing, transfer or full-event speedup. Verification checks source/build/
binary/input/helper pins, without repeating CFD/regressions. User extension adds
opt-in budget-aware outer-BL selection under explicit protected/allowed limits;
distance/region/normal/tangent diagnosis comes first. Not yet implemented/validated.
[Policy scope](GOAL3_BUDGET_AWARE_BL.md).

## 2026-10-18: budget-aware BL extension — diagnosis before relaxation

Worked in a frozen diagnostic scope: two saved8192-point clouds preserve original
RAW9 P1/composed correction rows; MPI2/4 new cost rows match exactly. Coarsest BL
correction805274/805597 (+/-28016/28029 HEURISTIC3SE). First-cloud protected10*h0
already costs~96587,50*h0~223596; outer span quarter~42%, with fine tangential sizes
persisting away from wall. No background/sensor or count/infeasibility confusion.

Independent one-wall/isotropic spectral experiment (native baseline drift<=8.1e-9)
shows growth2 with protect h0 leaves263--265k, growth10 leaves125--128k; diagnostic
near-limit1e6 still107--110k. Ordinary growth freedom is insufficient for100k in
these estimates; do not force a success with physical-accuracy defaults or an
unjustified tangential relaxation. Unchanged support/fade, no production selection
or accepted mesh demonstrated. All native rows, old/new source capsules, hashes,
failures and resource receipts retained in budget_bl_cost_v1.

A connected native3D opt-in policy draft now reaches config, original-family
normalization, bounded collective profile selection/versioning, sample/rule/cache
invalidation, retained source support and frozen remesher reuse. Curved/interacting/
finer/stale primitives and actual cfg-driven RANS workflow checks are written.
Build7876 is ongoing, no tests or production success claimed. Global error
calibration/allocation, complete policy validation and lifecycle performance remain.
[Current policy/diagnostic scope](GOAL3_BUDGET_AWARE_BL.md).

Policy draft follow-up: connected build completed after one unambiguous Metric-type
qualification fix; failed first build preserved. Reviewed stage handling rejects
clear estimated budget conflicts before publication and keeps correction available.
Four primitive cases/256 assertions PASS. Initial disabled campaign rejects only
unnecessarily changed exact legacy wording; initial enabled campaign stops at the
preliminary UNKNOWN dimension0 config before any metric/construction. Both complete
receipts preserved. Root fixes grouped: restore off-policy text, validate dimension
when known. Build15979 ongoing; qualification_v2 not run yet. No enabled workflow,
M6 auto-selection, physical accuracy or performance success is claimed.

Qualification_v2: disabled compatibility22 cases/2056 assertions PASS and primitive
4/256 PASS. Enabled budget3 selects first uncertain sampled profile(lambda.5),
then same original-field reassessment on private constructed mesh tightens indicator
3.55x:3.119586 +/-0.145765 ->3.162588 +/-0.041057. Publication correctly rejects at
original-family lower scale; no accepted workflow. This supports more informative
reassessment, not correctness of the initial budget choice or general construction.
Root correction: retain existing strict planar integration rather than forcing
this supported case onto uncertain sampling. Bounded fallback remains for difficult
integration; profile/version-aware rules and protected-distance cuts retained.
No ceilings, physical limits or mesh gates raised. Partial mandatory-band estimate
has infinite uncertainty until independently controlled. Build3010 ongoing; fresh
qualification_v3 pending. No new full CFD/cluster run.


## 2026-10-18: budget policy qualified slice — what worked and what did not

- Worked: explicit opt-in outer-normal-only freedoms, protected band, original-family
  profile/version freeze/cache invalidation and collective N/M agreement. Accepted
  controlled RANS/SA/BDF workflows pass independent geometry/height/q/J/full-edge/
  P1/conservation, atomic refusal, enabled SU2+CGNS and restart.3->2.95499 and
  20->19.99931 reference complexity; MPI1/2/4 scope retained. No physics-accuracy claim.
- Worked: reuse existing strict planar integration. Forcing that supported case onto
  fixed sampling selected an uncertain profile then failed budget reassessment.
  Strict integration admitslambda.875 rather than weakening budget/mesh gates.
- Worked: match normalization work to unresolved integration. At M6 budget300k,
  same profilelambda.75 now freezes the coarsest provisional field in6.55s rather
  than93.48s;7 vs933 gradation sweeps. The provisional scale changes; no same-field
  kernel/full-event speedup claim. Original donors and all final gates retained.
- Worked within scope: retain donor preparation across profile trials while rebuilding
  profile-dependent samples. M6 zero repeated donor searches,31956 refresh geometry
  queries,15.46MB retained integration max/rank. Cold prep3.51s/background.99s remain.
- Did not solve100k M6 budget: declared growth2/protecth0 ends260488 +/-10780,
  MPI2/4 agreement2.04e-13 relative. At1000 the protected lower estimate itself
  exceeds budget; at100k it does not. Neither result proves exact infeasibility.
- 300k remains uncertain: saved holdouts314146 +/-12153 and312539 +/-11852 put their
  lower heuristic endpoints just above budget. Selected308423 +/-12078 overlaps it.
  This is evidence for explicit reassessment, not proof an adapted M6 will succeed.
- Did not establish affordable selection on planar cells:90 strict rules/3.7M
  density calls remain for the tiny budget3 case. Did not implement calibrated
  absolute-global-error allocation, general curved/interacting accepted policy,
  2D opt-in or the wider repair-campaign query/transaction package in this slice.
- Preserved failed controls: fractional ADAP_COMPLEXITY parsed as integer, missing
  mesh-bound markers, and missing restarted Backend/reference initialization were
  fixture errors; none justifies relaxing production checks. Final scopes pass.

[Report, inspected grids, complete phase/byte/memory receipts and provenance](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/budget_bl_policy_v1/README.md).
Full Goal3 active; no accepted M6 construction/physical accuracy/scaling claim.
