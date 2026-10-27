# Curved boundary mobility: missing geometry capability

Read-only diagnosis, updated 2026-10-27. No curved cross-chart implementation or
numerical qualification is claimed here. Build18 and its quality capture are
terminal; no owned build/validation job remains live. Quality relaxation versus
enrichment staging implementation is starting separately; it does not provide
the cross-chart geometry capability described here.
The target remains a complete accepted M6 Euler/RANS workflow under the unchanged
geometry, contact, q/J/size/height, complexity, transfer and restart gates.

## What currently prevents movement

The optimizer's dimension-zero guard is only the first restriction:

- `CNativeAffineReference.hpp/cpp`, `MappedFacetReference`, rejects simultaneous
  cubic controls and `reference_components`. Consequently `PhysicalSupport`
  classifies every original curved vertex/edge as a physical support. Its curved
  `Canonical` is the original facet identity.
- `CNativeBoundaryParameters3D.cpp::ProposedBinding` preserves a previous original
  vertex binding. `Supports` and `FacetParameters` require a binding to belong to
  each individual original facet supporting an active face.
- `CNativeBoundary3D.cpp::MoveVertex` retains the incident face labels and requires
  each moved point/whole triangle to remain inside those same finite charts.
- `CNativeBoundary3D.cpp::ConstrainMovePoint` treats mapped cubic charts as
  faceted planes because they are absent from `Analytic()`. Independent original
  facet normals can give rank three and pin an original curved vertex; original
  seam constraints can also restrict motion to an artificial chart edge. Both
  distributed direct-move paths call it when metric composition is present.
  Semantic geometry freedoms must replace these chart constraints while retaining
  genuine feature, carrier and first-height apex constraints.
- `MappedFacetReference::ValidateSurface` requires complete tiling of every
  original facet and its three border chains. `BoundaryParameterState::Prepare`
  conserves interfaces separately by original facet. Moving an original vertex
  off a harmless chart intersection violates these independent contracts.
- `SurfaceError` and `PrepareSourcePatch` restrict one immutable cubic net using
  the three bindings in that same chart. Neither represents a face spanning
  several original charts.

Removing the dimension-zero guard or broadening `Supports` cannot establish this
capability. In particular, it would leave whole-face error, finite coverage,
publication and future source reconstruction without valid certificates.

`OptimizedRepairPatch` setting `options.boundary=false` is not a cause of this
restriction. That option gates `BoundaryFlip` and `BoundaryCollapse` only when no
geometry is supplied. With geometry and smoothing enabled, `Smooth` still calls
`SmoothBoundary` for physical skin vertices; its binding guards and the shared
geometry contracts above determine whether movement is allowed.

`ClosestCurved` already finds a certified distance interval and candidate binding
on an edge-connected same-marker component. Its documentation explicitly does
not certify a unique minimizer. It is useful proposal/clearance machinery, not a
whole-face atlas or permission to cross a genuine feature. The planar
`ReferenceComponents`, `CandidateFacets`, `ComponentBorder` and exact projected
containment APIs demonstrate useful patterns, but their planar assumptions cannot
be applied unchanged to a curved component or closed sphere.

## Reuse the existing physical feature policy

`CNativeBLTarget3D.hpp::BuildOptionA` uses original facet adjacency, the configured
`ADAP_ANGLE`, exact convexity and configuration-derived marker roles. Reflective
symmetry carriers are continuations for D1; marker names are not classifiers.
`CNativeMetricFamily3D` retains `feature_angle` and `feature_identity` so a metric
event cannot silently change that feature policy. `CBoundaryLayerMetric` exposes
its canonical D1 summary/identity; its complete segment storage is private.

Reuse that policy and its original geometry input instead of adding another angle
setting. D1's segment list alone is insufficient: D1 selects convex metric
features and can be disabled, while geometry must also retain concave sharp
creases, physical marker interfaces and carrier restrictions. A reflective
continuation avoids a D1 ridge; it still constrains a wall/carrier junction to the
prescribed intersection.

The immutable PN evaluator has `VertexFeature`, `CornerNormal`, `Derivatives` and
`ControlNets`; however those feature classifications are not in the current v7
mapped payload. It stores cubic controls, errors, affine carriers and bindings.
The current M6 payload has 48,954 cubic facets, 8,094 affine facets and geometry
version18; its saved configuration uses `ADAP_ANGLE=60`. Cubic shared-edge controls
are checked for equality, which establishes watertight position data, not that a
same-marker edge is a harmless seam.

A connected geometric feature graph should therefore be derived once from the
immutable authority and the existing configured policy. Keep marker/body and
carrier constraints. Bound one-sided differential normal variation along the
whole shared cubic edge, using the existing `CubicNormalControls`, `Blossom` and
interval arithmetic; a midpoint normal comparison is insufficient. Protect
unresolved classifications under explicit work/storage limits. A true curve's
sampling vertex can move along its prescribed curve chain; a branch/corner stays
fixed. Smooth chart edges/vertices are not physical features. Canonical graph
identity and angle must agree across workers and survive return/restart, without
changing the original cubic controls or sensor field.

## Smallest connected construction

The proposed data/API and certificate implementation are detailed in
[bounded curved atlas implementation](CURVED_ATLAS_IMPLEMENTATION.md); they are
not implemented or numerically qualified.

Use a bounded local curved atlas for each complete admitted boundary star/region,
not a global planar chart of an entire wing or closed surface. Start with existing
original adjacency and nearest/forward parameter APIs. A common projected graph
chart is practical where the union has a certified injective oriented projection;
prove that property over the complete local cubic patches, not only their corner
normals. A folded or unresolved union must refuse that certificate.

Each new active face needs a shared coverage certificate that partitions its
support over the immutable charts. Reuse exact finite projected containment and
polygon/triangle predicates, then restrict each participating cubic net with the
existing Bernstein machinery. Certify complete coverage, positive orientation,
no gaps/overlap, the retained perimeter and all physical feature chains. Compute
conservative whole-face deviation from the resulting pieces. Reuse that same
coverage in publication, error checks and source-envelope preparation.

An important unresolved detail is the edge transition contract: independently
choosing a projection for each neighboring face can assign different reference
paths to their shared edge. Moving only the vertices and clipping each face
independently is therefore not a complete design. Shared edge transitions and
perimeter traces must be canonical/reused across adjacent faces and subsequent
transactions. A local atlas certificate must either prove their agreement or
carry the necessary parameter-path data through publication and persistence.
This is the central cross-chart certification work, not an optimizer fallback.

A proposed representation is a topology-preserving piecewise affine flattening
of a bounded union of original reference triangles. Proposals would live in that
domain and forward-evaluate the unchanged cubic nets. Canonical shared edge
traces would be piecewise linear in original barycentric parameter domains;
active faces would carry their partition into original-chart parameter pieces.
Exact clipping predicates could operate in the flattened domain, with `Blossom`
restricting the cubic pieces for whole-face error and source-envelope bounds.
Inherited perimeter traces must agree exactly, including across later local
atlases, workers and restart. Projected physical cubic edges are generally curved;
clipping their corner triangles as straight boundaries does not certify coverage.

This route is unimplemented and unproven. A bijective reference-domain flattening
alone does not establish injectivity of the real cubic surface union or admissible
embedding of the new flat faces. Complete local geometric injectivity,
orientation, feature/carrier intersections, separation and contact certificates
remain necessary; a common physical projection is only a possible sufficient
certificate where it can be proved. Folded or unresolved geometry must refuse,
without weakening geometry or replacing the immutable controls.

Keep point bindings as forward chart/edge parameters where possible. Separate
point support from whole-face multi-chart coverage rather than pretending every
point belongs to every original facet in a component. Update the complete
admitted star atomically with a fresh node identity, preserving true features,
exact-positive cells, outside contacts and the existing distributed reservations.
Construction/quality stage budgets and final strict gates remain unchanged.

## Affected callers

1. `CNativeAffineReference.hpp/cpp`: derived feature graph, bounded atlas/coverage,
   whole-face containment/error, surface audit and source-patch restrictions.
2. `CNativeBoundaryParameters3D.hpp/cpp`: proposed moved binding, face lookup,
   publication interface conservation and serialized authority/coverage policy.
   A changed `Canonical` alone would corrupt the present facet-wise invariants.
3. `CNativeBoundary3D.cpp`: `ProjectMovePoint`, `ConstrainMovePoint`, `MoveVertex`,
   `Finish` and operators reusing physical-face checks; complete moved faces must
   use the atlas contract and semantic feature/carrier constraints.
4. `CNativeEngine3D.cpp`: `MeshOptimizer::SmoothBoundary` should use semantic
   mobility, then the shared move admission. The distributed quality lifecycle and
   its strict census remain the acceptance authority.
5. `CNativeImport3D.cpp`: curved `PrepareSourcePatch` use and swept-source
   separation must accept multi-chart active faces. Existing source hulls cover
   full original nets, but that alone does not certify a new face's cross-chart
   homotopy or a second adaptation event's source construction.
6. `CNativeReferenceIO3D.cpp` and boundary payload/reader paths: preserve original
   authority and re-establish identical graph/coverage semantics on restart.
   Configuration-derived angle/roles must be available or recorded; legacy v7
   controls alone do not record the event's feature policy.
7. `CNativeBLTarget3D.hpp`, `CNativeMetricFamily3D` and retained geometry setup:
   feed the same angle/role/feature identity rather than reconstructing policy
   independently from the changed active mesh.

## Acceptance checks

- A smooth multi-chart cubic star: move an original vertex and an original
  seam-edge point across authority charts. Require nonzero motion, certified
  whole-face coverage/error, exact-positive cells and immutable source/control
  hashes. Contrast it with a same-marker sharp crease and a real corner.
- A curved wall meeting an affine symmetry carrier: allow prescribed intersection
  motion while preserving carrier geometry. Exercise a concave crease, chart
  projection fold/hole and two nearby disconnected bodies carrying one marker;
  vertex-only projection or same-marker union must not admit them.
- Publish/reject the same distributed star on worker boundaries; verify identical
  shared transitions, rollback, contact veto and parameter return. Rebuild the
  original-source envelope from the accepted redistributed mesh, adapt again,
  transfer and restart. One successful first-event source query is insufficient.
- Use the saved valid-but-poor M6 midpoint cavity and a contrasting captured
  cavity with their actual v7 authority, rotating metric and requested heights.
  Verify boundary freedom and final strict patch gates under unchanged limits,
  then qualify the full M6 Euler and RANS workflow. Local mobility is diagnostic
  evidence; only the complete accepted workflow closes the capability.
