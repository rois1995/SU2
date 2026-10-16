# Goal3 — whole-cap point distribution and shared fronts

Full Goal3 ACTIVE/incomplete. Working source replaces inherited midpoint cap
refinement in the geometry-certified right-prism constructor. The earlier local
cap-operator failure and its withdrawal at c103aa9cf3 remain preserved in
[the prior strategy book](GOAL3_TANGENTIAL_CAPS.md).
Evidence: `integration_evidence/native_3d_bl_v1/tangential_caps_v2`.

## Missing capability and connected change

A good metric-balanced split on an inherited edge does not establish a suitable
whole point distribution. The frozen30degree tangent256:4 target exhausted the
local controller before completing even a cap. Its independent complete1248-cell
witness establishes a viable different layout under the mathematical target.

The new primary constructor builds the entire cap point set before reconnecting
its volume fronts. It uses one area-weighted tangential metric only to propose a
chart/lattice, finite-boundary full-metric equidistribution, and local actual-metric
boundary clearance. It then reuses native polygon triangulation, exact orientation,
complete containing-cavity insertion and improving complete edge-star reconnection.
No input volume/cap diagonal is protected. Shared sorted-ID three-tet prism
connectivity and existing normal-front interval closure connect the whole region.

Actual-query tangential tensors at all planned normal fronts guide proposals.
The chart average does not replace the original3D field. Frozen sensor-only P1,
geometric BL composition, finer demands, fade and first height remain unchanged.
Whole-region height/geometry/embedding and actual centroid q/J/every full composed
edge remain mandatory. A cap can look valid and still fail these3D gates.

Boundary placements, lattice candidates, site insertions and accepted reconnecting
proposals charge surface work. Predicted complete transition cell count participates
in admission throughout. Existing work/time/memory ceilings and atomic release
remain. Cap time is separately exported, nested in regional sampling. The collector
also pins the reused2D mesh header and exact-predicate source.

## Design checks and pending qualification

Offline whole-cap proposals and a native-style polygon/insertion/reconnection replay
pass preliminary3D shape checks for the fixed rotated field and an affine varying/
rotating original-P1 field:1248/2112 tetrahedra, q.3579/.3911 and J.1535/.2011.
The native-style replay uses67/212 flips. Scalar control384 cells also passes.
These are shape/midpoint-length diagnoses, not native/full integral/geometry/work
acceptance. SciPy is an offline reference dependency only; production adds none.

The connected build passes. Native strong target `native_prism_tangential_v4`
now passes6->1248 BL->447 sensor-only cells, work1646/628. Regional cap time4.63s
is nested in33.26s full construction; removal2.02s. These are local diagnostics,
not speedup/scaling or affordability proof. Independent whole audits pass both meshes. Spatially rotating original-P1 demand
also passes6->2112->722 in serial (work2728/877), independently q.431492/J.182629/
full L1.609667 in BL. MPI4 independently passes6->2112->700; constructed geometry/
boundary labels exactly equal the serial result. Do not infer identical removal
connectivity or scaling. Relevant actual backend and opposing-wall checks remain.
Do not repeat primitive suites for individual edits.

## Shared closest-facet query defect

The first complete1248-cell native candidate (v2/v3) was rejected when the existing
65536-evaluation edge integrator could not resolve an edge on the planar wall.
An independent exact geometry/height/all1967 full numerical-edge audit passes that
saved rejected candidate: q.357911/J.153523/L1.699959. This is not native acceptance.

Focused `native_planar_tangent_edge_v1` reproduces a much larger field defect:
near the wall facet diagonal, the ADT's tolerant triangle selection can choose the
wrong triangle. Its tiny in-plane closest-point residual becomes a false wall
normal: target Mzz changes100->4, maximum tensor error96. The mathematical planar
reference is constant along that edge, whose exact length is1.144577413898648778.

Shared `CBoundaryLayerMetric::Evaluate` now treats the ADT result as a seed. Its
recomputed closest-point distance plus the existing coordinate-roundoff allowance
bounds a conservative `DetermineElementsWithinDistance` query. Compare actual
closest points for all admitted facets, with deterministic exact-equality ties,
before using barycentrics/normal/size/fade. This also serves private/remote metric,
complexity and integration callers. The physical policy, finer tensors, sensors,
fade, normal formula and quadrature tolerances/limits are unchanged. This is no
hard-normal reset. No global ADT containment behavior changed.

Focused v2 passes: maximum tensor error2.56e-13, integral difference6.6e-16.
733 queries include128 diagnostic samples; this is not733 integral evaluations.
Both failed construction receipts/source snapshots and the failing focused test
are preserved. One test-build tensor-alias collision is also retained/corrected.
The corrected closest-facet search adds work; no per-query performance claim yet.
Shared-metric curved/edge controls and actual lifecycle qualification remain.

## Limits and unchanged milestone

One average chart can miss extreme localized density/orientation variation; it
must reject at full-field closure rather than change the target or try a spacing
ladder. This constructor requires one simple cap loop, shared through a finite
right prism; arbitrary normal/tangent coupling, holes, curved nonprismatic walls
and scalable generation remain unqualified. Source-bound coarse geometry, common
front footprint and high first-row tilt are not orthogonality guarantees.

This planar construction stress cannot substitute for the next curved milestone:
a complete accepted curved whole mesh under unchanged constraints, independently
checked. The prior2877-cell accepted curved baseline retains its scoped proof.
Wider curved/features/interactions, composed-gradation feasibility, physical
viscous accuracy and complete lifecycle affordability remain full Goal3 work.

## Current curved/control qualification

The unchanged high-ratio cylinder on this source accepts2877 cells/eight fronts/
work3771. Its complete mesh, geometry, original sensor records and configuration
are byte-identical to the prior accepted case. The production closest-facet source
changed, so native tensors/edge results are compared anew: all2877 samples/shapes
and all4727 full edges satisfy the existing independent reference. Point-reference
relative bound1.064e-14; maximum native/numerical edge residual4.986e-9. The new
checker reuses exact geometry/height/embedding and independent numerical integrals
only after checking all relevant hashes, complete edge coverage and new results.
An initial relative-path serialization failure is preserved; no SU2 rerun followed.
No unchanged-production-source or wider curved-geometry claim is made.

Existing geometric composition/edge/placement controls pass12 cases/1014 assertions;
the new frozen planar-edge regression separately passes1 case/3 assertions. All
completed current source/build/binary/object and artifact pins are checked by
`validation.json`. Actual RANS/transfer/output/restart checks remain for this batch.
Full Goal3 remains active; these are complete scoped meshes, not its full completion.
