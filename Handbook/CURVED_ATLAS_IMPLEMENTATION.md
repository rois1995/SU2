# Bounded curved atlas: connected implementation design

Implementation contract, 2026-10-27; connected C++ work is underway, not qualified. This expands
[the mobility diagnosis](CURVED_BOUNDARY_MOBILITY.md). The missing capability is
redistribution across artificial original cubic-chart seams, with genuine
features, bodies, carriers, requested heights, transfer and restart preserved.
Quality-stage ordering does not supply this capability. Build19's early capture
replaced seven generated physical points; all seven retained their dimension-one
original-edge feature identities. These are on-chart edge slides, not released
original reference vertices or demonstrated cross-chart freedom/M6 closure.

## Representation: retain the authority, persist the correspondence

Keep `SurfaceBinding` as the forward point location on an original vertex, edge
or facet. Its dimension describes the chart location, not physical mobility.
Keep `SurfaceFace::facet` and `FacetBinding::facet` as an immutable original
anchor used to recover the marker. Do not overload that integer with a transient
atlas identity or change `Canonical` globally. A face's pieces must all have the
anchor's physical marker and body. Containment can no longer infer its support
from that label alone.

Add boundary-only records, first in `Patch`, then in `BoundaryParameterState`:

| Record | Key and contents | Invariant |
|---|---|---|
| `EdgeTrace` | Current endpoint IDs in ascending order; ordered pieces `(original facet, r0, r1, t0, t1)` | Original barycentric points `r0,r1` vary affinely with the **active flat edge** parameter `t`; intervals tile `[0,1]` exactly |
| `FaceCoverage` | Current face in its oriented cyclic order; pieces `(original facet, r[3], b[3])` | `r` is a triangle in original barycentric coordinates; `b` is the corresponding triangle in the active face's barycentric simplex |
| `MobilityPolicy` | Schema/algorithm version, configured angle, marker roles, canonical feature graph and identity | Derived from immutable geometry/configuration; independent of mesh events and worker IDs |

For one coverage piece, with local barycentric coordinate `s`, the prescribed
surface is `P_f(sum(s_i*r_i))`; its corresponding flat point is
`sum(s_i*sum(b_ij*x_j))`. Both maps are explicit. Mere sets of chart IDs or a
pointwise closest binding would omit this correspondence and cannot support
whole-face error or the next source envelope.

Coverage microtriangles have positive exact parameter orientation. They tile the
active barycentric simplex, and their original-chart images tile the assigned
reference region. Their restrictions to each active edge equal its one shared
`EdgeTrace`, including the `t` mapping. Interior refinement knots are geometry
data, never added CFD nodes or sensor interpolation weights.

Normalize trace orientation by endpoint ID, chart-side representations of an
original seam point by original edge ID, and removable subdivision knots by exact
affine equality in both `r` and `t`. Never erase a genuine chart transition.
Two traces describing the same reference path with different active-edge
parameterizations are different contracts. Retained edges reuse their accepted
records; they are not independently reprojected or retraced.

### Exact coordinates are a necessary new primitive

`ParametricPoint` stores binary64 affine weights. The existing exact sign kernel
does not store arbitrary rational clipping intersections. A crossing of two
binary64 segments generally needs a non-dyadic ratio. Rounding that crossing and
using tolerance equality would break the representation above.

Implement a small bounded native rational primitive using signed 32-bit limbs,
with a positive denominator and reduced canonical form. Reuse the arithmetic
scheme of private `Integer<N>` in `CNativePredicates3D.cpp`; checked exact division
and GCD are additional required operations, not existing facilities. Leave the
validated determinant formulas/bounds intact. No rational dependency is presently
used by adaptation; do not silently add Boost or assume arbitrary precision is
free. Provide only conversion from exact binary64, comparison, arithmetic,
normalization and outward interval conversion needed by clipping/certification.

Admit limb storage and maximum intermediate bits before every bounded overlay;
fail explicitly on exhaustion. Payload parsing bounds counts and limb lengths
before allocation, and rejects zero denominators/noncanonical forms. Rational
growth across successive events is a measured resource risk. Bounds must be
qualified on the difficult replay and second event, not raised after a stall.

### Header contract to agree before dividing implementation

The minimal value types below belong with geometry; accepted maps, publication
and transport belong with parameters. Names are proposed, not existing APIs.
`ExactBary` stores two independent weights; its third is exactly `1-u-v`.

```cpp
struct ExactRatio {
  bool negative = false;
  std::vector<uint32_t> numerator, denominator; // Least-significant limb first.
};
struct ExactBary { ExactRatio u, v; };
struct TracePiece {
  Id facet;
  ExactBary first, last;
  ExactRatio end; // Active-edge t; first t is previous end, initially zero.
};
struct EdgeTrace { std::vector<TracePiece> pieces; };
struct CoveragePiece {
  Id facet;
  std::array<ExactBary, 3> reference, active;
};
struct FaceCoverage { std::vector<CoveragePiece> pieces; };
struct BoundaryCoverage {
  std::map<Edge, EdgeTrace> traces;      // Sorted current endpoint IDs.
  std::map<Face, FaceCoverage> faces;    // Oriented cyclic FaceOrder.
};
struct AtlasBudget {
  size_t maximum_work, maximum_bytes, maximum_integer_bits;
  size_t work = 0, peak_bytes = 0;
};
```

Canonical zero is empty numerator, denominator `{1}`, nonnegative sign. Other
integers have no high zero limbs; numerator/denominator are coprime. Geometry
operations receive one `AtlasBudget&` shared across the complete transaction,
including clipping and certificate subdivision. Limits come from the caller's
admitted geometry reservation and remaining stage resources, with no new user
configuration switch. The integer-bit cap must bound products **before** their
allocation; the byte cap includes retained rationals, temporaries and transport.
Neither cap is inferred solely from the number of CFD cells.

The geometry module needs these operations, with explicit output-on-success
semantics matching existing kernels:

```cpp
// Authority + immutable mobility policy are shared const state.
bool ResolveSingleChart(const SurfaceFace&, const BoundaryPoints&,
                        FaceCoverage&, AtlasBudget&, std::string&) const;
bool ValidateCoverage(const SurfaceFace&, const BoundaryPoints&,
                      const FaceCoverage&, const BoundaryCoverage&,
                      AtlasBudget&, std::string&) const;
bool PrepareAtlas(const Patch&, const BoundaryCoverage&, Atlas&,
                  AtlasBudget&, std::string&) const;
bool PrepareMovedSurface(const Atlas&, Id old_vertex, const BoundaryPoint& moved,
                         std::vector<SurfaceFace>&, BoundaryCoverage&,
                         AtlasBudget&, std::string&) const;
double SurfaceError(const FaceCoverage&, const std::array<Point,3>&) const;
SourcePatch PrepareSourcePatch(const FaceCoverage&,
                               const std::array<Point,3>&) const;
```

`Atlas` is a transaction-local certified common refinement, its exact forward
maps and inherited perimeter, not a persisted global chart. Production proposal
code first quantizes a target to the existing binary64 `SurfaceBinding`, then
maps that exact binding back into the atlas. `PrepareMovedSurface` must use that
actual endpoint; rational coverage must not retain the pre-quantization target.
The moved point's complete physical star is replaced together. Shared type/API
declarations can forward-declare `Patch`/`BoundaryPoints` where required to avoid
turning the immutable geometry header into a volume-state owner.

Store `BoundaryCoverage` sparsely. An absent face record authorizes only the
existing strict single-chart proof using its anchor and all three bindings;
`ResolveSingleChart` derives that identity coverage. An absent trace can likewise
derive its unique direct chart segment from an admitted incident single-chart
face, canonicalizing an original-edge segment to the original edge identity.
Cross-chart records are mandatory. This is a compact legacy representation,
not a fallback accepting a face on component membership. Any explicit cover's
edge restriction must equal the accepted explicit or derived trace. Preserve
existing v6 planar-component semantics separately; do not reinterpret a legacy
component as a curved atlas.

Add this sparse bundle to `Patch` and the parameter view. `ExportParameters`
exports the records required by its surface; `PrivateParameters` overlays them;
`Prepare` stages their changed map nodes alongside existing point/face nodes.
Provide `ResolveCoverage(face)` and `ResolveTrace(edge, incident_face)` in the
parameter layer so immutable geometry never owns current mesh IDs. Existing
`PlanarReference::ContainsFace`/`SurfaceError` signatures can resolve current IDs
through `Parameter(point)` and dispatch to these records. Query cache keys must
include policy/descriptor identity; coordinate-only memoization is insufficient.

Do not put variable vectors into `World::exchange` records: its `T{}` field walk
assumes a fixed record size. Encode descriptors to a measured word stream with
`RecordStream`, then transport fixed pages containing source rank, revision,
sequence, valid-word count and a fixed word array. Reassemble each source's
contiguous pages after collective count/byte admission, reject missing/duplicate
pages, then parse bounded record/limb counts. Existing one-proposal-per-worker
publication needs no new batch abstraction. Transport boundary deltas only.

Each `RecordStream` scalar occupies eight bytes, including a 32-bit limb. Encode
a rational as three scalar words `(sign, numerator_count, denominator_count)`,
then numerator limbs and denominator limbs; limb words must fit 32 bits. Counts,
total word/limb lengths, schema and policy identity are explicit block headers.
Snapshot/payload sizing uses the same encoder, not `sizeof` or the old fixed
`40 + 72*np + 32*nf` expression. A text v8 sidecar may use canonical integer
tokens, but it must decode to the same reduced values and enforce the same
pre-allocation limits. No change to v1-v7 text or old binary payload decoding is
implied by the new schema.

## One transaction, including inherited edge paths

1. Import/reserve the complete admitted volume star or coordinated region using
   the current Directory protocol. Export its accepted point, trace and face
   records. Cancel internal coverage edges to obtain its reference domain and
   retained perimeter. Treat physical feature chains as boundaries between
   smooth sheets; do not combine disconnected bodies by marker.
2. Build a common refinement of the **old accepted coverage**, original chart
   cuts and retained traces. It need not include entire original facets. Each
   sheet region must have certified disk topology for this atlas construction;
   holes/cuts must remain explicit rather than being filled by a convex hull.
3. Propose a local PL flattening of this refinement. Put retained perimeter CFD
   vertices on a deterministic convex polygon. Place every inherited trace knot
   at `(1-t)*A+t*B` on its perimeter edge using its stored exact `t`. Interior
   coordinates may be numerical proposals converted to exact dyadics. Certify a
   simple boundary and strictly positive exact signed area of every refined
   triangle. Those checks certify the PL domain map; harmonic/mean-value
   coordinates would only propose the interior coordinates. No physical cubic
   chart boundary is presumed straight under projection.
4. Choose smooth-point targets in this domain, then choose a representable
   forward original-chart binding and realize it with existing mapping-error
   checks. Recompute its exact domain position from that binding. A true curve
   sampling point uses its canonical chain parameter; a branch/corner is fixed.
   Two incident sheets share the same curve parameter/path. A carrier junction
   retains its simultaneous carrier constraints.
5. Retriangulate the admitted sheet domain with unchanged perimeter and physical
   constraints. Intersect the new active triangles with the refined atlas cells
   using rational clipping. Each intersection yields both original parameters
   and active-face barycentrics. Emit coverage microtriangles and shared traces;
   prove positive tiling and exact equality to the old covered reference region.
6. Rebuild the complete volume star, retaining exact-positive nondegenerate
   cells. Geometry/source, contact and current height admission still apply.
   Construction performs no q/J ranking. Quality/BL improvement and final strict
   gates retain their existing phase responsibilities.
7. Prepare and publish mesh, points, traces and covers atomically at one common
   revision. Rejected preparation leaves all accepted maps unchanged. A later
   adjacent transaction imports these same traces and can choose another local
   atlas without changing them. Independent transactions cannot invent two paths
   for a retained shared edge.

No global flattening of a wing or sphere is needed. Local atlas rejection is a
diagnosis, not permission for unconstrained projection or another local fallback.
Whether these admitted regions/certificates cover the difficult M6 support is an
implementation acceptance question.

## Real cubic injectivity: the central additional certificate

A positive PL flattening does **not** certify the image of the glued PN surface.
Neither watertight edge controls nor each original chart's positive Jacobian
establishes global injectivity. `ClosestCurved` does not certify unique projection;
`SourcePatch` hull bounds alone do not prove injectivity either.

Implement the following sufficient certificate for each admitted smooth-sheet
disk, including a same-sheet collar where its interface meets untouched charts:

1. Propose one common projection normal `n` and a consistent planar basis. On
   every covered cubic piece, prove `n dot (P_u cross P_v) > 0`, with the correct
   parameter orientation, using restricted `CubicNormalControls` and outward
   Bernstein subdivision. This is strict geometric positivity, with no q/J
   threshold. A corner-normal comparison is insufficient.
2. Prove the projected complete perimeter trace is a Jordan curve. Each trace
   segment is a cubic Bezier arc, not its endpoint chord. Subdivide into arcs
   whose derivative has strictly positive projection on a certified direction;
   each is then injective. Exclude all nonadjacent arc pairs with control-hull
   separation. Adjacent arcs must meet only at their declared endpoint: factor
   the displacement from that endpoint and certify separated outgoing tangent
   cones. Ordinary disjoint-box tests cannot settle their shared endpoint.
3. Establish local degree one across C0 chart glue. Interior chart points use
   the inverse function theorem. On an original seam, identical shared cubic
   controls give the same nonzero tangential derivative; consistent facet
   orientation and positive projected Jacobians place the two chart interiors
   on opposite local sides. At every interior original vertex, certify that
   projected one-sided tangent rays form exactly one cyclic fan, using exact
   ray ordering/winding and interval sector bounds in a sufficiently small
   neighborhood. Reject a branched/multiple-winding fan. Artificial subdivision
   seams inside one cubic chart do not create a new C0 glue issue.
4. These local checks plus the simple, consistently oriented perimeter give a
   proper degree-one map of the disk to its projected Jordan domain. Every
   interior preimage contributes positive degree one; there can be neither an
   uncovered point nor multiple preimages. Thus the projected map, and hence
   the real cubic union, is injective. The implementation needs this stated
   C0 edge/vertex proof and its checked hypotheses; a PL bijection theorem alone
   does not cover it.

All subdivision, pair work and storage are bounded. Failure to prove any
hypothesis refuses the certificate. Tangential cusps or unresolved cones cannot
be accepted from sampled normals. Numerical success of these certificates on M6
has not been established; that is a production blocker, not a deferred audit.

Separate this local embedding proof from body/contact clearance. Query complete
immutable control-hull bounds across **all** bodies/markers, not just the
same-marker `ClosestCurved` component. Reuse hull separation arithmetic, extending
it to cubic-piece pairs with subdivision where needed. Permit only declared
common edges/features; include adjacent untouched geometry in the collar proof.
Retain Directory clearance for the actual new flat mesh and the existing full
source-envelope separation checks. None of these certificates replaces another.

## Geometry features and persistence

Build the feature graph from original adjacency, `ADAP_ANGLE`, configuration
marker roles, affine carriers and immutable controls. Reuse `BuildOptionA`'s
configured inputs and identity discipline, **not** its convex-only D1 segment
selection: concave sharp creases and physical interfaces remain geometric
constraints even when D1 or the BL metric is disabled. Bound one-sided normal
variation over the whole cubic edge; uncertain edges stay protected. A reflective
carrier is a D1 continuation but still constrains its wall intersection. Curve
chain samples may slide through artificial original vertices; genuine branches
and corners remain fixed.

Record this policy separately from the immutable control geometry version.
Bindings retain the original geometry version; certificate/cache identities also
include the mobility policy and current trace/coverage revision. Never rebuild
the controls from redistributed coordinates or the current sensor field.

Use an explicit new reference format (v8) and parameter-payload schema for policy,
traces and coverage. Keep v1-v7 parsing and unchanged legacy encoding semantics.
A legacy v7 sidecar remains facet-local until an explicit production setup step,
with the actual configuration available, derives/agrees the policy and seeds
identity single-chart coverage. Do this before worker/reference cloning so parent
and worker authorities agree. No marker-name inference or silent upgrade during
`DecodeReference` is allowed. A v8 configuration-policy mismatch must refuse or
require an explicit new policy preparation, never silently retain another angle.

`Rebind` changes current point/edge/face IDs only. If an edge's new endpoint order
reverses, reverse its pieces and replace `t` by exact `1-t`. If a face's canonical
cyclic order rotates, permute its active barycentric coordinates. Original
parameters, feature identity and cubic controls do not change. Persistence must
restore identical traces before the next adaptation/source query.

## Minimal connected interfaces and owners

| Owner/files | Required connected change |
|---|---|
| Geometry: `CNativeAffineReference.hpp/cpp`, exact predicate internal helper | Exact rational coordinates, coverage/trace value types; PL overlay and real-union certificate; semantic feature graph; coverage-aware containment/error/source restrictions |
| Parameters: `CNativeBoundaryParameters3D.hpp/cpp` | Sparse `Patch`/accepted maps and private overlays; bind/propose by semantic mobility; export complete covers/traces; staged counts/rollback; payload, snapshots, renumbering and actual byte admission |
| Publication: `CNativeDistributed3D.cpp` | Transport boundary cover/trace edits alongside existing point/face edits; prepare all at the common revision before allocation-free commit; include descriptor bytes in admission and receipts |
| Operators: `CNativeBoundary3D.hpp/cpp` | `ProjectMovePoint`, `ConstrainMovePoint`, `MoveVertex`, `CheckSurface`/`Finish`; bind complete moved faces through the atlas; preserve descriptors in split/collapse/reconnect paths and retain first-height apex constraints |
| Lifecycle: `CNativeEngine3D.cpp` | `GeometryPatch`/optimizer assembly and commit must retain descriptors; `SmoothBoundary` uses semantic freedoms; geometry census uses explicit covers; keep construction/quality separation and final gates |
| Source/return: `CNativeImport3D.cpp` | `PrepareSourcePatch` resolves each active face's cover; source envelopes include every restricted cubic piece and its corresponding flat subtriangle; parent parameter return/reader rebinding account for new payload/limbs |
| Persistence/setup: `CNativeReferenceIO3D.cpp`, retained geometry setup, `CNativeMetricFamily3D` | Versioned bounded v8 decode/encode; collective policy preparation from configuration roles/angle; compatibility identity independent of D1 enablement |
| Evidence helpers/tests | Extend existing parameter and transaction serializers/replay readers, including `fixtures/native_transaction_capture.inc`; do not omit new maps from captures or memory estimates |

Keep the present single-chart methods for legacy callers. Add coverage-aware
overloads in immutable geometry; route active mapped-face queries through the
parameter view's explicit descriptor lookup. Candidate operators must install
the descriptor in their private overlay before `Finish`/error checks. A missing
descriptor must refuse; component membership is not an implicit certificate.
`SurfaceFace::facet` continues to identify a valid original marker anchor.

`Prepare` generalizes its current facet-edge interval check to oriented piece
boundaries in each original parameter triangle. Normalize collinear segments and
compare exact signed interval sums, allowing subdivisions; old/new 2-chain
boundaries must cancel. Also check positive active-simplex tiling and shared
trace restrictions. Track trace incidence/refcounts so retained edges cannot
change without retiring all incident faces. Full surface audit re-establishes
coverage of original charts using pieces rather than requiring each active face
to remain in one chart.

`SourcePatch` can remain the caller's existing aggregate type, with a bounded list
of restricted pieces internally. Each piece stores outward cubic control bounds
and bounds of its corresponding exact flat subtriangle; retain the parent flat
face's normal for outward displacement. `Bounds` unions, displacement takes the
maximum, and `Separated` requires every piece to pass. Reuse existing shell and
source-cell coverage/positivity checks. The common edge `t` contract makes the
flat-to-reference correspondence continuous across neighboring faces; pointwise
distance bounds alone do not provide that fact.

Parallel implementation can split geometry/exact arithmetic, parameter storage
and persistence after agreeing these value types and schema. The primary owner
then integrates operator/lifecycle/source callers. Do not run a build while any
owned source edits continue. Integrate one complete publish/return/source path
before a broad campaign; no branch counts as validated cross-chart mobility.

## Acceptance before claiming the capability

Reuse the mapped component/publication, curved mapped, move-constraint, source
and distributed rollback fixtures. Add only the connected missing cases:

- Exact non-dyadic intersections; mathematically identical cuts from different
  operation orders; reversed IDs; rational exhaustion/corrupt payload refusal.
- Smooth cubic original vertex and seam-point crossing with nonzero motion;
  identical inherited edge paths after two adjacent transactions using different
  atlases and different triangulations. Check active-edge `t`, not just knots.
- Same-marker sharp/concave features, curve-chain sampling motion, branch/corner
  protection and affine carrier intersection; unchanged immutable hashes.
- Projection fold, multiwinding vertex fan, non-Jordan cubic perimeter, hole and
  nearby same-marker body rejection. Include a case whose corner chords look
  admissible while its real cubic boundary is not.
- Serial and bounded MPI publication/rollback, ownership permutations, empty
  ranks, common-revision failure, contact veto, memory refusal and parent return.
- Persist/rebind/restart the redistributed mesh, reconstruct its full source
  envelope, adapt again against the original frozen field, and verify transfer.
- Saved difficult M6 midpoint cavity and contrasting captured support, then full
  M6 Euler and RANS under unchanged q/J/size/geometry/contact/height/complexity and
  transfer/BDF/restart gates. Local certificates or improved early debt totals
  do not establish this final accepted workflow.

The exact-coordinate primitive now passes four standalone cases/308 assertions
with unchanged pins (`staged57/atlas_arithmetic1`). Geometry/parameter storage and
publication/persistence implementation is underway. The approved remapper uses
per-sheet convex PL flattening, with the existing numerical matrix solver only
proposing interior coordinates; exact positivity certifies the result. Curve
sampling motion shares its exact parameter across incident sheets. Full operator,
source and lifecycle integration, C0 certificate qualification and resource
sufficiency on actual M6 support remain open.

Current integration review found and corrected a budget-lifetime defect: a later
smaller admitted workspace was rejected merely because an earlier query had a
larger historical peak. Current allocations remain checked against the current
window; the historical receipt remains cumulative. A focused exact-coordinate
regression was added, awaiting the connected build. The earlier standalone
arithmetic receipt qualifies the preceding primitive checkpoint, not this edit.

Build23 completed in939.91s with944 unchanged pins. First focused atlas_checks
(`staged57/cases/atlas_checks_20261027T115717_2061590`) is terminal:23 cases,
19 passed/4 failed,4040/4045 assertions,6.57s, pins unchanged. New remapper motion
and second-event fixtures passed; whole capability remains open. Failures exposed
archive scratch omission and false feature pinning from correlated edge-normal
intervals. The purported disconnected negative fixture was a valid notched disk;
it must use genuinely disconnected support. Corrections are underway.

Build24 PASS93.16s; first archive correction and edge-normal fix left two focused
failures. Build25 diagnostic-only PASS57.14s identified EncodeReference's decode
self-check storage bound and publication's arithmetic exhaustion. Build26
PASS39.13s included the corrected bound and certified nearby-double proposal;
focused23/24 cases passed. Build27 PASS33.08s includes dyadic coprime fast-path
validation; focused24/25 cases pass4202/4203 assertions in9.54s, unchanged pins.
The remaining fixture reaches publication after successful SplitEdge; debugger
`publication_work_debug2.log` locates cumulative1M-work exhaustion in the exact
old/new2-chain check. One fixture budget spans setup, split and repeated rejection/
publication transactions; reservation ownership is being corrected without
raising a production limit. Every failed receipt is retained under `staged57`.

Build28 PASS78.17s/944 unchanged pins. Focused atlas_checks
`staged57/cases/atlas_checks_20261027T121747_2119047` PASS9.48s:25 cases,
4261 assertions, pins unchanged. Independent fixture transactions now own separate
unchanged1M-work reservations; earlier attempted-work receipts remain asserted.
No production cap was raised. Certified nearby-double bounds and canonical-dyadic
GCD shortcuts reduce actual arithmetic work without skipping validation. Full
M6 diagnostic/session51670 is running on six ranks; acceptance remains open.
