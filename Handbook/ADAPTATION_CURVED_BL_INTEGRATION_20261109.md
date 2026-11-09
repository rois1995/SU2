# Connected curved-wall BL milestone

User steering2026-11-09. This is the next implementation milestone AFTER frozen
FIRST296 terminates and actual owned handles are verified terminal. No BL source
edits/build/campaign have begun. Preserve qualified Euler construction/certification,
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
