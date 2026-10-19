# Suggestion outcomes — current index

Full Goal3 is active/incomplete. This index separates measured scoped results from
unimplemented work and whole-event claims. Detailed successes, failures, rejected
experiments, accuracy/work/budget/memory/communication records and source provenance
are preserved in the [complete chronological ledger](SUGGESTION_OUTCOME_LEDGER_HISTORY_2026_10_18.md).
That archive stays in the same directory so its existing relative links remain valid.
The [initial suggestion snapshot](SUGGESTION_INITIAL_SNAPSHOT_2026_10_16.md) is retained.
This is not a completed-goal final report.

| Suggestion | Current outcome | Quantified scope and remaining limits |
|---|---|---|
| Coordinate surface and volume placement | Complete controlled annular serial and MPI lifecycles accepted; general scope open | Same50-cell patch, one split: off-edge insertion plus two interior points passes exact geometry/shape with77.24% changed-edge squared-excess reduction; adding two smooth symmetry movers gives86.97%. Restricted on-edge placement still fails q/J. Native serial annular24->218 now passes independent geometry/full metric/height, RANS/SA+BDF transfer and real restart. MPI2 shared-apex rounding blockage is now fixed; complete N1M1/N2M2/N4M2/N4M4 workflows independently pass. No general geometry or performance claim. [Replay](../integration_evidence/native_3d_bl_v1/annulus_coordinated_replay_v1/README.md). |
| General finite symmetry authority | Core/source backend qualified; full runtime integration open | Shared arbitrary2D/3D charts pass9cases/3731assertions; exact-rational320sample audit bounds rounded coordinates. Multiple holes/notch/disconnected components and BC-label independence pass. New native-ref v4/source backend passes serial12/3974 and MPI4, with independent75-stencil original-ID/tensor/codec equality. Cartesian N4/M2 lifecycle independently passes. No accepted oblique lifecycle, analytic/shared-plane source junction, affine-wall BL or exterior completion; Cartesian annular serial/MPI lifecycle now passes. [Core evidence](../integration_evidence/native_3d_bl_v1/affine_geometry_core_v1/README.md). |
| Match normalization accuracy to integration uncertainty | Worked in controlled3D workflows | At most24 correction trials/two frozen stages using the SAME original unclipped family; eight planar RANS workflows independently0.194--0.323% below budget. M6 initialization now freezes295068 +/-5769 against300k, explicitly integration_limited. No exact-root claim. [Stages](GOAL3_FROZEN_NORMALIZATION_STAGES.md), [M6 evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/adaptive_budget_evidence_v1/README.md). |
| Allocate absolute error globally | Partial | Owned reductions, global sensor/correction indicators and bounded8192->32768 evidence stages resolve M6 profile classification. No calibrated general tails, sequential confidence bound or distributed per-cell error-priority allocator. [Runtime policy](GOAL3_RUNTIME_COMPLEXITY.md). |
| Reuse geometry across scale/profile trials | Worked within bounded scopes | Original geometry/source stencils retained; current profile changes refresh actual wall samples. Latest identical-sensor background cache reduces46.72M->6.67M determinants (-85.7%), +1.077MiB/rank; observed total6.552->7.515s gives no event speedup. [Report](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/background_trial_reuse_v1/README.md). |
| Integrate sensor plus BL correction | Worked for difficult faceted3D initialization | Same-config saved M6 metric phase415.97->16.47s; coarsest790200.50 +/-27958.45 retained. Correction depends on the actual sensor-plus-BL composition; unresolved mass is not dropped. Not whole-remeshing speedup. [Scope](GOAL3_RUNTIME_COMPLEXITY.md). |
| More integration subdivisions | Mixed/negative evidence | Cell329 accuracy nonmonotone;88 work/time increases. More cells alone did not reliably improve accuracy/cost. Do not restore a subdivision/limit ladder. [History](SUGGESTION_OUTCOME_LEDGER_HISTORY_2026_10_18.md). |
| Geometric root screening | Withdrawn | Cell88 .2003->.2090s; no useful gain and unchanged estimates. Source/negative receipts retained. [History](SUGGESTION_OUTCOME_LEDGER_HISTORY_2026_10_18.md). |
| Actual adaptation should ease integration | Worked in controlled cases; M6 unresolved | Same corrected ORIGINAL field on refined partitions reduces indicators3.07--4.07x and observed error5.48--77.70x; coarsening does not guarantee improvement. No adapted-node donor refit. Saved rejected M6 finer partition now reduces the8192-point heuristic indicator39.27%, but assessment costs70.64% more; accuracy improvement is unproven. [Exact-field comparison](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/m6_same_field_reassessment_v1/README.md). [Stages](GOAL3_FROZEN_NORMALIZATION_STAGES.md). |
| Opt-in budget-aware BL relaxation | Scoped complete workflows qualified | Mandatory firstheight/protected/finer sensor/geometry retained; only declared outer-normal growth changes. M6 permitted growth2/protect2e-6:100k sampled-profile conflict,300k selectslambda.875; final estimate295068 +/-5769. No proof of exact infeasibility or automatic physical accuracy. [Policy](GOAL3_BUDGET_AWARE_BL.md). |
| Preserve geometric localization and fix strict source-point coverage | Worked in recorded arithmetic/geometry scope | Exact affine rounding/at most8 corners fixes captured outside source midpoint without loosening predicates. Complete N4M2 conservative RANS/SA/BDF/reload/restart controls pass. Historical owner-UPPER6 M6 still rejects~300s. [Construction](GOAL3_PROGRESSIVE_MEASUREMENT.md), [field identities](GOAL3_M6_FROZEN_FIELDS.md). |
| Retain repair-campaign donor support/indexes | Partial; broad retention open | Canonical owner sampling and physical-interior hypotheses qualify;1.407M hints in historical M6. Whole-donor preparation retention failed; broad campaign support/index retention is not implemented. No net event gain established. [Outcome](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/query_campaign_reuse_v1/README.md). |
| Extend shared-edge integral reuse | Worked in query planning; actual repeat incomplete | Exact endpoint/field masks now skip546985 support slots and deduplicate56451 on the saved N4M2 repeat; stage cache stays bounded. Rank0 donor189->41s, candidates34.9M->3.23M; event still rejects6144work/134s with344edits. Calls increase; no accepted-event speedup. [Current scope](GOAL3_QUERY_REUSE.md). Historical campaign adds6420 hits. [Current evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/query_campaign_reuse_v1/README.md). |
| Batch independent transactions | First-row gains; general faceted concurrency now passes controls |64-parent first-row stage12.7%/23.2% lower at2/4 ranks; later ranking removes~42% metric work (overlapping gains, do not add). New complete-proposal reservations permit4independent commits/epoch; serial9/735, MPI4 and actual opt-in/default N4M2 independent workflows pass. Exact frozen M6 reaches3simultaneous commits,326764->280568kernel calls/rank (-14.14%),1707->1955private edits (+14.53%), but still rejects300.609s with no clear residual improvement. No event speedup/accepted wing. [Current experiment](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/concurrent_composed_proposals_v1/README.md), [transaction scope](POST_COMPLEXITY_QUERY_TRANSACTIONS.md). |
| Avoid unchanged failed searches | Worked for progressive selection; broader memoization open | Existing blocker map now survives measurement and unmeasured owners. Actual unchanged-BL N4M2 repeat accepts5440->3768 at4511work/88.5s, previously rejected6144work/134.3s with the same344edits. Complete field/geometry/transfer gates pass. New reservation conflicts still defer. No general reconstruction memoization. [Current evidence](GOAL3_REJECTION_DEPENDENCIES.md). |
| Share transfer geometry across fields/histories | Existing facilities retained | Correction stages transfer once after final acceptance and share original reassessment/return source. Additional overlap-weight sharing/performance campaign not attempted. |
| Measured-cost balance/mid-remeshing migration | Not implemented | Existing N/M lifecycle qualified. Migration, index rebuilding, return and solver/transfer cost must enter break-even; historical2D lower imbalance could be slower. No new balance/scaling claim. |
| Apply successful policy in2D | Foundations retained; extension incomplete | Existing strict2D composition/integration controls retained. Opt-in profile selection/current correction provider is3D-only.30P30N2D input prepared, not yet adapted. Supplied3D hex case exceeds native tetra support. |

Additional accounting requests remain in the archive: cold preparation versus
reused work, geometry/source/placement versus density, peak/transient memory versus
retained-byte models, and nested/synchronized timing attribution. A residual is
not automatically pure geometry or MPI; maxima of nested clocks cannot be summed
as an exact critical path. No clean whole-event affordability/scaling conclusion
follows from contended correctness runs.

The previous complete selected-profile M6 attempt is [recorded here](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/selected_profile_construction_v1/README.md):
300.434s atomic rejection,1707private later edits, no transfer or accepted mesh.
Current concurrency work preserves its exact original sensor/profile and unchanged
mesh/time/work gates, avoiding another Hessian/normalization build. Construction
feasibility and estimated budget feasibility remain separate.


Latest original-edge witness: five original tetrahedra recover the persistent
reported obstruction face and an exact centroid match, outside configured BL
support; sensor edge2.60882>1.8. This motivates a small generic cavity replay,
not proof of late-cavity equality or a cause for the entire wing failure.
[Data and viewable original patch](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/concurrent_composed_proposals_v1/original_edge_witness/README.md).

Original-star native replay now establishes local feasibility: existing joint
placement passes unchanged geometry/Refine/q/J gates in3trials, q=.273986,
J=.086470, changed-edge excess.654190->.023844; trial work.1202s. Balanced
insertion fails J=.040146<.05. This does NOT resolve M6 or establish late-cavity
equality. The observer capture atomically rejects300.641s, retaining64exact
contexts/14distinct features; all input snapshots unchanged. Captures include
evolved volume stars and expanded wall/transition patches. No new placement
fallback or weakened gate is justified.
[Replay and inspectable private grids](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/README.md).

No new production improvement is claimed by this diagnostic checkpoint. Replay
the captured contexts before deciding whether reconstruction or coordination lacks
a capability. The original-star result specifically argues against another local
placement fallback. Exact runtime/build/input/helper provenance and verifier are
in the linked report. All owned jobs terminal.

Current connected follow-up:10EXACT evolved volume stars pass9/10; trial phases
sum1.818s/no source failures. The remaining7-cell star and negative full-fan probe
are preserved. Retaining the initial blocker enables bounded targeted star
admission when the old cover cannot fit64cells. Serial10/748, MPI4 and independent
opt-in/default N4M2 RANS workflows pass. M6 still rejects300.994s;167dependency
expansions demonstrate path use, not a construction fix or net speedup.
[Evolved evidence and full rejected grid](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/evolved_volume_v1/README.md).

**A sufficient obstruction is now diagnosed.** Current SYMMETRY edge211--3481 lies
between distinct exact near-coplanar facets. Its only interior binary64 point is
the midpoint; neither child has another. Full frozen composed lengths12.796parent,
11.572left/1.225right cannot satisfy1.8, while sensor-only parent is.325. The lattice
part is exact; metric values are native fixed-accuracy measurements, not interval
bounds. Under exact-facet/conforming/binary64 rules this frozen case cannot finish;
more cavities/ranks cannot repair it. This is not physical BL or budget infeasibility.
[Certificate and explicit assumptions](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/cavity_operator_replay_v1/evolved_volume_v1/full_state_v1/geometry_lattice_v1/README.md).

Complete terminal working mesh541027tets/139884points and exact versioned owner
records are preserved. Shared-node coordinate and unique ownership screens pass;
full mesh acceptance/transfer does not. The earlier volume witness edge/vertex are
absent, confirming that last intermediate rejections cannot alone diagnose final
blockage. Further unchanged M6 construction runs are unwarranted. Geometry-reference
preference is pending; no authority, target, profile or acceptance gate was changed.
Other full Goal3 work stays active. All owned jobs terminal.

Strict exact-facet diagnosis is now integrated on the mandatory composed seed path:
serial11/760, MPI4 eleven cases/rank and independent opt-in/default N4M2 workflows
pass. The actual captured M6 crease MPI1/2/4 elects the same conflict with immutable
input. At most two extra child integrals, no new collective/gather or sensor-only
import requirement. Preliminary broader diagnostic was narrowed after identifying
a compatibility risk; its source/controls and failed helper compile are preserved.
This does not produce an accepted wing or establish event speedup.
[Final evidence](../integration_evidence/native_3d_bl_v1/oneram6_uncertainty_v1/strict_geometry_preflight_v1/README.md).

Closed-sphere construction/removal adds a complete curved capability: generic
geodesic sampling and regional coarsening produce8->5440->320cells, native work
6087/6144, times202.612+57.384s under300s. All mesh contracts remain; exact geometry/
height/shape and complete native BL edges pass. Release additionally passes the
independent unchanged sensor tensor/Euclidean-edge oracle. Surface planning448->320
faces/604->162trials; this is not a whole-event speedup. New measurements identify
validation50.327/57.384s in release. Failed inherited plans/local coarsening/setup
and helpers are preserved. Final-source serial/MPI4 primitive N/M/transfer/restart/
SU2+CGNS compatibility independently passes15lifecycles/30events; big BL sphere
MPI generation/CFD and affordability remain open.
[Current scope](GOAL3_CLOSED_SPHERE.md).


Exact shared-simplex contact certification addresses the measured validation cost
without changing the BL policy or any acceptance gate. Same320-cell contact test
44.874->12.220s; complete byte-identical sphere construction202.612->103.576s,
removal57.384->20.121s, combined259.996->123.697s (observed52.42% reduction).
Work6087 remains. The90-second full-BL contact-only replay exhausts its ceiling;
its negative receipt remains, while the complete native construction passes all
checks. Current serial/MPI4 primitive lifecycles are qualified separately. These
contended comparisons establish no scaling/CFD affordability or accepted M6 claim.
The reaffirmed BL budget contract and its unfinished global absolute-error allocator
remain part of active Goal3.
[Exact certificate, receipts, output/oracle reuse and limits](GOAL3_CONTACT_VALIDATION.md).


## Actual curved RANS restart and donor-footprint follow-up — 2026-10-18

**Worked, with a remaining private-path limit.** The exact320-cell endpoint star
needs only6 positive-length edge donors. Existing canonical point sampling plus
metric-specific exact interval footprints fixes actual CGNS-restarted N4/M2
removal5440->320 without raising256. Native37.664s/483work, complete fixture38.355s;
actual current+BDF flow/SA integral defect2.06e-14 and sensor residual0, independent
fresh exact geometry and both outputs/reload pass. Cold accepted producer8->5440
(native114.506s) is reused across builds with separate provenance, not recomputed.
Serial11/828, MPI2/4 and independent strict/enabled planar N4M2 controls pass.

**Did not finish repeated BL adaptation.** Unchanged-BL update passes full refinement
then atomically rejects61.35s/1338work in the older private closed metric-cell
footprint. This is not an accepted update or affordability result. Required next
change covers canonical actual/ideal-midpoint queries and complete edge intervals
in private measurement/proxies; preserve exact source and geometry semantics.

**No new budget-policy claim.** Current sphere uses prescribed identity sensors,
not normalization or enabled selection. Full global absolute-error allocation,
enabled curved/interacting profiles, two-dimensional qualification and complete
performance remain open. No fixed-domain mass claim for curved cold construction:
its ledger explicitly includes2.713711347 fill volume. No CFD advances or viscous
convergence. Closed query contracts,256cap, profile/fade and gates unchanged.
[Receipts, costs, failures and limits](GOAL3_SPHERE_RANS_LIFECYCLE.md).


## Finite private queries/admission — 2026-10-18

**Worked:** canonical original centroid/midpoint support and complete positive edge
intervals remove the private donor-union obstruction without raising256. Existing
proposal-local refusal handles oversized coarsening stars without raising64.
Current serial16/1241, MPI2/4 and independent strict/enabled N4M2 controls pass;
the saved320cell core completes the capacity control. Exact endpoint-only face
filtering cuts71264->20768exact orientations in the identical32-pass probe; observed
.262->.095s is local, not full-event performance.

**Did not finish:** the real unchanged-BL repeat now makes344private edits/5499work (192rank0+152rank1),
then rejects at300.034s. CFD states remain unchanged; incomplete cached coverage
precludes acceptance. The initial16cells/64queries mismatch is corrected and its
failed attempt retained. Last local height errors do not prove infeasibility.

**Measured remaining gap, rank0:** donor188.971s, metric41.824s, update18.862s. Existing
scalar reuse records394472hits versus43336new private integrals, yet imports still
repeat. This does NOT implement the full suggested donor-campaign cache, missing-
edge planning or independent transaction batching. Metadata LookupBatch is not
transaction batching. Next package should avoid importing support for certified
reused lengths and share shape-first query work, preserving field/endpoint identity,
coverage and collective closure. No new normalization/2D/physical/scaling claim.
[Outcomes, receipts, controls and all retained failures](GOAL3_PRIVATE_QUERIES.md).


## Finite support reuse — 2026-10-18

**Worked:** import only missing full-field edge support under exact endpoint/identity
certificates, deduplicate segments, share the existing bounded edge cache for one
frozen stage, and reject sufficiently proven wall-height/geometry collapses before
transactions. Current serial22/1337, MPI2/4 and independently audited strict/enabled
N4M2 RANS/SA/current+BDF/SU2+CGNS/reload/restart pass. No cap or target changes.

**Still failed:** same difficult repeat makes344private edits then rejects6144work
in134.280s; all accepted CFD states unchanged. Previous reject was300.034s/5499work.
Rank0 donor188.971->41.248s and index candidates34.9M->3.23M show reduced waste;
tracked calls602671->789281 expose remaining coordination overhead. Different
rejected work gives no accepted-event affordability claim. No terminal private
grid was exported by this fixture. General failed-search memoization, donor-index
retention, transaction batching and global-error allocation remain open.
[Report, all failures and verifier](GOAL3_QUERY_REUSE.md).


## Accepted repeated BL workflow — rejection dependencies

**Worked:** measurement no longer reopens rejected directions; owners retain
blockers before edge measurement; actual changed stars still reactivate. Same
6144/300s/mesh gates now accept N4M2 actual CGNS/BDF/SA repeat5440->3768cells,
4511work/88.5native seconds,344committed edits. Independent exact geometry/height/
62804changed overlap pairs and actual flow/BDF/SA conservation1.87e-14/sensor0/
SU2+CGNS reload pass. Primitive serial30/1769 and MPI2/4 cover complete repeated
cycles, late field discovery, notification and atomic budgets.

**Reporting correction:** prior query-import donor timings overlapped seed/update
in the exclusive phase sum. Parent exclusion now restores the unchanged timing
invariant; both initial timing/stale-message test failures are preserved. No mesh
assertion was weakened. Native event versus independent audit costs are separated.

**Still open:** global error allocation, full enabled curved/interacting policy
workflows,2D opt-in, independent wider composed-field audit, viscous accuracy and
accepted-event affordability. Same edits do not prove identical unexported rejected
meshes; different work and local contention preclude clean kernel/scaling claims.
No general failed-search memo table, new transaction batching or broad donor-index
cache. [Complete records and grids](GOAL3_REJECTION_DEPENDENCIES.md).

## Declared symmetry geometry — 2026-10-18, in progress

User explicitly prioritized a declared planar symmetry reference after the accepted
spherical repeat. Input audit supports M6 Y=0:4152/4184 symmetry vertices differ,
maximum4.59885e-12;210wing/64farfield junction vertices;21367affected tetrahedra
retain orientation, screened volume ratio.99999999878..1.00000001727. This is an
input geometry audit, not global embedding, field transfer or mesh acceptance.
The8094-facet symmetry patch has Euler characteristic0 and274border edges: finite
hole-aware containment is required; an unbounded plane or approximate merging is
not a valid replacement.

Connected implementation adopts an explicit Cartesian plane only during initial
geometry definition, before states and original donors are frozen. Every shared
vertex copy moves consistently. Restart and returned geometry only audit exact
conformity. No old off-plane field is silently reinterpreted. Complete controlled
workflow and affected checks are pending; no new M6 event.
[Current policy/evidence](../integration_evidence/native_3d_bl_v1/planar_symmetry_reference_v1/README.md).

The curved selected-profile preflight exposed a separate provider routing gap:
opt-in analytic walls enter faceted correction support and reject before budget
trials in.346s. The collector's expected budget-conflict assertion also fails.
Preserve all evidence; this establishes neither an integration estimate nor a
construction result. Deferred under the user's geometry-first order.
[Preserved preflight](../integration_evidence/native_3d_bl_v1/budget_curved_interacting_v1/README.md).

## General symmetry extension and controlled feasibility — 2026-10-18

User requires finite arbitrary 2D lines/3D planes, multi-body/hole/disconnected
regions, simultaneous junctions and complete Euler/RANS/MPI/donor/restart/exterior
workflows. Recorded without narrowing Goal3; shared affine core is in development,
not runtime-qualified. Cartesian initializer passes independent serial/N2M2/N4M2
RANS/SA full lifecycle and focused MPI4 geometry/reference regression.

Annular normalization works (~20), cold native construction still rejects8192work;
exact terminal154-cell grid/last edge1--5 proposal preserved. Same-field independent
manual witness204tets passes q.305/J.0959/L1.740/height0/finite hole/embedding. This
establishes scoped target feasibility, NOT native construction or transfer. Need
coordinated construction reconsideration, no raised caps/fallback ladder/M6 retry.
See GOAL3_PLANAR_SYMMETRY.md and planar_symmetry_reference_v1 evidence.

## Joint surface/volume placement and multi-apex construction — 2026-10-18

**Worked in native code:** coupled point placement plus complete smooth surface
stars,7cases/18743assertions and current independently verified N4/M2 Cartesian
RANS/SA/BDF/SU2/CGNS/restart. Native exact saved-patch geometry/shape passes with
44.56% changed-edge squared-excess reduction; independent77%/87% oracles are not
native performance/results. Original target and all gates stay unchanged.

**Still fails:** complete native annulus makes96edits/339cells then stagnates at
2300/8192work. Four full-field1.865929edges remain (3.66% too long), sharing one
first-height apex; empty rejected queues are not zero metric residual. Analytic
retained-ownership lower bound1.834077>1.8 and the floating single-background
kernel diagnosis justify changing construction rather than raising limits.

**Worked offline, port pending:** three quality-directed binary star splits retire
that apex only, preserve the wall mesh/all original link vertices and replace
36cells by52. Complete355cells independently pass exact geometry/embedding/height,
q=.2167007/J=.0558188/Lmax1.7955165; original complexity20.00019535 (+9.77e-6relative).
Opus5.5/medium helped review the diagnosis; its additional-retirement necessity was
not supported by this valid witness. Plain positive partitions and fixed-topology
apex movement fail shape checks. The1e15-capacity SciPy min-cut draft overflowed
int32 and is **withdrawn**, not evidence of infeasibility; corrected bounded flow
self-checks and the independent audit pass. No native multi-apex implementation,
transfer/MPI/restart/viscous or affordability claim yet.
[Evidence, limitations, all failures and inspectable grid](GOAL3_ANNULAR_COORDINATION.md).

## Generic native height-star implementation — validation pending

Converted the offline coordination idea into a native bounded operator using the
actual wall-patch direction and metric spacing, exact-orientation implications,
uint64 quality-directed cut, fixed-interface checks and final first-height/full-
field gates. Existing complete-star lookup/reservation, original-donor import,
collective work admission and atomic publication are reused. No fixture IDs,
coordinates, solver modes, metric changes or larger cavity caps enter production.

The connected batch and difficult339-cell replay are BUILDING/UNQUALIFIED in50386.
Initial aggregate initializer compile failure is preserved; corrected source has
compiled, engine/CFD/test objects remain. Generic spacing choices1/1.25 work in the
offline kernel;1.5/1.75 fail visibility, not global feasibility. Optional Opus5.5/
medium review reached240s without a final report and is not evidence. Native
whole-field replay, full unchanged annular lifecycle and affected MPI still needed.
[Current sources/provenance/replay](../integration_evidence/native_3d_bl_v1/native_height_star_v1/README.md).

## Preserved04e9c75dd1 height-star outcome, 2026-10-18

Three private topology cuts work in native serial construction under unchanged
constraints. Full annulus24->218tetrahedra/71vertices; q=.2033345/J=.0572923/
Lmax1.7955165/height0, transfer defects<6.6e-16 and both real restart residuals0.
Tentative finite-hole collapse now refuses an invalid whole face before strict
surface error; the full accepted event passes. Two initial replay negatives were
fixture mistakes (missing cyclic face), explicitly withdrawn and preserved.

MPI2 does not pass:335cells retain four1.86593edges; exact34-cell-star serial replay
also rejects. Offline partition success motivates diagnosing native coordinate/
constraint handling; it does not qualify native MPI construction. No unchanged
full MPI rerun or cap increase. N4 cases not started.

Original-field reference complexity20.00019535; final estimate20.00728648 with
heuristic.185127 indicator. This refined assessment is not more accurate than the
initial one; no monotone-refinement claim. Actual BL metric initialization7.879s
versus native5.732s and replace.01757s suggests normalization remains material.
Contended correctness receipts are not speedup/affordability evidence. Combined
gradation18violations/max4.599 and strong first-row tilt remain explicit limits.
[Evidence, failures and current scope](../integration_evidence/native_3d_bl_v1/native_height_star_v1/README.md).

## Current coordinated construction and MPI outcome, 2026-10-18

The saved MPI negative was not an admission/deadlock problem. Native original-field
query returned xx/core differing by~1.8e-15 relative; all four naive proposed grids
missed2/8mandatory normal-foot triangles. Exact affine-midpoint bracketing also
failed6/8. Immediate binary64 neighbour admission, at most27coordinates per leaf,
recovers8/8 without changing the field, tolerances or geometric authority. Native
three-cut replay then passes61816assertions and an independent352cell/7929-overlap
full-field mesh audit. Original serial replay11/81755 and355cell audit also pass.

Complete initialized RANS/SA/BDF/IO/actual restart workflows now independently pass
N1/M1,N2/M2,N4/M2,N4/M4:218/194/194/215cells,1035/1136/1133/776shared work,
q>=.20238/J>=.05648/Lmax<=1.79828/height0,conservation<6.6e-16,restart0. Serial
accepted SU2 output is byte-identical; serial/N2 frozen profiles match prior inputs.
No arbitrary rotated/junction/multiple-body/2D/exterior/M6 or viscous-accuracy claim.

Observed metric/native/replace seconds are7.826/6.275/.0102 (N1),3.858/6.298/.0116
(N2),4.287/6.368/.0122 (N4M2),4.975/7.014/.0133 (N4M4). Different valid grids and
contended host prevent a scaling/speedup inference; normalization remains material.
Coordinate admission has no extra density/MPI query, bounded geometric work is
included in reconstruction but not separately timed. Full composed gradation and
global uncertainty calibration remain open; refinement did not monotonically
improve the estimates. [Complete receipts](../integration_evidence/native_3d_bl_v1/native_height_star_v1/lifecycle_summary.json).

## Affine adoption and retained-cell construction — 2026-10-19

Worked, scoped: build v1 and8serial cases/3981assertions pass, with independent exact
Euler/RANS one/two-component initial geometry/original sensor/reference audit and
positive-cell narrow-gap contact rejection. Original auditor rounding/orientation
mistakes are preserved and corrected on saved data, without CFD reruns.

Did not work: oblique annulus adopted geometry and normalized its frozen original
field, then rejected first-row construction after2work. All four background proposals
fail cell/fan containment although the required point is inside its wall tetrahedron.
An exact positive stellar-insertion witness establishes a missing placement/construction
freedom; it does not establish full metric/shape acceptance. Metric build15.2705s and
native.00487s are rejected/contended receipts, no performance improvement claim.

Connected follow-up removes the extra background and its ranking donor/query work:
retained-cell normal-point insertion, already-valid row retention, shared reconstruction
caller and affected tests. Build49032 and23affected cases pass at MPI1/2/4, with
independent source/initial geometry/collision audits and bitwise source consistency.
No accepted oblique mesh, complete new MPI lifecycle, affordability or full closure.
[Failure, diagnosis and immutable results](../integration_evidence/native_3d_bl_v1/affine_initial_adoption_v1/README.md).

Further exact audit changes the next action: wall/farfield edge1--9 is forced by two
undeclared exact planes, has no interior binary64 point, and has constant-original-P1
sensor length2.249645>1.8. Removing the background does not solve this later geometry/
target conflict. Unchanged full retry cancelled before runtime; affected controls
completed in17339. [Mapped-feature/source requirements](GOAL3_MAPPED_FEATURES.md).
An abstract Opus5.5/medium consultation is preserved with its corrections; no repository
source/data was supplied. Its initial exact-coordinate-only advice was withdrawn;
tags and conforming collars are design patterns, not proofs of general source coverage.

Historical prototype source/binary affected controls are sealed by controls_summary_v2:
23cases per size, serial6239assertions and MPI rank-specific assertions recorded.
No full regression/CFD run was added; source provenance was checked before reuse.
These controls verify the construction substep, not the complete required mesh.

## Scope correction after an adversarial coupled-wall witness

Did not work generally: replacing the shared-background constructor with sequential
wall-point insertion. A convex oblique two-wall tet blocks in both insertion orders;
an exact common-fan witness gives10positive/nonoverlapping cells and volume closure.
The23prototype MPI controls missed this configuration; no general robustness claim
survives them. Current source restores the shared constructor/reconstruction caller,
limits direct insertion to one wall face/tet and adds native constructor plus owner/
empty-rank regressions. Four translation units pass syntax; affected linked MPI1/2/4 controls now pass
in joint_affine_source_v1 (19cases per rank, not a general construction proof). Source/proof in joint_scope_provenance.json;
no extra full suite/CFD run was launched. This preserves useful single-wall insertion
without discarding existing coordinated construction.

## Source coverage comparison — 2026-10-19

Did not work generally: nearest-boundary trace selection. Exact rational reflex
wedge has unequal one-sided limits despite continuous SPD original trace. Saved
M6 owner-node P1 probes show relative spectral differences8.29e-11/8.29e-6/.08594
for1e-13/1e-8/1e-4bands. This is a diagnostic field, not retrofitted frozen rank-halo
samples, a corrected normalization or a composed-BL certificate.

Worked, scoped: common original-node columns give a complete saved annulus source
collar at the numerical mapping band:96positive cells/3762exact conformity checks/
1024exact old-outer face separation checks. Original embedded source is assumed.
This is source-domain coverage, not an adapted CFD grid or a general geometry result.

Did not work generally: wide common columns. At1e-4, two WING3D faces give four
negative prism cells even though all12sampled probes stay covered. Their source
altitudes are about9e-5. Keep actual mapping-envelope work separate from maximum
physical geometry tolerance; no fallback/cap/acceptance change was added.

M6 preparation4.83s/29.98MBnumeric arrays; selected work7.96s/1663active-set solves.
Independent follow-up13.44s/maxRSS279MiB. First report-only assembly failed after
21.4s; saved numerical results were sealed without rerun and original failure/source
retained. No C++/MPI/CFD/transfer/restart run, speedup or new accepted-mesh claim.
[Receipts, inspectable source grids and next implementation](GOAL3_SOURCE_ENVELOPE.md).

## Joint affine original-source implementation — scoped qualification

The measured numerical-band collar strategy now has production source code: current
closed physical skin, root-owned shared columns/canonical oriented faces, exact
positivity/all-pair contact and conservative finite-face clearance, original tensor
copies, collective atomic admission and work/payload counters. Pre-branch presence
and complete geometry definitions agree collectively. Existing isolated coverage
remains; joint analytic/affine and affine BL-wall support remain explicit gaps.

Worked in scope:9source and19affected cases per rank at MPI1/2/4, including
Euler/RANS and M1/M2/M4. All12 exported source domains independently pass8460exact
contact/1728clearance checks; original and extended sensor field signatures are
bitwise identical across solver/rank/worker choices. Narrow-gap/resource/declaration
refusal stays atomic, current/SA/BDF data unchanged. One current serial Cartesian
RANS transfer/SU2+CGNS/real-restart workflow independently passes:194cells,
q=.24895/J=.06308/Lmax1.79552, height0, conservation<6.6e-16/restart0.

Cost receipt:full build1749.210s plus two fixture-only incrementals196.128/195.983s;
full-build sampled descendant RSS sum2.355GiB, not PSS/guaranteed peak. Source fixture
prep.484--.489s serial for36cells/102column work/702contacts/144clearance checks,
1872replicated logical payload bytes/48.2MB admitted native bound (not wire/RSS).
These contended correctness cases establish no speedup or affordability.

Did not work initially:fixture omitted native remesher selection, causing config
refusal before the builder. Inspection caught steady/history-helper mismatch in
the negative fixture before execution. Shared native dual-time setup fixes both.
Final independent sealer passed audits then failed relative helper-path assembly;
failed source/receipt preserved, path fixed, independent audits repeated without
native/CFD rerun. No production code changed after2abdb7e5bf for these fixes.

Still missing:general mapped body/features and actual physical envelopes, affine
BL walls/mixed analytic joints,2D runtime/exterior/general complete MPI lifecycle,
viscous accuracy and complete accepted-event performance. Source-envelope grids
are query domains, not accepted adapted CFD meshes. Abstract Opus5.5/high review
supplies conditional reasoning only; no clamping or dropped native validation.
[Implementation, counters, failures and sealed evidence](GOAL3_JOINT_AFFINE_SOURCE.md).

## Mapped parameter geometry — 2026-10-19, lifecycle open

Worked as a representation foundation: original vertex/edge/facet support and
versioned scalar parameters, exact unrounded bounds/orientation, shared incident
error minima and complete original-facet tiling/feature chains.8focused cases/
2861assertions, current admission/interior1case/26assertions, independent40samples/
120bounds/120signs (60zeros), existing coordinate3cases/287assertions PASS. No full
SU2 rebuild/CFD/MPI campaign; only3geometry units compiled, pinned support reused.

Did not work as a transparent replacement: recovering original parameters from
rounded coordinates. Exact local examples show scalar information loss and loss
of valid ideal orientation under plane-foot recovery. Separately rounding1-t also
recreates chart-edge obstruction. A deliberately canonical inverse authority is a
different, more restrictive model; it is not silently adopted. Opus5.5/high abstract
review informed the choice; its normal-error example needed correction to epsilon/2,
verified rationally. No model-labelled global proof is claimed.

The first constructor copied error declarations before storage admission; fixed
with const-reference input and copy after admission, then only affected case checked.
No physical/source-metric acceptance gate or config relaxed. Original sensor P1,
actual-query BL/finer/fade untouched. The shared affine-point implementation now
uses exact parametric arithmetic; affected controls pass, whole-event cost unmeasured.

Still missing: boundary-state/private operator/atomic MPI/N-M transport, finite-plane
parameter membership, simultaneous junctions and admitted actual source envelope,
output-node renumbering/persistent native_ref/real restart,2D/general exterior meshes
and all accuracy/performance gates. All original triangle edges currently protected;
this may block construction and is not general surface coarsening. No new accepted
mapped mesh. [Evidence, costs and next complete implementation](GOAL3_MAPPED_PARAMETERS.md).

## Connected mapped geometry lifecycle — implementation under build, 2026-10-19

The forward-parameter strategy now reaches private edits, staged collective
mesh/geometry publication, isolated engine stages, original-source mapping bands,
parent N/M return, house-reader dense IDs and v5 reference persistence. Actual
boundary import checks stored IDs/coordinates/skin. Prepared original geometry
indexes are shared; no extra volume gather or per-query geometry negotiation.

Outcome is **not yet validated**. Ninja discovery and C++ helper/type-alias name
lookup failures were preserved and corrected. Official single-worker build remains
active. All6affected interface syntax checks PASS (54.07s); retained-output charging
and count-derived parent bounds were corrected before runtime. The saved-edge
transaction and oblique Euler/RANS reader/reload controls are prepared; no current
passing runtime test, new accepted oblique mesh, transfer/restart or
performance gain is claimed. All original triangle seams remain protected, and
full2D/multiple-body/exterior/mixed curved scope stays open.
[Receipts and exact validation scope](../integration_evidence/native_3d_bl_v1/mapped_boundary_lifecycle_v1/README.md).
