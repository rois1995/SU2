# Connected curved-wall BL milestone

User steering2026-11-09. This is the next implementation milestone AFTER frozen
FIRST296 terminated and actual owned handles were verified terminal. Connected BL
source implementation is built302/303; serial303 passed, broad MPI303 terminated
at its600s cap with failures. Current outcome/control work is recorded below.
Actual curved-M6 policy probes passed, but no BLmesh generation or RANSCFD yet.
Preserve qualified Euler construction/certification,
all gates/budgets and the completed293 two-event/restart baseline/checkpoints.
Small296 timing changes, reuse counters and speculative geometry optimizations do
not justify another Euler cycle. Only clear material outcome evidence can change
this ordering. One active implementation/validation cycle and one heavy workload.

## Missing capability and evidence

Actual M6 uses the original no-CAD cubic authority: preflight296 pins48954curved
nets,8094affine facets and zero analytic components. Current
CBoundaryLayerMetric::FromNativeReference rejects nonempty Geometry().Curves(),
which is the CurvedNets map. Its existing test expects refusal. Removing this guard
alone would feed planar facets to distance/normal, integration support and chord
ownership; that is not curved-wall support. See staged57/m6_bl_preflight296.json.

The connected capability is one retained curved wall authority used consistently
for distance/normal queries, combined flow/BL target evaluation, complexity
quadrature, chord integration, wall-aware construction and constrained repair.
CAD stays optional. Exact association/body/sheet/marker identity, invalidation,
work/time/storage admission and conservative uncertainty handling remain mandatory.

## Existing facilities to reuse and audit at their production callers

- CBoundaryLayerMetric.cpp2095+ converts retained native walls; EvaluateImpl1794+
  supplies existing normal/tangent/D1/fade/intersection laws. Wall.authority currently
  means one complete analytic component. Curved source support and semantic wall
  components must be retained instead of silently treating a cubic wall as faceted.
- MappedFacetReference::EvaluateSurface and ClosestCurved provide existing cubic
  evaluations, local differentials and distance enclosures. GeometryBackend3D::Project
  explicitly makes no globally closest/unique-foot claim: do not use a numerical
  proposal as proof of complete wall distance or hidden-band exclusion. Preserve
  full allowed support and competing-wall/component ownership. Query precision must
  resolve requested h0; a Hausdorff tolerance much larger than h0 is insufficient.
- CSolver::ComputeMetric, CSinglezoneDriver and CNativeBLTarget3D::MakeTargetPolicy
  all call the builder. IntegrationRule3D/IntegrationColumns3D/EdgeBreakpoints and
  ApplyPointOnChord must use the same curved policy as actual remesher point queries.
  Existing faceted/analytic support clipping alone does not cover cubic bands.
- Existing positive quadrature, original-cell sensor barycentrics, target-density
  error estimates, bounded hidden-support refinement and deterministic chord
  ownership are starting points. Preserve their convergence/resource gates; do not
  claim numerical error indicators are rigorous bounds. No nodal interpolation of
  an unresolved BL band and no sampling-only geometry certificate.
- BLFirstRow9, BLGrowth12, BLVolume13 and BLEvent15 have actual first-row/lift,
  direction-field, layer-transaction, joint-wall and outer-volume repair facilities.
  NextLayer advances one combined-metric unit and has thickness/visibility/size
  stops. This is not evidence of complete multilayer extrusion or universal coverage.
- BLVolume13 freezes some first-child/fan cells/vertices during private optimization
  and keeps registry identities. Audit lifetimes and the shared-stage handoff:
  temporary coordinated construction is allowed; permanent frozen columns/boundary
  coordinates and mandatory prism connectivity are not the production design.
- General native height/geometry checks and ordinary split/collapse/move/reconnect
  plus directory reservation/publication/migration are the shared machinery.
  CNativePartition3D preserves OwnedCell envelopes, not a serial row registry;
  copying cell ownership alone does not prove continued BL constraint coverage.

## Acceptance conditions and what remains unproven

Metric q/J is distinct from BL suitability. Before declaring a BL capability,
produce a current-mesh census of the following, with support/marker/body identities
and explicit feature/junction classifications. Do not infer these from successful
metric construction, a row count, or quality alone.

| Property | Existing enforcement/evidence | Required verification |
|---|---|---|
| First height | HeightConstruct/CheckFirstHeight/Admit12 and first-child checks exist | Every requested wall face has a known valid first child/foot on the prescribed authority within the existing height gate; cached/recomputed agree |
| Normal alignment | Direction-field constraints and analytic normals exist | Measure constructed displacement/cell directional alignment against curved wall normals; closest-foot definition alone is not an independent alignment test; identify justified edge/junction transitions |
| Progression/coverage | Metric normal profile, NextLayer and stop witnesses exist | Measure current normal spacing/progression, coverage through the requested band and outer transition; report stopped/uncovered/unresolved regions, not only registered rows |
| Tangential resolution | Flow/BL intersection and tangential samples exist | Production surface restriction and exact edge quadrature show conformity to the combined target; no weakening of flow demand through an unintended legacy tangential floor |
| Features/junctions | D1 policy, constrained directions and targeted joint-wall repair exist | Actual leading/trailing edges, root junction and competing-wall cases; explicit supported transitions, no universal perfect-orthogonality assumption |
| Geometry/volume | Positivity/topology/body/association checks and strict final geometry/q/J/size gates exist | All unchanged final gates pass on the complete affected mesh, with no unknown measurements; separate bodies remain separated |
| Continued adaptation | Several local height-preserving operators exist | Re-run the BL census after splitting, coarsening, relocation, reconnection and ownership migration, with complete dependency invalidation and empty-rank alignment |

No new arbitrary universal angle/progression gate is chosen here. Define each
measurement against the actual requested profile and geometry; qualify justified
feature transitions explicitly. The existing strict first-height, geometry1e-4,
q>=.2,J>=.05,maxedge1.8 and body/validity gates remain unchanged. A larger number of
rows or a visually layered patch cannot substitute for coverage and transition
checks. All-case acceptance remains unproven until actual curved M6 is exercised.

Account for minimum BL resolution when normalizing to complexity. If requested
heights/profile and the combined flow demand make a complexity target infeasible,
report that evidence; never inflate/weaken h0 or silently discard demands to fit
50k/500k. Actual point/tet counts are separate from metric complexity.

## Qualification and actual workflows

Implement one connected capability across the callers above. Focused checks during
development must cover true cubic walls and seams, genuine features, junctions and
close separate bodies, thin curved bands missed by coarse vertices, chord ownership,
quadrature convergence and resource refusal. Reuse passing relevant provenance.
Then ONE connected MPI milestone: curved walls/junctions, empty ranks, stale
neighbours/associations, cached/recomputed agreement, all requested heights,
post-operation/migration BL census and resource exhaustion. Do not repeat MPI for
each small edit. Return promptly to ACTUAL M6 BL; analytic fixtures alone cannot
close the milestone. Keep construction validity-first and final gates strict.

Then RANS500k uses a saved reasonable RANS solution, followed by adaptation,
conservative admissible flow AND turbulence transfer, finite continued CFD,
second adaptation and an actual isolated adapted disk restart. Independently
check turbulence projection and any solver-bounds correction ledger; do not call
post-clipping turbulence integrals exactly conserved. No Euler/RANS label dispatch
for reconstruction: actual geometry, metric and requested wall constraints drive it.

Read-only seed inventory found one real M6 RANS/SA120-iteration checkpoint at
integration_evidence/native_3d_bl_v1/oneram6_coarse_v1/flow_checkpoint/restart_flow.dat,
on the61330point/303359tet Coarse.su2. Its last screen sample119 has CL.157317/CD.053009;
iterations80..119 span CL.152913..157317/CD.046823..053813. This is an initial-guess
candidate, NOTa qualified reasonable/converged RANS seed. Inspect configuration,
field/mesh correspondence, positivity and force stability before qualification;
continue from it when appropriate instead of recomputing from scratch. Residual
convergence is not itself the user's sole criterion. Nearby original mesh directory
contains matching meshes but the bounded inventory found no alternate solved RANS
state. No RANS CFD was launched during frozen296.

## Working implementation — unbuilt, not qualification

The active working change retains one cubic same-marker connected component per
wall contribution, shared requested profile, and exact original facet order. Point
queries use existing certified ClosestCurved with h0-scaled absolute error and
existing bounded work/storage; barycentric tangential interpolation uses the actual
returned binding. Component-aware marker distance prevents last-component lookup
from silently choosing a different disconnected wall. Planar triangles provide a
Bernstein control-deviation envelope for broad-phase integration support only.
Whole-component distance lower bounds and the distance function's 1-Lipschitz
property conservatively screen hidden bands and bracket chord crossings; unresolved
queries/crossings throw at existing limits. Analytic/faceted paths are retained.
No new curvature metric or configuration is introduced. These claims describe
intended source behavior, not passing evidence. Closest-point candidate normals are
local differential evaluations, not certificates of a unique foot. Actual query
work, hidden-band convergence and M6 feasibility remain to be measured. BL current-
mesh suitability census, operation/migration preservation and RANS remain unproven.

### Build297 source review and remaining binding gap

Build297 freezes the six changed production/test files under build297_source_pins
and builds production CFD plus test_driver at nice19/j3 with resource receipts.
Actual host identities are in build297_owned_startup.json; verify terminal before
editing any pinned source. Euler296 source/binaries remain independently archived.
Read-only review found WallBounds missed cubic controls (fixed), and query admission
used the entire ceiling without live BL storage (fixed). Attached atlas reservations
are now respected and attempted query work charged even on failure. HOWEVER, the
production retained policy is constructed before the engine's private reservation;
that view does not reach captured full/base layers. The current per-query and outer
loop limits do not establish shared event geometry-work/deadline accounting. Connect
that existing reservation across normalization/construction/improvement before
calling this capability production-qualified. No deadline/quota increase authorized.
FIRST_ROW currently explicitly requires ONE native worker; general later stages
are distributed. This is an existing limitation, not qualified MPI BL generation.
The actual frozen-M6 diagnostic NativeFrozenBL14 accepts SU2_DC14_CONFIG/DONORS/OUT/
EVENT and uses production ComputeMetric/backend. Prefer it to invented analytic-only
campaigns when the affected focused behavior is sound. Actual M6 feasibility and
current-mesh BL suitability remain open.

Build297 terminated1 after522.606s, pins unchanged and all11actual host identities
terminal. Wrong geometry namespace and ambiguous test tensor prevented compilation;
no runtime result claimed. Six changed source files preserved in
staged57/build297_source_snapshot. Build298 corrects those names, keeps cubic
integration support outward rather than shrinking its envelope, and includes this
wall's D1 feature contributions in the chord uncertainty tensor bound. Source
frozen in build298_source_pins; session88564/actual identities in
build298_owned_startup.json. No BL runtime evidence yet.

### Runtime findings298/299 and current300

298 built successfully (64.161s, allpinsunchanged/8actualhandles terminal). Its
focused check passed21/22cases and281699assertions; the new cubic case failed
because the builder assigned native-memory0 literally instead of retaining the
existing default2GiB. This was corrected without changing any configured limit.
299 built successfully (42.125s/allpinsunchanged/14sampledhandles absent). Focused
299 terminated2 after62.691s, allpinsunchanged/exactactualhandles+wrapper absent,
21/22cases passed and281740assertions passed. Independent polynomial surface point
exposed a false near-wall normal: a closest candidate within the certified distance
error had tiny tangential residual, mistakenly normalized as the physical normal.
The new chord resolver also correctly refused total crossing uncertainty beyond1e-8;
per-leaf allowance alone did not bound the sum over multiple crossings.

300 fixes BOTH in the same ongoing connected cycle: use the authority differential
normal when distance lower bound cannot resolve separation from the wall; refine
crossings until summed metric uncertainty<=unchanged1e-8, still counting all passes
against original16384work. Not a tolerance increase. Frozen build300 session98477,
helper3490902/start28178881,ninja3490904/start28178899. No runtime300 result yet;
no MPI/actualM6BL qualification claimed. New true cubic hidden-volume and competing
component tests, shared event reservation binding and current-mesh suitability
census remain outstanding. Preserve failures; do not launch MPI prematurely.

### Demonstrated chord blocker300; current frozen301

Build300 PASSED/44.150s/all14sampledhandles absent. Focused300:
`cases/geometry_backend_20261109T151657_3493673`, tool46484 TERMINAL1/464.222s,
pins unchanged, all4exactHOST PID/start identities and wrapper3493673 absent.
21/22cases passed,281741/281742assertions passed. No near-wall normal assertion
failed. Cubic chord refused at unchanged16384interval work. This is actual bounded
runtime failure, not a speculative optimization opportunity. The restart-from-root
uncertainty loop repeated identical previously certified/excluded intervals.

301 keeps an admitted max16384 unresolved-leaf frontier and refines the largest
uncertainty, replacing parent error by children with outward arithmetic. Excluded
leaves remain excluded only within this synchronous frozen wall/profile/chord.
Existing AffinePointBounds bound exact chord midpoint/endpoints; radius to both
endpoint boxes covers the complete ideal affine interval by convexity. No scale-
heuristic coordinate padding. This should reduce duplicate queries and unnecessary
uncertainty, but benefit/correctness are not established until runtime. Total
uncertainty remains<=1e-8, intervalwork16384/cuts65536 unchanged. True coincidence,
tangency and representability floors must still refuse at existing limits.
Frontier admitted before reserve; live frontier/levels/cuts/set scratch is included
in query admission, and cut/set growth is admitted before mutation. Existing
attached atlas reservations receive peaks/work/deadline checks; missing production
reservation binding described above remains open. No new config/scheduler/CAD.

Build301 frozen session30473, helper3519596/start28249637,
ninja3519598/start28249654. Verify actual build301_owned_startup.json handles
terminal before any compiled/helper edits. No301runtime/MPI/M6BL claim.

Build301 PASSED328.339s with unchanged source pins; all11exact actual build handles
terminal. Focused301 is CURRENT FROZEN session60253 in
cases/geometry_backend_20261109T153336_3533357. Actual identities saved in its
owned_startup.json; same600s cap. No terminal outcome yet. Source review found no
concrete arithmetic/admission flaw, not a substitute for runtime evidence.

Reservation follow-up: normalization proposal already receives rank-local `within`,
ceiling and caller_bytes in CNativeNormalizationStage3D.hpp::job.propose; use that
existing scope for retained-layer queries rather than changing the factory API or
using its `time()` wrapper (which performs collectives and MUSTNOTbe called from a
rank-local geometry traversal). EngineOptions can carry a narrow optional borrowed
reservation binding callback/token for retained full/base; bind existing engine
AtlasBudget after initialized allowance, restore prior pointers before stack budget
destruction, including exceptions/nested contexts. FIRST_ROW has its existing ctl
budget/deadline/live StateBytes scope. Initial CSolver ComputeMetric is a separate
scope from the native engine: preserve existing enclosing runtime/sample/memory
limits, do not invent an unaccounted full event allowance or pretend an engine-only
binding covers initial target normalization. All this remains unimplemented.

### Connected reservation integration302 and actual-M6 policy evidence

302 builds10changed C++ files at nice19/j4/libs1, PASSED280.352s, unchangedpins;
all83sampledactualbuild PIDs absent. Initial metric setup reserves bounded query
workspace inside its existing native ceiling and binds a rank-local deadline/work
scope. The complete Remesh event shares ONE AtlasBudget across retained full/base,
source preparation, reassessment and normalization. Engine borrows the same ledger,
restores caller/byte/integer/work windows on exit but retains attemptedwork/sticky
failures/peaks. FIRST_ROW receives that ledger and originaltotal ceiling, reserves
queryscratch from state allowance, and admits state/facet/marker/input copies BEFORE
allocation. Source normals/support/metric decisions unchanged by budget binding.
All callbacks are rank-local; callback install failures use existing collective
failure election. Automatic existing publication-memory synchronization updates
both state and total reservation windows; explicit configured limits stayfixed.
No new scheduler/config/time/work allowance. This is source design, NOTMPIproof.

Nested scope restoration, elapsed/work/byte refusal and cached/fresh agreement
checks added. Reimport's expected donor growth was stale: unchanged
CNativeImport3D.cpp1135–1163 creates one donor per CFD simplex. Archived296 binary
with its pinned old source/runtime reproduces48>48 (controlv2,0.021stest/1.877shelper,
11assertionspassed/1failed). Firstcontrolv1 abortedMPI_Win_create beforeassertions;
v2 reapplies existingOMPI_MCA_osc=pt2pt/4TBscratch. Both preserved. New test verifies
same count, EACHsourcecell/nodeID/coordinates, unchanged cubic controls/state and
recomputed authority distance. Geometry and acceptance gates are not weakened.

Actual probe302 PASSED74assertions,20.436shelper/16.466stest, pins/inputhashes
unchanged/all5sampledhosthandles absent. Case:
`cases/actual_cubic_bl_20261109T162051_3649913`.
Inputs`m6_bl_inputs/curved_policy302_v1/provenance.json`: actual original no-CAD
referenceSHA2e99fb...42de/48954nets, physicalh0=2e-6,g1.2,T.01/Hausd1e-4. Mach-onlyL1,
nominal500k input, FIRST_ROW1worker, native querytime300s diagnostic bound unchanged
from source config,12GiBfixedcurrentcap. No CFDseed is read and NOflow target is
computed/normalized; the probe uses identity flow tensor for composition checks.
This does NOTqualify flow allocation, BL coverage/generation or RANS500k workflow.

Probe stages: reference6.596s, policy6.875s/23,456,436storagebytes; actualnearwall
6.952s/693work; chord6.975s/924work/2cuts; complete42-point tinycell16.333s/96750work.
Peak4,625,561,376bytes is the admitted/reserved model (including loader/cachequota),
NOTRSS; /usr/bin/time max332716KiB is oneprocess, not wholejobpeak. Wholeprobe used
16.36CPU/16.78wall seconds, so no contention-based speedup claim. Forward geometry
supplies the M6 point/normal oracle; independent polynomial fixture supplies the
separate truecurved distance/normal check. Hidden volume/competing curved components
and actual firstrow/migration suitability still require qualification.

CURRENT frozen focused302 session93759:
`cases/geometry_backend_20261109T162533_3661553`. Ownedstartup records exactPID/start;
600s cap. No terminal outcome yet. Do not edit pins. After affected focused checks,
ONE connected MPI milestone, then promptly actual M6BL generation using savedreal
CFD; analytic/unitpolicies are not the next capability milestone.

Focused302 PASSED22cases/283883assertions,303.957s, allpinsunchanged/all4exacthost
identities+wrapper terminal. No BLmesh/generation/coverage or currentRANSloop claim.

Current test-only303 build session44439/helper3823881,start28918849;
ninja3823928,start28918964. Only2tests changed; assert productionCFD79c4d51b... stays
byteidentical. New NativeSharedGeometryBudget3D checks completeevent/engine ledger
identity/monotonicity/restoration and absence of borrowed budget on published
reference. Affine-equivalent cubic cube/identitymetric isolatesplumbing; requested
h0=1 is installed/enforced, no actuallayergeneration claim. No assertions/workcharges
in noexceptshared-pointer teardown. 303 M6probe fixes query direction to FLUID side
(-modeloutwardnormal), signed alignment, positive localcellorientation.302was an
unsigned-distance policy check on the opposite side; notfluid-domaincoverage.
No303runtime result yet; do noteditpins untilactualterminalverification.

Read-only savedRANS assessment is staged57/rans_seed302_readonly_assessment.json.
All61330coords exact/coarseSHA8c8391..., all9fieldsfinite,ρmin.22430336,
ρe_min155839.804;νtilde_min1e-16/max.0003146127. All19506floorvalues areWINGnodes,
noneoffwall; no historicalclippingledgerclaim. Savedconfigacd22ae3... and restart
3b409077... matchprovenance. Safe admissibleinitialguess, NOTyetreasonable/converged
flow/currentbinaryrestartqualified. CL80..119+2.88%,CD+13.2%;110..119CL+.83%,CD−1.49%.
Reuse exactcopiedcheckpoint with unchangedphysics/nondimensionalization for bounded
continuation whenBLqualification permits; do notstartfromfreestream orsubstituteEuler.


303 TEST-ONLY build PASSED181.151s/pinsunchanged/exactbuildhandles terminal,
productionCFD byte-identical30279c4d51b587f... . Shared event/engine ledger fixture
PASSED34061assertions/1case (0.060stest/4.346shelper), requestedcubeh0=1 enforced,
noactuallayergenerationclaim. Identitysyntheticcomposition separatesresourceAPI
plumbing fromthealreadytestedphysicalCBCpolicy. Publishedreference has no borrowed
AtlasReservation; nested engine and event use SAMEledger. Constructor uses1native
worker soMPI6milestone also exercises idleparentalignment. No throw/workcharges in
shared-pointer teardown; state snapshots/restoration only.

Fluid-side actualM6probe303 PASSED88assertions (27.344stest/31.390shelper), allinput/
source/runtimepins unchanged/all5observedPIDsabsent. Case
actual_cubic_bl_20261109T172504_3838079; nativeh0=2e-6/g1.2/T.01/Hausd1e-4 unchanged.
Physicalquery -outwardnormal, signednormalalignment; positiveoriented localcell.
Reference6.708s,policy6.991s,fluidpoint7.111s/1233work,chord7.150s/1644work/2cuts;
56-point completecell27.232s/216726work. Loader/admittedpeak4625561376 NOTRSS;
oneprocess /time max332492KiB,27.20CPU/27.68wall. NoWholeM6mesh/flowclaim.

Serial303 PASSED107cases/544736assertions,334.570s; allpinsunchanged/exactowned
handles and wrapper absent. Case serial_20261109T172624_3841227.
MPI6-303 TERMINAL124/605.346s at600scap; allpinsunchanged/all10exacthost
identities+wrapper absent. Case mpi6_20261109T173409_3860527/assessment.json retains
222assertionfailures at9oldCNativeImport3D_tests locations andcompletedtests.
Shared event/engine ledger case completed ALL6ranks withoutreportedfailure;
wholeMPIqualification FAILED/INCOMPLETE. Matching archived296 control of5failed
cases planned; directextensiontests rununchangedimportcode but noexactbaselineproofyet.
No sourceedit/raisedbudget; reusecompleted evidence/resume affectedqualification.
ActualM6BLgeneration/normalized savedCFDtarget remain next capability.
Isolated80-iteration savedRANScontinuation inputs+launcher preparedonly, NOTRUN;
unchangedphysics/CFL, exactmesh/SAstate, distinctinput/outputstems. No reasonable-flow
qualification yet. Do not mistake policyprobe/ledgerfixture for BLsuitability/coverage.

Archived296 exact5-case MPI6control TERMINAL37/48.508s, archive/current/runtimepins
verified/all10observedhandlesabsent. Reproduces SAME222failures at SAME9import
locations, including productionwallpolicy completion refusal. No connectedproduction
fix justified by these pre-existing failures. Broadtimeout SIGTERM artifacts are
separate. mpi303_baseline_failure_comparison.json records scope.
CURRENT FROZEN resume session84753/case mpi6_resume303_20261109T174937_3895940,
118unfinishedcases only/same600scap, allcurrent303pins frozen. Priorcompleted
results retained; terminalhandles required before sourceedit/newheavyjob.

MPI303 resume TERMINAL18/42.366s,116/118casespassed EVERYrank;2remainingfailures
EXACTLYreproduce on archived296 MPI6control TERMINAL18/11.405s (108assertions,4locations).
Allpinsverified/allownedidentitiesabsent. All348selectedcases now covered;7old
failures remainFAIL, no newMPIregressionidentified. curved_bl303_qualification.json
explicitly all_required_checks_passed=false; no blanketqualificationpassclaim.

SavedRANS80-stepcontinuation303 TERMINAL0/23.568s, currentproductionCFD/MPI6,
allpins/mappedELFverified/all10actualhandlesabsent. Independentflow+SA/coords/
positivity checks passed. FinalCL.15573/CD.05027, last10spans.085%/.783%; no need
blindfreestreamrecompute. Spatial303 exactFloat32associations all19506wallnodes;
fivefiniteCpsections, Y+median32.22/p90193.74/max447.81 warnsnearwallresolution.
Parentuse decision qualifies ONLYcoarse-grid initialadaptationseed, notconverged/
wallresolved/experimentalphysics. Preservec90000...f1c78 restart and originals.
Next actualtwo-eventBL500k attempt withcurrent5400/event,14400outer,12GiB,
h02e-6/g1.2/T.01/MachonlyL1; FIRST_ROW still1nativeworker/MPI6CFD. No target/mesh
acceptance yet. Actualtarget normalization/construction outcome drivesnextfix.

CURRENT FROZEN actualtwo-eventM6BL run rans_curved303_v1, session79914,
wrapper3919208/start29162081; owned_handles recordsallactualidentities. Inputseed
c90000...f1c78/originalcubicreference2e99fb...42de/exact61330pointmesh, L1Machonly/
500k/h02e-6/g1.2/T.01,FIRST_ROW1native/MPI6CFD,native5400/event,outer14400TOTAL,12GiB.
AllCpp/helpers/binaries/runtime/config/input frozen. No target/meshacceptanceyet.
SeedandEuler293checkpointsremainseparate/intact. Newlauncher usesexisting selected
failureworkflow for firstsubstantial rejectedstate, not per-trial mesh exports.

### Actual full-target303 terminal failure — active blocker

Actual RANS/BL two-event attempt TERMINAL1/1332.237s, source/binary/runtime/inputpins
unchanged, all10exactownedhandlesabsent. InitialCFDfinite and truecubic builder work;
firstcomplexitytrial cell0 refuses `Unresolved cubic BL distance crossing` when
interval midpoint rounds toendpoint(CBC3476). NO normalized500k target or BLmesh.
Exactcell/sensors/core/logscale preserved under rans_curved303_v1/failed_complexity_cell0.json;
terminal_assessment.json recordsfailure/resources. No native-timeincrease.

Bounded investigation hypothesis: currentlevel-exclusion radius conservatively
contains ideal affinechord despite coordinate rounding; that radius has a nonzero
roundofffloor. Current unresolved integralbound reuses2*radius*metricBound, which
need notshrink with parameterintervalwidth. Investigate using certifiedidealchord
spanlength for integralmeasure while retainingfullradius for distanceexclusion.
Preserve1e-8 totaluncertainty, work/cuts/memory, exactsupport and finalstrictgates.
Independent proof review and exactcellreplay pending; no productioncorrectionyet.
This is the sameconnectedcurvedBL integration milestone, not an Euleroptimization.

Build304 CURRENT FROZEN session49142/helper3986594/start29345956/ninja3986595/
start29345972. OnlyCBC.cpp+BoundaryLayerMetric_tests.cpp changed. Sourcecorrection
usesoutwardidealchord spanlength for unresolvedintegralbound, keepsdistanceenclosure
radius for certifiedlevel exclusion. Adjacentparameterintervalsretainunresolvedspan
bound; impossibleaggregateconvergence stillrefuses. Same1e-8/work/cuts/memory/deadline.
Truecubic translatedmicroheight testcase andstrict50-value actualcellreplaybranch
added toexistinghiddenM6probe; no productionconfigoption. NOTYETVALIDATED.
Bounded failingcell input/provenance/plan under m6_bl_inputs/crossing303_cell0_v1.

### Focused chord validation and diagnostic setup305–308

Production304 build passed54.159s and affected22cases/283883assertionspassed. New
translatedfixture firstfailed open-reference topology, then305 missingadjacent
planarcontrolnets, then306 unrepresentable1e-12 realizationcontract at1e6 coordinates.
All failures preserved; allownedhandles terminal beforeedits. These were setup
failures beforechordexecution, not acceptedqualifications.

Test307 usesclosedtet/alladjacentnets/onegenuinelycurvedwall, a coordinate-scaled
realizationcontract64*eps*1e6 (~1.4e-8, <1%h0, belowstrict1e-4). H0/queryprecision
unchanged; forward/reversechordcutregression PASSED11assertions/12.997s. Production
CFD remainsBYTEIDENTICAL304c1803dab... . No completeM6closureclaim.

Savedactualcellreplay307 loadedoriginal48954netauthority andexact50values, then
failed diagnosticTetrahedronVolume positive-orientationprecheck (geometry_work0).
SourceCSolverusesglobal-nodeorder; replay keeps it forIntegrationRule3D. Test-only308
usespositiveorientedCOPYforindependentvolume check. All5replayhandlesverifiedabsent.
CURRENTfrozenbuild308 session41491/helper380409/start30865551/ninja380410/start30865565.
No sourceedits untilterminal. Nextsingle actualcellreplay308 bounded1800s, thenreturn
fullsame-seedM6 ifqualified. Reuse22passingcases and307regression; no broadrepeat.

Build308 passed98.139s/allpinsunchanged/all10sampledPIDsabsent; productionCFD remains
c1803dab... BYTEIDENTICAL304. Hiddenreplay positive-volumeCOPYfix is test-only.
CURRENT FROZEN replay308 session32708/cases/cubic_cell_replay308_v1,
wrapper387146/start30877976; owned_handles.json recordsall5exactidentities.
Actual48954netauthority/exact50values loaded; productionIntegrationRule3D began7.197s.
Serial1800diagnosticbound/currentnative5400, allsource/helpers/binary/runtime/input
frozen. Outcome pending; no fulltarget/mesh qualification yet. Returnfullsame-seed
M6 afterqualifyingactualblocker; no newbroadcampaign orspeculativeoptimization.

### 2026-11-10 — actualcell308 stillrefuses; isolateaggregatebound

Replay308 TERMINAL1/551.750s, allsource/input/runtimepinsverified, all5exact
identitiesabsent. ActualIntegrationRule3D failedafter548.959s/5502217geometrywork:
`adjacentintervalexceedstotaluncertaintyallowance`. Source-spanfix passesindependent
regressionbutisnot sufficientactualM6qualification. No target/mesh accepted.

Incrementaloutwardsum mayaccumulateresidue; anadjacentlargestleaf mayalsoblock other
refinableleaves; orleafsum mayactuallyexceedtolerance. Currentevidence doesnot
distinguishthese. CURRENTfrozenbuild309 session75176/helper429037/ninja429046 adds
ERROR-ONLYstructureddata: exactchorda/b/args/wall/component, length/metricbounds,
parentinterval, incrementalBEFOREremoval, freshoutwardsum includingparent, counts/
sums foradjacent/splittableleaves andmaxsplittableuncertainty. No acceptancechange.
Existinghiddenprobe addsbounded8-value directchord input, forisolatingcapturedchord
withoutanotherwholecellreplay. Nextexactcellreplay309 thenchoosecorrectionfrom
thesevalues; all1e-8/work/cuts/memory/timegates unchanged. No speculativefallback.
Preparedrun_rans_curved308.py isNOTRUN andmustremainblockedbyreplay308failure.

### Replay policy fidelity correction310 — supersedes308/309 scopeclaims

Sourceaudit foundreplay308/309 loadedexactcell/sensor/core/reference but OMITTED
ConfigureOuterTransition. ProductionCSolver3202/3206 andFIRST_ROW TargetPolicy.Update
initializefull at eigMin=1/Hmax²=.0001, then configurecapturedtrialcore. Monotone
transition supportretainsmincore; capturedtrial.007604989 leavesretained.0001.
PreviousreplaytransitionCore stayed0. Thus308'srealrefusal is NOT proofthatcorrected
productionM6 stillfails. Originalactual303 refusal remainsauthoritative. Audit
replay309_policy_fidelity_audit.json preservesexactsourceproof andscopecorrection.

309diagnosticSTOPauthorizedforvalidationfidelityerror, preservingpins/sourceevidence.
GNUtimeout moveddescendants toanothergroup; afterinitialtimewrappergroupSIGTERM,
threeexactremainingidentities were reverified/terminated ascompletionof SAMEstop.
All5identitiesnowabsent. No unrelatedjobs affected; neverrepeatstophelper.

CURRENTfrozenbuild310 session48949/helper458360/ninja458374. Test replaynowfaithfully
calls sharedfull ConfigureOuterTransition(initialCore,hgrad) then trialcore/hgrad,
assertsretainedmincore andprintsinitial/trial/retainedvalues. SourceCSolvercallback
usesnativeLayersdirectly; base/Apolicy are notconsumedthere, so nonewduplicatefactory
needed. BudgetBL=false explicitlyasserted. Error-onlyproductiondiagnostic nowalso
recordsretainedtransitioncore/slope/full/support; directchordinput9values includes
capturedtransitioncore. No acceptance/gate/budget changes. Nextfaithfulreplay310;
independentcallerfidelityreview pending. Source/helpers frozenuntilterminal.

Build310 PASSED90.153s/pinsunchanged/all21sampledhandlesabsent. Independentcaller
reviewconfirmsfirst-trial full-layer replaymutations andcallback; resourceadmission
differsfromfullsolver/base/Aretention, notwhole-normalizationqualification.
CURRENT FROZEN faithfulreplay310 session78676/cases/cubic_cell_replay310_v1;
wrapper466078/start31046059, fullownedidentities inowned_handles.json. Runtime
printsinitial/retainedtransition.0001, trial.007604989,hgrad1.3; integrationbegan7.888s.
Serial1800/currentnative5400 unchanged. No outcome yet.
parse_chord310_terminal.py prepared/syntheticselfcheckpassed; terminal+verifiedpins/
actualhandles requiredbefore extractingDecimaldiagnostics and9-valuechord+provenance.
No autosolverlaunch. Fullworkflowremainsblockeduntilactualtargetblockerqualified.

### Faithful310 terminal work-limit refusal; sharedcapture311

Faithful310 TERMINAL1/1264.192s, pins/runtimeverified/all5exactidentitiesabsent.
Correct.0001initial/retainedcore,.007604989trial/hgrad1.3; actualintegrationfailed
at1261.481s/13328602geometrywork, chord16384-stepguard/leaves7965. No target/mesh.
Oldstd::to_string printsremainingbound0.000000; no precisechordpayload (310only
instrumentedadjacentguard). This is nowauthoritativefirst-trial production-policy
workfailure; do notraisecap orinterpretroundedzeroasqualified.

311sharederrorformatter coversworkguardandadjacentguard. Duringpartialsplit, pending
coverisparent beforeleftchild, righthalfbefore rightchild; plusheap givesexactlyonce
coverofallremainingunresolvedregion. Freshoutwardsumvalidupper; unqueriedpending
region maylaterbecertifiedaway, soexceeds-tol isnotirreducibilityproof. Independently
reviewed; noqueryorder/acceptance/limitschanged. Build311 PASSED72.169s/allpins/
all14sampledbuildPIDsabsent.
CURRENT FROZEN replay311 session65502/cases/cubic_cell_replay311_v1, wrapper544556/
start31225876; owned_handles storesall5identities. Correctpolicyprinted; integration
began10.329s,1800diagnosticbound/native5400unchanged. Nextterminal311genericparser
extractsfailedchordfordirectisolation; notanotherfullbaselinecampaign.


## Faithful311 result and connected local bound312 — 2026-11-10

311 terminated1/1255.902s, unchanged pins/runtime and all five actual PID/start
identities absent. Geometry work13,328,602 exactly matches310. Precise chord capture
and Decimal extraction are under staged57/m6_bl_inputs/crossing311_chord_v1.
Fresh unresolved upper2.9878668513e-8 exceeds1e-8; adjacent1.1357150566e-9,
splittable2.8742953456e-8. Accumulator residue is not the main explanation.
Splittable/pending territory remains, so this is not a proof of irreducibility.
Global BL eigenvalue bound1.215e14 includes every associated feature at its h0
maximum, even distant segments/corners/images. Actual ApplyFeatures weakens those
terms at the transported anchor by 1/(h0+log(hgrad)*rho)^2.

Working312 keeps the wall eigenvalue bound and ALL feature terms, but bounds each
term over a chord-wide conservative anchor enclosure. Root projection is reused;
no extra geometry query or change to geometry exclusion. Candidate distance is
bounded by certified distance plus query precision, maximum original-facet
realization contract and affine coordinate error. A factor-three L1 envelope
covers rounded distance evaluation; normalized direction components are bounded
by two, enclosing inward transport and rounded anchor subtraction. Nonfinite
bounds retain the global estimate. Feature separation uses outward intervals for
the implemented a+s*t segment, downward distance/height arithmetic and upward
reciprocal/sum. No support-pruning, feature maximum replacement, new config, gate
or budget change. Existing storage accounting includes the new scalar contract.
Independent read-only review found no additional safety defect; actual checks are
still required and no work saving or full-target acceptance is established.

Build312 passed64.134s. Focused translated fixture ordinary section passed, but
new feature sections failed setup because an interior-fluid tetrahedron has no
solid-convex D1 edges. Preserve its2failedassertions/11.568s receipt under
cases/geometry_backend_20261110T001628_635339. All sampled PIDs absent. Build313
changes ONLY that fixture to a solid tetrahedron with outward-fluid normal
orientation and queries on its exterior side. Production code remains312.
No full BL normalized target or mesh, transfer or continued adapted RANS yet.

Build313 passed60.122s with production CFD SHA identical312. New genuine-feature
section passed; transported chord certified in both directions but test expected
identical frontier cardinality (44vs43), which adaptive stopping does not promise.
Preserve1failedassertion/55.526s under cases/geometry_backend_20261110T001909_643264;
all five sampled PIDs absent. Test-only314 checks sorted endpoints and resolution
of the independently known full-band crossing with the metric-weighted1e-8 bound,
while retaining the original forward/reverse equality check for the thin ray.
These are fixture/assertion corrections, not production changes or relaxed gates.

Build314 passed64.130s, all10 sampled handles absent; CFD identical312/313. Focused
fixture passed120assertions/1case/57.444s, all five sampled handles absent. This
qualifies those primitive paths only. Actual captured M6 chord314 is now frozen;
HANDOFF owns its current handles/outcome. Broader affected serial and ONE MPI
milestone follow only if that replay demonstrates useful progress.

Actual captured chord314 PASSED45assertions/27.681s helper, all five exact owned
identities absent and all source/runtime/input pins verified. Certification used
181652geometry work/17.014s and16cuts, unchanged1e-8/work/cut gates. This clears the
specific prior refused chord. Old311work13,328,602 is cumulative first-cell work,
not a separately measured old single-chord baseline; do not claim73x speedup.
Whole first-cell/target/mesh/workflow remain unproven. Affected serial22 and one
MPI milestone are next, then promptly the actual full SAMEseed RANS/BL attempt.

Affected314 serial passed22cases/283883assertions/291.681s, all five sampled actual
handles absent, all pins unchanged. ONE selected MPI6 milestone is now frozen;
qualification314.json remains pending until actual complete terminal results.
No actual full-target/BL mesh acceptance is claimed.

ONE MPI314 selected milestone PASSED27cases EVERYrank/66.660s, all10 sampled actual
PIDs absent and source/runtime pins unchanged. Assertions/rank1207,1737,1193,1244,
1255,1246. Coverage includes new cubic feature/transported crossings, D1,
curved analytic checks, height optimization, concurrent publication/dependencies,
repair batches and phase ownership. Prior303 known7 failures outside this affected
selection remain FAIL; no all-suite claim. qualification314.json seals current
primitive120assertions, affected serial22, actual chord45assertions and MPI27/rank.

Full SAMEseed RANS/BL314 is now frozen under unchanged303 inputs and strict gates.
No generation/acceptance/transfer/continued adapted RANS/restart proven yet. Follow
HANDOFF current handles; keep qualification and prior terminal receipts immutable
because the full run pins them. Actual outcome drives the next connected action.


## Full314 terminal and bounded point diagnosis315 — 2026-11-10

Full314 TERMINAL1/5462.393s, all source/input/runtime pins unchanged and all10
exact owned PID/start identities absent. No normalized target/BL mesh or adapted
RANS transfer/CFD/event/restart. Selected failure cell0/trial1/core/sensors/coords
match303 exactly. The earlier captured chord is cleared, but a later chord fails
its16384 classification cap: local eigenvalue7.00000248e12 vs global1.215e14;
7246splittable/0adjacent leaves, fresh3.13542258e-8 >1e-8. Fresh/incremental residue
~1.79e-14 cannot explain excess2.14e-8. Exact new payloads/provenance are in
staged57/m6_bl_inputs/crossing314_chord_v1. Whole target remains unproven.

CSolver breaks its local cell loop on error and then joins CollectiveFailure;
other ranks may continue their local work. Therefore5462s is complete collective
latency, not measured time in selected cell/chord, and selected global error does
not expose other-rank progress/failures. No claim of a global deadline refusal.
Maximum sampled process-tree RSS2,543,411,200B; sampled CPU31,422.76s includes MPI
polling/waiting. /usr/bin/time's mpirun RSS is not a whole-job peak.

Selected ideal leaf span~1.04e-18 is below coordinate spacing~2.22e-16. This
supports investigating the coordinate-enclosure/Lipschitz floor, amplified by
whole-chord feature bounds, but does not uniquely establish cause. Existing
external M6 query replay covers surface-union certificates, not arbitrary closest
point intervals; no whole-chord baseline replay is launched to diagnose this.
Test-only315 extends existing NativeActualCubicBLPolicy3D with max12 point records:
retained/trial core and original component hint/wall/xyz/precision, using existing
ClosestCurved plus separate actual policy Evaluate. Original authority, shared
work/storage/time admission and exact attempted-work receipts remain. Eight
Fraction-derived deduplicated leaf/neighbor points are prepared with classifier
radius2.48259476e-16/precision3.10324345e-17. Returned point/normal/distance are
explicit diagnostics, not certificates. No production path changed or correction
selected. Review measured intervals before choosing ONE connected next change.

Point315 PASSED164assertions/11.439s helper, all5 exact owned identities absent,
all pins/runtime verified. Build315 test-only passed56.156s/all10 handles absent;
production CFD byte-identical314. Direct6680work plus separate Evaluate1920work.
Certified center widths2.26e-17 vsradius2.48e-16; distance slope8.8649e-4 agrees
with finite differences (normalized.3779, not tangent in samples). Tensor maximum
8.54e6 at these points is far below7e12 chord bound. See point_probe_v1/analysis315.json.
This supports a conservative interval-local profile/feature bound; no new bound
or production speedup is established. Working design/proof obligations are in
ADAPTATION_CURVED_BL_LEAF_BOUNDS_20261110.md. No active heavy job now.
