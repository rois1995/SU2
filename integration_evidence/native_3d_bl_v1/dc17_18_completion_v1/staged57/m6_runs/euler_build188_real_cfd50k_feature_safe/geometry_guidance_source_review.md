# Build188 geometry guidance — read-only source review

Current attempt remains LIVE/source-frozen. This review and the linked counter
postprocessing change no repair decisions and establish no mesh acceptance.
Primary checked the cited source; both bounded read-only agents completed/no jobs.

`GeometryGuidanceExcess` (Boundary3D.cpp:3103) creates a fresh private parameter view
and calls validated SurfaceDeviation on each physical face in order, summing area-
weighted squared excess, with the existing conservative partial-sum early rejection.
`SmoothBoundary` (Engine.cpp:5946) computes old guidance lazily once per invocation
and reuses it over the candidate loop; each surviving fresh candidate is evaluated.
There is no evidence of repeated old-score computation within that same loop.
Repetition across separate calls/sweeps is possible but not quantified by these counters.

`HasAcceptedFaceCertificate` (BoundaryParameters.cpp:1001) certifies exact accepted
face/facet, coordinates/bindings and coverage/trace identity. It is used in atlas
preparation, parameter export/face validation and the new Engine preflight fast path.
It is NOT a numerical deviation bound. It can survive unrelated publications through
fresh views. Private ParameterView::Check (line911) rejects stale accepted revisions;
that guard must remain. Do not retain a private view across publication.

Numerical reuse ALREADY exists in MappedFacetReference::FaceMemo. Direct curved
SurfaceError (AffineReference.cpp:2487) keys facet plus binding feature/version/
dimension/u/v and realized coordinate bits. Entity cross-support ParameterError
(BoundaryParameters.cpp:1364) discovers nearby facets, filters exact entity identity,
then BoundPhysicalSurface (AffineReference.cpp:1627) keys the complete ordered facet
list, tolerance and all triangle-coordinate bits. Exact hits reuse a scalar bound.
Both maps share the authority-owned memo; FaceMemo::Reserve (line1059) clears both
maps when insertion would exceed fixed 2^14*EntryBytes capacity (4MiB on this build).
Oversize keys/transient admission denial bypass memo. Cache policy effect is unmeasured:
there are no dedicated FaceMemo hit/miss counters. Prepared facet_hits/misses refer
to a different cache. A union memo hit increments upper_calls without adding upper
query time/work; key/support work still occurs. Never claim numerical reuse is absent.

GeometryCertificates (BoundaryParameters.cpp:368) is another cache: entity-level
SurfaceUnionBounds, revision-keyed for import source enclosures. Its callers are
Import source preparation and final completion. Final complete=true deliberately
recomputes the full two-sided check. It is not the smoothing per-face memo.

Existing complete-census counters are summarized in quality_service_geometry_assessment.json.
In window10, rank2 reports upper152.091s/traversal143.868s and busy163.532s;
several peers report join-call160s. Across ranks facet building0.08169s, scope building
14.296s, support6.108s, traversal242.419s, upper260.460s. These scopes overlap;
NEVER add them or label join-call pure idle. Window12 again has traversal160.400s
inside upper173.489s, facet building0.146s. Boundary smoothing reports private whole-
patch debt23.878 over121.008rank-seconds vs interior94.988 over7.248rank-seconds;
these are private optimizer progress, not actual final published removals. Actual
whole-field removals are independently checked in passive_census_integrity_assessment.json.

Reusable representative query payload/provenance and limitations are verified in
reusable_geometry_query_inventory.json. Same geometry authority, older projected
queries; not exact build188 mesh/association revision/cache warmth. No new baseline,
perf/GDB or replay job was launched. Terminal outcome still drives the next cycle.

Pinned source hashes checked during review:
- Common/src/adaptation/CNativeBoundary3D.cpp: 7a72220c74a4440183f542c27864c88ebee454aed1c1b999a96799b2e0407d90
- Common/src/adaptation/CNativeBoundaryParameters3D.cpp: ef68913e69dea200e6078316a922e18117bf8e6f9a5cb27413c3ca45e62fa468
- Common/src/adaptation/CNativeAffineReference.cpp: 7cec90d82c774f5a170fc5a42386c9718782c386bc9ba4aa702e5b1ce6cde000
- Common/src/adaptation/CNativeEngine3D.cpp: 8bc370a97bc0205e043f56c61c6deb4f3b1a50a13e4cfec3a29480da515ec8e6
