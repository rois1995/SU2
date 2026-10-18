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
This is a design direction, not yet an implemented or verified authority.
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

Initial Cartesian 3D adoption and exact finite nonconvex/annular unions are
unpublished work. Cube RANS/SA current+BDF/transfer/SU2+CGNS/real-restart workflow
passes independently in serial; annular cold construction exhausts 8192 work
without a complete accepted mesh. Rotated charts, general simultaneous body
constraints, 2D declaration integration, multiple-body exterior workflows and
authority versioning are OPEN. No general geometry completion claim is justified.
See [planar work and exact evidence](GOAL3_PLANAR_SYMMETRY.md).
