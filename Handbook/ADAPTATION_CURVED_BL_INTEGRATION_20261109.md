# Connected curved-wall BL milestone

User steering2026-11-09. This is the next implementation milestone AFTER frozen
FIRST296 terminated and actual owned handles were verified terminal. Connected BL
source implementation has now begun; build301 is frozen/running (session30473); current state takes precedence below.
No BL tests/CFD yet. Preserve qualified Euler construction/certification,
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
