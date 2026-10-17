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
