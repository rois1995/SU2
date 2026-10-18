# General symmetry geometry — active Goal3 extension

User specification, 2026-10-18. Full Goal3 remains ACTIVE, with all existing
acceptance gates. Cartesian Y=0 and one annulus are limited evidence only.

## Required capability and connected reach

Geometry authority is independent of Euler/RANS. Existing body authorities stay
in use; RANS adds configured wall height and BL demands. A marker can contain
multiple bodies, components, holes and disconnected finite fluid regions.

Implement 2D finite unions of line intervals and 3D finite planar domains with
oriented outer/inner loops and disconnected components, at arbitrary positions
and orientations. Projection onto an infinite line/plane is insufficient.
Projection, membership, complete segment/face containment and feature/body
intersection queries must share an authority policy with dimension-specific
kernels. No edge or face may bridge a hole merely because its vertices pass.

Adopt initial geometry before geometric quantities, states and original P1 donors
are frozen. Existing states need matching authority or an explicit transfer;
never retrofit an archived frozen field. Keep actual-query geometric BL, finer
sensor demands and fade unchanged. Version authorities and dependent caches;
persist through both SU2 and CGNS output and real restart.

Junctions are simultaneous constraints: shared identities, coordinates and
ownership, preserved body geometry accuracy, explicit conflicting-declaration or
invalid-topology rejection. No marker-to-single-body assumption, testcase-specific
constructor, near-coplanar merging or global tolerance enlargement.

For arbitrary orientation, use local affine charts. A promising representation
uses dominant-axis projection: chart coordinates are unchanged binary64 global
coordinates, so footprint predicates can remain exact. Lifting the dependent
coordinate needs an explicit bounded rounding/error audit against that authority;
mathematical coplanarity of arbitrary rounded triples is not a requirement.
The geometry-only affine core now passes focused checks and an independent
exact-rational lifting audit. Complete runtime authority integration remains pending.
Robust global tetrahedron/triangle orientation and positivity stay mandatory.

MPI preparation/publication/failure must be collective and atomic. Stable feature
and component IDs derive from immutable geometry, with deterministic declaration
agreement. Reuse prepared topology/search by authority version, admit memory/work
before publication, no extra whole-volume gather or query-time negotiation.

## Validation and honest scope

Required: rotated/translated lines/planes; multiple body footprints; disconnected
regions and multiple holes; shared junctions; narrow-gap and conflicting-constraint
rejection; both dimensions and Euler/RANS; requested BL constraints; MPI N=M and
M<N; conservation/admissibility; output, persistence and actual restart. At least
one complete representative exterior-body workflow is required. Interior sphere
and annulus fixtures do not establish aerodynamic readiness.

Separate invalid geometry, geometry/height/metric incompatibility, integration
uncertainty, operator blockage and resource exhaustion. A valid reference proves
no construction feasibility. Preserve exact annular failed state and trial before
reconsidering coordinated surface/first-row/transition construction; do not add a
fallback ladder or simply increase the work cap.

Performance receipts must include cold preparation, query/reuse work, requested
memory/peak-memory scope, communication, reconstruction, transfer/rebuilding and
complete accepted-event cost. Rejected M6 timing proves no affordability result.

## Current checkpoint

Cartesian3D adoption and finite nonconvex/annular unions are integrated. Native
coordinated annular construction now passes initialized RANS/SA/current+BDF/
transfer/SU2+CGNS/real restart in N1/M1,N2/M2,N4/M2,N4/M4 with independent
finite-hole/volume/embedding/height/full-metric audits. All original gates stay fixed.
Bounded binary64 coordinate admission resolves the saved MPI shared-apex blockage;
its failures and rejected exact-midpoint alternative remain preserved.
[Current scoped results](../integration_evidence/native_3d_bl_v1/native_height_star_v1/README.md).

Arbitrary charts and original-source/backend primitives are qualified separately;
a complete oblique lifecycle, simultaneous curved body/plane ownership and source
coverage,2D declaration integration, multiple-body exterior workflows and complete
junction lifecycle semantics remain OPEN. Neither annular acceptance nor interior
sphere tests establish general aerodynamic readiness. No new M6 attempt is justified
until its complete geometry/source contract is implemented and audited.
See [planar work](GOAL3_PLANAR_SYMMETRY.md) and [construction history](GOAL3_ANNULAR_COORDINATION.md).

## Historical annular construction strategy evidence

The terminal private grid still contains symmetry diagonal1--5, from wall corner
(1,0,0) to body-footprint corner(.6,0,.4), with4tetrahedra in its complete edge star.
The saved attempted split replaces it but creates q=.145498/J=.0490366 at cell51370.
That cell couples a symmetry triangle to a volume apex far from the symmetry plane.
Original field/profile hashes match the independently feasible204-tet witness
exactly. This is bounded construction failure with evidence of operator limitation,
not invalid geometry, budget infeasibility or a proof that every proposal fails.

The missing capability to reconsider is coordinated finite-surface point
placement/reconnection plus first-row and volume transition reconstruction. A
surface-chart mesher could propose a distribution using the actual pulled-back
metric and immutable footprints/features, but independently meshing the surface
would not prove a valid positive/full-metric 3D volume replacement. Do not claim
this design is implemented, or add another edge-local fallback/cap increase.
Exact saved diagnostics: planar_symmetry_reference_v1/annulus_v4_n1/structural_diagnosis.json.

## Connected integration still required

V2 initial adoption now passes scoped serial/independent controls, while the oblique
construction rejects. Retained-cell first-row follow-up is building, UNQUALIFIED: [current handoff](../integration_evidence/native_3d_bl_v1/affine_initial_adoption_v1/README.md).
It includes boundary-only embedding/resource checks and preserves the open scope
below; no complete oblique/exterior/junction success is claimed before execution.


The shared affine core and scoped3D source/backend are qualified. Before exposing
arbitrary initial declarations with body junctions, complete the remaining path:

1. Independently declare original input-deviation limits and mapping/authority
   error limits. They serve different purposes. An accepted approximate boundary
   point must remain unchanged on repeated projection/return (no geometric creep).
2. Prepare finite component/feature ownership from immutable original boundary
   topology, not a body-per-marker assumption. Resolve body and symmetry constraints
   simultaneously and audit the existing body authority. Initial Cartesian adoption
   does not independently qualify arbitrary body-authority intersections.
3. Build original-sensor-only query coverage for the difference between the adopted
   rounded surface and original volume facets, reusing the existing analytic source
   coverage infrastructure where qualified. Original P1 has priority; no coarse
   spread of composed BL tensors, nodal-source refit or indefinite extrapolation.
4. Collectively admit geometry storage/work, agree authority definitions/versions
   and publish only after positivity/topology/body-accuracy audits. Keep geometry
   replicated at boundary scale, without a new volume gather or per-query negotiation.
5. Persist authoritative definitions, finite topology, features and versions in the
   existing dimension-specific native sidecars. Rebind through SU2/CGNS/restart;
   do not normalize an old off-plane frozen state without explicit state transfer.
6. Integrate operators and full-field/height/transfer acceptance in both dimensions;
   independently qualify actual exterior workflows before broad readiness claims.

Existing 30P30N inputs are available at
/media/rausa/4TB/Validation/ThreeElementAirfoil/30P30N/Meshes (2D and3D CGNS coarse
through fine). They have only been located, not opened, imported or run in this
extension. Their element types, symmetry suitability and local resource requirements
must be inspected before selecting a representative workflow. M6 remains on hold
until controlled construction closes under unchanged constraints.

## Qualified core checkpoint, distinct from runtime support

`affine_geometry_core_v1/serial_v1` passes9 cases/3731 assertions across five oblique
charts and all dominant axes, finite intervals, multiple holes, a notch and disconnected
regions.319/320 saved samples have a nonzero exact plane/line equation; all satisfy
reported physical bounds (largest sampled distance2.042804e-16, reported2.229226e-16).
Projection is idempotent inside the declared contract; BC labels do not partition
geometry. Straight footprint edges only; analytic body-intersection curves are OPEN.
Current-binary MPI4 reference controls and independent Cartesian N4/M2 initialized
RANS/SA transfer/output/actual restart pass. These are compatibility controls and
primitive evidence, not completion of arbitrary Euler/RANS geometry workflows.
[Executed scope and provenance](../integration_evidence/native_3d_bl_v1/affine_geometry_core_v1/README.md).

Current connected source/backend checkpoint is [tracked separately](GOAL3_AFFINE_RUNTIME.md).
It passes scoped Euler/RANS imports, serial/MPI original-field audits and Cartesian
compatibility; initial adoption/junction/2D/exterior and accepted oblique workflows
remain required and OPEN.
