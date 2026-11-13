# Connected M6 surface/volume trajectory

Current curved-BL cycle, ordered stage3→4. Read HANDOFF_Codex.md for actual handles
and frozen sources. This is one connected diagnostic/integration cycle, not a new
production scheduler, geometry authority, target or configuration. Production remains470; test-only472 diagnostics do not change construction policy.

## Current strategy, tested end-to-end within the stage boundary

This is a coupled **surface plus adjacent tetrahedral volume** trajectory. The
observation region may contain several overlapping stars; each transaction must
have its own complete valid interface and current dependencies. A large observation
union is not automatically a valid cavity. No operation is granted acceptance
because its core looks better while its neighbors are omitted.

Input is the saved441 complete composed target/profile and geometry authority,
saved RANS+SA seed, and the current transported nodal field/associations. Current
stage0 first height is6.4e-5 (32 times final2e-6), growth1.2, thickness.01. The
configuration literal final height is transformed by CConfig's existing event0
postprocessing; do not silently substitute the final-height target at this stage.
Source production470 and saved six-partition surface470 state are authoritative.
Sensor tensors alone cannot reconstruct this input. The completed470 sequential
trajectory started from the451 capture; the full470 surface capture is the newer
input for472 residual diagnosis. No new published complete trajectory from the470
capture is claimed.

The executable route and the conditions for advancing are:

1. **Restore and measure the complete affected region.** Validate target/profile,
   reference, associations, ownership and dependency versions. Measure all physical
   faces and production tangential planning edges; retain directional distributions,
   short-edge concentration and unresolved measurements. Positivity, topology,
   body separation, genuine features and already-passing heights remain mandatory.
   q/J debt is recorded, not used to veto validity-first construction.
2. **Close offending geometry while updating the neighboring volume.** Existing
   projected splitting shortens demonstrated offending chords. SplitSurfaceStrip
   retains compatible volume connectivity; if that connectivity inverts,
   ReconstructSplitPatch seeks a different positive cone. Add a neighboring face
   only for a demonstrated visibility/support obstruction and only within the
   existing bound. Rediscover and reserve the COMPLETE changed cavity before
   reconstruction/publication. Recompute the whole changed region afterward.
   Above-tolerance fitting debt may be provisional; it is never final geometry
   permission. Tangential planning success is not a bidirectional certificate.
3. **Deliver suitable tangential resolution.** Use exact SurfacePlanning lengths
   and the restricted target's directions, not triangle counts alone. Splitting
   serves overlong demands; collapse removes admissible redundant points; mapped
   relocation redistributes points on the authority; reconnection changes a
   connectivity obstruction. These operations must preserve complete-patch
   geometry, interfaces and passing heights. Coarsening currently requires zero
   candidate geometry-guidance excess, so it is not an unrestricted cleanup route
   on a geometry-indebted patch. Original wall spacing is not a physical constraint.
   Readiness requires complete requested-wall measurement, no unresolved geometry
   or overlong planning edges, and an explained directional distribution: unexplained
   spanwise crowding remains an open delivery defect even when maxedge passes.
   No blanket minimum-edge gate or new anisotropy threshold is introduced.
4. **Rebuild directions and construct a coordinated first row.** Rebuild existing
   constrained/blended direction data after relevant surface/connectivity changes;
   preserve carrier/symmetry tangency and smooth recovery along the wall. Existing
   first-child/lift/cavity transactions must cover the requested patch without
   gaps or overlaps and connect positively to retained volume. Check physical
   stage first height with the configured tolerance, certified fluid clearance,
   direction alignment/visibility, feature/junction transitions and preservation
   of previously passing rows. A measured provisional height debt or a retained
   positive cone is not first-row readiness. Do not advance merely because the
   direction routine exists or a few wall children are valid.
5. **Save a credible output or preserve an explicit stall.** A usable stage output
   includes mesh, complete parameters/associations, transported metrics, composed
   policy/profile/authority, continuation event and provenance. Only a credible
   surface/first-row output proceeds to successive fronts; actual progression,
   coverage, collisions, outer transition and final full-mesh certification remain
   downstream obligations. Final q/J/size/geometry/height gates are unchanged.

No new operator ladder or scheduling policy is implied by this sequence. In the
current source PrepareSurface delegates to Adapt with the tangential planning
target; full composed-volume edge certification belongs to AdaptMetric. The
production geometry-progress check is in CNativeEngine3D.cpp geometric_change;
GeometryGuidanceExcess aggregates face bounds. First-row facilities and strict
admission exist, but their existence does not prove this route reaches them.

### Stall and mandatory reconsideration

The SAME shared24constructor calls,300s geometry/600s outer,64old/128new cells and
12GiB ceiling cover the diagnostic trajectory. Stop safely on any exhausted bound,
incomplete/stale support, failed reservation/publication or unresolved certification.
Repeated unchanged complete-context refusals are a stall, not permission to retry
more variants. Reconsider the strategy if useful core cuts grow neighboring debt or
short-edge crowding without approaching readiness, if required placement/connectivity
cannot produce a valid complete replacement, or if geometry-progress ranking is
not comparable between old and new faces. A bounded failure does not establish
physical impossibility. Keep the last published state and exact refusal; never
raise limits, waive uncertainty or proceed to first row from failed prerequisites.

### Actual trajectory result: FAILED readiness, useful local capability only

The470 saved-input replay tested the connected route, including16 exact precursor
transactions, the real inherited split inversion, complete support rediscovery2→3,
new full reservation, positive reconstruction3→10 and publication, then continued
measurement and transactions within the original24-call bound. It ended with22
commits, observed cells36→110 (including11 previously unobserved old cells), geometry
bad16→24 and local introduced-minus-retired geometry+2, q/J+63/+63. The changing
observation scope prevents raw counts being interpreted as churn. Geometry closure
FAILED; tangential suitability UNPROVEN; first row UNREACHED. Trajectory74.886533s
wall/74.866628s CPU/1590930 geometry work; wrapper251.852s includes reconstruction
and other setup. Passing assertions mean the diagnostic behaved correctly, not that
the mesh became acceptable. R/surface_transaction470_assessment.json records every
transaction and observation.

The actual distributed surface470 integration corroborates the stage stall:
389.138s native at the unchanged388.8s stage budget,164commits,30500/39128wall cells
measured,1458known geometry and37size failures,273257unmeasured cells; all303757tets
positive,61494nodes. Zero complete global surface passes. q/J are UNKNOWN. This
is a preserved failure capture, not a successful reusable stage output.

The472 actual current residual diagnostic then separated constructor feasibility
from admission:24 bounded trials on two current residual stars and a native-measured
geometry-clean control. The control collapse10→8 is private-feasible; no residual
candidate passes every original guard. Historical rank5 clearance failure is NOT
reproduced. Its old private cell no longer exists. A control relocation refusal
comes from a diagnostic planar ConstrainMovePoint before mapped MoveVertex;
production SmoothBoundary uses mapped association/projection. This is a diagnostic
composition limitation, not evidence that production boundary mobility is absent.

Existing query tracing and eight exact support/coordinate replays isolate a
material admission uncertainty: the strict classifier can return a loose upper
bound immediately after finding an outside witness. The fitting merit then squares
that classification bound. Tighter valid certificates on ALL four child faces of
two same split candidates lower their complete-patch upper-bound excess below the
recorded old production score. This does not certify strict closure, physical
improvement relative to a comparably tightened old patch, or a published replacement.
See R/residual472_query_replay_assessment.json. Query precision is not a changed
geometry gate. The free current-heap-upper change does not address these root exits.

**Decision:** do not append another placement/split variant or launch another full
stage yet. The next connected correction must make old/fresh geometry-progress
assessment comparable, preserve cached/recomputed and census semantics, and then
retest this COMPLETE trajectory, including neighbor debt, tangential delivery and
first-row prerequisites. Fix the mapped diagnostic composition when testing
relocation. A candidate-only tighter query or an arbitrary larger certificate
ceiling is not yet a qualified production correction. If comparable assessment
still yields no useful complete replacements, reconsider coupled placement and
connectivity using the actual failed contexts; do not blame reference cavity
methods from this narrower operator composition.

## Evidence and corrected interpretation

466's fixed36-cell private cavity admitted20cuts/24attempts, geometrybad16→14,
corebad8→2/neighbors8→12, cells36→84,4inherited-volume inversion refusals. Geometry
excess fell36.39%, but no geometry closure or firstrow. Exact previous source and
receipts remain in staged57/surface_patch466_* and linked geometry-bound handbook.

An observation region need NOT be a single native topology ball. Each published
transaction does. Forcing all overlapping edge-star observations into one cavity
produced465's annular33-cell input and the later57/80-cell support unions. The latter
are NOT proven production limits or evidence that all smaller corrections fail.
Native production already queries/reserves/publishes current local transactions.

Source: CNativeDistributed3D.cpp Lookup/Fetch, Reserve fullvertices and Publish
owner/version+parameter atomic checks; CNativeEngine3D.cpp geometry split trial
routing around4127/4848/5016; CNativeBoundary3D.cpp SplitSurfaceStrip and
ReconstructSplitPatch2398–2514. InsertEdgeCavity on an unchanged completeedge star
still generates inherited split children. RefitWallSplit/ReconstructWallSplit start
with the same inherited split and do not alone escape an initial inversion.
ReconstructSplitPatch instead prepares a new positive cone and uses existing
ConstructFirstLayer/AdmitFirstHeight when requested walls are present.

##467 input/output and acceptance before build

Input: SAME saved CFD and full441 stage0 composed target; current451Owned/transported
nodal tensors and exact associations. h(stage0)=32h(final)=6.4e-5, final2e-6,
growth1.2/thickness.01, Mach-onlyL1, nominal500k. No upstream recomputation.
Reproduce the known four private perimeter cuts. Initial36cell observation is reused
for comparison, but no longer held as one aggregate transaction/reservation.

Expected output: bounded sequence of existing local native transactions, with exact
current dependency discovery, fullversioned support and per-step observation. Every
native input<=64, replacement<=128; aggregate observations are batched64 for
measurement and are NOT new larger cavity inputs. Same24constructor attempts,
300s shared geometry window/600souter/12GiB, oneheavyjob/nice19/libs1 remain.

1. Measure every current observed physical face, exact production planning wall-edge
   lengths and complete centroid q/J. Rank demonstrated outside-midpoint chords with
   existing certified distance lower bounds; sampling absence never certifies a face.
2. Lookup/Fetch the CURRENT complete selected edge star. Test untouched local native
   identity topology, then existing SplitSurfaceStrip. Allsource tensors are current
   published nodal fields, not original bootstrap data or stale precomputed proposals.
3. Only an actual inherited-connectivity inversion triggers the existing
   ReconstructSplitPatch alternate. Supply the EXACT requested height map and current
   target placement tensor; do not clear heights. If it names an obstructing face,
   discard the proposal and rediscover/fetch the full edge+face dependency union,
   within64, BEFORE a new complete reservation. Each constructor call consumes shared24work.
4. Certify complete current candidate parameters, topology/positivity, metric change,
   external clearance/body separation, passing-height preservation and unchanged
   provisional height assessment. Reserve ALL current support vertices BEFORE every constructor; reacquire the
   complete enlarged set on each support retry. Publish with the latest successful
   reservation and current dependency versions/geometry changes. No postreservation
   enlargement and no private context reuse after publication. Native publication
   failure must leave the published mesh and parameter state untouched.
5. Update observed cell identities and recompute the whole affected region after
   every commit. Grow observations to include newly affected neighbors; do not
   confuse this with increasing a cavity or granting acceptance. Accumulate q/J and
   geometry introduced-minus-retired from each complete old/fresh transaction, as local-change sums; they are not a separately certified global census. Thus
   changing observation scope cannot masquerade as measured local improvement. Raw regional counts
   and added old observation rows are separately reported.
6. Only if observed completeface guidance closes and wallplanningmax<=1.8, rebuild
   the full current field and existing constrained/blended directions, then use
   affected-vertex PhaseR2 with remaining work/time. Measure strict physical heights
   on every target face. These bits alone do not establish tangential suitability,
   alignment, full coverage, outer transition or final bidirectional certification.

Collapse removes unnecessary resolution only when its complete removed star,
geometry/features/heights and actual metric admission permit it. Relocation
redistributes positions on authority; reconnection changes obstructive connectivity.
Their need must be selected from the actual trajectory's measured obstruction;
467does not add an automatic larger operator ladder or short-edge minimum gate.
The immediate demonstrated inversion alternate is existing ReconstructSplitPatch.

Rejected constructor reuse is exact: serialized current Patch plus OwnedRows
(coordinates/associations/support/versions/sensors) and the full queried feature list;
fixed target/authority/heights remain invariant for this diagnostic. Only identical deterministic inversion/retained-skin visibility failures are
skipped; uncertain/resource, height, admission and contact failures are not memoized.
Cache bounded to at most24contexts and explicit pre-serialization scratch/cache
admission; incomplete or oversized keys disable reuse. No production
cache API or decision policy is added.

Stalls:24constructors, sharedtime/work/memory bound, incomplete/stale support,
local topology refusal, no demonstrated outside midpoint with unresolved face debt,
or all unchanged constructor contexts refused. Preserve exact reason and complete
patch observations. A local budget stop does NOT prove global infeasibility. If
progress requires repeated smallcuts but introduces neighbor failures, or requested
height/visibility cannot be met, reconsider coupled construction/support using the
actual failed context before another variant; do not raise limits or relax gates.

## Required trajectory and readiness (not commit-count success)

| Step | Input and operation contribution | Required output / stall |
|---|---|---|
| Restore | Saved CFD, complete441 composed profile/policy/authority and current451 nodal state; validate hashes and associations | Reconstruct identical target, stage height and complete current support. Missing/stale data refuses; never substitute sensor-only state. |
| Geometry closure | Measure complete physical-face guidance and unresolved witnesses; projected split shortens actual offending chords and updates adjacent tets | Recompute complete affected patch and neighbors after each transaction. Unresolved certification stays unresolved; local core improvement with growing neighboring debt is a strategy stall. Strict bidirectional certification is still required later. |
| Connectivity / placement | Existing reconnection rebuilds a positive volume replacement when inherited split children invert; bounded face-neighbor discovery repairs proven visibility obstruction. Relocation changes placement on the authority | Complete final cavity reserved with fresh versions before construction/publication, positive cells and valid interface. Repeated identical failure, missing mandatory support or capacity exhaustion records a stall. More splitting does not fix a demonstrated visibility/placement obstruction. |
| Tangential delivery | Exact production edge measurement plus directional surface restriction; collapse removes admissible redundant points, relocation/reconnection redistribute or unblock connectivity | No overlong edges; report directional conformity and short-edge concentration separately from physically necessary around-nose refinement. Passing maxedge alone is NOT suitability. If no admissible cleanup/correction exists, save the actual failing star; do not force short-edge or shape gates into construction. |
| First-row readiness | Rebuild constrained/blended direction data after changed surface/connectivity. Use coordinated wall child/transition transactions, checking the complete patch and neighbors | Measure physical stage height, certified fluid clearance, wall coverage/alignment, carrier tangency and direction continuity; preserve previously passing heights. Failed prerequisites mean firstrow UNREACHED, not a partial success. Height/footprint debt allowed provisionally is clearly distinguished from strict readiness. |
| Handoff | Save complete current mesh/parameters/transported metrics, unchanged profile/authority/policy, continuation stage and provenance | Only credible surface+firstrow output advances to growth. Otherwise retain failure evidence; no successful stage checkpoint or downstream layer claim. |

Bounds are shared across the trajectory, not restarted for each refusal. At24calls,
300sgeometry/600souter,64old/128replacement or memory denial, preserve the last complete
published state and exact failed candidate. No partial publication. A bound stop alone
is not impossibility. Reconsider strategy when the measured required operation cannot
produce an admissible complete replacement, or neighbor debt/short-edge proliferation
prevents readiness despite useful core cuts. Select ONE demonstrated obstruction;
do not append isolated variants until a fixture passes. q/J remains observational
through construction, with full strict closure mandatory before acceptance.

## Qualification scope

Runtime must preserve all303622original Owned records independently and all original
accepted parameter payloads. Reconstruct+split alternatives are provisional until
complete-patch effects and strict downstream constraints are demonstrated. No q/J
construction ranking or quality veto. Every final gate remains unchanged.

This diagnostic is serial (the existing loader explicitly requires Worldsize1).
Directory primitives retain their native collective/version contracts. A useful
production integration must use the existing engine's collective trial/election and
empty-rank alignment, not copy a rank-local loop into MPI. Dedicated cross-rank
reservation/growth/stale-dependency/constraint/resource qualification is still
required at the meaningful integration milestone. No MPI capability from this test.

Timings: separate complete input reconstruction, Directory work, constructor work,
measurement/certification, CPU/wall and observation-scope changes. No direct speedup
ratio against466: the observation/front and node transport trajectory now differ.
No newperf/GDB/baseline campaign; evidence/counters and actual saved M6 inputs only.


##467 terminal outcome and468 targeted checkpoint reconstruction

467buildPASS299.827s; runtime267.942swrapper/260.212stest/609580assertions/1763pins,
allactualhandles terminal/all303622Owned unchanged451. All24calls were successfully
published SplitSurfaceStrip transactions, not reconstruction trials. Observation
36→139 included36newlyobserved oldcells, so geometrybad16→48 cannot be interpreted
as pure churn. Sum of local published geometrybad deltas +12; q/J +67/+67. Excess
fell in an expanding scope; no closure/firstrow/tangential suitability follows.
No useful correction selected from24commits. R/surface_transaction467_assessment/
outcome/source_snapshot.json. ProductionCFD unchanged451.

The broader originalchord priority did not reach the actual466inversion. That is a
test-coverage gap, not evidence against ReconstructSplitPatch.468uses a pinned
DIAGNOSTICreplay file generated from preserved466evidence and unchanged451Owned:
16successful prior edges, eachprojected point expectedcoordinate, then the failing
4217–3879270complete star (2tets/5points/2physicalfaces). Native code reacquires all
CURRENTdeps and checks exacttopology/coordinates/physicalfacet identities before
requiring the native inheritedsplit refusal and invoking the coupled alternate.
It does not copy old projected coordinates into proposals or assume projection
idempotence: fresh projection results must MATCH the saved geometry. Current nodal
metrics are transported afresh per publication; no bitwise historical tensor identity
is asserted across the different transaction grouping. Same savedCFD/target/heightlaw.

All16replay constructors count within24. The remaining calls continue existing live
cavity discovery/validation/publication. Exactcase matching plus a reached coupled
alternate is required before the diagnostic is qualified. This fixed precursor is
not production selection/scheduling, a new ADAPoption, an accepted stage checkpoint
or authorization to dispatch from case identity. Reuse all existing complete target,
geometry authority, Owned/parameter and validation helpers; no baseline/perf/GDB run.
R/make_surface_replay468.py and surface_replay468_provenance.json preserve input hashes
and generation checks. Native input parser bounds counts and refuses malformed data.

467trajectory89.894379wall/89.498557CPU/3252109geometrywork is nested in95.849445s
prefix; runtime includes geometry/target reconstruction. SourceDirectory timers are
not individually exported by this diagnostic: constructor/Directory attribution is
a gap, not inferred MPIwait. SerialCPU includes library calls/polling. No467vs466
speedup ratio: workfront, observedscope and transport path differ. Sampledindividual
RSS2.287GB/time-processmax2.324GB are not whole-job peaks.

##468 outcome and469 connected hypothesis (before qualification)

Exact replay reached actual split inversion and reconstructed support2→3 BEFORE a
new reservation. Visibility then passes; raw flat-centroid first-row placement fails
positive curved-reference clearance. Negative clearance lower bound is uncertainty,
not a proven penetration. Outcome:24calls/21commits, observation36→97 (8oldcellsadded),
geometry16→22, local geometrydelta+1/qJ+53/+53. Firstrow remains UNREACHED.
Trajectory72.725633wall/72.714155CPU/1658670geometrywork, nested79.084899prefix.

469tests one shared correction: retain raw placement if currently admissible; otherwise
reuse existing projected-foot/physical-normal proposals with positivity and current
clearance/height admission. Final whole-cavity geometry, interfaces, current parameters,
metric/contact and passing-height preservation remain mandatory. No constrained column
direction replacement, q/J construction gate, new operator, config or larger bound.

Success requires an actual complete published replacement on the SAME replayed M6
failure, then continued complete-patch trajectory toward geometry closure, tangential
readiness and constrained firstrow within the SAME24calls/300s/600s/64old/128new bounds.
A successful primitive alone is insufficient. If placement still refuses or neighboring
geometry proliferates without readiness, retain the exact failed context and reconsider
the coupled placement/connectivity strategy; do not add another isolated variant.
ValidityOnly may retain measured height/footprint debt, NEVER uncertified clearance.
Final requested physical height/coverage/alignment and all strict gates remain OPEN
until measured on generated complete output. No MPI claim until useful serial output.

##469 terminal: complete trajectory rejects the isolated placement hypothesis

Build582.181s; focused14cases/2172assertions pass. Actual249.922swrapper, all5actual
handles terminal/1769pins/303622Owned unchanged. Exact replay and every one of22
published observation states match468 counts, excess and edge metrics.24constructors,
21commits; geometry16→22 in36→97scope(+8oldcells), localgeometrydelta+1/qJ+53/+53.
Geometry closure FALSE, tangential suitability UNPROVEN, firstrow UNREACHED.
Trajectory72.679326wall/72.643961CPU/1657690geometrywork; no useful progress gain.

The experimental shared physical-normal fallback resolves the earlier rawpoint
clearance refusal but the SAME3cell reconstruction then fails another wall child's
positive cone: facet10702/interface8076,3879271,3879273/apex3879275. This demonstrates
that independently finding a fluid-side wall point is insufficient for the coupled
replacement. It does NOT prove a joint candidate or a bounded support correction
impossible, nor establish a physical geometry violation. Rejected replacements have
no qualified q/J, metric, contact or height outcome; those values are UNKNOWN.

After independently verifying terminal handles and all frozen pins, exact experimental
sources/patch were archived and all three edited files restored to build468 hashes.
No production change integrated. Current469binaries remain unqualified experiment;
rebuild restored sources before further runtime. No MPI campaign: complete useful
serial replacement was not demonstrated. Receipts: physical_row469_disposition.json,
physical_row469_unqualified.patch, physical_row469_experiment_source/ and
surface_transaction469_assessment/comparison468.json under staged57.

RECONSIDER now: the whole wall-child/transition construction needs compatible
placements and connectivity over the complete affected surface/volume cavity,
rather than committing the first locally admissible apex then discovering that the
next one cannot fit. Audit/test existing coordinated facilities on this EXACT
obstruction. Joint feasibility remains a hypothesis, not authorization for another
local offset, more cuts, larger cavities, new scheduler or raised limits. Preserve
all already-passing geometry/features/heights and final gates; until a complete
replacement is demonstrated, no usable surface/firstrow checkpoint exists.

##470 phase-consistent surface reconstruction — acceptance before implementation

Source audit: engine Adapt enters existing ValidityOnly admission at1167; its height
check accepts fully measured provisional height/footprint debt but no missing child
or uncertified reference clearance. Surface candidates additionally protect all
already-passing rows. Ordinary surface splits use those conditions. In contrast,
ReconstructSplitPatch always calls ConstructFirstLayer before returning even an
otherwise positive geometry-construction cone.469's attempt19 reaches that forced
firstrow step before geometry closure; independent row placement then blocks volume
connectivity. ConstructWallRegion is not a mapped-M6 substitute: its general path
requires complete analytic cylinder/sphere authority (or its special prism chart).

Hypothesis: geometry preparation is unnecessarily coupled to premature firstrow
construction. Reuse existing provisional admission on the complete positive cone,
with the EXACT stage height map and passing-row preservation. Only ValidityOnly
may defer carving; absolute callers and any nonadmissible/protected cone retain the
existing constructor. Recheck admission/passing rows after restored retained points
and final Finish. No empty height map, altered requested stage height, new phase,
config, CAD dependence, direction-field change, q/J veto or relaxed final gate.
Actual firstrow construction remains required after surface readiness.

Input SAME451Owned+complete441target+exact468replay. Success requires complete
3cell replacement with positivity/topology, current associations, metric admission,
external clearance and preserved passing heights; measure actual geometry/qJ/edge
changes on that COMPLETE patch and neighboring observations. Continue SAME24call/
300s/600s/64old/128new/12GiB trajectory toward geometry/tangential closure then
existing constrained-direction/firstrow readiness. Fixture-only success is not
integration. Refusal or unchanged published progress rejects this hypothesis;
uncertain height/geometry cannot become admitted. MPI only after useful actual
serial replacement. All later strict BL/RANS workflow obligations remain open.


470qualification and actual progress: focused21cases/4077assertions pass; exact M6
replay24calls/22commits. Attempt19grown3cell support constructs10positivecells and
passes metric/parameters/contact/height-preservation/reservation/publication with
EXACT heights, provisional measured debt. Geometrybad3→3, excess5.169003664825083e-7
→4.913117750796659e-7; q/J3→10. No firstrow certificate. Current trajectory still
nonclosed:36→110scope(+11old), geometry16→24, localdelta+2/qJ+63/+63, firstrowunreached.
74.886533wall/74.866628CPU/1590930work; no like-for-like speedup or wholefieldchurn claim.

MPI32cases/everyrank155.969swrapper passes changed local admission/strict-row tests,
firstrow/corner engine constraints, and actual saved-M6 ownership-growth/conflict/
emptyrank/stale/cache control. The actualnew3→10case remains serial evidence; MPI
control is existing coarsening correction, not claimed same-cavity publication.
One full same-target distributed surface-stage attempt now tests production integration
under unchanged388.8s/233220work budget. Consult HANDOFF for live owned handles. No
finalgeometry/shape/height/BL/RANS workflow completion from these checks.


##470 actual full surface-stage terminal outcome and next evidence

Fullstage R/cases/curved470_phase_consistent_saved_target_surface_mpi6_20261113T013323_1333069
terminates normally at389.138snative/498.917swrapper.1740pins/all10actualidentities/
all303757positive tets/1153534exactsharednodalchecks verified.61494nodes,164commits/
3676globalwork;30500/39128requested-wall cells measured, ranks0–3 complete measurement,
ranks4/5 incomplete. Known1458geometry/37sizebad; q/J entirelyUNMEASURED. No global
complete surface pass, tangential readiness or firstrow. Target/CFD unchanged.
The six complete captures preserve Owned/donors/parameters, but are diagnosed failure
outputs, not a qualified credible successful stage checkpoint or adapted restart.

Compared with451coverage13779/39014at same388.8s, more service is demonstrated;
rawbadcounts on different measured/evolving sets cannot demonstrate closure or churn.
Geometrywork30.089M, rank4/5~25.970M dominates. Rank5guidance1124refusals/1157trials;
last rank4edge18152–18153 geometry-preservation refusal; last rank5feature2:8010
uncertified clearance at facet10649cell5930309lower/h0=-1.404292. No proven penetration
from that lower bound. Ranks0/1/3 have no cached geometry debt; rank2 has522knownbad
with its wall measurement complete. Resource/time stop leaves feasibility undetermined.

Next compare actual remaining contexts with full support/associations/current nodal
metrics and a clean control. Classify true geometric regression versus conservative
certification/association/height/topology/support limits before selecting another
connected correction. Existing background search still considers raw wall-point
proxies; whether this obstructs otherwise admissible geometry cones is an UNPROVEN
source hypothesis, not an authorized new variant. Do not simply lengthen the run,
raise cavity limits, waive old-row clearance or append placement offsets. Whole-patch
and neighboring-cell effects must drive geometry closure and eventual firstrow readiness.


##471 current residual reproduction — scope before build

470actual last-private cell5930309 is absent from all six saved Owned shards. Current
edge18152–18153 exists with2rank4cells; full removed18152star10cells.8010has16rank5
cells and six incident requested physical edges (other edges of its surface faces
are not incident demands). Python inventory preserves versions/coordinates/current
raw nodal tensors/physical facets but does NOT infer geometry associations or clean
control. All six5,075,160Bparameter payloads are BITIDENTICAL; Directory globally
publishes associations. Existing native loader therefore uses the full current
rootparameter state with all six Owned shards and unchanged complete441target.
R/residual471_inventory.json/entry.json are inventories, not certificates.

One TEST-ONLY selector reuses existing complete-cavity planning/operator preflight
for current rank4star→18153, current8010star and a dynamic smooth-wall control.
Native identity topology is checked before judging failures. All incident8010wall
collapse directions are compared; existing bounded growth/reconnect/reduction and
surface split/joint reconstruction remain within64old/128new/24totalconstructor/
300ssharedgeometry/600souter/12GiB limits. No production policy or gate changed.
This diagnostic does not reuse a vanished private candidate or presume unchanged queued
state. Control is called clean only after native measured zero excess, not by region.

Complete old/fresh patches are measured with existing exact SurfacePlanning edge
quadrature, current nodal metric/associations, geometry guidance, positive topology,
passing-height/provisional-clearance checks, final support reservation, parameter
preflight and external contact. q/J are centroid observations, not construction
ranking. No publication and no full composed-volume edge certificate is claimed.
Coarsening retains production's zero total candidate guidance-excess rule; geometry refinement needs
measured descent. All resources/unknowns remain refusals. Original Owned and parameter
payload must remain byte-identical. This is failure classification before ONE connected
hypothesis, not a new primitive variant, finalmesh acceptance or pyAMG equivalence.


471build passed297.809s but no runtime was launched: review identified two hidden
candidate paths outside the new counter (existing CorrectCollapse planner and
prescribed-interface control CollapseVertex). All10actualbuild handles terminal and
sourcepins verified before edits; exact source archived in residual471_unqualified_source/.
472disables these two paths only for current residual diagnostics, retaining bounded
explicit growth/reduction callbacks under24cap and native untouched-input topology.
Discovery exceptions are recorded separately as search refusals, not evidence that
all operators fail. Productioncode/CFDbinary470 unchanged. Currentbuild472 is FROZEN;
readHANDOFF for handles. This test-only classifier does not need another broad/MPI campaign; actual bounded
serial evidence must first select a useful connected production hypothesis.
